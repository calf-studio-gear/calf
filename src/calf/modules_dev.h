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

#include <calf/metadata.h>
#include <calf/modules.h>

namespace calf_plugins {

#if ENABLE_EXPERIMENTAL

/// Filterclavier --- MIDI controlled filter
// TODO: add bandpass (set_bp_rbj)
class filterclavier_audio_module: 
        public audio_module<filterclavier_metadata>, 
        public filter_module_with_inertia<biquad_filter_module, filterclavier_metadata>, 
        public line_graph_iface
    {
    public:    
        
        void params_changed()
        { 
            inertia_filter_module::params_changed(); 
        }
            
        void activate()
        {
            inertia_filter_module::activate();
        }
        
        void set_sample_rate(uint32_t sr)
        {
            inertia_filter_module::set_sample_rate(sr);
        }

        
        void deactivate()
        {
            inertia_filter_module::deactivate();
        }
      
        /// MIDI control
        virtual void note_on(int note, int vel)
        {
            inertia_filter_module::inertia_cutoff.set_inertia(
                    note_to_hz(note + *params[par_transpose], *params[par_detune]));
            inertia_filter_module::inertia_resonance.set_inertia( 
                    (float(vel) / 127.0) * (param_props[par_resonance].max - param_props[par_resonance].min)
                    + param_props[par_resonance].min);
            inertia_filter_module::calculate_filter();
        }
        
        virtual void note_off(int note, int vel)
        {
            inertia_filter_module::inertia_resonance.set_inertia(param_props[par_resonance].min);
            inertia_filter_module::calculate_filter();
        }        
        
        bool get_graph(int index, int subindex, float *data, int points, cairo_iface *context);
        bool get_gridline(int index, int subindex, float &pos, bool &vertical, std::string &legend, cairo_iface *context);
    };

#endif
    
};

#endif
