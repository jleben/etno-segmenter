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

#ifndef SEGMENTER_MEL_SPECTRUM_INCLUDED
#define SEGMENTER_MEL_SPECTRUM_INCLUDED

#include "module.hpp"

#include <vector>
#include <cmath>

namespace Segmenter {

class MelSpectrum : public Module
{
    struct Filter {
        int offset;
        std::vector<float> coeff;
    };

    std::vector<Filter> m_melFilterBank;
    std::vector<float> m_input;
    std::vector<float> m_output;

public:
    MelSpectrum( int coefficientCount, float sampleRate, int windowSize )
    {
        //const double fh = 0.5;
        const double fh = 11025*0.5/sampleRate;
        const double fl = 0;
        initMelFilters(coefficientCount, windowSize, sampleRate, fl, fh, m_melFilterBank);

        m_input.resize(windowSize / 2 + 1);
        m_output.resize(coefficientCount);
    }

    void process( const std::vector<float> & spectrumMagnitude )
    {
        const int filterBankSize = m_melFilterBank.size();

        for (int filterIdx = 0; filterIdx < filterBankSize; ++filterIdx)
        {
            const Filter & filter = m_melFilterBank[filterIdx];
            const int coeffCount = filter.coeff.size();

            float filterOut = 0;

            for (int idx = 0; idx < coeffCount; ++idx)
                filterOut += filter.coeff[idx] * spectrumMagnitude[filter.offset + idx];

            m_output[filterIdx] = filterOut;
        }
    }

    const std::vector<float> & output() const { return m_output; }

private:
    static void initMelFilters(int p, int n, int fs, double fl, double fh,
                               std::vector<Filter> & filterBank)
                               //int * p_offsets, std::vector<float> * p_values)
    {
        filterBank.resize(p);

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

        for (int idx = 0; idx < p; ++idx) {
            filterBank[idx].offset = -1;
            filterBank[idx].coeff.clear();
        }

        for (int idx = 0; idx <= k3; ++idx)
        {
            Filter & filt = filterBank[ fp[idx] ];
            filt.offset = (filt.offset == -1 ? idx + b1 : filt.offset);
            filt.coeff.push_back((float)(2 * pm[idx]));
        }
        for (int idx = k2; idx <= k4; idx++)
        {
            Filter & filt = filterBank[ fp[idx] - 1 ];
            filt.offset = (filt.offset == -1 ? idx + b1 : filt.offset);
            filt.coeff.push_back((float)(2 * (1 - pm[idx])));
        }

        delete[] pf;
        delete[] fp;
        delete[] pm;
    }
};

} // namespace Segmenter

#endif // SEGMENTER_MEL_SPECTRUM_INCLUDED
