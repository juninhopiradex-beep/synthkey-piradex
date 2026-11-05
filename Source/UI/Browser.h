// =============================================================================
//  SYNTHKEY by Piradex — navegador de presets
//    pesquisa · filtros (pack, categoria, motor, validação, tempo, tom, estrelas) · ordenação · favoritos · avaliação 1–5 estrelas
//    PARECIDOS (semelhança sonora com o som atual) · PARA TI (recomendações a partir de favoritos/estrelas) · SOUND MATCH (WAV de referência)
//    ASSISTENTE (descreve a mudança → variações) · CAMADA (Smart Layer Builder) · MACROS AUTO · A/B · DNA do preset
// =============================================================================
#pragma once
#include <set>
#include <map>
#include <thread>
#include "Pages2.h"
#include "ImportUI.h"
#include "../Core/SoundAssist.h"
#include "../Core/PresetCalib.h"

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

// ------------------------------------------------------------------------------- avaliação pessoal (1–5 estrelas), por utilizador
class Ratings
{
public:
    static Ratings& get() { static Ratings r; return r; }
    int of (const std::string& pack, const std::string& name) const { auto it = m.find (pack + "/" + name); return it == m.end() ? 0 : it->second; }
    void set (const std::string& pack, const std::string& name, int stars)
    {
        stars = std::max (0, std::min (5, stars)); const auto k = pack + "/" + name;
        if (stars == 0) m.erase (k); else m[k] = stars;
        save();
    }
    int count() const { return (int) m.size(); }
private:
    Ratings() { load(); }
    static juce::File file() { return skpaths::userDir().getChildFile ("avaliacoes.txt"); }
    void load() { juce::StringArray l; l.addLines (file().loadFileAsString()); for (auto& s : l) { const int t = s.lastIndexOfChar ('\t'); if (t <= 0) continue; const int v = s.substring (t + 1).getIntValue(); if (v >= 1 && v <= 5) m[s.substring (0, t).toStdString()] = v; } }
    void save() { juce::String t; for (auto& kv : m) t << juce::String::fromUTF8 (kv.first.c_str()) << "\t" << kv.second << "\n"; file().replaceWithText (t); }
    std::map<std::string, int> m;
};

// ------------------------------------------------------------------------------- navegador
class PresetBrowser : public juce::Component, private juce::ListBoxModel, private juce::TextEditor::Listener, private juce::Timer
{
public:
    PresetBrowser (SynthKeyProcessor& p, std::function<void (const juce::String&, bool)> toastFn, Services* services = nullptr) : proc (p), toast (std::move (toastFn)), svc (services)
    {
        setWantsKeyboardFocus (true);
        addAndMakeVisible (search); search.setTextToShowWhenEmpty (TX ("Pesquisar nome, estilo, etiqueta\xE2\x80\xA6 (ex.: lead, 808, pad escuro)"), theme::dim());
        search.setFont (theme::font (14.0f)); search.setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xFF0A0C18)); search.setColour (juce::TextEditor::textColourId, theme::text());
        search.setColour (juce::TextEditor::outlineColourId, theme::stroke()); search.setColour (juce::TextEditor::focusedOutlineColourId, theme::accent());
        search.addListener (this);
        for (auto* c : { &packBox, &catBox, &engBox, &qaBox, &tempoBox, &tonBox, &starBox, &sortBox }) addAndMakeVisible (c);
        addAndMakeVisible (list); addAndMakeVisible (closeBtn); addAndMakeVisible (modeBtn); modeBtn.setVisible (false);
        closeBtn.setButtonText (TX ("\xE2\x9C\x95")); closeBtn.onClick = [this] { close(); };
        engBox.addItem (TX ("Todos os motores"), 1); engBox.addItem (TX ("Anal\xC3\xB3gico"), 2); engBox.addItem ("Wavetable", 3); engBox.addItem ("Sample", 4); engBox.addItem (TX ("H\xC3\xAD" "brido"), 5); engBox.addItem (TX ("F\xC3\xADsico (Corda)"), 6); engBox.addItem (TX ("M\xC3\xB3" "dulos"), 7); engBox.addItem ("Granular", 8); engBox.addItem ("Espectral", 9);
        qaBox.addItem (TX ("Qualquer valida\xC3\xA7\xC3\xA3o"), 1); qaBox.addItem (TX ("Ouvidos / lan\xC3\xA7" "ados"), 2); qaBox.addItem (TX ("S\xC3\xB3 valida\xC3\xA7\xC3\xA3o t\xC3\xA9" "cnica"), 3);
        tempoBox.addItem (TX ("Tempo: qualquer"), 1); tempoBox.addItem (TX ("Sincroniza ao tempo (arp/delay)"), 2); tempoBox.addItem (TX ("Tempo livre"), 3);
        tonBox.addItem (TX ("Tom: qualquer"), 1); tonBox.addItem (TX ("Acorde maior"), 2); tonBox.addItem (TX ("Acorde menor"), 3); tonBox.addItem (TX ("Diat\xC3\xB3nico"), 4); tonBox.addItem (TX ("Nota \xC3\xBAnica (livre)"), 5);
        starBox.addItem (TX ("Qualquer avalia\xC3\xA7\xC3\xA3o"), 1); starBox.addItem (TX ("\xE2\x98\x85 1 ou mais"), 2); starBox.addItem (TX ("\xE2\x98\x85 3 ou mais"), 3); starBox.addItem (TX ("\xE2\x98\x85 4 ou mais"), 4); starBox.addItem (TX ("\xE2\x98\x85 5"), 5);
        sortBox.addItem (TX ("Ordem original"), 1); sortBox.addItem (TX ("Nome A\xE2\x80\x93Z"), 2); sortBox.addItem (TX ("Melhor avalia\xC3\xA7\xC3\xA3o"), 3); sortBox.addItem (TX ("Favoritos primeiro"), 4); sortBox.addItem (TX ("CPU leve primeiro"), 5);
        for (auto* c : { &engBox, &qaBox, &tempoBox, &tonBox, &starBox, &sortBox }) { c->setSelectedId (1, juce::dontSendNotification); c->onChange = [this] { toList(); }; }
        packBox.onChange = [this] { toList(); }; catBox.onChange = [this] { toList(); };
        modeBtn.onClick = [this] { if (mode != Mode::List) toList(); };
        list.setModel (this); list.setRowHeight (40); list.setColour (juce::ListBox::backgroundColourId, juce::Colour (0xFF0A0C18)); list.setOutlineThickness (0);
        // ---- ações (coluna da direita)
        const char* names[] = { "PARECIDOS", "PARA TI", "SOUND MATCH\xE2\x80\xA6", "ASSISTENTE\xE2\x80\xA6", "CAMADA\xE2\x80\xA6", "MACROS AUTO", "A / B", "GUARDAR\xE2\x80\xA6" };
        juce::TextButton* bs[] = { &similarBtn, &forYouBtn, &matchBtn, &assistBtn, &layerBtn, &macrosBtn, &abBtn, &saveBtn };
        for (int i = 0; i < 8; ++i) { bs[i]->setButtonText (TX (names[i])); addAndMakeVisible (bs[i]); }
        similarBtn.setTooltip (TX ("Encontra presets que soam como o som que est\xC3\xA1s a ouvir agora (mede o som real, n\xC3\xA3o o nome)"));
        forYouBtn.setTooltip (TX ("Recomenda presets parecidos com os teus favoritos e os que avaliaste com 3 ou mais estrelas"));
        matchBtn.setTooltip (TX ("Escolhe um WAV de refer\xC3\xAAncia (uma nota, um sample, um loop curto) e encontra os presets mais pr\xC3\xB3ximos"));
        assistBtn.setTooltip (TX ("Descreve a mudan\xC3\xA7""a (ex.: mais escuro e largo) e recebe variações calibradas do som atual"));
        layerBtn.setTooltip (TX ("Smart Layer Builder: sugere um segundo som que complementa o atual e junta os dois (empilhados ou divididos no teclado)"));
        macrosBtn.setTooltip (TX ("Liga as macros livres do som atual a destinos adequados \xC3\xA0 sua arquitectura (a 0 o som n\xC3\xA3o muda)"));
        abBtn.setTooltip (TX ("Compara dois sons: A e B. Edita ou carrega outro preset em B e alterna sem perder nenhum"));
        saveBtn.setTooltip (TX ("Guarda o som atual como preset do utilizador"));
        similarBtn.onClick = [this] { doSimilar(); }; forYouBtn.onClick = [this] { doForYou(); }; matchBtn.onClick = [this] { doMatchChoose(); };
        assistBtn.onClick = [this] { doAssistant(); }; layerBtn.onClick = [this] { doLayerOpen(); }; macrosBtn.onClick = [this] { doMacros(); };
        abBtn.onClick = [this] { proc.abToggle(); refreshDetail(); list.repaint(); }; saveBtn.onClick = [this] { doSave(); };
        addAndMakeVisible (splitTgl); splitTgl.setButtonText (TX ("dividir o teclado em D\xC3\xB3 central (60)")); splitTgl.setTooltip (TX ("Desligado: os dois sons tocam em todo o teclado. Ligado: o som atual abaixo do D\xC3\xB3 central, o parceiro desde a\xC3\xAD"));
        setVisible (false);
    }
    void open()
    {
        buildFilters(); setVisible (true); toFront (true); toList(); search.grabKeyboardFocus(); refreshDetail(); startTimerHz (4);
        const int cur = proc.currentPresetIndex();
        for (size_t r = 0; r < rows.size(); ++r) if (mode == Mode::List && rows[r] == cur) { suppress = true; list.selectRow ((int) r); suppress = false; list.scrollToEnsureRowIsOnscreen ((int) r); }
    }
    void close() { setVisible (false); stopTimer(); }
    bool isOpen() const { return isVisible(); }
    void refresh() { if (isVisible()) { buildFilters(); rebuild(); } }
    // Sound Match a partir de um ficheiro (também chamado quando se larga um WAV na janela)
    void soundMatch (const juce::File& f)
    {
        if (! isVisible()) { buildFilters(); setVisible (true); toFront (true); startTimerHz (4); }
        const auto name = f.getFileName();
        runAsync (TX ("A analisar ") + name + TX ("\xE2\x80\xA6"), [f]() -> std::function<void (PresetBrowser&)>
        {
            sk::SampleData sd; const auto err = useraudio::decode (f, sd);
            if (err.isNotEmpty()) return [err] (PresetBrowser& b) { b.say (err, true); };
            sk::search::Fingerprint fp; const bool ok = sk::search::fingerprintFromAudio (sd.l.data(), sd.stereo() ? sd.r.data() : nullptr, sd.l.size(), (double) sd.sampleRate, fp);
            if (! ok) return [] (PresetBrowser& b) { b.say (TX ("O \xC3\xA1udio \xC3\xA9 demasiado curto (m\xC3\xADnimo ~0,12 s) ou est\xC3\xA1 em sil\xC3\xAAncio."), true); };
            return [fp, name = f.getFileName()] (PresetBrowser& b) { b.showHits (Mode::Match, TX ("Sound Match: ") + name, sk::search::nearest (fp, b.proc.presetList(), 80, -1, b.acceptFn())); };
        });
    }

    // variação rápida do som atual numa direção (modo TOCAR): cria, calibra e carrega
    void quickVariation (const std::string& word)
    {
        const sk::Preset base = proc.capturePreset (proc.currentPresetName()); const uint32_t seed = ++assistSeed; auto lib = proc.currentLibrary();
        runAsync (TX ("A criar varia\xC3\xA7\xC3\xA3o\xE2\x80\xA6"), [base, word, seed, lib]() -> std::function<void (PresetBrowser&)>
        {
            auto vs = sk::assist::variations (base, word, 1, seed); static const sk::Library empty;
            if (vs.empty()) return [] (PresetBrowser& b) { b.say (TX ("N\xC3\xA3o consegui variar este som nessa dire\xC3\xA7\xC3\xA3o."), false); };
            sk::Preset q = vs[0].preset; sk::presets::calibrate (q, lib ? *lib : empty);
            return [q, note = vs[0].note] (PresetBrowser& b) { b.proc.loadPreset (q, -1); b.say (TX ("Varia\xC3\xA7\xC3\xA3o: ") + TX (note.c_str()), false); };
        });
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colour (0x88000000));
        theme::drawCard (g, panel.toFloat(), 12.0f, true);
        g.setColour (theme::text()); g.setFont (theme::font (15.0f, true)); g.drawText ("PRESETS", panel.getX() + 16, panel.getY() + 10, 200, 22, juce::Justification::centredLeft);
        g.setColour (theme::dim()); g.setFont (theme::font (11.5f));
        g.drawText (juce::String ((int) rows.size()) + " / " + juce::String (mode == Mode::Gen ? (int) gen.size() : (int) proc.presetList().size()) + TX ("   \xC2\xB7   \xE2\x86\x91\xE2\x86\x93 navegar   \xC2\xB7   Esc fechar   \xC2\xB7   \xE2\x98\x85 estrelas e \xE2\x99\xA5 favorito"), panel.getX() + 16, panel.getBottom() - 26, 500, 18, juce::Justification::centredLeft);
        // coluna do detalhe
        auto d = detailArea(); g.setColour (juce::Colour (0xFF0E1020)); g.fillRoundedRectangle (d.toFloat(), 8.0f); g.setColour (theme::stroke()); g.drawRoundedRectangle (d.toFloat().reduced (0.5f), 8.0f, 1.0f);
        g.setColour (theme::text()); g.setFont (theme::font (15.0f, true)); g.drawFittedText (curName(), d.getX() + 12, d.getY() + 8, d.getWidth() - 24, 22, juce::Justification::centredLeft, 1);
        g.setColour (theme::dim()); g.setFont (theme::font (11.0f)); g.drawText (detailSub, d.getX() + 12, d.getY() + 31, d.getWidth() - 24, 16, juce::Justification::centredLeft);
        // estrelas da avaliação do preset atual
        const int cr = curRating();
        for (int i = 0; i < 5; ++i) { g.setColour (i < cr ? theme::warn() : juce::Colour (0x55FFFFFF)); g.setFont (theme::font (22.0f)); g.drawText (i < cr ? TX ("\xE2\x98\x85") : TX ("\xE2\x98\x86"), d.getX() + 10 + i * 26, d.getY() + 52, 26, 26, juce::Justification::centred); }
        g.setColour (theme::dim()); g.setFont (theme::font (10.5f)); g.drawText (cr ? TX ("a tua avalia\xC3\xA7\xC3\xA3o") : TX ("avalia o som atual"), d.getX() + 144, d.getY() + 52, d.getWidth() - 150, 26, juce::Justification::centredLeft);
        g.setColour (theme::dim()); g.setFont (theme::font (11.0f)); g.drawFittedText (dnaText, d.getX() + 12, d.getY() + 84, d.getWidth() - 24, 128, juce::Justification::topLeft, 9);
        // estado A/B e tarefa em curso
        const int ab = proc.abState();
        if (ab) { g.setColour (theme::accent()); g.setFont (theme::font (11.5f, true)); g.drawText (TX ("A/B ativo: a editar ") + juce::String (ab == 1 ? "A" : "B") + "  (A: " + proc.abName (0) + " \xC2\xB7 B: " + proc.abName (1) + ")", d.getX() + 12, d.getBottom() - 56, d.getWidth() - 24, 16, juce::Justification::centredLeft); }
        if (busy) { g.setColour (theme::warn()); g.setFont (theme::font (11.5f, true)); g.drawText (busyText, d.getX() + 12, d.getBottom() - 36, d.getWidth() - 24, 16, juce::Justification::centredLeft); }
    }
    void resized() override
    {
        panel = { 12, 104, 780, 552 };
        const int x = panel.getX() + 14, y = panel.getY(); const int cw = 160, gap = 10;
        closeBtn.setBounds (panel.getRight() - 46, y + 8, 32, 26);
        search.setBounds (x, y + 42, 500, 32);
        packBox.setBounds (x, y + 82, cw, 28); catBox.setBounds (x + cw + gap, y + 82, cw, 28); engBox.setBounds (x + 2 * (cw + gap), y + 82, cw, 28);
        qaBox.setBounds (x, y + 116, cw, 28); tempoBox.setBounds (x + cw + gap, y + 116, cw, 28); tonBox.setBounds (x + 2 * (cw + gap), y + 116, cw, 28);
        starBox.setBounds (x, y + 150, cw, 28); sortBox.setBounds (x + cw + gap, y + 150, cw, 28); modeBtn.setBounds (x + 2 * (cw + gap), y + 150, cw, 28);
        list.setBounds (x, y + 188, 500, panel.getHeight() - 188 - 34);
        auto d = detailArea(); const int bw = (d.getWidth() - 24 - 8) / 2, bh = 30; int by = d.getY() + 220;
        juce::TextButton* bs[] = { &similarBtn, &forYouBtn, &matchBtn, &assistBtn, &layerBtn, &macrosBtn, &abBtn, &saveBtn };
        for (int i = 0; i < 8; ++i) bs[i]->setBounds (d.getX() + 12 + (i % 2) * (bw + 8), by + (i / 2) * (bh + 7), bw, bh);
        splitTgl.setBounds (d.getX() + 12, by + 4 * (bh + 7) + 2, d.getWidth() - 24, 24);
    }
    void mouseDown (const juce::MouseEvent& e) override
    {
        if (! panel.contains (e.getPosition())) { close(); return; }
        const auto d = detailArea();   // clique nas estrelas do detalhe
        if (e.y >= d.getY() + 52 && e.y < d.getY() + 78 && e.x >= d.getX() + 10 && e.x < d.getX() + 140) rateCurrent ((e.x - (d.getX() + 10)) / 26 + 1);
    }
    bool keyPressed (const juce::KeyPress& k) override
    {
        if (k == juce::KeyPress::escapeKey) { close(); return true; }
        if (k == juce::KeyPress::downKey || k == juce::KeyPress::upKey) { list.keyPressed (k); return true; }
        return false;
    }

private:
    enum class Mode { List, Similar, ForYou, Match, Layer, Gen };
    juce::Rectangle<int> detailArea() const { return { panel.getX() + 526, panel.getY() + 42, 240, panel.getHeight() - 42 - 14 }; }
    void say (const juce::String& m, bool e) { if (toast) toast (m, e); }

    // ---- estado do preset atual
    juce::String curName() const { return proc.currentPresetName(); }
    const sk::Preset* curListPreset() const { const int i = proc.currentPresetIndex(); const auto& L = proc.presetList(); return (i >= 0 && i < (int) L.size() && L[(size_t) i].name == proc.currentPresetName().toStdString()) ? &L[(size_t) i] : nullptr; }
    int curRating() const { auto* p = curListPreset(); return p ? Ratings::get().of (p->pack, p->name) : 0; }
    void rateCurrent (int stars)
    {
        auto* p = curListPreset(); if (! p) { say (TX ("Guarda o som como preset para o poderes avaliar."), false); return; }
        const int old = Ratings::get().of (p->pack, p->name); Ratings::get().set (p->pack, p->name, old == stars ? 0 : stars);
        repaint(); list.repaint(); if (starBox.getSelectedId() > 1 || sortBox.getSelectedId() == 3) rebuildKeepMode();
    }
    void timerCallback() override { refreshDetail(); }
    void refreshDetail()
    {
        const auto live = proc.capturePreset (proc.currentPresetName()); const auto d = sk::dna::of (live); const auto t = sk::dna::text (d);
        juce::String sub; if (auto* p = curListPreset()) { PackEntry pe; sub = (PackHub::get().entry (p->pack, pe) && ! pe.name.empty() ? TX (pe.name.c_str()) : juce::String (p->pack)) + "  \xC2\xB7  " + TX (p->category.c_str()); }
        else sub = TX ("som atual (n\xC3\xA3o guardado)");
        const auto key = juce::String (t) + sub + juce::String (proc.abState()) + juce::String (busy ? 1 : 0) + proc.currentPresetName();
        if (key != lastDetailKey) { lastDetailKey = key; dnaText = TX (t.c_str()); detailSub = sub; repaint(); }
    }

    // ---- filtros
    void buildFilters()
    {
        const int pk = packBox.getSelectedId(); const juce::String ctText = catBox.getText();
        packBox.clear (juce::dontSendNotification); catBox.clear (juce::dontSendNotification); packIds.clear();
        packBox.addItem (TX ("Todos os packs"), 1); packBox.addItem (TX ("\xE2\x98\x85 Favoritos"), 2); int id = 3;
        for (auto& e : PackHub::get().entries()) { packBox.addItem (TX (e.name.c_str()), id++); packIds.push_back (e.id); }
        packBox.addItem ("Utilizador", id); packIds.push_back ("user");
        packBox.setSelectedId (pk > 0 ? pk : 1, juce::dontSendNotification);
        juce::StringArray cats; for (auto& p : proc.presetList()) { const auto c = TX (p.category.c_str()); if (c.isNotEmpty() && ! cats.contains (c)) cats.add (c); }
        cats.sort (true); catBox.addItem (TX ("Todas as categorias"), 1); for (int i = 0; i < cats.size(); ++i) catBox.addItem (cats[i], i + 2);
        int sel = 1; for (int i = 0; i < cats.size(); ++i) if (cats[i] == ctText) sel = i + 2;
        catBox.setSelectedId (sel, juce::dontSendNotification);
    }
    struct Filt { int pk, engSel, qaSel, tempoSel, tonSel, minStars; juce::String cat; juce::StringArray tokens; };
    Filt currentFilt() const
    {
        static const int stars[5] = { 0, 1, 3, 4, 5 };
        Filt f { packBox.getSelectedId(), engBox.getSelectedId(), qaBox.getSelectedId(), tempoBox.getSelectedId(), tonBox.getSelectedId(), stars[std::max (0, std::min (4, starBox.getSelectedId() - 1))], catBox.getSelectedId() > 1 ? catBox.getText() : juce::String(), {} };
        f.tokens.addTokens (search.getText().toLowerCase(), " ", ""); f.tokens.removeEmptyStrings(); return f;
    }
    bool passes (const Filt& f, const sk::Preset& p) const
    {
        if (f.pk == 2 && ! Favourites::get().has (p.pack, p.name)) return false;
        if (f.pk >= 3 && (size_t) (f.pk - 3) < packIds.size() && p.pack != packIds[(size_t) (f.pk - 3)]) return false;
        if (f.cat.isNotEmpty() && TX (p.category.c_str()) != f.cat) return false;
        if (f.engSel > 1 && engineKey (p) != f.engSel) return false;
        if (f.qaSel == 2 && p.qa == "auto") return false;
        if (f.qaSel == 3 && p.qa != "auto") return false;
        if (f.minStars > 0 && Ratings::get().of (p.pack, p.name) < f.minStars) return false;
        if (f.tempoSel > 1 || f.tonSel > 1)
        {
            const auto d = sk::dna::of (p);
            if (f.tempoSel == 2 && ! d.tempoSync) return false; if (f.tempoSel == 3 && d.tempoSync) return false;
            if (f.tonSel == 2 && d.tonality != "maior") return false; if (f.tonSel == 3 && d.tonality != "menor") return false;
            if (f.tonSel == 4 && d.tonality != std::string ("diat\xC3\xB3nico")) return false; if (f.tonSel == 5 && d.tonality != "livre") return false;
        }
        if (! f.tokens.isEmpty())
        {
            juce::String hay = TX (p.name.c_str()) + " " + TX (p.category.c_str()) + " " + TX (p.style.c_str()) + " " + TX (p.pack.c_str()) + " " + p.family + " " + p.subfamily + " " + p.role + " " + p.genre;
            for (auto& t : p.tags) hay << " " << TX (t.c_str());
            hay = hay.toLowerCase(); for (auto& t : f.tokens) if (! hay.contains (t)) return false;
        }
        return true;
    }
    std::function<bool (const sk::Preset&)> acceptFn() const { const Filt f = currentFilt(); return [this, f] (const sk::Preset& p) { return passes (f, p); }; }

    void toList() { mode = Mode::List; modeBtn.setButtonText (TX ("")); modeBtn.setEnabled (false); modeBtn.setVisible (false); rebuild(); }
    void rebuildKeepMode() { if (mode == Mode::List) rebuild(); else list.repaint(); }
    void rebuild()
    {
        if (mode != Mode::List) { list.updateContent(); list.repaint(); repaint(); return; }
        rows.clear(); rowScore.clear(); const auto& L = proc.presetList(); const Filt f = currentFilt();
        for (size_t i = 0; i < L.size(); ++i) if (passes (f, L[i])) rows.push_back ((int) i);
        const int so = sortBox.getSelectedId();
        if (so >= 2)
        {
            auto key = [&] (int i) -> long long
            {
                const auto& p = L[(size_t) i];
                if (so == 3) return -(long long) Ratings::get().of (p.pack, p.name);
                if (so == 4) return Favourites::get().has (p.pack, p.name) ? 0 : 1;
                if (so == 5) return sk::dna::of (p).cpuCost;
                return 0;
            };
            std::vector<std::pair<long long, int>> k; k.reserve (rows.size()); for (int i : rows) k.push_back ({ key (i), i });
            std::stable_sort (k.begin(), k.end(), [&] (const std::pair<long long, int>& a, const std::pair<long long, int>& b)
            { if (so == 2) return TX (L[(size_t) a.second].name.c_str()).compareIgnoreCase (TX (L[(size_t) b.second].name.c_str())) < 0; return a.first < b.first; });
            for (size_t i = 0; i < k.size(); ++i) rows[i] = k[i].second;
        }
        list.updateContent(); list.repaint(); repaint();
    }
    void showHits (Mode m, const juce::String& title, const std::vector<sk::search::Hit>& hits)
    {
        mode = m; rows.clear(); rowScore.clear(); modeTitle = title;
        for (auto& h : hits) { rows.push_back (h.index); rowScore.push_back (m == Mode::ForYou ? 0.0f : 1.0f / (1.0f + 1.2f * std::max (0.0f, h.dist))); }
        if (m == Mode::Layer || m == Mode::ForYou) rowScore.clear();
        modeBtn.setEnabled (true); modeBtn.setVisible (true); modeBtn.setButtonText (TX ("\xE2\x9C\x95 ") + title.substring (0, 18));
        if (rows.empty()) say (TX ("Sem resultados com estes filtros \xE2\x80\x94 afrouxa os filtros (pack, categoria, motor\xE2\x80\xA6)."), false);
        list.updateContent(); list.scrollToEnsureRowIsOnscreen (0); list.repaint(); repaint();
    }
    // motor do preset (2 analógico, 3 wavetable, 4 sample, 5 híbrido, 6 físico, 7 módulo, 8 granular, 9 espectral) — deriva dos parâmetros nos presets antigos
    static int engineKey (const sk::Preset& p)
    {
        if (! p.module.empty()) return 7;
        const int ta = (int) p.get (P_A_TYPE), tb = (int) p.get (P_B_TYPE); const bool bOn = p.get (P_B_ON) > 0.5f;
        if (ta == 3 || (bOn && tb == 3)) return 6;
        if (ta == 4 || (bOn && tb == 4)) return 8;
        if (ta == 5 || (bOn && tb == 5)) return 9;
        if (bOn && tb != ta) return 5;
        return ta == 1 ? 3 : (ta == 2 ? 4 : 2);
    }
    void textEditorTextChanged (juce::TextEditor&) override { if (mode == Mode::List) rebuild(); }
    void textEditorEscapeKeyPressed (juce::TextEditor&) override { close(); }
    void textEditorReturnKeyPressed (juce::TextEditor&) override { if (! rows.empty()) activate (0); }

    // ---- tarefas em segundo plano (medir o som, descodificar áudio, calibrar): o resultado volta à thread de mensagens
    using Finish = std::function<void (PresetBrowser&)>;
    void runAsync (const juce::String& text, std::function<Finish()> work)
    {
        if (busy) { say (TX ("Aguarda: ainda h\xC3\xA1 uma an\xC3\xA1lise em curso."), false); return; }
        busy = true; busyText = text; refreshDetail(); repaint();
        juce::Component::SafePointer<PresetBrowser> self (this);
        std::thread ([self, work = std::move (work)] { Finish fin = work(); juce::MessageManager::callAsync ([self, fin] { if (self == nullptr) return; self->busy = false; self->repaint(); if (fin) fin (*self); }); }).detach();
    }
    // som atual → impressão digital (medida no motor, por isso reflete também as edições)
    template <class F> void withCurrentFingerprint (const juce::String& text, F&& onFp)
    {
        auto lib = proc.currentLibrary(); sk::Preset live = proc.capturePreset (proc.currentPresetName());
        runAsync (text, [lib, live, onFp]() -> Finish
        {
            sk::Preset q = live; sk::presets::Fingerprint fp; static const sk::Library empty;
            sk::presets::measure (q, lib ? *lib : empty, &fp);
            return [fp, onFp] (PresetBrowser& b) { if (! fp.ok) b.say (TX ("O som atual est\xC3\xA1 mudo \xE2\x80\x94 toca uma nota/escolhe um preset com som."), true); else onFp (b, fp); };
        });
    }

    // ---- ações
    void doSimilar()
    {
        const int skip = proc.currentPresetIndex();
        withCurrentFingerprint (TX ("A medir o som atual\xE2\x80\xA6"), [skip] (PresetBrowser& b, const sk::search::Fingerprint& fp)
        { b.showHits (Mode::Similar, TX ("Parecidos com ") + b.curName(), sk::search::nearest (fp, b.proc.presetList(), 80, skip, b.acceptFn())); });
    }
    void doForYou()
    {
        const auto& L = proc.presetList(); std::vector<std::pair<int, float>> liked;
        for (size_t i = 0; i < L.size(); ++i)
        {
            float w = 0; const int r = Ratings::get().of (L[i].pack, L[i].name); if (r >= 3) w += (float) (r - 2); if (Favourites::get().has (L[i].pack, L[i].name)) w += 1.5f;
            if (w > 0) liked.push_back ({ (int) i, w });
        }
        if (liked.empty()) { say (TX ("Marca favoritos (\xE2\x99\xA5) ou d\xC3\xA1 3+ estrelas a alguns presets para receber recomenda\xC3\xA7\xC3\xB5" "es."), false); return; }
        auto hits = sk::search::recommend (liked, L, 80, acceptFn());
        showHits (Mode::ForYou, TX ("Para ti (") + juce::String ((int) liked.size()) + TX (" gostos)"), hits);
    }
    void doMatchChoose()
    {
        chooseAudioFile (chooser, TX ("Sound Match \xE2\x80\x94 escolhe um WAV de refer\xC3\xAAncia"), [this] (const juce::File& f) { soundMatch (f); });
    }
    void doAssistant()
    {
        if (! svc || ! svc->prompt) return;
        svc->prompt (TX ("\xE2\x9C\xA6 Assistente de sound design"), TX ("Descreve a mudan\xC3\xA7""a: escuro \xC2\xB7 brilhante \xC2\xB7 largo \xC2\xB7 curto \xC2\xB7 quente \xC2\xB7 movimento \xC2\xB7 cat\xC3\xA9" "dral \xC2\xB7 lo-fi \xC2\xB7 camada et\xC3\xA9reo\xE2\x80\xA6 (vazio = varia\xC3\xA7\xC3\xB5" "es livres)"), juce::String(), TX ("CRIAR VARIA\xC3\x87\xC3\x95" "ES"),
            [this] (const juce::String& txt) -> juce::String
            {
                const sk::Preset base = proc.capturePreset (proc.currentPresetName()); const std::string text = txt.trim().toStdString(); const uint32_t seed = ++assistSeed; auto lib = proc.currentLibrary();
                runAsync (TX ("A criar e calibrar varia\xC3\xA7\xC3\xB5" "es\xE2\x80\xA6"), [base, text, seed, lib]() -> Finish
                {
                    std::string rep; auto vs = sk::assist::variations (base, text, 8, seed, &rep); static const sk::Library empty;
                    std::vector<sk::Preset> out; std::vector<std::string> notes;
                    for (auto& v : vs) { sk::Preset q = v.preset; sk::presets::calibrate (q, lib ? *lib : empty); out.push_back (std::move (q)); notes.push_back (v.note); }
                    return [out, notes, rep, text] (PresetBrowser& b)
                    {
                        if (out.empty()) { b.say (TX ("N\xC3\xA3o consegui criar variações deste som \xE2\x80\x94 tenta outras palavras."), false); return; }
                        b.gen = out; b.genNote = notes; b.mode = Mode::Gen; b.modeTitle = TX ("Varia\xC3\xA7\xC3\xB5" "es: ") + (text.empty() ? juce::String ("livres") : TX (text.c_str()));
                        b.rows.clear(); b.rowScore.clear(); for (size_t i = 0; i < out.size(); ++i) b.rows.push_back ((int) i);
                        b.modeBtn.setEnabled (true); b.modeBtn.setVisible (true); b.modeBtn.setButtonText (TX ("\xE2\x9C\x95 ") + b.modeTitle.substring (0, 18));
                        b.list.updateContent(); b.list.repaint(); b.repaint(); b.say (TX (rep.c_str()), false);
                    };
                });
                return {};
            });
    }
    void doLayerOpen()
    {
        const sk::Preset base = proc.capturePreset (proc.currentPresetName());
        if (base.get (P_B_ON) > 0.5f || base.get (P_A_ON) < 0.5f) { say (TX ("O som atual j\xC3\xA1 usa os dois motores. Escolhe um preset de um s\xC3\xB3 motor para construir uma camada."), false); return; }
        layerBase = base; const int skip = proc.currentPresetIndex();
        withCurrentFingerprint (TX ("A procurar parceiros\xE2\x80\xA6"), [skip] (PresetBrowser& b, const sk::search::Fingerprint& fp)
        {
            auto hits = sk::assist::suggestPartners (b.layerBase, fp, b.proc.presetList(), 40, skip, b.acceptFn());
            b.showHits (Mode::Layer, TX ("Camada com ") + b.curName(), hits);
            if (! hits.empty()) b.say (TX ("Clica num parceiro para juntar. Liga \"dividir o teclado\" para um som grave/agudo."), false);
        });
    }
    void doLayerBuild (int presetIdx)
    {
        const auto& L = proc.presetList(); if (presetIdx < 0 || presetIdx >= (int) L.size()) return;
        const sk::Preset partner = L[(size_t) presetIdx], base = layerBase; const bool split = splitTgl.getToggleState(); auto lib = proc.currentLibrary();
        runAsync (TX ("A construir a camada\xE2\x80\xA6"), [partner, base, split, lib]() -> Finish
        {
            sk::Preset out; if (! sk::assist::buildLayer (base, partner, split ? sk::assist::LAYER_SPLIT : sk::assist::LAYER_STACK, 60, out)) return [] (PresetBrowser& b) { b.say (TX ("N\xC3\xA3o foi poss\xC3\xADvel juntar estes dois sons."), true); };
            static const sk::Library empty; sk::presets::calibrate (out, lib ? *lib : empty);
            return [out] (PresetBrowser& b) { b.proc.loadPreset (out, -1); b.say (TX ("Camada: ") + TX (out.name.c_str()), false); b.refreshDetail(); };
        });
    }
    void doMacros()
    {
        sk::Preset p = proc.capturePreset (proc.currentPresetName()); auto made = sk::assist::autoMacros (p);
        if (made.empty()) { say (TX ("Nada a acrescentar: as macros j\xC3\xA1 est\xC3\xA3o atribu\xC3\xAD" "das ou o som n\xC3\xA3o tem destinos adequados."), false); return; }
        proc.loadPreset (p, -1);
        juce::String t = TX ("Macros criadas: "); for (size_t i = 0; i < made.size(); ++i) t << (i ? " \xC2\xB7 " : "") << TX (made[i].name.c_str()) << "\xE2\x86\x92" << TX (made[i].target.c_str());
        say (t, false); refreshDetail();
    }
    void doSave()
    {
        if (! svc || ! svc->prompt) return;
        svc->prompt (TX ("Guardar preset"), TX ("Aparece em \"Utilizador\". Etiquetas opcionais depois de # \xE2\x80\x94 ex.: Meu Lead #lead, brilhante"), proc.currentPresetName() == "Init" ? juce::String() : proc.currentPresetName(), TX ("GUARDAR"),
            [this] (const juce::String& txt) -> juce::String
            {
                const auto hash = txt.indexOfChar ('#'); const auto n = (hash >= 0 ? txt.substring (0, hash) : txt).trim();
                juce::StringArray tags; if (hash >= 0) tags.addTokens (txt.substring (hash + 1).replaceCharacter ('#', ','), ",", "");
                juce::String err; if (! proc.saveUserPreset (n, err, tags)) return err;
                say (TX ("Preset guardado."), false); return {};
            });
    }
    void activate (int row)
    {
        if (row < 0 || row >= (int) rows.size()) return;
        const int idx = rows[(size_t) row];
        if (mode == Mode::Gen) { if (idx >= 0 && idx < (int) gen.size()) { proc.loadPreset (gen[(size_t) idx], -1); refreshDetail(); } }
        else if (mode == Mode::Layer) doLayerBuild (idx);
        else { proc.loadPresetIndex (idx); refreshDetail(); }
    }

    // ---- lista
    int getNumRows() override { return (int) rows.size(); }
    const sk::Preset* rowPreset (int row) const
    {
        if (row < 0 || row >= (int) rows.size()) return nullptr; const int idx = rows[(size_t) row];
        if (mode == Mode::Gen) return idx >= 0 && idx < (int) gen.size() ? &gen[(size_t) idx] : nullptr;
        const auto& L = proc.presetList(); return idx >= 0 && idx < (int) L.size() ? &L[(size_t) idx] : nullptr;
    }
    void paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool selected) override
    {
        const auto* pp = rowPreset (row); if (! pp) return; const auto& p = *pp; const int idx = rows[(size_t) row];
        if (selected) { g.setColour (theme::accent().withAlpha (0.18f)); g.fillRect (0, 0, w, h); g.setColour (theme::accent()); g.fillRect (0, 0, 3, h); }
        else if (mode != Mode::Gen && idx == proc.currentPresetIndex()) { g.setColour (juce::Colour (0x14FFFFFF)); g.fillRect (0, 0, w, h); }
        g.setColour (juce::Colour (0x10FFFFFF)); g.drawHorizontalLine (h - 1, 0.0f, (float) w);
        PackEntry pe; juce::Colour tag = theme::dim();
        if (PackHub::get().entry (p.pack, pe)) tag = juce::Colour::fromString ("FF" + juce::String (pe.accent).substring (1));
        g.setColour (tag); g.fillEllipse (12.0f, (float) h * 0.5f - 4.0f, 8.0f, 8.0f);
        const bool locked = mode != Mode::Gen && proc.presetIsLocked (p); const bool res = mode != Mode::List && mode != Mode::Gen;
        const int right = mode == Mode::Gen ? 10 : 140 + (res && ! rowScore.empty() ? 40 : 0);
        g.setColour (locked ? theme::dim() : theme::text()); g.setFont (theme::font (14.0f, true));
        g.drawText ((locked ? TX ("\xF0\x9F\x94\x92 ") : juce::String()) + TX (p.name.c_str()), 30, 3, w - 30 - right, 20, juce::Justification::centredLeft);
        juce::String sub;
        if (mode == Mode::Gen) { sub = TX (p.category.c_str()) + "   mudou: " + (idx >= 0 && idx < (int) genNote.size() ? TX (genNote[(size_t) idx].c_str()) : juce::String()); }
        else { sub = TX (p.category.c_str()); int n = 0; for (auto& t : p.tags) { if (n++ >= 3) break; sub << "  \xC2\xB7  " << TX (t.c_str()); } sub = (pe.name.empty() ? juce::String (p.pack) : TX (pe.name.c_str())) + "   " + sub; }
        g.setColour (theme::dim()); g.setFont (theme::font (11.5f)); g.drawText (sub, 30, 21, w - 30 - right, 16, juce::Justification::centredLeft);
        if (mode == Mode::Gen) return;
        if (res && ! rowScore.empty() && row < (int) rowScore.size()) { g.setColour (theme::accent2()); g.setFont (theme::font (12.0f, true)); g.drawText (juce::String ((int) std::lround (100.0f * rowScore[(size_t) row])) + "%", w - 184, 0, 44, h, juce::Justification::centredRight); }
        const int r = Ratings::get().of (p.pack, p.name);
        for (int i = 0; i < 5; ++i) { g.setColour (i < r ? theme::warn() : juce::Colour (0x33FFFFFF)); g.setFont (theme::font (15.0f)); g.drawText (i < r ? TX ("\xE2\x98\x85") : TX ("\xE2\x98\x86"), w - 136 + i * 19, 0, 19, h, juce::Justification::centred); }
        const bool fav = Favourites::get().has (p.pack, p.name);
        g.setColour (fav ? theme::bad() : juce::Colour (0x55FFFFFF)); g.setFont (theme::font (18.0f)); g.drawText (fav ? TX ("\xE2\x99\xA5") : TX ("\xE2\x99\xA1"), w - 38, 0, 32, h, juce::Justification::centred);
    }
    void listBoxItemClicked (int row, const juce::MouseEvent& e) override
    {
        const auto* pp = rowPreset (row); if (! pp) return; const auto& p = *pp;
        if (mode != Mode::Gen)
        {
            if (e.x > list.getWidth() - 40) { Favourites::get().toggle (p.pack, p.name); if (mode == Mode::List && (packBox.getSelectedId() == 2 || sortBox.getSelectedId() == 4)) rebuild(); else list.repaint(); return; }
            if (e.x >= list.getWidth() - 136 && e.x < list.getWidth() - 40)
            {
                const int s = (e.x - (list.getWidth() - 136)) / 19 + 1; const int old = Ratings::get().of (p.pack, p.name); Ratings::get().set (p.pack, p.name, old == s ? 0 : s);
                if (mode == Mode::List && (starBox.getSelectedId() > 1 || sortBox.getSelectedId() == 3)) rebuild(); else list.repaint(); repaint(); return;
            }
        }
        activate (row);
    }
    void listBoxItemDoubleClicked (int, const juce::MouseEvent&) override { if (mode == Mode::List) close(); }
    void selectedRowsChanged (int row) override { if (! suppress && mode != Mode::Layer && row >= 0 && row < (int) rows.size()) activate (row); }

    SynthKeyProcessor& proc; std::function<void (const juce::String&, bool)> toast; Services* svc;
    juce::TextEditor search; juce::ComboBox packBox, catBox, engBox, qaBox, tempoBox, tonBox, starBox, sortBox; juce::ListBox list; juce::TextButton closeBtn, modeBtn;
    juce::TextButton similarBtn, forYouBtn, matchBtn, assistBtn, layerBtn, macrosBtn, abBtn, saveBtn; juce::ToggleButton splitTgl;
    juce::Rectangle<int> panel { 12, 104, 780, 552 }; std::vector<int> rows; std::vector<float> rowScore; std::vector<std::string> packIds; bool suppress = false;
    Mode mode = Mode::List; juce::String modeTitle, dnaText, detailSub, busyText, lastDetailKey; bool busy = false; uint32_t assistSeed = 100;
    std::vector<sk::Preset> gen; std::vector<std::string> genNote; sk::Preset layerBase; std::shared_ptr<juce::FileChooser> chooser;
};
} // namespace ui
