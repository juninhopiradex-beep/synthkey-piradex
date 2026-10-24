// =============================================================================
//  SYNTHKEY by Piradex — tema "Orbit": barra de cima, teclado de performance, rodas, dica de nota
// =============================================================================
#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <cmath>
#include "Controls.h"

namespace ui
{
using namespace sk;

// cor associada a cada origem de modulação (usada na barra de origens e na matriz)
inline juce::Colour srcColour (int src)
{
    switch (src)
    {
        case MS_LFO1:  return theme::accent();
        case MS_LFO2:  return theme::accent2();
        case MS_ENV2: case MS_ENV3: case MS_ENVAMP: return juce::Colour (0xFF3DE0C0);
        case MS_STEP:  return juce::Colour (0xFFFFB547);
        case MS_DRAW:  return juce::Colour (0xFFFF5FD2);
        case MS_MENV:  return juce::Colour (0xFF4CC9FF);
        case MS_NONE:  return juce::Colour (0xFF5F6470);
        default:       return juce::Colour (0xFFAEB4C0);
    }
}
inline const char* shortSrcName (int s)
{
    static const char* n[MS_COUNT] = { "---", "LFO 1", "LFO 2", "ENV 2", "ENV 3", "VELOC.", "TECLA", "MOD", "AFTER", "BEND", "ALEAT.",
        "MAC 1", "MAC 2", "MAC 3", "MAC 4", "MAC 5", "MAC 6", "MAC 7", "MAC 8", "ENV AMP", "CONST", "STEP", "DESEN.", "MULTI", "PRESS.", "SLIDE" };
    return n[juce::jlimit (0, MS_COUNT - 1, s)];
}

// ---------------------------------------------------------------------------------- ícones da barra de cima
class IconBtn : public juce::Button
{
public:
    enum Kind { Undo, Redo, Prev, Next, Menu, Help };
    explicit IconBtn (Kind k) : juce::Button (""), kind (k) { setMouseCursor (juce::MouseCursor::PointingHandCursor); }
    void paintButton (juce::Graphics& g, bool hover, bool down) override
    {
        auto r = getLocalBounds().toFloat(); const auto c = r.getCentre();
        if (hover || down) { g.setColour (juce::Colours::white.withAlpha (down ? 0.12f : 0.07f)); g.fillRoundedRectangle (r.reduced (1.0f), 8.0f); }
        g.setColour (isEnabled() ? (hover ? juce::Colours::white : theme::text().withAlpha (0.85f)) : theme::dim().withAlpha (0.4f));
        juce::Path p; const juce::PathStrokeType st (1.7f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);
        switch (kind)
        {
            case Undo: case Redo:
            {
                p.startNewSubPath (c.x - 3.0f, c.y - 6.0f); p.lineTo (c.x - 7.0f, c.y - 2.0f); p.lineTo (c.x - 3.0f, c.y + 2.0f);
                p.startNewSubPath (c.x - 7.0f, c.y - 2.0f); p.lineTo (c.x + 1.0f, c.y - 2.0f); p.quadraticTo (c.x + 7.0f, c.y - 2.0f, c.x + 7.0f, c.y + 3.0f);
                p.quadraticTo (c.x + 7.0f, c.y + 7.5f, c.x + 1.0f, c.y + 7.5f); p.lineTo (c.x - 3.0f, c.y + 7.5f);
                if (kind == Redo) p.applyTransform (juce::AffineTransform::scale (-1.0f, 1.0f, c.x, c.y));
                g.strokePath (p, st); break;
            }
            case Prev: p.startNewSubPath (c.x + 3, c.y - 6); p.lineTo (c.x - 3, c.y); p.lineTo (c.x + 3, c.y + 6); g.strokePath (p, st); break;
            case Next: p.startNewSubPath (c.x - 3, c.y - 6); p.lineTo (c.x + 3, c.y); p.lineTo (c.x - 3, c.y + 6); g.strokePath (p, st); break;
            case Menu: for (int i = -1; i <= 1; ++i) g.fillEllipse (c.x + (float) i * 7.0f - 1.8f, c.y - 1.8f, 3.6f, 3.6f); break;
            case Help: g.drawEllipse (c.x - 8.0f, c.y - 8.0f, 16.0f, 16.0f, 1.5f); g.setFont (theme::font (11.5f, true)); g.drawText ("?", getLocalBounds(), juce::Justification::centred); break;
        }
    }
private:
    Kind kind;
};

// ---------------------------------------------------------------------------------- nome do preset + etiqueta
class PresetPill : public juce::Button
{
public:
    PresetPill() : juce::Button ("") { setMouseCursor (juce::MouseCursor::PointingHandCursor); }
    juce::String tag;
    void paintButton (juce::Graphics& g, bool hover, bool) override
    {
        auto r = getLocalBounds().toFloat().reduced (0.5f);
        g.setColour (hover ? juce::Colour (0xFF1A1A26) : juce::Colour (0xFF13131C)); g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
        g.setColour (hover ? theme::accent().withAlpha (0.7f) : theme::stroke()); g.drawRoundedRectangle (r, r.getHeight() * 0.5f, 1.0f);
        g.setFont (theme::font (14.0f, true)); g.setColour (juce::Colours::white);
        int right = getWidth() - 14;
        if (tag.isNotEmpty())
        {
            g.setFont (theme::font (10.5f, true)); const int tw = juce::GlyphArrangement::getStringWidthInt (g.getCurrentFont(), tag) + 16;
            auto tr = juce::Rectangle<float> ((float) (getWidth() - tw - 10), r.getCentreY() - 10.0f, (float) tw, 20.0f);
            g.setColour (theme::accent().withAlpha (0.14f)); g.fillRoundedRectangle (tr, 6.0f);
            g.setColour (theme::accent().withAlpha (0.6f)); g.drawRoundedRectangle (tr.reduced (0.5f), 6.0f, 1.0f);
            g.setColour (theme::accent().brighter (0.5f)); g.drawText (tag, tr.toNearestInt(), juce::Justification::centred);
            right = (int) tr.getX() - 8;
        }
        g.setFont (theme::font (14.0f, true)); g.setColour (juce::Colours::white);
        g.drawFittedText (getButtonText(), 16, 0, std::max (10, right - 16), getHeight(), juce::Justification::centredLeft, 1, 0.8f);
    }
};

// ---------------------------------------------------------------------------------- botão-orbe "Descrever som"
class OrbButton : public juce::Button
{
public:
    OrbButton() : juce::Button ("") { setMouseCursor (juce::MouseCursor::PointingHandCursor); }
    void paintButton (juce::Graphics& g, bool hover, bool down) override
    {
        const float d = 32.0f, cy = (float) getHeight() * 0.5f; auto o = juce::Rectangle<float> (4.0f, cy - d * 0.5f, d, d);
        const auto c = theme::accent();
        g.setColour (c.withAlpha (hover ? 0.30f : 0.20f)); g.fillEllipse (o.expanded (5.0f));
        g.setColour (c.withAlpha (0.14f)); g.fillEllipse (o.expanded (9.0f));
        g.setGradientFill (juce::ColourGradient (c.brighter (0.9f), o.getX() + d * 0.3f, o.getY() + d * 0.25f, c.darker (0.8f), o.getRight(), o.getBottom(), true)); g.fillEllipse (down ? o.reduced (1.5f) : o);
        g.setColour (juce::Colours::white.withAlpha (0.35f)); g.fillEllipse (o.getX() + 6, o.getY() + 3, d * 0.5f, d * 0.22f);
        g.setColour (juce::Colours::white); g.setFont (theme::font (16.0f, true)); g.drawText (TX ("\xE2\x9C\xA6"), o.toNearestInt(), juce::Justification::centred);
        g.setColour (hover ? juce::Colours::white : c.brighter (0.6f)); g.setFont (theme::font (13.0f, true));
        g.drawText (TX ("Descrever som\xE2\x80\xA6"), (int) o.getRight() + 12, 0, getWidth() - (int) o.getRight() - 12, getHeight(), juce::Justification::centredLeft);
    }
};

// ---------------------------------------------------------------------------------- separadores de navegação (em baixo)
class NavBtn : public juce::Button
{
public:
    explicit NavBtn (const juce::String& t) : juce::Button (t) { setClickingTogglesState (false); setMouseCursor (juce::MouseCursor::PointingHandCursor); }
    void paintButton (juce::Graphics& g, bool hover, bool down) override
    {
        const bool on = getToggleState(); auto r = getLocalBounds().toFloat(); auto pill = r.reduced (3.0f, 2.0f);
        if (on)
        {
            g.setGradientFill (juce::ColourGradient (theme::accent().brighter (0.15f).withAlpha (0.95f), 0, pill.getY(), theme::accent2().withAlpha (0.55f), 0, pill.getBottom(), false)); g.fillRoundedRectangle (pill, 7.0f);
            g.setColour (theme::accent().brighter (0.3f).withAlpha (0.9f)); g.drawRoundedRectangle (pill.reduced (0.5f), 7.0f, 1.2f);
        }
        else if (hover || down) { g.setColour (theme::accent().withAlpha (down ? 0.28f : 0.16f)); g.fillRoundedRectangle (pill, 7.0f); }
        g.setFont (theme::font (13.0f, true)); g.setColour (on ? juce::Colours::white : (hover ? juce::Colours::white : theme::text().withAlpha (0.95f)));
        g.drawText (getButtonText(), r.toNearestInt(), juce::Justification::centred);
        if (on) { juce::Path p; p.startNewSubPath (r.getX() + 14, 1.5f); p.lineTo (r.getRight() - 14, 1.5f); theme::glowStroke (g, p, theme::accent().brighter (0.4f), 3.0f); }
    }
};

// ---------------------------------------------------------------------------------- teclado de performance
class PerfKeyboard : public juce::MidiKeyboardComponent
{
public:
    explicit PerfKeyboard (juce::MidiKeyboardState& s) : juce::MidiKeyboardComponent (s, juce::MidiKeyboardComponent::horizontalKeyboard), st (s)
    {
        for (auto& v : vel) v.store (0.8f);
        velListener.owner = this; st.addListener (&velListener);
        setAvailableRange (24, 96); setLowestVisibleKey (24); setScrollButtonsVisible (false); setMidiChannel (1);
        setBlackNoteLengthProportion (0.6f); setVelocity (0.85f, true); setOctaveForMiddleC (4); setWantsKeyboardFocus (true);
    }
    ~PerfKeyboard() override { st.removeListener (&velListener); }

    static juce::Colour noteColour (int note) { return juce::Colour::fromHSV (std::fmod ((float) (note % 12) / 12.0f + 0.50f, 1.0f), 0.62f, 1.0f, 1.0f); }

    // nota a mostrar na dica: a que está sob o rato, senão a última tocada que continua premida
    bool tipFor (int& note, float& v, juce::Rectangle<float>& keyRect) const
    {
        int n = hover;
        if (n < 0) { n = lastNote.load(); if (n < 0 || ! st.isNoteOnForChannels (0xffff, n)) return false; }
        note = n; v = vel[(size_t) n].load(); keyRect = getRectangleForKey (n); return true;
    }

    void mouseMove (const juce::MouseEvent& e) override { juce::MidiKeyboardComponent::mouseMove (e); hover = getNoteAndVelocityAtPosition (e.position).note; }
    void mouseDrag (const juce::MouseEvent& e) override { juce::MidiKeyboardComponent::mouseDrag (e); hover = getNoteAndVelocityAtPosition (e.position).note; }
    void mouseExit (const juce::MouseEvent& e) override { juce::MidiKeyboardComponent::mouseExit (e); hover = -1; }

    void drawWhiteNote (int n, juce::Graphics& g, juce::Rectangle<float> a, bool down, bool over, juce::Colour, juce::Colour) override
    {
        auto r = a.reduced (1.0f, 0.0f).withTrimmedBottom (1.0f);
        juce::Path p; p.addRoundedRectangle (r.getX(), r.getY(), r.getWidth(), r.getHeight(), 5.0f, 5.0f, false, false, true, true);
        const auto nc = noteColour (n);
        g.setGradientFill (juce::ColourGradient (over ? juce::Colour (0xFF34334A) : juce::Colour (0xFF2B2A3C), 0, r.getY(), juce::Colour (0xFF1B1A28), 0, r.getBottom(), false)); g.fillPath (p);
        if (down)
        {
            g.setGradientFill (juce::ColourGradient (nc.withAlpha (0.50f), 0, r.getY(), nc.withAlpha (0.14f), 0, r.getBottom(), false)); g.fillPath (p);
            const float h = std::max (6.0f, r.getHeight() * vel[(size_t) n].load());
            juce::Path bar; bar.addRoundedRectangle (r.getX(), r.getBottom() - h, r.getWidth(), h, 5.0f, 5.0f, false, false, true, true);
            g.setGradientFill (juce::ColourGradient (nc.withAlpha (0.10f), 0, r.getBottom() - h, nc.withAlpha (0.85f), 0, r.getBottom(), false)); g.fillPath (bar);
        }
        g.setColour (juce::Colour (0xFF0F0E17)); g.drawLine (a.getRight() - 0.5f, a.getY(), a.getRight() - 0.5f, a.getBottom(), 1.0f);
        if (n % 12 == 0 || down)
        {
            g.setFont (theme::font (8.5f, true)); g.setColour (down ? juce::Colours::white : juce::Colour (0xFF7C7A92));
            g.drawText (juce::MidiMessage::getMidiNoteName (n, true, true, 4), r.toNearestInt().withTrimmedBottom (3), juce::Justification::centredBottom);
        }
    }
    void drawBlackNote (int n, juce::Graphics& g, juce::Rectangle<float> a, bool down, bool over, juce::Colour) override
    {
        auto r = a.reduced (0.0f, 0.0f);
        g.setColour (juce::Colours::black.withAlpha (0.45f)); g.fillRoundedRectangle (r.translated (1.5f, 2.0f), 4.0f);
        juce::Path p; p.addRoundedRectangle (r.getX(), r.getY(), r.getWidth(), r.getHeight(), 4.0f, 4.0f, false, false, true, true);
        g.setGradientFill (juce::ColourGradient (over ? juce::Colour (0xFF181725) : juce::Colour (0xFF0A0A10), 0, r.getY(), juce::Colour (0xFF22212F), 0, r.getBottom(), false)); g.fillPath (p);
        if (down)
        {
            const auto nc = noteColour (n); const float h = std::max (5.0f, r.getHeight() * vel[(size_t) n].load());
            juce::Path bar; bar.addRoundedRectangle (r.getX(), r.getBottom() - h, r.getWidth(), h, 4.0f, 4.0f, false, false, true, true);
            g.setGradientFill (juce::ColourGradient (nc.withAlpha (0.10f), 0, r.getBottom() - h, nc.withAlpha (0.9f), 0, r.getBottom(), false)); g.fillPath (bar);
        }
        g.setColour (juce::Colour (0x22FFFFFF)); g.drawRoundedRectangle (r.reduced (0.5f), 4.0f, 1.0f);
    }
    void paintOverChildren (juce::Graphics& g) override
    {
        for (int n = 24; n <= 96; ++n)
        {
            if (! st.isNoteOnForChannels (0xffff, n)) continue;
            const auto r = getRectangleForKey (n); const bool black = juce::MidiMessage::isMidiNoteBlack (n);
            const float cx = r.getCentreX(), cy = black ? r.getBottom() - 12.0f : r.getY() + 11.0f; const auto c = noteColour (n);
            g.setColour (c.withAlpha (0.28f)); g.fillEllipse (cx - 12.0f, cy - 12.0f, 24.0f, 24.0f);
            g.setColour (c); g.fillEllipse (cx - 7.0f, cy - 7.0f, 14.0f, 14.0f);
            g.setColour (juce::Colours::white); g.drawEllipse (cx - 7.0f, cy - 7.0f, 14.0f, 14.0f, 2.0f);
        }
    }
private:
    struct VelListener : juce::MidiKeyboardState::Listener
    {
        void handleNoteOn (juce::MidiKeyboardState*, int, int n, float v) override { if (owner && n >= 0 && n < 128) { owner->vel[(size_t) n].store (v); owner->lastNote.store (n); } }
        void handleNoteOff (juce::MidiKeyboardState*, int, int, float) override {}
        PerfKeyboard* owner = nullptr;
    } velListener;
    juce::MidiKeyboardState& st; std::array<std::atomic<float>, 128> vel; std::atomic<int> lastNote { -1 }; int hover = -1;
};

// ---------------------------------------------------------------------------------- rodas de Pitch e Mod
class WheelStrip : public juce::Component
{
public:
    WheelStrip (SynthKeyProcessor& p, bool pitchWheel, juce::Colour c, const juce::String& cap) : proc (p), isPitch (pitchWheel), col (c), caption (cap)
    { value = pitchWheel ? 0.5f : 0.0f; setMouseCursor (juce::MouseCursor::UpDownResizeCursor); }
    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        g.setColour (juce::Colour (0xFF0D0C13)); g.fillRoundedRectangle (r, 8.0f);
        g.setColour (juce::Colour (0xFF2A2838)); g.drawRoundedRectangle (r.reduced (0.5f), 8.0f, 1.0f);
        const auto tr = juce::Rectangle<float> (r.getCentreX() - 4.0f, 7.0f, 8.0f, r.getHeight() - 24.0f);
        g.setColour (juce::Colour (0xFF1D1B29)); g.fillRoundedRectangle (tr, 4.0f);
        const float y = tr.getBottom() - value * tr.getHeight();
        if (isPitch) { g.setColour (col.withAlpha (0.5f)); g.fillRect (tr.getX(), std::min (y, tr.getCentreY()), tr.getWidth(), std::abs (y - tr.getCentreY())); }
        else { g.setColour (col.withAlpha (0.5f)); g.fillRoundedRectangle (tr.withTop (y), 4.0f); }
        auto th = juce::Rectangle<float> (r.getCentreX() - 11.0f, y - 5.0f, 22.0f, 10.0f);
        g.setColour (col.withAlpha (0.35f)); g.fillRoundedRectangle (th.expanded (3.0f), 7.0f);
        g.setGradientFill (juce::ColourGradient (juce::Colours::white, 0, th.getY(), col, 0, th.getBottom(), false)); g.fillRoundedRectangle (th, 4.0f);
        g.setColour (theme::dim()); g.setFont (theme::font (8.0f, true)); g.drawText (caption, getLocalBounds().removeFromBottom (14), juce::Justification::centred);
    }
    void mouseDown (const juce::MouseEvent& e) override { set (e); }
    void mouseDrag (const juce::MouseEvent& e) override { set (e); }
    void mouseUp (const juce::MouseEvent&) override { if (isPitch) { value = 0.5f; send(); repaint(); } }
    void mouseDoubleClick (const juce::MouseEvent&) override { value = isPitch ? 0.5f : 0.0f; send(); repaint(); }
private:
    void set (const juce::MouseEvent& e)
    {
        const float top = 7.0f, h = (float) getHeight() - 24.0f; value = juce::jlimit (0.0f, 1.0f, 1.0f - (e.position.y - top) / h);
        if (isPitch && std::abs (value - 0.5f) < 0.04f) value = 0.5f;
        send(); repaint();
    }
    void send()
    {
        juce::MidiMessage m = isPitch ? juce::MidiMessage::pitchWheel (1, juce::jlimit (0, 16383, (int) std::lround (value * 16383.0f)))
                                      : juce::MidiMessage::controllerEvent (1, 1, juce::jlimit (0, 127, (int) std::lround (value * 127.0f)));
        m.setTimeStamp (juce::Time::getMillisecondCounterHiRes() * 0.001); proc.uiMidi.addMessageToQueue (m);
    }
    SynthKeyProcessor& proc; bool isPitch; juce::Colour col; juce::String caption; float value = 0.0f;
};

// ---------------------------------------------------------------------------------- dica clara por cima do teclado
class KeyTip : public juce::Component
{
public:
    KeyTip() { setInterceptsMouseClicks (false, false); setVisible (false); }
    void show (int note, float vel)
    {
        const double hz = 440.0 * std::pow (2.0, (note - 69) / 12.0);
        const auto l1 = TX ("Nota ") + juce::MidiMessage::getMidiNoteName (note, true, true, 4), l2 = juce::String (hz, 2) + " Hz", l3 = TX ("Velocidade ") + juce::String ((int) std::lround (vel * 127.0f));
        if (l1 != a || l2 != b || l3 != c || ! isVisible()) { a = l1; b = l2; c = l3; repaint(); }
        setVisible (true);
    }
    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced (2.0f);
        g.setColour (juce::Colours::black.withAlpha (0.35f)); g.fillRoundedRectangle (r.translated (0, 3.0f), 9.0f);
        g.setColour (juce::Colour (0xFFECEBF4)); g.fillRoundedRectangle (r, 9.0f);
        g.setColour (juce::Colour (0xFF1B1A24)); g.setFont (theme::font (12.5f, true)); g.drawText (a, r.toNearestInt().removeFromTop (20).translated (0, 4), juce::Justification::centred);
        g.setFont (theme::font (12.0f)); g.drawText (b, r.toNearestInt().withTop ((int) r.getY() + 20).withHeight (15), juce::Justification::centred);
        g.setColour (juce::Colour (0xFF58566B)); g.setFont (theme::font (11.0f)); g.drawText (c, r.toNearestInt().withTop ((int) r.getY() + 35).withHeight (15), juce::Justification::centred);
    }
private:
    juce::String a, b, c;
};
} // namespace ui
