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

class wavetable_voice: public dsp::voice
{
public:
    enum { Channels = 2, BlockSize = 64, EnvCount = 3, OscCount = 1 };
    float output_buffer[BlockSize][Channels];
protected:
    int note;
    float **params;
    dsp::decay amp;
    dsp::simple_oscillator oscs[OscCount];
    dsp::adsr envs[EnvCount];
public:
    void set_params_ptr(float **_params) { params = _params; }
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
        return (note != -1) && (amp.get_active());
    }
};    

class wavetable_audio_module: public audio_module<wavetable_metadata>, public dsp::basic_synth
{
public:
    using dsp::basic_synth::note_on;
    using dsp::basic_synth::note_off;
    using dsp::basic_synth::control_change;
    using dsp::basic_synth::pitch_bend;
    uint32_t srate; // XXXKF hack!

protected:
    uint32_t crate;
    bool panic_flag;

public:
    float *ins[in_count]; 
    float *outs[out_count];
    float *params[param_count];

public:
    wavetable_audio_module()
    {
        panic_flag = false;
    }

    dsp::voice *alloc_voice() {
        dsp::block_voice<wavetable_voice> *v = new dsp::block_voice<wavetable_voice>();
        v->set_params_ptr(params);
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
};

    
};

#endif
