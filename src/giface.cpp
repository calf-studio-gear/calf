/* Calf DSP Library
 * Module wrapper methods.
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
#include <assert.h>
#include <memory.h>
#include <jack/jack.h>
#include <calf/giface.h>
#include <calf/modules.h>

using namespace synth;

float parameter_properties::from_01(float value01)
{
    switch(flags & PF_SCALEMASK)
    {
    case PF_SCALE_DEFAULT:
    case PF_SCALE_LINEAR:
    case PF_SCALE_PERC:
    default:
        return min + (max - min) * value01;
    case PF_SCALE_LOG:
        return min * pow(max / min, value01);
    case PF_SCALE_GAIN:
        if (value01 < 0.0001)
            return min;
        float rmin = 1.0f / 4096.0f;
        return rmin * pow(max / rmin, value01);
    }
}

float parameter_properties::to_01(float value)
{
    switch(flags & PF_SCALEMASK)
    {
    case PF_SCALE_DEFAULT:
    case PF_SCALE_LINEAR:
    case PF_SCALE_PERC:
    default:
        return (value - min) / (max - min);
    case PF_SCALE_LOG:
        value /= min;
        return log(value) / log(max / min);
    case PF_SCALE_GAIN:
        if (value < 1.0 / 4096.0)
            return 0;
        float rmin = 1.0f / 4096.0f;
        value /= rmin;
        return log(value) / log(max / rmin);
    }
}

