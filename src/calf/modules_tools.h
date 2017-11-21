/* Calf DSP plugin pack
 * Assorted plugins
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
#ifndef CALF_MODULES_TOOLS_H
#define CALF_MODULES_TOOLS_H

#include <assert.h>
#include <limits.h>
#include "biquad.h"
#include "inertia.h"
#include "audio_fx.h"
#include "analyzer.h"
#include "giface.h"
#include "metadata.h"
#include "loudness.h"
#include <math.h>
#include "plugin_tools.h"
#include "bypass.h"

namespace calf_plugins {

struct ladspa_plugin_info;

/**********************************************************************
 * STEREO TOOLS by Markus Schmidt 
**********************************************************************/

class stereo_audio_module:
    public audio_module<stereo_metadata>
{
    typedef stereo_audio_module AM;
    uint32_t srate;
    bool active;
    dsp::bypass bypass;
    float meter_inL, meter_inR, meter_outL, meter_outR, meter_phase;
    vumeters meters;
    
    float * buffer;
    unsigned int pos;
    unsigned int buffer_size;
    static inline float sign(float x) {
        if(x < 0) return -1.f;
        if(x > 0) return 1.f;
        return 0.f;
    }
    float _phase, _phase_sin_coef, _phase_cos_coef, _sc_level, _inv_atan_shape;
public:
    stereo_audio_module();
    ~stereo_audio_module();
    void params_changed();
    void activate();
    void set_sample_rate(uint32_t sr);
    void deactivate();
    uint32_t process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask);
};

/**********************************************************************
 * MONO INPUT by Markus Schmidt
**********************************************************************/

class mono_audio_module:
    public audio_module<mono_metadata>
{
    typedef mono_audio_module AM;
    uint32_t srate;
    bool active;
    dsp::bypass bypass;
    float meter_in, meter_outL, meter_outR;
    vumeters meters;
    
    float * buffer;
    unsigned int pos;
    unsigned int buffer_size;
    static inline float sign(float x) {
        if(x < 0) return -1.f;
        if(x > 0) return 1.f;
        return 0.f;
    }
    float _phase, _phase_sin_coef, _phase_cos_coef, _sc_level, _inv_atan_shape;
public:
    mono_audio_module();
    ~mono_audio_module();
    void params_changed();
    void activate();
    void set_sample_rate(uint32_t sr);
    void deactivate();
    uint32_t process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask);
};

/**********************************************************************
 * ANALYZER by Markus Schmidt and Christian Holschuh
**********************************************************************/

class analyzer_audio_module:
    public audio_module<analyzer_metadata>, public phase_graph_iface,
    public frequency_response_line_graph
{
    typedef analyzer_audio_module AM;
    uint32_t srate;
    bool active;
    float envelope;
    float attack_coef;
    float release_coef;
    uint32_t clip_L, clip_R;
    float meter_L, meter_R;
    analyzer _analyzer;
public:
    analyzer_audio_module();
    void params_changed();
    void activate();
    void set_sample_rate(uint32_t sr);
    void deactivate();
    uint32_t process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask);
    bool get_phase_graph(int index, float ** _buffer, int * _length, int * _mode, bool * _use_fade, float * _fade, int * _accuracy, bool * _display) const;
    bool get_graph(int index, int subindex, int phase, float *data, int points, cairo_iface *context, int *mode) const;
    bool get_moving(int index, int subindex, int &direction, float *data, int x, int y, int &offset, uint32_t &color) const;
    bool get_gridline(int index, int subindex, int phase, float &pos, bool &vertical, std::string &legend, cairo_iface *context) const;
    bool get_layers(int index, int generation, unsigned int &layers) const;
    ~analyzer_audio_module();
protected:
    static const int max_phase_buffer_size = 8192;
    int phase_buffer_size;
    float *phase_buffer;
    int ppos;
    int plength;
};

/**********************************************************************
 * MULTIBAND ENHANCER by Markus Schmidt
**********************************************************************/

class multibandenhancer_audio_module: public audio_module<multibandenhancer_metadata>,
    public frequency_response_line_graph, public phase_graph_iface {
private:
    static const int strips = 4;
    int _mode, mode_old, channels;
    float envelope[strips];
    float attack_coef;
    float release_coef;
    bool solo[strips];
    bool no_solo;
    dsp::crossover crossover;
    dsp::bypass bypass;
    vumeters meters;
    dsp::tap_distortion dist[strips][2];
public:
    uint32_t srate;
    bool is_active;
    multibandenhancer_audio_module();
    ~multibandenhancer_audio_module();
    void activate();
    void deactivate();
    void params_changed();
    uint32_t process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask);
    void set_sample_rate(uint32_t sr);
    bool get_phase_graph(int index, float ** _buffer, int * _length, int * _mode, bool * _use_fade, float * _fade, int * _accuracy, bool * _display) const;
    bool get_graph(int index, int subindex, int phase, float *data, int points, cairo_iface *context, int *mode) const;
    bool get_layers(int index, int generation, unsigned int &layers) const;
protected:
    static const int max_phase_buffer_size = 8192;
    int phase_buffer_size;
    float *phase_buffer[strips];
    int ppos;
    int plength;
};

/**********************************************************************
 * MULTIBAND SPREAD by Markus Schmidt
**********************************************************************/

class multispread_audio_module: public audio_module<multispread_metadata>,
    public frequency_response_line_graph, public phase_graph_iface {
private:
    dsp::bypass bypass;
    vumeters meters;
    dsp::biquad_d1 L[4*16], R[4*16];
public:
    uint32_t srate;
    bool is_active;
    mutable bool redraw_graph;
    float amount0, amount1, amount2, amount3, filters, intensity;
    float fcoeff;
    multispread_audio_module();
    ~multispread_audio_module();
    void activate();
    void deactivate();
    void params_changed();
    uint32_t process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask);
    void set_sample_rate(uint32_t sr);
    bool get_graph(int index, int subindex, int phase, float *data, int points, cairo_iface *context, int *mode) const;
    bool get_layers(int index, int generation, unsigned int &layers) const;
    bool get_gridline(int index, int subindex, int phase, float &pos, bool &vertical, std::string &legend, cairo_iface *context) const;
    bool get_phase_graph(int index, float ** _buffer, int * _length, int * _mode, bool * _use_fade, float * _fade, int * _accuracy, bool * _display) const;
    float freq_gain(int index, double freq) const;
protected:
    static const int max_phase_buffer_size = 8192;
    int phase_buffer_size;
    float *phase_buffer;
    int ppos;
    int plength;
    float envelope;
    float attack_coef;
    float release_coef;
};

/**********************************************************************
 * WIDGET TEST
**********************************************************************/

class widgets_audio_module:
    public audio_module<widgets_metadata>
{
    uint32_t srate;
    vumeters meters;
    dsp::simple_lfo lfo_sin, lfo_sqr, lfo_tri;
public:
    widgets_audio_module();
    ~widgets_audio_module();
    void params_changed();
    void set_sample_rate(uint32_t sr);
    uint32_t process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask);
};

};
#endif
