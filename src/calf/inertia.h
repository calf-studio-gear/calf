/* Calf DSP Library
 * Basic "inertia" (parameter smoothing) classes.
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
#ifndef __CALF_INERTIA_H
#define __CALF_INERTIA_H

#include "primitives.h"

namespace dsp {
    
/// Algorithm for a constant time linear ramp
class linear_ramp
{
public:
    int ramp_len;
    float mul, delta;
public:
    linear_ramp(int _ramp_len) {
        ramp_len = _ramp_len;
        mul = (float)(1.0f / ramp_len);
    }
    inline void set_length(int _ramp_len) {
        ramp_len = _ramp_len;
        mul = (float)(1.0f / ramp_len);
    }
    inline int length()
    {
        return ramp_len;
    }
    inline void start_ramp(float start, float end)
    {
        delta = mul * (end - start);
    }
    inline float ramp(float value)
    {
        return value + delta;
    }
};
    
/// Algorithm for a constant time linear ramp
class exponential_ramp
{
public:
    int ramp_len;
    float root, delta;
public:
    exponential_ramp(int _ramp_len) {
        ramp_len = _ramp_len;
        root = (float)(1.0f / ramp_len);
    }
    inline void set_length(int _ramp_len) {
        ramp_len = _ramp_len;
        root = (float)(1.0f / ramp_len);
    }
    inline int length()
    {
        return ramp_len;
    }
    inline void start_ramp(float start, float end)
    {
        delta = pow(end / start, root);
    }
    inline float ramp(float value)
    {
        return value * delta;
    }
};
    
/// Generic inertia using algorithm specified as template argument
template<class Ramp>
class inertia
{
public:
    float old_value;
    float value;
    int count;
    Ramp ramp;

public:
    inertia(const Ramp &_ramp, float init_value = 0.f)
    : ramp(_ramp)
    {
        value = old_value = init_value;
        count = 0;
    }
    /// Set value immediately (no inertia)
    void set_now(float _value)
    {
        value = old_value = _value;
        count = 0;
    }
    /// Set with inertia
    void set_inertia(float source)
    {
        if (source != old_value) {
            ramp.start_ramp(value, source);
            count = ramp.length();
            old_value = source;
        }
    }
    /// Get smoothed value of given source value
    inline float get(float source)
    {
        if (source != old_value) {
            ramp.start_ramp(value, source);
            count = ramp.length();
            old_value = source;
        }
        if (!count)
            return old_value;
        value = ramp.ramp(value);
        count--;
        if (!count) // finished ramping, set to desired value to get rid of accumulated rounding errors
            value = old_value;
        return value;
    }
    /// Get smoothed value assuming no new input
    inline float get()
    {
        if (!count)
            return old_value;
        value = ramp.ramp(value);
        count--;
        if (!count) // finished ramping, set to desired value to get rid of accumulated rounding errors
            value = old_value;
        return value;
    }
    /// Do one inertia step, without returning the new value and without changing inertia
    inline void step()
    {
        if (count) {
            value = ramp.ramp(value);
            count--;
            if (!count) // finished ramping, set to desired value to get rid of accumulated rounding errors
                value = old_value;
        }
    }
    /// Get last smoothed value, without affecting anything
    inline float get_last() const
    {
        return value;
    }
    inline bool active()
    {
        return count > 0;
    }
};

class once_per_n
{
public:
    unsigned int frequency;
    unsigned int left;
public:
    once_per_n(unsigned int _frequency)
    : frequency(_frequency), left(_frequency)
    {}
    inline void start()
    {
        left = frequency;
    }
    /// Set timer to "elapsed" state (elapsed() will return true during next call)
    inline void signal()
    {
        left = 0;
    }
    inline unsigned int get(unsigned int desired)
    {
        if (desired > left) {
            desired = left;
            left = 0;
            return desired;
        }
        left -= desired;
        return desired;
    }
    inline bool elapsed()
    {
        if (!left) {
            left = frequency;
            return true;
        }
        return false;
    }
};
    
}

#endif
