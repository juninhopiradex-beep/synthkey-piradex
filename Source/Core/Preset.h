// =============================================================================
//  SYNTHKEY by Piradex — preset (.skpreset): JSON com os parâmetros por id
// =============================================================================
#pragma once
#include "MiniJson.h"
#include "ParamTable.h"
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

namespace sk
{
struct Preset
{
    std::string name, style, author = "Piradex", pack = "nucleo", category;
    std::vector<std::string> tags;
    std::string sampleA, sampleB;                 // ids de instrumentos ("trap.808long"); vazio = nenhum
    // formato 2 — metadados de biblioteca (vazios em presets antigos; ver familyForCategory)
    std::string family, subfamily, engine, genre, role, qa;   // qa: "" | "auto" (validação técnica automática) | "ouvido" (validado ao ouvido)
    int version = 1;
    std::string module;                           // "" = preset completo; "osc" | "filter" | "env" | "mod" | "fx" | "arp" = só mexe nesse módulo
    std::vector<std::pair<int, float>> values;    // (índice do parâmetro, valor real) — só os que diferem do defeito

    void set (int idx, float v)
    {
        for (auto& kv : values) if (kv.first == idx) { kv.second = v; return; }
        values.emplace_back (idx, v);
    }
    float get (int idx) const { for (auto& kv : values) if (kv.first == idx) return kv.second; return kParams[idx].def; }
};

inline int paramIndexForId (const std::string& id)
{
    static std::vector<std::pair<std::string, int>> map;
    if (map.empty()) for (int i = 0; i < P_COUNT; ++i) map.emplace_back (kParams[i].id, i);
    for (auto& kv : map) if (kv.first == id) return kv.second;
    return -1;
}

// a que módulo pertence cada parâmetro (presets de módulo só escrevem estes)
inline bool moduleOwns (const std::string& module, const std::string& id)
{
    auto pre = [&] (const char* p) { return id.rfind (p, 0) == 0; };
    if (module == "osc")    return pre ("a_") || pre ("b_") || id == "blend" || pre ("sub_") || pre ("noise_");
    if (module == "filter") return pre ("f1_") || pre ("f2_") || pre ("filt_");
    if (module == "env")    return pre ("amp_") || pre ("env2_") || pre ("env3_");
    if (module == "mod")    return (pre ("mod") && id.size() > 3 && id[3] >= '0' && id[3] <= '9' && id.size() > 5 && id.find ("_", 3) != std::string::npos
                                    && std::atoi (id.c_str() + 3) >= 1 && std::atoi (id.c_str() + 3) <= 8) || pre ("lfo1_") || pre ("lfo2_");
    if (module == "fx")     return pre ("fxa") || pre ("fxb") || pre ("fxx") || pre ("send_") || pre ("aux_");
    if (module == "arp")    return pre ("arp_");
    return false;
}
inline bool moduleOwnsIndex (const std::string& module, int idx) { return idx >= 0 && idx < P_COUNT && moduleOwns (module, kParams[idx].id); }

// aplica um preset a um vetor de valores reais: completo = tudo; de módulo = só os parâmetros desse módulo (os outros ficam como estão)
inline void applyPresetToParams (const Preset& p, float* cur)
{
    for (int i = 0; i < P_COUNT; ++i) if (p.module.empty() || moduleOwnsIndex (p.module, i)) cur[i] = p.get (i);
}

// morph entre dois estados de parâmetros (valores reais): contínuos interpolam (os logarítmicos geometricamente), discretos mudam a meio
inline void morphParams (const float* a, const float* b, float t, float* out)
{
    t = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
    for (int i = 0; i < P_COUNT; ++i)
    {
        const auto& pi = kParams[i];
        if (pi.kind == PK_FLOAT) out[i] = a[i] + (b[i] - a[i]) * t;
        else if (pi.kind == PK_LOG && a[i] > 0 && b[i] > 0) out[i] = a[i] * std::pow (b[i] / a[i], t);
        else out[i] = t < 0.5f ? a[i] : b[i];
    }
}

// formato 1 -> família (a categoria antiga mapeia para a família da biblioteca nova)
inline std::string familyForCategory (const std::string& c)
{
    static const std::pair<const char*, const char*> m[] = {
        {"Baixo","Bass"},{"Lead","Leads"},{"Pad","Pads"},{"Teclas","Keys"},{"Pluck","Plucks"},{"Vozes","Vocals"},{"Bateria","Drums"},
        {"Percussão","Percussion"},{"Arp","Arpeggios"},{"FX","SFX"},{"Mallets","Mallets"},{"Melodia","Leads"},{"Sintetizador","Leads"},{"Efeitos","SFX"} };
    for (auto& kv : m) if (c == kv.first) return kv.second;
    return c;
}

inline std::string presetToJson (const Preset& p)
{
    json::Value o = json::Value::makeObject();
    o.set ("format", json::Value::makeNumber (2));
    o.set ("name", json::Value::makeString (p.name)); o.set ("style", json::Value::makeString (p.style));
    o.set ("author", json::Value::makeString (p.author)); o.set ("pack", json::Value::makeString (p.pack));
    o.set ("category", json::Value::makeString (p.category));
    o.set ("family", json::Value::makeString (p.family.empty() ? familyForCategory (p.category) : p.family));
    o.set ("subfamily", json::Value::makeString (p.subfamily)); o.set ("engine", json::Value::makeString (p.engine));
    o.set ("genre", json::Value::makeString (p.genre)); o.set ("role", json::Value::makeString (p.role));
    o.set ("module", json::Value::makeString (p.module)); o.set ("qa", json::Value::makeString (p.qa)); o.set ("version", json::Value::makeNumber (p.version));
    json::Value tg = json::Value::makeArray(); for (auto& t : p.tags) tg.arr.push_back (json::Value::makeString (t)); o.set ("tags", tg);
    o.set ("sampleA", json::Value::makeString (p.sampleA)); o.set ("sampleB", json::Value::makeString (p.sampleB));
    json::Value pr = json::Value::makeObject();
    for (auto& kv : p.values) pr.set (kParams[kv.first].id, json::Value::makeNumber (kv.second));
    o.set ("params", pr);
    return json::stringify (o, true);
}

inline bool presetFromJson (const std::string& text, Preset& out)
{
    json::Value v; if (! json::parse (text, v) || ! v.isObject()) return false;
    Preset p;
    p.name = v.getString ("name"); p.style = v.getString ("style"); p.author = v.getString ("author", "Piradex"); p.pack = v.getString ("pack", "nucleo");
    p.category = v.getString ("category");
    p.family = v.getString ("family", familyForCategory (p.category)); p.subfamily = v.getString ("subfamily"); p.engine = v.getString ("engine");
    p.genre = v.getString ("genre"); p.role = v.getString ("role"); p.qa = v.getString ("qa"); p.module = v.getString ("module");
    p.version = (int) v.getNumber ("version", 1);
    p.sampleA = v.getString ("sampleA"); p.sampleB = v.getString ("sampleB");
    if (auto* t = v.find ("tags")) if (t->isArray()) for (auto& x : t->arr) if (x.isString()) p.tags.push_back (x.str);
    if (auto* pr = v.find ("params")) if (pr->isObject())
        for (auto& kv : pr->obj) { const int idx = paramIndexForId (kv.first); if (idx >= 0 && kv.second.isNumber()) p.values.emplace_back (idx, (float) kv.second.num); }
    if (p.name.empty()) return false;
    out = std::move (p); return true;
}
} // namespace sk
