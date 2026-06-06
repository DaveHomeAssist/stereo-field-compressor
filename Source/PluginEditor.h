#pragma once

#include <functional>
#include <initializer_list>
#include <memory>

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

class StereoFieldCompressorEditor : public juce::AudioProcessorEditor,
                                    private juce::Timer
{
public:
    explicit StereoFieldCompressorEditor (StereoFieldCompressorProcessor&);
    ~StereoFieldCompressorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;

    class NeonLookAndFeel;
    class StatusChip;
    class StereoFieldView;
    class GainReductionMeter;

    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<SliderAttachment> attach;
    };

    StereoFieldCompressorProcessor& proc_;

    std::unique_ptr<NeonLookAndFeel> lookAndFeel_;
    std::unique_ptr<StatusChip> trackingChip_;
    std::unique_ptr<StatusChip> reductionChip_;
    std::unique_ptr<StatusChip> sidechainChip_;
    std::unique_ptr<StereoFieldView> fieldView_;
    std::unique_ptr<GainReductionMeter> grMeter_;

    Knob threshold_, ratio_, attack_, release_;
    Knob coneCenter_, coneWidth_, knee_, makeup_, mix_;

    juce::Rectangle<int> fieldPanelBounds_, fieldHeaderBounds_;
    juce::Rectangle<int> dynamicsPanelBounds_, dynamicsHeaderBounds_;
    juce::Rectangle<int> outputPanelBounds_, outputHeaderBounds_;

    void timerCallback() override;
    void setupKnob (Knob& knob,
                    const juce::String& paramId,
                    const juce::String& labelText,
                    const juce::String& tone,
                    std::function<juce::String (double)> textFromValue);
    void layoutKnobs (juce::Rectangle<int> area, std::initializer_list<Knob*> knobs);
    void updateMeters();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StereoFieldCompressorEditor)
};
