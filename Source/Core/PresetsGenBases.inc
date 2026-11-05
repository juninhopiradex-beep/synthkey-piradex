// =============================================================================
//  SYNTHKEY by Piradex — v1.6: bases novas para as famílias com falta de material (Synths, Percussão,
//  Baixos, Leads, Pads, Teclas, Bateria, Arps, Vozes, SFX, Texturas) e para os packs House, EDM, Pop e
//  Cinematic. Cada base é UM preset; as variações vêm de GenAxes.h (curadas offline em GenCurated.h).
//  Incluído dentro de PresetBuilder.h, antes dos #undef das macros. Estado: "auto" (nunca ouvido).
// =============================================================================
#define FT(t, r) " f1_type=" #t " f1_res=" #r
#define GRN(E, size, dens, pos, rnd, pj, spd, spr) " " #E "_on=1 " #E "_type=4 " #E "_gr_size=" #size " " #E "_gr_dens=" #dens " " #E "_gr_pos=" #pos " " #E "_gr_rand=" #rnd " " #E "_gr_pjit=" #pj " " #E "_gr_speed=" #spd " " #E "_gr_spread=" #spr
#define SPEC(E, tilt, form, fq, comb, odd, sweep, smear) " " #E "_on=1 " #E "_type=5 " #E "_sp_tilt=" #tilt " " #E "_sp_form=" #form " " #E "_sp_fq=" #fq " " #E "_sp_comb=" #comb " " #E "_sp_odd=" #odd " " #E "_sp_sweep=" #sweep " " #E "_sp_smear=" #smear
#define KSTR(E, damp, decay, pick) " " #E "_on=1 " #E "_type=3 " #E "_ks_damp=" #damp " " #E "_ks_decay=" #decay " " #E "_ks_pick=" #pick
#define ARPON(rate, mode, pat, gate, oct) " arp_on=1 arp_rate=" #rate " arp_mode=" #mode " arp_pattern=" #pat " arp_gate=" #gate " arp_oct=" #oct

inline void buildFamRecipes (const FamRecipe* R, size_t n, const char* pack, const char* style, std::vector<Preset>& out)
{
    for (size_t i = 0; i < n; ++i)
    {
        const auto& r = R[i];
        char cutTxt[48]; std::snprintf (cutTxt, sizeof cutTxt, " f1_cut=%.1f", (double) std::min (20000.0f, std::max (60.0f, r.cut)));
        const std::string kv = std::string (r.kv) + cutTxt;
        const Row row { r.name, r.cat, r.tags, pack, r.smpA, r.smpB, kv.c_str() };
        Preset p = make (row, style);
        p.family = r.family; p.subfamily = r.sub; p.role = r.role; p.qa = "auto"; p.genre = style;
        { const int ta = (int) p.get (P_A_TYPE), tb = (int) p.get (P_B_TYPE); const bool bOn = p.get (P_B_ON) > 0.5f;
          p.engine = ta == 3 ? "physical" : ta == 4 ? "granular" : ta == 5 ? "spectral" : (bOn && tb != ta) ? "hybrid" : ta == 2 ? "sample" : ta == 1 ? "wavetable" : "analog"; }
        out.push_back (std::move (p));
    }
}

// ============================================================================ Núcleo: famílias em falta ============================================================================
inline std::vector<Preset> genBasesNucleo()
{
    static const FamRecipe R[] = {
    // ---------------- SYNTHS (120) ----------------
    { "Synths", "Sintetizador", "Supersaw", "Supersaw Clássico", "synth,supersaw,largo", "pad", "", "", 7000, ANA (a, 2, 7, 28) ANA (b, 2, 5, 18) " b_coarse=12 b_level=-9 blend=0.5" FT (1, 0.1) AMP (6, 600, 0.8, 380) CHORUS ("fxa1", 0.3, 0.4, 0.5, 0.5, 1, 0.3) REVERB ("fxa3", 0.6, 0.55, 0.5, 0.1, 1, 0.22) },
    { "Synths", "Sintetizador", "Poly Synth", "Poly Brilhante", "synth,poly,brilhante", "pad", "", "", 6000, ANA (a, 2, 3, 14) ANA (b, 3, 2, 8) " a_pwm=0.4 b_pwm=0.45 b_level=-5 blend=0.5" FT (2, 0.15) " f1_env=0.35" ENV2 (4, 600, 0.35, 300) AMP (4, 500, 0.8, 280) CHORUS ("fxa1", 0.25, 0.4, 0.5, 0.5, 1, 0.35) },
    { "Synths", "Sintetizador", "Vintage Synth", "Poly Vintage", "synth,vintage,analógico", "pad", "", "", 3800, ANA (a, 2, 2, 12) ANA (b, 2, 2, 9) " b_coarse=-12 b_level=-6 blend=0.5" FT (2, 0.2) " f1_env=0.3" ENV2 (20, 700, 0.3, 350) AMP (12, 500, 0.8, 400) CHORUS ("fxa1", 0.2, 0.45, 0.5, 0.5, 1, 0.4) REVERB ("fxa3", 0.55, 0.5, 0.5, 0.1, 1, 0.2) },
    { "Synths", "Sintetizador", "Vintage Synth", "Brass Vintage Sintético", "synth,vintage,metais", "stab", "", "", 3200, ANA (a, 2, 4, 16) FT (2, 0.15) " f1_env=0.55" ENV2 (70, 600, 0.45, 250) AMP (30, 500, 0.8, 260) CHORUS ("fxa1", 0.3, 0.4, 0.5, 0.5, 1, 0.3) },
    { "Synths", "Sintetizador", "Digital Synth", "Digital Cristal", "synth,digital,cristal", "lead", "", "", 9000, WTB (a, 6, 0.4, 3, 12) FT (1, 0.05) MOD (1, 3, 8, 0.3) ENV2 (1, 700, 0.2, 300) AMP (3, 600, 0.7, 350) DELAY ("fxa2", 0.6, 0.3, 0.5, 0.6, 0.5, 0.2) REVERB ("fxa3", 0.6, 0.55, 0.45, 0.1, 1, 0.22) },
    { "Synths", "Sintetizador", "Digital Synth", "Digital Metálico", "synth,digital,metálico", "stab", "", "", 7500, WTB (a, 11, 0.5, 2, 10) FT (2, 0.12) " f1_env=0.45" ENV2 (1, 420, 0.0, 200) AMP (2, 420, 0.0, 220) SATUR ("fxa1", 0.25, 0.65, 0.05, 0.5, 0.8, 0.35) REVERB ("fxa3", 0.5, 0.45, 0.5, 0.06, 0.9, 0.18) },
    { "Synths", "Sintetizador", "Hybrid Synth", "Híbrido Serra e Tabela", "synth,híbrido,largo", "pad", "", "", 6500, ANA (a, 2, 4, 16) WTB (b, 10, 0.4, 3, 14) " b_level=-5 blend=0.5" FT (2, 0.1) AMP (60, 700, 0.85, 500) CHORUS ("fxa1", 0.25, 0.4, 0.5, 0.5, 1, 0.3) REVERB ("fxa3", 0.65, 0.6, 0.45, 0.1, 1, 0.28) },
    { "Synths", "Sintetizador", "Hybrid Synth", "Híbrido Espectral Largo", "synth,híbrido,espectral", "pad", "", "", 7000, ANA (a, 2, 3, 12) SPEC (b, 1.0, 10, 0.4, 0.15, 0.0, 0.5, 0.2) " b_level=-6 blend=0.5" FT (2, 0.1) AMP (150, 900, 0.85, 700) CHORUS ("fxa1", 0.2, 0.4, 0.5, 0.5, 1, 0.3) REVERB ("fxa3", 0.7, 0.65, 0.45, 0.1, 1, 0.3) },
    { "Synths", "Sintetizador", "Poly Synth", "Acorde Poly Curto", "synth,poly,curto", "stab", "", "", 5500, ANA (a, 2, 5, 18) FT (2, 0.12) " f1_env=0.55" ENV2 (3, 280, 0.0, 150) AMP (3, 320, 0.0, 180) CHORUS ("fxa1", 0.3, 0.4, 0.5, 0.5, 1, 0.3) DELAY ("fxa2", 0.6, 0.3, 0.5, 0.6, 0.5, 0.2) },
    { "Synths", "Sintetizador", "Experimental Synth", "Quadrada Ressonante", "synth,experimental,ressonante", "lead", "", "", 2200, ANA (a, 3, 2, 6) " a_pwm=0.5" FT (7, 0.65) " f1_env=0.5" MOD (1, 1, 19, 0.3) " lfo1_rate=0.6" ENV2 (4, 500, 0.2, 250) AMP (4, 400, 0.8, 250) DELAY ("fxa2", 0.6, 0.35, 0.5, 0.6, 0.5, 0.22) },
    { "Synths", "Sintetizador", "Experimental Synth", "FM Metálico", "synth,experimental,fm", "lead", "", "", 8000, ANA (a, 0, 1, 0) " a_fmamt=0.5 a_fmrat=5" MOD (1, 3, 21, -0.3) ENV2 (1, 600, 0.0, 300) FT (1, 0.05) AMP (2, 700, 0.5, 400) REVERB ("fxa3", 0.6, 0.55, 0.45, 0.1, 1, 0.25) },
    { "Synths", "Sintetizador", "Supersaw", "Supersaw Acorde Aberto", "synth,supersaw,acorde", "stab", "", "", 9000, ANA (a, 2, 7, 32) FT (2, 0.08) " f1_env=0.5" ENV2 (2, 380, 0.1, 200) AMP (2, 420, 0.1, 260) CHORUS ("fxa1", 0.3, 0.4, 0.5, 0.5, 1, 0.25) REVERB ("fxa3", 0.65, 0.6, 0.45, 0.1, 1, 0.3) },
    { "Synths", "Sintetizador", "Supersaw", "Supersaw Pad Lento", "synth,supersaw,pad", "pad", "", "", 4500, ANA (a, 2, 7, 26) ANA (b, 2, 5, 20) " b_coarse=-12 b_level=-6 blend=0.5" FT (2, 0.08) AMP (500, 900, 0.9, 900) CHORUS ("fxa1", 0.2, 0.45, 0.5, 0.5, 1, 0.35) REVERB ("fxa3", 0.8, 0.75, 0.45, 0.12, 1, 0.35) },
    { "Synths", "Sintetizador", "Poly Synth", "Arpa Pulsada", "synth,poly,pulsada", "pluck", "", "", 4800, ANA (a, 3, 3, 12) " a_pwm=0.35" FT (2, 0.2) " f1_env=0.6" ENV2 (1, 260, 0.0, 120) AMP (1, 300, 0.0, 160) DELAY ("fxa2", 0.6, 0.4, 0.5, 0.7, 0.5, 0.3) },
    { "Synths", "Sintetizador", "Digital Synth", "Vozes Digitais", "synth,digital,vogais", "lead", "", "", 4200, WTB (a, 3, 0.3, 2, 8) FT (1, 0.1) MOD (1, 1, 7, 0.3) " lfo1_rate=0.3 lfo1_wave=1" AMP (30, 500, 0.85, 400) REVERB ("fxa3", 0.6, 0.55, 0.45, 0.1, 1, 0.25) },
    { "Synths", "Sintetizador", "Vintage Synth", "Quadrada Quente", "synth,vintage,quadrada", "lead", "", "", 2800, ANA (a, 3, 2, 8) " a_pwm=0.42" MONO (40) FT (2, 0.2) " f1_env=0.4" ENV2 (5, 400, 0.3, 200) AMP (4, 350, 0.8, 200) MOD (1, 1, 19, 0.15) " lfo1_rate=0.7" DELAY ("fxa2", 0.55, 0.3, 0.5, 0.4, 0.5, 0.18) },
    // ---------------- BASS (180) ----------------
    { "Bass", "Baixo", "Sub Bass", "Sub Puro", "baixo,sub,limpo", "bass", "", "", 900, ANA (a, 0, 1, 0) MONO (15) FT (1, 0.05) AMP (3, 300, 0.9, 120) " sub_level=-14" },
    { "Bass", "Baixo", "Analog Bass", "Baixo Serra Moog", "baixo,moog,analógico", "bass", "", "", 1100, ANA (a, 2, 2, 7) MONO (25) FT (7, 0.35) " f1_env=0.7" ENV2 (2, 260, 0.2, 120) AMP (3, 350, 0.8, 140) SATUR ("fxa1", 0.2, 0.5, 0.05, 0.4, 0.85, 0.35) },
    { "Bass", "Baixo", "Growl Bass", "Growl Digital", "baixo,growl,wavetable", "bass", "", "", 2200, WTB (a, 11, 0.35, 2, 10) MONO (10) FT (2, 0.25) MOD (1, 1, 11, 0.5) " lfo1_rate=1.8" MOD (2, 1, 7, 0.3) AMP (3, 400, 0.85, 150) SATUR ("fxa1", 0.45, 0.6, 0.1, 0.6, 0.8, 0.5) },
    { "Bass", "Baixo", "FM Bass", "FM Profundo", "baixo,fm,profundo", "bass", "", "", 3000, ANA (a, 0, 1, 0) " a_fmamt=0.35 a_fmrat=1" MONO (10) MOD (1, 3, 21, -0.25) ENV2 (1, 350, 0.1, 150) FT (1, 0.05) AMP (2, 450, 0.8, 130) },
    { "Bass", "Baixo", "Reese Bass", "Reese Larga", "baixo,reese,largo", "bass", "", "", 1600, ANA (a, 2, 3, 22) ANA (b, 2, 3, 18) " b_level=-3 blend=0.5" MONO (15) FT (2, 0.15) AMP (4, 600, 0.9, 200) CHORUS ("fxa1", 0.2, 0.4, 0.5, 0.5, 1, 0.25) },
    { "Bass", "Baixo", "Moog-style Bass", "Moog Gordo", "baixo,moog,gordo", "bass", "", "", 900, ANA (a, 2, 2, 6) ANA (b, 3, 1, 0) " b_pwm=0.45 b_coarse=-12 b_level=-4 blend=0.5" MONO (30) FT (7, 0.45) " f1_env=0.65" ENV2 (2, 320, 0.2, 110) AMP (3, 380, 0.85, 120) },
    { "Bass", "Baixo", "Log Drum Bass", "Log Bass Corda", "baixo,log,físico", "bass", "", "", 1500, KSTR (a, 0.6, 0.45, 0.4) MONO (10) FT (1, 0.05) AMP (1, 700, 0.5, 150) SATUR ("fxa1", 0.2, 0.5, 0.05, 0.4, 0.85, 0.3) },
    { "Bass", "Baixo", "Analog Bass", "Baixo Acid Ressonante", "baixo,acid,303", "bass", "", "", 1800, ANA (a, 2, 1, 0) MONO (60) FT (2, 0.6) " f1_env=0.8" ENV2 (1, 240, 0.0, 90) AMP (2, 300, 0.7, 100) },
    { "Bass", "Baixo", "Sub Bass", "Sub Espectral", "baixo,sub,espectral", "bass", "", "", 1500, SPEC (a, 2.2, 2, 0.3, 0.0, 0.0, 0.0, 0.0) MONO (15) FT (1, 0.05) AMP (3, 350, 0.9, 130) },
    { "Bass", "Baixo", "FM Bass", "Baixo Dobra", "baixo,dobra,agressivo", "bass", "", "", 3500, ANA (a, 1, 1, 0) " a_fold=0.5" MONO (10) MOD (1, 3, 30, -0.3) ENV2 (1, 280, 0.2, 100) FT (2, 0.2) AMP (2, 380, 0.8, 120) SATUR ("fxa1", 0.3, 0.6, 0.05, 0.5, 0.8, 0.4) },
    // ---------------- LEADS (150) ----------------
    { "Leads", "Lead", "Mono Lead", "Mono Serra Sync", "lead,mono,sync", "lead", "", "", 4200, ANA (a, 2, 2, 8) " a_sync=7" MONO (60) FT (2, 0.2) " f1_env=0.45" ENV2 (3, 400, 0.3, 200) AMP (3, 350, 0.85, 200) MOD (1, 1, 3, 0.02) " lfo1_rate=5.5 lfo1_fade=600" DELAY ("fxa2", 0.55, 0.3, 0.5, 0.4, 0.5, 0.2) },
    { "Leads", "Lead", "Expressive Lead", "Expressivo Pressão", "lead,expressivo,pressão", "lead", "", "", 3800, ANA (a, 2, 3, 10) LEGATO (50) FT (2, 0.15) MOD (1, 8, 11, 0.4) MOD (2, 1, 3, 0.02) " lfo1_rate=5.2 lfo1_fade=500" AMP (25, 400, 0.9, 260) REVERB ("fxa3", 0.55, 0.5, 0.5, 0.08, 0.9, 0.22) },
    { "Leads", "Lead", "Vocal Lead", "Voz Formante", "lead,vocal,formante", "lead", "", "", 2800, ANA (a, 2, 2, 8) LEGATO (40) FT (9, 0.55) MOD (1, 1, 11, 0.5) " lfo1_rate=0.5" AMP (30, 500, 0.85, 300) REVERB ("fxa3", 0.6, 0.55, 0.45, 0.1, 1, 0.25) },
    { "Leads", "Lead", "Acid Lead", "Acid Líquido", "lead,acid,líquido", "lead", "", "", 2500, ANA (a, 2, 1, 0) MONO (45) FT (7, 0.55) " f1_env=0.8" ENV2 (1, 280, 0.0, 120) AMP (2, 320, 0.7, 120) DELAY ("fxa2", 0.6, 0.4, 0.5, 0.7, 0.5, 0.3) },
    { "Leads", "Lead", "Portamento Lead", "Glide Triângulo", "lead,portamento,suave", "lead", "", "", 4500, ANA (a, 1, 2, 7) LEGATO (110) FT (1, 0.05) MOD (1, 1, 3, 0.02) " lfo1_rate=5 lfo1_fade=700" AMP (20, 400, 0.9, 300) REVERB ("fxa3", 0.55, 0.5, 0.5, 0.08, 0.9, 0.22) },
    { "Leads", "Lead", "Poly Lead", "Poly Lead Tabela", "lead,poly,wavetable", "lead", "", "", 7000, WTB (a, 13, 0.3, 3, 12) FT (2, 0.1) " f1_env=0.3" ENV2 (4, 500, 0.4, 250) AMP (4, 450, 0.85, 280) CHORUS ("fxa1", 0.25, 0.4, 0.5, 0.5, 1, 0.3) DELAY ("fxa2", 0.6, 0.3, 0.5, 0.6, 0.5, 0.2) },
    { "Leads", "Lead", "Saw Lead", "Serra Festival", "lead,serra,festival", "lead", "", "", 8500, ANA (a, 2, 5, 20) ANA (b, 2, 3, 12) " b_coarse=12 b_level=-8 blend=0.5" FT (2, 0.12) AMP (4, 500, 0.85, 300) CHORUS ("fxa1", 0.3, 0.4, 0.5, 0.5, 1, 0.3) DELAY ("fxa2", 0.6, 0.35, 0.5, 0.6, 0.5, 0.25) REVERB ("fxa3", 0.65, 0.6, 0.45, 0.1, 1, 0.25) },
    { "Leads", "Lead", "Mono Lead", "Lead Espectral Ar", "lead,espectral,ar", "lead", "", "", 9000, SPEC (a, 0.8, 14, 0.5, 0.2, 0.0, 0.6, 0.2) FT (1, 0.05) MOD (1, 1, 7, 0.4) " lfo1_rate=0.35 lfo1_wave=1" AMP (20, 600, 0.8, 400) DELAY ("fxa2", 0.6, 0.35, 0.5, 0.6, 0.5, 0.22) REVERB ("fxa3", 0.65, 0.6, 0.45, 0.1, 1, 0.28) },
    // ---------------- PADS (150) ----------------
    { "Pads", "Pad", "Granular Pad", "Pad Granular Veludo", "pad,granular,veludo", "pad", "nucleo.pad", "", 7000, GRN (a, 160, 26, 0.3, 0.3, 0.05, 0.25, 0.8) FT (1, 0.05) AMP (700, 900, 0.9, 1100) CHORUS ("fxa1", 0.2, 0.4, 0.5, 0.5, 1, 0.25) REVERB ("fxa3", 0.8, 0.8, 0.45, 0.12, 1, 0.4) },
    { "Pads", "Pad", "Granular Pad", "Nuvem de Vozes", "pad,granular,vozes", "pad", "nucleo.vox", "", 6000, GRN (a, 220, 22, 0.2, 0.4, 0.08, 0.3, 0.9) FT (1, 0.05) AMP (800, 900, 0.9, 1300) REVERB ("fxa3", 0.85, 0.85, 0.45, 0.14, 1, 0.45) },
    { "Pads", "Pad", "Evolving Pad", "Espectro em Evolução", "pad,espectral,evolutivo", "pad", "", "", 9000, SPEC (a, 1.2, 4, 0.5, 0.15, 0.0, 1.0, 0.15) MOD (1, 1, 7, 0.45) " lfo1_rate=0.08 lfo1_wave=1" FT (1, 0.05) AMP (900, 1000, 0.9, 1500) CHORUS ("fxa1", 0.15, 0.4, 0.5, 0.5, 1, 0.3) REVERB ("fxa3", 0.85, 0.85, 0.45, 0.14, 1, 0.45) },
    { "Pads", "Pad", "Warm Pad", "Pad Quente Analógico", "pad,quente,analógico", "pad", "", "", 2600, ANA (a, 2, 5, 18) ANA (b, 1, 3, 10) " b_coarse=-12 b_level=-4 blend=0.5" FT (2, 0.1) AMP (600, 800, 0.9, 1000) CHORUS ("fxa1", 0.2, 0.45, 0.5, 0.5, 1, 0.4) REVERB ("fxa3", 0.75, 0.7, 0.5, 0.1, 1, 0.35) },
    { "Pads", "Pad", "Dark Pad", "Pad Sombrio Grave", "pad,sombrio,grave", "pad", "", "", 1500, ANA (a, 2, 6, 24) ANA (b, 2, 4, 16) " b_coarse=-12 b_level=-3 blend=0.5" FT (2, 0.15) MOD (1, 1, 11, 0.25) " lfo1_rate=0.1" AMP (1000, 900, 0.9, 1600) REVERB ("fxa3", 0.9, 0.9, 0.5, 0.16, 1, 0.5) },
    { "Pads", "Pad", "Ambient Pad", "Pad Cristal Ambiente", "pad,ambiente,cristal", "pad", "", "", 9500, WTB (a, 4, 0.4, 4, 16) FT (1, 0.05) MOD (1, 1, 7, 0.4) " lfo1_rate=0.09 lfo1_wave=1" AMP (1100, 1000, 0.9, 1800) DELAY ("fxa2", 0.7, 0.45, 0.5, 0.7, 0.5, 0.3) REVERB ("fxa3", 0.9, 0.9, 0.4, 0.14, 1, 0.5) },
    { "Pads", "Pad", "Analog Pad", "Pad PWM Lento", "pad,pwm,analógico", "pad", "", "", 3800, ANA (a, 3, 3, 10) " a_pwm=0.4" MOD (1, 1, 19, 0.35) " lfo1_rate=0.25" FT (2, 0.12) AMP (700, 800, 0.9, 900) CHORUS ("fxa1", 0.2, 0.45, 0.5, 0.5, 1, 0.4) REVERB ("fxa3", 0.7, 0.65, 0.5, 0.1, 1, 0.3) },
    { "Pads", "Pad", "Granular Pad", "Poeira Brilhante", "pad,granular,poeira", "pad", "nucleo.bell", "", 12000, GRN (a, 90, 40, 0.25, 0.5, 0.1, 0.2, 1.0) FT (1, 0.05) AMP (600, 900, 0.8, 1200) DELAY ("fxa2", 0.65, 0.4, 0.5, 0.7, 0.5, 0.3) REVERB ("fxa3", 0.85, 0.85, 0.4, 0.14, 1, 0.5) },
    // ---------------- KEYS (140) ----------------
    { "Keys", "Teclas", "Electric Piano", "Rhodes Suave Corda", "teclas,rhodes,suave", "keys", "", "", 6000, KSTR (a, 0.5, 0.55, 0.5) FT (1, 0.05) AMP (1, 1800, 0.0, 600) CHORUS ("fxa1", 0.25, 0.3, 0.5, 0.5, 1, 0.3) REVERB ("fxa3", 0.55, 0.5, 0.5, 0.08, 0.9, 0.22) },
    { "Keys", "Teclas", "Digital Keys", "Teclas Digitais FM", "teclas,digital,fm", "keys", "", "", 9000, WTB (a, 13, 0.3, 2, 8) FT (1, 0.05) MOD (1, 3, 8, -0.2) ENV2 (1, 800, 0.0, 400) AMP (2, 1400, 0.0, 500) CHORUS ("fxa1", 0.2, 0.3, 0.5, 0.5, 1, 0.3) REVERB ("fxa3", 0.55, 0.5, 0.5, 0.08, 0.9, 0.22) },
    { "Keys", "Teclas", "Vintage Keys", "Teclas Vintage Tremolo", "teclas,vintage,tremolo", "keys", "nucleo.ep", "", 5500, SMP (a) FT (1, 0.05) AMP (1, 1500, 0.8, 400) TREM ("fxa1", 0.45, 0.35, 0.0, 0.5, 0.5, 1) REVERB ("fxa3", 0.5, 0.45, 0.5, 0.06, 0.9, 0.2) },
    { "Keys", "Teclas", "Lo-Fi Keys", "Teclas Lo-Fi Sujas", "teclas,lofi,sujas", "keys", "nucleo.ep", "", 3200, SMP (a) FT (1, 0.1) AMP (1, 1400, 0.0, 300) CRUSH ("fxa1", 0.7, 0.55, 0.05, 0.8, 0.8, 0.55) CHORUS ("fxa2", 0.2, 0.35, 0.5, 0.5, 1, 0.35) REVERB ("fxa3", 0.5, 0.45, 0.5, 0.06, 0.9, 0.2) },
    { "Keys", "Teclas", "Soft Keys", "Teclas Suaves Lentas", "teclas,suaves,lentas", "keys", "nucleo.ep", "", 4000, SMP (a) FT (1, 0.05) AMP (40, 1600, 0.6, 700) REVERB ("fxa3", 0.7, 0.7, 0.5, 0.1, 1, 0.35) },
    { "Keys", "Teclas", "Hybrid Keys", "Teclas Híbridas Cristal", "teclas,híbridas,cristal", "keys", "nucleo.ep", "", 8000, SMP (a) WTB (b, 4, 0.4, 2, 8) " b_level=-9 blend=0.35" FT (1, 0.05) AMP (3, 1600, 0.3, 700) CHORUS ("fxa1", 0.2, 0.3, 0.5, 0.5, 1, 0.3) REVERB ("fxa3", 0.65, 0.6, 0.5, 0.1, 1, 0.3) },
    // ---------------- DRUMS (100) ----------------
    { "Drums", "Bateria", "Kick", "Bombo Sintético Sub", "bateria,bombo,sintético", "kick", "", "", 6000, ANA (a, 0, 1, 0) " a_retrig=1" MONO (1) MOD (1, 3, 3, 0.95) ENV2 (1, 55, 0.0, 30) FT (1, 0.05) AMP (1, 360, 0.0, 120) SATUR ("fxa1", 0.25, 0.5, 0.05, 0.5, 0.85, 0.4) },
    { "Drums", "Bateria", "Kick", "Bombo 909 Duro", "bateria,bombo,909", "kick", "", "", 7000, ANA (a, 0, 1, 0) " a_retrig=1" MONO (1) MOD (1, 3, 3, 0.7) ENV2 (1, 40, 0.0, 25) FT (1, 0.05) AMP (1, 260, 0.0, 100) SATUR ("fxa1", 0.45, 0.6, 0.05, 0.6, 0.8, 0.5) },
    { "Drums", "Bateria", "Snare", "Caixa Sintética Curta", "bateria,caixa,sintética", "snare", "nucleo.snare", "", 9000, SMP (a) FT (1, 0.05) AMP (1, 400, 0.0, 120) COMP ("fxa1", 0.45, 0.5, 0.3, 0.4, 0.3, 1) },
    { "Drums", "Bateria", "Clap", "Palmas Largas", "bateria,palmas,largas", "clap", "nucleo.clap", "", 12000, SMP (a) FT (1, 0.05) AMP (1, 600, 0.0, 200) REVERB ("fxa3", 0.5, 0.45, 0.5, 0.05, 0.9, 0.25) },
    { "Drums", "Bateria", "Hi-Hat", "Hat Fechado Seco", "bateria,hat,fechado", "hat", "nucleo.hat", "", 14000, SMP (a) FT (3, 0.05) AMP (1, 200, 0.0, 60) },
    { "Drums", "Bateria", "Hi-Hat", "Hat Aberto Longo", "bateria,hat,aberto", "hat", "nucleo.ohat", "", 14000, SMP (a) FT (3, 0.05) AMP (1, 700, 0.0, 250) REVERB ("fxa3", 0.5, 0.4, 0.5, 0.05, 0.9, 0.18) },
    { "Drums", "Bateria", "Tom", "Tom Grave Corda", "bateria,tom,grave", "tom", "nucleo.tom", "", 5000, SMP (a) FT (1, 0.05) AMP (1, 600, 0.0, 200) REVERB ("fxa3", 0.55, 0.5, 0.5, 0.06, 0.9, 0.22) },
    { "Drums", "Bateria", "Synth Drums", "Tom Sintético Pitch", "bateria,tom,sintético", "tom", "", "", 5000, ANA (a, 1, 1, 0) " a_retrig=1" MOD (1, 3, 3, 0.55) ENV2 (1, 160, 0.0, 80) FT (1, 0.05) AMP (1, 480, 0.0, 160) REVERB ("fxa3", 0.5, 0.45, 0.5, 0.06, 0.9, 0.2) },
    { "Drums", "Bateria", "Drum Kits", "Kit Núcleo Aberto", "bateria,kit,aberto", "kit", "nucleo.kit", "", 14000, SMP (a) FT (1, 0.05) AMP (1, 800, 1, 150) " a_loop=0 a_rr=1" COMP ("fxa1", 0.4, 0.5, 0.3, 0.4, 0.3, 1) },
    { "Drums", "Bateria", "Synth Drums", "Rim Sintético", "bateria,rim,sintético", "snare", "nucleo.rim", "", 10000, SMP (a) FT (1, 0.05) AMP (1, 240, 0.0, 80) REVERB ("fxa3", 0.4, 0.3, 0.5, 0.04, 0.9, 0.15) },
    // ---------------- PERCUSSION (80) ----------------
    { "Percussion", "Percussão", "Shakers", "Shaker Estéreo", "percussão,shaker", "shaker", "nucleo.shaker", "", 14000, SMP (a) FT (3, 0.05) AMP (1, 260, 0.0, 80) WIDTH ("fxa1", 0.9, 0.5, 0.5, 0.5, 0.5, 1) },
    { "Percussion", "Percussão", "Tuned Percussion", "Tom Afinado Madeira", "percussão,afinado,madeira", "perc", "nucleo.tom", "", 6000, SMP (a) FT (1, 0.05) AMP (1, 500, 0.0, 180) REVERB ("fxa3", 0.55, 0.5, 0.5, 0.06, 0.9, 0.22) },
    { "Percussion", "Percussão", "Tuned Percussion", "Corda Percussiva", "percussão,corda,percussiva", "perc", "", "", 5000, KSTR (a, 0.8, 0.25, 0.15) FT (1, 0.05) AMP (1, 400, 0.0, 120) REVERB ("fxa3", 0.5, 0.45, 0.5, 0.06, 0.9, 0.2) },
    { "Percussion", "Percussão", "Tuned Percussion", "Sino Percussivo", "percussão,sino,percussivo", "perc", "nucleo.bell", "", 12000, SMP (a) FT (1, 0.05) AMP (1, 500, 0.0, 200) REVERB ("fxa3", 0.6, 0.55, 0.45, 0.08, 1, 0.25) },
    { "Percussion", "Percussão", "African Percussion", "Pluck Africano Seco", "percussão,africano,seco", "perc", "nucleo.pluck", "", 6500, SMP (a) FT (1, 0.05) AMP (1, 300, 0.0, 110) REVERB ("fxa3", 0.45, 0.4, 0.5, 0.05, 0.9, 0.18) },
    { "Percussion", "Percussão", "Congas", "Madeira Sintética", "percussão,madeira,sintética", "perc", "", "", 6000, ANA (a, 1, 1, 0) " a_retrig=1" MOD (1, 3, 3, 0.35) ENV2 (1, 60, 0.0, 40) FT (1, 0.05) AMP (1, 220, 0.0, 90) REVERB ("fxa3", 0.45, 0.4, 0.5, 0.05, 0.9, 0.18) },
    // ---------------- ARPEGGIOS (100) ----------------
    { "Arpeggios", "Arp", "Melodic Arp", "Arp Melódico Pluck", "arp,melódico,pluck", "arp", "", "", 5500, ANA (a, 2, 2, 8) FT (2, 0.2) " f1_env=0.55" ENV2 (1, 250, 0.0, 120) AMP (1, 280, 0.0, 150) ARPON (12, 0, 0, 0.5, 2) DELAY ("fxa2", 0.6, 0.4, 0.5, 0.7, 0.5, 0.3) REVERB ("fxa3", 0.6, 0.5, 0.5, 0.08, 1, 0.22) },
    { "Arpeggios", "Arp", "Rhythmic Arp", "Arp Rítmico Estaca", "arp,rítmico,estaca", "arp", "", "", 4800, ANA (a, 3, 2, 8) " a_pwm=0.4" FT (2, 0.2) " f1_env=0.5" ENV2 (1, 200, 0.0, 100) AMP (1, 220, 0.0, 110) ARPON (13, 2, 1, 0.4, 1) DELAY ("fxa2", 0.55, 0.35, 0.5, 0.6, 0.5, 0.25) },
    { "Arpeggios", "Arp", "Acid Arp", "Arp Acid Rápido", "arp,acid,rápido", "arp", "", "", 2500, ANA (a, 2, 1, 0) MONO (30) FT (7, 0.55) " f1_env=0.8" ENV2 (1, 220, 0.0, 90) AMP (1, 240, 0.6, 100) ARPON (12, 0, 0, 0.55, 2) DELAY ("fxa2", 0.6, 0.4, 0.5, 0.7, 0.5, 0.3) },
    { "Arpeggios", "Arp", "Generative Arp", "Arp Gerativo Probabilístico", "arp,gerativo,probabilístico", "arp", "", "", 6000, WTB (a, 14, 0.3, 2, 6) FT (1, 0.1) AMP (1, 500, 0.0, 250) ARPON (12, 4, 0, 0.5, 3) " arp_prob=0.65 arp_seed=17" DELAY ("fxa2", 0.65, 0.45, 0.5, 0.7, 0.5, 0.3) REVERB ("fxa3", 0.7, 0.65, 0.45, 0.1, 1, 0.3) },
    { "Arpeggios", "Arp", "Rhythmic Arp", "Arp Euclidiano 5 de 16", "arp,euclidiano,rítmico", "arp", "", "", 5200, ANA (a, 2, 2, 8) FT (2, 0.2) " f1_env=0.5" ENV2 (1, 240, 0.0, 110) AMP (1, 260, 0.0, 130) ARPON (12, 0, 0, 0.5, 2) " arp_eucl_k=5 arp_eucl_rot=1" DELAY ("fxa2", 0.6, 0.35, 0.5, 0.6, 0.5, 0.25) },
    { "Arpeggios", "Arp", "Melodic Arp", "Arp Sino Cristal", "arp,sino,cristal", "arp", "", "", 11000, WTB (a, 5, 0.5, 2, 6) FT (1, 0.05) AMP (1, 900, 0.0, 500) ARPON (13, 2, 0, 0.7, 3) DELAY ("fxa2", 0.65, 0.45, 0.5, 0.7, 0.5, 0.3) REVERB ("fxa3", 0.75, 0.7, 0.4, 0.1, 1, 0.35) },
    // ---------------- VOCALS (70) ----------------
    { "Vocals", "Vozes", "Vocal Chops", "Corte de Voz Curto", "voz,corte,curto", "vocal", "nucleo.vox", "", 9000, SMP (a) FT (1, 0.05) AMP (1, 380, 0.0, 120) REVERB ("fxa3", 0.55, 0.5, 0.5, 0.06, 0.9, 0.2) },
    { "Vocals", "Vozes", "Choirs", "Coro Granular Aberto", "voz,coro,granular", "pad", "nucleo.vox", "", 6500, GRN (a, 180, 24, 0.25, 0.35, 0.04, 0.2, 0.9) FT (1, 0.05) AMP (500, 900, 0.9, 1100) REVERB ("fxa3", 0.8, 0.8, 0.45, 0.12, 1, 0.4) },
    { "Vocals", "Vozes", "Formant Leads", "Formante Vogal Móvel", "voz,formante,móvel", "lead", "", "", 3000, ANA (a, 2, 3, 10) LEGATO (40) FT (9, 0.6) MOD (1, 1, 11, 0.6) " lfo1_rate=0.45" AMP (40, 500, 0.85, 350) REVERB ("fxa3", 0.6, 0.55, 0.45, 0.1, 1, 0.28) },
    { "Vocals", "Vozes", "Vocal Pads", "Pad Vocal Espectral", "voz,pad,espectral", "pad", "", "", 8000, SPEC (a, 1.3, 5, 0.65, 0.0, 0.0, 0.5, 0.1) FT (1, 0.05) AMP (600, 900, 0.9, 1100) CHORUS ("fxa1", 0.2, 0.4, 0.5, 0.5, 1, 0.3) REVERB ("fxa3", 0.8, 0.8, 0.45, 0.12, 1, 0.4) },
    { "Vocals", "Vozes", "Vocal Textures", "Textura Vogais Tabela", "voz,textura,vogais", "texture", "", "", 6000, WTB (a, 3, 0.5, 4, 14) FT (1, 0.05) MOD (1, 1, 7, 0.5) " lfo1_rate=0.12 lfo1_wave=1" AMP (900, 900, 0.9, 1400) REVERB ("fxa3", 0.85, 0.85, 0.45, 0.14, 1, 0.45) },
    { "Vocals", "Vozes", "Vocal Chops", "Corte de Voz Pitch", "voz,corte,pitch", "vocal", "nucleo.vox", "", 9000, SMP (a) FT (1, 0.05) AMP (1, 280, 0.0, 100) DELAY ("fxa2", 0.6, 0.35, 0.5, 0.6, 0.5, 0.25) },
    // ---------------- SFX (80) ----------------
    { "SFX", "FX", "Risers", "Riser Ruído Longo", "fx,riser,ruído", "fx", "nucleo.riser", "", 14000, SMP (a) FT (1, 0.05) AMP (1, 3000, 1, 400) REVERB ("fxa3", 0.7, 0.65, 0.45, 0.1, 1, 0.3) },
    { "SFX", "FX", "Impacts", "Impacto Profundo", "fx,impacto,profundo", "fx", "nucleo.impact", "", 9000, SMP (a) FT (1, 0.05) AMP (1, 3000, 1, 400) REVERB ("fxa3", 0.8, 0.8, 0.45, 0.12, 1, 0.4) },
    { "SFX", "FX", "Sweeps", "Sweep Ruído Filtro", "fx,sweep,filtro", "fx", "", "", 8000, ANA (a, 2, 5, 40) " noise_level=-10" FT (5, 0.5) MOD (1, 4, 11, 0.9) ENV3 (3000, 500, 0.0, 600) AMP (500, 600, 0.9, 700) REVERB ("fxa3", 0.8, 0.8, 0.45, 0.12, 1, 0.4) },
    { "SFX", "FX", "Downlifters", "Descida Dissonante", "fx,descida,dissonante", "fx", "", "", 6000, ANA (a, 2, 3, 30) MOD (1, 3, 3, -0.9) ENV2 (1, 2200, 0.0, 400) FT (2, 0.3) " f1_env=-0.5" AMP (1, 2400, 0.0, 600) REVERB ("fxa3", 0.8, 0.8, 0.45, 0.12, 1, 0.4) },
    { "SFX", "FX", "Whooshes", "Whoosh Espectral", "fx,whoosh,espectral", "fx", "", "", 10000, SPEC (a, 0.6, 6, 0.4, 0.5, 0.0, 1.0, 0.6) MOD (1, 3, 7, 0.8) ENV2 (900, 900, 0.0, 500) FT (1, 0.05) AMP (400, 900, 0.0, 800) REVERB ("fxa3", 0.8, 0.8, 0.45, 0.12, 1, 0.4) },
    { "SFX", "FX", "Glitches", "Glitch Granular Picos", "fx,glitch,granular", "fx", "nucleo.pluck", "", 12000, GRN (a, 12, 90, 0.4, 0.9, 0.5, 1.0, 1.0) FT (1, 0.05) AMP (1, 700, 0.8, 200) CRUSH ("fxa1", 0.7, 0.6, 0.2, 0.8, 0.8, 0.5) },
    { "SFX", "FX", "Transitions", "Transição Granular Reversa", "fx,transição,granular", "fx", "nucleo.bell", "", 12000, GRN (a, 60, 50, 0.3, 0.6, 0.3, -0.5, 1.0) FT (1, 0.05) AMP (600, 800, 0.8, 900) REVERB ("fxa3", 0.85, 0.85, 0.4, 0.14, 1, 0.45) },
    // ---------------- TEXTURES (80) ----------------
    { "Textures", "Texturas", "Granular Textures", "Textura Granular Pad Lenta", "textura,granular,lenta", "texture", "nucleo.pad", "", 8000, GRN (a, 300, 18, 0.1, 0.6, 0.1, 0.1, 1.0) FT (1, 0.05) AMP (1200, 900, 0.9, 1800) REVERB ("fxa3", 0.9, 0.9, 0.45, 0.14, 1, 0.5) },
    { "Textures", "Texturas", "Atmospheric Textures", "Atmosfera Espectral Fria", "textura,atmosfera,espectral", "texture", "", "", 7000, SPEC (a, 1.5, 6, 0.4, 0.3, 0.0, 0.8, 0.5) MOD (1, 1, 7, 0.5) " lfo1_rate=0.06 lfo1_wave=1" FT (1, 0.05) AMP (1500, 1000, 0.9, 2000) CHORUS ("fxa1", 0.1, 0.4, 0.5, 0.5, 1, 0.3) REVERB ("fxa3", 0.9, 0.9, 0.45, 0.16, 1, 0.5) },
    { "Textures", "Texturas", "Noise Beds", "Cama de Ruído Filtrado", "textura,ruído,cama", "texture", "", "", 3000, " noise_level=-6 a_on=0 sub_level=-60" FT (5, 0.4) MOD (1, 1, 11, 0.4) " lfo1_rate=0.1" AMP (1500, 1000, 1, 1500) WIDTH ("fxa1", 1, 0.5, 0.5, 0.5, 0.5, 1) REVERB ("fxa3", 0.85, 0.85, 0.45, 0.14, 1, 0.4) },
    { "Textures", "Texturas", "Granular Textures", "Poeira de Sinos", "textura,granular,sinos", "texture", "nucleo.bell", "", 12000, GRN (a, 70, 45, 0.3, 0.7, 0.15, 0.2, 1.0) FT (1, 0.05) AMP (900, 900, 0.8, 1500) DELAY ("fxa2", 0.7, 0.5, 0.5, 0.7, 0.5, 0.3) REVERB ("fxa3", 0.9, 0.9, 0.4, 0.14, 1, 0.5) },
    { "Textures", "Texturas", "Organic Textures", "Corda Orgânica Solta", "textura,orgânica,corda", "texture", "", "", 4500, KSTR (a, 0.5, 0.9, 0.5) FT (1, 0.05) MOD (1, 1, 3, 0.01) " lfo1_rate=0.3" AMP (300, 1500, 1.0, 1400) REVERB ("fxa3", 0.85, 0.85, 0.45, 0.14, 1, 0.45) },
    { "Textures", "Texturas", "Field Recording Hybrid", "Híbrido Granular e Espectro", "textura,híbrido,granular", "texture", "nucleo.vox", "", 7000, GRN (a, 150, 20, 0.3, 0.4, 0.08, 0.2, 0.9) SPEC (b, 1.2, 6, 0.4, 0.2, 0.0, 0.7, 0.3) " b_level=-8 blend=0.5" FT (1, 0.05) AMP (1000, 900, 0.9, 1600) REVERB ("fxa3", 0.9, 0.9, 0.45, 0.14, 1, 0.5) },
    // ---------------- CHORDS (65) ----------------
    { "Chords", "Acordes", "House Chords", "Acorde House Stab", "acorde,house,stab", "stab", "", "", 4500, ANA (a, 2, 4, 14) ANA (b, 3, 2, 8) " b_pwm=0.4 b_level=-6 blend=0.5" " chord_type=2" FT (2, 0.15) " f1_env=0.45" ENV2 (2, 260, 0.0, 140) AMP (2, 300, 0.0, 180) REVERB ("fxa3", 0.5, 0.45, 0.5, 0.06, 0.9, 0.2) },
    { "Chords", "Acordes", "Neo-Soul Chords", "Neo-Soul Rhodes Largo", "acorde,neosoul,rhodes", "keys", "nucleo.ep", "", 5000, SMP (a) " chord_type=3" FT (1, 0.05) AMP (3, 1600, 0.3, 700) CHORUS ("fxa1", 0.2, 0.3, 0.5, 0.5, 1, 0.3) REVERB ("fxa3", 0.65, 0.6, 0.5, 0.1, 1, 0.3) },
    { "Chords", "Acordes", "Stabs", "Stab Brilhante Pop", "acorde,stab,pop", "stab", "", "", 7000, ANA (a, 2, 5, 18) " chord_type=1" FT (2, 0.1) " f1_env=0.5" ENV2 (2, 240, 0.0, 120) AMP (2, 260, 0.0, 160) CHORUS ("fxa1", 0.3, 0.4, 0.5, 0.5, 1, 0.25) REVERB ("fxa3", 0.55, 0.5, 0.5, 0.08, 0.9, 0.2) },
    { "Chords", "Acordes", "Chord Memory", "Memória Pad Espectral", "acorde,memória,espectral", "pad", "", "", 8000, SPEC (a, 1.1, 8, 0.4, 0.1, 0.0, 0.4, 0.2) " chord_type=2" FT (1, 0.05) AMP (400, 900, 0.9, 900) CHORUS ("fxa1", 0.2, 0.4, 0.5, 0.5, 1, 0.3) REVERB ("fxa3", 0.8, 0.75, 0.45, 0.12, 1, 0.35) },
    // ---------------- PERCUSSION: mais bases ----------------
    { "Percussion", "Percussão", "Congas", "Conga Corda Aberta", "percussão,conga,corda", "perc", "", "", 6500, KSTR (a, 0.7, 0.2, 0.2) FT (1, 0.05) AMP (1, 320, 0.0, 100) REVERB ("fxa3", 0.45, 0.4, 0.5, 0.05, 0.9, 0.18) },
    { "Percussion", "Percussão", "Bongos", "Bongó Sintético Agudo", "percussão,bongó,agudo", "perc", "", "", 8000, ANA (a, 1, 1, 0) " a_retrig=1 a_coarse=12" MOD (1, 3, 3, 0.3) ENV2 (1, 45, 0.0, 30) FT (1, 0.05) AMP (1, 160, 0.0, 70) REVERB ("fxa3", 0.4, 0.35, 0.5, 0.04, 0.9, 0.15) },
    { "Percussion", "Percussão", "Shakers", "Shaker Ruído Filtrado", "percussão,shaker,ruído", "shaker", "", "", 9000, " a_on=0 noise_level=-4" FT (5, 0.4) AMP (4, 160, 0.0, 60) WIDTH ("fxa1", 0.9, 0.5, 0.5, 0.5, 0.5, 1) },
    { "Percussion", "Percussão", "Timbales", "Timbal Metálico", "percussão,timbal,metálico", "perc", "nucleo.rim", "", 9000, SMP (a) " a_coarse=5" FT (1, 0.05) AMP (1, 380, 0.0, 120) REVERB ("fxa3", 0.45, 0.4, 0.5, 0.05, 0.9, 0.18) },
    { "Percussion", "Percussão", "African Percussion", "Marimbolo Seco", "percussão,africano,marimbolo", "perc", "nucleo.pluck", "", 5000, SMP (a) " a_coarse=-5" FT (1, 0.05) AMP (1, 420, 0.0, 140) REVERB ("fxa3", 0.5, 0.45, 0.5, 0.06, 0.9, 0.2) },
    { "Percussion", "Percussão", "Tuned Percussion", "Percussão Afinada Espectral", "percussão,afinada,espectral", "perc", "", "", 9000, SPEC (a, 1.4, 14, 0.7, 0.3, 0.0, 0.0, 0.0) FT (1, 0.05) AMP (1, 480, 0.0, 160) REVERB ("fxa3", 0.55, 0.5, 0.5, 0.06, 0.9, 0.22) },
    { "Percussion", "Percussão", "Tuned Percussion", "Tambor de Madeira Tom", "percussão,tambor,madeira", "perc", "nucleo.tom", "", 4500, SMP (a) " a_coarse=7" FT (1, 0.05) AMP (1, 360, 0.0, 120) },
    { "Percussion", "Percussão", "Shakers", "Chocalho Granular", "percussão,chocalho,granular", "shaker", "nucleo.shaker", "", 12000, GRN (a, 25, 55, 0.2, 0.8, 0.1, 1.0, 1.0) FT (3, 0.05) AMP (1, 420, 0.6, 120) },
    { "Percussion", "Percussão", "Congas", "Tabla Sintética", "percussão,tabla,sintética", "perc", "", "", 6000, ANA (a, 1, 1, 0) " a_retrig=1" MOD (1, 3, 3, 0.45) ENV2 (1, 90, 0.0, 50) FT (1, 0.05) AMP (1, 260, 0.0, 100) REVERB ("fxa3", 0.45, 0.4, 0.5, 0.05, 0.9, 0.18) },
    { "Percussion", "Percussão", "African Percussion", "Calabash Corda Dura", "percussão,calabash,corda", "perc", "", "", 5500, KSTR (a, 0.3, 0.3, 0.1) FT (1, 0.05) AMP (1, 380, 0.0, 120) REVERB ("fxa3", 0.5, 0.45, 0.5, 0.06, 0.9, 0.2) },
    // ---------------- ARPEGGIOS: mais bases ----------------
    { "Arpeggios", "Arp", "Afro Arp", "Arp Afro Kalimba", "arp,afro,kalimba", "arp", "", "", 7000, WTB (a, 14, 0.4, 2, 6) FT (1, 0.05) AMP (1, 600, 0.0, 300) ARPON (12, 0, 3, 0.55, 2) REVERB ("fxa3", 0.6, 0.55, 0.5, 0.08, 1, 0.25) },
    { "Arpeggios", "Arp", "Rhythmic Arp", "Arp Corda Pizzicato", "arp,corda,pizzicato", "arp", "", "", 5500, KSTR (a, 0.5, 0.35, 0.4) FT (1, 0.05) AMP (1, 500, 0.0, 200) ARPON (13, 2, 0, 0.5, 2) DELAY ("fxa2", 0.55, 0.4, 0.5, 0.7, 0.5, 0.25) },
    { "Arpeggios", "Arp", "Generative Arp", "Arp Granular Cristal", "arp,granular,cristal", "arp", "nucleo.bell", "", 11000, GRN (a, 90, 30, 0.2, 0.2, 0.05, 0.0, 0.8) FT (1, 0.05) AMP (1, 900, 0.0, 400) ARPON (12, 4, 0, 0.6, 2) " arp_prob=0.75 arp_seed=33" REVERB ("fxa3", 0.75, 0.7, 0.4, 0.1, 1, 0.3) },
    { "Arpeggios", "Arp", "Melodic Arp", "Arp Espectral Brilhante", "arp,espectral,brilhante", "arp", "", "", 9000, SPEC (a, 1.0, 10, 0.5, 0.2, 0.0, 0.5, 0.0) FT (1, 0.05) AMP (1, 420, 0.0, 200) ARPON (12, 0, 0, 0.5, 3) DELAY ("fxa2", 0.6, 0.4, 0.5, 0.7, 0.5, 0.28) },
    { "Arpeggios", "Arp", "Acid Arp", "Arp Trance Serra", "arp,trance,serra", "arp", "", "", 6500, ANA (a, 2, 3, 12) FT (2, 0.25) " f1_env=0.55" ENV2 (1, 200, 0.0, 90) AMP (1, 220, 0.0, 100) ARPON (13, 0, 0, 0.45, 3) DELAY ("fxa2", 0.55, 0.45, 0.5, 0.7, 0.5, 0.32) },
    { "Arpeggios", "Arp", "Rhythmic Arp", "Arp Euclidiano 7 de 16", "arp,euclidiano,sincopado", "arp", "", "", 5000, ANA (a, 3, 2, 8) " a_pwm=0.35" FT (2, 0.2) " f1_env=0.5" ENV2 (1, 220, 0.0, 100) AMP (1, 240, 0.0, 120) ARPON (12, 5, 0, 0.5, 2) " arp_eucl_k=7 arp_eucl_rot=2" DELAY ("fxa2", 0.6, 0.35, 0.5, 0.6, 0.5, 0.25) },
    { "Arpeggios", "Arp", "Melodic Arp", "Arp Sino FM Lento", "arp,sino,fm", "arp", "", "", 9000, ANA (a, 0, 1, 0) " a_fmamt=0.35 a_fmrat=3" MOD (1, 3, 21, -0.2) ENV2 (1, 700, 0.0, 300) FT (1, 0.05) AMP (1, 800, 0.0, 350) ARPON (11, 2, 0, 0.6, 2) REVERB ("fxa3", 0.7, 0.65, 0.4, 0.1, 1, 0.3) },
    { "Arpeggios", "Arp", "Afro Arp", "Arp Afro Guitarra", "arp,afro,guitarra", "arp", "", "", 5500, KSTR (a, 0.45, 0.5, 0.45) FT (1, 0.05) AMP (1, 700, 0.0, 280) ARPON (12, 0, 3, 0.55, 2) REVERB ("fxa3", 0.55, 0.5, 0.5, 0.08, 1, 0.22) },
    // ---------------- SFX: mais bases ----------------
    { "SFX", "FX", "Sweeps", "Sweep Espectral Subida", "fx,sweep,espectral", "fx", "", "", 12000, SPEC (a, 0.5, 4, 0.5, 0.4, 0.0, 1.0, 0.3) MOD (1, 3, 7, 0.9) ENV2 (2600, 800, 1.0, 400) FT (1, 0.05) AMP (800, 800, 1.0, 600) REVERB ("fxa3", 0.8, 0.8, 0.45, 0.12, 1, 0.35) },
    { "SFX", "FX", "Impacts", "Impacto Sub Queda", "fx,impacto,sub", "fx", "", "", 1500, ANA (a, 0, 1, 0) " a_retrig=1" MOD (1, 3, 3, 0.9) ENV2 (1, 500, 0.0, 200) FT (1, 0.05) AMP (1, 1800, 0.0, 400) REVERB ("fxa3", 0.8, 0.8, 0.45, 0.12, 1, 0.4) },
    { "SFX", "FX", "Whooshes", "Whoosh de Ruído", "fx,whoosh,ruído", "fx", "", "", 6000, " a_on=0 noise_level=-4" FT (5, 0.4) MOD (1, 3, 11, 0.9) ENV2 (700, 900, 0.0, 500) AMP (500, 800, 0.0, 700) WIDTH ("fxa1", 1, 0.5, 0.5, 0.5, 0.5, 1) REVERB ("fxa3", 0.8, 0.8, 0.45, 0.12, 1, 0.35) },
    { "SFX", "FX", "Glitches", "Glitch Crush Rápido", "fx,glitch,crush", "fx", "nucleo.stab", "", 10000, SMP (a) FT (1, 0.05) AMP (1, 260, 0.0, 80) CRUSH ("fxa1", 0.75, 0.7, 0.3, 0.8, 0.8, 0.7) DELAY ("fxa2", 0.5, 0.5, 0.5, 0.6, 0.5, 0.3) },
    { "SFX", "FX", "Risers", "Riser Espectral Tensão", "fx,riser,tensão", "fx", "", "", 12000, SPEC (a, 0.7, 5, 0.5, 0.5, 0.0, 1.0, 0.4) MOD (1, 3, 3, 0.5) ENV2 (3000, 500, 1.0, 300) FT (1, 0.05) AMP (1200, 800, 1.0, 400) REVERB ("fxa3", 0.8, 0.8, 0.45, 0.12, 1, 0.35) },
    { "SFX", "FX", "Transitions", "Transição Pitch Reversa", "fx,transição,reversa", "fx", "nucleo.pad", "", 9000, GRN (a, 80, 35, 0.4, 0.3, 0.2, -0.8, 1.0) FT (1, 0.05) AMP (700, 800, 0.8, 800) " fxa1_type=16 fxa1_p1=0.7 fxa1_p2=0.5 fxa1_p3=0.5 fxa1_p4=0.3 fxa1_p5=0.5 fxa1_mix=0.5" REVERB ("fxa3", 0.8, 0.8, 0.45, 0.12, 1, 0.4) },
    // ---------------- QUOTAS: bases extra (Vocals, Percussion, SFX, Drums) ----------------
    { "Vocals", "Vozes", "Vocal Chops", "Corte de Voz Grave", "voz,corte,grave", "vocal", "nucleo.vox", "", 5500, SMP (a) " a_coarse=-7" FT (1, 0.05) AMP (1, 420, 0.0, 140) REVERB ("fxa3", 0.55, 0.5, 0.5, 0.06, 0.9, 0.2) },
    { "Vocals", "Vozes", "Choirs", "Coro Reverso Granular", "voz,coro,reverso", "pad", "nucleo.vox", "", 7000, GRN (a, 220, 20, 0.3, 0.3, 0.05, -0.6, 1.0) FT (1, 0.05) AMP (700, 900, 0.9, 1200) REVERB ("fxa3", 0.85, 0.85, 0.45, 0.12, 1, 0.42) },
    { "Vocals", "Vozes", "Vocal Pads", "Pad Ah Wavetable", "voz,pad,ah", "pad", "", "", 7000, WTB (a, 3, 0.3, 3, 12) FT (1, 0.05) AMP (500, 900, 0.9, 1100) CHORUS ("fxa1", 0.2, 0.4, 0.5, 0.5, 0.8, 0.3) REVERB ("fxa3", 0.75, 0.75, 0.45, 0.12, 1, 0.38) },
    { "Vocals", "Vozes", "Formant Leads", "Formante Oo Suave", "voz,formante,oo", "lead", "", "", 2600, ANA (a, 1, 2, 8) LEGATO (60) FT (9, 0.5) MOD (1, 1, 11, 0.4) " lfo1_rate=0.35" AMP (60, 600, 0.85, 400) REVERB ("fxa3", 0.65, 0.6, 0.45, 0.1, 1, 0.3) },
    { "Vocals", "Vozes", "Vocal Textures", "Respiração Vocal Espectral", "voz,respiração,espectral", "texture", "", "", 7500, SPEC (a, 1.8, 9, 0.5, 0.0, 0.5, 0.7, 0.4) FT (1, 0.05) MOD (1, 1, 7, 0.4) " lfo1_rate=0.1 lfo1_wave=1" AMP (900, 900, 0.9, 1400) REVERB ("fxa3", 0.85, 0.85, 0.45, 0.14, 1, 0.42) },
    { "Percussion", "Percussão", "Tuned Percussion", "Marimba Seca Sintética", "percussão,marimba,seca", "perc", "nucleo.pluck", "", 7000, SMP (a) " a_coarse=-12" FT (1, 0.05) AMP (1, 520, 0.0, 180) REVERB ("fxa3", 0.5, 0.45, 0.5, 0.06, 0.9, 0.2) },
    { "Percussion", "Percussão", "African Percussion", "Djembé Seco Sintético", "percussão,djembé,seco", "perc", "nucleo.tom", "", 5500, SMP (a) " a_coarse=12" FT (1, 0.05) AMP (1, 300, 0.0, 100) REVERB ("fxa3", 0.4, 0.35, 0.5, 0.04, 0.9, 0.15) },
    { "Percussion", "Percussão", "Shakers", "Maraca Granular", "percussão,maraca,granular", "shaker", "nucleo.shaker", "", 13000, GRN (a, 18, 70, 0.1, 0.9, 0.1, 1.0, 1.0) FT (3, 0.05) AMP (1, 260, 0.5, 90) },
    { "Percussion", "Percussão", "Congas", "Conga Morta Curta", "percussão,conga,morta", "perc", "", "", 5000, ANA (a, 1, 1, 0) " a_retrig=1" MOD (1, 3, 3, 0.4) ENV2 (1, 70, 0.0, 40) FT (1, 0.05) AMP (1, 150, 0.0, 60) },
    { "Percussion", "Percussão", "Tuned Percussion", "Cowbell Metálico FM", "percussão,cowbell,fm", "perc", "", "", 9500, ANA (a, 0, 1, 0) " a_fmamt=0.5 a_fmrat=3 a_coarse=7" FT (1, 0.05) AMP (1, 260, 0.0, 100) REVERB ("fxa3", 0.45, 0.4, 0.5, 0.05, 0.9, 0.18) },
    { "Percussion", "Percussão", "African Percussion", "Agogô Duplo", "percussão,agogô,duplo", "perc", "nucleo.rim", "", 9000, SMP (a) " a_coarse=9" FT (1, 0.05) AMP (1, 320, 0.0, 110) DELAY ("fxa2", 0.5, 0.3, 0.5, 0.5, 0.5, 0.22) },
    { "SFX", "FX", "Risers", "Riser de Ruído Branco", "fx,riser,ruído", "fx", "", "", 14000, " a_on=0 noise_level=-3" FT (5, 0.3) MOD (1, 3, 11, 0.9) ENV2 (3500, 500, 1.0, 300) AMP (1500, 800, 1.0, 400) REVERB ("fxa3", 0.8, 0.8, 0.45, 0.12, 1, 0.35) },
    { "SFX", "FX", "Impacts", "Impacto Granular Pesado", "fx,impacto,granular", "fx", "nucleo.tom", "", 3000, GRN (a, 120, 30, 0.3, 0.3, 0.1, -0.5, 1.0) FT (1, 0.05) AMP (1, 2500, 0.0, 500) REVERB ("fxa3", 0.85, 0.85, 0.45, 0.12, 1, 0.4) },
    { "SFX", "FX", "Glitches", "Glitch Espectral Rápido", "fx,glitch,espectral", "fx", "", "", 11000, SPEC (a, 0.6, 20, 0.6, 0.5, 0.0, 1.0, 0.5) FT (1, 0.05) AMP (1, 220, 0.0, 80) CRUSH ("fxa1", 0.7, 0.7, 0.3, 0.8, 0.8, 0.6) },
    { "SFX", "FX", "Whooshes", "Whoosh Stereo Lento", "fx,whoosh,lento", "fx", "", "", 6500, " a_on=0 noise_level=-4" FT (5, 0.35) MOD (1, 3, 11, 0.8) ENV2 (1500, 1500, 0.0, 800) AMP (1000, 1200, 0.0, 900) WIDTH ("fxa1", 1, 0.5, 0.5, 0.5, 0.5, 1) REVERB ("fxa3", 0.85, 0.85, 0.45, 0.12, 1, 0.38) },
    { "Drums", "Bateria", "Kick", "Bombo Seco Curto", "bateria,bombo,seco", "kick", "", "", 6500, ANA (a, 0, 1, 0) " a_retrig=1" MONO (1) MOD (1, 3, 3, 0.8) ENV2 (1, 30, 0.0, 20) FT (1, 0.05) AMP (1, 170, 0.0, 60) SATUR ("fxa1", 0.35, 0.5, 0.05, 0.5, 0.85, 0.4) },
    { "Drums", "Bateria", "Snare", "Rimshot Sintético Duro", "bateria,rim,duro", "snare", "nucleo.rim", "", 9000, SMP (a) " a_coarse=3" FT (1, 0.05) AMP (1, 220, 0.0, 80) COMP ("fxa1", 0.45, 0.5, 0.3, 0.4, 0.3, 1) },
    };
    std::vector<Preset> out; buildFamRecipes (R, sizeof (R) / sizeof (R[0]), "nucleo", "multi", out); return out;
}

// ============================================================================ Packs novos ============================================================================
inline std::vector<Preset> genBasesPack (const std::string& id)
{
    std::vector<Preset> out;
    if (id == "house")
    {
        static const FamRecipe R[] = {
        { "Bass", "Baixo", "Deep Bass", "Baixo Deep Redondo", "house,deep,baixo", "bass", "", "", 900, ANA (a, 2, 1, 0) MONO (10) FT (2, 0.2) " f1_env=0.5" ENV2 (2, 220, 0.2, 100) AMP (3, 300, 0.8, 110) " sub_level=-12" },
        { "Bass", "Baixo", "Acid Bass", "Acid House 303", "house,acid,303", "bass", "", "", 2000, ANA (a, 2, 1, 0) MONO (40) FT (7, 0.6) " f1_env=0.8" ENV2 (1, 250, 0.0, 90) AMP (2, 300, 0.7, 100) },
        { "Chords", "Acordes", "Chord Stab", "Stab House Clássico", "house,stab,acorde", "stab", "", "", 4500, ANA (a, 2, 4, 14) ANA (b, 3, 2, 8) " b_pwm=0.4 b_level=-6 blend=0.5 chord_type=2" FT (2, 0.15) " f1_env=0.45" ENV2 (2, 260, 0.0, 140) AMP (2, 300, 0.0, 180) REVERB ("fxa3", 0.5, 0.45, 0.5, 0.06, 0.9, 0.2) },
        { "Organs", "Teclas", "Organ House", "Órgão House Dançante", "house,órgão,dança", "keys", "", "", 6000, WTB (a, 2, 0.5, 1, 0) FT (1, 0.05) AMP (3, 300, 0.9, 120) TREM ("fxa1", 0.5, 0.25, 0.0, 0.5, 0.5, 1) REVERB ("fxa3", 0.5, 0.45, 0.5, 0.06, 0.9, 0.2) },
        { "Keys", "Teclas", "Piano House", "Piano House Estaca", "house,piano,estaca", "keys", "house.piano", "", 9000, SMP (a) FT (1, 0.05) AMP (1, 700, 0.0, 200) COMP ("fxa1", 0.45, 0.5, 0.3, 0.4, 0.3, 1) REVERB ("fxa3", 0.55, 0.5, 0.5, 0.06, 0.9, 0.2) },
        { "Plucks", "Pluck", "Tech Pluck", "Tech Pluck Seco", "tech,pluck,seco", "pluck", "", "", 5000, ANA (a, 2, 2, 8) FT (2, 0.25) " f1_env=0.6" ENV2 (1, 200, 0.0, 100) AMP (1, 240, 0.0, 120) DELAY ("fxa2", 0.55, 0.4, 0.5, 0.7, 0.5, 0.3) },
        { "Leads", "Lead", "Analog Lead", "Lead Analógico Deep", "house,lead,analógico", "lead", "", "", 4000, ANA (a, 2, 2, 8) MONO (30) FT (2, 0.2) " f1_env=0.4" ENV2 (4, 450, 0.3, 200) AMP (4, 400, 0.85, 220) MOD (1, 1, 3, 0.015) " lfo1_rate=5.3 lfo1_fade=600" DELAY ("fxa2", 0.55, 0.3, 0.5, 0.4, 0.5, 0.2) },
        { "Arpeggios", "Arp", "Club Arp", "Arp de Clube Pulsante", "house,arp,clube", "arp", "", "", 5000, ANA (a, 3, 2, 8) " a_pwm=0.4" FT (2, 0.25) " f1_env=0.5" ENV2 (1, 220, 0.0, 100) AMP (1, 240, 0.0, 120) ARPON (12, 0, 0, 0.5, 2) DELAY ("fxa2", 0.55, 0.4, 0.5, 0.7, 0.5, 0.3) },
        { "Pads", "Pad", "Deep Pad", "Pad Deep Suave", "house,deep,pad", "pad", "", "", 2500, ANA (a, 2, 4, 14) FT (2, 0.12) AMP (300, 800, 0.85, 700) CHORUS ("fxa1", 0.2, 0.45, 0.5, 0.5, 1, 0.4) REVERB ("fxa3", 0.7, 0.65, 0.5, 0.1, 1, 0.3) },
        { "Drums", "Bateria", "Kit", "Kit House Club", "house,kit,bateria", "kit", "house.kit", "", 14000, SMP (a) AMP (1, 800, 1, 150) " a_loop=0 a_rr=1" },
        { "Bass", "Baixo", "Deep Bass", "Baixo Sub Techno", "tech,sub,baixo", "bass", "", "", 700, ANA (a, 0, 1, 0) MONO (5) FT (1, 0.05) AMP (3, 250, 0.9, 90) " sub_level=-10" },
        { "Chords", "Acordes", "Chord Stab", "Stab Deep Menor", "house,deep,stab", "stab", "", "", 3800, ANA (a, 2, 3, 12) ANA (b, 1, 2, 6) " b_level=-5 blend=0.5 chord_type=3" FT (2, 0.12) " f1_env=0.4" ENV2 (3, 300, 0.0, 160) AMP (3, 340, 0.0, 200) CHORUS ("fxa1", 0.25, 0.4, 0.5, 0.5, 1, 0.3) REVERB ("fxa3", 0.55, 0.5, 0.5, 0.08, 0.9, 0.22) },
        { "Synths", "Sintetizador", "Tech Synth", "Synth Tech Metálico", "tech,synth,metálico", "lead", "", "", 5500, WTB (a, 11, 0.4, 2, 8) FT (2, 0.2) " f1_env=0.45" ENV2 (1, 300, 0.1, 150) AMP (2, 340, 0.3, 160) DELAY ("fxa2", 0.55, 0.4, 0.5, 0.7, 0.5, 0.28) },
        { "Vocals", "Vozes", "Vocal Chop", "Corte de Voz House", "house,voz,corte", "vocal", "house.vox", "", 9000, SMP (a) FT (1, 0.05) AMP (1, 320, 0.0, 120) DELAY ("fxa2", 0.55, 0.35, 0.5, 0.6, 0.5, 0.25) REVERB ("fxa3", 0.55, 0.5, 0.5, 0.06, 0.9, 0.2) },
        { "SFX", "FX", "Risers", "Riser de Pista", "house,riser,pista", "fx", "house.riser", "", 14000, SMP (a) AMP (1, 3000, 1, 400) REVERB ("fxa3", 0.7, 0.65, 0.45, 0.1, 1, 0.3) },
        { "Keys", "Teclas", "Electric Piano", "Rhodes House Quente", "house,rhodes,quente", "keys", "house.organ", "", 7000, SMP (a) FT (1, 0.05) AMP (2, 1200, 0.2, 400) CHORUS ("fxa1", 0.25, 0.3, 0.5, 0.5, 1, 0.3) REVERB ("fxa3", 0.55, 0.5, 0.5, 0.08, 0.9, 0.22) },
        { "Percussion", "Percussão", "Shakers", "Shaker de Clube", "house,shaker,clube", "shaker", "house.shaker", "", 14000, SMP (a) FT (3, 0.05) AMP (1, 260, 0.0, 80) },
        };
        buildFamRecipes (R, sizeof (R) / sizeof (R[0]), "house", "house", out);
    }
    else if (id == "edm")
    {
        static const FamRecipe R[] = {
        { "Synths", "Sintetizador", "Supersaw", "Supersaw Festival", "edm,supersaw,festival", "lead", "", "", 9500, ANA (a, 2, 7, 30) ANA (b, 2, 5, 22) " b_coarse=12 b_level=-9 blend=0.5" FT (2, 0.1) AMP (4, 600, 0.85, 350) CHORUS ("fxa1", 0.3, 0.4, 0.5, 0.5, 1, 0.3) DELAY ("fxa2", 0.6, 0.35, 0.5, 0.6, 0.5, 0.22) REVERB ("fxa3", 0.65, 0.6, 0.45, 0.1, 1, 0.28) },
        { "Leads", "Lead", "Festival Lead", "Lead Festival Hook", "edm,lead,hook", "lead", "", "", 8500, ANA (a, 2, 5, 20) ANA (b, 3, 2, 8) " b_pwm=0.4 b_level=-8 blend=0.5" FT (2, 0.12) " f1_env=0.3" ENV2 (3, 500, 0.5, 250) AMP (3, 450, 0.85, 260) CHORUS ("fxa1", 0.3, 0.4, 0.5, 0.5, 1, 0.3) },
        { "Chords", "Acordes", "Future Chords", "Acordes Futuros Largos", "edm,futuro,acordes", "stab", "", "", 6500, ANA (a, 2, 7, 26) " chord_type=2" FT (2, 0.1) " f1_env=0.5" ENV2 (2, 400, 0.15, 220) AMP (2, 450, 0.15, 260) CHORUS ("fxa1", 0.3, 0.4, 0.5, 0.5, 1, 0.3) REVERB ("fxa3", 0.65, 0.6, 0.45, 0.1, 1, 0.3) },
        { "Bass", "Baixo", "Big Room Bass", "Baixo Big Room Sub", "edm,bigroom,baixo", "bass", "", "", 1400, ANA (a, 2, 3, 14) MONO (5) FT (2, 0.15) AMP (2, 300, 0.9, 100) SATUR ("fxa1", 0.4, 0.55, 0.05, 0.6, 0.8, 0.5) " sub_level=-8" },
        { "Bass", "Baixo", "Future Bass", "Baixo Future Wobble", "edm,future,wobble", "bass", "", "", 2500, ANA (a, 2, 5, 20) MONO (10) FT (2, 0.3) MOD (1, 1, 11, 0.6) " lfo1_rate=3 lfo1_sync=0" AMP (3, 400, 0.9, 150) CHORUS ("fxa1", 0.3, 0.4, 0.5, 0.5, 1, 0.3) },
        { "Plucks", "Pluck", "Bright Pluck", "Pluck Brilhante Futuro", "edm,pluck,brilhante", "pluck", "", "", 9000, ANA (a, 2, 3, 12) FT (2, 0.15) " f1_env=0.6" ENV2 (1, 280, 0.0, 120) AMP (1, 320, 0.0, 160) DELAY ("fxa2", 0.6, 0.4, 0.5, 0.7, 0.5, 0.3) REVERB ("fxa3", 0.65, 0.6, 0.45, 0.1, 1, 0.3) },
        { "Vocals", "Vozes", "Vocal Lead", "Voz Hook Formante", "edm,voz,hook", "lead", "edm.vox", "", 9000, SMP (a) FT (1, 0.05) AMP (3, 500, 0.8, 250) DELAY ("fxa2", 0.6, 0.35, 0.5, 0.6, 0.5, 0.25) REVERB ("fxa3", 0.6, 0.55, 0.45, 0.1, 1, 0.28) },
        { "SFX", "FX", "Impact Synth", "Impacto de Drop", "edm,impacto,drop", "fx", "edm.impact", "", 12000, SMP (a) AMP (1, 3000, 1, 400) REVERB ("fxa3", 0.8, 0.8, 0.45, 0.12, 1, 0.4) },
        { "SFX", "FX", "Risers", "Riser de Build-up", "edm,riser,buildup", "fx", "edm.riser", "", 14000, SMP (a) AMP (1, 3500, 1, 400) REVERB ("fxa3", 0.7, 0.65, 0.45, 0.1, 1, 0.3) },
        { "Drums", "Bateria", "Kit", "Kit Big Room", "edm,kit,bateria", "kit", "edm.kit", "", 14000, SMP (a) AMP (1, 800, 1, 150) " a_loop=0 a_rr=1" },
        { "Synths", "Sintetizador", "Supersaw", "Supersaw Future Largo", "edm,future,supersaw", "pad", "", "", 7500, ANA (a, 2, 7, 28) ANA (b, 2, 5, 20) " b_coarse=-12 b_level=-6 blend=0.5" FT (2, 0.08) AMP (80, 900, 0.9, 700) CHORUS ("fxa1", 0.25, 0.4, 0.5, 0.5, 1, 0.3) REVERB ("fxa3", 0.7, 0.65, 0.45, 0.1, 1, 0.3) },
        { "Leads", "Lead", "Vocal Lead", "Lead Vocal Formante Futuro", "edm,lead,formante", "lead", "", "", 5000, ANA (a, 2, 4, 14) LEGATO (30) FT (9, 0.55) MOD (1, 1, 11, 0.5) " lfo1_rate=0.6" AMP (10, 500, 0.85, 280) DELAY ("fxa2", 0.55, 0.35, 0.5, 0.5, 0.5, 0.25) REVERB ("fxa3", 0.65, 0.6, 0.45, 0.1, 1, 0.28) },
        { "Plucks", "Pluck", "Bright Pluck", "Pluck Espectral Brilhante", "edm,pluck,espectral", "pluck", "", "", 9500, SPEC (a, 0.9, 12, 0.5, 0.2, 0.0, 0.0, 0.0) FT (1, 0.05) AMP (1, 420, 0.0, 180) DELAY ("fxa2", 0.6, 0.4, 0.5, 0.7, 0.5, 0.3) REVERB ("fxa3", 0.65, 0.6, 0.45, 0.1, 1, 0.3) },
        { "Chords", "Acordes", "Future Chords", "Acordes Granulares Largos", "edm,acordes,granular", "pad", "nucleo.pad", "", 8000, GRN (a, 140, 30, 0.3, 0.3, 0.05, 0.2, 0.9) " chord_type=2" FT (1, 0.05) AMP (200, 900, 0.9, 900) REVERB ("fxa3", 0.8, 0.75, 0.45, 0.12, 1, 0.35) },
        { "Bass", "Baixo", "Future Bass", "Baixo Reese Futuro", "edm,reese,futuro", "bass", "", "", 1800, ANA (a, 2, 3, 22) ANA (b, 2, 3, 18) " b_level=-3 blend=0.5" MONO (10) FT (2, 0.15) AMP (4, 500, 0.9, 160) CHORUS ("fxa1", 0.2, 0.4, 0.5, 0.5, 1, 0.25) },
        };
        buildFamRecipes (R, sizeof (R) / sizeof (R[0]), "edm", "edm", out);
    }
    else if (id == "pop")
    {
        static const FamRecipe R[] = {
        { "Keys", "Teclas", "Radio Keys", "Teclas de Rádio Limpas", "pop,teclas,rádio", "keys", "pop.ep", "", 8000, SMP (a) FT (1, 0.05) AMP (2, 1500, 0.2, 500) CHORUS ("fxa1", 0.2, 0.3, 0.5, 0.5, 1, 0.3) REVERB ("fxa3", 0.55, 0.5, 0.5, 0.08, 0.9, 0.22) },
        { "Keys", "Teclas", "Piano Pop", "Piano Pop Compacto", "pop,piano", "keys", "pop.piano", "", 9000, SMP (a) FT (1, 0.05) AMP (1, 1200, 0.2, 400) COMP ("fxa1", 0.45, 0.5, 0.3, 0.4, 0.3, 1) REVERB ("fxa3", 0.5, 0.45, 0.5, 0.06, 0.9, 0.2) },
        { "Plucks", "Pluck", "Pop Pluck", "Pluck Pop Saltitante", "pop,pluck,saltitante", "pluck", "pop.pluck", "", 8000, SMP (a) FT (1, 0.05) AMP (1, 500, 0.0, 180) DELAY ("fxa2", 0.55, 0.35, 0.5, 0.6, 0.5, 0.22) },
        { "Leads", "Lead", "Bright Lead", "Lead Brilhante Hook", "pop,lead,brilhante", "lead", "", "", 8000, ANA (a, 2, 3, 12) ANA (b, 3, 2, 6) " b_pwm=0.4 b_level=-8 blend=0.5" FT (2, 0.12) AMP (4, 450, 0.85, 260) CHORUS ("fxa1", 0.3, 0.4, 0.5, 0.5, 1, 0.28) DELAY ("fxa2", 0.55, 0.3, 0.5, 0.4, 0.5, 0.2) },
        { "Bass", "Baixo", "Modern Bass", "Baixo Pop Moderno", "pop,baixo,moderno", "bass", "pop.bass", "", 3000, SMP (a) MONO (10) AMP (2, 500, 0.8, 130) SATUR ("fxa1", 0.2, 0.55, 0.05, 0.4, 0.85, 0.3) },
        { "Vocals", "Vozes", "Vocal Chop", "Corte de Voz Pop", "pop,voz,corte", "vocal", "pop.vox", "", 9000, SMP (a) FT (1, 0.05) AMP (1, 320, 0.0, 120) DELAY ("fxa2", 0.55, 0.35, 0.5, 0.6, 0.5, 0.22) },
        { "Chords", "Acordes", "Pop Synth Chords", "Acordes Synth Refrão", "pop,acordes,refrão", "stab", "", "", 7500, ANA (a, 2, 5, 18) " chord_type=1" FT (2, 0.1) " f1_env=0.5" ENV2 (2, 300, 0.1, 160) AMP (2, 340, 0.1, 200) CHORUS ("fxa1", 0.3, 0.4, 0.5, 0.5, 1, 0.28) REVERB ("fxa3", 0.6, 0.55, 0.45, 0.1, 1, 0.25) },
        { "Synths", "Sintetizador", "Hook Synth", "Hook Synth Contagiante", "pop,synth,hook", "lead", "", "", 6500, WTB (a, 13, 0.3, 3, 12) FT (2, 0.12) AMP (3, 400, 0.8, 240) CHORUS ("fxa1", 0.25, 0.35, 0.5, 0.5, 1, 0.3) DELAY ("fxa2", 0.55, 0.3, 0.5, 0.5, 0.5, 0.2) },
        };
        buildFamRecipes (R, sizeof (R) / sizeof (R[0]), "pop", "pop", out);
    }
    else if (id == "cinematic")
    {
        static const FamRecipe R[] = {
        { "Cinematic", "Cinemático", "Epic Brass", "Metais Épicos Trailer", "cinema,metais,épico", "stab", "", "", 2400, ANA (a, 2, 7, 20) ANA (b, 3, 4, 10) " b_level=-8 blend=0.5" FT (2, 0.1) " f1_env=0.5" ENV2 (120, 700, 0.6, 400) AMP (90, 700, 0.9, 500) REVERB ("fxa3", 0.85, 0.82, 0.45, 0.14, 1, 0.42) },
        { "Cinematic", "Cinemático", "Dark Drone", "Drone Escuro Profundo", "cinema,drone,escuro", "pad", "", "", 900, ANA (a, 2, 6, 20) ANA (b, 1, 3, 6) " b_coarse=-12 b_level=-3 blend=0.5" FT (2, 0.15) MOD (1, 1, 11, 0.3) " lfo1_rate=0.05" AMP (2500, 1000, 1.0, 3000) REVERB ("fxa3", 0.95, 0.92, 0.5, 0.18, 1, 0.55) },
        { "Cinematic", "Cinemático", "Emotional Strings", "Cordas Emotivas Sobem", "cinema,cordas,emotivas", "pad", "cinematic.strings", "", 6000, SMP (a) FT (1, 0.05) AMP (900, 900, 0.9, 1400) REVERB ("fxa3", 0.8, 0.78, 0.45, 0.12, 1, 0.4) },
        { "Pads", "Pad", "Shimmer Pad", "Pad Shimmer Celeste", "cinema,shimmer,pad", "pad", "", "", 9000, WTB (a, 10, 0.4, 4, 16) FT (1, 0.05) AMP (1200, 1000, 0.9, 2000) " fxa1_type=14 fxa1_p1=0.8 fxa1_p2=0.8 fxa1_p3=0.4 fxa1_p4=0.4 fxa1_p5=1 fxa1_mix=0.5" },
        { "Cinematic", "Cinemático", "Tension Pulse", "Pulso de Tensão", "cinema,tensão,pulso", "stab", "", "", 2500, ANA (a, 2, 3, 10) ANA (b, 2, 3, 12) " b_coarse=-12 b_level=-4 blend=0.5" FT (2, 0.2) MOD (1, 1, 29, 0.6) " lfo1_rate=2.2 lfo1_wave=4" AMP (30, 600, 0.9, 400) REVERB ("fxa3", 0.8, 0.75, 0.45, 0.12, 1, 0.35) },
        { "Cinematic", "Cinemático", "Evolving Atmosphere", "Atmosfera Granular Evolutiva", "cinema,atmosfera,granular", "texture", "cinematic.pad", "", 7000, GRN (a, 260, 20, 0.2, 0.5, 0.08, 0.15, 1.0) FT (1, 0.05) AMP (1800, 1000, 0.9, 2500) REVERB ("fxa3", 0.95, 0.92, 0.45, 0.16, 1, 0.55) },
        { "Cinematic", "Cinemático", "Soundscape", "Paisagem Espectral", "cinema,paisagem,espectral", "texture", "", "", 7500, SPEC (a, 1.4, 6, 0.4, 0.25, 0.0, 0.9, 0.5) MOD (1, 1, 7, 0.5) " lfo1_rate=0.05 lfo1_wave=1" FT (1, 0.05) AMP (2000, 1000, 0.9, 2800) REVERB ("fxa3", 0.95, 0.92, 0.45, 0.18, 1, 0.55) },
        { "Cinematic", "Cinemático", "Trailer Hit", "Impacto Trailer Épico", "cinema,impacto,trailer", "fx", "cinematic.impact", "", 10000, SMP (a) AMP (1, 3000, 1, 400) REVERB ("fxa3", 0.9, 0.9, 0.45, 0.14, 1, 0.45) },
        { "Cinematic", "Cinemático", "Epic Brass", "Metais Graves Ameaça", "cinema,metais,graves", "stab", "", "", 1800, ANA (a, 2, 6, 18) ANA (b, 3, 3, 8) " b_coarse=-12 b_level=-6 blend=0.5" FT (2, 0.1) " f1_env=0.4" ENV2 (200, 900, 0.5, 500) AMP (140, 900, 0.9, 700) REVERB ("fxa3", 0.88, 0.88, 0.45, 0.14, 1, 0.42) },
        { "Cinematic", "Cinemático", "Dark Drone", "Drone Granular Sombrio", "cinema,drone,granular", "texture", "cinematic.pad", "", 5500, GRN (a, 300, 18, 0.2, 0.4, 0.06, -0.2, 1.0) FT (1, 0.05) AMP (2200, 1000, 1.0, 3000) REVERB ("fxa3", 0.92, 0.9, 0.45, 0.14, 1, 0.45) },
        { "Cinematic", "Cinemático", "Soundscape", "Campo Espectral Aberto", "cinema,campo,espectral", "texture", "", "", 8500, SPEC (a, 0.9, 12, 0.35, 0.1, 0.0, 1.0, 0.6) MOD (1, 1, 7, 0.5) " lfo1_rate=0.04 lfo1_wave=1" FT (1, 0.05) AMP (2400, 1000, 0.9, 3000) REVERB ("fxa3", 0.92, 0.9, 0.45, 0.14, 1, 0.45) },
        };
        buildFamRecipes (R, sizeof (R) / sizeof (R[0]), "cinematic", "cinematic", out);
    }
    return out;
}
#undef FT
#undef GRN
#undef SPEC
#undef KSTR
#undef ARPON
