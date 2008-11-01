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
#include "envelope.h"
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
    enum { in_count = 0, out_count = 2, support_midi = true, require_midi = true, rt_capable = true };
    enum { step_size = 64 };
    static const char *port_names[];
    static synth::ladspa_plugin_info plugin_info;
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
    int wave1, wave2, filter_type, last_filter_type;
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
    /// Handle MIDI Note On message (does not immediately trigger a note, as it must start on
    /// boundary of step_size samples).
    void note_on(int note, int vel)
    {
        queue_note_on = note;
        last_key = note;
        queue_vel = vel / 127.f;
        stack.push(note);
    }
    /// Handle MIDI Note Off message
    void note_off(int note, int vel)
    {
        stack.pop(note);
        // If releasing the currently played note, try to get another one from note stack.
        if (note == last_key) {
            if (stack.count())
            {
                last_key = note = stack.nth(stack.count() - 1);
                start_freq = freq;
                target_freq = freq = note_to_hz(note);
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
    /// Handle pitch bend message.
    inline void pitch_bend(int value)
    {
        pitchbend = pow(2.0, value / 8192.0);
    }
    /// Update oscillator frequency based on base frequency, detune amount, pitch bend scaling factor and sample rate.
    inline void set_frequency()
    {
        osc1.set_freq(freq * (2 - detune) * pitchbend, srate);
        osc2.set_freq(freq * (detune)  * pitchbend * xpose, srate);
    }
    /// Handle control change messages.
    void control_change(int controller, int value);
    /// Update variables from control ports.
    void params_changed() {
        float sf = 0.001f;
        envelope.set(*params[par_attack] * sf, *params[par_decay] * sf, std::min(0.999f, *params[par_sustain]), *params[par_release] * sf, srate / step_size);
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
    /// Hard-knee 2:1 reducation outside range [-0.75, +0.75]
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
    /// Run oscillators and two filters in series to produce mono output samples.
    void calculate_buffer_ser();
    /// Run oscillators and just one filter to produce mono output samples.
    void calculate_buffer_single();
    /// Run oscillators and two filters (one per channel) to produce stereo output samples.
    void calculate_buffer_stereo();
    /// Retrieve filter graph (which is 'live' so it cannot be generated by get_static_graph), or fall back to get_static_graph.
    bool get_graph(int index, int subindex, float *data, int points, cairo_t *context);
    /// Retrieve waveform graph (which does not need information about synth state)
    static bool get_static_graph(int index, int subindex, float value, float *data, int points, cairo_t *context);
    /// @retval true if the filter 1 is to be used for the left channel and filter 2 for the right channel
    /// @retval false if filters are to be connected in series and sent (mono) to both channels    
    inline bool is_stereo_filter() const
    {
        return filter_type == flt_2lp12 || filter_type == flt_2bp6;
    }
    /// No CV inputs for now
    static bool is_cv(int param_no) { return false; }
    /// Practically all the stuff here is noisy
    static bool is_noisy(int param_no) { return true; }
    /// Calculate control signals and produce step_size samples of output.
    void calculate_step();
    /// Main processing function
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
    enum { in_count = 0, out_count = 2, support_midi = true, require_midi = true, rt_capable = true };
    static const char *port_names[];
    float *ins[in_count]; 
    float *outs[out_count];
    float *params[param_count];
    organ_parameters par_values;
    uint32_t srate;
    bool panic_flag;
    /// Value for configure variable map_curve
    std::string var_map_curve;

    organ_audio_module()
    : drawbar_organ(&par_values)
    {
    }
    static parameter_properties param_props[];
    static synth::ladspa_plugin_info plugin_info;
    static const char *get_gui_xml();

    void set_sample_rate(uint32_t sr) {
        srate = sr;
    }
    void params_changed() {
        for (int i = 0; i < param_count; i++)
            ((float *)&par_values)[i] = *params[i];
        update_params();
    }
    inline void pitch_bend(int amt)
    {
        drawbar_organ::pitch_bend(amt);
    }
    void activate() {
        setup(srate);
        panic_flag = false;
    }
    void deactivate() {
    }
    uint32_t process(uint32_t offset, uint32_t nsamples, uint32_t inputs_mask, uint32_t outputs_mask) {
        float *o[2] = { outs[0] + offset, outs[1] + offset };
        if (panic_flag)
        {
            control_change(120, 0); // stop all sounds
            control_change(121, 0); // reset all controllers
            panic_flag = false;
        }
        render_separate(o, nsamples);
        return 3;
    }
    /// No CV inputs for now
    static bool is_cv(int param_no) { return false; }
    /// Practically all the stuff here is noisy
    static bool is_noisy(int param_no) { return true; }
    void execute(int cmd_no);
    bool get_graph(int index, int subindex, float *data, int points, cairo_t *context);
    static const char *get_name() { return "organ"; }    
    static const char *get_id() { return "organ"; }    
    static const char *get_label() { return "Organ"; }    
    static plugin_command_info *get_commands();
    char *configure(const char *key, const char *value);
    void send_configures(send_configure_iface *);
    static const char **get_default_configure_vars();
};

};

#endif
