// =============================================================================
//  SYNTHKEY by Piradex — "Descrever o som": texto → preset
//
//  Gerador por regras (sem rede, sem IA externa): lê palavras em português ou inglês
//  ("baixo escuro e agressivo", "pad quente e largo", "bright short pluck", "808 drill")
//  e monta um patch a partir de modelos por tipo de som + traços (brilho, agressividade,
//  largura, duração, movimento, sujidade, espaço, ataque...). Cada nova geração com o mesmo
//  texto varia ligeiramente (semente). O nível é calibrado por render (calibrate()).
// =============================================================================
#pragma once
#include "Engine.h"
#include "PresetBuilder.h"
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace sk { namespace soundgen
{
enum Kind { K_NONE = -1, K_BASS, K_808, K_LEAD, K_PAD, K_PLUCK, K_KEYS, K_BELL, K_STAB, K_STRINGS, K_ORGAN, K_FX, K_VOCAL, K_ARP, K_LOG, K_COUNT };
enum Trait { T_BRIGHT, T_AGGR, T_WIDE, T_LEN, T_MOVE, T_DIRTY, T_SPACE, T_ATTACK, T_FAT, T_WARM, T_RHYTHM, T_GLIDE, T_METAL, T_VIBRATO, T_COUNT };

struct Traits { int kind = K_NONE; float t[T_COUNT] = {}; bool mono = false; bool arp = false; std::string genre; int hits = 0; };

// minúsculas, sem acentos (UTF-8 → ASCII), só letras/dígitos/&/#
inline std::string normalize (const std::string& in)
{
    std::string o; o.reserve (in.size());
    for (size_t i = 0; i < in.size(); ++i)
    {
        const unsigned char c = (unsigned char) in[i];
        if (c == 0xC3 && i + 1 < in.size())
        {
            const unsigned char d = (unsigned char) in[++i]; const unsigned char e = d >= 0x80 && d < 0xA0 ? (unsigned char) (d + 0x20) : d;   // maiúsculas → minúsculas
            if (e >= 0xA0 && e <= 0xA5) o += 'a'; else if (e == 0xA7) o += 'c'; else if (e >= 0xA8 && e <= 0xAB) o += 'e'; else if (e >= 0xAC && e <= 0xAF) o += 'i';
            else if (e >= 0xB2 && e <= 0xB6) o += 'o'; else if (e >= 0xB9 && e <= 0xBC) o += 'u'; else o += ' ';
        }
        else if (c >= 'A' && c <= 'Z') o += (char) (c + 32);
        else if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '&' || c == '#') o += (char) c;
        else o += ' ';
    }
    return o;
}

inline std::vector<std::string> tokens (const std::string& norm)
{
    std::vector<std::string> v; std::string cur;
    for (char c : norm) { if (c == ' ') { if (! cur.empty()) { v.push_back (cur); cur.clear(); } } else cur += c; }
    if (! cur.empty()) v.push_back (cur);
    return v;
}

inline bool stemMatch (const std::string& tok, const char* stem)
{
    const std::string s (stem);
    if (s.size() <= 3) return tok == s || tok == s + "s";
    return tok.compare (0, s.size(), s) == 0 && tok.size() <= s.size() + 5;
}

struct KindLex { const char* stem; int kind; };
struct TraitLex { const char* stem; int trait; float v; };

inline const std::vector<KindLex>& kindLex()
{
    static const std::vector<KindLex> L = {
        { "baix", K_BASS }, { "bass", K_BASS }, { "sub", K_808 }, { "808", K_808 }, { "kick", K_808 },
        { "lead", K_LEAD }, { "solo", K_LEAD }, { "melodi", K_LEAD }, { "synth", K_LEAD },
        { "pad", K_PAD }, { "atmosf", K_PAD }, { "ambient", K_PAD }, { "drone", K_PAD }, { "textura", K_PAD }, { "texture", K_PAD },
        { "pluck", K_PLUCK }, { "pizz", K_PLUCK }, { "dedilh", K_PLUCK }, { "harp", K_PLUCK }, { "marimba", K_PLUCK }, { "koto", K_PLUCK },
        { "piano", K_KEYS }, { "key", K_KEYS }, { "tecla", K_KEYS }, { "ep", K_KEYS }, { "rhodes", K_KEYS }, { "wurli", K_KEYS }, { "clav", K_KEYS },
        { "bell", K_BELL }, { "sino", K_BELL }, { "glock", K_BELL }, { "kalimba", K_BELL }, { "carrilh", K_BELL }, { "chime", K_BELL },
        { "stab", K_STAB }, { "brass", K_STAB }, { "metais", K_STAB }, { "trompet", K_STAB }, { "trumpet", K_STAB }, { "sopro", K_STAB }, { "horn", K_STAB },
        { "corda", K_STRINGS }, { "string", K_STRINGS }, { "violin", K_STRINGS }, { "orquest", K_STRINGS }, { "orchest", K_STRINGS }, { "ensemble", K_STRINGS },
        { "organ", K_ORGAN }, { "orgao", K_ORGAN }, { "gospel", K_ORGAN },
        { "fx", K_FX }, { "riser", K_FX }, { "sirene", K_FX }, { "siren", K_FX }, { "laser", K_FX }, { "efeito", K_FX }, { "effect", K_FX }, { "sweep", K_FX }, { "impact", K_FX }, { "transicao", K_FX },
        { "vox", K_VOCAL }, { "vocal", K_VOCAL }, { "coro", K_VOCAL }, { "choir", K_VOCAL }, { "voz", K_VOCAL }, { "vogal", K_VOCAL }, { "vowel", K_VOCAL }, { "formant", K_VOCAL },
        { "arp", K_ARP }, { "arpej", K_ARP }, { "sequenc", K_ARP },
        { "logdrum", K_LOG }, { "log", K_LOG } };
    return L;
}

inline const std::vector<TraitLex>& traitLex()
{
    static const std::vector<TraitLex> L = {
        { "escur", T_BRIGHT, -0.8f }, { "dark", T_BRIGHT, -0.8f }, { "sombri", T_BRIGHT, -0.9f }, { "obscur", T_BRIGHT, -0.8f }, { "abafad", T_BRIGHT, -0.7f }, { "muffl", T_BRIGHT, -0.7f }, { "opac", T_BRIGHT, -0.5f },
        { "brilh", T_BRIGHT, 0.8f }, { "bright", T_BRIGHT, 0.8f }, { "claro", T_BRIGHT, 0.5f }, { "clara", T_BRIGHT, 0.5f }, { "crisp", T_BRIGHT, 0.7f }, { "cristalin", T_BRIGHT, 0.8f }, { "agud", T_BRIGHT, 0.5f }, { "airy", T_BRIGHT, 0.6f },
        { "vidro", T_BRIGHT, 0.6f }, { "glass", T_BRIGHT, 0.6f }, { "cristal", T_BRIGHT, 0.6f },
        { "agress", T_AGGR, 0.8f }, { "aggress", T_AGGR, 0.8f }, { "violent", T_AGGR, 0.9f }, { "duro", T_AGGR, 0.6f }, { "hard", T_AGGR, 0.6f }, { "feroz", T_AGGR, 0.8f }, { "forte", T_AGGR, 0.5f }, { "intens", T_AGGR, 0.6f }, { "brutal", T_AGGR, 1.0f }, { "screamy", T_AGGR, 0.8f }, { "gritty", T_AGGR, 0.6f },
        { "suave", T_AGGR, -0.7f }, { "soft", T_AGGR, -0.7f }, { "macio", T_AGGR, -0.7f }, { "gentil", T_AGGR, -0.6f }, { "calm", T_AGGR, -0.6f }, { "relax", T_AGGR, -0.6f }, { "delicad", T_AGGR, -0.7f }, { "smooth", T_AGGR, -0.6f }, { "mellow", T_AGGR, -0.6f },
        { "larg", T_WIDE, 0.8f }, { "wide", T_WIDE, 0.8f }, { "espacos", T_WIDE, 0.5f }, { "amplo", T_WIDE, 0.7f }, { "stereo", T_WIDE, 0.6f }, { "estereo", T_WIDE, 0.6f }, { "gigante", T_WIDE, 0.8f }, { "huge", T_WIDE, 0.8f }, { "enorme", T_WIDE, 0.8f }, { "lush", T_WIDE, 0.7f }, { "encorpad", T_WIDE, 0.5f },
        { "estreit", T_WIDE, -0.8f }, { "narrow", T_WIDE, -0.8f }, { "mono", T_WIDE, -0.9f }, { "centrad", T_WIDE, -0.7f },
        { "curt", T_LEN, -0.8f }, { "short", T_LEN, -0.8f }, { "seco", T_LEN, -0.5f }, { "staccato", T_LEN, -0.9f }, { "rapid", T_LEN, -0.3f }, { "breve", T_LEN, -0.7f }, { "tight", T_LEN, -0.6f }, { "punchy", T_LEN, -0.4f },
        { "long", T_LEN, 0.8f }, { "sustent", T_LEN, 0.7f }, { "infinit", T_LEN, 1.0f }, { "eterno", T_LEN, 1.0f }, { "prolong", T_LEN, 0.8f }, { "ressoa", T_LEN, 0.5f },
        { "evolu", T_MOVE, 0.8f }, { "evolv", T_MOVE, 0.8f }, { "moviment", T_MOVE, 0.7f }, { "motion", T_MOVE, 0.7f }, { "movi", T_MOVE, 0.6f }, { "animad", T_MOVE, 0.6f }, { "vivo", T_MOVE, 0.5f }, { "alive", T_MOVE, 0.5f }, { "wobbl", T_MOVE, 0.9f }, { "ondul", T_MOVE, 0.7f }, { "morph", T_MOVE, 0.8f }, { "shimmer", T_MOVE, 0.6f },
        { "estatic", T_MOVE, -0.7f }, { "static", T_MOVE, -0.7f }, { "estavel", T_MOVE, -0.5f }, { "plan", T_MOVE, -0.4f },
        { "suj", T_DIRTY, 0.8f }, { "dirty", T_DIRTY, 0.8f }, { "lofi", T_DIRTY, 0.7f }, { "lo-fi", T_DIRTY, 0.7f }, { "distor", T_DIRTY, 0.9f }, { "saturad", T_DIRTY, 0.6f }, { "crunch", T_DIRTY, 0.7f }, { "granul", T_DIRTY, 0.6f }, { "degrad", T_DIRTY, 0.6f }, { "cassete", T_DIRTY, 0.5f }, { "tape", T_DIRTY, 0.5f }, { "rough", T_DIRTY, 0.6f }, { "aspero", T_DIRTY, 0.6f }, { "fuzz", T_DIRTY, 0.9f },
        { "limp", T_DIRTY, -0.6f }, { "clean", T_DIRTY, -0.7f }, { "puro", T_DIRTY, -0.5f }, { "pristin", T_DIRTY, -0.8f },
        { "espac", T_SPACE, 0.7f }, { "space", T_SPACE, 0.7f }, { "spacey", T_SPACE, 0.8f }, { "reverb", T_SPACE, 0.7f }, { "eco", T_SPACE, 0.6f }, { "echo", T_SPACE, 0.6f }, { "dream", T_SPACE, 0.7f }, { "sonhad", T_SPACE, 0.7f }, { "etere", T_SPACE, 0.8f }, { "ethereal", T_SPACE, 0.8f }, { "cinemat", T_SPACE, 0.7f }, { "cathedral", T_SPACE, 1.0f }, { "catedral", T_SPACE, 1.0f }, { "ambient", T_SPACE, 0.4f }, { "distant", T_SPACE, 0.7f },
        { "seco", T_SPACE, -0.6f }, { "dry", T_SPACE, -0.7f }, { "intim", T_SPACE, -0.5f }, { "proxim", T_SPACE, -0.4f },
        { "lent", T_ATTACK, 0.8f }, { "slow", T_ATTACK, 0.8f }, { "swell", T_ATTACK, 0.9f }, { "fade", T_ATTACK, 0.7f }, { "gradual", T_ATTACK, 0.7f }, { "crescen", T_ATTACK, 0.8f }, { "suavement", T_ATTACK, 0.4f },
        { "snappy", T_ATTACK, -0.8f }, { "percuss", T_ATTACK, -0.9f }, { "ataque", T_ATTACK, -0.2f }, { "imediat", T_ATTACK, -0.8f }, { "instant", T_ATTACK, -0.8f },
        { "gord", T_FAT, 0.8f }, { "fat", T_FAT, 0.8f }, { "gross", T_FAT, 0.8f }, { "thick", T_FAT, 0.8f }, { "pesad", T_FAT, 0.8f }, { "heavy", T_FAT, 0.8f }, { "cheio", T_FAT, 0.7f }, { "full", T_FAT, 0.6f }, { "poderos", T_FAT, 0.8f }, { "massiv", T_FAT, 0.9f }, { "profund", T_FAT, 0.7f }, { "deep", T_FAT, 0.7f },
        { "fino", T_FAT, -0.7f }, { "thin", T_FAT, -0.7f }, { "leve", T_FAT, -0.5f }, { "light", T_FAT, -0.5f }, { "magr", T_FAT, -0.7f },
        { "quent", T_WARM, 0.8f }, { "warm", T_WARM, 0.8f }, { "analog", T_WARM, 0.6f }, { "vintage", T_WARM, 0.7f }, { "retro", T_WARM, 0.7f }, { "aconcheg", T_WARM, 0.8f }, { "cozy", T_WARM, 0.8f }, { "organic", T_WARM, 0.5f }, { "madeira", T_WARM, 0.5f }, { "natural", T_WARM, 0.4f }, { "sensual", T_WARM, 0.6f },
        { "frio", T_WARM, -0.7f }, { "cold", T_WARM, -0.7f }, { "digital", T_WARM, -0.6f }, { "sintetic", T_WARM, -0.4f }, { "robotic", T_WARM, -0.6f },
        { "ritmic", T_RHYTHM, 0.8f }, { "rhythmic", T_RHYTHM, 0.8f }, { "pulsan", T_RHYTHM, 0.9f }, { "pulse", T_RHYTHM, 0.8f }, { "gate", T_RHYTHM, 0.9f }, { "chop", T_RHYTHM, 0.7f }, { "groov", T_RHYTHM, 0.7f }, { "sincopad", T_RHYTHM, 0.8f }, { "bounce", T_RHYTHM, 0.8f }, { "trance", T_RHYTHM, 0.9f },
        { "glide", T_GLIDE, 0.8f }, { "slide", T_GLIDE, 0.8f }, { "deslizan", T_GLIDE, 0.8f }, { "portament", T_GLIDE, 0.8f }, { "escorreg", T_GLIDE, 0.8f }, { "sliding", T_GLIDE, 0.9f },
        { "metalic", T_METAL, 0.8f }, { "metallic", T_METAL, 0.8f }, { "inharmon", T_METAL, 0.8f }, { "sino", T_METAL, 0.3f }, { "fm", T_METAL, 0.7f }, { "ferro", T_METAL, 0.6f }, { "industrial", T_METAL, 0.9f },
        { "vibrat", T_VIBRATO, 0.8f }, { "tremul", T_VIBRATO, 0.7f }, { "tremolo", T_VIBRATO, 0.5f } };
    return L;
}

// géneros / estilos: empurram traços e escolhem um tipo por defeito
inline void applyGenre (Traits& tr, const std::string& tok, float w)
{
    auto add = [&] (int t, float v) { tr.t[t] += v * w; };
    auto defKind = [&] (int k) { if (tr.kind == K_NONE) tr.kind = k; };
    if (tok == "trap")                 { add (T_AGGR, 0.3f); defKind (K_808); tr.genre = "trap"; ++tr.hits; }
    else if (tok == "drill")           { add (T_BRIGHT, -0.4f); add (T_GLIDE, 0.8f); add (T_FAT, 0.5f); defKind (K_808); tr.genre = "drill"; ++tr.hits; }
    else if (tok == "amapiano" || tok == "piano3") { add (T_SPACE, 0.2f); add (T_WARM, 0.3f); defKind (K_LOG); tr.genre = "amapiano"; ++tr.hits; }
    else if (tok == "afrobeat" || tok == "afrobeats" || tok == "afro") { add (T_BRIGHT, 0.2f); add (T_WARM, 0.3f); defKind (K_PLUCK); tr.genre = "afrobeat"; ++tr.hits; }
    else if (tok == "kuduro")          { add (T_AGGR, 0.5f); add (T_BRIGHT, 0.3f); add (T_RHYTHM, 0.3f); defKind (K_LEAD); tr.genre = "kuduro"; ++tr.hits; }
    else if (tok == "kizomba" || tok == "zouk" || tok == "semba") { add (T_WARM, 0.5f); add (T_AGGR, -0.4f); add (T_WIDE, 0.3f); defKind (K_KEYS); tr.genre = "kizomba"; ++tr.hits; }
    else if (tok == "rnb" || tok == "r&b" || tok == "soul") { add (T_WARM, 0.6f); add (T_SPACE, 0.3f); add (T_AGGR, -0.3f); defKind (K_KEYS); tr.genre = "rnb"; ++tr.hits; }
    else if (tok == "house" || tok == "techno" || tok == "edm" || tok == "dance") { add (T_BRIGHT, 0.3f); add (T_RHYTHM, 0.3f); defKind (K_STAB); tr.genre = "dance"; ++tr.hits; }
    else if (tok == "synthwave" || tok == "80s" || tok == "oitentas") { add (T_WARM, 0.5f); add (T_WIDE, 0.6f); add (T_SPACE, 0.4f); defKind (K_LEAD); tr.genre = "synthwave"; ++tr.hits; }
    else if (tok == "lofi")            { add (T_DIRTY, 0.5f); add (T_WARM, 0.5f); add (T_SPACE, 0.3f); defKind (K_KEYS); tr.genre = "lofi"; ++tr.hits; }
    else if (tok == "cinematic" || tok == "cinematico" || tok == "epic" || tok == "epico") { add (T_SPACE, 0.6f); add (T_WIDE, 0.6f); add (T_LEN, 0.4f); defKind (K_PAD); tr.genre = "cinematic"; ++tr.hits; }
}

inline Traits parse (const std::string& text)
{
    Traits tr; const auto toks = tokens (normalize (text)); float mult = 1.0f;
    for (size_t i = 0; i < toks.size(); ++i)
    {
        const auto& tok = toks[i];
        if (tok == "muito" || tok == "mt" || tok == "very" || tok == "super" || tok == "bem" || tok == "extremamente" || tok == "extra" || tok == "mega" || tok == "ultra" || tok == "bastante") { mult = 1.6f; continue; }
        if (tok == "pouco" || tok == "ligeiramente" || tok == "slightly" || tok == "levemente" || tok == "bit" || tok == "meio" || tok == "quase") { mult = 0.5f; continue; }
        if (tok == "nao" || tok == "sem" || tok == "not" || tok == "without" || tok == "non" || tok == "nunca") { mult = -1.0f; continue; }
        bool used = false;
        for (auto& k : kindLex()) if (stemMatch (tok, k.stem)) { if (tr.kind == K_NONE || k.kind != K_LEAD) { if (mult > 0) tr.kind = k.kind; } used = true; ++tr.hits; break; }
        if (tok == "mono" || tok == "monofonic" || tok == "monofonico") tr.mono = true;
        if (tok == "arp" || tok == "arpejo" || tok == "arpeggiated" || tok == "arpejado") tr.arp = true;
        for (auto& l : traitLex()) if (stemMatch (tok, l.stem)) { tr.t[l.trait] += l.v * mult; used = true; ++tr.hits; }
        applyGenre (tr, tok, mult > 0 ? 1.0f : 0.0f);
        if (used || true) mult = 1.0f;
    }
    for (auto& v : tr.t) v = std::max (-1.0f, std::min (1.0f, v));
    return tr;
}

// ------------------------------------------------------------------------------------------------
struct Gen
{
    Preset p; unsigned rs = 1;
    float rnd() { rs = rs * 1664525u + 1013904223u; return (float) ((rs >> 8) & 0xFFFF) / 65535.0f; }
    float jit (float amount) { return 1.0f + (rnd() * 2.0f - 1.0f) * amount; }
    void S (const char* id, float v) { const int i = paramIndexForId (id); if (i >= 0) p.set (i, std::max (kParams[i].mn, std::min (kParams[i].mx, v))); }
    float G (const char* id) const { const int i = paramIndexForId (id); return i >= 0 ? p.get (i) : 0.0f; }
    void mod (int slot, int src, int dst, float amt) { const int b = modSlotBase (slot); p.set (b, (float) src); p.set (b + 1, (float) dst); p.set (b + 2, std::max (-1.0f, std::min (1.0f, amt))); }
    void fx (const char* slot, int type, float a, float b, float c, float d, float e, float mix)
    {
        const std::string s (slot); S ((s + "_type").c_str(), (float) type); S ((s + "_p1").c_str(), a); S ((s + "_p2").c_str(), b); S ((s + "_p3").c_str(), c); S ((s + "_p4").c_str(), d); S ((s + "_p5").c_str(), e); S ((s + "_mix").c_str(), mix);
    }
};

inline Preset generate (const std::string& text, unsigned seed = 1)
{
    Traits tr = parse (text); Gen g; g.rs = seed * 2654435761u + 12345u; for (int i = 0; i < 3; ++i) g.rnd();
    std::string err; presets::applyKv (g.p, presets::defaultMacros(), &err);
    int kind = tr.kind;
    if (kind == K_NONE)   // sem tipo explícito: escolhe pelo carácter pedido
    {
        if (tr.t[T_LEN] > 0.4f || tr.t[T_SPACE] > 0.5f || tr.t[T_ATTACK] > 0.5f) kind = K_PAD;
        else if (tr.t[T_LEN] < -0.4f) kind = K_PLUCK; else if (tr.t[T_FAT] > 0.6f) kind = K_BASS; else if (tr.t[T_METAL] > 0.6f) kind = K_BELL; else kind = K_LEAD;
    }
    const float bright = tr.t[T_BRIGHT], aggr = tr.t[T_AGGR], wide = tr.t[T_WIDE], len = tr.t[T_LEN], move = tr.t[T_MOVE], dirty = tr.t[T_DIRTY], space = tr.t[T_SPACE];
    const float atk = tr.t[T_ATTACK], fat = tr.t[T_FAT], warm = tr.t[T_WARM], rhythm = tr.t[T_RHYTHM], glide = tr.t[T_GLIDE], metal = tr.t[T_METAL], vib = tr.t[T_VIBRATO];

    // ---- base por tipo ---------------------------------------------------------------------------
    float cut = 2500, res = 0.15f, aA = 5, aD = 400, aS = 0.8f, aR = 250, fenv = 0.0f; int ftype = 2; float revMix = 0.15f, vel = 0.7f;
    auto ana = [&] (const char* E, int wave, int uni, float det) { const std::string e (E); g.S ((e + "_on").c_str(), 1); g.S ((e + "_type").c_str(), 0); g.S ((e + "_wave").c_str(), (float) wave); g.S ((e + "_uni").c_str(), (float) uni); g.S ((e + "_unidet").c_str(), det); };
    auto wtb = [&] (const char* E, int tab, float pos, int uni, float det) { const std::string e (E); g.S ((e + "_on").c_str(), 1); g.S ((e + "_type").c_str(), 1); g.S ((e + "_wttab").c_str(), (float) tab); g.S ((e + "_wtpos").c_str(), pos); g.S ((e + "_uni").c_str(), (float) uni); g.S ((e + "_unidet").c_str(), det); };
    g.S ("b_on", 0);
    switch (kind)
    {
        case K_BASS:    ana ("a", 2, 1, 0); g.S ("sub_level", -8); g.S ("sub_wave", 0); cut = 650; res = 0.2f; aA = 2; aD = 450; aS = 0.8f; aR = 120; fenv = 0.25f; g.S ("voice_mode", 1); g.S ("glide", 20); revMix = 0.05f; break;
        case K_808:     ana ("a", 0, 1, 0); g.S ("sub_level", -10); g.S ("sub_wave", 0); cut = 900; res = 0.1f; aA = 1; aD = 1800; aS = 0.0f; aR = 350; ftype = 1; g.S ("voice_mode", 2); g.S ("glide", 45);
                        g.S ("env2_a", 0.5f); g.S ("env2_d", 70); g.S ("env2_s", 0); g.S ("env2_r", 60); g.mod (0, MS_ENV2, MD_PITCH_ALL, 0.22f); revMix = 0.04f; break;
        case K_LOG:     ana ("a", 0, 1, 0); g.S ("sub_level", -9); cut = 1400; res = 0.15f; aA = 1; aD = 650; aS = 0.0f; aR = 220; ftype = 1;
                        g.S ("env2_a", 0.5f); g.S ("env2_d", 90); g.S ("env2_s", 0); g.S ("env2_r", 60); g.mod (0, MS_ENV2, MD_PITCH_ALL, 0.12f); revMix = 0.08f; break;
        case K_LEAD:    ana ("a", 2, 3, 20); ana ("b", 2, 3, 12); g.S ("b_coarse", 12); g.S ("b_level", -9); g.S ("blend", 0.5f); cut = 3400; res = 0.22f; aA = 5; aD = 350; aS = 0.8f; aR = 260; fenv = 0.3f; revMix = 0.16f;
                        g.S ("env2_a", 2); g.S ("env2_d", 380); g.S ("env2_s", 0.3f); g.S ("env2_r", 300); break;
        case K_PAD:     ana ("a", 2, 4, 24); wtb ("b", 10, 0.5f, 3, 14); g.S ("b_level", -6); g.S ("blend", 0.45f); cut = 2200; res = 0.12f; ftype = 1; aA = 700; aD = 800; aS = 0.85f; aR = 1800; revMix = 0.35f; break;
        case K_PLUCK:   ana ("a", 2, 2, 8); cut = 2200; res = 0.15f; aA = 1; aD = 380; aS = 0.0f; aR = 220; fenv = 0.55f; revMix = 0.18f;
                        g.S ("env2_a", 1); g.S ("env2_d", 220); g.S ("env2_s", 0); g.S ("env2_r", 120); break;
        case K_KEYS:    wtb ("a", 13, 0.3f, 1, 0); cut = 5200; res = 0.08f; ftype = 1; aA = 3; aD = 1300; aS = 0.35f; aR = 420; revMix = 0.2f; break;
        case K_BELL:    wtb ("a", 5, 0.4f, 1, 0); g.S ("a_fmamt", 0.0f); cut = 9000; res = 0.05f; ftype = 1; aA = 1; aD = 2200; aS = 0.0f; aR = 1400; revMix = 0.3f; break;
        case K_STAB:    ana ("a", 2, 3, 14); ana ("b", 2, 2, 8); g.S ("b_level", -4); g.S ("blend", 0.5f); cut = 1600; res = 0.18f; aA = 8; aD = 260; aS = 0.55f; aR = 150; fenv = 0.5f; revMix = 0.14f;
                        g.S ("env2_a", 6); g.S ("env2_d", 260); g.S ("env2_s", 0.2f); g.S ("env2_r", 200); break;
        case K_STRINGS: ana ("a", 2, 6, 20); cut = 3200; res = 0.1f; ftype = 1; aA = 280; aD = 500; aS = 0.88f; aR = 650; revMix = 0.28f; break;
        case K_ORGAN:   wtb ("a", 2, 0.4f, 1, 0); cut = 7000; res = 0.05f; ftype = 1; aA = 3; aD = 100; aS = 1.0f; aR = 70; revMix = 0.15f; break;
        case K_FX:      ana ("a", 2, 3, 40); g.S ("noise_level", -9); g.S ("noise_color", 0.6f); ftype = 5; cut = 1200; res = 0.55f; aA = 900; aD = 1500; aS = 0.5f; aR = 1800; revMix = 0.45f;
                        g.S ("env2_a", 1800); g.S ("env2_d", 1500); g.S ("env2_s", 0.6f); g.S ("env2_r", 1500); fenv = 0.8f; break;
        case K_VOCAL:   ana ("a", 2, 3, 14); ftype = 9; cut = 900; res = 0.5f; aA = 140; aD = 500; aS = 0.85f; aR = 500; revMix = 0.3f; break;
        case K_ARP:     ana ("a", 2, 2, 10); cut = 3000; res = 0.2f; aA = 1; aD = 260; aS = 0.0f; aR = 160; fenv = 0.4f; revMix = 0.2f; g.S ("env2_a", 1); g.S ("env2_d", 200); g.S ("env2_s", 0); g.S ("env2_r", 100); break;
        default: ana ("a", 2, 1, 0); break;
    }

    // ---- traços ------------------------------------------------------------------------------------
    cut *= std::pow (2.0f, bright * 1.5f + warm * -0.45f + aggr * 0.25f);
    res = std::max (0.0f, std::min (0.9f, res + aggr * 0.2f - (aggr < 0 ? 0.05f : 0.0f) + (metal * 0.1f)));
    if (aggr < -0.35f && ftype == 2) ftype = 1;                      // suave: filtro de 12 dB
    if (aggr > 0.6f && kind != K_FX && kind != K_VOCAL) g.S ("f1_drive", 0.3f + 0.4f * aggr);
    if (warm > 0.3f) { g.S ("f1_drive", std::max (g.G ("f1_drive"), 0.15f * warm)); if (ftype == 2) ftype = 1; g.S ("sub_level", std::max (g.G ("sub_level"), -16.0f)); }
    if (len != 0.0f) { const float f = std::pow (2.0f, len * 2.0f); aD *= f; aR *= std::pow (2.0f, len * 1.6f); if (aS > 0.001f && len < 0) aS = std::max (0.0f, aS * (1.0f + len * 0.7f)); if (len > 0.4f && aS < 0.5f) aS = std::min (0.6f, aS + 0.15f); }
    if (atk != 0.0f) aA = std::max (0.5f, aA * std::pow (2.0f, atk * 5.0f) * (atk > 0 ? 3.0f : 1.0f) * (atk < 0 ? 0.3f : 1.0f));
    aA *= g.jit (0.1f); aD *= g.jit (0.1f); cut *= g.jit (0.08f);
    // largura: mais vozes de unisson / chorus; estreito: 1 voz
    int uniA = (int) g.G ("a_uni"); const bool pitched = kind != K_808 && kind != K_LOG && kind != K_BELL && kind != K_KEYS && kind != K_ORGAN;
    if (pitched) { uniA = std::max (1, std::min (8, uniA + (int) std::lround (wide * 3.0f + fat * 1.0f))); if (wide < -0.5f) uniA = 1; g.S ("a_uni", (float) uniA); g.S ("a_unidet", g.G ("a_unidet") * (1.0f + 0.5f * wide + 0.4f * fat) * g.jit (0.1f)); g.S ("a_unispr", std::max (0.1f, 0.6f + 0.35f * wide)); }
    if (fat > 0.1f) g.S ("sub_level", std::max (g.G ("sub_level"), -16.0f + 6.0f * (1.0f - fat)) + 0.0f);
    if (fat < -0.4f) { g.S ("sub_level", -60); g.S ("f1_type", 0); }
    if (metal > 0.3f)
    {
        if (kind == K_BELL || kind == K_KEYS || kind == K_PLUCK) { g.S ("a_type", 0); g.S ("a_wave", 0); g.S ("a_fmamt", 0.25f + 0.55f * metal); g.S ("a_fmrat", 3); }
        else { g.S ("a_fmamt", 0.15f + 0.4f * metal); g.S ("a_fmrat", 3); }
    }
    if (dirty > 0.2f) { g.S ("noise_level", std::max (g.G ("noise_level"), -34.0f + 12.0f * (1.0f - dirty))); g.S ("noise_color", 0.4f); }
    if (aggr > 0.5f && pitched) g.S ("a_fold", 0.25f * aggr);
    if (rhythm > 0.3f)
    {   // step sequencer a abrir/fechar o filtro em semicolcheias
        g.S ("step_div", 12); g.S ("step_len", 8); g.S ("step_smooth", 0.0f);
        const float pat[8] = { 1.0f, -0.2f, 0.6f, -0.6f, 0.9f, -0.3f, 0.4f, -0.8f }; for (int i = 0; i < 8; ++i) g.S ((std::string ("step_v") + std::to_string (i + 1)).c_str(), pat[i]);
        g.mod (2, MS_STEP, MD_CUT1, 0.35f * rhythm); if (kind == K_PAD || kind == K_STRINGS || kind == K_VOCAL) g.mod (3, MS_STEP, MD_LEVEL_A, 0.25f * rhythm);
    }
    if (move > 0.2f)
    {   // LFOs lentos: corte e posição / afinação do unisson
        g.S ("lfo1_wave", move > 0.7f ? 6.0f : 0.0f); g.S ("lfo1_rate", 0.12f + 0.8f * move * g.jit (0.3f)); g.mod (1, MS_LFO1, MD_CUT1, 0.18f + 0.3f * move);
        if (g.G ("a_type") > 0.5f) g.mod (4, MS_LFO2, MD_WTPOS_A, 0.25f * move); else g.mod (4, MS_LFO2, MD_UNIDET_A, 0.2f * move);
        g.S ("lfo2_rate", 0.07f + 0.35f * move); g.S ("lfo2_wave", 1);
    }
    if (vib > 0.2f) { g.S ("lfo1_rate", 5.4f); g.S ("lfo1_fade", 500); g.S ("lfo1_wave", 0); g.mod (5, MS_LFO1, MD_PITCH_ALL, 0.03f + 0.05f * vib); }
    if (glide > 0.2f) { g.S ("voice_mode", 2); g.S ("glide", 40 + 160 * glide); }
    if (tr.mono) { g.S ("voice_mode", 1); }
    if (tr.arp || kind == K_ARP) { g.S ("arp_on", 1); g.S ("arp_rate", 12); g.S ("arp_gate", 0.5f); g.S ("arp_mode", 0); g.S ("arp_oct", 2); }
    if (fenv != 0.0f) g.S ("f1_env", std::min (1.0f, fenv * (1.0f + 0.3f * aggr)));
    g.S ("f1_type", (float) ftype); g.S ("f1_cut", std::max (60.0f, std::min (18000.0f, cut))); g.S ("f1_res", res);
    if (kind == K_VOCAL) { g.S ("f1_cut", 650 + 600 * bright); }
    g.S ("amp_a", std::max (0.5f, aA)); g.S ("amp_d", std::max (5.0f, aD)); g.S ("amp_s", aS); g.S ("amp_r", std::max (20.0f, aR));
    g.S ("vel_sens", vel);

    // ---- efeitos: chorus/saturação (slot 1), delay (slot 2), reverb (slot 3) ----------------------------
    const bool satNeeded = dirty > 0.2f || aggr > 0.45f || warm > 0.5f || kind == K_808;
    if (satNeeded) g.fx ("fxa1", 3, 0.25f + 0.45f * std::max (dirty, aggr), 0.6f - 0.2f * dirty, 0.1f, 0.3f + 0.4f * dirty, 0.7f, 0.9f);
    else if (wide > 0.1f || kind == K_PAD || kind == K_STRINGS || kind == K_LEAD || kind == K_KEYS) g.fx ("fxa1", 4, 0.3f, 0.4f + 0.2f * wide, 0.5f, 0.6f, 1.0f, 0.2f + 0.3f * std::max (0.0f, wide));
    if (dirty > 0.55f) g.fx ("fxa2", 8, 0.65f - 0.3f * dirty, 0.7f - 0.3f * dirty, 0.2f, 0.7f, 0.7f, 0.5f);
    else if (space > 0.15f || rhythm > 0.3f || kind == K_LEAD || kind == K_PLUCK || kind == K_ARP) g.fx ("fxa2", 11, 0.43f, 0.35f, 0.55f, 0.5f, 0.5f, 0.12f + 0.25f * std::max (0.0f, space));
    revMix = std::max (0.0f, std::min (0.7f, revMix + 0.3f * space));
    if (space < -0.3f) revMix = 0.03f;
    if (revMix > 0.02f) g.fx ("fxa3", 6, 0.45f + 0.35f * std::max (0.0f, space), 0.45f + 0.35f * std::max (0.0f, space), 0.45f, 0.08f, 0.9f, revMix);
    if (kind == K_808 || kind == K_BASS) { g.fx ("fxa2", 3, 0.3f + 0.3f * aggr, 0.5f, 0.05f, 0.3f, 0.6f, 0.9f); g.S ("fxa1_type", 0); }
    if (tr.genre == "kuduro" || tr.genre == "dance") g.S ("send_a", 0.1f);

    // ---- nome, estilo, etiquetas -----------------------------------------------------------------------
    static const char* kindName[K_COUNT] = { "Baixo", "808", "Lead", "Pad", "Pluck", "Teclas", "Sino", "Stab", "Cordas", "Órgão", "FX", "Vocal", "Arp", "Log Drum" };
    g.p.category = kindName[std::max (0, std::min (K_COUNT - 1, kind))];
    g.p.style = "user"; g.p.pack = "user"; g.p.author = "Descrever o som";
    g.p.tags = { "gerado", g.p.category };
    return g.p;
}

// ------------------------------------------------------------------------------------------------
//  calibração: renderiza o patch e ajusta o volume master para ~ -16 dB RMS, pico abaixo do teto
// ------------------------------------------------------------------------------------------------
inline float measureRms (const Preset& pr, double sr, float* peakOut = nullptr, int note = 57)
{
    static float P[P_COUNT]; for (int i = 0; i < P_COUNT; ++i) P[i] = pr.get (i);
    Engine e; e.prepare (sr, 512); e.setParams (P);
    e.noteOn (note, 0.85f);
    const float att = std::max (0.0f, P[P_AMP_A]) * 0.001f;
    const int total = (int) (sr * std::max (1.0f, std::min (4.0f, 1.0f + 2.5f * att)) / 512.0);
    const int win = std::max (1, (int) (sr * 0.1 / 512.0));                       // janelas de ~100 ms: conta a mais alta (plucks curtos, pads lentos)
    std::vector<float> L (512), R (512); double acc = 0; int inWin = 0; float best = 0, pk = 0; const int releaseAt = (int) (total * 0.7);
    for (int b = 0; b < total; ++b)
    {
        if (b == releaseAt) e.noteOff (note);
        e.render (L.data(), R.data(), 512);
        for (int i = 0; i < 512; ++i) { acc += (double) L[(size_t) i] * L[(size_t) i] + (double) R[(size_t) i] * R[(size_t) i]; pk = std::max (pk, std::max (std::fabs (L[(size_t) i]), std::fabs (R[(size_t) i]))); }
        if (++inWin >= win) { best = std::max (best, (float) std::sqrt (acc / (double) (inWin * 1024))); acc = 0; inWin = 0; }
    }
    if (peakOut) *peakOut = pk;
    return best;
}

inline void calibrate (Preset& pr, double sr = 48000.0)
{
    pr.set (P_MASTER_VOL, 0.0f); pr.set (P_INSA_LEVEL, 0.0f);
    const float target = std::pow (10.0f, -15.0f / 20.0f);                 // ~ -15 dBFS RMS na janela mais forte
    float master = 0.0f, insert = 0.0f, lvl = 0.0f;
    for (int it = 0; it < 3; ++it)
    {
        const float rms = measureRms (pr, sr, nullptr);
        if (rms < 1.0e-6f) return;
        float need = 20.0f * std::log10 (target / rms);                    // dB que faltam (ou sobram)
        if (std::fabs (need) < 0.4f) break;
        // reparte: primeiro o volume master, depois o insert A, por fim o nível dos motores
        float m = std::max (-24.0f, std::min (6.0f, master + need)); need -= (m - master); master = m;
        if (need > 0.0f) { const float i2 = std::min (6.0f, insert + need); need -= (i2 - insert); insert = i2; }
        if (need > 0.0f) { const float l2 = std::min (6.0f, lvl + need); lvl = l2; }
        pr.set (P_MASTER_VOL, master); pr.set (P_INSA_LEVEL, insert);
        pr.set (P_A_LEVEL, lvl); if (pr.get (P_B_ON) > 0.5f) pr.set (P_B_LEVEL, std::min (6.0f, pr.get (P_B_LEVEL) + 0.0f + (it == 0 ? lvl : 0.0f)));
    }
}

inline Preset generateCalibrated (const std::string& text, unsigned seed = 1, double sr = 48000.0)
{
    Preset p = generate (text, seed); calibrate (p, sr);
    std::string label = text; while (! label.empty() && label.back() == ' ') label.pop_back();
    if (label.size() > 28) { size_t n = 28; while (n > 0 && ((unsigned char) label[n] & 0xC0) == 0x80) --n; label = label.substr (0, n) + "\xE2\x80\xA6"; }
    p.name = label.empty() ? "Som gerado" : label;
    return p;
}
}}
