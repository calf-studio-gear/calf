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
#include <sstream>
#include <ostream>
#include "giface.h"

namespace synth {

struct plugin_preset
{
    int bank, program;
    std::string name;
    std::string plugin;
    std::vector<std::string> param_names;
    std::vector<float> values;
    std::string blob;
    std::string to_xml()
    {
        std::stringstream ss;
        ss << "<preset bank=\"" << bank << "\" program=\"" << program << "\" plugin=\"" << plugin << "\" name=\"" << xml_escape(name) << "\">\n";
        for (unsigned int i = 0; i < values.size(); i++) {
            if (i < param_names.size())
                ss << "  <param name=\"" << param_names[i] << "\" value=\"" << values[i] << "\" />\n";
            else
                ss << "  <param value=\"" << values[i] << "\" />\n";
        }
        // XXXKF I'm not writing blob here, because I don't use blobs yet anyway
        ss << "</preset>\n";
        return ss.str();
    }
};

struct preset_exception
{
    std::string message, param, fulltext;
    int error;
    preset_exception(const char *_message, const char *_param, int _error)
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

extern std::string get_preset_filename();
extern void load_presets(const char *filename);
extern void save_presets(const char *filename);
extern void add_preset(const plugin_preset &sp);

// this global variable is a temporary measure
extern std::vector<plugin_preset> presets;

};

#endif
