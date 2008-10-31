/* Calf DSP Library
 * Module wrapper methods.
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
#include <config.h>
#include <assert.h>
#include <memory.h>
#include <calf/giface.h>
#include <calf/modules.h>
#include <calf/lv2wrap.h>

using namespace synth;
using namespace std;

static std::string i2s(int value)
{
    char buf[32];
    sprintf(buf, "%d", value);
    
    return string(buf);
}

static string f2s(double value)
{
    // XXXKF might not work with some locale settings
    char buf[64];
    sprintf(buf, "%g", value);
    return buf;
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

int parameter_properties::get_char_count() const
{
    if ((flags & PF_SCALEMASK) == PF_SCALE_PERC)
        return 6;
    if ((flags & PF_SCALEMASK) == PF_SCALE_GAIN) {
        char buf[256];
        size_t len = 0;
        sprintf(buf, "%0.0f dB", 6.0 * log(min) / log(2));
        len = strlen(buf);
        sprintf(buf, "%0.0f dB", 6.0 * log(max) / log(2));
        len = std::max(len, strlen(buf)) + 2;
        return (int)len;
    }
    return std::max(to_string(min).length(), std::max(to_string(max).length(), to_string(max * 0.999999).length()));
}

std::string parameter_properties::to_string(float value) const
{
    char buf[32];
    if ((flags & PF_SCALEMASK) == PF_SCALE_PERC) {
        sprintf(buf, "%0.f%%", 100.0 * value);
        return string(buf);
    }
    if ((flags & PF_SCALEMASK) == PF_SCALE_GAIN) {
        if (value < 1.0 / 1024.0) // new bottom limit - 60 dB
            return "-inf dB"; // XXXKF change to utf-8 infinity
        sprintf(buf, "%0.1f dB", 6.0 * log(value) / log(2));
        return string(buf);
    }
    switch(flags & PF_TYPEMASK)
    {
    case PF_INT:
    case PF_BOOL:
    case PF_ENUM:
    case PF_ENUM_MULTI:
        value = (int)value;
        break;
    }

    if ((flags & PF_SCALEMASK) == PF_SCALE_LOG_INF && IS_FAKE_INFINITY(value))
        sprintf(buf, "+inf"); // XXXKF change to utf-8 infinity
    else
        sprintf(buf, "%g", value);
    
    switch(flags & PF_UNITMASK) {
    case PF_UNIT_DB: return string(buf) + " dB";
    case PF_UNIT_HZ: return string(buf) + " Hz";
    case PF_UNIT_SEC: return string(buf) + " s";
    case PF_UNIT_MSEC: return string(buf) + " ms";
    case PF_UNIT_CENTS: return string(buf) + " ct";
    case PF_UNIT_SEMITONES: return string(buf) + "#";
    case PF_UNIT_BPM: return string(buf) + " bpm";
    case PF_UNIT_RPM: return string(buf) + " rpm";
    case PF_UNIT_DEG: return string(buf) + " deg";
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

std::string synth::xml_escape(const std::string &src)
{
    string dest;
    for (size_t i = 0; i < src.length(); i++) {
        // XXXKF take care of string encoding
        if (src[i] < 0 || src[i] == '"' || src[i] == '<' || src[i] == '>' || src[i] == '&')
            dest += "&"+i2s((uint8_t)src[i])+";";
        else
            dest += src[i];
    }
    return dest;
}

#if USE_LADSPA

static std::string unit_to_string(parameter_properties &props)
{
    uint32_t flags = props.flags & PF_UNITMASK;
    
    switch(flags) {
        case PF_UNIT_DB:
            return "ladspa:hasUnit=\"&ladspa;dB\" ";
        case PF_UNIT_COEF:
            return "ladspa:hasUnit=\"&ladspa;coef\" ";
        case PF_UNIT_HZ:
            return "ladspa:hasUnit=\"&ladspa;Hz\" ";
        case PF_UNIT_SEC:
            return "ladspa:hasUnit=\"&ladspa;seconds\" ";
        case PF_UNIT_MSEC:
            return "ladspa:hasUnit=\"&ladspa;milliseconds\" ";
        default:
            return string();
    }
}

static std::string scale_to_string(parameter_properties &props)
{
    if ((props.flags & PF_TYPEMASK) != PF_ENUM) {
        return "/";
    }
    string tmp = "><ladspa:hasScale><ladspa:Scale>\n";
    for (int i = (int)props.min; i <= (int)props.max; i++) {
        tmp += "          <ladspa:hasPoint><ladspa:Point rdf:value=\""+i2s(i)+"\" ladspa:hasLabel=\""+props.choices[(int)(i - props.min)]+"\" /></ladspa:hasPoint>\n";
    }
    return tmp+"        </ladspa:Scale></ladspa:hasScale></ladspa:InputControlPort";
}

std::string synth::generate_ladspa_rdf(const ladspa_plugin_info &info, parameter_properties *params, const char *param_names[], unsigned int count,
                                       unsigned int ctl_ofs)
{
    string rdf;
    string plugin_id = "&ladspa;" + i2s(info.unique_id);
    string plugin_type = string(info.plugin_type); 
    
    rdf += "  <ladspa:" + plugin_type + " rdf:about=\"" + plugin_id + "\">\n";
    rdf += "    <dc:creator>" + xml_escape(info.maker) + "</dc:creator>\n";
    rdf += "    <dc:title>" + xml_escape(info.name) + "</dc:title>\n";
    
    for (unsigned int i = 0; i < count; i++) {
        rdf += 
            "    <ladspa:hasPort>\n"
            "      <ladspa:" + string(params[i].flags & PF_PROP_OUTPUT ? "Output" : "Input") 
            + "ControlPort rdf:about=\"" + plugin_id + "."+i2s(ctl_ofs + i)+"\" "
            + unit_to_string(params[i]) +
            "ladspa:hasLabel=\"" + params[i].short_name + "\" "
            + scale_to_string(params[i]) + 
            ">\n"
            "    </ladspa:hasPort>\n";
    }
    rdf += "    <ladspa:hasSetting>\n"
        "      <ladspa:Default>\n";
    for (unsigned int i = 0; i < count; i++) {
        rdf += 
            "        <ladspa:hasPortValue>\n"
            "           <ladspa:PortValue rdf:value=\"" + f2s(params[i].def_value) + "\">\n"
            "             <ladspa:forPort rdf:resource=\"" + plugin_id + "." + i2s(ctl_ofs + i) + "\"/>\n"
            "           </ladspa:PortValue>\n"
            "        </ladspa:hasPortValue>\n";
    }
    rdf += "      </ladspa:Default>\n"
        "    </ladspa:hasSetting>\n";

    rdf += "  </ladspa:" + plugin_type + ">\n";
    return rdf;
}

#endif
