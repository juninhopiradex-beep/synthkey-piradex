// =============================================================================
//  SYNTHKEY by Piradex — painel de origens de modulação (arrasta uma origem para um botão)
// =============================================================================
#pragma once
#include "Controls.h"

namespace ui
{
using namespace sk;

class ModSourceStrip : public juce::Component, public juce::SettableTooltipClient
{
public:
    struct Chip { int src; const char* label; };
    ModSourceStrip()
    {
        static const Chip list[] = { { MS_LFO1, "LFO 1" }, { MS_LFO2, "LFO 2" }, { MS_ENV2, "ENV 2" }, { MS_ENV3, "ENV 3" }, { MS_STEP, "STEP" },
                                     { MS_DRAW, "DESEN." }, { MS_MENV, "MULTI" }, { MS_VEL, "VELOC." }, { MS_KEY, "TECLA" }, { MS_MODWHEEL, "MOD" },
                                     { MS_AFTERTOUCH, "AFTER" }, { MS_PRESSURE, "PRESS." }, { MS_SLIDE, "SLIDE" }, { MS_RANDOM, "ALEAT." }, { MS_PITCHBEND, "BEND" } };
        for (auto& c : list) chips.push_back (c);
        setTooltip (TX ("Arrasta uma origem para um bot\xC3\xA3o para o modular"));
    }
    void paint (juce::Graphics& g) override
    {
        g.setColour (theme::dim()); g.setFont (theme::font (9.5f, true));
        g.drawText (TX ("ORIGENS  \xC2\xB7  arrasta para um bot\xC3\xA3o"), 0, 0, getWidth(), 12, juce::Justification::centredLeft);
        for (size_t i = 0; i < chips.size(); ++i)
        {
            const auto r = rectFor ((int) i).toFloat(); const bool on = (int) i == hot;
            g.setColour (on ? theme::accent2().withAlpha (0.35f) : juce::Colour (0xFF151830)); g.fillRoundedRectangle (r, 5.0f);
            g.setColour (on ? theme::accent2() : theme::stroke()); g.drawRoundedRectangle (r.reduced (0.5f), 5.0f, 1.0f);
            g.setColour (on ? theme::text() : theme::dim().brighter (0.3f)); g.setFont (theme::font (10.5f, true));
            g.drawText (TX (chips[i].label), r.toNearestInt(), juce::Justification::centred);
        }
    }
    void mouseMove (const juce::MouseEvent& e) override { const int h = indexAt (e.getPosition()); if (h != hot) { hot = h; repaint(); } }
    void mouseExit (const juce::MouseEvent&) override { hot = -1; repaint(); }
    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (e.getDistanceFromDragStart() < 6) return;
        auto* container = juce::DragAndDropContainer::findParentDragContainerFor (this); if (! container || container->isDragAndDropActive()) return;
        const int i = indexAt (e.getMouseDownPosition()); if (i < 0) return;
        juce::Image img (juce::Image::ARGB, 64, 24, true); { juce::Graphics g (img); g.setColour (theme::accent2().withAlpha (0.85f)); g.fillRoundedRectangle (0, 0, 64, 24, 6.0f);
            g.setColour (juce::Colours::white); g.setFont (theme::font (12.0f, true)); g.drawText (TX (chips[(size_t) i].label), 0, 0, 64, 24, juce::Justification::centred); }
        container->startDragging (juce::String (kModDragPrefix) + juce::String (chips[(size_t) i].src), this, juce::ScaledImage (img), false);
    }
private:
    juce::Rectangle<int> rectFor (int i) const { const int cols = 5, w = (getWidth() - 4 * 4) / cols, h = 18; return { (i % cols) * (w + 4), 15 + (i / cols) * (h + 4), w, h }; }
    int indexAt (juce::Point<int> p) const { for (size_t i = 0; i < chips.size(); ++i) if (rectFor ((int) i).contains (p)) return (int) i; return -1; }
    std::vector<Chip> chips; int hot = -1;
};
}
