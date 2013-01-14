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

#include "linear_trend.hpp"

#include <marsyas/common.h>

namespace Marsyas {

LinearTrend::LinearTrend(mrs_string name) :
    MarSystem("LinearTrend", name)
{
    addControls();
}

LinearTrend::LinearTrend(const LinearTrend& a) :
    MarSystem(a),
    m_coeff( a.m_coeff )
{
}


LinearTrend::~LinearTrend()
{
}

MarSystem*
LinearTrend::clone() const
{
    return new LinearTrend(*this);
}

void
LinearTrend::addControls()
{
}

void
LinearTrend::myUpdate(MarControlPtr sender)
{
    MarSystem::myUpdate(sender);

    if (inSamples_ == 1)
    {
        m_coeff.resize(1);
        m_coeff[0] = 1.f;
    }
    else if (inSamples_ > 1)
    {
        m_coeff.resize( inSamples_ );

        float magnitude = (float) (inSamples_ - 1) / 2.f;

        float sum = 0.f;
        for (int i = 0; i < inSamples_; ++i) {
            m_coeff[i] = i - magnitude;
            sum += m_coeff[i] * m_coeff[i];
        }
        for (int i = 0; i < inSamples_; ++i)
            m_coeff[i] = m_coeff[i] / sum;
    }
    else
    {
        m_coeff.clear();
    }

    setControl( "mrs_natural/onSamples", 1 );
}

void
LinearTrend::myProcess(realvec& in, realvec& out)
{
    for (int o = 0; o < inObservations_; ++o)
    {
        mrs_real result = 0;
        for (int t = 0; t < inSamples_; ++t)
            result += in(o,t) * m_coeff[t];
        out(o,0) = result;
    }
}

} // namespace Marsyas
