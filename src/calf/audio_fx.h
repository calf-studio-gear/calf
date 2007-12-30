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
        this->mod_depth_samples = (int)(mod_depth * 128.0 * sample_rate);
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
        int mds = min_delay_samples + mod_depth_samples * 256 + 2*65536;
        int mdepth = mod_depth_samples;
        for (int i=0; i<nsamples; i++) {
            phase += dphase;
            unsigned int ipart = phase.ipart();
            
            float in = *buf_in++;
            int lfo = phase.lerp_by_fract_int<int, 14, int>(sine.data[ipart], sine.data[ipart+1]);
            int v = mds + (mdepth * lfo >> 8);
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
template<class T, int MaxDelay=256>
class simple_flanger: public chorus_base
{
protected:
    simple_delay<MaxDelay,T> delay;
    float fb;
public:
    simple_flanger()
    : fb(0) {}
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
    }
    float get_fb() {
        return fb;
    }
    void set_fb(float fb) {
        this->fb = fb;
    }
    template<class OutIter, class InIter>
    void process(OutIter buf_out, InIter buf_in, int nsamples) {
        int mds = this->min_delay_samples + this->mod_depth_samples * 256 + 2 * 65536;
        int mdepth = this->mod_depth_samples;
        for (int i=0; i<nsamples; i++) {
            this->phase += this->dphase;
            unsigned int ipart = this->phase.ipart();
            
            float in = *buf_in++;
            int lfo = phase.lerp_by_fract_int<int, 14, int>(this->sine.data[ipart], this->sine.data[ipart+1]);
            int v = mds + (mdepth * lfo >> 8);
            // if (!(i & 7)) printf("%d\n", v);
            int ifv = v >> 16;
            T fd; // signal from delay's output
            this->delay.get_interp(fd, ifv, (v & 0xFFFF)*(1.0/65536.0));
            sanitize(fd);
            T sdry = in * this->dry;
            T swet = fd * this->wet;
            *buf_out++ = sdry + swet;
            this->delay.put(in+fb*fd);
        }
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
    float time, fb, cutoff;
    int sr;
public:
    reverb()
    {
        phase = 0.0;
        time = 1.0;
        cutoff = 9000;
        setup(44100);
    }
    virtual void setup(int sample_rate) {
        sr = sample_rate;
        set_time(time);
        set_cutoff(cutoff);
        phase = 0.0;
        dphase = 0.5*128/sr;
    }
    float get_time() {
        return time;
    }
    void set_time(float time) {
        this->time = time;
        // fb = pow(1.0f/4096.0f, (float)(1700/(time*sr)));
        fb = 1.0 - 0.3 / (time * sr / 44100.0);
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
        const int tl1 =  697 << 16, tr1 =  783 << 16;
        const int tl2 =  957 << 16, tr2 =  929 << 16;
        const int tl3 =  649 << 16, tr3 =  531 << 16;
        const int tl4 = 1249 << 16, tr4 = 1377 << 16;
        const int tl5 = 1573 << 16, tr5 = 1671 << 16;
        const int tl6 = 1877 << 16, tr6 = 1781 << 16;
        static const float fDec=1700.f;
        static const float l1dec=exp(-697.f/fDec), r1dec=exp(-783.f/fDec);
        static const float l2dec=exp(-975.f/fDec), r2dec=exp(-929.f/fDec);
        static const float l3dec=exp(-649.f/fDec), r3dec=exp(-531.f/fDec);
        static const float l4dec=exp(-1249.f/fDec), r4dec=exp(-1377.f/fDec);
        static const float l5dec=exp(-1573.f/fDec), r5dec=exp(-1671.f/fDec);
        static const float l6dec=exp(-1877.f/fDec), r6dec=exp(-1781.f/fDec);
        
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
        left = apL1.process_allpass_comb_lerp16(left, tl1 - 45*lfo, l1dec);
        left = apL2.process_allpass_comb_lerp16(left, tl2 + 47*lfo, l2dec);
        float out_left = left;
        left = apL3.process_allpass_comb_lerp16(left, tl3 + 54*lfo, l3dec);
        left = apL4.process_allpass_comb_lerp16(left, tl4 - 69*lfo, l4dec);
        left = apL5.process_allpass_comb_lerp16(left, tl5 - 69*lfo, l5dec);
        left = apL6.process_allpass_comb_lerp16(left, tl6 - 46*lfo, l6dec);
        old_left = lp_left.process(left * fb);
        sanitize(old_left);

        right += old_left;
        right = apR1.process_allpass_comb_lerp16(right, tr1 - 45*lfo, r1dec);
        right = apR2.process_allpass_comb_lerp16(right, tr2 + 47*lfo, r2dec);
        float out_right = right;
        right = apR3.process_allpass_comb_lerp16(right, tr3 + 54*lfo, r3dec);
        right = apR4.process_allpass_comb_lerp16(right, tr4 - 69*lfo, r4dec);
        right = apR5.process_allpass_comb_lerp16(right, tr5 - 69*lfo, r5dec);
        right = apR6.process_allpass_comb_lerp16(right, tr6 - 46*lfo, r6dec);
        old_right = lp_right.process(right * fb);
        sanitize(old_right);
        
        left = out_left, right = out_right;
    }
};

#if 0
{ to keep editor happy
#endif
}

#endif
