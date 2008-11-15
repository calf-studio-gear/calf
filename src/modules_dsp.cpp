/* Calf DSP Library
 * Example audio modules - DSP code
 *
 * Copyright (C) 2001-2008 Krzysztof Foltman
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
#include <assert.h>
#include <memory.h>
#if USE_JACK
#include <jack/jack.h>
#endif
#include <calf/giface.h>
#include <calf/modules.h>
#include <cairo/cairo.h>

using namespace dsp;
using namespace calf_plugins;

static void set_channel_color(cairo_t *context, int channel)
{
    if (channel & 1)
        cairo_set_source_rgb(context, 0.75, 1, 0);
    else
        cairo_set_source_rgb(context, 0, 1, 0.75);
}

void flanger_audio_module::activate() {
    left.reset();
    right.reset();
    last_r_phase = *params[par_stereo] * (1.f / 360.f);
    left.reset_phase(0.f);
    right.reset_phase(last_r_phase);
    is_active = true;
}

void flanger_audio_module::set_sample_rate(uint32_t sr) {
    srate = sr;
    left.setup(sr);
    right.setup(sr);
}

void flanger_audio_module::deactivate() {
    is_active = false;
}

bool flanger_audio_module::get_graph(int index, int subindex, float *data, int points, cairo_t *context)
{
    if (!is_active)
        return false;
    if (index == par_delay && subindex < 2) 
    {
        set_channel_color(context, subindex);
        return calf_plugins::get_graph(*this, subindex, data, points);
    }
    return false;
}

float flanger_audio_module::freq_gain(int subindex, float freq, float srate)
{
    return (subindex ? right : left).freq_gain(freq, srate);                
}

bool flanger_audio_module::get_gridline(int index, int subindex, float &pos, bool &vertical, cairo_t *context)
{
    if (index == par_delay && !subindex)
    {
        pos = 0.5;
        vertical = false;
        return true;
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////

void phaser_audio_module::set_sample_rate(uint32_t sr)
{
    srate = sr;
    left.setup(sr);
    right.setup(sr);
}

void phaser_audio_module::activate()
{
    is_active = true;
    left.reset();
    right.reset();
    last_r_phase = *params[par_stereo] * (1.f / 360.f);
    left.reset_phase(0.f);
    right.reset_phase(last_r_phase);
}

void phaser_audio_module::deactivate()
{
    is_active = false;
}

///////////////////////////////////////////////////////////////////////////////////////////////

void reverb_audio_module::activate()
{
    reverb.reset();
}

void reverb_audio_module::deactivate()
{
}

void reverb_audio_module::set_sample_rate(uint32_t sr)
{
    srate = sr;
    reverb.setup(sr);
    amount.set_sample_rate(sr);
}

///////////////////////////////////////////////////////////////////////////////////////////////

void filter_audio_module::activate()
{
    params_changed();
    for (int i=0; i < order; i++) {
        left[i].reset();
        right[i].reset();
    }
    timer = once_per_n(srate / 1000);
    timer.start();
    is_active = true;
}

void filter_audio_module::deactivate()
{
    is_active = false;
}

void filter_audio_module::set_sample_rate(uint32_t sr)
{
    srate = sr;
}

bool filter_audio_module::get_graph(int index, int subindex, float *data, int points, cairo_t *context)
{
    if (!is_active)
        return false;
    if (index == par_cutoff && !subindex) 
        return calf_plugins::get_graph(*this, subindex, data, points);
    return false;
}

float filter_audio_module::freq_gain(int subindex, float freq, float srate)
{
    float level = 1.0;
    for (int j = 0; j < order; j++)
        level *= left[j].freq_gain(freq, srate);                
    return level;
}

bool filter_audio_module::get_gridline(int index, int subindex, float &pos, bool &vertical, cairo_t *context)
{
    if (index == par_cutoff && !subindex)
    {
        pos = 0.5;
        vertical = false;
        return true;
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////

rotary_speaker_audio_module::rotary_speaker_audio_module()
{
    mwhl_value = hold_value = 0.f;
    phase_h = phase_l = 0.f;
    aspeed_l = 1.f;
    aspeed_h = 1.f;
    dspeed = 0.f;
}    

void rotary_speaker_audio_module::set_sample_rate(uint32_t sr)
{
    srate = sr;
    setup();
}

void rotary_speaker_audio_module::setup()
{
    crossover1l.set_lp_rbj(800.f, 0.7, (float)srate);
    crossover1r.set_lp_rbj(800.f, 0.7, (float)srate);
    crossover2l.set_hp_rbj(800.f, 0.7, (float)srate);
    crossover2r.set_hp_rbj(800.f, 0.7, (float)srate);
    set_vibrato();
}

void rotary_speaker_audio_module::activate()
{
    phase_h = phase_l = 0.f;
    maspeed_h = maspeed_l = 0.f;
    setup();
}

void rotary_speaker_audio_module::deactivate()
{
}

void rotary_speaker_audio_module::control_change(int ctl, int val)
{
    if (vibrato_mode == 3 && ctl == 64)
    {
        hold_value = val / 127.f;
        set_vibrato();
        return;
    }
    if (vibrato_mode == 4 && ctl == 1)
    {
        mwhl_value = val / 127.f;
        set_vibrato();
        return;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////

void multichorus_audio_module::activate()
{
    is_active = true;
    params_changed();
}

void multichorus_audio_module::deactivate()
{
    is_active = false;
}

void multichorus_audio_module::set_sample_rate(uint32_t sr) {
    srate = sr;
    left.setup(sr);
    right.setup(sr);
}

bool multichorus_audio_module::get_graph(int index, int subindex, float *data, int points, cairo_t *context)
{
    if (!is_active)
        return false;
    if (index == par_delay && subindex < 2) 
    {
        set_channel_color(context, subindex);
        return calf_plugins::get_graph(*this, subindex, data, points);
    }
    if (index == par_rate && !subindex) {
        for (int i = 0; i < points; i++)
            data[i] = 0.95 * sin(i * 2 * M_PI / points);
        return true;
    }
    return false;
}

bool multichorus_audio_module::get_dot(int index, int subindex, float &x, float &y, int &size, cairo_t *context)
{
    if (index != par_rate || subindex >= 2 * (int)*params[par_voices])
        return false;

    set_channel_color(context, subindex);
    sine_multi_lfo<float, 8> &lfo = (subindex & 1 ? right : left).lfo;
    x = (double)(lfo.phase + lfo.vphase * (subindex >> 1)) / 4096.0;
    y = 0.95 * sin(x * 2 * M_PI);
    return true;
}

bool multichorus_audio_module::get_gridline(int index, int subindex, float &pos, bool &vertical, cairo_t *context)
{
    if (index == par_rate && !subindex)
    {
        pos = 0;
        vertical = false;
        return true;
    }
    if (index == par_delay && !subindex)
    {
        pos = 0.5;
        vertical = false;
        return true;
    }
    return false;
}

float multichorus_audio_module::freq_gain(int subindex, float freq, float srate)
{
    return (subindex ? right : left).freq_gain(freq, srate);                
}
