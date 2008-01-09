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
#if USE_LASH
#include <lash/lash.h>
#endif
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

struct host_session
{
    string client_name, input_name, output_name, midi_name;
    vector<string> plugin_names;
    map<int, string> presets;
#if USE_LASH
    lash_client_t *lash_client;
    lash_args_t *lash_args;
#endif
    
    // these are not saved
    jack_client client;
    string autoconnect_midi;
    set<int> chains;
    vector<jack_host_base *> plugins;
    vector<plugin_gui_window *> guis;
    int lash_source_id;
    bool restoring_session;
    
    host_session();
    void open();
    void connect();
    void close();
    static gboolean update_lash(void *self) { ((host_session *)self)->update_lash(); return TRUE; }
    void update_lash();
#if USE_LASH
    void send_lash(LASH_Event_Type type, const std::string &data) {
        lash_send_event(lash_client, lash_event_new_with_all(type, data.c_str()));
    }
    void send_config(const char *key, const std::string &data) {
        char *buffer = new char[data.length()];
        memcpy(buffer, data.data(), data.length());
        lash_config_t *cfg = lash_config_new_with_key(strdup(key));
        lash_config_set_value(cfg, buffer, data.length());
        lash_send_config(lash_client, cfg);
    }
#endif
};

host_session::host_session()
{
    client_name = "calf";
    lash_client = NULL;
    lash_args = NULL;
    lash_source_id = 0;
    restoring_session = false;
}

void host_session::open()
{
    if (!input_name.empty()) client.input_name = input_name;
    if (!output_name.empty()) client.output_name = output_name;
    if (!midi_name.empty()) client.midi_name = midi_name;
    client.open(client_name.c_str());
    for (unsigned int i = 0; i < plugin_names.size(); i++) {
        // if (presets.count(i))
        //    printf("%s : %s\n", names[i].c_str(), presets[i].c_str());
        jack_host_base *jh = create_jack_host(plugin_names[i].c_str());
        if (!jh) {
#ifdef ENABLE_EXPERIMENTAL
            throw audio_exception("Unknown plugin name; allowed are: reverb, flanger, filter, vintagedelay, monosynth, organ, rotaryspeaker\n");
#else
            throw audio_exception("Unknown plugin name; allowed are: reverb, flanger, filter, vintagedelay, monosynth\n");
#endif
        }
        jh->open(&client);
        gui_win = new plugin_gui_window;
        gui_win->conditions.insert("jackhost");
        gui_win->conditions.insert("directlink");
        gui_win->create(jh, (string(client_name)+" - "+plugin_names[i]).c_str(), plugin_names[i].c_str());
        gtk_signal_connect(GTK_OBJECT(gui_win->toplevel), "destroy", G_CALLBACK(destroy), NULL);
        gtk_widget_show_all(GTK_WIDGET(gui_win->toplevel));
        guis.push_back(gui_win);
        plugins.push_back(jh);
        client.add(jh);
        if (presets.count(i)) {
            string cur_plugin = plugin_names[i];
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
}

void host_session::connect()
{
    client.activate();
#if USE_LASH
    if (lash_client)
        lash_jack_client_name(lash_client, client.get_name().c_str());
#endif
    if (!restoring_session) 
    {
        string cnp = client.get_name() + ":";
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
    }
#if USE_LASH
    send_lash(LASH_Client_Name, "calf-"+client_name);
    lash_source_id = g_timeout_add_full(G_PRIORITY_LOW, 250, update_lash, this, NULL); // 4 LASH reads per second... should be enough?
#endif
}

void host_session::close()
{
    if (lash_source_id)
        g_source_remove(lash_source_id);
    client.deactivate();
    for (unsigned int i = 0; i < plugin_names.size(); i++) {
        delete guis[i];
        plugins[i]->close();
        delete plugins[i];
    }
    client.close();
}

#if USE_LASH
void host_session::update_lash()
{
    do {
        lash_event_t *event = lash_get_event(lash_client);
        if (!event)
            break;
        
        // printf("type = %d\n", lash_event_get_type(event));
        
        switch(lash_event_get_type(event)) {        
            case LASH_Save_Data_Set:
            {
                for (unsigned int i = 0; i < plugins.size(); i++) {
                    char ss[32];
                    plugin_preset preset;
                    preset.plugin = plugin_names[i];
                    preset.get_from(plugins[i]);
                    sprintf(ss, "plugin%d", i);
                    string pstr = preset.to_xml();
                    lash_config_t *cfg = lash_config_new_with_key(ss);
                    lash_config_set_value(cfg, pstr.c_str(), pstr.length());
                    lash_send_config(lash_client, cfg);
                }
                send_lash(LASH_Save_Data_Set, "");
                break;
            }
            
            case LASH_Restore_Data_Set:
            {
                // printf("!!!Restore data set!!!\n");
                while(lash_config_t *cfg = lash_get_config(lash_client)) {
                    const char *key = lash_config_get_key(cfg);
                    // printf("key = %s\n", lash_config_get_key(cfg));
                    string data = string((const char *)lash_config_get_value(cfg), lash_config_get_value_size(cfg));
                    if (!strncmp(key, "plugin", 6))
                    {
                        unsigned int nplugin = atoi(key + 6);
                        if (nplugin < plugins.size())
                        {
                            preset_list tmp;
                            tmp.parse("<presets>"+data+"</presets>");
                            if (tmp.presets.size())
                            {
                                tmp.presets[0].activate(plugins[nplugin]);
                                guis[nplugin]->gui->refresh();
                            }
                        }
                    }
                    lash_config_destroy(cfg);
                }
                send_lash(LASH_Restore_Data_Set, "");
                break;
            }
                
            case LASH_Quit:
                gtk_main_quit();
                break;
            
            default:
                g_warning("Unhandled LASH event %d (%s)", lash_event_get_type(event), lash_event_get_string(event));
                break;
        }
    } while(1);
}
#endif

host_session current_session;

int main(int argc, char *argv[])
{
    host_session &sess = current_session;
    map<int, string> preset_options;
    gtk_init(&argc, &argv);
#if USE_LASH
    for (int i = 1; i < argc; i++)
    {
        if (!strncmp(argv[i], "--lash-project=", 14)) {
            sess.restoring_session = true;
            break;
        }
    }
    sess.lash_args = lash_extract_args(&argc, &argv);
    sess.lash_client = lash_init(sess.lash_args, PACKAGE_NAME, LASH_Config_Data_Set, LASH_PROTOCOL(2, 0));
    if (!sess.lash_client) {
        g_warning("Warning: failed to create a LASH connection");
    }
#endif
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
                sess.client_name = optarg;
                break;
            case 'i':
                sess.input_name = string(optarg) + "_%d";
                break;
            case 'o':
                sess.output_name = string(optarg) + "_%d";
                break;
            case 'm':
                sess.midi_name = string(optarg) + "_%d";
                break;
            case 'M':
                if (atoi(optarg))
                    sess.autoconnect_midi = "system:midi_capture_" + string(optarg);
                else
                    sess.autoconnect_midi = string(optarg);
                break;
        }
    }
    while(optind < argc) {
        if (!strcmp(argv[optind], "!")) {
            sess.chains.insert(sess.plugin_names.size());
            optind++;
        } else {
            while (!preset_options.empty() && preset_options.begin()->first < optind) {
                sess.presets[sess.plugin_names.size()] = preset_options.begin()->second;
                // printf("preset[%s] = %s\n", argv[optind], presets[names.size()].c_str());
                preset_options.erase(preset_options.begin());
            }
            sess.plugin_names.push_back(argv[optind++]);
        }
    }
    if (!sess.plugin_names.size()) {
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
        sess.open();
        sess.connect();
        sess.client.activate();
        gtk_main();
        sess.close();
        
#if USE_LASH
        if (sess.lash_args)
            lash_args_destroy(sess.lash_args);
#endif
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
