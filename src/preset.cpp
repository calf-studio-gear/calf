/* Calf DSP Library
 * Preset management.
 *
 * Copyright (C) 2001-2007 Krzysztof Foltman
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
 * Boston, MA  02110-1301  USA
 */

#include <calf/giface.h>
#include <calf/preset.h>
#include <expat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>

using namespace std;
using namespace calf_plugins;
using namespace calf_utils;

extern calf_plugins::preset_list &calf_plugins::get_builtin_presets()
{
    static calf_plugins::preset_list plist;
    return plist;
}

extern calf_plugins::preset_list &calf_plugins::get_user_presets()
{
    static calf_plugins::preset_list plist;
    return plist;
}

std::string plugin_preset::to_xml()
{
    std::stringstream ss;
    ss << "<preset bank=\"" << bank << "\" program=\"" << program << "\" plugin=\"" << xml_escape(plugin) << "\" name=\"" << xml_escape(name) << "\">\n";
    for (unsigned int i = 0; i < values.size(); i++) {
        if (i < param_names.size())
            ss << "  <param name=\"" << xml_escape(param_names[i]) << "\" value=\"" << values[i] << "\" />\n";
        else
            ss << "  <param value=\"" << values[i] << "\" />\n";
    }
    for (map<string, string>::iterator i = variables.begin(); i != variables.end(); i++)
    {
        ss << "  <var name=\"" << xml_escape(i->first) << "\">" << xml_escape(i->second) << "</var>\n";
    }
    ss << "</preset>\n";
    return ss.str();
}

string plugin_preset::get_safe_name()
{
    stringstream ss;
    for (size_t i = 0; i < name.length(); i++)
    {
        if (isdigit(name[i]) || isalpha(name[i]))
            ss << name[i];
    }
    return ss.str();
}

void plugin_preset::activate(plugin_ctl_iface *plugin)
{
    // First, clear everything to default values (in case some parameters or variables are missing)
    plugin->clear_preset();
    const plugin_metadata_iface *metadata = plugin->get_metadata_iface();

    map<string, int> names;    
    int count = metadata->get_param_count();
    // this is deliberately done in two separate loops - if you wonder why, just think for a while :)
    for (int i = 0; i < count; i++)
        names[metadata->get_param_props(i)->name] = i;
    for (int i = 0; i < count; i++)
        names[metadata->get_param_props(i)->short_name] = i;
    // no support for unnamed parameters... tough luck :)
    for (unsigned int i = 0; i < min(param_names.size(), values.size()); i++)
    {
        map<string, int>::iterator pos = names.find(param_names[i]);
        if (pos == names.end()) {
            // XXXKF should have a mechanism for notifying a GUI
            printf("Warning: unknown parameter %s for plugin %s\n", param_names[i].c_str(), this->plugin.c_str());
            continue;
        }
        plugin->set_param_value(pos->second, values[i]);
    }
    const char *const *vnames = metadata->get_configure_vars();
    if (vnames)
    {
        for (; *vnames; vnames++)
        {
            const char *key = *vnames;
            map<string, string>::const_iterator i = variables.find(key);
            if (i == variables.end())
                plugin->configure(key, NULL);
            else
                plugin->configure(key, i->second.c_str());
        }
    }
}

void plugin_preset::get_from(plugin_ctl_iface *plugin)
{
    const plugin_metadata_iface *metadata = plugin->get_metadata_iface();
    int count = metadata->get_param_count();
    for (int i = 0; i < count; i++) {
        param_names.push_back(metadata->get_param_props(i)->short_name);
        values.push_back(plugin->get_param_value(i));
    }
    struct store_obj: public send_configure_iface
    {
        map<string, string> *data;
        void send_configure(const char *key, const char *value)
        {
            (*data)[key] = value;
        }
    } tmp;
    variables.clear();
    tmp.data = &variables;
    plugin->send_configures(&tmp);
}
    
string calf_plugins::preset_list::get_preset_filename(bool builtin)
{
    if (builtin)
    {
        return PKGLIBDIR "/presets.xml";
    }
    else
    {
        const char *home = getenv("HOME");
        return string(home)+"/.calfpresets";
    }
}

void preset_list::plugin_snapshot::reset()
{
    type.clear();
    instance_name.clear();
    preset_offset = input_index = output_index = midi_index = 0;
    
}

void preset_list::xml_start_element_handler(void *user_data, const char *name, const char *attrs[])
{
    preset_list &self = *(preset_list *)user_data;
    bool rack_mode = self.rack_mode;
    parser_state &state = self.state;
    plugin_preset &parser_preset = self.parser_preset;
    switch(state)
    {
    case START:
        if (!rack_mode && !strcmp(name, "presets")) {
            state = LIST;
            return;
        }
        if (rack_mode && !strcmp(name, "rack")) {
            state = RACK;
            return;
        }
        break;
    case RACK:
        if (!strcmp(name, "plugin")) {
            self.parser_plugin.reset();
            self.parser_plugin.preset_offset = self.presets.size();
            for(; *attrs; attrs += 2) {
                if (!strcmp(attrs[0], "type")) self.parser_plugin.type = attrs[1];
                else
                if (!strcmp(attrs[0], "instance-name")) self.parser_plugin.instance_name = attrs[1];
                else
                if (!strcmp(attrs[0], "input-index")) self.parser_plugin.input_index = atoi(attrs[1]);
                else
                if (!strcmp(attrs[0], "output-index")) self.parser_plugin.output_index = atoi(attrs[1]);
                else
                if (!strcmp(attrs[0], "midi-index")) self.parser_plugin.midi_index = atoi(attrs[1]);
            }
            state = PLUGIN;
            return;
        }
        break;
    case PLUGIN:
    case LIST:
        if (!strcmp(name, "preset")) {
            
            parser_preset.bank = parser_preset.program = 0;
            parser_preset.name = "";
            parser_preset.plugin = "";
            parser_preset.param_names.clear();
            parser_preset.values.clear();
            parser_preset.variables.clear();
            for(; *attrs; attrs += 2) {
                if (!strcmp(attrs[0], "name")) self.parser_preset.name = attrs[1];
                else
                if (!strcmp(attrs[0], "plugin")) self.parser_preset.plugin = attrs[1];
            }
            // autonumbering of programs for DSSI
            if (!self.last_preset_ids.count(self.parser_preset.plugin))
                self.last_preset_ids[self.parser_preset.plugin] = 0;
            self.parser_preset.program = ++self.last_preset_ids[self.parser_preset.plugin];
            self.parser_preset.bank = (self.parser_preset.program >> 7);
            self.parser_preset.program &= 127;
            state = PRESET;
            return;
        }
        break;
    case PRESET:
        if (!strcmp(name, "param")) {
            std::string name;
            float value = 0.f;
            for(; *attrs; attrs += 2) {
                if (!strcmp(attrs[0], "name")) name = attrs[1];
                else
                if (!strcmp(attrs[0], "value")) {
                    istringstream str(attrs[1]);
                    str >> value;
                }
            }
            self.parser_preset.param_names.push_back(name);
            self.parser_preset.values.push_back(value);
            state = VALUE;
            return;
        }
        if (!strcmp(name, "var")) {
            self.current_key = "";
            for(; *attrs; attrs += 2) {
                if (!strcmp(attrs[0], "name")) self.current_key = attrs[1];
            }
            if (self.current_key.empty())
                throw preset_exception("No name specified for preset variable", "", 0);
            self.parser_preset.variables[self.current_key].clear();
            state = VAR;
            return;
        }
        break;
    case VALUE:
        // no nested elements allowed inside <param>
        break;
    case VAR:
        // no nested elements allowed inside <var>
        break;
    }
    // g_warning("Invalid XML element: %s", name);
    throw preset_exception("Invalid XML element: %s", name, 0);
}

void preset_list::xml_end_element_handler(void *user_data, const char *name)
{
    preset_list &self = *(preset_list *)user_data;
    bool rack_mode = self.rack_mode;
    preset_vector &presets = self.presets;
    parser_state &state = self.state;
    switch(state)
    {
    case START:
        break;
    case LIST:
        if (!strcmp(name, "presets")) {
            state = START;
            return;
        }
        break;
    case PLUGIN:
        if (!strcmp(name, "plugin")) {
            self.plugins.push_back(self.parser_plugin);
            state = RACK;
            return;
        }
        break;
    case RACK:
        if (!strcmp(name, "rack")) {
            state = START;
            return;
        }
        break;
    case PRESET:
        if (!strcmp(name, "preset")) {
            presets.push_back(self.parser_preset);
            state = rack_mode ? PLUGIN : LIST;
            return;
        }
        break;
    case VALUE:
        if (!strcmp(name, "param")) {
            state = PRESET;
            return;
        }
        break;
    case VAR:
        if (!strcmp(name, "var")) {
            state = PRESET;
            return;
        }
        break;
    }
    throw preset_exception("Invalid XML element close: %s", name, 0);
}

void preset_list::xml_character_data_handler(void *user_data, const XML_Char *data, int len)
{
    preset_list &self = *(preset_list *)user_data;
    parser_state &state = self.state;
    if (state == VAR)
    {
        self.parser_preset.variables[self.current_key] += string(data, len);
        return;
    }
}

bool preset_list::load_defaults(bool builtin)
{
    try {
        struct stat st;
        string name = preset_list::get_preset_filename(builtin);
        if (!stat(name.c_str(), &st)) {
            load(name.c_str(), false);
            if (!presets.empty())
                return true;
        }
    }
    catch(preset_exception &ex)
    {
        return false;
    }
    return false;
}

void preset_list::parse(const std::string &data, bool in_rack_mode)
{
    rack_mode = in_rack_mode;
    state = START;
    XML_Parser parser = XML_ParserCreate("UTF-8");
    XML_SetUserData(parser, this);
    XML_SetElementHandler(parser, xml_start_element_handler, xml_end_element_handler);
    XML_SetCharacterDataHandler(parser, xml_character_data_handler);
    XML_Status status = XML_Parse(parser, data.c_str(), data.length(), 1);
    if (status == XML_STATUS_ERROR) {
        string err = string("Parse error: ") + XML_ErrorString(XML_GetErrorCode(parser))+ " in ";
        XML_ParserFree(parser);
        throw preset_exception(err, "string", errno);
    }
    XML_ParserFree(parser);
}

void preset_list::load(const char *filename, bool in_rack_mode)
{
    rack_mode = in_rack_mode;
    state = START;
    XML_Parser parser = XML_ParserCreate("UTF-8");
    XML_SetUserData(parser, this);
    int fd = open(filename, O_RDONLY);
    if (fd < 0) 
        throw preset_exception("Could not load the presets from ", filename, errno);
    XML_SetElementHandler(parser, xml_start_element_handler, xml_end_element_handler);
    XML_SetCharacterDataHandler(parser, xml_character_data_handler);
    char buf[4096];
    do
    {
        int len = read(fd, buf, 4096);
        // XXXKF not an optimal error/EOF handling :)
        if (len <= 0)
            break;
        XML_Status status = XML_Parse(parser, buf, len, 0);
        if (status == XML_STATUS_ERROR)
            throw preset_exception(string("Parse error: ") + XML_ErrorString(XML_GetErrorCode(parser))+ " in ", filename, errno);
    } while(1);
    XML_Status status = XML_Parse(parser, buf, 0, 1);
    close(fd);
    if (status == XML_STATUS_ERROR)
    {
        string err = string("Parse error: ") + XML_ErrorString(XML_GetErrorCode(parser))+ " in ";
        XML_ParserFree(parser);
        throw preset_exception(err, filename, errno);
    }
    XML_ParserFree(parser);
}

void preset_list::save(const char *filename)
{
    string xml = "<presets>\n";
    for (unsigned int i = 0; i < presets.size(); i++)
    {
        xml += presets[i].to_xml();
    }
    xml += "</presets>";
    int fd = open(filename, O_CREAT | O_WRONLY | O_TRUNC, 0640);
    if (fd < 0 || ((unsigned)write(fd, xml.c_str(), xml.length()) != xml.length()))
        throw preset_exception("Could not save the presets in ", filename, errno);
    close(fd);
}

void preset_list::get_for_plugin(preset_vector &vec, const char *plugin)
{
    for (unsigned int i = 0; i < presets.size(); i++)
    {
        if (presets[i].plugin == plugin)
            vec.push_back(presets[i]);
    }
}

void preset_list::add(const plugin_preset &sp)
{
    for (unsigned int i = 0; i < presets.size(); i++)
    {
        if (presets[i].plugin == sp.plugin && presets[i].name == sp.name)
        {
            presets[i] = sp;
            return;
        }
    }
    presets.push_back(sp);
}
