#include "PresetManager.h"
#include "PluginParameters.h"

PresetManager::PresetManager()
{
    // Initialize Factory Presets
    PresetData defaultPreset;
    defaultPreset.name = "Default";
    // We can leave values empty, or define them explicitly.
    // If empty, the caller might need to know how to "reset to default".
    // Actually, let's explicitly define a few to override anything.
    factoryPresets.push_back(defaultPreset);

    PresetData vocalAir;
    vocalAir.name = "Vocal Air";
    vocalAir.values["band4Gain"] = 2.0f;
    vocalAir.values["band5Gain"] = 4.0f;
    vocalAir.values["tube"] = 1.0f;
    vocalAir.values["colour"] = 30.0f;
    factoryPresets.push_back(vocalAir);

    PresetData busGlue;
    busGlue.name = "Bus Glue";
    busGlue.values["band1Gain"] = 1.0f;
    busGlue.values["band3Gain"] = -1.0f;
    busGlue.values["tube"] = 1.0f;
    busGlue.values["iron"] = 1.0f;
    busGlue.values["colour"] = 40.0f;
    busGlue.values["voicing"] = 40.0f;
    factoryPresets.push_back(busGlue);

    PresetData lowEndWeight;
    lowEndWeight.name = "Low-End Weight";
    lowEndWeight.values["band1Gain"] = 4.0f;
    lowEndWeight.values["band2Gain"] = 2.0f;
    lowEndWeight.values["iron"] = 1.0f;
    lowEndWeight.values["voicing"] = 10.0f;
    factoryPresets.push_back(lowEndWeight);

    PresetData vintageDark;
    vintageDark.name = "Vintage Dark";
    vintageDark.values["band5Gain"] = -3.0f;
    vintageDark.values["lpf"] = 12000.0f;
    vintageDark.values["tube"] = 1.0f;
    vintageDark.values["iron"] = 1.0f;
    vintageDark.values["voicing"] = 0.0f;
    factoryPresets.push_back(vintageDark);

    PresetData modernOpen;
    modernOpen.name = "Modern Open";
    modernOpen.values["band4Gain"] = 2.0f;
    modernOpen.values["band5Gain"] = 3.0f;
    modernOpen.values["voicing"] = 100.0f;
    modernOpen.values["tube"] = 1.0f;
    factoryPresets.push_back(modernOpen);

    PresetData masterPolish;
    masterPolish.name = "Master Polish";
    masterPolish.values["band1Gain"] = 1.0f;
    masterPolish.values["band5Gain"] = 1.5f;
    masterPolish.values["tube"] = 1.0f;
    masterPolish.values["iron"] = 1.0f;
    masterPolish.values["colour"] = 20.0f;
    factoryPresets.push_back(masterPolish);

    scanUserPresets();
}

juce::File PresetManager::getUserPresetsDirectory() const
{
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
           .getChildFile("Borato Company")
           .getChildFile("BORATO EQ")
           .getChildFile("Presets");
}

void PresetManager::scanUserPresets()
{
    userPresets.clear();
    auto dir = getUserPresetsDirectory();
    if (dir.exists() && dir.isDirectory())
    {
        juce::Array<juce::File> files = dir.findChildFiles(juce::File::findFiles, false, "*.boratoeq");
        for (const auto& f : files)
        {
            auto xml = juce::XmlDocument::parse(f);
            if (xml != nullptr)
            {
                PresetData pd;
                pd.name = xml->getStringAttribute("name", f.getFileNameWithoutExtension());
                // We store the values we can parse? 
                // Or maybe the user preset should just be an XML file that gets loaded fully by ValueTree?
                // The plan says: "- Um preset = nome + snapshot de todos os parâmetros editáveis da APVTS."
                // Wait, if it's an XML file of apvts, we can extract the values from the PARAM tags.
                // apvts.copyState().createXml() produces a standard JUCE ValueTree XML.
                
                auto vtXml = xml->getChildByName("PARAMETERS");
                if (vtXml == nullptr)
                    vtXml = xml.get(); // fallback if root is the value tree

                for (auto* child : vtXml->getChildIterator())
                {
                    if (child->hasTagName("PARAM"))
                    {
                        auto id = child->getStringAttribute("id");
                        auto val = child->getDoubleAttribute("value", 0.0);
                        pd.values[id] = (float) val;
                    }
                }
                userPresets.push_back(pd);
            }
        }
    }
}

int PresetManager::getNumPrograms() const
{
    return (int) (factoryPresets.size() + userPresets.size());
}

juce::String PresetManager::getProgramName(int index) const
{
    if (index >= 0 && index < (int) factoryPresets.size())
        return factoryPresets[(size_t) index].name;
    
    int userIndex = index - (int) factoryPresets.size();
    if (userIndex >= 0 && userIndex < (int) userPresets.size())
        return userPresets[(size_t) userIndex].name;

    return "Unknown";
}

bool PresetManager::getPresetValues(int index, std::map<juce::String, float>& outValues) const
{
    if (index >= 0 && index < (int) factoryPresets.size())
    {
        outValues = factoryPresets[(size_t) index].values;
        return true;
    }
    
    int userIndex = index - (int) factoryPresets.size();
    if (userIndex >= 0 && userIndex < (int) userPresets.size())
    {
        outValues = userPresets[(size_t) userIndex].values;
        return true;
    }

    return false;
}

bool PresetManager::saveUserPreset(const juce::String& name, const juce::XmlElement& apvtsState)
{
    auto dir = getUserPresetsDirectory();
    if (!dir.exists())
        dir.createDirectory();

    auto file = dir.getChildFile(name + ".boratoeq");
    
    juce::XmlElement root("BORATOEQ_PRESET");
    root.setAttribute("name", name);
    root.setAttribute("pluginVersion", "0.2.0");
    root.addChildElement(new juce::XmlElement(apvtsState));

    if (root.writeTo(file, juce::XmlElement::TextFormat()))
    {
        scanUserPresets();
        return true;
    }
    return false;
}

bool PresetManager::deleteUserPreset(const juce::String& name)
{
    auto file = getUserPresetsDirectory().getChildFile(name + ".boratoeq");
    if (file.existsAsFile() && file.deleteFile())
    {
        scanUserPresets();
        return true;
    }
    return false;
}

void PresetManager::openUserPresetsFolder() const
{
    auto dir = getUserPresetsDirectory();
    if (!dir.exists())
        dir.createDirectory();
    dir.revealToUser();
}
