/* Calf DSP plugin pack
 * Assorted plugins
 *
 * Copyright (C) 2001-2010 Krzysztof Foltman, Markus Schmidt, Thor Harald Johansen
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
#include <math.h>
#include <calf/giface.h>
#include <calf/modules_tools.h>
#include <calf/modules_dev.h>
#include <calf/utils.h>
#ifndef _MSC_VER
#include <sys/time.h>
#endif

using namespace dsp;
using namespace calf_plugins;

#define SET_IF_CONNECTED(name) if (params[AM::param_##name] != NULL) *params[AM::param_##name] = name;
#define sinc(x) (!x) ? 1 : sin(M_PI * x)/(M_PI * x);

/**********************************************************************
 * STEREO TOOLS by Markus Schmidt 
**********************************************************************/

stereo_audio_module::stereo_audio_module() {
    active      = false;
    _phase      = -1;
    buffer = NULL;
}
stereo_audio_module::~stereo_audio_module() {
    free(buffer);
}
void stereo_audio_module::activate() {
    active = true;
}

void stereo_audio_module::deactivate() {
    active = false;
}

void stereo_audio_module::params_changed() {
    if(*params[param_stereo_phase] != _phase) {
        _phase = *params[param_stereo_phase];
        _phase_cos_coef = cos(_phase / 180 * M_PI);
        _phase_sin_coef = sin(_phase / 180 * M_PI);
    }
    if(*params[param_sc_level] != _sc_level) {
        _sc_level = *params[param_sc_level];
        _inv_atan_shape = 1.0 / atan(_sc_level);
    }
}

uint32_t stereo_audio_module::process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask) {
    bool bypassed = bypass.update(*params[param_bypass] > 0.5f, numsamples);
    uint32_t orig_offset = offset;
    for(uint32_t i = offset; i < offset + numsamples; i++) {
        if(bypassed) {
            outs[0][i] = ins[0][i];
            outs[1][i] = ins[1][i];
            meter_inL  = 0.f;
            meter_inR  = 0.f;
            meter_outL = 0.f;
            meter_outR = 0.f;
        } else {
            meter_inL = 0.f;
            meter_inR = 0.f;
            meter_outL = 0.f;
            meter_outR = 0.f;
            
            float L = ins[0][i];
            float R = ins[1][i];
            
            // levels in
            L *= *params[param_level_in];
            R *= *params[param_level_in];
            
            // balance in
            L *= (1.f - std::max(0.f, *params[param_balance_in]));
            R *= (1.f + std::min(0.f, *params[param_balance_in]));
            
            // softclip
            if(*params[param_softclip]) {
                R = _inv_atan_shape * atan(R * _sc_level);
                L = _inv_atan_shape * atan(L * _sc_level);
            }
            
            // GUI stuff
            meter_inL = L;
            meter_inR = R;
            
            // modes
            float slev = *params[param_slev];       // slev - stereo level ( -2 -> 2 )
            float sbal = 1 + *params[param_sbal];   // sbal - stereo balance ( 0 -> 2 )
            float mlev = *params[param_mlev];       // mlev - mono level ( 0 -> 2 )
            float mpan = (1 + *params[param_mpan]); // mpan - mono pan ( 0 -> 1 )
            
            float l, r, m, s;
            
            switch((int)*params[param_mode])
            {
                case 0:
                    // LR > LR
                    m = (L + R) * 0.5;
                    s = (L - R) * 0.5;
                    l = m * mlev * std::min(1.f, 2.f - mpan) + s * slev * std::min(1.f, 2.f - sbal);
                    r = m * mlev * std::min(1.f, mpan)       - s * slev * std::min(1.f, sbal);
                    L = l;
                    R = r;
                    break;
                case 1:
                    // LR > MS
                    l = L * std::min(1.f, 2.f - sbal);
                    r = R * std::min(1.f, sbal);
                    L = 0.5 * (l + r) * mlev;
                    R = 0.5 * (l - r) * slev;
                    break;
                case 2:
                    // MS > LR
                    l = L * mlev * std::min(1.f, 2.f - mpan) + R * slev * std::min(1.f, 2.f - sbal);
                    r = L * mlev * std::min(1.f, mpan)       - R * slev * std::min(1.f, sbal);
                    L = l;
                    R = r;
                    break;
                case 3:
                    // LR > LL
                    R = L;
                    break;
                case 4:
                    // LR > RR
                    L = R;
                    break;
                case 5:
                    // LR > L+R
                    L = (L + R) / 2;
                    R = L;
                    break;
                case 6:
                    // LR > RL
                    l = L;
                    L = R;
                    R = l;
                    m = (L + R) * 0.5;
                    s = (L - R) * 0.5;
                    l = m * mlev * std::min(1.f, 2.f - mpan) + s * slev * std::min(1.f, 2.f - sbal);
                    r = m * mlev * std::min(1.f, mpan)       - s * slev * std::min(1.f, sbal);
                    L = l;
                    R = r;
                    break;
            }
            
            // mute
            L *= (1 - floor(*params[param_mute_l] + 0.5));
            R *= (1 - floor(*params[param_mute_r] + 0.5));
            
            // phase
            L *= (2 * (1 - floor(*params[param_phase_l] + 0.5))) - 1;
            R *= (2 * (1 - floor(*params[param_phase_r] + 0.5))) - 1;
            
            // delay
            buffer[pos]     = L;
            buffer[pos + 1] = R;
            
            int nbuf = srate * (fabs(*params[param_delay]) / 1000.f);
            nbuf -= nbuf % 2;
            if(*params[param_delay] > 0.f) {
                R = buffer[(pos - (int)nbuf + 1 + buffer_size) % buffer_size];
            } else if (*params[param_delay] < 0.f) {
                L = buffer[(pos - (int)nbuf + buffer_size)     % buffer_size];
            }
            
            // stereo base
            float _sb = *params[param_stereo_base];
            if(_sb < 0) _sb *= 0.5;
            
            float __l = L + _sb * L - _sb * R;
            float __r = R + _sb * R - _sb * L;
            
            L = __l;
            R = __r;
            
            // stereo phase
            __l = L * _phase_cos_coef - R * _phase_sin_coef;
            __r = L * _phase_sin_coef + R * _phase_cos_coef;
            
            L = __l;
            R = __r;
            
            pos = (pos + 2) % buffer_size;
            
            // balance out
            L *= (1.f - std::max(0.f, *params[param_balance_out]));
            R *= (1.f + std::min(0.f, *params[param_balance_out]));
            
            // level 
            L *= *params[param_level_out];
            R *= *params[param_level_out];
            
            //output
            outs[0][i] = L;
            outs[1][i] = R;
            
            meter_outL = L;
            meter_outR = R;
            
            // phase meter
            if(fabs(L) > 0.001 && fabs(R) > 0.001) {
                meter_phase = fabs(fabs(L+R) > 0.000000001 ? sin(fabs((L-R)/(L+R))) : 0.f);
            } else {
                meter_phase = 0.f;
            }
        }
        float values[] = {meter_inL, meter_inR, meter_outL, meter_outR};
        meters.process(values);
    }
    if (!bypassed)
        bypass.crossfade(ins, outs, 2, orig_offset, numsamples);
    meters.fall(numsamples);
    return outputs_mask;
}

void stereo_audio_module::set_sample_rate(uint32_t sr)
{
    srate = sr;
    // rebuild buffer
    buffer_size = (int)(srate * 0.05 * 2.f); // buffer size attack rate multiplied by 2 channels
    buffer = (float*) calloc(buffer_size, sizeof(float));
    pos = 0;
    int meter[] = {param_meter_inL, param_meter_inR,  param_meter_outL, param_meter_outR};
    int clip[] = {param_clip_inL, param_clip_inR, param_clip_outL, param_clip_outR};
    meters.init(params, meter, clip, 4, sr);
}

/**********************************************************************
 * MONO INPUT by Markus Schmidt
**********************************************************************/

mono_audio_module::mono_audio_module() {
    active      = false;
    meter_in    = 0.f;
    meter_outL  = 0.f;
    meter_outR  = 0.f;
    _phase      = -1.f;
    _sc_level   = 0.f;
    buffer = NULL;
}
mono_audio_module::~mono_audio_module() {
    free(buffer);
}
void mono_audio_module::activate() {
    active = true;
}

void mono_audio_module::deactivate() {
    active = false;
}

void mono_audio_module::params_changed() {
    if(*params[param_sc_level] != _sc_level) {
        _sc_level = *params[param_sc_level];
        _inv_atan_shape = 1.0 / atan(_sc_level);
    }
    if(*params[param_stereo_phase] != _phase) {
        _phase = *params[param_stereo_phase];
        _phase_cos_coef = cos(_phase / 180 * M_PI);
        _phase_sin_coef = sin(_phase / 180 * M_PI);
    }
}

uint32_t mono_audio_module::process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask) {
    bool bypassed = bypass.update(*params[param_bypass] > 0.5f, numsamples);
    uint32_t orig_offset = offset;
    for(uint32_t i = offset; i < offset + numsamples; i++) {
        if(bypassed) {
            outs[0][i] = ins[0][i];
            outs[1][i] = ins[0][i];
            meter_in    = 0.f;
            meter_outL  = 0.f;
            meter_outR  = 0.f;
        } else {
            meter_in     = 0.f;
            meter_outL   = 0.f;
            meter_outR   = 0.f;
            
            float L = ins[0][i];
            
            // levels in
            L *= *params[param_level_in];
            
            // softclip
            if(*params[param_softclip]) {
                //int ph = L / fabs(L);
                //L = L > 0.63 ? ph * (0.63 + 0.36 * (1 - pow(MATH_E, (1.f / 3) * (0.63 + L * ph)))) : L;
                L = _inv_atan_shape * atan(L * _sc_level);
            }
            
            // GUI stuff
            meter_in = L;
            
            float R = L;
            
            // mute
            L *= (1 - floor(*params[param_mute_l] + 0.5));
            R *= (1 - floor(*params[param_mute_r] + 0.5));
            
            // phase
            L *= (2 * (1 - floor(*params[param_phase_l] + 0.5))) - 1;
            R *= (2 * (1 - floor(*params[param_phase_r] + 0.5))) - 1;
            
            // delay
            buffer[pos]     = L;
            buffer[pos + 1] = R;
            
            int nbuf = srate * (fabs(*params[param_delay]) / 1000.f);
            nbuf -= nbuf % 2;
            if(*params[param_delay] > 0.f) {
                R = buffer[(pos - (int)nbuf + 1 + buffer_size) % buffer_size];
            } else if (*params[param_delay] < 0.f) {
                L = buffer[(pos - (int)nbuf + buffer_size)     % buffer_size];
            }
            
            // stereo base
            float _sb = *params[param_stereo_base];
            if(_sb < 0) _sb *= 0.5;
            
            float __l = L +_sb * L - _sb * R;
            float __r = R + _sb * R - _sb * L;
            
            L = __l;
            R = __r;
            
            // stereo phase
            __l = L * _phase_cos_coef - R * _phase_sin_coef;
            __r = L * _phase_sin_coef + R * _phase_cos_coef;
            
            L = __l;
            R = __r;
            
            pos = (pos + 2) % buffer_size;
            
            // balance out
            L *= (1.f - std::max(0.f, *params[param_balance_out]));
            R *= (1.f + std::min(0.f, *params[param_balance_out]));
            
            // level 
            L *= *params[param_level_out];
            R *= *params[param_level_out];
            
            //output
            outs[0][i] = L;
            outs[1][i] = R;
            
            meter_outL = L;
            meter_outR = R;
        }
        float values[] = {meter_in, meter_outL, meter_outR};
        meters.process(values);
    }
    if (!bypassed)
        bypass.crossfade(ins, outs, 2, orig_offset, numsamples);
    meters.fall(numsamples);
    return outputs_mask;
}

void mono_audio_module::set_sample_rate(uint32_t sr)
{
    srate = sr;
    // rebuild buffer
    buffer_size = (int)srate * 0.05 * 2; // delay buffer size multiplied by 2 channels
    buffer = (float*) calloc(buffer_size, sizeof(float));
    pos = 0;
    int meter[] = {param_meter_in,  param_meter_outL, param_meter_outR};
    int clip[] = {param_clip_in, param_clip_outL, param_clip_outR};
    meters.init(params, meter, clip, 3, sr);
}

/**********************************************************************
 * ANALYZER by Markus Schmidt and Christian Holschuh
**********************************************************************/

analyzer_audio_module::analyzer_audio_module() {

    active          = false;
    clip_L          = 0.f;
    clip_R          = 0.f;
    meter_L         = 0.f;
    meter_R         = 0.f;
    envelope        = 0.f;
    ppos            = 0;
    plength         = 0;
    phase_buffer = (float*) calloc(max_phase_buffer_size, sizeof(float));
}
analyzer_audio_module::~analyzer_audio_module() {
    free(phase_buffer);
}
void analyzer_audio_module::activate() {
    active = true;
}

void analyzer_audio_module::deactivate() {
    active = false;
}

void analyzer_audio_module::params_changed() {
    float resolution, offset;
    switch((int)*params[param_analyzer_mode]) {
        case 0:
        case 1:
        case 2:
        case 3:
        default:
            // analyzer
            resolution = pow(64, *params[param_analyzer_level]);
            offset = 0.75;
            break;
        case 4:
            // we want to draw Stereo Image
            resolution = pow(64, *params[param_analyzer_level] * 1.75);
            offset = 1.f;
            break;
        case 5:
            // We want to draw Stereo Difference
            offset = *params[param_analyzer_level] > 1
                ? 1 + (*params[param_analyzer_level] - 1) / 4
                : *params[param_analyzer_level];
            resolution = pow(64, 2 * offset);
            break;
    }
    
    _analyzer.set_params(
        resolution,
        offset,
        *params[param_analyzer_accuracy],
        *params[param_analyzer_hold],
        *params[param_analyzer_smoothing],
        *params[param_analyzer_mode],
        *params[param_analyzer_scale],
        *params[param_analyzer_post],
        *params[param_analyzer_speed],
        *params[param_analyzer_windowing],
        *params[param_analyzer_view],
        *params[param_analyzer_freeze]
    );
}

uint32_t analyzer_audio_module::process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask) {
    for(uint32_t i = offset; i < offset + numsamples; i++) {
        // let meters fall a bit
        clip_L   -= std::min(clip_L, numsamples);
        clip_R   -= std::min(clip_R, numsamples);
        meter_L   = 0.f;
        meter_R   = 0.f;
        
        float L = ins[0][i];
        float R = ins[1][i];
        
        // GUI stuff
        if(L > 1.f) clip_L = srate >> 3;
        if(R > 1.f) clip_R = srate >> 3;
        
        // goniometer
        //the goniometer tries to show the signal in maximum
        //size. therefor an envelope with fast attack and slow
        //release is calculated with the max value of left and right.
        //Multiplied envelope by sqrt(2) to push the boundary to the corners
        //of the square (for example (L,R) = (1,1) is sqrt(2) distance to center).
        float lemax  = fabs(L) > fabs(R) ? fabs(L)*sqrt(2) : fabs(R)*sqrt(2);
        if( lemax > envelope)
           envelope  = lemax; //attack_coef * (envelope - lemax) + lemax;
        else
           envelope  = (release_coef * (envelope - lemax) + lemax);
        
        //use the envelope to bring biggest signal to 1. the biggest
        //enlargement of the signal is 4.
        
        phase_buffer[ppos]     = L / std::max(0.25f, (envelope));
        phase_buffer[ppos + 1] = R / std::max(0.25f, (envelope));
        
        
        plength = std::min(phase_buffer_size, plength + 2);
        ppos += 2;
        ppos %= (phase_buffer_size - 2);
        
        // analyzer
        _analyzer.process(L, R);
        
        // meter
        meter_L = L;
        meter_R = R;
        
        //output
        outs[0][i] = L;
        outs[1][i] = R;
    }
    // draw meters
    SET_IF_CONNECTED(clip_L);
    SET_IF_CONNECTED(clip_R);
    SET_IF_CONNECTED(meter_L);
    SET_IF_CONNECTED(meter_R);
    return outputs_mask;
}

void analyzer_audio_module::set_sample_rate(uint32_t sr)
{
    srate = sr;
    phase_buffer_size = srate / 30 * 2;
    phase_buffer_size -= phase_buffer_size % 2;
    phase_buffer_size = std::min(phase_buffer_size, (int)max_phase_buffer_size);
    _analyzer.set_sample_rate(sr);
    attack_coef  = exp(log(0.01)/(0.01 * srate * 0.001));
    release_coef = exp(log(0.01)/(2000 * srate * 0.001));
}

bool analyzer_audio_module::get_phase_graph(int index, float ** _buffer, int *_length, int * _mode, bool * _use_fade, float * _fade, int * _accuracy, bool * _display) const {
    *_buffer   = &phase_buffer[0];
    *_length   = plength;
    *_use_fade = *params[param_gonio_use_fade];
    *_fade     = 0.6;
    *_mode     = *params[param_gonio_mode];
    *_accuracy = *params[param_gonio_accuracy];
    *_display  = *params[param_gonio_display];
    return false;
}

bool analyzer_audio_module::get_graph(int index, int subindex, int phase, float *data, int points, cairo_iface *context, int *mode) const
{
    if (*params[param_analyzer_display])
        return _analyzer.get_graph(subindex, phase, data, points, context, mode);
    return false;
}
bool analyzer_audio_module::get_moving(int index, int subindex, int &direction, float *data, int x, int y, int &offset, uint32_t &color) const
{
    if (*params[param_analyzer_display])
        return _analyzer.get_moving(subindex, direction, data, x, y, offset, color);
    return false;
}
bool analyzer_audio_module::get_gridline(int index, int subindex, int phase, float &pos, bool &vertical, std::string &legend, cairo_iface *context) const
{ 
    return _analyzer.get_gridline(subindex, phase, pos, vertical, legend, context);
}
bool analyzer_audio_module::get_layers(int index, int generation, unsigned int &layers) const
{
    return _analyzer.get_layers(generation, layers);
}


/**********************************************************************
 * MULTIBAND ENHANCER by Markus Schmidt
**********************************************************************/

multibandenhancer_audio_module::multibandenhancer_audio_module()
{
    srate               = 0;
    _mode               = -1;
    channels            = 2;
    is_active           = false;
    ppos                = 0;
    plength             = 0;
    for (int i = 0; i < strips; i++) {
        phase_buffer[i] = (float*) calloc(max_phase_buffer_size, sizeof(float));
        envelope[i] = 0;
    }
    crossover.init(channels, strips, 44100);
}
multibandenhancer_audio_module::~multibandenhancer_audio_module()
{
    for (int i = 0; i < strips; i++)
      free(phase_buffer[i]);
}
void multibandenhancer_audio_module::activate()
{
    is_active = true;
    for (int i = 0; i < strips; i++) {
        for (int j = 0; j < channels; j++) {
            dist[i][j].activate();
        }
    }
}

void multibandenhancer_audio_module::deactivate()
{
    is_active = false;
    for (int i = 0; i < strips; i++) {
        for (int j = 0; j < channels; j++) {
            dist[i][j].deactivate();
        }
    }
}

void multibandenhancer_audio_module::params_changed()
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
    if (m != _mode) {
        _mode = *params[param_mode];
    }
    
    crossover.set_mode(_mode + 1);
    crossover.set_filter(0, *params[param_freq0]);
    crossover.set_filter(1, *params[param_freq1]);
    crossover.set_filter(2, *params[param_freq2]);
    
    // set the params of all strips
    for (int i = 0; i < strips; i++) {
        for (int j = 0; j < channels; j++) {
            dist[i][j].set_params(*params[param_blend0 + i], *params[param_drive0 + i]);
        }
    }
}

void multibandenhancer_audio_module::set_sample_rate(uint32_t sr)
{
    srate = sr;
    int meter[] = {param_meter_inL, param_meter_inR,  param_meter_outL, param_meter_outR};
    int clip[] = {param_clip_inL, param_clip_inR, param_clip_outL, param_clip_outR};
    meters.init(params, meter, clip, 4, srate);
    crossover.set_sample_rate(srate);
    for (int i = 0; i < strips; i++) {
        for (int j = 0; j < channels; j++) {
            dist[i][j].set_sample_rate(srate);
        }
    }
    attack_coef  = exp(log(0.01)/(0.01 * srate * 0.001));
    release_coef = exp(log(0.01)/(2000 * srate * 0.001));
    phase_buffer_size = srate / 30 * 2;
    phase_buffer_size -= phase_buffer_size % 2;
    phase_buffer_size = std::min(phase_buffer_size, (int)max_phase_buffer_size);
}

uint32_t multibandenhancer_audio_module::process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask)
{
    bool bypassed = bypass.update(*params[param_bypass] > 0.5f, numsamples);
    uint32_t orig_numsamples = numsamples;
    uint32_t orig_offset = offset;
    numsamples += offset;
    if(bypassed) {
        // everything bypassed
        while(offset < numsamples) {
            for (int i = 0; i < strips; i ++) {
                phase_buffer[i][ppos]     = 0;
                phase_buffer[i][ppos + 1] = 0;
            }
            // phase buffer handling
            plength = std::min(phase_buffer_size, plength + 2);
            ppos += 2;
            ppos %= (phase_buffer_size - 2);
            
            outs[0][offset] = ins[0][offset];
            outs[1][offset] = ins[1][offset];
            float values[] = {0, 0, 0, 0};
            meters.process(values);
            ++offset;
        }
    } else {
        // process all strips
        while(offset < numsamples) {
            float inL  = ins[0][offset]; // input
            float inR  = ins[1][offset];
            float outL = 0.f; // final output
            float outR = 0.f;
            float tmpL = 0.f; // used for temporary purposes
            float tmpR = 0.f;
            
            // in level
            inR *= *params[param_level_in];
            inL *= *params[param_level_in];
            
            // process crossover
            float xin[] = {inL, inR};
            crossover.process(xin);
            
            for (int i = 0; i < strips; i ++) {
                // cycle trough strips
                float L = crossover.get_value(0, i);
                float R = crossover.get_value(1, i);
                // stereo base
                tmpL = L;
                tmpR = R;
                float _sb = *params[param_base0 + i];
                if (_sb != 0) {
                    if(_sb < 0) _sb *= 0.5;
                    tmpL = L + _sb * L - _sb * R;
                    tmpR = R + _sb * R - _sb * L;
                    // compensate loudness
                    float f = (_sb + 1) / 2 + 0.5;
                    tmpL /= f;
                    tmpR /= f;
                }
                L = tmpL;
                R = tmpR;
                if (solo[i] || no_solo) {
                    // process harmonics
                    if (*params[param_drive0 + i]) {
                        L = dist[i][0].process(tmpL);
                        R = dist[i][1].process(tmpR);
                    }
                    // compensate saturation
                    L /= (1 + *params[param_drive0 + i] * 0.075);
                    R /= (1 + *params[param_drive0 + i] * 0.075);
                    // sum up output
                    outL += L;
                    outR += R;
                }
                // goniometer
                float lemax  = fabs(L) > fabs(R) ? fabs(L) : fabs(R);
                if( lemax > envelope[i])
                   envelope[i] = lemax; //attack_coef * (envelope[i] - lemax) + lemax;
                else
                   envelope[i] = release_coef * (envelope[i] - lemax) + lemax;
                phase_buffer[i][ppos]     = L / std::max(0.25f, (envelope[i]));
                phase_buffer[i][ppos + 1] = R / std::max(0.25f, (envelope[i]));
            }
            // phase buffer handling
            plength = std::min(phase_buffer_size, plength + 2);
            ppos += 2;
            ppos %= (phase_buffer_size - 2);
                
            // out level
            outL *= *params[param_level_out];
            outR *= *params[param_level_out];

            // send to output
            outs[0][offset] = outL;
            outs[1][offset] = outR;

            // next sample
            ++offset;
            
            float values[] = {inL, inR, outL, outR};
            meters.process(values);
        } // cycle trough samples
        bypass.crossfade(ins, outs, 2, orig_offset, orig_numsamples);
    } // process (no bypass)
    meters.fall(numsamples);
    return outputs_mask;
}
bool multibandenhancer_audio_module::get_phase_graph(int index, float ** _buffer, int *_length, int * _mode, bool * _use_fade, float * _fade, int * _accuracy, bool * _display) const {
    int i = index - param_base0;
    *_buffer   = &phase_buffer[i][0];
    *_length   = plength;
    *_use_fade = 1;
    *_fade     = 0.6;
    *_mode     = 0;
    *_accuracy = 3;
    *_display  = solo[i] || no_solo;
    return false;
}
bool multibandenhancer_audio_module::get_graph(int index, int subindex, int phase, float *data, int points, cairo_iface *context, int *mode) const
{
    return crossover.get_graph(subindex, phase, data, points, context, mode);
}
bool multibandenhancer_audio_module::get_layers(int index, int generation, unsigned int &layers) const
{
    return crossover.get_layers(index, generation, layers);
}


/**********************************************************************
 * MULTIBAND SPREAD by Markus Schmidt
**********************************************************************/

multispread_audio_module::multispread_audio_module()
{
    srate               = 0;
    is_active           = false;
    redraw_graph        = true;
    fcoeff              = log10(20.f);
    ppos                = 0;
    plength             = 0;
    phase_buffer        = (float*) calloc(max_phase_buffer_size, sizeof(float));
    envelope            = 0;
}
multispread_audio_module::~multispread_audio_module()
{
    free(phase_buffer);
}
void multispread_audio_module::activate()
{
    is_active = true;
}

void multispread_audio_module::deactivate()
{
    is_active = false;
}

void multispread_audio_module::params_changed()
{
    if (*params[param_amount0] != amount0
    ||  *params[param_amount1] != amount1
    ||  *params[param_amount2] != amount2
    ||  *params[param_amount3] != amount3
    ||  *params[param_intensity] != intensity
    ||  *params[param_filters] != filters) {
        redraw_graph = true;
        amount0 = *params[param_amount0];
        amount1 = *params[param_amount1];
        amount2 = *params[param_amount2];
        amount3 = *params[param_amount3];
        filters = *params[param_filters];
        int amount = filters * 4;
        float q = filters / 3.;
        float gain1, gain2;
        float j = 1. + pow(1 - *params[param_intensity], 4) * 99;
        for (int i = 0; i < amount; i++) {
            float f = pow(*params[param_amount0 + int(i / filters)], 1. / j);
            gain1 = f;
            gain2 = 1. / f;
            L[i].set_peakeq_rbj(pow(10, fcoeff + (0.5f + (float)i) * 3.f / (float)amount), q, (i % 2) ? gain1 : gain2, (double)srate);
            R[i].set_peakeq_rbj(pow(10, fcoeff + (0.5f + (float)i) * 3.f / (float)amount), q, (i % 2) ? gain2 : gain1, (double)srate);
        }
    }
}

void multispread_audio_module::set_sample_rate(uint32_t sr)
{
    srate = sr;
    int meter[] = {param_meter_inL, param_meter_inR,  param_meter_outL, param_meter_outR};
    int clip[] = {param_clip_inL, param_clip_inR, param_clip_outL, param_clip_outR};
    meters.init(params, meter, clip, 4, srate);
    attack_coef  = exp(log(0.01)/(0.01 * srate * 0.001));
    release_coef = exp(log(0.01)/(2000 * srate * 0.001));
    phase_buffer_size = srate / 30 * 2;
    phase_buffer_size -= phase_buffer_size % 2;
    phase_buffer_size = std::min(phase_buffer_size, (int)max_phase_buffer_size);
}

uint32_t multispread_audio_module::process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask)
{
    bool bypassed = bypass.update(*params[param_bypass] > 0.5f, numsamples);
    uint32_t orig_numsamples = numsamples;
    uint32_t orig_offset = offset;
    numsamples += offset;
    if(bypassed) {
        // everything bypassed
        while(offset < numsamples) {
            outs[0][offset] = ins[0][offset];
            outs[1][offset] = *params[param_mono] > 0.5 ? ins[0][offset] : ins[1][offset];
            float values[] = {0, 0, 0, 0};
            meters.process(values);
            // phase buffer handling
            phase_buffer[ppos]     = 0;
            phase_buffer[ppos + 1] = 0;
            plength = std::min(phase_buffer_size, plength + 2);
            ppos += 2;
            ppos %= (phase_buffer_size - 2);
            ++offset;
        }
    } else {
        // process all strips
        while(offset < numsamples) {
            float inL  = ins[0][offset]; // input
            float inR  = *params[param_mono] > 0.5 ? ins[0][offset] : ins[1][offset];
            float outL = 0.f; // final output
            float outR = 0.f;
            
            // in level
            inR *= *params[param_level_in];
            inL *= *params[param_level_in];
            
            outL = inL;
            outR = inR;
            
            // filters
            int amount = filters * 4;
            for (int i = 0; i < amount; i++) {
                outL = L[i].process(outL);
                outR = R[i].process(outR);
            }
            // out level
            outL *= *params[param_level_out];
            outR *= *params[param_level_out];

            // phase buffer
            float lemax  = fabs(outL) > fabs(outR) ? fabs(outL) : fabs(outR);
            if (lemax > envelope)
               envelope = lemax; //attack_coef * (envelope[i] - lemax) + lemax;
            else
               envelope = release_coef * (envelope - lemax) + lemax;
            phase_buffer[ppos]     = outL / std::max(0.25f, (envelope));
            phase_buffer[ppos + 1] = outR / std::max(0.25f, (envelope));
            
            // phase buffer handling
            plength = std::min(phase_buffer_size, plength + 2);
            ppos += 2;
            ppos %= (phase_buffer_size - 2);
            
            // send to output
            outs[0][offset] = outL;
            outs[1][offset] = outR;
            
            // next sample
            ++offset;
            
            float values[] = {inL, inR, outL, outR};
            meters.process(values);
        } // cycle trough samples
        bypass.crossfade(ins, outs, 2, orig_offset, orig_numsamples);
    } // process (no bypass)
    meters.fall(numsamples);
    return outputs_mask;
}
bool multispread_audio_module::get_graph(int index, int subindex, int phase, float *data, int points, cairo_iface *context, int *mode) const
{
    if (subindex || phase)
        return false;
    return ::get_graph(*this, index, data, points, 64, 0);
}
bool multispread_audio_module::get_layers(int index, int generation, unsigned int &layers) const
{
    redraw_graph = redraw_graph || !generation;
    layers |= (redraw_graph ? LG_CACHE_GRAPH : LG_NONE) | (generation ? LG_NONE : LG_CACHE_GRID);
    int red = redraw_graph;
    if (index == param_amount1)
        redraw_graph = false;
    return red;
}
bool multispread_audio_module::get_gridline(int index, int subindex, int phase, float &pos, bool &vertical, std::string &legend, cairo_iface *context) const
{
    if (phase)
        return false;
    bool r = get_freq_gridline(subindex, pos, vertical, legend, context, true, 64, 0);
    if (!vertical)
        legend = "";
    return r;
}
float multispread_audio_module::freq_gain(int index, double freq) const
{
    float ret = 1.f;
    for (int i = 0; i < *params[param_filters] * 4; i++)
        ret *= (index == param_amount0 ? L : R)[i].freq_gain(freq, (float)srate);
    return ret;
}
bool multispread_audio_module::get_phase_graph(int index, float ** _buffer, int *_length, int * _mode, bool * _use_fade, float * _fade, int * _accuracy, bool * _display) const {
    *_buffer   = &phase_buffer[0];
    *_length   = plength;
    *_use_fade = 1;
    *_fade     = 0.6;
    *_mode     = 0;
    *_accuracy = 3;
    *_display  = true;
    return false;
}
/**********************************************************************
 * WIDGETS TEST 
**********************************************************************/

widgets_audio_module::widgets_audio_module() {
}
widgets_audio_module::~widgets_audio_module() {
}

void widgets_audio_module::params_changed() {
    
}

uint32_t widgets_audio_module::process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask) {
    float meter_1, meter_2, meter_3, meter_4;
    for(uint32_t i = offset; i < offset + numsamples; i++) {
        meter_1 = 0.f;
        meter_2 = 0.f;
        meter_3 = 0.f;
        meter_4 = 0.f;
        
        //float L = ins[0][i];
        //float R = ins[1][i];
        float values[] = {meter_1, meter_2, meter_3, meter_4};
        meters.process(values);
    }
    meters.fall(numsamples);
    return outputs_mask;
}

void widgets_audio_module::set_sample_rate(uint32_t sr)
{
    srate = sr;
    int meter[] = {param_meter1, param_meter2,  param_meter3, param_meter4};
    int clip[] = {0, 0, 0, 0};
    meters.init(params, meter, clip, 4, sr);
}
