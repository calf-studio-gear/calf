/* Calf DSP Library 
 * Peak metering facilities.
 *
 * Copyright (C) 2007 Krzysztof Foltman
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */
#ifndef __CALF_VUMETER_H
#define __CALF_VUMETER_H

/// Peak meter class
struct vumeter
{
    /// Measured signal level
    float level;
    /// Falloff of signal level (b1 coefficient of a 1-pole filter)
    float falloff;
    /// Clip indicator (set to 1 when |value| >= 1, fading otherwise)
    float clip;
    /// Falloff of clip indicator (b1 coefficient of a 1-pole filter); set to 1 if no falloff is required (manual reset of clip indicator)
    float clip_falloff;
    
    vumeter()
    {
        falloff = 0.999f;
        level = 0;
        clip_falloff = 0.999f;
        clip = 0;
    }
    
    /// Update peak meter based on input signal
    inline void update(float *src, unsigned int len)
    {
        // "Age" the old level by falloff^length
        float tmp = level * pow(falloff, len);
        // Same for clip level (using different fade constant)
        double tmp_clip = clip * pow(clip_falloff, len);
        // Process input samples - to get peak value, take a max of all values in the input signal and "aged" old peak
        // Clip is set to 1 if any sample is out-of-range, if no clip occurs, the "aged" value is assumed
        for (unsigned int i = 0; i < len; i++) {
            float sig = fabs(src[i]);
            tmp = std::max(tmp, sig);
            if (sig >= 1.f)
                tmp_clip = 1.f;
        }
        level = tmp;
        clip = tmp_clip;
        dsp::sanitize(level);
        dsp::sanitize(clip);
    }
    /// Update clip meter as if update was called with all-zero input signal
    inline void update_zeros(unsigned int len)
    {
        level *= pow((double)falloff, (double)len);
        clip *= pow((double)clip_falloff, (double)len);
        dsp::sanitize(level);
        dsp::sanitize(clip);
    }
};

#endif
