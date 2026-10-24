#include "PluginEditor.h"
#include "UI/Pages2.h"
#include "UI/Pages3.h"
#include "UI/Visual.h"
#include "UI/Pages4.h"
#include "UI/ImportUI.h"
#include "UI/ModDrag.h"
#include "UI/Help.h"
#include "Core/SoundGen.h"
#include "BinaryData.h"

using namespace ui;

namespace
{
// ------------------------------------------------------------------------- fundo "Liquid Chrome"
class Backdrop : public juce::Component
{
public:
    Backdrop() { setInterceptsMouseClicks (false, false); setBufferedToImage (true);
        bg = juce::ImageFileFormat::loadFrom (BinaryData::liquid_chrome_jpg, (size_t) BinaryData::liquid_chrome_jpgSize); }
    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        g.fillAll (theme::bg());
        if (bg.isValid())
        {
            const float sc = std::max (r.getWidth() / (float) bg.getWidth(), r.getHeight() / (float) bg.getHeight());
            const float w = (float) bg.getWidth() * sc, h = (float) bg.getHeight() * sc;
            g.setOpacity (0.55f); g.drawImage (bg, juce::Rectangle<float> ((r.getWidth() - w) * 0.5f, (r.getHeight() - h) * 0.5f, w, h), juce::RectanglePlacement::stretchToFit); g.setOpacity (1.0f);
        }
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xE6060712), 0, 0, juce::Colour (0xB3080A16), 0, r.getHeight(), false)); g.fillRect (r);
        g.setGradientFill (juce::ColourGradient (theme::accent().withAlpha (0.20f), r.getWidth() * 0.08f, 0, juce::Colours::transparentBlack, r.getWidth() * 0.08f, r.getHeight() * 0.62f, true)); g.fillRect (r);
        g.setGradientFill (juce::ColourGradient (theme::accent2().withAlpha (0.18f), r.getWidth() * 0.95f, r.getHeight(), juce::Colours::transparentBlack, r.getWidth() * 0.95f, r.getHeight() * 0.35f, true)); g.fillRect (r);
        g.setColour (juce::Colours::white.withAlpha (0.05f)); g.drawHorizontalLine (100, 0.0f, r.getWidth()); g.drawHorizontalLine (562, 0.0f, r.getWidth());
    }
private:
    juce::Image bg;
};

class TabBtn : public juce::Button
{
public:
    explicit TabBtn (const juce::String& t) : juce::Button (t) { setClickingTogglesState (false); }
    void paintButton (juce::Graphics& g, bool hover, bool) override
    {
        const bool on = getToggleState(); auto r = getLocalBounds().toFloat();
        if (on) { g.setGradientFill (juce::ColourGradient (theme::accent().withAlpha (0.20f), 0, 0, juce::Colours::transparentBlack, 0, r.getHeight(), false)); g.fillRoundedRectangle (r.reduced (1, 0), 8.0f); }
        g.setFont (theme::font (13.5f, true)); g.setColour (on ? theme::accent() : (hover ? theme::text() : theme::dim())); g.drawText (getButtonText(), r.toNearestInt(), juce::Justification::centred);
        if (on) { juce::Path p; p.startNewSubPath (r.getX() + 14, r.getBottom() - 2); p.lineTo (r.getRight() - 14, r.getBottom() - 2); theme::glowStroke (g, p, theme::accent(), 2.0f); }
    }
};

class ModeBtn : public juce::Button
{
public:
    explicit ModeBtn (const juce::String& t) : juce::Button (t) {}
    void paintButton (juce::Graphics& g, bool hover, bool) override
    {
        const bool on = getToggleState(); auto r = getLocalBounds().toFloat().reduced (1.0f);
        g.setColour (on ? theme::accent().withAlpha (0.22f) : juce::Colour (0xFF14172A)); g.fillRoundedRectangle (r, 7.0f);
        g.setColour (on ? theme::accent() : (hover ? theme::dim() : theme::stroke())); g.drawRoundedRectangle (r, 7.0f, 1.0f);
        g.setFont (theme::font (11.5f, true)); g.setColour (on ? theme::accent() : theme::dim()); g.drawText (getButtonText(), r.toNearestInt(), juce::Justification::centred);
    }
};

class PaletteDot : public juce::Component
{
public:
    PaletteDot (int i, std::function<void (int)> cb) : idx (i), onPick (std::move (cb)) { setMouseCursor (juce::MouseCursor::PointingHandCursor); }
    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced (4.0f); const bool sel = theme::paletteIndex() == idx;
        g.setGradientFill (juce::ColourGradient (juce::Colour (theme::kPalettes[idx].accent), r.getX(), r.getY(), juce::Colour (theme::kPalettes[idx].accent2), r.getRight(), r.getBottom(), false)); g.fillEllipse (r);
        if (sel) { g.setColour (juce::Colours::white); g.drawEllipse (r.expanded (3.0f), 1.6f); }
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
            g.setColour (juce::Colour (0xFF0B0D1A)); g.fillRoundedRectangle (r, 3.0f);
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
// ============================================================================================
class MainView : public juce::Component, public juce::DragAndDropContainer, private juce::Timer
{
public:
    explicit MainView (SynthKeyProcessor& p) : proc (p), keyboard (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
    {
        theme::paletteIndex() = juce::jlimit (0, 4, proc.getUiInt ("uiPalette", 0));
        mode = juce::jlimit (1, 3, proc.getUiInt ("uiMode", 2));
        svc.toast = [this] (const juce::String& t, bool e) { toast.show (t, e); };
        ui::toastFn() = svc.toast;
        svc.prompt = [this] (const juce::String& t, const juce::String& i, const juce::String& init, const juce::String& ok, std::function<juce::String (const juce::String&)> cb) { prompt.open (t, i, init, ok, std::move (cb)); };

        addAndMakeVisible (backdrop); addAndMakeVisible (decor); decor.fn = [this] (juce::Graphics& g) { paintDecor (g); };
        // ---- cabeçalho
        for (auto* b : { &prevBtn, &nextBtn, &menuBtn, &presetBtn }) addAndMakeVisible (b);
        prevBtn.setButtonText (TX ("\xE2\x97\x80")); nextBtn.setButtonText (TX ("\xE2\x96\xB6")); menuBtn.setButtonText (TX ("\xE2\x8B\xAF"));
        prevBtn.onClick = [this] { proc.stepPreset (-1); }; nextBtn.onClick = [this] { proc.stepPreset (1); };
        presetBtn.onClick = [this] { if (browser.isOpen()) browser.close(); else browser.open(); }; menuBtn.onClick = [this] { showToolsMenu(); };
        static const char* modeNames[3] = { "F\xC3\x81" "CIL", "AVAN\xC3\x87" "ADO", "EXPERT" };
        for (int i = 0; i < 3; ++i) { auto* b = new ModeBtn (TX (modeNames[i])); modeBtns.add (b); addAndMakeVisible (b); b->onClick = [this, i] { setMode (i + 1); }; }
        for (int i = 0; i < 5; ++i) { auto* d = new PaletteDot (i, [this] (int k) { setPalette (k); }); dots.add (d); addAndMakeVisible (d); }
        addAndMakeVisible (logout); logout.setButtonText (TX ("SAIR")); logout.onClick = [this] { AccessManager::get().logout(); };
        // ---- separadores
        static const char* tabNames[kNumTabs] = { "SOM", "MATRIZ", "MODULAD.", "EFEITOS", "ARP", "VISUAL", "GROOVES", "PACKS", "ADMIN" };
        for (int i = 0; i < kNumTabs; ++i) { auto* t = new TabBtn (TX (tabNames[i])); tabs.add (t); addAndMakeVisible (t); t->onClick = [this, i] { showTab (i); }; }
        // ---- páginas
        pages.add (new SomPage (proc)); pages.add (new MatrixPage (proc)); pages.add (new ModulatorsPage (proc)); pages.add (new FxPage (proc)); pages.add (new ArpPage (proc)); pages.add (new VisualPage (proc)); pages.add (new GroovesPage (proc, svc)); pages.add (new PacksPage (proc, svc));
        for (auto* pg : pages) { addAndMakeVisible (pg); pg->setBounds (12, 108, 1156, 452); }
        // ---- macros + master
        static const char* macroNames[8] = { "BRILHO", "CORPO", "MOVIM.", "ESPAÇO", "LARGURA", "SUJIDADE", "BALANÇO", "DOBRA" };
        for (int i = 0; i < 8; ++i) { auto* k = new Knob (proc, P_MACRO1 + i, TX (macroNames[i])); macros.add (k); addAndMakeVisible (k); k->setBounds (20 + i * 72, 578, 70, 76); }
        master = std::make_unique<Knob> (proc, P_MASTER_VOL, TX ("VOLUME")); addAndMakeVisible (*master); master->setBounds (640, 578, 70, 76);
        addAndMakeVisible (meter); meter.setBounds (722, 584, 30, 64);
        addAndMakeVisible (panic); panic.setButtonText ("PANIC"); panic.onClick = [this] { proc.keyboardState.allNotesOff (1); for (int n = 0; n < 128; ++n) proc.keyboardState.noteOff (1, n, 0.0f); };
        panic.setBounds (1080, 604, 80, 26);
        addAndMakeVisible (describeBtn); describeBtn.setButtonText (TX ("\xE2\x9C\xA6 DESCREVER")); describeBtn.onClick = [this] { openDescribe(); };
        describeBtn.setBounds (1076, 574, 88, 26); describeBtn.setTooltip (TX ("Descreve o som em palavras e o SYNTHKEY cria o preset"));
        addAndMakeVisible (helpBtn); helpBtn.setButtonText (TX ("? AJUDA")); helpBtn.onClick = [this] { help.open(); }; helpBtn.setBounds (1080, 633, 80, 24);
        addAndMakeVisible (modStrip); modStrip.setBounds (790, 574, 276, 80);
        // ---- teclado
        addAndMakeVisible (keyboard); keyboard.setBounds (12, 662, 1156, 88); keyboard.setAvailableRange (24, 96); keyboard.setKeyWidth (1156.0f / 43.0f);
        keyboard.setLowestVisibleKey (24); keyboard.setColour (juce::MidiKeyboardComponent::whiteNoteColourId, juce::Colour (0xFFE9EDF8));
        keyboard.setColour (juce::MidiKeyboardComponent::blackNoteColourId, juce::Colour (0xFF10131F)); keyboard.setColour (juce::MidiKeyboardComponent::keySeparatorLineColourId, juce::Colour (0xFF1B1F33));
        keyboard.setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, theme::accent().withAlpha (0.55f)); keyboard.setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, theme::accent().withAlpha (0.25f));
        keyboard.setScrollButtonsVisible (false); keyboard.setMidiChannel (1); keyboard.setWantsKeyboardFocus (true);
        // ---- sobreposições
        addChildComponent (browser); browser.setBounds (0, 0, 1180, 760);
        addChildComponent (toast); toast.setBounds (340, 540, 500, 44);
        addChildComponent (prompt); prompt.setBounds (0, 0, 1180, 760);
        addChildComponent (help); help.setBounds (0, 0, 1180, 760);
        addChildComponent (login); login.setBounds (0, 0, 1180, 760); login.onUnlocked = [this] { syncAccess(); };

        backdrop.setBounds (0, 0, 1180, 760); decor.setBounds (0, 0, 1180, 760);
        proc.onPresetLoaded = [this] { presetBtn.setButtonText (proc.currentPresetName()); presetBtn.repaint(); };
        presetBtn.setButtonText (proc.currentPresetName());
        setMode (mode); showTab (juce::jlimit (0, kTabAdmin - 1, proc.getUiInt ("uiTab", 0)));
        syncAccess(); startTimerHz (30);
        juce::MessageManager::callAsync ([sp = juce::Component::SafePointer<MainView> (this)] { if (sp != nullptr && ! sp->login.isVisible()) sp->help.openIfFirstRun(); });
    }
    ~MainView() override { proc.onPresetLoaded = nullptr; ui::toastFn() = nullptr; stopTimer(); }

    Services svc;
    static constexpr int kNumTabs = 9, kTabAdmin = 8;
    void paintDecor (juce::Graphics& g)
    {
        // logótipo
        auto t = juce::Rectangle<int> (22, 6, 230, 40);
        for (int i = 3; i >= 1; --i) { g.setColour (theme::accent().withAlpha (0.045f * (float) (4 - i))); g.setFont (theme::font (31.0f + (float) i * 1.5f, true)); g.drawText ("SYNTHKEY", t.expanded (i, 0), juce::Justification::centredLeft); }
        g.setGradientFill (juce::ColourGradient (theme::accent(), 22, 0, theme::accent2(), 200, 0, false)); g.setFont (theme::font (31.0f, true)); g.drawText ("SYNTHKEY", t, juce::Justification::centredLeft);
        g.setColour (theme::dim()); g.setFont (theme::font (11.5f)); g.drawText (TX ("by Piradex   \xC2\xB7   v" SK_VERSION_STRING), 24, 42, 220, 16, juce::Justification::centredLeft);
        // área do preset
        theme::drawCard (g, juce::Rectangle<float> (336.0f, 13.0f, 336.0f, 38.0f), 10.0f, true);
        g.setColour (theme::dim()); g.setFont (theme::font (9.5f, true)); g.drawText ("PRESET", 346, 14, 80, 12, juce::Justification::centredLeft);
        // estado de acesso
        if (AccessManager::get().isUnlocked())
        {
            const auto role = AccessManager::get().roleText(); g.setFont (theme::font (11.5f, true));
            const int w = juce::GlyphArrangement::getStringWidthInt (g.getCurrentFont(), role) + 24; auto b = juce::Rectangle<float> ((float) (1060 - w), 69.0f, (float) w, 22.0f);
            g.setColour (juce::Colour (0xAA0A0C18)); g.fillRoundedRectangle (b, 11.0f); g.setColour (theme::good()); g.drawRoundedRectangle (b, 11.0f, 1.0f); g.drawText (role, b.toNearestInt(), juce::Justification::centred);
        }
        // rótulos
        g.setColour (theme::dim()); g.setFont (theme::font (10.0f, true)); g.drawText ("MACROS", 22, 567, 120, 12, juce::Justification::centredLeft);
        g.drawText (juce::String (proc.voiceCount.load()) + (proc.voiceCount.load() == 1 ? " VOZ" : " VOZES"), 1080, 582, 90, 16, juce::Justification::centredLeft);
        g.setColour (theme::stroke()); g.drawVerticalLine (626, 580.0f, 652.0f);
    }
    void paintOverChildren (juce::Graphics& g) override
    {
        if (proc.previewMode.load() && AccessManager::get().isUnlocked())
        {
            const bool ex = proc.previewExpired.load(); auto r = juce::Rectangle<float> (12, 108, 1156, 26);
            g.setColour (ex ? juce::Colour (0xE6501020) : juce::Colour (0xE63A2A08)); g.fillRoundedRectangle (r, 8.0f); g.setColour (ex ? theme::bad() : theme::warn()); g.drawRoundedRectangle (r.reduced (0.5f), 8.0f, 1.2f);
            g.setFont (theme::font (12.5f, true));
            g.drawText (ex ? TX ("PR\xC3\x89-VISUALIZA\xC3\x87\xC3\x83O TERMINOU  \xC2\xB7  o som est\xC3\xA1 em silêncio. Insira a chave do pack no separador PACKS.")
                           : TX ("\xF0\x9F\x94\x92 PR\xC3\x89-VISUALIZA\xC3\x87\xC3\x83O DE PACK BLOQUEADO  \xC2\xB7  ") + juce::String (proc.previewSecondsLeft.load(), 0) + TX (" s restantes"), r.toNearestInt(), juce::Justification::centred);
        }
    }
private:
    void timerCallback() override
    {
        syncAccess(); proc.undoWatch();
        if (currentTab < pages.size()) { if (auto* pg = pages[currentTab]) pg->tick(); }
        if (toast.expired()) toast.setVisible (false);
        meter.set (proc.meterL.load(), proc.meterR.load());
        const int vc = proc.voiceCount.load(); const int pv = (proc.previewMode.load() ? 1 : 0) + (proc.previewExpired.load() ? 2 : 0) + ((int) proc.previewSecondsLeft.load() << 4);
        if (vc != lastVoices || pv != lastPreview || (++frame % 30) == 0) { lastVoices = vc; lastPreview = pv; decor.repaint(); repaint(); }
        if (presetBtn.getButtonText() != proc.currentPresetName()) presetBtn.setButtonText (proc.currentPresetName());
    }
    // desfazer / refazer (Ctrl/Cmd+Z, Ctrl/Cmd+Shift+Z, Ctrl+Y)
    void doUndo (bool undo)
    {
        const bool ok = undo ? proc.undo() : proc.redo();
        toast.show (ok ? TX (undo ? "Desfeito." : "Refeito.") : TX (undo ? "Nada para desfazer." : "Nada para refazer."), false);
        presetBtn.setButtonText (proc.currentPresetName()); repaint();
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
        if (admin != adminShown) { adminShown = admin; if (admin && ! adminPage) { adminPage.reset (new AdminPage (proc, svc)); addAndMakeVisible (*adminPage); adminPage->setBounds (12, 108, 1156, 452); adminPage->setMode (mode); adminPage->setVisible (currentTab == kTabAdmin); } layoutTabs(); if (! admin && currentTab == kTabAdmin) showTab (0); }
        if (unlocked != lastUnlocked) { lastUnlocked = unlocked; layoutTabs(); decor.repaint(); }
    }
    void layoutTabs()
    {
        int x = 16; for (int i = 0; i < kNumTabs; ++i) { const bool vis = i < kTabAdmin || adminShown; tabs[i]->setVisible (vis); if (vis) { tabs[i]->setBounds (x, 64, 100, 34); x += 102; } }
        logout.setBounds (1070, 68, 90, 24); logout.setVisible (AccessManager::get().isUnlocked());
    }
    void showTab (int i)
    {
        if (i == kTabAdmin && ! adminPage) i = 0;
        currentTab = i; proc.setUiInt ("uiTab", std::min (i, kTabAdmin - 1));
        for (int k = 0; k < kNumTabs; ++k) tabs[k]->setToggleState (k == i, juce::dontSendNotification);
        for (int k = 0; k < pages.size(); ++k) pages[k]->setVisible (k == i);
        if (adminPage) adminPage->setVisible (i == kTabAdmin);
        if (auto* pg = i == kTabAdmin ? static_cast<Page*> (adminPage.get()) : pages[i]) pg->tick();
        repaint();
    }
    void setMode (int m)
    {
        mode = m; proc.setUiInt ("uiMode", m); for (int i = 0; i < 3; ++i) modeBtns[i]->setToggleState (i + 1 == m, juce::dontSendNotification);
        for (auto* pg : pages) pg->setMode (m); if (adminPage) adminPage->setMode (m);
    }
    void setPalette (int i) { theme::paletteIndex() = i; proc.setUiInt ("uiPalette", i); backdrop.repaint(); keyboard.setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, theme::accent().withAlpha (0.55f)); for (auto* d : dots) d->repaint(); decor.repaint(); repaint(); }

    void showPresetMenu()
    {
        const auto& list = proc.presetList(); juce::PopupMenu menu; presetIds.clear(); int id = 2000;
        std::vector<PackEntry> entries = PackHub::get().entries(); PackEntry user; user.id = "user"; user.name = "Utilizador"; entries.push_back (user);
        for (auto& e : entries)
        {
            juce::PopupMenu sub; juce::String lastCat; int n = 0;
            for (size_t i = 0; i < list.size(); ++i)
            {
                if (list[i].pack != e.id) continue;
                if (TX (list[i].category.c_str()) != lastCat) { lastCat = TX (list[i].category.c_str()); sub.addSectionHeader (lastCat); }
                presetIds.push_back ((int) i);
                sub.addItem (id++, (proc.presetIsLocked (list[i]) ? TX ("\xF0\x9F\x94\x92 ") : juce::String()) + TX (list[i].name.c_str()), true, (int) i == proc.currentPresetIndex()); ++n;
            }
            if (n > 0) menu.addSubMenu (TX (e.name.c_str()) + "  (" + juce::String (n) + ")", sub);
        }
        menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&presetBtn).withMinimumWidth (presetBtn.getWidth()), [this] (int r)
        { if (r >= 2000 && r - 2000 < (int) presetIds.size()) proc.loadPresetIndex (presetIds[(size_t) (r - 2000)]); });
    }
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

    SynthKeyProcessor& proc; int mode = 2, currentTab = 0, lastVoices = -1, lastPreview = -1, frame = 0; bool adminShown = false, lastUnlocked = false;
    Backdrop backdrop; Decor decor; juce::TextButton prevBtn, nextBtn, menuBtn, presetBtn, logout, panic, describeBtn, helpBtn;
    juce::OwnedArray<ModeBtn> modeBtns; juce::OwnedArray<PaletteDot> dots; juce::OwnedArray<TabBtn> tabs; juce::OwnedArray<Page> pages; std::unique_ptr<AdminPage> adminPage;
    juce::OwnedArray<Knob> macros; std::unique_ptr<Knob> master; Meter meter; ModSourceStrip modStrip; juce::MidiKeyboardComponent keyboard;
    Toast toast; PromptOverlay prompt; HelpOverlay help; LoginOverlay login; unsigned describeSeed = 1; std::vector<int> presetIds;
    PresetBrowser browser { proc, [this] (const juce::String& t, bool e) { toast.show (t, e); } };
public:
    void layoutHeader()
    {
        prevBtn.setBounds (300, 16, 32, 32); presetBtn.setBounds (340, 24, 328, 26); nextBtn.setBounds (676, 16, 32, 32); menuBtn.setBounds (714, 16, 40, 32);
        for (int i = 0; i < 3; ++i) modeBtns[i]->setBounds (780 + i * 82, 18, 78, 28);
        for (int i = 0; i < 5; ++i) dots[i]->setBounds (1034 + i * 24, 20, 24, 24);
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
