#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

class StereoFieldCompressorEditor : public juce::AudioProcessorEditor
{
public:
    explicit StereoFieldCompressorEditor (StereoFieldCompressorProcessor&);
    ~StereoFieldCompressorEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;

    StereoFieldCompressorProcessor& proc_;

    struct LabeledSlider
    {
        juce::Slider slider;
        juce::Label  label;
        std::unique_ptr<SliderAttachment> attach;
    };

    LabeledSlider threshold_, ratio_, attack_, release_, knee_, makeup_, coneCenter_, coneWidth_, mix_;

    void setup (LabeledSlider& s, const juce::String& paramId, const juce::String& labelText);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StereoFieldCompressorEditor)
};
