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

#include <config.h>

#if USE_JACK

#include "utils.h"
#include "vumeter.h"
#include <pthread.h>
#include <jack/jack.h>

namespace calf_plugins {

class jack_host;
    
class jack_client {
protected:
    std::vector<jack_host *> plugins;
    calf_utils::ptmutex mutex;
public:
    jack_client_t *client;
    int input_nr, output_nr, midi_nr;
    std::string name, input_name, output_name, midi_name;
    int sample_rate;

    jack_client();
    void add(jack_host *plugin);
    void del(jack_host *plugin);
    void open(const char *client_name);
    std::string get_name();
    void activate();
    void deactivate();
    void delete_plugins();
    void connect(const std::string &p1, const std::string &p2);
    void close();
    void apply_plugin_order(const std::vector<int> &indices);
    void calculate_plugin_order(std::vector<int> &indices);
    const char **get_ports(const char *name_re, const char *type_re, unsigned long flags);
    
    static int do_jack_process(jack_nframes_t nframes, void *p);
    static int do_jack_bufsize(jack_nframes_t numsamples, void *p);
};
    
class jack_host: public plugin_ctl_iface {
public:
    struct port {
        jack_port_t *handle;
        float *data;
        std::string name, nice_name;
        dsp::vumeter meter;
        port() : handle(NULL), data(NULL) {}
        ~port() { }
    };
public:
    float **ins, **outs, **params;
    std::vector<port> inputs, outputs;
    float *param_values;
    float midi_meter;
    audio_module_iface *module;
    
public:
    typedef int (*process_func)(jack_nframes_t nframes, void *p);
    jack_client *client;
    bool changed;
    port midi_port;
    std::string name;
    std::string instance_name;
    int in_count, out_count, param_count;
    const plugin_metadata_iface *metadata;
    
public:
    jack_host(audio_module_iface *_module, const std::string &_name, const std::string &_instance_name, calf_plugins::progress_report_iface *_priface);
    void create(jack_client *_client);    
    void create_ports();
    void init_module();
    void destroy();
    ~jack_host();
    
    /// Handle JACK MIDI port data
    void handle_event(uint8_t *buffer, uint32_t size);
    /// Process audio and update meters
    void process_part(unsigned int time, unsigned int len);
    /// Get meter value for the Nth port
    virtual float get_level(unsigned int port);
    /// Process audio/MIDI buffers
    int process(jack_nframes_t nframes);
    /// Retrieve and cache output port buffers
    void cache_ports();
    /// Retrieve the full list of input ports, audio+MIDI (the pointers are temporary, may point to nowhere after any changes etc.)
    void get_all_input_ports(std::vector<port *> &ports);
    /// Retrieve the full list of output ports (the pointers are temporary, may point to nowhere after any changes etc.)
    void get_all_output_ports(std::vector<port *> &ports);
    
public:
    // Port access
    port *get_inputs() { return &inputs[0]; }
    port *get_outputs() { return &outputs[0]; }
    float *get_params() { return param_values; }
    port *get_midi_port() { return get_metadata_iface()->get_midi() ? &midi_port : NULL; }
public:
    // Implementations of methods in plugin_ctl_iface 
    bool activate_preset(int bank, int program) { return false; }
    virtual float get_param_value(int param_no) {
        return param_values[param_no];
    }
    virtual void set_param_value(int param_no, float value) {
        param_values[param_no] = value;
        changed = true;
    }
    virtual void execute(int cmd_no) { module->execute(cmd_no); }
    virtual char *configure(const char *key, const char *value) { return module->configure(key, value); }
    virtual void send_configures(send_configure_iface *sci) { module->send_configures(sci); }
    virtual int send_status_updates(send_updates_iface *sui, int last_serial) { return module->send_status_updates(sui, last_serial); }
    virtual const plugin_metadata_iface *get_metadata_iface() const { return module->get_metadata_iface(); }
    virtual const line_graph_iface *get_line_graph_iface() const { return module->get_line_graph_iface(); }
    virtual const phase_graph_iface *get_phase_graph_iface() const { return module->get_phase_graph_iface(); }
};

extern jack_host *create_jack_host(const char *name, const std::string &instance_name, calf_plugins::progress_report_iface *priface);

};

#endif

#endif
