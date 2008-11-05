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
#include <calf/modules_synths.h>
#include <iostream>

using namespace std;
using namespace dsp;
using namespace calf_plugins;

CALF_PLUGIN_INFO(organ) = { 0x8481, "Organ", "Calf Organ", "Krzysztof Foltman", calf_plugins::calf_copyright_info, "SynthesizerPlugin" };

#define DRAWBAR_UI(no) \
            "<label  attach-x=\"" no "\" attach-y=\"0\" param=\"l" no "\"/>" \
            "<vscale attach-x=\"" no "\" attach-y=\"1\" param=\"l" no "\"/>" \
            "<value  attach-x=\"" no "\" attach-y=\"2\" param=\"l" no "\"/>" \
            "<knob   attach-x=\"" no "\" attach-y=\"3\" param=\"f" no "\"/>" \
            "<value  attach-x=\"" no "\" attach-y=\"4\" param=\"f" no "\"/>" \
            "<combo  attach-x=\"" no "\" attach-y=\"5\" param=\"w" no "\"/>" \
            "<knob   attach-x=\"" no "\" attach-y=\"6\" param=\"detune" no "\" type=\"1\"/>" \
            "<value  attach-x=\"" no "\" attach-y=\"7\" param=\"detune" no "\"/>" \
            "<knob   attach-x=\"" no "\" attach-y=\"8\" param=\"phase" no "\" type=\"3\"/>" \
            "<value  attach-x=\"" no "\" attach-y=\"9\" param=\"phase" no "\"/>" \
            "<knob   attach-x=\"" no "\" attach-y=\"10\" param=\"pan" no "\" type=\"1\"/>" \
            "<value  attach-x=\"" no "\" attach-y=\"11\" param=\"pan" no "\"/>" \
            "<combo  attach-x=\"" no "\" attach-y=\"12\" param=\"routing" no "\"/>" 

const char *organ_metadata::get_gui_xml()
{
    return 
    "<vbox border=\"10\">"
        "<hbox>"
            "<if cond=\"directlink\">"
                "<align scale-x=\"0.0\" scale-y=\"1.0\" align-x=\"0\" align-y=\"0\">"
                    "<line-graph param=\"master\" refresh=\"1\" width=\"120\" height=\"40\" expand=\"0\" fill=\"0\"/>"
                "</align>"
            "</if>"
            "<align scale-x=\"0.0\" scale-y=\"0.0\" align-x=\"1.0\" align-y=\"0.5\">"
                "<vbox>"
                    "<label param=\"master\"/>"
                    "<knob param=\"master\"/>"
                    "<value param=\"master\"/>"
                "</vbox>"
            "</align>"
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
                                    "<align><knob param=\"f1_env1\" expand=\"0\" fill=\"0\" type=\"1\"/></align><value param=\"f1_env1\"/>"
                                "</vbox>"
                                "<vbox>"
                                    "<label param=\"f1_env2\" />"
                                    "<align><knob param=\"f1_env2\" expand=\"0\" fill=\"0\" type=\"1\"/></align><value param=\"f1_env2\"/>"
                                "</vbox>"
                                "<vbox>"
                                    "<label param=\"f1_env3\" />"
                                    "<align><knob param=\"f1_env3\" expand=\"0\" fill=\"0\" type=\"1\"/></align><value param=\"f1_env3\"/>"
                                "</vbox>"
                            "</hbox>"
                            "<align>"
                                "<hbox>"
                                    "<toggle expand=\"0\" fill=\"0\" param=\"filter_chain\" />"
                                    "<label param=\"filter_chain\" />"
                                "</hbox>"
                            "</align>"
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
                                    "<align><knob param=\"f2_env1\" expand=\"0\" fill=\"0\" type=\"1\"/></align><value param=\"f2_env1\"/>"
                                "</vbox>"
                                "<vbox>"
                                    "<label param=\"f2_env2\" />"
                                    "<align><knob param=\"f2_env2\" expand=\"0\" fill=\"0\" type=\"1\"/></align><value param=\"f2_env2\"/>"
                                "</vbox>"
                                "<vbox>"
                                    "<label param=\"f2_env3\" />"
                                    "<align><knob param=\"f2_env3\" expand=\"0\" fill=\"0\" type=\"1\"/></align><value param=\"f2_env3\"/>"
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
            "</hbox>"
            "<vbox page=\"Advanced\">"
                "<align scale-x=\"1.0\" scale-y=\"0.3\">"
                    "<frame label=\"Percussive section\">"
                        "<vbox>"
                            "<align scale-x=\"0.0\" scale-y=\"1.0\"><vbox><keyboard octaves=\"10\"/><curve key=\"map_curve\" maxpoints=\"4\"/></vbox></align>"
                            "<hbox>"
                                "<table rows=\"2\" cols=\"5\">"
                                    "<vbox attach-x=\"0\" attach-y=\"0\">"
                                        "<label param=\"perc_waveform\"/>"
                                        "<align><combo param=\"perc_waveform\"/></align>"
                                    "</vbox>"        
                                    "<vbox attach-x=\"1\" attach-y=\"0\">"
                                        "<label param=\"perc_harmonic\"/>"
                                        "<knob param=\"perc_harmonic\" expand=\"0\" fill=\"0\"/>"
                                        "<value param=\"perc_harmonic\"/>"
                                    "</vbox>"        
                                    "<vbox attach-x=\"2\" attach-y=\"0\">"
                                        "<label param=\"perc_level\"/>"
                                        "<knob param=\"perc_level\" expand=\"0\" fill=\"0\"/>"
                                        "<value param=\"perc_level\"/>"
                                    "</vbox>"        
                                    "<vbox attach-x=\"3\" attach-y=\"0\">"
                                        "<label param=\"perc_decay\"/>"
                                        "<knob param=\"perc_decay\" expand=\"0\" fill=\"0\"/>"
                                        "<value param=\"perc_decay\"/>"
                                    "</vbox>"
                                    "<vbox attach-x=\"4\" attach-y=\"0\">"
                                        "<label param=\"perc_vel2amp\"/>"
                                        "<knob param=\"perc_vel2amp\" expand=\"0\" fill=\"0\"/>"
                                        "<value param=\"perc_vel2amp\"/>"
                                    "</vbox>"        
                                    "<vbox attach-x=\"0\" attach-y=\"1\">"
                                        "<label param=\"perc_fm_waveform\"/>"
                                        "<align><combo param=\"perc_fm_waveform\"/></align>"
                                    "</vbox>"        
                                    "<vbox attach-x=\"1\" attach-y=\"1\">"
                                        "<label param=\"perc_fm_harmonic\"/>"
                                        "<knob param=\"perc_fm_harmonic\" expand=\"0\" fill=\"0\"/>"
                                        "<value param=\"perc_fm_harmonic\"/>"
                                    "</vbox>"        
                                    "<vbox attach-x=\"2\" attach-y=\"1\">"
                                        "<label param=\"perc_fm_depth\"/>"
                                        "<knob param=\"perc_fm_depth\" expand=\"0\" fill=\"0\"/>"
                                        "<value param=\"perc_fm_depth\"/>"
                                    "</vbox>"        
                                    "<vbox attach-x=\"3\" attach-y=\"1\">"
                                        "<label param=\"perc_fm_decay\"/>"
                                        "<knob param=\"perc_fm_decay\" expand=\"0\" fill=\"0\"/>"
                                        "<value param=\"perc_fm_decay\"/>"
                                    "</vbox>"
                                    "<vbox attach-x=\"4\" attach-y=\"1\">"
                                        "<label param=\"perc_vel2fm\"/>"
                                        "<knob param=\"perc_vel2fm\" expand=\"0\" fill=\"0\"/>"
                                        "<value param=\"perc_vel2fm\"/>"
                                    "</vbox>"        
                                "</table>"
                                "<vbox>"
                                    "<hbox>"
                                        "<align>"
                                            "<vbox>"
                                                "<label param=\"perc_stereo\"/>"
                                                "<knob param=\"perc_stereo\" expand=\"0\" fill=\"0\" type=\"3\"/>"
                                                "<value param=\"perc_stereo\"/>"
                                            "</vbox>"
                                        "</align>"
                                        "<align>"
                                            "<vbox>"
                                                "<label param=\"perc_trigger\"/>"
                                                "<align>"
                                                    "<combo param=\"perc_trigger\"/>"
                                                "</align>"
                                            "</vbox>"        
                                        "</align>"
                                    "</hbox>"
                                "</vbox>"
                            "</hbox>"
                        "</vbox>"
                    "</frame>"
                "</align>"
                "<frame label=\"Vibrato\">"
                    "<hbox>"
                        "<vbox expand=\"0\" fill=\"0\">"
                            "<label param=\"vib_rate\" />"
                            "<align><knob param=\"vib_rate\" expand=\"0\" fill=\"0\"/></align><value param=\"vib_rate\"/>"
                        "</vbox>"
                        "<vbox expand=\"0\" fill=\"0\">"
                            "<label param=\"vib_amt\" />"
                            "<align><knob param=\"vib_amt\" expand=\"0\" fill=\"0\"/></align><value param=\"vib_amt\"/>"
                        "</vbox>"
                        "<vbox expand=\"0\" fill=\"0\">"
                            "<label param=\"vib_wet\" />"
                            "<align><knob param=\"vib_wet\" expand=\"0\" fill=\"0\"/></align><value param=\"vib_wet\"/>"
                        "</vbox>"
                        "<vbox expand=\"0\" fill=\"0\">"
                            "<label param=\"vib_phase\" />"
                            "<align><knob param=\"vib_phase\" expand=\"0\" fill=\"0\" type=\"3\"/></align><value param=\"vib_phase\"/>"
                        "</vbox>"
                        "<vbox expand=\"0\" fill=\"0\">"
                            "<label param=\"vib_mode\" />"
                            "<align><combo param=\"vib_mode\" expand=\"0\" fill=\"0\"/></align>"
                        "</vbox>"
                    "</hbox>"
                "</frame>"
                "<frame label=\"Additional settings\">"
                    "<hbox>"
                        "<vbox>"
                            "<label param=\"transpose\"/>"
                            "<align><knob param=\"transpose\" type=\"1\"/></align>"
                            "<value param=\"transpose\"/>"
                        "</vbox>"
                        "<vbox>"
                            "<label param=\"detune\"/>"
                            "<align><knob param=\"detune\" type=\"1\"/></align>"
                            "<value param=\"detune\"/>"
                        "</vbox>"
                        "<vbox>"
                            "<label param=\"foldnote\"/>"
                            "<align><knob param=\"foldnote\"/></align>"
                            "<value param=\"foldnote\"/>"
                        "</vbox>"
                    "</hbox>"
                "</frame>"
                "<vbox>"
                "</vbox>"
            "</vbox>"
        "</notebook>"
    "</vbox>"
    ;
}

const char **organ_metadata::get_default_configure_vars()
{
    static const char *data[] = { "map_curve", "2\n0 1\n1 1\n", NULL };
    return data;
}

plugin_command_info *organ_metadata::get_commands()
{
    static plugin_command_info cmds[] = {
        { "cmd_panic", "Panic!", "Stop all sounds and reset all controllers" },
        { NULL }
    };
    return cmds;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////

bool organ_audio_module::get_graph(int index, int subindex, float *data, int points, cairo_t *context)
{
    if (index == par_master) {
        organ_voice_base::precalculate_waves();
        if (subindex)
            return false;
        float *waveforms[9];
        int S[9], S2[9];
        enum { small_waves = organ_voice_base::wave_count_small};
        for (int i = 0; i < 9; i++)
        {
            int wave = dsp::clip((int)(parameters->waveforms[i]), 0, (int)organ_voice_base::wave_count - 1);
            if (wave >= small_waves)
            {
                waveforms[i] = organ_voice_base::get_big_wave(wave - small_waves).original;
                S[i] = ORGAN_BIG_WAVE_SIZE;
                S2[i] = ORGAN_WAVE_SIZE / 64;
            }
            else
            {
                waveforms[i] = organ_voice_base::get_wave(wave).original;
                S[i] = S2[i] = ORGAN_WAVE_SIZE;
            }
        }
        for (int i = 0; i < points; i++)
        {
            float sum = 0.f;
            for (int j = 0; j < 9; j++)
            {
                float shift = parameters->phase[j] * S[j] / 360.0;
                sum += parameters->drawbars[j] * waveforms[j][int(parameters->harmonics[j] * i * S2[j] / points + shift) & (S[j] - 1)];
            }
            data[i] = sum * 2 / (9 * 8);
        }
        return true;
    }
    return false;
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
    "P:SynStr","P:WideStr","P:Sine","P:Bell","P:Space","P:Voice","P:Hiss","P:Chant",
};

const char *organ_routing_names[] = { "Out", "Flt 1", "Flt 2"  };

const char *organ_ampctl_names[] = { "None", "Direct", "Flt 1", "Flt 2", "All"  };

const char *organ_vibrato_mode_names[] = { "None", "Direct", "Flt 1", "Flt 2", "Voice", "Global"  };

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
    
    { 200,         10,  3000, 100, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "perc_decay", "P: Carrier Decay" },
    { 0.25,      0,  1, 100, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB, NULL, "perc_level", "P: Level" },
    { 0,         0,  organ_voice_base::wave_count_small - 1, 1, PF_ENUM | PF_CTL_COMBO, organ_wave_names, "perc_waveform", "P: Carrier Wave" },
    { 6,      1, 32, 32, PF_INT | PF_SCALE_LINEAR | PF_CTL_KNOB, NULL, "perc_harmonic", "P: Carrier Frq" },
    { 0,          0,    1,    0, PF_FLOAT | PF_SCALE_PERC, NULL, "perc_vel2amp", "P: Vel->Amp" },
    
    { 200,         10,  3000, 100, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "perc_fm_decay", "P: Modulator Decay" },
    { 0,          0,    4,    0, PF_FLOAT | PF_SCALE_PERC, NULL, "perc_fm_depth", "P: FM Depth" },
    { 0,         0,  organ_voice_base::wave_count_small - 1, 1, PF_ENUM | PF_CTL_COMBO, organ_wave_names, "perc_fm_waveform", "P: Modulator Wave" },
    { 6,      1, 32, 32, PF_INT | PF_SCALE_LINEAR | PF_CTL_KNOB, NULL, "perc_fm_harmonic", "P: Modulator Frq" },
    { 0,          0,    1,    0, PF_FLOAT | PF_SCALE_PERC, NULL, "perc_vel2fm", "P: Vel->FM" },
    
    { 0,         0,  organ_voice_base::perctrig_count - 1, 0, PF_ENUM | PF_CTL_COMBO, organ_percussion_trigger_names, "perc_trigger", "P: Trigger" },
    { 90,      0,360, 361, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_DEG, NULL, "perc_stereo", "P: Stereo Phase" },

    { 0,         0,  1, 0, PF_BOOL | PF_CTL_TOGGLE, NULL, "filter_chain", "Filter 1 To 2" },
    { 0.1,         0,  1, 100, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB | PF_PROP_OUTPUT_GAIN, NULL, "master", "Volume" },
    
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
    { organ_voice_base::lfomode_global,        0,  organ_voice_base::lfomode_count - 1,    0, PF_ENUM | PF_CTL_COMBO, organ_vibrato_mode_names, "vib_mode", "Vib Mode" },
//    { 0,  0, organ_voice_base::ampctl_count - 1,
//                              0, PF_INT | PF_CTL_COMBO, organ_ampctl_names, "vel_amp_ctl", "Vel To Amp"},

    { -12,        -24, 24,   49, PF_INT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_SEMITONES, NULL, "transpose", "Transpose" },
    { 0,       -100,  100,  201, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_CENTS, NULL, "detune", "Detune" },
};

////////////////////////////////////////////////////////////////////////////

organ_voice_base::small_wave_family (*organ_voice_base::waves)[organ_voice_base::wave_count_small];
organ_voice_base::big_wave_family (*organ_voice_base::big_waves)[organ_voice_base::wave_count_big];

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
        float frac = i * 2.0 / ORGAN_WAVE_SIZE;
        float phase = M_PI / sqrt(frac) ;
        complex<float> shift = complex<float>(cos(phase), sin(phase));
        bl.spectrum[i] *= shift;
        bl.spectrum[ORGAN_WAVE_SIZE - i] *= conj(shift);
    }
    bl.compute_waveform(tmp);
    normalize_waveform(tmp, ORGAN_WAVE_SIZE);
}

static void padsynth(bandlimiter<ORGAN_WAVE_BITS> blSrc, bandlimiter<ORGAN_BIG_WAVE_BITS> &blDest, organ_voice_base::big_wave_family &result, int bwscale = 20, float bell_factor = 0, bool foldover = false)
{
    // kept in a vector to avoid putting large arrays on stack
    vector<complex<float> >orig_spectrum;
    orig_spectrum.resize(ORGAN_WAVE_SIZE / 2);
    for (int i = 0; i < ORGAN_WAVE_SIZE / 2; i++) 
    {
        orig_spectrum[i] = blSrc.spectrum[i];
//        printf("@%d = %f\n", i, abs(orig_spectrum[i]));
    }
    
    int periods = (1 << ORGAN_BIG_WAVE_SHIFT) * ORGAN_BIG_WAVE_SIZE / ORGAN_WAVE_SIZE;
    for (int i = 0; i <= ORGAN_BIG_WAVE_SIZE / 2; i++) {
        blDest.spectrum[i] = 0;
    }
    for (int i = 1; i <= (ORGAN_WAVE_SIZE >> (1 + ORGAN_BIG_WAVE_SHIFT)); i++) {
        //float esc = 0.25 * (1 + 0.5 * log(i));
        float esc = 0.5;
        float amp = abs(blSrc.spectrum[i]);
        int bw = 1 + 20 * i;
        float sum = 1;
        int delta = 1;
        if (bw > 20) delta = bw / 20;
        for (int j = delta; j <= bw; j+=delta)
        {
            float p = j * 1.0 / bw;
            sum += 2 * exp(-p * p * esc);
        }
        if (sum < 0.0001)
            continue;
        amp *= (ORGAN_BIG_WAVE_SIZE / ORGAN_WAVE_SIZE);
        amp /= sum;
        int orig = i * periods + bell_factor * cos(i);
        if (orig > 0 && orig < ORGAN_BIG_WAVE_SIZE / 2)
            blDest.spectrum[orig] += amp;
        for (int j = delta; j <= bw; j += delta)
        {
            float p = j * 1.0 / bw;
            float val = amp * exp(-p * p * esc);
            int pos = orig + j * bwscale / 40;
            if (pos < 1 || pos >= ORGAN_BIG_WAVE_SIZE / 2)
                continue;
            int pos2 = 2 * orig - pos;
            if (pos2 < 1 || pos2 >= ORGAN_BIG_WAVE_SIZE / 2)
                continue;
            blDest.spectrum[pos] += val;
            if (j)
                blDest.spectrum[pos2] += val;
        }
    }
    for (int i = 1; i <= ORGAN_BIG_WAVE_SIZE / 2; i++) {
        float phase = M_PI * 2 * (rand() & 255) / 256;
        complex<float> shift = complex<float>(cos(phase), sin(phase));
        blDest.spectrum[i] *= shift;        
//      printf("@%d = %f\n", i, abs(blDest.spectrum[i]));
        
        blDest.spectrum[ORGAN_BIG_WAVE_SIZE - i] = conj(blDest.spectrum[i]);
    }
    // same as above - put large array on heap to avoid stack overflow in ingen
    vector<float> tmp;
    tmp.resize(ORGAN_BIG_WAVE_SIZE);
    blDest.compute_waveform(tmp.data());
    normalize_waveform(tmp.data(), ORGAN_BIG_WAVE_SIZE);
    blDest.compute_spectrum(tmp.data());
    
    // limit is 1/2 of the number of harmonics of the original wave
    result.make_from_spectrum(blDest, foldover, ORGAN_WAVE_SIZE >> (1 + ORGAN_BIG_WAVE_SHIFT));
    memcpy(result.original, result.begin()->second, sizeof(result.original));
    #if 0
    blDest.compute_waveform(result);
    normalize_waveform(result, ORGAN_BIG_WAVE_SIZE);
    result[ORGAN_BIG_WAVE_SIZE] = result[0];
    for (int i =0 ; i<ORGAN_BIG_WAVE_SIZE + 1; i++)
        printf("%f\n", result[i]);
    #endif
}

void organ_voice_base::precalculate_waves()
{
    static bool inited = false;
    if (!inited)
    {
        static organ_voice_base::small_wave_family waves[organ_voice_base::wave_count_small];
        static organ_voice_base::big_wave_family big_waves[organ_voice_base::wave_count_big];
        organ_voice_base::waves = &waves;
        organ_voice_base::big_waves = &big_waves;
        
        float tmp[ORGAN_WAVE_SIZE];
        static bandlimiter<ORGAN_WAVE_BITS> bl;
        static bandlimiter<ORGAN_BIG_WAVE_BITS> blBig;
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
        normalize_waveform(tmp, ORGAN_WAVE_SIZE);
        smoothen(bl, tmp);
        waves[wave_ssqr].make(bl, tmp);
        
        for (int i = 0; i < ORGAN_WAVE_SIZE; i++)
            tmp[i] = -1 + (i * 2.0 / ORGAN_WAVE_SIZE);
        normalize_waveform(tmp, ORGAN_WAVE_SIZE);
        smoothen(bl, tmp);
        waves[wave_ssaw].make(bl, tmp);

        for (int i = 0; i < ORGAN_WAVE_SIZE; i++)
            tmp[i] = (i < (ORGAN_WAVE_SIZE / 16)) ? 1 : 0;
        normalize_waveform(tmp, ORGAN_WAVE_SIZE);
        smoothen(bl, tmp);
        waves[wave_spls].make(bl, tmp);
        for (int i = 0; i < ORGAN_WAVE_SIZE; i++)
            tmp[i] = i < (ORGAN_WAVE_SIZE / 1.5) ? sin(i * 1.5 * 2 * M_PI / ORGAN_WAVE_SIZE) : 0;
        normalize_waveform(tmp, ORGAN_WAVE_SIZE);
        waves[wave_sinepl05].make(bl, tmp);
        for (int i = 0; i < ORGAN_WAVE_SIZE; i++)
            tmp[i] = i < (ORGAN_WAVE_SIZE / 1.5) ? (i < ORGAN_WAVE_SIZE / 3 ? -1 : +1) : 0;
        normalize_waveform(tmp, ORGAN_WAVE_SIZE);
        waves[wave_sqr05].make(bl, tmp);
        for (int i = 0; i < ORGAN_WAVE_SIZE; i++)
            tmp[i] = sin(i * M_PI / ORGAN_WAVE_SIZE);
        normalize_waveform(tmp, ORGAN_WAVE_SIZE);
        waves[wave_halfsin].make(bl, tmp);
        for (int i = 0; i < ORGAN_WAVE_SIZE; i++)
            tmp[i] = sin(i * 3 * M_PI / ORGAN_WAVE_SIZE);
        normalize_waveform(tmp, ORGAN_WAVE_SIZE);
        waves[wave_clvg].make(bl, tmp);
        for (int i = 0; i < ORGAN_WAVE_SIZE; i++)
        {
            float ph = i * 2 * M_PI / ORGAN_WAVE_SIZE;
            float fm = 0.3 * sin(6*ph) + 0.2 * sin(11*ph) + 0.2 * cos(17*ph) - 0.2 * cos(19*ph);
            tmp[i] = sin(5*ph + fm) + 0.7 * cos(7*ph - fm);
        }
        normalize_waveform(tmp, ORGAN_WAVE_SIZE);
        waves[wave_bell].make(bl, tmp, true);
        for (int i = 0; i < ORGAN_WAVE_SIZE; i++)
        {
            float ph = i * 2 * M_PI / ORGAN_WAVE_SIZE;
            float fm = 0.3 * sin(3*ph) + 0.3 * sin(11*ph) + 0.3 * cos(17*ph) - 0.3 * cos(19*ph)  + 0.3 * cos(25*ph)  - 0.3 * cos(31*ph) + 0.3 * cos(37*ph);
            tmp[i] = sin(3*ph + fm) + cos(7*ph - fm);
        }
        normalize_waveform(tmp, ORGAN_WAVE_SIZE);
        waves[wave_bell2].make(bl, tmp, true);
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

        for (int i = 0; i < ORGAN_WAVE_SIZE; i++)
            tmp[i] = -1 + (i * 2.0 / ORGAN_WAVE_SIZE);
        normalize_waveform(tmp, ORGAN_WAVE_SIZE);
        bl.compute_spectrum(tmp);
        padsynth(bl, blBig, big_waves[wave_strings - wave_count_small], 15);

        for (int i = 0; i < ORGAN_WAVE_SIZE; i++)
            tmp[i] = -1 + (i * 2.0 / ORGAN_WAVE_SIZE);
        normalize_waveform(tmp, ORGAN_WAVE_SIZE);
        bl.compute_spectrum(tmp);
        padsynth(bl, blBig, big_waves[wave_strings2 - wave_count_small], 40);

        for (int i = 0; i < ORGAN_WAVE_SIZE; i++)
            tmp[i] = sin(i * 2 * M_PI / ORGAN_WAVE_SIZE);
        normalize_waveform(tmp, ORGAN_WAVE_SIZE);
        bl.compute_spectrum(tmp);
        padsynth(bl, blBig, big_waves[wave_sinepad - wave_count_small], 20);

        for (int i = 0; i < ORGAN_WAVE_SIZE; i++)
        {
            float ph = i * 2 * M_PI / ORGAN_WAVE_SIZE;
            float fm = 0.3 * sin(6*ph) + 0.2 * sin(11*ph) + 0.2 * cos(17*ph) - 0.2 * cos(19*ph);
            tmp[i] = sin(5*ph + fm) + 0.7 * cos(7*ph - fm);
        }
        normalize_waveform(tmp, ORGAN_WAVE_SIZE);
        bl.compute_spectrum(tmp);
        padsynth(bl, blBig, big_waves[wave_bellpad - wave_count_small], 30, 30, true);

        for (int i = 0; i < ORGAN_WAVE_SIZE; i++)
        {
            float ph = i * 2 * M_PI / ORGAN_WAVE_SIZE;
            float fm = 0.3 * sin(3*ph) + 0.2 * sin(4*ph) + 0.2 * cos(5*ph) - 0.2 * cos(6*ph);
            tmp[i] = sin(2*ph + fm) + 0.7 * cos(3*ph - fm);
        }
        normalize_waveform(tmp, ORGAN_WAVE_SIZE);
        bl.compute_spectrum(tmp);
        padsynth(bl, blBig, big_waves[wave_space - wave_count_small], 30, 30);

        for (int i = 0; i < ORGAN_WAVE_SIZE; i++)
        {
            float ph = i * 2 * M_PI / ORGAN_WAVE_SIZE;
            float fm = 0.5 * sin(ph) + 0.5 * sin(2*ph) + 0.5 * sin(3*ph);
            tmp[i] = sin(ph + fm) + 0.5 * cos(7*ph - 2 * fm) + 0.25 * cos(13*ph - 4 * fm);
        }
        normalize_waveform(tmp, ORGAN_WAVE_SIZE);
        bl.compute_spectrum(tmp);
        padsynth(bl, blBig, big_waves[wave_choir - wave_count_small], 50, 10);

        for (int i = 0; i < ORGAN_WAVE_SIZE; i++)
        {
            float ph = i * 2 * M_PI / ORGAN_WAVE_SIZE;
            float fm = sin(ph) ;
            tmp[i] = sin(ph + fm) + 0.25 * cos(11*ph - 2 * fm) + 0.125 * cos(23*ph - 2 * fm) + 0.0625 * cos(49*ph - 2 * fm);
        }
        normalize_waveform(tmp, ORGAN_WAVE_SIZE);
        bl.compute_spectrum(tmp);
        padsynth(bl, blBig, big_waves[wave_choir2 - wave_count_small], 50, 10);

        for (int i = 0; i < ORGAN_WAVE_SIZE; i++)
        {
            float ph = i * 2 * M_PI / ORGAN_WAVE_SIZE;
            float fm = sin(ph) ;
            tmp[i] = sin(ph + 4 * fm) + 0.5 * sin(2 * ph + 4 * ph);
        }
        normalize_waveform(tmp, ORGAN_WAVE_SIZE);
        bl.compute_spectrum(tmp);
        padsynth(bl, blBig, big_waves[wave_choir3 - wave_count_small], 50, 10);
        
        inited = true;
    }
}

organ_voice_base::organ_voice_base(organ_parameters *_parameters, int &_sample_rate_ref, bool &_released_ref)
: parameters(_parameters)
, sample_rate_ref(_sample_rate_ref)
, released_ref(_released_ref)
{
    note = -1;
}

void organ_voice_base::render_percussion_to(float (*buf)[2], int nsamples)
{
    if (note == -1)
        return;

    if (!pamp.get_active())
        return;
    if (parameters->percussion_level < small_value<float>())
        return;
    float level = parameters->percussion_level * 9;
    static float zeros[ORGAN_WAVE_SIZE];
    // XXXKF the decay needs work!
    double age_const = parameters->perc_decay_const;
    double fm_age_const = parameters->perc_fm_decay_const;
    int timbre = parameters->get_percussion_wave();
    if (timbre < 0 || timbre >= wave_count_small)
        return;
    int timbre2 = parameters->get_percussion_fm_wave();
    if (timbre2 < 0 || timbre2 >= wave_count_small)
        timbre2 = wave_sine;
    float *fmdata = (*waves)[timbre2].get_level(moddphase.get());
    if (!fmdata)
        fmdata = zeros;
    float *data = (*waves)[timbre].get_level(dpphase.get());
    if (!data) {
        pamp.deactivate();
        return;
    }
    float s = parameters->percussion_stereo * ORGAN_WAVE_SIZE * (0.5 / 360.0);
    for (int i = 0; i < nsamples; i++) {
        float fm = wave(fmdata, modphase);
        fm *= ORGAN_WAVE_SIZE * parameters->percussion_fm_depth * fm_amp.get();
        modphase += moddphase;
        fm_amp.age_exp(fm_age_const, 1.0 / 32768.0);

        float lamp = level * pamp.get();
        buf[i][0] += lamp * wave(data, pphase + dsp::fixed_point<int64_t, 52>(fm - s));
        buf[i][1] += lamp * wave(data, pphase + dsp::fixed_point<int64_t, 52>(fm + s));
        if (released_ref)
            pamp.age_lin(rel_age_const,0.0);
        else
            pamp.age_exp(age_const, 1.0 / 32768.0);
        pphase += dpphase;
    }
}

void organ_vibrato::reset()
{
    for (int i = 0; i < VibratoSize; i++)
        vibrato_x1[i][0] = vibrato_y1[i][0] = vibrato_x1[i][1] = vibrato_y1[i][1] = 0.f;
    lfo_phase = 0.f;
}

void organ_vibrato::process(organ_parameters *parameters, float (*data)[2], unsigned int len, float sample_rate)
{
    float lfo1 = lfo_phase < 0.5 ? 2 * lfo_phase : 2 - 2 * lfo_phase;
    float lfo_phase2 = lfo_phase + parameters->lfo_phase * (1.0 / 360.0);
    if (lfo_phase2 >= 1.0)
        lfo_phase2 -= 1.0;
    float lfo2 = lfo_phase2 < 0.5 ? 2 * lfo_phase2 : 2 - 2 * lfo_phase2;
    lfo_phase += parameters->lfo_rate * len / sample_rate;
    if (lfo_phase >= 1.0)
        lfo_phase -= 1.0;
    vibrato[0].set_ap(3000 + 7000 * parameters->lfo_amt * lfo1 * lfo1, sample_rate);
    vibrato[1].set_ap(3000 + 7000 * parameters->lfo_amt * lfo2 * lfo2, sample_rate);
    
    float vib_wet = parameters->lfo_wet;
    for (int c = 0; c < 2; c++)
    {
        for (unsigned int i = 0; i < len; i++)
        {
            float v = data[i][c];
            float v0 = v;
            for (int t = 0; t < VibratoSize; t++)
                v = vibrato[c].process_ap(v, vibrato_x1[t][c], vibrato_y1[t][c]);
            
            data[i][c] += (v - v0) * vib_wet;
        }
        for (int t = 0; t < VibratoSize; t++)
        {
            sanitize(vibrato_x1[t][c]);
            sanitize(vibrato_y1[t][c]);
        }
    }
}

void organ_voice::update_pitch()
{
    organ_voice_base::update_pitch();
    dphase.set(dsp::midi_note_to_phase(note, 100 * parameters->global_transpose + parameters->global_detune, sample_rate) * parameters->pitch_bend);
}

void organ_voice::render_block() {
    if (note == -1)
        return;

    dsp::zero(&output_buffer[0][0], Channels * BlockSize);
    dsp::zero(&aux_buffers[1][0][0], 2 * Channels * BlockSize);
    if (!amp.get_active())
    {
        if (use_percussion())
            render_percussion_to(output_buffer, BlockSize);
        return;
    }

    update_pitch();
    dsp::fixed_point<int, 20> tphase, tdphase;
    unsigned int foldvalue = parameters->foldvalue;
    int vibrato_mode = fastf2i_drm(parameters->lfo_mode);
    for (int h = 0; h < 9; h++)
    {
        float amp = parameters->drawbars[h];
        if (amp < small_value<float>())
            continue;
        float *data;
        dsp::fixed_point<int, 24> hm = dsp::fixed_point<int, 24>(parameters->multiplier[h]);
        int waveid = (int)parameters->waveforms[h];
        if (waveid < 0 || waveid >= wave_count)
            waveid = 0;

        uint32_t rate = (dphase * hm).get();
        if (waveid >= wave_count_small)
        {
            float *data = (*big_waves)[waveid - wave_count_small].get_level(rate >> (ORGAN_BIG_WAVE_BITS - ORGAN_WAVE_BITS + ORGAN_BIG_WAVE_SHIFT));
            if (!data)
                continue;
            hm.set(hm.get() >> ORGAN_BIG_WAVE_SHIFT);
            dsp::fixed_point<int64_t, 20> tphase, tdphase;
            tphase.set(((phase * hm).get()) + parameters->phaseshift[h]);
            tdphase.set(rate >> ORGAN_BIG_WAVE_SHIFT);
            float ampl = amp * 0.5f * (1 - parameters->pan[h]);
            float ampr = amp * 0.5f * (1 + parameters->pan[h]);
            float (*out)[Channels] = aux_buffers[dsp::fastf2i_drm(parameters->routing[h])];
            
            for (int i=0; i < (int)BlockSize; i++) {
                float wv = big_wave(data, tphase);
                out[i][0] += wv * ampl;
                out[i][1] += wv * ampr;
                tphase += tdphase;
            }
        }
        else
        {
            unsigned int foldback = 0;
            while (rate > foldvalue)
            {
                rate >>= 1;
                foldback++;
            }
            hm.set(hm.get() >> foldback);
            data = (*waves)[waveid].get_level(rate);
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
        }
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
    for (int i = 0; i < ampctl_count - 1; i++)
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
    if (vibrato_mode >= lfomode_direct && vibrato_mode <= lfomode_filter2)
        vibrato.process(parameters, aux_buffers[vibrato_mode - lfomode_direct], BlockSize, sample_rate);
    if (!any_running)
        finishing = true;
    // calculate delta from pre and post
    for (int i = 0; i < ampctl_count - 1; i++)
        amp_post[i] = (amp_post[i] - amp_pre[i]) * (1.0 / BlockSize);
    float a0 = amp_pre[0], a1 = amp_pre[1], a2 = amp_pre[2], a3 = amp_pre[3];
    float d0 = amp_post[0], d1 = amp_post[1], d2 = amp_post[2], d3 = amp_post[3];
    if (parameters->filter_chain >= 0.5f)
    {
        for (int i=0; i < (int) BlockSize; i++) {
            output_buffer[i][0] = a3 * (a0 * output_buffer[i][0] + a2 * filterL[1].process(a1 * filterL[0].process(aux_buffers[1][i][0]) + aux_buffers[2][i][0]));
            output_buffer[i][1] = a3 * (a0 * output_buffer[i][1] + a2 * filterR[1].process(a1 * filterR[0].process(aux_buffers[1][i][1]) + aux_buffers[2][i][1]));
            a0 += d0, a1 += d1, a2 += d2, a3 += d3;
        }
    }
    else
    {
        for (int i=0; i < (int) BlockSize; i++) {
            output_buffer[i][0] = a3 * (a0 * output_buffer[i][0] + a1 * filterL[0].process(aux_buffers[1][i][0]) + a2 * filterL[1].process(aux_buffers[2][i][0]));
            output_buffer[i][1] = a3 * (a0 * output_buffer[i][1] + a1 * filterR[0].process(aux_buffers[1][i][1]) + a2 * filterR[1].process(aux_buffers[2][i][1]));
            a0 += d0, a1 += d1, a2 += d2, a3 += d3;
        }
    }
    filterL[0].sanitize();
    filterR[0].sanitize();
    filterL[1].sanitize();
    filterR[1].sanitize();
    if (vibrato_mode == lfomode_voice)
        vibrato.process(parameters, output_buffer, BlockSize, sample_rate);

    if (finishing)
    {
        for (int i = 0; i < (int) BlockSize; i++) {
            output_buffer[i][0] *= amp.get();
            output_buffer[i][1] *= amp.get();
            amp.age_lin((1.0/44100.0)/0.03,0.0);
        }
    }
    
    if (use_percussion())
        render_percussion_to(output_buffer, BlockSize);
}

void drawbar_organ::update_params()
{
    parameters->perc_decay_const = dsp::decay::calc_exp_constant(1.0 / 1024.0, 0.001 * parameters->percussion_time * sample_rate);
    parameters->perc_fm_decay_const = dsp::decay::calc_exp_constant(1.0 / 1024.0, 0.001 * parameters->percussion_fm_time * sample_rate);
    for (int i = 0; i < 9; i++)
    {
        parameters->multiplier[i] = parameters->harmonics[i] * pow(2.0, parameters->detune[i] * (1.0 / 1200.0));
        parameters->phaseshift[i] = int(parameters->phase[i] * 65536 / 360) << 16;
    }
    double dphase = dsp::midi_note_to_phase((int)parameters->foldover, 0, sample_rate);
    parameters->foldvalue = (int)(dphase);
}

void drawbar_organ::pitch_bend(int amt)
{
    parameters->pitch_bend = pow(2.0, amt * 2 / (12.0 * 8192));
    for (list<voice *>::iterator i = active_voices.begin(); i != active_voices.end(); i++)
    {
        organ_voice *v = dynamic_cast<organ_voice *>(*i);
        v->update_pitch();
    }
    percussion.update_pitch();
}

void organ_audio_module::execute(int cmd_no)
{
    switch(cmd_no)
    {
        case 0:
            panic_flag = true;
            break;
    }
}

void organ_voice_base::perc_note_on(int note, int vel)
{
    perc_reset();
    released_ref = false;
    this->note = note;
    pamp.set(1.0f + (vel - 127) * parameters->percussion_vel2amp / 127.0);
    update_pitch();
    float (*kt)[2] = parameters->percussion_keytrack;
    // assume last point (will be put there by padding)
    fm_keytrack = kt[ORGAN_KEYTRACK_POINTS - 1][1];
    // yes binary search would be nice if we had more than those crappy 4 points
    for (int i = 0; i < ORGAN_KEYTRACK_POINTS - 1; i++)
    {
        float &lower = kt[i][0], upper = kt[i + 1][0];
        if (note >= lower && note < upper)
        {
            fm_keytrack = kt[i][1] + (note - lower) * (kt[i + 1][1] - kt[i][1]) / (upper - lower);
            break;
        }
    }
    fm_amp.set(fm_keytrack * (1.0f + (vel - 127) * parameters->percussion_vel2fm / 127.0));
}

char *organ_audio_module::configure(const char *key, const char *value)
{
    if (!strcmp(key, "map_curve"))
    {
        var_map_curve = value;
        stringstream ss(value);
        int i = 0;
        float x = 0, y = 1;
        if (*value)
        {
            int points;
            ss >> points;
            for (i = 0; i < points; i++)
            {
                static const int whites[] = { 0, 2, 4, 5, 7, 9, 11 };
                ss >> x >> y;
                int wkey = (int)(x * 71);
                x = whites[wkey % 7] + 12 * (wkey / 7);
                parameters->percussion_keytrack[i][0] = x;
                parameters->percussion_keytrack[i][1] = y;
                // cout << "(" << x << ", " << y << ")" << endl;
            }
        }
        // pad with constant Y
        for (; i < ORGAN_KEYTRACK_POINTS; i++) {
            parameters->percussion_keytrack[i][0] = x;
            parameters->percussion_keytrack[i][1] = y;
        }
        return NULL;
    }
    cout << "Set configure value " << key << " to " << value;
    return NULL;
}

void organ_audio_module::send_configures(send_configure_iface *sci)
{
    sci->send_configure("map_curve", var_map_curve.c_str());
}

