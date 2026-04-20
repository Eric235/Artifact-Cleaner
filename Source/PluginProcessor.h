#pragma once

#include <JuceHeader.h>

class ArtifactCleanerAudioProcessor : public juce::AudioProcessor
{
public:
    ArtifactCleanerAudioProcessor();
    ~ArtifactCleanerAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
#endif

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // --- Audio Player Methods ---
    void loadFile(const juce::File& file);
    void play();
    void stop();

    // This object manages the parameters and saves/loads states
    juce::AudioProcessorValueTreeState apvts;

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // --- Audio Player Variables ---
    juce::AudioFormatManager formatManager;
    std::unique_ptr<juce::AudioFormatReaderSource> readerSource;
    juce::AudioTransportSource transportSource;

    // --- DSP Variables ---
    double currentSampleRate = 44100.0;

    // 1. Focus (Mid/Side)
    juce::dsp::IIR::Filter<float> sideHPF;

    // 2. Punch (Transient Shaper Envelope Followers)
    float punchFast = 0.0f;
    float punchSlow = 0.0f;
    float smoothPunchGain = 1.0f;

    // 3. De-Chirp (HF Transient Gate / Downward Expander)
    float lpfStateL = 0.0f;
    float lpfStateR = 0.0f;
    float dechirpFast = 0.0f;
    float dechirpSlow = 0.0f;
    float smoothDechirpGain = 1.0f;

    // 4. FFT Structural Variables (Kept for future expansion)
    static constexpr int fftOrder = 10;
    static constexpr int fftSize = 1 << fftOrder;
    juce::dsp::FFT forwardFFT;
    juce::dsp::WindowingFunction<float> window;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ArtifactCleanerAudioProcessor)
};