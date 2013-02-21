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

#ifndef SEGMENTER_CLASSIFICATION_HPP_INCLUDED
#define SEGMENTER_CLASSIFICATION_HPP_INCLUDED

#include "module.hpp"

#include <vector>
#include <string>
#include <cmath>
#include <iostream>

namespace Segmenter {

class Classifier : public Module
{
    static const int s_classCount = 5;

#if SEGMENTER_NEW_FEATURES
    static const int s_inputCount = 16;
#else
    static const int s_inputCount = 9;
#endif

    static const float s_coeffs[s_inputCount+1][s_classCount-1];

    typedef float InputArray [s_inputCount];

    std::vector<float> m_output;
    std::vector< std::string > m_classNames;

public:
    Classifier()
    {
        m_output.resize(s_classCount);
        std::string classNames[] = { "solo", "choir", "bell", "instrumental", "speech" };
        m_classNames.insert( m_classNames.end(), &classNames[0], &classNames[5] );

#if 0
        std::cout << "coeffs:" << std::endl;
        for (int i = 0; i < s_inputCount+1; ++i) {
            for(int j = 0; j < s_classCount-1; ++j) {
                std::cout << s_coeffs[i][j] << " ";
            }
            std::cout << std::endl;
        }
#endif
    }

    void process( const InputArray & input )
    {
#if 0
        std::cout << "stats: ";
        for (int i = 0; i < s_inputCount; ++i)
            std::cout << input[i] << " ";
        std::cout << std::endl;
#endif
        float *t = m_output.data();
        float sum = 1;
        for (int nJ = 0; nJ < s_classCount - 1; nJ++)
        {
            t[nJ] = 0;
            int nK;
            for (nK = 0; nK < s_inputCount; nK++)
                t[nJ] += s_coeffs[nK][nJ] * input[nK];
            t[nJ] = std::exp( t[nJ] + s_coeffs[nK][nJ] );
            sum += t[nJ];
        }
#if 0
        std::cout << "classes: ";
        for (int i = 0; i < s_classCount - 1; ++i)
            std::cout << t[i] << " ";
        std::cout << std::endl;
#endif
        for (int nJ = 0; nJ < s_classCount - 1; nJ++)
            t[nJ] /= sum;
        t[s_classCount - 1] = 1/sum;
    }

    const std::vector< std::string > & classNames() { return m_classNames; }

    const std::vector<float> & probabilities() { return m_output; }
};

} // namespace Segmenter

#endif // SEGMENTER_CLASSIFICATION_HPP_INCLUDED
