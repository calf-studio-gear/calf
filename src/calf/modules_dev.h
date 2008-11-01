/* Calf DSP Library
 * Prototype audio modules
 *
 * Copyright (C) 2008 Thor Harald Johansen <thj@thj.no>
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
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#ifndef __CALF_MODULES_DEV_H
#define __CALF_MODULES_DEV_H

namespace synth {

#if ENABLE_EXPERIMENTAL

class compressor_audio_module: public null_audio_module {
public:
    enum { in_count = 2, out_count = 2, support_midi = false, rt_capable = true };
    enum { param_threshold, param_ratio, param_attack, param_release, param_makeup, param_knee, param_rms, param_compression, param_peak, param_clip, param_count };

    static const char *port_names[in_count + out_count];
    static synth::ladspa_plugin_info plugin_info;
    float *ins[in_count];
    float *outs[out_count];
    float *params[param_count];
    uint32_t srate;
    static parameter_properties param_props[];
    float clip, peak;
    void activate() {
        target = 1.f;
        aim = 1.f;
        peak = 0.f;
    }
    uint32_t process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask) {
        numsamples += offset;
        bool rms = *params[param_rms] > 0.5f;
        float threshold = *params[param_threshold];
        float ratio = *params[param_ratio];
        float attack = *params[param_attack];
        float attack_coeff = std::min(1.f, 1.f / (attack * srate / 4000.f));
        float release = *params[param_release];
        float release_coeff = std::min(1.f, 1.f / (release * srate / 4000.f));
        float makeup = *params[param_makeup];
        float knee = *params[param_knee];

        while(offset < numsamples) {
            float asample = std::max(fabs(ins[0][offset]), fabs(ins[1][offset]));
            if(rms) asample *= asample;
            
            if(asample > 0 && (asample > threshold || knee < 1)) {
                if(IS_FAKE_INFINITY(ratio)) {
                    target = threshold;
                } else {
                    target = asample - threshold;
                    target /= ratio;
                    target += threshold;
                }
                
                if(knee < 1) {
                    float t = std::min(1.f, std::max(0.f, asample / threshold - knee) / (1.f - knee));
                    target = (target - asample) * t + asample;
                }
                
                target /= asample;
            } else {
                target = 1.f;
            }
        
            aim += (target - aim) * (aim > target ? attack_coeff : release_coeff);
            
            peak -= peak * 0.0001;
            
            float raim;
            if(rms) raim = sqrt(aim);
            raim = aim;

            float outL = ins[0][offset] * raim * makeup;
            outs[0][offset] = outL;

            float outR = ins[1][offset] * raim * makeup;
            outs[1][offset] = outR;

            ++offset;
            
            float maxLR = std::max(fabs(outL), fabs(outR));
           
            if(maxLR > 1) clip = srate / 10;
            if(maxLR > peak) peak = maxLR;
            
            if(clip > 0) {
                --clip;
            }

        }
        
        if(params[param_compression] != NULL) {
            *params[param_compression] = aim;
        }

        if(params[param_clip] != NULL) {
            *params[param_clip] = clip;
        }

        if(params[param_peak] != NULL) {
            *params[param_peak] = peak;
        }

        return inputs_mask;
    }

    static const char *get_name() { return "compressor"; }
    static const char *get_id() { return "compressor"; }
    static const char *get_label() { return "Compressor"; }

    void set_sample_rate(uint32_t sr) {
            srate = sr;
    }
private:
    float aim, target;
};

#endif
    
};

#endif
