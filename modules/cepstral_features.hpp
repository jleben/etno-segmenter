#ifndef SEGMENTER_CEPSTRAL_FEATURES_INCLUDED
#define SEGMENTER_CEPSTRAL_FEATURES_INCLUDED

#include "module.hpp"

#include <cmath>
#include <vector>
#include <algorithm>
#include <iostream>


namespace Segmenter {

class CepstralFeatures : public Module
{
    int m_nWin;
    int m_iMin;
    int m_iMax;

    float m_tonality;
    float m_tonality1;
    float m_pitchDensity;

    std::vector<float> m_buffer;

public:
    CepstralFeatures( float sampleRate, int windowSize ):
        m_nWin(windowSize)
    {
        int cepstrumSize = windowSize / 2 + 1;

        //m_iMin = std::min( (int) (400 * windowSize / sampleRate), cepstrumSize );
        //m_iMax = std::min( (int) (3000 * windowSize / sampleRate), cepstrumSize );
        // FIXME: don't use absolute indexes, use code above instead:
        m_iMin = lroundf( sampleRate / 1000.f );
        m_iMax = lroundf( sampleRate / 90.f ) + 1;
        std::cout << "*** CepstralFeatures: cepstrum range = " << m_iMin << "-" << m_iMax << std::endl;
    }

    void process ( const std::vector<float> & spectrumMagnitude, const std::vector<float> & realCepstrum )
    {
        if (m_iMin >= realCepstrum.size()) {
            m_tonality = 0.f;
            m_tonality1 = 0.f;
            m_pitchDensity = 0.f;
            return;
        }

        int nSpectrum = spectrumMagnitude.size();

        // preprocess spectrum: spectrum = square( max( magnitude, ath ) )
        // FIXME: is this processing actually intentional, or was simply power spectrum desired??
        static const float ath = 1.0f/65536;
        std::vector<float> & spectrum = m_buffer;
        spectrum.resize(nSpectrum);
        for( int i = 0; i < nSpectrum; ++i ) {
            spectrum[i] = std::max( spectrumMagnitude[i], ath );
            spectrum[i] *= spectrum[i];
        }

        float sumCeps = 0.f;
        float cepsMax = realCepstrum[m_iMin];
        int iCepsMax = m_iMin;

        // find highest cepstral value, and sum all absolute cepstral values
        for (int i = m_iMin + 1; i < m_iMax; ++i) {
            float ceps = realCepstrum[i];
            if (ceps > cepsMax) {
                iCepsMax = i;
                cepsMax = ceps;
            }
            sumCeps += std::abs( ceps );
        }

        float ratioC2S = (float) m_nWin / iCepsMax;

        // find first N partials (spectral values) corresponding to highest cepstral value
        float partials[5];
        int nPartials = 5;
        for (int iPartial = 0; iPartial < nPartials; ++iPartial)
        {
            // FIXME: should interpolate spectrum, instead of rounding index:
            int iSpectrum = lroundf( (iPartial + 1) * ratioC2S );
            if (iSpectrum >= nSpectrum) {
                nPartials = iPartial;
                break;
            }
            partials[iPartial] = spectrum[iSpectrum];
        }

        // find N highest spectral values
        float highest[5];
        std::partial_sort_copy( spectrum.begin(), spectrum.end(),
                                &highest[0], &highest[nPartials],
                                std::greater<float>() );

        // sum N heighest spectral values
        float sumHighest = 0.f;
        for (int iHighest = 0; iHighest < nPartials; ++iHighest)
            sumHighest += highest[iHighest];

        // sum N partials
        float sumPartials = 0.f;
        for (int iPartial = 0; iPartial < nPartials; ++iPartial)
            sumPartials += partials[iPartial];

        m_tonality = cepsMax;
        m_tonality1 = sumHighest > 0.f ? sumPartials / sumHighest : 0.f;
        m_pitchDensity = sumCeps / (m_iMax - m_iMin);
    }

    float tonality() const { return m_tonality; }
    float tonality1() const { return m_tonality1; }
    float pitchDensity() const { return m_pitchDensity; }
};

} // namespace Segmenter

#endif // SEGMENTER_CEPSTRAL_FEATURES_INCLUDED
