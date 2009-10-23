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
#include <calf/lv2_data_access.h>
#include <calf/lv2_string_port.h>
#include <calf/lv2_ui.h>
#include <calf/lv2_uri_map.h>
#include <calf/preset_gui.h>
#include <calf/utils.h>
#include <calf/lv2helpers.h>

using namespace std;
using namespace dsp;
using namespace calf_plugins;
using namespace calf_utils;

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
    
    plugin_proxy(plugin_metadata_iface *md)
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
    
    virtual line_graph_iface *get_line_graph_iface() {
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
    vector<plugin_metadata_iface *> plugins;
    get_all_plugins(plugins);
    const char *label = plugin_uri + sizeof("http://calf.sourceforge.net/plugins/") - 1;
    plugin_proxy *proxy = NULL;
    for (unsigned int i = 0; i < plugins.size(); i++)
    {
        if (!strcmp(plugins[i]->get_plugin_info().label, label))
        {
            proxy = new plugin_proxy(plugins[i]);
            break;
        }
    }
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

class small_plugin_gui: public uri_map_access
{
public:
    LV2UI_Write_Function write_function;
    LV2UI_Controller controller;
    GtkWidget **widget_ptr;

    void write(int port, const void *data, uint32_t size, uint32_t format)
    {
        (*write_function)(controller, port, size, format, &data);
    }

    virtual void use_feature(const char *URI, void *feature) {
    }
    
    virtual void parse_features(const LV2_Feature* const*features) {
        if (features) {
            for(;*features;features++)
                use_feature((*features)->URI, (*features)->data);
        }
    }
        
    virtual GtkWidget *create_widget()=0;

    virtual void init(const char* plugin_uri, const char* bundle_path, 
                LV2UI_Write_Function write_function, LV2UI_Controller controller,
                LV2UI_Widget* widget, const LV2_Feature* const*features)
    {
        this->write_function = write_function;
        this->controller = controller;
        widget_ptr = (GtkWidget **)widget;
        parse_features(features);
        *widget_ptr = create_widget();
    }
    
    virtual ~small_plugin_gui() {}
};

class msg_read_gui: public message_mixin<small_plugin_gui>
{
    GtkWidget *editor;
    uint32_t set_float_msg, float_type;
    
    static void bang(GtkWidget *widget, msg_read_gui *gui)
    {
        struct payload {
            uint32_t selector;
            uint32_t serial_no;
            uint32_t data_size;
            uint32_t parg_count;
            uint32_t data_type;
            float data_value;
            uint32_t narg_count;
        } data;
        data.selector = gui->set_float_msg;
        data.serial_no = 1337;
        data.data_size = 16;
        data.parg_count = 1;
        data.data_type = gui->float_type;
        data.data_value = atof(gtk_entry_get_text(GTK_ENTRY(gui->editor)));
        data.narg_count = 0;
        gui->write(0, &data, sizeof(data), gui->message_event_type);
    }
    virtual void map_uris()
    {
        message_mixin<small_plugin_gui>::map_uris();
        set_float_msg = map_uri("http://lv2plug.in/ns/dev/msg", "http://foltman.com/garbage/setFloat");
        float_type = map_uri("http://lv2plug.in/ns/dev/types", "http://lv2plug.in/ns/dev/types#float");
    }
    virtual GtkWidget *create_widget()
    {
        editor = gtk_entry_new();
        GtkWidget *button = gtk_button_new_with_label("Bang!");
        GtkWidget *vbox = gtk_vbox_new(false, 10);
        gtk_box_pack_start(GTK_BOX(vbox), editor, true, true, 5);
        gtk_box_pack_start(GTK_BOX(vbox), button, true, true, 5);
        GtkWidget *frame = gtk_frame_new("GUI");
        gtk_container_add(GTK_CONTAINER(frame), vbox);
        gtk_widget_queue_resize(frame);
        gtk_signal_connect( GTK_OBJECT(button), "clicked", G_CALLBACK(bang), this);
        return frame;
    }
};

LV2UI_Handle sgui_instantiate(const struct _LV2UI_Descriptor* descriptor,
                          const char*                     plugin_uri,
                          const char*                     bundle_path,
                          LV2UI_Write_Function            write_function,
                          LV2UI_Controller                controller,
                          LV2UI_Widget*                   widget,
                          const LV2_Feature* const*       features)
{
    small_plugin_gui *gui = NULL;
    if (!strcmp(plugin_uri, "http://calf.sourceforge.net/small_plugins/msgread_e"))
        gui = new msg_read_gui;
    else
        return NULL;
    gui->init(plugin_uri, bundle_path, write_function, controller, widget, features);
    return gui;
}

void sgui_cleanup(LV2UI_Handle handle)
{
    small_plugin_gui *gui = (small_plugin_gui *)handle;
    delete gui;
}

void sgui_port_event(LV2UI_Handle handle, uint32_t port, uint32_t buffer_size, uint32_t format, const void *buffer)
{
}

const void *sgui_extension(const char *uri)
{
    return NULL;
}

///////////////////////////////////////////////////////////////////////////////////////

const LV2UI_Descriptor* lv2ui_descriptor(uint32_t index)
{
    static LV2UI_Descriptor gui, sgui;
    gui.URI = "http://calf.sourceforge.net/plugins/gui/gtk2-gui";
    gui.instantiate = gui_instantiate;
    gui.cleanup = gui_cleanup;
    gui.port_event = gui_port_event;
    gui.extension_data = gui_extension;
    sgui.URI = "http://calf.sourceforge.net/small_plugins/gui/gtk2-gui";
    sgui.instantiate = sgui_instantiate;
    sgui.cleanup = sgui_cleanup;
    sgui.port_event = sgui_port_event;
    sgui.extension_data = sgui_extension;
    switch(index) {
        case 0:
            return &gui;
        case 1:
            return &sgui;
        default:
            return NULL;
    }
}

