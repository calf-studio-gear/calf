#ifndef __CALF_WAVETABLE_H
#define __CALF_WAVETABLE_H

#include <assert.h>
#include "biquad.h"
#include "onepole.h"
#include "audio_fx.h"
#include "inertia.h"
#include "osc.h"
#include "synth.h"
#include "envelope.h"

namespace calf_plugins {

#define WAVETABLE_WAVE_BITS 8

class wavetable_audio_module;
    
struct wavetable_oscillator: public dsp::simple_oscillator
{
    enum { SIZE = 1 << 8, MASK = SIZE - 1, SCALE = 1 << (32 - 8) };
    int16_t (*tables)[256];
    inline float get(uint8_t slice)
    {
        int16_t *waveform = tables[slice];
        float value = 0.f;
        uint32_t cphase = phase, cphasedelta = phasedelta >> 3;
        for (int j = 0; j < 8; j++)
        {
            uint32_t wpos = cphase >> (32 - 8);
            value += dsp::lerp((float)waveform[wpos], (float)waveform[(wpos + 1) & MASK], (cphase & (SCALE - 1)) * (1.0f / SCALE));
            cphase += cphasedelta;
        }
        value = value * (1.0 / 8.0) * (1.0 / 32768.0);
        phase += phasedelta;
        return value;
    }
};

class wavetable_voice: public dsp::voice
{
public:
    enum { Channels = 2, BlockSize = 64, EnvCount = 3, OscCount = 2 };
    float output_buffer[BlockSize][Channels];
protected:
    int note;
    wavetable_audio_module *parent;
    float **params;
    dsp::decay amp;
    wavetable_oscillator oscs[OscCount];
    dsp::adsr envs[EnvCount];
public:
    wavetable_voice();
    void set_params_ptr(wavetable_audio_module *_parent, int _srate);
    void reset();
    void note_on(int note, int vel);
    void note_off(int /* vel */);
    void steal();
    void render_block();
    virtual int get_current_note() {
        return note;
    }
    virtual bool get_active() {
        // printf("note %d getactive %d use_percussion %d pamp active %d\n", note, amp.get_active(), use_percussion(), pamp.get_active());
        return (note != -1) && (amp.get_active()) && !envs[0].stopped();
    }
};    

class wavetable_audio_module: public audio_module<wavetable_metadata>, public dsp::basic_synth
{
public:
    using dsp::basic_synth::note_on;
    using dsp::basic_synth::note_off;
    using dsp::basic_synth::control_change;
    using dsp::basic_synth::pitch_bend;

protected:
    uint32_t crate;
    bool panic_flag;

public:
    float *ins[in_count]; 
    float *outs[out_count];
    float *params[param_count];
    int16_t tables[128][256];

public:
    wavetable_audio_module();

    dsp::voice *alloc_voice() {
        dsp::block_voice<wavetable_voice> *v = new dsp::block_voice<wavetable_voice>();
        v->set_params_ptr(this, sample_rate);
        return v;
    }
    
    /// process function copied from Organ (will probably need some adjustments as well as implementing the panic flag elsewhere
    uint32_t process(uint32_t offset, uint32_t nsamples, uint32_t inputs_mask, uint32_t outputs_mask) {
        float *o[2] = { outs[0] + offset, outs[1] + offset };
        if (panic_flag)
        {
            control_change(120, 0); // stop all sounds
            control_change(121, 0); // reset all controllers
            panic_flag = false;
        }
        float buf[4096][2];
        dsp::zero(&buf[0][0], 2 * nsamples);
        basic_synth::render_to(buf, nsamples);
        float gain = 1.0f;
        for (uint32_t i=0; i<nsamples; i++) {
            o[0][i] = gain*buf[i][0];
            o[1][i] = gain*buf[i][1];
        }
        return 3;
    }

    void set_sample_rate(uint32_t sr) {
        setup(sr);
        crate = sample_rate / wavetable_voice::BlockSize;
    }
};

    
};

#endif
