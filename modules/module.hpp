#ifndef SEGMENTER_MODULE_HPP_INCLUDED
#define SEGMENTER_MODULE_HPP_INCLUDED

namespace Segmenter {

struct ProcessContext {
    ProcessContext(): sampleRate(0), blockSize(0), stepSize(0) {}
    float sampleRate;
    int blockSize;
    int stepSize;
};

class Module
{
public:
    virtual ~Module() {}
};

} // namespace Segmenter

#endif // SEGMENTER_MODULE_HPP_INCLUDED
