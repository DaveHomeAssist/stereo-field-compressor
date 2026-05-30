#pragma once

#include <juce_dsp/juce_dsp.h>

/**
    Per-sample estimator of stereo field position.

    Returns the angle in radians in [-π/2, +π/2] where
      -π/2 = hard left, 0 = center, +π/2 = hard right.

    Energy-balance estimate: the channel magnitudes |L| and |R| are smoothed
    with a one-pole RC lowpass, then the position is
      angle = 2 * (atan2(|R|, |L|) - π/4).
    Smoothing the magnitudes (not the raw bipolar samples) keeps the estimate
    stable for a steadily-panned source, whose per-sample sign would otherwise
    flip every half-cycle and average to centre.
*/
class SpatialDetector
{
public:
    void prepare (double sampleRate, int blockSize);
    void reset();

    /** Returns angle in radians for a stereo sample pair (L, R). */
    float process (float left, float right) noexcept;

    /** Smoothing time in milliseconds. Default ~5 ms. */
    void setSmoothingTimeMs (float ms) noexcept;  // reserved: not yet driven by a parameter

private:
    static constexpr float kSilenceFloor = 1.0e-7f;  // summed-magnitude floor → treat as centre

    double sampleRate_ = 44100.0;
    float  smoothingMs_ = 5.0f;
    float  alpha_ = 0.0f;      // one-pole coefficient
    float  magL_ = 0.0f;       // smoothed |L|
    float  magR_ = 0.0f;       // smoothed |R|

    void updateAlpha() noexcept;
};
