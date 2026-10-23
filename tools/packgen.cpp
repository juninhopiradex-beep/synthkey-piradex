// =============================================================================
//  SYNTHKEY by Piradex — gerador de packs (.bfpack) e ganhos do Núcleo
//
//  Uso:
//    packgen packs  <pasta_saída> <pasta_capas> <segredo_hex | -> [id_do_pack ...]
//    packgen gains  <ficheiro_saída.h>                      (calibra os presets do Núcleo)
//    packgen presets <pasta_saída>                          (exporta todos os presets em .skpreset, p/ inspeção)
// =============================================================================
#include "Core/PackIO.h"
#include "Core/PresetBuilder.h"
#include "Core/PresetCalib.h"
#if __has_include("Secrets.h")
 #include "Secrets.h"
 #define SK_HAVE_SECRETS 1
#endif
#include <cstdio>
#include <fstream>
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

int main (int argc, char** argv)
{
    if (argc < 3) { std::fprintf (stderr, "uso: packgen packs|gains|presets ...\n"); return 2; }
    const std::string mode = argv[1];
    const auto defs = buildPackDefs();
    const Library nuc = nucleoLib (defs);

    if (mode == "gains")
    {
        auto list = presets::nucleo();
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
