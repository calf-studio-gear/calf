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
#include "inertia.h"
#include "audio_fx.h"

namespace synth {

using namespace dsp;

class null_audio_module: public line_graph_iface
{
public:
    inline void note_on(int note, int velocity) {}
    inline void note_off(int note, int velocity) {}
    inline void program_change(int program) {}
    inline void control_change(int controller, int value) {}
    inline void pitch_bend(int value) {} // -8192 to 8191
    inline void params_changed() {}
    inline void activate() {}
    inline void deactivate() {}
    inline void set_sample_rate(uint32_t sr) { }
    inline bool get_graph(int index, int subindex, float *data, int points, cairo_t *context) { return false; }
    inline static const char *get_gui_xml() { return NULL; }
    inline static bool get_static_graph(int index, int subindex, float value, float *data, int points, cairo_t *context) { return false; }
};

class amp_audio_module: public null_audio_module
{
public:
    enum { in_count = 2, out_count = 2, param_count = 1, support_midi = false, rt_capable = true };
    float *ins[2]; 
    float *outs[2];
    float *params[1];
    uint32_t srate;
    static const char *port_names[];
    static parameter_properties param_props[];
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

class flanger_audio_module: public null_audio_module
{
public:
    enum { par_delay, par_depth, par_rate, par_fb, par_amount, param_count };
    enum { in_count = 2, out_count = 2, support_midi = false, rt_capable = true };
    static const char *port_names[];
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

class reverb_audio_module: public null_audio_module
{
public:    
    enum { par_decay, par_hfdamp, par_roomsize, par_diffusion, par_amount, param_count };
    enum { in_count = 2, out_count = 2, support_midi = false, rt_capable = true };
    static const char *port_names[];
    dsp::reverb<float> reverb;
    uint32_t srate;
    gain_smoothing amount;
    float *ins[in_count]; 
    float *outs[out_count];
    float *params[param_count];
    static parameter_properties param_props[];
    
    void params_changed() {
        //reverb.set_time(0.5*pow(8.0f, *params[par_decay]));
        //reverb.set_cutoff(2000*pow(10.0f, *params[par_hfdamp]));
        reverb.set_type_and_diffusion(fastf2i_drm(*params[par_roomsize]), *params[par_diffusion]);
        reverb.set_time(*params[par_decay]);
        reverb.set_cutoff(*params[par_hfdamp]);
        amount.set_inertia(*params[par_amount]);
    }
    void activate() {
        reverb.reset();
    }
    void deactivate() {
    }
    void set_sample_rate(uint32_t sr) {
        srate = sr;
        reverb.setup(sr);
        amount.set_sample_rate(sr);
    }
    uint32_t process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask) {
        numsamples += offset;
        for (uint32_t i = offset; i < numsamples; i++) {
            float wet = amount.get();
            float l = ins[0][i], r = ins[1][i];
            float rl = l, rr = r;
            reverb.process(rl, rr);
            outs[0][i] = l + wet*rl;
            outs[1][i] = r + wet*rr;
        }
        reverb.extra_sanitize();
        return outputs_mask;
    }
};

class filter_audio_module: public null_audio_module
{
public:    
    enum { par_cutoff, par_resonance, par_mode, par_inertia, param_count };
    enum { in_count = 2, out_count = 2, rt_capable = true, support_midi = false };
    float *ins[in_count]; 
    float *outs[out_count];
    float *params[param_count];
    static const char *port_names[];
    dsp::biquad<float> left[3], right[3];
    uint32_t srate;
    static parameter_properties param_props[];
    int order;
    inertia<exponential_ramp> inertia_cutoff, inertia_resonance;
    once_per_n timer;
    
    filter_audio_module()
    : inertia_cutoff(exponential_ramp(128), 20)
    , inertia_resonance(exponential_ramp(128), 20)
    , timer(128)
    {
        order = 0;
    }
    
    void calculate_filter()
    {
        float freq = inertia_cutoff.get_last();
        // printf("freq=%g inr.cnt=%d timer.left=%d\n", freq, inertia_cutoff.count, timer.left);
        // XXXKF this is resonance of a single stage, obviously for three stages, resonant gain will be different
        float q = inertia_resonance.get_last();
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
        int inertia = dsp::fastf2i_drm(*params[par_inertia]);
        if (inertia != inertia_cutoff.ramp.length()) {
            inertia_cutoff.ramp.set_length(inertia);
            inertia_resonance.ramp.set_length(inertia);
        }
        
        right[0].copy_coeffs(left[0]);
        for (int i = 1; i < order; i++) {
            left[i].copy_coeffs(left[0]);
            right[i].copy_coeffs(left[0]);
        }
    }
    void params_changed()
    {
        inertia_cutoff.set_inertia(*params[par_cutoff]);
        inertia_resonance.set_inertia(*params[par_resonance]);
        calculate_filter();
    }
    void on_timer()
    {
        inertia_cutoff.step();
        inertia_resonance.step();
        calculate_filter();
    }
    void activate() {
        params_changed();
        for (int i=0; i < order; i++) {
            left[i].reset();
            right[i].reset();
        }
        timer = once_per_n(srate / 1000);
        timer.start();
    }
    void deactivate() {
    }
    void set_sample_rate(uint32_t sr) {
        srate = sr;
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
//        printf("sr=%d cutoff=%f res=%f mode=%f\n", srate, *params[par_cutoff], *params[par_resonance], *params[par_mode]);
        uint32_t ostate = 0;
        numsamples += offset;
        while(offset < numsamples) {
            uint32_t numnow = numsamples - offset;
            // if inertia's inactive, we can calculate the whole buffer at once
            if (inertia_cutoff.active() || inertia_resonance.active())
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

class vintage_delay_audio_module: public null_audio_module
{
public:    
    // 1MB of delay memory per channel... uh, RAM is cheap
    enum { MAX_DELAY = 262144, ADDR_MASK = MAX_DELAY - 1 };
    enum { par_bpm, par_divide, par_time_l, par_time_r, par_feedback, par_amount, par_mixmode, par_medium, param_count };
    enum { in_count = 2, out_count = 2, rt_capable = true, support_midi = false };
    float *ins[in_count]; 
    float *outs[out_count];
    float *params[param_count];
    static const char *port_names[];
    float buffers[2][MAX_DELAY];
    int bufptr, deltime_l, deltime_r, mixmode, medium, old_medium;
    gain_smoothing amt_left, amt_right, fb_left, fb_right;
    
    dsp::biquad<float> biquad_left[2], biquad_right[2];
    
    uint32_t srate;
    static parameter_properties param_props[];
    
    vintage_delay_audio_module()
    {
        old_medium = -1;
    }
    
    void params_changed()
    {
        float unit = 60.0 * srate / (*params[par_bpm] * *params[par_divide]);
        deltime_l = dsp::fastf2i_drm(unit * *params[par_time_l]);
        deltime_r = dsp::fastf2i_drm(unit * *params[par_time_r]);
        amt_left.set_inertia(*params[par_amount]); amt_right.set_inertia(*params[par_amount]);
        float fb = *params[par_feedback];;
        mixmode = dsp::fastf2i_drm(*params[par_mixmode]);
        medium = dsp::fastf2i_drm(*params[par_medium]);
        if (mixmode == 0)
        {
            fb_left.set_inertia(fb);
            fb_right.set_inertia(pow(fb, *params[par_time_r] / *params[par_time_l]));
        } else {
            fb_left.set_inertia(fb);
            fb_right.set_inertia(fb);
        }
        if (medium != old_medium)
            calc_filters();
    }
    void activate() {
        bufptr = 0;
    }
    void deactivate() {
    }
    void set_sample_rate(uint32_t sr) {
        srate = sr;
        old_medium = -1;
        amt_left.set_sample_rate(sr); amt_right.set_sample_rate(sr);
        fb_left.set_sample_rate(sr); fb_right.set_sample_rate(sr);
        params_changed();
    }
    void calc_filters()
    {
        // parameters are heavily influenced by gordonjcp and his tape delay unit
        // although, don't blame him if it sounds bad - I've messed with them too :)
        biquad_left[0].set_lp_rbj(6000, 0.707, srate);
        biquad_left[1].set_bp_rbj(4500, 0.250, srate);
        biquad_right[0].copy_coeffs(biquad_left[0]);
        biquad_right[1].copy_coeffs(biquad_left[1]);
    }
    uint32_t process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask) {
        uint32_t ostate = 3; // XXXKF optimize!
        uint32_t end = offset + numsamples;
        int v = mixmode ? 1 : 0;
        int orig_bufptr = bufptr;
        for(uint32_t i = offset; i < end; i++)
        {
            float in_left = buffers[v][(bufptr - deltime_l) & ADDR_MASK], in_right = buffers[1 - v][(bufptr - deltime_r) & ADDR_MASK], out_left, out_right, del_left, del_right;
            dsp::sanitize(in_left), dsp::sanitize(in_right);

            out_left = ins[0][i] + in_left * amt_left.get();
            out_right = ins[1][i] + in_right * amt_right.get();
            del_left = ins[0][i] + in_left * fb_left.get();
            del_right = ins[1][i] + in_right * fb_right.get();
            
            outs[0][i] = out_left; outs[1][i] = out_right; buffers[0][bufptr] = del_left; buffers[1][bufptr] = del_right;
            bufptr = (bufptr + 1) & (MAX_DELAY - 1);
        }
        if (medium > 0) {
            bufptr = orig_bufptr;
            if (medium == 2)
            {
                for(uint32_t i = offset; i < end; i++)
                {
                    buffers[0][bufptr] = biquad_left[0].process_d2_lp(biquad_left[1].process_d2(buffers[0][bufptr]));
                    buffers[1][bufptr] = biquad_right[0].process_d2_lp(biquad_right[1].process_d2(buffers[1][bufptr]));
                    bufptr = (bufptr + 1) & (MAX_DELAY - 1);
                }
                biquad_left[0].sanitize_d2();biquad_right[0].sanitize_d2();
            } else {
                for(uint32_t i = offset; i < end; i++)
                {
                    buffers[0][bufptr] = biquad_left[1].process_d2(buffers[0][bufptr]);
                    buffers[1][bufptr] = biquad_right[1].process_d2(buffers[1][bufptr]);
                    bufptr = (bufptr + 1) & (MAX_DELAY - 1);
                }
            }
            biquad_left[1].sanitize_d2();biquad_right[1].sanitize_d2();
            
        }
        return ostate;
    }
};

extern std::string get_builtin_modules_rdf();

};

#include "modules_synths.h"

#endif
