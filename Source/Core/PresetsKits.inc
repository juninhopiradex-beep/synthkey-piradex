// =============================================================================
//  SYNTHKEY by Piradex — v1.4: variantes de kit (B/C), percussão, vozes e camadas.
//  Incluído dentro de PresetBuilder.h, antes dos #undef das macros.
// =============================================================================

// "Kit <Nome> B" / "C": mesmas notas do kit, mas outra variante de cada som
inline std::vector<Preset> kitVariants (const std::string& id)
{
    struct K { const char* pack; const char* style; const char* nameB; const char* nameC; const char* tagsB; const char* tagsC; const char* fx; };
    static const K ks[] = {
        { "nucleo",   "multi",    "Kit Núcleo B",   "Kit Núcleo C",   "bateria,kit,seco",    "bateria,kit,quente",   "" },
        { "trap",     "trap",     "Kit Trap B",     "Kit Trap C",     "bateria,kit,escuro",  "bateria,kit,agressivo", SATUR ("fxa1", 0.3, 0.55, 0.05, 0.5, 0.8, 0.4) },
        { "rnb",      "rnb",      "Kit R&B B",      "Kit R&B C",      "bateria,kit,suave",   "bateria,kit,vintage",  FIL1 (1, 11000, 0.05) },
        { "afrobeat", "afrobeat", "Kit Afro B",     "Kit Afro C",     "bateria,kit,leve",    "bateria,kit,seco",     COMP ("fxa1", 0.45, 0.5, 0.3, 0.4, 0.3, 1) },
        { "amapiano", "amapiano", "Kit Amapiano B", "Kit Amapiano C", "bateria,kit,profundo","bateria,kit,aberto",   SATUR ("fxa1", 0.2, 0.55, 0.05, 0.4, 0.8, 0.3) },
        { "drill",    "drill",    "Kit Drill B",    "Kit Drill C",    "bateria,kit,frio",    "bateria,kit,pesado",   SATUR ("fxa1", 0.3, 0.55, 0.05, 0.5, 0.8, 0.4) },
        { "kizomba",  "kizomba",  "Kit Kizomba B",  "Kit Kizomba C",  "bateria,kit,quente",  "bateria,kit,suave",    COMP ("fxa1", 0.45, 0.5, 0.3, 0.4, 0.3, 1) },
        { "kuduro",   "kuduro",   "Kit Kuduro B",   "Kit Kuduro C",   "bateria,kit,sujo",    "bateria,kit,explosivo", SATUR ("fxa1", 0.45, 0.55, 0.05, 0.6, 0.8, 0.5) },
    };
    std::vector<Preset> out;
    for (const auto& k : ks)
    {
        if (id != k.pack) continue;
        for (int v = 2; v <= 3; ++v)
        {
            const std::string smp = id + ".kit" + std::to_string (v);               // make() copia tudo antes de regressar
            const std::string kvText = std::string (SMP (a)) + " a_loop=0 a_rr=1 amp_a=1 amp_d=800 amp_s=1 amp_r=120" + k.fx;
            const Row r { v == 2 ? k.nameB : k.nameC, "Bateria", v == 2 ? k.tagsB : k.tagsC, k.pack, smp.c_str(), "", kvText.c_str() };
            out.push_back (make (r, k.style));
        }
    }
    return out;
}

inline std::vector<Preset> extrasMore (const std::string& id)
{
    if (id == "nucleo")
    {
        static const Row R[] = {
        { "Sino + Pad Etéreo", "Pad", "sino,pad,camada", "nucleo", "nucleo.bell", "nucleo.pad", SMP (a) SMP (b) " blend=0.45 b_level=-2" AMP (150, 900, 0.8, 1400) WIDTH ("fxa1", 0.7, 0.3, 0.5, 0.5, 0.55, 1) REVERB ("fxa3", 0.7, 0.72, 0.45, 0.12, 1, 0.35) },
        { "Piano + Cordas Suaves", "Teclas", "piano,pad,camada", "nucleo", "nucleo.ep", "nucleo.pad", SMP (a) SMP (b) " blend=0.35 b_level=-5" AMP (30, 1200, 0.6, 900) CHORUS ("fxa1", 0.25, 0.3, 0.5, 0.5, 1, 0.25) REVERB ("fxa3", 0.62, 0.6, 0.45, 0.1, 1, 0.28) },
        { "Voz + Sub", "Vozes", "voz,sub,camada", "nucleo", "nucleo.vox", "nucleo.sub", SMP (a) SMP (b) " blend=0.35 b_level=-6" AMP (60, 700, 0.85, 700) REVERB ("fxa3", 0.6, 0.6, 0.45, 0.1, 1, 0.3) },
        { "Stab + Sino", "Melodia", "stab,sino,camada", "nucleo", "nucleo.stab", "nucleo.bell", SMP (a) SMP (b) " blend=0.4 b_level=-4" AMP (1, 500, 0.0, 260) DELAY ("fxa2", 0.6, 0.35, 0.5, 0.6, 0.5, 0.22) REVERB ("fxa3", 0.55, 0.5, 0.5, 0.08, 1, 0.22) },
        };
        return ROWS (R, "multi");
    }
    if (id == "trap")
    {
        static const Row R[] = {
        { "808 + Sino", "Baixo", "808,sino,camada", "trap", "trap.808long", "trap.bell", SMP (a) SMP (b) " blend=0.2 b_level=-8" MONO (30) AMP (1, 900, 0.9, 240) SATUR ("fxa1", 0.3, 0.55, 0.05, 0.5, 0.8, 0.4) },
        { "Coro Ee Sombrio", "Vozes", "voz,coro,sombrio", "trap", "trap.voxee", "trap.pad", SMP (a) SMP (b) " blend=0.35 b_level=-5" AMP (120, 800, 0.9, 1100) WIDTH ("fxa1", 0.75, 0.3, 0.5, 0.5, 0.55, 1) REVERB ("fxa3", 0.72, 0.78, 0.5, 0.12, 1, 0.4) },
        };
        return ROWS (R, "trap");
    }
    if (id == "rnb")
    {
        static const Row R[] = {
        { "Rhodes + Cordas", "Teclas", "rhodes,cordas,camada", "rnb", "rnb.rhodes", "rnb.strings", SMP (a) SMP (b) " blend=0.4 b_level=-5" AMP (20, 1400, 0.5, 800) CHORUS ("fxa1", 0.25, 0.35, 0.5, 0.5, 1, 0.3) REVERB ("fxa3", 0.62, 0.62, 0.45, 0.1, 1, 0.3) },
        { "Coro Eh Soul", "Vozes", "voz,coro,soul", "rnb", "rnb.voxeh", "rnb.voxee", SMP (a) SMP (b) " blend=0.5 b_level=-3" AMP (90, 800, 0.9, 900) CHORUS ("fxa1", 0.25, 0.3, 0.5, 0.5, 1, 0.3) REVERB ("fxa3", 0.65, 0.68, 0.45, 0.1, 1, 0.35) },
        { "Wurli + Sino", "Teclas", "wurli,sino,camada", "rnb", "rnb.wurli", "rnb.bell", SMP (a) SMP (b) " blend=0.3 b_level=-6" AMP (1, 1300, 0.4, 600) REVERB ("fxa3", 0.58, 0.55, 0.5, 0.08, 1, 0.25) },
        };
        return ROWS (R, "rnb");
    }
    if (id == "afrobeat")
    {
        static const Row R[] = {
        { "Clave Padrão", "Percussão", "clave,padrão,groove", "afrobeat", "afrobeat.clave", "", SMP (a) AMP (1, 400, 1, 60) " arp_on=1 arp_rate=12 arp_mode=0 arp_pattern=3 arp_gate=0.4" },
        { "Timbal Padrão", "Percussão", "timbal,padrão,groove", "afrobeat", "afrobeat.timbal", "", SMP (a) AMP (1, 600, 1, 80) " arp_on=1 arp_rate=12 arp_mode=0 arp_pattern=4 arp_swing=0.15 arp_gate=0.5" },
        { "Pandeireta Padrão", "Percussão", "pandeireta,padrão,groove", "afrobeat", "afrobeat.pandeireta", "", SMP (a) AMP (1, 600, 1, 80) " arp_on=1 arp_rate=13 arp_mode=0 arp_pattern=1 arp_gate=0.5" },
        { "Coro Eh Afro", "Vozes", "voz,coro,eh", "afrobeat", "afrobeat.voxeh", "afrobeat.vox", SMP (a) SMP (b) " blend=0.4 b_level=-4" AMP (70, 700, 0.9, 700) REVERB ("fxa3", 0.62, 0.62, 0.45, 0.1, 1, 0.3) },
        { "Kalimba + Pad", "Pad", "kalimba,pad,camada", "afrobeat", "afrobeat.kalimba", "afrobeat.pad", SMP (a) SMP (b) " blend=0.4 b_level=-4" AMP (80, 900, 0.7, 1100) DELAY ("fxa2", 0.6, 0.35, 0.5, 0.6, 0.5, 0.2) REVERB ("fxa3", 0.65, 0.65, 0.45, 0.1, 1, 0.3) },
        };
        return ROWS (R, "afrobeat");
    }
    if (id == "amapiano")
    {
        static const Row R[] = {
        { "Chop Eh Delay", "Vozes", "voz,chop,eh", "amapiano", "amapiano.chopeh", "", SMP (a) AMP (1, 600, 0.8, 260) DELAY ("fxa2", 0.7, 0.5, 0.45, 0.8, 0.5, 0.4) REVERB ("fxa3", 0.6, 0.6, 0.45, 0.1, 1, 0.3) },
        { "Chop Ee Arp", "Vozes", "voz,chop,arp", "amapiano", "amapiano.chopee", "", SMP (a) AMP (1, 450, 0.0, 180) " arp_on=1 arp_rate=12 arp_mode=2 arp_pattern=4 arp_gate=0.45" DELAY ("fxa2", 0.6, 0.4, 0.5, 0.6, 0.5, 0.3) },
        { "Conga Amapiano", "Percussão", "conga,padrão,groove", "amapiano", "amapiano.conga", "", SMP (a) AMP (1, 600, 1, 80) " arp_on=1 arp_rate=12 arp_mode=0 arp_pattern=4 arp_swing=0.2 arp_gate=0.5" },
        { "Log Drum + Sub", "Baixo", "logdrum,sub,camada", "amapiano", "amapiano.logdrum", "amapiano.bass", SMP (a) SMP (b) " blend=0.3 b_level=-5" MONO (30) AMP (1, 800, 0.9, 260) SATUR ("fxa1", 0.35, 0.55, 0.05, 0.5, 0.8, 0.45) },
        };
        return ROWS (R, "amapiano");
    }
    if (id == "drill")
    {
        static const Row R[] = {
        { "Coro Ah Frio", "Vozes", "voz,coro,frio", "drill", "drill.voxah", "drill.pad", SMP (a) SMP (b) " blend=0.35 b_level=-5" AMP (110, 800, 0.9, 1100) WIDTH ("fxa1", 0.7, 0.3, 0.5, 0.5, 0.55, 1) REVERB ("fxa3", 0.72, 0.76, 0.5, 0.12, 1, 0.4) },
        { "808 + Bell", "Baixo", "808,sino,camada", "drill", "drill.808slide", "drill.bell", SMP (a) SMP (b) " blend=0.2 b_level=-8" MONO (120) AMP (1, 900, 0.9, 260) SATUR ("fxa1", 0.35, 0.55, 0.05, 0.5, 0.8, 0.45) },
        };
        return ROWS (R, "drill");
    }
    if (id == "kizomba")
    {
        static const Row R[] = {
        { "Voz Oh Zouk", "Vozes", "voz,oh,zouk", "kizomba", "kizomba.voxoh", "kizomba.strings", SMP (a) SMP (b) " blend=0.4 b_level=-4" AMP (80, 800, 0.9, 900) CHORUS ("fxa1", 0.25, 0.3, 0.5, 0.5, 1, 0.3) REVERB ("fxa3", 0.62, 0.62, 0.45, 0.1, 1, 0.3) },
        { "Clave Kizomba", "Percussão", "clave,padrão,groove", "kizomba", "kizomba.clave", "", SMP (a) AMP (1, 400, 1, 60) " arp_on=1 arp_rate=12 arp_mode=0 arp_pattern=3 arp_gate=0.4" },
        { "Piano + Cordas Zouk", "Teclas", "piano,cordas,camada", "kizomba", "kizomba.ep", "kizomba.strings", SMP (a) SMP (b) " blend=0.4 b_level=-5" AMP (40, 1200, 0.6, 900) CHORUS ("fxa1", 0.25, 0.3, 0.5, 0.5, 1, 0.3) REVERB ("fxa3", 0.62, 0.62, 0.45, 0.1, 1, 0.3) },
        };
        return ROWS (R, "kizomba");
    }
    if (id == "kuduro")
    {
        static const Row R[] = {
        { "Grito Ey", "Vozes", "voz,ey,grito", "kuduro", "kuduro.shout2", "", SMP (a) AMP (1, 450, 0.5, 180) REVERB ("fxa3", 0.5, 0.45, 0.5, 0.05, 0.9, 0.18) },
        { "Conga Kuduro", "Percussão", "conga,padrão,groove", "kuduro", "kuduro.conga", "", SMP (a) AMP (1, 600, 1, 80) " arp_on=1 arp_rate=13 arp_mode=0 arp_pattern=3 arp_gate=0.5" },
        { "Stab + Sirene", "Melodia", "stab,sirene,camada", "kuduro", "kuduro.stab", "kuduro.siren", SMP (a) SMP (b) " blend=0.3 b_level=-7" AMP (1, 500, 0.2, 200) SATUR ("fxa1", 0.35, 0.65, 0, 0.5, 0.8, 0.45) },
        };
        return ROWS (R, "kuduro");
    }
    return {};
}
