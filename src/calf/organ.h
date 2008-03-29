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
    float harmonics[9];
    float waveforms[9];
    float detune[9];
    float phase[9];
    float foldover;
    float percussion_time;
    float percussion_level;
    float percussion_harmonic;
    float master;
    
    double perc_decay_const;
    float multiplier[9];
    int phaseshift[9];

    inline bool get_foldover() { return foldover >= 0.5f; }
    inline int get_percussion_harmonic() { return dsp::fastf2i_drm(percussion_harmonic); }
};

#define ORGAN_WAVE_BITS 12
#define ORGAN_WAVE_SIZE 4096

class organ_voice_base
{
protected:
    enum { wave_sine, wave_sinepl1, wave_sinepl2, wave_sinepl3, wave_ssaw, wave_ssqr, wave_spls, wave_saw, wave_sqr, wave_pulse, wave_count };
    static waveform_family<ORGAN_WAVE_BITS> waves[wave_count];
    // dsp::sine_table<float, ORGAN_WAVE_SIZE, 1> sine_wave;
    dsp::fixed_point<int64_t, 52> phase, dphase;
    int note;
    dsp::decay amp;
    organ_parameters *parameters;

    organ_voice_base(organ_parameters *_parameters);
    
    inline float wave(float *data, dsp::fixed_point<int, 20> ph) {
        return ph.lerp_table_lookup_float(data);
    }
};

class organ_voice: public synth::voice, public organ_voice_base {
protected:    
    bool released;
    int h4, h6, h8, h12, h16;
    dsp::fixed_point<int, 3> h10;

public:
    organ_voice(organ_parameters *_parameters)
    : organ_voice_base(_parameters) {
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
        const int foc = 120, foc2 = 120 + 12, foc3 = 120 + 24;
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
            float tmp[256], *sig;
            if (released)
            {
                sig = tmp;            
                for (int i=0; i<nsamples; i++)
                    tmp[i] = 0;
            }
            else
                sig = buf[0];
            dsp::fixed_point<int, 20> tphase, tdphase;
            for (int h = 0; h < 9; h++)
            {
                float *data;
                dsp::fixed_point<int, 24> hm = dsp::fixed_point<int, 24>(parameters->multiplier[h]);
                int waveid = (int)parameters->waveforms[h];
                if (waveid < 0 || waveid >= wave_count)
                    waveid = 0;
                do {
                    data = waves[waveid].get_level((dphase * hm).get());
                    if (data)
                        break;
                    hm.set(hm.get() >> 1);
                } while(1);
                tphase.set(((phase * hm).get() & 0xFFFFFFFF) + parameters->phaseshift[h]);
                tdphase.set((dphase * hm).get() & 0xFFFFFFFF);
                float amp = parameters->drawbars[h];
                for (int i=0; i<nsamples; i++) {
    //                float osc = 0.2*sine(phase)+0.1*sine(4*phase)+0.08*sine(12*phase)+0.08*sine(16*phase);
                    sig[i] += wave(data, tphase)*amp;
                    tphase += tdphase;
                }
            }
            phase += dphase * nsamples;
            if (released)
            {
                for (int i=0; i<nsamples; i++) {
                    buf[0][i] += tmp[i] * amp.get();
                    amp.age_lin((1.0/44100.0)/0.03,0.0);
                }
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
    int sample_rate;

    percussion_voice(organ_parameters *_parameters)
    : organ_voice_base(_parameters)
    {
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
        if (parameters->percussion_level < small_value<float>())
            return;
        int percussion_harmonic = 2 * parameters->get_percussion_harmonic();
        float level = parameters->percussion_level * (8 * 9);
        // XXXKF the decay needs work!
        double age_const = parameters->perc_decay_const;
        float *data = waves[wave_sine].begin()->second;
        for (int i = 0; i < nsamples; i++) {
            float osc = level * wave(data, percussion_harmonic * phase);
            buf[i] += osc * amp.get();
            amp.age_exp(age_const, 1.0 / 32768.0);
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
    
    drawbar_organ(organ_parameters *_parameters)
    : parameters(_parameters)
    , percussion(_parameters) {
    }
    void render_to(float *output[], int nsamples)
    {
        float buf[4096], *bufptr[] = { buf };
        dsp::zero(buf, nsamples);
        basic_synth::render_to(bufptr, nsamples);
        if (percussion.get_active())
            percussion.render_to(buf, nsamples);
        float gain = parameters->master * (1.0 / (9 * 8));
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
        percussion.setup(sr);
        update_params();
    }
    void update_params()
    {
        parameters->perc_decay_const = dsp::decay::calc_exp_constant(1.0 / 1024.0, 0.001 * parameters->percussion_time * sample_rate);
        for (int i = 0; i < 9; i++)
        {
            parameters->multiplier[i] = parameters->harmonics[i] * pow(2.0, parameters->detune[i] * (1.0 / 1200.0));
            parameters->phaseshift[i] = int(parameters->phase[i] * 65536 / 360) << 16;
        }
    }
};

};

#endif
