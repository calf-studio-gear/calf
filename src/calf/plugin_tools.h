/* Calf DSP plugin pack
 * Tools to use in plugins
 *
 * Copyright (C) 2001-2010 Krzysztof Foltman, Markus Schmidt, Thor Harald Johansen and others
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 * Boston, MA 02111-1307, USA.
 */
#ifndef CALF_PLUGIN_TOOLS_H
#define CALF_PLUGIN_TOOLS_H

#include <config.h>

#include "giface.h"
#include "vumeter.h"

namespace calf_plugins {

class vumeters
{
public:
    static const int max = 128;
    int levels[max];
    int clips[max];
    dsp::vumeter *meters[max];
    float *const *params;
    int amount;
    vumeters() {};
    void init(float *const *prms, int *lvls, int *clps, int length, uint32_t srate) {
        length = std::min(max, length);
        for (int i = 0; i < length; i++) {
            levels[i] = lvls[i];
            clips[i] = clps[i];
            meters[i] = new dsp::vumeter;
            meters[i]->set_falloff(1.f, srate);
        }
        amount = length;
        params = prms;
    }
    void process(float *values) {
        for (int i = 0; i < amount; i++) {
            meters[i]->process(values[i]);
            if (levels[i] >= 0)
                *params[levels[i]] = meters[i]->level;
            if (clips[i] >= 0)
                *params[clips[i]] = meters[i]->clip > 0 ? 1.f : 0.f;
        }
    }
    void fall(unsigned int numsamples) {
        for (int i = 0; i < amount; i++)
            meters[i]->fall(numsamples);
    }
};

};

#endif
