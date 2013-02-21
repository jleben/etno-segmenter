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

#ifndef SEGMENTER_PLUGIN_HPP_INCLUDED
#define SEGMENTER_PLUGIN_HPP_INCLUDED

#include <vamp-sdk/Plugin.h>
#include <vector>
#include <fstream>

namespace Segmenter {

class Pipeline;

class Plugin : public Vamp::Plugin
{
public:
    Plugin(float inputSampleRate);
    virtual ~Plugin();

    bool initialise(size_t channels, size_t stepSize, size_t blockSize);
    void reset();

    std::string getIdentifier() const;
    std::string getName() const;
    std::string getDescription() const;
    std::string getMaker() const;
    std::string getCopyright() const;
    int getPluginVersion() const;

    //ParameterList getParameterDescriptors() const;

    InputDomain getInputDomain() const;
    size_t getMinChannelCount() const;
    size_t getMaxChannelCount() const;
    size_t getPreferredBlockSize() const;
    size_t getPreferredStepSize() const;

    OutputList getOutputDescriptors() const;

    //float getParameter(std::string) const;
    //void setParameter(std::string, float);

    FeatureSet process(const float *const *inputBuffers,
                       Vamp::RealTime timestamp);

    FeatureSet getRemainingFeatures();

private:
    void createPipeline();

    FeatureSet getFeatures(const float * input, Vamp::RealTime timestamp);

    int m_blockSize;

    Pipeline * m_pipeline;

    Vamp::RealTime m_featureDuration;
    Vamp::RealTime m_featureTime;
};

} // namespace Segmenter

#endif
