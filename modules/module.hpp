#ifndef SEGMENTER_MODULE_HPP_INCLUDED
#define SEGMENTER_MODULE_HPP_INCLUDED

namespace Segmenter {

struct ProcessContext {
    ProcessContext(): sampleRate(0), blockSize(0), stepSize(0) {}
    float sampleRate;
    int blockSize;
    int stepSize;
};

/*class Module
{
protected:
    const int m_sampleRate;
    const int m_channels;

public:
    Module( int sampleRate, int channels ):
        m_sampleRate(sampleRate),
        m_channels(channels)
    {}

    int sampleRate() const { return m_sampleRate; }
    int channels() const { return m_channels; }
};*/

} // namespace Segmenter

#endif // SEGMENTER_MODULE_HPP_INCLUDED
