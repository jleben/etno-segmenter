#ifndef SEGMENTER_PLUGIN_HPP_INCLUDED
#define SEGMENTER_PLUGIN_HPP_INCLUDED

#include "../modules/module.hpp"

#include <vamp-sdk/Plugin.h>
#include <vector>

namespace Segmenter {

class Resampler;
class PowerSpectrum;
class Mfcc;
class ChromaticEntropy;
class Statistics;
class Energy;
class Classifier;

class Plugin : public Vamp::Plugin
{
public:
    Plugin(float inputSampleRate);
    virtual ~Plugin();

    bool initialise(size_t channels, size_t stepSize, size_t blockSize);
    void reset();

    std::string getIdentifier() const;
    std::string getName() const;
    std::string getDescription() const;
    std::string getMaker() const;
    std::string getCopyright() const;
    int getPluginVersion() const;

    //ParameterList getParameterDescriptors() const;

    InputDomain getInputDomain() const;
    size_t getMinChannelCount() const;
    size_t getMaxChannelCount() const;
    size_t getPreferredBlockSize() const;
    size_t getPreferredStepSize() const;

    OutputList getOutputDescriptors() const;

    //float getParameter(std::string) const;
    //void setParameter(std::string, float);

    FeatureSet process(const float *const *inputBuffers,
                       Vamp::RealTime timestamp);

    FeatureSet getRemainingFeatures();

private:
    struct InputContext {
        InputContext(): sampleRate(0), blockSize(0) {}
        float sampleRate;
        int blockSize;
    } m_inputContext;

    ProcessContext m_procContext;

    void deleteModules();

    Resampler * m_resampler;
    Energy * m_energy;
    PowerSpectrum * m_spectrum;
    Mfcc * m_mfcc;
    ChromaticEntropy * m_entropy;
    Statistics *m_statistics;
    Classifier *m_classifier;

    int m_statBlockSize;
    int m_statStepSize;
    Vamp::RealTime m_statsStepDuration;
    Vamp::RealTime m_statsTime;

    std::vector<float> m_resampBuffer;

    void initStatistics()
    {
        m_statsStepDuration = Vamp::RealTime::fromSeconds
            ( (double) m_statStepSize * m_procContext.stepSize / m_procContext.sampleRate );
        m_statsTime = Vamp::RealTime::fromSeconds
            ( ((double) m_statBlockSize / 2.0) * m_procContext.stepSize / m_procContext.sampleRate );
    }
};

} // namespace Segmenter

#endif
