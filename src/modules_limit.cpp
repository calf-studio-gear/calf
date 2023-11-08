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
#include <calf/audio_fx.h>
#include <calf/modules_limit.h>
#include <calf/utils.h>

using namespace dsp;
using namespace calf_plugins;

#define SET_IF_CONNECTED(name) if (params[AM::param_##name] != NULL) *params[AM::param_##name] = name;

FORWARD_DECLARE_METADATA(limiter)
FORWARD_DECLARE_METADATA(multibandlimiter)
FORWARD_DECLARE_METADATA(sidechainlimiter)

/**********************************************************************
 * LIMITER by Christian Holschuh and Markus Schmidt
**********************************************************************/

limiter_audio_module::limiter_audio_module()
{
    is_active = false;
    srate = 0;
    asc_led    = 0.f;
    attack_old = -1.f;
    limit_old = -1.f;
    oversampling_old = -1;
    asc_old = true;
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
void limiter_audio_module::set_srates()
{
    if (params[param_oversampling]) {
        resampler[0].set_params(srate, *params[param_oversampling], 2);
        resampler[1].set_params(srate, *params[param_oversampling], 2);
        limiter.set_sample_rate(srate * *params[param_oversampling]);
    }
}
void limiter_audio_module::params_changed()
{
    limiter.set_params(*params[param_limit], *params[param_attack], *params[param_release], 1.f, *params[param_asc], pow(0.5, (*params[param_asc_coeff] - 0.5) * 2 * -1), true);
    if( *params[param_attack] != attack_old) {
        attack_old = *params[param_attack];
        limiter.reset();
    }
    if(*params[param_limit] != limit_old || *params[param_asc] != asc_old) {
        asc_old = *params[param_asc];
        limit_old = *params[param_limit];
        limiter.reset_asc();
    }
    if (oversampling_old != *params[param_oversampling]) {
        oversampling_old = *params[param_oversampling];
        set_srates();
    }
}

void limiter_audio_module::set_sample_rate(uint32_t sr)
{
    srate = sr;
    int meter[] = {param_meter_inL, param_meter_inR,  param_meter_outL, param_meter_outR, -param_att};
    int clip[] = {param_clip_inL, param_clip_inR, param_clip_outL, param_clip_outR, -1};
    meters.init(params, meter, clip, 5, srate);
    set_srates();
}

uint32_t limiter_audio_module::process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask)
{
    bool bypassed = bypass.update(*params[param_bypass] > 0.5f, numsamples);
    uint32_t orig_numsamples = numsamples;
    uint32_t orig_offset = offset;
    numsamples += offset;
    if(bypassed) {
        // everything bypassed
        while(offset < numsamples) {
            outs[0][offset] = ins[0][offset];
            outs[1][offset] = ins[1][offset];
            float values[] = {0, 0, 0, 0, 1};
            meters.process(values);
            ++offset;
        }
        asc_led    = 0.f;
    } else {
        asc_led   -= std::min(asc_led, numsamples);

        // allocate fickdich on the stack before entering loop
        STACKALLOC(float, fickdich, limiter.overall_buffer_size);
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
            
            // upsampling
            double *samplesL = resampler[0].upsample((double)outL);
            double *samplesR = resampler[1].upsample((double)outR);
            
            float tmpL;
            float tmpR;
            
            // process gain reduction
            for (int i = 0; i < *params[param_oversampling]; i ++) {
                tmpL = samplesL[i];
                tmpR = samplesR[i];
                limiter.process(tmpL, tmpR, fickdich);
                samplesL[i] = tmpL;
                samplesR[i] = tmpR;
                if(limiter.get_asc())
                    asc_led = srate >> 3;
            }
            
            // downsampling
            outL = resampler[0].downsample(samplesL);
            outR = resampler[1].downsample(samplesR);
            
            // should never be used. but hackers are paranoid by default.
            // so we make shure NOTHING is above limit
            outL = std::min(std::max(outL, -*params[param_limit]), *params[param_limit]);
            outR = std::min(std::max(outR, -*params[param_limit]), *params[param_limit]);

            // autolevel
            if (*params[param_auto_level]) {
                outL /= *params[param_limit];
                outR /= *params[param_limit];
            }

            // out level
            outL *= *params[param_level_out];
            outR *= *params[param_level_out];

            // send to output
            outs[0][offset] = outL;
            outs[1][offset] = outR;

            float values[] = {inL, inR, outL, outR, bypassed ? (float)1.0 : (float)limiter.get_attenuation()};
            meters.process (values);

            // next sample
            ++offset;
        } // cycle trough samples
        bypass.crossfade(ins, outs, 2, orig_offset, orig_numsamples);
    } // process (no bypass)
    meters.fall(numsamples);
    if (params[param_asc_led] != NULL) *params[param_asc_led] = asc_led;
    return outputs_mask;
}

/**********************************************************************
 * MULTIBAND LIMITER by Markus Schmidt and Christian Holschuh 
**********************************************************************/

multibandlimiter_audio_module::multibandlimiter_audio_module()
{
    srate               = 0;
    _mode               = 0;
    over                = 1;
    buffer_size         = 0;
    overall_buffer_size = 0;
    channels            = 2;
    asc_led             = 0.f;
    attack_old          = -1.f;
    oversampling_old    = -1.f;
    limit_old           = -1.f;
    asc_old             = true;
    _sanitize           = false;
    is_active           = false;
    cnt = 0;
    buffer = NULL;
    
    for(int i = 0; i < strips; i ++) {
        weight_old[i] = -1.f;
    }
    
    crossover.init(channels, strips, 44100);
}
multibandlimiter_audio_module::~multibandlimiter_audio_module()
{
    free(buffer);
}
void multibandlimiter_audio_module::activate()
{
    is_active = true;
    // set all filters and strips
    params_changed();
    // activate all strips
    for (int j = 0; j < strips; j ++) {
        strip[j].activate();
        strip[j].set_multi(true);
        strip[j].id = j;
    }
    broadband.activate();
    pos = 0;
}

void multibandlimiter_audio_module::deactivate()
{
    is_active = false;
    // deactivate all strips
    for (int j = 0; j < strips; j ++) {
        strip[j].deactivate();
    }
    broadband.deactivate();
}

void multibandlimiter_audio_module::params_changed()
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
    float rel;
    for (int i = 0; i < strips; i++) {
        rel = *params[param_release] *  pow(0.25, *params[param_release0 + i] * -1);
        rel = (*params[param_minrel] > 0.5) ? std::max(2500 * (1.f / (i ? *params[param_freq0 + i - 1] : 30)), rel) : rel;
        weight[i] = pow(0.25, *params[param_weight0 + i] * -1);
        strip[i].set_params(*params[param_limit], *params[param_attack], rel, weight[i], *params[param_asc], pow(0.5, (*params[param_asc_coeff] - 0.5) * 2 * -1), false);
        *params[param_effrelease0 + i] = rel;
    }
    
    // set broadband limiter
    broadband.set_params(*params[param_limit], *params[param_attack], rel, 1.f, *params[param_asc], pow(0.5, (*params[param_asc_coeff] - 0.5) * 2 * -1));
    
    if (over != *params[param_oversampling]) {
        over = *params[param_oversampling];
        set_srates();
    }
    
    // rebuild multiband buffer
    if( *params[param_attack] != attack_old || *params[param_oversampling] != oversampling_old) {
        int bs           = (int)(srate * (*params[param_attack] / 1000.f) * channels * over);
        buffer_size      = bs - bs % channels; // buffer size attack rate
        attack_old       = *params[param_attack];
        oversampling_old = *params[param_oversampling];
        _sanitize        = true;
        pos              = 0;
        for (int j = 0; j < strips; j ++) {
            strip[j].reset();
        }
        broadband.reset();
    }
    if(*params[param_limit] != limit_old || *params[param_asc] != asc_old || *params[param_weight0] != weight_old[0] || *params[param_weight1] != weight_old[1] || *params[param_weight2] != weight_old[2] || *params[param_weight3] != weight_old[3] ) {
        asc_old    = *params[param_asc];
        limit_old  = *params[param_limit];
        for (int j = 0; j < strips; j ++) {
            weight_old[j] = *params[param_weight0 + j];
            strip[j].reset_asc();
        }
        broadband.reset_asc();
    }
}

void multibandlimiter_audio_module::set_sample_rate(uint32_t sr)
{
    srate = sr;
    set_srates();
    int meter[] = {param_meter_inL, param_meter_inR,  param_meter_outL, param_meter_outR, -param_att0, -param_att1, -param_att2, -param_att3};
    int clip[] = {param_clip_inL, param_clip_inR, param_clip_outL, param_clip_outR, -1, -1, -1, -1};
    meters.init(params, meter, clip, 8, srate);
}

void multibandlimiter_audio_module::set_srates()
{
    broadband.set_sample_rate(srate * over);
    crossover.set_sample_rate(srate);
    for (int j = 0; j < strips; j ++) {
        strip[j].set_sample_rate(srate * over);
        resampler[j][0].set_params(srate, over, 2);
        resampler[j][1].set_params(srate, over, 2);
    }
    // rebuild buffer
    overall_buffer_size = (int)(srate * (100.f / 1000.f) * channels * over) + channels; // buffer size max attack rate
    buffer = (float*) calloc(overall_buffer_size, sizeof(float));
    pos = 0;
}

#define BYPASSED_COMPRESSION(index) \
    if(params[param_att##index] != NULL) \
        *params[param_att##index] = 1.0; \

#define ACTIVE_COMPRESSION(index) \
    if(params[param_att##index] != NULL) \
        *params[param_att##index] = strip[index].get_attenuation(); \


uint32_t multibandlimiter_audio_module::process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask)
{
    bool bypassed = bypass.update(*params[param_bypass] > 0.5f, numsamples);
    uint32_t orig_numsamples = numsamples;
    uint32_t orig_offset = offset;
    numsamples += offset;
    float batt = 0.f;
    if(bypassed) {
        // everything bypassed
        while(offset < numsamples) {
            outs[0][offset] = ins[0][offset];
            outs[1][offset] = ins[1][offset];
            float values[] = {0, 0, 0, 0, 1, 1, 1, 1};
            meters.process(values);
            ++offset;
        }
        asc_led    = 0.f;
    } else {
        // process all strips
        asc_led     -= std::min(asc_led, numsamples);
        // allocate fickdich on the stack before entering loop to avoid buffer overflow
        STACKALLOC(float, fickdich, broadband.overall_buffer_size);
        while(offset < numsamples) {
            float inL  = 0.f; // input
            float inR  = 0.f;
            float outL = 0.f; // final output
            float outR = 0.f;
            float tmpL = 0.f; // used for temporary purposes
            float tmpR = 0.f;
            double overL[strips * 16];
            double overR[strips * 16];
            double resL[16];
            double resR[16];
            
            bool asc_active = false;
            
            // cycle through samples
            if(!_sanitize) {
                inL = ins[0][offset];
                inR = ins[1][offset];
            }
            // in level
            inR *= *params[param_level_in];
            inL *= *params[param_level_in];
            
            //if(!(cnt%50)) printf("i: %.5f\n", inL);
            
            // process crossover
            float xin[] = {inL, inR};
            crossover.process(xin);
            
            // cycle over strips
            for (int i = 0; i < strips; i++) {
                // upsample
                double *samplesL = resampler[i][0].upsample((double)crossover.get_value(0, i));
                double *samplesR = resampler[i][1].upsample((double)crossover.get_value(1, i));
                // copy to cache
                memcpy(&overL[i * 16], samplesL, sizeof(double) * over);
                memcpy(&overR[i * 16], samplesR, sizeof(double) * over);
                //if(!(cnt%200)) printf("u0: %.5f\n", overL[i*16]);
            }
            
            // cycle over upsampled samples
            for (int o = 0; o < over; o++) {
                tmpL = 0.f;
                tmpR = 0.f;
                resL[o] = 0;
                resR[o] = 0;
                
                // cycle over strips for multiband coefficient
                
                // -------------------------------------------
                // The Multiband Coefficient
                //
                // The Multiband Coefficient tries to make sure, that after
                // summing up the 4 limited strips, the signal does not raise
                // above the limit. It works as a correction factor. Because
                // we use this concept, we can introduce a weighting to each
                // strip.
                // a1, a2, a3, ... : signals in strips
                // then a1 + a2 + a3 + ... might raise above the limit, because
                // the strips will be limited and the filters, which produced
                // the signals from source signals, are not complete precisely.
                // Morethough, external signals might be added in here in future
                // versions.
                //
                // So introduce correction factor:
                // Sum( a_i * weight_i) = limit / multi_coeff
                //
                // The multi_coeff now can be used in each strip i, to calculate
                // the real limit for strip i according to the signals in the
                // other strips and the weighting of the own strip i.
                // strip_limit_i = limit * multicoeff * weight_i
                //
                // -------------------------------------------
                
                
                for (int i = 0; i < strips; i++) {
                    // sum up for multiband coefficient
                    int p = i * 16 + o;
                    tmpL += ((fabs(overL[p]) > *params[param_limit]) ? *params[param_limit] * (fabs(overL[p]) / overL[p]) : overL[p]) * weight[i];
                    tmpR += ((fabs(overR[p]) > *params[param_limit]) ? *params[param_limit] * (fabs(overR[p]) / overR[p]) : overR[p]) * weight[i];
                }
                
                // write multiband coefficient to buffer
                buffer[pos] = std::min((float)(*params[param_limit] / std::max(fabs(tmpL), fabs(tmpR))), 1.0f);
                
                // step forward in multiband buffer
                pos = (pos + channels) % buffer_size;
                if(pos == 0) _sanitize = false;
                
                // limit and add up strips
                for (int i = 0; i < strips; i++) {
                    int p = i * 16 + o;
                    //if(!(cnt%200) and !o) printf("u1: %.5f\n", overL[p]);
                    // limit
                    tmpL = (float)overL[p];
                    tmpR = (float)overR[p];
                    //if(!(cnt%200)) printf("1: %.5f\n", tmpL);
                    strip[i].process(tmpL, tmpR, buffer);
                    //if(!(cnt%200)) printf("2: %.5f\n\n", tmpL);
                    if (solo[i] || no_solo) {
                        // add
                        resL[o] += (double)tmpL;
                        resR[o] += (double)tmpR;
                        // flash the asc led?
                        asc_active = asc_active || strip[i].get_asc();
                    }
                }
                
                // process broadband limiter
                tmpL = resL[o];
                tmpR = resR[o];
                broadband.process(tmpL, tmpR, fickdich);
                resL[o] = (double)tmpL;
                resR[o] = (double)tmpR;
                asc_active = asc_active || broadband.get_asc();
            }
            
            // downsampling
            outL = (float)resampler[0][0].downsample(resL);
            outR = (float)resampler[0][1].downsample(resR);
            
            //if(!(cnt%50)) printf("o: %.5f %.5f\n\n", outL, resL[0]);
            
            // should never be used. but hackers are paranoid by default.
            // so we make shure NOTHING is above limit
            outL = std::min(std::max(outL, -*params[param_limit]), *params[param_limit]);
            outR = std::min(std::max(outR, -*params[param_limit]), *params[param_limit]);
            
            // light led
            if(asc_active)  {
                asc_led = srate >> 3;
            }
            
            // autolevel
            if (*params[param_auto_level]) {
                outL /= *params[param_limit];
                outR /= *params[param_limit];
            }

            // out level
            outL *= *params[param_level_out];
            outR *= *params[param_level_out];

            // send to output
            outs[0][offset] = outL;
            outs[1][offset] = outR;

            // next sample
            ++offset;
            
            batt = broadband.get_attenuation();
            
            float values[] = {inL, inR, outL, outR,
                bypassed ? (float)1.0 : (float)strip[0].get_attenuation() * batt,
                bypassed ? (float)1.0 : (float)strip[1].get_attenuation() * batt,
                bypassed ? (float)1.0 : (float)strip[2].get_attenuation() * batt,
                bypassed ? (float)1.0 : (float)strip[3].get_attenuation() * batt};
            meters.process(values);
            
            
            
            //for (int i = 0; i < strips; i++) {
                //left  = crossover.get_value(0, i);
                //right = crossover.get_value(1, i);
                
                //// remember filtered values for limiting
                //// (we need multiband_coeff before we can call the limiter bands)
                //_tmpL[i] = left;
                //_tmpR[i] = right;

                //// sum up for multiband coefficient
                //sum_left += ((fabs(left) > *params[param_limit]) ? *params[param_limit] * (fabs(left) / left) : left) * weight[i];
                //sum_right += ((fabs(right) > *params[param_limit]) ? *params[param_limit] * (fabs(right) / right) : right) * weight[i];
            //} // process single strip with filter

            //// write multiband coefficient to buffer
            //buffer[pos] = std::min(*params[param_limit] / std::max(fabs(sum_left), fabs(sum_right)), 1.0);

            //for (int i = 0; i < strips; i++) {
                //// process gain reduction
                //strip[i].process(_tmpL[i], _tmpR[i], buffer);
                //// sum up output of limiters
                //if (solo[i] || no_solo) {
                    //outL += _tmpL[i];
                    //outR += _tmpR[i];
                //}
                //asc_active = asc_active || strip[i].get_asc();
            //} // process single strip again for limiter
            //float fickdich[0];
            //broadband.process(outL, outR, fickdich);
            //asc_active = asc_active || broadband.get_asc();
            
            //// should never be used. but hackers are paranoid by default.
            //// so we make shure NOTHING is above limit
            //outL = std::max(outL, -*params[param_limit]);
            //outL = std::min(outL, *params[param_limit]);
            //outR = std::max(outR, -*params[param_limit]);
            //outR = std::min(outR, *params[param_limit]);
            
            //if(asc_active)  {
                //asc_led = srate >> 3;
            //}
            cnt++;
        } // cycle trough samples
        bypass.crossfade(ins, outs, 2, orig_offset, orig_numsamples);
    } // process (no bypass)
    if (params[param_asc_led] != NULL) *params[param_asc_led] = asc_led;
    meters.fall(numsamples);
    return outputs_mask;
}

bool multibandlimiter_audio_module::get_graph(int index, int subindex, int phase, float *data, int points, cairo_iface *context, int *mode) const
{
    return crossover.get_graph(subindex, phase, data, points, context, mode);
}
bool multibandlimiter_audio_module::get_layers(int index, int generation, unsigned int &layers) const
{
    return crossover.get_layers(index, generation, layers);
}

/**********************************************************************
 * SIDECHAIN LIMITER by Markus Schmidt and Christian Holschuh 
**********************************************************************/

sidechainlimiter_audio_module::sidechainlimiter_audio_module()
{
    srate               = 0;
    _mode               = 0;
    over                = 1;
    buffer_size         = 0;
    overall_buffer_size = 0;
    channels            = 2;
    asc_led             = 0.f;
    attack_old          = -1.f;
    oversampling_old    = -1.f;
    limit_old           = -1.f;
    asc_old             = true;
    _sanitize           = false;
    is_active           = false;
    cnt = 0;
    buffer = NULL;
    
    for(int i = 0; i < strips; i ++) {
        weight_old[i] = -1.f;
    }
    
    crossover.init(channels, strips - 1, 44100);
}
sidechainlimiter_audio_module::~sidechainlimiter_audio_module()
{
    free(buffer);
}
void sidechainlimiter_audio_module::activate()
{
    is_active = true;
    // set all filters and strips
    params_changed();
    // activate all strips
    for (int j = 0; j < strips; j ++) {
        strip[j].activate();
        strip[j].set_multi(true);
        strip[j].id = j;
    }
    broadband.activate();
    pos = 0;
}

void sidechainlimiter_audio_module::deactivate()
{
    is_active = false;
    // deactivate all strips
    for (int j = 0; j < strips; j ++) {
        strip[j].deactivate();
    }
    broadband.deactivate();
}

void sidechainlimiter_audio_module::params_changed()
{
    // determine mute/solo states
    solo[0] = *params[param_solo0] > 0.f ? true : false;
    solo[1] = *params[param_solo1] > 0.f ? true : false;
    solo[2] = *params[param_solo2] > 0.f ? true : false;
    solo[3] = *params[param_solo3] > 0.f ? true : false;
    solo[4] = *params[param_solo_sc] > 0.f ? true : false;
    no_solo = (*params[param_solo0] > 0.f ||
            *params[param_solo1] > 0.f ||
            *params[param_solo2] > 0.f ||
            *params[param_solo3] > 0.f ||
            *params[param_solo_sc] > 0.f) ? false : true;

    int m = *params[param_mode];
    if (m != _mode) {
        _mode = *params[param_mode];
    }
    
    crossover.set_mode(_mode + 1);
    crossover.set_filter(0, *params[param_freq0]);
    crossover.set_filter(1, *params[param_freq1]);
    crossover.set_filter(2, *params[param_freq2]);
    
    // set the params of all strips
    float rel;
    for (int i = 0; i < strips; i++) {
        rel = *params[param_release] *  pow(0.25, *params[param_release0 + i] * -1);
        if (i < strips - 1) 
            rel = (*params[param_minrel] > 0.5) ? std::max(2500 * (1.f / (i ? *params[param_freq0 + i - 1] : 30)), rel) : rel;
        weight[i] = pow(0.25, *params[param_weight0 + i] * -1);
        strip[i].set_params(*params[param_limit], *params[param_attack], rel, weight[i], *params[param_asc], pow(0.5, (*params[param_asc_coeff] - 0.5) * 2 * -1), false);
        *params[param_effrelease0 + i] = rel;
    }
    
    // set broadband limiter
    broadband.set_params(*params[param_limit], *params[param_attack], rel, 1.f, *params[param_asc], pow(0.5, (*params[param_asc_coeff] - 0.5) * 2 * -1));
    
    if (over != *params[param_oversampling]) {
        over = *params[param_oversampling];
        set_srates();
    }
    
    // rebuild multiband buffer
    if( *params[param_attack] != attack_old || *params[param_oversampling] != oversampling_old) {
        int bs           = (int)(srate * (*params[param_attack] / 1000.f) * channels * over);
        buffer_size      = bs - bs % channels; // buffer size attack rate
        attack_old       = *params[param_attack];
        oversampling_old = *params[param_oversampling];
        _sanitize        = true;
        pos              = 0;
        for (int j = 0; j < strips; j ++) {
            strip[j].reset();
        }
        broadband.reset();
    }
    if(*params[param_limit] != limit_old || *params[param_asc] != asc_old || *params[param_weight0] != weight_old[0] || *params[param_weight1] != weight_old[1] || *params[param_weight2] != weight_old[2] || *params[param_weight3] != weight_old[3] ) {
        asc_old    = *params[param_asc];
        limit_old  = *params[param_limit];
        for (int j = 0; j < strips; j ++) {
            weight_old[j] = *params[param_weight0 + j];
            strip[j].reset_asc();
        }
        broadband.reset_asc();
    }
}

void sidechainlimiter_audio_module::set_sample_rate(uint32_t sr)
{
    srate = sr;
    set_srates();
    int meter[] = {param_meter_inL, param_meter_inR, param_meter_scL, param_meter_scR, param_meter_outL, param_meter_outR, -param_att0, -param_att1, -param_att2, -param_att3, -param_att_sc};
    int clip[] = {param_clip_inL, param_clip_inR, -1, -1, param_clip_outL, param_clip_outR, -1, -1, -1, -1, -1};
    meters.init(params, meter, clip, 8, srate);
}

void sidechainlimiter_audio_module::set_srates()
{
    broadband.set_sample_rate(srate * over);
    crossover.set_sample_rate(srate);
    for (int j = 0; j < strips; j ++) {
        strip[j].set_sample_rate(srate * over);
        resampler[j][0].set_params(srate, over, 2);
        resampler[j][1].set_params(srate, over, 2);
    }
    // rebuild buffer
    overall_buffer_size = (int)(srate * (100.f / 1000.f) * channels * over) + channels; // buffer size max attack rate
    buffer = (float*) calloc(overall_buffer_size, sizeof(float));
    pos = 0;
}

#define BYPASSED_COMPRESSION(index) \
    if(params[param_att##index] != NULL) \
        *params[param_att##index] = 1.0; \

#define ACTIVE_COMPRESSION(index) \
    if(params[param_att##index] != NULL) \
        *params[param_att##index] = strip[index].get_attenuation(); \


uint32_t sidechainlimiter_audio_module::process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask)
{
    bool bypassed = bypass.update(*params[param_bypass] > 0.5f, numsamples);
    uint32_t orig_numsamples = numsamples;
    uint32_t orig_offset = offset;
    numsamples += offset;
    float batt = 0.f;
    if(bypassed) {
        // everything bypassed
        while(offset < numsamples) {
            outs[0][offset] = ins[0][offset];
            outs[1][offset] = ins[1][offset];
            float values[] = {0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1};
            meters.process(values);
            ++offset;
        }
        asc_led    = 0.f;
    } else {
        // process all strips
        asc_led     -= std::min(asc_led, numsamples);
        // allocate fickdich on the stack before loops to avoid buffer overflow
        STACKALLOC(float, fickdich, broadband.overall_buffer_size);
        while(offset < numsamples) {
            float inL  = 0.f; // input
            float inR  = 0.f;
            float scL  = 0.f;
            float scR  = 0.f;
            float outL = 0.f; // final output
            float outR = 0.f;
            float tmpL = 0.f; // used for temporary purposes
            float tmpR = 0.f;
            double overL[strips * 16];
            double overR[strips * 16];
            double resL[16];
            double resR[16];
            
            bool asc_active = false;
            
            // cycle through samples
            if(!_sanitize) {
                inL = ins[0][offset];
                inR = ins[1][offset];
                scL = ins[2] ? ins[2][offset] : 0;
                scR = ins[3] ? ins[3][offset] : 0;
            }
            
            // in level
            inR *= *params[param_level_in];
            inL *= *params[param_level_in];
            
            scR *= *params[param_level_sc];
            scL *= *params[param_level_sc];
            
            //if(!(cnt%50)) printf("i: %.5f\n", inL);
            
            // process crossover
            float xin[] = {inL, inR};
            crossover.process(xin);
            
            // cycle over strips
            for (int i = 0; i < strips; i++) {
                double *samplesR, *samplesL;
                // upsample
                if (i < strips - 1) {
                    samplesL = resampler[i][0].upsample((double)crossover.get_value(0, i));
                    samplesR = resampler[i][1].upsample((double)crossover.get_value(1, i));
                } else {
                    samplesL = resampler[i][0].upsample((double)scL);
                    samplesR = resampler[i][1].upsample((double)scR);
                }
                // copy to cache
                memcpy(&overL[i * 16], samplesL, sizeof(double) * over);
                memcpy(&overR[i * 16], samplesR, sizeof(double) * over);
                //if(!(cnt%200)) printf("u0: %.5f\n", overL[i*16]);
            }
            
            // cycle over upsampled samples
            for (int o = 0; o < over; o++) {
                tmpL = 0.f;
                tmpR = 0.f;
                resL[o] = 0;
                resR[o] = 0;
                
                // cycle over strips for multiband coefficient
                for (int i = 0; i < strips; i++) {
                    // sum up for multiband coefficient
                    int p = i * 16 + o;
                    tmpL += ((fabs(overL[p]) > *params[param_limit]) ? *params[param_limit] * (fabs(overL[p]) / overL[p]) : overL[p]) * weight[i];
                    tmpR += ((fabs(overR[p]) > *params[param_limit]) ? *params[param_limit] * (fabs(overR[p]) / overR[p]) : overR[p]) * weight[i];
                }
                
                // write multiband coefficient to buffer
                buffer[pos] = std::min((float)(*params[param_limit] / std::max(fabs(tmpL), fabs(tmpR))), 1.0f);
                
                // step forward in multiband buffer
                pos = (pos + channels) % buffer_size;
                if(pos == 0) _sanitize = false;
                
                // limit and add up strips
                for (int i = 0; i < strips; i++) {
                    int p = i * 16 + o;
                    //if(!(cnt%200) and !o) printf("u1: %.5f\n", overL[p]);
                    // limit
                    tmpL = (float)overL[p];
                    tmpR = (float)overR[p];
                    //if(!(cnt%200)) printf("1: %.5f\n", tmpL);
                    strip[i].process(tmpL, tmpR, buffer);
                    //if(!(cnt%200)) printf("2: %.5f\n\n", tmpL);
                    if (solo[i] || no_solo) {
                        // add
                        resL[o] += (double)tmpL;
                        resR[o] += (double)tmpR;
                        // flash the asc led?
                        asc_active = asc_active || strip[i].get_asc();
                    }
                }
                
                // process broadband limiter
                tmpL = resL[o];
                tmpR = resR[o];
                broadband.process(tmpL, tmpR, fickdich);
                resL[o] = (double)tmpL;
                resR[o] = (double)tmpR;
                asc_active = asc_active || broadband.get_asc();
            }
            
            // downsampling
            outL = (float)resampler[0][0].downsample(resL);
            outR = (float)resampler[0][1].downsample(resR);
            
            //if(!(cnt%50)) printf("o: %.5f %.5f\n\n", outL, resL[0]);
            
            // should never be used. but hackers are paranoid by default.
            // so we make shure NOTHING is above limit
            outL = std::min(std::max(outL, -*params[param_limit]), *params[param_limit]);
            outR = std::min(std::max(outR, -*params[param_limit]), *params[param_limit]);
            
            // light led
            if(asc_active)  {
                asc_led = srate >> 3;
            }
            
            // autolevel
            if (*params[param_auto_level]) {
                outL /= *params[param_limit];
                outR /= *params[param_limit];
            }

            // out level
            outL *= *params[param_level_out];
            outR *= *params[param_level_out];

            // send to output
            outs[0][offset] = outL;
            outs[1][offset] = outR;

            // next sample
            ++offset;
            
            batt = broadband.get_attenuation();
            
            float values[] = {inL, inR, scL, scR, outL, outR,
                bypassed ? (float)1.0 : (float)strip[0].get_attenuation() * batt,
                bypassed ? (float)1.0 : (float)strip[1].get_attenuation() * batt,
                bypassed ? (float)1.0 : (float)strip[2].get_attenuation() * batt,
                bypassed ? (float)1.0 : (float)strip[3].get_attenuation() * batt,
                bypassed ? (float)1.0 : (float)strip[4].get_attenuation() * batt};
            meters.process(values);
            
            cnt++;
        } // cycle trough samples
        bypass.crossfade(ins, outs, 2, orig_offset, orig_numsamples);
    } // process (no bypass)
    if (params[param_asc_led] != NULL) *params[param_asc_led] = asc_led;
    meters.fall(numsamples);
    return outputs_mask;
}

bool sidechainlimiter_audio_module::get_graph(int index, int subindex, int phase, float *data, int points, cairo_iface *context, int *mode) const
{
    return crossover.get_graph(subindex, phase, data, points, context, mode);
}
bool sidechainlimiter_audio_module::get_layers(int index, int generation, unsigned int &layers) const
{
    return crossover.get_layers(index, generation, layers);
}
