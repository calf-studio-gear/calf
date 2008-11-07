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
#include <calf/modules_synths.h>

using namespace dsp;
using namespace calf_plugins;
using namespace std;

float silence[4097];

void monosynth_audio_module::activate() {
    monosynth_audio_module::generate_waves();
    running = false;
    output_pos = 0;
    queue_note_on = -1;
    stop_count = 0;
    pitchbend = 1.f;
    filter.reset();
    filter2.reset();
    stack.clear();
}

waveform_family<MONOSYNTH_WAVE_BITS> *monosynth_audio_module::waves;

void monosynth_audio_module::generate_waves()
{
    float data[1 << MONOSYNTH_WAVE_BITS];
    bandlimiter<MONOSYNTH_WAVE_BITS> bl;
    
    if (waves)
        return;
    
    static waveform_family<MONOSYNTH_WAVE_BITS> waves_data[wave_count];
    waves = waves_data;
    
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
        data[i] = (float)sin(i * M_PI / HS);
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
            data[i] = sin(M_PI * p * p * p);
        } else {
            float p = (i - QS3 * 1.0) / QS;
            data[i] = -0.5 * sin(3 * M_PI * p * p);
        }
    }
    waves[wave_test1].make(bl, data);
    for (int i = 0; i < S; i++) {
        data[i] = exp(-i * 1.0 / HS) * sin(i * M_PI / HS) * cos(2 * M_PI * i / HS);
    }
    normalize_waveform(data, S);
    waves[wave_test2].make(bl, data);
    for (int i = 0; i < S; i++) {
        //int ii = (i < HS) ? i : S - i;
        int ii = HS;
        data[i] = (ii * 1.0 / HS) * sin(i * 3 * M_PI / HS + 2 * M_PI * sin(M_PI / 4 + i * 4 * M_PI / HS)) * sin(i * 5 * M_PI / HS + 2 * M_PI * sin(M_PI / 8 + i * 6 * M_PI / HS));
    }
    waves[wave_test3].make(bl, data);
    for (int i = 0; i < S; i++) {
        data[i] = sin(i * 2 * M_PI / HS + sin(i * 2 * M_PI / HS + 0.5 * M_PI * sin(i * 18 * M_PI / HS)) * sin(i * 1 * M_PI / HS + 0.5 * M_PI * sin(i * 11 * M_PI / HS)));
    }
    waves[wave_test4].make(bl, data);
    for (int i = 0; i < S; i++) {
        data[i] = sin(i * 2 * M_PI / HS + 0.2 * M_PI * sin(i * 13 * M_PI / HS) + 0.1 * M_PI * sin(i * 37 * M_PI / HS)) * sin(i * M_PI / HS + 0.2 * M_PI * sin(i * 15 * M_PI / HS));
    }
    waves[wave_test5].make(bl, data);
    for (int i = 0; i < S; i++) {
        if (i < HS)
            data[i] = sin(i * 2 * M_PI / HS);
        else
        if (i < 3 * S / 4)
            data[i] = sin(i * 4 * M_PI / HS);
        else
        if (i < 7 * S / 8)
            data[i] = sin(i * 8 * M_PI / HS);
        else
            data[i] = sin(i * 8 * M_PI / HS) * (S - i) / (S / 8);
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

bool monosynth_audio_module::get_static_graph(int index, int subindex, float value, float *data, int points, cairo_t *context)
{
    monosynth_audio_module::generate_waves();
    if (index == par_wave1 || index == par_wave2) {
        if (subindex)
            return false;
        enum { S = 1 << MONOSYNTH_WAVE_BITS };
        int wave = dsp::clip(dsp::fastf2i_drm(value), 0, (int)wave_count - 1);

        float *waveform = waves[wave].original;
        for (int i = 0; i < points; i++)
            data[i] = waveform[i * S / points];
        return true;
    }
    return false;
}

bool monosynth_audio_module::get_graph(int index, int subindex, float *data, int points, cairo_t *context)
{
    monosynth_audio_module::generate_waves();
    // printf("get_graph %d %p %d wave1=%d wave2=%d\n", index, data, points, wave1, wave2);
    if (index == par_filtertype) {
        if (!running)
            return false;
        if (subindex > (is_stereo_filter() ? 1 : 0))
            return false;
        for (int i = 0; i < points; i++)
        {
            typedef complex<double> cfloat;
            double freq = 20.0 * pow (20000.0 / 20.0, i * 1.0 / points);
            
            dsp::biquad_d1<float> &f = subindex ? filter2 : filter;
            float level = f.freq_gain(freq, srate);
            if (!is_stereo_filter())
                level *= filter2.freq_gain(freq, srate);
            level *= fgain;
            
            data[i] = log(level) / log(1024.0) + 0.5;
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
        wave = filter.process(wave);
        wave = filter2.process(wave);
        buffer[i] = wave;
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
        wave = filter.process(wave);
        buffer[i] = wave;
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
        buffer[i] = fgain * filter.process(wave1);
        buffer2[i] = fgain * filter2.process(wave2);
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
    if (filter_type != last_filter_type)
    {
        filter.y2 = filter.y1 = filter.x2 = filter.x1 = filter.y1;
        filter2.y2 = filter2.y1 = filter2.x2 = filter2.x1 = filter2.y1;
        last_filter_type = filter_type;
    }
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

