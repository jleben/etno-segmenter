#include "plugin.hpp"
#include "../modules/resampler.hpp"
#include "../modules/energy.hpp"
#include "../modules/spectrum.hpp"
#include "../modules/mfcc.hpp"
#include "../modules/entropy.hpp"
#include "../modules/statistics.hpp"
#include "../modules/classification.hpp"

#include <vamp/vamp.h>
#include <vamp-sdk/PluginAdapter.h>

#include <sstream>
#include <iostream>

#define SEGMENTER_NO_RESAMPLING 0

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

Plugin::Plugin(float inputSampleRate):
    Vamp::Plugin(inputSampleRate),
    m_resampler(0),
    m_energy(0),
    m_spectrum(0),
    m_mfcc(0),
    m_entropy(0),
    m_classifier(0),
    m_statistics(0),
    m_statBlockSize(132),
    m_statStepSize(22)
{
    m_inputContext.sampleRate = inputSampleRate;

#if SEGMENTER_NO_RESAMPLING
    m_procContext.sampleRate = inputSampleRate;
    m_procContext.blockSize = 2048;
    m_procContext.stepSize = 1024;
#else
    m_procContext.sampleRate = 11025;
    m_procContext.blockSize = 512;
    m_procContext.stepSize = 256;
#endif

}

Plugin::~Plugin()
{
    deleteModules();
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
    // FIXME: do not assume a specific downsampling ratio
    return 4096; // == 512 * 4 (downsampling ratio) * 2 (resampler buffering)
}

size_t Plugin::getPreferredStepSize() const
{
    return getPreferredBlockSize();
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

    if (blockSize != stepSize)
        return false;

    m_inputContext.blockSize = blockSize;

    const int mfccFilterCount = 27;

    const int chromEntropyLoFreq = 55;
    const int chromEntropyHiFreq = 2000;

    const int statDeltaBlockSize = 5;

    m_resampler = new Resampler( m_inputContext.sampleRate, m_procContext.sampleRate );
    m_energy = new Energy( m_procContext.blockSize );
    m_spectrum = new PowerSpectrum( m_procContext.blockSize );
    m_mfcc = new Mfcc( m_procContext.sampleRate, m_procContext.blockSize, mfccFilterCount );
    m_entropy = new ChromaticEntropy( m_procContext.sampleRate, m_procContext.blockSize,
                                      chromEntropyLoFreq, chromEntropyHiFreq );
    m_statistics = new Statistics(m_statBlockSize, m_statStepSize, statDeltaBlockSize);
    m_classifier = new Classifier();

    initStatistics();
}

void Plugin::reset()
{
    std::cout << "**** Plugin::reset" << std::endl;
    deleteModules();
    initStatistics();
    m_resampBuffer.clear();
}

Vamp::Plugin::OutputList Plugin::getOutputDescriptors() const
{
    OutputList list;

    OutputDescriptor outFeature;
    outFeature.sampleType = OutputDescriptor::FixedSampleRate;
    outFeature.sampleRate = m_procContext.sampleRate / m_procContext.stepSize;

    outFeature.identifier = "entropy";
    outFeature.name = "Entropy";
    outFeature.description = "Chromatic Entropy";
    outFeature.hasFixedBinCount = true;
    outFeature.binCount = 1;
    list.push_back(outFeature);

    outFeature.identifier = "melspectrum";
    outFeature.name = "Mel-Spectrum";
    outFeature.description = "Mel Scale Power Spectrum";
    outFeature.hasFixedBinCount = true;
    outFeature.binCount = m_entropy ? m_entropy->melBinCount() : 1 ;
    if (m_entropy) {
        std::vector<std::string> labels;
        const std::vector<float> &freqs = m_entropy->melFrequencies();
        labels.reserve( freqs.size() );
        for (int idx = 0; idx < freqs.size(); ++idx) {
            std::ostringstream out;
            out << freqs[idx];
            labels.push_back( out.str() );
        }
        outFeature.binNames = labels;
    }
    list.push_back(outFeature);

    OutputDescriptor outStat;
    outStat.hasFixedBinCount = true;
    outStat.binCount = 1;
    outStat.sampleType = OutputDescriptor::VariableSampleRate;
    outStat.sampleRate = (double) m_procContext.sampleRate / (m_statStepSize * m_procContext.stepSize);

    outStat.identifier = "energyfluctuation";
    outStat.name = "Energy Fluctuation";
    outStat.description = "Energy Fluctuation";
    list.push_back(outStat);

    outStat.identifier = "entropymean";
    outStat.name = "Chromatic Entropy Mean";
    outStat.description = "Chromatic Entropy Mean";
    list.push_back(outStat);

    outStat.identifier = "deltaentropyvariance";
    outStat.name = "Chromatic Entropy Delta Variance";
    outStat.description = "Chromatic Entropy Delta Variance";
    list.push_back(outStat);

    outStat.identifier = "mfcc2var";
    outStat.name = "MFCC (2) Variance";
    list.push_back(outStat);

    outStat.identifier = "dmfcc2var";
    outStat.name = "MFCC (2) Delta Variance";
    list.push_back(outStat);

    OutputDescriptor outClasses;
    outClasses.hasFixedBinCount = true;
    outClasses.binCount = 5;
    if (m_classifier)
        outClasses.binNames = m_classifier->classNames();
    outClasses.sampleType = OutputDescriptor::FixedSampleRate;
    outClasses.sampleRate = outStat.sampleRate;
    outClasses.identifier = "classification";
    outClasses.name = "Classification";
    list.push_back(outClasses);

    return list;
}

Vamp::Plugin::FeatureSet Plugin::process(const float *const *inputBuffers, Vamp::RealTime timestamp)
{
    FeatureSet features;

    const float *input = inputBuffers[0];
#if SEGMENTER_NO_RESAMPLING
    m_resampBuffer.insert( m_resampBuffer.end(), input, input + m_inputContext.blockSize );
#else
    m_resampler->process( input, m_inputContext.blockSize, m_resampBuffer );
#endif

    int blockFrame;

    for ( blockFrame = 0;
          blockFrame <= (int) m_resampBuffer.size() - m_procContext.blockSize;
          blockFrame += m_procContext.stepSize )
    {
        const float *block = m_resampBuffer.data() + blockFrame;

        m_energy->process( block );
        m_spectrum->process( block );
        m_mfcc->process( m_spectrum->output() );
        m_entropy->process( m_spectrum->output() );
        m_statistics->process( m_energy->output(), m_mfcc->output(), m_entropy->output() );

        Feature entropy;
        entropy.values.push_back( m_entropy->output() );
        features[0].push_back(entropy);

        Feature melSpectrum;
        melSpectrum.values = m_entropy->melSpectrum();
        features[1].push_back(melSpectrum);

        const std::vector<Statistics::OutputFeatures> & stats = m_statistics->output();
        for (int i = 0; i < stats.size(); ++i)
        {
            const Statistics::OutputFeatures & stat = stats[i];

            m_classifier->process( stat.features );

/*
            Feature statFeature;
            statFeature.hasTimestamp = true;
            statFeature.timestamp = m_statsTime;
            statFeature.values.resize(1);
            float & value = statFeature.values[0];

            value = stat[Statistics::ENERGY_FLUX];
            features[2].push_back(statFeature);

            value = stat.entropyMean;
            features[3].push_back(statFeature);

            value = stat.deltaEntropyVar;
            features[4].push_back(statFeature);

            value = stat.mfcc2Var;
            features[5].push_back(statFeature);

            value = stat.deltaMfcc2Var;
            features[6].push_back(statFeature);
*/
            Feature classification;
            classification.hasTimestamp = true;
            classification.timestamp = m_statsTime;
            classification.values = m_classifier->probabilities();

            features[7].push_back( classification );

            m_statsTime = m_statsTime + m_statsStepDuration;
        }
    }

    m_resampBuffer.erase( m_resampBuffer.begin(), m_resampBuffer.begin() + blockFrame );

    return features;
}

Vamp::Plugin::FeatureSet Plugin::getRemainingFeatures()
{
    return FeatureSet();
}

void Plugin::deleteModules()
{
    delete m_resampler;
    m_resampler = 0;
    delete m_energy;
    m_energy = 0;
    delete m_spectrum;
    m_spectrum = 0;
    delete m_mfcc;
    m_mfcc = 0;
    delete m_entropy;
    m_energy = 0;
    delete m_statistics;
    m_statistics = 0;
    delete m_classifier;
    m_classifier = 0;
}

} // namespace Segmenter
