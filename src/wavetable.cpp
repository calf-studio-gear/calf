/* Calf DSP Library
 * Example audio modules - wavetable synthesizer
 *
 * Copyright (C) 2009 Krzysztof Foltman
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
#include <config.h>

#if ENABLE_EXPERIMENTAL
    
#include <assert.h>
#include <memory.h>
#include <complex>
#if USE_JACK
#include <jack/jack.h>
#endif
#include <calf/giface.h>
#include <calf/modules_synths.h>
#include <iostream>

using namespace dsp;
using namespace calf_plugins;

wavetable_voice::wavetable_voice()
{
    sample_rate = -1;
}

void wavetable_voice::set_params_ptr(wavetable_audio_module *_parent, int _srate)
{
    parent = _parent;
    params = parent->params;
    sample_rate = _srate;
}

void wavetable_voice::reset()
{
    note = -1;
}

void wavetable_voice::note_on(int note, int vel)
{
    this->note = note;
    velocity = vel / 127.0;
    amp.set(1.0);
    for (int i = 0; i < OscCount; i++) {
        oscs[i].tables = parent->tables;
        oscs[i].reset();
        oscs[i].set_freq(note_to_hz(note, 0), sample_rate);
    }
    int cr = sample_rate / BlockSize;
    for (int i = 0; i < EnvCount; i++) {
        envs[i].set(0.01, 0.1, 0.5, 1, cr);
        envs[i].note_on();
    }
}

void wavetable_voice::note_off(int vel)
{
    for (int i = 0; i < EnvCount; i++)
        envs[i].note_off();
}

void wavetable_voice::steal()
{
}

void wavetable_voice::render_block()
{
    typedef wavetable_metadata md;
    
    int ospc = md::par_o2level - md::par_o1level;
    int espc = md::par_eg2attack - md::par_eg1attack;
    for (int j = 0; j < OscCount; j++)
        oscs[j].set_freq(note_to_hz(note, *params[md::par_o1transpose + j * ospc] * 100+ *params[md::par_o1detune + j * ospc]), sample_rate);
    float s = 0.001;
    for (int j = 0; j < EnvCount; j++) {
        int o = j*espc;
        envs[j].set(*params[md::par_eg1attack + o] * s, *params[md::par_eg1decay + o] * s, *params[md::par_eg1sustain + o], *params[md::par_eg1release + o] * s, sample_rate / BlockSize); 
    }
    
    float prev_value = envs[0].value;
    for (int i = 0; i < EnvCount; i++)
        envs[i].advance();    
    float cur_value = envs[0].value;
    
    for (int i = 0; i < BlockSize; i++) {        
        float value = 0.f;
        
        float env = velocity * (prev_value + (cur_value - prev_value) * i * (1.0 / BlockSize));
        for (int j = 0; j < OscCount; j++) {
            value += oscs[j].get(dsp::clip(fastf2i_drm((env + *params[md::par_o1offset + j * ospc]) * 127.0 * 256), 0, 127 * 256)) * *params[md::par_o1level + j * ospc];
        }
        
        output_buffer[i][0] = output_buffer[i][1] = value * env * env;
    }
    if (envs[0].stopped())
        released = true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////

wavetable_audio_module::wavetable_audio_module()
{
    panic_flag = false;
    for (int i = 0; i < 128; i++)
    {
        for (int j = 0; j < 256; j++)
        {
            //tables[i][j] = i < j ? -32767 : 32767;
            float ph = j * 2 * M_PI / 256;
            float ii = (i & ~3) / 128.0;
            float ii2 = ((i & ~3) + 4) / 128.0;
            float peak = (32 * ii);
            float rezo1 = sin(floor(peak) * ph);
            float rezo2 = sin(floor(peak + 1) * ph);
            float v1 = sin (ph + 2 * ii * sin(2 * ph) + 2 * ii * ii * sin(4 * ph) + ii * ii * rezo1);
            float v2 = sin (ph + 2 * ii2 * sin(2 * ph) + 2 * ii2 * ii2 * sin(4 * ph) + ii2 * ii2 * rezo2);
            tables[i][j] = 32767 * lerp(v1, v2, (i & 3) / 4.0);
        }
    }
}



#endif
