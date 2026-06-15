#pragma once

#include <JuceHeader.h>
#include <array>
#include <memory>

// ============================================================================
//  ColourStage
//
//  Post-EQ analogue colour. Controls map onto the hardware's two buttons +
//  two knobs:
//
//      TUBE    (engage)  single-ended-triode-style saturation: an asymmetric
//                        curve that favours the 2nd harmonic (the "warm" tube
//                        character), driven by COLOR.
//      IRON    (engage)  transformer: low-mid weight + asymmetric saturation
//                        + gentle high-frequency rounding, driven by COLOR.
//      COLOR   (0..1)    drive pushed into the tube and iron stages.
//      VOICING (0..1)    global tonal personality of the circuit, ALWAYS on:
//                        0 = dark / vintage, 0.5 = neutral, 1 = open / modern.
//
//  Signal flow (VOICING sits before the saturation so it also shapes how the
//  stages react). The non-linear stages run 2x oversampled to keep the
//  generated harmonics from aliasing back into the audible band:
//
//      in -> VOICING tilt -> [ up 2x -> TUBE -> IRON -> down 2x ] -> out
// ============================================================================

// Asymmetric, single-ended-triode-style soft saturation. `drive` in [0,1];
// transparent at 0. Favours the 2nd harmonic.
float saturateTube(float x, float drive) noexcept;

// Asymmetric transformer-style saturation (even harmonics, DC removed
// analytically). `amount` in [0,1]; transparent at 0.
float saturateTransformer(float x, float amount) noexcept;

class ColourStage
{
public:
    void prepare(double sampleRate, int maxBlockSize, int numChannels);
    void reset();

    void setParams(bool tubeOn, bool ironOn, float colour01, float voicing01) noexcept;

    void process(juce::AudioBuffer<float>& buffer);

    // Latency (in samples, at the host rate) added by the oversampler.
    int getLatencySamples() const;

private:
    using Filter = juce::dsp::IIR::Filter<float>;
    using Coeffs = juce::dsp::IIR::Coefficients<float>;

    void updateVoicing();

    double sr { 44100.0 };
    int    channels { 2 };

    bool  tube { true };
    bool  iron { false };
    float colour { 0.0f };
    float voicing { 0.5f };
    float lastVoicing { -1.0f };

    // VOICING tonal tilt (always applied, at base rate).
    std::array<Filter, 2> voiceLow;   // low shelf  @ ~160 Hz
    std::array<Filter, 2> voiceHigh;  // high shelf @ ~9 kHz

    // IRON low-mid emphasis before the asymmetric saturation (base rate).
    std::array<Filter, 2> ironEmph;   // low shelf  @ ~220 Hz

    // IRON high-frequency rounding (one-pole smoother, runs at the oversampled
    // rate, so its coefficient is computed for 2*sr).
    float hfCoeffOs { 0.3f };
    std::array<float, 2> dampState {};

    // 2x oversampling around the non-linear stages only.
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampler;
};
