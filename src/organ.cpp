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

#if ENABLE_EXPERIMENTAL

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
                "<label param=\"perc_mode\"/>"
                "<combo param=\"perc_mode\"/>"
            "</vbox>"
            "<vbox>"
                "<label param=\"perc_hrm\"/>"
                "<combo param=\"perc_hrm\"/>"
            "</vbox>"        
            "<vbox>"
                "<label param=\"master\"/>"
                "<knob param=\"master\"/>"
                "<value param=\"master\"/>"
            "</vbox>"
        "</hbox>"
        "<table rows=\"3\" cols=\"9\">"
            "<label  attach-x=\"0\" attach-y=\"0\" param=\"h1\"/>"
            "<vscale attach-x=\"0\" attach-y=\"1\" param=\"h1\"/>"
            "<value  attach-x=\"0\" attach-y=\"2\" param=\"h1\"/>"
    
            "<label  attach-x=\"1\" attach-y=\"0\" param=\"h3\"/>"
            "<vscale attach-x=\"1\" attach-y=\"1\" param=\"h3\"/>"
            "<value  attach-x=\"1\" attach-y=\"2\" param=\"h3\"/>"
    
            "<label  attach-x=\"2\" attach-y=\"0\" param=\"h2\"/>"
            "<vscale attach-x=\"2\" attach-y=\"1\" param=\"h2\"/>"
            "<value  attach-x=\"2\" attach-y=\"2\" param=\"h2\"/>"
    
            "<label  attach-x=\"3\" attach-y=\"0\" param=\"h4\"/>"
            "<vscale attach-x=\"3\" attach-y=\"1\" param=\"h4\"/>"
            "<value  attach-x=\"3\" attach-y=\"2\" param=\"h4\"/>"

            "<label  attach-x=\"4\" attach-y=\"0\" param=\"h6\"/>"
            "<vscale attach-x=\"4\" attach-y=\"1\" param=\"h6\"/>"
            "<value  attach-x=\"4\" attach-y=\"2\" param=\"h6\"/>"
    
            "<label  attach-x=\"5\" attach-y=\"0\" param=\"h8\"/>"
            "<vscale attach-x=\"5\" attach-y=\"1\" param=\"h8\"/>"
            "<value  attach-x=\"5\" attach-y=\"2\" param=\"h8\"/>"

            "<label  attach-x=\"6\" attach-y=\"0\" param=\"h10\"/>"
            "<vscale attach-x=\"6\" attach-y=\"1\" param=\"h10\"/>"
            "<value  attach-x=\"6\" attach-y=\"2\" param=\"h10\"/>"

            "<label  attach-x=\"7\" attach-y=\"0\" param=\"h12\"/>"
            "<vscale attach-x=\"7\" attach-y=\"1\" param=\"h12\"/>"
            "<value  attach-x=\"7\" attach-y=\"2\" param=\"h12\"/>"

            "<label  attach-x=\"8\" attach-y=\"0\" param=\"h16\"/>"
            "<vscale attach-x=\"8\" attach-y=\"1\" param=\"h16\"/>"
            "<value  attach-x=\"8\" attach-y=\"2\" param=\"h16\"/>"
        "</table>"
    "</vbox>"
    ;
}

const char *organ_audio_module::port_names[] = {"Out L", "Out R"};

const char *organ_percussion_mode_names[] = { "Off", "Short", "Long" };
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

    { 1,         0,  1, 2, PF_BOOL | PF_CTL_TOGGLE, NULL, "foldover", "Foldover" },
    { 1,         0,  2, 3, PF_ENUM | PF_CTL_COMBO, organ_percussion_mode_names, "perc_mode", "Perc. mode" },
    { 3,         2,  3, 1, PF_ENUM | PF_CTL_COMBO, organ_percussion_harmonic_names, "perc_hrm", "Perc. harmonic" },

    { 0.1,         0,  1, 100, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB, NULL, "master", "Master" },
};

////////////////////////////////////////////////////////////////////////////

const char *rotary_speaker_audio_module::port_names[] = {"In L", "In R", "Out L", "Out R"};

const char *rotary_speaker_speed_names[] = { "Off", "Chorale", "Tremolo", "HoldPedal", "ModWheel" };

parameter_properties rotary_speaker_audio_module::param_props[] = {
    { 2,         0,  4, 1.01, PF_ENUM | PF_CTL_COMBO, rotary_speaker_speed_names, "vib_speed", "Speed Mode" },
};

#endif
