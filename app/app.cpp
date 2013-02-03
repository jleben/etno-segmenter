#include "../modules/pipeline.hpp"

#include <sndfile.h>

#include <iostream>
#include <fstream>
#include <cstring>
#include <cstdlib>

using namespace std;
using namespace Segmenter;

struct Options
{
    string input_filename;
    string output_filename;
    int output_limit;
    int blockSize;
    bool features;
    bool binary;

    Options() :
        output_limit(0),
        blockSize(4096),
        features(false),
        binary(false)
    {}
};

static void printUsage()
{
    cout << "Usage: segmenter <filename> <options...>" << endl;
}

static bool checkHasValue( int i, int argc, char **argv  ) {
    bool ok = (i+1 < argc) && (argv[i+1][0] != '-');
    if (!ok)
        cerr << "Option " << argv[i] << " needs an argument!" << endl;
    return ok;
}

static bool parseOptions( int argc, char **argv, Options & opt )
{
    if (argc < 2 || argv[1][0] == '-') {
        printUsage();
        return false;
    }

    opt.input_filename = argv[1];

    int argi = 2;
    while( argi < argc )
    {
        char *arg = argv[argi];

        if (strcmp(arg, "-o") == 0) {
            if ( !checkHasValue(argi, argc, argv) )
                return false;
            ++argi;
            opt.output_filename = argv[argi];
        }
        else if (strcmp(arg, "-l") == 0) {
            if ( !checkHasValue(argi, argc, argv) )
                return false;
            ++argi;
            char *val = argv[argi];
            opt.output_limit = atoi(val);
        }
        else if (strcmp(arg, "-f") == 0 || strcmp(arg, "--features") == 0)
            opt.features = true;
        else if (strcmp(arg, "-b") == 0 || strcmp(arg, "--binary") == 0)
            opt.binary = true;
        else if (arg[0] != '-') {
            cerr << "Redundant argument: " << arg << endl;
            return false;
        }
        else {
            cerr << "Unknown option: " << arg << endl;
            return false;
        }

        ++argi;
    }

    if (opt.output_filename.empty()) {
        opt.output_filename = "segmenter.out";
        if (opt.binary)
            opt.output_filename += ".wav";
        else
            opt.output_filename += ".txt";
    }

    return true;
}

int main ( int argc, char *argv[] )
{
    // parse options

    Options opt;

    if (!parseOptions( argc, argv, opt ))
        return 1;

    // open sound file

    SF_INFO sf_info;
    sf_info.format = 0;

    SNDFILE *sf = sf_open( opt.input_filename.data(), SFM_READ, &sf_info );

    if (!sf) {
        cerr << "*** Segmenter: Failed to open file for reading: " << opt.input_filename << endl;
        return 2;
    }

    if (sf_info.channels != 1) {
        cerr << "*** Segmenter: Can not use files with more than 1 channels." << endl;
        return 3;
    }

    // open output file

    fstream text_out;
    SNDFILE *sf_out = 0;
    SF_INFO sf_out_info;

    if (opt.binary) {
        sf_out_info.format = SF_FORMAT_WAV | SF_FORMAT_FLOAT;
        sf_out_info.samplerate = sf_info.samplerate;
        sf_out_info.channels =
            opt.features ?
            sizeof(Statistics::InputFeatures) / sizeof(float) :
            Statistics::OUTPUT_FEATURE_COUNT;

        sf_out = sf_open( opt.output_filename.data(), SFM_WRITE, &sf_out_info );
        if (!sf_out) {
            cerr << "*** Segmenter: Can not open output file for writing: " << opt.output_filename << endl;
            return 4;
        }
    }
    else {
        text_out.open( opt.output_filename.data(), ios::out );
        if (!text_out.is_open()) {
            cerr << "*** Segmenter: Can not open output file for writing: " << opt.output_filename << endl;
            return 4;
        }
        text_out << std::fixed;
    }

    // create pipeline

    InputContext inCtx;
    inCtx.sampleRate = sf_info.samplerate;
    inCtx.blockSize = opt.blockSize;

    FourierContext fCtx;
#if SEGMENTER_NO_RESAMPLING
    fCtx.sampleRate = sf_info.samplerate;
    fCtx.blockSize = 2048;
    fCtx.stepSize = 1024;
#else
    fCtx.sampleRate = 11025;
    fCtx.blockSize = 512;
    fCtx.stepSize = 256;
#endif

    StatisticContext statCtx;
    statCtx.blockSize = 132;
    statCtx.stepSize = 22;

    std::cout << "*** Segmenter: blocksize=" << fCtx.blockSize << " stepSize=" << fCtx.stepSize << std::endl;

    Pipeline * pipeline = new Pipeline( inCtx, fCtx, statCtx );

    // init processing

    float * input_buffer = new float[opt.blockSize];
    int frames = 0;
    int progress = 0;

    // go

    while( sf_read_float(sf, input_buffer, opt.blockSize) == opt.blockSize )
    {
        pipeline->computeStatistics( input_buffer );

        if (opt.features) {
            int featureN = pipeline->features().size();
            if (opt.binary) {
                sf_writef_float( sf_out,
                                 reinterpret_cast<const float*>(pipeline->features().data()),
                                 featureN );
            }
            else {
                for (int t = 0; t < featureN; ++t) {
                    const Statistics::InputFeatures & features = pipeline->features()[t];
                    text_out << features.energy << '\t';
                    text_out << features.entropy << '\t';
                    text_out << features.mfcc2 << '\t';
                    text_out << features.mfcc3 << '\t';
                    text_out << features.mfcc4 << '\t';
#if SEGMENTER_NEW_FEATURES
                    text_out << features.pitchDensity << '\t';
                    text_out << features.tonality << '\t';
                    text_out << features.tonality1 << '\t';
                    text_out << features.fourHzMod << '\t';
#endif
                    text_out << endl;
                }
            }
        }
        else {
            int statN = pipeline->statistics().size();
            if (opt.binary) {
                if (statN) {
                    sf_writef_float( sf_out,
                                    reinterpret_cast<const float*>( pipeline->statistics().data() ),
                                    statN );
                }
            }
            else {
                for (int t = 0; t < statN; ++t) {
                    const Statistics::OutputFeatures & stats = pipeline->statistics()[t];
                    for (int f = 0; f < Statistics::OUTPUT_FEATURE_COUNT; ++f)
                        text_out << stats[f] << '\t';
                    text_out << endl;
                }
            }
        }

        frames += opt.blockSize;
        int current_progress = (float) frames / sf_info.frames * 100.f;

        if (opt.output_limit > 0 && current_progress >= opt.output_limit) {
            cout << current_progress << "%" << endl;
            break;
        }

        if (current_progress > progress) {
            if (current_progress % 5 == 0)
                cout << current_progress << "%" << endl;
            else
                cout << ".";
        }
        progress = current_progress;
        cout.flush();
    }

    // cleanup
    sf_close(sf);
    if (sf_out)
        sf_close(sf_out);
    text_out.close();
    delete[] input_buffer;

    return 0;
}
