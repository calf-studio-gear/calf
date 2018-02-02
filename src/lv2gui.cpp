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
#include <calf/lv2_atom.h>
#include <calf/lv2_data_access.h>
#include <calf/lv2_options.h>
#include <calf/lv2_ui.h>
#include <calf/lv2_urid.h>
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
    /// URID map feature
    const LV2_URID_Map *urid_map;
    /// External UI host feature (must be set when instantiating external UI plugins)
    lv2_external_ui_host *ext_ui_host;
    bool atom_present;
    uint32_t property_type, string_type, event_transfer;
    
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
    /// Signal handler for main widget destroyed
    gulong widget_destroyed_signal;
    /// Signal handler for external window destroyed
    gulong window_destroyed_signal;
    
    plugin_proxy_base(const plugin_metadata_iface *metadata, LV2UI_Write_Function wf, LV2UI_Controller c, const LV2_Feature* const* features);

    /// Send a float value to a control port in the host
    void send_float_to_host(int param_no, float value);
    
    /// Send a string value to a string port in the host, by name (configure-like mechanism)
    char *configure(const char *key, const char *value);

    /// Obtain the list of variables from the plugin
    void send_configures(send_configure_iface *sci);

    /// Enable sending to host for all ports
    void enable_all_sends();
    
    /// Obtain instance pointers
    void resolve_instance();

    /// Obtain line graph interface if available
    const line_graph_iface *get_line_graph_iface() const;
    
    /// Obtain phase graph interface if available
    const phase_graph_iface *get_phase_graph_iface() const;
    
    /// Map an URI to an integer value using a given URID map
    uint32_t map_urid(const char *uri);


};

plugin_proxy_base::plugin_proxy_base(const plugin_metadata_iface *metadata, LV2UI_Write_Function wf, LV2UI_Controller c, const LV2_Feature* const* features)
  : instance_handle(NULL),
    data_access(NULL),
    urid_map(NULL),
    ext_ui_host(NULL),
    instance(NULL)
{
    plugin_metadata = metadata;
    
    write_function = wf;
    controller = c;
    
    atom_present = true; // XXXKF
    
    param_count = metadata->get_param_count();
    param_offset = metadata->get_param_port_offset();
    widget_destroyed_signal = 0;
    window_destroyed_signal = 0;
    
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

uint32_t plugin_proxy_base::map_urid(const char *uri)
{
    if (!urid_map)
        return 0;
    return urid_map->map(urid_map->handle, uri);
}

const line_graph_iface *plugin_proxy_base::get_line_graph_iface() const
{
    if (instance)
        return instance->get_line_graph_iface();
    return NULL;
}

const phase_graph_iface *plugin_proxy_base::get_phase_graph_iface() const
{
    if (instance)
        return instance->get_phase_graph_iface();
    return NULL;
}

char *plugin_proxy_base::configure(const char *key, const char *value)
{
    if (atom_present && event_transfer && string_type && property_type)
    {
        std::string pred = std::string("urn:calf:") + key;
        uint32_t ss = strlen(value);
        uint32_t ts = ss + 1 + sizeof(LV2_Atom_Property);
        char *temp = new char[ts];
        LV2_Atom_Property *prop = (LV2_Atom_Property *)temp;
        prop->atom.type = property_type;
        prop->atom.size = ts - sizeof(LV2_Atom);
        prop->body.key = map_urid(pred.c_str());
        prop->body.context = 0;
        prop->body.value.size = ss + 1;
        prop->body.value.type = string_type;
        memcpy(temp + sizeof(LV2_Atom_Property), value, ss + 1);
        write_function(controller, param_count + param_offset, ts, event_transfer, temp);
        delete []temp;
        return NULL;
    }
    else if (instance)
        return instance->configure(key, value);
    else
        return strdup("Configuration not available because of lack of instance-access/data-access");
}

void plugin_proxy_base::send_configures(send_configure_iface *sci)
{
    if (atom_present && event_transfer && string_type && property_type)
    {
        struct event_content
        {
            LV2_Atom_String str;
            char buf[8];
        } ec;
        ec.str.atom.type = string_type;
        ec.str.atom.size = 2;
        strcpy(ec.buf, "?");
        write_function(controller, param_count + param_offset, sizeof(LV2_Atom_String) + 2, event_transfer, &ec);
    }
    else if (instance)
    {
        fprintf(stderr, "Send configures...\n");
        instance->send_configures(sci);
    }
    else
        fprintf(stderr, "Configuration not available because of lack of instance-access/data-access\n");
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
        source_id = 0;
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
        plugin_proxy_base::send_configures(sci);
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
    
    /// Override for a method in plugin_ctl_iface - trivial delegation to base class
    virtual const phase_graph_iface *get_phase_graph_iface() const { return plugin_proxy_base::get_phase_graph_iface(); }
};

static gboolean plugin_on_idle(void *data)
{
    plugin_gui *self = (plugin_gui *)data;
    if (self->optwidget) {
        self->on_idle();
        return TRUE;
    } else {
        return FALSE;
    }
}

static void on_gui_widget_destroy(GtkWidget*, gpointer data)
{
    plugin_gui *gui = (plugin_gui *)data;
    // Remove the reference to the widget, so that it's not being manipulated
    // after it's been destroyed
    gui->optwidget = NULL;
}

LV2UI_Handle gui_instantiate(const struct _LV2UI_Descriptor* descriptor,
                          const char*                     plugin_uri,
                          const char*                     bundle_path,
                          LV2UI_Write_Function            write_function,
                          LV2UI_Controller                controller,
                          LV2UI_Widget*                   widget,
                          const LV2_Feature* const*       features)
{
    static int argc = 0;
    gtk_init(&argc, NULL);

    const plugin_metadata_iface *md = plugin_registry::instance().get_by_uri(plugin_uri);
    if (!md)
        return NULL;
    lv2_plugin_proxy *proxy = new lv2_plugin_proxy(md, write_function, controller, features);
    if (!proxy)
        return NULL;
    
    plugin_gui_window *window = new plugin_gui_window(proxy, NULL);
    plugin_gui *gui = new plugin_gui(window);
    
    const char *xml = proxy->plugin_metadata->get_gui_xml("gui");
    assert(xml);
    gui->optwidget = gui->create_from_xml(proxy, xml);
    proxy->enable_all_sends();
    if (gui->optwidget)
    {
        GtkWidget *decoTable = window->decorate(gui->optwidget);
        GtkWidget *eventbox  = gtk_event_box_new();
        gtk_widget_set_name( GTK_WIDGET(eventbox), "Calf-Plugin" );
        gtk_container_add( GTK_CONTAINER(eventbox), decoTable );
        gtk_widget_show_all(eventbox);
        gui->optwidget = eventbox;
        proxy->source_id = g_timeout_add_full(G_PRIORITY_LOW, 1000/30, plugin_on_idle, gui, NULL); // 30 fps should be enough for everybody    
        proxy->widget_destroyed_signal = g_signal_connect(G_OBJECT(gui->optwidget), "destroy", G_CALLBACK(on_gui_widget_destroy), (gpointer)gui);
    }
    std::string rcf = PKGLIBDIR "/styles/" + proxy->get_config()->style + "/gtk.rc";
    gtk_rc_parse(rcf.c_str());
    window->show_rack_ears(proxy->get_config()->rack_ears);
    
    *(GtkWidget **)(widget) = gui->optwidget;

    // find window title
    const LV2_Options_Option* options = NULL;
    const LV2_URID_Map* uridMap = NULL;

    for (int i=0; features[i]; i++)
    {
        if (!strcmp(features[i]->URI, LV2_OPTIONS__options))
            options = (const LV2_Options_Option*)features[i]->data;
        else if (!strcmp(features[i]->URI, LV2_URID_MAP_URI))
            uridMap = (const LV2_URID_Map*)features[i]->data;
    }

    if (!options || !uridMap)
        return (LV2UI_Handle)gui;

    const uint32_t uridWindowTitle = uridMap->map(uridMap->handle, LV2_UI__windowTitle);
    proxy->string_type = uridMap->map(uridMap->handle, LV2_ATOM__String);
    proxy->property_type = uridMap->map(uridMap->handle, LV2_ATOM__Property);
    proxy->event_transfer = uridMap->map(uridMap->handle, LV2_ATOM__eventTransfer);
    proxy->urid_map = uridMap;

    proxy->send_configures(gui);

    if (!uridWindowTitle)
        return (LV2UI_Handle)gui;

    for (int i=0; options[i].key; i++)
    {
        if (options[i].key == uridWindowTitle)
        {
            const char* windowTitle = (const char*)options[i].value;
            gui->opttitle = strdup(windowTitle);
            break;
        }
    }

    return (LV2UI_Handle)gui;
}

void gui_cleanup(LV2UI_Handle handle)
{
    plugin_gui *gui = (plugin_gui *)handle;
    lv2_plugin_proxy *proxy = dynamic_cast<lv2_plugin_proxy *>(gui->plugin);
    if (proxy->source_id)
        g_source_remove(proxy->source_id);
    // If the widget still exists, remove the handler
    if (gui->optwidget)
    {
        g_signal_handler_disconnect(gui->optwidget, proxy->widget_destroyed_signal);
        proxy->widget_destroyed_signal = 0;
    }
    gui->destroy_child_widgets(gui->optwidget);
    gui->optwidget = NULL;

    if (gui->opttitle)
    {
        free((void*)gui->opttitle);

        // idle is no longer called after this, so make sure all events are handled now
        while (gtk_events_pending())
            gtk_main_iteration();
    }

    delete gui;
}

void gui_port_event(LV2UI_Handle handle, uint32_t port, uint32_t buffer_size, uint32_t format, const void *buffer)
{
    plugin_gui *gui = (plugin_gui *)handle;
    if (gui->optclosed)
        return;
    lv2_plugin_proxy *proxy = dynamic_cast<lv2_plugin_proxy *>(gui->plugin);
    assert(proxy);
    float v = *(float *)buffer;
    int param = port - proxy->plugin_metadata->get_param_port_offset();
    if (param < 0 || param >= proxy->plugin_metadata->get_param_count())
    {
        if (format == proxy->event_transfer)
        {
            LV2_Atom *atom = (LV2_Atom *)buffer;
            if (atom->type == proxy->string_type)
                printf("Param %d string %s\n", param, (char *)LV2_ATOM_CONTENTS(LV2_Atom_String, atom));
            else if (atom->type == proxy->property_type)
            {
                LV2_Atom_Property_Body *prop = (LV2_Atom_Property_Body *)LV2_ATOM_BODY(atom);
                printf("Param %d key %d string %s\n", param, prop->key, (const char *)LV2_ATOM_CONTENTS(LV2_Atom_Property, atom));
            }
            else
                printf("Param %d type %d\n", param, atom->type);
        }
        return;
    }
    if (!proxy->sends[param])
        return;
    if (fabs(gui->plugin->get_param_value(param) - v) < 0.00001)
        return;
    {
        TempSendSetter _a_(proxy->sends[param], false);
        gui->set_param_value(param, v);
    }
}

void gui_destroy(GtkWidget*, gpointer data)
{
    plugin_gui *gui = (plugin_gui *)data;

    gui->optclosed = true;
    gui->optwindow = NULL;
}

int gui_show(LV2UI_Handle handle)
{
    plugin_gui *gui = (plugin_gui *)handle;
    lv2_plugin_proxy *proxy = dynamic_cast<lv2_plugin_proxy *>(gui->plugin);

    if (! gui->optwindow)
    {
        gui->optwindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        proxy->window_destroyed_signal = g_signal_connect(G_OBJECT(gui->optwindow), "destroy", G_CALLBACK(gui_destroy), (gpointer)gui);

        if (gui->optwidget)
            gtk_container_add(GTK_CONTAINER(gui->optwindow), gui->optwidget);

        if (gui->opttitle)
            gtk_window_set_title(GTK_WINDOW(gui->optwindow), gui->opttitle);

        gtk_window_set_resizable(GTK_WINDOW(gui->optwindow), false);
    }

    gtk_widget_show_all(gui->optwindow);
    gtk_window_present(GTK_WINDOW(gui->optwindow));

    return 0;
}

int gui_hide(LV2UI_Handle handle)
{
    plugin_gui *gui = (plugin_gui *)handle;
    lv2_plugin_proxy *proxy = dynamic_cast<lv2_plugin_proxy *>(gui->plugin);

    if (gui->optwindow)
    {
        g_signal_handler_disconnect(gui->optwindow, proxy->window_destroyed_signal);
        proxy->window_destroyed_signal = 0;

        gtk_widget_hide_all(gui->optwindow);
        gtk_widget_destroy(gui->optwindow);
        gui->optwindow = NULL;
        gui->optclosed = true;

        // idle is no longer called after this, so make sure all events are handled now
        while (gtk_events_pending())
            gtk_main_iteration();
    }

    return 0;
}

int gui_idle(LV2UI_Handle handle)
{
    plugin_gui *gui = (plugin_gui *)handle;

    if (gui->optclosed)
        return 1;

    if (gui->optwindow)
    {
        while (gtk_events_pending())
            gtk_main_iteration();
    }

    return 0;
}

const void *gui_extension(const char *uri)
{
    static const LV2UI_Show_Interface show = { gui_show, gui_hide };
    static const LV2UI_Idle_Interface idle = { gui_idle };
    if (!strcmp(uri, LV2_UI__showInterface))
        return &show;
    if (!strcmp(uri, LV2_UI__idleInterface))
        return &idle;
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
    
    static LV2UI_Descriptor gtkguireq;
    gtkguireq.URI = "http://calf.sourceforge.net/plugins/gui/gtk2-gui-req";
    gtkguireq.instantiate = gui_instantiate;
    gtkguireq.cleanup = gui_cleanup;
    gtkguireq.port_event = gui_port_event;
    gtkguireq.extension_data = gui_extension;
    if (!index--)
        return &gtkguireq;
    
    return NULL;
}
