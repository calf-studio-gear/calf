/* Calf DSP Library
 * Drawbar organ emulator. 
 *
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
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __CALF_ORGAN_H
#define __CALF_ORGAN_H

#include "synth.h"

namespace synth 
{

struct organ_parameters {
    float drawbars[9];
    float foldover;
    float percussion_mode;
    float harmonic;
    float vibrato_mode;
    float master;

    inline bool get_foldover() { return foldover >= 0.5f; }
    inline int get_percussion_mode() { return dsp::fastf2i_drm(percussion_mode); }
    inline int get_harmonic() { return dsp::fastf2i_drm(harmonic); }
    inline int get_vibrato_mode() { return dsp::fastf2i_drm(vibrato_mode); }
};

class organ_voice_base
{
protected:
    dsp::sine_table<float, 4096, 1> sine_wave;
    dsp::fixed_point<int, 20> phase, dphase;
    int note;
    dsp::decay<double> amp;

    inline float sine(dsp::fixed_point<int, 20> ph) {
        return ph.lerp_table_lookup_float(sine_wave.data);
    }
};

class organ_voice: public synth::voice, public organ_voice_base {
protected:    
    bool released;
    int h4, h6, h8, h12, h16;
    dsp::fixed_point<int, 3> h10;
    organ_parameters *parameters;

public:
    organ_voice(organ_parameters *_parameters)
    : parameters(_parameters) {
    }

    void reset() {
        phase = 0;
    }

    void note_on(int note, int /*vel*/) {
        reset();
        this->note = note;
        dphase.set(synth::midi_note_to_phase(note, 0, sample_rate));
        amp.set(1.0f);
        released = false;
        calc_foldover();
    }

    void note_off(int /* vel */) {
        released = true;
    }

    void calc_foldover() {
        h4 = 4, h6 = 6, h8 = 8, h10 = 10, h12 = 12, h16 = 16;
        if (!parameters->get_foldover()) return;
        const int foc = 108, foc2 = 108 + 12, foc3 = 108 + 24;
        if (note + 24 >= foc) h4 = 2;
        if (note + 24 >= foc2) h4 = 1;
        if (note + 36 >= foc) h8 = 4;
        if (note + 36 >= foc2) h8 = 2;
        if (note + 40 >= foc) h10 = 5.0;
        if (note + 40 >= foc2) h10 = 2.5;
        if (note + 43 >= foc) h12 = 6;
        if (note + 43 >= foc2) h12 = 3;
        if (note + 48 >= foc) h16 = 8;
        if (note + 48 >= foc2) h16 = 4;
        if (note + 48 >= foc3) h16 = 2;
//        printf("h= %d %d %d %f %d %d\n", h4, h6, h8, (float)h10, h12, h16);
    }

    void render_to(float *buf[1], int nsamples) {
        if (note == -1)
            return;

        if (!amp.get_active())
            dsp::zero(buf[0], nsamples);
        else {
            for (int i=0; i<nsamples; i++) {
//                float osc = 0.2*sine(phase)+0.1*sine(4*phase)+0.08*sine(12*phase)+0.08*sine(16*phase);
                float *drawbars = parameters->drawbars;
                float osc = 
                    drawbars[0]*sine(phase)+
                    drawbars[1]*sine(3*phase)+
                    drawbars[2]*sine(2*phase)+
                    drawbars[3]*sine(h4*phase)+
                    drawbars[4]*sine(h6*phase)+
                    drawbars[5]*sine(h8*phase)+
                    drawbars[6]*sine(h10*phase)+
                    drawbars[7]*sine(h12*phase)+
                    drawbars[8]*sine(h16*phase);
                buf[0][i] += osc*amp.get();
                if (released) amp.age_lin((1.0/44100.0)/0.03,0.0);
                phase += dphase;
            }
        }
    }
    virtual int get_current_note() {
        return note;
    }
    virtual bool get_active() {
        return (note != -1) && amp.get_active();
    }
};

/// Not a true voice, just something with similar-ish interface.
class percussion_voice: public organ_voice_base {
public:
    organ_parameters *parameters;
    int sample_rate;

    percussion_voice(organ_parameters *_parameters)
    {
        parameters = _parameters;
        note = -1;
    }
    void reset() {
        phase = 0;
        note = -1;
    }

    void note_on(int note, int /*vel*/) {
        reset();
        this->note = note;
        dphase.set(synth::midi_note_to_phase(note, 0, sample_rate));
        amp.set(1.0f);
    }

    // this doesn't really have a voice interface
    void render_to(float *buf, int nsamples) {
        if (note == -1)
            return;

        if (!amp.get_active())
            return;
        int mode = parameters->get_percussion_mode();
        if (!mode)
            return;
        int harmonic = 2 * parameters->get_harmonic();
        // XXXKF the decay needs work!
        float age_const = mode == 1 ? 0.001f : 0.0003f;
        for (int i = 0; i < nsamples; i++) {
            float osc = 0.5 * sine(harmonic * phase);
            buf[i] += osc * amp.get();
            amp.age_exp(age_const, 0.0001f);
            phase += dphase;
        }
    }
    bool get_active() {
        return (note != -1) && amp.get_active();
    }
    void setup(int sr) {
        sample_rate = sr;
    }
};

struct drawbar_organ: public synth::basic_synth {
    organ_parameters *parameters;
    percussion_voice percussion;
    // chorus instead of rotary speaker is, well, cheesy
    // let me think of something better some day
    dsp::simple_flanger<float, 4096> chorus, chorus2;
    dsp::biquad<float> crossover1, crossover2;
    dsp::simple_delay<8, float> phaseshift;
    float mwhl_value, hold_value;
    
    drawbar_organ(organ_parameters *_parameters)
    : parameters(_parameters)
    , percussion(_parameters) {
        mwhl_value = hold_value = 0.f;
    }
    void render_to(float *output[], int nsamples)
    {
        float buf[4096], *bufptr[] = { buf };
        dsp::zero(buf, nsamples);
        basic_synth::render_to(bufptr, nsamples);
        if (percussion.get_active())
            percussion.render_to(buf, nsamples);
        float gain = parameters->master;
        if (parameters->get_vibrato_mode())
        {
            float chorus_buffer[4096];
            float chorus_buffer2[4096];
            chorus.process(chorus_buffer, buf, nsamples);
            chorus2.process(chorus_buffer2, buf, nsamples);
            for (int i=0; i<nsamples; i++) {
                float lower_drum = crossover1.process_d2(chorus_buffer[i]);
                float upper_drum = crossover2.process_d2(chorus_buffer2[i]);
                output[0][i] = gain*(buf[i] + lower_drum - upper_drum);
                output[1][i] = gain*(buf[i] - phaseshift.process(lower_drum - upper_drum, 7));
            }
        }
        else
        for (int i=0; i<nsamples; i++) {
            output[0][i] = gain*buf[i];
            output[1][i] = gain*buf[i];
        }
    }
    synth::voice *alloc_voice() {
        return new organ_voice(parameters);
    }
    virtual void first_note_on(int note, int vel) {
        percussion.note_on(note, vel);
    }
    virtual void setup(int sr) {
        basic_synth::setup(sr);
        crossover1.set_lp_rbj(800.f, 0.7, (float)sr);
        crossover2.set_hp_rbj(800.f, 0.7, (float)sr);
        set_vibrato();
        chorus.setup(sr);chorus2.setup(sr);
        chorus.set_min_delay(0.0041f);chorus2.set_min_delay(0.0025f);
        chorus.set_mod_depth(0.0024f);chorus2.set_mod_depth(0.0034f);

        chorus.set_rate(0.63f);chorus2.set_rate(0.63f);
        chorus.set_wet(0.5f);chorus2.set_wet(0.5f);
        chorus.set_dry(0.0f);chorus2.set_dry(0.0f);
        percussion.setup(sr);
        mwhl_value = hold_value = 0.f;
    }
    virtual void control_change(int ctl, int val)
    {
        int mode = parameters->get_vibrato_mode();
        if (mode == 3 && ctl == 64)
        {
            hold_value = val / 127.f;
            set_vibrato();
            return;
        }
        if (mode == 4 && ctl == 1)
        {
            mwhl_value = val / 127.f;
            set_vibrato();
            return;
        }
        synth::basic_synth::control_change(ctl, val);
    }
    void set_vibrato()
    {
        int mode = parameters->get_vibrato_mode();
        if (!mode)
            return;
        float speed = mode - 1;
        if (mode == 3)
            speed = hold_value;
        if (mode == 4)
            speed = mwhl_value;
        chorus.set_mod_depth(0.002f - 0.0015f*speed);
        chorus2.set_mod_depth(0.0025f - 0.002f*speed);
        chorus.set_min_delay(0.0061f);
        chorus2.set_min_delay(0.0085f);
        chorus.set_rate((40.0 + (342 - 40)*speed) / 60.0);
        chorus2.set_rate((48.0 + (400 - 48)*speed) / 60.0);
        chorus.set_fb(0.3f);
        chorus2.set_fb(-0.3f);
    }
};

};

#endif
