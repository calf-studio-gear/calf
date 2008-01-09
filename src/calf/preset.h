/* Calf DSP Library
 * Preset management
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
#ifndef __CALF_PRESET_H
#define __CALF_PRESET_H

#include <vector>
#include <string>
#include <map>
#include <sstream>
#include <ostream>

namespace synth {

class plugin_ctl_iface;
    
struct plugin_preset
{
    int bank, program;
    std::string name;
    std::string plugin;
    std::vector<std::string> param_names;
    std::vector<float> values;
    std::string blob;

    plugin_preset() : bank(0), program(0) {}
    std::string to_xml();    
    void activate(plugin_ctl_iface *plugin);
    void get_from(plugin_ctl_iface *plugin);
};

struct preset_exception
{
    std::string message, param, fulltext;
    int error;
    preset_exception(const std::string &_message, const std::string &_param, int _error)
    : message(_message), param(_param), error(_error)
    {
    }
    const char *what() {
        if (error)
            fulltext = message + " " + param + " (" + strerror(error) + ")";
        else
            fulltext = message + " " + param;
        return fulltext.c_str();
    }
    ~preset_exception()
    {
    }
};

typedef std::vector<plugin_preset> preset_vector;

struct preset_list
{
    enum parser_state
    {
        START,
        LIST,
        PRESET,
        VALUE,
    } state;

    preset_vector presets;
    plugin_preset parser_preset;
    std::map<std::string, int> last_preset_ids;

    static std::string get_preset_filename();
    bool load_defaults();
    void parse(const std::string &data);
    void load(const char *filename);
    void save(const char *filename);
    void add(const plugin_preset &sp);
    void get_for_plugin(preset_vector &vec, const char *plugin);
    
protected:
    static void xml_start_element_handler(void *user_data, const char *name, const char *attrs[]);
    static void xml_end_element_handler(void *user_data, const char *name);
};

extern preset_list global_presets;

};

#endif
