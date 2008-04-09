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
    enum { FilterCount = 2, EnvCount = 3 };
    struct organ_filter_parameters
    {
        float cutoff;
        float resonance;
        float envmod[organ_parameters::EnvCount];
        float keyf;
    };

    struct organ_env_parameters
    {
        float attack, decay, sustain, release;
    };
        
    float drawbars[9];
    float harmonics[9];
    float waveforms[9];
    float detune[9];
    float phase[9];
    float pan[9];
    float routing[9];
    float foldover;
    float percussion_time;
    float percussion_level;
    float percussion_harmonic;
    float master;
    
    organ_filter_parameters filters[organ_parameters::FilterCount];
    organ_env_parameters envs[organ_parameters::EnvCount];
    
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
public:
    enum { 
        wave_sine, 
        wave_sinepl1, wave_sinepl2, wave_sinepl3,
        wave_ssaw, wave_ssqr, wave_spls, wave_saw, wave_sqr, wave_pulse, wave_sinepl05, wave_sqr05, wave_halfsin, wave_clvg, wave_bell, wave_bell2,
        wave_w1, wave_w2, wave_w3, wave_w4, wave_w5, wave_w6, wave_w7, wave_w8, wave_w9,
        wave_count };
protected:
    static waveform_family<ORGAN_WAVE_BITS> waves[wave_count];
    // dsp::sine_table<float, ORGAN_WAVE_SIZE, 1> sine_wave;
    int note;
    dsp::decay amp;

    organ_voice_base(organ_parameters *_parameters);
    
    inline float wave(float *data, dsp::fixed_point<int, 20> ph) {
        return ph.lerp_table_lookup_float(data);
    }
public:
    organ_parameters *parameters;
};

class organ_voice: public synth::voice, public organ_voice_base {
protected:    
    enum { Channels = 2, BlockSize = 64, EnvCount = organ_parameters::EnvCount, FilterCount = organ_parameters::FilterCount };
    union {
        float output_buffer[BlockSize][Channels];
        float aux_buffers[3][BlockSize][Channels];
    };
    bool released;
    dsp::fixed_point<int64_t, 52> phase, dphase;
    biquad<float> filterL[2], filterR[2];
    adsr envs[EnvCount];
    int age;

public:
    organ_voice()
    : organ_voice_base(NULL) {
    }

    void reset() {
        phase = 0;
        for (int i = 0; i < FilterCount; i++)
        {
            filterL[i].reset();
            filterR[i].reset();
        }
        age = 0;
    }

    void note_on(int note, int /*vel*/) {
        reset();
        this->note = note;
        const float sf = 0.001f;
        for (int i = 0; i < EnvCount; i++)
        {
            organ_parameters::organ_env_parameters &p = parameters->envs[i];
            envs[i].set(sf * p.attack, sf * p.decay, p.sustain, sf * p.release, sample_rate / BlockSize);
            envs[i].note_on();
        }
        dphase.set(synth::midi_note_to_phase(note, 0, sample_rate));
        amp.set(1.0f);
        released = false;
    }

    void note_off(int /* vel */) {
        released = true;
    }

    void render_block() {
        if (note == -1)
            return;

        dsp::zero(&output_buffer[0][0], Channels * BlockSize);
        dsp::zero(&aux_buffers[1][0][0], 2 * Channels * BlockSize);
        if (!amp.get_active())
            return;
        
        dsp::fixed_point<int, 20> tphase, tdphase;
        for (int h = 0; h < 9; h++)
        {
            float amp = parameters->drawbars[h];
            if (amp < small_value<float>())
                continue;
            float *data;
            dsp::fixed_point<int, 24> hm = dsp::fixed_point<int, 24>(parameters->multiplier[h] * (1.0 / 2.0));
            int waveid = (int)parameters->waveforms[h];
            if (waveid < 0 || waveid >= wave_count)
                waveid = 0;
            for (int i = 0; i < 4; i++)
            {
                data = waves[waveid].get_level((dphase * hm).get() >> i);
                if (data)
                    break;
            }
            if (!data)
                continue;
            tphase.set((uint32_t)((phase * hm).get()) + parameters->phaseshift[h]);
            tdphase.set((uint32_t)(dphase * hm).get());
            float ampl = amp * 0.5f * (-1 + parameters->pan[h]);
            float ampr = amp * 0.5f * (1 + parameters->pan[h]);
            float (*out)[Channels] = aux_buffers[dsp::fastf2i_drm(parameters->routing[h])];
            for (int i=0; i < (int)BlockSize; i++) {
                float wv = wave(data, tphase);
                out[i][0] += wv * ampl;
                out[i][1] += wv * ampr;
                tphase += tdphase;
            }
        }
        phase += dphase * BlockSize;
        for (int i = 0; i < FilterCount; i++)
        {
            float mod = parameters->filters[i].envmod[0] * envs[0].value;
            mod += parameters->filters[i].keyf * 100 * (note - 60);
            for (int j = 1; j < EnvCount; j++)
            {
                mod += parameters->filters[i].envmod[j] * envs[j].value;
            }
            float fc = parameters->filters[i].cutoff * pow(2.0f, mod * (1.f / 1200.f));
            filterL[i].set_lp_rbj(dsp::clip<float>(fc, 10, 18000), parameters->filters[i].resonance, sample_rate);
            filterR[i].copy_coeffs(filterL[i]);
        }
        for (int i = 0; i < EnvCount; i++)
            envs[i].advance();
        age++;
        for (int i=0; i < (int) BlockSize; i++) {
            output_buffer[i][0] += filterL[0].process_d1(aux_buffers[1][i][0]) + filterL[1].process_d1(aux_buffers[2][i][0]);
            output_buffer[i][1] += filterR[0].process_d1(aux_buffers[1][i][1]) + filterR[1].process_d1(aux_buffers[2][i][1]);
        }
        if (released)
        {
            for (int i=0; i < (int) BlockSize; i++) {
                output_buffer[i][0] *= amp.get();
                output_buffer[i][1] *= amp.get();
                amp.age_lin((1.0/44100.0)/0.03,0.0);
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
    dsp::fixed_point<int64_t, 20> phase, dphase;

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
    void render_to(float *buf[2], int nsamples) {
        if (note == -1)
            return;

        if (!amp.get_active())
            return;
        if (parameters->percussion_level < small_value<float>())
            return;
        int percussion_harmonic = 2 * parameters->get_percussion_harmonic();
        float level = parameters->percussion_level * 9;
        // XXXKF the decay needs work!
        double age_const = parameters->perc_decay_const;
        float *data = waves[wave_sine].begin()->second;
        for (int i = 0; i < nsamples; i++) {
            float osc = level * wave(data, percussion_harmonic * phase);
            osc *= level * amp.get();
            buf[0][i] += osc;
            buf[1][i] += osc;
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
        float buf[2][4096], *bufptr[] = { buf[0], buf[1] };
        dsp::zero(buf[0], nsamples);
        dsp::zero(buf[1], nsamples);
        basic_synth::render_to(bufptr, nsamples);
        if (percussion.get_active())
            percussion.render_to(bufptr, nsamples);
        float gain = parameters->master * (1.0 / (9 * 8));
        for (int i=0; i<nsamples; i++) {
            output[0][i] = gain*buf[0][i];
            output[1][i] = gain*buf[1][i];
        }
    }
    synth::voice *alloc_voice() {
        block_voice<organ_voice> *v = new block_voice<organ_voice>();
        v->parameters = parameters;
        return v;
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
