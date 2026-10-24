// Testes dos presets de fábrica: validade, samples existentes, calibração e estabilidade
#include "Core/PresetBuilder.h"
#include "Core/PresetCalib.h"
#include "Core/PackSpecs.h"
#include <chrono>
#include <cstdio>
#include <set>
using namespace sk;
static int fails = 0, checks = 0;
#define CHECK(c, msg) do { ++checks; if (! (c)) { ++fails; std::printf ("FALHOU: %s (linha %d)\n", msg, __LINE__); } } while (0)

int main()
{
    const auto defs = buildPackDefs();
    Library nuc; for (auto& d : defs) if (d.core) buildPackLibrary (d, nuc);
    size_t total = 0; std::set<std::string> names; int worst = 0; double sumDb = 0; int nCal = 0;
    for (auto& d : defs)
    {
        Library lib; lib.appendShared (nuc); if (! d.core) buildPackLibrary (d, lib);
        auto list = presets::forPack (d.id);
        std::printf ("%-9s %2zu presets\n", d.id.c_str(), list.size());
        CHECK (! list.empty(), "tem presets");
        for (auto& p : list)
        {
            ++total; CHECK (names.insert (p.pack + "/" + p.name).second, ("nome repetido: " + p.name).c_str());
            if (! p.sampleA.empty()) CHECK (lib.indexOf (p.sampleA) >= 0, ("sample A inexistente: " + p.sampleA + " em " + p.name).c_str());
            if (! p.sampleB.empty()) CHECK (lib.indexOf (p.sampleB) >= 0, ("sample B inexistente: " + p.sampleB + " em " + p.name).c_str());
            if (p.get (P_A_ON) > 0.5f && p.get (P_A_TYPE) == 2.0f) CHECK (! p.sampleA.empty(), ("motor A sample sem instrumento: " + p.name).c_str());
            if (p.get (P_B_ON) > 0.5f && p.get (P_B_TYPE) == 2.0f) CHECK (! p.sampleB.empty(), ("motor B sample sem instrumento: " + p.name).c_str());
            const float pk = presets::calibrate (p, lib);
            const float mv = p.get (P_MASTER_VOL);
            CHECK (pk > 0.002f, ("preset mudo: " + p.pack + "/" + p.name).c_str());
            if (pk <= 0.002f) std::printf ("   mudo: %s (pico %.5f)\n", p.name.c_str(), pk);
            sumDb += mv; ++nCal;
            // roundtrip JSON
            Preset q; CHECK (presetFromJson (presetToJson (p), q) && q.name == p.name && q.values.size() == p.values.size() && q.sampleA == p.sampleA, "json ida e volta");
            // verificação final: com o ganho aplicado, o pico fica perto do alvo
            std::vector<float> prm ((size_t) P_COUNT); presets::toParams (p, &lib, prm.data());
            if (mv > -23.5f && mv < 5.5f && pk > 0.002f) { static Engine e; std::vector<float> L (512), R (512); prm[P_LIM_ON] = 0; prm[P_ARP_ON] = 0; prm[P_CHORD_TYPE] = 0; e.prepare (44100, 512); e.setParams (prm.data()); e.setLibrary (&lib); for (int b = 0; b < 8; ++b) e.render (L.data(), R.data(), 512);
                const bool bass = p.category == "Baixo"; const bool single = bass || p.category == "Bateria" || p.category == "Percussão" || p.category == "FX" || p.category == "Arp" || p.get (P_CHORD_TYPE) != 0 || p.get (P_VOICE_MODE) != 0;
                if (single) e.noteOn (bass ? 36 : (p.category == "Bateria" ? 38 : 60), 100.f / 127.f); else { e.noteOn (60, 100.f / 127.f); e.noteOn (64, 100.f / 127.f); e.noteOn (67, 100.f / 127.f); }
                float peak = 0; for (int b = 0; b < 150; ++b) { e.render (L.data(), R.data(), 512); for (int i = 0; i < 512; ++i) { peak = std::max (peak, std::fabs (L[(size_t) i])); CHECK (std::isfinite (L[(size_t) i]) && std::isfinite (R[(size_t) i]), "finito"); } }
                const double pdb = 20.0 * std::log10 (std::max (peak, 1e-6f)); if (std::fabs (pdb + 8.0) > 1.5) std::printf ("   fora do alvo: %s %.1f dB (master %.1f)\n", p.name.c_str(), pdb, mv); }
        }
    }
    CHECK (total >= 230, "pelo menos 230 presets");
    for (auto& e : presets::lintErrors()) { ++fails; std::printf ("LINT: %s\n", e.c_str()); }
    std::printf ("%zu presets, master médio %.1f dB\n", total, sumDb / std::max (1, nCal));
    std::printf ("%d verificações, %d falhas\n", checks, fails);
    return fails ? 1 : 0;
}
