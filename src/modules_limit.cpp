/* Calf DSP plugin pack
 * Limiter related plugins
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
#include <calf/giface.h>
#include <calf/modules_limit.h>

using namespace dsp;
using namespace calf_plugins;

#define SET_IF_CONNECTED(name) if (params[AM::param_##name] != NULL) *params[AM::param_##name] = name;

/// Limiter by Markus Schmidt
///
/// This module splits the signal in four different bands
/// and sends them through multiple filters (implemented by
/// Krzysztof). They are processed by a compressing routine
/// (implemented by Steve Harris) afterwards and summed up to the
/// final output again.
///////////////////////////////////////////////////////////////////////////////////////////////

limiter_audio_module::limiter_audio_module()
{
    is_active = false;
    srate = 0;
    // zero all displays
    clip_inL    = 0.f;
    clip_inR    = 0.f;
    clip_outL   = 0.f;
    clip_outR   = 0.f;
    meter_inL  = 0.f;
    meter_inR  = 0.f;
    meter_outL = 0.f;
    meter_outR = 0.f;
}

void limiter_audio_module::activate()
{
    is_active = true;
    // set all filters and strips
    params_changed();
    limiter.activate();
}

void limiter_audio_module::deactivate()
{
    is_active = false;
    limiter.deactivate();
}

void limiter_audio_module::params_changed()
{
    limiter.set_params(*params[param_limit], *params[param_release], 1.f, srate);
}

void limiter_audio_module::set_sample_rate(uint32_t sr)
{
    srate = sr;
    limiter.set_sample_rate(srate);
}

uint32_t limiter_audio_module::process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask)
{
    bool bypass = *params[param_bypass] > 0.5f;
    numsamples += offset;
    if(bypass) {
        // everything bypassed
        while(offset < numsamples) {
            outs[0][offset] = ins[0][offset];
            outs[1][offset] = ins[1][offset];
            ++offset;
        }
        // displays, too
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
        while(offset < numsamples) {
            // cycle through samples
            float inL = ins[0][offset];
            float inR = ins[1][offset];
            // in level
            inR *= *params[param_level_in];
            inL *= *params[param_level_in];
            // out vars
            float outL = inL;
            float outR = inR;
            
            // process gain reduction
            limiter.process(outL, outR);
            
            // out level
            outL *= *params[param_level_out];
            outR *= *params[param_level_out];
            
            // send to output
            outs[0][offset] = outL;
            outs[1][offset] = outR;
            
            // clip LED's
            if(inL > 1.f) {
                clip_inL  = srate >> 3;
            }
            if(inR > 1.f) {
                clip_inR  = srate >> 3;
            }
            if(outL > 1.f) {
                clip_outL = srate >> 3;
            }
            if(outR > 1.f) {
                clip_outR = srate >> 3;
            }
            // set up in / out meters
            if(inL > meter_inL) {
                meter_inL = inL;
            }
            if(inR > meter_inR) {
                meter_inR = inR;
            }
            if(outL > meter_outL) {
                meter_outL = outL;
            }
            if(outR > meter_outR) {
                meter_outR = outR;
            }
            // next sample
            ++offset;
        } // cycle trough samples
        
    } // process (no bypass)
    
    // draw meters
    SET_IF_CONNECTED(clip_inL);
    SET_IF_CONNECTED(clip_inR);
    SET_IF_CONNECTED(clip_outL);
    SET_IF_CONNECTED(clip_outR);
    SET_IF_CONNECTED(meter_inL);
    SET_IF_CONNECTED(meter_inR);
    SET_IF_CONNECTED(meter_outL);
    SET_IF_CONNECTED(meter_outR);
    
    if (*params[param_att]) {
        if(bypass)
            *params[param_att] = 1.f;
        else
            *params[param_att] = limiter.get_attenuation();
    }
    
    // whatever has to be returned x)
    return outputs_mask;
}

/// Multiband Limiter by Markus Schmidt
///
/// This module splits the signal in four different bands
/// and sends them through multiple filters (implemented by
/// Krzysztof). They are processed by a compressing routine
/// (implemented by Steve Harris) afterwards and summed up to the
/// final output again.
///////////////////////////////////////////////////////////////////////////////////////////////

multibandlimiter_audio_module::multibandlimiter_audio_module()
{
    is_active = false;
    srate = 0;
    // zero all displays
    clip_inL    = 0.f;
    clip_inR    = 0.f;
    clip_outL   = 0.f;
    clip_outR   = 0.f;
    meter_inL  = 0.f;
    meter_inR  = 0.f;
    meter_outL = 0.f;
    meter_outR = 0.f;
}

void multibandlimiter_audio_module::activate()
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

void multibandlimiter_audio_module::deactivate()
{
    is_active = false;
    // deactivate all strips
    for (int j = 0; j < strips; j ++) {
        strip[j].deactivate();
    }
}

void multibandlimiter_audio_module::params_changed()
{
    mode_old = mode;
    mode = *params[param_mode];
    int i;
    int j1;
    switch(mode) {
        case 0:
            j1 = 0;
            break;
        case 1:
            j1 = 2;
            break;
    }
    // set the params of all filters
    if(*params[param_freq0] != freq_old[0] or *params[param_sep0] != sep_old[0] or *params[param_q0] != q_old[0] or *params[param_mode] != mode_old) {
        lpL[0][0].set_lp_rbj((float)(*params[param_freq0] * (1 - *params[param_sep0])), *params[param_q0], (float)srate);
        hpL[0][0].set_hp_rbj((float)(*params[param_freq0] * (1 + *params[param_sep0])), *params[param_q0], (float)srate);
        lpR[0][0].copy_coeffs(lpL[0][0]);
        hpR[0][0].copy_coeffs(hpL[0][0]);
        for(i = 1; i <= j1; i++) {
            lpL[0][i].copy_coeffs(lpL[0][0]);
            hpL[0][i].copy_coeffs(hpL[0][0]);
            lpR[0][i].copy_coeffs(lpL[0][0]);
            hpR[0][i].copy_coeffs(hpL[0][0]);
        }
        freq_old[0] = *params[param_freq0];
        sep_old[0]  = *params[param_sep0];
        q_old[0]    = *params[param_q0];
    }
    if(*params[param_freq1] != freq_old[1] or *params[param_sep1] != sep_old[1] or *params[param_q1] != q_old[1] or *params[param_mode] != mode_old) {
        lpL[1][0].set_lp_rbj((float)(*params[param_freq1] * (1 - *params[param_sep1])), *params[param_q1], (float)srate);
        hpL[1][0].set_hp_rbj((float)(*params[param_freq1] * (1 + *params[param_sep1])), *params[param_q1], (float)srate);
        lpR[1][0].copy_coeffs(lpL[1][0]);
        hpR[1][0].copy_coeffs(hpL[1][0]);
        for(i = 1; i <= j1; i++) {
            lpL[1][i].copy_coeffs(lpL[1][0]);
            hpL[1][i].copy_coeffs(hpL[1][0]);
            lpR[1][i].copy_coeffs(lpL[1][0]);
            hpR[1][i].copy_coeffs(hpL[1][0]);
        }
        freq_old[1] = *params[param_freq1];
        sep_old[1]  = *params[param_sep1];
        q_old[1]    = *params[param_q1];
    }
    if(*params[param_freq2] != freq_old[2] or *params[param_sep2] != sep_old[2] or *params[param_q2] != q_old[2] or *params[param_mode] != mode_old) {
        lpL[2][0].set_lp_rbj((float)(*params[param_freq2] * (1 - *params[param_sep2])), *params[param_q2], (float)srate);
        hpL[2][0].set_hp_rbj((float)(*params[param_freq2] * (1 + *params[param_sep2])), *params[param_q2], (float)srate);
        lpR[2][0].copy_coeffs(lpL[2][0]);
        hpR[2][0].copy_coeffs(hpL[2][0]);
        for(i = 1; i <= j1; i++) {
            lpL[2][i].copy_coeffs(lpL[2][0]);
            hpL[2][i].copy_coeffs(hpL[2][0]);
            lpR[2][i].copy_coeffs(lpL[2][0]);
            hpR[2][i].copy_coeffs(hpL[2][0]);
        }
        freq_old[2] = *params[param_freq2];
        sep_old[2]  = *params[param_sep2];
        q_old[2]    = *params[param_q2];
    }
    // set the params of all strips
    strip[0].set_params(*params[param_limit], std::max(1.f / 30, *params[param_release]), 1.f, srate);
    strip[1].set_params(*params[param_limit], std::max(1.f / *params[param_freq0], *params[param_release]), 1.f, srate);
    strip[2].set_params(*params[param_limit], std::max(1.f / *params[param_freq1], *params[param_release]), 1.f, srate);
    strip[3].set_params(*params[param_limit], std::max(1.f / *params[param_freq2], *params[param_release]), 1.f, srate, true);
}

void multibandlimiter_audio_module::set_sample_rate(uint32_t sr)
{
    srate = sr;
    // set srate of all strips
    for (int j = 0; j < strips; j ++) {
        strip[j].set_sample_rate(srate);
    }
}

#define BYPASSED_COMPRESSION(index) \
    if(params[param_att##index] != NULL) \
        *params[param_att##index] = 1.0; \

#define ACTIVE_COMPRESSION(index) \
    if(params[param_att##index] != NULL) \
        *params[param_att##index] = strip[index].get_attenuation(); \

uint32_t multibandlimiter_audio_module::process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask)
{
    bool bypass = *params[param_bypass] > 0.5f;
    numsamples += offset;
//    for (int i = 0; i < strips; i++)
//        strip[i].update_curve();
    if(bypass) {
        // everything bypassed
        while(offset < numsamples) {
            outs[0][offset] = ins[0][offset];
            outs[1][offset] = ins[1][offset];
            ++offset;
        }
        // displays, too
        clip_inL    = 0.f;
        clip_inR    = 0.f;
        clip_outL   = 0.f;
        clip_outR   = 0.f;
        meter_inL  = 0.f;
        meter_inR  = 0.f;
        meter_outL = 0.f;
        meter_outR = 0.f;
    } else {
        // process all strips
        
        // let meters fall a bit
        clip_inL    -= std::min(clip_inL,  numsamples);
        clip_inR    -= std::min(clip_inR,  numsamples);
        clip_outL   -= std::min(clip_outL, numsamples);
        clip_outR   -= std::min(clip_outR, numsamples);
        meter_inL = 0.f;
        meter_inR = 0.f;
        meter_outL = 0.f;
        meter_outR = 0.f;
        while(offset < numsamples) {
            // cycle through samples
            float inL = ins[0][offset];
            float inR = ins[1][offset];
            // in level
            inR *= *params[param_level_in];
            inL *= *params[param_level_in];
            // out vars
            float outL = 0.f;
            float outR = 0.f;
            int j1;
            float left;
            float right;
            for (int i = 0; i < strips; i++) {
                left  = inL;
                right = inR;
                // send trough filters
                
                switch(mode) {
                    case 0:
                        j1 = 0;
                        break;
                    case 1:
                        j1 = 2;
                        break;
                }
                for (int j = 0; j <= j1; j++){
                    if(i + 1 < strips) {
                        left  = lpL[i][j].process(left);
                        right = lpR[i][j].process(right);
                        lpL[i][j].sanitize();
                        lpR[i][j].sanitize();
                    }
                    if(i - 1 >= 0) {
                        left  = hpL[i - 1][j].process(left);
                        right = hpR[i - 1][j].process(right);
                        hpL[i - 1][j].sanitize();
                        hpR[i - 1][j].sanitize();
                    }
                }
                // process gain reduction
                strip[i].process(left, right);
                // sum up output
                outL += left;
                outR += right;
            } // process single strip
            
            // even out filters gain reduction
            // 3dB - levelled manually (based on default sep and q settings)
            switch(mode) {
                case 0:
                    outL *= 1.414213562;
                    outR *= 1.414213562;
                    break;
                case 1:
                    outL *= 0.61;
                    outR *= 0.61;
                    break;
            }
            // out level
            outL *= *params[param_level_out];
            outR *= *params[param_level_out];
            
            // send to output
            outs[0][offset] = outL;
            outs[1][offset] = outR;
            
            // clip LED's
            if(inL > 1.f) {
                clip_inL  = srate >> 3;
            }
            if(inR > 1.f) {
                clip_inR  = srate >> 3;
            }
            if(outL > 1.f) {
                clip_outL = srate >> 3;
            }
            if(outR > 1.f) {
                clip_outR = srate >> 3;
            }
            // set up in / out meters
            if(inL > meter_inL) {
                meter_inL = inL;
            }
            if(inR > meter_inR) {
                meter_inR = inR;
            }
            if(outL > meter_outL) {
                meter_outL = outL;
            }
            if(outR > meter_outR) {
                meter_outR = outR;
            }
            // next sample
            ++offset;
        } // cycle trough samples
        
    } // process all strips (no bypass)
    
    // draw meters
    SET_IF_CONNECTED(clip_inL);
    SET_IF_CONNECTED(clip_inR);
    SET_IF_CONNECTED(clip_outL);
    SET_IF_CONNECTED(clip_outR);
    SET_IF_CONNECTED(meter_inL);
    SET_IF_CONNECTED(meter_inR);
    SET_IF_CONNECTED(meter_outL);
    SET_IF_CONNECTED(meter_outR);
    // draw strip meters
    if(bypass > 0.5f) {
        BYPASSED_COMPRESSION(0)
        BYPASSED_COMPRESSION(1)
        BYPASSED_COMPRESSION(2)
        BYPASSED_COMPRESSION(3)
    } else {
        ACTIVE_COMPRESSION(0)
        ACTIVE_COMPRESSION(1)
        ACTIVE_COMPRESSION(2)
        ACTIVE_COMPRESSION(3)
    }
    // whatever has to be returned x)
    return outputs_mask;
}

bool multibandlimiter_audio_module::get_graph(int index, int subindex, float *data, int points, cairo_iface *context) const
{
    if (!is_active or subindex > 3)
        return false;
    float ret;
    double freq;
    int j1;
    for (int i = 0; i < points; i++)
    {
        ret = 1.f;
        freq = 20.0 * pow (20000.0 / 20.0, i * 1.0 / points);
        switch(mode) {
            case 0:
                j1 = 0;
                break;
            case 1:
                j1 = 2;
                break;
        }
        for(int j = 0; j <= j1; j ++) {
            switch(subindex) {
                case 0:
                    ret *= lpL[0][j].freq_gain(freq, (float)srate);
                    break;
                case 1:
                    ret *= hpL[0][j].freq_gain(freq, (float)srate);
                    ret *= lpL[1][j].freq_gain(freq, (float)srate);
                    break;
                case 2:
                    ret *= hpL[1][j].freq_gain(freq, (float)srate);
                    ret *= lpL[2][j].freq_gain(freq, (float)srate);
                    break;
                case 3:
                    ret *= hpL[2][j].freq_gain(freq, (float)srate);
                    break;
            }
        }
        data[i] = dB_grid(ret);
    }
    if (*params[param_bypass] > 0.5f)
        context->set_source_rgba(0.35, 0.4, 0.2, 0.3);
    else {
        context->set_source_rgba(0.35, 0.4, 0.2, 1);
        context->set_line_width(1.5);
    }
    return true;
}

bool multibandlimiter_audio_module::get_gridline(int index, int subindex, float &pos, bool &vertical, std::string &legend, cairo_iface *context) const
{
    if (!is_active) {
        return false;
    } else {
        vertical = (subindex & 1) != 0;
        return get_freq_gridline(subindex, pos, vertical, legend, context);
    }
}

