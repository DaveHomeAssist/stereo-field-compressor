#include "SpatialDetector.h"
#include <cmath>

void SpatialDetector::prepare (double sampleRate, int /*blockSize*/)
{
    sampleRate_ = sampleRate;
    updateAlpha();
    reset();
}

void SpatialDetector::reset()
{
    state_ = 0.0f;
}

void SpatialDetector::setSmoothingTimeMs (float ms) noexcept
{
    smoothingMs_ = juce::jmax (0.1f, ms);
    updateAlpha();
}

void SpatialDetector::updateAlpha() noexcept
{
    // Standard one-pole: alpha = 1 - exp(-1 / (tau * Fs)), tau in seconds.
    const float tau = smoothingMs_ * 0.001f;
    alpha_ = 1.0f - std::exp (-1.0f / (tau * static_cast<float> (sampleRate_)));
}

float SpatialDetector::process (float l, float r) noexcept
{
    // Mono-safe: if sum is ~0, treat as center.
    const float sum  = r + l;
    const float diff = r - l;
    float angle = 0.0f;

    if (std::abs (sum) > 1.0e-7f || std::abs (diff) > 1.0e-7f)
        angle = std::atan2 (diff, std::abs (sum));

    // Clamp to [-π/2, +π/2]
    const float halfPi = juce::MathConstants<float>::halfPi;
    angle = juce::jlimit (-halfPi, halfPi, angle);

    // One-pole smooth
    state_ += alpha_ * (angle - state_);
    return state_;
}
