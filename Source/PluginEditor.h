// =============================================================================
//  SYNTHKEY by Piradex — editor (interface gráfica)
// =============================================================================
#pragma once
#include "PluginProcessor.h"
#include "UI/Look.h"

class MainView;

class SynthKeyEditor : public juce::AudioProcessorEditor, public juce::FileDragAndDropTarget
{
public:
    explicit SynthKeyEditor (SynthKeyProcessor& p);
    ~SynthKeyEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    bool isInterestedInFileDrag (const juce::StringArray& files) override;
    void filesDropped (const juce::StringArray& files, int, int) override;
    SynthKeyProcessor& proc;
private:
    SynthLook look;
    std::unique_ptr<MainView> view;
    static constexpr int kW = 1180, kH = 760; bool ready = false;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SynthKeyEditor)
};
