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

#include "vampsink.hpp"

#include <marsyas/common.h>
#include <marsyas/MarControlManager.h>

using std::ostringstream;
using namespace Marsyas;

VampSink::VampSink(mrs_string name) :
    MarSystem("VampSink", name),
    m_featureSet(0),
    m_featureIndex(0),
    m_doTimeStamp(true)
{
    addControls();
}

VampSink::VampSink(const VampSink& a) :
    MarSystem(a),
    m_featureSet( a.m_featureSet ),
    m_featureIndex( a.m_featureIndex ),
    m_doTimeStamp( a.m_doTimeStamp ),
    m_time( a.m_time ),
    m_featureDistance( a.m_featureDistance )
{
}

VampSink::~VampSink()
{
}

MarSystem*
VampSink::clone() const
{
    return new VampSink(*this);
}

void
VampSink::addControls()
{
    /*MarControlManager *mcm = MarControlManager::getManager();
    if (!mcm->isRegistered("vamp_feature_set"))
    {
       mcm->registerPrototype( "vamp_feature_set",
                               new MarControlValueT< Vamp::Plugin::FeatureSet* >() );
    }

    addControl("vamp_feature_set/featureSet", 0);
    setControlState("vamp_feature_set/featureSet", true);*/

    addControl("mrs_natural/featureIndex", m_featureIndex);
    setControlState("mrs_natural/featureIndex", true);

    addControl("mrs_bool/timeStamp", m_doTimeStamp);
    setControlState("mrs_bool/timeStamp", true);

    addControl("mrs_real/featureDistance", 1.0);
    setControlState("mrs_real/featureDistance", true);

    setControl("mrs_natural/onObservations", 0);
    setControl("mrs_natural/onSamples", 0);

    m_featureDistance = Vamp::RealTime::fromSeconds(1.0);
}

void
VampSink::myUpdate(MarControlPtr sender)
{
    setControl("mrs_natural/onObservations", 0);
    setControl("mrs_natural/onSamples", 0);
    setControl("mrs_real/osrate", getControl("mrs_real/israte"));

    //m_featureSet = getControl("vamp_feature_set/featureSet")->to<Vamp::Plugin::FeatureSet*>();

    m_featureIndex = getControl("mrs_natural/featureIndex")->to<mrs_natural>();

    m_featureDistance = Vamp::RealTime::fromSeconds(
        getControl("mrs_real/featureDistance")->to<mrs_real>() );
}


void
VampSink::myProcess(realvec& in, realvec& out)
{
    if (!m_featureSet)
        return;

    for (mrs_natural s = 0; s < inSamples_; ++s)
    {
        Vamp::Plugin::Feature feature;

        feature.hasTimestamp = m_doTimeStamp;
        if (m_doTimeStamp)
            feature.timestamp = m_time;

        for( mrs_natural o = 0; o < inObservations_; ++o )
            feature.values.push_back( in(o, s) );

        (*m_featureSet)[ m_featureIndex ].push_back(feature);

        m_time = m_time + m_featureDistance;
    }
}
