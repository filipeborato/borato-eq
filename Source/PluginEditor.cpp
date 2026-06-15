#include "PluginEditor.h"

namespace
{
    // EQ column origins (x) in design space; all share width 293 except the last.
    constexpr std::array<float, BoratoEq::numBands> columnX { 209.0f, 504.0f, 799.0f, 1094.0f, 1389.0f };
    constexpr std::array<float, BoratoEq::numBands> columnW { 293.0f, 293.0f, 293.0f, 293.0f, 291.0f };
}

BoratoEqAudioProcessorEditor::BoratoEqAudioProcessorEditor(BoratoEqAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p),
      filterPanel(lookAndFeel, p.apvts),
      rightPanel(lookAndFeel, p.apvts)
{
    setLookAndFeel(&lookAndFeel);

    addAndMakeVisible(header);
    addAndMakeVisible(filterPanel);
    addAndMakeVisible(rightPanel);

    for (int b = 0; b < BoratoEq::numBands; ++b)
    {
        bands[(size_t) b] = std::make_unique<EqBandComponent>(lookAndFeel, p.apvts, b);
        addAndMakeVisible(*bands[(size_t) b]);
    }

    setResizable(true, true);
    if (auto* boundsConstrainer = getConstrainer())
        boundsConstrainer->setFixedAspectRatio((double) designW / (double) designH);
    setResizeLimits(1024, 520, 4096, 2080);
    setSize(1366, 694);
}

BoratoEqAudioProcessorEditor::~BoratoEqAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

juce::Rectangle<int> BoratoEqAudioProcessorEditor::mapRect(float x, float y, float w, float h) const
{
    const float sx = (float) getWidth() / designW;
    const float sy = (float) getHeight() / designH;
    return juce::Rectangle<float>(x * sx, y * sy, w * sx, h * sy).toNearestInt();
}

void BoratoEqAudioProcessorEditor::resized()
{
    header.setBounds(mapRect(0, 0, designW, 96));
    filterPanel.setBounds(mapRect(0, 96, 207, 944));

    for (int b = 0; b < BoratoEq::numBands; ++b)
        bands[(size_t) b]->setBounds(mapRect(columnX[(size_t) b], 96, columnW[(size_t) b], 944));

    rightPanel.setBounds(mapRect(1680, 96, 368, 944));
}

void BoratoEqAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(BoratoColours::faceplate());

    // Outer plugin border.
    g.setColour(juce::Colour::fromRGB(0x2c, 0x2c, 0x2e));
    g.drawRect(getLocalBounds().toFloat(), 2.0f);
}
