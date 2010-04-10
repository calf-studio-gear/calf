/* Calf DSP Library
 * Example audio modules - DSP code
 *
 * Copyright (C) 2001-2008 Krzysztof Foltman
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
 * Boston, MA  02110-1301  USA
 */
#include <limits.h>
#include <memory.h>
#include <calf/giface.h>
#include <calf/modules.h>
#include <calf/modules_dev.h>

using namespace dsp;
using namespace calf_plugins;

#define SET_IF_CONNECTED(name) if (params[AM::param_##name] != NULL) *params[AM::param_##name] = name;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool frequency_response_line_graph::get_gridline(int index, int subindex, float &pos, bool &vertical, std::string &legend, cairo_iface *context) const
{ 
    return get_freq_gridline(subindex, pos, vertical, legend, context);
}

int frequency_response_line_graph::get_changed_offsets(int index, int generation, int &subindex_graph, int &subindex_dot, int &subindex_gridline) const
{
    subindex_graph = 0;
    subindex_dot = 0;
    subindex_gridline = generation ? INT_MAX : 0;
    return 1;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void flanger_audio_module::activate() {
    left.reset();
    right.reset();
    last_r_phase = *params[par_stereo] * (1.f / 360.f);
    left.reset_phase(0.f);
    right.reset_phase(last_r_phase);
    is_active = true;
}

void flanger_audio_module::set_sample_rate(uint32_t sr) {
    srate = sr;
    left.setup(sr);
    right.setup(sr);
}

void flanger_audio_module::deactivate() {
    is_active = false;
}

bool flanger_audio_module::get_graph(int index, int subindex, float *data, int points, cairo_iface *context) const
{
    if (!is_active)
        return false;
    if (index == par_delay && subindex < 2) 
    {
        set_channel_color(context, subindex);
        return ::get_graph(*this, subindex, data, points);
    }
    return false;
}

float flanger_audio_module::freq_gain(int subindex, float freq, float srate) const
{
    return (subindex ? right : left).freq_gain(freq, srate);                
}

void flanger_audio_module::params_changed()
{
    float dry = *params[par_dryamount];
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

void flanger_audio_module::params_reset()
{
    if (clear_reset) {
        *params[par_reset] = 0.f;
        clear_reset = false;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////

phaser_audio_module::phaser_audio_module()
: left(MaxStages, x1vals[0], y1vals[0])
, right(MaxStages, x1vals[1], y1vals[1])
{
    is_active = false;
}

void phaser_audio_module::set_sample_rate(uint32_t sr)
{
    srate = sr;
    left.setup(sr);
    right.setup(sr);
}

void phaser_audio_module::activate()
{
    is_active = true;
    left.reset();
    right.reset();
    last_r_phase = *params[par_stereo] * (1.f / 360.f);
    left.reset_phase(0.f);
    right.reset_phase(last_r_phase);
}

void phaser_audio_module::deactivate()
{
    is_active = false;
}

bool phaser_audio_module::get_graph(int index, int subindex, float *data, int points, cairo_iface *context) const
{
    if (!is_active)
        return false;
    if (subindex < 2) 
    {
        set_channel_color(context, subindex);
        return ::get_graph(*this, subindex, data, points);
    }
    return false;
}

float phaser_audio_module::freq_gain(int subindex, float freq, float srate) const
{
    return (subindex ? right : left).freq_gain(freq, srate);                
}

bool phaser_audio_module::get_gridline(int index, int subindex, float &pos, bool &vertical, std::string &legend, cairo_iface *context) const
{
    return get_freq_gridline(subindex, pos, vertical, legend, context);
}

void phaser_audio_module::params_changed()
{
    float dry = *params[par_dryamount];
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

void phaser_audio_module::params_reset()
{
    if (clear_reset) {
        *params[par_reset] = 0.f;
        clear_reset = false;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////

void reverb_audio_module::activate()
{
    reverb.reset();
}

void reverb_audio_module::deactivate()
{
}

void reverb_audio_module::set_sample_rate(uint32_t sr)
{
    srate = sr;
    reverb.setup(sr);
    amount.set_sample_rate(sr);
}

void reverb_audio_module::params_changed()
{
    reverb.set_type_and_diffusion(fastf2i_drm(*params[par_roomsize]), *params[par_diffusion]);
    reverb.set_time(*params[par_decay]);
    reverb.set_cutoff(*params[par_hfdamp]);
    amount.set_inertia(*params[par_amount]);
    dryamount.set_inertia(*params[par_dry]);
    left_lo.set_lp(dsp::clip(*params[par_treblecut], 20.f, (float)(srate * 0.49f)), srate);
    left_hi.set_hp(dsp::clip(*params[par_basscut], 20.f, (float)(srate * 0.49f)), srate);
    right_lo.copy_coeffs(left_lo);
    right_hi.copy_coeffs(left_hi);
    predelay_amt = (int) (srate * (*params[par_predelay]) * (1.0f / 1000.0f) + 1);
}

uint32_t reverb_audio_module::process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask)
{
    numsamples += offset;
    clip   -= std::min(clip, numsamples);
    for (uint32_t i = offset; i < numsamples; i++) {
        float dry = dryamount.get();
        float wet = amount.get();
        stereo_sample<float> s(ins[0][i], ins[1][i]);
        stereo_sample<float> s2 = pre_delay.process(s, predelay_amt);
        
        float rl = s2.left, rr = s2.right;
        rl = left_lo.process(left_hi.process(rl));
        rr = right_lo.process(right_hi.process(rr));
        reverb.process(rl, rr);
        outs[0][i] = dry*s.left + wet*rl;
        outs[1][i] = dry*s.right + wet*rr;
        meter_wet = std::max(fabs(wet*rl), fabs(wet*rr));
        meter_out = std::max(fabs(outs[0][i]), fabs(outs[1][i]));
        if(outs[0][i] > 1.f or outs[1][i] > 1.f) {
            clip = srate >> 3;
        }
    }
    reverb.extra_sanitize();
    left_lo.sanitize();
    left_hi.sanitize();
    right_lo.sanitize();
    right_hi.sanitize();
    if(params[par_meter_wet] != NULL) {
        *params[par_meter_wet] = meter_wet;
    }
    if(params[par_meter_out] != NULL) {
        *params[par_meter_out] = meter_out;
    }
    if(params[par_clip] != NULL) {
        *params[par_clip] = clip;
    }
    return outputs_mask;
}

///////////////////////////////////////////////////////////////////////////////////////////////

vintage_delay_audio_module::vintage_delay_audio_module()
{
    old_medium = -1;
    for (int i = 0; i < MAX_DELAY; i++) {
        buffers[0][i] = 0.f;
        buffers[1][i] = 0.f;
    }
}

void vintage_delay_audio_module::params_changed()
{
    float unit = 60.0 * srate / (*params[par_bpm] * *params[par_divide]);
    deltime_l = dsp::fastf2i_drm(unit * *params[par_time_l]);
    deltime_r = dsp::fastf2i_drm(unit * *params[par_time_r]);
    amt_left.set_inertia(*params[par_amount]); amt_right.set_inertia(*params[par_amount]);
    float fb = *params[par_feedback];
    dry = *params[par_dryamount];
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

void vintage_delay_audio_module::activate()
{
    bufptr = 0;
    age = 0;
}

void vintage_delay_audio_module::deactivate()
{
}

void vintage_delay_audio_module::set_sample_rate(uint32_t sr)
{
    srate = sr;
    old_medium = -1;
    amt_left.set_sample_rate(sr); amt_right.set_sample_rate(sr);
    fb_left.set_sample_rate(sr); fb_right.set_sample_rate(sr);
}

void vintage_delay_audio_module::calc_filters()
{
    // parameters are heavily influenced by gordonjcp and his tape delay unit
    // although, don't blame him if it sounds bad - I've messed with them too :)
    biquad_left[0].set_lp_rbj(6000, 0.707, srate);
    biquad_left[1].set_bp_rbj(4500, 0.250, srate);
    biquad_right[0].copy_coeffs(biquad_left[0]);
    biquad_right[1].copy_coeffs(biquad_left[1]);
}

uint32_t vintage_delay_audio_module::process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask)
{
    uint32_t ostate = 3; // XXXKF optimize!
    uint32_t end = offset + numsamples;
    int v = mixmode ? 1 : 0;
    int orig_bufptr = bufptr;
    for(uint32_t i = offset; i < end; i++)
    {
        float out_left, out_right, del_left, del_right;
        // if the buffer hasn't been cleared yet (after activation), pretend we've read zeros

        if (deltime_l >= age) {
            del_left = ins[0][i];
            out_left = dry * del_left;
            amt_left.step();
            fb_left.step();
        }
        else
        {
            float in_left = buffers[v][(bufptr - deltime_l) & ADDR_MASK];
            dsp::sanitize(in_left);
            out_left = dry * ins[0][i] + in_left * amt_left.get();
            del_left = ins[0][i] + in_left * fb_left.get();
        }
        if (deltime_r >= age) {
            del_right = ins[1][i];
            out_right = dry * del_right;
            amt_right.step();
            fb_right.step();
        }
        else
        {
            float in_right = buffers[1 - v][(bufptr - deltime_r) & ADDR_MASK];
            dsp::sanitize(in_right);
            out_right = dry * ins[1][i] + in_right * amt_right.get();
            del_right = ins[1][i] + in_right * fb_right.get();
        }
        
        age++;
        outs[0][i] = out_left; outs[1][i] = out_right; buffers[0][bufptr] = del_left; buffers[1][bufptr] = del_right;
        bufptr = (bufptr + 1) & (MAX_DELAY - 1);
    }
    if (age >= MAX_DELAY)
        age = MAX_DELAY;
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

///////////////////////////////////////////////////////////////////////////////////////////////

bool filter_audio_module::get_graph(int index, int subindex, float *data, int points, cairo_iface *context) const
{
    if (!is_active)
        return false;
    if (index == par_cutoff && !subindex) {
        context->set_line_width(1.5);
        return ::get_graph(*this, subindex, data, points);
    }
    return false;
}

int filter_audio_module::get_changed_offsets(int index, int generation, int &subindex_graph, int &subindex_dot, int &subindex_gridline) const
{
    if (fabs(inertia_cutoff.get_last() - old_cutoff) + 100 * fabs(inertia_resonance.get_last() - old_resonance) + fabs(*params[par_mode] - old_mode) > 0.1f)
    {
        old_cutoff = inertia_cutoff.get_last();
        old_resonance = inertia_resonance.get_last();
        old_mode = *params[par_mode];
        last_generation++;
        subindex_graph = 0;
        subindex_dot = INT_MAX;
        subindex_gridline = INT_MAX;
    }
    else {
        subindex_graph = 0;
        subindex_dot = subindex_gridline = generation ? INT_MAX : 0;
    }
    if (generation == last_calculated_generation)
        subindex_graph = INT_MAX;
    return last_generation;
}


///////////////////////////////////////////////////////////////////////////////////////////////

filterclavier_audio_module::filterclavier_audio_module() 
: filter_module_with_inertia<biquad_filter_module, filterclavier_metadata>(ins, outs, params)
, min_gain(1.0)
, max_gain(32.0)
, last_note(-1)
, last_velocity(-1)
{
}
    
void filterclavier_audio_module::params_changed()
{ 
    inertia_filter_module::inertia_cutoff.set_inertia(
        note_to_hz(last_note + *params[par_transpose], *params[par_detune]));
    
    float min_resonance = param_props[par_max_resonance].min;
     inertia_filter_module::inertia_resonance.set_inertia( 
             (float(last_velocity) / 127.0)
             // 0.001: see below
             * (*params[par_max_resonance] - min_resonance + 0.001)
             + min_resonance);
         
    adjust_gain_according_to_filter_mode(last_velocity);
    
    inertia_filter_module::calculate_filter(); 
}

void filterclavier_audio_module::activate()
{
    inertia_filter_module::activate();
}

void filterclavier_audio_module::set_sample_rate(uint32_t sr)
{
    inertia_filter_module::set_sample_rate(sr);
}

void filterclavier_audio_module::deactivate()
{
    inertia_filter_module::deactivate();
}


void filterclavier_audio_module::note_on(int note, int vel)
{
    last_note     = note;
    last_velocity = vel;
    inertia_filter_module::inertia_cutoff.set_inertia(
            note_to_hz(note + *params[par_transpose], *params[par_detune]));

    float min_resonance = param_props[par_max_resonance].min;
    inertia_filter_module::inertia_resonance.set_inertia( 
            (float(vel) / 127.0) 
            // 0.001: if the difference is equal to zero (which happens
            // when the max_resonance knom is at minimum position
            // then the filter gain doesnt seem to snap to zero on most note offs
            * (*params[par_max_resonance] - min_resonance + 0.001) 
            + min_resonance);
    
    adjust_gain_according_to_filter_mode(vel);
    
    inertia_filter_module::calculate_filter();
}

void filterclavier_audio_module::note_off(int note, int vel)
{
    if (note == last_note) {
        inertia_filter_module::inertia_resonance.set_inertia(param_props[par_max_resonance].min);
        inertia_filter_module::inertia_gain.set_inertia(min_gain);
        inertia_filter_module::calculate_filter();
        last_velocity = 0;
    }
}

void filterclavier_audio_module::adjust_gain_according_to_filter_mode(int velocity)
{
    int   mode = dsp::fastf2i_drm(*params[par_mode]);
    
    // for bandpasses: boost gain for velocities > 0
    if ( (mode_6db_bp <= mode) && (mode <= mode_18db_bp) ) {
        // gain for velocity 0:   1.0
        // gain for velocity 127: 32.0
        float mode_max_gain = max_gain;
        // max_gain is right for mode_6db_bp
        if (mode == mode_12db_bp)
            mode_max_gain /= 6.0;
        if (mode == mode_18db_bp)
            mode_max_gain /= 10.5;
        
        inertia_filter_module::inertia_gain.set_now(
                (float(velocity) / 127.0) * (mode_max_gain - min_gain) + min_gain);
    } else {
        inertia_filter_module::inertia_gain.set_now(min_gain);
    }
}

bool filterclavier_audio_module::get_graph(int index, int subindex, float *data, int points, cairo_iface *context) const
{
    if (!is_active || index != par_mode) {
        return false;
    }
    if (!subindex) {
        context->set_line_width(1.5);
        return ::get_graph(*this, subindex, data, points);
    }
    return false;
}


///////////////////////////////////////////////////////////////////////////////////////////////

rotary_speaker_audio_module::rotary_speaker_audio_module()
{
    mwhl_value = hold_value = 0.f;
    phase_h = phase_l = 0.f;
    aspeed_l = 1.f;
    aspeed_h = 1.f;
    dspeed = 0.f;
}    

void rotary_speaker_audio_module::set_sample_rate(uint32_t sr)
{
    srate = sr;
    setup();
}

void rotary_speaker_audio_module::setup()
{
    crossover1l.set_lp_rbj(800.f, 0.7, (float)srate);
    crossover1r.set_lp_rbj(800.f, 0.7, (float)srate);
    crossover2l.set_hp_rbj(800.f, 0.7, (float)srate);
    crossover2r.set_hp_rbj(800.f, 0.7, (float)srate);
}

void rotary_speaker_audio_module::activate()
{
    phase_h = phase_l = 0.f;
    maspeed_h = maspeed_l = 0.f;
    setup();
}

void rotary_speaker_audio_module::deactivate()
{
}

void rotary_speaker_audio_module::control_change(int ctl, int val)
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

void rotary_speaker_audio_module::params_changed()
{
    set_vibrato();
}

void rotary_speaker_audio_module::set_vibrato()
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
uint32_t rotary_speaker_audio_module::rpm2dphase(float rpm)
{
    return (uint32_t)((rpm / (60.0 * srate)) * (1 << 30)) << 2;
}

/// Set delta-phase variables based on current calculated (and interpolated) RPM speed
void rotary_speaker_audio_module::update_speed()
{
    float speed_h = aspeed_h >= 0 ? (48 + (400-48) * aspeed_h) : (48 * (1 + aspeed_h));
    float speed_l = aspeed_l >= 0 ? 40 + (342-40) * aspeed_l : (40 * (1 + aspeed_l));
    dphase_h = rpm2dphase(speed_h);
    dphase_l = rpm2dphase(speed_l);
}

void rotary_speaker_audio_module::update_speed_manual(float delta)
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
    return (int) (32768 + 32768 * (v - v*v*v) * (1.0 / 0.3849));
}

/// Increase or decrease aspeed towards raspeed, with required negative and positive rate
inline bool rotary_speaker_audio_module::incr_towards(float &aspeed, float raspeed, float delta_decc, float delta_acc)
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

uint32_t rotary_speaker_audio_module::process(uint32_t offset, uint32_t nsamples, uint32_t inputs_mask, uint32_t outputs_mask)
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
        meter_l = xl;
        meter_h = xh;
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
    if(params[par_meter_l] != NULL) {
        *params[par_meter_l] = (float)meter_l / 65536.0;
    }
    if(params[par_meter_h] != NULL) {
        *params[par_meter_h] = (float)meter_h / 65536.0;
    }
    return outputs_mask;
}


///////////////////////////////////////////////////////////////////////////////////////////////

multichorus_audio_module::multichorus_audio_module()
{
    is_active = false;
    last_r_phase = -1;
}

void multichorus_audio_module::activate()
{
    is_active = true;
    params_changed();
}

void multichorus_audio_module::deactivate()
{
    is_active = false;
}

void multichorus_audio_module::set_sample_rate(uint32_t sr) {
    srate = sr;
    left.setup(sr);
    right.setup(sr);
}

bool multichorus_audio_module::get_graph(int index, int subindex, float *data, int points, cairo_iface *context) const
{
    if (!is_active)
        return false;
    int nvoices = (int)*params[par_voices];
    if (index == par_delay && subindex < 3) 
    {
        if (subindex < 2)
            set_channel_color(context, subindex);
        else {
            context->set_source_rgba(0.35, 0.4, 0.2);
            context->set_line_width(1.0);
        }
        return ::get_graph(*this, subindex, data, points);
    }
    if (index == par_rate && subindex < nvoices) {
        const sine_multi_lfo<float, 8> &lfo = left.lfo;
        for (int i = 0; i < points; i++) {
            float phase = i * 2 * M_PI / points;
            // original -65536 to 65535 value
            float orig = subindex * lfo.voice_offset + ((lfo.voice_depth >> (30-13)) * 65536.0 * (0.95 * sin(phase) + 1)/ 8192.0) - 65536;
            // scale to -1..1
            data[i] = orig / 65536.0;
        }
        return true;
    }
    return false;
}

bool multichorus_audio_module::get_dot(int index, int subindex, float &x, float &y, int &size, cairo_iface *context) const
{
    int voice = subindex >> 1;
    int nvoices = (int)*params[par_voices];
    if ((index != par_rate && index != par_depth) || voice >= nvoices)
        return false;

    float unit = (1 - *params[par_overlap]);
    float scw = 1 + unit * (nvoices - 1);
    set_channel_color(context, subindex);
    const sine_multi_lfo<float, 8> &lfo = (subindex & 1 ? right : left).lfo;
    if (index == par_rate)
    {
        x = (double)(lfo.phase + lfo.vphase * voice) / 4096.0;
        y = 0.95 * sin(x * 2 * M_PI);
        y = (voice * unit + (y + 1) / 2) / scw * 2 - 1;
    }
    else
    {
        double ph = (double)(lfo.phase + lfo.vphase * voice) / 4096.0;
        x = 0.5 + 0.5 * sin(ph * 2 * M_PI);
        y = subindex & 1 ? -0.75 : 0.75;
        x = (voice * unit + x) / scw;
    }
    return true;
}

bool multichorus_audio_module::get_gridline(int index, int subindex, float &pos, bool &vertical, std::string &legend, cairo_iface *context) const
{
    if (index == par_rate && !subindex)
    {
        pos = 0;
        vertical = false;
        return true;
    }
    if (index == par_delay)
        return get_freq_gridline(subindex, pos, vertical, legend, context);
    return false;
}

float multichorus_audio_module::freq_gain(int subindex, float freq, float srate) const
{
    if (subindex == 2)
        return *params[par_amount] * left.post.freq_gain(freq, srate);
    return (subindex ? right : left).freq_gain(freq, srate);                
}

void multichorus_audio_module::params_changed()
{
    // delicious copy-pasta from flanger module - it'd be better to keep it common or something
    float dry = *params[par_dryamount];
    float wet = *params[par_amount];
    float rate = *params[par_rate];
    float min_delay = *params[par_delay] / 1000.0;
    float mod_depth = *params[par_depth] / 1000.0;
    float overlap = *params[par_overlap];
    left.set_dry(dry); right.set_dry(dry);
    left.set_wet(wet); right.set_wet(wet);
    left.set_rate(rate); right.set_rate(rate);
    left.set_min_delay(min_delay); right.set_min_delay(min_delay);
    left.set_mod_depth(mod_depth); right.set_mod_depth(mod_depth);
    int voices = (int)*params[par_voices];
    left.lfo.set_voices(voices); right.lfo.set_voices(voices);
    left.lfo.set_overlap(overlap);right.lfo.set_overlap(overlap);
    float vphase = *params[par_vphase] * (1.f / 360.f);
    left.lfo.vphase = right.lfo.vphase = vphase * (4096 / std::max(voices - 1, 1));
    float r_phase = *params[par_stereo] * (1.f / 360.f);
    if (fabs(r_phase - last_r_phase) > 0.0001f) {
        right.lfo.phase = left.lfo.phase;
        right.lfo.phase += chorus_phase(r_phase * 4096);
        last_r_phase = r_phase;
    }
    left.post.f1.set_bp_rbj(*params[par_freq], *params[par_q], srate);
    left.post.f2.set_bp_rbj(*params[par_freq2], *params[par_q], srate);
    right.post.f1.copy_coeffs(left.post.f1);
    right.post.f2.copy_coeffs(left.post.f2);
}

uint32_t multichorus_audio_module::process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask)
{
    left.process(outs[0] + offset, ins[0] + offset, numsamples);
    right.process(outs[1] + offset, ins[1] + offset, numsamples);
    return outputs_mask; // XXXKF allow some delay after input going blank
}



/// Multibandcompressor by Markus Schmidt
///
/// This module splits the signal in four different bands
/// and sends them through multiple filters (implemented by
/// Krzysztof). They are processed by a compressing routine
/// (implemented by Thor) afterwards and summed up to the
/// final output again.
///////////////////////////////////////////////////////////////////////////////////////////////

multibandcompressor_audio_module::multibandcompressor_audio_module()
{
    is_active = false;
    srate = 0;
    // zero all displays
    clip_inL    = 0.f;
    clip_inR    = 0.f;
    clip_outL   = 0.f;
    clip_outR   = 0.f;
    meter_inL  = 0.f;
    meter_inR  = 0.f;
    meter_outL = 0.f;
    meter_outR = 0.f;
}

void multibandcompressor_audio_module::activate()
{
    is_active = true;
    // set all filters and strips
    params_changed();
    // activate all strips
    for (int j = 0; j < strips; j ++) {
        strip[j].activate();
        strip[j].id = j;
    }
}

void multibandcompressor_audio_module::deactivate()
{
    is_active = false;
    // deactivate all strips
    for (int j = 0; j < strips; j ++) {
        strip[j].deactivate();
    }
}

void multibandcompressor_audio_module::params_changed()
{
    // set the params of all filters
    if(*params[param_freq0] != freq_old[0] or *params[param_sep0] != sep_old[0] or *params[param_q0] != q_old[0]) {
        lpL0.set_lp_rbj((float)(*params[param_freq0] * (1 - *params[param_sep0])), *params[param_q0], (float)srate);
        lpR0.copy_coeffs(lpL0);
        hpL0.set_hp_rbj((float)(*params[param_freq0] * (1 + *params[param_sep0])), *params[param_q0], (float)srate);
        hpR0.copy_coeffs(hpL0);
        freq_old[0] = *params[param_freq0];
        sep_old[0]  = *params[param_sep0];
        q_old[0]    = *params[param_q0];
    }
    if(*params[param_freq1] != freq_old[1] or *params[param_sep1] != sep_old[1] or *params[param_q1] != q_old[1]) {
        lpL1.set_lp_rbj((float)(*params[param_freq1] * (1 - *params[param_sep1])), *params[param_q1], (float)srate);
        lpR1.copy_coeffs(lpL1);
        hpL1.set_hp_rbj((float)(*params[param_freq1] * (1 + *params[param_sep1])), *params[param_q1], (float)srate);
        hpR1.copy_coeffs(hpL1);
        freq_old[1] = *params[param_freq1];
        sep_old[1]  = *params[param_sep1];
        q_old[1]    = *params[param_q1];
    }
    if(*params[param_freq2] != freq_old[2] or *params[param_sep2] != sep_old[2] or *params[param_q2] != q_old[2]) {
        lpL2.set_lp_rbj((float)(*params[param_freq2] * (1 - *params[param_sep2])), *params[param_q2], (float)srate);
        lpR2.copy_coeffs(lpL2);
        hpL2.set_hp_rbj((float)(*params[param_freq2] * (1 + *params[param_sep2])), *params[param_q2], (float)srate);
        hpR2.copy_coeffs(hpL2);
        freq_old[2] = *params[param_freq2];
        sep_old[2]  = *params[param_sep2];
        q_old[2]    = *params[param_q2];
    }
    // set the params of all strips
    strip[0].set_params(*params[param_attack0], *params[param_release0], *params[param_threshold0], *params[param_ratio0], *params[param_knee0], *params[param_makeup0], *params[param_detection0], 1.f, *params[param_bypass0], *params[param_mute0]);
    strip[1].set_params(*params[param_attack1], *params[param_release1], *params[param_threshold1], *params[param_ratio1], *params[param_knee1], *params[param_makeup1], *params[param_detection1], 1.f, *params[param_bypass1], *params[param_mute1]);
    strip[2].set_params(*params[param_attack2], *params[param_release2], *params[param_threshold2], *params[param_ratio2], *params[param_knee2], *params[param_makeup2], *params[param_detection2], 1.f, *params[param_bypass2], *params[param_mute2]);
    strip[3].set_params(*params[param_attack3], *params[param_release3], *params[param_threshold3], *params[param_ratio3], *params[param_knee3], *params[param_makeup3], *params[param_detection3], 1.f, *params[param_bypass3], *params[param_mute3]);
}

void multibandcompressor_audio_module::set_sample_rate(uint32_t sr)
{
    srate = sr;
    // set srate of all strips
    for (int j = 0; j < strips; j ++) {
        strip[j].set_sample_rate(srate);
    }
}

#define BYPASSED_COMPRESSION(index) \
    if(params[param_compression##index] != NULL) \
        *params[param_compression##index] = 1.0; \
    if(params[param_output##index] != NULL) \
        *params[param_output##index] = 0.0; 

#define ACTIVE_COMPRESSION(index) \
    if(params[param_compression##index] != NULL) \
        *params[param_compression##index] = strip[index].get_comp_level(); \
    if(params[param_output##index] != NULL) \
        *params[param_output##index] = strip[index].get_output_level();

uint32_t multibandcompressor_audio_module::process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask)
{
    bool bypass = *params[param_bypass] > 0.5f;
    numsamples += offset;
    for (int i = 0; i < strips; i++)
        strip[i].update_curve();
    if(bypass) {
        // everything bypassed
        while(offset < numsamples) {
            outs[0][offset] = ins[0][offset];
            outs[1][offset] = ins[1][offset];
            ++offset;
        }
        // displays, too
        clip_inL    = 0.f;
        clip_inR    = 0.f;
        clip_outL   = 0.f;
        clip_outR   = 0.f;
        meter_inL  = 0.f;
        meter_inR  = 0.f;
        meter_outL = 0.f;
        meter_outR = 0.f;
    } else {
        // process all strips
        
        // determine mute state of strips
        mute[0] = *params[param_mute0] > 0.f ? true : false;
        mute[1] = *params[param_mute1] > 0.f ? true : false;
        mute[2] = *params[param_mute2] > 0.f ? true : false;
        mute[3] = *params[param_mute3] > 0.f ? true : false;
        
        // let meters fall a bit
        clip_inL    -= std::min(clip_inL,  numsamples);
        clip_inR    -= std::min(clip_inR,  numsamples);
        clip_outL   -= std::min(clip_outL, numsamples);
        clip_outR   -= std::min(clip_outR, numsamples);
        meter_inL = 0.f;
        meter_inR = 0.f;
        meter_outL = 0.f;
        meter_outR = 0.f;
        while(offset < numsamples) {
            // cycle through samples
            float inL = ins[0][offset];
            float inR = ins[1][offset];
            // in level
            inR *= *params[param_level_in];
            inL *= *params[param_level_in];
            // out vars
            float outL = 0.f;
            float outR = 0.f;
            for (int i = 0; i < strips; i ++) {
                // cycle trough strips
                if (!mute[i]) {
                    // strip unmuted
                    float left  = inL;
                    float right = inR;
                    // send trough filters
                    switch (i) {
                        case 0:
                            left  = lpL0.process(left);
                            right = lpR0.process(right);
                            lpL0.sanitize();
                            lpR0.sanitize();
                            break;
                        case 1:
                            left  = lpL1.process(left);
                            right = lpR1.process(right);
                            left  = hpL0.process(left);
                            right = hpR0.process(right);
                            lpL1.sanitize();
                            lpR1.sanitize();
                            hpL0.sanitize();
                            hpR0.sanitize();
                            break;
                        case 2:
                            left  = lpL2.process(left);
                            right = lpR2.process(right);
                            left  = hpL1.process(left);
                            right = hpR1.process(right);
                            lpL2.sanitize();
                            lpR2.sanitize();
                            hpL1.sanitize();
                            hpR1.sanitize();
                            break;
                        case 3:
                            left  = hpL2.process(left);
                            right = hpR2.process(right);
                            hpL2.sanitize();
                            hpR2.sanitize();
                            break;
                    }
                    // process gain reduction
                    strip[i].process(left, right);
                    // sum up output
                    outL += left;
                    outR += right;
                } else {
                    // strip muted
                    
                }
                
                
            } // process single strip
            
            // even out filters gain reduction
            // 3dB - levelled manually (based on default sep and q settings)
            outL *= 1.414213562;
            outR *= 1.414213562;
            
            // out level
            outL *= *params[param_level_out];
            outR *= *params[param_level_out];
            
            // send to output
            outs[0][offset] = outL;
            outs[1][offset] = outR;
            
            // clip LED's
            if(inL > 1.f) {
                clip_inL  = srate >> 3;
            }
            if(inR > 1.f) {
                clip_inR  = srate >> 3;
            }
            if(outL > 1.f) {
                clip_outL = srate >> 3;
            }
            if(outR > 1.f) {
                clip_outR = srate >> 3;
            }
            // set up in / out meters
            if(inL > meter_inL) {
                meter_inL = inL;
            }
            if(inR > meter_inR) {
                meter_inR = inR;
            }
            if(outL > meter_outL) {
                meter_outL = outL;
            }
            if(outR > meter_outR) {
                meter_outR = outR;
            }
            // next sample
            ++offset;
        } // cycle trough samples
        
    } // process all strips (no bypass)
    
    // draw meters
    SET_IF_CONNECTED(clip_inL);
    SET_IF_CONNECTED(clip_inR);
    SET_IF_CONNECTED(clip_outL);
    SET_IF_CONNECTED(clip_outR);
    SET_IF_CONNECTED(meter_inL);
    SET_IF_CONNECTED(meter_inR);
    SET_IF_CONNECTED(meter_outL);
    SET_IF_CONNECTED(meter_outR);
    // draw strip meters
    if(bypass > 0.5f) {
        BYPASSED_COMPRESSION(0)
        BYPASSED_COMPRESSION(1)
        BYPASSED_COMPRESSION(2)
        BYPASSED_COMPRESSION(3)
    } else {
        ACTIVE_COMPRESSION(0)
        ACTIVE_COMPRESSION(1)
        ACTIVE_COMPRESSION(2)
        ACTIVE_COMPRESSION(3)
    }
    // whatever has to be returned x)
    return outputs_mask;
}

const gain_reduction_audio_module *multibandcompressor_audio_module::get_strip_by_param_index(int index) const
{
    // let's handle by the corresponding strip
    switch (index) {
        case param_compression0:
            return &strip[0];
        case param_compression1:
            return &strip[1];
        case param_compression2:
            return &strip[2];
        case param_compression3:
            return &strip[3];
    }
    return NULL;
}

bool multibandcompressor_audio_module::get_graph(int index, int subindex, float *data, int points, cairo_iface *context) const
{
    const gain_reduction_audio_module *m = get_strip_by_param_index(index);
    if (m)
        return m->get_graph(subindex, data, points, context);
    return false;
}

bool multibandcompressor_audio_module::get_dot(int index, int subindex, float &x, float &y, int &size, cairo_iface *context) const
{
    const gain_reduction_audio_module *m = get_strip_by_param_index(index);
    if (m)
        return m->get_dot(subindex, x, y, size, context);
    return false;
}

bool multibandcompressor_audio_module::get_gridline(int index, int subindex, float &pos, bool &vertical, std::string &legend, cairo_iface *context) const
{ 
    const gain_reduction_audio_module *m = get_strip_by_param_index(index);
    if (m)
        return m->get_gridline(subindex, pos, vertical, legend, context);
    return false;
}

int multibandcompressor_audio_module::get_changed_offsets(int index, int generation, int &subindex_graph, int &subindex_dot, int &subindex_gridline) const
{
    const gain_reduction_audio_module *m = get_strip_by_param_index(index);
    if (m)
        return m->get_changed_offsets(generation, subindex_graph, subindex_dot, subindex_gridline);
    return 0;
}

/// Compressor originally by Thor
///
/// This module provides Thor's original compressor without any sidechain or weighting
///////////////////////////////////////////////////////////////////////////////////////////////

compressor_audio_module::compressor_audio_module()
{
    is_active = false;
    srate = 0;
    last_generation = 0;
}

void compressor_audio_module::activate()
{
    is_active = true;
    // set all filters and strips
    compressor.activate();
    params_changed();
    meter_in = 0.f;
    meter_out = 0.f;
    clip_in = 0.f;
    clip_out = 0.f;
}
void compressor_audio_module::deactivate()
{
    is_active = false;
    compressor.deactivate();
}

void compressor_audio_module::params_changed()
{
    compressor.set_params(*params[param_attack], *params[param_release], *params[param_threshold], *params[param_ratio], *params[param_knee], *params[param_makeup], *params[param_detection], *params[param_stereo_link], *params[param_bypass], 0.f);
}

void compressor_audio_module::set_sample_rate(uint32_t sr)
{
    srate = sr;
    compressor.set_sample_rate(srate);
}

uint32_t compressor_audio_module::process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask)
{
    bool bypass = *params[param_bypass] > 0.5f;
    numsamples += offset;
    if(bypass) {
        // everything bypassed
        while(offset < numsamples) {
            outs[0][offset] = ins[0][offset];
            outs[1][offset] = ins[1][offset];
            ++offset;
        }
        // displays, too
        clip_in    = 0.f;
        clip_out   = 0.f;
        meter_in   = 0.f;
        meter_out  = 0.f;
    } else {
        // process
        
        clip_in    -= std::min(clip_in,  numsamples);
        clip_out   -= std::min(clip_out,  numsamples);
        
        compressor.update_curve();        
        
        while(offset < numsamples) {
            // cycle through samples
            float outL = 0.f;
            float outR = 0.f;
            float inL = ins[0][offset];
            float inR = ins[1][offset];
            // in level
            inR *= *params[param_level_in];
            inL *= *params[param_level_in];
            
            float leftAC = inL;
            float rightAC = inR;
            
            compressor.process(leftAC, rightAC);
            
            outL = leftAC;
            outR = rightAC;
            
            // send to output
            outs[0][offset] = outL;
            outs[1][offset] = outR;
            
            // clip LED's
            if(std::max(fabs(inL), fabs(inR)) > 1.f) {
                clip_in   = srate >> 3;
            }
            if(std::max(fabs(outL), fabs(outR)) > 1.f) {
                clip_out  = srate >> 3;
            }
            // rise up out meter
            meter_in = std::max(fabs(inL), fabs(inR));;
            meter_out = std::max(fabs(outL), fabs(outR));;
            
            // next sample
            ++offset;
        } // cycle trough samples
    }
    // draw meters
    SET_IF_CONNECTED(clip_in)
    SET_IF_CONNECTED(clip_out)
    SET_IF_CONNECTED(meter_in)
    SET_IF_CONNECTED(meter_out)
    // draw strip meter
    if(bypass > 0.5f) {
        if(params[param_compression] != NULL) {
            *params[param_compression] = 1.0f;
        }
    } else {
        if(params[param_compression] != NULL) {
            *params[param_compression] = compressor.get_comp_level();
        }
    }
    // whatever has to be returned x)
    return outputs_mask;
}
bool compressor_audio_module::get_graph(int index, int subindex, float *data, int points, cairo_iface *context) const
{
    if (!is_active)
        return false;
    return compressor.get_graph(subindex, data, points, context);
}

bool compressor_audio_module::get_dot(int index, int subindex, float &x, float &y, int &size, cairo_iface *context) const
{
    if (!is_active)
        return false;
    return compressor.get_dot(subindex, x, y, size, context);
}

bool compressor_audio_module::get_gridline(int index, int subindex, float &pos, bool &vertical, std::string &legend, cairo_iface *context) const
{
    if (!is_active)
        return false;
    return compressor.get_gridline(subindex, pos, vertical, legend, context);
}

int compressor_audio_module::get_changed_offsets(int index, int generation, int &subindex_graph, int &subindex_dot, int &subindex_gridline) const
{
    if (!is_active)
        return false;
    return compressor.get_changed_offsets(generation, subindex_graph, subindex_dot, subindex_gridline);
}

/// Sidecain Compressor by Markus Schmidt
///
/// This module splits the signal in a sidechain- and a process signal.
/// The sidechain is processed through Krzystofs filters and compresses
/// the process signal via Thor's compression routine afterwards.
///////////////////////////////////////////////////////////////////////////////////////////////

sidechaincompressor_audio_module::sidechaincompressor_audio_module()
{
    is_active = false;
    srate = 0;
    last_generation = 0;
}

void sidechaincompressor_audio_module::activate()
{
    is_active = true;
    // set all filters and strips
    compressor.activate();
    params_changed();
    meter_in = 0.f;
    meter_out = 0.f;
    clip_in = 0.f;
    clip_out = 0.f;
}
void sidechaincompressor_audio_module::deactivate()
{
    is_active = false;
    compressor.deactivate();
}

sidechaincompressor_audio_module::cfloat sidechaincompressor_audio_module::h_z(const cfloat &z) const
{
    switch (sc_mode) {
        default:
        case WIDEBAND:
            return false;
            break;
        case DEESSER_WIDE:
        case DERUMBLER_WIDE:
        case WEIGHTED_1:
        case WEIGHTED_2:
        case WEIGHTED_3:
        case BANDPASS_2:
            return f1L.h_z(z) * f2L.h_z(z);
            break;
        case DEESSER_SPLIT:
            return f2L.h_z(z);
            break;
        case DERUMBLER_SPLIT:
        case BANDPASS_1:
            return f1L.h_z(z);
            break;
    }            
}

float sidechaincompressor_audio_module::freq_gain(int index, double freq, uint32_t sr) const
{
    typedef std::complex<double> cfloat;
    freq *= 2.0 * M_PI / sr;
    cfloat z = 1.0 / exp(cfloat(0.0, freq));
    
    return std::abs(h_z(z));
}

void sidechaincompressor_audio_module::params_changed()
{
    // set the params of all filters
    if(*params[param_f1_freq] != f1_freq_old or *params[param_f1_level] != f1_level_old
        or *params[param_f2_freq] != f2_freq_old or *params[param_f2_level] != f2_level_old
        or *params[param_sc_mode] != sc_mode) {
        float q = 0.707;
        switch ((int)*params[param_sc_mode]) {
            default:
            case WIDEBAND:
                f1L.set_hp_rbj((float)*params[param_f1_freq], q, (float)srate, *params[param_f1_level]);
                f1R.copy_coeffs(f1L);
                f2L.set_lp_rbj((float)*params[param_f2_freq], q, (float)srate, *params[param_f2_level]);
                f2R.copy_coeffs(f2L);
                f1_active = 0.f;
                f2_active = 0.f;
                break;
            case DEESSER_WIDE:
                f1L.set_peakeq_rbj((float)*params[param_f1_freq], q, *params[param_f1_level], (float)srate);
                f1R.copy_coeffs(f1L);
                f2L.set_hp_rbj((float)*params[param_f2_freq], q, (float)srate, *params[param_f2_level]);
                f2R.copy_coeffs(f2L);
                f1_active = 0.5f;
                f2_active = 1.f;
                break;
            case DEESSER_SPLIT:
                f1L.set_lp_rbj((float)*params[param_f2_freq] * (1 + 0.17), q, (float)srate);
                f1R.copy_coeffs(f1L);
                f2L.set_hp_rbj((float)*params[param_f2_freq] * (1 - 0.17), q, (float)srate, *params[param_f2_level]);
                f2R.copy_coeffs(f2L);
                f1_active = 0.f;
                f2_active = 1.f;
                break;
            case DERUMBLER_WIDE:
                f1L.set_lp_rbj((float)*params[param_f1_freq], q, (float)srate, *params[param_f1_level]);
                f1R.copy_coeffs(f1L);
                f2L.set_peakeq_rbj((float)*params[param_f2_freq], q, *params[param_f2_level], (float)srate);
                f2R.copy_coeffs(f2L);
                f1_active = 1.f;
                f2_active = 0.5f;
                break;
            case DERUMBLER_SPLIT:
                f1L.set_lp_rbj((float)*params[param_f1_freq] * (1 + 0.17), q, (float)srate, *params[param_f1_level]);
                f1R.copy_coeffs(f1L);
                f2L.set_hp_rbj((float)*params[param_f1_freq] * (1 - 0.17), q, (float)srate);
                f2R.copy_coeffs(f2L);
                f1_active = 1.f;
                f2_active = 0.f;
                break;
            case WEIGHTED_1:
                f1L.set_lowshelf_rbj((float)*params[param_f1_freq], q, *params[param_f1_level], (float)srate);
                f1R.copy_coeffs(f1L);
                f2L.set_highshelf_rbj((float)*params[param_f2_freq], q, *params[param_f2_level], (float)srate);
                f2R.copy_coeffs(f2L);
                f1_active = 0.5f;
                f2_active = 0.5f;
                break;
            case WEIGHTED_2:
                f1L.set_lowshelf_rbj((float)*params[param_f1_freq], q, *params[param_f1_level], (float)srate);
                f1R.copy_coeffs(f1L);
                f2L.set_peakeq_rbj((float)*params[param_f2_freq], q, *params[param_f2_level], (float)srate);
                f2R.copy_coeffs(f2L);
                f1_active = 0.5f;
                f2_active = 0.5f;
                break;
            case WEIGHTED_3:
                f1L.set_peakeq_rbj((float)*params[param_f1_freq], q, *params[param_f1_level], (float)srate);
                f1R.copy_coeffs(f1L);
                f2L.set_highshelf_rbj((float)*params[param_f2_freq], q, *params[param_f2_level], (float)srate);
                f2R.copy_coeffs(f2L);
                f1_active = 0.5f;
                f2_active = 0.5f;
                break;
            case BANDPASS_1:
                f1L.set_bp_rbj((float)*params[param_f1_freq], q, (float)srate, *params[param_f1_level]);
                f1R.copy_coeffs(f1L);
                f2L.set_hp_rbj((float)*params[param_f2_freq], q, *params[param_f2_level], (float)srate);
                f2R.copy_coeffs(f2L);
                f1_active = 1.f;
                f2_active = 0.f;
                break;
            case BANDPASS_2:
                f1L.set_hp_rbj((float)*params[param_f1_freq], q, (float)srate, *params[param_f1_level]);
                f1R.copy_coeffs(f1L);
                f2L.set_lp_rbj((float)*params[param_f2_freq], q, (float)srate, *params[param_f2_level]);
                f2R.copy_coeffs(f2L);
                f1_active = 1.f;
                f2_active = 1.f;
                break;
        }
        f1_freq_old = *params[param_f1_freq];
        f1_level_old = *params[param_f1_level];
        f2_freq_old = *params[param_f2_freq];
        f2_level_old = *params[param_f2_level];
        sc_mode = (CalfScModes)*params[param_sc_mode];
    }
    // light LED's
    if(params[param_f1_active] != NULL) {
        *params[param_f1_active] = f1_active;
    }
    if(params[param_f2_active] != NULL) {
        *params[param_f2_active] = f2_active;
    }
    // and set the compressor module
    compressor.set_params(*params[param_attack], *params[param_release], *params[param_threshold], *params[param_ratio], *params[param_knee], *params[param_makeup], *params[param_detection], *params[param_stereo_link], *params[param_bypass], 0.f);
}

void sidechaincompressor_audio_module::set_sample_rate(uint32_t sr)
{
    srate = sr;
    compressor.set_sample_rate(srate);
}

uint32_t sidechaincompressor_audio_module::process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask)
{
    bool bypass = *params[param_bypass] > 0.5f;
    numsamples += offset;
    if(bypass) {
        // everything bypassed
        while(offset < numsamples) {
            outs[0][offset] = ins[0][offset];
            outs[1][offset] = ins[1][offset];
            ++offset;
        }
        // displays, too
        clip_in    = 0.f;
        clip_out   = 0.f;
        meter_in   = 0.f;
        meter_out  = 0.f;
    } else {
        // process
        
        clip_in    -= std::min(clip_in,  numsamples);
        clip_out   -= std::min(clip_out,  numsamples);
        compressor.update_curve();        
        
        while(offset < numsamples) {
            // cycle through samples
            float outL = 0.f;
            float outR = 0.f;
            float inL = ins[0][offset];
            float inR = ins[1][offset];
            // in level
            inR *= *params[param_level_in];
            inL *= *params[param_level_in];
            
            
            float leftAC = inL;
            float rightAC = inR;
            float leftSC = inL;
            float rightSC = inR;
            float leftMC = inL;
            float rightMC = inR;
            
            switch ((int)*params[param_sc_mode]) {
                default:
                case WIDEBAND:
                    compressor.process(leftAC, rightAC);
                    break;
                case DEESSER_WIDE:
                case DERUMBLER_WIDE:
                case WEIGHTED_1:
                case WEIGHTED_2:
                case WEIGHTED_3:
                case BANDPASS_2:
                    leftSC = f2L.process(f1L.process(leftSC));
                    rightSC = f2R.process(f1R.process(rightSC));
                    leftMC = leftSC;
                    rightMC = rightSC;
                    compressor.process(leftAC, rightAC, &leftSC, &rightSC);
                    break;
                case DEESSER_SPLIT:
                    leftSC = f2L.process(leftSC);
                    rightSC = f2R.process(rightSC);
                    leftMC = leftSC;
                    rightMC = rightSC;
                    compressor.process(leftSC, rightSC, &leftSC, &rightSC);
                    leftAC = f1L.process(leftAC);
                    rightAC = f1R.process(rightAC);
                    leftAC += leftSC;
                    rightAC += rightSC;
                    break;
                case DERUMBLER_SPLIT:
                    leftSC = f1L.process(leftSC);
                    rightSC = f1R.process(rightSC);
                    leftMC = leftSC;
                    rightMC = rightSC;
                    compressor.process(leftSC, rightSC);
                    leftAC = f2L.process(leftAC);
                    rightAC = f2R.process(rightAC);
                    leftAC += leftSC;
                    rightAC += rightSC;
                    break;
                case BANDPASS_1:
                    leftSC = f1L.process(leftSC);
                    rightSC = f1R.process(rightSC);
                    leftMC = leftSC;
                    rightMC = rightSC;
                    compressor.process(leftAC, rightAC, &leftSC, &rightSC);
                    break;
            }
            
            if(*params[param_sc_listen] > 0.f) {
                outL = leftMC;
                outR = rightMC;
            } else {
                outL = leftAC;
                outR = rightAC;
            }
            
            // send to output
            outs[0][offset] = outL;
            outs[1][offset] = outR;
            
            // clip LED's
            if(std::max(fabs(inL), fabs(inR)) > 1.f) {
                clip_in   = srate >> 3;
            }
            if(std::max(fabs(outL), fabs(outR)) > 1.f) {
                clip_out  = srate >> 3;
            }
            // rise up out meter
            meter_in = std::max(fabs(inL), fabs(inR));;
            meter_out = std::max(fabs(outL), fabs(outR));;
            
            // next sample
            ++offset;
        } // cycle trough samples
        f1L.sanitize();
        f1R.sanitize();
        f2L.sanitize();
        f2R.sanitize();
            
    }
    // draw meters
    SET_IF_CONNECTED(clip_in)
    SET_IF_CONNECTED(clip_out)
    SET_IF_CONNECTED(meter_in)
    SET_IF_CONNECTED(meter_out)
    // draw strip meter
    if(bypass > 0.5f) {
        if(params[param_compression] != NULL) {
            *params[param_compression] = 1.0f;
        }
    } else {
        if(params[param_compression] != NULL) {
            *params[param_compression] = compressor.get_comp_level();
        }
    }
    // whatever has to be returned x)
    return outputs_mask;
}
bool sidechaincompressor_audio_module::get_graph(int index, int subindex, float *data, int points, cairo_iface *context) const
{
    if (!is_active)
        return false;
    if (index == param_f1_freq && !subindex) {
        context->set_line_width(1.5);
        return ::get_graph(*this, subindex, data, points);
    } else if(index == param_compression) {
        return compressor.get_graph(subindex, data, points, context);
    }
    return false;
}

bool sidechaincompressor_audio_module::get_dot(int index, int subindex, float &x, float &y, int &size, cairo_iface *context) const
{
    if (!is_active)
        return false;
    if (index == param_compression) {
        return compressor.get_dot(subindex, x, y, size, context);
    }
    return false;
}

bool sidechaincompressor_audio_module::get_gridline(int index, int subindex, float &pos, bool &vertical, std::string &legend, cairo_iface *context) const
{
    if (!is_active)
        return false;
    if (index == param_compression) {
        return compressor.get_gridline(subindex, pos, vertical, legend, context);
    } else {
        return get_freq_gridline(subindex, pos, vertical, legend, context);
    }
//    return false;
}

int sidechaincompressor_audio_module::get_changed_offsets(int index, int generation, int &subindex_graph, int &subindex_dot, int &subindex_gridline) const
{
    if (!is_active)
        return false;
    if(index == param_compression) {
        return compressor.get_changed_offsets(generation, subindex_graph, subindex_dot, subindex_gridline);
    } else {
        //  (fabs(inertia_cutoff.get_last() - old_cutoff) + 100 * fabs(inertia_resonance.get_last() - old_resonance) + fabs(*params[par_mode] - old_mode) > 0.1f)
        if (*params[param_f1_freq] != f1_freq_old1
            or *params[param_f2_freq] != f2_freq_old1
            or *params[param_f1_level] != f1_level_old1
            or *params[param_f2_level] != f2_level_old1
            or *params[param_sc_mode] !=sc_mode_old1)
        {
            f1_freq_old1 = *params[param_f1_freq];
            f2_freq_old1 = *params[param_f2_freq];
            f1_level_old1 = *params[param_f1_level];
            f2_level_old1 = *params[param_f2_level];
            sc_mode_old1 = (CalfScModes)*params[param_sc_mode];
            last_generation++;
            subindex_graph = 0;
            subindex_dot = INT_MAX;
            subindex_gridline = INT_MAX;
        }
        else {
            subindex_graph = 0;
            subindex_dot = subindex_gridline = generation ? INT_MAX : 0;
        }
        if (generation == last_calculated_generation)
            subindex_graph = INT_MAX;
        return last_generation;
    }
    return false;
}

/// Deesser by Markus Schmidt
///
/// This module splits the signal in a sidechain- and a process signal.
/// The sidechain is processed through Krzystofs filters and compresses
/// the process signal via Thor's compression routine afterwards.
///////////////////////////////////////////////////////////////////////////////////////////////

deesser_audio_module::deesser_audio_module()
{
    is_active = false;
    srate = 0;
    last_generation = 0;
}

void deesser_audio_module::activate()
{
    is_active = true;
    // set all filters and strips
    compressor.activate();
    params_changed();
    detected = 0.f;
    detected_led = 0.f;
    clip_out = 0.f;
}
void deesser_audio_module::deactivate()
{
    is_active = false;
    compressor.deactivate();
}

void deesser_audio_module::params_changed()
{
    // set the params of all filters
    if(*params[param_f1_freq] != f1_freq_old or *params[param_f1_level] != f1_level_old
        or *params[param_f2_freq] != f2_freq_old or *params[param_f2_level] != f2_level_old
        or *params[param_f2_q] != f2_q_old) {
        float q = 0.707;
        
        hpL.set_hp_rbj((float)*params[param_f1_freq] * (1 - 0.17), q, (float)srate, *params[param_f1_level]);
        hpR.copy_coeffs(hpL);
        lpL.set_lp_rbj((float)*params[param_f1_freq] * (1 + 0.17), q, (float)srate);
        lpR.copy_coeffs(lpL);
        pL.set_peakeq_rbj((float)*params[param_f2_freq], *params[param_f2_q], *params[param_f2_level], (float)srate);
        pR.copy_coeffs(pL);
        f1_freq_old = *params[param_f1_freq];
        f1_level_old = *params[param_f1_level];
        f2_freq_old = *params[param_f2_freq];
        f2_level_old = *params[param_f2_level];
        f2_q_old = *params[param_f2_q];
    }
    // and set the compressor module
    compressor.set_params((float)*params[param_laxity], (float)*params[param_laxity] * 1.33, *params[param_threshold], *params[param_ratio], 2.8, *params[param_makeup], *params[param_detection], 0.f, *params[param_bypass], 0.f);
}

void deesser_audio_module::set_sample_rate(uint32_t sr)
{
    srate = sr;
    compressor.set_sample_rate(srate);
}

uint32_t deesser_audio_module::process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask)
{
    bool bypass = *params[param_bypass] > 0.5f;
    numsamples += offset;
    if(bypass) {
        // everything bypassed
        while(offset < numsamples) {
            outs[0][offset] = ins[0][offset];
            outs[1][offset] = ins[1][offset];
            ++offset;
        }
        // displays, too
        clip_out   = 0.f;
        detected = 0.f;
        detected_led = 0.f;
    } else {
        // process
        
        detected_led -= std::min(detected_led,  numsamples);
        clip_led   -= std::min(clip_led,  numsamples);
        compressor.update_curve();        
        
        while(offset < numsamples) {
            // cycle through samples
            float outL = 0.f;
            float outR = 0.f;
            float inL = ins[0][offset];
            float inR = ins[1][offset];
            
            
            float leftAC = inL;
            float rightAC = inR;
            float leftSC = inL;
            float rightSC = inR;
            float leftRC = inL;
            float rightRC = inR;
            float leftMC = inL;
            float rightMC = inR;
            
            leftSC = pL.process(hpL.process(leftSC));
            rightSC = pR.process(hpR.process(rightSC));
            leftMC = leftSC;
            rightMC = rightSC;
                    
            switch ((int)*params[param_mode]) {
                default:
                case WIDE:
                    compressor.process(leftAC, rightAC, &leftSC, &rightSC);
                    break;
                case SPLIT:
                    hpL.sanitize();
                    hpR.sanitize();
                    leftRC = hpL.process(leftRC);
                    rightRC = hpR.process(rightRC);
                    compressor.process(leftRC, rightRC, &leftSC, &rightSC);
                    leftAC = lpL.process(leftAC);
                    rightAC = lpR.process(rightAC);
                    leftAC += leftRC;
                    rightAC += rightRC;
                    break;
            }
            
            if(*params[param_sc_listen] > 0.f) {
                outL = leftMC;
                outR = rightMC;
            } else {
                outL = leftAC;
                outR = rightAC;
            }
            
            // send to output
            outs[0][offset] = outL;
            outs[1][offset] = outR;
            
            if(std::max(fabs(leftSC), fabs(rightSC)) > *params[param_threshold]) {
                detected_led   = srate >> 3;
            }
            if(std::max(fabs(leftAC), fabs(rightAC)) > 1.f) {
                clip_led   = srate >> 3;
            }
            if(clip_led > 0) {
                clip_out = 1.f;
            } else {
                clip_out = std::max(fabs(outL), fabs(outR));
            }
            detected = std::max(fabs(leftMC), fabs(rightMC));
            
            // next sample
            ++offset;
        } // cycle trough samples
        hpL.sanitize();
        hpR.sanitize();
        lpL.sanitize();
        lpR.sanitize();
        pL.sanitize();
        pR.sanitize();
    }
    // draw meters
    if(params[param_detected_led] != NULL) {
        *params[param_detected_led] = detected_led;
    }
    if(params[param_clip_out] != NULL) {
        *params[param_clip_out] = clip_out;
    }
    if(params[param_detected] != NULL) {
        *params[param_detected] = detected;
    }
    // draw strip meter
    if(bypass > 0.5f) {
        if(params[param_compression] != NULL) {
            *params[param_compression] = 1.0f;
        }
    } else {
        if(params[param_compression] != NULL) {
            *params[param_compression] = compressor.get_comp_level();
        }
    }
    // whatever has to be returned x)
    return outputs_mask;
}
bool deesser_audio_module::get_graph(int index, int subindex, float *data, int points, cairo_iface *context) const
{
    if (!is_active)
        return false;
    if (index == param_f1_freq && !subindex) {
        context->set_line_width(1.5);
        return ::get_graph(*this, subindex, data, points);
    }
    return false;
}

bool deesser_audio_module::get_gridline(int index, int subindex, float &pos, bool &vertical, std::string &legend, cairo_iface *context) const
{
    return get_freq_gridline(subindex, pos, vertical, legend, context);
    
//    return false;
}

int deesser_audio_module::get_changed_offsets(int index, int generation, int &subindex_graph, int &subindex_dot, int &subindex_gridline) const
{
    if (!is_active) {
        return false;
    } else {
        //  (fabs(inertia_cutoff.get_last() - old_cutoff) + 100 * fabs(inertia_resonance.get_last() - old_resonance) + fabs(*params[par_mode] - old_mode) > 0.1f)
        if (*params[param_f1_freq] != f1_freq_old1
            or *params[param_f2_freq] != f2_freq_old1
            or *params[param_f1_level] != f1_level_old1
            or *params[param_f2_level] != f2_level_old1
            or *params[param_f2_q] !=f2_q_old1)
        {
            f1_freq_old1 = *params[param_f1_freq];
            f2_freq_old1 = *params[param_f2_freq];
            f1_level_old1 = *params[param_f1_level];
            f2_level_old1 = *params[param_f2_level];
            f2_q_old1 = *params[param_f2_q];
            last_generation++;
            subindex_graph = 0;
            subindex_dot = INT_MAX;
            subindex_gridline = INT_MAX;
        }
        else {
            subindex_graph = 0;
            subindex_dot = subindex_gridline = generation ? INT_MAX : 0;
        }
        if (generation == last_calculated_generation)
            subindex_graph = INT_MAX;
        return last_generation;
    }
    return false;
}

/// Gain reduction module by Thor
/// All functions of this module are originally written
/// by Thor, while some features have been stripped (mainly stereo linking
/// and frequency correction as implemented in Sidechain Compressor above)
/// To save some CPU.
////////////////////////////////////////////////////////////////////////////////
gain_reduction_audio_module::gain_reduction_audio_module()
{
    is_active       = false;
    last_generation = 0;
}

void gain_reduction_audio_module::activate()
{
    is_active = true;
    linSlope   = 0.f;
    meter_out  = 0.f;
    meter_comp = 1.f;
    float l, r;
    l = r = 0.f;
    float byp = bypass;
    bypass = 0.0;
    process(l, r);
    bypass = byp;
}

void gain_reduction_audio_module::deactivate()
{
    is_active = false;
}

void gain_reduction_audio_module::update_curve()
{
    float linThreshold = threshold;
    float linKneeSqrt = sqrt(knee);
    linKneeStart = linThreshold / linKneeSqrt;
    adjKneeStart = linKneeStart*linKneeStart;
    float linKneeStop = linThreshold * linKneeSqrt;
    thres = log(linThreshold);
    kneeStart = log(linKneeStart);
    kneeStop = log(linKneeStop);
    compressedKneeStop = (kneeStop - thres) / ratio + thres;
}

void gain_reduction_audio_module::process(float &left, float &right, const float *det_left, const float *det_right)
{
    if(!det_left) {
        det_left = &left;
    }
    if(!det_right) {
        det_right = &right;
    }
    float gain = 1.f;
    if(bypass < 0.5f) {
        // this routine is mainly copied from thor's compressor module
        // greatest sounding compressor I've heard!
        bool rms = detection == 0;
        bool average = stereo_link == 0;
        float attack_coeff = std::min(1.f, 1.f / (attack * srate / 4000.f));
        float release_coeff = std::min(1.f, 1.f / (release * srate / 4000.f));
        
        float absample = average ? (fabs(*det_left) + fabs(*det_right)) * 0.5f : std::max(fabs(*det_left), fabs(*det_right));
        if(rms) absample *= absample;
            
        linSlope += (absample - linSlope) * (absample > linSlope ? attack_coeff : release_coeff);
        
        if(linSlope > 0.f) {
            gain = output_gain(linSlope, rms);
        }

        left *= gain * makeup;
        right *= gain * makeup;
        meter_out = std::max(fabs(left), fabs(right));;
        meter_comp = gain;
        detected = rms ? sqrt(linSlope) : linSlope;
    }
}

float gain_reduction_audio_module::output_level(float slope) const {
    return slope * output_gain(slope, false) * makeup;
}

float gain_reduction_audio_module::output_gain(float linSlope, bool rms) const {
    //this calculation is also thor's work
    if(linSlope > (rms ? adjKneeStart : linKneeStart)) {
        float slope = log(linSlope);
        if(rms) slope *= 0.5f;

        float gain = 0.f;
        float delta = 0.f;
        if(IS_FAKE_INFINITY(ratio)) {
            gain = thres;
            delta = 0.f;
        } else {
            gain = (slope - thres) / ratio + thres;
            delta = 1.f / ratio;
        }
        
        if(knee > 1.f && slope < kneeStop) {
            gain = hermite_interpolation(slope, kneeStart, kneeStop, kneeStart, compressedKneeStop, 1.f, delta);
        }
        
        return exp(gain - slope);
    }

    return 1.f;
}

void gain_reduction_audio_module::set_sample_rate(uint32_t sr)
{
    srate = sr;
}
void gain_reduction_audio_module::set_params(float att, float rel, float thr, float rat, float kn, float mak, float det, float stl, float byp, float mu)
{
    // set all params
    attack          = att;
    release         = rel;
    threshold       = thr;
    ratio           = rat;
    knee            = kn;
    makeup          = mak;
    detection       = det;
    stereo_link     = stl;
    bypass          = byp;
    mute            = mu;
    if(mute > 0.f) {
        meter_out  = 0.f;
        meter_comp = 1.f;
    }
}
float gain_reduction_audio_module::get_output_level() {
    // returns output level (max(left, right))
    return meter_out;
}
float gain_reduction_audio_module::get_comp_level() {
    // returns amount of compression
    return meter_comp;
}

bool gain_reduction_audio_module::get_graph(int subindex, float *data, int points, cairo_iface *context) const
{
    if (!is_active)
        return false;
    if (subindex > 1) // 1
        return false;
    for (int i = 0; i < points; i++)
    {
        float input = dB_grid_inv(-1.0 + i * 2.0 / (points - 1));
        if (subindex == 0)
            data[i] = dB_grid(input);
        else {
            float output = output_level(input);
            data[i] = dB_grid(output);
        }
    }
    if (subindex == (bypass > 0.5f ? 1 : 0) or mute > 0.1f)
        context->set_source_rgba(0.35, 0.4, 0.2, 0.3);
    else {
        context->set_source_rgba(0.35, 0.4, 0.2, 1);
        context->set_line_width(1.5);
    }
    return true;
}

bool gain_reduction_audio_module::get_dot(int subindex, float &x, float &y, int &size, cairo_iface *context) const
{
    if (!is_active)
        return false;
    if (!subindex)
    {
        if(bypass > 0.5f or mute > 0.f) {
            return false;
        } else {
            bool rms = detection == 0;
            float det = rms ? sqrt(detected) : detected;
            x = 0.5 + 0.5 * dB_grid(det);
            y = dB_grid(bypass > 0.5f or mute > 0.f ? det : output_level(det));
            return true;
        }
    }
    return false;
}

bool gain_reduction_audio_module::get_gridline(int subindex, float &pos, bool &vertical, std::string &legend, cairo_iface *context) const
{
    bool tmp;
    vertical = (subindex & 1) != 0;
    bool result = get_freq_gridline(subindex >> 1, pos, tmp, legend, context, false);
    if (result && vertical) {
        if ((subindex & 4) && !legend.empty()) {
            legend = "";
        }
        else {
            size_t pos = legend.find(" dB");
            if (pos != std::string::npos)
                legend.erase(pos);
        }
        pos = 0.5 + 0.5 * pos;
    }
    return result;
}

int gain_reduction_audio_module::get_changed_offsets(int generation, int &subindex_graph, int &subindex_dot, int &subindex_gridline) const
{
    subindex_graph = 0;
    subindex_dot = 0;
    subindex_gridline = generation ? INT_MAX : 0;

    if (fabs(threshold-old_threshold) + fabs(ratio - old_ratio) + fabs(knee - old_knee) + fabs(makeup - old_makeup) + fabs(detection - old_detection) + fabs(bypass - old_bypass) + fabs(mute - old_mute) > 0.000001f)
    {
        old_threshold = threshold;
        old_ratio     = ratio;
        old_knee      = knee;
        old_makeup    = makeup;
        old_detection = detection;
        old_bypass    = bypass;
        old_mute      = mute;
        last_generation++;
    }

    if (generation == last_generation)
        subindex_graph = 2;
    return last_generation;
}

/// Equalizer 12 Band by Markus Schmidt
///
/// This module is based on Krzysztof's filters. It provides a couple
/// of different chained filters.
///////////////////////////////////////////////////////////////////////////////////////////////

template<class BaseClass, bool has_lphp>
equalizerNband_audio_module<BaseClass, has_lphp>::equalizerNband_audio_module()
{
    is_active = false;
    srate = 0;
    last_generation = 0;
    clip_inL = clip_inR = clip_outL = clip_outR  = 0.f;
    meter_inL = meter_inR = meter_outL = meter_outR = 0.f;
}

template<class BaseClass, bool has_lphp>
void equalizerNband_audio_module<BaseClass, has_lphp>::activate()
{
    is_active = true;
    // set all filters
    params_changed();
}

template<class BaseClass, bool has_lphp>
void equalizerNband_audio_module<BaseClass, has_lphp>::deactivate()
{
    is_active = false;
}

static inline void copy_lphp(biquad_d2<float> filters[3][2])
{
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 2; j++)
            if (i || j)
                filters[i][j].copy_coeffs(filters[0][0]);
}

template<class BaseClass, bool has_lphp>
void equalizerNband_audio_module<BaseClass, has_lphp>::params_changed()
{
    // set the params of all filters
    
    // lp/hp first (if available)
    if (has_lphp)
    {
        hp_mode = (CalfEqMode)(int)*params[AM::param_hp_mode];
        lp_mode = (CalfEqMode)(int)*params[AM::param_lp_mode];

        float hpfreq = *params[AM::param_hp_freq], lpfreq = *params[AM::param_lp_freq];
        
        if(hpfreq != hp_freq_old) {
            hp[0][0].set_hp_rbj(hpfreq, 0.707, (float)srate, 1.0);
            copy_lphp(hp);
            hp_freq_old = hpfreq;
        }
        if(lpfreq != lp_freq_old) {
            lp[0][0].set_lp_rbj(lpfreq, 0.707, (float)srate, 1.0);
            copy_lphp(lp);
            lp_freq_old = lpfreq;
        }
    }
    
    // then shelves
    float hsfreq = *params[AM::param_hs_freq], hslevel = *params[AM::param_hs_level];
    float lsfreq = *params[AM::param_ls_freq], lslevel = *params[AM::param_ls_level];
    
    if(lsfreq != ls_freq_old or lslevel != ls_level_old) {
        lsL.set_lowshelf_rbj(lsfreq, 0.707, lslevel, (float)srate);
        lsR.copy_coeffs(lsL);
        ls_level_old = lslevel;
        ls_freq_old = lsfreq;
    }
    if(hsfreq != hs_freq_old or hslevel != hs_level_old) {
        hsL.set_highshelf_rbj(hsfreq, 0.707, hslevel, (float)srate);
        hsR.copy_coeffs(hsL);
        hs_level_old = hslevel;
        hs_freq_old = hsfreq;
    }
    for (int i = 0; i < AM::PeakBands; i++)
    {
        int offset = i * params_per_band;
        float freq = *params[AM::param_p1_freq + offset];
        float level = *params[AM::param_p1_level + offset];
        float q = *params[AM::param_p1_q + offset];
        if(freq != p_freq_old[i] or level != p_level_old[i] or q != p_q_old[i]) {
            pL[i].set_peakeq_rbj(freq, q, level, (float)srate);
            pR[i].copy_coeffs(pL[i]);
            p_freq_old[i] = freq;
            p_level_old[i] = level;
            p_q_old[i] = q;
        }
    }
}

template<class BaseClass, bool has_lphp>
inline void equalizerNband_audio_module<BaseClass, has_lphp>::process_hplp(float &left, float &right)
{
    if (!has_lphp)
        return;
    if (*params[AM::param_lp_active] > 0.f)
    {
        switch(lp_mode)
        {
            case MODE12DB:
                left = lp[0][0].process(left);
                right = lp[0][1].process(right);
                break;
            case MODE24DB:
                left = lp[1][0].process(lp[0][0].process(left));
                right = lp[1][1].process(lp[0][1].process(right));
                break;
            case MODE36DB:
                left = lp[2][0].process(lp[1][0].process(lp[0][0].process(left)));
                right = lp[2][1].process(lp[1][1].process(lp[0][1].process(right)));
                break;
        }
    }
    if (*params[AM::param_hp_active] > 0.f)
    {
        switch(hp_mode)
        {
            case MODE12DB:
                left = hp[0][0].process(left);
                right = hp[0][1].process(right);
                break;
            case MODE24DB:
                left = hp[1][0].process(hp[0][0].process(left));
                right = hp[1][1].process(hp[0][1].process(right));
                break;
            case MODE36DB:
                left = hp[2][0].process(hp[1][0].process(hp[0][0].process(left)));
                right = hp[2][1].process(hp[1][1].process(hp[0][1].process(right)));
                break;
        }
    }
}

template<class BaseClass, bool has_lphp>
uint32_t equalizerNband_audio_module<BaseClass, has_lphp>::process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask)
{
    bool bypass = *params[AM::param_bypass] > 0.f;
    numsamples += offset;
    if(bypass) {
        // everything bypassed
        while(offset < numsamples) {
            outs[0][offset] = ins[0][offset];
            outs[1][offset] = ins[1][offset];
            ++offset;
        }
        // displays, too
        clip_inL = clip_inR = clip_outL = clip_outR = 0.f;
        meter_inL = meter_inR = meter_outL = meter_outR = 0.f;
    } else {
        
        clip_inL    -= std::min(clip_inL,  numsamples);
        clip_inR    -= std::min(clip_inR,  numsamples);
        clip_outL   -= std::min(clip_outL, numsamples);
        clip_outR   -= std::min(clip_outR, numsamples);
        meter_inL = 0.f;
        meter_inR = 0.f;
        meter_outL = 0.f;
        meter_outR = 0.f;
        
        // process
        while(offset < numsamples) {
            // cycle through samples
            float outL = 0.f;
            float outR = 0.f;
            float inL = ins[0][offset];
            float inR = ins[1][offset];
            // in level
            inR *= *params[AM::param_level_in];
            inL *= *params[AM::param_level_in];
            
            float procL = inL;
            float procR = inR;
            
            // all filters in chain
            process_hplp(procL, procR);
            if(*params[AM::param_ls_active] > 0.f) {
                procL = lsL.process(procL);
                procR = lsR.process(procR);
            }
            if(*params[AM::param_hs_active] > 0.f) {
                procL = hsL.process(procL);
                procR = hsR.process(procR);
            }
            for (int i = 0; i < AM::PeakBands; i++)
            {
                if(*params[AM::param_p1_active + i * params_per_band] > 0.f) {
                    procL = pL[i].process(procL);
                    procR = pR[i].process(procR);
                }
            }
            
            outL = procL * *params[AM::param_level_out];
            outR = procR * *params[AM::param_level_out];
            
            // send to output
            outs[0][offset] = outL;
            outs[1][offset] = outR;
            
            // clip LED's
            if(inL > 1.f) {
                clip_inL  = srate >> 3;
            }
            if(inR > 1.f) {
                clip_inR  = srate >> 3;
            }
            if(outL > 1.f) {
                clip_outL = srate >> 3;
            }
            if(outR > 1.f) {
                clip_outR = srate >> 3;
            }
            // set up in / out meters
            if(inL > meter_inL) {
                meter_inL = inL;
            }
            if(inR > meter_inR) {
                meter_inR = inR;
            }
            if(outL > meter_outL) {
                meter_outL = outL;
            }
            if(outR > meter_outR) {
                meter_outR = outR;
            }
            
            // next sample
            ++offset;
        } // cycle trough samples
        // clean up
        for(int i = 0; i < 3; ++i) {
            hp[i][0].sanitize();
            hp[i][1].sanitize();
            lp[i][0].sanitize();
            lp[i][1].sanitize();
        }
        lsL.sanitize();
        hsR.sanitize();
        for(int i = 0; i < AM::PeakBands; ++i) {
            pL[i].sanitize();
            pR[i].sanitize();
        }
    }
    // draw meters
    SET_IF_CONNECTED(clip_inL)
    SET_IF_CONNECTED(clip_inR)
    SET_IF_CONNECTED(clip_outL)
    SET_IF_CONNECTED(clip_outR)
    SET_IF_CONNECTED(meter_inL)
    SET_IF_CONNECTED(meter_inR)
    SET_IF_CONNECTED(meter_outL)
    SET_IF_CONNECTED(meter_outR)
    // whatever has to be returned x)
    return outputs_mask;
}

template<class BaseClass, bool has_lphp>
bool equalizerNband_audio_module<BaseClass, has_lphp>::get_graph(int index, int subindex, float *data, int points, cairo_iface *context) const
{
    if (!is_active)
        return false;
    if (index == AM::param_p1_freq && !subindex) {
        context->set_line_width(1.5);
        return ::get_graph(*this, subindex, data, points);
    }
    return false;
}

template<class BaseClass, bool has_lphp>
bool equalizerNband_audio_module<BaseClass, has_lphp>::get_gridline(int index, int subindex, float &pos, bool &vertical, std::string &legend, cairo_iface *context) const
{
    if (!is_active) {
        return false;
    } else {
        return get_freq_gridline(subindex, pos, vertical, legend, context);
    }
}

template<class BaseClass, bool has_lphp>
int equalizerNband_audio_module<BaseClass, has_lphp>::get_changed_offsets(int index, int generation, int &subindex_graph, int &subindex_dot, int &subindex_gridline) const
{
    if (!is_active) {
        return false;
    } else {
        bool changed = false;
        for (int i = 0; i < graph_param_count && !changed; i++)
        {
            if (*params[AM::first_graph_param + i] != old_params_for_graph[i])
                changed = true;
        }
        if (changed)
        {
            for (int i = 0; i < graph_param_count; i++)
                old_params_for_graph[i] = *params[AM::first_graph_param + i];
            
            last_generation++;
            subindex_graph = 0;
            subindex_dot = INT_MAX;
            subindex_gridline = INT_MAX;
        }
        else {
            subindex_graph = 0;
            subindex_dot = subindex_gridline = generation ? INT_MAX : 0;
        }
        if (generation == last_calculated_generation)
            subindex_graph = INT_MAX;
        return last_generation;
    }
    return false;
}

static inline float adjusted_lphp_gain(const float *const *params, int param_active, int param_mode, const biquad_d2<float> &filter, float freq, float srate)
{
    if(*params[param_active] > 0.f) {
        float gain = filter.freq_gain(freq, srate);
        switch((int)*params[param_mode]) {
            case MODE12DB:
                return gain;
            case MODE24DB:
                return gain * gain;
            case MODE36DB:
                return gain * gain * gain;
        }
    }
    return 1;
}

template<class BaseClass, bool use_hplp>
float equalizerNband_audio_module<BaseClass, use_hplp>::freq_gain(int index, double freq, uint32_t sr) const
{
    float ret = 1.f;
    if (use_hplp)
    {
        ret *= adjusted_lphp_gain(params, AM::param_hp_active, AM::param_hp_mode, hp[0][0], freq, (float)sr);
        ret *= adjusted_lphp_gain(params, AM::param_lp_active, AM::param_lp_mode, lp[0][0], freq, (float)sr);
    }
    ret *= (*params[AM::param_ls_active] > 0.f) ? lsL.freq_gain(freq, sr) : 1;
    ret *= (*params[AM::param_hs_active] > 0.f) ? hsL.freq_gain(freq, sr) : 1;
    for (int i = 0; i < PeakBands; i++)
        ret *= (*params[AM::param_p1_active + i * params_per_band] > 0.f) ? pL[i].freq_gain(freq, (float)sr) : 1;
    return ret;
}

template class equalizerNband_audio_module<equalizer5band_metadata, false>;
template class equalizerNband_audio_module<equalizer8band_metadata, true>;
template class equalizerNband_audio_module<equalizer12band_metadata, true>;

/// LFO module by Markus
/// This module provides simple LFO's (sine=0, triangle=1, square=2, saw_up=3, saw_down=4)
/// get_value() returns a value between -1 and 1
////////////////////////////////////////////////////////////////////////////////
lfo_audio_module::lfo_audio_module()
{
    is_active       = false;
    phase = 0.f;
}

void lfo_audio_module::activate()
{
    is_active = true;
    phase = 0.f;
}

void lfo_audio_module::deactivate()
{
    is_active = false;
}

float lfo_audio_module::get_value()
{
    return get_value_from_phase(phase, offset) * amount;
}

float lfo_audio_module::get_value_from_phase(float ph, float off) const
{
    float val = 0.f;
    float phs = ph + off;
    if (phs >= 1.0)
        phs = fmod(phs, 1.f);
    switch (mode) {
        default:
        case 0:
            // sine
            val = sin((phs * 360.f) * M_PI / 180);
            break;
        case 1:
            // triangle
            if(phs > 0.75)
                val = (phs - 0.75) * 4 - 1;
            else if(phs > 0.5)
                val = (phs - 0.5) * 4 * -1;
            else if(phs > 0.25)
                val = 1 - (phs - 0.25) * 4;
            else
                val = phs * 4;
            break;
        case 2:
            // square
            val = (phs < 0.5) ? -1 : +1;
            break;
        case 3:
            // saw up
                val = phs * 2.f - 1;
            break;
        case 4:
            // saw down
            val = 1 - phs * 2.f;
            break;
    }
    return val;
}

void lfo_audio_module::advance(uint32_t count)
{
    //this function walks from 0.f to 1.f and starts all over again
    phase += count * freq * (1.0 / srate);
    if (phase >= 1.0)
        phase = fmod(phase, 1.f);
}

void lfo_audio_module::set_phase(float ph)
{
    //set the phase from outsinde
    phase = fabs(ph);
    if (phase >= 1.0)
        phase = fmod(phase, 1.f);
}

void lfo_audio_module::set_params(float f, int m, float o, uint32_t sr, float a)
{
    // freq: a value in Hz
    // mode: sine=0, triangle=1, square=2, saw_up=3, saw_down=4
    // offset: value between 0.f and 1.f to offset the lfo in time
    freq = f;
    mode = m;
    offset = o;
    srate = sr;
    amount = a;
}

bool lfo_audio_module::get_graph(float *data, int points, cairo_iface *context) const
{
    if (!is_active)
        return false;
    for (int i = 0; i < points; i++) {
        float ph = (float)i / (float)points;
        data[i] = get_value_from_phase(ph, offset) * amount;
    }
    return true;
}

bool lfo_audio_module::get_dot(float &x, float &y, int &size, cairo_iface *context) const
{
    if (!is_active)
        return false;
    float phs = phase + offset;
    if (phs >= 1.0)
        phs = fmod(phs, 1.f);
    x = phase;
    y = get_value_from_phase(phase, offset) * amount;
    return true;
}

/// Pulsator by Markus Schmidt
///
/// This module provides a couple
/// of different LFO's for modulating the level of a signal.
///////////////////////////////////////////////////////////////////////////////////////////////

pulsator_audio_module::pulsator_audio_module()
{
    is_active = false;
    srate = 0;
//    last_generation = 0;
    clip_inL    = 0.f;
    clip_inR    = 0.f;
    clip_outL   = 0.f;
    clip_outR   = 0.f;
    meter_inL  = 0.f;
    meter_inR  = 0.f;
    meter_outL = 0.f;
    meter_outR = 0.f;
}

void pulsator_audio_module::activate()
{
    is_active = true;
    lfoL.activate();
    lfoR.activate();
    params_changed();
}
void pulsator_audio_module::deactivate()
{
    is_active = false;
    lfoL.deactivate();
    lfoR.deactivate();
}

void pulsator_audio_module::params_changed()
{
    lfoL.set_params(*params[param_freq], *params[param_mode], 0.f, srate, *params[param_amount]);
    lfoR.set_params(*params[param_freq], *params[param_mode], *params[param_offset], srate, *params[param_amount]);
    clear_reset = false;
    if (*params[param_reset] >= 0.5) {
        clear_reset = true;
        lfoL.set_phase(0.f);
        lfoR.set_phase(0.f);
    }
}

void pulsator_audio_module::set_sample_rate(uint32_t sr)
{
    srate = sr;
}

uint32_t pulsator_audio_module::process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask)
{
    bool bypass = *params[param_bypass] > 0.5f;
    uint32_t samples = numsamples + offset;
    if(bypass) {
        // everything bypassed
        while(offset < samples) {
            outs[0][offset] = ins[0][offset];
            outs[1][offset] = ins[1][offset];
            ++offset;
        }
        // displays, too
        clip_inL    = 0.f;
        clip_inR    = 0.f;
        clip_outL   = 0.f;
        clip_outR   = 0.f;
        meter_inL  = 0.f;
        meter_inR  = 0.f;
        meter_outL = 0.f;
        meter_outR = 0.f;
        
        // LFO's should go on
        lfoL.advance(numsamples);
        lfoR.advance(numsamples);
        
    } else {
        
        clip_inL    -= std::min(clip_inL,  numsamples);
        clip_inR    -= std::min(clip_inR,  numsamples);
        clip_outL   -= std::min(clip_outL, numsamples);
        clip_outR   -= std::min(clip_outR, numsamples);
        meter_inL = 0.f;
        meter_inR = 0.f;
        meter_outL = 0.f;
        meter_outR = 0.f;
        
        // process
        while(offset < samples) {
            // cycle through samples
            float outL = 0.f;
            float outR = 0.f;
            float inL = ins[0][offset];
            float inR = ins[1][offset];
            // in level
            inR *= *params[param_level_in];
            inL *= *params[param_level_in];
            
            if(*params[param_mono] > 0.5) {
                inL = (inL + inR) * 0.5;
                inR = inL;
            }
            float procL = inL;
            float procR = inR;
            
            procL *= (lfoL.get_value() * 0.5 + *params[param_amount] / 2);
            procR *= (lfoR.get_value() * 0.5 + *params[param_amount] / 2);
            
            outL = procL + inL * (1 - *params[param_amount]);
            outR = procR + inR * (1 - *params[param_amount]);
            
            outL *=  *params[param_level_out];
            outR *=  *params[param_level_out];
            
            // send to output
            outs[0][offset] = outL;
            outs[1][offset] = outR;
            
            // clip LED's
            if(inL > 1.f) {
                clip_inL  = srate >> 3;
            }
            if(inR > 1.f) {
                clip_inR  = srate >> 3;
            }
            if(outL > 1.f) {
                clip_outL = srate >> 3;
            }
            if(outR > 1.f) {
                clip_outR = srate >> 3;
            }
            // set up in / out meters
            if(inL > meter_inL) {
                meter_inL = inL;
            }
            if(inR > meter_inR) {
                meter_inR = inR;
            }
            if(outL > meter_outL) {
                meter_outL = outL;
            }
            if(outR > meter_outR) {
                meter_outR = outR;
            }
            
            // next sample
            ++offset;
            
            // advance lfo's
            lfoL.advance(1);
            lfoR.advance(1);
        } // cycle trough samples
    }
    // draw meters
    SET_IF_CONNECTED(clip_inL)
    SET_IF_CONNECTED(clip_inR)
    SET_IF_CONNECTED(clip_outL)
    SET_IF_CONNECTED(clip_outR)
    SET_IF_CONNECTED(meter_inL)
    SET_IF_CONNECTED(meter_inR)
    SET_IF_CONNECTED(meter_outL)
    SET_IF_CONNECTED(meter_outR)
    // whatever has to be returned x)
    return outputs_mask;
}

bool pulsator_audio_module::get_graph(int index, int subindex, float *data, int points, cairo_iface *context) const
{
    if (!is_active) {
        return false;
    } else if(index == param_freq) {
        if(subindex == 0) {
            context->set_source_rgba(0.35, 0.4, 0.2, 1);
            return lfoL.get_graph(data, points, context);
        }
        if(subindex == 1) {
            context->set_source_rgba(0.35, 0.4, 0.2, 0.5);
            return lfoR.get_graph(data, points, context);
        }
    }
    return false;
}

bool pulsator_audio_module::get_dot(int index, int subindex, float &x, float &y, int &size, cairo_iface *context) const
{
    if (!is_active) {
        return false;
    } else if(index == param_freq) {
        if(subindex == 0) {
            context->set_source_rgba(0.35, 0.4, 0.2, 1);
            return lfoL.get_dot(x, y, size, context);
        }
        if(subindex == 1) {
            context->set_source_rgba(0.35, 0.4, 0.2, 0.5);
            return lfoR.get_dot(x, y, size, context);
        }
        return false;
    }
    return false;
}

bool pulsator_audio_module::get_gridline(int index, int subindex, float &pos, bool &vertical, std::string &legend, cairo_iface *context) const
{
   if (index == param_freq && !subindex)
    {
        pos = 0;
        vertical = false;
        return true;
    }
    return false;
}
