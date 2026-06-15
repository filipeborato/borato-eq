#pragma once

#include <JuceHeader.h>
#include <array>
#include "PluginParameters.h"
#include "dsp/ParallelEq.h"
#include "dsp/ColourStage.h"

class BoratoEqAudioProcessor final : public juce::AudioProcessor
{
public:
    BoratoEqAudioProcessor();
    ~BoratoEqAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "BORATO EQ"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return "Default"; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    juce::AudioProcessorValueTreeState apvts;

private:
    void pullParametersIntoDsp();

    std::array<std::atomic<float>*, BoratoEq::numBands> freqParam {};
    std::array<std::atomic<float>*, BoratoEq::numBands> gainParam {};
    std::array<std::atomic<float>*, BoratoEq::numBands> qParam {};
    std::array<std::atomic<float>*, BoratoEq::numBands> shapeParam {};
    std::array<std::atomic<float>*, BoratoEq::numBands> onParam {};

    std::atomic<float>* lpfParam { nullptr };
    std::atomic<float>* hpfParam { nullptr };
    std::atomic<float>* inputParam { nullptr };
    std::atomic<float>* outputParam { nullptr };
    std::atomic<float>* linkParam { nullptr };
    std::atomic<float>* tubeParam { nullptr };
    std::atomic<float>* ironParam { nullptr };
    std::atomic<float>* colourParam { nullptr };
    std::atomic<float>* voicingParam { nullptr };
    std::atomic<float>* powerParam { nullptr };

    ParallelEq eq;
    ColourStage colour;

    juce::AudioBuffer<float> dryBuffer; // kept for the power-bypass crossfade

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> inputSmooth;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> outputSmooth;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> powerSmooth;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BoratoEqAudioProcessor)
};
