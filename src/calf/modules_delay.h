/* Calf DSP plugin pack
 * Modulation effect plugins
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
 * Boston, MA  02110-1301  USA
 */
 
#ifndef CALF_MODULES_DELAY_H
#define CALF_MODULES_DELAY_H

#include <assert.h>
#include <limits.h>
#include "biquad.h"
#include "bypass.h"
#include "inertia.h"
#include "audio_fx.h"
#include "giface.h"
#include "metadata.h"
#include "loudness.h"
#include <math.h>
#include "plugin_tools.h"

namespace calf_plugins {

struct ladspa_plugin_info;

/**********************************************************************
 * REVERB by Krzysztof Foltman
**********************************************************************/

class reverb_audio_module: public audio_module<reverb_metadata>
{
    vumeters meters;
public:    
    dsp::reverb reverb;
    dsp::simple_delay<131072, dsp::stereo_sample<float> > pre_delay;
    dsp::onepole<float> left_lo, right_lo, left_hi, right_hi;
    uint32_t srate;
    dsp::gain_smoothing amount, dryamount;
    int predelay_amt;
    
    void params_changed();
    uint32_t process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask);
    void activate();
    void set_sample_rate(uint32_t sr);
    void deactivate();
};

/**********************************************************************
 * VINTAGE DELAY by Krzysztof Foltman
**********************************************************************/

class vintage_delay_audio_module: public audio_module<vintage_delay_metadata>, public frequency_response_line_graph
{
public:    
    // 1MB of delay memory per channel... uh, RAM is cheap
    enum { MAX_DELAY = 524288, ADDR_MASK = MAX_DELAY - 1 };
    enum { MIXMODE_STEREO, MIXMODE_PINGPONG, MIXMODE_LR, MIXMODE_RL }; 
    enum { FRAG_PERIODIC, FRAG_PATTERN };
    float buffers[2][MAX_DELAY];
    int bufptr, deltime_l, deltime_r, mixmode, medium, old_medium;
    /// number of table entries written (value is only important when it is less than MAX_DELAY, which means that the buffer hasn't been totally filled yet)
    int age;
    
    dsp::gain_smoothing amt_left, amt_right, fb_left, fb_right, dry, chmix;
    
    dsp::biquad_d2 biquad_left[2], biquad_right[2];
    
    uint32_t srate;
    
    vintage_delay_audio_module();
    
    void params_changed();
    void activate();
    void deactivate();
    void set_sample_rate(uint32_t sr);
    void calc_filters();
    uint32_t process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask);
    virtual char *configure(const char *key, const char *value);
    
    long _tap_avg;
    long _tap_last;
    
    vumeters meters;
};

/**********************************************************************
 * COMPENSATION DELAY LINE by Vladimir Sadovnikov 
**********************************************************************/

// The maximum distance for knobs
#define COMP_DELAY_MAX_DISTANCE            (100.0 * 100.0 + 100.0 * 1.0 + 1.0)
// The actual speed of sound in normal conditions
#define COMP_DELAY_SOUND_SPEED_KM_H(temp)  1.85325 * (643.95 * std::pow(((temp + 273.15) / 273.15), 0.5))
#define COMP_DELAY_SOUND_SPEED_CM_S(temp)  (COMP_DELAY_SOUND_SPEED_KM_H(temp) * (1000.0 * 100.0) /* cm/km */ / (60.0 * 60.0) /* s/h */)
#define COMP_DELAY_SOUND_FRONT_DELAY(temp) (1.0 / COMP_DELAY_SOUND_SPEED_CM_S(temp))
// The maximum delay may be reached by this plugin
#define COMP_DELAY_MAX_DELAY               (COMP_DELAY_MAX_DISTANCE*COMP_DELAY_SOUND_FRONT_DELAY(50))

class comp_delay_audio_module: public audio_module<comp_delay_metadata>
{
public:
    float *buffer;
    uint32_t srate;
    uint32_t buf_size; // guaranteed to be power of 2
    uint32_t delay;
    uint32_t write_ptr;
    dsp::bypass bypass;
    vumeters meters;

    comp_delay_audio_module();
    virtual ~comp_delay_audio_module();

    void params_changed();
    void activate();
    void deactivate();
    void set_sample_rate(uint32_t sr);
    uint32_t process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask);
};

/**********************************************************************
 * HAAS enhancer by Vladimir Sadovnikov 
**********************************************************************/
#define HAAS_ENHANCER_MAX_DELAY            (10 * 0.001) /* 10 MSec */

class haas_enhancer_audio_module: public audio_module<haas_enhancer_metadata>
{
public:
    float *buffer;
    uint32_t srate;
    uint32_t buf_size; // guaranteed to be power of 2
    uint32_t write_ptr;
    
    dsp::bypass bypass;
    vumeters meters;
    
    uint32_t m_source, s_delay[2];
    float s_bal_l[2], s_bal_r[2];

    haas_enhancer_audio_module();
    virtual ~haas_enhancer_audio_module();

    void params_changed();
    void activate();
    void deactivate();
    void set_sample_rate(uint32_t sr);
    uint32_t process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask);
};

/**********************************************************************
 * REVERSE DELAY
**********************************************************************/

class reverse_delay_audio_module: public audio_module<reverse_delay_metadata>
{
public:
    enum { MAX_DELAY = 6144000, ADDR_MASK = MAX_DELAY - 1 };
    float buffers[2][MAX_DELAY];
    int counters[2];
    dsp::overlap_window ow[2];
    int deltime_l, deltime_r;
    
    dsp::bypass bypass;
    vumeters meters;

    dsp::gain_smoothing fb_val, dry, width;

    float feedback_buf[2];

    uint32_t srate;

    uint32_t line_state_old;

    reverse_delay_audio_module();

    void params_changed();
    void activate();
    void deactivate();
    void set_sample_rate(uint32_t sr);
    uint32_t process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask);
};

};
#endif
