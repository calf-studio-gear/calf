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
    inline void params_reset() {}
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
    static const char *get_name() { return "amp"; }
    static const char *get_id() { return "amp"; }
    static const char *get_label() { return "Amp"; }
};

class flanger_audio_module: public null_audio_module
{
public:
    enum { par_delay, par_depth, par_rate, par_fb, par_stereo, par_reset, par_amount, param_count };
    enum { in_count = 2, out_count = 2, support_midi = false, rt_capable = true };
    static const char *port_names[];
    dsp::simple_flanger<float, 2048> left, right;
    float *ins[in_count]; 
    float *outs[out_count];
    float *params[param_count];
    uint32_t srate;
    bool clear_reset;
    float last_r_phase;
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
        float r_phase = *params[par_stereo] * (1.f / 360.f);
        clear_reset = false;
        if (*params[par_reset] >= 0.5) {
            clear_reset = true;
            left.reset_phase(0.f);
            right.reset_phase(r_phase);
        } else {
            if (fabs(r_phase - last_r_phase) > 0.0001f) {
                right.phase = left.phase;
                right.inc_phase(r_phase);
                last_r_phase = r_phase;
            }
        }
    }
    void params_reset()
    {
        if (clear_reset) {
            *params[par_reset] = 0.f;
            clear_reset = false;
        }
    }
    void activate() {
        left.reset();
        right.reset();
        last_r_phase = *params[par_stereo] * (1.f / 360.f);
        left.reset_phase(0.f);
        right.reset_phase(last_r_phase);
    }
    void deactivate() {
    }
    uint32_t process(uint32_t offset, uint32_t nsamples, uint32_t inputs_mask, uint32_t outputs_mask) {
        left.process(outs[0] + offset, ins[0] + offset, nsamples);
        right.process(outs[1] + offset, ins[1] + offset, nsamples);
        return outputs_mask; // XXXKF allow some delay after input going blank
    }
    static const char *get_name() { return "flanger"; }
    static const char *get_id() { return "flanger"; }
    static const char *get_label() { return "Flanger"; }
};

class phaser_audio_module: public null_audio_module
{
public:
    enum { par_freq, par_depth, par_rate, par_fb, par_stages, par_stereo, par_reset, par_amount, param_count };
    enum { in_count = 2, out_count = 2, support_midi = false, rt_capable = true };
    static const char *port_names[];
    float *ins[in_count]; 
    float *outs[out_count];
    float *params[param_count];
    uint32_t srate;
    bool clear_reset;
    float last_r_phase;
    dsp::simple_phaser<12> left, right;
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
        float base_frq = *params[par_freq];
        float mod_depth = *params[par_depth];
        float fb = *params[par_fb];
        int stages = (int)*params[par_stages];
        left.set_dry(dry); right.set_dry(dry);
        left.set_wet(wet); right.set_wet(wet);
        left.set_rate(rate); right.set_rate(rate);
        left.set_base_frq(base_frq); right.set_base_frq(base_frq);
        left.set_mod_depth(mod_depth); right.set_mod_depth(mod_depth);
        left.set_fb(fb); right.set_fb(fb);
        left.set_stages(stages); right.set_stages(stages);
        float r_phase = *params[par_stereo] * (1.f / 360.f);
        clear_reset = false;
        if (*params[par_reset] >= 0.5) {
            clear_reset = true;
            left.reset_phase(0.f);
            right.reset_phase(r_phase);
        } else {
            if (fabs(r_phase - last_r_phase) > 0.0001f) {
                right.phase = left.phase;
                right.inc_phase(r_phase);
                last_r_phase = r_phase;
            }
        }
    }
    void params_reset()
    {
        if (clear_reset) {
            *params[par_reset] = 0.f;
            clear_reset = false;
        }
    }
    void activate() {
        left.reset();
        right.reset();
        last_r_phase = *params[par_stereo] * (1.f / 360.f);
        left.reset_phase(0.f);
        right.reset_phase(last_r_phase);
    }
    void deactivate() {
    }
    uint32_t process(uint32_t offset, uint32_t nsamples, uint32_t inputs_mask, uint32_t outputs_mask) {
        left.process(outs[0] + offset, ins[0] + offset, nsamples);
        right.process(outs[1] + offset, ins[1] + offset, nsamples);
        return outputs_mask; // XXXKF allow some delay after input going blank
    }
    static const char *get_name() { return "phaser"; }
    static const char *get_id() { return "phaser"; }
    static const char *get_label() { return "Phaser"; }
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
    static const char *get_name() { return "reverb"; }
    static const char *get_id() { return "reverb"; }
    static const char *get_label() { return "Reverb"; }
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
    static const char *get_id() { return "filter"; }
    static const char *get_name() { return "filter"; }
    static const char *get_label() { return "Filter"; }
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
    static const char *get_name() { return "vintage_delay"; }
    static const char *get_id() { return "vintagedelay"; }
    static const char *get_label() { return "Vintage Delay"; }
};

class rotary_speaker_audio_module: public null_audio_module
{
public:
    enum { par_speed, param_count };
    enum { in_count = 2, out_count = 2, support_midi = true, rt_capable = true };
    static const char *port_names[];
    float *ins[in_count]; 
    float *outs[out_count];
    float *params[param_count];
    double phase_l, dphase_l, phase_h, dphase_h;
    int cos_l, sin_l, cos_h, sin_h;
    dsp::simple_delay<4096, float> delay;
    dsp::biquad<float> crossover1l, crossover1r, crossover2l, crossover2r;
    dsp::simple_delay<8, float> phaseshift;
    uint32_t srate;
    int vibrato_mode;
    static parameter_properties param_props[];
    float mwhl_value, hold_value, aspeed_l, aspeed_h, dspeed;

    rotary_speaker_audio_module()
    {
        mwhl_value = hold_value = 0.f;
        phase_h = phase_l = 0.f;
        aspeed_l = 1.f;
        aspeed_h = 1.f;
        dspeed = 0.f;
    }    
    void set_sample_rate(uint32_t sr) {
        srate = sr;
        setup();
    }
    void setup()
    {
        crossover1l.set_lp_rbj(800.f, 0.7, (float)srate);
        crossover1r.set_lp_rbj(800.f, 0.7, (float)srate);
        crossover2l.set_hp_rbj(800.f, 0.7, (float)srate);
        crossover2r.set_hp_rbj(800.f, 0.7, (float)srate);
        set_vibrato();
    }
    void params_changed() {
        set_vibrato();
    }
    void activate() {
        phase_h = phase_l = 0.f;
        setup();
    }
    void deactivate() {
    }
    void set_vibrato()
    {
        vibrato_mode = fastf2i_drm(*params[par_speed]);
        if (!vibrato_mode)
            dspeed = -1;
        else {
            float speed = vibrato_mode - 1;
            if (vibrato_mode == 3)
                speed = hold_value;
            if (vibrato_mode == 4)
                speed = mwhl_value;
            dspeed = (speed < 0.5f) ? 0 : 1;
        }
        update_speed();
    }
    void update_speed()
    {
        float speed_h = aspeed_h >= 0 ? (48 + (400-48) * aspeed_h) : (48 * (1 + aspeed_h));
        float speed_l = aspeed_l >= 0 ? 40 + (342-40) * aspeed_l : (40 * (1 + aspeed_l));
        dphase_h = speed_h / (60 * srate);
        dphase_l = speed_l / (60 * srate);
        cos_h = (int)(16384*16384*cos(dphase_h * 2 * PI));
        sin_h = (int)(16384*16384*sin(dphase_h * 2 * PI));
        cos_l = (int)(16384*16384*cos(dphase_l * 2 * PI));
        sin_l = (int)(16384*16384*sin(dphase_l * 2 * PI));
    }
    static inline void update_euler(long long int &x, long long int &y, int dx, int dy)
    {
        long long int nx = (x * dx - y * dy) >> 28;
        long long int ny = (x * dy + y * dx) >> 28;
        x = nx;
        y = ny;
    }
    inline bool update_speed(float &aspeed, float delta_decc, float delta_acc)
    {
        if (aspeed < dspeed) {
            aspeed = min(dspeed, aspeed + delta_acc);
            return true;
        }
        else if (aspeed > dspeed) 
        {
            aspeed = max(dspeed, aspeed - delta_decc);
            return true;
        }        
        return false;
    }
    uint32_t process(uint32_t offset, uint32_t nsamples, uint32_t inputs_mask, uint32_t outputs_mask)
    {
        long long int xl0 = (int)(10000*16384*cos(phase_l * 2 * PI));
        long long int yl0 = (int)(10000*16384*sin(phase_l * 2 * PI));
        long long int xh0 = (int)(10000*16384*cos(phase_h * 2 * PI));
        long long int yh0 = (int)(10000*16384*sin(phase_h * 2 * PI));
        // printf("xl=%d yl=%d dx=%d dy=%d\n", (int)(xl0>>14), (int)(yl0 >> 14), cos_l, sin_l);
        for (unsigned int i = 0; i < nsamples; i++) {
            float in_l = ins[0][i + offset], in_r = ins[1][i + offset];
            float in_mono = 0.5f * (in_l + in_r);
            
            // int xl = (int)(10000 * cos(phase_l)), yl = (int)(10000 * sin(phase_l));
            //int xh = (int)(10000 * cos(phase_h)), yh = (int)(10000 * sin(phase_h));
            int xl = xl0 >> 14, yl = yl0 >> 14;
            int xh = xh0 >> 14, yh = yh0 >> 14;
            update_euler(xl0, yl0, cos_l, sin_l);
            // printf("xl=%d yl=%d xl'=%f yl'=%f\n", xl, yl, 16384*cos((phase_l + dphase_l * i) * 2 * PI), 16384*sin((phase_l + dphase_l * i) * 2 * PI));
            update_euler(xh0, yh0, cos_h, sin_h);
            
            float out_hi_l = delay.get_interp_1616(500000 + 40 * xh) + 0.0001 * xh * delay.get_interp_1616(650000 - 40 * yh) - delay.get_interp_1616(800000 - 60 * xh);
            float out_hi_r = delay.get_interp_1616(550000 - 48 * yh) - 0.0001 * yh * delay.get_interp_1616(700000 + 46 * xh) + delay.get_interp_1616(1000000 + 76 * yh);

            float out_lo_l = 0.5f * in_mono - delay.get_interp_1616(400000 + 34 * xl) + delay.get_interp_1616(650000 - 18 * yl);
            float out_lo_r = 0.5f * in_mono + delay.get_interp_1616(600000 - 50 * xl) - delay.get_interp_1616(900000 + 15 * yl);
            
            out_hi_l = crossover2l.process_d2(out_hi_l); // sanitize(out_hi_l);
            out_hi_r = crossover2r.process_d2(out_hi_r); // sanitize(out_hi_r);
            out_lo_l = crossover1l.process_d2(out_lo_l); // sanitize(out_lo_l);
            out_lo_r = crossover1r.process_d2(out_lo_r); // sanitize(out_lo_r);
            
            float out_l = out_hi_l + out_lo_l;
            float out_r = out_hi_r + out_lo_r;
            
            in_mono += 0.06f * (out_l + out_r);
            sanitize(in_mono);
            
            outs[0][i + offset] = out_l * 0.5f;
            outs[1][i + offset] = out_r * 0.5f;
            delay.put(in_mono);
        }
        crossover1l.sanitize_d2();
        crossover1r.sanitize_d2();
        crossover2l.sanitize_d2();
        crossover2r.sanitize_d2();
        phase_l = fmod(phase_l + nsamples * dphase_l, 1.0);
        phase_h = fmod(phase_h + nsamples * dphase_h, 1.0);
        float delta = nsamples * 1.0 / srate;
        bool u1 = update_speed(aspeed_l, delta * 0.2, delta * 0.14);
        bool u2 = update_speed(aspeed_h, delta, delta * 0.5);
        if (u1 || u2)
            set_vibrato();
        return outputs_mask;
    }
    virtual void control_change(int ctl, int val)
    {
        if (vibrato_mode == 3 && ctl == 64)
        {
            hold_value = val / 127.f;
            set_vibrato();
            return;
        }
        if (vibrato_mode == 4 && ctl == 1)
        {
            mwhl_value = val / 127.f;
            set_vibrato();
            return;
        }
    }
    static const char *get_name() { return "rotary_speaker"; }
    static const char *get_id() { return "rotaryspeaker"; }
    static const char *get_label() { return "Rotary Speaker"; }
};

extern std::string get_builtin_modules_rdf();

};

#include "modules_synths.h"

#endif
