// =============================================================================
//  SYNTHKEY by Piradex — presets de módulo (v1.5): só oscilador, só filtro, só envelopes, só modulação,
//  só efeitos, só arpejador. Carregar um destes NÃO altera nenhum parâmetro fora do módulo (ver moduleOwns).
//  Incluído dentro de PresetBuilder.h, antes dos #undef das macros. Não levam macros nem reverb de Aux por defeito.
// =============================================================================
struct ModRow { const char* module; const char* sub; const char* name; const char* tags; const char* kv; };

inline std::vector<Preset> modulePresets()
{
    static const ModRow M[] = {
    // ---------------- osciladores
    { "osc", "Analógico", "Osc · Serra Unisson 7", "osc,serra,unisson", ANA (a, 2, 7, 24) },
    { "osc", "Analógico", "Osc · Pulso PWM", "osc,pulso,pwm", ANA (a, 3, 3, 10) " a_pwm=0.35" },
    { "osc", "Analógico", "Osc · Seno Puro", "osc,seno", ANA (a, 0, 1, 0) },
    { "osc", "Analógico", "Osc · Triângulo Suave", "osc,triângulo", ANA (a, 1, 2, 6) },
    { "osc", "FM", "Osc · FM 2:1", "osc,fm", ANA (a, 0, 1, 0) " a_fmamt=0.4 a_fmrat=2" },
    { "osc", "Sync", "Osc · Sync Aberto", "osc,sync", ANA (a, 2, 1, 0) " a_sync=12" },
    { "osc", "Wavetable", "Osc · Vogais", "osc,wavetable,vogais", WTB (a, 3, 0.5, 3, 14) },
    { "osc", "Wavetable", "Osc · Pads Etéreos", "osc,wavetable,pad", WTB (a, 10, 0.4, 4, 18) },
    { "osc", "Duplo", "Osc · Serra + Serra Oitava", "osc,duplo,oitava", ANA (a, 2, 3, 10) ANA (b, 2, 3, 10) " b_coarse=12 b_level=-6 blend=0.5" },
    { "osc", "Camadas", "Osc · Sub e Ruído", "osc,sub,ruído", ANA (a, 2, 2, 8) " sub_level=-8 sub_wave=0 noise_level=-30 noise_color=0.5" },
    // ---------------- filtros
    { "filter", "Passa-baixo", "Filtro · LP24 Quente", "filtro,lp24", FIL1 (2, 1800, 0.12) " f1_env=0.2" },
    { "filter", "Ladder", "Filtro · Ladder Ácido", "filtro,ladder,acid", FIL1E (7, 700, 0.6, 0.7) },
    { "filter", "Passa-alto", "Filtro · HP Aberto", "filtro,hp", FIL1 (3, 300, 0.1) },
    { "filter", "Passa-banda", "Filtro · BP Vocal", "filtro,bp,vocal", FIL1 (5, 1200, 0.4) },
    { "filter", "Formante", "Filtro · Formante", "filtro,formante", FIL1 (9, 1500, 0.5) },
    { "filter", "Pente", "Filtro · Pente Metálico", "filtro,pente", FIL1 (8, 1500, 0.7) },
    { "filter", "Notch", "Filtro · Notch Faser", "filtro,notch", FIL1 (6, 1200, 0.3) },
    { "filter", "Duplo", "Filtro · LP + HP em Série", "filtro,duplo,série", FIL1 (2, 5000, 0.1) FIL2 (3, 150, 0.1) " filt_route=0" },
    // ---------------- envelopes
    { "env", "Pluck", "Env · Pluck", "env,pluck", AMP (1, 300, 0, 120) ENV2 (1, 250, 0, 100) },
    { "env", "Pad", "Env · Pad Lento", "env,pad", AMP (700, 800, 0.9, 1200) ENV2 (900, 900, 0.8, 800) },
    { "env", "Stab", "Env · Stab", "env,stab", AMP (2, 200, 0, 100) ENV2 (1, 180, 0, 80) },
    { "env", "Órgão", "Env · Órgão", "env,órgão", AMP (2, 50, 1, 60) },
    { "env", "Swell", "Env · Swell", "env,swell", AMP (1500, 300, 1, 1500) ENV2 (1500, 300, 1, 800) },
    { "env", "Percussivo", "Env · Percussivo", "env,percussivo", AMP (1, 150, 0, 50) ENV2 (1, 120, 0, 50) },
    { "env", "Piano", "Env · Piano", "env,piano", AMP (3, 1800, 0, 300) ENV2 (2, 900, 0, 200) },
    { "env", "Reverso", "Env · Reverso", "env,reverso", AMP (2000, 100, 1, 50) },
    // ---------------- modulação
    { "mod", "Vibrato", "Mod · Vibrato", "mod,vibrato", MOD (1, 1, 3, 0.03) " lfo1_rate=5.5 lfo1_fade=500" },
    { "mod", "Wobble", "Mod · Wobble de Filtro", "mod,wobble", MOD (1, 1, 1, 0.6) " lfo1_rate=2" },
    { "mod", "Wavetable", "Mod · Movimento de Wavetable", "mod,wavetable", MOD (1, 1, 7, 0.3) " lfo1_rate=0.2 lfo1_wave=1" },
    { "mod", "PWM", "Mod · PWM Lento", "mod,pwm", MOD (1, 1, 19, 0.3) " lfo1_rate=0.6" },
    { "mod", "Envelope", "Mod · Envelope no Filtro", "mod,envelope", MOD (1, 3, 1, 0.5) },
    // ---------------- efeitos (cadeia A completa; slots não usados ficam vazios)
    { "fx", "Reverb", "FX · Hall Grande", "fx,reverb,hall", " fxa1_type=0 fxa2_type=0" REVERB ("fxa3", 0.8, 0.8, 0.45, 0.1, 1, 0.4) },
    { "fx", "Chorus", "FX · Chorus Largo", "fx,chorus", " fxa2_type=0 fxa3_type=0" CHORUS ("fxa1", 0.3, 0.45, 0.5, 0.5, 1, 0.4) },
    { "fx", "Delay", "FX · Delay Estéreo", "fx,delay", " fxa1_type=0 fxa3_type=0" DELAY ("fxa2", 0.6, 0.4, 0.5, 0.6, 0.5, 0.3) },
    { "fx", "Flanger", "FX · Flanger Clássico", "fx,flanger", " fxa2_type=0 fxa3_type=0" FX ("fxa1", 12, 0.3, 0.6, 0.7, 0.3, 0.6, 0.5) },
    { "fx", "Ensemble", "FX · Ensemble de Cordas", "fx,ensemble", " fxa2_type=0 fxa3_type=0" FX ("fxa1", 13, 0.3, 0.5, 0.5, 0.7, 0.8, 0.6) },
    { "fx", "Shimmer", "FX · Shimmer Etéreo", "fx,shimmer,reverb", " fxa1_type=0 fxa2_type=0" FX ("fxa3", 14, 0.6, 0.7, 0.4, 0.5, 1, 0.4) },
    { "fx", "Multi-Tap", "FX · Multi-Tap Ritmado", "fx,multitap,delay", " fxa1_type=0 fxa3_type=0" FX ("fxa2", 15, 0.5, 0.6, 0.4, 0.6, 0.7, 0.35) },
    { "fx", "Pitch", "FX · Oitava Acima", "fx,pitch,oitava", " fxa2_type=0 fxa3_type=0" FX ("fxa1", 16, 1.0, 0.5, 0.4, 0.0, 0.5, 0.4) },
    { "fx", "Pitch", "FX · Oitava Abaixo", "fx,pitch,oitava", " fxa2_type=0 fxa3_type=0" FX ("fxa1", 16, 0.0, 0.5, 0.4, 0.0, 0.5, 0.4) },
    { "fx", "Saturação", "FX · Saturação Quente", "fx,saturação", " fxa2_type=0 fxa3_type=0" SATUR ("fxa1", 0.35, 0.55, 0.05, 0.4, 0.8, 0.5) },
    // ---------------- arpejador
    { "arp", "Euclidiano", "Arp · Euclidiano 5/16", "arp,euclidiano", " arp_on=1 arp_rate=12 arp_mode=0 arp_gate=0.5 arp_eucl_k=5 arp_eucl_rot=0 arp_prob=1" },
    { "arp", "Euclidiano", "Arp · Euclidiano 7/16 Rodado", "arp,euclidiano", " arp_on=1 arp_rate=12 arp_mode=2 arp_gate=0.45 arp_eucl_k=7 arp_eucl_rot=2 arp_prob=1" },
    { "arp", "Probabilidade", "Arp · Probabilístico 70%", "arp,probabilidade", " arp_on=1 arp_rate=12 arp_mode=4 arp_gate=0.5 arp_eucl_k=0 arp_prob=0.7 arp_seed=42" },
    { "arp", "Clássico", "Arp · Cima-Baixo 1/16", "arp,clássico", " arp_on=1 arp_rate=12 arp_mode=2 arp_gate=0.6 arp_eucl_k=0 arp_prob=1" },
    { "arp", "Acorde", "Arp · Acorde 1/8", "arp,acorde", " arp_on=1 arp_rate=10 arp_mode=6 arp_gate=0.4 arp_eucl_k=0 arp_prob=1" },
    };
    std::vector<Preset> out;
    for (const auto& r : M)
    {
        Preset p; p.name = r.name; p.style = "multi"; p.pack = "nucleo"; p.module = r.module; p.subfamily = r.sub; p.family = "Modules"; p.qa = "auto"; p.role = "module";
        p.category = std::string ("Módulo ") + r.module;
        std::string t (r.tags); size_t a = 0; while (a < t.size()) { size_t b = t.find (',', a); if (b == std::string::npos) b = t.size(); if (b > a) p.tags.push_back (t.substr (a, b - a)); a = b + 1; }
        std::string err; applyKv (p, r.kv, &err);
        for (const auto& kv : p.values) if (! moduleOwnsIndex (r.module, kv.first)) err += std::string ("fora do módulo: ") + kParams[kv.first].id + "; ";
        if (! err.empty()) lintErrors().push_back (std::string (r.name) + ": " + err);
        out.push_back (std::move (p));
    }
    return out;
}
