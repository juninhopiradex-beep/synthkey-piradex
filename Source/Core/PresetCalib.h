// =============================================================================
//  SYNTHKEY by Piradex — calibração de nível dos presets (render offline)
//  Toca notas de teste com o motor real e ajusta o Volume Master para que o pico
//  fique em ≈ −8 dBFS, de modo a que todos os presets soem com volume parecido.
// =============================================================================
#pragma once
#include <memory>
#include "Engine.h"
#include "Preset.h"
#include <vector>

namespace sk { namespace presets
{
// preenche o vetor de parâmetros reais a partir de um preset (defeitos + valores) e resolve os samples
inline void toParams (const Preset& p, const Library* lib, float* prm)
{
    for (int i = 0; i < P_COUNT; ++i) prm[i] = kParams[i].def;
    for (auto& kv : p.values) prm[kv.first] = kv.second;
    if (lib)
    {
        const int a = p.sampleA.empty() ? -1 : lib->indexOf (p.sampleA), b = p.sampleB.empty() ? -1 : lib->indexOf (p.sampleB);
        if (a >= 0) prm[P_A_SMP] = (float) a;
        if (b >= 0) prm[P_B_SMP] = (float) b;
    }
}

// impressão digital sonora de um preset (para a curadoria: rejeitar variantes que soam iguais)
struct Fingerprint { float peak = 0, v[8] = {}; bool ok = false; };

inline float fpDistance (const Fingerprint& a, const Fingerprint& b)
{ float d = 0; for (int i = 0; i < 8; ++i) { const float x = a.v[i] - b.v[i]; d += x * x; } return std::sqrt (d); }

// renderiza como a calibração e mede: pico (com master a 0 dB), centroides cedo/tarde, ataque, queda, cauda, largura, graves, ruído
inline float measure (Preset& p, const Library& lib, Fingerprint* fp = nullptr, float targetDb = -8.0f, double sr = 44100.0)
{
    auto engPtr = std::make_unique<Engine>(); Engine& eng = *engPtr;   // motor novo por medição (grande: heap): o estado residual de um preset anterior alterava o ganho em ±2 dB
    std::vector<float> prm ((size_t) P_COUNT), L (512), R (512);
    p.set (P_MASTER_VOL, 0.0f);
    toParams (p, &lib, prm.data());
    prm[P_LIM_ON] = 0.0f; prm[P_ARP_ON] = 0.0f; prm[P_CHORD_TYPE] = 0.0f; prm[P_MASTER_VOL] = 0.0f;
    eng.prepare (sr, 512); eng.setParams (prm.data()); eng.setLibrary (&lib); eng.setBpm (120);
    const bool bass = p.category == "Baixo";
    const bool single = bass || p.category == "Bateria" || p.category == "Percussão" || p.category == "FX" || p.category == "Arp" || p.values.size() == 0 || p.get (P_CHORD_TYPE) != 0.0f
                        || p.get (P_VOICE_MODE) != 0.0f;
    std::vector<int> notes = single ? std::vector<int> { bass ? 36 : (p.category == "Bateria" ? 38 : 60) } : std::vector<int> { 60, 64, 67 };
    for (int b = 0; b < 8; ++b) eng.render (L.data(), R.data(), 512);   // deixa assentar o suavizador do master
    for (int n : notes) eng.noteOn (n, 100.0f / 127.0f);
    float peak = 0; const int blocks = (int) (sr * 1.8 / 512);
    std::vector<float> ml, sd; if (fp) { ml.reserve ((size_t) blocks * 512); sd.reserve ((size_t) blocks * 512); }
    for (int b = 0; b < blocks; ++b)
    {
        eng.render (L.data(), R.data(), 512);
        for (int i = 0; i < 512; ++i) { peak = std::max (peak, std::max (std::fabs (L[(size_t) i]), std::fabs (R[(size_t) i]))); if (fp) { ml.push_back (0.5f * (L[(size_t) i] + R[(size_t) i])); sd.push_back (0.5f * (L[(size_t) i] - R[(size_t) i])); } }
    }
    for (int n : notes) eng.noteOff (n);
    float db = targetDb - 20.0f * std::log10 (std::max (peak, 1.0e-4f));
    db = std::max (-24.0f, std::min (6.0f, db));
    p.set (P_MASTER_VOL, db);
    if (fp)
    {
        fp->peak = peak; fp->ok = std::isfinite (peak) && peak > 2.0e-3f && peak < 1000.0f;
        const size_t N = ml.size(); const size_t blk = 512, nb = N / blk;
        std::vector<float> env (nb, 0.0f); float mx = 1.0e-9f;
        for (size_t b = 0; b < nb; ++b) { double e = 0; for (size_t i = 0; i < blk; ++i) e += (double) ml[b * blk + i] * ml[b * blk + i]; env[b] = (float) std::sqrt (e / (double) blk); mx = std::max (mx, env[b]); }
        size_t ia = 0; while (ia < nb && env[ia] < 0.7f * mx) ++ia;
        auto at = [&] (double sec) { return std::min (nb - 1, (size_t) (sec * sr / (double) blk)); };
        const float att = (float) ia * (float) blk / (float) sr * 1000.0f;
        const float dec = 20.0f * std::log10 ((env[at (1.1)] + 1.0e-6f) / mx), tail = 20.0f * std::log10 ((env[at (1.7)] + 1.0e-6f) / mx);
        auto spec = [&] (double t0, float& cen, float& low, float& flat)
        {
            const size_t S = 4096, a0 = std::min (N - S, (size_t) (t0 * sr)); std::vector<Cpx> c (S);
            for (size_t i = 0; i < S; ++i) { const float w = 0.5f - 0.5f * std::cos (2.0f * kPi * (float) i / (float) S); c[i] = { ml[a0 + i] * w, 0.0f }; }
            fft (c, false);
            double tot = 0, wsum = 0, lo = 0, lg = 0, am = 0; int nf = 0;
            for (size_t k = 2; k < S / 2; ++k)
            {
                const double f = (double) k * sr / (double) S, e = (double) c[k].re * c[k].re + (double) c[k].im * c[k].im; tot += e; wsum += e * std::log2 (f); if (f < 250.0) lo += e;
                if (f >= 100.0 && f <= 8000.0) { lg += std::log (e + 1.0e-12); am += e; ++nf; }
            }
            cen = tot > 1.0e-12 ? (float) (wsum / tot) : 6.0f; low = tot > 1.0e-12 ? (float) (lo / tot) : 0.0f;
            flat = nf > 0 && am > 1.0e-12 ? (float) std::exp (lg / nf) / (float) (am / nf) : 0.0f;
        };
        float cE, cL, lowE, flE, d1, d2; spec (0.12, cE, lowE, flE); spec (1.0, cL, d1, d2);
        double es = 0, em = 0; for (size_t i = (size_t) (0.1 * sr); i < std::min (N, (size_t) (1.0 * sr)); ++i) { es += (double) sd[i] * sd[i]; em += (double) ml[i] * ml[i]; }
        const float wid = (float) std::sqrt (es / std::max (em, 1.0e-12));
        fp->v[0] = cE; fp->v[1] = 0.7f * cL; fp->v[2] = 1.2f * std::log10 (att + 1.0f); fp->v[3] = std::max (-60.0f, dec) / 10.0f; fp->v[4] = std::max (-60.0f, tail) / 12.0f;
        fp->v[5] = 3.0f * std::min (1.0f, wid); fp->v[6] = 2.0f * lowE; fp->v[7] = 2.0f * std::sqrt (std::max (0.0f, flE));
    }
    return peak;
}

// devolve o pico medido (linear) com o master a 0 dB; aplica o ganho ao preset
inline float calibrate (Preset& p, const Library& lib, float targetDb = -8.0f, double sr = 44100.0) { return measure (p, lib, nullptr, targetDb, sr); }
}} // namespace sk::presets
