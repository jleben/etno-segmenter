#include "plugin.hpp"
#include "marsyas_plugin.hpp"

#include <vamp/vamp.h>
#include <vamp-sdk/PluginAdapter.h>

static Vamp::PluginAdapter<Segmenter::Plugin> rawPluginAdapter;
static Vamp::PluginAdapter<Segmenter::MarPlugin> marsyasPluginAdapter;

const VampPluginDescriptor *vampGetPluginDescriptor(unsigned int version, unsigned int index)
{
    if (version < 1) return 0;

    switch (index) {
    case 0:
        return rawPluginAdapter.getDescriptor();
    case 1:
        return marsyasPluginAdapter.getDescriptor();
    default:
        return 0;
    }
}
