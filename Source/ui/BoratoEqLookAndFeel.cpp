#include "BoratoEqLookAndFeel.h"

BoratoEqLookAndFeel::BoratoEqLookAndFeel()
{
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour(juce::Label::textColourId, BoratoColours::textLabel());
}

juce::Font BoratoEqLookAndFeel::monoFont(float height, bool bold)
{
    auto opts = juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), height,
                                  bold ? juce::Font::bold : juce::Font::plain);
    return juce::Font(opts);
}

void BoratoEqLookAndFeel::drawTrackedText(juce::Graphics& g, const juce::String& text,
                                          juce::Rectangle<float> area, juce::Colour colour,
                                          float fontHeight, float tracking,
                                          juce::Justification just)
{
    g.setColour(colour);
    g.setFont(monoFont(fontHeight));

    if (tracking <= 0.0f)
    {
        g.drawText(text, area, just, false);
        return;
    }

    // Manual letter spacing to emulate the SVG's wide tracking.
    const auto font = monoFont(fontHeight);
    float total = 0.0f;
    for (int i = 0; i < text.length(); ++i)
        total += juce::GlyphArrangement::getStringWidth(font, text.substring(i, i + 1)) + tracking;
    total -= tracking;

    float x = area.getCentreX() - total * 0.5f;
    if (just.testFlags(juce::Justification::left))
        x = area.getX();
    else if (just.testFlags(juce::Justification::right))
        x = area.getRight() - total;

    const float y = area.getCentreY();
    for (int i = 0; i < text.length(); ++i)
    {
        const auto ch = text.substring(i, i + 1);
        const float w = juce::GlyphArrangement::getStringWidth(font, ch);
        g.drawText(ch, juce::Rectangle<float>(x, y - fontHeight, w + tracking, fontHeight * 2.0f),
                   juce::Justification::centredLeft, false);
        x += w + tracking;
    }
}

void BoratoEqLookAndFeel::fillVerticalGradient(juce::Graphics& g, juce::Rectangle<float> area,
                                               juce::Colour top, juce::Colour bottom)
{
    g.setGradientFill(juce::ColourGradient(top, area.getCentreX(), area.getY(),
                                           bottom, area.getCentreX(), area.getBottom(), false));
    g.fillRect(area);
}

void BoratoEqLookAndFeel::drawLed(juce::Graphics& g, juce::Point<float> c, float radius, bool on)
{
    if (on)
    {
        // Soft glow halo.
        g.setColour(BoratoColours::ledCore().withAlpha(0.25f));
        g.fillEllipse(c.x - radius * 3.0f, c.y - radius * 3.0f, radius * 6.0f, radius * 6.0f);
        g.setColour(BoratoColours::ledCore().withAlpha(0.18f));
        g.fillEllipse(c.x - radius * 2.0f, c.y - radius * 2.0f, radius * 4.0f, radius * 4.0f);

        juce::ColourGradient grad(BoratoColours::ledHot(), c.x - radius * 0.4f, c.y - radius * 0.4f,
                                  BoratoColours::ledDeep(), c.x + radius, c.y + radius, true);
        grad.addColour(0.35, BoratoColours::ledCore());
        g.setGradientFill(grad);
        g.fillEllipse(c.x - radius, c.y - radius, radius * 2.0f, radius * 2.0f);

        g.setColour(juce::Colours::white.withAlpha(0.85f));
        g.fillEllipse(c.x - radius * 0.55f, c.y - radius * 0.55f, radius * 0.45f, radius * 0.45f);
    }
    else
    {
        g.setColour(BoratoColours::ledDeep().darker(0.4f));
        g.fillEllipse(c.x - radius, c.y - radius, radius * 2.0f, radius * 2.0f);
        g.setColour(juce::Colours::black.withAlpha(0.5f));
        g.drawEllipse(c.x - radius, c.y - radius, radius * 2.0f, radius * 2.0f, 1.0f);
    }
}

void BoratoEqLookAndFeel::drawRectButton(juce::Graphics& g, juce::Rectangle<float> r,
                                         bool active, bool down, float corner)
{
    auto rr = r.reduced(1.0f);
    if (down)
        rr.translate(0.0f, 1.0f);

    // Drop shadow.
    g.setColour(juce::Colours::black.withAlpha(0.45f));
    g.fillRoundedRectangle(rr.translated(0.0f, 2.0f), corner);

    fillVerticalGradient(g, rr, down ? BoratoColours::btnBottom() : BoratoColours::btnTop(),
                         BoratoColours::btnBottom().darker(down ? 0.2f : 0.0f));
    // Re-fill with rounded clip.
    {
        juce::Graphics::ScopedSaveState s(g);
        juce::Path p;
        p.addRoundedRectangle(rr, corner);
        g.reduceClipRegion(p);
        fillVerticalGradient(g, rr, BoratoColours::btnTop(), BoratoColours::btnBottom());

        // Subtle top sheen.
        g.setColour(juce::Colours::white.withAlpha(down ? 0.04f : 0.10f));
        g.fillRoundedRectangle(rr.reduced(3.0f, 2.0f).removeFromTop(rr.getHeight() * 0.32f), corner * 0.6f);
    }

    if (active)
    {
        // White glow outline (whiteGlow filter in the SVG).
        g.setColour(juce::Colours::white.withAlpha(0.22f));
        g.drawRoundedRectangle(rr.expanded(1.5f), corner + 1.0f, 2.5f);
        g.setColour(juce::Colours::white);
        g.drawRoundedRectangle(rr, corner, 1.6f);
    }
    else
    {
        g.setColour(BoratoColours::btnStroke());
        g.drawRoundedRectangle(rr, corner, 1.5f);
    }
}

void BoratoEqLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                               const juce::Colour&, bool highlighted, bool down)
{
    auto r = button.getLocalBounds().toFloat();
    drawRectButton(g, r, button.getToggleState(), down, 3.0f);

    if (highlighted && ! down && ! button.getToggleState())
    {
        g.setColour(juce::Colours::white.withAlpha(0.08f));
        g.drawRoundedRectangle(r.reduced(1.5f), 3.0f, 1.0f);
    }
}

void BoratoEqLookAndFeel::drawButtonText(juce::Graphics& g, juce::TextButton& button,
                                         bool, bool down)
{
    auto r = button.getLocalBounds().toFloat();
    if (down)
        r.translate(0.0f, 1.0f);

    const bool on = button.getToggleState();
    const auto colour = on ? BoratoColours::textBright() : BoratoColours::textDim();
    const float h = juce::jlimit(11.0f, 18.0f, r.getHeight() * 0.42f);
    drawTrackedText(g, button.getButtonText(), r, colour, h, h * 0.18f, juce::Justification::centred);
}

void BoratoEqLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                           float sliderPos, float rotaryStartAngle,
                                           float rotaryEndAngle, juce::Slider&)
{
    auto bounds = juce::Rectangle<float>((float) x, (float) y, (float) width, (float) height);
    const float radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f - 4.0f;
    const auto centre = bounds.getCentre();
    const float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    const auto circle = [&g](juce::Point<float> c, float rad)
    {
        g.fillEllipse(c.x - rad, c.y - rad, rad * 2.0f, rad * 2.0f);
    };

    // --- Recessed well ---
    g.setGradientFill(juce::ColourGradient(BoratoColours::wellTop(), centre.x - radius, centre.y - radius,
                                           BoratoColours::wellBottom(), centre.x + radius, centre.y + radius, false));
    circle(centre, radius);

    // --- Drop shadow behind the knob body ---
    g.setColour(juce::Colours::black.withAlpha(0.55f));
    circle({ centre.x + radius * 0.04f, centre.y + radius * 0.07f }, radius * 0.86f);

    // --- Chamfer / bevel body ---
    const float bodyR = radius * 0.84f;
    {
        juce::ColourGradient grad(BoratoColours::bevelTop(), centre.x - bodyR, centre.y - bodyR,
                                  BoratoColours::bevelBottom(), centre.x + bodyR, centre.y + bodyR, false);
        grad.addColour(0.45, BoratoColours::bevelMid());
        g.setGradientFill(grad);
        circle(centre, bodyR);
    }
    g.setColour(juce::Colours::black.withAlpha(0.6f));
    g.drawEllipse(centre.x - bodyR, centre.y - bodyR, bodyR * 2.0f, bodyR * 2.0f, 1.0f);

    // --- Recessed inner face ---
    const float faceR = radius * 0.66f;
    {
        juce::ColourGradient grad(BoratoColours::faceTop(), centre.x - faceR, centre.y - faceR,
                                  BoratoColours::faceBottom(), centre.x + faceR, centre.y + faceR, false);
        g.setGradientFill(grad);
        circle(centre, faceR);
    }

    // --- Pointer (white matte strip, pointing up at 12:00 then rotated) ---
    const float pw = juce::jmax(3.0f, radius * 0.10f);
    const float pTop = radius * 0.72f;
    const float pBottom = radius * 0.30f;
    juce::Path pointer;
    pointer.addRoundedRectangle(-pw * 0.5f, -pTop, pw, pTop - pBottom, pw * 0.4f);
    pointer.applyTransform(juce::AffineTransform::rotation(angle).translated(centre.x, centre.y));

    g.setColour(juce::Colours::black.withAlpha(0.5f));
    g.fillPath(pointer, juce::AffineTransform::translation(1.0f, 1.5f));
    g.setColour(BoratoColours::pointer());
    g.fillPath(pointer);
}
