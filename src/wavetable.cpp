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

void wavetable_voice::reset()
{
    note = -1;
}

void wavetable_voice::note_on(int note, int vel)
{
    this->note = note;
    amp.set(1.0);
    for (int i = 0; i < OscCount; i++) {
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
    for (int i = 0; i < BlockSize; i++)
        output_buffer[i][0] = output_buffer[i][1] = oscs[0].get() * envs[0].value * envs[0].value;
    for (int i = 0; i < EnvCount; i++)
        envs[i].advance();    
}

#endif
