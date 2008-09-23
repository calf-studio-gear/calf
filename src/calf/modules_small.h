/* Calf DSP Library
 * "Small" audio modules for modular synthesis
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
#ifndef __CALF_MODULES_SMALL_H
#define __CALF_MODULES_SMALL_H

/// Empty implementations for plugin functions. Note, that functions aren't virtual, because they're called via the particular
/// subclass via template wrappers (ladspa_small_wrapper<> etc), not via base class pointer/reference
class null_small_audio_module
{
public:
    /// LADSPA-esque activate function, except it is called after ports are connected, not before
    inline void activate() {}
    /// LADSPA-esque deactivate function
    inline void deactivate() {}
    /// Set sample rate for the plugin
    inline void set_sample_rate(uint32_t sr) { }
};
#endif
