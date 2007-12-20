/* Calf DSP Library
 * Audio modules - synthesizers
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
#ifndef __CALF_MODULES_SYNTHS_H
#define __CALF_MODULES_SYNTHS_H

#include <assert.h>
#include "biquad.h"
#include "audio_fx.h"
#include "inertia.h"
#include "osc.h"
#include "synth.h"

namespace synth {

/// Monosynth-in-making. Parameters may change at any point, so don't make songs with it!
/// It lacks inertia for parameters, even for those that really need it.
class monosynth_audio_module: public null_audio_module
{
public:
    enum { wave_saw, wave_sqr, wave_pulse, wave_sine, wave_triangle, wave_count };
    enum { flt_lp12, flt_lp24, flt_2lp12, flt_hp12, flt_lpbr, flt_hpbr, flt_bp6, flt_2bp6 };
    enum { par_wave1, par_wave2, par_detune, par_osc2xpose, par_oscmode, par_oscmix, par_filtertype, par_cutoff, par_resonance, par_cutoffsep, par_envmod, par_envtores, par_attack, par_decay, par_sustain, par_release, par_keyfollow, par_legato, par_portamento, par_vel2amp, par_vel2filter, param_count };
    enum { in_count = 0, out_count = 2, support_midi = true, rt_capable = true };
    enum { step_size = 64 };
    static const char *port_names[];
    float *ins[in_count]; 
    float *outs[out_count];
    float *params[param_count];
    uint32_t srate, crate;
    waveform_family<11> waves[wave_count];
    waveform_oscillator<11> osc1, osc2;
    bool running, stopping, gate;
    int last_key;
    
    float buffer[step_size], buffer2[step_size];
    uint32_t output_pos;
    biquad<float> filter;
    biquad<float> filter2;
    int wave1, wave2, filter_type;
    float freq, start_freq, target_freq, cutoff, decay_factor, fgain, separation;
    float detune, xpose, xfade, pitchbend, ampctl, fltctl, queue_vel;
    float odcr, porta_time;
    int queue_note_on;
    int legato;
    synth::adsr envelope;
    
    static parameter_properties param_props[];
    void set_sample_rate(uint32_t sr) {
        srate = sr;
        crate = sr / step_size;
        odcr = (float)(1.0 / crate);
    }
    void delayed_note_on()
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
        queue_note_on = -1;
    }
    void note_on(int note, int vel)
    {
        queue_note_on = note;
        last_key = note;
        queue_vel = vel / 127.f;
    }
    void note_off(int note, int vel)
    {
        if (note == last_key) {
            gate = false;
            envelope.note_off();
        }
    }
    void pitch_bend(int value)
    {
        pitchbend = pow(2.0, value / 8192.0);
    }
    void set_frequency()
    {
        osc1.set_freq(freq * (2 - detune) * pitchbend, srate);
        osc2.set_freq(freq * (detune)  * pitchbend * xpose, srate);
    }
    void params_changed() {
        float sf = 0.001f;
        envelope.set(*params[par_attack] * sf, *params[par_decay] * sf, *params[par_sustain], *params[par_release] * sf, srate / step_size);
        filter_type = fastf2i_drm(*params[par_filtertype]);
        decay_factor = odcr * 1000.0 / *params[par_decay];
        separation = pow(2.0, *params[par_cutoffsep] / 1200.0);
        wave1 = dsp::clip(dsp::fastf2i_drm(*params[par_wave1]), 0, (int)wave_count - 1);
        wave2 = dsp::clip(dsp::fastf2i_drm(*params[par_wave2]), 0, (int)wave_count - 1);
        detune = pow(2.0, *params[par_detune] / 1200.0);
        xpose = pow(2.0, *params[par_osc2xpose] / 12.0);
        xfade = *params[par_oscmix];
        legato = dsp::fastf2i_drm(*params[par_legato]);
        set_frequency();
    }
    void activate() {
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
    }
    void deactivate() {
    }
    inline float softclip(float wave) const
    {
        float abswave = fabs(wave);
        if (abswave > 0.75)
        {
            abswave = abswave - 0.5 * (abswave - 0.75);
            if (abswave > 1.0)
                abswave = 1.0;
            wave = (wave > 0.0) ? abswave : - abswave;
        }
        return wave;
    }
    void calculate_buffer_ser()
    {
        for (uint32_t i = 0; i < step_size; i++) 
        {
            float osc1val = osc1.get();
            float osc2val = osc2.get();
            float wave = fgain * (osc1val + (osc2val - osc1val) * xfade);
            wave = filter.process_d1(wave);
            wave = filter2.process_d1(wave);
            buffer[i] = softclip(wave);
        }
    }
    void calculate_buffer_single()
    {
        for (uint32_t i = 0; i < step_size; i++) 
        {
            float osc1val = osc1.get();
            float osc2val = osc2.get();
            float wave = fgain * (osc1val + (osc2val - osc1val) * xfade);
            wave = filter.process_d1(wave);
            buffer[i] = softclip(wave);
        }
    }
    void calculate_buffer_stereo()
    {
        for (uint32_t i = 0; i < step_size; i++) 
        {
            float osc1val = osc1.get();
            float osc2val = osc2.get();
            float wave1 = osc1val + (osc2val - osc1val) * xfade;
            float wave2 = osc1val + ((-osc2val) - osc1val) * xfade;
            buffer[i] = softclip(fgain * filter.process_d1(wave1));
            buffer2[i] = softclip(fgain * filter2.process_d1(wave2));
        }
    }
    bool is_stereo_filter() const
    {
        return filter_type == flt_2lp12 || filter_type == flt_2bp6;
    }
    void calculate_step() {
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
        resonance = resonance * (1 - e2r) + (0.7 + (resonance - 0.7) * env * env) * e2r;
        float cutoff2 = dsp::clip(cutoff * separation, 10.f, 18000.f);
        switch(filter_type)
        {
        case flt_lp12:
            filter.set_lp_rbj(cutoff, resonance, srate);
            fgain = min(0.7f, 0.7f / resonance) * ampctl;
            break;
        case flt_hp12:
            filter.set_hp_rbj(cutoff, resonance, srate);
            fgain = min(0.7f, 0.7f / resonance) * ampctl;
            break;
        case flt_lp24:
            filter.set_lp_rbj(cutoff, resonance, srate);
            filter2.set_lp_rbj(cutoff2, resonance, srate);
            fgain = min(0.5f, 0.5f / resonance) * ampctl;
            break;
        case flt_lpbr:
            filter.set_lp_rbj(cutoff, resonance, srate);
            filter2.set_br_rbj(cutoff2, resonance, srate);
            fgain = min(0.5f, 0.5f / resonance) * ampctl;        
            break;
        case flt_hpbr:
            filter.set_hp_rbj(cutoff, resonance, srate);
            filter2.set_br_rbj(cutoff2, resonance, srate);
            fgain = min(0.5f, 0.5f / resonance) * ampctl;        
            break;
        case flt_2lp12:
            filter.set_lp_rbj(cutoff, resonance, srate);
            filter2.set_lp_rbj(cutoff2, resonance, srate);
            fgain = min(0.7f, 0.7f / resonance) * ampctl;
            break;
        case flt_bp6:
            filter.set_bp_rbj(cutoff, resonance, srate);
            fgain = ampctl;
            break;
        case flt_2bp6:
            filter.set_bp_rbj(cutoff, resonance, srate);
            filter2.set_bp_rbj(cutoff2, resonance, srate);
            fgain = ampctl;        
            break;
        }
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
    uint32_t process(uint32_t offset, uint32_t nsamples, uint32_t inputs_mask, uint32_t outputs_mask) {
        if (!running && queue_note_on == -1)
            return 0;
        uint32_t op = offset;
        uint32_t op_end = offset + nsamples;
        while(op < op_end) {
            if (output_pos == 0) {
                if (running || queue_note_on != -1)
                    calculate_step();
                else 
                    dsp::zero(buffer, step_size);
            }
            if(op < op_end) {
                uint32_t ip = output_pos;
                uint32_t len = std::min(step_size - output_pos, op_end - op);
                if (is_stereo_filter())
                    for(uint32_t i = 0 ; i < len; i++)
                        outs[0][op + i] = buffer[ip + i],
                        outs[1][op + i] = buffer2[ip + i];
                else
                    for(uint32_t i = 0 ; i < len; i++)
                        outs[0][op + i] = outs[1][op + i] = buffer[ip + i];
                op += len;
                output_pos += len;
                if (output_pos == step_size)
                    output_pos = 0;
            }
        }
            
        return 3;
    }
};

};

#endif
