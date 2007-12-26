/* Calf DSP Library Utility Application - calfjackhost
 * API wrapper for JACK Audio Connection Kit
 *
 * Note: while this header file is licensed under LGPL license,
 * the application as a whole is GPLed because of partial dependency
 * on phat graphics library.
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

#include "gui.h"

namespace synth {

class jack_host_base;
    
class jack_client {
public:
    jack_client_t *client;
    int input_nr, output_nr, midi_nr;
    std::string input_name, output_name, midi_name;
    std::vector<jack_host_base *> plugins;
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
        plugins.push_back(plugin);
    }
    
    void open(const char *client_name)
    {
        jack_status_t status;
        client = jack_client_open(client_name, JackNullOption, &status);
        if (!client)
            throw audio_exception("Could not initialize Jack subsystem");
        sample_rate = jack_get_sample_rate(client);
        jack_set_process_callback(client, do_jack_process, this);
        jack_set_buffer_size_callback(client, do_jack_bufsize, this);
    }
    
    void activate()
    {
        jack_activate(client);        
    }

    void deactivate()
    {
        jack_deactivate(client);        
    }
    
    void connect(const std::string &p1, const std::string &p2)
    {
        if (jack_connect(client, p1.c_str(), p2.c_str()) != 0)
            throw audio_exception("Could not connect ports "+p1+" and "+p2);
    }
    
    void close()
    {
        jack_client_close(client);
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
    virtual int get_input_count()=0;
    virtual int get_output_count()=0;
    virtual port *get_inputs()=0;
    virtual port *get_outputs()=0;
    virtual port *get_midi_port() { return get_midi() ? &midi_port : NULL; }
    virtual float *get_params()=0;
    virtual void init_module()=0;
    virtual void cache_ports()=0;
    virtual bool get_midi()=0;
    virtual int process(jack_nframes_t nframes)=0;
    
    jack_host_base() {
        client = NULL;
        changed = true;
    }
    
    void set_params(const float *params) {
        memcpy(get_params(), params, get_param_count() * sizeof(float));
        changed = true;
    }

    void set_params() {
        changed = true;
    }
    
    void open(jack_client *_client)
    {
        client = _client; //jack_client_open(client_name, JackNullOption, &status);
        
        create_ports();
        
        cache_ports();
        
        init_module();
        changed = false;
    }
    
    virtual void create_ports() {
        char buf[32];
        port *inputs = get_inputs();
        port *outputs = get_outputs();
        int in_count = get_input_count(), out_count = get_output_count();
        for (int i=0; i<in_count; i++) {
            sprintf(buf, client->input_name.c_str(), client->input_nr++);
            inputs[i].name = buf;
            inputs[i].handle = jack_port_register(client->client, buf, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput | JackPortIsTerminal, 0);
            inputs[i].data = NULL;
        }
        for (int i=0; i<out_count; i++) {
            sprintf(buf, client->output_name.c_str(), client->output_nr++);
            outputs[i].name = buf;
            outputs[i].handle = jack_port_register(client->client, buf, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput | JackPortIsTerminal, 0);
            outputs[i].data = NULL;
        }
        if (get_midi()) {
            sprintf(buf, client->midi_name.c_str(), client->midi_nr++);
            midi_port.name = buf;
            midi_port.handle = jack_port_register(client->client, buf, JACK_DEFAULT_MIDI_TYPE, JackPortIsInput | JackPortIsTerminal, 0);
        }
    }
    
    void close() {
        port *inputs = get_inputs(), *outputs = get_outputs();
        int input_count = get_input_count(), output_count = get_output_count();
        for (int i = 0; i < input_count; i++)
            inputs[i].data = NULL;
        for (int i = 0; i < output_count; i++)
            outputs[i].data = NULL;
        client = NULL;
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
        module.set_sample_rate(client->sample_rate);
        module.activate();
        module.params_changed();
    }

    virtual synth::parameter_properties* get_param_props(int param_no) { return Module::param_props + param_no; }
    
    void handle_event(uint8_t *buffer, uint32_t size)
    {
        int value;
        switch(buffer[0] >> 4)
        {
        case 8:
            module.note_off(buffer[1], buffer[2]);
            break;
        case 9:
            module.note_on(buffer[1], buffer[2]);
            break;
        case 10:
            module.program_change(buffer[1]);
            break;
        case 11:
            module.control_change(buffer[1], buffer[2]);
            break;
        case 14:
            value = buffer[1] + 128 * buffer[2] - 8192;
            module.pitch_bend(value);
            break;
        }
    }
    int process(jack_nframes_t nframes)
    {
        for (int i=0; i<Module::in_count; i++) {
            module.ins[i] = inputs[i].data = (float *)jack_port_get_buffer(inputs[i].handle, 0);
        }
        if (Module::support_midi)
            midi_port.data = (float *)jack_port_get_buffer(midi_port.handle, 0);
        if (changed) {
            module.params_changed();
            changed = false;
        }

        unsigned int time = 0;
        unsigned int mask = 0;
        if (Module::support_midi)
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
                mask = module.process(time, event.time - time, -1, -1);
                for (int i = 0; i < Module::out_count; i++) {
                    if (!(mask & (1 << i)))
                        dsp::zero(module.outs[i] + time, event.time - time);
                }
                
                handle_event(event.buffer, event.size);
                
                time = event.time;
            }
        }
        mask = module.process(time, nframes - time, -1, -1);
        for (int i = 0; i < Module::out_count; i++) {
            // zero unfilled outputs
            if (!(mask & (1 << i)))
                dsp::zero(module.outs[i] + time, nframes - time);
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
    virtual bool get_midi() { return Module::support_midi; }
    virtual float get_param_value(int param_no) {
        return params[param_no];
    }
    virtual void set_param_value(int param_no, float value) {
        params[param_no] = value;
        changed = true;
    }
    virtual const char *get_gui_xml() {
        return Module::get_gui_xml();
    }
};

extern jack_host_base *create_jack_host(const char *name);

#endif

};

#endif
