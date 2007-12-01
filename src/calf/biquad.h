/* Calf DSP Library
 * Biquad filters
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
#ifndef __CALF_BIQUAD_H
#define __CALF_BIQUAD_H

#include "primitives.h"

namespace dsp {

/**
 * Two-pole two-zero filter, for floating point values.
 * Coefficient calculation is based on famous Robert Bristow-Johnson's equations.
 * The coefficient calculation is NOT mine, the only exception is the lossy 
 * optimization in Zoelzer and rbj HP filter code.
 * don't use this for integers because it won't work
 */
template<class T = float, class Coeff = float>
class biquad
{
public:
    // type I filter state variables
    T x1, y1, x2, y2;
    // type II filter state variables
    T w1, w2;
    // filter coefficients
    Coeff a0, a1, a2, b1, b2;
    
    /** Lowpass filter based on Robert Bristow-Johnson's equations
     * Perhaps every synth code that doesn't use SVF uses these
     * equations :)
     */
    inline void set_lp_rbj(float fc, float q, float sr, float gain = 1.0)
    {
        float omega=(float)(2*PI*fc/sr);
        float sn=sin(omega);
        float cs=cos(omega);
        float alpha=(float)(sn/(2*q));
        float inv=(float)(1.0/(1.0+alpha));

        a2 = a0 =  (float)(gain*inv*(1 - cs)*0.5f);
        a1 =  a0 + a0;
        b1 =  (float)(-2*cs*inv);
        b2 =  (float)((1 - alpha)*inv);
    }

    // different lowpass filter, based on Zoelzer's equations, modified by
    // me (kfoltman) to use polynomials to approximate tangent function
    // not very accurate, but perhaps good enough for synth work :)
    // odsr is "one divided by samplerate"
    // from how it looks, it perhaps uses bilinear transform - but who knows :)
    inline void set_lp_zoelzer(float fc, float q, float odsr, float gain=1.0)
    {
        Coeff omega=(Coeff)(PI*fc*odsr);
        Coeff omega2=omega*omega;
        Coeff K=omega*(1+omega2*omega2*Coeff(1.0/1.45));
        Coeff KK=K*K;
        Coeff QK=q*(KK+1.f);
        Coeff iQK=1.0f/(QK+K);
        Coeff inv=q*iQK;
        b2 =  (Coeff)(iQK*(QK-K));
        b1 =  (Coeff)(2.f*(KK-1.f)*inv);
        a2 = a0 =  (Coeff)(inv*gain*KK);
        a1 =  a0 + a0;
    }

    void set_hp_rbj(float fc, float q, float esr, float gain=1.0)
    {
        Coeff omega=(float)(2*PI*fc/esr);
		Coeff sn=sin(omega);
		Coeff cs=cos(omega);
        Coeff alpha=(float)(sn/(2*q));

        float inv=(float)(1.0/(1.0+alpha));

        a0 =  (Coeff)(gain*inv*(1 + cs)/2);
        a1 =  -2.f * a0;
        a2 =  a0;
        b1 =  (Coeff)(-2*cs*inv);
        b2 =  (Coeff)((1 - alpha)*inv);
    }

    // this replaces sin/cos with polynomial approximation
    void set_hp_rbj_optimized(float fc, float q, float esr, float gain=1.0)
    {
        Coeff omega=(float)(2*PI*fc/esr);
		Coeff sn=omega+omega*omega*omega*(1.0/6.0)+omega*omega*omega*omega*omega*(1.0/120);
		Coeff cs=1-omega*omega*(1.0/2.0)+omega*omega*omega*omega*(1.0/24);
        Coeff alpha=(float)(sn/(2*q));

        float inv=(float)(1.0/(1.0+alpha));

        a0 =  (Coeff)(gain*inv*(1 + cs)*(1.0/2.0));
        a1 =  -2.f * a0;
        a2 =  a0;
        b1 =  (Coeff)(-2*cs*inv);
        b2 =  (Coeff)((1 - alpha)*inv);
    }
    
    // rbj's bandpass
    void set_bp_rbj(double fc, double q, double esr, double gain=1.0)
    {
        float omega=(float)(2*PI*fc/esr);
        float sn=sin(omega);
        float cs=cos(omega);
        float alpha=(float)(sn/(2*q));

        float inv=(float)(1.0/(1.0+alpha));

        a0 =  (float)(gain*inv*alpha);
        a1 =  0.f;
        a2 =  (float)(-gain*inv*alpha);
        b1 =  (float)(-2*cs*inv);
        b2 =  (float)((1 - alpha)*inv);
    }
    
    // rbj's bandreject
    void set_br_rbj(double fc, double q, double esr, double gain=1.0)
    {
        float omega=(float)(2*PI*fc/esr);
        float sn=sin(omega);
        float cs=cos(omega);
        float alpha=(float)(sn/(2*q));

        float inv=(float)(1.0/(1.0+alpha));

        a0 =  (Coeff)(gain*inv);
        a1 =  (Coeff)(-gain*inv*2*cs);
        a2 =  (Coeff)(gain*inv);
        b1 =  (Coeff)(-2*cs*inv);
        b2 =  (Coeff)((1 - alpha)*inv);
    }
    // this is mine (and, I guess, it sucks/doesn't work)
	void set_allpass(float freq, float pole_r, float sr)
	{
		float a=prewarp(freq, sr);
		float q=pole_r;
		set_bilinear(a*a+q*q, -2.0f*a, 1, a*a+q*q, 2.0f*a, 1);
	}
    // prewarping for bilinear transform, maps given digital frequency to analog counterpart for analog filter design
	float prewarp(float freq, float sr)
	{
		if (freq>sr*0.49) freq=(float)(sr*0.49);
		return (float)(tan(3.1415926*freq/sr));
	}
    // set digital filter parameters based on given analog filter parameters
	void set_bilinear(float aa0, float aa1, float aa2, float ab0, float ab1, float ab2)
	{
		float q=(float)(1.0/(ab0+ab1+ab2));
		a0 = (aa0+aa1+aa2)*q;
		a1 = 2*(aa0-aa2)*q;
		a2 = (aa0-aa1+aa2)*q;
		b1 = 2*(ab0-ab2)*q;
		b2 = (ab0-ab1+ab2)*q;
	}

    // direct I form with four state variables
    inline T process_d1(T in)
    {
        T out = in * a0 + x1 * a1 + x2 * a2 - y1 * b1 - y2 * b2;
        x2 = x1;
        y2 = y1;
        x1 = in;
        y1 = out;
        return out;
    }
    
    // direct I form with zero input
    inline T process_d1_zeroin()
    {
        T out = - y1 * b1 - y2 * b2;
        y2 = y1;
        y1 = out;
        return out;
    }
    
    // simplified version for lowpass case with two zeros at -1
    inline T process_d1_lp(T in)
    {
        T out = a0*(in + x1 + x1 + x2) - y1 * b1 - y2 * b2;
        x2 = x1;
        y2 = y1;
        x1 = in;
        y1 = out;
        return out;
    }
    
    // direct II form with two state variables
    inline T process_d2(T in)
    {
        T tmp = in - y1 * b1 - y2 * b2;
        T out = tmp * a0 + w1 * a1 + w2 * a2;
        w2 = w1;
        w1 = tmp;
        return out;
    }
    
    // direct II form with two state variables, lowpass version
    inline T process_d2_lp(T in)
    {
        T tmp = in - y1 * b1 - y2 * b2;
        T out = (tmp + w1 + w1 + w2) * a0;
        w2 = w1;
        w1 = tmp;
        return out;
    }
    
    bool empty_d1() {
        return (y1 == 0.f && y2 == 0.f);
    }
    
    bool empty_d2() {
        return (w1 == 0.f && w2 == 0.f);
    }
    
    inline void sanitize_d1() 
    {
        dsp::sanitize(x1);
        dsp::sanitize(y1);
        dsp::sanitize(x2);
        dsp::sanitize(y2);
    }
    
    inline void sanitize_d2() 
    {
        dsp::sanitize(w1);
        dsp::sanitize(w2);
    }
    
    inline void reset()
    {
        dsp::zero(x1);
        dsp::zero(y1);
        dsp::zero(w1);
        dsp::zero(x2);
        dsp::zero(y2);
        dsp::zero(w2);
    }
};

};

#endif
