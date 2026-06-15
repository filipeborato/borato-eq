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
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary(*xml, destData);
}

void BoratoEqAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
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
