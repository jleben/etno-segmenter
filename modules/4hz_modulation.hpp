
#ifndef SEGMENTER_4HZ_MODULATION_INCLUDED
#define SEGMENTER_4HZ_MODULATION_INCLUDED

#include <cmath>

namespace Segmenter {

class FourHzModulation
{
    std::vector<float> m_filter;

    std::vector< std::vector<float> > m_buf;
    int m_buf_n;

    float m_output;

public:
    FourHzModulation( float sampleRate, int hopSize ):
        m_buf_n(0),
        m_output(0.f)
    {
        double dt = hopSize / (double) sampleRate;
        int filterN = std::ceil( 0.5 / dt );
        m_filter.reserve(filterN);
        for (int n = 0; n < filterN; ++n)
            m_filter.push_back( cos( 4 * 2 * PI * dt ) );

        m_buf.resize( filterN );
    }

    void process( const std::vector<float> & melSpectrum )
    {
        m_buf[m_buf_n] = melSpectrum;
        ++m_buf_n;
        int spectrum_N = melSpectrum.size();
        int filter_N = m_filter.size();

        float filteredSpectrumEnergy = 0.f;

        for (int spec_n = 0; spec_n < spectrum_N; ++spec_n)
        {
            float filteredBin = 0.f;
            float totalBinEnergy = 0.f;
            // process 2 parts of the circular buffer:
            int n_f, n_b;
            for( n_b = m_buf_n, n_f = 0; n_b < N; ++n_b, ++n_f )
            {
                float x = m_buf_n[n][spec_n];
                filteredBin += x * m_filter[n_f];
                totalBinEnergy += x;
            }
            for( n_b = 0; n_b < m_buf_n; ++n_b, ++n_f )
            {
                float x = m_buf_n[n][spec_n];
                filteredBin += x * m_filter[n_f];
                totalBinEnergy += x;
            }

            float filteredBinEnergy = std::abs(filteredBin) / totalBinEnergy;
            filteredSpectrumEnergy += filteredBinEnergy;
        }

        m_output = filteredSpectrumEnergy;
    }

    float output() const { return m_output; }
};

} // namespace Segmenter

#endif // SEGMENTER_4HZ_MODULATION_INCLUDED
