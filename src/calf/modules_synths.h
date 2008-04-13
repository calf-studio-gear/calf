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
#include "organ.h"

namespace synth {

#define MONOSYNTH_WAVE_BITS 12
    
/// Monosynth-in-making. Parameters may change at any point, so don't make songs with it!
/// It lacks inertia for parameters, even for those that really need it.
class monosynth_audio_module: public null_audio_module
{
public:
    enum { wave_saw, wave_sqr, wave_pulse, wave_sine, wave_triangle, wave_varistep, wave_skewsaw, wave_skewsqr, wave_test1, wave_test2, wave_test3, wave_test4, wave_test5, wave_test6, wave_test7, wave_test8, wave_count };
    enum { flt_lp12, flt_lp24, flt_2lp12, flt_hp12, flt_lpbr, flt_hpbr, flt_bp6, flt_2bp6 };
    enum { par_wave1, par_wave2, par_detune, par_osc2xpose, par_oscmode, par_oscmix, par_filtertype, par_cutoff, par_resonance, par_cutoffsep, par_envmod, par_envtores, par_envtoamp, par_attack, par_decay, par_sustain, par_release, par_keyfollow, par_legato, par_portamento, par_vel2filter, par_vel2amp, par_master, param_count };
    enum { in_count = 0, out_count = 2, support_midi = true, rt_capable = true };
    enum { step_size = 64 };
    static const char *port_names[];
    float *ins[in_count]; 
    float *outs[out_count];
    float *params[param_count];
    uint32_t srate, crate;
    static waveform_family<MONOSYNTH_WAVE_BITS> *waves;
    waveform_oscillator<MONOSYNTH_WAVE_BITS> osc1, osc2;
    bool running, stopping, gate;
    int last_key;
    
    float buffer[step_size], buffer2[step_size];
    uint32_t output_pos;
    onepole<float> phaseshifter;
    biquad<float> filter;
    biquad<float> filter2;
    int wave1, wave2, filter_type;
    float freq, start_freq, target_freq, cutoff, decay_factor, fgain, fgain_delta, separation;
    float detune, xpose, xfade, pitchbend, ampctl, fltctl, queue_vel;
    float odcr, porta_time;
    int queue_note_on, stop_count;
    int legato;
    synth::adsr envelope;
    synth::keystack stack;
    gain_smoothing master;
    
    static parameter_properties param_props[];
    static const char *get_gui_xml();
    static void generate_waves();
    void set_sample_rate(uint32_t sr);
    void delayed_note_on();
    void note_on(int note, int vel)
    {
        queue_note_on = note;
        last_key = note;
        queue_vel = vel / 127.f;
        stack.push(note);
    }
    void note_off(int note, int vel)
    {
        stack.pop(note);
        if (note == last_key) {
            if (stack.count())
            {
                last_key = note = stack.nth(stack.count() - 1);
                start_freq = freq;
                target_freq = freq = 440 * pow(2.0, (note - 69) / 12.0);
                set_frequency();
                if (!(legato & 1)) {
                    envelope.note_on();
                    stopping = false;
                    running = true;
                }
                return;
            }
            gate = false;
            envelope.note_off();
        }
    }
    inline void pitch_bend(int value)
    {
        pitchbend = pow(2.0, value / 8192.0);
    }
    inline void set_frequency()
    {
        osc1.set_freq(freq * (2 - detune) * pitchbend, srate);
        osc2.set_freq(freq * (detune)  * pitchbend * xpose, srate);
    }
    void control_change(int controller, int value);
    void params_changed() {
        float sf = 0.001f;
        envelope.set(*params[par_attack] * sf, *params[par_decay] * sf, min(0.999f, *params[par_sustain]), *params[par_release] * sf, srate / step_size);
        filter_type = fastf2i_drm(*params[par_filtertype]);
        decay_factor = odcr * 1000.0 / *params[par_decay];
        separation = pow(2.0, *params[par_cutoffsep] / 1200.0);
        wave1 = dsp::clip(dsp::fastf2i_drm(*params[par_wave1]), 0, (int)wave_count - 1);
        wave2 = dsp::clip(dsp::fastf2i_drm(*params[par_wave2]), 0, (int)wave_count - 1);
        detune = pow(2.0, *params[par_detune] / 1200.0);
        xpose = pow(2.0, *params[par_osc2xpose] / 12.0);
        xfade = *params[par_oscmix];
        legato = dsp::fastf2i_drm(*params[par_legato]);
        master.set_inertia(*params[par_master]);
        set_frequency();
    }
    void activate();
    void deactivate() {}
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
    void calculate_buffer_ser();
    void calculate_buffer_single();
    void calculate_buffer_stereo();
    bool get_graph(int index, int subindex, float *data, int points, cairo_t *context);
    static bool get_static_graph(int index, int subindex, float value, float *data, int points, cairo_t *context);
    inline bool is_stereo_filter() const
    {
        return filter_type == flt_2lp12 || filter_type == flt_2bp6;
    }
    void calculate_step();
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
                    for(uint32_t i = 0 ; i < len; i++) {
                        float vol = master.get();
                        outs[0][op + i] = buffer[ip + i] * vol,
                        outs[1][op + i] = buffer2[ip + i] * vol;
                    }
                else
                    for(uint32_t i = 0 ; i < len; i++)
                        outs[0][op + i] = outs[1][op + i] = buffer[ip + i] * master.get();
                op += len;
                output_pos += len;
                if (output_pos == step_size)
                    output_pos = 0;
            }
        }
            
        return 3;
    }
    static const char *get_name() { return "monosynth"; }
    static const char *get_id() { return "monosynth"; }
    static const char *get_label() { return "Monosynth"; }
};

using namespace dsp;

struct organ_audio_module: public null_audio_module, public drawbar_organ
{
public:
    using drawbar_organ::note_on;
    using drawbar_organ::note_off;
    using drawbar_organ::control_change;
    enum { 
        par_drawbar1, par_drawbar2, par_drawbar3, par_drawbar4, par_drawbar5, par_drawbar6, par_drawbar7, par_drawbar8, par_drawbar9, 
        par_frequency1, par_frequency2, par_frequency3, par_frequency4, par_frequency5, par_frequency6, par_frequency7, par_frequency8, par_frequency9, 
        par_waveform1, par_waveform2, par_waveform3, par_waveform4, par_waveform5, par_waveform6, par_waveform7, par_waveform8, par_waveform9, 
        par_detune1, par_detune2, par_detune3, par_detune4, par_detune5, par_detune6, par_detune7, par_detune8, par_detune9, 
        par_phase1, par_phase2, par_phase3, par_phase4, par_phase5, par_phase6, par_phase7, par_phase8, par_phase9, 
        par_pan1, par_pan2, par_pan3, par_pan4, par_pan5, par_pan6, par_pan7, par_pan8, par_pan9, 
        par_routing1, par_routing2, par_routing3, par_routing4, par_routing5, par_routing6, par_routing7, par_routing8, par_routing9, 
        par_foldover,
        par_percdecay, par_perclevel, par_percharm, par_master, 
        par_f1cutoff, par_f1res, par_f1env1, par_f1env2, par_f1env3, par_f1keyf,
        par_f2cutoff, par_f2res, par_f2env1, par_f2env2, par_f2env3, par_f2keyf,
        par_eg1attack, par_eg1decay, par_eg1sustain, par_eg1release, par_eg1velscl, par_eg1ampctl, 
        par_eg2attack, par_eg2decay, par_eg2sustain, par_eg2release, par_eg2velscl, par_eg2ampctl, 
        par_eg3attack, par_eg3decay, par_eg3sustain, par_eg3release, par_eg3velscl, par_eg3ampctl, 
        param_count };
    enum { in_count = 0, out_count = 2, support_midi = true, rt_capable = true };
    static const char *port_names[];
    float *ins[in_count]; 
    float *outs[out_count];
    float *params[param_count];
    organ_parameters par_values;
    uint32_t srate;

    organ_audio_module()
    : drawbar_organ(&par_values)
    {
    }
    static parameter_properties param_props[];
    static const char *get_gui_xml();

    void set_sample_rate(uint32_t sr) {
        srate = sr;
    }
    void params_changed() {
        for (int i = 0; i < param_count; i++)
            ((float *)&par_values)[i] = *params[i];
        update_params();
    }
    void activate() {
        setup(srate);
    }
    void deactivate() {
    }
    uint32_t process(uint32_t offset, uint32_t nsamples, uint32_t inputs_mask, uint32_t outputs_mask) {
        float *o[2] = { outs[0] + offset, outs[1] + offset };
        render_to(o, nsamples);
        return 3;
    }
    static const char *get_name() { return "organ"; }    
    static const char *get_id() { return "organ"; }    
    static const char *get_label() { return "Organ"; }    
};

};

#endif
