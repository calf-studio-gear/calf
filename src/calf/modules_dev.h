/* Calf DSP Library
 * Prototype audio modules
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
    enum { param_threshold, param_ratio, param_attack, param_release, param_makeup, param_compression, param_count };

    static const char *port_names[in_count + out_count];
    static synth::ladspa_plugin_info plugin_info;
    float *ins[in_count];
    float *outs[out_count];
    float *params[param_count];
    uint32_t srate;
    static parameter_properties param_props[];
    void activate() {
        target = 1;
        aim = 1;
    }
    uint32_t process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask) {
        numsamples += offset;
        float threshold = *params[param_threshold];
        float ratio = *params[param_ratio];
        float attack_coeff = 1 / (*params[param_attack] * srate / 1000);
        float release_coeff = 1 / (*params[param_release] * srate / 1000);
        float makeup = *params[param_makeup];

        *params[param_compression] = aim;
        
        while(offset < numsamples) {
            float asample = std::max(fabs(ins[0][offset]), fabs(ins[1][offset]));
            for(int channel = 0; channel < in_count; channel++) {
                if(asample > threshold) {
                    target = asample - threshold;
                    target /= ratio;
                    target += threshold;
                    target /= asample;
                } else {
                    target = 1;
                }
                
                outs[channel][offset] = ins[channel][offset] * aim * makeup;
            }
            
            aim += (target - aim) * (aim > target ? attack_coeff : release_coeff);
            ++offset;
        }

        return inputs_mask;
    }

    static const char *get_name() { return "compressor"; }
    static const char *get_id() { return "compressor"; }
    static const char *get_label() { return "Compressor"; }
private:
    float aim, target;
};

#endif
    
};

#endif
