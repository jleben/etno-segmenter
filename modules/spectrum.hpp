#ifndef SEGMENTER_FFT_HPP_INCLUDED
#define SEGMENTER_FFT_HPP_INCLUDED

#include "module.hpp"

#include <vector>
#include <cmath>
#include <cstring>
#include <fftw3.h>

#define POWER_SPECTRUM_SCALING SEGMENTER_NEW_FEATURES

namespace Segmenter {

class PowerSpectrum : public Module
{
    int m_windowSize;
    fftwf_plan m_plan;
    float *m_inBuffer;
    float *m_outBuffer;
    float *m_window;
    std::vector<float> m_output;
    float m_outputScale;

public:
    PowerSpectrum( int windowSize ):
        m_windowSize(windowSize)
    {
        m_inBuffer = fftwf_alloc_real(windowSize);
        m_outBuffer = fftwf_alloc_real(windowSize);
        m_plan = fftwf_plan_r2r_1d(windowSize, m_inBuffer, m_outBuffer,
                                   FFTW_R2HC, FFTW_ESTIMATE);

        double pi = Segmenter::pi();

        m_window = new float[windowSize];

        float sumWindow = 0.f;
        for (int idx=0; idx < windowSize; ++idx) {
            m_window[idx] = 0.54 - 0.46 * std::cos(2 * pi * idx / (windowSize-1) );
            sumWindow += m_window[idx];
        }

        m_outputScale = 2.f / sumWindow;
        m_outputScale *= m_outputScale; // square, because we'll be multiplying power instead of raw spectrum

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

        float r, i; // real and imaginary parts

        r = fft[0];
        out[0] = r * r;
#if POWER_SPECTRUM_SCALING
        out[0] *= m_outputScale;
#endif

        const int pairCount = (winSize + 1) / 2;
        for (int idx = 1; idx < pairCount; ++idx)
        {
            r = fft[idx];
            i = fft[winSize - idx];
            out[idx] = r * r + i * i;
#if POWER_SPECTRUM_SCALING
            out[idx] *= m_outputScale;
#endif
        }

        if (winSize % 2 == 0)
        {
            const int lastIdx = winSize / 2;
            r = fft[lastIdx];
            out[lastIdx] = r * r;
#if POWER_SPECTRUM_SCALING
            out[lastIdx] *= m_outputScale;
#endif
        }
    }

    const std::vector<float> & output() const { return m_output; }
};

} // namespace Segmenter

#endif // SEGMENTER_FFT_HPP_INCLUDED
