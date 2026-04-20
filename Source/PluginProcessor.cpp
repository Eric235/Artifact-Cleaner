#include "PluginProcessor.h"
#include "PluginEditor.h"

ArtifactCleanerAudioProcessor::ArtifactCleanerAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
#endif
    apvts(*this, nullptr, "Parameters", createParameterLayout()),
    forwardFFT(fftOrder),
    window(fftSize, juce::dsp::WindowingFunction<float>::hann)
{
    formatManager.registerBasicFormats();
}

ArtifactCleanerAudioProcessor::~ArtifactCleanerAudioProcessor()
{
}

juce::AudioProcessorValueTreeState::ParameterLayout ArtifactCleanerAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>("dechirp", "De-Chirp", 0.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("punch", "Punch", 0.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("focus", "Focus", 20.0f, 500.0f, 150.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("analog", "Analog Life", 0.0f, 1.0f, 0.0f));

    return { params.begin(), params.end() };
}

void ArtifactCleanerAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    transportSource.prepareToPlay(samplesPerBlock, sampleRate);

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = 1;

    sideHPF.prepare(spec);

    // Reset all states
    punchFast = 0.0f;
    punchSlow = 0.0f;
    smoothPunchGain = 1.0f;

    lpfStateL = 0.0f;
    lpfStateR = 0.0f;
    dechirpFast = 0.0f;
    dechirpSlow = 0.0f;
    smoothDechirpGain = 1.0f;
}

void ArtifactCleanerAudioProcessor::releaseResources()
{
    transportSource.releaseResources();
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool ArtifactCleanerAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    return true;
}
#endif

void ArtifactCleanerAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused(midiMessages);
    juce::ScopedNoDenormals noDenormals;

    if (readerSource.get() != nullptr && transportSource.isPlaying())
    {
        juce::AudioSourceChannelInfo audioInfo(&buffer, 0, buffer.getNumSamples());
        transportSource.getNextAudioBlock(audioInfo);
    }
    else
    {
        buffer.clear();
        return;
    }

    float dechirpParam = apvts.getRawParameterValue("dechirp")->load();
    float punchParam = apvts.getRawParameterValue("punch")->load();
    float focusParam = apvts.getRawParameterValue("focus")->load();
    float analogParam = apvts.getRawParameterValue("analog")->load();

    // Update Focus Filter
    sideHPF.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass(currentSampleRate, focusParam);

    // De-Chirp Crossover dropped to 2.5kHz to catch more harshness
    float crossoverAlpha = 1.0f - std::exp(-2.0f * juce::MathConstants<float>::pi * 2500.0f / currentSampleRate);

    float focusNormalized = (focusParam - 20.0f) / 480.0f;

    auto* left = buffer.getWritePointer(0);
    auto* right = buffer.getWritePointer(1);

    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        // --- INPUT GAIN STAGING (-12dB Pad) ---
        float l = left[sample] * 0.25f;
        float r = right[sample] * 0.25f;

        // --- A. FOCUS (Mid/Side Filtering & Obvious Mid Boost) ---
        float mid = (l + r) * 0.5f;
        float side = (l - r) * 0.5f;

        side = sideHPF.processSample(side);
        mid *= (1.0f + (focusNormalized * 1.0f));

        l = mid + side;
        r = mid - side;

        // --- B. DE-CHIRP (POWERFUL HF Downward Expander) ---
        // 1. Split audio at 2.5kHz
        lpfStateL += crossoverAlpha * (l - lpfStateL);
        lpfStateR += crossoverAlpha * (r - lpfStateR);

        float highL = l - lpfStateL;
        float highR = r - lpfStateR;

        // 2. Dual-envelope tracking
        float highMag = std::max(std::abs(highL), std::abs(highR));
        dechirpFast += 0.05f * (highMag - dechirpFast);
        dechirpSlow += 0.002f * (highMag - dechirpSlow);

        // 3. Ultra-sensitive Swish tracking (Multiplied by 1.5 to trigger faster)
        float isSwish = dechirpSlow / (dechirpFast + 0.0001f);
        isSwish = juce::jlimit(0.0f, 1.0f, isSwish * 1.5f);

        // 4. Massive ducking power (cuts up to 98% of the HF signal instead of 90%)
        float targetHighGain = 1.0f - (isSwish * dechirpParam * 0.98f);
        targetHighGain = std::max(0.02f, targetHighGain); // Hard floor at -34dB

        // Slightly faster reaction time (0.03f instead of 0.01f) so it chokes the noise tighter
        smoothDechirpGain += 0.03f * (targetHighGain - smoothDechirpGain);

        highL *= smoothDechirpGain;
        highR *= smoothDechirpGain;

        // 5. Mix perfectly back together
        l = lpfStateL + highL;
        r = lpfStateR + highR;

        // --- C. PUNCH (Smoothed Transient Shaper) ---
        float mag = std::max(std::abs(l), std::abs(r));
        punchFast += 0.05f * (mag - punchFast);
        punchSlow += 0.005f * (mag - punchSlow);

        float transient = std::max(0.0f, punchFast - punchSlow);

        float targetPunchGain = 1.0f + (transient * punchParam * 8.0f);
        smoothPunchGain += 0.01f * (targetPunchGain - smoothPunchGain);

        l *= smoothPunchGain;
        r *= smoothPunchGain;

        // --- D. ANALOG LIFE (Parallel Saturation) ---
        float drive = 2.0f;
        float saturatedL = std::tanh(l * drive) / drive;
        float saturatedR = std::tanh(r * drive) / drive;

        l = (l * (1.0f - analogParam)) + (saturatedL * analogParam);
        r = (r * (1.0f - analogParam)) + (saturatedR * analogParam);

        // --- OUTPUT GAIN STAGING & SAFETY LIMITER (+12dB Makeup) ---
        l *= 4.0f;
        r *= 4.0f;

        l = juce::jlimit(-1.0f, 1.0f, l);
        r = juce::jlimit(-1.0f, 1.0f, r);

        left[sample] = l;
        right[sample] = r;
    }
}

// --- Audio Player Methods ---

void ArtifactCleanerAudioProcessor::loadFile(const juce::File& file)
{
    if (auto* reader = formatManager.createReaderFor(file))
    {
        auto newSource = std::make_unique<juce::AudioFormatReaderSource>(reader, true);
        transportSource.setSource(newSource.get(), 0, nullptr, reader->sampleRate);

        readerSource.reset(newSource.release());
    }
}

void ArtifactCleanerAudioProcessor::play()
{
    if (readerSource != nullptr)
        transportSource.start();
}

void ArtifactCleanerAudioProcessor::stop()
{
    transportSource.stop();
    transportSource.setPosition(0.0);
}

// --- Boilerplate JUCE overrides ---

bool ArtifactCleanerAudioProcessor::hasEditor() const { return true; }
juce::AudioProcessorEditor* ArtifactCleanerAudioProcessor::createEditor() { return new ArtifactCleanerAudioProcessorEditor(*this); }
const juce::String ArtifactCleanerAudioProcessor::getName() const { return JucePlugin_Name; }
bool ArtifactCleanerAudioProcessor::acceptsMidi() const { return false; }
bool ArtifactCleanerAudioProcessor::producesMidi() const { return false; }
bool ArtifactCleanerAudioProcessor::isMidiEffect() const { return false; }
double ArtifactCleanerAudioProcessor::getTailLengthSeconds() const { return 0.0; }
int ArtifactCleanerAudioProcessor::getNumPrograms() { return 1; }
int ArtifactCleanerAudioProcessor::getCurrentProgram() { return 0; }

void ArtifactCleanerAudioProcessor::setCurrentProgram(int index) { juce::ignoreUnused(index); }
const juce::String ArtifactCleanerAudioProcessor::getProgramName(int index) { juce::ignoreUnused(index); return {}; }
void ArtifactCleanerAudioProcessor::changeProgramName(int index, const juce::String& newName) { juce::ignoreUnused(index, newName); }

void ArtifactCleanerAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void ArtifactCleanerAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ArtifactCleanerAudioProcessor();
}