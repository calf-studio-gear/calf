/* Calf DSP Library
 * Example audio modules
 *
 * Copyright (C) 2001-2007 Krzysztof Foltman
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
#ifndef __CALF_MODULES_H
#define __CALF_MODULES_H

#include <assert.h>
#include <limits.h>
#include "biquad.h"
#include "inertia.h"
#include "audio_fx.h"
#include "multichorus.h"
#include "giface.h"
#include "metadata.h"
#include "loudness.h"
#include "primitives.h"

namespace calf_plugins {

using namespace dsp;

struct ladspa_plugin_info;

#if 0
class amp_audio_module: public null_audio_module
{
public:
    enum { in_count = 2, out_count = 2, param_count = 1, support_midi = false, require_midi = false, rt_capable = true };
    float *ins[2]; 
    float *outs[2];
    float *params[1];
    uint32_t srate;
    static parameter_properties param_props[];
    uint32_t process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask) {
        if (!inputs_mask)
            return 0;
        float gain = *params[0];
        numsamples += offset;
        for (uint32_t i = offset; i < numsamples; i++) {
            outs[0][i] = ins[0][i] * gain;
            outs[1][i] = ins[1][i] * gain;
        }
        return inputs_mask;
    }
};
#endif

class frequency_response_line_graph: public line_graph_iface 
{
public:
    bool get_gridline(int index, int subindex, float &pos, bool &vertical, std::string &legend, cairo_iface *context) const;
    virtual int get_changed_offsets(int index, int generation, int &subindex_graph, int &subindex_dot, int &subindex_gridline) const;
};

class flanger_audio_module: public audio_module<flanger_metadata>, public frequency_response_line_graph
{
public:
    dsp::simple_flanger<float, 2048> left, right;
    float *ins[in_count]; 
    float *outs[out_count];
    float *params[param_count];
    uint32_t srate;
    bool clear_reset;
    float last_r_phase;
    bool is_active;
public:
    flanger_audio_module() {
        is_active = false;
    }
    void set_sample_rate(uint32_t sr);
    void params_changed();
    void params_reset();
    void activate();
    void deactivate();
    uint32_t process(uint32_t offset, uint32_t nsamples, uint32_t inputs_mask, uint32_t outputs_mask) {
        left.process(outs[0] + offset, ins[0] + offset, nsamples);
        right.process(outs[1] + offset, ins[1] + offset, nsamples);
        return outputs_mask; // XXXKF allow some delay after input going blank
    }
    bool get_graph(int index, int subindex, float *data, int points, cairo_iface *context) const;
    float freq_gain(int subindex, float freq, float srate) const;
};

class phaser_audio_module: public audio_module<phaser_metadata>, public frequency_response_line_graph
{
public:
    float *ins[in_count]; 
    float *outs[out_count];
    float *params[param_count];
    uint32_t srate;
    bool clear_reset;
    float last_r_phase;
    dsp::simple_phaser<12> left, right;
    bool is_active;
public:
    phaser_audio_module() {
        is_active = false;
    }
    void params_changed();
    void params_reset();
    void activate();
    void set_sample_rate(uint32_t sr);
    void deactivate();
    uint32_t process(uint32_t offset, uint32_t nsamples, uint32_t inputs_mask, uint32_t outputs_mask) {
        left.process(outs[0] + offset, ins[0] + offset, nsamples);
        right.process(outs[1] + offset, ins[1] + offset, nsamples);
        return outputs_mask; // XXXKF allow some delay after input going blank
    }
    bool get_graph(int index, int subindex, float *data, int points, cairo_iface *context) const;
    bool get_gridline(int index, int subindex, float &pos, bool &vertical, std::string &legend, cairo_iface *context) const;
    float freq_gain(int subindex, float freq, float srate) const;
};

class reverb_audio_module: public audio_module<reverb_metadata>
{
public:    
    dsp::reverb<float> reverb;
    dsp::simple_delay<16384, stereo_sample<float> > pre_delay;
    dsp::onepole<float> left_lo, right_lo, left_hi, right_hi;
    uint32_t srate;
    gain_smoothing amount, dryamount;
    int predelay_amt;
    float meter_wet, meter_out;
    uint32_t clip;
    float *ins[in_count]; 
    float *outs[out_count];
    float *params[param_count];
    
    void params_changed();
    uint32_t process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask);
    void activate();
    void set_sample_rate(uint32_t sr);
    void deactivate();
};

class vintage_delay_audio_module: public audio_module<vintage_delay_metadata>
{
public:    
    // 1MB of delay memory per channel... uh, RAM is cheap
    enum { MAX_DELAY = 262144, ADDR_MASK = MAX_DELAY - 1 };
    float *ins[in_count]; 
    float *outs[out_count];
    float *params[param_count];
    float buffers[2][MAX_DELAY];
    int bufptr, deltime_l, deltime_r, mixmode, medium, old_medium;
    /// number of table entries written (value is only important when it is less than MAX_DELAY, which means that the buffer hasn't been totally filled yet)
    int age;
    
    gain_smoothing amt_left, amt_right, fb_left, fb_right;
    float dry;
    
    dsp::biquad_d2<float> biquad_left[2], biquad_right[2];
    
    uint32_t srate;
    
    vintage_delay_audio_module();
    
    void params_changed();
    void activate();
    void deactivate();
    void set_sample_rate(uint32_t sr);
    void calc_filters();
    uint32_t process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask);
};

class rotary_speaker_audio_module: public audio_module<rotary_speaker_metadata>
{
public:
    float *ins[in_count]; 
    float *outs[out_count];
    float *params[param_count];
    /// Current phases and phase deltas for bass and treble rotors
    uint32_t phase_l, dphase_l, phase_h, dphase_h;
    dsp::simple_delay<1024, float> delay;
    dsp::biquad_d2<float> crossover1l, crossover1r, crossover2l, crossover2r;
    dsp::simple_delay<8, float> phaseshift;
    uint32_t srate;
    int vibrato_mode;
    /// Current CC1 (Modulation) value, normalized to [0, 1]
    float mwhl_value;
    /// Current CC64 (Hold) value, normalized to [0, 1]
    float hold_value;
    /// Current rotation speed for bass rotor - automatic mode
    float aspeed_l;
    /// Current rotation speed for treble rotor - automatic mode
    float aspeed_h;
    /// Desired speed (0=slow, 1=fast) - automatic mode
    float dspeed;
    /// Current rotation speed for bass rotor - manual mode
    float maspeed_l;
    /// Current rotation speed for treble rotor - manual mode
    float maspeed_h;
    
    int meter_l, meter_h;
    
    rotary_speaker_audio_module();
    void set_sample_rate(uint32_t sr);
    void setup();
    void activate();
    void deactivate();
    
    void params_changed();
    void set_vibrato();
    /// Convert RPM speed to delta-phase
    uint32_t rpm2dphase(float rpm);
    /// Set delta-phase variables based on current calculated (and interpolated) RPM speed
    void update_speed();
    void update_speed_manual(float delta);
    /// Increase or decrease aspeed towards raspeed, with required negative and positive rate
    bool incr_towards(float &aspeed, float raspeed, float delta_decc, float delta_acc);
    uint32_t process(uint32_t offset, uint32_t nsamples, uint32_t inputs_mask, uint32_t outputs_mask);
    virtual void control_change(int ctl, int val);
};

template<typename FilterClass, typename Metadata>
class filter_module_with_inertia: public FilterClass
{
public:
    typedef filter_module_with_inertia inertia_filter_module;
    
    float *ins[Metadata::in_count]; 
    float *outs[Metadata::out_count];
    float *params[Metadata::param_count];

    inertia<exponential_ramp> inertia_cutoff, inertia_resonance, inertia_gain;
    once_per_n timer;
    bool is_active;    
    mutable volatile int last_generation, last_calculated_generation;
    
    filter_module_with_inertia()
    : inertia_cutoff(exponential_ramp(128), 20)
    , inertia_resonance(exponential_ramp(128), 20)
    , inertia_gain(exponential_ramp(128), 1.0)
    , timer(128)
    {
        is_active = false;
    }
    
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
        timer = once_per_n(FilterClass::srate / 1000);
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

/// biquad filter module
class filter_audio_module: 
    public audio_module<filter_metadata>, 
    public filter_module_with_inertia<biquad_filter_module, filter_metadata>, 
    public frequency_response_line_graph
{
    mutable float old_cutoff, old_resonance, old_mode;
public:    
    filter_audio_module()
    {
        last_generation = 0;
    }
    void params_changed()
    { 
        inertia_cutoff.set_inertia(*params[par_cutoff]);
        inertia_resonance.set_inertia(*params[par_resonance]);
        inertia_filter_module::params_changed(); 
    }
        
    void activate()
    {
        inertia_filter_module::activate();
    }
    
    void set_sample_rate(uint32_t sr)
    {
        inertia_filter_module::set_sample_rate(sr);
    }

    
    void deactivate()
    {
        inertia_filter_module::deactivate();
    }
    
    bool get_graph(int index, int subindex, float *data, int points, cairo_iface *context) const;
    int get_changed_offsets(int index, int generation, int &subindex_graph, int &subindex_dot, int &subindex_gridline) const;
};

/// A multitap stereo chorus thing - processing
class multichorus_audio_module: public audio_module<multichorus_metadata>, public frequency_response_line_graph
{
public:
    float *ins[in_count]; 
    float *outs[out_count];
    float *params[param_count];
    uint32_t srate;
    dsp::multichorus<float, sine_multi_lfo<float, 8>, filter_sum<dsp::biquad_d2<>, dsp::biquad_d2<> >, 4096> left, right;
    float last_r_phase;
    float cutoff;
    bool is_active;
    
public:    
    multichorus_audio_module();
    void params_changed();
    uint32_t process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask);
    void activate();
    void deactivate();
    void set_sample_rate(uint32_t sr);
    bool get_graph(int index, int subindex, float *data, int points, cairo_iface *context) const;
    float freq_gain(int subindex, float freq, float srate) const;
    bool get_dot(int index, int subindex, float &x, float &y, int &size, cairo_iface *context) const;
    bool get_gridline(int index, int subindex, float &pos, bool &vertical, std::string &legend, cairo_iface *context) const;
};

/// Not a true _audio_module style class, just pretends to be one!
class gain_reduction_audio_module
{
private:
    float linSlope, detected, kneeSqrt, kneeStart, linKneeStart, kneeStop;
    float compressedKneeStop, adjKneeStart, thres;
    float attack, release, threshold, ratio, knee, makeup, detection, stereo_link, bypass, mute, meter_out, meter_comp;
    mutable float old_threshold, old_ratio, old_knee, old_makeup, old_bypass, old_mute, old_detection, old_stereo_link;
    mutable volatile int last_generation;
    uint32_t srate;
    bool is_active;
    inline float output_level(float slope) const;
    inline float output_gain(float linSlope, bool rms) const;
public:
    gain_reduction_audio_module();
    void set_params(float att, float rel, float thr, float rat, float kn, float mak, float det, float stl, float byp, float mu);
    void update_curve();
    void process(float &left, float &right, const float *det_left = NULL, const float *det_right = NULL);
    void activate();
    void deactivate();
    int id;
    void set_sample_rate(uint32_t sr);
    float get_output_level();
    float get_comp_level();
    bool get_graph(int subindex, float *data, int points, cairo_iface *context) const;
    bool get_dot(int subindex, float &x, float &y, int &size, cairo_iface *context) const;
    bool get_gridline(int subindex, float &pos, bool &vertical, std::string &legend, cairo_iface *context) const;
    int  get_changed_offsets(int generation, int &subindex_graph, int &subindex_dot, int &subindex_gridline) const;
};

/// Compressor by Thor
class compressor_audio_module: public audio_module<compressor_metadata>, public line_graph_iface  {
private:
    typedef compressor_audio_module AM;
    uint32_t clip_in, clip_out;
    float meter_in, meter_out;
    gain_reduction_audio_module compressor;
public:
    typedef std::complex<double> cfloat;
    float *ins[in_count];
    float *outs[out_count];
    float *params[param_count];
    uint32_t srate;
    bool is_active;
    mutable volatile int last_generation, last_calculated_generation;
    compressor_audio_module();
    void activate();
    void deactivate();
    void params_changed();
    void set_sample_rate(uint32_t sr);
    uint32_t process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask);
    bool get_graph(int index, int subindex, float *data, int points, cairo_iface *context) const;
    bool get_dot(int index, int subindex, float &x, float &y, int &size, cairo_iface *context) const;
    bool get_gridline(int index, int subindex, float &pos, bool &vertical, std::string &legend, cairo_iface *context) const;
    int  get_changed_offsets(int index, int generation, int &subindex_graph, int &subindex_dot, int &subindex_gridline) const;
};

/// Sidecain Compressor by Markus Schmidt (based on Thor's compressor and Krzysztof's filters)
class sidechaincompressor_audio_module: public audio_module<sidechaincompressor_metadata>, public frequency_response_line_graph  {
private:
    typedef sidechaincompressor_audio_module AM;
    enum CalfScModes {
        WIDEBAND,
        DEESSER_WIDE,
        DEESSER_SPLIT,
        DERUMBLER_WIDE,
        DERUMBLER_SPLIT,
        WEIGHTED_1,
        WEIGHTED_2,
        WEIGHTED_3,
        BANDPASS_1,
        BANDPASS_2
    };
    mutable float f1_freq_old, f2_freq_old, f1_level_old, f2_level_old;
    mutable float f1_freq_old1, f2_freq_old1, f1_level_old1, f2_level_old1;
    CalfScModes sc_mode;
    mutable CalfScModes sc_mode_old, sc_mode_old1;
    float f1_active, f2_active;
    uint32_t clip_in, clip_out;
    float meter_in, meter_out;
    gain_reduction_audio_module compressor;
    biquad_d2<float> f1L, f1R, f2L, f2R;
public:
    typedef std::complex<double> cfloat;
    float *ins[in_count];
    float *outs[out_count];
    float *params[param_count];
    uint32_t srate;
    bool is_active;
    mutable volatile int last_generation, last_calculated_generation;
    sidechaincompressor_audio_module();
    void activate();
    void deactivate();
    void params_changed();
    cfloat h_z(const cfloat &z) const;
    float freq_gain(int index, double freq, uint32_t sr) const;
    void set_sample_rate(uint32_t sr);
    uint32_t process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask);
    bool get_graph(int index, int subindex, float *data, int points, cairo_iface *context) const;
    bool get_dot(int index, int subindex, float &x, float &y, int &size, cairo_iface *context) const;
    bool get_gridline(int index, int subindex, float &pos, bool &vertical, std::string &legend, cairo_iface *context) const;
    int  get_changed_offsets(int index, int generation, int &subindex_graph, int &subindex_dot, int &subindex_gridline) const;
};

/// Multibandcompressor by Markus Schmidt
class multibandcompressor_audio_module: public audio_module<multibandcompressor_metadata>, public line_graph_iface {
private:
    typedef multibandcompressor_audio_module AM;
    static const int strips = 4;
    bool mute[strips];
    uint32_t clip_inL, clip_inR, clip_outL, clip_outR;
    float meter_inL, meter_inR, meter_outL, meter_outR;
    gain_reduction_audio_module strip[strips];
    dsp::biquad_d2<float> lpL0, lpR0, lpL1, lpR1, lpL2, lpR2, hpL0, hpR0, hpL1, hpR1, hpL2, hpR2;
    float freq_old[strips - 1], sep_old[strips - 1], q_old[strips - 1];
public:
    float *ins[in_count];
    float *outs[out_count];
    float *params[param_count];
    uint32_t srate;
    bool is_active;
    multibandcompressor_audio_module();
    void activate();
    void deactivate();
    void params_changed();
    uint32_t process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask);
    void set_sample_rate(uint32_t sr);
    const gain_reduction_audio_module *get_strip_by_param_index(int index) const;
    virtual bool get_graph(int index, int subindex, float *data, int points, cairo_iface *context) const;
    virtual bool get_dot(int index, int subindex, float &x, float &y, int &size, cairo_iface *context) const;
    virtual bool get_gridline(int index, int subindex, float &pos, bool &vertical, std::string &legend, cairo_iface *context) const;
    virtual int  get_changed_offsets(int index, int generation, int &subindex_graph, int &subindex_dot, int &subindex_gridline) const;
};

/// Deesser by Markus Schmidt (based on Thor's compressor and Krzysztof's filters)
class deesser_audio_module: public audio_module<deesser_metadata>, public frequency_response_line_graph  {
private:
    enum CalfDeessModes {
        WIDE,
        SPLIT
    };
    mutable float f1_freq_old, f2_freq_old, f1_level_old, f2_level_old, f2_q_old;
    mutable float f1_freq_old1, f2_freq_old1, f1_level_old1, f2_level_old1, f2_q_old1;
    uint32_t detected_led;
    float detected, clip_out;
    uint32_t clip_led;
    gain_reduction_audio_module compressor;
    biquad_d2<float> hpL, hpR, lpL, lpR, pL, pR;
public:
    float *ins[in_count];
    float *outs[out_count];
    float *params[param_count];
    uint32_t srate;
    bool is_active;
    mutable volatile int last_generation, last_calculated_generation;
    deesser_audio_module();
    void activate();
    void deactivate();
    void params_changed();
    float freq_gain(int index, double freq, uint32_t sr) const
    {
        return hpL.freq_gain(freq, sr) * pL.freq_gain(freq, sr);
    }
    void set_sample_rate(uint32_t sr);
    uint32_t process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask);
    bool get_graph(int index, int subindex, float *data, int points, cairo_iface *context) const;
    bool get_gridline(int index, int subindex, float &pos, bool &vertical, std::string &legend, cairo_iface *context) const;
    int  get_changed_offsets(int index, int generation, int &subindex_graph, int &subindex_dot, int &subindex_gridline) const;
};

/// Equalizer N Band by Markus Schmidt (based on Krzysztof's filters)
template<class BaseClass, bool has_lphp>
class equalizerNband_audio_module: public audio_module<BaseClass>, public frequency_response_line_graph {
public:
    typedef audio_module<BaseClass> AM;
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
    uint32_t clip_inL, clip_outL, clip_inR, clip_outR;
    float meter_inL, meter_outL, meter_inR, meter_outR;
    CalfEqMode hp_mode, lp_mode;
    biquad_d2<float> hp[3][2], lp[3][2];
    biquad_d2<float> lsL, lsR, hsL, hsR;
    biquad_d2<float> pL[PeakBands], pR[PeakBands];
    
    inline void process_hplp(float &left, float &right);
public:
    typedef std::complex<double> cfloat;
    float *ins[in_count];
    float *outs[out_count];
    float *params[param_count];
    uint32_t srate;
    bool is_active;
    mutable volatile int last_generation, last_calculated_generation;
    equalizerNband_audio_module();
    void activate();
    void deactivate();

    void params_changed();
    float freq_gain(int index, double freq, uint32_t sr) const;
    void set_sample_rate(uint32_t sr)
    {
        srate = sr;
    }
    uint32_t process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask);
    bool get_graph(int index, int subindex, float *data, int points, cairo_iface *context) const;
    bool get_gridline(int index, int subindex, float &pos, bool &vertical, std::string &legend, cairo_iface *context) const;
    int  get_changed_offsets(int index, int generation, int &subindex_graph, int &subindex_dot, int &subindex_gridline) const;
};

typedef equalizerNband_audio_module<equalizer5band_metadata, false> equalizer5band_audio_module;
typedef equalizerNband_audio_module<equalizer8band_metadata, true> equalizer8band_audio_module;
typedef equalizerNband_audio_module<equalizer12band_metadata, true> equalizer12band_audio_module;

/// LFO by Markus
class lfo_audio_module {
private:
    float phase, freq, offset, amount;
    int mode;
    uint32_t srate;
    bool is_active;
public:
    lfo_audio_module();
    void set_params(float f, int m, float o, uint32_t sr, float amount = 1.f);
    float get_value();
    void advance(uint32_t count);
    void set_phase(float ph);
    void activate();
    void deactivate();
    float get_value_from_phase(float ph, float off) const;
    virtual bool get_graph(float *data, int points, cairo_iface *context) const;
    virtual bool get_dot(float &x, float &y, int &size, cairo_iface *context) const;
};

/// Pulsator by Markus Schmidt
class pulsator_audio_module: public audio_module<pulsator_metadata>, public frequency_response_line_graph  {
private:
    typedef pulsator_audio_module AM;
    uint32_t clip_inL, clip_inR, clip_outL, clip_outR;
    float meter_inL, meter_inR, meter_outL, meter_outR;
    float offset_old;
    int mode_old;
    bool clear_reset;
    lfo_audio_module lfoL, lfoR;
public:
    float *ins[in_count];
    float *outs[out_count];
    float *params[param_count];
    uint32_t srate;
    bool is_active;
    pulsator_audio_module();
    void activate();
    void deactivate();
    void params_changed();
    void set_sample_rate(uint32_t sr);
    void params_reset()
    {
        if (clear_reset) {
            *params[param_reset] = 0.f;
            clear_reset = false;
        }
    }
    uint32_t process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask);
    bool get_graph(int index, int subindex, float *data, int points, cairo_iface *context) const;
    bool get_dot(int index, int subindex, float &x, float &y, int &size, cairo_iface *context) const;
    bool get_gridline(int index, int subindex, float &pos, bool &vertical, std::string &legend, cairo_iface *context) const;
};

/// Filterclavier --- MIDI controlled filter by Hans Baier
class filterclavier_audio_module: 
        public audio_module<filterclavier_metadata>, 
        public filter_module_with_inertia<biquad_filter_module, filterclavier_metadata>, 
        public frequency_response_line_graph
{        
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
    virtual void note_on(int note, int vel);
    virtual void note_off(int note, int vel);
    
    bool get_graph(int index, int subindex, float *data, int points, cairo_iface *context) const;
    
private:
    void adjust_gain_according_to_filter_mode(int velocity);
};

extern std::string get_builtin_modules_rdf();

};

#include "modules_synths.h"

#endif
