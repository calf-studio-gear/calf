/* Calf DSP Library
 * A-weighting filter for 
 * Copyright (C) 2001-2007 Krzysztof Foltman
 *
 * Most of code in this file is based on freely
 * available other work of other people (filter equations).
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
#ifndef __CALF_LOUDNESS_H
#define __CALF_LOUDNESS_H

#include "biquad.h"

namespace dsp {
    
class aweighter {
public:
    biquad_d2<float> bq1, bq2, bq3;
    
    float process(float sample)
    {
        return bq1.process(bq2.process(bq3.process(sample)));
    }
    
    void set(float sr)
    {
        // lowpass : H(s) = 1 / (1 + st) = 1 / (1 + s/w) = 1 / (1 + s/(2piF))
        // wc = 2pi * fc

        // This is not done yet - I need to finish redoing the coeffs properly
        float f1 = 129.4f / sr; 
        float f2 = 676.7f / sr; 
        float f3 = 4636.f / sr; 
        float f4 = 76655.f / sr;
        /*
        float f1 = biquad_coeffs<float>::unwarpf(129.4f, sr);
        float f2 = biquad_coeffs<float>::unwarpf(676.7f, sr);
        float f3 = biquad_coeffs<float>::unwarpf(4636.f, sr);
        float f4 = biquad_coeffs<float>::unwarpf(76655.f, sr);
        */
        bq1.set_bilinear(0, 0, 1, f1*f1, 2 * f1, 1);
        bq2.set_bilinear(4.0, 0, 0, f2*f3, f2 + f3, 1);
        bq3.set_bilinear(0, 0, 1, f4*f4, 2 * f4, 1);
    }
    
    void sanitize()
    {
        bq1.sanitize();
        bq2.sanitize();
        bq3.sanitize();
    }
    
    void reset()
    {
        bq1.reset();
        bq2.reset();
        bq3.reset();
    }
    
    float freq_gain(float freq, float sr)
    {
        return bq1.freq_gain(freq, sr) * bq2.freq_gain(freq, sr) * bq3.freq_gain(freq, sr);
    }
    
};

};

#endif
