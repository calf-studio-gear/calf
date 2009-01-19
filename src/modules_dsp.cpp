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

using namespace dsp;
using namespace calf_plugins;

/// convert amplitude value to normalized grid-ish value (0dB = 0.5, 30dB = 1.0, -30 dB = 0.0, -60dB = -0.5, -90dB = -1.0)
static inline float dB_grid(float amp)
{
    return log(amp) / log(256.0) + 0.4;
}

template<class Fx>
static bool get_graph(Fx &fx, int subindex, float *data, int points)
{
    for (int i = 0; i < points; i++)
    {
        typedef std::complex<double> cfloat;
        double freq = 20.0 * pow (20000.0 / 20.0, i * 1.0 / points);
        data[i] = dB_grid(fx.freq_gain(subindex, freq, fx.srate));
    }
    return true;
}

/// convert normalized grid-ish value back to amplitude value
static inline float dB_grid_inv(float pos)
{
    return pow(256.0, pos - 0.4);
}

static void set_channel_color(cairo_iface *context, int channel)
{
    if (channel & 1)
        context->set_source_rgba(0.75, 1, 0);
    else
        context->set_source_rgba(0, 1, 0.75);
    context->set_line_width(1.5);
}

static bool get_freq_gridline(int subindex, float &pos, bool &vertical, std::string &legend, cairo_iface *context)
{
    if (subindex < 28)
    {
        vertical = true;
        if (subindex == 9) legend = "100 Hz";
        if (subindex == 18) legend = "1 kHz";
        if (subindex == 27) legend = "10 kHz";
        float freq = 100;
        if (subindex < 9)
            freq = 10 * (subindex + 1);
        else if (subindex < 18)
            freq = 100 * (subindex - 9 + 1);
        else if (subindex < 27)
            freq = 1000 * (subindex - 18 + 1);
        else
            freq = 10000 * (subindex - 27 + 1);
        pos = log(freq / 20.0) / log(1000);
        if (!legend.empty())
            context->set_source_rgba(0.25, 0.25, 0.25, 0.75);
        else
            context->set_source_rgba(0.25, 0.25, 0.25, 0.5);
        return true;
    }
    subindex -= 28;
    float gain = 16.0 / (1 << subindex);
    pos = dB_grid(gain);
    if (pos < -1)
        return false;
    if (subindex != 4)
        context->set_source_rgba(0.25, 0.25, 0.25, subindex & 1 ? 0.5 : 0.75);
    if (!(subindex & 1))
    {
        std::stringstream ss;
        ss << (24 - 6 * subindex) << " dB";
        legend = ss.str();
    }
    vertical = false;
    return true;
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

bool flanger_audio_module::get_graph(int index, int subindex, float *data, int points, cairo_iface *context)
{
    if (!is_active)
        return false;
    if (index == par_delay && subindex < 2) 
    {
        set_channel_color(context, subindex);
        return ::get_graph(*this, subindex, data, points);
    }
    return false;
}

float flanger_audio_module::freq_gain(int subindex, float freq, float srate)
{
    return (subindex ? right : left).freq_gain(freq, srate);                
}

bool flanger_audio_module::get_gridline(int index, int subindex, float &pos, bool &vertical, std::string &legend, cairo_iface *context)
{
    if (index == par_delay)
        return get_freq_gridline(subindex, pos, vertical, legend, context);
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

bool phaser_audio_module::get_graph(int index, int subindex, float *data, int points, cairo_iface *context)
{
    if (!is_active)
        return false;
    if (subindex < 2) 
    {
        set_channel_color(context, subindex);
        return ::get_graph(*this, subindex, data, points);
    }
    return false;
}

float phaser_audio_module::freq_gain(int subindex, float freq, float srate)
{
    return (subindex ? right : left).freq_gain(freq, srate);                
}

bool phaser_audio_module::get_gridline(int index, int subindex, float &pos, bool &vertical, std::string &legend, cairo_iface *context)
{
    return get_freq_gridline(subindex, pos, vertical, legend, context);
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

bool filter_audio_module::get_graph(int index, int subindex, float *data, int points, cairo_iface *context)
{
    if (!is_active)
        return false;
    if (index == par_cutoff && !subindex) {
        context->set_line_width(1.5);
        return ::get_graph(*this, subindex, data, points);
    }
    return false;
}

float filter_audio_module::freq_gain(int subindex, float freq, float srate)
{
    float level = 1.0;
    for (int j = 0; j < order; j++)
        level *= left[j].freq_gain(freq, srate);                
    return level;
}

bool filter_audio_module::get_gridline(int index, int subindex, float &pos, bool &vertical, std::string &legend, cairo_iface *context)
{
    if (index == par_cutoff)
        return get_freq_gridline(subindex, pos, vertical, legend, context);
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

bool multichorus_audio_module::get_graph(int index, int subindex, float *data, int points, cairo_iface *context)
{
    if (!is_active)
        return false;
    if (index == par_delay && subindex < 3) 
    {
        if (subindex < 2)
            set_channel_color(context, subindex);
        else {
            context->set_source_rgba(0, 1, 0);
            context->set_line_width(1.0);
        }
        return ::get_graph(*this, subindex, data, points);
    }
    if (index == par_rate && !subindex) {
        for (int i = 0; i < points; i++)
            data[i] = 0.95 * sin(i * 2 * M_PI / points);
        return true;
    }
    return false;
}

bool multichorus_audio_module::get_dot(int index, int subindex, float &x, float &y, int &size, cairo_iface *context)
{
    if ((index != par_rate && index != par_depth) || subindex >= 2 * (int)*params[par_voices])
        return false;

    set_channel_color(context, subindex);
    sine_multi_lfo<float, 8> &lfo = (subindex & 1 ? right : left).lfo;
    if (index == par_rate)
    {
        x = (double)(lfo.phase + lfo.vphase * (subindex >> 1)) / 4096.0;
        y = 0.95 * sin(x * 2 * M_PI);
    }
    else
    {
        double ph = (double)(lfo.phase + lfo.vphase * (subindex >> 1)) / 4096.0;
        x = 0.5 + 0.5 * sin(ph * 2 * M_PI);
        y = subindex & 1 ? -0.75 : 0.75;
    }
    return true;
}

bool multichorus_audio_module::get_gridline(int index, int subindex, float &pos, bool &vertical, std::string &legend, cairo_iface *context)
{
    if (index == par_rate && !subindex)
    {
        pos = 0;
        vertical = false;
        return true;
    }
    if (index == par_delay)
        return get_freq_gridline(subindex, pos, vertical, legend, context);
    return false;
}

float multichorus_audio_module::freq_gain(int subindex, float freq, float srate)
{
    if (subindex == 2)
        return *params[par_amount] * left.post.freq_gain(freq, srate);
    return (subindex ? right : left).freq_gain(freq, srate);                
}

///////////////////////////////////////////////////////////////////////////////////////////////

compressor_audio_module::compressor_audio_module()
{
    is_active = false;
    srate = 0;
}

void compressor_audio_module::activate()
{
    is_active = true;
    linSlope = 0.f;
    peak = 0.f;
    clip = 0.f;
}

void compressor_audio_module::deactivate()
{
    is_active = false;
}

void compressor_audio_module::set_sample_rate(uint32_t sr)
{
    srate = sr;
    awL.set(sr);
    awR.set(sr);
}

bool compressor_audio_module::get_graph(int index, int subindex, float *data, int points, cairo_iface *context)
{ 
    if (!is_active)
        return false;
    if (subindex > 1) // 1
        return false;
    for (int i = 0; i < points; i++)
    {
        float input = dB_grid_inv(-1.0 + i * 2.0 / (points - 1));
        float output = output_level(input);
        if (subindex == 0)
            data[i] = dB_grid(input);
        else
            data[i] = dB_grid(output); 
    }
    if (subindex == (*params[param_bypass] > 0.5f ? 1 : 0))
        context->set_source_rgba(0.5, 0.5, 0.5, 0.5);
    else {
        context->set_source_rgba(0, 1, 0, 1);
        context->set_line_width(2);
    }
    return true;
}

bool compressor_audio_module::get_dot(int index, int subindex, float &x, float &y, int &size, cairo_iface *context)
{
    if (!is_active)
        return false;
    if (!subindex)
    {
        x = 0.5 + 0.5 * dB_grid(detected);
        y = dB_grid(*params[param_bypass] > 0.5f ? detected : output_level(detected));
        return *params[param_bypass] > 0.5f ? false : true;
    }
    return false;
}

bool compressor_audio_module::get_gridline(int index, int subindex, float &pos, bool &vertical, std::string &legend, cairo_iface *context)
{
    bool tmp;
    vertical = (subindex & 1) != 0;
    bool result = get_freq_gridline(subindex >> 1, pos, tmp, legend, context);
    if (result && vertical) {
        if ((subindex & 4) && !legend.empty()) {
            legend = "";
        }
        else {
            size_t pos = legend.find(" dB");
            if (pos != std::string::npos)
                legend.erase(pos);
        }
        pos = 0.5 + 0.5 * pos;
    }
    return result;
}
