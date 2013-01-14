/*
** Copyright (C) 1998-2010 George Tzanetakis <gtzan@cs.uvic.ca>
** Copyright (C) 2013 Jakob Leben <jakob.leben@gmail.com>
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include "chromaticentropy.hpp"
#include "marsyas/common.h"

#include <iostream>

using namespace std;
using namespace Marsyas;

ChromaticEntropy::ChromaticEntropy(mrs_string name) :
    MarSystem("ChromaticEntropy", name),
    m_loFreq(55),
    m_hiFreq(2200)
{
    addControls();
}

ChromaticEntropy::ChromaticEntropy(const ChromaticEntropy& a) :
    MarSystem(a),
    m_loFreq( a.m_loFreq ),
    m_hiFreq( a.m_hiFreq )
{

}


ChromaticEntropy::~ChromaticEntropy()
{
}

MarSystem*
ChromaticEntropy::clone() const
{
    return new ChromaticEntropy(*this);
}

void
ChromaticEntropy::addControls()
{
    /// Add any specific controls needed by this MarSystem.

    addControl("mrs_natural/lowestFrequency", m_loFreq);
    addControl("mrs_natural/highestFrequency", m_hiFreq);

    setControlState("mrs_natural/lowestFrequency", true);
    setControlState("mrs_natural/highestFrequency", true);
}

void
ChromaticEntropy::myUpdate(MarControlPtr sender)
{
    mrs_natural loFreq = getControl("mrs_natural/lowestFrequency")->to<mrs_natural>();
    mrs_natural hiFreq = getControl("mrs_natural/highestFrequency")->to<mrs_natural>();

    if ( !m_melFilters.size() ||
         m_loFreq != loFreq ||
         m_hiFreq != hiFreq ||
         tisrate_ != israte_ ||
         tinObservations_ != inObservations_ )
    {
        int nTimeSamples = (inObservations_ - 1) * 2;
        float timeSampleRate = israte_ * nTimeSamples;

        initFilters( loFreq, hiFreq, timeSampleRate, inObservations_ );

        m_melSpectrum.resize( m_melFilters.size() );
#if 0
        cout << "*** Mel Filters: ***" << endl;
        for (int i=0; i < m_melFilters.size(); ++i)
        {
            Filter & filter = m_melFilters[i];
            cout << m_melFreqs[i] << " @ " << filter.offset << " :";
            for (int c=0; c < filter.coeffs.size(); ++c)
                cout << " " << filter.coeffs[c];
            cout << endl;
        }
        cout << "*** End Filters ***" << endl;
#endif
    }

    m_loFreq = loFreq;
    m_hiFreq = hiFreq;

    setControl("mrs_natural/onSamples", getControl("mrs_natural/inSamples"));
    setControl("mrs_natural/onObservations", 1);
    setControl("mrs_real/osrate", getControl("mrs_real/israte"));
}

void
ChromaticEntropy::myProcess(realvec& in, realvec& out)
{
    const int melBinCount = m_melFilters.size();

    for (mrs_natural t = 0; t < inSamples_; ++t)
    {
        mrs_real * spectrum = in.getData() + t * inObservations_;

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

        out(0, t) = entropy;
    }
}

void ChromaticEntropy::initFilters( int loFreq, int hiFreq, float sampleRate, int powerBinCount )
{
    //cout << "filter args:" << " " << loFreq << " " << hiFreq << " " << sampleRate << " " << powerBinCount << endl;

    m_melFilters.clear();

    std::vector<float> freqs;
    freqs.reserve(100);
    int maxIndex = (int) std::floor( 12 * std::log( (float)hiFreq / loFreq ) / std::log(2) ) + 1;

    for (int idx = -1; idx <= maxIndex; ++idx)
        freqs.push_back( loFreq * std::pow( 2.0, (double) idx / 12 ) );

    int nFreqs = freqs.size() - 2;
    //int nSpecSize = std::ceil( (windowSize + 1.0) / 2 );
    //int nSpecSize = windowSize / 2 + 1;
    int nSpecSize = powerBinCount;

    float * triangleHeight = new float[nFreqs];
    for (int idx = 0; idx < nFreqs; idx++)
        triangleHeight[idx] = 2.0f / (freqs[idx + 2] - freqs[idx]);

    float * fft_freq = new float[nSpecSize];
    for (int idx = 0; idx < nSpecSize - 1; idx++)
        fft_freq[idx] = idx * sampleRate / 2 / (nSpecSize - 1);
    fft_freq[nSpecSize - 1] = sampleRate / 2;
#if 0
    cout << "orig freqs: ";
    for (int idx = 0; idx < nSpecSize; ++idx)
        cout << fft_freq[idx] << " ";
    cout << endl;
#endif
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
