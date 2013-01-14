#ifndef SEGMENTER_STATISTICS_HPP_INCLUDED
#define SEGMENTER_STATISTICS_HPP_INCLUDED

#include <vector>

namespace Segmenter {

class Statistics
{
public:
    /*
    struct OutputFeatures {
        float entropyMean;
        float mfcc2Var;
        float mfcc3Var;
        float mfcc4Var;
        float deltaEntropyVar;
        float deltaMfcc2Var;
        float deltaMfcc3Var;
        float deltaMfcc4Var;
        float energyFlux;
    };
    */
    enum OutputFeature {
        ENTROPY_MEAN = 0,
        MFCC2_VAR,
        MFCC3_VAR,
        MFCC4_VAR,
        ENTROPY_DELTA_VAR,
        MFCC2_DELTA_VAR,
        MFCC3_DELTA_VAR,
        MFCC4_DELTA_VAR,
        ENERGY_FLUX,

        OUTPUT_FEATURE_COUNT
    };

    struct OutputFeatures {
        float features [OUTPUT_FEATURE_COUNT];

        float & operator [] (int i) { return features[i]; }
        const float & operator [] (int i) const { return features[i]; }
    };

    //typedef float OutputFeatures [OUTPUT_FEATURE_COUNT];

private:
    struct DeltaFeatures {
        float entropy;
        float mfcc2;
        float mfcc3;
        float mfcc4;
    };

    struct InputFeatures {
        float entropy;
        float mfcc2;
        float mfcc3;
        float mfcc4;
        float energy;
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

    void process ( float energy, float entropy, const std::vector<float> & mfcc, std::vector<OutputFeatures> & outBuffer )
    {
        InputFeatures input;
        input.energy = energy;
        input.mfcc2 = mfcc[2];
        input.mfcc3 = mfcc[3];
        input.mfcc4 = mfcc[4];
        input.entropy = entropy;

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
            output[MFCC2_VAR] = variance( INPUT_VECTOR(mfcc2 ) );
            output[MFCC3_VAR] = variance( INPUT_VECTOR(mfcc3 ) );
            output[MFCC4_VAR] = variance( INPUT_VECTOR(mfcc4 ) );
            output[ENTROPY_DELTA_VAR] = variance( DELTA_VECTOR(entropy) );
            output[MFCC2_DELTA_VAR] = variance( DELTA_VECTOR(mfcc2) );
            output[MFCC3_DELTA_VAR] = variance( DELTA_VECTOR(mfcc3) );
            output[MFCC4_DELTA_VAR] = variance( DELTA_VECTOR(mfcc4) );
            float energyMean = mean( INPUT_VECTOR(energy) );
            float energyVar = variance( INPUT_VECTOR(energy), energyMean );
            output[ENERGY_FLUX] = energyMean != 0.f ? energyVar / (energyMean * energyMean) : 0.f;

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
};

} // namespace SEGMENTER

#endif // SEGMENTER_STATISTICS_HPP_INCLUDED
