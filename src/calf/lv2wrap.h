/* Calf DSP Library
 * LV2 wrapper templates
 *
 * Copyright (C) 2001-2008 Krzysztof Foltman
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
#ifndef CALF_LV2WRAP_H
#define CALF_LV2WRAP_H

#if USE_LV2

#include <string>
#include <lv2.h>
#include <calf/giface.h>
#include <calf/lv2-midiport.h>
#include <calf/lv2_event.h>
#include <calf/lv2_uri_map.h>
#include <calf/lv2_contexts.h>

namespace synth {

template<class Module>
struct lv2_instance: public Module, public plugin_ctl_iface
{
    bool set_srate;
    int srate_to_set;
    LV2_MIDI *midi_data;
    LV2_Event_Buffer *event_data;
    LV2_URI_Map_Feature *uri_map;
    LV2_Event_Feature *event_feature;
    uint32_t midi_event_type;
    lv2_instance()
    {
        for (int i=0; i < Module::in_count; i++)
            Module::ins[i] = NULL;
        for (int i=0; i < Module::out_count; i++)
            Module::outs[i] = NULL;
        for (int i=0; i < Module::param_count; i++)
            Module::params[i] = NULL;
        uri_map = NULL;
        midi_data = NULL;
        event_data = NULL;
        midi_event_type = 0xFFFFFFFF;
        set_srate = true;
        srate_to_set = 44100;
    }
    virtual parameter_properties *get_param_props(int param_no)
    {
        return &Module::param_props[param_no];
    }
    virtual float get_param_value(int param_no)
    {
        return *Module::params[param_no];
    }
    virtual void set_param_value(int param_no, float value)
    {
        *Module::params[param_no] = value;
    }
    virtual int get_param_count()
    {
        return Module::param_count;
    }
    virtual int get_param_port_offset() 
    {
        return Module::in_count + Module::out_count;
    }
    virtual const char *get_gui_xml() {
        return Module::get_gui_xml();
    }
    virtual line_graph_iface *get_line_graph_iface()
    {
        return this;
    }
    virtual bool activate_preset(int bank, int program) { 
        return false;
    }
    virtual const char *get_name()
    {
        return Module::get_name();
    }
    virtual const char *get_id()
    {
        return Module::get_id();
    }
    virtual const char *get_label()
    {
        return Module::get_label();
    }
    virtual int get_input_count() { return Module::in_count; }
    virtual int get_output_count() { return Module::out_count; }
    virtual bool get_midi() { return Module::support_midi; }
    virtual float get_level(unsigned int port) { return 0.f; }
    virtual void execute(int cmd_no) {
        Module::execute(cmd_no);
    }
    void send_configures(send_configure_iface *sci) { 
        Module::send_configures(sci);
    }
    virtual void clear_preset() {
        // This is never called in practice, at least for now
        // However, it will change when presets are implemented
        for (int i=0; i < Module::param_count; i++)
            *Module::params[i] = Module::param_props[i].def_value;
        /*
        const char **p = Module::get_default_configure_vars();
        if (p)
        {
            for(; p[0]; p += 2)
                configure(p[0], p[1]);
        }
        */
    }
};

template<class Module>
struct lv2_wrapper
{
    typedef lv2_instance<Module> instance;
    static LV2_Descriptor descriptor;
    std::string uri;
    
    lv2_wrapper(ladspa_plugin_info &info)
    {
        uri = "http://calf.sourceforge.net/plugins/" + std::string(info.label);
        descriptor.URI = uri.c_str();
        descriptor.instantiate = cb_instantiate;
        descriptor.connect_port = cb_connect;
        descriptor.activate = cb_activate;
        descriptor.run = cb_run;
        descriptor.deactivate = cb_deactivate;
        descriptor.cleanup = cb_cleanup;
        descriptor.extension_data = cb_ext_data;
    }

    static void cb_connect(LV2_Handle Instance, uint32_t port, void *DataLocation) {
        unsigned long ins = Module::in_count;
        unsigned long outs = Module::out_count;
        unsigned long params = Module::param_count;
        instance *const mod = (instance *)Instance;
        if (port < ins)
            mod->ins[port] = (float *)DataLocation;
        else if (port < ins + outs)
            mod->outs[port - ins] = (float *)DataLocation;
        else if (port < ins + outs + params) {
            int i = port - ins - outs;
            mod->params[i] = (float *)DataLocation;
        }
        else if (Module::support_midi && port == ins + outs + params) {
            mod->event_data = (LV2_Event_Buffer *)DataLocation;
        }
    }

    static void cb_activate(LV2_Handle Instance) {
    }
    
    static void cb_deactivate(LV2_Handle Instance) {
        instance *const mod = (instance *)Instance;
        mod->deactivate();
    }

    static LV2_Handle cb_instantiate(const LV2_Descriptor * Descriptor, double sample_rate, const char *bundle_path, const LV2_Feature *const *features)
    {
        instance *mod = new instance();
        // XXXKF some people use fractional sample rates; we respect them ;-)
        mod->srate_to_set = (uint32_t)sample_rate;
        mod->set_srate = true;
        while(*features)
        {
            if (!strcmp((*features)->URI, LV2_URI_MAP_URI))
            {
                mod->uri_map = (LV2_URI_Map_Feature *)((*features)->data);
                mod->midi_event_type = mod->uri_map->uri_to_id(
                    mod->uri_map->callback_data, 
                    "http://lv2plug.in/ns/ext/event",
                    "http://lv2plug.in/ns/ext/midi#MidiEvent");
            }
            else if (!strcmp((*features)->URI, LV2_EVENT_URI))
            {
                mod->event_feature = (LV2_Event_Feature *)((*features)->data);
            }
            features++;
        }
        return mod;
    }
    static inline void zero_by_mask(Module *module, uint32_t mask, uint32_t offset, uint32_t nsamples)
    {
        for (int i=0; i<Module::out_count; i++) {
            if ((mask & (1 << i)) == 0) {
                dsp::zero(module->outs[i] + offset, nsamples);
            }
        }
    }
    static inline void process_slice(Module *mod, uint32_t offset, uint32_t end)
    {
        while(offset < end)
        {
            uint32_t newend = std::min(offset + MAX_SAMPLE_RUN, end);
            uint32_t out_mask = mod->process(offset, newend - offset, -1, -1);
            zero_by_mask(mod, out_mask, offset, newend - offset);
            offset = newend;
        }
    }

    static void cb_run(LV2_Handle Instance, uint32_t SampleCount) {
        instance *const mod = (instance *)Instance;
        if (mod->set_srate) {
            mod->set_sample_rate(mod->srate_to_set);
            mod->activate();
            mod->set_srate = false;
        }
        mod->params_changed();
        uint32_t offset = 0;
        if (mod->event_data)
        {
            // printf("Event data: count %d\n", mod->event_data->event_count);
            struct LV2_Midi_Event: public LV2_Event {
                unsigned char data[1];
            };
            unsigned char *data = (unsigned char *)(mod->event_data->data);
            for (uint32_t i = 0; i < mod->event_data->event_count; i++) {
                LV2_Midi_Event *item = (LV2_Midi_Event *)data;
                uint32_t ts = item->frames;
                // printf("Event: timestamp %d subframes %d type %d vs %d\n", item->frames, item->subframes, item->type, mod->midi_event_type);
                if (ts > offset)
                {
                    process_slice(mod, offset, ts);
                    offset = ts;
                }
                if (item->type == mod->midi_event_type) 
                {
                    // printf("Midi message %x %x %x %x %d\n", item->data[0], item->data[1], item->data[2], item->data[3], item->size);
                    switch(item->data[0] >> 4)
                    {
                    case 8: mod->note_off(item->data[1], item->data[2]); break;
                    case 9: mod->note_on(item->data[1], item->data[2]); break;
                    case 10: mod->program_change(item->data[1]); break;
                    case 11: mod->control_change(item->data[1], item->data[2]); break;
                    case 14: mod->pitch_bend(item->data[1] + 128 * item->data[2] - 8192); break;
                    }
                }
                else
                if (item->type == 0 && mod->event_feature)
                    mod->event_feature->lv2_event_drop(mod->event_feature->callback_data, item, 0);
                // printf("timestamp %f item size %d first byte %x\n", item->timestamp, item->size, item->data[0]);
                data += ((sizeof(LV2_Event) + item->size + 7))&~7;
            }
        }
        process_slice(mod, offset, SampleCount);
    }
    static void cb_cleanup(LV2_Handle Instance) {
        instance *const mod = (instance *)Instance;
        delete mod;
    }
    static const void *cb_ext_data(const char *URI) {
        return NULL;
    }
};

template<class Module>
struct lv2_small_wrapper
{
    typedef Module instance;
    static LV2_Descriptor descriptor;
    std::string uri;
    
    lv2_small_wrapper(const char *id)
    {
        uri = "http://calf.sourceforge.net/small_plugins/" + std::string(id);
        descriptor.URI = uri.c_str();
        descriptor.instantiate = cb_instantiate;
        descriptor.connect_port = cb_connect;
        descriptor.activate = cb_activate;
        descriptor.run = cb_run;
        descriptor.deactivate = cb_deactivate;
        descriptor.cleanup = cb_cleanup;
        descriptor.extension_data = cb_ext_data;
    }

    static void cb_connect(LV2_Handle Instance, uint32_t port, void *DataLocation) {
        unsigned long ins = Module::in_count;
        unsigned long outs = Module::out_count;
        instance *const mod = (instance *)Instance;
        if (port < ins)
            mod->ins[port] = (float *)DataLocation;
        else if (port < ins + outs)
            mod->outs[port - ins] = (float *)DataLocation;
    }

    static void cb_activate(LV2_Handle Instance) {
        // Note the changed semantics (now more LV2-like)
        instance *const mod = (instance *)Instance;
        mod->activate();
    }
    
    static void cb_deactivate(LV2_Handle Instance) {
        instance *const mod = (instance *)Instance;
        mod->deactivate();
    }

    static LV2_Handle cb_instantiate(const LV2_Descriptor * Descriptor, double sample_rate, const char *bundle_path, const LV2_Feature *const *features)
    {
        instance *mod = new instance();
        // XXXKF some people use fractional sample rates; we respect them ;-)
        mod->set_bundle_path(bundle_path);
        mod->use_features(features);
        mod->set_sample_rate((uint32_t)sample_rate);
        return mod;
    }
    
    static void cb_run(LV2_Handle Instance, uint32_t SampleCount) {
        instance *const mod = (instance *)Instance;
        mod->process(SampleCount);
    }
    
    static void cb_cleanup(LV2_Handle Instance) {
        instance *const mod = (instance *)Instance;
        delete mod;
    }
    
    static const void *cb_ext_data(const char *URI) {
        return Module::ext_data(URI);
    }
};

extern const LV2_Descriptor *lv2_small_descriptor(uint32_t index);

};

#endif
#endif
