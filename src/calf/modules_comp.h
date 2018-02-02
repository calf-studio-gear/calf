/* Calf DSP plugin pack
 * Compression related plugins
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
#ifndef CALF_MODULES_COMP_H
#define CALF_MODULES_COMP_H

#include <assert.h>
#include <limits.h>
#include "biquad.h"
#include "inertia.h"
#include "audio_fx.h"
#include "giface.h"
#include "loudness.h"
#include "metadata.h"
#include "plugin_tools.h"
#include "bypass.h"

namespace calf_plugins {

/**********************************************************************
 * GAIN REDUCTION by Thor Harald Johanssen
**********************************************************************/
class gain_reduction_audio_module
{
private:
    float linSlope, detected, kneeSqrt, kneeStart, linKneeStart, kneeStop;
    float compressedKneeStop, adjKneeStart, thres;
    float attack, release, threshold, ratio, knee, makeup, detection, stereo_link, bypass, mute, meter_out, meter_comp;
    float old_threshold, old_ratio, old_knee, old_makeup, old_bypass, old_mute, old_detection, old_stereo_link;
    mutable bool redraw_graph;
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
    bool _get_graph(int subindex, float *data, int points, cairo_iface *context, int *mode) const;
    bool _get_dot(int subindex, float &x, float &y, int &size, cairo_iface *context) const;
    bool _get_gridline(int subindex, float &pos, bool &vertical, std::string &legend, cairo_iface *context) const;
    bool _get_layers(int index, int generation, unsigned int &layers) const;
};

/**********************************************************************
 * GAIN REDUCTION 2 by Damien Zammit
**********************************************************************/

class gain_reduction2_audio_module
{
private:
    float linSlope, detected, kneeSqrt, kneeStart, linKneeStart, kneeStop;
    float compressedKneeStop, adjKneeStart, thres;
    float attack, release, threshold, ratio, knee, makeup, detection, stereo_link, bypass, mute, meter_out, meter_comp;
    float old_threshold, old_ratio, old_knee, old_makeup, old_bypass, old_mute, old_detection, old_stereo_link;
    mutable bool redraw_graph;
    float old_y1, old_yl, old_mre, old_mae;
    uint32_t srate;
    bool is_active;
    inline float output_level(float inputt) const;
    inline float output_gain(float inputt) const;
public:
    gain_reduction2_audio_module();
    void set_params(float att, float rel, float thr, float rat, float kn, float mak, float byp, float mu);
    void update_curve();
    void process(float &left);
    void activate();
    void deactivate();
    int id;
    void set_sample_rate(uint32_t sr);
    float get_output_level();
    float get_comp_level();
    bool _get_graph(int subindex, float *data, int points, cairo_iface *context, int *mode) const;
    bool _get_dot(int subindex, float &x, float &y, int &size, cairo_iface *context) const;
    bool _get_gridline(int subindex, float &pos, bool &vertical, std::string &legend, cairo_iface *context) const;
    bool _get_layers(int index, int generation, unsigned int &layers) const;
};

/**********************************************************************
 * EXPANDER by Damien Zammit
**********************************************************************/

class expander_audio_module {
private:
    float linSlope, peak, detected, kneeSqrt, kneeStart, linKneeStart, kneeStop, linKneeStop;
    float compressedKneeStop, adjKneeStart, range, thres, attack_coeff, release_coeff;
    float attack, release, threshold, ratio, knee, makeup, detection, stereo_link, bypass, mute, meter_out, meter_gate;
    float old_threshold, old_ratio, old_knee, old_makeup, old_bypass, old_range, old_trigger, old_mute, old_detection, old_stereo_link;
    mutable bool redraw_graph;
    inline float output_level(float slope) const;
    inline float output_gain(float linSlope, bool rms) const;
public:
    uint32_t srate;
    bool is_active;
    expander_audio_module();
    void set_params(float att, float rel, float thr, float rat, float kn, float mak, float det, float stl, float byp, float mu, float ran);
    void update_curve();
    void process(float &left, float &right, const float *det_left = NULL, const float *det_right = NULL);
    void activate();
    void deactivate();
    int id;
    void set_sample_rate(uint32_t sr);
    float get_output_level();
    float get_expander_level();
    bool _get_graph(int subindex, float *data, int points, cairo_iface *context, int *mode) const;
    bool _get_dot(int subindex, float &x, float &y, int &size, cairo_iface *context) const;
    bool _get_gridline(int subindex, float &pos, bool &vertical, std::string &legend, cairo_iface *context) const;
    bool _get_layers(int index, int generation, unsigned int &layers) const;
};

/**********************************************************************
 * COMPRESSOR by Thor Harald Johanssen
**********************************************************************/

class compressor_audio_module: public audio_module<compressor_metadata>, public line_graph_iface  {
private:
    typedef compressor_audio_module AM;
    gain_reduction_audio_module compressor;
    dsp::bypass bypass;
    vumeters meters;
public:
    typedef std::complex<double> cfloat;
    uint32_t srate;
    bool is_active;
    compressor_audio_module();
    void activate();
    void deactivate();
    void params_changed();
    void set_sample_rate(uint32_t sr);
    uint32_t process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask);
    bool get_graph(int index, int subindex, int phase, float *data, int points, cairo_iface *context, int *mode) const;
    bool get_dot(int index, int subindex, int phase, float &x, float &y, int &size, cairo_iface *context) const;
    bool get_gridline(int index, int subindex, int phase, float &pos, bool &vertical, std::string &legend, cairo_iface *context) const;
    bool get_layers(int index, int generation, unsigned int &layers) const;
};

/**********************************************************************
 * SIDECHAIN COMPRESSOR by Markus Schmidt
**********************************************************************/

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
    float f1_freq_old, f2_freq_old, f1_level_old, f2_level_old;
    float f1_freq_old1, f2_freq_old1, f1_level_old1, f2_level_old1;
    CalfScModes sc_mode;
    CalfScModes sc_mode_old, sc_mode_old1;
    mutable bool redraw_graph;
    float f1_active, f2_active;
    gain_reduction_audio_module compressor;
    dsp::biquad_d2 f1L, f1R, f2L, f2R;
    dsp::bypass bypass;
    vumeters meters;
public:
    typedef std::complex<double> cfloat;
    uint32_t srate;
    bool is_active;
    sidechaincompressor_audio_module();
    void activate();
    void deactivate();
    void params_changed();
    cfloat h_z(const cfloat &z) const;
    float freq_gain(int index, double freq) const;
    void set_sample_rate(uint32_t sr);
    uint32_t process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask);
    bool get_graph(int index, int subindex, int phase, float *data, int points, cairo_iface *context, int *mode) const;
    bool get_dot(int index, int subindex, int phase, float &x, float &y, int &size, cairo_iface *context) const;
    bool get_gridline(int index, int subindex, int phase, float &pos, bool &vertical, std::string &legend, cairo_iface *context) const;
    bool get_layers(int index, int generation, unsigned int &layers) const;
};

/**********************************************************************
 * MULTIBAND COMPRESSOR by Markus Schmidt
**********************************************************************/

class multibandcompressor_audio_module: public audio_module<multibandcompressor_metadata>, public frequency_response_line_graph {
private:
    typedef multibandcompressor_audio_module AM;
    static const int strips = 4;
    bool solo[strips];
    float xout[strips], xin[2];
    bool no_solo;
    float meter_inL, meter_inR, meter_outL, meter_outR;
    gain_reduction_audio_module strip[strips];
    dsp::crossover crossover;
    dsp::bypass bypass;
    int mode, page, bypass_;
    mutable int redraw;
    vumeters meters;
public:
    uint32_t srate;
    bool is_active;
    multibandcompressor_audio_module();
    void activate();
    void deactivate();
    void params_changed();
    uint32_t process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask);
    void set_sample_rate(uint32_t sr);
    const gain_reduction_audio_module *get_strip_by_param_index(int index) const;
    virtual bool get_graph(int index, int subindex, int phase, float *data, int points, cairo_iface *context, int *mode) const;
    virtual bool get_dot(int index, int subindex, int phase, float &x, float &y, int &size, cairo_iface *context) const;
    virtual bool get_gridline(int index, int subindex, int phase, float &pos, bool &vertical, std::string &legend, cairo_iface *context) const;
    virtual bool get_layers(int index, int generation, unsigned int &layers) const;
};

/**********************************************************************
 * MONO COMPRESSOR by Damien Zammit
**********************************************************************/

class monocompressor_audio_module: public audio_module<monocompressor_metadata>, public line_graph_iface  {
private:
    typedef monocompressor_audio_module AM;
    gain_reduction2_audio_module monocompressor;
    vumeters meters;
    dsp::bypass bypass;
public:
    typedef std::complex<double> cfloat;
    uint32_t srate;
    bool is_active;
    monocompressor_audio_module();
    void activate();
    void deactivate();
    void params_changed();
    void set_sample_rate(uint32_t sr);
    uint32_t process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask);
    bool get_graph(int index, int subindex, int phase, float *data, int points, cairo_iface *context, int *mode) const;
    bool get_dot(int index, int subindex, int phase, float &x, float &y, int &size, cairo_iface *context) const;
    bool get_gridline(int index, int subindex, int phase, float &pos, bool &vertical, std::string &legend, cairo_iface *context) const;
    bool get_layers(int index, int generation, unsigned int &layers) const;
};

/**********************************************************************
 * DEESSER by Markus Schmidt
**********************************************************************/

class deesser_audio_module: public audio_module<deesser_metadata>, public frequency_response_line_graph  {
private:
    enum CalfDeessModes {
        WIDE,
        SPLIT
    };
    mutable float f1_freq_old, f2_freq_old, f1_level_old, f2_level_old, f2_q_old;
    mutable float f1_freq_old1, f2_freq_old1, f1_level_old1, f2_level_old1, f2_q_old1;
    uint32_t detected_led;
    float detected;
    gain_reduction_audio_module compressor;
    dsp::biquad_d2 hpL, hpR, lpL, lpR, pL, pR;
    dsp::bypass bypass;
    vumeters meters;
public:
    uint32_t srate;
    bool is_active;
    deesser_audio_module();
    void activate();
    void deactivate();
    void params_changed();
    float freq_gain(int index, double freq) const
    {
        return hpL.freq_gain(freq, srate) * pL.freq_gain(freq, srate);
    }
    void set_sample_rate(uint32_t sr);
    uint32_t process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask);
};

/**********************************************************************
 * GATE AUDIO MODULE Damien Zammit
**********************************************************************/

class gate_audio_module: public audio_module<gate_metadata>, public line_graph_iface  {
private:
    typedef gate_audio_module AM;
    expander_audio_module gate;
    dsp::bypass bypass;
    vumeters meters;
public:
    typedef std::complex<double> cfloat;
    uint32_t srate;
    bool is_active;
    gate_audio_module();
    void activate();
    void deactivate();
    void params_changed();
    void set_sample_rate(uint32_t sr);
    uint32_t process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask);
    bool get_graph(int index, int subindex, int phase, float *data, int points, cairo_iface *context, int *mode) const;
    bool get_dot(int index, int subindex, int phase, float &x, float &y, int &size, cairo_iface *context) const;
    bool get_gridline(int index, int subindex, int phase, float &pos, bool &vertical, std::string &legend, cairo_iface *context) const;
    bool get_layers(int index, int generation, unsigned int &layers) const;
};

/**********************************************************************
 * SIDECHAIN GATE by Markus Schmidt
**********************************************************************/

class sidechaingate_audio_module: public audio_module<sidechaingate_metadata>, public frequency_response_line_graph  {
private:
    typedef sidechaingate_audio_module AM;
    enum CalfScModes {
        WIDEBAND,
        HIGHGATE_WIDE,
        HIGHGATE_SPLIT,
        LOWGATE_WIDE,
        LOWGATE_SPLIT,
        WEIGHTED_1,
        WEIGHTED_2,
        WEIGHTED_3,
        BANDPASS_1,
        BANDPASS_2
    };
    float f1_freq_old, f2_freq_old, f1_level_old, f2_level_old;
    float f1_freq_old1, f2_freq_old1, f1_level_old1, f2_level_old1;
    CalfScModes sc_mode;
    CalfScModes sc_mode_old, sc_mode_old1;
    mutable bool redraw_graph;
    float f1_active, f2_active;
    expander_audio_module gate;
    dsp::biquad_d2 f1L, f1R, f2L, f2R;
    dsp::bypass bypass;
    vumeters meters;
public:
    typedef std::complex<double> cfloat;
    uint32_t srate;
    bool is_active;
    sidechaingate_audio_module();
    void activate();
    void deactivate();
    void params_changed();
    cfloat h_z(const cfloat &z) const;
    float freq_gain(int index, double freq) const;
    void set_sample_rate(uint32_t sr);
    uint32_t process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask);
    bool get_graph(int index, int subindex, int phase, float *data, int points, cairo_iface *context, int *mode) const;
    bool get_dot(int index, int subindex, int phase, float &x, float &y, int &size, cairo_iface *context) const;
    bool get_gridline(int index, int subindex, int phase, float &pos, bool &vertical, std::string &legend, cairo_iface *context) const;
    bool get_layers(int index, int generation, unsigned int &layers) const;
};


/**********************************************************************
 * MULTIBAND GATE by Markus Schmidt
**********************************************************************/

class multibandgate_audio_module: public audio_module<multibandgate_metadata>, public frequency_response_line_graph {
private:
    typedef multibandgate_audio_module AM;
    static const int strips = 4;
    bool solo[strips];
    float xout[strips], xin[2];
    bool no_solo;
    float meter_inL, meter_inR, meter_outL, meter_outR;
    expander_audio_module gate[strips];
    dsp::crossover crossover;
    dsp::bypass bypass;
    int mode, page, bypass_;
    mutable int redraw;
    vumeters meters;
public:
    uint32_t srate;
    bool is_active;
    multibandgate_audio_module();
    void activate();
    void deactivate();
    void params_changed();
    uint32_t process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask);
    void set_sample_rate(uint32_t sr);
    const expander_audio_module *get_strip_by_param_index(int index) const;
    virtual bool get_graph(int index, int subindex, int phase, float *data, int points, cairo_iface *context, int *mode) const;
    virtual bool get_dot(int index, int subindex, int phase, float &x, float &y, int &size, cairo_iface *context) const;
    virtual bool get_gridline(int index, int subindex, int phase, float &pos, bool &vertical, std::string &legend, cairo_iface *context) const;
    bool get_layers(int index, int generation, unsigned int &layers) const;
};

/**********************************************************************
 * TRANSIENT DESIGNER by Christian Holschuh and Markus Schmidt
**********************************************************************/

class transientdesigner_audio_module:
    public audio_module<transientdesigner_metadata>, public frequency_response_line_graph
{
    typedef transientdesigner_audio_module AM;
    static const int channels = 2;
    uint32_t srate;
    bool active;
    mutable bool redraw;
    float meter_inL, meter_inR, meter_outL, meter_outR;
    dsp::transients transients;
    dsp::bypass bypass;
    dsp::biquad_d2 hp[3], lp[3];
    float hp_f_old, hp_m_old, lp_f_old, lp_m_old;
    int display_old;
    mutable int pixels;
    mutable float *pbuffer;
    mutable int pbuffer_pos;
    mutable int pbuffer_size;
    mutable int pbuffer_sample;
    mutable int pbuffer_draw;
    mutable bool pbuffer_available;
    bool attacked;
    uint32_t attcount;
    int attack_pos;
    float display_max;
    vumeters meters;
public:
    transientdesigner_audio_module();
    ~transientdesigner_audio_module();
    void params_changed();
    void activate();
    void set_sample_rate(uint32_t sr);
    void deactivate();
    uint32_t process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask);
    bool get_graph(int index, int subindex, int phase, float *data, int points, cairo_iface *context, int *mode) const;
    bool get_gridline(int index, int subindex, int phase, float &pos, bool &vertical, std::string &legend, cairo_iface *context) const;
    bool get_layers(int index, int generation, unsigned int &layers) const;
};

};

#endif
