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
