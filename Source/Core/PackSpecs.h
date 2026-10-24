// =============================================================================
//  SYNTHKEY by Piradex — definição dos 8 packs (Núcleo + 7 estilos) e dos seus instrumentos
//  Os samples são gerados por SampleSynth.h; os WAV dos packs pagos são criados por tools/packgen.
// =============================================================================
#pragma once
#include "SampleSynth.h"

namespace sk
{
struct KitKey { int key; const char* inst; int variant; };

struct PackDef
{
    std::string id, name, style, desc, version = "1.1.0";
    bool core = false;                    // Núcleo: incluído, nunca bloqueado
    std::vector<synth::InstSpec> insts;
    std::vector<KitKey> kit;
    std::string accent;                   // cor de destaque (#RRGGBB) para a interface
};

namespace packs_detail
{
using synth::InstSpec; using namespace synth;
inline InstSpec D (const char* id, const char* nm, const char* cat, Gen g, float a, float b, float c, float d, int var)
{ return InstSpec { id, nm, cat, g, a, b, c, d, 0, 0, 0, var, false }; }
inline InstSpec T (const char* id, const char* nm, const char* cat, Gen g, float a, float b, float c, float d, int lo, int hi, int step, int var = 1)
{ return InstSpec { id, nm, cat, g, a, b, c, d, lo, hi, step, var, true }; }
}

inline std::vector<PackDef> buildPackDefs()
{
    using namespace packs_detail;
    std::vector<PackDef> v;

    // ------------------------------------------------------------------ Núcleo (incluído)
    { PackDef p; p.id = "nucleo"; p.name = "Núcleo"; p.style = "multi"; p.core = true; p.accent = "#6FE7F5";
      p.desc = "Base de sintetizador: bateria essencial, 808, teclas, pads, vozes e efeitos para qualquer estilo.";
      p.insts = {
        D ("kick", "Bombo", "Bateria", G_KICK, 160, 50, 0.02f, 0.26f, 3), D ("snare", "Caixa", "Bateria", G_SNARE, 195, 0.85f, 0.12f, 0.9f, 3),
        D ("clap", "Palmas", "Bateria", G_CLAP, 0.15f, 1.0f, 0, 0, 2), D ("hat", "Hi-Hat Fechado", "Bateria", G_HAT, 0.035f, 1.0f, 1, 0, 3),
        D ("ohat", "Hi-Hat Aberto", "Bateria", G_OHAT, 0.26f, 1.0f, 1, 0, 1), D ("shaker", "Shaker", "Percussão", G_SHAKER, 0.17f, 1.0f, 0, 0, 2),
        D ("rim", "Rimshot", "Bateria", G_RIM, 880, 0, 0, 0, 1), D ("tom", "Tom", "Bateria", G_TOM, 120, 0.3f, 0.2f, 0, 1),
        T ("808", "808 Sub", "Baixo", G_808, 1.3f, 0.4f, 0.4f, 2.4f, 24, 48, 4),
        T ("sub", "Sub Suave", "Baixo", G_SUB, 0.9f, 0.4f, 0, 0, 28, 52, 6),
        T ("ep", "Piano Elétrico", "Teclas", G_EP, 0.5f, 2.4f, 0.4f, 0, 48, 84, 6),
        T ("pluck", "Guitarra Pluck", "Teclas", G_PLUCK, 1.2f, 0.7f, 2.0f, 0, 40, 76, 6),
        T ("pad", "Pad Suave", "Pads", G_PAD, 6.0f, 12.0f, 0.4f, 0, 36, 72, 12),
        T ("bell", "Sino", "Mallets", G_BELL, 2.2f, 1.0f, 0, 0, 60, 84, 6),
        T ("vox", "Voz Ah", "Vozes", G_VOX, 0, 1.4f, 1.0f, 1, 48, 72, 6),
        T ("stab", "Stab Sintético", "Sintetizador", G_STAB, 0.6f, 3000, 0.6f, 0, 48, 72, 12),
        D ("riser", "Subida", "Efeitos", G_RISER, 2.5f, 0, 0, 0, 1), D ("impact", "Impacto", "Efeitos", G_IMPACT, 2.2f, 0, 0, 0, 1) };
      p.kit = { { 36, "kick", 0 }, { 37, "rim", 0 }, { 38, "snare", 0 }, { 39, "clap", 0 }, { 41, "tom", 0 }, { 42, "hat", 0 }, { 44, "shaker", 0 }, { 46, "ohat", 0 } };
      v.push_back (p); }

    // ------------------------------------------------------------------ Trap
    { PackDef p; p.id = "trap"; p.name = "Trap"; p.style = "trap"; p.accent = "#B565FF";
      p.desc = "808 longos e distorcidos, hats em rolos, sinos escuros, pads de coro e stabs.";
      p.insts = {
        T ("808long", "808 Longo", "Baixo", G_808, 1.9f, 0.75f, 0.45f, 3.4f, 24, 48, 3), T ("808curto", "808 Curto", "Baixo", G_808, 0.55f, 0.5f, 0.3f, 1.1f, 24, 48, 4),
        D ("kick", "Bombo", "Bateria", G_KICK, 155, 45, 0.016f, 0.3f, 3), D ("snare", "Caixa", "Bateria", G_SNARE, 205, 0.9f, 0.1f, 1.0f, 3),
        D ("clap", "Palmas", "Bateria", G_CLAP, 0.17f, 1.0f, 0, 0, 3), D ("hat", "Hi-Hat Fechado", "Bateria", G_HAT, 0.028f, 1.1f, 1, 0, 4),
        D ("ohat", "Hi-Hat Aberto", "Bateria", G_OHAT, 0.22f, 1.1f, 1, 0, 2), D ("rim", "Rimshot", "Bateria", G_RIM, 1000, 0, 0, 0, 2),
        T ("bell", "Sino Escuro", "Mallets", G_BELL, 2.6f, 0.9f, 0, 0, 60, 84, 4), T ("pluck", "Pluck Gelado", "Teclas", G_PLUCK, 0.8f, 0.35f, 1.6f, 0, 36, 72, 6),
        T ("pad", "Pad Coral Escuro", "Pads", G_PAD, 7.0f, 18.0f, 0.15f, 0, 36, 60, 12), T ("vox", "Voz Ah", "Vozes", G_VOX, 0, 1.2f, 1.0f, 1, 55, 79, 6),
        T ("stab", "Stab Duro", "Sintetizador", G_STAB, 0.5f, 2600, 0.8f, 0, 48, 72, 6),
        D ("riser", "Subida", "Efeitos", G_RISER, 2.6f, 0, 0, 0, 1), D ("impact", "Impacto", "Efeitos", G_IMPACT, 2.4f, 0, 0, 0, 1) };
      p.kit = { { 36, "kick", 0 }, { 37, "rim", 0 }, { 38, "snare", 0 }, { 39, "clap", 0 }, { 42, "hat", 0 }, { 46, "ohat", 0 } };
      v.push_back (p); }

    // ------------------------------------------------------------------ R&B
    { PackDef p; p.id = "rnb"; p.name = "R&B"; p.style = "rnb"; p.accent = "#E6B8A2";
      p.desc = "Rhodes e Wurlitzer, cordas suaves, baixos redondos, vozes e percussão leve.";
      p.insts = {
        T ("rhodes", "Rhodes", "Teclas", G_EP, 0.35f, 3.0f, 0.5f, 0, 36, 84, 4), T ("wurli", "Wurlitzer", "Teclas", G_EP, 0.8f, 2.2f, 0.2f, 0, 48, 84, 6),
        T ("strings", "Cordas Suaves", "Pads", G_PAD, 7.0f, 10.0f, 0.45f, 0, 36, 72, 12), T ("bass", "Baixo Redondo", "Baixo", G_SUB, 1.0f, 0.4f, 0, 0, 28, 50, 3),
        D ("kick", "Bombo Suave", "Bateria", G_KICK, 130, 52, 0.03f, 0.22f, 2), D ("snare", "Caixa Suave", "Bateria", G_SNARE, 185, 0.6f, 0.14f, 0.6f, 2),
        D ("rim", "Rimshot", "Bateria", G_RIM, 820, 0, 0, 0, 2), D ("clap", "Palmas", "Bateria", G_CLAP, 0.2f, 1.3f, 0, 0, 2),
        D ("hat", "Hi-Hat", "Bateria", G_HAT, 0.04f, 0.9f, 0, 0, 3), D ("shaker", "Shaker", "Percussão", G_SHAKER, 0.2f, 0.9f, 0, 0, 2),
        T ("vox", "Voz Ah", "Vozes", G_VOX, 0, 0.9f, 1.0f, 0, 48, 76, 4), T ("voxoo", "Coro Oo", "Vozes", G_VOX, 4, 1.8f, 1.0f, 1, 48, 72, 6),
        T ("guitar", "Guitarra Limpa", "Teclas", G_PLUCK, 1.4f, 0.5f, 2.0f, 0, 40, 76, 6), T ("bell", "Sino Suave", "Mallets", G_BELL, 2.0f, 0.8f, 0, 0, 60, 84, 6) };
      p.kit = { { 36, "kick", 0 }, { 37, "rim", 0 }, { 38, "snare", 0 }, { 39, "clap", 0 }, { 42, "hat", 0 }, { 44, "shaker", 0 } };
      v.push_back (p); }

    // ------------------------------------------------------------------ Afrobeats
    { PackDef p; p.id = "afrobeat"; p.name = "Afrobeats"; p.style = "afrobeat"; p.accent = "#FF9A3D";
      p.desc = "Congas e bongós, tambor falante, shakers, guitarras, kalimba, baixo e metais.";
      p.insts = {
        D ("congahi", "Conga Aguda", "Percussão", G_CONGA, 330, 0.18f, 0.5f, 0, 3), D ("congalo", "Conga Grave", "Percussão", G_CONGA, 210, 0.25f, 0.5f, 0, 3),
        D ("bongo", "Bongó", "Percussão", G_CONGA, 430, 0.12f, 0.5f, 0, 2), T ("talking", "Tambor Falante", "Percussão", G_TALK, 0.6f, 0, 0, 0, 48, 60, 4),
        D ("shaker", "Shaker", "Percussão", G_SHAKER, 0.15f, 1.0f, 0, 0, 4), D ("kick", "Bombo", "Bateria", G_KICK, 140, 50, 0.025f, 0.24f, 3),
        D ("snare", "Caixa", "Bateria", G_SNARE, 200, 0.7f, 0.11f, 0.8f, 2), D ("clap", "Palmas", "Bateria", G_CLAP, 0.16f, 1.1f, 0, 0, 2),
        D ("rim", "Rimshot", "Bateria", G_RIM, 900, 0, 0, 0, 2), D ("hat", "Hi-Hat", "Bateria", G_HAT, 0.032f, 1.0f, 0, 0, 3), D ("ohat", "Hi-Hat Aberto", "Bateria", G_OHAT, 0.22f, 1.0f, 0, 0, 1),
        T ("guitar", "Guitarra Afro", "Teclas", G_PLUCK, 1.0f, 0.75f, 1.6f, 0, 40, 76, 4), T ("bass", "Baixo Elétrico", "Baixo", G_BASSG, 1.0f, 0.2f, 1.3f, 0, 28, 52, 4),
        T ("kalimba", "Kalimba", "Mallets", G_KALIMBA, 1.6f, 0, 0, 0, 60, 84, 4), T ("pad", "Pad Quente", "Pads", G_PAD, 6.0f, 11.0f, 0.3f, 0, 36, 60, 12),
        T ("vox", "Voz Oh", "Vozes", G_VOX, 3, 0.7f, 1.0f, 0, 48, 72, 6), T ("horn", "Metais", "Sintetizador", G_STAB, 0.55f, 2600, 0.5f, 0, 48, 72, 6),
        D ("whistle", "Apito", "Efeitos", G_WHISTLE, 2400, 0.6f, 0, 0, 1) };
      p.kit = { { 36, "kick", 0 }, { 37, "rim", 0 }, { 38, "snare", 0 }, { 39, "clap", 0 }, { 42, "hat", 0 }, { 43, "congalo", 0 }, { 44, "shaker", 0 }, { 45, "congahi", 0 }, { 46, "ohat", 0 }, { 47, "bongo", 0 } };
      v.push_back (p); }

    // ------------------------------------------------------------------ Amapiano
    { PackDef p; p.id = "amapiano"; p.name = "Amapiano"; p.style = "amapiano"; p.accent = "#FFB36B";
      p.desc = "Log drums afinados, shakers, piano, pads aéreos, vozes cortadas e baixo profundo.";
      p.insts = {
        T ("logdrum", "Log Drum", "Baixo", G_LOGDRUM, 0.55f, 0.5f, 0, 0, 43, 67, 3), T ("logshort", "Log Drum Curto", "Baixo", G_LOGDRUM, 0.22f, 0.8f, 0, 0, 43, 67, 4),
        D ("kick", "Bombo Suave", "Bateria", G_KICK, 120, 52, 0.03f, 0.22f, 2), D ("snare", "Caixa", "Bateria", G_SNARE, 190, 0.5f, 0.09f, 0.4f, 2),
        D ("rim", "Rimshot", "Bateria", G_RIM, 860, 0, 0, 0, 2), D ("clap", "Palmas", "Bateria", G_CLAP, 0.18f, 1.2f, 0, 0, 2),
        D ("shaker", "Shaker", "Percussão", G_SHAKER, 0.14f, 1.1f, 0, 0, 4), D ("shakerl", "Shaker Longo", "Percussão", G_SHAKER, 0.3f, 1.0f, 0, 0, 2),
        D ("hat", "Hi-Hat", "Bateria", G_HAT, 0.05f, 0.9f, 0, 0, 3), D ("ohat", "Hi-Hat Aberto", "Bateria", G_OHAT, 0.3f, 0.9f, 0, 0, 1),
        T ("piano", "Piano", "Teclas", G_PIANO, 0.5f, 2.6f, 0, 0, 48, 84, 4), T ("keys", "Teclas Jazz", "Teclas", G_EP, 0.55f, 2.6f, 0.3f, 0, 48, 84, 6),
        T ("pad", "Pad Aéreo", "Pads", G_PAD, 7.0f, 14.0f, 0.35f, 0, 48, 72, 12), T ("chopah", "Vocal Chop Ah", "Vozes", G_VOX, 0, 0.55f, 0.5f, 0, 55, 79, 4),
        T ("chopoh", "Vocal Chop Oh", "Vozes", G_VOX, 3, 0.55f, 0.5f, 0, 55, 79, 4), T ("bass", "Baixo Amapiano", "Baixo", G_SUB, 1.3f, 0.9f, 0, 0, 29, 50, 3),
        D ("whistle", "Apito", "Efeitos", G_WHISTLE, 2200, 0.7f, 0, 0, 1), D ("riser", "Subida", "Efeitos", G_RISER, 2.4f, 0, 0, 0, 1), D ("impact", "Impacto", "Efeitos", G_IMPACT, 2.0f, 0, 0, 0, 1) };
      p.kit = { { 36, "kick", 0 }, { 37, "rim", 0 }, { 38, "snare", 0 }, { 39, "clap", 0 }, { 42, "hat", 0 }, { 44, "shaker", 0 }, { 46, "ohat", 0 }, { 47, "shakerl", 0 } };
      v.push_back (p); }

    // ------------------------------------------------------------------ Drill
    { PackDef p; p.id = "drill"; p.name = "Drill"; p.style = "drill"; p.accent = "#FF4D4D";
      p.desc = "808 com slides, snares secos, hats rápidos, sinos frios e pads sombrios.";
      p.insts = {
        T ("808slide", "808 Slide", "Baixo", G_808, 2.2f, 0.85f, 0.9f, 3.2f, 24, 48, 3), T ("808curto", "808 Curto", "Baixo", G_808, 0.6f, 0.6f, 0.5f, 1.2f, 24, 48, 6),
        D ("kick", "Bombo", "Bateria", G_KICK, 150, 44, 0.014f, 0.26f, 3), D ("snare", "Caixa", "Bateria", G_SNARE, 215, 1.0f, 0.085f, 1.2f, 3),
        D ("clap", "Palmas", "Bateria", G_CLAP, 0.14f, 0.8f, 0, 0, 2), D ("hat", "Hi-Hat", "Bateria", G_HAT, 0.022f, 1.1f, 1, 0, 4),
        D ("ohat", "Hi-Hat Aberto", "Bateria", G_OHAT, 0.18f, 1.1f, 1, 0, 1), D ("rim", "Rimshot", "Bateria", G_RIM, 1050, 0, 0, 0, 1),
        D ("tom", "Tom", "Bateria", G_TOM, 100, 0.3f, 0.2f, 0, 2),
        T ("bell", "Sino Frio", "Mallets", G_BELL, 3.0f, 0.8f, 0, 0, 48, 72, 4), T ("pad", "Pad Sombrio", "Pads", G_PAD, 7.0f, 18.0f, 0.1f, 0, 36, 60, 12),
        T ("pluck", "Pluck Sombrio", "Teclas", G_PLUCK, 0.7f, 0.3f, 1.4f, 0, 36, 72, 6), T ("vox", "Voz Oh", "Vozes", G_VOX, 3, 1.0f, 1.0f, 1, 55, 79, 6),
        D ("riser", "Subida", "Efeitos", G_RISER, 2.6f, 0, 0, 0, 1), D ("impact", "Impacto", "Efeitos", G_IMPACT, 2.4f, 0, 0, 0, 1) };
      p.kit = { { 36, "kick", 0 }, { 37, "rim", 0 }, { 38, "snare", 0 }, { 39, "clap", 0 }, { 41, "tom", 0 }, { 42, "hat", 0 }, { 46, "ohat", 0 } };
      v.push_back (p); }

    // ------------------------------------------------------------------ Kizomba · Zouk
    { PackDef p; p.id = "kizomba"; p.name = "Kizomba · Zouk"; p.style = "kizomba"; p.accent = "#E86A5A";
      p.desc = "Baixo de guitarra, teclas, guitarras limpas, cordas, vozes e percussão suave.";
      p.insts = {
        T ("bass", "Baixo de Guitarra", "Baixo", G_BASSG, 1.3f, 0.15f, 1.8f, 0, 28, 52, 3), T ("ep", "Piano Elétrico", "Teclas", G_EP, 0.3f, 3.0f, 0.4f, 0, 40, 84, 4),
        T ("guitar", "Guitarra Limpa", "Teclas", G_PLUCK, 1.3f, 0.55f, 2.0f, 0, 40, 76, 4), T ("strings", "Cordas", "Pads", G_PAD, 7.0f, 9.0f, 0.4f, 0, 36, 72, 12),
        T ("vox", "Voz Ah", "Vozes", G_VOX, 0, 0.9f, 1.0f, 0, 48, 76, 4), T ("bell", "Sino", "Mallets", G_BELL, 2.0f, 0.8f, 0, 0, 60, 84, 4),
        D ("kick", "Bombo Suave", "Bateria", G_KICK, 125, 50, 0.03f, 0.26f, 2), D ("snare", "Caixa Suave", "Bateria", G_SNARE, 185, 0.55f, 0.13f, 0.5f, 2),
        D ("rim", "Rimshot", "Bateria", G_RIM, 880, 0, 0, 0, 2), D ("clap", "Palmas", "Bateria", G_CLAP, 0.2f, 1.2f, 0, 0, 1),
        D ("guiro", "Guiro", "Percussão", G_SHAKER, 0.2f, 0.8f, 0, 0, 3), D ("conga", "Conga", "Percussão", G_CONGA, 240, 0.22f, 0.4f, 0, 2),
        D ("hat", "Hi-Hat", "Bateria", G_HAT, 0.04f, 0.9f, 0, 0, 2), D ("riser", "Subida", "Efeitos", G_RISER, 2.4f, 0, 0, 0, 1) };
      p.kit = { { 36, "kick", 0 }, { 37, "rim", 0 }, { 38, "snare", 0 }, { 39, "clap", 0 }, { 42, "hat", 0 }, { 44, "guiro", 0 }, { 45, "conga", 0 } };
      v.push_back (p); }

    // ------------------------------------------------------------------ Kuduro
    { PackDef p; p.id = "kuduro"; p.name = "Kuduro"; p.style = "kuduro"; p.accent = "#7CF0A6";
      p.desc = "Bombos duros, caixas e palmas apertadas, stabs serrilhados, baixos agressivos e sirenes.";
      p.insts = {
        D ("kick", "Bombo Duro", "Bateria", G_KICK, 190, 52, 0.012f, 0.3f, 3), D ("snare", "Caixa", "Bateria", G_SNARE, 230, 1.1f, 0.09f, 1.3f, 3),
        D ("clap", "Palmas", "Bateria", G_CLAP, 0.15f, 0.6f, 0, 0, 3), D ("hat", "Hi-Hat", "Bateria", G_HAT, 0.03f, 1.2f, 1, 0, 3),
        D ("ohat", "Hi-Hat Aberto", "Bateria", G_OHAT, 0.2f, 1.2f, 1, 0, 1), D ("tom", "Tom", "Bateria", G_TOM, 140, 0.25f, 0.2f, 0, 3),
        T ("stab", "Stab Serrilhado", "Sintetizador", G_STAB, 0.5f, 3500, 0.8f, 0, 36, 72, 3), T ("bass", "Baixo Agressivo", "Baixo", G_SUB, 1.0f, 1.0f, 0, 0, 24, 48, 3),
        T ("acid", "Pluck Ácido", "Sintetizador", G_PLUCK, 0.35f, 1.0f, 1.0f, 0, 36, 72, 6), T ("siren", "Sirene", "Efeitos", G_SIREN, 1.0f, 0, 0, 0, 48, 72, 12),
        T ("shout", "Voz Hey", "Vozes", G_VOX, 0, 0.35f, 0.3f, 0, 55, 67, 6), D ("whistle", "Apito", "Efeitos", G_WHISTLE, 2600, 0.5f, 0, 0, 1),
        D ("riser", "Subida", "Efeitos", G_RISER, 2.2f, 0, 0, 0, 1), D ("impact", "Impacto", "Efeitos", G_IMPACT, 2.0f, 0, 0, 0, 2) };
      p.kit = { { 36, "kick", 0 }, { 38, "snare", 0 }, { 39, "clap", 0 }, { 41, "tom", 0 }, { 42, "hat", 0 }, { 46, "ohat", 0 } };
      v.push_back (p); }
    return v;
}

// gera todos os instrumentos (e o kit) de um pack para uma biblioteca
inline void buildPackLibrary (const PackDef& p, Library& lib)
{
    Library local;
    for (const auto& s : p.insts) synth::addInstrument (local, s, p.id);
    // kit: usa o 1.º sample de cada instrumento de bateria/percussão
    if (! p.kit.empty())
    {
        Instrument kit; kit.id = p.id + ".kit"; kit.name = "Kit de Bateria"; kit.cat = "Bateria"; kit.pack = p.id; kit.fixedPitch = true;
        for (const auto& k : p.kit)
        {
            const int ii = local.indexOf (p.id + "." + k.inst);
            if (ii < 0) continue;
            const auto& src = local.inst[(size_t) ii];
            const auto& sz = src.zones[(size_t) std::min ((int) src.zones.size() - 1, k.variant)];
            Zone z = sz; z.loKey = z.hiKey = k.key; z.root = k.key; z.rr = 0;
            kit.zones.push_back (z);
        }
        local.inst.insert (local.inst.begin(), std::move (kit));
    }
    lib.absorb (std::move (local));
}
} // namespace sk
