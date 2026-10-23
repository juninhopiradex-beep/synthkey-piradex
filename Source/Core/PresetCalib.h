// =============================================================================
//  SYNTHKEY by Piradex — calibração de nível dos presets (render offline)
//  Toca notas de teste com o motor real e ajusta o Volume Master para que o pico
//  fique em ≈ −8 dBFS, de modo a que todos os presets soem com volume parecido.
// =============================================================================
#pragma once
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

// devolve o pico medido (linear) com o master a 0 dB; aplica o ganho ao preset
inline float calibrate (Preset& p, const Library& lib, float targetDb = -8.0f, double sr = 44100.0)
{
    static thread_local Engine eng; // grande: não ir para a pilha
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
    for (int b = 0; b < blocks; ++b) { eng.render (L.data(), R.data(), 512); for (int i = 0; i < 512; ++i) peak = std::max (peak, std::max (std::fabs (L[(size_t) i]), std::fabs (R[(size_t) i]))); }
    for (int n : notes) eng.noteOff (n);
    float db = targetDb - 20.0f * std::log10 (std::max (peak, 1.0e-4f));
    db = std::max (-24.0f, std::min (6.0f, db));
    p.set (P_MASTER_VOL, db);
    return peak;
}
}} // namespace sk::presets
