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

    struct MeterSnapshot
    {
        float detectedAngleRad = 0.0f;
        float coneWeight       = 0.0f;
        float gainReductionDb  = 0.0f;
        bool  externalSidechain = false;
    };

    MeterSnapshot getMeterSnapshot() const noexcept;

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

    // Audio thread publishes one cheap UI snapshot per block; the editor reads
    // it on its timer. Relaxed ordering is enough because each value is
    // independent and the display tolerates a single-frame mismatch.
    std::atomic<float> meterAngleRad_ { 0.0f };
    std::atomic<float> meterConeWeight_ { 0.0f };
    std::atomic<float> meterGainReductionDb_ { 0.0f };
    std::atomic<bool>  meterExternalSidechain_ { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StereoFieldCompressorProcessor)
};
