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
#include "multichorus.h"
#include "giface.h"
#include "metadata.h"
#include "loudness.h"
#include "primitives.h"

namespace calf_plugins {

using namespace dsp;

struct ladspa_plugin_info;
    
#if 0
class amp_audio_module: public null_audio_module
{
public:
    enum { in_count = 2, out_count = 2, param_count = 1, support_midi = false, require_midi = false, rt_capable = true };
    float *ins[2]; 
    float *outs[2];
    float *params[1];
    uint32_t srate;
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
#endif

class flanger_audio_module: public audio_module<flanger_metadata>, public line_graph_iface
{
public:
    dsp::simple_flanger<float, 2048> left, right;
    float *ins[in_count]; 
    float *outs[out_count];
    float *params[param_count];
    uint32_t srate;
    bool clear_reset;
    float last_r_phase;
    bool is_active;
public:
    flanger_audio_module() {
        is_active = false;
    }
    void set_sample_rate(uint32_t sr);
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
    void activate();
    void deactivate();
    uint32_t process(uint32_t offset, uint32_t nsamples, uint32_t inputs_mask, uint32_t outputs_mask) {
        left.process(outs[0] + offset, ins[0] + offset, nsamples);
        right.process(outs[1] + offset, ins[1] + offset, nsamples);
        return outputs_mask; // XXXKF allow some delay after input going blank
    }
    bool get_graph(int index, int subindex, float *data, int points, cairo_iface *context);
    bool get_gridline(int index, int subindex, float &pos, bool &vertical, std::string &legend, cairo_iface *context);
    float freq_gain(int subindex, float freq, float srate);
};

class phaser_audio_module: public audio_module<phaser_metadata>
{
public:
    float *ins[in_count]; 
    float *outs[out_count];
    float *params[param_count];
    uint32_t srate;
    bool clear_reset;
    float last_r_phase;
    dsp::simple_phaser<12> left, right;
    bool is_active;
public:
    phaser_audio_module() {
        is_active = false;
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
    void activate();
    void set_sample_rate(uint32_t sr);
    void deactivate();
    uint32_t process(uint32_t offset, uint32_t nsamples, uint32_t inputs_mask, uint32_t outputs_mask) {
        left.process(outs[0] + offset, ins[0] + offset, nsamples);
        right.process(outs[1] + offset, ins[1] + offset, nsamples);
        return outputs_mask; // XXXKF allow some delay after input going blank
    }
};

class reverb_audio_module: public audio_module<reverb_metadata>
{
public:    
    dsp::reverb<float> reverb;
    uint32_t srate;
    gain_smoothing amount, dryamount;
    float *ins[in_count]; 
    float *outs[out_count];
    float *params[param_count];
    
    void params_changed() {
        //reverb.set_time(0.5*pow(8.0f, *params[par_decay]));
        //reverb.set_cutoff(2000*pow(10.0f, *params[par_hfdamp]));
        reverb.set_type_and_diffusion(fastf2i_drm(*params[par_roomsize]), *params[par_diffusion]);
        reverb.set_time(*params[par_decay]);
        reverb.set_cutoff(*params[par_hfdamp]);
        amount.set_inertia(*params[par_amount]);
        dryamount.set_inertia(*params[par_dry]);
    }
    uint32_t process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask) {
        numsamples += offset;
        for (uint32_t i = offset; i < numsamples; i++) {
            float dry = dryamount.get();
            float wet = amount.get();
            float l = ins[0][i], r = ins[1][i];
            float rl = l, rr = r;
            reverb.process(rl, rr);
            outs[0][i] = dry*l + wet*rl;
            outs[1][i] = dry*r + wet*rr;
        }
        reverb.extra_sanitize();
        return outputs_mask;
    }
    void activate();
    void set_sample_rate(uint32_t sr);
    void deactivate();
};

class filter_audio_module: public audio_module<filter_metadata>, public line_graph_iface
{
public:    
    float *ins[in_count]; 
    float *outs[out_count];
    float *params[param_count];
    dsp::biquad_d1<float> left[3], right[3];
    uint32_t srate;
    int order;
    inertia<exponential_ramp> inertia_cutoff, inertia_resonance;
    once_per_n timer;
    bool is_active;
    
    filter_audio_module()
    : inertia_cutoff(exponential_ramp(128), 20)
    , inertia_resonance(exponential_ramp(128), 20)
    , timer(128)
    {
        order = 0;
        is_active = false;
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
            order = mode + 1;
            left[0].set_lp_rbj(freq, pow(q, 1.0 / order), srate);
        } else {
            order = mode - 2;
            left[0].set_hp_rbj(freq, pow(q, 1.0 / order), srate);
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
    void activate();
    void set_sample_rate(uint32_t sr);
    void deactivate();
    inline int process_channel(dsp::biquad_d1<float> *filter, float *in, float *out, uint32_t numsamples, int inmask) {
        if (inmask) {
            switch(order) {
                case 1:
                    for (uint32_t i = 0; i < numsamples; i++)
                        out[i] = filter[0].process(in[i]);
                    break;
                case 2:
                    for (uint32_t i = 0; i < numsamples; i++)
                        out[i] = filter[1].process(filter[0].process(in[i]));
                    break;
                case 3:
                    for (uint32_t i = 0; i < numsamples; i++)
                        out[i] = filter[2].process(filter[1].process(filter[0].process(in[i])));
                    break;
            }
        } else {
            if (filter[order - 1].empty())
                return 0;
            switch(order) {
                case 1:
                    for (uint32_t i = 0; i < numsamples; i++)
                        out[i] = filter[0].process_zeroin();
                    break;
                case 2:
                    if (filter[0].empty())
                        for (uint32_t i = 0; i < numsamples; i++)
                            out[i] = filter[1].process_zeroin();
                    else
                        for (uint32_t i = 0; i < numsamples; i++)
                            out[i] = filter[1].process(filter[0].process_zeroin());
                    break;
                case 3:
                    if (filter[1].empty())
                        for (uint32_t i = 0; i < numsamples; i++)
                            out[i] = filter[2].process_zeroin();
                    else
                        for (uint32_t i = 0; i < numsamples; i++)
                            out[i] = filter[2].process(filter[1].process(filter[0].process_zeroin()));
                    break;
            }
        }
        for (int i = 0; i < order; i++)
            filter[i].sanitize();
        return filter[order - 1].empty() ? 0 : inmask;
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
    bool get_graph(int index, int subindex, float *data, int points, cairo_iface *context);
    float freq_gain(int subindex, float freq, float srate);
    bool get_gridline(int index, int subindex, float &pos, bool &vertical, std::string &legend, cairo_iface *context);
};

class vintage_delay_audio_module: public audio_module<vintage_delay_metadata>
{
public:    
    // 1MB of delay memory per channel... uh, RAM is cheap
    enum { MAX_DELAY = 262144, ADDR_MASK = MAX_DELAY - 1 };
    float *ins[in_count]; 
    float *outs[out_count];
    float *params[param_count];
    float buffers[2][MAX_DELAY];
    int bufptr, deltime_l, deltime_r, mixmode, medium, old_medium;
    gain_smoothing amt_left, amt_right, fb_left, fb_right;
    
    dsp::biquad_d2<float> biquad_left[2], biquad_right[2];
    
    uint32_t srate;
    
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
                    buffers[0][bufptr] = biquad_left[0].process_lp(biquad_left[1].process(buffers[0][bufptr]));
                    buffers[1][bufptr] = biquad_right[0].process_lp(biquad_right[1].process(buffers[1][bufptr]));
                    bufptr = (bufptr + 1) & (MAX_DELAY - 1);
                }
                biquad_left[0].sanitize();biquad_right[0].sanitize();
            } else {
                for(uint32_t i = offset; i < end; i++)
                {
                    buffers[0][bufptr] = biquad_left[1].process(buffers[0][bufptr]);
                    buffers[1][bufptr] = biquad_right[1].process(buffers[1][bufptr]);
                    bufptr = (bufptr + 1) & (MAX_DELAY - 1);
                }
            }
            biquad_left[1].sanitize();biquad_right[1].sanitize();
            
        }
        return ostate;
    }
};

class rotary_speaker_audio_module: public audio_module<rotary_speaker_metadata>
{
public:
    float *ins[in_count]; 
    float *outs[out_count];
    float *params[param_count];
    /// Current phases and phase deltas for bass and treble rotors
    uint32_t phase_l, dphase_l, phase_h, dphase_h;
    dsp::simple_delay<1024, float> delay;
    dsp::biquad_d2<float> crossover1l, crossover1r, crossover2l, crossover2r;
    dsp::simple_delay<8, float> phaseshift;
    uint32_t srate;
    int vibrato_mode;
    /// Current CC1 (Modulation) value, normalized to [0, 1]
    float mwhl_value;
    /// Current CC64 (Hold) value, normalized to [0, 1]
    float hold_value;
    /// Current rotation speed for bass rotor - automatic mode
    float aspeed_l;
    /// Current rotation speed for treble rotor - automatic mode
    float aspeed_h;
    /// Desired speed (0=slow, 1=fast) - automatic mode
    float dspeed;
    /// Current rotation speed for bass rotor - manual mode
    float maspeed_l;
    /// Current rotation speed for treble rotor - manual mode
    float maspeed_h;

    rotary_speaker_audio_module();
    void set_sample_rate(uint32_t sr);
    void setup();
    void activate();
    void deactivate();
    
    void params_changed() {
        set_vibrato();
    }
    void set_vibrato()
    {
        vibrato_mode = fastf2i_drm(*params[par_speed]);
        // manual vibrato - do not recalculate speeds as they're not used anyway
        if (vibrato_mode == 5) 
            return;
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
    /// Convert RPM speed to delta-phase
    inline uint32_t rpm2dphase(float rpm)
    {
        return (uint32_t)((rpm / (60.0 * srate)) * (1 << 30)) << 2;
    }
    /// Set delta-phase variables based on current calculated (and interpolated) RPM speed
    void update_speed()
    {
        float speed_h = aspeed_h >= 0 ? (48 + (400-48) * aspeed_h) : (48 * (1 + aspeed_h));
        float speed_l = aspeed_l >= 0 ? 40 + (342-40) * aspeed_l : (40 * (1 + aspeed_l));
        dphase_h = rpm2dphase(speed_h);
        dphase_l = rpm2dphase(speed_l);
    }
    void update_speed_manual(float delta)
    {
        float ts = *params[par_treblespeed];
        float bs = *params[par_bassspeed];
        incr_towards(maspeed_h, ts, delta * 200, delta * 200);
        incr_towards(maspeed_l, bs, delta * 200, delta * 200);
        dphase_h = rpm2dphase(maspeed_h);
        dphase_l = rpm2dphase(maspeed_l);
    }
    /// map a ramp [int] to a sinusoid-like function [0, 65536]
    static inline int pseudo_sine_scl(int counter)
    {
        // premature optimization is a root of all evil; it can be done with integers only - but later :)
        double v = counter * (1.0 / (65536.0 * 32768.0));
        return 32768 + 32768 * (v - v*v*v) * (1.0 / 0.3849);
    }
    /// Increase or decrease aspeed towards raspeed, with required negative and positive rate
    inline bool incr_towards(float &aspeed, float raspeed, float delta_decc, float delta_acc)
    {
        if (aspeed < raspeed) {
            aspeed = std::min(raspeed, aspeed + delta_acc);
            return true;
        }
        else if (aspeed > raspeed) 
        {
            aspeed = std::max(raspeed, aspeed - delta_decc);
            return true;
        }        
        return false;
    }
    uint32_t process(uint32_t offset, uint32_t nsamples, uint32_t inputs_mask, uint32_t outputs_mask)
    {
        int shift = (int)(300000 * (*params[par_shift])), pdelta = (int)(300000 * (*params[par_spacing]));
        int md = (int)(100 * (*params[par_moddepth]));
        float mix = 0.5 * (1.0 - *params[par_micdistance]);
        float mix2 = *params[par_reflection];
        float mix3 = mix2 * mix2;
        for (unsigned int i = 0; i < nsamples; i++) {
            float in_l = ins[0][i + offset], in_r = ins[1][i + offset];
            float in_mono = 0.5f * (in_l + in_r);
            
            int xl = pseudo_sine_scl(phase_l), yl = pseudo_sine_scl(phase_l + 0x40000000);
            int xh = pseudo_sine_scl(phase_h), yh = pseudo_sine_scl(phase_h + 0x40000000);
            // printf("%d %d %d\n", shift, pdelta, shift + pdelta + 20 * xl);
            
            // float out_hi_l = in_mono - delay.get_interp_1616(shift + md * xh) + delay.get_interp_1616(shift + md * 65536 + pdelta - md * yh) - delay.get_interp_1616(shift + md * 65536 + pdelta + pdelta - md * xh);
            // float out_hi_r = in_mono + delay.get_interp_1616(shift + md * 65536 - md * yh) - delay.get_interp_1616(shift + pdelta + md * xh) + delay.get_interp_1616(shift + pdelta + pdelta + md * yh);
            float out_hi_l = in_mono + delay.get_interp_1616(shift + md * xh) - mix2 * delay.get_interp_1616(shift + md * 65536 + pdelta - md * yh) + mix3 * delay.get_interp_1616(shift + md * 65536 + pdelta + pdelta - md * xh);
            float out_hi_r = in_mono + delay.get_interp_1616(shift + md * 65536 - md * yh) - mix2 * delay.get_interp_1616(shift + pdelta + md * xh) + mix3 * delay.get_interp_1616(shift + pdelta + pdelta + md * yh);

            float out_lo_l = in_mono + delay.get_interp_1616(shift + md * xl); // + delay.get_interp_1616(shift + md * 65536 + pdelta - md * yl);
            float out_lo_r = in_mono + delay.get_interp_1616(shift + md * yl); // - delay.get_interp_1616(shift + pdelta + md * yl);
            
            out_hi_l = crossover2l.process(out_hi_l); // sanitize(out_hi_l);
            out_hi_r = crossover2r.process(out_hi_r); // sanitize(out_hi_r);
            out_lo_l = crossover1l.process(out_lo_l); // sanitize(out_lo_l);
            out_lo_r = crossover1r.process(out_lo_r); // sanitize(out_lo_r);
            
            float out_l = out_hi_l + out_lo_l;
            float out_r = out_hi_r + out_lo_r;
            
            float mic_l = out_l + mix * (out_r - out_l);
            float mic_r = out_r + mix * (out_l - out_r);
            
            outs[0][i + offset] = mic_l * 0.5f;
            outs[1][i + offset] = mic_r * 0.5f;
            delay.put(in_mono);
            phase_l += dphase_l;
            phase_h += dphase_h;
        }
        crossover1l.sanitize();
        crossover1r.sanitize();
        crossover2l.sanitize();
        crossover2r.sanitize();
        float delta = nsamples * 1.0 / srate;
        if (vibrato_mode == 5)
            update_speed_manual(delta);
        else
        {
            bool u1 = incr_towards(aspeed_l, dspeed, delta * 0.2, delta * 0.14);
            bool u2 = incr_towards(aspeed_h, dspeed, delta, delta * 0.5);
            if (u1 || u2)
                set_vibrato();
        }
        return outputs_mask;
    }
    virtual void control_change(int ctl, int val);
};

/// A multitap stereo chorus thing - processing
class multichorus_audio_module: public audio_module<multichorus_metadata>, public line_graph_iface
{
public:
    float *ins[in_count]; 
    float *outs[out_count];
    float *params[param_count];
    uint32_t srate;
    dsp::multichorus<float, sine_multi_lfo<float, 8>, 4096> left, right;
    float last_r_phase;
    bool is_active;
    
public:    
    multichorus_audio_module()
    {
        is_active = false;
    }
    
    void params_changed()
    {
        // delicious copy-pasta from flanger module - it'd be better to keep it common or something
        float dry = 1.0;
        float wet = *params[par_amount];
        float rate = *params[par_rate];
        float min_delay = *params[par_delay] / 1000.0;
        float mod_depth = *params[par_depth] / 1000.0;
        left.set_dry(dry); right.set_dry(dry);
        left.set_wet(wet); right.set_wet(wet);
        left.set_rate(rate); right.set_rate(rate);
        left.set_min_delay(min_delay); right.set_min_delay(min_delay);
        left.set_mod_depth(mod_depth); right.set_mod_depth(mod_depth);
        int voices = (int)*params[par_voices];
        left.lfo.set_voices(voices); right.lfo.set_voices(voices);
        float vphase = *params[par_vphase] * (1.f / 360.f);
        left.lfo.vphase = right.lfo.vphase = vphase * (4096 / std::max(voices - 1, 1));
        float r_phase = *params[par_stereo] * (1.f / 360.f);
        if (fabs(r_phase - last_r_phase) > 0.0001f) {
            right.lfo.phase = left.lfo.phase;
            right.lfo.phase += chorus_phase(r_phase * 4096);
            last_r_phase = r_phase;
        }
    }
    uint32_t process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask) {
        left.process(outs[0] + offset, ins[0] + offset, numsamples);
        right.process(outs[1] + offset, ins[1] + offset, numsamples);
        if (params[par_lfophase_l])
            *params[par_lfophase_l] = (double)left.lfo.phase * 360.0 / 4096.0;
        if (params[par_lfophase_r])
            *params[par_lfophase_r] = (double)right.lfo.phase * 360.0 / 4096.0;
        return outputs_mask; // XXXKF allow some delay after input going blank
    }
    void activate();
    void deactivate();
    void set_sample_rate(uint32_t sr);
    bool get_graph(int index, int subindex, float *data, int points, cairo_iface *context);
    float freq_gain(int subindex, float freq, float srate);
    bool get_dot(int index, int subindex, float &x, float &y, int &size, cairo_iface *context);
    bool get_gridline(int index, int subindex, float &pos, bool &vertical, std::string &legend, cairo_iface *context);
};

class compressor_audio_module: public audio_module<compressor_metadata>, public line_graph_iface {
private:
    float linSlope, peak, detected, kneeSqrt, kneeStart, linKneeStart, kneeStop, threshold, ratio, knee, makeup, compressedKneeStop, adjKneeStart;
    uint32_t clip;
    aweighter awL, awR;
public:
    float *ins[in_count];
    float *outs[out_count];
    float *params[param_count];
    uint32_t srate;
    bool is_active;
    compressor_audio_module();
    void activate();
    void deactivate();
    uint32_t process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask) {
        bool bypass = *params[param_bypass] > 0.5f;
        
        if(bypass) {
            int count = numsamples * sizeof(float);
            memcpy(outs[0], ins[0], count);
            memcpy(outs[1], ins[1], count);

            if(params[param_compression] != NULL) {
                *params[param_compression] = 1.f;
            }

            if(params[param_clip] != NULL) {
                *params[param_clip] = 0.f;
            }

            if(params[param_peak] != NULL) {
                *params[param_peak] = 0.f;
            }
      
            return inputs_mask;
        }

        bool rms = *params[param_detection] == 0;
        bool average = *params[param_stereo_link] == 0;
        float aweighting = *params[param_aweighting] > 0.5f;
        float linThreshold = *params[param_threshold];
        ratio = *params[param_ratio];
        float attack = *params[param_attack];
        float attack_coeff = std::min(1.f, 1.f / (attack * srate / 4000.f));
        float release = *params[param_release];
        float release_coeff = std::min(1.f, 1.f / (release * srate / 4000.f));
        makeup = *params[param_makeup];
        knee = *params[param_knee];

        float linKneeSqrt = sqrt(knee);
        linKneeStart = linThreshold / linKneeSqrt;
        adjKneeStart = linKneeStart*linKneeStart;
        float linKneeStop = linThreshold * linKneeSqrt;
        
        threshold = log(linThreshold);
        kneeStart = log(linKneeStart);
        kneeStop = log(linKneeStop);
        compressedKneeStop = (kneeStop - threshold) / ratio + threshold;

        numsamples += offset;
        
        float compression = 1.f;

        peak -= peak * 5.f * numsamples / srate;
        
        clip -= std::min(clip, numsamples);

        while(offset < numsamples) {
            float left = ins[0][offset];
            float right = ins[1][offset];
            
            if(aweighting) {
                left = awL.process(left);
                right = awR.process(right);
            }
            
            float absample = average ? (fabs(left) + fabs(right)) * 0.5f : std::max(fabs(left), fabs(right));
            if(rms) absample *= absample;
            
            linSlope += (absample - linSlope) * (absample > linSlope ? attack_coeff : release_coeff);
            
            float gain = 1.f;

            if(linSlope > 0.f) {
                gain = output_gain(linSlope, rms);
            }

            compression = gain;
            gain *= makeup;

            float outL = ins[0][offset] * gain;
            float outR = ins[1][offset] * gain;
            
            outs[0][offset] = outL;
            outs[1][offset] = outR;
            
            ++offset;
            
            float maxLR = std::max(fabs(outL), fabs(outR));
            
            if(maxLR > 1.f) clip = srate >> 3; /* blink clip LED for 125 ms */
            
            if(maxLR > peak) {
                peak = maxLR;
            }
        }
        
        detected = rms ? sqrt(linSlope) : linSlope;
        
        if(params[param_compression] != NULL) {
            *params[param_compression] = compression;
        }

        if(params[param_clip] != NULL) {
            *params[param_clip] = clip;
        }

        if(params[param_peak] != NULL) {
            *params[param_peak] = peak;
        }

        return inputs_mask;
    }

    inline float output_level(float slope) {
        return slope * output_gain(slope, false) * makeup;
    }
    
    inline float output_gain(float linSlope, bool rms) {
         if(linSlope > rms ? adjKneeStart : linKneeStart) {
            float slope = log(linSlope);
            if(rms) slope *= 0.5f;

            float gain = 0.f;
            float delta = 0.f;
            if(IS_FAKE_INFINITY(ratio)) {
                gain = threshold;
                delta = 0.f;
            } else {
                gain = (slope - threshold) / ratio + threshold;
                delta = 1.f / ratio;
            }
            
            if(knee > 1.f && slope < kneeStop) {
                gain = hermite_interpolation(slope, kneeStart, kneeStop, kneeStart, compressedKneeStop, 1.f, delta);
            }
            
            return exp(gain - slope);
        }

        return 1.f;
    }

    void set_sample_rate(uint32_t sr);
    
    virtual bool get_graph(int index, int subindex, float *data, int points, cairo_iface *context);
    virtual bool get_dot(int index, int subindex, float &x, float &y, int &size, cairo_iface *context);
    virtual bool get_gridline(int index, int subindex, float &pos, bool &vertical, std::string &legend, cairo_iface *context);
};

extern std::string get_builtin_modules_rdf();

};

#include "modules_synths.h"

#endif
