#ifndef SEGMENTER_REAL_CEPSTRUM_INCLUDED
#define SEGMENTER_REAL_CEPSTRUM_INCLUDED

#include "module.hpp"

#include <vector>
#include <cmath>
#include <fftw3.h>

namespace Segmenter {

class RealCepstrum : public Module
{
    fftwf_plan m_plan;
    float *m_fft_in;
    float *m_fft_out;

    int m_bufSize;

    std::vector<float> m_output;

public:

    RealCepstrum( int windowSize )
    {
        m_bufSize = (windowSize / 2) + 1;

        m_fft_in = fftwf_alloc_real(m_bufSize);
        m_fft_out = fftwf_alloc_real(m_bufSize);
        m_plan = fftwf_plan_r2r_1d(m_bufSize, m_fft_in, m_fft_out,
                                   FFTW_REDFT10, FFTW_ESTIMATE);

        for (int i = 0; i < m_bufSize; ++i)
            m_fft_in[i] = 0.f;

        m_output.resize(m_bufSize);
    }

    ~RealCepstrum()
    {
        fftwf_free(m_fft_in);
        fftwf_free(m_fft_out);
        fftwf_destroy_plan(m_plan);
    }

    void process ( const std::vector<float> & spectrumMagnitude )
    {
        static const float ath = 1.0f/65536;

        int nSpectrum = spectrumMagnitude.size();

        for (int i = 0; i < nSpectrum; ++i ) {
            float val = std::max( spectrumMagnitude[i], ath );
            val = std::log(val);
            val *= 0.5;
            m_fft_in[i] = val;
        }

        fftwf_execute( m_plan );

        memcpy( m_output.data(), m_fft_out, m_bufSize * sizeof(float) );
    }

    const std::vector<float> & output() const { return m_output; }
};

} // namespace Segmenter

#endif // SEGMENTER_REAL_CEPSTRUM_INCLUDED
