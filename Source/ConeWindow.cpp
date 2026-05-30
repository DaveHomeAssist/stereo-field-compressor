#include "ConeWindow.h"
#include <cmath>

void ConeWindow::setShape (float centerRad, float widthRad) noexcept
{
    const float halfPi = juce::MathConstants<float>::halfPi;
    center_    = juce::jlimit (-halfPi, halfPi, centerRad);
    halfWidth_ = juce::jmax (kMinHalfWidth, widthRad * 0.5f);
}

float ConeWindow::gainAt (float angle) const noexcept
{
    const float delta = angle - center_;
    if (std::abs (delta) >= halfWidth_)
        return 0.0f;

    const float x = juce::MathConstants<float>::pi * (delta / halfWidth_);
    return 0.5f * (1.0f + std::cos (x)); // ∈ [0, 1]
}
