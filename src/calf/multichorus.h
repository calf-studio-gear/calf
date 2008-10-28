/* Calf DSP Library
 * Multitap chorus class.
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
#ifndef __CALF_MULTICHORUS_H
#define __CALF_MULTICHORUS_H

#include "audio_fx.h"

namespace dsp {

typedef fixed_point<unsigned int, 20> chorus_phase;

template<class T, uint32_t Voices>
class sine_multi_lfo
{
protected:
    sine_table<int, 4096, 65536> sine;
    enum { Multiplier = (65536 / Voices) << 16 };
    
public:
    /// Current LFO phase
    chorus_phase phase;
    /// LFO phase increment
    chorus_phase dphase;
    /// LFO phase per-voice increment
    chorus_phase vphase;

public:
    sine_multi_lfo()
    {
        phase = dphase = vphase = 0.0;
    }
    inline uint32_t get_voices() const
    {
        return Voices;
    }
    /// Get LFO value for given voice, returns a values in range of [-65536, 65536]
    inline int get_value(uint32_t voice) {
        // find this voice's phase (= phase + voice * 360 degrees / number of voices)
        chorus_phase voice_phase = phase + vphase * (int)voice;
        // find table offset
        unsigned int ipart = voice_phase.ipart();
        // interpolate (use 14 bits of precision - because the table itself uses 18 bits and the result of multiplication must fit in int32_t)
        // note, the result is still -65536 .. 65536, it's just interpolated
        return voice_phase.lerp_by_fract_int<int, 14, int>(sine.data[ipart], sine.data[ipart+1]);
    }
    inline void step() {
        phase += dphase;
    }
    inline T get_scale() const {
        // use sqrt, because some phases will cancel each other - so 1 / N is usually too low
        return sqrt(1.0 / Voices);
    }
    void reset() {
        phase = 0.f;
    }
};

/**
 * Multi-tap chorus without feedback.
 * Perhaps MaxDelay should be a bit longer!
 */
template<class T, class MultiLfo, int MaxDelay=512>
class multichorus: public chorus_base
{
protected:
    simple_delay<MaxDelay,T> delay;
public:    
    MultiLfo lfo;
    multichorus() {
        rate = 0.63f;
        dry = 0.5f;
        wet = 0.5f;
        min_delay = 0.005f;
        mod_depth = 0.0025f;
        setup(44100);
    }
    void reset() {
        delay.reset();
        lfo.reset();
    }
    void set_rate(float rate) {
        chorus_base::set_rate(rate);
        lfo.dphase = dphase;
    }
    virtual void setup(int sample_rate) {
        modulation_effect::setup(sample_rate);
        delay.reset();
        lfo.reset();
        set_min_delay(get_min_delay());
        set_mod_depth(get_mod_depth());
    }
    template<class OutIter, class InIter>
    void process(OutIter buf_out, InIter buf_in, int nsamples) {
        int mds = min_delay_samples + mod_depth_samples * 1024 + 2*65536;
        int mdepth = mod_depth_samples; // 1 sample peak-to-peak = mdepth of 32 (this scaling stuff is tricky and may - but shouldn't - be wrong)
        T scale = lfo.get_scale();
        for (int i=0; i<nsamples; i++) {
            phase += dphase;
            
            float in = *buf_in++;
            
            delay.put(in);
            unsigned int nvoices = lfo.get_voices();
            T out = 0.f;
            // add up values from all voices, each voice tell its LFO phase and the buffer value is picked at that location
            for (unsigned int v = 0; v < nvoices; v++)
            {
                int lfo_output = lfo.get_value(v);
                int v = mds + (mdepth * lfo_output >> 6);
                int ifv = v >> 16;
                T fd; // signal from delay's output
                delay.get_interp(fd, ifv, (v & 0xFFFF)*(1.0/65536.0));
                out += fd;
            }
            T sdry = in * gs_dry.get();
            T swet = out * gs_wet.get() * scale;
            *buf_out++ = sdry + swet;
            lfo.step();
        }
    }
};

};

#endif


