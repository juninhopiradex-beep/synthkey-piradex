// =============================================================================
//  SYNTHKEY by Piradex — página VISUAL: osciloscópio, espectro, curva dos filtros e wavetable em 3D
// =============================================================================
#pragma once
#include <complex>
#include <juce_dsp/juce_dsp.h>
#include "Pages1.h"
#include "../Core/Wavetables.h"

namespace ui
{
// ----------------------------------------------------------------------------- base dos painéis
class VizPanel : public juce::Component
{
public:
    explicit VizPanel (const juce::String& t) : title (t) {}
    virtual void update() { repaint(); }
protected:
    juce::Rectangle<float> frame (juce::Graphics& g) const
    {
        theme::drawCard (g, getLocalBounds().toFloat());
        g.setColour (theme::dim()); g.setFont (theme::font (11.5f, true)); g.drawText (title, 14, 8, getWidth() - 28, 16, juce::Justification::centredLeft);
        return getLocalBounds().toFloat().withTrimmedTop (30.0f).reduced (12.0f, 6.0f).withTrimmedBottom (4.0f);
    }
    juce::String title;
};

// ----------------------------------------------------------------------------- osciloscópio
class ScopeView : public VizPanel
{
public:
    explicit ScopeView (SynthKeyProcessor& p) : VizPanel (TX ("OSCILOSC\xC3\x93PIO")), proc (p) {}
    void update() override
    {
        constexpr int N = SynthKeyProcessor::kScopeSize, kWin = 2048, kShow = 1024;
        const int w = proc.scopeWrite.load (std::memory_order_acquire);
        float tmp[kWin]; for (int i = 0; i < kWin; ++i) tmp[i] = proc.scopeBuf[(w - kWin + i + N) & (N - 1)];
        // disparo: passagem ascendente por zero na primeira metade da janela
        int start = 0; for (int i = 8; i < kWin - kShow; ++i) if (tmp[i - 1] <= 0.0f && tmp[i] > 0.0f) { start = i; break; }
        float pk = 0.02f; for (int i = 0; i < kShow; ++i) pk = std::max (pk, std::fabs (tmp[start + i]));
        gain = gain * 0.85f + 0.15f * (0.9f / pk);
        for (int i = 0; i < kShow; ++i) shown[(size_t) i] = tmp[start + i] * gain;
        active = pk > 0.021f; repaint();
    }
    void paint (juce::Graphics& g) override
    {
        const auto r = frame (g);
        g.setColour (juce::Colour (0xFF0A0C18)); g.fillRoundedRectangle (r, 6.0f);
        g.setColour (juce::Colour (0x14FFFFFF));
        for (int i = 1; i < 4; ++i) g.drawHorizontalLine ((int) (r.getY() + r.getHeight() * i / 4.0f), r.getX(), r.getRight());
        for (int i = 1; i < 8; ++i) g.drawVerticalLine ((int) (r.getX() + r.getWidth() * i / 8.0f), r.getY(), r.getBottom());
        g.setColour (juce::Colour (0x30FFFFFF)); g.drawHorizontalLine ((int) r.getCentreY(), r.getX(), r.getRight());
        juce::Path p; const int n = (int) shown.size();
        for (int i = 0; i < n; ++i)
        {
            const float x = r.getX() + r.getWidth() * (float) i / (float) (n - 1), y = r.getCentreY() - juce::jlimit (-1.0f, 1.0f, shown[(size_t) i]) * r.getHeight() * 0.46f;
            if (i == 0) p.startNewSubPath (x, y); else p.lineTo (x, y);
        }
        theme::glowStroke (g, p, active ? theme::accent() : theme::accent().withAlpha (0.35f), 2.0f);
        if (! active) { g.setColour (theme::dim()); g.setFont (theme::font (12.0f)); g.drawText (TX ("toca uma nota\xE2\x80\xA6"), r.toNearestInt(), juce::Justification::centred); }
    }
private:
    SynthKeyProcessor& proc; std::array<float, 1024> shown {}; float gain = 1.0f; bool active = false;
};

// ----------------------------------------------------------------------------- espectro
class SpectrumView : public VizPanel
{
public:
    explicit SpectrumView (SynthKeyProcessor& p) : VizPanel ("ESPECTRO"), proc (p), fft (kOrder), window ((size_t) kN, juce::dsp::WindowingFunction<float>::hann) { smooth.fill (-100.0f); }
    void update() override
    {
        constexpr int B = SynthKeyProcessor::kScopeSize; const int w = proc.scopeWrite.load (std::memory_order_acquire);
        std::fill (data.begin(), data.end(), 0.0f);
        for (int i = 0; i < kN; ++i) data[(size_t) i] = proc.scopeBuf[(w - kN + i + B) & (B - 1)];
        window.multiplyWithWindowingTable (data.data(), (size_t) kN);
        fft.performFrequencyOnlyForwardTransform (data.data());
        const double sr = std::max (8000.0, proc.sampleRateNow()); const int cols = std::max (8, getWidth() - 24);
        cols_ = std::min (cols, (int) smooth.size());
        for (int c = 0; c < cols_; ++c)
        {
            const float f0 = 20.0f * std::pow (1000.0f, (float) c / (float) cols_), f1 = 20.0f * std::pow (1000.0f, (float) (c + 1) / (float) cols_);
            const float binHz = (float) (sr / kN);
            int b0 = std::max (1, (int) std::floor (f0 / binHz)), b1 = std::min (kN / 2 - 1, std::max (b0, (int) std::ceil (f1 / binHz)));
            float m = 0.0f; for (int b = b0; b <= b1; ++b) m = std::max (m, data[(size_t) b]);
            const float db = 20.0f * std::log10 (std::max (1.0e-6f, m / (kN * 0.25f)));
            smooth[(size_t) c] = db > smooth[(size_t) c] ? db : smooth[(size_t) c] * 0.86f + db * 0.14f;
        }
        repaint();
    }
    void paint (juce::Graphics& g) override
    {
        const auto r = frame (g);
        g.setColour (juce::Colour (0xFF0A0C18)); g.fillRoundedRectangle (r, 6.0f);
        g.setFont (theme::font (10.0f));
        for (float f : { 100.0f, 1000.0f, 10000.0f })
        {
            const float x = r.getX() + r.getWidth() * std::log10 (f / 20.0f) / 3.0f;
            g.setColour (juce::Colour (0x18FFFFFF)); g.drawVerticalLine ((int) x, r.getY(), r.getBottom());
            g.setColour (theme::dim()); g.drawText (f >= 1000 ? juce::String ((int) (f / 1000)) + "k" : juce::String ((int) f), (int) x + 3, (int) r.getBottom() - 14, 34, 12, juce::Justification::left);
        }
        for (int d : { -60, -40, -20 }) { const float y = r.getBottom() - r.getHeight() * (float) (d + 90) / 93.0f; g.setColour (juce::Colour (0x14FFFFFF)); g.drawHorizontalLine ((int) y, r.getX(), r.getRight()); }
        if (cols_ < 2) return;
        juce::Path line, fill; const float base = r.getBottom();
        for (int c = 0; c < cols_; ++c)
        {
            const float x = r.getX() + r.getWidth() * (float) c / (float) (cols_ - 1), y = base - r.getHeight() * juce::jlimit (0.0f, 1.0f, (smooth[(size_t) c] + 90.0f) / 93.0f);
            if (c == 0) { line.startNewSubPath (x, y); fill.startNewSubPath (x, base); fill.lineTo (x, y); } else { line.lineTo (x, y); fill.lineTo (x, y); }
        }
        fill.lineTo (r.getRight(), base); fill.closeSubPath();
        g.setGradientFill (juce::ColourGradient (theme::accent2().withAlpha (0.55f), 0, r.getY(), theme::accent().withAlpha (0.02f), 0, base, false)); g.fillPath (fill);
        theme::glowStroke (g, line, theme::accent2(), 1.6f);
    }
private:
    static constexpr int kOrder = 11, kN = 1 << kOrder;
    SynthKeyProcessor& proc; juce::dsp::FFT fft; juce::dsp::WindowingFunction<float> window;
    std::array<float, 2 * kN> data {}; std::array<float, 1600> smooth {}; int cols_ = 0;
};

// ----------------------------------------------------------------------------- curva dos filtros
class FilterCurveView : public VizPanel
{
public:
    explicit FilterCurveView (SynthKeyProcessor& p) : VizPanel ("FILTROS"), proc (p) {}
    void update() override
    {
        int sig = 5; for (int i : { P_F1_TYPE, P_F1_CUT, P_F1_RES, P_F1_DRIVE, P_F2_TYPE, P_F2_CUT, P_F2_RES, P_F2_DRIVE, P_FILT_ROUTE, P_FILT_BAL }) sig = sig * 31 + (int) std::lround (proc.getReal (i) * 1000.0f);
        if (sig != lastSig) { lastSig = sig; repaint(); }
    }
    // resposta em amplitude (linear) de um filtro; x = f / corte
    static float response (int type, float cut, float res, float f)
    {
        if (type <= 0) return 1.0f;
        using C = std::complex<float>;
        const float x = f / std::max (20.0f, cut), q = 0.6f + res * res * 14.0f;
        const C s (0.0f, x);
        auto lp2 = [&] (float qq) { return C (1.0f) / (s * s + s / qq + C (1.0f)); };
        switch (type)
        {
            case 1: return std::abs (lp2 (q));
            case 2: return std::abs (lp2 (q * 0.7f) * lp2 (0.55f + res * 2.0f));
            case 3: return std::abs (s * s * lp2 (q));
            case 4: return std::abs (s * s * lp2 (q * 0.7f) * s * s * lp2 (0.55f + res * 2.0f)) ;
            case 5: return std::abs ((s / q) * lp2 (q));
            case 6: return std::abs ((s * s + C (1.0f)) * lp2 (q * 0.8f));
            case 7: { const C p = C (1.0f) + s; const C p4 = p * p * p * p; return std::abs (C (1.0f) / (p4 + C (res * 3.95f))); }
            case 8: { const float g = 0.3f + res * 0.65f; return std::sqrt (std::max (1.0e-4f, 1.0f + g * g + 2.0f * g * std::cos (6.2831853f * f / std::max (20.0f, cut)))) / (1.0f + g) * 1.0f; }
            default: return 1.0f;
        }
    }
    void paint (juce::Graphics& g) override
    {
        const auto r = frame (g);
        g.setColour (juce::Colour (0xFF0A0C18)); g.fillRoundedRectangle (r, 6.0f);
        const float dbTop = 18.0f, dbBot = -42.0f;
        auto yOf = [&] (float db) { return r.getBottom() - r.getHeight() * (juce::jlimit (dbBot, dbTop, db) - dbBot) / (dbTop - dbBot); };
        g.setFont (theme::font (10.0f));
        for (float f : { 100.0f, 1000.0f, 10000.0f })
        {
            const float x = r.getX() + r.getWidth() * std::log10 (f / 20.0f) / 3.0f;
            g.setColour (juce::Colour (0x18FFFFFF)); g.drawVerticalLine ((int) x, r.getY(), r.getBottom());
            g.setColour (theme::dim()); g.drawText (f >= 1000 ? juce::String ((int) (f / 1000)) + "k" : juce::String ((int) f), (int) x + 3, (int) r.getBottom() - 14, 34, 12, juce::Justification::left);
        }
        for (int d : { 12, 0, -12, -24, -36 }) { g.setColour (d == 0 ? juce::Colour (0x30FFFFFF) : juce::Colour (0x14FFFFFF)); g.drawHorizontalLine ((int) yOf ((float) d), r.getX(), r.getRight()); g.setColour (theme::dim()); g.drawText (juce::String (d), (int) r.getX() + 3, (int) yOf ((float) d) - 12, 30, 12, juce::Justification::left); }
        const int t1 = (int) proc.getReal (P_F1_TYPE), t2 = (int) proc.getReal (P_F2_TYPE), route = (int) proc.getReal (P_FILT_ROUTE);
        const float c1 = proc.getReal (P_F1_CUT), c2 = proc.getReal (P_F2_CUT), r1 = proc.getReal (P_F1_RES), r2 = proc.getReal (P_F2_RES);
        const int cols = std::max (16, (int) r.getWidth() / 2);
        juce::Path p1, p2, pm, fillM; bool first = true;
        for (int i = 0; i < cols; ++i)
        {
            const float tt = (float) i / (float) (cols - 1), f = 20.0f * std::pow (1000.0f, tt), x = r.getX() + r.getWidth() * tt;
            const float h1 = response (t1, c1, r1, f), h2 = response (t2, c2, r2, f);
            const float hm = route == 0 ? h1 * h2 : 0.5f * (h1 + h2);
            auto db = [] (float h) { return 20.0f * std::log10 (std::max (1.0e-4f, h)); };
            if (i == 0) { p1.startNewSubPath (x, yOf (db (h1))); p2.startNewSubPath (x, yOf (db (h2))); pm.startNewSubPath (x, yOf (db (hm))); } else { p1.lineTo (x, yOf (db (h1))); p2.lineTo (x, yOf (db (h2))); pm.lineTo (x, yOf (db (hm))); }
            if (first) { fillM.startNewSubPath (x, r.getBottom()); first = false; }
            fillM.lineTo (x, yOf (db (hm)));
        }
        fillM.lineTo (r.getRight(), r.getBottom()); fillM.closeSubPath();
        g.setGradientFill (juce::ColourGradient (theme::accent().withAlpha (0.30f), 0, r.getY(), theme::accent().withAlpha (0.0f), 0, r.getBottom(), false)); g.fillPath (fillM);
        if (t1 > 0) { g.setColour (theme::accent().withAlpha (0.55f)); g.strokePath (p1, juce::PathStrokeType (1.2f)); }
        if (t2 > 0) { g.setColour (theme::accent2().withAlpha (0.65f)); g.strokePath (p2, juce::PathStrokeType (1.2f)); }
        theme::glowStroke (g, pm, juce::Colours::white, 2.0f);
        auto mark = [&] (int t, float cut, juce::Colour c, const juce::String& lbl)
        {
            if (t <= 0) return; const float x = r.getX() + r.getWidth() * std::log10 (cut / 20.0f) / 3.0f;
            g.setColour (c.withAlpha (0.8f)); g.drawVerticalLine ((int) x, r.getY() + 14, r.getBottom()); g.fillEllipse (x - 3.5f, r.getY() + 10.5f, 7.0f, 7.0f);
            g.setFont (theme::font (10.5f, true)); g.drawText (lbl, (int) x - 20, (int) r.getY() - 2, 40, 12, juce::Justification::centred);
        };
        mark (t1, c1, theme::accent(), "F1"); mark (t2, c2, theme::accent2(), "F2");
        if (t1 <= 0 && t2 <= 0) { g.setColour (theme::dim()); g.setFont (theme::font (12.0f)); g.drawText ("Filtros desligados", r.toNearestInt(), juce::Justification::centred); }
    }
private:
    SynthKeyProcessor& proc; int lastSig = 0;
};

// ----------------------------------------------------------------------------- wavetable em 3D
class WavetableView : public VizPanel
{
public:
    explicit WavetableView (SynthKeyProcessor& p) : VizPanel ("WAVETABLE"), proc (p)
    {
        for (int e = 0; e < 2; ++e)
        {
            auto* b = btn.add (new juce::TextButton (e ? "B" : "A")); addAndMakeVisible (b); b->setClickingTogglesState (false);
            b->onClick = [this, e] { forced = e; repaint(); };
        }
    }
    void resized() override { btn[0]->setBounds (getWidth() - 74, 5, 28, 22); btn[1]->setBounds (getWidth() - 42, 5, 28, 22); }
    void update() override
    {
        int sig = 3; for (int e = 0; e < 2; ++e) { const int o = e ? P_B_ON - P_A_ON : 0; sig = sig * 31 + (int) proc.getReal (P_A_TYPE + o) + 7 * (int) proc.getReal (P_A_WTTAB + o) + 131 * (int) std::lround (proc.getReal (P_A_WTPOS + o) * 500.0f) + 17 * (int) std::lround (proc.getReal (P_A_WARP + o) * 200.0f); }
        sig = sig * 31 + forced; if (sig != lastSig) { lastSig = sig; repaint(); }
    }
    void paint (juce::Graphics& g) override
    {
        const auto r = frame (g);
        g.setColour (juce::Colour (0xFF0A0C18)); g.fillRoundedRectangle (r, 6.0f);
        int E = forced; if (E < 0) E = proc.getReal (P_A_TYPE) == 1.0f ? 0 : (proc.getReal (P_B_TYPE) == 1.0f ? 1 : 0);
        for (int e = 0; e < 2; ++e) btn[e]->setColour (juce::TextButton::buttonColourId, e == E ? theme::accent().withAlpha (0.55f) : juce::Colour (0xFF1A1D30));
        const int o = E ? P_B_ON - P_A_ON : 0; const bool isWt = proc.getReal (P_A_TYPE + o) == 1.0f;
        const int tab = juce::jlimit (0, Wavetables::kTables - 1, (int) proc.getReal (P_A_WTTAB + o)); const float pos = proc.getReal (P_A_WTPOS + o);
        const auto names = juce::StringArray::fromTokens (TX (kParams[P_A_WTTAB].choices), "|", "");
        g.setColour (theme::text()); g.setFont (theme::font (12.0f, true));
        g.drawText (juce::String (E ? "Motor B" : "Motor A") + (isWt ? TX ("  \xC2\xB7  ") + (tab < names.size() ? names[tab] : juce::String()) : juce::String()), (int) r.getX() + 6, (int) r.getY() + 2, 360, 16, juce::Justification::left);
        if (! isWt)
        {
            g.setColour (theme::dim()); g.setFont (theme::font (12.0f)); g.drawText (TX ("Este motor n\xC3\xA3o est\xC3\xA1 em modo Wavetable"), r.toNearestInt(), juce::Justification::centred); return;
        }
        const auto& wt = Wavetables::get(); constexpr int F = Wavetables::kFrames, S = 140;
        const int hot = juce::jlimit (0, F - 1, (int) std::lround (pos * (F - 1)));
        const float areaX = r.getX() + 14.0f, areaW = r.getWidth() - 100.0f, topY = r.getY() + 34.0f, areaH = r.getHeight() - 52.0f;
        const float dx = 70.0f / F, dy = (areaH - 46.0f) / F, hAmp = 40.0f;
        for (int f = F - 1; f >= 0; --f)   // do fundo para a frente
        {
            const float ox = areaX + (F - 1 - f) * dx, oy = topY + 42.0f + (float) (F - 1 - f) * dy;   // frames altas ficam atrás
            const float* cyc = wt.cycle (tab, f, 0);
            juce::Path p, fillP;
            for (int i = 0; i < S; ++i)
            {
                const float ph = (float) i / (float) (S - 1), y = oy - juce::jlimit (-1.4f, 1.4f, Wavetables::read (cyc, ph)) * hAmp * 0.55f, x = ox + ph * (areaW - 60.0f);
                if (i == 0) { p.startNewSubPath (x, y); fillP.startNewSubPath (x, oy + 30.0f); fillP.lineTo (x, y); } else { p.lineTo (x, y); fillP.lineTo (x, y); }
            }
            fillP.lineTo (ox + (areaW - 60.0f), oy + 30.0f); fillP.closeSubPath();
            g.setColour (juce::Colour (0xFF0A0C18)); g.fillPath (fillP);       // esconde as linhas de trás
            if (f == hot) theme::glowStroke (g, p, theme::accent(), 2.0f);
            else { const float a = 0.25f + 0.45f * (1.0f - (float) f / (float) F); g.setColour (theme::accent2().withAlpha (a)); g.strokePath (p, juce::PathStrokeType (1.0f)); }
        }
    }
private:
    SynthKeyProcessor& proc; juce::OwnedArray<juce::TextButton> btn; int forced = -1, lastSig = 0;
};

// ===================================================================================== página
class VisualPage : public Page
{
public:
    explicit VisualPage (SynthKeyProcessor& p) : Page (p), scope (p), spec (p), filt (p), wtv (p)
    {
        for (VizPanel* v : { (VizPanel*) &scope, (VizPanel*) &spec, (VizPanel*) &filt, (VizPanel*) &wtv }) addAndMakeVisible (v);
        scope.setBounds (0, 0, 564, 218); spec.setBounds (572, 0, 584, 218); filt.setBounds (0, 226, 564, 226); wtv.setBounds (572, 226, 584, 226);
    }
    void tick() override { Page::tick(); scope.update(); spec.update(); filt.update(); wtv.update(); }
private:
    ScopeView scope; SpectrumView spec; FilterCurveView filt; WavetableView wtv;
};
}
