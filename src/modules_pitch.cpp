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

#if ENABLE_EXPERIMENTAL

// http://www.cs.otago.ac.nz/tartini/papers/A_Smarter_Way_to_Find_Pitch.pdf

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
    for (size_t i = 0; i < 2 * BufferSize; ++i)
        waveform[i] = spectrum[i] = autocorr[i] = 0;
    for (size_t i = 0; i < BufferSize; ++i)
        inputbuf[i] = 0;
}

void pitch_audio_module::deactivate()
{
}

void pitch_audio_module::recompute()
{
    // second half of the waveform always zero
    double sumsquares_acc = 0.;
    for (int i = 0; i < BufferSize; ++i)
    {
        float val = inputbuf[(i + write_ptr) & (BufferSize - 1)];
        float win = 0.54 - 0.46 * cos(i * M_PI / BufferSize);
        val *= win;
        waveform[i] = val;
        sumsquares[i] = sumsquares_acc;
        sumsquares_acc += val * val;
    }
    sumsquares[BufferSize] = sumsquares_acc;
        //waveform[i] = inputbuf[(i + write_ptr) & (BufferSize - 1)];
    transform.calculate(waveform, spectrum, false);
    pfft::complex temp[2 * BufferSize];
    for (int i = 0; i < 2 * BufferSize; ++i)
    {
        float val = std::abs(spectrum[i]);
        temp[i] = val * val;
    }
        //temp[i] = spectrum[i] * std::conj(spectrum[i]);
    transform.calculate(temp, autocorr, true);
    sumsquares_last = sumsquares_acc;
    float maxpt = 0;
    int maxpos = -1;
    int i;
    for (i = 2; i < BufferSize / 2; ++i)
    {
        float mag = 2.0 * autocorr[i].real() / (sumsquares[BufferSize] + sumsquares[BufferSize - i] - sumsquares[i]);
        magarr[i] = mag;
        if (mag > maxpt)
        {
            maxpt = mag;
            maxpos = i;
        }
    }
    for (i = 2; i < BufferSize / 2 && magarr[i + 1] < magarr[i]; ++i)
        ;
    float thr = *params[par_pd_threshold];
    for (; i < BufferSize / 2; ++i)
    {
        if (magarr[i] >= thr * maxpt)
        {
            while(i < BufferSize / 2 - 1 && magarr[i + 1] > magarr[i])
                i++;
            maxpt = magarr[i];
            maxpos = i;
            break;
        }
    }
    if (maxpt > 0 && maxpos < BufferSize / 2 - 1)
    {
        float y1 = magarr[maxpos - 1];
        float y2 = magarr[maxpos];
        float y3 = magarr[maxpos + 1];
        float pos2 = maxpos + 0.5 * (y1 - y3) / (y1 - 2 * y2 + y3);
        dsp::note_desc desc = dsp::hz_to_note(srate / pos2, *params[par_tune]);
        *params[par_note]  = desc.note;
        *params[par_cents] = desc.cents;
        *params[par_freq]  = desc.freq;
        *params[par_clarity] = maxpt;
    }
    *params[par_clarity] = maxpt;
}

bool pitch_audio_module::get_graph(int index, int subindex, int phase, float *data, int points, cairo_iface *context, int *mode) const
{
    if (index == par_pd_threshold && subindex == 0)
    {
        context->set_source_rgba(1, 0, 0);
        for (int i = 0; i < points; i++)
        {
            float ac = autocorr[i * (BufferSize / 2 - 1) / (points - 1)].real();
            if (ac >= 0)
                data[i] = sqrt(ac / sumsquares_last);
            else
                data[i] = -sqrt(-ac / sumsquares_last);
        }
        return true;
    }
    if (index == par_pd_threshold && subindex == 1)
    {
        context->set_source_rgba(0, 0, 1);
        for (int i = 0; i < points; i++)
        {
            data[i] = 0.0625 * log(std::abs(spectrum[i * (BufferSize / 4 - 1) / (points - 1)]));
        }
        return true;
    }
    if (index == par_pd_threshold && subindex == 2)
    {
        context->set_source_rgba(0, 0, 0);
        for (int i = 0; i < points; i++)
        {
            int j = i * (BufferSize / 2 - 1) / (points - 1);
            float mag = magarr[j];
            data[i] = mag;
        }
        return true;
    }
    if (index == par_pd_threshold && subindex == 3)
    {
        context->set_source_rgba(0, 1, 1);
        for (int i = 0; i < points; i++)
        {
            int j = i * (BufferSize - 1) / (points - 1);
            float mag = std::abs(sumsquares[j]);
            data[i] = 0.25 * log(mag);
        }
        return true;
    }
    return false;
}

uint32_t pitch_audio_module::process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask)
{
    uint32_t endpos = offset + numsamples;
    bool has2nd = ins[1] != NULL;

    int bperiod = BufferSize;
    int sd = *params[par_pd_subdivide];
    if (sd >= 1 && sd <= 8)
        bperiod /= sd;

    // XXXKF note: not working, not optimized, just trying stuff out
    for (uint32_t i = offset; i < endpos; ++i)
    {
        float val = ins[0][i];
        inputbuf[write_ptr] = val;
        write_ptr = (write_ptr + 1) & (BufferSize - 1);
        if (!(write_ptr % bperiod))
            recompute();
        outs[0][i] = ins[0][i];
        if (has2nd)
            outs[1][i] = ins[1][i];
    }
    return outputs_mask;
}

#endif
