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
#include <sys/wait.h>
#include "config.h"
#include <calf/gui.h>
#include <calf/giface.h>
#include <calf/lv2_data_access.h>
#include <calf/lv2_ui.h>
#include <calf/lv2_uri_map.h>
#include <calf/lv2_external_ui.h>
#include <calf/lv2helpers.h>
#include <calf/utils.h>
#include <glib.h>

using namespace std;
using namespace calf_plugins;
using namespace calf_utils;

struct LV2_Calf_Descriptor {
    plugin_ctl_iface *(*get_pci)(LV2_Handle Instance);
};

/// Temporary assignment to a slot in vector<bool>
typedef scope_assign<bool, vector<bool>::reference> TempSendSetter;

/// Common data and functions for GTK+ GUI and External GUI
struct plugin_proxy_base
{
    const plugin_metadata_iface *plugin_metadata;
    LV2UI_Write_Function write_function;
    LV2UI_Controller controller;

    // Values extracted from the Features array from the host
    
    /// Handle to the plugin instance
    LV2_Handle instance_handle;
    /// Data access feature instance
    LV2_Extension_Data_Feature *data_access;
    /// URI map feature
    LV2_URI_Map_Feature *uri_map;
    /// External UI host feature (must be set when instantiating external UI plugins)
    lv2_external_ui_host *ext_ui_host;
    
    /// Instance pointer - usually NULL unless the host supports instance-access extension
    plugin_ctl_iface *instance;
    /// If true, a given parameter (not port) may be sent to host - it is blocked when the parameter is written to by the host
    vector<bool> sends;
    /// Map of parameter name to parameter index (used for mapping configure values to string ports)
    map<string, int> params_by_name;
    /// Values of parameters (float control ports)
    vector<float> params;
    /// Number of parameters (non-audio ports)
    int param_count;
    /// Number of the first parameter port
    int param_offset;
    
    plugin_proxy_base(const plugin_metadata_iface *metadata, LV2UI_Write_Function wf, LV2UI_Controller c, const LV2_Feature* const* features);

    /// Send a float value to a control port in the host
    void send_float_to_host(int param_no, float value);
    
    /// Send a string value to a string port in the host, by name (configure-like mechanism)
    char *configure(const char *key, const char *value);
    
    /// Enable sending to host for all ports
    void enable_all_sends();
    
    /// Obtain instance pointers
    void resolve_instance();

    /// Obtain line graph interface if available
    const line_graph_iface *get_line_graph_iface() const;
    
    /// Map an URI to an integer value using a given URI map
    uint32_t map_uri(const char *mapURI, const char *keyURI);

};

plugin_proxy_base::plugin_proxy_base(const plugin_metadata_iface *metadata, LV2UI_Write_Function wf, LV2UI_Controller c, const LV2_Feature* const* features)
{
    plugin_metadata = metadata;
    
    write_function = wf;
    controller = c;

    instance = NULL;
    instance_handle = NULL;
    data_access = NULL;
    ext_ui_host = NULL;
    
    param_count = metadata->get_param_count();
    param_offset = metadata->get_param_port_offset();
    
    /// Block all updates until GUI is ready
    sends.resize(param_count, false);
    params.resize(param_count);
    for (int i = 0; i < param_count; i++)
    {
        const parameter_properties *pp = metadata->get_param_props(i);
        params_by_name[pp->short_name] = i;
        params[i] = pp->def_value;
    }
    for (int i = 0; features[i]; i++)
    {
        if (!strcmp(features[i]->URI, "http://lv2plug.in/ns/ext/instance-access"))
        {
            instance_handle = features[i]->data;
        }
        else if (!strcmp(features[i]->URI, "http://lv2plug.in/ns/ext/data-access"))
        {
            data_access = (LV2_Extension_Data_Feature *)features[i]->data;
        }
        else if (!strcmp(features[i]->URI, LV2_EXTERNAL_UI_URI))
        {
            ext_ui_host = (lv2_external_ui_host *)features[i]->data;
        }
    }
    resolve_instance();
}

void plugin_proxy_base::send_float_to_host(int param_no, float value)
{
    params[param_no] = value;
    if (sends[param_no]) {
        TempSendSetter _a_(sends[param_no], false);
        write_function(controller, param_no + param_offset, sizeof(float), 0, &params[param_no]);
    }
}

void plugin_proxy_base::resolve_instance()
{
    fprintf(stderr, "CALF DEBUG: instance %p data %p\n", instance_handle, data_access);
    if (instance_handle && data_access)
    {
        LV2_Calf_Descriptor *calf = (LV2_Calf_Descriptor *)(*data_access->data_access)("http://foltman.com/ns/calf-plugin-instance");
        fprintf(stderr, "CALF DEBUG: calf %p cpi %p\n", calf, calf ? calf->get_pci : NULL);
        if (calf && calf->get_pci)
            instance = calf->get_pci(instance_handle);
    }
}

uint32_t plugin_proxy_base::map_uri(const char *mapURI, const char *keyURI)
{
    if (!uri_map)
        return 0;
    return uri_map->uri_to_id(uri_map->callback_data, mapURI, keyURI);
}

const line_graph_iface *plugin_proxy_base::get_line_graph_iface() const
{
    if (instance)
        return instance->get_line_graph_iface();
    return NULL;
}

char *plugin_proxy_base::configure(const char *key, const char *value)
{
    if (instance)
        return instance->configure(key, value);
    else
        return strdup("Configuration not available because of lack of instance-access/data-access");
}

void plugin_proxy_base::enable_all_sends()
{
    sends.clear();
    sends.resize(param_count, true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// Plugin controller that uses LV2 host with help of instance/data access to remotely
/// control a plugin from the GUI
struct lv2_plugin_proxy: public plugin_ctl_iface, public plugin_proxy_base, public gui_environment
{
    /// Plugin GTK+ GUI object pointer
    plugin_gui *gui;
    /// Glib source ID for update timer
    int source_id;
    
    lv2_plugin_proxy(const plugin_metadata_iface *md, LV2UI_Write_Function wf, LV2UI_Controller c, const LV2_Feature* const* f)
    : plugin_proxy_base(md, wf, c, f)
    {
        gui = NULL;
        if (instance)
        {
            conditions.insert("directlink");
            conditions.insert("configure");
        }
        conditions.insert("lv2gui");    
    }
    
    virtual float get_param_value(int param_no) {
        if (param_no < 0 || param_no >= param_count)
            return 0;
        return params[param_no];
    }
    
    virtual void set_param_value(int param_no, float value) {
        if (param_no < 0 || param_no >= param_count)
            return;
        send_float_to_host(param_no, value);
    }
    
    virtual bool activate_preset(int bank, int program)
    {
        return false;
    }
    
    /// Override for a method in plugin_ctl_iface - trivial delegation to base class
    virtual char *configure(const char *key, const char *value) { return plugin_proxy_base::configure(key, value); }
    
    virtual float get_level(unsigned int port) { return 0.f; }
    virtual void execute(int command_no) { assert(0); }
    virtual void send_configures(send_configure_iface *sci)
    {    
        if (instance)
        {
            fprintf(stderr, "Send configures...\n");
            instance->send_configures(sci);
        }
        else
            fprintf(stderr, "Configuration not available because of lack of instance-access/data-access\n");
    }
    virtual int send_status_updates(send_updates_iface *sui, int last_serial)
    { 
        if (instance)
            return instance->send_status_updates(sui, last_serial);
        else // no status updates because of lack of instance-access/data-access
            return 0;
    }
    virtual const plugin_metadata_iface *get_metadata_iface() const { return plugin_metadata; }
    /// Override for a method in plugin_ctl_iface - trivial delegation to base class
    virtual const line_graph_iface *get_line_graph_iface() const { return plugin_proxy_base::get_line_graph_iface(); }
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
    const plugin_metadata_iface *md = plugin_registry::instance().get_by_uri(plugin_uri);
    if (!md)
        return NULL;
    lv2_plugin_proxy *proxy = new lv2_plugin_proxy(md, write_function, controller, features);
    if (!proxy)
        return NULL;
    
    gtk_rc_parse(PKGLIBDIR "calf.rc");
    
    plugin_gui_window *window = new plugin_gui_window(proxy, NULL);
    plugin_gui *gui = new plugin_gui(window);
    const char *xml = proxy->plugin_metadata->get_gui_xml();
    assert(xml);
    *(GtkWidget **)(widget) = gui->create_from_xml(proxy, xml);
    proxy->enable_all_sends();
    if (*(GtkWidget **)(widget))
        proxy->source_id = g_timeout_add_full(G_PRIORITY_LOW, 1000/30, plugin_on_idle, gui, NULL); // 30 fps should be enough for everybody    
    gui->show_rack_ears(proxy->get_config()->rack_ears);
    
    return (LV2UI_Handle)gui;
}

void gui_cleanup(LV2UI_Handle handle)
{
    plugin_gui *gui = (plugin_gui *)handle;
    lv2_plugin_proxy *proxy = dynamic_cast<lv2_plugin_proxy *>(gui->plugin);
    if (proxy->source_id)
        g_source_remove(proxy->source_id);
    delete gui;
}

void gui_port_event(LV2UI_Handle handle, uint32_t port, uint32_t buffer_size, uint32_t format, const void *buffer)
{
    plugin_gui *gui = (plugin_gui *)handle;
    lv2_plugin_proxy *proxy = dynamic_cast<lv2_plugin_proxy *>(gui->plugin);
    assert(proxy);
    float v = *(float *)buffer;
    int param = port - proxy->plugin_metadata->get_param_port_offset();
    if (param >= proxy->plugin_metadata->get_param_count())
        return;
    if (!proxy->sends[param])
        return;
    if (fabs(gui->plugin->get_param_value(param) - v) < 0.00001)
        return;
    {
        TempSendSetter _a_(proxy->sends[param], false);
        gui->set_param_value(param, v);
    }
}

const void *gui_extension(const char *uri)
{
    return NULL;
}

const LV2UI_Descriptor* lv2ui_descriptor(uint32_t index)
{
    static LV2UI_Descriptor gtkgui;
    gtkgui.URI = "http://calf.sourceforge.net/plugins/gui/gtk2-gui";
    gtkgui.instantiate = gui_instantiate;
    gtkgui.cleanup = gui_cleanup;
    gtkgui.port_event = gui_port_event;
    gtkgui.extension_data = gui_extension;
    if (!index--)
        return &gtkgui;
    
    return NULL;
}

