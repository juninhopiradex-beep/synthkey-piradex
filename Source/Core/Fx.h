// =============================================================================
//  SYNTHKEY by Piradex — efeitos (10 tipos, 5 parâmetros normalizados 0..1 + mix)
//  Vazio, EQ, Compressor, Saturador, Chorus, Delay, Reverb, Faser, Bitcrusher,
//  Largura, Trémolo. Todos stereo, in-place, sem alocações no process().
// =============================================================================
#pragma once
#include "Dsp.h"

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
        const int i0 = (int) rp; const int i1 = i0 + 1 >= size ? 0 : i0 + 1; const float f = rp - (float) i0;
        return buf[(size_t) i0] + (buf[(size_t) i1] - buf[(size_t) i0]) * f;
    }
};

class FxUnit
{
public:
    enum Type { Empty = 0, EQ, Comp, Sat, Chorus, Delay, Reverb, Phaser, Crush, Width, Trem };

    void prepare (float sampleRate, int maxBlock)
    {
        sr = sampleRate;
        dBuf.assign ((size_t) std::max (maxBlock, 64), 0.0f);
        dl[0].allocate ((int) (sr * 1.7f)); dl[1].allocate ((int) (sr * 1.7f));
        ch[0].allocate ((int) (sr * 0.08f)); ch[1].allocate ((int) (sr * 0.08f));
        pre[0].allocate ((int) (sr * 0.25f)); pre[1].allocate ((int) (sr * 0.25f));
        const float scale = sr / 44100.0f;
        static const int base[8] = { 1103, 1327, 1531, 1783, 2017, 2243, 2477, 2707 };
        for (int i = 0; i < 8; ++i) { baseLen[i] = (int) ((float) base[i] * scale); rv[i].allocate ((int) ((float) baseLen[i] * 1.7f) + 64); }
        static const int apl[4] = { 142, 107, 379, 277 };
        for (int i = 0; i < 4; ++i) { rvAp[i].allocate ((int) ((float) apl[i] * scale) + 4); rvApLen[i] = (int) ((float) apl[i] * scale); }
        reset();
    }
    void reset()
    {
        for (auto& d : dl) d.clear(); for (auto& d : ch) d.clear(); for (auto& d : pre) d.clear();
        for (auto& d : rv) d.clear(); for (auto& d : rvAp) d.clear();
        for (auto& v : rvLp) v = 0; for (auto& b : eq) for (auto& x : b) x.reset();
        envC = 0; for (auto& v : lp1) v = 0; for (auto& v : fbLp) v = 0;
        for (auto& a : ap) for (auto& v : a) v = 0; fbPh[0] = fbPh[1] = 0;
        phase = 0; crushHold[0] = crushHold[1] = 0; crushCnt = 0; for (auto& b : wd) b.reset();
        satLp[0] = satLp[1] = 0; lastType = -1;
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
            case Delay: doDelay (p, L, R, n, bpm); break;
            case Reverb: doReverb (p, L, R, n); break;
            case Phaser: doPhaser (p, L, R, n); break;
            case Crush: doCrush (p, L, R, n); break;
            case Width: doWidth (p, L, R, n); break;
            case Trem: doTrem (p, L, R, n); break;
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

    // ---- Delay ---------------------------------------------------------------------
    DelayLine dl[2]; float fbLp[2] = {};
    void doDelay (const float* p, float* L, float* R, int n, float)
    {
        const float tms = mapLog (p[0], 30.0f, 1500.0f), fb = mapLin (p[1], 0.0f, 0.95f), fc = mapLog (p[2], 1000.0f, 16000.0f), pp = p[3], off = 1.0f + 0.5f * p[4];
        const float dL = std::min (tms * 0.001f * sr, (float) dl[0].size - 4.0f), dR = std::min (tms * off * 0.001f * sr, (float) dl[0].size - 4.0f);
        const float a = 1.0f - std::exp (-kTwoPi * fc / sr);
        for (int i = 0; i < n; ++i)
        {
            const float yL = dl[0].read (dL), yR = dl[1].read (dR);
            fbLp[0] += a * (yL - fbLp[0]); fbLp[1] += a * (yR - fbLp[1]);
            const float fL = fbLp[0], fR = fbLp[1];
            // ping-pong: cruza as realimentações
            const float inL = L[i], inR = R[i];
            dl[0].write (flushDenorm (inL + (fL * (1 - pp) + fR * pp) * fb));
            dl[1].write (flushDenorm (inR + (fR * (1 - pp) + fL * pp) * fb));
            L[i] = yL; R[i] = yR;
        }
    }

    // ---- Reverb (FDN de 8 linhas, matriz de Hadamard) -------------------------------
    DelayLine rv[8], rvAp[4], pre[2]; int baseLen[8] = {}, rvApLen[4] = {}; float rvLp[8] = {}; float lfoPh = 0;
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
            for (int k = 0; k < 4; ++k)   // difusão de entrada (all-pass)
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
            L[i] = m + s2; R[i] = m - s2;
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
        bufL.assign ((size_t) la + 1, 0.0f); bufR.assign ((size_t) la + 1, 0.0f); win.assign ((size_t) la + 1, 0.0f); reset();
        relC = std::exp (-1.0f / (0.08f * sr)); atkC = 1.0f - std::exp (-1.0f / (0.0004f * sr));
    }
    void reset() { std::fill (bufL.begin(), bufL.end(), 0.0f); std::fill (bufR.begin(), bufR.end(), 0.0f); std::fill (win.begin(), win.end(), 0.0f); pos = 0; gain = 1.0f; }
    int latency() const { return la; }
    void process (float* L, float* R, int n, float ceilingDb, bool on)
    {
        const float ceil = dbToGain (ceilingDb);
        for (int i = 0; i < n; ++i)
        {
            const float pk = std::max (std::fabs (L[i]), std::fabs (R[i]));
            bufL[(size_t) pos] = L[i]; bufR[(size_t) pos] = R[i]; win[(size_t) pos] = pk;
            // máximo na janela de lookahead (pequena: varredura direta)
            float m = 0; for (int k = 0; k <= la; ++k) m = std::max (m, win[(size_t) k]);
            const float target = (on && m > ceil) ? ceil / m : 1.0f;
            gain += (target - gain) * (target < gain ? atkC : (1.0f - relC));
            int rp = pos + 1; if (rp > la) rp = 0;                 // amostra mais antiga
            L[i] = bufL[(size_t) rp] * gain; R[i] = bufR[(size_t) rp] * gain;
            if (on) { if (L[i] > ceil) L[i] = ceil; if (L[i] < -ceil) L[i] = -ceil; if (R[i] > ceil) R[i] = ceil; if (R[i] < -ceil) R[i] = -ceil; }
            pos = rp;
        }
    }
private:
    float sr = 44100, gain = 1, relC = 0.99f, atkC = 0.2f; int la = 66, pos = 0; std::vector<float> bufL, bufR, win;
};
} // namespace sk
