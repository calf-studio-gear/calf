/* Calf DSP Library
 * Example audio modules - organ
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
#include <config.h>

#include <assert.h>
#include <memory.h>
#include <complex>
#if USE_JACK
#include <jack/jack.h>
#endif
#include <calf/giface.h>
#include <calf/modules.h>
#include <calf/modules_dev.h>

using namespace synth;
using namespace std;

const char *organ_audio_module::get_gui_xml()
{
    return 
    "<vbox border=\"10\">"
        "<hbox>"
            "<vbox>"
                "<label param=\"foldover\"/>"
                "<align><toggle param=\"foldover\"/></align>"
            "</vbox>"
            "<vbox>"
                "<label param=\"perc_decay\"/>"
                "<knob param=\"perc_decay\" expand=\"0\" fill=\"0\"/>"
                "<value param=\"perc_decay\"/>"
            "</vbox>"
            "<vbox>"
                "<label param=\"perc_level\"/>"
                "<knob param=\"perc_level\" expand=\"0\" fill=\"0\"/>"
                "<value param=\"perc_level\"/>"
            "</vbox>"        
            "<vbox>"
                "<label param=\"perc_harm\"/>"
                "<combo param=\"perc_harm\"/>"
            "</vbox>"        
            "<vbox>"
                "<label param=\"master\"/>"
                "<knob param=\"master\"/>"
                "<value param=\"master\"/>"
            "</vbox>"
        "</hbox>"
        "<table rows=\"5\" cols=\"9\">"
            "<label  attach-x=\"0\" attach-y=\"0\" param=\"h1\"/>"
            "<vscale attach-x=\"0\" attach-y=\"1\" param=\"h1\"/>"
            "<value  attach-x=\"0\" attach-y=\"2\" param=\"h1\"/>"
            "<knob   attach-x=\"0\" attach-y=\"3\" param=\"f1\"/>"
            "<value  attach-x=\"0\" attach-y=\"4\" param=\"f1\"/>"
    
            "<label  attach-x=\"1\" attach-y=\"0\" param=\"h3\"/>"
            "<vscale attach-x=\"1\" attach-y=\"1\" param=\"h3\"/>"
            "<value  attach-x=\"1\" attach-y=\"2\" param=\"h3\"/>"
            "<knob   attach-x=\"1\" attach-y=\"3\" param=\"f2\"/>"
            "<value  attach-x=\"1\" attach-y=\"4\" param=\"f2\"/>"
    
            "<label  attach-x=\"2\" attach-y=\"0\" param=\"h2\"/>"
            "<vscale attach-x=\"2\" attach-y=\"1\" param=\"h2\"/>"
            "<value  attach-x=\"2\" attach-y=\"2\" param=\"h2\"/>"
            "<knob   attach-x=\"2\" attach-y=\"3\" param=\"f3\"/>"
            "<value  attach-x=\"2\" attach-y=\"4\" param=\"f3\"/>"
    
            "<label  attach-x=\"3\" attach-y=\"0\" param=\"h4\"/>"
            "<vscale attach-x=\"3\" attach-y=\"1\" param=\"h4\"/>"
            "<value  attach-x=\"3\" attach-y=\"2\" param=\"h4\"/>"
            "<knob   attach-x=\"3\" attach-y=\"3\" param=\"f4\"/>"
            "<value  attach-x=\"3\" attach-y=\"4\" param=\"f4\"/>"

            "<label  attach-x=\"4\" attach-y=\"0\" param=\"h6\"/>"
            "<vscale attach-x=\"4\" attach-y=\"1\" param=\"h6\"/>"
            "<value  attach-x=\"4\" attach-y=\"2\" param=\"h6\"/>"
            "<knob   attach-x=\"4\" attach-y=\"3\" param=\"f5\"/>"
            "<value  attach-x=\"4\" attach-y=\"4\" param=\"f5\"/>"
    
            "<label  attach-x=\"5\" attach-y=\"0\" param=\"h8\"/>"
            "<vscale attach-x=\"5\" attach-y=\"1\" param=\"h8\"/>"
            "<value  attach-x=\"5\" attach-y=\"2\" param=\"h8\"/>"
            "<knob   attach-x=\"5\" attach-y=\"3\" param=\"f6\"/>"
            "<value  attach-x=\"5\" attach-y=\"4\" param=\"f6\"/>"

            "<label  attach-x=\"6\" attach-y=\"0\" param=\"h10\"/>"
            "<vscale attach-x=\"6\" attach-y=\"1\" param=\"h10\"/>"
            "<value  attach-x=\"6\" attach-y=\"2\" param=\"h10\"/>"
            "<knob   attach-x=\"6\" attach-y=\"3\" param=\"f7\"/>"
            "<value  attach-x=\"6\" attach-y=\"4\" param=\"f7\"/>"

            "<label  attach-x=\"7\" attach-y=\"0\" param=\"h12\"/>"
            "<vscale attach-x=\"7\" attach-y=\"1\" param=\"h12\"/>"
            "<value  attach-x=\"7\" attach-y=\"2\" param=\"h12\"/>"
            "<knob   attach-x=\"7\" attach-y=\"3\" param=\"f8\"/>"
            "<value  attach-x=\"7\" attach-y=\"4\" param=\"f8\"/>"

            "<label  attach-x=\"8\" attach-y=\"0\" param=\"h16\"/>"
            "<vscale attach-x=\"8\" attach-y=\"1\" param=\"h16\"/>"
            "<value  attach-x=\"8\" attach-y=\"2\" param=\"h16\"/>"
            "<knob   attach-x=\"8\" attach-y=\"3\" param=\"f9\"/>"
            "<value  attach-x=\"8\" attach-y=\"4\" param=\"f9\"/>"
        "</table>"
    "</vbox>"
    ;
}

const char *organ_audio_module::port_names[] = {"Out L", "Out R"};

const char *organ_percussion_harmonic_names[] = { "2nd", "3rd" };

parameter_properties organ_audio_module::param_props[] = {
    { 8,       0,  8, 80, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_FADER, NULL, "h1", "16'" },
    { 8,       0,  8, 80, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_FADER, NULL, "h3", "5 1/3'" },
    { 8,       0,  8, 80, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_FADER, NULL, "h2", "8'" },
    { 0,       0,  8, 80, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_FADER, NULL, "h4", "4'" },
    { 0,       0,  8, 80, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_FADER, NULL, "h6", "2 2/3'" },
    { 0,       0,  8, 80, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_FADER, NULL, "h8", "2'" },
    { 0,       0,  8, 80, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_FADER, NULL, "h10", "1 3/5'" },
    { 0,       0,  8, 80, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_FADER, NULL, "h12", "1 1/3'" },
    { 8,       0,  8, 80, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_FADER, NULL, "h16", "1'" },

    { 1,       1, 32, 32, PF_INT | PF_SCALE_LINEAR | PF_CTL_FADER, NULL, "f1", "Freq 1" },
    { 3,       1, 32, 32, PF_INT | PF_SCALE_LINEAR | PF_CTL_FADER, NULL, "f2", "Freq 2" },
    { 2,       1, 32, 32, PF_INT | PF_SCALE_LINEAR | PF_CTL_FADER, NULL, "f3", "Freq 3" },
    { 4,       1, 32, 32, PF_INT | PF_SCALE_LINEAR | PF_CTL_FADER, NULL, "f4", "Freq 4" },
    { 6,       1, 32, 32, PF_INT | PF_SCALE_LINEAR | PF_CTL_FADER, NULL, "f5", "Freq 5" },
    { 8,       1, 32, 32, PF_INT | PF_SCALE_LINEAR | PF_CTL_FADER, NULL, "f6", "Freq 6" },
    { 10,      1, 32, 32, PF_INT | PF_SCALE_LINEAR | PF_CTL_FADER, NULL, "f7", "Freq 7" },
    { 12,      1, 32, 32, PF_INT | PF_SCALE_LINEAR | PF_CTL_FADER, NULL, "f8", "Freq 8" },
    { 16,      1, 32, 32, PF_INT | PF_SCALE_LINEAR | PF_CTL_FADER, NULL, "f9", "Freq 9" },

    { 2,       0,  8, 0, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_FADER, NULL, "w1", "Wave 1" },
    { 2,       0,  8, 0, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_FADER, NULL, "w2", "Wave 2" },
    { 1,       0,  8, 0, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_FADER, NULL, "w3", "Wave 3" },
    { 1,       0,  8, 0, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_FADER, NULL, "w4", "Wave 4" },
    { 0,       0,  8, 0, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_FADER, NULL, "w5", "Wave 5" },
    { 0,       0,  8, 0, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_FADER, NULL, "w6", "Wave 6" },
    { 0,       0,  8, 0, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_FADER, NULL, "w7", "Wave 7" },
    { 0,       0,  8, 0, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_FADER, NULL, "w8", "Wave 8" },
    { 0,       0,  8, 0, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_FADER, NULL, "w9", "Wave 9" },

    { 1,         0,  1, 2, PF_BOOL | PF_CTL_TOGGLE, NULL, "foldover", "Foldover" },
    { 200,         10,  3000, 100, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "perc_decay", "Perc. decay" },
    { 0.25,      0,  1, 100, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB, NULL, "perc_level", "Perc. level" },
    { 3,         2,  3, 1, PF_ENUM | PF_CTL_COMBO, organ_percussion_harmonic_names, "perc_harm", "Perc. harmonic" },

    { 0.1,         0,  1, 100, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB, NULL, "master", "Volume" },
};

////////////////////////////////////////////////////////////////////////////

const char *rotary_speaker_audio_module::port_names[] = {"In L", "In R", "Out L", "Out R"};

const char *rotary_speaker_speed_names[] = { "Off", "Chorale", "Tremolo", "HoldPedal", "ModWheel" };

parameter_properties rotary_speaker_audio_module::param_props[] = {
    { 2,         0,  4, 1.01, PF_ENUM | PF_CTL_COMBO, rotary_speaker_speed_names, "vib_speed", "Speed Mode" },
};

waveform_family<ORGAN_WAVE_BITS> organ_voice_base::waves[organ_voice_base::wave_count];

organ_voice_base::organ_voice_base(organ_parameters *_parameters)
: parameters(_parameters)
{
    note = -1;
    static bool inited = false;
    if (!inited)
    {
        float tmp[ORGAN_WAVE_SIZE];
        bandlimiter<ORGAN_WAVE_BITS> bl;
        inited = true;
        for (int i = 0; i < ORGAN_WAVE_SIZE; i++)
            tmp[i] = sin(i * 2 * M_PI / ORGAN_WAVE_SIZE);
        waves[wave_sine].make(bl, tmp);
        for (int i = 0; i < ORGAN_WAVE_SIZE; i++)
            tmp[i] = (i < (ORGAN_WAVE_SIZE / 16)) ? 1 : 0;
        normalize_waveform(tmp, ORGAN_WAVE_SIZE);
        waves[wave_pulse].make(bl, tmp);
        for (int i = 0; i < ORGAN_WAVE_SIZE; i++)
            tmp[i] = i < (ORGAN_WAVE_SIZE / 4) ? sin(i * 4 * 2 * M_PI / ORGAN_WAVE_SIZE) : 0;
        waves[wave_stretchsine].make(bl, tmp);
    }
}
