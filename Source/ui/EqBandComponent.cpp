#include "EqBandComponent.h"

// ============================================================================
//  BandShapeButton
// ============================================================================
void BandShapeButton::paintButton(juce::Graphics& g, bool highlighted, bool down)
{
    auto r = getLocalBounds().toFloat();
    BoratoEqLookAndFeel::drawRectButton(g, r, getToggleState(), down, 3.0f);

    if (highlighted && ! getToggleState())
    {
        g.setColour(juce::Colours::white.withAlpha(0.08f));
        g.drawRoundedRectangle(r.reduced(1.5f), 3.0f, 1.0f);
    }

    const auto colour = getToggleState() ? juce::Colours::white : BoratoColours::textDim();
    const float lw = juce::jmax(1.5f, r.getHeight() * 0.06f);

    // Icon drawn inside a normalised inner area.
    auto a = r.reduced(r.getWidth() * 0.28f, r.getHeight() * 0.30f);
    const float lx = a.getX(), rx = a.getRight();
    const float top = a.getY(), bot = a.getBottom();
    const float midX = a.getCentreX();

    juce::Path p;
    switch (shape)
    {
        case BoratoEq::FilterType::lowShelf:
            p.startNewSubPath(lx, top);
            p.lineTo(midX - a.getWidth() * 0.1f, top);
            p.cubicTo(midX, top, midX, bot, midX + a.getWidth() * 0.1f, bot);
            p.lineTo(rx, bot);
            break;
        case BoratoEq::FilterType::highShelf:
            p.startNewSubPath(lx, bot);
            p.lineTo(midX - a.getWidth() * 0.1f, bot);
            p.cubicTo(midX, bot, midX, top, midX + a.getWidth() * 0.1f, top);
            p.lineTo(rx, top);
            break;
        case BoratoEq::FilterType::notch:
            p.startNewSubPath(lx, top);
            p.cubicTo(midX - a.getWidth() * 0.15f, top, midX - a.getWidth() * 0.1f, bot, midX, bot);
            p.cubicTo(midX + a.getWidth() * 0.1f, bot, midX + a.getWidth() * 0.15f, top, rx, top);
            break;
        case BoratoEq::FilterType::bell:
        default:
            p.startNewSubPath(lx, bot);
            p.cubicTo(midX - a.getWidth() * 0.15f, bot, midX - a.getWidth() * 0.1f, top, midX, top);
            p.cubicTo(midX + a.getWidth() * 0.1f, top, midX + a.getWidth() * 0.15f, bot, rx, bot);
            break;
    }

    g.setColour(colour);
    g.strokePath(p, juce::PathStrokeType(lw, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

// ============================================================================
//  EqBandComponent
// ============================================================================
EqBandComponent::EqBandComponent(BoratoEqLookAndFeel& lnf,
                                 juce::AudioProcessorValueTreeState& state, int bandIndex)
    : lookAndFeelRef(lnf), apvts(state), cfg(BoratoEq::bands[(size_t) bandIndex])
{
    for (auto* k : { &freqKnob, &gainKnob })
    {
        k->setLookAndFeel(&lookAndFeelRef);
        addAndMakeVisible(*k);
    }

    freqAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, cfg.freqId, freqKnob);
    gainAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, cfg.gainId, gainKnob);

    for (size_t i = 0; i < qButtons.size(); ++i)
    {
        auto& b = qButtons[i];
        b.setButtonText(juce::String(BoratoEq::qValues[i], 1));
        b.setLookAndFeel(&lookAndFeelRef);
        b.setClickingTogglesState(false);
        b.onClick = [this, i] { setChoiceParam(cfg.qId, (int) i); };
        addAndMakeVisible(b);
    }

    for (size_t i = 0; i < shapeButtons.size(); ++i)
    {
        auto& b = shapeButtons[i];
        b.setShape(BoratoEq::shapeForIndex(cfg, (int) i));
        b.setLookAndFeel(&lookAndFeelRef);
        b.setClickingTogglesState(false);
        b.onClick = [this, i] { setChoiceParam(cfg.shapeId, (int) i); };
        addAndMakeVisible(b);
    }

    startTimerHz(20);
}

EqBandComponent::~EqBandComponent()
{
    stopTimer();
    freqKnob.setLookAndFeel(nullptr);
    gainKnob.setLookAndFeel(nullptr);
    for (auto& b : qButtons) b.setLookAndFeel(nullptr);
    for (auto& b : shapeButtons) b.setLookAndFeel(nullptr);
}

void EqBandComponent::setChoiceParam(const juce::String& paramId, int index)
{
    if (auto* p = apvts.getParameter(paramId))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost(p->convertTo0to1((float) index));
        p->endChangeGesture();
    }
}

int EqBandComponent::getChoiceParam(const juce::String& paramId) const
{
    if (auto* v = apvts.getRawParameterValue(paramId))
        return (int) v->load();
    return 0;
}

void EqBandComponent::toggleBoolParam(const juce::String& paramId)
{
    if (auto* p = apvts.getParameter(paramId))
    {
        const bool newState = p->getValue() < 0.5f;
        p->beginChangeGesture();
        p->setValueNotifyingHost(newState ? 1.0f : 0.0f);
        p->endChangeGesture();
    }
}

juce::Point<float> EqBandComponent::ledCentre() const
{
    const float s = scale();
    return { 146.5f * s, 38.0f * s };
}

float EqBandComponent::ledRadius() const
{
    return 6.0f * scale();
}

void EqBandComponent::mouseDown(const juce::MouseEvent& e)
{
    // Clicking the status LED toggles the whole band on/off.
    if (e.position.getDistanceFrom(ledCentre()) <= ledRadius() * 2.6f)
        toggleBoolParam(cfg.onId);
}

juce::String EqBandComponent::formatFreq(float hz)
{
    if (hz >= 1000.0f)
    {
        const float k = hz / 1000.0f;
        // Show a decimal only when it carries information (e.g. 1.5kHz).
        const juce::String num = (k == std::floor(k)) ? juce::String((int) k)
                                                       : juce::String(k, 1);
        return num + "kHz";
    }
    return juce::String(juce::roundToInt(hz)) + "Hz";
}

void EqBandComponent::timerCallback()
{
    const int qIndex = getChoiceParam(cfg.qId);
    for (size_t i = 0; i < qButtons.size(); ++i)
        qButtons[i].setToggleState((int) i == qIndex, juce::dontSendNotification);

    const int shapeIndex = getChoiceParam(cfg.shapeId);
    for (size_t i = 0; i < shapeButtons.size(); ++i)
        shapeButtons[i].setToggleState((int) i == shapeIndex, juce::dontSendNotification);

    repaint();
}

void EqBandComponent::resized()
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

    freqKnob.setBounds(box(146.5f, 260.0f, 196.0f));
    gainKnob.setBounds(box(146.5f, 628.0f, 150.0f));

    const float qX = 16.5f;
    for (size_t i = 0; i < qButtons.size(); ++i)
        qButtons[i].setBounds(R(qX + (float) i * 54.0f, 787.0f, 42.0f, 40.0f));

    shapeButtons[0].setBounds(R(26.0f, 863.0f, 112.0f, 42.0f));
    shapeButtons[1].setBounds(R(155.0f, 863.0f, 112.0f, 42.0f));
}

void EqBandComponent::paint(juce::Graphics& g)
{
    const float s = scale();
    auto bounds = getLocalBounds().toFloat();

    BoratoEqLookAndFeel::fillVerticalGradient(g, bounds, BoratoColours::panelTop(), BoratoColours::panelBottom());

    // Right edge divider.
    g.setColour(BoratoColours::divDark());
    g.fillRect(juce::Rectangle<float>(bounds.getRight() - 2 * s, 0, 2 * s, bounds.getHeight()));

    // Status LED (lit when the band is engaged).
    const bool bandOn = apvts.getRawParameterValue(cfg.onId)->load() > 0.5f;
    BoratoEqLookAndFeel::drawLed(g, ledCentre(), ledRadius(), bandOn);

    // Frequency label tracks the FREQ knob; dims when the band is bypassed.
    const float freqHz = apvts.getRawParameterValue(cfg.freqId)->load();
    BoratoEqLookAndFeel::drawTrackedText(g, formatFreq(freqHz),
        juce::Rectangle<float>(0, 86 * s, designW * s, 36 * s),
        bandOn ? BoratoColours::textLabel() : BoratoColours::textDim(),
        21 * s, 2 * s, juce::Justification::centred);

    // FREQ caption.
    BoratoEqLookAndFeel::drawTrackedText(g, "FREQ",
        juce::Rectangle<float>(0, 398 * s, designW * s, 22 * s),
        BoratoColours::textDim(), 12 * s, 4 * s, juce::Justification::centred);

    // Gain readout.
    const float gainDb = apvts.getRawParameterValue(cfg.gainId)->load();
    juce::String gainText = (gainDb >= 0.0f ? "+" : "") + juce::String(gainDb, 2) + " dB";
    BoratoEqLookAndFeel::drawTrackedText(g, gainText,
        juce::Rectangle<float>(0, 488 * s, designW * s, 26 * s),
        BoratoColours::textLabel(), 18 * s, 1 * s, juce::Justification::centred);

    // GAIN caption.
    BoratoEqLookAndFeel::drawTrackedText(g, "GAIN",
        juce::Rectangle<float>(0, 728 * s, designW * s, 22 * s),
        BoratoColours::textDim(), 12 * s, 4 * s, juce::Justification::centred);
}
