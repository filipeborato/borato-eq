#pragma once

#include <JuceHeader.h>
#include <array>
#include "PluginProcessor.h"
#include "ui/BoratoEqLookAndFeel.h"
#include "ui/HeaderBarComponent.h"
#include "ui/FilterPanelComponent.h"
#include "ui/EqBandComponent.h"
#include "ui/RightPanelComponent.h"

class BoratoEqAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit BoratoEqAudioProcessorEditor(BoratoEqAudioProcessor&);
    ~BoratoEqAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    // Design canvas matches assets/iron_glass_redesign.svg.
    static constexpr float designW = 2048.0f;
    static constexpr float designH = 1040.0f;

    juce::Rectangle<int> mapRect(float x, float y, float w, float h) const;

    BoratoEqAudioProcessor& processor;
    BoratoEqLookAndFeel lookAndFeel;

    HeaderBarComponent header { processor, lookAndFeel };
    FilterPanelComponent filterPanel;
    std::array<std::unique_ptr<EqBandComponent>, BoratoEq::numBands> bands;
    RightPanelComponent rightPanel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BoratoEqAudioProcessorEditor)
};
