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
#ifndef __CALF_AMP_H
#define __CALF_AMP_H

#include <assert.h>
#include "biquad.h"
#include "audio_fx.h"

using namespace synth;

class amp_audio_module
{
public:
    enum { in_count = 2, out_count = 2, param_count = 1, rt_capable = true };
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
    uint32_t process(uint32_t nsamples, uint32_t inputs_mask, uint32_t outputs_mask) {
        if (!inputs_mask)
            return 0;
        float gain = *params[0];
        for (uint32_t i=0; i<nsamples; i++) {
            outs[0][i] = ins[0][i] * gain;
            outs[1][i] = ins[1][i] * gain;
        }
        return inputs_mask;
    }
};

class flanger_audio_module
{
public:
    enum { par_delay, par_depth, par_rate, par_fb, par_amount, param_count };
    enum { in_count = 2, out_count = 2, rt_capable = true };
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
    void handle_midi_event_now(synth::midi_event event) {
    }
    void handle_param_event_now(uint32_t param, float value) {
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
    uint32_t process(uint32_t nsamples, uint32_t inputs_mask, uint32_t outputs_mask) {
        left.process(outs[0], ins[0], nsamples);
        right.process(outs[1], ins[1], nsamples);
        return outputs_mask; // XXXKF allow some delay after input going blank
    }
};

class reverb_audio_module
{
public:    
    enum { par_decay, par_hfdamp, par_amount, param_count };
    enum { in_count = 2, out_count = 2, rt_capable = true };
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
    void handle_midi_event_now(synth::midi_event event) {
    }
    void handle_param_event_now(uint32_t param, float value) {
    }
    void set_sample_rate(uint32_t sr) {
        srate = sr;
        reverb.setup(sr);
    }
    uint32_t process(uint32_t nsamples, uint32_t inputs_mask, uint32_t outputs_mask) {
        float wet = *params[par_amount];
        for (uint32_t i=0; i<nsamples; i++) {
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
    enum { par_cutoff, par_resonance, par_mode, param_count };
    enum { in_count = 2, out_count = 2, rt_capable = true };
    float *ins[in_count]; 
    float *outs[out_count];
    float *params[param_count];
    static const char *param_names[];
    dsp::biquad<float> left, right;
    uint32_t srate;
    static parameter_properties param_props[];
    
    void params_changed() {
        float freq = *params[par_cutoff];
        float q = *params[par_resonance];
        int mode = (*params[par_mode] <= 0.5 ? 0 : 1);
        // printf("freq = %f q = %f mode = %d\n", freq, q, mode);
        if (mode == 0) {
            left.set_lp_rbj(freq, q, srate);
            right.set_lp_rbj(freq, q, srate);
        } else {
            left.set_hp_rbj(freq, q, srate);
            right.set_hp_rbj(freq, q, srate);
        }
    }
    void activate() {
        left.reset();
        right.reset();
    }
    void deactivate() {
    }
    void handle_midi_event_now(synth::midi_event event) {
    }
    void handle_param_event_now(uint32_t param, float value) {
    }
    void set_sample_rate(uint32_t sr) {
        srate = sr;
        params_changed();
    }
    inline int process_channel(dsp::biquad<float> &filter, float *in, float *out, uint32_t numsamples, int inmask) {
        if (inmask) {
            for (uint32_t i = 0; i < numsamples; i++)
                out[i] = filter.process_d1(in[i]);
        } else {
            for (uint32_t i = 0; i < numsamples; i++)
                out[i] = filter.process_d1_zeroin();
        }
        filter.sanitize_d1();
        return filter.empty_d1() ? 0 : inmask;
    }
    uint32_t process(uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask) {
        if (outputs_mask & 1) {
            outputs_mask &= ~1;
            outputs_mask |= process_channel(left, ins[0], outs[0], numsamples, inputs_mask & 1);
        }
        if (outputs_mask & 2) {
            outputs_mask &= ~2;
            outputs_mask |= process_channel(right, ins[1], outs[1], numsamples, inputs_mask & 2);
        }
        return outputs_mask;
    }
};

extern std::string get_builtin_modules_rdf();

#endif
