#include "PluginParameters.h"

namespace
{
    juce::String shapeName(BoratoEq::FilterType t)
    {
        switch (t)
        {
            case BoratoEq::FilterType::bell:      return "Bell";
            case BoratoEq::FilterType::lowShelf:  return "Low Shelf";
            case BoratoEq::FilterType::highShelf: return "High Shelf";
            case BoratoEq::FilterType::notch:     return "Notch";
        }
        return "Bell";
    }

    juce::NormalisableRange<float> freqRange(float min, float max, float centre)
    {
        juce::NormalisableRange<float> range { min, max, 1.0f };
        range.setSkewForCentre(centre);
        return range;
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    using namespace BoratoEq;
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    juce::StringArray qChoices;
    for (auto q : qValues)
        qChoices.add(juce::String(q, 1));

    for (const auto& b : bands)
    {
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { b.freqId, 1 }, juce::String(b.label) + " Freq",
            freqRange(b.freqMin, b.freqMax, b.freqDefault), b.freqDefault,
            juce::AudioParameterFloatAttributes().withLabel("Hz")));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { b.gainId, 1 }, juce::String(b.label) + " Gain",
            juce::NormalisableRange<float> { -18.0f, 18.0f, 0.01f }, 0.0f,
            juce::AudioParameterFloatAttributes().withLabel("dB")));

        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID { b.qId, 1 }, juce::String(b.label) + " Q",
            qChoices, defaultQIndex));

        juce::StringArray shapeChoices { shapeName(b.shapeA), shapeName(b.shapeB) };
        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID { b.shapeId, 1 }, juce::String(b.label) + " Shape",
            shapeChoices, b.defaultShape));

        params.push_back(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID { b.onId, 1 }, juce::String(b.label) + " On", true));
    }

    // Global low/high pass. "OFF" is the parameter resting at the extreme that
    // makes the filter transparent (LPF wide open, HPF fully down).
    {
        auto lpfRange = freqRange(1000.0f, 20000.0f, 6000.0f);
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { ParamIDs::lpf, 1 }, "LPF", lpfRange, 20000.0f,
            juce::AudioParameterFloatAttributes().withLabel("Hz")));

        auto hpfRange = freqRange(20.0f, 1000.0f, 120.0f);
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { ParamIDs::hpf, 1 }, "HPF", hpfRange, 20.0f,
            juce::AudioParameterFloatAttributes().withLabel("Hz")));
    }

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamIDs::inputGain, 1 }, "Input",
        juce::NormalisableRange<float> { -24.0f, 24.0f, 0.1f }, 0.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamIDs::outputGain, 1 }, "Output",
        juce::NormalisableRange<float> { -24.0f, 24.0f, 0.1f }, 0.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { ParamIDs::link, 1 }, "Link", false));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { ParamIDs::tube, 1 }, "Tube", true));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { ParamIDs::iron, 1 }, "Iron", false));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamIDs::colour, 1 }, "Color",
        juce::NormalisableRange<float> { 0.0f, 100.0f, 0.1f }, 0.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamIDs::voicing, 1 }, "Voicing",
        juce::NormalisableRange<float> { 0.0f, 100.0f, 0.1f }, 0.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { ParamIDs::power, 1 }, "Power", true));

    return { params.begin(), params.end() };
}
