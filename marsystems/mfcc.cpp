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

#include "mfcc.hpp"

#include <marsyas/common.h>
#include <iostream>
#include <cmath>

using namespace std;

namespace Marsyas {

MyMFCC::MyMFCC(mrs_string name) :
    MarSystem("MyMFCC", name),
    m_filterCount(0),
    m_melOffsets(0),
    m_melCoeff(0),
    m_plan(0),
    m_dctIn(0),
    m_dctOut(0),
    m_win(0),
    m_sr(0)
{
    addControls();
}

MyMFCC::MyMFCC(const MyMFCC& a) :
    MarSystem(a),
    m_filterCount(0),
    m_melOffsets(0),
    m_melCoeff(0),
    m_plan(0),
    m_dctIn(0),
    m_dctOut(0),
    m_win(0),
    m_sr(0)
{
}


MyMFCC::~MyMFCC()
{
    delete[] m_melOffsets;
    delete[] m_melCoeff;
    fftwf_free(m_dctIn);
    fftwf_free(m_dctOut);
    fftwf_destroy_plan(m_plan);
}

MarSystem*
MyMFCC::clone() const
{
    return new MyMFCC(*this);
}

void
MyMFCC::addControls()
{
    addControl("mrs_natural/coefficients", 13);
    setControlState("mrs_natural/coefficients", true);
}

void
MyMFCC::myUpdate(MarControlPtr sender)
{
    static const double fh = 0.5;
    static const double fl = 0;

    mrs_natural filterCount = inObservations_;

    //mrs_natural filterCount = getControl("mrs_natural/coefficients")->to<mrs_natural>();
    //mrs_natural windowSize = 2 * (inObservations_ - 1);
    //mrs_real sampleRate = israte_ * windowSize;

    if (filterCount < 1 ) //|| windowSize < 1)
    {
         // skip unnecessary updates
        m_filterCount = filterCount;
        //m_win = windowSize;
        //m_sr = sampleRate;
        return;
    }

    //cout << "*** MyMFCC: sampleRate = " << sampleRate << endl;
/*
    if (filterCount != m_filterCount || windowSize != m_win || sampleRate != m_sr)
    {
        initMelFilters(filterCount, windowSize, sampleRate, fl, fh);
    }
*/
    if (filterCount != m_filterCount)
    {
        fftwf_free(m_dctIn);
        fftwf_free(m_dctOut);
        fftwf_destroy_plan(m_plan);

        m_dctIn = fftwf_alloc_real(filterCount);
        m_dctOut = fftwf_alloc_real(filterCount);
        m_plan = fftwf_plan_r2r_1d(filterCount, m_dctIn, m_dctOut,
                                    FFTW_REDFT10, FFTW_ESTIMATE);
    }
    /*
    if ( windowSize != m_win )
    {
        m_buf.allocate( inObservations_ );
    }
    */

    setControl("mrs_natural/onObservations", filterCount);
    setControl("mrs_natural/onSamples", inSamples_);

    m_filterCount = filterCount;
    //m_win = windowSize;
    //m_sr = sampleRate;
}

void
MyMFCC::myProcess(realvec& in, realvec& out)
{
    if (m_win < 1 || m_filterCount < 1) {
        out.setval(0);
        return;
    }

    static const double ath = 1.0f/65536;

    for (int t = 0; t < inSamples_; ++t)
    {
        /*
        for (int o = 0; o < inObservations_; ++o)
            m_buf(o) = std::sqrt( in(o,t) );

        for (int filterIdx = 0; filterIdx < m_filterCount; ++filterIdx)
        {
            const std::vector<float> & filter = m_melCoeff[filterIdx];
            const int offset = m_melOffsets[filterIdx];
            const int filterSize = filter.size();

            float filterOut = 0;

            for (int idx = 0; idx < filterSize; ++idx)
                filterOut += filter[idx] * m_buf(offset + idx);

            if (filterOut < ath)
                filterOut = ath;

            filterOut = std::log(filterOut);

            m_dctIn[filterIdx] = filterOut;
        }
        */

        for (int filterIdx = 0; filterIdx < m_filterCount; ++filterIdx)
        {
            m_dctIn[filterIdx] = std::log( std::max(ath, in(filterIdx,t)) );
        }

        fftwf_execute( m_plan );

        for (int filterIdx = 0; filterIdx < m_filterCount; ++filterIdx)
        {
            out(filterIdx, t) = m_dctOut[filterIdx];
        }
    }
}

void MyMFCC::initMelFilters(int p, int n, int fs, double fl, double fh)
{
    delete[] m_melOffsets;
    delete[] m_melCoeff;

    m_melOffsets = new int[p];
    m_melCoeff = new std::vector<float>[p];

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
        m_melOffsets[idx] = -1;

    for (int idx = 0; idx <= k3; ++idx)
    {
        int filt = fp[idx];
        m_melOffsets[filt] = (m_melOffsets[filt] == -1 ? idx + b1 : m_melOffsets[filt]);
        m_melCoeff[filt].push_back((float)(2 * pm[idx]));
    }
    for (int idx = k2; idx <= k4; idx++)
    {
        int filt = fp[idx] - 1;
        m_melOffsets[filt] = (m_melOffsets[filt] == -1 ? idx + b1 : m_melOffsets[filt]);
        m_melCoeff[filt].push_back((float)(2 * (1 - pm[idx])));
    }

    delete[] pf;
    delete[] fp;
    delete[] pm;
}

} // namespace Marsyas
