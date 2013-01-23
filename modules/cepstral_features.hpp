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

public:
    CepstralFeatures( float sampleRate, int windowSize ):
        m_nWin(windowSize)
    {
        int cepstrumSize = windowSize / 2 + 1;
        m_iMin = std::min( (int) (400 * windowSize / sampleRate), cepstrumSize );
        m_iMax = std::min( (int) (3000 * windowSize / sampleRate), cepstrumSize );
    }

    void process ( const std::vector<float> & powerSpectrum, const std::vector<float> & realCepstrum )
    {
        if (m_iMin >= realCepstrum.size()) {
            m_tonality = 0.f;
            m_tonality1 = 0.f;
            m_pitchDensity = 0.f;
            return;
        }

        int nSpectrum = powerSpectrum.size();

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

        float ratioC2S = m_nWin / iCepsMax;

        // find first N partials (spectral values) corresponding to highest cepstral value
        float partials[5];
        int nPartials = 5;
        for (int iPartial = 0; iPartial < nPartials; ++iPartial)
        {
            int iSpectrum = (int) (iPartial * ratioC2S) + 1;
            if (iSpectrum >= nSpectrum) {
                nPartials = iPartial;
                break;
            }
            partials[iPartial] = powerSpectrum[iSpectrum];
        }
#if 0
        std::cout << "partials:  ";
        for (int iPartial = 0; iPartial < nPartials; ++iPartial)
            std::cout << partials[iPartial] << "  ";
        std::cout << std::endl;
#endif

        // find N highest spectral values
        float highest[5];
        std::partial_sort_copy( powerSpectrum.begin(), powerSpectrum.end(),
                                &highest[0], &highest[nPartials],
                                std::greater<float>() );
#if 0
        std::cout << "highest:  ";
        for (int iHighest = 0; iHighest < nPartials; ++iHighest)
            std::cout << highest[iHighest] << "  ";
        std::cout << std::endl;
#endif
        // sum N heighest spectral values
        float sumHighest = 0.f;
        for (int iHighest = 0; iHighest < nPartials; ++iHighest)
            sumHighest += highest[iHighest];

        // sum partial values relative to sum of heighest spectral values
        float sumRelativePartials = 0.f;
        if (sumHighest > 0.f) {
            for (int iPartial = 0; iPartial < nPartials; ++iPartial)
                sumRelativePartials += partials[iPartial] / sumHighest;
        }

        m_tonality = cepsMax;
        m_tonality1 = sumRelativePartials;
        m_pitchDensity = sumCeps / (m_iMax - m_iMin);
    }

    float tonality() const { return m_tonality; }
    float tonality1() const { return m_tonality1; }
    float pitchDensity() const { return m_pitchDensity; }
};

} // namespace Segmenter

#endif // SEGMENTER_CEPSTRAL_FEATURES_INCLUDED
