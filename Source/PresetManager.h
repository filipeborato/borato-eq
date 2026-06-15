#pragma once

#include <JuceHeader.h>
#include <map>
#include <vector>

struct PresetData
{
    juce::String name;
    std::map<juce::String, float> values;
};

class PresetManager
{
public:
    PresetManager();

    void scanUserPresets();

    int getNumPrograms() const;
    juce::String getProgramName(int index) const;
    bool getPresetValues(int index, std::map<juce::String, float>& outValues) const;

    int getUserPresetStartIndex() const { return (int) factoryPresets.size(); }
    const std::vector<PresetData>& getFactoryPresets() const { return factoryPresets; }
    const std::vector<PresetData>& getUserPresets() const { return userPresets; }

    bool saveUserPreset(const juce::String& name, const juce::XmlElement& state);
    bool deleteUserPreset(const juce::String& name);
    void openUserPresetsFolder() const;

private:
    juce::File getUserPresetsDirectory() const;

    std::vector<PresetData> factoryPresets;
    std::vector<PresetData> userPresets;
};
