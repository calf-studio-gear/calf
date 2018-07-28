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
#include "analyzer.h"
#include "bypass.h"
#include "orfanidis_eq.h"

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
    analyzer _analyzer;
    enum { graph_param_count = BaseClass::last_graph_param - BaseClass::first_graph_param + 1, params_per_band = AM::param_p2_active - AM::param_p1_active };
    float hp_mode_old, hp_freq_old, hp_q_old;
    float lp_mode_old, lp_freq_old, lp_q_old;
    float ls_level_old, ls_freq_old, ls_q_old;
    float hs_level_old, hs_freq_old, hs_q_old;
    int indiv_old;
    bool analyzer_old;
    float p_level_old[PeakBands], p_freq_old[PeakBands], p_q_old[PeakBands];
    mutable float old_params_for_graph[graph_param_count];
    vumeters meters;
    CalfEqMode hp_mode, lp_mode;
    dsp::biquad_d2 hp[3][2], lp[3][2];
    dsp::biquad_d2 lsL, lsR, hsL, hsR;
    dsp::biquad_d2 pL[PeakBands], pR[PeakBands];
    dsp::bypass bypass;
    int keep_gliding;
    mutable int last_peak;
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
    bool get_gridline(int index, int subindex, int phase, float &pos, bool &vertical, std::string &legend, cairo_iface *context) const;
    bool get_graph(int index, int subindex, int phase, float *data, int points, cairo_iface *context, int *mode) const;
    bool get_layers(int index, int generation, unsigned int &layers) const;
    float freq_gain(int index, double freq) const;
    std::string get_crosshair_label(int x, int y, int sx, int sy, float q, int dB, int name, int note, int cents) const;

    void set_sample_rate(uint32_t sr)
    {
        srate = sr;
        _analyzer.set_sample_rate(sr);
        int meter[] = {AM::param_meter_inL, AM::param_meter_inR,  AM::param_meter_outL, AM::param_meter_outR};
        int clip[] = {AM::param_clip_inL, AM::param_clip_inR, AM::param_clip_outL, AM::param_clip_outR};
        meters.init(params, meter, clip, 4, sr);
    }
    uint32_t process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask);
};

typedef equalizerNband_audio_module<equalizer5band_metadata,  false> equalizer5band_audio_module;
typedef equalizerNband_audio_module<equalizer8band_metadata,  true>  equalizer8band_audio_module;
typedef equalizerNband_audio_module<equalizer12band_metadata, true>  equalizer12band_audio_module;

/**********************************************************************
 * EQUALIZER 30 BAND
**********************************************************************/

class equalizer30band_audio_module: public audio_module<equalizer30band_metadata> {
    OrfanidisEq::Conversions conv;
    OrfanidisEq::FrequencyGrid fg;
    std::vector<OrfanidisEq::Eq*> eq_arrL;
    std::vector<OrfanidisEq::Eq*> eq_arrR;

    OrfanidisEq::filter_type flt_type;
    OrfanidisEq::filter_type flt_type_old;

    dsp::switcher<OrfanidisEq::filter_type> swL;
    dsp::switcher<OrfanidisEq::filter_type> swR;

public:
    uint32_t srate;
    bool is_active;
    dsp::bypass bypass;
    dsp::bypass eq_switch;
    vumeters meters;
    equalizer30band_audio_module();
    ~equalizer30band_audio_module();

    void activate();
    void deactivate();
    void params_changed();
    void set_sample_rate(uint32_t sr);
    uint32_t process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask);
};

/**********************************************************************
 * FILTER MODULE by Krzysztof Foltman
**********************************************************************/

template<typename FilterClass, typename Metadata>
class filter_module_with_inertia: public audio_module<Metadata>, public FilterClass,
public frequency_response_line_graph
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
    
    dsp::bypass bypass;
    vumeters meters;

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
        int meter[] = {Metadata::param_meter_inL,  Metadata::param_meter_inR, Metadata::param_meter_outL, Metadata::param_meter_outR};
        int clip[]  = {Metadata::param_clip_inL, Metadata::param_clip_inR, Metadata::param_clip_outL, Metadata::param_clip_outR};
        meters.init(params, meter, clip, 4, sr);
    }

    
    void deactivate()
    {
        is_active = false;
    }

    uint32_t process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask) {
//        printf("sr=%d cutoff=%f res=%f mode=%f\n", FilterClass::srate, *params[Metadata::par_cutoff], *params[Metadata::par_resonance], *params[Metadata::par_mode]);
        uint32_t ostate = 0;
        uint32_t orig_offset = offset, orig_numsamples = numsamples;
        bool bypassed = bypass.update(*params[Metadata::param_bypass] > 0.5f, numsamples);
        if (bypassed) {
            float values[] = {0,0,0,0};
            for (uint32_t i = offset; i < offset + numsamples; i++) {
                outs[0][i] = ins[0][i];
                outs[1][i] = ins[1][i];
                //float values[] = {ins[0][i],ins[1][i],outs[0][i],outs[1][i]};
                meters.process(values);
                ostate = -1;
            }
        } else {
            numsamples += offset;
            while(offset < numsamples) {
                uint32_t numnow = numsamples - offset;
                // if inertia's inactive, we can calculate the whole buffer at once
                if (inertia_cutoff.active() || inertia_resonance.active() || inertia_gain.active())
                    numnow = timer.get(numnow);
                if (outputs_mask & 1) {
                    ostate |= FilterClass::process_channel(0, ins[0] + offset, outs[0] + offset, numnow, inputs_mask & 1, *params[Metadata::param_level_in], *params[Metadata::param_level_out]);
                }
                if (outputs_mask & 2) {
                    ostate |= FilterClass::process_channel(1, ins[1] + offset, outs[1] + offset, numnow, inputs_mask & 2, *params[Metadata::param_level_in], *params[Metadata::param_level_out]);
                }
                if (timer.elapsed()) {
                    on_timer();
                }
                for (uint32_t i = offset; i < offset + numnow; i++) {
                    float values[] = {ins[0][i] * *params[Metadata::param_level_in], ins[1][i] * *params[Metadata::param_level_in], outs[0][i], outs[1][i]};
                    meters.process(values);
                }
                offset += numnow;
            }
            bypass.crossfade(ins, outs, 2, orig_offset, orig_numsamples);
        }
        meters.fall(orig_numsamples);
        return ostate;
    }
    float freq_gain(int index, double freq) const {
        return FilterClass::freq_gain(index, (float)freq, (float)FilterClass::srate);
    }
};

/**********************************************************************
 * FILTER by Krzysztof Foltman
**********************************************************************/

class filter_audio_module: 
    public filter_module_with_inertia<dsp::biquad_filter_module, filter_metadata>
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
        public filter_module_with_inertia<dsp::biquad_filter_module, filterclavier_metadata>
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
 * EMPHASIS by Damien Zammit 
**********************************************************************/

class emphasis_audio_module: public audio_module<emphasis_metadata>, public frequency_response_line_graph {
public:
    dsp::riaacurve riaacurvL, riaacurvR;
    dsp::bypass bypass;
    int mode, type, bypass_;
    typedef std::complex<double> cfloat;
    uint32_t srate;
    bool is_active;
    vumeters meters;
    emphasis_audio_module();
    void activate();
    void deactivate();
    void params_changed();
    void set_sample_rate(uint32_t sr)
    {
        srate = sr;
        int meter[] = {param_meter_inL, param_meter_inR,  param_meter_outL, param_meter_outR};
        int clip[] = {param_clip_inL, param_clip_inR, param_clip_outL, param_clip_outR};
        meters.init(params, meter, clip, 4, sr);
    }
    virtual float freq_gain(int index, double freq) const {
        return riaacurvL.freq_gain(freq, (float)srate);
    }
    virtual bool get_graph(int index, int subindex, int phase, float *data, int points, cairo_iface *context, int *mode) const;
    virtual bool get_gridline(int index, int subindex, int phase, float &pos, bool &vertical, std::string &legend, cairo_iface *context) const;
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
    unsigned int pos;
    unsigned int buffer_size;
    int last_peak;
    static inline float sign(float x) {
        if(x < 0) return -1.f;
        if(x > 0) return 1.f;
        return 0.f;
    }
    vumeters meters;
    dsp::crossover crossover;
    xover_audio_module();
    ~xover_audio_module();
    void activate();
    void deactivate();
    void params_changed();
    void set_sample_rate(uint32_t sr);
    uint32_t process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask);
    bool get_graph(int index, int subindex, int phase, float *data, int points, cairo_iface *context, int *mode) const;
    bool get_layers(int index, int generation, unsigned int &layers) const;
};

typedef xover_audio_module<xover2_metadata> xover2_audio_module;
typedef xover_audio_module<xover3_metadata> xover3_audio_module;
typedef xover_audio_module<xover4_metadata> xover4_audio_module;

/**********************************************************************
 * VOCODER by Markus Schmidt & Christian Holschuh
**********************************************************************/

class vocoder_audio_module: public audio_module<vocoder_metadata>, public frequency_response_line_graph {
public:
    int bands, bands_old, order, hiq_old;
    float order_old, lower_old, upper_old, tilt_old;
    float q_old[32];
    float bandfreq[32];
    uint32_t srate;
    bool is_active;
    static const int maxorder = 8;
    dsp::biquad_d2 detector[2][maxorder][32], modulator[2][maxorder][32];
    dsp::bypass bypass;
    double env_mods[2][32];
    vumeters meters;
    analyzer _analyzer;
    double attack, release, fcoeff, log2_;
    vocoder_audio_module();
    void activate();
    void deactivate();
    void params_changed();
    int get_solo() const;
    void set_sample_rate(uint32_t sr)
    {
        srate = sr;
        _analyzer.set_sample_rate(sr);
        int meter[] = {param_carrier_inL, param_carrier_inR,  param_mod_inL, param_mod_inR, param_outL, param_outR};
        int clip[] = {param_carrier_clip_inL, param_carrier_clip_inR, param_mod_clip_inL, param_mod_clip_inR, param_clip_outL, param_clip_outR};
        meters.init(params, meter, clip, 6, sr);
    }
    virtual bool get_graph(int index, int subindex, int phase, float *data, int points, cairo_iface *context, int *mode) const;
    virtual bool get_layers(int index, int generation, unsigned int &layers) const;
    uint32_t process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask);
};

/**********************************************************************
 * ENVELOPE FILTER by Markus Schmidt
**********************************************************************/

class envelopefilter_audio_module: public audio_module<envelopefilter_metadata>, public dsp::biquad_filter_module,
public frequency_response_line_graph
{
public:
    uint32_t srate;
    bool is_active;
    
    dsp::bypass bypass;
    vumeters meters;
    
    float envelope, attack, release, envelope_old, attack_old, release_old, q_old;
    float gain, gain_old, upper, upper_old, lower, lower_old;
    float coefa, coefb, coefz;
    int mode, mode_old;
    
    envelopefilter_audio_module()
    {
        envelope = envelope_old = 0;
        lower = 10; lower_old = 0;
        upper = 10; upper_old = 0;
        gain  = 1;  gain_old  = 0;
        attack = release = attack_old = release_old = -1;
        mode = mode_old = q_old = 0;
        coefa = coefb = 0;
        coefz = 2;
    }
    
    void activate()
    {
        params_changed();
        filter_activate();
        is_active = true;
    }
    
    void deactivate()
    {
        is_active = false;
    }
    
    void set_sample_rate(uint32_t sr)
    {
        srate = sr;
        dsp::biquad_filter_module::srate = sr;
        int meter[] = {param_meter_inL, param_meter_inR, param_meter_outL, param_meter_outR};
        int clip[] = {param_clip_inL, param_clip_inR, param_clip_outL, param_clip_outR};
        meters.init(params, meter, clip, 4, sr);
    }
    
    void params_changed()
    {
        if (*params[param_attack] != attack_old) {
            attack_old = *params[param_attack];
            attack     = exp(log(0.01)/( attack_old * srate * 0.001));
        }
        if (*params[param_release] != release_old) {
            release_old = *params[param_release];
            release     = exp(log(0.01)/( release_old * srate * 0.001));
        }
        if (*params[param_mode] != mode_old) {
            mode = dsp::fastf2i_drm(*params[param_mode]);
            mode_old = *params[param_mode];
            calc_filter();
        }
        if (*params[param_q] != q_old) {
            q_old = *params[param_q];
            calc_filter();
        }
        if (*params[param_upper] != upper_old) {
            upper = *params[param_upper];
            upper_old = *params[param_upper];
            calc_coef();
            calc_filter();
        }
        if (*params[param_lower] != lower_old) {
            lower = *params[param_lower];
            lower_old = *params[param_lower];
            calc_coef();
            calc_filter();
        }
        if (*params[param_gain] != gain_old) {
            gain = *params[param_gain];
            gain_old = *params[param_gain];
            calc_filter();
        }
    }
    
    uint32_t process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask)
    {
        bool bypassed = bypass.update(*params[param_bypass] > 0.5f, numsamples);
        uint32_t end = numsamples + offset;
        while(offset < end) {
            float D;
            if (*params[param_sidechain] > 0.5)
                D = std::max(fabs(ins[2][offset]), fabs(ins[3][offset])) * *params[param_gain];
            else
                D = std::max(fabs(ins[0][offset]), fabs(ins[1][offset])) * *params[param_gain];
                
            // advance envelope
            envelope = std::min(1.f, (D > envelope ? attack : release) * (envelope - D) + D);
            if (envelope != envelope_old) {
                envelope_old = envelope;
                redraw_graph = true;
                dsp::biquad_filter_module::calculate_filter(get_freq(envelope), *params[param_q], mode, 1.0);
            }
                
            if(bypassed) {
                outs[0][offset]  = ins[0][offset];
                outs[1][offset]  = ins[1][offset];
                float values[] = {0, 0, 0, 0};
                meters.process(values);
            } else {
                const float inL  = ins[0][offset] * *params[param_level_in];
                const float inR  = ins[1][offset] * *params[param_level_in];
                float outL = outs[0][offset];
                float outR = outs[1][offset];
                
                // process filters
                dsp::biquad_filter_module::process_channel(0, &inL, &outL, 1, inputs_mask & 1);
                dsp::biquad_filter_module::process_channel(1, &inR, &outR, 1, inputs_mask & 2);
                
                // mix and out level
                outs[0][offset] = (outL * *params[param_mix] + inL * (*params[param_mix] * -1 + 1)) * *params[param_level_out];
                outs[1][offset] = (outR * *params[param_mix] + inR * (*params[param_mix] * -1 + 1)) * *params[param_level_out];
                
                // meters
                float values[] = {inL, inR, outs[0][offset], outs[1][offset]};
                meters.process(values);
            }
            // step on
            offset += 1;
        }
        if (bypassed)
            bypass.crossfade(ins, outs, 2, offset - numsamples, numsamples);
        meters.fall(numsamples);
        return outputs_mask;
    }
    
    float freq_gain(int index, double freq) const {
        return dsp::biquad_filter_module::freq_gain(index, (float)freq, (float)srate);
    }
    
    void calc_filter ()
    {
        redraw_graph = true;
        dsp::biquad_filter_module::calculate_filter(get_freq(envelope), *params[param_q], mode, 1.0);
    }
    
    void calc_coef ()
    {
        coefa = log10(upper) - log10(lower);
        coefb = log10(lower);
    }
    
    float get_freq(float envelope) const {
        float diff = upper - lower;
        float env  = pow(envelope, pow(2, *params[param_response] * -2));
        float freq = pow(10, coefa * env + coefb);
        if (diff < 0)
            return  std::max(upper, std::min(lower, freq));
        return std::min(upper, std::max(lower, freq));
    }
    
};

};

#endif
