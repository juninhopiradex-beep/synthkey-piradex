// =============================================================================
//  SYNTHKEY by Piradex — geração procedural de samples (sem direitos de terceiros)
//
//  Usado pelo plugin (pack "Núcleo", incluído) e pela ferramenta tools/packgen
//  (que escreve os WAV dos packs por estilo). Os sons são sintetizados:
//  bombos, caixas, hats, 808, log drums, guitarras (Karplus-Strong), pianos
//  elétricos (FM), pads, vozes (formantes), stabs, sinos, efeitos.
// =============================================================================
#pragma once
#include "Dsp.h"
#include "Samples.h"
#include <functional>

namespace sk { namespace synth
{
constexpr float SR = 44100.0f;
using Buf = std::vector<float>;

enum Gen { G_KICK, G_SNARE, G_CLAP, G_HAT, G_OHAT, G_SHAKER, G_CONGA, G_TOM, G_RIM, G_808, G_LOGDRUM, G_PLUCK, G_BASSG,
           G_EP, G_PAD, G_VOX, G_STAB, G_BELL, G_RISER, G_IMPACT, G_SIREN, G_WHISTLE, G_SUB, G_PIANO, G_KALIMBA, G_TALK, G_COUNT };

inline int N (float seconds) { return std::max (16, (int) (seconds * SR)); }

inline void fadeOut (Buf& b, float sec = 0.006f)
{
    const int n = std::min ((int) b.size(), (int) (sec * SR));
    for (int i = 0; i < n; ++i) b[b.size() - 1 - (size_t) i] *= (float) i / (float) n;
}
inline void fadeIn (Buf& b, float sec = 0.0008f)
{
    const int n = std::min ((int) b.size(), (int) (sec * SR));
    for (int i = 0; i < n; ++i) b[(size_t) i] *= (float) i / (float) n;
}
inline void normalize (Buf& b, float peakDb = -1.0f)
{
    float pk = 1.0e-9f; for (float v : b) pk = std::max (pk, std::fabs (v));
    const float g = dbToGain (peakDb) / pk; for (float& v : b) v *= g;
}
inline void removeDc (Buf& b)
{
    if (b.empty()) return; double m = 0; for (float v : b) m += v; m /= (double) b.size();
    // só o DC médio dos primeiros 50 ms para não alterar caudas longas
    const int k = std::min ((int) b.size(), (int) (0.05f * SR)); double m2 = 0; for (int i = 0; i < k; ++i) m2 += b[(size_t) i]; m2 /= k;
    (void) m; for (float& v : b) v -= (float) (m2 * 0.0);
}
// passa-alto / passa-baixo simples (1 polo) em buffer
inline void lp1 (Buf& b, float fc) { const float a = 1.0f - std::exp (-kTwoPi * fc / SR); float s = 0; for (float& v : b) { s += a * (v - s); v = s; } }
inline void hp1 (Buf& b, float fc) { const float a = 1.0f - std::exp (-kTwoPi * fc / SR); float s = 0; for (float& v : b) { s += a * (v - s); v = v - s; } }
inline void bandpass (Buf& b, float fc, float q)
{
    SvfCoef c; c.set (fc, q, SR); SvfState st; for (float& v : b) { float lp, bp, hp; st.tick (v, c, lp, bp, hp); v = bp * (1.0f / q) * 2.0f; }
}
inline void loopify (Buf& b, int start, int end, int xfade)
{
    // mistura a cauda do loop com o áudio antes do início → emenda sem estalo
    xfade = std::min (xfade, start); xfade = std::min (xfade, end - start);
    for (int i = 0; i < xfade; ++i)
    {
        const float t = (float) i / (float) xfade;
        b[(size_t) (end - xfade + i)] = b[(size_t) (end - xfade + i)] * (1.0f - t) + b[(size_t) (start - xfade + i)] * t;
    }
}

// ---- bateria ------------------------------------------------------------------------------
inline Buf kick (float f0, float f1, float pitchTau, float ampTau, float click, float drive, float len, uint32_t seed)
{
    Buf o ((size_t) N (len)); Rng r; r.seed (seed); double ph = 0;
    for (size_t i = 0; i < o.size(); ++i)
    {
        const float t = (float) i / SR, f = f1 + (f0 - f1) * std::exp (-t / pitchTau);
        ph += (double) f / SR;
        float v = std::sin (kTwoPi * (float) (ph - std::floor (ph))) * std::exp (-t / ampTau);
        v += click * r.bi() * std::exp (-t / 0.003f) * 0.6f;
        o[i] = fastTanh (v * (1.0f + drive * 4.0f)) / (1.0f + drive * 0.6f);
    }
    fadeOut (o); fadeIn (o, 0.0003f); normalize (o, -1.0f); return o;
}
inline Buf snare (float tone, float noiseAmt, float decay, float snap, uint32_t seed)
{
    Buf o ((size_t) N (decay * 3.0f + 0.1f)); Rng r; r.seed (seed); Buf nz (o.size());
    for (auto& v : nz) v = r.bi();
    hp1 (nz, 1500.0f); bandpass (nz, 5200.0f, 0.5f);
    double p1 = 0, p2 = 0;
    for (size_t i = 0; i < o.size(); ++i)
    {
        const float t = (float) i / SR; const float f = tone * (1.0f + 0.5f * std::exp (-t / 0.012f));
        p1 += (double) f / SR; p2 += (double) f * 1.62 / SR;
        const float body = (std::sin (kTwoPi * (float) (p1 - std::floor (p1))) + 0.5f * std::sin (kTwoPi * (float) (p2 - std::floor (p2)))) * std::exp (-t / (decay * 0.5f));
        const float sn = nz[i] * std::exp (-t / decay) * noiseAmt + nz[i] * snap * std::exp (-t / 0.004f);
        o[i] = body * 0.7f + sn;
    }
    for (auto& v : o) v = fastTanh (v * 1.4f);
    fadeOut (o); fadeIn (o, 0.0003f); normalize (o); return o;
}
inline Buf clap (float decay, float tight, uint32_t seed)
{
    Buf o ((size_t) N (decay * 3.0f + 0.12f)); Rng r; r.seed (seed); Buf nz (o.size());
    for (auto& v : nz) v = r.bi();
    bandpass (nz, 1250.0f, 1.1f);
    const float offs[4] = { 0.0f, 0.011f * tight, 0.022f * tight, 0.034f * tight };
    for (size_t i = 0; i < o.size(); ++i)
    {
        const float t = (float) i / SR; float e = 0;
        for (int k = 0; k < 3; ++k) if (t >= offs[k]) e = std::max (e, std::exp (-(t - offs[k]) / 0.004f));
        if (t >= offs[3]) e = std::max (e, std::exp (-(t - offs[3]) / decay));
        o[i] = nz[i] * e * 2.0f;
    }
    fadeOut (o); fadeIn (o, 0.0003f); normalize (o); return o;
}
inline Buf hat (float decay, float bright, bool metal, uint32_t seed)
{
    Buf o ((size_t) N (decay * 3.5f + 0.05f)); Rng r; r.seed (seed);
    static const float ratios[6] = { 1.0f, 1.4471f, 1.617f, 1.9265f, 2.5028f, 2.6637f };
    double ph[6] = {};
    for (size_t i = 0; i < o.size(); ++i)
    {
        const float t = (float) i / SR; float m = 0;
        if (metal) for (int k = 0; k < 6; ++k) { ph[k] += (double) (330.0f * ratios[k]) / SR; m += ((ph[k] - std::floor (ph[k])) < 0.5 ? 1.0f : -1.0f); }
        o[i] = (m * 0.15f * (metal ? 1.0f : 0.0f) + r.bi() * 0.7f) * std::exp (-t / decay);
    }
    hp1 (o, 5000.0f * bright); hp1 (o, 3000.0f * bright); bandpass (o, 9500.0f * bright, 0.9f);
    fadeOut (o); fadeIn (o, 0.0002f); normalize (o); return o;
}
inline Buf shaker (float len, float color, uint32_t seed)
{
    Buf o ((size_t) N (len)); Rng r; r.seed (seed);
    for (size_t i = 0; i < o.size(); ++i)
    {
        const float t = (float) i / SR, a = std::min (1.0f, t / (len * 0.18f));
        o[i] = r.bi() * a * a * std::exp (-t * 3.2f / len * (1.0f + a));
    }
    bandpass (o, 6200.0f * color, 0.8f); hp1 (o, 2500.0f);
    fadeOut (o); normalize (o); return o;
}
inline Buf conga (float f, float decay, float slap, uint32_t seed)
{
    Buf o ((size_t) N (decay * 3.0f)); Rng r; r.seed (seed); double ph = 0;
    for (size_t i = 0; i < o.size(); ++i)
    {
        const float t = (float) i / SR, fr = f * (1.0f + 0.35f * std::exp (-t / 0.02f));
        ph += (double) fr / SR;
        o[i] = std::sin (kTwoPi * (float) (ph - std::floor (ph))) * std::exp (-t / decay) + slap * r.bi() * std::exp (-t / 0.006f);
    }
    bandpass (o, f * 1.2f, 0.7f); for (auto& v : o) v = fastTanh (v * 1.5f);
    fadeOut (o); fadeIn (o, 0.0003f); normalize (o); return o;
}
inline Buf rim (float f, uint32_t seed)
{
    Buf o ((size_t) N (0.2f)); Rng r; r.seed (seed); double p = 0;
    for (size_t i = 0; i < o.size(); ++i)
    {
        const float t = (float) i / SR; p += (double) f / SR;
        o[i] = (std::sin (kTwoPi * (float) (p - std::floor (p))) * 0.8f + r.bi() * 0.3f) * std::exp (-t / 0.012f);
    }
    hp1 (o, 400.0f); fadeOut (o); fadeIn (o, 0.0002f); normalize (o); return o;
}

// ---- tonais -----------------------------------------------------------------------------
inline Buf sub808 (float f, float decay, float drive, float glide, float len)
{
    Buf o ((size_t) N (len)); double ph = 0;
    for (size_t i = 0; i < o.size(); ++i)
    {
        const float t = (float) i / SR, fr = f * (1.0f + glide * std::exp (-t / 0.045f));
        ph += (double) fr / SR;
        const float env = std::exp (-t / decay) * (1.0f - std::exp (-t / 0.002f));
        float v = std::sin (kTwoPi * (float) (ph - std::floor (ph)));
        v = fastTanh (v * (1.0f + drive * 5.0f)) / (1.0f + drive * 0.7f);
        o[i] = v * env;
    }
    fadeOut (o, 0.02f); normalize (o); return o;
}
inline Buf logDrum (float f, float decay, float tok, uint32_t seed)
{
    Buf o ((size_t) N (decay * 2.6f + 0.1f)); Rng r; r.seed (seed); double p1 = 0, p2 = 0, p3 = 0; Buf nz (o.size());
    for (auto& v : nz) v = r.bi(); bandpass (nz, 1400.0f, 2.0f);
    for (size_t i = 0; i < o.size(); ++i)
    {
        const float t = (float) i / SR, g = 1.0f + 0.9f * std::exp (-t / 0.035f);
        p1 += (double) (f * g) / SR; p2 += (double) (f * 2.31f * g) / SR; p3 += (double) (f * 0.5f) / SR;
        const float s = std::sin (kTwoPi * (float) (p1 - std::floor (p1))) * std::exp (-t / decay)
                      + 0.35f * std::sin (kTwoPi * (float) (p2 - std::floor (p2))) * std::exp (-t / (decay * 0.3f))
                      + 0.25f * std::sin (kTwoPi * (float) (p3 - std::floor (p3))) * std::exp (-t / (decay * 1.2f));
        o[i] = fastTanh ((s + tok * nz[i] * std::exp (-t / 0.012f)) * 1.8f);
    }
    fadeOut (o, 0.03f); fadeIn (o, 0.0004f); normalize (o); return o;
}
inline Buf pluckKS (float f, float decay, float bright, float len, uint32_t seed)
{
    const float L = std::max (4.0f, SR / f - 0.5f); const int M = (int) L; const float d = L - (float) M, a = (1.0f - d) / (1.0f + d);
    std::vector<float> line ((size_t) M + 2, 0.0f); Rng r; r.seed (seed);
    float lpS = 0; for (int i = 0; i < M + 1; ++i) { const float w = r.bi(); lpS += (1.0f - 0.55f * (1.0f - bright)) * (w - lpS); line[(size_t) i] = lpS; }
    Buf o ((size_t) N (len)); int w = 0; float apS = 0, prev = 0;
    const float fb = std::exp (-1.0f / (f * decay));
    for (size_t i = 0; i < o.size(); ++i)
    {
        const int rIdx = w; const float y0 = line[(size_t) rIdx], y1 = line[(size_t) ((rIdx + 1) % (M + 1))];
        float avg = 0.5f * (y0 + y1); avg = (1.0f - 0.35f * (1.0f - bright)) * avg + 0.35f * (1.0f - bright) * prev; prev = avg;
        const float ap = a * avg + apS; apS = avg - a * ap;
        line[(size_t) w] = flushDenorm (ap * fb); w = (w + 1) % (M + 1);
        o[i] = y0;
    }
    fadeOut (o, 0.03f); fadeIn (o, 0.0003f); normalize (o); return o;
}
inline Buf ep (float f, float bright, float len, float tine)
{
    Buf o ((size_t) N (len)); double pc = 0, pm = 0, pt = 0;
    for (size_t i = 0; i < o.size(); ++i)
    {
        const float t = (float) i / SR;
        pc += (double) f / SR; pm += (double) f / SR; pt += (double) (f * 14.0f) / SR;
        const float idx = (0.8f + 3.2f * bright) * std::exp (-t / 0.35f) + 0.15f;
        float v = std::sin (kTwoPi * (float) (pc - std::floor (pc)) + idx * std::sin (kTwoPi * (float) (pm - std::floor (pm))));
        v += tine * std::sin (kTwoPi * (float) (pt - std::floor (pt))) * std::exp (-t / 0.018f);
        o[i] = v * std::exp (-t / (len * 0.45f)) * (1.0f - std::exp (-t / 0.002f));
    }
    fadeOut (o, 0.05f); normalize (o); return o;
}
inline Buf piano (float f, float bright, float len)
{
    Buf o ((size_t) N (len)); double ph[6] = {}; static const float amp[6] = { 1.0f, 0.55f, 0.32f, 0.2f, 0.1f, 0.06f };
    for (size_t i = 0; i < o.size(); ++i)
    {
        const float t = (float) i / SR; float v = 0;
        for (int h = 1; h <= 6; ++h) { ph[h - 1] += (double) (f * (float) h * (1.0f + 0.00035f * (float) h * (float) h)) / SR; v += amp[h - 1] * std::exp (-t * (1.2f + 1.8f * (float) h) / (len * 0.6f)) * std::sin (kTwoPi * (float) (ph[h - 1] - std::floor (ph[h - 1]))); }
        v += bright * 0.4f * std::exp (-t / 0.01f) * std::sin (kTwoPi * 7.0f * f * t);
        o[i] = v * (1.0f - std::exp (-t / 0.0015f));
    }
    fadeOut (o, 0.06f); normalize (o); return o;
}
inline Buf pad (float f, float len, float detuneCents, float bright, bool stereoR, uint32_t seed)
{
    Buf o ((size_t) N (len), 0.0f); Rng r; r.seed (seed);
    const int maxH = std::max (2, std::min (28, (int) (0.42f * SR / f)));
    const int voices = 4; const float dets[4] = { -1.0f, -0.35f, 0.4f, 1.0f };
    for (int v = 0; v < voices; ++v)
    {
        const float fv = f * std::pow (2.0f, dets[v] * detuneCents / 1200.0f) * (stereoR ? 1.0007f : 1.0f);
        for (int h = 1; h <= maxH; ++h)
        {
            const float amp = std::exp (-(float) h / (3.0f + 14.0f * bright)) / (float) h;
            const double w = (double) kTwoPi * (double) fv * (double) h / (double) SR; const double ph0 = (double) r.uni() * 6.2831853;
            double c = std::cos (ph0), s = std::sin (ph0); const double cw = std::cos (w), sw = std::sin (w);
            for (size_t i = 0; i < o.size(); ++i) { o[i] += amp * (float) s; const double nc = c * cw - s * sw; s = s * cw + c * sw; c = nc; }
        }
    }
    const float attack = std::min (0.9f, len * 0.12f);
    for (size_t i = 0; i < o.size(); ++i) { const float t = (float) i / SR; o[i] *= std::min (1.0f, t / attack); }
    normalize (o, -3.0f);
    const int ls = (int) (len * 0.35f * SR), le = (int) (len * 0.96f * SR);
    loopify (o, ls, le, (int) (0.4f * SR)); o.resize ((size_t) le); return o;
}
inline Buf vox (float f, int vowel, float len, float vib, bool sustained, uint32_t seed)
{
    static const float F[5][3] = { { 800, 1150, 2900 }, { 400, 1600, 2700 }, { 350, 1700, 2700 }, { 450, 800, 2830 }, { 325, 700, 2530 } };
    Buf o ((size_t) N (len)); Rng r; r.seed (seed); double ph = 0;
    SvfCoef c[3]; SvfState st[3]; const float q[3] = { 8.0f, 10.0f, 12.0f }; const float g[3] = { 1.0f, 0.7f, 0.4f };
    for (int k = 0; k < 3; ++k) c[k].set (F[vowel % 5][k], q[k], SR);
    for (size_t i = 0; i < o.size(); ++i)
    {
        const float t = (float) i / SR, fr = f * (1.0f + 0.006f * std::sin (kTwoPi * 5.2f * t) * vib * std::min (1.0f, t / 0.3f));
        ph += (double) fr / SR; const float x = (float) (ph - std::floor (ph));
        const float saw = (2.0f * x - 1.0f) + 0.04f * r.bi();
        float y = 0; for (int k = 0; k < 3; ++k) { float lp, bp, hp; st[k].tick (saw, c[k], lp, bp, hp); y += bp * g[k] * 0.4f; }
        const float env = std::min (1.0f, t / 0.06f) * (sustained ? 1.0f : std::exp (-t / (len * 0.45f)));
        o[i] = y * env;
    }
    normalize (o, -3.0f);
    if (sustained)
    {
        const int ls = (int) (len * 0.3f * SR), le = (int) (len * 0.95f * SR);
        loopify (o, ls, le, (int) (0.25f * SR)); o.resize ((size_t) le);
    }
    else fadeOut (o, 0.05f);
    return o;
}
inline Buf stab (float f, float len, float cutoff, float width, uint32_t seed)
{
    Buf o ((size_t) N (len)); double p1 = 0, p2 = 0, p3 = 0; Rng r; r.seed (seed); SvfState st, st2; (void) r;
    for (size_t i = 0; i < o.size(); ++i)
    {
        const float t = (float) i / SR;
        p1 += (double) f / SR; p2 += (double) (f * 1.0058f) / SR; p3 += (double) (f * 0.4971f) / SR;
        auto saw = [] (double p) { return 2.0f * (float) (p - std::floor (p)) - 1.0f; };
        const float x = saw (p1) * 0.5f + saw (p2) * 0.5f + saw (p3) * width * 0.5f;
        SvfCoef c; c.set (cutoff * (0.3f + 1.7f * std::exp (-t / 0.09f)), 1.6f, SR);
        float lp, bp, hp; st.tick (x, c, lp, bp, hp); st2.tick (lp, c, lp, bp, hp);
        o[i] = lp * (1.0f - std::exp (-t / 0.003f)) * std::exp (-t / (len * 0.55f));
    }
    fadeOut (o, 0.03f); normalize (o); return o;
}
inline Buf bell (float f, float len, float inharm)
{
    Buf o ((size_t) N (len), 0.0f); static const float pr[6] = { 1.0f, 2.0f, 2.76f, 5.4f, 8.93f, 11.34f }; static const float am[6] = { 1.0f, 0.6f, 0.45f, 0.25f, 0.12f, 0.06f };
    for (int k = 0; k < 6; ++k)
    {
        const float fr = f * (1.0f + (pr[k] - 1.0f) * inharm + (1.0f - inharm) * ((float) k > 0 ? (float) (k + 1) - pr[k] : 0.0f)), tau = len * 0.3f / (1.0f + 0.7f * (float) k);
        if (fr > SR * 0.45f) continue;
        double ph = 0;
        for (size_t i = 0; i < o.size(); ++i) { const float t = (float) i / SR; ph += (double) fr / SR; o[i] += am[k] * std::sin (kTwoPi * (float) (ph - std::floor (ph))) * std::exp (-t / tau); }
    }
    for (size_t i = 0; i < o.size(); ++i) o[i] *= 1.0f - std::exp (-((float) i / SR) / 0.001f);
    fadeOut (o, 0.05f); normalize (o); return o;
}
inline Buf kalimba (float f, float len)
{
    Buf o ((size_t) N (len)); double p1 = 0, p2 = 0;
    for (size_t i = 0; i < o.size(); ++i)
    {
        const float t = (float) i / SR; p1 += (double) f / SR; p2 += (double) (f * 5.9f) / SR;
        o[i] = (std::sin (kTwoPi * (float) (p1 - std::floor (p1))) * std::exp (-t / (len * 0.35f)) + 0.35f * std::sin (kTwoPi * (float) (p2 - std::floor (p2))) * std::exp (-t / 0.05f)) * (1.0f - std::exp (-t / 0.001f));
    }
    fadeOut (o, 0.05f); normalize (o); return o;
}
inline Buf talking (float f, float len)   // tambor falante: seno com glissando rápido
{
    Buf o ((size_t) N (len)); double ph = 0;
    for (size_t i = 0; i < o.size(); ++i)
    {
        const float t = (float) i / SR, fr = f * (0.7f + 0.5f * std::exp (-t / 0.12f) + 0.35f * std::sin (kTwoPi * 3.0f * t) * 0.2f);
        ph += (double) fr / SR; o[i] = std::sin (kTwoPi * (float) (ph - std::floor (ph))) * std::exp (-t / (len * 0.4f)) * (1.0f - std::exp (-t / 0.003f));
    }
    for (auto& v : o) v = fastTanh (v * 1.6f);
    fadeOut (o, 0.04f); normalize (o); return o;
}
inline Buf subBass (float f, float len, float harm)
{
    Buf o ((size_t) N (len)); double ph = 0;
    for (size_t i = 0; i < o.size(); ++i)
    {
        const float t = (float) i / SR; ph += (double) f / SR; const float x = kTwoPi * (float) (ph - std::floor (ph));
        float v = std::sin (x) + harm * 0.35f * std::sin (2.0f * x + 0.5f) + harm * 0.18f * std::sin (3.0f * x);
        o[i] = v * std::exp (-t / (len * 0.55f)) * (1.0f - std::exp (-t / 0.004f));
    }
    for (auto& v : o) v = fastTanh (v * 1.3f);
    fadeOut (o, 0.05f); normalize (o); return o;
}
inline Buf whistle (float f, float len)
{
    Buf o ((size_t) N (len)); double ph = 0; Rng r; r.seed (5);
    for (size_t i = 0; i < o.size(); ++i)
    {
        const float t = (float) i / SR, fr = f * (1.0f + 0.012f * std::sin (kTwoPi * 6.0f * t) + 0.06f * std::exp (-t / 0.1f));
        ph += (double) fr / SR;
        o[i] = (std::sin (kTwoPi * (float) (ph - std::floor (ph))) + 0.03f * r.bi()) * std::min (1.0f, t / 0.05f) * std::exp (-t / (len * 0.7f));
    }
    fadeOut (o, 0.05f); normalize (o, -3.0f); return o;
}
inline Buf siren (float f, float len)
{
    Buf o ((size_t) N (len)); double ph = 0;
    for (size_t i = 0; i < o.size(); ++i)
    {
        const float t = (float) i / SR, fr = f * std::pow (2.0f, 0.9f * std::sin (kTwoPi * 2.2f * t));
        ph += (double) fr / SR; const float x = (float) (ph - std::floor (ph));
        o[i] = (x < 0.5f ? 1.0f : -1.0f) * 0.7f * std::min (1.0f, t / 0.01f) * std::exp (-t / (len * 0.8f));
    }
    lp1 (o, 5500.0f); fadeOut (o, 0.05f); normalize (o, -3.0f); return o;
}
inline Buf riser (float len, uint32_t seed)
{
    Buf o ((size_t) N (len)); Rng r; r.seed (seed); SvfState st; SvfCoef c;
    for (size_t i = 0; i < o.size(); ++i)
    {
        const float t = (float) i / len / SR; c.set (200.0f * std::pow (60.0f, t), 2.5f, SR);
        float lp, bp, hp; st.tick (r.bi(), c, lp, bp, hp);
        o[i] = bp * 0.5f * (0.1f + t * t);
    }
    fadeOut (o, 0.01f); normalize (o, -2.0f); return o;
}
inline Buf impact (float len, uint32_t seed)
{
    Buf o ((size_t) N (len)); Rng r; r.seed (seed); double ph = 0; Buf nz (o.size());
    for (auto& v : nz) v = r.bi(); lp1 (nz, 2500.0f);
    for (size_t i = 0; i < o.size(); ++i)
    {
        const float t = (float) i / SR, f = 38.0f + 120.0f * std::exp (-t / 0.09f); ph += (double) f / SR;
        o[i] = std::sin (kTwoPi * (float) (ph - std::floor (ph))) * std::exp (-t / (len * 0.3f)) + nz[i] * 0.6f * std::exp (-t / (len * 0.18f));
    }
    for (auto& v : o) v = fastTanh (v * 1.5f);
    fadeOut (o, 0.1f); fadeIn (o, 0.0005f); normalize (o); return o;
}

// ---- especificação dos instrumentos e packs ---------------------------------------------------
struct InstSpec
{
    const char* id; const char* name; const char* cat;
    Gen gen; float a, b, c, d;      // parâmetros do gerador (ver makeBuf)
    int lo, hi, step;               // notas de raiz (step 0 = um só sample sem repetição de altura)
    int variants;                   // variações (round-robin)
    bool pitched;
};

inline Buf makeBuf (const InstSpec& s, int note, int variant, bool stereoR)
{
    const float f = midiToHz ((float) note); const uint32_t sd = 1000u + (uint32_t) variant * 7919u + (uint32_t) note * 31u;
    const float vv = 1.0f + 0.06f * (float) variant;
    switch (s.gen)
    {
        case G_KICK:   return kick (s.a * vv, s.b, s.c, s.d * (1.0f + 0.04f * (float) variant), 0.5f, 0.35f + 0.1f * (float) variant, s.d * 5.5f + 0.1f, sd);
        case G_SNARE:  return snare (s.a * vv, s.b, s.c, s.d, sd);
        case G_CLAP:   return clap (s.a * vv, s.b, sd);
        case G_HAT:    return hat (s.a * (1.0f + 0.1f * (float) variant), s.b, s.c > 0.5f, sd);
        case G_OHAT:   return hat (s.a * (1.0f + 0.08f * (float) variant), s.b, s.c > 0.5f, sd);
        case G_SHAKER: return shaker (s.a * (1.0f + 0.12f * (float) variant), s.b, sd);
        case G_CONGA:  return conga (s.a * vv, s.b, s.c, sd);
        case G_TOM:    return conga (s.a * vv, s.b, s.c, sd);
        case G_RIM:    return rim (s.a * vv, sd);
        case G_808:    return sub808 (f, s.a, s.b, s.c, s.d);
        case G_LOGDRUM:return logDrum (f, s.a, s.b, sd);
        case G_PLUCK:  return pluckKS (f, s.a, s.b, s.c, sd);
        case G_BASSG:  return pluckKS (f, s.a, s.b, s.c, sd);
        case G_EP:     return ep (f, s.a, s.b, s.c);
        case G_PAD:    return pad (f, s.a, s.b, s.c, stereoR, sd);
        case G_VOX:    return vox (f, (int) s.a, s.b, s.c, s.d > 0.5f, sd);
        case G_STAB:   return stab (f, s.a, s.b, s.c, sd);
        case G_BELL:   return bell (f, s.a, s.b);
        case G_RISER:  return riser (s.a, sd);
        case G_IMPACT: return impact (s.a, sd);
        case G_SIREN:  return siren (f, s.a);
        case G_WHISTLE:return whistle (s.a, s.b);
        case G_SUB:    return subBass (f, s.a, s.b);
        case G_PIANO:  return piano (f, s.a, s.b);
        case G_KALIMBA:return kalimba (f, s.a);
        case G_TALK:   return talking (f, s.a);
        default: return Buf (64, 0.0f);
    }
}

// adiciona o instrumento à biblioteca (gera todos os samples)
inline void addInstrument (Library& lib, const InstSpec& s, const std::string& pack)
{
    Instrument in; in.id = pack.empty() ? std::string (s.id) : pack + "." + s.id; in.name = s.name; in.cat = s.cat; in.pack = pack;
    const bool pitched = s.pitched && s.step > 0;
    std::vector<int> roots;
    if (pitched) for (int r = s.lo; r <= s.hi; r += s.step) roots.push_back (r); else roots.push_back (60);
    const bool looped = (s.gen == G_PAD) || (s.gen == G_VOX && s.d > 0.5f);
    const float loopFrac = s.gen == G_PAD ? 0.35f / 0.96f : 0.30f / 0.95f;
    in.fixedPitch = ! pitched;
    for (size_t ri = 0; ri < roots.size(); ++ri)
    {
        const int root = roots[ri];
        for (int v = 0; v < std::max (1, s.variants); ++v)
        {
            auto sd = std::make_unique<SampleData> (); sd->sampleRate = SR;
            sd->l = makeBuf (s, root, v, false);
            if (s.gen == G_PAD) sd->r = makeBuf (s, root, v, true);
            Zone z; z.root = root; z.rr = v;
            z.loKey = (ri == 0) ? 0 : (roots[ri - 1] + root) / 2 + 1;
            z.hiKey = (ri + 1 == roots.size()) ? 127 : (root + roots[ri + 1]) / 2;
            if (looped) { z.loopStart = (int) ((float) sd->frames() * loopFrac); z.loopEnd = sd->frames(); }
            char nm[96];
            if (pitched) std::snprintf (nm, sizeof nm, "%s_%03d_v%d.wav", s.id, root, v); else std::snprintf (nm, sizeof nm, "%s_v%d.wav", s.id, v);
            z.file = std::string ("samples/") + nm;
            z.data = lib.addData (std::move (sd));
            in.zones.push_back (z);
        }
    }
    lib.inst.push_back (std::move (in));
}
}} // namespace sk::synth
