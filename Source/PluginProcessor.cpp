#include "PluginProcessor.h"

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
    pThreshold_  = apvts.getRawParameterValue (IDs::threshold);
    pRatio_      = apvts.getRawParameterValue (IDs::ratio);
    pAttack_     = apvts.getRawParameterValue (IDs::attack);
    pRelease_    = apvts.getRawParameterValue (IDs::release);
    pKnee_       = apvts.getRawParameterValue (IDs::knee);
    pMakeup_     = apvts.getRawParameterValue (IDs::makeup);
    pConeCenter_ = apvts.getRawParameterValue (IDs::coneCenter);
    pConeWidth_  = apvts.getRawParameterValue (IDs::coneWidth);
    pMix_        = apvts.getRawParameterValue (IDs::mix);
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

    mixSm_.reset (sampleRate, 0.02); // 20 ms dry/wet smoothing
    mixSm_.setCurrentAndTargetValue (pMix_->load());

    syncParams();
}

void StereoFieldCompressorProcessor::reset()
{
    detector_.reset();
    comp_.reset();
    mixSm_.setCurrentAndTargetValue (pMix_->load());
    meterAngleRad_.store (0.0f, std::memory_order_relaxed);
    meterConeWeight_.store (0.0f, std::memory_order_relaxed);
    meterGainReductionDb_.store (0.0f, std::memory_order_relaxed);
}

void StereoFieldCompressorProcessor::syncParams() noexcept
{
    comp_.setThresholdDb (pThreshold_->load());
    comp_.setRatio       (pRatio_->load());
    comp_.setAttackMs    (pAttack_->load());
    comp_.setReleaseMs   (pRelease_->load());
    comp_.setKneeDb      (pKnee_->load());
    comp_.setMakeupDb    (pMakeup_->load());

    window_.setShape (pConeCenter_->load(), pConeWidth_->load());

    mixSm_.setTargetValue (pMix_->load());
}

void StereoFieldCompressorProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                                   juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    syncParams();

    const int numSamples = buffer.getNumSamples();
    auto mainIn  = getBusBuffer (buffer, true, 0);
    auto mainOut = getBusBuffer (buffer, false, 0);

    // Sidechain routing: drive the spatial detector from the external sidechain
    // bus when the host has one enabled; otherwise fall back to internal
    // detection on the main input. isBusesLayoutSupported forces both the main
    // input and any enabled sidechain to stereo, so detectBuf always has two
    // channels here — the detectStereo check is belt-and-suspenders in case that
    // layout constraint is ever relaxed to admit a mono source (then L==R → centre).
    auto* scBus = getBus (true, 1);
    const bool useSidechain = scBus != nullptr && scBus->isEnabled();
    meterExternalSidechain_.store (useSidechain, std::memory_order_relaxed);
    auto detectBuf = useSidechain ? getBusBuffer (buffer, true, 1) : mainIn;

    const bool detectStereo = detectBuf.getNumChannels() >= 2;
    const float* scL = detectBuf.getReadPointer (0);
    const float* scR = detectStereo ? detectBuf.getReadPointer (1) : scL;

    auto* l = mainOut.getWritePointer (0);
    auto* r = mainOut.getWritePointer (1);

    float lastAngle = 0.0f;
    float lastSpat = 0.0f;

    for (int i = 0; i < numSamples; ++i)
    {
        const float angle  = detector_.process (scL[i], scR[i]);
        const float spat   = window_.gainAt (angle);
        lastAngle = angle;
        lastSpat  = spat;

        // Detector magnitude weighted by cone — outside the cone = 0 → no compression.
        const float scMag  = 0.5f * (std::abs (scL[i]) + std::abs (scR[i]));
        const float gain   = comp_.processSample (scMag * spat);

        const float mix  = mixSm_.getNextValue();   // smoothed dry/wet
        const float dryL = l[i];
        const float dryR = r[i];
        const float wetL = dryL * gain;
        const float wetR = dryR * gain;

        l[i] = dryL + mix * (wetL - dryL);
        r[i] = dryR + mix * (wetR - dryR);
    }

    meterAngleRad_.store (lastAngle, std::memory_order_relaxed);
    meterConeWeight_.store (lastSpat, std::memory_order_relaxed);
    meterGainReductionDb_.store (comp_.getLastGainReductionDb(), std::memory_order_relaxed);
}

StereoFieldCompressorProcessor::MeterSnapshot
StereoFieldCompressorProcessor::getMeterSnapshot() const noexcept
{
    return {
        meterAngleRad_.load (std::memory_order_relaxed),
        meterConeWeight_.load (std::memory_order_relaxed),
        meterGainReductionDb_.load (std::memory_order_relaxed),
        meterExternalSidechain_.load (std::memory_order_relaxed)
    };
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
