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

#ifndef MARSYAS_LINEAR_TREND_HPP
#define MARSYAS_LINEAR_TREND_HPP

#include <marsyas/MarSystem.h>

namespace Marsyas
{
/**
    \class LinearTrend
    \ingroup Analysis
*/

class marsyas_EXPORT LinearTrend: public MarSystem
{
private:
    void myUpdate(MarControlPtr sender);
    void addControls();
    
    std::vector<float> m_coeff;

public:
    LinearTrend(std::string name);
    LinearTrend(const LinearTrend& a);
    ~LinearTrend();
    MarSystem* clone() const;

    void myProcess(realvec& in, realvec& out);
};

} // namespace Marsyas

#endif // MARSYAS_LINEAR_TREND_HPP
