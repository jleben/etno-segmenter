#ifndef SEGMENTER_CEPSTRAL_FEATURES_INCLUDED
#define SEGMENTER_CEPSTRAL_FEATURES_INCLUDED

#include "module.h"

#include <cmath>
#include <vector>

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
        nPartials = 5;
        for (int iPartial = 0; iPartial < nPartials; ++iPartial)
        {
            int iSpectrum = (int) (iPartial * ratioC2S) + 1;
            if (iSpectrum >= nSpectrum) {
                nPartials = iPartial;
                break;
            }
            partials[iPartial] = powerSpectrum[iSpectrum];
        }

        // find N highest spectral values
        float maxBins[5];
        std::memset( &maxBins, 0, 5 * sizeof(float) );
        for (int iBin = 0; iBin < nSpectrum; ++iBin)
        {
            float bin = powerSpectrum[iBin];
            for (int iMaxBin = 0; iMaxBin < nPartials; ++iMaxBin)
            {
                if (bin > maxBins[iMaxBin]) {
                    for(int iSmallerBin = iMaxBin; iSmallerBin > 0; --iSmallerBin)
                        maxBins[iSmallerBin-1] = maxBins[iSmallerBin];
                    maxBins[iMaxBin] = bin;
                    break;
                }
            }
        }

        // sum N heighest spectral values
        float sumMaxBins = 0.f;
        for (int iMaxBin = 0; iMaxBin < nPartials; ++iMaxBin)
            sumMaxBins += maxBins[iMaxBin];

        // sum partial values relative to sum of heighest spectral values
        float sumRelativePartials = 0.f;
        if (sumMaxBins > 0.f) {
            for (int iPartial = 0; iPartial < nPartials; ++iPartial)
                sumRelativePartials += partials[iPartial] / sumMaxBins;
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
