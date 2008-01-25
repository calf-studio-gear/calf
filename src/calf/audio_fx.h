/* Calf DSP Library
 * Reusable audio effect classes.
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
#ifndef __CALF_AUDIOFX_H
#define __CALF_AUDIOFX_H

#include "primitives.h"
#include "delay.h"
#include "fixed_point.h"

namespace dsp {
#if 0
}; to keep editor happy
#endif

/**
 * Audio effect base class. Not really useful until it gets more developed.
 */
class audio_effect
{
public:
    virtual void setup(int sample_rate)=0;
    virtual ~audio_effect() {}
};

/**
 * Base class for chorus and flanger. Wouldn't be needed if it wasn't
 * for odd behaviour of GCC when deriving templates from template
 * base classes (not seeing fields from base classes!).
 */
class chorus_base: public audio_effect
{
protected:
    int sample_rate, min_delay_samples, mod_depth_samples;
    float rate, wet, dry, min_delay, mod_depth, odsr;
    fixed_point<unsigned int, 20> phase, dphase;
    sine_table<int, 4096, 65536> sine;
public:
    float get_rate() {
        return rate;
    }
    void set_rate(float rate) {
        this->rate = rate;
        dphase = rate/sample_rate*4096;        
    }
    float get_wet() {
        return wet;
    }
    void set_wet(float wet) {
        this->wet = wet;
    }
    float get_dry() {
        return dry;
    }
    void set_dry(float dry) {
        this->dry = dry;
    }
    float get_min_delay() {
        return min_delay;
    }
    void set_min_delay(float min_delay) {
        this->min_delay = min_delay;
        this->min_delay_samples = (int)(min_delay * 65536.0 * sample_rate);
    }
    float get_mod_depth() {
        return mod_depth;
    }
    void set_mod_depth(float mod_depth) {
        this->mod_depth = mod_depth;
        // 128 because it's then multiplied by (hopefully) a value of 32768..-32767
        this->mod_depth_samples = (int)(mod_depth * 32.0 * sample_rate);
    }
};

/**
 * Single-tap chorus without feedback.
 * Perhaps MaxDelay should be a bit longer!
 */
template<class T, int MaxDelay=512>
class simple_chorus: public chorus_base
{
protected:
    simple_delay<MaxDelay,T> delay;
public:    
    simple_chorus() {
        rate = 0.63f;
        dry = 0.5f;
        wet = 0.5f;
        min_delay = 0.005f;
        mod_depth = 0.0025f;
        setup(44100);
    }
    void reset() {
        delay.reset();
    }
    virtual void setup(int sample_rate) {
        this->sample_rate = sample_rate;
        this->odsr = 1.0 / sample_rate;
        delay.reset();
        phase = 0;
        set_rate(get_rate());
        set_min_delay(get_min_delay());
        set_mod_depth(get_mod_depth());
    }
    template<class OutIter, class InIter>
    void process(OutIter buf_out, InIter buf_in, int nsamples) {
        int mds = min_delay_samples + mod_depth_samples * 1024 + 2*65536;
        int mdepth = mod_depth_samples;
        for (int i=0; i<nsamples; i++) {
            phase += dphase;
            unsigned int ipart = phase.ipart();
            
            float in = *buf_in++;
            int lfo = phase.lerp_by_fract_int<int, 14, int>(sine.data[ipart], sine.data[ipart+1]);
            int v = mds + (mdepth * lfo >> 6);
            // if (!(i & 7)) printf("%d\n", v);
            int ifv = v >> 16;
            delay.put(in);
            T fd; // signal from delay's output
            delay.get_interp(fd, ifv, (v & 0xFFFF)*(1.0/65536.0));
            T sdry = in * dry;
            T swet = fd * wet;
            *buf_out++ = sdry + swet;
        }
    }
};

/**
 * Single-tap flanger (chorus plus feedback).
 */
template<class T, int MaxDelay=1024>
class simple_flanger: public chorus_base
{
protected:
    simple_delay<MaxDelay,T> delay;
    float fb;
    int last_delay_pos, last_actual_delay_pos;
    int ramp_pos, ramp_delay_pos;
public:
    simple_flanger()
    : fb(0) {}
    void reset() {
        delay.reset();
        last_delay_pos = 0;
        ramp_pos = 1024;
    }
    virtual void setup(int sample_rate) {
        this->sample_rate = sample_rate;
        this->odsr = 1.0 / sample_rate;
        delay.reset();
        phase = 0;
        set_rate(get_rate());
        set_min_delay(get_min_delay());
    }
    float get_fb() {
        return fb;
    }
    void set_fb(float fb) {
        this->fb = fb;
    }
    template<class OutIter, class InIter>
    void process(OutIter buf_out, InIter buf_in, int nsamples) {
        if (!nsamples)
            return;
        int mds = this->min_delay_samples + this->mod_depth_samples * 1024 + 2 * 65536;
        int mdepth = this->mod_depth_samples;
        int delay_pos;
        unsigned int ipart = this->phase.ipart();
        int lfo = phase.lerp_by_fract_int<int, 14, int>(this->sine.data[ipart], this->sine.data[ipart+1]);
        delay_pos = mds + (mdepth * lfo >> 6);
        
        if (delay_pos != last_delay_pos || ramp_pos < 1024)
        {
            if (delay_pos != last_delay_pos) {
                // we need to ramp from what the delay tap length actually was, 
                // not from old (ramp_delay_pos) or desired (delay_pos) tap length
                ramp_delay_pos = last_actual_delay_pos;
                ramp_pos = 0;
            }
            
            int64_t dp;
            for (int i=0; i<nsamples; i++) {
                float in = *buf_in++;
                T fd; // signal from delay's output
                dp = (((int64_t)ramp_delay_pos) * (1024 - ramp_pos) + ((int64_t)delay_pos) * ramp_pos) >> 10;
                ramp_pos++;
                if (ramp_pos > 1024) ramp_pos = 1024;
                this->delay.get_interp(fd, dp >> 16, (dp & 0xFFFF)*(1.0/65536.0));
                sanitize(fd);
                T sdry = in * this->dry;
                T swet = fd * this->wet;
                *buf_out++ = sdry + swet;
                this->delay.put(in+fb*fd);

                this->phase += this->dphase;
                ipart = this->phase.ipart();
                lfo = phase.lerp_by_fract_int<int, 14, int>(this->sine.data[ipart], this->sine.data[ipart+1]);
                delay_pos = mds + (mdepth * lfo >> 6);
            }
            last_actual_delay_pos = dp;
        }
        else {
            for (int i=0; i<nsamples; i++) {
                float in = *buf_in++;
                T fd; // signal from delay's output
                this->delay.get_interp(fd, delay_pos >> 16, (delay_pos & 0xFFFF)*(1.0/65536.0));
                sanitize(fd);
                T sdry = in * this->dry;
                T swet = fd * this->wet;
                *buf_out++ = sdry + swet;
                this->delay.put(in+fb*fd);

                this->phase += this->dphase;
                ipart = this->phase.ipart();
                lfo = phase.lerp_by_fract_int<int, 14, int>(this->sine.data[ipart], this->sine.data[ipart+1]);
                delay_pos = mds + (mdepth * lfo >> 6);
            }
            last_actual_delay_pos = delay_pos;
        }
        last_delay_pos = delay_pos;
    }
};

#if ENABLE_EXPERIMENTAL

/**
 * This is crap, not really sounding like rotary speaker.
 * I needed it for tests, maybe I'll give it more time later.
 */
template<class T>
class rotary_speaker
{
    simple_delay<256,T> delay;
    fixed_point<unsigned int, 20> phase, dphase;
    T last;
public:    
    rotary_speaker() {
        reset();
    }
    void reset() {
        delay.reset();
        dphase = 6.3/44100*4096;
        phase = 0;
        last = 0;
    }
    template<class InT>
    void process(T *out, InT *in, int nsamples) {
        for (int i=0; i<nsamples; i++) {
            phase += dphase;

            double sig = sin(double(phase)*2*M_PI/4096);
            double v = 100+30*sig;
            double fv = floor(v);
            delay.Put(in[i]);
            T fd, fd2, fd3; // signal from delay's output
            int ifv = (int)fv;
            delay.GetInterp(fd, ifv, v-fv);
            delay.GetInterp(fd2, ifv+3, v-fv);
            delay.GetInterp(fd3, ifv+4, v-fv);
            out[i] = 0.25f * (in[i] + fd + (last - in[i])*(0.5+0.25*sig));
            last = last * 0.9f + dsp_mono(fd2 * 0.25f + fd3 * 0.25f);
        }
    }
};

#endif

/**
 * A classic allpass loop reverb with modulated allpass filter.
 * Just started implementing it, so there is no control over many
 * parameters.
 */
template<class T>
class reverb: public audio_effect
{
    simple_delay<2048, T> apL1, apL2, apL3, apL4, apL5, apL6;
    simple_delay<2048, T> apR1, apR2, apR3, apR4, apR5, apR6;
    fixed_point<unsigned int, 25> phase, dphase;
    sine_table<int, 128, 10000> sine;
    onepole<T> lp_left, lp_right;
    T old_left, old_right;
    int type;
    float time, fb, cutoff, diffusion;
    int tl[6], tr[6];
    float ldec[6], rdec[6];
    
    int sr;
public:
    reverb()
    {
        phase = 0.0;
        time = 1.0;
        cutoff = 9000;
        type = 2;
        diffusion = 1.f;
        setup(44100);
    }
    virtual void setup(int sample_rate) {
        sr = sample_rate;
        set_time(time);
        set_cutoff(cutoff);
        phase = 0.0;
        dphase = 0.5*128/sr;
        update_times();
    }
    void update_times()
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
        }
        
        float fDec=1000 + 1400.f * diffusion;
        for (int i = 0 ; i < 6; i++)
            ldec[i]=exp(-float(tl[i] >> 16) / fDec), 
            rdec[i]=exp(-float(tr[i] >> 16) / fDec);
    }
    float get_time() {
        return time;
    }
    void set_time(float time) {
        this->time = time;
        // fb = pow(1.0f/4096.0f, (float)(1700/(time*sr)));
        fb = 1.0 - 0.3 / (time * sr / 44100.0);
    }
    float get_type() {
        return type;
    }
    void set_type(int type) {
        this->type = type;
        update_times();
    }
    float get_diffusion() {
        return diffusion;
    }
    void set_diffusion(float diffusion) {
        this->diffusion = diffusion;
        update_times();
    }
    void set_type_and_diffusion(int type, float diffusion) {
        this->type = type;
        this->diffusion = diffusion;
        update_times();
    }
    float get_fb()
    {
        return this->fb;
    }
    void set_fb(float fb)
    {
        this->fb = fb;
    }
    float get_cutoff() {
        return cutoff;
    }
    void set_cutoff(float cutoff) {
        this->cutoff = cutoff;
        lp_left.set_lp(cutoff,sr);
        lp_right.set_lp(cutoff,sr);
    }
    void reset()
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
    void process(T &left, T &right)
    {
        unsigned int ipart = phase.ipart();
        
        // the interpolated LFO might be an overkill here
        int lfo = phase.lerp_by_fract_int<int, 14, int>(sine.data[ipart], sine.data[ipart+1]);
        /*
        static int tmp = 0;
        if ((tmp++) % 10 == 0)
            printf("lfo=%d\n", lfo);
            */
        phase += dphase;
        
        left += old_right;
        left = apL1.process_allpass_comb_lerp16(left, tl[0] - 45*lfo, ldec[0]);
        left = apL2.process_allpass_comb_lerp16(left, tl[1] + 47*lfo, ldec[1]);
        float out_left = left;
        left = apL3.process_allpass_comb_lerp16(left, tl[2] + 54*lfo, ldec[2]);
        left = apL4.process_allpass_comb_lerp16(left, tl[3] - 69*lfo, ldec[3]);
        left = apL5.process_allpass_comb_lerp16(left, tl[4] - 69*lfo, ldec[4]);
        left = apL6.process_allpass_comb_lerp16(left, tl[5] - 46*lfo, ldec[5]);
        old_left = lp_left.process(left * fb);
        sanitize(old_left);

        right += old_left;
        right = apR1.process_allpass_comb_lerp16(right, tr[0] - 45*lfo, rdec[0]);
        right = apR2.process_allpass_comb_lerp16(right, tr[1] + 47*lfo, rdec[1]);
        float out_right = right;
        right = apR3.process_allpass_comb_lerp16(right, tr[2] + 54*lfo, rdec[2]);
        right = apR4.process_allpass_comb_lerp16(right, tr[3] - 69*lfo, rdec[3]);
        right = apR5.process_allpass_comb_lerp16(right, tr[4] - 69*lfo, rdec[4]);
        right = apR6.process_allpass_comb_lerp16(right, tr[5] - 46*lfo, rdec[5]);
        old_right = lp_right.process(right * fb);
        sanitize(old_right);
        
        left = out_left, right = out_right;
    }
    void extra_sanitize()
    {
        lp_left.sanitize();
        lp_right.sanitize();
    }
};

#if 0
{ to keep editor happy
#endif
}

#endif
