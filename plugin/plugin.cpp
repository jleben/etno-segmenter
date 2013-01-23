#include "plugin.hpp"
#include "../modules/resampler.hpp"
#include "../modules/energy.hpp"
#include "../modules/spectrum.hpp"
#include "../modules/mel_spectrum.hpp"
#include "../modules/mfcc.hpp"
#include "../modules/entropy.hpp"
#include "../modules/statistics.hpp"
#include "../modules/classification.hpp"

#include <vamp/vamp.h>

#include <sstream>
#include <iostream>

#define SEGMENTER_NO_RESAMPLING 0

using namespace std;

namespace Segmenter {

Plugin::Plugin(float inputSampleRate):
    Vamp::Plugin(inputSampleRate),
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

    deleteModules();

    m_modules.resize( ModuleCount );

    m_modules[Resampler] = new Segmenter::Resampler( m_inputContext.sampleRate, m_procContext.sampleRate );
    m_modules[Energy] = new Segmenter::Energy( m_procContext.blockSize );
    m_modules[PowerSpectrum] = new Segmenter::PowerSpectrum( m_procContext.blockSize );
    m_modules[MelSpectrum] = new Segmenter::MelSpectrum( mfccFilterCount, m_procContext.sampleRate,  m_procContext.blockSize );
    m_modules[Mfcc] = new Segmenter::Mfcc( mfccFilterCount );
    m_modules[ChromaticEntropy] = new Segmenter::ChromaticEntropy( m_procContext.sampleRate, m_procContext.blockSize,
                                                          chromEntropyLoFreq, chromEntropyHiFreq );
    m_modules[Statistics] = new Segmenter::Statistics(m_statBlockSize, m_statStepSize, statDeltaBlockSize);
    m_modules[Classifier] = new Segmenter::Classifier();

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

    int featureFrames = std::ceil(
        m_procContext.stepSize /* * m_pipeInfo.statStepSize*/
        * ((double) m_inputContext.sampleRate / m_procContext.sampleRate) );

    double featureSampleRate = (double) m_inputContext.sampleRate / featureFrames;

    OutputDescriptor outStat;
    outStat.identifier = "features";
    outStat.name = "Features";
    outStat.hasFixedBinCount = true;
    outStat.binCount = 5;
    outStat.sampleType = OutputDescriptor::FixedSampleRate;
    outStat.sampleRate = featureSampleRate;
    list.push_back(outStat);


    OutputDescriptor outClasses;
    outClasses.hasFixedBinCount = true;
    outClasses.binCount = 1;
    /*outClasses.binCount = classifier ? classifier->classNames().count() : 0;
    if (classifier) outClasses.binNames = classifier->classNames();*/
    /*
    outClasses.isQuantized;
    outClasses.quantizeStep = 1;
    outClasses.hasKnownExtents = true;
    outClasses.minValue = 1;
    outClasses.maxValue = 5;
    */

    outClasses.sampleType = OutputDescriptor::FixedSampleRate;
    outClasses.sampleRate = outStat.sampleRate;
    outClasses.identifier = "classification";
    outClasses.name = "Classification";
    outStat.unit = "class";
    list.push_back(outClasses);

    return list;
}

Vamp::Plugin::FeatureSet Plugin::process(const float *const *inputBuffers, Vamp::RealTime timestamp)
{
    return getFeatures( inputBuffers[0], timestamp );
}

Vamp::Plugin::FeatureSet Plugin::getRemainingFeatures()
{
    return getFeatures( 0, Vamp::RealTime() );
}

Vamp::Plugin::FeatureSet Plugin::getFeatures(const float * input, Vamp::RealTime timestamp)
{
    Segmenter::Resampler *resampler = static_cast<Segmenter::Resampler*>( module(Resampler) );
    Segmenter::Energy *energy = static_cast<Segmenter::Energy*>( module(Energy) );
    Segmenter::PowerSpectrum *powerSpectrum = static_cast<Segmenter::PowerSpectrum*>( module(PowerSpectrum) );
    Segmenter::MelSpectrum *melSpectrum = static_cast<Segmenter::MelSpectrum*>( module(MelSpectrum) );
    Segmenter::Mfcc *mfcc = static_cast<Segmenter::Mfcc*>( module(Mfcc) );
    Segmenter::ChromaticEntropy *chromaticEntropy = static_cast<Segmenter::ChromaticEntropy*>( module(ChromaticEntropy) );
    Segmenter::Statistics *statistics = static_cast<Segmenter::Statistics*>( module(Statistics) );
    Segmenter::Classifier *classifier = static_cast<Segmenter::Classifier*>( module(Classifier) );

    bool endOfStream = input == 0;

    FeatureSet features;

#if SEGMENTER_NO_RESAMPLING
    if (!endOfStream)
        m_resampBuffer.insert( m_resampBuffer.end(), input, input + m_inputContext.blockSize );
#else
    if (!endOfStream)
        resampler->process( input, m_inputContext.blockSize, m_resampBuffer );
    else
        resampler->processRemainingData( m_resampBuffer );
#endif

    int blockFrame;

    for ( blockFrame = 0;
          blockFrame <= (int) m_resampBuffer.size() - m_procContext.blockSize;
          blockFrame += m_procContext.stepSize )
    {
        const float *block = m_resampBuffer.data() + blockFrame;

        energy->process( block );
        powerSpectrum->process( block );
        melSpectrum->process( powerSpectrum->output() );
        mfcc->process( melSpectrum->output() );
        chromaticEntropy->process( powerSpectrum->output() );
        statistics->process( energy->output(), chromaticEntropy->output(),
                                     mfcc->output(), m_statsBuffer );

        Feature basicFeatures;
        basicFeatures.values.push_back( chromaticEntropy->output() );
        basicFeatures.values.push_back( mfcc->output()[2] );
        basicFeatures.values.push_back( mfcc->output()[3] );
        basicFeatures.values.push_back( mfcc->output()[4] );
        basicFeatures.values.push_back( energy->output() );

        features[0].push_back(basicFeatures);
    }

    if (endOfStream)
        statistics->processRemainingData( m_statsBuffer );

    for (int i = 0; i < m_statsBuffer.size(); ++i)
    {
        const Statistics::OutputFeatures & stat = m_statsBuffer[i];

        classifier->process( stat.features );

        const std::vector<float> & distribution = classifier->probabilities();
        float avgClass = 0;
        for (int i = 0; i < distribution.size(); ++i)
            avgClass += distribution[i] * i;
        avgClass /= distribution.size() - 1;

        Feature classification;
        classification.hasTimestamp = true;
        classification.timestamp = m_statsTime;
        classification.values.push_back( avgClass );
        //classification.label = "x";
        //classification.values = classifier->probabilities();

        features[1].push_back( classification );

        m_statsTime = m_statsTime + m_statsStepDuration;
    }

    m_statsBuffer.clear();
    m_resampBuffer.erase( m_resampBuffer.begin(), m_resampBuffer.begin() + blockFrame );

    return features;
}

void Plugin::deleteModules()
{
    int moduleCount = m_modules.size();

    for (int idx = 0; idx < m_modules.size(); ++idx)
        delete m_modules[idx];

    m_modules.clear();
}

} // namespace Segmenter
