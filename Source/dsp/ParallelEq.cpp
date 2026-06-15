#include "ParallelEq.h"

void ParallelEq::prepare(double sampleRate, int maxBlockSize, int numChannels)
{
    sr = sampleRate;
    channels = juce::jlimit(1, 2, numChannels);

    scratch.setSize(channels, maxBlockSize, false, false, true);

    reset();

    for (int b = 0; b < BoratoEq::numBands; ++b)
        updateBandCoeffs(b);

    setHpf(hpfFreq);
    setLpf(lpfFreq);
}

void ParallelEq::reset()
{
    for (auto& band : bandFilters)
        for (auto& f : band)
            f.reset();

    for (auto& pass : passFilters)
        for (auto& f : pass)
            f.reset();
}

void ParallelEq::setBand(int bandIndex, const BandSettings& s)
{
    if (bandIndex < 0 || bandIndex >= BoratoEq::numBands)
        return;

    auto& cur = bandSettings[(size_t) bandIndex];
    const bool changed = cur.freq != s.freq || cur.gainDb != s.gainDb
                       || cur.q != s.q || cur.type != s.type;

    cur = s;
    bandActive[(size_t) bandIndex] = s.on && std::abs(s.gainDb) > 0.01f;

    if (changed)
        updateBandCoeffs(bandIndex);
}

void ParallelEq::updateBandCoeffs(int bandIndex)
{
    const auto& s = bandSettings[(size_t) bandIndex];
    const float freq = juce::jlimit(20.0f, (float) (sr * 0.49), s.freq);
    const float q = juce::jmax(0.1f, s.q);
    const float gain = juce::Decibels::decibelsToGain(s.gainDb);

    Coeffs::Ptr c;
    switch (s.type)
    {
        case BoratoEq::FilterType::lowShelf:
            c = Coeffs::makeLowShelf(sr, freq, q, gain);
            break;
        case BoratoEq::FilterType::highShelf:
            c = Coeffs::makeHighShelf(sr, freq, q, gain);
            break;
        case BoratoEq::FilterType::bell:
        case BoratoEq::FilterType::notch: // v1: notch behaves as a narrow bell
        default:
            c = Coeffs::makePeakFilter(sr, freq, q, gain);
            break;
    }

    for (int ch = 0; ch < 2; ++ch)
        bandFilters[(size_t) bandIndex][(size_t) ch].coefficients = c;
}

void ParallelEq::setHpf(float freqHz)
{
    hpfFreq = freqHz;
    hpfActive = freqHz > 20.5f;
    if (! hpfActive)
        return;

    auto c = Coeffs::makeHighPass(sr, juce::jlimit(20.0f, (float) (sr * 0.49), freqHz), 0.707f);
    for (int ch = 0; ch < 2; ++ch)
        passFilters[0][(size_t) ch].coefficients = c;
}

void ParallelEq::setLpf(float freqHz)
{
    lpfFreq = freqHz;
    lpfActive = freqHz < 19900.0f;
    if (! lpfActive)
        return;

    auto c = Coeffs::makeLowPass(sr, juce::jlimit(1000.0f, (float) (sr * 0.49), freqHz), 0.707f);
    for (int ch = 0; ch < 2; ++ch)
        passFilters[1][(size_t) ch].coefficients = c;
}

void ParallelEq::process(juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    const int numCh = juce::jmin(channels, buffer.getNumChannels());

    if (scratch.getNumSamples() < numSamples || scratch.getNumChannels() < numCh)
        scratch.setSize(juce::jmax(numCh, channels), numSamples, false, false, true);

    for (int ch = 0; ch < numCh; ++ch)
    {
        auto* dry = buffer.getWritePointer(ch);
        auto* acc = scratch.getWritePointer(ch);

        for (int n = 0; n < numSamples; ++n)
        {
            float d = dry[n];

            if (hpfActive) d = passFilters[0][(size_t) ch].processSample(d);
            if (lpfActive) d = passFilters[1][(size_t) ch].processSample(d);

            // Parallel sum of per-band deltas around the (post-pass) dry signal.
            float sum = d;
            for (int b = 0; b < BoratoEq::numBands; ++b)
            {
                if (! bandActive[(size_t) b])
                    continue;
                const float f = bandFilters[(size_t) b][(size_t) ch].processSample(d);
                sum += f - d;
            }

            acc[n] = sum;
        }

        juce::FloatVectorOperations::copy(dry, acc, numSamples);
    }
}
