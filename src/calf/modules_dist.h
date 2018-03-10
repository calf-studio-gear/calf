/* Calf DSP plugin pack
 * Distortion related plugins
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
#ifndef CALF_MODULES_DIST_H
#define CALF_MODULES_DIST_H

#include <assert.h>
#include <limits.h>
#include "biquad.h"
#include "bypass.h"
#include "audio_fx.h"
#include "giface.h"
#include "metadata.h"
#include "plugin_tools.h"
#include <fluidsynth.h>


namespace calf_plugins {

/**********************************************************************
 * SATURATOR by Markus Schmidt
**********************************************************************/

class saturator_audio_module: public audio_module<saturator_metadata> {
private:
    float hp_pre_freq_old, lp_pre_freq_old;
    float hp_post_freq_old, lp_post_freq_old;
    float p_level_old, p_freq_old, p_q_old;
    dsp::biquad_d2 lp[2][4], hp[2][4];
    dsp::biquad_d2 p[2];
    dsp::tap_distortion dist[2];
    dsp::bypass bypass;
    vumeters meters;
public:
    uint32_t srate;
    bool is_active;
    saturator_audio_module();
    void activate();
    void deactivate();
    void params_changed();
    void set_sample_rate(uint32_t sr);
    uint32_t process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask);
};

/**********************************************************************
 * EXCITER by Markus Schmidt
**********************************************************************/

class exciter_audio_module: public audio_module<exciter_metadata> {
private:
    float freq_old, ceil_old;
    bool ceil_active_old;
    float meter_drive;
    dsp::biquad_d2 hp[2][4];
    dsp::biquad_d2 lp[2][2];
    dsp::tap_distortion dist[2];
    dsp::bypass bypass;
    vumeters meters;
public:
    uint32_t srate;
    bool is_active;
    exciter_audio_module();
    void activate();
    void deactivate();
    void params_changed();
    void set_sample_rate(uint32_t sr);
    uint32_t process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask);
};

/**********************************************************************
 * BASS ENHANCER by Markus Schmidt
**********************************************************************/

class bassenhancer_audio_module: public audio_module<bassenhancer_metadata> {
private:
    float freq_old, floor_old;
    bool floor_active_old;
    float meter_drive;
    dsp::biquad_d2 lp[2][4];
    dsp::biquad_d2 hp[2][2];
    dsp::tap_distortion dist[2];
    dsp::bypass bypass;
    vumeters meters;
public:
    uint32_t srate;
    bool is_active;
    bassenhancer_audio_module();
    void activate();
    void deactivate();
    void params_changed();
    void set_sample_rate(uint32_t sr);
    uint32_t process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask);
};

/**********************************************************************
 * VINYL by Markus Schmidt
**********************************************************************/

class vinyl_audio_module:
    public audio_module<vinyl_metadata>, public frequency_response_line_graph
{
    typedef vinyl_audio_module AM;
    bool active;
    uint32_t clip_inL, clip_inR, clip_outL, clip_outR;
    float meter_inL, meter_inR, meter_outL, meter_outR;
    mutable bool redraw_output;
    
    float speed_old, freq_old, aging_old;
    
    const static int channels = 2;
    const static int _filters = 5;
    const static int _synths = 7;
    const static int _synthsp = 3;
    
    dsp::bypass bypass;
    vumeters meters;
    dsp::simple_lfo lfo;
    dsp::biquad_d2 filters[2][_filters];
    fluid_synth_t *synth;
    fluid_settings_t* settings;
    float last_gain[_synths];
    
    uint32_t dbufsize, dbufpos;
    float *dbuf;
    float dbufrange;
    
public:
    uint32_t srate;
    vinyl_audio_module();
    ~vinyl_audio_module();
    void params_changed();
    void activate();
    void post_instantiate(uint32_t sr);
    void set_sample_rate(uint32_t sr);
    void deactivate();
    uint32_t process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask);
    float freq_gain(int index, double freq) const;
    bool get_graph(int index, int subindex, int phase, float *data, int points, cairo_iface *context, int *mode) const;
    bool get_layers(int index, int generation, unsigned int &layers) const;
};

/**********************************************************************
 * TAPESIMULATOR by Markus Schmidt
**********************************************************************/

class tapesimulator_audio_module:
    public audio_module<tapesimulator_metadata>, public frequency_response_line_graph
{
    typedef tapesimulator_audio_module AM;
    bool active;
    uint32_t clip_inL, clip_inR, clip_outL, clip_outR;
    float meter_inL, meter_inR, meter_outL, meter_outR;
    bool mech_old;
    mutable bool redraw_output;
    dsp::biquad_d2 lp[2][2];
    dsp::biquad_d2 noisefilters[2][3];
    dsp::transients transients;
    dsp::bypass bypass;
    vumeters meters;
    const static int channels = 2;
    dsp::simple_lfo lfo1, lfo2;
    float lp_old, output_old;
    mutable float rms, input;
public:
    uint32_t srate;
    tapesimulator_audio_module();
    void params_changed();
    void activate();
    void set_sample_rate(uint32_t sr);
    void deactivate();
    float freq_gain(int index, double freq) const;
    uint32_t process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask);
    bool get_graph(int index, int subindex, int phase, float *data, int points, cairo_iface *context, int *mode) const;
    bool get_dot(int index, int subindex, int phase, float &x, float &y, int &size, cairo_iface *context) const;
    bool get_gridline(int index, int subindex, int phase, float &pos, bool &vertical, std::string &legend, cairo_iface *context) const;
    bool get_layers(int index, int generation, unsigned int &layers) const;
};

/**********************************************************************
 * CRUSHER by Markus Schmidt and Christian Holschuh
**********************************************************************/

class crusher_audio_module:
    public audio_module<crusher_metadata>, public line_graph_iface
{
private:
    vumeters meters;
    dsp::bitreduction bitreduction;
    dsp::samplereduction samplereduction[2];
    dsp::simple_lfo lfo;
    dsp::bypass bypass;
    float smin, sdiff;
public:
    uint32_t srate;
    crusher_audio_module();
    void set_sample_rate(uint32_t sr);
    void activate();
    void deactivate();
    void params_changed();
    uint32_t process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask);
    bool get_graph(int index, int subindex, int phase, float *data, int points, cairo_iface *context, int *mode) const;
    bool get_layers(int index, int generation, unsigned int &layers) const;
    bool get_gridline(int index, int subindex, int phase, float &pos, bool &vertical, std::string &legend, cairo_iface *context) const;
};

};

#endif
