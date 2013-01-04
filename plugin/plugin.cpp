#include "plugin.hpp"
#include "../modules/resampler.hpp"
#include "../modules/energy.hpp"
#include "../modules/spectrum.hpp"
#include "../modules/mfcc.hpp"
#include "../modules/entropy.hpp"
#include "../modules/statistics.hpp"

#include <vamp/vamp.h>
#include <vamp-sdk/PluginAdapter.h>

#include <sstream>
#include <iostream>

using namespace std;

static Vamp::PluginAdapter<Segmenter::Plugin> segmenterAdapter;

const VampPluginDescriptor *vampGetPluginDescriptor(unsigned int version, unsigned int index)
{
    if (version < 1) return 0;

    switch (index) {
    case 0:
        return segmenterAdapter.getDescriptor();
    default:
        return 0;
    }
}

namespace Segmenter {

static const int mfccFilterCount = 27;

Plugin::Plugin(float inputSampleRate):
    Vamp::Plugin(inputSampleRate),
    m_sampleRate( inputSampleRate ),
    m_blockSize(0),
    m_stepSize(0),
    m_resampler(0),
    m_spectrum(0),
    m_mfcc(0),
    m_entropy(0),
    m_statistics(0)
{}

Plugin::~Plugin()
{
    delete m_resampler;
    delete m_spectrum;
    delete m_mfcc;
}

string Plugin::getIdentifier() const
{
    return "audiosegmenter";
}

string Plugin::getName() const
{
    return "Audio Segmenter";
}

string Plugin::getDescription() const
{
    return "Segment audio into segments of various sound types";
}


string Plugin::getMaker() const
{
    return "Matija Marolt & Jakob Leben";
}

string Plugin::getCopyright() const
{
    return ("Copyright 2013 Matija Marolt & Jakob Leben, "
            "Faculty of Computer and Information Science, University of Ljubljana.");
}

int Plugin::getPluginVersion() const
{
    return 1;
}

Vamp::Plugin::InputDomain Plugin::getInputDomain() const
{
    return TimeDomain;
}

size_t Plugin::getMinChannelCount() const
{
    return 1;
}

size_t Plugin::getMaxChannelCount() const
{
    return 1;
}

size_t Plugin::getPreferredBlockSize() const
{
    return 2046;//512 * 4;
}

size_t Plugin::getPreferredStepSize() const
{
    return 1024;//256 * 4;
}

bool Plugin::initialise(size_t channels, size_t stepSize, size_t blockSize)
{
    std::cout << "**** Plugin::initialise:"
        << " " << channels
        << " " << stepSize
        << " " << blockSize
        << " " << std::endl;

    if (channels != 1)
        return false;

    //if (stepSize != 0

    m_blockSize = blockSize;
    m_stepSize = stepSize;

    m_resampler = new Resampler(m_sampleRate);
    m_energy = new Energy(m_blockSize);
    m_spectrum = new PowerSpectrum(m_blockSize);
    m_mfcc = new Mfcc(m_sampleRate, m_blockSize, mfccFilterCount);
    m_entropy = new ChromaticEntropy( m_sampleRate, m_blockSize, 55, 12000 );
    m_statistics = new Statistics();
}

void Plugin::reset()
{
    std::cout << "**** Plugin::reset" << std::endl;
    delete m_resampler;
    delete m_energy;
    delete m_spectrum;
    delete m_mfcc;
    delete m_entropy;
    delete m_statistics;
}

Vamp::Plugin::OutputList Plugin::getOutputDescriptors() const
{
    OutputList list;

    OutputDescriptor d;
    d.identifier = "entropy";
    d.name = "Entropy";
    d.description = "Chromatic Entropy";
    d.sampleType = OutputDescriptor::OneSamplePerStep;
    d.hasFixedBinCount = true;
    d.binCount = 1;
    list.push_back(d);

    d.identifier = "melspectrum";
    d.name = "MelSpectrum";
    d.description = "Mel-frequency power spectrum";
    d.sampleType = OutputDescriptor::OneSamplePerStep;
    d.hasFixedBinCount = true;
    d.binCount = m_entropy ? m_entropy->melBinCount() : 1 ;
    if (m_entropy) {
        std::vector<std::string> labels;
        const std::vector<float> &freqs = m_entropy->melFrequencies();
        labels.reserve( freqs.size() );
        for (int idx = 0; idx < freqs.size(); ++idx) {
            std::ostringstream out;
            out << freqs[idx];
            labels.push_back( out.str() );
        }
        d.binNames = labels;
    }
    list.push_back(d);

    OutputDescriptor outStat;
    outStat.hasFixedBinCount = true;
    outStat.binCount = 1;
    outStat.sampleType = OutputDescriptor::VariableSampleRate;
    outStat.sampleRate = m_sampleRate / (22.f * m_stepSize);

    outStat.identifier = "energyfluctuation";
    outStat.name = "Energy Fluctuation";
    outStat.name = "Energy Fluctuation";
    list.push_back(outStat);

    return list;
}

Vamp::Plugin::FeatureSet Plugin::process(const float *const *inputBuffers, Vamp::RealTime timestamp)
{
    const float *input = inputBuffers[0];

    m_energy->process(input);
    m_spectrum->process(input);
    m_mfcc->process( m_spectrum->output() );
    m_entropy->process( m_spectrum->output() );
    m_statistics->process( m_energy->output(), m_mfcc->output(), m_entropy->output(), false );


    Feature output;
    output.hasTimestamp = false;
    output.values.push_back( m_entropy->output() );
    //spectrum.hasDuration = true;
    //spectrum.duration = Vamp::RealTime::fromSeconds((double) m_blockSize / m_sampleRate);
    //spectrum.values = m_spectrum->output();

    FeatureSet features;
    features[0].push_back(output);

    Feature melSpectrum;
    melSpectrum.values = m_entropy->melSpectrum();
    features[1].push_back(melSpectrum);

    const int statOutOffset = 132 * m_stepSize / m_sampleRate;
    Vamp::RealTime statTimestamp = timestamp - Vamp::RealTime::fromSeconds(statOutOffset);

    const std::vector<Statistics::OutputFeatures> & stats = m_statistics->output();
    for (int i = 0; i < stats.size(); ++i)
    {
        const Statistics::OutputFeatures & stat = stats[i];

        Feature energyFlux;
        energyFlux.values.push_back( stat.energyFlux );
        energyFlux.hasTimestamp = true;
        energyFlux.timestamp = statTimestamp;
        features[2].push_back(energyFlux);
    }

    return features;
}

Vamp::Plugin::FeatureSet Plugin::getRemainingFeatures()
{
    return FeatureSet();
}

} // namespace Segmenter
