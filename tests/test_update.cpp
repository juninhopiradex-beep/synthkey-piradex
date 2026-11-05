// Testes do manifesto de atualização de packs (sem rede): análise, validação, comparação de versões, decisão
#include "Core/PackUpdate.h"
#include <cstdio>
using namespace sk::update;
static int fails = 0, checks = 0;
#define CHECK(c, ...) do { ++checks; if (! (c)) { ++fails; std::printf ("FALHA (l.%d): ", __LINE__); std::printf (__VA_ARGS__); std::printf ("\n"); } } while (0)
static std::string H (char c) { return std::string (64, c); }
int main()
{
    CHECK (compareVersions ("1.2.10", "1.2.9") > 0 && compareVersions ("1.2.9", "1.2.10") < 0 && compareVersions ("1.0", "1.0.0") == 0 && compareVersions ("2", "1.9.9") > 0 && compareVersions ("1.2.0-beta", "1.2.0") == 0 && compareVersions ("", "0.0.1") < 0, "versões");
    Manifest m; std::string err; int skipped = 0;
    const std::string good = "{\"schema\":1,\"published\":\"2026-11-05\",\"packs\":["
        "{\"id\":\"trap\",\"name\":\"Trap\",\"version\":\"1.1.0\",\"url\":\"https://x.pt/a.bfpack\",\"sha256\":\"" + H ('A') + "\",\"size\":1000,\"notes\":\"novo\"},"
        "{\"id\":\"house\",\"version\":\"1.0.0\",\"url\":\"https://x.pt/b.bfpack\",\"sha256\":\"" + H ('b') + "\"},"
        "{\"id\":\"nucleo\",\"version\":\"9.9.9\",\"url\":\"https://x.pt/c.bfpack\",\"sha256\":\"" + H ('c') + "\"},"
        "{\"id\":\"edm\",\"version\":\"1.0.0\",\"url\":\"http://x.pt/d.bfpack\",\"sha256\":\"" + H ('d') + "\"},"            // http → rejeitado
        "{\"id\":\"pop\",\"version\":\"1.0.0\",\"url\":\"https://x.pt/e.bfpack\"},"                                           // sem sha256 → rejeitado
        "{\"id\":\"../x\",\"version\":\"1.0.0\",\"url\":\"https://x.pt/f.bfpack\",\"sha256\":\"" + H ('f') + "\"},"        // id inseguro → rejeitado
        "{\"id\":\"\",\"version\":\"1\",\"url\":\"https://x.pt/g\",\"sha256\":\"" + H ('1') + "\"}]}";
    CHECK (parseManifest (good, m, &err, &skipped), "manifesto válido: %s", err.c_str());
    CHECK (m.packs.size() == 3 && skipped == 4, "entradas aceites %zu, rejeitadas %d", m.packs.size(), skipped);
    CHECK (m.packs[0].sha256 == H ('a'), "sha256 em minúsculas");
    std::map<std::string, std::string> inst = { { "nucleo", "1.0.0" }, { "trap", "1.0.0" }, { "house", "1.0.0" } };
    auto p = pending (m, inst);
    CHECK (p.size() == 1 && p[0].remote.id == "trap" && p[0].installedVersion == "1.0.0" && ! p[0].isNew, "só o trap tem atualização (house igual, núcleo excluído): %zu", p.size());
    inst["trap"] = "1.1.0"; CHECK (pending (m, inst).empty(), "versão igual não atualiza");
    inst["trap"] = "2.0.0"; CHECK (pending (m, inst).empty(), "versão instalada mais recente não é rebaixada");
    inst.erase ("house"); p = pending (m, inst); bool newHouse = false; for (auto& x : p) if (x.remote.id == "house" && x.isNew) newHouse = true; CHECK (newHouse, "pack não instalado aparece como novo");
    CHECK (! parseManifest ("isto não é json", m, &err), "json inválido"); CHECK (! parseManifest ("{\"schema\":2,\"packs\":[]}", m, &err), "schema desconhecido");
    CHECK (! parseManifest ("{\"schema\":1}", m, &err), "sem lista de packs"); CHECK (parseManifest ("{\"schema\":1,\"packs\":[]}", m, &err) && m.packs.empty(), "lista vazia");
    CHECK (parseManifest (exampleManifest(), m, &err) && m.packs.size() == 1, "o exemplo da documentação tem de ser válido");
    std::printf ("test_update: %d verificações, %d falhas\n", checks, fails);
    return fails ? 1 : 0;
}
