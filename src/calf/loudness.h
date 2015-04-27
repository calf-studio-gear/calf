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
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 * Boston, MA 02111-1307, USA.
 */
#ifndef __CALF_LOUDNESS_H
#define __CALF_LOUDNESS_H

#include "biquad.h"

namespace dsp {
    
class aweighter {
public:
    biquad_d2 bq1, bq2, bq3;
    
    /// Produce one output sample from one input sample
    float process(float sample)
    {
        return bq1.process(bq2.process(bq3.process(sample)));
    }
    
    /// Set sample rate (updates filter coefficients)
    void set(float sr)
    {
        // analog coeffs taken from: http://www.diracdelta.co.uk/science/source/a/w/aweighting/source.html
        // first we need to adjust them by doing some obscene sort of reverse pre-warping (a broken one, too!)
        float f1 = biquad_coeffs::unwarpf(20.6f, sr);
        float f2 = biquad_coeffs::unwarpf(107.7f, sr);
        float f3 = biquad_coeffs::unwarpf(738.f, sr);
        float f4 = biquad_coeffs::unwarpf(12200.f, sr);
        // then map s domain to z domain using bilinear transform
        // note: f1 and f4 are double poles
        bq1.set_bilinear(0, 0, 1, f1*f1, 2 * f1, 1);
        bq2.set_bilinear(1, 0, 0, f2*f3, f2 + f3, 1);
        bq3.set_bilinear(0, 0, 1, f4*f4, 2 * f4, 1);
        // the coeffs above give non-normalized value, so it should be normalized to produce 0dB at 1 kHz
        // find actual gain
        float gain1kHz = freq_gain(1000.0, sr);
        // divide one filter's x[n-m] coefficients by that value
        float gc = 1.0 / gain1kHz;
        bq1.a0 *= gc;
        bq1.a1 *= gc;
        bq1.a2 *= gc;
    }
    
    /// Reset to zero if at risk of denormals
    void sanitize()
    {
        bq1.sanitize();
        bq2.sanitize();
        bq3.sanitize();
    }
    
    /// Reset state to zero
    void reset()
    {
        bq1.reset();
        bq2.reset();
        bq3.reset();
    }
    
    /// Gain and a given frequency
    float freq_gain(float freq, float sr)
    {
        return bq1.freq_gain(freq, sr) * bq2.freq_gain(freq, sr) * bq3.freq_gain(freq, sr);
    }
    
};

class riaacurve {
public:
    biquad_d2 r1;
    biquad_d2 brickw;
    bool use_brickw;

    riaacurve()
    {
        use_brickw = true;
    }
    
    /// Produce one output sample from one input sample
    float process(float sample)
    {
        return r1.process(use_brickw ? brickw.process(sample) : sample);
    }
    
    /// Set sample rate (updates filter coefficients)
    void set(float sr, int mode, int type)
    {
        float i,j,k,g,a0,a1,a2,b1,b2,tau1,tau2,tau3;
        switch(type) {
            case 0: //"Columbia"
                i = 100.f;
                j = 500.f;
                k = 1590.f;
                break;
            case 1: //"EMI"
                i = 70.f;
                j = 500.f;
                k = 2500.f;
                break;
            case 2: //"BSI(78rpm)"
                i = 50.f;
                j = 353.f;
                k = 3180.f;
                break;
            case 3: //"RIAA"
            default:
                tau1 = 0.003180f;
                tau2 = 0.000318f;
                tau3 = 0.000075f;
                i = 1.f / (2.f * M_PI * tau1);
                j = 1.f / (2.f * M_PI * tau2);
                k = 1.f / (2.f * M_PI * tau3);
            break;
	    case 4: //"CD Mastering"
	        tau1 = 0.000050f;
                tau2 = 0.000015f;
                tau3 = 0.0000001f;// 1.6MHz out of audible range for null impact
                i = 1.f / (2.f * M_PI * tau1);
                j = 1.f / (2.f * M_PI * tau2);
                k = 1.f / (2.f * M_PI * tau3);
            break;
	    case 5: //"50µs FM (Europe)"
	        tau1 = 0.000050f;
                tau2 = tau1 / 20;// not used
                tau3 = tau1 / 50;//
                i = 1.f / (2.f * M_PI * tau1);
                j = 1.f / (2.f * M_PI * tau2);
                k = 1.f / (2.f * M_PI * tau3);
            break;
	    case 6: //"75µs FM (US)"
	        tau1 = 0.000075f;
                tau2 = tau1 / 20;// not used
                tau3 = tau1 / 50;//
                i = 1.f / (2.f * M_PI * tau1);
                j = 1.f / (2.f * M_PI * tau2);
                k = 1.f / (2.f * M_PI * tau3);
            break;
        }

        i *= 2.f * M_PI;
        j *= 2.f * M_PI;
        k *= 2.f * M_PI;

        float t = 1.f / sr;

        //swap a1 b1, a2 b2
        biquad_coeffs coeffs;
        if (type == 7 || type == 8)
        {
            use_brickw = false;
            float tau = (type == 7 ? 0.000050 : 0.000075);
            float f = 1.0 / (2 * M_PI * tau);
            float nyq = sr * 0.5f;
            float gain = sqrt(1.0 + nyq * nyq / (f * f)); // gain at Nyquist
            float cfreq = sqrt((gain - 1.0) * f * f); // frequency 
            float q = 1.0;
            if (type == 8)
                q = pow((sr / 3269.0) + 19.5, -0.25); // somewhat poor curve-fit
            if (type == 7)
                q = pow((sr / 4750.0) + 19.5, -0.25);
            if (mode == 0)
                r1.set_highshelf_rbj(cfreq, q, 1.f / gain, sr);
            else
                r1.set_highshelf_rbj(cfreq, q, gain, sr);
        }
        else
        {
            use_brickw = true;
            if (mode == 0) { //Reproduction
                g = 1.f / (4.f+2.f*i*t+2.f*k*t+i*k*t*t);
                a0 = (2.f*t+j*t*t)*g;
                a1 = (2.f*j*t*t)*g;
                a2 = (-2.f*t+j*t*t)*g;
                b1 = (-8.f+2.f*i*k*t*t)*g;
                b2 = (4.f-2.f*i*t-2.f*k*t+i*k*t*t)*g;
            } else {  //Production
                g = 1.f / (2.f*t+j*t*t);
                a0 = (4.f+2.f*i*t+2.f*k*t+i*k*t*t)*g;
                a1 = (-8.f+2.f*i*k*t*t)*g;
                a2 = (4.f-2.f*i*t-2.f*k*t+i*k*t*t)*g;
                b1 = (2.f*j*t*t)*g;
                b2 = (-2.f*t+j*t*t)*g;
            }
            coeffs.set_bilinear_direct(a0, a1, a2, b1, b2);

            // the coeffs above give non-normalized value, so it should be normalized to produce 0dB at 1 kHz
            // find actual gain
            // Note: for FM emphasis, use 100 Hz for normalization instead
            float gain1kHz = coeffs.freq_gain(1000.0, sr);
            // divide one filter's x[n-m] coefficients by that value
            float gc = 1.0 / gain1kHz;
            r1.a0 = coeffs.a0 * gc;
            r1.a1 = coeffs.a1 * gc;
            r1.a2 = coeffs.a2 * gc;
            r1.b1 = coeffs.b1;
            r1.b2 = coeffs.b2;
        }

        r1.sanitize();

        float cutfreq = std::min(0.45f * sr, 21000.f);
        brickw.set_lp_rbj(cutfreq, 0.707f, sr, 1.f);
        brickw.sanitize();
    }
    
    /// Reset to zero if at risk of denormals
    void sanitize()
    {
        r1.sanitize();
        brickw.sanitize();
    }
    
    /// Reset state to zero
    void reset()
    {
        r1.reset();
    }
    
    /// Gain and a given frequency
    float freq_gain(float freq, float sr) const
    {
        return r1.freq_gain(freq, sr) * (use_brickw ? brickw.freq_gain(freq, sr) : 1.f);
    }
    
};

};

#endif
