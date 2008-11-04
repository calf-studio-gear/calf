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
#include <calf/modules.h>
#include <calf/modules_dev.h>

using namespace synth;

const char *synth::calf_copyright_info = "(C) 2001-2008 Krzysztof Foltman, license: LGPL";

////////////////////////////////////////////////////////////////////////////

CALF_PORT_NAMES(flanger) = {"In L", "In R", "Out L", "Out R"};

CALF_PORT_PROPS(flanger) = {
    { 0.1,      0.1, 10,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "min_delay", "Minimum delay" },
    { 0.5,      0.1, 10,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "mod_depth", "Modulation depth" },
    { 0.25,    0.01, 20,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL, "mod_rate", "Modulation rate" },
    { 0.90,   -0.99, 0.99,  0, PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB | PF_UNIT_COEF, NULL, "feedback", "Feedback" },
    { 0,          0, 360,   9, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_DEG, NULL, "stereo", "Stereo phase" },
    { 0,          0, 1,     2, PF_BOOL | PF_CTL_BUTTON , NULL, "reset", "Reset" },
    { 1,          0, 2,     0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_NOBOUNDS, NULL, "amount", "Amount" },
};

CALF_PLUGIN_INFO(flanger) = { 0x847d, "Flanger", "Calf Flanger", "Krzysztof Foltman", synth::calf_copyright_info, "FlangerPlugin" };

////////////////////////////////////////////////////////////////////////////

CALF_PORT_NAMES(phaser) = {"In L", "In R", "Out L", "Out R"};

CALF_PORT_PROPS(phaser) = {
    { 1000,      20, 20000, 0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL, "base_freq", "Center Freq" },
    { 4000,       0, 10800,  0, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_CENTS, NULL, "mod_depth", "Modulation depth" },
    { 0.25,    0.01, 20,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL, "mod_rate", "Modulation rate" },
    { 0.25,   -0.99, 0.99,  0, PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB | PF_UNIT_COEF, NULL, "feedback", "Feedback" },
    { 6,          1, 12,   12, PF_INT | PF_SCALE_LINEAR | PF_CTL_KNOB, NULL, "stages", "# Stages" },
    { 180,        0, 360,   9, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_DEG, NULL, "stereo", "Stereo phase" },
    { 0,          0, 1,     2, PF_BOOL | PF_CTL_BUTTON , NULL, "reset", "Reset" },
    { 1,          0, 2,     0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_NOBOUNDS, NULL, "amount", "Amount" },
};

CALF_PLUGIN_INFO(phaser) = { 0x8484, "Phaser", "Calf Phaser", "Krzysztof Foltman", synth::calf_copyright_info, "PhaserPlugin" };

////////////////////////////////////////////////////////////////////////////

CALF_PORT_NAMES(reverb) = {"In L", "In R", "Out L", "Out R"};

const char *reverb_room_sizes[] = { "Small", "Medium", "Large", "Tunnel-like" };

CALF_PORT_PROPS(reverb) = {
    { 1.5,      0.5, 15.0,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_SEC, NULL, "decay_time", "Decay time" },
    { 5000,    2000,20000,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL, "hf_damp", "High Frq Damp" },
    { 2,          0,    3,    0, PF_ENUM | PF_CTL_COMBO , reverb_room_sizes, "room_size", "Room size", },
    { 0.5,        0,    1,    0, PF_FLOAT | PF_CTL_KNOB | PF_SCALE_PERC, NULL, "diffusion", "Diffusion" },
    { 0.25,       0,    2,    0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_NOBOUNDS, NULL, "amount", "Amount" },
};

CALF_PLUGIN_INFO(reverb) = { 0x847e, "Reverb", "Calf Reverb", "Krzysztof Foltman", synth::calf_copyright_info, "ReverbPlugin" };

////////////////////////////////////////////////////////////////////////////

CALF_PORT_NAMES(filter) = {"In L", "In R", "Out L", "Out R"};

const char *filter_choices[] = {
    "12dB/oct Lowpass",
    "24dB/oct Lowpass",
    "36dB/oct Lowpass",
    "12dB/oct Highpass",
    "24dB/oct Highpass",
    "36dB/oct Highpass",
};

CALF_PORT_PROPS(filter) = {
    { 2000,      10,20000,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL, "freq", "Frequency" },
    { 0.707,  0.707,   20,    0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_COEF, NULL, "res", "Resonance" },
    { 0,          0,    5,    1, PF_ENUM | PF_CTL_COMBO, filter_choices, "mode", "Mode" },
    { 20,         5,  100,    20, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "inertia", "Inertia"},
};

CALF_PLUGIN_INFO(filter) = { 0x847f, "Filter", "Calf Filter", "Krzysztof Foltman", synth::calf_copyright_info, "FilterPlugin" };

////////////////////////////////////////////////////////////////////////////

CALF_PORT_NAMES(vintage_delay) = {"In L", "In R", "Out L", "Out R"};

const char *vintage_delay_mixmodes[] = {
    "Stereo",
    "Ping-Pong",
};

const char *vintage_delay_fbmodes[] = {
    "Plain",
    "Tape",
    "Old Tape",
};

CALF_PORT_PROPS(vintage_delay) = {
    { 120,      30,    300,2701, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_BPM, NULL, "bpm", "Tempo" },
    {  4,        1,    16,    1, PF_INT | PF_SCALE_LINEAR | PF_CTL_FADER, NULL, "subdiv", "Subdivide"},
    {  3,        1,    16,    1, PF_INT | PF_SCALE_LINEAR | PF_CTL_FADER, NULL, "time_l", "Time L"},
    {  5,        1,    16,    1, PF_INT | PF_SCALE_LINEAR | PF_CTL_FADER, NULL, "time_r", "Time R"},
    { 0.5,       0,    1,     0, PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB, NULL, "feedback", "Feedback" },
    { 0.25,      0,    2,   100, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_COEF, NULL, "amount", "Amount" },
    { 1,         0,    1,     0, PF_ENUM | PF_CTL_COMBO, vintage_delay_mixmodes, "mix_mode", "Mix mode" },
    { 1,         0,    2,     0, PF_ENUM | PF_CTL_COMBO, vintage_delay_fbmodes, "medium", "Medium" },
};

CALF_PLUGIN_INFO(vintage_delay) = { 0x8482, "VintageDelay", "Calf Vintage Delay", "Krzysztof Foltman", synth::calf_copyright_info, "DelayPlugin" };

////////////////////////////////////////////////////////////////////////////

CALF_PORT_NAMES(rotary_speaker) = {"In L", "In R", "Out L", "Out R"};

const char *rotary_speaker_speed_names[] = { "Off", "Chorale", "Tremolo", "HoldPedal", "ModWheel", "Manual" };

CALF_PORT_PROPS(rotary_speaker) = {
    { 2,         0,  5, 1.01, PF_ENUM | PF_CTL_COMBO, rotary_speaker_speed_names, "vib_speed", "Speed Mode" },
    { 0.5,        0,    1,    0, PF_FLOAT | PF_CTL_KNOB | PF_SCALE_PERC, NULL, "spacing", "Tap Spacing" },
    { 0.5,        0,    1,    0, PF_FLOAT | PF_CTL_KNOB | PF_SCALE_PERC, NULL, "shift", "Tap Offset" },
    { 0.10,       0,    1,    0, PF_FLOAT | PF_CTL_KNOB | PF_SCALE_PERC, NULL, "mod_depth", "Mod Depth" },
    { 390,       10,   600,    0, PF_FLOAT | PF_CTL_KNOB | PF_SCALE_LOG | PF_UNIT_RPM, NULL, "treble_speed", "Treble Motor" },
    { 410,      10,   600,    0, PF_FLOAT | PF_CTL_KNOB | PF_SCALE_LOG | PF_UNIT_RPM, NULL, "bass_speed", "Bass Motor" },
    { 0.7,        0,    1,  101, PF_FLOAT | PF_CTL_KNOB | PF_SCALE_PERC, NULL, "mic_distance", "Mic Distance" },
    { 0.3,        0,    1,  101, PF_FLOAT | PF_CTL_KNOB | PF_SCALE_PERC, NULL, "reflection", "Reflection" },
};

CALF_PLUGIN_INFO(rotary_speaker) = { 0x8483, "RotarySpeaker", "Calf Rotary Speaker", "Krzysztof Foltman", synth::calf_copyright_info, "SimulationPlugin" };

////////////////////////////////////////////////////////////////////////////

CALF_PORT_NAMES(multichorus) = {"In L", "In R", "Out L", "Out R"};

CALF_PORT_PROPS(multichorus) = {
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

CALF_PLUGIN_INFO(multichorus) = { 0x8501, "MultiChorus", "Calf MultiChorus", "Krzysztof Foltman", synth::calf_copyright_info, "ChorusPlugin" };

////////////////////////////////////////////////////////////////////////////

#if ENABLE_EXPERIMENTAL

CALF_PORT_NAMES(compressor) = {"In L", "In R", "Out L", "Out R"};

const char *compressor_detection_names[] = { "RMS", "Peak" };
const char *compressor_stereo_link_names[] = { "Average", "Maximum" };

CALF_PORT_PROPS(compressor) = {
    { 0.0625,      0, 1,    0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "threshold", "Threshold" },
    { 5,      1, 100,  101, PF_FLOAT | PF_SCALE_LOG_INF | PF_CTL_KNOB | PF_UNIT_COEF, NULL, "ratio", "Ratio" },
    { 15,     0.01, 2000, 0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "attack", "Attack" },
    { 150,    0.01, 2000, 0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "release", "Release" },
    { 2,      1, 16,   0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "makeup", "Makeup Gain" },
    { 0.25,      0,  1,   0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "knee", "Knee" },
    { 0,      0,  1,   0, PF_ENUM | PF_CTL_COMBO, compressor_detection_names, "detection", "Detection" },
    { 0,      0,  1,   0, PF_ENUM | PF_CTL_COMBO, compressor_stereo_link_names, "stereo_link", "Stereo Link" },
    { 0,      0,  1,   0, PF_BOOL | PF_CTL_TOGGLE, NULL, "aweighting", "A-weighting" },
    { 0, 0.03125, 1,    0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_CTLO_REVERSE | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "compression", "Compression" },
    { 0,      0,  1,    0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "peak", "Peak" },
    { 0,      1,  0,    0, PF_BOOL | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "clip", "Clip" },
    { 0,      0,  1,    0, PF_BOOL | PF_CTL_TOGGLE, NULL, "bypass", "Bypass" },
};

CALF_PLUGIN_INFO(compressor) = { 0x8502, "Compressor", "Calf Compressor", "Thor Harald Johansen", synth::calf_copyright_info, "CompressorPlugin" };

#endif

////////////////////////////////////////////////////////////////////////////

void synth::get_all_plugins(std::vector<plugin_metadata_iface *> &plugins)
{
    #define PER_MODULE_ITEM(name, isSynth, jackname) plugins.push_back(new name##_metadata);
    #define PER_SMALL_MODULE_ITEM(...)
    #include <calf/modulelist.h>
}

