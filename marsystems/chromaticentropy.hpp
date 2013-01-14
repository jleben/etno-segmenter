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

#ifndef MARSYAS_CHROMATIC_ENTROPY_HPP
#define MARSYAS_CHROMATIC_ENTROPY_HPP

#include "marsyas/MarSystem.h"

namespace Marsyas
{
/**
    \class ChromaticEntropy
    \ingroup Processing
    \brief Entropy of power spectrum warped to chromatic scale

    Controls:

*/

class marsyas_EXPORT ChromaticEntropy: public MarSystem
{
private:
    void myUpdate(MarControlPtr sender);
    void addControls();
    void initFilters( int loFreq, int hiFreq, float sampleRate, int powerBinCount );

    struct Filter {
        int offset;
        std::vector<mrs_real> coeffs;
    };

    mrs_natural m_loFreq;
    mrs_natural m_hiFreq;

    std::vector<Filter> m_melFilters;
    std::vector<float> m_melFreqs;
    std::vector<float> m_melSpectrum;

public:
    ChromaticEntropy(std::string name);
    ChromaticEntropy(const ChromaticEntropy& a);
    ~ChromaticEntropy();
    MarSystem* clone() const;

    void myProcess(realvec& in, realvec& out);
};

} // namespace Marsyas

#endif // MARSYAS_CHROMATIC_ENTROPY_HPP
