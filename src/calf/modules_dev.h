/* Calf DSP Library
 * Prototype audio modules
 *
 * Copyright (C) 2008 Thor Harald Johansen <thj@thj.no>
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

#include <calf/inertia.h>
#include <calf/biquad.h>
#include <stdexcept>

namespace calf_plugins {

#if ENABLE_EXPERIMENTAL
class filter_module_base
{
public:
	virtual void  calculate_filter(float freq, float q, int mode) = 0;
	virtual void  filter_activate() = 0;
	virtual int   process_channel(uint16_t channel_no, float *in, float *out, uint32_t numsamples, int inmask) = 0;
	virtual float freq_gain(int subindex, float freq, float srate) = 0;
};

class biquad_filter_module: public filter_module_base
{
private:
    dsp::biquad_d1<float> left[3], right[3];
    int order;
    
public:    
    uint32_t srate;
    
public:
	biquad_filter_module() : order(0) {}
	
    void calculate_filter(float freq, float q, int mode)
    {
        if (mode < 3) {
            order = mode + 1;
            left[0].set_lp_rbj(freq, pow(q, 1.0 / order), srate);
        } else {
            order = mode - 2;
            left[0].set_hp_rbj(freq, pow(q, 1.0 / order), srate);
        }
        
        right[0].copy_coeffs(left[0]);
        for (int i = 1; i < order; i++) {
            left[i].copy_coeffs(left[0]);
            right[i].copy_coeffs(left[0]);
        }
    }
    
    void filter_activate()
    {
        for (int i=0; i < order; i++) {
            left[i].reset();
            right[i].reset();
        }
    }
	
    inline int process_channel(uint16_t channel_no, float *in, float *out, uint32_t numsamples, int inmask) {
    	dsp::biquad_d1<float> *filter;
    	switch (channel_no) {
    	case 0:
    		filter = left;
    		break;
    		
    	case 1:
    		filter = right;
    		break;
    	
    	default:
    		throw std::invalid_argument("channel_no");
    		break;
    	}
    	
        if (inmask) {
            switch(order) {
                case 1:
                    for (uint32_t i = 0; i < numsamples; i++)
                        out[i] = filter[0].process(in[i]);
                    break;
                case 2:
                    for (uint32_t i = 0; i < numsamples; i++)
                        out[i] = filter[1].process(filter[0].process(in[i]));
                    break;
                case 3:
                    for (uint32_t i = 0; i < numsamples; i++)
                        out[i] = filter[2].process(filter[1].process(filter[0].process(in[i])));
                    break;
            }
        } else {
            if (filter[order - 1].empty())
                return 0;
            switch(order) {
                case 1:
                    for (uint32_t i = 0; i < numsamples; i++)
                        out[i] = filter[0].process_zeroin();
                    break;
                case 2:
                    if (filter[0].empty())
                        for (uint32_t i = 0; i < numsamples; i++)
                            out[i] = filter[1].process_zeroin();
                    else
                        for (uint32_t i = 0; i < numsamples; i++)
                            out[i] = filter[1].process(filter[0].process_zeroin());
                    break;
                case 3:
                    if (filter[1].empty())
                        for (uint32_t i = 0; i < numsamples; i++)
                            out[i] = filter[2].process_zeroin();
                    else
                        for (uint32_t i = 0; i < numsamples; i++)
                            out[i] = filter[2].process(filter[1].process(filter[0].process_zeroin()));
                    break;
            }
        }
        for (int i = 0; i < order; i++)
            filter[i].sanitize();
        return filter[order - 1].empty() ? 0 : inmask;
    }
    
    float freq_gain(int subindex, float freq, float srate)
    {
        float level = 1.0;
        for (int j = 0; j < order; j++)
            level *= left[j].freq_gain(freq, srate);                
        return level;
    }
};

template<typename FilterClass, typename Metadata>
class filter_module_with_inertia: public FilterClass
{
public:
    float *ins[Metadata::in_count]; 
    float *outs[Metadata::out_count];
    float *params[Metadata::param_count];

    inertia<exponential_ramp> inertia_cutoff, inertia_resonance;
    once_per_n timer;
    bool is_active;    
    
    filter_module_with_inertia()
    : inertia_cutoff(exponential_ramp(128), 20)
    , inertia_resonance(exponential_ramp(128), 20)
    , timer(128)
    {
        is_active = false;
    }
    
    void calculate_filter()
    {
        float freq = inertia_cutoff.get_last();
        // printf("freq=%g inr.cnt=%d timer.left=%d\n", freq, inertia_cutoff.count, timer.left);
        // XXXKF this is resonance of a single stage, obviously for three stages, resonant gain will be different
        float q    = inertia_resonance.get_last();
        int   mode = dsp::fastf2i_drm(*params[Metadata::par_mode]);
        // printf("freq = %f q = %f mode = %d\n", freq, q, mode);
        
        int inertia = dsp::fastf2i_drm(*params[Metadata::par_inertia]);
        if (inertia != inertia_cutoff.ramp.length()) {
            inertia_cutoff.ramp.set_length(inertia);
            inertia_resonance.ramp.set_length(inertia);
        }
        
        FilterClass::calculate_filter(freq, q, mode);
    }
    
    void template_params_changed()
    {
        inertia_cutoff.set_inertia(*params[Metadata::par_cutoff]);
        inertia_resonance.set_inertia(*params[Metadata::par_resonance]);
        calculate_filter();
    }
    
    void on_timer()
    {
        inertia_cutoff.step();
        inertia_resonance.step();
        calculate_filter();
    }
    
    void template_activate()
    {
    	template_params_changed();
        FilterClass::filter_activate();
        timer = once_per_n(FilterClass::srate / 1000);
        timer.start();
        is_active = true;
    }
    
    void template_set_sample_rate(uint32_t sr)
    {
    	FilterClass::srate = sr;
    }

    
    void template_deactivate()
    {
        is_active = false;
    }

    uint32_t process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask) {
//        printf("sr=%d cutoff=%f res=%f mode=%f\n", FilterClass::srate, *params[Metadata::par_cutoff], *params[Metadata::par_resonance], *params[Metadata::par_mode]);
        uint32_t ostate = 0;
        numsamples += offset;
        while(offset < numsamples) {
            uint32_t numnow = numsamples - offset;
            // if inertia's inactive, we can calculate the whole buffer at once
            if (inertia_cutoff.active() || inertia_resonance.active())
                numnow = timer.get(numnow);
            
            if (outputs_mask & 1) {
                ostate |= FilterClass::process_channel(0, ins[0] + offset, outs[0] + offset, numnow, inputs_mask & 1);
            }
            if (outputs_mask & 2) {
                ostate |= FilterClass::process_channel(1, ins[1] + offset, outs[1] + offset, numnow, inputs_mask & 2);
            }
            
            if (timer.elapsed()) {
                on_timer();
            }
            offset += numnow;
        }
        return ostate;
    }
};

class filter_audio_module: 
	public audio_module<filter_metadata>, 
	public filter_module_with_inertia<biquad_filter_module, filter_metadata>, 
	public line_graph_iface
{
public:    
    void params_changed()
    { 
    	template_params_changed(); 
    }
        
    void activate()
    {
    	template_activate();
    }
    
    void set_sample_rate(uint32_t sr)
    {
    	template_set_sample_rate(sr);
    }

    
    void deactivate()
    {
    	template_deactivate();
    }
    
	
    bool get_graph(int index, int subindex, float *data, int points, cairo_iface *context);
    bool get_gridline(int index, int subindex, float &pos, bool &vertical, std::string &legend, cairo_iface *context);
};
#endif
    
};

#endif
