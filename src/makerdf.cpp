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
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include <getopt.h>
#include <stdint.h>
#include <stdlib.h>
#include <config.h>
#include <calf/giface.h>
#include <calf/modules.h>
#include <calf/modules_dev.h>
#include <calf/plugininfo.h>
#include <calf/utils.h>
#if USE_LV2
#include <calf/lv2_event.h>
#include <calf/lv2_uri_map.h>
#endif

using namespace std;
using namespace synth;
using namespace calf_utils;

static struct option long_options[] = {
    {"help", 0, 0, 'h'},
    {"version", 0, 0, 'v'},
    {"mode", 0, 0, 'm'},
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

    #define RDF_EXPR(Module) \
        generate_ladspa_rdf(Module::plugin_info, Module::param_props, Module::port_names, Module::param_count, Module::in_count + Module::out_count);
    
    #define PER_MODULE_ITEM(name, isSynth, jackname) if (!isSynth) rdf += RDF_EXPR(name##_metadata)
    #define PER_SMALL_MODULE_ITEM(...)
    #include <calf/modulelist.h>
    
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

static void add_ctl_port(string &ports, parameter_properties &pp, int pidx, giface_plugin_info *gpi, int param)
{
    stringstream ss;
    const char *ind = "        ";

    
    if (ports != "") ports += " , ";
    ss << "[\n";
    if (pp.flags & PF_PROP_OUTPUT)
        ss << ind << "a lv2:OutputPort ;\n";
    else
        ss << ind << "a lv2:InputPort ;\n";
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
    if ((*gpi->is_noisy)(param))
        ss << ind << "lv2:portProperty epp:causesArtifacts ;\n";
    if (!(*gpi->is_cv)(param))
        ss << ind << "lv2:portProperty epp:notAutomatic ;\n";
    if (pp.flags & PF_PROP_OUTPUT_GAIN)
        ss << ind << "lv2:portProperty epp:outputGain ;\n";
    if ((pp.flags & PF_TYPEMASK) == PF_BOOL)
        ss << ind << "lv2:portProperty lv2:toggled ;\n";
    else if ((pp.flags & PF_TYPEMASK) == PF_ENUM)
    {
        ss << ind << "lv2:portProperty lv2:integer ;\n";
        for (int i = (int)pp.min; i <= (int)pp.max; i++)
            ss << ind << "lv2:scalePoint [ rdfs:label \"" << pp.choices[i - (int)pp.min] << "\"; rdf:value " << i <<" ] ;\n";
    }
    else if ((pp.flags & PF_TYPEMASK) > 0)
        ss << ind << "lv2:portProperty lv2:integer ;\n";
    ss << showpoint;
    ss << ind << "lv2:default " << pp.def_value << " ;\n";
    ss << ind << "lv2:minimum " << pp.min << " ;\n";
    ss << ind << "lv2:maximum " << pp.max << " ;\n";
    uint8_t unit = (pp.flags & PF_UNITMASK) >> 24;
    if (unit > 0 && unit < (sizeof(units) / sizeof(char *)) && units[unit - 1] != NULL)
        ss << ind << "ue:unit " << units[unit - 1] << " ;\n";
    
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
        if (!extras.empty())
            ss << ind << extras;
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
    string header;
    
    header = 
        "@prefix rdf:  <http://www.w3.org/1999/02/22-rdf-syntax-ns#> .\n"
        "@prefix lv2:  <http://lv2plug.in/ns/lv2core#> .\n"
        "@prefix rdfs: <http://www.w3.org/2000/01/rdf-schema#> .\n"
        "@prefix doap: <http://usefulinc.com/ns/doap#> .\n"
        "@prefix uiext: <http://lv2plug.in/ns/extensions/ui#> .\n"
        "@prefix lv2ev: <http://lv2plug.in/ns/ext/event#> .\n"
        "@prefix lv2midi: <http://lv2plug.in/ns/ext/midi#> .\n"
        "@prefix lv2ctx: <http://lv2plug.in/ns/dev/contexts#> .\n"
        "@prefix pg: <http://ll-plugins.nongnu.org/lv2/ext/portgroups#> .\n"
        "@prefix ue: <http://lv2plug.in/ns/extensions/units#> .\n"
        "@prefix epp: <http://lv2plug.in/ns/dev/extportinfo#> .\n"
        "@prefix kf: <http://foltman.com/ns/> .\n"

        "\n"
    ;
    
    vector<synth::giface_plugin_info> plugins;
    synth::get_all_plugins(plugins);
    
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
        
    string gui_header;
    
#if USE_LV2_GUI
    gui_header = "<http://calf.sourceforge.net/plugins/gui/gtk2-gui>\n"
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
    
    for (unsigned int i = 0; i < plugins.size(); i++) {
        synth::giface_plugin_info &pi = plugins[i];
        string uri = string("<http://calf.sourceforge.net/plugins/")  + string(pi.info->label) + ">";
        string ttl;
        ttl = "@prefix : " + uri + " .\n" + header + gui_header;
        
        ttl += uri + " a lv2:Plugin ;\n";
        
        if (classes.count(pi.info->plugin_type))
            ttl += "    a " + classes[pi.info->plugin_type]+" ;\n";
        
            
        ttl += "    doap:name \""+string(pi.info->name)+"\" ;\n";

#if USE_LV2_GUI
        ttl += "    uiext:ui <http://calf.sourceforge.net/plugins/gui/gtk2-gui> ;\n";
#endif
        
        ttl += "    doap:license <http://usefulinc.com/doap/licenses/lgpl> ;\n";
        // XXXKF not really optional for now, to be honest
        ttl += "    lv2:optionalFeature epp:supportsStrictBounds ;\n";
        if (pi.rt_capable)
            ttl += "    lv2:optionalFeature lv2:hardRtCapable ;\n";
        if (pi.midi_in_capable)
        {
            if (pi.midi_in_required) {
                ttl += "    lv2:requiredFeature <" LV2_EVENT_URI "> ;\n";
                ttl += "    lv2:requiredFeature <" LV2_URI_MAP_URI "> ;\n";                
            }
            else {
                ttl += "    lv2:optionalFeature <" LV2_EVENT_URI "> ;\n";
                ttl += "    lv2:optionalFeature <" LV2_URI_MAP_URI "> ;\n";                
            }
        }
        
        string ports = "";
        int pn = 0;
        const char *in_names[] = { "in_l", "in_r" };
        const char *out_names[] = { "out_l", "out_r" };
        for (int i = 0; i < pi.inputs; i++)
            add_port(ports, in_names[i], in_names[i], "Input", pn++);
        for (int i = 0; i < pi.outputs; i++)
            add_port(ports, out_names[i], out_names[i], "Output", pn++);
        for (int i = 0; i < pi.params; i++)
            add_ctl_port(ports, pi.param_props[i], pn++, &pi, i);
        if (pi.midi_in_capable) {
            add_port(ports, "event_in", "Event", "Input", pn++, "lv2ev:EventPort", true);
        }
        if (!ports.empty())
            ttl += "    lv2:port " + ports + "\n";
        ttl += ".\n\n";
        
        FILE *f = fopen((path_prefix+string(pi.info->label)+".ttl").c_str(), "w");
        fprintf(f, "%s\n", ttl.c_str());
        fclose(f);
    }
    lv2_plugin_list lpl;
    synth::get_all_small_plugins(&lpl);
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
}

void make_manifest()
{
    string ttl;
    
    ttl = 
        "@prefix lv2:  <http://lv2plug.in/ns/lv2core#> .\n"
        "@prefix rdfs: <http://www.w3.org/2000/01/rdf-schema#> .\n"
        "@prefix kf: <http://foltman.com/ns/> .\n"
        "\n"
        "kf:BooleanPlugin a rdfs:Class ; rdfs:label \"Boolean-oriented\" ; rdfs:subClassOf lv2:UtilityPlugin ; rdfs:comment \"\"\"Modules heavily inspired by digital electronics (gates, flip-flops etc.)\"\"\" .\n"
        "kf:MathOperatorPlugin a rdfs:Class ; rdfs:label \"Math operators\" ; rdfs:subClassOf lv2:UtilityPlugin ; rdfs:comment \"\"\"Mathematical operators and utility functions\"\"\" .\n"
        "kf:IntegerPlugin a rdfs:Class ; rdfs:label \"Integer-oriented\" ; rdfs:subClassOf lv2:UtilityPlugin ; rdfs:comment \"\"\"Operations using integer values (counters, multiplexers etc.)\"\"\" .\n"
        "kf:MIDIPlugin a rdfs:Class ; rdfs:label \"MIDI\" ; rdfs:subClassOf lv2:UtilityPlugin ; rdfs:comment \"\"\"Operations on MIDI streams (filters, transposers, mappers etc.)\"\"\" .\n"
    ;
    
    vector<synth::giface_plugin_info> plugins;
    synth::get_all_plugins(plugins);
    for (unsigned int i = 0; i < plugins.size(); i++)
        ttl += string("<http://calf.sourceforge.net/plugins/") 
            + string(plugins[i].info->label)
            + "> a lv2:Plugin ; lv2:binary <calf.so> ; rdfs:seeAlso <" + string(plugins[i].info->label) + ".ttl> .\n";

    lv2_plugin_list lpl;
    synth::get_all_small_plugins(&lpl);
    for (unsigned int i = 0; i < lpl.size(); i++)
        ttl += string("<http://calf.sourceforge.net/small_plugins/") 
            + string(lpl[i]->id)
            + "> a lv2:Plugin ; lv2:binary <calf.so> ; rdfs:seeAlso <" + string(lpl[i]->id) + ".ttl> .\n";

    printf("%s\n", ttl.c_str());    
}
#else
void make_manifest()
{
    fprintf(stderr, "LV2 not supported.\n");
}
void make_ttl(string tmp)
{
    fprintf(stderr, "LV2 not supported.\n");
}
#endif

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
                printf("LADSPA RDF / LV2 TTL generator for Calf plugin pack\nSyntax: %s [--help] [--version] [--mode rdf|ttl|manifest]\n", argv[0]);
                return 0;
            case 'v':
                printf("%s\n", PACKAGE_STRING);
                return 0;
            case 'm':
                mode = optarg;
                if (mode != "rdf" && mode != "ttl" && mode != "manifest") {
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
    else if (mode == "manifest")
        make_manifest();
#endif
    else
    {
        fprintf(stderr, "calfmakerdf: Mode %s unsupported in this version\n", optarg);
        return 1;
    }
    return 0;
}
