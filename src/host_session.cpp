/* Calf DSP Library Utility Application - calfjackhost
 * A class that contains a JACK host session
 *
 * Copyright (C) 2007-2011 Krzysztof Foltman
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

#include <calf/giface.h>
#include <calf/host_session.h>
#include <calf/gui.h>
#include <calf/preset.h>
#include <getopt.h>
#include <sys/stat.h>

using namespace std;
using namespace calf_utils;
using namespace calf_plugins;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

host_session *host_session::instance = NULL;

host_session::host_session(session_environment_iface *se)
{
    client_name = "calf";
    session_env = se;
    autoconnect_midi_index = -1;
    gui_win = NULL;
    session_manager = NULL;
    only_load_if_exists = false;
    save_file_on_next_idle_call = false;
    quit_on_next_idle_call = 0;

    main_win = session_env->create_main_window();
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

void host_session::add_plugin(string name, string preset, string instance_name)
{
    if (instance_name.empty())
        instance_name = get_next_instance_name(name);
    jack_host *jh = create_jack_host(name.c_str(), instance_name, main_win);
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
    jh->create(&client);
    
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

void host_session::on_main_window_destroy()
{
    session_env->quit_gui_loop();
}

void host_session::open()
{
    if (!input_name.empty()) client.input_name = input_name;
    if (!output_name.empty()) client.output_name = output_name;
    if (!midi_name.empty()) client.midi_name = midi_name;
    
    client.open(client_name.c_str());
    main_win->add_condition("jackhost");
    main_win->add_condition("directlink");
    main_win->add_condition("configure");
    if (!session_manager || !session_manager->is_being_restored()) 
        create_plugins_from_list();
    main_win->create();
}

void host_session::new_plugin(const char *name)
{
    jack_host *jh = create_jack_host(name, get_next_instance_name(name), main_win);
    if (!jh)
        return;
    instances.insert(jh->instance_name);
    jh->create(&client);
    
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
            instances.erase(plugins[i]->instance_name);
            client.del(plugins[i]);
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
        jack_host *plugin = plugins[0];
        client.del(plugins[0]);
        plugins.erase(plugins.begin());
        main_win->del_plugin(plugin);
        delete plugin;
    }
    instances.clear();
}

bool host_session::activate_preset(int plugin_no, const std::string &preset, bool builtin)
{
    string cur_plugin = plugins[plugin_no]->metadata->get_id();
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
    if (session_manager)
        session_manager->set_jack_client_name(client.get_name());
    if ((!session_manager || !session_manager->is_being_restored()) && load_name.empty())
    {
        string cnp = client.get_name() + ":";
        for (unsigned int i = 0; i < plugins.size(); i++) {
            if (chains.count(i)) {
                if (!i)
                {
                    if (plugins[0]->metadata->get_input_count() < 2)
                    {
                        fprintf(stderr, "Cannot connect input to plugin %s - the plugin no input ports\n", plugins[0]->name.c_str());
                    } else {
                        client.connect("system:capture_1", cnp + plugins[0]->get_inputs()[0].name);
                        client.connect("system:capture_2", cnp + plugins[0]->get_inputs()[1].name);
                    }
                }
                else
                {
                    if (plugins[i - 1]->metadata->get_output_count() < 2 || plugins[i]->metadata->get_input_count() < 2)
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
            if (plugins[last]->metadata->get_output_count() < 2)
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
                if (plugins[i]->metadata->get_midi())
                    client.connect(autoconnect_midi, cnp + plugins[i]->get_midi_port()->name);
            }
        }
        else
        if (autoconnect_midi_index != -1) {
            const char **ports = client.get_ports(".*:.*", JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput | JackPortIsPhysical);
            for (int j = 0; ports && ports[j]; j++)
            {
                if (j + 1 == autoconnect_midi_index) {
                    for (unsigned int i = 0; i < plugins.size(); i++)
                    {
                        if (plugins[i]->metadata->get_midi())
                            client.connect(ports[j], cnp + plugins[i]->get_midi_port()->name);
                    }
                    break;
                }
            }
        }
    }
    if (!load_name.empty())
    {
        char *error = open_file(load_name.c_str());
        if (error)
        {
            bool suppress_error = false;
            if (only_load_if_exists)
            {
                struct stat s;
                int stat_result = stat(load_name.c_str(), &s);
                if (stat_result == -1 && errno == ENOENT)
                    suppress_error = true;
            }
            // If the file is optional and it didn't exist, suppress the error
            if (!suppress_error)
            {
                main_win->show_error("Cannot load '" + load_name + "': " + error);
                
                load_name = "";
            }
            g_free(error);
        }
        set_current_filename(load_name);
    }
    if (session_manager)
        session_manager->connect("calf-" + client_name);
}

void host_session::close()
{
    if (session_manager)
        session_manager->disconnect();
    main_win->on_closed();
    client.deactivate();
    client.delete_plugins();
    client.close();
}

static string stripfmt(string x)
{
    if (x.length() < 2)
        return x;
    if (x.substr(x.length() - 2) != "%d")
        return x;
    return x.substr(0, x.length() - 2);
}

char *host_session::open_file(const char *name)
{
    preset_list pl;
    try {
        remove_all_plugins();
        pl.load(name, true);
        printf("Size %d\n", (int)pl.plugins.size());
        for (unsigned int i = 0; i < pl.plugins.size(); i++)
        {
            preset_list::plugin_snapshot &ps = pl.plugins[i];
            client.input_nr = ps.input_index;
            client.output_nr = ps.output_index;
            client.midi_nr = ps.midi_index;
            printf("Loading %s\n", ps.type.c_str());
            if (ps.preset_offset < (int)pl.presets.size())
            {
                add_plugin(ps.type, "", ps.instance_name);
                pl.presets[ps.preset_offset].activate(plugins[i]);
                main_win->refresh_plugin(plugins[i]);
            }
        }
    }
    catch(preset_exception &e)
    {
        // XXXKF this will leak
        char *data = strdup(e.what());
        return data;
    }
    
    return NULL;
}

char *host_session::save_file(const char *name)
{
    string i_name = stripfmt(client.input_name);
    string o_name = stripfmt(client.output_name);
    string m_name = stripfmt(client.midi_name);
    string data;
    data = "<?xml version=\"1.1\" encoding=\"utf-8\">\n";
    data = "<rack>\n";
    for (unsigned int i = 0; i < plugins.size(); i++) {
        jack_host *p = plugins[i];
        plugin_preset preset;
        preset.plugin = p->metadata->get_id();
        preset.get_from(p);
        data += "<plugin";
        data += to_xml_attr("type", preset.plugin);
        data += to_xml_attr("instance-name", p->instance_name);
        if (p->metadata->get_input_count())
            data += to_xml_attr("input-index", p->get_inputs()[0].name.substr(i_name.length()));
        if (p->metadata->get_output_count())
            data += to_xml_attr("output-index", p->get_outputs()[0].name.substr(o_name.length()));
        if (p->get_midi_port())
            data += to_xml_attr("midi-index", p->get_midi_port()->name.substr(m_name.length()));
        data += ">\n";
        data += preset.to_xml();
        data += "</plugin>\n";
    }
    data += "</rack>\n";
    FILE *f = fopen(name, "w");
    if (!f || 1 != fwrite(data.c_str(), data.length(), 1, f))
    {
        int e = errno;
        if (f)
            fclose(f);
        return strdup(strerror(e));
    }
    if (fclose(f))
        return strdup(strerror(errno));
    
    return NULL;
}

void host_session::load(session_load_iface *stream)
{
    // printf("!!!Restore data set!!!\n");
    remove_all_plugins();
    string key, data;
    while(stream->get_next_item(key, data)) {
        if (key == "global")
        {
            dictionary dict;
            decode_map(dict, data);
            if (dict.count("input_prefix")) client.input_name = dict["input_prefix"]+"%d";
            if (dict.count("output_prefix")) client.output_name = dict["output_prefix"]+"%d";
            if (dict.count("midi_prefix")) client.midi_name = dict["midi_prefix"]+"%d";
        }
        if (!strncmp(key.c_str(), "Plugin", 6))
        {
            unsigned int nplugin = atoi(key.c_str() + 6);
            dictionary dict;
            decode_map(dict, data);
            data = dict["preset"];
            string instance_name;
            if (dict.count("instance_name")) instance_name = dict["instance_name"];
            if (dict.count("input_name")) client.input_nr = atoi(dict["input_name"].c_str());
            if (dict.count("output_name")) client.output_nr = atoi(dict["output_name"].c_str());
            if (dict.count("midi_name")) client.midi_nr = atoi(dict["midi_name"].c_str());
            preset_list tmp;
            tmp.parse("<presets>"+data+"</presets>", false);
            if (tmp.presets.size())
            {
                printf("Load plugin %s\n", tmp.presets[0].plugin.c_str());
                add_plugin(tmp.presets[0].plugin, "", instance_name);
                tmp.presets[0].activate(plugins[nplugin]);
                main_win->refresh_plugin(plugins[nplugin]);
            }
        }
    }
}

void host_session::save(session_save_iface *stream)
{
    dictionary tmp;
    string pstr;
    string i_name = stripfmt(client.input_name);
    string o_name = stripfmt(client.output_name);
    string m_name = stripfmt(client.midi_name);
    tmp["input_prefix"] = i_name;
    tmp["output_prefix"] = stripfmt(client.output_name);
    tmp["midi_prefix"] = stripfmt(client.midi_name);
    pstr = encode_map(tmp);
    stream->write_next_item("global", pstr);
    
    for (unsigned int i = 0; i < plugins.size(); i++) {
        jack_host *p = plugins[i];
        char ss[32];
        plugin_preset preset;
        preset.plugin = p->metadata->get_id();
        preset.get_from(p);
        sprintf(ss, "Plugin%d", i);
        pstr = preset.to_xml();
        tmp.clear();
        tmp["instance_name"] = p->instance_name;
        if (p->metadata->get_input_count())
            tmp["input_name"] = p->get_inputs()[0].name.substr(i_name.length());
        if (p->metadata->get_output_count())
            tmp["output_name"] = p->get_outputs()[0].name.substr(o_name.length());
        if (p->get_midi_port())
            tmp["midi_name"] = p->get_midi_port()->name.substr(m_name.length());
        tmp["preset"] = pstr;
        pstr = encode_map(tmp);
        stream->write_next_item(ss, pstr);
    }
}

void host_session::signal_handler(int signum)
{
    switch (signum)
    {
    case SIGUSR1:
        instance->save_file_on_next_idle_call = true;
        break;
    case SIGTERM:
    case SIGHUP:
        instance->quit_on_next_idle_call = signum;
        break;
    }
}

void host_session::on_idle()
{
    if (save_file_on_next_idle_call)
    {
        save_file_on_next_idle_call = false;
        main_win->save_file();
        printf("LADISH Level 1 support: file '%s' saved\n", get_current_filename().c_str());
    }

    if (quit_on_next_idle_call > 0)
    {
        printf("Quit requested through signal %d\n", quit_on_next_idle_call);
        quit_on_next_idle_call = -quit_on_next_idle_call; // mark the event as handled but preserve signal number
        session_env->quit_gui_loop();
    }
}

void host_session::set_signal_handlers()
{
    instance = this;

    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGHUP,  &sa, NULL);
    sigaction(SIGUSR1, &sa, NULL);
}

void host_session::reorder_plugins()
{
    vector<int> order;
    client.calculate_plugin_order(order);
    client.apply_plugin_order(order);
}

std::string host_session::get_client_name() const
{
    return client.name;
}

std::string host_session::get_current_filename() const
{
    return current_filename;
}

void host_session::set_current_filename(const std::string &name)
{
    current_filename = name;
}
