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

namespace synth {

using namespace dsp;

#define PLUGIN_NAME_ID_LABEL(name, id, label) \
    static const char *get_name() { return name; } \
    static const char *get_id() { return id; } \
    static const char *get_label() { return label; } \
    
class null_audio_module;
struct ladspa_plugin_info;
    
/// Empty implementations for plugin functions. Note, that functions aren't virtual, because they're called via the particular
/// subclass (flanger_audio_module etc) via template wrappers (ladspa_wrapper<> etc), not via base class pointer/reference
class null_audio_module: public line_graph_iface
{
public:
    /// Handle MIDI Note On
    inline void note_on(int note, int velocity) {}
    /// Handle MIDI Note Off
    inline void note_off(int note, int velocity) {}
    /// Handle MIDI Program Change
    inline void program_change(int program) {}
    /// Handle MIDI Control Change
    inline void control_change(int controller, int value) {}
    /// Handle MIDI Pitch Bend
    /// @param value pitch bend value (-8192 to 8191, defined as in MIDI ie. 8191 = 200 ct by default)
    inline void pitch_bend(int value) {}
    /// Called when params are changed (before processing)
    inline void params_changed() {}
    /// LADSPA-esque activate function, except it is called after ports are connected, not before
    inline void activate() {}
    /// LADSPA-esque deactivate function
    inline void deactivate() {}
    /// Set sample rate for the plugin
    inline void set_sample_rate(uint32_t sr) { }
    /// Get "live" graph
    inline bool get_graph(int index, int subindex, float *data, int points, cairo_t *context) { return false; }
    inline static const char *get_gui_xml() { return NULL; }
    /// Get "static" graph (one that is dependent on single parameter value and doesn't use other parameters or plugin internal state).
    /// Used by waveform graphs.
    inline static bool get_static_graph(int index, int subindex, float value, float *data, int points, cairo_t *context) { return false; }
    /// Return the NULL-terminated list of menu commands
    inline static plugin_command_info *get_commands() { return NULL; }
    /// does parameter number represent a CV port? (yes by default, except for synths)
    static bool is_cv(int param_no) { return true; }
    /// does parameter change cause an audible click?
    static bool is_noisy(int param_no) { return false; }
    /// Execute menu command with given number
    inline void execute(int cmd_no) {}
    /// DSSI configure call
    inline char *configure(const char *key, const char *value) { return NULL; }
    /// Send all understood configure vars
    inline void send_configures(send_configure_iface *sci) {}
    /// Get all configure vars that are supposed to be set to initialize a preset
    static inline const char **get_default_configure_vars() { return NULL; }
    /// Reset parameter values for epp:trigger type parameters (ones activated by oneshot push button instead of check box)
    inline void params_reset() {}
};

class amp_audio_module: public null_audio_module
{
public:
    enum { in_count = 2, out_count = 2, param_count = 1, support_midi = false, rt_capable = true };
    float *ins[2]; 
    float *outs[2];
    float *params[1];
    uint32_t srate;
    static const char *port_names[in_count + out_count];
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
    static const char *port_names[in_count + out_count];
    static synth::ladspa_plugin_info plugin_info;
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
    PLUGIN_NAME_ID_LABEL("flanger", "flanger", "Flanger")
};

class phaser_audio_module: public null_audio_module
{
public:
    enum { par_freq, par_depth, par_rate, par_fb, par_stages, par_stereo, par_reset, par_amount, param_count };
    enum { in_count = 2, out_count = 2, support_midi = false, rt_capable = true };
    static const char *port_names[in_count + out_count];
    static synth::ladspa_plugin_info plugin_info;
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
    PLUGIN_NAME_ID_LABEL("phaser", "phaser", "Phaser")
};

class reverb_audio_module: public null_audio_module
{
public:    
    enum { par_decay, par_hfdamp, par_roomsize, par_diffusion, par_amount, param_count };
    enum { in_count = 2, out_count = 2, support_midi = false, rt_capable = true };
    static const char *port_names[in_count + out_count];
    static synth::ladspa_plugin_info plugin_info;
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
    PLUGIN_NAME_ID_LABEL("reverb", "reverb", "Reverb")
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
    static synth::ladspa_plugin_info plugin_info;
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
    
    /// do not export mode and inertia as CVs, as those are settings and not parameters
    inline static bool is_cv(int param_no) { return param_no != par_mode && param_no != par_inertia; }
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
    PLUGIN_NAME_ID_LABEL("filter", "filter", "Filter")
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
    static const char *port_names[in_count + out_count];
    static synth::ladspa_plugin_info plugin_info;
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
    PLUGIN_NAME_ID_LABEL("vintage_delay", "vintagedelay", "Vintage Delay")
};

class rotary_speaker_audio_module: public null_audio_module
{
public:
    enum { par_speed, par_spacing, par_shift, par_moddepth, par_treblespeed, par_bassspeed, par_micdistance, par_reflection, param_count };
    enum { in_count = 2, out_count = 2, support_midi = true, rt_capable = true };
    static const char *port_names[];
    float *ins[in_count]; 
    float *outs[out_count];
    float *params[param_count];
    /// Current phases and phase deltas for bass and treble rotors
    uint32_t phase_l, dphase_l, phase_h, dphase_h;
    dsp::simple_delay<1024, float> delay;
    dsp::biquad<float> crossover1l, crossover1r, crossover2l, crossover2r;
    dsp::simple_delay<8, float> phaseshift;
    uint32_t srate;
    int vibrato_mode;
    static parameter_properties param_props[];
    static synth::ladspa_plugin_info plugin_info;
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
        maspeed_h = maspeed_l = 0.f;
        setup();
    }
    void deactivate() {
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
            
            out_hi_l = crossover2l.process_d2(out_hi_l); // sanitize(out_hi_l);
            out_hi_r = crossover2r.process_d2(out_hi_r); // sanitize(out_hi_r);
            out_lo_l = crossover1l.process_d2(out_lo_l); // sanitize(out_lo_l);
            out_lo_r = crossover1r.process_d2(out_lo_r); // sanitize(out_lo_r);
            
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
        crossover1l.sanitize_d2();
        crossover1r.sanitize_d2();
        crossover2l.sanitize_d2();
        crossover2r.sanitize_d2();
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
    PLUGIN_NAME_ID_LABEL("rotary_speaker", "rotaryspeaker", "Rotary Speaker")
};

// A multitap stereo chorus thing
class multichorus_audio_module: public null_audio_module
{
public:    
    enum { par_delay, par_depth, par_rate, par_stereo, par_voices, par_vphase, par_amount, par_lfophase_l, par_lfophase_r, param_count };
    enum { in_count = 2, out_count = 2, rt_capable = true, support_midi = false };
    float *ins[in_count]; 
    float *outs[out_count];
    float *params[param_count];
    static const char *port_names[];
    uint32_t srate;
    dsp::multichorus<float, sine_multi_lfo<float, 8>, 4096> left, right;
    float last_r_phase;
    
    static parameter_properties param_props[];
    static synth::ladspa_plugin_info plugin_info;
public:    
    multichorus_audio_module()
    {
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
        left.lfo.vphase = right.lfo.vphase = vphase * 4096;
        float r_phase = *params[par_stereo] * (1.f / 360.f);
        if (fabs(r_phase - last_r_phase) > 0.0001f) {
            right.lfo.phase = left.lfo.phase;
            right.lfo.phase += chorus_phase(r_phase * 4096);
            last_r_phase = r_phase;
        }
    }
    void activate() {
        params_changed();
    }
    void set_sample_rate(uint32_t sr) {
        srate = sr;
        left.setup(sr);
        right.setup(sr);
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
    PLUGIN_NAME_ID_LABEL("multichorus", "multichorus", "Multi Chorus")
};

extern std::string get_builtin_modules_rdf();
extern const char *calf_copyright_info;

};

#include "modules_synths.h"

#endif
