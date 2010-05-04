/* Calf DSP Library utility application.
 * Reusable audio effect classes - implementation.
 * Copyright (C) 2007 Krzysztof Foltman
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

#include <calf/audio_fx.h>

using namespace dsp;

simple_phaser::simple_phaser(int _max_stages, float *x1vals, float *y1vals)
{
    max_stages = _max_stages;
    x1 = x1vals;
    y1 = y1vals;

    set_base_frq(1000);
    set_mod_depth(1000);
    set_fb(0);
    state = 0;
    cnt = 0;
    stages = 0;
    set_stages(_max_stages);    
}

void simple_phaser::set_stages(int _stages)
{
    if (_stages > stages)
    {
        assert(_stages <= max_stages);
        if (_stages > max_stages)
            _stages = max_stages;
        for (int i = stages; i < _stages; i++)
        {
            x1[i] = x1[stages-1];
            y1[i] = y1[stages-1];
        }
    }
    stages = _stages;
}

void simple_phaser::reset()
{
    cnt = 0;
    state = 0;
    phase.set(0);
    for (int i = 0; i < max_stages; i++)
        x1[i] = y1[i] = 0;
    control_step();
}

void simple_phaser::control_step()
{
    cnt = 0;
    int v = phase.get() + 0x40000000;
    int sign = v >> 31;
    v ^= sign;
    // triangle wave, range from 0 to INT_MAX
    double vf = (double)((v >> 16) * (1.0 / 16384.0) - 1);
    
    float freq = base_frq * pow(2.0, vf * mod_depth / 1200.0);
    freq = dsp::clip<float>(freq, 10.0, 0.49 * sample_rate);
    stage1.set_ap_w(freq * (M_PI / 2.0) * odsr);
    phase += dphase * 32;
    for (int i = 0; i < stages; i++)
    {
        dsp::sanitize(x1[i]);
        dsp::sanitize(y1[i]);
    }
    dsp::sanitize(state);
}

void simple_phaser::process(float *buf_out, float *buf_in, int nsamples)
{
    for (int i=0; i<nsamples; i++) {
        cnt++;
        if (cnt == 32)
            control_step();
        float in = *buf_in++;
        float fd = in + state * fb;
        for (int j = 0; j < stages; j++)
            fd = stage1.process_ap(fd, x1[j], y1[j]);
        state = fd;
        
        float sdry = in * gs_dry.get();
        float swet = fd * gs_wet.get();
        *buf_out++ = sdry + swet;
    }
}

float simple_phaser::freq_gain(float freq, float sr) const
{
    typedef std::complex<double> cfloat;
    freq *= 2.0 * M_PI / sr;
    cfloat z = 1.0 / exp(cfloat(0.0, freq)); // z^-1
    
    cfloat p = cfloat(1.0);
    cfloat stg = stage1.h_z(z);
    
    for (int i = 0; i < stages; i++)
        p = p * stg;
    
    p = p / (cfloat(1.0) - cfloat(fb) * p);        
    return std::abs(cfloat(gs_dry.get_last()) + cfloat(gs_wet.get_last()) * p);
}

///////////////////////////////////////////////////////////////////////////////////

void biquad_filter_module::calculate_filter(float freq, float q, int mode, float gain)
{
    if (mode <= mode_36db_lp) {
        order = mode + 1;
        left[0].set_lp_rbj(freq, pow(q, 1.0 / order), srate, gain);
    } else if ( mode_12db_hp <= mode && mode <= mode_36db_hp ) {
        order = mode - mode_12db_hp + 1;
        left[0].set_hp_rbj(freq, pow(q, 1.0 / order), srate, gain);
    } else if ( mode_6db_bp <= mode && mode <= mode_18db_bp ) {
        order = mode - mode_6db_bp + 1;
        left[0].set_bp_rbj(freq, pow(q, 1.0 / order), srate, gain);
    } else { // mode_6db_br <= mode <= mode_18db_br
        order = mode - mode_6db_br + 1;
        left[0].set_br_rbj(freq, order * 0.1 * q, srate, gain);
    }
    
    right[0].copy_coeffs(left[0]);
    for (int i = 1; i < order; i++) {
        left[i].copy_coeffs(left[0]);
        right[i].copy_coeffs(left[0]);
    }
}

void biquad_filter_module::filter_activate()
{
    for (int i=0; i < order; i++) {
        left[i].reset();
        right[i].reset();
    }
}

void biquad_filter_module::sanitize()
{
    for (int i=0; i < order; i++) {
        left[i].sanitize();
        right[i].sanitize();
    }
}

int biquad_filter_module::process_channel(uint16_t channel_no, const float *in, float *out, uint32_t numsamples, int inmask) {
    dsp::biquad_d1<float> *filter;
    switch (channel_no) {
    case 0:
        filter = left;
        break;
        
    case 1:
        filter = right;
        break;
    
    default:
        assert(false);
        return 0;
    }
    
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

float biquad_filter_module::freq_gain(int subindex, float freq, float srate) const
{
    float level = 1.0;
    for (int j = 0; j < order; j++)
        level *= left[j].freq_gain(freq, srate);
    return level;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////

/// Distortion Module by Tom Szilagyi
///
/// This module provides a blendable saturation stage
///////////////////////////////////////////////////////////////////////////////////////////////

tap_distortion::tap_distortion()
{
    is_active = false;
    srate = 0;
    meter = 0.f;
}

void tap_distortion::activate()
{
    is_active = true;
    set_params(0.f, 0.f);
}
void tap_distortion::deactivate()
{
    is_active = false;
}

void tap_distortion::set_params(float blend, float drive)
{
    // set distortion coeffs
    if ((drive_old != drive) || (blend_old != blend)) {
        rdrive = 12.0f / drive;
        rbdr = rdrive / (10.5f - blend) * 780.0f / 33.0f;
        kpa = D(2.0f * (rdrive*rdrive) - 1.0f) + 1.0f;
        kpb = (2.0f - kpa) / 2.0f;
        ap = ((rdrive*rdrive) - kpa + 1.0f) / 2.0f;
        kc = kpa / D(2.0f * D(2.0f * (rdrive*rdrive) - 1.0f) - 2.0f * rdrive*rdrive);

        srct = (0.1f * srate) / (0.1f * srate + 1.0f);
        sq = kc*kc + 1.0f;
        knb = -1.0f * rbdr / D(sq);
        kna = 2.0f * kc * rbdr / D(sq);
        an = rbdr*rbdr / sq;
        imr = 2.0f * knb + D(2.0f * kna + 4.0f * an - 1.0f);
        pwrq = 2.0f / (imr + 1.0f);
        
        drive_old = drive;
        blend_old = blend;
    }
}

void tap_distortion::set_sample_rate(uint32_t sr)
{
    srate = sr;
}

float tap_distortion::process(float in)
{
    meter = 0.f;
    float out = 0.f;
    float proc = in;
    float med;
    if (proc >= 0.0f) {
        med = (D(ap + proc * (kpa - proc)) + kpb) * pwrq;
    } else {
        med = (D(an - proc * (kna + proc)) + knb) * pwrq * -1.0f;
    }
    proc = srct * (med - prev_med + prev_out);
    prev_med = M(med);
    prev_out = M(proc);
    out = proc;
    meter = proc;
    return out;
}

float tap_distortion::get_distortion_level()
{
    return meter;
}
