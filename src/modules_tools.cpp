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
#include <fftw3.h>
#include <calf/giface.h>
#include <calf/modules_tools.h>
#include <calf/modules_dev.h>
#include <sys/time.h>
#include <calf/utils.h>


using namespace dsp;
using namespace calf_plugins;

#define SET_IF_CONNECTED(name) if (params[AM::param_##name] != NULL) *params[AM::param_##name] = name;
#define sinc(x) (!x) ? 1 : sin(M_PI * x)/(M_PI * x);

/**********************************************************************
 * STEREO TOOLS by Markus Schmidt 
**********************************************************************/

stereo_audio_module::stereo_audio_module() {
    active      = false;
    clip_inL    = 0.f;
    clip_inR    = 0.f;
    clip_outL   = 0.f;
    clip_outR   = 0.f;
    meter_inL   = 0.f;
    meter_inR   = 0.f;
    meter_outL  = 0.f;
    meter_outR  = 0.f;
    _phase      = -1;
}

void stereo_audio_module::activate() {
    active = true;
}

void stereo_audio_module::deactivate() {
    active = false;
}

void stereo_audio_module::params_changed() {
    float slev = 2 * *params[param_slev]; // stereo level ( -2 -> 2 )
    float sbal = 1 + *params[param_sbal]; // stereo balance ( 0 -> 2 )
    float mlev = 2 * *params[param_mlev]; // mono level ( -2 -> 2 )
    float mpan = 1 + *params[param_mpan]; // mono pan ( 0 -> 2 )

    switch((int)*params[param_mode])
    {
        case 0:
        default:
            //LR->LR
            LL = (mlev * (2.f - mpan) + slev * (2.f - sbal));
            LR = (mlev * mpan - slev * sbal);
            RL = (mlev * (2.f - mpan) - slev * (2.f - sbal));
            RR = (mlev * mpan + slev * sbal);
            break;
        case 1:
            //LR->MS
            LL = (2.f - mpan) * (2.f - sbal);
            LR = mpan * (2.f - sbal) * -1;
            RL = (2.f - mpan) * sbal;
            RR = mpan * sbal;
            break;
        case 2:
            //MS->LR
            LL = mlev * (2.f - sbal);
            LR = mlev * mpan;
            RL = slev * (2.f - sbal);
            RR = slev * sbal * -1;
            break;
        case 3:
        case 4:
        case 5:
        case 6:
            //LR->LL
            LL = 0.f;
            LR = 0.f;
            RL = 0.f;
            RR = 0.f;
            break;
    }
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
    for(uint32_t i = offset; i < offset + numsamples; i++) {
        if(*params[param_bypass] > 0.5) {
            outs[0][i] = ins[0][i];
            outs[1][i] = ins[1][i];
            clip_inL    = 0.f;
            clip_inR    = 0.f;
            clip_outL   = 0.f;
            clip_outR   = 0.f;
            meter_inL  = 0.f;
            meter_inR  = 0.f;
            meter_outL = 0.f;
            meter_outR = 0.f;
        } else {
            // let meters fall a bit
            clip_inL    -= std::min(clip_inL,  numsamples);
            clip_inR    -= std::min(clip_inR,  numsamples);
            clip_outL   -= std::min(clip_outL, numsamples);
            clip_outR   -= std::min(clip_outR, numsamples);
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
            
            // copy / flip / mono ...
            switch((int)*params[param_mode])
            {
                case 0:
                default:
                    // LR > LR
                    break;
                case 1:
                    // LR > MS
                    break;
                case 2:
                    // MS > LR
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
                    float tmp = L;
                    L = R;
                    R = tmp;
                    break;
            }
            
            // softclip
            if(*params[param_softclip]) {
//                int ph;
//                ph = L / fabs(L);
//                L = L > 0.63 ? ph * (0.63 + 0.36 * (1 - pow(MATH_E, (1.f / 3) * (0.63 + L * ph)))) : L;
//                ph = R / fabs(R);
//                R = R > 0.63 ? ph * (0.63 + 0.36 * (1 - pow(MATH_E, (1.f / 3) * (0.63 + R * ph)))) : R;
                R = _inv_atan_shape * atan(R * _sc_level);
                L = _inv_atan_shape * atan(L * _sc_level);
            }
            
            // GUI stuff
            if(L > meter_inL) meter_inL = L;
            if(R > meter_inR) meter_inR = R;
            if(L > 1.f) clip_inL  = srate >> 3;
            if(R > 1.f) clip_inR  = srate >> 3;
            
            // mute
            L *= (1 - floor(*params[param_mute_l] + 0.5));
            R *= (1 - floor(*params[param_mute_r] + 0.5));
            
            // phase
            L *= (2 * (1 - floor(*params[param_phase_l] + 0.5))) - 1;
            R *= (2 * (1 - floor(*params[param_phase_r] + 0.5))) - 1;
            
            // LR/MS
            L += LL*L + RL*R;
            R += RR*R + LR*L;
            
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
            
            // clip LED's
            if(L > 1.f) clip_outL = srate >> 3;
            if(R > 1.f) clip_outR = srate >> 3;
            if(L > meter_outL) meter_outL = L;
            if(R > meter_outR) meter_outR = R;
            
            // phase meter
            if(fabs(L) > 0.001 and fabs(R) > 0.001) {
                meter_phase = fabs(fabs(L+R) > 0.000000001 ? sin(fabs((L-R)/(L+R))) : 0.f);
            } else {
                meter_phase = 0.f;
            }
        }
    }
    // draw meters
    SET_IF_CONNECTED(clip_inL);
    SET_IF_CONNECTED(clip_inR);
    SET_IF_CONNECTED(clip_outL);
    SET_IF_CONNECTED(clip_outR);
    SET_IF_CONNECTED(meter_inL);
    SET_IF_CONNECTED(meter_inR);
    SET_IF_CONNECTED(meter_outL);
    SET_IF_CONNECTED(meter_outR);
    SET_IF_CONNECTED(meter_phase);
    return outputs_mask;
}

void stereo_audio_module::set_sample_rate(uint32_t sr)
{
    srate = sr;
    // rebuild buffer
    buffer_size = (int)(srate * 0.05 * 2.f); // buffer size attack rate multiplied by 2 channels
    buffer = (float*) calloc(buffer_size, sizeof(float));
    dsp::zero(buffer, buffer_size); // reset buffer to zero
    pos = 0;
}

/**********************************************************************
 * MONO INPUT by Markus Schmidt
**********************************************************************/

mono_audio_module::mono_audio_module() {
    active      = false;
    clip_in     = 0.f;
    clip_outL   = 0.f;
    clip_outR   = 0.f;
    meter_in    = 0.f;
    meter_outL  = 0.f;
    meter_outR  = 0.f;
    _phase      = -1.f;
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
    for(uint32_t i = offset; i < offset + numsamples; i++) {
        if(*params[param_bypass] > 0.5) {
            outs[0][i] = ins[0][i];
            outs[1][i] = ins[0][i];
            clip_in     = 0.f;
            clip_outL   = 0.f;
            clip_outR   = 0.f;
            meter_in    = 0.f;
            meter_outL  = 0.f;
            meter_outR  = 0.f;
        } else {
            // let meters fall a bit
            clip_in     -= std::min(clip_in,  numsamples);
            clip_outL   -= std::min(clip_outL, numsamples);
            clip_outR   -= std::min(clip_outR, numsamples);
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
            if(L > meter_in) meter_in = L;
            if(L > 1.f) clip_in = srate >> 3;
            
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
            
            // clip LED's
            if(L > 1.f) clip_outL = srate >> 3;
            if(R > 1.f) clip_outR = srate >> 3;
            if(L > meter_outL) meter_outL = L;
            if(R > meter_outR) meter_outR = R;
        }
    }
    // draw meters
    SET_IF_CONNECTED(clip_in);
    SET_IF_CONNECTED(clip_outL);
    SET_IF_CONNECTED(clip_outR);
    SET_IF_CONNECTED(meter_in);
    SET_IF_CONNECTED(meter_outL);
    SET_IF_CONNECTED(meter_outR);
    return outputs_mask;
}

void mono_audio_module::set_sample_rate(uint32_t sr)
{
    srate = sr;
    // rebuild buffer
    buffer_size = (int)srate * 0.05 * 2; // delay buffer size multiplied by 2 channels
    buffer = (float*) calloc(buffer_size, sizeof(float));
    dsp::zero(buffer, buffer_size); // reset buffer to zero
    pos = 0;
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
    dsp::zero(phase_buffer, max_phase_buffer_size);
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
        float lemax  = fabs(L) > fabs(R) ? fabs(L) : fabs(R);
        attack_coef  = exp(log(0.01)/(0.01 * srate * 0.001));
        release_coef = exp(log(0.01)/(2000 * srate * 0.001));
        if( lemax > envelope)
           envelope  = lemax; //attack_coef * (envelope - lemax) + lemax;
        else
           envelope  = release_coef * (envelope - lemax) + lemax;
        
        //use the envelope to bring biggest signal to 1. the biggest
        //enlargement of the signal is 4.
        
        phase_buffer[ppos]     = L / std::max(0.25f , (envelope));
        phase_buffer[ppos + 1] = R / std::max(0.25f , (envelope));
        
        
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
}

bool analyzer_audio_module::get_phase_graph(float ** _buffer, int *_length, int * _mode, bool * _use_fade, float * _fade, int * _accuracy, bool * _display) const {
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
