/* Calf DSP Library
 * Example audio modules - monosynth
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
#include <complex>
#if USE_JACK
#include <jack/jack.h>
#endif
#include <calf/giface.h>
#include <calf/modules.h>
#include <calf/modules_synths.h>

using namespace synth;
using namespace std;

const char *monosynth_audio_module::port_names[] = {
    "Out L", "Out R", 
};

const char *monosynth_waveform_names[] = { "Sawtooth", "Square", "Pulse", "Sine", "Triangle", "Varistep", "Skewed Saw", "Skewed Square", 
    "Test 1", "Test 2", "Test 3", "Test 4", "Test 5", "Test 6", "Test 7", "Test 8" };
const char *monosynth_mode_names[] = { "0 : 0", "0 : 180", "0 : 90", "90 : 90", "90 : 270", "Random" };
const char *monosynth_legato_names[] = { "Retrig", "Legato", "Fng Retrig", "Fng Legato" };

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

static const char *monosynth_gui_xml =
    "<vbox border=\"10\">"
        "<hbox spacing=\"10\">"
            "<frame label=\"Oscillators\">"
                "<vbox border=\"10\" spacing=\"10\">"
                    "<table rows=\"2\" cols=\"2\">"
                    "<label attach-x=\"0\" attach-y=\"0\" param=\"o1_wave\" />"
                    "<label attach-x=\"1\" attach-y=\"0\" param=\"o2_wave\" />"
                    "<combo attach-x=\"0\" attach-y=\"1\" param=\"o1_wave\" />"
                    "<combo attach-x=\"1\" attach-y=\"1\" param=\"o2_wave\" />"
                    "</table>"
                    "<hbox>"
                        "<line-graph param=\"o1_wave\"/>"
                        "<vbox>"
                            "<label param=\"o12_mix\"/>"
                            "<hscale param=\"o12_mix\" position=\"bottom\"/>"
                        "</vbox>"
                        "<line-graph param=\"o2_wave\"/>"
                    "</hbox>"
                    "<hbox>"
                        "<vbox>"
                        "  <label param=\"o12_detune\" />"
                        "  <align align-x=\"0.5\" align-y=\"0.5\"><knob param=\"o12_detune\" expand=\"0\" fill=\"0\"/></align><value param=\"o12_detune\"/>"
                        "</vbox>"
                        "<align align-x=\"0.5\" align-y=\"0.5\">"
                        "  <vbox>"
                        "    <label param=\"phase_mode\" />"
                        "    <combo param=\"phase_mode\"/>"
                        "  </vbox>"
                        "</align>" 
                        "<vbox>"
                        "  <label param=\"o2_xpose\" />"
                        "  <align align-x=\"0.5\" align-y=\"0.5\"><knob param=\"o2_xpose\" expand=\"0\" fill=\"0\"/></align><value param=\"o2_xpose\"/>"
                        "</vbox>"
                    "</hbox>"
                "</vbox>"
            "</frame>"
            "<frame label=\"Filter\">"
                "<vbox border=\"10\">"
                    "<align align-x=\"0.5\" align-y=\"0.5\">"
                        "<hbox>"
                            "<label param=\"filter\" /><combo param=\"filter\" />"
                            "<line-graph param=\"filter\" refresh=\"1\"/>"
                        "</hbox>"
                    "</align>"
                    "<hbox>"
                        "<vbox>"
                        "  <label param=\"cutoff\" />"
                        "  <align align-x=\"0.5\" align-y=\"0.5\"><knob param=\"cutoff\" expand=\"0\" fill=\"0\"/></align><value param=\"cutoff\"/>"
                        "</vbox>"
                        "<vbox>"
                        "  <label param=\"res\" />"
                        "  <align align-x=\"0.5\" align-y=\"0.5\"><knob param=\"res\" expand=\"0\" fill=\"0\"/></align><value param=\"res\"/>"
                        "</vbox>"
                        "<vbox>"
                        "  <label param=\"filter_sep\" />"
                        "  <align align-x=\"0.5\" align-y=\"0.5\"><knob param=\"filter_sep\" expand=\"0\" fill=\"0\"/></align><value param=\"filter_sep\"/>"
                        "</vbox>"
                    "</hbox>"
                "</vbox>"
            "</frame>"
        "</hbox>"
        "<hbox spacing=\"10\">"
            "<frame label=\"Envelope\">"
                "<vbox border=\"10\" spacing=\"10\">"
                    "<hbox>"
                        "<vbox>"
                        "  <label param=\"adsr_a\" />"
                        "  <align align-x=\"0.5\" align-y=\"0.5\"><knob param=\"adsr_a\" expand=\"0\" fill=\"0\"/></align><value param=\"adsr_a\"/>"
                        "</vbox>"
                        "<vbox>"
                        "  <label param=\"adsr_d\" />"
                        "  <align align-x=\"0.5\" align-y=\"0.5\"><knob param=\"adsr_d\" expand=\"0\" fill=\"0\"/></align><value param=\"adsr_d\"/>"
                        "</vbox>"
                        "<vbox>"
                        "  <label param=\"adsr_s\" />"
                        "  <align align-x=\"0.5\" align-y=\"0.5\"><knob param=\"adsr_s\" expand=\"0\" fill=\"0\"/></align><value param=\"adsr_s\"/>"
                        "</vbox>"
                        "<vbox>"
                        "  <label param=\"adsr_r\" />"
                        "  <align align-x=\"0.5\" align-y=\"0.5\"><knob param=\"adsr_r\" expand=\"0\" fill=\"0\"/></align><value param=\"adsr_r\"/>"
                        "</vbox>"
                    "</hbox>"
                    "<hbox>"
                        "<vbox>"
                        "  <label param=\"env2cutoff\" />"
                        "  <align align-x=\"0.5\" align-y=\"0.5\"><knob param=\"env2cutoff\" expand=\"0\" fill=\"0\"/></align><value param=\"env2cutoff\"/>"
                        "</vbox>"
                        "<vbox>"
                        "  <label param=\"env2res\" />"
                        "  <align align-x=\"0.5\" align-y=\"0.5\"><knob param=\"env2res\" expand=\"0\" fill=\"0\"/></align><value param=\"env2res\"/>"
                        "</vbox>"
                        "<vbox>"
                        "  <label param=\"env2amp\" />"
                        "  <align align-x=\"0.5\" align-y=\"0.5\"><knob param=\"env2amp\" expand=\"0\" fill=\"0\"/></align><value param=\"env2amp\"/>"
                        "</vbox>"
                    "</hbox>"
                "</vbox>"
            "</frame>"
            "<frame label=\"Settings\">"
                "<vbox border=\"10\" spacing=\"10\">"
                    "<hbox>"
                        "<vbox>"
                            "<label param=\"key_follow\" />"
                            "<align align-x=\"0.5\" align-y=\"0.5\"><toggle param=\"key_follow\" /></align>"
                        "</vbox>"
                        "<vbox>"
                            "<label param=\"legato\"  expand=\"0\"/>"
                            "<combo param=\"legato\" expand=\"0\" fill=\"0\"/>"
                        "</vbox>"
                    "</hbox>"
                    "<hbox>"
                        "<vbox>"
                        "  <label param=\"portamento\" />"
                        "  <align align-x=\"0.5\" align-y=\"0.5\"><knob param=\"portamento\" /></align><value param=\"portamento\"/>"
                        "</vbox>"
                        "<vbox>"
                        "  <label param=\"vel2filter\" />"
                        "  <align align-x=\"0.5\" align-y=\"0.5\"><knob param=\"vel2filter\" /></align><value param=\"vel2filter\"/>"
                        "</vbox>"
                        "<vbox>"
                        "  <label param=\"vel2amp\" />"
                        "  <align align-x=\"0.5\" align-y=\"0.5\"><knob param=\"vel2amp\" /></align><value param=\"vel2amp\"/>"
                        "</vbox>"
                    "</hbox>"
                "</vbox>"
            "</frame>"
        "</hbox>"
    "</vbox>";

parameter_properties monosynth_audio_module::param_props[] = {
    { wave_saw,         0, wave_count - 1, 1, PF_ENUM | PF_CTL_COMBO, monosynth_waveform_names, "o1_wave", "Osc1 Wave" },
    { wave_sqr,         0, wave_count - 1, 1, PF_ENUM | PF_CTL_COMBO, monosynth_waveform_names, "o2_wave", "Osc2 Wave" },
    { 10,         0,  100, 1.01, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_CENTS, NULL, "o12_detune", "O1<>2 Detune" },
    { 12,       -24,   24, 1.01, PF_INT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_SEMITONES, NULL, "o2_xpose", "Osc 2 transpose" },
    { 0,          0,    5, 1.01, PF_ENUM | PF_CTL_COMBO, monosynth_mode_names, "phase_mode", "Phase mode" },
    { 0.5,        0,    1, 1.01, PF_FLOAT | PF_SCALE_PERC, NULL, "o12_mix", "O1<>2 Mix" },
    { 1,          0,    7, 1.01, PF_ENUM | PF_CTL_COMBO, monosynth_filter_choices, "filter", "Filter" },
    { 33,        10,16000, 1.01, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL, "cutoff", "Cutoff" },
    { 2,        0.7,    8, 1.01, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB, NULL, "res", "Resonance" },
    { 0,      -2400, 2400, 1.01, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_CENTS, NULL, "filter_sep", "Separation" },
    { 8000,  -10800,10800, 1.01, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_CENTS, NULL, "env2cutoff", "Env->Cutoff" },
    { 1,          0,    1, 1.01, PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB, NULL, "env2res", "Env->Res" },
    { 1,          0,    1, 1.01, PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB, NULL, "env2amp", "Env->Amp" },
    
    { 1,          1,20000, 1.01, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "adsr_a", "Attack" },
    { 350,       10,20000, 1.01, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "adsr_d", "Decay" },
    { 0.5,        0,    1, 1.01, PF_FLOAT | PF_SCALE_PERC, NULL, "adsr_s", "Sustain" },
    { 50,       10,20000, 1.01, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "adsr_r", "Release" },
    
    { 0,          0,    1, 1.01, PF_BOOL | PF_CTL_TOGGLE, NULL, "key_follow", "Key Follow" },
    { 0,          0,    3, 1.01, PF_ENUM | PF_CTL_COMBO, monosynth_legato_names, "legato", "Legato Mode" },
    { 1,          1, 2000, 1.01, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "portamento", "Portamento" },
    
    { 0.5,        0,    1,  0.1, PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB, NULL, "vel2filter", "Vel->Filter" },
    { 0,          0,    1,  0.1, PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB, NULL, "vel2amp", "Vel->Amp" },
};

const char *monosynth_audio_module::get_gui_xml()
{
    return monosynth_gui_xml;
}

void monosynth_audio_module::activate() {
    running = false;
    output_pos = 0;
    queue_note_on = -1;
    pitchbend = 1.f;
    filter.reset();
    filter2.reset();
    float data[2048];
    bandlimiter<11> bl;

    // yes these waves don't have really perfect 1/x spectrum because of aliasing
    // (so what?)
    for (int i = 0 ; i < 1024; i++)
        data[i] = (float)(i / 1024.f),
        data[i + 1024] = (float)(i / 1024.f - 1.0f);
    waves[wave_saw].make(bl, data);

    for (int i = 0 ; i < 2048; i++)
        data[i] = (float)(i < 1024 ? -1.f : 1.f);
    waves[wave_sqr].make(bl, data);

    for (int i = 0 ; i < 2048; i++)
        data[i] = (float)(i < 64 ? -1.f : 1.f);
    waves[wave_pulse].make(bl, data);

    // XXXKF sure this is a waste of space, this will be fixed some day by better bandlimiter
    for (int i = 0 ; i < 2048; i++)
        data[i] = (float)sin(i * PI / 1024);
    waves[wave_sine].make(bl, data);

    for (int i = 0 ; i < 512; i++) {
        data[i] = i / 512.0,
        data[i + 512] = 1 - i / 512.0,
        data[i + 1024] = - i / 512.0,
        data[i + 1536] = -1 + i / 512.0;
    }
    waves[wave_triangle].make(bl, data);
    
    for (int i = 0, j = 1; i < 2048; i++) {
        data[i] = -1 + j / 1024.0;
        if (i == j)
            j *= 2;
    }
    waves[wave_varistep].make(bl, data);

    for (int i = 0; i < 2048; i++) {
        int ii = (i < 1024) ? i : 2048 - i;
        
        data[i] = (min(1.f, (float)(i / 64.f))) * (1.0 - i / 2048.0) * (-1 + fmod (i * ii / 65536.0, 2.0));
    }
    waves[wave_skewsaw].make(bl, data);
    for (int i = 0; i < 2048; i++) {
        int ii = (i < 1024) ? i : 2048 - i;
        data[i] = (min(1.f, (float)(i / 64.f))) * (1.0 - i / 2048.0) * (fmod (i * ii / 32768.0, 2.0) < 1.0 ? -1.0 : +1.0);
    }
    waves[wave_skewsqr].make(bl, data);

    for (int i = 0; i < 1024; i++) {
        data[i] = exp(-i / 1024.0);
        data[i + 1024] = -exp(-i / 1024.0);
    }
    waves[wave_test1].make(bl, data);
    for (int i = 0; i < 2048; i++) {
        data[i] = exp(-i / 1024.0) * sin(i * PI / 1024) * cos(2 * PI * i / 1024);
    }
    waves[wave_test2].make(bl, data);
    for (int i = 0; i < 2048; i++) {
        int ii = (i < 1024) ? i : 2048 - i;
        data[i] = (ii / 1024.0) * sin(i * 15 * PI / 1024 + 2 * PI * sin(i * 18 * PI / 1024)) * sin(i * 12 * PI / 1024 + 2 * PI * sin(i * 11 * PI / 1024));
    }
    waves[wave_test3].make(bl, data);
    for (int i = 0; i < 2048; i++) {
        data[i] = sin(i * 2 * PI / 1024) * sin(i * 2 * PI / 1024 + 0.5 * PI * sin(i * 18 * PI / 1024)) * sin(i * 1 * PI / 1024 + 0.5 * PI * sin(i * 11 * PI / 1024));
    }
    waves[wave_test4].make(bl, data);
    for (int i = 0; i < 2048; i++) {
        data[i] = sin(i * 2 * PI / 1024 + 0.2 * PI * sin(i * 13 * PI / 1024)) * sin(i * PI / 1024 + 0.2 * PI * sin(i * 15 * PI / 1024));
    }
    waves[wave_test5].make(bl, data);
    waves[wave_test6].make(bl, data);
    waves[wave_test7].make(bl, data);
    waves[wave_test8].make(bl, data);
}

bool monosynth_audio_module::get_graph(int index, int subindex, float *data, int points, cairo_t *context)
{
    // printf("get_graph %d %p %d wave1=%d wave2=%d\n", index, data, points, wave1, wave2);
    if (index == par_wave1 || index == par_wave2) {
        if (subindex)
            return false;
        int wave = dsp::clip(dsp::fastf2i_drm(*params[index]), 0, (int)wave_count - 1);

        float *waveform = waves[wave].get_level(0);
        for (int i = 0; i < points; i++)
            data[i] = 0.5 * (waveform[i * 2047 / points] + waveform[i * 2047 / points + 1]);
        return true;
    }
    if (index == par_filtertype) {
        if (!running)
            return false;
        if (subindex > (is_stereo_filter() ? 1 : 0))
            return false;
        for (int i = 0; i < points; i++)
        {
            typedef complex<double> cfloat;
            double freq = 20.0 * pow (20000.0 / 20.0, i * 1.0 / points) * PI / srate;
            cfloat z = 1.0 / exp(cfloat(0.0, freq));
            
            biquad<float> &f = subindex ? filter2 : filter;
            float level = abs((cfloat(f.a0) + double(f.a1) * z + double(f.a2) * z*z) / (cfloat(1.0) + double(f.b1) * z + double(f.b2) * z*z));
            if (!is_stereo_filter())
                level *= abs((cfloat(filter2.a0) + double(filter2.a1) * z + double(filter2.a2) * z*z) / (cfloat(1.0) + double(filter2.b1) * z + double(filter2.b2) * z*z));
            level *= fgain;
            
            data[i] = log(level) / log(1024.0) + 0.25;
        }
        return true;
    }
    return false;
}

void monosynth_audio_module::calculate_buffer_ser()
{
    for (uint32_t i = 0; i < step_size; i++) 
    {
        float osc1val = osc1.get();
        float osc2val = osc2.get();
        float wave = fgain * (osc1val + (osc2val - osc1val) * xfade);
        wave = filter.process_d1(wave);
        wave = filter2.process_d1(wave);
        buffer[i] = softclip(wave);
        fgain += fgain_delta;
    }
}

void monosynth_audio_module::calculate_buffer_single()
{
    for (uint32_t i = 0; i < step_size; i++) 
    {
        float osc1val = osc1.get();
        float osc2val = osc2.get();
        float wave = fgain * (osc1val + (osc2val - osc1val) * xfade);
        wave = filter.process_d1(wave);
        buffer[i] = softclip(wave);
        fgain += fgain_delta;
    }
}

void monosynth_audio_module::calculate_buffer_stereo()
{
    for (uint32_t i = 0; i < step_size; i++) 
    {
        float osc1val = osc1.get();
        float osc2val = osc2.get();
        float wave1 = osc1val + (osc2val - osc1val) * xfade;
        float wave2 = phaseshifter.process_ap(wave1);
        buffer[i] = softclip(fgain * filter.process_d1(wave1));
        buffer2[i] = softclip(fgain * filter2.process_d1(wave2));
        fgain += fgain_delta;
    }
}

void monosynth_audio_module::delayed_note_on()
{
    porta_time = 0.f;
    start_freq = freq;
    target_freq = freq = 440 * pow(2.0, (queue_note_on - 69) / 12.0);
    ampctl = 1.0 + (queue_vel - 1.0) * *params[par_vel2amp];
    fltctl = 1.0 + (queue_vel - 1.0) * *params[par_vel2filter];
    set_frequency();
    osc1.waveform = waves[wave1].get_level(osc1.phasedelta);
    osc2.waveform = waves[wave2].get_level(osc2.phasedelta);
    
    if (!running)
    {
        if (legato >= 2)
            porta_time = -1.f;
        osc1.reset();
        osc2.reset();
        filter.reset();
        filter2.reset();
        switch((int)*params[par_oscmode])
        {
        case 1:
            osc2.phase = 0x80000000;
            break;
        case 2:
            osc2.phase = 0x40000000;
            break;
        case 3:
            osc1.phase = osc2.phase = 0x40000000;
            break;
        case 4:
            osc1.phase = 0x40000000;
            osc2.phase = 0xC0000000;
            break;
        case 5:
            // rand() is crap, but I don't have any better RNG in Calf yet
            osc1.phase = rand() << 16;
            osc2.phase = rand() << 16;
            break;
        default:
            break;
        }
        envelope.note_on();
        running = true;
    }
    if (legato >= 2 && !gate)
        porta_time = -1.f;
    gate = true;
    stopping = false;
    if (!(legato & 1) || envelope.released()) {
        envelope.note_on();
    }
    envelope.advance();
    queue_note_on = -1;
}

void monosynth_audio_module::set_sample_rate(uint32_t sr) {
    srate = sr;
    crate = sr / step_size;
    odcr = (float)(1.0 / crate);
    phaseshifter.set_ap(1000.f, sr);
    fgain = 0.f;
    fgain_delta = 0.f;
}

void monosynth_audio_module::calculate_step()
{
    if (queue_note_on != -1)
        delayed_note_on();
    else if (stopping)
    {
        running = false;
        dsp::zero(buffer, step_size);
        if (is_stereo_filter())
            dsp::zero(buffer2, step_size);
        return;
    }
    float porta_total_time = *params[par_portamento] * 0.001f;
    
    if (porta_total_time >= 0.00101f && porta_time >= 0) {
        // XXXKF this is criminal, optimize!
        float point = porta_time / porta_total_time;
        if (point >= 1.0f) {
            freq = target_freq;
            porta_time = -1;
        } else {
            freq = start_freq * pow(target_freq / start_freq, point);
            porta_time += odcr;
        }
    }
    set_frequency();
    envelope.advance();
    float env = envelope.value;
    cutoff = *params[par_cutoff] * pow(2.0f, env * fltctl * *params[par_envmod] * (1.f / 1200.f));
    if (*params[par_keyfollow] >= 0.5f)
        cutoff *= freq / 264.0f;
    cutoff = dsp::clip(cutoff , 10.f, 18000.f);
    float resonance = *params[par_resonance];
    float e2r = *params[par_envtores];
    float e2a = *params[par_envtoamp];
    resonance = resonance * (1 - e2r) + (0.7 + (resonance - 0.7) * env * env) * e2r;
    float cutoff2 = dsp::clip(cutoff * separation, 10.f, 18000.f);
    float newfgain = 0.f;
    switch(filter_type)
    {
    case flt_lp12:
        filter.set_lp_rbj(cutoff, resonance, srate);
        filter2.set_null();
        newfgain = min(0.7f, 0.7f / resonance) * ampctl;
        break;
    case flt_hp12:
        filter.set_hp_rbj(cutoff, resonance, srate);
        filter2.set_null();
        newfgain = min(0.7f, 0.7f / resonance) * ampctl;
        break;
    case flt_lp24:
        filter.set_lp_rbj(cutoff, resonance, srate);
        filter2.set_lp_rbj(cutoff2, resonance, srate);
        newfgain = min(0.5f, 0.5f / resonance) * ampctl;
        break;
    case flt_lpbr:
        filter.set_lp_rbj(cutoff, resonance, srate);
        filter2.set_br_rbj(cutoff2, 1.0 / resonance, srate);
        newfgain = min(0.5f, 0.5f / resonance) * ampctl;        
        break;
    case flt_hpbr:
        filter.set_hp_rbj(cutoff, resonance, srate);
        filter2.set_br_rbj(cutoff2, 1.0 / resonance, srate);
        newfgain = min(0.5f, 0.5f / resonance) * ampctl;        
        break;
    case flt_2lp12:
        filter.set_lp_rbj(cutoff, resonance, srate);
        filter2.set_lp_rbj(cutoff2, resonance, srate);
        newfgain = min(0.7f, 0.7f / resonance) * ampctl;
        break;
    case flt_bp6:
        filter.set_bp_rbj(cutoff, resonance, srate);
        filter2.set_null();
        newfgain = ampctl;
        break;
    case flt_2bp6:
        filter.set_bp_rbj(cutoff, resonance, srate);
        filter2.set_bp_rbj(cutoff2, resonance, srate);
        newfgain = ampctl;        
        break;
    }
    newfgain *= 1.0 - (1.0 - env) * e2a;
    fgain_delta = (newfgain - fgain) * (1.0 / step_size);
    switch(filter_type)
    {
    case flt_lp24:
    case flt_lpbr:
    case flt_hpbr: // Oomek's wish
        calculate_buffer_ser();
        break;
    case flt_lp12:
    case flt_hp12:
    case flt_bp6:
        calculate_buffer_single();
        break;
    case flt_2lp12:
    case flt_2bp6:
        calculate_buffer_stereo();
        break;
    }
    if (envelope.state == adsr::STOP)
    {
        for (int i = 0; i < step_size; i++)
            buffer[i] *= (step_size - i) * (1.0f / step_size);
        if (is_stereo_filter())
            for (int i = 0; i < step_size; i++)
                buffer2[i] *= (step_size - i) * (1.0f / step_size);
        stopping = true;
    }
}
