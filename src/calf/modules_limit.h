/* Calf DSP plugin pack
 * Limiter related plugins
 *
 * Copyright (C) 2001-2010 Krzysztof Foltman, Markus Schmidt, Thor Harald Johansen and others
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
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 * Boston, MA 02111-1307, USA.
 */
#ifndef CALF_MODULES_LIMIT_H
#define CALF_MODULES_LIMIT_H

#include <assert.h>
#include <limits.h>
#include "biquad.h"
#include "inertia.h"
#include "audio_fx.h"
#include "giface.h"
#include "metadata.h"
#include "plugin_tools.h"

namespace calf_plugins {

/// Multiband Limiter by Markus Schmidt (based on Krzysztof's filters and Steve's lookahead limiter algorythm)
class multibandlimiter_audio_module: public audio_module<multibandlimiter_metadata>, public line_graph_iface {
private:
    typedef multibandlimiter_audio_module AM;
    static const int strips = 4;
    uint32_t clip_inL, clip_inR, clip_outL, clip_outR;
    float meter_inL, meter_inR, meter_outL, meter_outR;
    dsp::lookahead_limiter strip[strips];
    dsp::biquad_d2<float> lpL0, lpR0, lpL1, lpR1, lpL2, lpR2, hpL0, hpR0, hpL1, hpR1, hpL2, hpR2;
    float freq_old[strips - 1], sep_old[strips - 1], q_old[strips - 1];
public:
    uint32_t srate;
    bool is_active;
    multibandlimiter_audio_module();
    void activate();
    void deactivate();
    void params_changed();
    uint32_t process(uint32_t offset, uint32_t numsamples, uint32_t inputs_mask, uint32_t outputs_mask);
    void set_sample_rate(uint32_t sr);
    bool get_graph(int index, int subindex, float *data, int points, cairo_iface *context) const;
    bool get_gridline(int index, int subindex, float &pos, bool &vertical, std::string &legend, cairo_iface *context) const;
};

};

#endif
