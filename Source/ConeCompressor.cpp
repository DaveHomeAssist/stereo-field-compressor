#include "ConeCompressor.h"
#include <cmath>

void ConeCompressor::prepare (double sampleRate, int /*blockSize*/)
{
    sampleRate_ = sampleRate;
    updateCoeffs();
    makeupSm_.reset (sampleRate, kMakeupRampSec);
    reset();
}

void ConeCompressor::reset()
{
    envDb_    = kMinEnvDb;
    lastGrDb_ = 0.0f;
    makeupSm_.setCurrentAndTargetValue (makeupDb_);
}

void ConeCompressor::setThresholdDb (float v) noexcept { thresholdDb_ = v; }
void ConeCompressor::setRatio       (float v) noexcept { ratio_ = juce::jmax (1.0f, v); }
void ConeCompressor::setKneeDb      (float v) noexcept { kneeDb_ = juce::jmax (0.0f, v); }
void ConeCompressor::setMakeupDb    (float v) noexcept { makeupDb_ = v; makeupSm_.setTargetValue (v); }

void ConeCompressor::setAttackMs  (float v) noexcept { attackMs_  = juce::jmax (0.1f, v); updateCoeffs(); }
void ConeCompressor::setReleaseMs (float v) noexcept { releaseMs_ = juce::jmax (1.0f, v); updateCoeffs(); }

void ConeCompressor::updateCoeffs() noexcept
{
    const float fs = static_cast<float> (sampleRate_);
    attackCoeff_  = 1.0f - std::exp (-1.0f / (attackMs_  * 0.001f * fs));
    releaseCoeff_ = 1.0f - std::exp (-1.0f / (releaseMs_ * 0.001f * fs));
}

float ConeCompressor::linToDb (float x) noexcept
{
    return 20.0f * std::log10 (juce::jmax (kLogFloor, std::abs (x)));
}

float ConeCompressor::dbToLin (float db) noexcept
{
    return std::pow (10.0f, db * 0.05f);
}

float ConeCompressor::computeGainReductionDb (float inDb) const noexcept
{
    const float halfKnee = kneeDb_ * 0.5f;
    const float overshoot = inDb - thresholdDb_;

    if (overshoot <= -halfKnee)
        return 0.0f;

    if (overshoot >= halfKnee)
        return -(overshoot - overshoot / ratio_);

    // Soft knee (quadratic interpolation). (1/ratio - 1) is already negative for
    // ratio > 1, so this term is a gain *reduction* and is continuous with the
    // hard-knee branch at overshoot == +halfKnee.
    const float x = overshoot + halfKnee;
    const float gr = (1.0f / ratio_ - 1.0f) * (x * x) / (2.0f * kneeDb_);
    return gr;
}

float ConeCompressor::processSample (float sc) noexcept
{
    const float inDb = linToDb (sc);

    // Envelope follower (attack when rising, release when falling)
    const float coeff = (inDb > envDb_) ? attackCoeff_ : releaseCoeff_;
    envDb_ += coeff * (inDb - envDb_);

    const float grDb = computeGainReductionDb (envDb_);
    lastGrDb_ = grDb;

    return dbToLin (grDb + makeupSm_.getNextValue());
}
