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
#if USE_JACK
#include <jack/jack.h>
#endif
#include <calf/giface.h>
#include <calf/lv2wrap.h>
#include <calf/modules.h>
#include <calf/modules_dev.h>

using namespace synth;

#if USE_LADSPA
#define LADSPA_WRAPPER(mod) static synth::ladspa_wrapper<mod##_audio_module> ladspa_##mod(mod##_info);
#else
#define LADSPA_WRAPPER(mod)
#endif

#if USE_LV2
#define LV2_WRAPPER(mod) static synth::lv2_wrapper<mod##_audio_module> lv2_##mod(mod##_info);
#else
#define LV2_WRAPPER(mod)
#endif

#define ALL_WRAPPERS(mod) LADSPA_WRAPPER(mod) LV2_WRAPPER(mod)

const char *copyright = "(C) 2001-2007 Krzysztof Foltman, license: LGPL";

////////////////////////////////////////////////////////////////////////////

const char *flanger_audio_module::port_names[] = {"In L", "In R", "Out L", "Out R"};

parameter_properties flanger_audio_module::param_props[] = {
    { 0.1,      0.1, 10,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "min_delay", "Minimum delay" },
    { 0.5,      0.1, 10,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "mod_depth", "Modulation depth" },
    { 0.25,    0.01, 20,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL, "mod_rate", "Modulation rate" },
    { 0.90,   -0.99, 0.99,  0, PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB | PF_UNIT_COEF, NULL, "feedback", "Feedback" },
    { 0,          0, 360,  10, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_DEG, NULL, "stereo", "Stereo phase" },
    { 0,          0, 1,     2, PF_BOOL | PF_CTL_BUTTON , NULL, "reset", "Reset" },
    { 1,          0, 2,     0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_COEF, NULL, "amount", "Amount" },
};

static synth::ladspa_info flanger_info = { 0x847d, "Flanger", "Calf Flanger", "Krzysztof Foltman", copyright, "FlangerPlugin" };

ALL_WRAPPERS(flanger)

////////////////////////////////////////////////////////////////////////////

const char *phaser_audio_module::port_names[] = {"In L", "In R", "Out L", "Out R"};

parameter_properties phaser_audio_module::param_props[] = {
    { 1000,      20, 20000, 0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL, "base_freq", "Center Freq" },
    { 4000,       0, 10800,  0, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_CENTS, NULL, "mod_depth", "Modulation depth" },
    { 0.25,    0.01, 20,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL, "mod_rate", "Modulation rate" },
    { 0.25,   -0.99, 0.99,  0, PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB | PF_UNIT_COEF, NULL, "feedback", "Feedback" },
    { 6,          1, 12,   12, PF_INT | PF_SCALE_LINEAR | PF_CTL_KNOB, NULL, "stages", "# Stages" },
    { 180,        0, 360,  10, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_DEG, NULL, "stereo", "Stereo phase" },
    { 0,          0, 1,     2, PF_BOOL | PF_CTL_BUTTON , NULL, "reset", "Reset" },
    { 1,          0, 2,     0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_COEF, NULL, "amount", "Amount" },
};

static synth::ladspa_info phaser_info = { 0x847d, "Phaser", "Calf Phaser", "Krzysztof Foltman", copyright, "PhaserPlugin" };

ALL_WRAPPERS(phaser)

////////////////////////////////////////////////////////////////////////////

const char *reverb_audio_module::port_names[] = {"In L", "In R", "Out L", "Out R"};

const char *reverb_room_sizes[] = { "Small", "Medium", "Large", "Tunnel-like" };

parameter_properties reverb_audio_module::param_props[] = {
    { 1.5,      0.5, 15.0,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_SEC, NULL, "decay_time", "Decay time" },
    { 5000,    2000,20000,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL, "hf_damp", "High Frq Damp" },
    { 2,          0,    3,    0, PF_ENUM | PF_CTL_COMBO , reverb_room_sizes, "room_size", "Room size", },
    { 0.5,        0,    1,    0, PF_FLOAT | PF_CTL_KNOB | PF_SCALE_PERC, NULL, "diffusion", "Diffusion" },
    { 0.25,       0,    2,    0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_COEF, NULL, "amount", "Amount" },
};

static synth::ladspa_info reverb_info = { 0x847e, "Reverb", "Calf Reverb", "Krzysztof Foltman", copyright, "ReverbPlugin" };

ALL_WRAPPERS(reverb)

////////////////////////////////////////////////////////////////////////////

const char *filter_audio_module::port_names[] = {"In L", "In R", "Out L", "Out R"};

const char *filter_choices[] = {
    "12dB/oct Lowpass",
    "24dB/oct Lowpass",
    "36dB/oct Lowpass",
    "12dB/oct Highpass",
    "24dB/oct Highpass",
    "36dB/oct Highpass",
};

parameter_properties filter_audio_module::param_props[] = {
    { 2000,      10,20000,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL, "freq", "Frequency" },
    { 0.707,  0.707,   20,    0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_COEF, NULL, "res", "Resonance" },
    { 0,          0,    5,    1, PF_ENUM | PF_CTL_COMBO, filter_choices, "mode", "Mode" },
    { 20,         5,  100,    20, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "inertia", "Inertia"},
};

static synth::ladspa_info filter_info = { 0x847f, "Filter", "Calf Filter", "Krzysztof Foltman", copyright, "FilterPlugin" };

ALL_WRAPPERS(filter)

////////////////////////////////////////////////////////////////////////////

const char *vintage_delay_audio_module::port_names[] = {"In L", "In R", "Out L", "Out R"};

const char *vintage_delay_mixmodes[] = {
    "Stereo",
    "Ping-Pong",
};

const char *vintage_delay_fbmodes[] = {
    "Plain",
    "Tape",
    "Old Tape",
};

parameter_properties vintage_delay_audio_module::param_props[] = {
    { 120,      30,    300,2701, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_BPM, NULL, "bpm", "Tempo" },
    {  4,        1,    16,    1, PF_INT | PF_SCALE_LINEAR | PF_CTL_FADER, NULL, "subdiv", "Subdivide"},
    {  3,        1,    16,    1, PF_INT | PF_SCALE_LINEAR | PF_CTL_FADER, NULL, "time_l", "Time L"},
    {  5,        1,    16,    1, PF_INT | PF_SCALE_LINEAR | PF_CTL_FADER, NULL, "time_r", "Time R"},
    { 0.5,       0,    1,     0, PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB, NULL, "feedback", "Feedback" },
    { 0.25,      0,    2,   100, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_COEF, NULL, "amount", "Amount" },
    { 1,         0,    1,     0, PF_ENUM | PF_CTL_COMBO, vintage_delay_mixmodes, "mix_mode", "Mix mode" },
    { 1,         0,    2,     0, PF_ENUM | PF_CTL_COMBO, vintage_delay_fbmodes, "medium", "Medium" },
};

static synth::ladspa_info vintage_delay_info = { 0x8482, "VintageDelay", "Calf Vintage Delay", "Krzysztof Foltman", copyright, "DelayPlugin" };

ALL_WRAPPERS(vintage_delay)

////////////////////////////////////////////////////////////////////////////

const char *rotary_speaker_audio_module::port_names[] = {"In L", "In R", "Out L", "Out R"};

const char *rotary_speaker_speed_names[] = { "Off", "Chorale", "Tremolo", "HoldPedal", "ModWheel" };

parameter_properties rotary_speaker_audio_module::param_props[] = {
    { 2,         0,  4, 1.01, PF_ENUM | PF_CTL_COMBO, rotary_speaker_speed_names, "vib_speed", "Speed Mode" },
    { 0.5,        0,    1,    0, PF_FLOAT | PF_CTL_KNOB | PF_SCALE_PERC, NULL, "spacing", "Tap Spacing" },
    { 0.5,        0,    1,    0, PF_FLOAT | PF_CTL_KNOB | PF_SCALE_PERC, NULL, "shift", "Tap Offset" },
    { 0.15,       0,    1,    0, PF_FLOAT | PF_CTL_KNOB | PF_SCALE_PERC, NULL, "mod_depth", "Mod Depth" },
};

static synth::ladspa_info rotary_speaker_info = { 0x8483, "RotarySpeaker", "Calf Rotary Speaker", "Krzysztof Foltman", copyright, "SimulationPlugin" };

ALL_WRAPPERS(rotary_speaker)

////////////////////////////////////////////////////////////////////////////

static synth::ladspa_info organ_info = { 0x8481, "Organ", "Calf Organ", "Krzysztof Foltman", copyright, "SynthesizerPlugin" };

ALL_WRAPPERS(organ)

#ifdef ENABLE_EXPERIMENTAL
#endif
////////////////////////////////////////////////////////////////////////////

static synth::ladspa_info monosynth_info = { 0x8480, "Monosynth", "Calf Monosynth", "Krzysztof Foltman", copyright, "SynthesizerPlugin" };

ALL_WRAPPERS(monosynth)

////////////////////////////////////////////////////////////////////////////

#if USE_LV2
extern "C" {

const LV2_Descriptor *lv2_descriptor(uint32_t index)
{
    switch (index) {
        case 0: return &::lv2_filter.descriptor;
        case 1: return &::lv2_flanger.descriptor;
        case 2: return &::lv2_reverb.descriptor;
        case 3: return &::lv2_vintage_delay.descriptor;
        case 4: return &::lv2_monosynth.descriptor;
        case 5: return &::lv2_organ.descriptor;
        case 6: return &::lv2_rotary_speaker.descriptor;
        case 7: return &::lv2_phaser.descriptor;
#ifdef ENABLE_EXPERIMENTAL
#endif
        default: return NULL;
    }
}

};

#endif
#if USE_LADSPA
extern "C" {

const LADSPA_Descriptor *ladspa_descriptor(unsigned long Index)
{
    switch (Index) {
        case 0: return &::ladspa_filter.descriptor;
        case 1: return &::ladspa_flanger.descriptor;
        case 2: return &::ladspa_reverb.descriptor;
        case 3: return &::ladspa_vintage_delay.descriptor;
        case 4: return &::ladspa_rotary_speaker.descriptor;
        case 5: return &::ladspa_phaser.descriptor;
#ifdef ENABLE_EXPERIMENTAL
#endif
        default: return NULL;
    }
}

};

#if USE_DSSI
extern "C" {

const DSSI_Descriptor *dssi_descriptor(unsigned long Index)
{
    switch (Index) {
        case 0: return &::ladspa_filter.dssi_descriptor;
        case 1: return &::ladspa_flanger.dssi_descriptor;
        case 2: return &::ladspa_reverb.dssi_descriptor;
        case 3: return &::ladspa_monosynth.dssi_descriptor;
        case 4: return &::ladspa_vintage_delay.dssi_descriptor;
        case 5: return &::ladspa_organ.dssi_descriptor;
        case 6: return &::ladspa_rotary_speaker.dssi_descriptor;
        case 7: return &::ladspa_phaser.dssi_descriptor;
#ifdef ENABLE_EXPERIMENTAL
#endif
        default: return NULL;
    }
}

};
#endif

std::string synth::get_builtin_modules_rdf()
{
    std::string rdf;
    
    rdf += ::ladspa_flanger.generate_rdf();
    rdf += ::ladspa_reverb.generate_rdf();
    rdf += ::ladspa_filter.generate_rdf();
    rdf += ::ladspa_vintage_delay.generate_rdf();
    rdf += ::ladspa_monosynth.generate_rdf();
    rdf += ::ladspa_organ.generate_rdf();
    rdf += ::ladspa_rotary_speaker.generate_rdf();
    rdf += ::ladspa_phaser.generate_rdf();
    
    return rdf;
}

#endif

template<class Module>
giface_plugin_info create_plugin_info(ladspa_info &info)
{
    giface_plugin_info pi;
    pi.info = &info;
    pi.inputs = Module::in_count;
    pi.outputs = Module::out_count;
    pi.params = Module::param_count;
    pi.rt_capable = Module::rt_capable;
    pi.midi_in_capable = Module::support_midi;
    pi.param_props = Module::param_props;
    return pi;
}

void synth::get_all_plugins(std::vector<giface_plugin_info> &plugins)
{
    plugins.push_back(create_plugin_info<filter_audio_module>(filter_info));
    plugins.push_back(create_plugin_info<flanger_audio_module>(flanger_info));
    plugins.push_back(create_plugin_info<reverb_audio_module>(reverb_info));
    plugins.push_back(create_plugin_info<monosynth_audio_module>(monosynth_info));
    plugins.push_back(create_plugin_info<vintage_delay_audio_module>(vintage_delay_info));
    plugins.push_back(create_plugin_info<organ_audio_module>(organ_info));
    plugins.push_back(create_plugin_info<rotary_speaker_audio_module>(rotary_speaker_info));
    plugins.push_back(create_plugin_info<phaser_audio_module>(phaser_info));
}
