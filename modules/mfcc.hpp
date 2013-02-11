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

    float m_outputScale;

public:
    Mfcc( int coefficientCount )
    {
        m_dctIn = fftwf_alloc_real(coefficientCount);
        m_dctOut = fftwf_alloc_real(coefficientCount);
        m_plan = fftwf_plan_r2r_1d(coefficientCount, m_dctIn, m_dctOut,
//                                   FFTW_R2HC, FFTW_ESTIMATE);
                                   FFTW_REDFT10, FFTW_ESTIMATE);

        m_output.resize(coefficientCount);

        m_outputScale = 1.f / std::sqrt( 2.0f * coefficientCount );
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

        //int cnt=0;
        //for (int idx = 0; idx < coeffCount; idx+=2)
        //{
        //    m_dctIn[cnt++] = std::log( std::max(ath, melSpectrum[idx]) );
        //}
        //for (int idx = 2*(int)floor(coeffCount/2.0f)-1; idx > 0; idx-=2)
        //{
        //    m_dctIn[cnt++] = std::log( std::max(ath, melSpectrum[idx]) );
        //}

        fftwf_execute( m_plan );

//#if SEGMENTER_NEW_FEATURES
//        for (int idx = 0; idx < coeffCount; ++idx)
//            m_output[idx] = m_dctOut[idx] * m_outputScale;
//#else
        m_dctOut[0] /= sqrt(2.0f);
        std::memcpy( m_output.data(), m_dctOut, sizeof(float) * coeffCount );
//#endif
    }

    const std::vector<float> & output() const { return m_output; }
};

} // namespace Segmenter

#endif // SEGMENTER_MFCC_HPP_INCLUDED
