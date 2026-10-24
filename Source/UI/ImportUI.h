// =============================================================================
//  SYNTHKEY by Piradex — interface de importação de áudio (wavetable / sample do utilizador)
// =============================================================================
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include "../PluginProcessor.h"
#include "../PackHub.h"
#include "../UserAudio.h"
#include "ModRing.h"

namespace ui
{
using namespace sk;

inline void importWavetableFile (SynthKeyProcessor& p, int E, const juce::File& f)
{
    const auto err = p.importWavetable (f);
    if (err.isNotEmpty()) { say (err, true); return; }
    const int o = E ? (P_B_ON - P_A_ON) : 0;
    p.setReal (P_A_ON + o, 1.0f); p.setReal (P_A_TYPE + o, 1.0f); p.setReal (P_A_WTTAB + o, (float) Wavetables::kUserTable);
    say (juce::String (juce::CharPointer_UTF8 ("Wavetable importada: ")) + f.getFileNameWithoutExtension(), false);
}

inline void importSampleFile (SynthKeyProcessor& p, int E, const juce::File& f)
{
    std::string id; const auto err = PackHub::get().importUserSample (f, &id);
    if (err.isNotEmpty()) { say (err, true); return; }
    const int o = E ? (P_B_ON - P_A_ON) : 0;
    p.setReal (P_A_ON + o, 1.0f); p.setReal (P_A_TYPE + o, 2.0f); p.setSampleId (E, juce::String (id));
    say (juce::String (juce::CharPointer_UTF8 ("Sample importado: ")) + f.getFileNameWithoutExtension(), false);
}

inline void chooseAudioFile (std::shared_ptr<juce::FileChooser>& keep, const juce::String& title, std::function<void (const juce::File&)> done)
{
    keep = std::make_shared<juce::FileChooser> (title, juce::File::getSpecialLocation (juce::File::userMusicDirectory), "*.wav;*.aif;*.aiff;*.flac");
    keep->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles, [done] (const juce::FileChooser& fc)
    { const auto f = fc.getResult(); if (f != juce::File()) done (f); });
}

// botão "WAV…" ao lado do seletor de tabelas: importa um ficheiro como wavetable do motor
class WavetableImportButton : public juce::TextButton
{
public:
    WavetableImportButton (SynthKeyProcessor& p, int engine) : proc (p), E (engine)
    {
        setButtonText ("WAV");
        setTooltip (juce::String (juce::CharPointer_UTF8 ("Importar um WAV como wavetable (ciclos de 2048/1024/512/256 amostras, um ciclo \xC3\xBAnico ou qualquer som)")));
        onClick = [this] { chooseAudioFile (chooser, juce::String (juce::CharPointer_UTF8 ("Importar wavetable (WAV)")), [this] (const juce::File& f) { importWavetableFile (proc, E, f); }); };
    }
private:
    SynthKeyProcessor& proc; int E; std::shared_ptr<juce::FileChooser> chooser;
};

// menu "largar um ficheiro de áudio na janela": o que fazer com ele
inline void showDropMenu (SynthKeyProcessor& p, juce::Component* target, const juce::File& f)
{
    juce::PopupMenu m; m.addSectionHeader (f.getFileName());
    m.addItem (1, juce::String (juce::CharPointer_UTF8 ("Wavetable \xE2\x86\x92 Motor A")));
    m.addItem (2, juce::String (juce::CharPointer_UTF8 ("Wavetable \xE2\x86\x92 Motor B")));
    m.addSeparator();
    m.addItem (3, juce::String (juce::CharPointer_UTF8 ("Instrumento (sample) \xE2\x86\x92 Motor A")));
    m.addItem (4, juce::String (juce::CharPointer_UTF8 ("Instrumento (sample) \xE2\x86\x92 Motor B")));
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (target), [&p, f] (int r)
    {
        if (r == 1 || r == 2) importWavetableFile (p, r - 1, f);
        else if (r == 3 || r == 4) importSampleFile (p, r - 3, f);
    });
}
}
