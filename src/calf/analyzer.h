/* Calf Analyzer FFT Library
 * Copyright (C) 2007-2013 Krzysztof Foltman, Markus Schmidt,
 * Christian Holschuh and others
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
 * Boston, MA  02110-1301  USA
 */
 
#ifndef CALF_ANALYZER_H
#define CALF_ANALYZER_H

#include <assert.h>
#include <fftw3.h>
#include <limits.h>
#include "biquad.h"
#include "inertia.h"
#include "audio_fx.h"
#include "giface.h"
#include "metadata.h"
#include "loudness.h"
#include <math.h>
#include "plugin_tools.h"

namespace dsp {
#if 0
}; to keep editor happy
#endif

class analyzer {
private:
public:
    uint32_t srate;
    analyzer();
    void process();
    void set_sample_rate(uint32_t sr);
    void set_params();
};



#if 0
{ to keep editor happy
#endif
}

#endif
