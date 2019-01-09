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
 * Boston, MA  02110-1301  USA
 */
#include <limits.h>
#include <memory.h>
#include <calf/audio_fx.h>
#include <calf/giface.h>
#include <calf/modules_comp.h>

using namespace dsp;
using namespace calf_plugins;

#define SET_IF_CONNECTED(name) if (params[AM::param_##name] != NULL) *params[AM::param_##name] = name;


/**********************************************************************
 * GAIN REDUCTION by Thor Harald Johanssen
**********************************************************************/

gain_reduction_audio_module::gain_reduction_audio_module()
{
    is_active       = false;
    srate           = 0;
    old_threshold   = 0.f;
    old_ratio       = 0.f;
    old_knee        = 0.f;
    old_makeup      = 0.f;
    old_detection   = 0.f;
    old_bypass      = 0.f;
    old_mute        = 0.f;
    linSlope        = 0.f;
    attack          = -1;
    release         = -1;
    detection       = -1;
    stereo_link     = -1;
    threshold       = -1;
    ratio           = -1;
    knee            = -1;
    makeup          = -1;
    bypass          = -1;
    mute            = -1;
    redraw_graph    = true;
}

void gain_reduction_audio_module::activate()
{
    is_active = true;
    float l, r;
    l = r = 0.f;
    float byp = bypass;
    bypass = 0.0;
    process(l, r, 0, 0);
    bypass = byp;
}

void gain_reduction_audio_module::deactivate()
{
    is_active = false;
}

void gain_reduction_audio_module::update_curve()
{
    float linThreshold = threshold;
    float linKneeSqrt = sqrt(knee);
    linKneeStart = linThreshold / linKneeSqrt;
    adjKneeStart = linKneeStart*linKneeStart;
    float linKneeStop = linThreshold * linKneeSqrt;
    thres = log(linThreshold);
    kneeStart = log(linKneeStart);
    kneeStop = log(linKneeStop);
    compressedKneeStop = (kneeStop - thres) / ratio + thres;
}

void gain_reduction_audio_module::process(float &left, float &right, const float *det_left, const float *det_right)
{
    if(!det_left) {
        det_left = &left;
    }
    if(!det_right) {
        det_right = &right;
    }
    float gain = 1.f;
    if(bypass < 0.5f) {
        // this routine is mainly copied from thor's compressor module
        // greatest sounding compressor I've heard!
        bool rms = (detection == 0);
        bool average = (stereo_link == 0);
        float attack_coeff = std::min(1.f, 1.f / (attack * srate / 4000.f));
        float release_coeff = std::min(1.f, 1.f / (release * srate / 4000.f));

        float absample = average ? (fabs(*det_left) + fabs(*det_right)) * 0.5f : std::max(fabs(*det_left), fabs(*det_right));
        if(rms) absample *= absample;

        dsp::sanitize(linSlope);

        linSlope += (absample - linSlope) * (absample > linSlope ? attack_coeff : release_coeff);
        
        if(linSlope > 0.f) {
            gain = output_gain(linSlope, rms);
        }
        left *= gain * makeup;
        right *= gain * makeup;
        meter_out = std::max(fabs(left), fabs(right));;
        meter_comp = gain;
        detected = rms ? sqrt(linSlope) : linSlope;
    }
}

float gain_reduction_audio_module::output_level(float slope) const {
    return slope * output_gain(slope, false) * makeup;
}

float gain_reduction_audio_module::output_gain(float linSlope, bool rms) const {
    //this calculation is also thor's work
    if(linSlope > (rms ? adjKneeStart : linKneeStart)) {
        float slope = log(linSlope);
        if(rms) slope *= 0.5f;

        float gain = 0.f;
        float delta = 0.f;
        if(IS_FAKE_INFINITY(ratio)) {
            gain = thres;
            delta = 0.f;
        } else {
            gain = (slope - thres) / ratio + thres;
            delta = 1.f / ratio;
        }

        if(knee > 1.f && slope < kneeStop) {
            gain = hermite_interpolation(slope, kneeStart, kneeStop, kneeStart, compressedKneeStop, 1.f, delta);
        }

        return exp(gain - slope);
    }

    return 1.f;
}

void gain_reduction_audio_module::set_sample_rate(uint32_t sr)
{
    srate = sr;
}
void gain_reduction_audio_module::set_params(float att, float rel, float thr, float rat, float kn, float mak, float det, float stl, float byp, float mu)
{
    // set all params
    attack          = att;
    release         = rel;
    threshold       = thr;
    ratio           = rat;
    knee            = kn;
    makeup          = mak;
    detection       = det;
    stereo_link     = stl;
    bypass          = byp;
    mute            = mu;
    if(mute > 0.f) {
        meter_out  = 0.f;
        meter_comp = 1.f;
    }
    
    if (fabs(threshold-old_threshold) + fabs(ratio - old_ratio) + fabs(knee - old_knee) + fabs(makeup - old_makeup) + fabs(detection - old_detection) + fabs(bypass - old_bypass) + fabs(mute - old_mute) > 0.000001f) {
        old_threshold = threshold;
        old_ratio     = ratio;
        old_knee      = knee;
        old_makeup    = makeup;
        old_detection = detection;
        old_bypass    = bypass;
        old_mute      = mute;
        redraw_graph  = true;
    }
}
float gain_reduction_audio_module::get_output_level() {
    // returns output level (max(left, right))
    return meter_out;
}
float gain_reduction_audio_module::get_comp_level() {
    // returns amount of compression
    return meter_comp;
}

bool gain_reduction_audio_module::_get_graph(int subindex, float *data, int points, cairo_iface *context, int *mode) const
{
    redraw_graph = false;
    if (!is_active || subindex > 1)
        return false;
    
    for (int i = 0; i < points; i++)
    {
        float input = dB_grid_inv(-1.0 + i * 2.0 / (points - 1));
        if (subindex == 0) {
            if (i == 0 || i >= points - 1)
                data[i] = dB_grid(input);
            else
                data[i] = INFINITY;
        } else {
            float output = output_level(input);
            data[i] = dB_grid(output);
        }
    }
    if (subindex == (bypass > 0.5f ? 1 : 0) || mute > 0.1f)
        context->set_source_rgba(0.15, 0.2, 0.0, 0.3);
    else {
        context->set_source_rgba(0.15, 0.2, 0.0, 0.8);
    }
    if (!subindex)
        context->set_line_width(1.);
    return true;
}

bool gain_reduction_audio_module::_get_dot(int subindex, float &x, float &y, int &size, cairo_iface *context) const
{
    if (!is_active || bypass > 0.5f || mute > 0.f || subindex)
        return false;
        
    bool rms = (detection == 0);
    float det = rms ? sqrt(detected) : detected;
    x = 0.5 + 0.5 * dB_grid(det);
    y = dB_grid(bypass > 0.5f || mute > 0.f ? det : output_level(det));
    return true;
}

bool gain_reduction_audio_module::_get_gridline(int subindex, float &pos, bool &vertical, std::string &legend, cairo_iface *context) const
{
    if (!is_active)
        return false;
    bool tmp;
    vertical = (subindex & 1) != 0;
    bool result = get_freq_gridline(subindex >> 1, pos, tmp, legend, context, false);
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

bool gain_reduction_audio_module::_get_layers(int index, int generation, unsigned int &layers) const
{
    layers = LG_REALTIME_DOT | (generation ? 0 : LG_CACHE_GRID) | ((redraw_graph || !generation) ? LG_CACHE_GRAPH : 0);
    return true;
}

/**********************************************************************
 * GAIN REDUCTION 2 by Damien Zammit
**********************************************************************/

gain_reduction2_audio_module::gain_reduction2_audio_module()
{
    is_active       = false;
    srate           = 0;
    old_threshold   = 0.f;
    old_ratio       = 0.f;
    old_knee        = 0.f;
    old_makeup      = 0.f;
    old_detection   = 0.f;
    old_bypass      = 0.f;
    old_mute        = 0.f;
    linSlope        = 0.f;
    attack          = -1;
    release         = -1;
    detection       = -1;
    stereo_link     = -1;
    threshold       = -1;
    ratio           = -1;
    knee            = -1;
    makeup          = -1;
    bypass          = -1;
    mute            = -1;
    old_y1          = 0.f;
    old_yl          = 0.f;
    old_mae         = 0.f;
    old_mre         = 0.f;
    redraw_graph    = true;
}

void gain_reduction2_audio_module::activate()
{
    is_active = true;
    float l;
    l = 0.f;
    float byp = bypass;
    bypass = 0.0;
    process(l);
    bypass = byp;
}

void gain_reduction2_audio_module::deactivate()
{
    is_active = false;
}

void gain_reduction2_audio_module::update_curve()
{

}

void gain_reduction2_audio_module::process(float &left)
{
    if(bypass < 0.5f) {
        float width=(knee-0.99f)*8.f;
        float cdb=0.f;
        float attack_coeff = exp(-1000.f/(attack * srate));
        float release_coeff = exp(-1000.f/(release * srate));
        float thresdb=20.f*log10(threshold);

        float gain = 1.f;
        float xg, xl, yg, yl, y1;
        yg=0.f;
        xg = (left==0.f) ? -160.f : 20.f*log10(fabs(left));

        if (2.f*(xg-thresdb)<-width) {
            yg = xg;
        }
        if (2.f*fabs(xg-thresdb)<=width) {
            yg = xg + (1.f/ratio-1.f)*(xg-thresdb+width/2.f)*(xg-thresdb+width/2.f)/(2.f*width);
        }
        if (2.f*(xg-thresdb)>width) {
            yg = thresdb + (xg-thresdb)/ratio;
        }
            
        xl = xg - yg;
            
        y1 = _sanitize(std::max(xl, release_coeff*old_y1+(1.f-release_coeff)*xl));
        yl = _sanitize(attack_coeff*old_yl+(1.f-attack_coeff)*y1);
        
        cdb = -yl;
        gain = exp(cdb/20.f*log(10.f));

        left *= gain * makeup;
        meter_out = (fabs(left));
        meter_comp = gain;

        float mre = _sanitize(std::max(xg, release_coeff*old_mre+(1.f-release_coeff)*xg));
        float mae = _sanitize(attack_coeff*old_mae+(1.f-attack_coeff)*mre);
        
        old_mre = mre;
        old_mae = mae;
        
        detected = exp(mae/20.f*log(10.f));
    
        old_yl = yl;
        old_y1 = y1;
    }
}

float gain_reduction2_audio_module::output_level(float inputt) const {
    return (output_gain(inputt) * makeup);
}

float gain_reduction2_audio_module::output_gain(float inputt) const {
        float width=(knee-0.99f)*8.f;
        float thresdb=20.f*log10(threshold);

        float xg, yg;
        yg=0.f;
    xg = (inputt==0.f) ? -160.f : 20.f*log10(fabs(inputt));

        if (2.f*(xg-thresdb)<-width) {
            yg = xg;
        }
        if (2.f*fabs(xg-thresdb)<=width) {
            yg = xg + (1.f/ratio-1.f)*(xg-thresdb+width/2.f)*(xg-thresdb+width/2.f)/(2.f*width);
        }
        if (2.f*(xg-thresdb)>width) {
            yg = thresdb + (xg-thresdb)/ratio;
        }
    
    return(exp(yg/20.f*log(10.f)));
}

void gain_reduction2_audio_module::set_sample_rate(uint32_t sr)
{
    srate = sr;
}
void gain_reduction2_audio_module::set_params(float att, float rel, float thr, float rat, float kn, float mak, float byp, float mu)
{
    // set all params
    attack          = att;
    release         = rel;
    threshold       = thr;
    ratio           = rat;
    knee            = kn;
    makeup          = mak;
    bypass          = byp;
    mute            = mu;
    if(mute > 0.f) {
        meter_out  = 0.f;
        meter_comp = 1.f;
    }
    if (fabs(threshold-old_threshold) + fabs(ratio - old_ratio) + fabs(knee - old_knee) + fabs(makeup - old_makeup) + fabs(detection - old_detection) + fabs(bypass - old_bypass) + fabs(mute - old_mute) > 0.000001f) {
        old_threshold = threshold;
        old_ratio     = ratio;
        old_knee      = knee;
        old_makeup    = makeup;
        old_detection = detection;
        old_bypass    = bypass;
        old_mute      = mute;
        redraw_graph  = true;
    }
}
float gain_reduction2_audio_module::get_output_level() {
    // returns output level (max(left, right))
    return meter_out;
}
float gain_reduction2_audio_module::get_comp_level() {
    // returns amount of compression
    return meter_comp;
}

bool gain_reduction2_audio_module::_get_graph(int subindex, float *data, int points, cairo_iface *context, int *mode) const
{
    redraw_graph = false;
    if (!is_active || subindex > 1)
        return false;
    
    for (int i = 0; i < points; i++)
    {
        float input = dB_grid_inv(-1.0 + i * 2.0 / (points - 1));
        if (subindex == 0) {
            if (i == 0 || i >= points - 1)
                data[i] = dB_grid(input);
            else
                data[i] = INFINITY;
        } else {
            float output = output_level(input);
            data[i] = dB_grid(output);
        }
    }
    if (subindex == (bypass > 0.5f ? 1 : 0) || mute > 0.1f)
        context->set_source_rgba(0.15, 0.2, 0.0, 0.15);
    else {
        context->set_source_rgba(0.15, 0.2, 0.0, 0.5);
    }
    if (!subindex)
        context->set_line_width(1.);
    return true;
}

bool gain_reduction2_audio_module::_get_dot(int subindex, float &x, float &y, int &size, cairo_iface *context) const
{
    if (!is_active || bypass > 0.5f || mute > 0.f || subindex)
        return false;
        
    bool rms = (detection == 0);
    float det = rms ? sqrt(detected) : detected;
    x = 0.5 + 0.5 * dB_grid(det);
    y = dB_grid(bypass > 0.5f || mute > 0.f ? det : output_level(det));
    return true;
}

bool gain_reduction2_audio_module::_get_gridline(int subindex, float &pos, bool &vertical, std::string &legend, cairo_iface *context) const
{
    bool tmp;
    vertical = (subindex & 1) != 0;
    bool result = get_freq_gridline(subindex >> 1, pos, tmp, legend, context, false);
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

bool gain_reduction2_audio_module::_get_layers(int index, int generation, unsigned int &layers) const
{
    layers = LG_REALTIME_DOT | (generation ? 0 : LG_CACHE_GRID) | ((redraw_graph || !generation) ? LG_CACHE_GRAPH : 0);
    return true;
}


/**********************************************************************
 * EXPANDER by Damien Zammit
**********************************************************************/

expander_audio_module::expander_audio_module()
{
    is_active       = false;
    srate           = 0;
    range     = -1;
    threshold = -1;
    ratio     = -1;
    knee      = -1;
    makeup    = -1;
    detection = -1;
    bypass    = -1;
    mute      = -1;
    stereo_link = -1;
    old_range     = 0.f;
    old_threshold = 0.f;
    old_ratio     = 0.f;
    old_knee      = 0.f;
    old_makeup    = 0.f;
    old_detection = 0.f;
    old_bypass    = 0.f;
    old_mute      = 0.f;
    old_trigger   = 0.f;
    old_stereo_link = 0.f;
    linSlope      = 0.f;
    linKneeStop   = 0.f;
    redraw_graph  = true;
}

void expander_audio_module::activate()
{
    is_active = true;
    update_curve();
    float l, r;
    l = r = 0.f;
    float byp = bypass;
    bypass = 0.0;
    process(l, r);
    bypass = byp;
}

void expander_audio_module::deactivate()
{
    is_active = false;
}

void expander_audio_module::update_curve()
{
    bool rms = (detection == 0);
    float linThreshold = threshold;
    if (rms)
        linThreshold = linThreshold * linThreshold;
    attack_coeff = std::min(1.f, 1.f / (attack * srate / 4000.f));
    release_coeff = std::min(1.f, 1.f / (release * srate / 4000.f));
    float linKneeSqrt = sqrt(knee);
    linKneeStart = linThreshold / linKneeSqrt;
    adjKneeStart = linKneeStart*linKneeStart;
    linKneeStop = linThreshold * linKneeSqrt;
    thres = log(linThreshold);
    kneeStart = log(linKneeStart);
    kneeStop = log(linKneeStop);
    compressedKneeStop = (kneeStop - thres) / ratio + thres;
}

void expander_audio_module::process(float &left, float &right, const float *det_left, const float *det_right)
{
    if(!det_left) {
        det_left = &left;
    }
    if(!det_right) {
        det_right = &right;
    }
    if(bypass < 0.5f) {
        // this routine is mainly copied from Damien's expander module based on Thor's compressor
        bool rms = (detection == 0);
        bool average = (stereo_link == 0);
        float absample = average ? (fabs(*det_left) + fabs(*det_right)) * 0.5f : std::max(fabs(*det_left), fabs(*det_right));
        if(rms) absample *= absample;

        dsp::sanitize(linSlope);

        linSlope += (absample - linSlope) * (absample > linSlope ? attack_coeff : release_coeff);
        float gain = 1.f;
        if(linSlope > 0.f) {
            gain = output_gain(linSlope, rms);
        }
        left *= gain * makeup;
        right *= gain * makeup;
        meter_out = std::max(fabs(left), fabs(right));
        meter_gate = gain;
        detected = linSlope;
    }
}

float expander_audio_module::output_level(float slope) const {
    bool rms = (detection == 0);
    return slope * output_gain(rms ? slope*slope : slope, rms) * makeup;
}

float expander_audio_module::output_gain(float linSlope, bool rms) const {
    //this calculation is also Damiens's work based on Thor's compressor
    if(linSlope < linKneeStop) {
        float slope = log(linSlope);
        //float tratio = rms ? sqrt(ratio) : ratio;
        float tratio = ratio;
        float gain = 0.f;
        float delta = 0.f;
        if(IS_FAKE_INFINITY(ratio))
            tratio = 1000.f;
        gain = (slope-thres) * tratio + thres;
        delta = tratio;

        if(knee > 1.f && slope > kneeStart ) {
            gain = dsp::hermite_interpolation(slope, kneeStart, kneeStop, ((kneeStart - thres) * tratio  + thres), kneeStop, delta,1.f);
        }
        return std::max(range, expf(gain-slope));
    }
    return 1.f;
}

void expander_audio_module::set_sample_rate(uint32_t sr)
{
    srate = sr;
}

void expander_audio_module::set_params(float att, float rel, float thr, float rat, float kn, float mak, float det, float stl, float byp, float mu, float ran)
{
    // set all params
    attack          = att;
    release         = rel;
    threshold       = thr;
    ratio           = rat;
    knee            = kn;
    makeup          = mak;
    detection       = det;
    stereo_link     = stl;
    bypass          = byp;
    mute            = mu;
    range           = ran;
    if(mute > 0.f) {
        meter_out  = 0.f;
        meter_gate = 1.f;
    }
    if (fabs(range - old_range) + fabs(threshold - old_threshold) + fabs(ratio - old_ratio) + fabs(knee - old_knee) + fabs(makeup - old_makeup) + fabs(detection - old_detection) + fabs(bypass - old_bypass) + fabs(mute - old_mute) > 0.000001f) {
        old_range     = range;
        old_threshold = threshold;
        old_ratio     = ratio;
        old_knee      = knee;
        old_makeup    = makeup;
        old_detection = detection;
        old_bypass    = bypass;
        old_mute      = mute;
        redraw_graph  = true;
    }
}

float expander_audio_module::get_output_level() {
    // returns output level (max(left, right))
    return meter_out;
}
float expander_audio_module::get_expander_level() {
    // returns amount of gating
    return meter_gate;
}

bool expander_audio_module::_get_graph(int subindex, float *data, int points, cairo_iface *context, int *mode) const
{
    redraw_graph = false;
    if (!is_active || subindex > 1)
        return false;
        
    for (int i = 0; i < points; i++) {
        float input = dB_grid_inv(-1.0 + i * 2.0 / (points - 1));
        if (subindex == 0) {
            if (i == 0 || i >= points - 1)
                data[i] = dB_grid(input);
            else
                data[i] = INFINITY;
        } else {
            float output = output_level(input);
            data[i] = dB_grid(output);
        }
    }
    if (subindex == (bypass > 0.5f ? 1 : 0) || mute > 0.1f)
        context->set_source_rgba(0.15, 0.2, 0.0, 0.15);
    else {
        context->set_source_rgba(0.15, 0.2, 0.0, 0.5);
    }
    if (!subindex)
        context->set_line_width(1.);
    return true;
}

bool expander_audio_module::_get_dot(int subindex, float &x, float &y, int &size, cairo_iface *context) const
{
    if (!is_active || bypass > 0.5f || mute > 0.f || subindex)
        return false;
        
    bool rms = (detection == 0);
    float det = rms ? sqrt(detected) : detected;
    x = 0.5 + 0.5 * dB_grid(det);
    y = dB_grid(bypass > 0.5f || mute > 0.f ? det : output_level(det));
    return true;
}

bool expander_audio_module::_get_gridline(int subindex, float &pos, bool &vertical, std::string &legend, cairo_iface *context) const
{
    bool tmp;
    vertical = (subindex & 1) != 0;
    bool result = get_freq_gridline(subindex >> 1, pos, tmp, legend, context, false);
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

bool expander_audio_module::_get_layers(int index, int generation, unsigned int &layers) const
{
    layers = LG_REALTIME_DOT | (generation ? 0 : LG_CACHE_GRID) | ((redraw_graph || !generation) ? LG_CACHE_GRAPH : 0);
    return true;
}


/**********************************************************************
 * COMPRESSOR by Thor Harald Johanssen
**********************************************************************/

compressor_audio_module::compressor_audio_module()
{
    is_active = false;
    srate = 0;
}

void compressor_audio_module::activate()
{
    is_active = true;
    // set all filters and strips
    compressor.activate();
    params_changed();
}
void compressor_audio_module::deactivate()
{
    is_active = false;
    compressor.deactivate();
}

void compressor_audio_module::params_changed()
{
    compressor.set_params(*params[param_attack], *params[param_release], *params[param_threshold], *params[param_ratio], *params[param_knee], *params[param_makeup], *params[param_detection], *params[param_stereo_link], *params[param_bypass], 0.f);
}

void compressor_audio_module::set_sample_rate(uint32_t sr)
{
    srate = sr;
    compressor.set_sample_rate(srate);
    int meter[] = {param_meter_in, param_meter_out, -param_compression};
    int clip[] = {param_clip_in, param_clip_out, -1};
    meters.init(params, meter, clip, 3, srate);
}

uint32_t compressor_audio_module::process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask)
{
    bool bypassed = bypass.update(*params[param_bypass] > 0.5f, numsamples);
    numsamples += offset;
    if(bypassed) {
        // everything bypassed
        while(offset < numsamples) {
            outs[0][offset] = ins[0][offset];
            outs[1][offset] = ins[1][offset];
            float values[] = {0, 0, 1};
            meters.process(values);
            ++offset;
        }
        // displays, too
    } else {
        // process
        uint32_t orig_numsamples = numsamples-offset;
        uint32_t orig_offset = offset;
        compressor.update_curve();

        while(offset < numsamples) {
            // cycle through samples
            float outL = 0.f;
            float outR = 0.f;
            float inL = ins[0][offset];
            float inR = ins[1][offset];
            float Lin = ins[0][offset];
            float Rin = ins[1][offset];
            
            // in level
            inR *= *params[param_level_in];
            inL *= *params[param_level_in];

            float leftAC = inL;
            float rightAC = inR;

            compressor.process(leftAC, rightAC);

            outL = leftAC;
            outR = rightAC;

            // mix
            outL = outL * *params[param_mix] + Lin * (*params[param_mix] * -1 + 1);
            outR = outR * *params[param_mix] + Rin * (*params[param_mix] * -1 + 1);
                
            // send to output
            outs[0][offset] = outL;
            outs[1][offset] = outR;

            float values[] = {std::max(inL, inR), std::max(outL, outR), compressor.get_comp_level()};
            meters.process(values);
            
            // next sample
            ++offset;
        } // cycle trough samples
        bypass.crossfade(ins, outs, 2, orig_offset, orig_numsamples);
    }
    meters.fall(numsamples);
    return outputs_mask;
}
bool compressor_audio_module::get_graph(int index, int subindex, int phase, float *data, int points, cairo_iface *context, int *mode) const
{
    return compressor._get_graph(subindex, data, points, context, mode);
}

bool compressor_audio_module::get_dot(int index, int subindex, int phase, float &x, float &y, int &size, cairo_iface *context) const
{
    return compressor._get_dot(subindex, x, y, size, context);
}

bool compressor_audio_module::get_gridline(int index, int subindex, int phase, float &pos, bool &vertical, std::string &legend, cairo_iface *context) const
{
    return compressor._get_gridline(subindex, pos, vertical, legend, context);
}

bool compressor_audio_module::get_layers(int index, int generation, unsigned int &layers) const
{
    return compressor._get_layers(index, generation, layers);
}

/**********************************************************************
 * SIDECHAIN COMPRESSOR by Markus Schmidt
**********************************************************************/

sidechaincompressor_audio_module::sidechaincompressor_audio_module()
{
    is_active = false;
    srate = 0;
    f1_freq_old1  = 0.f;
    f2_freq_old1  = 0.f;
    f1_level_old1 = 0.f;
    f2_level_old1 = 0.f;
    f1_freq_old  = 0.f;
    f2_freq_old  = 0.f;
    f1_level_old = 0.f;
    f2_level_old = 0.f;
    sc_mode_old1  = WIDEBAND;
    redraw_graph = true;
}

void sidechaincompressor_audio_module::activate()
{
    is_active = true;
    // set all filters and strips
    compressor.activate();
    params_changed();
}
void sidechaincompressor_audio_module::deactivate()
{
    is_active = false;
    compressor.deactivate();
}

sidechaincompressor_audio_module::cfloat sidechaincompressor_audio_module::h_z(const cfloat &z) const
{
    switch ((CalfScModes)sc_mode) {
        default:
        case WIDEBAND:
            return false;
            break;
        case DEESSER_WIDE:
        case DERUMBLER_WIDE:
        case WEIGHTED_1:
        case WEIGHTED_2:
        case WEIGHTED_3:
        case BANDPASS_2:
            return f1L.h_z(z) * f2L.h_z(z);
            break;
        case DEESSER_SPLIT:
            return f2L.h_z(z);
            break;
        case DERUMBLER_SPLIT:
        case BANDPASS_1:
            return f1L.h_z(z);
            break;
    }
}

float sidechaincompressor_audio_module::freq_gain(int index, double freq) const
{
    typedef std::complex<double> cfloat;
    freq *= 2.0 * M_PI / srate;
    cfloat z = 1.0 / exp(cfloat(0.0, freq));

    return std::abs(h_z(z));
}

void sidechaincompressor_audio_module::params_changed()
{
    // set the params of all filters
    if(*params[param_f1_freq] != f1_freq_old || *params[param_f1_level] != f1_level_old
        || *params[param_f2_freq] != f2_freq_old || *params[param_f2_level] != f2_level_old
        || *params[param_sc_mode] != sc_mode) {
        float q = 0.707;
        switch ((CalfScModes)int(*params[param_sc_mode])) {
            default:
            case WIDEBAND:
                f1L.set_hp_rbj((float)*params[param_f1_freq], q, (float)srate, *params[param_f1_level]);
                f1R.copy_coeffs(f1L);
                f2L.set_lp_rbj((float)*params[param_f2_freq], q, (float)srate, *params[param_f2_level]);
                f2R.copy_coeffs(f2L);
                f1_active = 0.f;
                f2_active = 0.f;
                break;
            case DEESSER_WIDE:
                f1L.set_peakeq_rbj((float)*params[param_f1_freq], q, *params[param_f1_level], (float)srate);
                f1R.copy_coeffs(f1L);
                f2L.set_hp_rbj((float)*params[param_f2_freq], q, (float)srate, *params[param_f2_level]);
                f2R.copy_coeffs(f2L);
                f1_active = 0.5f;
                f2_active = 1.f;
                break;
            case DEESSER_SPLIT:
                f1L.set_lp_rbj((float)*params[param_f2_freq] * (1 + 0.17), q, (float)srate);
                f1R.copy_coeffs(f1L);
                f2L.set_hp_rbj((float)*params[param_f2_freq] * (1 - 0.17), q, (float)srate, *params[param_f2_level]);
                f2R.copy_coeffs(f2L);
                f1_active = 0.f;
                f2_active = 1.f;
                break;
            case DERUMBLER_WIDE:
                f1L.set_lp_rbj((float)*params[param_f1_freq], q, (float)srate, *params[param_f1_level]);
                f1R.copy_coeffs(f1L);
                f2L.set_peakeq_rbj((float)*params[param_f2_freq], q, *params[param_f2_level], (float)srate);
                f2R.copy_coeffs(f2L);
                f1_active = 1.f;
                f2_active = 0.5f;
                break;
            case DERUMBLER_SPLIT:
                f1L.set_lp_rbj((float)*params[param_f1_freq] * (1 + 0.17), q, (float)srate, *params[param_f1_level]);
                f1R.copy_coeffs(f1L);
                f2L.set_hp_rbj((float)*params[param_f1_freq] * (1 - 0.17), q, (float)srate);
                f2R.copy_coeffs(f2L);
                f1_active = 1.f;
                f2_active = 0.f;
                break;
            case WEIGHTED_1:
                f1L.set_lowshelf_rbj((float)*params[param_f1_freq], q, *params[param_f1_level], (float)srate);
                f1R.copy_coeffs(f1L);
                f2L.set_highshelf_rbj((float)*params[param_f2_freq], q, *params[param_f2_level], (float)srate);
                f2R.copy_coeffs(f2L);
                f1_active = 0.5f;
                f2_active = 0.5f;
                break;
            case WEIGHTED_2:
                f1L.set_lowshelf_rbj((float)*params[param_f1_freq], q, *params[param_f1_level], (float)srate);
                f1R.copy_coeffs(f1L);
                f2L.set_peakeq_rbj((float)*params[param_f2_freq], q, *params[param_f2_level], (float)srate);
                f2R.copy_coeffs(f2L);
                f1_active = 0.5f;
                f2_active = 0.5f;
                break;
            case WEIGHTED_3:
                f1L.set_peakeq_rbj((float)*params[param_f1_freq], q, *params[param_f1_level], (float)srate);
                f1R.copy_coeffs(f1L);
                f2L.set_highshelf_rbj((float)*params[param_f2_freq], q, *params[param_f2_level], (float)srate);
                f2R.copy_coeffs(f2L);
                f1_active = 0.5f;
                f2_active = 0.5f;
                break;
            case BANDPASS_1:
                f1L.set_bp_rbj((float)*params[param_f1_freq], q, (float)srate, *params[param_f1_level]);
                f1R.copy_coeffs(f1L);
                f2L.set_hp_rbj((float)*params[param_f2_freq], q, *params[param_f2_level], (float)srate);
                f2R.copy_coeffs(f2L);
                f1_active = 1.f;
                f2_active = 0.f;
                break;
            case BANDPASS_2:
                f1L.set_hp_rbj((float)*params[param_f1_freq], q, (float)srate, *params[param_f1_level]);
                f1R.copy_coeffs(f1L);
                f2L.set_lp_rbj((float)*params[param_f2_freq], q, (float)srate, *params[param_f2_level]);
                f2R.copy_coeffs(f2L);
                f1_active = 1.f;
                f2_active = 1.f;
                break;
        }
        f1_freq_old = *params[param_f1_freq];
        f1_level_old = *params[param_f1_level];
        f2_freq_old = *params[param_f2_freq];
        f2_level_old = *params[param_f2_level];
        sc_mode = (CalfScModes)int(*params[param_sc_mode]);
    }
    // light LED's
    if(params[param_f1_active] != NULL) {
        *params[param_f1_active] = f1_active;
    }
    if(params[param_f2_active] != NULL) {
        *params[param_f2_active] = f2_active;
    }
    // and set the compressor module
    compressor.set_params(*params[param_attack], *params[param_release], *params[param_threshold], *params[param_ratio], *params[param_knee], *params[param_makeup], *params[param_detection], *params[param_stereo_link], *params[param_bypass], 0.f);
    
    if (*params[param_f1_freq] != f1_freq_old1
        || *params[param_f2_freq] != f2_freq_old1
        || *params[param_f1_level] != f1_level_old1
        || *params[param_f2_level] != f2_level_old1
        || *params[param_sc_mode] != sc_mode_old1)
    {
        f1_freq_old1 = *params[param_f1_freq];
        f2_freq_old1 = *params[param_f2_freq];
        f1_level_old1 = *params[param_f1_level];
        f2_level_old1 = *params[param_f2_level];
        sc_mode_old1 = (CalfScModes)int(*params[param_sc_mode]);
        redraw_graph = true;
    }
}

void sidechaincompressor_audio_module::set_sample_rate(uint32_t sr)
{
    srate = sr;
    compressor.set_sample_rate(srate);
    int meter[] = {param_meter_in, param_meter_out, -param_compression};
    int clip[] = {param_clip_in, param_clip_out, -1};
    meters.init(params, meter, clip, 3, srate);
}

uint32_t sidechaincompressor_audio_module::process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask)
{
    bool bypassed = bypass.update(*params[param_bypass] > 0.5f, numsamples);
    numsamples += offset;
    if(bypassed) {
        // everything bypassed
        while(offset < numsamples) {
            outs[0][offset] = ins[0][offset];
            outs[1][offset] = ins[1][offset];
            float values[] = {0, 0, 1};
            meters.process(values);
            ++offset;
        }
    } else {
        // process
        uint32_t orig_numsamples = numsamples-offset;
        uint32_t orig_offset = offset;
        compressor.update_curve();

        while(offset < numsamples) {
            // cycle through samples
            float outL = 0.f;
            float outR = 0.f;
            float inL  = ins[0][offset];
            float inR  = ins[1][offset];
            float Lin  = ins[0][offset];
            float Rin  = ins[1][offset];
            
            float in2L = ins[2] ? ins[2][offset] : 0;
            float in2R = ins[3] ? ins[3][offset] : 0;
            
            // in level
            inR *= *params[param_level_in];
            inL *= *params[param_level_in];

            float leftAC  = inL;
            float rightAC = inR;
            float leftSC  = inL * *params[param_sc_level];
            float rightSC = inR * *params[param_sc_level];
            float leftMC  = inL;
            float rightMC = inR;
            float leftTC  = inL;
            float rightTC = inR;
            
            if (*params[param_sc_route] > 0.5) {
                leftSC  = in2L * *params[param_sc_level];
                rightSC = in2R * *params[param_sc_level];
            }
            
            int mode = (CalfScModes)int(*params[param_sc_mode]);
            switch (mode) {
                default:
                case WIDEBAND:
                    compressor.process(leftAC, rightAC, &leftSC, &rightSC);
                    leftMC  = leftSC;
                    rightMC = rightSC;
                    break;
                case DEESSER_WIDE:
                case DERUMBLER_WIDE:
                case WEIGHTED_1:
                case WEIGHTED_2:
                case WEIGHTED_3:
                case BANDPASS_2:
                    leftSC  = f2L.process(f1L.process(leftSC));
                    rightSC = f2R.process(f1R.process(rightSC));
                    leftMC  = leftSC;
                    rightMC = rightSC;
                    compressor.process(leftAC, rightAC, &leftSC, &rightSC);
                    break;
                case BANDPASS_1:
                    leftSC  = f1L.process(leftSC);
                    rightSC = f1R.process(rightSC);
                    leftMC  = leftSC;
                    rightMC = rightSC;
                    compressor.process(leftAC, rightAC, &leftSC, &rightSC);
                    break;
                case DEESSER_SPLIT:
                case DERUMBLER_SPLIT:
                    if (mode == DEESSER_SPLIT) {
                        leftTC  = f2L.process(leftTC);
                        rightTC = f2R.process(rightTC);
                        leftAC   = f1L.process(leftAC);
                        rightAC  = f1R.process(rightAC);
                    } else {
                        leftTC  = f1L.process(leftTC);
                        rightTC = f1R.process(rightTC);
                        leftAC   = f2L.process(leftAC);
                        rightAC  = f2R.process(rightAC);
                    }
                    if (*params[param_sc_route] <= 0.5) {
                        leftSC  = leftTC;
                        rightSC = rightTC;
                    }
                    compressor.process(leftTC, rightTC, &leftSC, &rightSC);
                    leftMC  = leftSC;
                    rightMC = rightSC;
                    leftAC  += leftTC;
                    rightAC += rightTC;
                    break;
            }

            if(*params[param_sc_listen] > 0.f) {
                outL = leftMC;
                outR = rightMC;
            } else {
                outL = leftAC;
                outR = rightAC;
                // mix
                outL = outL * *params[param_mix] + Lin * (*params[param_mix] * -1 + 1);
                outR = outR * *params[param_mix] + Rin * (*params[param_mix] * -1 + 1);
            }

            // send to output
            outs[0][offset] = outL;
            outs[1][offset] = outR;
            
            float values[] = {std::max(inL, inR), std::max(outL, outR), compressor.get_comp_level()};
            meters.process(values);
            
            // next sample
            ++offset;
        } // cycle trough samples
        bypass.crossfade(ins, outs, 2, orig_offset, orig_numsamples);
        f1L.sanitize();
        f1R.sanitize();
        f2L.sanitize();
        f2R.sanitize();
    }
    meters.fall(numsamples);
    return outputs_mask;
}
bool sidechaincompressor_audio_module::get_graph(int index, int subindex, int phase, float *data, int points, cairo_iface *context, int *mode) const
{
    if (!is_active || phase)
        return false;
    if (index == param_sc_listen && !subindex) {
        return ::get_graph(*this, subindex, data, points);
    } else if(index == param_bypass) {
        return compressor._get_graph(subindex, data, points, context, mode);
    }
    return false;
}

bool sidechaincompressor_audio_module::get_dot(int index, int subindex, int phase, float &x, float &y, int &size, cairo_iface *context) const
{
    if (!is_active || !phase)
        return false;
    if (index == param_bypass) {
        return compressor._get_dot(subindex, x, y, size, context);
    }
    return false;
}

bool sidechaincompressor_audio_module::get_gridline(int index, int subindex, int phase, float &pos, bool &vertical, std::string &legend, cairo_iface *context) const
{
    if (!is_active || phase)
        return false;
    if (index == param_bypass) {
        return compressor._get_gridline(subindex, pos, vertical, legend, context);
    } else {
        return get_freq_gridline(subindex, pos, vertical, legend, context);
    }
    return false;
}

bool sidechaincompressor_audio_module::get_layers(int index, int generation, unsigned int &layers) const
{
    if(index == param_bypass)
        return compressor._get_layers(index, generation, layers);
    bool redraw = redraw_graph || !generation;
    layers = 0 | (generation ? 0 : LG_CACHE_GRID) | (redraw ? LG_CACHE_GRAPH : 0);
    redraw_graph = false;
    return redraw;
}

/**********************************************************************
 * MULTIBAND COMPRESSOR by Markus Schmidt
**********************************************************************/

multibandcompressor_audio_module::multibandcompressor_audio_module()
{
    is_active = false;
    srate      = 0;
    mode       = 0;
    redraw     = 0;
    page       = 0;
    bypass_    = 0;
    crossover.init(2, 4, 44100);
}

void multibandcompressor_audio_module::activate()
{
    is_active = true;
    // set all filters and strips
    params_changed();
    // activate all strips
    for (int j = 0; j < strips; j ++) {
        strip[j].activate();
        strip[j].id = j;
    }
}

void multibandcompressor_audio_module::deactivate()
{
    is_active = false;
    // deactivate all strips
    for (int j = 0; j < strips; j ++) {
        strip[j].deactivate();
    }
}

void multibandcompressor_audio_module::params_changed()
{
    // determine mute/solo states
    solo[0] = *params[param_solo0] > 0.f ? true : false;
    solo[1] = *params[param_solo1] > 0.f ? true : false;
    solo[2] = *params[param_solo2] > 0.f ? true : false;
    solo[3] = *params[param_solo3] > 0.f ? true : false;
    no_solo = (*params[param_solo0] > 0.f ||
            *params[param_solo1] > 0.f ||
            *params[param_solo2] > 0.f ||
            *params[param_solo3] > 0.f) ? false : true;

    
    int m = *params[param_mode];
    if (m != mode) {
        mode = *params[param_mode];
    }
    
    int p = (int)*params[param_notebook];
    if (p != page) {
        page = p;
        redraw = strips * 2 + strips;
    }
    
    int b = (int)*params[param_bypass0] + (int)*params[param_bypass1] + (int)*params[param_bypass2] + (int)*params[param_bypass3];
    if (b != bypass_) {
        redraw = strips * 2 + strips;
        bypass_ = b;
    }
    
    crossover.set_mode(mode + 1);
    crossover.set_filter(0, *params[param_freq0]);
    crossover.set_filter(1, *params[param_freq1]);
    crossover.set_filter(2, *params[param_freq2]);

    // set the params of all strips
    strip[0].set_params(*params[param_attack0], *params[param_release0], *params[param_threshold0], *params[param_ratio0], *params[param_knee0], *params[param_makeup0], *params[param_detection0], 1.f, *params[param_bypass0], !(solo[0] || no_solo));
    strip[1].set_params(*params[param_attack1], *params[param_release1], *params[param_threshold1], *params[param_ratio1], *params[param_knee1], *params[param_makeup1], *params[param_detection1], 1.f, *params[param_bypass1], !(solo[1] || no_solo));
    strip[2].set_params(*params[param_attack2], *params[param_release2], *params[param_threshold2], *params[param_ratio2], *params[param_knee2], *params[param_makeup2], *params[param_detection2], 1.f, *params[param_bypass2], !(solo[2] || no_solo));
    strip[3].set_params(*params[param_attack3], *params[param_release3], *params[param_threshold3], *params[param_ratio3], *params[param_knee3], *params[param_makeup3], *params[param_detection3], 1.f, *params[param_bypass3], !(solo[3] || no_solo));
}

void multibandcompressor_audio_module::set_sample_rate(uint32_t sr)
{
    srate = sr;
    // set srate of all strips
    for (int j = 0; j < strips; j ++) {
        strip[j].set_sample_rate(srate);
    }
    // set srate of crossover
    crossover.set_sample_rate(srate);
    int meter[] = {param_meter_inL, param_meter_inR,  param_meter_outL, param_meter_outR,
                   param_output0, -param_compression0,
                   param_output1, -param_compression1,
                   param_output2, -param_compression2,
                   param_output3, -param_compression3 };
    int clip[] = {param_clip_inL, param_clip_inR, param_clip_outL, param_clip_outR, -1, -1, -1, -1, -1, -1, -1, -1};
    meters.init(params, meter, clip, 12, srate);
}

uint32_t multibandcompressor_audio_module::process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask)
{
    bool bypassed = bypass.update(*params[param_bypass] > 0.5f, numsamples);
    numsamples += offset;
    
    for (int i = 0; i < strips; i++)
        strip[i].update_curve();
    if(bypassed) {
        // everything bypassed
        while(offset < numsamples) {
            outs[0][offset] = ins[0][offset];
            outs[1][offset] = ins[1][offset];
            float values[] = {0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 1};
            meters.process(values);
            ++offset;
        }
    } else {
        // process all strips
        uint32_t orig_numsamples = numsamples-offset;
        uint32_t orig_offset = offset;
        while(offset < numsamples) {
            // cycle through samples
            float inL = ins[0][offset];
            float inR = ins[1][offset];
            // in level
            inR *= *params[param_level_in];
            inL *= *params[param_level_in];
            // process crossover
            xin[0] = inL;
            xin[1] = inR;
            crossover.process(xin);
            // out vars
            float outL = 0.f;
            float outR = 0.f;
            for (int i = 0; i < strips; i ++) {
                // cycle trough strips
                if (solo[i] || no_solo) {
                    // strip unmuted
                    float left  = crossover.get_value(0, i);
                    float right = crossover.get_value(1, i);
                    // process gain reduction
                    strip[i].process(left, right);
                    // sum up output
                    outL += left;
                    outR += right;
                }
            } // process single strip

            // out level
            outL *= *params[param_level_out];
            outR *= *params[param_level_out];

            // send to output
            outs[0][offset] = outL;
            outs[1][offset] = outR;
            
            float values[] = {inL, inR, outL, outR,
                *params[param_bypass0] > 0.5f ? 0 : strip[0].get_output_level(), *params[param_bypass0] > 0.5f ? 1 : strip[0].get_comp_level(),
                *params[param_bypass1] > 0.5f ? 0 : strip[1].get_output_level(), *params[param_bypass1] > 0.5f ? 1 : strip[1].get_comp_level(),
                *params[param_bypass2] > 0.5f ? 0 : strip[2].get_output_level(), *params[param_bypass2] > 0.5f ? 1 : strip[2].get_comp_level(),
                *params[param_bypass3] > 0.5f ? 0 : strip[3].get_output_level(), *params[param_bypass3] > 0.5f ? 1 : strip[3].get_comp_level() };
            meters.process(values);
                
            // next sample
            ++offset;
        } // cycle trough samples
        bypass.crossfade(ins, outs, 2, orig_offset, orig_numsamples);
    } // process all strips (no bypass)
    meters.fall(numsamples);
    return outputs_mask;
}

const gain_reduction_audio_module *multibandcompressor_audio_module::get_strip_by_param_index(int index) const
{
    // let's handle by the corresponding strip
    switch (index) {
        case param_solo0:
            return &strip[0];
        case param_solo1:
            return &strip[1];
        case param_solo2:
            return &strip[2];
        case param_solo3:
            return &strip[3];
    }
    return NULL;
}

bool multibandcompressor_audio_module::get_graph(int index, int subindex, int phase, float *data, int points, cairo_iface *context, int *mode) const
{
    bool r;
    
    if (redraw)
        redraw = std::max(0, redraw - 1);
        
    const gain_reduction_audio_module *m = get_strip_by_param_index(index);
    if (m) {
        r = m->_get_graph(subindex, data, points, context, mode);
    } else {
        r = crossover.get_graph(subindex, phase, data, points, context, mode);
    }
    if ((index == param_solo0 + 11 * page && subindex == 1)
    ||  (index == param_bypass && subindex == page)) {
        *mode = 1;
    }
    if ((subindex == 1 && index != param_bypass)
    ||  (index == param_bypass)) {
        if (r 
        && ((index != param_bypass && *params[index - 1])
        ||   (index == param_bypass && *params[param_bypass0 + 11 * subindex])))
            context->set_source_rgba(0.15, 0.2, 0.0, 0.15);
        else
            context->set_source_rgba(0.15, 0.2, 0.0, 0.5);
    }
    return r;
}

bool multibandcompressor_audio_module::get_dot(int index, int subindex, int phase, float &x, float &y, int &size, cairo_iface *context) const
{
    const gain_reduction_audio_module *m = get_strip_by_param_index(index);
    if (m)
        return m->_get_dot(subindex, x, y, size, context);
    return false;
}

bool multibandcompressor_audio_module::get_gridline(int index, int subindex, int phase, float &pos, bool &vertical, std::string &legend, cairo_iface *context) const
{
    const gain_reduction_audio_module *m = get_strip_by_param_index(index);
    if (m)
        return m->_get_gridline(subindex, pos, vertical, legend, context);
    if (phase) return false;
    return get_freq_gridline(subindex, pos, vertical, legend, context);
}

bool multibandcompressor_audio_module::get_layers(int index, int generation, unsigned int &layers) const
{
    bool r;
    const gain_reduction_audio_module *m = get_strip_by_param_index(index);
    if (m) {
        r = m->_get_layers(index, generation, layers);
    } else {
        r = crossover.get_layers(index, generation, layers);
    }
    if (redraw) {
        layers |= LG_CACHE_GRAPH;
        r = true;
    }
    return r;
}


/**********************************************************************
 * MONO COMPRESSOR by Damien Zammit
**********************************************************************/

monocompressor_audio_module::monocompressor_audio_module()
{
    is_active = false;
    srate = 0;
}

void monocompressor_audio_module::activate()
{
    is_active = true;
    // set all filters and strips
    monocompressor.activate();
    params_changed();
}
void monocompressor_audio_module::deactivate()
{
    is_active = false;
    monocompressor.deactivate();
}

void monocompressor_audio_module::params_changed()
{
    monocompressor.set_params(*params[param_attack], *params[param_release], *params[param_threshold], *params[param_ratio], *params[param_knee], *params[param_makeup], *params[param_bypass], 0.f);
}

void monocompressor_audio_module::set_sample_rate(uint32_t sr)
{
    srate = sr;
    monocompressor.set_sample_rate(srate);
    int meter[] = {param_meter_in, param_meter_out, -param_compression};
    int clip[] = {param_clip_in, param_clip_out, -1};
    meters.init(params, meter, clip, 3, srate);
}

uint32_t monocompressor_audio_module::process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask)
{
    bool bypassed = bypass.update(*params[param_bypass] > 0.5f, numsamples);
    numsamples += offset;
    if(bypassed) {
        // everything bypassed
        while(offset < numsamples) {
            outs[0][offset] = ins[0][offset];
            float values[] = {0, 0, 1};
            meters.process(values);
            ++offset;
        }
    } else {
        // process
        uint32_t orig_numsamples = numsamples-offset;
        uint32_t orig_offset = offset;
        monocompressor.update_curve();

        while(offset < numsamples) {
            // cycle through samples
            float outL = 0.f;
            //float outR = 0.f;
            float inL = ins[0][offset];
            //float inR = ins[1][offset];
            float Lin = ins[0][offset];
            //float Rin = ins[1][offset];
            
            // in level
            //inR *= *params[param_level_in];
            inL *= *params[param_level_in];

            float leftAC = inL;
            //float rightAC = inR;

            monocompressor.process(leftAC);

            outL = leftAC;
            //outR = rightAC;
            
            // mix
            outL = outL * *params[param_mix] + Lin * (*params[param_mix] * -1 + 1);
            //outR = outR * *params[param_mix] + Rin * (*params[param_mix] * -1 + 1);
                
            // send to output
            outs[0][offset] = outL;
            //outs[1][offset] = 0.f;
            
            float values[] = {inL, outL, monocompressor.get_comp_level()};
            meters.process(values);
            
            // next sample
            ++offset;
        } // cycle trough samples
        bypass.crossfade(ins, outs, 1, orig_offset, orig_numsamples);
    }
    meters.fall(numsamples);
    return outputs_mask;
}
bool monocompressor_audio_module::get_graph(int index, int subindex, int phase, float *data, int points, cairo_iface *context, int *mode) const
{
    return monocompressor._get_graph(subindex, data, points, context, mode);
}

bool monocompressor_audio_module::get_dot(int index, int subindex, int phase, float &x, float &y, int &size, cairo_iface *context) const
{
    return monocompressor._get_dot(subindex, x, y, size, context);
}

bool monocompressor_audio_module::get_gridline(int index, int subindex, int phase, float &pos, bool &vertical, std::string &legend, cairo_iface *context) const
{
    return monocompressor._get_gridline(subindex, pos, vertical, legend, context);
}

bool monocompressor_audio_module::get_layers(int index, int generation, unsigned int &layers) const
{
    return monocompressor._get_layers(index, generation, layers);
}

/**********************************************************************
 * DEESSER by Markus Schmidt
**********************************************************************/

deesser_audio_module::deesser_audio_module()
{
    is_active = false;
    srate = 0;
    f1_freq_old1  = 0.f;
    f2_freq_old1  = 0.f;
    f1_level_old1 = 0.f;
    f2_level_old1 = 0.f;
    f2_q_old1     = 0.f;
    f1_freq_old  = 0.f;
    f2_freq_old  = 0.f;
    f1_level_old = 0.f;
    f2_level_old = 0.f;
    f2_q_old     = 0.f;
    detected_led = 0;
    redraw_graph = true;
}

void deesser_audio_module::activate()
{
    is_active = true;
    // set all filters and strips
    compressor.activate();
    params_changed();
    detected = 0.f;
    detected_led = 0.f;
}
void deesser_audio_module::deactivate()
{
    is_active = false;
    compressor.deactivate();
}

void deesser_audio_module::params_changed()
{
    // set the params of all filters
    if(*params[param_f1_freq] != f1_freq_old || *params[param_f1_level] != f1_level_old
        || *params[param_f2_freq] != f2_freq_old || *params[param_f2_level] != f2_level_old
        || *params[param_f2_q] != f2_q_old) {
        float q = 0.707;

        hpL.set_hp_rbj((float)*params[param_f1_freq] * (1 - 0.17), q, (float)srate, *params[param_f1_level]);
        hpR.copy_coeffs(hpL);
        lpL.set_lp_rbj((float)*params[param_f1_freq] * (1 + 0.17), q, (float)srate);
        lpR.copy_coeffs(lpL);
        pL.set_peakeq_rbj((float)*params[param_f2_freq], *params[param_f2_q], *params[param_f2_level], (float)srate);
        pR.copy_coeffs(pL);
        f1_freq_old = *params[param_f1_freq];
        f1_level_old = *params[param_f1_level];
        f2_freq_old = *params[param_f2_freq];
        f2_level_old = *params[param_f2_level];
        f2_q_old = *params[param_f2_q];
    }
    // and set the compressor module
    compressor.set_params((float)*params[param_laxity], (float)*params[param_laxity] * 1.33, *params[param_threshold], *params[param_ratio], 2.8, *params[param_makeup], *params[param_detection], 0.f, *params[param_bypass], 0.f);
    if (*params[param_f1_freq] != f1_freq_old1
        || *params[param_f2_freq] != f2_freq_old1
        || *params[param_f1_level] != f1_level_old1
        || *params[param_f2_level] != f2_level_old1
        || *params[param_f2_q] !=f2_q_old1)
    {
        f1_freq_old1 = *params[param_f1_freq];
        f2_freq_old1 = *params[param_f2_freq];
        f1_level_old1 = *params[param_f1_level];
        f2_level_old1 = *params[param_f2_level];
        f2_q_old1 = *params[param_f2_q];
        redraw_graph = true;
    }
}

void deesser_audio_module::set_sample_rate(uint32_t sr)
{
    srate = sr;
    compressor.set_sample_rate(srate);
    int meter[] = {param_detected, -param_compression};
    int clip[] = {param_clip_out, -1};
    meters.init(params, meter, clip, 2, srate);
}

uint32_t deesser_audio_module::process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask)
{
    bool bypassed = bypass.update(*params[param_bypass] > 0.5f, numsamples);
    numsamples += offset;
    detected_led -= std::min(detected_led,  numsamples);
    float gain = 1.f;
    if(bypassed) {
        // everything bypassed
        while(offset < numsamples) {
            outs[0][offset] = ins[0][offset];
            outs[1][offset] = ins[1][offset];
            float values[] = {0, 1};
            meters.process(values);
            ++offset;
        }
    } else {
        // process
        uint32_t orig_numsamples = numsamples-offset;
        uint32_t orig_offset = offset;
        compressor.update_curve();

        while(offset < numsamples) {
            // cycle through samples
            float outL = 0.f;
            float outR = 0.f;
            float inL = ins[0][offset];
            float inR = ins[1][offset];


            float leftAC = inL;
            float rightAC = inR;
            float leftSC = inL;
            float rightSC = inR;
            float leftRC = inL;
            float rightRC = inR;
            float leftMC = inL;
            float rightMC = inR;

            leftSC = pL.process(hpL.process(leftSC));
            rightSC = pR.process(hpR.process(rightSC));
            leftMC = leftSC;
            rightMC = rightSC;

            switch ((int)*params[param_mode]) {
                default:
                case WIDE:
                    compressor.process(leftAC, rightAC, &leftSC, &rightSC);
                    break;
                case SPLIT:
                    hpL.sanitize();
                    hpR.sanitize();
                    leftRC = hpL.process(leftRC);
                    rightRC = hpR.process(rightRC);
                    compressor.process(leftRC, rightRC, &leftSC, &rightSC);
                    leftAC = lpL.process(leftAC);
                    rightAC = lpR.process(rightAC);
                    leftAC += leftRC;
                    rightAC += rightRC;
                    break;
            }

            if(*params[param_sc_listen] > 0.f) {
                outL = leftMC;
                outR = rightMC;
            } else {
                outL = leftAC;
                outR = rightAC;
            }

            // send to output
            outs[0][offset] = outL;
            outs[1][offset] = outR;

            detected = std::max(fabs(leftMC), fabs(rightMC));
            
            float comp = compressor.get_comp_level();
            float values[] = {detected, comp};
            meters.process(values);
            gain = std::min(comp, gain);
            // next sample
            ++offset;
        } // cycle trough samples
        bypass.crossfade(ins, outs, 2, orig_offset, orig_numsamples);
        hpL.sanitize();
        hpR.sanitize();
        lpL.sanitize();
        lpR.sanitize();
        pL.sanitize();
        pR.sanitize();
    }
    if(params[param_detected_led] != NULL && gain < 0.89)
        detected_led = srate >> 3;
    *params[param_detected_led] = detected_led;
    meters.fall(numsamples);
    return outputs_mask;
}


/**********************************************************************
 * GATE AUDIO MODULE Damien Zammit
**********************************************************************/

gate_audio_module::gate_audio_module()
{
    is_active = false;
    srate = 0;
}

void gate_audio_module::activate()
{
    is_active = true;
    // set all filters and strips
    gate.activate();
    params_changed();
}
void gate_audio_module::deactivate()
{
    is_active = false;
    gate.deactivate();
}

void gate_audio_module::params_changed()
{
    gate.set_params(*params[param_attack], *params[param_release], *params[param_threshold], *params[param_ratio], *params[param_knee], *params[param_makeup], *params[param_detection], *params[param_stereo_link], *params[param_bypass], 0.f, *params[param_range]);
}

void gate_audio_module::set_sample_rate(uint32_t sr)
{
    srate = sr;
    gate.set_sample_rate(srate);
    int meter[] = {param_meter_in, param_meter_out, -param_gating};
    int clip[] = {param_clip_in, param_clip_out, -1};
    meters.init(params, meter, clip, 3, srate);
}

uint32_t gate_audio_module::process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask)
{
    bool bypassed = bypass.update(*params[param_bypass] > 0.5f, numsamples);
    numsamples += offset;
    if(bypassed) {
        // everything bypassed
        while(offset < numsamples) {
            outs[0][offset] = ins[0][offset];
            outs[1][offset] = ins[1][offset];
            float values[] = {0, 0, 1};
            meters.process(values);
            ++offset;
        }
    } else {
        // process
        gate.update_curve();
        uint32_t orig_numsamples = numsamples-offset;
        uint32_t orig_offset = offset;
        while(offset < numsamples) {
            // cycle through samples
            float outL = 0.f;
            float outR = 0.f;
            float inL = ins[0][offset];
            float inR = ins[1][offset];
            // in level
            inR *= *params[param_level_in];
            inL *= *params[param_level_in];

            float leftAC = inL;
            float rightAC = inR;

            gate.process(leftAC, rightAC);

            outL = leftAC;
            outR = rightAC;

            // send to output
            outs[0][offset] = outL;
            outs[1][offset] = outR;
            
            float values[] = {std::max(inL, inR), std::max(outL, outR), gate.get_expander_level()};
            meters.process(values);
            
            // next sample
            ++offset;
        } // cycle trough samples
        bypass.crossfade(ins, outs, 2, orig_offset, orig_numsamples);
    }
    meters.fall(numsamples);
    return outputs_mask;
}
bool gate_audio_module::get_graph(int index, int subindex, int phase, float *data, int points, cairo_iface *context, int *mode) const
{
    return gate._get_graph(subindex, data, points, context, mode);
}

bool gate_audio_module::get_dot(int index, int subindex, int phase, float &x, float &y, int &size, cairo_iface *context) const
{
    return gate._get_dot(subindex, x, y, size, context);
}

bool gate_audio_module::get_gridline(int index, int subindex, int phase, float &pos, bool &vertical, std::string &legend, cairo_iface *context) const
{
    return gate._get_gridline(subindex, pos, vertical, legend, context);
}

bool gate_audio_module::get_layers(int index, int generation, unsigned int &layers) const
{
    return gate._get_layers(index, generation, layers);
}

/**********************************************************************
 * SIDECHAIN GATE by Markus Schmidt
**********************************************************************/

sidechaingate_audio_module::sidechaingate_audio_module()
{
    is_active = false;
    srate = 0;
    redraw_graph = true;
    f1_freq_old = f2_freq_old = f1_level_old = f2_level_old = 0;
    f1_freq_old1 = f2_freq_old1 = f1_level_old1 = f2_level_old1 = 0;
    sc_mode_old = sc_mode_old1 = WIDEBAND; // doesn't matter as long as it's sane
}

void sidechaingate_audio_module::activate()
{
    is_active = true;
    // set all filters and strips
    gate.activate();
    params_changed();
}
void sidechaingate_audio_module::deactivate()
{
    is_active = false;
    gate.deactivate();
}

sidechaingate_audio_module::cfloat sidechaingate_audio_module::h_z(const cfloat &z) const
{
    switch ((CalfScModes)sc_mode) {
        default:
        case WIDEBAND:
            return false;
            break;
        case HIGHGATE_WIDE:
        case LOWGATE_WIDE:
        case WEIGHTED_1:
        case WEIGHTED_2:
        case WEIGHTED_3:
        case BANDPASS_2:
            return f1L.h_z(z) * f2L.h_z(z);
            break;
        case HIGHGATE_SPLIT:
            return f2L.h_z(z);
            break;
        case LOWGATE_SPLIT:
        case BANDPASS_1:
            return f1L.h_z(z);
            break;
    }
}

float sidechaingate_audio_module::freq_gain(int index, double freq) const
{
    typedef std::complex<double> cfloat;
    freq *= 2.0 * M_PI / srate;
    cfloat z = 1.0 / exp(cfloat(0.0, freq));

    return std::abs(h_z(z));
}

void sidechaingate_audio_module::params_changed()
{
    // set the params of all filters
    if(*params[param_f1_freq] != f1_freq_old || *params[param_f1_level] != f1_level_old
        || *params[param_f2_freq] != f2_freq_old || *params[param_f2_level] != f2_level_old
        || *params[param_sc_mode] != sc_mode) {
        float q = 0.707;
        switch ((CalfScModes)int(*params[param_sc_mode])) {
            default:
            case WIDEBAND:
                f1L.set_hp_rbj((float)*params[param_f1_freq], q, (float)srate, *params[param_f1_level]);
                f1R.copy_coeffs(f1L);
                f2L.set_lp_rbj((float)*params[param_f2_freq], q, (float)srate, *params[param_f2_level]);
                f2R.copy_coeffs(f2L);
                f1_active = 0.f;
                f2_active = 0.f;
                break;
            case HIGHGATE_WIDE:
                f1L.set_peakeq_rbj((float)*params[param_f1_freq], q, *params[param_f1_level], (float)srate);
                f1R.copy_coeffs(f1L);
                f2L.set_hp_rbj((float)*params[param_f2_freq], q, (float)srate, *params[param_f2_level]);
                f2R.copy_coeffs(f2L);
                f1_active = 0.5f;
                f2_active = 1.f;
                break;
            case HIGHGATE_SPLIT:
                f1L.set_lp_rbj((float)*params[param_f2_freq] * (1 + 0.17), q, (float)srate);
                f1R.copy_coeffs(f1L);
                f2L.set_hp_rbj((float)*params[param_f2_freq] * (1 - 0.17), q, (float)srate, *params[param_f2_level]);
                f2R.copy_coeffs(f2L);
                f1_active = 0.f;
                f2_active = 1.f;
                break;
            case LOWGATE_WIDE:
                f1L.set_lp_rbj((float)*params[param_f1_freq], q, (float)srate, *params[param_f1_level]);
                f1R.copy_coeffs(f1L);
                f2L.set_peakeq_rbj((float)*params[param_f2_freq], q, *params[param_f2_level], (float)srate);
                f2R.copy_coeffs(f2L);
                f1_active = 1.f;
                f2_active = 0.5f;
                break;
            case LOWGATE_SPLIT:
                f1L.set_lp_rbj((float)*params[param_f1_freq] * (1 + 0.17), q, (float)srate, *params[param_f1_level]);
                f1R.copy_coeffs(f1L);
                f2L.set_hp_rbj((float)*params[param_f1_freq] * (1 - 0.17), q, (float)srate);
                f2R.copy_coeffs(f2L);
                f1_active = 1.f;
                f2_active = 0.f;
                break;
            case WEIGHTED_1:
                f1L.set_lowshelf_rbj((float)*params[param_f1_freq], q, *params[param_f1_level], (float)srate);
                f1R.copy_coeffs(f1L);
                f2L.set_highshelf_rbj((float)*params[param_f2_freq], q, *params[param_f2_level], (float)srate);
                f2R.copy_coeffs(f2L);
                f1_active = 0.5f;
                f2_active = 0.5f;
                break;
            case WEIGHTED_2:
                f1L.set_lowshelf_rbj((float)*params[param_f1_freq], q, *params[param_f1_level], (float)srate);
                f1R.copy_coeffs(f1L);
                f2L.set_peakeq_rbj((float)*params[param_f2_freq], q, *params[param_f2_level], (float)srate);
                f2R.copy_coeffs(f2L);
                f1_active = 0.5f;
                f2_active = 0.5f;
                break;
            case WEIGHTED_3:
                f1L.set_peakeq_rbj((float)*params[param_f1_freq], q, *params[param_f1_level], (float)srate);
                f1R.copy_coeffs(f1L);
                f2L.set_highshelf_rbj((float)*params[param_f2_freq], q, *params[param_f2_level], (float)srate);
                f2R.copy_coeffs(f2L);
                f1_active = 0.5f;
                f2_active = 0.5f;
                break;
            case BANDPASS_1:
                f1L.set_bp_rbj((float)*params[param_f1_freq], q, (float)srate, *params[param_f1_level]);
                f1R.copy_coeffs(f1L);
                f2L.set_hp_rbj((float)*params[param_f2_freq], q, *params[param_f2_level], (float)srate);
                f2R.copy_coeffs(f2L);
                f1_active = 1.f;
                f2_active = 0.f;
                break;
            case BANDPASS_2:
                f1L.set_hp_rbj((float)*params[param_f1_freq], q, (float)srate, *params[param_f1_level]);
                f1R.copy_coeffs(f1L);
                f2L.set_lp_rbj((float)*params[param_f2_freq], q, (float)srate, *params[param_f2_level]);
                f2R.copy_coeffs(f2L);
                f1_active = 1.f;
                f2_active = 1.f;
                break;
        }
        f1_freq_old = *params[param_f1_freq];
        f1_level_old = *params[param_f1_level];
        f2_freq_old = *params[param_f2_freq];
        f2_level_old = *params[param_f2_level];
        sc_mode = (CalfScModes)int(*params[param_sc_mode]);
    }
    // light LED's
    if(params[param_f1_active] != NULL) {
        *params[param_f1_active] = f1_active;
    }
    if(params[param_f2_active] != NULL) {
        *params[param_f2_active] = f2_active;
    }
    // and set the expander module
    gate.set_params(*params[param_attack], *params[param_release], *params[param_threshold], *params[param_ratio], *params[param_knee], *params[param_makeup], *params[param_detection], *params[param_stereo_link], *params[param_bypass], 0.f, *params[param_range]);
    
    if (*params[param_f1_freq] != f1_freq_old1
        || *params[param_f2_freq] != f2_freq_old1
        || *params[param_f1_level] != f1_level_old1
        || *params[param_f2_level] != f2_level_old1
        || *params[param_sc_mode] != sc_mode_old1)
    {
        f1_freq_old1 = *params[param_f1_freq];
        f2_freq_old1 = *params[param_f2_freq];
        f1_level_old1 = *params[param_f1_level];
        f2_level_old1 = *params[param_f2_level];
        sc_mode_old1 = (CalfScModes)int(*params[param_sc_mode]);
        redraw_graph = true;
    }
}

void sidechaingate_audio_module::set_sample_rate(uint32_t sr)
{
    srate = sr;
    gate.set_sample_rate(srate);
    int meter[] = {param_meter_in, param_meter_out, -param_gating};
    int clip[] = {param_clip_in, param_clip_out, -1};
    meters.init(params, meter, clip, 3, srate);
}

uint32_t sidechaingate_audio_module::process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask)
{
    bool bypassed = bypass.update(*params[param_bypass] > 0.5f, numsamples);
    numsamples += offset;
    if(bypassed) {
        // everything bypassed
        while(offset < numsamples) {
            outs[0][offset] = ins[0][offset];
            outs[1][offset] = ins[1][offset];
            float values[] = {0, 0, 1};
            meters.process(values);
            ++offset;
        }
    } else {
        // process
        uint32_t orig_numsamples = numsamples-offset;
        uint32_t orig_offset = offset;
        gate.update_curve();

        while(offset < numsamples) {
            // cycle through samples
            float outL = 0.f;
            float outR = 0.f;
            float inL  = ins[0][offset];
            float inR  = ins[1][offset];
            
            float in2L = ins[2] ? ins[2][offset] : 0;
            float in2R = ins[3] ? ins[3][offset] : 0;
            
            // in level
            inR *= *params[param_level_in];
            inL *= *params[param_level_in];
            
            float leftAC  = inL;
            float rightAC = inR;
            float leftSC  = inL * *params[param_sc_level];
            float rightSC = inR * *params[param_sc_level];
            float leftMC  = inL;
            float rightMC = inR;
            float leftTC  = inL;
            float rightTC = inR;
            
            if (*params[param_sc_route] > 0.5) {
                leftSC  = in2L * *params[param_sc_level];
                rightSC = in2R * *params[param_sc_level];
            }
            
            int mode = (CalfScModes)int(*params[param_sc_mode]);
            
            switch (mode) {
                default:
                case WIDEBAND:
                    gate.process(leftAC, rightAC, &leftSC, &rightSC);
                    leftMC  = leftSC;
                    rightMC = rightSC;
                    break;
                case HIGHGATE_WIDE:
                case LOWGATE_WIDE:
                case WEIGHTED_1:
                case WEIGHTED_2:
                case WEIGHTED_3:
                case BANDPASS_2:
                    leftSC  = f2L.process(f1L.process(leftSC));
                    rightSC = f2R.process(f1R.process(rightSC));
                    leftMC  = leftSC;
                    rightMC = rightSC;
                    gate.process(leftAC, rightAC, &leftSC, &rightSC);
                    break;
                case BANDPASS_1:
                    leftSC  = f1L.process(leftSC);
                    rightSC = f1R.process(rightSC);
                    leftMC  = leftSC;
                    rightMC = rightSC;
                    gate.process(leftAC, rightAC, &leftSC, &rightSC);
                    break;
                case HIGHGATE_SPLIT:
                case LOWGATE_SPLIT:
                    if (mode == HIGHGATE_SPLIT) {
                        leftTC  = f2L.process(leftTC);
                        rightTC = f2R.process(rightTC);
                        leftAC   = f1L.process(leftAC);
                        rightAC  = f1R.process(rightAC);
                    } else {
                        leftTC  = f1L.process(leftTC);
                        rightTC = f1R.process(rightTC);
                        leftAC   = f2L.process(leftAC);
                        rightAC  = f2R.process(rightAC);
                    }
                    if (*params[param_sc_route] <= 0.5) {
                        leftSC  = leftTC;
                        rightSC = rightTC;
                    }
                    gate.process(leftTC, rightTC, &leftSC, &rightSC);
                    leftMC  = leftSC;
                    rightMC = rightSC;
                    leftAC  += leftTC;
                    rightAC += rightTC;
                    break;
            }

            if(*params[param_sc_listen] > 0.f) {
                outL = leftMC;
                outR = rightMC;
            } else {
                outL = leftAC;
                outR = rightAC;
            }

            // send to output
            outs[0][offset] = outL;
            outs[1][offset] = outR;
            
            float values[] = {std::max(inL, inR), std::max(outL, outR), gate.get_expander_level()};
            meters.process(values);
            
            // next sample
            ++offset;
        } // cycle trough samples
        bypass.crossfade(ins, outs, 2, orig_offset, orig_numsamples);
        f1L.sanitize();
        f1R.sanitize();
        f2L.sanitize();
        f2R.sanitize();

    }
    meters.fall(numsamples);
    return outputs_mask;
}
bool sidechaingate_audio_module::get_graph(int index, int subindex, int phase, float *data, int points, cairo_iface *context, int *mode) const
{
    if (!is_active || phase)
        return false;
    if (index == param_sc_listen && !subindex) {
        return ::get_graph(*this, subindex, data, points);
    } else if(index == param_bypass) {
        return gate._get_graph(subindex, data, points, context, mode);
    }
    return false;
}

bool sidechaingate_audio_module::get_dot(int index, int subindex, int phase, float &x, float &y, int &size, cairo_iface *context) const
{
    if (!is_active || !phase)
        return false;
    if (index == param_bypass) {
        return gate._get_dot(subindex, x, y, size, context);
    }
    return false;
}

bool sidechaingate_audio_module::get_gridline(int index, int subindex, int phase, float &pos, bool &vertical, std::string &legend, cairo_iface *context) const
{
    if (!is_active || phase)
        return false;
    if (index == param_bypass) {
        return gate._get_gridline(subindex, pos, vertical, legend, context);
    } else {
        return get_freq_gridline(subindex, pos, vertical, legend, context);
    }
    return false;
}
bool sidechaingate_audio_module::get_layers(int index, int generation, unsigned int &layers) const
{
    if(index == param_bypass)
        return gate._get_layers(index, generation, layers);
    bool redraw = redraw_graph || !generation;
    layers = 0 | (generation ? 0 : LG_CACHE_GRID) | (redraw ? LG_CACHE_GRAPH : 0);
    redraw_graph = false;
    return redraw;
}


/**********************************************************************
 * MULTIBAND GATE by Markus Schmidt
**********************************************************************/

multibandgate_audio_module::multibandgate_audio_module()
{
    is_active = false;
    srate = 0;
    mode       = 0;
    redraw     = 0;
    page       = 0;
    bypass_    = 0;
    crossover.init(2, 4, 44100);
}

void multibandgate_audio_module::activate()
{
    is_active = true;
    // set all filters and strips
    params_changed();
    // activate all strips
    for (int j = 0; j < strips; j ++) {
        gate[j].activate();
        gate[j].id = j;
    }
}

void multibandgate_audio_module::deactivate()
{
    is_active = false;
    // deactivate all strips
    for (int j = 0; j < strips; j ++) {
        gate[j].deactivate();
    }
}

void multibandgate_audio_module::params_changed()
{
    // determine mute/solo states
    solo[0] = *params[param_solo0] > 0.f ? true : false;
    solo[1] = *params[param_solo1] > 0.f ? true : false;
    solo[2] = *params[param_solo2] > 0.f ? true : false;
    solo[3] = *params[param_solo3] > 0.f ? true : false;
    no_solo = (*params[param_solo0] > 0.f ||
            *params[param_solo1] > 0.f ||
            *params[param_solo2] > 0.f ||
            *params[param_solo3] > 0.f) ? false : true;

    int m = *params[param_mode];
    if (m != mode) {
        mode = *params[param_mode];
    }
    
    int p = (int)*params[param_notebook];
    if (p != page) {
        page = p;
        redraw = strips * 2 + strips;
    }
    
    int b = (int)*params[param_bypass0] + (int)*params[param_bypass1] + (int)*params[param_bypass2] + (int)*params[param_bypass3];
    if (b != bypass_) {
        redraw = strips * 2 + strips;
        bypass_ = b;
    }
    
    crossover.set_mode(mode + 1);
    crossover.set_filter(0, *params[param_freq0]);
    crossover.set_filter(1, *params[param_freq1]);
    crossover.set_filter(2, *params[param_freq2]);

    // set the params of all strips
    gate[0].set_params(*params[param_attack0], *params[param_release0], *params[param_threshold0], *params[param_ratio0], *params[param_knee0], *params[param_makeup0], *params[param_detection0], 1.f, *params[param_bypass0], !(solo[0] || no_solo), *params[param_range0]);
    gate[1].set_params(*params[param_attack1], *params[param_release1], *params[param_threshold1], *params[param_ratio1], *params[param_knee1], *params[param_makeup1], *params[param_detection1], 1.f, *params[param_bypass1], !(solo[1] || no_solo), *params[param_range1]);
    gate[2].set_params(*params[param_attack2], *params[param_release2], *params[param_threshold2], *params[param_ratio2], *params[param_knee2], *params[param_makeup2], *params[param_detection2], 1.f, *params[param_bypass2], !(solo[2] || no_solo), *params[param_range2]);
    gate[3].set_params(*params[param_attack3], *params[param_release3], *params[param_threshold3], *params[param_ratio3], *params[param_knee3], *params[param_makeup3], *params[param_detection3], 1.f, *params[param_bypass3], !(solo[3] || no_solo), *params[param_range3]);
}

void multibandgate_audio_module::set_sample_rate(uint32_t sr)
{
    srate = sr;
    // set srate of all strips
    for (int j = 0; j < strips; j ++) {
        gate[j].set_sample_rate(srate);
    }
    // set srate of crossover
    crossover.set_sample_rate(srate);
    int meter[] = {param_meter_inL, param_meter_inR,  param_meter_outL, param_meter_outR,
                   param_output0, -param_gating0,
                   param_output1, -param_gating1,
                   param_output2, -param_gating2,
                   param_output3, -param_gating3 };
    int clip[] = {param_clip_inL, param_clip_inR, param_clip_outL, param_clip_outR, -1, -1, -1, -1, -1, -1, -1, -1};
    meters.init(params, meter, clip, 12, srate);
}

uint32_t multibandgate_audio_module::process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask)
{
    bool bypassed = bypass.update(*params[param_bypass] > 0.5f, numsamples);
    numsamples += offset;
    for (int i = 0; i < strips; i++)
        gate[i].update_curve();
    if(bypassed) {
        // everything bypassed
        while(offset < numsamples) {
            outs[0][offset] = ins[0][offset];
            outs[1][offset] = ins[1][offset];
            float values[] = {0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 1};
            meters.process(values);
            ++offset;
        }
    } else {
        // process all strips
        uint32_t orig_numsamples = numsamples-offset;
        uint32_t orig_offset = offset;
        while(offset < numsamples) {
            // cycle through samples
            float inL = ins[0][offset];
            float inR = ins[1][offset];
            // in level
            inR *= *params[param_level_in];
            inL *= *params[param_level_in];
            // process crossover
            xin[0] = inL;
            xin[1] = inR;
            crossover.process(xin);
            // out vars
            float outL = 0.f;
            float outR = 0.f;
            for (int i = 0; i < strips; i ++) {
                // cycle trough strips
                if (solo[i] || no_solo) {
                    // strip unmuted
                    float left  = crossover.get_value(0, i);
                    float right = crossover.get_value(1, i);
                    gate[i].process(left, right);
                    // sum up output
                    outL += left;
                    outR += right;
                }
            } // process single strip

            // out level
            outL *= *params[param_level_out];
            outR *= *params[param_level_out];

            // send to output
            outs[0][offset] = outL;
            outs[1][offset] = outR;

            float values[] = {inL, inR, outL, outR,
                *params[param_bypass0] > 0.5f ? 0 : gate[0].get_output_level(), *params[param_bypass0] > 0.5f ? 1 : gate[0].get_expander_level(),
                *params[param_bypass1] > 0.5f ? 0 : gate[1].get_output_level(), *params[param_bypass1] > 0.5f ? 1 : gate[1].get_expander_level(),
                *params[param_bypass2] > 0.5f ? 0 : gate[2].get_output_level(), *params[param_bypass2] > 0.5f ? 1 : gate[2].get_expander_level(),
                *params[param_bypass3] > 0.5f ? 0 : gate[3].get_output_level(), *params[param_bypass3] > 0.5f ? 1 : gate[3].get_expander_level() };
            meters.process(values);
            
            // next sample
            ++offset;
        } // cycle trough samples
        bypass.crossfade(ins, outs, 2, orig_offset, orig_numsamples);

    } // process all strips (no bypass)
    meters.fall(numsamples);
    return outputs_mask;
}

const expander_audio_module *multibandgate_audio_module::get_strip_by_param_index(int index) const
{
    // let's handle by the corresponding strip
    switch (index) {
        case param_solo0:
            return &gate[0];
        case param_solo1:
            return &gate[1];
        case param_solo2:
            return &gate[2];
        case param_solo3:
            return &gate[3];
    }
    return NULL;
}

bool multibandgate_audio_module::get_graph(int index, int subindex, int phase, float *data, int points, cairo_iface *context, int *mode) const
{
    bool r;
    if (redraw)
        redraw = std::max(0, redraw - 1);
        
    const expander_audio_module *m = get_strip_by_param_index(index);
    if (m) {
        r = m->_get_graph(subindex, data, points, context, mode);
    } else {
        r = crossover.get_graph(subindex, phase, data, points, context, mode);
    }
    if ((index == param_solo0 + 12 * page && subindex == 1)
    ||  (index == param_bypass && subindex == page)) {
        *mode = 1;
    }
    if ((subindex == 1 && index != param_bypass)
    ||  (index == param_bypass)) {
        if (r 
        && ((index != param_bypass && *params[index - 1])
        ||   (index == param_bypass && *params[param_bypass0 + 12 * subindex])))
            context->set_source_rgba(0.15, 0.2, 0.0, 0.15);
        else
            context->set_source_rgba(0.15, 0.2, 0.0, 0.5);
    }
    return r;
}

bool multibandgate_audio_module::get_dot(int index, int subindex, int phase, float &x, float &y, int &size, cairo_iface *context) const
{
    const expander_audio_module *m = get_strip_by_param_index(index);
    if (m)
        return m->_get_dot(subindex, x, y, size, context);
    return false;
}

bool multibandgate_audio_module::get_gridline(int index, int subindex, int phase, float &pos, bool &vertical, std::string &legend, cairo_iface *context) const
{
    const expander_audio_module *m = get_strip_by_param_index(index);
    if (m)
        return m->_get_gridline(subindex, pos, vertical, legend, context);
    if (phase) return false;
    return get_freq_gridline(subindex, pos, vertical, legend, context);
}

bool multibandgate_audio_module::get_layers(int index, int generation, unsigned int &layers) const
{
    bool r;
    const expander_audio_module *m = get_strip_by_param_index(index);
    if (m) {
        r = m->_get_layers(index, generation, layers);
    } else {
        r = crossover.get_layers(index, generation, layers);
    }
    if (redraw) {
        layers |= LG_CACHE_GRAPH;
        r = true;
    }
    return r;
}


/**********************************************************************
 * TRANSIENT DESIGNER by Christian Holschuh and Markus Schmidt
**********************************************************************/


transientdesigner_audio_module::transientdesigner_audio_module() {
    active          = false;
    pixels          = 0;
    pbuffer_pos     = 0;
    pbuffer_sample  = 0;
    pbuffer_size    = 0;
    attcount        = 0;
    attacked        = false;
    attack_pos      = 0;
    display_old     = 0;
    pbuffer_available = false;
    display_max     = pow(2,-12);
    transients.set_channels(channels);
    hp_f_old = hp_m_old = lp_f_old = lp_m_old = 0;
    redraw = false;
    pbuffer = NULL;
}
transientdesigner_audio_module::~transientdesigner_audio_module()
{
    free(pbuffer);
}
void transientdesigner_audio_module::activate() {
    active = true;
}

void transientdesigner_audio_module::deactivate() {
    active = false;
}

void transientdesigner_audio_module::params_changed() {
    if (*params[param_display] != display_old) {
        dsp::zero(pbuffer, (int)(pixels * 2));
        display_old = *params[param_display];
    }
    transients.set_params(*params[param_attack_time],
                          *params[param_attack_boost],
                          *params[param_release_time],
                          *params[param_release_boost],
                          *params[param_sustain_threshold],
                          *params[param_lookahead]);
    if (hp_f_old != *params[param_hipass]) {
        hp[0].set_hp_rbj(*params[param_hipass], 0.707, (float)srate, 1.0);
        hp[1].copy_coeffs(hp[0]);
        hp[2].copy_coeffs(hp[0]);
        redraw = true;
        hp_f_old = *params[param_hipass];
    }
    if (lp_f_old != *params[param_lopass]) {
        lp[0].set_lp_rbj(*params[param_lopass], 0.707, (float)srate, 1.0);
        lp[1].copy_coeffs(lp[0]);
        lp[2].copy_coeffs(lp[0]);
        redraw = true;
        lp_f_old = *params[param_lopass];
    }
    if (hp_m_old != *params[param_hp_mode]) {
        redraw = true;
        hp_m_old = *params[param_hp_mode];
    }
    if (lp_m_old != *params[param_lp_mode]) {
        redraw = true;
        lp_m_old = *params[param_lp_mode];
    }
}

uint32_t transientdesigner_audio_module::process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask) {
    uint32_t orig_offset = offset;
    bool bypassed = bypass.update(*params[param_bypass] > 0.5f, numsamples);
    for(uint32_t i = offset; i < offset + numsamples; i++) {
        float L = ins[0][i];
        float R = ins[1][i];
        meter_inL   = 0.f;
        meter_inR   = 0.f;
        meter_outL  = 0.f;
        meter_outR  = 0.f;
        float s = (fabs(L) + fabs(R)) / 2;
        if(bypassed) {
            outs[0][i]  = ins[0][i];
            outs[1][i]  = ins[1][i];
            
        } else {
            // levels in
            L *= *params[param_level_in];
            R *= *params[param_level_in];
            
            // GUI stuff
            meter_inL = L;
            meter_inR = R;
            
            // transient designer
            float s = (L + R) / 2.f;
            for (int k = 0; k < *params[param_hp_mode]; k ++)
                s = hp[k].process(s);
            for (int j = 0; j < *params[param_lp_mode]; j ++)
                s = lp[j].process(s);
                
            float values[] = {L, R};
            transients.process(values, s);
            
            L = values[0] * *params[param_mix] + L * (*params[param_mix] * -1 + 1);
            R = values[1] * *params[param_mix] + R * (*params[param_mix] * -1 + 1);
            
            // levels out
            L *= *params[param_level_out];
            R *= *params[param_level_out];
            
            // output
            if (*params[param_listen] > 0.5) {
                outs[0][i] = s;
                outs[1][i] = s;
            } else {
                outs[0][i] = L;
                outs[1][i] = R;
            }
            meter_outL = L;
            meter_outR = R;
        }
        // fill pixel buffer (pbuffer)
        //
        // the pixel buffer is an array holding all necessary values for
        // the line graph per pixel. its length is 5 times the available
        // pixel width of the graph multiplied with the maximum zoom
        // to hold input, output, attack, release and envelope data.
        //
        // here we write the pixel buffer, get_graph will read its
        // contents later in a drawing event.
        //
        // pbuffer_pos is the actual position in the array we are writing
        // to. It points to the first position of a set of the 5 values.
        // 
        // Since we have more audio samples than pixels we add a couple
        // of samples to one pixel before we step forward with pbuffer_pos.
        
        if (pbuffer_available) {
            // sanitize the buffer position if enough samples have
            // been captured. This is recognized by a negative value
            for (int i = 0; i < 5; i++) {
                pbuffer[pbuffer_pos + i]     = std::max(pbuffer[pbuffer_pos + i], 0.f);
            }
            
            // add samples to the buffer at the actual address
            pbuffer[pbuffer_pos]     = std::max(s, pbuffer[pbuffer_pos]);
            pbuffer[pbuffer_pos + 1] = std::max((float)(fabs(L) + fabs(R)), (float)pbuffer[pbuffer_pos + 1]);
            
            if (bypassed) {
                pbuffer[pbuffer_pos + 2] = 0;
                pbuffer[pbuffer_pos + 3] = 0;
                pbuffer[pbuffer_pos + 4] = 0;
            } else {
                pbuffer[pbuffer_pos + 2] = transients.envelope;
                pbuffer[pbuffer_pos + 3] = transients.attack;
                pbuffer[pbuffer_pos + 4] = transients.release;
            }
            
            pbuffer_sample += 1;
            
            if (pbuffer_sample >= (int)(srate * *params[param_display] / 1000.f / pixels)) {
                // we captured enough samples for one pixel on this
                // address. to keep track of the finalization invert
                // its values as a marker to sanitize in the next
                // cycle before adding samples again
                pbuffer[pbuffer_pos]     *= -1.f * *params[param_level_in];
                pbuffer[pbuffer_pos + 1] /= -2.f;
                
                // advance the buffer position
                pbuffer_pos = (pbuffer_pos + 5) % pbuffer_size;
                
                // reset sample counter
                pbuffer_sample = 0;
            }
        }
        
        attcount += 1;
        if ( transients.envelope == transients.release
        && transients.envelope > *params[param_display_threshold]
        && attcount >= srate / 100
        && pbuffer_available) {
            int diff = (int)(srate / 10 / pixels);
            diff += diff & 1;
            attack_pos = (pbuffer_pos - diff * 5 + pbuffer_size) % pbuffer_size;
            attcount = 0;
        }
        float mval[] = {meter_inL, meter_inR, meter_outL, meter_outR};
        meters.process(mval);
    }
    if (!bypassed)
        bypass.crossfade(ins, outs, 2, orig_offset, numsamples);
    meters.fall(numsamples);
    return outputs_mask;
}

void transientdesigner_audio_module::set_sample_rate(uint32_t sr)
{
    srate = sr;
    attcount = srate / 5;
    transients.set_sample_rate(srate);
    int meter[] = {param_meter_inL, param_meter_inR,  param_meter_outL, param_meter_outR};
    int clip[] = {param_clip_inL, param_clip_inR, param_clip_outL, param_clip_outR};
    meters.init(params, meter, clip, 4, srate);
}
bool transientdesigner_audio_module::get_graph(int index, int subindex, int phase, float *data, int points, cairo_iface *context, int *mode) const
{
    if (index == param_hipass) {
        // frequency response
        if (subindex) return false;
        float ret;
        double freq;
        for (int i = 0; i < points; i++) {
            ret = 1.f;
            freq = 20.0 * pow (20000.0 / 20.0, i * 1.0 / points);
            if(*params[param_hp_mode])
                ret *= pow(hp[0].freq_gain(freq, (float)srate), *params[param_hp_mode]);
            if(*params[param_lp_mode])
                ret *= pow(lp[0].freq_gain(freq, (float)srate), *params[param_lp_mode]);
            data[i] = dB_grid(ret);
        }
        redraw = false;
        return true;
    }
    
    if (subindex >= 2 || (*params[param_bypass] > 0.5f && subindex >= 1))
        return false;
    if (points <= 0)
        return false;
    if (points != pixels) {
        // the zoom level changed or it's the first time we want to draw
        // some graphs
        // 
        // buffer size is the amount of pixels for the max display value
        // if drawn in the min display zoom level multiplied by 5 for
        // keeping the input and the output fabs signals and all graphs
        // of the envelopes
        pbuffer_size = (int)(points * 5 * 100);
        // create array
        pbuffer = (float*) calloc(pbuffer_size, sizeof(float));
        
        // sanitize some indexes and addresses
        pbuffer_pos    = 0;
        pbuffer_sample = 0;
        pbuffer_draw   = 0;
        pbuffer_available = true;
        pixels = points;
    }
    // check if threshold is above minimum - we want to see trigger hold
    bool hold = *params[param_display_threshold] > display_max;
    // set the address to start from in both drawing cycles
    // to amount of pixels before pbuffer_pos or to attack_pos
    if (subindex == 0) {
        int pos = hold ? attack_pos : pbuffer_pos;
        pbuffer_draw = hold ? pos : (pbuffer_size + pos - pixels * 5) % pbuffer_size;
    }
    
    int draw_curve = 0;
    if (subindex)
        draw_curve = subindex + *params[param_view];
    
    if (!draw_curve) {
        // input is drawn as bars with less opacity
        *mode = 1;
        context->set_source_rgba(0.15, 0.2, 0.0, 0.2);
    } else {
        // output/envelope is a precise line
        context->set_line_width(0.75);
    }
                
    // draw curve
    for (int i = 0; i <= points; i++) {
        int pos = (pbuffer_draw + i * 5) % pbuffer_size + draw_curve;
        if (hold
        && ((pos > pbuffer_pos && ((pbuffer_pos > attack_pos && pos > attack_pos)
            || (pbuffer_pos < attack_pos && pos < attack_pos)))
            ||  (pbuffer_pos > attack_pos && pos < attack_pos))) {
            // we are drawing trigger hold stuff outside the hold window
            // so we don't want to see old data - zero it out.
            data[i] = dB_grid(2.51e-10, 128, 0.6);
        } else {
            // draw normally
            data[i] = dB_grid(fabs(pbuffer[pos]) + 2.51e-10, 128, 0.6);
        }
    }
    return true;
}
bool transientdesigner_audio_module::get_gridline(int index, int subindex, int phase, float &pos, bool &vertical, std::string &legend, cairo_iface *context) const
{
    if (index == param_hipass)
        // frequency response
        return get_freq_gridline(subindex, pos, vertical, legend, context);
    
    if (subindex >= 16 || phase)
        return false;
    float gain = 16.f / (1 << subindex);
    pos = dB_grid(gain, 128, 0.6);
    context->set_source_rgba(0, 0, 0, subindex & 1 ? 0.1 : 0.2);
    if (!(subindex & 1) && subindex) {
        std::stringstream ss;
        ss << (24 - 6 * subindex) << " dB";
        legend = ss.str();
    }
    return true;
}

bool transientdesigner_audio_module::get_layers(int index, int generation, unsigned int &layers) const
{
    if (index == param_hipass) {
        layers = (redraw || !generation ? LG_CACHE_GRAPH : LG_NONE) | (generation ? LG_NONE : LG_CACHE_GRID);
        return true;
    }
    layers = LG_REALTIME_GRAPH | (generation ? 0 : LG_CACHE_GRID);
    return true;
}
