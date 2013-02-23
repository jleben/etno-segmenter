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
#include <cassert>

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

    enum InputFeature {
        ENERGY = 0,
        ENERGY_GATE,
        ENTROPY,
        PITCH_DENSITY,
        TONALITY,
        TONALITY1,
        FOUR_HZ_MOD,
        MFCC2,
        MFCC3,
        MFCC4,

        INPUT_FEATURE_COUNT
    };

    struct InputFeatures {
        float data [INPUT_FEATURE_COUNT];
        float & operator [] (InputFeature i) { return data[i]; }
        const float & operator [] (InputFeature i) const { return data[i]; }
    };

    struct OutputFeatures {
        float data [OUTPUT_FEATURE_COUNT];
        float & operator [] (OutputFeature i) { return data[i]; }
        const float & operator [] (OutputFeature i) const { return data[i]; }
    };

private:
    enum DeltaFeature {
        ENTROPY_DELTA = 0,
        MFCC2_DELTA,
        MFCC3_DELTA,
        MFCC4_DELTA,

        DELTA_FEATURE_COUNT
    };

    struct DeltaFeatures {
        float data [DELTA_FEATURE_COUNT];
        float & operator [] (DeltaFeature i) { return data[i]; }
        const float & operator [] (DeltaFeature i) const { return data[i]; }
    };


    struct Vector {
        float *data;
        int observations;
        int samples;

        Vector( std::vector<InputFeatures> & feature_vector, int index, int count, InputFeature feature ):
            data( &feature_vector[index][feature] ),
            observations( INPUT_FEATURE_COUNT ),
            samples( count )
        {};

        Vector( std::vector<DeltaFeatures> & feature_vector, int index, int count, DeltaFeature feature ):
            data( &feature_vector[index][feature] ),
            observations( DELTA_FEATURE_COUNT ),
            samples( count )
        {};
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
            DeltaFeatures deltaFeatures;
            deltaFeatures[ENTROPY_DELTA] = delta( vector(ENTROPY, idx, deltaFilterSize) );
            deltaFeatures[MFCC2_DELTA] = delta( vector(MFCC2, idx, deltaFilterSize) );
            deltaFeatures[MFCC3_DELTA] = delta( vector(MFCC3, idx, deltaFilterSize) );
            deltaFeatures[MFCC4_DELTA] = delta( vector(MFCC4, idx, deltaFilterSize) );

            m_deltaBuffer.push_back( deltaFeatures );
        }

        // compute statistics on inputs & deltas
        const int lastDeltaToProcess = (int) m_deltaBuffer.size() - m_windowSize;
        int idx;
        for (idx = 0; idx <= lastDeltaToProcess; idx += m_stepSize)
        {
            int input_idx = idx + m_halfFilterLen;

            Vector gate_vector = vector(ENERGY_GATE, input_idx, m_windowSize);

#define INPUT_VECTOR( feature ) \
    vector(feature, input_idx, m_windowSize), gate_vector

#define DELTA_VECTOR( feature ) \
    vector(feature, idx, m_windowSize), gate_vector

            OutputFeatures output;

            output[ENTROPY_MEAN] = mean( INPUT_VECTOR( ENTROPY ) );
            output[PITH_DENSITY_MEAN] = mean( INPUT_VECTOR( PITCH_DENSITY ) );
            output[TONALITY_MEAN] = mean( INPUT_VECTOR( TONALITY ) );
            output[TONALITY1_MEAN] = mean( INPUT_VECTOR( TONALITY1 ) );
            output[FOUR_HZ_MOD_MEAN] = mean( INPUT_VECTOR( FOUR_HZ_MOD ) );
            output[MFCC2_MEAN] = mean( INPUT_VECTOR( MFCC2 ) );
            output[MFCC3_MEAN] = mean( INPUT_VECTOR( MFCC3 ) );
            output[MFCC4_MEAN] = mean( INPUT_VECTOR( MFCC4 ) );
            output[ENTROPY_DELTA_VAR] = variance( DELTA_VECTOR( ENTROPY_DELTA ) );
            float tonalityMean = output[TONALITY_MEAN];
            float tonalityVar = variance( INPUT_VECTOR( TONALITY ), tonalityMean );
            output[TONALITY_FLUCT] = tonalityMean != 0.f ? tonalityVar / (tonalityMean * tonalityMean) : 0.f;
            output[MFCC2_STD] = stdDev( INPUT_VECTOR( MFCC2 ), output[MFCC2_MEAN] );
            output[MFCC3_STD] = stdDev( INPUT_VECTOR( MFCC3 ), output[MFCC3_MEAN] );
            output[MFCC4_STD] = stdDev( INPUT_VECTOR( MFCC4 ), output[MFCC4_MEAN] );
            output[MFCC2_DELTA_STD] = stdDev( DELTA_VECTOR( MFCC2_DELTA ) );
            output[MFCC3_DELTA_STD] = stdDev( DELTA_VECTOR( MFCC3_DELTA ) );
            output[MFCC4_DELTA_STD] = stdDev( DELTA_VECTOR( MFCC4_DELTA ) );

            float gate_mean = 0.f;
            float *gate_data = gate_vector.data;
            for (int i = 0; i < gate_vector.samples; ++i, gate_data += gate_vector.observations)
                gate_mean += *gate_data;
            gate_mean /= gate_vector.samples;
            output[ENERGY_GATE_MEAN] = gate_mean;

            outBuffer.push_back(output);

#undef INPUT_VECTOR
#undef DELTA_VECTOR
        }

        // remove processed inputs & deltas
        m_inputBuffer.erase( m_inputBuffer.begin(), m_inputBuffer.begin() + idx );
        m_deltaBuffer.erase( m_deltaBuffer.begin(), m_deltaBuffer.begin() + idx );
    }

    Vector vector( InputFeature feature, int idx, int count )
    {
        return Vector(m_inputBuffer, idx, count, feature);
    }

    Vector vector( DeltaFeature feature, int idx, int count )
    {
        return Vector(m_deltaBuffer, idx, count, feature);
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

    float delta( const Vector & feature_vector )
    {
        int filter_size = m_deltaFilter.size();
        float *feature_data = feature_vector.data;
        float output = 0;
        for (int idx = 0; idx < filter_size; ++idx, feature_data += feature_vector.observations)
            output += *feature_data * m_deltaFilter[idx];
        return output;
    }

    float mean ( const Vector & feature_vector, const Vector & gate_vector )
    {
        assert(feature_vector.samples == gate_vector.samples);
        float *feature_data = feature_vector.data;
        float *gate_data = gate_vector.data;
        float mean = 0.f;
        int count = 0;
        for (int i = 0; i < feature_vector.samples; ++i,
             feature_data += feature_vector.observations, gate_data += gate_vector.observations)
        {
            if (*gate_data == 1.f) {
                mean += *feature_data;
                ++count;
            }
        }
        if (count)
            mean /= count;
        return mean;
    }

    float variance ( const Vector & feature_vector, const Vector & gate_vector )
    {
        float m = mean( feature_vector, gate_vector );
        return variance( feature_vector, gate_vector, m );
    }

    float variance ( const Vector & feature_vector, const Vector & gate_vector, float mean )
    {
        assert(feature_vector.samples == gate_vector.samples);
        float *feature_data = feature_vector.data;
        float *gate_data = gate_vector.data;
        float variance = 0.f;
        int count = 0;
        for (int i = 0; i < feature_vector.samples; ++i,
             feature_data += feature_vector.observations, gate_data += gate_vector.observations)
        {
            if (*gate_data == 1.f) {
                float dev = *feature_data - mean;
                variance += dev * dev;
                ++count;
            }
        }
        if (count > 1)
            variance /= count - 1;
        else
            variance = 0.f;
        return variance;
    }

    float stdDev ( const Vector & feature_vector, const Vector & gate_vector, float mean )
    {
        float var = variance( feature_vector, gate_vector, mean );
        return std::sqrt(var);
    }

    float stdDev ( const Vector & feature_vector, const Vector & gate_vector )
    {
        float var = variance( feature_vector, gate_vector );
        return std::sqrt(var);
    }
};

} // namespace SEGMENTER

#endif // SEGMENTER_STATISTICS_HPP_INCLUDED
