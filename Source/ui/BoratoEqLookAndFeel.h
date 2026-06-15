#pragma once

#include <JuceHeader.h>

// ============================================================================
//  Shared visual language for BORATO EQ. Colours and helpers are derived from
//  assets/iron_glass_redesign.svg, but everything here is drawn procedurally —
//  no rasterised panel, no SVG background.
// ============================================================================

namespace BoratoColours
{
    inline juce::Colour faceplate()   { return juce::Colour::fromRGB(0x09, 0x09, 0x0a); }
    inline juce::Colour headerBg()    { return juce::Colour::fromRGB(0x08, 0x08, 0x09); }
    inline juce::Colour panelTop()    { return juce::Colour::fromRGB(0x22, 0x22, 0x22); }
    inline juce::Colour panelBottom() { return juce::Colour::fromRGB(0x1c, 0x1c, 0x1c); }
    inline juce::Colour sideTop()     { return juce::Colour::fromRGB(0x16, 0x16, 0x16); }
    inline juce::Colour sideBottom()  { return juce::Colour::fromRGB(0x0f, 0x0f, 0x0f); }
    inline juce::Colour btnTop()      { return juce::Colour::fromRGB(0x2e, 0x2e, 0x2e); }
    inline juce::Colour btnBottom()   { return juce::Colour::fromRGB(0x16, 0x16, 0x16); }
    inline juce::Colour btnStroke()   { return juce::Colour::fromRGB(0x12, 0x12, 0x13); }

    inline juce::Colour wellTop()     { return juce::Colour::fromRGB(0x05, 0x05, 0x06); }
    inline juce::Colour wellBottom()  { return juce::Colour::fromRGB(0x26, 0x28, 0x2a); }
    inline juce::Colour bevelTop()    { return juce::Colour::fromRGB(0x5c, 0x5e, 0x61); }
    inline juce::Colour bevelMid()    { return juce::Colour::fromRGB(0x2c, 0x2e, 0x30); }
    inline juce::Colour bevelBottom() { return juce::Colour::fromRGB(0x0e, 0x0f, 0x10); }
    inline juce::Colour faceTop()     { return juce::Colour::fromRGB(0x12, 0x13, 0x14); }
    inline juce::Colour faceBottom()  { return juce::Colour::fromRGB(0x34, 0x36, 0x38); }
    inline juce::Colour pointer()     { return juce::Colour::fromRGB(0xe2, 0xe4, 0xe6); }

    inline juce::Colour ledCore()     { return juce::Colour::fromRGB(0xff, 0x26, 0x26); }
    inline juce::Colour ledHot()      { return juce::Colour::fromRGB(0xff, 0xa3, 0xa3); }
    inline juce::Colour ledDeep()     { return juce::Colour::fromRGB(0x50, 0x00, 0x00); }

    inline juce::Colour textBright()  { return juce::Colour::fromRGB(0xf5, 0xf5, 0xf5); }
    inline juce::Colour textLabel()   { return juce::Colour::fromRGB(0xe8, 0xe8, 0xe8); }
    inline juce::Colour textDim()     { return juce::Colour::fromRGB(0x7c, 0x7c, 0x7c); }
    inline juce::Colour textTiny()    { return juce::Colour::fromRGB(0xa0, 0xa0, 0xa0); }

    inline juce::Colour divDark()     { return juce::Colour::fromRGB(0x12, 0x12, 0x13); }
    inline juce::Colour divLight()    { return juce::Colour::fromRGB(0x2a, 0x2b, 0x2d); }
    inline juce::Colour divSoft()     { return juce::Colour::fromRGB(0x25, 0x25, 0x28); }
}

class BoratoEqLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    BoratoEqLookAndFeel();

    void drawRotarySlider(juce::Graphics&, int x, int y, int width, int height,
                          float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                          juce::Slider&) override;

    void drawButtonBackground(juce::Graphics&, juce::Button&, const juce::Colour&,
                              bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

    void drawButtonText(juce::Graphics&, juce::TextButton&,
                        bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

    // ---- shared procedural helpers (static so components can reuse them) ----

    // A monospaced, industrial label font.
    static juce::Font monoFont(float height, bool bold = true);

    // Draws spaced uppercase text (mimics SVG letter-spacing) centred in area.
    static void drawTrackedText(juce::Graphics&, const juce::String& text,
                                juce::Rectangle<float> area, juce::Colour colour,
                                float fontHeight, float tracking,
                                juce::Justification just = juce::Justification::centred);

    static void drawLed(juce::Graphics&, juce::Point<float> centre, float radius, bool on);

    static void fillVerticalGradient(juce::Graphics&, juce::Rectangle<float> area,
                                     juce::Colour top, juce::Colour bottom);

    static void drawRectButton(juce::Graphics&, juce::Rectangle<float> r,
                               bool active, bool down, float corner = 3.0f);
};
