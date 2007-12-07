/* Calf DSP Library
 * API wrapper for JACK Audio Connection Kit
 *
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
#ifndef __CALF_JACKHOST_H
#define __CALF_JACKHOST_H

#if USE_JACK

namespace synth {

class jack_host_base {
public:
    typedef int (*process_func)(jack_nframes_t nframes, void *p);
    struct port {
        jack_port_t *handle;
        float *data;
        port() : handle(NULL), data(NULL) {}
        ~port() { }
    };
    jack_client_t *client;
    jack_status_t status;
    int sample_rate;
    bool changed;
    port midi_port;
    virtual int get_input_count()=0;
    virtual int get_output_count()=0;
    virtual int get_param_count()=0;
    virtual port *get_inputs()=0;
    virtual port *get_outputs()=0;
    virtual float *get_params()=0;
    virtual void init_module()=0;
    virtual void cache_ports()=0;
    virtual process_func get_process_func()=0;
    virtual const char ** get_param_names()=0;
    virtual parameter_properties* get_param_props()=0;
    virtual bool get_midi()=0;
    
    jack_host_base() {
        client = NULL;
        sample_rate = 0;
        changed = true;
    }
    
    void set_params(const float *params) {
        memcpy(get_params(), params, get_param_count() * sizeof(float));
        changed = true;
    }

    void set_params() {
        changed = true;
    }
    
    void open(const char *client_name, const char *in_prefix, const char *out_prefix, const char *midi_name)
    {
        client = jack_client_open(client_name, JackNullOption, &status);
        
        if (!client)
            throw audio_exception("Could not initialize Jack subsystem");
        
        sample_rate = jack_get_sample_rate(client);
        
        create_ports(in_prefix, out_prefix, midi_name);
        
        cache_ports();
        
        jack_set_process_callback(client, get_process_func(), this);
        jack_set_buffer_size_callback(client, do_jack_bufsize, this);

        init_module();
        changed = false;

        jack_activate(client);        
    }
    
    virtual void create_ports(const char *in_prefix, const char *out_prefix, const char *midi_name) {
        char buf[32];
        port *inputs = get_inputs();
        port *outputs = get_outputs();
        int in_count = get_input_count(), out_count = get_output_count();
        for (int i=0; i<in_count; i++) {
            sprintf(buf, "%s%d", in_prefix, i+1);
            inputs[i].handle = jack_port_register(client, buf, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput | JackPortIsTerminal, 0);
            inputs[i].data = NULL;
        }
        for (int i=0; i<out_count; i++) {
            sprintf(buf, "%s%d", out_prefix, i+1);
            outputs[i].handle = jack_port_register(client, buf, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput | JackPortIsTerminal, 0);
            outputs[i].data = NULL;
        }
        if (get_midi())
            midi_port.handle = jack_port_register(client, midi_name, JACK_DEFAULT_MIDI_TYPE, JackPortIsInput | JackPortIsTerminal, 0);
    }
    
    static int do_jack_bufsize(jack_nframes_t numsamples, void *p) {
        jack_host_base *app = (jack_host_base *)p;
        app->cache_ports();
        return 0;
    }
    
    void close() {
        jack_client_close(client);
        client = NULL;
        port *inputs = get_inputs(), *outputs = get_outputs();
        int input_count = get_input_count(), output_count = get_output_count();
        for (int i = 0; i < input_count; i++)
            inputs[i].data = NULL;
        for (int i = 0; i < output_count; i++)
            outputs[i].data = NULL;
    }
    
    virtual ~jack_host_base() {
        if (client)
            close();
    }
};

template<class Module>
class jack_host: public jack_host_base {
public:
    Module module;
    port inputs[Module::in_count], outputs[Module::out_count];
    float params[Module::param_count];
    
    jack_host()
    {
        for (int i = 0; i < Module::param_count; i++) {
            module.params[i] = &params[i];
            params[i] = Module::param_props[i].def_value;
        }
    }
    
    virtual void init_module() {
        module.set_sample_rate(sample_rate);
        module.params_changed();
    }
    
    static int do_jack_process(jack_nframes_t nframes, void *p) {
        jack_host *host = (jack_host *)p;
        Module *module = &host->module;
        for (int i=0; i<Module::in_count; i++) {
            module->ins[i] = host->inputs[i].data = (float *)jack_port_get_buffer(host->inputs[i].handle, 0);
        }
        if (Module::support_midi)
            host->midi_port.data = (float *)jack_port_get_buffer(host->midi_port.handle, 0);
        if (host->changed) {
            module->params_changed();
            host->changed = false;
        }

        unsigned int time = 0;
        unsigned int mask = 0;
        if (Module::support_midi)
        {
            jack_midi_event_t event;
            int count = jack_midi_get_event_count(host->midi_port.data);
            for (int i = 0; i < count; i++)
            {
                jack_midi_event_get(&event, host->midi_port.data, i);
                mask = module->process(time, event.time - time, -1, -1);
                for (int i = 0; i < Module::out_count; i++) {
                    if (!(mask & (1 << i)))
                        dsp::zero(module->outs[i] + time, event.time - time);
                }
                
                module->handle_event(event.buffer, event.size);
                
                time = event.time;
            }
        }
        mask = module->process(time, nframes - time, -1, -1);
        for (int i = 0; i < Module::out_count; i++) {
            // zero unfilled outputs
            if (!(mask & (1 << i)))
                dsp::zero(module->outs[i] + time, nframes - time);
        }
        return 0;
    }
    
    void cache_ports()
    {
        for (int i=0; i<Module::out_count; i++) {
            module.outs[i] = outputs[i].data = (float *)jack_port_get_buffer(outputs[i].handle, 0);
        }
    }
    
    virtual void zero_io() {
        memset(inputs, 0, sizeof(inputs));
        memset(outputs, 0, sizeof(outputs));
    }
    
    virtual port *get_inputs() { return inputs; }
    virtual port *get_outputs() { return outputs; }
    virtual float *get_params() { return params; }
    virtual int get_input_count() { return Module::in_count; }
    virtual int get_output_count() { return Module::out_count; }
    virtual int get_param_count() { return Module::param_count; }
    virtual process_func get_process_func() { return do_jack_process; }
    virtual const char ** get_param_names() { return Module::param_names; }
    virtual parameter_properties* get_param_props() { return Module::param_props; }
    virtual bool get_midi() { return Module::support_midi; }
};

#endif

};

#endif
