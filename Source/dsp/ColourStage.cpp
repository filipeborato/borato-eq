#include "ColourStage.h"

float saturateTube(float x, float drive) noexcept
{
    // Single-ended-triode character: a fixed input bias makes the curve
    // asymmetric so it generates a dominant 2nd harmonic (the "warm" tube
    // colour) rather than the odd-only harmonics of a symmetric tanh.
    //   - kBias: operating-point offset (part of the voice, not user facing).
    //   - kMax:  gentler max drive than IRON -> softer, more musical knee.
    // The DC the bias introduces is removed analytically (subtract tanh(bias)),
    // and the result is normalised so small signals stay near unity gain.
    constexpr float kBias = 0.18f;
    constexpr float kMax  = 3.5f;

    const float k   = 1.0f + drive * (kMax - 1.0f);
    const float dc  = std::tanh(kBias * k);
    const float wet = (std::tanh((x + kBias) * k) - dc) / (1.0f - dc + 1.0e-9f);
    return x + drive * (wet - x); // transparent at drive == 0
}

float saturateTransformer(float x, float amount) noexcept
{
    // Slightly asymmetric drive: the +bias before tanh generates even-order
    // harmonics; subtracting tanh(bias) removes the DC the bias introduces.
    const float drive = juce::jmap(amount, 0.0f, 1.0f, 1.0f, 2.5f);
    const float bias = 0.03f * amount;
    float y = std::tanh((x + bias) * drive) - std::tanh(bias);
    y *= juce::jmap(amount, 0.0f, 1.0f, 1.0f, 0.72f); // simple gain compensation
    return x + amount * (y - x); // transparent at amount == 0
}

void ColourStage::prepare(double sampleRate, int maxBlockSize, int numChannels)
{
    sr = sampleRate;
    channels = juce::jlimit(1, 2, numChannels);

    // 2x oversampling (factor exponent 1) with a steep equiripple half-band
    // FIR, integer latency for clean host reporting.
    oversampler = std::make_unique<juce::dsp::Oversampling<float>>(
        (size_t) channels, 1,
        juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple,
        true,   // maximum quality
        true);  // integer latency
    oversampler->initProcessing((size_t) juce::jmax(1, maxBlockSize));

    // IRON HF rounding runs inside the oversampled block, so use 2*sr.
    hfCoeffOs = 1.0f - std::exp(-2.0f * juce::MathConstants<float>::pi * 6000.0f / (float) (sr * 2.0));

    // IRON low-mid emphasis is fixed (it is the "iron in the path"): a gentle
    // shelf that gives body before the asymmetric saturation.
    auto emph = Coeffs::makeLowShelf(sr, 220.0f, 0.6f, juce::Decibels::decibelsToGain(2.5f));
    for (auto& f : ironEmph)
        f.coefficients = emph;

    lastVoicing = -1.0f;
    updateVoicing();
    reset();
}

void ColourStage::reset()
{
    for (auto& f : voiceLow)  f.reset();
    for (auto& f : voiceHigh) f.reset();
    for (auto& f : ironEmph)  f.reset();
    dampState.fill(0.0f);
    if (oversampler != nullptr)
        oversampler->reset();
}

int ColourStage::getLatencySamples() const
{
    return oversampler != nullptr ? juce::roundToInt(oversampler->getLatencyInSamples()) : 0;
}

void ColourStage::updateVoicing()
{
    if (voicing == lastVoicing)
        return;
    lastVoicing = voicing;

    // Tilt around the 0.5 neutral point.
    //   voicing 0   -> low +3 dB, high -3 dB   (dark / vintage)
    //   voicing 0.5 -> flat                     (neutral)
    //   voicing 1   -> low -1.5 dB, high +3 dB  (open / modern)
    const float lowDb  = juce::jmap(voicing, 0.0f, 1.0f, 3.0f, -1.5f);
    const float highDb = juce::jmap(voicing, 0.0f, 1.0f, -3.0f, 3.0f);

    auto low  = Coeffs::makeLowShelf(sr, 160.0f, 0.5f, juce::Decibels::decibelsToGain(lowDb));
    auto high = Coeffs::makeHighShelf(sr, 9000.0f, 0.5f, juce::Decibels::decibelsToGain(highDb));

    for (auto& f : voiceLow)  f.coefficients = low;
    for (auto& f : voiceHigh) f.coefficients = high;
}

void ColourStage::setParams(bool tubeOn, bool ironOn, float colour01, float voicing01) noexcept
{
    tube = tubeOn;
    iron = ironOn;
    colour = juce::jlimit(0.0f, 1.0f, colour01);
    voicing = juce::jlimit(0.0f, 1.0f, voicing01);
    updateVoicing();
}

void ColourStage::process(juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    const int numCh = juce::jmin(channels, buffer.getNumChannels());
    if (numCh <= 0 || numSamples <= 0 || oversampler == nullptr)
        return;

    // IRON drive scales with COLOR, but keeps a baseline when engaged so the
    // button is audible (weight + rounding) even before COLOR is raised.
    const float ironSat  = juce::jmap(colour, 0.0f, 1.0f, 0.15f, 1.0f);
    const float ironDamp = juce::jmap(colour, 0.0f, 1.0f, 0.20f, 0.50f);

    // --- Base-rate linear shaping: VOICING tilt (+ IRON emphasis) ---
    for (int ch = 0; ch < numCh; ++ch)
    {
        auto* data = buffer.getWritePointer(ch);
        for (int n = 0; n < numSamples; ++n)
        {
            float s = data[n];
            s = voiceLow[(size_t) ch].processSample(s);
            s = voiceHigh[(size_t) ch].processSample(s);
            if (iron)
                s = ironEmph[(size_t) ch].processSample(s);
            data[n] = s;
        }
    }

    // --- Oversampled non-linear stages: TUBE + IRON saturation/rounding ---
    juce::dsp::AudioBlock<float> block(buffer);
    auto sub = block.getSubsetChannelBlock(0, (size_t) numCh);
    auto up = oversampler->processSamplesUp(sub);

    const int osN = (int) up.getNumSamples();
    for (int ch = 0; ch < numCh; ++ch)
    {
        auto* data = up.getChannelPointer((size_t) ch);
        for (int n = 0; n < osN; ++n)
        {
            float s = data[n];

            if (tube)
                s = saturateTube(s, colour);

            if (iron)
            {
                s = saturateTransformer(s, ironSat);
                dampState[(size_t) ch] += hfCoeffOs * (s - dampState[(size_t) ch]);
                s = s * (1.0f - ironDamp) + dampState[(size_t) ch] * ironDamp;
            }

            data[n] = s;
        }
    }

    oversampler->processSamplesDown(sub);
}
