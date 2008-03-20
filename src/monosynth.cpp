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
#include <config.h>
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
    "Smooth Brass", "Bass", "Dark FM", "Multiwave", "Bell FM", "Dark Pad", "DCO Saw", "DCO Maze" };
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
//                    "<line-graph param=\"o1_wave\"/>"
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
                        "  <align><knob param=\"o12_detune\" expand=\"0\" fill=\"0\"/></align><value param=\"o12_detune\"/>"
                        "</vbox>"
                        "<align>"
                        "  <vbox>"
                        "    <label param=\"phase_mode\" />"
                        "    <combo param=\"phase_mode\"/>"
                        "  </vbox>"
                        "</align>" 
                        "<vbox>"
                        "  <label param=\"o2_xpose\" />"
                        "  <align><knob type=\"1\" param=\"o2_xpose\" expand=\"0\" fill=\"0\"/></align><value param=\"o2_xpose\"/>"
                        "</vbox>"
                    "</hbox>"
                "</vbox>"
            "</frame>"
            "<frame label=\"Filter\">"
                "<vbox border=\"10\">"
                    "<align>"
                        "<hbox>"
                            "<label param=\"filter\" /><combo param=\"filter\" />"
                            "<if cond=\"directlink\">"
                                "<line-graph param=\"filter\" refresh=\"1\" width=\"80\" height=\"60\"/>"
                            "</if>"
                        "</hbox>"
                    "</align>"
                    "<hbox>"
                        "<vbox>"
                        "  <label param=\"cutoff\" />"
                        "  <align><knob param=\"cutoff\" expand=\"0\" fill=\"0\"/></align><value param=\"cutoff\"/>"
                        "</vbox>"
                        "<vbox>"
                        "  <label param=\"res\" />"
                        "  <align><knob param=\"res\" expand=\"0\" fill=\"0\"/></align><value param=\"res\"/>"
                        "</vbox>"
                        "<vbox>"
                        "  <label param=\"filter_sep\" />"
                        "  <align><knob type=\"1\" param=\"filter_sep\" expand=\"0\" fill=\"0\"/></align><value param=\"filter_sep\"/>"
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
                        "  <align><knob param=\"adsr_a\" expand=\"0\" fill=\"0\"/></align><value param=\"adsr_a\"/>"
                        "</vbox>"
                        "<vbox>"
                        "  <label param=\"adsr_d\" />"
                        "  <align><knob param=\"adsr_d\" expand=\"0\" fill=\"0\"/></align><value param=\"adsr_d\"/>"
                        "</vbox>"
                        "<vbox>"
                        "  <label param=\"adsr_s\" />"
                        "  <align><knob param=\"adsr_s\" expand=\"0\" fill=\"0\"/></align><value param=\"adsr_s\"/>"
                        "</vbox>"
                        "<vbox>"
                        "  <label param=\"adsr_r\" />"
                        "  <align><knob param=\"adsr_r\" expand=\"0\" fill=\"0\"/></align><value param=\"adsr_r\"/>"
                        "</vbox>"
                    "</hbox>"
                    "<hbox>"
                        "<vbox>"
                        "  <label param=\"env2cutoff\" />"
                        "  <align><knob type=\"1\" param=\"env2cutoff\" expand=\"0\" fill=\"0\"/></align><value param=\"env2cutoff\"/>"
                        "</vbox>"
                        "<vbox>"
                        "  <label param=\"env2res\" />"
                        "  <align><knob param=\"env2res\" expand=\"0\" fill=\"0\"/></align><value param=\"env2res\"/>"
                        "</vbox>"
                        "<vbox>"
                        "  <label param=\"env2amp\" />"
                        "  <align><knob param=\"env2amp\" expand=\"0\" fill=\"0\"/></align><value param=\"env2amp\"/>"
                        "</vbox>"
                    "</hbox>"
                "</vbox>"
            "</frame>"
            "<frame label=\"Settings\">"
                "<vbox border=\"10\" spacing=\"10\">"
                    "<hbox>"
                        "<vbox>"
                            "<label param=\"key_follow\" />"
                            "<align><knob param=\"key_follow\" /></align>"
                            "<value param=\"key_follow\" />"
                        "</vbox>"
                        "<vbox>"
                            "<label param=\"legato\"  expand=\"0\"/>"
                            "<combo param=\"legato\" expand=\"0\" fill=\"0\"/>"
                        "</vbox>"
                        "<vbox>"
                            "<label param=\"master\" />"
                            "<align><knob param=\"master\" /></align>"
                            "<value param=\"master\" />"
                        "</vbox>"
                    "</hbox>"
                    "<hbox>"
                        "<vbox>"
                        "  <label param=\"portamento\" />"
                        "  <align><knob param=\"portamento\" /></align><value param=\"portamento\"/>"
                        "</vbox>"
                        "<vbox>"
                        "  <label param=\"vel2filter\" />"
                        "  <align><knob param=\"vel2filter\" /></align><value param=\"vel2filter\"/>"
                        "</vbox>"
                        "<vbox>"
                        "  <label param=\"vel2amp\" />"
                        "  <align><knob param=\"vel2amp\" /></align><value param=\"vel2amp\"/>"
                        "</vbox>"
                    "</hbox>"
                "</vbox>"
            "</frame>"
        "</hbox>"
    "</vbox>";

parameter_properties monosynth_audio_module::param_props[] = {
    { wave_saw,         0, wave_count - 1, 1, PF_ENUM | PF_CTL_COMBO, monosynth_waveform_names, "o1_wave", "Osc1 Wave" },
    { wave_sqr,         0, wave_count - 1, 1, PF_ENUM | PF_CTL_COMBO, monosynth_waveform_names, "o2_wave", "Osc2 Wave" },
    { 10,         0,  100,    0, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_CENTS, NULL, "o12_detune", "O1<>2 Detune" },
    { 12,       -24,   24,    0, PF_INT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_SEMITONES, NULL, "o2_xpose", "Osc 2 transpose" },
    { 0,          0,    5,    0, PF_ENUM | PF_CTL_COMBO, monosynth_mode_names, "phase_mode", "Phase mode" },
    { 0.5,        0,    1,    0, PF_FLOAT | PF_SCALE_PERC, NULL, "o12_mix", "O1<>2 Mix" },
    { 1,          0,    7,    0, PF_ENUM | PF_CTL_COMBO, monosynth_filter_choices, "filter", "Filter" },
    { 33,        10,16000,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_HZ, NULL, "cutoff", "Cutoff" },
    { 2,        0.7,    8,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB, NULL, "res", "Resonance" },
    { 0,      -2400, 2400,    0, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_CENTS, NULL, "filter_sep", "Separation" },
    { 8000,  -10800,10800,    0, PF_FLOAT | PF_SCALE_LINEAR | PF_CTL_KNOB | PF_UNIT_CENTS, NULL, "env2cutoff", "Env->Cutoff" },
    { 1,          0,    1,    0, PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB, NULL, "env2res", "Env->Res" },
    { 1,          0,    1,    0, PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB, NULL, "env2amp", "Env->Amp" },
    
    { 1,          1,20000,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "adsr_a", "Attack" },
    { 350,       10,20000,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "adsr_d", "Decay" },
    { 0.5,        0,    1,    0, PF_FLOAT | PF_SCALE_PERC, NULL, "adsr_s", "Sustain" },
    { 50,       10,20000,     0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "adsr_r", "Release" },
    
    { 0,          0,    2,    0, PF_FLOAT | PF_SCALE_PERC, NULL, "key_follow", "Key Follow" },
    { 0,          0,    3,    0, PF_ENUM | PF_CTL_COMBO, monosynth_legato_names, "legato", "Legato Mode" },
    { 1,          1, 2000,    0, PF_FLOAT | PF_SCALE_LOG | PF_CTL_KNOB | PF_UNIT_MSEC, NULL, "portamento", "Portamento" },
    
    { 0.5,        0,    1,  0.1, PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB, NULL, "vel2filter", "Vel->Filter" },
    { 0,          0,    1,  0.1, PF_FLOAT | PF_SCALE_PERC | PF_CTL_KNOB, NULL, "vel2amp", "Vel->Amp" },

    { 0.5,         0,   1, 100, PF_FLOAT | PF_SCALE_GAIN | PF_CTL_KNOB, NULL, "master", "Volume" },
};

float silence[4097];

const char *monosynth_audio_module::get_gui_xml()
{
    return monosynth_gui_xml;
}

void monosynth_audio_module::activate() {
    running = false;
    output_pos = 0;
    queue_note_on = -1;
    stop_count = 0;
    pitchbend = 1.f;
    filter.reset();
    filter2.reset();
    stack.clear();
}

waveform_family<MONOSYNTH_WAVE_BITS> monosynth_audio_module::waves[wave_count];

void monosynth_audio_module::generate_waves()
{
    float data[1 << MONOSYNTH_WAVE_BITS];
    bandlimiter<MONOSYNTH_WAVE_BITS> bl;
    
    enum { S = 1 << MONOSYNTH_WAVE_BITS, HS = S / 2, QS = S / 4, QS3 = 3 * QS };
    float iQS = 1.0 / QS;

    // yes these waves don't have really perfect 1/x spectrum because of aliasing
    // (so what?)
    for (int i = 0 ; i < HS; i++)
        data[i] = (float)(i * 1.0 / HS),
        data[i + HS] = (float)(i * 1.0 / HS - 1.0f);
    waves[wave_saw].make(bl, data);

    for (int i = 0 ; i < S; i++)
        data[i] = (float)(i < HS ? -1.f : 1.f);
    waves[wave_sqr].make(bl, data);

    for (int i = 0 ; i < S; i++)
        data[i] = (float)(i < (64 * S / 2048)? -1.f : 1.f);
    waves[wave_pulse].make(bl, data);

    // XXXKF sure this is a waste of space, this will be fixed some day by better bandlimiter
    for (int i = 0 ; i < S; i++)
        data[i] = (float)sin(i * PI / HS);
    waves[wave_sine].make(bl, data);

    for (int i = 0 ; i < QS; i++) {
        data[i] = i * iQS,
        data[i + QS] = 1 - i * iQS,
        data[i + HS] = - i * iQS,
        data[i + QS3] = -1 + i * iQS;
    }
    waves[wave_triangle].make(bl, data);
    
    for (int i = 0, j = 1; i < S; i++) {
        data[i] = -1 + j * 1.0 / HS;
        if (i == j)
            j *= 2;
    }
    waves[wave_varistep].make(bl, data);

    for (int i = 0; i < S; i++) {
        data[i] = (min(1.f, (float)(i / 64.f))) * (1.0 - i * 1.0 / S) * (-1 + fmod (i * i / 262144.0, 2.0));
    }
    waves[wave_skewsaw].make(bl, data);
    for (int i = 0; i < S; i++) {
        data[i] = (min(1.f, (float)(i / 64.f))) * (1.0 - i * 1.0 / S) * (fmod (i * i / 262144.0, 2.0) < 1.0 ? -1.0 : +1.0);
    }
    waves[wave_skewsqr].make(bl, data);

    for (int i = 0; i < S; i++) {
        if (i < QS3) {
            float p = i * 1.0 / QS3;
            data[i] = sin(PI * p * p * p);
        } else {
            float p = (i - QS3 * 1.0) / QS;
            data[i] = -0.5 * sin(3 * PI * p * p);
        }
    }
    waves[wave_test1].make(bl, data);
    for (int i = 0; i < S; i++) {
        data[i] = exp(-i * 1.0 / HS) * sin(i * PI / HS) * cos(2 * PI * i / HS);
    }
    normalize_waveform(data, S);
    waves[wave_test2].make(bl, data);
    for (int i = 0; i < S; i++) {
        //int ii = (i < HS) ? i : S - i;
        int ii = HS;
        data[i] = (ii * 1.0 / HS) * sin(i * 3 * PI / HS + 2 * PI * sin(PI / 4 + i * 4 * PI / HS)) * sin(i * 5 * PI / HS + 2 * PI * sin(PI / 8 + i * 6 * PI / HS));
    }
    waves[wave_test3].make(bl, data);
    for (int i = 0; i < S; i++) {
        data[i] = sin(i * 2 * PI / HS + sin(i * 2 * PI / HS + 0.5 * PI * sin(i * 18 * PI / HS)) * sin(i * 1 * PI / HS + 0.5 * PI * sin(i * 11 * PI / HS)));
    }
    waves[wave_test4].make(bl, data);
    for (int i = 0; i < S; i++) {
        data[i] = sin(i * 2 * PI / HS + 0.2 * PI * sin(i * 13 * PI / HS) + 0.1 * PI * sin(i * 37 * PI / HS)) * sin(i * PI / HS + 0.2 * PI * sin(i * 15 * PI / HS));
    }
    waves[wave_test5].make(bl, data);
    for (int i = 0; i < S; i++) {
        if (i < HS)
            data[i] = sin(i * 2 * PI / HS);
        else
        if (i < 3 * S / 4)
            data[i] = sin(i * 4 * PI / HS);
        else
        if (i < 7 * S / 8)
            data[i] = sin(i * 8 * PI / HS);
        else
            data[i] = sin(i * 8 * PI / HS) * (S - i) / (S / 8);
    }
    waves[wave_test6].make(bl, data);
    for (int i = 0; i < S; i++) {
        int j = i >> (MONOSYNTH_WAVE_BITS - 11);
        data[i] = (j ^ 0x1D0) * 1.0 / HS - 1;
    }
    waves[wave_test7].make(bl, data);
    for (int i = 0; i < S; i++) {
        int j = i >> (MONOSYNTH_WAVE_BITS - 11);
        data[i] = -1 + 0.66 * (3 & ((j >> 8) ^ (j >> 10) ^ (j >> 6)));
    }
    waves[wave_test8].make(bl, data);
}

void __attribute__((constructor)) generate_monosynth_waves() 
{
    monosynth_audio_module::generate_waves();
}

bool monosynth_audio_module::get_static_graph(int index, int subindex, float value, float *data, int points, cairo_t *context)
{
    if (index == par_wave1 || index == par_wave2) {
        if (subindex)
            return false;
        enum { S = 1 << MONOSYNTH_WAVE_BITS };
        int wave = dsp::clip(dsp::fastf2i_drm(value), 0, (int)wave_count - 1);

        float *waveform = waves[wave].get_level(0);
        for (int i = 0; i < points; i++)
            data[i] = waveform[i * S / points];
        return true;
    }
    return false;
}

bool monosynth_audio_module::get_graph(int index, int subindex, float *data, int points, cairo_t *context)
{
    // printf("get_graph %d %p %d wave1=%d wave2=%d\n", index, data, points, wave1, wave2);
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
    return get_static_graph(index, subindex, *params[index], data, points, context);
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
    stop_count = 0;
    porta_time = 0.f;
    start_freq = freq;
    target_freq = freq = 440 * pow(2.0, (queue_note_on - 69) / 12.0);
    ampctl = 1.0 + (queue_vel - 1.0) * *params[par_vel2amp];
    fltctl = 1.0 + (queue_vel - 1.0) * *params[par_vel2filter];
    set_frequency();
    osc1.waveform = waves[wave1].get_level(osc1.phasedelta);
    osc2.waveform = waves[wave2].get_level(osc2.phasedelta);
    if (!osc1.waveform) osc1.waveform = silence;
    if (!osc2.waveform) osc2.waveform = silence;
    
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
            freq = start_freq + (target_freq - start_freq) * point;
            // freq = start_freq * pow(target_freq / start_freq, point);
            porta_time += odcr;
        }
    }
    set_frequency();
    envelope.advance();
    float env = envelope.value;
    cutoff = *params[par_cutoff] * pow(2.0f, env * fltctl * *params[par_envmod] * (1.f / 1200.f));
    if (*params[par_keyfollow] > 0.01f)
        cutoff *= pow(freq / 264.f, *params[par_keyfollow]);
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
    float aenv = env;
    /* isn't as good as expected
    if (e2a > 1.0) { // extra-steep release on amplitude envelope only
        if (envelope.state == adsr::RELEASE && env < envelope.sustain) {
            aenv -= (envelope.sustain - env) * (e2a - 1.0);
            if (aenv < 0.f) aenv = 0.f;
            printf("aenv = %f\n", aenv);
        }
        e2a = 1.0;
    }
    */
    newfgain *= 1.0 - (1.0 - aenv) * e2a;
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
        enum { ramp = step_size * 4 };
        for (int i = 0; i < step_size; i++)
            buffer[i] *= (ramp - i - stop_count) * (1.0f / ramp);
        if (is_stereo_filter())
            for (int i = 0; i < step_size; i++)
                buffer2[i] *= (ramp - i - stop_count) * (1.0f / ramp);
        stop_count += step_size;
        if (stop_count >= ramp)
            stopping = true;
    }
}

void monosynth_audio_module::control_change(int controller, int value)
{
    switch(controller)
    {
        case 120: // all sounds off
        case 123: // all notes off
            gate = false;
            envelope.note_off();
            stack.clear();
            break;
    }
}

