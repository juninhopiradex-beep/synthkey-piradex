// =============================================================================
//  SYNTHKEY by Piradex — presets de fábrica, ronda 2 (incluído dentro de PresetBuilder.h,
//  antes dos #undef das macros). Núcleo +51 · cada estilo +11.
// =============================================================================

inline std::vector<Preset> nucleoMore()
{
    static const Row R[] = {
    // ---- leads
    { "Supersaw Anthem", "Lead", "lead,supersaw,hino", "nucleo", "", "",
      ANA (a, 2, 8, 38) ANA (b, 2, 6, 28) " b_coarse=12 b_level=-8 blend=0.5" FIL1E (2, 6500, 0.15, 0.3) ENV2 (3, 500, 0.4, 300) AMP (5, 300, 0.8, 350)
      CHORUS ("fxa1", 0.3, 0.4, 0.5, 0.6, 1, 0.3) DELAY ("fxa2", 0.6, 0.35, 0.55, 0.6, 0.5, 0.22) REVERB ("fxa3", 0.6, 0.6, 0.45, 0.1, 1, 0.25) },
    { "FM Pluck Lead", "Lead", "lead,fm,pluck", "nucleo", "", "",
      ANA (a, 0, 1, 0) " a_fmamt=0.3 a_fmrat=2" MOD (1, 3, 21, 0.35) ENV2 (1, 380, 0.0, 200) FIL1 (1, 9000, 0.05) AMP (1, 650, 0.35, 320)
      DELAY ("fxa2", 0.6, 0.34, 0.5, 0.6, 0.5, 0.25) REVERB ("fxa3", 0.55, 0.5, 0.5, 0.08, 1, 0.2) },
    { "Lead Vogal", "Lead", "lead,vogal,expressivo", "nucleo", "", "",
      WTB (a, 3, 0.5, 2, 8) LEGATO (60) FIL1 (1, 6000, 0.1) MOD (1, 7, 7, 0.5) MOD (2, 1, 3, 0.03) " lfo1_rate=5.5 lfo1_fade=700" AMP (25, 400, 0.85, 320)
      REVERB ("fxa3", 0.55, 0.55, 0.45, 0.08, 0.9, 0.2) },
    { "Lead Assobio", "Lead", "lead,assobio,solo", "nucleo", "", "",
      ANA (a, 1, 1, 0) " a_fold=0.12" LEGATO (90) FIL1 (1, 5500, 0.05) MOD (1, 1, 3, 0.05) " lfo1_rate=5.6 lfo1_fade=600" AMP (30, 300, 0.9, 260)
      DELAY ("fxa2", 0.6, 0.3, 0.5, 0.5, 0.5, 0.2) REVERB ("fxa3", 0.6, 0.55, 0.5, 0.1, 1, 0.25) },
    { "Lead Dobrado", "Lead", "lead,fold,agressivo", "nucleo", "", "",
      ANA (a, 0, 3, 10) " a_fold=0.5" MOD (1, 3, 30, 0.25) ENV2 (1, 500, 0.3, 250) FIL1 (1, 8000, 0.1) AMP (2, 400, 0.8, 200)
      SATUR ("fxa1", 0.3, 0.6, 0.05, 0.5, 0.8, 0.4) DELAY ("fxa2", 0.6, 0.3, 0.5, 0.6, 0.5, 0.2) },
    { "Pulso Duplo", "Lead", "lead,pulso,pwm", "nucleo", "", "",
      ANA (a, 3, 2, 12) " a_pwm=0.35" ANA (b, 3, 2, 12) " b_pwm=0.65 b_coarse=7 b_level=-8 blend=0.5" FIL1 (2, 4500, 0.15)
      MOD (1, 1, 19, 0.3) MOD (2, 1, 20, -0.3) " lfo1_rate=0.6" AMP (3, 350, 0.75, 220) CHORUS ("fxa1", 0.3, 0.4, 0.5, 0.5, 0.9, 0.3) },
    { "Lead 8-Bit", "Lead", "lead,chip,retro", "nucleo", "", "",
      ANA (a, 3, 1, 0) " a_pwm=0.25" MONO (5) FIL1 (0, 20000, 0.1) AMP (1, 200, 0.9, 60) CRUSH ("fxa1", 0.45, 0.5, 0.1, 0.8, 0.7, 0.5) },
    { "Seno Suave", "Lead", "lead,seno,suave", "nucleo", "", "",
      ANA (a, 0, 2, 6) FIL1 (1, 5000, 0.05) AMP (20, 300, 0.85, 500) CHORUS ("fxa1", 0.25, 0.3, 0.5, 0.5, 1, 0.3) REVERB ("fxa3", 0.6, 0.6, 0.5, 0.1, 1, 0.3) },
    // ---- baixos
    { "Wobble Bass", "Baixo", "baixo,wobble,dubstep", "nucleo", "", "",
      ANA (a, 2, 2, 14) MONO (25) FIL1 (2, 500, 0.4) MOD (1, 1, 11, 0.6) " lfo1_rate=3 lfo1_wave=0 lfo1_sync=1 lfo1_div=9" AMP (2, 200, 0.9, 100)
      SATUR ("fxa1", 0.4, 0.5, 0.05, 0.5, 0.8, 0.5) },
    { "Growl FM", "Baixo", "baixo,growl,fm", "nucleo", "", "",
      ANA (a, 0, 1, 0) " a_fmamt=0.6 a_fmrat=1" MONO (20) MOD (1, 2, 21, 0.3) " lfo2_rate=6" FIL1 (2, 1400, 0.3) AMP (2, 250, 0.85, 120)
      SATUR ("fxa1", 0.5, 0.5, 0.05, 0.6, 0.7, 0.6) },
    { "Sub Quadrado", "Baixo", "baixo,sub,quadrado", "nucleo", "", "",
      ANA (a, 3, 1, 0) " a_pwm=0.5" MONO (25) FIL1 (1, 420, 0.1) AMP (3, 250, 0.9, 130) },
    { "Acid 303", "Baixo", "baixo,acid,303", "nucleo", "", "",
      ANA (a, 2, 1, 0) MONO (40) " glide_mode=1" FIL1E (7, 600, 0.75, 0.8) ENV2 (1, 260, 0.0, 90) AMP (1, 300, 0.75, 90)
      SATUR ("fxa1", 0.45, 0.55, 0.05, 0.6, 0.75, 0.5) DELAY ("fxa2", 0.6, 0.35, 0.5, 0.5, 0.5, 0.2) },
    { "Baixo Saw Gordo", "Baixo", "baixo,saw,gordo", "nucleo", "", "",
      ANA (a, 2, 4, 24) ANA (b, 3, 1, 0) " b_coarse=-12 b_level=-4 blend=0.4" MONO (20) FIL1E (2, 900, 0.2, 0.4) ENV2 (1, 280, 0.2, 150) AMP (2, 300, 0.85, 140)
      SATUR ("fxa1", 0.35, 0.5, 0.05, 0.5, 0.8, 0.4) },
    { "Baixo Pluck", "Baixo", "baixo,pluck,curto", "nucleo", "", "",
      ANA (a, 2, 2, 8) MONO (15) FIL1E (2, 500, 0.25, 0.7) ENV2 (1, 200, 0.0, 100) AMP (1, 300, 0.0, 150) SATUR ("fxa1", 0.25, 0.5, 0, 0.4, 0.8, 0.35) },
    { "Baixo Funk", "Baixo", "baixo,funk,slap", "nucleo", "", "",
      ANA (a, 3, 1, 0) " a_pwm=0.35" MONO (10) FIL1E (2, 700, 0.3, 0.6) ENV2 (1, 180, 0.1, 120) AMP (1, 260, 0.6, 120) COMP ("fxa1", 0.4, 0.5, 0.3, 0.4, 0.4, 1) },
    { "Sub Wavetable", "Baixo", "baixo,sub,wavetable", "nucleo", "", "",
      WTB (a, 12, 0.3, 1, 0) MONO (25) FIL1 (1, 1200, 0.05) AMP (3, 250, 0.9, 140) SATUR ("fxa1", 0.2, 0.5, 0, 0.3, 0.8, 0.3) },
    { "Baixo Distorcido", "Baixo", "baixo,distorção,pesado", "nucleo", "", "",
      ANA (a, 2, 3, 16) MONO (20) FIL1 (2, 800, 0.3) AMP (2, 300, 0.85, 140) SATUR ("fxa1", 0.8, 0.5, 0.1, 0.8, 0.6, 0.8) COMP ("fxa2", 0.35, 0.5, 0.3, 0.4, 0.3, 1) },
    { "Dub Sub", "Baixo", "baixo,dub,sub", "nucleo", "", "",
      ANA (a, 0, 1, 0) " sub_level=-6" MONO (80) AMP (5, 800, 0.9, 500) FIL1 (1, 700, 0.05) DELAY ("fxa2", 0.66, 0.5, 0.4, 0.7, 0.5, 0.3) REVERB ("fxa3", 0.6, 0.6, 0.4, 0.1, 1, 0.2) },
    // ---- teclas
    { "Piano Digital", "Teclas", "teclas,piano,digital", "nucleo", "", "",
      WTB (a, 13, 0.2, 1, 0) FIL1 (1, 9000, 0.05) MOD (1, 5, 7, 0.5) AMP (1, 1800, 0.2, 500) REVERB ("fxa3", 0.55, 0.5, 0.5, 0.08, 1, 0.2) },
    { "Clav Funky", "Teclas", "teclas,clav,funk", "nucleo", "", "",
      ANA (a, 3, 1, 0) " a_pwm=0.2" FIL1E (2, 2500, 0.2, 0.5) ENV2 (1, 200, 0.0, 100) AMP (1, 300, 0.0, 120) PHASER ("fxa1", 0.3, 0.5, 0.3, 0.5, 0.5, 0.3) },
    { "Vibrafone", "Teclas", "teclas,vibrafone,mallet", "nucleo", "", "",
      WTB (a, 5, 0.15, 1, 0) AMP (1, 2200, 0.0, 1200) TREM ("fxa1", 0.3, 0.45, 0.0, 0.5, 0.5, 0.6) REVERB ("fxa3", 0.6, 0.6, 0.4, 0.1, 1, 0.28) },
    { "Caixa de Música", "Teclas", "teclas,música,delicado", "nucleo", "", "",
      WTB (a, 14, 0.4, 1, 0) " transpose=12" AMP (1, 1200, 0.0, 900) DELAY ("fxa2", 0.66, 0.4, 0.5, 0.6, 0.5, 0.25) REVERB ("fxa3", 0.65, 0.65, 0.4, 0.12, 1, 0.3) },
    { "Teclas Pulso", "Teclas", "teclas,pulso,quente", "nucleo", "", "",
      ANA (a, 3, 3, 14) " a_pwm=0.3" FIL1 (1, 5000, 0.1) AMP (2, 500, 0.5, 300) CHORUS ("fxa1", 0.3, 0.45, 0.5, 0.5, 1, 0.4) },
    { "EP Suave Chorus", "Teclas", "teclas,ep,chorus", "nucleo", "", "",
      WTB (a, 13, 0.5, 2, 6) FIL1 (1, 6500, 0.05) AMP (1, 1500, 0.45, 450) CHORUS ("fxa1", 0.25, 0.55, 0.5, 0.6, 1, 0.5) REVERB ("fxa3", 0.55, 0.5, 0.5, 0.08, 0.9, 0.2) },
    { "Cravo", "Teclas", "teclas,cravo,barroco", "nucleo", "", "",
      ANA (a, 2, 2, 5) FIL1 (2, 5200, 0.1) AMP (1, 350, 0.0, 120) REVERB ("fxa3", 0.45, 0.4, 0.5, 0.05, 0.9, 0.18) },
    { "Órgão Gospel", "Teclas", "teclas,órgão,gospel", "nucleo", "", "",
      WTB (a, 2, 0.8, 1, 0) FIL1 (1, 7500, 0.05) AMP (3, 200, 0.95, 90) CHORUS ("fxa1", 0.35, 0.35, 0.5, 0.5, 1, 0.5) REVERB ("fxa3", 0.5, 0.45, 0.5, 0.05, 0.9, 0.2) },
    // ---- pads
    { "Pad Quente", "Pad", "pad,quente,analógico", "nucleo", "", "",
      ANA (a, 2, 6, 26) FIL1 (2, 2400, 0.1) AMP (600, 800, 0.85, 1500) CHORUS ("fxa1", 0.2, 0.5, 0.5, 0.6, 1, 0.4) REVERB ("fxa3", 0.65, 0.65, 0.45, 0.1, 1, 0.3) },
    { "Coro Digital", "Pad", "pad,coro,digital", "nucleo", "", "",
      WTB (a, 3, 0.3, 4, 14) MOD (1, 1, 7, 0.3) " lfo1_rate=0.2 lfo1_wave=1" FIL1 (1, 7000, 0.1) AMP (900, 800, 0.85, 1700) REVERB ("fxa3", 0.7, 0.7, 0.4, 0.12, 1, 0.38) },
    { "Pad Sombrio", "Pad", "pad,escuro,tenso", "nucleo", "", "",
      ANA (a, 2, 4, 20) ANA (b, 3, 2, 10) " b_coarse=-12 b_level=-6 blend=0.4" FIL1 (2, 1400, 0.2) MOD (1, 1, 11, 0.2) " lfo1_rate=0.1 lfo1_wave=1" AMP (1000, 800, 0.85, 2000)
      REVERB ("fxa3", 0.72, 0.75, 0.5, 0.12, 1, 0.4) },
    { "Pad Gelo", "Pad", "pad,gelo,frio", "nucleo", "", "",
      WTB (a, 5, 0.6, 3, 16) FIL1 (1, 8500, 0.1) MOD (1, 1, 8, 0.25) " lfo1_rate=0.22" AMP (800, 900, 0.8, 1800) DELAY ("fxa2", 0.66, 0.4, 0.5, 0.7, 0.5, 0.25) REVERB ("fxa3", 0.7, 0.72, 0.35, 0.12, 1, 0.4) },
    { "Pad Varrimento", "Pad", "pad,sweep,movimento", "nucleo", "", "",
      ANA (a, 2, 5, 22) FIL1 (2, 800, 0.35) MOD (1, 1, 11, 0.7) " lfo1_rate=0.12 lfo1_wave=1" AMP (1500, 800, 0.9, 2000) CHORUS ("fxa1", 0.2, 0.5, 0.5, 0.5, 1, 0.4) REVERB ("fxa3", 0.65, 0.65, 0.45, 0.1, 1, 0.3) },
    { "Pad Metais", "Pad", "pad,metais,brass", "nucleo", "", "",
      ANA (a, 2, 3, 10) FIL1E (2, 1200, 0.15, 0.5) ENV2 (250, 500, 0.6, 400) AMP (180, 500, 0.85, 350) REVERB ("fxa3", 0.55, 0.5, 0.5, 0.08, 0.9, 0.2) },
    { "Drone Ambiente", "Pad", "pad,drone,ambiente", "nucleo", "", "",
      WTB (a, 15, 0.5, 6, 30) ANA (b, 0, 1, 0) " b_coarse=-12 b_level=-6 blend=0.4" FIL1 (1, 3500, 0.15) MOD (1, 1, 7, 0.4) " lfo1_rate=0.05 lfo1_wave=6" AMP (2500, 1000, 0.95, 4000)
      REVERB ("fxa3", 0.8, 0.85, 0.5, 0.15, 1, 0.5) },
    { "Shimmer", "Pad", "pad,shimmer,etéreo", "nucleo", "", "",
      WTB (a, 4, 0.7, 4, 20) WTB (b, 5, 0.3, 3, 14) " b_coarse=12 b_level=-9 blend=0.4" FIL1 (1, 9500, 0.1) MOD (1, 1, 8, 0.3) " lfo1_rate=0.15" AMP (1000, 900, 0.85, 2200)
      CHORUS ("fxa1", 0.2, 0.5, 0.5, 0.6, 1, 0.4) REVERB ("fxa3", 0.8, 0.82, 0.3, 0.15, 1, 0.5) },
    { "Cordas Ensemble", "Pad", "pad,cordas,ensemble", "nucleo", "", "",
      ANA (a, 2, 7, 30) FIL1 (1, 3500, 0.1) AMP (350, 500, 0.9, 700) CHORUS ("fxa1", 0.28, 0.6, 0.5, 0.6, 1, 0.5) REVERB ("fxa3", 0.62, 0.6, 0.5, 0.1, 1, 0.28) },
    // ---- plucks
    { "Pizzicato", "Pluck", "pluck,pizzicato,cordas", "nucleo", "", "",
      ANA (a, 2, 1, 0) FIL1E (2, 1800, 0.1, 0.5) ENV2 (1, 120, 0.0, 80) AMP (1, 200, 0.0, 90) REVERB ("fxa3", 0.5, 0.45, 0.5, 0.05, 0.9, 0.2) },
    { "Harpa", "Pluck", "pluck,harpa,delicado", "nucleo", "", "",
      WTB (a, 14, 0.2, 1, 0) AMP (1, 1500, 0.0, 1100) CHORUS ("fxa1", 0.25, 0.3, 0.5, 0.5, 1, 0.3) REVERB ("fxa3", 0.65, 0.65, 0.4, 0.1, 1, 0.3) },
    { "Koto", "Pluck", "pluck,koto,oriental", "nucleo", "", "",
      ANA (a, 2, 1, 0) FIL1E (2, 3000, 0.3, 0.6) ENV2 (1, 350, 0.0, 200) AMP (1, 900, 0.0, 500) DELAY ("fxa2", 0.6, 0.3, 0.5, 0.5, 0.5, 0.2) REVERB ("fxa3", 0.55, 0.5, 0.5, 0.08, 1, 0.22) },
    { "Pluck Vidro", "Pluck", "pluck,vidro,brilhante", "nucleo", "", "",
      WTB (a, 4, 0.3, 2, 10) AMP (1, 500, 0.0, 350) DELAY ("fxa2", 0.66, 0.38, 0.5, 0.6, 0.5, 0.25) REVERB ("fxa3", 0.6, 0.6, 0.4, 0.1, 1, 0.25) },
    { "Marimba", "Pluck", "pluck,marimba,mallet", "nucleo", "", "",
      ANA (a, 0, 1, 0) " a_fmamt=0.2 a_fmrat=4" MOD (1, 3, 21, -0.2) ENV2 (1, 150, 0.0, 80) AMP (1, 600, 0.0, 300) REVERB ("fxa3", 0.5, 0.5, 0.5, 0.08, 0.9, 0.22) },
    { "Pluck Borracha", "Pluck", "pluck,borracha,elástico", "nucleo", "", "",
      ANA (a, 3, 2, 10) " a_pwm=0.3" MOD (1, 3, 19, 0.5) ENV2 (1, 200, 0.0, 100) FIL1E (2, 1500, 0.3, 0.5) AMP (1, 350, 0.0, 160) DELAY ("fxa2", 0.6, 0.3, 0.5, 0.5, 0.5, 0.2) },
    // ---- arpejos
    { "Arp Trance", "Arp", "arp,trance,energia", "nucleo", "", "",
      ANA (a, 2, 3, 12) FIL1E (2, 2200, 0.2, 0.5) ENV2 (1, 180, 0.0, 100) AMP (1, 240, 0.0, 120) " arp_on=1 arp_rate=12 arp_mode=2 arp_oct=2 arp_gate=0.5"
      DELAY ("fxa2", 0.6, 0.4, 0.5, 0.7, 0.5, 0.3) REVERB ("fxa3", 0.55, 0.5, 0.5, 0.08, 1, 0.22) },
    { "Arp Chill", "Arp", "arp,chill,suave", "nucleo", "", "",
      WTB (a, 13, 0.3, 1, 0) FIL1 (1, 6500, 0.05) AMP (1, 900, 0.1, 400) " arp_on=1 arp_rate=9 arp_mode=0 arp_oct=2 arp_gate=0.5" CHORUS ("fxa1", 0.25, 0.4, 0.5, 0.5, 1, 0.3) REVERB ("fxa3", 0.6, 0.6, 0.45, 0.1, 1, 0.3) },
    { "Arp Pulso", "Arp", "arp,pulso,aleatório", "nucleo", "", "",
      ANA (a, 3, 1, 0) " a_pwm=0.3" FIL1E (2, 2000, 0.25, 0.5) ENV2 (1, 150, 0.0, 90) AMP (1, 220, 0.0, 110) " arp_on=1 arp_rate=12 arp_mode=4 arp_oct=2 arp_gate=0.5" DELAY ("fxa2", 0.6, 0.35, 0.5, 0.6, 0.5, 0.25) },
    { "Arp Sinos", "Arp", "arp,sinos,cristal", "nucleo", "", "",
      WTB (a, 5, 0.4, 2, 8) AMP (1, 800, 0.0, 400) " arp_on=1 arp_rate=12 arp_mode=3 arp_oct=3 arp_gate=0.55" DELAY ("fxa2", 0.66, 0.4, 0.5, 0.7, 0.5, 0.3) REVERB ("fxa3", 0.65, 0.65, 0.4, 0.12, 1, 0.3) },
    { "Arp Baixo", "Arp", "arp,baixo,sequência", "nucleo", "", "",
      ANA (a, 2, 1, 0) MONO (10) FIL1E (7, 700, 0.5, 0.6) ENV2 (1, 200, 0.0, 90) AMP (1, 240, 0.4, 100) " arp_on=1 arp_rate=12 arp_mode=0 arp_oct=2 arp_gate=0.5" SATUR ("fxa1", 0.3, 0.5, 0, 0.5, 0.8, 0.4) },
    // ---- efeitos / texturas
    { "Laser", "FX", "efeito,laser,pitch", "nucleo", "", "",
      ANA (a, 2, 1, 0) MOD (1, 3, 3, 0.9) ENV2 (1, 400, 0.0, 100) FIL1 (1, 9000, 0.1) AMP (1, 400, 0.0, 150) DELAY ("fxa2", 0.6, 0.4, 0.5, 0.6, 0.5, 0.25) },
    { "Subida Ruído", "FX", "efeito,riser,ruído", "nucleo", "", "",
      " a_level=-60 noise_level=-4 noise_color=0.4" FIL1 (5, 400, 0.5) MOD (1, 3, 11, 0.9) ENV2 (3000, 1, 1, 400) AMP (1500, 100, 1, 600) REVERB ("fxa3", 0.65, 0.65, 0.45, 0.1, 1, 0.3) },
    { "Descida Ruído", "FX", "efeito,downlifter,ruído", "nucleo", "", "",
      " a_level=-60 noise_level=-4 noise_color=0.5" FIL1 (5, 6000, 0.4) MOD (1, 3, 11, -0.8) ENV2 (1, 2500, 0, 400) AMP (1, 2500, 0.0, 500) REVERB ("fxa3", 0.65, 0.65, 0.45, 0.1, 1, 0.3) },
    { "Alien", "FX", "efeito,alien,estranho", "nucleo", "", "",
      WTB (a, 11, 0.5, 3, 40) MOD (1, 2, 7, 0.8) " lfo2_rate=4 lfo2_wave=5" FIL1 (5, 2500, 0.4) AMP (20, 400, 0.8, 600) DELAY ("fxa2", 0.66, 0.5, 0.5, 0.7, 0.5, 0.3) REVERB ("fxa3", 0.65, 0.7, 0.4, 0.1, 1, 0.3) },
    { "Sirene Analógica", "FX", "efeito,sirene,alerta", "nucleo", "", "",
      ANA (a, 3, 1, 0) " a_pwm=0.5" MOD (1, 1, 3, 0.4) " lfo1_rate=1.2 lfo1_wave=1" FIL1 (1, 5000, 0.1) AMP (30, 300, 0.9, 300) DELAY ("fxa2", 0.6, 0.35, 0.5, 0.6, 0.5, 0.2) },
    // ---- cordas e metais sintéticos
    { "Metais Sintéticos", "Teclas", "metais,brass,sintético", "nucleo", "", "",
      ANA (a, 2, 4, 14) FIL1E (2, 1500, 0.15, 0.55) ENV2 (60, 350, 0.6, 250) AMP (35, 300, 0.85, 200) CHORUS ("fxa1", 0.25, 0.3, 0.5, 0.5, 0.9, 0.3) REVERB ("fxa3", 0.5, 0.45, 0.5, 0.08, 0.9, 0.18) },
    { "Brass Stab", "Pluck", "metais,stab,brass", "nucleo", "", "",
      ANA (a, 2, 4, 14) " chord_type=3" FIL1E (2, 2200, 0.15, 0.5) ENV2 (5, 220, 0.0, 120) AMP (4, 320, 0.0, 160) REVERB ("fxa3", 0.5, 0.45, 0.5, 0.08, 0.9, 0.18) },
    };
    return ROWS (R, "multi");
}

inline std::vector<Preset> trapMore()
{
    static const Row R[] = {
    { "808 Reverso", "Baixo", "808,reverso,efeito", "trap", "trap.808long", "", SMP (a) " a_rev=1 a_loop=0" MONO (30) AMP (1, 1500, 0.9, 400) SATUR ("fxa1", 0.2, 0.6, 0, 0.4, 0.8, 0.3) },
    { "808 Limpo", "Baixo", "808,limpo,sub", "trap", "trap.808curto", "", SMP (a) MONO (35) FIL1 (1, 700, 0.05) AMP (1, 800, 0.9, 200) },
    { "808 Gritty", "Baixo", "808,crush,sujo", "trap", "trap.808long", "", SMP (a) MONO (35) AMP (1, 800, 0.95, 260) SATUR ("fxa1", 0.55, 0.55, 0.05, 0.7, 0.75, 0.6) CRUSH ("fxa2", 0.55, 0.45, 0.1, 0.8, 0.7, 0.35) },
    { "808 Slide Longo", "Baixo", "808,slide,longo", "trap", "trap.808long", "", SMP (a) MONO (280) " glide_mode=0" AMP (1, 900, 0.95, 320) SATUR ("fxa1", 0.3, 0.6, 0, 0.5, 0.8, 0.4) },
    { "Sino Cristal", "Melodia", "sino,cristal,largo", "trap", "trap.bell", "", SMP (a) AMP (1, 2000, 0.5, 1300) CHORUS ("fxa1", 0.22, 0.4, 0.5, 0.5, 1, 0.35) DELAY ("fxa2", 0.66, 0.4, 0.5, 0.7, 0.5, 0.3) REVERB ("fxa3", 0.7, 0.75, 0.35, 0.12, 1, 0.38) },
    { "Sino Reverso", "FX", "sino,reverso,transição", "trap", "trap.bell", "", SMP (a) " a_rev=1 a_loop=0" AMP (1, 2500, 1, 300) REVERB ("fxa3", 0.7, 0.75, 0.4, 0.12, 1, 0.4) },
    { "Coro Dark Wide", "Pad", "pad,coro,largo", "trap", "trap.pad", "trap.vox", SMP (a) SMP (b) " blend=0.45 b_level=-4" AMP (700, 800, 0.9, 1700) WIDTH ("fxa1", 0.75, 0.3, 0.5, 0.5, 0.55, 1) REVERB ("fxa3", 0.72, 0.78, 0.5, 0.12, 1, 0.42) },
    { "Stab Menor", "Melodia", "stab,menor,acorde", "trap", "trap.stab", "", SMP (a) " chord_type=4" AMP (1, 420, 0.0, 220) REVERB ("fxa3", 0.55, 0.5, 0.45, 0.06, 0.9, 0.22) },
    { "Pluck Delay Trap", "Melodia", "pluck,delay,ambiente", "trap", "trap.pluck", "", SMP (a) AMP (1, 500, 0.0, 300) DELAY ("fxa2", 0.7, 0.55, 0.45, 0.8, 0.5, 0.45) REVERB ("fxa3", 0.6, 0.6, 0.45, 0.1, 1, 0.3) },
    { "Hats Triplet", "Bateria", "hats,triplet,arp", "trap", "trap.hat", "", SMP (a) AMP (1, 600, 1, 60) " arp_on=1 arp_rate=13 arp_mode=0 arp_pattern=1 arp_gate=0.55" },
    { "Kit Trap Duro", "Bateria", "bateria,kit,pesado", "trap", "trap.kit", "", SMP (a) AMP (1, 800, 1, 120) " a_loop=0 a_rr=1" SATUR ("fxa1", 0.35, 0.55, 0.05, 0.5, 0.8, 0.45) COMP ("fxa2", 0.4, 0.55, 0.25, 0.3, 0.35, 1) },
    };
    return ROWS (R, "trap");
}

inline std::vector<Preset> rnbMore()
{
    static const Row R[] = {
    { "Rhodes Escuro", "Teclas", "rhodes,escuro,noite", "rnb", "rnb.rhodes", "", SMP (a) FIL1 (1, 2600, 0.1) AMP (1, 1500, 0.55, 520) CHORUS ("fxa1", 0.25, 0.45, 0.5, 0.5, 1, 0.4) REVERB ("fxa3", 0.62, 0.62, 0.4, 0.1, 1, 0.28) },
    { "Rhodes Trémolo", "Teclas", "rhodes,trémolo,vintage", "rnb", "rnb.rhodes", "", SMP (a) AMP (1, 1400, 0.55, 480) TREM ("fxa1", 0.3, 0.5, 0.0, 0.6, 0.5, 0.7) REVERB ("fxa3", 0.55, 0.5, 0.5, 0.08, 0.9, 0.2) },
    { "Wurli Faser", "Teclas", "wurlitzer,faser,psicadélico", "rnb", "rnb.wurli", "", SMP (a) AMP (1, 1100, 0.5, 420) PHASER ("fxa1", 0.25, 0.6, 0.35, 0.5, 0.6, 0.5) REVERB ("fxa3", 0.55, 0.5, 0.5, 0.08, 0.9, 0.2) },
    { "Cordas Largas", "Pad", "cordas,largo,cinema", "rnb", "rnb.strings", "", SMP (a) AMP (500, 700, 0.9, 1200) WIDTH ("fxa1", 0.75, 0.3, 0.5, 0.5, 0.55, 1) REVERB ("fxa3", 0.68, 0.7, 0.5, 0.12, 1, 0.35) },
    { "Baixo Veludo", "Baixo", "baixo,veludo,suave", "rnb", "rnb.bass", "", SMP (a) MONO (30) FIL1 (1, 900, 0.05) AMP (3, 600, 0.9, 220) COMP ("fxa1", 0.4, 0.45, 0.3, 0.4, 0.35, 1) },
    { "Guitarra Wah", "Teclas", "guitarra,wah,funk", "rnb", "rnb.guitar", "", SMP (a) FIL1E (5, 900, 0.4, 0.6) ENV2 (1, 300, 0.2, 200) AMP (1, 800, 0.4, 300) CHORUS ("fxa1", 0.3, 0.35, 0.5, 0.5, 0.9, 0.3) },
    { "Coro Cordas", "Vozes", "coro,cordas,camada", "rnb", "rnb.voxoo", "rnb.strings", SMP (a) SMP (b) " blend=0.5 b_level=-3" AMP (400, 600, 0.9, 1000) REVERB ("fxa3", 0.68, 0.7, 0.45, 0.12, 1, 0.35) },
    { "Rhodes Min9", "Teclas", "rhodes,acordes,min9", "rnb", "rnb.rhodes", "", SMP (a) " chord_type=9" AMP (1, 1500, 0.55, 520) CHORUS ("fxa1", 0.25, 0.45, 0.5, 0.5, 1, 0.4) REVERB ("fxa3", 0.6, 0.6, 0.45, 0.1, 1, 0.25) },
    { "Sino Etéreo", "Melodia", "sino,etéreo,ambiente", "rnb", "rnb.bell", "", SMP (a) AMP (1, 2000, 0.5, 1400) DELAY ("fxa2", 0.7, 0.5, 0.45, 0.8, 0.5, 0.35) REVERB ("fxa3", 0.75, 0.8, 0.35, 0.14, 1, 0.45) },
    { "Kit Lo-Fi", "Bateria", "bateria,kit,lofi", "rnb", "rnb.kit", "", SMP (a) AMP (1, 800, 1, 120) " a_loop=0 a_rr=1" FIL1 (1, 7000, 0.05) CRUSH ("fxa1", 0.6, 0.5, 0.1, 0.8, 0.8, 0.4) },
    { "Arp Rhodes Chill", "Arp", "arp,rhodes,chill", "rnb", "rnb.rhodes", "", SMP (a) AMP (1, 800, 0.2, 350) " arp_on=1 arp_rate=11 arp_mode=2 arp_oct=2 arp_gate=0.55 arp_pattern=2" CHORUS ("fxa1", 0.25, 0.4, 0.5, 0.5, 1, 0.3) REVERB ("fxa3", 0.6, 0.6, 0.45, 0.1, 1, 0.28) },
    };
    return ROWS (R, "rnb");
}

inline std::vector<Preset> afrobeatMore()
{
    static const Row R[] = {
    { "Kalimba Arp", "Arp", "kalimba,arp,melodia", "afrobeat", "afrobeat.kalimba", "", SMP (a) AMP (1, 900, 0.0, 450) " arp_on=1 arp_rate=12 arp_mode=0 arp_oct=2 arp_gate=0.5 arp_pattern=3" DELAY ("fxa2", 0.6, 0.35, 0.5, 0.6, 0.5, 0.25) REVERB ("fxa3", 0.55, 0.55, 0.45, 0.1, 1, 0.25) },
    { "Guitarra Afro Delay", "Teclas", "guitarra,delay,highlife", "afrobeat", "afrobeat.guitar", "", SMP (a) AMP (1, 750, 0.3, 280) DELAY ("fxa2", 0.66, 0.5, 0.5, 0.7, 0.5, 0.35) CHORUS ("fxa1", 0.3, 0.3, 0.5, 0.5, 0.9, 0.3) },
    { "Metais Acorde", "Melodia", "metais,acorde,fanfarra", "afrobeat", "afrobeat.horn", "", SMP (a) " chord_type=3" AMP (4, 420, 0.0, 220) REVERB ("fxa3", 0.55, 0.5, 0.5, 0.08, 0.9, 0.22) },
    { "Conga Padrão", "Percussão", "conga,padrão,groove", "afrobeat", "afrobeat.congalo", "", SMP (a) AMP (1, 700, 1, 90) " arp_on=1 arp_rate=12 arp_mode=0 arp_pattern=3 arp_gate=0.5" },
    { "Bongo Padrão", "Percussão", "bongo,padrão,groove", "afrobeat", "afrobeat.bongo", "", SMP (a) AMP (1, 600, 1, 80) " arp_on=1 arp_rate=12 arp_mode=0 arp_pattern=3 arp_swing=0.15 arp_gate=0.5" },
    { "Kit Afro Pesado", "Bateria", "bateria,kit,pesado", "afrobeat", "afrobeat.kit", "", SMP (a) AMP (1, 800, 1, 120) " a_loop=0 a_rr=1" SATUR ("fxa1", 0.3, 0.55, 0.05, 0.5, 0.8, 0.4) COMP ("fxa2", 0.4, 0.55, 0.25, 0.3, 0.35, 1) },
    { "Baixo Afro Slap", "Baixo", "baixo,slap,groove", "afrobeat", "afrobeat.bass", "", SMP (a) MONO (15) AMP (1, 500, 0.8, 140) COMP ("fxa1", 0.4, 0.5, 0.2, 0.3, 0.4, 1) },
    { "Pad Savana", "Pad", "pad,savana,amplo", "afrobeat", "afrobeat.pad", "", SMP (a) AMP (600, 800, 0.9, 1500) WIDTH ("fxa1", 0.7, 0.3, 0.5, 0.5, 0.55, 1) REVERB ("fxa3", 0.68, 0.7, 0.45, 0.12, 1, 0.35) },
    { "Coro Tribal", "Vozes", "coro,tribal,oitava", "afrobeat", "afrobeat.vox", "", SMP (a) " chord_type=1" AMP (60, 600, 0.9, 600) REVERB ("fxa3", 0.62, 0.62, 0.45, 0.1, 1, 0.3) },
    { "Apito Eco", "FX", "apito,eco,efeito", "afrobeat", "afrobeat.whistle", "", SMP (a) AMP (1, 800, 1, 100) DELAY ("fxa2", 0.66, 0.55, 0.5, 0.7, 0.5, 0.4) REVERB ("fxa3", 0.6, 0.6, 0.45, 0.1, 1, 0.3) },
    { "Talking Drum Glide", "Percussão", "tambor,talking,glide", "afrobeat", "afrobeat.talking", "", SMP (a) LEGATO (150) AMP (1, 700, 0.8, 200) REVERB ("fxa3", 0.5, 0.45, 0.5, 0.06, 0.9, 0.18) },
    };
    return ROWS (R, "afrobeat");
}

inline std::vector<Preset> amapianoMore()
{
    static const Row R[] = {
    { "Log Drum Gordo", "Baixo", "logdrum,gordo,saturado", "amapiano", "amapiano.logdrum", "", SMP (a) MONO (30) AMP (1, 800, 0.9, 260) SATUR ("fxa1", 0.45, 0.55, 0.05, 0.6, 0.75, 0.55) COMP ("fxa2", 0.4, 0.5, 0.3, 0.4, 0.35, 1) },
    { "Log Drum Melódico", "Baixo", "logdrum,melódico,arp", "amapiano", "amapiano.logshort", "", SMP (a) AMP (1, 500, 0.0, 200) " arp_on=1 arp_rate=12 arp_mode=0 arp_pattern=4 arp_gate=0.5" REVERB ("fxa3", 0.5, 0.45, 0.5, 0.06, 0.9, 0.18) },
    { "Piano Soul Min9", "Teclas", "piano,soul,min9", "amapiano", "amapiano.piano", "", SMP (a) " chord_type=9" AMP (1, 1500, 0.35, 450) CHORUS ("fxa1", 0.25, 0.35, 0.5, 0.5, 1, 0.3) REVERB ("fxa3", 0.58, 0.55, 0.5, 0.08, 1, 0.22) },
    { "Piano Gospel", "Teclas", "piano,gospel,maj9", "amapiano", "amapiano.piano", "", SMP (a) " chord_type=8" AMP (1, 1700, 0.4, 500) REVERB ("fxa3", 0.6, 0.6, 0.45, 0.1, 1, 0.25) },
    { "Teclas Lo-Fi", "Teclas", "teclas,lofi,jazzy", "amapiano", "amapiano.keys", "", SMP (a) FIL1 (1, 5500, 0.05) AMP (1, 1100, 0.5, 400) CRUSH ("fxa1", 0.65, 0.5, 0.1, 0.8, 0.8, 0.35) CHORUS ("fxa2", 0.3, 0.4, 0.5, 0.5, 1, 0.3) },
    { "Chop Ah Delay", "Vozes", "voz,chop,delay", "amapiano", "amapiano.chopah", "", SMP (a) AMP (1, 700, 0.8, 300) DELAY ("fxa2", 0.7, 0.55, 0.45, 0.8, 0.5, 0.45) REVERB ("fxa3", 0.6, 0.6, 0.45, 0.1, 1, 0.3) },
    { "Chop Oh Arp", "Vozes", "voz,chop,arp", "amapiano", "amapiano.chopoh", "", SMP (a) AMP (1, 500, 0.0, 200) " arp_on=1 arp_rate=12 arp_mode=2 arp_pattern=4 arp_gate=0.45" DELAY ("fxa2", 0.6, 0.4, 0.5, 0.6, 0.5, 0.3) },
    { "Piano + Pad", "Pad", "piano,pad,camada", "amapiano", "amapiano.piano", "amapiano.pad", SMP (a) SMP (b) " blend=0.4 b_level=-3" AMP (200, 900, 0.8, 1200) REVERB ("fxa3", 0.65, 0.65, 0.45, 0.1, 1, 0.3) },
    { "Kit Amapiano Groove", "Bateria", "bateria,kit,groove", "amapiano", "amapiano.kit", "", SMP (a) AMP (1, 800, 1, 120) " a_loop=0 a_rr=1" SATUR ("fxa1", 0.25, 0.55, 0.05, 0.4, 0.8, 0.35) },
    { "Shaker Padrão", "Percussão", "shaker,padrão,groove", "amapiano", "amapiano.shakerl", "", SMP (a) AMP (1, 600, 1, 80) " arp_on=1 arp_rate=12 arp_mode=0 arp_pattern=4 arp_swing=0.2 arp_gate=0.5" },
    { "Baixo Profundo", "Baixo", "baixo,profundo,sub", "amapiano", "amapiano.bass", "", SMP (a) MONO (30) FIL1 (1, 800, 0.05) AMP (3, 600, 0.9, 240) },
    };
    return ROWS (R, "amapiano");
}

inline std::vector<Preset> drillMore()
{
    static const Row R[] = {
    { "808 Slide Grave", "Baixo", "808,slide,grave", "drill", "drill.808slide", "", SMP (a) MONO (200) " glide_mode=0 a_coarse=-5" AMP (1, 900, 0.95, 300) SATUR ("fxa1", 0.3, 0.6, 0, 0.5, 0.8, 0.4) },
    { "808 Slide Agudo", "Baixo", "808,slide,agudo", "drill", "drill.808slide", "", SMP (a) MONO (160) " glide_mode=0 a_coarse=5" AMP (1, 800, 0.95, 260) SATUR ("fxa1", 0.3, 0.6, 0, 0.5, 0.8, 0.4) },
    { "Sino Gelo", "Melodia", "sino,gelo,frio", "drill", "drill.bell", "", SMP (a) AMP (1, 2000, 0.5, 1200) CHORUS ("fxa1", 0.2, 0.35, 0.5, 0.5, 1, 0.3) DELAY ("fxa2", 0.66, 0.4, 0.45, 0.7, 0.5, 0.3) REVERB ("fxa3", 0.7, 0.75, 0.35, 0.12, 1, 0.38) },
    { "Sino Reverso Drill", "FX", "sino,reverso,transição", "drill", "drill.bell", "", SMP (a) " a_rev=1 a_loop=0" AMP (1, 2500, 1, 300) REVERB ("fxa3", 0.7, 0.75, 0.4, 0.12, 1, 0.4) },
    { "Pad Sombrio Largo", "Pad", "pad,sombrio,largo", "drill", "drill.pad", "", SMP (a) AMP (800, 800, 0.9, 1800) WIDTH ("fxa1", 0.75, 0.3, 0.5, 0.5, 0.55, 1) REVERB ("fxa3", 0.72, 0.78, 0.5, 0.12, 1, 0.42) },
    { "Pluck Drill Arp", "Arp", "pluck,arp,drill", "drill", "drill.pluck", "", SMP (a) AMP (1, 450, 0.0, 250) " arp_on=1 arp_rate=12 arp_mode=0 arp_pattern=5 arp_gate=0.5" DELAY ("fxa2", 0.66, 0.4, 0.45, 0.7, 0.5, 0.28) REVERB ("fxa3", 0.55, 0.5, 0.45, 0.08, 1, 0.22) },
    { "Voz Grave", "Vozes", "voz,grave,sombrio", "drill", "drill.vox", "", SMP (a) " a_coarse=-5" AMP (60, 600, 0.85, 800) DELAY ("fxa2", 0.66, 0.4, 0.45, 0.7, 0.5, 0.28) REVERB ("fxa3", 0.68, 0.72, 0.4, 0.12, 1, 0.35) },
    { "Kit Drill Duro", "Bateria", "bateria,kit,duro", "drill", "drill.kit", "", SMP (a) AMP (1, 800, 1, 120) " a_loop=0 a_rr=1" SATUR ("fxa1", 0.35, 0.55, 0.05, 0.5, 0.8, 0.45) COMP ("fxa2", 0.4, 0.55, 0.25, 0.3, 0.35, 1) },
    { "Hats Drill Padrão", "Bateria", "hats,padrão,slide", "drill", "drill.hat", "", SMP (a) AMP (1, 600, 1, 60) " arp_on=1 arp_rate=14 arp_mode=0 arp_pattern=5 arp_gate=0.55" },
    { "Tom Drill Grave", "Bateria", "tom,grave,groove", "drill", "drill.tom", "", SMP (a) AMP (1, 700, 1, 100) " arp_on=1 arp_rate=12 arp_mode=0 arp_pattern=5 arp_gate=0.5" REVERB ("fxa3", 0.5, 0.45, 0.5, 0.06, 0.9, 0.2) },
    { "Impacto Reverso", "FX", "impacto,reverso,transição", "drill", "drill.impact", "", SMP (a) " a_rev=1 a_loop=0" AMP (1, 3000, 1, 300) },
    };
    return ROWS (R, "drill");
}

inline std::vector<Preset> kizombaMore()
{
    static const Row R[] = {
    { "Baixo Zouk Slap", "Baixo", "baixo,slap,zouk", "kizomba", "kizomba.bass", "", SMP (a) MONO (15) AMP (1, 600, 0.85, 160) COMP ("fxa1", 0.4, 0.5, 0.2, 0.3, 0.4, 1) },
    { "EP Romântico", "Teclas", "ep,romântico,suave", "kizomba", "kizomba.ep", "", SMP (a) AMP (1, 1500, 0.55, 520) CHORUS ("fxa1", 0.25, 0.5, 0.5, 0.6, 1, 0.5) REVERB ("fxa3", 0.62, 0.62, 0.45, 0.1, 1, 0.3) },
    { "Guitarra Dedilhada", "Arp", "guitarra,dedilhada,arp", "kizomba", "kizomba.guitar", "", SMP (a) AMP (1, 800, 0.3, 300) " arp_on=1 arp_rate=11 arp_mode=2 arp_pattern=6 arp_oct=2 arp_gate=0.55" CHORUS ("fxa1", 0.28, 0.35, 0.5, 0.5, 0.9, 0.3) REVERB ("fxa3", 0.55, 0.5, 0.5, 0.08, 1, 0.22) },
    { "Cordas Românticas", "Pad", "cordas,romântico,largo", "kizomba", "kizomba.strings", "", SMP (a) AMP (450, 700, 0.9, 1100) WIDTH ("fxa1", 0.75, 0.3, 0.5, 0.5, 0.55, 1) REVERB ("fxa3", 0.68, 0.7, 0.5, 0.12, 1, 0.35) },
    { "Voz Zouk Dueto", "Vozes", "voz,zouk,dueto", "kizomba", "kizomba.vox", "kizomba.vox", SMP (a) SMP (b) " b_coarse=7 b_level=-6 blend=0.4" AMP (120, 600, 0.9, 700) CHORUS ("fxa1", 0.2, 0.3, 0.5, 0.5, 1, 0.3) REVERB ("fxa3", 0.65, 0.68, 0.45, 0.12, 1, 0.32) },
    { "Sino Delicado", "Melodia", "sino,delicado,romântico", "kizomba", "kizomba.bell", "", SMP (a) AMP (1, 1800, 0.5, 1100) DELAY ("fxa2", 0.66, 0.4, 0.5, 0.7, 0.5, 0.28) REVERB ("fxa3", 0.68, 0.7, 0.4, 0.12, 1, 0.35) },
    { "Kit Kizomba Suave", "Bateria", "bateria,kit,suave", "kizomba", "kizomba.kit", "", SMP (a) AMP (1, 800, 1, 120) " a_loop=0 a_rr=1" FIL1 (1, 9000, 0.05) COMP ("fxa1", 0.45, 0.5, 0.3, 0.4, 0.3, 1) },
    { "Conga Padrão Kizomba", "Percussão", "conga,padrão,groove", "kizomba", "kizomba.conga", "", SMP (a) AMP (1, 700, 1, 90) " arp_on=1 arp_rate=12 arp_mode=0 arp_pattern=6 arp_gate=0.5" },
    { "Guiro Padrão", "Percussão", "guiro,padrão,ritmo", "kizomba", "kizomba.guiro", "", SMP (a) AMP (1, 600, 1, 80) " arp_on=1 arp_rate=12 arp_mode=0 arp_pattern=6 arp_swing=0.12 arp_gate=0.5" },
    { "EP Min9", "Teclas", "ep,acordes,min9", "kizomba", "kizomba.ep", "", SMP (a) " chord_type=9" AMP (1, 1500, 0.55, 520) CHORUS ("fxa1", 0.25, 0.5, 0.5, 0.6, 1, 0.45) REVERB ("fxa3", 0.62, 0.62, 0.45, 0.1, 1, 0.28) },
    { "Violinos Pad", "Pad", "violinos,pad,suave", "kizomba", "kizomba.strings", "", SMP (a) FIL1 (1, 4500, 0.05) " a_coarse=12" AMP (500, 700, 0.9, 1200) CHORUS ("fxa1", 0.25, 0.5, 0.5, 0.6, 1, 0.4) REVERB ("fxa3", 0.65, 0.65, 0.45, 0.1, 1, 0.3) },
    };
    return ROWS (R, "kizomba");
}

inline std::vector<Preset> kuduroMore()
{
    static const Row R[] = {
    { "Stab Agressivo", "Melodia", "stab,agressivo,acorde", "kuduro", "kuduro.stab", "", SMP (a) " chord_type=3" AMP (1, 380, 0.0, 180) SATUR ("fxa1", 0.55, 0.6, 0.05, 0.7, 0.75, 0.6) },
    { "Stab Oitava", "Melodia", "stab,oitava,duro", "kuduro", "kuduro.stab", "", SMP (a) " chord_type=1" AMP (1, 350, 0.0, 160) SATUR ("fxa1", 0.45, 0.6, 0.05, 0.6, 0.75, 0.5) DELAY ("fxa2", 0.6, 0.3, 0.5, 0.5, 0.5, 0.2) },
    { "Baixo Sujo", "Baixo", "baixo,sujo,pesado", "kuduro", "kuduro.bass", "", SMP (a) MONO (15) AMP (1, 500, 0.9, 130) SATUR ("fxa1", 0.7, 0.55, 0.1, 0.8, 0.7, 0.75) COMP ("fxa2", 0.35, 0.5, 0.25, 0.3, 0.3, 1) },
    { "Ácido Rápido", "Arp", "ácido,arp,rápido", "kuduro", "kuduro.acid", "", SMP (a) AMP (1, 300, 0.0, 140) " arp_on=1 arp_rate=14 arp_mode=0 arp_pattern=7 arp_gate=0.5" SATUR ("fxa1", 0.5, 0.6, 0.05, 0.6, 0.75, 0.5) },
    { "Sirene Pitch", "FX", "sirene,pitch,efeito", "kuduro", "kuduro.siren", "", SMP (a) LEGATO (200) AMP (20, 800, 0.9, 300) REVERB ("fxa3", 0.55, 0.5, 0.5, 0.08, 0.9, 0.2) },
    { "Sirene Lenta", "FX", "sirene,lenta,tensão", "kuduro", "kuduro.siren", "", SMP (a) " a_coarse=-7" AMP (200, 800, 0.9, 600) DELAY ("fxa2", 0.66, 0.5, 0.5, 0.7, 0.5, 0.3) REVERB ("fxa3", 0.65, 0.65, 0.45, 0.1, 1, 0.3) },
    { "Grito Delay", "Vozes", "voz,grito,delay", "kuduro", "kuduro.shout", "", SMP (a) AMP (1, 500, 0.5, 200) DELAY ("fxa2", 0.7, 0.55, 0.5, 0.8, 0.5, 0.4) REVERB ("fxa3", 0.6, 0.6, 0.45, 0.1, 1, 0.3) },
    { "Kit Kuduro Pesado", "Bateria", "bateria,kit,pesado", "kuduro", "kuduro.kit", "", SMP (a) AMP (1, 800, 1, 120) " a_loop=0 a_rr=1" SATUR ("fxa1", 0.45, 0.55, 0.05, 0.6, 0.8, 0.55) COMP ("fxa2", 0.4, 0.55, 0.25, 0.3, 0.4, 1) },
    { "Impacto Reverso Kuduro", "FX", "impacto,reverso,transição", "kuduro", "kuduro.impact", "", SMP (a) " a_rev=1 a_loop=0" AMP (1, 3000, 1, 300) },
    { "Arp Kuduro Rápido", "Arp", "arp,stab,rápido", "kuduro", "kuduro.stab", "", SMP (a) AMP (1, 260, 0.0, 130) " arp_on=1 arp_rate=14 arp_mode=2 arp_pattern=7 arp_oct=2 arp_gate=0.5" SATUR ("fxa1", 0.5, 0.6, 0.05, 0.6, 0.75, 0.5) },
    { "Tom Kuduro Padrão", "Bateria", "tom,padrão,percussão", "kuduro", "kuduro.tom", "", SMP (a) AMP (1, 700, 1, 100) " arp_on=1 arp_rate=14 arp_mode=0 arp_pattern=7 arp_gate=0.5" REVERB ("fxa3", 0.5, 0.45, 0.5, 0.06, 0.9, 0.2) },
    };
    return ROWS (R, "kuduro");
}
