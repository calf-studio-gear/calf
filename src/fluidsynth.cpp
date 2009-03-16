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

        fluid_sfont_t* sfont = fluid_synth_get_sfont(s, 0);
        soundfont_name = (*sfont->get_name)(sfont);

        sfont->iteration_start(sfont);
        
        string preset_list;
        fluid_preset_t tmp;
        while(sfont->iteration_next(sfont, &tmp))
        {
            string pname = tmp.get_name(&tmp);
            int bank = tmp.get_banknum(&tmp);
            int num = tmp.get_num(&tmp);
            sf_preset_names[num + 128 * bank] = pname;
            preset_list += calf_utils::i2s(num + 128 * bank) + "\t" + pname + "\n";
        }
        soundfont_preset_list = preset_list;
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

    if (controller == 0 || controller == 32)
        update_preset_num();
}

void fluidsynth_audio_module::program_change(int program)
{
    fluid_synth_program_change(synth, 0, program);

    update_preset_num();
}


void fluidsynth_audio_module::update_preset_num()
{
    fluid_preset_t *p = fluid_synth_get_channel_preset(synth, 0);
    if (p)
        last_selected_preset = p->get_num(p) + 128 * p->get_banknum(p);
    else
        last_selected_preset = 0;
}

uint32_t fluidsynth_audio_module::process(uint32_t offset, uint32_t nsamples, uint32_t inputs_mask, uint32_t outputs_mask)
{
    fluid_synth_set_gain(synth, *params[par_master]);
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
            update_preset_num();
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

int fluidsynth_audio_module::send_status_updates(send_updates_iface *sui, int last_serial)
{
    sui->send_status("sf_name", soundfont_name.c_str());
    sui->send_status("preset_list", soundfont_preset_list.c_str());

    map<uint32_t, string>::const_iterator i = sf_preset_names.find(last_selected_preset);
    if (i == sf_preset_names.end())
        sui->send_status("preset_name", "");
    else
        sui->send_status("preset_name", i->second.c_str());
    return last_serial + 1;
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
