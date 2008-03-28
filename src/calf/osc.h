/* Calf DSP Library
 * Oscillators
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

#ifndef __CALF_OSC_H
#define __CALF_OSC_H

#include "fft.h"
#include <map>

namespace synth
{

/// Very simple, non-bandlimited saw oscillator. Should not be used for anything
/// else than testing/prototyping.
struct simple_oscillator
{
    uint32_t phase, phasedelta;
    void reset()
    {
        phase = 0;
    }
    void set_freq(float freq, float sr)
    {
        phasedelta = (int)(freq * 65536.0 * 256.0 * 16.0 / sr) << 4;
    }
    inline float get()
    {
        float value = (phase >> 16 ) / 65535.0 - 0.5;
        phase += phasedelta;
        return value;
    }
};

template<int SIZE_BITS>
struct bandlimiter
{
    enum { SIZE = 1 << SIZE_BITS };
    static dsp::fft<float, SIZE_BITS> fft;
    
    std::complex<float> spectrum[SIZE];
    
    void compute_spectrum(float input[SIZE])
    {
        std::complex<float> data[SIZE];
        for (int i = 0; i < SIZE; i++)
            data[i] = input[i];
        fft.calculate(data, spectrum, false);
    }
    
    void compute_waveform(float output[SIZE])
    {
        std::complex<float> data[SIZE];
        fft.calculate(spectrum, data, true);
        for (int i = 0; i < SIZE; i++)
            output[i] = data[i].real();
    }
    
    /// remove DC offset of the spectrum (it usually does more harm than good!)
    void remove_dc()
    {
        spectrum[0] = 0.f;
    }
    
    /// very basic bandlimiting (brickwall filter)
    /// might need to be improved much in future!
    void make_waveform(float output[SIZE], int cutoff)
    {
        std::complex<float> new_spec[SIZE], iffted[SIZE];
        for (int i = 0; i < cutoff; i++)
            new_spec[i] = spectrum[i], 
            new_spec[SIZE - 1 - i] = spectrum[SIZE - 1 - i];
        for (int i = cutoff; i < SIZE / 2; i++)
            new_spec[i] = 0.f,
            new_spec[SIZE - 1 - i] = 0.f;
        fft.calculate(new_spec, iffted, true);
        for (int i = 0; i < SIZE; i++)
            output[i] = iffted[i].real();
    }
};

template<int SIZE_BITS>
dsp::fft<float, SIZE_BITS> bandlimiter<SIZE_BITS>::fft;

/// Set of bandlimited wavetables
template<int SIZE_BITS>
struct waveform_family: public map<uint32_t, float *>
{
    enum { SIZE = 1 << SIZE_BITS };
    using map<uint32_t, float *>::iterator;
    using map<uint32_t, float *>::end;
    using map<uint32_t, float *>::lower_bound;
    float original[SIZE];
    
    void make(bandlimiter<SIZE_BITS> &bl, float input[SIZE])
    {
        memcpy(original, input, sizeof(original));
        bl.compute_spectrum(input);
        bl.remove_dc();
        
        uint32_t multiple = 1, base = 1 << (32 - SIZE_BITS);
        while(multiple < SIZE / 2) {
            float *wf = new float[SIZE];
            bl.make_waveform(wf, (int)((1 << SIZE_BITS) / (1.5 * multiple)));
            (*this)[base * multiple] = wf;
            multiple = multiple << 1;
        }
    }
    
    inline float *get_level(uint32_t phase_delta)
    {
        iterator i = upper_bound(phase_delta);
        if (i == end())
            return NULL;
        // printf("Level = %08x\n", i->first);
        return i->second;
    }
};

template<int SIZE_BITS>
struct waveform_oscillator: public simple_oscillator
{
    enum { SIZE = 1 << SIZE_BITS, MASK = SIZE - 1 };
    float *waveform;
    inline float get()
    {
        uint32_t wpos = phase >> (32 - SIZE_BITS);
        float value = lerp(waveform[wpos], waveform[(wpos + 1) & MASK], (phase & (SIZE - 1)) * (1.0f / SIZE));
        phase += phasedelta;
        return value;
    }
};

static inline void normalize_waveform(float *table, unsigned int size)
{
    float thismax = 0;
    for (unsigned int i = 0; i < size; i++)
        thismax = std::max(thismax, fabs(table[i]));
    if (thismax < 0.000001f)
        return;
    for (unsigned int i = 0; i < size; i++)
        table[i] /= thismax;
}



};

#endif
