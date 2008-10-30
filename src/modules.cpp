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

template<class Module> LV2_Descriptor lv2_small_wrapper<Module>::descriptor;

const char *synth::calf_copyright_info = "(C) 2001-2008 Krzysztof Foltman, license: LGPL";

////////////////////////////////////////////////////////////////////////////

const char *flanger_audio_module::port_names[] = {"In L", "In R", "Out L", "Out R"};

parameter_properties flanger_audio_module::param_props[] = {
    { 0.1,      0.1, 10,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "min_delay", "Minimum delay" },
    { 0.5,      0.1, 10,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "mod_depth", "Modulation depth" },
    { 0.25,    0.01, 20,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL, "mod_rate", "Modulation rate" },
    { 0.90,   -0.99, 0.99,  0, PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB | PF_UNIT_COEF, NULL, "feedback", "Feedback" },
    { 0,          0, 360,   9, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_DEG, NULL, "stereo", "Stereo phase" },
    { 0,          0, 1,     2, PF_BOOL | PF_CTL_BUTTON , NULL, "reset", "Reset" },
    { 1,          0, 2,     0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_NOBOUNDS, NULL, "amount", "Amount" },
};

synth::ladspa_plugin_info flanger_audio_module::plugin_info = { 0x847d, "Flanger", "Calf Flanger", "Krzysztof Foltman", synth::calf_copyright_info, "FlangerPlugin" };

////////////////////////////////////////////////////////////////////////////

const char *phaser_audio_module::port_names[] = {"In L", "In R", "Out L", "Out R"};

parameter_properties phaser_audio_module::param_props[] = {
    { 1000,      20, 20000, 0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL, "base_freq", "Center Freq" },
    { 4000,       0, 10800,  0, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_CENTS, NULL, "mod_depth", "Modulation depth" },
    { 0.25,    0.01, 20,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL, "mod_rate", "Modulation rate" },
    { 0.25,   -0.99, 0.99,  0, PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB | PF_UNIT_COEF, NULL, "feedback", "Feedback" },
    { 6,          1, 12,   12, PF_INT | PF_SCALE_LINEAR | PF_CTL_KNOB, NULL, "stages", "# Stages" },
    { 180,        0, 360,   9, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_DEG, NULL, "stereo", "Stereo phase" },
    { 0,          0, 1,     2, PF_BOOL | PF_CTL_BUTTON , NULL, "reset", "Reset" },
    { 1,          0, 2,     0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_NOBOUNDS, NULL, "amount", "Amount" },
};

synth::ladspa_plugin_info phaser_audio_module::plugin_info = { 0x8484, "Phaser", "Calf Phaser", "Krzysztof Foltman", synth::calf_copyright_info, "PhaserPlugin" };

////////////////////////////////////////////////////////////////////////////

const char *reverb_audio_module::port_names[] = {"In L", "In R", "Out L", "Out R"};

const char *reverb_room_sizes[] = { "Small", "Medium", "Large", "Tunnel-like" };

parameter_properties reverb_audio_module::param_props[] = {
    { 1.5,      0.5, 15.0,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_SEC, NULL, "decay_time", "Decay time" },
    { 5000,    2000,20000,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL, "hf_damp", "High Frq Damp" },
    { 2,          0,    3,    0, PF_ENUM | PF_CTL_COMBO , reverb_room_sizes, "room_size", "Room size", },
    { 0.5,        0,    1,    0, PF_FLOAT | PF_CTL_KNOB | PF_SCALE_PERC, NULL, "diffusion", "Diffusion" },
    { 0.25,       0,    2,    0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_NOBOUNDS, NULL, "amount", "Amount" },
};

synth::ladspa_plugin_info reverb_audio_module::plugin_info = { 0x847e, "Reverb", "Calf Reverb", "Krzysztof Foltman", synth::calf_copyright_info, "ReverbPlugin" };

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

synth::ladspa_plugin_info filter_audio_module::plugin_info = { 0x847f, "Filter", "Calf Filter", "Krzysztof Foltman", synth::calf_copyright_info, "FilterPlugin" };

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

synth::ladspa_plugin_info vintage_delay_audio_module::plugin_info = { 0x8482, "VintageDelay", "Calf Vintage Delay", "Krzysztof Foltman", synth::calf_copyright_info, "DelayPlugin" };

////////////////////////////////////////////////////////////////////////////

const char *rotary_speaker_audio_module::port_names[] = {"In L", "In R", "Out L", "Out R"};

const char *rotary_speaker_speed_names[] = { "Off", "Chorale", "Tremolo", "HoldPedal", "ModWheel", "Manual" };

parameter_properties rotary_speaker_audio_module::param_props[] = {
    { 2,         0,  5, 1.01, PF_ENUM | PF_CTL_COMBO, rotary_speaker_speed_names, "vib_speed", "Speed Mode" },
    { 0.5,        0,    1,    0, PF_FLOAT | PF_CTL_KNOB | PF_SCALE_PERC, NULL, "spacing", "Tap Spacing" },
    { 0.5,        0,    1,    0, PF_FLOAT | PF_CTL_KNOB | PF_SCALE_PERC, NULL, "shift", "Tap Offset" },
    { 0.10,       0,    1,    0, PF_FLOAT | PF_CTL_KNOB | PF_SCALE_PERC, NULL, "mod_depth", "Mod Depth" },
    { 390,       10,   600,    0, PF_FLOAT | PF_CTL_KNOB | PF_SCALE_LOG | PF_UNIT_RPM, NULL, "treble_speed", "Treble Motor" },
    { 410,      10,   600,    0, PF_FLOAT | PF_CTL_KNOB | PF_SCALE_LOG | PF_UNIT_RPM, NULL, "bass_speed", "Bass Motor" },
    { 0.7,        0,    1,  101, PF_FLOAT | PF_CTL_KNOB | PF_SCALE_PERC, NULL, "mic_distance", "Mic Distance" },
    { 0.3,        0,    1,  101, PF_FLOAT | PF_CTL_KNOB | PF_SCALE_PERC, NULL, "reflection", "Reflection" },
};

synth::ladspa_plugin_info rotary_speaker_audio_module::plugin_info = { 0x8483, "RotarySpeaker", "Calf Rotary Speaker", "Krzysztof Foltman", synth::calf_copyright_info, "SimulationPlugin" };

////////////////////////////////////////////////////////////////////////////

const char *multichorus_audio_module::port_names[] = {"In L", "In R", "Out L", "Out R"};

parameter_properties multichorus_audio_module::param_props[] = {
    { 5,        0.1,  10,   0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "min_delay", "Minimum delay" },
    { 6,        0.1,  10,   0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "mod_depth", "Modulation depth" },
    { 0.5,     0.01,  20,   0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL, "mod_rate", "Modulation rate" },
    { 180,        0, 360,  91, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_DEG, NULL, "stereo", "Stereo phase" },
    { 4,          1,   8,   8, PF_INT | PF_SCALE_LINEAR | PF_CTL_FADER, NULL, "voices", "Voices"},
    { 64,         0, 360,  91, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_DEG, NULL, "vphase", "Inter-voice phase" },
    { 2,          0,   4,   0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_NOBOUNDS, NULL, "amount", "Amount" },
    { 180,        0, 360,  91, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_DEG | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "lfo_phase_l", "Left LFO phase" },
    { 180,        0, 360,  91, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_DEG | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "lfo_phase_r", "Right LFO phase" },
};

synth::ladspa_plugin_info multichorus_audio_module::plugin_info = { 0x8501, "MultiChorus", "Calf MultiChorus", "Krzysztof Foltman", synth::calf_copyright_info, "ChorusPlugin" };

parameter_properties compressor_audio_module::param_props[] = {
    { 1,      0, 1,    0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "threshold", "Threshold" },
    { 1,      1, 100,  0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_COEF, NULL, "ratio", "Ratio" },
    { 0,      0, 1,    0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_FADER | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "compression", "Compression" }
};

synth::ladspa_plugin_info compressor_audio_module::plugin_info = { 0x8502, "Compressor", "Calf Compressor", "Thor Harald Johansen", synth::calf_copyright_info, "CompressorPlugin" };

const char *compressor_audio_module::port_names[] = {"In", "Out"};

////////////////////////////////////////////////////////////////////////////

template<class Module>
giface_plugin_info create_plugin_info(ladspa_plugin_info &info)
{
    giface_plugin_info pi;
    pi.info = &info;
    pi.inputs = Module::in_count;
    pi.outputs = Module::out_count;
    pi.params = Module::param_count;
    pi.rt_capable = Module::rt_capable;
    pi.midi_in_capable = Module::support_midi;
    pi.param_props = Module::param_props;
    pi.is_noisy = Module::is_noisy;
    pi.is_cv = Module::is_cv;
    return pi;
}

void synth::get_all_plugins(std::vector<giface_plugin_info> &plugins)
{
    #define PER_MODULE_ITEM(name, isSynth, jackname) plugins.push_back(create_plugin_info<name##_audio_module>(name##_audio_module::plugin_info));
    #define PER_SMALL_MODULE_ITEM(...)
    #include <calf/modulelist.h>
}

