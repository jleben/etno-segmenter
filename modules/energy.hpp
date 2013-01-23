#ifndef SEGMENTER_ENERGY_HPP_INCLUDED
#define SEGMENTER_ENERGY_HPP_INCLUDED

#include "module.hpp"

#include <vector>

namespace Segmenter {

class Energy : public Module
{
    int m_windowSize;
    float m_output;

public:
    Energy( int windowSize ):
        m_windowSize(windowSize),
        m_output(0.f)
    {}

    void process ( const float *samples )
    {
        m_output = 0.f;
        for (int i = 0; i < m_windowSize; ++i)
            m_output += samples[i] * samples[i];
        m_output = m_output / m_windowSize;
    }

    float output() const { return m_output; }
};

} // namespace Segmenter

#endif // SEGMENTER_ENERGY_HPP_INCLUDED
