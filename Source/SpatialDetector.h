#pragma once

#include <juce_dsp/juce_dsp.h>

/**
    Per-sample estimator of stereo field position.

    Returns the angle in radians in [-π/2, +π/2] where
      -π/2 = hard left, 0 = center, +π/2 = hard right.

    Uses instantaneous pan law:  angle = atan2(R - L, R + L).
    Output is smoothed with a one-pole RC lowpass.
*/
class SpatialDetector
{
public:
    void prepare (double sampleRate, int blockSize);
    void reset();

    /** Returns angle in radians for a stereo sample pair (L, R). */
    float process (float left, float right) noexcept;

    /** Smoothing time in milliseconds. Default ~5 ms. */
    void setSmoothingTimeMs (float ms) noexcept;

private:
    double sampleRate_ = 44100.0;
    float  smoothingMs_ = 5.0f;
    float  alpha_ = 0.0f;      // one-pole coefficient
    float  state_ = 0.0f;      // smoothed angle

    void updateAlpha() noexcept;
};
