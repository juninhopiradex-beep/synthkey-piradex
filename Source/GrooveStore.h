// =============================================================================
//  SYNTHKEY by Piradex — grooves do utilizador: guardar / carregar (.skgroove) e importar MIDI (.mid)
//  Pasta: <dados do utilizador>/MeusGrooves.  Os grooves de fábrica nunca são alterados:
//  editar um deles cria uma cópia do utilizador.
// =============================================================================
#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <algorithm>
#include <list>
#include <map>
#include "AppPaths.h"
#include "Core/Grooves.h"
#include "Core/GrooveJson.h"

namespace sk
{
class GrooveStore
{
public:
    static GrooveStore& get() { static GrooveStore s; return s; }
    static juce::File folder() { auto d = skpaths::userDir().getChildFile ("MeusGrooves"); d.createDirectory(); return d; }

    // fábrica + utilizador (ponteiros estáveis até à próxima remoção)
    std::vector<const Groove*> all() const
    {
        std::vector<const Groove*> v; for (auto& g : allGrooves()) v.push_back (&g);
        for (auto& g : packG) v.push_back (&g);
        for (auto& g : mine) v.push_back (&g);
        return v;
    }
    const std::list<Groove>& userGrooves() const { return mine; }
    void setPackGrooves (const std::vector<Groove>& v) { packG.assign (v.begin(), v.end()); }     // grooves dos packs instalados

    void reload()
    {
        mine.clear();
        auto files = folder().findChildFiles (juce::File::findFiles, false, "*.skgroove");
        files.sort();
        for (auto& f : files) { Groove g; if (fromJson (f.loadFileAsString().toStdString(), g)) { g.user = true; g.file = f.getFullPathName().toStdString(); mine.push_back (std::move (g)); } }
    }

    // grava (cria ou substitui) e devolve o groove guardado; 'g.file' vazio = ficheiro novo com nome único
    Groove* save (Groove g, juce::String* err = nullptr)
    {
        g.user = true; sanitize (g);
        juce::File f;
        if (! g.file.empty()) f = juce::File (juce::String::fromUTF8 (g.file.c_str()));
        else
        {
            const auto base = juce::File::createLegalFileName (juce::String::fromUTF8 (g.name.c_str())).trim();
            f = folder().getChildFile ((base.isEmpty() ? juce::String ("groove") : base) + ".skgroove"); int n = 2;
            while (f.existsAsFile()) f = folder().getChildFile (base + " " + juce::String (n++) + ".skgroove");
            g.file = f.getFullPathName().toStdString();
        }
        if (! f.replaceWithText (juce::String::fromUTF8 (toJson (g).c_str()))) { if (err) *err = juce::CharPointer_UTF8 ("N\xC3\xA3o foi poss\xC3\xADvel gravar o groove."); return nullptr; }
        for (auto& m : mine) if (m.file == g.file) { m = std::move (g); return &m; }
        mine.push_back (std::move (g)); return &mine.back();
    }
    bool remove (const Groove* g)
    {
        if (! g || ! g->user) return false;
        for (auto it = mine.begin(); it != mine.end(); ++it)
            if (&*it == g) { juce::File (juce::String::fromUTF8 (it->file.c_str())).deleteFile(); mine.erase (it); return true; }
        return false;
    }

    // ---------------------------------------------------------------- JSON (ver Core/GrooveJson.h)
    static std::string toJson (const Groove& g) { return groovejson::toJson (g); }
    static bool fromJson (const std::string& text, Groove& out) { return groovejson::fromJson (text, out); }
    static void sanitize (Groove& g) { groovejson::sanitize (g); }

    // ---------------------------------------------------------------- importar MIDI
    // Quantiza para semicolcheias. Deteta o tipo: bateria (canal 10), acordes (muitas notas em simultâneo), baixo (grave) ou melodia.
    static bool importMidi (const juce::File& f, const std::string& style, Groove& out, juce::String& err)
    {
        juce::FileInputStream in (f); juce::MidiFile mf;
        if (! in.openedOk() || ! mf.readFrom (in)) { err = juce::CharPointer_UTF8 ("N\xC3\xA3o consegui ler o ficheiro MIDI."); return false; }
        const int tf = mf.getTimeFormat(); if (tf <= 0) { err = juce::CharPointer_UTF8 ("Este MIDI usa tempo SMPTE (n\xC3\xA3o suportado)."); return false; }
        const double tps = (double) tf / 4.0;                       // ticks por semicolcheia
        double bpm = 120.0; bool gotTempo = false; std::vector<GNote> raw; int drumNotes = 0, total = 0; double endTick = 0;
        for (int t = 0; t < mf.getNumTracks(); ++t)
        {
            juce::MidiMessageSequence seq (*mf.getTrack (t)); seq.updateMatchedPairs();
            for (int i = 0; i < seq.getNumEvents(); ++i)
            {
                auto* e = seq.getEventPointer (i); const auto& m = e->message;
                if (m.isTempoMetaEvent() && ! gotTempo) { const double spq = m.getTempoSecondsPerQuarterNote(); if (spq > 0) { bpm = 60.0 / spq; gotTempo = true; } }
                if (! m.isNoteOn()) continue;
                const double t0 = m.getTimeStamp(), t1 = e->noteOffObject ? e->noteOffObject->message.getTimeStamp() : t0 + tps * 2.0;
                GNote n; n.step = (int) std::lround (t0 / tps); n.len = std::max (1, (int) std::lround ((t1 - t0) / tps)); n.note = m.getNoteNumber(); n.vel = std::max (1, (int) m.getVelocity());
                const bool drumCh = m.getChannel() == 10; ++total; if (drumCh) ++drumNotes;
                n.len = drumCh ? 1 : std::min (n.len, 64);
                raw.push_back (n); endTick = std::max (endTick, t1);
                if (drumCh) raw.back().step = -raw.back().step - 1;  // marca temporária: nota de bateria
            }
        }
        if (raw.empty()) { err = juce::CharPointer_UTF8 ("O MIDI n\xC3\xA3o tem notas."); return false; }
        const bool drums = drumNotes * 2 > total;
        Groove g; g.style = style.empty() ? "trap" : style; g.name = juce::File::createLegalFileName (f.getFileNameWithoutExtension()).toStdString(); g.user = true;
        if (g.name.empty()) g.name = "MIDI importado";
        g.bpm = (int) std::lround (juce::jlimit (40.0, 240.0, bpm));
        int maxStep = 0;
        for (auto n : raw)
        {
            const bool dn = n.step < 0; if (dn) n.step = -n.step - 1;
            if (drums != dn) continue;                                   // mistura de bateria e outros: fica só o tipo dominante
            if (drums) { const int mapped = mapDrum (n.note); if (mapped < 0) continue; n.note = mapped; }
            if (n.step >= 16 * 16) continue;
            maxStep = std::max (maxStep, n.step + n.len); g.notes.push_back (n);
        }
        if (g.notes.empty()) { err = juce::CharPointer_UTF8 ("Nenhuma nota aproveit\xC3\xA1vel no MIDI."); return false; }
        g.bars = std::max (1, (maxStep + 15) / 16); if (g.bars == 3) g.bars = 4; g.bars = std::min (16, g.bars);
        // tipo
        if (drums) g.kind = "Bateria";
        else
        {
            std::map<int, int> perStep; double sum = 0; for (auto& n : g.notes) { ++perStep[n.step]; sum += n.note; }
            int chordSteps = 0; for (auto& kv : perStep) if (kv.second >= 3) ++chordSteps;
            const double avg = sum / (double) g.notes.size();
            g.kind = chordSteps * 3 >= (int) perStep.size() && chordSteps > 0 ? "Acordes" : (avg < 52.0 ? "Baixo" : "Melodia");
        }
        (void) endTick; sanitize (g); out = std::move (g); return true;
    }
    // notas GM de bateria → as 11 pistas do plugin (kick, rim, caixa, palmas, tom, hat, shaker, hat aberto, congas, bongo)
    static int mapDrum (int n)
    {
        switch (n)
        {
            case 35: case 36: return 36;
            case 37: case 56: return 37;
            case 38: case 40: return 38;
            case 39: return 39;
            case 41: case 48: case 50: return 41;
            case 42: case 44: return 42;
            case 43: return 43;
            case 45: return 45;
            case 46: case 49: case 51: case 52: case 55: case 57: case 59: return 46;
            case 47: return 47;
            case 54: case 69: case 70: case 82: return 44;
            default: return -1;
        }
    }
private:
    GrooveStore() { reload(); }
    std::list<Groove> mine, packG;
};
}
