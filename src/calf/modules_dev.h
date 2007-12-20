/* Calf DSP Library
 * Prototype audio modules
 *
 * Copyright (C) 2001-2007 Krzysztof Foltman
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
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#ifndef __CALF_MODULES_DEV_H
#define __CALF_MODULES_DEV_H

#if ENABLE_EXPERIMENTAL

#include <assert.h>
#include "biquad.h"
#include "audio_fx.h"
#include "synth.h"
#include "organ.h"

namespace synth {

using namespace dsp;

struct organ_audio_module: public null_audio_module, public drawbar_organ
{
public:
    using drawbar_organ::note_on;
    using drawbar_organ::note_off;
    using drawbar_organ::control_change;
    enum { par_drawbar1, par_drawbar2, par_drawbar3, par_drawbar4, par_drawbar5, par_drawbar6, par_drawbar7, par_drawbar8, par_drawbar9, par_foldover,
        par_percmode, par_percharm, par_vibrato, par_master, param_count };
    enum { in_count = 0, out_count = 2, support_midi = true, rt_capable = true };
    static const char *port_names[];
    float *ins[in_count]; 
    float *outs[out_count];
    float *params[param_count];
    organ_parameters par_values;
    uint32_t srate;

    organ_audio_module()
    : drawbar_organ(&par_values)
    {
    }
    static parameter_properties param_props[];
    void set_sample_rate(uint32_t sr) {
        srate = sr;
    }
    void params_changed() {
        for (int i = 0; i < param_count; i++)
            ((float *)&par_values)[i] = *params[i];
        set_vibrato();
    }
    void activate() {
        setup(srate);
    }
    void deactivate() {
    }
    uint32_t process(uint32_t offset, uint32_t nsamples, uint32_t inputs_mask, uint32_t outputs_mask) {
        float *o[2] = { outs[0] + offset, outs[1] + offset };
        render_to(o, nsamples);
        return 3;
    }
    
};

};

#endif

#endif
