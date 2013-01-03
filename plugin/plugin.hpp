#ifndef SEGMENTER_PLUGIN_HPP_INCLUDED
#define SEGMENTER_PLUGIN_HPP_INCLUDED

#include <vamp-sdk/Plugin.h>

namespace Segmenter {

class Resampler;
class PowerSpectrum;
class Mfcc;
class ChromaticEntropy;

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
    float m_sampleRate;
    size_t m_blockSize;
    size_t m_stepSize;

    Resampler * m_resampler;
    PowerSpectrum * m_spectrum;
    Mfcc * m_mfcc;
    ChromaticEntropy * m_entropy;
};

} // namespace Segmenter

#endif
