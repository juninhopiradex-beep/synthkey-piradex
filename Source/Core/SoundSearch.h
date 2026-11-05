// =============================================================================
//  SYNTHKEY by Piradex — pesquisa por semelhança sonora (Sound Match) e recomendações
//
//  Cada preset de fábrica tem uma impressão digital de 8 características medidas por render real
//  (centroide cedo/tarde, ataque, queda, cauda, largura, graves, ruído) — tabela PresetFp.h,
//  gerada por `packgen fingerprints`. A distância euclidiana entre impressões digitais mede
//  "soa parecido". O mesmo cálculo corre sobre um WAV de referência (Sound Match).
// =============================================================================
#pragma once
#include "PresetCalib.h"
#include "PresetFp.h"
#include <algorithm>
#include <functional>

namespace sk { namespace search
{
using presets::Fingerprint; using presets::fpDistance; using presets::analyseFingerprint;
inline uint32_t presetKey (const std::string& pack, const std::string& name)
{
    uint32_t h = 2166136261u;
    for (unsigned char c : pack) h = (h ^ c) * 16777619u;
    h = (h ^ (unsigned char) '/') * 16777619u;
    for (unsigned char c : name) h = (h ^ c) * 16777619u;
    return h ? h : 1u;
}

// impressão digital guardada para um preset de fábrica (tabela ordenada por chave)
inline bool tableFp (uint32_t key, Fingerprint& out)
{
    size_t lo = 0, hi = kPresetFpN;
    while (lo < hi) { const size_t mid = (lo + hi) / 2; if (kPresetFp[mid].key < key) lo = mid + 1; else hi = mid; }
    if (lo >= kPresetFpN || kPresetFp[lo].key != key) return false;
    for (int i = 0; i < 8; ++i) out.v[i] = kPresetFp[lo].v[i];
    out.ok = true; out.peak = 0.4f; return true;
}
inline bool fpOf (const Preset& p, Fingerprint& out) { return p.module.empty() && tableFp (presetKey (p.pack, p.name), out); }

struct Hit { int index = -1; float dist = 0; };

// os `topN` presets mais parecidos com `ref` (ignora `skip`, módulos e presets sem impressão digital)
inline std::vector<Hit> nearest (const Fingerprint& ref, const std::vector<Preset>& list, int topN, int skip = -1, const std::function<bool (const Preset&)>& accept = {})
{
    std::vector<Hit> hits; Fingerprint f;
    for (size_t i = 0; i < list.size(); ++i)
    {
        if ((int) i == skip || ! fpOf (list[i], f)) continue;
        if (accept && ! accept (list[i])) continue;
        hits.push_back ({ (int) i, fpDistance (ref, f) });
    }
    std::sort (hits.begin(), hits.end(), [] (const Hit& a, const Hit& b) { return a.dist < b.dist; });
    if ((int) hits.size() > topN) hits.resize ((size_t) topN);
    return hits;
}

// recomendações: presets próximos dos que o utilizador gosta (índice, peso — favoritos e estrelas); exclui os próprios
inline std::vector<Hit> recommend (const std::vector<std::pair<int, float>>& liked, const std::vector<Preset>& list, int topN, const std::function<bool (const Preset&)>& accept = {})
{
    std::vector<std::pair<Fingerprint, float>> refs; Fingerprint f;
    for (auto& l : liked) if (l.first >= 0 && l.first < (int) list.size() && fpOf (list[(size_t) l.first], f)) refs.push_back ({ f, l.second });
    std::vector<Hit> hits; if (refs.empty()) return hits;
    std::vector<char> isLiked (list.size(), 0); for (auto& l : liked) if (l.first >= 0 && l.first < (int) list.size()) isLiked[(size_t) l.first] = 1;
    for (size_t i = 0; i < list.size(); ++i)
    {
        if (isLiked[i] || ! fpOf (list[i], f)) continue;
        if (accept && ! accept (list[i])) continue;
        float score = 0, wsum = 0; for (auto& r : refs) { score += r.second / (0.25f + fpDistance (r.first, f)); wsum += r.second; }
        hits.push_back ({ (int) i, -score / std::max (wsum, 1e-6f) });          // dist negativa = maior pontuação primeiro
    }
    std::sort (hits.begin(), hits.end(), [] (const Hit& a, const Hit& b) { return a.dist < b.dist; });
    if ((int) hits.size() > topN) hits.resize ((size_t) topN);
    return hits;
}

// impressão digital de um excerto de áudio de referência (um sample / nota / loop curto). R pode ser nulo (mono).
// Alinha ao primeiro ataque, normaliza e analisa 1,8 s como o render dos presets. Devolve false se for demasiado curto/silencioso.
inline bool fingerprintFromAudio (const float* L, const float* R, size_t n, double sr, Fingerprint& out)
{
    if (! L || n < (size_t) (0.12 * sr)) return false;
    float mx = 0; for (size_t i = 0; i < n; ++i) mx = std::max (mx, std::fabs (R ? 0.5f * (L[i] + R[i]) : L[i]));
    if (mx < 1.0e-4f) return false;
    size_t start = 0; while (start < n && std::fabs (R ? 0.5f * (L[start] + R[start]) : L[start]) < 0.05f * mx) ++start;
    start = start > 64 ? start - 64 : 0;
    const size_t len = (size_t) (1.8 * sr);
    std::vector<float> ml (len, 0.0f), sd (len, 0.0f);
    const float g = 0.4f / mx; float pk = 0;
    for (size_t i = 0; i < len && start + i < n; ++i)
    {
        const float l = L[start + i], r = R ? R[start + i] : L[start + i];
        ml[i] = 0.5f * (l + r) * g; sd[i] = 0.5f * (l - r) * g; pk = std::max (pk, std::fabs (ml[i]));
    }
    analyseFingerprint (ml, sd, sr, std::max (pk, 0.01f), &out);
    return out.ok;
}
}} // namespace sk::search
