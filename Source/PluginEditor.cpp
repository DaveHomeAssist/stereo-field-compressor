#include "PluginEditor.h"

#include <cmath>

namespace
{
constexpr float kHalfPi = juce::MathConstants<float>::halfPi;

const auto bg        = juce::Colour::fromRGB (7, 6, 15);
const auto shell     = juce::Colour::fromRGB (18, 14, 36);
const auto shell2    = juce::Colour::fromRGB (10, 8, 24);
const auto text      = juce::Colour::fromRGB (236, 234, 255);
const auto muted     = juce::Colour::fromRGB (154, 150, 196);
const auto quiet     = juce::Colour::fromRGB (111, 107, 150);
const auto cyan      = juce::Colour::fromRGB (0, 224, 255);
const auto mint      = juce::Colour::fromRGB (56, 249, 215);
const auto pink      = juce::Colour::fromRGB (255, 45, 155);
const auto violet    = juce::Colour::fromRGB (177, 77, 255);
const auto green     = juce::Colour::fromRGB (45, 255, 176);
const auto glass     = juce::Colour::fromRGBA (255, 255, 255, 10);
const auto glassLine = juce::Colour::fromRGBA (255, 255, 255, 24);

juce::Font monoFont (float height, int style = juce::Font::plain)
{
    return juce::Font (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), height, style));
}

juce::Font sansFont (float height, int style = juce::Font::plain)
{
    return juce::Font (juce::FontOptions (height, style));
}

juce::Point<float> polarPoint (juce::Point<float> centre, float radius, float angleRad)
{
    return {
        centre.x + radius * std::sin (angleRad),
        centre.y - radius * std::cos (angleRad)
    };
}

juce::Path arcPath (juce::Point<float> centre, float radius, float startRad, float endRad)
{
    juce::Path p;
    const auto start = polarPoint (centre, radius, startRad);
    p.startNewSubPath (start);
    p.addCentredArc (centre.x, centre.y, radius, radius, 0.0f, startRad - kHalfPi, endRad - kHalfPi, true);
    return p;
}

float normaliseAngle (float angleRad)
{
    return juce::jlimit (-1.0f, 1.0f, angleRad / kHalfPi);
}

juce::String angleLabel (float angleRad)
{
    const auto pan = normaliseAngle (angleRad);
    if (std::abs (pan) < 0.05f)
        return "CENTER";

    return juce::String (pan < 0.0f ? "L " : "R ")
        + juce::String (juce::roundToInt (std::abs (pan) * 100.0f))
        + "%";
}

juce::String signedDegrees (float angleRad)
{
    const auto degrees = juce::radiansToDegrees (angleRad);
    return (degrees >= 0.0f ? "+" : "") + juce::String (degrees, 0) + " deg";
}

void drawPanel (juce::Graphics& g, juce::Rectangle<float> r)
{
    g.setColour (glass);
    g.fillRoundedRectangle (r, 16.0f);
    g.setColour (glassLine);
    g.drawRoundedRectangle (r, 16.0f, 1.0f);
}

void drawPanelHeader (juce::Graphics& g,
                      juce::Rectangle<int> area,
                      const juce::String& title,
                      const juce::String& detail)
{
    auto r = area.toFloat();
    g.setColour (text);
    g.setFont (monoFont (12.0f, juce::Font::bold));
    g.drawText (title, area.removeFromLeft (220), juce::Justification::centredLeft);

    g.setColour (quiet);
    g.setFont (monoFont (10.0f));
    g.drawText (detail, area, juce::Justification::centredRight);

    g.setColour (juce::Colours::white.withAlpha (0.07f));
    g.drawLine (r.getX(), r.getBottom(), r.getRight(), r.getBottom(), 1.0f);
}

juce::Colour toneColour (const juce::String& tone)
{
    return tone == "cyan" ? cyan : pink;
}

juce::String plusDb (double value)
{
    return (value > 0.0 ? "+" : "") + juce::String (value, 1) + " dB";
}
}

class StereoFieldCompressorEditor::NeonLookAndFeel : public juce::LookAndFeel_V4
{
public:
    NeonLookAndFeel()
    {
        setColour (juce::Slider::textBoxTextColourId, text);
        setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour (juce::Slider::textBoxHighlightColourId, cyan.withAlpha (0.35f));
        setColour (juce::Label::textColourId, muted);
    }

    void drawRotarySlider (juce::Graphics& g,
                           int x,
                           int y,
                           int width,
                           int height,
                           float sliderPos,
                           float rotaryStartAngle,
                           float rotaryEndAngle,
                           juce::Slider& slider) override
    {
        auto bounds = juce::Rectangle<float> (static_cast<float> (x), static_cast<float> (y),
                                             static_cast<float> (width), static_cast<float> (height))
                          .reduced (5.0f);
        const auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.42f;
        const auto centre = bounds.getCentre();
        const auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
        const auto tone = slider.getProperties()["tone"].toString();
        const auto active = toneColour (tone);
        const auto end = tone == "cyan" ? mint : violet;

        juce::Path track = arcPath (centre, radius, rotaryStartAngle, rotaryEndAngle);
        g.setColour (juce::Colours::white.withAlpha (0.08f));
        g.strokePath (track, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        juce::Path value = arcPath (centre, radius, rotaryStartAngle, angle);
        g.setGradientFill (juce::ColourGradient (active, bounds.getBottomLeft(), end, bounds.getTopRight(), false));
        g.strokePath (value, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        g.setColour (active.withAlpha (0.18f));
        g.fillEllipse (centre.x - radius * 0.8f, centre.y - radius * 0.8f, radius * 1.6f, radius * 1.6f);

        g.setGradientFill (juce::ColourGradient (juce::Colour::fromRGB (34, 26, 61),
                                                 bounds.getX(), bounds.getY(),
                                                 juce::Colour::fromRGB (11, 8, 23),
                                                 bounds.getRight(), bounds.getBottom(),
                                                 false));
        g.fillEllipse (centre.x - radius * 0.72f, centre.y - radius * 0.72f, radius * 1.44f, radius * 1.44f);

        g.setColour (juce::Colours::white.withAlpha (0.10f));
        g.drawEllipse (centre.x - radius * 0.72f, centre.y - radius * 0.72f, radius * 1.44f, radius * 1.44f, 1.0f);

        const auto tip = polarPoint (centre, radius * 0.58f, angle);
        g.setColour (juce::Colours::white.withAlpha (0.95f));
        g.drawLine (juce::Line<float> (centre, tip), 2.4f);
        g.fillEllipse (tip.x - 2.5f, tip.y - 2.5f, 5.0f, 5.0f);
        g.fillEllipse (centre.x - 2.0f, centre.y - 2.0f, 4.0f, 4.0f);
    }

    juce::Font getLabelFont (juce::Label&) override
    {
        return monoFont (10.0f, juce::Font::bold);
    }
};

class StereoFieldCompressorEditor::StatusChip : public juce::Component
{
public:
    explicit StatusChip (juce::String title) : kicker_ (std::move (title)) {}

    void setValue (juce::String value, juce::String tone, bool dim = false)
    {
        value_ = std::move (value);
        tone_ = std::move (tone);
        dim_ = dim;
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced (0.5f);
        drawPanel (g, r);

        const auto active = toneColour (tone_);
        g.setGradientFill (juce::ColourGradient (active, r.getBottomLeft(), tone_ == "cyan" ? mint : violet, r.getTopLeft(), false));
        g.fillRoundedRectangle (r.removeFromLeft (3.0f), 2.0f);

        auto textArea = getLocalBounds().reduced (18, 11);
        g.setColour (quiet);
        g.setFont (monoFont (10.0f));
        g.drawText (kicker_, textArea.removeFromTop (15), juce::Justification::centredLeft);

        g.setColour (dim_ ? muted.withAlpha (0.62f) : text);
        g.setFont (monoFont (18.0f, juce::Font::bold));
        g.drawText (value_, textArea, juce::Justification::centredLeft);
    }

private:
    juce::String kicker_;
    juce::String value_ { "--" };
    juce::String tone_ { "cyan" };
    bool dim_ = false;
};

class StereoFieldCompressorEditor::StereoFieldView : public juce::Component
{
public:
    void setState (float detectedAngleRad, float coneCenterRad, float coneWidthRad, float coneWeight)
    {
        detectedAngleRad_ = detectedAngleRad;
        coneCenterRad_ = coneCenterRad;
        coneWidthRad_ = coneWidthRad;
        coneWeight_ = coneWeight;
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        auto header = getLocalBounds().removeFromTop (24);

        g.setColour (quiet);
        g.setFont (monoFont (10.0f));
        g.drawText ("STEREO IMAGE", header.removeFromLeft (130), juce::Justification::centredLeft);
        g.drawText ("SOURCE " + angleLabel (detectedAngleRad_) + " | CENTER " + signedDegrees (coneCenterRad_),
                    header, juce::Justification::centredRight);

        auto plot = bounds.withTrimmedTop (24.0f).withTrimmedBottom (24.0f).reduced (8.0f, 0.0f);
        const auto radius = juce::jmin (plot.getWidth() * 0.43f, plot.getHeight() * 0.82f);
        const auto centre = juce::Point<float> (plot.getCentreX(), plot.getBottom() - 8.0f);

        for (auto factor : { 1.0f, 0.75f, 0.5f, 0.25f })
        {
            auto arc = arcPath (centre, radius * factor, -kHalfPi, kHalfPi);
            g.setColour ((factor == 1.0f ? violet : violet.withAlpha (0.52f)).withAlpha (factor == 1.0f ? 0.38f : 0.18f));
            g.strokePath (arc, juce::PathStrokeType (factor == 1.0f ? 1.2f : 0.9f));
        }

        for (auto marker : { -1.0f, -0.5f, 0.0f, 0.5f, 1.0f })
        {
            const auto angle = marker * kHalfPi;
            const auto p = polarPoint (centre, radius, angle);
            g.setColour ((marker == 0.0f ? cyan : violet).withAlpha (marker == 0.0f ? 0.38f : 0.16f));
            g.drawLine ({ centre, p }, marker == 0.0f ? 1.1f : 0.8f);
        }

        const auto halfWidth = coneWidthRad_ * 0.5f;
        const auto left = juce::jlimit (-kHalfPi, kHalfPi, coneCenterRad_ - halfWidth);
        const auto right = juce::jlimit (-kHalfPi, kHalfPi, coneCenterRad_ + halfWidth);
        juce::Path cone;
        cone.startNewSubPath (centre);
        constexpr int steps = 40;
        for (int i = 0; i <= steps; ++i)
        {
            const auto a = left + (right - left) * static_cast<float> (i) / static_cast<float> (steps);
            cone.lineTo (polarPoint (centre, radius * 0.96f, a));
        }
        cone.closeSubPath();

        const auto activeCone = coneWeight_ > 0.25f ? pink : cyan;
        g.setGradientFill (juce::ColourGradient (cyan.withAlpha (0.06f), centre,
                                                 activeCone.withAlpha (0.34f),
                                                 polarPoint (centre, radius, coneCenterRad_),
                                                 true));
        g.fillPath (cone);
        g.setColour (activeCone.withAlpha (0.82f));
        g.strokePath (cone, juce::PathStrokeType (1.6f));

        const auto dot = polarPoint (centre, radius, juce::jlimit (-kHalfPi, kHalfPi, detectedAngleRad_));
        g.setColour (juce::Colours::white.withAlpha (0.16f));
        g.fillEllipse (dot.x - 13.0f, dot.y - 13.0f, 26.0f, 26.0f);
        g.setColour (juce::Colours::white);
        g.fillEllipse (dot.x - 4.5f, dot.y - 4.5f, 9.0f, 9.0f);

        g.setFont (monoFont (12.0f, juce::Font::bold));
        g.setColour (muted);
        g.drawText ("L", juce::Rectangle<float> (centre.x - radius - 16.0f, centre.y - 4.0f, 18.0f, 18.0f), juce::Justification::centred);
        g.drawText ("C", juce::Rectangle<float> (centre.x - 9.0f, centre.y - radius - 24.0f, 18.0f, 18.0f), juce::Justification::centred);
        g.drawText ("R", juce::Rectangle<float> (centre.x + radius - 2.0f, centre.y - 4.0f, 18.0f, 18.0f), juce::Justification::centred);

        drawBalanceBars (g, bounds.withTrimmedTop (bounds.getHeight() - 24.0f).reduced (8.0f, 0.0f));
    }

private:
    void drawBalanceBars (juce::Graphics& g, juce::Rectangle<float> area)
    {
        const auto pan = (normaliseAngle (detectedAngleRad_) + 1.0f) * 0.5f;
        const auto left = 1.0f - pan;
        const auto right = pan;

        auto leftArea = area.removeFromLeft (area.getWidth() * 0.5f).reduced (4.0f, 6.0f);
        auto rightArea = area.reduced (4.0f, 6.0f);

        auto drawOne = [&g] (juce::Rectangle<float> r, float value, bool reverse)
        {
            g.setColour (juce::Colours::white.withAlpha (0.06f));
            g.fillRoundedRectangle (r, 3.0f);
            auto fill = r;
            fill.setWidth (r.getWidth() * juce::jlimit (0.0f, 1.0f, value));
            if (reverse)
                fill.setX (r.getRight() - fill.getWidth());
            g.setGradientFill (juce::ColourGradient (violet, fill.getBottomLeft(), cyan, fill.getTopRight(), false));
            g.fillRoundedRectangle (fill, 3.0f);
        };

        drawOne (leftArea, left, true);
        drawOne (rightArea, right, false);
    }

    float detectedAngleRad_ = 0.0f;
    float coneCenterRad_ = 0.0f;
    float coneWidthRad_ = kHalfPi;
    float coneWeight_ = 0.0f;
};

class StereoFieldCompressorEditor::GainReductionMeter : public juce::Component
{
public:
    void setGainReduction (float gainReductionDb)
    {
        reductionDb_ = juce::jlimit (0.0f, 30.0f, std::abs (gainReductionDb));
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        auto area = getLocalBounds();
        g.setColour (quiet);
        g.setFont (monoFont (10.0f, juce::Font::bold));
        g.drawText ("GR", area.removeFromTop (24), juce::Justification::centred);

        auto track = area.reduced (26, 8).toFloat();
        g.setColour (juce::Colours::white.withAlpha (0.05f));
        g.fillRoundedRectangle (track, 8.0f);
        g.setColour (glassLine);
        g.drawRoundedRectangle (track, 8.0f, 1.0f);

        const auto proportion = juce::jlimit (0.0f, 1.0f, reductionDb_ / 24.0f);
        auto fill = track;
        fill.removeFromTop (track.getHeight() * (1.0f - proportion));
        g.setGradientFill (juce::ColourGradient (cyan, fill.getBottomLeft(), pink, fill.getTopRight(), false));
        g.fillRoundedRectangle (fill, 8.0f);

        g.setColour (juce::Colours::white.withAlpha (0.12f));
        for (auto db : { 0.0f, 6.0f, 12.0f, 18.0f, 24.0f })
        {
            const auto y = track.getBottom() - track.getHeight() * (db / 24.0f);
            g.drawLine (track.getX(), y, track.getRight(), y, 1.0f);
        }

        g.setColour (text);
        g.setFont (monoFont (13.0f, juce::Font::bold));
        g.drawText ("-" + juce::String (reductionDb_, 1) + " dB", area.removeFromBottom (24), juce::Justification::centred);
    }

private:
    float reductionDb_ = 0.0f;
};

StereoFieldCompressorEditor::StereoFieldCompressorEditor (StereoFieldCompressorProcessor& p)
    : juce::AudioProcessorEditor (&p), proc_ (p)
{
    lookAndFeel_ = std::make_unique<NeonLookAndFeel>();
    setLookAndFeel (lookAndFeel_.get());

    trackingChip_ = std::make_unique<StatusChip> ("TRACKING");
    reductionChip_ = std::make_unique<StatusChip> ("GAIN REDUCTION");
    sidechainChip_ = std::make_unique<StatusChip> ("SIDECHAIN");
    fieldView_ = std::make_unique<StereoFieldView>();
    grMeter_ = std::make_unique<GainReductionMeter>();

    juce::Component* components[] = { trackingChip_.get(), reductionChip_.get(), sidechainChip_.get(), fieldView_.get(), grMeter_.get() };
    for (auto* c : components)
        addAndMakeVisible (*c);

    setupKnob (threshold_,  "threshold",  "THRESHOLD", "pink", [] (double v) { return juce::String (v, 1) + " dB"; });
    setupKnob (ratio_,      "ratio",      "RATIO",     "pink", [] (double v) { return juce::String (v, 1) + ":1"; });
    setupKnob (attack_,     "attack",     "ATTACK",    "pink", [] (double v) { return juce::String (v, 1) + " ms"; });
    setupKnob (release_,    "release",    "RELEASE",   "pink", [] (double v) { return juce::String (v, 0) + " ms"; });

    setupKnob (coneCenter_, "coneCenter", "CONE CENTER", "cyan", [] (double v) { return signedDegrees (static_cast<float> (v)); });
    setupKnob (coneWidth_,  "coneWidth",  "CONE WIDTH", "cyan", [] (double v) { return juce::String (juce::radiansToDegrees (static_cast<float> (v)), 0) + " deg"; });
    setupKnob (knee_,       "knee",       "KNEE",       "pink", [] (double v) { return juce::String (v, 1) + " dB"; });
    setupKnob (makeup_,     "makeup",     "MAKEUP",     "pink", [] (double v) { return plusDb (v); });
    setupKnob (mix_,        "mix",        "MIX",        "cyan", [] (double v) { return juce::String (juce::roundToInt (v * 100.0)) + "%"; });

    setResizable (true, true);
    setResizeLimits (760, 620, 1120, 900);
    setSize (920, 760);

    updateMeters();
    startTimerHz (30);
}

StereoFieldCompressorEditor::~StereoFieldCompressorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void StereoFieldCompressorEditor::setupKnob (Knob& knob,
                                             const juce::String& paramId,
                                             const juce::String& labelText,
                                             const juce::String& toneName,
                                             std::function<juce::String (double)> textFromValue)
{
    knob.label.setText (labelText, juce::dontSendNotification);
    knob.label.setJustificationType (juce::Justification::centred);
    knob.label.setColour (juce::Label::textColourId, muted);
    addAndMakeVisible (knob.label);

    knob.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    knob.slider.setRotaryParameters (juce::degreesToRadians (-135.0f), juce::degreesToRadians (135.0f), true);
    knob.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 92, 22);
    knob.slider.setNumDecimalPlacesToDisplay (2);
    knob.slider.textFromValueFunction = std::move (textFromValue);
    knob.slider.getProperties().set ("tone", toneName);
    knob.slider.setName (labelText);
    addAndMakeVisible (knob.slider);

    knob.attach = std::make_unique<SliderAttachment> (proc_.apvts, paramId, knob.slider);
}

void StereoFieldCompressorEditor::paint (juce::Graphics& g)
{
    g.fillAll (bg);

    auto bounds = getLocalBounds().toFloat();
    g.setGradientFill (juce::ColourGradient (pink.withAlpha (0.22f), bounds.getTopLeft(),
                                             cyan.withAlpha (0.16f), bounds.getTopRight(), false));
    g.fillEllipse (bounds.getX() - 120.0f, bounds.getY() - 170.0f, 420.0f, 360.0f);
    g.setGradientFill (juce::ColourGradient (violet.withAlpha (0.20f), bounds.getBottomRight(),
                                             cyan.withAlpha (0.09f), bounds.getBottomLeft(), false));
    g.fillEllipse (bounds.getRight() - 360.0f, bounds.getBottom() - 240.0f, 460.0f, 360.0f);

    auto shellBounds = getLocalBounds().reduced (22).toFloat();
    g.setGradientFill (juce::ColourGradient (shell.withAlpha (0.92f), shellBounds.getTopLeft(),
                                             shell2.withAlpha (0.94f), shellBounds.getBottomRight(), false));
    g.fillRoundedRectangle (shellBounds, 22.0f);
    g.setColour (glassLine);
    g.drawRoundedRectangle (shellBounds, 22.0f, 1.0f);

    auto topLine = shellBounds.removeFromTop (2.0f);
    g.setGradientFill (juce::ColourGradient (pink, topLine.getTopLeft(), cyan, topLine.getTopRight(), false));
    g.fillRoundedRectangle (topLine, 1.0f);

    auto area = getLocalBounds().reduced (48, 38);
    auto header = area.removeFromTop (120);
    auto badge = header.removeFromRight (130).removeFromTop (32);

    g.setColour (cyan);
    g.setFont (monoFont (11.0f));
    g.drawText ("DAVETOOLS // SPATIAL DYNAMICS", header.removeFromTop (22), juce::Justification::centredLeft);

    g.setFont (sansFont (34.0f, juce::Font::bold));
    g.setColour (text);
    g.drawText ("STEREO FIELD", header.removeFromTop (38), juce::Justification::centredLeft);
    g.setColour (pink);
    g.drawText ("COMPRESSOR", header.removeFromTop (38), juce::Justification::centredLeft);

    g.setColour (muted);
    g.setFont (sansFont (14.0f));
    g.drawFittedText ("Sidechain-aware compression that ducks only what lives inside the cone - keep the width, control the focus.",
                      header.removeFromTop (24), juce::Justification::centredLeft, 1);

    drawPanel (g, badge.toFloat());
    g.setColour (green);
    g.fillEllipse (badge.getX() + 14.0f, badge.getY() + 12.0f, 7.0f, 7.0f);
    g.setColour (muted);
    g.setFont (monoFont (11.0f, juce::Font::bold));
    g.drawText ("SFC-01", badge.reduced (30, 0), juce::Justification::centredLeft);

    if (! fieldPanelBounds_.isEmpty())
    {
        drawPanel (g, fieldPanelBounds_.toFloat());
        drawPanelHeader (g, fieldHeaderBounds_, "FIELD FOCUS", "POSITION | CONE | REDUCTION");
    }

    if (! dynamicsPanelBounds_.isEmpty())
    {
        drawPanel (g, dynamicsPanelBounds_.toFloat());
        drawPanelHeader (g, dynamicsHeaderBounds_, "DYNAMICS", "THRESHOLD | RATIO | ATTACK | RELEASE");
    }

    if (! outputPanelBounds_.isEmpty())
    {
        drawPanel (g, outputPanelBounds_.toFloat());
        drawPanelHeader (g, outputHeaderBounds_, "SPATIAL / OUTPUT", "CENTER | WIDTH | KNEE | MAKEUP | MIX");
    }

    auto footer = getLocalBounds().reduced (48, 26).removeFromBottom (22);
    g.setColour (quiet);
    g.setFont (monoFont (10.0f));
    g.drawText ("v0.1.0 | DRAG | SHIFT FINE | DBL-CLICK RESET", footer, juce::Justification::centred);
}

void StereoFieldCompressorEditor::resized()
{
    auto area = getLocalBounds().reduced (48, 38);
    area.removeFromTop (126);

    auto chips = area.removeFromTop (72);
    const auto chipGap = 12;
    const auto chipWidth = (chips.getWidth() - chipGap * 2) / 3;
    trackingChip_->setBounds (chips.removeFromLeft (chipWidth));
    chips.removeFromLeft (chipGap);
    reductionChip_->setBounds (chips.removeFromLeft (chipWidth));
    chips.removeFromLeft (chipGap);
    sidechainChip_->setBounds (chips);

    area.removeFromTop (14);

    auto fieldPanel = area.removeFromTop (300);
    fieldPanelBounds_ = fieldPanel;
    auto fieldInner = fieldPanel.reduced (18);
    fieldHeaderBounds_ = fieldInner.removeFromTop (34);
    auto meter = fieldInner.removeFromRight (106);
    fieldView_->setBounds (fieldInner.reduced (0, 4));
    grMeter_->setBounds (meter.reduced (6, 4));

    area.removeFromTop (14);

    auto dynamicsPanel = area.removeFromTop (152);
    dynamicsPanelBounds_ = dynamicsPanel;
    auto dynamicsInner = dynamicsPanel.reduced (18);
    dynamicsHeaderBounds_ = dynamicsInner.removeFromTop (34);
    layoutKnobs (dynamicsInner, { &threshold_, &ratio_, &attack_, &release_ });

    area.removeFromTop (14);

    auto outputPanel = area.removeFromTop (154);
    outputPanelBounds_ = outputPanel;
    auto outputInner = outputPanel.reduced (18);
    outputHeaderBounds_ = outputInner.removeFromTop (34);
    layoutKnobs (outputInner, { &coneCenter_, &coneWidth_, &knee_, &makeup_, &mix_ });
}

void StereoFieldCompressorEditor::layoutKnobs (juce::Rectangle<int> area, std::initializer_list<Knob*> knobs)
{
    const int count = static_cast<int> (knobs.size());
    if (count <= 0)
        return;

    const int gap = 10;
    const int cellWidth = (area.getWidth() - gap * (count - 1)) / count;
    for (auto* knob : knobs)
    {
        auto cell = area.removeFromLeft (cellWidth);
        area.removeFromLeft (gap);
        knob->label.setBounds (cell.removeFromTop (18));
        knob->slider.setBounds (cell.reduced (2, 0));
    }
}

void StereoFieldCompressorEditor::timerCallback()
{
    updateMeters();
}

void StereoFieldCompressorEditor::updateMeters()
{
    const auto snapshot = proc_.getMeterSnapshot();
    const auto reduction = std::abs (snapshot.gainReductionDb);

    trackingChip_->setValue (angleLabel (snapshot.detectedAngleRad), "cyan");
    reductionChip_->setValue ("-" + juce::String (reduction, 1) + " dB", reduction > 0.5f ? "pink" : "cyan", reduction <= 0.5f);
    sidechainChip_->setValue (snapshot.externalSidechain ? "EXTERNAL" : "INTERNAL",
                              snapshot.externalSidechain ? "cyan" : "pink");

    const auto* coneCenter = proc_.apvts.getRawParameterValue ("coneCenter");
    const auto* coneWidth = proc_.apvts.getRawParameterValue ("coneWidth");
    fieldView_->setState (snapshot.detectedAngleRad,
                          coneCenter != nullptr ? coneCenter->load() : 0.0f,
                          coneWidth != nullptr ? coneWidth->load() : kHalfPi,
                          snapshot.coneWeight);
    grMeter_->setGainReduction (snapshot.gainReductionDb);
}

juce::AudioProcessorEditor* StereoFieldCompressorProcessor::createEditor()
{
    return new StereoFieldCompressorEditor (*this);
}
