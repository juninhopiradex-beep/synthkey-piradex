// =============================================================================
//  SYNTHKEY by Piradex — tabela única de parâmetros (ids estáveis!)
//
//  Esta tabela gera: o enum P_*, os metadados (nome, intervalo, valor por
//  defeito, tipo) e o layout do AudioProcessorValueTreeState. Os ids
//  (segunda coluna) NUNCA devem mudar depois do lançamento: são o que os
//  hosts guardam nos projetos e o que os presets referenciam.
//  Valores no motor: sempre "reais" (dB, Hz, ms, índice de escolha...).
// =============================================================================
#pragma once
#include <cstdint>

namespace sk
{
enum ParamKind : uint8_t { PK_FLOAT = 0, PK_LOG, PK_INT, PK_BOOL, PK_CHOICE };

struct ParamInfo
{
    const char* id;
    const char* name;
    ParamKind   kind;
    float       mn, mx, def;
    const char* unit;      // "dB","Hz","ms","%","st","ct","" ...
    const char* choices;   // "A|B|C" ou "@MODSRC" / "@MODDST"
};

// ---- origens e destinos de modulação (ordem = valor guardado nos presets) ----
enum ModSrc : int
{
    MS_NONE = 0, MS_LFO1, MS_LFO2, MS_ENV2, MS_ENV3, MS_VEL, MS_KEY, MS_MODWHEEL, MS_AFTERTOUCH,
    MS_PITCHBEND, MS_RANDOM, MS_MACRO1, MS_MACRO2, MS_MACRO3, MS_MACRO4, MS_MACRO5, MS_MACRO6,
    MS_MACRO7, MS_MACRO8, MS_ENVAMP, MS_CONST, MS_COUNT
};
inline const char* const kModSrcNames[MS_COUNT] = {
    "Nenhuma", "LFO 1", "LFO 2", "Env 2", "Env 3", "Velocidade", "Tecla", "Mod Wheel", "Aftertouch",
    "Pitch Bend", "Aleatório", "Macro 1", "Macro 2", "Macro 3", "Macro 4", "Macro 5", "Macro 6",
    "Macro 7", "Macro 8", "Env Amp", "Constante" };

enum ModDst : int
{
    MD_NONE = 0, MD_PITCH_A, MD_PITCH_B, MD_PITCH_ALL, MD_LEVEL_A, MD_LEVEL_B, MD_PAN, MD_WTPOS_A,
    MD_WTPOS_B, MD_WARP_A, MD_WARP_B, MD_CUT1, MD_CUT2, MD_RES1, MD_RES2, MD_DRIVE1, MD_DRIVE2,
    MD_UNIDET_A, MD_UNIDET_B, MD_PWM_A, MD_PWM_B, MD_FM_A, MD_FM_B, MD_START_A, MD_START_B,
    MD_SUB, MD_NOISE, MD_LFO1RATE, MD_LFO2RATE, MD_AMP, MD_FOLD_A, MD_FOLD_B, MD_FILTBAL,
    MD_AUXRET, MD_SENDA, MD_SENDB, MD_COUNT
};
inline const char* const kModDstNames[MD_COUNT] = {
    "Nenhum", "Afinação A", "Afinação B", "Afinação Geral", "Nível A", "Nível B", "Pan",
    "Pos. Wavetable A", "Pos. Wavetable B", "Warp A", "Warp B", "Corte F1", "Corte F2", "Ress. F1",
    "Ress. F2", "Drive F1", "Drive F2", "Detune Unisson A", "Detune Unisson B", "PWM A", "PWM B",
    "FM A", "FM B", "Início Sample A", "Início Sample B", "Nível Sub", "Nível Ruído", "Taxa LFO 1",
    "Taxa LFO 2", "Amplitude", "Dobra A", "Dobra B", "Balanço F1-F2", "Retorno Aux", "Send A", "Send B" };

inline constexpr int kNumWavetables = 16;
inline constexpr int kNumModSlots   = 16;
inline constexpr int kNumMacros     = 8;
inline constexpr int kNumFxSlots    = 9;   // A1-A3, B1-B3, X1-X3 (aux)

// Divisões de tempo (LFO / Arp): multiplicador em "batidas" (1 = semínima)
inline const char* const kDivNames[] = { "4/1", "2/1", "1/1", "1/2", "1/2T", "1/4.", "1/4", "1/4T", "1/8.", "1/8", "1/8T", "1/16.", "1/16", "1/16T", "1/32", "1/32T" };
inline const float kDivBeats[] = { 16.f, 8.f, 4.f, 2.f, 4.f / 3.f, 1.5f, 1.f, 2.f / 3.f, 0.75f, 0.5f, 1.f / 3.f, 0.375f, 0.25f, 1.f / 6.f, 0.125f, 1.f / 12.f };
inline constexpr int kNumDivs = 16;

#define SK_DIVLIST "4/1|2/1|1/1|1/2|1/2T|1/4.|1/4|1/4T|1/8.|1/8|1/8T|1/16.|1/16|1/16T|1/32|1/32T"
#define SK_WTNAMES "Básica|Harmónicos|Órgão|Vogais|Coro de Vidro|Sinos Digitais|FM Digital|PWM|Dobra|Pente|Pads Etéreos|Digital Duro|Sub Baixo|Piano Elétrico|Kalimba|Textura"
#define SK_FILTERTYPES "Desligado|Passa-baixo 12|Passa-baixo 24|Passa-alto 12|Passa-alto 24|Passa-banda|Notch|Ladder|Pente"
#define SK_FXTYPES "Vazio|EQ|Compressor|Saturador|Chorus|Delay|Reverb|Faser|Bitcrusher|Largura|Trémolo"
#define SK_CHORDS "Desligado|Oitava|Quinta|Maior|Menor|Maj7|Min7|Dom7|Maj9|Min9|Sus2|Sus4|Diatónico 3|Diatónico 7|Diatónico 9"
#define SK_SCALES "Desligada|Maior|Menor natural|Dórico|Frígio|Lídio|Mixolídio|Menor harmónica|Pentatónica maior|Pentatónica menor|Blues"
#define SK_KEYS "Dó|Dó#|Ré|Ré#|Mi|Fá|Fá#|Sol|Sol#|Lá|Lá#|Si"
#define SK_ARPPATTERNS "Liso|Trap Hi-Hat|R&B Quebrado|Afrobeat Sincopado|Amapiano Groove|Drill Slides|Kizomba Zouk|Kuduro"

// ----------------------------- grupos repetidos -------------------------------
#define SK_ENGINE(X, E, id, NM, onDef, typeDef) \
    X(E##_ON,     id "_on",     "Motor " NM " Ativo",            PK_BOOL,   0, 1, onDef, "", "") \
    X(E##_TYPE,   id "_type",   "Motor " NM " Tipo",             PK_CHOICE, 0, 2, typeDef, "", "Analógico|Wavetable|Sample") \
    X(E##_LEVEL,  id "_level",  "Motor " NM " Nível",            PK_FLOAT,  -60, 6, 0, "dB", "") \
    X(E##_PAN,    id "_pan",    "Motor " NM " Pan",              PK_FLOAT,  -1, 1, 0, "", "") \
    X(E##_COARSE, id "_coarse", "Motor " NM " Afinação",         PK_INT,    -36, 36, 0, "st", "") \
    X(E##_FINE,   id "_fine",   "Motor " NM " Afinação Fina",    PK_FLOAT,  -100, 100, 0, "ct", "") \
    X(E##_UNI,    id "_uni",    "Motor " NM " Unisson",          PK_INT,    1, 8, 1, "", "") \
    X(E##_UNIDET, id "_unidet", "Motor " NM " Detune Unisson",   PK_FLOAT,  0, 100, 20, "ct", "") \
    X(E##_UNISPR, id "_unispr", "Motor " NM " Espalhar Unisson", PK_FLOAT,  0, 1, 0.6f, "%", "") \
    X(E##_PHASE,  id "_phase",  "Motor " NM " Fase",             PK_FLOAT,  0, 1, 0, "%", "") \
    X(E##_RETRIG, id "_retrig", "Motor " NM " Reiniciar Fase",   PK_BOOL,   0, 1, 1, "", "") \
    X(E##_WAVE,   id "_wave",   "Motor " NM " Onda",             PK_CHOICE, 0, 3, 2, "", "Seno|Triângulo|Serra|Pulso") \
    X(E##_PWM,    id "_pwm",    "Motor " NM " Largura Pulso",    PK_FLOAT,  0.02f, 0.98f, 0.5f, "%", "") \
    X(E##_SYNC,   id "_sync",   "Motor " NM " Sync",             PK_FLOAT,  0, 48, 0, "st", "") \
    X(E##_FMAMT,  id "_fmamt",  "Motor " NM " FM",               PK_FLOAT,  0, 1, 0, "%", "") \
    X(E##_FMRAT,  id "_fmrat",  "Motor " NM " Razão FM",         PK_CHOICE, 0, 7, 2, "", "0.5|1|2|3|4|5|6|8") \
    X(E##_WTTAB,  id "_wttab",  "Motor " NM " Tabela",           PK_CHOICE, 0, 15, 0, "", SK_WTNAMES) \
    X(E##_WTPOS,  id "_wtpos",  "Motor " NM " Posição",          PK_FLOAT,  0, 1, 0, "%", "") \
    X(E##_WARP,   id "_warp",   "Motor " NM " Warp",             PK_FLOAT,  -1, 1, 0, "", "") \
    X(E##_FOLD,   id "_fold",   "Motor " NM " Dobra",            PK_FLOAT,  0, 1, 0, "%", "") \
    X(E##_START,  id "_start",  "Motor " NM " Início Sample",    PK_FLOAT,  0, 1, 0, "%", "") \
    X(E##_LOOP,   id "_loop",   "Motor " NM " Loop",             PK_BOOL,   0, 1, 1, "", "") \
    X(E##_REV,    id "_rev",    "Motor " NM " Reverso",          PK_BOOL,   0, 1, 0, "", "") \
    X(E##_RR,     id "_rr",     "Motor " NM " Round-Robin",      PK_BOOL,   0, 1, 1, "", "") \
    X(E##_TRACK,  id "_track",  "Motor " NM " Seguir Tecla",     PK_BOOL,   0, 1, 1, "", "") \
    X(E##_SMP,    id "_smp",    "Motor " NM " Instrumento",      PK_INT,    0, 4095, 0, "", "")

#define SK_FILTER(X, E, id, NM, typeDef, cutDef) \
    X(E##_TYPE,  id "_type",  "Filtro " NM " Tipo",       PK_CHOICE, 0, 8, typeDef, "", SK_FILTERTYPES) \
    X(E##_CUT,   id "_cut",   "Filtro " NM " Corte",      PK_LOG,    20, 20000, cutDef, "Hz", "") \
    X(E##_RES,   id "_res",   "Filtro " NM " Ressonância", PK_FLOAT, 0, 1, 0.1f, "%", "") \
    X(E##_DRIVE, id "_drive", "Filtro " NM " Drive",      PK_FLOAT,  0, 1, 0, "%", "") \
    X(E##_KEY,   id "_key",   "Filtro " NM " Keytrack",   PK_FLOAT,  -1, 1, 0, "%", "") \
    X(E##_ENV,   id "_env",   "Filtro " NM " Qtd Env",    PK_FLOAT,  -1, 1, 0, "%", "")

#define SK_ENV(X, E, id, NM, aDef, dDef, sDef, rDef) \
    X(E##_A, id "_a", NM " Ataque",    PK_LOG,   0.1f, 10000, aDef, "ms", "") \
    X(E##_D, id "_d", NM " Decaimento", PK_LOG,  1, 10000, dDef, "ms", "") \
    X(E##_S, id "_s", NM " Sustain",   PK_FLOAT, 0, 1, sDef, "%", "") \
    X(E##_R, id "_r", NM " Libertação", PK_LOG,  1, 15000, rDef, "ms", "")

#define SK_LFO(X, E, id, NM) \
    X(E##_WAVE,  id "_wave",  NM " Onda",     PK_CHOICE, 0, 6, 0, "", "Seno|Triângulo|Serra Cima|Serra Baixo|Quadrada|S&H|Aleatório Suave") \
    X(E##_RATE,  id "_rate",  NM " Taxa",     PK_LOG,    0.02f, 40, 2, "Hz", "") \
    X(E##_SYNC,  id "_sync",  NM " Sync",     PK_BOOL,   0, 1, 0, "", "") \
    X(E##_DIV,   id "_div",   NM " Divisão",  PK_CHOICE, 0, 15, 9, "", SK_DIVLIST) \
    X(E##_PHASE, id "_phase", NM " Fase",     PK_FLOAT,  0, 1, 0, "%", "") \
    X(E##_FADE,  id "_fade",  NM " Fade-in",  PK_LOG,    1, 5000, 1, "ms", "") \
    X(E##_KEY,   id "_key",   NM " Reiniciar na tecla", PK_BOOL, 0, 1, 1, "", "")

#define SK_MOD(X, n) \
    X(MOD##n##_SRC, "mod" #n "_src", "Mod " #n " Origem",  PK_CHOICE, 0, 20, 0, "", "@MODSRC") \
    X(MOD##n##_DST, "mod" #n "_dst", "Mod " #n " Destino", PK_CHOICE, 0, 35, 0, "", "@MODDST") \
    X(MOD##n##_AMT, "mod" #n "_amt", "Mod " #n " Quantidade", PK_FLOAT, -1, 1, 0, "%", "")

#define SK_ARPSTEP(X, n) \
    X(ARP_V##n, "arp_v" #n, "Arp Passo " #n " Nível",     PK_FLOAT, 0, 1, 1, "%", "") \
    X(ARP_O##n, "arp_o" #n, "Arp Passo " #n " Oitava",    PK_INT,  -1, 1, 0, "", "") \
    X(ARP_R##n, "arp_r" #n, "Arp Passo " #n " Repetições", PK_INT,   0, 2, 0, "", "")

#define SK_FXSLOT(X, E, id, NM) \
    X(E##_TYPE, id "_type", "FX " NM " Tipo",    PK_CHOICE, 0, 10, 0, "", SK_FXTYPES) \
    X(E##_P1,   id "_p1",   "FX " NM " Param 1", PK_FLOAT, 0, 1, 0.5f, "", "") \
    X(E##_P2,   id "_p2",   "FX " NM " Param 2", PK_FLOAT, 0, 1, 0.5f, "", "") \
    X(E##_P3,   id "_p3",   "FX " NM " Param 3", PK_FLOAT, 0, 1, 0.5f, "", "") \
    X(E##_P4,   id "_p4",   "FX " NM " Param 4", PK_FLOAT, 0, 1, 0.5f, "", "") \
    X(E##_P5,   id "_p5",   "FX " NM " Param 5", PK_FLOAT, 0, 1, 0.5f, "", "") \
    X(E##_MIX,  id "_mix",  "FX " NM " Mix",     PK_FLOAT, 0, 1, 1, "%", "")

#define SK_MACRO(X, n) X(MACRO##n, "macro" #n, "Macro " #n, PK_FLOAT, 0, 1, 0, "%", "")

// ------------------------------ tabela completa ---------------------------------
#define SK_PARAMS(X) \
    /* globais */ \
    X(MASTER_VOL, "master_vol", "Volume Master", PK_FLOAT, -60, 6, 0, "dB", "") \
    X(VOICE_MODE, "voice_mode", "Modo de Voz", PK_CHOICE, 0, 2, 0, "", "Poli|Mono|Legato") \
    X(POLYPHONY,  "polyphony",  "Polifonia", PK_INT, 1, 32, 16, "", "") \
    X(GLIDE,      "glide",      "Glide", PK_LOG, 1, 2000, 1, "ms", "") \
    X(GLIDE_MODE, "glide_mode", "Modo Glide", PK_CHOICE, 0, 1, 1, "", "Sempre|Legato") \
    X(PB_RANGE,   "pb_range",   "Pitch Bend", PK_INT, 0, 24, 2, "st", "") \
    X(TUNE,       "tune",       "Afinação Master", PK_FLOAT, -100, 100, 0, "ct", "") \
    X(TRANSPOSE,  "transpose",  "Transposição", PK_INT, -24, 24, 0, "st", "") \
    X(VEL_SENS,   "vel_sens",   "Sensib. Velocidade", PK_FLOAT, 0, 1, 0.7f, "%", "") \
    X(BLEND,      "blend",      "Mistura A-B", PK_FLOAT, 0, 1, 0.5f, "%", "") \
    X(OUT_BUS,    "out_bus",    "Saída Filtros", PK_CHOICE, 0, 2, 0, "", "Insert A|Insert B|A+B") \
    X(LIM_ON,     "lim_on",     "Limitador", PK_BOOL, 0, 1, 1, "", "") \
    X(LIM_CEIL,   "lim_ceil",   "Teto do Limitador", PK_FLOAT, -12, 0, -0.3f, "dB", "") \
    /* motores */ \
    SK_ENGINE(X, A, "a", "A", 1, 0) \
    SK_ENGINE(X, B, "b", "B", 0, 2) \
    /* utilidades */ \
    X(SUB_LEVEL,   "sub_level",   "Sub Nível", PK_FLOAT, -60, 0, -60, "dB", "") \
    X(SUB_WAVE,    "sub_wave",    "Sub Onda", PK_CHOICE, 0, 2, 0, "", "Seno|Triângulo|Quadrada") \
    X(SUB_OCT,     "sub_oct",     "Sub Oitava", PK_CHOICE, 0, 1, 0, "", "-1|-2") \
    X(NOISE_LEVEL, "noise_level", "Ruído Nível", PK_FLOAT, -60, 0, -60, "dB", "") \
    X(NOISE_COLOR, "noise_color", "Ruído Cor", PK_FLOAT, 0, 1, 0.3f, "%", "") \
    /* filtros */ \
    SK_FILTER(X, F1, "f1", "1", 2, 20000) \
    SK_FILTER(X, F2, "f2", "2", 0, 20000) \
    X(FILT_ROUTE, "filt_route", "Routing Filtros", PK_CHOICE, 0, 2, 0, "", "Série|Paralelo|Divisão A/B") \
    X(FILT_BAL,   "filt_bal",   "Balanço Filtros", PK_FLOAT, 0, 1, 0.5f, "%", "") \
    /* envelopes */ \
    SK_ENV(X, AMP,  "amp",  "Amp",  3, 300, 0.7f, 250) \
    SK_ENV(X, ENV2, "env2", "Env 2", 5, 300, 0.4f, 250) \
    SK_ENV(X, ENV3, "env3", "Env 3", 5, 400, 0.0f, 300) \
    /* LFOs */ \
    SK_LFO(X, LFO1, "lfo1", "LFO 1") \
    SK_LFO(X, LFO2, "lfo2", "LFO 2") \
    /* macros */ \
    SK_MACRO(X, 1) SK_MACRO(X, 2) SK_MACRO(X, 3) SK_MACRO(X, 4) \
    SK_MACRO(X, 5) SK_MACRO(X, 6) SK_MACRO(X, 7) SK_MACRO(X, 8) \
    /* matriz de modulação */ \
    SK_MOD(X, 1) SK_MOD(X, 2) SK_MOD(X, 3) SK_MOD(X, 4) SK_MOD(X, 5) SK_MOD(X, 6) SK_MOD(X, 7) SK_MOD(X, 8) \
    SK_MOD(X, 9) SK_MOD(X, 10) SK_MOD(X, 11) SK_MOD(X, 12) SK_MOD(X, 13) SK_MOD(X, 14) SK_MOD(X, 15) SK_MOD(X, 16) \
    /* acordes, escalas e arpejador */ \
    X(CHORD_TYPE, "chord_type", "Acorde", PK_CHOICE, 0, 14, 0, "", SK_CHORDS) \
    X(SCALE_KEY,  "scale_key",  "Tónica da Escala", PK_CHOICE, 0, 11, 0, "", SK_KEYS) \
    X(SCALE_TYPE, "scale_type", "Escala", PK_CHOICE, 0, 10, 0, "", SK_SCALES) \
    X(SCALE_SNAP, "scale_snap", "Forçar Escala", PK_BOOL, 0, 1, 0, "", "") \
    X(ARP_ON,     "arp_on",     "Arpejador", PK_BOOL, 0, 1, 0, "", "") \
    X(ARP_MODE,   "arp_mode",   "Arp Modo", PK_CHOICE, 0, 6, 0, "", "Cima|Baixo|Cima-Baixo|Baixo-Cima|Aleatório|Ordem tocada|Acorde") \
    X(ARP_RATE,   "arp_rate",   "Arp Taxa", PK_CHOICE, 0, 15, 12, "", SK_DIVLIST) \
    X(ARP_GATE,   "arp_gate",   "Arp Gate", PK_FLOAT, 0.05f, 1, 0.6f, "%", "") \
    X(ARP_OCT,    "arp_oct",    "Arp Oitavas", PK_INT, 1, 4, 1, "", "") \
    X(ARP_SWING,  "arp_swing",  "Arp Swing", PK_FLOAT, 0, 0.75f, 0, "%", "") \
    X(ARP_LATCH,  "arp_latch",  "Arp Latch", PK_BOOL, 0, 1, 0, "", "") \
    X(ARP_PATTERN,"arp_pattern","Arp Padrão", PK_CHOICE, 0, 7, 0, "", SK_ARPPATTERNS) \
    /* efeitos */ \
    SK_FXSLOT(X, FXA1, "fxa1", "A1") SK_FXSLOT(X, FXA2, "fxa2", "A2") SK_FXSLOT(X, FXA3, "fxa3", "A3") \
    SK_FXSLOT(X, FXB1, "fxb1", "B1") SK_FXSLOT(X, FXB2, "fxb2", "B2") SK_FXSLOT(X, FXB3, "fxb3", "B3") \
    SK_FXSLOT(X, FXX1, "fxx1", "Aux1") SK_FXSLOT(X, FXX2, "fxx2", "Aux2") SK_FXSLOT(X, FXX3, "fxx3", "Aux3") \
    X(INSA_LEVEL, "insa_level", "Insert A Nível", PK_FLOAT, -60, 6, 0, "dB", "") \
    X(INSB_LEVEL, "insb_level", "Insert B Nível", PK_FLOAT, -60, 6, 0, "dB", "") \
    X(SEND_A,     "send_a",     "Send A para Aux", PK_FLOAT, 0, 1, 0, "%", "") \
    X(SEND_B,     "send_b",     "Send B para Aux", PK_FLOAT, 0, 1, 0, "%", "") \
    X(AUX_RETURN, "aux_return", "Retorno Aux", PK_FLOAT, -60, 6, 0, "dB", "") \
    /* padrão do arpejador editável (16 passos: nível 0 = pausa, oitava -1/0/+1, repetições 0/×2/×3) */ \
    X(ARP_EDIT, "arp_edit", "Arp Padrão Próprio", PK_BOOL, 0, 1, 0, "", "") \
    X(ARP_LEN,  "arp_len",  "Arp Passos", PK_INT, 1, 16, 16, "", "") \
    SK_ARPSTEP(X, 1) SK_ARPSTEP(X, 2) SK_ARPSTEP(X, 3) SK_ARPSTEP(X, 4) SK_ARPSTEP(X, 5) SK_ARPSTEP(X, 6) SK_ARPSTEP(X, 7) SK_ARPSTEP(X, 8) \
    SK_ARPSTEP(X, 9) SK_ARPSTEP(X, 10) SK_ARPSTEP(X, 11) SK_ARPSTEP(X, 12) SK_ARPSTEP(X, 13) SK_ARPSTEP(X, 14) SK_ARPSTEP(X, 15) SK_ARPSTEP(X, 16)

// --------------------------------- enum + tabela --------------------------------
#define SK_ENUM_ROW(E, id, name, kind, mn, mx, def, unit, choices) P_##E,
enum ParamIndex : int { SK_PARAMS(SK_ENUM_ROW) P_COUNT };
#undef SK_ENUM_ROW

#define SK_INFO_ROW(E, id, name, kind, mn, mx, def, unit, choices) { id, name, kind, (float) (mn), (float) (mx), (float) (def), unit, choices },
inline const ParamInfo kParams[P_COUNT] = { SK_PARAMS(SK_INFO_ROW) };
#undef SK_INFO_ROW

// índices das famílias (para ciclos)
inline constexpr int fxSlotBase(int slot) { return P_FXA1_TYPE + slot * (P_FXA2_TYPE - P_FXA1_TYPE); }
inline constexpr int fxSlotStride() { return P_FXA2_TYPE - P_FXA1_TYPE; }
inline constexpr int modSlotBase(int slot) { return P_MOD1_SRC + slot * (P_MOD2_SRC - P_MOD1_SRC); }

} // namespace sk
