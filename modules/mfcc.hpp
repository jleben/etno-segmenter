#ifndef SEGMENTER_MFCC_HPP_INCLUDED
#define SEGMENTER_MFCC_HPP_INCLUDED

//#include "module.hpp"

#include <vector>
#include <cmath>
#include <fftw3.h>

namespace Segmenter {

class Mfcc
{
    int m_windowSize;
    int m_filterCount;

    int * m_melOffsets;
    std::vector<float> * m_melValues;

    fftwf_plan m_plan;
    float *m_dctIn;
    float *m_dctOut;

    float *m_inBuf;

    std::vector<float> m_output;

public:
    Mfcc( int sampleRate, int windowSize, int filterCount = 27 ):
        m_windowSize(windowSize),
        m_filterCount(filterCount)
    {
        m_melOffsets = new int[filterCount];
        m_melValues = new std::vector<float>[filterCount];
        const double fh = 0.5;
        const double fl = 0;
        initMelFilters(filterCount, windowSize, sampleRate, fl, fh, m_melOffsets, m_melValues);

        m_dctIn = fftwf_alloc_real(filterCount);
        m_dctOut = fftwf_alloc_real(filterCount);
        m_plan = fftwf_plan_r2r_1d(filterCount, m_dctIn, m_dctOut,
                                   FFTW_REDFT10, FFTW_ESTIMATE);

        m_inBuf = new float[windowSize / 2 + 1];
        m_output.resize(filterCount - 1);
    }

    ~Mfcc()
    {
        fftwf_free(m_dctIn);
        fftwf_free(m_dctOut);
        fftwf_destroy_plan(m_plan);
        delete[] m_melOffsets;
        delete[] m_melValues;
        delete[] m_inBuf;
    }

    void process ( const std::vector<float> & spectrum )
    {
        static const float ath = 1.0f/65536;
        const int spectrumSize = spectrum.size();

        for (int idx = 0; idx < spectrumSize; ++idx)
            m_inBuf[idx] = std::sqrt( spectrum[idx] );

        for (int filterIdx = 0; filterIdx < m_filterCount; ++filterIdx)
        {
            const std::vector<float> & filter = m_melValues[filterIdx];
            const int offset = m_melOffsets[filterIdx];
            const int filterSize = filter.size();

            float filterOut = 0;

            for (int idx = 0; idx < filterSize; ++idx)
                filterOut += filter[idx] * m_inBuf[offset + idx];

            if (filterOut < ath)
                filterOut = ath;

            filterOut = std::log(filterOut);

            m_dctIn[filterIdx] = filterOut;
        }

        fftwf_execute( m_plan );

        std::memcpy( m_output.data(), m_dctOut + 1, sizeof(float) * (m_filterCount - 1) );
    }

    const std::vector<float> & output() const { return m_output; }

private:

    static void initMelFilters(int p, int n, int fs, double fl, double fh,
                               int * p_offsets, std::vector<float> * p_values)
    {
        double f0 = 700.0 / fs;
        int fn2 = (int) std::floor(n / 2.0);
        double lr = std::log((f0 + fh) / (f0 + fl)) / (p + 1);

        double bl[] = {
            n * ((f0 + fl) - f0),
            n * ((f0 + fl) * std::exp(1 * lr) - f0),
            n * ((f0 + fl) * std::exp(p * lr) - f0),
            n * ((f0 + fl) * std::exp((p + 1) * lr) - f0)
        };

        int b2 = (int) std::ceil(bl[1]);
        int b3 = (int) std::floor(bl[2]);
        int b1 = (int) std::floor(bl[0]) + 1;
        int b4 = (int) std::min( fn2, (int)std::ceil(bl[3]) ) - 1;

        int len = b4 - b1 + 1;
        double * pf = new double[len];
        int * fp = new int[len];
        double * pm = new double[len];
        for (int idx = 0; idx <= b4 - b1; ++idx)
        {
            pf[idx] = std::log((f0 + (double)(idx + b1) / n) / (f0 + fl)) / lr;
            fp[idx] = (int) std::floor(pf[idx]);
            pm[idx] = pf[idx] - fp[idx];
        }

        int k2 = b2 - b1;
        int k3 = b3 - b1;
        int k4 = b4 - b1;

        for (int idx = 0; idx < p; ++idx)
            p_offsets[idx] = -1;

        for (int idx = 0; idx <= k3; ++idx)
        {
            int filt = fp[idx];
            p_offsets[filt] = (p_offsets[filt] == -1 ? idx + b1 : p_offsets[filt]);
            p_values[filt].push_back((float)(2 * pm[idx]));
        }
        for (int idx = k2; idx <= k4; idx++)
        {
            int filt = fp[idx] - 1;
            p_offsets[filt] = (p_offsets[filt] == -1 ? idx + b1 : p_offsets[filt]);
            p_values[filt].push_back((float)(2 * (1 - pm[idx])));
        }

        delete[] pf;
        delete[] fp;
        delete[] pm;
    }
};

} // namespace Segmenter

#endif // SEGMENTER_MFCC_HPP_INCLUDED
