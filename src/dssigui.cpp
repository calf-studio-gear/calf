/* Calf DSP Library utility application.
 * DSSI GUI application.
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
#include <assert.h>
#include <getopt.h>
#include <stdint.h>
#include <stdlib.h>
#include <config.h>
#include <calf/giface.h>
#include <calf/gui.h>
#include <calf/main_win.h>
#include <calf/osctl.h>
#include <calf/osctlnet.h>

using namespace std;
using namespace dsp;
using namespace calf_plugins;
using namespace osctl;

#define debug_printf(...)

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

struct plugin_proxy: public plugin_ctl_iface, public plugin_metadata_proxy
{
    osc_client *client;
    bool send_osc;
    plugin_gui *gui;
    map<string, string> cfg_vars;
    int param_count;
    float *params;

    plugin_proxy(plugin_metadata_iface *md)
    : plugin_metadata_proxy(md)
    {
        client = NULL;
        send_osc = false;
        gui = NULL;
        param_count = md->get_param_count();
        params = new float[param_count];
        for (int i = 0; i < param_count; i++)
            params[i] = get_param_props(i)->def_value;
    }
    virtual float get_param_value(int param_no) {
        if (param_no < 0 || param_no >= param_count)
            return 0;
        return params[param_no];
    }
    virtual void set_param_value(int param_no, float value) {
        if (param_no < 0 || param_no >= param_count)
            return;
        params[param_no] = value;
        if (send_osc)
        {
            osc_inline_typed_strstream str;
            str << (uint32_t)(param_no + get_param_port_offset()) << value;
            client->send("/control", str);
        }
    }
    virtual bool activate_preset(int bank, int program) { 
        if (send_osc) {
            osc_inline_typed_strstream str;
            str << (uint32_t)(bank) << (uint32_t)(program);
            client->send("/program", str);
            return false;
        }
        return false;
    }
    virtual float get_level(unsigned int port) { return 0.f; }
    virtual void execute(int command_no) { 
        if (send_osc) {
            stringstream ss;
            ss << command_no;
            
            osc_inline_typed_strstream str;
            str << "ExecCommand" << ss.str();
            client->send("/configure", str);
            
            str.clear();
            str << "ExecCommand" << "";
            client->send("/configure", str);
        }
    }
    char *configure(const char *key, const char *value) { 
        cfg_vars[key] = value;
        osc_inline_typed_strstream str;
        str << key << value;
        client->send("/configure", str);
        return NULL;
    }
    void send_configures(send_configure_iface *sci) { 
        for (map<string, string>::iterator i = cfg_vars.begin(); i != cfg_vars.end(); i++)
            sci->send_configure(i->first.c_str(), i->second.c_str());
    }
};

void help(char *argv[])
{
    printf("GTK+ user interface for Calf DSSI plugins\nSyntax: %s [--help] [--version] <osc-url> <so-file> <plugin-label> <instance-name>\n", argv[0]);
}

static struct option long_options[] = {
    {"help", 0, 0, 'h'},
    {"version", 0, 0, 'v'},
    {"debug", 0, 0, 'd'},
    {0,0,0,0},
};

GMainLoop *mainloop;

static bool osc_debug = false;

struct dssi_osc_server: public osc_server, public osc_message_sink<osc_strstream>
{
    plugin_proxy *plugin;
    main_window *main_win;
    plugin_gui_window *window;
    string effect_name, title;
    osc_client cli;
    bool in_program, enable_dump;
    vector<plugin_preset> presets;
    
    dssi_osc_server()
    : plugin(NULL)
    , main_win(new main_window)
    , window(new plugin_gui_window(main_win))
    {
        sink = this;
    }
    
    static void on_destroy(GtkWindow *window, dssi_osc_server *self)
    {
        debug_printf("on_destroy, have to send \"exiting\"\n");
        bool result = self->cli.send("/exiting");
        debug_printf("result = %d\n", result ? 1 : 0);
        g_main_loop_quit(mainloop);
        // eliminate a warning with empty debug_printf
        result = false;
    }
    
    void create_window()
    {
        plugin = NULL;
        vector<plugin_metadata_iface *> plugins;
        get_all_plugins(plugins);
        for (unsigned int i = 0; i < plugins.size(); i++)
        {
            if (!strcmp(plugins[i]->get_id(), effect_name.c_str()))
            {
                plugin = new plugin_proxy(plugins[i]);
                break;
            }
        }
        if (!plugin)
        {
            fprintf(stderr, "Unknown plugin: %s\n", effect_name.c_str());
            exit(1);
        }
        plugin->client = &cli;
        plugin->send_osc = true;
        ((main_window *)window->main)->conditions.insert("dssi");
        window->create(plugin, title.c_str(), effect_name.c_str());
        plugin->gui = window->gui;
        gtk_signal_connect(GTK_OBJECT(window->toplevel), "destroy", G_CALLBACK(on_destroy), this);
        vector<plugin_preset> tmp_presets;
        get_builtin_presets().get_for_plugin(presets, effect_name.c_str());
        get_user_presets().get_for_plugin(tmp_presets, effect_name.c_str());
        presets.insert(presets.end(), tmp_presets.begin(), tmp_presets.end());
    }
    
    virtual void receive_osc_message(std::string address, std::string args, osc_strstream &buffer)
    {
        if (osc_debug)
            dump.receive_osc_message(address, args, buffer);
        if (address == prefix + "/update" && args == "s")
        {
            string str;
            buffer >> str;
            debug_printf("UPDATE: %s\n", str.c_str());
            return;
        }
        if (address == prefix + "/quit")
        {
            debug_printf("QUIT\n");
            g_main_loop_quit(mainloop);
            return;
        }
        if (address == prefix + "/configure"&& args == "ss")
        {
            string key, value;
            buffer >> key >> value;
            plugin->cfg_vars[key] = value;
            // XXXKF perhaps this should be queued !
            window->gui->refresh();
            return;
        }
        if (address == prefix + "/program"&& args == "ii")
        {
            uint32_t bank, program;
            
            buffer >> bank >> program;
            
            unsigned int nr = bank * 128 + program;
            debug_printf("PROGRAM %d\n", nr);
            if (nr == 0)
            {
                bool sosc = plugin->send_osc;
                plugin->send_osc = false;
                int count = plugin->get_param_count();
                for (int i =0 ; i < count; i++)
                    plugin->set_param_value(i, plugin->get_param_props(i)->def_value);
                plugin->send_osc = sosc;
                window->gui->refresh();
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
            window->gui->refresh();
            
            // cli.send("/update", data);
            return;
        }
        if (address == prefix + "/control" && args == "if")
        {
            uint32_t port;
            float val;
            
            buffer >> port >> val;
            
            int idx = port - plugin->get_param_port_offset();
            debug_printf("CONTROL %d %f\n", idx, val);
            bool sosc = plugin->send_osc;
            plugin->send_osc = false;
            window->gui->set_param_value(idx, val);
            plugin->send_osc = sosc;
            return;
        }
        if (address == prefix + "/show")
        {
            gtk_widget_show_all(GTK_WIDGET(window->toplevel));
            return;
        }
        if (address == prefix + "/hide")
        {
            gtk_widget_hide(GTK_WIDGET(window->toplevel));
            return;
        }
    }
};

int main(int argc, char *argv[])
{
    char *debug_var = getenv("OSC_DEBUG");
    if (debug_var && atoi(debug_var))
        osc_debug = true;
    
    gtk_init(&argc, &argv);
    while(1) {
        int option_index;
        int c = getopt_long(argc, argv, "dhv", long_options, &option_index);
        if (c == -1)
            break;
        switch(c) {
            case 'd':
                osc_debug = true;
                break;
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
        get_builtin_presets().load_defaults(true);
        get_user_presets().load_defaults(false);
    }
    catch(calf_plugins::preset_exception &e)
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
    
    debug_printf("URI = %s\n", srv.get_uri().c_str());
    
    string data_buf, type_buf;
    osc_inline_typed_strstream data;
    data << srv.get_uri();
    if (!srv.cli.send("/update", data))
    {
        g_error("Could not send the initial update message via OSC to %s", argv[optind]);
        return 1;
    }
    
    g_main_loop_run(mainloop);
    debug_printf("exited\n");
    
    return 0;
}
