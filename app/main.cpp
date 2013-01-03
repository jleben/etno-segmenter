#include "../modules/resampler.hpp"
#include "../modules/energy.hpp"
#include "../modules/spectrum.hpp"
#include "../modules/mfcc.hpp"

using namespace Segmenter;

int main()
{
    int sr = 44100;

    Resampler resamp(sr);

    float frames[41000];
    float *input = frames;
    for (int i = 0; i < 10; ++i) {
        resamp.process( input, 4100, false );
    }

    return 0;
}
