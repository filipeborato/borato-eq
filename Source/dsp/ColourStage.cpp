#include "ColourStage.h"

float saturateTube(float x, float drive) noexcept
{
    // drive maps to a tanh curve whose full-scale points are preserved
    // (tanh(k*x)/tanh(k)), wet/dry blended by the same drive so that
    // drive == 0 is perfectly transparent.
    const float k = 1.0f + drive * 5.0f;
    const float shaped = std::tanh(k * x) / std::tanh(k);
    return x + drive * (shaped - x);
}

float saturateTransformer(float x, float amount) noexcept
{
    // Asymmetric bias before a tanh introduces even-order harmonics. The DC
    // offset this creates is removed by the per-channel DC blocker in process().
    const float b = amount * 0.25f;
    const float shaped = std::tanh(x + b * x * x);
    return x + amount * (shaped - x);
}

void ColourStage::prepare(double sampleRate, int numChannels)
{
    sr = sampleRate;
    channels = juce::jlimit(1, 2, numChannels);

    const auto onePole = [this](float cutoffHz)
    {
        return 1.0f - std::exp(-2.0f * juce::MathConstants<float>::pi * cutoffHz / (float) sr);
    };

    hfCoeff = onePole(7000.0f); // smoother above ~7 kHz when damping
    lfCoeff = onePole(160.0f);  // extracts low-frequency body for weight

    reset();
}

void ColourStage::reset()
{
    state.fill({});
}

void ColourStage::setParams(bool tubeOn, bool ironOn, float colour01, float voicing01) noexcept
{
    tube = tubeOn;
    iron = ironOn;
    colour = juce::jlimit(0.0f, 1.0f, colour01);
    voicing = juce::jlimit(0.0f, 1.0f, voicing01);
}

void ColourStage::process(juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    const int numCh = juce::jmin(channels, buffer.getNumChannels());

    const bool doTube = tube && colour > 0.0001f;
    const bool doIron = iron && voicing > 0.0001f;

    if (! doTube && ! doIron)
        return;

    const float dampMix   = voicing * 0.45f; // how much HF damping to blend in
    const float weightAmt = voicing * 0.18f; // how much LF body to add back

    for (int ch = 0; ch < numCh; ++ch)
    {
        auto* data = buffer.getWritePointer(ch);
        auto& st = state[(size_t) ch];

        for (int n = 0; n < numSamples; ++n)
        {
            float s = data[n];

            if (doTube)
                s = saturateTube(s, colour);

            if (doIron)
            {
                // Asymmetric transformer drive + DC blocker.
                const float a = saturateTransformer(s, voicing);
                const float dc = a - st.dcX1 + 0.995f * st.dcY1;
                st.dcX1 = a;
                st.dcY1 = dc;
                s = dc;

                // Gentle high-frequency damping.
                st.dampState += hfCoeff * (s - st.dampState);
                s = s * (1.0f - dampMix) + st.dampState * dampMix;

                // Low-frequency weight: add a touch of the extracted body.
                st.weightState += lfCoeff * (s - st.weightState);
                s += weightAmt * st.weightState;
            }

            data[n] = s;
        }
    }
}
