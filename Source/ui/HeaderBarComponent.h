#pragma once

#include <JuceHeader.h>
#include "BoratoEqLookAndFeel.h"

// Top header: logo, subtitle, fake preset navigator, A/B controls, settings gear.
// Preset/A-B are visual placeholders for this first version.
class BoratoEqAudioProcessor;

class HeaderBarComponent final : public juce::Component,
                                 private juce::Timer
{
public:
    HeaderBarComponent(BoratoEqAudioProcessor& processor, BoratoEqLookAndFeel& lnf);
    ~HeaderBarComponent() override;

    void paint(juce::Graphics&) override;
    void paintOverChildren(juce::Graphics&) override;
    void resized() override;
    void mouseUp(const juce::MouseEvent& e) override;

    void timerCallback() override;
    
    void showPresetMenu();
    void showSettingsMenu();
    void saveUserPreset();

    static constexpr float designW = 2048.0f;
    static constexpr float designH = 96.0f;

    float scale() const { return (float) getWidth() / designW; }

    BoratoEqAudioProcessor& processorRef;
    BoratoEqLookAndFeel& lookAndFeelRef;

    juce::TextButton prevButton { "<" };
    juce::TextButton nextButton { ">" };
    juce::TextButton aButton { "A" };
    juce::TextButton bButton { "B" };
    juce::TextButton abButton { "A > B" };
    juce::TextButton gearButton;

    juce::Rectangle<int> presetDisplayArea;
    juce::Rectangle<int> gearArea;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HeaderBarComponent)
};
