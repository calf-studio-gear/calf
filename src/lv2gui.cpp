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
#include <sys/wait.h>
#include <calf/giface.h>
#include <calf/gui.h>
#include <calf/main_win.h>
#include <calf/lv2_data_access.h>
#include <calf/lv2_string_port.h>
#include <calf/lv2_ui.h>
#include <calf/lv2_uri_map.h>
#include <calf/lv2_external_ui.h>
#include <calf/osctlnet.h>
#include <calf/osctlserv.h>
#include <calf/preset_gui.h>
#include <calf/utils.h>
#include <calf/lv2helpers.h>

using namespace std;
using namespace dsp;
using namespace calf_plugins;
using namespace calf_utils;
using namespace osctl;

struct LV2_Calf_Descriptor {
    plugin_ctl_iface *(*get_pci)(LV2_Handle Instance);
};

struct plugin_proxy: public plugin_ctl_iface, public plugin_metadata_proxy
{
    LV2UI_Write_Function write_function;
    LV2UI_Controller controller;
    
    bool send;
    plugin_gui *gui;
    float *params;
    int param_count;
    /// Instance pointer - usually NULL unless the host supports instance-access extension
    plugin_ctl_iface *instance;
    int source_id;
    LV2_Handle instance_handle;
    LV2_Extension_Data_Feature *data_access;
    LV2_URI_Map_Feature *uri_map;
    map<string, int> params_by_name;
    uint32_t string_port_uri;
    
    plugin_proxy(const plugin_metadata_iface *md)
    : plugin_metadata_proxy(md)
    {
        gui = NULL;
        instance = NULL;
        instance_handle = NULL;
        data_access = NULL;
        send = true;
        param_count = get_param_count();
        params = new float[param_count];
        string_port_uri = 0;
        for (int i = 0; i < param_count; i++)
        {
            parameter_properties *pp = get_param_props(i);
            params_by_name[pp->short_name] = i;
            if ((pp->flags & PF_TYPEMASK) < PF_STRING)
                params[i] = pp->def_value;
        }
    }
    
    void setup(LV2UI_Write_Function wfn, LV2UI_Controller ctl)
    {
        write_function = wfn;
        controller = ctl;
    }
    
    virtual float get_param_value(int param_no) {
        if (param_no < 0 || param_no >= param_count)
            return 0;
        return params[param_no];
    }
    
    virtual void set_param_value(int param_no, float value) {
        if (param_no < 0 || param_no >= param_count)
            return;
        if ((get_param_props(param_no)->flags & PF_TYPEMASK) >= PF_STRING)
        {
            //assert(0);
            return;
        }
        params[param_no] = value;
        if (send) {
            scope_assign<bool> _a_(send, false);
            write_function(controller, param_no + get_param_port_offset(), sizeof(float), 0, &params[param_no]);
        }
    }
    
    virtual bool activate_preset(int bank, int program)
    {
        return false;
    }
    
    virtual const line_graph_iface *get_line_graph_iface() const {
        if (instance)
            return instance->get_line_graph_iface();
        return NULL;
    }
    
    virtual char *configure(const char *key, const char *value)
    {
        if (!send)
            return NULL;
        map<string, int>::iterator i = params_by_name.find(key);
        if (i == params_by_name.end())
        {
            fprintf(stderr, "ERROR: configure called for unknown key %s\n", key);
            assert(0);
            return NULL;
        }
        LV2_String_Data data;
        data.data = (char *)value;
        data.len = strlen(value);
        data.storage = -1; // host doesn't need that
        data.flags = 0;
        data.pad = 0;
        
        int idx = i->second;
        if (string_port_uri) {
            write_function(controller, idx + get_param_port_offset(), sizeof(LV2_String_Data), string_port_uri, &data);
        }
        
        return NULL;
    }
    
    virtual float get_level(unsigned int port) { return 0.f; }
    virtual void execute(int command_no) { assert(0); }
    void send_configures(send_configure_iface *sci) { 
        fprintf(stderr, "TODO: send_configures (non-control port configuration dump) not implemented in LV2 GUIs\n");
    }
    void resolve_instance() {
        fprintf(stderr, "CALF DEBUG: instance %p data %p\n", instance_handle, data_access);
        if (instance_handle && data_access)
        {
            LV2_Calf_Descriptor *calf = (LV2_Calf_Descriptor *)(*data_access->data_access)("http://foltman.com/ns/calf-plugin-instance");
            fprintf(stderr, "CALF DEBUG: calf %p cpi %p\n", calf, calf ? calf->get_pci : NULL);
            if (calf && calf->get_pci)
                instance = calf->get_pci(instance_handle);
        }
    }
    uint32_t map_uri(const char *mapURI, const char *keyURI)
    {
        if (!uri_map)
            return 0;
        return uri_map->uri_to_id(uri_map->callback_data, mapURI, keyURI);
    }
        
    ~plugin_proxy()
    {
        delete []params;
    }
};

static gboolean plugin_on_idle(void *data)
{
    plugin_gui *self = (plugin_gui *)data;
    self->on_idle();
    return TRUE;
}

LV2UI_Handle gui_instantiate(const struct _LV2UI_Descriptor* descriptor,
                          const char*                     plugin_uri,
                          const char*                     bundle_path,
                          LV2UI_Write_Function            write_function,
                          LV2UI_Controller                controller,
                          LV2UI_Widget*                   widget,
                          const LV2_Feature* const*       features)
{
    plugin_proxy *proxy = NULL;
    const plugin_metadata_iface *md = plugin_registry::instance().get_by_uri(plugin_uri);
    if (!md)
        return NULL;
    proxy = new plugin_proxy(md);
    if (!proxy)
        return NULL;
    for (int i = 0; features[i]; i++)
    {
        if (!strcmp(features[i]->URI, "http://lv2plug.in/ns/ext/instance-access"))
            proxy->instance_handle = features[i]->data;
        else if (!strcmp(features[i]->URI, "http://lv2plug.in/ns/ext/data-access"))
            proxy->data_access = (LV2_Extension_Data_Feature *)features[i]->data;
        else if (!strcmp(features[i]->URI, LV2_URI_MAP_URI))
        {
            proxy->uri_map = (LV2_URI_Map_Feature *)features[i]->data;
            proxy->string_port_uri = proxy->map_uri("http://lv2plug.in/ns/extensions/ui", 
                LV2_STRING_PORT_URI);
        }
    }
    proxy->resolve_instance();
    scope_assign<bool> _a_(proxy->send, false);
    proxy->setup(write_function, controller);
    // dummy window
    main_window *main = new main_window;
    if (proxy->instance)
        main->conditions.insert("directlink");
    main->conditions.insert("lv2gui");    
    plugin_gui_window *window = new plugin_gui_window(main);
    plugin_gui *gui = new plugin_gui(window);
    const char *xml = proxy->get_gui_xml();
    assert(xml);
    *(GtkWidget **)(widget) = gui->create_from_xml(proxy, xml);
    gtk_rc_parse(PKGLIBDIR "calf.rc");
    if (*(GtkWidget **)(widget))
        proxy->source_id = g_timeout_add_full(G_PRIORITY_LOW, 1000/30, plugin_on_idle, gui, NULL); // 30 fps should be enough for everybody    
    
    return (LV2UI_Handle)gui;
}

void gui_cleanup(LV2UI_Handle handle)
{
    plugin_gui *gui = (plugin_gui *)handle;
    plugin_proxy *proxy = dynamic_cast<plugin_proxy *>(gui->plugin);
    if (proxy->source_id)
        g_source_remove(proxy->source_id);
    delete gui;
}

void gui_port_event(LV2UI_Handle handle, uint32_t port, uint32_t buffer_size, uint32_t format, const void *buffer)
{
    plugin_gui *gui = (plugin_gui *)handle;
    plugin_proxy *proxy = dynamic_cast<plugin_proxy *>(gui->plugin);
    assert(proxy);
    float v = *(float *)buffer;
    port -= gui->plugin->get_param_port_offset();
    if (port >= (uint32_t)gui->plugin->get_param_count())
        return;
    if ((gui->plugin->get_param_props(port)->flags & PF_TYPEMASK) == PF_STRING)
    {
        scope_assign<bool> _a_(proxy->send, false);
        gui->plugin->configure(gui->plugin->get_param_props(port)->short_name, ((LV2_String_Data *)buffer)->data);
        return;
    }
    if (fabs(gui->plugin->get_param_value(port) - v) < 0.00001)
        return;
    {
        scope_assign<bool> _a_(proxy->send, false);
        gui->set_param_value(port, v);
    }
}

const void *gui_extension(const char *uri)
{
    return NULL;
}

namespace calf_plugins {

// this function is normally implemented in preset_gui.cpp, but we're not using that file
void activate_preset(GtkAction *action, activate_preset_params *params)
{
}

// so is this function
void store_preset(GtkWindow *toplevel, plugin_gui *gui)
{
}

};

///////////////////////////////////////////////////////////////////////////////////////

class ext_plugin_gui: public lv2_external_ui, public osc_message_sink<osc_strstream>
{
public:
    LV2UI_Write_Function write_function;
    LV2UI_Controller controller;
    const LV2_Feature* const* features;
    lv2_external_ui_host *host;
    GPid child_pid;
    osc_server srv;
    osc_client cli;
    bool confirmed;
    string prefix;

    ext_plugin_gui(LV2UI_Write_Function wf, LV2UI_Controller c, const LV2_Feature* const* f);

    bool initialise();

    void show_impl();
    void hide_impl();
    void run_impl();
    void port_event_impl(uint32_t port, uint32_t buffer_size, uint32_t format, const void *buffer);

    virtual void receive_osc_message(std::string address, std::string args, osc_strstream &buffer);
    virtual ~ext_plugin_gui() {}
        
private:
    static void show_(lv2_external_ui *h) { ((ext_plugin_gui *)(h))->show_impl(); }
    static void hide_(lv2_external_ui *h) { ((ext_plugin_gui *)(h))->hide_impl(); }
    static void run_(lv2_external_ui *h) { ((ext_plugin_gui *)(h))->run_impl(); }
};

ext_plugin_gui::ext_plugin_gui(LV2UI_Write_Function wf, LV2UI_Controller c, const LV2_Feature* const* f)
{
    write_function = wf;
    controller = c;
    features = f;
    host = NULL;
    confirmed = false;
    
    show = show_;
    hide = hide_;
    run = run_;
}

bool ext_plugin_gui::initialise()
{
    for(const LV2_Feature* const* i = features; *i; i++)
    {
        if (!strcmp((*i)->URI, LV2_EXTERNAL_UI_URI))
        {
            host = (lv2_external_ui_host *)(*i)->data;
            break;
        }
    }
    if (host == NULL)
        return false;
    
    srv.sink = this;
    srv.bind("127.0.0.1");
    
    return true;
}

void ext_plugin_gui::show_impl()
{
    printf("show\n");
    cli.send("/show");
}

void ext_plugin_gui::hide_impl()
{
    printf("hide\n");
    cli.send("/hide");
}

void ext_plugin_gui::port_event_impl(uint32_t port, uint32_t buffer_size, uint32_t format, const void *buffer)
{
    assert(confirmed);
    osc_inline_typed_strstream data;
    if (format == 0)
    {
        data << port; 
        data << *(float *)buffer;
        cli.send("/control", data);
        printf("control %d %f\n", port, *(float *)buffer);
    }
}

void ext_plugin_gui::run_impl()
{
    if (waitpid(child_pid, NULL, WNOHANG) != 0)
    {
        host->ui_closed(controller);
    }
    srv.read_from_socket();
}

void ext_plugin_gui::receive_osc_message(std::string address, std::string args, osc_strstream &buffer)
{
    if (address == "/bridge/update" && args == "s")
    {
        string url;
        buffer >> url;
        cli.bind();
        cli.set_url(url.c_str());
        confirmed = true;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////

LV2UI_Handle extgui_instantiate(const struct _LV2UI_Descriptor* descriptor,
                          const char*                     plugin_uri,
                          const char*                     bundle_path,
                          LV2UI_Write_Function            write_function,
                          LV2UI_Controller                controller,
                          LV2UI_Widget*                   widget,
                          const LV2_Feature* const*       features)
{
    ext_plugin_gui *ui = new ext_plugin_gui(write_function, controller, features);
    if (!ui->initialise())
        return NULL;
    
    string url = ui->srv.get_url() + "/bridge";
    const gchar *argv[] = { "./calf_gtk", url.c_str(), "calf.so", plugin_uri, (ui->host->plugin_human_id ? ui->host->plugin_human_id : "Unknown"), NULL };
    GError *error = NULL;
    if (g_spawn_async(bundle_path, (gchar **)argv, NULL, (GSpawnFlags)G_SPAWN_DO_NOT_REAP_CHILD, NULL, NULL, &ui->child_pid, &error))
    {
        // wait for the sign of life from the GUI
        while(!ui->confirmed && waitpid(ui->child_pid, NULL, WNOHANG) == 0)
        {
            printf("Waiting for the GUI to open\n");
            ui->srv.read_from_socket();
            usleep(500000);
        }
        
        if (ui->confirmed)
        {
            *(lv2_external_ui **)widget = ui;
            return (LV2UI_Handle)ui;
        }
        else
        {
            g_warning("The GUI exited before establishing contact with the host");
            return NULL;
        }
    }
    else
    {
        g_warning("%s", error->message);
        return NULL;
    }
}

void extgui_cleanup(LV2UI_Handle handle)
{
    ext_plugin_gui *gui = (ext_plugin_gui *)handle;
    delete gui;
    printf("cleanup\n");
}

void extgui_port_event(LV2UI_Handle handle, uint32_t port, uint32_t buffer_size, uint32_t format, const void *buffer)
{
    ((ext_plugin_gui *)handle)->port_event_impl(port, buffer_size, format, buffer);;
}

const void *extgui_extension(const char *uri)
{
    printf("extgui_extension %s\n", uri);
    return NULL;
}

///////////////////////////////////////////////////////////////////////////////////////

const LV2UI_Descriptor* lv2ui_descriptor(uint32_t index)
{
    static LV2UI_Descriptor gui, extgui;
    gui.URI = "http://calf.sourceforge.net/plugins/gui/gtk2-gui";
    gui.instantiate = gui_instantiate;
    gui.cleanup = gui_cleanup;
    gui.port_event = gui_port_event;
    gui.extension_data = gui_extension;
    extgui.URI = "http://calf.sourceforge.net/plugins/gui/ext-gui";
    extgui.instantiate = extgui_instantiate;
    extgui.cleanup = extgui_cleanup;
    extgui.port_event = extgui_port_event;
    extgui.extension_data = extgui_extension;
    switch(index) {
        case 0:
            return &gui;
        case 1:
            return &extgui;
        default:
            return NULL;
    }
}

