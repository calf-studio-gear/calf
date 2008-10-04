/* Calf DSP Library
 * Example audio modules - parameters and LADSPA wrapper instantiation
 *
 * Copyright (C) 2001-2008 Krzysztof Foltman
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
#include <config.h>
#include <assert.h>
#include <memory.h>
#include <calf/primitives.h>
#include <calf/biquad.h>
#include <calf/inertia.h>
#include <calf/audio_fx.h>
#include <calf/plugininfo.h>
#include <calf/giface.h>
#include <calf/lv2wrap.h>
#include <calf/osc.h>
#include <calf/modules_small.h>

#ifdef ENABLE_EXPERIMENTAL

#if USE_LV2
#define LV2_SMALL_WRAPPER(mod, name) static synth::lv2_small_wrapper<small_plugins::mod##_audio_module> lv2_small_##mod(name);
#else
#define LV2_SMALL_WRAPPER(...)
#endif

#define SMALL_WRAPPERS(mod, name) LV2_SMALL_WRAPPER(mod, name)

#if USE_LV2

using namespace synth;
using namespace dsp;
using namespace std;

template<class Module> LV2_Descriptor lv2_small_wrapper<Module>::descriptor;

namespace small_plugins
{

class filter_base: public null_small_audio_module
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
    
    void activate() {
        filter.reset_d1();
    }
    inline void process_inner(uint32_t count) {
        for (uint32_t i = 0; i < count; i++)
            outs[out_signal][i] = filter.process_d1(ins[in_signal][i]);
        filter.sanitize_d1();
    }
};

class lp_filter_audio_module: public filter_base
{
public:
    inline void process(uint32_t count) {
        filter.set_lp_rbj(clip<float>(*ins[in_cutoff], 0.0001, 0.48 * srate), *ins[in_resonance], srate);
        process_inner(count);
    }
    static void plugin_info(plugin_info_iface *pii)
    {
        pii->names("lowpass12", "12dB/oct RBJ Lowpass", "lv2:LowpassPlugin");
        port_info(pii);
    }
};

class hp_filter_audio_module: public filter_base
{
public:
    inline void process(uint32_t count) {
        filter.set_hp_rbj(clip<float>(*ins[in_cutoff], 0.0001, 0.48 * srate), *ins[in_resonance], srate);
        process_inner(count);
    }
    static void plugin_info(plugin_info_iface *pii)
    {
        pii->names("highpass12", "12dB/oct RBJ Highpass", "lv2:HighpassPlugin");
        port_info(pii);
    }
};

class bp_filter_audio_module: public filter_base
{
public:
    inline void process(uint32_t count) {
        filter.set_bp_rbj(clip<float>(*ins[in_cutoff], 0.0001, 0.48 * srate), *ins[in_resonance], srate);
        process_inner(count);
    }
    static void plugin_info(plugin_info_iface *pii)
    {
        pii->names("bandpass6", "6dB/oct RBJ Bandpass", "lv2:BandpassPlugin");
        port_info(pii);
    }
};

class br_filter_audio_module: public filter_base
{
public:
    inline void process(uint32_t count) {
        filter.set_br_rbj(clip<float>(*ins[in_cutoff], 0.0001, 0.48 * srate), *ins[in_resonance], srate);
        process_inner(count);
    }
    static void plugin_info(plugin_info_iface *pii)
    {
        pii->names("notch6", "6dB/oct RBJ Bandpass", "lv2:FilterPlugin");
        port_info(pii);
    }
};

class onepole_filter_base: public null_small_audio_module
{
public:    
    enum { in_signal, in_cutoff, in_count };
    enum { out_signal, out_count};
    float *ins[in_count]; 
    float *outs[out_count];
    dsp::onepole<float> filter;
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

class onepole_lp_filter_audio_module: public onepole_filter_base
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
        pii->names("lowpass6", "6dB/oct Lowpass Filter", "lv2:LowpassPlugin");
        port_info(pii);
    }
};

class onepole_hp_filter_audio_module: public onepole_filter_base
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
        pii->names("highpass6", "6dB/oct Highpass Filter", "lv2:HighpassPlugin");
        port_info(pii);
    }
};

class onepole_ap_filter_audio_module: public onepole_filter_base
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
        pii->names("allpass", "1-pole 1-zero Allpass Filter", "lv2:AllpassPlugin");
        port_info(pii);
    }
};

/// This works for 1 or 2 operands only...
template<int Inputs>
class audio_operator_audio_module: public small_audio_module_base<Inputs, 1>
{
public:    
    static void port_info(plugin_info_iface *pii)
    {
        if (Inputs == 1)
            pii->audio_port("in", "In", "").input();
        else
        {
            pii->audio_port("in_1", "In 1", "").input();
            pii->audio_port("in_2", "In 2", "").input();
        }
        pii->audio_port("out", "Out", "").output();
    }
};

class min_audio_module: public audio_operator_audio_module<2>
{
public:
    void process(uint32_t count) {
        for (uint32_t i = 0; i < count; i++)
            outs[0][i] = std::min(ins[0][i], ins[1][i]);
    }
    static void plugin_info(plugin_info_iface *pii)
    {
        pii->names("min", "Min (A)", "kf:MathOperatorPlugin", "min");
        port_info(pii);
    }
};

class max_audio_module: public audio_operator_audio_module<2>
{
public:
    void process(uint32_t count) {
        for (uint32_t i = 0; i < count; i++)
            outs[0][i] = std::max(ins[0][i], ins[1][i]);
    }
    static void plugin_info(plugin_info_iface *pii)
    {
        pii->names("max", "Max (A)", "kf:MathOperatorPlugin", "max");
        port_info(pii);
    }
};

class minus_audio_module: public audio_operator_audio_module<2>
{
public:
    void process(uint32_t count) {
        for (uint32_t i = 0; i < count; i++)
            outs[0][i] = ins[0][i] - ins[1][i];
    }
    static void plugin_info(plugin_info_iface *pii)
    {
        pii->names("minus", "Subtract (A)", "kf:MathOperatorPlugin", "-");
        port_info(pii);
    }
};

class mul_audio_module: public audio_operator_audio_module<2>
{
public:
    void process(uint32_t count) {
        for (uint32_t i = 0; i < count; i++)
            outs[0][i] = ins[0][i] * ins[1][i];
    }
    static void plugin_info(plugin_info_iface *pii)
    {
        pii->names("mul", "Multiply (A)", "kf:MathOperatorPlugin", "*");
        port_info(pii);
    }
};

class neg_audio_module: public audio_operator_audio_module<1>
{
public:
    void process(uint32_t count) {
        for (uint32_t i = 0; i < count; i++)
            outs[0][i] = -ins[0][i];
    }
    static void plugin_info(plugin_info_iface *pii)
    {
        pii->names("neg", "Negative (A)", "kf:MathOperatorPlugin", "-");
        port_info(pii);
    }
};

/// This works for 1 or 2 operands only...
template<int Inputs>
class control_operator_audio_module: public small_audio_module_base<Inputs, 1>
{
public:    
    static void port_info(plugin_info_iface *pii, control_port_info_iface *cports[Inputs + 1], float in1 = 0, float in2 = 0)
    {
        int idx = 0;
        if (Inputs == 1)
            cports[idx++] = &pii->control_port("in", "In", in1, "").input();
        else
        {
            cports[idx++] = &pii->control_port("in_1", "In 1", in1, "").input();
            cports[idx++] = &pii->control_port("in_2", "In 2", in2, "").input();
        }
        cports[idx++] = &pii->control_port("out", "Out", 0, "").output();
    }
};

class minus_c_audio_module: public control_operator_audio_module<2>
{
public:
    void process(uint32_t count) {
        outs[0][0] = ins[0][0] - ins[1][0];
    }
    static void plugin_info(plugin_info_iface *pii)
    {
        pii->names("minus_c", "Subtract (C)", "kf:MathOperatorPlugin", "-");
        control_port_info_iface *cports[3];
        port_info(pii, cports);
    }
};

class mul_c_audio_module: public control_operator_audio_module<2>
{
public:
    void process(uint32_t count) {
        outs[0][0] = ins[0][0] * ins[1][0];
    }
    static void plugin_info(plugin_info_iface *pii)
    {
        pii->names("mul_c", "Multiply (C)", "kf:MathOperatorPlugin", "*");
        control_port_info_iface *cports[3];
        port_info(pii, cports);
    }
};

class neg_c_audio_module: public control_operator_audio_module<1>
{
public:
    void process(uint32_t count) {
        *outs[0] = -*ins[0];
    }
    static void plugin_info(plugin_info_iface *pii)
    {
        pii->names("neg_c", "Negative (C)", "kf:MathOperatorPlugin", "-");
        control_port_info_iface *cports[2];
        port_info(pii, cports);
    }
};

class less_c_audio_module: public control_operator_audio_module<2>
{
public:
    void process(uint32_t count) {
        *outs[0] = *ins[0] < *ins[1];
    }
    static void plugin_info(plugin_info_iface *pii)
    {
        pii->names("less_c", "Less than (C)", "kf:MathOperatorPlugin", "<");
        control_port_info_iface *cports[2];
        port_info(pii, cports);
        cports[2]->toggle();
    }
};

class level2edge_c_audio_module: public control_operator_audio_module<1>
{
public:
    bool last_value;
    void activate() {
        last_value = false;
    }
    void process(uint32_t count) {
        *outs[0] = (*ins[0] > 0 && !last_value) ? 1.f : 0.f;
        last_value = *ins[0] > 0;
    }
    static void plugin_info(plugin_info_iface *pii)
    {
        pii->names("level2edge_c", "Level to edge (C)", "kf:BooleanPlugin");
        control_port_info_iface *cports[2];
        port_info(pii, cports);
        cports[0]->toggle();
        cports[1]->toggle().trigger();
    }
};

class flipflop_c_audio_module: public control_operator_audio_module<1>
{
public:
    bool last_value, output;
    void activate() {
        last_value = false;
        output = false;
    }
    void process(uint32_t count) {
        if (*ins[0] > 0 && !last_value)
            output = !output;
        *outs[0] = output ? 1.f : 0.f;
        last_value = *ins[0] > 0;
    }
    static void plugin_info(plugin_info_iface *pii)
    {
        pii->names("flipflop_c", "T Flip-Flop (C)", "kf:BooleanPlugin");
        control_port_info_iface *cports[2];
        port_info(pii, cports);
        cports[0]->toggle().trigger();
        cports[1]->toggle();
    }
};

class logical_and_c_audio_module: public control_operator_audio_module<2>
{
public:
    void process(uint32_t count) {
        *outs[0] = (*ins[0] > 0 && *ins[1] > 0) ? 1.f : 0.f;
    }
    static void plugin_info(plugin_info_iface *pii)
    {
        pii->names("logical_and_c", "Logical AND (C)", "kf:BooleanPlugin", "&&");
        control_port_info_iface *cports[3];
        port_info(pii, cports);
        cports[0]->toggle();
        cports[1]->toggle();
        cports[2]->toggle();
    }
};

class logical_or_c_audio_module: public control_operator_audio_module<2>
{
public:
    void process(uint32_t count) {
        *outs[0] = (*ins[0] > 0 || *ins[1] > 0) ? 1.f : 0.f;
    }
    static void plugin_info(plugin_info_iface *pii)
    {
        pii->names("logical_or_c", "Logical OR (C)", "kf:BooleanPlugin", "||");
        control_port_info_iface *cports[3];
        port_info(pii, cports);
        cports[0]->toggle();
        cports[1]->toggle();
        cports[2]->toggle();
    }
};

class logical_xor_c_audio_module: public control_operator_audio_module<2>
{
public:
    void process(uint32_t count) {
        *outs[0] = ((*ins[0] > 0) != (*ins[1] > 0)) ? 1.f : 0.f;
    }
    static void plugin_info(plugin_info_iface *pii)
    {
        pii->names("logical_xor_c", "Logical XOR (C)", "kf:BooleanPlugin", "xor");
        control_port_info_iface *cports[3];
        port_info(pii, cports);
        cports[0]->toggle();
        cports[1]->toggle();
        cports[2]->toggle();
    }
};

class logical_not_c_audio_module: public control_operator_audio_module<1>
{
public:
    void process(uint32_t count) {
        *outs[0] = (*ins[0] > 0) ? 0.f : 1.f;
    }
    static void plugin_info(plugin_info_iface *pii)
    {
        pii->names("logical_not_c", "Logical NOT (C)", "kf:BooleanPlugin", "!");
        control_port_info_iface *cports[2];
        port_info(pii, cports);
        cports[0]->toggle();
        cports[1]->toggle();
    }
};

/// Monostable multivibrator like 74121 or 74123, with reset input, progress output (0 to 1), "finished" signal, configurable to allow or forbid retriggering.
class timer_c_audio_module: public null_small_audio_module
{
public:    
    enum { in_trigger, in_time, in_reset, in_allow_retrig, in_count };
    enum { out_running, out_finished, out_progress, out_count };
    float *ins[in_count]; 
    float *outs[out_count];
    bool running, finished, old_trigger;
    double state;
    
    void activate()
    {
        state = 0.f;
        running = false;
        finished = false;
        old_trigger = false;
    }
    static void plugin_info(plugin_info_iface *pii)
    {
        pii->names("timer_c", "Timer (C)", "lv2:UtilityPlugin");
        pii->control_port("trigger", "Trigger", 0.f).input().toggle().trigger();
        pii->control_port("time", "Time", 0.f).input();
        pii->control_port("reset", "Reset", 0).input().toggle();
        pii->control_port("allow_retrig", "Allow retrig", 0).input().toggle();
        pii->control_port("running", "Running", 0.f).output().toggle();
        pii->control_port("finished", "Finished", 0.f).output().toggle();
        pii->control_port("progress", "Progress", 0.f).output().lin_range(0, 1);
    }
    void process(uint32_t count)
    {
        // This is a branch city, which is definitely a bad thing.
        // Perhaps I'll add a bunch of __builtin_expect hints some day, but somebody would have to start using it first.
        if (*ins[in_reset] > 0)
        {
            state = 0.0;
            running = finished = false;
        }
        else
        if (!old_trigger && *ins[in_trigger] > 0 && (!running || *ins[in_allow_retrig] > 0))
        {
            state = 0.0;
            running = true;
            finished = false;
        }
        else
        if (running)
        {
            float rate = (1.0 / std::max(0.0000001f, *ins[in_time]));
            state += rate * odsr * count;
            if (state >= 1.0)
            {
                running = false;
                finished = true;
                state = 1.0;
            }
        }
        old_trigger = *ins[in_trigger] > 0;
        *outs[out_running] = running ? 1.f : 0.f;
        *outs[out_finished] = finished ? 1.f : 0.f;
        *outs[out_progress] = state;
    }
};

/// 4-input priority multiplexer - without inertia. Outputs the first input if gate_1 is TRUE, else second input if gate_2 is TRUE, else... else "Else" input
class prio_mux_c_audio_module: public null_small_audio_module
{
public:    
    enum { in_in1, in_gate1, in_in2, in_gate2, in_in3, in_gate3, in_in4, in_gate4, in_else, in_count };
    enum { out_value, out_count };
    float *ins[in_count]; 
    float *outs[out_count];
    
    static void plugin_info(plugin_info_iface *pii)
    {
        pii->names("prio_mux_c", "Priority Multiplexer (C)", "kf:BooleanPlugin");
        for (int i = 1; i <= 4; i++)
        {
            string num = string("1234" + i - 1, 1);
            pii->control_port("in_"+num, "In "+num, 0.f).input();
            pii->control_port("gate_"+num, "Gate "+num, 0.f).input().toggle();
        }
        pii->control_port("in_else", "Else", 0.f).input();
        pii->control_port("out", "Out", 0.f).output();
    }
    void process(uint32_t count)
    {
        for (int i = 0; i < 4; i++)
        {
            if (*ins[i * 2 + in_gate1] > 0)
            {
                *outs[out_value] = *ins[i * 2 + in_in1];
                return;
            }
        }
        *outs[out_value] = *ins[in_else];
    }
};

/// Linear-to-exponential mapper
class map_lin2exp_audio_module: public null_small_audio_module
{
public:    
    enum { in_signal, in_from_min, in_from_max, in_to_min, in_to_max, in_count };
    enum { out_signal, out_count };
    float *ins[in_count]; 
    float *outs[out_count];
    
    static void plugin_info(plugin_info_iface *pii)
    {
        pii->names("lin2exp", "Lin-Exp Mapper", "lv2:UtilityPlugin");
        pii->control_port("in", "In", 0.f).input();
        pii->control_port("from_min", "Min (from)", 0).input();
        pii->control_port("from_max", "Max (from)", 1).input();
        pii->control_port("to_min", "Min (to)", 20).input();
        pii->control_port("to_max", "Max (to)", 20000).input();
        pii->control_port("out", "Out", 0.f).output();
    }
    void process(uint32_t count)
    {
        float normalized = (*ins[in_signal] - *ins[in_from_min]) / (*ins[in_from_max] - *ins[in_from_min]);
        *outs[out_signal] = *ins[in_to_min] * pow(*ins[in_to_max] / *ins[in_to_min], normalized);
    }
};

/// Schmitt trigger - http://en.wikipedia.org/wiki/Schmitt_trigger - also outputs a change signal (positive spike whenever output value is changed)
class schmitt_c_audio_module: public null_small_audio_module
{
public:    
    enum { in_signal, in_low, in_high, in_count };
    enum { out_signal, out_change, out_count };
    float *ins[in_count]; 
    float *outs[out_count];
    bool state;
    
    void activate()
    {
        state = false;
    }
    static void plugin_info(plugin_info_iface *pii)
    {
        pii->names("schmitt_c", "Schmitt Trigger (C)", "kf:BooleanPlugin");
        pii->control_port("in", "In", 0.f).input();
        pii->control_port("low", "Low threshold", 0).input();
        pii->control_port("high", "High threshold", 0.5).input();
        pii->control_port("out", "Out", 0.f).output();
        pii->control_port("change", "Change", 0.f).output();
    }
    void process(uint32_t count)
    {
        float value = *ins[in_signal];
        bool new_state = state;
        if (value <= *ins[in_low])
            new_state = false;
        if (value >= *ins[in_high])
            new_state = true;
        *outs[out_signal] = new_state ? 1.f : 0.f;
        *outs[out_change] = (new_state != state) ? 1.f : 0.f;
        state = new_state;
    }
};

/// Stateless 'between' operator (lo <= in <= hi)
class between_c_audio_module: public null_small_audio_module
{
public:    
    enum { in_signal, in_low, in_high, in_count };
    enum { out_signal, out_count };
    float *ins[in_count]; 
    float *outs[out_count];
    
    static void plugin_info(plugin_info_iface *pii)
    {
        pii->names("between_c", "Between (C)", "kf:MathOperatorPlugin");
        pii->control_port("in", "In", 0.f).input();
        pii->control_port("low", "Low threshold", 0).input();
        pii->control_port("high", "High threshold", 1).input();
        pii->control_port("out", "Out", 0.f).output();
    }
    void process(uint32_t count)
    {
        float value = *ins[in_signal];
        *outs[out_signal] = (value >= *ins[in_low] && value <= *ins[in_high]) ? 1.f : 0.f;
    }
};

/// Clip to range
class clip_c_audio_module: public null_small_audio_module
{
public:    
    enum { in_signal, in_min, in_max, in_count };
    enum { out_signal, out_count };
    float *ins[in_count]; 
    float *outs[out_count];
    
    static void plugin_info(plugin_info_iface *pii)
    {
        pii->names("clip_c", "Clip (C)", "kf:MathOperatorPlugin", "clip");
        pii->control_port("in", "In", 0.f).input();
        pii->control_port("min", "Min", 0).input();
        pii->control_port("max", "Max", 1).input();
        pii->control_port("out", "Out", 0.f).output();
    }
    void process(uint32_t count)
    {
        float value = *ins[in_signal];
        *outs[out_signal] = std::min(*ins[in_max], std::max(value, *ins[in_min]));
    }
};

/// Two input control crossfader
class crossfader2_c_audio_module: public null_small_audio_module
{
public:    
    enum { in_a, in_b, in_ctl, in_count };
    enum { out_value, out_count };
    float *ins[in_count]; 
    float *outs[out_count];
    
    static void plugin_info(plugin_info_iface *pii)
    {
        pii->names("crossfader2_c", "2-in crossfader (C)", "kf:MathOperatorPlugin", "xfC");
        pii->control_port("in_a", "In A", 0.f).input();
        pii->control_port("in_b", "In B", 0).input();
        pii->control_port("mix", "B in mix", 0.5).input();
        pii->control_port("out", "Out", 0.f).output();
    }
    void process(uint32_t count)
    {
        *outs[out_value] = *ins[in_a] + (*ins[in_b] - *ins[in_a]) * dsp::clip(*ins[in_ctl], 0.f, 1.f);
    }
};

/// Two input audio crossfader
class crossfader2_a_audio_module: public null_small_audio_module
{
public:    
    enum { in_a, in_b, in_ctl, in_count };
    enum { out_value, out_count };
    float *ins[in_count]; 
    float *outs[out_count];
    
    static void plugin_info(plugin_info_iface *pii)
    {
        pii->names("crossfader2_a", "2-in crossfader (A)", "kf:MathOperatorPlugin", "xfA");
        pii->audio_port("in_a", "In A").input();
        pii->audio_port("in_b", "In B").input();
        pii->audio_port("mix", "B in mix").input();
        pii->audio_port("out", "Out").output();
    }
    void process(uint32_t count)
    {
        for (uint32_t i = 0; i < count; i++)
            outs[out_value][i] = ins[in_a][i] + (ins[in_b][i] - ins[in_a][i]) * dsp::clip(ins[in_ctl][i], 0.f, 1.f);
    }
};

/// Base class for LFOs with frequency and reset inputs
class freq_phase_lfo_base: public small_audio_module_base<2, 1>
{
public:
    enum { in_freq, in_reset };
    double phase;
    inline void activate()
    {
        phase = 0;
    }
    static void port_info(plugin_info_iface *pii)
    {
        pii->control_port("freq", "Frequency", 1).input().log_range(0.02, 100);
        pii->control_port("reset", "Reset", 0).input().toggle();
        pii->control_port("out", "Out", 0).output();
    }
    inline void check_inputs()
    {
        if (*ins[in_reset])
            phase = 0;
    }
    inline void advance(uint32_t count)
    {
        phase += count * *ins[in_freq] * odsr;
        if (phase >= 1.0)
            phase = fmod(phase, 1.0);
    }
};

class square_lfo_audio_module: public freq_phase_lfo_base
{
public:
    void process(uint32_t count)
    {
        check_inputs();
        *outs[0] = (phase < 0.5) ? -1 : +1;
        advance(count);
    }
    static void plugin_info(plugin_info_iface *pii)
    {
        pii->names("square_lfo", "Square LFO", "lv2:OscillatorPlugin");
        port_info(pii);
    }
};

class saw_lfo_audio_module: public freq_phase_lfo_base
{
public:
    void process(uint32_t count)
    {
        check_inputs();
        *outs[0] = -1 + 2 * phase;
        advance(count);
    }
    static void plugin_info(plugin_info_iface *pii)
    {
        pii->names("saw_lfo", "Saw LFO", "lv2:OscillatorPlugin");
        port_info(pii);
    }
};

class pulse_lfo_audio_module: public freq_phase_lfo_base
{
public:
    inline void activate()
    {
        phase = 1.0;
    }
    void process(uint32_t count)
    {
        check_inputs();
        double oldphase = phase;
        advance(count);
        *outs[0] = (phase < oldphase) ? 1.f : 0.f;
    }
    static void plugin_info(plugin_info_iface *pii)
    {
        pii->names("pulse_lfo", "Pulse LFO", "lv2:OscillatorPlugin");
        port_info(pii);
    }
};

#define SMALL_OSC_TABLE_BITS 12

class freq_only_osc_base: public null_small_audio_module
{
public:    
    typedef waveform_family<SMALL_OSC_TABLE_BITS> waves_type;
    enum { in_freq, in_count };
    enum { out_signal, out_count};
    enum { wave_size = 1 << SMALL_OSC_TABLE_BITS };
    float *ins[in_count]; 
    float *outs[out_count];
    waves_type *waves;
    waveform_oscillator<SMALL_OSC_TABLE_BITS> osc;

    /// Fill the array with the original, non-bandlimited, waveform
    virtual void get_original_waveform(float data[wave_size]) = 0;
    /// This function needs to be virtual to ensure a separate wave family for each class (but not each instance)
    virtual waves_type *get_waves() = 0;
    void activate() {
        waves = get_waves();
    }
    void process(uint32_t count)
    {
        osc.set_freq_odsr(*ins[in_freq], odsr);
        osc.waveform = waves->get_level(osc.phasedelta);
        if (osc.waveform)
        {
            for (uint32_t i = 0; i < count; i++)
                outs[out_signal][i] = osc.get();
        }
        else
            dsp::zero(outs[out_signal], count);
    }
    static void port_info(plugin_info_iface *pii)
    {
        pii->control_port("freq", "Frequency", 440).input().log_range(20, 20000);
        pii->audio_port("out", "Out").output();
    }
    /// Generate a wave family (bandlimit levels) from the original wave
    waves_type *create_waves() {
        waves_type *ptr = new waves_type;
        float source[wave_size];
        get_original_waveform(source);
        bandlimiter<SMALL_OSC_TABLE_BITS> bl;
        ptr->make(bl, source);
        return ptr;
    }
};

#define OSC_MODULE_GET_WAVES() \
    virtual waves_type *get_waves() { \
        static waves_type *waves = NULL; \
        if (!waves) \
            waves = create_waves(); \
        return waves; \
    }

class square_osc_audio_module: public freq_only_osc_base
{
public:
    OSC_MODULE_GET_WAVES()

    virtual void get_original_waveform(float data[wave_size]) {
        for (int i = 0; i < wave_size; i++)
            data[i] = (i < wave_size / 2) ? +1 : -1;
    }
    static void plugin_info(plugin_info_iface *pii)
    {
        pii->names("squareosc", "Square Oscillator", "lv2:OscillatorPlugin");
        port_info(pii);
    }
};

class saw_osc_audio_module: public freq_only_osc_base
{
public:
    OSC_MODULE_GET_WAVES()

    virtual void get_original_waveform(float data[wave_size]) {
        for (int i = 0; i < wave_size; i++)
            data[i] = (i * 2.0 / wave_size) - 1;
    }
    static void plugin_info(plugin_info_iface *pii)
    {
        pii->names("sawosc", "Saw Oscillator", "lv2:OscillatorPlugin");
        port_info(pii);
    }
};

class print_c_audio_module: public small_audio_module_base<1, 0>
{
public:    
    static void plugin_info(plugin_info_iface *pii)
    {
        pii->names("print_c", "Print To Console (C)", "lv2:UtilityPlugin");
        pii->control_port("in", "In", 0).input();
    }
    void process(uint32_t)
    {
        printf("%f\n", *ins[0]);
    }
};

class print_e_audio_module: public small_audio_module_base<1, 0>
{
public:    
    static void plugin_info(plugin_info_iface *pii)
    {
        pii->names("print_e", "Print To Console (E)", "lv2:UtilityPlugin");
        pii->event_port("in", "In").input();
    }
    void dump(LV2_Event_Buffer *event_data)
    {
        uint8_t *data = event_data->data;
        for(uint32_t i = 0; i < event_data->event_count; i++)
        {
            LV2_Event *event = (LV2_Event *)data;
            printf("Event at %d.%d, type %d, size %d:", event->frames, event->subframes, (int)event->type, (int)event->size);
            uint32_t size = std::min((uint16_t)16, event->size);
            
            for (uint32_t j = 0; j < size; j++)
                printf(" %02X", (uint32_t)data[12 + j]);
            if (event->size > size)
                printf("...\n");
            else
                printf("\n");
            data += (event->size + 19) &~ 7;
        }
    }
    void process(uint32_t)
    {
        LV2_Event_Buffer *event_data = (LV2_Event_Buffer *)ins[0];
        dump(event_data);
    }
};

class print_em_audio_module: public print_e_audio_module
{
public:    
    LV2_Event_Buffer *events;
    static void plugin_info(plugin_info_iface *pii)
    {
        pii->names("print_em", "Print To Console (EM)", "lv2:UtilityPlugin");
        pii->lv2_ttl("lv2:requiredFeature <http://lv2plug.in/ns/dev/contexts> ;");
        pii->lv2_ttl("lv2:requiredFeature lv2ctx:MessageContext ;");
        pii->lv2_ttl("lv2:requiredContext lv2ctx:MessageContext ;");
        pii->event_port("in", "In").input().lv2_ttl("lv2ctx:context lv2ctx:MessageContext ;");
    }
    void process(uint32_t)
    {
    }
    static bool message_run(LV2_Handle instance, uint32_t *outputs_written)
    {
        print_em_audio_module *self =  (print_em_audio_module *)instance;
        printf("message_run (events = %p)\n", self->events);
        self->dump(self->events);
        *outputs_written = 0;
        return false;
    }
    static void message_connect_port(LV2_Handle instance, uint32_t port, void* data)
    {
        print_em_audio_module *self =  (print_em_audio_module *)instance;
        printf("message_connect_port %d -> %p\n", port, data);
        assert(!port);
        self->events = (LV2_Event_Buffer *)data;
    }
    static inline const void *ext_data(const char *URI) { 
        static LV2MessageContext ctx_ext_data = { message_run, message_connect_port };
        printf("URI=%s\n", URI);
        if (!strcmp(URI, LV2_CONTEXT_MESSAGE))
        {
            return &ctx_ext_data;
        }
        return NULL;
    }
};

class copy_em_audio_module: public small_audio_module_base<0, 0>
{
public:    
    LV2_Event_Buffer *events_in, *events_out;
    static void plugin_info(plugin_info_iface *pii)
    {
        pii->names("copy_em", "Message pass-through (EM)", "lv2:UtilityPlugin");
        pii->lv2_ttl("lv2:requiredFeature lv2ctx:MessageContext ;");
        pii->lv2_ttl("lv2:requiredFeature <http://lv2plug.in/ns/dev/contexts> ;");
        pii->lv2_ttl("lv2:requiredContext lv2ctx:MessageContext ;");
        pii->event_port("in", "In").input().lv2_ttl("lv2ctx:context lv2ctx:MessageContext ;");
        pii->event_port("out", "Out").output().lv2_ttl("lv2ctx:context lv2ctx:MessageContext ;");
    }
    void process(uint32_t)
    {
    }
    static bool message_run(LV2_Handle instance, uint32_t *outputs_written)
    {
        copy_em_audio_module *self =  (copy_em_audio_module *)instance;
        return self->message_run(outputs_written);
    }
    bool message_run(uint32_t *outputs_written)
    {
        if (events_in->size > events_in->capacity)
        {
            printf("Buffer capacity exceeded!\n");
            return false;
        }
        events_out->event_count = events_in->event_count;
        events_out->size = events_in->size;
        memcpy(events_out->data, events_in->data, events_in->size);
        *outputs_written = (events_in->event_count != 0) ? 2 : 0;
        return true;
    }
    static void message_connect_port(LV2_Handle instance, uint32_t port, void* data)
    {
        copy_em_audio_module *self =  (copy_em_audio_module *)instance;
        printf("message_connect_port %d -> %p\n", port, data);
        if (port == 0) self->events_in = (LV2_Event_Buffer *)data;
        if (port == 1) self->events_out = (LV2_Event_Buffer *)data;
    }
    static inline const void *ext_data(const char *URI) { 
        static LV2MessageContext ctx_ext_data = { message_run, message_connect_port };
        if (!strcmp(URI, LV2_CONTEXT_MESSAGE))
        {
            printf("URI=%s\n", URI);
            return &ctx_ext_data;
        }
        return NULL;
    }
};

class print_a_audio_module: public small_audio_module_base<1, 0>
{
public:    
    static void plugin_info(plugin_info_iface *pii)
    {
        pii->names("print_a", "Print To Console (A)", "lv2:UtilityPlugin");
        pii->audio_port("in", "In").input();
    }
    void process(uint32_t)
    {
        printf("%f\n", *ins[0]);
    }
};

template<bool audio>
class quadpower_base: public null_small_audio_module
{
public:    
    enum { in_value, in_factor, in_count , out_count = 4 };
    float *ins[in_count]; 
    float *outs[out_count];
    
    static void plugin_info(plugin_info_iface *pii)
    {
        const char *names[8] = {"xa", "x*a^1", "xaa", "x*a^2", "xaaa", "x*a^3", "xaaaa", "x*a^4" };
        if (audio)
            pii->names("quadpower_a", "Quad Power (A)", "kf:MathOperatorPlugin");
        else
            pii->names("quadpower_c", "Quad Power (C)", "kf:MathOperatorPlugin");
        if (audio)
            pii->audio_port("x", "x").input();
        else
            pii->control_port("x", "x", 1).input();
        pii->control_port("a", "a", 1).input();
        for (int i = 0; i < 8; i += 2)
            if (audio)
                pii->audio_port(names[i], names[i+1]).output();
            else
                pii->control_port(names[i], names[i+1], 0).output();
    }
};

class quadpower_a_audio_module: public quadpower_base<true>
{
public:
    void process(uint32_t count)
    {
        float a = *ins[in_factor];
        for (uint32_t i = 0; i < count; i++)
        {
            float x = ins[in_value][i];
            outs[0][i] = x * a;
            outs[1][i] = x * a * a;
            outs[2][i] = x * a * a * a;
            outs[3][i] = x * a * a * a * a;
        }
    }
};

class quadpower_c_audio_module: public quadpower_base<false>
{
public:
    void process(uint32_t)
    {
        float x = *ins[in_value];
        float a = *ins[in_factor];
        *outs[0] = x * a;
        *outs[1] = x * a * a;
        *outs[2] = x * a * a * a;
        *outs[3] = x * a * a * a * a;
    }
};

template<class Ramp>
class inertia_c_module_base: public small_audio_module_base<3, 1>
{
public:
    enum { in_value, in_inertia, in_immediate };
    bool reset;
    inertia<Ramp> state;
    inertia_c_module_base()
    : state(Ramp(1))
    {}
    void activate()
    {
        reset = true;
    }
    static void port_info(plugin_info_iface *pii)
    {
        pii->control_port("in", "In", 0).input();
        pii->control_port("time", "Inertia time", 100).input();
        pii->control_port("reset", "Reset", 0).input().toggle().trigger();
        pii->control_port("out", "Out", 0).output();
    }
    void process(uint32_t count)
    {
        float value = *ins[in_value];
        if (reset || *ins[in_immediate] > 0)
        {
            *outs[0] = value;
            state.set_now(value);
            reset = false;
        }
        else
        {
            if (value != state.get_last())
            {
                state.ramp.set_length(dsp::clip((int)(srate * 0.001 * *ins[in_inertia]), 1, 10000000));
            }
            state.set_inertia(value);
            *outs[0] = state.get_last();
            if (count)
                state.step_many(count);
        }
    }
};

class linear_inertia_c_audio_module: public inertia_c_module_base<linear_ramp>
{
public:
    static void plugin_info(plugin_info_iface *pii)
    {
        pii->names("linear_inertia_c", "Linear Inertia (C)", "lv2:FilterPlugin");
        port_info(pii);
    }
};

class exp_inertia_c_audio_module: public inertia_c_module_base<exponential_ramp>
{
public:
    static void plugin_info(plugin_info_iface *pii)
    {
        pii->names("exp_inertia_c", "Exponential Inertia (C)", "lv2:FilterPlugin");
        port_info(pii);
    }
};

class sample_hold_base: public small_audio_module_base<2, 1>
{
public:
    enum { in_value, in_gate };
    static void port_info(plugin_info_iface *pii, const char *clock_symbol, const char *clock_name)
    {
        pii->control_port("in", "In", 0).input();
        pii->control_port(clock_symbol, clock_name, 0).input().toggle().trigger();
        pii->control_port("out", "Out", 0).output();
    }
};

class sample_hold_edge_c_audio_module: public sample_hold_base
{
public:
    bool prev_clock;
    float value;
    void activate()
    {
        prev_clock = false;
        value = 0;
    }
    void process(uint32_t count)
    {
        if (!prev_clock && *ins[in_gate] > 0)
            value = *ins[in_value];
        prev_clock = *ins[in_gate] > 0;
        *outs[0] = value;
    }
    static void plugin_info(plugin_info_iface *pii)
    {
        pii->names("sample_hold_edge", "Sample&Hold (Edge, C)", "lv2:UtilityPlugin");
        port_info(pii, "clock", "Clock");
    }
};

class sample_hold_level_c_audio_module: public sample_hold_base
{
public:
    float value;
    void activate()
    {
        value = 0;
    }
    void process(uint32_t count)
    {
        if (*ins[in_gate] > 0)
            value = *ins[in_value];
        *outs[0] = value;
    }
    static void plugin_info(plugin_info_iface *pii)
    {
        pii->names("sample_hold_level", "Sample&Hold (Level, C)", "lv2:UtilityPlugin");
        port_info(pii, "gate", "Gate");
    }
};

};

#define PER_MODULE_ITEM(...) 
#define PER_SMALL_MODULE_ITEM(name, id) SMALL_WRAPPERS(name, id)
#include <calf/modulelist.h>

const LV2_Descriptor *synth::lv2_small_descriptor(uint32_t index)
{
    #define PER_MODULE_ITEM(...) 
    #define PER_SMALL_MODULE_ITEM(name, id) if (!(index--)) return &::lv2_small_##name.descriptor;
    #include <calf/modulelist.h>
    return NULL;
}
#endif
#endif

void synth::get_all_small_plugins(plugin_list_info_iface *iface)
{
    #define PER_MODULE_ITEM(name, isSynth, jackname) 
    #define PER_SMALL_MODULE_ITEM(name, id) { plugin_info_iface *pii = &iface->plugin(id); small_plugins::name##_audio_module::plugin_info(pii); pii->finalize(); }
    #include <calf/modulelist.h>
}

