/* Calf DSP Library
 * "Small" audio modules for modular synthesis
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
#ifndef __CALF_MODULES_SMALL_H
#define __CALF_MODULES_SMALL_H

#include <assert.h>
#include "biquad.h"
#include "inertia.h"
#include "audio_fx.h"
#include "giface.h"
#include "modules.h"

namespace synth {

using namespace dsp;

class small_filter_audio_module: public null_audio_module
{
public:    
    enum { par_cutoff, par_resonance, param_count };
    enum { in_count = 1, out_count = 1, rt_capable = true, support_midi = false };
    float *ins[in_count]; 
    float *outs[out_count];
    float *params[param_count];
    static const char *port_names[];
    dsp::biquad<float> filter;
    uint32_t srate;
    static parameter_properties param_props[];
    
    /// do not export mode and inertia as CVs, as those are settings and not parameters
    void params_changed()
    {
    }
    void activate() {
        params_changed();
        filter.reset();
    }
    void deactivate() {
    }
    void set_sample_rate(uint32_t sr) {
        srate = sr;
    }
};

class small_lp_filter_audio_module: public small_filter_audio_module
{
public:
    uint32_t process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask) {
        filter.set_lp_rbj(*params[par_cutoff], *params[par_resonance], srate);
        for (uint32_t i = offset; i < offset + numsamples; i++)
            outs[0][i] = filter.process_d1(ins[0][i]);
        filter.sanitize_d1();
        return !filter.empty_d1();
    }
    static const char *get_id() { return "lowpass12"; }
    static const char *get_name() { return "lowpass12"; }
    static const char *get_label() { return "12dB/oct RBJ Lowpass"; }
};

class small_hp_filter_audio_module: public small_filter_audio_module
{
public:
    uint32_t process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask) {
        filter.set_lp_rbj(*params[par_cutoff], *params[par_resonance], srate);
        for (uint32_t i = offset; i < offset + numsamples; i++)
            outs[0][i] = filter.process_d1(ins[0][i]);
        filter.sanitize_d1();
        return !filter.empty_d1();
    }
    static const char *get_id() { return "highpass12"; }
    static const char *get_name() { return "highpass12"; }
    static const char *get_label() { return "12dB/oct RBJ Highpass"; }
};

class small_bp_filter_audio_module: public small_filter_audio_module
{
public:
    uint32_t process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask) {
        filter.set_bp_rbj(*params[par_cutoff], *params[par_resonance], srate);
        for (uint32_t i = offset; i < offset + numsamples; i++)
            outs[0][i] = filter.process_d1(ins[0][i]);
        filter.sanitize_d1();
        return !filter.empty_d1();
    }
    static const char *get_id() { return "bandpass6"; }
    static const char *get_name() { return "bandpass6"; }
    static const char *get_label() { return "6dB/oct RBJ Bandpass"; }
};

};

#endif
