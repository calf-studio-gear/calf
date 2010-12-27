/* Calf DSP Library Utility Application - calfjackhost
 * A class wrapping a JACK client
 *
 * Copyright (C) 2007-2010 Krzysztof Foltman
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

#include <stdint.h>
#include <jack/jack.h>
#include <calf/giface.h>
#include <calf/jackhost.h>
#include <set>

using namespace std;
using namespace calf_utils;
using namespace calf_plugins;

jack_client::jack_client()
{
    input_nr = output_nr = midi_nr = 1;
    input_name = "input_%d";
    output_name = "output_%d";
    midi_name = "midi_%d";
    sample_rate = 0;
    client = NULL;
}

void jack_client::add(jack_host *plugin)
{
    calf_utils::ptlock lock(mutex);
    plugins.push_back(plugin);
}

void jack_client::del(jack_host *plugin)
{
    calf_utils::ptlock lock(mutex);
    for (unsigned int i = 0; i < plugins.size(); i++)
    {
        if (plugins[i] == plugin)
        {
            plugins.erase(plugins.begin()+i);
            return;
        }
    }
    assert(0);
}

void jack_client::open(const char *client_name)
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

std::string jack_client::get_name()
{
    return std::string(jack_get_client_name(client));
}

void jack_client::activate()
{
    jack_activate(client);        
}

void jack_client::deactivate()
{
    jack_deactivate(client);        
}

void jack_client::connect(const std::string &p1, const std::string &p2)
{
    if (jack_connect(client, p1.c_str(), p2.c_str()) != 0)
        throw calf_utils::text_exception("Could not connect JACK ports "+p1+" and "+p2);
}

void jack_client::close()
{
    jack_client_close(client);
}

const char **jack_client::get_ports(const char *name_re, const char *type_re, unsigned long flags)
{
    return jack_get_ports(client, name_re, type_re, flags);
}

int jack_client::do_jack_process(jack_nframes_t nframes, void *p)
{
    jack_client *self = (jack_client *)p;
    pttrylock lock(self->mutex);
    if (lock.is_locked())
    {
        for(unsigned int i = 0; i < self->plugins.size(); i++)
            self->plugins[i]->process(nframes);
    }
    return 0;
}

int jack_client::do_jack_bufsize(jack_nframes_t numsamples, void *p)
{
    jack_client *self = (jack_client *)p;
    ptlock lock(self->mutex);
    for(unsigned int i = 0; i < self->plugins.size(); i++)
        self->plugins[i]->cache_ports();
    return 0;
}

void jack_client::delete_plugins()
{
    ptlock lock(mutex);
    for (unsigned int i = 0; i < plugins.size(); i++) {
        delete plugins[i];
    }
    plugins.clear();
}

void jack_client::calculate_plugin_order(std::vector<int> &indices)
{
    map<string, int> port_to_plugin;
    multimap<int, int> run_before;
    for (unsigned int i = 0; i < plugins.size(); i++)
    {
        vector<jack_host::port *> ports;
        plugins[i]->get_all_input_ports(ports);
        for (unsigned int j = 0; j < ports.size(); j++)
            port_to_plugin[ports[j]->nice_name] = i;
    }
    
    for (unsigned int i = 0; i < plugins.size(); i++)
    {
        vector<jack_host::port *> ports;
        plugins[i]->get_all_output_ports(ports);
        for (unsigned int j = 0; j < ports.size(); j++)
        {
            const char **conns = jack_port_get_connections(ports[j]->handle);
            if (!conns)
                continue;
            for (const char **k = conns; *k; k++)
            {
                int cnlen = name.length();
                if (0 != strncmp(*k, name.c_str(), cnlen))
                    continue;
                if ((*k)[cnlen] != ':')
                    continue;
                map<string, int>::const_iterator p = port_to_plugin.find((*k) + cnlen + 1);
                if (p != port_to_plugin.end())
                {
                    run_before.insert(make_pair<int, int>(p->second, i));
                }
            }
            jack_free(conns);
        }
    }
    
    struct deptracker
    {
        vector<int> &indices;
        const multimap<int, int> &run_before;
        set<int> already_added;
        int count;
        
        deptracker(vector<int> &i, const multimap<int, int> &rb, int c)
        : indices(i)
        , run_before(rb)
        , count(c)
        {
        }
        void add_with_dependent(int item)
        {
            if (already_added.count(item))
                return;
            already_added.insert(item);
            for(multimap<int, int>::const_iterator i = run_before.find(item); i != run_before.end() && i->first == item; i++)
                add_with_dependent(i->second);
            indices.push_back(item);
        }
        void run()
        {
            for (int i = 0; i < count; i++)
                add_with_dependent(i);                    
        }
    };
    indices.clear();
    deptracker(indices, run_before, plugins.size()).run();
}

void jack_client::apply_plugin_order(const std::vector<int> &indices)
{
    std::vector<jack_host *> plugins_new;
    assert(indices.size() == plugins.size());
    for (unsigned int i = 0; i < indices.size(); i++)
        plugins_new.push_back(plugins[indices[i]]);
    ptlock lock(mutex);
    plugins.swap(plugins_new);
    
    string s;
    for (unsigned int i = 0; i < plugins.size(); i++)    
    {
        if (i)
            s += " -> ";
        s += plugins[i]->instance_name;
    }
    printf("Order: %s\n", s.c_str());
}

