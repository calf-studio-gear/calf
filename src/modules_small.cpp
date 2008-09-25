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
            pii->audio_port("in", "In").input();
        else
        {
            pii->audio_port("in_1", "In 1").input();
            pii->audio_port("in_2", "In 2").input();
        }
        pii->audio_port("out", "Out").output();
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
        pii->names("min", "Function min (A)", "lv2:UtilityPlugin");
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
        pii->names("max", "Function max (A)", "lv2:UtilityPlugin");
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
        pii->names("minus", "Subtract (A)", "lv2:UtilityPlugin");
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
        pii->names("mul", "Multiply (A)", "lv2:UtilityPlugin");
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
        pii->names("neg", "Negative value (A)", "lv2:UtilityPlugin");
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
            cports[idx++] = &pii->control_port("in", "In", in1).input();
        else
        {
            cports[idx++] = &pii->control_port("in_1", "In 1", in1).input();
            cports[idx++] = &pii->control_port("in_2", "In 2", in2).input();
        }
        cports[idx++] = &pii->control_port("out", "Out", 0).output();
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
        pii->names("level2edge_c", "Level to edge (C)", "lv2:UtilityPlugin");
        control_port_info_iface *cports[2];
        port_info(pii, cports);
        cports[0]->toggle().trigger();
    }
};

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
    void process(uint32_t count) {
        float normalized = (*ins[in_signal] - *ins[in_from_min]) / (*ins[in_from_max] - *ins[in_from_min]);
        *outs[out_signal] = *ins[in_to_min] * pow(*ins[in_to_max] / *ins[in_to_min], normalized);
    }
};

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

class print_audio_module: public small_audio_module_base<1, 0>
{
public:    
    static void plugin_info(plugin_info_iface *pii)
    {
        pii->names("print", "Print To Console (C)", "lv2:UtilityPlugin");
        pii->control_port("in", "In", 0).input();
    }
    void process(uint32_t)
    {
        printf("%f\n", *ins[0]);
    }
};

class print2_audio_module: public small_audio_module_base<1, 0>
{
public:    
    static void plugin_info(plugin_info_iface *pii)
    {
        pii->names("print2", "Print To Console (A)", "lv2:UtilityPlugin");
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
            pii->names("quadpower_a", "Quad Power (A)", "lv2:UtilityPlugin");
        else
            pii->names("quadpower_c", "Quad Power (C)", "lv2:UtilityPlugin");
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
        pii->names("sample_hold_edge", "Sample&Hold (Edge, C)", "lv2:FilterPlugin");
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
        pii->names("sample_hold_level", "Sample&Hold (Level, C)", "lv2:FilterPlugin");
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

