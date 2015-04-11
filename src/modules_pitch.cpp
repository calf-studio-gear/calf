/* Calf DSP plugin pack
 * Assorted plugins
 *
 * Copyright (C) 2015 Krzysztof Foltman, Markus Schmidt, Thor Harald Johansen
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
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 * Boston, MA  02110-1301  USA
 */

#include <limits.h>
#include <memory.h>
#include <math.h>
#include <calf/giface.h>
#include <calf/modules_pitch.h>
#include <calf/utils.h>

using namespace calf_plugins;

pitch_audio_module::pitch_audio_module()
{
}

pitch_audio_module::~pitch_audio_module()
{
}

void pitch_audio_module::set_sample_rate(uint32_t sr)
{
    srate = sr;
}

void pitch_audio_module::params_changed()
{
}

void pitch_audio_module::activate()
{
    write_ptr = 0;
}

void pitch_audio_module::deactivate()
{
}

void pitch_audio_module::recompute()
{
    transform.calculate(waveform, spectrum, false);
    for (int i = 0; i < 2 * BufferSize; ++i)
        spectrum[i] = std::abs(spectrum[i] * spectrum[i]);
    spectrum[0] = 0;
    transform.calculate(spectrum, autocorr, true);
    float maxpt = 0;
    int maxpos = -1;
    int i;
    for (i = 2; i < BufferSize / 2; ++i)
    {
        if (std::abs(autocorr[i]) > std::abs(autocorr[i - 1]))
            break;
    }
    for (; i < BufferSize / 2; ++i)
    {
        float mag = std::abs(autocorr[i]);
        if (mag > maxpt)
        {
            maxpt = mag;
            maxpos = i;
        }
    }
    if (maxpt > 0)
    {
        float y1 = std::abs(autocorr[maxpos - 1]);
        float y2 = std::abs(autocorr[maxpos]);
        float y3 = std::abs(autocorr[maxpos + 1]);
        
        float pos2 = maxpos + 0.5 * (y1 - y3) / (y1 - 2 * y2 + y3);
        printf("pos %d mag %f freq %f posx %f freqx %f\n", maxpos, maxpt, srate * 1.0 / maxpos, pos2, srate / pos2);
    }
}

bool pitch_audio_module::get_graph(int index, int subindex, int phase, float *data, int points, cairo_iface *context, int *mode) const
{
    if (index == par_threshold && subindex == 0)
    {
        context->set_source_rgba(1, 0, 0);
        for (int i = 0; i < points; i++)
        {
            data[i] = 0.25 * log(std::abs(autocorr[i]));
        }
        return true;
    }
    if (index == par_threshold && subindex == 1)
    {
        context->set_source_rgba(0, 0, 1);
        for (int i = 0; i < points; i++)
        {
            data[i] = 0.25 * log(std::abs(spectrum[i]));
        }
        return true;
    }
    return false;
}

uint32_t pitch_audio_module::process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask)
{
    uint32_t endpos = offset + numsamples;
    bool has2nd = ins[1] != NULL;
    for (uint32_t i = offset; i < endpos; ++i)
    {
        waveform[write_ptr] = ins[0][i];
        write_ptr = (write_ptr + 1) & (BufferSize - 1);
        if (!write_ptr)
            recompute();
        outs[0][i] = ins[0][i];
        if (has2nd)
            outs[1][i] = ins[1][i];
    }
    return outputs_mask;
}
