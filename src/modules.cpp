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
const char *monosynth_audio_module::param_names[] = {"Out L", "Out R", "Cutoff", "Resonance", "Filter Env", "Decay"};

parameter_properties monosynth_audio_module::param_props[] = {
    { 33,        10,16000, 1.01, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL },
    { 2,        0.7,    4, 1.01, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB, NULL },
    { 8000,       0,10800, 1.01, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_CENTS, NULL },
    { 350,      10,20000, 1.01, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL },
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

std::string synth::get_builtin_modules_rdf()
{
    std::string rdf;
    
    rdf += ::flanger.generate_rdf();
    rdf += ::reverb.generate_rdf();
    rdf += ::filter.generate_rdf();
    
    return rdf;
}
#endif
