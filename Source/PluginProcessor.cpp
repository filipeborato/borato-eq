#include "PluginProcessor.h"
#include "PluginEditor.h"

BoratoEqAudioProcessor::BoratoEqAudioProcessor()
    : AudioProcessor(BusesProperties()
          .withInput("Input", juce::AudioChannelSet::stereo(), true)
          .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "PARAMETERS", createParameterLayout())
{
    for (int b = 0; b < BoratoEq::numBands; ++b)
    {
        const auto& cfg = BoratoEq::bands[(size_t) b];
        freqParam[(size_t) b]  = apvts.getRawParameterValue(cfg.freqId);
        gainParam[(size_t) b]  = apvts.getRawParameterValue(cfg.gainId);
        qParam[(size_t) b]     = apvts.getRawParameterValue(cfg.qId);
        shapeParam[(size_t) b] = cfg.showShape ? apvts.getRawParameterValue(cfg.shapeId) : nullptr;
        onParam[(size_t) b]    = apvts.getRawParameterValue(cfg.onId);
    }

    lpfParam     = apvts.getRawParameterValue(ParamIDs::lpf);
    hpfParam     = apvts.getRawParameterValue(ParamIDs::hpf);
    inputParam   = apvts.getRawParameterValue(ParamIDs::inputGain);
    outputParam  = apvts.getRawParameterValue(ParamIDs::outputGain);
    linkParam    = apvts.getRawParameterValue(ParamIDs::link);
    tubeParam    = apvts.getRawParameterValue(ParamIDs::tube);
    ironParam    = apvts.getRawParameterValue(ParamIDs::iron);
    colourParam  = apvts.getRawParameterValue(ParamIDs::colour);
    voicingParam = apvts.getRawParameterValue(ParamIDs::voicing);
    powerParam   = apvts.getRawParameterValue(ParamIDs::power);

    // Listen to every parameter so coefficient recomputation is triggered from
    // the message thread, not re-run on every audio block.
    for (auto* p : getParameters())
        if (auto* withId = dynamic_cast<juce::AudioProcessorParameterWithID*>(p))
            apvts.addParameterListener(withId->paramID, this);

    storedSnapshot = captureEditableParameters(apvts);
    abA = storedSnapshot;
    abB = storedSnapshot;
}

BoratoEqAudioProcessor::~BoratoEqAudioProcessor()
{
    for (auto* p : getParameters())
        if (auto* withId = dynamic_cast<juce::AudioProcessorParameterWithID*>(p))
            apvts.removeParameterListener(withId->paramID, this);
}

void BoratoEqAudioProcessor::parameterChanged(const juce::String&, float)
{
    paramsDirty.store(true, std::memory_order_relaxed);
    presetDirty = true;
}

void BoratoEqAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    const int channels = getTotalNumOutputChannels();

    eq.prepare(sampleRate, samplesPerBlock, channels);
    colour.prepare(sampleRate, samplesPerBlock, channels);
    setLatencySamples(colour.getLatencySamples());

    dryBuffer.setSize(channels, samplesPerBlock, false, false, true);

    inputSmooth.reset(sampleRate, 0.01);
    outputSmooth.reset(sampleRate, 0.01);
    powerSmooth.reset(sampleRate, 0.02);
    inputSmooth.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(inputParam->load()));
    outputSmooth.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(outputParam->load()));
    powerSmooth.setCurrentAndTargetValue(powerParam->load() > 0.5f ? 1.0f : 0.0f);

    paramsDirty.store(true, std::memory_order_relaxed);
    pullParametersIntoDsp();
}

void BoratoEqAudioProcessor::pullParametersIntoDsp()
{
    for (int b = 0; b < BoratoEq::numBands; ++b)
    {
        const auto& cfg = BoratoEq::bands[(size_t) b];
        const int qIndex = juce::jlimit(0, (int) BoratoEq::qValues.size() - 1,
                                        (int) qParam[(size_t) b]->load());
        const int shapeIndex = shapeParam[(size_t) b] != nullptr
                                   ? (int) shapeParam[(size_t) b]->load() : 0;

        ParallelEq::BandSettings s;
        s.freq   = freqParam[(size_t) b]->load();
        s.gainDb = gainParam[(size_t) b]->load();
        s.q      = BoratoEq::qValues[(size_t) qIndex];
        s.type   = BoratoEq::shapeForIndex(cfg, shapeIndex);
        s.on     = onParam[(size_t) b]->load() > 0.5f;
        eq.setBand(b, s);
    }

    eq.setHpf(hpfParam->load());
    eq.setLpf(lpfParam->load());

    colour.setParams(tubeParam->load() > 0.5f,
                     ironParam->load() > 0.5f,
                     colourParam->load() / 100.0f,
                     voicingParam->load() / 100.0f);
}

void BoratoEqAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    const int inputChannels = getTotalNumInputChannels();
    const int outputChannels = getTotalNumOutputChannels();

    for (int ch = inputChannels; ch < outputChannels; ++ch)
        buffer.clear(ch, 0, numSamples);

    if (paramsDirty.exchange(false, std::memory_order_relaxed))
        pullParametersIntoDsp();

    // Keep a clean copy of the input for the POWER bypass crossfade.
    if (dryBuffer.getNumChannels() < outputChannels || dryBuffer.getNumSamples() < numSamples)
        dryBuffer.setSize(outputChannels, numSamples, false, false, true);
    for (int ch = 0; ch < outputChannels; ++ch)
        dryBuffer.copyFrom(ch, 0, buffer, ch, 0, numSamples);

    // Input gain staging.
    inputSmooth.setTargetValue(juce::Decibels::decibelsToGain(inputParam->load()));
    if (numSamples > 0)
    {
        const float start = inputSmooth.getCurrentValue();
        const float end = inputSmooth.skip(numSamples);
        for (int ch = 0; ch < outputChannels; ++ch)
            buffer.applyGainRamp(ch, 0, numSamples, start, end);
    }

    // Parallel EQ then analogue colour stage.
    eq.process(buffer);
    colour.process(buffer);

    // Output gain staging. When LINK is engaged the output mirrors the input
    // trim so that pushing the input harder into the colour stage does not
    // change the overall level.
    const bool linked = linkParam->load() > 0.5f;
    const float outputDb = linked ? -inputParam->load() : outputParam->load();
    outputSmooth.setTargetValue(juce::Decibels::decibelsToGain(outputDb));
    if (numSamples > 0)
    {
        const float start = outputSmooth.getCurrentValue();
        const float end = outputSmooth.skip(numSamples);
        for (int ch = 0; ch < outputChannels; ++ch)
            buffer.applyGainRamp(ch, 0, numSamples, start, end);
    }

    // Master POWER: crossfade between the processed signal and the clean dry
    // copy so toggling bypass never clicks.
    powerSmooth.setTargetValue(powerParam->load() > 0.5f ? 1.0f : 0.0f);
    if (! powerSmooth.isSmoothing() && powerSmooth.getCurrentValue() >= 0.999f)
        return; // fully powered: nothing to blend

    for (int n = 0; n < numSamples; ++n)
    {
        const float p = powerSmooth.getNextValue();
        for (int ch = 0; ch < outputChannels; ++ch)
        {
            auto* wet = buffer.getWritePointer(ch);
            const auto* dry = dryBuffer.getReadPointer(ch);
            wet[n] = wet[n] * p + dry[n] * (1.0f - p);
        }
    }
}

juce::AudioProcessorEditor* BoratoEqAudioProcessor::createEditor()
{
    return new BoratoEqAudioProcessorEditor(*this);
}

void BoratoEqAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    if (auto xml = createPersistedStateXml(apvts, storedSnapshot, abA, abB, abSlotB))
        copyXmlToBinary(*xml, destData);
}

void BoratoEqAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
    {
        if (xml->hasTagName("BORATOEQ_STATE"))
            restorePersistedState(*xml);
        else
            apvts.replaceState(juce::ValueTree::fromXml(*xml));
    }
}

bool BoratoEqAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto& mainOut = layouts.getMainOutputChannelSet();
    if (mainOut != juce::AudioChannelSet::mono() && mainOut != juce::AudioChannelSet::stereo())
        return false;

    return layouts.getMainInputChannelSet() == mainOut;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BoratoEqAudioProcessor();
}

int BoratoEqAudioProcessor::getNumPrograms() { return presetManager.getNumPrograms(); }
int BoratoEqAudioProcessor::getCurrentProgram() { return currentProgram; }
const juce::String BoratoEqAudioProcessor::getProgramName(int index) { return presetManager.getProgramName(index); }
void BoratoEqAudioProcessor::setCurrentProgram(int index) { applyPreset(index); }

void BoratoEqAudioProcessor::applyPreset(int index)
{
    std::map<juce::String, float> values;
    if (presetManager.getPresetValues(index, values))
    {
        applyParameterMap(apvts, values);
        currentProgram = index;
        presetDirty = false;
        storeSnapshot();
        if (abSlotB) abB = storedSnapshot;
        else abA = storedSnapshot;
    }
}

void BoratoEqAudioProcessor::storeSnapshot()
{
    storedSnapshot = captureEditableParameters(apvts);
}

void BoratoEqAudioProcessor::recallSnapshot()
{
    applyParameterMap(apvts, storedSnapshot);
    presetDirty = false;
}

void BoratoEqAudioProcessor::swapAB()
{
    auto current = captureEditableParameters(apvts);
    if (abSlotB)
    {
        abB = current;
        applyParameterMap(apvts, abA);
    }
    else
    {
        abA = current;
        applyParameterMap(apvts, abB);
    }
    abSlotB = !abSlotB;
}

void BoratoEqAudioProcessor::copyAToB()
{
    auto current = captureEditableParameters(apvts);
    if (abSlotB) {
        abB = abA; 
        applyParameterMap(apvts, abB);
    } else {
        abA = current;
        abB = current;
    }
}

void BoratoEqAudioProcessor::setParameterValue(juce::AudioProcessorValueTreeState& state, const juce::String& id, float value)
{
    if (auto* parameter = state.getParameter(id))
    {
        const float normalised = parameter->convertTo0to1(value);
        parameter->beginChangeGesture();
        parameter->setValueNotifyingHost(normalised);
        parameter->endChangeGesture();
    }
}

BoratoEqAudioProcessor::ParameterMap BoratoEqAudioProcessor::captureEditableParameters(const juce::AudioProcessorValueTreeState& state)
{
    ParameterMap map;
    for (const auto* id : { ParamIDs::lpf, ParamIDs::hpf, ParamIDs::inputGain, ParamIDs::outputGain, ParamIDs::link,
                            ParamIDs::tube, ParamIDs::iron, ParamIDs::colour, ParamIDs::voicing, ParamIDs::power })
    {
        if (auto* value = state.getRawParameterValue(id))
            map[id] = value->load();
    }
    for (int b = 0; b < BoratoEq::numBands; ++b)
    {
        const auto& cfg = BoratoEq::bands[(size_t) b];
        for (const auto* id : { cfg.freqId, cfg.gainId, cfg.qId, cfg.onId })
            if (auto* value = state.getRawParameterValue(id))
                map[id] = value->load();
        
        if (cfg.showShape)
            if (auto* value = state.getRawParameterValue(cfg.shapeId))
                map[cfg.shapeId] = value->load();
    }
    return map;
}

void BoratoEqAudioProcessor::applyParameterMap(juce::AudioProcessorValueTreeState& state, const ParameterMap& map)
{
    for (const auto& [id, value] : map)
        setParameterValue(state, id, value);
}

juce::XmlElement BoratoEqAudioProcessor::createParameterMapXml(const juce::String& tagName, const ParameterMap& map)
{
    juce::XmlElement element(tagName);
    for (const auto& [id, value] : map)
    {
        auto* param = element.createNewChildElement("PARAM");
        param->setAttribute("id", id);
        param->setAttribute("value", value);
    }
    return element;
}

BoratoEqAudioProcessor::ParameterMap BoratoEqAudioProcessor::parseParameterMapXml(const juce::XmlElement& element)
{
    ParameterMap map;
    for (auto* child = element.getFirstChildElement(); child != nullptr; child = child->getNextElement())
    {
        if (child->hasTagName("PARAM"))
        {
            const auto id = child->getStringAttribute("id");
            if (id.isNotEmpty())
                map[id] = child->getDoubleAttribute("value", 0.0);
        }
    }
    return map;
}

std::unique_ptr<juce::XmlElement> BoratoEqAudioProcessor::createPersistedStateXml(
    const juce::AudioProcessorValueTreeState& state,
    const ParameterMap& storedSnapshotIn,
    const ParameterMap& abAIn,
    const ParameterMap& abBIn,
    bool abSlotBIn)
{
    auto root = std::make_unique<juce::XmlElement>("BORATOEQ_STATE");
    root->setAttribute("version", 1);
    root->setAttribute("abSlotB", abSlotBIn ? 1 : 0);

    root->addChildElement(state.state.createXml().release());
    root->addChildElement(new juce::XmlElement(createParameterMapXml("STORED_SNAPSHOT", storedSnapshotIn)));
    root->addChildElement(new juce::XmlElement(createParameterMapXml("AB_A", abAIn)));
    root->addChildElement(new juce::XmlElement(createParameterMapXml("AB_B", abBIn)));
    return root;
}

void BoratoEqAudioProcessor::restorePersistedState(const juce::XmlElement& element)
{
    if (auto* child = element.getFirstChildElement())
    {
        while (child != nullptr)
        {
            const auto tag = child->getTagName();
            if (tag != "STORED_SNAPSHOT" && tag != "AB_A" && tag != "AB_B")
            {
                apvts.replaceState(juce::ValueTree::fromXml(*child));
                break;
            }
            child = child->getNextElement();
        }
    }

    if (auto* stored = element.getChildByName("STORED_SNAPSHOT"))
        storedSnapshot = parseParameterMapXml(*stored);
    else
        storedSnapshot = captureEditableParameters(apvts);

    if (auto* a = element.getChildByName("AB_A"))
        abA = parseParameterMapXml(*a);
    else
        abA = storedSnapshot;

    if (auto* b = element.getChildByName("AB_B"))
        abB = parseParameterMapXml(*b);
    else
        abB = storedSnapshot;

    abSlotB = element.getBoolAttribute("abSlotB", false);
}
