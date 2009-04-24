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
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 * Boston, MA  02110-1301  USA
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
#include <calf/osctlserv.h>

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

struct cairo_params
{
    enum { HAS_COLOR = 1, HAS_WIDTH = 2 };
    uint32_t flags;
    float r, g, b, a;
    float line_width;
    
    cairo_params()
    : flags(0)
    , r(0.f)
    , g(0.f)
    , b(0.f)
    , a(1.f)
    , line_width(1)
    {
    }
};

struct graph_item: public cairo_params
{
    float data[128];
};

struct dot_item: public cairo_params
{
    float x, y;
    int32_t size;
};

struct gridline_item: public cairo_params
{
    float pos;
    int32_t vertical;
    std::string text;
};

struct param_line_graphs
{
    vector<graph_item *> graphs;
    vector<dot_item *> dots;
    vector<gridline_item *> gridlines;
    
    void clear();
};

void param_line_graphs::clear()
{
    for (size_t i = 0; i < graphs.size(); i++)
        delete graphs[i];
    graphs.clear();

    for (size_t i = 0; i < dots.size(); i++)
        delete dots[i];
    dots.clear();

    for (size_t i = 0; i < gridlines.size(); i++)
        delete gridlines[i];
    gridlines.clear();

}

struct plugin_proxy: public plugin_ctl_iface, public plugin_metadata_proxy, public line_graph_iface
{
    osc_client *client;
    bool send_osc;
    plugin_gui *gui;
    map<string, string> cfg_vars;
    int param_count;
    float *params;
    map<int, param_line_graphs> graphs;
    bool update_graphs;

    plugin_proxy(plugin_metadata_iface *md)
    : plugin_metadata_proxy(md)
    {
        client = NULL;
        send_osc = false;
        update_graphs = true;
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
        update_graphs = true;
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
        // do not store temporary vars in presets
        if (strncmp(key, "OSC:", 4))
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
    virtual line_graph_iface *get_line_graph_iface() { return this; }
    virtual bool get_graph(int index, int subindex, float *data, int points, cairo_iface *context);
    virtual bool get_dot(int index, int subindex, float &x, float &y, int &size, cairo_iface *context);
    virtual bool get_gridline(int index, int subindex, float &pos, bool &vertical, std::string &legend, cairo_iface *context);
    void update_cairo_context(cairo_iface *context, cairo_params &item);
};

bool plugin_proxy::get_graph(int index, int subindex, float *data, int points, cairo_iface *context)
{
    if (!graphs.count(index))
        return false;
    param_line_graphs &g = graphs[index];
    if (subindex < (int)g.graphs.size())
    {
        float *sdata = g.graphs[subindex]->data;
        for (int i = 0; i < points; i++) {
            float pos = i * 127.0 / points;
            int ipos = i * 127 / points;
            data[i] = sdata[ipos] + (sdata[ipos + 1] - sdata[ipos]) * (pos-ipos);
        }
        update_cairo_context(context, *g.graphs[subindex]);
        return true;
    }
    return false;
}

bool plugin_proxy::get_dot(int index, int subindex, float &x, float &y, int &size, cairo_iface *context)
{
    if (!graphs.count(index))
        return false;
    param_line_graphs &g = graphs[index];
    if (subindex < (int)g.dots.size())
    {
        dot_item &item = *g.dots[subindex];
        x = item.x;
        y = item.y;
        size = item.size;
        update_cairo_context(context, item);
        return true;
    }
    return false;
}

bool plugin_proxy::get_gridline(int index, int subindex, float &pos, bool &vertical, std::string &legend, cairo_iface *context)
{
    if (!graphs.count(index))
        return false;
    param_line_graphs &g = graphs[index];
    if (subindex < (int)g.gridlines.size())
    {
        gridline_item &item = *g.gridlines[subindex];
        pos = item.pos;
        vertical = item.vertical != 0;
        legend = item.text;
        update_cairo_context(context, item);
        return true;
    }
    return false;
}

void plugin_proxy::update_cairo_context(cairo_iface *context, cairo_params &item)
{
    if (item.flags & cairo_params::HAS_COLOR)
        context->set_source_rgba(item.r, item.g, item.b, item.a);
    if (item.flags & cairo_params::HAS_WIDTH)
        context->set_line_width(item.line_width);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////

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
    /// Timeout callback source ID
    int source_id;
    bool osc_link_active;
    
    dssi_osc_server()
    : plugin(NULL)
    , main_win(new main_window)
    , window(new plugin_gui_window(main_win))
    {
        sink = this;
        source_id = 0;
        osc_link_active = false;
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
        ((main_window *)window->main)->conditions.insert("directlink");
        window->create(plugin, title.c_str(), effect_name.c_str());
        plugin->gui = window->gui;
        gtk_signal_connect(GTK_OBJECT(window->toplevel), "destroy", G_CALLBACK(on_destroy), this);
        vector<plugin_preset> tmp_presets;
        get_builtin_presets().get_for_plugin(presets, effect_name.c_str());
        get_user_presets().get_for_plugin(tmp_presets, effect_name.c_str());
        presets.insert(presets.end(), tmp_presets.begin(), tmp_presets.end());
        source_id = g_timeout_add_full(G_PRIORITY_LOW, 1000/30, on_idle, this, NULL);
    }
    
    static gboolean on_idle(void *self)
    {
        dssi_osc_server *self_ptr = (dssi_osc_server *)(self);
        if (self_ptr->osc_link_active)
            self_ptr->send_osc_update();
        return TRUE;
    }
    
    void set_osc_update(bool enabled);
    void send_osc_update();
    
    virtual void receive_osc_message(std::string address, std::string args, osc_strstream &buffer);
    void unmarshal_line_graph(osc_strstream &buffer);
};

void dssi_osc_server::set_osc_update(bool enabled)
{
    osc_link_active = enabled;
    osc_inline_typed_strstream data;
    data << "OSC:FEEDBACK_URI";
    data << (enabled ? get_uri() : "");
    cli.send("/configure", data);
}

void dssi_osc_server::send_osc_update()
{
    static int serial_no = 0;
    osc_inline_typed_strstream data;
    data << "OSC:UPDATE";
    data << calf_utils::i2s(serial_no++);
    cli.send("/configure", data);
}

void dssi_osc_server::unmarshal_line_graph(osc_strstream &buffer)
{
    uint32_t cmd;
    
    do {
        buffer >> cmd;
        if (cmd == LGI_GRAPH)
        {
            uint32_t param;
            buffer >> param;
            param_line_graphs &graphs = plugin->graphs[param];
            
            graphs.clear();
            cairo_params params;

            do {
                buffer >> cmd;
                if (cmd == LGI_SET_RGBA)
                {
                    params.flags |= cairo_params::HAS_COLOR;
                    buffer >> params.r >> params.g >> params.b >> params.a;
                }
                else
                if (cmd == LGI_SET_WIDTH)
                {
                    params.flags |= cairo_params::HAS_WIDTH;
                    buffer >> params.line_width;
                }
                else
                if (cmd == LGI_SUBGRAPH)
                {
                    buffer >> param; // ignore number of points
                    graph_item *gi = new graph_item;
                    (cairo_params &)*gi = params;
                    for (int i = 0; i < 128; i++)
                        buffer >> gi->data[i];
                    graphs.graphs.push_back(gi);
                    params.flags = 0;
                }
                else
                if (cmd == LGI_DOT)
                {
                    dot_item *di = new dot_item;
                    (cairo_params &)*di = params;
                    buffer >> di->x >> di->y >> di->size;
                    graphs.dots.push_back(di);
                    params.flags = 0;
                }
                else
                if (cmd == LGI_LEGEND)
                {
                    gridline_item *li = new gridline_item;
                    (cairo_params &)*li = params;
                    buffer >> li->pos >> li->vertical >> li->text;
                    graphs.gridlines.push_back(li);
                    params.flags = 0;
                }
                else
                    break;
            } while(1);
            
        }
        else
            break;
    } while(1);
}

void dssi_osc_server::receive_osc_message(std::string address, std::string args, osc_strstream &buffer)
{
    if (osc_debug)
        dump.receive_osc_message(address, args, buffer);
    if (address == prefix + "/update" && args == "s")
    {
        string str;
        buffer >> str;
        debug_printf("UPDATE: %s\n", str.c_str());
        set_osc_update(true);
        send_osc_update();
        return;
    }
    else if (address == prefix + "/quit")
    {
        set_osc_update(false);
        debug_printf("QUIT\n");
        g_main_loop_quit(mainloop);
        return;
    }
    else if (address == prefix + "/configure"&& args == "ss")
    {
        string key, value;
        buffer >> key >> value;
        // do not store temporary vars in presets
        if (strncmp(key.c_str(), "OSC:", 4))
            plugin->cfg_vars[key] = value;
        // XXXKF perhaps this should be queued !
        window->gui->refresh();
        return;
    }
    else if (address == prefix + "/program"&& args == "ii")
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
    else if (address == prefix + "/control" && args == "if")
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
        if (plugin->get_param_props(idx)->flags & PF_PROP_GRAPH)
            plugin->update_graphs = true;
        return;
    }
    else if (address == prefix + "/show")
    {
        set_osc_update(true);

        gtk_widget_show_all(GTK_WIDGET(window->toplevel));
        return;
    }
    else if (address == prefix + "/hide")
    {
        set_osc_update(false);

        gtk_widget_hide(GTK_WIDGET(window->toplevel));
        return;
    }
    else if (address == prefix + "/lineGraph")
    {
        unmarshal_line_graph(buffer);
        if (plugin->update_graphs) {
            // updates graphs that are only redrawn on startup and parameter changes
            // (the OSC message may come a while after the parameter has been changed,
            // so the redraw triggered by parameter change usually shows stale values)
            window->gui->refresh();
            plugin->update_graphs = false;
        }
        return;
    }
    else
        printf("Unknown OSC address: %s\n", address.c_str());
}

//////////////////////////////////////////////////////////////////////////////////

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
    if (srv.source_id)
        g_source_remove(srv.source_id);

    srv.set_osc_update(false);
    debug_printf("exited\n");
    
    return 0;
}
