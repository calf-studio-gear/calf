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

namespace synth {

#if ENABLE_EXPERIMENTAL

class compressor_audio_module: public null_audio_module {
public:
    enum { in_count = 1, out_count = 1, support_midi = false, rt_capable = true };
    enum { dummy, param_count };

    static const char *port_names[in_count + out_count];
    static synth::ladspa_plugin_info plugin_info;
    float *ins[in_count];
    float *outs[out_count];
    float *params[param_count];
    uint32_t srate;
    static parameter_properties param_props[];
    uint32_t process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask) {
        return 0;
    }

    static const char *get_name() { return "compressor"; }
    static const char *get_id() { return "compressor"; }
    static const char *get_label() { return "Compressor"; }

};

#endif
    
};

#endif
