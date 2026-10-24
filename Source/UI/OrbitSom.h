// =============================================================================
//  SYNTHKEY by Piradex — ecrã SOM no formato "Orbit" (uma só vista: origens | som | matriz)
//  Coordenadas = as da janela (1180 × 760); a página ocupa x 0..1180, y 0..640.
// =============================================================================
#pragma once
#include "OrbitPanels.h"
#include "ImportUI.h"

namespace ui
{
class OrbitSomPage : public Page
{
public:
    std::function<void (int)> gotoTab;           // 0 SOM, 1 MATRIZ, 2 MODULAD., 3 EFEITOS, 4 ARP …

    explicit OrbitSomPage (SynthKeyProcessor& p) : Page (p), rail (p), matrix (p), orb (p), curve (p), fx (p)
    {
        const juce::Colour mint (0xFF3DE0C0), pink (0xFFFF5FD2), vio (0xFF8B7BFF);
        auto card = [this] (juce::Rectangle<int> r, const juce::String& t, juce::Colour dot = {}) { auto* c = cards.add (new CardBg (t)); c->dot = dot; addAndMakeVisible (c); c->setBounds (r); return c; };
        card ({ 252, 58, 660, 238 }, {});
        card ({ 252, 306, 660, 164 }, "FILTROS", theme::accent2());
        card ({ 664, 480, 248, 92 }, "EFEITOS", vio);
        matCard = card ({ 928, 58, 236, 372 }, "MATRIZ", pink);
        card ({ 928, 438, 236, 134 }, "MACROS", vio);
        card ({ 928, 582, 236, 58 }, TX ("SA\xC3\x8D" "DA"), mint);

        addAndMakeVisible (rail); rail.setBounds (16, 58, 220, 570);

        // ---- oscilador (separadores A | B | SUB + RUÍDO | VOZES)
        addAndMakeVisible (tabs); tabs.setBounds (262, 62, 400, 30); tabs.onPick = [this] (int i) { setOscTab (i); };
        addAndMakeVisible (orb); orb.setBounds (264, 98, 340, 142);
        buildEngine (0); buildEngine (1); buildSubNoise(); buildVoices();
        // ---- filtros
        buildFilters();
        // ---- envelope de volume
        {
            auto* s = addSection ("VOLUME", { 252, 480, 404, 92 }); s->titleH = 22; s->setAccent (mint, true);
            s->add (new EnvView (proc, P_AMP_A), 150, 60, 1);
            K (s, P_AMP_A, "ATAQUE", 1, {}, 50, 64); K (s, P_AMP_D, "DECAIM.", 1, {}, 50, 64); K (s, P_AMP_S, "SUSTAIN", 1, {}, 50, 64); K (s, P_AMP_R, "LIBERT.", 1, {}, 50, 64);
        }
        // ---- arpejador em miniatura
        {
            auto* s = addSection (TX ("ARPEJADOR  \xC2\xB7  PASSOS"), { 252, 582, 660, 58 }); s->titleH = 22; s->setAccent (juce::Colour (0xFFFFB547), true);
            T (s, P_ARP_ON, "LIGADO", 84, 1, {}, -1); C (s, P_ARP_PATTERN, 176, 1, {}, -1);
            arp = s->add (new ArpMini (proc), 200, 30, 1); s->setGrow (arp); arp->onClick = [this] { if (gotoTab) gotoTab (4); };
        }
        addAndMakeVisible (fx); fx.setBounds (672, 506, 232, 56); fx.onClick = [this] { if (gotoTab) gotoTab (3); };
        addAndMakeVisible (matrix); matrix.setBounds (932, 88, 228, 336);
        setOscTab (0); setFilterTab (0); setMode (2);
    }

    void setMode (int m) override { Page::setMode (m); refreshFilterVis(); }

    void tick() override
    {
        Page::tick();
        orb.update(); curve.update(); rail.update(); matrix.update(); fx.update(); arp->update();
        matCard->setRight (juce::String (matrix.activeCount()) + TX (" / 16 liga\xC3\xA7\xC3\xB5" "es"));
        const auto ty = [this] (int idx) { return juce::StringArray::fromTokens (TX (kParams[P_A_TYPE].choices), "|", "")[juce::jlimit (0, 2, (int) proc.getReal (idx))]; };
        const juce::String la = TX ("A \xC2\xB7 ") + ty (P_A_TYPE), lb = TX ("B \xC2\xB7 ") + ty (P_B_TYPE);
        if (la != lastA || lb != lastB) { lastA = la; lastB = lb; tabs.setLabels ({ la, lb, TX ("SUB + RU\xC3\x8D" "DO"), "VOZES" }, { 120, 120, 104, 62 }); }
        refreshFilterVis();
    }

    void paintOverChildren (juce::Graphics&) override {}

private:
    Section* bare (juce::Rectangle<int> r) { auto* s = addSection ({}, r); s->bare = true; s->titleH = 0; return s; }
    void group (int g, juce::Component* c) { groups[g].add (c); }

    void buildEngine (int E)
    {
        const int o = E ? (P_B_ON - P_A_ON) : 0;
        auto type = [this, o] { return val (P_A_TYPE + o); };
        auto* c = bare ({ 258, 244, 350, 36 });
        C (c, P_A_TYPE + o, 100);
        c->add (new SampleChooser (proc, E), 190, 28, 1, [=] { return type() == 2; });
        C (c, P_A_WAVE + o, 110, 1, [=] { return type() == 0; });
        C (c, P_A_WTTAB + o, 136, 1, [=] { return type() == 1; });
        c->add (new WavetableImportButton (proc, E), 44, 28, 1, [=] { return type() == 1; });
        T (c, P_A_RETRIG + o, "RETRIG.", 88, 3, [=] { return type() != 2; });
        group (E, c);
        auto* t = bare ({ 806, 60, 102, 32 }); T (t, P_A_ON + o, "ATIVO", 74); group (E, t);
        auto* s = bare ({ 612, 94, 296, 200 }); group (E, s);
        auto KK = [&] (int idx, const char* label, int lvl, std::function<bool()> sh = {}) { return K (s, idx, TX (label), lvl, std::move (sh), 52, 64); };
        KK (P_A_LEVEL + o, "N\xC3\x8DVEL", 1); KK (P_A_PAN + o, "PAN", 2); KK (P_A_COARSE + o, "SEMITONS", 1); KK (P_A_FINE + o, "FINO", 2);
        KK (P_A_UNI + o, "UNISSON", 1); KK (P_A_UNIDET + o, "DETUNE", 1, [=] { return val (P_A_UNI + o) > 1; }); KK (P_A_UNISPR + o, "ESPALHAR", 2, [=] { return val (P_A_UNI + o) > 1; });
        KK (P_A_PWM + o, "PWM", 1, [=] { return type() == 0 && val (P_A_WAVE + o) == 3; });
        KK (P_A_SYNC + o, "SYNC", 2, [=] { return type() == 0; }); KK (P_A_FMAMT + o, "FM", 2, [=] { return type() == 0; });
        C (s, P_A_FMRAT + o, 52, 3, [=] { return type() == 0; }, 20);
        KK (P_A_FOLD + o, "DOBRA", 2, [=] { return type() != 2; });
        KK (P_A_WTPOS + o, "POSI\xC3\x87\xC3\x83O", 1, [=] { return type() == 1; }); KK (P_A_WARP + o, "WARP", 2, [=] { return type() == 1; });
        KK (P_A_START + o, "IN\xC3\x8DICIO", 2, [=] { return type() == 2; });
        T (s, P_A_LOOP + o, "LOOP", 62, 2, [=] { return type() == 2; }, 18); T (s, P_A_REV + o, "REV", 60, 2, [=] { return type() == 2; }, 18);
        T (s, P_A_RR + o, "RR", 56, 3, [=] { return type() == 2; }, 18); T (s, P_A_TRACK + o, "TECLA", 64, 3, [=] { return type() == 2; }, 18);
        KK (P_A_PHASE + o, "FASE", 3, [=] { return type() != 2; });
    }
    void buildSubNoise()
    {
        auto* c = bare ({ 258, 244, 350, 36 }); C (c, P_SUB_WAVE, 100, 2); C (c, P_SUB_OCT, 80, 2); group (2, c);
        auto* s = bare ({ 612, 94, 296, 200 }); group (2, s);
        K (s, P_SUB_LEVEL, "SUB", 1, {}, 52, 64); K (s, P_NOISE_LEVEL, TX ("RU\xC3\x8D" "DO"), 1, {}, 52, 64); K (s, P_NOISE_COLOR, "COR", 2, {}, 52, 64);
    }
    void buildVoices()
    {
        auto* c = bare ({ 258, 244, 350, 36 }); C (c, P_VOICE_MODE, 124); C (c, P_GLIDE_MODE, 100, 2); group (3, c);
        auto* s = bare ({ 612, 94, 296, 200 }); group (3, s);
        K (s, P_POLYPHONY, "POLIFONIA", 1, {}, 52, 64); K (s, P_GLIDE, "GLIDE", 1, {}, 52, 64); K (s, P_TRANSPOSE, "TRANSP.", 1, {}, 52, 64); K (s, P_BLEND, TX ("A \xE2\x97\x82\xE2\x96\xB8 B"), 1, {}, 52, 64);
        K (s, P_PB_RANGE, "PITCH BEND", 2, {}, 52, 64); K (s, P_TUNE, TX ("AFINA\xC3\x87\xC3\x83O"), 2, {}, 52, 64); K (s, P_VEL_SENS, "VELOC.", 2, {}, 52, 64);
        s->newRow(); C (s, P_OUT_BUS, 140, 3);
    }
    void buildFilters()
    {
        addAndMakeVisible (ftabs); ftabs.setBounds (338, 310, 110, 28); ftabs.setLabels ({ "F1", "F2" }, { 50, 50 }); ftabs.onPick = [this] (int i) { setFilterTab (i); };
        f1Type = std::make_unique<Combo> (proc, P_F1_TYPE); f2Type = std::make_unique<Combo> (proc, P_F2_TYPE); route = std::make_unique<Combo> (proc, P_FILT_ROUTE);
        for (auto* c : { f1Type.get(), f2Type.get(), route.get() }) addAndMakeVisible (c);
        f1Type->setBounds (458, 311, 150, 26); f2Type->setBounds (458, 311, 150, 26); route->setBounds (618, 311, 118, 26);
        addAndMakeVisible (curve); curve.setBare (true); curve.setBounds (262, 344, 394, 120);
        static const int base[2] = { P_F1_TYPE, P_F2_TYPE };
        for (int f = 0; f < 2; ++f)
        {
            auto* s = bare ({ 664, 342, 244, 128 }); fgroups[f] = s;
            auto on = [this, f] { return val (base[f]) != 0; };
            K (s, base[f] + 1, "CORTE", 1, on, 52, 62); K (s, base[f] + 2, "RESS.", 1, on, 52, 62); K (s, base[f] + 3, "DRIVE", 2, on, 52, 62); K (s, base[f] + 4, "TECLA", 2, on, 52, 62);
            K (s, base[f] + 5, f ? "ENV 3" : "ENV 2", 1, on, 52, 62);
            if (f) K (s, P_FILT_BAL, TX ("BALAN\xC3\x87O"), 2, on, 52, 62);
        }
    }
    void setOscTab (int t)
    {
        oscTab = t; tabs.setSelected (t);
        for (int g = 0; g < 4; ++g) for (auto* c : groups[g]) c->setVisible (g == t);
        orb.setMode (t < 2 ? t : 2);
    }
    void setFilterTab (int t) { fTab = t; ftabs.setSelected (t); refreshFilterVis(); }
    void refreshFilterVis()
    {
        if (! f1Type) return;
        f1Type->setVisible (fTab == 0); f2Type->setVisible (fTab == 1); route->setVisible (mode >= 2 && val (P_F2_TYPE) != 0);
        for (int f = 0; f < 2; ++f) fgroups[f]->setVisible (f == fTab);
    }

    juce::OwnedArray<CardBg> cards; CardBg* matCard = nullptr;
    OrbitSourceRail rail; MatrixMini matrix; OrbDisplay orb; FilterCurveView curve; FxSummary fx; ArpMini* arp = nullptr;
    PillTabs tabs, ftabs; juce::Array<juce::Component*> groups[4]; Section* fgroups[2] = { nullptr, nullptr };
    std::unique_ptr<Combo> f1Type, f2Type, route; int oscTab = 0, fTab = 0; juce::String lastA, lastB;
};
} // namespace ui
