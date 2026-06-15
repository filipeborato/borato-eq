#pragma once

#include <JuceHeader.h>
#include "BoratoEqLookAndFeel.h"

// Right panel: GAIN STAGING (input/link/output), CHARACTER (tube/iron/color/
// voicing) and the lower branding block.
class RightPanelComponent final : public juce::Component,
                                  private juce::Timer
{
public:
    RightPanelComponent(BoratoEqLookAndFeel& lnf, juce::AudioProcessorValueTreeState& state);
    ~RightPanelComponent() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent&) override;

private:
    void timerCallback() override;
    juce::Point<float> powerLedCentre() const;
    float powerLedRadius() const;

    static constexpr float designW = 368.0f;
    static constexpr float designH = 944.0f;

    float scale() const { return (float) getWidth() / designW; }

    BoratoEqLookAndFeel& lookAndFeelRef;
    juce::AudioProcessorValueTreeState& apvts;

    juce::Slider inputKnob   { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox };
    juce::Slider outputKnob  { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox };
    juce::Slider colourKnob  { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox };
    juce::Slider voicingKnob { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox };

    juce::TextButton linkButton { "LINK" };
    juce::TextButton tubeButton { "TUBE" };
    juce::TextButton ironButton { "IRON" };

    using SliderAttach = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttach = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<SliderAttach> inputAttach, outputAttach, colourAttach, voicingAttach;
    std::unique_ptr<ButtonAttach> linkAttach, tubeAttach, ironAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RightPanelComponent)
};
