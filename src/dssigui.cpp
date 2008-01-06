/* Calf DSP Library
 * Benchmark for selected parts of the library.
 * Copyright (C) 2007 Krzysztof Foltman
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
#include <getopt.h>
#include <stdint.h>
#include <stdlib.h>
#include <config.h>
#include <calf/giface.h>
#include <calf/gui.h>
#include <calf/modules.h>
#include <calf/modules_dev.h>
#include <calf/benchmark.h>

using namespace std;
using namespace dsp;
using namespace synth;

#include <calf/osctl.h>
using namespace osctl;

#if 0
void osctl_test()
{
    string sdata = string("\000\000\000\003123\000test\000\000\000\000\000\000\000\001\000\000\000\002", 24);
    osc_stream is(sdata);
    vector<osc_data> data;
    is.read("bsii", data);
    assert(is.pos == sdata.length());
    assert(data.size() == 4);
    assert(data[0].type == osc_blob);
    assert(data[1].type == osc_string);
    assert(data[2].type == osc_i32);
    assert(data[3].type == osc_i32);
    assert(data[0].strval == "123");
    assert(data[1].strval == "test");
    assert(data[2].i32val == 1);
    assert(data[3].i32val == 2);
    osc_stream os("");
    os.write(data);
    assert(os.buffer == sdata);
    osc_server srv;
    srv.bind("0.0.0.0", 4541);
    
    osc_client cli;
    cli.bind("0.0.0.0", 0);
    cli.set_addr("0.0.0.0", 4541);
    if (!cli.send("/blah", data))
        g_error("Could not send the OSC message");
    
    g_main_loop_run(g_main_loop_new(NULL, FALSE));
}
#endif

struct plugin_proxy_base: public plugin_ctl_iface
{
    osc_client *client;
    bool send_osc;
    plugin_gui *gui;
    
    plugin_proxy_base()
    {
        client = NULL;
        send_osc = false;
        gui = NULL;
    }
};

template<class Module>
struct plugin_proxy: public plugin_proxy_base, public line_graph_iface
{
    float params[Module::param_count];
    virtual parameter_properties *get_param_props(int param_no) {
        return Module::param_props + param_no;
    }
    virtual float get_param_value(int param_no) {
        if (param_no < 0 || param_no >= Module::param_count)
            return 0;
        return params[param_no];
    }
    virtual void set_param_value(int param_no, float value) {
        if (param_no < 0 || param_no >= Module::param_count)
            return;
        params[param_no] = value;
        if (send_osc)
        {
            vector<osc_data> data;
            data.push_back(osc_data(param_no + get_param_port_offset()));
            data.push_back(osc_data(value));
            client->send("/control", data);
        }
    }
    virtual int get_param_count() {
        return Module::param_count;
    }
    virtual int get_param_port_offset() {
        return Module::in_count + Module::out_count;
    }
    virtual const char *get_gui_xml() {
        return Module::get_gui_xml();
    }
    virtual line_graph_iface *get_line_graph_iface() {
        return this;
    }
    virtual bool activate_preset(int bank, int program) { 
        if (send_osc) {
            vector<osc_data> data;
            data.push_back(osc_data(bank));
            data.push_back(osc_data(program));
            client->send("/program", data);
            return false;
        }
        return false;
    }
    virtual bool get_graph(int index, int subindex, float *data, int points, cairo_t *context) {
        return Module::get_static_graph(index, subindex, params[index], data, points, context);
    }
};

plugin_proxy_base *create_plugin_proxy(const char *effect_name)
{
    if (!strcmp(effect_name, "reverb"))
        return new plugin_proxy<reverb_audio_module>();
    else if (!strcmp(effect_name, "flanger"))
        return new plugin_proxy<flanger_audio_module>();
    else if (!strcmp(effect_name, "filter"))
        return new plugin_proxy<filter_audio_module>();
    else if (!strcmp(effect_name, "monosynth"))
        return new plugin_proxy<monosynth_audio_module>();
    else if (!strcmp(effect_name, "vintagedelay"))
        return new plugin_proxy<vintage_delay_audio_module>();
#ifdef ENABLE_EXPERIMENTAL
    else if (!strcmp(effect_name, "organ"))
        return new plugin_proxy<organ_audio_module>();
    else if (!strcmp(effect_name, "rotaryspeaker"))
        return new plugin_proxy<rotary_speaker_audio_module>();
#endif
    else
        return NULL;
}

void help(char *argv[])
{
    printf("GTK+ user interface for Calf DSSI plugins\nSyntax: %s [--help] [--version] <osc-url> <so-file> <plugin-label> <instance-name>\n", argv[0]);
}

static struct option long_options[] = {
    {"help", 0, 0, 'h'},
    {"version", 0, 0, 'v'},
    {0,0,0,0},
};

GMainLoop *mainloop;

struct dssi_osc_server: public osc_server, public osc_message_sink
{
    plugin_proxy_base *plugin;
    plugin_gui_window window;
    string effect_name, title;
    osc_client cli;
    bool in_program;
    vector<plugin_preset> presets;
    
    dssi_osc_server()
    : plugin(NULL)
    {
        sink = this;
    }
    
    static void on_destroy(GtkWindow *window, dssi_osc_server *self)
    {
        printf("on_destroy, have to send \"exiting\"\n");
        bool result = self->cli.send("/exiting");
        printf("result = %d\n", result ? 1 : 0);
        g_main_loop_quit(mainloop);
    }
    
    void create_window()
    {
        plugin = create_plugin_proxy(effect_name.c_str());
        plugin->client = &cli;
        plugin->send_osc = true;
        window.conditions.insert("dssi");
        window.create(plugin, title.c_str(), effect_name.c_str());
        plugin->gui = window.gui;
        gtk_signal_connect(GTK_OBJECT(window.toplevel), "destroy", G_CALLBACK(on_destroy), this);
        global_presets.get_for_plugin(presets, effect_name.c_str());
    }
    
    void receive_osc_message(std::string address, std::string type_tag, const std::vector<osc_data> &args)
    {
        dump.receive_osc_message(address, type_tag, args);
        if (address == prefix + "/update" && args.size() && args[0].type == osc_string)
        {
            printf("UPDATE: %s\n", args[0].strval.c_str());
            return;
        }
        if (address == prefix + "/quit")
        {
            printf("QUIT\n");
            g_main_loop_quit(mainloop);
            return;
        }
        if (address == prefix + "/program"&& args.size() >= 2 && args[0].type == osc_i32 && args[1].type == osc_i32)
        {
            unsigned int nr = args[0].i32val * 128 + args[1].i32val;
            printf("PROGRAM %d\n", nr);
            if (nr == 0)
            {
                bool sosc = plugin->send_osc;
                plugin->send_osc = false;
                int count = plugin->get_param_count();
                for (int i =0 ; i < count; i++)
                    plugin->set_param_value(i, plugin->get_param_props(i)->def_value);
                plugin->send_osc = sosc;
                window.gui->refresh();
                // special handling for default preset
                return;
            }
            nr--;
            if (nr >= presets.size())
                return;
            bool sosc = plugin->send_osc;
            plugin->send_osc = false;
            presets[nr].activate(plugin);
            plugin->send_osc = sosc;
            window.gui->refresh();
            
            // cli.send("/update", data);
            return;
        }
        if (address == prefix + "/control" && args.size() >= 2 && args[0].type == osc_i32 && args[1].type == osc_f32)
        {
            int idx = args[0].i32val - plugin->get_param_port_offset();
            float val = args[1].f32val;
            printf("CONTROL %d %f\n", idx, val);
            bool sosc = plugin->send_osc;
            plugin->send_osc = false;
            window.gui->set_param_value(idx, val);
            plugin->send_osc = sosc;
            return;
        }
        if (address == prefix + "/show")
        {
            gtk_widget_show_all(GTK_WIDGET(window.toplevel));
            return;
        }
        if (address == prefix + "/hide")
        {
            gtk_widget_hide(GTK_WIDGET(window.toplevel));
            return;
        }
    }
};

int main(int argc, char *argv[])
{
    gtk_init(&argc, &argv);
    while(1) {
        int option_index;
        int c = getopt_long(argc, argv, "hv", long_options, &option_index);
        if (c == -1)
            break;
        switch(c) {
            case 'h':
                help(argv);
                return 0;
            case 'v':
                printf("%s\n", PACKAGE_STRING);
                return 0;
        }
    }
    if (argc - optind != 4)
    {
        help(argv);
        exit(0);
    }

    try {
        global_presets.load_defaults();
    }
    catch(synth::preset_exception &e)
    {
        fprintf(stderr, "Error while loading presets: %s\n", e.what());
        exit(1);
    }

    dssi_osc_server srv;
    srv.prefix = "/dssi/"+string(argv[optind + 1]) + "/" + string(argv[optind + 2]);
    for (char *p = argv[optind + 2]; *p; p++)
        *p = tolower(*p);
    srv.effect_name = argv[optind + 2];
    srv.title = argv[optind + 3];
    
    srv.bind();
    srv.create_window();
    
    mainloop = g_main_loop_new(NULL, FALSE);

    srv.cli.bind();
    srv.cli.set_url(argv[optind]);
    
    printf("URI = %s\n", srv.get_uri().c_str());
    
    vector<osc_data> data;
    data.push_back(osc_data(srv.get_uri(), osc_string));
    if (!srv.cli.send("/update", data))
    {
        g_error("Could not send the initial update message via OSC to %s", argv[optind]);
        return 1;
    }
    
    g_main_loop_run(mainloop);
    printf("exited\n");
    
    return 0;
}
