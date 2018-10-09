/* Calf DSP Library
 * Fluidsynth wrapper
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
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 * Boston, MA  02110-1301  USA
 */
#include <calf/giface.h>
#include <calf/modules_dev.h>
#include <calf/utils.h>
#include <string.h>

#if ENABLE_EXPERIMENTAL

using namespace dsp;
using namespace calf_plugins;
using namespace std;

fluidsynth_audio_module::fluidsynth_audio_module()
{
    settings = NULL;
    synth = NULL;
    status_serial = 1;
    std::fill(set_presets, set_presets + 16, -1);
    std::fill(last_selected_presets, last_selected_presets + 16, -1);
}

void fluidsynth_audio_module::post_instantiate(uint32_t sr)
{
    srate = sr;
    settings = new_fluid_settings();
    synth = create_synth(sfid);
    soundfont_loaded = sfid != -1;
}

void fluidsynth_audio_module::activate()
{
}

void fluidsynth_audio_module::deactivate()
{
}

fluid_synth_t *fluidsynth_audio_module::create_synth(int &new_sfid)
{
    std::fill(set_presets, set_presets + 16, -1);
    fluid_settings_t *new_settings = new_fluid_settings();
    fluid_settings_setnum(new_settings, "synth.sample-rate", srate);
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
        new_sfid = sid;

        fluid_sfont_t* sfont = fluid_synth_get_sfont(s, 0);
#if FLUIDSYNTH_VERSION_MAJOR < 2
        soundfont_name = (*sfont->get_name)(sfont);

        sfont->iteration_start(sfont);
        
        string preset_list;
        fluid_preset_t tmp;
        int first_preset = -1;
        while(sfont->iteration_next(sfont, &tmp))
        {
            string pname = tmp.get_name(&tmp);
            int bank = tmp.get_banknum(&tmp);
            int num = tmp.get_num(&tmp);
            int id = num + 128 * bank;
            sf_preset_names[id] = pname;
            preset_list += calf_utils::i2s(id) + "\t" + pname + "\n";
            if (first_preset == -1)
                first_preset = id;
        }
#else
        soundfont_name = fluid_sfont_get_name(sfont);

        fluid_sfont_iteration_start(sfont);

        string preset_list;
        fluid_preset_t* tmp;
        int first_preset = -1;
        while((tmp = fluid_sfont_iteration_next(sfont)))
        {
            string pname = fluid_preset_get_name(tmp);
            int bank = fluid_preset_get_banknum(tmp);
            int num = fluid_preset_get_num(tmp);
            int id = num + 128 * bank;
            sf_preset_names[id] = pname;
            preset_list += calf_utils::i2s(id) + "\t" + pname + "\n";
            if (first_preset == -1)
                first_preset = id;
        }
#endif
        if (first_preset != -1)
        {
            fluid_synth_bank_select(s, 0, first_preset >> 7);
            fluid_synth_program_change(s, 0, first_preset & 127);        
        }
        soundfont_preset_list = preset_list;
    }
    else
        new_sfid = -1;
    return s;
}

void fluidsynth_audio_module::note_on(int channel, int note, int vel)
{
    fluid_synth_noteon(synth, channel, note, vel);
}

void fluidsynth_audio_module::note_off(int channel, int note, int vel)
{
    fluid_synth_noteoff(synth, channel, note);
}

void fluidsynth_audio_module::control_change(int channel, int controller, int value)
{
    fluid_synth_cc(synth, channel, controller, value);

    if (controller == 0 || controller == 32)
        update_preset_num(channel);
}

void fluidsynth_audio_module::program_change(int channel, int program)
{
    fluid_synth_program_change(synth, channel, program);

    update_preset_num(channel);
}


void fluidsynth_audio_module::update_preset_num(int channel)
{
    fluid_preset_t *p = fluid_synth_get_channel_preset(synth, channel);
    if (p)
#if FLUIDSYNTH_VERSION_MAJOR < 2
        last_selected_presets[channel] = p->get_num(p) + 128 * p->get_banknum(p);
#else
        last_selected_presets[channel] = fluid_preset_get_num(p) + 128 * fluid_preset_get_banknum(p);
#endif
    else
        last_selected_presets[channel] = -1;
    status_serial++;
}

void fluidsynth_audio_module::select_preset_in_channel(int channel, int new_preset)
{
    fluid_synth_bank_select(synth, channel, new_preset >> 7);
    fluid_synth_program_change(synth, channel, new_preset & 127);
    last_selected_presets[channel] = new_preset;
}

uint32_t fluidsynth_audio_module::process(uint32_t offset, uint32_t nsamples, uint32_t inputs_mask, uint32_t outputs_mask)
{
    static const int interp_lens[] = { 0, 1, 4, 7 };
    for (int i = 0; i < 16; ++i)
    {
        int new_preset = set_presets[i];
        if (new_preset != -1 && soundfont_loaded)
        {
            // XXXKF yeah there's a tiny chance of race here, have to live with it until I write some utility classes for lock-free data passing
            set_presets[i] = -1;
            select_preset_in_channel(i, new_preset);
        }
    }
    if (!soundfont_loaded)
    {
        // Reset selected presets array to 'no preset'
        std::fill(last_selected_presets, last_selected_presets + 16, -1);
    }
    fluid_synth_set_interp_method(synth, -1, interp_lens[dsp::clip<int>(fastf2i_drm(*params[par_interpolation]), 0, 3)]);
    fluid_synth_set_reverb_on(synth, *params[par_reverb] > 0);
    fluid_synth_set_chorus_on(synth, *params[par_chorus] > 0);
    fluid_synth_set_gain(synth, *params[par_master]);
    fluid_synth_write_float(synth, nsamples, outs[0], offset, 1, outs[1], offset, 1);
    return 3;
}

char *fluidsynth_audio_module::configure(const char *key, const char *value)
{
    if (!strncmp(key, "preset_key_set", 14))
    {
        int ch = atoi(key + 14);
        if (ch > 0)
            ch--;
        if (ch >= 0 && ch <= 15)
            set_presets[ch] = value ? atoi(value) : 0;
        return NULL;
    }
    if (!strcmp(key, "soundfont"))
    {
        if (value && *value)
        {
            printf("Loading %s\n", value);
            soundfont = value;
        }
        else
        {
            printf("Creating a blank synth\n");
            soundfont.clear();
        }
        // First synth not yet created - defer creation up to post_instantiate
        if (!synth)
            return NULL;
        int newsfid = -1;
        fluid_synth_t *new_synth = create_synth(newsfid);
        soundfont_loaded = newsfid != -1;

        status_serial++;
        
        if (new_synth)
        {
            synth = new_synth;
            sfid = newsfid;
            for (int i = 0; i < 16; ++i)
                update_preset_num(i);
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
    sci->send_configure("preset_key_set", calf_utils::i2s(last_selected_presets[0]).c_str());
    for (int i = 1; i < 16; ++i)
    {
        string key = "preset_key_set" + calf_utils::i2s(i + 1);
        sci->send_configure(key.c_str(), calf_utils::i2s(last_selected_presets[i]).c_str());
    }
}

int fluidsynth_audio_module::send_status_updates(send_updates_iface *sui, int last_serial)
{
    int cur_serial = status_serial;
    if (cur_serial != last_serial)
    {
        sui->send_status("sf_name", soundfont_name.c_str());
        sui->send_status("preset_list", soundfont_preset_list.c_str());
        for (int i = 0; i < 16; ++i)
        {
            string id = i ? calf_utils::i2s(i + 1) : string();
            string key = "preset_key" + id;
            sui->send_status(key.c_str(), calf_utils::i2s(last_selected_presets[i]).c_str());
            key = "preset_name" + id;
            map<uint32_t, string>::const_iterator it = sf_preset_names.find(last_selected_presets[i]);
            if (it == sf_preset_names.end())
                sui->send_status(key.c_str(), "");
            else
                sui->send_status(key.c_str(), it->second.c_str());
        }
    }
    return cur_serial;
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
