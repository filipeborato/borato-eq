#pragma once

#include <JuceHeader.h>
#include <array>

// ============================================================================
//  ColourStage
//
//  Post-EQ analogue colour: a tube soft-saturation followed by a transformer
//  stage (asymmetric drive + low-frequency weight + gentle high-frequency
//  damping). This is deliberately a light, musical model rather than a
//  physical circuit simulation — enough to add weight and character that
//  tracks the material.
//
//      tube  : engaged by `tube`,  driven by COLOR   (0..1)
//      iron  : engaged by `iron`,  driven by VOICING (0..1)
// ============================================================================

// Symmetric tube-style soft saturation. `drive` in [0,1].
// Transparent at drive == 0 (returns x), increasingly warm above.
float saturateTube(float x, float drive) noexcept;

// Asymmetric transformer-style saturation (adds even harmonics). `amount` in
// [0,1]. Transparent at amount == 0.
float saturateTransformer(float x, float amount) noexcept;

class ColourStage
{
public:
    void prepare(double sampleRate, int numChannels);
    void reset();

    void setParams(bool tubeOn, bool ironOn, float colour01, float voicing01) noexcept;

    void process(juce::AudioBuffer<float>& buffer);

private:
    double sr { 44100.0 };
    int    channels { 2 };

    bool  tube { true };
    bool  iron { false };
    float colour { 0.0f };
    float voicing { 0.0f };

    // One-pole coefficients (set in prepare).
    float hfCoeff { 0.3f }; // high-frequency damping smoother
    float lfCoeff { 0.02f }; // low-frequency weight extractor

    struct ChannelState
    {
        float dampState { 0.0f };   // HF damping one-pole
        float weightState { 0.0f };  // LF weight one-pole
        float dcX1 { 0.0f };        // DC blocker input history
        float dcY1 { 0.0f };        // DC blocker output history
    };

    std::array<ChannelState, 2> state {};
};
