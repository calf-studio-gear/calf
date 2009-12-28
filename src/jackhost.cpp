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
#include <stdio.h>
using namespace std;
using namespace calf_utils;
using namespace calf_plugins;

// I don't need anyone to tell me this is stupid. I already know that :)
plugin_gui_window *gui_win;

const char *client_name = "calfhost";

#if USE_LASH_0_6
static bool save_data_set_cb(lash_config_handle_t *handle, void *user_data);
static bool load_data_set_cb(lash_config_handle_t *handle, void *user_data);
static bool quit_cb(void *user_data);
#endif

jack_host_base *calf_plugins::create_jack_host(const char *effect_name, const std::string &instance_name, calf_plugins::progress_report_iface *priface)
{
    #define PER_MODULE_ITEM(name, isSynth, jackname) if (!strcasecmp(effect_name, jackname)) return new jack_host<name##_audio_module>(effect_name, instance_name, priface);
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
    string prefix = client->name + ":";
    static const char *suffixes[] = { "l", "r", "2l", "2r" };
    port *inputs = get_inputs();
    port *outputs = get_outputs();
    int in_count = get_input_count(), out_count = get_output_count();
    for (int i=0; i<in_count; i++) {
        sprintf(buf, "%s_in_%s", instance_name.c_str(), suffixes[i]);
        sprintf(buf2, client->input_name.c_str(), client->input_nr++);
        inputs[i].name = buf2;
        inputs[i].handle = jack_port_register(client->client, buf, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput , 0);
        inputs[i].data = NULL;
        if (!inputs[i].handle)
            throw text_exception("Could not create JACK input port");
        jack_port_set_alias(inputs[i].handle, (prefix + buf2).c_str());
    }
    if (get_midi()) {
        sprintf(buf, "%s_midi_in", instance_name.c_str());
        sprintf(buf2, client->midi_name.c_str(), client->midi_nr++);
        midi_port.name = buf2;
        midi_port.handle = jack_port_register(client->client, buf, JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
        if (!midi_port.handle)
            throw text_exception("Could not create JACK MIDI port");
        jack_port_set_alias(midi_port.handle, (prefix + buf2).c_str());
    }
    for (int i=0; i<out_count; i++) {
        sprintf(buf, "%s_out_%s", instance_name.c_str(), suffixes[i]);
        sprintf(buf2, client->output_name.c_str(), client->output_nr++);
        outputs[i].name = buf2;
        outputs[i].handle = jack_port_register(client->client, buf, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput , 0);
        outputs[i].data = NULL;
        if (!outputs[i].handle)
            throw text_exception("Could not create JACK output port");
        jack_port_set_alias(outputs[i].handle, (prefix + buf2).c_str());
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
    {"input", 1, 0, 'i'},
    {"output", 1, 0, 'o'},
    {"connect-midi", 1, 0, 'M'},
    {0,0,0,0},
};

void print_help(char *argv[])
{
    printf("JACK host for Calf effects\n"
        "Syntax: %s [--client <name>] [--input <name>] [--output <name>] [--midi <name>]\n"
        "       [--connect-midi <name|capture-index>] [--help] [--version] [!] pluginname[:<preset>] [!] ...\n", 
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

struct host_session: public main_window_owner_iface, public calf_plugins::progress_report_iface
{
    string client_name, input_name, output_name, midi_name;
    vector<string> plugin_names;
    map<int, string> presets;
#if USE_LASH
    int lash_source_id;
    lash_client_t *lash_client;
# if !USE_LASH_0_6
    lash_args_t *lash_args;
# endif
#endif
    
    // these are not saved
    jack_client client;
    string autoconnect_midi;
    int autoconnect_midi_index;
    set<int> chains;
    vector<jack_host_base *> plugins;
    main_window *main_win;
    bool restoring_session;
    std::set<std::string> instances;
    GtkWidget *progress_window;
    
    host_session();
    void open();
    void add_plugin(string name, string preset, string instance_name = string());
    void create_plugins_from_list();
    void connect();
    void close();
    bool activate_preset(int plugin, const std::string &preset, bool builtin);
#if USE_LASH
    static gboolean update_lash(void *self) { ((host_session *)self)->update_lash(); return TRUE; }
    void update_lash();
# if !USE_LASH_0_6
    void send_lash(LASH_Event_Type type, const std::string &data) {
        lash_send_event(lash_client, lash_event_new_with_all(type, data.c_str()));
    }
# endif
#endif
    virtual void new_plugin(const char *name);    
    virtual void remove_plugin(plugin_ctl_iface *plugin);
    void remove_all_plugins();
    std::string get_next_instance_name(const std::string &effect_name);
    
    /// Create a toplevel window with progress bar
    GtkWidget *create_progress_window();
    /// Implementation of progress_report_iface function
    void report_progress(float percentage, const std::string &message);
    
    /// Implementation of open file functionality (TODO)
    virtual const char *open_file(const char *name) { return "Not implemented yet"; }
    /// Implementation of save file functionality (TODO)
    virtual const char *save_file(const char *name) { return "Not implemented yet"; }
};

host_session::host_session()
{
    client_name = "calf";
#if USE_LASH
    lash_source_id = 0;
    lash_client = NULL;
# if !USE_LASH_0_6
    lash_args = NULL;
# endif
#endif
    restoring_session = false;
    main_win = new main_window;
    main_win->set_owner(this);
    progress_window = NULL;
    autoconnect_midi_index = -1;
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

void host_session::add_plugin(string name, string preset, string instance_name)
{
    if (instance_name.empty())
        instance_name = get_next_instance_name(name);
    jack_host_base *jh = create_jack_host(name.c_str(), instance_name, this);
    if (!jh) {
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

void host_session::report_progress(float percentage, const std::string &message)
{
    if (percentage < 100)
    {
        if (!progress_window) {
            progress_window = create_progress_window();
            gtk_window_set_modal (GTK_WINDOW (progress_window), TRUE);
            if (main_win->toplevel)
                gtk_window_set_transient_for (GTK_WINDOW (progress_window), main_win->toplevel);
        }
        gtk_widget_show(progress_window);
        GtkWidget *pbar = gtk_bin_get_child (GTK_BIN (progress_window));
        if (!message.empty())
            gtk_progress_bar_set_text (GTK_PROGRESS_BAR (pbar), message.c_str());
        gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (pbar), percentage / 100.0);
    }
    else
    {
        if (progress_window) {
            gtk_window_set_modal (GTK_WINDOW (progress_window), FALSE);
            gtk_widget_destroy (progress_window);
            progress_window = NULL;
        }
    }
    
    while (gtk_events_pending ())
        gtk_main_iteration ();
}


void host_session::create_plugins_from_list()
{
    for (unsigned int i = 0; i < plugin_names.size(); i++) {
        add_plugin(plugin_names[i], presets.count(i) ? presets[i] : string());
    }
}

GtkWidget *host_session::create_progress_window()
{
    GtkWidget *tlw = gtk_window_new ( GTK_WINDOW_TOPLEVEL );
    gtk_window_set_type_hint (GTK_WINDOW (tlw), GDK_WINDOW_TYPE_HINT_DIALOG);
    GtkWidget *pbar = gtk_progress_bar_new();
    gtk_container_add (GTK_CONTAINER(tlw), pbar);
    gtk_widget_show_all (pbar);
    return tlw;
}

void host_session::open()
{
    if (!input_name.empty()) client.input_name = input_name;
    if (!output_name.empty()) client.output_name = output_name;
    if (!midi_name.empty()) client.midi_name = midi_name;
    
    gtk_rc_parse(PKGLIBDIR "calf.rc");
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
    jack_host_base *jh = create_jack_host(name, get_next_instance_name(name), this);
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

void host_session::remove_all_plugins()
{
    while(!plugins.empty())
    {
        plugin_ctl_iface *plugin = plugins[0];
        client.del(0);
        plugins.erase(plugins.begin());
        main_win->del_plugin(plugin);
        delete plugin;
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
#if USE_LASH && !USE_LASH_0_6
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
        else
        if (autoconnect_midi_index != -1) {
            const char **ports = client.get_ports("(system|alsa_pcm):.*", JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput);
            for (int j = 0; ports && ports[j]; j++)
            {
                if (j + 1 == autoconnect_midi_index) {
                    for (unsigned int i = 0; i < plugins.size(); i++)
                    {
                        if (plugins[i]->get_midi())
                            client.connect(ports[j], cnp + plugins[i]->get_midi_port()->name);
                    }
                    break;
                }
            }
        }
    }
#if USE_LASH
    if (lash_client)
    {
# if !USE_LASH_0_6
        send_lash(LASH_Client_Name, "calf-"+client_name);
# endif
        lash_source_id = g_timeout_add_full(G_PRIORITY_LOW, 250, update_lash, this, NULL); // 4 LASH reads per second... should be enough?
    }
#endif
}

void host_session::close()
{
#if USE_LASH
    if (lash_source_id)
        g_source_remove(lash_source_id);
#endif
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

# if !USE_LASH_0_6

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
                    tmp["instance_name"] = p->instance_name;
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
                remove_all_plugins();
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
                        string instance_name;
                        if (dict.count("instance_name")) instance_name = dict["instance_name"];
                        if (dict.count("input_name")) client.input_nr = atoi(dict["input_name"].c_str());
                        if (dict.count("output_name")) client.output_nr = atoi(dict["output_name"].c_str());
                        if (dict.count("midi_name")) client.midi_nr = atoi(dict["midi_name"].c_str());
                        preset_list tmp;
                        tmp.parse("<presets>"+data+"</presets>");
                        if (tmp.presets.size())
                        {
                            printf("Load plugin %s\n", tmp.presets[0].plugin.c_str());
                            add_plugin(tmp.presets[0].plugin, "", instance_name);
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

# else

void host_session::update_lash()
{
    lash_dispatch(lash_client);
}

bool save_data_set_cb(lash_config_handle_t *handle, void *user_data)
{
    host_session *sess = static_cast<host_session *>(user_data);
    dictionary tmp;
    string pstr;
    string i_name = stripfmt(sess->client.input_name);
    string o_name = stripfmt(sess->client.output_name);
    string m_name = stripfmt(sess->client.midi_name);
    tmp["input_prefix"] = i_name;
    tmp["output_prefix"] = stripfmt(sess->client.output_name);
    tmp["midi_prefix"] = stripfmt(sess->client.midi_name);
    pstr = encode_map(tmp);
    lash_config_write_raw(handle, "global", pstr.c_str(), pstr.length());
    for (unsigned int i = 0; i < sess->plugins.size(); i++) {
        jack_host_base *p = sess->plugins[i];
        char ss[32];
        plugin_preset preset;
        preset.plugin = p->get_id();
        preset.get_from(p);
        sprintf(ss, "Plugin%d", i);
        pstr = preset.to_xml();
        tmp.clear();
        tmp["instance_name"] = p->instance_name;
        if (p->get_input_count())
            tmp["input_name"] = p->get_inputs()[0].name.substr(i_name.length());
        if (p->get_output_count())
            tmp["output_name"] = p->get_outputs()[0].name.substr(o_name.length());
        if (p->get_midi_port())
            tmp["midi_name"] = p->get_midi_port()->name.substr(m_name.length());
        tmp["preset"] = pstr;
        pstr = encode_map(tmp);
        lash_config_write_raw(handle, ss, pstr.c_str(), pstr.length());
    }
    return true;
}

bool load_data_set_cb(lash_config_handle_t *handle, void *user_data)
{
    host_session *sess = static_cast<host_session *>(user_data);
    int size, type;
    const char *key;
    const char *value;
    sess->remove_all_plugins();
    while((size = lash_config_read(handle, &key, (void *)&value, &type))) {
        if (size == -1 || type != LASH_TYPE_RAW)
            continue;
        string data = string(value, size);
        if (!strcmp(key, "global"))
        {
            dictionary dict;
            decode_map(dict, data);
            if (dict.count("input_prefix")) sess->client.input_name = dict["input_prefix"]+"%d";
            if (dict.count("output_prefix")) sess->client.output_name = dict["output_prefix"]+"%d";
            if (dict.count("midi_prefix")) sess->client.midi_name = dict["midi_prefix"]+"%d";
        } else if (!strncmp(key, "Plugin", 6)) {
            unsigned int nplugin = atoi(key + 6);
            dictionary dict;
            decode_map(dict, data);
            data = dict["preset"];
            string instance_name;
            if (dict.count("instance_name")) instance_name = dict["instance_name"];
            if (dict.count("input_name")) sess->client.input_nr = atoi(dict["input_name"].c_str());
            if (dict.count("output_name")) sess->client.output_nr = atoi(dict["output_name"].c_str());
            if (dict.count("midi_name")) sess->client.midi_nr = atoi(dict["midi_name"].c_str());
            preset_list tmp;
            tmp.parse("<presets>"+data+"</presets>");
            if (tmp.presets.size())
            {
                printf("Load plugin %s\n", tmp.presets[0].plugin.c_str());
                sess->add_plugin(tmp.presets[0].plugin, "", instance_name);
                tmp.presets[0].activate(sess->plugins[nplugin]);
                sess->main_win->refresh_plugin(sess->plugins[nplugin]);
            }
        }
    }
    return true;
}

bool quit_cb(void *user_data)
{
    gtk_main_quit();
    return true;
}

# endif
#endif

host_session current_session;

int main(int argc, char *argv[])
{
    host_session &sess = current_session;
    gtk_rc_add_default_file(PKGLIBDIR "calf.rc");
    gtk_init(&argc, &argv);
    
#if USE_LASH
# if !USE_LASH_0_6
    for (int i = 1; i < argc; i++)
    {
        if (!strncmp(argv[i], "--lash-project=", 14)) {
            sess.restoring_session = true;
            break;
        }
    }
    sess.lash_args = lash_extract_args(&argc, &argv);
    sess.lash_client = lash_init(sess.lash_args, PACKAGE_NAME, LASH_Config_Data_Set, LASH_PROTOCOL(2, 0));
# else
    sess.lash_client = lash_client_open(PACKAGE_NAME, LASH_Config_Data_Set, argc, argv);
    sess.restoring_session = lash_client_is_being_restored(sess.lash_client);
    lash_set_save_data_set_callback(sess.lash_client, save_data_set_cb, &sess);
    lash_set_load_data_set_callback(sess.lash_client, load_data_set_cb, &sess);
    lash_set_quit_callback(sess.lash_client, quit_cb, NULL);
# endif
    if (!sess.lash_client) {
        g_warning("Warning: failed to create a LASH connection");
    }
#endif
    glade_init();
    while(1) {
        int option_index;
        int c = getopt_long(argc, argv, "c:i:o:m:M:ehv", long_options, &option_index);
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
                if (atoi(optarg)) {
                    sess.autoconnect_midi_index = atoi(optarg);
                }
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
            string plugname = argv[optind++];
            size_t pos = plugname.find(":");
            if (pos != string::npos) {
                sess.presets[sess.plugin_names.size()] = plugname.substr(pos + 1);
                plugname = plugname.substr(0, pos);
            }
            sess.plugin_names.push_back(plugname);
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
        
#if USE_LASH && !USE_LASH_0_6
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
