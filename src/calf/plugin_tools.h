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
    struct meter_data
    {
        int level_idx;
        int clip_idx;
        dsp::vumeter meter;
    };
    
    std::vector<meter_data> meters;    
    float *const *params;

    void init(float *const *prms, int *lvls, int *clps, int length, uint32_t srate) {
        meters.resize(length);
        for (int i = 0; i < length; i++) {
            meter_data &md = meters[i];
            md.level_idx = lvls[i];
            md.clip_idx = clps[i];
            md.meter.set_reverse(lvls[i] < -1);
            md.meter.set_falloff(1.f, srate);
        }
        params = prms;
    }
    void process(float *values) {
        for (size_t i = 0; i < meters.size(); ++i) {
            meter_data &md = meters[i];
            if ((md.level_idx != -1 && params[(int)abs(md.level_idx)] != NULL) || 
                (md.clip_idx != -1 && params[(int)abs(md.clip_idx)] != NULL))
            {
                md.meter.process(values[i]);
                if (md.level_idx != -1 && params[(int)abs(md.level_idx)])
                    *params[(int)abs(md.level_idx)] = md.meter.level;
                if (md.clip_idx != -1 && params[(int)abs(md.clip_idx)])
                    *params[(int)abs(md.clip_idx)] = md.meter.clip > 0 ? 1.f : 0.f;
            }
        }
    }
    void fall(unsigned int numsamples) {
        for (size_t i = 0; i < meters.size(); ++i)
            if (meters[i].level_idx != -1)
                meters[i].meter.fall(numsamples);
    }
};

struct debug_send_configure_iface: public send_configure_iface
{
    void send_configure(const char *key, const char *value)
    {
        printf("send_configure key=%s value=%s\n", key, value);
    }
};

};

#endif
