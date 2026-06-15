#pragma once

#include <JuceHeader.h>
#include <array>
#include "PluginParameters.h"
#include "dsp/ParallelEq.h"
#include "dsp/ColourStage.h"
#include "PresetManager.h"

class BoratoEqAudioProcessor final : public juce::AudioProcessor,
                                     private juce::AudioProcessorValueTreeState::Listener
{
public:
    BoratoEqAudioProcessor();
    ~BoratoEqAudioProcessor() override;

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

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    juce::AudioProcessorValueTreeState apvts;

    PresetManager& getPresetManager() { return presetManager; }

    void applyPreset(int index);
    void storeSnapshot();
    void recallSnapshot();
    void swapAB();
    void copyAToB();
    bool getABSlot() const { return abSlotB; }
    bool isPresetDirty() const { return presetDirty; }

private:
    void parameterChanged(const juce::String& parameterID, float newValue) override;
    void pullParametersIntoDsp();

    // Set from the message thread whenever a parameter changes; the audio
    // thread then recomputes filter coefficients only on the next block,
    // instead of re-deriving (and reallocating) them on every block.
    std::atomic<bool> paramsDirty { true };

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

    using ParameterMap = std::map<juce::String, float>;
    static void setParameterValue(juce::AudioProcessorValueTreeState& state, const juce::String& id, float value);
    static ParameterMap captureEditableParameters(const juce::AudioProcessorValueTreeState& state);
    static void applyParameterMap(juce::AudioProcessorValueTreeState& state, const ParameterMap& map);
    static juce::XmlElement createParameterMapXml(const juce::String& tagName, const ParameterMap& map);
    static ParameterMap parseParameterMapXml(const juce::XmlElement& element);
    static std::unique_ptr<juce::XmlElement> createPersistedStateXml(const juce::AudioProcessorValueTreeState& state,
                                                                    const ParameterMap& storedSnapshot,
                                                                    const ParameterMap& abA,
                                                                    const ParameterMap& abB,
                                                                    bool abSlotB);
    void restorePersistedState(const juce::XmlElement& element);

    PresetManager presetManager;
    ParameterMap storedSnapshot;
    ParameterMap abA;
    ParameterMap abB;
    bool abSlotB { false };
    bool presetDirty { false };
    int currentProgram { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BoratoEqAudioProcessor)
};
