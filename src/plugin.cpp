/* Calf DSP Library
 * Example audio modules - LADSPA/DSSI/LV2 wrapper instantiation
 *
 * Copyright (C) 2001-2008 Krzysztof Foltman
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
#include <config.h>
#include <calf/lv2wrap.h>
#include <calf/modules.h>
#include <calf/modules_dev.h>

using namespace synth;

#if USE_LADSPA
#define LADSPA_WRAPPER(mod) static synth::ladspa_wrapper<mod##_audio_module> ladspa_##mod(mod##_audio_module::plugin_info);
#else
#define LADSPA_WRAPPER(mod)
#endif

#if USE_LV2
#define LV2_WRAPPER(mod) static synth::lv2_wrapper<mod##_audio_module> lv2_##mod(mod##_audio_module::plugin_info);
#else
#define LV2_WRAPPER(...)
#endif

#define ALL_WRAPPERS(mod) LADSPA_WRAPPER(mod) LV2_WRAPPER(mod)

#define PER_MODULE_ITEM(name, isSynth, jackname) ALL_WRAPPERS(name)
#include <calf/modulelist.h>

#if USE_LV2
// instantiate descriptor templates
template<class Module> LV2_Descriptor synth::lv2_wrapper<Module>::descriptor;

extern "C" {

const LV2_Descriptor *lv2_descriptor(uint32_t index)
{
    #define PER_MODULE_ITEM(name, isSynth, jackname) if (!(index--)) return &::lv2_##name.descriptor;
    #include <calf/modulelist.h>
#ifdef ENABLE_EXPERIMENTAL
    return lv2_small_descriptor(index);
#else
    return NULL;
#endif
}

};

#endif

#if USE_LADSPA
extern "C" {

const LADSPA_Descriptor *ladspa_descriptor(unsigned long Index)
{
    #define PER_MODULE_ITEM(name, isSynth, jackname) if (!isSynth && !(Index--)) return &::ladspa_##name.descriptor;
    #include <calf/modulelist.h>
    return NULL;
}

};

#if USE_DSSI
extern "C" {

const DSSI_Descriptor *dssi_descriptor(unsigned long Index)
{
    #define PER_MODULE_ITEM(name, isSynth, jackname) if (!(Index--)) return &::ladspa_##name.dssi_descriptor;
    #include <calf/modulelist.h>
    return NULL;
}

};
#endif

#endif

