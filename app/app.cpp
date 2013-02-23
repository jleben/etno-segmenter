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

#include "../modules/pipeline.hpp"

#include <boost/program_options.hpp>

#include <sndfile.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <cstdlib>

using namespace std;
using namespace Segmenter;
namespace po = boost::program_options;

struct Options
{
    string input_filename;
    string output_filename;
    int block_size;
    float resample_rate;
    int resample_type;
    int limit;
    bool features;
    bool binary;

    Options() :
        block_size(4096 * 3),
        resample_rate(11025.f),
        resample_type(1),
        limit(0),
        features(false),
        binary(true)
    {}
};

static void printUsage(po::options_description opt_description)
{
    cout << "Usage: extract file [options...]" << endl;
    cout << opt_description << endl;
}

static void printHelpStatistics()
{
    static const char * strings[] = {
        "Entropy Mean",
        "Pitch Density Mean",
        "Tonality Mean",
        "Tonality 1 Mean",
        "4 Hz Modulation Mean",
        "MFCC 2 Mean",
        "MFCC 3 Mean",
        "MFCC 4 Mean",
        "Entropy Delta Variance",
        "Tonality Fluctuation",
        "MFCC 2 Standard Deviation",
        "MFCC 3 Standard Deviation",
        "MFCC 4 Standard Deviation",
        "MFCC 2 Delta Standard Deviation",
        "MFCC 3 Delta Standard Deviation",
        "MFCC 4 Delta Standard Deviation",
        "Energy Gate Mean"
    };

    cout << "Statistics output contains:" << endl;
    for (int i = 0; i < sizeof(strings) / sizeof(char*); ++i)
        cout << '\t' << i << ' ' << strings[i] << endl;
}

static void printHelpFeatures()
{
    static const char * strings[] = {
        "Energy",
        "Energy Gate",
        "Entropy",
        "Pitch Density",
        "Tonality",
        "Tonality1",
        "4 Hz Modulation",
        "MFCC 2",
        "MFCC 3",
        "MFCC 4"
    };

    cout << "Features output contains:" << endl;
    for (int i = 0; i < sizeof(strings) / sizeof(char*); ++i)
        cout << '\t' << i << ' ' << strings[i] << endl;
}

void printOptions( Options & opt )
{
    cout << "Options:" << endl;
    cout << '\t' << "- input: " << opt.input_filename << endl;
    cout << '\t' << "- output: " << opt.output_filename << endl;
    cout << '\t' << "- block size: " << opt.block_size << " samples" << endl;
    if (opt.resample_rate > 0)
        cout << '\t' << "- resampling: "
             << opt.resample_rate << " Hz "
             << (opt.resample_type == 0 ? "(linear)" : "(sinc)")
             << endl;
    else
        cout << '\t' << "- resampling: none" << endl;
    if (opt.limit > 0)
        cout << '\t' << "- limit: " << opt.limit << "%" << endl;
    else
        cout << '\t' << "- limit: none" << endl;
    cout << '\t' << "- mode: " << (opt.features ? "features" : "statistics") << endl;
    cout << '\t' << "- format: " << (opt.binary ? "binary" : "text") << endl;
}


static bool parse_options( int argc, char **argv, Options & opt )
{
    po::options_description desc("Allowed options");
    desc.add_options()
            ("help,h", po::value<string>()->implicit_value("usage"),
             "Print this help.\n"
             "Optionally, one of the following arguments may be given:\n"
             "  features: \tPrint order of output features.\n"
             "  stats: \tPrint order of output statistics.\n")
            ("input,i", po::value<string>()->required(), "Input file.")
            ("output,o", po::value<string>(), "Output file.")
            ("block-size,b", po::value<int>()->default_value(4096 * 3),
             "Amount of input frames to read and process at a time.")
            ("resample,r", po::value<float>()->default_value(11025.f),
             "Resample to 'arg' sampling rate before feature extraction. 0 implies no resampling.")
            ("resample-linear", "Use linear instead of sinc resampling.")
            ("features,f", "Output raw features instead of statistics.")
            ("text,t", "Output text instead of binary.")
            ("limit,l", po::value<int>(), "Percentage of input to process.")
    ;

    po::positional_options_description positional_desc;
    positional_desc.add("input", 1);

    po::variables_map var;
    po::store(po::command_line_parser(argc, argv)
              .options(desc).positional(positional_desc).run(), var);

    if (!var["input"].empty())
        opt.input_filename = var["input"].as<string>();
    if (!var["output"].empty())
        opt.output_filename = var["output"].as<string>();
    opt.block_size = var["block-size"].as<int>();
    opt.resample_rate = var["resample"].as<float>();
    opt.resample_type = var.count("resample-linear") ? 0 : 1;
    opt.features = var.count("features") > 0;
    opt.binary = var.count("text") == 0;
    if (!var["limit"].empty())
        opt.limit = var["limit"].as<int>();

    if (opt.output_filename.empty()) {
        opt.output_filename = "extract.out";
        if (opt.binary)
            opt.output_filename += ".wav";
        else
            opt.output_filename += ".txt";
    }

    if (var.count("help")) {
        string help_topic = var["help"].as<string>();
        if (help_topic == "usage")
            printUsage(desc);
        else if (help_topic == "features")
            printHelpFeatures();
        else if (help_topic == "stats")
            printHelpStatistics();
        else
            cout << "No help for '" << help_topic << "'" << endl;
        return false;
    }

    if (opt.input_filename.empty()) {
        printUsage(desc);
        return false;
    }

    return true;
}

int main ( int argc, char *argv[] )
{
    // parse options

    Options opt;

    try {
        if (!parse_options( argc, argv, opt ))
            return 0;
    } catch (std::exception & e) {
        cerr << "ERROR in options: " << e.what() << endl;
        return 1;
    }

    if (opt.block_size < 1024) {
        cout << "WARNING: Clipping requested block size (" << opt.block_size << ")"
             << " to minimum (1024)." << endl;
        opt.block_size = 1024;
    }
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
    inCtx.resampleType = opt.resample_type == 0 ? SRC_LINEAR : SRC_SINC_FASTEST;

    FourierContext fCtx;
    fCtx.sampleRate = opt.resample_rate > 0 ? opt.resample_rate : inCtx.sampleRate;
    fCtx.blockSize = std::pow(2, std::floor( std::log(0.05 * fCtx.sampleRate) / std::log(2.0) ));
    fCtx.stepSize = fCtx.blockSize / 2;

    StatisticContext statCtx;
    statCtx.blockSize = 3 * fCtx.sampleRate / fCtx.stepSize;
    statCtx.stepSize = statCtx.blockSize / 6;

    std::cout << "-- input sample rate = " << inCtx.sampleRate << endl;

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
    size_t frames_read = 0;
    bool endOfStream = false;

    // go

    do
    {
        frames_read = sf_read_float(sf, input_buffer, opt.block_size);

        endOfStream = frames_read < opt.block_size;

        pipeline->computeStatistics( input_buffer, frames_read, endOfStream );

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
                    text_out << features.energyGate << '\t';
                    text_out << features.entropy << '\t';
                    text_out << features.pitchDensity << '\t';
                    text_out << features.tonality << '\t';
                    text_out << features.tonality1 << '\t';
                    text_out << features.fourHzMod << '\t';
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

        frames += frames_read;
        int current_progress = (float) frames / sf_info.frames * 100.f;

        if (current_progress > progress) {
            if (current_progress % 5 == 0)
                cout << current_progress << "%" << endl;
            else
                cout << ".";
        }
        cout.flush();

        progress = current_progress;

        if (opt.limit > 0 && progress >= opt.limit)
            break;
    } while (!endOfStream);

    if (progress % 5 != 0)
        cout << progress << "%" << endl;
    cout << "Done" << endl;

    // cleanup
    sf_close(sf);
    if (sf_out)
        sf_close(sf_out);
    text_out.close();
    delete[] input_buffer;

    return 0;
}
