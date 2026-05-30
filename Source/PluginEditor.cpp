#include "PluginEditor.h"

StereoFieldCompressorEditor::StereoFieldCompressorEditor (StereoFieldCompressorProcessor& p)
    : juce::AudioProcessorEditor (&p), proc_ (p)
{
    setSize (620, 300);

    setup (threshold_,  "threshold",  "Threshold (dB)");
    setup (ratio_,      "ratio",      "Ratio");
    setup (attack_,     "attack",     "Attack (ms)");
    setup (release_,    "release",    "Release (ms)");
    setup (knee_,       "knee",       "Knee (dB)");
    setup (makeup_,     "makeup",     "Makeup (dB)");
    setup (coneCenter_, "coneCenter", "Cone Center");
    setup (coneWidth_,  "coneWidth",  "Cone Width");
    setup (mix_,        "mix",        "Mix");
}

void StereoFieldCompressorEditor::setup (LabeledSlider& s,
                                         const juce::String& paramId,
                                         const juce::String& labelText)
{
    s.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    s.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 18);
    addAndMakeVisible (s.slider);

    s.label.setText (labelText, juce::dontSendNotification);
    s.label.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (s.label);

    s.attach = std::make_unique<SliderAttachment> (proc_.apvts, paramId, s.slider);
}

void StereoFieldCompressorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour::fromRGB (18, 18, 22));
    g.setColour (juce::Colours::white);
    g.setFont (16.0f);
    g.drawText ("Stereo Field Compressor — scaffold build",
                getLocalBounds().removeFromTop (28), juce::Justification::centred);
}

void StereoFieldCompressorEditor::resized()
{
    auto area = getLocalBounds().reduced (10);
    area.removeFromTop (28); // header

    const int rowHeight = 130;
    const int columns   = 5;                     // top row defines the grid; row 2 leaves one empty cell
    const int cellWidth = area.getWidth() / columns;

    auto row1 = area.removeFromTop (rowHeight);
    LabeledSlider* top[] = { &threshold_, &ratio_, &attack_, &release_, &knee_ };
    for (auto* s : top)
    {
        auto cell = row1.removeFromLeft (cellWidth);
        s->label.setBounds  (cell.removeFromTop (20));
        s->slider.setBounds (cell);
    }

    auto row2 = area.removeFromTop (rowHeight);
    LabeledSlider* bot[] = { &makeup_, &coneCenter_, &coneWidth_, &mix_ };
    for (auto* s : bot)
    {
        auto cell = row2.removeFromLeft (cellWidth);
        s->label.setBounds  (cell.removeFromTop (20));
        s->slider.setBounds (cell);
    }
}
