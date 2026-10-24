// =============================================================================
//  SYNTHKEY by Piradex — página MODULADORES: step sequencer, LFO desenhado, envelope multi-ponto, MPE
// =============================================================================
#pragma once
#include "Pages1.h"

namespace ui
{
using namespace sk;

// ------------------------------------------------------------------------------ utilitários
inline void paintEditorFrame (juce::Graphics& g, juce::Rectangle<float> r)
{
    g.setColour (juce::Colour (0xFF0A0C18)); g.fillRoundedRectangle (r, 6.0f);
    g.setColour (theme::stroke()); g.drawRoundedRectangle (r.reduced (0.5f), 6.0f, 1.0f);
}
inline float snapBip (float v) { v = juce::jlimit (-1.0f, 1.0f, v); if (std::fabs (v) < 0.045f) return 0.0f; return std::round (v * 20.0f) / 20.0f; }

// ------------------------------------------------------------------------------ STEP SEQUENCER
class StepEditor : public juce::Component
{
public:
    explicit StepEditor (SynthKeyProcessor& p) : proc (p) { setMouseCursor (juce::MouseCursor::UpDownResizeCursor); }
    void paint (juce::Graphics& g) override
    {
        auto full = getLocalBounds().toFloat(); paintEditorFrame (g, full);
        auto r = full.reduced (8.0f, 6.0f); const int len = juce::jlimit (1, 16, (int) proc.getReal (P_STEP_LEN));
        g.setColour (theme::dim()); g.setFont (theme::font (10.5f));
        g.drawText (TX ("arrasta as barras  \xC2\xB7  bot\xC3\xA3o direito = terminar aqui (n.\xC2\xBA de passos)"), r.toNearestInt().withHeight (14), juce::Justification::centredRight);
        r.removeFromTop (12.0f); lab = r.removeFromBottom (14.0f); area = r; cw = area.getWidth() / 16.0f;
        const float midY = area.getCentreY();
        g.setColour (theme::dim().withAlpha (0.35f)); g.drawHorizontalLine ((int) midY, area.getX(), area.getRight());
        for (int i = 0; i < 16; ++i)
        {
            const bool on = i < len; const float v = proc.getReal (P_STEP_V1 + i);
            auto cell = juce::Rectangle<float> (area.getX() + cw * (float) i + 2.5f, area.getY(), cw - 5.0f, area.getHeight());
            g.setColour (juce::Colour (on ? 0xFF11142A : 0xFF0B0D1B)); g.fillRoundedRectangle (cell, 4.0f);
            const float y = midY - v * area.getHeight() * 0.5f;
            auto bar = juce::Rectangle<float> (cell.getX() + 3, std::min (y, midY), cell.getWidth() - 6, std::max (2.0f, std::fabs (y - midY)));
            if (on) { g.setGradientFill (juce::ColourGradient (theme::accent(), bar.getX(), bar.getY(), theme::accent2(), bar.getX(), bar.getBottom(), false)); }
            else g.setColour (theme::dim().withAlpha (0.25f));
            g.fillRoundedRectangle (bar, 2.5f);
            g.setColour (on ? theme::dim().withAlpha (0.8f) : theme::dim().withAlpha (0.3f)); g.setFont (theme::font (9.5f));
            g.drawText (juce::String (i + 1), (int) cell.getX(), (int) lab.getY(), (int) cell.getWidth(), 14, juce::Justification::centred);
        }
    }
    void mouseDown (const juce::MouseEvent& e) override
    {
        const int i = col (e.position.x); if (i < 0) return;
        if (e.mods.isPopupMenu()) { proc.setReal (P_STEP_LEN, (float) (i + 1)); repaint(); return; }
        setVal (i, e.position.y);
    }
    void mouseDrag (const juce::MouseEvent& e) override { if (e.mods.isPopupMenu()) return; const int i = col (e.position.x); if (i >= 0) setVal (i, e.position.y); }
private:
    int col (float x) const { const int i = (int) std::floor ((x - area.getX()) / std::max (1.0f, cw)); return i >= 0 && i < 16 ? i : -1; }
    void setVal (int i, float y) { proc.setReal (P_STEP_V1 + i, snapBip (1.0f - 2.0f * (y - area.getY()) / std::max (1.0f, area.getHeight()))); repaint(); }
    SynthKeyProcessor& proc; juce::Rectangle<float> area, lab; float cw = 30.0f;
};

// ------------------------------------------------------------------------------ LFO DESENHADO
class DrawEditor : public juce::Component
{
public:
    explicit DrawEditor (SynthKeyProcessor& p) : proc (p) { setMouseCursor (juce::MouseCursor::CrosshairCursor); }
    void paint (juce::Graphics& g) override
    {
        auto full = getLocalBounds().toFloat(); paintEditorFrame (g, full);
        area = full.reduced (8.0f, 8.0f); area.removeFromTop (8.0f);
        g.setColour (theme::dim()); g.setFont (theme::font (10.5f));
        g.drawText (TX ("desenha a forma com o rato  \xC2\xB7  32 pontos, repete-se em ciclo"), getLocalBounds().reduced (8, 5).withHeight (14), juce::Justification::centredRight);
        const float midY = area.getCentreY(), w = area.getWidth(), step = w / 32.0f;
        g.setColour (theme::dim().withAlpha (0.35f)); g.drawHorizontalLine ((int) midY, area.getX(), area.getRight());
        juce::Path line, fill; fill.startNewSubPath (area.getX(), midY);
        for (int i = 0; i < 32; ++i)
        {
            const float v = proc.getReal (P_DRAW_P1 + i), x = area.getX() + (i + 0.5f) * step, y = midY - v * area.getHeight() * 0.5f;
            if (i == 0) line.startNewSubPath (x, y); else line.lineTo (x, y);
            fill.lineTo (x, y);
        }
        fill.lineTo (area.getRight() - step * 0.5f, midY); fill.closeSubPath();
        g.setColour (theme::accent().withAlpha (0.14f)); g.fillPath (fill);
        theme::glowStroke (g, line, theme::accent(), 2.0f);
        for (int i = 0; i < 32; ++i)
        {
            const float v = proc.getReal (P_DRAW_P1 + i), x = area.getX() + (i + 0.5f) * step, y = midY - v * area.getHeight() * 0.5f;
            g.setColour (theme::accent2()); g.fillEllipse (x - 2.5f, y - 2.5f, 5.0f, 5.0f);
        }
    }
    void mouseDown (const juce::MouseEvent& e) override { last = -1; paintAt (e.position); }
    void mouseDrag (const juce::MouseEvent& e) override { paintAt (e.position); }
    static void applyShape (SynthKeyProcessor& p, int kind)
    {
        unsigned rs = 2463534242u;
        for (int i = 0; i < 32; ++i)
        {
            const float t = (float) i / 32.0f; float v = 0;
            switch (kind)
            {
                case 0: v = std::sin (6.2831853f * t); break;
                case 1: v = 4.0f * std::fabs (t - 0.5f) - 1.0f; v = -v; break;
                case 2: v = 2.0f * t - 1.0f; break;
                case 3: v = 1.0f - 2.0f * t; break;
                case 4: v = t < 0.5f ? 1.0f : -1.0f; break;
                case 5: rs ^= rs << 13; rs ^= rs >> 17; rs ^= rs << 5; v = snapBip ((float) (rs & 0xFFFF) / 32767.5f - 1.0f); break;
                default: v = 0; break;
            }
            p.setReal (P_DRAW_P1 + i, snapBip (v));
        }
    }
private:
    void paintAt (juce::Point<float> pt)
    {
        const int i = juce::jlimit (0, 31, (int) std::floor ((pt.x - area.getX()) / std::max (1.0f, area.getWidth()) * 32.0f));
        const float v = snapBip (1.0f - 2.0f * (pt.y - area.getY()) / std::max (1.0f, area.getHeight()));
        if (last >= 0 && std::abs (i - last) > 1)
        { const int a = std::min (i, last), b = std::max (i, last); const float va = i < last ? v : lastV, vb = i < last ? lastV : v; for (int k = a; k <= b; ++k) proc.setReal (P_DRAW_P1 + k, snapBip (va + (vb - va) * (float) (k - a) / (float) (b - a))); }
        else proc.setReal (P_DRAW_P1 + i, v);
        last = i; lastV = v; repaint();
    }
    SynthKeyProcessor& proc; juce::Rectangle<float> area; int last = -1; float lastV = 0;
};

class ShapeButtons : public juce::Component
{
public:
    explicit ShapeButtons (SynthKeyProcessor& p)
    {
        static const char* names[6] = { "SENO", "TRI", "SERRA", "RAMPA", "QUAD.", "ALEAT." };
        for (int i = 0; i < 6; ++i) { auto* b = new juce::TextButton (TX (names[i])); btn.add (b); addAndMakeVisible (b); b->onClick = [&p, i, this] { DrawEditor::applyShape (p, i); if (auto* par = getParentComponent()) par->repaint(); }; }
    }
    void resized() override { const int w = getWidth() / 6; for (int i = 0; i < 6; ++i) btn[i]->setBounds (i * w, 0, w - 3, getHeight()); }
private:
    juce::OwnedArray<juce::TextButton> btn;
};

// ------------------------------------------------------------------------------ ENVELOPE MULTI-PONTO
class MultiEnvEditor : public juce::Component
{
public:
    explicit MultiEnvEditor (SynthKeyProcessor& p) : proc (p) {}
    void paint (juce::Graphics& g) override
    {
        auto full = getLocalBounds().toFloat(); paintEditorFrame (g, full);
        area = full.reduced (14.0f, 12.0f); area.removeFromBottom (4.0f);
        const int N = n(), sus = (int) proc.getReal (P_MENV_SUS);
        g.setColour (theme::dim()); g.setFont (theme::font (10.5f));
        g.drawText (TX ("arrasta os pontos  \xC2\xB7  bot\xC3\xA3o direito num ponto = sustain (S)  \xC2\xB7  + / \xE2\x88\x92 = pontos"), getLocalBounds().reduced (8, 3).withHeight (14), juce::Justification::centredRight);
        g.setColour (theme::dim().withAlpha (0.25f)); g.drawHorizontalLine ((int) area.getBottom(), area.getX(), area.getRight()); g.drawHorizontalLine ((int) area.getY(), area.getX(), area.getRight());
        juce::Path line, fill; fill.startNewSubPath (px (0), area.getBottom());
        for (int i = 0; i < N; ++i) { const auto q = pt (i); if (i == 0) line.startNewSubPath (q); else line.lineTo (q); fill.lineTo (q); }
        fill.lineTo (pt (N - 1).x, area.getBottom()); fill.closeSubPath();
        g.setColour (theme::accent().withAlpha (0.13f)); g.fillPath (fill);
        theme::glowStroke (g, line, theme::accent(), 2.0f);
        if (sus >= 1 && sus <= N)   // libertação a partir do ponto de sustain
        {
            const auto s = pt (sus - 1); juce::Path rel; rel.startNewSubPath (s); rel.lineTo (std::min (area.getRight(), s.x + 0.18f * area.getWidth()), area.getBottom());
            const float dash[2] = { 4.0f, 4.0f }; juce::Path d; juce::PathStrokeType (1.5f).createDashedStroke (d, rel, dash, 2); g.setColour (theme::accent2().withAlpha (0.8f)); g.fillPath (d);
        }
        for (int i = 0; i < N; ++i)
        {
            const auto q = pt (i); const bool isSus = (i + 1 == sus);
            g.setColour (isSus ? theme::accent2() : juce::Colours::white); g.fillEllipse (q.x - 5.0f, q.y - 5.0f, 10.0f, 10.0f);
            g.setColour (theme::accent()); g.drawEllipse (q.x - 5.0f, q.y - 5.0f, 10.0f, 10.0f, 1.5f);
            if (isSus) { g.setColour (theme::accent2()); g.setFont (theme::font (10.0f, true)); g.drawText ("S", (int) q.x - 8, (int) q.y - 22, 16, 12, juce::Justification::centred); }
        }
    }
    void mouseDown (const juce::MouseEvent& e) override
    {
        drag = hit (e.position); if (drag < 0) return;
        if (e.mods.isPopupMenu()) { proc.setReal (P_MENV_SUS, (int) proc.getReal (P_MENV_SUS) == drag + 1 ? 0.0f : (float) (drag + 1)); drag = -1; repaint(); }
    }
    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (drag < 0) return;
        const float x = juce::jlimit (0.0f, 1.0f, (e.position.x - area.getX()) / std::max (1.0f, area.getWidth()));
        const float y = juce::jlimit (0.0f, 1.0f, 1.0f - (e.position.y - area.getY()) / std::max (1.0f, area.getHeight()));
        const int N = n();
        if (drag > 0)
        {
            const float lo = proc.getReal (P_MENV_X1 + 2 * (drag - 1)), hi = drag + 1 < N ? proc.getReal (P_MENV_X1 + 2 * (drag + 1)) : 1.0f;
            proc.setReal (P_MENV_X1 + 2 * drag, juce::jlimit (lo, std::max (lo, hi), x));
        }
        proc.setReal (P_MENV_Y1 + 2 * drag, std::round (y * 100.0f) / 100.0f); repaint();
    }
    void mouseUp (const juce::MouseEvent&) override { drag = -1; }
    void addPoint()
    {
        const int N = n(); if (N >= 8) return;
        const float lx = proc.getReal (P_MENV_X1 + 2 * (N - 1)), ly = proc.getReal (P_MENV_Y1 + 2 * (N - 1));
        // o último ponto passa a ser o novo; o antigo "fim" fica a meio caminho
        proc.setReal (P_MENV_X1 + 2 * N, std::min (1.0f, lx + (1.0f - lx) * 0.5f + (lx >= 0.999f ? 0.0f : 0.0f))); proc.setReal (P_MENV_Y1 + 2 * N, ly);
        if (lx >= 0.999f) { proc.setReal (P_MENV_X1 + 2 * (N - 1), std::max (proc.getReal (P_MENV_X1 + 2 * (N - 2)), lx - 0.15f)); proc.setReal (P_MENV_X1 + 2 * N, 1.0f); }
        proc.setReal (P_MENV_N, (float) (N + 1)); repaint();
    }
    void removePoint()
    {
        const int N = n(); if (N <= 2) return;
        proc.setReal (P_MENV_N, (float) (N - 1)); if ((int) proc.getReal (P_MENV_SUS) > N - 1) proc.setReal (P_MENV_SUS, 0.0f); repaint();
    }
private:
    int n() const { return juce::jlimit (2, 8, (int) proc.getReal (P_MENV_N)); }
    float px (int i) const { return area.getX() + (i == 0 ? 0.0f : proc.getReal (P_MENV_X1 + 2 * i) * area.getWidth()); }
    juce::Point<float> pt (int i) const { return { px (i), area.getBottom() - proc.getReal (P_MENV_Y1 + 2 * i) * area.getHeight() }; }
    int hit (juce::Point<float> p) const { int best = -1; float bd = 14.0f; for (int i = 0; i < n(); ++i) { const float d = pt (i).getDistanceFrom (p); if (d < bd) { bd = d; best = i; } } return best; }
    SynthKeyProcessor& proc; juce::Rectangle<float> area; int drag = -1;
};

class PointButtons : public juce::Component
{
public:
    explicit PointButtons (MultiEnvEditor& ed)
    {
        plus.setButtonText ("+"); minus.setButtonText (TX ("\xE2\x88\x92")); addAndMakeVisible (plus); addAndMakeVisible (minus);
        plus.onClick = [&ed] { ed.addPoint(); }; minus.onClick = [&ed] { ed.removePoint(); };
    }
    void resized() override { plus.setBounds (0, 0, getWidth() / 2 - 2, getHeight()); minus.setBounds (getWidth() / 2 + 2, 0, getWidth() / 2 - 2, getHeight()); }
private:
    juce::TextButton plus, minus;
};

class HintBox : public juce::Component
{
public:
    explicit HintBox (const juce::String& t) : txt (t) {}
    void paint (juce::Graphics& g) override { g.setColour (theme::dim()); g.setFont (theme::font (12.0f)); g.drawFittedText (txt, getLocalBounds(), juce::Justification::topLeft, 8); }
private:
    juce::String txt;
};

// ================================================================================ página
class ModulatorsPage : public Page
{
public:
    explicit ModulatorsPage (SynthKeyProcessor& p) : Page (p)
    {
        {   // step sequencer
            auto* s = addSection (TX ("STEP SEQUENCER  \xC2\xB7  origem \"Step Seq\""), { 0, 0, 566, 222 });
            C (s, P_STEP_DIV, 92, 1, {}, 20); K (s, P_STEP_LEN, "PASSOS"); K (s, P_STEP_SMOOTH, "SUAVIZAR", 2);
            s->add (new HintBox (TX ("Cada passo vale de \xE2\x88\x92" "1 a +1. Liga a origem \"Step Seq\" a qualquer destino na MATRIZ (ou arrasta-a para um bot\xC3\xA3o).")), 290, 60, 1);
            s->newRow(); s->add (new StepEditor (proc), 542, 112, 1);
        }
        {   // LFO desenhado
            auto* s = addSection (TX ("LFO DESENHADO  \xC2\xB7  origem \"LFO Desenhado\""), { 574, 0, 582, 222 });
            s->add (new juce::Component(), 1, 68, 1);                      // fixa a altura da linha (o botão TAXA aparece/desaparece)
            T (s, P_DRAW_SYNC, "SYNC", 80, 1, {}, 20);
            K (s, P_DRAW_RATE, "TAXA", 1, [=] { return val (P_DRAW_SYNC) == 0; });
            C (s, P_DRAW_DIV, 84, 1, [=] { return val (P_DRAW_SYNC) != 0; }, 20);
            { auto* sb = s->add (new ShapeButtons (proc), 300, 26, 1, {}, false); s->setYOffset (sb, 22); }
            s->newRow(); s->add (new DrawEditor (proc), 558, 112, 1);
        }
        {   // envelope multi-ponto
            auto* s = addSection (TX ("ENVELOPE MULTI-PONTO  \xC2\xB7  origem \"Env Multi-ponto\""), { 0, 230, 720, 222 });
            K (s, P_MENV_LEN, TX ("DURA\xC3\x87\xC3\x83O")); K (s, P_MENV_REL, "LIBERT."); K (s, P_MENV_SUS, "SUSTAIN", 2); T (s, P_MENV_LOOP, "LOOP", 84, 2, {}, 20);
            auto* ed = new MultiEnvEditor (proc);
            { auto* pb = s->add (new PointButtons (*ed), 90, 28, 1, {}, false); s->setYOffset (pb, 20); }
            s->newRow(); s->add (ed, 696, 112, 1);
        }
        {   // MPE
            auto* s = addSection (TX ("MPE & EXPRESS\xC3\x83O POR NOTA"), { 728, 230, 428, 222 });
            s->add (new juce::Component(), 1, 68, 1);
            T (s, P_MPE_ON, "MPE ATIVO", 110, 1, {}, 20); K (s, P_MPE_RANGE, "CURVATURA", 1, [this] { return val (P_MPE_ON) != 0; });
            s->newRow();
            s->add (new HintBox (TX ("Com um controlador MPE (Seaboard, Linnstrument, Sensel\xE2\x80\xA6) cada nota curva a altura, "
                                      "tem press\xC3\xA3o e slide (CC74) pr\xC3\xB3prios. Usa \"Press\xC3\xA3o (por nota)\" e \"Slide (MPE)\" como origens na MATRIZ. "
                                      "O aftertouch polif\xC3\xB3nico de qualquer teclado tamb\xC3\xA9m alimenta \"Press\xC3\xA3o (por nota)\".")), 404, 100, 1);
        }
        setMode (2);
    }
};
}
