/* Calf DSP Library Utility Application - calfjackhost
 * API wrapper for JACK Audio Connection Kit
 *
 * Copyright (C) 2007 Krzysztof Foltman
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */
#ifndef __CALF_JACKHOST_H
#define __CALF_JACKHOST_H

#if USE_JACK

#include <jack/jack.h>
#include <jack/midiport.h>
#include "gui.h"
#include "utils.h"
#include "vumeter.h"
#include <pthread.h>

namespace calf_plugins {

class jack_host_base;
    
class jack_client {
protected:
    std::vector<jack_host_base *> plugins;
    calf_utils::ptmutex mutex;
public:
    jack_client_t *client;
    int input_nr, output_nr, midi_nr;
    std::string name, input_name, output_name, midi_name;
    int sample_rate;

    jack_client()
    {
        input_nr = output_nr = midi_nr = 1;
        input_name = "input_%d";
        output_name = "output_%d";
        midi_name = "midi_%d";
        sample_rate = 0;
        client = NULL;
    }
    
    void add(jack_host_base *plugin)
    {
        calf_utils::ptlock lock(mutex);
        plugins.push_back(plugin);
    }
    
    void del(int item)
    {
        calf_utils::ptlock lock(mutex);
        plugins.erase(plugins.begin()+item);
    }
    
    void open(const char *client_name)
    {
        jack_status_t status;
        client = jack_client_open(client_name, JackNullOption, &status);
        if (!client)
            throw calf_utils::text_exception("Could not initialize JACK subsystem");
        sample_rate = jack_get_sample_rate(client);
        jack_set_process_callback(client, do_jack_process, this);
        jack_set_buffer_size_callback(client, do_jack_bufsize, this);
        name = get_name();
    }
    
    std::string get_name()
    {
        return std::string(jack_get_client_name(client));
    }
    
    void activate()
    {
        jack_activate(client);        
    }

    void deactivate()
    {
        jack_deactivate(client);        
    }
    
    void delete_plugins();
    
    void connect(const std::string &p1, const std::string &p2)
    {
        if (jack_connect(client, p1.c_str(), p2.c_str()) != 0)
            throw calf_utils::text_exception("Could not connect JACK ports "+p1+" and "+p2);
    }
    
    void close()
    {
        jack_client_close(client);
    }
    
    const char **get_ports(const char *name_re, const char *type_re, unsigned long flags)
    {
        return jack_get_ports(client, name_re, type_re, flags);
    }
    
    static int do_jack_process(jack_nframes_t nframes, void *p);
    static int do_jack_bufsize(jack_nframes_t numsamples, void *p);
};
    
class jack_host_base: public plugin_ctl_iface {
public:
    typedef int (*process_func)(jack_nframes_t nframes, void *p);
    struct port {
        jack_port_t *handle;
        float *data;
        std::string name;
        port() : handle(NULL), data(NULL) {}
        ~port() { }
    };
    jack_client *client;
    bool changed;
    port midi_port;
    std::string name;
    std::string instance_name;
    int in_count, out_count;
    const plugin_metadata_iface *metadata;
    virtual port *get_inputs()=0;
    virtual port *get_outputs()=0;
    virtual port *get_midi_port() { return get_metadata_iface()->get_midi() ? &midi_port : NULL; }
    virtual float *get_params()=0;
    virtual void init_module()=0;
    virtual void cache_ports()=0;
    virtual int process(jack_nframes_t nframes)=0;
    
    jack_host_base(const std::string &_name, const std::string &_instance_name) {
        name = _name;
        instance_name = _instance_name;
        
        client = NULL;
        changed = true;
    }
    
    void set_params(const float *params) {
        memcpy(get_params(), params, get_metadata_iface()->get_param_count() * sizeof(float));
        changed = true;
    }

    void set_params() {
        changed = true;
    }
    
    void open(jack_client *_client);
    
    virtual void create_ports();
    
    void close();
    
    virtual ~jack_host_base() {
    }
};

template<class Module>
class jack_host: public jack_host_base {
public:
    float **ins, **outs, **params;
    std::vector<port> inputs, outputs;
    std::vector<vumeter> input_vus, output_vus;
    float *param_values;
    float midi_meter;
    audio_module_iface *iface;
    
    jack_host(audio_module_iface *_iface, const std::string &_name, const std::string &_instance_name, calf_plugins::progress_report_iface *_priface)
    : jack_host_base(_name, _instance_name), iface(_iface)
    {
        iface->get_port_arrays(ins, outs, params);
        metadata = iface->get_metadata_iface();
        in_count = metadata->get_input_count();
        out_count = metadata->get_output_count();
        inputs.resize(in_count);
        outputs.resize(out_count);
        input_vus.resize(in_count);
        output_vus.resize(out_count);
        param_values = new float[metadata->get_param_count()];
        for (int i = 0; i < metadata->get_param_count(); i++) {
            params[i] = &param_values[i];
        }
        clear_preset();
        midi_meter = 0;
        iface->set_progress_report_iface(_priface);
        iface->post_instantiate();
    }
    
    ~jack_host()
    {
        delete []param_values;
        if (client)
            close();
    }
    
    virtual void init_module() {
        iface->set_sample_rate(client->sample_rate);
        iface->activate();
        iface->params_changed();
    }

    virtual const parameter_properties* get_param_props(int param_no) { return metadata->get_param_props(param_no); }
    
    void handle_event(uint8_t *buffer, uint32_t size)
    {
        int value;
        switch(buffer[0] >> 4)
        {
        case 8:
            iface->note_off(buffer[1], buffer[2]);
            break;
        case 9:
            if (!buffer[2])
                iface->note_off(buffer[1], 0);
            else
                iface->note_on(buffer[1], buffer[2]);
            break;
        case 11:
            iface->control_change(buffer[1], buffer[2]);
            break;
        case 12:
            iface->program_change(buffer[1]);
            break;
        case 13:
            iface->channel_pressure(buffer[1]);
            break;
        case 14:
            value = buffer[1] + 128 * buffer[2] - 8192;
            iface->pitch_bend(value);
            break;
        }
    }
    void process_part(unsigned int time, unsigned int len)
    {
        if (!len)
            return;
        for (int i = 0; i < in_count; i++)
            input_vus[i].update(ins[i] + time, len);
        unsigned int mask = iface->process(time, len, -1, -1);
        for (int i = 0; i < out_count; i++)
        {
            if (!(mask & (1 << i))) {
                dsp::zero(outs[i] + time, len);
                output_vus[i].update_zeros(len);
            } else
                output_vus[i].update(outs[i] + time, len);
        }
        // decay linearly for 0.1s
        float new_meter = midi_meter - len / (0.1 * client->sample_rate);
        if (new_meter < 0)
            new_meter = 0;
        midi_meter = new_meter;
    }
    virtual float get_level(unsigned int port) { 
        if (port < (unsigned)in_count)
            return input_vus[port].level;
        port -= in_count;
        if (port < (unsigned)out_count)
            return output_vus[port].level;
        port -= out_count;
        if (port == 0 && metadata->get_midi())
            return midi_meter;
        return 0.f;
    }
    int process(jack_nframes_t nframes)
    {
        for (int i=0; i<in_count; i++) {
            ins[i] = inputs[i].data = (float *)jack_port_get_buffer(inputs[i].handle, nframes);
        }
        if (metadata->get_midi())
            midi_port.data = (float *)jack_port_get_buffer(midi_port.handle, nframes);
        if (changed) {
            iface->params_changed();
            changed = false;
        }

        unsigned int time = 0;
        if (metadata->get_midi())
        {
            jack_midi_event_t event;
#ifdef OLD_JACK
            int count = jack_midi_get_event_count(midi_port.data, nframes);
#else
            int count = jack_midi_get_event_count(midi_port.data);
#endif
            for (int i = 0; i < count; i++)
            {
#ifdef OLD_JACK
                jack_midi_event_get(&event, midi_port.data, i, nframes);
#else
                jack_midi_event_get(&event, midi_port.data, i);
#endif
                unsigned int len = event.time - time;
                process_part(time, len);
                
                midi_meter = 1.f;
                handle_event(event.buffer, event.size);
                
                time = event.time;
            }
        }
        process_part(time, nframes - time);
        iface->params_reset();
        return 0;
    }
    
    void cache_ports()
    {
        for (int i=0; i<out_count; i++) {
            outs[i] = outputs[i].data = (float *)jack_port_get_buffer(outputs[i].handle, 0);
        }
    }
    
    virtual port *get_inputs() { return &inputs[0]; }
    virtual port *get_outputs() { return &outputs[0]; }
    virtual float *get_params() { return param_values; }
    virtual bool activate_preset(int bank, int program) { return false; }
    virtual float get_param_value(int param_no) {
        return param_values[param_no];
    }
    virtual void set_param_value(int param_no, float value) {
        param_values[param_no] = value;
        changed = true;
    }
    virtual void execute(int cmd_no) {
        iface->execute(cmd_no);
    }
    virtual char *configure(const char *key, const char *value) { 
        return iface->configure(key, value);
    }
    virtual void send_configures(send_configure_iface *sci) {
        iface->send_configures(sci);
    }
    virtual int send_status_updates(send_updates_iface *sui, int last_serial) { 
        return iface->send_status_updates(sui, last_serial);
    }
    virtual const plugin_metadata_iface *get_metadata_iface() const
    {
        return iface->get_metadata_iface();
    }
};

extern jack_host_base *create_jack_host(const char *name, const std::string &instance_name, calf_plugins::progress_report_iface *priface);

};

#endif

#endif
