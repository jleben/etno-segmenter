#include "marsyas_plugin.hpp"
#include "../marsystems/buffer.hpp"
#include "../marsystems/chromaticentropy.hpp"
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
    m_pipeline(0)
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

    int featureFrames = std::ceil( m_pipeInfo.stepSize * ((double) m_inputInfo.sampleRate / m_pipeInfo.sampleRate) );
    double featureSampleRate = (double) m_inputInfo.sampleRate / featureFrames;

    OutputDescriptor out;
    out.identifier = "energy";
    out.name = "Energy";
    out.sampleType = OutputDescriptor::FixedSampleRate;
    out.sampleRate = featureSampleRate;
    out.hasFixedBinCount = true;
    out.binCount = 9;

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
#if 0
void MarPlugin::createPipeline()
{
    deletePipeline();

    MarSystemManager mng;

    MarSystem *mfccPipe = mng.create("Series", "mfccPipe");
    mfccPipe->addMarSystem( mng.create("MFCC", "mfcc") );
    mfccPipe->addMarSystem( mng.create("RemoveObservations", "mfccRange") );

    MarSystem *spectrumCascade = mng.create("Cascade", "cascade");
    spectrumCascade->addMarSystem( mfccPipe );
    spectrumCascade->addMarSystem( mng.create("DeltaFirstOrderRegression", "delta") );

    MarSystem *spectrumPipe = mng.create("Series", "spectrumPipe");
    spectrumPipe->addMarSystem( mng.create("Spectrum", "spectrum") );
    spectrumPipe->addMarSystem( mng.create("PowerSpectrum", "powerSpectrum") );
    spectrumPipe->addMarSystem( spectrumCascade );

    MarSystem *windowFan = mng.create("Fanout", "windowFan");
    windowFan->addMarSystem( mng.create("Energy", "energy") );
    windowFan->addMarSystem( spectrumPipe );

    MarSystem *windowedPipe = mng.create("Series", "windowedPipe");
    windowedPipe->addMarSystem( mng.create("ShiftInput", "windower") );
    windowedPipe->addMarSystem( windowFan );

    MarSystem *shredder = mng.create("Shredder", "shredder");
    shredder->addMarSystem( windowedPipe );

    MarSystem *statPipe = mng.create("Parallel", "statPipe");
    statPipe->addMarSystem( mng.create("Mean", "meanEnergy") );
    statPipe->addMarSystem( mng.create("Mean", "meanMfcc2") );
    statPipe->addMarSystem( mng.create("Mean", "meanMfcc3") );
    statPipe->addMarSystem( mng.create("Mean", "meanMfcc4") );
    statPipe->addMarSystem( mng.create("Mean", "meanDeltaMfcc2") );
    statPipe->addMarSystem( mng.create("Mean", "meanDeltaMfcc3") );
    statPipe->addMarSystem( mng.create("Mean", "meanDeltaMfcc4") );

    MarSystem *resampler = mng.create("ResampleLinear", "resampler");

    m_pipeline = mng.create("Series", "pipeline");
    m_pipeline->addMarSystem( resampler );
    m_pipeline->addMarSystem( shredder );
    m_pipeline->addMarSystem( mng.create("Memory", "statBuffer") );
    m_pipeline->addMarSystem( statPipe );

    cout << "*** input blockSize = " << m_inputInfo.blockSize << endl;

    /*resampler->setControl( "mrs_real/israte", m_inputInfo.sampleRate );
    resampler->setControl( "mrs_natural/inSamples", m_inputInfo.blockSize );
    resampler->setControl( "mrs_real/stretch", m_pipeInfo.sampleRate / m_inputInfo.sampleRate );
    resampler->update();*/

    m_pipeline->setControl( "mrs_real/israte", m_inputInfo.sampleRate );
    m_pipeline->setControl( "mrs_natural/inSamples", m_inputInfo.blockSize );
    resampler->setControl( "mrs_real/stretch", m_pipeInfo.sampleRate / m_inputInfo.sampleRate );
    shredder->setControl( "mrs_natural/nTimes", m_pipeInfo.statStepSize );
    shredder->setControl( "mrs_bool/accumulate", true );
    windowedPipe->setControl( "ShiftInput/windower/mrs_natural/winSize", m_pipeInfo.blockSize );
    mfccPipe->setControl( "MFCC/mfcc/mrs_natural/coefficients", m_pipeInfo.mfccCount );
    mfccPipe->setControl( "RemoveObservations/mfccRange/mrs_real/lowCutoff", (float) 2 / (m_pipeInfo.mfccCount-1) );
    mfccPipe->setControl( "RemoveObservations/mfccRange/mrs_real/highCutoff", (float) 4 / (m_pipeInfo.mfccCount-1) );
    m_pipeline->setControl( "Memory/statBuffer/mrs_natural/memSize", m_pipeInfo.statBlockSize / m_pipeInfo.statStepSize );

    m_pipeline->update();

    int resampOutCount = resampler->getControl( "mrs_natural/onSamples" )->to<mrs_natural>();

    cout << "*** resamp stretch = " << resampler->getControl("mrs_real/stretch")->to<mrs_real>() << endl;
    cout << "*** resamp out = " << resampOutCount << endl;

    int outObservationCount = m_pipeline->getControl( "mrs_natural/onObservations" )->to<mrs_natural>();
    int outSampleCount = m_pipeline->getControl( "mrs_natural/onSamples" )->to<mrs_natural>();

    cout << "*** windows in = "
        << windowedPipe->getControl( "mrs_natural/inObservations" )->to<mrs_natural>()
        << "/"
        << windowedPipe->getControl( "mrs_natural/inSamples" )->to<mrs_natural>()
        << endl;
    cout << "*** win fan out = "
        << windowFan->getControl( "mrs_natural/onObservations" )->to<mrs_natural>()
        << "/"
        << windowFan->getControl( "mrs_natural/onSamples" )->to<mrs_natural>()
        << endl;
    cout << "*** output format = [" << outObservationCount << "/" << outSampleCount << "]" << endl;

    m_inputVector.allocate( 1, m_inputInfo.blockSize );
    m_outputVector.allocate( outObservationCount, outSampleCount );
}
#endif

void MarPlugin::createPipeline()
{
    deletePipeline();

    MarSystemManager mng;

    // prototype MarSystems

    MarSystem* protoBuffer = new Marsyas::Buffer("protoBuffer");
    mng.registerPrototype("Buffer", protoBuffer);

    MarSystem* protoVampSink = new Marsyas::VampSink("protoVampSink");
    mng.registerPrototype("VampSink", protoVampSink);

    MarSystem* protoChromaticEntropy = new Marsyas::ChromaticEntropy("protoChromaticEntropy");
    mng.registerPrototype("ChromaticEntropy", protoChromaticEntropy);

    // create pipeline

    VampSink *vampSink = static_cast<VampSink*>( mng.create("VampSink", "vampSink") );
    vampSink->setFeatureSet( &m_featureSet );

    MarSystem *mfccPipe = mng.create("Series", "mfccPipe");
    mfccPipe->addMarSystem( mng.create("MFCC", "mfcc") );
    mfccPipe->addMarSystem( mng.create("RemoveObservations", "mfccRange") );

    MarSystem *spectrumProcFan = mng.create("Fanout", "specProcFan");
    spectrumProcFan->addMarSystem( mng.create("ChromaticEntropy", "entropy") );
    spectrumProcFan->addMarSystem( mfccPipe );

    MarSystem *spectrumProc = mng.create("Cascade", "specProc");
    spectrumProc->addMarSystem( spectrumProcFan );
    spectrumProc->addMarSystem( mng.create("DeltaFirstOrderRegression", "delta") );

    MarSystem *spectrumPipe = mng.create("Series", "spectrumPipe");
    spectrumPipe->addMarSystem( mng.create("Spectrum", "spectrum") );
    spectrumPipe->addMarSystem( mng.create("PowerSpectrum", "powerSpectrum") );
    spectrumPipe->addMarSystem( spectrumProc );

    MarSystem *windowFan = mng.create("Fanout", "windowFan");
    windowFan->addMarSystem( spectrumPipe );
    windowFan->addMarSystem( mng.create("Energy", "energy") );

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

    MarSystem *windowedPipe = mng.create("Series", "windowedPipe");
    windowedPipe->addMarSystem( windowFan );
    windowedPipe->addMarSystem( mng.create("Memory", "statBuffer") );
    windowedPipe->addMarSystem( statPipe );
    windowedPipe->addMarSystem( vampSink );

    MarSystem *winBuffer = mng.create("Buffer", "winBuffer");
    winBuffer->addMarSystem( windowedPipe );

    m_pipeline = mng.create("Series", "pipeline");
    m_pipeline->addMarSystem( mng.create("ResampleLinear", "resampler") );
    m_pipeline->addMarSystem( winBuffer );

    // set controls

    m_pipeline->setControl( "mrs_real/israte", m_inputInfo.sampleRate );
    m_pipeline->setControl( "mrs_natural/inSamples", m_inputInfo.blockSize );

    m_pipeline->setControl( "ResampleLinear/resampler/mrs_real/stretch", m_pipeInfo.sampleRate / m_inputInfo.sampleRate );

    winBuffer->setControl( "mrs_natural/blockSize", m_pipeInfo.blockSize );
    winBuffer->setControl( "mrs_natural/hopSize", m_pipeInfo.stepSize );

    windowedPipe->setControl( "Memory/statBuffer/mrs_natural/memSize", 1 );

    mfccPipe->setControl( "MFCC/mfcc/mrs_natural/coefficients", m_pipeInfo.mfccCount );
    mfccPipe->setControl( "RemoveObservations/mfccRange/mrs_real/lowCutoff", (float) 2 / (m_pipeInfo.mfccCount-1) );
    mfccPipe->setControl( "RemoveObservations/mfccRange/mrs_real/highCutoff", (float) 4 / (m_pipeInfo.mfccCount-1) );

    int featureFrames = std::ceil( m_pipeInfo.stepSize * (m_inputInfo.sampleRate / m_pipeInfo.sampleRate) );
    double featureDistance = (double) featureFrames / m_inputInfo.sampleRate;
    vampSink->setControl( "mrs_real/featureDistance", featureDistance );

    m_pipeline->update();

    int outObservationCount = m_pipeline->getControl( "mrs_natural/onObservations" )->to<mrs_natural>();
    int outSampleCount = m_pipeline->getControl( "mrs_natural/onSamples" )->to<mrs_natural>();

    //std::cout << "*** Marsyas Segmenter: output format = [" << outObservationCount << "/" << outSampleCount << "]" << std::endl;

    m_inputVector.allocate( 1, m_inputInfo.blockSize );
    m_outputVector.allocate( outObservationCount, outSampleCount );
}

void MarPlugin::deletePipeline()
{
    delete(m_pipeline);
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
