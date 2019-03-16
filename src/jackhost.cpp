/* Calf DSP Library Utility Application - calfjackhost
 * Standalone application module wrapper example.
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
#include <jack/midiport.h>
#include <calf/host_session.h>
#include <calf/preset.h>
#include <calf/gtk_session_env.h>
#include <calf/plugin_tools.h>
#include <getopt.h>

using namespace std;
using namespace calf_utils;
using namespace calf_plugins;

const char *client_name = "calfhost";

extern "C" audio_module_iface *create_calf_plugin_by_name(const char *effect_name);

jack_host *calf_plugins::create_jack_host(jack_client *client, const char *effect_name, const std::string &instance_name, calf_plugins::progress_report_iface *priface)
{
    audio_module_iface *plugin = create_calf_plugin_by_name(effect_name);
    if (plugin != NULL)
        return new jack_host(client, plugin, effect_name, instance_name, priface);
    return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

jack_host::jack_host(jack_client *_client, audio_module_iface *_module, const std::string &_name, const std::string &_instance_name, calf_plugins::progress_report_iface *_priface)
: module(_module)
{
    name = _name;
    instance_name = _instance_name;
    
    client = _client;
    cc_mappings = NULL;
    changed = true;

    module->get_port_arrays(ins, outs, params);
    metadata = module->get_metadata_iface();
    in_count = metadata->get_input_count();
    out_count = metadata->get_output_count();
    param_count = metadata->get_param_count();
    inputs.resize(in_count);
    outputs.resize(out_count);
    param_values = new float[param_count];
    write_serials.resize(param_count);
    fill(write_serials.begin(), write_serials.end(), 0);
    last_modify_serial = 0;
    for (int i = 0; i < param_count; i++) {
        params[i] = &param_values[i];
    }
    clear_preset();
    midi_meter = 0;
    last_designator = 0xFFFFFFFF;
    module->set_progress_report_iface(_priface);
    module->post_instantiate(client->sample_rate);
}

jack_host::~jack_host()
{
    delete cc_mappings;
    cc_mappings = NULL;
    delete []param_values;
    if (client)
        destroy();
}

void jack_host::create()
{
    create_ports();    
    cache_ports();
    init_module();
    
    changed = false;
}

void jack_host::create_ports() {
    char buf[64];
    char buf2[64];
    string prefix = client->name + ":";
    port *inputs = get_inputs();
    port *outputs = get_outputs();
    int in_count = metadata->get_input_count(), out_count = metadata->get_output_count();
    for (int i=0; i<in_count; i++) {
        snprintf(buf, sizeof(buf), "%s In #%d", instance_name.c_str(), i+1);
        snprintf(buf2, sizeof(buf2), client->input_name.c_str(), client->input_nr++);
        inputs[i].nice_name = buf;
        inputs[i].name = buf2;
        inputs[i].handle = jack_port_register(client->client, buf, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput , 0);
        inputs[i].data = NULL;
        inputs[i].meter.set_falloff(0.f, client->sample_rate);
        if (!inputs[i].handle)
            throw text_exception("Could not create JACK input port");
        jack_port_set_alias(inputs[i].handle, (prefix + buf2).c_str());
    }
    if (metadata->get_midi()) {
        snprintf(buf, sizeof(buf), "%s MIDI In", instance_name.c_str());
        snprintf(buf2, sizeof(buf2), client->midi_name.c_str(), client->midi_nr++);
        midi_port.nice_name = buf;
        midi_port.name = buf2;
        midi_port.handle = jack_port_register(client->client, buf, JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
        if (!midi_port.handle)
            throw text_exception("Could not create JACK MIDI port");
        jack_port_set_alias(midi_port.handle, (prefix + buf2).c_str());
    }
    for (int i=0; i<out_count; i++) {
        snprintf(buf, sizeof(buf), "%s Out #%d", instance_name.c_str(), i+1);
        snprintf(buf2, sizeof(buf2), client->output_name.c_str(), client->output_nr++);
        outputs[i].nice_name = buf;
        outputs[i].name = buf2;
        outputs[i].handle = jack_port_register(client->client, buf, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput , 0);
        outputs[i].data = NULL;
        if (!outputs[i].handle)
            throw text_exception("Could not create JACK output port");
        jack_port_set_alias(outputs[i].handle, (prefix + buf2).c_str());
    }
}

#if JACK_HAS_RENAME
#define jack_port_rename_fn jack_port_rename
#else
#define jack_port_rename_fn(client, handle, name) jack_port_set_name(handle, name)
#endif

void jack_host::rename_ports() {
    char buf[64];
    port *inputs = get_inputs();
    port *outputs = get_outputs();
    int in_count = metadata->get_input_count(), out_count = metadata->get_output_count();
    for (int i=0; i<in_count; i++) {
        snprintf(buf, sizeof(buf), "%s In #%d", instance_name.c_str(), i+1);
        inputs[i].nice_name = buf;
        jack_port_rename_fn(client->client, inputs[i].handle, buf);
    }
    if (metadata->get_midi()) {
        snprintf(buf, sizeof(buf), "%s MIDI In", instance_name.c_str());
        midi_port.nice_name = buf;
        jack_port_rename_fn(client->client, midi_port.handle, buf);
    }
    for (int i=0; i<out_count; i++) {
        snprintf(buf, sizeof(buf), "%s Out #%d", instance_name.c_str(), i+1);
        outputs[i].nice_name = buf;
        jack_port_rename_fn(client->client, outputs[i].handle, buf);
        // replacement:
        // jack_port_rename(client->client, (outputs[i].handle, buf);
        // but we have to check which jack version first
    }
}

void jack_host::rename(std::string name) {
    instance_name = name;
    rename_ports();
}

void jack_host::handle_automation_cc(uint32_t designator, int value)
{
    last_designator = designator;
    if (!cc_mappings)
        return;
    automation_map::const_iterator i = cc_mappings->find(designator);
    while (i != cc_mappings->end() && i->first == designator)
    {
        const automation_range &r = i->second;
        const parameter_properties *props = metadata->get_param_props(r.param_no);
        set_param_value(r.param_no, props->from_01(r.min_value + value * (r.max_value - r.min_value)/ 127.0));
        write_serials[r.param_no] = ++last_modify_serial;
        ++i;
    }
}

uint32_t jack_host::get_last_automation_source()
{
    return last_designator;
}


void jack_host::handle_event(uint8_t *buffer, uint32_t size)
{
    int channel = (buffer[0] & 0x0f) + 1;
    int value;
    switch(buffer[0] >> 4)
    {
    case 8:
        module->note_off(channel, buffer[1], buffer[2]);
        break;
    case 9:
        if (!buffer[2])
            module->note_off(channel, buffer[1], 0);
        else
            module->note_on(channel, buffer[1], buffer[2]);
        break;
    case 11:
        module->control_change(channel, buffer[1], buffer[2]);
        break;
    case 12:
        module->program_change(channel, buffer[1]);
        break;
    case 13:
        module->channel_pressure(channel, buffer[1]);
        break;
    case 14:
        value = buffer[1] + 128 * buffer[2] - 8192;
        module->pitch_bend(channel, value);
        break;
    }
}

void jack_host::destroy()
{
    port *inputs = get_inputs(), *outputs = get_outputs();
    int input_count = metadata->get_input_count(), output_count = metadata->get_output_count();
    for (int i = 0; i < input_count; i++) {
        jack_port_unregister(client->client, inputs[i].handle);
        inputs[i].data = NULL;
    }
    for (int i = 0; i < output_count; i++) {
        jack_port_unregister(client->client, outputs[i].handle);
        outputs[i].data = NULL;
    }
    if (metadata->get_midi())
        jack_port_unregister(client->client, midi_port.handle);
    client = NULL;
}

void jack_host::process_part(unsigned int time, unsigned int len)
{
    if (!len)
        return;
    for (int i = 0; i < in_count; i++)
        inputs[i].meter.update(ins[i] + time, len);
    unsigned int mask = module->process_slice(time, time + len);
    for (int i = 0; i < out_count; i++)
    {
        if (!(mask & (1 << i))) {
            dsp::zero(outs[i] + time, len);
            outputs[i].meter.update_zeros(len);
        } else
            outputs[i].meter.update(outs[i] + time, len);
    }
    // decay linearly for 0.1s
    float new_meter = midi_meter - len / (0.1 * client->sample_rate);
    if (new_meter < 0)
        new_meter = 0;
    midi_meter = new_meter;
}

float jack_host::get_level(unsigned int port)
{ 
    if (port < (unsigned)in_count)
        return inputs[port].meter.level;
    port -= in_count;
    if (port < (unsigned)out_count)
        return outputs[port].meter.level;
    port -= out_count;
    if (port == 0 && metadata->get_midi())
        return midi_meter;
    return 0.f;
}

int jack_host::process(jack_nframes_t nframes, automation_iface &automation)
{
    for (int i=0; i<in_count; i++) {
        ins[i] = inputs[i].data = (float *)jack_port_get_buffer(inputs[i].handle, nframes);
    }
    if (metadata->get_midi())
        midi_port.data = (float *)jack_port_get_buffer(midi_port.handle, nframes);
    if (changed) {
        module->params_changed();
        changed = false;
    }

    unsigned int time = 0;
    if (metadata->get_midi())
    {
        jack_midi_event_t event;
        int count = jack_midi_get_event_count(midi_port.data NFRAMES_MAYBE(nframes));
        for (int i = 0; i < count; i++)
        {
            jack_midi_event_get(&event, midi_port.data, i NFRAMES_MAYBE(nframes));
            uint32_t endtime = automation.apply_and_adjust(time, event.time);
            unsigned int len = endtime - time;
            process_part(time, len);
            
            midi_meter = 1.f;
            handle_event(event.buffer, event.size);
            
            time = event.time;
        }
    }
    while(time < nframes)
    {
        uint32_t endtime = automation.apply_and_adjust(time, nframes);
        process_part(time, endtime - time);
        time = endtime;
    }
    module->params_reset();
    return 0;
}

void jack_host::init_module()
{
    module->set_sample_rate(client->sample_rate);
    module->activate();
    module->params_changed();
}

void jack_host::cache_ports()
{
    for (int i=0; i<out_count; i++) {
        outs[i] = outputs[i].data = (float *)jack_port_get_buffer(outputs[i].handle, 0);
    }
}

void jack_host::get_all_input_ports(std::vector<port *> &ports)
{
    for (int i = 0; i < in_count; i++)
        ports.push_back(&inputs[i]);
    if (metadata->get_midi())
        ports.push_back(&midi_port);
}

void jack_host::get_all_output_ports(std::vector<port *> &ports)
{
    for (int i = 0; i < out_count; i++)
        ports.push_back(&outputs[i]);
}

static void remove_mapping(automation_map &amap, uint32_t source, int param_no)
{
    for(automation_map::iterator i = amap.find(source); i != amap.end() && i->first == source; )
    {
        automation_map::iterator j = i;
        ++j;
        if (i->second.param_no == param_no)
            amap.erase(i);
        i = j;
    }
}

void jack_host::add_automation(uint32_t source, const automation_range &dest)
{
    automation_map *amap = new automation_map;
    if (cc_mappings)
        amap->insert(cc_mappings->begin(), cc_mappings->end());
    remove_mapping(*amap, source, dest.param_no);
    amap->insert(make_pair(source, dest));
    replace_automation_map(amap);
}

void jack_host::delete_automation(uint32_t source, int param_no)
{
    automation_map *amap = new automation_map;
    if (cc_mappings)
        amap->insert(cc_mappings->begin(), cc_mappings->end());
    remove_mapping(*amap, source, param_no);
    replace_automation_map(amap);
}

void jack_host::replace_automation_map(automation_map *amap)
{
    client->atomic_swap(cc_mappings, amap);
    delete amap;
}

void jack_host::get_automation(int param_no, multimap<uint32_t, automation_range> &dests)
{
    dests.clear();
    if (!cc_mappings)
        return;
    for(automation_map::iterator i = cc_mappings->begin(); i != cc_mappings->end(); ++i)
    {
        if (param_no == -1 || param_no == i->second.param_no)
            dests.insert(*i);
    }
}

void jack_host::send_automation_configures(send_configure_iface *sci)
{
    if (!cc_mappings)
        return;
    for(automation_map::iterator i = cc_mappings->begin(); i != cc_mappings->end(); ++i)
    {
        i->second.send_configure(metadata, i->first, sci);
    }
}

char *jack_host::configure(const char *key, const char *value)
{
    uint32_t controller;
    automation_range *ar = automation_range::new_from_configure(metadata, key, value, controller);
    if (ar)
    {
        add_automation(controller, *ar);
        delete ar;
        return NULL;
    }
    return module->configure(key, value);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static const char *short_options = "c:i:l:o:m:M:s:S:ehvLnt";

static struct option long_options[] = {
    {"help", 0, 0, 'h'},
    {"version", 0, 0, 'v'},
    {"client", 1, 0, 'c'},
    {"effect", 0, 0, 'e'},
    {"load", 1, 0, 'l'},
    {"input", 1, 0, 'i'},
    {"output", 1, 0, 'o'},
    {"midi", 1, 0, 'm'},
    {"state", 1, 0, 's'},
    {"connect-midi", 1, 0, 'M'},
    {"session-id", 1, 0, 'S'},
    {"list", 0, 0, 'L'},
    {"no-gui", 0, 0, 'n'},
    {"no-tray", 0, 0, 't'},
    {0,0,0,0},
};

void print_help(char *argv[])
{
    printf("JACK host for Calf effects\n"
        "Syntax: %s [--client, -c <name>] [--input, -i <name>] [--output, -o <name>] [--midi, -m <name>] [--load|state, -l|s <session>]\n"
        "       [--connect-midi, -M <name|capture-index>] [--help, -h] [--version, -v] [--list, -L] [--no-tray, -t]\n"
        "       [!] pluginname[:<preset>] [!] ...\n", 
        argv[0]);
}

int main(int argc, char *argv[])
{
#if !GLIB_CHECK_VERSION(2, 36, 0)
    g_type_init();
#endif
#if !GLIB_CHECK_VERSION(2, 32, 0)
    if (!g_thread_supported()) g_thread_init(NULL);
#endif
    
    host_session sess(new gtk_session_environment());
    if (argc > 0 && argv[0] && argv[0] != sess.calfjackhost_cmd)
    {
        char *path = realpath(argv[0], NULL);
        sess.calfjackhost_cmd = path;
        free(path);
    }
    
    // Scan the options for the first time to find switches like --help, -h or -?
    // This avoids starting communication with LASH when displaying help text.
    while(1)
    {
        int option_index;
        int c = getopt_long(argc, argv, short_options, long_options, &option_index);
        if (c == -1)
            break;
        if (c == 'h' || c == '?')
        {
            print_help(argv);
            return 0;
        }
    }
    // Rewind options to start
    optind = 1;
    
#if USE_LASH
    sess.session_manager = create_lash_session_mgr(&sess, argc, argv);
#else
    sess.session_manager = NULL;
#endif
    while(1)
    {
        int option_index;
        int c = getopt_long(argc, argv, short_options, long_options, &option_index);
        if (c == -1)
            break;
        switch(c) {
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
            case 'S':
                sess.jack_session_id = optarg;
                break;
            case 'n':
                sess.has_gui = false;
                sess.has_trayicon = false;
                break;
            case 't':
                sess.has_trayicon = false;
                break;
            case 'l':
            case 's':
            {
                if (!g_path_is_absolute(optarg))
                {
                    gchar *curdir = g_get_current_dir();
                    gchar *str = g_build_filename(curdir, optarg, NULL);
                    sess.load_name = str;
                    g_free(str);
                    g_free(curdir);
                }
                else
                    sess.load_name = optarg;
                sess.only_load_if_exists = (c == 's');
                break;
            }
            case 'M':
                if (atoi(optarg)) {
                    sess.autoconnect_midi_index = atoi(optarg);
                }
                else
                    sess.autoconnect_midi = string(optarg);
                break;
            case 'L':
                string s = 
                #define PER_MODULE_ITEM(name, isSynth, jackname) jackname " "
                #include <calf/modulelist.h>
                ;
                if (!s.empty())
                    s = s.substr(0, s.length() - 1);
                printf("%s\n", s.c_str());
                return 0;
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
        if(sess.has_gui){
            sess.session_env->init_gui(argc, argv);
        }
        sess.open();
        sess.connect();
        sess.client.activate();
        sess.set_signal_handlers();
        if (sess.has_gui) {
            sess.session_env->start_gui_loop();
        } else {
            while (sess.quit_on_next_idle_call == 0){
                sleep(1);
            }   
        }
        sess.close();
    }
    catch(std::exception &e)
    {
        fprintf(stderr, "%s\n", e.what());
        exit(1);
    }
    return 0;
}
