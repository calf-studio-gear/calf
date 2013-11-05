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

using namespace dsp;
using namespace calf_plugins;

#define SET_IF_CONNECTED(name) if (params[AM::param_##name] != NULL) *params[AM::param_##name] = name;
#define sinc(x) (!x) ? 1 : sin(M_PI * x)/(M_PI * x);

/**********************************************************************
 * STEREO TOOLS by Markus Schmidt 
**********************************************************************/

stereo_audio_module::stereo_audio_module() {
    active = false;
    clip_inL    = 0.f;
    clip_inR    = 0.f;
    clip_outL   = 0.f;
    clip_outR   = 0.f;
    meter_inL  = 0.f;
    meter_inR  = 0.f;
    meter_outL = 0.f;
    meter_outR = 0.f;
    _phase = -1;
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
    active = false;
    clip_in    = 0.f;
    clip_outL   = 0.f;
    clip_outR   = 0.f;
    meter_in  = 0.f;
    meter_outL = 0.f;
    meter_outR = 0.f;
    _phase = -1.f;
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

    active = false;
    clip_L   = 0.f;
    clip_R   = 0.f;
    meter_L = 0.f;
    meter_R = 0.f;
    _accuracy = -1;
    _acc_old = -1;
    _scale_old = -1;
    _mode_old = -1;
    _post_old = -1;
    _hold_old = -1;
    _smooth_old = -1;
    ppos = 0;
    plength = 0;
    fpos = 0;
    envelope = 0.f;
    
    
    spline_buffer = (int*) calloc(200, sizeof(int));
    memset(spline_buffer, 0, 200 * sizeof(int)); // reset buffer to zero
    
    phase_buffer = (float*) calloc(max_phase_buffer_size, sizeof(float));
    dsp::zero(phase_buffer, max_phase_buffer_size);
    
    fft_buffer = (float*) calloc(max_fft_buffer_size, sizeof(float));
    
    fft_inL = (float*) calloc(max_fft_cache_size, sizeof(float));
    fft_outL = (float*) calloc(max_fft_cache_size, sizeof(float));
    fft_inR = (float*) calloc(max_fft_cache_size, sizeof(float));
    fft_outR = (float*) calloc(max_fft_cache_size, sizeof(float));
    
    fft_smoothL = (float*) calloc(max_fft_cache_size, sizeof(float));
    fft_smoothR = (float*) calloc(max_fft_cache_size, sizeof(float));
    
    fft_fallingL = (float*) calloc(max_fft_cache_size, sizeof(float));
    fft_fallingR = (float*) calloc(max_fft_cache_size, sizeof(float));
    dsp::fill(fft_fallingL, 1.f, max_fft_cache_size);
    dsp::fill(fft_fallingR, 1.f, max_fft_cache_size);
    
    fft_deltaL = (float*) calloc(max_fft_cache_size, sizeof(float));
    fft_deltaR = (float*) calloc(max_fft_cache_size, sizeof(float));
    
    fft_holdL = (float*) calloc(max_fft_cache_size, sizeof(float));
    fft_holdR = (float*) calloc(max_fft_cache_size, sizeof(float));
    
    fft_freezeL = (float*) calloc(max_fft_cache_size, sizeof(float));
    fft_freezeR = (float*) calloc(max_fft_cache_size, sizeof(float));
    
    fft_plan = NULL;
    
    ____analyzer_phase_was_drawn_here = 0;
    ____analyzer_sanitize = 0;
}

analyzer_audio_module::~analyzer_audio_module()
{
    free(fft_freezeR);
    free(fft_freezeL);
    free(fft_holdR);
    free(fft_holdL);
    free(fft_deltaR);
    free(fft_deltaL);
    free(fft_fallingR);
    free(fft_fallingL);
    free(fft_smoothR);
    free(fft_smoothL);
    free(fft_outR);
    free(fft_outL);
    free(fft_inR);
    free(fft_inL);
    free(phase_buffer);
    free(spline_buffer);
    if (fft_plan)
    {
        fftwf_destroy_plan(fft_plan);
        fft_plan = NULL;
    }
}

void analyzer_audio_module::activate() {
    active = true;
}

void analyzer_audio_module::deactivate() {
    active = false;
}

void analyzer_audio_module::params_changed() {
    bool ___sanitize = false;
    if(*params[param_analyzer_accuracy] != _acc_old) {
        _accuracy = 1 << (7 + (int)*params[param_analyzer_accuracy]);
        _acc_old = *params[param_analyzer_accuracy];
        // recreate fftw plan
        if (fft_plan) fftwf_destroy_plan (fft_plan);
        //fft_plan = rfftw_create_plan(_accuracy, FFTW_FORWARD, 0);
        fft_plan = fftwf_plan_r2r_1d(_accuracy, NULL, NULL, FFTW_R2HC, FFTW_ESTIMATE);
        lintrans = -1;
        ___sanitize = true;
    }
    if(*params[param_analyzer_hold] != _hold_old) {
        _hold_old = *params[param_analyzer_hold];
        ___sanitize = true;
    }
    if(*params[param_analyzer_smoothing] != _smooth_old) {
        _smooth_old = *params[param_analyzer_smoothing];
        ___sanitize = true;
    }
    if(*params[param_analyzer_mode] != _mode_old) {
        _mode_old = *params[param_analyzer_mode];
        ___sanitize = true;
    }
    if(*params[param_analyzer_scale] != _scale_old) {
        _scale_old = *params[param_analyzer_scale];
        ___sanitize = true;
    }
    if(*params[param_analyzer_post] != _post_old) {
        _post_old = *params[param_analyzer_post];
        ___sanitize = true;
    }
    if(___sanitize) {
        // null the overall buffer
        dsp::zero(fft_inL, max_fft_cache_size);
        dsp::zero(fft_inR, max_fft_cache_size);
        dsp::zero(fft_outL, max_fft_cache_size);
        dsp::zero(fft_outR, max_fft_cache_size);
        dsp::zero(fft_holdL, max_fft_cache_size);
        dsp::zero(fft_holdR, max_fft_cache_size);
        dsp::zero(fft_smoothL, max_fft_cache_size);
        dsp::zero(fft_smoothR, max_fft_cache_size);
        dsp::zero(fft_deltaL, max_fft_cache_size);
        dsp::zero(fft_deltaR, max_fft_cache_size);
//        memset(fft_fallingL, 1.f, max_fft_cache_size * sizeof(float));
//        memset(fft_fallingR, 1.f, max_fft_cache_size * sizeof(float));
        dsp::zero(spline_buffer, 200);
        ____analyzer_phase_was_drawn_here = 0;
    }
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
        float lemax = fabs(L) > fabs(R) ? fabs(L) : fabs(R);
        attack_coef = exp(log(0.01)/(0.01 * srate * 0.001));
        release_coef = exp(log(0.01)/(2000 * srate * 0.001));
        if( lemax > envelope)
           envelope = lemax; //attack_coef * (envelope - lemax) + lemax;
        else
           envelope = release_coef * (envelope - lemax) + lemax;
        
        //use the envelope to bring biggest signal to 1. the biggest
        //enlargement of the signal is 4.
        
        phase_buffer[ppos]     = L / std::max(0.25f , (envelope));
        phase_buffer[ppos + 1] = R / std::max(0.25f , (envelope));
        
        
        plength = std::min(phase_buffer_size, plength + 2);
        ppos += 2;
        ppos %= (phase_buffer_size - 2);
        
        // analyzer
        fft_buffer[fpos] = L;
        fft_buffer[fpos + 1] = R;
        
        fpos += 2;
        fpos %= (max_fft_buffer_size - 2);
        
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
}

bool analyzer_audio_module::get_phase_graph(float ** _buffer, int *_length, int * _mode, bool * _use_fade, float * _fade, int * _accuracy, bool * _display) const {
    *_buffer = &phase_buffer[0];
    *_length = plength;
    *_use_fade = *params[param_gonio_use_fade];
    *_fade = *params[param_gonio_fade];
    *_mode = *params[param_gonio_mode];
    *_accuracy = *params[param_gonio_accuracy];
    *_display = *params[param_gonio_display];
    return false;
}

bool analyzer_audio_module::get_graph(int index, int subindex, float *data, int points, cairo_iface *context, int *mode) const
{
    if(____analyzer_sanitize) {
        // null the overall buffer to feed the fft with if someone requested so
        // in the last cycle
        // afterwards quit this call
        dsp::zero(fft_inL, max_fft_cache_size);
        dsp::zero(fft_inR, max_fft_cache_size);
        ____analyzer_sanitize = 0;
        return false;
    }
    
    // just for the stoopid
    int _param_mode = (int)*params[param_analyzer_mode];
    bool _param_hold = (bool)*params[param_analyzer_hold];
    
    if (!active \
        or !*params[param_analyzer_display] \
        
        or (subindex == 1 and !_param_hold and _param_mode < 3) \
        
        or (subindex > 1 and _param_mode < 3) \
        
        or (subindex == 2 and !_param_hold and (_param_mode == 3 \
            or _param_mode == 4)) \
        
        or (subindex == 4 and (_param_mode == 3 or _param_mode == 4)) \
            
        or (subindex == 1 and _param_mode == 5) \
        
        or (subindex == 1 and _param_mode > 5 and _param_mode < 9) \
    ) {
        // stop drawing when all curves have been drawn according to the mode
        // and hold settings
        return false;
    }
    bool fftdone = false; // if fft was renewed, this one is set to true
    double freq; // here the frequency of the actual drawn pixel gets stored
    float stereo_coeff = pow(2, 4 * *params[param_analyzer_level] - 2);
    int iter = 0; // this is the pixel we have been drawing the last box/bar/line
    int _iter = 1; // this is the next pixel we want to draw a box/bar/line
    float posneg = 1;
    int _param_speed = 16 - (int)*params[param_analyzer_speed];
    int _param_smooth = (int)*params[param_analyzer_smoothing];
    if(_param_mode == 5 and _param_smooth) {
        // there's no falling for difference mode, only smoothing
        _param_smooth = 2;
    }
    if(_param_mode > 5 and _param_mode < 9) {
        // there's no smoothing for spectralizer mode
        //_param_smooth = 0;
    }
    
    if(subindex == 0) {
        // #####################################################################
        // We are doing FFT here, so we first have to setup fft-buffers from
        // the main buffer and we use this cycle for filling other buffers
        // like smoothing, delta and hold
        // #####################################################################
        if(!((int)____analyzer_phase_was_drawn_here % _param_speed)) {
            // seems we have to do a fft, so let's read the latest data from the
            // buffer to send it to fft afterwards
            // we want to remember old fft_out values for smoothing as well
            // and we fill the hold buffer in this (extra) cycle
            for(int i = 0; i < _accuracy; i++) {
                // go to the right position back in time according to accuracy
                // settings and cycling in the main buffer
                int _fpos = (fpos - _accuracy * 2 \
                    + (i * 2)) % max_fft_buffer_size;
                if(_fpos < 0)
                    _fpos = max_fft_buffer_size + _fpos;
                float L = fft_buffer[_fpos];
                float R = fft_buffer[_fpos + 1];
                float win = 0.54 - 0.46 * cos(2 * M_PI * i / _accuracy);
                L *= win;
                R *= win;
                
                // #######################################
                // Do some windowing functions on the
                // buffer
                // #######################################
                int _m = 2;
                float _f = 1.f;
                float _a, a0, a1, a2, a3;
                switch((int)*params[param_analyzer_windowing]) {
                    case 0:
                    default:
                        // Linear
                        _f = 1.f;
                        break;
                    case 1:
                        // Hamming
                        _f = 0.54 + 0.46 * cos(2 * M_PI * (i - 2 / points));
                        break;
                    case 2:
                        // von Hann
                        _f = 0.5 * (1 + cos(2 * M_PI * (i - 2 / points)));
                        break;
                    case 3:
                        // Blackman
                        _a = 0.16;
                        a0 = 1.f - _a / 2.f;
                        a1 = 0.5;
                        a2 = _a / 2.f;
                        _f = a0 + a1 * cos((2.f * M_PI * i) / points - 1) + \
                            a2 * cos((4.f * M_PI * i) / points - 1);
                        break;
                    case 4:
                        // Blackman-Harris
                        a0 = 0.35875;
                        a1 = 0.48829;
                        a2 = 0.14128;
                        a3 = 0.01168;
                        _f = a0 - a1 * cos((2.f * M_PI * i) / points - 1) + \
                            a2 * cos((4.f * M_PI * i) / points - 1) - \
                            a3 * cos((6.f * M_PI * i) / points - 1);
                        break;
                    case 5:
                        // Blackman-Nuttall
                        a0 = 0.3653819;
                        a1 = 0.4891775;
                        a2 = 0.1365995;
                        a3 = 0.0106411;
                        _f = a0 - a1 * cos((2.f * M_PI * i) / points - 1) + \
                            a2 * cos((4.f * M_PI * i) / points - 1) - \
                            a3 * cos((6.f * M_PI * i) / points - 1);
                        break;
                    case 6:
                        // Sine
                        _f = sin((M_PI * i) / (points - 1));
                        break;
                    case 7:
                        // Lanczos
                        _f = sinc((2.f * i) / (points - 1) - 1);
                        break;
                    case 8:
                        // Gauß
                        _a = 2.718281828459045;
                        _f = pow(_a, -0.5f * pow((i - (points - 1) / 2) / (0.4 * (points - 1) / 2.f), 2));
                        break;
                    case 9:
                        // Bartlett
                        _f = (2.f / (points - 1)) * (((points - 1) / 2.f) - \
                            fabs(i - ((points - 1) / 2.f)));
                        break;
                    case 10:
                        // Triangular
                        _f = (2.f / points) * ((2.f / points) - fabs(i - ((points - 1) / 2.f)));
                        break;
                    case 11:
                        // Bartlett-Hann
                        a0 = 0.62;
                        a1 = 0.48;
                        a2 = 0.38;
                        _f = a0 - a1 * fabs((i / (points - 1)) - 0.5) - \
                            a2 * cos((2 * M_PI * i) / (points - 1));
                        break;
                }
                L *= _f;
                if(_param_mode > _m)
                    R *= _f;

                // perhaps we need to compute two FFT's, so store left and right
                // channel in case we need only one FFT, the left channel is
                // used as 'standard'"
                float valL;
                float valR;
                
                switch(_param_mode) {
                    case 0:
                    case 6:
                        // average (mode 0)
                        valL = (L + R) / 2;
                        valR = (L + R) / 2;
                        break;
                    case 1:
                    default:
                        // left channel (mode 1)
                        // or both channels (mode 3, 4, 5)
                        valL = L;
                        valR = R;
                        break;
                    case 2:
                    case 8:
                        // right channel (mode 2)
                        valL = R;
                        valR = L;
                        break;
                }
                // store values in analyzer buffer
                fft_inL[i] = valL;
                fft_inR[i] = valR;
                
                // fill smoothing & falling buffer
                if(_param_smooth == 2) {
                    fft_smoothL[i] = fft_outL[i];
                    fft_smoothR[i] = fft_outR[i];
                }
                if(_param_smooth == 1) {
                    if(fft_smoothL[i] < fabs(fft_outL[i])) {
                        fft_smoothL[i] = fabs(fft_outL[i]);
                        fft_deltaL[i] = 1.f;
                    }
                    if(fft_smoothR[i] < fabs(fft_outR[i])) {
                        fft_smoothR[i] = fabs(fft_outR[i]);
                        fft_deltaR[i] = 1.f;
                    }
                }
                
                // fill hold buffer with last out values
                // before fft is recalced
                if(fabs(fft_outL[i]) > fft_holdL[i])
                    fft_holdL[i] = fabs(fft_outL[i]);
                if(fabs(fft_outR[i]) > fft_holdR[i])
                    fft_holdR[i] = fabs(fft_outR[i]);
            }
            
            // run fft
            // this takes our latest buffer and returns an array with
            // non-normalized
            if (fft_plan)
                fftwf_execute_r2r(fft_plan, fft_inL, fft_outL);
            //run fft for for right channel too. it is needed for stereo image 
            //and stereo difference modes
            if(_param_mode >= 3 and fft_plan) {
                fftwf_execute_r2r(fft_plan, fft_inR, fft_outR);
            }
            // ...and set some values for later use
            ____analyzer_phase_was_drawn_here = 0;     
            fftdone = true;  
        }
        ____analyzer_phase_was_drawn_here ++;
    }
    if (lintrans < 0) {
        // accuracy was changed so we have to recalc linear transition
        int _lintrans = (int)((float)points * log((20.f + 2.f * \
            (float)srate / (float)_accuracy) / 20.f) / log(1000.f));  
        lintrans = (int)(_lintrans + points % _lintrans / \
            floor(points / _lintrans)) / 2; // / 4 was added to see finer bars but breaks low end
    }
    for (int i = 0; i <= points; i++)
    {
        // #####################################################################
        // Real business starts here. We will cycle through all pixels in
        // x-direction of the line-graph and decide what to show
        // #####################################################################
        // cycle through the points to draw
        // we need to know the exact frequency at this pixel
        freq = 20.f * pow (1000.f, (float)i / points);
        
        // we need to know the last drawn value over the time
        float lastoutL = 0.f; 
        float lastoutR = 0.f;
        
        // let's see how many pixels we may want to skip until the drawing
        // function has to draw a bar/box/line
        if(*params[param_analyzer_scale] or *params[param_analyzer_view] == 2) {
            // we have linear view enabled or we want to see tit... erm curves
            if((i % lintrans == 0 and points - i > lintrans) or i == points - 1) {
                _iter = std::max(1, (int)floor(freq * \
                    (float)_accuracy / (float)srate));
            }    
        } else {
            // we have logarithmic view enabled
            _iter = std::max(1, (int)floor(freq * (float)_accuracy / (float)srate));
        }
        if(_iter > iter) {
            // we are flipping one step further in drawing
            if(fftdone and i) {
                // ################################
                // Manipulate the fft_out values
                // according to the post processing
                // ################################
                int n = 0;
                float var1L = 0.f; // used later for denoising peaks
                float var1R = 0.f;
                float diff_fft;
                switch(_param_mode) {
                    default:
                        // all normal modes
                        posneg = 1;
                        // only if we don't see difference mode
                        switch((int)*params[param_analyzer_post]) {
                            case 0:
                                // Analyzer Normalized - nothing to do
                                break;
                            case 1:
                                // Analyzer Additive - cycle through skipped values and
                                // add them
                                // if fft was renewed, recalc the absolute values if
                                // frequencies are skipped
                                for(int j = iter + 1; j < _iter; j++) {
                                    fft_outL[_iter] += fabs(fft_outL[j]);
                                    fft_outR[_iter] += fabs(fft_outR[j]);
                                }
                                fft_outL[_iter] /= (_iter - iter);
                                fft_outR[_iter] /= (_iter - iter);
                                break;
                            case 2:
                                // Analyzer Additive - cycle through skipped values and
                                // add them
                                // if fft was renewed, recalc the absolute values if
                                // frequencies are skipped
                                for(int j = iter + 1; j < _iter; j++) {
                                    fft_outL[_iter] += fabs(fft_outL[j]);
                                    fft_outR[_iter] += fabs(fft_outR[j]);
                                }
                                break;
                            case 3:
                                // Analyzer Denoised Peaks - filter out unwanted noise
                                for(int k = 0; k < std::max(10 , std::min(400 ,\
                                    (int)(2.f*(float)((_iter - iter))))); k++) {
                                    //collect amplitudes in the environment of _iter to
                                    //be able to erase them from signal and leave just
                                    //the peaks
                                    if(_iter - k > 0) {
                                        var1L += fabs(fft_outL[_iter - k]);
                                        n++;
                                    }
                                    if(k != 0) var1L += fabs(fft_outL[_iter + k]);
                                    else if(i) var1L += fabs(lastoutL);
                                    else var1L += fabs(fft_outL[_iter]);
                                    if(_param_mode == 3 or _param_mode == 4) {
                                        if(_iter - k > 0) {
                                            var1R += fabs(fft_outR[_iter - k]);
                                            n++;
                                        }
                                        if(k != 0) var1R += fabs(fft_outR[_iter + k]);
                                        else if(i) var1R += fabs(lastoutR);
                                        else var1R += fabs(fft_outR[_iter]);
                                    }
                                    n++;
                                }
                                //do not forget fft_out[_iter] for the next time
                                lastoutL = fft_outL[_iter];
                                //pumping up actual signal an erase surrounding
                                // sounds
                                fft_outL[_iter] = 0.25f * std::max(n * 0.6f * \
                                    fabs(fft_outL[_iter]) - var1L , 1e-20);
                                if(_param_mode == 3 or _param_mode == 4) {
                                    // do the same with R channel if needed
                                    lastoutR = fft_outR[_iter];
                                    fft_outR[_iter] = 0.25f * std::max(n * \
                                        0.6f * fabs(fft_outR[_iter]) - var1R , 1e-20);
                                }
                                break;
                        }
                        break;
                    case 5:
                        // Stereo Difference - draw the difference between left
                        // and right channel if fft was renewed, recalc the
                        // absolute values in left and right if frequencies are
                        // skipped.
                        // this is additive mode - no other mode is available
                        //for(int j = iter + 1; j < _iter; j++) {
                        //    fft_outL[_iter] += fabs(fft_outL[j]);
                        //    fft_outR[_iter] += fabs(fft_outR[j]);
                        //}
                        //calculate difference between left an right channel                        
                        diff_fft = fabs(fft_outL[_iter]) - fabs(fft_outR[_iter]);
                        posneg = fabs(diff_fft) / diff_fft;
                        //fft_outL[_iter] = diff_fft / _accuracy;
                        break;
                }
            }
            iter = _iter;
            // #######################################
            // Calculate transitions for falling and
            // smooting and fill delta buffers if fft
            // was done above
            // #######################################
            if(subindex == 0) {
                float _fdelta = 0.91;
                if(_param_mode > 5 and _param_mode < 9)
                    _fdelta = .99f;
                float _ffactor = 2000.f;
                if(_param_mode > 5 and _param_mode < 9)
                    _ffactor = 50.f;
                if(_param_smooth == 2) {
                    // smoothing
                    if(fftdone) {
                        // rebuild delta values after fft was done
                        if(_param_mode < 5 or _param_mode > 5) {
                            fft_deltaL[iter] = pow(fabs(fft_outL[iter]) / fabs(fft_smoothL[iter]), 1.f / _param_speed);
                        } else {
                            fft_deltaL[iter] = (posneg * fabs(fft_outL[iter]) - fft_smoothL[iter]) / _param_speed;
                        }
                    } else {
                        // change fft_smooth according to delta
                        if(_param_mode < 5 or _param_mode > 5) {
                            fft_smoothL[iter] *= fft_deltaL[iter];
                        } else {
                            fft_smoothL[iter] += fft_deltaL[iter];
                        }
                    }
                } else if(_param_smooth == 1) {
                    // falling
                    if(fftdone) {
                        // rebuild delta values after fft was done
                        //fft_deltaL[iter] = _fdelta;
                    }
                    // change fft_smooth according to delta
                    fft_smoothL[iter] *= fft_deltaL[iter];
                    
                    if(fft_deltaL[iter] > _fdelta) {
                        fft_deltaL[iter] *= 1.f - (16.f - _param_speed) / _ffactor;
                    }
                }
                
                if(_param_mode > 2 and _param_mode < 5) {
                    // we need right buffers, too for stereo image and
                    // stereo analyzer
                    if(_param_smooth == 2) {
                        // smoothing
                        if(fftdone) {
                            // rebuild delta values after fft was done
                            if(_param_mode < 5) {
                                fft_deltaR[iter] = pow(fabs(fft_outR[iter]) / fabs(fft_smoothR[iter]), 1.f / _param_speed);
                            } else {
                                fft_deltaR[iter] = (posneg * fabs(fft_outR[iter]) - fft_smoothR[iter]) / _param_speed;
                            }
                        } else {
                            // change fft_smooth according to delta
                            if(_param_mode < 5) {
                                fft_smoothR[iter] *= fft_deltaR[iter];
                            } else {
                                fft_smoothR[iter] += fft_deltaR[iter];
                            }
                        }
                    } else if(_param_smooth == 1) {
                        // falling
                        if(fftdone) {
                            // rebuild delta values after fft was done
                            //fft_deltaR[iter] = _fdelta;
                        }
                        // change fft_smooth according to delta
                        fft_smoothR[iter] *= fft_deltaR[iter];
                        if(fft_deltaR[iter] > _fdelta)
                            fft_deltaR[iter] *= 1.f - (16.f - _param_speed) / 2000.f;
                    }
                }
            }
            // #######################################
            // Choose the L and R value from the right
            // buffer according to view settings
            // #######################################
            float valL = 0.f;
            float valR = 0.f;
            if (*params[param_analyzer_freeze]) {
                // freeze enabled
                valL = fft_freezeL[iter];
                valR = fft_freezeR[iter];
            } else if ((subindex == 1 and _param_mode < 3) \
                or subindex > 1 \
                or (_param_mode > 5 and *params[param_analyzer_hold])) {
                // we draw the hold buffer
                valL = fft_holdL[iter];
                valR = fft_holdR[iter];
            } else {
                // we draw normally (no freeze)
                switch(_param_smooth) {
                    case 0:
                        // off
                        valL = fft_outL[iter];
                        valR = fft_outR[iter];
                        break;
                    case 1:
                        // falling
                        valL = fft_smoothL[iter];
                        valR = fft_smoothR[iter];
                        break;
                    case 2:
                        // smoothing
                        valL = fft_smoothL[iter];
                        valR = fft_smoothR[iter];
                        break;
                }
                // fill freeze buffer
                fft_freezeL[iter] = valL;
                fft_freezeR[iter] = valR;
            }
            if(*params[param_analyzer_view] < 2) {
                // #####################################
                // send values back to painting function
                // according to mode setting but only if
                // we are drawing lines or boxes
                // #####################################
                float dummy;
                switch(_param_mode) {
                    case 3:
                        // stereo analyzer
                        if(subindex == 0 or subindex == 2) {
                            data[i] = dB_grid(fabs(valL) / _accuracy * 2.f + 1e-20, pow(64, *params[param_analyzer_level]), 0.5f);
                        } else {
                            data[i] = dB_grid(fabs(valR) / _accuracy * 2.f + 1e-20, pow(64, *params[param_analyzer_level]), 0.5f);
                        }
                        break;
                    case 4:
                        // we want to draw Stereo Image
                        if(subindex == 0 or subindex == 2) {
                            // Left channel signal
                            dummy = dB_grid(fabs(valL) / _accuracy * 2.f + 1e-20, pow(64, *params[param_analyzer_level]), 1.f);
                            //only signals above the middle are interesting
                            data[i] = dummy < 0 ? 0 : dummy;
                        } else if (subindex == 1 or subindex == 3) {
                            // Right channel signal
                            dummy = dB_grid(fabs(valR) / _accuracy * 2.f + 1e-20, pow(64, *params[param_analyzer_level]), 1.f);
                            //only signals above the middle are interesting. after cutting away
                            //the unneeded stuff, the curve is flipped vertical at the middle.
                            if(dummy < 0) dummy = 0;
                            data[i] = -1.f * dummy;
                        }
                        break;
                    case 5:
                        // We want to draw Stereo Difference
                        if(i) {
                            float levelanpassung = *params[param_analyzer_level] > 1 ? 1 + (*params[param_analyzer_level] - 1) / 4 : *params[param_analyzer_level];
                            dummy = dB_grid(fabs((fabs(valL) - fabs(valR))) / _accuracy * 2.f +  1e-20, pow(64, 2 * levelanpassung), 1.f / levelanpassung);
                            //only show differences above a threshhold which results from the db_grid-calculation
                            if (dummy < 0) dummy=0;
                            //bring right signals below the middle
                            dummy *= fabs(valL) < fabs(valR) ? -1.f : 1.f;
                            data[i] = dummy;
                        }
                        else data[i] = 0.f;
                        break;
                    default:
                        // normal analyzer behavior
                        data[i] = dB_grid(fabs(valL) / _accuracy * 2.f + 1e-20, pow(64, *params[param_analyzer_level]), 0.5f);
                        break;
                }
            }
        }
        else if(*params[param_analyzer_view] == 2) {
            // we have to draw splines, so we draw every x-pixel according to
            // the pre-generated fft_splinesL and fft_splinesR buffers
            data[i] = INFINITY;
            
//            int _splinepos = -1;
//            *mode=0;
//            
//            for (int i = 0; i<=points; i++) {
//                if (subindex == 1 and i == spline_buffer[_splinepos]) _splinepos++;
//                
//                freq = 20.f * pow (1000.f, (float)i / points); //1000=20000/20
//                float a0,b0,c0,d0,a1,b1,c1,d1,a2,b2,c2,d2;
//                
//                if(((i % lintrans == 0 and points - i > lintrans) or i == points - 1 ) and subindex == 0) {

//                    _iter = std::max(1, (int)floor(freq * (float)_accuracy / (float)srate));
//                    //printf("_iter %3d\n",_iter);
//                }
//                if(_iter > iter and subindex == 0)
//                {
//                    _splinepos++;
//                    spline_buffer[_splinepos] = _iter;
//                    //printf("_splinepos: %3d - lintrans: %3d\n", _splinepos,lintrans);
//                    if(fftdone and i and subindex == 0) 
//                    {
//                        // if fft was renewed, recalc the absolute values if frequencies
//                        // are skipped
//                        for(int j = iter + 1; j < _iter; j++) {
//                            fft_out[_iter] += fabs(fft_out[j]);
//                        }
//                    }
//                }
//                
//                if(fftdone and subindex == 1 and _splinepos >= 0 and (_splinepos % 3 == 0 or _splinepos == 0)) 
//                {
//                    float mleft, mright, y0, y1, y2, y3, y4;
//                    
//                    //jetzt spline interpolation
//                    y0 = dB_grid(fft_out[spline_buffer[_splinepos]] / _accuracy * 2.f + 1e-20, pow(64, *params[param_analyzer_level]), 0.5f);
//                    y1 = dB_grid(fft_out[spline_buffer[_splinepos + 1]] / _accuracy * 2.f + 1e-20, pow(64, *params[param_analyzer_level]), 0.5f);
//                    y2 = dB_grid(fft_out[spline_buffer[_splinepos + 2]] / _accuracy * 2.f + 1e-20, pow(64, *params[param_analyzer_level]), 0.5f);
//                    y3 = dB_grid(fft_out[spline_buffer[_splinepos + 3]] / _accuracy * 2.f + 1e-20, pow(64, *params[param_analyzer_level]), 0.5f);
//                    y4 = dB_grid(fft_out[spline_buffer[_splinepos + 4]] / _accuracy * 2.f + 1e-20, pow(64, *params[param_analyzer_level]), 0.5f);
//                    mleft  = y1 - y0;
//                    mright = y4 - y3;
//                    printf("y-werte %3d, %3d, %3d, %3d, %3d\n",y0,y1,y2,y3,y4);
//                    a0 = (-3*y3+15*y2-44*y1+32*y0+mright+22*mleft)/34;
//                    b0 = -(-3*y3+15*y2-78*y1+66*y0+mright+56*mleft)/34;
//                    c0 = mleft;
//                    d0 = y0;
//                    a1 = -(-15*y3+41*y2-50*y1+24*y0+5*mright+8*mleft)/34;
//                    b1 = (-6*y3+30*y2-54*y1+30*y0+2*mright+10*mleft)/17;
//                    c1 = -(3*y3-15*y2-24*y1+36*y0-mright+12*mleft)/34;
//                    d1 = y1;
//                    a2 = (-25*y3+40*y2-21*y1+6*y0+14*mright+2*mleft)/17;
//                    b2 = -(-33*y3+63*y2-42*y1+12*y0+11*mright+4*mleft)/17;
//                    c2 = (9*y3+6*y2-21*y1+6*y0-3*mright+2*mleft)/17;
//                    d2 = y2;
//                }
//                iter = _iter;
//                
//                
//                if(i > spline_buffer[_splinepos] and i <= spline_buffer[_splinepos + 1] and _splinepos >= 0 and subindex == 1) 
//                {
//                data[i] = a0 * pow(i / lintrans - _splinepos, 3) + b0 * pow(i / lintrans - _splinepos, 2) + c0 * (i / lintrans - _splinepos) + d0;
//                printf("1.spline\n");
//                }
//                if(i > spline_buffer[_splinepos + 1] and i <= spline_buffer[_splinepos + 2] and _splinepos >= 0 and subindex == 1) 
//                {
//                printf("2.spline\n");
//                data[i] = a1 * pow(i / lintrans - _splinepos, 3) + b1 * pow(i / lintrans - _splinepos, 2) + c1 * (i / lintrans - _splinepos) + d1;
//                }
//                if(i > spline_buffer[_splinepos + 2] and i <= spline_buffer[_splinepos + 3] and _splinepos >= 0 and subindex == 1) 
//                {
//                printf("3.spline\n");
//                data[i] = a2 * pow(i / lintrans - _splinepos, 3) + b2 * pow(i / lintrans - _splinepos, 2) + c2 * (i / lintrans - _splinepos) + d2;
//                }
//                if(subindex==1) printf("data[i] %3d _splinepos %2d\n", data[i], _splinepos);
//                if (subindex == 0) 
//                {
//                    data[i] = INFINITY;
//                } else data[i] = dB_grid(i/200, pow(64, *params[param_analyzer_level]), 0.5f);
//            
//            }
            
        }
        else {
            data[i] = INFINITY;
        }
    }
    
    // ###################################
    // colorize the curves/boxes according
    // to the chosen display settings
    // ###################################
    
    // change alpha (or color) for hold lines or stereo modes
    if((subindex == 1 and _param_mode < 3) or (subindex > 1 and _param_mode == 4)) {
        // subtle hold line on left, right, average or stereo image
        context->set_source_rgba(0.35, 0.4, 0.2, 0.2);
    }
    if(subindex == 0 and _param_mode == 3) {
        // left channel in stereo analyzer
        context->set_source_rgba(0.25, 0.10, 0.0, 0.3);
    }
    if(subindex == 1 and _param_mode == 3) {
        // right channel in stereo analyzer
        context->set_source_rgba(0.05, 0.25, 0.0, 0.3);
    }
    if(subindex == 2 and _param_mode == 3) {
        // left hold in stereo analyzer
        context->set_source_rgba(0.45, 0.30, 0.2, 0.2);
    }
    if(subindex == 3 and _param_mode == 3) {
        // right hold in stereo analyzer
        context->set_source_rgba(0.25, 0.45, 0.2, 0.2);
    }
    
    // #############################
    // choose a drawing mode between
    // boxes, lines and bars
    // #############################
    
    // modes:
    // 0: left
    // 1: right
    // 2: average
    // 3: stereo
    // 4: image
    // 5: difference
    
    // views:
    // 0: bars
    // 1: lines
    // 2: cubic splines
    
    // *modes to set:
    // 0: lines
    // 1: bars
    // 2: boxes (little things on the values position
    // 3: centered bars (0dB is centered in y direction)
    
    if (_param_mode > 3 and _param_mode < 6) {
        // centered viewing modes like stereo image and stereo difference
        if(!*params[param_analyzer_view]) {
            // boxes
            if(subindex > 1) {
                // boxes (hold)
                *mode = 2;
            } else {
                // bars (signal)
                *mode = 3;
            }
        } else {
            // lines
            *mode = 0;
        }
    } else if (_param_mode > 5 and _param_mode < 9) {
        // spectrum analyzer
        *mode = 4;
    } else if(!*params[param_analyzer_view]) {
        // bars
        if((subindex == 0 and _param_mode < 3) or (subindex <= 1 and _param_mode == 3)) {
            // draw bars
            *mode = 1;
        } else {
            // draw boxes
            *mode = 2;
        }
    } else {
        // draw lines
        *mode = 0;
    }
    ____analyzer_sanitize = 0;
    return true;
}

bool analyzer_audio_module::get_gridline(int index, int subindex, float &pos, bool &vertical, std::string &legend, cairo_iface *context) const
{ 
    bool out;
    if(*params[param_analyzer_mode] <= 3)
        out = get_freq_gridline(subindex, pos, vertical, legend, context, true, pow(64, *params[param_analyzer_level]), 0.5f);
    else if (*params[param_analyzer_mode] < 5)
        out = get_freq_gridline(subindex, pos, vertical, legend, context, true, pow(64, *params[param_analyzer_level]), 1);
    else if (*params[param_analyzer_mode] < 6) {
        
        //create a grid in which the middle is fix while rest is sizeable
        float levelanpassung = *params[param_analyzer_level] > 1 ? 1 + (*params[param_analyzer_level] - 1) / 4 : *params[param_analyzer_level];
        out = get_freq_gridline(subindex, pos, vertical, legend, context, true, pow(64, 2 * levelanpassung), 1.f / levelanpassung);
        }
    else if (*params[param_analyzer_mode] < 9)
        out = get_freq_gridline(subindex, pos, vertical, legend, context, true, 0, 1.1f);
    else
        out = false;
        
    if(*params[param_analyzer_mode] > 3 and *params[param_analyzer_mode] < 6 and not vertical) {
        if(subindex == 30)
            legend="L";
        else if(subindex == 34)
            legend="R";
    //    else
    //        legend = "";
    }
    if(*params[param_analyzer_mode] > 5 and *params[param_analyzer_mode] < 9 and not vertical) {
        legend = "";
    }
    return out;
}
int analyzer_audio_module::get_changed_offsets(int index, int generation, int force_cache, int &subindex_graph, int &subindex_dot, int &subindex_gridline) const
{
    subindex_graph = 0;
    if(*params[param_analyzer_mode] != _mode_old or *params[param_analyzer_level]) {
        _mode_old = *params[param_analyzer_mode];
        return generation++;
    }
    return generation;
}
