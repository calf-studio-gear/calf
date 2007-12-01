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
#include <jack/jack.h>
#include <calf/giface.h>
#include <calf/modules.h>

const char *copyright = "(C) 2001-2007 Krzysztof Foltman, license: LGPL";

const char *amp_audio_module::param_names[] = {"In L", "In R", "Out L", "Out R", "Gain"};

parameter_properties amp_audio_module::param_props[] = {
    { 1, 0, 4, 1.1, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB, NULL }
};

static synth::ladspa_info amp_info = { 0x847c, "Amp", "Calf Const Amp", "Krzysztof Foltman", copyright, NULL };

synth::ladspa_wrapper<amp_audio_module> amp(amp_info);

////////////////////////////////////////////////////////////////////////////

const char *flanger_audio_module::param_names[] = {"In L", "In R", "Out L", "Out R", "Min. delay", "Mod. depth", "Mod. rate", "Feedback", "Amount"};

parameter_properties flanger_audio_module::param_props[] = {
    { 0.1,      0.1, 10, 1.01, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB, NULL },
    { 0.5,      0.1, 10, 1.01, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB, NULL },
    { 0.25,    0.01, 20, 1.01, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB, NULL },
    { 0.90,   -0.99, 0.99, 1.01, PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB, NULL },
    { 1, 0, 2, 1.1, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB, NULL },
};

static synth::ladspa_info flanger_info = { 0x847d, "Flanger", "Calf Flanger", "Krzysztof Foltman", copyright, NULL };

synth::ladspa_wrapper<flanger_audio_module> flanger(flanger_info);

////////////////////////////////////////////////////////////////////////////

const char *reverb_audio_module::param_names[] = {"In L", "In R", "Out L", "Out R", "Dec. time", "HF Damp", "Amount"};

parameter_properties reverb_audio_module::param_props[] = {
    { 1.5,      1.0,  4.0, 1.01, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB, NULL },
    { 5000,    2000,20000, 1.01, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB, NULL },
    { 0.25,       0,    2, 1.1, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB, NULL },
};

static synth::ladspa_info reverb_info = { 0x847e, "Reverb", "Calf Reverb", "Krzysztof Foltman", copyright, NULL };

synth::ladspa_wrapper<reverb_audio_module> reverb(reverb_info);

////////////////////////////////////////////////////////////////////////////

const char *filter_audio_module::param_names[] = {"In L", "In R", "Out L", "Out R", "Cutoff", "Resonance", "Mode"};

parameter_properties filter_audio_module::param_props[] = {
    { 2000,      20,20000, 1.01, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB, NULL },
    { 0.707,  0.707,   20,  1.1, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB, NULL },
    { 0,          0,    1,    1, PF_INT | PF_CTL_COMBO, "Lowpass\0Highpass\0" },
};

static synth::ladspa_info filter_info = { 0x847f, "Filter", "Calf Filter", "Krzysztof Foltman", copyright, NULL };

synth::ladspa_wrapper<filter_audio_module> filter(filter_info);

////////////////////////////////////////////////////////////////////////////

extern "C" {

const LADSPA_Descriptor *ladspa_descriptor(unsigned long Index)
{
    switch (Index) {
        case 0: return &filter.descriptor;
        case 1: return &flanger.descriptor;
        case 2: return &reverb.descriptor;
        default: return NULL;
    }
}

};

