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

#include "loudness.h"

namespace calf_plugins {

#if ENABLE_EXPERIMENTAL

class compressor_audio_module: public audio_module<compressor_metadata>, public line_graph_iface {
private:
    float linslope, clip, peak, detected;
    bool aweighting;
    aweighter awL, awR;
public:

    float *ins[in_count];
    float *outs[out_count];
    float *params[param_count];
    uint32_t srate;
    void activate() {
        linslope = 0.f;
        peak = 0.f;
        clip = 0.f;
    }
    uint32_t process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask) {
        bool bypass = *params[param_bypass] > 0.5f;
        bool rms = *params[param_detection] == 0;
        bool average = *params[param_stereo_link] == 0;
        aweighting = *params[param_aweighting] > 0.5f;
        float threshold = *params[param_threshold];
        float ratio = *params[param_ratio];
        float attack = *params[param_attack];
        float attack_coeff = std::min(1.f, 1.f / (attack * srate / 4000.f));
        float release = *params[param_release];
        float release_coeff = std::min(1.f, 1.f / (release * srate / 4000.f));
        float makeup = *params[param_makeup];
        float knee = *params[param_knee];

        numsamples += offset;
        
        if(bypass) {
            int count = numsamples * sizeof(float);
            memcpy(outs[0], ins[0], count);
            memcpy(outs[1], ins[1], count);

            if(params[param_compression] != NULL) {
                *params[param_compression] = 1.f;
            }

            if(params[param_clip] != NULL) {
                *params[param_clip] = 0.f;
            }

            if(params[param_peak] != NULL) {
                *params[param_peak] = 0.f;
            }
      
            return inputs_mask;
        }

        float gain = 1.f;
        
        while(offset < numsamples) {
            float left = ins[0][offset];
            float right = ins[1][offset];
            if(aweighting) {
                left = awL.process(left);
                right = awR.process(right);
            }
            float absample = average ? (fabs(left) + fabs(right)) / 2 : std::max(fabs(left), fabs(right));
            if(rms) absample *= absample;
            linslope += (absample - linslope) * (absample > linslope ? attack_coeff : release_coeff);
            float slope = rms ? sqrt(linslope) : linslope;
            detected = slope;

            if(slope > 0.f && (slope > threshold || knee < 1.f)) {
                if(IS_FAKE_INFINITY(ratio)) {
                    gain = threshold;
                } else {
                    gain = (slope - threshold) / ratio + threshold;
                }
                
                if(knee < 1.f) {
                    float t = std::min(1.f, std::max(0.f, slope / threshold - knee) / (1.f - knee));
                    gain = (gain - slope) * t + slope;
                }
                
                gain /= slope;
            }
        
            
            float outL = ins[0][offset] * gain * makeup;
            outs[0][offset] = outL;

            float outR = ins[1][offset] * gain * makeup;
            outs[1][offset] = outR;

            ++offset;
            
            float maxLR = std::max(fabs(outL), fabs(outR));
            
            if(clip > 0) {
                --clip;
            }
            if(maxLR > 1.f) clip = srate / 10.f; /* blink clip LED for 100 ms */
            
            peak -= peak * 0.0001;
            if(maxLR > peak) {
                peak = maxLR;
            }
        }
        
        if(params[param_compression] != NULL) {
            *params[param_compression] = gain;
        }

        if(params[param_clip] != NULL) {
            *params[param_clip] = clip;
        }

        if(params[param_peak] != NULL) {
            *params[param_peak] = peak;
        }

        return inputs_mask;
    }

    void set_sample_rate(uint32_t sr) {
            srate = sr;
            awL.set(sr);
            awR.set(sr);
    }
    inline float output_level(float slope)
    {
        float threshold = *params[param_threshold];
        float ratio = *params[param_ratio];
        float makeup = *params[param_makeup];
        float knee = *params[param_knee];
        
        if(slope > 0.f && (slope > threshold || knee < 1.f)) {
            float gain = 0.f;
            if(IS_FAKE_INFINITY(ratio)) {
                gain = threshold;
            } else {
                gain = (slope - threshold) / ratio + threshold;
            }
            
            if(knee < 1.f) {
                float t = std::min(1.f, std::max(0.f, slope / threshold - knee) / (1.f - knee));
                gain = (gain - slope) * t + slope;
            }
            return gain * makeup;
        }
        return slope * makeup;
    }
    virtual bool get_graph(int index, int subindex, float *data, int points, cairo_t *context) { 
        if (subindex > 0) // 1
            return false;
        for (int i = 0; i < points; i++)
        {
            float input = pow(65536.0, i * 1.0 / (points - 1) - 1);
            float output = output_level(input);
            //if (subindex == 0)
            //    data[i] = 1 + 2 * log(input) / log(65536);
            //else
            data[i] = 1 + 2 * log(output) / log(65536);
        }
        return true;
    }
    virtual bool get_dot(int index, int subindex, float &x, float &y, int &size, cairo_t *context) {
        if (!subindex)
        {
            x = 1 + 2 * log(detected) / log(65536);
            y = 1 + 2 * log(output_level(detected)) / log(65536);
            return true;
        }
        return false;
    }
};

#endif
    
};

#endif
