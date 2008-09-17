/* Calf DSP Library
 * "Small" audio modules for modular synthesis
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
#ifndef __CALF_MODULES_SMALL_H
#define __CALF_MODULES_SMALL_H

#include <assert.h>
#include "biquad.h"
#include "inertia.h"
#include "audio_fx.h"
#include "giface.h"
#include "modules.h"
#include "plugininfo.h"

namespace synth {

using namespace dsp;

class small_filter_audio_module: public null_audio_module
{
public:    
    enum { in_signal, in_cutoff, in_resonance, in_count };
    enum { out_signal, out_count};
    float *ins[in_count]; 
    float *outs[out_count];
    static void port_info(plugin_info_iface *pii)
    {
        pii->audio_port("in", "In").input();
        pii->control_port("cutoff", "Cutoff", 1000).input().log_range(20, 20000);
        pii->control_port("res", "Resonance", 0.707).input().log_range(0.707, 20);
        pii->audio_port("out", "Out").output();
    }
    dsp::biquad<float> filter;
    uint32_t srate;
    
    void activate() {
        filter.reset();
    }
    void set_sample_rate(uint32_t sr) {
        srate = sr;
    }
    inline void process_inner(uint32_t count) {
        for (uint32_t i = 0; i < count; i++)
            outs[out_signal][i] = filter.process_d1(ins[in_signal][i]);
        filter.sanitize_d1();
    }
};

class small_lp_filter_audio_module: public small_filter_audio_module
{
public:
    inline void process(uint32_t count) {
        filter.set_lp_rbj(clip<float>(*ins[in_cutoff], 0.0001, 0.48 * srate), *ins[in_resonance], srate);
        process_inner(count);
    }
    static void plugin_info(plugin_info_iface *pii)
    {
        pii->names("lowpass12", "lowpass12", "12dB/oct RBJ Lowpass", "lv2:LowpassPlugin");
        port_info(pii);
    }
};

class small_hp_filter_audio_module: public small_filter_audio_module
{
public:
    inline void process(uint32_t count) {
        filter.set_hp_rbj(clip<float>(*ins[in_cutoff], 0.0001, 0.48 * srate), *ins[in_resonance], srate);
        process_inner(count);
    }
    static void plugin_info(plugin_info_iface *pii)
    {
        pii->names("highpass12", "highpass12", "12dB/oct RBJ Highpass", "lv2:HighpassPlugin");
        port_info(pii);
    }
};

class small_bp_filter_audio_module: public small_filter_audio_module
{
public:
    inline void process(uint32_t count) {
        filter.set_bp_rbj(clip<float>(*ins[in_cutoff], 0.0001, 0.48 * srate), *ins[in_resonance], srate);
        process_inner(count);
    }
    static void plugin_info(plugin_info_iface *pii)
    {
        pii->names("bandpass6", "bandpass6", "6dB/oct RBJ Bandpass", "lv2:BandpassPlugin");
        port_info(pii);
    }
};

class small_br_filter_audio_module: public small_filter_audio_module
{
public:
    inline void process(uint32_t count) {
        filter.set_br_rbj(clip<float>(*ins[in_cutoff], 0.0001, 0.48 * srate), *ins[in_resonance], srate);
        process_inner(count);
    }
    static void plugin_info(plugin_info_iface *pii)
    {
        pii->names("notch6", "notch6", "6dB/oct RBJ Bandpass", "lv2:FilterPlugin");
        port_info(pii);
    }
};

class small_onepole_filter_audio_module: public null_audio_module
{
public:    
    enum { in_signal, in_cutoff, in_count };
    enum { out_signal, out_count};
    float *ins[in_count]; 
    float *outs[out_count];
    dsp::onepole<float> filter;
    uint32_t srate;
    static parameter_properties param_props[];
    
    static void port_info(plugin_info_iface *pii)
    {
        pii->audio_port("In", "in").input();
        pii->control_port("Cutoff", "cutoff", 1000).input().log_range(20, 20000);
        pii->audio_port("Out", "out").output();
    }
    /// do not export mode and inertia as CVs, as those are settings and not parameters
    void activate() {
        filter.reset();
    }
};

class small_onepole_lp_filter_audio_module: public small_onepole_filter_audio_module
{
public:
    void process(uint32_t count) {
        filter.set_lp(clip<float>(*ins[in_cutoff], 0.0001, 0.48 * srate), srate);
        for (uint32_t i = 0; i < count; i++)
            outs[0][i] = filter.process_lp(ins[0][i]);
        filter.sanitize();
    }
    static void plugin_info(plugin_info_iface *pii)
    {
        pii->names("lowpass6", "lowpass6", "6dB/oct Lowpass Filter", "lv2:LowpassPlugin");
        port_info(pii);
    }
};

class small_onepole_hp_filter_audio_module: public small_onepole_filter_audio_module
{
public:
    void process(uint32_t count) {
        filter.set_hp(clip<float>(*ins[in_cutoff], 0.0001, 0.48 * srate), srate);
        for (uint32_t i = 0; i < count; i++)
            outs[0][i] = filter.process_hp(ins[0][i]);
        filter.sanitize();
    }
    static void plugin_info(plugin_info_iface *pii)
    {
        pii->names("highpass6", "highpass6", "6dB/oct Highpass Filter", "lv2:HighpassPlugin");
        port_info(pii);
    }
};

class small_onepole_ap_filter_audio_module: public small_onepole_filter_audio_module
{
public:
    void process(uint32_t count) {
        filter.set_ap(clip<float>(*ins[in_cutoff], 0.0001, 0.48 * srate), srate);
        for (uint32_t i = 0; i < count; i++)
            outs[0][i] = filter.process_ap(ins[0][i]);
        filter.sanitize();
    }
    static void plugin_info(plugin_info_iface *pii)
    {
        pii->names("allpass", "allpass", "1-pole 1-zero Allpass Filter", "lv2:AllpassPlugin");
        port_info(pii);
    }
};

/// This works for 1 or 2 operands only...
template<int Inputs>
class audio_operator_audio_module: public null_audio_module
{
public:    
    enum { in_count = Inputs, out_count = 1 };
    float *ins[in_count]; 
    float *outs[out_count];
    uint32_t srate;
    
    static void port_info(plugin_info_iface *pii)
    {
        if (Inputs == 1)
            pii->audio_port("in", "In").input();
        else
        {
            pii->audio_port("in_1", "In 1").input();
            pii->audio_port("in_2", "In 2").input();
        }
        pii->audio_port("out", "Out").output();
    }
    void set_sample_rate(uint32_t sr) {
        srate = sr;
    }
};

class small_min_audio_module: public audio_operator_audio_module<2>
{
public:
    void process(uint32_t count) {
        for (uint32_t i = 0; i < count; i++)
            outs[0][i] = std::min(ins[0][i], ins[1][i]);
    }
    static void plugin_info(plugin_info_iface *pii)
    {
        pii->names("min", "min", "Function min", "lv2:UtilityPlugin");
        port_info(pii);
    }
};

class small_max_audio_module: public audio_operator_audio_module<2>
{
public:
    void process(uint32_t count) {
        for (uint32_t i = 0; i < count; i++)
            outs[0][i] = std::max(ins[0][i], ins[1][i]);
    }
    static void plugin_info(plugin_info_iface *pii)
    {
        pii->names("max", "max", "Function max", "lv2:UtilityPlugin");
        port_info(pii);
    }
};

class small_minus_audio_module: public audio_operator_audio_module<2>
{
public:
    void process(uint32_t count) {
        for (uint32_t i = 0; i < count; i++)
            outs[0][i] = ins[0][i] - ins[1][i];
    }
    static void plugin_info(plugin_info_iface *pii)
    {
        pii->names("minus", "minus", "Subtract", "lv2:UtilityPlugin");
        port_info(pii);
    }
};

class small_mul_audio_module: public audio_operator_audio_module<2>
{
public:
    void process(uint32_t count) {
        for (uint32_t i = 0; i < count; i++)
            outs[0][i] = ins[0][i] * ins[1][i];
    }
    static void plugin_info(plugin_info_iface *pii)
    {
        pii->names("mul", "mul", "Multiply", "lv2:UtilityPlugin");
        port_info(pii);
    }
};

class small_neg_audio_module: public audio_operator_audio_module<1>
{
public:
    void process(uint32_t count) {
        for (uint32_t i = 0; i < count; i++)
            outs[0][i] = -ins[0][i];
    }
    static void plugin_info(plugin_info_iface *pii)
    {
        pii->names("neg", "neg", "Negative value", "lv2:UtilityPlugin");
        port_info(pii);
    }
};

class small_map_lin2exp_audio_module: public null_audio_module
{
public:    
    enum { in_signal, in_from_min, in_from_max, in_to_min, in_to_max, in_count };
    enum { out_signal, out_count };
    float *ins[in_count]; 
    float *outs[out_count];
    uint32_t srate;
    
    static void plugin_info(plugin_info_iface *pii)
    {
        pii->names("lin2exp", "lin2exp", "Lin-Exp Mapper", "lv2:UtilityPlugin");
        pii->control_port("in", "In", 0.f).input();
        pii->control_port("from_min", "Min (from)", 0).input();
        pii->control_port("from_max", "Max (from)", 1).input();
        pii->control_port("to_min", "Min (to)", 20).input();
        pii->control_port("to_max", "Max (to)", 20000).input();
        pii->control_port("out", "Out", 0.f).output();
    }
    void set_sample_rate(uint32_t sr) {
        srate = sr;
    }
    void process(uint32_t count) {
        float normalized = (*ins[in_signal] - *ins[in_from_min]) / (*ins[in_from_max] - *ins[in_from_min]);
        *outs[out_signal] = *ins[in_to_min] * pow(*ins[in_to_max] / *ins[in_to_min], normalized);
    }
};

};

#endif
