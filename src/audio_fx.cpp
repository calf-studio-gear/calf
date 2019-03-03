/* Calf DSP Library
 * Reusable audio effect classes - implementation.
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

#include <calf/audio_fx.h>
#include <calf/giface.h>
#include <limits.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

using namespace calf_plugins;
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
    if (lfo_active)
        phase += dphase * 32;
    for (int i = 0; i < stages; i++)
    {
        dsp::sanitize(x1[i]);
        dsp::sanitize(y1[i]);
    }
    dsp::sanitize(state);
}

void simple_phaser::process(float *buf_out, float *buf_in, int nsamples, bool active, float level_in, float level_out)
{
    for (int i=0; i<nsamples; i++) {
        cnt++;
        if (cnt == 32)
            control_step();
        float in = *buf_in++ * level_in;
        float fd = in + state * fb;
        for (int j = 0; j < stages; j++)
            fd = stage1.process_ap(fd, x1[j], y1[j]);
        state = fd;

        float sdry = in * gs_dry.get();
        float swet = fd * gs_wet.get();
        *buf_out++ = (sdry + (active ? swet : 0)) * level_out;
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
    } else if ( mode_6db_br <= mode && mode <= mode_18db_br) {
        order = mode - mode_6db_br + 1;
        left[0].set_br_rbj(freq, order * 0.1 * q, srate, gain);
    } else { // mode_allpass
        order = 3;
        left[0].set_allpass(freq, 1, srate);
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

int biquad_filter_module::process_channel(uint16_t channel_no, const float *in, float *out, uint32_t numsamples, int inmask, float lvl_in, float lvl_out) {
    dsp::biquad_d1 *filter;
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
                for (uint32_t i = 0; i < numsamples; i++) {
                    out[i] = filter[0].process(in[i] * lvl_in);
                    out[i] *= lvl_out;
                }
                break;
            case 2:
                for (uint32_t i = 0; i < numsamples; i++) {
                    out[i] = filter[1].process(filter[0].process(in[i] * lvl_in));
                    out[i] *= lvl_out;
                }
                break;
            case 3:
                for (uint32_t i = 0; i < numsamples; i++) {
                    out[i] = filter[2].process(filter[1].process(filter[0].process(in[i] * lvl_in)));
                    out[i] *= lvl_out;
                }
                break;
        }
    } else {
        if (filter[order - 1].empty())
            return 0;
        switch(order) {
            case 1:
                for (uint32_t i = 0; i < numsamples; i++) {
                    out[i] = filter[0].process_zeroin();
                    out[i] *= lvl_out;
                }
                break;
            case 2:
                if (filter[0].empty())
                    for (uint32_t i = 0; i < numsamples; i++) {
                        out[i] = filter[1].process_zeroin();
                        out[i] *= lvl_out;
                    }
                else
                    for (uint32_t i = 0; i < numsamples; i++) {
                        out[i] = filter[1].process(filter[0].process_zeroin());
                        out[i] *= lvl_out;
                    }
                break;
            case 3:
                if (filter[1].empty())
                    for (uint32_t i = 0; i < numsamples; i++) {
                        out[i] = filter[2].process_zeroin();
                        out[i] *= lvl_out;
                    }
                else
                    for (uint32_t i = 0; i < numsamples; i++) {
                        out[i] = filter[2].process(filter[1].process(filter[0].process_zeroin()));
                        out[i] *= lvl_out;
                    }
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

void reverb::update_times()
{
    switch(type)
    {
    case 0:
        tl[0] =  397 << 16, tr[0] =  383 << 16;
        tl[1] =  457 << 16, tr[1] =  429 << 16;
        tl[2] =  549 << 16, tr[2] =  631 << 16;
        tl[3] =  649 << 16, tr[3] =  756 << 16;
        tl[4] =  773 << 16, tr[4] =  803 << 16;
        tl[5] =  877 << 16, tr[5] =  901 << 16;
        break;
    case 1:
        tl[0] =  697 << 16, tr[0] =  783 << 16;
        tl[1] =  957 << 16, tr[1] =  929 << 16;
        tl[2] =  649 << 16, tr[2] =  531 << 16;
        tl[3] = 1049 << 16, tr[3] = 1177 << 16;
        tl[4] =  473 << 16, tr[4] =  501 << 16;
        tl[5] =  587 << 16, tr[5] =  681 << 16;
        break;
    case 2:
    default:
        tl[0] =  697 << 16, tr[0] =  783 << 16;
        tl[1] =  957 << 16, tr[1] =  929 << 16;
        tl[2] =  649 << 16, tr[2] =  531 << 16;
        tl[3] = 1249 << 16, tr[3] = 1377 << 16;
        tl[4] = 1573 << 16, tr[4] = 1671 << 16;
        tl[5] = 1877 << 16, tr[5] = 1781 << 16;
        break;
    case 3:
        tl[0] = 1097 << 16, tr[0] = 1087 << 16;
        tl[1] = 1057 << 16, tr[1] = 1031 << 16;
        tl[2] = 1049 << 16, tr[2] = 1039 << 16;
        tl[3] = 1083 << 16, tr[3] = 1055 << 16;
        tl[4] = 1075 << 16, tr[4] = 1099 << 16;
        tl[5] = 1003 << 16, tr[5] = 1073 << 16;
        break;
    case 4:
        tl[0] =  197 << 16, tr[0] =  133 << 16;
        tl[1] =  357 << 16, tr[1] =  229 << 16;
        tl[2] =  549 << 16, tr[2] =  431 << 16;
        tl[3] =  949 << 16, tr[3] = 1277 << 16;
        tl[4] = 1173 << 16, tr[4] = 1671 << 16;
        tl[5] = 1477 << 16, tr[5] = 1881 << 16;
        break;
    case 5:
        tl[0] =  197 << 16, tr[0] =  133 << 16;
        tl[1] =  257 << 16, tr[1] =  179 << 16;
        tl[2] =  549 << 16, tr[2] =  431 << 16;
        tl[3] =  619 << 16, tr[3] =  497 << 16;
        tl[4] = 1173 << 16, tr[4] = 1371 << 16;
        tl[5] = 1577 << 16, tr[5] = 1881 << 16;
        break;
    }

    float fDec=1000 + 2400.f * diffusion;
    for (int i = 0 ; i < 6; i++) {
        ldec[i]=exp(-float(tl[i] >> 16) / fDec),
        rdec[i]=exp(-float(tr[i] >> 16) / fDec);
    }
}

void reverb::reset()
{
    apL1.reset();apR1.reset();
    apL2.reset();apR2.reset();
    apL3.reset();apR3.reset();
    apL4.reset();apR4.reset();
    apL5.reset();apR5.reset();
    apL6.reset();apR6.reset();
    lp_left.reset();lp_right.reset();
    old_left = 0; old_right = 0;
}

void reverb::process(float &left, float &right)
{
    unsigned int ipart = phase.ipart();

    // the interpolated LFO might be an overkill here
    int lfo = phase.lerp_by_fract_int<int, 14, int>(sine.data[ipart], sine.data[ipart+1]) >> 2;
    phase += dphase;

    left += old_right;
    left = apL1.process_allpass_comb_lerp16(left, tl[0] - 45*lfo, ldec[0]);
    left = apL2.process_allpass_comb_lerp16(left, tl[1] + 47*lfo, ldec[1]);
    float out_left = left;
    left = apL3.process_allpass_comb_lerp16(left, tl[2] + 54*lfo, ldec[2]);
    left = apL4.process_allpass_comb_lerp16(left, tl[3] - 69*lfo, ldec[3]);
    left = apL5.process_allpass_comb_lerp16(left, tl[4] + 69*lfo, ldec[4]);
    left = apL6.process_allpass_comb_lerp16(left, tl[5] - 46*lfo, ldec[5]);
    old_left = lp_left.process(left * fb);
    sanitize(old_left);

    right += old_left;
    right = apR1.process_allpass_comb_lerp16(right, tr[0] - 45*lfo, rdec[0]);
    right = apR2.process_allpass_comb_lerp16(right, tr[1] + 47*lfo, rdec[1]);
    float out_right = right;
    right = apR3.process_allpass_comb_lerp16(right, tr[2] + 54*lfo, rdec[2]);
    right = apR4.process_allpass_comb_lerp16(right, tr[3] - 69*lfo, rdec[3]);
    right = apR5.process_allpass_comb_lerp16(right, tr[4] + 69*lfo, rdec[4]);
    right = apR6.process_allpass_comb_lerp16(right, tr[5] - 46*lfo, rdec[5]);
    old_right = lp_right.process(right * fb);
    sanitize(old_right);

    left = out_left, right = out_right;
}

/// Distortion Module by Tom Szilagyi
///
/// This module provides a blendable saturation stage
///////////////////////////////////////////////////////////////////////////////////////////////

tap_distortion::tap_distortion()
{
    is_active = false;
    srate = 0;
    meter = 0.f;
    rdrive = rbdr = kpa = kpb = kna = knb = ap = an = imr = kc = srct = sq = pwrq = prev_med = prev_out = 0.f;
    drive_old = blend_old = -1.f;
    over = 1;
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
    over = srate * 2 > 96000 ? 1 : 2;
    resampler.set_params(srate, over, 2);
}

float tap_distortion::process(float in)
{
    double *samples = resampler.upsample((double)in);
    meter = 0.f;
    for (int o = 0; o < over; o++) {
        float proc = samples[o];
        float med;
        if (proc >= 0.0f) {
            med = (D(ap + proc * (kpa - proc)) + kpb) * pwrq;
        } else {
            med = (D(an - proc * (kna + proc)) + knb) * pwrq * -1.0f;
        }
        proc = srct * (med - prev_med + prev_out);
        prev_med = M(med);
        prev_out = M(proc);
        samples[o] = proc;
        meter = std::max(meter, proc);
    }
    float out = (float)resampler.downsample(samples);
    return out;
}

float tap_distortion::get_distortion_level()
{
    return meter;
}

////////////////////////////////////////////////////////////////////////////////

simple_lfo::simple_lfo()
{
    is_active       = false;
    phase = 0.f;
    pwidth = 1.f;
}

void simple_lfo::activate()
{
    is_active = true;
    phase = 0.f;
}

void simple_lfo::deactivate()
{
    is_active = false;
}

float simple_lfo::get_value()
{
    return get_value_from_phase(phase);
}

float simple_lfo::get_value_from_phase(float ph) const
{
    float val = 0.f;
    float phs = std::min(100.f, ph / std::min(1.99f, std::max(0.01f, pwidth)) + offset);
    if (phs > 1)
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
    return val * amount;
}

void simple_lfo::advance(uint32_t count)
{
    //this function walks from 0.f to 1.f and starts all over again
    set_phase(phase + count * freq * (1.0 / srate));
}

void simple_lfo::set_phase(float ph)
{
    //set the phase from outsinde
    phase = fabs(ph);
    if (phase >= 1.0)
        phase = fmod(phase, 1.f);
}

void simple_lfo::set_params(float f, int m, float o, uint32_t sr, float a, float p)
{
    // freq: a value in Hz
    // mode: sine=0, triangle=1, square=2, saw_up=3, saw_down=4
    // offset: value between 0.f and 1.f to offset the lfo in time
    freq   = f;
    mode   = m;
    offset = o;
    srate  = sr;
    amount = a;
    pwidth = p;
}
void simple_lfo::set_freq(float f)
{
    freq = f;
}
void simple_lfo::set_mode(int m)
{
    mode = m;
}
void simple_lfo::set_offset(float o)
{
    offset = o;
}
void simple_lfo::set_amount(float a)
{
    amount = a;
}
void simple_lfo::set_pwidth(float p)
{
    pwidth = p;
}
bool simple_lfo::get_graph(float *data, int points, cairo_iface *context, int *mode) const
{
    if (!is_active)
        return false;
    for (int i = 0; i < points; i++) {
        float ph = (float)i / (float)points;
        data[i] = get_value_from_phase(ph);
    }
    return true;
}

bool simple_lfo::get_dot(float &x, float &y, int &size, cairo_iface *context) const
{
    if (!is_active)
        return false;
    float phs = phase + offset;
    if (phs >= 1.0)
        phs = fmod(phs, 1.f);
    x = phase;
    y = get_value_from_phase(phase);
    return true;
}


/// Lookahead Limiter by Christian Holschuh and Markus Schmidt

lookahead_limiter::lookahead_limiter() {
    is_active = false;
    channels = 2;
    id = 0;
    buffer_size = 0;
    overall_buffer_size = 0;
    att = 1.f;
    att_max = 1.0;
    pos = 0;
    delta = 0.f;
    _delta = 0.f;
    peak = 0.f;
    over_s = 0;
    over_c = 1.f;
    attack = 0.005;
    use_multi = false;
    weight = 1.f;
    _sanitize = false;
    auto_release = false;
    asc_active = false;
    nextiter = 0;
    nextlen = 0;
    asc = 0.f;
    asc_c = 0;
    asc_pos = -1;
    asc_changed = false;
    asc_coeff = 1.f;
    buffer = NULL;
    nextpos = NULL;
    nextdelta = NULL;
}
lookahead_limiter::~lookahead_limiter()
{
    free(buffer);
    free(nextpos);
    free(nextdelta);
}

void lookahead_limiter::activate()
{
    is_active = true;
    pos = 0;

}

void lookahead_limiter::set_multi(bool set) { use_multi = set; }

void lookahead_limiter::deactivate()
{
    is_active = false;
}

float lookahead_limiter::get_attenuation()
{
    float a = att_max;
    att_max = 1.0;
    return a;
}

void lookahead_limiter::set_sample_rate(uint32_t sr)
{
    srate = sr;
    
    free(buffer);
    free(nextpos);
    free(nextdelta);
    
    // rebuild buffer
    overall_buffer_size = (int)(srate * (100.f / 1000.f) * channels) + channels; // buffer size attack rate multiplied by 2 channels
    buffer = (float*) calloc(overall_buffer_size, sizeof(float));
    pos = 0;

    nextdelta = (float*) calloc(overall_buffer_size, sizeof(float));
    nextpos = (int*) malloc(overall_buffer_size * sizeof(int));
    memset(nextpos, -1, overall_buffer_size * sizeof(int));
    
    reset();
}

void lookahead_limiter::set_params(float l, float a, float r, float w, bool ar, float arc, bool d)
{
    limit = l;
    attack = a / 1000.f;
    release = r / 1000.f;
    auto_release = ar;
    asc_coeff = arc;
    debug = d;
    weight = w;
}

void lookahead_limiter::reset() {
    int bs = (int)(srate * attack * channels);
    buffer_size = bs - bs % channels; // buffer size attack rate
    _sanitize = true;
    pos = 0;
    nextpos[0] = -1;
    nextlen = 0;
    nextiter = 0;
    delta = 0.f;
    att = 1.f;
    reset_asc();
}

void lookahead_limiter::reset_asc() {
    asc = 0.f;
    asc_c = 0;
    asc_pos = pos;
    asc_changed = true;
}

float lookahead_limiter::get_rdelta(float peak, float _limit, float _att, bool _asc) {
    
    // calc the att for average input to walk to if we use asc (att of average signal)
    float _a_att = (limit * weight) / (asc_coeff * asc) * (float)asc_c;

    // calc a release delta from this attenuation
    float _rdelta = (1.0 - _att) / (srate * release);
    if(_asc && auto_release && asc_c > 0 && _a_att > _att) {
        // check if releasing to average level of peaks is steeper than
        // releasing to 1.f
        float _delta = std::max((_a_att - _att) / (srate * release), _rdelta / 10);
        if(_delta < _rdelta) {
            asc_active = true;
            _asc_used = true;
            _rdelta = _delta;
        }
    }
    return _rdelta;
}

void lookahead_limiter::process(float &left, float &right, float * multi_buffer)
{
    // PROTIP: harming paying customers enough to make them develop a competing
    // product may be considered an example of a less than sound business practice.

    // fill lookahead buffer
    if(_sanitize) {
        // if we're sanitizing (zeroing) the buffer on attack time change,
        // don't write the samples to the buffer
        buffer[pos] = 0.f;
        buffer[pos + 1] = 0.f;
    } else {
        buffer[pos] = left;
        buffer[pos + 1] = right;
    }
    
    // are we using multiband? get the multiband coefficient or use 1.f
    float multi_coeff = (use_multi) ? multi_buffer[pos] : 1.f;
    
    // calc the real limit including weight and multi coeff
    float _limit = limit * multi_coeff * weight;
    
    // input peak - impact higher in left or right channel?
    peak = fabs(left) > fabs(right) ? fabs(left) : fabs(right);

    // add an eventually appearing peak to the asc fake buffer if asc active
    if(auto_release && peak > _limit) {
        asc += peak;
        asc_c ++;
    }

    if(peak > _limit || multi_coeff < 1.0) {
        float _multi_coeff = 1.f;
        float _peak;
        
        // calc the attenuation needed to reduce incoming peak
        float _att = std::min(_limit / peak, 1.f);
        // calc release without any asc to keep all relevant peaks
        float _rdelta = get_rdelta(peak, _limit, _att, false);

        // calc the delta for walking to incoming peak attenuation
        float _delta = (_limit / peak - att) / buffer_size * channels;

        if(_delta < delta) {
            // is the delta more important than the actual one?
            // if so, we can forget about all stored deltas (because they can't
            // be more important - we already checked that earlier) and use this
            // delta now. and we have to create a release delta in nextpos buffer
            nextpos[0] = pos;
            nextpos[1] = -1;
            nextdelta[0] = _rdelta;
            nextlen = 1;
            nextiter = 0;
            delta = _delta;
        } else {
            // we have a peak on input its delta is less important than the
            // actual delta. But what about the stored deltas we're following?
            bool _found = false;
            int i = 0;
            for(i = nextiter; i < nextiter + nextlen; i++) {
                // walk through our nextpos buffer
                int j = i % buffer_size;
                // calculate a delta for the next stored peak
                // are we using multiband? then get the multi_coeff for the
                // stored position
                _multi_coeff = (use_multi) ? multi_buffer[nextpos[j]] : 1.f;
                // is the left or the right channel on this position more
                // important?
                _peak = fabs(buffer[nextpos[j]]) > fabs(buffer[nextpos[j] + 1]) ? fabs(buffer[nextpos[j]]) : fabs(buffer[nextpos[j] + 1]);
                // calc a delta to use to reach our incoming peak from the
                // stored position
                _delta = (_limit / peak - (limit * _multi_coeff * weight) / _peak) / (((buffer_size - nextpos[j] + pos) % buffer_size) / channels);
                if(_delta < nextdelta[j]) {
                    // if the buffered delta is more important than the delta
                    // used to reach our peak from the stored position, store
                    // the new delta at that position and stop the loop
                    nextdelta[j] = _delta;
                    _found = true;
                    break;
                }
            }
            if(_found) {
                // there was something more important in the next-buffer.
                // throw away any position and delta after the important
                // position and add a new release delta
                nextlen = i - nextiter + 1;
                nextpos[(nextiter + nextlen) % buffer_size] = pos;
                nextdelta[(nextiter + nextlen) % buffer_size] = _rdelta;
                // set the next following position value to -1 (cleaning up the
                // nextpos buffer)
                nextpos[(nextiter + nextlen + 1) % buffer_size] = -1;
                // and raise the length of our nextpos buffer for keeping the
                // release value
                nextlen ++;
            }
        }
    }

    // switch left and right pointers in buffer to output position
    left = buffer[(pos + channels) % buffer_size];
    right = buffer[(pos + channels + 1) % buffer_size];

    // if a peak leaves the buffer, remove it from asc fake buffer
    // but only if we're not sanitizing asc buffer
    float _peak = fabs(left) > fabs(right) ? fabs(left) : fabs(right);
    float _multi_coeff = (use_multi) ? multi_buffer[(pos + channels) % buffer_size] : 1.f;
    if(pos == asc_pos && !asc_changed) {
        asc_pos = -1;
    }
    if(auto_release && asc_pos == -1 && _peak > (limit * weight * _multi_coeff)) {
        asc -= _peak;
        asc_c --;
    }

    // change the attenuation level
    att += delta;

    // ...and calculate outpout from it
    left *= att;
    right *= att;
    
    if((pos + channels) % buffer_size == nextpos[nextiter]) {
        // if we reach a buffered position, change the actual delta and erase
        // this (the first) element from nextpos and nextdelta buffer
        if(auto_release) {
            // set delta to asc influenced release delta
            delta = get_rdelta(_peak, (limit * weight * _multi_coeff), att);
            if(nextlen > 1) {
                // if there are more positions to walk to, calc delta to next
                // position in buffer and compare it to release delta (keep
                // changes between peaks below asc steepness)
                int _nextpos = nextpos[(nextiter + 1) % buffer_size];
                float __peak = fabs(buffer[_nextpos]) > fabs(buffer[_nextpos + 1]) ? fabs(buffer[_nextpos]) : fabs(buffer[_nextpos + 1]);
                float __multi_coeff = (use_multi) ? multi_buffer[_nextpos] : 1.f;
                float __delta = ((limit * __multi_coeff * weight) / __peak - att) / (((buffer_size + _nextpos - ((pos + channels) % buffer_size)) % buffer_size) / channels);
                if(__delta < delta) {
                    delta = __delta;
                }
            }
        } else {
            // if no asc set delta from nextdelta buffer and fix the attenuation
            delta = nextdelta[nextiter];
            att = (limit * weight * _multi_coeff) / _peak;
        }
        // remove first element from circular nextpos buffer
        nextlen -= 1;
        nextpos[nextiter] = -1;
        nextiter = (nextiter + 1) % buffer_size;
    }

    if (att > 1.0f) {
        // release time seems over, reset attenuation and delta
        att = 1.0f;
        delta = 0.0f;
        nextiter = 0;
        nextlen = 0;
        nextpos[0] = -1;
    }

    // main limiting party is over, let's cleanup the puke

    if(_sanitize) {
        // we're sanitizing? then send 0.f as output
        left = 0.f;
        right = 0.f;
    }

    // security personnel pawing your values
    if(att <= 0.f) {
        // if this happens we're doomed!!
        // may happen on manually lowering attack
        att = 0.0000000000001;
        delta = (1.0f - att) / (srate * release);
    }

    if(att != 1.f && 1 - att < 0.0000000000001) {
        // denormalize att
        att = 1.f;
    }

    if(delta != 0.f && fabs(delta) < 0.00000000000001) {
        // denormalize delta
        delta = 0.f;
    }

    // post treatment (denormal, limit)
    denormal(&left);
    denormal(&right);

    // store max attenuation for meter output
    att_max = (att < att_max) ? att : att_max;

    // step forward in our sample ring buffer
    pos = (pos + channels) % buffer_size;

    // sanitizing is always done after a full cycle through the lookahead buffer
    if(_sanitize && pos == 0) _sanitize = false;

    asc_changed = false;
}

bool lookahead_limiter::get_asc() {
    if(!asc_active) return false;
    asc_active = false;
    return true;
}


////////////////////////////////////////////////////////////////////////////////

transients::transients() {
    envelope        = 0.f;
    attack          = 0.f;
    release         = 0.f;
    attack_coef     = 0.f;
    release_coef    = 0.f;
    att_time        = 0.f;
    att_level       = 0.f;
    rel_time        = 0.f;
    rel_level       = 0.f;
    sust_thres      = 1.f;
    maxdelta        = 0.f;
    new_return      = 1.f;
    old_return      = 1.f;
    lookahead       = 0;
    lookpos         = 0;
    channels        = 1;
    sustain_ended   = false;
    srand(1);
}
transients::~transients()
{
    free(lookbuf);
}
void transients::set_channels(int ch) {
    channels = ch;
    lookbuf = (float*) calloc(looksize * channels, sizeof(float));
    lookpos = 0;
}
void transients::set_sample_rate(uint32_t sr) {
    srate = sr;
    attack_coef  = exp(log(0.01) / (0.001 * srate));
    release_coef = exp(log(0.01) / (0.2f  * srate));
    // due to new calculation in attack, we sometimes get harsh
    // gain reduction/boost. 
    // to prevent "clicks" a maxdelta is set, which allows the signal
    // to raise/fall ~6dB/ms. 
    maxdelta = pow(4, 1.f / (0.001 * srate));
    calc_relfac();
}
void transients::set_params(float att_t, float att_l, float rel_t, float rel_l, float sust_th, int look) {
    lookahead  = look;
    sust_thres = sust_th;
    att_time   = att_t;
    rel_time   = rel_t;
    att_level  = att_l > 0 ? 0.25f * pow(att_l * 8, 2)
                          : -0.25f * pow(att_l * 4, 2);
    rel_level  = rel_l > 0 ? 0.5f  * pow(rel_l * 8, 2)
                          : -0.25f * pow(rel_l * 4, 2);
    calc_relfac();
}
void transients::calc_relfac()
{
    relfac = pow(0.5f, 1.f / (0.001 * rel_time * srate));
}
void transients::process(float *in, float s) {
    s = fabs(s) + 1e-10f * ((float)rand() / (float)RAND_MAX);
    // fill lookahead buffer
    for (int i = 0; i < channels; i++) {
        lookbuf[lookpos + i] = in[i];
    }
    
    // envelope follower
    // this is the real envelope follower curve. It raises as
    // fast as the signal is raising and falls much slower
    // depending on the sample rate and the ffactor
    // (the falling factor)
    if(s > envelope)
        envelope = attack_coef * (envelope - s) + s;
    else
        envelope = release_coef * (envelope - s) + s;
    
    // attack follower
    // this is a curve which follows the envelope slowly.
    // It never can rise above the envelope. It reaches 70.7%
    // of the envelope in a certain amount of time set by the user
    
    double attdelta = (envelope - attack)
                   * 0.707
                   / (srate * att_time * 0.001);
    if (sustain_ended == true && envelope / attack - 1 > 0.2f)
        sustain_ended = false;
    attack += attdelta;
    
    // never raise above envelope
    attack = std::min(envelope, attack);
    
    // release follower
    // this is a curve which is always above the envelope. It
    // starts to fall when the envelope falls beneath the
    // sustain threshold
        
    if ((envelope / release) - sust_thres < 0 && sustain_ended == false) 
        sustain_ended = true; 
    double reldelta = sustain_ended ? relfac : 1; 
                   
    // release delta can never raise above 0
    release *= reldelta;
    
    // never fall below envelope
    release = std::max(envelope, release);
    
    // difference between attack and envelope
    double attdiff = attack > 0 ? log(envelope / attack) : 0;
    
    // difference between release and envelope
    double reldiff = envelope > 0 ? log(release / envelope) : 0;
    
    // amplification factor from attack and release curve
    double ampfactor = attdiff * att_level + reldiff * rel_level;
    old_return = new_return;
    new_return = 1 + (ampfactor < 0 ? std::max(-1 + 1e-15, exp(ampfactor) - 1) : ampfactor);
    if (new_return / old_return > maxdelta) 
        new_return = old_return * maxdelta;
    else if (new_return / old_return < 1 / maxdelta)
        new_return = old_return / maxdelta;
        
    int pos = (lookpos + looksize * channels - lookahead * channels) % (looksize * channels);
    
    for (int i = 0; i < channels; i++)
        in[i] = lookbuf[pos + i] * new_return;
    
    // advance lookpos
    lookpos = (lookpos + channels) % (looksize * channels);
}


//////////////////////////////////////////////////////////////////

crossover::crossover() {
    bands     = -1;
    mode      = -1;
    redraw_graph = 1;
}
void crossover::set_sample_rate(uint32_t sr) {
    srate = sr;
}
void crossover::init(int c, int b, uint32_t sr) {
    channels = std::min(8, c);
    bands    = std::min(8, b);
    srate    = sr;
    for(int b = 0; b < bands; b ++) {
        // reset frequency settings
        freq[b]     = 1.0;
        active[b]   = true;
        level[b]    = 1.0;
        for (int c = 0; c < channels; c ++) {
            // reset outputs
            out[c][b] = 0.f;
        }
    }
}
float crossover::set_filter(int b, float f, bool force) {
    // keep between neighbour bands
    if (b)
        f = std::max((float)freq[b-1] * 1.1f, f);
    if (b < bands - 2)
        f = std::min((float)freq[b+1] * 0.9f, f);
    // restrict to 10-20k
    f = std::max(10.f, std::min(20000.f, f));
    // nothing changed? return
    if (freq[b] == f && !force)
        return freq[b];
    freq[b] = f;
    float q;
    switch (mode) {
        case 0:
        default:
            q = 0.5;
            break;
        case 1:
            q = 0.7071068123730965;
            break;
        case 2:
            q = 0.54;
            break;
    }
    for (int c = 0; c < channels; c ++) {
        if (!c) {
            lp[c][b][0].set_lp_rbj(freq[b], q, (float)srate);
            hp[c][b][0].set_hp_rbj(freq[b], q, (float)srate);
        } else {
            lp[c][b][0].copy_coeffs(lp[c-1][b][0]);
            hp[c][b][0].copy_coeffs(hp[c-1][b][0]);
        }
        if (mode > 1) {
            if (!c) {
                lp[c][b][1].set_lp_rbj(freq[b], 1.34, (float)srate);
                hp[c][b][1].set_hp_rbj(freq[b], 1.34, (float)srate);
            } else {
                lp[c][b][1].copy_coeffs(lp[c-1][b][1]);
                hp[c][b][1].copy_coeffs(hp[c-1][b][1]);
            }
            lp[c][b][2].copy_coeffs(lp[c][b][0]);
            hp[c][b][2].copy_coeffs(hp[c][b][0]);
            lp[c][b][3].copy_coeffs(lp[c][b][1]);
            hp[c][b][3].copy_coeffs(hp[c][b][1]);
        } else {
            lp[c][b][1].copy_coeffs(lp[c][b][0]);
            hp[c][b][1].copy_coeffs(hp[c][b][0]);
        }
    }
    redraw_graph = std::min(2, redraw_graph + 1);
    return freq[b];
}
void crossover::set_mode(int m) {
    if(mode == m)
        return;
    mode = m;
    for(int i = 0; i < bands - 1; i ++) {
        set_filter(i, freq[i], true);
    }
    redraw_graph = std::min(2, redraw_graph + 1);
}
void crossover::set_active(int b, bool a) {
    if (active[b] == a)
        return;
    active[b] = a;
    redraw_graph = std::min(2, redraw_graph + 1);
}
void crossover::set_level(int b, float l) {
    if (level[b] == l)
        return;
    level[b] = l;
    redraw_graph = std::min(2, redraw_graph + 1);
}
void crossover::process(float *data) {
    for (int c = 0; c < channels; c++) {
        for(int b = 0; b < bands; b ++) {
            out[c][b] = data[c];
            for (int f = 0; f < get_filter_count(); f++){
                if(b + 1 < bands) {
                    out[c][b] = lp[c][b][f].process(out[c][b]);
                    lp[c][b][f].sanitize();
                }
                if(b - 1 >= 0) {
                    out[c][b] = hp[c][b - 1][f].process(out[c][b]);
                    hp[c][b - 1][f].sanitize();
                }
            }
            out[c][b] *= level[b];
        }
    }
}
float crossover::get_value(int c, int b) {
    return out[c][b];
}
bool crossover::get_graph(int subindex, int phase, float *data, int points, cairo_iface *context, int *mode) const
{
    if (subindex >= bands) {
        redraw_graph = std::max(0, redraw_graph - 1);
        return false;
    }
    float ret;
    double freq;
    for (int i = 0; i < points; i++) {
        ret = 1.f;
        freq = 20.0 * pow (20000.0 / 20.0, i * 1.0 / points);
        for(int f = 0; f < get_filter_count(); f ++) {
            if(subindex < bands -1)
                ret *= lp[0][subindex][f].freq_gain(freq, (float)srate);
            if(subindex > 0)
                ret *= hp[0][subindex - 1][f].freq_gain(freq, (float)srate);
        }
        ret *= level[subindex];
        context->set_source_rgba(0.15, 0.2, 0.0, !active[subindex] ? 0.3 : 0.8);
        data[i] = dB_grid(ret);
    }
    return true;
}
bool crossover::get_layers(int index, int generation, unsigned int &layers) const
{
    layers = 0 | (redraw_graph || !generation ? LG_CACHE_GRAPH : 0) | (!generation ? LG_CACHE_GRID : 0);
    return redraw_graph || !generation;
}

int crossover::get_filter_count() const
{
    switch (mode) {
        case 0:
        default:
            return 1;
        case 1:
            return 2;
        case 2:
            return 4;
    }
}

//////////////////////////////////////////////////////////////////

bitreduction::bitreduction()
{
    coeff        = 1;
    morph        = 0;
    mode         = 0;
    dc           = 0;
    sqr          = 0;
    aa           = 0;
    aa1          = 0;
    redraw_graph = true;
    bypass       = true;
}
void bitreduction::set_sample_rate(uint32_t sr)
{
    srate = sr;
}
void bitreduction::set_params(float b, float mo, bool bp, uint32_t md, float d, float a)
{
    morph        = 1 - mo;
    bypass       = bp;
    dc           = d;
    aa           = a;
    mode         = md;
    coeff        = powf(2.0f, b) - 1;
    sqr          = sqrt(coeff / 2);
    aa1          = (1.f - aa) / 2.f;
    redraw_graph = true;
}
float bitreduction::add_dc(float s, float dc) const
{
    return s > 0 ? s *= dc : s /= dc;
}
float bitreduction::remove_dc(float s, float dc) const
{
    return s > 0 ? s /= dc : s *= dc;
}
float bitreduction::waveshape(float in) const
{
    double y;
    double k;
    
    // add dc
    in = add_dc(in, dc);
    
    // main rounding calculation depending on mode
    
    // the idea for anti-aliasing:
    // you need a function f which brings you to the scale, where you want to round
    // and the function f_b (with f(f_b)=id) which brings you back to your original scale.
    //
    // then you can use the logic below in the following way:
    // y = f(in) and k = roundf(y)
    // if (y > k + aa1)
    //      k = f_b(k) + ( f_b(k+1) - f_b(k) ) *0.5 * (sin(x - PI/2) + 1)
    // if (y < k + aa1)
    //      k = f_b(k) - ( f_b(k+1) - f_b(k) ) *0.5 * (sin(x - PI/2) + 1)
    //
    // whereas x = (fabs(f(in) - k) - aa1) * PI / aa 
    // for both cases.
    
    switch (mode) {
        case 0:
        default:
            // linear
            y = (in) * coeff;
            k = roundf(y);
            if (k - aa1 <= y && y <= k + aa1) {
                k /= coeff;
            } else if (y > k + aa1) {
                k = k / coeff + ((k + 1) / coeff - k / coeff) * 0.5 * (sin(M_PI * (fabs(y - k) - aa1) / aa - M_PI_2) + 1);
            } else {
                k = k / coeff - (k / coeff - (k - 1) / coeff) * 0.5 * (sin(M_PI * (fabs(y - k) - aa1) / aa - M_PI_2) + 1);
            }
            break;
        case 1:
            // logarithmic
            y = sqr * log(fabs(in)) + sqr * sqr;
            k = roundf(y);
            if(!in) {
                k = 0;
            } else if (k - aa1 <= y && y <= k + aa1) {
                k = in / fabs(in) * exp(k / sqr - sqr);
            } else if (y > k + aa1) {
                k = in / fabs(in) * (exp(k / sqr - sqr) + (exp((k + 1) / sqr - sqr) - exp(k / sqr - sqr)) * 0.5 * (sin((fabs(y - k) - aa1) / aa * M_PI - M_PI_2) + 1));
            } else {
                k = in / fabs(in) * (exp(k / sqr - sqr) - (exp(k / sqr - sqr) - exp((k - 1) / sqr - sqr)) * 0.5 * (sin((fabs(y - k) - aa1) / aa * M_PI - M_PI_2) + 1));
            }
            break;
    }
            
    // morph between dry and wet signal
    k += (in - k) * morph;
    
    // remove dc
    k = remove_dc(k, dc);
    
    return k;
}
float bitreduction::process(float in)
{
    return waveshape(in);
}

bool bitreduction::get_layers(int index, int generation, unsigned int &layers) const
{
    layers = redraw_graph || !generation ? LG_CACHE_GRAPH | LG_CACHE_GRID : 0;
    return redraw_graph || !generation;
}
bool bitreduction::get_graph(int subindex, int phase, float *data, int points, cairo_iface *context, int *mode) const
{
    if (subindex >= 2) {
        redraw_graph = false;
        return false;
    }
    for (int i = 0; i < points; i++) {
         data[i] = sin(((float)i / (float)points * 360.) * M_PI / 180.);
        if (subindex && !bypass)
            data[i] = waveshape(data[i]);
        else {
            context->set_line_width(1);
            context->set_source_rgba(0.15, 0.2, 0.0, 0.15);
        }
    }
    return true;
}
bool bitreduction::get_gridline(int subindex, int phase, float &pos, bool &vertical, std::string &legend, cairo_iface *context) const
{
    if (phase || subindex)
        return false;
    pos = 0;
    vertical = false;
    return true;
}

//////////////////////////////////////////////////////////////////

resampleN::resampleN()
{
    factor  = 2;
    srate   = 0;
    filters = 2;
}
resampleN::~resampleN()
{
}
void resampleN::set_params(uint32_t sr, int fctr = 2, int fltrs = 2)
{
    srate   = std::max(2u, sr);
    factor  = std::min(16, std::max(1, fctr));
    filters = std::min(4, std::max(1, fltrs));
    // set all filters
    filter[0][0].set_lp_rbj(std::max(25000., (double)srate / 2), 0.8, (float)srate * factor);
    for (int i = 1; i < filters; i++) {
        filter[0][i].copy_coeffs(filter[0][0]);
        filter[1][i].copy_coeffs(filter[0][0]);
    }
}
double *resampleN::upsample(double sample)
{
    tmp[0] = sample;
    if (factor > 1) {
        for (int f = 0; f < filters; f++)
            tmp[0] = filter[0][f].process(sample);
        for (int i = 1; i < factor; i++) {
            tmp[i] = 0;
            for (int f = 0; f < filters; f++)
                tmp[i] = filter[0][f].process(sample);
        }
    }
    return tmp;
}
double resampleN::downsample(double *sample)
{
    if (factor > 1) {
        for(int i = 0; i < factor; i++) {
            for (int f = 0; f < filters; f++) {
                sample[i] = filter[1][f].process(sample[i]);
            }
        }
    }
    return sample[0];
}

//////////////////////////////////////////////////////////////////

samplereduction::samplereduction()
{
    target  = 0;
    real    = 0;
    samples = 0;
    last    = 0;
    amount  = 0;
    round   = 0;
}
void samplereduction::set_params(float am)
{
    amount  = am;
    round   = roundf(amount);
    //samples = round;
}
double samplereduction::process(double in)
{
    samples ++;
    if (samples >= round) {
        target += amount;
        real += round;
        if (target + amount >= real + 1) {
            last = in;
            target = 0;
            real   = 0;
        }
        samples = 0;
    }
    return last;
}
