// =============================================================================
//  SYNTHKEY by Piradex — presets de fábrica (Núcleo + 7 estilos)
//
//  Cada preset é uma linha "chave=valor ..." sobre os ids da ParamTable. Todos recebem
//  as macros padrão (BRILHO, CORPO, MOVIM., ESPAÇO, LARGURA, SUJIDADE, BALANÇO, DOBRA)
//  nos slots 9–16 da matriz e um reverb no Aux; um preset pode redefinir qualquer chave.
//  O nível é calibrado por render (PresetCalib.h) para todos soarem com volume parecido.
// =============================================================================
#pragma once
#include "Preset.h"
#include <cstdlib>
#include <iterator>
#include <string>
#include <vector>

namespace sk { namespace presets
{
// ---- vocabulário (macros de texto) -------------------------------------------------------------
#define AMP(a, d, s, r)   " amp_a=" #a " amp_d=" #d " amp_s=" #s " amp_r=" #r
#define ENV2(a, d, s, r)  " env2_a=" #a " env2_d=" #d " env2_s=" #s " env2_r=" #r
#define ENV3(a, d, s, r)  " env3_a=" #a " env3_d=" #d " env3_s=" #s " env3_r=" #r
#define FIL1(t, c, r)     " f1_type=" #t " f1_cut=" #c " f1_res=" #r
#define FIL1E(t, c, r, e) " f1_type=" #t " f1_cut=" #c " f1_res=" #r " f1_env=" #e
#define FIL2(t, c, r)     " f2_type=" #t " f2_cut=" #c " f2_res=" #r
#define FX(slot, t, a, b, c, d, e, m) " " slot "_type=" #t " " slot "_p1=" #a " " slot "_p2=" #b " " slot "_p3=" #c " " slot "_p4=" #d " " slot "_p5=" #e " " slot "_mix=" #m
// tipos de FX: 1 EQ, 2 Comp, 3 Sat, 4 Chorus, 5 Delay, 6 Reverb, 7 Faser, 8 Crush, 9 Largura, 10 Trémolo
#define REVERB(slot, size, rt, damp, pre, wide, mix) FX (slot, 6, size, rt, damp, pre, wide, mix)
#define DELAY(slot, time, fb, tone, pp, off, mix)    FX (slot, 5, time, fb, tone, pp, off, mix)
#define CHORUS(slot, rate, depth, ctr, vox, wide, mix) FX (slot, 4, rate, depth, ctr, vox, wide, mix)
#define SATUR(slot, drive, tone, bias, kind, out, mix) FX (slot, 3, drive, tone, bias, kind, out, mix)
#define COMP(slot, th, ratio, at, rl, mk, mix)       FX (slot, 2, th, ratio, at, rl, mk, mix)
#define EQ3(slot, lo, mid, mf, hi, q, mix)           FX (slot, 1, lo, mid, mf, hi, q, mix)
#define PHASER(slot, rate, depth, fb, fc, sp, mix)   FX (slot, 7, rate, depth, fb, fc, sp, mix)
#define CRUSH(slot, bits, dec, jit, fc, out, mix)    FX (slot, 8, bits, dec, jit, fc, out, mix)
#define WIDTH(slot, w, fc, bal, mg, sg, mix)         FX (slot, 9, w, fc, bal, mg, sg, mix)
#define TREM(slot, rate, depth, shape, sp, x, mix)   FX (slot, 10, rate, depth, shape, sp, x, mix)
#define MOD(n, s, d, a)  " mod" #n "_src=" #s " mod" #n "_dst=" #d " mod" #n "_amt=" #a
#define ANA(E, wave, uni, det)   " " #E "_on=1 " #E "_type=0 " #E "_wave=" #wave " " #E "_uni=" #uni " " #E "_unidet=" #det
#define WTB(E, tab, pos, uni, det) " " #E "_on=1 " #E "_type=1 " #E "_wttab=" #tab " " #E "_wtpos=" #pos " " #E "_uni=" #uni " " #E "_unidet=" #det
#define SMP(E) " " #E "_on=1 " #E "_type=2"
#define MONO(g) " voice_mode=1 glide=" #g
#define LEGATO(g) " voice_mode=2 glide=" #g

// macros padrão + aux (aplicadas antes do texto de cada preset)
inline const char* defaultMacros()
{
    return
        MOD (9,  11, 11, 0.5) MOD (10, 12, 13, 0.35) MOD (11, 13, 27, 0.5) MOD (12, 14, 34, 0.6)
        MOD (13, 15, 17, 0.5) MOD (14, 16, 15, 0.5)  MOD (15, 17, 32, 0.5) MOD (16, 18, 30, 0.4)
        REVERB ("fxx1", 0.5, 0.55, 0.45, 0.1, 0.9, 1) " aux_return=0";
}

inline bool applyKv (Preset& p, const char* kv, std::string* err = nullptr)
{
    std::string s (kv); size_t i = 0; bool ok = true;
    while (i < s.size())
    {
        while (i < s.size() && (s[i] == ' ' || s[i] == '\n' || s[i] == '\t')) ++i;
        const size_t e = s.find ('=', i); if (e == std::string::npos) break;
        const std::string key = s.substr (i, e - i); size_t j = e + 1; while (j < s.size() && s[j] != ' ' && s[j] != '\n' && s[j] != '\t') ++j;
        const float val = (float) std::atof (s.substr (e + 1, j - e - 1).c_str());
        const int idx = paramIndexForId (key);
        if (idx < 0) { ok = false; if (err) *err += "chave desconhecida: " + key + "; "; }
        else
        {
            const auto& pi = kParams[idx];
            if (val < pi.mn - 1e-4f || val > pi.mx + 1e-4f) { ok = false; if (err) *err += "fora do intervalo: " + key + "; "; }
            p.set (idx, val);
        }
        i = j;
    }
    return ok;
}

struct Row { const char* name; const char* cat; const char* tags; const char* pack; const char* smpA; const char* smpB; const char* kv; };

inline std::vector<std::string>& lintErrors() { static std::vector<std::string> e; return e; }

inline Preset make (const Row& r, const char* style)
{
    Preset p; p.name = r.name; p.category = r.cat; p.pack = r.pack; p.style = style; p.sampleA = r.smpA; p.sampleB = r.smpB;
    std::string t (r.tags); size_t a = 0; while (a < t.size()) { size_t b = t.find (',', a); if (b == std::string::npos) b = t.size(); if (b > a) p.tags.push_back (t.substr (a, b - a)); a = b + 1; }
    std::string err; applyKv (p, defaultMacros(), &err); applyKv (p, r.kv, &err);
    if (! err.empty()) lintErrors().push_back (std::string (r.name) + ": " + err);
    return p;
}

inline std::vector<Preset> rows (const Row* r, size_t n, const char* style) { std::vector<Preset> v; for (size_t i = 0; i < n; ++i) v.push_back (make (r[i], style)); return v; }
#define ROWS(arr, style) rows (arr, sizeof (arr) / sizeof (arr[0]), style)

// =================================================================================================
//  NÚCLEO (30) — incluído, para qualquer estilo
// =================================================================================================
inline std::vector<Preset> nucleo()
{
    static const Row R[] = {
    // ---- leads
    { "Neon Saw Lead", "Lead", "lead,saw,brilhante", "nucleo", "", "",
      ANA (a, 2, 5, 22) ANA (b, 2, 3, 12) " b_coarse=12 b_level=-9 blend=0.5" FIL1E (2, 2600, 0.22, 0.35) ENV2 (2, 380, 0.3, 300) AMP (4, 420, 0.75, 260)
      CHORUS ("fxa1", 0.35, 0.4, 0.4, 0.5, 0.8, 0.35) DELAY ("fxa2", 0.6, 0.32, 0.55, 0.6, 0.5, 0.22) REVERB ("fxa3", 0.55, 0.5, 0.5, 0.1, 0.9, 0.18)
      " send_a=0.15" MOD (1, 1, 3, 0.03) " lfo1_rate=5.2 lfo1_fade=400" },
    { "Glass Lead", "Lead", "lead,vidro,wavetable", "nucleo", "", "",
      WTB (a, 4, 0.35, 3, 14) " a_level=-3" FIL1 (1, 7500, 0.1) AMP (6, 500, 0.7, 380) MOD (1, 1, 7, 0.18) " lfo1_rate=0.35 lfo1_wave=1"
      CHORUS ("fxa1", 0.25, 0.35, 0.5, 0.5, 0.9, 0.3) DELAY ("fxa2", 0.66, 0.38, 0.5, 0.7, 0.5, 0.25) REVERB ("fxa3", 0.6, 0.6, 0.4, 0.12, 1, 0.22) },
    { "Square Mono", "Lead", "lead,mono,quadrada", "nucleo", "", "",
      ANA (a, 3, 2, 8) " a_pwm=0.42" MONO (70) " glide_mode=1" FIL1E (2, 1800, 0.28, 0.45) ENV2 (1, 260, 0.25, 180) AMP (2, 300, 0.8, 160)
      MOD (1, 1, 19, 0.2) " lfo1_rate=0.8" DELAY ("fxa2", 0.54, 0.3, 0.5, 0.3, 0.5, 0.2) },
    { "Sync Scream", "Lead", "lead,sync,agressivo", "nucleo", "", "",
      ANA (a, 2, 2, 10) " a_sync=14" MOD (1, 3, 1, 0.5) ENV2 (1, 650, 0.1, 200) FIL1 (2, 5200, 0.2) AMP (2, 380, 0.65, 180)
      SATUR ("fxa1", 0.3, 0.7, 0.1, 0.4, 0.8, 0.5) DELAY ("fxa2", 0.6, 0.3, 0.5, 0.5, 0.5, 0.2) },
    { "Triângulo Suave", "Lead", "lead,suave,flauta", "nucleo", "", "",
      ANA (a, 1, 2, 9) " a_fold=0.1" FIL1 (1, 4500, 0.05) AMP (35, 400, 0.8, 340) MOD (1, 1, 3, 0.04) " lfo1_rate=5 lfo1_fade=800"
      REVERB ("fxa3", 0.55, 0.55, 0.5, 0.08, 0.9, 0.22) },
    { "Sino Digital Lead", "Lead", "lead,fm,sino", "nucleo", "", "",
      WTB (a, 5, 0.5, 2, 10) " a_level=-4" ANA (b, 0, 1, 0) " b_coarse=12 b_fmamt=0.2 b_level=-14" FIL1 (1, 9000, 0.05) AMP (1, 900, 0.3, 700)
      DELAY ("fxa2", 0.66, 0.4, 0.55, 0.6, 0.5, 0.28) REVERB ("fxa3", 0.6, 0.62, 0.35, 0.1, 1, 0.25) },
    // ---- baixos
    { "Sub Seno", "Baixo", "baixo,sub,limpo", "nucleo", "", "",
      ANA (a, 0, 1, 0) MONO (30) FIL1 (1, 900, 0.05) AMP (3, 200, 0.9, 120) SATUR ("fxa1", 0.15, 0.5, 0, 0.2, 0.8, 0.3) },
    { "Baixo Serra LP", "Baixo", "baixo,serra,filtro", "nucleo", "", "",
      ANA (a, 2, 2, 8) MONO (25) FIL1E (2, 420, 0.3, 0.5) ENV2 (1, 260, 0.1, 150) AMP (2, 240, 0.8, 110) SATUR ("fxa1", 0.3, 0.5, 0.05, 0.4, 0.8, 0.45) },
    { "808 Núcleo", "Baixo", "baixo,808,sample", "nucleo", "nucleo.808", "",
      SMP (a) MONO (45) " glide_mode=1" FIL1 (1, 6000, 0.05) AMP (1, 800, 0.9, 260) SATUR ("fxa1", 0.2, 0.6, 0, 0.5, 0.8, 0.35) },
    { "Baixo FM", "Baixo", "baixo,fm,metálico", "nucleo", "", "",
      ANA (a, 0, 1, 0) " a_fmamt=0.35 a_fmrat=2" MONO (20) MOD (1, 3, 21, 0.45) ENV2 (1, 300, 0.05, 150) FIL1 (1, 3500, 0.1) AMP (2, 260, 0.75, 130) },
    { "Reese", "Baixo", "baixo,reese,detune", "nucleo", "", "",
      ANA (a, 2, 3, 30) ANA (b, 2, 3, 36) " b_fine=7 a_fine=-7" MONO (30) FIL1 (2, 520, 0.25) AMP (5, 300, 0.85, 200)
      PHASER ("fxa2", 0.3, 0.5, 0.35, 0.4, 0.5, 0.35) },
    // ---- teclas
    { "Rhodes Núcleo", "Teclas", "teclas,rhodes,sample", "nucleo", "nucleo.ep", "",
      SMP (a) FIL1 (1, 8000, 0.05) AMP (1, 1200, 0.55, 420) CHORUS ("fxa1", 0.28, 0.4, 0.45, 0.5, 1, 0.4) REVERB ("fxa3", 0.55, 0.5, 0.5, 0.08, 0.9, 0.2) },
    { "EP Digital", "Teclas", "teclas,digital,fm", "nucleo", "", "",
      WTB (a, 13, 0.3, 1, 0) FIL1 (1, 7000, 0.05) MOD (1, 5, 7, 0.4) AMP (1, 1100, 0.4, 380) CHORUS ("fxa1", 0.3, 0.45, 0.45, 0.5, 1, 0.4) },
    { "Órgão Soul", "Teclas", "teclas,órgão", "nucleo", "", "",
      WTB (a, 2, 0.4, 1, 0) FIL1 (1, 6500, 0.05) AMP (3, 200, 0.95, 90) TREM ("fxa1", 0.35, 0.3, 0.2, 0.5, 0.5, 0.5) REVERB ("fxa3", 0.5, 0.45, 0.5, 0.05, 0.9, 0.18) },
    { "Kalimba Pluck", "Teclas", "teclas,kalimba,pluck", "nucleo", "", "",
      WTB (a, 14, 0.25, 1, 0) FIL1 (1, 8500, 0.05) AMP (1, 650, 0.0, 450) DELAY ("fxa2", 0.6, 0.35, 0.5, 0.6, 0.5, 0.3) REVERB ("fxa3", 0.55, 0.55, 0.45, 0.1, 1, 0.25) },
    { "Sino Núcleo", "Teclas", "teclas,sino,sample", "nucleo", "nucleo.bell", "",
      SMP (a) AMP (1, 1500, 0.6, 900) REVERB ("fxa3", 0.6, 0.65, 0.4, 0.1, 1, 0.3) },
    // ---- pads
    { "Aurora Pad", "Pad", "pad,etéreo,evolutivo", "nucleo", "", "",
      WTB (a, 10, 0.2, 4, 22) ANA (b, 2, 4, 20) " b_level=-12 blend=0.4" FIL1 (1, 5000, 0.12) MOD (1, 1, 7, 0.35) " lfo1_rate=0.18 lfo1_wave=1"
      MOD (2, 2, 11, 0.15) " lfo2_rate=0.11" AMP (900, 800, 0.85, 1800) CHORUS ("fxa1", 0.2, 0.5, 0.5, 0.6, 1, 0.4) REVERB ("fxa3", 0.65, 0.7, 0.45, 0.1, 1, 0.35) },
    { "Pad Vidro", "Pad", "pad,vidro,frio", "nucleo", "", "",
      WTB (a, 4, 0.5, 5, 18) FIL1 (1, 7500, 0.1) MOD (1, 1, 8, 0.2) " lfo1_rate=0.25" AMP (700, 900, 0.8, 1600) WIDTH ("fxa1", 0.7, 0.3, 0.5, 0.5, 0.55, 1) REVERB ("fxa3", 0.65, 0.7, 0.4, 0.12, 1, 0.38) },
    { "Pad Núcleo", "Pad", "pad,sample", "nucleo", "nucleo.pad", "",
      SMP (a) AMP (500, 800, 0.9, 1400) CHORUS ("fxa1", 0.22, 0.4, 0.5, 0.5, 1, 0.3) REVERB ("fxa3", 0.65, 0.68, 0.45, 0.1, 1, 0.3) },
    { "Coro Ah", "Pad", "pad,coro,voz", "nucleo", "nucleo.vox", "",
      SMP (a) AMP (350, 600, 0.9, 900) CHORUS ("fxa1", 0.2, 0.3, 0.5, 0.5, 1, 0.3) REVERB ("fxa3", 0.65, 0.7, 0.45, 0.12, 1, 0.32) },
    { "Cordas Analógicas", "Pad", "pad,cordas,quente", "nucleo", "", "",
      ANA (a, 2, 6, 24) FIL1 (2, 3200, 0.1) AMP (260, 500, 0.85, 700) CHORUS ("fxa1", 0.3, 0.55, 0.5, 0.6, 1, 0.5) REVERB ("fxa3", 0.6, 0.6, 0.5, 0.1, 1, 0.25) },
    // ---- plucks / stabs
    { "Pluck Núcleo", "Pluck", "pluck,sample", "nucleo", "nucleo.pluck", "",
      SMP (a) AMP (1, 500, 0.0, 350) DELAY ("fxa2", 0.6, 0.3, 0.5, 0.5, 0.5, 0.2) REVERB ("fxa3", 0.5, 0.45, 0.5, 0.05, 0.9, 0.16) },
    { "Pluck Sintético", "Pluck", "pluck,curto", "nucleo", "", "",
      ANA (a, 2, 3, 12) FIL1E (2, 800, 0.2, 0.55) ENV2 (1, 220, 0.0, 120) AMP (1, 320, 0.0, 200) DELAY ("fxa2", 0.6, 0.32, 0.55, 0.6, 0.5, 0.25) REVERB ("fxa3", 0.5, 0.45, 0.5, 0.05, 0.9, 0.2) },
    { "Stab Núcleo", "Pluck", "stab,sample", "nucleo", "nucleo.stab", "",
      SMP (a) AMP (1, 400, 0.0, 220) REVERB ("fxa3", 0.5, 0.45, 0.5, 0.05, 0.9, 0.18) },
    { "Arp Neon", "Arp", "arp,pluck,neon", "nucleo", "", "",
      ANA (a, 2, 3, 14) FIL1E (2, 1400, 0.25, 0.4) ENV2 (1, 200, 0.0, 120) AMP (1, 260, 0.0, 160)
      " arp_on=1 arp_rate=12 arp_mode=0 arp_oct=2 arp_gate=0.55" DELAY ("fxa2", 0.6, 0.35, 0.5, 0.7, 0.5, 0.3) REVERB ("fxa3", 0.55, 0.5, 0.5, 0.08, 1, 0.22) },
    // ---- texturas / FX
    { "Subida Núcleo", "FX", "efeito,riser,sample", "nucleo", "nucleo.riser", "", SMP (a) AMP (1, 3000, 1, 300) },
    { "Impacto Núcleo", "FX", "efeito,impacto,sample", "nucleo", "nucleo.impact", "", SMP (a) AMP (1, 3000, 1, 300) },
    { "Textura Vogal", "FX", "textura,vogal,evolutivo", "nucleo", "", "",
      WTB (a, 3, 0.1, 3, 16) FIL1 (1, 6000, 0.1) MOD (1, 1, 7, 0.45) " lfo1_rate=0.3 lfo1_wave=6" AMP (600, 600, 0.8, 1500) REVERB ("fxa3", 0.65, 0.7, 0.4, 0.1, 1, 0.4) },
    // ---- bateria
    { "Kit Núcleo", "Bateria", "bateria,kit", "nucleo", "nucleo.kit", "", SMP (a) AMP (1, 800, 1, 120) " a_loop=0 a_rr=1" },
    { "Shaker Groove", "Bateria", "bateria,shaker,arp", "nucleo", "nucleo.shaker", "",
      SMP (a) AMP (1, 600, 1, 80) " arp_on=1 arp_rate=12 arp_mode=0 arp_gate=0.5 arp_swing=0.2" },
    };
    return ROWS (R, "multi");
}

// =================================================================================================
//  TRAP (12)
// =================================================================================================
inline std::vector<Preset> trap()
{
    static const Row R[] = {
    { "808 Longo", "Baixo", "808,sub,longo", "trap", "trap.808long", "", SMP (a) MONO (45) " glide_mode=1" AMP (1, 800, 0.95, 300) SATUR ("fxa1", 0.25, 0.6, 0, 0.5, 0.8, 0.35) },
    { "808 Distorcido", "Baixo", "808,distorção,sujo", "trap", "trap.808long", "", SMP (a) MONO (40) " glide_mode=1" AMP (1, 800, 0.95, 260) SATUR ("fxa1", 0.7, 0.55, 0.1, 0.8, 0.7, 0.8) COMP ("fxa2", 0.35, 0.5, 0.3, 0.4, 0.3, 1) },
    { "808 Slide", "Baixo", "808,glide,slide", "trap", "trap.808long", "", SMP (a) MONO (160) " glide_mode=0" AMP (1, 800, 0.95, 280) SATUR ("fxa1", 0.3, 0.6, 0, 0.5, 0.8, 0.4) },
    { "808 Curto Punch", "Baixo", "808,curto,punch", "trap", "trap.808curto", "", SMP (a) MONO (30) AMP (1, 400, 0.8, 120) SATUR ("fxa1", 0.3, 0.6, 0, 0.5, 0.8, 0.4) },
    { "Sino Sombrio", "Melodia", "sino,escuro,melodia", "trap", "trap.bell", "", SMP (a) AMP (1, 1800, 0.5, 1100) DELAY ("fxa2", 0.6, 0.38, 0.45, 0.7, 0.5, 0.28) REVERB ("fxa3", 0.65, 0.7, 0.35, 0.1, 1, 0.3) },
    { "Pad Coro Trap", "Pad", "pad,coro,dark", "trap", "trap.pad", "", SMP (a) AMP (500, 800, 0.9, 1500) CHORUS ("fxa1", 0.2, 0.4, 0.5, 0.5, 1, 0.3) REVERB ("fxa3", 0.68, 0.72, 0.5, 0.12, 1, 0.38) },
    { "Stab Trap", "Melodia", "stab,staccato", "trap", "trap.stab", "", SMP (a) AMP (1, 380, 0.0, 200) REVERB ("fxa3", 0.5, 0.45, 0.5, 0.05, 0.9, 0.18) },
    { "Pluck Escuro", "Melodia", "pluck,dark", "trap", "trap.pluck", "", SMP (a) AMP (1, 500, 0.0, 300) DELAY ("fxa2", 0.6, 0.32, 0.45, 0.6, 0.5, 0.22) REVERB ("fxa3", 0.55, 0.5, 0.45, 0.08, 1, 0.22) },
    { "Voz Oh Trap", "Vozes", "voz,oh", "trap", "trap.vox", "", SMP (a) AMP (60, 600, 0.85, 800) DELAY ("fxa2", 0.66, 0.4, 0.45, 0.7, 0.5, 0.28) REVERB ("fxa3", 0.65, 0.7, 0.4, 0.12, 1, 0.32) },
    { "Kit Trap", "Bateria", "bateria,kit", "trap", "trap.kit", "", SMP (a) AMP (1, 800, 1, 120) " a_loop=0 a_rr=1" },
    { "Hi-Hat Roll", "Bateria", "hats,roll,arp", "trap", "trap.hat", "", SMP (a) AMP (1, 600, 1, 60) " arp_on=1 arp_rate=14 arp_mode=0 arp_pattern=1 arp_gate=0.6" },
    { "Subida + Impacto", "FX", "efeito,riser", "trap", "trap.riser", "trap.impact", SMP (a) SMP (b) " blend=0.5" AMP (1, 3000, 1, 300) },
    };
    return ROWS (R, "trap");
}

// =================================================================================================
//  R&B (12)
// =================================================================================================
inline std::vector<Preset> rnb()
{
    static const Row R[] = {
    { "Rhodes Lento", "Teclas", "rhodes,lento,suave", "rnb", "rnb.rhodes", "", SMP (a) AMP (1, 1400, 0.6, 500) CHORUS ("fxa1", 0.25, 0.45, 0.45, 0.5, 1, 0.45) REVERB ("fxa3", 0.6, 0.6, 0.5, 0.1, 1, 0.25) },
    { "Wurli Quente", "Teclas", "wurlitzer,quente", "rnb", "rnb.wurli", "", SMP (a) AMP (1, 1100, 0.5, 420) PHASER ("fxa1", 0.28, 0.5, 0.3, 0.4, 0.5, 0.35) REVERB ("fxa3", 0.55, 0.55, 0.5, 0.08, 0.9, 0.22) },
    { "Cordas Seda", "Pad", "cordas,seda", "rnb", "rnb.strings", "", SMP (a) AMP (350, 600, 0.9, 900) CHORUS ("fxa1", 0.2, 0.4, 0.5, 0.5, 1, 0.3) REVERB ("fxa3", 0.65, 0.68, 0.45, 0.12, 1, 0.32) },
    { "Baixo Suave", "Baixo", "baixo,suave,quente", "rnb", "rnb.bass", "", SMP (a) MONO (25) AMP (2, 600, 0.9, 200) COMP ("fxa1", 0.45, 0.4, 0.4, 0.5, 0.3, 1) },
    { "Guitarra Limpa", "Teclas", "guitarra,limpa", "rnb", "rnb.guitar", "", SMP (a) AMP (1, 800, 0.4, 300) CHORUS ("fxa1", 0.3, 0.4, 0.45, 0.5, 1, 0.35) DELAY ("fxa2", 0.6, 0.3, 0.5, 0.6, 0.5, 0.18) REVERB ("fxa3", 0.55, 0.5, 0.5, 0.08, 1, 0.22) },
    { "Coro Ooh", "Vozes", "voz,ooh,pad", "rnb", "rnb.voxoo", "", SMP (a) AMP (300, 600, 0.9, 900) REVERB ("fxa3", 0.65, 0.7, 0.4, 0.12, 1, 0.34) },
    { "Voz Ah Suave", "Vozes", "voz,ah", "rnb", "rnb.vox", "", SMP (a) AMP (120, 600, 0.9, 700) REVERB ("fxa3", 0.62, 0.65, 0.45, 0.1, 1, 0.3) },
    { "Sino Brilho", "Melodia", "sino,brilho", "rnb", "rnb.bell", "", SMP (a) AMP (1, 1500, 0.5, 900) DELAY ("fxa2", 0.66, 0.35, 0.5, 0.6, 0.5, 0.25) REVERB ("fxa3", 0.62, 0.65, 0.4, 0.1, 1, 0.3) },
    { "Kit R&B", "Bateria", "bateria,kit", "rnb", "rnb.kit", "", SMP (a) AMP (1, 800, 1, 120) " a_loop=0 a_rr=1" },
    { "Arp R&B Quebrado", "Arp", "arp,rhodes,groove", "rnb", "rnb.rhodes", "", SMP (a) AMP (1, 700, 0.3, 300) " arp_on=1 arp_rate=12 arp_mode=0 arp_pattern=2 arp_gate=0.55 arp_swing=0.25"
      CHORUS ("fxa1", 0.28, 0.4, 0.45, 0.5, 1, 0.4) REVERB ("fxa3", 0.55, 0.55, 0.5, 0.1, 1, 0.22) },
    { "Shaker Swing", "Bateria", "shaker,swing", "rnb", "rnb.shaker", "", SMP (a) AMP (1, 600, 1, 80) " arp_on=1 arp_rate=12 arp_mode=0 arp_gate=0.5 arp_swing=0.3" },
    { "Rhodes Acordes", "Teclas", "rhodes,acordes,maj7", "rnb", "rnb.rhodes", "", SMP (a) " chord_type=5" AMP (1, 1400, 0.6, 500) CHORUS ("fxa1", 0.25, 0.45, 0.45, 0.5, 1, 0.45) REVERB ("fxa3", 0.6, 0.6, 0.5, 0.1, 1, 0.26) },
    };
    return ROWS (R, "rnb");
}

// =================================================================================================
//  AFROBEATS (12)
// =================================================================================================
inline std::vector<Preset> afrobeat()
{
    static const Row R[] = {
    { "Kit Afro", "Bateria", "bateria,kit", "afrobeat", "afrobeat.kit", "", SMP (a) AMP (1, 800, 1, 120) " a_loop=0 a_rr=1" },
    { "Conga Groove", "Percussão", "conga,groove,arp", "afrobeat", "afrobeat.congahi", "", SMP (a) AMP (1, 700, 1, 90) " arp_on=1 arp_rate=12 arp_mode=0 arp_pattern=3 arp_gate=0.55 arp_swing=0.15" },
    { "Talking Drum", "Percussão", "tambor,talking", "afrobeat", "afrobeat.talking", "", SMP (a) MONO (90) " glide_mode=0" AMP (1, 800, 0.9, 200) REVERB ("fxa3", 0.5, 0.45, 0.5, 0.05, 0.9, 0.15) },
    { "Guitarra Highlife", "Teclas", "guitarra,highlife", "afrobeat", "afrobeat.guitar", "", SMP (a) AMP (1, 700, 0.35, 260) " arp_on=1 arp_rate=12 arp_mode=0 arp_pattern=3 arp_gate=0.5"
      DELAY ("fxa2", 0.6, 0.3, 0.5, 0.6, 0.5, 0.18) REVERB ("fxa3", 0.5, 0.45, 0.5, 0.06, 0.9, 0.18) },
    { "Baixo Afro", "Baixo", "baixo,afro", "afrobeat", "afrobeat.bass", "", SMP (a) MONO (20) AMP (2, 500, 0.9, 160) COMP ("fxa1", 0.45, 0.4, 0.4, 0.5, 0.3, 1) },
    { "Kalimba Líder", "Melodia", "kalimba,melodia", "afrobeat", "afrobeat.kalimba", "", SMP (a) AMP (1, 900, 0.3, 500) DELAY ("fxa2", 0.6, 0.35, 0.5, 0.6, 0.5, 0.25) REVERB ("fxa3", 0.55, 0.55, 0.45, 0.08, 1, 0.25) },
    { "Pad Afro", "Pad", "pad,quente", "afrobeat", "afrobeat.pad", "", SMP (a) AMP (400, 700, 0.9, 1100) CHORUS ("fxa1", 0.2, 0.4, 0.5, 0.5, 1, 0.3) REVERB ("fxa3", 0.62, 0.65, 0.5, 0.1, 1, 0.3) },
    { "Coro Chant", "Vozes", "voz,coro,chant", "afrobeat", "afrobeat.vox", "", SMP (a) AMP (80, 600, 0.9, 600) REVERB ("fxa3", 0.6, 0.6, 0.45, 0.1, 1, 0.28) },
    { "Metais Stab", "Melodia", "metais,stab", "afrobeat", "afrobeat.horn", "", SMP (a) AMP (4, 400, 0.0, 200) REVERB ("fxa3", 0.5, 0.45, 0.5, 0.05, 0.9, 0.18) },
    { "Apito", "FX", "apito,efeito", "afrobeat", "afrobeat.whistle", "", SMP (a) AMP (1, 800, 1, 100) },
    { "Bongo Roll", "Percussão", "bongo,roll", "afrobeat", "afrobeat.bongo", "", SMP (a) AMP (1, 600, 1, 80) " arp_on=1 arp_rate=13 arp_mode=4 arp_gate=0.5" },
    { "Shaker Afro", "Percussão", "shaker,groove", "afrobeat", "afrobeat.shaker", "", SMP (a) AMP (1, 600, 1, 80) " arp_on=1 arp_rate=12 arp_mode=0 arp_pattern=3 arp_gate=0.5 arp_swing=0.2" },
    };
    return ROWS (R, "afrobeat");
}

// =================================================================================================
//  AMAPIANO (12)
// =================================================================================================
inline std::vector<Preset> amapiano()
{
    static const Row R[] = {
    { "Log Drum Profundo", "Baixo", "logdrum,profundo", "amapiano", "amapiano.logdrum", "", SMP (a) MONO (35) " glide_mode=1" AMP (1, 800, 0.9, 250) SATUR ("fxa1", 0.2, 0.55, 0, 0.4, 0.8, 0.3) },
    { "Log Drum Curto", "Baixo", "logdrum,curto", "amapiano", "amapiano.logshort", "", SMP (a) MONO (20) AMP (1, 500, 0.7, 150) SATUR ("fxa1", 0.2, 0.55, 0, 0.4, 0.8, 0.3) },
    { "Kit Amapiano", "Bateria", "bateria,kit", "amapiano", "amapiano.kit", "", SMP (a) AMP (1, 800, 1, 120) " a_loop=0 a_rr=1" },
    { "Piano Soul", "Teclas", "piano,soul,acordes", "amapiano", "amapiano.piano", "", SMP (a) " chord_type=6" AMP (1, 1500, 0.5, 500) REVERB ("fxa3", 0.6, 0.6, 0.5, 0.1, 1, 0.26) },
    { "Teclas Jazzy", "Teclas", "teclas,jazz", "amapiano", "amapiano.keys", "", SMP (a) AMP (1, 1100, 0.5, 400) CHORUS ("fxa1", 0.28, 0.4, 0.45, 0.5, 1, 0.35) REVERB ("fxa3", 0.55, 0.55, 0.5, 0.08, 1, 0.22) },
    { "Pad Amapiano", "Pad", "pad,ambiente", "amapiano", "amapiano.pad", "", SMP (a) AMP (500, 800, 0.9, 1400) CHORUS ("fxa1", 0.2, 0.4, 0.5, 0.5, 1, 0.3) REVERB ("fxa3", 0.66, 0.7, 0.5, 0.12, 1, 0.36) },
    { "Chop Ah", "Vozes", "voz,chop,ah", "amapiano", "amapiano.chopah", "", SMP (a) AMP (1, 700, 0.8, 300) DELAY ("fxa2", 0.6, 0.3, 0.5, 0.6, 0.5, 0.2) REVERB ("fxa3", 0.55, 0.5, 0.45, 0.08, 1, 0.22) },
    { "Chop Oh", "Vozes", "voz,chop,oh", "amapiano", "amapiano.chopoh", "", SMP (a) AMP (1, 700, 0.8, 300) DELAY ("fxa2", 0.6, 0.3, 0.5, 0.6, 0.5, 0.2) REVERB ("fxa3", 0.55, 0.5, 0.45, 0.08, 1, 0.22) },
    { "Baixo Amapiano", "Baixo", "baixo,sub", "amapiano", "amapiano.bass", "", SMP (a) MONO (25) AMP (2, 500, 0.9, 180) COMP ("fxa1", 0.45, 0.4, 0.4, 0.5, 0.3, 1) },
    { "Shaker Groove", "Percussão", "shaker,groove,arp", "amapiano", "amapiano.shaker", "", SMP (a) AMP (1, 600, 1, 80) " arp_on=1 arp_rate=12 arp_mode=0 arp_pattern=4 arp_gate=0.5 arp_swing=0.2" },
    { "Apito Amapiano", "FX", "apito,efeito", "amapiano", "amapiano.whistle", "", SMP (a) AMP (1, 800, 1, 100) },
    { "Subida", "FX", "efeito,riser", "amapiano", "amapiano.riser", "amapiano.impact", SMP (a) SMP (b) " blend=0.5" AMP (1, 3000, 1, 300) },
    };
    return ROWS (R, "amapiano");
}

// =================================================================================================
//  DRILL (10)
// =================================================================================================
inline std::vector<Preset> drill()
{
    static const Row R[] = {
    { "808 Deslizante", "Baixo", "808,slide,drill", "drill", "drill.808slide", "", SMP (a) MONO (180) " glide_mode=0" AMP (1, 800, 0.95, 280) SATUR ("fxa1", 0.35, 0.6, 0, 0.5, 0.8, 0.45) },
    { "808 Curto Drill", "Baixo", "808,curto", "drill", "drill.808curto", "", SMP (a) MONO (30) AMP (1, 400, 0.8, 120) SATUR ("fxa1", 0.3, 0.6, 0, 0.5, 0.8, 0.4) },
    { "Sino Drill", "Melodia", "sino,frio,sombrio", "drill", "drill.bell", "", SMP (a) AMP (1, 1800, 0.5, 1000) DELAY ("fxa2", 0.6, 0.38, 0.45, 0.7, 0.5, 0.28) REVERB ("fxa3", 0.66, 0.7, 0.35, 0.1, 1, 0.3) },
    { "Pad Sombrio", "Pad", "pad,dark", "drill", "drill.pad", "", SMP (a) AMP (600, 800, 0.9, 1600) REVERB ("fxa3", 0.68, 0.72, 0.5, 0.12, 1, 0.38) },
    { "Pluck Drill", "Melodia", "pluck,melodia", "drill", "drill.pluck", "", SMP (a) AMP (1, 500, 0.0, 300) DELAY ("fxa2", 0.6, 0.32, 0.45, 0.6, 0.5, 0.22) REVERB ("fxa3", 0.55, 0.5, 0.45, 0.08, 1, 0.22) },
    { "Voz Drill", "Vozes", "voz", "drill", "drill.vox", "", SMP (a) AMP (60, 600, 0.85, 800) DELAY ("fxa2", 0.66, 0.4, 0.45, 0.7, 0.5, 0.28) REVERB ("fxa3", 0.65, 0.7, 0.4, 0.12, 1, 0.32) },
    { "Kit Drill", "Bateria", "bateria,kit", "drill", "drill.kit", "", SMP (a) AMP (1, 800, 1, 120) " a_loop=0 a_rr=1" },
    { "Hats Slide", "Bateria", "hats,slide,arp", "drill", "drill.hat", "", SMP (a) AMP (1, 600, 1, 60) " arp_on=1 arp_rate=13 arp_mode=0 arp_pattern=5 arp_gate=0.55" },
    { "Subida Drill", "FX", "efeito,riser", "drill", "drill.riser", "", SMP (a) AMP (1, 3000, 1, 300) },
    { "Impacto Drill", "FX", "efeito,impacto", "drill", "drill.impact", "", SMP (a) AMP (1, 3000, 1, 300) },
    };
    return ROWS (R, "drill");
}

// =================================================================================================
//  KIZOMBA · ZOUK (12)
// =================================================================================================
inline std::vector<Preset> kizomba()
{
    static const Row R[] = {
    { "Baixo Zouk", "Baixo", "baixo,guitarra,zouk", "kizomba", "kizomba.bass", "", SMP (a) MONO (20) AMP (2, 600, 0.9, 200) COMP ("fxa1", 0.45, 0.4, 0.4, 0.5, 0.3, 1) },
    { "EP Kizomba", "Teclas", "piano elétrico,kizomba", "kizomba", "kizomba.ep", "", SMP (a) AMP (1, 1400, 0.55, 500) CHORUS ("fxa1", 0.25, 0.45, 0.45, 0.5, 1, 0.45) REVERB ("fxa3", 0.6, 0.6, 0.5, 0.1, 1, 0.25) },
    { "Guitarra Kizomba", "Teclas", "guitarra,limpa", "kizomba", "kizomba.guitar", "", SMP (a) AMP (1, 800, 0.4, 300) CHORUS ("fxa1", 0.3, 0.4, 0.45, 0.5, 1, 0.35) DELAY ("fxa2", 0.6, 0.3, 0.5, 0.6, 0.5, 0.18) REVERB ("fxa3", 0.55, 0.5, 0.5, 0.08, 1, 0.22) },
    { "Cordas Kizomba", "Pad", "cordas,romântico", "kizomba", "kizomba.strings", "", SMP (a) AMP (350, 600, 0.9, 900) CHORUS ("fxa1", 0.2, 0.4, 0.5, 0.5, 1, 0.3) REVERB ("fxa3", 0.65, 0.68, 0.45, 0.12, 1, 0.32) },
    { "Voz Zouk", "Vozes", "voz,zouk", "kizomba", "kizomba.vox", "", SMP (a) AMP (120, 600, 0.9, 700) REVERB ("fxa3", 0.62, 0.65, 0.45, 0.1, 1, 0.3) },
    { "Sino Zouk", "Melodia", "sino,zouk", "kizomba", "kizomba.bell", "", SMP (a) AMP (1, 1500, 0.5, 900) DELAY ("fxa2", 0.66, 0.35, 0.5, 0.6, 0.5, 0.25) REVERB ("fxa3", 0.62, 0.65, 0.4, 0.1, 1, 0.3) },
    { "Kit Kizomba", "Bateria", "bateria,kit", "kizomba", "kizomba.kit", "", SMP (a) AMP (1, 800, 1, 120) " a_loop=0 a_rr=1" },
    { "Conga Zouk", "Percussão", "conga,zouk,arp", "kizomba", "kizomba.conga", "", SMP (a) AMP (1, 700, 1, 90) " arp_on=1 arp_rate=12 arp_mode=0 arp_pattern=6 arp_gate=0.5 arp_swing=0.1" },
    { "Guiro", "Percussão", "guiro,ritmo", "kizomba", "kizomba.guiro", "", SMP (a) AMP (1, 600, 1, 80) " arp_on=1 arp_rate=12 arp_mode=0 arp_pattern=6 arp_gate=0.5" },
    { "EP Acordes Maj7", "Teclas", "ep,acordes,maj7", "kizomba", "kizomba.ep", "", SMP (a) " chord_type=5" AMP (1, 1400, 0.55, 500) CHORUS ("fxa1", 0.25, 0.45, 0.45, 0.5, 1, 0.45) REVERB ("fxa3", 0.6, 0.6, 0.5, 0.1, 1, 0.26) },
    { "Subida Zouk", "FX", "efeito,riser", "kizomba", "kizomba.riser", "", SMP (a) AMP (1, 3000, 1, 300) },
    { "Pluck Zouk", "Melodia", "pluck,guitarra", "kizomba", "kizomba.guitar", "", SMP (a) AMP (1, 450, 0.0, 280) DELAY ("fxa2", 0.6, 0.32, 0.5, 0.6, 0.5, 0.2) REVERB ("fxa3", 0.55, 0.5, 0.5, 0.08, 1, 0.22) },
    };
    return ROWS (R, "kizomba");
}

// =================================================================================================
//  KUDURO (10)
// =================================================================================================
inline std::vector<Preset> kuduro()
{
    static const Row R[] = {
    { "Kit Kuduro", "Bateria", "bateria,kit", "kuduro", "kuduro.kit", "", SMP (a) AMP (1, 800, 1, 120) " a_loop=0 a_rr=1" },
    { "Stab Kuduro", "Melodia", "stab,serra", "kuduro", "kuduro.stab", "", SMP (a) AMP (1, 380, 0.0, 200) SATUR ("fxa1", 0.3, 0.7, 0, 0.5, 0.8, 0.4) },
    { "Baixo Agressivo", "Baixo", "baixo,agressivo", "kuduro", "kuduro.bass", "", SMP (a) MONO (20) AMP (1, 500, 0.9, 140) SATUR ("fxa1", 0.5, 0.6, 0.05, 0.6, 0.75, 0.6) },
    { "Ácido Kuduro", "Melodia", "ácido,pluck", "kuduro", "kuduro.acid", "", SMP (a) AMP (1, 350, 0.0, 180) SATUR ("fxa1", 0.4, 0.7, 0, 0.6, 0.8, 0.5) DELAY ("fxa2", 0.6, 0.3, 0.5, 0.5, 0.5, 0.2) },
    { "Sirene", "FX", "sirene,efeito", "kuduro", "kuduro.siren", "", SMP (a) AMP (20, 800, 0.9, 300) REVERB ("fxa3", 0.5, 0.45, 0.5, 0.05, 0.9, 0.15) },
    { "Grito Hey", "Vozes", "voz,hey,grito", "kuduro", "kuduro.shout", "", SMP (a) AMP (1, 500, 0.5, 200) REVERB ("fxa3", 0.5, 0.45, 0.5, 0.05, 0.9, 0.18) },
    { "Apito Kuduro", "FX", "apito", "kuduro", "kuduro.whistle", "", SMP (a) AMP (1, 800, 1, 100) },
    { "Subida Kuduro", "FX", "efeito,riser", "kuduro", "kuduro.riser", "", SMP (a) AMP (1, 3000, 1, 300) },
    { "Impacto Kuduro", "FX", "efeito,impacto", "kuduro", "kuduro.impact", "", SMP (a) AMP (1, 3000, 1, 300) },
    { "Arp Kuduro", "Arp", "arp,stab,kuduro", "kuduro", "kuduro.stab", "", SMP (a) AMP (1, 300, 0.0, 160) " arp_on=1 arp_rate=13 arp_mode=0 arp_pattern=7 arp_gate=0.5" SATUR ("fxa1", 0.3, 0.7, 0, 0.5, 0.8, 0.4) },
    };
    return ROWS (R, "kuduro");
}

#include "PresetsMore.inc"
#include "PresetsKits.inc"

inline std::vector<Preset> forPackBase (const std::string& id)
{
    auto cat = [] (std::vector<Preset> a, std::vector<Preset> b) { a.insert (a.end(), std::make_move_iterator (b.begin()), std::make_move_iterator (b.end())); return a; };
    if (id == "nucleo") return cat (nucleo(), nucleoMore());
    if (id == "trap") return cat (trap(), trapMore());
    if (id == "rnb") return cat (rnb(), rnbMore());
    if (id == "afrobeat") return cat (afrobeat(), afrobeatMore());
    if (id == "amapiano") return cat (amapiano(), amapianoMore());
    if (id == "drill") return cat (drill(), drillMore());
    if (id == "kizomba") return cat (kizomba(), kizombaMore());
    if (id == "kuduro") return cat (kuduro(), kuduroMore());
    return {};
}
inline std::vector<Preset> forPack (const std::string& id)
{
    auto v = forPackBase (id);
    for (auto* more : { &kitVariants, &extrasMore })
    { auto x = (*more) (id); v.insert (v.end(), std::make_move_iterator (x.begin()), std::make_move_iterator (x.end())); }
    return v;
}
}} // namespace sk::presets

#undef AMP
#undef ENV2
#undef ENV3
#undef FIL1
#undef FIL1E
#undef FIL2
#undef FX
#undef REVERB
#undef DELAY
#undef CHORUS
#undef SATUR
#undef COMP
#undef EQ3
#undef PHASER
#undef CRUSH
#undef WIDTH
#undef TREM
#undef MOD
#undef ANA
#undef WTB
#undef SMP
#undef MONO
#undef LEGATO
#undef ROWS
