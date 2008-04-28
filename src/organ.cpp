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

#define DRAWBAR_UI(no) \
            "<label  attach-x=\"" no "\" attach-y=\"0\" param=\"l" no "\"/>" \
            "<vscale attach-x=\"" no "\" attach-y=\"1\" param=\"l" no "\"/>" \
            "<value  attach-x=\"" no "\" attach-y=\"2\" param=\"l" no "\"/>" \
            "<knob   attach-x=\"" no "\" attach-y=\"3\" param=\"f" no "\"/>" \
            "<value  attach-x=\"" no "\" attach-y=\"4\" param=\"f" no "\"/>" \
            "<combo  attach-x=\"" no "\" attach-y=\"5\" param=\"w" no "\"/>" \
            "<knob   attach-x=\"" no "\" attach-y=\"6\" param=\"detune" no "\"/>" \
            "<value  attach-x=\"" no "\" attach-y=\"7\" param=\"detune" no "\"/>" \
            "<knob   attach-x=\"" no "\" attach-y=\"8\" param=\"phase" no "\"/>" \
            "<value  attach-x=\"" no "\" attach-y=\"9\" param=\"phase" no "\"/>" \
            "<knob   attach-x=\"" no "\" attach-y=\"10\" param=\"pan" no "\"/>" \
            "<value  attach-x=\"" no "\" attach-y=\"11\" param=\"pan" no "\"/>" \
            "<combo  attach-x=\"" no "\" attach-y=\"12\" param=\"routing" no "\"/>" 

const char *organ_audio_module::get_gui_xml()
{
    return 
    "<vbox border=\"10\">"
        "<hbox>"
            "<vbox>"
                "<label param=\"foldnote\"/>"
                "<align><knob param=\"foldnote\"/></align>"
                "<value param=\"foldnote\"/>"
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
        "<notebook>"
            "<vbox page=\"Tone generator\">"
                "<table rows=\"12\" cols=\"9\">"
                    "<label  attach-x=\"0\" attach-y=\"1\" text=\"Level\"/>"
                    "<label  attach-x=\"0\" attach-y=\"3\" text=\"Harmonic\"/>"
                    "<label  attach-x=\"0\" attach-y=\"5\" text=\"Wave\"/>"
                    "<label  attach-x=\"0\" attach-y=\"6\" text=\"Detune\"/>"
                    "<label  attach-x=\"0\" attach-y=\"8\" text=\"Phase\"/>"
                    "<label  attach-x=\"0\" attach-y=\"10\" text=\"Pan\"/>"
                    "<label  attach-x=\"0\" attach-y=\"12\" text=\"Send to\"/>"

                    DRAWBAR_UI("1")
                    DRAWBAR_UI("2")
                    DRAWBAR_UI("3")
                    DRAWBAR_UI("4")
                    DRAWBAR_UI("5")
                    DRAWBAR_UI("6")
                    DRAWBAR_UI("7")
                    DRAWBAR_UI("8")
                    DRAWBAR_UI("9")
                "</table>"
            "</vbox>"
            "<hbox page=\"Sound processor\">"
                "<vbox>"
                    "<frame label=\"Filter 1\">"
                        "<vbox>"
                            "<hbox>"
                                "<vbox>"
                                    "<label param=\"f1_cutoff\" />"
                                    "<align><knob param=\"f1_cutoff\" expand=\"0\" fill=\"0\"/></align><value param=\"f1_cutoff\"/>"
                                "</vbox>"
                                "<vbox>"
                                    "<label param=\"f1_res\" />"
                                    "<align><knob param=\"f1_res\" expand=\"0\" fill=\"0\"/></align><value param=\"f1_res\"/>"
                                "</vbox>"
                                "<vbox>"
                                    "<label param=\"f1_keyf\" />"
                                    "<align><knob param=\"f1_keyf\" expand=\"0\" fill=\"0\"/></align><value param=\"f1_keyf\"/>"
                                "</vbox>"
                            "</hbox>"
                            "<hbox>"
                                "<vbox>"
                                    "<label param=\"f1_env1\" />"
                                    "<align><knob param=\"f1_env1\" expand=\"0\" fill=\"0\"/></align><value param=\"f1_env1\"/>"
                                "</vbox>"
                                "<vbox>"
                                    "<label param=\"f1_env2\" />"
                                    "<align><knob param=\"f1_env2\" expand=\"0\" fill=\"0\"/></align><value param=\"f1_env2\"/>"
                                "</vbox>"
                                "<vbox>"
                                    "<label param=\"f1_env3\" />"
                                    "<align><knob param=\"f1_env3\" expand=\"0\" fill=\"0\"/></align><value param=\"f1_env3\"/>"
                                "</vbox>"
                            "</hbox>"
                        "</vbox>"
                    "</frame>"
                    "<frame label=\"Filter 2\">"
                        "<vbox>"
                            "<hbox>"
                                "<vbox>"
                                    "<label param=\"f2_cutoff\" />"
                                    "<align><knob param=\"f2_cutoff\" expand=\"0\" fill=\"0\"/></align><value param=\"f2_cutoff\"/>"
                                "</vbox>"
                                "<vbox>"
                                    "<label param=\"f2_res\" />"
                                    "<align><knob param=\"f2_res\" expand=\"0\" fill=\"0\"/></align><value param=\"f2_res\"/>"
                                "</vbox>"
                                "<vbox>"
                                    "<label param=\"f2_keyf\" />"
                                    "<align><knob param=\"f2_keyf\" expand=\"0\" fill=\"0\"/></align><value param=\"f2_keyf\"/>"
                                "</vbox>"
                            "</hbox>"
                            "<hbox>"
                                "<vbox>"
                                    "<label param=\"f2_env1\" />"
                                    "<align><knob param=\"f2_env1\" expand=\"0\" fill=\"0\"/></align><value param=\"f2_env1\"/>"
                                "</vbox>"
                                "<vbox>"
                                    "<label param=\"f2_env2\" />"
                                    "<align><knob param=\"f2_env2\" expand=\"0\" fill=\"0\"/></align><value param=\"f2_env2\"/>"
                                "</vbox>"
                                "<vbox>"
                                    "<label param=\"f2_env3\" />"
                                    "<align><knob param=\"f2_env3\" expand=\"0\" fill=\"0\"/></align><value param=\"f2_env3\"/>"
                                "</vbox>"
                            "</hbox>"
                        "</vbox>"
                    "</frame>"
                    "<frame label=\"Amplifier\">"
                        "<vbox>"
                            "<hbox><label param=\"eg1_amp_ctl\"/><combo param=\"eg1_amp_ctl\"/></hbox>"
                            "<hbox><label param=\"eg2_amp_ctl\"/><combo param=\"eg2_amp_ctl\"/></hbox>"
                            "<hbox><label param=\"eg3_amp_ctl\"/><combo param=\"eg3_amp_ctl\"/></hbox>"
                        "</vbox>"
                    "</frame>"
                "</vbox>"
                "<vbox>"
                    "<frame label=\"EG 1\">"
                        "<vbox>"
                            "<hbox>"
                                "<vbox>"
                                    "<label param=\"adsr_a\" />"
                                    "<align><knob param=\"adsr_a\" expand=\"0\" fill=\"0\"/></align><value param=\"adsr_a\"/>"
                                "</vbox>"
                                "<vbox>"
                                    "<label param=\"adsr_d\" />"
                                    "<align><knob param=\"adsr_d\" expand=\"0\" fill=\"0\"/></align><value param=\"adsr_d\"/>"
                                "</vbox>"
                                "<vbox>"
                                    "<label param=\"adsr_s\" />"
                                    "<align><knob param=\"adsr_s\" expand=\"0\" fill=\"0\"/></align><value param=\"adsr_s\"/>"
                                "</vbox>"
                                "<vbox>"
                                    "<label param=\"adsr_r\" />"
                                    "<align><knob param=\"adsr_r\" expand=\"0\" fill=\"0\"/></align><value param=\"adsr_r\"/>"
                                "</vbox>"
                                "<vbox>"
                                    "<label param=\"adsr_v\" />"
                                    "<align><knob param=\"adsr_v\" expand=\"0\" fill=\"0\"/></align><value param=\"adsr_v\"/>"
                                "</vbox>"
                            "</hbox>"
                        "</vbox>"
                    "</frame>"
                    "<frame label=\"EG 2\">"
                        "<vbox>"
                            "<hbox>"
                                "<vbox>"
                                    "<label param=\"adsr2_a\" />"
                                    "<align><knob param=\"adsr2_a\" expand=\"0\" fill=\"0\"/></align><value param=\"adsr2_a\"/>"
                                "</vbox>"
                                "<vbox>"
                                    "<label param=\"adsr2_d\" />"
                                    "<align><knob param=\"adsr2_d\" expand=\"0\" fill=\"0\"/></align><value param=\"adsr2_d\"/>"
                                "</vbox>"
                                "<vbox>"
                                    "<label param=\"adsr2_s\" />"
                                    "<align><knob param=\"adsr2_s\" expand=\"0\" fill=\"0\"/></align><value param=\"adsr2_s\"/>"
                                "</vbox>"
                                "<vbox>"
                                    "<label param=\"adsr2_r\" />"
                                    "<align><knob param=\"adsr2_r\" expand=\"0\" fill=\"0\"/></align><value param=\"adsr2_r\"/>"
                                "</vbox>"
                                "<vbox>"
                                    "<label param=\"adsr2_v\" />"
                                    "<align><knob param=\"adsr2_v\" expand=\"0\" fill=\"0\"/></align><value param=\"adsr2_v\"/>"
                                "</vbox>"
                            "</hbox>"
                        "</vbox>"
                    "</frame>"
                    "<frame label=\"EG 3\">"
                        "<vbox>"
                            "<hbox>"
                                "<vbox>"
                                    "<label param=\"adsr3_a\" />"
                                    "<align><knob param=\"adsr3_a\" expand=\"0\" fill=\"0\"/></align><value param=\"adsr3_a\"/>"
                                "</vbox>"
                                "<vbox>"
                                    "<label param=\"adsr3_d\" />"
                                    "<align><knob param=\"adsr3_d\" expand=\"0\" fill=\"0\"/></align><value param=\"adsr3_d\"/>"
                                "</vbox>"
                                "<vbox>"
                                    "<label param=\"adsr3_s\" />"
                                    "<align><knob param=\"adsr3_s\" expand=\"0\" fill=\"0\"/></align><value param=\"adsr3_s\"/>"
                                "</vbox>"
                                "<vbox>"
                                    "<label param=\"adsr3_r\" />"
                                    "<align><knob param=\"adsr3_r\" expand=\"0\" fill=\"0\"/></align><value param=\"adsr3_r\"/>"
                                "</vbox>"
                                "<vbox>"
                                    "<label param=\"adsr3_v\" />"
                                    "<align><knob param=\"adsr3_v\" expand=\"0\" fill=\"0\"/></align><value param=\"adsr3_v\"/>"
                                "</vbox>"
                            "</hbox>"
                        "</vbox>"
                    "</frame>"
                "</vbox>"
                "<frame label=\"LFO\">"
                    "<vbox>"
                        "<vbox>"
                            "<label param=\"vib_rate\" />"
                            "<align><knob param=\"vib_rate\" expand=\"0\" fill=\"0\"/></align><value param=\"vib_rate\"/>"
                        "</vbox>"
                        "<vbox>"
                            "<label param=\"vib_amt\" />"
                            "<align><knob param=\"vib_amt\" expand=\"0\" fill=\"0\"/></align><value param=\"vib_amt\"/>"
                        "</vbox>"
                        "<vbox>"
                            "<label param=\"vib_wet\" />"
                            "<align><knob param=\"vib_wet\" expand=\"0\" fill=\"0\"/></align><value param=\"vib_wet\"/>"
                        "</vbox>"
                        "<vbox>"
                            "<label param=\"vib_phase\" />"
                            "<align><knob param=\"vib_phase\" expand=\"0\" fill=\"0\"/></align><value param=\"vib_phase\"/>"
                        "</vbox>"
                    "</vbox>"
                "</frame>"
            "</hbox>"
        "</notebook>"
    "</vbox>"
    ;
}

const char *organ_audio_module::port_names[] = {"Out L", "Out R"};

const char *organ_percussion_harmonic_names[] = { "2nd", "3rd" };

const char *organ_wave_names[] = { 
    "Sin", 
    "S0", "S00", "S000", 
    "SSaw", "SSqr", "SPls", 
    "Saw", "Sqr", "Pls", 
    "S(", "Sq(", "S+", "Clvg", 
    "Bell", "Bell2", 
    "W1", "W2", "W3", "W4", "W5", "W6", "W7", "W8", "W9",
    "DSaw", "DSqr", "DPls",
};

const char *organ_routing_names[] = { "Out", "Flt 1", "Flt 2"  };

const char *organ_ampctl_names[] = { "None", "Direct", "Flt 1", "Flt 2", "All"  };

parameter_properties organ_audio_module::param_props[] = {
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

    { 0,       0,  organ_voice_base::wave_count - 1, 0, PF_ENUM | PF_SCALE_LINEAR | PF_CTL_COMBO, organ_wave_names, "w1", "Wave 1" },
    { 0,       0,  organ_voice_base::wave_count - 1, 0, PF_ENUM | PF_SCALE_LINEAR | PF_CTL_COMBO, organ_wave_names, "w2", "Wave 2" },
    { 0,       0,  organ_voice_base::wave_count - 1, 0, PF_ENUM | PF_SCALE_LINEAR | PF_CTL_COMBO, organ_wave_names, "w3", "Wave 3" },
    { 0,       0,  organ_voice_base::wave_count - 1, 0, PF_ENUM | PF_SCALE_LINEAR | PF_CTL_COMBO, organ_wave_names, "w4", "Wave 4" },
    { 0,       0,  organ_voice_base::wave_count - 1, 0, PF_ENUM | PF_SCALE_LINEAR | PF_CTL_COMBO, organ_wave_names, "w5", "Wave 5" },
    { 0,       0,  organ_voice_base::wave_count - 1, 0, PF_ENUM | PF_SCALE_LINEAR | PF_CTL_COMBO, organ_wave_names, "w6", "Wave 6" },
    { 0,       0,  organ_voice_base::wave_count - 1, 0, PF_ENUM | PF_SCALE_LINEAR | PF_CTL_COMBO, organ_wave_names, "w7", "Wave 7" },
    { 0,       0,  organ_voice_base::wave_count - 1, 0, PF_ENUM | PF_SCALE_LINEAR | PF_CTL_COMBO, organ_wave_names, "w8", "Wave 8" },
    { 0,       0,  organ_voice_base::wave_count - 1, 0, PF_ENUM | PF_SCALE_LINEAR | PF_CTL_COMBO, organ_wave_names, "w9", "Wave 9" },

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

    { 96,      0,  127, 128, PF_INT | PF_CTL_KNOB | PF_UNIT_NOTE, NULL, "foldnote", "Foldover" },
    { 200,         10,  3000, 100, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "perc_decay", "Perc. decay" },
    { 0.25,      0,  1, 100, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB, NULL, "perc_level", "Perc. level" },
    { 3,         2,  3, 1, PF_ENUM | PF_CTL_COMBO, organ_percussion_harmonic_names, "perc_harm", "Perc. harmonic" },

    { 0.1,         0,  1, 100, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB, NULL, "master", "Volume" },
    
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
    { 0,  0, organ_voice_base::ampctl_count - 1,
                              0, PF_INT | PF_CTL_COMBO, organ_ampctl_names, "eg1_amp_ctl", "EG1 To Amp"},

    { 1,          1,20000,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "adsr2_a", "EG2 Attack" },
    { 350,       10,20000,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "adsr2_d", "EG2 Decay" },
    { 0.5,        0,    1,    0, PF_FLOAT | PF_SCALE_PERC, NULL, "adsr2_s", "EG2 Sustain" },
    { 50,       10,20000,     0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "adsr2_r", "EG2 Release" },
    { 0,          0,    1,    0, PF_FLOAT | PF_SCALE_PERC, NULL, "adsr2_v", "EG2 VelMod" },
    { 0,  0, organ_voice_base::ampctl_count - 1,
                              0, PF_INT | PF_CTL_COMBO, organ_ampctl_names, "eg2_amp_ctl", "EG2 To Amp"},

    { 1,          1,20000,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "adsr3_a", "EG3 Attack" },
    { 350,       10,20000,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "adsr3_d", "EG3 Decay" },
    { 0.5,        0,    1,    0, PF_FLOAT | PF_SCALE_PERC, NULL, "adsr3_s", "EG3 Sustain" },
    { 50,       10,20000,     0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "adsr3_r", "EG3 Release" },
    { 0,          0,    1,    0, PF_FLOAT | PF_SCALE_PERC, NULL, "adsr3_v", "EG3 VelMod" },
    { 0,  0, organ_voice_base::ampctl_count - 1,
                              0, PF_INT | PF_CTL_COMBO, organ_ampctl_names, "eg3_amp_ctl", "EG3 To Amp"},

    { 6.6,     0.01,   80,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL, "vib_rate", "Vib Rate" },
    { 0.5,        0,    1,    0, PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB , NULL, "vib_amt", "Vib Mod Amt" },
    { 0.5,        0,    1,    0, PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB , NULL, "vib_wet", "Vib Wet" },
    { 180,        0,  360,    0, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_DEG, NULL, "vib_phase", "Vib Stereo" },
//    { 0,  0, organ_voice_base::ampctl_count - 1,
//                              0, PF_INT | PF_CTL_COMBO, organ_ampctl_names, "vel_amp_ctl", "Vel To Amp"},
};

////////////////////////////////////////////////////////////////////////////

waveform_family<ORGAN_WAVE_BITS> organ_voice_base::waves[organ_voice_base::wave_count];


static void smoothen(bandlimiter<ORGAN_WAVE_BITS> &bl, float tmp[ORGAN_WAVE_SIZE])
{
    bl.compute_spectrum(tmp);
    for (int i = 1; i <= ORGAN_WAVE_SIZE / 2; i++) {
        bl.spectrum[i] *= 1.0 / sqrt(i);
        bl.spectrum[ORGAN_WAVE_SIZE - i] *= 1.0 / sqrt(i);
    }
    bl.compute_waveform(tmp);
    normalize_waveform(tmp, ORGAN_WAVE_SIZE);
}

static void phaseshift(bandlimiter<ORGAN_WAVE_BITS> &bl, float tmp[ORGAN_WAVE_SIZE])
{
    bl.compute_spectrum(tmp);
    for (int i = 1; i <= ORGAN_WAVE_SIZE / 2; i++) {
        float phase = sin(i*i);
        complex<float> shift = complex<float>(cos(phase), sin(phase));
        bl.spectrum[i] *= shift;
        bl.spectrum[ORGAN_WAVE_SIZE - i] *= conj(shift);
    }
    bl.compute_waveform(tmp);
    normalize_waveform(tmp, ORGAN_WAVE_SIZE);
}


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
            tmp[i] = i < (ORGAN_WAVE_SIZE / 2) ? sin(i * 2 * 2 * M_PI / ORGAN_WAVE_SIZE) : 0;
        waves[wave_sinepl1].make(bl, tmp);
        for (int i = 0; i < ORGAN_WAVE_SIZE; i++)
            tmp[i] = i < (ORGAN_WAVE_SIZE / 3) ? sin(i * 3 * 2 * M_PI / ORGAN_WAVE_SIZE) : 0;
        waves[wave_sinepl2].make(bl, tmp);
        for (int i = 0; i < ORGAN_WAVE_SIZE; i++)
            tmp[i] = i < (ORGAN_WAVE_SIZE / 4) ? sin(i * 4 * 2 * M_PI / ORGAN_WAVE_SIZE) : 0;
        waves[wave_sinepl3].make(bl, tmp);
        for (int i = 0; i < ORGAN_WAVE_SIZE; i++)
            tmp[i] = (i < (ORGAN_WAVE_SIZE / 2)) ? 1 : -1;
        normalize_waveform(tmp, ORGAN_WAVE_SIZE);
        waves[wave_sqr].make(bl, tmp);
        for (int i = 0; i < ORGAN_WAVE_SIZE; i++)
            tmp[i] = -1 + (i * 2.0 / ORGAN_WAVE_SIZE);
        normalize_waveform(tmp, ORGAN_WAVE_SIZE);
        waves[wave_saw].make(bl, tmp);
        
        for (int i = 0; i < ORGAN_WAVE_SIZE; i++)
            tmp[i] = (i < (ORGAN_WAVE_SIZE / 2)) ? 1 : -1;
        smoothen(bl, tmp);
        waves[wave_ssqr].make(bl, tmp);
        
        for (int i = 0; i < ORGAN_WAVE_SIZE; i++)
            tmp[i] = -1 + (i * 2.0 / ORGAN_WAVE_SIZE);
        smoothen(bl, tmp);
        waves[wave_ssaw].make(bl, tmp);

        for (int i = 0; i < ORGAN_WAVE_SIZE; i++)
            tmp[i] = (i < (ORGAN_WAVE_SIZE / 16)) ? 1 : 0;
        smoothen(bl, tmp);
        waves[wave_spls].make(bl, tmp);
        for (int i = 0; i < ORGAN_WAVE_SIZE; i++)
            tmp[i] = i < (ORGAN_WAVE_SIZE / 1.5) ? sin(i * 1.5 * 2 * M_PI / ORGAN_WAVE_SIZE) : 0;
        waves[wave_sinepl05].make(bl, tmp);
        for (int i = 0; i < ORGAN_WAVE_SIZE; i++)
            tmp[i] = i < (ORGAN_WAVE_SIZE / 1.5) ? (i < ORGAN_WAVE_SIZE / 3 ? -1 : +1) : 0;
        waves[wave_sqr05].make(bl, tmp);
        for (int i = 0; i < ORGAN_WAVE_SIZE; i++)
            tmp[i] = sin(i * M_PI / ORGAN_WAVE_SIZE);
        waves[wave_halfsin].make(bl, tmp);
        for (int i = 0; i < ORGAN_WAVE_SIZE; i++)
            tmp[i] = sin(i * 3 * M_PI / ORGAN_WAVE_SIZE);
        waves[wave_clvg].make(bl, tmp);
        for (int i = 0; i < ORGAN_WAVE_SIZE; i++)
        {
            float ph = i * 2 * M_PI / ORGAN_WAVE_SIZE;
            float fm = 0.3 * sin(6*ph) + 0.3 * sin(11*ph) + 0.3 * cos(17*ph) - 0.3 * cos(19*ph);
            tmp[i] = sin(5*ph + fm) + cos(7*ph - fm);
        }
        normalize_waveform(tmp, ORGAN_WAVE_SIZE);
        waves[wave_bell].make(bl, tmp);
        for (int i = 0; i < ORGAN_WAVE_SIZE; i++)
        {
            float ph = i * 2 * M_PI / ORGAN_WAVE_SIZE;
            float fm = 0.3 * sin(3*ph) + 0.3 * sin(11*ph) + 0.3 * cos(17*ph) - 0.3 * cos(19*ph)  + 0.3 * cos(25*ph)  - 0.3 * cos(31*ph) + 0.3 * cos(37*ph);
            tmp[i] = sin(3*ph + fm) + cos(7*ph - fm);
        }
        normalize_waveform(tmp, ORGAN_WAVE_SIZE);
        waves[wave_bell2].make(bl, tmp);
        for (int i = 0; i < ORGAN_WAVE_SIZE; i++)
        {
            float ph = i * 2 * M_PI / ORGAN_WAVE_SIZE;
            float fm = 0.5 * sin(3*ph) + 0.3 * sin(5*ph) + 0.3 * cos(6*ph) - 0.3 * cos(9*ph);
            tmp[i] = sin(4*ph + fm) + cos(ph - fm);
        }
        normalize_waveform(tmp, ORGAN_WAVE_SIZE);
        waves[wave_w1].make(bl, tmp);
        for (int i = 0; i < ORGAN_WAVE_SIZE; i++)
        {
            float ph = i * 2 * M_PI / ORGAN_WAVE_SIZE;
            tmp[i] = sin(ph) * sin(2 * ph) * sin(4 * ph) * sin(8 * ph);
        }
        normalize_waveform(tmp, ORGAN_WAVE_SIZE);
        waves[wave_w2].make(bl, tmp);
        for (int i = 0; i < ORGAN_WAVE_SIZE; i++)
        {
            float ph = i * 2 * M_PI / ORGAN_WAVE_SIZE;
            tmp[i] = sin(ph) * sin(3 * ph) * sin(5 * ph) * sin(7 * ph) * sin(9 * ph);
        }
        normalize_waveform(tmp, ORGAN_WAVE_SIZE);
        waves[wave_w3].make(bl, tmp);
        for (int i = 0; i < ORGAN_WAVE_SIZE; i++)
        {
            float ph = i * 2 * M_PI / ORGAN_WAVE_SIZE;
            tmp[i] = sin(ph + 2 * sin(ph + 2 * sin(ph)));
        }
        normalize_waveform(tmp, ORGAN_WAVE_SIZE);
        waves[wave_w4].make(bl, tmp);
        for (int i = 0; i < ORGAN_WAVE_SIZE; i++)
        {
            float ph = i * 2 * M_PI / ORGAN_WAVE_SIZE;
            tmp[i] = ph * sin(ph);
        }
        normalize_waveform(tmp, ORGAN_WAVE_SIZE);
        waves[wave_w5].make(bl, tmp);
        for (int i = 0; i < ORGAN_WAVE_SIZE; i++)
        {
            float ph = i * 2 * M_PI / ORGAN_WAVE_SIZE;
            tmp[i] = ph * sin(ph) + (2 * M_PI - ph) * sin(2 * ph);
        }
        normalize_waveform(tmp, ORGAN_WAVE_SIZE);
        waves[wave_w6].make(bl, tmp);
        for (int i = 0; i < ORGAN_WAVE_SIZE; i++)
        {
            float ph = i * 1.0 / ORGAN_WAVE_SIZE;
            tmp[i] = exp(-ph * ph);
        }
        normalize_waveform(tmp, ORGAN_WAVE_SIZE);
        waves[wave_w7].make(bl, tmp);
        for (int i = 0; i < ORGAN_WAVE_SIZE; i++)
        {
            float ph = i * 1.0 / ORGAN_WAVE_SIZE;
            tmp[i] = exp(-ph * sin(2 * M_PI * ph));
        }
        normalize_waveform(tmp, ORGAN_WAVE_SIZE);
        waves[wave_w8].make(bl, tmp);
        for (int i = 0; i < ORGAN_WAVE_SIZE; i++)
        {
            float ph = i * 1.0 / ORGAN_WAVE_SIZE;
            tmp[i] = sin(2 * M_PI * ph * ph);
        }
        normalize_waveform(tmp, ORGAN_WAVE_SIZE);
        waves[wave_w9].make(bl, tmp);

        for (int i = 0; i < ORGAN_WAVE_SIZE; i++)
            tmp[i] = -1 + (i * 2.0 / ORGAN_WAVE_SIZE);
        normalize_waveform(tmp, ORGAN_WAVE_SIZE);
        phaseshift(bl, tmp);
        waves[wave_dsaw].make(bl, tmp);

        for (int i = 0; i < ORGAN_WAVE_SIZE; i++)
            tmp[i] = (i < (ORGAN_WAVE_SIZE / 2)) ? 1 : -1;
        normalize_waveform(tmp, ORGAN_WAVE_SIZE);
        phaseshift(bl, tmp);
        waves[wave_dsqr].make(bl, tmp);

        for (int i = 0; i < ORGAN_WAVE_SIZE; i++)
            tmp[i] = (i < (ORGAN_WAVE_SIZE / 16)) ? 1 : 0;
        normalize_waveform(tmp, ORGAN_WAVE_SIZE);
        phaseshift(bl, tmp);
        waves[wave_dpls].make(bl, tmp);
    }
}

void organ_voice::render_block() {
    if (note == -1)
        return;

    dsp::zero(&output_buffer[0][0], Channels * BlockSize);
    dsp::zero(&aux_buffers[1][0][0], 2 * Channels * BlockSize);
    if (!amp.get_active())
        return;
    
    dsp::fixed_point<int, 20> tphase, tdphase;
    unsigned int foldvalue = parameters->foldvalue;
    int muln = 0;
    for (int h = 0; h < 9; h++)
    {
        float amp = parameters->drawbars[h];
        if (amp < small_value<float>())
            continue;
        float *data;
        dsp::fixed_point<int, 24> hm = dsp::fixed_point<int, 24>(parameters->multiplier[h] * (1.0 / 2.0));
        int waveid = (int)parameters->waveforms[h];
        if (waveid < 0 || waveid >= wave_count)
            waveid = 0;

        uint32_t rate = (dphase * hm).get();
        unsigned int foldback = 0;
        while (rate > foldvalue)
        {
            rate >>= 1;
            foldback++;
        }
        hm.set(hm.get() >> foldback);
        data = waves[waveid].get_level(rate);
        if (!data)
            continue;
        tphase.set((uint32_t)((phase * hm).get()) + parameters->phaseshift[h]);
        tdphase.set((uint32_t)rate);
        float ampl = amp * 0.5f * (1 - parameters->pan[h]);
        float ampr = amp * 0.5f * (1 + parameters->pan[h]);
        float (*out)[Channels] = aux_buffers[dsp::fastf2i_drm(parameters->routing[h])];
        for (int i=0; i < (int)BlockSize; i++) {
            float wv = wave(data, tphase);
            out[i][0] += wv * ampl;
            out[i][1] += wv * ampr;
            tphase += tdphase;
        }
        if (h == 3 || h == 6)
            muln++;
    }
    expression.set_inertia(parameters->cutoff);
    phase += dphase * BlockSize;
    float escl[EnvCount], eval[EnvCount];
    for (int i = 0; i < EnvCount; i++)
        escl[i] = (1.f + parameters->envs[i].velscale * (velocity - 1.f));
    
    for (int i = 0; i < EnvCount; i++)
        eval[i] = envs[i].value * escl[i];
    for (int i = 0; i < FilterCount; i++)
    {
        float mod = parameters->filters[i].envmod[0] * eval[0] ;
        mod += parameters->filters[i].keyf * 100 * (note - 60);
        for (int j = 1; j < EnvCount; j++)
        {
            mod += parameters->filters[i].envmod[j] * eval[j];
        }
        if (i) mod += expression.get() * 1200 * 4;
        float fc = parameters->filters[i].cutoff * pow(2.0f, mod * (1.f / 1200.f));
        filterL[i].set_lp_rbj(dsp::clip<float>(fc, 10, 18000), parameters->filters[i].resonance, sample_rate);
        filterR[i].copy_coeffs(filterL[i]);
    }
    float amp_pre[ampctl_count - 1], amp_post[ampctl_count - 1];
    for (int i = 0; i < ampctl_count; i++)
    {
        amp_pre[i] = 1.f;
        amp_post[i] = 1.f;
    }
    bool any_running = false;
    for (int i = 0; i < EnvCount; i++)
    {
        float pre = eval[i];
        envs[i].advance();
        int mode = fastf2i_drm(parameters->envs[i].ampctl);
        if (!envs[i].stopped())
            any_running = true;
        if (mode == ampctl_none)
            continue;
        float post = envs[i].value * escl[i];
        amp_pre[mode - 1] *= pre;
        amp_post[mode - 1] *= post;
    }
    if (!any_running)
        released = true;
    // calculate delta from pre and post
    for (int i = 0; i < ampctl_count; i++)
        amp_post[i] = (amp_post[i] - amp_pre[i]) * (1.0 / BlockSize);
    float a0 = amp_pre[0], a1 = amp_pre[1], a2 = amp_pre[2], a3 = amp_pre[3];
    float d0 = amp_post[0], d1 = amp_post[1], d2 = amp_post[2], d3 = amp_post[3];
    for (int i=0; i < (int) BlockSize; i++) {
        output_buffer[i][0] = a3 * (a0 * output_buffer[i][0] + a1 * filterL[0].process_d1(aux_buffers[1][i][0]) + a2 * filterL[1].process_d1(aux_buffers[2][i][0]));
        output_buffer[i][1] = a3 * (a0 * output_buffer[i][1] + a1 * filterR[0].process_d1(aux_buffers[1][i][1]) + a2 * filterR[1].process_d1(aux_buffers[2][i][1]));
        a0 += d0, a1 += d1, a2 += d2, a3 += d3;
    }

    float lfo1 = lfo_phase < 0.5 ? 2 * lfo_phase : 2 - 2 * lfo_phase;
    float lfo_phase2 = lfo_phase + parameters->lfo_phase * (1.0 / 360.0);
    if (lfo_phase2 >= 1.0)
        lfo_phase2 -= 1.0;
    float lfo2 = lfo_phase2 < 0.5 ? 2 * lfo_phase2 : 2 - 2 * lfo_phase2;
    lfo_phase += parameters->lfo_rate * BlockSize / sample_rate;
    if (lfo_phase >= 1.0)
        lfo_phase -= 1.0;
    vibrato[0].set_ap(3000 + 7000 * parameters->lfo_amt * lfo1 * lfo1, sample_rate);
    vibrato[1].set_ap(3000 + 7000 * parameters->lfo_amt * lfo2 * lfo2, sample_rate);
    
    float vib_wet = parameters->lfo_wet;
    for (int c = 0; c < 2; c++)
    {
        for (int i = 0; i < (int) BlockSize; i++)
        {
            float v = output_buffer[i][c];
            float v0 = v;
            for (int t = 0; t < VibratoSize; t++)
                v = vibrato[c].process_ap(v, vibrato_x1[t][c], vibrato_y1[t][c]);
            
            output_buffer[i][c] += (v - v0) * vib_wet;
        }
        for (int t = 0; t < VibratoSize; t++)
        {
            sanitize(vibrato_x1[t][c]);
            sanitize(vibrato_y1[t][c]);
        }
    }
    if (released)
    {
        for (int i = 0; i < (int) BlockSize; i++) {
            output_buffer[i][0] *= amp.get();
            output_buffer[i][1] *= amp.get();
            amp.age_lin((1.0/44100.0)/0.03,0.0);
        }
    }
}

void drawbar_organ::update_params()
{
    parameters->perc_decay_const = dsp::decay::calc_exp_constant(1.0 / 1024.0, 0.001 * parameters->percussion_time * sample_rate);
    for (int i = 0; i < 9; i++)
    {
        parameters->multiplier[i] = parameters->harmonics[i] * pow(2.0, parameters->detune[i] * (1.0 / 1200.0));
        parameters->phaseshift[i] = int(parameters->phase[i] * 65536 / 360) << 16;
    }
    double dphase = synth::midi_note_to_phase((int)parameters->foldover, 0, sample_rate);
    parameters->foldvalue = (int)(dphase);
}
