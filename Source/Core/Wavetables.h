// =============================================================================
//  SYNTHKEY by Piradex — wavetables (16 tabelas × 32 frames, com mips limitados em banda)
//
//  Cada frame é descrito pelas suas harmónicas (ou por uma forma de onda no
//  tempo que passa por FFT). Para cada frame constroem-se 8 versões ("mips") com
//  menos harmónicas, para o oscilador escolher a que não faz aliasing para a
//  nota tocada. Tudo é gerado uma única vez, fora da thread de áudio.
// =============================================================================
#pragma once
#include "Dsp.h"
#include <memory>
#include <mutex>

namespace sk
{
class Wavetables
{
public:
    static constexpr int kTables = 16;
    static constexpr int kFrames = 32;
    static constexpr int kSize   = 1024;           // amostras por ciclo
    static constexpr int kMips   = 8;
    static constexpr int kMaxHarm = kSize / 2;     // 512 harmónicas no mip 0

    static const Wavetables& get()
    {
        static std::once_flag flag;
        static std::unique_ptr<Wavetables> inst;
        std::call_once (flag, [] { inst.reset (new Wavetables()); });
        return *inst;
    }

    // formas básicas band-limited para o oscilador analógico: 0 seno, 1 triângulo, 2 serra (a serra é ascendente)
    const float* basic (int shape, int mip) const { return extra.data() + ((size_t) shape * kMips + (size_t) mip) * kStride; }

    // ciclo (kSize + 3 amostras: guarda para interpolação) de uma tabela/frame/mip
    const float* cycle (int table, int frame, int mip) const
    {
        return data.data() + (((size_t) table * kFrames + (size_t) frame) * kMips + (size_t) mip) * kStride;
    }
    static int mipForFrequency (float f0, float sampleRate)
    {
        const float needed = std::max (1.0f, 0.5f * sampleRate / std::max (f0, 1.0f));
        const int m = (int) std::ceil (std::log2 (std::max (1.0f, (float) kMaxHarm / needed)));
        return std::min (std::max (m, 0), kMips - 1);
    }
    // ler com interpolação linear (fase em [0,1))
    static inline float read (const float* cyc, float phase)
    {
        const float x = phase * (float) kSize;
        const int i = (int) x;
        const float f = x - (float) i;
        return cyc[i] + (cyc[i + 1] - cyc[i]) * f;
    }
    // posição 0..1 → frames adjacentes
    static inline void framePos (float pos, int& f0, int& f1, float& fr)
    {
        const float x = clampf (pos, 0.0f, 1.0f) * (float) (kFrames - 1);
        f0 = (int) x; if (f0 >= kFrames - 1) f0 = kFrames - 2;
        f1 = f0 + 1; fr = x - (float) f0;
    }

private:
    static constexpr int kStride = kSize + 4;
    using Spec = std::array<Cpx, kMaxHarm + 1>;   // H[h]: x(φ) = Σ Re(H[h]·e^{j2πhφ})
    std::vector<float> data, extra;

    // ---- utilitários de espectro --------------------------------------------------
    static void setHarm (Spec& s, int h, float amp, float ph = 0.0f)   // seno com fase ph
    {
        if (h < 1 || h > kMaxHarm) return;
        const float th = ph - 0.5f * kPi;      // sin(x+ph) = cos(x+ph-π/2)
        s[(size_t) h] = { amp * std::cos (th), amp * std::sin (th) };
    }
    static void clear (Spec& s) { for (auto& c : s) c = { 0, 0 }; }
    static float hashPhase (int a, int b)
    {
        uint32_t x = ((uint32_t) a * 73856093u) ^ ((uint32_t) b * 19349663u) ^ 0x9E3779B9u;
        x ^= x >> 15; x *= 0x2c1b3c6dU; x ^= x >> 12; x *= 0x297a2d39U; x ^= x >> 15;
        return (float) (x & 0xFFFFFF) / 16777216.0f * kTwoPi;
    }
    static void fromTime (Spec& s, float (*fn) (float, float), float t)
    {
        constexpr int N = 4096;
        std::vector<Cpx> a ((size_t) N);
        for (int i = 0; i < N; ++i) a[(size_t) i] = { fn ((float) i / (float) N, t), 0.0f };
        fft (a, false);
        clear (s);
        for (int h = 1; h <= kMaxHarm; ++h)
        {
            s[(size_t) h].re =  2.0f * a[(size_t) h].re / (float) N;
            s[(size_t) h].im =  2.0f * a[(size_t) h].im / (float) N;
        }
    }

    // ---- formas de onda no tempo -------------------------------------------------
    static float fmShape (float ph, float t)
    {
        return std::sin (kTwoPi * ph + (t * 8.0f) * std::sin (kTwoPi * 2.0f * ph));
    }
    static float foldShape (float ph, float t)
    {
        const float g = 1.0f + t * 7.0f;
        return std::sin (g * std::sin (kTwoPi * ph));
    }
    static float digitalShape (float ph, float t)
    {
        const float steps = std::pow (2.0f, 1.0f + (1.0f - t) * 5.0f);
        const float v = std::sin (kTwoPi * ph) * 0.8f + 0.2f * std::sin (kTwoPi * 3.0f * ph);
        return std::round (v * steps) / steps;
    }

    // ---- espectros por tabela -----------------------------------------------------
    // formas básicas: 0 seno, 1 triângulo, 2 serra, 3 quadrada, 4 pulso 25%, 5 pulso 10%
    static void basicHarm (Spec& s, int shape)
    {
        clear (s);
        for (int h = 1; h <= kMaxHarm; ++h)
        {
            const float fh = (float) h;
            switch (shape)
            {
                case 0: if (h == 1) setHarm (s, h, 1.0f); break;
                case 1: if (h & 1) setHarm (s, h, 8.0f / (kPi * kPi * fh * fh), (((h - 1) / 2) & 1) ? kPi : 0.0f); break;
                case 2: setHarm (s, h, 2.0f / (kPi * fh), (h & 1) ? 0.0f : kPi); break;
                case 3: if (h & 1) setHarm (s, h, 4.0f / (kPi * fh)); break;
                default:
                {
                    const float w = shape == 4 ? 0.25f : 0.10f;
                    const float a = 2.0f * std::sin (kPi * fh * w) / (kPi * fh);
                    setHarm (s, h, std::fabs (a), a < 0 ? kPi : 0.0f);
                }
            }
        }
    }
    static void pulseHarm (Spec& s, float w)
    {
        clear (s);
        for (int h = 1; h <= kMaxHarm; ++h)
        {
            const float a = 2.0f * std::sin (kPi * (float) h * w) / (kPi * (float) h);
            setHarm (s, h, std::fabs (a), a < 0 ? kPi : 0.0f);
        }
    }

    static void frameSpec (int table, int frame, Spec& s)
    {
        const float t = (float) frame / (float) (kFrames - 1);
        clear (s);
        switch (table)
        {
            case 0:   // Básica: seno → triângulo → serra → quadrada → pulso 25% → pulso 10%
            {
                const float pos = t * 5.0f; const int a = std::min ((int) pos, 4); const float fr = pos - (float) a;
                Spec A, B;
                basicHarm (A, a); basicHarm (B, a + 1);
                for (int h = 1; h <= kMaxHarm; ++h)
                    s[(size_t) h] = { lerpf (A[(size_t) h].re, B[(size_t) h].re, fr), lerpf (A[(size_t) h].im, B[(size_t) h].im, fr) };
                break;
            }
            case 1:   // Harmónicos: de 1 a 31 harmónicas com ordem 1/h
            {
                const int n = 1 + (int) std::round (t * 30.0f);
                for (int h = 1; h <= n; ++h) setHarm (s, h, 1.0f / std::pow ((float) h, 0.75f));
                break;
            }
            case 2:   // Órgão (barras de harmónicos que passam de jazz a rock a gospel a flauta)
            {
                static const float bars[4][7] = {   // 1, 2, 3, 4, 5, 6, 8
                    { 1.0f, 0.8f, 0.5f, 0.2f, 0.0f, 0.1f, 0.0f }, { 1.0f, 0.2f, 0.9f, 0.6f, 0.3f, 0.2f, 0.4f },
                    { 1.0f, 0.9f, 0.9f, 0.8f, 0.5f, 0.4f, 0.4f }, { 1.0f, 0.0f, 0.15f, 0.0f, 0.0f, 0.0f, 0.0f } };
                static const int hs[7] = { 1, 2, 3, 4, 5, 6, 8 };
                const float pos = t * 3.0f; const int a = std::min ((int) pos, 2); const float fr = pos - (float) a;
                for (int i = 0; i < 7; ++i) setHarm (s, hs[i], lerpf (bars[a][i], bars[a + 1][i], fr) * (i ? 0.7f : 1.0f));
                break;
            }
            case 3:   // Vogais (A E I O U) — formantes sobre uma base de 110 Hz
            {
                static const float F[5][3] = { { 800, 1150, 2900 }, { 400, 1600, 2700 }, { 350, 1700, 2700 }, { 450, 800, 2830 }, { 325, 700, 2530 } };
                const float pos = t * 4.0f; const int a = std::min ((int) pos, 3); const float fr = pos - (float) a;
                for (int h = 1; h <= 120; ++h)
                {
                    const float f = 110.0f * (float) h; float g = 0;
                    for (int k = 0; k < 3; ++k)
                    {
                        const float c = lerpf (F[a][k], F[a + 1][k], fr), bw = 70.0f + 40.0f * (float) k;
                        const float d = (f - c) / bw;
                        g += std::exp (-d * d) * (k == 0 ? 1.0f : (k == 1 ? 0.6f : 0.35f));
                    }
                    setHarm (s, h, g / std::pow ((float) h, 0.35f));
                }
                break;
            }
            case 4:   // Coro de Vidro: pico de harmónicas que se desloca, fases espalhadas
            {
                const float c = 2.0f + t * 18.0f, w = 1.5f + t * 3.0f;
                for (int h = 1; h <= 48; ++h)
                {
                    const float d = ((float) h - c) / w;
                    setHarm (s, h, (0.15f + std::exp (-d * d)) / std::pow ((float) h, 0.6f), hashPhase (h, 4));
                }
                break;
            }
            case 5:   // Sinos Digitais: harmónicas "de sino" com brilho crescente
            {
                static const int part[10] = { 1, 2, 3, 4, 5, 6, 8, 9, 11, 13 };
                for (int i = 0; i < 10; ++i)
                {
                    const float br = std::exp (-(float) i * (1.1f - t));
                    setHarm (s, part[i], br * ((i % 3 == 1) ? 0.6f : 1.0f), hashPhase (part[i], 5) * 0.2f);
                }
                break;
            }
            case 6: fromTime (s, fmShape, t); break;
            case 7:   // PWM
            {
                const float w = 0.5f - t * 0.46f;
                pulseHarm (s, w);
                break;
            }
            case 8: fromTime (s, foldShape, t); break;
            case 9:   // Pente
            {
                const float p = 2.0f + t * 14.0f;
                for (int h = 1; h <= 96; ++h)
                    setHarm (s, h, (0.5f + 0.5f * std::cos (kTwoPi * (float) h / p)) / (float) h * 1.5f);
                break;
            }
            case 10:  // Pads Etéreos
            {
                const float c = 4.0f + t * 14.0f;
                for (int h = 1; h <= 64; ++h)
                    setHarm (s, h, std::exp (-(float) h / c) * (0.6f + 0.4f * std::sin ((float) h * 0.7f + t * 6.0f)), hashPhase (h, 10));
                break;
            }
            case 11: fromTime (s, digitalShape, t); break;
            case 12:  // Sub Baixo
            {
                setHarm (s, 1, 1.0f);
                setHarm (s, 2, 0.05f + 0.55f * t);
                setHarm (s, 3, 0.35f * t * t);
                setHarm (s, 4, 0.2f * t * t * t);
                break;
            }
            case 13:  // Piano Elétrico (corpo + "tine")
            {
                setHarm (s, 1, 1.0f); setHarm (s, 2, 0.35f - 0.15f * t); setHarm (s, 3, 0.12f + 0.1f * t);
                setHarm (s, 4, 0.05f + 0.45f * t); setHarm (s, 5, 0.04f); setHarm (s, 6, 0.1f * t);
                setHarm (s, 7, 0.2f * t * t); setHarm (s, 9, 0.12f * t);
                break;
            }
            case 14:  // Kalimba
            {
                setHarm (s, 1, 1.0f); setHarm (s, 4, 0.2f + 0.8f * t); setHarm (s, 6, 0.1f + 0.5f * t * t);
                setHarm (s, 9, 0.3f * t); setHarm (s, 2, 0.15f * (1.0f - t));
                break;
            }
            default:  // Textura (amplitudes pseudo-aleatórias suaves, evoluem com o frame)
            {
                for (int h = 1; h <= 40; ++h)
                {
                    const float a = std::sin (hashPhase (h, 15) + t * (1.0f + (float) (h % 5)));
                    setHarm (s, h, std::max (0.0f, a) / std::pow ((float) h, 0.7f) * 1.4f, hashPhase (h, 16));
                }
                break;
            }
        }
        s[0] = { 0, 0 };
    }

    void buildMips (const Spec& spec, float* dstBase, bool normalizeTo09, std::vector<Cpx>& buf)
    {
        float scale = 1.0f;
        for (int mip = 0; mip < kMips; ++mip)
        {
            const int limit = kMaxHarm >> mip;
            std::fill (buf.begin(), buf.end(), Cpx { 0, 0 });
            for (int h = 1; h <= limit; ++h)
            {
                const Cpx c = spec[(size_t) h];
                const float re = 0.5f * (float) kSize * c.re, im = 0.5f * (float) kSize * c.im;
                buf[(size_t) h] = { re, im };
                buf[(size_t) (kSize - h)] = { re, -im };
            }
            fft (buf, true);
            float* out = dstBase + (size_t) mip * kStride;
            if (mip == 0 && normalizeTo09)
            {
                float pk = 1.0e-6f;
                for (int i = 0; i < kSize; ++i) pk = std::max (pk, std::fabs (buf[(size_t) i].re));
                scale = 0.9f / pk;
            }
            for (int i = 0; i < kSize; ++i) out[i] = buf[(size_t) i].re * scale;
            out[kSize] = out[0]; out[kSize + 1] = out[1]; out[kSize + 2] = out[2]; out[kSize + 3] = out[3];
        }
    }

    Wavetables()
    {
        data.assign ((size_t) kTables * kFrames * kMips * kStride, 0.0f);
        extra.assign ((size_t) 3 * kMips * kStride, 0.0f);
        std::vector<Cpx> buf ((size_t) kSize);
        Spec spec;
        for (int sh = 0; sh < 3; ++sh)
        {
            basicHarm (spec, sh);   // 0 seno, 1 triângulo, 2 serra
            buildMips (spec, extra.data() + (size_t) sh * kMips * kStride, true, buf);
        }
        for (int tb = 0; tb < kTables; ++tb)
            for (int fr = 0; fr < kFrames; ++fr)
            {
                frameSpec (tb, fr, spec);
                buildMips (spec, data.data() + (((size_t) tb * kFrames + (size_t) fr) * kMips) * kStride, true, buf);
            }
    }
};
} // namespace sk
