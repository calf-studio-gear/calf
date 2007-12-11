/* Calf DSP Library
 * Prototype audio modules
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
#ifndef __CALF_MODULES_DEV_H
#define __CALF_MODULES_DEV_H

#if ENABLE_EXPERIMENTAL

#include <assert.h>
#include "biquad.h"
#include "audio_fx.h"
#include "inertia.h"
#include "osc.h"
#include "organ.h"

namespace synth {

using namespace dsp;

/// Monosynth-in-making. Parameters may change at any point, so don't make songs with it!
/// It lacks inertia for parameters, even for those that really need it.
class monosynth_audio_module
{
public:
    enum { wave_saw, wave_sqr, wave_pulse, wave_sine, wave_triangle, wave_count };
    enum { par_wave1, par_wave2, par_detune, par_osc2xpose, par_oscmode, par_oscmix, par_cutoff, par_resonance, par_envmod, par_envtores, par_decay, par_keyfollow, par_legato, param_count };
    enum { in_count = 0, out_count = 2, support_midi = true, rt_capable = true };
    enum { step_size = 64 };
    static const char *param_names[];
    float *ins[in_count]; 
    float *outs[out_count];
    float *params[param_count];
    uint32_t srate, crate;
    waveform_family<11> waves[wave_count];
    waveform_oscillator<11> osc1, osc2;
    bool running, stopping, gate;
    int last_key;
    
    float buffer[step_size];
    uint32_t output_pos;
    biquad<float> filter;
    biquad<float> filter2;
    int wave1, wave2;
    float freq, cutoff, decay_factor;
    float detune, xpose, xfade, pitchbend;
    int voice_age;
    float odcr;
    int queue_note_on;
    bool legato;
    
    static parameter_properties param_props[];
    void set_sample_rate(uint32_t sr) {
        srate = sr;
        crate = sr / step_size;
        odcr = (float)(1.0 / crate);
    }
    void note_on()
    {
        freq = 440 * pow(2.0, (queue_note_on - 69) / 12.0);
        set_frequency();
        osc1.waveform = waves[wave1].get_level(osc1.phasedelta);
        osc2.waveform = waves[wave2].get_level(osc2.phasedelta);
        
        if (!running)
        {
            osc1.reset();
            osc2.reset();
            filter.reset();
            filter2.reset();
            switch((int)*params[par_oscmode])
            {
            case 1:
                osc2.phase = 0x80000000;
                break;
            case 2:
                osc2.phase = 0x40000000;
                break;
            case 3:
                osc1.phase = osc2.phase = 0x40000000;
                break;
            case 4:
                osc1.phase = 0x40000000;
                osc2.phase = 0xC0000000;
                break;
            case 5:
                // rand() is crap, but I don't have any better RNG in Calf yet
                osc1.phase = rand() << 16;
                osc2.phase = rand() << 16;
                break;
            default:
                break;
            }
            voice_age = 0;
            running = true;
        }
        gate = true;
        stopping = false;
        if (!legato)
            voice_age = 0;
        queue_note_on = -1;
    }
    void handle_event(uint8_t *data, int len) {
        switch(*data >> 4)
        {
        case 9:
            if (data[2]) {
                queue_note_on = data[1];
                last_key = data[1];
            }
            // printf("note on %d %d\n", data[1], data[2]);
            break;
        case 8:
            // printf("note off %d %d\n", data[1], data[2]);
            if (data[1] == last_key)
                gate = false;
            break;
        case 14:
            float value = data[1] + 128 *data[2] - 8192;
            pitchbend = pow(2.0, value / 8192.0);
            break;
        }
        default_handle_event(data, len, params, param_count);
    }
    void set_frequency()
    {
        osc1.set_freq(freq * (2 - detune) * pitchbend, srate);
        osc2.set_freq(freq * (detune)  * pitchbend * xpose, srate);
    }
    void params_changed() {
        decay_factor = odcr * 1000.0 / *params[par_decay];
        wave1 = dsp::clip(dsp::fastf2i_drm(*params[par_wave1]), 0, (int)wave_count - 1);
        wave2 = dsp::clip(dsp::fastf2i_drm(*params[par_wave2]), 0, (int)wave_count - 1);
        detune = pow(2.0, *params[par_detune] / 1200.0);
        xpose = pow(2.0, *params[par_osc2xpose] / 12.0);
        xfade = *params[par_oscmix];
        legato = *params[par_legato] >= 0.5f;
        set_frequency();
    }
    void activate() {
        running = false;
        output_pos = 0;
        queue_note_on = -1;
        pitchbend = 1.f;
        filter.reset();
        filter2.reset();
        float data[2048];
        bandlimiter<11> bl;

        // yes these waves don't have really perfect 1/x spectrum because of aliasing
        // (so what?)
        for (int i = 0 ; i < 1024; i++)
            data[i] = (float)(i / 1024.f),
            data[i + 1024] = (float)(i / 1024.f - 1.0f);
        waves[wave_saw].make(bl, data);

        for (int i = 0 ; i < 2048; i++)
            data[i] = (float)(i < 1024 ? -1.f : 1.f);
        waves[wave_sqr].make(bl, data);

        for (int i = 0 ; i < 2048; i++)
            data[i] = (float)(i < 64 ? -1.f : 1.f);
        waves[wave_pulse].make(bl, data);

        // XXXKF sure this is a waste of space, this will be fixed some day by better bandlimiter
        for (int i = 0 ; i < 2048; i++)
            data[i] = (float)sin(i * PI / 1024);
        waves[wave_sine].make(bl, data);

        for (int i = 0 ; i < 512; i++) {
            data[i] = i / 512.0,
            data[i + 512] = 1 - i / 512.0,
            data[i + 1024] = - i / 512.0,
            data[i + 1536] = -1 + i / 512.0;
        }
        waves[wave_triangle].make(bl, data);
    }
    void deactivate() {
    }
    void calculate_step() {
        if (queue_note_on != -1)
            note_on();
        else if (stopping)
        {
            running = false;
            dsp::zero(buffer, step_size);
            return;
        }
        set_frequency();
        float env = max(0.f, 1.f - voice_age * decay_factor);
        cutoff = *params[par_cutoff] * pow(2.0f, env * *params[par_envmod] * (1.f / 1200.f));
        if (*params[par_keyfollow] >= 0.5f)
            cutoff *= freq / 264.0f;
        if (cutoff < 10.f)
            cutoff = 10.f;
        if (cutoff > 16000.f)
            cutoff = 16000.f;
        float resonance = *params[par_resonance];
        float e2r = *params[par_envtores];
        resonance = resonance * (1 - e2r) + (0.7 + (resonance - 0.7) * env) * e2r;
        filter.set_lp_rbj(cutoff, resonance, srate);
        float fgain = min(0.5, 0.5 / resonance);
        filter2.copy_coeffs(filter);
        for (uint32_t i = 0; i < step_size; i++) 
        {
            float osc1val = osc1.get();
            float osc2val = osc2.get();
            float wave = fgain * (osc1val + (osc2val - osc1val) * xfade);
            wave = filter.process_d1(wave);
            wave = filter2.process_d1(wave);
            // primitive waveshaping (hard-knee)
            float abswave = fabs(wave);
            if (abswave > 0.75)
            {
                abswave = abswave - 0.5 * (abswave - 0.75);
                if (abswave > 1.0)
                    abswave = 1.0;
                wave = (wave > 0.0) ? abswave : - abswave;
                
            }
            buffer[i] = dsp::clip(wave, -1.f, +1.f);
        }
        if (!gate)
        {
            for (int i = 0; i < step_size; i++)
                buffer[i] *= (step_size - i) * (1.0f / step_size);
            stopping = true;
        }

        voice_age++;
    }
    uint32_t process(uint32_t offset, uint32_t nsamples, uint32_t inputs_mask, uint32_t outputs_mask) {
        if (!running && queue_note_on == -1)
            return 0;
        uint32_t op = offset;
        uint32_t op_end = offset + nsamples;
        while(op < op_end) {
            if (output_pos == 0) {
                if (running || queue_note_on != -1)
                    calculate_step();
                else 
                    dsp::zero(buffer, step_size);
            }
            if(op < op_end) {
                uint32_t ip = output_pos;
                uint32_t len = std::min(step_size - output_pos, op_end - op);
                for(uint32_t i = 0 ; i < len; i++)
                    outs[0][op + i] = outs[1][op + i] = buffer[ip + i];
                op += len;
                output_pos += len;
                if (output_pos == step_size)
                    output_pos = 0;
            }
        }
            
        return 3;
    }
};

struct organ_audio_module: public drawbar_organ
{
public:
    enum { par_drawbar1, par_drawbar2, par_drawbar3, par_drawbar4, par_drawbar5, par_drawbar6, par_drawbar7, par_drawbar8, par_drawbar9, par_foldover,
        par_percmode, par_percharm, par_vibrato, par_master, param_count };
    enum { in_count = 0, out_count = 2, support_midi = true, rt_capable = true };
    static const char *param_names[];
    float *ins[in_count]; 
    float *outs[out_count];
    float *params[param_count];
    organ_parameters par_values;
    uint32_t srate;

    organ_audio_module()
    : drawbar_organ(&par_values)
    {
    }
    static parameter_properties param_props[];
    void set_sample_rate(uint32_t sr) {
        srate = sr;
    }
    void handle_event(uint8_t *data, int len) {
        switch(*data >> 4)
        {
        case 9:
            note_on(data[1], data[2]);
            break;
        case 8:
            note_off(data[1], data[2]);
            break;
        case 11:
            control_change(data[1], data[2]);
            break;
        }
        default_handle_event(data, len, params, param_count);
    }
    void params_changed() {
        for (int i = 0; i < param_count; i++)
            ((float *)&par_values)[i] = *params[i];
        set_vibrato();
    }
    void activate() {
        setup(srate);
    }
    void deactivate() {
    }
    uint32_t process(uint32_t offset, uint32_t nsamples, uint32_t inputs_mask, uint32_t outputs_mask) {
        float *o[2] = { outs[0] + offset, outs[1] + offset };
        render_to(o, nsamples);
        return 3;
    }
    
};

};

#endif

#endif
