// =============================================================================
//  SYNTHKEY by Piradex — tema visual: "Liquid Chrome" com 5 paletas neon
// =============================================================================
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

inline juce::String TX (const char* utf8) { return juce::String::fromUTF8 (utf8); }

namespace theme
{
struct Palette { const char* name; juce::uint32 accent, accent2; };
inline const Palette kPalettes[6] = {
    { "ORBIT",   0xFFA58BFF, 0xFF5FE0FF },
    { "CIANO",   0xFF3DF2FF, 0xFF7B5CFF },
    { "MAGENTA", 0xFFFF4FD8, 0xFF8A5CFF },
    { "\xC3\x82MBAR", 0xFFFFB547, 0xFFFF5E7A },
    { "LIMA",    0xFFB6FF4D, 0xFF22E3A0 },
    { "VIOLETA", 0xFFA678FF, 0xFF4DB2FF } };

inline int& paletteIndex() { static int i = 0; return i; }
inline juce::Colour accent()  { return juce::Colour (kPalettes[paletteIndex()].accent); }
inline juce::Colour accent2() { return juce::Colour (kPalettes[paletteIndex()].accent2); }
inline juce::Colour bg()      { return juce::Colour (0xFF0A0A10); }
inline juce::Colour panel()   { return juce::Colour (0xF2121219); }
inline juce::Colour panelHi() { return juce::Colour (0xF21A1A26); }
inline juce::Colour stroke()  { return juce::Colour (0x24FFFFFF); }
inline juce::Colour text()    { return juce::Colour (0xFFE8ECF8); }
inline juce::Colour dim()     { return juce::Colour (0xFF8A91AB); }
inline juce::Colour warn()    { return juce::Colour (0xFFFFB547); }
inline juce::Colour bad()     { return juce::Colour (0xFFFF5470); }
inline juce::Colour good()    { return juce::Colour (0xFF3DF2A0); }

inline juce::Font font (float h, bool bold = false) { return juce::Font (juce::FontOptions (h, bold ? juce::Font::bold : juce::Font::plain)); }

// cartão de vidro
inline void drawCard (juce::Graphics& g, juce::Rectangle<float> r, float radius = 10.0f, bool hi = false)
{
    g.setColour (hi ? panelHi() : panel()); g.fillRoundedRectangle (r, radius);
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.07f), r.getX(), r.getY(), juce::Colours::transparentWhite, r.getX(), r.getY() + 40.0f, false));
    g.fillRoundedRectangle (r.withHeight (40.0f), radius);
    g.setColour (stroke()); g.drawRoundedRectangle (r.reduced (0.5f), radius, 1.0f);
}
// brilho neon suave por baixo de um arco/linha
inline void glowStroke (juce::Graphics& g, const juce::Path& p, juce::Colour c, float w)
{
    g.setColour (c.withAlpha (0.12f)); g.strokePath (p, juce::PathStrokeType (w + 6.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (c.withAlpha (0.28f)); g.strokePath (p, juce::PathStrokeType (w + 3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (c);                   g.strokePath (p, juce::PathStrokeType (w, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}
}

class SynthLook : public juce::LookAndFeel_V4
{
public:
    SynthLook()
    {
        using namespace theme;
        setColour (juce::ResizableWindow::backgroundColourId, bg());
        setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xF40C0E18));
        setColour (juce::PopupMenu::textColourId, text());
        setColour (juce::PopupMenu::highlightedBackgroundColourId, juce::Colour (0xFF22263B));
        setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
        setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xFF121528));
        setColour (juce::ComboBox::textColourId, text());
        setColour (juce::ComboBox::outlineColourId, stroke());
        setColour (juce::ComboBox::arrowColourId, dim());
        setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xFF0C0E1C));
        setColour (juce::TextEditor::textColourId, text());
        setColour (juce::TextEditor::outlineColourId, stroke());
        setColour (juce::TextEditor::focusedOutlineColourId, juce::Colour (0xFF3DF2FF));
        setColour (juce::TextEditor::highlightColourId, juce::Colour (0x553DF2FF));
        setColour (juce::CaretComponent::caretColourId, juce::Colour (0xFF3DF2FF));
        setColour (juce::Label::textColourId, text());
        setColour (juce::ListBox::backgroundColourId, juce::Colours::transparentBlack);
        setColour (juce::ScrollBar::thumbColourId, juce::Colour (0x66FFFFFF));
        setColour (juce::AlertWindow::backgroundColourId, juce::Colour (0xFF10121E));
        setColour (juce::AlertWindow::textColourId, text());
        setColour (juce::AlertWindow::outlineColourId, stroke());
        setColour (juce::TextButton::buttonColourId, juce::Colour (0xFF1A1D30));
        setColour (juce::TextButton::textColourOffId, text());
        setColour (juce::TextButton::textColourOnId, text());
    }
    juce::Font getComboBoxFont (juce::ComboBox&) override { return theme::font (13.0f); }
    juce::Font getPopupMenuFont() override { return theme::font (14.0f); }
    juce::Font getTextButtonFont (juce::TextButton&, int) override { return theme::font (13.0f, true); }
    juce::Font getLabelFont (juce::Label&) override { return theme::font (13.0f); }
    juce::Font getAlertWindowMessageFont() override { return theme::font (14.0f); }
    juce::Font getAlertWindowTitleFont() override { return theme::font (17.0f, true); }

    void drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box) override
    {
        auto r = juce::Rectangle<float> (0, 0, (float) w, (float) h).reduced (0.5f);
        g.setColour (juce::Colour (0xFF121528)); g.fillRoundedRectangle (r, 6.0f);
        g.setColour (box.hasKeyboardFocus (true) || box.isMouseOver() ? theme::accent().withAlpha (0.8f) : theme::stroke()); g.drawRoundedRectangle (r, 6.0f, 1.0f);
        juce::Path arrow; const float cx = (float) w - 12.0f, cy = (float) h * 0.5f; arrow.addTriangle (cx - 4, cy - 2, cx + 4, cy - 2, cx, cy + 3);
        g.setColour (theme::dim()); g.fillPath (arrow);
    }
    void positionComboBoxText (juce::ComboBox& box, juce::Label& label) override { label.setBounds (8, 1, box.getWidth() - 26, box.getHeight() - 2); label.setFont (getComboBoxFont (box)); }

    void drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool hover, bool down) override
    {
        auto r = b.getLocalBounds().toFloat().reduced (0.5f); const bool on = b.getToggleState();
        auto base = on ? theme::accent().withAlpha (0.22f) : juce::Colour (0xFF1A1D30);
        g.setColour (down ? base.brighter (0.25f) : (hover ? base.brighter (0.12f) : base)); g.fillRoundedRectangle (r, 7.0f);
        g.setColour (on || hover ? theme::accent().withAlpha (on ? 0.95f : 0.6f) : theme::stroke()); g.drawRoundedRectangle (r, 7.0f, 1.0f);
    }
    void drawButtonText (juce::Graphics& g, juce::TextButton& b, bool, bool) override
    {
        g.setFont (theme::font (12.5f, true)); g.setColour (b.getToggleState() ? theme::accent() : theme::text().withAlpha (b.isEnabled() ? 1.0f : 0.4f));
        g.drawFittedText (b.getButtonText(), b.getLocalBounds().reduced (4, 0), juce::Justification::centred, 1);
    }
    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool hover, bool) override
    {
        auto r = b.getLocalBounds().toFloat(); const bool on = b.getToggleState();
        const float tw = 30.0f, th = 16.0f; auto track = juce::Rectangle<float> (r.getX() + 2, r.getCentreY() - th * 0.5f, tw, th);
        g.setColour (on ? theme::accent().withAlpha (0.30f) : juce::Colour (0xFF1A1D30)); g.fillRoundedRectangle (track, th * 0.5f);
        g.setColour (on ? theme::accent() : (hover ? theme::dim() : theme::stroke())); g.drawRoundedRectangle (track, th * 0.5f, 1.2f);
        const float kx = on ? track.getRight() - th + 3 : track.getX() + 3;
        if (on) { g.setColour (theme::accent().withAlpha (0.35f)); g.fillEllipse (kx - 2, track.getY() + 1, th - 2, th - 2); }
        g.setColour (on ? theme::accent() : theme::dim()); g.fillEllipse (kx, track.getY() + 3, th - 6, th - 6);
        g.setColour (on ? theme::text() : theme::dim()); g.setFont (theme::font (12.0f));
        g.drawText (b.getButtonText(), r.withTrimmedLeft (tw + 8).toNearestInt(), juce::Justification::centredLeft);
    }
    void drawPopupMenuBackground (juce::Graphics& g, int w, int h) override
    {
        g.fillAll (juce::Colour (0xF40C0E18)); g.setColour (theme::stroke()); g.drawRect (0, 0, w, h);
    }
    void drawTabButton (juce::TabBarButton&, juce::Graphics&, bool, bool) override {}
    void drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float, float, const juce::Slider::SliderStyle, juce::Slider& s) override
    {
        const float cy = (float) y + (float) h * 0.5f; const float mid = (float) x + (float) w * 0.5f;
        g.setColour (juce::Colour (0xFF1A1D30)); g.fillRoundedRectangle ((float) x, cy - 3, (float) w, 6, 3);
        const bool bipolar = s.getMinimum() < 0;
        const float a = bipolar ? mid : (float) x, b = pos;
        g.setColour (theme::accent().withAlpha (0.85f)); g.fillRoundedRectangle (std::min (a, b), cy - 3, std::abs (b - a), 6, 3);
        g.setColour (theme::text()); g.fillEllipse (pos - 5, cy - 5, 10, 10);
        if (bipolar) { g.setColour (theme::dim().withAlpha (0.6f)); g.fillRect (mid - 0.5f, cy - 6, 1.0f, 12.0f); }
    }
};
