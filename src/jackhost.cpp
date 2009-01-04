/* Calf DSP Library Utility Application - calfjackhost
 * Standalone application module wrapper example.
 *
 * Copyright (C) 2007 Krzysztof Foltman
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
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
#include <calf/main_win.h>
#include <calf/utils.h>

using namespace std;
using namespace calf_utils;
using namespace calf_plugins;

// I don't need anyone to tell me this is stupid. I already know that :)
plugin_gui_window *gui_win;

const char *client_name = "calfhost";

jack_host_base *calf_plugins::create_jack_host(const char *effect_name, const std::string &instance_name)
{
    #define PER_MODULE_ITEM(name, isSynth, jackname) if (!strcasecmp(effect_name, jackname)) return new jack_host<name##_audio_module>(effect_name, instance_name);
    #include <calf/modulelist.h>
    return NULL;
}

void jack_host_base::open(jack_client *_client)
{
    client = _client; //jack_client_open(client_name, JackNullOption, &status);
    
    create_ports();
    
    cache_ports();
    
    init_module();
    changed = false;
}

void jack_host_base::create_ports() {
    char buf[32];
    char buf2[64];
    static const char *suffixes[] = { "l", "r", "2l", "2r" };
    port *inputs = get_inputs();
    port *outputs = get_outputs();
    int in_count = get_input_count(), out_count = get_output_count();
    for (int i=0; i<in_count; i++) {
        sprintf(buf, "%s_in_%s", instance_name.c_str(), suffixes[i]);
        sprintf(buf2, client->input_name.c_str(), client->input_nr++);
        inputs[i].name = buf;
        inputs[i].handle = jack_port_register(client->client, buf, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput , 0);
        inputs[i].data = NULL;
        if (!inputs[i].handle)
            throw text_exception("Could not create JACK input port");
        jack_port_set_alias(inputs[i].handle, buf2);
    }
    if (get_midi()) {
        sprintf(buf, "%s_midi_in", instance_name.c_str());
        sprintf(buf2, client->midi_name.c_str(), client->midi_nr++);
        midi_port.name = buf;
        midi_port.handle = jack_port_register(client->client, buf, JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
        if (!midi_port.handle)
            throw text_exception("Could not create JACK MIDI port");
        jack_port_set_alias(midi_port.handle, buf2);
    }
    for (int i=0; i<out_count; i++) {
        sprintf(buf, "%s_out_%s", instance_name.c_str(), suffixes[i]);
        sprintf(buf2, client->output_name.c_str(), client->output_nr++);
        outputs[i].name = buf;
        outputs[i].handle = jack_port_register(client->client, buf, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput , 0);
        outputs[i].data = NULL;
        if (!outputs[i].handle)
            throw text_exception("Could not create JACK output port");
        jack_port_set_alias(outputs[i].handle, buf2);
    }
}

void jack_host_base::close() {
    port *inputs = get_inputs(), *outputs = get_outputs();
    int input_count = get_input_count(), output_count = get_output_count();
    for (int i = 0; i < input_count; i++) {
        jack_port_unregister(client->client, inputs[i].handle);
        inputs[i].data = NULL;
    }
    for (int i = 0; i < output_count; i++) {
        jack_port_unregister(client->client, outputs[i].handle);
        outputs[i].data = NULL;
    }
    if (get_midi())
        jack_port_unregister(client->client, midi_port.handle);
    client = NULL;
}

void destroy(GtkWindow *window, gpointer data)
{
    gtk_main_quit();
}

void gui_win_destroy(GtkWindow *window, gpointer data)
{
}

static struct option long_options[] = {
    {"help", 0, 0, 'h'},
    {"version", 0, 0, 'v'},
    {"client", 1, 0, 'c'},
    {"effect", 0, 0, 'e'},
    {"preset", 1, 0, 'p'},
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
    ptlock lock(self->mutex);
    for(unsigned int i = 0; i < self->plugins.size(); i++)
        self->plugins[i]->process(nframes);
    return 0;
}

int jack_client::do_jack_bufsize(jack_nframes_t numsamples, void *p)
{
    jack_client *self = (jack_client *)p;
    ptlock lock(self->mutex);
    for(unsigned int i = 0; i < self->plugins.size(); i++)
        self->plugins[i]->cache_ports();
    return 0;
}

void jack_client::delete_plugins()
{
    ptlock lock(mutex);
    for (unsigned int i = 0; i < plugins.size(); i++) {
        // plugins[i]->close();
        delete plugins[i];
    }
    plugins.clear();
}

struct host_session: public main_window_owner_iface
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
    main_window *main_win;
    int lash_source_id;
    bool restoring_session;
    std::set<std::string> instances;
    
    host_session();
    void open();
    void add_plugin(string name, string preset);
    void create_plugins_from_list();
    void connect();
    void close();
    static gboolean update_lash(void *self) { ((host_session *)self)->update_lash(); return TRUE; }
    void update_lash();
    bool activate_preset(int plugin, const std::string &preset, bool builtin);
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
    virtual void new_plugin(const char *name);    
    virtual void remove_plugin(plugin_ctl_iface *plugin);
    std::string get_next_instance_name(const std::string &effect_name);
};

host_session::host_session()
{
    client_name = "calf";
#if USE_LASH
    lash_client = NULL;
    lash_args = NULL;
#endif
    lash_source_id = 0;
    restoring_session = false;
    main_win = new main_window;
    main_win->set_owner(this);
}

std::string host_session::get_next_instance_name(const std::string &effect_name)
{
    if (!instances.count(effect_name))
        return effect_name;
    for (int i = 2; ; i++)
    {
        string tmp = string(effect_name) + i2s(i);
        if (!instances.count(tmp))
            return tmp;
    }
    assert(0);
    return "-";
}

void host_session::add_plugin(string name, string preset)
{
    jack_host_base *jh = create_jack_host(name.c_str(), get_next_instance_name(name));
    if (!jh) {
#ifdef ENABLE_EXPERIMENTAL
#else
#endif
        string s = 
        #define PER_MODULE_ITEM(name, isSynth, jackname) jackname ", "
        #include <calf/modulelist.h>
        ;
        if (!s.empty())
            s = s.substr(0, s.length() - 2);
        throw text_exception("Unknown plugin name; allowed are: " + s);
    }
    instances.insert(jh->instance_name);
    jh->open(&client);
    
    plugins.push_back(jh);
    client.add(jh);
    main_win->add_plugin(jh);
    if (!preset.empty()) {
        if (!activate_preset(plugins.size() - 1, preset, false))
        {
            if (!activate_preset(plugins.size() - 1, preset, true))
            {
                fprintf(stderr, "Unknown preset: %s\n", preset.c_str());
            }
        }
    }
}

void host_session::create_plugins_from_list()
{
    for (unsigned int i = 0; i < plugin_names.size(); i++) {
        add_plugin(plugin_names[i], presets.count(i) ? presets[i] : string());
    }
}

void host_session::open()
{
    if (!input_name.empty()) client.input_name = input_name;
    if (!output_name.empty()) client.output_name = output_name;
    if (!midi_name.empty()) client.midi_name = midi_name;
    client.open(client_name.c_str());
    main_win->prefix = client_name + " - ";
    main_win->conditions.insert("jackhost");
    main_win->conditions.insert("directlink");
    if (!restoring_session)
        create_plugins_from_list();
    main_win->create();
    gtk_signal_connect(GTK_OBJECT(main_win->toplevel), "destroy", G_CALLBACK(destroy), NULL);
}

void host_session::new_plugin(const char *name)
{
    jack_host_base *jh = create_jack_host(name, get_next_instance_name(name));
    if (!jh)
        return;
    instances.insert(jh->instance_name);
    jh->open(&client);
    
    plugins.push_back(jh);
    client.add(jh);
    main_win->add_plugin(jh);
}

void host_session::remove_plugin(plugin_ctl_iface *plugin)
{
    for (unsigned int i = 0; i < plugins.size(); i++)
    {
        if (plugins[i] == plugin)
        {
            client.del(i);
            plugins.erase(plugins.begin() + i);
            main_win->del_plugin(plugin);
            delete plugin;
            return;
        }
    }
}

bool host_session::activate_preset(int plugin_no, const std::string &preset, bool builtin)
{
    string cur_plugin = plugins[plugin_no]->get_id();
    preset_vector &pvec = (builtin ? get_builtin_presets() : get_user_presets()).presets;
    for (unsigned int i = 0; i < pvec.size(); i++) {
        if (pvec[i].name == preset && pvec[i].plugin == cur_plugin)
        {
            pvec[i].activate(plugins[plugin_no]);
            if (gui_win && gui_win->gui)
                gui_win->gui->refresh();
            return true;
        }
    }
    return false;
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
    main_win->on_closed();
    main_win->close_guis();
    client.deactivate();
    client.delete_plugins();
    client.close();
}

#if USE_LASH

static string stripfmt(string x)
{
    if (x.length() < 2)
        return x;
    if (x.substr(x.length() - 2) != "%d")
        return x;
    return x.substr(0, x.length() - 2);
}

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
                lash_config_t *cfg = lash_config_new_with_key("global");
                dictionary tmp;
                string pstr;
                string i_name = stripfmt(client.input_name);
                string o_name = stripfmt(client.output_name);
                string m_name = stripfmt(client.midi_name);
                tmp["input_prefix"] = i_name;
                tmp["output_prefix"] = stripfmt(client.output_name);
                tmp["midi_prefix"] = stripfmt(client.midi_name);
                pstr = encode_map(tmp);
                lash_config_set_value(cfg, pstr.c_str(), pstr.length());
                lash_send_config(lash_client, cfg);
                
                for (unsigned int i = 0; i < plugins.size(); i++) {
                    jack_host_base *p = plugins[i];
                    char ss[32];
                    plugin_preset preset;
                    preset.plugin = p->get_id();
                    preset.get_from(p);
                    sprintf(ss, "Plugin%d", i);
                    pstr = preset.to_xml();
                    tmp.clear();
                    if (p->get_input_count())
                        tmp["input_name"] = p->get_inputs()[0].name.substr(i_name.length());
                    if (p->get_output_count())
                        tmp["output_name"] = p->get_outputs()[0].name.substr(o_name.length());
                    if (p->get_midi_port())
                        tmp["midi_name"] = p->get_midi_port()->name.substr(m_name.length());
                    tmp["preset"] = pstr;
                    pstr = encode_map(tmp);
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
                    if (!strcmp(key, "global"))
                    {
                        dictionary dict;
                        decode_map(dict, data);
                        if (dict.count("input_prefix")) client.input_name = dict["input_prefix"]+"%d";
                        if (dict.count("output_prefix")) client.output_name = dict["output_prefix"]+"%d";
                        if (dict.count("midi_prefix")) client.midi_name = dict["midi_prefix"]+"%d";
                    }
                    if (!strncmp(key, "Plugin", 6))
                    {
                        unsigned int nplugin = atoi(key + 6);
                        dictionary dict;
                        decode_map(dict, data);
                        data = dict["preset"];
                        if (dict.count("input_name")) client.input_nr = atoi(dict["input_name"].c_str());
                        if (dict.count("output_name")) client.output_nr = atoi(dict["output_name"].c_str());
                        if (dict.count("midi_name")) client.midi_nr = atoi(dict["midi_name"].c_str());
                        preset_list tmp;
                        tmp.parse("<presets>"+data+"</presets>");
                        if (tmp.presets.size())
                        {
                            printf("Load plugin %s\n", tmp.presets[0].plugin.c_str());
                            add_plugin(tmp.presets[0].plugin, "");
                            tmp.presets[0].activate(plugins[nplugin]);
                            main_win->refresh_plugin(plugins[nplugin]);
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
    try {
        get_builtin_presets().load_defaults(true);
        get_user_presets().load_defaults(false);
    }
    catch(calf_plugins::preset_exception &e)
    {
        // XXXKF this exception is already handled by load_defaults, so this is redundant
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
