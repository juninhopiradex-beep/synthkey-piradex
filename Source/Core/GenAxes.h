// =============================================================================
//  SYNTHKEY by Piradex — gerador de variações de presets (v1.6)
//
//  Cada preset escrito à mão é uma BASE. Um "código" de 27 bits (9 eixos × 3 bits) descreve uma variação
//  musical da base: tom, envelope, espaço, largura, movimento, carácter, registo, oscilador, camada
//  (Espectral / Granular / Sub / Ruído). A mesma base + o mesmo código dão SEMPRE o mesmo preset.
//
//  Quais códigos entram na biblioteca é decidido offline (packgen curate): renderiza cada candidato,
//  mede-o, rejeita o que soa igual ao que já foi aceite e calibra o nível — o resultado fica em
//  GenCurated.h. Estado de validação de tudo o que sai daqui: "auto" (nunca ouvido por uma pessoa).
// =============================================================================
#pragma once
#include "Preset.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <string>
#include <vector>

namespace sk { namespace presets { namespace gen
{
enum Axis { A_TONE = 0, A_SHAPE, A_SPACE, A_WIDTH, A_MOVE, A_CHAR, A_REG, A_OSC, A_LAYER, A_COUNT };
enum Kind { K_SUST = 0, K_LEAD, K_TONAL, K_BASS, K_PERC, K_ARP, K_FX };

inline uint32_t mix32 (uint32_t x) { x ^= x >> 16; x *= 0x7feb352dU; x ^= x >> 15; x *= 0x846ca68bU; x ^= x >> 16; return x; }
inline uint32_t hashStr (const std::string& s) { uint32_t h = 2166136261u; for (unsigned char c : s) h = (h ^ c) * 16777619u; return h; }

inline int digit (uint32_t code, int axis) { return (int) ((code >> (3 * axis)) & 7u); }

inline Kind kindOf (const Preset& p)
{
    const std::string& f = p.family.empty() ? familyForCategory (p.category) : p.family;
    if (f == "Bass") return K_BASS;
    if (f == "Drums" || f == "Percussion") return K_PERC;
    if (f == "SFX") return K_FX;
    if (f == "Arpeggios" || f == "Sequences" || p.get (P_ARP_ON) > 0.5f) return K_ARP;
    if (f == "Leads" || f == "Synths") return K_LEAD;
    if (f == "Keys" || f == "Plucks" || f == "Bells" || f == "Mallets" || f == "Guitars" || f == "Chords") return K_TONAL;
    return K_SUST;      // pads, cordas, metais, vozes, órgãos, sopros, texturas, cinemático, híbrido, experimental
}

namespace detail
{
inline void setc (Preset& p, int idx, float v) { p.set (idx, std::max (kParams[idx].mn, std::min (kParams[idx].mx, v))); }
inline void mulc (Preset& p, int idx, float k) { setc (p, idx, p.get (idx) * k); }
inline bool isTone (const Preset& p, int E) { const int t = (int) p.get (E ? P_B_TYPE : P_A_TYPE); const bool on = E ? p.get (P_B_ON) > 0.5f : p.get (P_A_ON) > 0.5f; return on && (t == 0 || t == 1); }
inline int freeFx (const Preset& p) { for (int s = 0; s < 3; ++s) if ((int) p.get (fxSlotBase (s)) == 0) return s; return -1; }
inline int findFx (const Preset& p, int type) { for (int s = 0; s < 3; ++s) if ((int) p.get (fxSlotBase (s)) == type) return s; return -1; }
inline void putFx (Preset& p, int s, int type, float a, float b, float c, float d, float e, float mixv)
{ const int bs = fxSlotBase (s); p.set (bs, (float) type); const float v[5] = { a, b, c, d, e }; for (int i = 0; i < 5; ++i) p.set (bs + 1 + i, v[i]); p.set (bs + 6, mixv); }
inline int freeMod (const Preset& p) { for (int s = 0; s < 8; ++s) { const int b = modSlotBase (s); if ((int) p.get (b) == 0 || (int) p.get (b + 1) == 0 || std::fabs (p.get (b + 2)) < 1e-4f) return s; } return -1; }
inline void putMod (Preset& p, int s, int src, int dst, float amt) { const int b = modSlotBase (s); p.set (b, (float) src); p.set (b + 1, (float) dst); p.set (b + 2, amt); }
}

// o eixo faz alguma coisa neste preset?
inline bool applicable (Axis a, const Preset& p, Kind k)
{
    using namespace detail;
    switch (a)
    {
        case A_TONE:  return (int) p.get (P_F1_TYPE) != 0;
        case A_SHAPE: return k != K_FX;
        case A_SPACE: return true;
        case A_WIDTH: return (k == K_SUST || k == K_LEAD || k == K_TONAL || k == K_ARP) && (isTone (p, 0) || isTone (p, 1));
        case A_MOVE:  return k != K_PERC && freeMod (p) >= 0;
        case A_CHAR:  return freeFx (p) >= 0;
        case A_REG:   return k != K_FX;
        case A_OSC:   return isTone (p, 0) || (int) p.get (P_A_TYPE) == 3;
        case A_LAYER: return (k == K_SUST || k == K_LEAD || k == K_TONAL || k == K_BASS) && p.get (P_B_ON) < 0.5f;
        default: return false;
    }
}

// aplica a opção (1..5) de um eixo; devolve a palavra para o nome (ou "" se não fez nada)
inline const char* apply (Axis a, int opt, Preset& p, Kind k)
{
    using namespace detail;
    switch (a)
    {
    case A_TONE:
    {
        static const float m[6] = { 1, 0.42f, 0.7f, 1.0f, 1.6f, 2.8f }; static const char* w[6] = { "", "Escuro", "Suave", "", "Brilhante", "Aberto" };
        if (opt == 3 || opt > 5) return ""; mulc (p, P_F1_CUT, m[opt]); if (opt == 5) mulc (p, P_F1_RES, 0.8f); return w[opt];
    }
    case A_SHAPE:
    {
        if (k == K_SUST)
        {   static const char* w[6] = { "", "Lento", "Fluido", "Curto", "Respirado", "Rápido" };
            switch (opt) { case 1: mulc (p, P_AMP_A, 2.6f); mulc (p, P_ENV2_A, 2.4f); break; case 2: mulc (p, P_AMP_R, 1.9f); break; case 3: mulc (p, P_AMP_R, 0.45f); mulc (p, P_AMP_D, 0.7f); break;
                           case 4: mulc (p, P_AMP_A, 1.8f); mulc (p, P_AMP_R, 1.5f); setc (p, P_AMP_S, p.get (P_AMP_S) * 0.8f); break; case 5: mulc (p, P_AMP_A, 0.3f); break; default: return ""; }
            return w[opt]; }
        static const char* w[6] = { "", "Curto", "Longo", "Percussivo", "Macio", "Seco" };
        switch (opt) { case 1: mulc (p, P_AMP_D, 0.6f); mulc (p, P_AMP_R, 0.55f); break; case 2: mulc (p, P_AMP_D, 1.7f); mulc (p, P_AMP_R, 1.8f); break;
                       case 3: setc (p, P_AMP_A, 1.0f); mulc (p, P_AMP_D, 0.5f); mulc (p, P_ENV2_D, 0.5f); break; case 4: mulc (p, P_AMP_A, 4.0f); break;
                       case 5: mulc (p, P_AMP_D, 0.4f); mulc (p, P_AMP_R, 0.3f); setc (p, P_AMP_S, p.get (P_AMP_S) * 0.5f); break; default: return ""; }
        return w[opt];
    }
    case A_SPACE:
    {
        static const char* w[6] = { "", "Seco", "Sala", "Amplo", "Catedral", "Brilho" };
        int s = findFx (p, 6);
        if (opt == 1) { if (s >= 0) { setc (p, fxSlotBase (s) + 6, p.get (fxSlotBase (s) + 6) * 0.25f); } else return ""; return w[1]; }
        if (opt == 5) { const int f = findFx (p, 14); if (f >= 0) return ""; const int fs = freeFx (p); if (fs < 0) return ""; putFx (p, fs, 14, 0.7f, 0.72f, 0.45f, 0.35f, 1.0f, 0.4f); return w[5]; }
        if (s < 0) { s = freeFx (p); if (s < 0) return ""; putFx (p, s, 6, 0.5f, 0.5f, 0.5f, 0.08f, 1.0f, 0.2f); }
        const int b = fxSlotBase (s);
        switch (opt) { case 2: setc (p, b + 1, 0.55f); setc (p, b + 2, 0.5f); setc (p, b + 6, std::max (0.22f, p.get (b + 6))); break;
                       case 3: setc (p, b + 1, 0.78f); setc (p, b + 2, 0.7f); setc (p, b + 6, std::max (0.32f, p.get (b + 6) * 1.3f)); break;
                       case 4: setc (p, b + 1, 0.95f); setc (p, b + 2, 0.9f); setc (p, b + 4, 1.0f); setc (p, b + 6, std::max (0.42f, p.get (b + 6) * 1.6f)); break; default: return ""; }
        return w[opt];
    }
    case A_WIDTH:
    {
        static const char* w[6] = { "", "Mono", "Estreito", "Largo", "Super", "Denso" };
        static const int uni[6] = { 0, 1, 2, 5, 7, 4 }; static const float det[6] = { 0, 0, 0.5f, 1.3f, 1.8f, 0.8f };
        if (opt < 1 || opt > 5) return "";
        for (int E = 0; E < 2; ++E) if (isTone (p, E)) { const int u = E ? P_B_UNI : P_A_UNI, d = E ? P_B_UNIDET : P_A_UNIDET; setc (p, u, (float) uni[opt]); if (uni[opt] > 1) setc (p, d, std::max (8.0f, p.get (d)) * det[opt]); else setc (p, d, 0.0f); }
        return w[opt];
    }
    case A_MOVE:
    {
        static const char* w[6] = { "", "Respira", "Vibrato", "Ondula", "Tremolo", "Deriva" };
        const int s = freeMod (p); if (s < 0 || opt < 1 || opt > 5) return "";
        switch (opt)
        {   case 1: putMod (p, s, MS_LFO1, MD_CUT1, 0.35f); setc (p, P_LFO1_RATE, 0.25f); setc (p, P_LFO1_WAVE, 0); break;
            case 2: putMod (p, s, MS_LFO1, MD_PITCH_ALL, 0.03f); setc (p, P_LFO1_RATE, 5.4f); setc (p, P_LFO1_FADE, 500.0f); setc (p, P_LFO1_WAVE, 0); break;
            case 3: putMod (p, s, MS_LFO1, p.get (P_A_TYPE) == 1.0f ? MD_WTPOS_A : MD_PWM_A, 0.22f); setc (p, P_LFO1_RATE, 0.4f); setc (p, P_LFO1_WAVE, 1); break;
            case 4: putMod (p, s, MS_LFO1, MD_LEVEL_A, 0.3f); setc (p, P_LFO1_RATE, 6.0f); setc (p, P_LFO1_WAVE, 0); break;
            case 5: putMod (p, s, MS_RANDOM, MD_PITCH_ALL, 0.006f); break; }
        return w[opt];
    }
    case A_CHAR:
    {
        static const char* w[6] = { "", "Quente", "Sujo", "Lo-Fi", "Chorus", "Fase" };
        const int s = freeFx (p); if (s < 0 || opt < 1 || opt > 5) return "";
        switch (opt) { case 1: putFx (p, s, 3, 0.22f, 0.4f, 0.05f, 0.2f, 0.85f, 0.5f); break; case 2: putFx (p, s, 3, 0.6f, 0.7f, 0.1f, 0.6f, 0.75f, 0.7f); break;
                       case 3: putFx (p, s, 8, 0.6f, 0.45f, 0.1f, 0.8f, 0.8f, 0.7f); break; case 4: putFx (p, s, 4, 0.3f, 0.45f, 0.5f, 0.5f, 0.9f, 0.4f); break;
                       case 5: putFx (p, s, 7, 0.3f, 0.6f, 0.4f, 0.5f, 0.6f, 0.5f); break; }
        return w[opt];
    }
    case A_REG:
    {
        static const char* w[6] = { "", "Grave", "Agudo", "Oitava Acima", "Quinta", "Duas Oitavas" };
        static const float st[6] = { 0, -12, 12, 12, 7, -24 };
        if (opt < 1 || opt > 5) return "";
        if (k == K_BASS && (opt == 3 || opt == 4)) return "";
        if (k != K_BASS && opt == 5) return "";
        if (k == K_PERC && opt >= 3) return "";
        setc (p, P_A_COARSE, p.get (P_A_COARSE) + st[opt]); if (p.get (P_B_ON) > 0.5f) setc (p, P_B_COARSE, p.get (P_B_COARSE) + st[opt]);
        return w[opt];
    }
    case A_OSC:
    {
        if ((int) p.get (P_A_TYPE) == 3)
        {   static const char* w[6] = { "", "Macio", "Duro", "Aberto", "Nylon", "Aço" };
            switch (opt) { case 1: setc (p, P_A_KS_DAMP, p.get (P_A_KS_DAMP) + 0.3f); break; case 2: setc (p, P_A_KS_DAMP, p.get (P_A_KS_DAMP) - 0.25f); setc (p, P_A_KS_PICK, 0.2f); break;
                           case 3: setc (p, P_A_KS_DECAY, p.get (P_A_KS_DECAY) + 0.25f); break; case 4: setc (p, P_A_KS_PICK, 0.6f); setc (p, P_A_KS_DAMP, 0.55f); break; case 5: setc (p, P_A_KS_PICK, 0.12f); setc (p, P_A_KS_DAMP, 0.1f); break; default: return ""; }
            return w[opt]; }
        if ((int) p.get (P_A_TYPE) == 1)
        {   static const char* w[6] = { "", "Tabela Vidro", "Tabela Digital", "Posição Alta", "Posição Baixa", "Tabela Órgão" };
            switch (opt) { case 1: setc (p, P_A_WTTAB, 4); break; case 2: setc (p, P_A_WTTAB, 6); break; case 3: setc (p, P_A_WTPOS, p.get (P_A_WTPOS) + 0.3f); break; case 4: setc (p, P_A_WTPOS, p.get (P_A_WTPOS) - 0.25f); break; case 5: setc (p, P_A_WTTAB, 2); break; default: return ""; }
            return w[opt]; }
        static const char* w[6] = { "", "Serra", "Quadrada", "Triângulo", "Seno", "Pulso" };
        static const int wave[6] = { 0, 2, 3, 1, 0, 3 };
        if (opt < 1 || opt > 5) return "";
        if ((int) p.get (P_A_WAVE) == wave[opt] && opt != 5) return "";
        setc (p, P_A_WAVE, (float) wave[opt]); if (opt == 5) setc (p, P_A_PWM, 0.3f);
        return w[opt];
    }
    case A_LAYER:
    {
        static const char* w[6] = { "", "Espectral", "Granular", "Sub", "Ruído", "Etéreo" };
        switch (opt)
        {   case 1: p.set (P_B_ON, 1); p.set (P_B_TYPE, 5); setc (p, P_B_SP_TILT, 1.1f); setc (p, P_B_SP_FORM, 10); setc (p, P_B_SP_FQ, 0.45f); setc (p, P_B_SP_COMB, 0.15f); setc (p, P_B_SP_SWEEP, 0.4f); setc (p, P_B_LEVEL, -9); setc (p, P_B_UNI, 2); setc (p, P_B_UNIDET, 10); setc (p, P_BLEND, 0.4f); break;
            case 2: p.set (P_B_ON, 1); p.set (P_B_TYPE, 4); p.sampleB = k == K_BASS ? "nucleo.sub" : "nucleo.pad"; setc (p, P_B_GR_SIZE, 120); setc (p, P_B_GR_DENS, 28); setc (p, P_B_GR_RAND, 0.3f); setc (p, P_B_GR_PJIT, 0.1f); setc (p, P_B_GR_SPEED, 0.4f); setc (p, P_B_LEVEL, -10); setc (p, P_BLEND, 0.4f); break;
            case 3: p.set (P_SUB_LEVEL, std::max (p.get (P_SUB_LEVEL), -14.0f)); break;
            case 4: setc (p, P_NOISE_LEVEL, -30); setc (p, P_NOISE_COLOR, 0.7f); break;
            case 5: p.set (P_B_ON, 1); p.set (P_B_TYPE, 5); setc (p, P_B_SP_TILT, 0.8f); setc (p, P_B_SP_FORM, 24); setc (p, P_B_SP_FQ, 0.6f); setc (p, P_B_SP_COMB, 0.5f); setc (p, P_B_SP_SWEEP, 0.7f); setc (p, P_B_SP_SMEAR, 0.4f); setc (p, P_B_LEVEL, -11); setc (p, P_B_UNI, 3); setc (p, P_B_UNIDET, 14); setc (p, P_BLEND, 0.35f); break;
            default: return ""; }
        return w[opt];
    }
    default: return "";
    }
}

// etiqueta de motor de um preset (para o browser): o motor principal (A) manda; B diferente = híbrido
inline std::string engineLabel (const Preset& p)
{
    const int ta = (int) p.get (P_A_TYPE), tb = (int) p.get (P_B_TYPE); const bool bOn = p.get (P_B_ON) > 0.5f;
    if (ta == 3) return "physical"; if (ta == 4) return "granular"; if (ta == 5) return "spectral";
    if (bOn && tb != ta) return "hybrid";
    return ta == 2 ? "sample" : (ta == 1 ? "wavetable" : "analog");
}

// o n-ésimo candidato (n = 0,1,2,…) de uma base: 2–3 eixos aplicáveis, opção 1..5 de cada
inline uint32_t candidateCode (const Preset& base, uint32_t n)
{
    const Kind k = kindOf (base); const uint32_t h0 = mix32 (hashStr (base.name) ^ mix32 (n * 2654435761u + 17u));
    int ax[A_COUNT], na = 0; for (int a = 0; a < A_COUNT; ++a) if (applicable ((Axis) a, base, k)) ax[na++] = a;
    if (na == 0) return 0;
    const int want = std::min (na, 2 + (int) (h0 % 3u == 0 ? 1 : 0) + (int) (n % 5u == 4 ? 1 : 0));
    uint32_t code = 0, h = h0;
    for (int i = 0; i < want; ++i)
    {
        h = mix32 (h + 0x9E3779B9u); const int j = i + (int) (h % (uint32_t) (na - i)); std::swap (ax[i], ax[j]);
        h = mix32 (h + 1u); code |= (uint32_t) (1 + (int) (h % 5u)) << (3 * ax[i]);
    }
    return code;
}

// constrói o preset derivado; `dup` > 0 acrescenta um numeral para desfazer colisões de nome
inline Preset derive (const Preset& base, uint32_t code, int dup = 0)
{
    Preset p = base; const Kind k = kindOf (base); std::vector<std::string> words;
    for (int a = 0; a < A_COUNT; ++a)
    {
        const int d = digit (code, a); if (d == 0 || ! applicable ((Axis) a, p, k)) continue;
        const char* w = apply ((Axis) a, d, p, k); if (w && w[0]) words.push_back (w);
    }
    for (size_t i = 0; i < words.size() && i < 2; ++i) p.name += " " + words[i];
    if (dup > 0) p.name += " " + std::to_string (dup + 1);
    for (auto& w : words) { std::string t = w; for (auto& c : t) c = (char) std::tolower ((unsigned char) c); p.tags.push_back (t); }
    p.qa = "auto"; p.version = 1;
    p.engine = engineLabel (p);
    return p;
}
}}} // namespace sk::presets::gen
