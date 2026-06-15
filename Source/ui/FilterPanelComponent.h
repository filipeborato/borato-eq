#pragma once

#include <JuceHeader.h>
#include "BoratoEqLookAndFeel.h"

// Left "FILTERS" panel: global LPF and HPF knobs.
class FilterPanelComponent final : public juce::Component,
                                   private juce::Timer
{
public:
    FilterPanelComponent(BoratoEqLookAndFeel& lnf, juce::AudioProcessorValueTreeState& state);
    ~FilterPanelComponent() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    static juce::String formatFilter(float hz, bool isLpf);

    static constexpr float designW = 207.0f;
    static constexpr float designH = 944.0f;

    float scale() const { return (float) getWidth() / designW; }

    BoratoEqLookAndFeel& lookAndFeelRef;
    juce::AudioProcessorValueTreeState& apvts;

    juce::Slider lpfKnob { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox };
    juce::Slider hpfKnob { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox };

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> lpfAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> hpfAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FilterPanelComponent)
};
