/* Calf DSP Library
 * Example audio modules - monosynth
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
#include <config.h>
#include <complex>
#if USE_JACK
#include <jack/jack.h>
#endif
#include <calf/giface.h>
#include <calf/modules_synths.h>
#include <calf/modules_dev.h>

#if ENABLE_EXPERIMENTAL

using namespace dsp;
using namespace calf_plugins;
using namespace std;

fluidsynth_audio_module::fluidsynth_audio_module()
{
    settings = NULL;
    synth = NULL;
}

void fluidsynth_audio_module::post_instantiate()
{
    settings = new_fluid_settings();
    synth = create_synth(sfid);
}

void fluidsynth_audio_module::activate()
{
}

void fluidsynth_audio_module::deactivate()
{
}

fluid_synth_t *fluidsynth_audio_module::create_synth(int &new_sfid)
{
    fluid_settings_t *new_settings = new_fluid_settings();
    fluid_synth_t *s = new_fluid_synth(new_settings);
    if (!soundfont.empty())
    {
        int sid = fluid_synth_sfload(s, soundfont.c_str(), 1);
        if (sid == -1)
        {
            delete_fluid_synth(s);
            return NULL;
        }
        assert(sid >= 0);
        printf("sid=%d\n", sid);
        fluid_synth_sfont_select(s, 0, sid);
        fluid_synth_bank_select(s, 0, 0);
        fluid_synth_program_change(s, 0, 0);
        new_sfid = sid;
    }
    else
        new_sfid = -1;
    return s;
}

void fluidsynth_audio_module::note_on(int note, int vel)
{
    fluid_synth_noteon(synth, 0, note, vel);
}

void fluidsynth_audio_module::note_off(int note, int vel)
{
    fluid_synth_noteoff(synth, 0, note);
}

void fluidsynth_audio_module::control_change(int controller, int value)
{
    fluid_synth_cc(synth, 0, controller, value);
}

void fluidsynth_audio_module::program_change(int program)
{
    fluid_synth_program_change(synth, 0, program);
}

uint32_t fluidsynth_audio_module::process(uint32_t offset, uint32_t nsamples, uint32_t inputs_mask, uint32_t outputs_mask)
{
    fluid_synth_write_float(synth, nsamples, outs[0], offset, 1, outs[1], offset, 1);
    return 3;
}

char *fluidsynth_audio_module::configure(const char *key, const char *value)
{
    if (!strcmp(key, "soundfont"))
    {
        printf("Loading %s\n", value);
        soundfont = value;
        int newsfid = -1;
        fluid_synth_t *new_synth = create_synth(newsfid);
        
        if (new_synth)
        {
            synth = new_synth;
            sfid = newsfid;
        }
        else
            return strdup("Cannot load a soundfont");
        // XXXKF memory leak
    }
    return NULL;
}

void fluidsynth_audio_module::send_configures(send_configure_iface *sci)
{
    sci->send_configure("soundfont", soundfont.c_str());
}

fluidsynth_audio_module::~fluidsynth_audio_module()
{
    if (synth) {
        delete_fluid_synth(synth);
        synth = NULL;
    }
    if (settings) {
        // delete_fluid_settings(settings);
        settings = NULL;
    }
}

#endif
