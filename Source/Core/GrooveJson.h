// =============================================================================
//  SYNTHKEY by Piradex — grooves em JSON (.skgroove), C++ puro (usado pelo plugin, pelos packs e pelas ferramentas)
// =============================================================================
#pragma once
#include "Grooves.h"
#include "MiniJson.h"

namespace sk { namespace groovejson
{
inline void sanitize (Groove& g)
{
    g.bars = std::max (1, std::min (16, g.bars)); g.bpm = std::max (40, std::min (240, g.bpm)); g.swing = std::max (0.0f, std::min (0.5f, g.swing));
    const int total = g.totalSteps();
    g.notes.erase (std::remove_if (g.notes.begin(), g.notes.end(), [total] (const GNote& n) { return n.step < 0 || n.step >= total || n.note < 0 || n.note > 127; }), g.notes.end());
    for (auto& n : g.notes) { n.len = std::max (1, std::min (total - n.step, n.len)); n.vel = std::max (1, std::min (127, n.vel)); }
    std::stable_sort (g.notes.begin(), g.notes.end(), [] (const GNote& a, const GNote& b) { return a.step < b.step; });
}

inline std::string toJson (const Groove& g)
{
    json::Value o = json::Value::makeObject();
    o.set ("format", json::Value::makeNumber (1)); o.set ("name", json::Value::makeString (g.name)); o.set ("style", json::Value::makeString (g.style));
    o.set ("kind", json::Value::makeString (g.kind)); o.set ("bpm", json::Value::makeNumber (g.bpm)); o.set ("bars", json::Value::makeNumber (g.bars));
    o.set ("swing", json::Value::makeNumber (g.swing));
    json::Value ns = json::Value::makeArray();
    for (auto& n : g.notes)
    {
        json::Value a = json::Value::makeArray();
        for (double x : { (double) n.step, (double) n.note, (double) n.len, (double) n.vel }) a.arr.push_back (json::Value::makeNumber (x));
        ns.arr.push_back (std::move (a));
    }
    o.set ("notes", std::move (ns));
    return json::stringify (o, false);
}

inline bool fromJson (const std::string& text, Groove& out)
{
    json::Value v; if (! json::parse (text, v) || ! v.isObject()) return false;
    Groove g; g.name = v.getString ("name"); g.style = v.getString ("style", "trap"); g.kind = v.getString ("kind", "Melodia");
    g.bpm = (int) v.getNumber ("bpm", 120); g.bars = (int) v.getNumber ("bars", 2); g.swing = (float) v.getNumber ("swing", 0);
    if (auto* ns = v.find ("notes")) if (ns->isArray())
        for (auto& a : ns->arr) if (a.isArray() && a.arr.size() >= 4)
        { GNote n; n.step = (int) a.arr[0].num; n.note = (int) a.arr[1].num; n.len = (int) a.arr[2].num; n.vel = (int) a.arr[3].num; g.notes.push_back (n); }
    if (g.name.empty()) return false;
    sanitize (g); out = std::move (g); return true;
}
}}
