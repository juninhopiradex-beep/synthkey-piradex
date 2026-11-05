// Testes dos presets de fábrica: validade, samples existentes, calibração e estabilidade
#include "Core/PresetBuilder.h"
#include "Core/PresetCalib.h"
#include "Core/PackSpecs.h"
#include "Core/NucleoGains.h"
#include <chrono>
#include <cstdio>
#include <map>
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
                float peak = 0; for (int b = 0; b < 150; ++b) { e.render (L.data(), R.data(), 512); for (int i = 0; i < 512; ++i) { peak = std::max (peak, std::max (std::fabs (L[(size_t) i]), std::fabs (R[(size_t) i]))); CHECK (std::isfinite (L[(size_t) i]) && std::isfinite (R[(size_t) i]), "finito"); } }
                const double pdb = 20.0 * std::log10 (std::max (peak, 1e-6f)); if (std::fabs (pdb + 8.0) > 1.5) std::printf ("   fora do alvo: %s %.1f dB (master %.1f)\n", p.name.c_str(), pdb, mv); }
        }
    }
    CHECK (total >= 500, "pelo menos 500 presets");
    // presets de módulo + calibração: todos os presets do Núcleo têm ganho em NucleoGains.h
    {
        auto nuc = presets::forPack ("nucleo"); int missing = 0;
        for (auto& p : nuc) { bool f = false; for (auto& g : presets::kNucleoGains) if (p.name == g.first) { f = true; break; } if (! f) { ++missing; std::printf ("   sem ganho calibrado: %s\n", p.name.c_str()); } }
        CHECK (missing == 0, "presets do Núcleo sem ganho em NucleoGains.h (regenerar com packgen gains)");
        auto mods = presets::modulePresets(); std::set<std::string> mn; std::map<std::string, int> per;
        CHECK (mods.size() >= 40, "presets de módulo");
        std::vector<Preset> base; Preset ref;
        for (auto& p : nuc) if (p.name == "Neon Saw Lead") ref = p;
        std::vector<float> cur ((size_t) P_COUNT), before ((size_t) P_COUNT);
        for (auto& m : mods)
        {
            CHECK (mn.insert (m.name).second, ("módulo repetido: " + m.name).c_str()); ++per[m.module];
            CHECK (! m.module.empty() && ! m.values.empty(), "módulo sem valores");
            for (auto& kv : m.values) CHECK (moduleOwnsIndex (m.module, kv.first), ("parâmetro fora do módulo em " + m.name).c_str());
            presets::toParams (ref, nullptr, cur.data()); before = cur;
            applyPresetToParams (m, cur.data());
            int outside = 0, inside = 0; for (int i = 0; i < P_COUNT; ++i) { if (cur[(size_t) i] != before[(size_t) i]) { if (moduleOwnsIndex (m.module, i)) ++inside; else ++outside; } }
            CHECK (outside == 0, ("carregar o módulo alterou parâmetros fora dele: " + m.name).c_str());
            CHECK (inside > 0, ("módulo não alterou nada: " + m.name).c_str());
            // som: finito e com sinal, aplicado sobre o Neon Saw Lead
            static Engine e; std::vector<float> L (512), R (512); cur[P_LIM_ON] = 0;
            Library lib2; e.prepare (44100, 512); e.setParams (cur.data()); e.setLibrary (&lib2); e.setBpm (120); e.noteOn (60, 0.8f); e.noteOn (64, 0.8f);
            float pk = 0; bool fin = true; for (int b = 0; b < 60; ++b) { e.render (L.data(), R.data(), 512); for (int i = 0; i < 512; ++i) { if (! std::isfinite (L[(size_t) i]) || ! std::isfinite (R[(size_t) i])) fin = false; pk = std::max (pk, std::max (std::fabs (L[(size_t) i]), std::fabs (R[(size_t) i]))); } }
            e.allNotesOff();
            CHECK (fin, ("módulo gera NaN: " + m.name).c_str());
            CHECK (pk < 6.0f, ("módulo com pico exagerado: " + m.name).c_str());
            if (m.module != "arp") CHECK (pk > 0.001f, ("módulo deixa o som mudo: " + m.name).c_str());
            Preset q; CHECK (presetFromJson (presetToJson (m), q) && q.module == m.module && q.values.size() == m.values.size(), "módulo json ida e volta");
        }
        for (auto kv : per) std::printf ("   módulos %s: %d\n", kv.first.c_str(), kv.second);
        CHECK (per.size() == 6, "6 módulos");
    }
    // formato 2: metadados ida e volta + formato 1 antigo continua a abrir com família derivada da categoria
    {
        auto nuc = presets::forPack ("nucleo"); int fam = 0, qaAuto = 0, legacy = 0;
        for (auto& p : nuc)
        {
            Preset q; CHECK (presetFromJson (presetToJson (p), q), "formato 2 carrega");
            CHECK (! q.family.empty(), "família preenchida");
            if (! p.qa.empty()) { ++qaAuto; CHECK (q.qa == "auto" && q.family == p.family && q.subfamily == p.subfamily && q.role == p.role && q.engine == p.engine && q.version == 1, "metadados ida e volta"); }
            ++fam;
        }
        const std::string old = "{\"format\":1,\"name\":\"Antigo\",\"category\":\"Baixo\",\"params\":{}}";
        Preset o; CHECK (presetFromJson (old, o) && o.family == "Bass" && o.qa.empty() && o.version == 1, "formato 1 antigo abre"); ++legacy;
        std::printf("nucleo: %d presets, %d com QA automático (não ouvidos), %d teste formato 1\n", fam, qaAuto, legacy);
    }
    for (auto& e : presets::lintErrors()) { ++fails; std::printf ("LINT: %s\n", e.c_str()); }
    std::printf ("%zu presets, master médio %.1f dB\n", total, sumDb / std::max (1, nCal));
    std::printf ("%d verificações, %d falhas\n", checks, fails);
    return fails ? 1 : 0;
}
