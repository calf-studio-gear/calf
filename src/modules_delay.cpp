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
#include <math.h>
#include <calf/giface.h>
#include <calf/modules_delay.h>
#include <calf/modules_dev.h>
#ifndef _MSC_VER
#include <sys/time.h>
#endif
using namespace dsp;
using namespace calf_plugins;

#define SET_IF_CONNECTED(name) if (params[AM::param_##name] != NULL) *params[AM::param_##name] = name;

/**********************************************************************
 * REVERB by Krzysztof Foltman
**********************************************************************/

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
    int meter[] = {param_meter_inL, param_meter_inR, param_meter_outL, param_meter_outR};
    int clip[] = {param_clip_inL, param_clip_inR, param_clip_outL, param_clip_outR};
    meters.init(params, meter, clip, 4, srate);
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
    for (uint32_t i = offset; i < numsamples; i++) {
        float dry = dryamount.get();
        float wet = amount.get();
        stereo_sample<float> s(ins[0][i] * *params[param_level_in],
                               ins[1][i] * *params[param_level_in]);
        stereo_sample<float> s2 = pre_delay.process(s, predelay_amt);
        
        float rl = s2.left, rr = s2.right;
        rl = left_lo.process(left_hi.process(rl));
        rr = right_lo.process(right_hi.process(rr));
        if (*params[par_on] > 0.5)
            reverb.process(rl, rr);
        outs[0][i] = dry*s.left;
        outs[1][i] = dry*s.right;
        if (*params[par_on] > 0.5) {
            outs[0][i] += wet*rl;
            outs[1][i] += wet*rr;
        }
        outs[0][i] *= *params[param_level_out];
        outs[1][i] *= *params[param_level_out];
        
        float values[] = {s.left, s.right, outs[0][i], outs[1][i]};
        meters.process(values);
    }
    meters.fall(numsamples);
    reverb.extra_sanitize();
    left_lo.sanitize();
    left_hi.sanitize();
    right_lo.sanitize();
    right_hi.sanitize();
    return outputs_mask;
}

/**********************************************************************
 * VINTAGE DELAY by Krzysztof Foltman
**********************************************************************/

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

char *vintage_delay_audio_module::configure(const char *key, const char *value)
{
    if (!strcmp(key, "pattern_l"))
    {
        return NULL;
    }
    else
    if (!strcmp(key, "pattern_r"))
    {
        return NULL;
    }
    else
    return strdup("Unsupported key");
}

void vintage_delay_audio_module::params_changed()
{
    double bpm = 120;
    bpm = convert_periodic(*params[param_bpm + (int)((periodic_unit)int(*params[param_timing]))],
                                  (periodic_unit)int(*params[param_timing]), UNIT_BPM);
    
    // not implemented by now
    //switch ((int)*params[par_frag]) {
        //case FRAG_PERIODIC:
            
            //break;
        //case FRAG_PATTERN:
            //int amnt = *params[par_pbeats] * *params[par_pfrag];
            //break;
    //}
    
    float unit = 60.0 * srate / (bpm * *params[par_divide]);
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

    int meter[] = {param_meter_inL, param_meter_inR, param_meter_outL, param_meter_outR};
    int clip[] = {param_clip_inL, param_clip_inR, param_clip_outL, param_clip_outR};
    meters.init(params, meter, clip, 4, srate);
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
    float out_left, out_right, del_left, del_right, inL, inR;
    
    switch(mixmode)
    {
        case MIXMODE_STEREO:
        case MIXMODE_PINGPONG:
        {
            int v = mixmode == MIXMODE_PINGPONG ? 1 : 0;
            for(uint32_t i = offset; i < end; i++)
            {       
                inL = ins[0][i] * *params[param_level_in];
                inR = ins[1][i] * *params[param_level_in];
                delayline_impl(age, deltime_l, *params[param_on] > 0.5 ? inL : 0, buffers[v][(bufptr - deltime_l) & ADDR_MASK], out_left, del_left, amt_left, fb_left);
                delayline_impl(age, deltime_r, *params[param_on] > 0.5 ? inR : 0, buffers[1 - v][(bufptr - deltime_r) & ADDR_MASK], out_right, del_right, amt_right, fb_right);
                delay_mix(inL, inR, out_left, out_right, dry.get(), chmix.get());
                
                age++;
                outs[0][i] = out_left * *params[param_level_out];
                outs[1][i] = out_right * *params[param_level_out];
                buffers[0][bufptr] = del_left; buffers[1][bufptr] = del_right;
                bufptr = (bufptr + 1) & (MAX_DELAY - 1);
                float values[] = {inL, inR, outs[0][i], outs[1][i]};
                meters.process(values);
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
                inL = ins[0][i] * *params[param_level_in];
                inR = ins[1][i] * *params[param_level_in];
                delayline2_impl(age, deltime_l, *params[param_on] > 0.5 ? inL : 0, buffers[v][(bufptr - deltime_l_corr) & ADDR_MASK], buffers[v][(bufptr - deltime_fb) & ADDR_MASK], out_left, del_left, amt_left, fb_left);
                delayline2_impl(age, deltime_r, *params[param_on] > 0.5 ? inR : 0, buffers[1 - v][(bufptr - deltime_r_corr) & ADDR_MASK], buffers[1-v][(bufptr - deltime_fb) & ADDR_MASK], out_right, del_right, amt_right, fb_right);
                delay_mix(inL, inR, out_left, out_right, dry.get(), chmix.get());
                
                age++;
                outs[0][i] = out_left * *params[param_level_out];
                outs[1][i] = out_right * *params[param_level_out];
                buffers[0][bufptr] = del_left; buffers[1][bufptr] = del_right;
                bufptr = (bufptr + 1) & (MAX_DELAY - 1);
                float values[] = {inL, inR, outs[0][i], outs[1][i]};
                meters.process(values);
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
    meters.fall(numsamples);
    return ostate;
}

/**********************************************************************
 * COMPENSATION DELAY LINE by Vladimir Sadovnikov 
**********************************************************************/

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

    uint32_t min_buf_size = (uint32_t)(srate * COMP_DELAY_MAX_DELAY * 2);
    uint32_t new_buf_size = 2;
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
        
    int meter[] = {param_meter_inL,  param_meter_inR, param_meter_outL, param_meter_outR};
    int clip[]  = {param_clip_inL, param_clip_inR, param_clip_outL, param_clip_outR};
    meters.init(params, meter, clip, 4, srate);
}

uint32_t comp_delay_audio_module::process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask)
{
    bool bypassed   = bypass.update(*params[param_bypass] > 0.5f, numsamples);
    bool stereo     = ins[1];
    uint32_t w_ptr  = write_ptr;
    uint32_t b_mask = buf_size - 2;
    uint32_t end    = offset + numsamples;
    uint32_t off    = offset;
    
    if (bypassed) {
        float values[] = {0,0,0,0};
        while(offset < end) {
            outs[0][offset] = ins[0][offset];
            buffer[w_ptr]   = ins[0][offset];
            if (stereo) {
                outs[1][offset]   = ins[1][offset];
                buffer[w_ptr + 1] = ins[1][offset];
            }
            w_ptr = (w_ptr + 2) & b_mask;
            meters.process(values);
            ++offset;
        }
    } else {
        uint32_t r_ptr  = (write_ptr + buf_size - delay) & b_mask; // Unsigned math, that's why we add buf_size
        float dry       = *params[par_dry];
        float wet       = *params[par_wet];
        float L = 0, R = 0;
        
        for (uint32_t i=offset; i<end; i++)
        {
            L = ins[0][i] * *params[param_level_in];
            buffer[w_ptr] = L;
            outs[0][i] = dry * L + wet * buffer[r_ptr];
            outs[0][i] *= *params[param_level_out];
            if (stereo) {
                R = ins[1][i] * *params[param_level_in];
                buffer[w_ptr + 1] = R;
                outs[1][i] = dry * R + wet * buffer[r_ptr + 1];
                outs[1][i] *= *params[param_level_out];
            }
            w_ptr = (w_ptr + 2) & b_mask;
            r_ptr = (r_ptr + 2) & b_mask;
            
            float values[] = {L, R, outs[0][i], outs[1][i]};
            meters.process(values);
        }
    }
    if (!bypassed)
        bypass.crossfade(ins, outs, stereo ? 2 : 1, off, numsamples);
    write_ptr = w_ptr;
    meters.fall(numsamples);
    return outputs_mask;
}

/**********************************************************************
 * HAAS enhancer by Vladimir Sadovnikov
**********************************************************************/

haas_enhancer_audio_module::haas_enhancer_audio_module()
{
    buffer              = NULL;
    srate               = 0;
    buf_size            = 0;
    write_ptr           = 0;

    m_source            = 2;
    s_delay[0]          = 0;
    s_delay[1]          = 0;
    s_bal_l[0]          = 0.0f;
    s_bal_l[1]          = 0.0f;
    s_bal_r[0]          = 0.0f;
    s_bal_r[1]          = 0.0f;
}

haas_enhancer_audio_module::~haas_enhancer_audio_module()
{
    if (buffer != NULL)
    {
        delete [] buffer;
        buffer = NULL;
    }
}

void haas_enhancer_audio_module::params_changed()
{
    m_source            = (uint32_t)(*params[par_m_source]);
    s_delay[0]          = (uint32_t)(*params[par_s_delay0] * 0.001 * srate);
    s_delay[1]          = (uint32_t)(*params[par_s_delay1] * 0.001 * srate);
    
    float phase0        = ((*params[par_s_phase0]) > 0.5f) ? 1.0f : -1.0f;
    float phase1        = ((*params[par_s_phase1]) > 0.5f) ? 1.0f : -1.0f;
    
    s_bal_l[0]          = (*params[par_s_balance0] + 1) / 2 * (*params[par_s_gain0]) * phase0;
    s_bal_r[0]          = (1.0 - (*params[par_s_balance0] + 1) / 2) * (*params[par_s_gain0]) * phase0;
    s_bal_l[1]          = (*params[par_s_balance1] + 1) / 2 * (*params[par_s_gain1]) * phase1;
    s_bal_r[1]          = (1.0 - (*params[par_s_balance1] + 1) / 2) * (*params[par_s_gain1]) * phase1;
}

void haas_enhancer_audio_module::activate()
{
    write_ptr   = 0;
}

void haas_enhancer_audio_module::deactivate()
{
}

void haas_enhancer_audio_module::set_sample_rate(uint32_t sr)
{
    srate = sr;
    float *old_buf = buffer;

    uint32_t min_buf_size = (uint32_t)(srate * HAAS_ENHANCER_MAX_DELAY);
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
        
    int meter[] = {param_meter_inL, param_meter_inR,  param_meter_outL, param_meter_outR, param_meter_sideL, param_meter_sideR};
    int clip[] = {param_clip_inL, param_clip_inR, param_clip_outL, param_clip_outR, -1, -1};
    meters.init(params, meter, clip, 6, srate);
}

uint32_t haas_enhancer_audio_module::process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask)
{
    bool bypassed  = bypass.update(*params[param_bypass] > 0.5f, numsamples);
    uint32_t end   = offset + numsamples;
    uint32_t off_  = offset;
    uint32_t w_ptr = write_ptr;
    
    // Sample variables
    float mid, side[2], side_l, side_r;
    // Boundaries and pointers
    uint32_t b_mask = buf_size-1;
            
    while(offset < end) {
        float values[] = {0, 0, 0, 0, 0, 0};
        float *p = values;
        
        // Get middle sample
        switch (m_source)
        {
            case 0:  mid = ins[0][offset]; break;
            case 1:  mid = ins[1][offset]; break;
            case 2:  mid = (ins[0][offset] + ins[1][offset]) * 0.5f; break;
            case 3:  mid = (ins[0][offset] - ins[1][offset]) * 0.5f; break;
            default: mid = 0.0f;
        }

        // Store middle
        buffer[w_ptr] = mid * *params[param_level_in];
            
        if (bypassed) {
            outs[0][offset] = ins[0][offset];
            outs[1][offset] = ins[1][offset];
        } else {
            // Delays for mid and side. Unsigned math, that's why we add buf_size
            uint32_t s0_ptr = (w_ptr + buf_size - s_delay[0]) & b_mask;
            uint32_t s1_ptr = (w_ptr + buf_size - s_delay[1]) & b_mask;
    
            // Calculate side
            mid     = mid * *params[param_level_in];
            if (*params[par_m_phase] > 0.5f)
                mid = -mid;
            side[0] = buffer[s0_ptr] * (*params[par_s_gain]);
            side[1] = buffer[s1_ptr] * (*params[par_s_gain]);
            side_l  = side[0] * s_bal_l[0] - side[1] * s_bal_l[1];
            side_r  = side[1] * s_bal_r[1] - side[0] * s_bal_r[0];
    
            // Output stereo image
            outs[0][offset] = (mid + side_l) * *params[param_level_out];
            outs[1][offset] = (mid + side_r) * *params[param_level_out];
    
            // Update pointers
            s0_ptr = (s0_ptr + 1) & b_mask;
            s1_ptr = (s1_ptr + 1) & b_mask;
            
            *p++ = ins[0][offset];  *p++ = ins[1][offset];
            *p++ = outs[0][offset]; *p++ = outs[1][offset];
            *p++ = side_l;          *p++ = side_r;
        }
        meters.process (values);
        w_ptr = (w_ptr + 1) & b_mask;
        ++offset;
    }
    if (!bypassed)
        bypass.crossfade(ins, outs, 2, off_, numsamples);
    write_ptr = w_ptr;
    meters.fall(numsamples);
    return outputs_mask;
}

/**********************************************************************
 * REVERSE DELAY
**********************************************************************/

reverse_delay_audio_module::reverse_delay_audio_module()
{
    for (int i = 0; i < MAX_DELAY; i++) {
        buffers[0][i] = 0.f;
        buffers[1][i] = 0.f;
    }

    counters[0] = 0;
    counters[1] = 0;

    feedback_buf[0] = 0;
    feedback_buf[1] = 0;
}

void reverse_delay_audio_module::params_changed()
{
    if (*params[par_sync] > 0.5f)
        *params[par_bpm] = *params[par_bpm_host];

    //Max delay line length: (60*192000/30)*16 = 6144000, see buffers declaration
    float unit = 60.0 * srate / (*params[par_bpm] * *params[par_divide]);
    deltime_l = dsp::fastf2i_drm(unit * *params[par_time_l]);
    deltime_r = dsp::fastf2i_drm(unit * *params[par_time_r]);

    fb_val.set_inertia(*params[par_feedback]);
    dry.set_inertia(*params[par_amount]);

    counters[0] = 0;
    counters[1] = 0;

    ow[0].set_coef((*params[par_window] + 0.005), deltime_l/2);
    ow[1].set_coef((*params[par_window] + 0.005), deltime_r/2);

    width.set_inertia(*params[par_width]);

    //Cleanup delay line buffers if reset
    if(*params[par_reset])
    {
        for (int i = 0; i < MAX_DELAY; i++) {
            buffers[0][i] = 0.f;
            buffers[1][i] = 0.f;
        }

        feedback_buf[0] = 0;
        feedback_buf[1] = 0;
    }
}

void reverse_delay_audio_module::activate()
{
}

void reverse_delay_audio_module::deactivate()
{
}

void reverse_delay_audio_module::set_sample_rate(uint32_t sr)
{
    srate = sr;
    fb_val.set_sample_rate(sr);
    dry.set_sample_rate(sr);
    width.set_sample_rate(sr);
    int meter[] = {param_meter_inL,  param_meter_inR, param_meter_outL, param_meter_outR};
    int clip[]  = {param_clip_inL, param_clip_inR, param_clip_outL, param_clip_outR};
    meters.init(params, meter, clip, 4, srate);
}

static float reverse_delay_line_impl(float in, float* buf, int* counter, int length)
{
    float out = 0;

    //Read data
    if(*counter < length - 1)
    {
        unsigned int read_counter = length - 1 - (*counter);
        out = buf[read_counter];
    }

    //Write data
    *(buf + (*counter)) = in;
    (*counter)++;
    if ((*counter) > length-1)
        (*counter) = 0;

    return (out);
}

uint32_t reverse_delay_audio_module::process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask)
{
    bool bypassed  = bypass.update(*params[param_bypass] > 0.5f, numsamples);
    uint32_t ostate = 3; // XXXKF optimize!
    uint32_t end = offset + numsamples;

    //Loop
    for(uint32_t i = offset; i < end; i++)
    {
        //Update sync led's
        if(counters[0] < deltime_l/4)
            *params[par_sync_led_l] = true;
        else
            *params[par_sync_led_l] = false;

        if(counters[1] < deltime_r/4)
            *params[par_sync_led_r] = true;
        else
            *params[par_sync_led_r] = false;
        //Process
        float inL = 0., inR = 0., outL = 0., outR = 0.;
        if (bypassed) {
            outs[0][offset] = ins[0][offset];
            outs[1][offset] = ins[1][offset];
        } else {
            float feedback_val = fb_val.get();
            float st_width_val = width.get();
    
            inL = ins[0][i] + st_width_val*ins[1][i];
            inR = ins[1][i]*(1 - st_width_val);
            inL *= *params[param_level_in];
            inR *= *params[param_level_in];
            
            inL = inL + feedback_buf[0]* feedback_val*(1 - st_width_val) + feedback_buf[1]* st_width_val*feedback_val;
            inR = inR + feedback_buf[1]* feedback_val*(1 - st_width_val) + feedback_buf[0]* st_width_val*feedback_val;
    
            outL = reverse_delay_line_impl(inL, &buffers[0][0], &counters[0], deltime_l);
            outR = reverse_delay_line_impl(inR, &buffers[1][0], &counters[1], deltime_r);
            feedback_buf[0] = outL;
            feedback_buf[1] = outR;
    
            outL *= ow[0].get();
            outR *= ow[1].get();
    
            outL = outL * (1 + dry.get()) + inL*(1 - dry.get());
            outR = outR * (1 + dry.get()) + inR*(1 - dry.get());
    
            outs[0][i] = outL * *params[param_level_out];
            outs[1][i] = outR * *params[param_level_out];
            bypass.crossfade(ins, outs, 2, offset, numsamples);
        }
        float values[] = {inL, inR, outL, outR};
        meters.process(values);
    }
    meters.fall(numsamples);
    return ostate;
}
