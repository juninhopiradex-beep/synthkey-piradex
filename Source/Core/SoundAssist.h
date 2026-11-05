// =============================================================================
//  SYNTHKEY by Piradex — assistente de sound design, macros automáticas e Layer Builder
//
//  Tudo opera sobre presets reais (valores de parâmetros), por isso o resultado é sempre carregável e
//  audível; o gerador de variações é o mesmo `gen::derive` que produziu a biblioteca de fábrica.
// =============================================================================
#pragma once
#include "GenAxes.h"
#include "SoundSearch.h"
#include "PresetDna.h"
#include <cctype>

namespace sk { namespace assist
{
using presets::Fingerprint; using presets::fpDistance; namespace gen = ::sk::presets::gen;
// ------------------------------------------------------------------------------------------- intenção em texto
inline std::string fold (const std::string& s)       // minúsculas sem acentos (UTF-8 PT)
{
    static const char* from[] = { "á", "à", "â", "ã", "ä", "é", "ê", "è", "í", "î", "ó", "ô", "õ", "ö", "ú", "ü", "ç", "-" };
    static const char* to[]   = { "a", "a", "a", "a", "a", "e", "e", "e", "i", "i", "o", "o", "o", "o", "u", "u", "c", "" };
    std::string o; for (unsigned char c : s) o += (char) std::tolower (c);
    for (size_t k = 0; k < sizeof (from) / sizeof (from[0]); ++k)
        for (size_t pos = 0; (pos = o.find (from[k], pos)) != std::string::npos;) { o.replace (pos, std::strlen (from[k]), to[k]); pos += std::strlen (to[k]); }
    return o;
}

struct Intent { int opt[gen::A_COUNT] = {}; std::vector<std::string> understood; bool any() const { for (int a = 0; a < gen::A_COUNT; ++a) if (opt[a]) return true; return false; } };

// palavras (PT/EN) → (eixo, opção). A opção de FORMA depende do tipo de preset (sustentado vs. curto), por isso recebe o tipo.
inline Intent parseIntent (const std::string& text, gen::Kind kind)
{
    using namespace gen;
    const bool sust = (kind == K_SUST);
    struct W { const char* w; Axis a; int o; int oSust; };
    static const W table[] = {
        { "escuro", A_TONE, 1, 1 }, { "dark", A_TONE, 1, 1 }, { "abafado", A_TONE, 1, 1 }, { "suave", A_TONE, 2, 2 }, { "soft", A_TONE, 2, 2 },
        { "brilhante", A_TONE, 4, 4 }, { "bright", A_TONE, 4, 4 }, { "claro", A_TONE, 4, 4 }, { "brilho", A_TONE, 4, 4 }, { "aberto", A_TONE, 5, 5 }, { "open", A_TONE, 5, 5 },
        { "curto", A_SHAPE, 1, 3 }, { "short", A_SHAPE, 1, 3 }, { "staccato", A_SHAPE, 1, 3 }, { "longo", A_SHAPE, 2, 2 }, { "long", A_SHAPE, 2, 2 }, { "sustentado", A_SHAPE, 2, 2 },
        { "percussivo", A_SHAPE, 3, 5 }, { "ataque", A_SHAPE, 3, 5 }, { "rapido", A_SHAPE, 5, 5 }, { "fast", A_SHAPE, 5, 5 },
        { "lento", A_SHAPE, 4, 1 }, { "slow", A_SHAPE, 4, 1 }, { "macio", A_SHAPE, 4, 4 }, { "seco", A_SPACE, 1, 1 }, { "dry", A_SPACE, 1, 1 },
        { "sala", A_SPACE, 2, 2 }, { "room", A_SPACE, 2, 2 }, { "amplo", A_SPACE, 3, 3 }, { "espacial", A_SPACE, 3, 3 }, { "ambiente", A_SPACE, 3, 3 }, { "ambient", A_SPACE, 3, 3 },
        { "grande", A_SPACE, 3, 3 }, { "big", A_SPACE, 3, 3 }, { "catedral", A_SPACE, 4, 4 }, { "epico", A_SPACE, 4, 4 }, { "enorme", A_SPACE, 4, 4 }, { "huge", A_SPACE, 4, 4 },
        { "shimmer", A_SPACE, 5, 5 }, { "cintilante", A_SPACE, 5, 5 },
        { "mono", A_WIDTH, 1, 1 }, { "estreito", A_WIDTH, 2, 2 }, { "narrow", A_WIDTH, 2, 2 }, { "largo", A_WIDTH, 3, 3 }, { "wide", A_WIDTH, 3, 3 }, { "estereo", A_WIDTH, 3, 3 },
        { "stereo", A_WIDTH, 3, 3 }, { "super", A_WIDTH, 4, 4 }, { "supersaw", A_WIDTH, 4, 4 }, { "denso", A_WIDTH, 5, 5 }, { "thick", A_WIDTH, 5, 5 },
        { "respira", A_MOVE, 1, 1 }, { "respirar", A_MOVE, 1, 1 }, { "breathing", A_MOVE, 1, 1 }, { "vibrato", A_MOVE, 2, 2 }, { "ondula", A_MOVE, 3, 3 }, { "movimento", A_MOVE, 3, 3 },
        { "moving", A_MOVE, 3, 3 }, { "evolutivo", A_MOVE, 3, 3 }, { "evolving", A_MOVE, 3, 3 }, { "tremolo", A_MOVE, 4, 4 }, { "deriva", A_MOVE, 5, 5 }, { "drift", A_MOVE, 5, 5 },
        { "quente", A_CHAR, 1, 1 }, { "warm", A_CHAR, 1, 1 }, { "sujo", A_CHAR, 2, 2 }, { "dirty", A_CHAR, 2, 2 }, { "distorcido", A_CHAR, 2, 2 }, { "gritty", A_CHAR, 2, 2 },
        { "lofi", A_CHAR, 3, 3 }, { "crush", A_CHAR, 3, 3 }, { "chorus", A_CHAR, 4, 4 }, { "fase", A_CHAR, 5, 5 }, { "phaser", A_CHAR, 5, 5 },
        { "grave", A_REG, 1, 1 }, { "low", A_REG, 1, 1 }, { "agudo", A_REG, 2, 2 }, { "high", A_REG, 2, 2 }, { "oitava", A_REG, 3, 3 }, { "quinta", A_REG, 4, 4 },
        { "camada", A_LAYER, 5, 5 }, { "layer", A_LAYER, 5, 5 }, { "etereo", A_LAYER, 5, 5 }, { "ethereal", A_LAYER, 5, 5 }, { "granular", A_LAYER, 2, 2 }, { "espectral", A_LAYER, 1, 1 },
        { "sub", A_LAYER, 3, 3 }, { "ruido", A_LAYER, 4, 4 }, { "noise", A_LAYER, 4, 4 },
    };
    Intent in; const std::string t = fold (text);
    std::string cur;
    auto flush = [&] ()
    {
        if (cur.empty()) return;
        for (auto& w : table) if (cur == w.w) { const int o = sust ? w.oSust : w.o; if (! in.opt[w.a]) { in.opt[w.a] = o; in.understood.push_back (cur); } break; }
        cur.clear();
    };
    for (unsigned char c : t) { if (std::isalnum (c)) cur += (char) c; else flush(); }
    flush();
    return in;
}

inline const char* axisName (int a)
{
    static const char* n[gen::A_COUNT] = { "tom", "forma", "espaço", "largura", "movimento", "carácter", "registo", "oscilador", "camada" };
    return n[a];
}

struct Variation { Preset preset; std::string note; };

// `n` variações de `base`: as primeiras aplicam a intenção (mais um eixo extra para variar), as restantes exploram livremente.
// Se o texto não tiver palavras reconhecidas, todas são exploração livre. Resultado determinístico para a mesma semente.
inline std::vector<Variation> variations (const Preset& base, const std::string& text, int n, uint32_t seed, std::string* report = nullptr)
{
    using namespace gen;
    const Kind k = kindOf (base); const Intent in = parseIntent (text, k);
    std::vector<Variation> out; std::set<std::string> seen; seen.insert (base.name);
    std::string rep;
    if (in.any()) { rep = "Entendi: "; for (size_t i = 0; i < in.understood.size(); ++i) rep += (i ? ", " : "") + in.understood[i]; }
    else rep = text.empty() ? "Sem texto: variações livres." : "Nenhuma palavra reconhecida — variações livres.";
    // eixos da intenção que não se aplicam a este preset
    uint32_t forced = 0; std::string ignored;
    for (int a = 0; a < A_COUNT; ++a) if (in.opt[a]) { if (applicable ((Axis) a, base, k)) forced |= (uint32_t) in.opt[a] << (3 * a); else ignored += std::string (ignored.empty() ? "" : ", ") + axisName (a); }
    if (! ignored.empty()) rep += " · não aplicável a este preset: " + ignored;
    for (uint32_t round = 0; (int) out.size() < n && round < 200; ++round)
    {
        uint32_t code = forced;
        const uint32_t extra = candidateCode (base, seed * 7919u + round);           // 2–3 eixos aleatórios
        for (int a = 0; a < A_COUNT; ++a)
        {
            const uint32_t d = (extra >> (3 * a)) & 7u;
            if (! d || (forced >> (3 * a) & 7u)) continue;
            // com intenção: no máximo 1 eixo extra; sem intenção: os que o candidato trouxe
            if (forced) { if (round == 0) continue; if ((code & ~forced) != 0) continue; }
            code |= d << (3 * a);
        }
        if (code == 0) continue;
        Preset v = derive (base, code, 0);
        if (v.values == base.values || seen.count (v.name)) continue;
        seen.insert (v.name); v.qa = "auto"; v.author = "Assistente";
        std::string note; for (int a = 0; a < A_COUNT; ++a) if ((code >> (3 * a)) & 7u) note += std::string (note.empty() ? "" : " · ") + axisName (a);
        out.push_back ({ std::move (v), note });
    }
    if (report) *report = rep;
    return out;
}

// ------------------------------------------------------------------------------------------- macros automáticas
// Atribui às 8 macros de cada preset as ligações PADRÃO (as mesmas etiquetas do painel: BRILHO, CORPO, MOVIM., ESPAÇO, LARGURA, SUJIDADE,
// BALANÇO, DOBRA), mas escolhendo o destino conforme a arquitectura real do som (filtro, motores, sub, LFO, unisson). Só mexe nas macros
// que ainda não têm rotas e nas rotas livres. Unipolares: com o botão a 0 o som fica exactamente como estava.
struct MacroInfo { int index; std::string name; std::string target; };
inline std::vector<MacroInfo> autoMacros (Preset& p)
{
    using namespace gen::detail;
    std::vector<MacroInfo> made;
    auto freeSlot = [&] () { for (int s = 0; s < kNumModSlots; ++s) { const int b = modSlotBase (s); if ((int) p.get (b) == 0 || (int) p.get (b + 1) == 0 || std::fabs (p.get (b + 2)) < 1e-4f) return s; } return -1; };
    const bool f1 = (int) p.get (P_F1_TYPE) != 0, f2 = (int) p.get (P_F2_TYPE) != 0, aOn = p.get (P_A_ON) > 0.5f, bOn = p.get (P_B_ON) > 0.5f;
    const int ta = aOn ? (int) p.get (P_A_TYPE) : -1;
    const bool subOn = p.get (P_SUB_LEVEL) > -59.0f, uni = aOn && (int) p.get (P_A_UNI) > 1 && (ta == 0 || ta == 1);
    bool lfoUsed = false; for (int s = 0; s < kNumModSlots; ++s) { const int b = modSlotBase (s); if ((int) p.get (b) == MS_LFO1 && (int) p.get (b + 1) != 0 && std::fabs (p.get (b + 2)) > 1e-4f) lfoUsed = true; }
    bool used[kNumMacros] = {};
    for (int s = 0; s < kNumModSlots; ++s) { const int b = modSlotBase (s); const int src = (int) p.get (b); if (src >= MS_MACRO1 && src < MS_MACRO1 + kNumMacros && (int) p.get (b + 1) != 0 && std::fabs (p.get (b + 2)) > 1e-4f) used[src - MS_MACRO1] = true; }
    // ESPAÇO precisa de um reverb no AUX: se o AUX está vazio, instala um (envio a 0: não muda o som)
    const bool auxFree = (int) p.get (fxSlotBase (6)) == 0 && (int) p.get (fxSlotBase (7)) == 0 && (int) p.get (fxSlotBase (8)) == 0;
    bool auxReverb = false; for (int k = 6; k < 9; ++k) if ((int) p.get (fxSlotBase (k)) == 6) auxReverb = true;
    struct Cand { const char* name; int dst; float amt; bool ok; };
    const Cand c[kNumMacros] = {
        { "BRILHO",   f1 ? MD_CUT1 : (ta == 1 || ta == 5 ? MD_WTPOS_A : MD_FOLD_A), 0.5f, f1 || ta == 0 || ta == 1 || ta == 5 },
        { "CORPO",    subOn ? MD_SUB : (f1 ? MD_RES1 : MD_UNIDET_A), subOn ? 0.5f : 0.35f, subOn || f1 || uni },
        { "MOVIM.",   MD_LFO1RATE, 0.5f, lfoUsed },
        { "ESPAÇO",   MD_SENDA, 0.6f, auxReverb || auxFree },
        { "LARGURA",  MD_UNIDET_A, 0.5f, uni },
        { "SUJIDADE", MD_DRIVE1, 0.5f, f1 },
        { "BALANÇO",  MD_FILTBAL, 0.5f, f1 && f2 },
        { "DOBRA",    ta == 1 || ta == 5 ? MD_WARP_A : MD_FOLD_A, 0.4f, ta == 0 || ta == 1 || ta == 5 },
    };
    for (int m = 0; m < kNumMacros; ++m)
    {
        if (used[m] || ! c[m].ok) continue;
        const int s1 = freeSlot(); if (s1 < 0) break;
        if (m == 3 && ! auxReverb)
        {
            const int fb = fxSlotBase (6); p.set (fb, 6.0f); p.set (fb + 1, 0.5f); p.set (fb + 2, 0.55f); p.set (fb + 3, 0.45f); p.set (fb + 4, 0.1f); p.set (fb + 5, 0.9f); p.set (fb + 6, 1.0f);
            p.set (P_AUX_RETURN, 0.0f); auxReverb = true;
        }
        putMod (p, s1, MS_MACRO1 + m, c[m].dst, c[m].amt);
        p.set (P_MACRO1 + m, 0.0f);
        made.push_back ({ m + 1, c[m].name, kModDstNames[c[m].dst] });
    }
    return made;
}

// ------------------------------------------------------------------------------------------- Smart Layer Builder
enum LayerMode { LAYER_STACK = 0, LAYER_SPLIT = 1 };

// parceiros sugeridos para `a`: timbres que COMPLETAM o de `a` (centroide e ataque diferentes, mas não opostos ao ponto de ser incoerente)
inline std::vector<search::Hit> suggestPartners (const Preset& a, const Fingerprint& fa, const std::vector<Preset>& list, int topN, int skip, const std::function<bool (const Preset&)>& accept = {})
{
    std::vector<search::Hit> hits; Fingerprint f;
    for (size_t i = 0; i < list.size(); ++i)
    {
        const auto& b = list[i];
        if ((int) i == skip || ! search::fpOf (b, f) || (accept && ! accept (b))) continue;
        if (b.get (P_A_ON) < 0.5f || b.get (P_B_ON) > 0.5f || b.get (P_ARP_ON) > 0.5f) continue;        // só presets de um motor, sem arpejador
        const gen::Kind kb = gen::kindOf (b); if (kb == gen::K_PERC || kb == gen::K_FX) continue;
        const float dCen = std::fabs (f.v[0] - fa.v[0]), dAtt = std::fabs (f.v[2] - fa.v[2]), dLow = std::fabs (f.v[6] - fa.v[6]);
        const float tooFar = std::fabs (f.v[5] - fa.v[5]) > 2.0f ? 1.0f : 0.0f;
        // complementaridade: diferente em registo e ataque, parecido em largura; evita gémeos (fpDistance pequeno)
        const float twin = fpDistance (fa, f) < 0.8f ? 3.0f : 0.0f;
        const float score = -(std::min (dCen, 2.0f) + 0.8f * std::min (dAtt, 1.5f) + 0.6f * std::min (dLow, 1.0f)) + twin + tooFar;
        hits.push_back ({ (int) i, score });
    }
    std::sort (hits.begin(), hits.end(), [] (const search::Hit& x, const search::Hit& y) { return x.dist < y.dist; });
    if ((int) hits.size() > topN) hits.resize ((size_t) topN);
    return hits;
}

// junta `b` (motor A) ao motor B de `a`. STACK: as duas em toda a extensão (níveis −3 dB, blend 0,5). SPLIT: `a` abaixo de `splitKey`, `b` desde `splitKey`.
// Devolve false se `a` já usa o motor B ou `b` não tem motor A ligado. O preset resultante tem de ser calibrado (ganho) por quem chama.
inline bool buildLayer (const Preset& a, const Preset& b, LayerMode mode, int splitKey, Preset& out)
{
    using namespace gen::detail;
    if (a.get (P_B_ON) > 0.5f || b.get (P_A_ON) < 0.5f) return false;
    out = a; out.module.clear();
    // copia o motor A de b para o motor B de a (todos os ids "a_*" → "b_*")
    for (auto& kv : b.values)
    {
        const std::string id = kParams[kv.first].id;
        if (id.size() > 2 && id[0] == 'a' && id[1] == '_') { const int j = paramIndexForId ("b_" + id.substr (2)); if (j >= 0) out.set (j, kv.second); }
    }
    // valores de b que são defeito mas diferem do defeito de a (ex.: a tinha outro valor): repõe o defeito de b
    for (int i = 0; i < P_COUNT; ++i)
    {
        const std::string id = kParams[i].id; if (id.size() <= 2 || id[0] != 'a' || id[1] != '_') continue;
        const int j = paramIndexForId ("b_" + id.substr (2)); if (j < 0) continue;
        bool inB = false; for (auto& kv : b.values) if (kv.first == i) { inB = true; break; }
        if (! inB) out.set (j, kParams[i].def);
    }
    out.sampleB = b.sampleA; out.set (P_B_ON, 1.0f);
    out.set (P_B_LEVEL, b.get (P_A_LEVEL)); out.set (P_BLEND, 0.5f);
    if (mode == LAYER_SPLIT)
    {
        const int k = std::max (24, std::min (100, splitKey));
        out.set (P_A_KEYLO, 0.0f); out.set (P_A_KEYHI, (float) (k - 1)); out.set (P_B_KEYLO, (float) k); out.set (P_B_KEYHI, 127.0f);
    }
    else { out.set (P_A_KEYLO, 0.0f); out.set (P_A_KEYHI, 127.0f); out.set (P_B_KEYLO, 0.0f); out.set (P_B_KEYHI, 127.0f); }
    out.name = a.name + " + " + b.name; out.qa = "auto"; out.author = "Layer Builder"; out.family = a.family.empty() ? familyForCategory (a.category) : a.family; out.subfamily = "Layer";
    out.engine = "hybrid"; out.tags = a.tags; out.tags.push_back (mode == LAYER_SPLIT ? "split" : "layer");
    return true;
}
}} // namespace sk::assist
