#include "Settings.h"

Settings::Settings()
{
    juce::PropertiesFile::Options options;
    options.applicationName = "BORATO EQ";
    options.filenameSuffix = ".settings";
    options.osxLibrarySubFolder = "Application Support";
    options.folderName = "Borato Company";
    
    properties.setStorageParameters(options);
}

int Settings::getOversamplingMode()
{
    if (auto* file = properties.getUserSettings())
        return file->getIntValue("oversamplingMode", 1); // 0=Off, 1=2x, 2=4x
    return 1;
}

void Settings::setOversamplingMode(int mode)
{
    if (auto* file = properties.getUserSettings())
    {
        file->setValue("oversamplingMode", mode);
        file->saveIfNeeded();
    }
}

float Settings::getGuiScale()
{
    if (auto* file = properties.getUserSettings())
        return (float) file->getDoubleValue("guiScale", 1.0);
    return 1.0f;
}

void Settings::setGuiScale(float scale)
{
    if (auto* file = properties.getUserSettings())
    {
        file->setValue("guiScale", scale);
        file->saveIfNeeded();
    }
}

bool Settings::getShowTooltips()
{
    if (auto* file = properties.getUserSettings())
        return file->getBoolValue("showTooltips", true);
    return true;
}

void Settings::setShowTooltips(bool show)
{
    if (auto* file = properties.getUserSettings())
    {
        file->setValue("showTooltips", show);
        file->saveIfNeeded();
    }
}
