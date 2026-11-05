// =============================================================================
//  SYNTHKEY by Piradex — Preset DNA: metadados derivados que documentam a arquitectura de um preset
//  (motores, vozes de unisson, efeitos, rotas de modulação, macros, dependências de samples/packs,
//  tempo/tonalidade e uma estimativa de custo de CPU). Calculado a partir dos parâmetros reais — nunca
//  fica desactualizado em relação ao som.
// =============================================================================
#pragma once
#include "Preset.h"
#include <set>
#include <sstream>

namespace sk { namespace dna
{
inline const char* const kFxNames[] = { "Vazio", "EQ", "Compressor", "Saturador", "Chorus", "Delay", "Reverb", "Faser", "Bitcrusher", "Largura", "Trémolo", "Delay Tempo",
                                         "Flanger", "Ensemble", "Shimmer", "Multi-Tap", "Pitch", "Transientes", "Sidechain", "Convolução" };
inline constexpr int kNumFxNames = (int) (sizeof (kFxNames) / sizeof (kFxNames[0]));
inline const char* const kEngineNames[] = { "Analógico", "Wavetable", "Sample", "Corda (físico)", "Granular", "Espectral" };

struct Dna
{
    std::vector<std::string> engines;        // motores ligados (A e/ou B)
    int oscVoices = 0;                       // osciladores por nota (unisson incluído) + sub + ruído
    std::vector<std::string> fx;             // efeitos activos por ordem de slot
    int modRoutes = 0, macroRoutes = 0;
    bool arp = false, chord = false, mono = false, layered = false, tempoSync = false;
    std::string tonality = "livre";          // "livre" | "maior" | "menor" | "diatónico"
    std::vector<std::string> instruments;    // ids de samples usados
    std::string packRequired;                // pack (≠ núcleo) de que o preset depende; "" = só núcleo
    int cpuCost = 0, cpuClass = 1;           // 1 leve · 2 médio · 3 pesado
};

inline Dna of (const Preset& p)
{
    Dna d;
    for (int E = 0; E < 2; ++E)
    {
        const int o = E ? (P_B_ON - P_A_ON) : 0;
        if (p.get (P_A_ON + o) < 0.5f) continue;
        const int t = std::max (0, std::min (5, (int) p.get (P_A_TYPE + o))); const int uni = std::max (1, (int) p.get (P_A_UNI + o));
        d.engines.push_back (kEngineNames[t]);
        const int voices = (t == 3 || t == 4) ? 1 : uni; d.oscVoices += voices;
        d.cpuCost += (int) std::ceil ((float) voices * (t == 4 ? 3.0f : (t == 3 ? 2.0f : (t == 5 ? 1.2f : 1.0f))));
    }
    d.layered = d.engines.size() > 1;
    if (p.get (P_SUB_LEVEL) > -59.0f) { ++d.oscVoices; d.cpuCost += 1; }
    if (p.get (P_NOISE_LEVEL) > -59.0f) { ++d.oscVoices; }
    for (int s = 0; s < 9; ++s)
    {
        const int t = (int) p.get (fxSlotBase (s)); if (t <= 0 || t >= kNumFxNames) continue;
        d.fx.push_back (kFxNames[t]);
        static const int cost[kNumFxNames] = { 0, 1, 1, 1, 2, 1, 3, 2, 1, 1, 1, 1, 2, 3, 6, 2, 6, 1, 1, 6 };
        d.cpuCost += cost[t]; if (t == 11) d.tempoSync = true;
    }
    for (int s = 0; s < kNumModSlots; ++s)
    {
        const int b = modSlotBase (s); const int src = (int) p.get (b), dst = (int) p.get (b + 1);
        if (src == 0 || dst == 0 || std::fabs (p.get (b + 2)) < 1.0e-4f) continue;
        ++d.modRoutes; if (src >= MS_MACRO1 && src <= MS_MACRO8) ++d.macroRoutes;
    }
    d.arp = p.get (P_ARP_ON) > 0.5f; d.chord = (int) p.get (P_CHORD_TYPE) != 0; d.mono = (int) p.get (P_VOICE_MODE) != 0;
    if (d.arp) d.tempoSync = true;
    switch ((int) p.get (P_CHORD_TYPE))
    {
        case 3: case 5: case 7: case 8: d.tonality = "maior"; break;
        case 4: case 6: case 9: d.tonality = "menor"; break;
        case 12: case 13: case 14: d.tonality = "diatónico"; break;
        default: break;
    }
    std::set<std::string> inst; for (const auto* s : { &p.sampleA, &p.sampleB }) if (! s->empty()) inst.insert (*s);
    for (auto& s : inst)
    {
        d.instruments.push_back (s); const auto dot = s.find ('.'); const std::string pk = dot == std::string::npos ? "" : s.substr (0, dot);
        if (! pk.empty() && pk != "nucleo" && d.packRequired.empty()) d.packRequired = pk;
    }
    d.cpuClass = d.cpuCost <= 6 ? 1 : (d.cpuCost <= 14 ? 2 : 3);
    return d;
}

inline std::string text (const Dna& d)
{
    auto join = [] (const std::vector<std::string>& v, const char* sep) { std::string o; for (size_t i = 0; i < v.size(); ++i) { if (i) o += sep; o += v[i]; } return o; };
    std::ostringstream o;
    o << "Motores: " << (d.engines.empty() ? "nenhum" : join (d.engines, " + ")) << (d.layered ? " (camada)" : "") << " · " << d.oscVoices << " osc/nota\n";
    o << "Efeitos: " << (d.fx.empty() ? "nenhum" : join (d.fx, " → ")) << "\n";
    o << "Modulação: " << d.modRoutes << " rotas" << (d.macroRoutes ? ", " + std::to_string (d.macroRoutes) + " em macros" : std::string()) << "\n";
    o << "Tocar: " << (d.arp ? "arpejador · " : "") << (d.chord ? "acordes · " : "") << (d.mono ? "mono · " : "poli · ") << "tom " << d.tonality << (d.tempoSync ? " · sincronizado ao tempo" : "") << "\n";
    o << "Dependências: " << (d.instruments.empty() ? "nenhum sample" : join (d.instruments, ", ")) << (d.packRequired.empty() ? " (só Núcleo)" : " · pack «" + d.packRequired + "»") << "\n";
    o << "CPU: " << (d.cpuClass == 1 ? "leve" : (d.cpuClass == 2 ? "médio" : "pesado")) << " (índice " << d.cpuCost << ")";
    return o.str();
}
}} // namespace sk::dna
