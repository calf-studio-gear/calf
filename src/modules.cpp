/* Calf DSP Library
 * Example audio modules - parameters and LADSPA wrapper instantiation
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
#include <assert.h>
#include <memory.h>
#if USE_JACK
#include <jack/jack.h>
#endif
#include <calf/giface.h>
#include <calf/modules.h>
#include <calf/modules_dev.h>

using namespace synth;

const char *copyright = "(C) 2001-2007 Krzysztof Foltman, license: LGPL";

const char *amp_audio_module::port_names[] = {"In L", "In R", "Out L", "Out R"};

parameter_properties amp_audio_module::param_props[] = {
    { 1, 0, 4, 1.1, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB, NULL, "gain", "Gain" }
};

static synth::ladspa_info amp_info = { 0x847c, "Amp", "Calf Const Amp", "Krzysztof Foltman", copyright, "AmplifierPlugin" };

#if USE_LADSPA
static synth::ladspa_wrapper<amp_audio_module> amp(amp_info);
#endif

////////////////////////////////////////////////////////////////////////////

const char *flanger_audio_module::port_names[] = {"In L", "In R", "Out L", "Out R"};

parameter_properties flanger_audio_module::param_props[] = {
    { 0.1,      0.1, 10, 1.01, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "min_delay", "Minimum delay" },
    { 0.5,      0.1, 10, 1.01, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "mod_depth", "Modulation depth" },
    { 0.25,    0.01, 20, 1.01, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL, "mod_rate", "Modulation rate" },
    { 0.90,   -0.99, 0.99, 1.01, PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB | PF_UNIT_COEF, NULL, "feedback", "Feedback" },
    { 1, 0, 2, 1.1, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_COEF, NULL, "amount", "Amount" },
};

static synth::ladspa_info flanger_info = { 0x847d, "Flanger", "Calf Flanger", "Krzysztof Foltman", copyright, "FlangerPlugin" };

#if USE_LADSPA
static synth::ladspa_wrapper<flanger_audio_module> flanger(flanger_info);
#endif

////////////////////////////////////////////////////////////////////////////

const char *reverb_audio_module::port_names[] = {"In L", "In R", "Out L", "Out R"};

parameter_properties reverb_audio_module::param_props[] = {
    { 1.5,      1.0,  4.0, 1.01, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_SEC, NULL, "decay_time", "Decay time" },
    { 5000,    2000,20000, 1.01, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL, "hf_damp", "High Frq Damp" },
    { 0.25,       0,    2, 1.1, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_COEF, NULL, "amount", "Amount" },
};

static synth::ladspa_info reverb_info = { 0x847e, "Reverb", "Calf Reverb", "Krzysztof Foltman", copyright, "ReverbPlugin" };

#if USE_LADSPA
static synth::ladspa_wrapper<reverb_audio_module> reverb(reverb_info);
#endif

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
    { 2000,      10,20000, 1.01, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL, "freq", "Frequency" },
    { 0.707,  0.707,   20,  1.1, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_COEF, NULL, "res", "Resonance" },
    { 0,          0,    5,    1, PF_ENUM | PF_CTL_COMBO, filter_choices, "mode", "Mode" },
    { 20,         5,  100,    1, PF_INT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "inertia", "Inertia"},
};

static synth::ladspa_info filter_info = { 0x847f, "Filter", "Calf Filter", "Krzysztof Foltman", copyright, "FilterPlugin" };

#if USE_LADSPA
static synth::ladspa_wrapper<filter_audio_module> filter(filter_info);
#endif

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
    { 120,      30,    300, 1.01, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_BPM, NULL, "bpm", "Tempo" },
    {  4,        1,    16,    1, PF_INT | PF_SCALE_LINEAR | PF_CTL_FADER, NULL, "subdiv", "Subdivide"},
    {  3,        1,    16,    1, PF_INT | PF_SCALE_LINEAR | PF_CTL_FADER, NULL, "time_l", "Time L"},
    {  5,        1,    16,    1, PF_INT | PF_SCALE_LINEAR | PF_CTL_FADER, NULL, "time_r", "Time R"},
    { 0.5,        0,    1, 1.01, PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB, NULL, "feedback", "Feedback" },
    { 0.25,       0,    2, 1.1, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_COEF, NULL, "amount", "Amount" },
    { 1,         0,  1, 1.01, PF_ENUM | PF_CTL_COMBO, vintage_delay_mixmodes, "mix_mode", "Mix mode" },
    { 1,         0,  2, 1.01, PF_ENUM | PF_CTL_COMBO, vintage_delay_fbmodes, "medium", "Medium" },
};

static synth::ladspa_info vintage_delay_info = { 0x8482, "VintageDelay", "Calf Vintage Delay", "Krzysztof Foltman", copyright, "DelayPlugin" };

#if USE_LADSPA
static synth::ladspa_wrapper<vintage_delay_audio_module> vintage_delay(vintage_delay_info);
#endif

////////////////////////////////////////////////////////////////////////////
#ifdef ENABLE_EXPERIMENTAL
const char *organ_audio_module::port_names[] = {"Out L", "Out R"};

const char *organ_percussion_mode_names[] = { "Off", "Short", "Long" };
const char *organ_percussion_harmonic_names[] = { "2nd", "3rd" };
const char *organ_vibrato_speed_names[] = { "Off", "Swell", "Tremolo", "HoldPedal", "ModWheel" };

parameter_properties organ_audio_module::param_props[] = {
    { 0.3,       0,  1, 1.01, PF_FLOAT | PF_SCALE_QUAD | PF_CTL_FADER, NULL, "h1", "16'" },
    { 0.3,       0,  1, 1.01, PF_FLOAT | PF_SCALE_QUAD | PF_CTL_FADER, NULL, "h3", "5 1/3'" },
    { 0.3,       0,  1, 1.01, PF_FLOAT | PF_SCALE_QUAD | PF_CTL_FADER, NULL, "h2", "8'" },
    { 0,         0,  1, 1.01, PF_FLOAT | PF_SCALE_QUAD | PF_CTL_FADER, NULL, "h4", "4'" },
    { 0,         0,  1, 1.01, PF_FLOAT | PF_SCALE_QUAD | PF_CTL_FADER, NULL, "h6", "2 2/3'" },
    { 0,         0,  1, 1.01, PF_FLOAT | PF_SCALE_QUAD | PF_CTL_FADER, NULL, "h8", "2'" },
    { 0,         0,  1, 1.01, PF_FLOAT | PF_SCALE_QUAD | PF_CTL_FADER, NULL, "h10", "1 3/5'" },
    { 0,         0,  1, 1.01, PF_FLOAT | PF_SCALE_QUAD | PF_CTL_FADER, NULL, "h12", "1 1/3'" },
    { 0,         0,  1, 1.01, PF_FLOAT | PF_SCALE_QUAD | PF_CTL_FADER, NULL, "h16", "1'" },

    { 1,         0,  1, 1.01, PF_BOOL | PF_CTL_TOGGLE, NULL, "foldover", "Foldover" },
    { 1,         0,  2, 1.01, PF_ENUM | PF_CTL_COMBO, organ_percussion_mode_names, "perc_mode", "Perc. mode" },
    { 3,         2,  3, 1.01, PF_ENUM | PF_CTL_COMBO, organ_percussion_harmonic_names, "perc_hrm", "Perc. harmonic" },
    { 1,         0,  4, 1.01, PF_ENUM | PF_CTL_COMBO, organ_vibrato_speed_names, "vib_speed", "Vibrato mode" },

    { 0.2,         0,  1, 1.01, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB, NULL, "amount", "Amount" },
};

static synth::ladspa_info organ_info = { 0x8481, "Organ", "Calf Organ", "Krzysztof Foltman", copyright, "SynthesizerPlugin" };

#if USE_LADSPA
static synth::ladspa_wrapper<organ_audio_module> organ(organ_info);
#endif

#endif
////////////////////////////////////////////////////////////////////////////

static synth::ladspa_info monosynth_info = { 0x8480, "Monosynth", "Calf Monosynth", "Krzysztof Foltman", copyright, "SynthesizerPlugin" };

#if USE_LADSPA
static synth::ladspa_wrapper<monosynth_audio_module> monosynth(monosynth_info);
#endif

////////////////////////////////////////////////////////////////////////////

#if USE_LADSPA
extern "C" {

const LADSPA_Descriptor *ladspa_descriptor(unsigned long Index)
{
    switch (Index) {
        case 0: return &::filter.descriptor;
        case 1: return &::flanger.descriptor;
        case 2: return &::reverb.descriptor;
        case 3: return &::vintage_delay.descriptor;
        default: return NULL;
    }
}

};

#if USE_DSSI
extern "C" {

const DSSI_Descriptor *dssi_descriptor(unsigned long Index)
{
    switch (Index) {
        case 0: return &::filter.dssi_descriptor;
        case 1: return &::flanger.dssi_descriptor;
        case 2: return &::reverb.dssi_descriptor;
        case 3: return &::monosynth.dssi_descriptor;
        case 4: return &::vintage_delay.dssi_descriptor;
#ifdef ENABLE_EXPERIMENTAL
        case 5: return &::organ.dssi_descriptor;
#endif
        default: return NULL;
    }
}

};
#endif

std::string synth::get_builtin_modules_rdf()
{
    std::string rdf;
    
    rdf += ::flanger.generate_rdf();
    rdf += ::reverb.generate_rdf();
    rdf += ::filter.generate_rdf();
    rdf += ::vintage_delay.generate_rdf();
    rdf += ::monosynth.generate_rdf();
    
    return rdf;
}
#endif
