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

namespace synth {

using namespace dsp;
        
/// Monosynth-in-making. Parameters may change at any point, so don't make songs with it!
class monosynth_audio_module
{
public:
    enum { par_cutoff, par_resonance, par_envmod, par_decay, param_count };
    enum { in_count = 0, out_count = 2, support_midi = true, rt_capable = true };
    enum { step_size = 64 };
    static const char *param_names[];
    float *ins[in_count]; 
    float *outs[out_count];
    float *params[param_count];
    uint32_t srate, crate;
    dsp::sine_table<float, 2048, 1> waveform_sine;
    waveform_family<11> waves_saw;
    waveform_oscillator<11> osc1, osc2;
    bool running;
    int last_key;
    
    float buffer[step_size];
    uint32_t output_pos;
    biquad<float> filter;
    biquad<float> filter2;
    float cutoff, decay_factor;
    int voice_age;
    float odcr;
    
    static parameter_properties param_props[];
    void set_sample_rate(uint32_t sr) {
        srate = sr;
        crate = sr / step_size;
        odcr = (float)(1.0 / crate);
    }
    void handle_event(uint8_t *data, int len) {
        switch(*data >> 4)
        {
        case 9:
            if (data[2]) {
                last_key = data[1];
                float freq = 440 * pow(2.0, (last_key - 69) / 12.0);
                osc1.set_freq(freq*0.995, srate);
                osc2.set_freq(freq*1.005, srate);
                osc2.waveform = osc1.waveform = waves_saw.get_level(osc1.phasedelta);
                if (!running)
                {
                    osc1.reset();
                    osc2.reset();
                    filter.reset();
                    filter2.reset();
                    cutoff = 16000;
                    voice_age = 0;
                    running = true;
                }
            }
            // printf("note on %d %d\n", data[1], data[2]);
            break;
        case 8:
            // printf("note off %d %d\n", data[1], data[2]);
            if (data[1] == last_key)
                running = false;
            break;
        }
        default_handle_event(data, len, params, param_count);
    }
    void params_changed() {
        decay_factor = odcr * 1000.0 / *params[par_decay];
    }
    void activate() {
        running = false;
        output_pos = 0;
        filter.reset();
        filter2.reset();
        float data[2048];
        for (int i = 0 ; i < 2048; i++)
            data[i] = (float)(-1.f + i / 1024.f);
        bandlimiter<11> bl;
        waves_saw.make(bl, data);
    }
    void deactivate() {
    }
    void calculate_step() {
        float env = max(0.f, 1.f - voice_age * decay_factor);
        cutoff = *params[par_cutoff] * pow(2.0f, env * *params[par_envmod] * (1.f / 1200.f));
        if (cutoff < 33.f)
            cutoff = 33.f;
        if (cutoff > 16000.f)
            cutoff = 16000.f;
        filter.set_lp_rbj(cutoff, *params[par_resonance], srate);
        filter2.copy_coeffs(filter);
        for (uint32_t i = 0; i < step_size; i++) 
        {
            float wave = 0.2f * (osc1.get() + osc2.get());
            wave = filter.process_d1(wave);
            wave = filter2.process_d1(wave);
            buffer[i] = dsp::clip(wave, -1.f, +1.f);
        }

        voice_age++;
    }
    uint32_t process(uint32_t offset, uint32_t nsamples, uint32_t inputs_mask, uint32_t outputs_mask) {
        if (!running)
            return 0;
        uint32_t op = offset;
        uint32_t op_end = offset + nsamples;
        while(op < op_end) {
            if (output_pos == 0)
                calculate_step();
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
    
};

#endif

#endif
