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

#ifndef SEGMENTER_ENERGY_GATE_INCLUDED
#define SEGMENTER_ENERGY_GATE_INCLUDED

#include "module.hpp"

#include <vector>
#include <cmath>

namespace Segmenter {

class Biquad
{
private:
    double b0;
    double b1;
    double b2;
    double a1;
    double a2;
    double s1;
    double s2;

public:
    Biquad(double b0, double b1, double b2, double a1, double a2):
        b0(b0), b1(b1), b2(b2), a1(a1), a2(a2), s1(0.0), s2(0.0)
    {}

    double process( double x )
    {
        // Transposed Direct Form II
        double y = b0 * x + s1;
        s1 = b1 * x - a1 * y + s2;
        s2 = b2 * x - a2 * y;
        return y;
    }
};

class EnergyGate : public Module
{
private:
    Biquad m_filter;
    double m_absoluteThreshold;
    double m_relativeThreshold; // (energy / filtered energy)
    float m_output;

public:
    EnergyGate( double absoluteThresholdDeciBels, double relativeThresholdDeciBels ):
        // coefficients produced by MATLAB: butter(2, 0.015);
        m_filter(5.3717e-04, 1.0743e-03, 5.3717e-04, -1.93338, 0.93553),
        m_absoluteThreshold( std::pow(10.0, absoluteThresholdDeciBels / 10.0) ),
        m_relativeThreshold( std::pow(10.0, relativeThresholdDeciBels / 10.0) ),
        m_output(0.f)
    {}

    void process( float energy )
    {
        double energyFiltered = m_filter.process( std::sqrt((double)energy) );
        energyFiltered *= energyFiltered;

        bool pass = energy >= m_absoluteThreshold && energy >= energyFiltered * m_relativeThreshold;

        m_output = pass ? 1.f : 0.f;
    }

    float output() { return m_output; }
};

} // namespace Segmenter

#endif // SEGMENTER_ENERGY_GATE_INCLUDED
