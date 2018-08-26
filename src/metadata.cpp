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
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301  USA
 */
#include <config.h>
#include <calf/giface.h>
#include <calf/metadata.h>
#include <calf/audio_fx.h>
#include <calf/modmatrix.h>
#include <calf/utils.h>

using namespace dsp;
using namespace std;

namespace calf_plugins {

const char *calf_copyright_info = "(C) 2001-2017 Krzysztof Foltman, Thor Harald Johanssen, Markus Schmidt, Christian Holschuh and others; license: LGPL";
const char *crossover_filter_choices[] = { "LR2", "LR4", "LR8" };
const char *mb_crossover_filter_choices[] = { "LR4", "LR8" };

////////////////////////////////////////////////////////////////////////////
// A few macros to make
const char *eq_analyzer_mode_names[] = { "Input", "Output", "Difference" };
const char *periodical_mode_names[] = {
    "BPM",
    "ms",
    "Hz",
    "Sync",
};

#define BYPASS_AND_LEVEL_PARAMS \
    { 0,           0,           1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "bypass", "Bypass" }, \
    { 1,           0.015625,    64,    0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB | PF_PROP_NOBOUNDS, NULL, "level_in", "Input Gain" }, \
    { 1,           0.015625,    64,    0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB | PF_PROP_NOBOUNDS, NULL, "level_out", "Output Gain" },

#define METERING_PARAMS \
    { 0,           0,           1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "meter_inL", "Meter-InL" }, \
    { 0,           0,           1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "meter_inR", "Meter-InR" }, \
    { 0,           0,           1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "meter_outL", "Meter-OutL" }, \
    { 0,           0,           1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "meter_outR", "Meter-OutR" }, \
    { 0,           0,           1,     0,  PF_FLOAT | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "clip_inL", "0dB-InL" }, \
    { 0,           0,           1,     0,  PF_FLOAT | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "clip_inR", "0dB-InR" }, \
    { 0,           0,           1,     0,  PF_FLOAT | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "clip_outL", "0dB-OutL" }, \
    { 0,           0,           1,     0,  PF_FLOAT | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "clip_outR", "0dB-OutR" },

#define LPHP_PARAMS \
    { 0,           0,           5,     0,  PF_ENUM | PF_CTL_COMBO, active_mode_names, "hp_active", "HP Active" }, \
    { 30,          10,          20000, 0,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL, "hp_freq", "HP Freq" }, \
    { 1,           0,           2,     0,  PF_ENUM | PF_CTL_COMBO, rolloff_mode_names, "hp_mode", "HP Mode" }, \
    { 0.707,       0.1,         10,    1,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_COEF, NULL, "hp_q", "HP Q" }, \
    { 0,           0,           5,     0,  PF_ENUM | PF_CTL_COMBO, active_mode_names, "lp_active", "LP Active" }, \
    { 18000,       10,          20000, 0,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL, "lp_freq", "LP Freq" }, \
    { 1,           0,           2,     0,  PF_ENUM | PF_CTL_COMBO, rolloff_mode_names, "lp_mode", "LP Mode" }, \
    { 0.707,       0.1,         10,    1,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_COEF, NULL, "lp_q", "LP Q" }, \

#define SHELF_PARAMS \
    { 0,           0,           5,     0,  PF_ENUM | PF_CTL_COMBO, active_mode_names, "ls_active", "LS Active" }, \
    { 1,           0.015625,    64,    0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "ls_level", "Level L" }, \
    { 100,         10,          20000, 0,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL, "ls_freq", "Freq L" }, \
    { 0.707,       0.1,         10,    1,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_COEF, NULL, "ls_q", "LS Q" }, \
    { 0,           0,           5,     0,  PF_ENUM | PF_CTL_COMBO, active_mode_names, "hs_active", "HS Active" }, \
    { 1,           0.015625,    64,    0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "hs_level", "Level H" }, \
    { 5000,        10,          20000, 0,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL, "hs_freq", "Freq H" }, \
    { 0.707,       0.1,         10,    1,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_COEF, NULL, "hs_q", "HS Q" },

#define EQ_BAND_PARAMS(band, frequency) \
    { 0,           0,           5,     0,  PF_ENUM | PF_CTL_COMBO, active_mode_names, "p" #band "_active", "F" #band " Active" }, \
    { 1,           0.015625,    64,    0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "p" #band "_level", "Level " #band }, \
    { frequency,   10,          20000, 0,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ | PF_PROP_GRAPH, NULL, "p" #band "_freq", "Freq " #band }, \
    { 1,           0.1,         100,   1,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_COEF, NULL, "p" #band "_q", "Q " #band },

#define EQ_DISPLAY_PARAMS \
    { 1,           0,           1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "individuals", "Individual Filters" }, \
    { 0.25,   0.0625,           1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_FADER | PF_UNIT_DB, NULL, "zoom", "Zoom" }, \
    { 0,           0,           1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "analyzer", "Analyzer Active" }, \
    { 1,           0,           2,     0,  PF_ENUM | PF_CTL_COMBO, eq_analyzer_mode_names, "analyzer_mode", "Analyzer Mode" }, \

#define PERIODICAL_DEFINITIONS(init) \
    { init,      0,    3,     0, PF_ENUM | PF_CTL_COMBO, periodical_mode_names, "timing", "Timing" }, \
    { 120,       30,   300,   1, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_BPM, NULL, "bpm", "BPM" }, \
    { 500,       10,   2000,  1, PF_INT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "ms", "ms" }, \
    { 2,         0.01, 100,   0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL, "hz", "Frequency" }, \
    { 120,       1,    300,   1, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_BPM | PF_SYNC_BPM, NULL, "bpm_host", "Host BPM" }, \


////////////////////////////////////////////////////////////////////////////

CALF_PORT_NAMES(flanger) = {"In L", "In R", "Out L", "Out R"};

CALF_PORT_PROPS(flanger) = {
    { 0.5,      0.1, 10,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC | PF_PROP_GRAPH, NULL, "min_delay", "Min delay" },
    { 2.0,      0.1, 10,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "mod_depth", "Mod depth" },
    { 0.1,     0.01, 20,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL, "mod_rate", "Mod rate" },
    { 0.80,   -0.99, 0.99,  0, PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB | PF_UNIT_COEF, NULL, "feedback", "Feedback" },
    { 90,          0, 360,   9, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_DEG, NULL, "stereo", "Stereo phase" },
    { 0,          0, 1,     2, PF_BOOL | PF_CTL_BUTTON , NULL, "reset", "Reset" },
    { 0.5,        0, 4,     0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_NOBOUNDS, NULL, "amount", "Amount" },
    { 1.0,        0, 4,     0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_NOBOUNDS, NULL, "dry", "Dry Amount" },
    { 1,          0,     1, 0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "on", "Active" },
    { 1,           0.015625,    64,    0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB | PF_PROP_NOBOUNDS, NULL, "level_in", "Input Gain" }, \
    { 1,           0.015625,    64,    0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB | PF_PROP_NOBOUNDS, NULL, "level_out", "Output Gain" },
    METERING_PARAMS
    { 1,           0,           1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "lfo", "LFO" }, \
    {}
};

CALF_PLUGIN_INFO(flanger) = { 0x847d, "Flanger", "Calf Flanger", "Calf Studio Gear", calf_plugins::calf_copyright_info, "ModulatorPlugin" };

////////////////////////////////////////////////////////////////////////////

CALF_PORT_NAMES(phaser) = {"In L", "In R", "Out L", "Out R"};

CALF_PORT_PROPS(phaser) = {
    { 1000,      20, 20000, 0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ | PF_PROP_GRAPH, NULL, "base_freq", "Center Freq" },
    { 4000,       0, 10800,  0, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_CENTS, NULL, "mod_depth", "Mod depth" },
    { 0.1,    0.01, 20,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL, "mod_rate", "Mod rate" },
    { 0.5,    -0.99, 0.99,  0, PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB | PF_UNIT_COEF, NULL, "feedback", "Feedback" },
    { 6,          1, 12,   12, PF_INT | PF_SCALE_LINEAR | PF_CTL_KNOB, NULL, "stages", "# Stages" },
    { 180,        0, 360,   9, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_DEG, NULL, "stereo", "Stereo phase" },
    { 0,          0, 1,     2, PF_BOOL | PF_CTL_BUTTON , NULL, "reset", "Reset" },
    { 0.5,          0, 4,     0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_NOBOUNDS, NULL, "amount", "Amount" },
    { 1.0,        0, 4,     0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_NOBOUNDS, NULL, "dry", "Dry Amount" },
    { 1,          0,     1, 0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "on", "Active" },
    { 1,           0.015625,    64,    0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB | PF_PROP_NOBOUNDS, NULL, "level_in", "Input Gain" }, \
    { 1,           0.015625,    64,    0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB | PF_PROP_NOBOUNDS, NULL, "level_out", "Output Gain" },
    METERING_PARAMS
    { 1,           0,           1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "lfo", "LFO" }, \
    {}
};

CALF_PLUGIN_INFO(phaser) = { 0x8484, "Phaser", "Calf Phaser", "Calf Studio Gear", calf_plugins::calf_copyright_info, "ModulatorPlugin" };

////////////////////////////////////////////////////////////////////////////

CALF_PORT_NAMES(reverb) = {"In L", "In R", "Out L", "Out R"};

const char *reverb_room_sizes[] = { "Small", "Medium", "Large", "Tunnel-like", "Large/smooth", "Experimental" };

CALF_PORT_PROPS(reverb) = {
    { 0,           0,           1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "meter_inL", "Meter-InL" }, \
    { 0,           0,           1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "meter_inR", "Meter-InR" }, \
    { 0,           0,           1,     0,  PF_FLOAT | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "clip_outL", "0dB-OutL" }, \

    { 1.5,      0.4, 15.0,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_SEC, NULL, "decay_time", "Decay time" },
    { 5000,    2000,20000,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL, "hf_damp", "High Frq Damp" },
    { 2,          0,    5,    0, PF_ENUM | PF_CTL_COMBO , reverb_room_sizes, "room_size", "Room size", },
    { 0.5,        0,    1,    0, PF_FLOAT | PF_CTL_KNOB | PF_SCALE_PERC, NULL, "diffusion", "Diffusion" },
    { 0.25,       0,    2,    0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_NOBOUNDS, NULL, "amount", "Wet Amount" },
    { 1.0,        0,    2,    0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_NOBOUNDS, NULL, "dry", "Dry Amount" },
    { 0,          0,   500,    0, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "predelay", "Pre Delay" },
    { 300,       20, 20000, 0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL, "bass_cut", "Bass Cut" },
    { 5000,      20, 20000, 0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL, "treble_cut", "Treble Cut" },
    { 1,          0,     1, 0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "on", "Active" },
    { 1,           0.015625,    64,    0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB | PF_PROP_NOBOUNDS, NULL, "level_in", "Input Gain" }, \
    { 1,           0.015625,    64,    0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB | PF_PROP_NOBOUNDS, NULL, "level_out", "Output Gain" },

    { 0,           0,           1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "meter_outL", "Meter-OutL" }, \
    { 0,           0,           1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "meter_outR", "Meter-OutR" }, \
    { 0,           0,           1,     0,  PF_FLOAT | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "clip_inL", "0dB-InL" }, \
    { 0,           0,           1,     0,  PF_FLOAT | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "clip_inR", "0dB-InR" }, \
    { 0,           0,           1,     0,  PF_FLOAT | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "clip_outR", "0dB-OutR" },
    {}
};

CALF_PLUGIN_INFO(reverb) = { 0x847e, "Reverb", "Calf Reverb", "Calf Studio Gear", calf_plugins::calf_copyright_info, "ReverbPlugin" };

////////////////////////////////////////////////////////////////////////////

CALF_PORT_NAMES(filter) = {"In L", "In R", "Out L", "Out R"};

const char *filter_choices[] = {
    "12dB/oct Lowpass",
    "24dB/oct Lowpass",
    "36dB/oct Lowpass",
    "12dB/oct Highpass",
    "24dB/oct Highpass",
    "36dB/oct Highpass",
    "6dB/oct Bandpass",
    "12dB/oct Bandpass",
    "18dB/oct Bandpass",
    "6dB/oct Bandreject",
    "12dB/oct Bandreject",
    "18dB/oct Bandreject",
    "Allpass",
};

CALF_PORT_PROPS(filter) = {
    { 2000,      10,20000,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ | PF_PROP_GRAPH, NULL, "freq", "Frequency" },
    { 0.707,  0.707,   32,    0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_COEF, NULL, "res", "Resonance" },
    { biquad_filter_module::mode_12db_lp,
      biquad_filter_module::mode_12db_lp,
      biquad_filter_module::mode_count - 1,
                                1,  PF_ENUM | PF_CTL_COMBO, filter_choices, "mode", "Mode" },
    { 20,         5,  100,    20, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "inertia", "Inertia"},
    BYPASS_AND_LEVEL_PARAMS
    METERING_PARAMS
};

CALF_PLUGIN_INFO(filter) = { 0x847f, "Filter", "Calf Filter", "Calf Studio Gear", calf_plugins::calf_copyright_info, "FilterPlugin" };

////////////////////////////////////////////////////////////////////////////

CALF_PORT_NAMES(filterclavier) = {"In L", "In R", "Out L", "Out R"};

CALF_PORT_PROPS(filterclavier) = {
    { 0,        -48,   48, 48*2+1, PF_INT   | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_SEMITONES, NULL, "transpose", "Transpose" },
    { 0,       -100,  100,      0, PF_INT   | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_CENTS, NULL, "detune", "Detune" },
    { 32,     0.707,   32,      0, PF_FLOAT | PF_SCALE_GAIN   | PF_CTL_KNOB | PF_UNIT_COEF, NULL, "maxres", "Max. Resonance" },
    { biquad_filter_module::mode_6db_bp,
      biquad_filter_module::mode_12db_lp,
      biquad_filter_module::mode_count - 1,
                                1, PF_ENUM  | PF_CTL_COMBO | PF_PROP_GRAPH, filter_choices, "mode", "Mode" },
    { 20,         1,  2000,    20, PF_FLOAT | PF_SCALE_LOG    | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "inertia", "Portamento time"},
    BYPASS_AND_LEVEL_PARAMS
    METERING_PARAMS
    {}
};

CALF_PLUGIN_INFO(filterclavier) = { 0x849f, "Filterclavier", "Calf Filterclavier", "Calf Studio Gear", calf_plugins::calf_copyright_info, "FilterPlugin" };

////////////////////////////////////////////////////////////////////////////

CALF_PORT_NAMES(envelopefilter) = {"In L", "In R", "Out L", "Out R"};

CALF_PORT_PROPS(envelopefilter) = {
    { 0,           0,           1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "bypass", "Bypass" },
    { 1,           0.015625,    64,    0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB | PF_PROP_NOBOUNDS, NULL, "level_in", "Input Gain" },
    { 2,           0.015625,    64,    0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB | PF_PROP_NOBOUNDS, NULL, "level_out", "Output Gain" },
    METERING_PARAMS
    { 0.85,        0,           1,     0,  PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB , NULL, "mix", "Mix" },
    { 2,     0.707,   32,    0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_COEF, NULL, "res", "Resonance" },
    { biquad_filter_module::mode_12db_bp,
      biquad_filter_module::mode_12db_lp,
      biquad_filter_module::mode_count - 1,
                                1,  PF_ENUM | PF_CTL_COMBO, filter_choices, "mode", "Mode" },
    { 20.f, 1.0f, 500.f,  0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "attack", "Attack" },
    { 200.f,10.f, 5000.f, 0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "release", "Release" },
    { 3000,   10,20000,   0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ | PF_PROP_GRAPH, NULL, "upper", "Upper" },
    { 80,     10,20000,   0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ | PF_PROP_GRAPH, NULL, "lower", "Lower" },
    { 1,0.015625,   64,   0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB | PF_PROP_NOBOUNDS, NULL, "gain", "Activation" },
    { 0,           0,           1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "sidechain_enable", "Sidechain" },
    { 0.,         -1,           1,     0,  PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB , NULL, "response", "Response" },
    {}
};

CALF_PLUGIN_INFO(envelopefilter) = { 0x8432, "EnvelopeFilter", "Calf Envelope Filter", "Calf Studio Gear", calf_plugins::calf_copyright_info, "FilterPlugin" };

////////////////////////////////////////////////////////////////////////////

CALF_PORT_NAMES(vintage_delay) = {"In L", "In R", "Out L", "Out R"};

const char *vintage_delay_mixmodes[] = {
    "Stereo",
    "Ping-Pong",
    "L then R",
    "R then L",
};

const char *vintage_delay_fbmodes[] = {
    "Plain",
    "Tape",
    "Old Tape",
};

const char *vintage_delay_fragmentation[] = {
    "Repeating",
    "Pattern",
};

CALF_PORT_PROPS(vintage_delay) = {
    { 1,          0,     1, 0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "on", "Active" },
    { 1,           0.015625,    64,    0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB | PF_PROP_NOBOUNDS, NULL, "level_in", "Input Gain" }, \
    { 1,           0.015625,    64,    0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB | PF_PROP_NOBOUNDS, NULL, "level_out", "Output Gain" },
    METERING_PARAMS
    {  4,        1,    16,    1, PF_INT | PF_SCALE_LINEAR | PF_CTL_FADER, NULL, "subdiv", "Subdivide"},
    {  3,        1,    16,    1, PF_INT | PF_SCALE_LINEAR | PF_CTL_FADER, NULL, "time_l", "Time L"},
    {  5,        1,    16,    1, PF_INT | PF_SCALE_LINEAR | PF_CTL_FADER, NULL, "time_r", "Time R"},
    { 0.5,       0,    1,     0, PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB, NULL, "feedback", "Feedback" },
    { 0.25,      0,    4,   100, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_COEF, NULL, "amount", "Wet" },
    { 1,         0,    3,     0, PF_ENUM | PF_CTL_COMBO, vintage_delay_mixmodes, "mix_mode", "Mix mode" },
    { 1,         0,    2,     0, PF_ENUM | PF_CTL_COMBO, vintage_delay_fbmodes, "medium", "Medium" },
    { 1.0,       0,    4,     0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_NOBOUNDS, NULL, "dry", "Dry" },
    { 1.0,      -1,    1,     0, PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB , NULL, "width", "Stereo Width" },
    { 0,         0,    1,     0, PF_ENUM | PF_CTL_COMBO, vintage_delay_fragmentation, "fragmentation", "Fragmentation" },
    { 4,         1,    8,     1, PF_INT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_COEF, NULL, "pbeats", "Pattern Beats" },
    { 4,         1,    8,     1, PF_INT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_COEF, NULL, "pfrag", "Pattern Fragmentation" },
    PERIODICAL_DEFINITIONS(0)
    {}
};

CALF_PLUGIN_INFO(vintage_delay) = { 0x8482, "VintageDelay", "Calf Vintage Delay", "Calf Studio Gear", calf_plugins::calf_copyright_info, "DelayPlugin" };

////////////////////////////////////////////////////////////////////////////
CALF_PORT_NAMES(comp_delay) = { "In L", "In R", "Out L", "Out R" };

CALF_PORT_PROPS(comp_delay) = {
    {  0,        0,    10,    1, PF_INT | PF_SCALE_LINEAR | PF_CTL_KNOB, NULL, "distance_mm", "Distance (mm)"},
    {  0,        0,    100,   1, PF_INT | PF_SCALE_LINEAR | PF_CTL_KNOB, NULL, "distance_cm", "Distance (cm)"},
    {  0,        0,    100,   1, PF_INT | PF_SCALE_LINEAR | PF_CTL_KNOB, NULL, "distance_m", "Distance (m)"},
    {  0.000244140625,        0.000244140625,    1,     0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_NOBOUNDS, NULL, "dry", "Dry Amount" },
    { 1.0,       0.000244140625,    1,     0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_NOBOUNDS, NULL, "wet", "Wet Amount" },
    {  20,       -50,  50,    1, PF_INT | PF_SCALE_LINEAR | PF_CTL_KNOB, NULL, "temp", "Temperature °C"},
    BYPASS_AND_LEVEL_PARAMS
    METERING_PARAMS
    {}
};
CALF_PLUGIN_INFO(comp_delay) = { 0x8485, "CompensationDelay", "Calf Compensation Delay Line", "Calf Studio Gear", calf_plugins::calf_copyright_info, "DelayPlugin" };

////////////////////////////////////////////////////////////////////////////

CALF_PORT_NAMES(reverse_delay) = {"In L", "In R", "Out L", "Out R"};

CALF_PORT_PROPS(reverse_delay) = {
    { 120,      30,    300,   1, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_BPM, NULL, "bpm", "Tempo" },
    { 120,       1,    300,   1, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_BPM | PF_SYNC_BPM, NULL, "bpm_host", "Host BPM" },
    {  4,        1,    16,    1, PF_INT | PF_SCALE_LINEAR | PF_CTL_FADER, NULL, "subdiv", "Subdivide"},
    {  5,        1,    16,    1, PF_INT | PF_SCALE_LINEAR | PF_CTL_FADER, NULL, "time_l", "Time L"},
    {  5,        1,    16,    1, PF_INT | PF_SCALE_LINEAR | PF_CTL_FADER, NULL, "time_r", "Time R"},
    { 0.5,       0,    1,     0, PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB, NULL, "feedback", "Feedback" },
    { 0,        -1,    1,     0, PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_NOBOUNDS, NULL, "amount", "Dry/Wet" },
    { 0,       0,    1,     0, PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB , NULL, "width", "Stereo Width" },
    { 0,         0,    1,     0, PF_BOOL | PF_CTL_TOGGLE, NULL, "sync", "Sync BPM" },
    { 0,         0,    1,     0, PF_INT | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "sync_led_l", "Left" },
    { 0,         0,    1,     0, PF_INT | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "sync_led_r", "Right" },
    { 0,         0,    1,     2, PF_BOOL | PF_CTL_BUTTON , NULL, "reset", "Reset" },
    { 0.5,       0,    1,     0, PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB, NULL, "window", "Window" },
    BYPASS_AND_LEVEL_PARAMS
    METERING_PARAMS
    {}
};

CALF_PLUGIN_INFO(reverse_delay) = { 0x8482, "ReverseDelay", "Calf Reverse Delay", "Calf Studio Gear", calf_plugins::calf_copyright_info, "DelayPlugin" };

////////////////////////////////////////////////////////////////////////////

CALF_PORT_NAMES(rotary_speaker) = {"In L", "In R", "Out L", "Out R"};

const char *rotary_speaker_speed_names[] = { "Off", "Chorale", "Tremolo", "HoldPedal", "ModWheel", "Manual" };

CALF_PORT_PROPS(rotary_speaker) = {
    { 5,         0,  5, 1.01, PF_ENUM | PF_CTL_COMBO, rotary_speaker_speed_names, "vib_speed", "Speed Mode" },
    { 0.5,        0,    1,    0, PF_FLOAT | PF_CTL_KNOB | PF_SCALE_PERC, NULL, "spacing", "Tap Spacing" },
    { 0.5,        0,    1,    0, PF_FLOAT | PF_CTL_KNOB | PF_SCALE_PERC, NULL, "shift", "Tap Offset" },
    { 0.6,       0,    1,    0, PF_FLOAT | PF_CTL_KNOB | PF_SCALE_PERC, NULL, "mod_depth", "FM Depth" },
    { 36,       10,   600,    0, PF_FLOAT | PF_DIGIT_1 | PF_CTL_KNOB | PF_SCALE_LOG | PF_UNIT_RPM, NULL, "treble_speed", "Treble Motor" },
    { 30,      10,   600,    0, PF_FLOAT | PF_DIGIT_1 | PF_CTL_KNOB | PF_SCALE_LOG | PF_UNIT_RPM, NULL, "bass_speed", "Bass Motor" },
    { 0.7,        0,    1,  101, PF_FLOAT | PF_CTL_KNOB | PF_SCALE_PERC, NULL, "mic_distance", "Mic Distance" },
    { 0.3,        0,    1,  101, PF_FLOAT | PF_CTL_KNOB | PF_SCALE_PERC, NULL, "reflection", "Reflection" },
    { 0.6,        0,           1,     0,  PF_FLOAT | PF_CTL_KNOB | PF_SCALE_PERC, NULL, "am_depth", "AM Depth" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_CTL_KNOB | PF_SCALE_PERC, NULL, "test", "Test" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "meter_l", "Low rotor" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "meter_h", "High rotor" },
    { 0,           0,           1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "bypass", "Bypass" }, \
    { 1,           0.015625,    64,    0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB | PF_PROP_NOBOUNDS, NULL, "level_in", "Input Gain" }, \
    { 2,           0.015625,    64,    0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB | PF_PROP_NOBOUNDS, NULL, "level_out", "Output Gain" },
    METERING_PARAMS
    {}
};

CALF_PLUGIN_INFO(rotary_speaker) = { 0x8483, "RotarySpeaker", "Calf Rotary Speaker", "Calf Studio Gear", calf_plugins::calf_copyright_info, "SimulatorPlugin" };

////////////////////////////////////////////////////////////////////////////

CALF_PORT_NAMES(multichorus) = {"In L", "In R", "Out L", "Out R"};

CALF_PORT_PROPS(multichorus) = {
    { 5,        0.1,  10,   0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC | PF_PROP_GRAPH, NULL, "min_delay", "Min Delay" },
    { 6,        0.1,  10,   0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC| PF_PROP_GRAPH, NULL, "mod_depth", "Mod Depth" },
    { 0.1,     0.01,  20,   0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ| PF_PROP_GRAPH, NULL, "mod_rate", "Mod Rate" },
    { 180,        0, 360,  91, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_DEG, NULL, "stereo", "Stereo Phase" },
    { 4,          1,   8,   8, PF_INT | PF_SCALE_LINEAR | PF_CTL_FADER, NULL, "voices", "Voices"},
    { 64,         0, 360,  91, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_DEG, NULL, "vphase", "Inter-Voice Phase" },
    { 0.5,        0,   4,   0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_NOBOUNDS, NULL, "amount", "Amount" },
    { 0.5,        0,   4,     0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_NOBOUNDS, NULL, "dry", "Dry Amount" },
    { 100,       10,20000,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ | PF_PROP_GRAPH, NULL, "freq", "Center Frq 1" },
    { 5000,      10,20000,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ | PF_PROP_GRAPH, NULL, "freq2", "Center Frq 2" },
    { 0.125,  0.125,   8,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_COEF, NULL, "q", "Q" },
    { 0.75,       0,    1,     0, PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB, NULL, "overlap", "Overlap" },
    { 1,          0,     1, 0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "on", "Active" },
    { 1,           0.015625,    64,    0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB | PF_PROP_NOBOUNDS, NULL, "level_in", "Input Gain" }, \
    { 1,           0.015625,    64,    0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB | PF_PROP_NOBOUNDS, NULL, "level_out", "Output Gain" },
    METERING_PARAMS
    { 1,           0,           1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "lfo", "LFO" }, \
};

CALF_PLUGIN_INFO(multichorus) = { 0x8501, "MultiChorus", "Calf Multi Chorus", "Calf Studio Gear", calf_plugins::calf_copyright_info, "ModulatorPlugin" };

////////////////////////////////////////////////////////////////////////////

CALF_PORT_NAMES(monocompressor) = {"In", "Out"};

const char *monocompressor_detection_names[] = { "RMS", "Peak" };
const char *monocompressor_stereo_link_names[] = { "Average", "Maximum" };

CALF_PORT_PROPS(monocompressor) = {
    { 0,      0,  1,    0, PF_BOOL | PF_CTL_TOGGLE, NULL, "bypass", "Bypass" },
    { 1,           0.015625,    64,    0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB | PF_PROP_NOBOUNDS, NULL, "level_in", "Input" },
    { 0,      0,  1,    0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "meter_in", "Input" },
    { 0,      0,  1,    0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "meter_out", "Output" },
    { 0,      0,  1,    0, PF_BOOL | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "clip_in", "0dB-In" },
    { 0,      0,  1,    0, PF_BOOL | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "clip_out", "0dB-Out" },
    { 0.125,      0.000976563, 1,    0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DBFS, NULL, "threshold", "Threshold" },
    { 2,      1, 20,  21, PF_FLOAT | PF_SCALE_LOG_INF | PF_CTL_KNOB | PF_UNIT_COEF, NULL, "ratio", "Ratio" },
    { 20,     0.01, 2000, 0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "attack", "Attack" },
    { 250,    0.01, 2000, 0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "release", "Release" },
    { 1,      1, 64,    0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "makeup", "Makeup Gain" },
    { 2.828427125,      1,  8,   0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "knee", "Knee" },
    //{ 0,      0,  1,    0, PF_ENUM | PF_CTL_COMBO, monocompressor_detection_names, "detection", "Detection" },
    //{ 0,      0,  1,    0, PF_ENUM | PF_CTL_COMBO, monocompressor_stereo_link_names, "stereo_link", "Stereo Link" },
    { 0, 0.03125, 1,    0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_CTLO_REVERSE | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL| PF_PROP_GRAPH, NULL, "compression", "Reduction" },
    { 1,         0,           1,     0,  PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB , NULL, "mix", "Mix" },
    {}
};

CALF_PLUGIN_INFO(monocompressor) = { 0x8577, "MonoCompressor", "Calf Mono Compressor", "Calf Studio Gear", calf_plugins::calf_copyright_info, "CompressorPlugin" };

////////////////////////////////////////////////////////////////////////////

CALF_PORT_NAMES(compressor) = {"In L", "In R", "Out L", "Out R"};

const char *compressor_detection_names[] = { "RMS", "Peak" };
const char *compressor_stereo_link_names[] = { "Average", "Maximum" };

CALF_PORT_PROPS(compressor) = {
    { 0,      0,  1,    0, PF_BOOL | PF_CTL_TOGGLE, NULL, "bypass", "Bypass" },
    { 1,           0.015625,    64,    0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB | PF_PROP_NOBOUNDS, NULL, "level_in", "Input" },
    { 0,      0,  1,    0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "meter_in", "Input" },
    { 0,      0,  1,    0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "meter_out", "Output" },
    { 0,      0,  1,    0, PF_BOOL | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "clip_in", "0dB-In" },
    { 0,      0,  1,    0, PF_BOOL | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "clip_out", "0dB-Out" },
    { 0.125,      0.000976563, 1,    0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DBFS, NULL, "threshold", "Threshold" },
    { 2,      1, 20,  21, PF_FLOAT | PF_SCALE_LOG_INF | PF_CTL_KNOB | PF_UNIT_COEF, NULL, "ratio", "Ratio" },
    { 20,     0.01, 2000, 0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "attack", "Attack" },
    { 250,    0.01, 2000, 0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "release", "Release" },
    { 1,      1, 64,    0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "makeup", "Makeup Gain" },
    { 2.828427125,      1,  8,   0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "knee", "Knee" },
    { 0,      0,  1,    0, PF_ENUM | PF_CTL_COMBO, compressor_detection_names, "detection", "Detection" },
    { 0,      0,  1,    0, PF_ENUM | PF_CTL_COMBO, compressor_stereo_link_names, "stereo_link", "Stereo Link" },
    { 0, 0.03125, 1,    0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_CTLO_REVERSE | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL| PF_PROP_GRAPH, NULL, "compression", "Reduction" },
    { 1,         0,           1,     0,  PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB , NULL, "mix", "Mix" },
    {}
};

CALF_PLUGIN_INFO(compressor) = { 0x8502, "Compressor", "Calf Compressor", "Calf Studio Gear", calf_plugins::calf_copyright_info, "CompressorPlugin" };

////////////////////////////////////////////////////////////////////////////

CALF_PORT_NAMES(sidechaincompressor) = {"In L", "In R", "Out L", "Out R"};

const char *sidechaincompressor_detection_names[] = { "RMS", "Peak" };
const char *sidechaincompressor_stereo_link_names[] = { "Average", "Maximum" };
const char *sidechaincompressor_mode_names[] = {"Wideband (F1:off / F2:off)",
                                                "Deesser wide (F1:Bell / F2:HP)",
                                                "Deesser split (F1:off / F2:HP)",
                                                "Derumbler wide (F1:LP / F2:Bell)",
                                                "Derumbler split (F1:LP / F2:off)",
                                                "Weighted #1 (F1:Shelf / F2:Shelf)",
                                                "Weighted #2 (F1:Shelf / F2:Bell)",
                                                "Weighted #3 (F1:Bell / F2:Shelf)",
                                                "Bandpass #1 (F1:BP / F2:off)",
                                                "Bandpass #2 (F1:HP / F2:LP)"};
const char *sidechaincompressor_filter_choices[] = { "12dB", "24dB", "36dB"};


CALF_PORT_PROPS(sidechaincompressor) = {
    { 0,      0,  1,    0, PF_BOOL | PF_CTL_TOGGLE, NULL, "bypass", "Bypass" },
    { 1,           0.015625,    64,    0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB | PF_PROP_NOBOUNDS, NULL, "level_in", "Input" },
    { 0,      0,  1,    0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "meter_in", "Input" },
    { 0,      0,  1,    0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "meter_out", "Output" },
    { 0,      0,  1,    0, PF_BOOL | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "clip_in", "0dB-In" },
    { 0,      0,  1,    0, PF_BOOL | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "clip_out", "0dB-Out" },
    { 0.125,      0.000976563, 1,    0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DBFS, NULL, "threshold", "Threshold" },
    { 2,      1, 20,  21, PF_FLOAT | PF_SCALE_LOG_INF | PF_CTL_KNOB | PF_UNIT_COEF, NULL, "ratio", "Ratio" },
    { 20,     0.01, 2000, 0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "attack", "Attack" },
    { 250,    0.01, 2000, 0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "release", "Release" },
    { 1,      1, 64,    0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "makeup", "Makeup Gain" },
    { 2.828427125,      1,  8,   0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "knee", "Knee" },
    { 0,      0,  1,    0, PF_ENUM | PF_CTL_COMBO, sidechaincompressor_detection_names, "detection", "Detection" },
    { 1,      0,  1,    0, PF_ENUM | PF_CTL_COMBO, sidechaincompressor_stereo_link_names, "stereo_link", "Stereo Link" },
    { 0, 0.03125, 1,    0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_CTLO_REVERSE | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL| PF_PROP_GRAPH, NULL, "compression", "Gain Reduction" },
    { 0,      0,  9,    0, PF_ENUM | PF_CTL_COMBO, sidechaincompressor_mode_names, "sc_mode", "S/C Mode" },
    { 250,    10,18000, 0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ | PF_PROP_GRAPH, NULL, "f1_freq", "F1 Freq" },
    { 4500,   10,18000, 0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL, "f2_freq", "F2 Freq" },
    { 1,      0.0625,  16, 0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "f1_level", "F1 Level" },
    { 1,      0.0625,  16, 0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "f2_level", "F2 Level" },
    { 0,      0,  1,    0, PF_BOOL | PF_CTL_TOGGLE, NULL, "sc_listen", "S/C-Listen" },
    { 0,      0,  1,    0, PF_BOOL | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "f1_active", "F1 Active" },
    { 0,      0,  1,    0, PF_BOOL | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "f2_active", "F2 Active" },
    { 0,      0,  1,    0, PF_BOOL | PF_CTL_TOGGLE, NULL, "sc_route", "S/C Route" },
    { 1,           0.015625,           64,    0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_NOBOUNDS, NULL, "sc_level", "S/C Level" },
    { 1,         0,           1,     0,  PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB , NULL, "mix", "Mix" },
    {}
};

CALF_PLUGIN_INFO(sidechaincompressor) = { 0x8517, "SidechainCompressor", "Calf Sidechain Compressor", "Calf Studio Gear", calf_plugins::calf_copyright_info, "CompressorPlugin" };

////////////////////////////////////////////////////////////////////////////
#define MULTI_BAND_COMP_PARAMS(band1, band2) \
    { 0.25,      0.000976563, 1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DBFS, NULL, "threshold" #band1, "Threshold " #band2 }, \
    { 2,           1,           20,    21, PF_FLOAT | PF_SCALE_LOG_INF | PF_CTL_KNOB | PF_UNIT_COEF, NULL, "ratio" #band1, "Ratio " #band2 }, \
    { 150,          0.01,        2000,  0,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "attack" #band1, "Attack " #band2 }, \
    { 300,         0.01,        2000,  0,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "release" #band1, "Release " #band2 }, \
    { 1,           1,           64,    0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "makeup" #band1, "Makeup " #band2 }, \
    { 2.828427125, 1,           8,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "knee" #band1, "Knee " #band2 }, \
    { 0,           0,           1,     0,  PF_ENUM | PF_CTL_COMBO, multibandcompressor_detection_names, "detection" #band1, "Detection " #band2 }, \
    { 1,           0.0625,     1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_CTLO_REVERSE | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL| PF_PROP_GRAPH, NULL, "compression" #band1, "Gain Reduction " #band2 }, \
    { 0,           0,           1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "output" #band1, "Output " #band2 }, \
    { 1,           0,           1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "bypass" #band1, "Bypass " #band2 }, \
    { 0,           0,           1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "solo" #band1, "Solo " #band2 },

CALF_PORT_NAMES(multibandcompressor) = {"In L", "In R", "Out L", "Out R"};

const char *multibandcompressor_detection_names[] = { "RMS", "Peak" };

CALF_PORT_PROPS(multibandcompressor) = {
    BYPASS_AND_LEVEL_PARAMS
    METERING_PARAMS
    { 120,         10,          20000, 0,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ | PF_PROP_GRAPH, NULL, "freq0", "Split 1/2" },
    { 1000,        10,          20000, 0,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ | PF_PROP_GRAPH, NULL, "freq1", "Split 2/3" },
    { 6000,        10,          20000, 0,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ | PF_PROP_GRAPH, NULL, "freq2", "Split 3/4" },
    { 1,           0,           1,     0, PF_ENUM | PF_CTL_COMBO, mb_crossover_filter_choices, "mode", "Filter Mode" },
    MULTI_BAND_COMP_PARAMS(0,1)
    MULTI_BAND_COMP_PARAMS(1,2)
    MULTI_BAND_COMP_PARAMS(2,3)
    MULTI_BAND_COMP_PARAMS(3,4)
    { 0,           0,           3,     0,  PF_INT | PF_SCALE_LINEAR,  NULL, "notebook", "Notebook" },
    {}
};

CALF_PLUGIN_INFO(multibandcompressor) = { 0x8516, "MultibandCompressor", "Calf Multiband Compressor", "Calf Studio Gear", calf_plugins::calf_copyright_info, "CompressorPlugin" };

////////////////////////////////////////////////////////////////////////////

CALF_PORT_NAMES(deesser) = {"In L", "In R", "Out L", "Out R"};

const char *deesser_detection_names[] = { "RMS", "Peak" };
const char *deesser_mode_names[] = { "Wide", "Split" };


CALF_PORT_PROPS(deesser) = {
    { 0,      0,  1,    0, PF_BOOL | PF_CTL_TOGGLE, NULL, "bypass", "Bypass" },
    { 0,      0,  1,    0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "detected", "Detected" },
    { 0, 0.0625, 1,    0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_CTLO_REVERSE | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL| PF_PROP_GRAPH, NULL, "compression", "Gain Reduction" },
    { 0,      0,  1,    0, PF_BOOL | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "detected_led", "Active" },
    { 0,      0,  1,    0, PF_BOOL | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "clip_out", "Out" },
    { 0,      0,  1,    0, PF_ENUM | PF_CTL_COMBO, deesser_detection_names, "detection", "Detection" },
    { 0,      0,  1,    0, PF_ENUM | PF_CTL_COMBO, deesser_mode_names, "mode", "Mode" },
    { 0.125,  0.000976563, 1,    0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DBFS, NULL, "threshold", "Threshold" },
    { 3,      1, 20,  21, PF_FLOAT | PF_SCALE_LOG_INF | PF_CTL_KNOB | PF_UNIT_COEF, NULL, "ratio", "Ratio" },
    { 15,     1, 100,   1, PF_INT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_COEF, NULL, "laxity", "Laxity" },
    { 1,      1, 64,    0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "makeup", "Makeup" },

    { 6000,   10,   18000, 0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ | PF_PROP_GRAPH, NULL, "f1_freq", "Split" },
    { 4500,   10,   18000, 0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL, "f2_freq", "Peak" },
    { 1,      0.0625,  16, 0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "f1_level", "Gain" },
    { 4,      0.0625,  16, 0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DBFS, NULL, "f2_level", "Level" },
    { 1,      0.1,     100,1, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_COEF, NULL, "f2_q", "Peak Q" },
    { 0,      0,  1,    0, PF_BOOL | PF_CTL_TOGGLE, NULL, "sc_listen", "S/C-Listen" },
    {}
};

CALF_PLUGIN_INFO(deesser) = { 0x8515, "Deesser", "Calf Deesser", "Calf Studio Gear", calf_plugins::calf_copyright_info, "CompressorPlugin" };

////////////////////////////////////////////////////////////////////////////

CALF_PORT_NAMES(gate) = {"In L", "In R", "Out L", "Out R"};

const char *gate_detection_names[] = { "RMS", "Peak" };
const char *gate_stereo_link_names[] = { "Average", "Maximum" };

CALF_PORT_PROPS(gate) = {
    { 0,      0,  1,    0, PF_BOOL | PF_CTL_TOGGLE, NULL, "bypass", "Bypass" },
    { 1,           0.015625,    64,    0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB | PF_PROP_NOBOUNDS, NULL, "level_in", "Input" },
    { 0,      0,  1,    0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "meter_in", "Input" },
    { 0,      0,  1,    0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "meter_out", "Output" },
    { 0,      0,  1,    0, PF_BOOL | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "clip_in", "0dB-In" },
    { 0,      0,  1,    0, PF_BOOL | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "clip_out", "0dB-Out" },
    { 0.06125,   0.000015849, 1, 0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "range", "Max Gain Reduction" },
    { 0.125,      0.000976563, 1,    0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DBFS, NULL, "threshold", "Threshold" },
    { 2,      1, 20,  21, PF_FLOAT | PF_SCALE_LOG_INF | PF_CTL_KNOB | PF_UNIT_COEF, NULL, "ratio", "Ratio" },
    { 20,     0.01, 2000, 0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "attack", "Attack" },
    { 250,    0.01, 2000, 0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "release", "Release" },
    { 1,      1, 64,    0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "makeup", "Makeup Gain" },
    { 2.828427125,      1,  8,   0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "knee", "Knee" },
    { 0,      0,  1,    0, PF_ENUM | PF_CTL_COMBO, gate_detection_names, "detection", "Detection" },
    { 0,      0,  1,    0, PF_ENUM | PF_CTL_COMBO, gate_stereo_link_names, "stereo_link", "Stereo Link" },
    { 0, 0.03125, 1,    0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_CTLO_REVERSE | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL| PF_PROP_GRAPH, NULL, "gating", "Gating" },
    {}
};

CALF_PLUGIN_INFO(gate) = { 0x8503, "Gate", "Calf Gate", "Calf Studio Gear", calf_plugins::calf_copyright_info, "ExpanderPlugin" };

////////////////////////////////////////////////////////////////////////////

CALF_PORT_NAMES(sidechaingate) = {"In L", "In R", "Out L", "Out R"};

const char *sidechaingate_detection_names[] = { "RMS", "Peak" };
const char *sidechaingate_stereo_link_names[] = { "Average", "Maximum" };
const char *sidechaingate_mode_names[] = {"Wideband (F1:off / F2:off)",
                                                "High gate wide (F1:Bell / F2:HP)",
                                                "High gate split (F1:off / F2:HP)",
                                                "Low Gate wide (F1:LP / F2:Bell)",
                                                "Low gate split (F1:LP / F2:off)",
                                                "Weighted #1 (F1:Shelf / F2:Shelf)",
                                                "Weighted #2 (F1:Shelf / F2:Bell)",
                                                "Weighted #3 (F1:Bell / F2:Shelf)",
                                                "Bandpass #1 (F1:BP / F2:off)",
                                                "Bandpass #2 (F1:HP / F2:LP)"};
const char *sidechaingate_filter_choices[] = { "12dB", "24dB", "36dB"};


CALF_PORT_PROPS(sidechaingate) = {
    { 0,      0,  1,    0, PF_BOOL | PF_CTL_TOGGLE, NULL, "bypass", "Bypass" },
    { 1,           0.015625,    64,    0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB | PF_PROP_NOBOUNDS, NULL, "level_in", "Input" },
    { 0,      0,  1,    0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "meter_in", "Input" },
    { 0,      0,  1,    0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "meter_out", "Output" },
    { 0,      0,  1,    0, PF_BOOL | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "clip_in", "0dB-In" },
    { 0,      0,  1,    0, PF_BOOL | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "clip_out", "0dB-Out" },
    { 0.06125,   0.000015849, 1, 0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "range", "Max Gain Reduction" },
    { 0.125,      0.000976563, 1,    0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DBFS, NULL, "threshold", "Threshold" },
    { 2,      1, 20,  21, PF_FLOAT | PF_SCALE_LOG_INF | PF_CTL_KNOB | PF_UNIT_COEF, NULL, "ratio", "Ratio" },
    { 20,     0.01, 2000, 0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "attack", "Attack" },
    { 250,    0.01, 2000, 0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "release", "Release" },
    { 1,      1, 64,    0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "makeup", "Makeup Gain" },
    { 2.828427125,      1,  8,   0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "knee", "Knee" },
    { 0,      0,  1,    0, PF_ENUM | PF_CTL_COMBO, sidechaingate_detection_names, "detection", "Detection" },
    { 1,      0,  1,    0, PF_ENUM | PF_CTL_COMBO, sidechaingate_stereo_link_names, "stereo_link", "Stereo Link" },
    { 0, 0.03125, 1,    0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_CTLO_REVERSE | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL| PF_PROP_GRAPH, NULL, "gating", "Gating" },
    { 0,      0,  9,    0, PF_ENUM | PF_CTL_COMBO, sidechaingate_mode_names, "sc_mode", "S/C Mode" },
    { 250,    10,18000, 0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ | PF_PROP_GRAPH, NULL, "f1_freq", "F1 Freq" },
    { 4500,   10,18000, 0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL, "f2_freq", "F2 Freq" },
    { 1,      0.0625,  16, 0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "f1_level", "F1 Level" },
    { 1,      0.0625,  16, 0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "f2_level", "F2 Level" },
    { 0,      0,  1,    0, PF_BOOL | PF_CTL_TOGGLE, NULL, "sc_listen", "S/C-Listen" },
    { 0,      0,  1,    0, PF_BOOL | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "f1_active", "F1 Active" },
    { 0,      0,  1,    0, PF_BOOL | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "f2_active", "F2 Active" },
    { 0,      0,  1,    0, PF_BOOL | PF_CTL_TOGGLE, NULL, "sc_route", "S/C Route" },
    { 1,           0.015625,           64,    0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_NOBOUNDS, NULL, "sc_level", "S/C Level" },
    {}
};

CALF_PLUGIN_INFO(sidechaingate) = { 0x8504, "SidechainGate", "Calf Sidechain Gate", "Calf Studio Gear", calf_plugins::calf_copyright_info, "ExpanderPlugin" };

////////////////////////////////////////////////////////////////////////////
#define MULTI_BAND_GATE_PARAMS(band1, band2) \
    { 0.06125,   0.000015849, 1, 0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "range" #band1, "Reduction " #band2 }, \
    { 0.25,      0.000976563, 1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DBFS, NULL, "threshold" #band1, "Threshold " #band2 }, \
    { 2,           1,           20,    21, PF_FLOAT | PF_SCALE_LOG_INF | PF_CTL_KNOB | PF_UNIT_COEF, NULL, "ratio" #band1, "Ratio " #band2 }, \
    { 150,          0.01,        2000,  0,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "attack" #band1, "Attack " #band2 }, \
    { 300,         0.01,        2000,  0,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "release" #band1, "Release " #band2 }, \
    { 2,           1,           64,    0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "makeup" #band1, "Makeup " #band2 }, \
    { 2.828427125, 1,           8,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "knee" #band1, "Knee " #band2 }, \
    { 0,           0,           1,     0,  PF_ENUM | PF_CTL_COMBO, multibandcompressor_detection_names, "detection" #band1, "Detection " #band2 }, \
    { 1,           0.0625,     1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_CTLO_REVERSE | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL| PF_PROP_GRAPH, NULL, "gating" #band1, "Gating " #band2 }, \
    { 0,           0,           1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "output" #band1, "Output " #band2 }, \
    { 1,           0,           1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "bypass" #band1, "Bypass " #band2 }, \
    { 0,           0,           1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "solo" #band1, "Solo " #band2 },

CALF_PORT_NAMES(multibandgate) = {"In L", "In R", "Out L", "Out R"};

const char *multibandgate_detection_names[] = { "RMS", "Peak" };

CALF_PORT_PROPS(multibandgate) = {
    BYPASS_AND_LEVEL_PARAMS
    METERING_PARAMS
    { 120,         10,          20000, 0,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ | PF_PROP_GRAPH, NULL, "freq0", "Split 1/2" },
    { 1000,        10,          20000, 0,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ | PF_PROP_GRAPH, NULL, "freq1", "Split 2/3" },
    { 6000,        10,          20000, 0,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ | PF_PROP_GRAPH, NULL, "freq2", "Split 3/4" },
    { 1,      0,  1,    0, PF_ENUM | PF_CTL_COMBO, mb_crossover_filter_choices, "mode", "Filter Mode" },
    MULTI_BAND_GATE_PARAMS(0,1)
    MULTI_BAND_GATE_PARAMS(1,2)
    MULTI_BAND_GATE_PARAMS(2,3)
    MULTI_BAND_GATE_PARAMS(3,4)
    { 0,           0,           3,     0,  PF_INT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_COEF,  NULL, "notebook", "Notebook" },
    {}
};

CALF_PLUGIN_INFO(multibandgate) = { 0x8505, "MultibandGate", "Calf Multiband Gate", "Calf Studio Gear", calf_plugins::calf_copyright_info, "ExpanderPlugin" };

////////////////////////////////////////////////////////////////////////////

CALF_PORT_NAMES(limiter) = {"In L", "In R", "Out L", "Out R"};

CALF_PORT_PROPS(limiter) = {
    BYPASS_AND_LEVEL_PARAMS
    METERING_PARAMS
    { 1,      0.0625, 1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DBFS, NULL, "limit", "Limit" },
    { 5,         0.1,        10,  0,  PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "attack", "Lookahead" },
    { 50,         1,        1000,  0,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "release", "Release" },

    { 1,           0.125,     1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_CTLO_REVERSE | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL| PF_PROP_GRAPH, NULL, "att", "Attenuation" },

    { 1,           0,           1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "asc", "ASC" },

    { 0,           0,           1,     0,  PF_FLOAT | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "asc_led", "asc active" },

    { 0.5f,      0.f,         1.f,   0,  PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_GRAPH, NULL, "asc_coeff", "ASC Level" },
    { 1,           1,           4,   0,  PF_INT | PF_SCALE_LINEAR | PF_UNIT_COEF | PF_CTL_KNOB, NULL, "oversampling", "Oversampling" },
    { 1,           0,           1,   0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "auto_level", "Auto-level" },
    {}
};

CALF_PLUGIN_INFO(limiter) = { 0x8521, "Limiter", "Calf Limiter", "Calf Studio Gear", calf_plugins::calf_copyright_info, "LimiterPlugin" };

////////////////////////////////////////////////////////////////////////////

CALF_PORT_NAMES(multibandlimiter) = {"In L", "In R", "Out L", "Out R"};

CALF_PORT_PROPS(multibandlimiter) = {
    BYPASS_AND_LEVEL_PARAMS
    METERING_PARAMS
    { 100,         10,          20000, 0,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ | PF_PROP_GRAPH, NULL, "freq0", "Split 1/2" },
    { 750,        10,          20000, 0,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ | PF_PROP_GRAPH, NULL, "freq1", "Split 2/3" },
    { 5000,        10,          20000, 0,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ | PF_PROP_GRAPH, NULL, "freq2", "Split 3/4" },

    { 1,      0,  1,    0, PF_ENUM | PF_CTL_COMBO, mb_crossover_filter_choices, "mode", "Filter Mode" },

    { 1,      0.0625, 1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DBFS, NULL, "limit", "Limit" },
    { 4,         0.1,        10,  0,  PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "attack", "Lookahead" },
    { 30,         1,        1000,  0,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "release", "Release" },
    { 0,           0,           1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "minrel", "Min Release" },

    { 1,           0.125,     1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_CTLO_REVERSE | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL| PF_PROP_GRAPH, NULL, "att0", "Low" },
    { 1,           0.125,     1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_CTLO_REVERSE | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL| PF_PROP_GRAPH, NULL, "att1", "LMid" },
    { 1,           0.125,     1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_CTLO_REVERSE | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL| PF_PROP_GRAPH, NULL, "att2", "HMid" },
    { 1,           0.125,     1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_CTLO_REVERSE | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL| PF_PROP_GRAPH, NULL, "att3", "Hi" },

    { 0.f,      -1.f,         1.f,   0,  PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_GRAPH, NULL, "weight0", "Weight 1" },
    { 0.f,      -1.f,         1.f,   0,  PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_GRAPH, NULL, "weight1", "Weight 2" },
    { 0.f,      -1.f,         1.f,   0,  PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_GRAPH, NULL, "weight2", "Weight 3" },
    { 0.f,      -1.f,         1.f,   0,  PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_GRAPH, NULL, "weight3", "Weight 4" },

    { 0.5f,      -1.f,         1.f,   0,  PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_GRAPH, NULL, "release0", "Release 1" },
    { 0.2f,      -1.f,         1.f,   0,  PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_GRAPH, NULL, "release1", "Release 2" },
    { -0.2f,      -1.f,         1.f,   0,  PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_GRAPH, NULL, "release2", "Release 3" },
    { -0.5f,      -1.f,         1.f,   0,  PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_GRAPH, NULL, "release3", "Release 4" },

    { 0,           0,           1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "solo0", "Solo 1" },
    { 0,           0,           1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "solo1", "Solo 2" },
    { 0,           0,           1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "solo2", "Solo 3" },
    { 0,           0,           1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "solo3", "Solo 4" },

    { 1,         0.f,        1000,  0,  PF_FLOAT | PF_UNIT_MSEC | PF_PROP_OUTPUT, NULL, "effrelease0", "Effectively Release 1" },
    { 1,         0.f,        1000,  0,  PF_FLOAT | PF_UNIT_MSEC | PF_PROP_OUTPUT, NULL, "effrelease1", "Effectively Release 2" },
    { 1,         0.f,        1000,  0,  PF_FLOAT | PF_UNIT_MSEC | PF_PROP_OUTPUT, NULL, "effrelease2", "Effectively Release 3" },
    { 1,         0.f,        1000,  0,  PF_FLOAT | PF_UNIT_MSEC | PF_PROP_OUTPUT, NULL, "effrelease3", "Effectively Release 4" },

    { 1,           0,           1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "asc", "ASC" },

    { 0,           0,           1,     0,  PF_FLOAT | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "asc_led", "asc active" },

    { 0.5f,      0.f,         1.f,   0,  PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_GRAPH, NULL, "asc_coeff", "ASC Level" },

    { 1,           1,           4,   0,  PF_INT | PF_SCALE_LINEAR | PF_UNIT_COEF | PF_CTL_KNOB, NULL, "oversampling", "Oversampling" },
    { 1,           0,           1,   0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "auto_level", "Auto-level" },

    {}
};

CALF_PLUGIN_INFO(multibandlimiter) = { 0x8520, "MultibandLimiter", "Calf Multiband Limiter", "Calf Studio Gear", calf_plugins::calf_copyright_info, "LimiterPlugin" };

////////////////////////////////////////////////////////////////////////////

CALF_PORT_NAMES(sidechainlimiter) = {"In L", "In R", "Side L", "Side R", "Out L", "Out R"};

CALF_PORT_PROPS(sidechainlimiter) = {
    BYPASS_AND_LEVEL_PARAMS
    METERING_PARAMS
    { 0,           0,           1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "meter_scL", "Meter S/C L" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "meter_scR", "Meter S/C R" },
    { 100,         10,          20000, 0,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ | PF_PROP_GRAPH, NULL, "freq0", "Split 1/2" },
    { 750,        10,          20000, 0,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ | PF_PROP_GRAPH, NULL, "freq1", "Split 2/3" },
    { 5000,        10,          20000, 0,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ | PF_PROP_GRAPH, NULL, "freq2", "Split 3/4" },

    { 1,      0,  1,    0, PF_ENUM | PF_CTL_COMBO, mb_crossover_filter_choices, "mode", "Filter Mode" },

    { 1,      0.0625, 1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DBFS, NULL, "limit", "Limit" },
    { 4,         0.1,        10,  0,  PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "attack", "Lookahead" },
    { 30,         1,        1000,  0,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "release", "Release" },
    { 0,           0,           1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "minrel", "Min Release" },

    { 1,           0.125,     1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_CTLO_REVERSE | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL| PF_PROP_GRAPH, NULL, "att0", "Low" },
    { 1,           0.125,     1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_CTLO_REVERSE | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL| PF_PROP_GRAPH, NULL, "att1", "LMid" },
    { 1,           0.125,     1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_CTLO_REVERSE | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL| PF_PROP_GRAPH, NULL, "att2", "HMid" },
    { 1,           0.125,     1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_CTLO_REVERSE | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL| PF_PROP_GRAPH, NULL, "att3", "Hi" },
    { 1,           0.125,     1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_CTLO_REVERSE | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL| PF_PROP_GRAPH, NULL, "att_sc", "S/C" },

    { 0.f,      -1.f,         1.f,   0,  PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_GRAPH, NULL, "weight0", "Weight 1" },
    { 0.f,      -1.f,         1.f,   0,  PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_GRAPH, NULL, "weight1", "Weight 2" },
    { 0.f,      -1.f,         1.f,   0,  PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_GRAPH, NULL, "weight2", "Weight 3" },
    { 0.f,      -1.f,         1.f,   0,  PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_GRAPH, NULL, "weight3", "Weight 4" },
    { 0.f,      -1.f,         1.f,   0,  PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_GRAPH, NULL, "weight_sc", "Weight S/C" },

    { 0.5f,      -1.f,         1.f,   0,  PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_GRAPH, NULL, "release0", "Release 1" },
    { 0.2f,      -1.f,         1.f,   0,  PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_GRAPH, NULL, "release1", "Release 2" },
    { -0.2f,      -1.f,         1.f,   0,  PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_GRAPH, NULL, "release2", "Release 3" },
    { -0.5f,      -1.f,         1.f,   0,  PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_GRAPH, NULL, "release3", "Release 4" },
    { -0.5f,      -1.f,         1.f,   0,  PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_GRAPH, NULL, "release_sc", "Release S/C" },

    { 0,           0,           1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "solo0", "Solo 1" },
    { 0,           0,           1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "solo1", "Solo 2" },
    { 0,           0,           1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "solo2", "Solo 3" },
    { 0,           0,           1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "solo3", "Solo 4" },
    { 0,           0,           1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "solo_sc", "Solo S/C" },

    { 1,         0.f,        1000,  0,  PF_FLOAT | PF_UNIT_MSEC | PF_PROP_OUTPUT, NULL, "effrelease0", "Effectively Release 1" },
    { 1,         0.f,        1000,  0,  PF_FLOAT | PF_UNIT_MSEC | PF_PROP_OUTPUT, NULL, "effrelease1", "Effectively Release 2" },
    { 1,         0.f,        1000,  0,  PF_FLOAT | PF_UNIT_MSEC | PF_PROP_OUTPUT, NULL, "effrelease2", "Effectively Release 3" },
    { 1,         0.f,        1000,  0,  PF_FLOAT | PF_UNIT_MSEC | PF_PROP_OUTPUT, NULL, "effrelease3", "Effectively Release 4" },
    { 1,         0.f,        1000,  0,  PF_FLOAT | PF_UNIT_MSEC | PF_PROP_OUTPUT, NULL, "effrelease_sc", "Effectively Release S/C" },

    { 1,           0,           1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "asc", "ASC" },

    { 0,           0,           1,     0,  PF_FLOAT | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "asc_led", "asc active" },

    { 0.5f,      0.f,         1.f,   0,  PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_GRAPH, NULL, "asc_coeff", "ASC Level" },

    { 1,           1,           4,   0,  PF_INT | PF_SCALE_LINEAR | PF_UNIT_COEF | PF_CTL_KNOB, NULL, "oversampling", "Oversampling" },
    { 1,           0.015625,    64,    0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "level_sc", "Level S/C"},
    { 1,           0,           1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "auto_level", "Auto-level" },
    {}
};

CALF_PLUGIN_INFO(sidechainlimiter) = { 0x8522, "SidechainLimiter", "Calf Sidechain Limiter", "Calf Studio Gear", calf_plugins::calf_copyright_info, "LimiterPlugin" };


////////////////////////////////////////////////////////////////////////////

CALF_PORT_NAMES(emphasis) = {"In L", "In R", "Out L", "Out R"};

const char *emphasis_filter_modes[] = { "Reproduction", "Production"};
const char *emphasis_filter_types[] = { "Columbia", "EMI", "BSI(78rpm)", "RIAA", "Compact Disc (CD)", "50µs (FM)", "75µs (FM)", "50µs (FM-KF)", "75µs (FM-KF)",  };

CALF_PORT_PROPS(emphasis) = {
    BYPASS_AND_LEVEL_PARAMS
    METERING_PARAMS
    { 0,      0,  1,    0, PF_ENUM | PF_CTL_COMBO, emphasis_filter_modes, "mode", "Filter Mode" },
    { 4,      0,  8,    0, PF_ENUM | PF_CTL_COMBO, emphasis_filter_types, "type", "Filter Type" },
    {}
};
CALF_PLUGIN_INFO(emphasis) = { 0x8599, "Emphasis", "Calf Emphasis", "Calf Studio Gear", calf_plugins::calf_copyright_info, "FilterPlugin" };

////////////////////////////////////////////////////////////////////////////
const char *active_mode_names[] = { " ", "ON", "Left", "Right", "Mid", "Side" };

CALF_PORT_NAMES(equalizer5band) = {"In L", "In R", "Out L", "Out R"};

CALF_PORT_PROPS(equalizer5band) = {
    BYPASS_AND_LEVEL_PARAMS
    METERING_PARAMS
    SHELF_PARAMS
    EQ_BAND_PARAMS(1, 250)
    EQ_BAND_PARAMS(2, 1000)
    EQ_BAND_PARAMS(3, 4000)
    EQ_DISPLAY_PARAMS
    {}
};

CALF_PLUGIN_INFO(equalizer5band) = { 0x8511, "Equalizer5Band", "Calf Equalizer 5 Band", "Calf Studio Gear", calf_plugins::calf_copyright_info, "EQPlugin" };

//////////////////////////////////////////////////////////////////////////////


CALF_PORT_NAMES(equalizer8band) = {"In L", "In R", "Out L", "Out R"};
const char *rolloff_mode_names[] = {"12dB/oct", "24dB/oct", "36dB/oct"};

CALF_PORT_PROPS(equalizer8band) = {
    BYPASS_AND_LEVEL_PARAMS
    METERING_PARAMS
    LPHP_PARAMS
    SHELF_PARAMS
    EQ_BAND_PARAMS(1, 100)
    EQ_BAND_PARAMS(2, 500)
    EQ_BAND_PARAMS(3, 2000)
    EQ_BAND_PARAMS(4, 5000)
    EQ_DISPLAY_PARAMS
    {}
};

CALF_PLUGIN_INFO(equalizer8band) = { 0x8512, "Equalizer8Band", "Calf Equalizer 8 Band", "Calf Studio Gear", calf_plugins::calf_copyright_info, "EQPlugin" };

////////////////////////////////////////////////////////////////////////////

CALF_PORT_NAMES(equalizer12band) = {"In L", "In R", "Out L", "Out R"};

CALF_PORT_PROPS(equalizer12band) = {
    BYPASS_AND_LEVEL_PARAMS
    METERING_PARAMS
    LPHP_PARAMS
    SHELF_PARAMS
    EQ_BAND_PARAMS(1, 60)
    EQ_BAND_PARAMS(2, 120)
    EQ_BAND_PARAMS(3, 250)
    EQ_BAND_PARAMS(4, 500)
    EQ_BAND_PARAMS(5, 1000)
    EQ_BAND_PARAMS(6, 2000)
    EQ_BAND_PARAMS(7, 4000)
    EQ_BAND_PARAMS(8, 8000)
    EQ_DISPLAY_PARAMS
    {}
};

CALF_PLUGIN_INFO(equalizer12band) = { 0x8513, "Equalizer12Band", "Calf Equalizer 12 Band", "Calf Studio Gear", calf_plugins::calf_copyright_info, "EQPlugin" };

////////////////////////////////////////////////////////////////////////////

#define GRAPHICEQ_BAND_PARAMS(band) \
    {           0, -1, 1, 0, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_OPTIONAL, NULL, "gain" #band, "Gain " #band },\
    {           0, -32, 32, 0, PF_FLOAT | PF_DIGIT_1 | PF_SCALE_LINEAR | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "gain_scale" #band, "Gain Scale " #band },

#define GRAPHICEQ_GAIN_PARAMS(band) \
    {           0, -1,  1, 0, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "gain" #band, "Gain " #band },\
    {           0, -32, 32, 0, PF_FLOAT | PF_UNIT_DB | PF_SCALE_LINEAR | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "gain_scale" #band, "Gain Scale " #band },

const char *equalizer30band_filters_modes[] = {"Butterworth", "Chebyshev 1", "Chebyshev 2", "Elliptic"};
const char *equalizer30band_channel_modes[] = {"Individual Stereo", "Linked L ⎈ L/R", "Linked R ⎈ L/R"};
const char *equalizer30band_gainscale_modes1[] = {"6 dB", "12 dB", "18 dB", "24 dB", "30 dB"};

CALF_PORT_NAMES(equalizer30band) = {"In L", "In R", "Out L", "Out R"};

CALF_PORT_PROPS(equalizer30band) = {

    { 1, 0.015625,    64, 0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "level_in", "In Level" },
    { 0,           0,  1, 0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "level_in_vuL", "Level In L" },
    { 0,           0,  1, 0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "level_in_vuR", "Level In R" },
    { 0,           0,  1, 0, PF_FLOAT | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "level_in_clipL", "Level Clip In L" },
    { 0,           0,  1, 0, PF_FLOAT | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "level_in_clipR", "Level Clip In R" },

    { 0,           0,  1, 0, PF_BOOL | PF_CTL_TOGGLE, NULL, "bypass", "Bypass" },
    { 0,           0,  3, 0, PF_ENUM | PF_CTL_COMBO, equalizer30band_filters_modes, "filters", "Filters Type" },

    { 18,          6,  30, 0, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_DB, NULL, "gainscale1", "Gain scale 1" },
    { 18,          6,  30, 0, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_DB, NULL, "gainscale2", "Gain scale 2" },

    { 1, 0.015625,    64, 0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "level_out", "Out Level" },
    { 0,           0,  1, 0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "level_out_vuL", "Level Out L" },
    { 0,           0,  1, 0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "level_out_vuR", "Level Out R" },
    { 0,           0,  1, 0, PF_FLOAT | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "level_out_clipL", "Level Clip Out L" },
    { 0,           0,  1, 0, PF_FLOAT | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "level_out_clipR", "Level Clip Out R" },

    //1 - 30 Bands
    GRAPHICEQ_GAIN_PARAMS(10)
    GRAPHICEQ_BAND_PARAMS(11)
    GRAPHICEQ_BAND_PARAMS(12)
    GRAPHICEQ_BAND_PARAMS(13)
    GRAPHICEQ_BAND_PARAMS(14)
    GRAPHICEQ_BAND_PARAMS(15)
    GRAPHICEQ_BAND_PARAMS(16)
    GRAPHICEQ_BAND_PARAMS(17)
    GRAPHICEQ_BAND_PARAMS(18)
    GRAPHICEQ_BAND_PARAMS(19)
    GRAPHICEQ_BAND_PARAMS(110)
    GRAPHICEQ_BAND_PARAMS(111)
    GRAPHICEQ_BAND_PARAMS(112)
    GRAPHICEQ_BAND_PARAMS(113)
    GRAPHICEQ_BAND_PARAMS(114)
    GRAPHICEQ_BAND_PARAMS(115)
    GRAPHICEQ_BAND_PARAMS(116)
    GRAPHICEQ_BAND_PARAMS(117)
    GRAPHICEQ_BAND_PARAMS(118)
    GRAPHICEQ_BAND_PARAMS(119)
    GRAPHICEQ_BAND_PARAMS(120)
    GRAPHICEQ_BAND_PARAMS(121)
    GRAPHICEQ_BAND_PARAMS(122)
    GRAPHICEQ_BAND_PARAMS(123)
    GRAPHICEQ_BAND_PARAMS(124)
    GRAPHICEQ_BAND_PARAMS(125)
    GRAPHICEQ_BAND_PARAMS(126)
    GRAPHICEQ_BAND_PARAMS(127)
    GRAPHICEQ_BAND_PARAMS(128)
    GRAPHICEQ_BAND_PARAMS(129)
    GRAPHICEQ_BAND_PARAMS(130)

    //2 - 30 Bands
    GRAPHICEQ_GAIN_PARAMS(20)
    GRAPHICEQ_BAND_PARAMS(21)
    GRAPHICEQ_BAND_PARAMS(22)
    GRAPHICEQ_BAND_PARAMS(23)
    GRAPHICEQ_BAND_PARAMS(24)
    GRAPHICEQ_BAND_PARAMS(25)
    GRAPHICEQ_BAND_PARAMS(26)
    GRAPHICEQ_BAND_PARAMS(27)
    GRAPHICEQ_BAND_PARAMS(28)
    GRAPHICEQ_BAND_PARAMS(29)
    GRAPHICEQ_BAND_PARAMS(210)
    GRAPHICEQ_BAND_PARAMS(211)
    GRAPHICEQ_BAND_PARAMS(212)
    GRAPHICEQ_BAND_PARAMS(213)
    GRAPHICEQ_BAND_PARAMS(214)
    GRAPHICEQ_BAND_PARAMS(215)
    GRAPHICEQ_BAND_PARAMS(216)
    GRAPHICEQ_BAND_PARAMS(217)
    GRAPHICEQ_BAND_PARAMS(218)
    GRAPHICEQ_BAND_PARAMS(219)
    GRAPHICEQ_BAND_PARAMS(220)
    GRAPHICEQ_BAND_PARAMS(221)
    GRAPHICEQ_BAND_PARAMS(222)
    GRAPHICEQ_BAND_PARAMS(223)
    GRAPHICEQ_BAND_PARAMS(224)
    GRAPHICEQ_BAND_PARAMS(225)
    GRAPHICEQ_BAND_PARAMS(226)
    GRAPHICEQ_BAND_PARAMS(227)
    GRAPHICEQ_BAND_PARAMS(228)
    GRAPHICEQ_BAND_PARAMS(229)
    GRAPHICEQ_BAND_PARAMS(230)

    { 1,           0,  1, 0, PF_FLOAT | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "l_active", "L Active" },
    { 1,           0,  1, 0, PF_FLOAT | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "r_active", "R Active" },
    { 0,           0,  2, 0, PF_ENUM | PF_CTL_COMBO, equalizer30band_channel_modes, "linked", "Link Mode" },

    {}
};

CALF_PLUGIN_INFO(equalizer30band) = { 0x8514, "Equalizer30Band", "Calf Equalizer 30 Band", "Calf Studio Gear", calf_plugins::calf_copyright_info, "EQPlugin" };

////////////////////////////////////////////////////////////////////////////

#define XOVER_BAND_PARAMS(band) \
    { 1,           0.015625,    64,    0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "level" #band, "Gain " #band }, \
    { 1,           0,           1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "active" #band, "Active " #band }, \
    { 0,           0,           1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "phase" #band, "Phase " #band }, \
    { 0.0,       0.0,        20.0,     0,  PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_COEF, NULL, "delay" #band, "Delay " #band }, \
    { 0,           0,           1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "meter_L" #band, "Level L " #band }, \
    { 0,           0,           1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "meter_R" #band, "Level R " #band },

////////////////////////////////////////////////////////////////////////////

CALF_PORT_NAMES(xover2) = {"In L", "In R", "Out 1 L", "Out 1 R", "Out 2 L", "Out 2 R"};

CALF_PORT_PROPS(xover2) = {
    { 1,           0.015625,    64,    0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_NOBOUNDS, NULL, "level", "Gain" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "meter_L", "Input L" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "meter_R", "Input R" },
    { 1,      0,  2,    0, PF_ENUM | PF_CTL_COMBO, crossover_filter_choices, "mode", "Filter Mode" },
    { 1000,       10,           20000, 0,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL, "freq0", "Transition 1" },
    XOVER_BAND_PARAMS(1)
    XOVER_BAND_PARAMS(2)
    {}
};
CALF_PLUGIN_INFO(xover2) = { 0x8515, "XOver2Band", "Calf X-Over 2 Band", "Calf Studio Gear", calf_plugins::calf_copyright_info, "UtilityPlugin" };

////////////////////////////////////////////////////////////////////////////

CALF_PORT_NAMES(xover3) = {"In L", "In R", "Out 1 L", "Out 1 R", "Out 2 L", "Out 2 R", "Out 3 L", "Out 3 R"};

CALF_PORT_PROPS(xover3) = {
    { 1,           0.015625,    64,    0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_NOBOUNDS, NULL, "level", "Gain" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "meter_L", "Input L" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "meter_R", "Input R" },
    { 1,      0,  2,    0, PF_ENUM | PF_CTL_COMBO, crossover_filter_choices, "mode", "Filter Mode" },
    { 150,        10,           20000, 0,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL, "freq0", "Transition 1" },
    { 3000,       10,           20000, 0,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL, "freq1", "Transition 2" },
    XOVER_BAND_PARAMS(1)
    XOVER_BAND_PARAMS(2)
    XOVER_BAND_PARAMS(3)
    {}
};
CALF_PLUGIN_INFO(xover3) = { 0x8515, "XOver3Band", "Calf X-Over 3 Band", "Calf Studio Gear", calf_plugins::calf_copyright_info, "UtilityPlugin" };

////////////////////////////////////////////////////////////////////////////

CALF_PORT_NAMES(xover4) = {"In L", "In R", "Out 1 L", "Out 1 R", "Out 2 L", "Out 2 R", "Out 3 L", "Out 3 R", "Out 4 L", "Out 4 R"};

CALF_PORT_PROPS(xover4) = {
    { 1,           0.015625,    64,    0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_NOBOUNDS, NULL, "level", "Gain" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "meter_L", "Input L" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "meter_R", "Input R" },
    { 1,      0,  2,    0, PF_ENUM | PF_CTL_COMBO, crossover_filter_choices, "mode", "Filter Mode" },
    { 50,        10,           20000, 0,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL, "freq0", "Transition 1" },
    { 500,       10,           20000, 0,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL, "freq1", "Transition 2" },
    { 5000,       10,           20000, 0,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL, "freq2", "Transition 3" },
    XOVER_BAND_PARAMS(1)
    XOVER_BAND_PARAMS(2)
    XOVER_BAND_PARAMS(3)
    XOVER_BAND_PARAMS(4)
    {}
};
CALF_PLUGIN_INFO(xover4) = { 0x8515, "XOver4Band", "Calf X-Over 4 Band", "Calf Studio Gear", calf_plugins::calf_copyright_info, "UtilityPlugin" };

////////////////////////////////////////////////////////////////////////////

#define VOCODER_BAND_PARAMS(band) \
    {           1, 0.000015849, 16, 0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_COEF, NULL, "volume" #band, "Vol " #band }, \
    {           0,          -1,  1, 0, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_COEF, NULL, "pan" #band, "Pan " #band }, \
    { 0.000015849, 0.000015849, 16, 0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_COEF, NULL, "noise" #band, "Noise " #band }, \
    { 0.000015849, 0.000015849, 16, 0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_COEF, NULL, "mod" #band, "Dry " #band }, \
    {           0,           0,  1, 0, PF_BOOL | PF_CTL_TOGGLE, NULL, "solo" #band, "Solo " #band }, \
    {           0,           0,  1, 0, PF_FLOAT | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "level" #band, "Level " #band }, \
    {           1,         0.25, 4, 0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_COEF, NULL, "q" #band, "Q " #band },

const char *vocoder_analyzer_modes[] = {"Off", "Carrier", "Modulator", "Processed", "Output"};

CALF_PORT_NAMES(vocoder) = {"In L", "In R", "Out L", "Out R"};

CALF_PORT_PROPS(vocoder) = {
    { 0,           0,  1, 0, PF_BOOL | PF_CTL_TOGGLE, NULL, "bypass", "Bypass" },
    { 1,           0,  1, 0, PF_BOOL | PF_CTL_TOGGLE, NULL, "link", "Link" },
    { 1,           0,  1, 0, PF_BOOL | PF_CTL_TOGGLE, NULL, "detectors", "Detectors" },

    { 1, 0.015625,    64, 0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "carrier_in", "Carrier In" },
    { 0,           0,  1, 0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "carrier_inL", "Carrier In L" },
    { 0,           0,  1, 0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "carrier_inR", "Carrier In R" },
    { 0,           0,  1, 0, PF_FLOAT | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "carrier_clip_inL", "Carrier Clip In L" },
    { 0,           0,  1, 0, PF_FLOAT | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "carrier_clip_inR", "Carrier Clip In R" },

    { 1, 0.015625,    64, 0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "mod_in", "Modulator In" },
    { 0,           0,  1, 0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "mod_inL", "Modulator In L" },
    { 0,           0,  1, 0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "mod_inR", "Modulator In R" },
    { 0,           0,  1, 0, PF_FLOAT | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "mod_clip_inL", "Modulator Clip In L" },
    { 0,           0,  1, 0, PF_FLOAT | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "mod_clip_inR", "Modulator Clip In R" },

    { 1, 0.015625,    64, 0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "out", "Out" },
    { 0,           0,  1, 0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "outL", "Out L" },
    { 0,           0,  1, 0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "outR", "Out R" },
    { 0,           0,  1, 0, PF_FLOAT | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "clip_outL", "Clip Out L" },
    { 0,           0,  1, 0, PF_FLOAT | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "clip_outR", "Clip Out R" },

    { 0,           0, 16, 0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "carrier", "Carrier" },
    { 0,           0, 16, 0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "modulator", "Modulator" },
    { 1, 0.000015849, 16, 0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "processed", "Processed" },

    { 4,           2,  9, 0, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_COEF, NULL, "order", "Isolation" },
    { 2,           0,  4, 0, PF_INT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_COEF, NULL, "bands", "Bands" },
    { 0,           0,  1, 0, PF_BOOL | PF_CTL_TOGGLE , NULL, "hiq", "High-Q" },

    {  5.f, 0.1f, 500.f,  0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "attack", "Attack" },
    { 50.f, 0.1f, 5000.f, 0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "release", "Release" },

    { 0,           0,  4, 0, PF_ENUM | PF_CTL_COMBO, vocoder_analyzer_modes, "analyzer", "Analyzer" },

    VOCODER_BAND_PARAMS(1)
    VOCODER_BAND_PARAMS(2)
    VOCODER_BAND_PARAMS(3)
    VOCODER_BAND_PARAMS(4)
    VOCODER_BAND_PARAMS(5)
    VOCODER_BAND_PARAMS(6)
    VOCODER_BAND_PARAMS(7)
    VOCODER_BAND_PARAMS(8)
    VOCODER_BAND_PARAMS(9)
    VOCODER_BAND_PARAMS(10)
    VOCODER_BAND_PARAMS(11)
    VOCODER_BAND_PARAMS(12)
    VOCODER_BAND_PARAMS(13)
    VOCODER_BAND_PARAMS(14)
    VOCODER_BAND_PARAMS(15)
    VOCODER_BAND_PARAMS(16)
    VOCODER_BAND_PARAMS(17)
    VOCODER_BAND_PARAMS(18)
    VOCODER_BAND_PARAMS(19)
    VOCODER_BAND_PARAMS(20)
    VOCODER_BAND_PARAMS(21)
    VOCODER_BAND_PARAMS(22)
    VOCODER_BAND_PARAMS(23)
    VOCODER_BAND_PARAMS(24)
    VOCODER_BAND_PARAMS(25)
    VOCODER_BAND_PARAMS(26)
    VOCODER_BAND_PARAMS(27)
    VOCODER_BAND_PARAMS(28)
    VOCODER_BAND_PARAMS(29)
    VOCODER_BAND_PARAMS(30)
    VOCODER_BAND_PARAMS(31)
    VOCODER_BAND_PARAMS(32)

    { 20,    20, 20000, 0,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL, "lower", "Lower" },
    { 20000, 20, 20000, 0,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL, "upper", "Upper" },
    { 0,     -1,     1, 0,  PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_COEF, NULL, "tilt", "Tilt" },

    {}
};

CALF_PLUGIN_INFO(vocoder) = { 0x8514, "Vocoder", "Calf Vocoder", "Calf Studio Gear", calf_plugins::calf_copyright_info, "FilterPlugin" };


////////////////////////////////////////////////////////////////////////////

CALF_PORT_NAMES(pulsator) = {"In L", "In R", "Out L", "Out R"};

const char *pulsator_mode_names[] = { "Sine", "Triangle", "Square", "Saw up", "Saw down" };
const char *pulsator_pulse_width[] = { "8 Periods", "4 Periods", "2 Periods", "1 Period", "Half Period" };

CALF_PORT_PROPS(pulsator) = {
    BYPASS_AND_LEVEL_PARAMS
    METERING_PARAMS
    { 0,           0,           4,     0, PF_ENUM | PF_CTL_COMBO, pulsator_mode_names, "mode", "Mode" },
    { 1,           0,           1,     0, PF_FLOAT | PF_SCALE_PERC, NULL, "amount", "Modulation" },
    { 0.0,         0,           1,     0, PF_FLOAT | PF_SCALE_PERC, NULL, "offset_l", "Offset L" },
    { 0.5,         0,           1,     0, PF_FLOAT | PF_SCALE_PERC, NULL, "offset_r", "Offset R" },
    { 0,           0,           1,     0, PF_BOOL | PF_CTL_TOGGLE, NULL, "mono", "Mono-in" },
    { 0,           0,           1,     2, PF_BOOL | PF_CTL_BUTTON , NULL, "reset", "Reset" },
    { 3,           0,           4,     0, PF_ENUM | PF_CTL_COMBO, pulsator_pulse_width, "pulsewidth", "Pulse Width" },
    PERIODICAL_DEFINITIONS(2)
    {}
};

CALF_PLUGIN_INFO(pulsator) = { 0x8514, "Pulsator", "Calf Pulsator", "Calf Studio Gear", calf_plugins::calf_copyright_info, "ModulatorPlugin" };

////////////////////////////////////////////////////////////////////////////

CALF_PORT_NAMES(ringmodulator) = {"In L", "In R", "Out L", "Out R"};

const char *ringmod_mode_names[] = { "Sine", "Triangle", "Square", "Saw up", "Saw down" };

CALF_PORT_PROPS(ringmodulator) = {
    BYPASS_AND_LEVEL_PARAMS
    METERING_PARAMS
    { 0,           0,           4,     0,  PF_ENUM | PF_CTL_COMBO, ringmod_mode_names, "mod_mode", "Modulator" },
    { 1000,        1,       20000,     0,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL, "mod_freq", "Mod Freq" },
    { 0.5,         0,           1,     0,  PF_FLOAT | PF_SCALE_PERC, NULL, "mod_amount", "Mod Amount" },
    { 0.5,         0,           1,     0,  PF_FLOAT | PF_SCALE_PERC | PF_UNIT_COEF | PF_CTL_KNOB, NULL, "mod_phase", "Mod Phase" },
    { 0,           -200,        200,  401, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_CENTS, NULL, "mod_detune", "Mod Detune" },
    { 0,           0,           1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "mod_listen", "Listen" },

    { 0,           0,           4,     0,  PF_ENUM | PF_CTL_COMBO, ringmod_mode_names, "lfo1_mode", "LFO 1" },
    { 0.1,         0.01,        10,    0,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL, "lfo1_freq", "LFO 1 Freq" },
    { 0,           0,           1,     2,  PF_BOOL | PF_CTL_BUTTON , NULL, "lfo1_reset", "Reset 1" },
    { 100,         1,       20000,     0,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL, "lfo1_mod_freq_lo", "Mod Freq LO" },
    { 10000,       1,       20000,     0,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL, "lfo1_mod_freq_hi", "Mod Freq HI" },
    { 0,           0,           1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "lfo1_mod_freq_active", "Mod Freq Active" },
    { -100,        -200,        200,  401, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_CENTS, NULL, "lfo1_mod_detune_lo", "Mod Detune LO" },
    { 100,         -200,        200,  401, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_CENTS, NULL, "lfo1_mod_detune_hi", "Mod Detune HI" },
    { 0,           0,           1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "lfo1_mod_detune_active", "Mod Detune Active" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "lfo1_activity", "Activity 1" },

    { 0,           0,           4,     0,  PF_ENUM | PF_CTL_COMBO, ringmod_mode_names, "lfo2_mode", "LFO 2" },
    { 0.2,         0.01,        10,    0,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL, "lfo2_freq", "LFO 2 Freq" },
    { 0,           0,           1,     2,  PF_BOOL | PF_CTL_BUTTON , NULL, "lfo2_reset", "Reset 2" },
    { 0.05,        0.01,        10,    0,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL, "lfo2_lfo1_freq_lo", "LFO Freq LO" },
    { 0.5,         0.01,        10,    0,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL, "lfo2_lfo1_freq_hi", "LFO Freq HI" },
    { 0,           0,           1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "lfo2_lfo1_freq_active", "LFO 1 Freq Active" },
    { 0.3,         0,           1,     0,  PF_FLOAT | PF_SCALE_PERC, NULL, "lfo2_mod_amount_lo", "Mod Amount LO" },
    { 0.6,         0,           1,     0,  PF_FLOAT | PF_SCALE_PERC, NULL, "lfo2_mod_amount_hi", "Mod Amount HI" },
    { 0,           0,           1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "lfo2_mod_amount_active", "Mod Amount Active" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "lfo2_activity", "Activity 2" },

    {}
};

CALF_PLUGIN_INFO(ringmodulator) = { 0x8514, "RingModulator", "Calf Ring Modulator", "Calf Studio Gear", calf_plugins::calf_copyright_info, "ModulatorPlugin" };


////////////////////////////////////////////////////////////////////////////

CALF_PORT_NAMES(saturator) = {"In L", "In R", "Out L", "Out R"};

CALF_PORT_PROPS(saturator) = {
    BYPASS_AND_LEVEL_PARAMS
    METERING_PARAMS
    { 1,         0,           1,     0,  PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB , NULL, "mix", "Mix" },

    { 5,           0.1,         10,    0,  PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_COEF, NULL, "drive", "Saturation" },
    { 10,         -10,          10,    0,  PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_FADER | PF_UNIT_COEF, NULL, "blend", "Blend" },

    { 20000,      10,           20000, 0,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL, "lp_pre_freq", "Lowpass" },
    { 10,         10,           20000, 0,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL, "hp_pre_freq", "Highpass" },

    { 20000,      10,           20000, 0,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL, "lp_post_freq", "Lowpass" },
    { 10,         10,           20000, 0,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL, "hp_post_freq", "Highpass" },

    { 2000,       80,           8000,  0,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL, "p_freq", "Tone" },
    { 1,          0.0625,       16,    0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "p_level", "Amount" },
    { 1,          0.1,          10,    1,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_COEF, NULL, "p_q", "Gradient" },

    { 0,           0,           1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "pre", "Activate Pre" },
    { 0,           0,           1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "post", "Activate Post" },

    {}
};

CALF_PLUGIN_INFO(saturator) = { 0x8530, "Saturator", "Calf Saturator", "Calf Studio Gear", calf_plugins::calf_copyright_info, "DistortionPlugin" };

////////////////////////////////////////////////////////////////////////////

CALF_PORT_NAMES(exciter) = {"In L", "In R", "Out L", "Out R"};

CALF_PORT_PROPS(exciter) = {
    { 0,           0,           1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "bypass", "Bypass" },
    { 1,           0.015625,    64,    0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB | PF_PROP_NOBOUNDS, NULL, "level_in", "Input" },
    { 1,           0.015625,    64,    0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB | PF_PROP_NOBOUNDS, NULL, "level_out", "Output" },
    { 1,           0,           64,    0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB | PF_PROP_NOBOUNDS, NULL, "amount", "Amount" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "meter_in", "Input" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "meter_out", "Output" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "clip_in", "0dB" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "clip_out", "0dB" },

    { 8.5,         0.1,         10,    0,  PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_COEF, NULL, "drive", "Harmonics" },
    { 0,          -10,          10,    0,  PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_FADER | PF_UNIT_COEF, NULL, "blend", "Blend harmonics" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "meter_drive", "Harmonics level" },

    { 7500,       2000,         12000, 0,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL, "freq", "Scope" },
    { 0,          0,            1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "listen", "Listen" },
    { 0,          0,            1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "ceil_active", "Ceiling active" },
    { 16000,      10000,        20000, 0,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL, "ceil", "Ceiling" },
    {}
};

CALF_PLUGIN_INFO(exciter) = { 0x8531, "Exciter", "Calf Exciter", "Calf Studio Gear", calf_plugins::calf_copyright_info, "SpectralPlugin" };

////////////////////////////////////////////////////////////////////////////

CALF_PORT_NAMES(bassenhancer) = {"In L", "In R", "Out L", "Out R"};

CALF_PORT_PROPS(bassenhancer) = {
    { 0,           0,           1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "bypass", "Bypass" },
    { 1,           0.015625,    64,    0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB | PF_PROP_NOBOUNDS, NULL, "level_in", "Input" },
    { 1,           0.015625,    64,    0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB | PF_PROP_NOBOUNDS, NULL, "level_out", "Output" },
    { 1,           0,           64,    0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB | PF_PROP_NOBOUNDS, NULL, "amount", "Amount" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "meter_in", "Input" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "meter_out", "Output" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "clip_in", "0dB" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "clip_out", "0dB" },

    { 8.5,         0.1,         10,    0,  PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_COEF, NULL, "drive", "Harmonics" },
    { 0,          -10,          10,    0,  PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_FADER | PF_UNIT_COEF, NULL, "blend", "Blend Harmonics" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "meter_drive", "Harmonics Level" },

    { 100,        10,           250,   0,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL, "freq", "Scope" },
    { 0,          0,            1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "listen", "Listen" },
    { 0,           0,           1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "floor_active", "Floor active" },
    { 20,         10,           120,   0,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL, "floor", "Floor" },
    {}
};

CALF_PLUGIN_INFO(bassenhancer) = { 0x8532, "BassEnhancer", "Calf Bass Enhancer", "Calf Studio Gear", calf_plugins::calf_copyright_info, "SpectralPlugin" };

////////////////////////////////////////////////////////////////////////////

CALF_PORT_NAMES(stereo) = {"In L", "In R", "Out L", "Out R"};
const char *stereo_mode_names[] = { "LR > LR (Stereo Default)", "LR > MS (Stereo to Mid-Side)", "MS > LR (Mid-Side to Stereo)", "LR > LL (Mono Left Channel)", "LR > RR (Mono Right Channel)", "LR > L+R (Mono Sum L+R)", "LR > RL (Stereo Flip Channels)" };
CALF_PORT_PROPS(stereo) = {
    BYPASS_AND_LEVEL_PARAMS
    METERING_PARAMS

    { 0.f,      -1.f,           1.f,   0,  PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_COEF, NULL, "balance_in", "Balance In" },
    { 0.f,      -1.f,           1.f,   0,  PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_COEF, NULL, "balance_out", "Balance Out" },

    { 0,          0,            1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "softclip", "Softclip" },
    { 0,          0,            1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "mutel", "Mute L" },
    { 0,          0,            1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "muter", "Mute R" },
    { 0,          0,            1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "phasel", "Phase L" },
    { 0,          0,            1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "phaser", "Phase R" },

    { 0,          0,            6,     0,  PF_ENUM | PF_CTL_COMBO, stereo_mode_names, "mode", "Mode" },

    { 1,   0.015625,           64,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "slev", "S Level" },
    { 0.f,     -1.f,          1.f,     0,  PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_COEF | PF_DIGIT_2, NULL, "sbal", "S Bal" },
    { 1,   0.015625,           64,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "mlev", "M Level" },
    { 0.f,     -1.f,          1.f,     0,  PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_COEF | PF_DIGIT_2, NULL, "mpan", "M Pan" },

    { 0.f,           -1.f,           1.f,    0,  PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_NOBOUNDS | PF_DIGIT_2, NULL, "stereo_base", "Stereo Base" },
    { 0.f,         -20.f,        20.f,  0,  PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "delay", "Delay" },

    { 0.f,      0.f,           1.f,   0,  PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_COEF | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "meter_phase", "Phase Correlation" },

    { 1,           1,           100,    0,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_NOBOUNDS, NULL, "sc_level", "S/C Level" },
    { 0,        0, 360,  91, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_DEG, NULL, "stereo_phase", "Stereo Phase" },
    {}
};

CALF_PLUGIN_INFO(stereo) = { 0x8588, "StereoTools", "Calf Stereo Tools", "Calf Studio Gear", calf_plugins::calf_copyright_info, "SpatialPlugin" };

////////////////////////////////////////////////////////////////////////////

CALF_PORT_NAMES(multibandenhancer) = {"In L", "In R", "Out L", "Out R"};

CALF_PORT_PROPS(multibandenhancer) = {
    BYPASS_AND_LEVEL_PARAMS
    METERING_PARAMS
    { 100,        10,        20000,  0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ | PF_PROP_GRAPH, NULL, "freq0", "Split 1/2" },
    { 750,        10,        20000,  0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ | PF_PROP_GRAPH, NULL, "freq1", "Split 2/3" },
    { 5000,       10,        20000,  0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ | PF_PROP_GRAPH, NULL, "freq2", "Split 3/4" },

    { 1,           0,           1,   0, PF_ENUM | PF_CTL_COMBO, mb_crossover_filter_choices, "mode", "Filter Mode" },

    { 0.f,      -1.f,         1.f,   0, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_COEF | PF_DIGIT_2, NULL, "base0", "Base 1" },
    { 0.f,      -1.f,         1.f,   0, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_COEF | PF_DIGIT_2, NULL, "base1", "Base 2" },
    { 0.f,      -1.f,         1.f,   0, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_COEF | PF_DIGIT_2, NULL, "base2", "Base 3" },
    { 0.f,      -1.f,         1.f,   0, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_COEF | PF_DIGIT_2, NULL, "base3", "Base 4" },

    { 0,         0.0,         10,    0, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_COEF, NULL, "drive0", "Harmonics 1" },
    { 0,         0.0,         10,    0, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_COEF, NULL, "drive1", "Harmonics 2" },
    { 0,         0.0,         10,    0, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_COEF, NULL, "drive2", "Harmonics 3" },
    { 0,         0.0,         10,    0, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_COEF, NULL, "drive3", "Harmonics 4" },

    { 0,        -10,          10,    0, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_FADER | PF_UNIT_COEF, NULL, "blend0", "Blend Harmonics 1" },
    { 0,        -10,          10,    0, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_FADER | PF_UNIT_COEF, NULL, "blend1", "Blend Harmonics 2" },
    { 0,        -10,          10,    0, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_FADER | PF_UNIT_COEF, NULL, "blend2", "Blend Harmonics 3" },
    { 0,        -10,          10,    0, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_FADER | PF_UNIT_COEF, NULL, "blend3", "Blend Harmonics 4" },

    { 0,         0,           1,     0, PF_BOOL | PF_CTL_TOGGLE, NULL, "solo0", "Solo 1" },
    { 0,         0,           1,     0, PF_BOOL | PF_CTL_TOGGLE, NULL, "solo1", "Solo 2" },
    { 0,         0,           1,     0, PF_BOOL | PF_CTL_TOGGLE, NULL, "solo2", "Solo 3" },
    { 0,         0,           1,     0, PF_BOOL | PF_CTL_TOGGLE, NULL, "solo3", "Solo 4" },

    {}
};

CALF_PLUGIN_INFO(multibandenhancer) = { 0x8121, "MultibandEnhancer", "Calf Multiband Enhancer", "Calf Studio Gear", calf_plugins::calf_copyright_info, "SpatialPlugin" };


////////////////////////////////////////////////////////////////////////////

CALF_PORT_NAMES(multispread) = {"In L", "In R", "Out L", "Out R"};

CALF_PORT_PROPS(multispread) = {
    BYPASS_AND_LEVEL_PARAMS
    METERING_PARAMS
    { 0,          0,            1,    0, PF_BOOL | PF_CTL_TOGGLE, NULL, "mono", "Mono" },
    { 2,          1,           16,    0, PF_INT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_COEF, NULL, "filters", "Filters" },
    { 1,   0.015625,           64,    0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "amount0", "Intensity Sub" },
    { 1,   0.015625,           64,    0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "amount1", "Intensity Low" },
    { 1,   0.015625,           64,    0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "amount2", "Intensity Mid" },
    { 1,   0.015625,           64,    0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "amount3", "Intensity High" },
    { 1,          0,            1,    0, PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB | PF_UNIT_COEF, NULL, "intensity", "Intensity" },

    {}
};

CALF_PLUGIN_INFO(multispread) = { 0x8123, "MultiSpread", "Calf Multi Spread", "Calf Studio Gear", calf_plugins::calf_copyright_info, "SpatialPlugin" };

////////////////////////////////////////////////////////////////////////////

CALF_PORT_NAMES(haas_enhancer) = {"In L", "In R", "Out L", "Out R"};

const char *haas_enhancer_source[] = {
    "Left",
    "Right",
    "Mid (L+R)",
    "Side (L-R)",
    "Mute",
};

CALF_PORT_PROPS(haas_enhancer) = {
    BYPASS_AND_LEVEL_PARAMS
    {  1.0,    0.015625, 64.0,     0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_NOBOUNDS, NULL, "s_gain", "Side Gain" },
    METERING_PARAMS
    {    0,        0,    1,     0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "meter_sideL", "Side L" },
    {    0,        0,    1,     0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "meter_sideR", "Side R" },

    {    2,        0,    4,     1, PF_ENUM | PF_CTL_COMBO, haas_enhancer_source, "m_source", "Middle source" },
    {  0.0,      0.0,  1.0,   1.0, PF_BOOL | PF_CTL_TOGGLE, NULL, "m_phase", "Middle phase" },

    { 2.05,      0.0, 10.0,  0.01, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "s_delay1", "Left Delay" },
    {  -1.,     -1.0,  1.0,  0.01, PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB, NULL, "s_balance1", "Left Balance" },
    {  1.0, 0.015625, 64.0,     0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB | PF_PROP_NOBOUNDS, NULL, "s_gain1", "Left Gain" },
    {  0.0,      0.0,  1.0,   1.0, PF_BOOL | PF_CTL_TOGGLE, NULL, "s_phase1", "Left Phase" },

    { 2.12,      0.0, 10.0,  0.01, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "s_delay2", "Right Delay" },
    {  1.0,     -1.0,  1.0,  0.01, PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB, NULL, "s_balance2", "Right Balance" },
    {  1.0, 0.015625, 64.0,     0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB | PF_PROP_NOBOUNDS, NULL, "s_gain2", "Right Gain" },
    {  1.0,      0.0,  1.0,   1.0, PF_BOOL | PF_CTL_TOGGLE, NULL, "s_phase2", "Right Phase" },

    {}
};
CALF_PLUGIN_INFO(haas_enhancer) = { 0x8486, "HaasEnhancer", "Calf Haas Stereo Enhancer", "Calf Studio Gear", calf_plugins::calf_copyright_info, "SpatialPlugin" };

////////////////////////////////////////////////////////////////////////////

CALF_PORT_NAMES(mono) = {"In", "Out L", "Out R"};
CALF_PORT_PROPS(mono) = {
    { 0,           0,           1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "bypass", "Bypass" },
    { 1,           0.015625,    64,    0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_NOBOUNDS, NULL, "level_in", "Input" },
    { 1,           0.015625,    64,    0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_NOBOUNDS, NULL, "level_out", "Output" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "meter_in", "Input" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "meter_outL", "Output L" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "meter_outR", "Output R" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "clip_in", "0dB-In" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "clip_outL", "0dB-OutL" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "clip_outR", "0dB-OutR" },

    { 0.f,      -1.f,           1.f,   0,  PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_GRAPH, NULL, "balance_out", "Balance" },

    { 0,          0,            1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "softclip", "Softclip" },
    { 0,          0,            1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "mutel", "Mute L" },
    { 0,          0,            1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "muter", "Mute R" },
    { 0,          0,            1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "phasel", "Phase L" },
    { 0,          0,            1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "phaser", "Phase R" },

    { 0.f,         -20.f,        20.f,  0,  PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "delay", "Delay" },
    { 0.f,           -1.f,           1.f,    0,  PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_NOBOUNDS, NULL, "stereo_base", "Stereo Base" },
    { 0,        0, 360,  91, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_DEG, NULL, "stereo_phase", "Stereo Phase" },
    { 1,           1,           100,    0,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_NOBOUNDS, NULL, "sc_level", "S/C Level" },
    {}
};

CALF_PLUGIN_INFO(mono) = { 0x8589, "MonoInput", "Calf Mono Input", "Calf Studio Gear", calf_plugins::calf_copyright_info, "UtilityPlugin" };


////////////////////////////////////////////////////////////////////////////

CALF_PORT_NAMES(analyzer) = {"In L", "In R", "Out L", "Out R"};
const char *gonio_mode_names[] = { "Small Dots", "Medium Dots", "Big Dots", "Fields", "Lines (High CPU)" };
const char *analyzer_mode_names[] = { "Analyzer Average", "Analyzer Left", "Analyzer Right", "Analyzer Stereo", "Stereo Image", "Stereo Difference", "Spectralizer Average", "Spectralizer Left", "Spectralizer Right", "Spectralizer Colored Stereo", "Spectralizer Parallel Stereo" };
const char *analyzer_smooth_names[] = { "Off", "Falling", "Transition" };
const char *analyzer_post_names[] = { "Normalized", "Average", "Additive", "Denoised Peaks" };
const char *analyzer_view_names[] = { "Bars", "Lines", "Cubic Splines" };
const char *analyzer_scale_names[] = { "Logarithmic", "Linear" };
const char *analyzer_windowing_names[] = { "Rectangular", "Hamming", "von Hann", "Blackman", "Blackman-Harris", "Blackman-Nuttall", "Sine", "Lanczos", "Gauß", "Bartlett", "Triangular", "Bartlett-Hann" };
CALF_PORT_PROPS(analyzer) = {
    { 0,           0,           1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "meter_L", "Level L" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "meter_R", "Level R" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "clip_L", "Clip L" },
    { 0,           0,           1,     0,  PF_FLOAT | PF_CTL_LED | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "clip_R", "Clip R" },

    { 1.25,      0.5,           2,     0,  PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_NOBOUNDS, NULL, "analyzer_level", "Analyzer Level" },
    { 0,           0,          10,     0,  PF_ENUM | PF_CTL_COMBO, analyzer_mode_names, "analyzer_mode", "Analyzer Mode" },
    { 0,           0,           1,     2,  PF_ENUM | PF_CTL_COMBO, analyzer_scale_names, "analyzer_scale", "Analyzer Scale" },
    { 0,           0,           3,     0,  PF_ENUM | PF_CTL_COMBO, analyzer_post_names, "analyzer_post", "Analyzer Post FFT" },
    { 1,           0,           1,     2,  PF_ENUM | PF_CTL_COMBO , analyzer_view_names, "analyzer_view", "Analyzer View" },
    { 1,           0,           2,     0,  PF_ENUM | PF_CTL_COMBO, analyzer_smooth_names, "analyzer_smoothing", "Analyzer Smoothing" },
    { 2,           0,          11,     2,  PF_ENUM | PF_CTL_COMBO, analyzer_windowing_names, "analyzer_windowing", "Analyzer Windowing" },
    { 7,           2,           8,     0,  PF_INT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_GRAPH, NULL, "analyzer_accuracy", "Analyzer Accuracy" },
    { 15,          1,          15,     0,  PF_INT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_GRAPH, NULL, "analyzer_speed", "Analyzer Speed" },
    { 1,           0,           1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "analyzer_display", "Analyzer Display" },
    { 0,           0,           1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "analyzer_hold", "Analyzer Hold" },
    { 0,           0,           1,     2,  PF_BOOL | PF_CTL_TOGGLE , NULL, "analyzer_freeze", "Analyzer Freeze" },

    { 1,           0,           4,     0,  PF_ENUM | PF_CTL_COMBO, gonio_mode_names, "gonio_mode", "Gonio Mode" },
    { 1,           0,           1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "gonio_use_fade", "Gonio Fade Active" },
    { 0.5f,      0.f,         1.f,     0,  PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_GRAPH, NULL, "gonio_fade", "Gonio Fade" },
    { 4,           1,           5,     0,  PF_INT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_GRAPH, NULL, "gonio_accuracy", "Gonio Accuracy" },
    { 1,           0,           1,     0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "gonio_display", "Gonio Display" },

    {}
};

CALF_PLUGIN_INFO(analyzer) = { 0x8588, "Analyzer", "Calf Analyzer", "Calf Studio Gear", calf_plugins::calf_copyright_info, "AnalyserPlugin" };

////////////////////////////////////////////////////////////////////////////
const char *transientdesigner_view_names[] = { "Output", "Envelope", "Attack", "Release" };
const char *transientdesigner_filter_modes[] = { "Off", "12dB", "24dB", "36dB" };
CALF_PORT_NAMES(transientdesigner) = {"In L", "In R", "Out L", "Out R"};
CALF_PORT_PROPS(transientdesigner) = {
    BYPASS_AND_LEVEL_PARAMS
    METERING_PARAMS
    { 1,      0,     1,      0,  PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_GRAPH, NULL, "mix", "Mix" },
    { 30.f,  1.f,   500.f,  0,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "attack_time", "Attack Time" },
    { 0.f,   -1.f,   1.f,    0,  PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_GRAPH, NULL, "attack_boost", "Attack Boost" },
    { 1.f,    0.0009765625f, 1.f,    0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DBFS | PF_PROP_GRAPH, NULL, "sustain_threshold", "Sustain Threshold" },
    { 300.f,  1.f,   5000.f, 0,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "release_time", "Release Time" },
    { 0.f,   -1.f,   1.f,    0,  PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_GRAPH, NULL, "release_boost", "Release Boost" },
    { 2000.f, 50.f,  5000.f, 0,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "display", "Display" },
    { pow(2.0f,-12.0f), pow(2.0f,-12.0f),1, 0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DBFS | PF_PROP_GRAPH, NULL, "display_threshold", "Threshold" },
    { 0,      0,     100,     0,  PF_INT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_SAMPLES, NULL, "lookahead", "Lookahead" },
    { 0,     0,     3,      0,  PF_ENUM | PF_CTL_COMBO, transientdesigner_view_names, "view", "View Mode" },
    { 100,    20,20000, 0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ | PF_PROP_GRAPH, NULL, "hipass", "Highpass" },
    { 5000,   20,20000, 0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL, "lopass", "Lowpass" },
    { 0,        0,      3, 0,  PF_ENUM | PF_CTL_COMBO, transientdesigner_filter_modes, "hp_mode", "HP-Mode" },
    { 0,        0,      3, 0,  PF_ENUM | PF_CTL_COMBO, transientdesigner_filter_modes, "lp_mode", "LP-Mode" },
    { 0,        0,      1, 0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "listen", "Listen" },
    {}
};

CALF_PLUGIN_INFO(transientdesigner) = { 0x8588, "TransientDesigner", "Calf Transient Designer", "Calf Studio Gear", calf_plugins::calf_copyright_info, "EnvelopePlugin" };

////////////////////////////////////////////////////////////////////////////

#define VINYL_SYNTH(band) \
    { 0, -1, 1, 0, PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB | PF_UNIT_COEF, NULL, "pitch" #band, "Pitch " #band }, \
    {           0,           0,  1, 0, PF_BOOL | PF_CTL_TOGGLE, NULL, "active" #band, "Activate " #band },

CALF_PORT_NAMES(vinyl) = {"In L", "In R", "Out L", "Out R"};
CALF_PORT_PROPS(vinyl) = {
    { 0,        0,      1, 0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "bypass", "Bypass" },
    { 1,      0.015625,    64, 0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB | PF_PROP_NOBOUNDS, NULL, "level_in", "Input Gain" },
    { 1,        0.015625,    64, 0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB | PF_PROP_NOBOUNDS, NULL, "level_out", "Output Gain" },
    METERING_PARAMS
    { 0,        0,      1, 0,  PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB | PF_UNIT_COEF , NULL, "drone", "Drone" },
    { 33,      33,     78, 1,  PF_INT | PF_CTL_KNOB | PF_UNIT_COEF , NULL, "speed", "Speed" },
    { 0,        0,      1, 0,  PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB | PF_UNIT_COEF , NULL, "aging", "Aging" },
    { 1000,   600,   1800, 0,  PF_INT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ , NULL, "freq", "Frequency" },

    { 0.0078125, 0.000015849, 1, 0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "gain0", "Vol 0" },
    VINYL_SYNTH(0)
    { 0.0078125, 0.000015849, 1, 0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "gain1", "Vol 1" },
    VINYL_SYNTH(1)
    { 0.015625, 0.000015849, 1, 0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "gain2", "Vol 2" },
    VINYL_SYNTH(2)
    { 0.0078125, 0.000015849, 1, 0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "gain3", "Vol 3" },
    VINYL_SYNTH(3)
    { 0.03125, 0.000015849, 1, 0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "gain4", "Vol 4" },
    VINYL_SYNTH(4)
    { 0.0625, 0.000015849, 1, 0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "gain5", "Vol 5" },
    VINYL_SYNTH(5)
    { 0.0625, 0.000015849, 1, 0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB, NULL, "gain6", "Vol 6" },
    VINYL_SYNTH(6)

    {}
};

CALF_PLUGIN_INFO(vinyl) = { 0x1589, "Vinyl", "Calf Vinyl", "Calf Studio Gear", calf_plugins::calf_copyright_info, "SimulatorPlugin" };

////////////////////////////////////////////////////////////////////////////

CALF_PORT_NAMES(tapesimulator) = {"In L", "In R", "Out L", "Out R"};
const char *tapesimulator_speed_names[] = { "Slow", "Fast" };
CALF_PORT_PROPS(tapesimulator) = {
    { 0,        0,      1, 0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "bypass", "Bypass" },
    { 0.5,      0.015625,    64, 0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB | PF_PROP_NOBOUNDS, NULL, "level_in", "Input Gain" },
    { 1,        0.015625,    64, 0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB | PF_PROP_NOBOUNDS, NULL, "level_out", "Output Gain" },
    METERING_PARAMS
    { 1,        0,      1, 0,  PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB | PF_UNIT_COEF , NULL, "mix", "Mix" },
    { 12500, 1000,  20000, 0,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL, "lowpass", "Filter" },
    { 1,        0,      1, 0,  PF_ENUM | PF_CTL_COMBO, tapesimulator_speed_names, "speed", "Speed Simulation" },
    { 0.10,     0,      1, 0,  PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB | PF_UNIT_COEF , NULL, "noise", "Noise" },
    { 0.20,     0,      1, 0,  PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB | PF_UNIT_COEF , NULL, "mechanical", "Mechanical" },
    { 1,        0,      1, 0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "magnetical", "Magnetical" },
    { 0,        0,      1, 0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "post", "Post-Filter" },
    {}
};

CALF_PLUGIN_INFO(tapesimulator) = { 0x8588, "TapeSimulator", "Calf Tape Simulator", "Calf Studio Gear", calf_plugins::calf_copyright_info, "SimulatorPlugin" };

////////////////////////////////////////////////////////////////////////////

CALF_PORT_NAMES(crusher) = {"In L", "In R", "Out L", "Out R"};
const char *crusher_mode_names[] = { "Linear", "Logarithmic" };
CALF_PORT_PROPS(crusher) = {
    BYPASS_AND_LEVEL_PARAMS
    METERING_PARAMS
    { 4,     1,    16,      0,  PF_FLOAT | PF_CTL_KNOB | PF_SCALE_LOG | PF_UNIT_COEF, NULL, "bits", "Bit Reduction" },
    { 0.5,   0,     1,      0,  PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB | PF_UNIT_COEF , NULL, "morph", "Morph" },
    { 0,     0,     1,      0,  PF_ENUM | PF_CTL_COMBO, crusher_mode_names, "mode", "Mode" },
    { 1,     0.25,  4,      0,  PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_DB | PF_PROP_NOBOUNDS, NULL, "dc", "DC" },
    { 0.5,   0,     1,      0,  PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB | PF_UNIT_COEF , NULL, "anti_aliasing", "Anti-Aliasing" },
    { 1,     1,   250,      0,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_COEF , NULL, "samples", "Sample Reduction" },
    { 0,     0,     1,      0,  PF_BOOL | PF_CTL_TOGGLE, NULL, "lfo", "LFO Active" },
    { 20,    1,   250,      0,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_COEF , NULL, "lforange", "LFO Depth" },
    { 0.3, 0.01,  200,      0,  PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL, "lforate", "LFO Rate" },
    {}
};

CALF_PLUGIN_INFO(crusher) = { 0x8587, "Crusher", "Calf Crusher", "Calf Studio Gear", calf_plugins::calf_copyright_info, "DistortionPlugin" };

////////////////////////////////////////////////////////////////////////////

CALF_PORT_NAMES(monosynth) = {
    "Out L", "Out R",
};

const char *monosynth_waveform_names[] = { "Sawtooth", "Square", "Pulse", "Sine", "Triangle", "Varistep", "Skewed Saw", "Skewed Square",
    "Smooth Brass", "Bass", "Dark FM", "Multiwave", "Bell FM", "Dark Pad", "DCO Saw", "DCO Maze" };
const char *monosynth_mode_names[] = { "0\xC2\xB0 : 0\xC2\xB0", "0\xC2\xB0 : 180\xC2\xB0", "0\xC2\xB0 : 90\xC2\xB0", "90\xC2\xB0 : 90\xC2\xB0", "90\xC2\xB0 : 270\xC2\xB0", "Random" };
const char *monosynth_legato_names[] = { "Retrig", "Legato", "Fng Retrig", "Fng Legato" };
const char *monosynth_lfotrig_names[] = { "Retrig", "Free" };

const char *monosynth_filter_choices[] = {
    "12dB/oct Lowpass",
    "24dB/oct Lowpass",
    "2x12dB/oct Lowpass",
    "12dB/oct Highpass",
    "Lowpass+Notch",
    "Highpass+Notch",
    "6dB/oct Bandpass",
    "2x6dB/oct Bandpass",
};

CALF_PLUGIN_INFO(monosynth) = { 0x8480, "Monosynth", "Calf Monosynth", "Calf Studio Gear", calf_plugins::calf_copyright_info, "InstrumentPlugin" };

CALF_PORT_PROPS(monosynth) = {
    { monosynth_metadata::wave_saw,         0, monosynth_metadata::wave_count - 1, 1, PF_ENUM | PF_CTL_COMBO | PF_PROP_GRAPH, monosynth_waveform_names, "o1_wave", "Osc1 Wave" },
    { monosynth_metadata::wave_sqr,         0, monosynth_metadata::wave_count - 1, 1, PF_ENUM | PF_CTL_COMBO | PF_PROP_GRAPH, monosynth_waveform_names, "o2_wave", "Osc2 Wave" },

    { 0,         -1,    1,  0.1, PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB, NULL, "o1_pw", "Osc1 PW" },
    { 0,         -1,    1,  0.1, PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB, NULL, "o2_pw", "Osc2 PW" },

    { 10,         0,  100,    0, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_CENTS, NULL, "o12_detune", "O1<>2 Detune" },
    { 12,       -24,   24,    0, PF_INT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_SEMITONES, NULL, "o2_xpose", "Osc2 Transpose" },
    { 0,          0,    5,    0, PF_ENUM | PF_CTL_COMBO, monosynth_mode_names, "phase_mode", "Phase mode" },
    { 0.5,        0,    1,    0, PF_FLOAT | PF_SCALE_PERC, NULL, "o12_mix", "O1<>2 Mix" },
    { 1,          0,    7,    0, PF_ENUM | PF_CTL_COMBO | PF_PROP_GRAPH, monosynth_filter_choices, "filter", "Filter" },
    { 33,        10,16000,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL, "cutoff", "Cutoff" },
    { 3,        0.7,    8,    0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB, NULL, "res", "Resonance" },
    { 0,      -2400, 2400,    0, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_CENTS, NULL, "filter_sep", "Separation" },
    { 8000,  -10800,10800,    0, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_CENTS, NULL, "env2cutoff", "Env->Cutoff" },
    { 1,          0,    1,    0, PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB, NULL, "env2res", "Env->Res" },
    { 0,          0,    1,    0, PF_BOOL | PF_CTL_TOGGLE, NULL, "env2amp", "Env->Amp" },

    { 1,          1,20000,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_FADER | PF_UNIT_MSEC, NULL, "adsr_a", "EG1 Attack" },
    { 350,       10,20000,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_FADER | PF_UNIT_MSEC, NULL, "adsr_d", "EG1 Decay" },
    { 0.5,        0,    1,    0, PF_FLOAT | PF_SCALE_PERC, NULL, "adsr_s", "EG1 Sustain" },
    { 0,     -10000,10000,   21, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_FADER | PF_UNIT_MSEC, NULL, "adsr_f", "EG1 Fade" },
    { 100,       10,20000,     0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_FADER | PF_UNIT_MSEC, NULL, "adsr_r", "EG1 Release" },

    { 0,          0,    2,    0, PF_FLOAT | PF_SCALE_PERC, NULL, "key_follow", "Key Follow" },
    { 0,          0,    3,    0, PF_ENUM | PF_CTL_COMBO, monosynth_legato_names, "legato", "Legato Mode" },
    { 1,          1, 2000,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "portamento", "Portamento" },

    { 0.5,        0,    1,  0.1, PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB, NULL, "vel2filter", "Vel->Filter" },
    { 0,          0,    1,  0.1, PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB, NULL, "vel2amp", "Vel->Amp" },

    { 0.5,         0,   1, 100, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_PROP_OUTPUT_GAIN, NULL, "master", "Volume" },

    { 200,         0, 2400,   25, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_CENTS, NULL, "pbend_range", "PBend Range" },

    { 5,       0.01, 20,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL, "lfo_rate", "LFO1 Rate" },
    { 0.5,        0,  5,    0, PF_FLOAT | PF_SCALE_QUAD | PF_CTL_KNOB | PF_UNIT_SEC, NULL, "lfo_delay", "LFO1 Delay" },
    { 0,      -4800, 4800,  0, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_CENTS, NULL, "lfo2filter", "LFO1->Filter" },
    { 100,        0, 1200,  0, PF_FLOAT | PF_SCALE_QUAD | PF_CTL_KNOB | PF_UNIT_CENTS, NULL, "lfo2pitch", "LFO1->Pitch" },
    { 0,          0,    1,  0.1, PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB, NULL, "lfo2pw", "LFO1->PW" },
    { 1,          0,    1,  0.1, PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB, NULL, "mwhl2lfo", "ModWheel->LFO1" },

    { 1,          0,    1,    0, PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB, NULL, "scale_detune", "Scale Detune" },

    { 0,  -10800,10800,    0, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_CENTS, NULL, "adsr2_cutoff", "EG2->Cutoff" },
    { 0.3,        0,    1,    0, PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB, NULL, "adsr2_res", "EG2->Res" },
    { 1,          0,    1,    0, PF_BOOL | PF_CTL_TOGGLE, NULL, "adsr2_amp", "EG2->Amp" },

    { 1,          1,20000,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_FADER | PF_UNIT_MSEC, NULL, "adsr2_a", "EG2 Attack" },
    { 100,       10,20000,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_FADER | PF_UNIT_MSEC, NULL, "adsr2_d", "EG2 Decay" },
    { 0.5,        0,    1,    0, PF_FLOAT | PF_SCALE_PERC, NULL, "adsr2_s", "EG2 Sustain" },
    { 0,     -10000,10000,   21, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_FADER | PF_UNIT_MSEC, NULL, "adsr2_f", "EG2 Fade" },
    { 50,       10,20000,     0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_FADER | PF_UNIT_MSEC, NULL, "adsr2_r", "Release" },

    { 1,          1,   16,    0, PF_FLOAT | PF_SCALE_LOG | PF_UNIT_COEF | PF_CTL_KNOB, NULL, "o1_stretch", "Osc1 Stretch" },
    { 0,          0,    1,    0, PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB, NULL, "o1_window", "Osc1 Window" },

    { 0,          0,    1,    0, PF_ENUM | PF_CTL_COMBO, monosynth_lfotrig_names, "lfo1_trig", "LFO1 Trigger Mode" },
    { 0,          0,    1,    0, PF_ENUM | PF_CTL_COMBO, monosynth_lfotrig_names, "lfo2_trig", "LFO2 Trigger Mode" },
    { 5,       0.01, 20,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL, "lfo2_rate", "LFO1 Rate" },
    { 0.5,      0.1,  5,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_SEC, NULL, "lfo2_delay", "LFO1 Delay" },
    { 0,          0,    1,    0, PF_FLOAT | PF_CTL_KNOB | PF_SCALE_PERC, NULL, "o2_unison", "Osc2 Unison" },
    { 2,       0.01, 20,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL, "o2_unisonfrq", "Osc2 Unison Detune" },
    { 0,       -24,   24,    0, PF_INT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_SEMITONES, NULL, "o1_xpose", "Osc1 Transpose" },
    
    { 0,       0,   16,    0, PF_INT | PF_SCALE_LINEAR | PF_CTL_KNOB , NULL, "midi", "MIDI Channel" },
    
    {}
};

static const char *monosynth_mod_src_names[] = {
    "None",
    "Velocity",
    "Pressure",
    "ModWheel",
    "Envelope 1",
    "Envelope 2",
    "LFO 1",
    "LFO 2",
    NULL
};

static const char *monosynth_mod_dest_names[] = {
    "None",
    "Attenuation",
    "Osc Mix Ratio (%)",
    "Cutoff [ct]",
    "Resonance",
    "O1: Detune [ct]",
    "O2: Detune [ct]",
    "O1: PW (%)",
    "O2: PW (%)",
    "O1: Stretch",
    "O2: Unison Amt (%)",
    "O2: Unison Detune (log2)",
    NULL
};

monosynth_metadata::monosynth_metadata()
: mm_metadata(mod_matrix_slots, monosynth_mod_src_names, monosynth_mod_dest_names)
{
}

void monosynth_metadata::get_configure_vars(vector<string> &names) const
{
    mm_metadata.get_configure_vars(names);
}

////////////////////////////////////////////////////////////////////////////

CALF_PLUGIN_INFO(organ) = { 0x8481, "Organ", "Calf Organ", "Calf Studio Gear", calf_plugins::calf_copyright_info, "InstrumentPlugin" };

plugin_command_info *organ_metadata::get_commands() const
{
    static plugin_command_info cmds[] = {
        { "cmd_panic", "Panic!", "Stop all sounds and reset all controllers" },
        { NULL }
    };
    return cmds;
}

CALF_PORT_NAMES(organ) = {"Out L", "Out R"};

const char *organ_percussion_trigger_names[] = { "First note", "Each note", "Each, no retrig", "Polyphonic" };

const char *organ_wave_names[] = {
    "Sin",
    "S0", "S00", "S000",
    "SSaw", "SSqr", "SPls",
    "Saw", "Sqr", "Pls",
    "S(", "Sq(", "S+", "Clvg",
    "Bell", "Bell2",
    "W1", "W2", "W3", "W4", "W5", "W6", "W7", "W8", "W9",
    "DSaw", "DSqr", "DPls",
    "P:SynS","P:WideS","P:Sine","P:Bell","P:Space","P:Voice","P:Hiss","P:Chant",
};

const char *organ_routing_names[] = { "Out", "Flt 1", "Flt 2"  };

const char *organ_ampctl_names[] = { "None", "Direct", "Flt 1", "Flt 2", "All"  };

const char *organ_vibrato_mode_names[] = { "None", "Direct", "Flt 1", "Flt 2", "Voice", "Global"  };

const char *organ_vibrato_type_names[] = { "Allpass", "Scanner (V1/C1)", "Scanner (V2/C2)", "Scanner (V3/C3)", "Scanner (Full)"  };

const char *organ_filter_type_names[] = { "12dB/oct LP", "12dB/oct HP" };

const char *organ_filter_send_names[] = { "Output", "Filter 2" };

CALF_PORT_PROPS(organ) = {
    { 8,       0,  8, 80, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_FADER, NULL, "l1", "16'" },
    { 8,       0,  8, 80, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_FADER, NULL, "l2", "5 1/3'" },
    { 8,       0,  8, 80, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_FADER, NULL, "l3", "8'" },
    { 0,       0,  8, 80, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_FADER, NULL, "l4", "4'" },
    { 0,       0,  8, 80, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_FADER, NULL, "l5", "2 2/3'" },
    { 0,       0,  8, 80, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_FADER, NULL, "l6", "2'" },
    { 0,       0,  8, 80, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_FADER, NULL, "l7", "1 3/5'" },
    { 0,       0,  8, 80, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_FADER, NULL, "l8", "1 1/3'" },
    { 8,       0,  8, 80, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_FADER, NULL, "l9", "1'" },

    { 1,       1, 32, 32, PF_INT | PF_SCALE_LINEAR | PF_CTL_KNOB, NULL, "f1", "Freq 1" },
    { 3,       1, 32, 32, PF_INT | PF_SCALE_LINEAR | PF_CTL_KNOB, NULL, "f2", "Freq 2" },
    { 2,       1, 32, 32, PF_INT | PF_SCALE_LINEAR | PF_CTL_KNOB, NULL, "f3", "Freq 3" },
    { 4,       1, 32, 32, PF_INT | PF_SCALE_LINEAR | PF_CTL_KNOB, NULL, "f4", "Freq 4" },
    { 6,       1, 32, 32, PF_INT | PF_SCALE_LINEAR | PF_CTL_KNOB, NULL, "f5", "Freq 5" },
    { 8,       1, 32, 32, PF_INT | PF_SCALE_LINEAR | PF_CTL_KNOB, NULL, "f6", "Freq 6" },
    { 10,      1, 32, 32, PF_INT | PF_SCALE_LINEAR | PF_CTL_KNOB, NULL, "f7", "Freq 7" },
    { 12,      1, 32, 32, PF_INT | PF_SCALE_LINEAR | PF_CTL_KNOB, NULL, "f8", "Freq 8" },
    { 16,      1, 32, 32, PF_INT | PF_SCALE_LINEAR | PF_CTL_KNOB, NULL, "f9", "Freq 9" },

    { 0,       0,  organ_enums::wave_count - 1, 0, PF_ENUM | PF_SCALE_LINEAR | PF_CTL_COMBO, organ_wave_names, "w1", "Wave 1" },
    { 0,       0,  organ_enums::wave_count - 1, 0, PF_ENUM | PF_SCALE_LINEAR | PF_CTL_COMBO, organ_wave_names, "w2", "Wave 2" },
    { 0,       0,  organ_enums::wave_count - 1, 0, PF_ENUM | PF_SCALE_LINEAR | PF_CTL_COMBO, organ_wave_names, "w3", "Wave 3" },
    { 0,       0,  organ_enums::wave_count - 1, 0, PF_ENUM | PF_SCALE_LINEAR | PF_CTL_COMBO, organ_wave_names, "w4", "Wave 4" },
    { 0,       0,  organ_enums::wave_count - 1, 0, PF_ENUM | PF_SCALE_LINEAR | PF_CTL_COMBO, organ_wave_names, "w5", "Wave 5" },
    { 0,       0,  organ_enums::wave_count - 1, 0, PF_ENUM | PF_SCALE_LINEAR | PF_CTL_COMBO, organ_wave_names, "w6", "Wave 6" },
    { 0,       0,  organ_enums::wave_count - 1, 0, PF_ENUM | PF_SCALE_LINEAR | PF_CTL_COMBO, organ_wave_names, "w7", "Wave 7" },
    { 0,       0,  organ_enums::wave_count - 1, 0, PF_ENUM | PF_SCALE_LINEAR | PF_CTL_COMBO, organ_wave_names, "w8", "Wave 8" },
    { 0,       0,  organ_enums::wave_count - 1, 0, PF_ENUM | PF_SCALE_LINEAR | PF_CTL_COMBO, organ_wave_names, "w9", "Wave 9" },

    { 0,    -100,100, 401, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_CENTS, NULL, "detune1", "Detune 1" },
    { 0,    -100,100, 401, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_CENTS, NULL, "detune2", "Detune 2" },
    { 0,    -100,100, 401, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_CENTS, NULL, "detune3", "Detune 3" },
    { 0,    -100,100, 401, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_CENTS, NULL, "detune4", "Detune 4" },
    { 0,    -100,100, 401, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_CENTS, NULL, "detune5", "Detune 5" },
    { 0,    -100,100, 401, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_CENTS, NULL, "detune6", "Detune 6" },
    { 0,    -100,100, 401, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_CENTS, NULL, "detune7", "Detune 7" },
    { 0,    -100,100, 401, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_CENTS, NULL, "detune8", "Detune 8" },
    { 0,    -100,100, 401, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_CENTS, NULL, "detune9", "Detune 9" },

    { 0,       0,360, 361, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_DEG, NULL, "phase1", "Phase 1" },
    { 0,       0,360, 361, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_DEG, NULL, "phase2", "Phase 2" },
    { 0,       0,360, 361, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_DEG, NULL, "phase3", "Phase 3" },
    { 0,       0,360, 361, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_DEG, NULL, "phase4", "Phase 4" },
    { 0,       0,360, 361, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_DEG, NULL, "phase5", "Phase 5" },
    { 0,       0,360, 361, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_DEG, NULL, "phase6", "Phase 6" },
    { 0,       0,360, 361, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_DEG, NULL, "phase7", "Phase 7" },
    { 0,       0,360, 361, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_DEG, NULL, "phase8", "Phase 8" },
    { 0,       0,360, 361, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_DEG, NULL, "phase9", "Phase 9" },

    { 0,      -1,  1, 201, PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB, NULL, "pan1", "Pan 1" },
    { 0,      -1,  1, 201, PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB, NULL, "pan2", "Pan 2" },
    { 0,      -1,  1, 201, PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB, NULL, "pan3", "Pan 3" },
    { 0,      -1,  1, 201, PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB, NULL, "pan4", "Pan 4" },
    { 0,      -1,  1, 201, PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB, NULL, "pan5", "Pan 5" },
    { 0,      -1,  1, 201, PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB, NULL, "pan6", "Pan 6" },
    { 0,      -1,  1, 201, PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB, NULL, "pan7", "Pan 7" },
    { 0,      -1,  1, 201, PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB, NULL, "pan8", "Pan 8" },
    { 0,      -1,  1, 201, PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB, NULL, "pan9", "Pan 9" },

    { 0,       0,  2, 0, PF_ENUM | PF_SCALE_LINEAR | PF_CTL_COMBO, organ_routing_names, "routing1", "Routing 1" },
    { 0,       0,  2, 0, PF_ENUM | PF_SCALE_LINEAR | PF_CTL_COMBO, organ_routing_names, "routing2", "Routing 2" },
    { 0,       0,  2, 0, PF_ENUM | PF_SCALE_LINEAR | PF_CTL_COMBO, organ_routing_names, "routing3", "Routing 3" },
    { 0,       0,  2, 0, PF_ENUM | PF_SCALE_LINEAR | PF_CTL_COMBO, organ_routing_names, "routing4", "Routing 4" },
    { 0,       0,  2, 0, PF_ENUM | PF_SCALE_LINEAR | PF_CTL_COMBO, organ_routing_names, "routing5", "Routing 5" },
    { 0,       0,  2, 0, PF_ENUM | PF_SCALE_LINEAR | PF_CTL_COMBO, organ_routing_names, "routing6", "Routing 6" },
    { 0,       0,  2, 0, PF_ENUM | PF_SCALE_LINEAR | PF_CTL_COMBO, organ_routing_names, "routing7", "Routing 7" },
    { 0,       0,  2, 0, PF_ENUM | PF_SCALE_LINEAR | PF_CTL_COMBO, organ_routing_names, "routing8", "Routing 8" },
    { 0,       0,  2, 0, PF_ENUM | PF_SCALE_LINEAR | PF_CTL_COMBO, organ_routing_names, "routing9", "Routing 9" },

    { 96 + 12,      0,  127, 128, PF_INT | PF_CTL_KNOB | PF_UNIT_NOTE, NULL, "foldnote", "Foldover" },

    { 200,         10,  3000, 100, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "perc_decay", "P: Carrier Decay" },
    { 0.25,      0,  1, 100, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB, NULL, "perc_level", "P: Level" },
    { 0,         0,  organ_enums::wave_count_small - 1, 1, PF_ENUM | PF_CTL_COMBO, organ_wave_names, "perc_waveform", "P: Carrier Wave" },
    { 6,      1, 32, 32, PF_INT | PF_SCALE_LINEAR | PF_CTL_KNOB, NULL, "perc_harmonic", "P: Carrier Frq" },
    { 0,          0,    1,    0, PF_FLOAT | PF_SCALE_PERC, NULL, "perc_vel2amp", "P: Vel->Amp" },

    { 200,         10,  3000, 100, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "perc_fm_decay", "P: Modulator Decay" },
    { 0,          0,    4,    0, PF_FLOAT | PF_SCALE_PERC, NULL, "perc_fm_depth", "P: FM Depth" },
    { 0,         0,  organ_enums::wave_count_small - 1, 1, PF_ENUM | PF_CTL_COMBO, organ_wave_names, "perc_fm_waveform", "P: Modulator Wave" },
    { 6,      1, 32, 32, PF_INT | PF_SCALE_LINEAR | PF_CTL_KNOB, NULL, "perc_fm_harmonic", "P: Modulator Frq" },
    { 0,          0,    1,    0, PF_FLOAT | PF_SCALE_PERC, NULL, "perc_vel2fm", "P: Vel->FM" },

    { 0,         0,  organ_enums::perctrig_count - 1, 0, PF_ENUM | PF_CTL_COMBO, organ_percussion_trigger_names, "perc_trigger", "P: Trigger" },
    { 90,      0,360, 361, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_DEG, NULL, "perc_stereo", "P: Stereo Phase" },

    { 0,         0,  1, 0, PF_ENUM | PF_CTL_COMBO, organ_filter_send_names, "filter_chain", "Filter 1 To" },
    { 0,         0,  1, 0, PF_ENUM | PF_CTL_COMBO, organ_filter_type_names, "filter1_type", "Filter 1 Type" },
    { 0.1,         0,  1, 100, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_PROP_OUTPUT_GAIN | PF_PROP_GRAPH, NULL, "master", "Volume" },

    { 2000,   20, 20000, 100, PF_FLOAT | PF_SCALE_LOG | PF_UNIT_HZ | PF_CTL_KNOB, NULL, "f1_cutoff", "F1 Cutoff" },
    { 2,        0.7,    8,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB, NULL, "f1_res", "F1 Res" },
    { 8000,  -10800,10800,    0, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_CENTS, NULL, "f1_env1", "F1 Env1" },
    { 0,     -10800,10800,    0, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_CENTS, NULL, "f1_env2", "F1 Env2" },
    { 0,     -10800,10800,    0, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_CENTS, NULL, "f1_env3", "F1 Env3" },
    { 0,          0,    2,    0, PF_FLOAT | PF_SCALE_PERC, NULL, "f1_keyf", "F1 KeyFollow" },

    { 2000,   20, 20000, 100, PF_FLOAT | PF_SCALE_LOG | PF_UNIT_HZ | PF_CTL_KNOB, NULL, "f2_cutoff", "F2 Cutoff" },
    { 2,        0.7,    8,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB, NULL, "f2_res", "F2 Res" },
    { 0,     -10800,10800,    0, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_CENTS, NULL, "f2_env1", "F2 Env1" },
    { 8000,  -10800,10800,    0, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_CENTS, NULL, "f2_env2", "F2 Env2" },
    { 0,     -10800,10800,    0, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_CENTS, NULL, "f2_env3", "F2 Env3" },
    { 0,          0,    2,    0, PF_FLOAT | PF_SCALE_PERC, NULL, "f2_keyf", "F2 KeyFollow" },

    { 1,          1,20000,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "adsr_a", "EG1 Attack" },
    { 350,       10,20000,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "adsr_d", "EG1 Decay" },
    { 0.5,        0,    1,    0, PF_FLOAT | PF_SCALE_PERC, NULL, "adsr_s", "EG1 Sustain" },
    { 50,       10,20000,     0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "adsr_r", "EG1 Release" },
    { 0,          0,    1,    0, PF_FLOAT | PF_SCALE_PERC, NULL, "adsr_v", "EG1 VelMod" },
    { 0,  0, organ_enums::ampctl_count - 1,
                              0, PF_INT | PF_CTL_COMBO, organ_ampctl_names, "eg1_amp_ctl", "EG1 To Amp"},

    { 1,          1,20000,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "adsr2_a", "EG2 Attack" },
    { 350,       10,20000,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "adsr2_d", "EG2 Decay" },
    { 0.5,        0,    1,    0, PF_FLOAT | PF_SCALE_PERC, NULL, "adsr2_s", "EG2 Sustain" },
    { 50,       10,20000,     0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "adsr2_r", "EG2 Release" },
    { 0,          0,    1,    0, PF_FLOAT | PF_SCALE_PERC, NULL, "adsr2_v", "EG2 VelMod" },
    { 0,  0, organ_enums::ampctl_count - 1,
                              0, PF_INT | PF_CTL_COMBO, organ_ampctl_names, "eg2_amp_ctl", "EG2 To Amp"},

    { 1,          1,20000,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "adsr3_a", "EG3 Attack" },
    { 350,       10,20000,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "adsr3_d", "EG3 Decay" },
    { 0.5,        0,    1,    0, PF_FLOAT | PF_SCALE_PERC, NULL, "adsr3_s", "EG3 Sustain" },
    { 50,       10,20000,     0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "adsr3_r", "EG3 Release" },
    { 0,          0,    1,    0, PF_FLOAT | PF_SCALE_PERC, NULL, "adsr3_v", "EG3 VelMod" },
    { 0,  0, organ_enums::ampctl_count - 1,
                              0, PF_INT | PF_CTL_COMBO, organ_ampctl_names, "eg3_amp_ctl", "EG3 To Amp"},

    { 6.6,     0.01,  240,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL, "vib_rate", "Vib Rate" },
    { 1.0,        0,    1,    0, PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB , NULL, "vib_amt", "Vib Mod Amt" },
    { 0.5,        0,    1,    0, PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB , NULL, "vib_wet", "Vib Wet" },
    { 180,        0,  360,    0, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_DEG, NULL, "vib_phase", "Vib Stereo" },
    { organ_enums::lfomode_global,    0,  organ_enums::lfomode_count - 1,    0, PF_ENUM | PF_CTL_COMBO, organ_vibrato_mode_names, "vib_mode", "Vib Mode" },
    { organ_enums::lfotype_cv3,       0,   organ_enums::lfotype_count - 1,    0, PF_ENUM | PF_CTL_COMBO, organ_vibrato_type_names, "vib_type", "Vib Type" },
//    { 0,  0, organ_enums::ampctl_count - 1,
//                              0, PF_INT | PF_CTL_COMBO, organ_ampctl_names, "vel_amp_ctl", "Vel To Amp"},

    { -12,        -24, 24,   49, PF_INT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_SEMITONES, NULL, "transpose", "Transpose" },
    { 0,       -100,  100,  201, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_CENTS, NULL, "detune", "Detune" },

    { 16,         1,   32,   32, PF_INT | PF_SCALE_LINEAR | PF_CTL_KNOB, NULL, "polyphony", "Polyphony" },

    { 1,          0,    1,    0, PF_BOOL | PF_CTL_TOGGLE, NULL, "quad_env", "Quadratic AmpEnv" },

    { 200,        0, 2400,   25, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_CENTS, NULL, "pbend_range", "PBend Range" },

    { 120,       20, 20000, 0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL, "bass_freq", "Bass Freq" },
    { 1,        0.1, 10,     0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_NOBOUNDS, NULL, "bass_gain", "Bass Gain" },
    { 6000,      20, 20000, 0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL, "treble_freq", "Treble Freq" },
    { 1,        0.1, 10,     0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_NOBOUNDS, NULL, "treble_gain", "Treble Gain" },
    
    { 0,       0,   16,    0, PF_INT | PF_SCALE_LINEAR | PF_CTL_KNOB , NULL, "midi", "MIDI Channel" },
};

void organ_metadata::get_configure_vars(vector<string> &names) const
{
    names.push_back("map_curve");
}

////////////////////////////////////////////////////////////////////////////

const char *fluidsynth_interpolation_names[] = { "None (zero-hold)", "Linear", "Cubic", "7-point" };

CALF_PORT_NAMES(fluidsynth) = {
    "Out L", "Out R",
};

CALF_PLUGIN_INFO(fluidsynth) = { 0x8700, "Fluidsynth", "Calf Fluidsynth", "Calf Studio Gear", calf_plugins::calf_copyright_info, "InstrumentPlugin" };

CALF_PORT_PROPS(fluidsynth) = {
    { 0.5,         0,   1, 100, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_PROP_OUTPUT_GAIN, NULL, "master", "Volume" },
    { 2,          0,    3,    0, PF_ENUM | PF_CTL_COMBO, fluidsynth_interpolation_names, "interpolation", "Interpolation" },
    { 1,          0,    1,    0, PF_BOOL | PF_CTL_TOGGLE, NULL, "reverb", "Reverb" },
    { 1,          0,    1,    0, PF_BOOL | PF_CTL_TOGGLE, NULL, "chorus", "Chorus" },
};

void fluidsynth_metadata::get_configure_vars(vector<string> &names) const
{
    names.push_back("soundfont");
    names.push_back("preset_key_set");
    for (int i = 1; i < 16; i++)
        names.push_back("preset_key_set" + calf_utils::i2s(i + 1));
}

////////////////////////////////////////////////////////////////////////////

const char *wavetable_names[] = {
    "Shiny1",
    "Shiny2",
    "Rezo",
    "Metal",
    "Bell",
    "Blah",
    "Pluck",
    "Stretch",
    "Stretch 2",
    "Hard Sync",
    "Hard Sync 2",
    "Soft Sync",
    "Bell 2",
    "Bell 3",
    "Tine",
    "Tine 2",
    "Clav",
    "Clav 2",
    "Gtr",
    "Gtr 2",
    "Gtr 3",
    "Gtr 4",
    "Gtr 5",
    "Reed",
    "Reed 2",
    "Silver",
    "Brass",
    "Multi",
    "Multi 2",
};

static const char *wavetable_mod_src_names[] = {
    "None",
    "Velocity",
    "Pressure",
    "ModWheel",
    "Env 1",
    "Env 2",
    "Env 3",
    "LFO 1",
    "LFO 2",
    "Key Flw",
    NULL
};

static const char *wavetable_mod_dest_names[] = {
    "None",
    "Attenuation",
    "Osc Mix Ratio (%)",
    "Cutoff [ct]",
    "Resonance",
    "O1: Shift (%)",
    "O2: Shift (%)",
    "O1: Detune [ct]",
    "O2: Detune [ct]",
    "Pitch [ct]",
    NULL
};

CALF_PORT_NAMES(wavetable) = {
    "Out L", "Out R",
};

CALF_PLUGIN_INFO(wavetable) = { 0x8701, "Wavetable", "Calf Wavetable", "Calf Studio Gear", calf_plugins::calf_copyright_info, "InstrumentPlugin" };

CALF_PORT_PROPS(wavetable) = {
    { wavetable_metadata::wt_count - 1,       0,  wavetable_metadata::wt_count - 1, 0, PF_ENUM | PF_SCALE_LINEAR | PF_CTL_COMBO, wavetable_names, "o1wave", "Osc1 Wave" },
    { 0.2,     -1,      1,  0, PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB | PF_UNIT_COEF, NULL, "o1offset", "Osc1 Ctl"},
    { 0,        -48,   48, 48*2+1, PF_INT   | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_SEMITONES, NULL, "o1trans", "Osc1 Transpose" },
    { 6,       -100,  100,      0, PF_INT   | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_CENTS, NULL, "o1detune", "Osc1 Detune" },
    { 0.1,      0,   1,     0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_NOBOUNDS, NULL, "o1level", "Osc1 Level" },

    { 0,       0,  wavetable_metadata::wt_count - 1, 0, PF_ENUM | PF_SCALE_LINEAR | PF_CTL_COMBO, wavetable_names, "o2wave", "Osc2 Wave" },
    { 0.4,     -1,      1,  0, PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB | PF_UNIT_COEF, NULL, "o2offset", "Osc2 Ctl"},
    { 0,        -48,   48, 48*2+1, PF_INT   | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_SEMITONES, NULL, "o2trans", "Osc2 Transpose" },
    { -6,     -100,  100,      0, PF_INT   | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_CENTS, NULL, "o2detune", "Osc2 Detune" },
    { 0,        0,   1,     0, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_COEF | PF_PROP_NOBOUNDS, NULL, "o2level", "Osc2 Level" },

    { 1,          1,20000,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "adsr_a", "EG1 Attack" },
    { 350,       10,20000,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "adsr_d", "EG1 Decay" },
    { 0.5,        0,    1,    0, PF_FLOAT | PF_SCALE_PERC, NULL, "adsr_s", "EG1 Sustain" },
    { 0,     -10000,10000,   21, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "adsr_f", "EG1 Fade" },
    { 50,       10,20000,     0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "adsr_r", "EG1 Release" },
    { 1,          0,    1,    0, PF_FLOAT | PF_SCALE_PERC, NULL, "adsr_v", "EG1 VelMod" },

    { 1,          1,20000,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "adsr2_a", "EG2 Attack" },
    { 350,       10,20000,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "adsr2_d", "EG2 Decay" },
    { 0.5,        0,    1,    0, PF_FLOAT | PF_SCALE_PERC, NULL, "adsr2_s", "EG2 Sustain" },
    { 0,     -10000,10000,   21, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "adsr2_f", "EG2 Fade" },
    { 50,       10,20000,     0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "adsr2_r", "EG2 Release" },
    { 1,          0,    1,    0, PF_FLOAT | PF_SCALE_PERC, NULL, "adsr2_v", "EG2 VelMod" },

    { 1,          1,20000,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "adsr3_a", "EG3 Attack" },
    { 350,       10,20000,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "adsr3_d", "EG3 Decay" },
    { 0.5,        0,    1,    0, PF_FLOAT | PF_SCALE_PERC, NULL, "adsr3_s", "EG3 Sustain" },
    { 0,     -10000,10000,   21, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "adsr3_f", "EG3 Fade" },
    { 50,       10, 20000,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "adsr3_r", "EG3 Release" },
    { 0,          0,    1,    0, PF_FLOAT | PF_SCALE_PERC, NULL, "adsr3_v", "EG3 VelMod" },

    { 200,        0, 2400,   25, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_CENTS, NULL, "pbend_range", "PBend Range" },
    { 1,          0,    1,    0, PF_BOOL | PF_CTL_TOGGLE, NULL, "eg1amp", "EG1->Amp" },
    { 5,       0.01, 20,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL, "lfo1_rate", "LFO1 Rate" },
    { 0.25,       0.01, 20,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL, "lfo2_rate", "LFO2 Rate" },
    
    { 0,       0,   16,    0, PF_INT | PF_SCALE_LINEAR | PF_CTL_KNOB , NULL, "midi", "MIDI Channel" },
    {}
};

wavetable_metadata::wavetable_metadata()
: mm_metadata(mod_matrix_slots, wavetable_mod_src_names, wavetable_mod_dest_names)
{
}

void wavetable_metadata::get_configure_vars(std::vector<std::string> &names) const
{
    mm_metadata.get_configure_vars(names);
}

////////////////////////////////////////////////////////////////////////////

CALF_PORT_NAMES(pitch) = {"In L", "In R", "Out L", "Out R"};
CALF_PORT_PROPS(pitch) = {
    { 0.9,  0.1,   1,    0, PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB, NULL, "pd_threshold", "Pitch Det:Peak Threshold" },
    { 1,      1,   8,    3, PF_INT | PF_CTL_KNOB, NULL, "pd_subdivide", "Pitch Det:Subdiv" },
    { 440,    427, 453,  0.1, PF_FLOAT | PF_CTL_KNOB | PF_UNIT_HZ, NULL, "tune", "Tune" },
    { 0,      0,   127,  1, PF_INT | PF_PROP_OUTPUT, NULL, "note", "MIDI Note" },
    { 0,     -100, 100,  1, PF_FLOAT | PF_PROP_OUTPUT, NULL, "cents", "Cents" },
    { 0,     -1, 1,      0, PF_FLOAT | PF_SCALE_PERC | PF_PROP_OUTPUT, NULL, "clarity", "Clarity" },
    { 1,     1, 20000,   1, PF_FLOAT | PF_PROP_OUTPUT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL, "freq", "Frequency" },
    {}
};

CALF_PLUGIN_INFO(pitch) = { 0x85AA, "Pitch", "Calf Pitch Tools", "Calf Studio Gear", calf_plugins::calf_copyright_info, "PitchPlugin" };

////////////////////////////////////////////////////////////////////////////

CALF_PORT_NAMES(widgets) = {"In L", "In R", "Out L", "Out R"};
CALF_PORT_PROPS(widgets) = {
    { 0,    0,   1,    0,   PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "meter_1", "Meter-1" },
    { 0,    0,   1,    0,   PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "meter_2", "Meter-2" },
    { 0,    0,   1,    0,   PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "meter_3", "Meter-3" },
    { 0,    0,   1,    0,   PF_FLOAT | PF_SCALE_GAIN | PF_CTL_METER | PF_CTLO_LABEL | PF_UNIT_DB | PF_PROP_OUTPUT | PF_PROP_OPTIONAL, NULL, "meter_4", "Meter-4" },

    { 0,    100, 100,  0.1, PF_FLOAT | PF_CTL_KNOB | PF_UNIT_COEF, NULL, "value_lin", "Value Linear" },
    { 0,    100, 100,  0.1, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_COEF, NULL, "value_log", "Value Logarithmic" },
    {}
};

CALF_PLUGIN_INFO(widgets) = { 0x85AB, "Widgets", "Calf Widget Test", "Calf Studio Gear", calf_plugins::calf_copyright_info, "UtilityPlugin" };


////////////////////////////////////////////////////////////////////////////

plugin_registry::plugin_registry()
{
    #define PER_MODULE_ITEM(name, isSynth, jackname) plugins.push_back((new name##_metadata));
    #include <calf/modulelist.h>
}

}
