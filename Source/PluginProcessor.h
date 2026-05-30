#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#include "SpatialDetector.h"
#include "ConeWindow.h"
#include "ConeCompressor.h"

class StereoFieldCompressorProcessor : public juce::AudioProcessor
{
public:
    StereoFieldCompressorProcessor();
    ~StereoFieldCompressorProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void reset() override;
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Stereo Field Compressor"; }
    bool acceptsMidi()  const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
    ConeCompressor& getCompressor() noexcept { return comp_; } // reserved: GR meter / polar view

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    void syncParams() noexcept;

    SpatialDetector detector_;
    ConeWindow      window_;
    ConeCompressor  comp_;

    // Cached APVTS atomics — resolved once in the constructor so the audio
    // thread does plain atomic loads instead of per-block string-keyed lookups.
    std::atomic<float>* pThreshold_  = nullptr;
    std::atomic<float>* pRatio_      = nullptr;
    std::atomic<float>* pAttack_     = nullptr;
    std::atomic<float>* pRelease_    = nullptr;
    std::atomic<float>* pKnee_       = nullptr;
    std::atomic<float>* pMakeup_     = nullptr;
    std::atomic<float>* pConeCenter_ = nullptr;
    std::atomic<float>* pConeWidth_  = nullptr;
    std::atomic<float>* pMix_        = nullptr;

    juce::SmoothedValue<float> mixSm_;   // smoothed dry/wet — avoids zipper noise on automation

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StereoFieldCompressorProcessor)
};
