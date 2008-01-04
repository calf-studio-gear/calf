/* Calf DSP Library Utility Application - calfjackhost
 * Standalone application module wrapper example.
 * Copyright (C) 2007 Krzysztof Foltman
 *
 * Note: This module uses phat graphics library, so it's 
 * licensed under GPL license, not LGPL.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */
#include <set>
#include <getopt.h>
#include <stdint.h>
#include <stdlib.h>
#include <config.h>
#include <glade/glade.h>
#include <jack/jack.h>
#include <calf/giface.h>
#include <calf/jackhost.h>
#include <calf/modules.h>
#include <calf/modules_dev.h>
#include <calf/gui.h>
#include <calf/preset.h>
#include <calf/preset_gui.h>

using namespace synth;
using namespace std;

// I don't need anyone to tell me this is stupid. I already know that :)
plugin_gui_window *gui_win;

jack_client client;

const char *client_name = "calfhost";

jack_host_base *synth::create_jack_host(const char *effect_name)
{
    if (!strcmp(effect_name, "reverb"))
        return new jack_host<reverb_audio_module>();
    else if (!strcmp(effect_name, "flanger"))
        return new jack_host<flanger_audio_module>();
    else if (!strcmp(effect_name, "filter"))
        return new jack_host<filter_audio_module>();
    else if (!strcmp(effect_name, "monosynth"))
        return new jack_host<monosynth_audio_module>();
    else if (!strcmp(effect_name, "vintagedelay"))
        return new jack_host<vintage_delay_audio_module>();
#ifdef ENABLE_EXPERIMENTAL
    else if (!strcmp(effect_name, "organ"))
        return new jack_host<organ_audio_module>();
    else if (!strcmp(effect_name, "rotaryspeaker"))
        return new jack_host<rotary_speaker_audio_module>();
#endif
    else
        return NULL;
}

void destroy(GtkWindow *window, gpointer data)
{
    gtk_main_quit();
}

static struct option long_options[] = {
    {"help", 0, 0, 'h'},
    {"version", 0, 0, 'v'},
    {"client", 1, 0, 'c'},
    {"effect", 0, 0, 'e'},
    {"preset", 0, 0, 'p'},
    {"input", 1, 0, 'i'},
    {"output", 1, 0, 'o'},
    {"connect-midi", 1, 0, 'M'},
    {0,0,0,0},
};

void print_help(char *argv[])
{
    printf("JACK host for Calf effects\n"
        "Syntax: %s [--client <name>] [--input <name>] [--output <name>] [--midi <name>]\n"
        "       [--connect-midi <name|capture-index>] [--help] [--version] [!] [--preset <name>] pluginname [!] ...\n", 
        argv[0]);
}

int jack_client::do_jack_process(jack_nframes_t nframes, void *p)
{
    jack_client *self = (jack_client *)p;
    for(unsigned int i = 0; i < self->plugins.size(); i++)
        self->plugins[i]->process(nframes);
    return 0;
}

int jack_client::do_jack_bufsize(jack_nframes_t numsamples, void *p)
{
    jack_client *self = (jack_client *)p;
    for(unsigned int i = 0; i < self->plugins.size(); i++)
        self->plugins[i]->cache_ports();
    return 0;
}

int main(int argc, char *argv[])
{
    vector<string> names;
    vector<jack_host_base *> plugins;
    vector<plugin_gui_window *> guis;
    set<int> chains;
    map<int, string> preset_options;
    map<int, string> presets;
    string autoconnect_midi;
    gtk_init(&argc, &argv);
    glade_init();
    while(1) {
        int option_index;
        int c = getopt_long(argc, argv, "c:i:o:m:M:ep:hv", long_options, &option_index);
        if (c == -1)
            break;
        switch(c) {
            case 'h':
            case '?':
                print_help(argv);
                return 0;
            case 'v':
                printf("%s\n", PACKAGE_STRING);
                return 0;
            case 'e':
                fprintf(stderr, "Warning: switch -%c is deprecated!\n", c);
                break;
            case 'p':
                preset_options[optind] = optarg;
                break;
            case 'c':
                client_name = optarg;
                break;
            case 'i':
                client.input_name = string(optarg) + "_%d";
                break;
            case 'o':
                client.output_name = string(optarg) + "_%d";
                break;
            case 'm':
                client.midi_name = string(optarg) + "_%d";
                break;
            case 'M':
                if (atoi(optarg))
                    autoconnect_midi = "system:midi_capture_" + string(optarg);
                else
                    autoconnect_midi = string(optarg);
                break;
        }
    }
    while(optind < argc) {
        if (!strcmp(argv[optind], "!")) {
            chains.insert(names.size());
            optind++;
        } else {
            while (!preset_options.empty() && preset_options.begin()->first < optind) {
                presets[names.size()] = preset_options.begin()->second;
                // printf("preset[%s] = %s\n", argv[optind], presets[names.size()].c_str());
                preset_options.erase(preset_options.begin());
            }
            names.push_back(argv[optind++]);
        }
    }
    if (!names.size()) {
        print_help(argv);
        return 0;
    }
    try {
        global_presets.load_defaults();
    }
    catch(synth::preset_exception &e)
    {
        fprintf(stderr, "Error while loading presets: %s\n", e.what());
        exit(1);
    }
    try {
        client.open(client_name);
        string cnp = client.get_name() + ":";
        for (unsigned int i = 0; i < names.size(); i++) {
            // if (presets.count(i))
            //    printf("%s : %s\n", names[i].c_str(), presets[i].c_str());
            jack_host_base *jh = create_jack_host(names[i].c_str());
            if (!jh) {
#ifdef ENABLE_EXPERIMENTAL
                fprintf(stderr, "Unknown plugin name; allowed are: reverb, flanger, filter, vintagedelay, monosynth, organ, rotaryspeaker\n");
#else
                fprintf(stderr, "Unknown plugin name; allowed are: reverb, flanger, filter, vintagedelay, monosynth\n");
#endif
                return 1;
            }
            jh->open(&client);
            gui_win = new plugin_gui_window;
            gui_win->conditions.insert("jackhost");
            gui_win->conditions.insert("directlink");
            gui_win->create(jh, (string(client_name)+" - "+names[i]).c_str(), names[i].c_str());
            gtk_signal_connect(GTK_OBJECT(gui_win->toplevel), "destroy", G_CALLBACK(destroy), NULL);
            gtk_widget_show_all(GTK_WIDGET(gui_win->toplevel));
            guis.push_back(gui_win);
            plugins.push_back(jh);
            client.add(jh);
            if (presets.count(i)) {
                string cur_plugin = names[i];
                string preset = presets[i];
                preset_vector &pvec = global_presets.presets;
                bool found = false;
                for (unsigned int i = 0; i < pvec.size(); i++) {
                    if (pvec[i].name == preset && pvec[i].plugin == cur_plugin)
                    {
                        pvec[i].activate(jh);
                        gui_win->gui->refresh();
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    fprintf(stderr, "Warning: unknown preset %s %s\n", preset.c_str(), cur_plugin.c_str());
                }
            }
        }
        client.activate();
        for (unsigned int i = 0; i < plugins.size(); i++) {
            if (chains.count(i)) {
                if (!i)
                {
                    if (plugins[0]->get_output_count() < 2)
                    {
                        fprintf(stderr, "Cannot connect input to plugin %s - incompatible ports\n", plugins[0]->name.c_str());
                    } else {
                        client.connect("system:capture_1", cnp + plugins[0]->get_inputs()[0].name);
                        client.connect("system:capture_2", cnp + plugins[0]->get_inputs()[1].name);
                    }
                }
                else
                {
                    if (plugins[i - 1]->get_output_count() < 2 || plugins[i]->get_input_count() < 2)
                    {
                        fprintf(stderr, "Cannot connect plugins %s and %s - incompatible ports\n", plugins[i - 1]->name.c_str(), plugins[i]->name.c_str());
                    }
                    else {
                        client.connect(cnp + plugins[i - 1]->get_outputs()[0].name, cnp + plugins[i]->get_inputs()[0].name);
                        client.connect(cnp + plugins[i - 1]->get_outputs()[1].name, cnp + plugins[i]->get_inputs()[1].name);
                    }
                }
            }
        }
        if (chains.count(plugins.size()) && plugins.size())
        {
            int last = plugins.size() - 1;
            if (plugins[last]->get_output_count() < 2)
            {
                fprintf(stderr, "Cannot connect plugin %s to output - incompatible ports\n", plugins[last]->name.c_str());
            } else {
                client.connect(cnp + plugins[last]->get_outputs()[0].name, "system:playback_1");
                client.connect(cnp + plugins[last]->get_outputs()[1].name, "system:playback_2");
            }
        }
        if (autoconnect_midi != "") {
            for (unsigned int i = 0; i < plugins.size(); i++)
            {
                if (plugins[i]->get_midi())
                    client.connect(autoconnect_midi, cnp + plugins[i]->get_midi_port()->name);
            }
        }
        gtk_main();
        client.deactivate();
        for (unsigned int i = 0; i < names.size(); i++) {
            delete guis[i];
            plugins[i]->close();
            delete plugins[i];
        }
        client.close();
        
        // this is now done on preset add
        // save_presets(get_preset_filename().c_str());
    }
    catch(std::exception &e)
    {
        fprintf(stderr, "%s\n", e.what());
        exit(1);
    }
    return 0;
}
