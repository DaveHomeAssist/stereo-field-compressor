#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace IDs
{
    constexpr const char* threshold  = "threshold";
    constexpr const char* ratio      = "ratio";
    constexpr const char* attack     = "attack";
    constexpr const char* release    = "release";
    constexpr const char* knee       = "knee";
    constexpr const char* makeup     = "makeup";
    constexpr const char* coneCenter = "coneCenter";
    constexpr const char* coneWidth  = "coneWidth";
    constexpr const char* mix        = "mix";
}

juce::AudioProcessorValueTreeState::ParameterLayout
StereoFieldCompressorProcessor::createParameterLayout()
{
    using P = juce::AudioParameterFloat;

    const auto halfPi = juce::MathConstants<float>::halfPi;
    const auto pi     = juce::MathConstants<float>::pi;

    return {
        std::make_unique<P> (juce::ParameterID { IDs::threshold, 1 }, "Threshold",
                             juce::NormalisableRange<float> (-60.0f, 0.0f, 0.1f), -18.0f),
        std::make_unique<P> (juce::ParameterID { IDs::ratio, 1 }, "Ratio",
                             juce::NormalisableRange<float> (1.0f, 20.0f, 0.1f, 0.4f), 4.0f),
        std::make_unique<P> (juce::ParameterID { IDs::attack, 1 }, "Attack",
                             juce::NormalisableRange<float> (0.1f, 200.0f, 0.1f, 0.3f), 10.0f),
        std::make_unique<P> (juce::ParameterID { IDs::release, 1 }, "Release",
                             juce::NormalisableRange<float> (5.0f, 2000.0f, 1.0f, 0.3f), 100.0f),
        std::make_unique<P> (juce::ParameterID { IDs::knee, 1 }, "Knee",
                             juce::NormalisableRange<float> (0.0f, 24.0f, 0.1f), 6.0f),
        std::make_unique<P> (juce::ParameterID { IDs::makeup, 1 }, "Makeup",
                             juce::NormalisableRange<float> (-12.0f, 24.0f, 0.1f), 0.0f),
        std::make_unique<P> (juce::ParameterID { IDs::coneCenter, 1 }, "Cone Center",
                             juce::NormalisableRange<float> (-halfPi, halfPi, 0.001f), 0.0f),
        std::make_unique<P> (juce::ParameterID { IDs::coneWidth, 1 }, "Cone Width",
                             juce::NormalisableRange<float> (0.01f, pi, 0.001f), pi * 0.5f),
        std::make_unique<P> (juce::ParameterID { IDs::mix, 1 }, "Mix",
                             juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 1.0f),
    };
}

StereoFieldCompressorProcessor::StereoFieldCompressorProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",     juce::AudioChannelSet::stereo(), true)
                          .withInput  ("Sidechain", juce::AudioChannelSet::stereo(), false)
                          .withOutput ("Output",    juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createParameterLayout())
{
}

bool StereoFieldCompressorProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto main = layouts.getMainInputChannelSet();
    if (main != juce::AudioChannelSet::stereo())
        return false;
    if (layouts.getMainOutputChannelSet() != main)
        return false;

    const auto sc = layouts.getChannelSet (true, 1);
    if (! sc.isDisabled() && sc != juce::AudioChannelSet::stereo())
        return false;

    return true;
}

void StereoFieldCompressorProcessor::prepareToPlay (double sampleRate, int blockSize)
{
    detector_.prepare (sampleRate, blockSize);
    comp_.prepare     (sampleRate, blockSize);
    syncParams();
}

void StereoFieldCompressorProcessor::syncParams() noexcept
{
    comp_.setThresholdDb (apvts.getRawParameterValue (IDs::threshold)->load());
    comp_.setRatio       (apvts.getRawParameterValue (IDs::ratio)->load());
    comp_.setAttackMs    (apvts.getRawParameterValue (IDs::attack)->load());
    comp_.setReleaseMs   (apvts.getRawParameterValue (IDs::release)->load());
    comp_.setKneeDb      (apvts.getRawParameterValue (IDs::knee)->load());
    comp_.setMakeupDb    (apvts.getRawParameterValue (IDs::makeup)->load());

    window_.setShape (apvts.getRawParameterValue (IDs::coneCenter)->load(),
                      apvts.getRawParameterValue (IDs::coneWidth)->load());
}

void StereoFieldCompressorProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                                   juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    syncParams();

    const int numSamples = buffer.getNumSamples();
    auto mainIn = getBusBuffer (buffer, true, 0);
    auto mainOut = getBusBuffer (buffer, false, 0);

    // TODO(sidechain): when a sidechain bus is connected, feed the detector
    // from that bus instead of the main input. See getBusBuffer(buffer, true, 1).
    auto scBus = getBusBuffer (buffer, true, 1);
    const bool scActive = scBus.getNumChannels() >= 2;

    auto* l = mainOut.getWritePointer (0);
    auto* r = mainOut.getWritePointer (1);

    const float* scL = scActive ? scBus.getReadPointer (0) : mainIn.getReadPointer (0);
    const float* scR = scActive ? scBus.getReadPointer (1) : mainIn.getReadPointer (1);

    const float mix = apvts.getRawParameterValue (IDs::mix)->load();

    for (int i = 0; i < numSamples; ++i)
    {
        const float angle  = detector_.process (scL[i], scR[i]);
        const float spat   = window_.gainAt (angle);

        // Detector magnitude weighted by cone — outside the cone = 0 → no compression.
        const float scMag  = 0.5f * (std::abs (scL[i]) + std::abs (scR[i]));
        const float gain   = comp_.processSample (scMag * spat);

        const float dryL = l[i];
        const float dryR = r[i];
        const float wetL = dryL * gain;
        const float wetR = dryR * gain;

        l[i] = dryL + mix * (wetL - dryL);
        r[i] = dryR + mix * (wetR - dryR);
    }
}

juce::AudioProcessorEditor* StereoFieldCompressorProcessor::createEditor()
{
    return new StereoFieldCompressorEditor (*this);
}

void StereoFieldCompressorProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto state = apvts.copyState(); state.isValid())
    {
        juce::MemoryOutputStream mos (dest, true);
        state.writeToStream (mos);
    }
}

void StereoFieldCompressorProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto tree = juce::ValueTree::readFromData (data, static_cast<size_t> (sizeInBytes));
    if (tree.isValid())
        apvts.replaceState (tree);
}

// ---- Factory ----
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new StereoFieldCompressorProcessor();
}
