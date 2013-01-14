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

#ifndef MARSYAS_ETNO_MUSE_CLASSIFIER_HPP
#define MARSYAS_ETNO_MUSE_CLASSIFIER_HPP

#include "marsyas/MarSystem.h"

#include <string>

namespace Marsyas
{
/**
    \class EtnoMuseClassifier
*/

class marsyas_EXPORT EtnoMuseClassifier: public MarSystem
{
private:
    void myUpdate(MarControlPtr sender);
    void addControls();

    static const int s_classCount = 5;
    static const int s_inputCount = 9;
    static const float s_coeffs[s_inputCount+1][s_classCount-1];
    static std::string s_classNames[];

public:
    EtnoMuseClassifier(std::string name);
    EtnoMuseClassifier(const EtnoMuseClassifier& a);
    ~EtnoMuseClassifier();
    MarSystem* clone() const;

    void myProcess(realvec& in, realvec& out);
};

} // namespace Marsyas

#endif // MARSYAS_ETNO_MUSE_CLASSIFIER_HPP
