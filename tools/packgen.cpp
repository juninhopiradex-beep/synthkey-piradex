// =============================================================================
//  SYNTHKEY by Piradex — gerador de packs (.bfpack) e ganhos do Núcleo
//
//  Uso:
//    packgen packs  <pasta_saída> <pasta_capas> <segredo_hex | -> [id_do_pack ...]
//    packgen gains  <ficheiro_saída.h>                      (calibra os presets do Núcleo)
//    packgen presets <pasta_saída>                          (exporta todos os presets em .skpreset, p/ inspeção)
//    packgen fingerprints <PresetFp.h>                      (impressões digitais sonoras de todos os presets — Sound Match / semelhança)
//    packgen curate <GenCurated.h> [eps]                    (gera e cura as variações de presets: mede, rejeita iguais, calibra)
// =============================================================================
#include "Core/PackIO.h"
#include "Core/PresetBuilder.h"
#include "Core/PresetCalib.h"
#include "Core/SoundSearch.h"
#if __has_include("Secrets.h")
 #include "Secrets.h"
 #define SK_HAVE_SECRETS 1
#endif
#include <algorithm>
#include <atomic>
#include <cstdio>
#include <fstream>
#include <map>
#include <set>
#include <thread>
#include <iostream>
#include <sstream>
#if __has_include(<filesystem>)
 #include <filesystem>
 namespace fs = std::filesystem;
#endif

using namespace sk;

static std::string slurp (const std::string& path) { std::ifstream f (path, std::ios::binary); std::ostringstream ss; ss << f.rdbuf(); return ss.str(); }
static bool spit (const std::string& path, const std::string& data) { std::ofstream f (path, std::ios::binary); f.write (data.data(), (std::streamsize) data.size()); return (bool) f; }

static std::string secretFromHeader()
{
   #ifdef SK_HAVE_SECRETS
    return sk::secrets::macSecret();
   #else
    std::fprintf (stderr, "Source/Secrets.h nao encontrado\n"); std::exit (2);
   #endif
}

static Library nucleoLib (const std::vector<PackDef>& defs) { Library l; for (auto& d : defs) if (d.core) buildPackLibrary (d, l); return l; }


// ------------------------------------------------------------------------------------------------
//  curate: para cada grupo (família do Núcleo | pack de expansão) gera variações das bases, mede cada
//  uma, rejeita as que soam a menos de `eps` de qualquer preset já aceite e guarda código + ganho.
// ------------------------------------------------------------------------------------------------
static int runCurate (const std::string& outPath, float eps, const std::vector<PackDef>& defs, const Library& nuc)
{
    using namespace sk::presets;
    struct Q { const char* key; int target; };
    static const Q famQ[] = { { "Bass", 180 }, { "Leads", 150 }, { "Pads", 150 }, { "Keys", 140 }, { "Plucks", 130 }, { "Synths", 120 }, { "Strings", 90 }, { "Brass", 70 }, { "Bells", 80 },
        { "Mallets", 60 }, { "Guitars", 80 }, { "Vocals", 70 }, { "Organs", 45 }, { "Woodwinds", 40 }, { "Drums", 100 }, { "Percussion", 80 }, { "Arpeggios", 100 }, { "Sequences", 90 },
        { "Chords", 65 }, { "Textures", 80 }, { "SFX", 80 }, { "Cinematic", 65 }, { "Hybrid", 55 }, { "Experimental", 30 } };
    static const Q packQ[] = { { "amapiano", 150 }, { "kizomba", 120 }, { "afrobeat", 140 }, { "kuduro", 110 }, { "rnb", 130 }, { "trap", 130 }, { "drill", 100 },
        { "house", 130 }, { "edm", 120 }, { "pop", 100 }, { "cinematic", 100 } };
    struct Row { std::string pack, base; uint32_t code; int dup; float gain; };
    std::vector<Row> rowsOut; std::set<std::string> names;
    const unsigned nth = std::max (1u, std::thread::hardware_concurrency());
    auto famOf = [] (const Preset& p) { return p.family.empty() ? familyForCategory (p.category) : p.family; };

    // nomes já usados por presets escritos à mão em TODOS os packs
    for (auto& d : defs) for (auto& p : forPackHand (d.id)) names.insert (p.name);

    auto runGroup = [&] (const std::string& packId, const std::string& groupKey, int target, const std::vector<Preset>& handAll, const Library& lib)
    {
        std::vector<const Preset*> hand; for (auto& p : handAll) if (packId != "nucleo" || famOf (p) == groupKey) hand.push_back (&p);
        // todos os presets da mesma família/pack contam para a quota; a base de variações é o mesmo conjunto
        std::vector<Fingerprint> fps; fps.reserve ((size_t) target + 64);
        for (auto* p : hand) { Preset q = *p; Fingerprint f; measure (q, lib, &f); if (f.ok) fps.push_back (f); }
        int need = target - (int) hand.size(); int got = 0; std::set<std::pair<std::string, uint32_t>> seen;
        std::vector<const Preset*> bases = hand; std::sort (bases.begin(), bases.end(), [] (const Preset* a, const Preset* b) { return gen::hashStr (a->name) < gen::hashStr (b->name); });
        for (uint32_t round = 0; round < 60 && got < need; ++round)
        {
            struct Cand { const Preset* base; uint32_t code; Preset p; Fingerprint f; };
            std::vector<Cand> cands;
            for (auto* b : bases) { const uint32_t code = gen::candidateCode (*b, round); if (code == 0 || seen.count ({ b->name, code })) continue; seen.insert ({ b->name, code }); cands.push_back ({ b, code, gen::derive (*b, code), {} }); }
            std::atomic<size_t> next { 0 }; std::vector<std::thread> th;
            for (unsigned t = 0; t < nth; ++t) th.emplace_back ([&] { for (size_t i; (i = next++) < cands.size();) measure (cands[i].p, lib, &cands[i].f); });
            for (auto& t : th) t.join();
            for (auto& c : cands)
            {
                if (got >= need) break; if (! c.f.ok) continue;
                bool far = true; for (auto& f : fps) if (fpDistance (f, c.f) < eps) { far = false; break; }
                if (! far) continue;
                const float gain = c.p.get (P_MASTER_VOL); if (gain <= -23.9f || gain >= 5.9f) continue;       // nível fora da faixa calibrável
                int dup = 0; std::string nm = c.p.name; while (names.count (nm)) { ++dup; nm = c.p.name + " " + std::to_string (dup + 1); }
                names.insert (nm); fps.push_back (c.f); rowsOut.push_back ({ packId, c.base->name, c.code, dup, gain }); ++got;
            }
        }
        std::printf ("  %-12s %-13s quota %3d  à mão %3zu  geradas %3d  %s\n", packId.c_str(), groupKey.c_str(), target, hand.size(), got, got < need ? "(FALTAM)" : "");
        std::fflush (stdout);
    };

    { const auto hand = forPackHand ("nucleo"); for (auto& q : famQ) runGroup ("nucleo", q.key, q.target, hand, nuc); }
    for (auto& q : packQ)
    {
        const PackDef* d = nullptr; for (auto& x : defs) if (x.id == q.key) d = &x; if (! d) continue;
        Library lib; lib.appendShared (nuc); buildPackLibrary (*d, lib);
        runGroup (d->id, d->id, q.target, forPackHand (d->id), lib);
    }
    std::string out = "// GERADO por tools/packgen (modo curate) — variações curadas de presets: base + código de variação + ganho calibrado.\n// Estado de validação: \"auto\" (medido e filtrado por máquina; nunca ouvido).\n#pragma once\n#include <cstddef>\n#include <cstdint>\nnamespace sk { namespace presets\n{\nstruct GenRow { const char* pack; const char* base; uint32_t code; uint8_t dup; float gain; };\ninline const GenRow kGenCurated[] =\n{\n";
    for (auto& r : rowsOut) { char b[400]; std::snprintf (b, sizeof b, "    { \"%s\", \"%s\", 0x%xu, %d, %.2ff },\n", r.pack.c_str(), r.base.c_str(), r.code, r.dup, (double) r.gain); out += b; }
    out += "    { \"\", \"\", 0, 0, 0.0f }\n};\ninline constexpr size_t kGenCuratedN = " + std::to_string (rowsOut.size()) + ";\n}} // namespace sk::presets\n";
    spit (outPath, out); std::printf ("%zu variações curadas em %s\n", rowsOut.size(), outPath.c_str()); return 0;
}

int main (int argc, char** argv)
{
    if (argc < 3) { std::fprintf (stderr, "uso: packgen packs|gains|presets ...\n"); return 2; }
    const std::string mode = argv[1];
    const auto defs = buildPackDefs();
    const Library nuc = nucleoLib (defs);

    if (mode == "curate") return runCurate (argv[2], argc > 3 ? (float) std::atof (argv[3]) : 0.6f, defs, nuc);
    if (mode == "fingerprints")
    {
        struct Row { uint32_t key; presets::Fingerprint f; std::string name; };
        std::vector<Row> rows; const unsigned nth = std::max (1u, std::thread::hardware_concurrency());
        for (auto& d : defs)
        {
            Library lib; lib.appendShared (nuc); if (! d.core) buildPackLibrary (d, lib);
            auto list = presets::forPack (d.id); std::vector<Row> part (list.size());
            std::atomic<size_t> next { 0 }; std::vector<std::thread> th;
            for (unsigned t = 0; t < nth; ++t) th.emplace_back ([&] { for (size_t i; (i = next++) < list.size();) { Preset q = list[i]; part[i].key = search::presetKey (q.pack, q.name); part[i].name = q.name; if (q.module.empty()) presets::measure (q, lib, &part[i].f); } });
            for (auto& t : th) t.join();
            for (auto& r : part) if (r.f.ok) rows.push_back (r);
            std::printf ("  %-10s %zu impressões digitais\n", d.id.c_str(), list.size()); std::fflush (stdout);
        }
        std::sort (rows.begin(), rows.end(), [] (const Row& a, const Row& b) { return a.key < b.key; });
        for (size_t i = 1; i < rows.size(); ++i) if (rows[i].key == rows[i - 1].key) { std::fprintf (stderr, "colisão de chave: %s / %s\n", rows[i].name.c_str(), rows[i - 1].name.c_str()); return 1; }
        std::string out = "// GERADO por tools/packgen (modo fingerprints) — impressões digitais sonoras dos presets de fábrica. NÃO EDITAR À MÃO.\n#pragma once\n#include <cstddef>\n#include <cstdint>\nnamespace sk { namespace search\n{\nstruct FpRow { uint32_t key; float v[8]; };\ninline constexpr FpRow kPresetFp[] = {\n";
        for (auto& r : rows) { char b[300]; std::snprintf (b, sizeof b, "    { %uu, { %.4ff, %.4ff, %.4ff, %.4ff, %.4ff, %.4ff, %.4ff, %.4ff } },\n", r.key, (double) r.f.v[0], (double) r.f.v[1], (double) r.f.v[2], (double) r.f.v[3], (double) r.f.v[4], (double) r.f.v[5], (double) r.f.v[6], (double) r.f.v[7]); out += b; }
        out += "    { 0u, { 0, 0, 0, 0, 0, 0, 0, 0 } }\n};\ninline constexpr size_t kPresetFpN = " + std::to_string (rows.size()) + ";\n}} // namespace sk::search\n";
        spit (argv[2], out); std::printf ("%zu impressões digitais em %s\n", rows.size(), argv[2]); return 0;
    }
    if (mode == "gains")
    {
        auto list = presets::forPackHand ("nucleo");
        std::string out = "// GERADO por tools/packgen (modo gains) — ganhos calibrados dos presets do Núcleo\n#pragma once\n#include <utility>\nnamespace sk { namespace presets\n{\ninline const std::pair<const char*, float> kNucleoGains[] =\n{\n";
        for (auto& p : list) { presets::calibrate (p, nuc); char b[200]; std::snprintf (b, sizeof b, "    { \"%s\", %.2ff },\n", p.name.c_str(), p.get (P_MASTER_VOL)); out += b; }
        out += "};\n}} // namespace sk::presets\n";
        spit (argv[2], out); std::printf ("%zu ganhos escritos em %s\n", list.size(), argv[2]); return 0;
    }
    if (mode == "presets")
    {
        fs::create_directories (argv[2]); int n = 0;
        for (auto& d : defs)
        {
            Library lib; lib.appendShared (nuc); if (! d.core) buildPackLibrary (d, lib);
            for (auto& p : presets::forPack (d.id)) { presets::calibrate (p, lib); std::string leaf = d.id + "_" + p.name; for (auto& c : leaf) if (! std::isalnum ((unsigned char) c)) c = '_'; spit (std::string (argv[2]) + "/" + leaf + ".skpreset", presetToJson (p)); ++n; }
        }
        std::printf ("%d presets exportados\n", n); return 0;
    }
    if (mode == "packs" && argc >= 5)
    {
        const std::string outDir = argv[2], coverDir = argv[3], secret = std::string (argv[4]) == "-" ? secretFromHeader() : crypto::fromHex (argv[4]);
        std::vector<std::string> only; for (int i = 5; i < argc; ++i) only.push_back (argv[i]);
        fs::create_directories (outDir);
        for (auto& d : defs)
        {
            if (d.core) continue;
            if (! only.empty() && std::find (only.begin(), only.end(), d.id) == only.end()) continue;
            Library lib; buildPackLibrary (d, lib);
            Library full; full.appendShared (nuc); full.appendShared (lib);
            auto list = presets::forPack (d.id);
            for (auto& p : list) presets::calibrate (p, full);
            const std::string cover = slurp (coverDir + "/" + d.id + ".jpg");
            const std::string bytes = packio::buildPack (d, lib, list, cover, secret);
            const std::string path = outDir + "/SYNTHKEY-" + d.name + "-" + d.version + ".bfpack";
            std::string safe = path; // sem espaços/acentos no nome de ficheiro
            const std::string leaf = "SYNTHKEY_Pack_" + d.id + "_" + d.version + ".bfpack"; safe = outDir + "/" + leaf;
            if (! spit (safe, bytes)) { std::fprintf (stderr, "erro a escrever %s\n", safe.c_str()); return 1; }
            std::printf ("%-10s %5.1f MB  %2zu instrumentos  %2zu presets  -> %s\n", d.id.c_str(), bytes.size() / 1048576.0, lib.inst.size(), list.size(), leaf.c_str());
        }
        return 0;
    }
    std::fprintf (stderr, "argumentos inválidos\n"); return 2;
}
