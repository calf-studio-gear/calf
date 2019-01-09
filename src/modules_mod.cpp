/* Calf DSP plugin pack
 * Modulation effect plugins
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
 * Boston, MA  02110-1301  USA
 */
#include <limits.h>
#include <memory.h>
#include <calf/giface.h>
#include <calf/modules_mod.h>

using namespace dsp;
using namespace calf_plugins;

#define SET_IF_CONNECTED(name) if (params[AM::param_##name] != NULL) *params[AM::param_##name] = name;

/**********************************************************************
 * FLANGER by Krzysztof Foltman
**********************************************************************/

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
    int meter[] = {param_meter_inL,  param_meter_inR, param_meter_outL, param_meter_outR};
    int clip[]  = {param_clip_inL, param_clip_inR, param_clip_outL, param_clip_outR};
    meters.init(params, meter, clip, 4, srate);
}

void flanger_audio_module::deactivate() {
    is_active = false;
}

void flanger_audio_module::params_changed()
{
    float dry = *params[par_dryamount];
    float wet = *params[par_amount];
    float rate = *params[par_rate]; // 0.01*pow(1000.0f,*params[par_rate]);
    float min_delay = *params[par_delay] / 1000.0;
    float mod_depth = *params[par_depth] / 1000.0;
    float fb = *params[par_fb];
    int lfo_active = *params[param_lfo];
    left.set_dry(dry); right.set_dry(dry);
    left.set_wet(wet); right.set_wet(wet);
    left.set_rate(rate); right.set_rate(rate);
    left.set_min_delay(min_delay); right.set_min_delay(min_delay);
    left.set_mod_depth(mod_depth); right.set_mod_depth(mod_depth);
    left.set_fb(fb); right.set_fb(fb);
    left.set_lfo_active(lfo_active); right.set_lfo_active(lfo_active);
    
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
bool flanger_audio_module::get_graph(int index, int subindex, int phase, float *data, int points, cairo_iface *context, int *mode) const
{
    if (!is_active || !phase || subindex >= 2)
        return false;
    set_channel_color(context, subindex);
    return ::get_graph(*this, subindex, data, points, 32, 0);
}
bool flanger_audio_module::get_gridline(int index, int subindex, int phase, float &pos, bool &vertical, std::string &legend, cairo_iface *context) const
{
    if (!is_active || phase)
        return false;
    return get_freq_gridline(subindex, pos, vertical, legend, context, true, 32, 0);
}
bool flanger_audio_module::get_layers(int index, int generation, unsigned int &layers) const
{
    layers = LG_REALTIME_GRAPH | (generation ? 0 : LG_CACHE_GRID);
    return true;
}
float flanger_audio_module::freq_gain(int subindex, float freq) const
{
    return (subindex ? right : left).freq_gain(freq, srate);                
}


/**********************************************************************
 * PHASER by Krzysztof Foltman
**********************************************************************/


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
    int meter[] = {param_meter_inL,  param_meter_inR, param_meter_outL, param_meter_outR};
    int clip[]  = {param_clip_inL, param_clip_inR, param_clip_outL, param_clip_outR};
    meters.init(params, meter, clip, 4, srate);
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

void phaser_audio_module::params_changed()
{
    float dry = *params[par_dryamount];
    float wet = *params[par_amount];
    float rate = *params[par_rate]; // 0.01*pow(1000.0f,*params[par_rate]);
    float base_frq = *params[par_freq];
    float mod_depth = *params[par_depth];
    float fb = *params[par_fb];
    int lfo_active = *params[param_lfo];
    int stages = (int)*params[par_stages];
    left.set_dry(dry); right.set_dry(dry);
    left.set_wet(wet); right.set_wet(wet);
    left.set_rate(rate); right.set_rate(rate);
    left.set_base_frq(base_frq); right.set_base_frq(base_frq);
    left.set_mod_depth(mod_depth); right.set_mod_depth(mod_depth);
    left.set_fb(fb); right.set_fb(fb);
    left.set_stages(stages); right.set_stages(stages);
    left.set_lfo_active(lfo_active); right.set_lfo_active(lfo_active);
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
bool phaser_audio_module::get_gridline(int index, int subindex, int phase, float &pos, bool &vertical, std::string &legend, cairo_iface *context) const
{
    if (!is_active || phase)
        return false;
    return get_freq_gridline(subindex, pos, vertical, legend, context, true, 32, 0);
}
bool phaser_audio_module::get_graph(int index, int subindex, int phase, float *data, int points, cairo_iface *context, int *mode) const
{
    if (!is_active || subindex >= 2 || !phase)
        return false;
    set_channel_color(context, subindex);
    return ::get_graph(*this, subindex, data, points, 32, 0);
}
bool phaser_audio_module::get_layers(int index, int generation, unsigned int &layers) const
{
    layers = LG_REALTIME_GRAPH | (generation ? 0 : LG_CACHE_GRID);
    return true;
}

float phaser_audio_module::freq_gain(int subindex, float freq) const
{
    return (subindex ? right : left).freq_gain(freq, srate);                
}


/**********************************************************************
 * ROTARY SPEAKER by Krzysztof Foltman
**********************************************************************/

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
    int meter[] = {param_meter_inL,  param_meter_inR, param_meter_outL, param_meter_outR};
    int clip[]  = {param_clip_inL, param_clip_inR, param_clip_outL, param_clip_outR};
    meters.init(params, meter, clip, 4, srate);
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

void rotary_speaker_audio_module::control_change(int /*channel*/, int ctl, int val)
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
    bool bypassed = bypass.update(*params[param_bypass] > 0.5f, nsamples);
    if (bypassed) {
        for (unsigned int i = offset; i < nsamples + offset; i++) {
            outs[0][i] = ins[0][i];
            outs[1][i] = ins[1][i];
            float values[] = {0,0,0,0};
            meters.process(values);
        }
    } else {
        if (true)
        {
            crossover2l.set_bp_rbj(2000.f, 0.7, (float)srate);
            crossover2r.copy_coeffs(crossover2l);
            damper1l.set_bp_rbj(1000.f*pow(4.0, *params[par_test]), 0.7, (float)srate);
            damper1r.copy_coeffs(damper1l);
        }
        else
        {
            crossover2l.set_hp_rbj(800.f, 0.7, (float)srate);
            crossover2r.copy_coeffs(crossover2l);
        }
        int shift = (int)(300000 * (*params[par_shift])), pdelta = (int)(300000 * (*params[par_spacing]));
        int md = (int)(100 * (*params[par_moddepth]));
        float mix = 0.5 * (1.0 - *params[par_micdistance]);
        float mix2 = *params[par_reflection];
        float mix3 = mix2 * mix2;
        double am_depth = *params[par_am_depth];
        for (unsigned int i = 0; i < nsamples; i++) {
            float in_l = ins[0][i + offset], in_r = ins[1][i + offset];
            in_l *= *params[param_level_in];
            in_r *= *params[param_level_in];
            double in_mono = atan(0.5f * (in_l + in_r));
            
            int xl = pseudo_sine_scl(phase_l), yl = pseudo_sine_scl(phase_l + 0x40000000);
            int xh = pseudo_sine_scl(phase_h), yh = pseudo_sine_scl(phase_h + 0x40000000);
            // printf("%d %d %d\n", shift, pdelta, shift + pdelta + 20 * xl);
            meter_l = xl;
            meter_h = xh;
            // float out_hi_l = in_mono - delay.get_interp_1616(shift + md * xh) + delay.get_interp_1616(shift + md * 65536 + pdelta - md * yh) - delay.get_interp_1616(shift + md * 65536 + pdelta + pdelta - md * xh);
            // float out_hi_r = in_mono + delay.get_interp_1616(shift + md * 65536 - md * yh) - delay.get_interp_1616(shift + pdelta + md * xh) + delay.get_interp_1616(shift + pdelta + pdelta + md * yh);
            float fm_hi_l = delay.get_interp_1616(shift + md * xh) - mix2 * delay.get_interp_1616(shift + md * 65536 + pdelta - md * yh) + mix3 * delay.get_interp_1616(shift + md * 65536 + pdelta + pdelta - md * xh);
            float fm_hi_r = delay.get_interp_1616(shift + md * 65536 - md * yh) - mix2 * delay.get_interp_1616(shift + pdelta + md * xh) + mix3 * delay.get_interp_1616(shift + pdelta + pdelta + md * yh);
            float out_hi_l = lerp(in_mono, (double)damper1l.process(fm_hi_l), lerp(0.5, xh * 1.0 / 65536.0, am_depth));
            float out_hi_r = lerp(in_mono, (double)damper1r.process(fm_hi_r), lerp(0.5, yh * 1.0 / 65536.0, am_depth));
    
            float out_lo_l = lerp(in_mono, (double)delay.get_interp_1616(shift + (md * xl >> 2)), lerp(0.5, yl * 1.0 / 65536.0, am_depth)); // + delay.get_interp_1616(shift + md * 65536 + pdelta - md * yl);
            float out_lo_r = lerp(in_mono, (double)delay.get_interp_1616(shift + (md * yl >> 2)), lerp(0.5, xl * 1.0 / 65536.0, am_depth)); // + delay.get_interp_1616(shift + md * 65536 + pdelta - md * yl);
            
            out_hi_l = crossover2l.process(out_hi_l); // sanitize(out_hi_l);
            out_hi_r = crossover2r.process(out_hi_r); // sanitize(out_hi_r);
            out_lo_l = crossover1l.process(out_lo_l); // sanitize(out_lo_l);
            out_lo_r = crossover1r.process(out_lo_r); // sanitize(out_lo_r);
            
            float out_l = out_hi_l + out_lo_l;
            float out_r = out_hi_r + out_lo_r;
            
            float mic_l = out_l + mix * (out_r - out_l);
            float mic_r = out_r + mix * (out_l - out_r);
            
            outs[0][i + offset] = mic_l * *params[param_level_out];
            outs[1][i + offset] = mic_r * *params[param_level_out];
            delay.put(in_mono);
            phase_l += dphase_l;
            phase_h += dphase_h;
            
            float values[] = {in_l, in_r, outs[0][i + offset], outs[1][i + offset]};
            meters.process(values);
        }
        crossover1l.sanitize();
        crossover1r.sanitize();
        crossover2l.sanitize();
        crossover2r.sanitize();
        damper1l.sanitize();
        damper1r.sanitize();
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
        bypass.crossfade(ins, outs, 2, offset, nsamples);
    }
    meters.fall(nsamples);
    if(params[par_meter_l] != NULL) {
        *params[par_meter_l] = (float)meter_l / 65536.0;
    }
    if(params[par_meter_h] != NULL) {
        *params[par_meter_h] = (float)meter_h / 65536.0;
    }
    return outputs_mask;
}

/**********************************************************************
 * MULTI CHORUS by Krzysztof Foltman
**********************************************************************/

multichorus_audio_module::multichorus_audio_module()
{
    is_active = false;
    last_r_phase = -1;
    freq_old = freq2_old = q_old = -1;
    redraw_graph = true;
    redraw_sine = true;
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
    last_r_phase = -1;
    left.setup(sr);
    right.setup(sr);
    int meter[] = {param_meter_inL,  param_meter_inR, param_meter_outL, param_meter_outR};
    int clip[]  = {param_clip_inL, param_clip_inR, param_clip_outL, param_clip_outR};
    meters.init(params, meter, clip, 4, srate);
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
    int lfo_active = *params[param_lfo];
    left.set_dry(dry); right.set_dry(dry);
    left.set_wet(wet); right.set_wet(wet);
    left.set_rate(rate); right.set_rate(rate);
    left.set_min_delay(min_delay); right.set_min_delay(min_delay);
    left.set_mod_depth(mod_depth); right.set_mod_depth(mod_depth);
    left.set_lfo_active(lfo_active); right.set_lfo_active(lfo_active);
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
    if (*params[par_freq] != freq_old || *params[par_freq2] != freq2_old || *params[par_q] != q_old) {
        left.post.f1.set_bp_rbj(*params[par_freq], *params[par_q], srate);
        left.post.f2.set_bp_rbj(*params[par_freq2], *params[par_q], srate);
        right.post.f1.copy_coeffs(left.post.f1);
        right.post.f2.copy_coeffs(left.post.f2);
        freq_old = *params[par_freq];
        freq2_old = *params[par_freq2];
        q_old = *params[par_q];
        redraw_graph = true;
    }
    redraw_sine = true;
}

uint32_t multichorus_audio_module::process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask)
{
    left.process(outs[0] + offset, ins[0] + offset, numsamples, *params[param_on] > 0.5, *params[param_level_in], *params[param_level_out]);
    right.process(outs[1] + offset, ins[1] + offset, numsamples, *params[param_on] > 0.5, *params[param_level_in], *params[param_level_out]);
    for (uint32_t i = offset; i < offset + numsamples; i++) {
        float values[] = {ins[0][i] * *params[param_level_in], ins[1][i] * *params[param_level_in], outs[0][i], outs[1][i]};
        meters.process(values);
    }
    meters.fall(numsamples);
    return outputs_mask; // XXXKF allow some delay after input going blank
}

bool multichorus_audio_module::get_layers(int index, int generation, unsigned int &layers) const
{
    layers = 0;
    // frequency response
    if (index == par_delay) {
        layers = (generation ? 0 : LG_CACHE_GRID) | (redraw_graph ? LG_CACHE_GRAPH : 0) | LG_REALTIME_GRAPH;
    }
    // sine display
    if (index == par_rate) {
        layers = LG_REALTIME_DOT | (redraw_sine ? LG_CACHE_GRAPH : 0);
    }
    // dot display
    if (index == par_depth)
        layers = LG_REALTIME_DOT;
    return true;
}

bool multichorus_audio_module::get_graph(int index, int subindex, int phase, float *data, int points, cairo_iface *context, int *mode) const
{
    if (!is_active)
        return false;
    
    // the filter graph (cached) in frequency response
    if (index == par_delay && subindex == 2 && !phase) {
        context->set_source_rgba(0.15, 0.2, 0.0, 0.8);
        redraw_graph = false;
        return ::get_graph(*this, subindex, data, points, 64, 0.5);
    }
    // the realtime graphs in frequency response
    if (index == par_delay && subindex < 2 && phase) {
        set_channel_color(context, subindex);
        context->set_line_width(1);
        return ::get_graph(*this, subindex, data, points, 64, 0.5);
    }
    
    // the sine curves in modulation display
    if (index == par_rate && subindex < (int)*params[par_voices] && !phase) {
        const sine_multi_lfo<float, 8> &lfo = left.lfo;
        for (int i = 0; i < points; i++) {
            float _phase = i * 2 * M_PI / points;
            // original -65536 to 65535 value
            float orig = subindex * lfo.voice_offset + ((lfo.voice_depth >> (30-13)) * 65536.0 * (0.95 * sin(_phase) + 1)/ 8192.0) - 65536;
            // scale to -1..1
            data[i] = orig / 65536.0;
        }
        redraw_sine = false;
        return true;
    }
    return false;
}

bool multichorus_audio_module::get_dot(int index, int subindex, int phase, float &x, float &y, int &size, cairo_iface *context) const
{
    int voice = subindex >> 1;
    int nvoices = (int)*params[par_voices];
    if (!is_active
     || !phase
     || (index != par_rate && index != par_depth)
     || voice >= nvoices)
        return false;

    float unit = (1 - *params[par_overlap]);
    float scw = 1 + unit * (nvoices - 1);
    const sine_multi_lfo<float, 8> &lfo = (subindex & 1 ? right : left).lfo;
    
    // the display with the sine curves
    if (index == par_rate) {
        x = (double)(lfo.phase + lfo.vphase * voice) / 4096.0;
        y = 0.95 * sin(x * 2 * M_PI);
        y = (voice * unit + (y + 1) / 2) / scw * 2 - 1;
    }
    // the tiny dot display
    if (index == par_depth) {
        double ph = (double)(lfo.phase + lfo.vphase * voice) / 4096.0;
        x = 0.5 + 0.5 * sin(ph * 2 * M_PI);
        y = subindex & 1 ? -0.5 : 0.5;
        x = (voice * unit + x) / scw;
    }
    return true;
}

bool multichorus_audio_module::get_gridline(int index, int subindex, int phase, float &pos, bool &vertical, std::string &legend, cairo_iface *context) const
{
    if (index == par_delay && !phase && is_active)
        return get_freq_gridline(subindex, pos, vertical, legend, context, true, 64, 0.5);
    return false;
}

float multichorus_audio_module::freq_gain(int subindex, float freq) const
{
    if (subindex == 2)
        return *params[par_amount] * left.post.freq_gain(freq, srate);
    return (subindex ? right : left).freq_gain(freq, srate);                
}

/**********************************************************************
 * PULSATOR by Markus Schmidt
**********************************************************************/

pulsator_audio_module::pulsator_audio_module()
{
    is_active = false;
    srate = 0;
    mode_old     = -1;
    amount_old   = -1;
    offset_l_old = -1;
    offset_r_old = -1;
    reset_old    = -1;
    pwidth_old   = 0;
    freq_old     = 0;
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
    clear_reset = false;
    if (*params[param_reset] >= 0.5 && reset_old != true) {
        clear_reset = true;
        lfoL.set_phase(0.f);
        lfoR.set_phase(0.f);
        reset_old = true;
    }
    if (*params[param_reset] < 0.5)
        reset_old = false;
    
    double freq = 2;
    freq = convert_periodic(*params[param_bpm + (int)((periodic_unit)int(*params[param_timing]))],
                                   (periodic_unit)int(*params[param_timing]), UNIT_HZ);
    if (freq != freq_old) {
        freq_old = freq;
        clear_reset = true;
    }
    
    if (*params[param_mode]   != mode_old
     || *params[param_amount] != amount_old
     || *params[param_offset_l] != offset_l_old
     || *params[param_offset_r] != offset_r_old
     || *params[param_pwidth] != pwidth_old
     || clear_reset) {
        float pw = 1;
        switch (int(*params[param_pwidth])) {
            case 0: pw = 0.125; break;
            case 1: pw = 0.25;  break;
            case 2: pw = 0.5;   break;
            case 3: pw = 1;     break;
            case 4: pw = 2;     break;
        }
        lfoL.set_params(freq, *params[param_mode], *params[param_offset_l], srate, *params[param_amount], pw);
        lfoR.set_params(freq, *params[param_mode], *params[param_offset_r], srate, *params[param_amount], pw);
        mode_old     = *params[param_mode];
        amount_old   = *params[param_amount];
        offset_l_old = *params[param_offset_l];
        offset_r_old = *params[param_offset_r];
        pwidth_old   = *params[param_pwidth];
        redraw_graph = true;
    }
}

void pulsator_audio_module::set_sample_rate(uint32_t sr)
{
    srate = sr;
    int meter[] = {param_meter_inL, param_meter_inR,  param_meter_outL, param_meter_outR};
    int clip[] = {param_clip_inL, param_clip_inR, param_clip_outL, param_clip_outR};
    meters.init(params, meter, clip, 4, sr);
}

uint32_t pulsator_audio_module::process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask)
{
    bool bypassed = bypass.update(*params[param_bypass] > 0.5f, numsamples);
    uint32_t samples = numsamples + offset;
    if(bypassed) {
        // everything bypassed
        while(offset < samples) {
            outs[0][offset] = ins[0][offset];
            outs[1][offset] = ins[1][offset];
            ++offset;
        }
        // LFO's should go on
        lfoL.advance(numsamples);
        lfoR.advance(numsamples);
        
        float values[] = {0, 0, 0, 0};
        meters.process(values);
    } else {
        // process
        uint32_t orig_offset = offset;
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
            
            // next sample
            ++offset;
            
            // advance lfo's
            lfoL.advance(1);
            lfoR.advance(1);
            
            float values[] = {inL, inR, outL, outR};
            meters.process(values);

        } // cycle trough samples
        bypass.crossfade(ins, outs, 2, orig_offset, numsamples);
    }
    meters.fall(numsamples);
    return outputs_mask;
}

bool pulsator_audio_module::get_graph(int index, int subindex, int phase, float *data, int points, cairo_iface *context, int *mode) const
{
    if (!is_active || phase || subindex > 1) {
        redraw_graph = false;
        return false;
    }
    set_channel_color(context, subindex);
    return (subindex ? lfoR : lfoL).get_graph(data, points, context, mode);
}

bool pulsator_audio_module::get_dot(int index, int subindex, int phase, float &x, float &y, int &size, cairo_iface *context) const
{
    if (!is_active || !phase || subindex > 1)
        return false;
    set_channel_color(context, subindex);
    return (subindex ? lfoR : lfoL).get_dot(x, y, size, context);
}

bool pulsator_audio_module::get_gridline(int index, int subindex, int phase, float &pos, bool &vertical, std::string &legend, cairo_iface *context) const
{
    if (phase || subindex)
        return false;
    pos = 0;
    vertical = false;
    return true;
}
bool pulsator_audio_module::get_layers(int index, int generation, unsigned int &layers) const
{
    layers = LG_REALTIME_DOT | (generation ? 0 : LG_CACHE_GRID) | ((redraw_graph || !generation) ? LG_CACHE_GRAPH : 0);
    return true;
}


/**********************************************************************
 * RING MODULATOR by Markus Schmidt
**********************************************************************/

ringmodulator_audio_module::ringmodulator_audio_module()
{
    is_active = false;
    srate = 0;
}

void ringmodulator_audio_module::activate()
{
    is_active = true;
    lfo1.activate();
    lfo2.activate();
    modL.activate();
    modR.activate();
    params_changed();
}
void ringmodulator_audio_module::deactivate()
{
    is_active = false;
    lfo1.deactivate();
    lfo2.deactivate();
    modL.deactivate();
    modR.deactivate();
}

void ringmodulator_audio_module::params_changed()
{
    lfo1.set_params(*params[param_lfo1_freq], *params[param_lfo1_mode], 0, srate, 1);
    lfo2.set_params(*params[param_lfo2_freq], *params[param_lfo2_mode], 0, srate, 1);
    modL.set_params(*params[param_mod_freq] * pow(pow(2, 1.0 / 1200.0), *params[param_mod_detune] / 2),
                    *params[param_mod_mode], 0, srate, 1);
    modR.set_params(*params[param_mod_freq] * pow(pow(2, 1.0 / 1200.0), *params[param_mod_detune] / -2),
                    *params[param_mod_mode], *params[param_mod_phase], srate, 1);
    
    clear_reset = false;
    
    if (*params[param_lfo1_reset] >= 0.5) {
        clear_reset = true;
        lfo1.set_phase(0.f);
    }
    if (*params[param_lfo2_reset] >= 0.5) {
        clear_reset = true;
        lfo2.set_phase(0.f);
    }
    
    //redraw_graph = true;
}

void ringmodulator_audio_module::set_sample_rate(uint32_t sr)
{
    srate = sr;
    int meter[] = {param_meter_inL, param_meter_inR,  param_meter_outL, param_meter_outR};
    int clip[] = {param_clip_inL, param_clip_inR, param_clip_outL, param_clip_outR};
    meters.init(params, meter, clip, 4, sr);
}

uint32_t ringmodulator_audio_module::process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask)
{
    bool bypassed = bypass.update(*params[param_bypass] > 0.5f, numsamples);
    uint32_t samples = numsamples + offset;
    float led1 = 0;
    float led2 = 0;
    if(bypassed) {
        // everything bypassed
        while(offset < samples) {
            outs[0][offset] = ins[0][offset];
            outs[1][offset] = ins[1][offset];
            ++offset;
        }
        // LFO's should go on
        lfo1.advance(numsamples);
        lfo1.advance(numsamples);
        modL.advance(numsamples);
        modR.advance(numsamples);
        
        float values[] = {0, 0, 0, 0};
        meters.process(values);
    } else {
        // process
        uint32_t orig_offset = offset;
        while(offset < samples) {
            // cycle through samples
            
            // set oscillators
            // mod frequency
            float freq = 0;
            if (*params[param_lfo1_mod_freq_active] > 0.5) {
                freq = (*params[param_lfo1_mod_freq_hi]
                      - *params[param_lfo1_mod_freq_lo])
                        * ((lfo1.get_value() + 1) / 2.)
                      + *params[param_lfo1_mod_freq_lo];
                modL.set_freq(freq);
                modR.set_freq(freq);
            }
            // mod detune
            if (*params[param_lfo1_mod_detune_active] > 0.5) {
                float detune = (*params[param_lfo1_mod_detune_hi]
                              - *params[param_lfo1_mod_detune_lo])
                              * ((lfo1.get_value() + 1) / 2.)
                              + *params[param_lfo1_mod_detune_lo];
                modL.set_freq((freq ? freq : *params[param_mod_freq]) * pow(pow(2, 1.0 / 1200.0), detune / 2));
                modR.set_freq((freq ? freq : *params[param_mod_freq]) * pow(pow(2, 1.0 / 1200.0), detune / -2));
            }
            // lfo1 frequency
            if (*params[param_lfo2_lfo1_freq_active] > 0.5) {
                lfo1.set_freq((*params[param_lfo2_lfo1_freq_hi]
                             - *params[param_lfo2_lfo1_freq_lo])
                             * ((lfo2.get_value() + 1) / 2.)
                             + *params[param_lfo2_lfo1_freq_lo]);
            }
            // mod amount
            float mod_amount = *params[param_mod_amount];
            if (*params[param_lfo2_mod_amount_active] > 0.5) {
                mod_amount = (*params[param_lfo2_mod_amount_hi]
                            - *params[param_lfo2_mod_amount_lo])
                            * ((lfo2.get_value() + 1) / 2.)
                            + *params[param_lfo2_mod_amount_lo];
            }
            
            float outL = 0.f;
            float outR = 0.f;
            float inL = ins[0][offset];
            float inR = ins[1][offset];
            
            // in level
            inR *= *params[param_level_in];
            inL *= *params[param_level_in];
            
            // modulator
            float modulL = modL.get_value() * mod_amount;
            float modulR = modR.get_value() * mod_amount;
            
            float procL = inL * modulL;
            float procR = inR * modulR;
            
            outL = *params[param_mod_listen] > 0.5 ? modulL : procL + inL * (1 - mod_amount);
            outR = *params[param_mod_listen] > 0.5 ? modulR : procR + inR * (1 - mod_amount);
            
            outL *=  *params[param_level_out];
            outR *=  *params[param_level_out];
            
            // send to output
            outs[0][offset] = outL;
            outs[1][offset] = outR;
            
            // next sample
            ++offset;
            
            // leds
            led1 = std::max(led1, lfo1.get_value() / 2 + 0.5f);
            led2 = std::max(led2, lfo2.get_value() / 2 + 0.5f);
            
            // advance lfo's
            lfo1.advance(1);
            lfo2.advance(1);
            modL.advance(1);
            modR.advance(1);
            
            float values[] = {inL, inR, outL, outR};
            meters.process(values);

        } // cycle trough samples
        bypass.crossfade(ins, outs, 2, orig_offset, numsamples);
    }
    *params[param_lfo1_activity] = led1;
    *params[param_lfo2_activity] = led2;
    meters.fall(numsamples);
    return outputs_mask;
}

bool ringmodulator_audio_module::get_graph(int index, int subindex, int phase, float *data, int points, cairo_iface *context, int *mode) const
{
    if (!is_active || phase || subindex > 1) {
        redraw_graph = false;
        return false;
    }
    set_channel_color(context, subindex);
    return (subindex ? lfo2 : lfo1).get_graph(data, points, context, mode);
}

bool ringmodulator_audio_module::get_dot(int index, int subindex, int phase, float &x, float &y, int &size, cairo_iface *context) const
{
    if (!is_active || !phase || subindex > 1)
        return false;
    set_channel_color(context, subindex);
    return (subindex ? lfo2 : lfo1).get_dot(x, y, size, context);
}

bool ringmodulator_audio_module::get_gridline(int index, int subindex, int phase, float &pos, bool &vertical, std::string &legend, cairo_iface *context) const
{
    if (phase || subindex)
        return false;
    pos = 0;
    vertical = false;
    return true;
}
bool ringmodulator_audio_module::get_layers(int index, int generation, unsigned int &layers) const
{
    layers = LG_REALTIME_DOT | (generation ? 0 : LG_CACHE_GRID) | ((redraw_graph || !generation) ? LG_CACHE_GRAPH : 0);
    return true;
}
