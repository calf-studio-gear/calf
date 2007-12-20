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
#include <getopt.h>
#include <stdint.h>
#include <stdlib.h>
#include <config.h>
#include <sys/stat.h>
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
#ifdef ENABLE_EXPERIMENTAL
    else if (!strcmp(effect_name, "organ"))
        return new jack_host<organ_audio_module>();
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
    {"plugin", 0, 0, 'p'},
    {"input", 1, 0, 'i'},
    {"output", 1, 0, 'o'},
    {0,0,0,0},
};

void print_help(char *argv[])
{
    printf("JACK host for Calf effects\n"
        "Syntax: %s [--client <name>] [--input <name>] [--output <name>] [--midi <name>]\n"
        "       [--help] [--version] pluginname ...\n", 
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
    vector<jack_host_base *> hosts;
    vector<plugin_gui_window *> guis;
    gtk_init(&argc, &argv);
    glade_init();
    while(1) {
        int option_index;
        int c = getopt_long(argc, argv, "c:i:o:m:ephv", long_options, &option_index);
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
            case 'p':
                fprintf(stderr, "Warning: switch -%c is deprecated!\n", c);
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
        }
    }
    while(optind < argc)
        names.push_back(argv[optind++]);
    if (!names.size()) {
        print_help(argv);
        return 0;
    }
    try {
        struct stat st;
        if (!stat(get_preset_filename().c_str(), &st))
            load_presets(get_preset_filename().c_str());
        else if (!stat(PKGLIBDIR "/presets.xml", &st))
            load_presets(PKGLIBDIR "/presets.xml");
        
        client.open(client_name);
        for (unsigned int i = 0; i < names.size(); i++) {
            jack_host_base *jh = create_jack_host(names[i].c_str());
            if (!jh) {
    #ifdef ENABLE_EXPERIMENTAL
                fprintf(stderr, "Unknown plugin name; allowed are: reverb, flanger, filter, monosynth, organ\n");
    #else
                fprintf(stderr, "Unknown plugin name; allowed are: reverb, flanger, filter, monosynth\n");
    #endif
                return 1;
            }
            jh->open(&client);
            gui_win = new plugin_gui_window;
            gui_win->create(jh, (string(client_name)+" - "+names[i]).c_str(), names[i].c_str());
            gtk_signal_connect(GTK_OBJECT(gui_win->toplevel), "destroy", G_CALLBACK(destroy), NULL);
            guis.push_back(gui_win);
            hosts.push_back(jh);
            client.add(jh);
        }
        client.activate();
        gtk_main();
        client.deactivate();
        for (unsigned int i = 0; i < names.size(); i++) {
            delete guis[i];
            hosts[i]->close();
            delete hosts[i];
        }
        client.close();
        
        save_presets(get_preset_filename().c_str());
    }
    catch(std::exception &e)
    {
        fprintf(stderr, "%s\n", e.what());
        exit(1);
    }
    return 0;
}
