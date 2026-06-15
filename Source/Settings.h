#pragma once

#include <JuceHeader.h>

class Settings
{
public:
    static Settings& getInstance()
    {
        static Settings instance;
        return instance;
    }

    int getOversamplingMode();
    void setOversamplingMode(int mode);

    float getGuiScale();
    void setGuiScale(float scale);

    bool getShowTooltips();
    void setShowTooltips(bool show);

private:
    Settings();
    ~Settings() = default;

    juce::ApplicationProperties properties;
};
