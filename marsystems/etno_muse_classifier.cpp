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

#include "etno_muse_classifier.hpp"

#include <marsyas/common.h>

namespace Marsyas {

const float EtnoMuseClassifier::s_coeffs
    [EtnoMuseClassifier::s_inputCount+1]
    [EtnoMuseClassifier::s_classCount-1]
    =
    {
        {-2.54360f,   1.20630f,   1.25340f,   1.02670f},
        {0.01410f,   0.01860f,   0.00570f,   0.02100f},
        {0.03250f,   0.04000f,  -0.05880f,  -0.07790f},
        {-0.00070f,   0.00770f,   0.00980f,  -0.03070f},
        {-45.79580f, -17.22620f, -57.71280f,  85.87760f},
        {-0.16760f,  -0.27780f,  -1.14750f,  -0.71080f},
        {-0.24500f,  -0.61390f,  -2.13820f,   0.05400f},
        {-0.28430f,  -0.60280f,  -3.08030f,  -0.43420f},
        {-0.40990f,  -1.52120f,   -1.16710f, -0.98170f},
        {9.72240f, -0.61690f,     10.70270f,   2.26640f}
    };

std::string EtnoMuseClassifier::s_classNames[] =
    { "solo", "choir", "bell", "instrumental", "speech" };

EtnoMuseClassifier::EtnoMuseClassifier(mrs_string name) :
    MarSystem("EtnoMuseClassifier", name)
{
    addControls();
}

EtnoMuseClassifier::EtnoMuseClassifier(const EtnoMuseClassifier& a) :
    MarSystem(a)
{
}

EtnoMuseClassifier::~EtnoMuseClassifier()
{
}

MarSystem*
EtnoMuseClassifier::clone() const
{
    return new EtnoMuseClassifier(*this);
}

void
EtnoMuseClassifier::addControls()
{
}

void
EtnoMuseClassifier::myUpdate(MarControlPtr sender)
{
    setControl("mrs_natural/onObservations", s_classCount);
    setControl("mrs_natural/onSamples", 1);
    setControl("mrs_real/osrate", getControl("mrs_real/israte"));

    if (inObservations_ != s_inputCount) {
        MRSWARN("EtnoMuseClassifier: number of input observations (" << inObservations_ << ")"
                << " does not match required number (" << s_inputCount << ")");
    }
}

static float variance( realvec& row )
{
    int n = row.getSize();

    if (n < 2)
        return 0.f;

    mrs_real *d = row.getData();
    mrs_real mean = row.mean();
    float var = 0.f;
    for (int i = 0; i < n; ++i) {
        float dev = d[i] - mean;
        var += dev * dev;
    }
    var /= n - 1;
    return var;
}

void
EtnoMuseClassifier::myProcess(realvec& in, realvec& out)
{
    if (inObservations_ != s_inputCount)
        return;

        /*
    ENTROPY_MEAN,
    MFCC2_VAR,
    MFCC3_VAR,
    MFCC4_VAR,
    ENTROPY_DELTA_VAR,
    MFCC2_DELTA_VAR,
    MFCC3_DELTA_VAR,
    MFCC4_DELTA_VAR,
    ENERGY_FLUX,
    */

    // Compute statistical features

    realvec row;
    mrs_real features[s_inputCount];

    in.getRow(0, row);
    features[0] = row.mean();

    in.getRow(1, row);
    features[1] = variance(row);

    in.getRow(2, row);
    features[2] = variance(row);

    in.getRow(3, row);
    features[3] = variance(row);

    in.getRow(4, row);
    features[4] = variance(row);

    in.getRow(5, row);
    features[5] = variance(row);

    in.getRow(6, row);
    features[6] = variance(row);

    in.getRow(7, row);
    features[7] = variance(row);

    in.getRow(8, row);
    mrs_real var8 = variance(row);
    mrs_real mean8 = row.mean();
    features[8] = mean8 != 0 ? var8 / (mean8 * mean8) : 0;

    // Compute class probability distribution

    float p[s_classCount];
    float sum = 1;
    for (int nJ = 0; nJ < s_classCount - 1; nJ++)
    {
        p[nJ] = 0;
        int nK;
        for (nK = 0; nK < s_inputCount; nK++)
            p[nJ] += s_coeffs[nK][nJ] * features[nK];
        p[nJ] = std::exp( p[nJ] + s_coeffs[nK][nJ] );
        sum += p[nJ];
    }
    for (int nJ = 0; nJ < s_classCount - 1; nJ++)
        p[nJ] /= sum;
    p[s_classCount - 1] = 1/sum;
    
    for (int o = 0; o < s_classCount; ++o)
        out(o, 0) = p[o];
#if 0
    // Get the "mean" class index
    float meanClass = 0;
    for (int i = 0; i < s_classCount; ++i)
        meanClass += p[i] * i;
    meanClass /= s_classCount - 1;
    
    out(0,0) = meanClass;
#endif
}

} // namespace Marsyas
