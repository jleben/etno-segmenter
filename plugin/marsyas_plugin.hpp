#ifndef SEGMENTER_MARSYAS_PLUGIN_INCLUDED
#define SEGMENTER_MARSYAS_PLUGIN_INCLUDED

#include <vamp-sdk/Plugin.h>
#include <marsyas/realvec.h>
#include <marsyas/MarSystem.h>

namespace Marsyas { class VampSink; }

namespace Segmenter {

using Marsyas::realvec;
using Marsyas::MarSystem;

class MarPlugin : public Vamp::Plugin
{

public:
    MarPlugin(float inputSampleRate);
    virtual ~MarPlugin();

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
    void deletePipeline();
    void pipeProcess( const float *input );

    struct {
        float sampleRate;
        int blockSize;
    } m_inputInfo;

    struct {
        float sampleRate;
        int blockSize;
        int stepSize;
        int mfccCount;
        int statBlockSize;
        int statStepSize;
    } m_pipeInfo;

    realvec m_inputVector;
    realvec m_outputVector;

    MarSystem *m_pipeline;
    Marsyas::VampSink *m_vampSink;
    
    FeatureSet m_featureSet;
};

} // namespace Segmenter

#endif // SEGMENTER_MARSYAS_PLUGIN_INCLUDED
