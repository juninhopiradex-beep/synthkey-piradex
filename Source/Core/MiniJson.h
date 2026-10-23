// =============================================================================
//  SYNTHKEY by Piradex — JSON mínimo (leitura e escrita), C++17 sem dependências.
//  Usado nos presets (.skpreset) e no manifesto dos packs (manifest.json).
// =============================================================================
#pragma once
#include <cstdio>
#include <cstdlib>
#include <string>
#include <utility>
#include <vector>

namespace sk { namespace json
{
struct Value
{
    enum Type { Null, Bool, Number, String, Array, Object } type = Null;
    bool b = false; double num = 0; std::string str;
    std::vector<Value> arr; std::vector<std::pair<std::string, Value>> obj;

    bool isObject() const { return type == Object; }
    bool isArray() const { return type == Array; }
    bool isString() const { return type == String; }
    bool isNumber() const { return type == Number; }
    const Value* find (const std::string& k) const { for (auto& kv : obj) if (kv.first == k) return &kv.second; return nullptr; }
    std::string getString (const std::string& k, const std::string& d = "") const { auto* v = find (k); return v && v->type == String ? v->str : d; }
    double getNumber (const std::string& k, double d = 0) const { auto* v = find (k); return v && v->type == Number ? v->num : (v && v->type == Bool ? (v->b ? 1.0 : 0.0) : d); }
    bool getBool (const std::string& k, bool d = false) const { auto* v = find (k); return v && v->type == Bool ? v->b : (v && v->type == Number ? v->num != 0 : d); }
    static Value makeString (const std::string& s) { Value v; v.type = String; v.str = s; return v; }
    static Value makeNumber (double n) { Value v; v.type = Number; v.num = n; return v; }
    static Value makeObject() { Value v; v.type = Object; return v; }
    static Value makeArray() { Value v; v.type = Array; return v; }
    void set (const std::string& k, Value v) { for (auto& kv : obj) if (kv.first == k) { kv.second = std::move (v); return; } obj.emplace_back (k, std::move (v)); }
};

namespace detail
{
struct Parser
{
    const std::string& s; size_t i = 0; bool ok = true;
    explicit Parser (const std::string& t) : s (t) {}
    void ws() { while (i < s.size() && (s[i] == ' ' || s[i] == '\n' || s[i] == '\r' || s[i] == '\t')) ++i; }
    bool lit (const char* w) { size_t n = std::char_traits<char>::length (w); if (s.compare (i, n, w) == 0) { i += n; return true; } return false; }
    static void putUtf8 (std::string& o, unsigned cp)
    {
        if (cp < 0x80) o += (char) cp; else if (cp < 0x800) { o += (char) (0xC0 | (cp >> 6)); o += (char) (0x80 | (cp & 63)); }
        else { o += (char) (0xE0 | (cp >> 12)); o += (char) (0x80 | ((cp >> 6) & 63)); o += (char) (0x80 | (cp & 63)); }
    }
    std::string str()
    {
        std::string o; ++i;   // abre aspas
        while (i < s.size() && s[i] != '"')
        {
            if (s[i] == '\\' && i + 1 < s.size())
            {
                ++i; const char c = s[i++];
                switch (c)
                {
                    case 'n': o += '\n'; break; case 't': o += '\t'; break; case 'r': o += '\r'; break; case 'b': o += '\b'; break; case 'f': o += '\f'; break;
                    case 'u': { unsigned cp = 0; for (int k = 0; k < 4 && i < s.size(); ++k) { char h = s[i++]; cp = cp * 16 + (unsigned) (h >= '0' && h <= '9' ? h - '0' : (h | 32) - 'a' + 10); } putUtf8 (o, cp); break; }
                    default: o += c;
                }
            }
            else o += s[i++];
        }
        if (i >= s.size()) ok = false; else ++i;
        return o;
    }
    Value value (int depth = 0)
    {
        Value v; ws(); if (i >= s.size() || depth > 64) { ok = false; return v; }
        const char c = s[i];
        if (c == '{')
        {
            v.type = Value::Object; ++i; ws();
            if (i < s.size() && s[i] == '}') { ++i; return v; }
            while (ok)
            {
                ws(); if (i >= s.size() || s[i] != '"') { ok = false; break; }
                std::string k = str(); ws(); if (i >= s.size() || s[i] != ':') { ok = false; break; } ++i;
                v.obj.emplace_back (std::move (k), value (depth + 1)); ws();
                if (i < s.size() && s[i] == ',') { ++i; continue; }
                if (i < s.size() && s[i] == '}') { ++i; break; }
                ok = false;
            }
        }
        else if (c == '[')
        {
            v.type = Value::Array; ++i; ws();
            if (i < s.size() && s[i] == ']') { ++i; return v; }
            while (ok)
            {
                v.arr.push_back (value (depth + 1)); ws();
                if (i < s.size() && s[i] == ',') { ++i; continue; }
                if (i < s.size() && s[i] == ']') { ++i; break; }
                ok = false;
            }
        }
        else if (c == '"') { v.type = Value::String; v.str = str(); }
        else if (lit ("true")) { v.type = Value::Bool; v.b = true; }
        else if (lit ("false")) { v.type = Value::Bool; v.b = false; }
        else if (lit ("null")) { v.type = Value::Null; }
        else
        {
            char* end = nullptr; const double n = std::strtod (s.c_str() + i, &end);
            if (end == s.c_str() + i) { ok = false; return v; }
            v.type = Value::Number; v.num = n; i = (size_t) (end - s.c_str());
        }
        return v;
    }
};
inline void escape (std::string& o, const std::string& s)
{
    o += '"';
    for (unsigned char c : s)
    {
        switch (c)
        {
            case '"': o += "\\\""; break; case '\\': o += "\\\\"; break; case '\n': o += "\\n"; break; case '\t': o += "\\t"; break; case '\r': o += "\\r"; break;
            default: if (c < 0x20) { char b[8]; std::snprintf (b, sizeof b, "\\u%04x", c); o += b; } else o += (char) c;
        }
    }
    o += '"';
}
inline void write (std::string& o, const Value& v, int ind, bool pretty)
{
    auto nl = [&] (int n) { if (pretty) { o += '\n'; o.append ((size_t) n * 2, ' '); } };
    switch (v.type)
    {
        case Value::Null: o += "null"; break;
        case Value::Bool: o += v.b ? "true" : "false"; break;
        case Value::Number: { char b[40]; if (v.num == (double) (long long) v.num && std::abs (v.num) < 1e15) std::snprintf (b, sizeof b, "%lld", (long long) v.num); else std::snprintf (b, sizeof b, "%.9g", v.num); o += b; break; }
        case Value::String: escape (o, v.str); break;
        case Value::Array:
            o += '['; for (size_t k = 0; k < v.arr.size(); ++k) { if (k) o += ','; nl (ind + 1); write (o, v.arr[k], ind + 1, pretty); } if (! v.arr.empty()) nl (ind); o += ']'; break;
        case Value::Object:
            o += '{'; for (size_t k = 0; k < v.obj.size(); ++k) { if (k) o += ','; nl (ind + 1); escape (o, v.obj[k].first); o += pretty ? ": " : ":"; write (o, v.obj[k].second, ind + 1, pretty); } if (! v.obj.empty()) nl (ind); o += '}'; break;
    }
}
} // namespace detail

inline bool parse (const std::string& text, Value& out)
{
    detail::Parser p (text); out = p.value(); p.ws();
    return p.ok && p.i == text.size();
}
inline std::string stringify (const Value& v, bool pretty = true) { std::string o; detail::write (o, v, 0, pretty); if (pretty) o += '\n'; return o; }
}} // namespace sk::json
