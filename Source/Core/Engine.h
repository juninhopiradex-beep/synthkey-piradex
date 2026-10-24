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
#include "Fx.h"
#include "Samples.h"

namespace sk
{
class Engine
{
public:
    static constexpr int kMaxVoices = 40;
    static constexpr int kSub = 32;
    static constexpr int kMaxUni = 8;

    void prepare (double sampleRate, int maxBlock)
    {
        sr = (float) sampleRate; maxN = std::max (maxBlock, kSub);
        wt = &Wavetables::get();
        for (auto& v : voices) { v.f1.prepare (sr); v.f2.prepare (sr); v.amp.setSampleRate (sr); v.e2.setSampleRate (sr); v.e3.setSampleRate (sr); }
        for (auto* b : { &busL, &busR, &aL, &aR, &bL, &bR, &xL, &xR }) b->assign ((size_t) maxN, 0.0f);
        for (auto& f : fx) f.prepare (sr, maxN);
        lim.prepare (sr);
        masterGain.setTime (sr, 15.0f);
        reset();
    }
    void reset()
    {
        for (auto& v : voices) { v.active = false; v.f1.reset(); v.f2.reset(); }
        for (auto& f : fx) f.reset();
        lim.reset(); stack.clear(); ageCounter = 0; sustain = false; lastNote = -1;
        for (auto* b : { &busL, &busR }) std::fill (b->begin(), b->end(), 0.0f);
        masterGain.snap (1.0f); peakL = peakR = 0;
    }
    void setParams (const float* p) { prm = p; }
    void setLibrary (const Library* l) { if (l != lib) { lib = l; panic(); for (auto& v : voices) { v.zone[0] = v.zone[1] = nullptr; v.sd[0] = v.sd[1] = nullptr; v.instr[0] = v.instr[1] = nullptr; } } }
    void setBpm (double b) { bpm = (float) std::max (20.0, std::min (400.0, b)); }
    void setModWheel (float v) { modWheel = v; }
    void setAftertouch (float v) { aftertouch = v; }
    void setPitchBend (float v) { pitchBend = v; }
    void setSustain (bool on)
    {
        sustain = on;
        if (! on) for (auto& v : voices) if (v.active && v.held) { v.held = false; releaseVoice (v); }
    }
    int latencySamples() const { return lim.latency(); }
    int activeVoices() const { int n = 0; for (auto& v : voices) if (v.active && ! v.stolen) ++n; return n; }
    float getPeakL() const { return peakL; }
    float getPeakR() const { return peakR; }

    // ---------------------------- notas ---------------------------------------------
    void noteOn (int note, float vel)
    {
        vel = clampf (vel, 0.0f, 1.0f);
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
    void panic() { stack.clear(); for (auto& v : voices) { v.active = false; v.f1.reset(); v.f2.reset(); } }

    // ---------------------------- áudio -----------------------------------------------
    void render (float* L, float* R, int n)
    {
        n = std::min (n, maxN);
        std::fill (busL.begin(), busL.begin() + n, 0.0f); std::fill (busR.begin(), busR.begin() + n, 0.0f);
        int pos = 0;
        while (pos < n)
        {
            const int m = std::min (kSub, n - pos);
            for (auto& v : voices) if (v.active) renderVoice (v, m, busL.data() + pos, busR.data() + pos);
            pos += m;
        }
        // ---- inserts, aux, volume, limitador ----
        const int route = (int) prm[P_OUT_BUS];
        const float inA = route == 0 ? 1.0f : (route == 2 ? 0.5f : 0.0f), inB = route == 1 ? 1.0f : (route == 2 ? 0.5f : 0.0f);
        for (int i = 0; i < n; ++i) { aL[(size_t) i] = busL[(size_t) i] * inA; aR[(size_t) i] = busR[(size_t) i] * inA; bL[(size_t) i] = busL[(size_t) i] * inB; bR[(size_t) i] = busR[(size_t) i] * inB; }
        runChain (0, aL.data(), aR.data(), n); runChain (3, bL.data(), bR.data(), n);
        const float lvA = dbToGain (prm[P_INSA_LEVEL]), lvB = dbToGain (prm[P_INSB_LEVEL]);
        const float sendA = clampf (prm[P_SEND_A] + globalDst[MD_SENDA], 0.0f, 1.0f), sendB = clampf (prm[P_SEND_B] + globalDst[MD_SENDB], 0.0f, 1.0f);
        const float ret = dbToGain (prm[P_AUX_RETURN]) * clampf (1.0f + globalDst[MD_AUXRET], 0.0f, 2.0f);
        for (int i = 0; i < n; ++i) { aL[(size_t) i] *= lvA; aR[(size_t) i] *= lvA; bL[(size_t) i] *= lvB; bR[(size_t) i] *= lvB; xL[(size_t) i] = aL[(size_t) i] * sendA + bL[(size_t) i] * sendB; xR[(size_t) i] = aR[(size_t) i] * sendA + bR[(size_t) i] * sendB; }
        runChain (6, xL.data(), xR.data(), n);
        masterGain.tgt = dbToGain (prm[P_MASTER_VOL]);
        for (int i = 0; i < n; ++i)
        {
            const float g = masterGain.tick();
            L[i] = (aL[(size_t) i] + bL[(size_t) i] + xL[(size_t) i] * ret) * g;
            R[i] = (aR[(size_t) i] + bR[(size_t) i] + xR[(size_t) i] * ret) * g;
        }
        lim.process (L, R, n, prm[P_LIM_CEIL], prm[P_LIM_ON] > 0.5f);
        float pl = 0, pr = 0; for (int i = 0; i < n; ++i) { pl = std::max (pl, std::fabs (L[i])); pr = std::max (pr, std::fabs (R[i])); }
        peakL = std::max (pl, peakL * 0.92f); peakR = std::max (pr, peakR * 0.92f);
    }

private:
    struct Held { int note; float vel; };
    struct Voice
    {
        bool active = false, gate = false, stolen = false, held = false, mono = false;
        int note = 60; float vel = 0.8f; uint64_t age = 0;
        float curNote = 60, tgtNote = 60;
        Adsr amp, e2, e3; Lfo l1, l2; float lfoT = 0, rnd = 0;
        VoiceFilter f1, f2;
        float ph[2][kMaxUni] = {}, ph2[2][kMaxUni] = {}, pm[2][kMaxUni] = {};
        double sp[2][kMaxUni] = {};
        const Zone* zone[2] = { nullptr, nullptr };
        const SampleData* sd[2] = { nullptr, nullptr };
        const Instrument* instr[2] = { nullptr, nullptr };
        float subPh = 0; NoiseGen noise; float prevGain = 0, fade = 0;
    };

    float sr = 44100.0f, bpm = 120.0f; int maxN = 512;
    const float* prm = nullptr; const Wavetables* wt = nullptr; const Library* lib = nullptr;
    Voice voices[kMaxVoices];
    std::vector<Held> stack; uint64_t ageCounter = 0; bool sustain = false; int lastNote = -1;
    float modWheel = 0, aftertouch = 0, pitchBend = 0;
    std::vector<float> busL, busR, aL, aR, bL, bR, xL, xR;
    FxUnit fx[kNumFxSlots]; Limiter lim; Smoother masterGain; float peakL = 0, peakR = 0;
    float globalDst[MD_COUNT] = {};
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
            v.ph2[E][u] = 0; v.pm[E][u] = 0; v.sp[E][u] = 0;
        }
        v.zone[E] = nullptr; v.sd[E] = nullptr; v.instr[E] = nullptr;
        if (pe[P_A_ON] < 0.5f || (int) pe[P_A_TYPE] != 2 || ! lib || lib->inst.empty()) return;
        const int ii = std::max (0, std::min ((int) lib->inst.size() - 1, (int) pe[P_A_SMP]));
        const Instrument& I = lib->inst[(size_t) ii];
        const int vel127 = std::max (1, std::min (127, (int) std::lround (v.vel * 127.0f)));
        const Zone* cand[32]; int nc = 0;
        for (const auto& z : I.zones) if (v.note >= z.loKey && v.note <= z.hiKey && vel127 >= z.loVel && vel127 <= z.hiVel && nc < 32) cand[nc++] = &z;
        if (nc == 0) return;
        const int pick = pe[P_A_RR] > 0.5f ? (rrCounter[E]++ % nc) : 0;
        const Zone* z = cand[pick];
        if (z->data < 0 || z->data >= (int) lib->data.size()) return;
        v.zone[E] = z; v.sd[E] = lib->data[(size_t) z->data].get(); v.instr[E] = &I;
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
        for (int d = 0; d < MD_COUNT; ++d) globalDst[d] = 0;
        globalDst[MD_SENDA] = dst[MD_SENDA]; globalDst[MD_SENDB] = dst[MD_SENDB]; globalDst[MD_AUXRET] = dst[MD_AUXRET];

        // ---------- altura ----------
        const float gl = prm[P_GLIDE];
        if (gl > 1.5f) v.curNote += (v.tgtNote - v.curNote) * (1.0f - std::exp (-(float) m / (gl * 0.001f * sr))); else v.curNote = v.tgtNote;
        const float common = prm[P_TRANSPOSE] + prm[P_TUNE] * 0.01f + pitchBend * prm[P_PB_RANGE] + dst[MD_PITCH_ALL];

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
        for (int i = 0; i < m; ++i)
        {
            const float e = v.amp.tick();
            const float g = (g0 + (g1 - g0) * ((float) (i + 1) / (float) m)) * e;
            v.fade = std::min (1.0f, v.fade + fadeStep);
            outL[i] += outFL[i] * g * gPanL * v.fade; outR[i] += outFR[i] * g * gPanR * v.fade;
        }
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
        const int uni = std::max (1, std::min (kMaxUni, (int) pe[P_A_UNI]));
        const bool track = pe[P_A_TRACK] > 0.5f;
        const float levDb = pe[P_A_LEVEL]; if (levDb <= -59.9f) return;
        const float lvMod = clampf (1.0f + dst[E ? MD_LEVEL_B : MD_LEVEL_A], 0.0f, 2.0f);
        const float gain = dbToGain (levDb) * lvMod * blendGain * (uni > 1 ? 1.0f / std::sqrt ((float) uni) : 1.0f);
        const float pitchMod = dst[E ? MD_PITCH_B : MD_PITCH_A];
        float keyNote = track ? v.curNote : 60.0f;
        const Zone* zone = v.zone[E];
        if (type == 2) { if (! zone) return; if (v.instr[E] && v.instr[E]->fixedPitch) keyNote = (float) zone->root; else if (! track) keyNote = (float) zone->root; }
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
        if (type == 2)
        {
            renderSample (v, E, m, pe, note, ratio, gL, gR, uni, oL, oR, dst);
            return;
        }
        const int mip = Wavetables::mipForFrequency (freq * maxRatio, sr);
        const float fmAmt = clampf (pe[P_A_FMAMT] + dst[E ? MD_FM_B : MD_FM_A], 0.0f, 1.0f), fold = clampf (pe[P_A_FOLD] + dst[E ? MD_FOLD_B : MD_FOLD_A], 0.0f, 1.0f);
        const float fmRatio = kFmRatios[std::min (7, std::max (0, (int) pe[P_A_FMRAT]))];
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
                    float y;
                    if (w == 3) { float p2 = p + pw; p2 -= std::floor (p2); y = (Wavetables::read (tab, p) - Wavetables::read (tab, p2)) * 0.5f; }
                    else y = Wavetables::read (tab, p);
                    if (fold > 0.001f) y = foldFn (y, fold);
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
        const int tabI = std::max (0, std::min (Wavetables::kTables - 1, (int) pe[P_A_WTTAB]));
        const float pos = pe[P_A_WTPOS] + dst[E ? MD_WTPOS_B : MD_WTPOS_A], warp = clampf (pe[P_A_WARP] + dst[E ? MD_WARP_B : MD_WARP_A], -1.0f, 1.0f);
        int f0, f1; float fr; Wavetables::framePos (pos, f0, f1, fr);
        const float* c0 = wt->cycle (tabI, f0, mip); const float* c1 = wt->cycle (tabI, f1, mip);
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
                if (fold > 0.001f) y = foldFn (y, fold);
                tmp[i] = y;
                ph += dt; if (ph >= 1.0f) ph -= 1.0f;
                pm += dtm; pm -= std::floor (pm);
            }
            v.ph[E][u] = ph; v.pm[E][u] = pm;
            for (int i = 0; i < m; ++i) { oL[i] += tmp[i] * gL[u]; oR[i] += tmp[i] * gR[u]; }
        }
    }
    static constexpr float kFmRatios[8] = { 0.5f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 8.0f };
    static float foldFn (float x, float amt) { return std::sin (x * (1.0f + amt * 5.0f) * 1.5707963f * 1.0f) * (1.0f / (1.0f + 0.15f * amt)); }

    static float readHermite (const std::vector<float>& d, double pos)
    {
        const int len = (int) d.size(); const int i = (int) pos; const float t = (float) (pos - (double) i);
        auto g = [&] (int k) { return (k < 0 || k >= len) ? 0.0f : d[(size_t) k]; };
        return hermite (g (i - 1), g (i), g (i + 1), g (i + 2), t);
    }

    void renderSample (Voice& v, int E, int m, const float* pe, float note, const float* ratio, const float* gL, const float* gR, int uni, float* oL, float* oR, const float* dst)
    {
        const Zone* z = v.zone[E]; const SampleData* sd = v.sd[E]; if (! z || ! sd) return;
        const int len = sd->frames(); if (len < 4) return;
        const float baseStep = std::exp2 ((note - (float) z->root) / 12.0f) * (sd->sampleRate / sr);
        const bool loop = pe[P_A_LOOP] > 0.5f && z->loopStart >= 0 && z->loopEnd > z->loopStart + 8; const bool rev = pe[P_A_REV] > 0.5f;
        const float zg = z->gain * 1.0f; (void) dst;
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
