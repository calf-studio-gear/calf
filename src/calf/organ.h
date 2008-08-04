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

#define ORGAN_KEYTRACK_POINTS 4

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
        float attack, decay, sustain, release, velscale, ampctl;
    };
        
    //////////////////////////////////////////////////////////////////////////
    // these parameters are binary-copied from control ports (order is important!)
    
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
    float percussion_wave;
    float percussion_harmonic;
    float percussion_vel2amp;
    float percussion_fm_time;
    float percussion_fm_depth;
    float percussion_fm_wave;
    float percussion_fm_harmonic;
    float percussion_vel2fm;
    float percussion_trigger;
    float percussion_stereo;
    float filter_chain;
    float master;

    organ_filter_parameters filters[organ_parameters::FilterCount];
    organ_env_parameters envs[organ_parameters::EnvCount];
    float lfo_rate;
    float lfo_amt;
    float lfo_wet;
    float lfo_phase;
    float lfo_mode;
    
    float global_transpose;
    float global_detune;
    
    //////////////////////////////////////////////////////////////////////////
    // these parameters are calculated
    
    double perc_decay_const, perc_fm_decay_const;
    float multiplier[9];
    int phaseshift[9];
    float cutoff;
    unsigned int foldvalue;
    float pitch_bend;

    float percussion_keytrack[ORGAN_KEYTRACK_POINTS][2];
    
    organ_parameters() : pitch_bend(1.0f) {}

    inline int get_percussion_wave() { return dsp::fastf2i_drm(percussion_wave); }
    inline int get_percussion_fm_wave() { return dsp::fastf2i_drm(percussion_fm_wave); }
};

#define ORGAN_WAVE_BITS 12
#define ORGAN_WAVE_SIZE 4096
#define ORGAN_BIG_WAVE_BITS 17
#define ORGAN_BIG_WAVE_SIZE 131072
/// 2^ORGAN_BIG_WAVE_SHIFT = how many (quasi)periods per sample
#define ORGAN_BIG_WAVE_SHIFT 5

class organ_voice_base
{
public:
    enum organ_waveform { 
        wave_sine, 
        wave_sinepl1, wave_sinepl2, wave_sinepl3,
        wave_ssaw, wave_ssqr, wave_spls, wave_saw, wave_sqr, wave_pulse, wave_sinepl05, wave_sqr05, wave_halfsin, wave_clvg, wave_bell, wave_bell2,
        wave_w1, wave_w2, wave_w3, wave_w4, wave_w5, wave_w6, wave_w7, wave_w8, wave_w9,
        wave_dsaw, wave_dsqr, wave_dpls,
        wave_count_small,
        wave_strings = wave_count_small,
        wave_strings2,
        wave_sinepad,
        wave_bellpad,
        wave_space,
        wave_choir,
        wave_choir2,
        wave_choir3,
        wave_count,
        wave_count_big = wave_count - wave_count_small
    };
    enum {
        ampctl_none,
        ampctl_direct,
        ampctl_f1,
        ampctl_f2,
        ampctl_all,
        ampctl_count
    };
    enum { 
        lfomode_off = 0,
        lfomode_direct,
        lfomode_filter1,
        lfomode_filter2,
        lfomode_voice,
        lfomode_global,
        lfomode_count
    };
    enum {
        perctrig_first = 0,
        perctrig_each,
        perctrig_eachplus,
        perctrig_polyphonic,
        perctrig_count
    };
    typedef waveform_family<ORGAN_WAVE_BITS> small_wave_family;
    typedef waveform_family<ORGAN_BIG_WAVE_BITS> big_wave_family;
public:
    organ_parameters *parameters;
protected:
    static small_wave_family (*waves)[wave_count_small];
    static big_wave_family (*big_waves)[wave_count_big];

    // dsp::sine_table<float, ORGAN_WAVE_SIZE, 1> sine_wave;
    int note;
    dsp::decay amp;
    /// percussion FM carrier amplitude envelope
    dsp::decay pamp;
    /// percussion FM modulator amplitude envelope
    dsp::decay fm_amp;
    dsp::fixed_point<int64_t, 20> pphase, dpphase;
    dsp::fixed_point<int64_t, 20> modphase, moddphase;
    float fm_keytrack;
    int &sample_rate_ref;

    organ_voice_base(organ_parameters *_parameters, int &_sample_rate_ref);
    
    inline float wave(float *data, dsp::fixed_point<int, 20> ph) {
        return ph.lerp_table_lookup_float(data);
    }
    inline float big_wave(float *data, dsp::fixed_point<int64_t, 20> &ph) {
        // wrap to fit within the wave
        return ph.lerp_table_lookup_float_mask(data, ORGAN_BIG_WAVE_SIZE - 1);
    }
public:
    static inline small_wave_family &get_wave(int wave) {
        return (*waves)[wave];
    }
    static inline big_wave_family &get_big_wave(int wave) {
        return (*big_waves)[wave];
    }
    static void precalculate_waves();
    void update_pitch()
    {
        float phase = synth::midi_note_to_phase(note, 100 * parameters->global_transpose + parameters->global_detune, sample_rate_ref);
        dpphase.set(phase * parameters->percussion_harmonic * parameters->pitch_bend);
        moddphase.set(phase * parameters->percussion_fm_harmonic * parameters->pitch_bend);
    }
    // this doesn't really have a voice interface
    void render_percussion_to(float (*buf)[2], int nsamples);
    void perc_note_on(int note, int vel);
    void perc_reset()
    {
        pphase = 0;
        modphase = 0;
        dpphase = 0;
        moddphase = 0;
        note = -1;
    }
};

class organ_vibrato
{
protected:
    enum { VibratoSize = 6 };
    float vibrato_x1[VibratoSize][2], vibrato_y1[VibratoSize][2];
    float lfo_phase;
    onepole<float> vibrato[2];
public:
    void reset();
    void process(organ_parameters *parameters, float (*data)[2], unsigned int len, float sample_rate);
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
    inertia<linear_ramp> expression;
    organ_vibrato vibrato;
    float velocity;

public:
    organ_voice()
    : organ_voice_base(NULL, sample_rate),
    expression(linear_ramp(16)) {
    }

    void reset() {
        vibrato.reset();
        phase = 0;
        for (int i = 0; i < FilterCount; i++)
        {
            filterL[i].reset();
            filterR[i].reset();
        }
    }

    void note_on(int note, int vel) {
        reset();
        this->note = note;
        const float sf = 0.001f;
        for (int i = 0; i < EnvCount; i++)
        {
            organ_parameters::organ_env_parameters &p = parameters->envs[i];
            envs[i].set(sf * p.attack, sf * p.decay, p.sustain, sf * p.release, sample_rate / BlockSize);
            envs[i].note_on();
        }
        update_pitch();
        velocity = vel * 1.0 / 127.0;
        amp.set(1.0f);
        released = false;
        perc_note_on(note, vel);
    }

    void note_off(int /* vel */) {
        for (int i = 0; i < EnvCount; i++)
            envs[i].note_off();
    }

    void render_block();
    
    virtual int get_current_note() {
        return note;
    }
    virtual bool get_active() {
        return (note != -1) && (amp.get_active() || (use_percussion() && pamp.get_active()));
    }
    void update_pitch();
    inline bool use_percussion()
    {
        return dsp::fastf2i_drm(parameters->percussion_trigger) == perctrig_polyphonic;
    }
};

/// Not a true voice, just something with similar-ish interface.
class percussion_voice: public organ_voice_base {
public:
    int sample_rate;

    percussion_voice(organ_parameters *_parameters)
    : organ_voice_base(_parameters, sample_rate)
    {
    }
    
    bool get_active() {
        return (note != -1) && pamp.get_active();
    }
    bool get_noticable() {
        return (note != -1) && (pamp.get() > 0.2 * parameters->percussion_level);
    }
    void setup(int sr) {
        sample_rate = sr;
    }
};

struct drawbar_organ: public synth::basic_synth {
    organ_parameters *parameters;
    percussion_voice percussion;
    organ_vibrato global_vibrato;
    
    enum { 
        par_drawbar1, par_drawbar2, par_drawbar3, par_drawbar4, par_drawbar5, par_drawbar6, par_drawbar7, par_drawbar8, par_drawbar9, 
        par_frequency1, par_frequency2, par_frequency3, par_frequency4, par_frequency5, par_frequency6, par_frequency7, par_frequency8, par_frequency9, 
        par_waveform1, par_waveform2, par_waveform3, par_waveform4, par_waveform5, par_waveform6, par_waveform7, par_waveform8, par_waveform9, 
        par_detune1, par_detune2, par_detune3, par_detune4, par_detune5, par_detune6, par_detune7, par_detune8, par_detune9, 
        par_phase1, par_phase2, par_phase3, par_phase4, par_phase5, par_phase6, par_phase7, par_phase8, par_phase9, 
        par_pan1, par_pan2, par_pan3, par_pan4, par_pan5, par_pan6, par_pan7, par_pan8, par_pan9, 
        par_routing1, par_routing2, par_routing3, par_routing4, par_routing5, par_routing6, par_routing7, par_routing8, par_routing9, 
        par_foldover,
        par_percdecay, par_perclevel, par_percwave, par_percharm, par_percvel2amp,
        par_percfmdecay, par_percfmdepth, par_percfmwave, par_percfmharm, par_percvel2fm,
        par_perctrigger, par_percstereo,
        par_filterchain,
        par_master, 
        par_f1cutoff, par_f1res, par_f1env1, par_f1env2, par_f1env3, par_f1keyf,
        par_f2cutoff, par_f2res, par_f2env1, par_f2env2, par_f2env3, par_f2keyf,
        par_eg1attack, par_eg1decay, par_eg1sustain, par_eg1release, par_eg1velscl, par_eg1ampctl, 
        par_eg2attack, par_eg2decay, par_eg2sustain, par_eg2release, par_eg2velscl, par_eg2ampctl, 
        par_eg3attack, par_eg3decay, par_eg3sustain, par_eg3release, par_eg3velscl, par_eg3ampctl, 
        par_lforate, par_lfoamt, par_lfowet, par_lfophase, par_lfomode,
        par_transpose, par_detune,
        param_count
    };

     drawbar_organ(organ_parameters *_parameters)
    : parameters(_parameters)
    , percussion(_parameters) {
        organ_voice_base::precalculate_waves();
    }
    void render_separate(float *output[], int nsamples)
    {
        float buf[4096][2];
        dsp::zero(&buf[0][0], 2 * nsamples);
        basic_synth::render_to(buf, nsamples);
        if (fastf2i_drm(parameters->lfo_mode) == organ_voice_base::lfomode_global)
        {
            for (int i = 0; i < nsamples; i += 64)
                global_vibrato.process(parameters, buf + i, min(64, nsamples - i), sample_rate);
        }
        if (percussion.get_active())
            percussion.render_percussion_to(buf, nsamples);
        float gain = parameters->master * (1.0 / (9 * 8));
        for (int i=0; i<nsamples; i++) {
            output[0][i] = gain*buf[i][0];
            output[1][i] = gain*buf[i][1];
        }
    }
    synth::voice *alloc_voice() {
        block_voice<organ_voice> *v = new block_voice<organ_voice>();
        v->parameters = parameters;
        return v;
    }
    virtual void percussion_note_on(int note, int vel) {
        percussion.perc_note_on(note, vel);
    }
    virtual void params_changed() = 0;
    virtual void setup(int sr) {
        basic_synth::setup(sr);
        percussion.setup(sr);
        parameters->cutoff = 0;
        params_changed();
        global_vibrato.reset();
    }
    void update_params();
    void control_change(int controller, int value)
    {
        if (controller == 11)
        {
            parameters->cutoff = value / 64.0 - 1;
        }
        synth::basic_synth::control_change(controller, value);
    }
    void pitch_bend(int amt);
    virtual bool check_percussion() { 
        switch(dsp::fastf2i_drm(parameters->percussion_trigger))
        {        
            case organ_voice_base::perctrig_first:
                return active_voices.empty();
            case organ_voice_base::perctrig_each: 
            default:
                return true;
            case organ_voice_base::perctrig_eachplus:
                return !percussion.get_noticable();
            case organ_voice_base::perctrig_polyphonic:
                return false;
        }
    }
};

};

#endif
