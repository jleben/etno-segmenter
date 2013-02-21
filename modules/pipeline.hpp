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

#ifndef SEGMENTER_PIPELINE_INCLUDED
#define SEGMENTER_PIPELINE_INCLUDED

#include "module.hpp"
#include "statistics.hpp"

#include <vector>

#include <vamp-sdk/RealTime.h>
#include <vamp-sdk/Plugin.h>

#include <samplerate.h>

namespace Segmenter {

struct InputContext {
    InputContext(): sampleRate(1), blockSize(0), resampleType(SRC_SINC_FASTEST) {}
    float sampleRate;
    int resampleType;
    int blockSize;
};

class Pipeline
{
public:
    Pipeline ( const InputContext & inCtx,
               const FourierContext & fCtx = FourierContext(),
               const StatisticContext & statCtx = StatisticContext());

    ~Pipeline();

    const InputContext & inputContext() const { return m_inputContext; }
    const FourierContext & fourierContext() const { return m_fourierContext; }
    const StatisticContext & statisticContext() const { return m_statContext; }

    void computeStatistics( const float * input, int count, bool last = false );
    void computeClassification( Vamp::Plugin::FeatureList & output );

    const std::vector<Statistics::InputFeatures> & features() const { return m_featBuffer; }
    const std::vector<Statistics::OutputFeatures> & statistics() const { return m_statsBuffer; }

private:
    enum ModuleType {
        ResamplerModule = 0,
        EnergyModule,
        PowerSpectrumModule,
        MelSpectrumModule,
        MfccModule,
        ChromaticEntropyModule,
        StatisticsModule,
        ClassifierModule,

    #if SEGMENTER_NEW_FEATURES
        RealCepstrumModule,
        CepstralFeaturesModule,
        FourHzModulationModule,
    #endif

        ModuleCount
    };

    Module *& get( ModuleType type ) { return m_modules[type]; }

private:
    InputContext m_inputContext;
    FourierContext m_fourierContext;
    StatisticContext m_statContext;

    Vamp::RealTime m_statsStepDuration;
    Vamp::RealTime m_statsTime;

    std::vector<Module*> m_modules;

    std::vector<float> m_resampBuffer;
    std::vector<float> m_spectrumMag;
    std::vector<Statistics::InputFeatures> m_featBuffer;
    std::vector<Segmenter::Statistics::OutputFeatures> m_statsBuffer;

    bool m_resample;
};

} // namespace Segmenter

#endif // SEGMENTER_PIPELINE_INCLUDED
