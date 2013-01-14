#include "marsyas_plugin.hpp"
#include "../marsystems/buffer.hpp"
#include "../marsystems/linear_trend.hpp"
#include "../marsystems/mfcc.hpp"
#include "../marsystems/chromaticentropy.hpp"
#include "../marsystems/etno_muse_classifier.hpp"
#include "../marsystems/vampsink.hpp"

#include <marsyas/MarSystemManager.h>

#include <string>
#include <iostream>
#include <cmath>

using namespace std;
using namespace Marsyas;

namespace Segmenter {

MarPlugin::MarPlugin(float inputSampleRate):
    Vamp::Plugin(inputSampleRate),
    m_pipeline(0),
    m_vampSink(0)
{
    m_inputInfo.sampleRate = inputSampleRate;

    m_pipeInfo.sampleRate = 11025;
    m_pipeInfo.blockSize = 512;
    m_pipeInfo.stepSize = 256;
    m_pipeInfo.mfccCount = 27;
    m_pipeInfo.statStepSize = 22;
    m_pipeInfo.statBlockSize = m_pipeInfo.statStepSize * 6;

    m_inputInfo.blockSize =
        std::ceil( m_pipeInfo.stepSize * (m_inputInfo.sampleRate / m_pipeInfo.sampleRate) );
}

MarPlugin::~MarPlugin()
{
    deletePipeline();
}

string MarPlugin::getIdentifier() const
{
    return "segmenter-marsyas";
}

string MarPlugin::getName() const
{
    return "Segmenter (Marsyas)";
}

string MarPlugin::getDescription() const
{
    return "Segment audio into segments of various sound types";
}


string MarPlugin::getMaker() const
{
    return "Matija Marolt & Jakob Leben";
}

string MarPlugin::getCopyright() const
{
    return ("Copyright 2013 Matija Marolt & Jakob Leben, "
            "Faculty of Computer and Information Science, University of Ljubljana.");
}

int MarPlugin::getPluginVersion() const
{
    return 1;
}

Vamp::Plugin::InputDomain MarPlugin::getInputDomain() const
{
    return TimeDomain;
}

size_t MarPlugin::getMinChannelCount() const
{
    return 1;
}

size_t MarPlugin::getMaxChannelCount() const
{
    return 1;
}

size_t MarPlugin::getPreferredBlockSize() const
{
    return m_inputInfo.blockSize;
}

size_t MarPlugin::getPreferredStepSize() const
{
    return getPreferredBlockSize();
}

bool MarPlugin::initialise(size_t channels, size_t stepSize, size_t blockSize)
{
    if (channels != 1)
        return false;

    if (stepSize != blockSize)
        return false;

    m_inputInfo.blockSize = blockSize;

    createPipeline();

    return true;
}

void MarPlugin::reset()
{
    createPipeline();
}

Vamp::Plugin::OutputList MarPlugin::getOutputDescriptors() const
{
    OutputList outputs;

    int featureFrames = std::ceil(
        m_pipeInfo.stepSize * m_pipeInfo.statStepSize
        * ((double) m_inputInfo.sampleRate / m_pipeInfo.sampleRate) );

    double featureSampleRate = (double) m_inputInfo.sampleRate / featureFrames;

    OutputDescriptor out;
    out.identifier = "energy";
    out.name = "Energy";
    out.sampleType = OutputDescriptor::FixedSampleRate;
    out.sampleRate = featureSampleRate;
    out.hasFixedBinCount = true;
    out.binCount = m_vampSink ? m_vampSink->getControl("mrs_natural/inObservations")->to<mrs_natural>() : 1;

    outputs.push_back(out);

    return outputs;
}

Vamp::Plugin::FeatureSet MarPlugin::process(const float *const *inputBuffers, Vamp::RealTime timeStamp)
{
    m_featureSet.clear();

    pipeProcess( inputBuffers[0] );

    return m_featureSet;
}

Vamp::Plugin::FeatureSet MarPlugin::getRemainingFeatures()
{
    return FeatureSet();
}

void MarPlugin::createPipeline()
{
    deletePipeline();

    MarSystemManager mng;

    // prototype MarSystems

    MarSystem* protoBuffer = new Marsyas::Buffer("protoBuffer");
    mng.registerPrototype("Buffer", protoBuffer);

    MarSystem* protoMyMfcc = new Marsyas::MyMFCC("protoMyMfcc");
    mng.registerPrototype("MyMFCC", protoMyMfcc);

    MarSystem* protoLinearTrend = new Marsyas::LinearTrend("protoLinearTrend");
    mng.registerPrototype("LinearTrend", protoLinearTrend);

    MarSystem* protoChromaticEntropy = new Marsyas::ChromaticEntropy("protoChromaticEntropy");
    mng.registerPrototype("ChromaticEntropy", protoChromaticEntropy);

    MarSystem* protoClassifier = new Marsyas::EtnoMuseClassifier("protoEtnoMuseClassifier");
    mng.registerPrototype("EtnoMuseClassifier", protoClassifier);

    MarSystem* protoVampSink = new Marsyas::VampSink("protoVampSink");
    mng.registerPrototype("VampSink", protoVampSink);

    // create pipeline

    VampSink *vampSink = static_cast<VampSink*>( mng.create("VampSink", "vampSink") );
    vampSink->setFeatureSet( &m_featureSet );

    MarSystem *mfccPipe = mng.create("Series", "mfccPipe");
    mfccPipe->addMarSystem( mng.create("MyMFCC", "mfcc") );
    mfccPipe->addMarSystem( mng.create("RemoveObservations", "mfccRange") );

    MarSystem *spectrumProcFan = mng.create("Fanout", "specProcFan");
    spectrumProcFan->addMarSystem( mng.create("ChromaticEntropy", "entropy") );
    spectrumProcFan->addMarSystem( mfccPipe );

    MarSystem *trendPipe = mng.create("Series", "trendPipe");
    trendPipe->addMarSystem( mng.create("Memory", "buffer") );
    trendPipe->addMarSystem( mng.create("LinearTrend", "trend") );

    MarSystem *spectrumProc = mng.create("Cascade", "specProc");
    spectrumProc->addMarSystem( spectrumProcFan );
    spectrumProc->addMarSystem( trendPipe );

    MarSystem *spectrumPipe = mng.create("Series", "spectrumPipe");
    spectrumPipe->addMarSystem( mng.create("Spectrum", "spectrum") );
    spectrumPipe->addMarSystem( mng.create("PowerSpectrum", "powerSpectrum") );
    spectrumPipe->addMarSystem( spectrumProc );

    MarSystem *energyPipe = mng.create("Series", "energyPipe");
    energyPipe->addMarSystem( mng.create("Square", "square") );
    energyPipe->addMarSystem( mng.create("Mean", "mean") );

    MarSystem *windowFan = mng.create("Fanout", "windowFan");
    windowFan->addMarSystem( spectrumPipe );
    windowFan->addMarSystem( energyPipe );

    MarSystem *classificationPipe = mng.create("Series", "classificationPipe");
    classificationPipe->addMarSystem( mng.create("EtnoMuseClassifier", "classifier") );
    classificationPipe->addMarSystem( vampSink );

    MarSystem *statBuffer = mng.create("Buffer", "statBuffer");
    statBuffer->addMarSystem( classificationPipe );

#if 0
    MarSystem *statPipe = mng.create("Parallel", "statPipe");
    statPipe->addMarSystem( mng.create("Mean", "meanEntropy") );
    statPipe->addMarSystem( mng.create("Mean", "meanMfcc2") );
    statPipe->addMarSystem( mng.create("Mean", "meanMfcc3") );
    statPipe->addMarSystem( mng.create("Mean", "meanMfcc4") );
    statPipe->addMarSystem( mng.create("Mean", "meanDeltaEntropy") );
    statPipe->addMarSystem( mng.create("Mean", "meanDeltaMfcc2") );
    statPipe->addMarSystem( mng.create("Mean", "meanDeltaMfcc3") );
    statPipe->addMarSystem( mng.create("Mean", "meanDeltaMfcc4") );
    statPipe->addMarSystem( mng.create("Mean", "meanEnergy") );
#endif

    MarSystem *windowedPipe = mng.create("Series", "windowedPipe");
    windowedPipe->addMarSystem( windowFan );
    //windowedPipe->addMarSystem( vampSink );
    windowedPipe->addMarSystem( statBuffer );

    MarSystem *winBuffer = mng.create("Buffer", "winBuffer");
    winBuffer->addMarSystem( windowedPipe );

    m_pipeline = mng.create("Series", "pipeline");
    m_pipeline->addMarSystem( mng.create("ResampleSinc", "resampler") );
    m_pipeline->addMarSystem( winBuffer );

    // set controls

    m_pipeline->setControl( "mrs_real/israte", m_inputInfo.sampleRate );
    m_pipeline->setControl( "mrs_natural/inSamples", m_inputInfo.blockSize );

    m_pipeline->setControl( "ResampleSinc/resampler/mrs_real/stretch", m_pipeInfo.sampleRate / m_inputInfo.sampleRate );

    winBuffer->setControl( "mrs_natural/blockSize", m_pipeInfo.blockSize );
    winBuffer->setControl( "mrs_natural/hopSize", m_pipeInfo.stepSize );

    statBuffer->setControl( "mrs_natural/blockSize", m_pipeInfo.statBlockSize );
    statBuffer->setControl( "mrs_natural/hopSize", m_pipeInfo.statStepSize );

    trendPipe->setControl( "Memory/buffer/mrs_natural/memSize", 5 );

    mfccPipe->setControl( "MyMFCC/mfcc/mrs_natural/coefficients", m_pipeInfo.mfccCount );
    mfccPipe->setControl( "RemoveObservations/mfccRange/mrs_real/lowCutoff", (float) 2 / (m_pipeInfo.mfccCount-1) );
    mfccPipe->setControl( "RemoveObservations/mfccRange/mrs_real/highCutoff", (float) 4 / (m_pipeInfo.mfccCount-1) );

    int featureFrames = std::ceil(
        m_pipeInfo.stepSize * m_pipeInfo.statStepSize
        * (m_inputInfo.sampleRate / m_pipeInfo.sampleRate) );

    double featureDistance = (double) featureFrames / m_inputInfo.sampleRate;

    vampSink->setControl( "mrs_real/featureDistance", featureDistance );

    m_pipeline->update();

    int outObservationCount = m_pipeline->getControl( "mrs_natural/onObservations" )->to<mrs_natural>();
    int outSampleCount = m_pipeline->getControl( "mrs_natural/onSamples" )->to<mrs_natural>();

    //std::cout << "*** Marsyas Segmenter: output format = [" << outObservationCount << "/" << outSampleCount << "]" << std::endl;

    m_inputVector.allocate( 1, m_inputInfo.blockSize );
    m_outputVector.allocate( outObservationCount, outSampleCount );

    m_vampSink = vampSink;
}

void MarPlugin::deletePipeline()
{
    delete(m_pipeline);
    m_pipeline = 0;
    m_vampSink = 0;
}

void MarPlugin::pipeProcess( const float *input )
{
    for (int s = 0; s < m_inputInfo.blockSize; ++s)
    {
        m_inputVector(0, s) = input[s];
    }

    m_pipeline->process( m_inputVector, m_outputVector );

}

} // namespace Segmenter
