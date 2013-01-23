#ifndef SEGMENTER_FFT_HPP_INCLUDED
#define SEGMENTER_FFT_HPP_INCLUDED

#include "module.hpp"

#include <vector>
#include <cmath>
#include <cstring>
#include <fftw3.h>

namespace Segmenter {

class PowerSpectrum : public Module
{
    int m_windowSize;
    fftwf_plan m_plan;
    float *m_inBuffer;
    float *m_outBuffer;
    float *m_window;
    std::vector<float> m_output;

public:
    PowerSpectrum( int windowSize ):
        m_windowSize(windowSize)
    {
        m_inBuffer = fftwf_alloc_real(windowSize);
        m_outBuffer = fftwf_alloc_real(windowSize);
        m_plan = fftwf_plan_r2r_1d(windowSize, m_inBuffer, m_outBuffer,
                                   FFTW_R2HC, FFTW_ESTIMATE);

        static const double PI (4.0 * std::atan2(1.0, 1.0));

        m_window = new float[windowSize];
        for (int idx=0; idx < windowSize; ++idx)
            m_window[idx] = 0.54 - 0.46 * std::cos(2 * PI * idx / (windowSize-1) );

        m_output.resize(windowSize / 2 + 1);
    }

    ~PowerSpectrum()
    {
        fftwf_free(m_inBuffer);
        fftwf_free(m_outBuffer);
        fftwf_destroy_plan(m_plan);
    }

    void process ( const float *input )
    {
        for (int idx = 0; idx < m_windowSize; ++idx)
            m_inBuffer[idx] = input[idx] * m_window[idx];

        fftwf_execute( m_plan );

        float * out = m_output.data();
        float * fft = m_outBuffer;
        const int winSize = m_windowSize;

        out[0] = fft[0] * fft[0];

        const int pairCount = (winSize + 1) / 2;
        for (int idx = 1; idx < pairCount; ++idx)
        {
            out[idx] = fft[idx] * fft[idx] + fft[winSize - idx] * fft[winSize - idx];
        }

        if (winSize % 2 == 0)
        {
            const int lastIdx = winSize / 2;
            out[lastIdx] = fft[lastIdx] * fft[lastIdx];
        }
    }

    const std::vector<float> & output() const { return m_output; }
};

} // namespace Segmenter

#endif // SEGMENTER_FFT_HPP_INCLUDED
