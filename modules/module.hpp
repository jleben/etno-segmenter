#ifndef SEGMENTER_MODULE_HPP_INCLUDED
#define SEGMENTER_MODULE_HPP_INCLUDED

#include <cmath>

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

inline double pi() {
    static double s_pi (4.0 * std::atan2(1.0, 1.0));
    return s_pi;
}

} // namespace Segmenter

#endif // SEGMENTER_MODULE_HPP_INCLUDED
