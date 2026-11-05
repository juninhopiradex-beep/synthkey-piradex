// Testes numéricos dos 3 efeitos novos: Transientes (17), Sidechain interno (18), Convolução (19)
#include "Core/Fx.h"
#include <cstdio>
#include <cmath>
using namespace sk;
static int fails = 0, checks = 0;
#define CHECK(c, ...) do { ++checks; if (! (c)) { ++fails; std::printf ("FALHA: "); std::printf (__VA_ARGS__); std::printf ("\n"); } } while (0)
static const float SR = 48000.0f;
static float rms (const std::vector<float>& v, size_t a, size_t b) { double s = 0; for (size_t i = a; i < b && i < v.size(); ++i) s += (double) v[i] * v[i]; return (float) std::sqrt (s / std::max<size_t> (1, b - a)); }
static float peak (const std::vector<float>& v, size_t a, size_t b) { float m = 0; for (size_t i = a; i < b && i < v.size(); ++i) m = std::max (m, std::fabs (v[i])); return m; }
static void runFx (FxUnit& fx, int type, const float* p, float mix, std::vector<float>& L, std::vector<float>& R, double ppq = 0, float bpm = 120.0f, int blk = 256)
{
    const int N = (int) L.size();
    for (int i = 0; i < N; i += blk)
    {
        const int m = std::min (blk, N - i); fx.setBeat (ppq + (double) i * bpm / (60.0 * SR));
        fx.process (type, p, mix, L.data() + i, R.data() + i, m, bpm);
    }
}
// pluck sintético: seno 220 Hz com queda exponencial (tau) — ataque nítido, sustentação curta
static std::vector<float> pluck (int N, float tauMs, float f = 220.0f) { std::vector<float> v ((size_t) N); for (int i = 0; i < N; ++i) v[(size_t) i] = 0.6f * std::sin (6.2831853f * f * (float) i / SR) * std::exp (-(float) i / (0.001f * tauMs * SR)); return v; }

int main()
{
    // ---------------- Transientes ----------------
    {
        const int N = (int) (1.0f * SR);
        FxUnit fx; fx.prepare (SR, 512);
        // neutro (tudo a 0,5): sinal estacionário e pluck passam ~inalterados
        std::vector<float> L (N), R; for (int i = 0; i < N; ++i) L[(size_t) i] = 0.4f * std::sin (6.2831853f * 440.0f * (float) i / SR); R = L; auto ref = L;
        const float neutral[5] = { 0.5f, 0.5f, 0.5f, 0.5f, 0.5f };
        runFx (fx, FxUnit::Transient, neutral, 1.0f, L, R);
        double e = 0; for (int i = 0; i < N; ++i) e = std::max (e, (double) std::fabs (L[(size_t) i] - ref[(size_t) i]));
        CHECK (e < 1.0e-3, "transientes neutro altera sinal estacionário: %g", e);
        // sinal estacionário com ataque +12 / sustentação -12 também fica igual (d≈0)
        FxUnit fx2; fx2.prepare (SR, 512); const float ex[5] = { 1.0f, 0.0f, 0.5f, 0.5f, 0.5f };
        L = ref; R = ref; runFx (fx2, FxUnit::Transient, ex, 1.0f, L, R);
        const float rr = rms (L, (size_t) (0.5f * SR), (size_t) (0.9f * SR)) / rms (ref, (size_t) (0.5f * SR), (size_t) (0.9f * SR));
        CHECK (std::fabs (gainToDb (rr)) < 0.6f, "sinal estacionário deveria ficar a ±0,6 dB: %.2f dB", gainToDb (rr));
        // pluck: ataque +12 vs -12
        auto pl = pluck (N, 60.0f);
        auto go = [&] (float a, float s) { FxUnit f; f.prepare (SR, 512); std::vector<float> l = pl, r = pl; const float pp[5] = { a, s, 0.5f, 0.5f, 0.5f }; runFx (f, FxUnit::Transient, pp, 1.0f, l, r); return l; };
        auto up = go (1.0f, 0.5f), dn = go (0.0f, 0.5f);
        const float pu = peak (up, 0, 960), pd = peak (dn, 0, 960);
        CHECK (gainToDb (pu / pd) > 8.0f, "ataque +12 vs -12 deveria dar >8 dB no pico inicial: %.2f dB", gainToDb (pu / pd));
        // sustentação: -12 reduz a cauda; +12 aumenta
        auto su = go (0.5f, 1.0f), sd = go (0.5f, 0.0f);
        const float tu = rms (su, 4800, 14400), td = rms (sd, 4800, 14400);
        CHECK (gainToDb (tu / td) > 4.0f, "sustentação +12 vs -12 deveria dar >4 dB na cauda: %.2f dB", gainToDb (tu / td));
        // sem NaN/Inf e ganho de saída
        const float po[5] = { 0.5f, 0.5f, 0.5f, 1.0f, 0.5f }; FxUnit f3; f3.prepare (SR, 512); L = ref; R = ref; runFx (f3, FxUnit::Transient, po, 1.0f, L, R);
        CHECK (std::fabs (gainToDb (rms (L, 20000, 40000) / rms (ref, 20000, 40000)) - 12.0f) < 0.5f, "saída +12 dB");
        bool fin = true; for (float v : L) if (! std::isfinite (v)) fin = false; CHECK (fin, "transientes: valores não finitos");
    }
    // ---------------- Sidechain ----------------
    {
        const int N = (int) (2.0f * SR);
        const float pp[5] = { 0.5f, 1.0f, 0.5f, 0.5f, 1.0f };     // 1/4, profundidade 100%, tempo
        FxUnit fx; fx.prepare (SR, 512); std::vector<float> L (N, 0.5f), R (N, 0.5f);
        runFx (fx, FxUnit::Sidechain, pp, 1.0f, L, R, 0.0, 120.0f);      // 1/4 a 120 bpm = 0,5 s
        const size_t beat = (size_t) (0.5f * SR);
        CHECK (L[beat + 5 * 48] < 0.05f, "no tempo o nível devia estar abaixado perto do batimento: %.3f", L[beat + 5 * 48]);
        CHECK (L[beat - 100] > 0.45f, "antes do batimento devia estar recuperado: %.3f", L[beat - 100]);
        CHECK (L[(size_t) (0.25f * SR)] > 0.4f, "meio do compasso quase recuperado: %.3f", L[(size_t) (0.25f * SR)]);
        // alinhamento ao ppq: começar a meio de um tempo desloca o abaixamento
        FxUnit fx2; fx2.prepare (SR, 512); std::vector<float> L2 (N, 0.5f), R2 (N, 0.5f); runFx (fx2, FxUnit::Sidechain, pp, 1.0f, L2, R2, 0.5, 120.0f);   // 0,5 tempo = 0,25 s de avanço
        CHECK (L2[(size_t) (0.25f * SR) + 5 * 48] < 0.05f, "alinhamento ao ppq do anfitrião falhou: %.3f", L2[(size_t) (0.25f * SR) + 5 * 48]);
        // modo notas
        const float pn[5] = { 0.5f, 0.5f, 0.5f, 0.5f, 0.0f };
        FxUnit fx3; fx3.prepare (SR, 512); std::vector<float> L3 (N, 0.5f), R3 (N, 0.5f);
        runFx (fx3, FxUnit::Sidechain, pn, 1.0f, L3, R3, 0.0, 120.0f);
        CHECK (L3[1000] > 0.49f && L3[(size_t) (1.5f * SR)] > 0.49f, "modo notas sem nota não devia abaixar");
        FxUnit fx4; fx4.prepare (SR, 512); std::vector<float> L4 (N, 0.5f), R4 (N, 0.5f); fx4.noteOn();
        runFx (fx4, FxUnit::Sidechain, pn, 1.0f, L4, R4, 0.0, 120.0f);
        CHECK (L4[200] < 0.35f && L4[(size_t) (1.5f * SR)] > 0.49f, "modo notas: abaixa após nota e recupera (%.3f / %.3f)", L4[200], L4[(size_t) (1.5f * SR)]);
        // profundidade 0 = transparente
        const float p0[5] = { 0.5f, 0.0f, 0.5f, 0.5f, 1.0f }; FxUnit fx5; fx5.prepare (SR, 512); std::vector<float> L5 (N, 0.5f), R5 (N, 0.5f); runFx (fx5, FxUnit::Sidechain, p0, 1.0f, L5, R5);
        CHECK (peak (L5, 0, N) <= 0.5001f && L5[7000] > 0.499f, "profundidade 0 devia ser transparente");
    }
    // ---------------- Convolução ----------------
    for (double sr : { 44100.0, 48000.0, 96000.0 })
    {
        const auto bank = ConvBank::get (sr); const int B = bank->B;
        // IR: RT60 por integração de Schroeder, energia normalizada, determinismo
        for (int idx = 0; idx < kConvNumIr; ++idx)
        {
            auto h = convIr (idx, sr, 0), h2 = convIr (idx, sr, 0), hr = convIr (idx, sr, 1);
            double en = 0, dot = 0, er = 0; for (size_t i = 0; i < h.size(); ++i) { en += (double) h[i] * h[i]; er += (double) hr[i] * hr[i]; dot += (double) h[i] * hr[i]; }
            CHECK (h == h2, "IR %d não é determinística", idx);
            CHECK (std::fabs (en - 0.35) < 0.01 && std::fabs (er - 0.35) < 0.01, "IR %d energia %.3f", idx, en);
            CHECK (std::fabs (dot / 0.35) < 0.25, "IR %d canais pouco descorrelacionados: %.3f", idx, dot / 0.35);
            if (idx != 5) // Schroeder: tempo até cair 20 dB, extrapolado a RT60
            {
                std::vector<double> sch (h.size()); double acc = 0; for (size_t i = h.size(); i-- > 0;) { acc += (double) h[i] * h[i]; sch[i] = acc; }
                size_t t20 = 0; for (size_t i = 0; i < sch.size(); ++i) if (10.0 * std::log10 (sch[i] / sch[0]) < -20.0) { t20 = i; break; }
                const double rt = 3.0 * (double) t20 / sr; const double want = convdetail::kSpecs[idx].rt60;
                CHECK (t20 > 0 && rt > 0.55 * want && rt < 1.6 * want, "IR %d RT60 medido %.2f s esperado ~%.2f s", idx, rt, want);
            }
        }
        // convolução particionada == convolução directa (IR 2 = Hall), atrasada de B amostras
        const int idx = 2; auto h0 = convIr (idx, sr, 0), h1 = convIr (idx, sr, 1);
        const int N = 6 * B + 123; std::vector<float> xl ((size_t) N), xr ((size_t) N); Rng rg; rg.seed (7);
        for (int i = 0; i < N; ++i) { xl[(size_t) i] = 0.5f * rg.bi(); xr[(size_t) i] = 0.5f * rg.bi(); }
        Convolver cv; cv.prepare (sr); std::vector<float> yl = xl, yr = xr;
        for (int i = 0; i < N; i += 333) { const int m = std::min (333, N - i); cv.process (idx, 1.0f, yl.data() + i, yr.data() + i, m); }
        double maxErr = 0; const int chk = N - B;
        for (int i = 0; i < chk; i += 7)
        {
            double dl = 0, dr = 0; for (int k = 0; k <= i && k < (int) h0.size(); ++k) { dl += (double) h0[(size_t) k] * xl[(size_t) (i - k)]; dr += (double) h1[(size_t) k] * xr[(size_t) (i - k)]; }
            maxErr = std::max (maxErr, std::max (std::fabs (dl - yl[(size_t) (i + B)]), std::fabs (dr - yr[(size_t) (i + B)])));
        }
        CHECK (maxErr < 2.0e-3, "convolução FFT vs directa (sr %.0f): erro máx %.6f", sr, maxErr);
        // impulso: não há saída antes de B amostras e a cauda é finita
        Convolver c2; c2.prepare (sr); std::vector<float> a ((size_t) (4 * B), 0.0f), b = a; a[0] = 1.0f; c2.process (3, 1.0f, a.data(), b.data(), 4 * B);
        double pre = 0; for (int i = 0; i < B; ++i) pre += std::fabs (a[(size_t) i]); CHECK (pre < 1.0e-9, "saída antes da latência");
        // tamanho (fração) encurta a cauda sem cortar seco
        Convolver c3; c3.prepare (sr); const int Lt = (int) (3.0 * sr); std::vector<float> u ((size_t) Lt, 0.0f), w = u, u2 = u, w2 = u; u[0] = u2[0] = 1.0f;
        c3.process (4, 1.0f, u.data(), w.data(), Lt); Convolver c4; c4.prepare (sr); c4.process (4, 0.25f, u2.data(), w2.data(), Lt);
        CHECK (rms (u, (size_t) (1.5 * sr), (size_t) (3.0 * sr)) > 4.0f * rms (u2, (size_t) (1.5 * sr), (size_t) (3.0 * sr)) + 1e-9f, "fração de tamanho não encurta a cauda (sr %.0f)", sr);
        bool fin = true; for (float v : u2) if (! std::isfinite (v)) fin = false; CHECK (fin, "convolução: valores não finitos");
    }
    // FxUnit completo: mix, tom, largura e mudança de IR sem NaN
    {
        FxUnit fx; fx.prepare (SR, 512); const int N = (int) (2 * SR); std::vector<float> L (N), R; for (int i = 0; i < N; ++i) L[(size_t) i] = (i < 4800 ? 0.5f : 0.0f) * std::sin (6.2831853f * 330.0f * (float) i / SR); R = L;
        float p[5] = { 0.4f, 0.2f, 0.5f, 1.0f, 0.5f }; runFx (fx, FxUnit::Convolve, p, 0.5f, L, R);
        CHECK (rms (L, (size_t) (0.3 * SR), (size_t) (1.0 * SR)) > 1.0e-3f, "convolução sem cauda audível (%g)", rms (L, 14400, 48000));
        p[0] = 0.9f; auto L2 = L, R2 = R; runFx (fx, FxUnit::Convolve, p, 0.5f, L2, R2); bool fin = true; for (float v : L2) if (! std::isfinite (v)) fin = false; CHECK (fin, "troca de IR: não finito");
    }
    // ---------------- robustez: cantos dos parâmetros e independência do tamanho do bloco ----------------
    for (int type : { (int) FxUnit::Transient, (int) FxUnit::Sidechain, (int) FxUnit::Convolve })
    {
        const int N = 20000; Rng rg; rg.seed (11u + (uint32_t) type); std::vector<float> xl ((size_t) N), xr ((size_t) N); for (int i = 0; i < N; ++i) { xl[(size_t) i] = 0.9f * rg.bi(); xr[(size_t) i] = 0.9f * rg.bi(); }
        float worst = 0; bool fin = true;
        for (int mask = 0; mask < 32; ++mask)
        {
            float p[5]; for (int k = 0; k < 5; ++k) p[k] = (mask >> k) & 1 ? 1.0f : 0.0f;
            FxUnit fx; fx.prepare (SR, 512); auto l = xl, r = xr; runFx (fx, type, p, 1.0f, l, r, 0.0, 140.0f, 480);
            for (int i = 0; i < N; ++i) { if (! std::isfinite (l[(size_t) i]) || ! std::isfinite (r[(size_t) i])) fin = false; worst = std::max (worst, std::max (std::fabs (l[(size_t) i]), std::fabs (r[(size_t) i]))); }
        }
        CHECK (fin, "tipo %d: valores não finitos nos cantos dos parâmetros", type); CHECK (worst < (type == (int) FxUnit::Transient ? 16.0f : (type == (int) FxUnit::Convolve ? 8.0f : 1.0f)), "tipo %d: saída fora do limite teórico %.2f (transientes: +12 dB do efeito + 12 dB de saída = 14,3 com entrada 0,9)", type, worst);
        const float pm[5] = { 0.7f, 0.3f, 0.6f, 0.4f, 0.8f }; FxUnit a, b; a.prepare (SR, 512); b.prepare (SR, 512); auto l1 = xl, r1 = xr, l2 = xl, r2 = xr;
        runFx (a, type, pm, 1.0f, l1, r1, 3.25, 120.0f, 512); runFx (b, type, pm, 1.0f, l2, r2, 3.25, 120.0f, 37);
        double md = 0; for (int i = 0; i < N; ++i) md = std::max (md, (double) std::fabs (l1[(size_t) i] - l2[(size_t) i]));
        CHECK (md < 2.0e-4, "tipo %d: resultado depende do tamanho do bloco (%g)", type, md);
    }
    std::printf ("test_fx_new: %d verificações, %d falhas\n", checks, fails);
    return fails ? 1 : 0;
}
