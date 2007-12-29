/* Calf DSP Library
 * Prototype audio modules
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
#ifndef __CALF_MODULES_DEV_H
#define __CALF_MODULES_DEV_H

#if ENABLE_EXPERIMENTAL

#include <assert.h>
#include "biquad.h"
#include "audio_fx.h"
#include "synth.h"
#include "organ.h"

namespace synth {

using namespace dsp;

struct organ_audio_module: public null_audio_module, public drawbar_organ
{
public:
    using drawbar_organ::note_on;
    using drawbar_organ::note_off;
    using drawbar_organ::control_change;
    enum { par_drawbar1, par_drawbar2, par_drawbar3, par_drawbar4, par_drawbar5, par_drawbar6, par_drawbar7, par_drawbar8, par_drawbar9, par_foldover,
        par_percmode, par_percharm, par_master, param_count };
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
    
};

class rotary_speaker_audio_module: public null_audio_module
{
public:
    enum { par_speed, param_count };
    enum { in_count = 2, out_count = 2, support_midi = true, rt_capable = true };
    static const char *port_names[];
    float *ins[in_count]; 
    float *outs[out_count];
    float *params[param_count];
    double phase_l, dphase_l, phase_h, dphase_h;
    int cos_l, sin_l, cos_h, sin_h;
    dsp::simple_delay<4096, float> delay;
    dsp::biquad<float> crossover1l, crossover1r, crossover2l, crossover2r;
    dsp::simple_delay<8, float> phaseshift;
    uint32_t srate;
    int vibrato_mode;
    static parameter_properties param_props[];
    float mwhl_value, hold_value;

    rotary_speaker_audio_module()
    {
        mwhl_value = hold_value = 0.f;
        phase_h = phase_l = 0.f;
    }    
    void set_sample_rate(uint32_t sr) {
        srate = sr;
        setup();
    }
    void setup()
    {
        crossover1l.set_lp_rbj(800.f, 0.7, (float)srate);
        crossover1r.set_lp_rbj(800.f, 0.7, (float)srate);
        crossover2l.set_hp_rbj(800.f, 0.7, (float)srate);
        crossover2r.set_hp_rbj(800.f, 0.7, (float)srate);
        set_vibrato();
    }
    void params_changed() {
        set_vibrato();
    }
    void activate() {
        phase_h = phase_l = 0.f;
        setup();
    }
    void deactivate() {
    }
    void set_vibrato()
    {
        vibrato_mode = fastf2i_drm(*params[par_speed]);
        if (!vibrato_mode)
            return;
        float speed = vibrato_mode - 1;
        if (vibrato_mode == 3)
            speed = hold_value;
        if (vibrato_mode == 4)
            speed = mwhl_value;
        speed = (speed < 0.5f) ? 0 : 1;
        float speed_h = 48 + (400-48) * speed;
        float speed_l = 40 + (342-40) * speed;
        dphase_h = speed_h / (60 * srate);
        dphase_l = speed_l / (60 * srate);
        update_speed();
    }
    void update_speed()
    {
        cos_h = (int)(16384*16384*cos(dphase_h * 2 * PI));
        sin_h = (int)(16384*16384*sin(dphase_h * 2 * PI));
        cos_l = (int)(16384*16384*cos(dphase_l * 2 * PI));
        sin_l = (int)(16384*16384*sin(dphase_l * 2 * PI));
    }
    static inline void update_euler(long long int &x, long long int &y, int dx, int dy)
    {
        long long int nx = (x * dx - y * dy) >> 28;
        long long int ny = (x * dy + y * dx) >> 28;
        x = nx;
        y = ny;
    }    
    uint32_t process(uint32_t offset, uint32_t nsamples, uint32_t inputs_mask, uint32_t outputs_mask)
    {
        if (vibrato_mode)
        {
            long long int xl0 = (int)(10000*16384*cos(phase_l * 2 * PI));
            long long int yl0 = (int)(10000*16384*sin(phase_l * 2 * PI));
            long long int xh0 = (int)(10000*16384*cos(phase_h * 2 * PI));
            long long int yh0 = (int)(10000*16384*sin(phase_h * 2 * PI));
            // printf("xl=%d yl=%d dx=%d dy=%d\n", (int)(xl0>>14), (int)(yl0 >> 14), cos_l, sin_l);
            for (unsigned int i = 0; i < nsamples; i++) {
                float in_l = ins[0][i + offset], in_r = ins[1][i + offset];
                float in_mono = 0.5f * (in_l + in_r);
                
                // int xl = (int)(10000 * cos(phase_l)), yl = (int)(10000 * sin(phase_l));
                //int xh = (int)(10000 * cos(phase_h)), yh = (int)(10000 * sin(phase_h));
                update_euler(xl0, yl0, cos_l, sin_l);
                int xl = xl0 >> 14, yl = yl0 >> 14;
                int xh = xh0 >> 14, yh = yh0 >> 14;
                // printf("xl=%d yl=%d xl'=%f yl'=%f\n", xl, yl, 16384*cos((phase_l + dphase_l * i) * 2 * PI), 16384*sin((phase_l + dphase_l * i) * 2 * PI));
                update_euler(xh0, yh0, cos_h, sin_h);
                
                float out_hi_l = delay.get_interp_1616(500000 + 40 * xh) + 0.0001 * xh * delay.get_interp_1616(500000 - 40 * yh) - delay.get_interp_1616(800000 - 60 * xh);
                float out_hi_r = delay.get_interp_1616(550000 - 48 * yh) - 0.0001 * yh * delay.get_interp_1616(700000 + 46 * xh) - delay.get_interp_1616(1000000 + 76 * yh);

                float out_lo_l = 0.5f * in_mono + delay.get_interp_1616(400000 + 34 * xl) + delay.get_interp_1616(600000 - 18 * yl);
                float out_lo_r = 0.5f * in_mono + delay.get_interp_1616(600000 - 50 * xl) - delay.get_interp_1616(900000 + 15 * yl);
                
                out_hi_l = crossover2l.process_d2(out_hi_l); // sanitize(out_hi_l);
                out_hi_r = crossover2r.process_d2(out_hi_r); // sanitize(out_hi_r);
                out_lo_l = crossover1l.process_d2(out_lo_l); // sanitize(out_lo_l);
                out_lo_r = crossover1r.process_d2(out_lo_r); // sanitize(out_lo_r);
                
                float out_l = out_hi_l + out_lo_l;
                float out_r = out_hi_r + out_lo_r;
                
                in_mono += 0.06f * (out_l + out_r);
                sanitize(in_mono);
                
                outs[0][i + offset] = out_l;
                outs[1][i + offset] = out_r;
                delay.put(in_mono);
            }
            crossover1l.sanitize_d2();
            crossover1r.sanitize_d2();
            crossover2l.sanitize_d2();
            crossover2r.sanitize_d2();
            phase_l = fmod(phase_l + nsamples * dphase_l, 1.0);
            phase_h = fmod(phase_h + nsamples * dphase_h, 1.0);
        } else
        {
            memcpy(outs[0] + offset, ins[0] + offset, sizeof(float) * nsamples);
            memcpy(outs[1] + offset, ins[1] + offset, sizeof(float) * nsamples);
        }
        return outputs_mask;
    }
    virtual void control_change(int ctl, int val)
    {
        if (vibrato_mode == 3 && ctl == 64)
        {
            hold_value = val / 127.f;
            set_vibrato();
            return;
        }
        if (vibrato_mode == 4 && ctl == 1)
        {
            mwhl_value = val / 127.f;
            set_vibrato();
            return;
        }
    }
};

};

#endif

#endif
