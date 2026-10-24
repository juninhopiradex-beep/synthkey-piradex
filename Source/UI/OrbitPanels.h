// =============================================================================
//  SYNTHKEY by Piradex — peças do ecrã SOM no formato "Orbit"
//  (orbe do oscilador, barra de origens, matriz compacta, resumo de efeitos, arpejador em miniatura)
// =============================================================================
#pragma once
#include <juce_dsp/juce_dsp.h>
#include "OrbitWidgets.h"
#include "Visual.h"
#include "ModDrag.h"

namespace ui
{
using namespace sk;

// ---------------------------------------------------------------------------------- cartão de fundo com título
class CardBg : public juce::Component
{
public:
    explicit CardBg (const juce::String& t = {}) : title (t) { setInterceptsMouseClicks (false, false); }
    void setRight (const juce::String& s) { if (s != right) { right = s; repaint(); } }
    juce::Colour dot;
    void paint (juce::Graphics& g) override
    {
        theme::drawCard (g, getLocalBounds().toFloat());
        if (title.isEmpty()) return;
        const auto c = dot == juce::Colour() ? theme::accent() : dot;
        g.setColour (c.withAlpha (0.25f)); g.fillEllipse (11.0f, 10.0f, 11.0f, 11.0f); g.setColour (c); g.fillEllipse (13.5f, 12.5f, 6.0f, 6.0f);
        g.setFont (theme::font (10.5f, true)); g.setColour (theme::text().withAlpha (0.88f)); g.drawText (title, 28, 5, getWidth() - 34, 20, juce::Justification::centredLeft);
        if (right.isNotEmpty()) { g.setFont (theme::font (10.0f)); g.setColour (theme::dim()); g.drawText (right, 0, 5, getWidth() - 12, 20, juce::Justification::centredRight); }
    }
private:
    juce::String title, right;
};

// ---------------------------------------------------------------------------------- separadores pequenos (cartão do oscilador / filtros)
class PillTabs : public juce::Component
{
public:
    std::function<void (int)> onPick;
    void setLabels (const juce::StringArray& l, const juce::Array<int>& widths) { labels = l; w = widths; repaint(); }
    void setSelected (int i) { if (i != sel) { sel = i; repaint(); } }
    int selected() const { return sel; }
    void paint (juce::Graphics& g) override
    {
        auto full = getLocalBounds().toFloat();
        g.setColour (juce::Colour (0xFF0C0C12)); g.fillRoundedRectangle (full.withWidth (totalW() + 6.0f), 9.0f);
        g.setColour (theme::stroke()); g.drawRoundedRectangle (full.withWidth (totalW() + 6.0f).reduced (0.5f), 9.0f, 1.0f);
        for (int i = 0; i < labels.size(); ++i)
        {
            auto r = rectFor (i); const bool on = i == sel;
            if (on) { g.setColour (juce::Colour (0xFF2B2A3A)); g.fillRoundedRectangle (r, 6.0f); g.setColour (theme::accent().withAlpha (0.55f)); g.drawRoundedRectangle (r.reduced (0.5f), 6.0f, 1.0f); }
            g.setFont (theme::font (11.0f, on)); g.setColour (on ? juce::Colours::white : (i == hot ? theme::text() : theme::dim()));
            g.drawFittedText (labels[i], r.toNearestInt(), juce::Justification::centred, 1, 0.8f);
        }
    }
    void mouseMove (const juce::MouseEvent& e) override { const int h = at (e.position); if (h != hot) { hot = h; repaint(); } }
    void mouseExit (const juce::MouseEvent&) override { hot = -1; repaint(); }
    void mouseDown (const juce::MouseEvent& e) override { const int i = at (e.position); if (i >= 0) { sel = i; repaint(); if (onPick) onPick (i); } }
private:
    float totalW() const { float t = 0; for (int x : w) t += (float) x; return t; }
    juce::Rectangle<float> rectFor (int i) const { float x = 3.0f; for (int k = 0; k < i; ++k) x += (float) w[k]; return { x, 3.0f, (float) w[i], (float) getHeight() - 6.0f }; }
    int at (juce::Point<float> p) const { for (int i = 0; i < labels.size(); ++i) if (rectFor (i).contains (p)) return i; return -1; }
    juce::StringArray labels; juce::Array<int> w; int sel = 0, hot = -1;
};

// ---------------------------------------------------------------------------------- orbe do oscilador
class OrbDisplay : public juce::Component
{
public:
    explicit OrbDisplay (SynthKeyProcessor& p) : proc (p), fft (kOrder), window ((size_t) kN, juce::dsp::WindowingFunction<float>::hann)
    { smooth.fill (-100.0f); live.fill (0.0f); setInterceptsMouseClicks (false, false); }
    void setMode (int m) { mode = m; repaint(); }              // 0 = motor A, 1 = motor B, 2 = ao vivo
    void update()
    {
        constexpr int B = SynthKeyProcessor::kScopeSize; const int w = proc.scopeWrite.load (std::memory_order_acquire);
        std::fill (data.begin(), data.end(), 0.0f);
        for (int i = 0; i < kN; ++i) data[(size_t) i] = proc.scopeBuf[(w - kN + i + B) & (B - 1)];
        // forma de onda ao vivo (disparo no cruzamento por zero)
        { int start = 0; for (int i = 8; i < kN / 2; ++i) if (data[(size_t) i - 1] <= 0.0f && data[(size_t) i] > 0.0f) { start = i; break; }
          float pk = 0.02f; for (int i = 0; i < 400; ++i) pk = std::max (pk, std::fabs (data[(size_t) (start + i)]));
          gain = gain * 0.85f + 0.15f * (0.9f / pk);
          for (int i = 0; i < kLive; ++i) live[(size_t) i] = data[(size_t) (start + i * 400 / kLive)] * gain; active = pk > 0.021f; }
        window.multiplyWithWindowingTable (data.data(), (size_t) kN);
        fft.performFrequencyOnlyForwardTransform (data.data());
        const float binHz = (float) (std::max (8000.0, proc.sampleRateNow()) / kN);
        for (int c = 0; c < kCols; ++c)
        {
            const float f0 = 20.0f * std::pow (1000.0f, (float) c / (float) kCols), f1 = 20.0f * std::pow (1000.0f, (float) (c + 1) / (float) kCols);
            const int b0 = std::max (1, (int) std::floor (f0 / binHz)), b1 = std::min (kN / 2 - 1, std::max (b0, (int) std::ceil (f1 / binHz)));
            float m = 0.0f; for (int b = b0; b <= b1; ++b) m = std::max (m, data[(size_t) b]);
            const float db = 20.0f * std::log10 (std::max (1.0e-6f, m / (kN * 0.25f)));
            smooth[(size_t) c] = db > smooth[(size_t) c] ? db : smooth[(size_t) c] * 0.88f + db * 0.12f;
        }
        repaint();
    }
    void paint (juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xFF1A1A22), b.getCentreX(), b.getCentreY(), juce::Colour (0xFF07070B), b.getX(), b.getY(), true)); g.fillRoundedRectangle (b, 9.0f);
        g.setColour (theme::stroke()); g.drawRoundedRectangle (b.reduced (0.5f), 9.0f, 1.0f);
        const float cx = b.getCentreX(), cy = b.getCentreY(), R = b.getHeight() * 0.5f - 9.0f;
        auto P2 = [&] (float r, float deg) { const float t = deg * juce::MathConstants<float>::pi / 180.0f; return juce::Point<float> (cx + r * std::sin (t), cy - r * std::cos (t)); };
        auto arcPath = [&] (float r, float a0, float a1) { juce::Path p; p.addCentredArc (cx, cy, r, r, 0.0f, a0 * juce::MathConstants<float>::pi / 180.0f, a1 * juce::MathConstants<float>::pi / 180.0f, true); return p; };

        // ---- dados do motor
        const int E = mode == 1 ? 1 : 0, o = E ? P_B_ON - P_A_ON : 0; const int type = (int) proc.getReal (P_A_TYPE + o);
        float arcVal = 0.5f; int posParam = -1;
        if (mode == 2) arcVal = juce::jlimit (0.0f, 1.0f, (proc.getReal (P_MASTER_VOL) + 60.0f) / 66.0f);
        else if (type == 1) { arcVal = proc.getReal (P_A_WTPOS + o); posParam = P_A_WTPOS + o; }
        else arcVal = juce::jlimit (0.0f, 1.0f, (proc.getReal (P_A_LEVEL + o) + 60.0f) / 66.0f);

        // ---- anéis
        g.setColour (juce::Colours::white.withAlpha (0.05f)); g.drawEllipse (cx - R, cy - R, 2 * R, 2 * R, 1.0f);
        g.setColour (juce::Colour (0xFF2A2C33)); g.strokePath (arcPath (R, -150.0f, 150.0f), juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        if (arcVal > 0.004f) theme::glowStroke (g, arcPath (R, -150.0f, -150.0f + 300.0f * arcVal), theme::accent(), 3.0f);
        if (posParam >= 0) { float lo = 0, hi = 0; if (modRangeFor (proc, posParam, lo, hi)) { const float a = juce::jlimit (0.0f, 1.0f, arcVal + lo), c = juce::jlimit (0.0f, 1.0f, arcVal + hi); if (c - a > 0.004f) theme::glowStroke (g, arcPath (R + 5.0f, -150.0f + 300.0f * a, -150.0f + 300.0f * c), theme::accent2(), 2.5f); } }
        // ---- disco
        const float rd = R - 9.0f;
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xFF23252C), cx, cy - rd * 0.2f, juce::Colour (0xFF08090B), cx, cy + rd, true)); g.fillEllipse (cx - rd, cy - rd, 2 * rd, 2 * rd);
        g.setColour (juce::Colour (0xFF32353D)); g.drawEllipse (cx - rd, cy - rd, 2 * rd, 2 * rd, 1.6f);
        // ---- espectro radial (espelhado)
        const float r0 = rd * 0.52f, maxLen = rd * 0.42f; juce::Path bars, bars2; const int nb = kCols * 2;
        for (int i = 0; i < nb; ++i)
        {
            const int c = i < kCols ? i : nb - 1 - i; const float v = juce::jlimit (0.0f, 1.0f, (smooth[(size_t) c] + 84.0f) / 70.0f); const float len = 2.0f + maxLen * v;
            const float deg = (float) i * 360.0f / (float) nb; bars.startNewSubPath (P2 (r0, deg)); bars.lineTo (P2 (r0 + len, deg));
            if (len > 8.0f) { bars2.startNewSubPath (P2 (r0 + len + 2.0f, deg)); bars2.lineTo (P2 (r0 + len + 2.0f + len * 0.14f, deg)); }
        }
        g.setColour (theme::accent2().withAlpha (0.9f)); g.strokePath (bars, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (theme::accent().withAlpha (0.5f)); g.strokePath (bars2, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        // ---- núcleo + forma de onda
        const float rc = r0 - 5.0f;
        g.setColour (juce::Colour (0xFF0B0B10)); g.fillEllipse (cx - rc, cy - rc, 2 * rc, 2 * rc); g.setColour (juce::Colour (0xFF30333B)); g.drawEllipse (cx - rc, cy - rc, 2 * rc, 2 * rc, 1.3f);
        juce::Path w; const int pts = 64; const float ww = rc * 1.55f, amp = rc * 0.5f;
        for (int i = 0; i < pts; ++i)
        {
            const float ph = (float) i / (float) (pts - 1); float v = 0.0f;
            if (mode == 2 || type == 2) v = live[(size_t) std::min (kLive - 1, (int) (ph * (kLive - 1)))];
            else if (type == 1)
            {
                const int tab = juce::jlimit (0, Wavetables::kTables, (int) proc.getReal (P_A_WTTAB + o)); const int fr = juce::jlimit (0, Wavetables::kFrames - 1, (int) std::lround (proc.getReal (P_A_WTPOS + o) * (Wavetables::kFrames - 1)));
                const float* cyc = (tab >= Wavetables::kUserTable) ? proc.userWavetable().cycle (fr, 0) : Wavetables::get().cycle (tab, fr, 0);
                v = cyc ? juce::jlimit (-1.2f, 1.2f, Wavetables::read (cyc, ph)) : 0.0f;
            }
            else
            {
                const int wv = (int) proc.getReal (P_A_WAVE + o); const float pw = proc.getReal (P_A_PWM + o);
                v = wv == 0 ? std::sin (juce::MathConstants<float>::twoPi * ph) : (wv == 1 ? (ph < 0.5f ? 4.0f * ph - 1.0f : 3.0f - 4.0f * ph) : (wv == 2 ? 1.0f - 2.0f * ph : (ph < pw ? 1.0f : -1.0f)));
            }
            const float x = cx - ww * 0.5f + ww * ph, y = cy - v * amp; if (i == 0) w.startNewSubPath (x, y); else w.lineTo (x, y);
        }
        theme::glowStroke (g, w, juce::Colour (0xFFFFE3B0).interpolatedWith (theme::accent(), 0.15f), 2.0f);
        g.setColour (juce::Colours::white.withAlpha (0.06f)); g.fillEllipse (cx - rd * 0.8f, cy - rd * 0.95f, rd * 1.2f, rd * 0.45f);

        // ---- textos nos cantos
        g.setFont (theme::font (9.5f, true)); g.setColour (theme::dim());
        juce::String l1, v1, l2, v2;
        if (mode == 2) { l1 = TX ("MESTRE"); v1 = juce::String (proc.getReal (P_MASTER_VOL), 1) + " dB"; l2 = TX ("QUALIDADE"); v2 = juce::StringArray::fromTokens (TX (kParams[P_QUALITY].choices), "|", "")[(int) proc.getReal (P_QUALITY)]; }
        else if (type == 1) { l1 = TX ("POSI\xC3\x87\xC3\x83O"); v1 = juce::String ((int) std::lround (arcVal * 100.0f)) + "%"; l2 = "FRAME"; v2 = juce::String (1 + (int) std::lround (arcVal * (Wavetables::kFrames - 1))) + " / " + juce::String (Wavetables::kFrames); }
        else if (type == 2) { l1 = "SAMPLE"; v1 = proc.sampleDisplayName (E); l2 = TX ("N\xC3\x8DVEL"); v2 = juce::String (proc.getReal (P_A_LEVEL + o), 1) + " dB"; }
        else { l1 = "ONDA"; v1 = juce::StringArray::fromTokens (TX (kParams[P_A_WAVE].choices), "|", "")[(int) proc.getReal (P_A_WAVE + o)]; l2 = "UNISSON"; v2 = juce::String ((int) proc.getReal (P_A_UNI + o)); }
        auto lbl = [&] (juce::Rectangle<int> r, const juce::String& a, const juce::String& bb, juce::Justification j)
        { g.setColour (theme::dim()); g.setFont (theme::font (9.0f, true)); g.drawText (a, r.withHeight (11), j); g.setColour (juce::Colours::white); g.setFont (theme::font (12.0f, true)); g.drawFittedText (bb, r.withTrimmedTop (11).withHeight (15), j, 1, 0.75f); };
        lbl ({ 12, 10, 84, 26 }, l1, v1, juce::Justification::centredLeft); lbl ({ 12, 40, 84, 26 }, l2, v2, juce::Justification::centredLeft);
        // modulação que chega a este motor
        g.setColour (theme::dim()); g.setFont (theme::font (9.0f, true)); g.drawText (TX ("MODULA\xC3\x87\xC3\x83O"), getWidth() - 100, 10, 90, 11, juce::Justification::centredRight);
        int line = 0;
        if (mode != 2)
            for (int s = 0; s < kNumModSlots && line < 3; ++s)
            {
                const int bs = modSlotBase (s); const int src = (int) proc.getReal (bs), dst = (int) proc.getReal (bs + 1); const float amt = proc.getReal (bs + 2);
                if (src == 0 || dst == 0 || std::fabs (amt) < 0.005f || ! dstBelongs (dst, E)) continue;
                g.setColour (srcColour (src)); g.setFont (theme::font (10.5f)); g.drawFittedText (juce::String (juce::CharPointer_UTF8 ("\xE2\x97\x8F ")) + shortSrcName (src) + " \xC2\xB7 " + (amt >= 0 ? "+" : "") + juce::String ((int) std::lround (amt * 100.0f)) + "%", getWidth() - 110, 22 + line * 14, 100, 14, juce::Justification::centredRight, 1, 0.8f); ++line;
            }
        g.setColour (theme::dim()); g.setFont (theme::font (9.5f, true)); g.drawText (juce::String (proc.voiceCount.load()) + (proc.voiceCount.load() == 1 ? " VOZ" : " VOZES"), getWidth() - 100, getHeight() - 20, 90, 12, juce::Justification::centredRight);
        if (! active && (mode == 2 || type == 2)) { g.setColour (theme::dim()); g.setFont (theme::font (10.5f)); g.drawText (TX ("toca uma nota\xE2\x80\xA6"), 0, getHeight() - 20, getWidth(), 12, juce::Justification::centred); }
    }
private:
    static bool dstBelongs (int dst, int E)
    {
        static const int A[] = { MD_PITCH_A, MD_LEVEL_A, MD_WTPOS_A, MD_WARP_A, MD_UNIDET_A, MD_PWM_A, MD_FM_A, MD_START_A, MD_FOLD_A };
        static const int Bn[] = { MD_PITCH_B, MD_LEVEL_B, MD_WTPOS_B, MD_WARP_B, MD_UNIDET_B, MD_PWM_B, MD_FM_B, MD_START_B, MD_FOLD_B };
        for (int d : (E ? Bn : A)) if (d == dst) return true; return false;
    }
    static constexpr int kOrder = 11, kN = 1 << kOrder, kCols = 60, kLive = 64;
    SynthKeyProcessor& proc; juce::dsp::FFT fft; juce::dsp::WindowingFunction<float> window;
    std::array<float, 2 * kN> data {}; std::array<float, kCols> smooth {}; std::array<float, kLive> live {}; float gain = 1.0f; bool active = false; int mode = 0;
};

// ---------------------------------------------------------------------------------- barra de origens (esquerda)
class OrbitSourceRail : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit OrbitSourceRail (SynthKeyProcessor& p) : proc (p) { setTooltip (TX ("Arrasta uma origem para um bot\xC3\xA3o (ou para a matriz) para a ligar")); }
    static constexpr int kTileH = 70, kTilePitch = 74;
    void update()
    {
        int sig = modMatrixSignature (proc);
        for (int i : { P_LFO1_WAVE, P_LFO1_RATE, P_LFO1_SYNC, P_LFO1_DIV, P_LFO2_WAVE, P_LFO2_RATE, P_LFO2_SYNC, P_LFO2_DIV, P_ENV2_A, P_ENV2_D, P_ENV2_S, P_ENV2_R, P_ENV3_A, P_ENV3_D, P_ENV3_S, P_ENV3_R, P_STEP_LEN, P_STEP_DIV, P_MENV_N, P_MENV_LOOP })
            sig = sig * 31 + (int) std::lround (proc.getReal (i) * 100.0f);
        for (int i = 0; i < 16; ++i) sig = sig * 31 + (int) std::lround (proc.getReal (P_STEP_V1 + i) * 100.0f);
        for (int i = 0; i < 32; ++i) sig = sig * 31 + (int) std::lround (proc.getReal (P_DRAW_P1 + i) * 100.0f);
        for (int i = 0; i < 16; ++i) sig = sig * 31 + (int) std::lround (proc.getReal (P_MENV_X1 + i) * 100.0f);
        if (sig != lastSig) { lastSig = sig; repaint(); }
    }
    void paint (juce::Graphics& g) override
    {
        for (int i = 0; i < 6; ++i) paintTile (g, i);
        g.setColour (theme::dim()); g.setFont (theme::font (9.5f, true)); g.drawText ("OUTRAS ORIGENS", 2, chipsTop() - 17, 200, 12, juce::Justification::centredLeft);
        for (int j = 0; j < (int) chips.size(); ++j)
        {
            const auto r = chipRect (j).toFloat(); const bool on = hot == 100 + j; const bool used = isUsed (chips[(size_t) j].src);
            g.setColour (on ? juce::Colour (0xFF22222F) : juce::Colour (0xFF13131C)); g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
            g.setColour (used ? theme::accent().withAlpha (0.8f) : (on ? theme::accent2().withAlpha (0.7f) : theme::stroke())); g.drawRoundedRectangle (r.reduced (0.5f), r.getHeight() * 0.5f, 1.0f);
            g.setColour (used ? theme::accent() : theme::dim()); g.fillEllipse (r.getX() + 9, r.getCentreY() - 3.5f, 7, 7);
            g.setColour (on || used ? theme::text() : theme::dim().brighter (0.3f)); g.setFont (theme::font (10.0f, true)); g.drawText (chips[(size_t) j].label, r.toNearestInt().withTrimmedLeft (14), juce::Justification::centred);
        }
    }
    void mouseMove (const juce::MouseEvent& e) override { const int h = indexAt (e.getPosition()); if (h != hot) { hot = h; repaint(); } }
    void mouseExit (const juce::MouseEvent&) override { hot = -1; repaint(); }
    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (e.getDistanceFromDragStart() < 6) return;
        auto* container = juce::DragAndDropContainer::findParentDragContainerFor (this); if (! container || container->isDragAndDropActive()) return;
        const int i = indexAt (e.getMouseDownPosition()); if (i < 0) return;
        const int src = i < 100 ? tiles[(size_t) i].src : chips[(size_t) (i - 100)].src; const juce::String lab = i < 100 ? tiles[(size_t) i].label : chips[(size_t) (i - 100)].label;
        juce::Image img (juce::Image::ARGB, 72, 24, true); { juce::Graphics g (img); g.setColour (srcColour (src).darker (0.2f).withAlpha (0.92f)); g.fillRoundedRectangle (0, 0, 72, 24, 12.0f);
            g.setColour (juce::Colours::white); g.setFont (theme::font (12.0f, true)); g.drawText (lab, 0, 0, 72, 24, juce::Justification::centred); }
        container->startDragging (juce::String (kModDragPrefix) + juce::String (src), this, juce::ScaledImage (img), false);
    }
private:
    struct Src { int src; juce::String label; };
    std::vector<Src> tiles { { MS_LFO1, "LFO 1" }, { MS_LFO2, "LFO 2" }, { MS_ENV2, "ENV 2" }, { MS_STEP, "STEP" }, { MS_DRAW, "DESENHO" }, { MS_MENV, "ENV MULTI" } };
    std::vector<Src> chips { { MS_VEL, "VELOC." }, { MS_KEY, "TECLA" }, { MS_PRESSURE, "PRESS." }, { MS_SLIDE, "SLIDE" }, { MS_RANDOM, "ALEAT." }, { MS_PITCHBEND, "BEND" },
                             { MS_MODWHEEL, "MOD" }, { MS_AFTERTOUCH, "AFTER" }, { MS_ENV3, "ENV 3" } };
    int chipsTop() const { return 6 * kTilePitch + 18; }
    juce::Rectangle<int> tileRect (int i) const { return { 0, i * kTilePitch, getWidth(), kTileH }; }
    juce::Rectangle<int> chipRect (int j) const { const int w = (getWidth() - 16) / 3; return { (j % 3) * (w + 8), chipsTop() + (j / 3) * 30, w, 26 }; }
    int indexAt (juce::Point<int> p) const { for (int i = 0; i < 6; ++i) if (tileRect (i).contains (p)) return i; for (int j = 0; j < (int) chips.size(); ++j) if (chipRect (j).contains (p)) return 100 + j; return -1; }
    bool isUsed (int src) const
    {
        for (int s = 0; s < kNumModSlots; ++s) { const int b = modSlotBase (s); if ((int) proc.getReal (b) == src && (int) proc.getReal (b + 1) != 0 && std::fabs (proc.getReal (b + 2)) > 0.004f) return true; }
        return false;
    }
    juce::String subText (int i) const
    {
        auto names = [] (int idx) { return juce::StringArray::fromTokens (TX (kParams[idx].choices), "|", ""); };
        switch (i)
        {
            case 0: case 1: { const int o = i ? P_LFO2_WAVE - P_LFO1_WAVE : 0; const auto w = names (P_LFO1_WAVE)[(int) proc.getReal (P_LFO1_WAVE + o)];
                return proc.getReal (P_LFO1_SYNC + o) > 0.5f ? juce::String (kDivNames[juce::jlimit (0, 15, (int) proc.getReal (P_LFO1_DIV + o))]) + " \xC2\xB7 " + w : formatValue (kParams[P_LFO1_RATE], proc.getReal (P_LFO1_RATE + o)) + " \xC2\xB7 " + w; }
            case 2: return TX ("filtro 1");
            case 3: return juce::String ((int) proc.getReal (P_STEP_LEN)) + " passos \xC2\xB7 " + kDivNames[juce::jlimit (0, 15, (int) proc.getReal (P_STEP_DIV))];
            case 4: return "32 pontos";
            default: return juce::String ((int) proc.getReal (P_MENV_N)) + " pontos" + (proc.getReal (P_MENV_LOOP) > 0.5f ? " \xC2\xB7 loop" : "");
        }
    }
    void paintTile (juce::Graphics& g, int i)
    {
        const auto r = tileRect (i).toFloat(); const auto& t = tiles[(size_t) i]; const auto c = srcColour (t.src); const bool used = isUsed (t.src), on = hot == i;
        if (used) { g.setColour (c.withAlpha (0.10f)); g.fillRoundedRectangle (r.expanded (2.0f), 13.0f); }
        g.setColour (used ? juce::Colour (0xFF13131C).overlaidWith (c.withAlpha (0.12f)) : (on ? juce::Colour (0xFF1A1A26) : juce::Colour (0xFF13131C))); g.fillRoundedRectangle (r, 11.0f);
        g.setColour (used ? c : (on ? c.withAlpha (0.6f) : theme::stroke())); g.drawRoundedRectangle (r.reduced (0.5f), 11.0f, used ? 1.4f : 1.0f);
        g.setColour (c); g.fillRoundedRectangle (r.getX(), r.getY() + 10, 3.5f, r.getHeight() - 20, 1.7f);
        g.setFont (theme::font (10.5f, true)); g.drawText (t.label, (int) r.getX() + 15, (int) r.getY() + 7, 100, 14, juce::Justification::centredLeft);
        g.setColour (theme::dim()); g.setFont (theme::font (9.5f)); g.drawFittedText (subText (i), (int) r.getX() + 15, (int) r.getY() + 22, (int) r.getWidth() - 46, 12, juce::Justification::centredLeft, 1, 0.8f);
        for (int k = 0; k < 6; ++k) { g.setColour (juce::Colour (0xFF7D838F)); g.fillEllipse (r.getRight() - 18 + (float) (k % 2) * 5.0f, r.getY() + 8 + (float) (k / 2) * 5.0f, 2.6f, 2.6f); }
        paintMini (g, i, juce::Rectangle<float> (r.getX() + 14, r.getY() + 36, r.getWidth() - 30, r.getHeight() - 42), c);
    }
    void paintMini (juce::Graphics& g, int i, juce::Rectangle<float> a, juce::Colour c)
    {
        juce::Path p; const float W = a.getWidth(), H = a.getHeight();
        auto X = [&] (float u) { return a.getX() + W * u; }; auto Y = [&] (float v) { return a.getBottom() - H * v; };      // v 0..1
        auto stroke = [&] (const juce::Path& pp, bool fill) {
            if (fill) { juce::Path f (pp); f.lineTo (a.getRight(), a.getBottom()); f.lineTo (a.getX(), a.getBottom()); f.closeSubPath(); g.setColour (c.withAlpha (0.14f)); g.fillPath (f); }
            g.setColour (c); g.strokePath (pp, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded)); };
        switch (i)
        {
            case 0: case 1:
            {
                const int o = i ? P_LFO2_WAVE - P_LFO1_WAVE : 0; const int wv = (int) proc.getReal (P_LFO1_WAVE + o); const int N = 96; const float cyc = 2.0f;
                auto h = [] (int k) { unsigned x = (unsigned) k * 2654435761u; x ^= x >> 15; x *= 2246822519u; x ^= x >> 13; return (float) (x & 0xffff) / 65535.0f; };
                for (int k = 0; k <= N; ++k)
                {
                    const float u = (float) k / N, ph = std::fmod (u * cyc, 1.0f); float v = 0.5f;
                    switch (wv) { case 0: v = 0.5f + 0.5f * std::sin (juce::MathConstants<float>::twoPi * ph); break; case 1: v = ph < 0.5f ? ph * 2 : 2 - ph * 2; break; case 2: v = ph; break; case 3: v = 1 - ph; break;
                                  case 4: v = ph < 0.5f ? 1.0f : 0.0f; break; case 5: v = h ((int) (u * cyc * 4)); break;
                                  default: { const float t = u * cyc * 3; const int k0 = (int) t; const float f = t - (float) k0; const float s = f * f * (3 - 2 * f); v = h (k0) * (1 - s) + h (k0 + 1) * s; } }
                    const float x = X (u), y = Y (0.08f + 0.84f * v); if (k == 0) p.startNewSubPath (x, y); else p.lineTo (x, y);
                }
                stroke (p, true); break;
            }
            case 2:
            {
                const int b = P_ENV2_A; auto seg = [] (float ms) { return std::pow (std::max (ms, 0.1f) / 15000.0f, 0.4f); };
                const float wa = seg (proc.getReal (b)), wd = seg (proc.getReal (b + 1)), wr = seg (proc.getReal (b + 3)), ws = 0.25f, tot = wa + wd + ws + wr, s = proc.getReal (b + 2);
                p.startNewSubPath (X (0), Y (0.04f)); p.lineTo (X (wa / tot), Y (0.95f)); p.lineTo (X ((wa + wd) / tot), Y (0.04f + 0.9f * s)); p.lineTo (X ((wa + wd + ws) / tot), Y (0.04f + 0.9f * s)); p.lineTo (X (1), Y (0.04f));
                stroke (p, true); break;
            }
            case 3:
            {
                const int len = juce::jlimit (1, 16, (int) proc.getReal (P_STEP_LEN)); const float cw = W / 16.0f;
                for (int k = 0; k < 16; ++k)
                {
                    const float v = proc.getReal (P_STEP_V1 + k); const float mid = a.getCentreY(), y = mid - v * H * 0.5f; auto bar = juce::Rectangle<float> (a.getX() + cw * (float) k + 1.5f, std::min (y, mid), cw - 3.0f, std::max (1.5f, std::fabs (y - mid)));
                    g.setColour (k < len ? c.withAlpha (0.9f) : c.withAlpha (0.18f)); g.fillRoundedRectangle (bar, 1.5f);
                }
                break;
            }
            case 4:
            {
                for (int k = 0; k < 32; ++k) { const float v = proc.getReal (P_DRAW_P1 + k); const float x = X ((float) k / 31.0f), y = a.getCentreY() - v * H * 0.5f; if (k == 0) p.startNewSubPath (x, y); else p.lineTo (x, y); }
                stroke (p, false); break;
            }
            default:
            {
                const int N = juce::jlimit (2, 8, (int) proc.getReal (P_MENV_N));
                for (int k = 0; k < N; ++k) { const float x = X (k == 0 ? 0.0f : proc.getReal (P_MENV_X1 + 2 * k)), y = Y (0.04f + 0.9f * proc.getReal (P_MENV_Y1 + 2 * k)); if (k == 0) p.startNewSubPath (x, y); else p.lineTo (x, y); }
                stroke (p, true); break;
            }
        }
    }
    SynthKeyProcessor& proc; int hot = -1, lastSig = 0;
};

// ---------------------------------------------------------------------------------- matriz compacta (8 ligações)
class MatrixMini : public juce::Component
{
public:
    static constexpr int kRows = 8, kRowH = 42;
    explicit MatrixMini (SynthKeyProcessor& p) : proc (p) {}
    int activeCount() const { int n = 0; for (int s = 0; s < kNumModSlots; ++s) { const int b = modSlotBase (s); if ((int) proc.getReal (b) != 0 && (int) proc.getReal (b + 1) != 0) ++n; } return n; }
    void update() { const int sig = modMatrixSignature (proc); if (sig != lastSig) { lastSig = sig; repaint(); } }
    void paint (juce::Graphics& g) override
    {
        for (int i = 0; i < kRows; ++i)
        {
            const int b = modSlotBase (i); const int src = (int) proc.getReal (b), dst = (int) proc.getReal (b + 1); const float amt = proc.getReal (b + 2);
            const bool on = src != 0 && dst != 0; const auto c = srcColour (src); auto r = rowRect (i).toFloat();
            g.setColour (on ? juce::Colour (0xFF16151F) : juce::Colour (0xFF101016)); g.fillRoundedRectangle (r, 8.0f);
            g.setColour (on ? c.withAlpha (0.35f) : juce::Colour (0xFF1E1E28)); g.drawRoundedRectangle (r.reduced (0.5f), 8.0f, 1.0f);
            auto chip = chipRect (i).toFloat();
            g.setColour (juce::Colour (0xFF0A0A10)); g.fillRoundedRectangle (chip, chip.getHeight() * 0.5f); g.setColour (on ? c.withAlpha (0.5f) : juce::Colour (0xFF2A2A36)); g.drawRoundedRectangle (chip.reduced (0.5f), chip.getHeight() * 0.5f, 1.0f);
            g.setColour (c); g.fillEllipse (chip.getX() + 7, chip.getCentreY() - 3, 6, 6);
            g.setColour (src ? juce::Colours::white.withAlpha (0.92f) : theme::dim()); g.setFont (theme::font (9.5f, true)); g.drawText (shortSrcName (src), chip.toNearestInt().withTrimmedLeft (12), juce::Justification::centred);
            g.setColour (dst ? juce::Colour (0xFFE3E6EC) : theme::dim()); g.setFont (theme::font (11.0f, dst != 0)); g.drawFittedText (dst ? TX (kModDstNames[dst]) : TX ("destino\xE2\x80\xA6"), dstRect (i), juce::Justification::centredLeft, 1, 0.75f);
            g.setColour (on ? c : theme::dim()); g.setFont (theme::font (11.0f, true)); g.drawText ((amt >= 0 ? "+" : "") + juce::String ((int) std::lround (amt * 100.0f)) + "%", r.toNearestInt().removeFromRight (44).translated (-6, -8), juce::Justification::centredRight);
            // barra bipolar
            auto bar = barRect (i); g.setColour (juce::Colour (0xFF26262E)); g.fillRoundedRectangle (bar, 2.0f);
            const float mid = bar.getCentreX(), px = mid + amt * bar.getWidth() * 0.5f;
            if (on) { g.setColour (c); g.fillRoundedRectangle (juce::Rectangle<float> (std::min (mid, px), bar.getY(), std::max (1.5f, std::abs (px - mid)), bar.getHeight()), 2.0f); }
            g.setColour (juce::Colour (0xFF4A4D57)); g.fillRect (mid - 0.5f, bar.getY() - 3, 1.0f, bar.getHeight() + 6);
            g.setColour (juce::Colours::white); g.fillEllipse (px - 5, bar.getCentreY() - 5, 10, 10); g.setColour (on ? c : juce::Colour (0xFF4A4D57)); g.drawEllipse (px - 5, bar.getCentreY() - 5, 10, 10, 2.0f);
        }
    }
    void mouseDown (const juce::MouseEvent& e) override
    {
        const int i = rowAt (e.getPosition()); if (i < 0) return; const int b = modSlotBase (i); dragRow = -1;
        if (e.mods.isPopupMenu()) { proc.setReal (b, 0); proc.setReal (b + 1, 0); proc.setReal (b + 2, 0); repaint(); return; }
        if (chipRect (i).contains (e.getPosition())) pickMenu (i, true);
        else if (dstRect (i).contains (e.getPosition())) pickMenu (i, false);
        else if (barHit (i).contains (e.position)) { dragRow = i; setAmt (i, e.position.x); }
    }
    void mouseDrag (const juce::MouseEvent& e) override { if (dragRow >= 0) setAmt (dragRow, e.position.x); }
    void mouseUp (const juce::MouseEvent&) override { dragRow = -1; }
    void mouseDoubleClick (const juce::MouseEvent& e) override { const int i = rowAt (e.getPosition()); if (i >= 0 && barHit (i).contains (e.position)) { proc.setReal (modSlotBase (i) + 2, 0.0f); repaint(); } }
private:
    juce::Rectangle<int> rowRect (int i) const { return { 0, i * kRowH, getWidth(), kRowH - 4 }; }
    juce::Rectangle<int> chipRect (int i) const { return { 6, i * kRowH + 5, 68, 18 }; }
    juce::Rectangle<int> dstRect (int i) const { return { 80, i * kRowH + 4, getWidth() - 80 - 46, 20 }; }
    juce::Rectangle<float> barRect (int i) const { return { 10.0f, (float) (i * kRowH + 31), (float) getWidth() - 20.0f, 4.0f }; }
    juce::Rectangle<float> barHit (int i) const { return barRect (i).expanded (0.0f, 7.0f); }
    int rowAt (juce::Point<int> p) const { const int i = p.y / kRowH; return i >= 0 && i < kRows && rowRect (i).contains (p) ? i : -1; }
    void setAmt (int i, float x)
    {
        auto bar = barRect (i); float v = (x - bar.getCentreX()) / (bar.getWidth() * 0.5f); v = juce::jlimit (-1.0f, 1.0f, std::round (v * 100.0f) / 100.0f); if (std::abs (v) < 0.03f) v = 0.0f;
        proc.setReal (modSlotBase (i) + 2, v); repaint();
    }
    void pickMenu (int i, bool source)
    {
        const int b = modSlotBase (i); const int cur = (int) proc.getReal (b + (source ? 0 : 1)); juce::PopupMenu m; const int n = source ? MS_COUNT : MD_COUNT;
        for (int k = 0; k < n; ++k) m.addItem (k + 1, TX (source ? kModSrcNames[k] : kModDstNames[k]), true, k == cur);
        auto area = (source ? chipRect (i) : dstRect (i)); m.showMenuAsync (juce::PopupMenu::Options().withTargetScreenArea (localAreaToGlobal (area)).withMinimumWidth (160),
            [this, b, source, i] (int r) { if (r > 0) { proc.setReal (b + (source ? 0 : 1), (float) (r - 1)); if (source && r - 1 != 0 && std::fabs (proc.getReal (b + 2)) < 0.004f) proc.setReal (b + 2, 0.3f); repaint(); } (void) i; });
    }
    SynthKeyProcessor& proc; int lastSig = 0, dragRow = -1;
};

// ---------------------------------------------------------------------------------- resumo dos efeitos (9 blocos)
class FxSummary : public juce::Component
{
public:
    std::function<void()> onClick;
    explicit FxSummary (SynthKeyProcessor& p) : proc (p) { setMouseCursor (juce::MouseCursor::PointingHandCursor); setTooltip2 (TX ("Abrir os efeitos")); }
    void update() { int sig = 0; for (int s = 0; s < kNumFxSlots; ++s) sig = sig * 13 + (int) proc.getReal (fxSlotBase (s)); if (sig != lastSig) { lastSig = sig; repaint(); } }
    void paint (juce::Graphics& g) override
    {
        static const char* sh[12] = { "", "EQ", "COM", "SAT", "CHO", "DLY", "REV", "FAS", "BIT", "LAR", "TRE", "DLY" };
        static const char* grp[3] = { "INSERT A", "INSERT B", "AUX" };
        const int bw = 20, gap = 3, gg = 10; int x = 0;
        for (int gi = 0; gi < 3; ++gi)
        {
            g.setColour (theme::dim()); g.setFont (theme::font (8.5f, true)); g.drawText (grp[gi], x, 0, 3 * (bw + gap), 11, juce::Justification::centredLeft);
            for (int k = 0; k < 3; ++k)
            {
                const int t = juce::jlimit (0, 11, (int) proc.getReal (fxSlotBase (gi * 3 + k))); const bool on = t != 0; auto r = juce::Rectangle<float> ((float) x, 15.0f, (float) bw, 38.0f);
                const auto c = gi == 2 ? theme::accent2() : theme::accent();
                if (on) { g.setColour (c.withAlpha (0.28f)); g.fillRoundedRectangle (r.expanded (2.0f), 8.0f); }
                g.setColour (on ? juce::Colour (0xFF14131C).overlaidWith (c.withAlpha (0.20f)) : juce::Colour (0xFF0E0E14)); g.fillRoundedRectangle (r, 6.0f);
                g.setColour (on ? c : juce::Colour (0xFF2A2A36)); g.drawRoundedRectangle (r.reduced (0.5f), 6.0f, 1.0f);
                g.setColour (on ? juce::Colours::white : juce::Colour (0xFF55556A)); g.setFont (theme::font (8.5f, true)); g.drawText (on ? sh[t] : "-", r.toNearestInt(), juce::Justification::centred);
                x += bw + gap;
            }
            x += gg;
        }
    }
    void mouseDown (const juce::MouseEvent&) override { if (onClick) onClick(); }
private:
    void setTooltip2 (const juce::String&) {}
    SynthKeyProcessor& proc; int lastSig = -1;
};

// ---------------------------------------------------------------------------------- arpejador em miniatura
class ArpMini : public juce::Component
{
public:
    std::function<void()> onClick;
    explicit ArpMini (SynthKeyProcessor& p) : proc (p) { setMouseCursor (juce::MouseCursor::PointingHandCursor); }
    void update() { int sig = (int) proc.getReal (P_ARP_ON) * 7 + (int) proc.getReal (P_ARP_PATTERN) * 31 + (int) proc.getReal (P_ARP_EDIT) * 131 + (int) proc.getReal (P_ARP_LEN); for (int i = 0; i < 48; ++i) sig = sig * 5 + (int) std::lround (proc.getReal (P_ARP_V1 + i) * 20.0f); if (sig != lastSig) { lastSig = sig; repaint(); } }
    void paint (juce::Graphics& g) override
    {
        const bool on = proc.getReal (P_ARP_ON) > 0.5f, edit = proc.getReal (P_ARP_EDIT) > 0.5f; const int len = edit ? juce::jlimit (1, 16, (int) proc.getReal (P_ARP_LEN)) : 16;
        const auto& pat = kArpPatterns[juce::jlimit (0, 7, (int) proc.getReal (P_ARP_PATTERN))];
        const float cw = (float) getWidth() / 16.0f, H = (float) getHeight();
        for (int i = 0; i < 16; ++i)
        {
            ArpStep st = pat.s[i % pat.len];
            if (edit) { const int b = P_ARP_V1 + 3 * i; st.vel = proc.getReal (b); const int rr = (int) std::lround (proc.getReal (b + 1 + 1)); st.flags = st.vel < 0.02f ? AF_REST : (rr >= 2 ? AF_R3 : (rr == 1 ? AF_R2 : 0)); st.oct = (int8_t) std::lround (proc.getReal (b + 1)); }
            const bool off = edit && i >= len; const bool rest = (st.flags & (AF_REST | AF_TIE)) != 0;
            auto cell = juce::Rectangle<float> (cw * (float) i + 2.0f, 0, cw - 4.0f, H);
            g.setColour (juce::Colour (0xFF0C0C12)); g.fillRoundedRectangle (cell, 3.0f);
            if (off || rest) { g.setColour (theme::dim().withAlpha (0.3f)); g.fillRect (cell.getX() + 3, cell.getBottom() - 4, cell.getWidth() - 6, 2.0f); continue; }
            const float h = std::max (4.0f, H * st.vel);
            auto bar = cell.withTop (cell.getBottom() - h).reduced (1.5f, 0);
            g.setGradientFill (juce::ColourGradient ((on ? theme::accent() : theme::dim()).withAlpha (on ? 1.0f : 0.5f), bar.getX(), bar.getY(), (on ? theme::accent2() : theme::dim()).withAlpha (on ? 0.7f : 0.3f), bar.getX(), bar.getBottom(), false));
            g.fillRoundedRectangle (bar, 2.5f);
        }
    }
    void mouseDown (const juce::MouseEvent&) override { if (onClick) onClick(); }
private:
    SynthKeyProcessor& proc; int lastSig = -1;
};
} // namespace ui
