#pragma once

#include <juce_core/juce_core.h>

/**
    Raised-cosine cone window over stereo angle space.

    gainAt(theta) = 0.5 * (1 + cos(π * (theta - center) / halfWidth))
                    clamped to [0, 1] with 0 outside the cone.

    The output is the weighting multiplied into the sidechain detector:
    signals inside the cone drive compression; signals outside pass through.
*/
class ConeWindow
{
public:
    /** @param centerRad  angle in [-π/2, +π/2], where 0 = center */
    /** @param widthRad   full cone width in radians (e.g. π/2 gives a ±45° cone) */
    void setShape (float centerRad, float widthRad) noexcept;

    float getCenter()    const noexcept { return center_;    } // reserved: polar view
    float getHalfWidth() const noexcept { return halfWidth_; } // reserved: polar view

    /** Returns gain in [0, 1] at an arbitrary angle. */
    float gainAt (float angleRad) const noexcept;

private:
    // Defensive floor for the half-width (guards gainAt's delta/halfWidth_ divide).
    // Sits below the smallest valid parameter half-width (Cone Width min 0.01 →
    // 0.005), so it only clamps out-of-range/corrupt input, never valid automation.
    static constexpr float kMinHalfWidth = 1.0e-3f;

    float center_    = 0.0f;
    float halfWidth_ = juce::MathConstants<float>::pi * 0.25f; // ±45° cone (full width π/2) default
};
