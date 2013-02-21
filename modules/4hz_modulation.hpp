/*
    Etno Segmenter - automatic segmentation of etnomusicological recordings

    Copyright (c) 2012 - 2013 Matija Marolt & Jakob Leben

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software Foundation,
    Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/

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
