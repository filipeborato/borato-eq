#include "PresetManager.h"
#include "PluginParameters.h"

PresetManager::PresetManager()
{
    std::map<juce::String, float> def;
    def["lpf"] = 20000.0f; def["hpf"] = 20.0f;
    def["inputGain"] = 0.0f; def["outputGain"] = 0.0f; def["link"] = 0.0f;
    def["tube"] = 0.0f; def["iron"] = 0.0f; def["colour"] = 50.0f; def["voicing"] = 50.0f; def["power"] = 1.0f;

    def["band1Freq"] = 100.0f; def["band1Gain"] = 0.0f; def["band1Q"] = 2.0f; def["band1Shape"] = 0.0f; def["band1On"] = 1.0f;
    def["band2Freq"] = 300.0f; def["band2Gain"] = 0.0f; def["band2Q"] = 2.0f; def["band2Shape"] = 0.0f; def["band2On"] = 1.0f;
    def["band3Freq"] = 1000.0f; def["band3Gain"] = 0.0f; def["band3Q"] = 2.0f; def["band3On"] = 1.0f;
    def["band4Freq"] = 4000.0f; def["band4Gain"] = 0.0f; def["band4Q"] = 2.0f; def["band4Shape"] = 0.0f; def["band4On"] = 1.0f;
    def["band5Freq"] = 10000.0f; def["band5Gain"] = 0.0f; def["band5Q"] = 2.0f; def["band5Shape"] = 1.0f; def["band5On"] = 1.0f;

    auto makePreset = [&](const juce::String& name) {
        PresetData pd; pd.name = name; pd.values = def; return pd;
    };

    auto p = makePreset("Default");
    factoryPresets.push_back(p);

    p = makePreset("Vocal Air");
    p.values["band4Gain"] = 2.0f;
    p.values["band5Gain"] = 4.0f;
    p.values["tube"] = 1.0f;
    p.values["colour"] = 30.0f;
    factoryPresets.push_back(p);

    p = makePreset("Bus Glue");
    p.values["band1Gain"] = 1.0f;
    p.values["band3Gain"] = -1.0f;
    p.values["tube"] = 1.0f;
    p.values["iron"] = 1.0f;
    p.values["colour"] = 40.0f;
    p.values["voicing"] = 40.0f;
    factoryPresets.push_back(p);

    p = makePreset("Low-End Weight");
    p.values["band1Gain"] = 4.0f;
    p.values["band2Gain"] = 2.0f;
    p.values["iron"] = 1.0f;
    p.values["voicing"] = 10.0f;
    factoryPresets.push_back(p);

    p = makePreset("Vintage Dark");
    p.values["band5Gain"] = -3.0f;
    p.values["lpf"] = 12000.0f;
    p.values["tube"] = 1.0f;
    p.values["iron"] = 1.0f;
    p.values["voicing"] = 0.0f;
    factoryPresets.push_back(p);

    p = makePreset("Modern Open");
    p.values["band4Gain"] = 2.0f;
    p.values["band5Gain"] = 3.0f;
    p.values["voicing"] = 100.0f;
    p.values["tube"] = 1.0f;
    factoryPresets.push_back(p);

    p = makePreset("Master Polish");
    p.values["band1Gain"] = 1.0f;
    p.values["band5Gain"] = 1.5f;
    p.values["tube"] = 1.0f;
    p.values["iron"] = 1.0f;
    p.values["colour"] = 20.0f;
    factoryPresets.push_back(p);

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
