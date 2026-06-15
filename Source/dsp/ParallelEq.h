#pragma once

#include <JuceHeader.h>
#include <array>
#include "../PluginParameters.h"

// ============================================================================
//  ParallelEq
//
//  The heart of the "parallel" concept. Instead of running the five bands in
//  series (output of one feeds the next), every band filters the SAME dry
//  input and we sum each band's *delta* (filtered - dry) back onto the dry
//  signal:
//
//      out = dry + Σ (bandFilter_i(dry) - dry)
//
//  At unity gain a band contributes nothing, so the resting state is fully
//  transparent. When bands boost/cut they all act on the original spectrum at
//  once and interact differently from a serial chain — the passive-hardware
//  flavour the plugin is going for.
//
//  Global HPF/LPF are applied to the input before the parallel split.
// ============================================================================

class ParallelEq
{
public:
    void prepare(double sampleRate, int maxBlockSize, int numChannels);
    void reset();

    struct BandSettings
    {
        float freq;
        float gainDb;
        float q;
        BoratoEq::FilterType type;
        bool  on { true }; // band engaged (status LED)
    };

    // Called once per block from the audio thread (no allocation).
    void setBand(int bandIndex, const BandSettings& s);
    void setHpf(float freqHz);   // <= 20 Hz -> bypassed
    void setLpf(float freqHz);   // >= 20 kHz -> bypassed

    void process(juce::AudioBuffer<float>& buffer);

private:
    using Filter = juce::dsp::IIR::Filter<float>;
    using Coeffs = juce::dsp::IIR::Coefficients<float>;

    void updateBandCoeffs(int bandIndex);

    double sr { 44100.0 };
    int    channels { 2 };

    // [band][channel]
    std::array<std::array<Filter, 2>, BoratoEq::numBands> bandFilters;
    std::array<std::array<Filter, 2>, 2>                  passFilters; // [0]=HPF [1]=LPF

    std::array<BandSettings, BoratoEq::numBands> bandSettings {};
    std::array<bool, BoratoEq::numBands>         bandActive {}; // gain != 0

    float hpfFreq { 20.0f };
    float lpfFreq { 20000.0f };
    bool  hpfActive { false };
    bool  lpfActive { false };

    juce::AudioBuffer<float> scratch; // per-band filtered copy
};
