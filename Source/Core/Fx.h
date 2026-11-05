// =============================================================================
//  SYNTHKEY by Piradex — efeitos (10 tipos, 5 parâmetros normalizados 0..1 + mix)
//  Vazio, EQ, Compressor, Saturador, Chorus, Delay, Reverb, Faser, Bitcrusher,
//  Largura, Trémolo. Todos stereo, in-place, sem alocações no process().
// =============================================================================
#pragma once
#include "Dsp.h"
#include "Convolver.h"

namespace sk
{
inline float mapLog (float x, float a, float b) { return a * std::pow (b / a, clampf (x, 0.0f, 1.0f)); }
inline float mapLin (float x, float a, float b) { return a + (b - a) * clampf (x, 0.0f, 1.0f); }

// linha de atraso com leitura interpolada
struct DelayLine
{
    std::vector<float> buf; int w = 0, size = 1;
    void allocate (int n) { size = std::max (n, 4); buf.assign ((size_t) size, 0.0f); w = 0; }
    void clear() { std::fill (buf.begin(), buf.end(), 0.0f); w = 0; }
    inline void write (float x) { buf[(size_t) w] = x; if (++w >= size) w = 0; }
    inline float read (float d) const   // d em amostras (>=1), interpolação linear
    {
        float rp = (float) w - d; while (rp < 0) rp += (float) size;
        if (rp >= (float) size) rp = 0.0f;                        // rp<0 minúsculo + size arredonda para size: fora do buffer
        const int i0 = std::min ((int) rp, size - 1); const int i1 = i0 + 1 >= size ? 0 : i0 + 1; const float f = rp - (float) i0;
        return buf[(size_t) i0] + (buf[(size_t) i1] - buf[(size_t) i0]) * f;
    }
};

// transposição por duas cabeças de leitura com janela Hann (sem formantes; ~25–60 ms de grão)
struct PitchShifter
{
    DelayLine dl; float ph = 0.0f;
    void allocate (int n) { dl.allocate (n); ph = 0.0f; }
    void clear() { dl.clear(); ph = 0.0f; }
    inline float tick (float x, float ratio, float grain)      // grain em amostras
    {
        dl.write (x);
        ph += (1.0f - ratio) / grain; ph -= std::floor (ph);
        const float p2 = ph + 0.5f - std::floor (ph + 0.5f);
        const float w1 = std::sin (3.14159265f * ph), w2 = std::sin (3.14159265f * p2);
        return dl.read (std::max (2.0f, ph * grain + 2.0f)) * w1 * w1 + dl.read (std::max (2.0f, p2 * grain + 2.0f)) * w2 * w2;
    }
};

class FxUnit
{
public:
    enum Type { Empty = 0, EQ, Comp, Sat, Chorus, Delay, Reverb, Phaser, Crush, Width, Trem, DelaySync, Flanger, Ensemble, Shimmer, MultiTap, Pitch, Transient, Sidechain, Convolve };

    void prepare (float sampleRate, int maxBlock)
    {
        sr = sampleRate;
        dBuf.assign ((size_t) std::max (maxBlock, 64), 0.0f);
        dl[0].allocate ((int) (sr * 2.4f)); dl[1].allocate ((int) (sr * 2.4f));
        er[0].allocate ((int) (sr * 0.12f)); er[1].allocate ((int) (sr * 0.12f));
        ch[0].allocate ((int) (sr * 0.08f)); ch[1].allocate ((int) (sr * 0.08f));
        pre[0].allocate ((int) (sr * 0.25f)); pre[1].allocate ((int) (sr * 0.25f));
        fl[0].allocate ((int) (sr * 0.03f)); fl[1].allocate ((int) (sr * 0.03f));
        for (int i = 0; i < 2; ++i) { psA[i].allocate ((int) (sr * 0.2f)); psB[i].allocate ((int) (sr * 0.2f)); }
        const float scale = sr / 44100.0f;
        static const int base[8] = { 1103, 1327, 1531, 1783, 2017, 2243, 2477, 2707 };
        for (int i = 0; i < 8; ++i) { baseLen[i] = (int) ((float) base[i] * scale); rv[i].allocate ((int) ((float) baseLen[i] * 1.7f) + 64); }
        static const int apl[6] = { 142, 107, 379, 277, 89, 233 };
        for (int i = 0; i < 6; ++i) { rvAp[i].allocate ((int) ((float) apl[i] * scale) + 4); rvApLen[i] = (int) ((float) apl[i] * scale); }
        conv.prepare (sr);
        reset();
    }
    // sincronização do Sidechain: posição do projeto (tempos) no início do bloco; noteOn() reinicia o abaixamento no modo "Notas"
    void setBeat (double ppq) { ppq0 = ppq; }
    void noteOn() { trigPending = true; }
    void reset()
    {
        conv.reset(); duckT = 1.0e9f; duckG = 1.0f; trigPending = false; tsF = tsS = 0; tsG = 1.0f;
        for (auto& d : dl) d.clear(); for (auto& d : er) d.clear(); for (auto& d : ch) d.clear(); for (auto& d : pre) d.clear();
        for (auto& d : rv) d.clear(); for (auto& d : rvAp) d.clear();
        for (auto& v : rvLp) v = 0; for (auto& b : eq) for (auto& x : b) x.reset();
        envC = 0; for (auto& v : lp1) v = 0; for (auto& v : fbLp) v = 0;
        for (auto& a : ap) for (auto& v : a) v = 0; fbPh[0] = fbPh[1] = 0;
        phase = 0; crushHold[0] = crushHold[1] = 0; crushCnt = 0; for (auto& b : wd) b.reset();
        for (auto& d : fl) d.clear(); for (auto& d : psA) d.clear(); for (auto& d : psB) d.clear();
        flFb[0] = flFb[1] = 0; ensPh = 0; ensLp[0] = ensLp[1] = 0; mtLp[0] = mtLp[1] = 0; shFb[0] = shFb[1] = 0; shN = 0; for (auto& v : shBufL) v = 0; for (auto& v : shBufR) v = 0;
        satLp[0] = satLp[1] = 0; lastType = -1; fbHp[0] = fbHp[1] = 0; dSm[0] = dSm[1] = -1.0f; wowPh = 0; inHp = 0;
    }

    void process (int type, const float* p, float mix, float* L, float* R, int n, float bpm)
    {
        if (type != lastType) { if (lastType >= 0) reset(); lastType = type; }
        if (type == Empty || n <= 0) return;
        float* dry0 = nullptr;
        const bool needDry = mix < 0.999f && type != EQ && type != Comp && type != Width;
        if (needDry) { drySave.resize ((size_t) n * 2); std::memcpy (drySave.data(), L, (size_t) n * 4); std::memcpy (drySave.data() + n, R, (size_t) n * 4); dry0 = drySave.data(); }
        switch (type)
        {
            case EQ: doEq (p, L, R, n); break;
            case Comp: doComp (p, L, R, n); break;
            case Sat: doSat (p, L, R, n); break;
            case Chorus: doChorus (p, L, R, n); break;
            case Delay: doDelay (p, L, R, n, bpm, false); break;
            case DelaySync: doDelay (p, L, R, n, bpm, true); break;
            case Reverb: doReverb (p, L, R, n); break;
            case Phaser: doPhaser (p, L, R, n); break;
            case Crush: doCrush (p, L, R, n); break;
            case Width: doWidth (p, L, R, n); break;
            case Trem: doTrem (p, L, R, n); break;
            case Flanger: doFlanger (p, L, R, n); break;
            case Ensemble: doEnsemble (p, L, R, n); break;
            case Shimmer: doShimmer (p, L, R, n); break;
            case MultiTap: doMultiTap (p, L, R, n); break;
            case Pitch: doPitch (p, L, R, n); break;
            case Transient: doTransient (p, L, R, n); break;
            case Sidechain: doDuck (p, L, R, n, bpm); break;
            case Convolve: doConv (p, L, R, n); break;
            default: break;
        }
        if (needDry)
        {
            const float wet = mix, d = 1.0f - mix;
            for (int i = 0; i < n; ++i) { L[i] = dry0[i] * d + L[i] * wet; R[i] = dry0[n + i] * d + R[i] * wet; }
        }
    }

private:
    float sr = 44100.0f; int lastType = -1;
    std::vector<float> dBuf, drySave;

    // ---- EQ: shelf grave 120 Hz, pico médio, shelf agudo 6 kHz ------------------
    Biquad eq[2][3];
    void doEq (const float* p, float* L, float* R, int n)
    {
        const float gl = mapLin (p[0], -15, 15), gm = mapLin (p[1], -15, 15), fm = mapLog (p[2], 200, 8000), gh = mapLin (p[3], -15, 15), q = mapLog (p[4], 0.3f, 4.0f);
        for (int c = 0; c < 2; ++c)
        {
            eq[c][0].c = BiquadCoeffs::lowShelf (120.0f, gl, sr);
            eq[c][1].c = BiquadCoeffs::peak (fm, q, gm, sr);
            eq[c][2].c = BiquadCoeffs::highShelf (6000.0f, gh, sr);
            float* x = c ? R : L;
            for (int i = 0; i < n; ++i) x[i] = eq[c][2].tick (eq[c][1].tick (eq[c][0].tick (x[i])));
        }
    }

    // ---- Transientes: dois seguidores de envolvente (rápido/lento); a diferença em dB separa ataque de sustentação ----------
    //  p0 ataque ±12 dB · p1 sustentação ±12 dB · p2 velocidade (escala dos tempos) · p3 saída ±12 dB · p4 suavização do ganho.
    //  Tudo a 0,5 = neutro. Sinais estacionários (d≈0) passam sem alteração.
    float tsF = 0, tsS = 0, tsG = 1.0f;
    void doTransient (const float* p, float* L, float* R, int n)
    {
        const float atk = mapLin (p[0], -12, 12), sus = mapLin (p[1], -12, 12), spd = mapLog (p[2], 0.4f, 2.5f), out = dbToGain (mapLin (p[3], -12, 12));
        const float fa = std::exp (-1.0f / (0.001f * 0.8f * spd * sr)), fr = std::exp (-1.0f / (0.001f * 45.0f * spd * sr));
        const float sa = std::exp (-1.0f / (0.001f * 30.0f * spd * sr));                           // o lento é o rápido suavizado: em regime estacionário os dois coincidem (d = 0)
        const float gs = std::exp (-1.0f / (0.001f * mapLog (p[4], 0.3f, 6.0f) * sr));
        for (int i = 0; i < n; ++i)
        {
            const float x = std::max (std::fabs (L[i]), std::fabs (R[i])) + 1.0e-9f;
            tsF = x > tsF ? fa * tsF + (1 - fa) * x : fr * tsF + (1 - fr) * x;
            tsS = sa * tsS + (1 - sa) * tsF;
            const float d = 8.685889638f * std::log ((tsF + 1.0e-7f) / (tsS + 1.0e-7f));            // dB: >0 no ataque, <0 na queda
            const float gdb = atk * clampf (d / 6.0f, 0.0f, 1.0f) + sus * clampf (-d / 3.0f, 0.0f, 1.0f);
            const float g = dbToGain (gdb);
            tsG = g + (tsG - g) * gs;
            L[i] *= tsG * out; R[i] *= tsG * out;
        }
    }

    // ---- Sidechain interno: abaixamento do nível por tempo (alinhado ao compasso do anfitrião) ou por notas ----------------
    //  p0 divisão (1/16 · 1/8 · 1/4 · 1/2 · compasso) · p1 profundidade · p2 libertação · p3 curva · p4 fonte (≥0,5 = tempo, <0,5 = notas).
    //  Não há entrada de sidechain externa: o gatilho é o relógio do projeto ou as notas tocadas neste instrumento.
    float duckT = 1.0e9f, duckG = 1.0f; double ppq0 = 0; bool trigPending = false;
    void doDuck (const float* p, float* L, float* R, int n, float bpm)
    {
        static const float divs[5] = { 0.25f, 0.5f, 1.0f, 2.0f, 4.0f };
        const float div = divs[(int) std::lround (clampf (p[0], 0.0f, 1.0f) * 4.0f)];
        const float gMin = std::max (0.001f, 1.0f - clampf (p[1], 0.0f, 1.0f)), rel = mapLog (p[2], 0.02f, 0.8f), curve = mapLog (p[3], 0.35f, 3.0f);
        const bool tempo = p[4] >= 0.5f; const float sm = std::exp (-1.0f / (0.0012f * sr));
        const double beatsPerSample = (double) std::max (20.0f, bpm) / (60.0 * (double) sr);
        if (! tempo && trigPending) duckT = 0.0f;
        trigPending = false;
        for (int i = 0; i < n; ++i)
        {
            float t;
            if (tempo) { const double b = ppq0 + (double) i * beatsPerSample; t = (float) ((b - std::floor (b / div) * div) * 60.0 / std::max (20.0f, bpm)); }
            else { t = duckT; duckT += 1.0f / sr; }
            const float r = std::min (1.0f, t / rel);
            const float target = gMin + (1.0f - gMin) * std::pow (r, curve);
            duckG = target + (duckG - target) * sm;
            L[i] *= duckG; R[i] *= duckG;
        }
    }

    // ---- Convolução: IR procedurais (Quarto/Sala/Hall/Placa/Catedral/Mola), convolução particionada em FFT -----------------
    //  p0 IR · p1 pré-delay · p2 tom (inclinação ±9 dB) · p3 tamanho (fração da IR) · p4 largura.  Latência do molhado = 1 bloco (≈10,7 ms).
    Convolver conv;
    void doConv (const float* p, float* L, float* R, int n)
    {
        const int ir = (int) std::lround (clampf (p[0], 0.0f, 1.0f) * (float) (kConvNumIr - 1));
        const float pd = clampf (p[1], 0.0f, 1.0f) * clampf (p[1], 0.0f, 1.0f) * 0.15f * sr, tilt = mapLin (p[2], -9, 9), frac = mapLin (p[3], 0.2f, 1.0f), wd_ = mapLin (p[4], 0.0f, 2.0f);
        for (int i = 0; i < n; ++i) { pre[0].write (L[i]); pre[1].write (R[i]); L[i] = pre[0].read (std::max (1.0f, pd)); R[i] = pre[1].read (std::max (1.0f, pd)); }
        conv.process (ir, frac, L, R, n);
        for (int c = 0; c < 2; ++c)
        {
            eq[c][0].c = BiquadCoeffs::lowShelf (300.0f, -tilt, sr); eq[c][2].c = BiquadCoeffs::highShelf (3000.0f, tilt, sr);
            float* x = c ? R : L; for (int i = 0; i < n; ++i) x[i] = eq[c][2].tick (eq[c][0].tick (x[i]));
        }
        for (int i = 0; i < n; ++i) { const float m = 0.5f * (L[i] + R[i]), s = 0.5f * (L[i] - R[i]) * wd_; L[i] = m + s; R[i] = m - s; }
    }

    // ---- Compressor ----------------------------------------------------------------
    float envC = 0;
    void doComp (const float* p, float* L, float* R, int n)
    {
        const float th = mapLin (p[0], -40, 0), ratio = mapLog (p[1], 1.5f, 20.0f);
        const float at = std::exp (-1.0f / (0.001f * mapLog (p[2], 0.5f, 50.0f) * sr)), rl = std::exp (-1.0f / (0.001f * mapLog (p[3], 20, 500) * sr));
        const float mk = dbToGain (mapLin (p[4], 0, 18));
        for (int i = 0; i < n; ++i)
        {
            const float pk = std::max (std::fabs (L[i]), std::fabs (R[i]));
            envC = pk > envC ? at * envC + (1 - at) * pk : rl * envC + (1 - rl) * pk;
            const float db = gainToDb (envC), over = db - th;
            const float gr = over > 0 ? -over * (1.0f - 1.0f / ratio) : 0.0f;
            const float g = dbToGain (gr) * mk;
            L[i] *= g; R[i] *= g;
        }
    }

    // ---- Saturador -----------------------------------------------------------------
    float satLp[2] = {};
    static float shaper (float v, float kind)
    {
        const float soft = fastTanh (v), hard = clampf (v, -1.0f, 1.0f), fold = std::sin (clampf (v, -3.2f, 3.2f) * 1.2f);
        return kind < 0.5f ? lerpf (soft, hard, kind * 2.0f) : lerpf (hard, fold, (kind - 0.5f) * 2.0f);
    }
    void doSat (const float* p, float* L, float* R, int n)
    {
        const float drive = dbToGain (mapLin (p[0], 0, 30)), fc = mapLog (p[1], 1000, 20000), bias = mapLin (p[2], 0, 0.4f), kind = p[3], out = dbToGain (mapLin (p[4], -12, 0) + 6.0f * 0.0f);
        const float a = 1.0f - std::exp (-kTwoPi * fc / sr);
        const float comp = 1.0f / (1.0f + 0.12f * (drive - 1.0f));
        for (int c = 0; c < 2; ++c)
        {
            float* x = c ? R : L;
            for (int i = 0; i < n; ++i)
            {
                const float y = shaper (x[i] * drive + bias, kind) - shaper (bias, kind);
                satLp[c] += a * (y - satLp[c]);
                x[i] = flushDenorm (satLp[c]) * comp * out * 1.4f;
            }
        }
    }

    // ---- Chorus (3 vozes) ----------------------------------------------------------
    DelayLine ch[2]; float phase = 0;
    void doChorus (const float* p, float* L, float* R, int n)
    {
        const float rate = mapLog (p[0], 0.1f, 5.0f), depth = mapLin (p[1], 0.2f, 8.0f) * 0.001f * sr, center = mapLin (p[2], 5, 25) * 0.001f * sr, wide = p[4];
        const float inc = rate / sr;
        for (int i = 0; i < n; ++i)
        {
            ch[0].write (L[i]); ch[1].write (R[i]);
            float oL = 0, oR = 0;
            for (int v = 0; v < 3; ++v)
            {
                const float ph = phase + (float) v / 3.0f;
                const float m = std::sin (kTwoPi * (ph - std::floor (ph)));
                const float d = center + depth * m + (float) v * 0.4f * 0.001f * sr * p[3];
                const float sL = ch[0].read (std::max (d, 2.0f)), sR = ch[1].read (std::max (d, 2.0f));
                const float pan = (v == 0 ? -1.0f : (v == 1 ? 0.0f : 1.0f)) * wide;
                oL += sL * (0.5f - 0.5f * pan) * 0.9f + sR * 0.0f; oR += sR * (0.5f + 0.5f * pan) * 0.9f;
                oL += sL * 0.25f; oR += sR * 0.25f;
            }
            phase += inc; if (phase >= 1) phase -= 1;
            L[i] = oL * 0.6f; R[i] = oR * 0.6f;
        }
    }

    // ---- Delay (livre em ms ou sincronizado ao tempo) -------------------------------------
    //  Realimentação com filtro passa-baixo + passa-alto e saturação suave (tipo fita), tempo com
    //  deslize (sem estalidos ao mexer) e uma "oscilação" mínima para dar vida.
    DelayLine dl[2]; float fbLp[2] = {}, fbHp[2] = {}, dSm[2] = { -1.0f, -1.0f }, wowPh = 0;
    void doDelay (const float* p, float* L, float* R, int n, float bpm, bool sync)
    {
        static const float beats[8] = { 2.0f, 1.5f, 1.0f, 0.75f, 0.5f, 1.0f / 3.0f, 0.25f, 1.0f / 6.0f };   // 1/2, 1/4., 1/4, 1/8., 1/8, 1/8T, 1/16, 1/16T
        const float tms = sync ? beats[(int) std::lround (clampf (p[0], 0.0f, 1.0f) * 7.0f)] * 60000.0f / std::max (20.0f, bpm) : mapLog (p[0], 30.0f, 1500.0f);
        const float fb = mapLin (p[1], 0.0f, 0.95f), fc = mapLog (p[2], 1000.0f, 16000.0f), pp = p[3], off = sync ? (1.0f + 0.5f * p[4] * 0.0f) : 1.0f + 0.5f * p[4];
        const float maxD = (float) dl[0].size - 6.0f;
        const float tL = std::min (tms * 0.001f * sr, maxD), tR = std::min (tms * (sync && p[4] > 0.5f ? 1.5f : off) * 0.001f * sr, maxD);
        if (dSm[0] < 0) { dSm[0] = tL; dSm[1] = tR; }
        const float a = 1.0f - std::exp (-kTwoPi * fc / sr), hpA = 1.0f - std::exp (-kTwoPi * 120.0f / sr), sm = 1.0f - std::exp (-1.0f / (0.06f * sr));
        const float wowDepth = 0.0004f * sr, wowInc = 0.35f / sr;
        for (int i = 0; i < n; ++i)
        {
            dSm[0] += (tL - dSm[0]) * sm; dSm[1] += (tR - dSm[1]) * sm;
            wowPh += wowInc; if (wowPh >= 1.0f) wowPh -= 1.0f;
            const float w = wowDepth * std::sin (kTwoPi * wowPh);
            const float yL = dl[0].read (std::max (2.0f, dSm[0] + w)), yR = dl[1].read (std::max (2.0f, dSm[1] - w));
            fbLp[0] += a * (yL - fbLp[0]); fbLp[1] += a * (yR - fbLp[1]);
            fbHp[0] += hpA * (fbLp[0] - fbHp[0]); fbHp[1] += hpA * (fbLp[1] - fbHp[1]);
            const float fL = fastTanh ((fbLp[0] - fbHp[0]) * 1.3f) * 0.77f, fR = fastTanh ((fbLp[1] - fbHp[1]) * 1.3f) * 0.77f;
            const float inL = L[i], inR = R[i];
            dl[0].write (flushDenorm (inL + (fL * (1 - pp) + fR * pp) * fb));
            dl[1].write (flushDenorm (inR + (fR * (1 - pp) + fL * pp) * fb));
            L[i] = yL; R[i] = yR;
        }
    }

    // ---- Reverb (FDN de 8 linhas, matriz de Hadamard) -------------------------------
    DelayLine rv[8], rvAp[6], pre[2], er[2]; int baseLen[8] = {}, rvApLen[6] = {}; float inHp = 0; float rvLp[8] = {}; float lfoPh = 0;
    static constexpr float erL[6] = { 7.0f, 13.0f, 19.0f, 29.0f, 41.0f, 53.0f }, erR[6] = { 5.0f, 11.0f, 23.0f, 31.0f, 37.0f, 47.0f }, erG[6] = { 0.6f, 0.5f, 0.45f, 0.35f, 0.3f, 0.2f };
    void doReverb (const float* p, float* L, float* R, int n)
    {
        const float size = mapLin (p[0], 0.4f, 1.5f), rt60 = mapLog (p[1], 0.3f, 14.0f), damp = p[2], pd = mapLin (p[3], 0.0f, 0.2f) * sr, wide = p[4];
        float g[8], len[8];
        for (int i = 0; i < 8; ++i)
        {
            len[i] = std::min ((float) baseLen[i] * size, (float) rv[i].size - 8.0f);
            g[i] = std::pow (10.0f, -3.0f * len[i] / (sr * rt60));
        }
        const float dc = 1.0f - 0.9f * damp, lpA = clampf (dc, 0.05f, 1.0f);
        for (int i = 0; i < n; ++i)
        {
            pre[0].write (L[i]); pre[1].write (R[i]);
            float in = 0.5f * (pre[0].read (std::max (pd, 1.0f)) + pre[1].read (std::max (pd, 1.0f)));
            inHp += 0.012f * (in - inHp); in -= inHp;                 // tira a lama abaixo de ~80 Hz
            er[0].write (in); er[1].write (in);
            for (int k = 0; k < 6; ++k)   // difusão de entrada (all-pass)
            {
                const float d = rvAp[k].read ((float) rvApLen[k]);
                const float v = in + 0.6f * d; rvAp[k].write (v); in = d - 0.6f * v;
            }
            lfoPh += 0.35f / sr; if (lfoPh >= 1) lfoPh -= 1;
            float t[8];
            for (int k = 0; k < 8; ++k)
            {
                const float mod = 1.5f * std::sin (kTwoPi * (lfoPh + (float) k * 0.125f));
                const float y = rv[k].read (len[k] + mod);
                rvLp[k] += lpA * (y - rvLp[k]);
                t[k] = flushDenorm (rvLp[k]) * g[k];
            }
            // Hadamard 8x8 (sem multiplicações)
            for (int s = 1; s < 8; s <<= 1)
                for (int a = 0; a < 8; a += s * 2)
                    for (int b = a; b < a + s; ++b) { const float x = t[b], y = t[b + s]; t[b] = x + y; t[b + s] = x - y; }
            const float norm = 0.35355339f;
            float oL = 0, oR = 0;
            for (int k = 0; k < 8; ++k)
            {
                rv[k].write (clampf (in * 0.35f + t[k] * norm, -4.0f, 4.0f));
                if (k & 1) oR += rv[k].read (len[k] * 0.9f); else oL += rv[k].read (len[k] * 0.9f);
            }
            oL *= 0.35f; oR *= 0.35f;
            const float m = 0.5f * (oL + oR), s2 = 0.5f * (oL - oR) * (0.4f + 0.6f * wide);
            // reflexões iniciais (6 + 6 ecos curtos, espalhados entre L e R)
            float eL = 0, eR = 0;
            for (int k = 0; k < 6; ++k) { eL += er[0].read (erL[k] * size * sr * 0.001f) * erG[k]; eR += er[1].read (erR[k] * size * sr * 0.001f) * erG[k]; }
            L[i] = m + s2 + eL * 0.22f; R[i] = m - s2 + eR * 0.22f;
        }
    }

    // ---- Faser (4 all-pass) ----------------------------------------------------------
    float ap[2][4] = {}; float fbPh[2] = {};
    void doPhaser (const float* p, float* L, float* R, int n)
    {
        const float rate = mapLog (p[0], 0.05f, 4.0f), depth = p[1], fb = mapLin (p[2], 0, 0.9f), fc = mapLog (p[3], 200, 4000), sp = p[4] * 0.5f;
        const float inc = rate / sr;
        for (int i = 0; i < n; ++i)
        {
            for (int c = 0; c < 2; ++c)
            {
                const float ph = phase + (c ? sp : 0.0f);
                const float m = 0.5f + 0.5f * std::sin (kTwoPi * (ph - std::floor (ph)));
                const float f = fc * std::pow (2.0f, (m - 0.5f) * 4.0f * depth);
                const float tn = std::tan (kPi * clampf (f, 20.0f, sr * 0.45f) / sr), a = (tn - 1.0f) / (tn + 1.0f);
                float x = (c ? R : L)[i] + fbPh[c] * fb;
                for (int s = 0; s < 4; ++s) { const float y = a * x + ap[c][s]; ap[c][s] = flushDenorm (x - a * y); x = y; }
                fbPh[c] = x;
                (c ? R : L)[i] = ((c ? R : L)[i] + x) * 0.5f;
            }
            phase += inc; if (phase >= 1) phase -= 1;
        }
    }

    // ---- Bitcrusher ------------------------------------------------------------------
    float crushHold[2] = {}; int crushCnt = 0; float lp1[2] = {};
    void doCrush (const float* p, float* L, float* R, int n)
    {
        const float bits = mapLin (p[0], 16, 2), levels = std::pow (2.0f, bits - 1.0f);
        const int dec = (int) mapLog (p[1], 1, 50), fc = (int) mapLog (p[3], 1000, 20000); const float a = 1.0f - std::exp (-kTwoPi * (float) fc / sr);
        const float out = dbToGain (mapLin (p[4], -12, 0));
        for (int i = 0; i < n; ++i)
        {
            if (crushCnt <= 0) { crushHold[0] = std::round (L[i] * levels) / levels; crushHold[1] = std::round (R[i] * levels) / levels; crushCnt = dec; }
            --crushCnt;
            lp1[0] += a * (crushHold[0] - lp1[0]); lp1[1] += a * (crushHold[1] - lp1[1]);
            L[i] = lp1[0] * out; R[i] = lp1[1] * out;
        }
    }

    // ---- Largura (M/S + graves em mono) ------------------------------------------------
    Biquad wd[2];
    void doWidth (const float* p, float* L, float* R, int n)
    {
        const float width = mapLin (p[0], 0, 2), fc = mapLog (p[1], 20, 400), bal = mapLin (p[2], -1, 1), mg = dbToGain (mapLin (p[3], -9, 9)), sg = dbToGain (mapLin (p[4], -9, 9));
        wd[0].c = BiquadCoeffs::highpass (fc, 0.7071f, sr);
        for (int i = 0; i < n; ++i)
        {
            const float m = 0.5f * (L[i] + R[i]) * mg, s = 0.5f * (L[i] - R[i]) * sg * width;
            const float sh = wd[0].tick (s);                       // graves da side vão para o centro
            const float lo = s - sh;
            const float mm = m + 0.0f * lo, ss = sh;
            const float gl = bal > 0 ? 1.0f - bal : 1.0f, gr = bal < 0 ? 1.0f + bal : 1.0f;
            L[i] = (mm + ss) * gl; R[i] = (mm - ss) * gr;
        }
    }

    // ---- Trémolo / auto-pan ---------------------------------------------------------------
    void doTrem (const float* p, float* L, float* R, int n)
    {
        const float rate = mapLog (p[0], 0.5f, 20.0f), depth = p[1], shape = p[2], sp = p[3] * 0.5f; const float inc = rate / sr;
        for (int i = 0; i < n; ++i)
        {
            for (int c = 0; c < 2; ++c)
            {
                const float ph = phase + (c ? sp : 0.0f);
                float m = std::sin (kTwoPi * (ph - std::floor (ph)));
                const float sq = clampf (m * 6.0f, -1.0f, 1.0f); m = lerpf (m, sq, shape);
                (c ? R : L)[i] *= 1.0f - depth * (0.5f - 0.5f * m);
            }
            phase += inc; if (phase >= 1) phase -= 1;
        }
    }

    // ============================ v1.5: efeitos novos ============================
    DelayLine fl[2]; PitchShifter psA[2], psB[2]; float flFb[2] = {}, ensPh = 0, ensLp[2] = {}, mtLp[2] = {}, shFb[2] = {};
    int shN = 0; float shBufL[32] = {}, shBufR[32] = {};

    // ---- Flanger: atraso curto modulado + realimentação (p: taxa, profundidade, feedback ±, atraso base, largura) ----
    void doFlanger (const float* p, float* L, float* R, int n)
    {
        const float rate = mapLog (p[0], 0.05f, 5.0f), depth = p[1], fb = mapLin (p[2], -0.9f, 0.9f), man = mapLog (p[3], 0.3f, 6.0f) * 0.001f * sr, sp = p[4] * 0.5f;
        const float inc = rate / sr;
        for (int i = 0; i < n; ++i)
        {
            for (int c = 0; c < 2; ++c)
            {
                float* x = c ? R : L; const float in = x[i];
                const float ph = phase + (c ? sp : 0.0f), lfo = std::sin (kTwoPi * (ph - std::floor (ph)));
                const float d = clampf (man * (1.0f + 0.9f * depth * lfo), 2.0f, sr * 0.028f);
                const float y = fl[c].read (d);
                fl[c].write (fastTanh (in + fb * flFb[c]));
                flFb[c] = flushDenorm (y);
                x[i] = 0.5f * (in + y);
            }
            phase += inc; if (phase >= 1.0f) phase -= 1.0f;
        }
    }

    // ---- Ensemble: 6 vozes com duas LFOs lentas (p: taxa, profundidade, atraso, brilho, largura) ----
    void doEnsemble (const float* p, float* L, float* R, int n)
    {
        const float rate = mapLog (p[0], 0.1f, 3.0f), depth = mapLin (p[1], 0.5f, 6.0f) * 0.001f * sr, center = mapLin (p[2], 8, 28) * 0.001f * sr, wide = p[4];
        const float a = 1.0f - std::exp (-kTwoPi * mapLog (p[3], 1500, 16000) / sr), inc = rate / sr;
        for (int i = 0; i < n; ++i)
        {
            ch[0].write (L[i]); ch[1].write (R[i]);
            float oL = 0, oR = 0;
            for (int v = 0; v < 6; ++v)
            {
                const float ph1 = ensPh + (float) v / 6.0f, ph2 = ensPh * 0.37f + (float) v * 0.21f;
                const float m = 0.7f * std::sin (kTwoPi * (ph1 - std::floor (ph1))) + 0.3f * std::sin (kTwoPi * (ph2 - std::floor (ph2)));
                const float d = std::max (2.0f, center + depth * m + (float) v * 0.0007f * sr);
                const float pan = ((v & 1) ? 1.0f : -1.0f) * wide * (0.4f + 0.12f * (float) v);
                const float sL = ch[0].read (d), sR = ch[1].read (d);
                oL += sL * (1.0f - std::max (0.0f, pan)) * 0.3f + sR * std::max (0.0f, -pan) * 0.3f;
                oR += sR * (1.0f - std::max (0.0f, -pan)) * 0.3f + sL * std::max (0.0f, pan) * 0.3f;
            }
            ensLp[0] += a * (oL - ensLp[0]); ensLp[1] += a * (oR - ensLp[1]);
            ensPh += inc; if (ensPh >= 1.0f) ensPh -= 1.0f;
            L[i] = 0.5f * L[i] + ensLp[0]; R[i] = 0.5f * R[i] + ensLp[1];
        }
    }

    // ---- Atraso de 4 taps (p: tempo base, espalhamento, feedback, tom, estéreo) ----
    void doMultiTap (const float* p, float* L, float* R, int n)
    {
        const float base = mapLog (p[0], 30.0f, 500.0f) * 0.001f * sr, spread = lerpf (0.5f, 1.0f, p[1]), fb = mapLin (p[2], 0.0f, 0.75f), st = p[4];
        const float a = 1.0f - std::exp (-kTwoPi * mapLog (p[3], 800, 12000) / sr);
        static const float gain[4] = { 1.0f, 0.72f, 0.52f, 0.38f };
        for (int i = 0; i < n; ++i)
        {
            float tap[2][4];
            for (int c = 0; c < 2; ++c)
                for (int k = 0; k < 4; ++k) tap[c][k] = dl[c].read (std::min (base * (float) (k + 1) * spread, sr * 2.3f)) * gain[k];
            for (int c = 0; c < 2; ++c) { mtLp[c] += a * (tap[c][3] - mtLp[c]); mtLp[c] = flushDenorm (mtLp[c]); }
            dl[0].write (fastTanh (L[i] + fb * mtLp[0])); dl[1].write (fastTanh (R[i] + fb * mtLp[1]));
            float oL = 0, oR = 0;
            for (int k = 0; k < 4; ++k)
            {
                const float gL = 1.0f - st * ((k & 1) ? 1.0f : 0.0f), gR = 1.0f - st * ((k & 1) ? 0.0f : 1.0f);   // taps alternados esquerda/direita
                oL += tap[0][k] * gL; oR += tap[1][k] * gR;
            }
            L[i] = oL * 0.7f; R[i] = oR * 0.7f;
        }
    }

    // ---- Transposição (p: semitons ±12, afinação fina ±50 cents, tamanho do grão, feedback, estéreo) ----
    void doPitch (const float* p, float* L, float* R, int n)
    {
        const float st = std::round (mapLin (p[0], -12.0f, 12.0f)) + mapLin (p[1], -0.5f, 0.5f), grain = mapLin (p[2], 0.025f, 0.08f) * sr, fb = mapLin (p[3], 0.0f, 0.6f), det = (p[4] - 0.5f) * 0.2f;
        const float rL = std::pow (2.0f, (st + det) / 12.0f), rR = std::pow (2.0f, (st - det) / 12.0f);
        for (int i = 0; i < n; ++i)
        {
            const float yL = psA[0].tick (L[i] + fb * shFb[0], rL, grain), yR = psA[1].tick (R[i] + fb * shFb[1], rR, grain);
            shFb[0] = flushDenorm (fastTanh (yL)); shFb[1] = flushDenorm (fastTanh (yR));
            L[i] = yL; R[i] = yR;
        }
    }

    // ---- Shimmer: reverb com +12 semitons na realimentação (p: tamanho, decaimento, amortecimento, shimmer, largura) ----
    //  a realimentação passa por um anel de 32 amostras e o reverb corre em segmentos de até 32, por isso o
    //  resultado não depende do tamanho do bloco do anfitrião
    void doShimmer (const float* p, float* L, float* R, int n)
    {
        const float rp[5] = { p[0], p[1], p[2], 0.1f, p[4] };
        const float amt = p[3], fbg = 0.45f * amt, grain = 0.05f * sr;
        int done = 0;
        while (done < n)
        {
            const int m = std::min (32, n - done);
            float* l = L + done; float* r = R + done;
            for (int i = 0; i < m; ++i) { const int k = (shN + i) & 31; l[i] += fbg * shBufL[k]; r[i] += fbg * shBufR[k]; }
            doReverb (rp, l, r, m);
            for (int i = 0; i < m; ++i)
            {
                const int k = (shN + i) & 31;
                const float uL = psB[0].tick (l[i], 2.0f, grain), uR = psB[1].tick (r[i], 2.0f, grain);
                shBufL[k] = flushDenorm (fastTanh (uL)); shBufR[k] = flushDenorm (fastTanh (uR));
                l[i] += amt * uL; r[i] += amt * uR;
            }
            shN = (shN + m) & 31; done += m;
        }
    }
};

// ----------------------------------------------------------------------------
//  Limitador "lookahead" de pico (1.5 ms) com libertação suave
// ----------------------------------------------------------------------------
class Limiter
{
public:
    void prepare (float sampleRate)
    {
        sr = sampleRate; la = std::max (8, (int) (0.0015f * sr));
        bufL.assign ((size_t) la + 1, 0.0f); bufR.assign ((size_t) la + 1, 0.0f); dqI.assign ((size_t) la + 3, 0); dqV.assign ((size_t) la + 3, 0.0f); reset();
        relC = std::exp (-1.0f / (0.08f * sr)); atkC = 1.0f - std::exp (-1.0f / (0.0004f * sr));
    }
    void reset() { std::fill (bufL.begin(), bufL.end(), 0.0f); std::fill (bufR.begin(), bufR.end(), 0.0f); dqHead = dqTail = dqCount = 0; cnt = 0; pos = 0; gain = 1.0f; }
    int latency() const { return la; }
    void process (float* L, float* R, int n, float ceilingDb, bool on)
    {
        const float ceil = dbToGain (ceilingDb);
        for (int i = 0; i < n; ++i)
        {
            const float pk = std::max (std::fabs (L[i]), std::fabs (R[i]));
            bufL[(size_t) pos] = L[i]; bufR[(size_t) pos] = R[i];
            // máximo da janela de lookahead (la+1 amostras) por fila monótona: O(1) amortizado (auditoria C-06)
            const int cap = la + 3;
            while (dqCount > 0 && dqV[(size_t) ((dqTail + cap - 1) % cap)] <= pk) { dqTail = (dqTail + cap - 1) % cap; --dqCount; }
            dqI[(size_t) dqTail] = cnt; dqV[(size_t) dqTail] = pk; dqTail = (dqTail + 1) % cap; ++dqCount;
            while (dqCount > 0 && dqI[(size_t) dqHead] <= cnt - (long) (la + 1)) { dqHead = (dqHead + 1) % cap; --dqCount; }
            const float m = dqV[(size_t) dqHead]; ++cnt;
            const float target = (on && m > ceil) ? ceil / m : 1.0f;
            gain += (target - gain) * (target < gain ? atkC : (1.0f - relC));
            int rp = pos + 1; if (rp > la) rp = 0;                 // amostra mais antiga
            L[i] = bufL[(size_t) rp] * gain; R[i] = bufR[(size_t) rp] * gain;
            if (on) { if (L[i] > ceil) L[i] = ceil; if (L[i] < -ceil) L[i] = -ceil; if (R[i] > ceil) R[i] = ceil; if (R[i] < -ceil) R[i] = -ceil; }
            pos = rp;
        }
    }
private:
    float sr = 44100, gain = 1, relC = 0.99f, atkC = 0.2f; int la = 66, pos = 0; std::vector<float> bufL, bufR; std::vector<long> dqI; std::vector<float> dqV; int dqHead = 0, dqTail = 0, dqCount = 0; long cnt = 0;
};
} // namespace sk
