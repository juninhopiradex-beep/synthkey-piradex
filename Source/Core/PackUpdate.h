// =============================================================================
//  SYNTHKEY by Piradex — atualizações de packs: manifesto remoto, comparação de versões, decisão do que atualizar
//
//  Formato do manifesto (JSON, publicado no endereço configurado — ver docs/PACKS.md):
//    { "schema": 1, "published": "2026-11-05",
//      "packs": [ { "id": "trap", "name": "Trap", "version": "1.2.0", "url": "https://…/SYNTHKEY_Pack_trap_1.2.0.bfpack",
//                   "sha256": "<64 hex do ficheiro .bfpack>", "size": 123456789, "notes": "5 presets novos" } ] }
//  Regras de segurança: só https, sha256 obrigatório (verificado antes de instalar), e o instalador ainda valida o HMAC do próprio pack.
// =============================================================================
#pragma once
#include "MiniJson.h"
#include <cctype>
#include <map>

namespace sk { namespace update
{
struct Remote { std::string id, name, version, url, sha256, notes; long long size = 0; };
struct Manifest { int schema = 1; std::string published; std::vector<Remote> packs; };
struct Pending { Remote remote; std::string installedVersion; bool isNew = false; };

// "1.2.10" > "1.2.9"; ignora sufixos não numéricos ("1.2.0-beta" = "1.2.0"); versões vazias = 0
inline int compareVersions (const std::string& a, const std::string& b)
{
    auto split = [] (const std::string& s) { std::vector<long long> v; long long cur = 0; bool have = false;
        for (char c : s) { if (std::isdigit ((unsigned char) c)) { cur = cur * 10 + (c - '0'); have = true; } else if (c == '.') { v.push_back (have ? cur : 0); cur = 0; have = false; } else break; }
        if (have) v.push_back (cur); return v; };
    auto x = split (a), y = split (b); const size_t n = std::max (x.size(), y.size());
    for (size_t i = 0; i < n; ++i) { const long long p = i < x.size() ? x[i] : 0, q = i < y.size() ? y[i] : 0; if (p != q) return p < q ? -1 : 1; }
    return 0;
}

inline bool isHex64 (const std::string& s) { if (s.size() != 64) return false; for (char c : s) if (! std::isxdigit ((unsigned char) c)) return false; return true; }
inline bool isSafeId (const std::string& s) { if (s.empty() || s.size() > 40) return false; for (char c : s) if (! (std::isalnum ((unsigned char) c) || c == '_' || c == '-')) return false; return true; }
inline bool isHttps (const std::string& u) { return u.size() > 12 && u.compare (0, 8, "https://") == 0 && u.find (' ') == std::string::npos && u.find ('\n') == std::string::npos; }

// lê o manifesto; entradas inválidas são ignoradas (e contadas em `skipped`); devolve false se o JSON não for um manifesto
inline bool parseManifest (const std::string& text, Manifest& out, std::string* err = nullptr, int* skipped = nullptr)
{
    out = Manifest(); if (skipped) *skipped = 0;
    json::Value v; if (text.size() > 4 * 1024 * 1024 || ! json::parse (text, v) || ! v.isObject()) { if (err) *err = "manifesto inválido"; return false; }
    out.schema = (int) v.getNumber ("schema", 1); out.published = v.getString ("published");
    if (out.schema != 1) { if (err) *err = "versão de manifesto não suportada: " + std::to_string (out.schema); return false; }
    const json::Value* arr = v.find ("packs"); if (! arr || ! arr->isArray()) { if (err) *err = "manifesto sem lista de packs"; return false; }
    for (auto& e : arr->arr)
    {
        Remote r; r.id = e.getString ("id"); r.name = e.getString ("name", r.id); r.version = e.getString ("version"); r.url = e.getString ("url");
        r.sha256 = e.getString ("sha256"); for (auto& c : r.sha256) c = (char) std::tolower ((unsigned char) c); r.notes = e.getString ("notes"); r.size = (long long) e.getNumber ("size", 0);
        const bool ok = e.isObject() && isSafeId (r.id) && ! r.version.empty() && isHttps (r.url) && isHex64 (r.sha256) && r.size >= 0;
        if (ok) out.packs.push_back (r); else if (skipped) ++*skipped;
    }
    return true;
}

// o que há para atualizar: packs instalados com versão remota mais recente (+ packs novos, marcados isNew, que nunca se instalam sozinhos).
// `installed`: id → versão. O Núcleo (core) nunca se atualiza por aqui: vai com o plugin.
inline std::vector<Pending> pending (const Manifest& m, const std::map<std::string, std::string>& installed, const std::vector<std::string>& coreIds = { "nucleo" })
{
    std::vector<Pending> out;
    for (auto& r : m.packs)
    {
        bool core = false; for (auto& c : coreIds) if (c == r.id) core = true; if (core) continue;
        auto it = installed.find (r.id);
        if (it == installed.end()) { out.push_back ({ r, "", true }); continue; }
        if (compareVersions (r.version, it->second) > 0) out.push_back ({ r, it->second, false });
    }
    return out;
}

inline std::string exampleManifest()
{
    return "{\n  \"schema\": 1,\n  \"published\": \"2026-11-05\",\n  \"packs\": [\n    {\n      \"id\": \"trap\", \"name\": \"Trap\", \"version\": \"1.0.1\",\n"
           "      \"url\": \"https://exemplo.pt/packs/SYNTHKEY_Pack_trap_1.0.1.bfpack\",\n      \"sha256\": \"0000000000000000000000000000000000000000000000000000000000000000\",\n"
           "      \"size\": 12345678, \"notes\": \"exemplo\"\n    }\n  ]\n}\n";
}
}} // namespace sk::update
