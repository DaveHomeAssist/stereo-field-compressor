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
    magL_ = 0.0f;
    magR_ = 0.0f;
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
    // Smooth channel magnitudes (energy balance) rather than the raw bipolar
    // samples: a steadily-panned source flips sign every half-cycle, so
    // smoothing the signed difference would average the angle to centre.
    magL_ += alpha_ * (std::abs (l) - magL_);
    magR_ += alpha_ * (std::abs (r) - magR_);

    // Silence → centre.
    if (magL_ + magR_ <= kSilenceFloor)
        return 0.0f;

    // Balance angle atan2(|R|,|L|) ∈ [0, π/2]; remap to [-π/2, +π/2], 0 = centre.
    const float quarterPi = juce::MathConstants<float>::pi * 0.25f;
    const float halfPi     = juce::MathConstants<float>::halfPi;
    const float angle = 2.0f * (std::atan2 (magR_, magL_) - quarterPi);
    return juce::jlimit (-halfPi, halfPi, angle);
}
