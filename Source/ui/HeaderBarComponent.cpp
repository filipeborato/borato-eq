#include "HeaderBarComponent.h"

HeaderBarComponent::HeaderBarComponent(BoratoEqLookAndFeel& lnf)
    : lookAndFeelRef(lnf)
{
    for (auto* b : { &prevButton, &nextButton, &aButton, &bButton, &abButton, &gearButton })
    {
        b->setLookAndFeel(&lookAndFeelRef);
        addAndMakeVisible(*b);
    }

    aButton.setClickingTogglesState(true);
    bButton.setClickingTogglesState(true);
    aButton.setToggleState(true, juce::dontSendNotification);
    aButton.setRadioGroupId(100);
    bButton.setRadioGroupId(100);

    gearButton.setButtonText({});
}

HeaderBarComponent::~HeaderBarComponent()
{
    for (auto* b : { &prevButton, &nextButton, &aButton, &bButton, &abButton, &gearButton })
        b->setLookAndFeel(nullptr);
}

void HeaderBarComponent::resized()
{
    const float s = scale();
    const auto R = [s](float x, float y, float w, float h)
    {
        return juce::Rectangle<float>(x * s, y * s, w * s, h * s).toNearestInt();
    };

    prevButton.setBounds(R(712, 25, 49, 46));
    presetDisplayArea = R(771, 25, 742, 46);
    nextButton.setBounds(R(1523, 25, 49, 46));

    aButton.setBounds(R(1679, 25, 49, 46));
    bButton.setBounds(R(1728, 25, 49, 46));
    abButton.setBounds(R(1794, 25, 80, 46));

    gearArea = R(1978, 25, 45, 46);
    gearButton.setBounds(gearArea);
}

void HeaderBarComponent::paint(juce::Graphics& g)
{
    const float s = scale();
    auto bounds = getLocalBounds().toFloat();

    g.setColour(BoratoColours::headerBg());
    g.fillRect(bounds);

    // Logo + subtitle.
    BoratoEqLookAndFeel::drawTrackedText(g, "BORATO EQ",
        juce::Rectangle<float>(30 * s, 0, 320 * s, designH * s),
        BoratoColours::textBright(), 28 * s, 12 * s, juce::Justification::centredLeft);

    BoratoEqLookAndFeel::drawTrackedText(g, "PARALLEL TUBE EQ",
        juce::Rectangle<float>(360 * s, 0, 340 * s, designH * s),
        BoratoColours::textDim(), 14 * s, 5 * s, juce::Justification::centredLeft);

    // Preset display field.
    auto disp = presetDisplayArea.toFloat();
    g.setColour(BoratoColours::divDark());
    g.fillRoundedRectangle(disp, 3 * s);
    g.setColour(BoratoColours::divLight().withAlpha(0.6f));
    g.drawRoundedRectangle(disp, 3 * s, 1.5f);
    BoratoEqLookAndFeel::drawTrackedText(g, "Default", disp, BoratoColours::textLabel(),
                                         16 * s, 3 * s, juce::Justification::centred);

    // Bottom divider.
    g.setColour(juce::Colour::fromRGB(0x1c, 0x1c, 0x1e));
    g.fillRect(juce::Rectangle<float>(0, bounds.getBottom() - 1.5f * s, bounds.getWidth(), 1.5f * s));
}

void HeaderBarComponent::paintOverChildren(juce::Graphics& g)
{
    // Procedural gear glyph drawn on top of the (empty) gear button.
    const auto c = gearArea.toFloat().getCentre();
    const float r = gearArea.getHeight() * 0.22f;

    g.setColour(BoratoColours::textDim());
    g.drawEllipse(c.x - r, c.y - r, r * 2.0f, r * 2.0f, r * 0.45f);
    for (int i = 0; i < 8; ++i)
    {
        const float a = juce::MathConstants<float>::twoPi * (float) i / 8.0f;
        const auto p1 = c.translated(std::cos(a) * r * 1.15f, std::sin(a) * r * 1.15f);
        const auto p2 = c.translated(std::cos(a) * r * 1.7f, std::sin(a) * r * 1.7f);
        g.drawLine({ p1, p2 }, r * 0.45f);
    }
    g.setColour(BoratoColours::headerBg());
    g.fillEllipse(c.x - r * 0.45f, c.y - r * 0.45f, r * 0.9f, r * 0.9f);
}
