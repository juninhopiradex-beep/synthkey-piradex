// =============================================================================
//  SYNTHKEY by Piradex — navegador de presets (pesquisa, etiquetas, favoritos) e página de GROOVES
// =============================================================================
#pragma once
#include <set>
#include "Pages2.h"
#include "../Core/Grooves.h"

namespace ui
{
// ------------------------------------------------------------------------------- favoritos
class Favourites
{
public:
    static Favourites& get() { static Favourites f; return f; }
    bool has (const std::string& pack, const std::string& name) const { return keys.count (pack + "/" + name) > 0; }
    void toggle (const std::string& pack, const std::string& name)
    {
        const auto k = pack + "/" + name; if (! keys.erase (k)) keys.insert (k);
        save();
    }
private:
    Favourites() { load(); }
    static juce::File file() { return skpaths::userDir().getChildFile ("favoritos.txt"); }
    void load() { juce::StringArray l; l.addLines (file().loadFileAsString()); for (auto& s : l) if (s.trim().isNotEmpty()) keys.insert (s.trim().toStdString()); }
    void save() { juce::String t; for (auto& k : keys) t << juce::String::fromUTF8 (k.c_str()) << "\n"; file().replaceWithText (t); }
    std::set<std::string> keys;
};

// ------------------------------------------------------------------------------- navegador de presets
class PresetBrowser : public juce::Component, private juce::ListBoxModel, private juce::TextEditor::Listener
{
public:
    PresetBrowser (SynthKeyProcessor& p, std::function<void (const juce::String&, bool)> toastFn) : proc (p), toast (std::move (toastFn))
    {
        setWantsKeyboardFocus (true);
        addAndMakeVisible (search); search.setTextToShowWhenEmpty (TX ("Pesquisar nome, estilo, etiqueta\xE2\x80\xA6 (ex.: lead, 808, pad escuro)"), theme::dim());
        search.setFont (theme::font (14.0f)); search.setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xFF0A0C18)); search.setColour (juce::TextEditor::textColourId, theme::text());
        search.setColour (juce::TextEditor::outlineColourId, theme::stroke()); search.setColour (juce::TextEditor::focusedOutlineColourId, theme::accent());
        search.addListener (this);
        addAndMakeVisible (packBox); addAndMakeVisible (catBox); addAndMakeVisible (list); addAndMakeVisible (closeBtn);
        closeBtn.setButtonText (TX ("\xE2\x9C\x95")); closeBtn.onClick = [this] { close(); };
        packBox.onChange = [this] { rebuild(); }; catBox.onChange = [this] { rebuild(); };
        list.setModel (this); list.setRowHeight (40); list.setColour (juce::ListBox::backgroundColourId, juce::Colour (0xFF0A0C18)); list.setOutlineThickness (0);
        setVisible (false);
    }
    void open()
    {
        buildFilters(); setVisible (true); toFront (true); rebuild(); search.grabKeyboardFocus();
        const int cur = proc.currentPresetIndex();
        for (size_t r = 0; r < rows.size(); ++r) if (rows[r] == cur) { suppress = true; list.selectRow ((int) r); suppress = false; list.scrollToEnsureRowIsOnscreen ((int) r); }
    }
    void close() { setVisible (false); }
    bool isOpen() const { return isVisible(); }
    // chamado quando a lista de presets / packs mudou
    void refresh() { if (isVisible()) { buildFilters(); rebuild(); } }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colour (0x88000000));
        theme::drawCard (g, panel.toFloat(), 12.0f, true);
        g.setColour (theme::text()); g.setFont (theme::font (15.0f, true)); g.drawText ("PRESETS", panel.getX() + 16, panel.getY() + 10, 200, 22, juce::Justification::centredLeft);
        g.setColour (theme::dim()); g.setFont (theme::font (11.5f)); g.drawText (juce::String ((int) rows.size()) + " / " + juce::String ((int) proc.presetList().size()) + TX ("   \xC2\xB7   \xE2\x86\x91\xE2\x86\x93 navegar   \xC2\xB7   Esc fechar   \xC2\xB7   \xE2\x98\x85 favorito"), panel.getX() + 16, panel.getBottom() - 26, panel.getWidth() - 32, 18, juce::Justification::centredLeft);
    }
    void resized() override
    {
        panel = { 12, 104, 500, 552 };
        closeBtn.setBounds (panel.getRight() - 46, panel.getY() + 8, 32, 26);
        search.setBounds (panel.getX() + 14, panel.getY() + 42, 472, 32);
        packBox.setBounds (panel.getX() + 14, panel.getY() + 82, 232, 28); catBox.setBounds (panel.getX() + 254, panel.getY() + 82, 232, 28);
        list.setBounds (panel.getX() + 14, panel.getY() + 120, 472, panel.getHeight() - 120 - 34);
    }
    void mouseDown (const juce::MouseEvent& e) override { if (! panel.contains (e.getPosition())) close(); }
    bool keyPressed (const juce::KeyPress& k) override
    {
        if (k == juce::KeyPress::escapeKey) { close(); return true; }
        if (k == juce::KeyPress::downKey || k == juce::KeyPress::upKey) { list.keyPressed (k); return true; }
        return false;
    }
private:
    // ---- filtros
    void buildFilters()
    {
        const int pk = packBox.getSelectedId(), ct = catBox.getSelectedId(); const juce::String ctText = catBox.getText();
        packBox.clear (juce::dontSendNotification); catBox.clear (juce::dontSendNotification); packIds.clear();
        packBox.addItem (TX ("Todos os packs"), 1); packBox.addItem (TX ("\xE2\x98\x85 Favoritos"), 2); int id = 3;
        for (auto& e : PackHub::get().entries()) { packBox.addItem (TX (e.name.c_str()), id++); packIds.push_back (e.id); }
        packBox.addItem ("Utilizador", id); packIds.push_back ("user");
        packBox.setSelectedId (pk > 0 ? pk : 1, juce::dontSendNotification);
        juce::StringArray cats; for (auto& p : proc.presetList()) { const auto c = TX (p.category.c_str()); if (c.isNotEmpty() && ! cats.contains (c)) cats.add (c); }
        cats.sort (true); catBox.addItem (TX ("Todas as categorias"), 1); for (int i = 0; i < cats.size(); ++i) catBox.addItem (cats[i], i + 2);
        int sel = 1; for (int i = 0; i < cats.size(); ++i) if (cats[i] == ctText) sel = i + 2; (void) ct;
        catBox.setSelectedId (sel, juce::dontSendNotification);
    }
    void rebuild()
    {
        rows.clear(); const auto& L = proc.presetList(); const int pk = packBox.getSelectedId(); const juce::String cat = catBox.getSelectedId() > 1 ? catBox.getText() : juce::String();
        juce::StringArray tokens; tokens.addTokens (search.getText().toLowerCase(), " ", ""); tokens.removeEmptyStrings();
        for (size_t i = 0; i < L.size(); ++i)
        {
            const auto& p = L[i];
            if (pk == 2 && ! Favourites::get().has (p.pack, p.name)) continue;
            if (pk >= 3 && (size_t) (pk - 3) < packIds.size() && p.pack != packIds[(size_t) (pk - 3)]) continue;
            if (cat.isNotEmpty() && TX (p.category.c_str()) != cat) continue;
            if (! tokens.isEmpty())
            {
                juce::String hay = TX (p.name.c_str()) + " " + TX (p.category.c_str()) + " " + TX (p.style.c_str()) + " " + TX (p.pack.c_str());
                for (auto& t : p.tags) hay << " " << TX (t.c_str());
                hay = hay.toLowerCase(); bool ok = true; for (auto& t : tokens) if (! hay.contains (t)) { ok = false; break; }
                if (! ok) continue;
            }
            rows.push_back ((int) i);
        }
        list.updateContent(); list.repaint(); repaint();
    }
    void textEditorTextChanged (juce::TextEditor&) override { rebuild(); }
    void textEditorEscapeKeyPressed (juce::TextEditor&) override { close(); }
    void textEditorReturnKeyPressed (juce::TextEditor&) override { if (! rows.empty()) { proc.loadPresetIndex (rows[0]); } }

    // ---- lista
    int getNumRows() override { return (int) rows.size(); }
    void paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool selected) override
    {
        if (row < 0 || row >= (int) rows.size()) return;
        const int idx = rows[(size_t) row]; const auto& p = proc.presetList()[(size_t) idx];
        if (selected) { g.setColour (theme::accent().withAlpha (0.18f)); g.fillRect (0, 0, w, h); g.setColour (theme::accent()); g.fillRect (0, 0, 3, h); }
        else if (idx == proc.currentPresetIndex()) { g.setColour (juce::Colour (0x14FFFFFF)); g.fillRect (0, 0, w, h); }
        g.setColour (juce::Colour (0x10FFFFFF)); g.drawHorizontalLine (h - 1, 0.0f, (float) w);
        PackEntry pe; juce::Colour tag = theme::dim();
        if (PackHub::get().entry (p.pack, pe)) tag = juce::Colour::fromString ("FF" + juce::String (pe.accent).substring (1));
        g.setColour (tag); g.fillEllipse (12.0f, (float) h * 0.5f - 4.0f, 8.0f, 8.0f);
        const bool locked = proc.presetIsLocked (p);
        g.setColour (locked ? theme::dim() : theme::text()); g.setFont (theme::font (14.0f, true));
        g.drawText ((locked ? TX ("\xF0\x9F\x94\x92 ") : juce::String()) + TX (p.name.c_str()), 30, 3, w - 80, 20, juce::Justification::centredLeft);
        juce::String sub = TX (p.category.c_str()); int n = 0; for (auto& t : p.tags) { if (n++ >= 3) break; sub << "  \xC2\xB7  " << TX (t.c_str()); }
        g.setColour (theme::dim()); g.setFont (theme::font (11.5f)); g.drawText ((pe.name.empty() ? juce::String(p.pack) : TX (pe.name.c_str())) + "   " + sub, 30, 21, w - 80, 16, juce::Justification::centredLeft);
        const bool fav = Favourites::get().has (p.pack, p.name);
        g.setColour (fav ? theme::warn() : juce::Colour (0x55FFFFFF)); g.setFont (theme::font (20.0f)); g.drawText (fav ? TX ("\xE2\x98\x85") : TX ("\xE2\x98\x86"), w - 40, 0, 32, h, juce::Justification::centred);
    }
    void listBoxItemClicked (int row, const juce::MouseEvent& e) override
    {
        if (row < 0 || row >= (int) rows.size()) return;
        const auto& p = proc.presetList()[(size_t) rows[(size_t) row]];
        if (e.x > list.getWidth() - 46) { Favourites::get().toggle (p.pack, p.name); if (packBox.getSelectedId() == 2) rebuild(); else list.repaint(); return; }
        proc.loadPresetIndex (rows[(size_t) row]);
    }
    void listBoxItemDoubleClicked (int, const juce::MouseEvent&) override { close(); }
    void selectedRowsChanged (int row) override { if (! suppress && row >= 0 && row < (int) rows.size()) proc.loadPresetIndex (rows[(size_t) row]); }

    SynthKeyProcessor& proc; std::function<void (const juce::String&, bool)> toast;
    juce::TextEditor search; juce::ComboBox packBox, catBox; juce::ListBox list; juce::TextButton closeBtn;
    juce::Rectangle<int> panel { 12, 104, 500, 552 }; std::vector<int> rows; std::vector<std::string> packIds; bool suppress = false;
};

// ------------------------------------------------------------------------------- vista do piano-roll de um groove
class GrooveRoll : public juce::Component
{
public:
    void set (const sk::Groove* gr, double playhead) { if (gr != g || std::abs (playhead - head) > 1e-6) { g = gr; head = playhead; repaint(); } }
    void paint (juce::Graphics& gg) override
    {
        auto r = getLocalBounds().toFloat(); gg.setColour (juce::Colour (0xFF0A0C18)); gg.fillRoundedRectangle (r, 6.0f);
        if (! g || g->notes.empty()) return;
        r = r.reduced (6.0f); const int steps = g->totalSteps(); const bool drums = g->kind == "Bateria";
        std::vector<int> lanes; for (auto& n : g->notes) if (std::find (lanes.begin(), lanes.end(), n.note) == lanes.end()) lanes.push_back (n.note);
        std::sort (lanes.begin(), lanes.end()); int lo = lanes.front(), hi = lanes.back();
        const int nl = drums ? (int) lanes.size() : std::max (8, hi - lo + 1); const float lh = r.getHeight() / (float) nl, sw = r.getWidth() / (float) steps;
        for (int s = 0; s <= steps; ++s) { gg.setColour (s % 16 == 0 ? juce::Colour (0x40FFFFFF) : (s % 4 == 0 ? juce::Colour (0x22FFFFFF) : juce::Colour (0x0DFFFFFF))); gg.drawVerticalLine ((int) (r.getX() + s * sw), r.getY(), r.getBottom()); }
        for (auto& n : g->notes)
        {
            int li = drums ? (int) (std::find (lanes.begin(), lanes.end(), n.note) - lanes.begin()) : (n.note - lo); const float y = r.getBottom() - (li + 1) * lh;
            auto b = juce::Rectangle<float> (r.getX() + n.step * sw + 1.0f, y + 1.0f, std::max (3.0f, n.len * sw - 2.0f), std::max (3.0f, lh - 2.0f));
            gg.setColour ((drums ? theme::accent2() : theme::accent()).withAlpha (0.35f + 0.65f * (float) n.vel / 127.0f)); gg.fillRoundedRectangle (b, 2.0f);
        }
        if (head >= 0) { const float x = r.getX() + (float) head * sw; gg.setColour (juce::Colours::white.withAlpha (0.85f)); gg.drawVerticalLine ((int) x, r.getY(), r.getBottom()); }
    }
private:
    const sk::Groove* g = nullptr; double head = -1;
};

class MidiDragHandle : public juce::Component
{
public:
    std::function<juce::File()> makeFile;
    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced (1.0f); g.setColour (isMouseOver() ? theme::accent().withAlpha (0.20f) : juce::Colour (0xFF121528)); g.fillRoundedRectangle (r, 8.0f);
        g.setColour (theme::accent()); const float d[2] = { 5.0f, 5.0f }; juce::Path p; p.addRoundedRectangle (r.reduced (0.5f), 8.0f);
        juce::PathStrokeType st (1.2f); juce::Path dashed; st.createDashedStroke (dashed, p, d, 2); g.fillPath (dashed);
        g.setColour (theme::text()); g.setFont (theme::font (13.0f, true));
        g.drawText (TX ("\xE2\xA0\xBF  ARRASTAR MIDI PARA O DAW"), getLocalBounds(), juce::Justification::centred);
    }
    void mouseEnter (const juce::MouseEvent&) override { repaint(); } void mouseExit (const juce::MouseEvent&) override { repaint(); }
    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (dragging || e.getDistanceFromDragStart() < 6 || ! makeFile) return;
        const auto f = makeFile(); if (! f.existsAsFile()) return;
        dragging = true; juce::DragAndDropContainer::performExternalDragDropOfFiles (juce::StringArray (f.getFullPathName()), false, this, [this] { dragging = false; });
    }
    void mouseUp (const juce::MouseEvent&) override { dragging = false; }
private:
    bool dragging = false;
};

// ================================================================================== página GROOVES
class GroovesPage : public Page, private juce::ListBoxModel
{
public:
    GroovesPage (SynthKeyProcessor& p, Services& s) : Page (p), svc (s)
    {
        static const char* styles[8] = { "todos", "trap", "rnb", "afrobeat", "amapiano", "drill", "kizomba", "kuduro" };
        static const char* labels[8] = { "TODOS", "TRAP", "R&B", "AFROBEATS", "AMAPIANO", "DRILL", "KIZOMBA", "KUDURO" };
        for (int i = 0; i < 8; ++i)
        {
            styleIds.push_back (styles[i]); auto* b = styleBtns.add (new juce::TextButton (labels[i])); addAndMakeVisible (b);
            b->setClickingTogglesState (false); b->onClick = [this, i] { setStyleFilter (i); }; b->setBounds (i * 104, 0, 100, 32);
        }
        addAndMakeVisible (kindBox); kindBox.addItem ("Todos os tipos", 1); kindBox.addItem ("Bateria", 2); kindBox.addItem ("Baixo", 3); kindBox.addItem ("Acordes", 4); kindBox.addItem ("Melodia", 5);
        kindBox.setSelectedId (1, juce::dontSendNotification); kindBox.onChange = [this] { rebuild(); }; kindBox.setBounds (840, 2, 140, 28);
        addAndMakeVisible (styleModeBtn); styleModeBtn.setButtonText ("MODO ESTILO"); styleModeBtn.setBounds (996, 0, 160, 32); styleModeBtn.onClick = [this] { applyStyleMode(); };
        styleModeBtn.setTooltip (TX ("Escolhe o estilo: muda o padr\xC3\xA3o do arpejador, filtra os grooves e carrega um preset do estilo"));
        addAndMakeVisible (list); list.setModel (this); list.setRowHeight (36); list.setColour (juce::ListBox::backgroundColourId, juce::Colour (0xFF0A0C18)); list.setOutlineThickness (0); list.setBounds (0, 42, 470, 410);
        addAndMakeVisible (roll); roll.setBounds (494, 92, 648, 236);
        addAndMakeVisible (playBtn); playBtn.setButtonText (TX ("\xE2\x96\xB6  TOCAR")); playBtn.setBounds (494, 348, 120, 40); playBtn.onClick = [this] { togglePlay(); };
        addAndMakeVisible (keyBox); const char* keys[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
        for (int i = 0; i < 12; ++i) keyBox.addItem (juce::String ("Tom: ") + keys[i], i + 1); keyBox.setSelectedId (1, juce::dontSendNotification); keyBox.setBounds (626, 354, 110, 28);
        keyBox.onChange = [this] { if (playing) restart(); };
        addAndMakeVisible (bpmSlider); bpmSlider.setSliderStyle (juce::Slider::LinearHorizontal); bpmSlider.setRange (60.0, 200.0, 1.0); bpmSlider.setValue (120.0, juce::dontSendNotification);
        bpmSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 64, 24); bpmSlider.setTextValueSuffix (" bpm"); bpmSlider.setBounds (750, 352, 392, 32);
        bpmSlider.onValueChange = [this] { if (playing) proc.groove.setBpm (bpmSlider.getValue()); };
        addAndMakeVisible (drag); drag.setBounds (494, 402, 290, 44); drag.makeFile = [this] { return exportMidi(); };
        addAndMakeVisible (soundBtn); soundBtn.setBounds (796, 402, 226, 44); soundBtn.onClick = [this] { chooseSound(); };
        soundBtn.setTooltip (TX ("Som do groove: toca num motor \xC3\xA0 parte (ex.: kit de bateria), por isso podes tocar outro som por cima ao mesmo tempo"));
        addAndMakeVisible (volSlider); volSlider.setSliderStyle (juce::Slider::LinearHorizontal); volSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        volSlider.setRange (0.0, 1.5, 0.01); volSlider.setValue (proc.grooveVolume(), juce::dontSendNotification); volSlider.setDoubleClickReturnValue (true, 0.85);
        volSlider.setTooltip (TX ("Volume do groove")); volSlider.setBounds (1030, 402, 112, 44); volSlider.onValueChange = [this] { proc.setGrooveVolume ((float) volSlider.getValue()); };
        refreshSoundBtn();
        setStyleFilter (0);
    }
    void tick() override
    {
        Page::tick(); const bool run = proc.groove.isRunning();
        if (run != playing) { playing = run; playBtn.setButtonText (playing ? TX ("\xE2\x96\xA0  PARAR") : TX ("\xE2\x96\xB6  TOCAR")); }
        roll.set (selected(), playing ? proc.groove.stepPosition() : -1.0);
    }
    void paint (juce::Graphics& g) override
    {
        const auto* gr = selected();
        theme::drawCard (g, juce::Rectangle<float> (482, 42, 674, 410), 12.0f);
        g.setColour (theme::text()); g.setFont (theme::font (16.0f, true)); g.drawText (gr ? TX (gr->name.c_str()) : TX ("Escolhe um groove na lista"), 494, 50, 640, 24, juce::Justification::centredLeft);
        if (gr) { g.setColour (theme::dim()); g.setFont (theme::font (12.0f)); g.drawText (TX (gr->style.c_str()) + "  \xC2\xB7  " + TX (gr->kind.c_str()) + "  \xC2\xB7  " + juce::String (gr->bpm) + " bpm  \xC2\xB7  " + juce::String (gr->bars) + (gr->bars == 1 ? " compasso" : " compassos"), 494, 72, 640, 16, juce::Justification::centredLeft); }
    }
private:
    const sk::Groove* selected() const { return selRow >= 0 && selRow < (int) rows.size() ? rows[(size_t) selRow] : nullptr; }
    void setStyleFilter (int i)
    {
        styleIdx = i; for (int k = 0; k < styleBtns.size(); ++k) styleBtns[k]->setColour (juce::TextButton::buttonColourId, k == i ? theme::accent().withAlpha (0.55f) : juce::Colour (0xFF1A1D30));
        rebuild();
    }
    void rebuild()
    {
        rows.clear(); const int k = kindBox.getSelectedId(); static const char* kinds[5] = { "", "Bateria", "Baixo", "Acordes", "Melodia" };
        for (auto& g : sk::allGrooves()) { if (styleIdx > 0 && g.style != styleIds[(size_t) styleIdx]) continue; if (k > 1 && g.kind != kinds[k - 1]) continue; rows.push_back (&g); }
        selRow = rows.empty() ? -1 : 0; list.updateContent(); list.selectRow (selRow); list.repaint(); onSelect(); repaint();
    }
    void onSelect() { if (const auto* g = selected()) bpmSlider.setValue (g->bpm, juce::dontSendNotification); if (playing) restart(); repaint(); }
    int transposeSemis() const { const int k = keyBox.getSelectedId() - 1; return k <= 6 ? k : k - 12; }
    void togglePlay() { if (playing) { proc.groove.stop(); } else restart(); }
    void restart() { if (const auto* g = selected()) { proc.prepareGrooveLayer (*g); refreshSoundBtn(); proc.groove.start (*g, transposeSemis(), bpmSlider.getValue()); } }
    void refreshSoundBtn()
    {
        const auto k = proc.grooveSoundKey(); juce::String s;
        if (k == "auto") s = proc.grooveLayerOn() ? TX ("Auto \xC2\xB7 ") + proc.grooveLayerName() : TX ("Auto \xC2\xB7 o teu som");
        else if (k == "same") s = TX ("Igual ao teu som"); else s = k;
        soundBtn.setButtonText (TX ("SOM: ") + s);
    }
    void chooseSound()
    {
        juce::PopupMenu m; m.addItem (1, TX ("Auto \xE2\x80\x94 kit do estilo na bateria, o resto no teu som"), true, proc.grooveSoundKey() == "auto");
        m.addItem (2, TX ("Igual ao teu som"), true, proc.grooveSoundKey() == "same"); m.addSeparator();
        const auto& L = proc.presetList(); std::vector<std::string> cats;
        for (auto& p : L) if (std::find (cats.begin(), cats.end(), p.category) == cats.end()) cats.push_back (p.category);
        std::sort (cats.begin(), cats.end());
        for (auto& cat : cats)
        {
            juce::PopupMenu sub; for (size_t i = 0; i < L.size(); ++i) if (L[i].category == cat) sub.addItem (100 + (int) i, TX (L[i].name.c_str()), ! proc.presetIsLocked (L[i]), proc.grooveSoundKey() == juce::String (L[i].name));
            m.addSubMenu (TX (cat.empty() ? "Outros" : cat.c_str()), sub);
        }
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&soundBtn), [this] (int r)
        {
            if (r == 1) proc.setGrooveSoundKey ("auto"); else if (r == 2) proc.setGrooveSoundKey ("same");
            else if (r >= 100 && r - 100 < (int) proc.presetList().size()) proc.setGrooveSoundKey (juce::String (proc.presetList()[(size_t) (r - 100)].name)); else return;
            if (playing) restart(); else if (const auto* g = selected()) { proc.prepareGrooveLayer (*g); } refreshSoundBtn();
        });
    }
    void applyStyleMode()
    {
        const int idx = styleIdx > 0 ? styleIdx : 1; const std::string st = styleIds[(size_t) idx]; if (styleIdx == 0) setStyleFilter (idx);
        proc.setReal (P_ARP_PATTERN, (float) (idx));                   // trap 1, rnb 2, afrobeat 3, amapiano 4, drill 5, kizomba 6, kuduro 7
        const auto& L = proc.presetList(); bool loaded = false;
        for (size_t i = 0; i < L.size() && ! loaded; ++i) if (L[i].pack == st && ! proc.presetIsLocked (L[i])) { proc.loadPresetIndex ((int) i); loaded = true; }
        PackEntry pe; const juce::String nm = PackHub::get().entry (st, pe) ? TX (pe.name.c_str()) : TX (st.c_str());
        if (svc.toast) svc.toast (loaded ? TX ("Modo estilo: ") + nm + TX (" \xE2\x80\x94 arpejador, grooves e preset ajustados") : TX ("Modo estilo: ") + nm + TX (" \xE2\x80\x94 instala o pack para carregar os presets"), false);
    }
    juce::File exportMidi()
    {
        const auto* g = selected(); if (! g) return {};
        const int tr = transposeSemis(); const double bpm = bpmSlider.getValue(); const bool drums = g->kind == "Bateria";
        juce::MidiMessageSequence seq; constexpr int ppq = 480, tps = ppq / 4; const int ch = drums ? 10 : 1;
        seq.addEvent (juce::MidiMessage::tempoMetaEvent ((int) std::lround (60000000.0 / bpm)), 0.0);
        seq.addEvent (juce::MidiMessage::timeSignatureMetaEvent (4, 4), 0.0);
        for (auto& n : g->notes)
        {
            const int note = juce::jlimit (0, 127, n.note + (drums ? 0 : tr)); const double sw = (n.step % 2 == 1) ? g->swing : 0.0;
            const double t0 = (n.step + sw) * tps, t1 = t0 + n.len * tps - 4;
            seq.addEvent (juce::MidiMessage::noteOn (ch, note, (juce::uint8) n.vel), t0); seq.addEvent (juce::MidiMessage::noteOff (ch, note), t1);
        }
        seq.updateMatchedPairs();
        juce::MidiFile mf; mf.setTicksPerQuarterNote (ppq); mf.addTrack (seq);
        auto dir = skpaths::userDir().getChildFile ("Grooves"); dir.createDirectory();
        static const char* keys[12] = { "C", "Cs", "D", "Ds", "E", "F", "Fs", "G", "Gs", "A", "As", "B" };
        const auto file = dir.getChildFile (juce::File::createLegalFileName (TX (g->name.c_str()) + "_" + keys[((tr % 12) + 12) % 12] + "_" + juce::String ((int) bpm) + "bpm") + ".mid");
        file.deleteFile(); juce::FileOutputStream os (file); if (! os.openedOk()) return {}; mf.writeTo (os); return file;
    }

    // lista
    int getNumRows() override { return (int) rows.size(); }
    void paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool sel) override
    {
        if (row < 0 || row >= (int) rows.size()) return; const auto& gr = *rows[(size_t) row];
        if (sel) { g.setColour (theme::accent().withAlpha (0.18f)); g.fillRect (0, 0, w, h); g.setColour (theme::accent()); g.fillRect (0, 0, 3, h); }
        g.setColour (juce::Colour (0x10FFFFFF)); g.drawHorizontalLine (h - 1, 0.0f, (float) w);
        const bool d = gr.kind == "Bateria"; g.setColour (d ? theme::accent2() : theme::accent()); g.setFont (theme::font (10.5f, true));
        g.drawText (TX (gr.kind.c_str()).toUpperCase(), 12, 0, 74, h, juce::Justification::centredLeft);
        g.setColour (theme::text()); g.setFont (theme::font (13.5f, true)); g.drawText (TX (gr.name.c_str()), 90, 2, w - 190, 18, juce::Justification::centredLeft);
        g.setColour (theme::dim()); g.setFont (theme::font (11.0f)); g.drawText (TX (gr.style.c_str()), 90, 19, w - 190, 14, juce::Justification::centredLeft);
        g.drawText (juce::String (gr.bpm) + " bpm", w - 96, 0, 84, h, juce::Justification::centredRight);
    }
    void selectedRowsChanged (int row) override { if (row != selRow) { selRow = row; onSelect(); } }
    void listBoxItemDoubleClicked (int, const juce::MouseEvent&) override { restart(); }

    Services& svc; juce::OwnedArray<juce::TextButton> styleBtns; std::vector<std::string> styleIds; int styleIdx = 0;
    juce::ComboBox kindBox, keyBox; juce::TextButton styleModeBtn, playBtn, soundBtn; juce::Slider volSlider; juce::ListBox list; GrooveRoll roll; juce::Slider bpmSlider; MidiDragHandle drag;
    std::vector<const sk::Groove*> rows; int selRow = -1; bool playing = false;
};
}
