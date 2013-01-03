#ifndef SEGMENTER_RESAMPLER_HPP_INCLUDED
#define SEGMENTER_RESAMPLER_HPP_INCLUDED

//#include "module.hpp"

#include <samplerate.h>
#include <vector>
#include <iostream>
#include <cstdlib>

namespace Segmenter {

using std::vector;

class Resampler
{
    const int m_inSampleRate;
    const int m_outSampleRate;
    vector<float> m_inBuffer;
    vector<float> m_outBuffer;
    SRC_STATE *m_srcState;
    SRC_DATA m_srcData;

public:
    Resampler( int inputSampleRate, int outputSampleRate = 11025 ):
        m_inSampleRate(inputSampleRate),
        m_outSampleRate(outputSampleRate)
    {
        int error = 0;
        const int channelCount = 1;
        m_srcState = src_new( SRC_SINC_BEST_QUALITY, channelCount, &error );
        if (error) {
            std::cout << "Resampler ERROR: " << src_strerror(error) << std::endl;
            return;
        }

        double conversionRatio = (double) m_outSampleRate / m_inSampleRate;
        m_srcData.src_ratio = conversionRatio;
    }

    ~Resampler()
    {
        src_delete(m_srcState);
    }

    void process( const float *data, int size, bool isLastWindow )
    {
        if (!m_srcState)
            return;

        m_outBuffer.clear();

        /*for (int i = 0; i < size; ++i)
            m_inBuffer.push_back( data[i] );*/
        m_inBuffer.insert( m_inBuffer.end(), data, data + size );

        int outBufferSize = m_inBuffer.size() * m_srcData.src_ratio;
        float * outBuffer = new float[outBufferSize];

        m_srcData.data_in = m_inBuffer.data();
        m_srcData.input_frames = m_inBuffer.size();
        m_srcData.data_out = outBuffer;
        m_srcData.output_frames = outBufferSize;
        m_srcData.end_of_input = isLastWindow;

        int error = src_process( m_srcState, &m_srcData );

        if (error) {
            std::cout << "Resampler ERROR: " << src_strerror(error) << std::endl;
            m_inBuffer.clear();
            return;
        }

        /*for (int i = 0; i < m_srcData.output_frames_gen; ++i)
            m_outBuffer.push_back( outBuffer[i] );*/
        m_outBuffer.insert( m_outBuffer.end(), outBuffer, outBuffer + m_srcData.output_frames_gen );
        m_inBuffer.erase( m_inBuffer.begin(), m_inBuffer.begin() + m_srcData.input_frames_used );

        delete outBuffer;
    }

    const std::vector<float> output() const { return m_outBuffer; }
};

} // namespace Segmenter

#endif // SEGMENTER_RESAMPLER_HPP_INCLUDED
