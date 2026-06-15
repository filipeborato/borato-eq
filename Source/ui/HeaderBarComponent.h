#pragma once

#include <JuceHeader.h>
#include "BoratoEqLookAndFeel.h"

// Top header: logo, subtitle, fake preset navigator, A/B controls, settings gear.
// Preset/A-B are visual placeholders for this first version.
class HeaderBarComponent final : public juce::Component
{
public:
    explicit HeaderBarComponent(BoratoEqLookAndFeel& lnf);
    ~HeaderBarComponent() override;

    void paint(juce::Graphics&) override;
    void paintOverChildren(juce::Graphics&) override;
    void resized() override;

private:
    static constexpr float designW = 2048.0f;
    static constexpr float designH = 96.0f;

    float scale() const { return (float) getWidth() / designW; }

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
