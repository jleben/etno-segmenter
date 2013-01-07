#ifndef SEGMENTER_CLASSIFICATION_HPP_INCLUDED
#define SEGMENTER_CLASSIFICATION_HPP_INCLUDED

//#include "module.hpp"

#include <vector>

namespace Segmenter {

class Classifier
{

    static const float s_coeffs[][] = {
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
};

} // namespace Segmenter

#endif // SEGMENTER_CLASSIFICATION_HPP_INCLUDED
