#include "RightPanelComponent.h"
#include "../PluginParameters.h"

RightPanelComponent::RightPanelComponent(BoratoEqLookAndFeel& lnf,
                                         juce::AudioProcessorValueTreeState& state)
    : lookAndFeelRef(lnf), apvts(state)
{
    for (auto* k : { &inputKnob, &outputKnob, &colourKnob, &voicingKnob })
    {
        k->setLookAndFeel(&lookAndFeelRef);
        addAndMakeVisible(*k);
    }

    for (auto* b : { &linkButton, &tubeButton, &ironButton })
    {
        b->setLookAndFeel(&lookAndFeelRef);
        b->setClickingTogglesState(true);
        addAndMakeVisible(*b);
    }

    inputAttach   = std::make_unique<SliderAttach>(apvts, ParamIDs::inputGain, inputKnob);
    outputAttach  = std::make_unique<SliderAttach>(apvts, ParamIDs::outputGain, outputKnob);
    colourAttach  = std::make_unique<SliderAttach>(apvts, ParamIDs::colour, colourKnob);
    voicingAttach = std::make_unique<SliderAttach>(apvts, ParamIDs::voicing, voicingKnob);

    linkAttach = std::make_unique<ButtonAttach>(apvts, ParamIDs::link, linkButton);
    tubeAttach = std::make_unique<ButtonAttach>(apvts, ParamIDs::tube, tubeButton);
    ironAttach = std::make_unique<ButtonAttach>(apvts, ParamIDs::iron, ironButton);

    startTimerHz(20);
}

RightPanelComponent::~RightPanelComponent()
{
    stopTimer();
    for (auto* k : { &inputKnob, &outputKnob, &colourKnob, &voicingKnob })
        k->setLookAndFeel(nullptr);
    for (auto* b : { &linkButton, &tubeButton, &ironButton })
        b->setLookAndFeel(nullptr);
}

void RightPanelComponent::timerCallback()
{
    repaint();
}

juce::Point<float> RightPanelComponent::powerLedCentre() const
{
    const float s = scale();
    return { 334.0f * s, 910.0f * s };
}

float RightPanelComponent::powerLedRadius() const
{
    return 6.0f * scale();
}

void RightPanelComponent::mouseDown(const juce::MouseEvent& e)
{
    // Clicking the POWER LED bypasses (or re-enables) the whole plugin.
    if (e.position.getDistanceFrom(powerLedCentre()) <= powerLedRadius() * 2.6f)
    {
        if (auto* p = apvts.getParameter(ParamIDs::power))
        {
            const bool newState = p->getValue() < 0.5f;
            p->beginChangeGesture();
            p->setValueNotifyingHost(newState ? 1.0f : 0.0f);
            p->endChangeGesture();
        }
    }
}

void RightPanelComponent::resized()
{
    const float s = scale();
    const auto box = [s](float cx, float cy, float size)
    {
        return juce::Rectangle<float>((cx - size * 0.5f) * s, (cy - size * 0.5f) * s, size * s, size * s).toNearestInt();
    };
    const auto R = [s](float x, float y, float w, float h)
    {
        return juce::Rectangle<float>(x * s, y * s, w * s, h * s).toNearestInt();
    };

    inputKnob.setBounds(box(107, 128, 92));
    outputKnob.setBounds(box(107, 321, 92));
    linkButton.setBounds(R(208, 203, 111, 43));

    tubeButton.setBounds(R(56, 532, 105, 39));
    ironButton.setBounds(R(212, 532, 105, 39));

    colourKnob.setBounds(box(107, 648, 92));
    voicingKnob.setBounds(box(265, 648, 92));
}

void RightPanelComponent::paint(juce::Graphics& g)
{
    const float s = scale();
    auto bounds = getLocalBounds().toFloat();

    BoratoEqLookAndFeel::fillVerticalGradient(g, bounds, BoratoColours::sideTop(), BoratoColours::sideBottom());

    const auto section = [&g, s](const juce::String& t, float y)
    {
        BoratoEqLookAndFeel::drawTrackedText(g, t,
            juce::Rectangle<float>(28 * s, y * s, 320 * s, 26 * s),
            BoratoColours::textDim(), 16 * s, 5 * s, juce::Justification::centredLeft);
    };
    const auto caption = [&g, s](const juce::String& value, const juce::String& name, float cx, float cy)
    {
        BoratoEqLookAndFeel::drawTrackedText(g, value,
            juce::Rectangle<float>((cx - 90) * s, (cy + 60) * s, 180 * s, 24 * s),
            BoratoColours::textLabel(), 16 * s, 1 * s, juce::Justification::centred);
        BoratoEqLookAndFeel::drawTrackedText(g, name,
            juce::Rectangle<float>((cx - 90) * s, (cy + 84) * s, 180 * s, 20 * s),
            BoratoColours::textDim(), 11 * s, 4 * s, juce::Justification::centred);
    };
    const auto divider = [&g, s](float y)
    {
        g.setColour(BoratoColours::divSoft());
        g.fillRect(juce::Rectangle<float>(0, y * s, designW * s, 1.5f * s));
    };

    const auto fmtDb = [](float db) { return (db >= 0.0f ? "+" : "") + juce::String(db, 1) + " dB"; };

    section("GAIN STAGING", 24);
    caption(fmtDb(apvts.getRawParameterValue(ParamIDs::inputGain)->load()), "INPUT", 107, 128);
    caption(fmtDb(apvts.getRawParameterValue(ParamIDs::outputGain)->load()), "OUTPUT", 107, 321);

    divider(454);
    section("CHARACTER", 481);

    caption(juce::String(juce::roundToInt(apvts.getRawParameterValue(ParamIDs::colour)->load())), "COLOR", 107, 648);
    caption(juce::String(juce::roundToInt(apvts.getRawParameterValue(ParamIDs::voicing)->load())), "VOICING", 265, 648);

    divider(760);

    // Branding.
    BoratoEqLookAndFeel::drawTrackedText(g, "BORATO",
        juce::Rectangle<float>(0, 788 * s, designW * s, 30 * s),
        BoratoColours::textBright(), 24 * s, 6 * s, juce::Justification::centred);
    BoratoEqLookAndFeel::drawTrackedText(g, "COMPANY",
        juce::Rectangle<float>(0, 824 * s, designW * s, 22 * s),
        BoratoColours::textDim(), 13 * s, 6 * s, juce::Justification::centred);

    BoratoEqLookAndFeel::drawTrackedText(g, "v0.1.0",
        juce::Rectangle<float>(29 * s, 902 * s, 120 * s, 20 * s),
        BoratoColours::textDim(), 13 * s, 2 * s, juce::Justification::centredLeft);
    const bool powered = apvts.getRawParameterValue(ParamIDs::power)->load() > 0.5f;
    BoratoEqLookAndFeel::drawTrackedText(g, "POWER",
        juce::Rectangle<float>(180 * s, 902 * s, 130 * s, 20 * s),
        powered ? BoratoColours::textLabel() : BoratoColours::textDim(),
        14 * s, 4 * s, juce::Justification::centredRight);

    BoratoEqLookAndFeel::drawLed(g, powerLedCentre(), powerLedRadius(), powered);
}
