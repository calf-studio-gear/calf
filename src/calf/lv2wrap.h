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
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 * Boston, MA 02111-1307, USA.
 */
#ifndef CALF_LV2WRAP_H
#define CALF_LV2WRAP_H

#if USE_LV2

#include <string>
#include <vector>
#include <lv2.h>
#include <calf/giface.h>
#include <calf/lv2_atom.h>
#include <calf/lv2_atom_util.h>
#include <calf/lv2_midi.h>
#include <calf/lv2_state.h>
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
    LV2_Atom_Sequence *event_data;
    LV2_URID_Map *urid_map;
    uint32_t midi_event_type;
    LV2_Progress *progress_report_feature;
    float **ins, **outs, **params;
    int out_count;
    int real_param_count;
    lv2_instance(audio_module_iface *_module)
    {
        module = _module;
        module->get_port_arrays(ins, outs, params);
        metadata = module->get_metadata_iface();
        out_count = metadata->get_output_count();
        real_param_count = metadata->get_param_count();
        
        urid_map = NULL;
        event_data = NULL;
        progress_report_feature = NULL;
        midi_event_type = 0xFFFFFFFF;

        srate_to_set = 44100;
        set_srate = true;
    }
    /// This, and not Module::post_instantiate, is actually called by lv2_wrapper class
    void post_instantiate()
    {
        if (progress_report_feature)
            module->set_progress_report_iface(this);
        module->post_instantiate(srate_to_set);
    }
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
    void impl_restore(LV2_State_Retrieve_Function retrieve, void *callback_data)
    {
        std::vector<std::string> vars;
        module->get_metadata_iface()->get_configure_vars(vars);
        if (vars.empty())
            return;
        assert(urid_map);
        uint32_t string_type = urid_map->map(urid_map->handle, LV2_ATOM__String);
        assert(string_type);
        for (size_t i = 0; i < vars.size(); ++i)
        {
            std::string    pred  = std::string("urn:calf:") + vars[i];
            const uint32_t key   = urid_map->map(urid_map->handle, pred.c_str());
            size_t         len   = 0;
            uint32_t       type  = 0;
            uint32_t       flags = 0;
            const void *ptr = (*retrieve)(callback_data, key, &len, &type, &flags);
            if (ptr)
            {
                if (type != string_type)
                    fprintf(stderr, "Warning: type is %d, expected %d\n", (int)type, (int)string_type);
                printf("Calling configure on %s\n", vars[i].c_str());
                configure(vars[i].c_str(), std::string((const char *)ptr, len).c_str());
            }
            else
                configure(vars[i].c_str(), NULL);
        }
    }
    char *configure(const char *key, const char *value) { 
        // disambiguation - the plugin_ctl_iface version is just a stub, so don't use it
        return module->configure(key, value);
    }
    
    void process_events(uint32_t &offset) {
        LV2_ATOM_SEQUENCE_FOREACH(event_data, ev) {
            const uint8_t* const data = (const uint8_t*)(ev + 1);
            uint32_t ts = ev->time.frames;
            // printf("Event: timestamp %d type %d vs %d\n", ts, ev->body.type, midi_event_type);
            if (ts > offset)
            {
                module->process_slice(offset, ts);
                offset = ts;
            }
            if (ev->body.type == midi_event_type)
            {
                // printf("Midi message %x %x %x %d\n", data[0], data[1], data[2], ev->body.size);
                int channel = data[0] & 0x0f;
                switch (lv2_midi_message_type(data))
                {
                case LV2_MIDI_MSG_INVALID: break;
                case LV2_MIDI_MSG_NOTE_OFF : module->note_off(channel, data[1], data[2]); break;
                case LV2_MIDI_MSG_NOTE_ON: module->note_on(channel, data[1], data[2]); break;
                case LV2_MIDI_MSG_CONTROLLER: module->control_change(channel, data[1], data[2]); break;
                case LV2_MIDI_MSG_PGM_CHANGE: module->program_change(channel, data[1]); break;
                case LV2_MIDI_MSG_CHANNEL_PRESSURE: module->channel_pressure(channel, data[1]); break;
                case LV2_MIDI_MSG_BENDER: module->pitch_bend(channel, data[1] + 128 * data[2] - 8192); break;
                case LV2_MIDI_MSG_NOTE_PRESSURE: break;
                case LV2_MIDI_MSG_SYSTEM_EXCLUSIVE: break;
                case LV2_MIDI_MSG_MTC_QUARTER: break;
                case LV2_MIDI_MSG_SONG_POS: break;
                case LV2_MIDI_MSG_SONG_SELECT: break;
                case LV2_MIDI_MSG_TUNE_REQUEST: break;
                case LV2_MIDI_MSG_CLOCK: break;
                case LV2_MIDI_MSG_START: break;
                case LV2_MIDI_MSG_CONTINUE: break;
                case LV2_MIDI_MSG_STOP: break;
                case LV2_MIDI_MSG_ACTIVE_SENSE: break;
                case LV2_MIDI_MSG_RESET: break;
                }
            }            
        }
    }

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
        if (port < ins)
            mod->ins[port] = (float *)DataLocation;
        else if (port < ins + outs)
            mod->outs[port - ins] = (float *)DataLocation;
        else if (port < ins + outs + params) {
            int i = port - ins - outs;
            mod->params[i] = (float *)DataLocation;
        }
        else if (md->get_midi() && port == ins + outs + params) {
            mod->event_data = (LV2_Atom_Sequence *)DataLocation;
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
        // XXXKF some people use fractional sample rates; we respect them ;-)
        mod->srate_to_set = (uint32_t)sample_rate;
        mod->set_srate = true;
        while(*features)
        {
            if (!strcmp((*features)->URI, LV2_URID_MAP_URI))
            {
                mod->urid_map = (LV2_URID_Map *)((*features)->data);
                mod->midi_event_type = mod->urid_map->map(
                    mod->urid_map->handle, LV2_MIDI__MidiEvent);
            }           
            else if (!strcmp((*features)->URI, LV2_PROGRESS_URI))
            {
                mod->progress_report_feature = (LV2_Progress *)((*features)->data);
            }
            features++;
        }
        mod->post_instantiate();
        return mod;
    }
    static plugin_ctl_iface *cb_get_pci(LV2_Handle Instance)
    {
        return static_cast<plugin_ctl_iface *>(Instance);
    }

    static void cb_run(LV2_Handle Instance, uint32_t SampleCount)
    {
        instance *const inst = (instance *)Instance;
        audio_module_iface *mod = inst->module;
        if (inst->set_srate) {
            mod->set_sample_rate(inst->srate_to_set);
            mod->activate();
            inst->set_srate = false;
        }
        mod->params_changed();
        uint32_t offset = 0;
        if (inst->event_data)
        {
            inst->process_events(offset);
        }
        bool simulate_stereo_input = (Module::in_count > 1) && Module::simulate_stereo_input && !inst->ins[1];
        if (simulate_stereo_input)
            inst->ins[1] = inst->ins[0];
        inst->module->process_slice(offset, SampleCount);
        if (simulate_stereo_input)
            inst->ins[1] = NULL;
    }
    static void cb_cleanup(LV2_Handle Instance)
    {
        instance *const mod = (instance *)Instance;
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
        struct store_state: public send_configure_iface
        {
            LV2_State_Store_Function store;
            void *callback_data;
            instance *inst;
            uint32_t string_data_type;
            
            virtual void send_configure(const char *key, const char *value)
            {
                std::string pred = std::string("urn:calf:") + key;
                (*store)(callback_data,
                         inst->urid_map->map(inst->urid_map->handle, pred.c_str()),
                         value,
                         strlen(value) + 1,
                         string_data_type,
                         LV2_STATE_IS_POD|LV2_STATE_IS_PORTABLE);
            }
        };
        // A host that supports State MUST support URID-Map as well.
        assert(inst->urid_map);
        store_state s;
        s.store = store;
        s.callback_data = handle;
        s.inst = inst;
        s.string_data_type = inst->urid_map->map(inst->urid_map->handle, LV2_ATOM__String);

        inst->send_configures(&s);
        return LV2_STATE_SUCCESS;
    }
    static LV2_State_Status cb_state_restore(
        LV2_Handle Instance, LV2_State_Retrieve_Function retrieve, LV2_State_Handle callback_data,
        uint32_t flags, const LV2_Feature *const * features)
    {
        instance *const inst = (instance *)Instance;
        if (inst->set_srate)
            inst->module->set_sample_rate(inst->srate_to_set);
        
        inst->impl_restore(retrieve, callback_data);
        return LV2_STATE_SUCCESS;
    }
    
    static lv2_wrapper &get() { 
        static lv2_wrapper *instance = new lv2_wrapper;
        return *instance;
    }
};

};

#endif
#endif
