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
#include <assert.h>
#include <memory.h>
#include <jack/jack.h>
#include <calf/giface.h>
#include <calf/modules.h>

using namespace synth;
using namespace std;

float parameter_properties::from_01(float value01)
{
    switch(flags & PF_SCALEMASK)
    {
    case PF_SCALE_DEFAULT:
    case PF_SCALE_LINEAR:
    case PF_SCALE_PERC:
    default:
        return min + (max - min) * value01;
    case PF_SCALE_LOG:
        return min * pow(max / min, value01);
    case PF_SCALE_GAIN:
        if (value01 < 0.0001)
            return min;
        float rmin = 1.0f / 4096.0f;
        return rmin * pow(max / rmin, value01);
    }
}

float parameter_properties::to_01(float value)
{
    switch(flags & PF_SCALEMASK)
    {
    case PF_SCALE_DEFAULT:
    case PF_SCALE_LINEAR:
    case PF_SCALE_PERC:
    default:
        return (value - min) / (max - min);
    case PF_SCALE_LOG:
        value /= min;
        return log(value) / log(max / min);
    case PF_SCALE_GAIN:
        if (value < 1.0 / 4096.0)
            return 0;
        float rmin = 1.0f / 4096.0f;
        value /= rmin;
        return log(value) / log(max / rmin);
    }
}

static std::string i2s(int value)
{
    char buf[32];
    sprintf(buf, "%d", value);
    return buf;
}

static string f2s(double value)
{
    // XXXKF might not work with some locale settings
    char buf[64];
    sprintf(buf, "%g", value);
    return buf;
}

static string rdf_escape(const string &src)
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

static std::string unit_to_string(parameter_properties &props)
{
    uint32_t flags = props.flags & PF_UNITS;
    
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

std::string synth::generate_ladspa_rdf(const ladspa_info &info, parameter_properties *params, const char *param_names[], unsigned int count,
                                       unsigned int ctl_ofs)
{
    string rdf;
    string plugin_id = "&ladspa;" + i2s(info.unique_id);
    string plugin_type = string(info.plugin_type); 
    
    rdf += "  <ladspa:" + plugin_type + " rdf:about=\"" + plugin_id + "\">\n";
    rdf += "    <dc:creator>" + rdf_escape(info.maker) + "</dc:creator>\n";
    rdf += "    <dc:title>" + rdf_escape(info.name) + "</dc:title>\n";
    
    for (unsigned int i = 0; i < count; i++) {
        rdf += 
            "    <ladspa:hasPort>\n"
            "      <ladspa:InputControlPort rdf:about=\"" + plugin_id + "."+i2s(i )+"\" "
            + unit_to_string(params[i]) +
            "ladspa:hasLabel=\"par_"+i2s(i + ctl_ofs)+"\" "
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
            "             <ladspa:forPort rdf:resource=\"" + plugin_id + "." + i2s(i ) + "\"/>\n"
            "           </ladspa:PortValue>\n"
            "        </ladspa:hasPortValue>\n";
    }
    rdf += "      </ladspa:Default>\n"
        "    </ladspa:hasSetting>\n";

    rdf += "  </ladspa:" + plugin_type + ">\n";
    return rdf;
}
