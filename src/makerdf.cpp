/* Calf DSP Library
 * RDF file generator for LADSPA plugins.
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
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 * Boston, MA  02110-1301  USA
 */
#include <getopt.h>
#include <stdint.h>
#include <stdlib.h>
#include <config.h>
#include <calf/giface.h>
#include <calf/plugininfo.h>
#include <calf/utils.h>
#if USE_LV2
#include <calf/lv2_contexts.h>
#include <calf/lv2_event.h>
#include <calf/lv2_uri_map.h>
#endif

using namespace std;
using namespace calf_utils;
using namespace calf_plugins;

static struct option long_options[] = {
    {"help", 0, 0, 'h'},
    {"version", 0, 0, 'v'},
    {"mode", 0, 0, 'm'},
    {"path", 0, 0, 'p'},
    {0,0,0,0},
};

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

std::string generate_ladspa_rdf(const ladspa_plugin_info &info, parameter_properties *params, const char *param_names[], unsigned int count,
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

void make_rdf()
{
    string rdf;
    rdf = 
        "<?xml version='1.0' encoding='ISO-8859-1'?>\n"
        "<!DOCTYPE rdf:RDF [\n"
        "  <!ENTITY rdf 'http://www.w3.org/1999/02/22-rdf-syntax-ns#'>\n"
        "  <!ENTITY rdfs 'http://www.w3.org/2000/01/rdf-schema#'>\n"
        "  <!ENTITY dc 'http://purl.org/dc/elements/1.1/'>\n"
        "  <!ENTITY ladspa 'http://ladspa.org/ontology#'>\n"
        "]>\n";
    
    rdf += "<rdf:RDF xmlns:rdf=\"&rdf;\" xmlns:rdfs=\"&rdfs;\" xmlns:dc=\"&dc;\" xmlns:ladspa=\"&ladspa;\">\n";

    vector<calf_plugins::plugin_metadata_iface *> plugins;
    calf_plugins::get_all_plugins(plugins);
    for (unsigned int i = 0; i < plugins.size(); i++)
    {
        plugin_metadata_iface *p = plugins[i];
        if (!p->requires_midi()) {
            rdf += generate_ladspa_rdf(p->get_plugin_info(), p->get_param_props(0), p->get_port_names(), p->get_param_count(), p->get_param_port_offset());
        }
        delete p;
    }    
    plugins.clear();
    rdf += "</rdf:RDF>\n";
    
    printf("%s\n", rdf.c_str());
}
#endif

#if USE_LV2
static void add_port(string &ports, const char *symbol, const char *name, const char *direction, int pidx, const char *type = "lv2:AudioPort", bool optional = false)
{
    stringstream ss;
    const char *ind = "        ";

    
    if (ports != "") ports += " , ";
    ss << "[\n";
    if (direction) ss << ind << "a lv2:" << direction << "Port ;\n";
    ss << ind << "a " << type << " ;\n";
    ss << ind << "lv2:index " << pidx << " ;\n";
    ss << ind << "lv2:symbol \"" << symbol << "\" ;\n";
    ss << ind << "lv2:name \"" << name << "\" ;\n";
    if (optional)
        ss << ind << "lv2:portProperty lv2:connectionOptional ;\n";
    if (!strcmp(type, "lv2ev:EventPort")) {
        ss << ind << "lv2ev:supportsEvent lv2midi:MidiEvent ;\n";
        // XXXKF add a correct timestamp type here
        ss << ind << "lv2ev:supportsTimestamp <lv2ev:TimeStamp> ;\n";
    }
    if (!strcmp(symbol, "in_l")) 
        ss << ind << "pg:membership [ pg:group <#stereoIn>; pg:role pg:leftChannel ] ;" << endl;
    else
    if (!strcmp(symbol, "in_r")) 
        ss << ind << "pg:membership [ pg:group <#stereoIn>; pg:role pg:rightChannel ] ;" << endl;
    else
    if (!strcmp(symbol, "out_l")) 
        ss << ind << "pg:membership [ pg:group <#stereoOut>; pg:role pg:leftChannel ] ;" << endl;
    else
    if (!strcmp(symbol, "out_r")) 
        ss << ind << "pg:membership [ pg:group <#stereoOut>; pg:role pg:rightChannel ] ;" << endl;
    ss << "    ]";
    ports += ss.str();
}

static const char *units[] = { 
    "ue:db", 
    "ue:coef",
    "ue:hz",
    "ue:s",
    "ue:ms",
    "ue:cent", // - ask SWH (or maybe ue2: and write the extension by myself)
    "ue:semitone12TET", // - ask SWH
    "ue:bpm",
    "ue:degree",
    "ue:midiNote", // question to SWH: the extension says midiNode, but that must be a typo
    NULL // rotations per minute
};

//////////////// To all haters: calm down, I'll rewrite it to use the new interface one day

static void add_ctl_port(string &ports, parameter_properties &pp, int pidx, plugin_metadata_iface *pmi, int param)
{
    stringstream ss;
    const char *ind = "        ";

    parameter_flags type = (parameter_flags)(pp.flags & PF_TYPEMASK);
    uint8_t unit = (pp.flags & PF_UNITMASK) >> 24;
    
    if (ports != "") ports += " , ";
    ss << "[\n";
    if (pp.flags & PF_PROP_OUTPUT)
        ss << ind << "a lv2:OutputPort ;\n";
    else
        ss << ind << "a lv2:InputPort ;\n";
    if (type == PF_STRING)
        ss << ind << "a strport:StringPort ;\n";
    else
        ss << ind << "a lv2:ControlPort ;\n";
    ss << ind << "lv2:index " << pidx << " ;\n";
    ss << ind << "lv2:symbol \"" << pp.short_name << "\" ;\n";
    ss << ind << "lv2:name \"" << pp.name << "\" ;\n";
    if ((pp.flags & PF_CTLMASK) == PF_CTL_BUTTON)
        ss << ind << "lv2:portProperty epp:trigger ;\n";
    if (!(pp.flags & PF_PROP_NOBOUNDS))
        ss << ind << "lv2:portProperty epp:hasStrictBounds ;\n";
    if (pp.flags & PF_PROP_EXPENSIVE)
        ss << ind << "lv2:portProperty epp:expensive ;\n";
    if (pp.flags & PF_PROP_OPTIONAL)
        ss << ind << "lv2:portProperty lv2:connectionOptional ;\n";
    if (pmi->is_noisy(param))
        ss << ind << "lv2:portProperty epp:causesArtifacts ;\n";
    if (!pmi->is_cv(param))
        ss << ind << "lv2:portProperty epp:notAutomatic ;\n";
    if (pp.flags & PF_PROP_OUTPUT_GAIN)
        ss << ind << "lv2:portProperty epp:outputGain ;\n";
    if (pp.flags & PF_PROP_MSGCONTEXT)
        ss << ind << "lv2ctx:context lv2ctx:MessageContext ;\n";
    if (type == PF_STRING)
    {
        ss << ind << "strport:default \"\"\"" << pp.choices[0] << "\"\"\" ;\n";
    }
    else if (type == PF_BOOL)
        ss << ind << "lv2:portProperty lv2:toggled ;\n";
    else if (type == PF_ENUM)
    {
        ss << ind << "lv2:portProperty lv2:integer ;\n";
        for (int i = (int)pp.min; i <= (int)pp.max; i++)
            ss << ind << "lv2:scalePoint [ rdfs:label \"" << pp.choices[i - (int)pp.min] << "\"; rdf:value " << i <<" ] ;\n";
    }
    else if (type == PF_INT || type == PF_ENUM_MULTI)
        ss << ind << "lv2:portProperty lv2:integer ;\n";
    else if ((pp.flags & PF_SCALEMASK) == PF_SCALE_LOG)
        ss << ind << "lv2:portProperty epp:logarithmic ;\n";
    ss << showpoint;
    if (type != PF_STRING)
    {
        ss << ind << "lv2:default " << pp.def_value << " ;\n";
        ss << ind << "lv2:minimum " << pp.min << " ;\n";
        ss << ind << "lv2:maximum " << pp.max << " ;\n";
        if (pp.step > 1)
            ss << ind << "epp:rangeSteps " << pp.step << " ;\n";
        if (unit > 0 && unit < (sizeof(units) / sizeof(char *)) && units[unit - 1] != NULL)
            ss << ind << "ue:unit " << units[unit - 1] << " ;\n";
    }
    
    // for now I assume that the only tempo passed is the tempo the plugin should operate with
    // this may change as more complex plugins are added
    if (unit == (PF_UNIT_BPM >> 24))
        ss << ind << "lv2:portProperty epp:reportsBpm ;\n";
    
    ss << "    ]";
    ports += ss.str();
}

struct lv2_port_base {
    int index;
    std::string symbol, name, extras, microname;
    bool is_input;

    virtual std::string to_string() = 0;
    virtual ~lv2_port_base() {}
    void to_stream_base(stringstream &ss, string ind, string port_class)
    {
        ss << ind << (is_input ? "a lv2:InputPort ;\n" : "a lv2:OutputPort ;\n");
        ss << ind << "a " << port_class << " ;\n";
        ss << ind << "lv2:index " << index << " ;\n";
        ss << ind << "lv2:symbol \"" << symbol << "\" ;\n";
        ss << ind << "lv2:name \"" << name << "\" ;\n";
        if (!extras.empty()) {
            ss << calf_utils::indent(extras, ind);
        }
        if (microname != "N/A")
            ss << ind << "<http://lv2plug.in/ns/dev/tiny-name> \"" << microname << "\" ;\n";
    }
};

struct lv2_audio_port_base: public lv2_port_base
{
    std::string to_string() {
        stringstream ss;
        const char *ind = "        ";
        ss << "[\n";
        to_stream_base(ss, ind, "lv2:AudioPort");
        ss << "    ]\n";
        
        return ss.str();
    }
};

struct lv2_event_port_base: public lv2_port_base
{
    std::string to_string() {
        stringstream ss;
        const char *ind = "        ";
        ss << "[\n";
        to_stream_base(ss, ind, "lv2ev:EventPort");
        ss << "    ]\n";
        
        return ss.str();
    }
};

template<class base_iface, class base_data>
struct lv2_port_impl: public base_iface, public base_data
{
    lv2_port_impl(int _index, const std::string &_symbol, const std::string &_name, const std::string &_microname)
    {
        this->index = _index;
        this->symbol = _symbol;
        this->name = _name;
        this->microname = _microname;
        this->is_input = true;
    }
    /// Called if it's an input port
    virtual base_iface& input() { this->is_input = true; return *this; }
    /// Called if it's an output port
    virtual base_iface& output() { this->is_input = false; return *this; }
    virtual base_iface& lv2_ttl(const std::string &text) { this->extras += text + "\n"; return *this; }
};

struct lv2_control_port_base: public lv2_port_base
{
    bool is_log, is_toggle, is_trigger, is_integer;
    double min, max, def_value;
    bool has_min, has_max;
    
    lv2_control_port_base()
    {
        has_min = has_max = is_log = is_toggle = is_trigger = is_integer = false;
        def_value = 0.f;
    }
};

typedef lv2_port_impl<plain_port_info_iface, lv2_audio_port_base> lv2_audio_port_info;
typedef lv2_port_impl<plain_port_info_iface, lv2_event_port_base> lv2_event_port_info;

struct lv2_control_port_info: public lv2_port_impl<control_port_info_iface, lv2_control_port_base> 
{
    lv2_control_port_info(int _index, const std::string &_symbol, const std::string &_name, const std::string &_microname)
    : lv2_port_impl<control_port_info_iface, lv2_control_port_base>(_index, _symbol, _name, _microname)
    {
    }
    /// Called to mark the port as using linear range [from, to]
    virtual control_port_info_iface& lin_range(double from, double to) { min = from, max = to, has_min = true, has_max = true, is_log = false; return *this; }
    /// Called to mark the port as using log range [from, to]
    virtual control_port_info_iface& log_range(double from, double to) { min = from, max = to, has_min = true, has_max = true, is_log = true; return *this; }
    virtual control_port_info_iface& toggle() { is_toggle = true; return *this; }
    virtual control_port_info_iface& trigger() { is_trigger = true; return *this; }
    virtual control_port_info_iface& integer() { is_integer = true; return *this; }
    virtual control_port_info_iface& lv2_ttl(const std::string &text) { extras += text + "\n"; return *this; }
    std::string to_string() {
        stringstream ss;
        const char *ind = "        ";
        ss << "[\n";
        to_stream_base(ss, ind, "lv2:ControlPort");
        if (is_toggle)
            ss << ind << "lv2:portProperty lv2:toggled ;\n";
        if (is_integer)
            ss << ind << "lv2:portProperty lv2:integer ;\n";
        if (is_input)
            ss << ind << "lv2:default " << def_value << " ;\n";
        if (has_min)
            ss << ind << "lv2:minimum " << min << " ;\n";
        if (has_max)
            ss << ind << "lv2:maximum " << max << " ;\n";
        ss << "    ]\n";
        
        return ss.str();
    }
};

struct lv2_plugin_info: public plugin_info_iface
{
    /// Plugin id
    std::string id;
    /// Plugin name
    std::string name;
    /// Plugin label (short name)
    std::string label;
    /// Plugin class (category)
    std::string category;
    /// Plugin micro-name
    std::string microname;
    /// Extra declarations for LV2
    std::string extras;
    /// Vector of ports
    vector<lv2_port_base *> ports;
    /// Set plugin names (ID, name and label)
    virtual void names(const std::string &_name, const std::string &_label, const std::string &_category, const std::string &_microname) {
        name = _name;
        label = _label;
        category = _category;
        microname = _microname;
    }
    /// Add an audio port (returns a sink which accepts further description)
    virtual plain_port_info_iface &audio_port(const std::string &id, const std::string &name, const std::string &_microname) {
        lv2_audio_port_info *port = new lv2_audio_port_info(ports.size(), id, name, _microname);
        ports.push_back(port);
        return *port;
    }
    /// Add an eventport (returns a sink which accepts further description)
    virtual plain_port_info_iface &event_port(const std::string &id, const std::string &name, const std::string &_microname) {
        lv2_event_port_info *port = new lv2_event_port_info(ports.size(), id, name, _microname);
        ports.push_back(port);
        return *port;
    }
    /// Add a control port (returns a sink which accepts further description)
    virtual control_port_info_iface &control_port(const std::string &id, const std::string &name, double def_value, const std::string &_microname) {
        lv2_control_port_info *port = new lv2_control_port_info(ports.size(), id, name, _microname);
        port->def_value = def_value;
        ports.push_back(port);
        return *port;
    }
    /// Add extra TTL to plugin declaration
    virtual void lv2_ttl(const std::string &text) { this->extras += "    " + text + "\n";  }
    /// Called after plugin has reported all the information
    virtual void finalize() {
    }
};

struct lv2_plugin_list: public plugin_list_info_iface, public vector<lv2_plugin_info *>
{
    virtual plugin_info_iface &plugin(const std::string &id) {
        lv2_plugin_info *pi = new lv2_plugin_info;
        pi->id = id;
        push_back(pi);
        return *pi;
    }
};

void make_ttl(string path_prefix)
{
    if (path_prefix.empty())
    {
        fprintf(stderr, "Path parameter is required for TTL mode\n");
        exit(1);
    }
    string header;
    
    header = 
        "@prefix rdf:  <http://www.w3.org/1999/02/22-rdf-syntax-ns#> .\n"
        "@prefix lv2:  <http://lv2plug.in/ns/lv2core#> .\n"
        "@prefix rdfs: <http://www.w3.org/2000/01/rdf-schema#> .\n"
        "@prefix dc: <http://dublincore.org/documents/dcmi-namespace/> .\n"
        "@prefix doap: <http://usefulinc.com/ns/doap#> .\n"
        "@prefix uiext: <http://lv2plug.in/ns/extensions/ui#> .\n"
        "@prefix lv2ev: <http://lv2plug.in/ns/ext/event#> .\n"
        "@prefix lv2midi: <http://lv2plug.in/ns/ext/midi#> .\n"
        "@prefix lv2ctx: <http://lv2plug.in/ns/dev/contexts#> .\n"
        "@prefix strport: <http://lv2plug.in/ns/dev/string-port#> .\n"
        "@prefix pg: <http://ll-plugins.nongnu.org/lv2/ext/portgroups#> .\n"
        "@prefix ue: <http://lv2plug.in/ns/extensions/units#> .\n"
        "@prefix epp: <http://lv2plug.in/ns/dev/extportinfo#> .\n"
        "@prefix kf: <http://foltman.com/ns/> .\n"
        "@prefix foaf: <http://xmlns.com/foaf/0.1/> .\n"
        "@prefix poly: <http://lv2plug.in/ns/dev/polymorphic-port#> .\n"

        "\n"
    ;
    
    vector<plugin_metadata_iface *> plugins;
    calf_plugins::get_all_plugins(plugins);
    
    map<string, string> classes;
    
    const char *ptypes[] = {
        "Flanger", "Reverb", "Generator", "Instrument", "Oscillator",
        "Utility", "Converter", "Analyser", "Mixer", "Simulator",
        "Delay", "Modulator", "Phaser", "Chorus", "Filter",
        "Lowpass", "Highpass", "Bandpass", "Comb", "Allpass",
        "Amplifier", "Distortion", "Waveshaper", "Dynamics", "Compressor",
        "Expander", "Limiter", "Gate", NULL
    };
    
    for(const char **p = ptypes; *p; p++) {
        string name = string(*p) + "Plugin";
        classes[name] = "lv2:" + name;
    }
    classes["SynthesizerPlugin"] = "lv2:InstrumentPlugin";
        
    string plugin_uri_prefix = "http://calf.sourceforge.net/plugins/";

    string gui_header;
    
#if USE_LV2_GUI
    string gui_uri = "<http://calf.sourceforge.net/plugins/gui/gtk2-gui>";
    gui_header = gui_uri + "\n"
        "    a uiext:GtkUI ;\n"
        "    uiext:binary <calflv2gui.so> ;\n"
        "    uiext:requiredFeature uiext:makeResident .\n"
        "    \n"
#ifdef ENABLE_EXPERIMENTAL
    "<http://calf.sourceforge.net/small_plugins/gui/gtk2-gui>\n"
        "    a uiext:GtkUI ;\n"
        "    uiext:binary <calflv2gui.so> ;\n"
        "    uiext:requiredFeature uiext:makeResident .\n"
        "    \n"
#endif
    ;
#endif
    
    map<string, string> id_to_label;
    
    for (unsigned int i = 0; i < plugins.size(); i++) {
        plugin_metadata_iface *pi = plugins[i];
        const ladspa_plugin_info &lpi = pi->get_plugin_info();
        id_to_label[pi->get_id()] = pi->get_label();
        string uri = string("<" + plugin_uri_prefix)  + string(lpi.label) + ">";
        string ttl;
        ttl = "@prefix : " + uri + " .\n" + header + gui_header;
        
#if USE_LV2_GUI
        for (int j = 0; j < pi->get_param_count(); j++)
        {
            parameter_properties &props = *pi->get_param_props(j);
            if (props.flags & PF_PROP_OUTPUT)
                ttl += gui_uri + " uiext:portNotification [\n    uiext:plugin " + uri + " ;\n    uiext:portIndex " + i2s(j) + "\n] .\n\n";
        }
#endif
        
        ttl += uri + " a lv2:Plugin ;\n";
        
        if (classes.count(lpi.plugin_type))
            ttl += "    a " + classes[lpi.plugin_type]+" ;\n";
        
            
        ttl += "    doap:name \""+string(lpi.name)+"\" ;\n";
        ttl += "    doap:maintainer [ foaf:name \""+string(lpi.maker)+"\" ; ] ;\n";

#if USE_LV2_GUI
        ttl += "    uiext:ui <http://calf.sourceforge.net/plugins/gui/gtk2-gui> ;\n";
        ttl += "    lv2:optionalFeature <http://lv2plug.in/ns/ext/instance-access> ;\n";
        ttl += "    lv2:optionalFeature <http://lv2plug.in/ns/ext/data-access> ;\n";
#endif
        
        ttl += "    doap:license <http://usefulinc.com/doap/licenses/lgpl> ;\n";
        ttl += "    dc:replaces <ladspa:" + i2s(lpi.unique_id) + "> ;\n";
        // XXXKF not really optional for now, to be honest
        ttl += "    lv2:optionalFeature epp:supportsStrictBounds ;\n";
        if (pi->is_rt_capable())
            ttl += "    lv2:optionalFeature lv2:hardRtCapable ;\n";
        if (pi->get_midi())
        {
            if (pi->requires_midi()) {
                ttl += "    lv2:requiredFeature <" LV2_EVENT_URI "> ;\n";
                ttl += "    lv2:requiredFeature <" LV2_URI_MAP_URI "> ;\n";                
            }
            else {
                ttl += "    lv2:optionalFeature <" LV2_EVENT_URI "> ;\n";
                ttl += "    lv2:optionalFeature <" LV2_URI_MAP_URI "> ;\n";                
            }
        }
        
        if (pi->requires_message_context())
        {
            ttl += "    lv2:requiredFeature <http://lv2plug.in/ns/dev/contexts> ;\n";
            ttl += "    lv2:requiredFeature <" LV2_CONTEXT_MESSAGE "> ;\n";
            ttl += "    lv2ctx:requiredContext lv2ctx:MessageContext ;\n";
        }
        if (pi->requires_string_ports())
            ttl += "    lv2:requiredFeature <http://lv2plug.in/ns/dev/string-port#StringTransfer> ;\n";
        
        string ports = "";
        int pn = 0;
        const char *in_names[] = { "in_l", "in_r" };
        const char *out_names[] = { "out_l", "out_r" };
        for (int i = 0; i < pi->get_input_count(); i++)
            add_port(ports, in_names[i], in_names[i], "Input", pn++);
        for (int i = 0; i < pi->get_output_count(); i++)
            add_port(ports, out_names[i], out_names[i], "Output", pn++);
        for (int i = 0; i < pi->get_param_count(); i++)
            add_ctl_port(ports, *pi->get_param_props(i), pn++, pi, i);
        if (pi->get_midi()) {
            add_port(ports, "event_in", "Event", "Input", pn++, "lv2ev:EventPort", true);
        }
        if (!ports.empty())
            ttl += "    lv2:port " + ports + "\n";
        ttl += ".\n\n";
        
        FILE *f = fopen((path_prefix+string(lpi.label)+".ttl").c_str(), "w");
        fprintf(f, "%s\n", ttl.c_str());
        fclose(f);
    }
    lv2_plugin_list lpl;
    calf_plugins::get_all_small_plugins(&lpl);
    for (unsigned int i = 0; i < lpl.size(); i++)
    {
        lv2_plugin_info *pi = lpl[i];
        // Copy-pasted code is the root of all evil, I know!
        string uri = string("<http://calf.sourceforge.net/small_plugins/")  + string(pi->id) + ">";
        string ttl;
        ttl = "@prefix : " + uri + " .\n" + header + gui_header;
        
        ttl += uri + " a lv2:Plugin ;\n";
        
        if (!pi->category.empty())
            ttl += "    a " + pi->category+" ;\n";
        
        ttl += "    doap:name \""+string(pi->label)+"\" ;\n";
        ttl += "    doap:license <http://usefulinc.com/doap/licenses/lgpl> ;\n";
        if (!pi->microname.empty())
            ttl += "    <http://lv2plug.in/ns/dev/tiny-name> \"" + pi->microname + "\" ;\n";
        ttl += pi->extras;

        if (!pi->ports.empty())
        {
            ttl += "    lv2:port ";
            for (unsigned int i = 0; i < pi->ports.size(); i++)
            {
                if (i)
                    ttl += "    ,\n    ";
                ttl += pi->ports[i]->to_string();
            }
            ttl += ".\n\n";
        }
        FILE *f = fopen((path_prefix+string(pi->id)+".ttl").c_str(), "w");
        fprintf(f, "%s\n", ttl.c_str());
        fclose(f);
    }
    
    // Generate a manifest
    
    string ttl = 
        "@prefix lv2:  <http://lv2plug.in/ns/lv2core#> .\n"
        "@prefix lv2p:  <http://lv2plug.in/ns/dev/presets#> .\n"
        "@prefix rdfs: <http://www.w3.org/2000/01/rdf-schema#> .\n"
        "@prefix kf: <http://foltman.com/ns/> .\n"
        "\n"
        "kf:BooleanPlugin a rdfs:Class ; rdfs:subClassOf lv2:UtilityPlugin ;\n"
        "    rdfs:label \"Boolean-oriented\" ;\n    rdfs:comment \"Modules heavily inspired by digital electronics (gates, flip-flops, etc.)\" .\n"
        "kf:MathOperatorPlugin a rdfs:Class ; rdfs:subClassOf lv2:UtilityPlugin ;\n"
        "    rdfs:label \"Math operators\" ;\n    rdfs:comment \"Mathematical operators and utility functions\" .\n"
        "kf:IntegerPlugin a rdfs:Class ; rdfs:subClassOf lv2:UtilityPlugin ;\n"
        "    rdfs:label \"Integer-oriented\" ;\n    rdfs:comment \"Operations on integer values (counters, multiplexers, etc.)\" .\n"
        "kf:MIDIPlugin a rdfs:Class ; rdfs:subClassOf lv2:UtilityPlugin ;\n"
        "    rdfs:label \"MIDI\" ;\n    rdfs:comment \"Operations on MIDI streams (filters, transposers, mappers, etc.)\" .\n"
		"\n"
    ;
    
    string presets_ttl =
        "@prefix lv2:  <http://lv2plug.in/ns/lv2core#> .\n"
        "@prefix lv2p:  <http://lv2plug.in/ns/dev/presets#> .\n"
        "@prefix rdfs: <http://www.w3.org/2000/01/rdf-schema#> .\n"
        "@prefix dc: <http://dublincore.org/documents/dcmi-namespace/> .\n"
        "\n"
    ;

    for (unsigned int i = 0; i < plugins.size(); i++)
        ttl += string("<" + plugin_uri_prefix) 
            + string(plugins[i]->get_plugin_info().label)
            + "> a lv2:Plugin ;\n    lv2:binary <calf.so> ; rdfs:seeAlso <" + string(plugins[i]->get_plugin_info().label) + ".ttl> , <presets.ttl> .\n";

    for (unsigned int i = 0; i < lpl.size(); i++)
        ttl += string("<http://calf.sourceforge.net/small_plugins/") 
            + string(lpl[i]->id)
            + "> a lv2:Plugin ;\n    lv2:binary <calf.so> ; rdfs:seeAlso <" + string(lpl[i]->id) + ".ttl> .\n";

    calf_plugins::get_builtin_presets().load_defaults(true);
    calf_plugins::preset_vector &factory_presets = calf_plugins::get_builtin_presets().presets;

    ttl += "\n";

    for (unsigned int i = 0; i < factory_presets.size(); i++)
    {
        plugin_preset &pr = factory_presets[i];
        map<string, string>::iterator ilm = id_to_label.find(pr.plugin);
        if (ilm == id_to_label.end())
            continue;
        string uri = "<http://calf.sourceforge.net/factory_presets#"
            + pr.plugin + "_" + pr.get_safe_name()
            + ">";
        ttl += string("<" + plugin_uri_prefix + ilm->second + "> lv2p:hasPreset\n    " + uri + " .\n");
        
        presets_ttl += uri + 
            " a lv2p:Preset ;\n"
            "    dc:title \"" + pr.name + "\" ;\n"
            "    lv2p:appliesTo <" + plugin_uri_prefix + ilm->second + "> ;\n"
            "    lv2:port \n"
        ;
        
        unsigned int count = min(pr.param_names.size(), pr.values.size());
        for (unsigned int j = 0; j < count; j++)
        {
            presets_ttl += "        [ lv2:symbol \"" + pr.param_names[j] + "\" ; lv2p:value " + ff2s(pr.values[j]) + " ] ";
            if (j < count - 1)
                presets_ttl += ',';
            presets_ttl += '\n';
        }
        presets_ttl += ".\n\n";
    }
    FILE *f = fopen((path_prefix+"manifest.ttl").c_str(), "w");
    fprintf(f, "%s\n", ttl.c_str());
    fclose(f);
    f = fopen((path_prefix+"presets.ttl").c_str(), "w");
    fprintf(f, "%s\n", presets_ttl.c_str());
    fclose(f);
}

#else
void make_ttl(string tmp)
{
    fprintf(stderr, "LV2 not supported.\n");
}

#endif

void make_gui(string path_prefix)
{
    if (path_prefix.empty())
    {
        fprintf(stderr, "Path parameter is required for GUI mode\n");
        exit(1);
    }
    vector<plugin_metadata_iface *> plugins;
    calf_plugins::get_all_plugins(plugins);
    path_prefix += "/gui-";
    for (unsigned int i = 0; i < plugins.size(); i++)
    {
        plugin_metadata_iface *pi = plugins[i];
        
        stringstream xml;
        int graphs = 0;
        for (int j = 0; j < pi->get_param_count(); j++)
        {
            parameter_properties &props = *pi->get_param_props(j);
            if (props.flags & PF_PROP_GRAPH)
                graphs++;
        }
        xml << "<table rows=\"" << pi->get_param_count() << "\" cols=\"" << (graphs ? "4" : "3") << "\">\n";
        for (int j = 0; j < pi->get_param_count(); j++)
        {
            if (j)
                xml << "\n    <!-- -->\n\n";
            parameter_properties &props = *pi->get_param_props(j);
            string expand_x = "expand-x=\"1\" ";
            string fill_x = "fill-x=\"1\" ";
            string shrink_x = "shrink-x=\"1\" ";
            string pad_x = "pad-x=\"10\" ";
            string attach_x = "attach-x=\"1\" attach-w=\"2\" ";
            string attach_y = "attach-y=\"" + i2s(j) + "\" ";
            string param = "param=\"" + string(props.short_name) + "\" ";
            string label = "    <align attach-x=\"0\" " + attach_y + " " + " align-x=\"1\"><label " + param + " /></align>\n";
            string value = "    <value " + param + " " + "attach-x=\"2\" " + attach_y + pad_x + "/>\n";
            string attach_xv = "attach-x=\"1\" attach-w=\"1\" ";
            string ctl;
            if ((props.flags & PF_TYPEMASK) == PF_ENUM && 
                (props.flags & PF_CTLMASK) == PF_CTL_COMBO)
            {
                ctl = "    <combo " + attach_x + attach_y + param + expand_x + pad_x + " />\n";
            }
            else if ((props.flags & PF_TYPEMASK) == PF_STRING)
            {
                ctl = "    <entry " + attach_x + attach_y + "key=\"" + props.short_name + "\" " + expand_x + pad_x + " />\n";
            }
            else if ((props.flags & PF_TYPEMASK) == PF_BOOL && 
                     (props.flags & PF_CTLMASK) == PF_CTL_TOGGLE)
            {
                ctl = "    <align " + attach_x + attach_y + expand_x + fill_x + pad_x + "><toggle " + param + "/></align>\n";
            }
            else if ((props.flags & PF_TYPEMASK) == PF_BOOL && 
                     (props.flags & PF_CTLMASK) == PF_CTL_BUTTON)
            {
                ctl = "    <button attach-x=\"0\" attach-w=\"3\" " + expand_x + attach_y + param + pad_x + "/>\n";
                label.clear();
            }
            else if ((props.flags & PF_TYPEMASK) == PF_BOOL && 
                     (props.flags & PF_CTLMASK) == PF_CTL_LED)
            {
                ctl = "    <led " + attach_x + attach_y + param + shrink_x + pad_x + " />\n";
            }
            else if ((props.flags & PF_CTLMASK) == PF_CTL_METER)
            {
                if (props.flags & PF_CTLO_LABEL) {
                    attach_x = attach_xv;
                    pad_x.clear();
                }
                if (props.flags & PF_CTLO_REVERSE)
                    ctl = "    <vumeter " + attach_x + attach_y + expand_x + fill_x + param + pad_x + "mode=\"2\"/>\n";
                else
                    ctl = "    <vumeter " + attach_x + attach_y + expand_x + fill_x + param + pad_x + "/>\n";
                if (props.flags & PF_CTLO_LABEL)
                    ctl += value;
            }
            else if ((props.flags & PF_CTLMASK) != PF_CTL_FADER)
            {
                if ((props.flags & PF_UNITMASK) == PF_UNIT_DEG)
                    ctl = "    <knob " + attach_xv + attach_y + shrink_x + param + " type=\"3\"/>\n";
                else
                    ctl = "    <knob " + attach_xv + attach_y + shrink_x + param + " />\n";
                ctl += value;
            }
            else
            {
                ctl += "     <hscale " + param + " " + attach_x + attach_y + " pad-x=\"10\"/>";
            }
            if (!label.empty()) 
                xml << label;
            if (!ctl.empty()) 
                xml << ctl;
        }
        if (graphs)
        {
            xml << "    <if cond=\"directlink\">" << endl;
            xml << "        <vbox expand-x=\"1\" fill-x=\"1\" attach-x=\"3\" attach-y=\"0\" attach-h=\"" << pi->get_param_count() << "\">" << endl;
            for (int j = 0; j < pi->get_param_count(); j++)
            {
                parameter_properties &props = *pi->get_param_props(j);
                if (props.flags & PF_PROP_GRAPH)
                {
                    xml << "            <line-graph refresh=\"1\" width=\"160\" param=\"" << props.short_name << "\"/>\n" << endl;
                }
            }
            xml << "        </vbox>" << endl;
            xml << "    </if>" << endl;
        }
        xml << "</table>\n";
        FILE *f = fopen((path_prefix+string(pi->get_id())+".xml").c_str(), "w");
        fprintf(f, "%s\n", xml.str().c_str());
        fclose(f);
    }
}

int main(int argc, char *argv[])
{
    string mode = "rdf";
    string path_prefix;
    while(1) {
        int option_index;
        int c = getopt_long(argc, argv, "hvm:p:", long_options, &option_index);
        if (c == -1)
            break;
        switch(c) {
            case 'h':
            case '?':
                printf("LADSPA RDF / LV2 TTL / XML GUI generator for Calf plugin pack\nSyntax: %s [--help] [--version] [--mode rdf|ttl|gui] [--path <path>]\n", argv[0]);
                return 0;
            case 'v':
                printf("%s\n", PACKAGE_STRING);
                return 0;
            case 'm':
                mode = optarg;
                if (mode != "rdf" && mode != "ttl" && mode != "gui") {
                    fprintf(stderr, "calfmakerdf: Invalid mode %s\n", optarg);
                    return 1;
                }
                break;
            case 'p':
                path_prefix = optarg;
                break;
        }
    }
    if (false)
    {
    }
#if USE_LADSPA
    else if (mode == "rdf")
        make_rdf();
#endif
#if USE_LV2
    else if (mode == "ttl")
        make_ttl(path_prefix);
#endif
    else if (mode == "gui")
        make_gui(path_prefix);
    else
    {
        fprintf(stderr, "calfmakerdf: Mode %s unsupported in this version\n", optarg);
        return 1;
    }
    return 0;
}
