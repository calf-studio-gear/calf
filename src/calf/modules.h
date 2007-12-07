/* Calf DSP Library
 * Example audio modules
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
#ifndef __CALF_MODULES_H
#define __CALF_MODULES_H

#include <assert.h>
#include "biquad.h"
#include "audio_fx.h"
#include "inertia.h"

namespace synth {

using namespace dsp;

class amp_audio_module
{
public:
    enum { in_count = 2, out_count = 2, param_count = 1, support_midi = false, rt_capable = true };
    float *ins[2]; 
    float *outs[2];
    float *params[1];
    uint32_t srate;
    static const char *param_names[];
    static parameter_properties param_props[];
    void set_sample_rate(uint32_t sr) {
    }
    void handle_midi_event_now(synth::midi_event event);
    void handle_param_event_now(uint32_t param, float value);
    void params_changed() {
    }
    void activate() {
    }
    void deactivate() {
    }
    uint32_t process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask) {
        if (!inputs_mask)
            return 0;
        float gain = *params[0];
        numsamples += offset;
        for (uint32_t i = offset; i < numsamples; i++) {
            outs[0][i] = ins[0][i] * gain;
            outs[1][i] = ins[1][i] * gain;
        }
        return inputs_mask;
    }
};

inline void default_handle_event(uint8_t *data, int len, float *params[], unsigned int param_count)
{
    if (data[0] == 0x7F && len >= 8) {
        midi_event *p = (midi_event *)data;
        if (p->param2 < param_count)
            *params[p->param2] = p->param3;
    }
}

class flanger_audio_module
{
public:
    enum { par_delay, par_depth, par_rate, par_fb, par_amount, param_count };
    enum { in_count = 2, out_count = 2, support_midi = false, rt_capable = true };
    static const char *param_names[];
    dsp::simple_flanger<float, 2048> left, right;
    float *ins[in_count]; 
    float *outs[out_count];
    float *params[param_count];
    uint32_t srate;
    static parameter_properties param_props[];
    void set_sample_rate(uint32_t sr) {
        srate = sr;
        left.setup(sr);
        right.setup(sr);
    }
    void handle_event(uint8_t *data, int len) {
        default_handle_event(data, len, params, param_count);
    }
    void params_changed() {
        float dry = 1.0;
        float wet = *params[par_amount];
        float rate = *params[par_rate]; // 0.01*pow(1000.0f,*params[par_rate]);
        float min_delay = *params[par_delay] / 1000.0;
        float mod_depth = *params[par_depth] / 1000.0;
        float fb = *params[par_fb];
        left.set_dry(dry); right.set_dry(dry);
        left.set_wet(wet); right.set_wet(wet);
        left.set_rate(rate); right.set_rate(rate);
        left.set_min_delay(min_delay); right.set_min_delay(min_delay);
        left.set_mod_depth(mod_depth); right.set_mod_depth(mod_depth);
        left.set_fb(fb); right.set_fb(fb);
    }
    void activate() {
        left.reset();
        right.reset();
    }
    void deactivate() {
    }
    uint32_t process(uint32_t offset, uint32_t nsamples, uint32_t inputs_mask, uint32_t outputs_mask) {
        left.process(outs[0] + offset, ins[0] + offset, nsamples);
        right.process(outs[1] + offset, ins[1] + offset, nsamples);
        return outputs_mask; // XXXKF allow some delay after input going blank
    }
};

class reverb_audio_module
{
public:    
    enum { par_decay, par_hfdamp, par_amount, param_count };
    enum { in_count = 2, out_count = 2, support_midi = false, rt_capable = true };
    static const char *param_names[];
    dsp::reverb<float> reverb;
    uint32_t srate;
    float *ins[in_count]; 
    float *outs[out_count];
    float *params[param_count];
    static parameter_properties param_props[];
    
    void params_changed() {
        //reverb.set_time(0.5*pow(8.0f, *params[par_decay]));
        //reverb.set_cutoff(2000*pow(10.0f, *params[par_hfdamp]));
        reverb.set_time(*params[par_decay]);
        reverb.set_cutoff(*params[par_hfdamp]);
    }
    void activate() {
        reverb.reset();
    }
    void deactivate() {
    }
    void handle_event(uint8_t *data, int len) {
        default_handle_event(data, len, params, param_count);
    }
    void set_sample_rate(uint32_t sr) {
        srate = sr;
        reverb.setup(sr);
    }
    uint32_t process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask) {
        float wet = *params[par_amount];
        numsamples += offset;
        for (uint32_t i = offset; i < numsamples; i++) {
            float l = ins[0][i], r = ins[1][i];
            float rl = l, rr = r;
            reverb.process(rl, rr);
            outs[0][i] = l + wet*rl;
            outs[1][i] = r + wet*rr;
        }
        return outputs_mask;
    }
};

class filter_audio_module
{
public:    
    enum { par_cutoff, par_resonance, par_mode, par_inertia, param_count };
    enum { in_count = 2, out_count = 2, rt_capable = true, support_midi = false };
    float *ins[in_count]; 
    float *outs[out_count];
    float *params[param_count];
    static const char *param_names[];
    dsp::biquad<float> left[3], right[3];
    uint32_t srate;
    static parameter_properties param_props[];
    int order;
    inertia<exponential_ramp> inertia_cutoff;
    once_per_n timer;
    
    filter_audio_module()
    : inertia_cutoff(exponential_ramp(128), 20)
    , timer(128)
    {
    }
    
    void calculate_filter()
    {
        float freq = inertia_cutoff.get_last();
        // printf("freq=%g inr.cnt=%d timer.left=%d\n", freq, inertia_cutoff.count, timer.left);
        // XXXKF this is resonance of a single stage, obviously for three stages, resonant gain will be different
        float q = *params[par_resonance];
        // XXXKF this is highly inefficient and should be replaced as soon as I have fast f2i in primitives.h
        int mode = (int)*params[par_mode];
        // printf("freq = %f q = %f mode = %d\n", freq, q, mode);
        if (mode < 3) {
            left[0].set_lp_rbj(freq, q, srate);
            order = mode + 1;
        } else {
            left[0].set_hp_rbj(freq, q, srate);
            order = mode - 2;
        }
        // XXXKF this is highly inefficient and should be replaced as soon as I have fast f2i in primitives.h
        int inertia = (int)*params[par_inertia];
        if (inertia != inertia_cutoff.ramp.length())
            inertia_cutoff.ramp.set_length(inertia);
        
        right[0].copy_coeffs(left[0]);
        for (int i = 1; i < order; i++) {
            left[i].copy_coeffs(left[0]);
            right[i].copy_coeffs(left[0]);
        }
    }
    void params_changed()
    {
        inertia_cutoff.set_inertia(*params[par_cutoff]);
        calculate_filter();
    }
    void on_timer()
    {
        inertia_cutoff.step();
        calculate_filter();
    }
    void activate() {
        calculate_filter();
        for (int i=0; i < order; i++) {
            left[i].reset();
            right[i].reset();
        }
        timer = once_per_n(srate / 1000);
        timer.start();
    }
    void deactivate() {
    }
    void handle_event(uint8_t *data, int len) {
        default_handle_event(data, len, params, param_count);
    }
    void set_sample_rate(uint32_t sr) {
        srate = sr;
        params_changed();
    }
    inline int process_channel(dsp::biquad<float> *filter, float *in, float *out, uint32_t numsamples, int inmask) {
        if (inmask) {
            switch(order) {
                case 1:
                    for (uint32_t i = 0; i < numsamples; i++)
                        out[i] = filter[0].process_d1(in[i]);
                    break;
                case 2:
                    for (uint32_t i = 0; i < numsamples; i++)
                        out[i] = filter[1].process_d1(filter[0].process_d1(in[i]));
                    break;
                case 3:
                    for (uint32_t i = 0; i < numsamples; i++)
                        out[i] = filter[2].process_d1(filter[1].process_d1(filter[0].process_d1(in[i])));
                    break;
            }
        } else {
            if (filter[order - 1].empty_d1())
                return 0;
            switch(order) {
                case 1:
                    for (uint32_t i = 0; i < numsamples; i++)
                        out[i] = filter[0].process_d1_zeroin();
                    break;
                case 2:
                    if (filter[0].empty_d1())
                        for (uint32_t i = 0; i < numsamples; i++)
                            out[i] = filter[1].process_d1_zeroin();
                    else
                        for (uint32_t i = 0; i < numsamples; i++)
                            out[i] = filter[1].process_d1(filter[0].process_d1_zeroin());
                    break;
                case 3:
                    if (filter[1].empty_d1())
                        for (uint32_t i = 0; i < numsamples; i++)
                            out[i] = filter[2].process_d1_zeroin();
                    else
                        for (uint32_t i = 0; i < numsamples; i++)
                            out[i] = filter[2].process_d1(filter[1].process_d1(filter[0].process_d1_zeroin()));
                    break;
            }
        }
        for (int i = 0; i < order; i++)
            filter[i].sanitize_d1();
        return filter[order - 1].empty_d1() ? 0 : inmask;
    }
    uint32_t process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask) {
        uint32_t ostate = 0;
        numsamples += offset;
        while(offset < numsamples) {
            uint32_t numnow = numsamples - offset;
            // if inertia's inactive, we can calculate the whole buffer at once
            if (inertia_cutoff.active())
                numnow = timer.get(numnow);
            if (outputs_mask & 1) {
                ostate |= process_channel(left, ins[0] + offset, outs[0] + offset, numnow, inputs_mask & 1);
            }
            if (outputs_mask & 2) {
                ostate |= process_channel(right, ins[1] + offset, outs[1] + offset, numnow, inputs_mask & 2);
            }
            if (timer.elapsed()) {
                on_timer();
            }
            offset += numnow;
        }
        return ostate;
    }
};

extern std::string get_builtin_modules_rdf();

};

#endif
