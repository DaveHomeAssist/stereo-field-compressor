#pragma once

#include <juce_dsp/juce_dsp.h>

/**
    Feed-forward peak compressor with external spatial gate.

    Unlike a standard compressor, the detector signal is weighted by the cone
    window before envelope following. Signals outside the cone contribute
    zero to the detector, so they are not compressed even if they are loud.
*/
class ConeCompressor
{
public:
    void prepare (double sampleRate, int blockSize);
    void reset();

    /** Sets compressor parameters. Ratio >= 1. Threshold in dBFS. */
    void setThresholdDb (float thresholdDb) noexcept;
    void setRatio       (float ratio)       noexcept;
    void setAttackMs    (float attackMs)    noexcept;
    void setReleaseMs   (float releaseMs)   noexcept;
    void setKneeDb      (float kneeDb)      noexcept;
    void setMakeupDb    (float makeupDb)    noexcept;

    /** Process one stereo sample pair; returns gain in linear units. */
    float processSample (float sidechainWeighted) noexcept;

    /** Last gain reduction, dB (negative = compression). */
    float getLastGainReductionDb() const noexcept { return lastGrDb_; }

private:
    double sampleRate_ = 44100.0;

    float thresholdDb_ = -18.0f;
    float ratio_       = 4.0f;
    float attackMs_    = 10.0f;
    float releaseMs_   = 100.0f;
    float kneeDb_      = 6.0f;
    float makeupDb_    = 0.0f;

    float attackCoeff_  = 0.0f;
    float releaseCoeff_ = 0.0f;

    float envDb_  = -120.0f;
    float lastGrDb_ = 0.0f;

    static float linToDb (float x) noexcept;
    static float dbToLin (float db) noexcept;
    float computeGainReductionDb (float inDb) const noexcept;
    void  updateCoeffs() noexcept;
};
