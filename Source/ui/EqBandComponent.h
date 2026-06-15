#pragma once

#include <JuceHeader.h>
#include <array>
#include "BoratoEqLookAndFeel.h"
#include "../PluginParameters.h"

// A small button that draws a procedural filter-shape icon (shelf / bell /
// notch) instead of text.
class BandShapeButton final : public juce::Button
{
public:
    BandShapeButton() : juce::Button({}) {}
    void setShape(BoratoEq::FilterType t) { shape = t; }

    void paintButton(juce::Graphics&, bool highlighted, bool down) override;

private:
    BoratoEq::FilterType shape { BoratoEq::FilterType::bell };
};

// One EQ band column: status LED, freq label, FREQ knob, gain readout,
// GAIN knob, Q ratio buttons and two shape buttons.
class EqBandComponent final : public juce::Component,
                              private juce::Timer
{
public:
    EqBandComponent(BoratoEqLookAndFeel& lnf, juce::AudioProcessorValueTreeState& state, int bandIndex);
    ~EqBandComponent() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent&) override;

private:
    void timerCallback() override;
    void setChoiceParam(const juce::String& paramId, int index);
    int  getChoiceParam(const juce::String& paramId) const;
    void toggleBoolParam(const juce::String& paramId);
    juce::Point<float> ledCentre() const;
    float ledRadius() const;
    static juce::String formatFreq(float hz);

    static constexpr float designW = 293.0f;
    static constexpr float designH = 944.0f;

    float scale() const { return (float) getWidth() / designW; }

    BoratoEqLookAndFeel& lookAndFeelRef;
    juce::AudioProcessorValueTreeState& apvts;
    const BoratoEq::BandConfig& cfg;

    juce::Slider freqKnob { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox };
    juce::Slider gainKnob { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox };

    std::array<juce::TextButton, 5> qButtons;
    std::array<BandShapeButton, 2>      shapeButtons;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> freqAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gainAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EqBandComponent)
};
