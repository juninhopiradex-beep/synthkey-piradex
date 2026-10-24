// =============================================================================
//  SYNTHKEY by Piradex — ecrã ESTÚDIO DE CONTEÚDO (dentro da aba ADMIN, só administrador)
//  Larga WAV / loops / MIDI, escolhe estilo e categoria e gera o pack .bfpack protegido.
// =============================================================================
#pragma once
#include "Controls.h"
#include "../Studio.h"

namespace ui
{
class StudioPanel : public juce::Component, private juce::ListBoxModel, public juce::FileDragAndDropTarget
{
public:
    StudioPanel (std::function<void (const juce::String&, bool)> toastFn,
                 std::function<void (const juce::String&, const juce::String&, const juce::String&, const juce::String&, std::function<juce::String (const juce::String&)>)> promptFn)
        : toast (std::move (toastFn)), prompt (std::move (promptFn))
    {
        for (auto* c : { (juce::Component*) &projBox, (juce::Component*) &styleBox, (juce::Component*) &list }) addAndMakeVisible (c);
        for (auto* b : { &backBtn, &newBtn, &addWav, &addMulti, &addLoops, &addMidi, &addMine, &addPresets, &removeBtn, &catBtn, &nameBtn, &coverBtn, &genBtn, &installBtn, &openBtn }) addAndMakeVisible (b);
        for (auto* e : { &nameEd, &versionEd, &accentEd, &descEd }) { addAndMakeVisible (e); e->setFont (theme::font (13.5f)); }
        for (auto* t : { &kitTgl, &presetTgl }) addAndMakeVisible (t);
        backBtn.setButtonText (TX ("\xE2\x97\x82 CHAVES")); backBtn.onClick = [this] { if (onBack) onBack(); }; newBtn.setButtonText (TX ("NOVO PACK")); addWav.setButtonText (TX ("+ WAV (um por ficheiro)")); addMulti.setButtonText (TX ("+ MULTI-SAMPLE")); addLoops.setButtonText ("+ LOOPS");
        addMidi.setButtonText (TX ("+ MIDI \xE2\x86\x92 groove")); addMine.setButtonText (TX ("+ MEUS GROOVES")); addPresets.setButtonText (TX ("+ MEUS PRESETS"));
        removeBtn.setButtonText ("REMOVER"); catBtn.setButtonText (TX ("CATEGORIA\xE2\x80\xA6")); nameBtn.setButtonText (TX ("NOME\xE2\x80\xA6")); coverBtn.setButtonText (TX ("CAPA\xE2\x80\xA6"));
        genBtn.setButtonText (TX ("GERAR PACK (.bfpack)")); installBtn.setButtonText (TX ("INSTALAR NO PLUGIN")); openBtn.setButtonText (TX ("ABRIR PASTA"));
        kitTgl.setButtonText (TX ("Criar kit de bateria automaticamente (pelos nomes: kick, snare, hat\xE2\x80\xA6)")); presetTgl.setButtonText (TX ("Criar um preset para cada instrumento"));
        for (auto* s : { "trap", "rnb", "afrobeat", "amapiano", "drill", "kizomba", "kuduro" }) styleIds.add (s);
        const char* names[7] = { "Trap", "R&B", "Afrobeats", "Amapiano", "Drill", "Kizomba", "Kuduro" }; for (int i = 0; i < 7; ++i) styleBox.addItem (names[i], i + 1);
        list.setModel (this); list.setRowHeight (28); list.setColour (juce::ListBox::backgroundColourId, juce::Colour (0xFF0A0C18)); list.setOutlineThickness (0);
        projBox.onChange = [this] { const int id = projBox.getSelectedId(); if (id > 0 && id - 1 < (int) projects.size()) openProject (projects[(size_t) id - 1]); };
        styleBox.onChange = [this] { P.style = styleIds[styleBox.getSelectedId() - 1].toStdString(); commit(); };
        for (auto* e : { &nameEd, &versionEd, &accentEd, &descEd }) e->onTextChange = [this] { readFields(); };
        nameEd.setTextToShowWhenEmpty (TX ("Nome do pack"), theme::dim()); versionEd.setTextToShowWhenEmpty ("1.0.0", theme::dim());
        accentEd.setTextToShowWhenEmpty ("#A58BFF", theme::dim()); descEd.setTextToShowWhenEmpty (TX ("Descri\xC3\xA7\xC3\xA3o curta (aparece na carta do pack)"), theme::dim());
        accentEd.setInputRestrictions (7, "#0123456789abcdefABCDEF");
        kitTgl.onClick = [this] { P.autoKit = kitTgl.getToggleState(); commit(); }; presetTgl.onClick = [this] { P.autoPresets = presetTgl.getToggleState(); commit(); };
        newBtn.onClick = [this] { newProject(); };
        addWav.onClick = [this] { chooseAudio (sk::studio::ImportMode::OneShots); }; addMulti.onClick = [this] { chooseAudio (sk::studio::ImportMode::MultiSample); }; addLoops.onClick = [this] { chooseAudio (sk::studio::ImportMode::Loops); };
        addMidi.onClick = [this] { chooseMidi(); }; addMine.onClick = [this] { pickMyGroove(); }; addPresets.onClick = [this] { pickPresets(); };
        removeBtn.onClick = [this] { removeSelected(); }; catBtn.onClick = [this] { changeCategory(); }; nameBtn.onClick = [this] { renameSelected(); };
        coverBtn.onClick = [this] { chooseCover(); }; genBtn.onClick = [this] { generate(); }; installBtn.onClick = [this] { install(); };
        openBtn.onClick = [this] { P.dir().revealToUser(); };
        genBtn.setTooltip (TX ("Cria o ficheiro .bfpack com os samples cifrados. Depois gera as chaves na aba CHAVES.")); installBtn.setTooltip (TX ("Instala o \xC3\xBAltimo pack gerado neste computador para o testares e gerares chaves"));
        refreshProjects();
        if (projects.empty()) { P.id = "meu-pack"; P.name = "Meu Pack"; P.save(); refreshProjects(); }
        openProject (projects.empty() ? P.id : projects.front());
    }
    void paint (juce::Graphics& g) override
    {
        g.fillAll (theme::bg());
        theme::drawCard (g, left.toFloat()); theme::drawCard (g, right.toFloat());
        g.setColour (theme::accent()); g.fillRoundedRectangle (left.getX() + 10.0f, 9.0f, 3.0f, 12.0f, 1.5f); g.fillRoundedRectangle (right.getX() + 10.0f, 9.0f, 3.0f, 12.0f, 1.5f);
        g.setFont (theme::font (11.5f, true)); g.setColour (theme::text());
        g.drawText (TX ("EST\xC3\x9A" "DIO DE CONTE\xC3\x9A" "DO  \xC2\xB7  pack"), left.getX() + 18, 5, 360, 20, juce::Justification::centredLeft);
        g.drawText (TX ("CONTE\xC3\x9A" "DO DO PACK  \xC2\xB7  larga aqui ficheiros WAV / MIDI"), right.getX() + 18, 5, 560, 20, juce::Justification::centredLeft);
        g.setColour (theme::dim()); g.setFont (theme::font (10.5f, true));
        g.drawText ("ID", left.getX() + 18, 66, 40, 14, juce::Justification::centredLeft); g.drawText (TX ("ESTILO"), left.getX() + 18, 126, 60, 14, juce::Justification::centredLeft);
        g.drawText ("COR", left.getX() + 232, 126, 40, 14, juce::Justification::centredLeft); g.drawText (TX ("VERS\xC3\x83" "O"), left.getX() + 232, 174, 70, 14, juce::Justification::centredLeft);
        g.setFont (theme::font (12.0f, true)); g.setColour (theme::accent()); g.drawText (juce::String (P.id) + "   \xC2\xB7   " + TX ("c\xC3\xB3pias dos teus ficheiros ficam no projeto"), left.getX() + 40, 64, 330, 18, juce::Justification::centredLeft);
        // capa
        auto cv = juce::Rectangle<float> ((float) left.getX() + 18, 306.0f, 104.0f, 104.0f);
        g.setColour (juce::Colour (0xFF0A0C18)); g.fillRoundedRectangle (cv, 6.0f);
        if (cover.isValid()) g.drawImage (cover, cv.reduced (2.0f), juce::RectanglePlacement::centred | juce::RectanglePlacement::onlyReduceInSize);
        else { g.setColour (theme::dim()); g.setFont (theme::font (11.0f)); g.drawText (TX ("sem capa"), cv.toNearestInt(), juce::Justification::centred); }
        g.setColour (juce::Colour::fromString ("FF" + juce::String (P.accent).trimCharactersAtStart ("#"))); g.fillRoundedRectangle ((float) left.getX() + 328, 148.0f, 20.0f, 16.0f, 4.0f);
        g.setColour (theme::dim()); g.setFont (theme::font (11.5f)); g.drawFittedText (statusText, left.getX() + 130, 306, 232, 104, juce::Justification::topLeft, 7);
        g.drawText (TX ("Os samples ficam cifrados no pack. Para venderes: gera o pack, instala, e emite chaves na aba CHAVES."), right.getX() + 18, 424, 590, 18, juce::Justification::centredLeft);
        if (rows.empty()) { g.setColour (theme::dim()); g.setFont (theme::font (13.0f)); g.drawFittedText (TX ("Larga aqui ficheiros WAV (um instrumento por ficheiro) ou MIDI (grooves).\nOu usa os bot\xC3\xB5" "es acima: multi-sample para um instrumento tocado em v\xC3\xA1rias notas, loops para loops de \xC3\xA1udio."), right.getX() + 60, 150, right.getWidth() - 120, 120, juce::Justification::centred, 4); }
        if (dropHover) { g.setColour (theme::accent().withAlpha (0.18f)); g.fillRoundedRectangle (right.toFloat(), 10.0f); g.setColour (theme::accent()); g.drawRoundedRectangle (right.toFloat().reduced (1.0f), 10.0f, 2.0f); }
    }
    void resized() override
    {
        left = { 0, 0, 380, 452 }; right = { 388, 0, 768, 452 };
        const int x = left.getX() + 18, w = left.getWidth() - 36;
        backBtn.setBounds (right.getRight() - 110, 2, 100, 26); projBox.setBounds (x, 30, w - 100, 28); newBtn.setBounds (x + w - 92, 30, 92, 28);
        nameEd.setBounds (x, 84, w, 28); styleBox.setBounds (x, 142, 160, 28); accentEd.setBounds (x + 214, 142, 90, 28);
        versionEd.setBounds (x + 214, 190, 90, 28); descEd.setBounds (x, 228, w, 28);
        kitTgl.setBounds (x, 262, w, 22); presetTgl.setBounds (x, 284, w, 22);
        coverBtn.setBounds (x, 414, 104, 28);
        genBtn.setBounds (x + 120, 414, 230, 28);
        const int rx = right.getX() + 14, rw = right.getWidth() - 28;
        addWav.setBounds (rx, 30, 170, 28); addMulti.setBounds (rx + 176, 30, 130, 28); addLoops.setBounds (rx + 312, 30, 80, 28);
        addMidi.setBounds (rx + 398, 30, 128, 28); addMine.setBounds (rx + 532, 30, 112, 28);
        addPresets.setBounds (rx + 650, 30, rw - 650, 28);
        list.setBounds (rx, 66, rw, 304);
        removeBtn.setBounds (rx, 378, 90, 28); catBtn.setBounds (rx + 96, 378, 110, 28); nameBtn.setBounds (rx + 212, 378, 90, 28);
        installBtn.setBounds (rx + rw - 330, 378, 170, 28); openBtn.setBounds (rx + rw - 154, 378, 154, 28);
    }
    // ---- arrastar ficheiros
    bool isInterestedInFileDrag (const juce::StringArray& f) override { for (auto& s : f) if (isAudio (s) || isMidi (s)) return true; return false; }
    void fileDragEnter (const juce::StringArray&, int, int) override { dropHover = true; repaint(); }
    void fileDragExit (const juce::StringArray&) override { dropHover = false; repaint(); }
    void filesDropped (const juce::StringArray& f, int, int) override
    {
        dropHover = false; juce::Array<juce::File> wavs, mids;
        for (auto& s : f) { if (isAudio (s)) wavs.add (juce::File (s)); else if (isMidi (s)) mids.add (juce::File (s)); }
        if (! wavs.isEmpty()) addAudioFiles (wavs, sk::studio::ImportMode::OneShots);
        if (! mids.isEmpty()) addMidiFiles (mids);
        repaint();
    }
    std::function<void()> onBack;
    void tick() { static int n = 0; if (++n % 20 == 0) refreshStatus(); }

private:
    static bool isAudio (const juce::String& s) { return s.endsWithIgnoreCase (".wav") || s.endsWithIgnoreCase (".aif") || s.endsWithIgnoreCase (".aiff") || s.endsWithIgnoreCase (".flac"); }
    static bool isMidi (const juce::String& s) { return s.endsWithIgnoreCase (".mid") || s.endsWithIgnoreCase (".midi"); }
    struct Row { int type = 0; int idx = 0; };   // 0 instrumento, 1 groove, 2 preset
    // ---- projeto
    void refreshProjects()
    {
        projects = sk::studio::Project::listProjects(); projBox.clear (juce::dontSendNotification);
        for (size_t i = 0; i < projects.size(); ++i) projBox.addItem (TX (projects[i].c_str()), (int) i + 1);
    }
    void openProject (const std::string& id)
    {
        sk::studio::Project p; if (sk::studio::Project::load (id, p)) P = std::move (p);
        for (size_t i = 0; i < projects.size(); ++i) if (projects[i] == P.id) projBox.setSelectedId ((int) i + 1, juce::dontSendNotification);
        loadFields(); rebuildRows(); refreshStatus();
    }
    void loadFields()
    {
        nameEd.setText (TX (P.name.c_str()), false); versionEd.setText (TX (P.version.c_str()), false); accentEd.setText (TX (P.accent.c_str()), false); descEd.setText (TX (P.desc.c_str()), false);
        const int si = styleIds.indexOf (juce::String (P.style)); styleBox.setSelectedId (si >= 0 ? si + 1 : 1, juce::dontSendNotification);
        kitTgl.setToggleState (P.autoKit, juce::dontSendNotification); presetTgl.setToggleState (P.autoPresets, juce::dontSendNotification);
        cover = P.coverFile.empty() ? juce::Image() : juce::ImageFileFormat::loadFrom (P.dir().getChildFile (juce::String (P.coverFile))); repaint();
    }
    void readFields()
    {
        P.name = nameEd.getText().trim().toStdString(); P.version = versionEd.getText().trim().toStdString(); P.accent = accentEd.getText().trim().toStdString(); P.desc = descEd.getText().trim().toStdString();
        if (P.version.empty()) P.version = "1.0.0"; if (P.accent.size() != 7 || P.accent[0] != '#') P.accent = "#A58BFF";
        P.save(); repaint();
    }
    void commit() { P.save(); rebuildRows(); refreshStatus(); }
    void newProject()
    {
        if (! prompt) return;
        prompt (TX ("Novo pack"), TX ("Nome do pack (ex.: Trap Vol. 2). O id e a pasta s\xC3\xA3o criados a partir do nome."), TX ("Meu Pack"), TX ("Criar"), [this] (const juce::String& nm) -> juce::String
        {
            const auto n = nm.trim(); if (n.isEmpty()) return TX ("Escolhe um nome.");
            sk::studio::Project p; p.name = n.toStdString(); std::string base = sk::studio::slugify (n); if (base.size() < 3) base += "-pack"; std::string id = base; int k = 2;
            while (sk::studio::Project::root().getChildFile (juce::String (id)).isDirectory()) id = base + "-" + std::to_string (k++);
            p.id = id; p.save(); refreshProjects(); openProject (id); return {};
        });
    }
    // ---- importar
    void chooseAudio (sk::studio::ImportMode mode)
    {
        chooser = std::make_shared<juce::FileChooser> (TX ("Escolhe os ficheiros de \xC3\xA1udio"), juce::File::getSpecialLocation (juce::File::userMusicDirectory), "*.wav;*.aif;*.aiff;*.flac");
        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::canSelectMultipleItems, [this, mode] (const juce::FileChooser& fc)
        { auto r = fc.getResults(); if (r.isEmpty()) return; juce::Array<juce::File> a; for (auto& f : r) a.add (f); addAudioFiles (a, mode); });
    }
    void addAudioFiles (const juce::Array<juce::File>& files, sk::studio::ImportMode mode)
    {
        if (mode == sk::studio::ImportMode::MultiSample && prompt)
        {
            prompt (TX ("Multi-sample"), TX ("Nome do instrumento (as notas s\xC3\xA3o detetadas em cada ficheiro e o teclado \xC3\xA9 distribu\xC3\xAD" "do entre eles)."), TX ("Piano"), TX ("Criar"),
                [this, files] (const juce::String& nm) -> juce::String { if (nm.trim().isEmpty()) return TX ("Escolhe um nome."); doAddAudio (files, sk::studio::ImportMode::MultiSample, nm.trim()); return {}; });
            return;
        }
        doAddAudio (files, mode, {});
    }
    void doAddAudio (const juce::Array<juce::File>& files, sk::studio::ImportMode mode, const juce::String& group)
    {
        const size_t before = P.insts.size(); auto warn = P.addAudio (files, mode, group); rebuildRows(); refreshStatus();
        if (toast) toast (juce::String ((int) (P.insts.size() - before)) + TX (" instrumento(s) adicionado(s)") + (warn.isEmpty() ? juce::String() : TX ("  \xC2\xB7  ignorados: ") + warn.joinIntoString ("; ")), ! warn.isEmpty() && P.insts.size() == before);
    }
    void chooseMidi()
    {
        chooser = std::make_shared<juce::FileChooser> (TX ("Escolhe ficheiros MIDI"), juce::File::getSpecialLocation (juce::File::userMusicDirectory), "*.mid;*.midi");
        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::canSelectMultipleItems, [this] (const juce::FileChooser& fc)
        { auto r = fc.getResults(); if (r.isEmpty()) return; juce::Array<juce::File> a; for (auto& f : r) a.add (f); addMidiFiles (a); });
    }
    void addMidiFiles (const juce::Array<juce::File>& files)
    {
        int ok = 0; juce::StringArray bad;
        for (auto& f : files) { sk::Groove g; juce::String err; if (sk::GrooveStore::importMidi (f, P.style, g, err)) { P.grooves.push_back (std::move (g)); ++ok; } else bad.add (f.getFileName() + ": " + err); }
        commit(); if (toast) toast (juce::String (ok) + TX (" groove(s) adicionado(s)") + (bad.isEmpty() ? juce::String() : TX ("  \xC2\xB7  ") + bad.joinIntoString ("; ")), ok == 0);
    }
    void pickMyGroove()
    {
        juce::PopupMenu m; int i = 1; std::vector<const sk::Groove*> v;
        for (auto& g : sk::GrooveStore::get().userGrooves()) { v.push_back (&g); m.addItem (i++, TX (g.name.c_str()) + "   (" + TX (g.kind.c_str()) + ")"); }
        if (v.empty()) { if (toast) toast (TX ("Ainda n\xC3\xA3o tens grooves teus. Cria-os ou importa MIDI na aba GROOVES."), true); return; }
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&addMine), [this, v] (int r) { if (r < 1) return; P.grooves.push_back (*v[(size_t) r - 1]); commit(); });
    }
    void pickPresets()
    {
        juce::PopupMenu m; auto ups = PackHub::get().userPresets(); std::vector<std::string> names; int i = 1;
        for (auto& p : ups) { const bool in = std::find (P.extraPresets.begin(), P.extraPresets.end(), p.name) != P.extraPresets.end(); names.push_back (p.name); m.addItem (i++, TX (p.name.c_str()), ! in, in); }
        if (names.empty()) { if (toast) toast (TX ("Ainda n\xC3\xA3o guardaste presets teus (menu \xE2\x80\xA2\xE2\x80\xA2\xE2\x80\xA2 \xE2\x86\x92 Guardar preset)."), true); return; }
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&addPresets), [this, names] (int r) { if (r < 1) return; P.extraPresets.push_back (names[(size_t) r - 1]); commit(); });
    }
    void chooseCover()
    {
        chooser = std::make_shared<juce::FileChooser> (TX ("Escolhe a capa (imagem)"), juce::File::getSpecialLocation (juce::File::userPicturesDirectory), "*.jpg;*.jpeg;*.png");
        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles, [this] (const juce::FileChooser& fc)
        {
            const auto f = fc.getResult(); if (f == juce::File()) return;
            const auto dst = P.dir().getChildFile ("capa" + f.getFileExtension().toLowerCase()); if (dst != f) f.copyFileTo (dst);
            P.coverFile = dst.getFileName().toStdString(); P.save(); loadFields();
        });
    }
    // ---- lista
    void rebuildRows()
    {
        rows.clear(); for (size_t i = 0; i < P.insts.size(); ++i) rows.push_back ({ 0, (int) i });
        for (size_t i = 0; i < P.grooves.size(); ++i) rows.push_back ({ 1, (int) i }); for (size_t i = 0; i < P.extraPresets.size(); ++i) rows.push_back ({ 2, (int) i });
        list.updateContent(); list.repaint();
    }
    int getNumRows() override { return (int) rows.size(); }
    void paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool sel) override
    {
        if (row < 0 || row >= (int) rows.size()) return; const auto r = rows[(size_t) row];
        if (sel) { g.setColour (theme::accent().withAlpha (0.18f)); g.fillRect (0, 0, w, h); }
        g.setColour (juce::Colour (0x10FFFFFF)); g.drawHorizontalLine (h - 1, 0.0f, (float) w);
        juce::String tag, name, cat, info; juce::Colour tc = theme::accent();
        if (r.type == 0) { const auto& in = P.insts[(size_t) r.idx]; tag = in.cat == "Loops" ? "LOOP" : (in.fixed ? "ONE-SHOT" : (in.zones.size() > 1 ? "MULTI" : "TONAL")); name = TX (in.name.c_str()); cat = TX (in.cat.c_str());
                       info = juce::String ((int) in.zones.size()) + (in.zones.size() == 1 ? TX (" zona") : TX (" zonas")); if (! in.fixed && in.zones.size() == 1) info += "  \xC2\xB7  " + juce::MidiMessage::getMidiNoteName (in.zones[0].root, true, true, 3); }
        else if (r.type == 1) { const auto& gr = P.grooves[(size_t) r.idx]; tag = "GROOVE"; tc = theme::accent2(); name = TX (gr.name.c_str()); cat = TX (gr.kind.c_str()); info = juce::String (gr.bpm) + " bpm  \xC2\xB7  " + juce::String (gr.bars) + (gr.bars == 1 ? TX (" compasso") : TX (" compassos")); }
        else { tag = "PRESET"; tc = juce::Colour (0xFFFFB45F); name = TX (P.extraPresets[(size_t) r.idx].c_str()); cat = TX ("preset do utilizador"); }
        g.setColour (tc); g.setFont (theme::font (10.0f, true)); g.drawText (tag, 10, 0, 70, h, juce::Justification::centredLeft);
        g.setColour (theme::text()); g.setFont (theme::font (13.0f, true)); g.drawText (name, 84, 0, w - 380, h, juce::Justification::centredLeft);
        g.setColour (theme::dim()); g.setFont (theme::font (11.5f)); g.drawText (cat, w - 290, 0, 120, h, juce::Justification::centredLeft); g.drawText (info, w - 166, 0, 156, h, juce::Justification::centredRight);
    }
    void deleteKeyPressed (int) override { removeSelected(); }
    void removeSelected()
    {
        const int s = list.getSelectedRow(); if (s < 0 || s >= (int) rows.size()) return; const auto r = rows[(size_t) s];
        if (r.type == 0) P.insts.erase (P.insts.begin() + r.idx); else if (r.type == 1) P.grooves.erase (P.grooves.begin() + r.idx); else P.extraPresets.erase (P.extraPresets.begin() + r.idx);
        commit();
    }
    void changeCategory()
    {
        const int s = list.getSelectedRow(); if (s < 0 || s >= (int) rows.size() || rows[(size_t) s].type != 0) { if (toast) toast (TX ("Escolhe um instrumento na lista."), true); return; }
        static const char* cats[] = { "Bateria", "Percuss\xC3\xA3o", "Baixo", "Teclas", "Pads", "Vozes", "Mallets", "Sintetizador", "Loops", "FX" };
        juce::PopupMenu m; for (int i = 0; i < 10; ++i) m.addItem (i + 1, TX (cats[i]));
        const int idx = rows[(size_t) s].idx;
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&catBtn), [this, idx] (int r) { if (r < 1 || idx >= (int) P.insts.size()) return; P.insts[(size_t) idx].cat = cats[r - 1]; if (std::string (cats[r - 1]) == "Loops") P.insts[(size_t) idx].fixed = true; commit(); });
    }
    void renameSelected()
    {
        const int s = list.getSelectedRow(); if (s < 0 || s >= (int) rows.size() || ! prompt) return; const auto r = rows[(size_t) s]; if (r.type == 2) return;
        const juce::String cur = r.type == 0 ? TX (P.insts[(size_t) r.idx].name.c_str()) : TX (P.grooves[(size_t) r.idx].name.c_str());
        prompt (TX ("Mudar nome"), TX ("Nome que aparece no plugin."), cur, TX ("Guardar"), [this, r] (const juce::String& nm) -> juce::String
        {
            const auto n = nm.trim(); if (n.isEmpty()) return TX ("Escolhe um nome.");
            if (r.type == 0 && r.idx < (int) P.insts.size()) P.insts[(size_t) r.idx].name = n.toStdString(); else if (r.type == 1 && r.idx < (int) P.grooves.size()) P.grooves[(size_t) r.idx].name = n.toStdString();
            commit(); return {};
        });
    }
    // ---- gerar / instalar
    void refreshStatus()
    {
        int zones = 0; for (auto& in : P.insts) zones += (int) in.zones.size();
        statusText = juce::String ((int) P.insts.size()) + TX (" instrumentos  \xC2\xB7  ") + juce::String (zones) + TX (" samples\n") + juce::String ((int) P.grooves.size()) + " grooves  \xC2\xB7  " + juce::String ((int) P.extraPresets.size()) + TX (" presets extra")
                     + (lastSummary.isNotEmpty() ? TX ("\n\n\xE2\x9C\x93 \xC3\x9Altimo pack: ") + lastSummary : juce::String());
        repaint();
    }
    void generate()
    {
        readFields(); juce::String summary; genBtn.setEnabled (false);
        const auto err = P.exportPack (lastFile, summary); genBtn.setEnabled (true);
        if (err.isNotEmpty()) { if (toast) toast (err, true); return; }
        lastSummary = summary + "\n" + lastFile.getFullPathName(); refreshStatus();
        if (toast) toast (TX ("Pack gerado: ") + lastFile.getFileName() + "  (" + summary + ")", false);
    }
    void install()
    {
        if (! lastFile.existsAsFile()) { if (toast) toast (TX ("Gera o pack primeiro."), true); return; }
        const auto err = PackHub::get().install (lastFile);
        if (toast) toast (err.isEmpty() ? TX ("Pack instalado. J\xC3\xA1 podes gerar chaves para \"") + TX (P.id.c_str()) + TX ("\" na aba CHAVES.") : err, err.isNotEmpty());
    }

    std::function<void (const juce::String&, bool)> toast;
    std::function<void (const juce::String&, const juce::String&, const juce::String&, const juce::String&, std::function<juce::String (const juce::String&)>)> prompt;
    sk::studio::Project P; std::vector<std::string> projects; std::vector<Row> rows; juce::StringArray styleIds; juce::Image cover; juce::String statusText, lastSummary; juce::File lastFile;
    juce::Rectangle<int> left, right; bool dropHover = false; std::shared_ptr<juce::FileChooser> chooser;
    juce::ComboBox projBox, styleBox; juce::ListBox list; juce::TextEditor nameEd, versionEd, accentEd, descEd; juce::ToggleButton kitTgl, presetTgl;
    juce::TextButton backBtn, newBtn, addWav, addMulti, addLoops, addMidi, addMine, addPresets, removeBtn, catBtn, nameBtn, coverBtn, genBtn, installBtn, openBtn;
};
}
