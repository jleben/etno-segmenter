
#ifndef SEGMENTER_4HZ_MODULATION_INCLUDED
#define SEGMENTER_4HZ_MODULATION_INCLUDED

#include "module.hpp"

#include <cmath>

namespace Segmenter {

class FourHzModulation : public Module
{
    std::vector<float> m_filter;

    std::vector< std::vector<float> > m_buf;
    int m_iBufWrite;

    float m_output;

public:
    FourHzModulation( float sampleRate, int windowSize, int hopSize ):
        m_iBufWrite(0),
        m_output(0.f)
    {
        const double pi = Segmenter::pi();

        double dt = hopSize / (double) sampleRate;
        int nFilter = std::ceil( 0.5 / dt );
        m_filter.reserve(nFilter);
        for (int i = 0; i < nFilter; ++i)
            m_filter.push_back( cos( 4 * 2 * pi * i * dt ) );

        int spectrumSize = windowSize / 2 + 1;
        m_buf.resize( nFilter );
        for (int i = 0; i < nFilter; ++i)
            m_buf[i].resize(spectrumSize);
    }

    void process( const std::vector<float> & melSpectrum )
    {
        int nSpectrum = melSpectrum.size();
        int nFilter = m_filter.size();

        std::memcpy( m_buf[m_iBufWrite].data(), melSpectrum.data(), nSpectrum * sizeof(float) );
        ++m_iBufWrite;
        if (m_iBufWrite >= nFilter)
            m_iBufWrite = 0;

        float filteredSpectrumEnergy = 0.f;

        for (int iSpec = 0; iSpec < nSpectrum; ++iSpec)
        {
            float filteredBin = 0.f;
            float totalBinEnergy = 0.f;
            // process 2 parts of the circular buffer:
            int iFilter, iBuf;
            for( iBuf = m_iBufWrite, iFilter = 0; iBuf < nFilter; ++iBuf, ++iFilter )
            {
                float x = m_buf[iBuf][iSpec];
                filteredBin += x * m_filter[iFilter];
                totalBinEnergy += x;
            }
            for( iBuf = 0; iBuf < m_iBufWrite; ++iBuf, ++iFilter )
            {
                float x = m_buf[iBuf][iSpec];
                filteredBin += x * m_filter[iFilter];
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
