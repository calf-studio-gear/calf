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
#include <getopt.h>
#include <stdint.h>
#include <stdlib.h>
#include <config.h>
#include <calf/giface.h>
#include <calf/gui.h>
#include <calf/modules.h>
#include <calf/modules_dev.h>
#include <calf/benchmark.h>
#include <calf/lv2-gui.h>
#include <calf/preset_gui.h>

using namespace std;
using namespace dsp;
using namespace synth;

struct plugin_proxy_base: public plugin_ctl_iface
{
    LV2UI_Write_Function write_function;
    LV2UI_Controller controller;
    
    bool send;
    plugin_gui *gui;
    
    plugin_proxy_base()
    {
        send = false;
        gui = NULL;
    }
    
    void setup(LV2UI_Write_Function wfn, LV2UI_Controller ctl)
    {
        write_function = wfn;
        controller = ctl;
    }
    
};

template<class Module>
struct plugin_proxy: public plugin_proxy_base, public line_graph_iface
{
    float params[Module::param_count];
    plugin_proxy()
    {
        send = true;
        for (int i = 0; i < Module::param_count; i++)
            params[i] = Module::param_props[i].def_value;
    }
    
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
        if (send) {
            send = false;
            write_function(controller, param_no + Module::in_count + Module::out_count, sizeof(float), &params[param_no]);
            send = true;
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
        return false;
    }
    virtual bool get_graph(int index, int subindex, float *data, int points, cairo_t *context) {
        return Module::get_static_graph(index, subindex, params[index], data, points, context);
    }
};

plugin_proxy_base *create_plugin_proxy(const char *effect_name)
{
    if (!strcmp(effect_name, "Reverb"))
        return new plugin_proxy<reverb_audio_module>();
    else if (!strcmp(effect_name, "Flanger"))
        return new plugin_proxy<flanger_audio_module>();
    else if (!strcmp(effect_name, "Filter"))
        return new plugin_proxy<filter_audio_module>();
    else if (!strcmp(effect_name, "Monosynth"))
        return new plugin_proxy<monosynth_audio_module>();
    else if (!strcmp(effect_name, "VintageDelay"))
        return new plugin_proxy<vintage_delay_audio_module>();
#ifdef ENABLE_EXPERIMENTAL
    else if (!strcmp(effect_name, "Organ"))
        return new plugin_proxy<organ_audio_module>();
    else if (!strcmp(effect_name, "RotarySpeaker"))
        return new plugin_proxy<rotary_speaker_audio_module>();
#endif
    else
        return NULL;
}

LV2UI_Descriptor gui;

LV2UI_Handle gui_instantiate(const struct _LV2UI_Descriptor* descriptor,
                          const char*                     plugin_uri,
                          const char*                     bundle_path,
                          LV2UI_Write_Function            write_function,
                          LV2UI_Controller                controller,
                          LV2UI_Widget*                   widget,
                          const LV2_Feature* const*       features)
{
    plugin_proxy_base *proxy = create_plugin_proxy(plugin_uri + sizeof("http://calf.sourceforge.net/plugins/") - 1);
    proxy->setup(write_function, controller);
    // dummy window
    plugin_gui_window *window = new plugin_gui_window;
    window->conditions.insert("lv2gui");
    plugin_gui *gui = new plugin_gui(window);
    *(GtkWidget **)(widget) = gui->create(proxy);
    
    return (LV2UI_Handle)gui;
}

void gui_cleanup(LV2UI_Handle handle)
{
    plugin_gui *gui = (plugin_gui *)handle;
    delete gui;
}

void gui_port_event(LV2UI_Handle handle, uint32_t port, uint32_t buffer_size, const void *buffer)
{
    plugin_gui *gui = (plugin_gui *)handle;
    float v = *(float *)buffer;
    printf("spv %d %f\n", port, v);
    port -= gui->plugin->get_param_port_offset();
    if (port >= (uint32_t)gui->plugin->get_param_count())
        return;
    gui->set_param_value(port, v);
}

const void *gui_extension(const char *uri)
{
    return NULL;
}

namespace synth {

// this function is normally implemented in preset_gui.cpp, but we're not using that file
void activate_preset(GtkAction *action, activate_preset_params *params)
{
}

};

const LV2UI_Descriptor* lv2ui_descriptor(uint32_t index)
{
    gui.URI = "http://calf.sourceforge.net/plugins/gui/gtk2-gui";
    gui.instantiate = gui_instantiate;
    gui.cleanup = gui_cleanup;
    gui.port_event = gui_port_event;
    gui.extension_data = gui_extension;
    switch(index) {
        case 0:
            return &gui;
        default:
            return NULL;
    }
}

