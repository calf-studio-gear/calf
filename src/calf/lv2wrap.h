/* Calf DSP Library
 * LV2 wrapper templates
 *
 * Copyright (C) 2001-2015 Krzysztof Foltman
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
 * Boston, MA 02111-1307, USA.
 */
#ifndef CALF_LV2WRAP_H
#define CALF_LV2WRAP_H

#if USE_LV2

#include <string>
#include <vector>
#include "lv2/lv2plug.in/ns/lv2core/lv2.h"
#include <calf/giface.h>
#include <calf/lv2_atom.h>
#include <calf/lv2_atom_util.h>
#include <calf/lv2_midi.h>
#include <calf/lv2_state.h>
#include <calf/lv2_options.h>
#include <calf/lv2_progress.h>
#include <calf/lv2_urid.h>
#include <string.h>

namespace calf_plugins {

struct lv2_instance: public plugin_ctl_iface, public progress_report_iface
{
    const plugin_metadata_iface *metadata;
    audio_module_iface *module;
    bool set_srate;
    int srate_to_set;
    LV2_Atom_Sequence *event_in_data, *event_out_data;
    uint32_t event_out_capacity;
    LV2_URID_Map *urid_map;
    uint32_t midi_event_type, property_type, string_type, sequence_type;
    LV2_Progress *progress_report_feature;
    LV2_Options_Interface *options_feature;
    float **ins, **outs, **params;
    int in_count;
    int out_count;
    int real_param_count;
    struct lv2_var
    {
        std::string name;
        uint32_t mapped_uri;
    };
    std::vector<lv2_var> vars;
    std::map<uint32_t, int> uri_to_var;

    lv2_instance(audio_module_iface *_module);
    void lv2_instantiate(const LV2_Descriptor * Descriptor, double sample_rate, const char *bundle_path, const LV2_Feature *const *features);

/// This, and not Module::post_instantiate, is actually called by lv2_instantiate
    void post_instantiate();
    
    virtual bool activate_preset(int bank, int program) { 
        return false;
    }
    virtual float get_level(unsigned int port) { return 0.f; }
    virtual void execute(int cmd_no) {
        module->execute(cmd_no);
    }
    virtual void report_progress(float percentage, const std::string &message) {
        if (progress_report_feature)
            (*progress_report_feature->progress)(progress_report_feature->context, percentage, !message.empty() ? message.c_str() : NULL);
    }
    void send_configures(send_configure_iface *sci) { 
        module->send_configures(sci);
    }
    LV2_State_Status state_save(LV2_State_Store_Function store, LV2_State_Handle handle,
        uint32_t flags, const LV2_Feature *const * features);
    void impl_restore(LV2_State_Retrieve_Function retrieve, void *callback_data);
    char *configure(const char *key, const char *value) { 
        // disambiguation - the plugin_ctl_iface version is just a stub, so don't use it
        return module->configure(key, value);
    }
    /* Loosely based on David Robillard's lv2_atom_sequence_append_event */
    inline void* add_event_to_seq(uint64_t time_frames, uint32_t type, uint32_t data_size)
    {
        uint32_t remaining = event_out_capacity - event_out_data->atom.size;
        uint32_t hdr_size = sizeof(LV2_Atom_Event);
        if (remaining < sizeof(LV2_Atom_Event) + data_size)
            return NULL;
        LV2_Atom_Event *event = lv2_atom_sequence_end(&event_out_data->body, event_out_data->atom.size);
        event->time.frames = time_frames;
        event->body.type = type;
        event->body.size = data_size;
        event_out_data->atom.size += lv2_atom_pad_size(hdr_size + data_size);
        return ((uint8_t *)event) + hdr_size;
    }
    void output_event_string(const char *str, int len = -1);
    void output_event_property(const char *key, const char *value);
    void process_event_string(const char *str);
    void process_event_property(const LV2_Atom_Property *prop);
    void process_events(uint32_t &offset);
    void run(uint32_t SampleCount, bool has_simulate_stereo_input_flag);
    virtual float get_param_value(int param_no)
    {
        // XXXKF hack
        if (param_no >= real_param_count)
            return 0;
        return (*params)[param_no];
    }
    virtual void set_param_value(int param_no, float value)
    {
        // XXXKF hack
        if (param_no >= real_param_count)
            return;
        *params[param_no] = value;
    }
    virtual const plugin_metadata_iface *get_metadata_iface() const { return metadata; }
    virtual const line_graph_iface *get_line_graph_iface() const { return module->get_line_graph_iface(); }
    virtual const phase_graph_iface *get_phase_graph_iface() const { return module->get_phase_graph_iface(); }
    virtual int send_status_updates(send_updates_iface *sui, int last_serial) { return module->send_status_updates(sui, last_serial); }
};

struct LV2_Calf_Descriptor {
    plugin_ctl_iface *(*get_pci)(LV2_Handle Instance);
};

struct store_lv2_state: public send_configure_iface
{
    LV2_State_Store_Function store;
    void *callback_data;
    lv2_instance *inst;
    uint32_t string_data_type;
    
    virtual void send_configure(const char *key, const char *value);
};


template<class Module>
struct lv2_wrapper
{
    typedef lv2_instance instance;
    static LV2_Descriptor descriptor;
    static LV2_Calf_Descriptor calf_descriptor;
    static LV2_State_Interface state_iface;
    std::string uri;
    
    lv2_wrapper()
    {
        ladspa_plugin_info &info = Module::plugin_info;
        uri = "http://calf.sourceforge.net/plugins/" + std::string(info.label);
        descriptor.URI = uri.c_str();
        descriptor.instantiate = cb_instantiate;
        descriptor.connect_port = cb_connect;
        descriptor.activate = cb_activate;
        descriptor.run = cb_run;
        descriptor.deactivate = cb_deactivate;
        descriptor.cleanup = cb_cleanup;
        descriptor.extension_data = cb_ext_data;
        state_iface.save = cb_state_save;
        state_iface.restore = cb_state_restore;
        calf_descriptor.get_pci = cb_get_pci;
    }

    static void cb_connect(LV2_Handle Instance, uint32_t port, void *DataLocation)
    {
        instance *const mod = (instance *)Instance;
        const plugin_metadata_iface *md = mod->metadata;
        unsigned long ins = md->get_input_count();
        unsigned long outs = md->get_output_count();
        unsigned long params = md->get_param_count();
        bool has_event_in = md->get_midi() || md->sends_live_updates();
        bool has_event_out = md->sends_live_updates();

        if (port < ins)
            mod->ins[port] = (float *)DataLocation;
        else if (port < ins + outs)
            mod->outs[port - ins] = (float *)DataLocation;
        else if (port < ins + outs + params) {
            int i = port - ins - outs;
            mod->params[i] = (float *)DataLocation;
        }
        else if (has_event_in && port == ins + outs + params) {
            mod->event_in_data = (LV2_Atom_Sequence *)DataLocation;
        }
        else if (has_event_out && port == ins + outs + params + (has_event_in ? 1 : 0)) {
            mod->event_out_data = (LV2_Atom_Sequence *)DataLocation;
        }
    }

    static void cb_activate(LV2_Handle Instance)
    {
        instance *const mod = (instance *)Instance;
        mod->set_srate = true;
    }
    
    static void cb_deactivate(LV2_Handle Instance)
    {
        instance *const mod = (instance *)Instance;
        mod->module->deactivate();
    }

    static LV2_Handle cb_instantiate(const LV2_Descriptor * Descriptor, double sample_rate, const char *bundle_path, const LV2_Feature *const *features)
    {
        instance *mod = new instance(new Module);
        mod->lv2_instantiate(Descriptor, sample_rate, bundle_path, features);
        return mod;
    }
    static plugin_ctl_iface *cb_get_pci(LV2_Handle Instance)
    {
        return static_cast<plugin_ctl_iface *>(Instance);
    }

    static void cb_run(LV2_Handle Instance, uint32_t SampleCount)
    {
        instance *const inst = (instance *)Instance;
        inst->run(SampleCount, Module::simulate_stereo_input);
    }
    static void cb_cleanup(LV2_Handle Instance)
    {
        instance *const mod = (instance *)Instance;
        delete mod->module;
        delete mod;
    }

    static const void *cb_ext_data(const char *URI)
    {
        if (!strcmp(URI, "http://foltman.com/ns/calf-plugin-instance"))
            return &calf_descriptor;
        if (!strcmp(URI, LV2_STATE__interface))
            return &state_iface;
        return NULL;
    }
    static LV2_State_Status cb_state_save(
        LV2_Handle Instance, LV2_State_Store_Function store, LV2_State_Handle handle,
        uint32_t flags, const LV2_Feature *const * features)
    {
        instance *const inst = (instance *)Instance;
        return inst->state_save(store, handle, flags, features);
    }
    static LV2_State_Status cb_state_restore(
        LV2_Handle Instance, LV2_State_Retrieve_Function retrieve, LV2_State_Handle callback_data,
        uint32_t flags, const LV2_Feature *const * features)
    {
        instance *const inst = (instance *)Instance;
        inst->impl_restore(retrieve, callback_data);
        return LV2_STATE_SUCCESS;
    }
    
    static lv2_wrapper &get() { 
        static lv2_wrapper instance;
        return instance;
    }
};

};

#endif
#endif
