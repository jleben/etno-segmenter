#include "../modules/pipeline.hpp"

#include <sndfile.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <cstdlib>

using namespace std;
using namespace Segmenter;

struct Options
{
    string input_filename;
    string output_filename;
    int block_size;
    float resample_rate;
    int limit;
    bool features;
    bool binary;

    Options() :
        block_size(4096),
        resample_rate(11025.f),
        limit(0),
        features(false),
        binary(false)
    {}
};

static void printUsage()
{
    cout << "Usage: extract <filename> <options...>" << endl;
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

        if (strcmp(arg, "-r") == 0) {
            if ( !checkHasValue(argi, argc, argv) )
                return false;
            ++argi;
            std::istringstream val_str(argv[argi]);
            float val;
            val_str >> val;
            if (!val_str.eof()) {
                cerr << "ERROR: Wrong argument format for option -r!" << endl;
                return false;
            }
            opt.resample_rate = val;
        }
        else if (strcmp(arg, "--no-resample") == 0)
            opt.resample_rate = 0;
        else if (strcmp(arg, "-o") == 0) {
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
            opt.limit = atoi(val);
        }
        else if (strcmp(arg, "-f") == 0 || strcmp(arg, "--features") == 0)
            opt.features = true;
        else if (strcmp(arg, "-b") == 0 || strcmp(arg, "--binary") == 0)
            opt.binary = true;
        else if (arg[0] != '-') {
            cerr << "ERROR: Redundant argument: " << arg << endl;
            return false;
        }
        else {
            cerr << "ERROR: Unknown option: " << arg << endl;
            return false;
        }

        ++argi;
    }

    if (opt.output_filename.empty()) {
        opt.output_filename = "extract.out";
        if (opt.binary)
            opt.output_filename += ".wav";
        else
            opt.output_filename += ".txt";
    }

    return true;
}

void printOptions( Options & opt )
{
    cout << "Options:" << endl;
    cout << '\t' << "- input: " << opt.input_filename << endl;
    cout << '\t' << "- output: " << opt.output_filename << endl;
    cout << '\t' << "- block size: " << opt.block_size << endl;
    if (opt.resample_rate > 0)
        cout << '\t' << "- resampling: " << opt.resample_rate << endl;
    else
        cout << '\t' << "- resampling: none" << endl;
    if (opt.limit > 0)
        cout << '\t' << "- limit: " << opt.limit << "%" << endl;
    else
        cout << '\t' << "- limit: none" << endl;
    cout << '\t' << "- type: " << (opt.features ? "features" : "statistics") << endl;
    cout << '\t' << "- format: " << (opt.binary ? "binary" : "text") << endl;
}

int main ( int argc, char *argv[] )
{
    // parse options

    Options opt;

    if (!parseOptions( argc, argv, opt ))
        return 1;

    printOptions(opt);

    // open sound file

    SF_INFO sf_info;
    sf_info.format = 0;

    SNDFILE *sf = sf_open( opt.input_filename.data(), SFM_READ, &sf_info );

    if (!sf) {
        cerr << "ERROR: Failed to open file for reading: " << opt.input_filename << endl;
        return 2;
    }

    if (sf_info.channels != 1) {
        cerr << "ERROR: Can not use files with more than 1 channels." << endl;
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
            cerr << "ERROR: Can not open output file for writing: " << opt.output_filename << endl;
            return 4;
        }
    }
    else {
        text_out.open( opt.output_filename.data(), ios::out );
        if (!text_out.is_open()) {
            cerr << "ERROR: Can not open output file for writing: " << opt.output_filename << endl;
            return 4;
        }
        text_out << std::fixed;
    }

    // create pipeline

    InputContext inCtx;
    inCtx.sampleRate = sf_info.samplerate;
    inCtx.blockSize = opt.block_size;

    FourierContext fCtx;
    fCtx.sampleRate = opt.resample_rate > 0 ? opt.resample_rate : inCtx.sampleRate;
    fCtx.blockSize = std::pow(2, std::floor( std::log(0.05 * fCtx.sampleRate) / std::log(2.0) ));
    fCtx.stepSize = fCtx.blockSize / 2;

    StatisticContext statCtx;
    statCtx.blockSize = 3 * fCtx.sampleRate / fCtx.stepSize;
    statCtx.stepSize = statCtx.blockSize / 6;

    std::cout << "-- processing"
        << " block size = " << fCtx.blockSize
        << ", step size = " << fCtx.stepSize
        << std::endl;

    std::cout << "-- statistics"
        << " block size = " << statCtx.blockSize
        << ", step size = " << statCtx.stepSize
        << std::endl;

    Pipeline * pipeline = new Pipeline( inCtx, fCtx, statCtx );

    // init processing

    float * input_buffer = new float[opt.block_size];
    int frames = 0;
    int progress = 0;

    // go

    while( sf_read_float(sf, input_buffer, opt.block_size) == opt.block_size )
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
#if SEGMENTER_NEW_FEATURES
                    text_out << features.pitchDensity << '\t';
                    text_out << features.tonality << '\t';
                    text_out << features.tonality1 << '\t';
                    text_out << features.fourHzMod << '\t';
#endif
                    text_out << features.mfcc2 << '\t';
                    text_out << features.mfcc3 << '\t';
                    text_out << features.mfcc4 << '\t';
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

        frames += opt.block_size;
        int current_progress = (float) frames / sf_info.frames * 100.f;

        if (opt.limit > 0 && current_progress >= opt.limit) {
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
