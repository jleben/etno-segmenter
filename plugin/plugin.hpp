#ifndef SEGMENTER_PLUGIN_HPP_INCLUDED
#define SEGMENTER_PLUGIN_HPP_INCLUDED

#include <vamp-sdk/Plugin.h>
#include <vector>
#include <fstream>

namespace Segmenter {

class Pipeline;

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
    void createPipeline();

    FeatureSet getFeatures(const float * input, Vamp::RealTime timestamp);

    int m_blockSize;

    Pipeline * m_pipeline;

    Vamp::RealTime m_featureDuration;
    Vamp::RealTime m_featureTime;
};

} // namespace Segmenter

#endif
