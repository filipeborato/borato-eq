#include "FilterPanelComponent.h"
#include "../PluginParameters.h"

FilterPanelComponent::FilterPanelComponent(BoratoEqLookAndFeel& lnf,
                                           juce::AudioProcessorValueTreeState& state)
    : lookAndFeelRef(lnf), apvts(state)
{
    for (auto* k : { &lpfKnob, &hpfKnob })
    {
        k->setLookAndFeel(&lookAndFeelRef);
        addAndMakeVisible(*k);
    }

    lpfAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, ParamIDs::lpf, lpfKnob);
    hpfAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, ParamIDs::hpf, hpfKnob);

    startTimerHz(20);
}

FilterPanelComponent::~FilterPanelComponent()
{
    stopTimer();
    lpfKnob.setLookAndFeel(nullptr);
    hpfKnob.setLookAndFeel(nullptr);
}

void FilterPanelComponent::timerCallback()
{
    repaint();
}

void FilterPanelComponent::resized()
{
    const float s = scale();
    const float box = 96.0f;
    const auto knobBox = [s, box](float cx, float cy)
    {
        return juce::Rectangle<float>((cx - box * 0.5f) * s, (cy - box * 0.5f) * s, box * s, box * s).toNearestInt();
    };

    lpfKnob.setBounds(knobBox(103, 230));
    hpfKnob.setBounds(knobBox(103, 654));
}

juce::String FilterPanelComponent::formatFilter(float hz, bool isLpf)
{
    if (isLpf && hz >= 19900.0f) return "OFF";
    if (! isLpf && hz <= 20.5f)  return "OFF";
    if (hz >= 1000.0f)
        return juce::String(hz / 1000.0f, 1) + "k";
    return juce::String(juce::roundToInt(hz));
}

void FilterPanelComponent::paint(juce::Graphics& g)
{
    const float s = scale();
    auto bounds = getLocalBounds().toFloat();

    BoratoEqLookAndFeel::fillVerticalGradient(g, bounds, BoratoColours::sideTop(), BoratoColours::sideBottom());

    // Right edge dividers.
    g.setColour(BoratoColours::divDark());
    g.fillRect(juce::Rectangle<float>(bounds.getRight() - 2 * s, 0, 2 * s, bounds.getHeight()));

    BoratoEqLookAndFeel::drawTrackedText(g, "FILTERS",
        juce::Rectangle<float>(20 * s, 18 * s, 180 * s, 28 * s),
        BoratoColours::textDim(), 16 * s, 7 * s, juce::Justification::centredLeft);

    const auto lpfHz = apvts.getRawParameterValue(ParamIDs::lpf)->load();
    const auto hpfHz = apvts.getRawParameterValue(ParamIDs::hpf)->load();

    const auto label = [&g, s](const juce::String& value, const juce::String& name, float cy)
    {
        BoratoEqLookAndFeel::drawTrackedText(g, value,
            juce::Rectangle<float>(0, (cy + 70) * s, designW * s, 26 * s),
            BoratoColours::textLabel(), 16 * s, 1 * s, juce::Justification::centred);
        BoratoEqLookAndFeel::drawTrackedText(g, name,
            juce::Rectangle<float>(0, (cy + 96) * s, designW * s, 20 * s),
            BoratoColours::textDim(), 11 * s, 4 * s, juce::Justification::centred);
    };

    label(formatFilter(lpfHz, true), "LPF", 230);

    // Separator line.
    g.setColour(BoratoColours::divSoft());
    g.fillRect(juce::Rectangle<float>(15 * s, 476 * s, 177 * s, 1.5f * s));

    label(formatFilter(hpfHz, false), "HPF", 654);
}
