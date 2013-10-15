/* Calf DSP plugin pack
 * Assorted plugins
 *
 * Copyright (C) 2001-2010 Krzysztof Foltman, Markus Schmidt, Thor Harald Johansen
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
#include <math.h>
#include <fftw3.h>
#include <calf/giface.h>
#include <calf/modules.h>
#include <calf/modules_dev.h>
#include <sys/time.h>

using namespace dsp;
using namespace calf_plugins;

#define SET_IF_CONNECTED(name) if (params[AM::param_##name] != NULL) *params[AM::param_##name] = name;
#define sinc(x) (!x) ? 1 : sin(M_PI * x)/(M_PI * x);

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
    _tap_avg = 0;
    _tap_last = 0;
}

void vintage_delay_audio_module::params_changed()
{
    if(*params[par_tap] >= .5f) {
        timeval tv;
        gettimeofday(&tv, 0);
        long _now = tv.tv_sec * 1000000 + tv.tv_usec;
        if(_tap_last) {
            if(_tap_avg)
                _tap_avg = (_tap_avg * 3 + (_now - _tap_last)) / 4.f;
            else
                _tap_avg = _now - _tap_last;
            *params[par_bpm] = 60.f / (float)(_tap_avg / 1000000.f);
            printf("bpm: %.5f\n", *params[par_bpm]);
        }
        _tap_last = _now;
        *params[par_tap] = 0.f;
    }
    float unit = 60.0 * srate / (*params[par_bpm] * *params[par_divide]);
    deltime_l = dsp::fastf2i_drm(unit * *params[par_time_l]);
    deltime_r = dsp::fastf2i_drm(unit * *params[par_time_r]);
    int deltime_fb = deltime_l + deltime_r;
    float fb = *params[par_feedback];
    dry.set_inertia(*params[par_dryamount]);
    mixmode = dsp::fastf2i_drm(*params[par_mixmode]);
    medium = dsp::fastf2i_drm(*params[par_medium]);
    switch(mixmode)
    {
    case MIXMODE_STEREO:
        fb_left.set_inertia(fb);
        fb_right.set_inertia(pow(fb, *params[par_time_r] / *params[par_time_l]));
        amt_left.set_inertia(*params[par_amount]);
        amt_right.set_inertia(*params[par_amount]);
        break;
    case MIXMODE_PINGPONG:
        fb_left.set_inertia(fb);
        fb_right.set_inertia(fb);
        amt_left.set_inertia(*params[par_amount]);
        amt_right.set_inertia(*params[par_amount]);
        break;
    case MIXMODE_LR:
        fb_left.set_inertia(fb);
        fb_right.set_inertia(fb);
        amt_left.set_inertia(*params[par_amount]);                                          // L is straight 'amount'
        amt_right.set_inertia(*params[par_amount] * pow(fb, 1.0 * deltime_r / deltime_fb)); // R is amount with feedback based dampening as if it ran through R/FB*100% of delay line's dampening
        // deltime_l <<< deltime_r -> pow() = fb -> full delay line worth of dampening
        // deltime_l >>> deltime_r -> pow() = 1 -> no dampening
        break;
    case MIXMODE_RL:
        fb_left.set_inertia(fb);
        fb_right.set_inertia(fb);
        amt_left.set_inertia(*params[par_amount] * pow(fb, 1.0 * deltime_l / deltime_fb));
        amt_right.set_inertia(*params[par_amount]);
        break;
    }
    chmix.set_inertia((1 - *params[par_width]) * 0.5);
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

/// Single delay line with feedback at the same tap
static inline void delayline_impl(int age, int deltime, float dry_value, const float &delayed_value, float &out, float &del, gain_smoothing &amt, gain_smoothing &fb)
{
    // if the buffer hasn't been cleared yet (after activation), pretend we've read zeros
    if (age <= deltime) {
        out = 0;
        del = dry_value;
        amt.step();
        fb.step();
    }
    else
    {
        float delayed = delayed_value; // avoid dereferencing the pointer in 'then' branch of the if()
        dsp::sanitize(delayed);
        out = delayed * amt.get();
        del = dry_value + delayed * fb.get();
    }
}

/// Single delay line with tap output
static inline void delayline2_impl(int age, int deltime, float dry_value, const float &delayed_value, const float &delayed_value_for_fb, float &out, float &del, gain_smoothing &amt, gain_smoothing &fb)
{
    if (age <= deltime) {
        out = 0;
        del = dry_value;
        amt.step();
        fb.step();
    }
    else
    {
        out = delayed_value * amt.get();
        del = dry_value + delayed_value_for_fb * fb.get();
        dsp::sanitize(out);
        dsp::sanitize(del);
    }
}

static inline void delay_mix(float dry_left, float dry_right, float &out_left, float &out_right, float dry, float chmix)
{
    float tmp_left = lerp(out_left, out_right, chmix);
    float tmp_right = lerp(out_right, out_left, chmix);
    out_left = dry_left * dry + tmp_left;
    out_right = dry_right * dry + tmp_right;
}

uint32_t vintage_delay_audio_module::process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask)
{
    uint32_t ostate = 3; // XXXKF optimize!
    uint32_t end = offset + numsamples;
    int orig_bufptr = bufptr;
    float out_left, out_right, del_left, del_right;
    
    switch(mixmode)
    {
        case MIXMODE_STEREO:
        case MIXMODE_PINGPONG:
        {
            int v = mixmode == MIXMODE_PINGPONG ? 1 : 0;
            for(uint32_t i = offset; i < end; i++)
            {                
                delayline_impl(age, deltime_l, ins[0][i], buffers[v][(bufptr - deltime_l) & ADDR_MASK], out_left, del_left, amt_left, fb_left);
                delayline_impl(age, deltime_r, ins[1][i], buffers[1 - v][(bufptr - deltime_r) & ADDR_MASK], out_right, del_right, amt_right, fb_right);
                delay_mix(ins[0][i], ins[1][i], out_left, out_right, dry.get(), chmix.get());
                
                age++;
                outs[0][i] = out_left; outs[1][i] = out_right; buffers[0][bufptr] = del_left; buffers[1][bufptr] = del_right;
                bufptr = (bufptr + 1) & (MAX_DELAY - 1);
            }
        }
        break;
        
        case MIXMODE_LR:
        case MIXMODE_RL:
        {
            int v = mixmode == MIXMODE_RL ? 1 : 0;
            int deltime_fb = deltime_l + deltime_r;
            int deltime_l_corr = mixmode == MIXMODE_RL ? deltime_fb : deltime_l;
            int deltime_r_corr = mixmode == MIXMODE_LR ? deltime_fb : deltime_r;
            
            for(uint32_t i = offset; i < end; i++)
            {
                delayline2_impl(age, deltime_l, ins[0][i], buffers[v][(bufptr - deltime_l_corr) & ADDR_MASK], buffers[v][(bufptr - deltime_fb) & ADDR_MASK], out_left, del_left, amt_left, fb_left);
                delayline2_impl(age, deltime_r, ins[1][i], buffers[1 - v][(bufptr - deltime_r_corr) & ADDR_MASK], buffers[1-v][(bufptr - deltime_fb) & ADDR_MASK], out_right, del_right, amt_right, fb_right);
                delay_mix(ins[0][i], ins[1][i], out_left, out_right, dry.get(), chmix.get());
                
                age++;
                outs[0][i] = out_left; outs[1][i] = out_right; buffers[0][bufptr] = del_left; buffers[1][bufptr] = del_right;
                bufptr = (bufptr + 1) & (MAX_DELAY - 1);
            }
        }
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
    
    // get microseconds
    if(_tap_last) {
        timeval tv;
        gettimeofday(&tv, 0);
        long _now = tv.tv_sec * 1000 * 1000 + tv.tv_usec;
        if(_now > _tap_last + 2000000) {
            // user stopped tapping
            _tap_avg = 0;
            _tap_last = 0;
        }
        *params[par_waiting] = 1.f;
    } else {
        *params[par_waiting] = 0.f;
    }
    
    return ostate;
}

///////////////////////////////////////////////////////////////////////////////////////////////

comp_delay_audio_module::comp_delay_audio_module()
{
    buffer      = NULL;
    buf_size    = 0;
    delay       = 0;
    write_ptr   = 0;
}

comp_delay_audio_module::~comp_delay_audio_module()
{
    if (buffer != NULL)
        delete [] buffer;
}

void comp_delay_audio_module::params_changed()
{
    delay = (uint32_t)
        (
            (
                (*params[par_distance_m] * 100.0) +
                (*params[par_distance_cm] * 1.0) +
                (*params[par_distance_mm] * 0.1)
            ) * COMP_DELAY_SOUND_FRONT_DELAY(std::max(50, (int) *params[param_temp])) * srate
        );
}

void comp_delay_audio_module::activate()
{
    write_ptr   = 0;
}

void comp_delay_audio_module::deactivate()
{
}

void comp_delay_audio_module::set_sample_rate(uint32_t sr)
{
    srate = sr;
    float *old_buf = buffer;

    uint32_t min_buf_size = (uint32_t)(srate * COMP_DELAY_MAX_DELAY);
    uint32_t new_buf_size = 1;
    while (new_buf_size < min_buf_size)
        new_buf_size <<= 1;

    float *new_buf = new float[new_buf_size];
    for (size_t i=0; i<new_buf_size; i++)
        new_buf[i] = 0.0f;

    // Assign new pointer and size
    buffer         = new_buf;
    buf_size       = new_buf_size;

    // Delete old buffer
    if (old_buf != NULL)
        delete [] old_buf;
}

uint32_t comp_delay_audio_module::process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask)
{
    if (*params[param_bypass] > 0.5f) {
        while(offset < numsamples) {
            outs[0][offset] = ins[0][offset];
            ++offset;
        }
    } else {
        uint32_t b_mask = buf_size-1;
        uint32_t end    = offset + numsamples;
        uint32_t w_ptr  = write_ptr;
        uint32_t r_ptr  = (write_ptr + buf_size - delay) & b_mask; // Unsigned math, that's why we add buf_size
        float dry       = *params[par_dry];
        float wet       = *params[par_wet];
    
        for (uint32_t i=offset; i<end; i++)
        {
            float sample = ins[0][i];
            buffer[w_ptr] = sample;
    
            outs[0][i] = dry * sample + wet * buffer[r_ptr];
    
            w_ptr = (w_ptr + 1) & b_mask;
            r_ptr = (r_ptr + 1) & b_mask;
        }
        write_ptr = w_ptr;
    }
    return outputs_mask;
}

///////////////////////////////////////////////////////////////////////////////////////////////

bool filter_audio_module::get_graph(int index, int subindex, float *data, int points, cairo_iface *context, int *mode) const
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


void filterclavier_audio_module::note_on(int channel, int note, int vel)
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

void filterclavier_audio_module::note_off(int channel, int note, int vel)
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

bool filterclavier_audio_module::get_graph(int index, int subindex, float *data, int points, cairo_iface *context, int *mode) const
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

stereo_audio_module::stereo_audio_module() {
    active = false;
    clip_inL    = 0.f;
    clip_inR    = 0.f;
    clip_outL   = 0.f;
    clip_outR   = 0.f;
    meter_inL  = 0.f;
    meter_inR  = 0.f;
    meter_outL = 0.f;
    meter_outR = 0.f;
    _phase = -1;
}

void stereo_audio_module::activate() {
    active = true;
}

void stereo_audio_module::deactivate() {
    active = false;
}

void stereo_audio_module::params_changed() {
    float slev = 2 * *params[param_slev]; // stereo level ( -2 -> 2 )
    float sbal = 1 + *params[param_sbal]; // stereo balance ( 0 -> 2 )
    float mlev = 2 * *params[param_mlev]; // mono level ( -2 -> 2 )
    float mpan = 1 + *params[param_mpan]; // mono pan ( 0 -> 2 )

    switch((int)*params[param_mode])
    {
        case 0:
        default:
            //LR->LR
            LL = (mlev * (2.f - mpan) + slev * (2.f - sbal));
            LR = (mlev * mpan - slev * sbal);
            RL = (mlev * (2.f - mpan) - slev * (2.f - sbal));
            RR = (mlev * mpan + slev * sbal);
            break;
        case 1:
            //LR->MS
            LL = (2.f - mpan) * (2.f - sbal);
            LR = mpan * (2.f - sbal) * -1;
            RL = (2.f - mpan) * sbal;
            RR = mpan * sbal;
            break;
        case 2:
            //MS->LR
            LL = mlev * (2.f - sbal);
            LR = mlev * mpan;
            RL = slev * (2.f - sbal);
            RR = slev * sbal * -1;
            break;
        case 3:
        case 4:
        case 5:
        case 6:
            //LR->LL
            LL = 0.f;
            LR = 0.f;
            RL = 0.f;
            RR = 0.f;
            break;
    }
    if(*params[param_stereo_phase] != _phase) {
        _phase = *params[param_stereo_phase];
        _phase_cos_coef = cos(_phase / 180 * M_PI);
        _phase_sin_coef = sin(_phase / 180 * M_PI);
    }
    if(*params[param_sc_level] != _sc_level) {
        _sc_level = *params[param_sc_level];
        _inv_atan_shape = 1.0 / atan(_sc_level);
    }
}

uint32_t stereo_audio_module::process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask) {
    for(uint32_t i = offset; i < offset + numsamples; i++) {
        if(*params[param_bypass] > 0.5) {
            outs[0][i] = ins[0][i];
            outs[1][i] = ins[1][i];
            clip_inL    = 0.f;
            clip_inR    = 0.f;
            clip_outL   = 0.f;
            clip_outR   = 0.f;
            meter_inL  = 0.f;
            meter_inR  = 0.f;
            meter_outL = 0.f;
            meter_outR = 0.f;
        } else {
            // let meters fall a bit
            clip_inL    -= std::min(clip_inL,  numsamples);
            clip_inR    -= std::min(clip_inR,  numsamples);
            clip_outL   -= std::min(clip_outL, numsamples);
            clip_outR   -= std::min(clip_outR, numsamples);
            meter_inL = 0.f;
            meter_inR = 0.f;
            meter_outL = 0.f;
            meter_outR = 0.f;
            
            float L = ins[0][i];
            float R = ins[1][i];
            
            // levels in
            L *= *params[param_level_in];
            R *= *params[param_level_in];
            
            // balance in
            L *= (1.f - std::max(0.f, *params[param_balance_in]));
            R *= (1.f + std::min(0.f, *params[param_balance_in]));
            
            // copy / flip / mono ...
            switch((int)*params[param_mode])
            {
                case 0:
                default:
                    // LR > LR
                    break;
                case 1:
                    // LR > MS
                    break;
                case 2:
                    // MS > LR
                    break;
                case 3:
                    // LR > LL
                    R = L;
                    break;
                case 4:
                    // LR > RR
                    L = R;
                    break;
                case 5:
                    // LR > L+R
                    L = (L + R) / 2;
                    R = L;
                    break;
                case 6:
                    // LR > RL
                    float tmp = L;
                    L = R;
                    R = tmp;
                    break;
            }
            
            // softclip
            if(*params[param_softclip]) {
//                int ph;
//                ph = L / fabs(L);
//                L = L > 0.63 ? ph * (0.63 + 0.36 * (1 - pow(MATH_E, (1.f / 3) * (0.63 + L * ph)))) : L;
//                ph = R / fabs(R);
//                R = R > 0.63 ? ph * (0.63 + 0.36 * (1 - pow(MATH_E, (1.f / 3) * (0.63 + R * ph)))) : R;
                R = _inv_atan_shape * atan(R * _sc_level);
                L = _inv_atan_shape * atan(L * _sc_level);
            }
            
            // GUI stuff
            if(L > meter_inL) meter_inL = L;
            if(R > meter_inR) meter_inR = R;
            if(L > 1.f) clip_inL  = srate >> 3;
            if(R > 1.f) clip_inR  = srate >> 3;
            
            // mute
            L *= (1 - floor(*params[param_mute_l] + 0.5));
            R *= (1 - floor(*params[param_mute_r] + 0.5));
            
            // phase
            L *= (2 * (1 - floor(*params[param_phase_l] + 0.5))) - 1;
            R *= (2 * (1 - floor(*params[param_phase_r] + 0.5))) - 1;
            
            // LR/MS
            L += LL*L + RL*R;
            R += RR*R + LR*L;
            
            // delay
            buffer[pos]     = L;
            buffer[pos + 1] = R;
            
            int nbuf = srate * (fabs(*params[param_delay]) / 1000.f);
            nbuf -= nbuf % 2;
            if(*params[param_delay] > 0.f) {
                R = buffer[(pos - (int)nbuf + 1 + buffer_size) % buffer_size];
            } else if (*params[param_delay] < 0.f) {
                L = buffer[(pos - (int)nbuf + buffer_size)     % buffer_size];
            }
            
            // stereo base
            float _sb = *params[param_stereo_base];
            if(_sb < 0) _sb *= 0.5;
            
            float __l = L +_sb * L - _sb * R;
            float __r = R + _sb * R - _sb * L;
            
            L = __l;
            R = __r;
            
            // stereo phase
            __l = L * _phase_cos_coef - R * _phase_sin_coef;
            __r = L * _phase_sin_coef + R * _phase_cos_coef;
            
            L = __l;
            R = __r;
            
            pos = (pos + 2) % buffer_size;
            
            // balance out
            L *= (1.f - std::max(0.f, *params[param_balance_out]));
            R *= (1.f + std::min(0.f, *params[param_balance_out]));
            
            // level 
            L *= *params[param_level_out];
            R *= *params[param_level_out];
            
            //output
            outs[0][i] = L;
            outs[1][i] = R;
            
            // clip LED's
            if(L > 1.f) clip_outL = srate >> 3;
            if(R > 1.f) clip_outR = srate >> 3;
            if(L > meter_outL) meter_outL = L;
            if(R > meter_outR) meter_outR = R;
            
            // phase meter
            if(fabs(L) > 0.001 and fabs(R) > 0.001) {
                meter_phase = fabs(fabs(L+R) > 0.000000001 ? sin(fabs((L-R)/(L+R))) : 0.f);
            } else {
                meter_phase = 0.f;
            }
        }
    }
    // draw meters
    SET_IF_CONNECTED(clip_inL);
    SET_IF_CONNECTED(clip_inR);
    SET_IF_CONNECTED(clip_outL);
    SET_IF_CONNECTED(clip_outR);
    SET_IF_CONNECTED(meter_inL);
    SET_IF_CONNECTED(meter_inR);
    SET_IF_CONNECTED(meter_outL);
    SET_IF_CONNECTED(meter_outR);
    SET_IF_CONNECTED(meter_phase);
    return outputs_mask;
}

void stereo_audio_module::set_sample_rate(uint32_t sr)
{
    srate = sr;
    // rebuild buffer
    buffer_size = (int)(srate * 0.05 * 2.f); // buffer size attack rate multiplied by 2 channels
    buffer = (float*) calloc(buffer_size, sizeof(float));
    dsp::zero(buffer, buffer_size); // reset buffer to zero
    pos = 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////

mono_audio_module::mono_audio_module() {
    active = false;
    clip_in    = 0.f;
    clip_outL   = 0.f;
    clip_outR   = 0.f;
    meter_in  = 0.f;
    meter_outL = 0.f;
    meter_outR = 0.f;
    _phase = -1.f;
}

void mono_audio_module::activate() {
    active = true;
}

void mono_audio_module::deactivate() {
    active = false;
}

void mono_audio_module::params_changed() {
    if(*params[param_sc_level] != _sc_level) {
        _sc_level = *params[param_sc_level];
        _inv_atan_shape = 1.0 / atan(_sc_level);
    }
    if(*params[param_stereo_phase] != _phase) {
        _phase = *params[param_stereo_phase];
        _phase_cos_coef = cos(_phase / 180 * M_PI);
        _phase_sin_coef = sin(_phase / 180 * M_PI);
    }
}

uint32_t mono_audio_module::process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask) {
    for(uint32_t i = offset; i < offset + numsamples; i++) {
        if(*params[param_bypass] > 0.5) {
            outs[0][i] = ins[0][i];
            outs[1][i] = ins[0][i];
            clip_in     = 0.f;
            clip_outL   = 0.f;
            clip_outR   = 0.f;
            meter_in    = 0.f;
            meter_outL  = 0.f;
            meter_outR  = 0.f;
        } else {
            // let meters fall a bit
            clip_in     -= std::min(clip_in,  numsamples);
            clip_outL   -= std::min(clip_outL, numsamples);
            clip_outR   -= std::min(clip_outR, numsamples);
            meter_in     = 0.f;
            meter_outL   = 0.f;
            meter_outR   = 0.f;
            
            float L = ins[0][i];
            
            // levels in
            L *= *params[param_level_in];
            
            // softclip
            if(*params[param_softclip]) {
                //int ph = L / fabs(L);
                //L = L > 0.63 ? ph * (0.63 + 0.36 * (1 - pow(MATH_E, (1.f / 3) * (0.63 + L * ph)))) : L;
                L = _inv_atan_shape * atan(L * _sc_level);
            }
            
            // GUI stuff
            if(L > meter_in) meter_in = L;
            if(L > 1.f) clip_in = srate >> 3;
            
            float R = L;
            
            // mute
            L *= (1 - floor(*params[param_mute_l] + 0.5));
            R *= (1 - floor(*params[param_mute_r] + 0.5));
            
            // phase
            L *= (2 * (1 - floor(*params[param_phase_l] + 0.5))) - 1;
            R *= (2 * (1 - floor(*params[param_phase_r] + 0.5))) - 1;
            
            // delay
            buffer[pos]     = L;
            buffer[pos + 1] = R;
            
            int nbuf = srate * (fabs(*params[param_delay]) / 1000.f);
            nbuf -= nbuf % 2;
            if(*params[param_delay] > 0.f) {
                R = buffer[(pos - (int)nbuf + 1 + buffer_size) % buffer_size];
            } else if (*params[param_delay] < 0.f) {
                L = buffer[(pos - (int)nbuf + buffer_size)     % buffer_size];
            }
            
            // stereo base
            float _sb = *params[param_stereo_base];
            if(_sb < 0) _sb *= 0.5;
            
            float __l = L +_sb * L - _sb * R;
            float __r = R + _sb * R - _sb * L;
            
            L = __l;
            R = __r;
            
            // stereo phase
            __l = L * _phase_cos_coef - R * _phase_sin_coef;
            __r = L * _phase_sin_coef + R * _phase_cos_coef;
            
            L = __l;
            R = __r;
            
            pos = (pos + 2) % buffer_size;
            
            // balance out
            L *= (1.f - std::max(0.f, *params[param_balance_out]));
            R *= (1.f + std::min(0.f, *params[param_balance_out]));
            
            // level 
            L *= *params[param_level_out];
            R *= *params[param_level_out];
            
            //output
            outs[0][i] = L;
            outs[1][i] = R;
            
            // clip LED's
            if(L > 1.f) clip_outL = srate >> 3;
            if(R > 1.f) clip_outR = srate >> 3;
            if(L > meter_outL) meter_outL = L;
            if(R > meter_outR) meter_outR = R;
        }
    }
    // draw meters
    SET_IF_CONNECTED(clip_in);
    SET_IF_CONNECTED(clip_outL);
    SET_IF_CONNECTED(clip_outR);
    SET_IF_CONNECTED(meter_in);
    SET_IF_CONNECTED(meter_outL);
    SET_IF_CONNECTED(meter_outR);
    return outputs_mask;
}

void mono_audio_module::set_sample_rate(uint32_t sr)
{
    srate = sr;
    // rebuild buffer
    buffer_size = (int)srate * 0.05 * 2; // delay buffer size multiplied by 2 channels
    buffer = (float*) calloc(buffer_size, sizeof(float));
    dsp::zero(buffer, buffer_size); // reset buffer to zero
    pos = 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////
analyzer_audio_module::analyzer_audio_module() {

    active = false;
    clip_L   = 0.f;
    clip_R   = 0.f;
    meter_L = 0.f;
    meter_R = 0.f;
    _accuracy = -1;
    _acc_old = -1;
    _scale_old = -1;
    _mode_old = -1;
    _post_old = -1;
    _hold_old = -1;
    _smooth_old = -1;
    ppos = 0;
    plength = 0;
    fpos = 0;
    
    spline_buffer = (int*) calloc(200, sizeof(int));
    memset(spline_buffer, 0, 200 * sizeof(int)); // reset buffer to zero
    
    phase_buffer = (float*) calloc(max_phase_buffer_size, sizeof(float));
    dsp::zero(phase_buffer, max_phase_buffer_size);
    
    fft_buffer = (float*) calloc(max_fft_buffer_size, sizeof(float));
    
    fft_inL = (float*) calloc(max_fft_cache_size, sizeof(float));
    fft_outL = (float*) calloc(max_fft_cache_size, sizeof(float));
    fft_inR = (float*) calloc(max_fft_cache_size, sizeof(float));
    fft_outR = (float*) calloc(max_fft_cache_size, sizeof(float));
    
    fft_smoothL = (float*) calloc(max_fft_cache_size, sizeof(float));
    fft_smoothR = (float*) calloc(max_fft_cache_size, sizeof(float));
    
    fft_fallingL = (float*) calloc(max_fft_cache_size, sizeof(float));
    fft_fallingR = (float*) calloc(max_fft_cache_size, sizeof(float));
    dsp::fill(fft_fallingL, 1.f, max_fft_cache_size);
    dsp::fill(fft_fallingR, 1.f, max_fft_cache_size);
    
    fft_deltaL = (float*) calloc(max_fft_cache_size, sizeof(float));
    fft_deltaR = (float*) calloc(max_fft_cache_size, sizeof(float));
    
    fft_holdL = (float*) calloc(max_fft_cache_size, sizeof(float));
    fft_holdR = (float*) calloc(max_fft_cache_size, sizeof(float));
    
    fft_freezeL = (float*) calloc(max_fft_cache_size, sizeof(float));
    fft_freezeR = (float*) calloc(max_fft_cache_size, sizeof(float));
    
    fft_plan = NULL;
    
    ____analyzer_phase_was_drawn_here = 0;
    ____analyzer_sanitize = 0;
}

analyzer_audio_module::~analyzer_audio_module()
{
    free(fft_freezeR);
    free(fft_freezeL);
    free(fft_holdR);
    free(fft_holdL);
    free(fft_deltaR);
    free(fft_deltaL);
    free(fft_fallingR);
    free(fft_fallingL);
    free(fft_smoothR);
    free(fft_smoothL);
    free(fft_outR);
    free(fft_outL);
    free(fft_inR);
    free(fft_inL);
    free(phase_buffer);
    free(spline_buffer);
    if (fft_plan)
    {
        fftwf_destroy_plan(fft_plan);
        fft_plan = NULL;
    }
}

void analyzer_audio_module::activate() {
    active = true;
}

void analyzer_audio_module::deactivate() {
    active = false;
}

void analyzer_audio_module::params_changed() {
    bool ___sanitize = false;
    if(*params[param_analyzer_accuracy] != _acc_old) {
        _accuracy = 1 << (7 + (int)*params[param_analyzer_accuracy]);
        _acc_old = *params[param_analyzer_accuracy];
        // recreate fftw plan
        if (fft_plan) fftwf_destroy_plan (fft_plan);
        //fft_plan = rfftw_create_plan(_accuracy, FFTW_FORWARD, 0);
        fft_plan = fftwf_plan_r2r_1d(_accuracy, NULL, NULL, FFTW_R2HC, FFTW_ESTIMATE);
        lintrans = -1;
        ___sanitize = true;
    }
    if(*params[param_analyzer_hold] != _hold_old) {
        _hold_old = *params[param_analyzer_hold];
        ___sanitize = true;
    }
    if(*params[param_analyzer_smoothing] != _smooth_old) {
        _smooth_old = *params[param_analyzer_smoothing];
        ___sanitize = true;
    }
    if(*params[param_analyzer_mode] != _mode_old) {
        _mode_old = *params[param_analyzer_mode];
        ___sanitize = true;
    }
    if(*params[param_analyzer_scale] != _scale_old) {
        _scale_old = *params[param_analyzer_scale];
        ___sanitize = true;
    }
    if(*params[param_analyzer_post] != _post_old) {
        _post_old = *params[param_analyzer_post];
        ___sanitize = true;
    }
    if(___sanitize) {
        // null the overall buffer
        dsp::zero(fft_inL, max_fft_cache_size);
        dsp::zero(fft_inR, max_fft_cache_size);
        dsp::zero(fft_outL, max_fft_cache_size);
        dsp::zero(fft_outR, max_fft_cache_size);
        dsp::zero(fft_holdL, max_fft_cache_size);
        dsp::zero(fft_holdR, max_fft_cache_size);
        dsp::zero(fft_smoothL, max_fft_cache_size);
        dsp::zero(fft_smoothR, max_fft_cache_size);
        dsp::zero(fft_deltaL, max_fft_cache_size);
        dsp::zero(fft_deltaR, max_fft_cache_size);
//        memset(fft_fallingL, 1.f, max_fft_cache_size * sizeof(float));
//        memset(fft_fallingR, 1.f, max_fft_cache_size * sizeof(float));
        dsp::zero(spline_buffer, 200);
        ____analyzer_phase_was_drawn_here = 0;
    }
}

uint32_t analyzer_audio_module::process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask) {
    for(uint32_t i = offset; i < offset + numsamples; i++) {
        // let meters fall a bit
        clip_L   -= std::min(clip_L, numsamples);
        clip_R   -= std::min(clip_R, numsamples);
        meter_L   = 0.f;
        meter_R   = 0.f;
        
        float L = ins[0][i];
        float R = ins[1][i];
        
        // GUI stuff
        if(L > 1.f) clip_L = srate >> 3;
        if(R > 1.f) clip_R = srate >> 3;
        
        // goniometer
        phase_buffer[ppos] = L * *params[param_gonio_level];
        phase_buffer[ppos + 1] = R * *params[param_gonio_level];
        
        plength = std::min(phase_buffer_size, plength + 2);
        ppos += 2;
        ppos %= (phase_buffer_size - 2);
        
        // analyzer
        fft_buffer[fpos] = L;
        fft_buffer[fpos + 1] = R;
        
        fpos += 2;
        fpos %= (max_fft_buffer_size - 2);
        
        // meter
        meter_L = L;
        meter_R = R;
        
        //output
        outs[0][i] = L;
        outs[1][i] = R;
    }
    // draw meters
    SET_IF_CONNECTED(clip_L);
    SET_IF_CONNECTED(clip_R);
    SET_IF_CONNECTED(meter_L);
    SET_IF_CONNECTED(meter_R);
    return outputs_mask;
}

void analyzer_audio_module::set_sample_rate(uint32_t sr)
{
    srate = sr;
    phase_buffer_size = srate / 30 * 2;
    phase_buffer_size -= phase_buffer_size % 2;
    phase_buffer_size = std::min(phase_buffer_size, (int)max_phase_buffer_size);
}

bool analyzer_audio_module::get_phase_graph(float ** _buffer, int *_length, int * _mode, bool * _use_fade, float * _fade, int * _accuracy, bool * _display) const {
    *_buffer = &phase_buffer[0];
    *_length = plength;
    *_use_fade = *params[param_gonio_use_fade];
    *_fade = *params[param_gonio_fade];
    *_mode = *params[param_gonio_mode];
    *_accuracy = *params[param_gonio_accuracy];
    *_display = *params[param_gonio_display];
    return false;
}

bool analyzer_audio_module::get_graph(int index, int subindex, float *data, int points, cairo_iface *context, int *mode) const
{
    if(____analyzer_sanitize) {
        // null the overall buffer to feed the fft with if someone requested so
        // in the last cycle
        // afterwards quit this call
        dsp::zero(fft_inL, max_fft_cache_size);
        dsp::zero(fft_inR, max_fft_cache_size);
        ____analyzer_sanitize = 0;
        return false;
    }
    
    // just for the stoopid
    int _param_mode = (int)*params[param_analyzer_mode];
    bool _param_hold = (bool)*params[param_analyzer_hold];
    
    if (!active \
        or !*params[param_analyzer_display] \
        
        or (subindex == 1 and !_param_hold and _param_mode < 3) \
        
        or (subindex > 1 and _param_mode < 3) \
        
        or (subindex == 2 and !_param_hold and (_param_mode == 3 \
            or _param_mode == 4)) \
        
        or (subindex == 4 and (_param_mode == 3 or _param_mode == 4)) \
            
        or (subindex == 1 and _param_mode == 5) \
        
        or (subindex == 1 and _param_mode > 5 and _param_mode < 9) \
    ) {
        // stop drawing when all curves have been drawn according to the mode
        // and hold settings
        return false;
    }
    bool fftdone = false; // if fft was renewed, this one is set to true
    double freq; // here the frequency of the actual drawn pixel gets stored
    float stereo_coeff = pow(2, 4 * *params[param_analyzer_level] - 2);
    int iter = 0; // this is the pixel we have been drawing the last box/bar/line
    int _iter = 1; // this is the next pixel we want to draw a box/bar/line
    float posneg = 1;
    int _param_speed = 16 - (int)*params[param_analyzer_speed];
    int _param_smooth = (int)*params[param_analyzer_smoothing];
    if(_param_mode == 5 and _param_smooth) {
        // there's no falling for difference mode, only smoothing
        _param_smooth = 2;
    }
    if(_param_mode > 5 and _param_mode < 9) {
        // there's no smoothing for spectralizer mode
        //_param_smooth = 0;
    }
    
    if(subindex == 0) {
        // #####################################################################
        // We are doing FFT here, so we first have to setup fft-buffers from
        // the main buffer and we use this cycle for filling other buffers
        // like smoothing, delta and hold
        // #####################################################################
        if(!((int)____analyzer_phase_was_drawn_here % _param_speed)) {
            // seems we have to do a fft, so let's read the latest data from the
            // buffer to send it to fft afterwards
            // we want to remember old fft_out values for smoothing as well
            // and we fill the hold buffer in this (extra) cycle
            for(int i = 0; i < _accuracy; i++) {
                // go to the right position back in time according to accuracy
                // settings and cycling in the main buffer
                int _fpos = (fpos - _accuracy * 2 \
                    + (i * 2)) % max_fft_buffer_size;
                if(_fpos < 0)
                    _fpos = max_fft_buffer_size + _fpos;
                float L = fft_buffer[_fpos];
                float R = fft_buffer[_fpos + 1];
                float win = 0.54 - 0.46 * cos(2 * M_PI * i / _accuracy);
                L *= win;
                R *= win;
                
                // #######################################
                // Do some windowing functions on the
                // buffer
                // #######################################
                int _m = 2;
                float _f = 1.f;
                float _a, a0, a1, a2, a3;
                switch((int)*params[param_analyzer_windowing]) {
                    case 0:
                    default:
                        // Linear
                        _f = 1.f;
                        break;
                    case 1:
                        // Hamming
                        _f = 0.54 + 0.46 * cos(2 * M_PI * (i - 2 / points));
                        break;
                    case 2:
                        // von Hann
                        _f = 0.5 * (1 + cos(2 * M_PI * (i - 2 / points)));
                        break;
                    case 3:
                        // Blackman
                        _a = 0.16;
                        a0 = 1.f - _a / 2.f;
                        a1 = 0.5;
                        a2 = _a / 2.f;
                        _f = a0 + a1 * cos((2.f * M_PI * i) / points - 1) + \
                            a2 * cos((4.f * M_PI * i) / points - 1);
                        break;
                    case 4:
                        // Blackman-Harris
                        a0 = 0.35875;
                        a1 = 0.48829;
                        a2 = 0.14128;
                        a3 = 0.01168;
                        _f = a0 - a1 * cos((2.f * M_PI * i) / points - 1) + \
                            a2 * cos((4.f * M_PI * i) / points - 1) - \
                            a3 * cos((6.f * M_PI * i) / points - 1);
                        break;
                    case 5:
                        // Blackman-Nuttall
                        a0 = 0.3653819;
                        a1 = 0.4891775;
                        a2 = 0.1365995;
                        a3 = 0.0106411;
                        _f = a0 - a1 * cos((2.f * M_PI * i) / points - 1) + \
                            a2 * cos((4.f * M_PI * i) / points - 1) - \
                            a3 * cos((6.f * M_PI * i) / points - 1);
                        break;
                    case 6:
                        // Sine
                        _f = sin((M_PI * i) / (points - 1));
                        break;
                    case 7:
                        // Lanczos
                        _f = sinc((2.f * i) / (points - 1) - 1);
                        break;
                    case 8:
                        // Gauß
                        _a = 2.718281828459045;
                        _f = pow(_a, -0.5f * pow((i - (points - 1) / 2) / (0.4 * (points - 1) / 2.f), 2));
                        break;
                    case 9:
                        // Bartlett
                        _f = (2.f / (points - 1)) * (((points - 1) / 2.f) - \
                            fabs(i - ((points - 1) / 2.f)));
                        break;
                    case 10:
                        // Triangular
                        _f = (2.f / points) * ((2.f / points) - fabs(i - ((points - 1) / 2.f)));
                        break;
                    case 11:
                        // Bartlett-Hann
                        a0 = 0.62;
                        a1 = 0.48;
                        a2 = 0.38;
                        _f = a0 - a1 * fabs((i / (points - 1)) - 0.5) - \
                            a2 * cos((2 * M_PI * i) / (points - 1));
                        break;
                }
                L *= _f;
                if(_param_mode > _m)
                    R *= _f;

                // perhaps we need to compute two FFT's, so store left and right
                // channel in case we need only one FFT, the left channel is
                // used as 'standard'"
                float valL;
                float valR;
                
                switch(_param_mode) {
                    case 0:
                    case 6:
                        // average (mode 0)
                        valL = (L + R) / 2;
                        valR = (L + R) / 2;
                        break;
                    case 1:
                    default:
                        // left channel (mode 1)
                        // or both channels (mode 3, 4, 5)
                        valL = L;
                        valR = R;
                        break;
                    case 2:
                    case 8:
                        // right channel (mode 2)
                        valL = R;
                        valR = L;
                        break;
                }
                // store values in analyzer buffer
                fft_inL[i] = valL;
                fft_inR[i] = valR;
                
                // fill smoothing & falling buffer
                if(_param_smooth == 2) {
                    fft_smoothL[i] = fft_outL[i];
                    fft_smoothR[i] = fft_outR[i];
                }
                if(_param_smooth == 1) {
                    if(fft_smoothL[i] < fabs(fft_outL[i])) {
                        fft_smoothL[i] = fabs(fft_outL[i]);
                        fft_deltaL[i] = 1.f;
                    }
                    if(fft_smoothR[i] < fabs(fft_outR[i])) {
                        fft_smoothR[i] = fabs(fft_outR[i]);
                        fft_deltaR[i] = 1.f;
                    }
                }
                
                // fill hold buffer with last out values
                // before fft is recalced
                if(fabs(fft_outL[i]) > fft_holdL[i])
                    fft_holdL[i] = fabs(fft_outL[i]);
                if(fabs(fft_outR[i]) > fft_holdR[i])
                    fft_holdR[i] = fabs(fft_outR[i]);
            }
            
            // run fft
            // this takes our latest buffer and returns an array with
            // non-normalized
            if (fft_plan)
                fftwf_execute_r2r(fft_plan, fft_inL, fft_outL);
            //run fft for for right channel too. it is needed for stereo image 
            //and stereo difference modes
            if(_param_mode >= 3 and fft_plan) {
                fftwf_execute_r2r(fft_plan, fft_inR, fft_outR);
            }
            // ...and set some values for later use
            ____analyzer_phase_was_drawn_here = 0;     
            fftdone = true;  
        }
        ____analyzer_phase_was_drawn_here ++;
    }
    if (lintrans < 0) {
        // accuracy was changed so we have to recalc linear transition
        int _lintrans = (int)((float)points * log((20.f + 2.f * \
            (float)srate / (float)_accuracy) / 20.f) / log(1000.f));  
        lintrans = (int)(_lintrans + points % _lintrans / \
            floor(points / _lintrans)) / 2; // / 4 was added to see finer bars but breaks low end
    }
    for (int i = 0; i <= points; i++)
    {
        // #####################################################################
        // Real business starts here. We will cycle through all pixels in
        // x-direction of the line-graph and decide what to show
        // #####################################################################
        // cycle through the points to draw
        // we need to know the exact frequency at this pixel
        freq = 20.f * pow (1000.f, (float)i / points);
        
        // we need to know the last drawn value over the time
        float lastoutL = 0.f; 
        float lastoutR = 0.f;
        
        // let's see how many pixels we may want to skip until the drawing
        // function has to draw a bar/box/line
        if(*params[param_analyzer_scale] or *params[param_analyzer_view] == 2) {
            // we have linear view enabled or we want to see tit... erm curves
            if((i % lintrans == 0 and points - i > lintrans) or i == points - 1) {
                _iter = std::max(1, (int)floor(freq * \
                    (float)_accuracy / (float)srate));
            }    
        } else {
            // we have logarithmic view enabled
            _iter = std::max(1, (int)floor(freq * (float)_accuracy / (float)srate));
        }
        if(_iter > iter) {
            // we are flipping one step further in drawing
            if(fftdone and i) {
                // ################################
                // Manipulate the fft_out values
                // according to the post processing
                // ################################
                int n = 0;
                float var1L = 0.f; // used later for denoising peaks
                float var1R = 0.f;
                float diff_fft;
                switch(_param_mode) {
                    default:
                        // all normal modes
                        posneg = 1;
                        // only if we don't see difference mode
                        switch((int)*params[param_analyzer_post]) {
                            case 0:
                                // Analyzer Normalized - nothing to do
                                break;
                            case 1:
                                // Analyzer Additive - cycle through skipped values and
                                // add them
                                // if fft was renewed, recalc the absolute values if
                                // frequencies are skipped
                                for(int j = iter + 1; j < _iter; j++) {
                                    fft_outL[_iter] += fabs(fft_outL[j]);
                                    fft_outR[_iter] += fabs(fft_outR[j]);
                                }
                                fft_outL[_iter] /= (_iter - iter);
                                fft_outR[_iter] /= (_iter - iter);
                                break;
                            case 2:
                                // Analyzer Additive - cycle through skipped values and
                                // add them
                                // if fft was renewed, recalc the absolute values if
                                // frequencies are skipped
                                for(int j = iter + 1; j < _iter; j++) {
                                    fft_outL[_iter] += fabs(fft_outL[j]);
                                    fft_outR[_iter] += fabs(fft_outR[j]);
                                }
                                break;
                            case 3:
                                // Analyzer Denoised Peaks - filter out unwanted noise
                                for(int k = 0; k < std::max(10 , std::min(400 ,\
                                    (int)(2.f*(float)((_iter - iter))))); k++) {
                                    //collect amplitudes in the environment of _iter to
                                    //be able to erase them from signal and leave just
                                    //the peaks
                                    if(_iter - k > 0) {
                                        var1L += fabs(fft_outL[_iter - k]);
                                        n++;
                                    }
                                    if(k != 0) var1L += fabs(fft_outL[_iter + k]);
                                    else if(i) var1L += fabs(lastoutL);
                                    else var1L += fabs(fft_outL[_iter]);
                                    if(_param_mode == 3 or _param_mode == 4) {
                                        if(_iter - k > 0) {
                                            var1R += fabs(fft_outR[_iter - k]);
                                            n++;
                                        }
                                        if(k != 0) var1R += fabs(fft_outR[_iter + k]);
                                        else if(i) var1R += fabs(lastoutR);
                                        else var1R += fabs(fft_outR[_iter]);
                                    }
                                    n++;
                                }
                                //do not forget fft_out[_iter] for the next time
                                lastoutL = fft_outL[_iter];
                                //pumping up actual signal an erase surrounding
                                // sounds
                                fft_outL[_iter] = 0.25f * std::max(n * 0.6f * \
                                    fabs(fft_outL[_iter]) - var1L , 1e-20);
                                if(_param_mode == 3 or _param_mode == 4) {
                                    // do the same with R channel if needed
                                    lastoutR = fft_outR[_iter];
                                    fft_outR[_iter] = 0.25f * std::max(n * \
                                        0.6f * fabs(fft_outR[_iter]) - var1R , 1e-20);
                                }
                                break;
                        }
                        break;
                    case 5:
                        // Stereo Difference - draw the difference between left
                        // and right channel if fft was renewed, recalc the
                        // absolute values in left and right if frequencies are
                        // skipped.
                        // this is additive mode - no other mode is available
                        for(int j = iter + 1; j < _iter; j++) {
                            fft_outL[_iter] += fabs(fft_outL[j]);
                            fft_outR[_iter] += fabs(fft_outR[j]);
                        }
                        //calculate difference between left an right channel                        
                        diff_fft = fabs(fft_outL[_iter]) - fabs(fft_outR[_iter]);
                        posneg = fabs(diff_fft) / diff_fft;
                        fft_outL[_iter] = diff_fft / _accuracy;
                        break;
                }
            }
            iter = _iter;
            // #######################################
            // Calculate transitions for falling and
            // smooting and fill delta buffers if fft
            // was done above
            // #######################################
            if(subindex == 0) {
                float _fdelta = 0.91;
                if(_param_mode > 5 and _param_mode < 9)
                    _fdelta = .99f;
                float _ffactor = 2000.f;
                if(_param_mode > 5 and _param_mode < 9)
                    _ffactor = 50.f;
                if(_param_smooth == 2) {
                    // smoothing
                    if(fftdone) {
                        // rebuild delta values after fft was done
                        if(_param_mode < 5 or _param_mode > 5) {
                            fft_deltaL[iter] = pow(fabs(fft_outL[iter]) / fabs(fft_smoothL[iter]), 1.f / _param_speed);
                        } else {
                            fft_deltaL[iter] = (posneg * fabs(fft_outL[iter]) - fft_smoothL[iter]) / _param_speed;
                        }
                    } else {
                        // change fft_smooth according to delta
                        if(_param_mode < 5 or _param_mode > 5) {
                            fft_smoothL[iter] *= fft_deltaL[iter];
                        } else {
                            fft_smoothL[iter] += fft_deltaL[iter];
                        }
                    }
                } else if(_param_smooth == 1) {
                    // falling
                    if(fftdone) {
                        // rebuild delta values after fft was done
                        //fft_deltaL[iter] = _fdelta;
                    }
                    // change fft_smooth according to delta
                    fft_smoothL[iter] *= fft_deltaL[iter];
                    
                    if(fft_deltaL[iter] > _fdelta) {
                        fft_deltaL[iter] *= 1.f - (16.f - _param_speed) / _ffactor;
                    }
                }
                
                if(_param_mode > 2 and _param_mode < 5) {
                    // we need right buffers, too for stereo image and
                    // stereo analyzer
                    if(_param_smooth == 2) {
                        // smoothing
                        if(fftdone) {
                            // rebuild delta values after fft was done
                            if(_param_mode < 5) {
                                fft_deltaR[iter] = pow(fabs(fft_outR[iter]) / fabs(fft_smoothR[iter]), 1.f / _param_speed);
                            } else {
                                fft_deltaR[iter] = (posneg * fabs(fft_outR[iter]) - fft_smoothR[iter]) / _param_speed;
                            }
                        } else {
                            // change fft_smooth according to delta
                            if(_param_mode < 5) {
                                fft_smoothR[iter] *= fft_deltaR[iter];
                            } else {
                                fft_smoothR[iter] += fft_deltaR[iter];
                            }
                        }
                    } else if(_param_smooth == 1) {
                        // falling
                        if(fftdone) {
                            // rebuild delta values after fft was done
                            //fft_deltaR[iter] = _fdelta;
                        }
                        // change fft_smooth according to delta
                        fft_smoothR[iter] *= fft_deltaR[iter];
                        if(fft_deltaR[iter] > _fdelta)
                            fft_deltaR[iter] *= 1.f - (16.f - _param_speed) / 2000.f;
                    }
                }
            }
            // #######################################
            // Choose the L and R value from the right
            // buffer according to view settings
            // #######################################
            float valL = 0.f;
            float valR = 0.f;
            if (*params[param_analyzer_freeze]) {
                // freeze enabled
                valL = fft_freezeL[iter];
                valR = fft_freezeR[iter];
            } else if ((subindex == 1 and _param_mode < 3) \
                or subindex > 1 \
                or (_param_mode > 5 and *params[param_analyzer_hold])) {
                // we draw the hold buffer
                valL = fft_holdL[iter];
                valR = fft_holdR[iter];
            } else {
                // we draw normally (no freeze)
                switch(_param_smooth) {
                    case 0:
                        // off
                        valL = fft_outL[iter];
                        valR = fft_outR[iter];
                        break;
                    case 1:
                        // falling
                        valL = fft_smoothL[iter];
                        valR = fft_smoothR[iter];
                        break;
                    case 2:
                        // smoothing
                        valL = fft_smoothL[iter];
                        valR = fft_smoothR[iter];
                        break;
                }
                // fill freeze buffer
                fft_freezeL[iter] = valL;
                fft_freezeR[iter] = valR;
            }
            if(*params[param_analyzer_view] < 2) {
                // #####################################
                // send values back to painting function
                // according to mode setting but only if
                // we are drawing lines or boxes
                // #####################################
                switch(_param_mode) {
                    case 3:
                        // stereo analyzer
                        if(subindex == 0 or subindex == 2) {
                            data[i] = dB_grid(fabs(valL) / _accuracy * 2.f + 1e-20, pow(64, *params[param_analyzer_level]), 0.5f);
                        } else {
                            data[i] = dB_grid(fabs(valR) / _accuracy * 2.f + 1e-20, pow(64, *params[param_analyzer_level]), 0.5f);
                        }
                        break;
                    case 4:
                        // we want to draw Stereo Image
                        if(subindex == 0 or subindex == 2) {
                            // Left channel signal
                            data[i] = fabs(valL) * stereo_coeff * 0.0001;
                        } else if (subindex == 1 or subindex == 3) {
                            // Right channel signal
                            data[i] = fabs(valR) * stereo_coeff * -0.0001f;
                        }
                        break;
                    case 5:
                        // We want to draw Stereo Difference
                        if(i) {
                            
                            //data[i] = stereo_coeff * valL * 2.f;
                            data[i] = (0.2f * (1.f - stereo_coeff) * pow(valL, 5.f) + 0.8f * ( 1.f - stereo_coeff) * pow(valL, 3.f) + valL * stereo_coeff) * 1.8f;
                        }
                        else data[i] = 0.f;
                        break;
                    default:
                        // normal analyzer behavior
                        data[i] = dB_grid(fabs(valL) / _accuracy * 2.f + 1e-20, pow(64, *params[param_analyzer_level]), 0.5f);
                        break;
                }
            }
        }
        else if(*params[param_analyzer_view] == 2) {
            // we have to draw splines, so we draw every x-pixel according to
            // the pre-generated fft_splinesL and fft_splinesR buffers
            data[i] = INFINITY;
            
//            int _splinepos = -1;
//            *mode=0;
//            
//            for (int i = 0; i<=points; i++) {
//                if (subindex == 1 and i == spline_buffer[_splinepos]) _splinepos++;
//                
//                freq = 20.f * pow (1000.f, (float)i / points); //1000=20000/20
//                float a0,b0,c0,d0,a1,b1,c1,d1,a2,b2,c2,d2;
//                
//                if(((i % lintrans == 0 and points - i > lintrans) or i == points - 1 ) and subindex == 0) {

//                    _iter = std::max(1, (int)floor(freq * (float)_accuracy / (float)srate));
//                    //printf("_iter %3d\n",_iter);
//                }
//                if(_iter > iter and subindex == 0)
//                {
//                    _splinepos++;
//                    spline_buffer[_splinepos] = _iter;
//                    //printf("_splinepos: %3d - lintrans: %3d\n", _splinepos,lintrans);
//                    if(fftdone and i and subindex == 0) 
//                    {
//                        // if fft was renewed, recalc the absolute values if frequencies
//                        // are skipped
//                        for(int j = iter + 1; j < _iter; j++) {
//                            fft_out[_iter] += fabs(fft_out[j]);
//                        }
//                    }
//                }
//                
//                if(fftdone and subindex == 1 and _splinepos >= 0 and (_splinepos % 3 == 0 or _splinepos == 0)) 
//                {
//                    float mleft, mright, y0, y1, y2, y3, y4;
//                    
//                    //jetzt spline interpolation
//                    y0 = dB_grid(fft_out[spline_buffer[_splinepos]] / _accuracy * 2.f + 1e-20, pow(64, *params[param_analyzer_level]), 0.5f);
//                    y1 = dB_grid(fft_out[spline_buffer[_splinepos + 1]] / _accuracy * 2.f + 1e-20, pow(64, *params[param_analyzer_level]), 0.5f);
//                    y2 = dB_grid(fft_out[spline_buffer[_splinepos + 2]] / _accuracy * 2.f + 1e-20, pow(64, *params[param_analyzer_level]), 0.5f);
//                    y3 = dB_grid(fft_out[spline_buffer[_splinepos + 3]] / _accuracy * 2.f + 1e-20, pow(64, *params[param_analyzer_level]), 0.5f);
//                    y4 = dB_grid(fft_out[spline_buffer[_splinepos + 4]] / _accuracy * 2.f + 1e-20, pow(64, *params[param_analyzer_level]), 0.5f);
//                    mleft  = y1 - y0;
//                    mright = y4 - y3;
//                    printf("y-werte %3d, %3d, %3d, %3d, %3d\n",y0,y1,y2,y3,y4);
//                    a0 = (-3*y3+15*y2-44*y1+32*y0+mright+22*mleft)/34;
//                    b0 = -(-3*y3+15*y2-78*y1+66*y0+mright+56*mleft)/34;
//                    c0 = mleft;
//                    d0 = y0;
//                    a1 = -(-15*y3+41*y2-50*y1+24*y0+5*mright+8*mleft)/34;
//                    b1 = (-6*y3+30*y2-54*y1+30*y0+2*mright+10*mleft)/17;
//                    c1 = -(3*y3-15*y2-24*y1+36*y0-mright+12*mleft)/34;
//                    d1 = y1;
//                    a2 = (-25*y3+40*y2-21*y1+6*y0+14*mright+2*mleft)/17;
//                    b2 = -(-33*y3+63*y2-42*y1+12*y0+11*mright+4*mleft)/17;
//                    c2 = (9*y3+6*y2-21*y1+6*y0-3*mright+2*mleft)/17;
//                    d2 = y2;
//                }
//                iter = _iter;
//                
//                
//                if(i > spline_buffer[_splinepos] and i <= spline_buffer[_splinepos + 1] and _splinepos >= 0 and subindex == 1) 
//                {
//                data[i] = a0 * pow(i / lintrans - _splinepos, 3) + b0 * pow(i / lintrans - _splinepos, 2) + c0 * (i / lintrans - _splinepos) + d0;
//                printf("1.spline\n");
//                }
//                if(i > spline_buffer[_splinepos + 1] and i <= spline_buffer[_splinepos + 2] and _splinepos >= 0 and subindex == 1) 
//                {
//                printf("2.spline\n");
//                data[i] = a1 * pow(i / lintrans - _splinepos, 3) + b1 * pow(i / lintrans - _splinepos, 2) + c1 * (i / lintrans - _splinepos) + d1;
//                }
//                if(i > spline_buffer[_splinepos + 2] and i <= spline_buffer[_splinepos + 3] and _splinepos >= 0 and subindex == 1) 
//                {
//                printf("3.spline\n");
//                data[i] = a2 * pow(i / lintrans - _splinepos, 3) + b2 * pow(i / lintrans - _splinepos, 2) + c2 * (i / lintrans - _splinepos) + d2;
//                }
//                if(subindex==1) printf("data[i] %3d _splinepos %2d\n", data[i], _splinepos);
//                if (subindex == 0) 
//                {
//                    data[i] = INFINITY;
//                } else data[i] = dB_grid(i/200, pow(64, *params[param_analyzer_level]), 0.5f);
//            
//            }
            
        }
        else {
            data[i] = INFINITY;
        }
    }
    
    // ###################################
    // colorize the curves/boxes according
    // to the chosen display settings
    // ###################################
    
    // change alpha (or color) for hold lines or stereo modes
    if((subindex == 1 and _param_mode < 3) or (subindex > 1 and _param_mode == 4)) {
        // subtle hold line on left, right, average or stereo image
        context->set_source_rgba(0.35, 0.4, 0.2, 0.2);
    }
    if(subindex == 0 and _param_mode == 3) {
        // left channel in stereo analyzer
        context->set_source_rgba(0.25, 0.10, 0.0, 0.3);
    }
    if(subindex == 1 and _param_mode == 3) {
        // right channel in stereo analyzer
        context->set_source_rgba(0.05, 0.25, 0.0, 0.3);
    }
    if(subindex == 2 and _param_mode == 3) {
        // left hold in stereo analyzer
        context->set_source_rgba(0.45, 0.30, 0.2, 0.2);
    }
    if(subindex == 3 and _param_mode == 3) {
        // right hold in stereo analyzer
        context->set_source_rgba(0.25, 0.45, 0.2, 0.2);
    }
    
    // #############################
    // choose a drawing mode between
    // boxes, lines and bars
    // #############################
    
    // modes:
    // 0: left
    // 1: right
    // 2: average
    // 3: stereo
    // 4: image
    // 5: difference
    
    // views:
    // 0: bars
    // 1: lines
    // 2: cubic splines
    
    // *modes to set:
    // 0: lines
    // 1: bars
    // 2: boxes (little things on the values position
    // 3: centered bars (0dB is centered in y direction)
    
    if (_param_mode > 3 and _param_mode < 6) {
        // centered viewing modes like stereo image and stereo difference
        if(!*params[param_analyzer_view]) {
            // boxes
            if(subindex > 1) {
                // boxes (hold)
                *mode = 2;
            } else {
                // bars (signal)
                *mode = 3;
            }
        } else {
            // lines
            *mode = 0;
        }
    } else if (_param_mode > 5 and _param_mode < 9) {
        // spectrum analyzer
        *mode = 4;
    } else if(!*params[param_analyzer_view]) {
        // bars
        if((subindex == 0 and _param_mode < 3) or (subindex <= 1 and _param_mode == 3)) {
            // draw bars
            *mode = 1;
        } else {
            // draw boxes
            *mode = 2;
        }
    } else {
        // draw lines
        *mode = 0;
    }
    ____analyzer_sanitize = 0;
    return true;
}

bool analyzer_audio_module::get_gridline(int index, int subindex, float &pos, bool &vertical, std::string &legend, cairo_iface *context) const
{ 
    bool out;
    if(*params[param_analyzer_mode] <= 3)
        out = get_freq_gridline(subindex, pos, vertical, legend, context, true, pow(64, *params[param_analyzer_level]), 0.5f);
    else if (*params[param_analyzer_mode] < 6)
        out = get_freq_gridline(subindex, pos, vertical, legend, context, true, 16, 0.f);
    else if (*params[param_analyzer_mode] < 9)
        out = get_freq_gridline(subindex, pos, vertical, legend, context, true, 0, 1.1f);
    else
        out = false;
        
    if(*params[param_analyzer_mode] > 3 and *params[param_analyzer_mode] < 6 and not vertical) {
        if(subindex == 30)
            legend="L";
        else if(subindex == 34)
            legend="R";
        else
            legend = "";
    }
    if(*params[param_analyzer_mode] > 5 and *params[param_analyzer_mode] < 9 and not vertical) {
        legend = "";
    }
    return out;
}
bool analyzer_audio_module::get_clear_all(int index) const {
    if(*params[param_analyzer_mode] != _mode_old or *params[param_analyzer_level]) {
        _mode_old = *params[param_analyzer_mode];
        return true;
    }
    return false;
}


///////////////////////////////////////////////////////////////////////////////////////////////

transientdesigner_audio_module::transientdesigner_audio_module() {
    active       = false;
    clip_inL     = 0.f;
    clip_inR     = 0.f;
    clip_outL    = 0.f;
    clip_outR    = 0.f;
    meter_inL    = 0.f;
    meter_inR    = 0.f;
    meter_outL   = 0.f;
    meter_outR   = 0.f;
    envelope     = 0.f;
    attack       = 0.f;
    release      = 0.f;
    attack_coef  = 0.f;
    release_coef = 0.f;
    attacking    = 0.f;
    sustaining   = 0.f;
    releasing    = 0.f;
    _attacking   = false;
    _sustaining  = false;
    _releasing   = false;
}

void transientdesigner_audio_module::activate() {
    active = true;
}

void transientdesigner_audio_module::deactivate() {
    active = false;
}

void transientdesigner_audio_module::params_changed() {
    
}

uint32_t transientdesigner_audio_module::process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask) {
    for(uint32_t i = offset; i < offset + numsamples; i++) {
        if(*params[param_bypass] > 0.5) {
            outs[0][i] = ins[0][i];
            outs[1][i] = ins[1][i];
            clip_inL    = 0.f;
            clip_inR    = 0.f;
            clip_outL   = 0.f;
            clip_outR   = 0.f;
            meter_inL  = 0.f;
            meter_inR  = 0.f;
            meter_outL = 0.f;
            meter_outR = 0.f;
            attacking  = 0.f;
            sustaining = 0.f;
            releasing  = 0.f;
        } else {
            // let meters fall a bit
            clip_inL    -= std::min(clip_inL,  numsamples);
            clip_inR    -= std::min(clip_inR,  numsamples);
            clip_outL   -= std::min(clip_outL, numsamples);
            clip_outR   -= std::min(clip_outR, numsamples);
            meter_inL = 0.f;
            meter_inR = 0.f;
            meter_outL = 0.f;
            meter_outR = 0.f;
            //attacking  -= std::min(attacking,  numsamples);
            //sustaining -= std::min(sustaining, numsamples);
            //releasing  -= std::min(releasing,  numsamples);
            
            float L = ins[0][i];
            float R = ins[1][i];
            
            // levels in
            L *= *params[param_level_in];
            R *= *params[param_level_in];
            
            // GUI stuff
            if(L > meter_inL) meter_inL = L;
            if(R > meter_inR) meter_inR = R;
            if(L > 1.f) clip_inL  = srate >> 3;
            if(R > 1.f) clip_inR  = srate >> 3;
            
            // get average value of input
            float s = (fabs(L) + fabs(R)) / 2;
            
            // envelope follower
            // this is the real envelope follower curve. It raises as
            // fast as the signal is raising and falls much slower
            // depending on the sample rate and the ffactor
            // (the falling factor)
            
            if(s > envelope)
                envelope = attack_coef * (envelope - s) + s;
            else
                envelope = release_coef * (envelope - s) + s;
            
            //envelope = (s >= envelope) ? s : envelope + 0.5 / (srate * 0.1)
            
            // attack follower
            // this is a curve which follows the envelope slowly.
            // It never can rise above the envelope. It reaches 70.7%
            // of the envelope in a certain amount of time set by the user
            float attdelta = (envelope - attack)
                           * 0.707
                           / (srate * *params[param_attack_time] * 0.001);
            attack += attdelta;
            // never raise above envelope
            attack = std::min(envelope, attack);
            // check if we're in attack phase
            if (attack < envelope) {
                if (!_attacking) {
                    _attacking = true;
                }
            } else if (_attacking) {
                _sustaining = true;
                _attacking = false;
            }
            
            // release follower
            // this is a curve which is always above the envelope. It
            // starts to fall when the envelope falls beneath the
            // sustain threshold
            float reldelta = (envelope / release - *params[param_sustain_threshold])
                           * 0.707 * release
                           / (*params[param_release_time] * srate * 0.001 * *params[param_sustain_threshold]);
            // release delta can never raise above 0
            reldelta = std::min(0.f, reldelta);
            release += reldelta;
            // never fall below envelope
            release = std::max(envelope, release);
            // check if we enter release phase
            if (reldelta < 0 and !_releasing) {
                _sustaining = false;
                _releasing = true;
            }
            // check if releasing has ended
            if (release == envelope) {
                _releasing = false;
            }
            
            // difference between attack and envelope
            float attdiff = envelope - attack;
            // difference between release and envelope
            float reldiff = release - envelope;
            
            // amplification factor from attack and release curve
            float sum = 1 + attdiff * *params[param_attack_boost]
                          + *params[param_release_boost]
                          * ((release - reldiff == 0) ? 0 : (release / (release - reldiff) - 1));
            L *= sum;
            R *= sum;
            
            // prevent a phase of 180°
            if (ins[0][i] * L < 0) L = 0;
            if (ins[1][i] * R < 0) R = 0;
            
            // levels out
            L *= *params[param_level_out];
            R *= *params[param_level_out];
            
            // output
            outs[0][i] = L * *params[param_mix] + ins[0][i] * (*params[param_mix] * -1 + 1);
            outs[1][i] = R * *params[param_mix] + ins[1][i] * (*params[param_mix] * -1 + 1);
            
            // clip LED's
            if(L > 1.f) clip_outL = srate >> 3;
            if(R > 1.f) clip_outR = srate >> 3;
            if(L > meter_outL) meter_outL = L;
            if(R > meter_outR) meter_outR = R;
            
            // action LED's
            if (_attacking)  attacking  = 1; else attacking = 0;//srate >> 3;
            if (_sustaining) sustaining = 1; else sustaining = 0;//srate >> 3;
            if (_releasing)  releasing  = 1; else releasing = 0;//srate >> 3;
        }
    }
    // draw meters
    SET_IF_CONNECTED(clip_inL);
    SET_IF_CONNECTED(clip_inR);
    SET_IF_CONNECTED(clip_outL);
    SET_IF_CONNECTED(clip_outR);
    SET_IF_CONNECTED(meter_inL);
    SET_IF_CONNECTED(meter_inR);
    SET_IF_CONNECTED(meter_outL);
    SET_IF_CONNECTED(meter_outR);
    SET_IF_CONNECTED(attacking);
    SET_IF_CONNECTED(sustaining);
    SET_IF_CONNECTED(releasing);
    return outputs_mask;
}

void transientdesigner_audio_module::set_sample_rate(uint32_t sr)
{
    srate = sr;
    attack_coef  = exp(log(0.01) / (0.001 * srate));
    release_coef = exp(log(0.01) / (0.2f  * srate));
}
