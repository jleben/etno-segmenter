#ifndef SEGMENTER_MFCC_HPP_INCLUDED
#define SEGMENTER_MFCC_HPP_INCLUDED

#include "module.hpp"

#include <vector>
#include <cmath>
#include <fftw3.h>

namespace Segmenter {

class Mfcc : public Module
{
    fftwf_plan m_plan;
    float *m_dctIn;
    float *m_dctOut;

    std::vector<float> m_output;

public:
    Mfcc( int coefficientCount )
    {
        m_dctIn = fftwf_alloc_real(coefficientCount);
        m_dctOut = fftwf_alloc_real(coefficientCount);
        m_plan = fftwf_plan_r2r_1d(coefficientCount, m_dctIn, m_dctOut,
                                   FFTW_REDFT10, FFTW_ESTIMATE);

        m_output.resize(coefficientCount);
    }

    ~Mfcc()
    {
        fftwf_free(m_dctIn);
        fftwf_free(m_dctOut);
        fftwf_destroy_plan(m_plan);
    }

    void process ( const std::vector<float> & melSpectrum )
    {
        static const float ath = 1.0f/65536;
        const int coeffCount = melSpectrum.size();

        for (int idx = 0; idx < coeffCount; ++idx)
        {
            m_dctIn[idx] = std::log( std::max(ath, melSpectrum[idx]) );
        }

        fftwf_execute( m_plan );

        std::memcpy( m_output.data(), m_dctOut, sizeof(float) * coeffCount );
    }

    const std::vector<float> & output() const { return m_output; }
};

} // namespace Segmenter

#endif // SEGMENTER_MFCC_HPP_INCLUDED
