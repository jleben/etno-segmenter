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

#ifndef SEGMENTER_SPECTRAL_ENTROPY_HPP_INCLUDED
#define SEGMENTER_SPECTRAL_ENTROPY_HPP_INCLUDED

#include "module.hpp"

#include <vector>
#include <list>
#include <cmath>

namespace Segmenter {

class ChromaticEntropy : public Module
{
    struct Filter {
        int offset;
        std::vector<float> coeffs;
    };

    std::vector<Filter> m_melFilters;
    std::vector<float> m_melFreqs;
    std::vector<float> m_melSpectrum;
    float m_output;

public:
    ChromaticEntropy( int sampleRate, int windowSize, int loFreq = 55, int hiFreq = 2200 )
    {
        initFilter(loFreq, hiFreq, sampleRate, windowSize);
        m_melSpectrum.resize( m_melFilters.size() );
    }

    void process( const std::vector<float> & spectrum )
    {
        const int melBinCount = m_melFilters.size();

        float sum = 0;
        for (int melBin = 0; melBin < melBinCount; ++melBin)
        {
            const Filter & filter = m_melFilters[melBin];
            const int filterSize = filter.coeffs.size();

            float melPower = 0.f;
            for (int bin = 0; bin < filterSize; ++bin)
                melPower += spectrum[bin + filter.offset] * filter.coeffs[bin];

            sum += melPower;
            m_melSpectrum[melBin] = melPower;
        }

        float entropy = 0;
        if (sum != 0)
        {
            static const double oneOverLog2 = 1.0 / std::log(2.0);

            for (int melBin = 0; melBin < melBinCount; ++melBin)
            {
                float power = m_melSpectrum[melBin];
                power /= sum;
                if (power != 0)
                    entropy += power * std::log(power) * oneOverLog2;
            }
            entropy *= -1;
        }

        m_output = entropy;
    }

    float output() const { return m_output; }

    const std::vector<float> melSpectrum() { return m_melSpectrum; }
    const std::vector<float> melFrequencies() { return m_melFreqs; }
    int melBinCount() { return m_melSpectrum.size(); }

private:
    void initFilter( int loFreq, int hiFreq, int sampleRate, int windowSize )
    {
        std::vector<float> freqs;
        freqs.reserve(100);
        int maxIndex = (int) std::floor( 12 * std::log( (float)hiFreq / loFreq ) / std::log(2) ) + 1;

        for (int idx = -1; idx <= maxIndex; ++idx)
            freqs.push_back( loFreq * std::pow( 2.0, (double) idx / 12 ) );

        int nFreqs = freqs.size() - 2;
        //int nSpecSize = std::ceil( (windowSize + 1.0) / 2 );
        int nSpecSize = windowSize / 2 + 1;

        float * triangleHeight = new float[nFreqs];
        for (int idx = 0; idx < nFreqs; idx++)
            triangleHeight[idx] = 2.0f / (freqs[idx + 2] - freqs[idx]);

        float * fft_freq = new float[nSpecSize];
        for (int idx = 0; idx < nSpecSize - 1; idx++)
            fft_freq[idx] = idx * (float)sampleRate / 2 / (nSpecSize - 1);
        fft_freq[nSpecSize - 1] = (float)sampleRate / 2;

        m_melFilters.reserve(nFreqs);

        for (int i = 0; i < nFreqs; i++)
        {
            Filter filter;
            filter.offset = -1;
            for (int j = 0; j < nSpecSize - 1; j++)
            {
                if (fft_freq[j] > freqs[i] && fft_freq[j] <= freqs[i + 1])
                {
                    filter.offset = (filter.offset == -1 ? j : filter.offset);
                    filter.coeffs.push_back(
                        triangleHeight[i] * (fft_freq[j] - freqs[i]) / (freqs[i + 1] - freqs[i])
                    );
                }
                else if (fft_freq[j] > freqs[i + 1] && fft_freq[j] < freqs[i + 2])
                {
                    filter.offset = (filter.offset == -1 ? j : filter.offset);
                    filter.coeffs.push_back(
                        triangleHeight[i] * (freqs[i + 2] - fft_freq[j]) / (freqs[i + 2] - freqs[i + 1])
                    );
                }
            }
            m_melFilters.push_back(filter);
        }

        m_melFreqs = freqs;

        delete[] triangleHeight;
        delete[] fft_freq;
    }
};

} // namespace Segmenter

#endif // SEGMENTER_SPECTRAL_ENTROPY_HPP_INCLUDED

