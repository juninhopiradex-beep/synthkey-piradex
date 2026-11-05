// =============================================================================
//  SYNTHKEY by Piradex — famílias novas (v1.5): Cordas, Metais, Sinos, Mallets, Guitarras, Órgãos,
//  Sopros, Acordes, Sequências, Texturas, Cinemático, Híbrido, Experimental.
//  Incluído dentro de PresetBuilder.h, antes dos #undef das macros.
//
//  Estado de validação: "auto" = passou os testes técnicos automáticos (parâmetros válidos, nível calibrado,
//  sem NaN, cauda finita, sem duplicados). NENHUM destes presets foi ouvido/aprovado por um humano.
//  Cada receita gera 3 presets (base · Suave · Brilhante) com cortes de filtro diferentes.
// =============================================================================
#define FT(t, r) " f1_type=" #t " f1_res=" #r

struct FamRecipe { const char* family; const char* cat; const char* sub; const char* name; const char* tags; const char* role;
                   const char* smpA; const char* smpB; float cut; const char* kv; };

inline std::vector<Preset> familiesNucleo()
{
    static const FamRecipe R[] = {
    // ================================ CORDAS ================================
    { "Strings", "Cordas", "Ensemble", "Cordas Orquestra Larga", "cordas,orquestra,larga", "pad", "", "", 3800,
      ANA (a, 2, 6, 18) ANA (b, 2, 4, 12) " b_coarse=0 b_level=-4 blend=0.5" FT (1, 0.1) AMP (260, 600, 0.85, 520)
      CHORUS ("fxa1", 0.25, 0.4, 0.5, 0.5, 1, 0.35) REVERB ("fxa3", 0.65, 0.65, 0.45, 0.1, 1, 0.3) },
    { "Strings", "Cordas", "Ensemble", "Cordas Lentas", "cordas,lento,pad", "pad", "", "", 2800,
      ANA (a, 2, 7, 22) FT (1, 0.08) AMP (650, 800, 0.9, 900) CHORUS ("fxa1", 0.2, 0.4, 0.5, 0.5, 1, 0.35) REVERB ("fxa3", 0.7, 0.7, 0.45, 0.1, 1, 0.32) },
    { "Strings", "Cordas", "Staccato", "Cordas Spiccato", "cordas,staccato,curto", "stab", "", "", 4500,
      ANA (a, 2, 3, 10) FT (2, 0.15) " f1_env=0.35" ENV2 (4, 260, 0.0, 150) AMP (8, 220, 0.0, 160) REVERB ("fxa3", 0.5, 0.45, 0.5, 0.05, 0.9, 0.2) },
    { "Strings", "Cordas", "Pizzicato", "Cordas Pizzicato", "cordas,pizzicato,curto", "pluck", "", "", 3200,
      ANA (a, 1, 2, 6) FT (1, 0.1) " f1_env=0.4" ENV2 (1, 300, 0.0, 120) AMP (1, 320, 0.0, 140) REVERB ("fxa3", 0.5, 0.45, 0.5, 0.05, 0.9, 0.2) },
    { "Strings", "Cordas", "Ensemble", "Ensemble Digital Cordas", "cordas,digital,ensemble", "pad", "", "", 5000,
      WTB (a, 10, 0.4, 4, 16) FT (1, 0.08) AMP (300, 700, 0.85, 700) CHORUS ("fxa1", 0.25, 0.35, 0.5, 0.5, 1, 0.3) REVERB ("fxa3", 0.62, 0.62, 0.45, 0.1, 1, 0.28) },
    { "Strings", "Cordas", "Quarteto", "Quarteto Íntimo", "cordas,quarteto,expressivo", "lead", "", "", 3600,
      ANA (a, 2, 3, 8) FT (1, 0.1) MOD (1, 1, 3, 0.03) " lfo1_rate=5.4 lfo1_fade=500" AMP (120, 500, 0.9, 450) REVERB ("fxa3", 0.55, 0.55, 0.5, 0.08, 0.9, 0.22) },
    { "Strings", "Cordas", "Orquestral", "Cordas Cinemáticas", "cordas,cinema,épico", "pad", "", "", 3000,
      ANA (a, 2, 8, 28) ANA (b, 2, 6, 20) " b_coarse=-12 b_level=-6 blend=0.5 sub_level=-16" FT (1, 0.1) AMP (500, 900, 0.9, 1400)
      CHORUS ("fxa1", 0.2, 0.4, 0.5, 0.5, 1, 0.3) REVERB ("fxa3", 0.8, 0.8, 0.45, 0.12, 1, 0.4) },
    { "Strings", "Cordas", "Tremolo", "Cordas Trémolo", "cordas,trémolo,tensão", "pad", "", "", 3800,
      ANA (a, 2, 4, 14) FT (1, 0.1) AMP (40, 500, 0.9, 500) TREM ("fxa1", 0.62, 0.5, 0.0, 0.5, 0.5, 0.9) REVERB ("fxa3", 0.6, 0.6, 0.45, 0.1, 1, 0.28) },
    // ================================ METAIS ================================
    { "Brass", "Metais", "Ensemble", "Metais Ensemble", "metais,ensemble,quente", "stab", "", "", 2600,
      ANA (a, 2, 4, 14) ANA (b, 2, 3, 10) " b_level=-5 blend=0.5" FT (2, 0.12) " f1_env=0.55" ENV2 (60, 500, 0.6, 200) AMP (40, 400, 0.85, 200) REVERB ("fxa3", 0.55, 0.5, 0.5, 0.08, 0.9, 0.22) },
    { "Brass", "Metais", "Solo", "Trompete Solo", "metais,trompete,solo", "lead", "", "", 3200,
      ANA (a, 2, 2, 6) LEGATO (40) FT (2, 0.15) " f1_env=0.5" ENV2 (35, 300, 0.6, 150) MOD (1, 1, 3, 0.02) " lfo1_rate=5.2 lfo1_fade=700" AMP (25, 300, 0.9, 160) REVERB ("fxa3", 0.5, 0.45, 0.5, 0.06, 0.9, 0.2) },
    { "Brass", "Metais", "Stab", "Stab de Metais", "metais,stab,curto", "stab", "", "", 3500,
      ANA (a, 2, 5, 16) FT (2, 0.12) " f1_env=0.6" ENV2 (8, 300, 0.0, 160) AMP (4, 280, 0.0, 150) REVERB ("fxa3", 0.5, 0.45, 0.5, 0.06, 0.9, 0.2) },
    { "Brass", "Metais", "Solo", "Trombone Suave", "metais,trombone,grave", "lead", "", "", 1600,
      ANA (a, 2, 3, 8) LEGATO (50) FT (2, 0.1) " f1_env=0.45" ENV2 (90, 500, 0.5, 200) AMP (70, 400, 0.85, 220) REVERB ("fxa3", 0.55, 0.5, 0.5, 0.08, 0.9, 0.22) },
    { "Brass", "Metais", "Orquestral", "Fanfarra Cinemática", "metais,fanfarra,épico", "pad", "", "", 2200,
      ANA (a, 2, 7, 20) ANA (b, 3, 4, 10) " b_level=-8 blend=0.5" FT (2, 0.1) " f1_env=0.4" ENV2 (150, 600, 0.7, 400) AMP (120, 600, 0.9, 500) REVERB ("fxa3", 0.8, 0.78, 0.45, 0.12, 1, 0.38) },
    { "Brass", "Metais", "Sintético", "Metais Sintéticos 80s", "metais,80s,sintético", "stab", "", "", 3000,
      ANA (a, 2, 6, 20) FT (2, 0.15) " f1_env=0.5" ENV2 (80, 700, 0.5, 300) AMP (30, 500, 0.8, 300) CHORUS ("fxa1", 0.3, 0.4, 0.5, 0.5, 1, 0.3) REVERB ("fxa3", 0.6, 0.55, 0.5, 0.1, 1, 0.25) },
    { "Brass", "Metais", "Swell", "Swell de Metais", "metais,swell,crescendo", "pad", "", "", 2000,
      ANA (a, 2, 5, 16) FT (2, 0.1) " f1_env=0.6" ENV2 (800, 900, 0.8, 500) AMP (700, 900, 0.9, 600) REVERB ("fxa3", 0.7, 0.7, 0.45, 0.1, 1, 0.32) },
    // ================================ SINOS ================================
    { "Bells", "Sinos", "Cristal", "Sino Cristal", "sino,cristal,brilhante", "bell", "", "", 9000,
      WTB (a, 5, 0.5, 2, 8) FT (1, 0.05) AMP (1, 2400, 0.0, 1500) REVERB ("fxa3", 0.75, 0.78, 0.4, 0.1, 1, 0.35) },
    { "Bells", "Sinos", "FM", "Sino FM Clássico", "sino,fm,digital", "bell", "", "", 9000,
      ANA (a, 0, 1, 0) " a_fmamt=0.45 a_fmrat=3" MOD (1, 3, 21, -0.3) ENV2 (1, 1200, 0.0, 600) FT (1, 0.05) AMP (1, 1800, 0.0, 1000) REVERB ("fxa3", 0.65, 0.65, 0.4, 0.1, 1, 0.3) },
    { "Bells", "Sinos", "Sample", "Sino Sample Limpo", "sino,sample,limpo", "bell", "nucleo.bell", "", 12000,
      SMP (a) FT (1, 0.05) AMP (1, 2600, 1, 1200) REVERB ("fxa3", 0.65, 0.65, 0.4, 0.1, 1, 0.3) },
    { "Bells", "Sinos", "Carrilhão", "Carrilhão", "sino,carrilhão,agudo", "bell", "", "", 11000,
      WTB (a, 5, 0.8, 2, 6) FT (1, 0.05) AMP (1, 3000, 0.0, 1800) DELAY ("fxa2", 0.6, 0.35, 0.5, 0.6, 0.5, 0.2) REVERB ("fxa3", 0.7, 0.72, 0.4, 0.1, 1, 0.35) },
    { "Bells", "Sinos", "Camada", "Sino com Pad", "sino,pad,camada", "bell", "nucleo.bell", "nucleo.pad", 9000,
      SMP (a) SMP (b) " blend=0.45 b_level=-3" FT (1, 0.05) AMP (60, 1600, 0.6, 1600) REVERB ("fxa3", 0.75, 0.75, 0.45, 0.1, 1, 0.35) },
    { "Bells", "Sinos", "Escuro", "Sino Sombrio", "sino,sombrio,grave", "bell", "", "", 4000,
      WTB (a, 5, 0.25, 2, 12) FT (1, 0.08) AMP (1, 3200, 0.0, 2000) REVERB ("fxa3", 0.8, 0.8, 0.55, 0.12, 1, 0.4) },
    // ================================ MALLETS ================================
    { "Mallets", "Mallets", "Marimba", "Marimba Sintética", "marimba,madeira,curto", "mallet", "", "", 4500,
      ANA (a, 1, 1, 0) FT (1, 0.08) " f1_env=0.35" ENV2 (1, 250, 0.0, 100) AMP (1, 520, 0.0, 160) REVERB ("fxa3", 0.5, 0.45, 0.5, 0.06, 0.9, 0.2) },
    { "Mallets", "Mallets", "Kalimba", "Kalimba Digital", "kalimba,delicado", "mallet", "", "", 7000,
      WTB (a, 14, 0.3, 1, 0) FT (1, 0.05) AMP (1, 1100, 0.0, 400) DELAY ("fxa2", 0.55, 0.3, 0.5, 0.4, 0.5, 0.15) REVERB ("fxa3", 0.6, 0.55, 0.45, 0.08, 1, 0.25) },
    { "Mallets", "Mallets", "Xilofone", "Xilofone", "xilofone,seco,brilhante", "mallet", "", "", 7500,
      ANA (a, 3, 1, 0) " a_pwm=0.2" FT (1, 0.06) AMP (1, 320, 0.0, 100) REVERB ("fxa3", 0.5, 0.45, 0.5, 0.05, 0.9, 0.18) },
    { "Mallets", "Mallets", "Vibrafone", "Vibrafone Jazz", "vibrafone,jazz,trémolo", "mallet", "", "", 6500,
      WTB (a, 13, 0.4, 1, 0) FT (1, 0.05) AMP (1, 2200, 0.0, 900) TREM ("fxa1", 0.3, 0.35, 0.0, 0.5, 0.5, 0.7) REVERB ("fxa3", 0.6, 0.6, 0.45, 0.08, 1, 0.28) },
    { "Mallets", "Mallets", "Caixa de Música", "Caixa de Música Doce", "caixa de música,infantil,doce", "mallet", "", "", 9000,
      WTB (a, 5, 0.2, 1, 0) FT (1, 0.05) AMP (1, 1500, 0.0, 700) REVERB ("fxa3", 0.65, 0.6, 0.45, 0.08, 1, 0.3) },
    { "Mallets", "Mallets", "Glockenspiel", "Glockenspiel", "glockenspiel,agudo,metal", "mallet", "", "", 12000,
      WTB (a, 5, 0.7, 1, 0) FT (1, 0.05) AMP (1, 1700, 0.0, 800) REVERB ("fxa3", 0.65, 0.65, 0.4, 0.1, 1, 0.3) },
    { "Mallets", "Mallets", "Steel Drum", "Steel Drum Sintético", "steel drum,caraíbas", "mallet", "", "", 6000,
      ANA (a, 0, 1, 0) " a_fmamt=0.3 a_fmrat=2" MOD (1, 3, 21, -0.2) ENV2 (1, 600, 0.0, 300) FT (1, 0.05) AMP (1, 900, 0.0, 300) DELAY ("fxa2", 0.55, 0.3, 0.5, 0.4, 0.5, 0.15) },
    // ================================ GUITARRAS ================================
    { "Guitars", "Guitarras", "Nylon", "Guitarra Nylon", "guitarra,nylon,suave", "pluck", "nucleo.pluck", "", 6500,
      SMP (a) FT (1, 0.05) AMP (1, 1500, 0.0, 400) REVERB ("fxa3", 0.55, 0.5, 0.5, 0.08, 0.9, 0.22) },
    { "Guitars", "Guitarras", "Aço", "Guitarra de Aço Sintética", "guitarra,aço,brilhante", "pluck", "", "", 3500,
      ANA (a, 2, 2, 6) FT (2, 0.12) " f1_env=0.5" ENV2 (1, 450, 0.0, 200) AMP (1, 900, 0.0, 300) CHORUS ("fxa1", 0.3, 0.3, 0.5, 0.5, 1, 0.25) REVERB ("fxa3", 0.55, 0.5, 0.5, 0.08, 0.9, 0.22) },
    { "Guitars", "Guitarras", "Violão", "Violão Suave", "violão,suave,íntimo", "pluck", "", "", 3000,
      ANA (a, 1, 2, 5) FT (1, 0.08) " f1_env=0.3" ENV2 (1, 500, 0.0, 200) AMP (1, 1100, 0.0, 400) REVERB ("fxa3", 0.5, 0.45, 0.5, 0.06, 0.9, 0.2) },
    { "Guitars", "Guitarras", "Chorus", "Guitarra Limpa Chorus", "guitarra,limpa,chorus", "pluck", "nucleo.pluck", "", 8000,
      SMP (a) FT (1, 0.05) AMP (1, 1500, 0.0, 400) CHORUS ("fxa1", 0.3, 0.45, 0.5, 0.5, 1, 0.45) REVERB ("fxa3", 0.6, 0.55, 0.5, 0.08, 1, 0.25) },
    { "Guitars", "Guitarras", "Arpejo", "Arpejo de Guitarra", "guitarra,arpejo,sequência", "arp", "nucleo.pluck", "", 8000,
      SMP (a) FT (1, 0.05) AMP (1, 700, 0.0, 250) " arp_on=1 arp_rate=12 arp_mode=0 arp_pattern=0 arp_gate=0.5" DELAY ("fxa2", 0.6, 0.35, 0.5, 0.6, 0.5, 0.2) REVERB ("fxa3", 0.55, 0.5, 0.5, 0.08, 1, 0.22) },
    { "Guitars", "Guitarras", "Funky", "Guitarra Funky", "guitarra,funk,wah", "rhythm", "", "", 2500,
      ANA (a, 2, 2, 6) FT (7, 0.3) " f1_env=0.6" ENV2 (1, 160, 0.0, 80) AMP (1, 200, 0.0, 90) PHASER ("fxa1", 0.3, 0.4, 0.4, 0.5, 0.5, 0.35) },
    // ================================ ÓRGÃOS ================================
    { "Organs", "Órgãos", "Drawbar", "Órgão Drawbar", "órgão,drawbar,clássico", "keys", "", "", 9000,
      WTB (a, 2, 0.4, 1, 0) FT (1, 0.05) AMP (2, 100, 1.0, 60) TREM ("fxa1", 0.55, 0.15, 0.0, 0.5, 0.5, 0.5) REVERB ("fxa3", 0.5, 0.45, 0.5, 0.05, 0.9, 0.18) },
    { "Organs", "Órgãos", "Igreja", "Órgão de Igreja", "órgão,igreja,solene", "pad", "", "", 7000,
      WTB (a, 2, 0.7, 1, 0) FT (1, 0.05) AMP (30, 200, 1.0, 400) REVERB ("fxa3", 0.9, 0.9, 0.45, 0.12, 1, 0.5) },
    { "Organs", "Órgãos", "Rock", "Órgão Rock Sujo", "órgão,rock,sujo", "keys", "", "", 7000,
      WTB (a, 2, 0.55, 1, 0) FT (1, 0.05) AMP (2, 100, 1.0, 70) SATUR ("fxa1", 0.55, 0.55, 0.05, 0.5, 0.8, 0.6) REVERB ("fxa3", 0.45, 0.4, 0.5, 0.05, 0.9, 0.15) },
    { "Organs", "Órgãos", "Leslie", "Órgão Leslie", "órgão,leslie,rotativo", "keys", "", "", 8000,
      WTB (a, 2, 0.45, 2, 5) FT (1, 0.05) AMP (2, 100, 1.0, 80) PHASER ("fxa1", 0.4, 0.5, 0.3, 0.5, 0.6, 0.5) TREM ("fxa2", 0.5, 0.2, 0.0, 0.5, 0.5, 0.4) REVERB ("fxa3", 0.5, 0.45, 0.5, 0.05, 0.9, 0.18) },
    { "Organs", "Órgãos", "Pipas", "Órgão de Pipas", "órgão,pipas,largo", "pad", "", "", 5000,
      ANA (a, 3, 2, 4) " a_pwm=0.5" ANA (b, 3, 2, 4) " b_pwm=0.5 b_coarse=12 b_level=-6 blend=0.5" FT (1, 0.05) AMP (40, 300, 1.0, 350) REVERB ("fxa3", 0.8, 0.8, 0.45, 0.1, 1, 0.4) },
    { "Organs", "Órgãos", "Compacto", "Órgão Compacto Retro", "órgão,compacto,retro", "keys", "", "", 6000,
      ANA (a, 3, 1, 0) " a_pwm=0.35" FT (1, 0.05) AMP (3, 120, 0.95, 80) CHORUS ("fxa1", 0.3, 0.3, 0.5, 0.5, 1, 0.3) REVERB ("fxa3", 0.45, 0.4, 0.5, 0.05, 0.9, 0.15) },
    // ================================ SOPROS ================================
    { "Woodwinds", "Sopros", "Flauta", "Flauta Sintética", "flauta,sopro,suave", "lead", "", "", 5500,
      ANA (a, 1, 2, 5) " noise_level=-34 noise_color=0.6" LEGATO (50) FT (1, 0.08) MOD (1, 1, 3, 0.03) " lfo1_rate=5.3 lfo1_fade=700" AMP (60, 300, 0.9, 200) REVERB ("fxa3", 0.55, 0.5, 0.5, 0.08, 0.9, 0.22) },
    { "Woodwinds", "Sopros", "Clarinete", "Clarinete Sintético", "clarinete,sopro,oco", "lead", "", "", 3500,
      ANA (a, 3, 1, 0) " a_pwm=0.5" LEGATO (40) FT (1, 0.08) MOD (1, 1, 3, 0.02) " lfo1_rate=5 lfo1_fade=800" AMP (40, 300, 0.9, 180) REVERB ("fxa3", 0.5, 0.45, 0.5, 0.06, 0.9, 0.2) },
    { "Woodwinds", "Sopros", "Oboé", "Oboé Sintético", "oboé,sopro,nasal", "lead", "", "", 3000,
      ANA (a, 3, 1, 0) " a_pwm=0.2" LEGATO (40) FT (5, 0.25) MOD (1, 1, 3, 0.02) " lfo1_rate=5.2 lfo1_fade=700" AMP (50, 300, 0.9, 180) REVERB ("fxa3", 0.5, 0.45, 0.5, 0.06, 0.9, 0.2) },
    { "Woodwinds", "Sopros", "Sax", "Sax Sintético", "sax,sopro,jazz", "lead", "", "", 3200,
      ANA (a, 2, 2, 6) " a_fold=0.15" LEGATO (45) FT (2, 0.12) " f1_env=0.3" ENV2 (30, 300, 0.7, 150) MOD (1, 1, 3, 0.025) " lfo1_rate=5.4 lfo1_fade=600" AMP (35, 300, 0.9, 180) REVERB ("fxa3", 0.5, 0.45, 0.5, 0.06, 0.9, 0.2) },
    { "Woodwinds", "Sopros", "Ocarina", "Ocarina", "ocarina,seno,doce", "lead", "", "", 6000,
      ANA (a, 0, 1, 0) LEGATO (40) FT (1, 0.05) MOD (1, 1, 3, 0.02) " lfo1_rate=5 lfo1_fade=500" AMP (50, 300, 0.9, 200) REVERB ("fxa3", 0.55, 0.5, 0.5, 0.08, 0.9, 0.22) },
    { "Woodwinds", "Sopros", "Shakuhachi", "Shakuhachi Soproso", "shakuhachi,soproso,oriental", "lead", "", "", 4500,
      ANA (a, 0, 1, 0) " noise_level=-20 noise_color=0.5" LEGATO (60) FT (5, 0.2) MOD (1, 1, 3, 0.04) " lfo1_rate=4.6 lfo1_fade=900" AMP (90, 400, 0.85, 300) DELAY ("fxa2", 0.65, 0.4, 0.5, 0.6, 0.5, 0.25) REVERB ("fxa3", 0.65, 0.65, 0.45, 0.1, 1, 0.3) },
    // ================================ ACORDES ================================
    { "Chords", "Acordes", "Stab", "Stab Piano House", "acorde,stab,house", "chord", "nucleo.ep", "", 9000,
      SMP (a) FT (1, 0.05) AMP (1, 380, 0.0, 180) REVERB ("fxa3", 0.5, 0.45, 0.5, 0.06, 0.9, 0.2) },
    { "Chords", "Acordes", "Stab", "Stab Rave", "acorde,stab,rave", "chord", "", "", 5000,
      ANA (a, 2, 7, 24) FT (2, 0.15) " f1_env=0.4" ENV2 (2, 280, 0.0, 120) AMP (1, 300, 0.0, 140) SATUR ("fxa1", 0.3, 0.6, 0.05, 0.4, 0.8, 0.35) DELAY ("fxa2", 0.6, 0.35, 0.5, 0.6, 0.5, 0.2) },
    { "Chords", "Acordes", "Suspenso", "Acorde Suspenso", "acorde,suspenso,aberto", "chord", "", "", 3200,
      ANA (a, 2, 5, 16) ANA (b, 1, 3, 10) " b_coarse=12 b_level=-8 blend=0.5" FT (1, 0.1) AMP (200, 700, 0.8, 900) CHORUS ("fxa1", 0.2, 0.4, 0.5, 0.5, 1, 0.3) REVERB ("fxa3", 0.7, 0.7, 0.45, 0.1, 1, 0.32) },
    { "Chords", "Acordes", "Jazz", "Acorde Jazz EP", "acorde,jazz,piano elétrico", "chord", "nucleo.ep", "", 8000,
      SMP (a) FT (1, 0.05) AMP (3, 1400, 0.4, 500) CHORUS ("fxa1", 0.25, 0.3, 0.5, 0.5, 1, 0.3) REVERB ("fxa3", 0.6, 0.55, 0.5, 0.08, 1, 0.25) },
    { "Chords", "Acordes", "Dub", "Stab Dub", "acorde,stab,dub", "chord", "", "", 1800,
      ANA (a, 2, 3, 10) FT (7, 0.3) " f1_env=0.5" ENV2 (2, 260, 0.0, 120) AMP (1, 280, 0.0, 120) DELAY ("fxa2", 0.66, 0.6, 0.45, 0.8, 0.5, 0.45) REVERB ("fxa3", 0.6, 0.6, 0.45, 0.1, 1, 0.3) },
    { "Chords", "Acordes", "Soul", "Acorde Neo-Soul", "acorde,soul,quente", "chord", "nucleo.ep", "nucleo.pad", 7000,
      SMP (a) SMP (b) " blend=0.3 b_level=-6" FT (1, 0.05) AMP (20, 1400, 0.6, 800) CHORUS ("fxa1", 0.25, 0.35, 0.5, 0.5, 1, 0.3) REVERB ("fxa3", 0.62, 0.6, 0.5, 0.08, 1, 0.28) },
    // ================================ SEQUÊNCIAS ================================
    { "Sequences", "Sequências", "Pulso", "Sequência Pulso", "sequência,pulso,eletrónico", "arp", "", "", 3000,
      ANA (a, 3, 2, 6) " a_pwm=0.4" FT (2, 0.2) " f1_env=0.4" ENV2 (1, 220, 0.0, 100) AMP (1, 260, 0.0, 100) " arp_on=1 arp_rate=12 arp_mode=0 arp_pattern=2 arp_gate=0.5" DELAY ("fxa2", 0.6, 0.35, 0.5, 0.6, 0.5, 0.22) },
    { "Sequences", "Sequências", "Trance", "Sequência Trance", "sequência,trance,gate", "arp", "", "", 3500,
      ANA (a, 2, 5, 16) FT (2, 0.2) " f1_env=0.4" ENV2 (1, 250, 0.0, 100) AMP (1, 280, 0.0, 100) " arp_on=1 arp_rate=13 arp_mode=0 arp_pattern=1 arp_gate=0.45" DELAY ("fxa2", 0.66, 0.4, 0.5, 0.6, 0.5, 0.28) REVERB ("fxa3", 0.6, 0.55, 0.5, 0.08, 1, 0.22) },
    { "Sequences", "Sequências", "Sinos", "Sequência de Sinos", "sequência,sinos,etérea", "arp", "", "", 9000,
      WTB (a, 5, 0.5, 1, 0) FT (1, 0.05) AMP (1, 900, 0.0, 400) " arp_on=1 arp_rate=12 arp_mode=2 arp_pattern=0 arp_gate=0.5" DELAY ("fxa2", 0.6, 0.4, 0.5, 0.6, 0.5, 0.3) REVERB ("fxa3", 0.7, 0.7, 0.4, 0.1, 1, 0.32) },
    { "Sequences", "Sequências", "Sub", "Sequência Sub", "sequência,sub,grave", "arp", "", "", 900,
      ANA (a, 0, 1, 0) FT (1, 0.05) AMP (1, 300, 0.0, 100) " arp_on=1 arp_rate=12 arp_mode=0 arp_pattern=3 arp_gate=0.55" SATUR ("fxa1", 0.2, 0.5, 0, 0.3, 0.8, 0.3) },
    { "Sequences", "Sequências", "Pluck", "Sequência Pluck", "sequência,pluck,melódica", "arp", "nucleo.pluck", "", 8000,
      SMP (a) FT (1, 0.05) AMP (1, 600, 0.0, 200) " arp_on=1 arp_rate=13 arp_mode=3 arp_pattern=4 arp_gate=0.5" DELAY ("fxa2", 0.6, 0.35, 0.5, 0.6, 0.5, 0.22) },
    { "Sequences", "Sequências", "Acid", "Sequência Acid", "sequência,acid,ressonante", "arp", "", "", 900,
      ANA (a, 2, 1, 0) FT (7, 0.6) " f1_env=0.7" ENV2 (1, 200, 0.0, 80) AMP (1, 250, 0.0, 80) " arp_on=1 arp_rate=13 arp_mode=0 arp_pattern=5 arp_gate=0.5" SATUR ("fxa1", 0.4, 0.6, 0.05, 0.5, 0.8, 0.4) },
    // ================================ TEXTURAS ================================
    { "Textures", "Texturas", "Vento", "Textura Vento", "textura,vento,ruído", "texture", "", "", 2500,
      ANA (a, 0, 1, 0) " a_level=-30 noise_level=-8 noise_color=0.3" FT (5, 0.6) MOD (1, 1, 1, 0.4) " lfo1_rate=0.15" AMP (900, 800, 0.9, 1400) REVERB ("fxa3", 0.8, 0.8, 0.5, 0.1, 1, 0.4) },
    { "Textures", "Texturas", "Névoa", "Névoa Digital", "textura,névoa,digital", "texture", "", "", 4000,
      WTB (a, 15, 0.4, 3, 20) FT (1, 0.1) MOD (1, 1, 7, 0.3) " lfo1_rate=0.12 lfo1_wave=1" AMP (800, 900, 0.9, 1500) CHORUS ("fxa1", 0.2, 0.4, 0.5, 0.5, 1, 0.3) REVERB ("fxa3", 0.8, 0.8, 0.45, 0.12, 1, 0.4) },
    { "Textures", "Texturas", "Cristais", "Cristais Suspensos", "textura,cristais,brilhante", "texture", "", "", 9000,
      WTB (a, 5, 0.6, 3, 24) FT (1, 0.08) MOD (1, 1, 7, 0.25) " lfo1_rate=0.2 lfo1_wave=1" AMP (700, 900, 0.8, 1600) DELAY ("fxa2", 0.7, 0.5, 0.5, 0.7, 0.5, 0.3) REVERB ("fxa3", 0.85, 0.85, 0.4, 0.12, 1, 0.45) },
    { "Textures", "Texturas", "Drone", "Drone Profundo", "textura,drone,grave", "drone", "", "", 1200,
      ANA (a, 2, 4, 12) " sub_level=-8" FT (2, 0.2) MOD (1, 1, 1, 0.3) " lfo1_rate=0.08" AMP (1200, 900, 1.0, 1800) REVERB ("fxa3", 0.8, 0.8, 0.5, 0.12, 1, 0.35) },
    { "Textures", "Texturas", "Areia", "Areia Fina", "textura,areia,ruído", "texture", "", "", 7000,
      ANA (a, 0, 1, 0) " a_level=-40 noise_level=-12 noise_color=0.85" FT (5, 0.3) MOD (1, 1, 1, 0.3) " lfo1_rate=0.3" AMP (500, 700, 0.9, 900) WIDTH ("fxa1", 0.8, 0.3, 0.5, 0.5, 0.55, 1) REVERB ("fxa3", 0.7, 0.7, 0.5, 0.1, 1, 0.3) },
    { "Textures", "Texturas", "Atmosfera", "Atmosfera Etérea", "textura,atmosfera,etérea", "pad", "", "", 5000,
      WTB (a, 10, 0.5, 4, 18) FT (1, 0.08) MOD (1, 1, 7, 0.2) " lfo1_rate=0.1 lfo1_wave=1" AMP (1000, 900, 0.9, 1800) CHORUS ("fxa1", 0.2, 0.4, 0.5, 0.5, 1, 0.3) REVERB ("fxa3", 0.85, 0.85, 0.45, 0.12, 1, 0.45) },
    // ================================ CINEMÁTICO ================================
    { "Cinematic", "FX", "Impacto", "Boom Sub Impacto", "cinema,impacto,boom", "hit", "nucleo.impact", "", 6000,
      SMP (a) FT (1, 0.05) AMP (1, 3000, 1, 300) REVERB ("fxa3", 0.8, 0.8, 0.5, 0.1, 1, 0.35) },
    { "Cinematic", "FX", "Riser", "Riser Tensão", "cinema,riser,tensão", "riser", "nucleo.riser", "", 8000,
      SMP (a) FT (1, 0.05) AMP (1, 3000, 1, 300) REVERB ("fxa3", 0.7, 0.7, 0.5, 0.1, 1, 0.3) },
    { "Cinematic", "Cinemático", "Drone", "Drone Cinemático", "cinema,drone,escuro", "drone", "", "", 1500,
      ANA (a, 2, 6, 20) ANA (b, 2, 4, 14) " b_coarse=-12 b_level=-3 blend=0.5 sub_level=-10" FT (2, 0.2) MOD (1, 1, 1, 0.3) " lfo1_rate=0.07" AMP (1500, 900, 1.0, 2200) REVERB ("fxa3", 0.85, 0.85, 0.5, 0.12, 1, 0.4) },
    { "Cinematic", "Cinemático", "Pulso", "Pulso Ominoso", "cinema,pulso,ominoso", "arp", "", "", 1800,
      ANA (a, 2, 3, 10) " sub_level=-8" FT (2, 0.2) " f1_env=0.4" ENV2 (1, 300, 0.0, 120) AMP (1, 350, 0.0, 140) " arp_on=1 arp_rate=11 arp_mode=0 arp_pattern=1 arp_gate=0.6" REVERB ("fxa3", 0.75, 0.75, 0.5, 0.1, 1, 0.35) },
    { "Cinematic", "Cinemático", "Pad", "Pad Épico", "cinema,pad,épico", "pad", "", "", 3500,
      ANA (a, 2, 8, 26) WTB (b, 10, 0.5, 4, 18) " b_level=-6 blend=0.5" FT (1, 0.1) AMP (900, 900, 0.9, 1800) CHORUS ("fxa1", 0.2, 0.4, 0.5, 0.5, 1, 0.3) REVERB ("fxa3", 0.85, 0.85, 0.45, 0.12, 1, 0.45) },
    { "Cinematic", "Cinemático", "Hit", "Hit Orquestral Sintético", "cinema,hit,orquestral", "hit", "", "", 3000,
      ANA (a, 2, 6, 18) " sub_level=-8" FT (2, 0.12) " f1_env=0.5" ENV2 (2, 500, 0.0, 300) AMP (2, 900, 0.0, 500) REVERB ("fxa3", 0.8, 0.8, 0.45, 0.1, 1, 0.4) },
    // ================================ HÍBRIDO ================================
    { "Hybrid", "Híbrido", "Piano+Synth", "Piano com Pad Sintético", "híbrido,piano,pad", "keys", "nucleo.ep", "", 8000,
      SMP (a) ANA (b, 2, 4, 14) " b_level=-8 blend=0.4" FT (1, 0.05) AMP (30, 1200, 0.6, 900) CHORUS ("fxa1", 0.25, 0.3, 0.5, 0.5, 1, 0.3) REVERB ("fxa3", 0.65, 0.62, 0.5, 0.08, 1, 0.3) },
    { "Hybrid", "Híbrido", "Voz+Saw", "Voz com Serra", "híbrido,voz,serra", "lead", "nucleo.vox", "", 6000,
      SMP (a) ANA (b, 2, 3, 10) " b_level=-9 blend=0.35" FT (1, 0.08) AMP (60, 600, 0.85, 500) REVERB ("fxa3", 0.6, 0.6, 0.45, 0.1, 1, 0.3) },
    { "Hybrid", "Híbrido", "Sino+Pluck", "Sino com Pluck", "híbrido,sino,pluck", "pluck", "nucleo.bell", "", 9000,
      SMP (a) ANA (b, 1, 2, 6) " b_level=-8 blend=0.4" FT (1, 0.05) " f1_env=0.2" ENV2 (1, 400, 0.0, 150) AMP (1, 1400, 0.0, 600) DELAY ("fxa2", 0.6, 0.35, 0.5, 0.6, 0.5, 0.2) REVERB ("fxa3", 0.65, 0.65, 0.4, 0.1, 1, 0.3) },
    { "Hybrid", "Híbrido", "Stab+Pad", "Stab com Pad Digital", "híbrido,stab,pad", "chord", "nucleo.stab", "", 7000,
      SMP (a) WTB (b, 10, 0.4, 3, 12) " b_level=-6 blend=0.4" FT (1, 0.05) AMP (4, 700, 0.3, 500) CHORUS ("fxa1", 0.25, 0.3, 0.5, 0.5, 1, 0.3) REVERB ("fxa3", 0.6, 0.6, 0.45, 0.1, 1, 0.28) },
    { "Hybrid", "Híbrido", "Pad Vocal", "Pad Vocal Híbrido", "híbrido,pad,vocal", "pad", "nucleo.vox", "nucleo.pad", 6000,
      SMP (a) SMP (b) " blend=0.5 b_level=-3" FT (1, 0.05) AMP (400, 900, 0.9, 1200) CHORUS ("fxa1", 0.2, 0.35, 0.5, 0.5, 1, 0.3) REVERB ("fxa3", 0.75, 0.75, 0.45, 0.1, 1, 0.4) },
    { "Hybrid", "Híbrido", "Sub+Saw", "Sub com Serra Filtrada", "híbrido,sub,serra", "bass", "nucleo.sub", "", 1200,
      SMP (a) ANA (b, 2, 1, 0) " b_level=-10 blend=0.3" MONO (30) FT (2, 0.15) AMP (3, 300, 0.9, 140) SATUR ("fxa1", 0.3, 0.5, 0.05, 0.4, 0.8, 0.4) },
    // ================================ MODELAÇÃO FÍSICA (motor Corda) ================================
    { "Guitars", "Guitarras", "Corda", "Corda Nylon Física", "corda,nylon,física", "pluck", "", "", 5000,
      " a_on=1 a_type=3 a_ks_damp=0.55 a_ks_decay=0.55 a_ks_pick=0.35" FT (1, 0.05) AMP (1, 4000, 1.0, 350) REVERB ("fxa3", 0.55, 0.5, 0.5, 0.08, 0.9, 0.22) },
    { "Guitars", "Guitarras", "Corda", "Corda Aço Física", "corda,aço,brilhante", "pluck", "", "", 9000,
      " a_on=1 a_type=3 a_ks_damp=0.2 a_ks_decay=0.7 a_ks_pick=0.25" FT (1, 0.05) AMP (1, 4000, 1.0, 400) CHORUS ("fxa1", 0.3, 0.3, 0.5, 0.5, 1, 0.25) REVERB ("fxa3", 0.55, 0.5, 0.5, 0.08, 0.9, 0.22) },
    { "Plucks", "Pluck", "Corda", "Harpa Física", "harpa,corda,etérea", "pluck", "", "", 8000,
      " a_on=1 a_type=3 a_ks_damp=0.3 a_ks_decay=0.8 a_ks_pick=0.5" FT (1, 0.05) AMP (1, 4000, 1.0, 600) DELAY ("fxa2", 0.6, 0.35, 0.5, 0.6, 0.5, 0.2) REVERB ("fxa3", 0.7, 0.7, 0.45, 0.1, 1, 0.35) },
    { "Plucks", "Pluck", "Corda", "Banjo Físico", "banjo,corda,seco", "pluck", "", "", 9000,
      " a_on=1 a_type=3 a_ks_damp=0.05 a_ks_decay=0.35 a_ks_pick=0.15" FT (1, 0.05) AMP (1, 2500, 1.0, 200) REVERB ("fxa3", 0.4, 0.35, 0.5, 0.05, 0.9, 0.15) },
    { "Plucks", "Pluck", "Corda", "Alaúde Físico", "alaúde,corda,medieval", "pluck", "", "", 6000,
      " a_on=1 a_type=3 a_ks_damp=0.45 a_ks_decay=0.6 a_ks_pick=0.4" FT (1, 0.05) AMP (1, 4000, 1.0, 350) REVERB ("fxa3", 0.6, 0.55, 0.5, 0.08, 1, 0.25) },
    { "Plucks", "Pluck", "Corda", "Koto Físico", "koto,corda,oriental", "pluck", "", "", 7000,
      " a_on=1 a_type=3 a_ks_damp=0.35 a_ks_decay=0.65 a_ks_pick=0.1" FT (1, 0.05) AMP (1, 4000, 1.0, 300) DELAY ("fxa2", 0.6, 0.3, 0.5, 0.6, 0.5, 0.15) REVERB ("fxa3", 0.6, 0.55, 0.5, 0.08, 1, 0.25) },
    { "Bass", "Baixo", "Corda", "Baixo de Corda Físico", "baixo,corda,acústico", "bass", "", "", 2500,
      " a_on=1 a_type=3 a_ks_damp=0.7 a_ks_decay=0.55 a_ks_pick=0.4" MONO (20) FT (1, 0.05) AMP (1, 4000, 1.0, 250) SATUR ("fxa1", 0.2, 0.5, 0, 0.3, 0.8, 0.3) },
    { "Strings", "Cordas", "Corda", "Corda Dedilhada Longa", "corda,dedilhada,longa", "pad", "", "", 6000,
      " a_on=1 a_type=3 a_ks_damp=0.5 a_ks_decay=0.95 a_ks_pick=0.45" FT (1, 0.05) AMP (1, 4000, 1.0, 1200) CHORUS ("fxa1", 0.2, 0.35, 0.5, 0.5, 1, 0.3) REVERB ("fxa3", 0.75, 0.75, 0.45, 0.1, 1, 0.35) },
    // ================================ SPLIT / LAYER (zonas de teclas por motor) ================================
    { "Hybrid", "Híbrido", "Split", "Split Baixo e Piano", "split,baixo,piano", "split", "", "nucleo.ep", 4000,
      ANA (a, 2, 1, 0) " a_keyhi=59 sub_level=-8" SMP (b) " b_keylo=60 blend=0.5" MONO (20) FT (1, 0.1) AMP (3, 900, 0.7, 300) REVERB ("fxa3", 0.55, 0.5, 0.5, 0.08, 1, 0.22) },
    { "Hybrid", "Híbrido", "Split", "Split Baixo e Lead", "split,baixo,lead", "split", "", "", 3500,
      ANA (a, 0, 1, 0) " a_keyhi=52" ANA (b, 2, 3, 12) " b_keylo=53 b_level=-6 blend=0.5" FT (2, 0.12) AMP (3, 500, 0.8, 200) DELAY ("fxa2", 0.6, 0.3, 0.5, 0.6, 0.5, 0.18) },
    { "Hybrid", "Híbrido", "Split", "Split Cordas e Sino", "split,cordas,sino", "split", "", "nucleo.bell", 4500,
      ANA (a, 2, 5, 16) " a_keyhi=71" SMP (b) " b_keylo=72 blend=0.5" FT (1, 0.08) AMP (60, 1200, 0.8, 800) CHORUS ("fxa1", 0.25, 0.35, 0.5, 0.5, 1, 0.28) REVERB ("fxa3", 0.7, 0.7, 0.45, 0.1, 1, 0.32) },
    { "Hybrid", "Híbrido", "Split", "Split Corda e Pad", "split,corda,pad", "split", "", "nucleo.pad", 5000,
      " a_on=1 a_type=3 a_ks_damp=0.4 a_ks_decay=0.6 a_ks_pick=0.3 a_keyhi=64" SMP (b) " b_keylo=65 blend=0.5" FT (1, 0.05) AMP (80, 4000, 1.0, 900) REVERB ("fxa3", 0.65, 0.65, 0.45, 0.1, 1, 0.3) },
    { "Hybrid", "Híbrido", "Layer", "Layer Corda e Serra", "layer,corda,serra", "layer", "", "", 5000,
      " a_on=1 a_type=3 a_ks_damp=0.35 a_ks_decay=0.6 a_ks_pick=0.3" ANA (b, 2, 3, 12) " b_level=-10 blend=0.5" FT (1, 0.08) AMP (2, 4000, 0.9, 500) CHORUS ("fxa1", 0.3, 0.3, 0.5, 0.5, 1, 0.25) REVERB ("fxa3", 0.6, 0.55, 0.5, 0.08, 1, 0.25) },
    { "Hybrid", "Híbrido", "Layer", "Layer Piano e Cordas", "layer,piano,cordas", "layer", "nucleo.ep", "", 6000,
      SMP (a) ANA (b, 2, 5, 16) " b_level=-9 blend=0.35" FT (1, 0.08) AMP (20, 1200, 0.7, 800) CHORUS ("fxa1", 0.25, 0.3, 0.5, 0.5, 1, 0.28) REVERB ("fxa3", 0.65, 0.62, 0.5, 0.08, 1, 0.28) },
    { "Hybrid", "Híbrido", "Layer", "Layer Sino e Pad Etéreo", "layer,sino,pad", "layer", "", "", 7000,
      WTB (a, 5, 0.5, 2, 8) WTB (b, 10, 0.4, 4, 16) " b_level=-5 blend=0.45" FT (1, 0.05) AMP (120, 1600, 0.7, 1500) REVERB ("fxa3", 0.8, 0.8, 0.45, 0.12, 1, 0.4) },
    // ================================ EXPERIMENTAL ================================
    { "Experimental", "Experimental", "FM", "FM Caos Controlado", "experimental,fm,caos", "lead", "", "", 6000,
      ANA (a, 0, 1, 0) " a_fmamt=0.7 a_fmrat=3" MOD (1, 1, 21, 0.3) " lfo1_rate=0.4" FT (1, 0.1) AMP (10, 500, 0.8, 400) DELAY ("fxa2", 0.6, 0.4, 0.5, 0.6, 0.5, 0.25) },
    { "Experimental", "Experimental", "Dobra", "Dobra Total", "experimental,dobra,metálico", "lead", "", "", 6000,
      ANA (a, 1, 2, 8) " a_fold=0.9" MOD (1, 1, 30, 0.3) " lfo1_rate=0.25" FT (1, 0.1) AMP (10, 500, 0.8, 400) },
    { "Experimental", "Experimental", "Sync", "Sync Sirene", "experimental,sync,sirene", "lead", "", "", 7000,
      ANA (a, 2, 1, 0) " a_sync=18" MOD (1, 1, 1, 0.4) " lfo1_rate=0.5" FT (2, 0.15) AMP (5, 500, 0.8, 300) DELAY ("fxa2", 0.6, 0.35, 0.5, 0.6, 0.5, 0.22) },
    { "Experimental", "Experimental", "Bitcrush", "Pad Bitcrush", "experimental,bitcrush,lo-fi", "pad", "", "", 4000,
      ANA (a, 2, 5, 16) FT (1, 0.1) AMP (400, 800, 0.9, 900) CRUSH ("fxa1", 0.55, 0.45, 0.1, 0.8, 0.7, 0.6) REVERB ("fxa3", 0.7, 0.7, 0.5, 0.1, 1, 0.3) },
    { "Experimental", "Experimental", "Pente", "Pente Ressonante", "experimental,pente,metálico", "texture", "", "", 2500,
      ANA (a, 2, 2, 8) " noise_level=-26" FT (8, 0.7) MOD (1, 1, 1, 0.4) " lfo1_rate=0.2" AMP (120, 800, 0.9, 700) REVERB ("fxa3", 0.65, 0.65, 0.5, 0.1, 1, 0.3) },
    { "Experimental", "Experimental", "Formante", "Formante Alienígena", "experimental,formante,alien", "lead", "", "", 2500,
      ANA (a, 2, 2, 8) FT (9, 0.5) MOD (1, 1, 1, 0.5) " lfo1_rate=0.35" AMP (60, 600, 0.85, 500) DELAY ("fxa2", 0.6, 0.4, 0.5, 0.6, 0.5, 0.25) REVERB ("fxa3", 0.6, 0.6, 0.5, 0.1, 1, 0.25) },
    };

    struct Var { const char* suffix; float cutMul; float resAdd; const char* tag; };
    static const Var V[] = { { "", 1.0f, 0.0f, "" }, { " Suave", 0.55f, 0.0f, "suave" }, { " Brilhante", 1.8f, 0.0f, "brilhante" } };

    std::vector<Preset> out;
    for (const auto& r : R)
        for (const auto& v : V)
        {
            const float cut = std::min (20000.0f, std::max (60.0f, r.cut * v.cutMul));
            char cutTxt[48]; std::snprintf (cutTxt, sizeof cutTxt, " f1_cut=%.1f", (double) cut);
            const std::string name = std::string (r.name) + v.suffix, kv = std::string (r.kv) + cutTxt;
            std::string tags = r.tags; if (v.tag[0]) tags += std::string (",") + v.tag;
            const Row row { name.c_str(), r.cat, tags.c_str(), "nucleo", r.smpA, r.smpB, kv.c_str() };
            Preset p = make (row, "multi");
            p.family = r.family; p.subfamily = r.sub; p.role = r.role; p.qa = "auto";
            const bool aS = p.get (P_A_TYPE) == 2.0f, bOn = p.get (P_B_ON) > 0.5f;
            p.engine = p.get (P_A_TYPE) == 3.0f ? "physical" : (aS && bOn) ? "hybrid" : aS ? "sample" : p.get (P_A_TYPE) == 1.0f ? "wavetable" : bOn && p.get (P_B_TYPE) != p.get (P_A_TYPE) ? "hybrid" : "analog";
            out.push_back (std::move (p));
        }
    return out;
}
#undef FT
