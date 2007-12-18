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
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>

#include <expat.h>
#include <calf/preset.h>

using namespace synth;
using namespace std;

vector<plugin_preset> synth::presets;

string synth::get_preset_filename()
{
    const char *home = getenv("HOME");
    return string(home)+"/.calfpresets";
}

enum PresetParserState
{
    START,
    LIST,
    PRESET,
    VALUE,
};

static PresetParserState xml_parser_state;
static plugin_preset parser_preset;

static void xml_start_element_handler(void *user_data, const char *name, const char *attrs[])
{
    switch(xml_parser_state)
    {
    case START:
        if (!strcmp(name, "presets")) {
            xml_parser_state = LIST;
            return;
        }
        break;
    case LIST:
        if (!strcmp(name, "preset")) {
            parser_preset.bank = parser_preset.program = 0;
            parser_preset.name = "";
            parser_preset.plugin = "";
            parser_preset.blob = "";
            parser_preset.param_names.clear();
            parser_preset.values.clear();
            for(; *attrs; attrs += 2) {
                if (!strcmp(attrs[0], "bank")) parser_preset.bank = atoi(attrs[1]);
                else
                if (!strcmp(attrs[0], "program")) parser_preset.program = atoi(attrs[1]);
                else
                if (!strcmp(attrs[0], "name")) parser_preset.name = attrs[1];
                else
                if (!strcmp(attrs[0], "plugin")) parser_preset.plugin = attrs[1];
            }
            xml_parser_state = PRESET;
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
            parser_preset.param_names.push_back(name);
            parser_preset.values.push_back(value);
            xml_parser_state = VALUE;
            return;
        }
        break;
    case VALUE:
        break;
    }
    // g_warning("Invalid XML element: %s", name);
    throw preset_exception("Invalid XML element: %s", name, 0);
}

static void xml_end_element_handler(void *user_data, const char *name)
{
    switch(xml_parser_state)
    {
    case START:
        break;
    case LIST:
        if (!strcmp(name, "presets")) {
            xml_parser_state = START;
            return;
        }
        break;
    case PRESET:
        if (!strcmp(name, "preset")) {
            presets.push_back(parser_preset);
            xml_parser_state = LIST;
            return;
        }
        break;
    case VALUE:
        if (!strcmp(name, "param")) {
            xml_parser_state = PRESET;
            return;
        }
        break;
    }
    throw preset_exception("Invalid XML element close: %s", name, 0);
}

void synth::load_presets(const char *filename)
{
    xml_parser_state = START;
    XML_Parser parser = XML_ParserCreate("UTF-8");
    int fd = open(filename, O_RDONLY);
    if (fd < 0) 
        throw preset_exception("Could not load the presets from ", filename, errno);
    XML_SetElementHandler(parser, xml_start_element_handler, xml_end_element_handler);
    char buf[4096];
    do
    {
        int len = read(fd, buf, 4096);
        // XXXKF not an optimal error/EOF handling :)
        if (len <= 0)
            break;
        XML_Parse(parser, buf, len, 0);
    } while(1);
    XML_Parse(parser, buf, 0, 1);
    close(fd);
}

void synth::save_presets(const char *filename)
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

void synth::add_preset(const plugin_preset &sp)
{
    presets.push_back(sp);
}
