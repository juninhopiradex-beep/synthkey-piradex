// =============================================================================
//  SYNTHKEY by Piradex — processador de áudio
// =============================================================================
#pragma once
#include "Core/Grooves.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <array>
#include <atomic>
#include <memory>
#include "Access.h"
#include "PackHub.h"
#include "Core/Engine.h"
#include "Core/MidiFx.h"
#include "Core/Preset.h"

class SynthKeyProcessor : public juce::AudioProcessor, private juce::ChangeListener, private juce::Timer
{
public:
    SynthKeyProcessor();
    ~SynthKeyProcessor() override;

    // ---- AudioProcessor ----
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& l) const override { return l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo(); }
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "SYNTHKEY by Piradex"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    // ---- parâmetros ----
    juce::AudioProcessorValueTreeState apvts;
    juce::RangedAudioParameter* param (int idx) const { return params[(size_t) idx]; }
    float getReal (int idx) const { auto* r = raw[(size_t) idx]; return r ? r->load() : sk::kParams[idx].def; }
    void setReal (int idx, float v);                     // define pelo valor real (com gesto para o host)

    // ---- samples (instrumentos dos motores A/B) ----
    juce::String getSampleId (int engine) const;
    void setSampleId (int engine, const juce::String& id);
    bool sampleMissing (int engine) const { return missing[(size_t) engine].load(); }
    juce::String sampleDisplayName (int engine) const;
    std::shared_ptr<const sk::Library> currentLibrary() const;

    // ---- presets ----
    void refreshPresetList();
    const std::vector<sk::Preset>& presetList() const { return presets; }
    int currentPresetIndex() const { return currentPreset; }
    juce::String currentPresetName() const { return presetName; }
    void loadPreset (const sk::Preset& p, int indexInList = -1);
    void loadPresetIndex (int i);
    void stepPreset (int dir);
    void initPreset();
    bool saveUserPreset (const juce::String& name, juce::String& error);
    sk::Preset capturePreset (const juce::String& name) const;
    bool presetIsLocked (const sk::Preset& p) const;      // usa instrumentos de um pack bloqueado?
    void randomizePatch (float amount);

    // ---- medidores / estado para a interface ----
    std::atomic<float> meterL { 0 }, meterR { 0 };
    // buffer circular (mono) para o osciloscópio e o espectro; escrito na thread de áudio, lido pela interface
    static constexpr int kScopeSize = 8192;
    float scopeBuf[kScopeSize] = {};
    std::atomic<int> scopeWrite { 0 };
    double sampleRateNow() const { return sr; }
    std::atomic<int> voiceCount { 0 };
    std::atomic<bool> previewMode { false }, previewExpired { false };
    std::atomic<float> previewSecondsLeft { 0 };
    juce::MidiKeyboardState keyboardState;
    sk::GroovePlayer groove;                              // reprodutor de grooves (injeta notas no MIDI de entrada)
    std::function<void()> onPresetLoaded;                // chamado na thread de mensagens

    // propriedades de interface guardadas no estado do plugin
    int getUiInt (const char* key, int def) const { return (int) apvts.state.getProperty (key, def); }
    void setUiInt (const char* key, int v) { apvts.state.setProperty (key, v, nullptr); }

private:
    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void timerCallback() override;
    void rebuildLibrary();
    void resolveSamples();
    void updatePreviewState();
    static std::string fallbackFor (const std::string& id);

    std::array<juce::RangedAudioParameter*, sk::P_COUNT> params {};
    std::array<std::atomic<float>*, sk::P_COUNT> raw {};
    float prm[sk::P_COUNT] = {};
    std::unique_ptr<sk::Engine> engine;
    sk::MidiFx midiFx;
    std::vector<sk::NoteEvent> evs;
    std::vector<sk::GMidiEvent> grooveEvs;
    double sr = 44100.0; int engineMax = 512;
    float modWheel = 0, sustainPedal = 0;

    // biblioteca: o objeto fica na thread de mensagens; a thread de áudio só vê um ponteiro
    mutable juce::CriticalSection libLock;
    std::shared_ptr<const sk::Library> combined;
    std::vector<std::shared_ptr<const sk::Library>> retired;
    std::atomic<const sk::Library*> pendingLib { nullptr };
    std::atomic<const sk::Library*> engineLib { nullptr };
    std::atomic<int> smpIdx[2] = { { 0 }, { 0 } };
    std::atomic<bool> missing[2] = { { false }, { false } };
    juce::String sampleId[2];
    std::atomic<int> previewLeftSamples { 0 };
    float previewGain = 1.0f;

    std::vector<sk::Preset> presets; int currentPreset = -1; juce::String presetName { "Init" };
    bool loadingPreset = false;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SynthKeyProcessor)
};
