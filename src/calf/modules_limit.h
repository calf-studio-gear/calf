/* Calf DSP plugin pack
 * Limiter related plugins
 *
 * Copyright (C) 2001-2010 Krzysztof Foltman, Markus Schmidt, Thor Harald Johansen and others
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
 * Boston, MA 02111-1307, USA.
 */
#ifndef CALF_MODULES_LIMIT_H
#define CALF_MODULES_LIMIT_H

#include <assert.h>
#include <limits.h>
#include "biquad.h"
#include "inertia.h"
#include "audio_fx.h"
#include "giface.h"
#include "metadata.h"
#include "plugin_tools.h"
#include "bypass.h"

namespace calf_plugins {

/**********************************************************************
 * LIMITER by Christian Holschuh and Markus Schmidt
**********************************************************************/

class limiter_audio_module: public audio_module<limiter_metadata>, public line_graph_iface {
private:
    typedef limiter_audio_module AM;
    uint32_t asc_led;
    int mode, mode_old, oversampling_old;
    dsp::lookahead_limiter limiter;
    dsp::resampleN resampler[2];
    dsp::bypass bypass;
    vumeters meters;
public:
    uint32_t srate;
    bool is_active;
    float limit_old;
    bool asc_old;
    float attack_old;
    limiter_audio_module();
    void activate();
    void deactivate();
    void params_changed();
    void set_srates();
    uint32_t process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask);
    void set_sample_rate(uint32_t sr);
};

/**********************************************************************
 * MULTIBAND LIMITER by Markus Schmidt and Christian Holschuh 
**********************************************************************/

class multibandlimiter_audio_module: public audio_module<multibandlimiter_metadata>, public frequency_response_line_graph {
private:
    typedef multibandlimiter_audio_module AM;
    static const int strips = 4;
    uint32_t asc_led, cnt;
    int _mode, mode_old;
    bool solo[strips];
    bool no_solo;
    dsp::lookahead_limiter strip[strips];
    dsp::lookahead_limiter broadband;
    dsp::resampleN resampler[strips][2];
    dsp::crossover crossover;
    dsp::bypass bypass;
    float over;
    unsigned int pos;
    unsigned int buffer_size;
    unsigned int overall_buffer_size;
    float *buffer;
    int channels;
    float striprel[strips];
    float weight[strips];
    float weight_old[strips];
    float limit_old;
    bool asc_old;
    float attack_old;
    float oversampling_old;
    bool _sanitize;
    vumeters meters;
public:
    uint32_t srate;
    bool is_active;
    multibandlimiter_audio_module();
    ~multibandlimiter_audio_module();
    void activate();
    void deactivate();
    void params_changed();
    void set_srates();
    uint32_t process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask);
    void set_sample_rate(uint32_t sr);
    bool get_graph(int index, int subindex, int phase, float *data, int points, cairo_iface *context, int *mode) const;
    bool get_layers(int index, int generation, unsigned int &layers) const;
};

class sidechainlimiter_audio_module: public audio_module<sidechainlimiter_metadata>, public frequency_response_line_graph {
private:
    typedef sidechainlimiter_audio_module AM;
    static const int strips = 5;
    uint32_t asc_led, cnt;
    int _mode, mode_old;
    bool solo[strips];
    bool no_solo;
    dsp::lookahead_limiter strip[strips];
    dsp::lookahead_limiter broadband;
    dsp::resampleN resampler[strips][2];
    dsp::crossover crossover;
    dsp::bypass bypass;
    float over;
    unsigned int pos;
    unsigned int buffer_size;
    unsigned int overall_buffer_size;
    float *buffer;
    int channels;
    float striprel[strips];
    float weight[strips];
    float weight_old[strips];
    float limit_old;
    bool asc_old;
    float attack_old;
    float oversampling_old;
    bool _sanitize;
    vumeters meters;
public:
    uint32_t srate;
    bool is_active;
    sidechainlimiter_audio_module();
    ~sidechainlimiter_audio_module();
    void activate();
    void deactivate();
    void params_changed();
    void set_srates();
    uint32_t process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask);
    void set_sample_rate(uint32_t sr);
    bool get_graph(int index, int subindex, int phase, float *data, int points, cairo_iface *context, int *mode) const;
    bool get_layers(int index, int generation, unsigned int &layers) const;
};

};

#endif
