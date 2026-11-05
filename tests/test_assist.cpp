// Testes do Sound Match, recomendações, Preset DNA, Assistente, macros automáticas e Layer Builder
#include "Core/PresetBuilder.h"
#include "Core/PresetCalib.h"
#include "Core/PackSpecs.h"
#include "Core/NucleoGains.h"
#include "Core/SoundAssist.h"
#include <cstdio>
#include <set>
using namespace sk; namespace gen = ::sk::presets::gen;
static int fails = 0, checks = 0;
#define CHECK(c, ...) do { ++checks; if (! (c)) { ++fails; std::printf ("FALHA (l.%d): ", __LINE__); std::printf (__VA_ARGS__); std::printf ("\n"); } } while (0)

// renderiza um preset como a medição (mesmas notas) e devolve áudio estéreo a 44,1 kHz
static void renderAudio (Preset p, const Library& lib, std::vector<float>& L, std::vector<float>& R, double sr = 44100.0, int leadSilence = 0, float gain = 1.0f, int blocks = 220)
{
    auto eng = std::make_unique<Engine>(); std::vector<float> prm ((size_t) P_COUNT), l (512), r (512);
    p.set (P_MASTER_VOL, 0.0f); presets::toParams (p, &lib, prm.data()); prm[P_LIM_ON] = 0; prm[P_ARP_ON] = 0; prm[P_CHORD_TYPE] = 0; prm[P_MASTER_VOL] = 0;
    eng->prepare (sr, 512); eng->setParams (prm.data()); eng->setLibrary (&lib); eng->setBpm (120);
    const bool bass = p.category == "Baixo";
    const bool single = bass || p.category == "Bateria" || p.category == "Percussão" || p.category == "FX" || p.category == "Arp" || p.values.size() == 0 || p.get (P_VOICE_MODE) != 0.0f;
    std::vector<int> notes = single ? std::vector<int> { bass ? 36 : (p.category == "Bateria" ? 38 : 60) } : std::vector<int> { 60, 64, 67 };
    for (int b = 0; b < 8; ++b) eng->render (l.data(), r.data(), 512);
    for (int n : notes) eng->noteOn (n, 100.0f / 127.0f);
    L.assign ((size_t) leadSilence, 0.0f); R.assign ((size_t) leadSilence, 0.0f);
    for (int b = 0; b < blocks; ++b) { eng->render (l.data(), r.data(), 512); for (int i = 0; i < 512; ++i) { L.push_back (l[(size_t) i] * gain); R.push_back (r[(size_t) i] * gain); } }
}

int main()
{
    const auto defs = buildPackDefs();
    Library nuc; for (auto& d : defs) if (d.core) buildPackLibrary (d, nuc);
    std::vector<Preset> all, nucleo;                 // todos os presets de fábrica (para a tabela) e os do núcleo (para medir sem packs)
    for (auto& d : defs) { auto l = presets::forPack (d.id); if (d.id == "nucleo") nucleo = l; all.insert (all.end(), l.begin(), l.end()); }

    // ================= tabela de impressões digitais =================
    {
        size_t nonModule = 0, found = 0; std::set<uint32_t> keys; search::Fingerprint f;
        for (auto& p : all) { if (! p.module.empty()) continue; ++nonModule; if (search::fpOf (p, f)) ++found; keys.insert (search::presetKey (p.pack, p.name)); }
        CHECK (found == nonModule, "impressões digitais em falta: %zu de %zu (regenerar com packgen fingerprints)", nonModule - found, nonModule);
        CHECK (keys.size() == nonModule, "colisão de chaves");
        CHECK (search::kPresetFpN == nonModule, "tabela com %zu linhas, presets %zu", search::kPresetFpN, nonModule);
        bool sorted = true; for (size_t i = 1; i < search::kPresetFpN; ++i) if (search::kPresetFp[i].key <= search::kPresetFp[i - 1].key) sorted = false; CHECK (sorted, "tabela não ordenada");
        // a tabela corresponde ao som actual: remede 40 presets do núcleo e compara
        int checked = 0; float worst = 0;
        for (size_t i = 0; i < nucleo.size() && checked < 40; i += nucleo.size() / 40)
        {
            Preset q = nucleo[i]; presets::Fingerprint m; presets::measure (q, nuc, &m); search::Fingerprint t; if (! search::fpOf (nucleo[i], t)) continue;
            worst = std::max (worst, presets::fpDistance (m, t)); ++checked;
        }
        CHECK (checked >= 30 && worst < 0.02f, "tabela desactualizada face ao som: pior distância %.4f em %d presets (regenerar com packgen fingerprints)", worst, checked);
        std::printf ("  tabela: %zu presets, remedição: pior distância %.5f\n", found, worst);
    }
    // ================= pesquisa por semelhança =================
    {
        search::Fingerprint fa; const int ia = 10; search::fpOf (all[(size_t) ia], fa);
        auto h = search::nearest (fa, all, 8, -1);
        CHECK (h.size() == 8 && h[0].index == ia && h[0].dist < 1.0e-6f, "o próprio preset deve ser o mais parecido");
        auto h2 = search::nearest (fa, all, 8, ia);
        CHECK (h2.size() == 8 && h2[0].index != ia, "skip exclui o próprio");
        bool asc = true; for (size_t i = 1; i < h2.size(); ++i) if (h2[i].dist < h2[i - 1].dist) asc = false; CHECK (asc, "resultados ordenados");
        auto h3 = search::nearest (fa, all, 8, ia, [&] (const Preset& p) { return p.pack == "nucleo"; });
        bool allNuc = true; for (auto& x : h3) if (all[(size_t) x.index].pack != "nucleo") allNuc = false; CHECK (allNuc && h3.size() == 8, "filtro accept");
        // coerência: vizinhos mais próximos partilham família (amostra de 300 presets)
        int same = 0, tot = 0; for (size_t i = 0; i < all.size(); i += all.size() / 300)
        { search::Fingerprint f; if (! search::fpOf (all[i], f)) continue; auto nn = search::nearest (f, all, 5, (int) i); for (auto& x : nn) { ++tot; if (all[(size_t) x.index].family == all[i].family) ++same; } }
        const float frac = (float) same / (float) std::max (1, tot);
        std::printf ("  vizinhos na mesma família: %.1f%% (acaso ~%.0f%%)\n", 100.0f * frac, 100.0f * 1.0f / 12.0f);
        CHECK (frac > 0.30f, "vizinhos sonoros pouco coerentes com a família: %.2f", frac);
        // recomendações
        std::vector<std::pair<int, float>> liked = { { 10, 1.0f }, { 20, 1.0f }, { 30, 2.0f } };
        auto rec = search::recommend (liked, all, 10);
        CHECK (rec.size() == 10, "recomendações: 10");
        bool noLiked = true; for (auto& x : rec) if (x.index == 10 || x.index == 20 || x.index == 30) noLiked = false; CHECK (noLiked, "recomendações não repetem os favoritos");
        CHECK (search::recommend ({}, all, 5).empty(), "sem favoritos → sem recomendações");
        auto recOne = search::recommend ({ { 10, 1.0f } }, all, 1, {}); auto nnOne = search::nearest (fa, all, 1, 10);
        CHECK (! recOne.empty() && recOne[0].index == nnOne[0].index, "com um favorito, recomendar = vizinho mais próximo");
    }
    // ================= Sound Match: áudio → impressão digital → presets =================
    {
        int hit1 = 0, hit3 = 0, n = 0; float dsum = 0;
        for (size_t i = 3; i < nucleo.size() && n < 60; i += nucleo.size() / 60)
        {
            const Preset& p = nucleo[i]; if (! p.module.empty()) continue;
            std::vector<float> L, R; renderAudio (p, nuc, L, R, 44100.0, 8000 + (int) (i % 5) * 3000, 0.2f + 0.1f * (float) (i % 7));   // silêncio inicial e ganho arbitrários
            search::Fingerprint fa, ft; if (! search::fingerprintFromAudio (L.data(), R.data(), L.size(), 44100.0, fa)) continue;
            if (! search::fpOf (p, ft)) continue;
            ++n; dsum += presets::fpDistance (fa, ft);
            const int gi = (int) (std::find_if (all.begin(), all.end(), [&] (const Preset& q) { return q.pack == p.pack && q.name == p.name; }) - all.begin());
            auto nn = search::nearest (fa, all, 3); if (! nn.empty() && nn[0].index == gi) ++hit1; for (auto& x : nn) if (x.index == gi) { ++hit3; break; }
        }
        std::printf ("  Sound Match (áudio renderizado, ganho/silêncio arbitrários): top-1 %d/%d, top-3 %d/%d, distância média %.3f\n", hit1, n, hit3, n, dsum / (float) std::max (1, n));
        CHECK (n >= 50, "amostra de Sound Match pequena: %d", n);
        CHECK ((float) hit3 / (float) std::max (1, n) > 0.8f, "Sound Match: o preset de origem devia estar no top-3 em >80%% dos casos (%d/%d)", hit3, n);
        // entradas inválidas
        float z[100] = {}; search::Fingerprint f; CHECK (! search::fingerprintFromAudio (z, nullptr, 100, 44100.0, f), "áudio curto rejeitado");
        std::vector<float> sil (44100, 0.0f); CHECK (! search::fingerprintFromAudio (sil.data(), nullptr, sil.size(), 44100.0, f), "silêncio rejeitado");
        std::vector<float> mono (88200); for (size_t i = 0; i < mono.size(); ++i) mono[i] = 0.5f * std::sin (0.0626f * (float) i) * std::exp (-(float) i / 20000.0f);
        CHECK (search::fingerprintFromAudio (mono.data(), nullptr, mono.size(), 44100.0, f) && f.ok, "mono aceite");
        search::Fingerprint f96; std::vector<float> m96 (192000); for (size_t i = 0; i < m96.size(); ++i) m96[i] = 0.5f * std::sin (0.0313f * (float) i) * std::exp (-(float) i / 40000.0f);
        CHECK (search::fingerprintFromAudio (m96.data(), nullptr, m96.size(), 96000.0, f96) && std::fabs (f96.v[0] - f.v[0]) < 0.3f, "96 kHz coerente com 44,1 kHz (mesma nota): %.2f vs %.2f", f96.v[0], f.v[0]);
    }
    // ================= Preset DNA =================
    {
        int withPack = 0, layered = 0, heavy = 0, light = 0;
        for (auto& p : all)
        {
            if (! p.module.empty()) continue; const auto d = dna::of (p);
            CHECK (! d.engines.empty() || p.get (P_A_ON) < 0.5f, "dna sem motores: %s", p.name.c_str());
            CHECK (d.cpuClass >= 1 && d.cpuClass <= 3 && d.oscVoices >= 0, "dna cpu: %s", p.name.c_str());
            CHECK (! dna::text (d).empty(), "dna texto");
            if (! d.packRequired.empty()) { ++withPack; CHECK (d.packRequired == p.pack || p.pack == "nucleo", "dependência de pack inesperada: %s (%s) pack %s", p.name.c_str(), d.packRequired.c_str(), p.pack.c_str()); }
            if (d.layered) ++layered; if (d.cpuClass == 3) ++heavy; if (d.cpuClass == 1) ++light;
        }
        std::printf("  DNA: %d com pack, %d em camada, CPU leve %d / pesado %d\n", withPack, layered, light, heavy);
        CHECK (withPack > 100 && light > 100 && heavy > 0, "DNA: distribuição implausível");
        Preset q; q.set (P_A_ON, 1.0f); q.set (P_A_TYPE, 4.0f); q.set (fxSlotBase (0), 19.0f); q.set (fxSlotBase (1), 17.0f); q.set (P_ARP_ON, 1.0f); q.set (P_CHORD_TYPE, 3.0f);
        const auto d = dna::of (q); const auto t = dna::text (d);
        CHECK (d.engines.size() == 1 && d.engines[0] == "Granular" && d.fx.size() == 2 && d.fx[0] == "Convolução" && d.fx[1] == "Transientes" && d.arp && d.tonality == "maior" && d.tempoSync, "dna conteúdo: %s", t.c_str());
        CHECK (t.find ("Granular") != std::string::npos && t.find ("Convolução → Transientes") != std::string::npos, "dna texto: %s", t.c_str());
    }
    // ================= Assistente =================
    {
        auto in = assist::parseIntent ("Escuro, largo e com muito movimento!", gen::K_SUST);
        CHECK (in.opt[gen::A_TONE] == 1 && in.opt[gen::A_WIDTH] == 3 && in.opt[gen::A_MOVE] == 3 && in.understood.size() == 3, "parseIntent PT");
        auto en = assist::parseIntent ("bright wide", gen::K_SUST); CHECK (en.opt[gen::A_TONE] == 4 && en.opt[gen::A_WIDTH] == 3, "parseIntent EN");
        CHECK (! assist::parseIntent ("blá blá", gen::K_SUST).any(), "texto sem palavras reconhecidas");
        // variações com intenção: escuro baixa o centroide, brilhante sobe (médias sobre 6 bases sustentadas)
        std::vector<const Preset*> bases; for (auto& p : nucleo) if (p.module.empty() && p.author != "Assistente" && gen::kindOf (p) == gen::K_SUST && p.get (P_A_ON) > 0.5f && p.get (P_B_ON) < 0.5f && p.get (P_ARP_ON) < 0.5f && bases.size() < 6 && (p.name.size() % 3) == 0) bases.push_back (&p);
        CHECK (bases.size() == 6, "bases para o assistente: %zu", bases.size());
        double dDark = 0, dBright = 0; int nd = 0, nb = 0, distinct = 0, total = 0;
        for (auto* b : bases)
        {
            presets::Fingerprint fb; Preset bb = *b; presets::measure (bb, nuc, &fb);
            for (int pass = 0; pass < 2; ++pass)
            {
                std::string rep; auto vs = assist::variations (*b, pass ? "brilhante" : "escuro", 4, 42u, &rep);
                CHECK (rep.find ("Entendi") == 0, "relatório: %s", rep.c_str()); CHECK (vs.size() >= 3, "variações: %zu", vs.size());
                std::set<std::string> nm;
                for (auto& v : vs)
                {
                    ++total; nm.insert (v.preset.name); Preset q = v.preset; presets::Fingerprint fv; const float pk = presets::measure (q, nuc, &fv);
                    Preset rt; CHECK (presetFromJson (presetToJson (q), rt) && rt.name == q.name, "json da variação"); CHECK (pk > 0.002f && fv.ok, "variação muda: %s", q.name.c_str());
                    (pass ? dBright : dDark) += fv.v[0] - fb.v[0]; ++(pass ? nb : nd);
                }
                distinct += (int) nm.size() == (int) vs.size();
            }
        }
        std::printf ("  Assistente: Δcentroide médio  «escuro» %+.3f  «brilhante» %+.3f  (oitavas, %d + %d variações)\n", dDark / nd, dBright / nb, nd, nb);
        CHECK (dDark / nd < -0.05 && dBright / nb > 0.05, "a intenção «escuro/brilhante» devia deslocar o centroide em sentidos opostos (%.3f / %.3f)", dDark / nd, dBright / nb);
        CHECK (distinct == 12, "variações com nomes repetidos");
        auto a = assist::variations (*bases[0], "largo", 4, 7u), b = assist::variations (*bases[0], "largo", 4, 7u), c = assist::variations (*bases[0], "largo", 4, 8u);
        bool same = a.size() == b.size(); for (size_t i = 0; same && i < a.size(); ++i) same = a[i].preset.values == b[i].preset.values; CHECK (same, "variações não determinísticas");
        bool diff = false; for (size_t i = 0; i < std::min (a.size(), c.size()); ++i) if (a[i].preset.values != c[i].preset.values) diff = true; CHECK (diff, "sementes diferentes devem dar resultados diferentes");
        std::string rep; auto freeV = assist::variations (*bases[1], "", 3, 3u, &rep); CHECK (freeV.size() == 3 && rep.find ("livres") != std::string::npos, "variações livres: %s", rep.c_str());
    }
    // ================= macros automáticas =================
    {
        int tried = 0, withMacros = 0, neutral = 0, active = 0; double worstNeutral = 0;
        for (size_t i = 5; i < nucleo.size() && tried < 40; i += nucleo.size() / 40)
        {
            Preset p = nucleo[i]; if (! p.module.empty() || p.get (P_ARP_ON) > 0.5f) continue; ++tried;
            // sons "de utilizador": sem macros atribuídas (os de fábrica já trazem as suas), e macros a zero
            for (int sl = 0; sl < kNumModSlots; ++sl) { const int b = modSlotBase (sl); const int src = (int) p.get (b); if (src >= MS_MACRO1 && src < MS_MACRO1 + kNumMacros) { p.set (b, 0.0f); p.set (b + 1, 0.0f); p.set (b + 2, 0.0f); } }
            for (int k = 0; k < kNumMacros; ++k) p.set (P_MACRO1 + k, 0.0f);
            const Preset orig = p; auto made = assist::autoMacros (p); if (made.empty()) continue; ++withMacros;
            for (auto& m : made) CHECK (m.index >= 1 && m.index <= kNumMacros && std::fabs (p.get (P_MACRO1 + m.index - 1)) < 1e-6f, "macro %d deve ficar a 0", m.index);
            // som: macros a 0 = original (diferença numérica mínima); macros a 1,0 = som diferente
            std::vector<float> L0, R0, L1, R1, L2, R2; renderAudio (orig, nuc, L0, R0, 44100.0, 0, 1.0f, 60); renderAudio (p, nuc, L1, R1, 44100.0, 0, 1.0f, 60);
            double dmax = 0, pmax = 1e-9; for (size_t k = 0; k < L0.size(); ++k) { dmax = std::max (dmax, (double) std::fabs (L0[k] - L1[k])); pmax = std::max (pmax, (double) std::fabs (L0[k])); }
            worstNeutral = std::max (worstNeutral, dmax / pmax); if (dmax / pmax < 0.01) ++neutral;
            Preset hi = p; for (auto& m : made) hi.set (P_MACRO1 + m.index - 1, 1.0f); renderAudio (hi, nuc, L2, R2, 44100.0, 0, 1.0f, 60);
            double dd = 0; for (size_t k = 0; k < L0.size(); ++k) dd += std::fabs (L0[k] - L2[k]); dd /= (double) L0.size(); double ee = 0; for (float v : L0) ee += std::fabs (v); ee /= (double) L0.size();
            if (dd / std::max (ee, 1e-9) > 0.05) ++active;
        }
        std::printf ("  Macros automáticas: %d/%d presets receberam macros; neutras a 0: %d; audíveis a 1,0: %d; pior desvio neutro %.4f\n", withMacros, tried, neutral, active, worstNeutral);
        CHECK (withMacros >= tried * 3 / 4, "poucos presets receberam macros: %d/%d", withMacros, tried);
        { Preset f = nucleo[40]; int before = 0; for (int sl = 0; sl < kNumModSlots; ++sl) { const int src = (int) f.get (modSlotBase (sl)); if (src >= MS_MACRO1 && src < MS_MACRO1 + kNumMacros) ++before; }
          auto mk = assist::autoMacros (f); std::printf ("  preset de fábrica com %d rotas de macro: autoMacros acrescentou %zu\n", before, mk.size()); CHECK (before == 8 ? mk.empty() : true, "preset de fábrica já tem as 8 macros: nada a acrescentar"); }
        CHECK (neutral == withMacros, "macros a 0 não neutras (pior %.4f)", worstNeutral);
        CHECK (active >= withMacros * 3 / 4, "macros a 1,0 pouco audíveis: %d/%d", active, withMacros);
    }
    // ================= Layer Builder =================
    {
        int built = 0, ok = 0;
        std::vector<const Preset*> singles; for (size_t i = 0; i < nucleo.size(); ++i) { auto& p = nucleo[i]; if (p.module.empty() && p.get (P_A_ON) > 0.5f && p.get (P_B_ON) < 0.5f && p.get (P_ARP_ON) < 0.5f && gen::kindOf (p) == gen::K_SUST) singles.push_back (&p); }
        for (size_t s = 0; s < 8 && s * 17 < singles.size(); ++s)
        {
            const Preset& a = *singles[s * 17]; search::Fingerprint fa; if (! search::fpOf (a, fa)) continue;
            const int ia = (int) (std::find_if (all.begin(), all.end(), [&] (const Preset& q) { return q.pack == a.pack && q.name == a.name; }) - all.begin());
            auto ps = assist::suggestPartners (a, fa, all, 5, ia, [] (const Preset& q) { return q.pack == "nucleo"; });
            CHECK (ps.size() == 5, "parceiros sugeridos: %zu", ps.size());
            for (int mode = 0; mode < 2 && ! ps.empty(); ++mode)
            {
                Preset out; const Preset& b = all[(size_t) ps[0].index]; ++built;
                if (! assist::buildLayer (a, b, (assist::LayerMode) mode, 60, out)) { CHECK (false, "buildLayer falhou"); continue; }
                presets::Fingerprint fo; const float pk = presets::measure (out, nuc, &fo); Preset rt;
                const bool valid = presetFromJson (presetToJson (out), rt) && rt.name == out.name && out.get (P_A_ON) > 0.5f && out.get (P_B_ON) > 0.5f && pk > 0.002f && fo.ok;
                CHECK (valid, "camada inválida: %s", out.name.c_str()); if (valid) ++ok;
                if (mode == 1) CHECK (out.get (P_A_KEYHI) == 59.0f && out.get (P_B_KEYLO) == 60.0f, "zonas do split");
                else CHECK (out.get (P_A_KEYHI) == 127.0f && out.get (P_B_KEYLO) == 0.0f, "zonas do stack");
                CHECK (out.get (P_B_TYPE) == b.get (P_A_TYPE) && out.get (P_B_UNI) == b.get (P_A_UNI), "motor B deve copiar o motor A do parceiro");
            }
        }
        std::printf ("  Layer Builder: %d camadas construídas, %d válidas e audíveis\n", built, ok);
        CHECK (built >= 12 && ok == built, "camadas válidas %d/%d", ok, built);
        Preset two = *singles[0]; Preset o2; assist::buildLayer (*singles[1], *singles[2], assist::LAYER_STACK, 60, two); CHECK (! assist::buildLayer (two, *singles[3], assist::LAYER_STACK, 60, o2), "recusa camada sobre preset que já usa o motor B");
    }
    std::printf ("test_assist: %d verificações, %d falhas\n", checks, fails);
    return fails ? 1 : 0;
}
