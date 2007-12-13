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

const char *amp_audio_module::param_names[] = {"In L", "In R", "Out L", "Out R", "Gain"};

parameter_properties amp_audio_module::param_props[] = {
    { 1, 0, 4, 1.1, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB, NULL }
};

static synth::ladspa_info amp_info = { 0x847c, "Amp", "Calf Const Amp", "Krzysztof Foltman", copyright, "AmplifierPlugin" };

#if USE_LADSPA
synth::ladspa_wrapper<amp_audio_module> amp(amp_info);
#endif

////////////////////////////////////////////////////////////////////////////

const char *flanger_audio_module::param_names[] = {"In L", "In R", "Out L", "Out R", "Minimum delay", "Modulation depth", "Modulation rate", "Feedback", "Amount"};

parameter_properties flanger_audio_module::param_props[] = {
    { 0.1,      0.1, 10, 1.01, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL },
    { 0.5,      0.1, 10, 1.01, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL },
    { 0.25,    0.01, 20, 1.01, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL },
    { 0.90,   -0.99, 0.99, 1.01, PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB | PF_UNIT_COEF, NULL },
    { 1, 0, 2, 1.1, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_COEF, NULL },
};

static synth::ladspa_info flanger_info = { 0x847d, "Flanger", "Calf Flanger", "Krzysztof Foltman", copyright, "FlangerPlugin" };

#if USE_LADSPA
synth::ladspa_wrapper<flanger_audio_module> flanger(flanger_info);
#endif

////////////////////////////////////////////////////////////////////////////

const char *reverb_audio_module::param_names[] = {"In L", "In R", "Out L", "Out R", "Decay time", "HF Damp", "Amount"};

parameter_properties reverb_audio_module::param_props[] = {
    { 1.5,      1.0,  4.0, 1.01, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_SEC, NULL },
    { 5000,    2000,20000, 1.01, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL },
    { 0.25,       0,    2, 1.1, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_COEF, NULL },
};

static synth::ladspa_info reverb_info = { 0x847e, "Reverb", "Calf Reverb", "Krzysztof Foltman", copyright, "ReverbPlugin" };

#if USE_LADSPA
synth::ladspa_wrapper<reverb_audio_module> reverb(reverb_info);
#endif

////////////////////////////////////////////////////////////////////////////

const char *filter_audio_module::param_names[] = {"In L", "In R", "Out L", "Out R", "Frequency", "Resonance", "Mode", "Inertia"};

const char *filter_choices[] = {
    "12dB/oct Lowpass",
    "24dB/oct Lowpass",
    "36dB/oct Lowpass",
    "12dB/oct Highpass",
    "24dB/oct Highpass",
    "36dB/oct Highpass",
};

parameter_properties filter_audio_module::param_props[] = {
    { 2000,      10,20000, 1.01, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL },
    { 0.707,  0.707,   20,  1.1, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_UNIT_COEF, NULL },
    { 0,          0,    5,    1, PF_ENUM | PF_CTL_COMBO, filter_choices },
    { 20,         5,  100,    1, PF_INT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL},
};

static synth::ladspa_info filter_info = { 0x847f, "Filter", "Calf Filter", "Krzysztof Foltman", copyright, "FilterPlugin" };

#if USE_LADSPA
synth::ladspa_wrapper<filter_audio_module> filter(filter_info);
#endif

////////////////////////////////////////////////////////////////////////////
#ifdef ENABLE_EXPERIMENTAL
const char *organ_audio_module::param_names[] = {"Out L", "Out R", "16'", "5 1/3'", "8'", "4'", "2 2/3'", "2'", "1 3/5'", "1 1/3'", "1'", "Foldover", "Perc Mode", "Perc Harm", "Vibrato Speed", "Master Volume"};

const char *organ_percussion_mode_names[] = { "Off", "Short", "Long" };
const char *organ_percussion_harmonic_names[] = { "2nd", "3rd" };
const char *organ_vibrato_speed_names[] = { "Off", "Swell", "Tremolo", "HoldPedal", "ModWheel" };

parameter_properties organ_audio_module::param_props[] = {
    { 0.3,       0,  1, 1.01, PF_FLOAT | PF_SCALE_QUAD | PF_CTL_FADER, NULL },
    { 0.3,       0,  1, 1.01, PF_FLOAT | PF_SCALE_QUAD | PF_CTL_FADER, NULL },
    { 0.3,       0,  1, 1.01, PF_FLOAT | PF_SCALE_QUAD | PF_CTL_FADER, NULL },
    { 0,         0,  1, 1.01, PF_FLOAT | PF_SCALE_QUAD | PF_CTL_FADER, NULL },
    { 0,         0,  1, 1.01, PF_FLOAT | PF_SCALE_QUAD | PF_CTL_FADER, NULL },
    { 0,         0,  1, 1.01, PF_FLOAT | PF_SCALE_QUAD | PF_CTL_FADER, NULL },
    { 0,         0,  1, 1.01, PF_FLOAT | PF_SCALE_QUAD | PF_CTL_FADER, NULL },
    { 0,         0,  1, 1.01, PF_FLOAT | PF_SCALE_QUAD | PF_CTL_FADER, NULL },
    { 0,         0,  1, 1.01, PF_FLOAT | PF_SCALE_QUAD | PF_CTL_FADER, NULL },

    { 1,         0,  1, 1.01, PF_BOOL | PF_CTL_TOGGLE, NULL },
    { 1,         0,  2, 1.01, PF_ENUM | PF_CTL_COMBO, organ_percussion_mode_names },
    { 3,         2,  3, 1.01, PF_ENUM | PF_CTL_COMBO, organ_percussion_harmonic_names },
    { 1,         0,  4, 1.01, PF_ENUM | PF_CTL_COMBO, organ_vibrato_speed_names },

    { 0.2,         0,  1, 1.01, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB, NULL },
};

static synth::ladspa_info organ_info = { 0x8481, "Organ", "Calf Organ", "Krzysztof Foltman", copyright, "SynthesizerPlugin" };

#if USE_LADSPA
synth::ladspa_wrapper<organ_audio_module> organ(organ_info);
#endif

#endif
////////////////////////////////////////////////////////////////////////////
#ifdef ENABLE_EXPERIMENTAL

const char *monosynth_audio_module::param_names[] = {"Out L", "Out R", "Osc1 Wave", "Osc2 Wave", "Osc 1/2 Detune", "Osc 2 Transpose", "Phase Mode", "Osc Mix", "Filter", "Cutoff", "Resonance", "Separation", "Env->Cutoff", "Env->Res", "Decay", "Key Follow", "Legato", "Vel->Amp", "Vel->Filter"};

const char *monosynth_waveform_names[] = { "Sawtooth", "Square", "Pulse", "Sine", "Triangle" };
const char *monosynth_mode_names[] = { "0 : 0", "0 : 180", "0 : 90", "90 : 90", "90 : 270", "Random" };

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

parameter_properties monosynth_audio_module::param_props[] = {
    { wave_saw,         0,wave_count - 1, 1, PF_ENUM | PF_CTL_COMBO, monosynth_waveform_names },
    { wave_pulse,         0,wave_count - 1, 1, PF_ENUM | PF_CTL_COMBO, monosynth_waveform_names },
    { 10,         0,  100, 1.01, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_CENTS, NULL },
    { 12,       -24,   24, 1.01, PF_INT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_SEMITONES, NULL },
    { 0,          0,    5, 1.01, PF_ENUM | PF_CTL_COMBO, monosynth_mode_names },
    { 0.5,        0,    1, 1.01, PF_FLOAT | PF_SCALE_PERC, NULL },
    { 0,          0,    7, 1.01, PF_ENUM | PF_CTL_COMBO, monosynth_filter_choices },
    { 33,        10,16000, 1.01, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL },
    { 2,        0.7,    8, 1.01, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB, NULL },
    { 0,      -2400, 2400, 1.01, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_CENTS, NULL },
    { 8000,  -10800,10800, 1.01, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_CENTS, NULL },
    { 0.5,        0,    1, 1.01, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB, NULL },
    { 350,       10,20000, 1.01, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL },
    { 0,          0,    1, 1.01, PF_BOOL | PF_CTL_TOGGLE, NULL },
    { 0,          0,    1, 1.01, PF_BOOL | PF_CTL_TOGGLE, NULL },
    { 0,          0,    1,  0.1, PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB, NULL },
    { 0,          0,    1,  0.1, PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB, NULL },
};

static synth::ladspa_info monosynth_info = { 0x8480, "Monosynth", "Calf Monosynth", "Krzysztof Foltman", copyright, "SynthesizerPlugin" };

#if USE_LADSPA
synth::ladspa_wrapper<monosynth_audio_module> monosynth(monosynth_info);
#endif

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
        default: return NULL;
    }
}

};

#if USE_DSSI && ENABLE_EXPERIMENTAL
extern "C" {

const DSSI_Descriptor *dssi_descriptor(unsigned long Index)
{
    switch (Index) {
        case 0: return &::monosynth.dssi_descriptor;
        case 1: return &::organ.dssi_descriptor;
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
    
    return rdf;
}
#endif
