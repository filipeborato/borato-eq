#pragma once

#include <JuceHeader.h>
#include <array>

// ============================================================================
//  BORATO EQ — parameter model
//
//  Five-band PARALLEL tube EQ. Every band reads the same input, contributes a
//  delta, and the deltas are summed back together before a tube/transformer
//  colour stage and input/output gain staging.
//
//  The BandConfig table below is the single source of truth shared by the DSP
//  (ParallelEq) and the UI (EqBandComponent), so adding/retuning a band only
//  needs editing here.
// ============================================================================

namespace BoratoEq
{
    static constexpr int numBands = 5;

    // Filter shapes a band can take. The two shape buttons on each band select
    // between that band's shapeA / shapeB entries.
    enum class FilterType
    {
        bell,
        lowShelf,
        highShelf,
        notch
    };

    // The five discrete Q values offered by the band's ratio-row buttons.
    static constexpr std::array<float, 5> qValues { 0.4f, 0.7f, 1.0f, 1.5f, 2.5f };
    static constexpr int defaultQIndex = 2; // 1.0

    struct BandConfig
    {
        const char* freqId;
        const char* gainId;
        const char* qId;
        const char* shapeId;
        const char* onId; // band engage (the column's status LED)

        const char* label; // e.g. "100Hz"

        float freqMin;
        float freqMax;
        float freqDefault;

        // The two selectable shapes (left button = A, right button = B).
        FilterType shapeA;
        FilterType shapeB;
        int        defaultShape; // 0 -> A, 1 -> B
    };

    inline constexpr std::array<BandConfig, numBands> bands { {
        { "band1Freq", "band1Gain", "band1Q", "band1Shape", "band1On", "100Hz",
          20.0f,   500.0f,   100.0f,   FilterType::lowShelf,  FilterType::bell,      0 },
        { "band2Freq", "band2Gain", "band2Q", "band2Shape", "band2On", "300Hz",
          80.0f,   1200.0f,  300.0f,   FilterType::lowShelf,  FilterType::bell,      1 },
        { "band3Freq", "band3Gain", "band3Q", "band3Shape", "band3On", "1kHz",
          200.0f,  5000.0f,  1000.0f,  FilterType::bell,      FilterType::notch,     0 },
        { "band4Freq", "band4Gain", "band4Q", "band4Shape", "band4On", "4kHz",
          1000.0f, 12000.0f, 4000.0f,  FilterType::bell,      FilterType::highShelf, 0 },
        { "band5Freq", "band5Gain", "band5Q", "band5Shape", "band5On", "10kHz",
          2000.0f, 20000.0f, 10000.0f, FilterType::bell,      FilterType::highShelf, 1 },
    } };

    inline FilterType shapeForIndex(const BandConfig& cfg, int index)
    {
        return index == 0 ? cfg.shapeA : cfg.shapeB;
    }
}

namespace ParamIDs
{
    // Global filters (left panel)
    static constexpr auto lpf = "lpf";
    static constexpr auto hpf = "hpf";

    // Gain staging (right panel)
    static constexpr auto inputGain  = "inputGain";
    static constexpr auto outputGain = "outputGain";
    static constexpr auto link       = "link";

    // Character / colour (right panel)
    static constexpr auto tube    = "tube";    // engage tube stage
    static constexpr auto iron    = "iron";    // engage transformer stage
    static constexpr auto colour  = "colour";  // 0..100 tube drive amount
    static constexpr auto voicing = "voicing"; // 0..100 transformer character

    // Master power (POWER LED): false = full plugin bypass.
    static constexpr auto power = "power";
}

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
