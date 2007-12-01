/* Calf DSP Library
 * Basic one-pole one-zero filters based on bilinear transform.
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
#ifndef __CALF_ONEPOLE_H
#define __CALF_ONEPOLE_H

#include "primitives.h"

namespace dsp {

/**
 * one-pole filter, for floating point values
 * coefficient calculation is based on bilinear transform, and the code itself is based on my very old OneSignal lib
 * lp and hp are *somewhat* tested, allpass is not tested at all
 * don't use this for integers because it won't work
 */
template<class T = float, class Coeff = float>
class onepole
{
public:
    T x1, y1;
    Coeff a0, a1, b1;
    
    void set_lp(float fc, float sr)
    {
        //   x   x
        //  x+1 x-1
		Coeff x = tan (PI * fc / (2 * sr));
        Coeff q = 1/(1+x);
		a0 = a1 = x*q;
		b1 = (x-1)*q;
    }
    
    void set_ap(float fc, float sr)
    {
        // x-1  x+1
        // x+1  x-1
		Coeff x = tan (PI * fc / (2 * sr));
		Coeff q = 1/(1+x);
		b1 = a0 = (x-1) / (1+x);
        a1 = 1;
    }
    
    void set_hp(float fc, float sr)
    {
        //   x   -x
        //  x+1  x-1
		Coeff x = tan (PI * fc / (2 * sr));
		Coeff q = 1/(1+x);
		a0 = q;
        a1 = -a0;
		b1 = (x-1)*q;
    }
    
    inline T process(T in)
    {
        T out = in * a0 + x1 * a1 - y1 * b1;
        x1 = in;
        y1 = out;
        return out;
    }
    
    inline T process_lp(T in)
    {
        T out = (in + x1) * a0 - y1 * b1;
        x1 = in;
        y1 = out;
        return out;
    }
    
    inline T process_hp(T in)
    {
        T out = (in - x1) * a0 - y1 * b1;
        x1 = in;
        y1 = out;
        return out;
    }
    
    inline T process_ap(T in)
    {
        T out = (in - y1) * a0 + x1;
        x1 = in;
        y1 = out;
        return out;
    }
    
    inline void sanitize() 
    {
        dsp::sanitize(x1);
        dsp::sanitize(y1);
    }
    
    inline void reset()
    {
        dsp::zero(x1);
        dsp::zero(y1);
    }
};

};

#endif
