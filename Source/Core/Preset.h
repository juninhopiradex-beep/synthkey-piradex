// =============================================================================
//  SYNTHKEY by Piradex — preset (.skpreset): JSON com os parâmetros por id
// =============================================================================
#pragma once
#include "MiniJson.h"
#include "ParamTable.h"
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

inline std::string presetToJson (const Preset& p)
{
    json::Value o = json::Value::makeObject();
    o.set ("format", json::Value::makeNumber (1));
    o.set ("name", json::Value::makeString (p.name)); o.set ("style", json::Value::makeString (p.style));
    o.set ("author", json::Value::makeString (p.author)); o.set ("pack", json::Value::makeString (p.pack));
    o.set ("category", json::Value::makeString (p.category));
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
    p.category = v.getString ("category"); p.sampleA = v.getString ("sampleA"); p.sampleB = v.getString ("sampleB");
    if (auto* t = v.find ("tags")) if (t->isArray()) for (auto& x : t->arr) if (x.isString()) p.tags.push_back (x.str);
    if (auto* pr = v.find ("params")) if (pr->isObject())
        for (auto& kv : pr->obj) { const int idx = paramIndexForId (kv.first); if (idx >= 0 && kv.second.isNumber()) p.values.emplace_back (idx, (float) kv.second.num); }
    if (p.name.empty()) return false;
    out = std::move (p); return true;
}
} // namespace sk
