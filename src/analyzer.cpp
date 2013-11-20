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
 
#include <limits.h>
#include <memory.h>
#include <math.h>
#include <fftw3.h>
#include <calf/giface.h>
#include <calf/analyzer.h>
#include <calf/modules_dev.h>
#include <sys/time.h>
#include <calf/utils.h>

using namespace dsp;
using namespace calf_plugins;

#define sinc(x) (!x) ? 1 : sin(M_PI * x)/(M_PI * x);

analyzer::analyzer() {
    
}
void analyzer::set_sample_rate(uint32_t sr) {
    srate = sr;
}
void analyzer::set_params()
{
    
}
void analyzer::process() {
    
}
