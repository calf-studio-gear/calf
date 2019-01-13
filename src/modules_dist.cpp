/* Calf DSP plugin pack
 * Distortion related plugins
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
#include <calf/utils.h>
#include <calf/giface.h>
#include <calf/modules_dist.h>
#include <string>
#ifdef _MSC_VER
#include <intrin.h>
inline int clz(unsigned int value)
{
	unsigned long leading_zero = 0;

	if (_BitScanReverse(&leading_zero, value))
	{
		//_BitScanReverse return the position while GCC returns number of leading zeros
		//so reverse it.
		return 31 - leading_zero;
	}
	else
	{
		//GCC implementation says it's undefined in this case
		return 32;
	}
}
#else 
inline int clz(unsigned int value) { return __builtin_clz(value); }
#endif
using namespace dsp;
using namespace calf_plugins;

#define SET_IF_CONNECTED(name) if (params[AM::param_##name] != NULL) *params[AM::param_##name] = name;

/**********************************************************************
 * SATURATOR by Markus Schmidt
**********************************************************************/

saturator_audio_module::saturator_audio_module()
{
    is_active        = false;
    srate            = 0;
    lp_pre_freq_old  = -1;
    hp_pre_freq_old  = -1;
    lp_post_freq_old = -1;
    hp_post_freq_old = -1;
    p_freq_old       = -1;
    p_level_old      = -1;
    p_q_old          = -1;
}

void saturator_audio_module::activate()
{
    is_active = true;
    // set all filters
    params_changed();
}
void saturator_audio_module::deactivate()
{
    is_active = false;
}

void saturator_audio_module::params_changed()
{
    // set the params of all filters
    if(*params[param_lp_pre_freq] != lp_pre_freq_old) {
        lp[0][0].set_lp_rbj(*params[param_lp_pre_freq], 0.707, (float)srate);
        if(in_count > 1 && out_count > 1)
            lp[1][0].copy_coeffs(lp[0][0]);
        lp[0][1].copy_coeffs(lp[0][0]);
        if(in_count > 1 && out_count > 1)
            lp[1][1].copy_coeffs(lp[0][0]);
        lp_pre_freq_old = *params[param_lp_pre_freq];
    }
    if(*params[param_hp_pre_freq] != hp_pre_freq_old) {
        hp[0][0].set_hp_rbj(*params[param_hp_pre_freq], 0.707, (float)srate);
        if(in_count > 1 && out_count > 1)
            hp[1][0].copy_coeffs(hp[0][0]);
        hp[0][1].copy_coeffs(hp[0][0]);
        if(in_count > 1 && out_count > 1)
            hp[1][1].copy_coeffs(hp[0][0]);
        hp_pre_freq_old = *params[param_hp_pre_freq];
    }
    if(*params[param_lp_post_freq] != lp_post_freq_old) {
        lp[0][2].set_lp_rbj(*params[param_lp_post_freq], 0.707, (float)srate);
        if(in_count > 1 && out_count > 1)
            lp[1][2].copy_coeffs(lp[0][2]);
        lp[0][3].copy_coeffs(lp[0][2]);
        if(in_count > 1 && out_count > 1)
            lp[1][3].copy_coeffs(lp[0][2]);
        lp_post_freq_old = *params[param_lp_post_freq];
    }
    if(*params[param_hp_post_freq] != hp_post_freq_old) {
        hp[0][2].set_hp_rbj(*params[param_hp_post_freq], 0.707, (float)srate);
        if(in_count > 1 && out_count > 1)
            hp[1][2].copy_coeffs(hp[0][2]);
        hp[0][3].copy_coeffs(hp[0][2]);
        if(in_count > 1 && out_count > 1)
            hp[1][3].copy_coeffs(hp[0][2]);
        hp_post_freq_old = *params[param_hp_post_freq];
    }
    if(*params[param_p_freq] != p_freq_old || *params[param_p_level] != p_level_old || *params[param_p_q] != p_q_old) {
        p[0].set_peakeq_rbj((float)*params[param_p_freq], (float)*params[param_p_q], (float)*params[param_p_level], (float)srate);
        if(in_count > 1 && out_count > 1)
            p[1].copy_coeffs(p[0]);
        p_freq_old = *params[param_p_freq];
        p_level_old = *params[param_p_level];
        p_q_old = *params[param_p_q];
    }
    // set distortion
    dist[0].set_params(*params[param_blend], *params[param_drive]);
    if(in_count > 1 && out_count > 1)
        dist[1].set_params(*params[param_blend], *params[param_drive]);
}

void saturator_audio_module::set_sample_rate(uint32_t sr)
{
    srate = sr;
    dist[0].set_sample_rate(sr);
    if(in_count > 1 && out_count > 1)
        dist[1].set_sample_rate(sr);
    int meter[] = {param_meter_inL,  param_meter_inR, param_meter_outL, param_meter_outR};
    int clip[] = {param_clip_inL, param_clip_inR, param_clip_outL, param_clip_outR};
    meters.init(params, meter, clip, 4, srate);
}

uint32_t saturator_audio_module::process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask)
{
    bool bypassed = bypass.update(*params[param_bypass] > 0.5f, numsamples);
    numsamples += offset;
    if(bypassed) {
        // everything bypassed
        while(offset < numsamples) {
            if(in_count > 1 && out_count > 1) {
                outs[0][offset] = ins[0][offset];
                outs[1][offset] = ins[1][offset];
            } else if(in_count > 1) {
                outs[0][offset] = (ins[0][offset] + ins[1][offset]) / 2;
            } else if(out_count > 1) {
                outs[0][offset] = ins[0][offset];
                outs[1][offset] = ins[0][offset];
            } else {
                outs[0][offset] = ins[0][offset];
            }
            float values[] = {0, 0, 0, 0};
            meters.process(values);
            ++offset;
        }
    } else {
        uint32_t orig_numsamples = numsamples-offset;
        uint32_t orig_offset = offset;
        // process
        while(offset < numsamples) {
            // cycle through samples
            float out[2], in[2] = {0.f, 0.f};
            int c = 0;
            if(in_count > 1 && out_count > 1) {
                // stereo in/stereo out
                // handle full stereo
                in[0] = ins[0][offset];
                in[1] = ins[1][offset];
                c = 2;
            } else {
                // in and/or out mono
                // handle mono
                in[0] = ins[0][offset];
                in[1] = in[0];
                c = 1;
            }
            
            float proc[2];
            proc[0] = in[0] * *params[param_level_in];
            proc[1] = in[1] * *params[param_level_in];
            
            float onedivlevelin = 1.0 / *params[param_level_in];
            
            for (int i = 0; i < c; ++i) {
                // all pre filters in chain
                if (*params[param_pre] > 0.5) {
                    proc[i] = lp[i][1].process(lp[i][0].process(proc[i]));
                    proc[i] = hp[i][1].process(hp[i][0].process(proc[i]));
                }
                // ...saturate...
                proc[i] = dist[i].process(proc[i]);
                
                // tone control
                proc[i] = p[i].process(proc[i]);
                
                // all post filters in chain
                if (*params[param_post] > 0.5) {
                    proc[i] = lp[i][2].process(lp[i][3].process(proc[i]));
                    proc[i] = hp[i][2].process(hp[i][3].process(proc[i]));
                }
                //subtract gain
                proc[i] *= onedivlevelin;
                
                if (*params[param_level_in] > 1)
                    proc[i] *= 1 + (*params[param_level_in] - 1) / 32;
            }
            
            if(in_count > 1 && out_count > 1) {
                // full stereo
                out[0] = ((proc[0] * *params[param_mix]) + in[0] * (1 - *params[param_mix])) * *params[param_level_out];
                outs[0][offset] = out[0];
                out[1] = ((proc[1] * *params[param_mix]) + in[1] * (1 - *params[param_mix])) * *params[param_level_out];
                outs[1][offset] = out[1];
            } else if(out_count > 1) {
                // mono -> pseudo stereo
                out[0] = ((proc[0] * *params[param_mix]) + in[0] * (1 - *params[param_mix])) * *params[param_level_out];
                outs[0][offset] = out[0];
                out[1] = out[0];
                outs[1][offset] = out[1];
            } else {
                // stereo -> mono
                // or full mono
                out[0] = ((proc[0] * *params[param_mix]) + in[0] * (1 - *params[param_mix])) * *params[param_level_out];
                outs[0][offset] = out[0];
            }
            float values[] = {in[0],  in[1], out[0], out[1]};
            meters.process(values);

            // next sample
            ++offset;
        } // cycle trough samples
        
        // clean up
        lp[0][0].sanitize();
        lp[1][0].sanitize();
        lp[0][1].sanitize();
        lp[1][1].sanitize();
        lp[0][2].sanitize();
        lp[1][2].sanitize();
        lp[0][3].sanitize();
        lp[1][3].sanitize();
        hp[0][0].sanitize();
        hp[1][0].sanitize();
        hp[0][1].sanitize();
        hp[1][1].sanitize();
        hp[0][2].sanitize();
        hp[1][2].sanitize();
        hp[0][3].sanitize();
        hp[1][3].sanitize();
        p[0].sanitize();
        p[1].sanitize();
        bypass.crossfade(ins, outs, 2, orig_offset, orig_numsamples);
    }
    meters.fall(numsamples);
    return outputs_mask;
}

/**********************************************************************
 * EXCITER by Markus Schmidt
**********************************************************************/

exciter_audio_module::exciter_audio_module()
{
    freq_old        = 0.f;
    ceil_old        = 0.f;
    ceil_active_old = false;    
    meter_drive     = 0.f;
    is_active       = false;
    srate           = 0;
}

void exciter_audio_module::activate()
{
    is_active = true;
    // set all filters
    params_changed();
}

void exciter_audio_module::deactivate()
{
    is_active = false;
}

void exciter_audio_module::params_changed()
{
    // set the params of all filters
    if(*params[param_freq] != freq_old) {
        hp[0][0].set_hp_rbj(*params[param_freq], 0.707, (float)srate);
        hp[0][1].copy_coeffs(hp[0][0]);
        hp[0][2].copy_coeffs(hp[0][0]);
        hp[0][3].copy_coeffs(hp[0][0]);
        if(in_count > 1 && out_count > 1) {
            hp[1][0].copy_coeffs(hp[0][0]);
            hp[1][1].copy_coeffs(hp[0][0]);
            hp[1][2].copy_coeffs(hp[0][0]);
            hp[1][3].copy_coeffs(hp[0][0]);
        }
        freq_old = *params[param_freq];
    }
    // set the params of all filters
    if(*params[param_ceil] != ceil_old || *params[param_ceil_active] != ceil_active_old) {
        lp[0][0].set_lp_rbj(*params[param_ceil], 0.707, (float)srate);
        lp[0][1].copy_coeffs(lp[0][0]);
        lp[1][0].copy_coeffs(lp[0][0]);
        lp[1][1].copy_coeffs(lp[0][0]);
        ceil_old = *params[param_ceil];
        ceil_active_old = *params[param_ceil_active];
    }
    // set distortion
    dist[0].set_params(*params[param_blend], *params[param_drive]);
    if(in_count > 1 && out_count > 1)
        dist[1].set_params(*params[param_blend], *params[param_drive]);
}

void exciter_audio_module::set_sample_rate(uint32_t sr)
{
    srate = sr;
    dist[0].set_sample_rate(sr);
    if(in_count > 1 && out_count > 1)
        dist[1].set_sample_rate(sr);
    int meter[] = {param_meter_in,  param_meter_out, param_meter_drive};
    int clip[] = {param_clip_in, param_clip_out, -1};
    meters.init(params, meter, clip, 3, srate);
}

uint32_t exciter_audio_module::process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask)
{
    bool bypassed = bypass.update(*params[param_bypass] > 0.5f, numsamples);
    numsamples += offset;
    if(bypassed) {
        // everything bypassed
        while(offset < numsamples) {
            if(in_count > 1 && out_count > 1) {
                outs[0][offset] = ins[0][offset];
                outs[1][offset] = ins[1][offset];
            } else if(in_count > 1) {
                outs[0][offset] = (ins[0][offset] + ins[1][offset]) / 2;
            } else if(out_count > 1) {
                outs[0][offset] = ins[0][offset];
                outs[1][offset] = ins[0][offset];
            } else {
                outs[0][offset] = ins[0][offset];
            }
            float values[] = {0, 0, 0, 0, 0};
            meters.process(values);
            ++offset;
        }
        // displays, too
        meter_drive = 0.f;
    } else {
        uint32_t orig_numsamples = numsamples-offset;
        uint32_t orig_offset = offset;
        meter_drive = 0.f;
        
        float in2out = *params[param_listen] > 0.f ? 0.f : 1.f;
        
        // process
        while(offset < numsamples) {
            // cycle through samples
            float out[2], in[2] = {0.f, 0.f};
            float maxDrive = 0.f;
            int c = 0;
            
            if(in_count > 1 && out_count > 1) {
                // stereo in/stereo out
                // handle full stereo
                in[0] = ins[0][offset];
                in[0] *= *params[param_level_in];
                in[1] = ins[1][offset];
                in[1] *= *params[param_level_in];
                c = 2;
            } else {
                // in and/or out mono
                // handle mono
                in[0] = ins[0][offset];
                in[0] *= *params[param_level_in];
                in[1] = in[0];
                c = 1;
            }
            
            float proc[2];
            proc[0] = in[0];
            proc[1] = in[1];
            
            for (int i = 0; i < c; ++i) {
                // all pre filters in chain
                proc[i] = hp[i][1].process(hp[i][0].process(proc[i]));
                
                // saturate
                proc[i] = dist[i].process(proc[i]);

                // all post filters in chain
                proc[i] = hp[i][2].process(hp[i][3].process(proc[i]));
                
                if(*params[param_ceil_active] > 0.5f) {
                    // all H/P post filters in chain
                    proc[i] = lp[i][0].process(lp[i][1].process(proc[i]));
                    
                }
            }
            maxDrive = dist[0].get_distortion_level() * *params[param_amount];
            
            if(in_count > 1 && out_count > 1) {
                maxDrive = std::max(maxDrive, dist[1].get_distortion_level() * *params[param_amount]);
                // full stereo
                out[0] = (proc[0] * *params[param_amount] + in2out * in[0]) * *params[param_level_out];
                out[1] = (proc[1] * *params[param_amount] + in2out * in[1]) * *params[param_level_out];
                outs[0][offset] = out[0];
                outs[1][offset] = out[1];
            } else if(out_count > 1) {
                // mono -> pseudo stereo
                out[1] = out[0] = (proc[0] * *params[param_amount] + in2out * in[0]) * *params[param_level_out];
                outs[0][offset] = out[0];
                outs[1][offset] = out[1];
            } else {
                // stereo -> mono
                // or full mono
                out[0] = (proc[0] * *params[param_amount] + in2out * in[0]) * *params[param_level_out];
                outs[0][offset] = out[0];
            }
            float values[] = {(in[0] + in[1]) / 2, (out[0] + out[1]) / 2, maxDrive};
            meters.process(values);
            // next sample
            ++offset;
        } // cycle trough samples
        bypass.crossfade(ins, outs, 2, orig_offset, orig_numsamples);
        // clean up
        hp[0][0].sanitize();
        hp[1][0].sanitize();
        hp[0][1].sanitize();
        hp[1][1].sanitize();
        hp[0][2].sanitize();
        hp[1][2].sanitize();
        hp[0][3].sanitize();
        hp[1][3].sanitize();
        
        lp[0][0].sanitize();
        lp[1][0].sanitize();
        lp[0][1].sanitize();
        lp[1][1].sanitize();
    }
    meters.fall(numsamples);
    return outputs_mask;
}

/**********************************************************************
 * BASS ENHANCER by Markus Schmidt
**********************************************************************/

bassenhancer_audio_module::bassenhancer_audio_module()
{
    freq_old         = 0.f;
    floor_old        = 0.f;
    floor_active_old = false;    
    meter_drive      = 0.f;
    is_active        = false;
    srate            = 0;
}

void bassenhancer_audio_module::activate()
{
    is_active = true;
    // set all filters
    params_changed();
}
void bassenhancer_audio_module::deactivate()
{
    is_active = false;
}

void bassenhancer_audio_module::params_changed()
{
    // set the params of all filters
    if(*params[param_freq] != freq_old) {
        lp[0][0].set_lp_rbj(*params[param_freq], 0.707, (float)srate);
        lp[0][1].copy_coeffs(lp[0][0]);
        lp[0][2].copy_coeffs(lp[0][0]);
        lp[0][3].copy_coeffs(lp[0][0]);
        if(in_count > 1 && out_count > 1) {
            lp[1][0].copy_coeffs(lp[0][0]);
            lp[1][1].copy_coeffs(lp[0][0]);
            lp[1][2].copy_coeffs(lp[0][0]);
            lp[1][3].copy_coeffs(lp[0][0]);
        }
        freq_old = *params[param_freq];
    }
    // set the params of all filters
    if(*params[param_floor] != floor_old || *params[param_floor_active] != floor_active_old) {
        hp[0][0].set_hp_rbj(*params[param_floor], 0.707, (float)srate);
        hp[0][1].copy_coeffs(hp[0][0]);
        hp[1][0].copy_coeffs(hp[0][0]);
        hp[1][1].copy_coeffs(hp[0][0]);
        floor_old = *params[param_floor];
        floor_active_old = *params[param_floor_active];
    }
    // set distortion
    dist[0].set_params(*params[param_blend], *params[param_drive]);
    if(in_count > 1 && out_count > 1)
        dist[1].set_params(*params[param_blend], *params[param_drive]);
}

void bassenhancer_audio_module::set_sample_rate(uint32_t sr)
{
    srate = sr;
    dist[0].set_sample_rate(sr);
    if(in_count > 1 && out_count > 1)
        dist[1].set_sample_rate(sr);
    int meter[] = {param_meter_in,  param_meter_out, param_meter_drive};
    int clip[] = {param_clip_in, param_clip_out, -1};
    meters.init(params, meter, clip, 3, srate);
}

uint32_t bassenhancer_audio_module::process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask)
{
    bool bypassed = bypass.update(*params[param_bypass] > 0.5f, numsamples);
    numsamples += offset;
    if(bypassed) {
        // everything bypassed
        while(offset < numsamples) {
            if(in_count > 1 && out_count > 1) {
                outs[0][offset] = ins[0][offset];
                outs[1][offset] = ins[1][offset];
            } else if(in_count > 1) {
                outs[0][offset] = (ins[0][offset] + ins[1][offset]) / 2;
            } else if(out_count > 1) {
                outs[0][offset] = ins[0][offset];
                outs[1][offset] = ins[0][offset];
            } else {
                outs[0][offset] = ins[0][offset];
            }
            float values[] = {0, 0, 0};
            meters.process(values);
            ++offset;
        }
    } else {
        // process
        uint32_t orig_numsamples = numsamples-offset;
        uint32_t orig_offset = offset;
        while(offset < numsamples) {
            // cycle through samples
            float out[2], in[2] = {0.f, 0.f};
            float maxDrive = 0.f;
            int c = 0;
            
            if(in_count > 1 && out_count > 1) {
                // stereo in/stereo out
                // handle full stereo
                in[0] = ins[0][offset];
                in[0] *= *params[param_level_in];
                in[1] = ins[1][offset];
                in[1] *= *params[param_level_in];
                c = 2;
            } else {
                // in and/or out mono
                // handle mono
                in[0] = ins[0][offset];
                in[0] *= *params[param_level_in];
                in[1] = in[0];
                c = 1;
            }
            
            float proc[2];
            proc[0] = in[0];
            proc[1] = in[1];
            
            for (int i = 0; i < c; ++i) {
                // all pre filters in chain
                proc[i] = lp[i][1].process(lp[i][0].process(proc[i]));
                
                // saturate
                proc[i] = dist[i].process(proc[i]);

                // all post filters in chain
                proc[i] = lp[i][2].process(lp[i][3].process(proc[i]));
                
                if(*params[param_floor_active] > 0.5f) {
                    // all H/P post filters in chain
                    proc[i] = hp[i][0].process(hp[i][1].process(proc[i]));
                    
                }
            }
            
            if(in_count > 1 && out_count > 1) {
                // full stereo
                if(*params[param_listen] > 0.f)
                    out[0] = proc[0] * *params[param_amount] * *params[param_level_out];
                else
                    out[0] = (proc[0] * *params[param_amount] + in[0]) * *params[param_level_out];
                outs[0][offset] = out[0];
                if(*params[param_listen] > 0.f)
                    out[1] = proc[1] * *params[param_amount] * *params[param_level_out];
                else
                    out[1] = (proc[1] * *params[param_amount] + in[1]) * *params[param_level_out];
                outs[1][offset] = out[1];
                maxDrive = std::max(dist[0].get_distortion_level() * *params[param_amount],
                                            dist[1].get_distortion_level() * *params[param_amount]);
            } else if(out_count > 1) {
                // mono -> pseudo stereo
                if(*params[param_listen] > 0.f)
                    out[0] = proc[0] * *params[param_amount] * *params[param_level_out];
                else
                    out[0] = (proc[0] * *params[param_amount] + in[0]) * *params[param_level_out];
                outs[0][offset] = out[0];
                out[1] = out[0];
                outs[1][offset] = out[1];
                maxDrive = dist[0].get_distortion_level() * *params[param_amount];
            } else {
                // stereo -> mono
                // or full mono
                if(*params[param_listen] > 0.f)
                    out[0] = proc[0] * *params[param_amount] * *params[param_level_out];
                else
                    out[0] = (proc[0] * *params[param_amount] + in[0]) * *params[param_level_out];
                outs[0][offset] = out[0];
                maxDrive = dist[0].get_distortion_level() * *params[param_amount];
            }
            
            float values[] = {(in[0] + in[1]) / 2, (out[0] + out[1]) / 2, maxDrive};
            meters.process(values);
            // next sample
            ++offset;
        } // cycle trough samples
        bypass.crossfade(ins, outs, 2, orig_offset, orig_numsamples);
        // clean up
        lp[0][0].sanitize();
        lp[1][0].sanitize();
        lp[0][1].sanitize();
        lp[1][1].sanitize();
        lp[0][2].sanitize();
        lp[1][2].sanitize();
        lp[0][3].sanitize();
        lp[1][3].sanitize();
        hp[0][0].sanitize();
        hp[1][0].sanitize();
        hp[0][1].sanitize();
        hp[1][1].sanitize();
    }
    meters.fall(numsamples);
    return outputs_mask;
}


/**********************************************************************
 * VINYL by Markus Schmidt
**********************************************************************/

vinyl_audio_module::vinyl_audio_module() {
    active          = false;
    clip_inL        = 0.f;
    clip_inR        = 0.f;
    clip_outL       = 0.f;
    clip_outR       = 0.f;
    meter_inL       = 0.f;
    meter_inR       = 0.f;
    meter_outL      = 0.f;
    meter_outR      = 0.f;
    dbufsize        = 0;
    
    speed_old       = 0.f;
    freq_old        = 0.f;
    aging_old       = 0.f;
}

void vinyl_audio_module::activate() {
    active = true;
}

void vinyl_audio_module::deactivate() {
    active = false;
}

void vinyl_audio_module::params_changed() {
    if (speed_old != *params[param_speed]) {
        lfo.set_params(1.f / (60.f / *params[param_speed]), 0, 0.f, srate, 0.5f);
        speed_old = *params[param_speed];
    }
    if (freq_old != *params[param_freq] || aging_old != *params[param_aging]) {
        aging_old = *params[param_aging];
        freq_old = *params[param_freq];
        float lp = (freq_old + 500.f) * pow(double(20000.f / (freq_old + 500.f)), 1.f - aging_old);
        float hp = 10.f * pow(double((freq_old - 250) / 10.f), aging_old);
        filters[0][0].set_hp_rbj(hp, 0.707 + aging_old * 0.5, (float)srate);
        filters[0][1].copy_coeffs(filters[0][0]);
        filters[0][2].set_peakeq_rbj(freq_old, 1, 1.f + aging_old * 4.f, (float)srate);
        filters[0][3].set_lp_rbj(lp, 0.707 + aging_old * 0.5, (float)srate);
        filters[0][4].copy_coeffs(filters[0][0]);
        for (int i = 1; i < channels; i++) {
            for (int j = 0; j < _filters; j++)
                filters[i][j].copy_coeffs(filters[0][j]);
        }
    }
    for (int j = 0; j < _synths; j++) {
        fluid_synth_pitch_bend(synth, j, (int)(*params[param_pitch0 + j * _synthsp] * 8191 + 8192));
    }
}

uint32_t vinyl_audio_module::process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask) {
    bool bypassed = bypass.update(*params[param_bypass] > 0.5f, numsamples);
    uint32_t orig_offset = offset;
	STACKALLOC(float, sL, numsamples);
	STACKALLOC(float, sR, numsamples);
    if (!bypassed) {
        for (int j = 0; j < _synths; ++j) {
            float gain = 0;
            if (*params[param_active0 + j * _synthsp] >= 0.5f) {
                gain = *params[param_gain0 + j * _synthsp];
            }
            if (gain == last_gain[j])
                continue;
            if (last_gain[j] == 0 && gain != 0) {
                fluid_synth_noteon(synth, j, 60, 127);
            }
            if (last_gain[j] != 0 && gain == 0) {
                fluid_synth_noteoff(synth, j, 60);
            }
            if (gain > 0) {
                fluid_synth_set_gen(synth, j, GEN_ATTENUATION, 200 * log10(1.0 / gain));
            }
            last_gain[j] = gain;
        }
        fluid_synth_write_float(synth, numsamples, sL, 0, 1, sR, 0, 1);
    }
    for(uint32_t i = offset; i < offset + numsamples; i++) {
        float L = ins[0][i];
        float R = ins[1][i];
        if(bypassed) {
            outs[0][i]  = ins[0][i];
            outs[1][i]  = ins[1][i];
            float values[] = {0, 0, 0, 0};
            meters.process(values);
        } else {
            // gain
            L *= *params[param_level_in];
            R *= *params[param_level_in];
            
            // meters
            float inL = L;
            float inR = R;
            
            // droning
            dbuf[dbufpos * channels + 0] = L;
            dbuf[dbufpos * channels + 1] = R;
            
            if (*params[param_drone] > 0.f) {
                int32_t bpos = (lfo.get_value() + 0.5) * *params[param_drone] * dbufrange;
                bpos = (dbufpos - bpos) & (dbufsize - 1);
            
                L = dbuf[bpos * channels];
                R = dbuf[bpos * channels + 1];

                lfo.advance(1);
            }
            dbufpos = (dbufpos + 1) & (dbufsize - 1);
            
            // synths
            L += sL[i - offset];
            R += sR[i - offset];
            
            // filter
            if (*params[param_aging] > 0.f) {
                for (int j = 0; j < _filters; j++) {
                    L = filters[0][j].process(L);
                    R = filters[1][j].process(R);
                }
            }
            
            // levels out
            float areduce = (1.f - aging_old * 0.9);
            L *= *params[param_level_out] * areduce;
            R *= *params[param_level_out] * areduce;
            
            // output
            outs[0][i] = L;
            outs[1][i] = R;
            
            // sanitize filters
            if (*params[param_aging] > 0.f) {
                for (int j = 0; j < _filters; j++) {
                    filters[0][j].sanitize();
                    filters[1][j].sanitize();
                }
            }
            
            float values[] = {inL, inR, outs[0][i], outs[1][i]};
            meters.process(values);
        }
    }
    if (bypassed)
        bypass.crossfade(ins, outs, 2, orig_offset, numsamples);
    meters.fall(numsamples);
    return outputs_mask;
}

void vinyl_audio_module::set_sample_rate(uint32_t sr)
{
    srate = sr;
    int meter[] = {param_meter_inL,  param_meter_inR, param_meter_outL, param_meter_outR};
    int clip[]  = {param_clip_inL, param_clip_inR, param_clip_outL, param_clip_outR};
    meters.init(params, meter, clip, 4, srate);
}

void vinyl_audio_module::post_instantiate(uint32_t sr)
{    
    dbufsize = (sr + 49) / 50;
    // Round up to the nearest power of 2 if it's not already
    // This makes the circular buffer implementation faster and simpler
    if((dbufsize & (dbufsize - 1)))
        dbufsize = 1 << (32 - clz(dbufsize - 1));
    dbufrange = sr / 100.0;
    dbuf = (float*) calloc(dbufsize * channels, sizeof(float));
    dbufpos = 0;
    settings = new_fluid_settings();
    fluid_settings_setnum(settings, "synth.sample-rate", sr);
    fluid_settings_setint(settings, "synth.polyphony", 32);
    fluid_settings_setint(settings, "synth.midi-channels", _synths);
    fluid_settings_setint(settings, "synth.reverb.active", 0);
    fluid_settings_setint(settings, "synth.chorus.active", 0);
    
    char const * paths[] = {
        PKGLIBDIR "sf2/Hum.sf2",
        PKGLIBDIR "sf2/Motor.sf2",
        PKGLIBDIR "sf2/Static.sf2",
        PKGLIBDIR "sf2/Noise.sf2",
        PKGLIBDIR "sf2/Rumble.sf2",
        PKGLIBDIR "sf2/Crackle.sf2",
        PKGLIBDIR "sf2/Crinkle.sf2"
    };
    synth = new_fluid_synth(settings);
    fluid_synth_set_gain(synth, 1.f);
    for (int i = 0; i < _synths; i++) {
        int id = fluid_synth_sfload(synth, paths[i], 0);
        fluid_synth_program_select (synth, i, id, 0, 0);
        fluid_synth_pitch_wheel_sens(synth, i, 12);
        last_gain[i] = 0;
    }
}
vinyl_audio_module::~vinyl_audio_module()
{
    free(dbuf);
    delete_fluid_synth(synth);
    delete_fluid_settings(settings);
}


bool vinyl_audio_module::get_graph(int index, int subindex, int phase, float *data, int points, cairo_iface *context, int *mode) const
{
    if (subindex > 0)
        return false;
    return ::get_graph(*this, subindex, data, points);
}
float vinyl_audio_module::freq_gain(int index, double freq) const
{
    float s = 1.f;
    if (*params[param_aging] > 0)
        for (int i = 0; i < _filters; i++)
            s *= filters[0][i].freq_gain(freq, srate);
    return s;
}
bool vinyl_audio_module::get_layers(int index, int generation, unsigned int &layers) const
{
    layers = 0;
    if (!generation)
        layers |= LG_CACHE_GRID;
    if (index == param_freq)
        layers |= LG_REALTIME_GRAPH;
    return true;
}


/**********************************************************************
 * TAPESIMULATOR by Markus Schmidt
**********************************************************************/

tapesimulator_audio_module::tapesimulator_audio_module() {
    active          = false;
    clip_inL        = 0.f;
    clip_inR        = 0.f;
    clip_outL       = 0.f;
    clip_outR       = 0.f;
    meter_inL       = 0.f;
    meter_inR       = 0.f;
    meter_outL      = 0.f;
    meter_outR      = 0.f;
    lp_old          = -1.f;
    output_old      = -1.f;
    rms             = 0.f;
    mech_old        = false;
    transients.set_channels(channels);
}

void tapesimulator_audio_module::activate() {
    active = true;
}

void tapesimulator_audio_module::deactivate() {
    active = false;
}

void tapesimulator_audio_module::params_changed() {
    if(*params[param_lp] != lp_old || *params[param_mechanical] != mech_old) {
        lp[0][0].set_lp_rbj(*params[param_lp], 0.707, (float)srate);
        lp[0][1].copy_coeffs(lp[0][0]);
        lp[1][0].copy_coeffs(lp[0][0]);
        lp[1][1].copy_coeffs(lp[0][0]);
        lp_old = *params[param_lp];
        mech_old = *params[param_mechanical] > 0.5;
    }
    transients.set_params(50.f / (*params[param_speed] + 1),
                          -0.05f / (*params[param_speed] + 1),
                          100.f,
                          0.f,
                          1.f,
                          0);
    lfo1.set_params((*params[param_speed] + 1) / 2, 0, 0.f, srate, 1.f);
    lfo2.set_params((*params[param_speed] + 1) / 9.38, 0, 0.f, srate, 1.f);
    if (*params[param_level_out] != output_old) {
        output_old = *params[param_level_out];
        redraw_output = true;
    }
}

uint32_t tapesimulator_audio_module::process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask) {
    bool bypassed = bypass.update(*params[param_bypass] > 0.5f, numsamples);
    uint32_t orig_offset = offset;
    for(uint32_t i = offset; i < offset + numsamples; i++) {
        float L = ins[0][i];
        float R = ins[1][i];
        float Lin = ins[0][i];
        float Rin = ins[1][i];
        if(bypassed) {
            outs[0][i]  = ins[0][i];
            outs[1][i]  = ins[1][i];
            float values[] = {0, 0, 0, 0};
            meters.process(values);
        } else {
            // transients
            float inL = 0;
            float inR = 0;
            if(*params[param_magnetical] > 0.5f) {
                float values[] = {L, R};
                transients.process(values, (L + R) / 2.f);
                L = values[0];
                R = values[1];
            }
            
            // noise
            if (*params[param_noise]) {
                float Lnoise = rand() % 2 - 1;
                float Rnoise = rand() % 2 - 1;
                Lnoise = noisefilters[0][2].process(noisefilters[0][1].process(noisefilters[0][0].process(Lnoise)));
                Rnoise = noisefilters[1][2].process(noisefilters[1][1].process(noisefilters[1][0].process(Rnoise)));
                L += Lnoise * *params[param_noise] / 12.f;
                R += Rnoise * *params[param_noise] / 12.f;
            }
            
            // lfo filters / phasing
            if (*params[param_mechanical]) {
                // filtering
                float freqL1 = *params[param_lp] * (1 - ((lfo1.get_value() + 1) * 0.3 * *params[param_mechanical]));
                float freqL2 = *params[param_lp] * (1 - ((lfo2.get_value() + 1) * 0.2 * *params[param_mechanical]));
                
                float freqR1 = *params[param_lp] * (1 - ((lfo1.get_value() * -1 + 1) * 0.3 * *params[param_mechanical]));
                float freqR2 = *params[param_lp] * (1 - ((lfo2.get_value() * -1 + 1) * 0.2 * *params[param_mechanical]));
                
                lp[0][0].set_lp_rbj(freqL1, 0.707, (float)srate);
                lp[0][1].set_lp_rbj(freqL2, 0.707, (float)srate);
                    
                lp[1][0].set_lp_rbj(freqR1, 0.707, (float)srate);
                lp[1][1].set_lp_rbj(freqR2, 0.707, (float)srate);
                
                // phasing
                float _phase = lfo1.get_value() * *params[param_mechanical] * -36;
                
                float _phase_cos_coef = cos(_phase / 180 * M_PI);
                float _phase_sin_coef = sin(_phase / 180 * M_PI);
                
                float _l = L * _phase_cos_coef - R * _phase_sin_coef;
                float _r = L * _phase_sin_coef + R * _phase_cos_coef;
                L = _l;
                R = _r;
            }
            
            // gain
            L *= *params[param_level_in];
            R *= *params[param_level_in];
            
            inL = L;
            inR = R;
            
            // save for drawing input/output curve
            float Lc = L;
            float Rc = R;
            float Lo = L;
            float Ro = R;
            
            // filter
            if (*params[param_post] < 0.5) {
                L = lp[0][1].process(lp[0][0].process(L));
                R = lp[1][1].process(lp[1][0].process(R));
            }
            
            // distortion
            if (L) L = L / fabs(L) * (1 - exp((-1) * 3 * fabs(L)));
            if (R) R = R / fabs(R) * (1 - exp((-1) * 3 * fabs(R)));
            
            if (Lo) Lo = Lo / fabs(Lo) * (1 - exp((-1) * 3 * fabs(Lo)));
            if (Ro) Ro = Ro / fabs(Ro) * (1 - exp((-1) * 3 * fabs(Ro)));
            
            // filter
            if (*params[param_post] >= 0.5) {
                L = lp[0][1].process(lp[0][0].process(L));
                R = lp[1][1].process(lp[1][0].process(R));
            }
            
            // mix
            L = L * *params[param_mix] + Lin * (*params[param_mix] * -1 + 1);
            R = R * *params[param_mix] + Rin * (*params[param_mix] * -1 + 1);
            
            // levels out
            L *= *params[param_level_out];
            R *= *params[param_level_out];
            
            // output
            outs[0][i] = L;
            outs[1][i] = R;
            
            // sanitize filters
            lp[0][0].sanitize();
            lp[1][0].sanitize();
            lp[0][1].sanitize();
            lp[1][1].sanitize();
            
            // LFO's should go on
            lfo1.advance(1);
            lfo2.advance(1);
            
            // dot
            rms = std::max((double)rms, (double)((fabs(Lo) + fabs(Ro)) / 2));
            input = std::max((double)input, (double)((fabs(Lc) + fabs(Rc)) / 2));
            
            float values[] = {inL, inR, outs[0][i], outs[1][i]};
            meters.process(values);
        }
    }
    if (bypassed)
        bypass.crossfade(ins, outs, 2, orig_offset, numsamples);
    meters.fall(numsamples);
    return outputs_mask;
}

void tapesimulator_audio_module::set_sample_rate(uint32_t sr)
{
    srate = sr;
    int meter[] = {param_meter_inL,  param_meter_inR, param_meter_outL, param_meter_outR};
    int clip[]  = {param_clip_inL, param_clip_inR, param_clip_outL, param_clip_outR};
    meters.init(params, meter, clip, 4, srate);
    transients.set_sample_rate(srate);
    noisefilters[0][0].set_hp_rbj(120.f, 0.707, (float)srate);
    noisefilters[1][0].copy_coeffs(noisefilters[0][0]);
    noisefilters[0][1].set_lp_rbj(5500.f, 0.707, (float)srate);
    noisefilters[1][1].copy_coeffs(noisefilters[0][1]);
    noisefilters[0][2].set_highshelf_rbj(1000.f, 0.707, 0.5, (float)srate);
    noisefilters[1][2].copy_coeffs(noisefilters[0][2]);
}

bool tapesimulator_audio_module::get_graph(int index, int subindex, int phase, float *data, int points, cairo_iface *context, int *mode) const
{
    if (subindex > 1)
        return false;
    if(index == param_lp && phase) {
        set_channel_color(context, subindex);
        return ::get_graph(*this, subindex, data, points);
    } else if (index == param_level_in && !phase) {
        if (!subindex) {
            context->set_source_rgba(0.15, 0.2, 0.0, 0.3);
            context->set_line_width(1);
        }
        for (int i = 0; i < points; i++) {
            if (!subindex) {
                float input = dB_grid_inv(-1.0 + (float)i * 2.0 / ((float)points - 1.f));
                data[i] = dB_grid(input);
            } else {
                float output = 1 - exp(-3 * (pow(2, -10 + 14 * (float)i / (float) points)));
                data[i] = dB_grid(output * *params[param_level_out]);
            }
        }
        return true;
    }
    return false;
}
float tapesimulator_audio_module::freq_gain(int index, double freq) const
{
    return lp[index][0].freq_gain(freq, srate) * lp[index][1].freq_gain(freq, srate);
}

bool tapesimulator_audio_module::get_gridline(int index, int subindex, int phase, float &pos, bool &vertical, std::string &legend, cairo_iface *context) const
{
    if(!active || phase)
        return false;
    if (index == param_level_in) {
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
    } else if (index == param_lp) {
        return get_freq_gridline(subindex, pos, vertical, legend, context);
    }
    return false;
}
bool tapesimulator_audio_module::get_dot(int index, int subindex, int phase, float &x, float &y, int &size, cairo_iface *context) const
{
    if (index == param_level_in && !subindex && phase) {
        x = log(input) / log(2) / 14.f + 5.f / 7.f;
        y = dB_grid(rms * *params[param_level_out]);
        rms = 0.f;
        input = 0.f;
        return true;
    }
    return false;
}
bool tapesimulator_audio_module::get_layers(int index, int generation, unsigned int &layers) const
{
    layers = 0;
    // always draw grid on cache if surfaces are new on both widgets
    if (!generation)
        layers |= LG_CACHE_GRID;
    // compression: dot in realtime, graphs as cache on new surfaces
    if (index == param_level_in && (!generation || redraw_output)) {
        layers |= LG_CACHE_GRAPH;
        redraw_output = false;
    }
    if (index == param_level_in)
        layers |= LG_REALTIME_DOT;
    // frequency: both graphs in realtime
    if (index == param_lp)
        layers |= LG_REALTIME_GRAPH;
    // draw always
    return true;
}



/**********************************************************************
 * CRUSHER by Markus Schmidt and Christian Holschuh
**********************************************************************/


crusher_audio_module::crusher_audio_module()
{
    smin = sdiff = 0.f;
}

void crusher_audio_module::activate()
{
    lfo.activate();
}
void crusher_audio_module::deactivate()
{
    lfo.deactivate();
}

void crusher_audio_module::params_changed()
{
    bitreduction.set_params(*params[param_bits],
                            *params[param_morph],
                            *params[param_bypass] > 0.5,
                            *params[param_mode],
                            *params[param_dc],
                            *params[param_aa]);
    samplereduction[0].set_params(*params[param_samples]);
    samplereduction[1].set_params(*params[param_samples]);
    lfo.set_params(*params[param_lforate], 0, 0.f, srate, 0.5f);
    // calc lfo offsets
    float rad  = *params[param_lforange] / 2.f;
    smin = std::max(*params[param_samples] - rad, 1.f);
    float sun  = *params[param_samples] - rad - smin;
    float smax = std::min(*params[param_samples] + rad, 250.f);
    float sov  = *params[param_samples] + rad - smax;
    smax -= sun;
    smin -= sov;
    sdiff = smax - smin;
}

void crusher_audio_module::set_sample_rate(uint32_t sr)
{
    srate = sr;
    int meter[] = {param_meter_inL,  param_meter_inR, param_meter_outL, param_meter_outR};
    int clip[]  = {param_clip_inL, param_clip_inR, param_clip_outL, param_clip_outR};
    meters.init(params, meter, clip, 4, srate);
    bitreduction.set_sample_rate(srate);
}

uint32_t crusher_audio_module::process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask)
{
    bool bypassed = bypass.update(*params[param_bypass] > 0.5f, numsamples);
    numsamples += offset;
    if(bypassed) {
        // everything bypassed
        while(offset < numsamples) {
            outs[0][offset] = ins[0][offset];
            outs[1][offset] = ins[1][offset];
            float values[] = {0, 0, 0, 0};
            meters.process(values);
            ++offset;
        }
    } else {
        // process
        uint32_t orig_numsamples = numsamples-offset;
        uint32_t orig_offset = offset;
        while(offset < numsamples) {
            // cycle through samples
            if (*params[param_lfo] > 0.5) {
                samplereduction[0].set_params(smin + sdiff * (lfo.get_value() + 0.5));
                samplereduction[1].set_params(smin + sdiff * (lfo.get_value() + 0.5));
            }
            outs[0][offset] = samplereduction[0].process(ins[0][offset] * *params[param_level_in]);
            outs[1][offset] = samplereduction[1].process(ins[1][offset] * *params[param_level_in]);
            outs[0][offset] = outs[0][offset] * *params[param_morph] + ins[0][offset] * (*params[param_morph] * -1 + 1) * *params[param_level_in];
            outs[1][offset] = outs[1][offset] * *params[param_morph] + ins[1][offset] * (*params[param_morph] * -1 + 1) * *params[param_level_in];
            outs[0][offset] = bitreduction.process(outs[0][offset]) * *params[param_level_out];
            outs[1][offset] = bitreduction.process(outs[1][offset]) * *params[param_level_out];
            float values[] = {ins[0][offset], ins[1][offset], outs[0][offset], outs[1][offset]};
            meters.process(values);
            // next sample
            ++offset;
            if (*params[param_lforate])
                lfo.advance(1);
        } // cycle trough samples
        bypass.crossfade(ins, outs, 2, orig_offset, orig_numsamples);
    }
    meters.fall(numsamples);
    return outputs_mask;
}
bool crusher_audio_module::get_graph(int index, int subindex, int phase, float *data, int points, cairo_iface *context, int *mode) const
{
    return bitreduction.get_graph(subindex, phase, data, points, context, mode);
}
bool crusher_audio_module::get_layers(int index, int generation, unsigned int &layers) const
{
    return bitreduction.get_layers(index, generation, layers);
}
bool crusher_audio_module::get_gridline(int index, int subindex, int phase, float &pos, bool &vertical, std::string &legend, cairo_iface *context) const
{
    return bitreduction.get_gridline(subindex, phase, pos, vertical, legend, context);
}
