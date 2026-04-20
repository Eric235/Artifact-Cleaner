#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class ArtifactCleanerAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    ArtifactCleanerAudioProcessorEditor(ArtifactCleanerAudioProcessor&);
    ~ArtifactCleanerAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    ArtifactCleanerAudioProcessor& audioProcessor;

    // Sliders
    juce::Slider dechirpSlider;
    juce::Slider punchSlider;
    juce::Slider focusSlider;
    juce::Slider analogSlider;

    // Labels
    juce::Label dechirpLabel;
    juce::Label punchLabel;
    juce::Label focusLabel;
    juce::Label analogLabel;

    // Buttons for Audio Player
    juce::TextButton loadButton{ "Load WAV/MP3" };
    juce::TextButton playButton{ "Play" };
    juce::TextButton stopButton{ "Stop" };

    // File chooser
    std::unique_ptr<juce::FileChooser> fileChooser;

    // APVTS Attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> dechirpAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> punchAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> focusAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> analogAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ArtifactCleanerAudioProcessorEditor)
};