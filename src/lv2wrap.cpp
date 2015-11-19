#include <config.h>
#include "calf/lv2wrap.h"

#if USE_LV2

using namespace calf_plugins;

lv2_instance::lv2_instance(audio_module_iface *_module)
{
    module = _module;
    module->get_port_arrays(ins, outs, params);
    metadata = module->get_metadata_iface();
    in_count = metadata->get_input_count();
    out_count = metadata->get_output_count();
    real_param_count = metadata->get_param_count();
    
    urid_map = NULL;
    event_in_data = NULL;
    event_out_data = NULL;
    progress_report_feature = NULL;
    options_feature = NULL;
    midi_event_type = 0xFFFFFFFF;

    srate_to_set = 44100;
    set_srate = true;
}

void lv2_instance::lv2_instantiate(const LV2_Descriptor * Descriptor, double sample_rate, const char *bundle_path, const LV2_Feature *const *features)
{
    // XXXKF some people use fractional sample rates; we respect them ;-)
    srate_to_set = (uint32_t)sample_rate;
    set_srate = true;
    while(*features)
    {
        if (!strcmp((*features)->URI, LV2_URID_MAP_URI))
        {
            urid_map = (LV2_URID_Map *)((*features)->data);
            midi_event_type = urid_map->map(
                urid_map->handle, LV2_MIDI__MidiEvent);
        }           
        else if (!strcmp((*features)->URI, LV2_PROGRESS_URI))
        {
            progress_report_feature = (LV2_Progress *)((*features)->data);
        }
        else if (!strcmp((*features)->URI, LV2_OPTIONS_URI))
        {
            options_feature = (LV2_Options_Interface *)((*features)->data);
        }
        features++;
    }
    post_instantiate();
}

void lv2_instance::post_instantiate()
{
    if (progress_report_feature)
        module->set_progress_report_iface(this);
    if (urid_map)
    {
        std::vector<std::string> varnames;
        module->get_metadata_iface()->get_configure_vars(varnames);
        for (size_t i = 0; i < varnames.size(); ++i)
        {
            std::string pred = std::string("urn:calf:") + varnames[i];
            lv2_var tmp;
            tmp.name = varnames[i];
            tmp.mapped_uri = urid_map->map(urid_map->handle, pred.c_str());
            if (!tmp.mapped_uri)
            {
                vars.clear();
                uri_to_var.clear();
                break;
            }
            vars.push_back(tmp);
            uri_to_var[tmp.mapped_uri] = i;
        }
        string_type = urid_map->map(urid_map->handle, LV2_ATOM__String);
        assert(string_type);
        sequence_type = urid_map->map(urid_map->handle, LV2_ATOM__Sequence);
        assert(sequence_type);
        property_type = urid_map->map(urid_map->handle, LV2_ATOM__Property);
        assert(property_type);
    }
    module->post_instantiate(srate_to_set);
}

void lv2_instance::impl_restore(LV2_State_Retrieve_Function retrieve, void *callback_data)
{
    if (set_srate)
        module->set_sample_rate(srate_to_set);
    
    if (vars.empty())
        return;
    assert(urid_map);
    for (size_t i = 0; i < vars.size(); ++i)
    {
        size_t         len   = 0;
        uint32_t       type  = 0;
        uint32_t       flags = 0;
        const void *ptr = (*retrieve)(callback_data, vars[i].mapped_uri, &len, &type, &flags);
        if (ptr)
        {
            if (type != string_type)
                fprintf(stderr, "Warning: type is %d, expected %d\n", (int)type, (int)string_type);
            printf("Calling configure on %s\n", vars[i].name.c_str());
            configure(vars[i].name.c_str(), std::string((const char *)ptr, len).c_str());
        }
        else
            configure(vars[i].name.c_str(), NULL);
    }
}

void lv2_instance::output_event_string(const char *str, int len)
{
    if (len == -1)
        len = strlen(str);
    memcpy(add_event_to_seq(0, string_type, len + 1), str, len + 1);
}

void lv2_instance::output_event_property(const char *key, const char *value)
{
    // XXXKF super slow
    uint32_t keyv = 0;
    for (size_t i = 0; i < vars.size(); ++i)
    {
        if (vars[i].name == key)
        {
            keyv = vars[i].mapped_uri;
        }
    }
    uint32_t len = strlen(value);
    LV2_Atom_Property_Body *p = (LV2_Atom_Property_Body *)add_event_to_seq(0, property_type, sizeof(LV2_Atom_Property_Body) + len + 1);
    p->key = keyv;
    p->context = 0;
    p->value.type = string_type;
    p->value.size = len + 1;
    memcpy(p + 1, value, len + 1);
}

void lv2_instance::run(uint32_t SampleCount, bool has_simulate_stereo_input_flag)
{
    if (set_srate) {
        module->set_sample_rate(srate_to_set);
        module->activate();
        set_srate = false;
    }
    module->params_changed();
    uint32_t offset = 0;
    if (event_out_data)
    {
        LV2_Atom *atom = &event_out_data->atom;
        event_out_capacity = atom->size;
        atom->type = sequence_type;
        event_out_data->body.unit = 0;
        lv2_atom_sequence_clear(event_out_data);
    }
    if (event_in_data)
    {
        process_events(offset);
    }
    bool simulate_stereo_input = (in_count > 1) && has_simulate_stereo_input_flag && !ins[1];
    if (simulate_stereo_input)
        ins[1] = ins[0];
    module->process_slice(offset, SampleCount);
    if (simulate_stereo_input)
        ins[1] = NULL;
}

void lv2_instance::process_event_string(const char *str)
{
    if (str[0] == '?' && str[1] == '\0')
    {
        struct sci: public send_configure_iface
        {
            lv2_instance *inst;
            void send_configure(const char *key, const char *value)
            {
                inst->output_event_property(key, value);
            }
        } tmp;
        tmp.inst = this;
        send_configures(&tmp);
    }
}

void lv2_instance::process_event_property(const LV2_Atom_Property *prop)
{
    if (prop->body.value.type == string_type)
    {
        std::map<uint32_t, int>::iterator i = uri_to_var.find(prop->body.key);
        if (i == uri_to_var.end())
            printf("Set property %d -> %s\n", prop->body.key, (const char *)((&prop->body)+1));
        else
            printf("Set property %s -> %s\n", vars[i->second].name.c_str(), (const char *)((&prop->body)+1));

        if (i != uri_to_var.end())
            configure(vars[i->second].name.c_str(), (const char *)((&prop->body)+1));
    }
    else
        printf("Set property %d -> unknown type %d\n", prop->body.key, prop->body.value.type);
}

void lv2_instance::process_events(uint32_t &offset)
{
    LV2_ATOM_SEQUENCE_FOREACH(event_in_data, ev) {
        const uint8_t* const data = (const uint8_t*)(ev + 1);
        uint32_t ts = ev->time.frames;
        // printf("Event: timestamp %d type %x vs %x vs %x\n", ts, ev->body.type, midi_event_type, property_type);
        if (ts > offset)
        {
            module->process_slice(offset, ts);
            offset = ts;
        }
        if (ev->body.type == string_type)
        {
            process_event_string((const char *)LV2_ATOM_CONTENTS(LV2_Atom_String, &ev->body));
        }
        if (ev->body.type == property_type)
        {
            process_event_property((LV2_Atom_Property *)(&ev->body));
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

LV2_State_Status lv2_instance::state_save(
    LV2_State_Store_Function store, LV2_State_Handle handle,
    uint32_t flags, const LV2_Feature *const * features)
{
    // A host that supports State MUST support URID-Map as well.
    assert(urid_map);
    store_lv2_state s;
    s.store = store;
    s.callback_data = handle;
    s.inst = this;
    s.string_data_type = urid_map->map(urid_map->handle, LV2_ATOM__String);

    send_configures(&s);
    return LV2_STATE_SUCCESS;
}

void store_lv2_state::send_configure(const char *key, const char *value)
{
    std::string pred = std::string("urn:calf:") + key;
    (*store)(callback_data,
             inst->urid_map->map(inst->urid_map->handle, pred.c_str()),
             value,
             strlen(value) + 1,
             string_data_type,
             LV2_STATE_IS_POD|LV2_STATE_IS_PORTABLE);
}

#endif
