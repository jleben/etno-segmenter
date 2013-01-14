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

#ifndef MARSYAS_MY_MFCC_HPP
#define MARSYAS_MY_MFCC_HPP

#include <marsyas/MarSystem.h>
#include <vector>
#include <fftw3.h>

namespace Marsyas
{
/**
    \class MyMFCC
    \ingroup Analysis
*/

class marsyas_EXPORT MyMFCC: public MarSystem
{
private:
    void addControls();
    void myUpdate(MarControlPtr sender);
    void initMelFilters(int p, int n, int fs, double fl, double fh);


    int m_filterCount;

    int * m_melOffsets;
    std::vector<float> * m_melCoeff;

    fftwf_plan m_plan;
    float *m_dctIn;
    float *m_dctOut;

    realvec m_buf;
    
    // cache
    mrs_natural m_win;
    mrs_natural m_sr;
    
public:
    MyMFCC(std::string name);
    MyMFCC(const MyMFCC& a);
    ~MyMFCC();
    MarSystem* clone() const;

    void myProcess(realvec& in, realvec& out);
};

} // namespace Marsyas

#endif // MARSYAS_MY_MFCC_HPP
