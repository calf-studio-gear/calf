/* Calf DSP plugin pack
 * Equalization related plugins
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
#ifndef CALF_MODULES_FILTER_H
#define CALF_MODULES_FILTER_H

#include <assert.h>
#include <limits.h>
#include "biquad.h"
#include "inertia.h"
#include "audio_fx.h"
#include "giface.h"
#include "metadata.h"
#include "plugin_tools.h"
#include "loudness.h"

namespace calf_plugins {

/**********************************************************************
 * EQUALIZER N BAND by Markus Schmidt and Krzysztof Foltman
**********************************************************************/

template<class BaseClass, bool has_lphp>
class equalizerNband_audio_module: public audio_module<BaseClass>, public frequency_response_line_graph {
public:
    typedef audio_module<BaseClass> AM;
    using AM::ins;
    using AM::outs;
    using AM::params;
    using AM::in_count;
    using AM::out_count;
    using AM::param_count;
    using AM::PeakBands;
private:
    enum { graph_param_count = BaseClass::last_graph_param - BaseClass::first_graph_param + 1, params_per_band = AM::param_p2_active - AM::param_p1_active };
    float hp_mode_old, hp_freq_old;
    float lp_mode_old, lp_freq_old;
    float ls_level_old, ls_freq_old;
    float hs_level_old, hs_freq_old;
    float p_level_old[PeakBands], p_freq_old[PeakBands], p_q_old[PeakBands];
    mutable float old_params_for_graph[graph_param_count];
    dual_in_out_metering<BaseClass> meters;
    CalfEqMode hp_mode, lp_mode;
    dsp::biquad_d2<float> hp[3][2], lp[3][2];
    dsp::biquad_d2<float> lsL, lsR, hsL, hsR;
    dsp::biquad_d2<float> pL[PeakBands], pR[PeakBands];
    int keep_gliding;
    
    inline void process_hplp(float &left, float &right);
public:
    typedef std::complex<double> cfloat;
    uint32_t srate;
    bool is_active;
    mutable volatile int last_generation, last_calculated_generation;
    equalizerNband_audio_module();
    void activate();
    void deactivate();

    void params_changed();
    bool get_gridline(int index, int subindex, int phase, float &pos, bool &vertical, std::string &legend, cairo_iface *context);
    bool get_graph(int index, int subindex, int phase, float *data, int points, cairo_iface *context, int *mode);
    float freq_gain(int index, double freq, uint32_t sr);
    void set_sample_rate(uint32_t sr)
    {
        srate = sr;
        meters.set_sample_rate(sr);
    }
    uint32_t process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask);
};

typedef equalizerNband_audio_module<equalizer5band_metadata,  false> equalizer5band_audio_module;
typedef equalizerNband_audio_module<equalizer8band_metadata,  true>  equalizer8band_audio_module;
typedef equalizerNband_audio_module<equalizer12band_metadata, true>  equalizer12band_audio_module;

/**********************************************************************
 * FILTER MODULE by Krzysztof Foltman
**********************************************************************/

template<typename FilterClass, typename Metadata>
class filter_module_with_inertia: public audio_module<Metadata>, public FilterClass
{
public:
    /// These are pointers to the ins, outs, params arrays in the main class
    typedef filter_module_with_inertia inertia_filter_module;
    using audio_module<Metadata>::ins;
    using audio_module<Metadata>::outs;
    using audio_module<Metadata>::params;
    
    dsp::inertia<dsp::exponential_ramp> inertia_cutoff, inertia_resonance, inertia_gain;
    dsp::once_per_n timer;
    bool is_active;    
    mutable volatile int last_generation, last_calculated_generation;
    
    filter_module_with_inertia(float **ins, float **outs, float **params)
    : inertia_cutoff(dsp::exponential_ramp(128), 20)
    , inertia_resonance(dsp::exponential_ramp(128), 20)
    , inertia_gain(dsp::exponential_ramp(128), 1.0)
    , timer(128)
    , is_active(false)
    , last_generation(-1)
    , last_calculated_generation(-2)
    {}
    
    void calculate_filter()
    {
        float freq = inertia_cutoff.get_last();
        // printf("freq=%g inr.cnt=%d timer.left=%d\n", freq, inertia_cutoff.count, timer.left);
        // XXXKF this is resonance of a single stage, obviously for three stages, resonant gain will be different
        float q    = inertia_resonance.get_last();
        int   mode = dsp::fastf2i_drm(*params[Metadata::par_mode]);
        // printf("freq = %f q = %f mode = %d\n", freq, q, mode);
        
        int inertia = dsp::fastf2i_drm(*params[Metadata::par_inertia]);
        if (inertia != inertia_cutoff.ramp.length()) {
            inertia_cutoff.ramp.set_length(inertia);
            inertia_resonance.ramp.set_length(inertia);
            inertia_gain.ramp.set_length(inertia);
        }
        
        FilterClass::calculate_filter(freq, q, mode, inertia_gain.get_last());
    }
    
    virtual void params_changed()
    {
        calculate_filter();
    }
    
    void on_timer()
    {
        int gen = last_generation;
        inertia_cutoff.step();
        inertia_resonance.step();
        inertia_gain.step();
        calculate_filter();
        last_calculated_generation = gen;
    }
    
    void activate()
    {
        params_changed();
        FilterClass::filter_activate();
        timer = dsp::once_per_n(FilterClass::srate / 1000);
        timer.start();
        is_active = true;
    }
    
    void set_sample_rate(uint32_t sr)
    {
        FilterClass::srate = sr;
    }

    
    void deactivate()
    {
        is_active = false;
    }

    uint32_t process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask) {
//        printf("sr=%d cutoff=%f res=%f mode=%f\n", FilterClass::srate, *params[Metadata::par_cutoff], *params[Metadata::par_resonance], *params[Metadata::par_mode]);
        uint32_t ostate = 0;
        numsamples += offset;
        while(offset < numsamples) {
            uint32_t numnow = numsamples - offset;
            // if inertia's inactive, we can calculate the whole buffer at once
            if (inertia_cutoff.active() || inertia_resonance.active() || inertia_gain.active())
                numnow = timer.get(numnow);
            
            if (outputs_mask & 1) {
                ostate |= FilterClass::process_channel(0, ins[0] + offset, outs[0] + offset, numnow, inputs_mask & 1);
            }
            if (outputs_mask & 2) {
                ostate |= FilterClass::process_channel(1, ins[1] + offset, outs[1] + offset, numnow, inputs_mask & 2);
            }
            
            if (timer.elapsed()) {
                on_timer();
            }
            offset += numnow;
        }
        return ostate;
    }
};

/**********************************************************************
 * FILTER by Krzysztof Foltman
**********************************************************************/

class filter_audio_module: 
    public filter_module_with_inertia<dsp::biquad_filter_module, filter_metadata>, 
    public frequency_response_line_graph
{
    mutable float old_cutoff, old_resonance, old_mode;
public:    
    filter_audio_module()
    : filter_module_with_inertia<dsp::biquad_filter_module, filter_metadata>(ins, outs, params)
    {
        last_generation = 0;
        old_mode = old_resonance = old_cutoff = -1;
        redraw_graph = true;
    }
    void params_changed()
    { 
        inertia_cutoff.set_inertia(*params[par_cutoff]);
        inertia_resonance.set_inertia(*params[par_resonance]);
        inertia_filter_module::params_changed();
        redraw_graph = true;
    }
};

/**********************************************************************
 * FILTERKLAVIER by Hans Baier 
**********************************************************************/

class filterclavier_audio_module: 
        public filter_module_with_inertia<dsp::biquad_filter_module, filterclavier_metadata>, 
        public frequency_response_line_graph
{        
    using audio_module<filterclavier_metadata>::ins;
    using audio_module<filterclavier_metadata>::outs;
    using audio_module<filterclavier_metadata>::params;

    const float min_gain;
    const float max_gain;
    
    int last_note;
    int last_velocity;
public:    
    filterclavier_audio_module();
    void params_changed();
    void activate();
    void set_sample_rate(uint32_t sr);
    void deactivate();
  
    /// MIDI control
    virtual void note_on(int channel, int note, int vel);
    virtual void note_off(int channel, int note, int vel);

private:
    void adjust_gain_according_to_filter_mode(int velocity);
};

/**********************************************************************
 * PHONO EQ by Damien Zamit 
**********************************************************************/

class phonoeq_audio_module: public audio_module<phonoeq_metadata>, public frequency_response_line_graph {
public:
    dual_in_out_metering<phonoeq_metadata> meters;
    dsp::riaacurve riaacurvL, riaacurvR;
    typedef std::complex<double> cfloat;
    uint32_t srate;
    bool is_active;
    phonoeq_audio_module();
    void activate();
    void deactivate();
    void params_changed();
    float freq_gain(int index, double freq, uint32_t sr) const;
    void set_sample_rate(uint32_t sr)
    {
        srate = sr;
        meters.set_sample_rate(sr);
    }
    uint32_t process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask);
};

/**********************************************************************
 * CROSSOVER MODULES by Markus Schmidt
**********************************************************************/

template<class XoverBaseClass>
class xover_audio_module: public audio_module<XoverBaseClass>, public frequency_response_line_graph {
public:
    typedef audio_module<XoverBaseClass> AM;
    using AM::ins;
    using AM::outs;
    using AM::params;
    using AM::in_count;
    using AM::out_count;
    using AM::param_count;
    using AM::bands;
    using AM::channels;
    enum { params_per_band = AM::param_level2 - AM::param_level1 };
    uint32_t srate;
    bool is_active;
    float * buffer;
    float in[channels];
    float meter[channels][bands];
    float meter_in[channels];
    unsigned int pos;
    unsigned int buffer_size;
    static inline float sign(float x) {
        if(x < 0) return -1.f;
        if(x > 0) return 1.f;
        return 0.f;
    }
    dsp::crossover crossover;
    xover_audio_module();
    void activate();
    void deactivate();
    void params_changed();
    void set_sample_rate(uint32_t sr);
    uint32_t process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask);
    bool get_graph(int index, int subindex, int phase, float *data, int points, cairo_iface *context, int *mode) const;
};

typedef xover_audio_module<xover2_metadata> xover2_audio_module;
typedef xover_audio_module<xover3_metadata> xover3_audio_module;
typedef xover_audio_module<xover4_metadata> xover4_audio_module;

};

#endif
