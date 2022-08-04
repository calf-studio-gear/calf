/* Calf DSP Library
 * Implementation of various helpers for the plugin interface.
 *
 * Copyright (C) 2001-2010 Krzysztof Foltman
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
#include <config.h>
#include <limits.h>
#include <stdlib.h>
#include <math.h>
#include <calf/giface.h>
#include <calf/utils.h>
#include <string.h>

#ifdef _MSC_VER 
#define strncasecmp _strnicmp
#define strcasecmp _stricmp
#endif

using namespace std;
using namespace calf_utils;
using namespace calf_plugins;

static const char automation_key_prefix[] = "automation_v1_";

void automation_range::send_configure(const plugin_metadata_iface *metadata, uint32_t from_controller, send_configure_iface *sci)
{
    std::stringstream ss1, ss2;
    ss1 << automation_key_prefix << from_controller << "_to_" << metadata->get_param_props(param_no)->short_name;
    ss2 << min_value << " " << max_value;
    sci->send_configure(ss1.str().c_str(), ss2.str().c_str());
}

automation_range *automation_range::new_from_configure(const plugin_metadata_iface *metadata, const char *key, const char *value, uint32_t &from_controller)
{
    if (0 != strncmp(key, automation_key_prefix, sizeof(automation_key_prefix) - 1))
        return NULL;
    key += sizeof(automation_key_prefix) - 1;
    const char *totoken = strstr(key, "_to_");
    if (!totoken)
        return NULL;
    string from_ctl(key, totoken - key);
    for (size_t i = 0; i < from_ctl.length(); i++)
    {
        if (!isdigit(from_ctl[i]))
            return NULL;
    }
    from_controller = (uint32_t)atoi(from_ctl.c_str());
    key = totoken + 4;

    size_t pcount = metadata->get_param_count();
    for (size_t i = 0; i < pcount; ++i) {
        const parameter_properties *props = metadata->get_param_props(i);
        if (!strcmp(key, props->short_name))
        {
            std::stringstream ss(value);
            double minv, maxv;
            ss >> minv >> maxv;
            return new automation_range(minv, maxv, i);
        }
    }

    return NULL;
}

float parameter_properties::from_01(double value01) const
{
    double value = dsp::clip(value01, 0., 1.);
    switch(flags & PF_SCALEMASK)
    {
    case PF_SCALE_DEFAULT:
    case PF_SCALE_LINEAR:
    case PF_SCALE_PERC:
    default:
        value = min + (max - min) * value01;
        break;
    case PF_SCALE_QUAD:
        value = min + (max - min) * value01 * value01;
        break;
    case PF_SCALE_LOG:
        value = min * pow(double(max / min), value01);
        break;
    case PF_SCALE_GAIN:
        if (value01 < 0.00001)
            value = min;
        else {
            float rmin = std::max(1.0f / 1024.0f, min);
            value = rmin * pow(double(max / rmin), value01);
        }
        break;
    case PF_SCALE_LOG_INF:
        assert(step);
        if (value01 > (step - 1.0) / step)
            value = FAKE_INFINITY;
        else
            value = min * pow(double(max / min), value01 * step / (step - 1.0));
        break;
    }
    switch(flags & PF_TYPEMASK)
    {
    case PF_INT:
    case PF_BOOL:
    case PF_ENUM:
    case PF_ENUM_MULTI:
        if (value > 0)
            value = (int)(value + 0.5);
        else
            value = (int)(value - 0.5);
        break;
    }
    return value;
}

double parameter_properties::to_01(float value) const
{
    switch(flags & PF_SCALEMASK)
    {
    case PF_SCALE_DEFAULT:
    case PF_SCALE_LINEAR:
    case PF_SCALE_PERC:
    default:
        return double(value - min) / (max - min);
    case PF_SCALE_QUAD:
        return sqrt(double(value - min) / (max - min));
    case PF_SCALE_LOG:
        value /= min;
        return log((double)value) / log((double)max / min);
    case PF_SCALE_LOG_INF:
        if (IS_FAKE_INFINITY(value))
            return max;
        value /= min;
        assert(step);
        return (step - 1.0) * log((double)value) / (step * log((double)max / min));
    case PF_SCALE_GAIN:
        if (value < 1.0 / 1024.0) // new bottom limit - 60 dB
            return 0;
        double rmin = std::max(1.0f / 1024.0f, min);
        value /= rmin;
        return log((double)value) / log(max / rmin);
    }
}

float parameter_properties::get_increment() const
{
    float increment = 0.01;
    if (step > 1)
        increment = 1.0 / (step - 1);
    else
    if (step > 0 && step < 1)
        increment = step;
    else
    if ((flags & PF_TYPEMASK) != PF_FLOAT)
        increment = 1.0 / (max - min);
    return increment;
}
std::string human_readable(float value, uint32_t base, char *format)
{
    // format is something like "%.2f%sB" for e.g. "1.23kB"
    // base is something like 1000 or 1024
    char buf[32];
    const char *suf[] = { "", "k", "m", "g", "t", "p", "e" };
    if (value == 0) {
        sprintf(buf, format, 0.f, "");
        return string(buf);
    }
    double val = abs(value);
    int place = (int)(log(val) / log(base));
    double num = val / pow(base, place);
    sprintf(buf, format, (float)((value > 0) - (value < 0)) * num, suf[place]);
    return string(buf);
}

int parameter_properties::get_char_count() const
{
    if ((flags & PF_SCALEMASK) == PF_SCALE_PERC)
        return 6;
    if ((flags & PF_SCALEMASK) == PF_SCALE_GAIN) {
        char buf[256];
        size_t len = 0;
        snprintf(buf, sizeof(buf), "%0.0f dB", 6.0 * log(min) / log(2));
        len = strlen(buf);
        snprintf(buf, sizeof(buf), "%0.0f dB", 6.0 * log(max) / log(2));
        len = std::max(len, strlen(buf)) + 2;
        return (int)len;
    }
    std::string min_ = to_string(min);
    std::string max_ = to_string(max);
    std::string mid_ = to_string(min + (max - min) * 1. / 3);
    return std::max((int)min_.length(), std::max((int)max_.length(), std::max((int)mid_.length(), 3)));
}


std::string parameter_properties::to_string(float value) const
{
    char buf[32];
    if ((flags & PF_SCALEMASK) == PF_SCALE_PERC) {
        snprintf(buf, sizeof(buf), "%0.2f%%", 100.0 * value);
        return string(buf);
    }
    if ((flags & PF_SCALEMASK) == PF_SCALE_GAIN) {
        if ((flags & PF_UNITMASK) == PF_UNIT_DBFS) {
            if (value < 1.0 / 1024.0) // new bottom limit - 60 dBFS
                return "-inf dBFS"; // XXXKF change to utf-8 infinity
            snprintf(buf, sizeof(buf), "%0.1f dBFS", dsp::amp2dB(value));
            return string(buf);
        }
        else {
            if (value < 1.0 / 1024.0) // new bottom limit - 60 dB
                return "-inf dB"; // XXXKF change to utf-8 infinity
            snprintf(buf, sizeof(buf), "%0.1f dB", dsp::amp2dB(value));
            return string(buf);
        }
    }
    std::string s_;
    switch(flags & PF_TYPEMASK)
    {
    case PF_INT:
    case PF_BOOL:
    case PF_ENUM:
    case PF_ENUM_MULTI:
        value = (int)value;
        //s_ = human_readable(value, 1000, (char*)"%g%s");
        //snprintf(buf, sizeof(buf), "%s", s_.c_str());
        snprintf(buf, sizeof(buf), "%g", value);
        //printf("%.2f %s\n", value, buf);
        break;
    case PF_FLOAT:
        value = round(value * 1000) / 1000;
        switch (flags & PF_DIGITMASK) {
            case PF_DIGIT_0:
                snprintf(buf, sizeof(buf), "%.0f", value);
                break;
            case PF_DIGIT_1:
                snprintf(buf, sizeof(buf), "%.1f", value);
                break;
            case PF_DIGIT_2:
                snprintf(buf, sizeof(buf), "%.2f", value);
                break;
            case PF_DIGIT_3:
                snprintf(buf, sizeof(buf), "%.3f", value);
                break;
            case PF_DIGIT_ALL:
            default:
                snprintf(buf, sizeof(buf), "%g", value);
                break;
        }
        break;
    default:
        snprintf(buf, sizeof(buf), "%g", value);
        break;
    }

    if ((flags & PF_SCALEMASK) == PF_SCALE_LOG_INF && IS_FAKE_INFINITY(value))
        snprintf(buf, sizeof(buf), "∞"); // XXXKF change to utf-8 infinity

    switch(flags & PF_UNITMASK) {
    case PF_UNIT_DB: return string(buf) + " dB";
    case PF_UNIT_DBFS: return string(buf) + " dBFS";
    case PF_UNIT_HZ: return string(buf) + " Hz";
    case PF_UNIT_SEC: return string(buf) + " s";
    case PF_UNIT_MSEC: return string(buf) + " ms";
    case PF_UNIT_CENTS: return string(buf) + " ct";
    case PF_UNIT_SEMITONES: return string(buf) + "#";
    case PF_UNIT_BPM: return string(buf) + " bpm";
    case PF_UNIT_RPM: return string(buf) + " rpm";
    case PF_UNIT_DEG: return string(buf) + " deg";
    case PF_UNIT_SAMPLES: return string(buf) + " smpl";
    case PF_UNIT_NOTE:
        {
            static const char *notes = "C C#D D#E F F#G G#A A#B ";
            int note = (int)value;
            if (note < 0 || note > 127)
                return "---";
            return string(notes + 2 * (note % 12), 2) + i2s(note / 12 - 2);
        }
    }

    return string(buf);
}

float parameter_properties::string_to_value(const char* string) const
{
    float value = atof(string);
    if ((flags & PF_SCALEMASK) == PF_SCALE_PERC) {
        return value / 100.0;
    }
    if ((flags & PF_SCALEMASK) == PF_SCALE_GAIN) {
        return dsp::dB2amp(value);
    }
    return value;
}

////////////////////////////////////////////////////////////////////////

void calf_plugins::plugin_ctl_iface::clear_preset() {
    int param_count = get_metadata_iface()->get_param_count();
    for (int i = 0; i < param_count; i++)
    {
        const parameter_properties &pp = *get_metadata_iface()->get_param_props(i);
        set_param_value(i, pp.def_value);
    }
    vector<string> vars;
    get_metadata_iface()->get_configure_vars(vars);
    for (size_t i = 0; i < vars.size(); ++i)
        configure(vars[i].c_str(), NULL);
}

char *calf_plugins::load_gui_xml(const std::string &plugin_id)
{
    try {
        return strdup(calf_utils::load_file((std::string(PKGLIBDIR) + "/" + plugin_id + ".xml").c_str()).c_str());
    }
    catch(file_exception &e)
    {
        return NULL;
    }
}

bool calf_plugins::get_freq_gridline(int subindex, float &pos, bool &vertical, std::string &legend, cairo_iface *context, bool use_frequencies, float res, float ofs)
{
    if (subindex < 0)
    return false;
    static const double dash[] = {2.0};
    // frequency grid
    if (use_frequencies)
    {
        if (subindex < 28)
        {
            vertical = true;
            if (subindex == 9) legend = "100 Hz";
            if (subindex == 18) legend = "1 kHz";
            if (subindex == 27) legend = "10 kHz";

            float freq = subindex_to_freq(subindex);
            pos = log(freq / 20.0) / log(1000);

            if (!legend.empty()) {
                context->set_source_rgba(0, 0, 0, 0.1);
                context->set_dash(dash, 0);
            } else {
                context->set_source_rgba(0, 0, 0, 0.1);
                context->set_dash(dash, 1);
            }
            return true;
        }
        subindex -= 28;
    }

    if (subindex >= 32)
        return false;

    // gain/dB grid
    float gain = 64.0 / (1 << subindex);
    pos = dB_grid(gain, res, ofs);

    if (pos < -1)
        return false;

    if (!(subindex & 1)) {
        std::stringstream ss;
        ss << (36 - 6 * subindex) << " dBFS";
        legend = ss.str();
    }
    if (!legend.empty() && subindex != 6) {
        context->set_source_rgba(0, 0, 0, 0.1);
        context->set_dash(dash, 0);
    } else if (subindex != 6) {
        context->set_source_rgba(0, 0, 0, 0.1);
        context->set_dash(dash, 1);
    } else {
        context->set_dash(dash, 0);
    }
    vertical = false;
    return true;
}

void calf_plugins::set_channel_color(cairo_iface *context, int channel, float alpha)
{
    if (channel & 1)
        context->set_source_rgba(0.25, 0.10, 0.0, alpha);
    else
        context->set_source_rgba(0.05, 0.25, 0.0, alpha);
}
void calf_plugins::set_channel_dash(cairo_iface *context, int channel)
{
    double dash[] = {8,2};
    int length = 2;
    switch (channel) {
        case 0:
        default:
            dash[0] = 6;
            dash[1] = 1.5;
            length = 2;
            break;
        case 1:
            dash[0] = 4.5;
            dash[1] = 1.5;
            length = 2;
            break;
        case 2:
            dash[0] = 3;
            dash[1] = 1.5;
            length = 2;
            break;
        case 3:
            dash[0] = 1.5;
            dash[1] = 1.5;
            length = 2;
            break;
    }
    context->set_dash(dash, length);
}

void calf_plugins::draw_cairo_label(cairo_iface *context, const char *label, float x, float y, int pos, float margin, float align)
{
    context->draw_label(label, x, y, pos, margin, align);
}
////////////////////////////////////////////////////////////////////////

bool frequency_response_line_graph::get_graph(int index, int subindex, int phase, float *data, int points, cairo_iface *context, int *mode) const
{
    if (phase || subindex)
        return false;
    return ::get_graph(*this, subindex, data, points);
}
bool frequency_response_line_graph::get_gridline(int index, int subindex, int phase, float &pos, bool &vertical, std::string &legend, cairo_iface *context) const
{
    if (phase)
        return false;
    return get_freq_gridline(subindex, pos, vertical, legend, context, true);
}
bool frequency_response_line_graph::get_layers(int index, int generation, unsigned int &layers) const
{
    redraw_graph = redraw_graph || !generation;
    layers = (generation ? LG_NONE : LG_CACHE_GRID) | (redraw_graph ? LG_CACHE_GRAPH : LG_NONE);
    bool r = redraw_graph;
    redraw_graph = false;
    return r;
}
std::string frequency_response_line_graph::get_crosshair_label(int x, int y, int sx, int sy, float q, int dB, int name, int note, int cents) const
{
    return frequency_crosshair_label(x, y, sx, sy, q, dB, name, note, cents);
}
std::string calf_plugins::frequency_crosshair_label(int x, int y, int sx, int sy, float q, int dB, int name, int note, int cents, double res, double ofs)
{
    std::stringstream ss;
    char str[1024];
    char tmp[1024];
    float f = exp((float(x) / float(sx)) * log(1000)) * 20.0;
    float db = dsp::amp2dB(dB_grid_inv(-1 + (2 - float(y) / float(sy) * 2), res, ofs));
    dsp::note_desc desc = dsp::hz_to_note(f, 440);
    sprintf(str, "%.2f Hz", f);
    if (dB) {
        sprintf(tmp, "%s\n%.2f dB", str, db);
        strcpy(str, tmp);
    }
    if (q) {
        sprintf(tmp, "%s\nQ: %.3f", str, q);
        strcpy(str, tmp);
    }
    if (name) {
        sprintf(tmp, "%s\nNote: %s%d", str, desc.name, desc.octave);
        strcpy(str, tmp);
    }
    if (cents) {
        sprintf(tmp, "%s\nCents: %+.2f", str, desc.cents);
        strcpy(str, tmp);
    }
    if (note) {
        sprintf(tmp, "%s\nMIDI: %d", str, desc.note);
        strcpy(str, tmp);
    }
    return string(str);
}



////////////////////////////////////////////////////////////////////////

calf_plugins::plugin_registry &calf_plugins::plugin_registry::instance()
{
    static calf_plugins::plugin_registry registry;
    return registry;
}

const plugin_metadata_iface *calf_plugins::plugin_registry::get_by_uri(const char *plugin_uri)
{
    static const char prefix[] = "http://calf.sourceforge.net/plugins/";
    if (strncmp(plugin_uri, prefix, sizeof(prefix) - 1))
        return NULL;
    const char *label = plugin_uri + sizeof(prefix) - 1;
    for (unsigned int i = 0; i < plugins.size(); i++)
    {
        if (!strcmp(plugins[i]->get_plugin_info().label, label))
            return plugins[i];
    }
    return NULL;
}

const plugin_metadata_iface *calf_plugins::plugin_registry::get_by_id(const char *id, bool case_sensitive)
{
    typedef int (*comparator)(const char *, const char *);
    comparator comp = case_sensitive ? strcmp : strcasecmp;
    for (unsigned int i = 0; i < plugins.size(); i++)
    {
        if (!comp(plugins[i]->get_id(), id))
            return plugins[i];
    }
    return NULL;
}

////////////////////////////////////////////////////////////////////////

bool calf_plugins::parse_table_key(const char *key, const char *prefix, bool &is_rows, int &row, int &column)
{
    is_rows = false;
    row = -1;
    column = -1;
    if (0 != strncmp(key, prefix, strlen(prefix)))
        return false;

    key += strlen(prefix);

    if (!strcmp(key, "rows"))
    {
        is_rows = true;
        return true;
    }

    const char *comma = strchr(key, ',');
    if (comma)
    {
        row = atoi(string(key, comma - key).c_str());
        column = atoi(comma + 1);
        return true;
    }

    printf("Unknown key %s under prefix %s", key, prefix);

    return false;
}

////////////////////////////////////////////////////////////////////////

const char *mod_mapping_names[] = { "0..1", "-1..1", "-1..0", "x^2", "2x^2-1", "ASqr", "ASqrBip", "Para", NULL };

mod_matrix_metadata::mod_matrix_metadata(unsigned int _rows, const char **_src_names, const char **_dest_names)
: mod_src_names(_src_names)
, mod_dest_names(_dest_names)
, matrix_rows(_rows)
{
    table_column_info tci[6] = {
        { "Source", TCT_ENUM, 0, 0, 0, mod_src_names },
        { "Mapping", TCT_ENUM, 0, 0, 0, mod_mapping_names },
        { "Modulator", TCT_ENUM, 0, 0, 0, mod_src_names },
        { "Amount", TCT_FLOAT, 0, 1, 1, NULL},
        { "Destination", TCT_ENUM, 0, 0, 0, mod_dest_names  },
        { NULL }
    };
    assert(sizeof(table_columns) == sizeof(tci));
    memcpy(table_columns, tci, sizeof(table_columns));
}

const table_column_info *mod_matrix_metadata::get_table_columns() const
{
    return table_columns;
}

uint32_t mod_matrix_metadata::get_table_rows() const
{
    return matrix_rows;
}

/// Return a list of configure variables used by the modulation matrix
void mod_matrix_metadata::get_configure_vars(std::vector<std::string> &names) const
{
    for (unsigned int i = 0; i < matrix_rows; ++i)
    {
        for (int j = 0; j < 5; j++)
        {
            char buf[40];
            snprintf(buf, sizeof(buf), "mod_matrix:%d,%d", i, j);
            names.push_back(buf);
        }
    }
}



////////////////////////////////////////////////////////////////////////

#if USE_EXEC_GUI

table_via_configure::table_via_configure()
{
    rows = 0;
}

table_via_configure::~table_via_configure()
{
}

#endif
