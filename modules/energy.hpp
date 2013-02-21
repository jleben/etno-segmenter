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

#ifndef SEGMENTER_ENERGY_HPP_INCLUDED
#define SEGMENTER_ENERGY_HPP_INCLUDED

#include "module.hpp"

#include <vector>

namespace Segmenter {

class Energy : public Module
{
    int m_windowSize;
    float m_output;

public:
    Energy( int windowSize ):
        m_windowSize(windowSize),
        m_output(0.f)
    {}

    void process ( const float *samples )
    {
        m_output = 0.f;
        for (int i = 0; i < m_windowSize; ++i)
            m_output += samples[i] * samples[i];
        m_output = m_output / m_windowSize;
    }

    float output() const { return m_output; }
};

} // namespace Segmenter

#endif // SEGMENTER_ENERGY_HPP_INCLUDED
