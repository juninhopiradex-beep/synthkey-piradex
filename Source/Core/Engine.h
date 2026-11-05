// =============================================================================
//  SYNTHKEY by Piradex — motor de síntese (vozes, matriz de modulação, efeitos)
//
//  Cada voz tem 2 motores (A e B): Analógico (seno/triângulo/serra/pulso, sync, FM,
//  dobra), Wavetable (16 tabelas × 32 frames, warp, dobra) ou Sample (instrumentos
//  dos packs). Depois: sub-oscilador, ruído, 2 filtros com routing, 3 envelopes,
//  2 LFOs, 8 macros e uma matriz de 16 ligações. À saída: 2 inserts (3 efeitos cada),
//  um aux com envios, volume e limitador.
//
//  O motor só lê o vetor de parâmetros "reais" (float p[P_COUNT]); não depende do JUCE.
// =============================================================================
#pragma once
#include "ParamTable.h"
#include "Dsp.h"
#include "Wavetables.h"
#include "Spectral.h"
#include "Fx.h"
#include "Samples.h"

namespace sk
{
// Qualidade "Auto": liga 2x só quando o patch usa dobra forte ou sync alto no motor Analógico (os casos em que o modo Normal alias de forma audível:
// dobra 100% na nota 93 ≈ −22 dB em Normal vs −44 dB a 2x; sync 24 st na nota 93 ≈ −13 dB vs −47 dB). Histerese para não oscilar com o botão.
inline int autoQuality (const float* p, bool currentlyOn)
{
    auto needs = [&] (bool on, int type, float fold, float sync) { return on && type == 0 && (currentlyOn ? (fold > 0.20f || sync >= 8.0f) : (fold > 0.30f || sync >= 12.0f)); };
    return (needs (p[P_A_ON] > 0.5f, (int) p[P_A_TYPE], p[P_A_FOLD], p[P_A_SYNC]) || needs (p[P_B_ON] > 0.5f, (int) p[P_B_TYPE], p[P_B_FOLD], p[P_B_SYNC])) ? 1 : 0;
}
inline int resolveQuality (const float* p, bool currentlyOn) { const int q = (int) p[P_QUALITY]; return q >= 3 ? autoQuality (p, currentlyOn) : std::max (0, std::min (2, q)); }

class Engine
{
public:
    static constexpr int kMaxVoices = 40;
    static constexpr int kSub = 32;
    static constexpr int kMaxUni = 8;

    // sampleRate / maxBlock são os do anfitrião; com oversampling o motor corre a sampleRate * factor
    void prepare (double sampleRate, int maxBlock)
    {
        hostSr = sampleRate; hostMax = std::max (maxBlock, 1);
        prepareCore (sampleRate * (double) factor, std::max (maxBlock, 1) * factor);
        buildDecimator();
    }
    // 0 = normal, 1 = 2x, 2 = 4x (reinicia as vozes)
    void setQuality (int q)
    {
        const int f = q >= 2 ? 4 : (q == 1 ? 2 : 1);
        if (f == factor) return;
        factor = f; if (hostSr > 0) prepare (hostSr, hostMax);
    }
    int quality() const { return factor == 4 ? 2 : (factor == 2 ? 1 : 0); }

    void prepareCore (double sampleRate, int maxBlock)
    {
        sr = (float) sampleRate; maxN = std::max (maxBlock, kSub);
        wt = &Wavetables::get();
        for (auto& v : voices) { v.f1.prepare (sr); v.f2.prepare (sr); v.amp.setSampleRate (sr); v.e2.setSampleRate (sr); v.e3.setSampleRate (sr); }
        for (auto* b : { &busL, &busR, &vsL, &vsR, &aL, &aR, &bL, &bR, &xL, &xR }) b->assign ((size_t) maxN, 0.0f);
        ksSize = (int) (sr / 16.0f) + 16; ksBuf.assign ((size_t) kMaxVoices * 2 * (size_t) ksSize, 0.0f);
        for (auto& f : fx) f.prepare (sr, maxN);
        lim.prepare (sr);
        masterGain.setTime (sr, 15.0f);
        reset();
    }
    void reset()
    {
        for (auto& v : voices) { v.active = false; v.f1.reset(); v.f2.reset(); }
        for (auto& f : fx) f.reset();
        lim.reset(); stack.clear(); ageCounter = 0; sustain = false; lastNote = -1; rrCounter[0] = rrCounter[1] = 0; rngGlobal.seed (0xA5F1523Du);   // determinismo: mesma predefinição, mesmo som
        for (auto* b : { &busL, &busR, &vsL, &vsR }) std::fill (b->begin(), b->end(), 0.0f);
        masterGain.snap (1.0f); peakL = peakR = 0;
    }
    void setParams (const float* p) { prm = p; }
    UserWavetable& userWavetable() { return userWt; }
    void setLibrary (const Library* l) { if (l != lib) { lib = l; panic(); for (auto& v : voices) { v.zone[0] = v.zone[1] = nullptr; v.sd[0] = v.sd[1] = nullptr; v.instr[0] = v.instr[1] = nullptr; } } }
    void setBpm (double b) { bpm = (float) std::max (20.0, std::min (400.0, b)); }
    // posição do projeto no início do próximo render (em tempos/"ppq") e se o transporte está a andar — alinha os loops sincronizados
    void setTransport (double ppq, bool playing) { hostPpq = ppq; hostPlaying = playing; }
    // fase (0..1) do último loop sincronizado a tocar, para a interface
    float loopPhase() const { return loopPh.load (std::memory_order_relaxed); }
    float loopBpmSrc() const { return loopSrc.load (std::memory_order_relaxed); }
    bool loopStretched() const { return loopSt.load (std::memory_order_relaxed); }
    void setModWheel (float v) { modWheel = v; }
    void setAftertouch (float v) { aftertouch = v; }
    void setPitchBend (float v) { pitchBend = v; }
    // expressão por nota (MPE: curvatura −1..1, pressão 0..1, slide 0..1; aftertouch polifónico usa a pressão)
    void setNoteBend (int note, float v)  { if (note >= 0 && note < 128) exPb[note] = clampf (v, -1.0f, 1.0f); }
    void setNotePressure (int note, float v) { if (note >= 0 && note < 128) exPress[note] = clampf (v, 0.0f, 1.0f); }
    void setNoteSlide (int note, float v) { if (note >= 0 && note < 128) exSlide[note] = clampf (v, 0.0f, 1.0f); }
    void clearNoteExpression() { for (int i = 0; i < 128; ++i) exPb[i] = exPress[i] = exSlide[i] = 0.0f; }
    void setSustain (bool on)
    {
        sustain = on;
        if (! on) for (auto& v : voices) if (v.active && v.held) { v.held = false; releaseVoice (v); }
    }
    int latencySamples() const { return (lim.latency() + (factor > 1 ? (kTaps[factor == 4 ? 1 : 0] - 1) / 2 : 0) + factor - 1) / factor; }
    int activeVoices() const { int n = 0; for (auto& v : voices) if (v.active && ! v.stolen) ++n; return n; }
    float getPeakL() const { return peakL; }
    float getPeakR() const { return peakR; }

    // ---------------------------- notas ---------------------------------------------
    void noteOn (int note, float vel)
    {
        vel = clampf (vel, 0.0f, 1.0f);
        for (auto& f : fx) f.noteOn();                    // gatilho do Sidechain (modo Notas)
        const int mode = (int) prm[P_VOICE_MODE];       // 0 poli, 1 mono, 2 legato
        const bool anyHeld = ! stack.empty();
        stack.push_back ({ note, vel });
        if (mode == 0) { startVoice (allocate(), note, vel, false); lastNote = note; return; }
        Voice* mv = nullptr;
        for (auto& v : voices) if (v.active && v.mono && ! v.stolen) { mv = &v; break; }
        if (! mv) startVoice (allocate(), note, vel, true);
        else if (mode == 2 && anyHeld && mv->gate) { mv->tgtNote = (float) note; mv->note = note; mv->vel = vel; }   // legato: só muda a altura
        else startVoice (mv, note, vel, true);
        lastNote = note;
    }
    void noteOff (int note)
    {
        const int mode = (int) prm[P_VOICE_MODE];
        for (size_t i = 0; i < stack.size(); ++i) if (stack[i].note == note) { stack.erase (stack.begin() + (long) i); break; }
        if (mode != 0)
        {
            for (auto& v : voices)
                if (v.active && v.mono && ! v.stolen)
                {
                    if (! stack.empty())
                    {
                        const auto back = stack.back();
                        if (mode == 1) startVoice (&v, back.note, back.vel, true); else { v.tgtNote = (float) back.note; v.note = back.note; }
                    }
                    else if (sustain) v.held = true; else releaseVoice (v);
                }
            return;
        }
        for (auto& v : voices)
            if (v.active && v.gate && v.note == note && ! v.stolen)
            {
                if (sustain) v.held = true; else releaseVoice (v);
            }
    }
    void allNotesOff()
    {
        stack.clear();
        for (auto& v : voices) if (v.active) { v.held = false; releaseVoice (v); }
    }
    void panic() { clearNoteExpression(); stack.clear(); for (auto& v : voices) { v.active = false; v.f1.reset(); v.f2.reset(); } }

    // ---------------------------- áudio -----------------------------------------------
    void render (float* L, float* R, int n)
    {
        renderPpq = hostPpq;
        if (factor == 1) { renderCore (L, R, n); return; }
        int done = 0;
        while (done < n)
        {
            const int k = std::min (n - done, hostMax);
            renderCore (osL.data() + (kTapsMax - 1), osR.data() + (kTapsMax - 1), k * factor);
            decimate (L + done, R + done, k);
            done += k;
        }
    }

    void renderCore (float* L, float* R, int n)
    {
        n = std::min (n, maxN); loopSeen = false; chainPpq = renderPpq;
        updateSpectral();
        std::fill (busL.begin(), busL.begin() + n, 0.0f); std::fill (busR.begin(), busR.begin() + n, 0.0f);
        std::fill (vsL.begin(), vsL.begin() + n, 0.0f); std::fill (vsR.begin(), vsR.begin() + n, 0.0f);
        int pos = 0;
        while (pos < n)
        {
            const int m = std::min (kSub, n - pos);
            subPpq = renderPpq; sendAcc[0] = sendAcc[1] = sendAcc[2] = 0.0f; sendW = 0.0f;
            vsOutL = vsL.data() + pos; vsOutR = vsR.data() + pos;
            for (auto& v : voices) if (v.active) renderVoice (v, m, busL.data() + pos, busR.data() + pos);
            if (sendW > 1.0e-9f) globalDst[MD_AUXRET] = sendAcc[2] / sendW;     // o retorno é global (média ponderada); os envios A/B são por voz (vsL/vsR)
            renderPpq += (double) m * (double) bpm / (60.0 * (double) sr);
            pos += m;
        }
        // ---- inserts, aux, volume, limitador ----
        const int route = (int) prm[P_OUT_BUS];
        const float inA = route == 0 ? 1.0f : (route == 2 ? 0.5f : 0.0f), inB = route == 1 ? 1.0f : (route == 2 ? 0.5f : 0.0f);
        for (int i = 0; i < n; ++i) { aL[(size_t) i] = busL[(size_t) i] * inA; aR[(size_t) i] = busR[(size_t) i] * inA; bL[(size_t) i] = busL[(size_t) i] * inB; bR[(size_t) i] = busR[(size_t) i] * inB; }
        runChain (0, aL.data(), aR.data(), n); runChain (3, bL.data(), bR.data(), n);
        const float lvA = dbToGain (prm[P_INSA_LEVEL]), lvB = dbToGain (prm[P_INSB_LEVEL]);
        const float sendA = clampf (prm[P_SEND_A], 0.0f, 1.0f), sendB = clampf (prm[P_SEND_B], 0.0f, 1.0f);
        const float ret = dbToGain (prm[P_AUX_RETURN]) * clampf (1.0f + globalDst[MD_AUXRET], 0.0f, 2.0f);
        for (int i = 0; i < n; ++i) { aL[(size_t) i] *= lvA; aR[(size_t) i] *= lvA; bL[(size_t) i] *= lvB; bR[(size_t) i] *= lvB; xL[(size_t) i] = aL[(size_t) i] * sendA + bL[(size_t) i] * sendB + vsL[(size_t) i]; xR[(size_t) i] = aR[(size_t) i] * sendA + bR[(size_t) i] * sendB + vsR[(size_t) i]; }
        runChain (6, xL.data(), xR.data(), n);
        masterGain.tgt = dbToGain (prm[P_MASTER_VOL]);
        for (int i = 0; i < n; ++i)
        {
            const float g = masterGain.tick();
            L[i] = (aL[(size_t) i] + bL[(size_t) i] + xL[(size_t) i] * ret) * g;
            R[i] = (aR[(size_t) i] + bR[(size_t) i] + xR[(size_t) i] * ret) * g;
        }
        if (! loopSeen) loopSrc.store (0.0f, std::memory_order_relaxed);
        lim.process (L, R, n, prm[P_LIM_CEIL], prm[P_LIM_ON] > 0.5f);
        float pl = 0, pr = 0; for (int i = 0; i < n; ++i) { pl = std::max (pl, std::fabs (L[i])); pr = std::max (pr, std::fabs (R[i])); }
        peakL = std::max (pl, peakL * 0.92f); peakR = std::max (pr, peakR * 0.92f);
    }

private:
    // ---- oversampling: decimação FIR (Kaiser) linear-fase ----
    static constexpr int kTaps[2] = { 191, 383 }, kTapsMax = 383;   // Kaiser ~90 dB: a banda de paragem começa em Nyquist do anfitrião (auditoria C-05)
    static double bessel0 (double x) { double s = 1, t = 1; for (int k = 1; k < 30; ++k) { t *= (x / (2.0 * k)) * (x / (2.0 * k)); s += t; } return s; }
    void buildDecimator()
    {
        if (factor == 1) { osL.clear(); osR.clear(); return; }
        taps = kTaps[factor == 4 ? 1 : 0]; hfir.assign ((size_t) taps, 0.0f);
        const double beta = 9.0, mid = (taps - 1) * 0.5; double sum = 0;
        const double dF = (82.0 / (2.285 * (double) (taps - 1))) / (2.0 * 3.14159265358979323846);   // largura de transição (fração da taxa sobre-amostrada)
        const double fcN = 0.5 / factor - 0.5 * dF;
        std::vector<double> h ((size_t) taps);
        for (int i = 0; i < taps; ++i)
        {
            const double x = i - mid, sinc = x == 0 ? 2 * fcN : std::sin (2 * 3.14159265358979323846 * fcN * x) / (3.14159265358979323846 * x);
            const double r = x / mid, w = bessel0 (beta * std::sqrt (std::max (0.0, 1.0 - r * r))) / bessel0 (beta);
            h[(size_t) i] = sinc * w; sum += h[(size_t) i];
        }
        for (int i = 0; i < taps; ++i) hfir[(size_t) i] = (float) (h[(size_t) i] / sum);
        const size_t len = (size_t) (kTapsMax - 1) + (size_t) hostMax * (size_t) factor + 8;
        osL.assign (len, 0.0f); osR.assign (len, 0.0f);
    }
    // osL/osR guardam [histórico (kTapsMax-1) | novo bloco]; a saída j usa as amostras [j*f, j*f+taps)
    void decimate (float* L, float* R, int k)
    {
        const int hist = kTapsMax - 1, total = k * factor, off = hist - (taps - 1);
        for (int j = 0; j < k; ++j)
        {
            const float* a = osL.data() + off + j * factor; const float* b = osR.data() + off + j * factor; float yl = 0, yr = 0;
            for (int t = 0; t < taps; ++t) { yl += hfir[(size_t) t] * a[t]; yr += hfir[(size_t) t] * b[t]; }
            L[j] = yl; R[j] = yr;
        }
        std::memmove (osL.data(), osL.data() + total, (size_t) hist * 4); std::memmove (osR.data(), osR.data() + total, (size_t) hist * 4);
    }
    int factor = 1, taps = 47, hostMax = 512; double hostSr = 0; std::vector<float> hfir, osL, osR;

    struct Held { int note; float vel; };
    struct Voice
    {
        bool active = false, gate = false, stolen = false, held = false, mono = false;
        int note = 60; float vel = 0.8f; uint64_t age = 0;
        float curNote = 60, tgtNote = 60;
        Adsr amp, e2, e3; Lfo l1, l2; float lfoT = 0, rnd = 0;
        float stepBeat = 0, stepSm = 0, drawPh = 0, menvT = 0, menvRelT = 0, menvRelLevel = 0, menvLast = 0; bool menvRel = false;
        VoiceFilter f1, f2;
        float ph[2][kMaxUni] = {}, ph2[2][kMaxUni] = {}, pm[2][kMaxUni] = {};
        float syncD[2][kMaxUni] = {}, fprev[2][kMaxUni] = {};   // salto da sync (polyBLEP) e amostra anterior da dobra (ADAA)
        bool startDone[2] = { false, false };                  // ponto de arranque do sample já aplicado (MD_START)
        double sp[2][kMaxUni] = {};
        float ksLp[2] = {}; int ksW[2] = {};                    // motor Corda: filtro de perdas e posição de escrita
        struct Grain { double pos = 0; float step = 1, pan = 0; int age = 0, len = 0; bool on = false; };
        static constexpr int kMaxGrains = 12;
        Grain gr[2][kMaxGrains]; float grAcc[2] = {}; double grHead[2] = {}; Rng grRng[2];   // motor Granular
        const Zone* zone[2] = { nullptr, nullptr };
        const SampleData* sd[2] = { nullptr, nullptr };
        const Instrument* instr[2] = { nullptr, nullptr };
        double lbeat[2] = { 0.0, 0.0 };                      // posição (em tempos) dos loops sincronizados quando o projeto está parado
        float subPh = 0; NoiseGen noise; float prevGain = 0, fade = 0;
    };

    float sr = 44100.0f, bpm = 120.0f; int maxN = 512;
    double hostPpq = 0, renderPpq = 0, subPpq = 0; bool hostPlaying = false;
    std::atomic<float> loopPh { 0.0f }, loopSrc { 0.0f }; std::atomic<bool> loopSt { false }; bool loopSeen = false;
    UserWavetable userWt;
    SpectralBank spBank[2];
    const float* prm = nullptr; const Wavetables* wt = nullptr; const Library* lib = nullptr;
    Voice voices[kMaxVoices];
    std::vector<float> ksBuf;                                // kMaxVoices*2 linhas de atraso (Karplus-Strong)
    int ksSize = 0;
    float* ksLine (int vi, int E) { return ksBuf.data() + ((size_t) vi * 2 + (size_t) E) * (size_t) ksSize; }
    std::vector<Held> stack; uint64_t ageCounter = 0; bool sustain = false; int lastNote = -1;
    float modWheel = 0, aftertouch = 0, pitchBend = 0;
    float exPb[128] = {}, exPress[128] = {}, exSlide[128] = {};      // expressão por nota (MPE / aftertouch polifónico)
    std::vector<float> busL, busR, vsL, vsR, aL, aR, bL, bR, xL, xR; float* vsOutL = nullptr; float* vsOutR = nullptr;
    double chainPpq = 0; FxUnit fx[kNumFxSlots]; Limiter lim; Smoother masterGain; float peakL = 0, peakR = 0;
    float globalDst[MD_COUNT] = {}; float sendAcc[3] = {}, sendW = 0.0f;
    int rrCounter[2] = { 0, 0 }; Rng rngGlobal;

    // ---- gestão de vozes ------------------------------------------------------------------
    Voice* allocate()
    {
        const int poly = std::max (1, std::min (kMaxVoices - 8, (int) prm[P_POLYPHONY]));
        int count = 0; for (auto& v : voices) if (v.active && ! v.stolen) ++count;
        if (count >= poly)
        {
            Voice* victim = nullptr;
            for (auto& v : voices) if (v.active && ! v.stolen && ! v.gate && (! victim || v.age < victim->age)) victim = &v;   // em libertação: a mais antiga
            if (! victim) for (auto& v : voices) if (v.active && ! v.stolen && (! victim || v.age < victim->age)) victim = &v;
            if (victim) { victim->stolen = true; victim->gate = false; victim->amp.fastRelease (0.004f); }
        }
        for (auto& v : voices) if (! v.active) return &v;
        Voice* o = &voices[0]; for (auto& v : voices) if (v.age < o->age) o = &v;   // último recurso
        return o;
    }
    void releaseVoice (Voice& v) { v.gate = false; v.amp.gateOff(); v.e2.gateOff(); v.e3.gateOff(); }

    float envMs (int base, int k) const { return prm[base + k]; }
    void setEnv (Adsr& e, int base) { e.set (prm[base] * 0.001f, prm[base + 1] * 0.001f, prm[base + 2], prm[base + 3] * 0.001f); }

    void startVoice (Voice* vp, int note, float vel, bool mono)
    {
        Voice& v = *vp;
        const bool wasActive = v.active && ! v.stolen;
        v.stolen = false; v.held = false; v.gate = true; v.note = note; v.vel = vel; v.age = ++ageCounter; v.mono = mono;
        const bool glideAlways = prm[P_GLIDE_MODE] < 0.5f;
        v.curNote = (prm[P_GLIDE] > 1.5f && glideAlways && lastNote >= 0) ? (float) lastNote : (wasActive && mono && prm[P_GLIDE] > 1.5f ? v.curNote : (float) note);
        v.tgtNote = (float) note;
        setEnv (v.amp, P_AMP_A); setEnv (v.e2, P_ENV2_A); setEnv (v.e3, P_ENV3_A);
        v.amp.gateOn(); v.e2.gateOn(); v.e3.gateOn();
        const bool k1 = prm[P_LFO1_KEY] > 0.5f, k2 = prm[P_LFO2_KEY] > 0.5f;
        v.l1.reset (k1 ? prm[P_LFO1_PHASE] : rngGlobal.uni(), (uint32_t) (note * 7919 + (int) ageCounter));
        v.l2.reset (k2 ? prm[P_LFO2_PHASE] : rngGlobal.uni(), (uint32_t) (note * 104729 + (int) ageCounter * 3));
        v.stepBeat = 0; v.stepSm = prm[P_STEP_V1]; v.drawPh = 0; v.menvT = 0; v.menvRelT = 0; v.menvRelLevel = 0; v.menvLast = 0; v.menvRel = false;
        v.lfoT = 0; v.rnd = rngGlobal.bi(); v.noise.seed ((uint32_t) (ageCounter * 2654435761u));
        if (! wasActive) { v.f1.reset (); v.f2.reset (); v.prevGain = 0; v.subPh = 0; }
        v.fade = 0.0f;
        for (int e = 0; e < 2; ++e) initOsc (v, e);
        v.active = true;
    }

    void initOsc (Voice& v, int E)
    {
        const float* pe = prm + (E ? (P_B_ON - P_A_ON) : 0);
        const int uni = std::max (1, std::min (kMaxUni, (int) pe[P_A_UNI]));
        const bool retrig = pe[P_A_RETRIG] > 0.5f;
        for (int u = 0; u < kMaxUni; ++u)
        {
            const float rp = rngGlobal.uni();
            v.ph[E][u] = retrig ? pe[P_A_PHASE] + (uni > 1 ? 0.011f * (float) u : 0.0f) : rp;
            v.ph[E][u] -= std::floor (v.ph[E][u]);
            v.ph2[E][u] = 0; v.pm[E][u] = 0; v.sp[E][u] = 0; v.syncD[E][u] = 0; v.fprev[E][u] = 1.0e9f;
        }
        v.zone[E] = nullptr; v.sd[E] = nullptr; v.instr[E] = nullptr; v.startDone[E] = false;
        for (auto& g : v.gr[E]) g.on = false;
        v.grAcc[E] = 1.0e9f; v.grHead[E] = 0.0; v.grRng[E].seed (0x51ED270Bu ^ ((uint32_t) v.note * 2654435761u) ^ ((uint32_t) E * 40503u));
        const int tyI = (int) pe[P_A_TYPE];
        if (pe[P_A_ON] < 0.5f || (tyI != 2 && tyI != 4) || ! lib || lib->inst.empty()) return;
        const int ii = std::max (0, std::min ((int) lib->inst.size() - 1, (int) pe[P_A_SMP]));
        const Instrument& I = lib->inst[(size_t) ii];
        const int vel127 = std::max (1, std::min (127, (int) std::lround (v.vel * 127.0f)));
        const Zone* cand[32]; int nc = 0;
        for (const auto& z : I.zones) if (v.note >= z.loKey && v.note <= z.hiKey && vel127 >= z.loVel && vel127 <= z.hiVel && nc < 32) cand[nc++] = &z;
        if (nc == 0) return;
        const int pick = pe[P_A_RR] > 0.5f ? (rrCounter[E]++ % nc) : 0;
        const Zone* z = cand[pick];
        if (z->data < 0 || z->data >= (int) lib->data.size()) return;
        v.zone[E] = z; v.sd[E] = lib->data[(size_t) z->data].get(); v.instr[E] = &I; v.lbeat[E] = 0.0;
        const int len = v.sd[E]->frames();
        const float st = clampf (pe[P_A_START], 0.0f, 0.98f);
        for (int u = 0; u < kMaxUni; ++u) v.sp[E][u] = pe[P_A_REV] > 0.5f ? (double) (len - 3) * (1.0 - st) : (double) len * st;
    }

    void runChain (int firstSlot, float* L, float* R, int n)
    {
        for (int s = 0; s < 3; ++s)
        {
            const int base = fxSlotBase (firstSlot + s);
            const int type = (int) prm[base];
            if (type == 0) continue;
            const float pp[5] = { prm[base + 1], prm[base + 2], prm[base + 3], prm[base + 4], prm[base + 5] };
            fx[firstSlot + s].setBeat (chainPpq);
            fx[firstSlot + s].process (type, pp, prm[base + 6], L, R, n, bpm);
        }
    }

    // ---- modulação -----------------------------------------------------------------------
    static float dstScale (int d)
    {
        switch (d)
        {
            case MD_PITCH_A: case MD_PITCH_B: case MD_PITCH_ALL: return 24.0f;
            case MD_CUT1: case MD_CUT2: return 5.0f;     // oitavas
            case MD_UNIDET_A: case MD_UNIDET_B: return 50.0f;
            case MD_PWM_A: case MD_PWM_B: return 0.45f;
            case MD_SUB: case MD_NOISE: return 30.0f;    // dB
            case MD_LFO1RATE: case MD_LFO2RATE: return 2.0f;
            default: return 1.0f;
        }
    }

    // envelope multi-ponto: nível (0..1) no instante t (segundos), pontos em prm[P_MENV_X1..]
    float menvLevelAt (float tSec) const
    {
        const int N = std::max (2, std::min (8, (int) prm[P_MENV_N])); const float len = std::max (0.02f, prm[P_MENV_LEN] * 0.001f);
        const float x = clampf (tSec / len, 0.0f, 1.0f);
        float px = 0.0f, py = prm[P_MENV_Y1];
        for (int i = 1; i < N; ++i)
        {
            const float cx = std::max (px, prm[P_MENV_X1 + 2 * i]), cy = prm[P_MENV_Y1 + 2 * i];
            if (x <= cx) return cx - px < 1.0e-5f ? cy : py + (cy - py) * (x - px) / (cx - px);
            px = cx; py = cy;
        }
        return py;
    }
    float menvEndTime() const { const int N = std::max (2, std::min (8, (int) prm[P_MENV_N])); float mx = 0; for (int i = 0; i < N; ++i) mx = std::max (mx, prm[P_MENV_X1 + 2 * i]); return mx * prm[P_MENV_LEN] * 0.001f; }

    void renderVoice (Voice& v, int m, float* outL, float* outR)
    {
        // ---------- fontes ----------
        float src[MS_COUNT];
        src[MS_NONE] = 0; src[MS_VEL] = v.vel; src[MS_KEY] = clampf ((v.curNote - 60.0f) / 36.0f, -1.0f, 1.0f);
        src[MS_MODWHEEL] = modWheel; src[MS_AFTERTOUCH] = aftertouch; src[MS_PITCHBEND] = pitchBend; src[MS_RANDOM] = v.rnd; src[MS_CONST] = 1.0f;
        for (int k = 0; k < kNumMacros; ++k) src[MS_MACRO1 + k] = prm[P_MACRO1 + k];
        // envelopes (avançam m amostras)
        setEnv (v.e2, P_ENV2_A); setEnv (v.e3, P_ENV3_A); if (! v.stolen) setEnv (v.amp, P_AMP_A);
        float ev2 = 0, ev3 = 0; for (int i = 0; i < m; ++i) { ev2 = v.e2.tick(); ev3 = v.e3.tick(); }
        src[MS_ENV2] = ev2; src[MS_ENV3] = ev3; src[MS_ENVAMP] = v.amp.getLevel();
        // ---- novas fontes: step sequencer, LFO desenhado, envelope multi-ponto, pressão/slide por nota
        {
            const float dt = (float) m / sr, beatsPerSec = bpm / 60.0f;
            const int stepsN = std::max (1, std::min (16, (int) prm[P_STEP_LEN]));
            const float stepBeats = kDivBeats[std::min (15, std::max (0, (int) prm[P_STEP_DIV]))];
            v.stepBeat += dt * beatsPerSec;
            const int idx = (int) std::floor (v.stepBeat / stepBeats) % stepsN;
            const float target = prm[P_STEP_V1 + idx], smooth = prm[P_STEP_SMOOTH];
            if (smooth < 0.01f) v.stepSm = target; else v.stepSm += (target - v.stepSm) * (1.0f - std::exp (-dt / (0.002f + smooth * 0.18f)));
            src[MS_STEP] = v.stepSm;
            float cps = prm[P_DRAW_SYNC] > 0.5f ? beatsPerSec / kDivBeats[std::min (15, std::max (0, (int) prm[P_DRAW_DIV]))] : prm[P_DRAW_RATE];
            v.drawPh += cps * dt; v.drawPh -= std::floor (v.drawPh);
            const float dp = v.drawPh * 32.0f; const int i0 = std::min (31, (int) dp), i1 = (i0 + 1) & 31; const float fr = dp - (float) i0;
            src[MS_DRAW] = prm[P_DRAW_P1 + i0] + (prm[P_DRAW_P1 + i1] - prm[P_DRAW_P1 + i0]) * fr;
            // multi-ponto
            const float relS = std::max (0.005f, prm[P_MENV_REL] * 0.001f); float lvl;
            if (v.gate)
            {
                const int susP = (int) prm[P_MENV_SUS], N = std::max (2, std::min (8, (int) prm[P_MENV_N]));
                float tmax = menvEndTime();
                if (susP >= 1 && susP <= N) tmax = std::max (0.0f, prm[P_MENV_X1 + 2 * (susP - 1)] * prm[P_MENV_LEN] * 0.001f);
                v.menvT += dt;
                if (prm[P_MENV_LOOP] > 0.5f && (susP == 0 || susP > N) && v.menvT > tmax && tmax > 0.001f) v.menvT = std::fmod (v.menvT, tmax);
                else if (v.menvT > tmax) v.menvT = tmax;
                lvl = menvLevelAt (v.menvT); v.menvLast = lvl;
            }
            else
            {
                if (! v.menvRel) { v.menvRel = true; v.menvRelLevel = v.menvLast; v.menvRelT = 0; }
                v.menvRelT += dt; lvl = v.menvRelLevel * std::max (0.0f, 1.0f - v.menvRelT / relS);
            }
            src[MS_MENV] = lvl;
            const int nn = std::max (0, std::min (127, v.note));
            src[MS_PRESSURE] = exPress[nn]; src[MS_SLIDE] = exSlide[nn];
        }
        float dst[MD_COUNT] = {};
        // LFOs: a taxa pode ser modulada por outras ligações → 1.ª passagem só para as taxas
        float rateMod[2] = { 0, 0 };
        for (int s = 0; s < kNumModSlots; ++s)
        {
            const int b = modSlotBase (s); const int sr_ = (int) prm[b], ds = (int) prm[b + 1];
            if (ds == MD_LFO1RATE && sr_ != MS_LFO1 && sr_ != MS_LFO2) rateMod[0] += prm[b + 2] * 2.0f * src[sr_];
            if (ds == MD_LFO2RATE && sr_ != MS_LFO1 && sr_ != MS_LFO2) rateMod[1] += prm[b + 2] * 2.0f * src[sr_];
        }
        for (int k = 0; k < 2; ++k)
        {
            const int base = k ? (P_LFO2_WAVE) : (P_LFO1_WAVE); Lfo& L = k ? v.l2 : v.l1;
            float cps;
            if (prm[base + 2] > 0.5f) cps = (bpm / 60.0f) / kDivBeats[std::min (15, std::max (0, (int) prm[base + 3]))];
            else cps = prm[base + 1];
            cps *= std::exp2 (rateMod[k]);
            const float fadeMs = prm[base + 5];
            const float fadeG = fadeMs > 1.5f ? std::min (1.0f, v.lfoT / (fadeMs * 0.001f)) : 1.0f;
            src[MS_LFO1 + k] = L.advance ((int) prm[base], cps / sr, m) * fadeG;
        }
        v.lfoT += (float) m / sr;
        for (int s = 0; s < kNumModSlots; ++s)
        {
            const int b = modSlotBase (s); const int sI = (int) prm[b], dI = (int) prm[b + 1]; const float amt = prm[b + 2];
            if (sI == MS_NONE || dI == MD_NONE || amt == 0.0f) continue;
            dst[dI] += amt * src[std::min (sI, (int) MS_COUNT - 1)] * dstScale (dI);
        }
        // globais (só fontes não-voz; calculado a partir da 1.ª voz ativa é suficiente)
        // envios/retorno aux: média das vozes ponderada pelo seu ganho (independente da ordem das vozes; auditoria C-02).
        // Com fontes globais (macro, mod wheel) todas as vozes valem o mesmo e o resultado é igual ao anterior.
        const float sndNow[3] = { dst[MD_SENDA], dst[MD_SENDB], dst[MD_AUXRET] };

        // ---------- altura ----------
        const float gl = prm[P_GLIDE];
        if (gl > 1.5f) v.curNote += (v.tgtNote - v.curNote) * (1.0f - std::exp (-(float) m / (gl * 0.001f * sr))); else v.curNote = v.tgtNote;
        const float common = prm[P_TRANSPOSE] + prm[P_TUNE] * 0.01f + pitchBend * prm[P_PB_RANGE] + dst[MD_PITCH_ALL] + exPb[std::max (0, std::min (127, v.note))] * prm[P_MPE_RANGE];

        // ---------- motores ----------
        float aOutL[kSub], aOutR[kSub], bOutL[kSub], bOutR[kSub];
        std::fill (aOutL, aOutL + m, 0.0f); std::fill (aOutR, aOutR + m, 0.0f); std::fill (bOutL, bOutL + m, 0.0f); std::fill (bOutR, bOutR + m, 0.0f);
        const float blend = prm[P_BLEND];
        const float gBlendA = std::min (1.0f, 1.41421356f * std::cos (blend * 0.5f * kPi)), gBlendB = std::min (1.0f, 1.41421356f * std::sin (blend * 0.5f * kPi));
        if (prm[P_A_ON] > 0.5f) renderEngine (v, 0, m, common, dst, aOutL, aOutR, gBlendA);
        if (prm[P_B_ON] > 0.5f) renderEngine (v, 1, m, common, dst, bOutL, bOutR, gBlendB);

        // sub + ruído
        float subL[kSub], nzL[kSub]; std::fill (subL, subL + m, 0.0f); std::fill (nzL, nzL + m, 0.0f);
        const float subDb = prm[P_SUB_LEVEL] + dst[MD_SUB], nzDb = prm[P_NOISE_LEVEL] + dst[MD_NOISE];
        if (subDb > -59.0f)
        {
            const float g = dbToGain (std::min (subDb, 6.0f)), f = midiToHz (v.curNote + common - (prm[P_SUB_OCT] > 0.5f ? 24.0f : 12.0f)), dt = f / sr; const int w = (int) prm[P_SUB_WAVE];
            for (int i = 0; i < m; ++i)
            {
                float y = w == 0 ? std::sin (kTwoPi * v.subPh) : (w == 1 ? 4.0f * std::fabs (v.subPh - 0.5f) - 1.0f : (v.subPh < 0.5f ? 0.8f : -0.8f));
                subL[i] = y * g; v.subPh += dt; if (v.subPh >= 1.0f) v.subPh -= 1.0f;
            }
        }
        if (nzDb > -59.0f) { const float g = dbToGain (std::min (nzDb, 6.0f)) * 0.7f, col = prm[P_NOISE_COLOR]; for (int i = 0; i < m; ++i) nzL[i] = v.noise.tick (col) * g; }

        // ---------- filtros ----------
        float mixL[kSub], mixR[kSub], outFL[kSub], outFR[kSub];
        const int route = (int) prm[P_FILT_ROUTE];
        const float bal = clampf (prm[P_FILT_BAL] + dst[MD_FILTBAL], 0.0f, 1.0f);
        const float w1 = std::min (1.0f, 1.41421356f * std::cos (bal * 0.5f * kPi)), w2 = std::min (1.0f, 1.41421356f * std::sin (bal * 0.5f * kPi));
        float f1c = cutoffFor (v, 1, ev2, ev3, dst[MD_CUT1]), f2c = cutoffFor (v, 2, ev2, ev3, dst[MD_CUT2]);
        const int t1 = (int) prm[P_F1_TYPE], t2 = (int) prm[P_F2_TYPE];
        const float r1 = clampf (prm[P_F1_RES] + dst[MD_RES1], 0.0f, 1.0f), r2 = clampf (prm[P_F2_RES] + dst[MD_RES2], 0.0f, 1.0f);
        const float d1 = clampf (prm[P_F1_DRIVE] + dst[MD_DRIVE1], 0.0f, 1.0f), d2 = clampf (prm[P_F2_DRIVE] + dst[MD_DRIVE2], 0.0f, 1.0f);
        if (route == 0)
        {
            for (int i = 0; i < m; ++i) { mixL[i] = aOutL[i] + bOutL[i] + subL[i] + nzL[i]; mixR[i] = aOutR[i] + bOutR[i] + subL[i] + nzL[i]; }
            v.f1.process (t1, mixL, mixR, m, f1c, r1, d1); v.f2.process (t2, mixL, mixR, m, f2c, r2, d2);
            for (int i = 0; i < m; ++i) { outFL[i] = mixL[i]; outFR[i] = mixR[i]; }
        }
        else
        {
            float pL[kSub], pR[kSub], qL[kSub], qR[kSub];
            if (route == 1) for (int i = 0; i < m; ++i) { pL[i] = qL[i] = aOutL[i] + bOutL[i] + subL[i] + nzL[i]; pR[i] = qR[i] = aOutR[i] + bOutR[i] + subL[i] + nzL[i]; }
            else for (int i = 0; i < m; ++i) { pL[i] = aOutL[i] + subL[i]; pR[i] = aOutR[i] + subL[i]; qL[i] = bOutL[i] + nzL[i]; qR[i] = bOutR[i] + nzL[i]; }
            v.f1.process (t1, pL, pR, m, f1c, r1, d1); v.f2.process (t2, qL, qR, m, f2c, r2, d2);
            for (int i = 0; i < m; ++i) { outFL[i] = pL[i] * w1 + qL[i] * w2; outFR[i] = pR[i] * w1 + qR[i] * w2; }
        }

        // ---------- amplificador ----------
        const float velSens = prm[P_VEL_SENS];
        const float velG = (1.0f - velSens) + velSens * v.vel;
        const float ampMod = clampf (1.0f + dst[MD_AMP], 0.0f, 2.0f), pan = clampf (dst[MD_PAN], -1.0f, 1.0f);
        const float gPanL = std::cos ((pan + 1.0f) * 0.25f * kPi) * 1.41421356f, gPanR = std::sin ((pan + 1.0f) * 0.25f * kPi) * 1.41421356f;
        const float target = velG * ampMod * 0.5f;
        const float g0 = v.prevGain, g1 = target;
        v.prevGain = target;
        const float fadeStep = 1.0f / 64.0f;
        const float vsend = std::min (1.0f, std::max (0.0f, sndNow[0]) + std::max (0.0f, sndNow[1]));
        for (int i = 0; i < m; ++i)
        {
            const float e = v.amp.tick();
            const float g = (g0 + (g1 - g0) * ((float) (i + 1) / (float) m)) * e;
            v.fade = std::min (1.0f, v.fade + fadeStep);
            const float yL = outFL[i] * g * gPanL * v.fade, yR = outFR[i] * g * gPanR * v.fade;
            outL[i] += yL; outR[i] += yR;
            if (vsend > 0.0f) { vsOutL[i] += yL * vsend; vsOutR[i] += yR * vsend; }     // envio por voz (mod SEND A/B), antes dos inserts
        }
        { const float w = std::max (target, 1.0e-6f) * v.amp.getLevel(); for (int k = 0; k < 3; ++k) sendAcc[k] += sndNow[k] * w; sendW += w; }
        if (! v.amp.active() || (v.amp.getStage() == Adsr::Sustain && v.amp.getLevel() < 1.0e-4f)) { v.active = false; v.stolen = false; v.gate = false; }
    }

    float cutoffFor (const Voice& v, int which, float ev2, float ev3, float dstOct) const
    {
        const int base = which == 1 ? P_F1_TYPE : P_F2_TYPE;
        const float cut = prm[base + 1], key = prm[base + 4], envAmt = prm[base + 5];
        const float env = which == 1 ? ev2 : ev3;
        const float oct = dstOct + key * (v.curNote - 60.0f) / 12.0f + envAmt * env * 5.0f;
        return clampf (cut * std::exp2 (oct), 20.0f, 20000.0f);
    }

    // ---- um motor de oscilador (A ou B) -----------------------------------------------------
    void renderEngine (Voice& v, int E, int m, float common, const float* dst, float* oL, float* oR, float blendGain)
    {
        const float* pe = prm + (E ? (P_B_ON - P_A_ON) : 0);
        const int type = (int) pe[P_A_TYPE];
        const int uni = (type == 3 || type == 4) ? 1 : std::max (1, std::min (kMaxUni, (int) pe[P_A_UNI]));
        const bool track = pe[P_A_TRACK] > 0.5f;
        { const int klo = (int) prm[E ? P_B_KEYLO : P_A_KEYLO], khi = (int) prm[E ? P_B_KEYHI : P_A_KEYHI]; if (v.note < klo || v.note > khi) return; }   // split / layer
        const float levDb = pe[P_A_LEVEL]; if (levDb <= -59.9f) return;
        const float lvMod = clampf (1.0f + dst[E ? MD_LEVEL_B : MD_LEVEL_A], 0.0f, 2.0f);
        const float gain = dbToGain (levDb) * lvMod * blendGain * (uni > 1 ? 1.0f / std::sqrt ((float) uni) : 1.0f);
        const float pitchMod = dst[E ? MD_PITCH_B : MD_PITCH_A];
        float keyNote = track ? v.curNote : 60.0f;
        const Zone* zone = v.zone[E];
        if (type == 2 || type == 4) { if (! zone) return; if (v.instr[E] && v.instr[E]->fixedPitch) keyNote = (float) zone->root; else if (! track) keyNote = (float) zone->root; }
        const float note = keyNote + common + pe[P_A_COARSE] + pe[P_A_FINE] * 0.01f + pitchMod;
        const float freq = midiToHz (note);
        const float det = clampf (pe[P_A_UNIDET] + dst[E ? MD_UNIDET_B : MD_UNIDET_A], 0.0f, 100.0f), spread = pe[P_A_UNISPR], panE = clampf (pe[P_A_PAN] + 0.0f, -1.0f, 1.0f);
        float ratio[kMaxUni], gL[kMaxUni], gR[kMaxUni]; float maxRatio = 1.0f;
        for (int u = 0; u < uni; ++u)
        {
            const float o = uni > 1 ? ((float) u - 0.5f * (float) (uni - 1)) / (0.5f * (float) (uni - 1)) : 0.0f;
            ratio[u] = std::exp2 (o * det / 1200.0f); maxRatio = std::max (maxRatio, ratio[u]);
            const float pn = clampf (panE + o * spread, -1.0f, 1.0f);
            gL[u] = std::cos ((pn + 1.0f) * 0.25f * kPi) * 1.41421356f * 0.7071f; gR[u] = std::sin ((pn + 1.0f) * 0.25f * kPi) * 1.41421356f * 0.7071f;
            gL[u] *= gain; gR[u] *= gain;
        }
        float tmp[kSub];
        if (type == 3) { renderString (v, E, m, pe, freq, gL[0], gR[0], oL, oR); return; }
        if (type == 4) { renderGranular (v, E, m, note, gL[0], gR[0], oL, oR, dst); return; }
        if (type == 2)
        {
            renderSample (v, E, m, pe, note, ratio, gL, gR, uni, oL, oR, dst);
            return;
        }
        const float syncMul = (type == 0 && pe[P_A_SYNC] > 0.01f) ? std::exp2 (pe[P_A_SYNC] / 12.0f) : 1.0f;   // o escravo da sync corre mais depressa: a tabela tem de ser band-limited para ele (auditoria C-03)
        const int mip = Wavetables::mipForFrequency (freq * maxRatio * syncMul, factor > 1 ? (float) hostSr : sr);   // com oversampling, a onda-base fica limitada à Nyquist do anfitrião (auditoria C-05)
        float fmAmt = clampf (pe[P_A_FMAMT] + dst[E ? MD_FM_B : MD_FM_A], 0.0f, 1.0f); const float fold = clampf (pe[P_A_FOLD] + dst[E ? MD_FOLD_B : MD_FOLD_A], 0.0f, 1.0f);
        const float fmRatio = kFmRatios[std::min (7, std::max (0, (int) pe[P_A_FMRAT]))];
        if (fmAmt > 0.001f)
        {   // índice de FM limitado para que a banda lateral mais alta fique abaixo de ~0,35 da taxa do motor (auditoria C-04)
            const float beta = 6.2831853f * fmAmt, fmHz = std::max (1.0f, freq * maxRatio * fmRatio);
            const float allowed = (0.35f * sr - freq * maxRatio) / fmHz - 1.0f;
            if (beta > allowed) fmAmt *= std::max(0.0f, allowed) / beta;
        }
        const float* sineT = wt->basic (0, 0);
        if (type == 0)
        {
            const int w = (int) pe[P_A_WAVE];
            const float pw = clampf (pe[P_A_PWM] + dst[E ? MD_PWM_B : MD_PWM_A], 0.02f, 0.98f);
            const float sync = pe[P_A_SYNC];
            const float* tab = wt->basic (w == 0 ? 0 : (w == 1 ? 1 : 2), mip);
            for (int u = 0; u < uni; ++u)
            {
                const float dt = freq * ratio[u] / sr; float ph = v.ph[E][u], ph2 = v.ph2[E][u], pm = v.pm[E][u];
                const float dt2 = sync > 0.01f ? dt * std::exp2 (sync / 12.0f) : dt;
                const float dtm = dt * fmRatio;
                for (int i = 0; i < m; ++i)
                {
                    float p = sync > 0.01f ? ph2 : ph;
                    if (fmAmt > 0.001f) { p += fmAmt * Wavetables::read (sineT, pm); p -= std::floor (p); }
                    auto shape = [&] (float q) { if (w == 3) { float q2 = q + pw; q2 -= std::floor (q2); return (Wavetables::read (tab, q) - Wavetables::read (tab, q2)) * 0.5f; } return Wavetables::read (tab, q); };
                    float y = shape (p);
                    if (sync > 0.01f)
                    {   // polyBLEP no salto do reinício de fase da sync (auditoria C-03)
                        if (ph > 1.0f - dt) { float tail = ph2 + dt2 * (1.0f - ph) / dt; tail -= std::floor (tail); v.syncD[E][u] = shape (0.0f) - shape (tail); y += 0.5f * v.syncD[E][u] * polyBlep (ph, dt); }
                        else if (ph < dt) y += 0.5f * v.syncD[E][u] * polyBlep (ph, dt);
                    }
                    if (fold > 0.001f) y = foldAdaa (y, fold, v.fprev[E][u]);
                    tmp[i] = y;
                    ph += dt; if (ph >= 1.0f) { ph -= 1.0f; if (sync > 0.01f) ph2 = ph * (dt2 / dt); }
                    if (sync > 0.01f) { ph2 += dt2; if (ph2 >= 1.0f) ph2 -= std::floor (ph2); }
                    pm += dtm; pm -= std::floor (pm);
                }
                v.ph[E][u] = ph; v.ph2[E][u] = ph2; v.pm[E][u] = pm;
                for (int i = 0; i < m; ++i) { oL[i] += tmp[i] * gL[u]; oR[i] += tmp[i] * gR[u]; }
            }
            return;
        }
        // wavetable
        const int tabI = std::max (0, std::min (Wavetables::kTables, (int) pe[P_A_WTTAB]));
        const float pos = pe[P_A_WTPOS] + dst[E ? MD_WTPOS_B : MD_WTPOS_A], warp = clampf (pe[P_A_WARP] + dst[E ? MD_WARP_B : MD_WARP_A], -1.0f, 1.0f);
        int f0, f1; float fr; Wavetables::framePos (pos, f0, f1, fr);
        const float *c0, *c1;
        if (type == 5) { if (! spBank[E].ready()) return; c0 = spBank[E].cycle (f0, mip); c1 = spBank[E].cycle (f1, mip); }
        else if (tabI >= Wavetables::kUserTable && userWt.loaded()) { c0 = userWt.cycle (f0, mip); c1 = userWt.cycle (f1, mip); }
        else { const int tb = std::min (tabI, Wavetables::kTables - 1); c0 = wt->cycle (tb, f0, mip); c1 = wt->cycle (tb, f1, mip); }
        for (int u = 0; u < uni; ++u)
        {
            const float dt = freq * ratio[u] / sr; float ph = v.ph[E][u], pm = v.pm[E][u]; const float dtm = dt * fmRatio;
            for (int i = 0; i < m; ++i)
            {
                float p = ph;
                if (fmAmt > 0.001f) { p += fmAmt * Wavetables::read (sineT, pm); p -= std::floor (p); }
                if (warp > 0.001f) { p *= 1.0f + 3.0f * warp; p -= std::floor (p); }
                else if (warp < -0.001f) p = p + warp * p * (1.0f - p);
                float y = Wavetables::read (c0, p); y += (Wavetables::read (c1, p) - y) * fr;
                if (fold > 0.001f) y = foldAdaa (y, fold, v.fprev[E][u]);
                tmp[i] = y;
                ph += dt; if (ph >= 1.0f) ph -= 1.0f;
                pm += dtm; pm -= std::floor (pm);
            }
            v.ph[E][u] = ph; v.pm[E][u] = pm;
            for (int i = 0; i < m; ++i) { oL[i] += tmp[i] * gL[u]; oR[i] += tmp[i] * gR[u]; }
        }
    }

    // ---- motor Espectral: reconstrução incremental da tabela quando os controlos mudam ----
    SpectralParams spectralParams (int E) const
    {
        const int o = E ? (P_B_SP_TILT - P_A_SP_TILT) : 0;
        SpectralParams sp; sp.tilt = prm[P_A_SP_TILT + o]; sp.form = prm[P_A_SP_FORM + o]; sp.fq = prm[P_A_SP_FQ + o]; sp.comb = prm[P_A_SP_COMB + o];
        sp.odd = prm[P_A_SP_ODD + o]; sp.sweep = prm[P_A_SP_SWEEP + o]; sp.smear = prm[P_A_SP_SMEAR + o]; return sp;
    }
    void updateSpectral()
    {
        for (int E = 0; E < 2; ++E)
        {
            const float* pe = prm + (E ? (P_B_ON - P_A_ON) : 0);
            if (pe[P_A_ON] > 0.5f && (int) pe[P_A_TYPE] == 5) spBank[E].update (spectralParams (E), 2);
        }
    }
    void rebuildSpectralNow() { for (int E = 0; E < 2; ++E) spBank[E].rebuildNow (spectralParams (E)); }   // offline / testes

    // ---- motor Granular: nuvem de grãos lidos do sample do voz (janela de Hann, posição/altura/pan aleatórios) ----
    void renderGranular (Voice& v, int E, int m, float note, float gL0, float gR0, float* oL, float* oR, const float* dst)
    {
        const Zone* z = v.zone[E]; const SampleData* sd = v.sd[E]; if (! z || ! sd || z->loopBeats > 0.0f) return;
        const int len = sd->frames(); if (len < 64) return;
        const int o = E ? (P_B_GR_SIZE - P_A_GR_SIZE) : 0;
        const float sizeMs = prm[P_A_GR_SIZE + o], dens = prm[P_A_GR_DENS + o], rnd = prm[P_A_GR_RAND + o], pj = prm[P_A_GR_PJIT + o];
        const float speed = prm[P_A_GR_SPEED + o], spread = prm[P_A_GR_SPREAD + o];
        const float posP = clampf (prm[P_A_GR_POS + o] + dst[E ? MD_START_B : MD_START_A], 0.0f, 1.0f);
        const float baseStep = std::exp2 ((note - (float) z->root) / 12.0f) * (sd->sampleRate / sr);
        const int glen = std::max (32, (int) (sizeMs * 0.001f * sr));
        const float interval = std::max (1.0f, sr / dens);
        const float overlap = std::max (1.0f, dens * sizeMs * 0.001f);
        const float norm = 0.9f * zgain (z) / std::sqrt (overlap);
        const double headInc = (double) speed * (double) (sd->sampleRate / sr);
        auto& G = v.gr[E];
        for (int i = 0; i < m; ++i)
        {
            v.grAcc[E] += 1.0f;
            if (v.grAcc[E] >= interval)
            {
                v.grAcc[E] = v.grAcc[E] >= 1.0e8f ? 0.0f : v.grAcc[E] - interval;
                for (auto& g : G) if (! g.on)
                {
                    double head = (double) posP * (double) len + v.grHead[E]; head -= std::floor (head / (double) len) * (double) len;
                    double st = head + (double) (rnd * v.grRng[E].bi() * 0.5f) * (double) len;
                    st = std::max (0.0, std::min ((double) (len - 4), st - std::floor (st / (double) len) * (double) len));
                    g.pos = st; g.step = baseStep * std::exp2 (pj * 4.0f * v.grRng[E].bi() / 12.0f); g.pan = spread * v.grRng[E].bi();
                    g.age = 0; g.len = glen; g.on = true; break;
                }
            }
            float l = 0.0f, r = 0.0f;
            for (auto& g : G)
            {
                if (! g.on) continue;
                if (g.pos >= (double) (len - 2) || g.age >= g.len) { g.on = false; continue; }
                const float w = 0.5f - 0.5f * std::cos (2.0f * kPi * (float) g.age / (float) g.len);
                float s = readHermite (sd->l, g.pos); if (sd->stereo()) s = 0.5f * (s + readHermite (sd->r, g.pos));
                s *= w;
                l += s * std::cos ((g.pan + 1.0f) * 0.25f * kPi) * 1.41421356f; r += s * std::sin ((g.pan + 1.0f) * 0.25f * kPi) * 1.41421356f;
                g.pos += (double) g.step; ++g.age;
            }
            oL[i] += l * gL0 * norm; oR[i] += r * gR0 * norm;
            v.grHead[E] += headInc;
        }
    }
    static float zgain (const Zone* z) { return z->gain; }

    // ---- motor Corda: Karplus-Strong estendido (linha de atraso + perdas passa-baixo), excitado no início da nota ----
    void renderString (Voice& v, int E, int m, const float* pe, float freq, float gL, float gR, float* oL, float* oR)
    {
        (void) pe;
        const int vi = (int) (&v - voices);
        float* buf = ksLine (vi, E);
        const float damp = prm[E ? P_B_KS_DAMP : P_A_KS_DAMP], decay = prm[E ? P_B_KS_DECAY : P_A_KS_DECAY], pick = prm[E ? P_B_KS_PICK : P_A_KS_PICK];
        const float a = clampf (1.0f - 0.92f * damp, 0.05f, 1.0f);                       // coeficiente do passa-baixo (1 = sem perdas de agudos)
        const float gd = (1.0f - a) / a;                                                 // atraso de grupo do passa-baixo a baixa frequência
        const float D = clampf (sr / std::max (20.0f, freq) - gd, 4.0f, (float) ksSize - 4.0f);
        const float t60 = 0.15f * std::pow (80.0f, decay);                               // 0,15 s … 12 s
        const float g = std::pow (10.0f, -3.0f * (D / sr) / t60);                        // ganho por volta para atingir -60 dB em t60
        if (! v.startDone[E])
        {
            v.startDone[E] = true; v.ksLp[E] = 0;
            std::fill (buf, buf + ksSize, 0.0f);
            const int N = std::max (4, (int) std::lround (D));
            Rng ex; ex.seed (0x9E3779B9u ^ ((uint32_t) v.note * 2654435761u) ^ ((uint32_t) E * 40503u));       // ruído determinístico por nota: calibração e QA reprodutíveis
            for (int i = 0; i < N; ++i) buf[i] = ex.bi();
            const int pp = std::max (1, (int) (pick * 0.5f * (float) N));               // posição da palheta: entalhes em pente na excitação
            if (pick > 0.02f) for (int i = N - 1; i >= pp; --i) buf[i] -= buf[i - pp] * pick;
            float mean = 0; for (int i = 0; i < N; ++i) mean += buf[i]; mean /= (float) N;
            for (int i = 0; i < N; ++i) buf[i] -= mean;                                  // sem DC
            v.ksW[E] = N;                                                                // a escrita começa logo a seguir à excitação
        }
        int w = v.ksW[E]; float lp = v.ksLp[E];
        for (int i = 0; i < m; ++i)
        {
            float rp = (float) w - D; if (rp < 0) rp += (float) ksSize;
            int i0 = (int) rp; if (i0 >= ksSize) i0 = ksSize - 1; const int i1 = i0 + 1 >= ksSize ? 0 : i0 + 1; const float fr = rp - (float) i0;
            const float y = buf[i0] + (buf[i1] - buf[i0]) * fr;
            lp += a * (y - lp); lp = flushDenorm (lp);
            buf[w] = lp * g;
            if (++w >= ksSize) w = 0;
            oL[i] += y * gL * 1.6f; oR[i] += y * gR * 1.6f;
        }
        v.ksW[E] = w; v.ksLp[E] = lp;
    }
    static constexpr float kFmRatios[8] = { 0.5f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 8.0f };
    // dobra com ADAA de 1.ª ordem (antiderivada analítica de sin): reduz o aliasing da dobra sem oversampling (auditoria C-04)
    static float foldAdaa (float x, float amt, float& xp)
    {
        const float K = (1.0f + amt * 5.0f) * 1.5707963f, norm = 1.0f / (1.0f + 0.15f * amt);
        if (xp > 1.0e8f) xp = x;
        const float dx = x - xp;
        const float y = std::fabs (dx) < 1.0e-4f ? std::sin (K * 0.5f * (x + xp)) : (std::cos (K * xp) - std::cos (K * x)) / (K * dx);
        xp = x; return y * norm;
    }
    static float foldFn (float x, float amt) { return std::sin (x * (1.0f + amt * 5.0f) * 1.5707963f * 1.0f) * (1.0f / (1.0f + 0.15f * amt)); }

    static float readHermite (const std::vector<float>& d, double pos)
    {
        const int len = (int) d.size(); const int i = (int) pos; const float t = (float) (pos - (double) i);
        auto g = [&] (int k) { return (k < 0 || k >= len) ? 0.0f : d[(size_t) k]; };
        return hermite (g (i - 1), g (i), g (i + 1), g (i + 2), t);
    }

    // loop sincronizado com o andamento do projeto: a posição vem do compasso (ppq), nunca deriva nem desalinha
    static float hermiteWrap (const float* d, int len, double pos)
    {
        const int i = (int) pos; const float t = (float) (pos - (double) i);
        auto g = [&] (int k) { k %= len; if (k < 0) k += len; return d[k]; };
        return hermite (g (i - 1), g (i), g (i + 1), g (i + 2), t);
    }
    void renderLoopZone (Voice& v, int E, int m, const float* pe, const float* gL, const float* gR, int uni, float* oL, float* oR)
    {
        const Zone* z = v.zone[E]; const SampleData* sd = v.sd[E];
        const float* dl = sd->l.data(); const float* dr = sd->stereo() ? sd->r.data() : dl; int len = sd->frames(); bool stretched = false;
        if (auto* sy = sd->sync.get())
        {
            sy->want.store (bpm, std::memory_order_relaxed); sy->active.store (true, std::memory_order_relaxed);
            if (auto* st = sy->cur.load (std::memory_order_acquire))
                if (std::fabs (st->bpm - bpm) <= 0.0015f * bpm && st->l.size() >= 64) { dl = st->l.data(); dr = st->r.empty() ? dl : st->r.data(); len = (int) st->l.size(); stretched = true; }
        }
        if (len < 8) return;
        float sumL = 0, sumR = 0; for (int u = 0; u < uni; ++u) { sumL += gL[u]; sumR += gR[u]; }
        const double dp = (double) bpm / (60.0 * (double) sr), invB = 1.0 / (double) z->loopBeats; const bool rev = pe[P_A_REV] > 0.5f;
        const double b0 = hostPlaying ? subPpq : v.lbeat[E]; const float zg = z->gain; double ph = 0;
        for (int i = 0; i < m; ++i)
        {
            ph = (b0 + (double) i * dp) * invB; ph -= std::floor (ph); if (rev) ph = 1.0 - ph;
            const double pos = ph * (double) len;
            oL[i] += hermiteWrap (dl, len, pos) * sumL * zg; oR[i] += hermiteWrap (dr, len, pos) * sumR * zg;
        }
        v.lbeat[E] = b0 + (double) m * dp; loopSeen = true;
        loopPh.store ((float) ph, std::memory_order_relaxed); loopSrc.store (z->loopBpm, std::memory_order_relaxed); loopSt.store (stretched, std::memory_order_relaxed);
    }
    void renderSample (Voice& v, int E, int m, const float* pe, float note, const float* ratio, const float* gL, const float* gR, int uni, float* oL, float* oR, const float* dst)
    {
        const Zone* z = v.zone[E]; const SampleData* sd = v.sd[E]; if (! z || ! sd) return;
        if (z->loopBeats > 0.0f) { renderLoopZone (v, E, m, pe, gL, gR, uni, oL, oR); return; }
        const int len = sd->frames(); if (len < 4) return;
        const float baseStep = std::exp2 ((note - (float) z->root) / 12.0f) * (sd->sampleRate / sr);
        const bool loop = pe[P_A_LOOP] > 0.5f && z->loopStart >= 0 && z->loopEnd > z->loopStart + 8; const bool rev = pe[P_A_REV] > 0.5f;
        const float zg = z->gain * 1.0f;
        if (! v.startDone[E])
        {   // MD_START_A/B: o ponto de arranque modulado é lido uma vez, no primeiro bloco da nota (auditoria C-01)
            v.startDone[E] = true;
            const float st = clampf (pe[P_A_START] + dst[E ? MD_START_B : MD_START_A], 0.0f, 0.98f);
            for (int u = 0; u < kMaxUni; ++u) v.sp[E][u] = rev ? (double) (len - 3) * (1.0 - st) : (double) len * st;
        }
        for (int u = 0; u < uni; ++u)
        {
            double pos = v.sp[E][u]; const double step = (double) (baseStep * ratio[u]) * (rev ? -1.0 : 1.0);
            for (int i = 0; i < m; ++i)
            {
                if (pos < 0.0 || pos >= (double) (len - 2)) { if (loop && ! rev && pos >= (double) (z->loopEnd - 1)) {} else if (! loop || rev) { pos = -1.0e9; break; } }
                if (loop && ! rev && pos >= (double) z->loopEnd) pos -= (double) (z->loopEnd - z->loopStart);
                if (pos < 0.0 || pos >= (double) (len - 2)) { pos = -1.0e9; break; }
                const float l = readHermite (sd->l, pos), r = sd->stereo() ? readHermite (sd->r, pos) : l;
                oL[i] += l * gL[u] * zg; oR[i] += r * gR[u] * zg;
                pos += step;
            }
            v.sp[E][u] = pos;
        }
    }
};
} // namespace sk
