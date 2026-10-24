#include "PluginEditor.h"
#include "UI/Pages2.h"
#include "UI/Pages3.h"
#include "UI/Visual.h"
#include "UI/Pages4.h"
#include "UI/ImportUI.h"
#include "UI/ModDrag.h"
#include "UI/Help.h"
#include "UI/OrbitSom.h"
#include "Core/SoundGen.h"

using namespace ui;

namespace
{
// ------------------------------------------------------------------------- fundo (grafite com brilhos suaves)
class Backdrop : public juce::Component
{
public:
    Backdrop() { setInterceptsMouseClicks (false, false); setBufferedToImage (true); }
    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xFF16161E), 0, 0, juce::Colour (0xFF08080C), 0, r.getHeight(), false)); g.fillRect (r);
        g.setGradientFill (juce::ColourGradient (theme::accent().withAlpha (0.10f), r.getWidth() * 0.12f, 0, juce::Colours::transparentBlack, r.getWidth() * 0.12f, r.getHeight() * 0.55f, true)); g.fillRect (r);
        g.setGradientFill (juce::ColourGradient (theme::accent2().withAlpha (0.08f), r.getWidth() * 0.95f, r.getHeight(), juce::Colours::transparentBlack, r.getWidth() * 0.95f, r.getHeight() * 0.40f, true)); g.fillRect (r);
    }
};

class ModeBtn : public juce::Button
{
public:
    explicit ModeBtn (const juce::String& t) : juce::Button (t) {}
    void paintButton (juce::Graphics& g, bool hover, bool) override
    {
        const bool on = getToggleState(); auto r = getLocalBounds().toFloat().reduced (1.0f);
        g.setColour (on ? theme::accent().withAlpha (0.22f) : juce::Colour (0xFF13131C)); g.fillRoundedRectangle (r, 7.0f);
        g.setColour (on ? theme::accent() : (hover ? theme::dim() : theme::stroke())); g.drawRoundedRectangle (r, 7.0f, 1.0f);
        g.setFont (theme::font (10.0f, true)); g.setColour (on ? theme::accent().brighter (0.5f) : theme::dim()); g.drawText (getButtonText(), r.toNearestInt(), juce::Justification::centred);
    }
};

class PaletteDot : public juce::Component
{
public:
    PaletteDot (int i, std::function<void (int)> cb) : idx (i), onPick (std::move (cb)) { setMouseCursor (juce::MouseCursor::PointingHandCursor); }
    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced (2.5f); const bool sel = theme::paletteIndex() == idx;
        g.setGradientFill (juce::ColourGradient (juce::Colour (theme::kPalettes[idx].accent), r.getX(), r.getY(), juce::Colour (theme::kPalettes[idx].accent2), r.getRight(), r.getBottom(), false)); g.fillEllipse (r);
        if (sel) { g.setColour (juce::Colours::white); g.drawEllipse (r.expanded (2.0f), 1.5f); }
    }
    void mouseDown (const juce::MouseEvent&) override { onPick (idx); }
private:
    int idx; std::function<void (int)> onPick;
};

class Meter : public juce::Component
{
public:
    void set (float l, float r) { l = std::min (1.2f, l); r = std::min (1.2f, r); if (std::abs (l - L) > 0.004f || std::abs (r - R) > 0.004f) { L = l; R = r; repaint(); } }
    void paint (juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat(); const float bw = (b.getWidth() - 6) * 0.5f;
        for (int c = 0; c < 2; ++c)
        {
            auto r = juce::Rectangle<float> (b.getX() + (float) c * (bw + 6), b.getY(), bw, b.getHeight());
            g.setColour (juce::Colour (0xFF0B0B12)); g.fillRoundedRectangle (r, 3.0f);
            const float v = juce::jlimit (0.0f, 1.0f, juce::Decibels::gainToDecibels ((c ? R : L), -60.0f) / 60.0f + 1.0f); const float h = r.getHeight() * v;
            g.setGradientFill (juce::ColourGradient (v > 0.92f ? theme::bad() : theme::accent(), 0, r.getBottom() - h, theme::accent2(), 0, r.getBottom(), false));
            g.fillRoundedRectangle (r.withTop (r.getBottom() - h), 3.0f);
        }
    }
private:
    float L = 0, R = 0;
};

class Decor : public juce::Component
{
public:
    Decor() { setInterceptsMouseClicks (false, false); }
    std::function<void (juce::Graphics&)> fn;
    void paint (juce::Graphics& g) override { if (fn) fn (g); }
};

class Toast : public juce::Component
{
public:
    Toast() { setInterceptsMouseClicks (false, false); setVisible (false); }
    void show (const juce::String& t, bool err) { text = t; isErr = err; setVisible (true); toFront (false); repaint(); expiry = juce::Time::getMillisecondCounter() + 3800; }
    bool expired() const { return isVisible() && juce::Time::getMillisecondCounter() > expiry; }
    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat(); g.setColour (juce::Colour (0xF2101322)); g.fillRoundedRectangle (r, 12.0f);
        g.setColour (isErr ? theme::bad() : theme::accent()); g.drawRoundedRectangle (r.reduced (0.5f), 12.0f, 1.4f);
        g.setColour (theme::text()); g.setFont (theme::font (13.5f, true)); g.drawFittedText (text, getLocalBounds().reduced (14, 2), juce::Justification::centred, 2);
    }
private:
    juce::String text; bool isErr = false; juce::uint32 expiry = 0;
};
} // namespace

// ============================================================================================
//  Vista principal (coordenadas de desenho 1180 × 760)
//    0-46 barra de cima  |  52-640 conteúdo  |  648-724 teclado de performance  |  730-758 navegação
// ============================================================================================
class MainView : public juce::Component, public juce::DragAndDropContainer, private juce::Timer
{
public:
    explicit MainView (SynthKeyProcessor& p) : proc (p), keyboard (p.keyboardState), pitchWheel (p, true, juce::Colour (0xFF5FE0FF), "PITCH"), modWheel (p, false, juce::Colour (0xFFA58BFF), "MOD")
    {
        theme::paletteIndex() = juce::jlimit (0, 5, proc.getUiInt ("uiPalette2", 0));
        mode = juce::jlimit (1, 3, proc.getUiInt ("uiMode", 2));
        svc.toast = [this] (const juce::String& t, bool e) { toast.show (t, e); };
        ui::toastFn() = svc.toast;
        svc.prompt = [this] (const juce::String& t, const juce::String& i, const juce::String& init, const juce::String& ok, std::function<juce::String (const juce::String&)> cb) { prompt.open (t, i, init, ok, std::move (cb)); };

        addAndMakeVisible (backdrop); addAndMakeVisible (decor); decor.fn = [this] (juce::Graphics& g) { paintDecor (g); };
        // ---- barra de cima
        for (auto* b : { (juce::Component*) &undoBtn, (juce::Component*) &redoBtn, (juce::Component*) &prevBtn, (juce::Component*) &presetBtn, (juce::Component*) &nextBtn, (juce::Component*) &menuBtn, (juce::Component*) &orbBtn, (juce::Component*) &helpBtn }) addAndMakeVisible (b);
        undoBtn.setTooltip (TX ("Desfazer (Ctrl/Cmd+Z)")); redoBtn.setTooltip (TX ("Refazer (Ctrl/Cmd+Shift+Z)")); helpBtn.setTooltip (TX ("Ajuda e novidades"));
        undoBtn.onClick = [this] { doUndo (true); }; redoBtn.onClick = [this] { doUndo (false); };
        prevBtn.onClick = [this] { proc.stepPreset (-1); }; nextBtn.onClick = [this] { proc.stepPreset (1); };
        presetBtn.onClick = [this] { if (browser.isOpen()) browser.close(); else browser.open(); }; menuBtn.onClick = [this] { showToolsMenu(); };
        orbBtn.onClick = [this] { openDescribe(); }; orbBtn.setTooltip (TX ("Descreve o som em palavras e o SYNTHKEY cria o preset"));
        helpBtn.onClick = [this] { help.open(); };
        static const char* modeNames[3] = { "F\xC3\x81" "CIL", "AVAN\xC3\x87.", "EXPERT" };
        for (int i = 0; i < 3; ++i) { auto* b = new ModeBtn (TX (modeNames[i])); modeBtns.add (b); addAndMakeVisible (b); b->onClick = [this, i] { setMode (i + 1); }; }
        for (int i = 0; i < 6; ++i) { auto* d = new PaletteDot (i, [this] (int k) { setPalette (k); }); dots.add (d); addAndMakeVisible (d); }
        // ---- navegação (em baixo)
        static const char* tabNames[kNumTabs] = { "SOM", "MATRIZ", "MODULAD.", "EFEITOS", "ARP", "VISUAL", "GROOVES", "PACKS", "ADMIN" };
        for (int i = 0; i < kNumTabs; ++i) { auto* t = new NavBtn (TX (tabNames[i])); tabs.add (t); addAndMakeVisible (t); t->onClick = [this, i] { showTab (i); }; }
        addAndMakeVisible (logout); logout.setButtonText (TX ("SAIR")); logout.onClick = [this] { AccessManager::get().logout(); };
        // ---- páginas
        auto* som = new OrbitSomPage (proc); som->gotoTab = [this] (int t) { showTab (t); }; som->setInterceptsMouseClicks (false, true);
        pages.add (som); pages.add (new MatrixPage (proc)); pages.add (new ModulatorsPage (proc)); pages.add (new FxPage (proc)); pages.add (new ArpPage (proc)); pages.add (new VisualPage (proc)); pages.add (new GroovesPage (proc, svc)); pages.add (new PacksPage (proc, svc));
        for (auto* pg : pages) addAndMakeVisible (pg);
        // ---- macros + master
        static const char* macroNames[8] = { "BRILHO", "CORPO", "MOVIM.", "ESPAÇO", "LARGURA", "SUJIDADE", "BALANÇO", "DOBRA" };
        for (int i = 0; i < 8; ++i) { auto* k = new Knob (proc, P_MACRO1 + i, TX (macroNames[i])); macros.add (k); addAndMakeVisible (k); }
        master = std::make_unique<Knob> (proc, P_MASTER_VOL, TX ("VOLUME")); addAndMakeVisible (*master);
        addAndMakeVisible (meter);
        addAndMakeVisible (panic); panic.setButtonText ("PANIC"); panic.onClick = [this] { proc.keyboardState.allNotesOff (1); for (int n = 0; n < 128; ++n) proc.keyboardState.noteOff (1, n, 0.0f); };
        addAndMakeVisible (modStrip);
        // ---- teclado de performance
        addAndMakeVisible (pitchWheel); addAndMakeVisible (modWheel); pitchWheel.setBounds (26, 654, 30, 64); modWheel.setBounds (62, 654, 30, 64);
        addAndMakeVisible (keyboard); keyboard.setBounds (104, 654, 1048, 64); keyboard.setKeyWidth (1048.0f / 43.0f); keyboard.setLowestVisibleKey (24);
        keyboard.setColour (juce::MidiKeyboardComponent::keySeparatorLineColourId, juce::Colour (0xFF0F0E17));
        // ---- sobreposições
        addChildComponent (keyTip); keyTip.setSize (132, 54);
        addChildComponent (browser); browser.setBounds (0, 0, 1180, 760);
        addChildComponent (toast); toast.setBounds (340, 560, 500, 44);
        addChildComponent (prompt); prompt.setBounds (0, 0, 1180, 760);
        addChildComponent (help); help.setBounds (0, 0, 1180, 760);
        addChildComponent (login); login.setBounds (0, 0, 1180, 760); login.onUnlocked = [this] { syncAccess(); };

        backdrop.setBounds (0, 0, 1180, 760); decor.setBounds (0, 0, 1180, 760);
        proc.onPresetLoaded = [this] { refreshPresetPill(); };
        refreshPresetPill();
        setMode (mode); showTab (juce::jlimit (0, kTabAdmin - 1, proc.getUiInt ("uiTab", 0)));
        syncAccess(); startTimerHz (30);
        juce::MessageManager::callAsync ([sp = juce::Component::SafePointer<MainView> (this)] { if (sp != nullptr && ! sp->login.isVisible()) sp->help.openIfFirstRun(); });
    }
    ~MainView() override { proc.onPresetLoaded = nullptr; ui::toastFn() = nullptr; stopTimer(); }

    Services svc;
    static constexpr int kNumTabs = 9, kTabAdmin = 8;
    void paintDecor (juce::Graphics& g)
    {
        // ---- barra de cima
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xFF1B1B24), 0, 0, juce::Colour (0xFF121218), 0, 46, false)); g.fillRect (0, 0, 1180, 46);
        g.setColour (juce::Colour (0xFF22222B)); g.drawHorizontalLine (46, 0.0f, 1180.0f);
        {   // separador do logótipo
            juce::Path tab; tab.addRoundedRectangle (18.0f, -2.0f, 226.0f, 46.0f, 22.0f, 22.0f, false, false, true, true);
            g.setGradientFill (juce::ColourGradient (juce::Colour (0xFF272833), 0, 0, juce::Colour (0xFF17171E), 0, 44, false)); g.fillPath (tab);
            g.setColour (juce::Colour (0xFF30313B)); g.strokePath (tab, juce::PathStrokeType (1.0f));
            const float cx = 44.0f, cy = 22.0f;
            for (int k = 0; k < 2; ++k)
            {
                juce::Path o; o.addEllipse (cx - 11.0f, cy - 5.0f, 22.0f, 10.0f); o.applyTransform (juce::AffineTransform::rotation ((k ? 0.52f : -0.52f), cx, cy));
                g.setColour (k ? theme::accent2() : theme::accent()); g.strokePath (o, juce::PathStrokeType (1.6f));
            }
            g.setColour (theme::accent()); g.fillEllipse (cx - 3.2f, cy - 3.2f, 6.4f, 6.4f);
            g.setGradientFill (juce::ColourGradient (juce::Colours::white, 66, 0, theme::accent().brighter (0.7f), 200, 0, false));
            g.setFont (theme::font (18.0f, true).withExtraKerningFactor (0.16f)); g.drawText ("SYNTHKEY", 66, 6, 170, 22, juce::Justification::centredLeft);
            g.setColour (theme::dim()); g.setFont (theme::font (9.0f).withExtraKerningFactor (0.2f)); g.drawText (TX ("BY PIRADEX  \xC2\xB7  V" SK_VERSION_STRING), 67, 26, 170, 12, juce::Justification::centredLeft);
        }
        // ---- teclado (cartão)
        theme::drawCard (g, juce::Rectangle<float> (16.0f, 648.0f, 1148.0f, 76.0f), 12.0f);
        // ---- faixa de macros / volume (todos os separadores menos o SOM)
        if (currentTab != 0)
        {
            theme::drawCard (g, juce::Rectangle<float> (12.0f, 518.0f, 1156.0f, 122.0f));
            g.setColour (theme::accent()); g.fillEllipse (23.5f, 530.5f, 6.0f, 6.0f); g.setColour (theme::accent().withAlpha (0.25f)); g.fillEllipse (21.0f, 528.0f, 11.0f, 11.0f);
            g.setColour (theme::text().withAlpha (0.88f)); g.setFont (theme::font (10.5f, true)); g.drawText ("MACROS", 38, 525, 120, 18, juce::Justification::centredLeft);
            g.setColour (theme::stroke()); g.drawVerticalLine (546, 534.0f, 626.0f);
        }
        // ---- navegação
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xFF1B1B2B), 0, 730.0f, juce::Colour (0xFF0D0D14), 0, 758.0f, false)); g.fillRect (0, 730, 1180, 28); g.setColour (theme::accent().withAlpha (0.45f)); g.drawHorizontalLine (730, 0.0f, 1180.0f);
        g.setColour (theme::dim()); g.setFont (theme::font (10.0f, true));
        g.drawText (juce::String (proc.voiceCount.load()) + (proc.voiceCount.load() == 1 ? " VOZ" : " VOZES"), 800, 731, 90, 26, juce::Justification::centredRight);
        if (AccessManager::get().isUnlocked())
        {
            const auto role = AccessManager::get().roleText(); g.setFont (theme::font (10.5f, true));
            const int w = juce::GlyphArrangement::getStringWidthInt (g.getCurrentFont(), role) + 22; auto b = juce::Rectangle<float> (1086.0f - (float) w - 6.0f, 734.0f, (float) w, 20.0f);
            g.setColour (juce::Colour (0xAA0A0C18)); g.fillRoundedRectangle (b, 10.0f); g.setColour (theme::good()); g.drawRoundedRectangle (b, 10.0f, 1.0f); g.drawText (role, b.toNearestInt(), juce::Justification::centred);
        }
    }
    void paintOverChildren (juce::Graphics& g) override
    {
        if (proc.previewMode.load() && AccessManager::get().isUnlocked())
        {
            const bool ex = proc.previewExpired.load(); auto r = juce::Rectangle<float> (252, 49, 660, 22);
            g.setColour (ex ? juce::Colour (0xF0501020) : juce::Colour (0xF03A2A08)); g.fillRoundedRectangle (r, 8.0f); g.setColour (ex ? theme::bad() : theme::warn()); g.drawRoundedRectangle (r.reduced (0.5f), 8.0f, 1.2f);
            g.setFont (theme::font (12.0f, true));
            g.drawText (ex ? TX ("PR\xC3\x89-VISUALIZA\xC3\x87\xC3\x83O TERMINOU  \xC2\xB7  o som est\xC3\xA1 em sil\xC3\xAAncio. Insira a chave do pack no separador PACKS.")
                           : TX ("\xF0\x9F\x94\x92 PR\xC3\x89-VISUALIZA\xC3\x87\xC3\x83O DE PACK BLOQUEADO  \xC2\xB7  ") + juce::String (proc.previewSecondsLeft.load(), 0) + TX (" s restantes"), r.toNearestInt(), juce::Justification::centred);
        }
    }
private:
    void timerCallback() override
    {
        syncAccess(); proc.undoWatch();
        if (currentTab < pages.size()) { if (auto* pg = pages[currentTab]) pg->tick(); }
        if (currentTab == kTabAdmin && adminPage) adminPage->tick();
        if (toast.expired()) toast.setVisible (false);
        meter.set (proc.meterL.load(), proc.meterR.load());
        const int vc = proc.voiceCount.load(); const int pv = (proc.previewMode.load() ? 1 : 0) + (proc.previewExpired.load() ? 2 : 0) + ((int) proc.previewSecondsLeft.load() << 4);
        if (vc != lastVoices || pv != lastPreview || (++frame % 30) == 0) { lastVoices = vc; lastPreview = pv; decor.repaint(); repaint(); }
        if (presetBtn.getButtonText() != proc.currentPresetName() || proc.currentPresetIndex() != lastPresetIdx) refreshPresetPill();
        undoBtn.setEnabled (proc.canUndo()); redoBtn.setEnabled (proc.canRedo());
        // dica da nota
        int note = 0; float v = 0; juce::Rectangle<float> kr;
        if (keyboard.tipFor (note, v, kr))
        {
            keyTip.show (note, v); const int cx = keyboard.getX() + (int) kr.getCentreX();
            keyTip.setBounds (juce::jlimit (16, 1164 - 132, cx - 66), keyboard.getY() - 58, 132, 54); keyTip.toFront (false);
        }
        else keyTip.setVisible (false);
    }
    void refreshPresetPill()
    {
        presetBtn.setButtonText (proc.currentPresetName()); lastPresetIdx = proc.currentPresetIndex();
        const auto& l = proc.presetList(); const int i = proc.currentPresetIndex();
        presetBtn.tag = (i >= 0 && i < (int) l.size()) ? TX (l[(size_t) i].category.c_str()) : juce::String();
        presetBtn.repaint();
    }
    // desfazer / refazer (Ctrl/Cmd+Z, Ctrl/Cmd+Shift+Z, Ctrl+Y)
    void doUndo (bool undo)
    {
        const bool ok = undo ? proc.undo() : proc.redo();
        toast.show (ok ? TX (undo ? "Desfeito." : "Refeito.") : TX (undo ? "Nada para desfazer." : "Nada para refazer."), false);
        refreshPresetPill(); repaint();
    }
    bool keyPressed (const juce::KeyPress& k) override
    {
        const auto m = k.getModifiers();
        if (m.isCommandDown() || m.isCtrlDown())
        {
            const int c = k.getKeyCode();
            if (c == 'Z' || c == 'z') { doUndo (! m.isShiftDown()); return true; }
            if (c == 'Y' || c == 'y') { doUndo (false); return true; }
        }
        return false;
    }
    void syncAccess()
    {
        auto& acc = AccessManager::get(); const bool unlocked = acc.isUnlocked();
        if (login.isVisible() == unlocked) { login.setVisible (! unlocked); if (! unlocked) { login.toFront (true); login.focus(); } else { decor.repaint(); repaint(); keyboard.grabKeyboardFocus(); help.openIfFirstRun(); } }
        const bool admin = acc.isAdmin();
        if (admin != adminShown) { adminShown = admin; if (admin && ! adminPage) { adminPage.reset (new AdminPage (proc, svc)); addAndMakeVisible (*adminPage); adminPage->setBounds (12, 58, 1156, 452); adminPage->setMode (mode); adminPage->setVisible (currentTab == kTabAdmin); } layoutTabs(); if (! admin && currentTab == kTabAdmin) showTab (0); }
        if (unlocked != lastUnlocked) { lastUnlocked = unlocked; layoutTabs(); decor.repaint(); }
    }
    void layoutTabs()
    {
        static const int widths[kNumTabs] = { 84, 92, 104, 94, 74, 88, 98, 84, 80 };
        int x = 12; for (int i = 0; i < kNumTabs; ++i) { const bool vis = i < kTabAdmin || adminShown; tabs[i]->setVisible (vis); if (vis) { tabs[i]->setBounds (x, 731, widths[i], 27); x += widths[i]; } }
        logout.setBounds (1096, 734, 64, 20); logout.setVisible (AccessManager::get().isUnlocked());
    }
    // macros / volume / origens: no SOM ficam nos cartões à direita; nos outros separadores, na faixa de baixo
    void layoutBottom()
    {
        if (currentTab == 0)
        {
            for (int i = 0; i < 8; ++i) macros[i]->setBounds (934 + (i % 4) * 57, 466 + (i / 4) * 52, 56, 52);
            master->setBounds (1000, 585, 64, 54); meter.setBounds (1072, 590, 24, 44); panic.setBounds (1104, 600, 54, 26); modStrip.setVisible (false);
        }
        else
        {
            for (int i = 0; i < 8; ++i) macros[i]->setBounds (24 + i * 64, 544, 62, 76);
            master->setBounds (562, 544, 70, 76); meter.setBounds (640, 552, 30, 64); panic.setBounds (1066, 590, 86, 26);
            modStrip.setBounds (700, 542, 340, 88); modStrip.setVisible (true);
        }
    }
    void showTab (int i)
    {
        if (i == kTabAdmin && ! adminPage) i = 0;
        currentTab = i; proc.setUiInt ("uiTab", std::min (i, kTabAdmin - 1));
        for (int k = 0; k < kNumTabs; ++k) tabs[k]->setToggleState (k == i, juce::dontSendNotification);
        for (int k = 0; k < pages.size(); ++k) { pages[k]->setBounds (k == 0 ? juce::Rectangle<int> (0, 0, 1180, 640) : juce::Rectangle<int> (12, 58, 1156, 452)); pages[k]->setVisible (k == i); }
        if (adminPage) adminPage->setVisible (i == kTabAdmin);
        layoutBottom();
        if (auto* pg = i == kTabAdmin ? static_cast<Page*> (adminPage.get()) : pages[i]) pg->tick();
        decor.repaint(); repaint();
    }
    void setMode (int m)
    {
        mode = m; proc.setUiInt ("uiMode", m); for (int i = 0; i < 3; ++i) modeBtns[i]->setToggleState (i + 1 == m, juce::dontSendNotification);
        for (auto* pg : pages) pg->setMode (m); if (adminPage) adminPage->setMode (m);
    }
    void setPalette (int i) { theme::paletteIndex() = i; proc.setUiInt ("uiPalette2", i); backdrop.repaint(); for (auto* d : dots) d->repaint(); decor.repaint(); repaint(); }

    void openDescribe()
    {
        prompt.open (TX ("\xE2\x9C\xA6 Descrever som"), TX ("Ex.: baixo escuro e agressivo \xC2\xB7 pad quente e largo \xC2\xB7 pluck brilhante curto \xC2\xB7 808 drill \xC2\xB7 amapiano log drum"), juce::String(), TX ("CRIAR"),
            [this] (const juce::String& txt) -> juce::String
            {
                const auto pr = sk::soundgen::generateCalibrated (txt.trim().toStdString(), describeSeed++);
                proc.loadPreset (pr, -1); toast.show (TX ("Criado: ") + TX (pr.name.c_str()) + TX ("  \xC2\xB7 carrega outra vez em CRIAR para variantes"), false); return {};
            });
    }
    void showToolsMenu()
    {
        juce::PopupMenu m; m.addItem (6, TX ("Desfazer   (Ctrl/Cmd+Z)"), proc.canUndo()); m.addItem (7, TX ("Refazer   (Ctrl/Cmd+Shift+Z)"), proc.canRedo()); m.addSeparator();
        m.addItem (1, TX ("Guardar preset\xE2\x80\xA6")); m.addItem (2, TX ("Preset vazio (Init)")); m.addItem (3, TX ("Aleatorizar timbre (suave)")); m.addItem (4, TX ("Aleatorizar timbre (forte)"));
        m.addItem (8, TX ("\xE2\x9C\xA6 Descrever som\xE2\x80\xA6")); m.addItem (9, TX ("Ajuda e novidades"));
        m.addSeparator(); m.addItem (5, TX ("Abrir pasta de presets do utilizador"));
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&menuBtn), [this] (int r)
        {
            if (r == 1) prompt.open (TX ("Guardar preset"), TX ("Aparece em \"Utilizador\". Etiquetas opcionais depois de # \xE2\x80\x94 ex.: Meu Lead #lead, brilhante, agressivo"), proc.currentPresetName() == "Init" ? juce::String() : proc.currentPresetName(), TX ("GUARDAR"),
                [this] (const juce::String& txt) -> juce::String
                {
                    const auto hash = txt.indexOfChar ('#'); const auto n = (hash >= 0 ? txt.substring (0, hash) : txt).trim();
                    juce::StringArray tags; if (hash >= 0) tags.addTokens (txt.substring (hash + 1).replaceCharacter ('#', ','), ",", "");
                    juce::String err; if (! proc.saveUserPreset (n, err, tags)) return err;
                    toast.show (TX ("Preset guardado.") + (tags.size() ? TX ("  (com etiquetas)") : juce::String()), false); return {};
                });
            else if (r == 2) proc.initPreset(); else if (r == 3) proc.randomizePatch (0.25f); else if (r == 4) proc.randomizePatch (0.7f);
            else if (r == 5) PackHub::userPresetFolder().revealToUser();
            else if (r == 6) doUndo (true); else if (r == 7) doUndo (false); else if (r == 8) openDescribe(); else if (r == 9) help.open();
        });
    }

    SynthKeyProcessor& proc; int mode = 2, currentTab = 0, lastVoices = -1, lastPreview = -1, frame = 0, lastPresetIdx = -2; bool adminShown = false, lastUnlocked = false;
    Backdrop backdrop; Decor decor; IconBtn undoBtn { IconBtn::Undo }, redoBtn { IconBtn::Redo }, prevBtn { IconBtn::Prev }, nextBtn { IconBtn::Next }, menuBtn { IconBtn::Menu }, helpBtn { IconBtn::Help };
    PresetPill presetBtn; OrbButton orbBtn; juce::TextButton logout, panic;
    juce::OwnedArray<ModeBtn> modeBtns; juce::OwnedArray<PaletteDot> dots; juce::OwnedArray<NavBtn> tabs; juce::OwnedArray<Page> pages; std::unique_ptr<AdminPage> adminPage;
    juce::OwnedArray<Knob> macros; std::unique_ptr<Knob> master; Meter meter; ModSourceStrip modStrip; PerfKeyboard keyboard; WheelStrip pitchWheel, modWheel; KeyTip keyTip;
    Toast toast; PromptOverlay prompt; HelpOverlay help; LoginOverlay login; unsigned describeSeed = 1;
    PresetBrowser browser { proc, [this] (const juce::String& t, bool e) { toast.show (t, e); } };
public:
    void layoutHeader()
    {
        undoBtn.setBounds (258, 8, 32, 30); redoBtn.setBounds (292, 8, 32, 30);
        prevBtn.setBounds (336, 8, 28, 30); presetBtn.setBounds (368, 8, 264, 30); nextBtn.setBounds (636, 8, 28, 30); menuBtn.setBounds (668, 8, 34, 30);
        orbBtn.setBounds (712, 2, 176, 42); helpBtn.setBounds (890, 8, 30, 30);
        for (int i = 0; i < 3; ++i) modeBtns[i]->setBounds (928 + i * 50, 11, 48, 24);
        for (int i = 0; i < 6; ++i) dots[i]->setBounds (1082 + i * 14, 16, 14, 14);
        layoutTabs();
    }
    void installFiles (const juce::StringArray& files)
    {
        for (auto& f : files) { const auto err = PackHub::get().install (juce::File (f)); toast.show (err.isEmpty() ? TX ("Pack instalado: ") + juce::File (f).getFileName() : err, err.isNotEmpty()); }
    }
};

// ============================================================================================
SynthKeyEditor::SynthKeyEditor (SynthKeyProcessor& p) : AudioProcessorEditor (p), proc (p)
{
    setLookAndFeel (&look);
    view = std::make_unique<MainView> (p); addAndMakeVisible (*view); view->layoutHeader();
    setResizable (true, true); setResizeLimits (kW / 2, kH / 2, kW * 2, kH * 2);
    if (auto* c = getConstrainer()) c->setFixedAspectRatio ((double) kW / (double) kH);
    const int w = juce::jlimit (kW / 2, kW * 2, p.getUiInt ("uiW", kW));
    setSize (w, (int) std::lround ((double) w * kH / kW));
    ready = true;
}
SynthKeyEditor::~SynthKeyEditor() { setLookAndFeel (nullptr); view.reset(); }
void SynthKeyEditor::paint (juce::Graphics& g) { g.fillAll (theme::bg()); }
void SynthKeyEditor::resized()
{
    const float s = (float) getWidth() / (float) kW;
    view->setTransform (juce::AffineTransform::scale (s)); view->setBounds (0, 0, kW, kH);
    if (ready && getWidth() >= kW / 2) proc.setUiInt ("uiW", getWidth());
}
bool SynthKeyEditor::isInterestedInFileDrag (const juce::StringArray& files) { for (auto& f : files) if (f.endsWithIgnoreCase (".bfpack") || useraudio::isAudioFile (juce::File (f))) return true; return false; }
void SynthKeyEditor::filesDropped (const juce::StringArray& files, int, int)
{
    juce::StringArray packs; juce::File audio;
    for (auto& f : files) { if (f.endsWithIgnoreCase (".bfpack")) packs.add (f); else if (audio == juce::File() && useraudio::isAudioFile (juce::File (f))) audio = juce::File (f); }
    if (packs.size()) view->installFiles (packs);
    if (audio != juce::File()) ui::showDropMenu (proc, this, audio);
}
