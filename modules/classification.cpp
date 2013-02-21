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

#include "classification.hpp"

namespace Segmenter {

const float Classifier::s_coeffs[Classifier::s_inputCount+1][Classifier::s_classCount-1] =
#if SEGMENTER_NEW_FEATURES
{
    { 7.2167803598f, 6.7546674784f,  2.4266379892f, -2.4717760029f },
    { -3.5872281700f, 0.6072347911f, 1.8414028714f, 2.2034845670f },
    { -3.6282955710f, 4.0023822690f, 24.5460436473f, 9.8999564256f },
    { 1.5852707243f, 0.8001852249f, -3.0620514025f, 0.6355285476f },
    { 13.4272833985f, -6.5855565616f, -15.3262530286f, -5.7903571674f },
    { -1.1057892874f, -2.1950890900f, 1.5537406267f, 1.5332794847f },
    { -0.0938808825f, -0.1271246065f, -0.1365147948f, -0.0805778048f },
    { 0.0329521240f, -0.0907322511f, -0.0545751082f, 0.0301488257f },
    { -0.0127837404f, -0.1146084664f, -0.0472986272f, 0.0686640993f },
    { -48.8447183118f, -43.3820036434f, -118.4792918921f, 19.6619504215f },
    { 1.1102278505f, -5.9568952783f, -8.1252887526f, -5.7130277204f },
    { -0.0271290595f, 0.0941640111f, 0.3104335501f, 0.0046523263f },
    { 0.2328909073f, 0.3577255579f, -0.4705257276f, -0.2329033807f },
    { 0.2366534740f, 0.2733698412f, 0.1162459615f, -0.2195797060f },
    { -0.7064083607f, -0.3722998341f, -3.3327763422f, -3.2676743146f },
    { 0.0810537624f, -0.9897814477f, -0.1841711166f, 0.1095410510f },
    { -0.2802757255f, -1.3975128828f, -4.3334540597f, -1.7647213147f },
};
#else
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
#endif

} // namespace Segmenter
