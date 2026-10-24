// =============================================================================
//  SYNTHKEY by Piradex — controlos ligados aos parâmetros (potenciómetros, combos, interruptores)
// =============================================================================
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <functional>
#include <memory>
#include "Look.h"
#include "../PluginProcessor.h"
#include "../AppPaths.h"
#include "ModRing.h"

namespace ui
{
using Apvts = juce::AudioProcessorValueTreeState;

inline juce::String formatValue (const sk::ParamInfo& pi, double v)
{
    const juce::String u (pi.unit);
    if (pi.kind == sk::PK_INT) return juce::String ((int) std::lround (v)) + (u.isEmpty() ? "" : " " + u);
    if (u == "%") return juce::String ((int) std::lround (v * 100.0)) + " %";
    if (u == "dB") return (v <= -59.9 ? juce::String ("-inf") : juce::String (v, 1)) + " dB";
    if (u == "Hz") return v >= 1000.0 ? juce::String (v / 1000.0, v >= 10000 ? 1 : 2) + " kHz" : juce::String (v, v >= 100 ? 0 : 1) + " Hz";
    if (u == "ms") return v >= 1000.0 ? juce::String (v / 1000.0, 2) + " s" : juce::String (v, v >= 100 ? 0 : 1) + " ms";
    if (u == "ct" || u == "st") return juce::String (v, 1) + " " + u;
    return juce::String (v, 2);
}

// -------------------------------------------------------------------------------- potenciómetro
class Knob : public juce::Slider, public juce::DragAndDropTarget
{
public:
    Knob (SynthKeyProcessor& p, int paramIdx, const juce::String& label, bool bipolarArc = false, bool fxNormalized = false)
        : info (sk::kParams[paramIdx]), caption (label), bipolar (bipolarArc), fx (fxNormalized), procRef (&p), pidx (paramIdx)
    {
        setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag); setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        setRotaryParameters (juce::MathConstants<float>::pi * 1.2f, juce::MathConstants<float>::pi * 2.8f, true);
        setPopupDisplayEnabled (false, false, nullptr); setMouseDragSensitivity (180); setScrollWheelEnabled (true); setWantsKeyboardFocus (false);
        attach = std::make_unique<juce::SliderParameterAttachment> (*p.param (paramIdx), *this, nullptr);
        textFromValueFunction = [pi = info, nf = fx] (double v) { if (nf) return juce::String ((int) std::lround (v * 100.0)) + " %"; return formatValue (pi, v); };
        setDoubleClickReturnValue (true, info.def);
    }
    // ---- largar uma origem de modulação (do painel de origens) em cima do botão liga-a automaticamente
    bool isInterestedInDragSource (const SourceDetails& d) override { return d.description.toString().startsWith (kModDragPrefix) && modDstForParam (pidx) != 0; }
    void itemDragEnter (const SourceDetails&) override { dropHover = true; repaint(); }
    void itemDragExit (const SourceDetails&) override { dropHover = false; repaint(); }
    void itemDropped (const SourceDetails& d) override
    {
        dropHover = false; repaint();
        const int src = d.description.toString().substring ((int) std::strlen (kModDragPrefix)).getIntValue();
        const auto msg = assignModulation (*procRef, src, modDstForParam (pidx)); if (msg.isNotEmpty()) say (msg, false);
    }
    void setCaption (const juce::String& c) { if (c != caption) { caption = c; repaint(); } }
    juce::String caption;
    void paint (juce::Graphics& g) override
    {
        const auto b = getLocalBounds().toFloat(); const float d = std::min (b.getWidth() - 8.0f, b.getHeight() - 22.0f);
        const auto kr = juce::Rectangle<float> (b.getCentreX() - d * 0.5f, 3.0f, d, d);
        const float prop = (float) valueToProportionOfLength (getValue());
        const float a0 = getRotaryParameters().startAngleRadians, a1 = getRotaryParameters().endAngleRadians, ang = a0 + prop * (a1 - a0);
        const auto c = theme::accent(); const float rad = d * 0.5f, cx = kr.getCentreX(), cy = kr.getCentreY();
        // corpo
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xFF2A2E48), cx, kr.getY(), juce::Colour (0xFF0D0F1D), cx, kr.getBottom(), false));
        g.fillEllipse (kr.reduced (rad * 0.22f));
        g.setColour (juce::Colour (0x33FFFFFF)); g.drawEllipse (kr.reduced (rad * 0.22f), 1.0f);
        // trilho + valor
        juce::Path track; track.addCentredArc (cx, cy, rad - 2.0f, rad - 2.0f, 0.0f, a0, a1, true);
        g.setColour (juce::Colour (0xFF1E2238)); g.strokePath (track, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        juce::Path val; const float zero = bipolar ? a0 + 0.5f * (a1 - a0) : a0;
        val.addCentredArc (cx, cy, rad - 2.0f, rad - 2.0f, 0.0f, std::min (zero, ang), std::max (zero, ang), true);
        if (std::abs (ang - zero) > 0.01f) theme::glowStroke (g, val, isEnabled() ? c : c.withAlpha (0.3f), 3.0f);
        // anel de modulação (o que a matriz faz a este parâmetro)
        { float lo = 0, hi = 0;
          if (modRangeFor (*procRef, pidx, lo, hi))
          {
              const float pl = juce::jlimit (0.0f, 1.0f, prop + lo), ph = juce::jlimit (0.0f, 1.0f, prop + hi);
              if (ph - pl > 0.004f)
              {
                  juce::Path mr; mr.addCentredArc (cx, cy, rad + 0.5f, rad + 0.5f, 0.0f, a0 + pl * (a1 - a0), a0 + ph * (a1 - a0), true);
                  g.setColour (theme::accent2().withAlpha (0.28f)); g.strokePath (mr, juce::PathStrokeType (5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
                  g.setColour (theme::accent2()); g.strokePath (mr, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
              }
          } }
        if (dropHover) { g.setColour (theme::accent2()); g.drawEllipse (kr.expanded (3.0f), 2.5f); g.setColour (theme::accent2().withAlpha (0.18f)); g.fillEllipse (kr.expanded (3.0f)); }
        // ponteiro
        juce::Path ptr; ptr.addRoundedRectangle (-1.2f, -rad * 0.62f, 2.4f, rad * 0.34f, 1.0f);
        g.setColour (juce::Colours::white); g.fillPath (ptr, juce::AffineTransform::rotation (ang).translated (cx, cy));
        // texto
        const bool showVal = isMouseOverOrDragging();
        g.setFont (theme::font (showVal ? 11.5f : 11.0f, showVal)); g.setColour (showVal ? c : theme::dim());
        g.drawFittedText (showVal ? getTextFromValue (getValue()) : caption, 0, (int) b.getHeight() - 17, (int) b.getWidth(), 15, juce::Justification::centred, 1, 0.8f);
    }
private:
    sk::ParamInfo info; bool bipolar, fx; SynthKeyProcessor* procRef; int pidx; bool dropHover = false;
    std::unique_ptr<juce::SliderParameterAttachment> attach;
};

// ------------------------------------------------------------------------------------ combo
class Combo : public juce::ComboBox
{
public:
    Combo (SynthKeyProcessor& p, int paramIdx)
    {
        auto* cp = dynamic_cast<juce::AudioParameterChoice*> (p.param (paramIdx));
        if (cp) addItemList (cp->choices, 1);
        setJustificationType (juce::Justification::centredLeft);
        attach = std::make_unique<juce::ComboBoxParameterAttachment> (*p.param (paramIdx), *this, nullptr);
    }
private:
    std::unique_ptr<juce::ComboBoxParameterAttachment> attach;
};

class Toggle : public juce::ToggleButton
{
public:
    Toggle (SynthKeyProcessor& p, int paramIdx, const juce::String& label)
    {
        setButtonText (label);
        attach = std::make_unique<juce::ButtonParameterAttachment> (*p.param (paramIdx), *this, nullptr);
    }
private:
    std::unique_ptr<juce::ButtonParameterAttachment> attach;
};

// pequena etiqueta com título (por cima de um combo)
class Caption : public juce::Component
{
public:
    Caption (const juce::String& t, juce::Justification j = juce::Justification::centredLeft) : text (t), just (j) { setInterceptsMouseClicks (false, false); }
    void paint (juce::Graphics& g) override { g.setFont (theme::font (11.0f)); g.setColour (theme::dim()); g.drawFittedText (text, getLocalBounds(), just, 1); }
    juce::String text; juce::Justification just;
};

// ------------------------------------------------------------------------------- secção (cartão)
class Section : public juce::Component
{
public:
    explicit Section (const juce::String& t) : title (t) {}
    struct Item { std::unique_ptr<juce::Component> comp; int level = 1; std::function<bool()> showIf; int w = 56, h = 76; bool breakBefore = false; bool grow = false; int yoff = 0; };

    template <typename T> T* add (T* c, int w, int h, int level = 1, std::function<bool()> showIf = {}, bool breakBefore = false)
    {
        Item it; it.comp.reset (c); it.w = w; it.h = h; it.level = level; it.showIf = std::move (showIf); it.breakBefore = breakBefore || pendingBreak; pendingBreak = false;
        addAndMakeVisible (c); items.push_back (std::move (it)); return c;
    }
    void newRow() { pendingBreak = true; }                                  // o próximo item começa nova linha
    void setYOffset (juce::Component* c, int dy) { for (auto& it : items) if (it.comp.get() == c) it.yoff = dy; }
    void setGrow (juce::Component* c) { for (auto& it : items) if (it.comp.get() == c) it.grow = true; }
    void setMode (int m) { mode = m; relayout(); }
    void setAccent (juce::Colour c, bool use) { accentOverride = c; useOverride = use; repaint(); }
    juce::String title;
    int titleH = 26, insetLeft = 0, insetRight = 0; bool dotTitle = true, bare = false;

    // devolve a "assinatura" de visibilidade para detetar mudanças que exijam novo layout
    int visibilitySignature() const
    {
        uint32_t sig = 2166136261u; uint32_t k = 0;
        for (auto& it : items) { sig = (sig ^ (isShown (it) ? 2u * k + 1u : 2u * k)) * 16777619u; ++k; }
        return (int) (sig ^ (uint32_t) mode);
    }
    void relayout()
    {
        const int pad = 8 + insetLeft, gap = 4; int x = pad, y = titleH, rowH = 0;
        for (auto& it : items)
        {
            if (! isShown (it)) { it.comp->setVisible (false); continue; }
            it.comp->setVisible (true);
            if ((it.breakBefore && x > pad) || x + it.w > getWidth() - 8 - insetRight + 1) { y += rowH + gap; x = pad; rowH = 0; }
            const int w = it.grow ? getWidth() - 8 - insetRight - x : it.w;
            it.comp->setBounds (x, y + it.yoff, w, it.h); x += w + gap; rowH = std::max (rowH, it.h);
        }
    }
    void resized() override { relayout(); }
    void paint (juce::Graphics& g) override
    {
        if (bare) return;
        auto r = getLocalBounds().toFloat();
        theme::drawCard (g, r);
        const auto c = useOverride ? accentOverride : theme::accent();
        if (title.isEmpty()) return;
        if (dotTitle) { g.setColour (c.withAlpha (0.25f)); g.fillEllipse (11.0f, 10.0f, 11.0f, 11.0f); g.setColour (c); g.fillEllipse (13.5f, 12.5f, 6.0f, 6.0f); }
        else { g.setColour (c); g.fillRoundedRectangle (10.0f, 9.0f, 3.0f, 12.0f, 1.5f); }
        g.setFont (theme::font (10.5f, true)); g.setColour (theme::text().withAlpha (0.88f)); g.drawText (title.toUpperCase(), 28, 5, getWidth() - 34, 20, juce::Justification::centredLeft);
    }
private:
    bool isShown (const Item& it) const { return it.level <= mode && (! it.showIf || it.showIf()); }
    std::vector<Item> items; int mode = 2; bool pendingBreak = false; juce::Colour accentOverride; bool useOverride = false;
};

// ------------------------------------------------------------------- gráfico de envelope ADSR
class EnvView : public juce::Component, private juce::Timer
{
public:
    EnvView (SynthKeyProcessor& p, int baseParam) : proc (p), base (baseParam) { startTimerHz (12); setInterceptsMouseClicks (false, false); }
    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced (2.0f);
        g.setColour (juce::Colour (0xFF0B0D1A)); g.fillRoundedRectangle (r, 5.0f);
        const float a = proc.getReal (base) , d = proc.getReal (base + 1), s = proc.getReal (base + 2), rl = proc.getReal (base + 3);
        auto seg = [] (float ms) { return std::pow (std::max (ms, 0.1f) / 15000.0f, 0.4f); };   // escala comprimida
        const float wa = seg (a), wd = seg (d), wr = seg (rl), ws = 0.25f, tot = wa + wd + ws + wr;
        const float W = r.getWidth() - 8.0f, H = r.getHeight() - 8.0f, x0 = r.getX() + 4.0f, y0 = r.getY() + 4.0f;
        auto X = [&] (float u) { return x0 + W * u / tot; }; auto Y = [&] (float lvl) { return y0 + H * (1.0f - lvl); };
        juce::Path p; p.startNewSubPath (X (0), Y (0)); p.lineTo (X (wa), Y (1.0f)); p.lineTo (X (wa + wd), Y (s)); p.lineTo (X (wa + wd + ws), Y (s)); p.lineTo (X (tot), Y (0));
        juce::Path fill (p); fill.lineTo (X (tot), Y (0)); fill.closeSubPath();
        g.setColour (theme::accent().withAlpha (0.12f)); g.fillPath (fill);
        theme::glowStroke (g, p, theme::accent(), 1.8f);
    }
private:
    void timerCallback() override { repaint(); }
    SynthKeyProcessor& proc; int base;
};
} // namespace ui
