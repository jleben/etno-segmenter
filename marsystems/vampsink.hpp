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

#ifndef MARSYAS_VAMP_SINK_HPP
#define MARSYAS_VAMP_SINK_HPP

#include <marsyas/MarSystem.h>
#include <vamp-sdk/Plugin.h>

namespace Marsyas
{
/**
    \class VampSink
    \ingroup Input/Output
    \brief Output to a Vamp::Plugin::FeatureSet

    VampSink outputs its input as Vamp plugin output data.

    Each sample becomes a Vamp::Plugin::Feature, and each observation becomes a value of
    that feature. The features are stored into the Vamp::Plugin::FeatureSet given with
    setFeatureSet() method.

    Controls:
    - \b mrs_natural/featureIndex [rw] : output index within the feature set
    - \b mrs_bool/timeStamp [rw] : whether to timestamp the output features
    - \b mrs_real/featureDistance [rw] : the distance between output features, in seconds
*/

class marsyas_EXPORT VampSink: public MarSystem
{
private:
    void addControls();
    void myUpdate(MarControlPtr sender);

    Vamp::Plugin::FeatureSet *m_featureSet;
    mrs_natural m_featureIndex;
    bool m_doTimeStamp;
    Vamp::RealTime m_featureDistance;
    Vamp::RealTime m_time;

public:
    VampSink(std::string name);
    VampSink(const VampSink& a);
    ~VampSink();
    MarSystem* clone() const;

    void setFeatureSet( Vamp::Plugin::FeatureSet * featureSet ) { m_featureSet = featureSet; }

    void myProcess(realvec& in, realvec& out);
};

} // namespace Marsyas

#endif // MARSYAS_VAMP_SINK_HPP

