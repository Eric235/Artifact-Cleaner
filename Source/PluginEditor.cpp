#include "PluginProcessor.h"
#include "PluginEditor.h"

ArtifactCleanerAudioProcessorEditor::ArtifactCleanerAudioProcessorEditor(ArtifactCleanerAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    // De-Chirp
    dechirpSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    dechirpSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
    addAndMakeVisible(dechirpSlider);
    dechirpAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "dechirp", dechirpSlider);

    dechirpLabel.setText("De-Chirp", juce::dontSendNotification);
    dechirpLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(dechirpLabel);

    // Punch
    punchSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    punchSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
    addAndMakeVisible(punchSlider);
    punchAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "punch", punchSlider);

    punchLabel.setText("Punch", juce::dontSendNotification);
    punchLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(punchLabel);

    // Focus
    focusSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    focusSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
    addAndMakeVisible(focusSlider);
    focusAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "focus", focusSlider);

    focusLabel.setText("Focus", juce::dontSendNotification);
    focusLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(focusLabel);

    // Analog Life
    analogSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    analogSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
    addAndMakeVisible(analogSlider);
    analogAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "analog", analogSlider);

    analogLabel.setText("Analog Life", juce::dontSendNotification);
    analogLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(analogLabel);

    // Setup Buttons
    addAndMakeVisible(loadButton);
    addAndMakeVisible(playButton);
    addAndMakeVisible(stopButton);

    loadButton.onClick = [this]()
    {
        fileChooser = std::make_unique<juce::FileChooser>("Select an Audio File", juce::File{}, "*.wav;*.mp3;*.aiff");
        fileChooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [this](const juce::FileChooser& fc)
            {
                auto file = fc.getResult();
                if (file.existsAsFile())
                    audioProcessor.loadFile(file);
            });
    };

    playButton.onClick = [this]() { audioProcessor.play(); };
    stopButton.onClick = [this]() { audioProcessor.stop(); };

    setSize(600, 320); // Made slightly taller to fit buttons
}

ArtifactCleanerAudioProcessorEditor::~ArtifactCleanerAudioProcessorEditor()
{
}

void ArtifactCleanerAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::darkgrey);
    g.setColour(juce::Colours::white);
    g.setFont(24.0f);
    g.drawText("Artifact Cleaner", 0, 10, getWidth(), 40, juce::Justification::centred, true);
}

void ArtifactCleanerAudioProcessorEditor::resized()
{
    int knobWidth = 100;
    int knobHeight = 100;
    int spacing = 30;

    // Center the 4 knobs dynamically
    int startX = (getWidth() - (4 * knobWidth + 3 * spacing)) / 2;
    int startY = 80;

    // Position De-Chirp
    dechirpSlider.setBounds(startX, startY, knobWidth, knobHeight);
    dechirpLabel.setBounds(startX, startY - 25, knobWidth, 20);

    // Position Punch
    startX += knobWidth + spacing;
    punchSlider.setBounds(startX, startY, knobWidth, knobHeight);
    punchLabel.setBounds(startX, startY - 25, knobWidth, 20);

    // Position Focus
    startX += knobWidth + spacing;
    focusSlider.setBounds(startX, startY, knobWidth, knobHeight);
    focusLabel.setBounds(startX, startY - 25, knobWidth, 20);

    // Position Analog Life
    startX += knobWidth + spacing;
    analogSlider.setBounds(startX, startY, knobWidth, knobHeight);
    analogLabel.setBounds(startX, startY - 25, knobWidth, 20);

    // Position Buttons
    int buttonWidth = 120;
    int buttonHeight = 40;
    int buttonY = 240;
    int buttonStartX = (getWidth() - (3 * buttonWidth + 2 * spacing)) / 2;

    loadButton.setBounds(buttonStartX, buttonY, buttonWidth, buttonHeight);
    playButton.setBounds(buttonStartX + buttonWidth + spacing, buttonY, buttonWidth, buttonHeight);
    stopButton.setBounds(buttonStartX + 2 * (buttonWidth + spacing), buttonY, buttonWidth, buttonHeight);
}