#include "HeaderBarComponent.h"
#include "../PluginProcessor.h"
#include "../Settings.h"

HeaderBarComponent::HeaderBarComponent(BoratoEqAudioProcessor& processor, BoratoEqLookAndFeel& lnf)
    : processorRef(processor), lookAndFeelRef(lnf)
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

    aButton.onClick = [this] { 
        if (processorRef.getABSlot() == true) processorRef.swapAB(); 
    };
    bButton.onClick = [this] { 
        if (processorRef.getABSlot() == false) processorRef.swapAB(); 
    };
    abButton.onClick = [this] {
        processorRef.copyAToB();
    };

    prevButton.onClick = [this] {
        int current = processorRef.getCurrentProgram();
        if (current > 0)
            processorRef.setCurrentProgram(current - 1);
        else
            processorRef.setCurrentProgram(processorRef.getNumPrograms() - 1);
    };

    nextButton.onClick = [this] {
        int current = processorRef.getCurrentProgram();
        if (current < processorRef.getNumPrograms() - 1)
            processorRef.setCurrentProgram(current + 1);
        else
            processorRef.setCurrentProgram(0);
    };

    gearButton.onClick = [this] { showSettingsMenu(); };

    startTimerHz(15);
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
    
    juce::String presetText = processorRef.getProgramName(processorRef.getCurrentProgram());
    if (processorRef.isPresetDirty()) presetText += " *";
    
    BoratoEqLookAndFeel::drawTrackedText(g, presetText, disp, BoratoColours::textLabel(),
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

void HeaderBarComponent::mouseUp(const juce::MouseEvent& e)
{
    if (presetDisplayArea.toFloat().contains(e.position))
        showPresetMenu();
}

void HeaderBarComponent::timerCallback()
{
    aButton.setToggleState(!processorRef.getABSlot(), juce::dontSendNotification);
    bButton.setToggleState(processorRef.getABSlot(), juce::dontSendNotification);
    repaint(presetDisplayArea);
}

void HeaderBarComponent::showPresetMenu()
{
    juce::PopupMenu menu;
    auto& pm = processorRef.getPresetManager();
    int current = processorRef.getCurrentProgram();
    
    juce::PopupMenu factoryMenu;
    int idx = 0;
    for (const auto& p : pm.getFactoryPresets())
    {
        factoryMenu.addItem(1000 + idx, p.name, true, idx == current);
        idx++;
    }
    menu.addSubMenu("Factory Presets", factoryMenu);

    juce::PopupMenu userMenu;
    userMenu.addItem(2000, "Save User Preset...", true, false);
    userMenu.addSeparator();

    for (const auto& p : pm.getUserPresets())
    {
        userMenu.addItem(1000 + idx, p.name, true, idx == current);
        idx++;
    }
    userMenu.addSeparator();
    userMenu.addItem(3000, "Open presets folder...");
    
    menu.addSubMenu("User Presets", userMenu);

    menu.showMenuAsync(juce::PopupMenu::Options().withTargetScreenArea(localAreaToGlobal(presetDisplayArea)),
        [this, &pm](int result)
        {
            if (result >= 1000 && result < 2000)
            {
                processorRef.setCurrentProgram(result - 1000);
            }
            else if (result == 2000)
            {
                saveUserPreset();
            }
            else if (result == 3000)
            {
                pm.openUserPresetsFolder();
            }
        });
}

void HeaderBarComponent::saveUserPreset()
{
    auto* aw = new juce::AlertWindow("Save Preset", "Enter a name:", juce::MessageBoxIconType::QuestionIcon);
    aw->addTextEditor("name", processorRef.getProgramName(processorRef.getCurrentProgram()));
    aw->addButton("Save", 1, juce::KeyPress(juce::KeyPress::returnKey));
    aw->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));

    aw->enterModalState(true, juce::ModalCallbackFunction::create([this, aw](int result)
    {
        if (result == 1)
        {
            auto name = aw->getTextEditorContents("name");
            if (name.isNotEmpty())
            {
                if (auto stateXml = processorRef.apvts.copyState().createXml())
                    processorRef.getPresetManager().saveUserPreset(name, *stateXml);
            }
        }
    }), true);
}

void HeaderBarComponent::showSettingsMenu()
{
    juce::PopupMenu menu;
    auto& settings = Settings::getInstance();
    
    juce::PopupMenu osMenu;
    int osMode = settings.getOversamplingMode();
    osMenu.addItem(10, "Off", true, osMode == 0);
    osMenu.addItem(11, "2x", true, osMode == 1);
    osMenu.addItem(12, "4x", true, osMode == 2);
    menu.addSubMenu("Oversampling", osMenu);
    
    juce::PopupMenu scaleMenu;
    float scale = settings.getGuiScale();
    scaleMenu.addItem(20, "100%", true, scale == 1.0f);
    scaleMenu.addItem(21, "125%", true, scale == 1.25f);
    scaleMenu.addItem(22, "150%", true, scale == 1.5f);
    scaleMenu.addItem(23, "200%", true, scale == 2.0f);
    menu.addSubMenu("GUI Scale", scaleMenu);

    bool tt = settings.getShowTooltips();
    menu.addItem(30, "Show Tooltips", true, tt);
    
    menu.addSeparator();
    menu.addItem(40, "About BORATO EQ...");

    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&gearButton),
        [this, &settings](int result)
        {
            if (result >= 10 && result <= 12)
            {
                settings.setOversamplingMode(result - 10);
            }
            else if (result >= 20 && result <= 23)
            {
                float scales[] = {1.0f, 1.25f, 1.5f, 2.0f};
                settings.setGuiScale(scales[result - 20]);
            }
            else if (result == 30)
            {
                settings.setShowTooltips(!settings.getShowTooltips());
            }
            else if (result == 40)
            {
                juce::AlertWindow::showMessageBoxAsync(
                    juce::MessageBoxIconType::InfoIcon,
                    "About",
                    "BORATO EQ v0.2.0\nBorato Company"
                );
            }
        });
}
