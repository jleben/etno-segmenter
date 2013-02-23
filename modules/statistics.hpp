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

#ifndef SEGMENTER_STATISTICS_HPP_INCLUDED
#define SEGMENTER_STATISTICS_HPP_INCLUDED

#include "module.hpp"

#include <vector>

namespace Segmenter {

class Statistics : public Module
{
public:

    enum OutputFeature {
        ENTROPY_MEAN = 0,
        PITH_DENSITY_MEAN,
        TONALITY_MEAN,
        TONALITY1_MEAN,
        FOUR_HZ_MOD_MEAN,
        MFCC2_MEAN,
        MFCC3_MEAN,
        MFCC4_MEAN,
        ENTROPY_DELTA_VAR,
        TONALITY_FLUCT,
        MFCC2_STD,
        MFCC3_STD,
        MFCC4_STD,
        MFCC2_DELTA_STD,
        MFCC3_DELTA_STD,
        MFCC4_DELTA_STD,
        ENERGY_GATE_MEAN,

        OUTPUT_FEATURE_COUNT
    };

    struct InputFeatures {
        float energy;
        float energyGate;
        float entropy;
        float pitchDensity;
        float tonality;
        float tonality1;
        float fourHzMod;
        float mfcc2;
        float mfcc3;
        float mfcc4;
    };

    struct OutputFeatures {
        float features [OUTPUT_FEATURE_COUNT];

        float & operator [] (int i) { return features[i]; }
        const float & operator [] (int i) const { return features[i]; }
    };

private:
    struct DeltaFeatures {
        float entropy;
        float mfcc2;
        float mfcc3;
        float mfcc4;
    };

    int m_windowSize;
    int m_stepSize;

    std::vector<float> m_deltaFilter;
    int m_halfFilterLen;

    std::vector<InputFeatures> m_inputBuffer;
    std::vector<DeltaFeatures> m_deltaBuffer;

    bool m_first;

public:
    Statistics( int windowSize, int stepSize, int deltaWindowSize ):
        m_windowSize(windowSize),
        m_stepSize(stepSize),
        m_first(false)
    {
        initDeltaFilter( deltaWindowSize );
    }

    void process ( const InputFeatures & input, std::vector<OutputFeatures> & outBuffer )
    {
        // populate input buffer;
        // pad beginning and end for the purpose of delta computations
        if (m_first) {
            m_first = false;
            m_inputBuffer.insert( m_inputBuffer.begin(), m_halfFilterLen, input );
        }

        m_inputBuffer.push_back(input);

        process( outBuffer );
    }

    void processRemainingData ( std::vector<OutputFeatures> & outBuffer )
    {
        if ( !m_inputBuffer.size() )
            return;

        InputFeatures lastInput = m_inputBuffer.back();

        m_inputBuffer.insert( m_inputBuffer.end(), m_halfFilterLen, lastInput );

        process( outBuffer );
    }

private:
    void process ( std::vector<OutputFeatures> & outBuffer )
    {
        if (m_inputBuffer.size() < m_deltaFilter.size())
            return;

        const int inputBufSize = m_inputBuffer.size();
        const int deltaFilterSize = m_deltaFilter.size();

        // compute deltas for new inputs and populate delta buffer
        const int lastInputToProcess = inputBufSize - deltaFilterSize;
        for (int idx = m_deltaBuffer.size(); idx <= lastInputToProcess; ++idx)
        {
#define APPLY_FILTER( dst, member, srcIdx ) \
    dst.member = 0; \
    for (int filterIdx = 0; filterIdx < deltaFilterSize; ++filterIdx) \
        dst.member += m_deltaFilter[filterIdx] * m_inputBuffer[srcIdx + filterIdx].member;

            DeltaFeatures delta;
            APPLY_FILTER( delta, entropy, idx );
            APPLY_FILTER( delta, mfcc2, idx );
            APPLY_FILTER( delta, mfcc3, idx );
            APPLY_FILTER( delta, mfcc4, idx );
            m_deltaBuffer.push_back( delta );

#undef APPLY_FILTER
        }

        // compute statistics on inputs & deltas
        const int lastDeltaToProcess = (int) m_deltaBuffer.size() - m_windowSize;
        int idx;
        for (idx = 0; idx <= lastDeltaToProcess; idx += m_stepSize)
        {
            //std::cout << "*********** idx = " << idx;
#define INPUT_VECTOR( feature ) \
        &m_inputBuffer[idx + m_halfFilterLen].feature, m_windowSize, (sizeof(InputFeatures) / sizeof(float))

#define DELTA_VECTOR( feature ) \
        &m_deltaBuffer[idx].feature, m_windowSize, (sizeof(DeltaFeatures) / sizeof(float))

            OutputFeatures output;

            output[ENTROPY_MEAN] = mean( INPUT_VECTOR(entropy) );
            output[PITH_DENSITY_MEAN] = mean( INPUT_VECTOR(pitchDensity) );
            output[TONALITY_MEAN] = mean( INPUT_VECTOR(tonality) );
            output[TONALITY1_MEAN] = mean( INPUT_VECTOR(tonality1) );
            output[FOUR_HZ_MOD_MEAN] = mean( INPUT_VECTOR(fourHzMod) );
            output[MFCC2_MEAN] = mean( INPUT_VECTOR(mfcc2) );
            output[MFCC3_MEAN] = mean( INPUT_VECTOR(mfcc3) );
            output[MFCC4_MEAN] = mean( INPUT_VECTOR(mfcc4) );
            output[ENTROPY_DELTA_VAR] = variance( DELTA_VECTOR(entropy) );
            float tonalityMean = output[TONALITY_MEAN];
            float tonalityVar = variance( INPUT_VECTOR(tonality), tonalityMean );
            output[TONALITY_FLUCT] = tonalityMean != 0.f ? tonalityVar / (tonalityMean * tonalityMean) : 0.f;
            output[MFCC2_STD] = stdDev( INPUT_VECTOR(mfcc2), output[MFCC2_MEAN] );
            output[MFCC3_STD] = stdDev( INPUT_VECTOR(mfcc3), output[MFCC3_MEAN] );
            output[MFCC4_STD] = stdDev( INPUT_VECTOR(mfcc4), output[MFCC4_MEAN] );
            output[MFCC2_DELTA_STD] = stdDev( DELTA_VECTOR(mfcc2) );
            output[MFCC3_DELTA_STD] = stdDev( DELTA_VECTOR(mfcc3) );
            output[MFCC4_DELTA_STD] = stdDev( DELTA_VECTOR(mfcc4) );
            output[ENERGY_GATE_MEAN] = mean( INPUT_VECTOR(energyGate) );

            outBuffer.push_back(output);

#undef INPUT_VECTOR
#undef DELTA_VECTOR
        }

        // remove processed inputs & deltas
        m_inputBuffer.erase( m_inputBuffer.begin(), m_inputBuffer.begin() + idx );
        m_deltaBuffer.erase( m_deltaBuffer.begin(), m_deltaBuffer.begin() + idx );
    }

    void initDeltaFilter( int filterLen )
    {
        int halfFilterLen = (filterLen - 1) / 2;
        m_deltaFilter.resize(2 * halfFilterLen + 1);
        float sum = 0;
        for (int i = -halfFilterLen; i <= halfFilterLen; ++i)
            sum += i*i;
        for (int i = -halfFilterLen; i <= halfFilterLen; ++i)
            m_deltaFilter[ halfFilterLen + i ] = i / sum;
        m_halfFilterLen = halfFilterLen;
    }

    void applyFilter( const std::vector<float> & filter, float *dst, float *src, int src_spacing )
    {
        int filterSize = filter.size();
        *dst = 0;
        for (int filterIdx = 0; filterIdx < filterSize; ++filterIdx) {
            *dst += *src * filter[filterIdx];
            src += src_spacing;
        }
    }

    float mean ( float *vec, int count, int spacing )
    {
        float mean = 0.f;
        for (int i = 0; i < count; ++i, vec += spacing)
            mean += *vec;
        mean /= count;
        return mean;
    }

    float variance ( float *vec, int count, int spacing )
    {
        float m = mean( vec, count, spacing );
        return variance( vec, count, spacing, m );
    }

    float variance ( float *vec, int count, int spacing, float mean )
    {
        float variance = 0.f;
        for (int i = 0; i < count; ++i, vec += spacing) {
            float dev = *vec - mean;
            variance += dev * dev;
        }
        variance /= count - 1;
        return variance;
    }

    float stdDev ( float *vec, int count, int spacing, float mean )
    {
        float var = variance(vec, count, spacing, mean);
        return std::sqrt(var);
    }

    float stdDev ( float *vec, int count, int spacing )
    {
        float var = variance(vec, count, spacing);
        return std::sqrt(var);
    }
};

} // namespace SEGMENTER

#endif // SEGMENTER_STATISTICS_HPP_INCLUDED
