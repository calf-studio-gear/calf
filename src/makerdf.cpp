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
#include <calf/giface.h>
#include <calf/preset.h>
#include <calf/utils.h>
#if USE_LV2
#include <lv2.h>
#include <calf/lv2_event.h>
#include <calf/lv2_persist.h>
#include <calf/lv2_uri_map.h>
#endif
#include <getopt.h>
#include <string.h>
#include <set>

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

static std::string unit_to_string(const parameter_properties &props)
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

static std::string scale_to_string(const parameter_properties &props)
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

std::string generate_ladspa_rdf(const ladspa_plugin_info &info, const parameter_properties *params, const char *param_names[], unsigned int count,
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

    const plugin_registry::plugin_vector &plugins = plugin_registry::instance().get_all();
    set<int> used_ids;
    for (unsigned int i = 0; i < plugins.size(); i++)
    {
        const plugin_metadata_iface *p = plugins[i];
        const ladspa_plugin_info &info = p->get_plugin_info();
        
        if(used_ids.count(info.unique_id))
        {
            fprintf(stderr, "ERROR: Duplicate ID %d in plugin %s\n", info.unique_id, info.name);
            assert(0);
        }
        used_ids.insert(info.unique_id);

        if (!p->requires_midi()) {
            rdf += generate_ladspa_rdf(info, p->get_param_props(0), p->get_port_names(), p->get_param_count(), p->get_param_port_offset());
        }
        delete p;
    }    
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

static bool add_ctl_port(string &ports, const parameter_properties &pp, int pidx, const plugin_metadata_iface *pmi, int param)
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
    if (type == PF_BOOL)
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
    if (!(pp.flags & PF_PROP_OUTPUT))
        ss << ind << "lv2:default " << pp.def_value << " ;\n";
    ss << ind << "lv2:minimum " << pp.min << " ;\n";
    ss << ind << "lv2:maximum " << pp.max << " ;\n";
    if (pp.step > 1)
        ss << ind << "epp:rangeSteps " << pp.step << " ;\n";
    if (unit > 0 && unit < (sizeof(units) / sizeof(char *)) && units[unit - 1] != NULL)
        ss << ind << "ue:unit " << units[unit - 1] << " ;\n";
    
    // for now I assume that the only tempo passed is the tempo the plugin should operate with
    // this may change as more complex plugins are added
    if (unit == (PF_UNIT_BPM >> 24))
        ss << ind << "lv2:portProperty epp:reportsBpm ;\n";
    
    ss << "    ]";
    ports += ss.str();
    return true;
}

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
        "@prefix foaf: <http://xmlns.com/foaf/0.1/> .\n"

        "\n"
    ;
    
    const plugin_registry::plugin_vector &plugins = plugin_registry::instance().get_all();
    
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
    string gtkgui_uri = "<http://calf.sourceforge.net/plugins/gui/gtk2-gui>";
#if USE_LV2_GTK_GUI
    gui_header = gtkgui_uri + "\n"
        "    a uiext:GtkUI ;\n"
        "    uiext:binary <calflv2gui.so> ;\n"
        "    uiext:requiredFeature uiext:makeResident .\n"
        "\n"
    ;
#endif
    string extgui_uri = "<http://calf.sourceforge.net/plugins/gui/ext-gui>";
    gui_header += extgui_uri + "\n"
        "    a uiext:external ;\n"
        "    uiext:binary <calflv2gui.so> .\n"
        "\n"
    ;
#endif
    
    map<string, string> id_to_label;
    
    for (unsigned int i = 0; i < plugins.size(); i++) {
        const plugin_metadata_iface *pi = plugins[i];
        const ladspa_plugin_info &lpi = pi->get_plugin_info();
        id_to_label[pi->get_id()] = pi->get_label();
        string uri = string("<" + plugin_uri_prefix)  + string(lpi.label) + ">";
        string ttl;
        ttl = "@prefix : " + uri + " .\n" + header + gui_header;
        
#if USE_LV2_GUI
        for (int j = 0; j < pi->get_param_count(); j++)
        {
            const parameter_properties &props = *pi->get_param_props(j);
            if (props.flags & PF_PROP_OUTPUT)
            {
                string portnot = " uiext:portNotification [\n    uiext:plugin " + uri + " ;\n    uiext:portIndex " + i2s(j) + "\n] .\n\n";
#if USE_LV2_GTK_GUI
                ttl += gtkgui_uri + portnot;
#endif
                ttl += extgui_uri + portnot;
            }
        }
#endif
        
        ttl += uri + " a lv2:Plugin ;\n";
        
        if (classes.count(lpi.plugin_type))
            ttl += "    a " + classes[lpi.plugin_type]+" ;\n";
        
            
        ttl += "    doap:name \""+string(lpi.name)+"\" ;\n";
        ttl += "    doap:maintainer [ foaf:name \""+string(lpi.maker)+"\" ; ] ;\n";

#if USE_LV2_GUI
#if USE_LV2_GTK_GUI
        ttl += "    uiext:ui <http://calf.sourceforge.net/plugins/gui/gtk2-gui> ;\n";
#endif
        ttl += "    uiext:ui <http://calf.sourceforge.net/plugins/gui/ext-gui> ;\n";
        ttl += "    lv2:optionalFeature <http://lv2plug.in/ns/ext/instance-access> ;\n";
        ttl += "    lv2:optionalFeature <http://lv2plug.in/ns/ext/data-access> ;\n";
#endif
        
        ttl += "    doap:license <http://usefulinc.com/doap/licenses/lgpl> ;\n";
        ttl += "    dc:replaces <urn:ladspa:" + i2s(lpi.unique_id) + "> ;\n";
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
        
        if (pi->get_configure_vars())
        {
            ttl += "    lv2:optionalFeature <" LV2_PERSIST_URI "> ;\n";
        }
        
        string ports = "";
        int pn = 0;
        const char *in_names[] = { "in_l", "in_r", "sidechain" };
        const char *out_names[] = { "out_l", "out_r" };
        for (int i = 0; i < pi->get_input_count(); i++)
            if(i <= pi->get_input_count() - pi->get_inputs_optional() - 1)
                add_port(ports, in_names[i], in_names[i], "Input", pn++);
            else
                add_port(ports, in_names[i], in_names[i], "Input", pn++, "lv2:AudioPort", true);
        for (int i = 0; i < pi->get_output_count(); i++)
            if(i <= pi->get_output_count() - pi->get_outputs_optional() - 1)
                add_port(ports, out_names[i], out_names[i], "Output", pn++);
            else
                add_port(ports, out_names[i], out_names[i], "Output", pn++, "lv2:AudioPort", true);
        for (int i = 0; i < pi->get_param_count(); i++)
            if (add_ctl_port(ports, *pi->get_param_props(i), pn, pi, i))
                pn++;
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
    // Generate a manifest
    
    // Prefixes for the manifest TTL
    string ttl = 
        "@prefix lv2:  <http://lv2plug.in/ns/lv2core#> .\n"
        "@prefix lv2p:  <http://lv2plug.in/ns/dev/presets#> .\n"
        "@prefix rdfs: <http://www.w3.org/2000/01/rdf-schema#> .\n"
        "\n"
    ;
    
    // Prefixes for the preset TTL
    string presets_ttl_head =
        "@prefix lv2:  <http://lv2plug.in/ns/lv2core#> .\n"
        "@prefix lv2p:  <http://lv2plug.in/ns/dev/presets#> .\n"
        "@prefix rdfs: <http://www.w3.org/2000/01/rdf-schema#> .\n"
        "@prefix dc: <http://dublincore.org/documents/dcmi-namespace/> .\n"
        "\n"
    ;
    
    calf_plugins::get_builtin_presets().load_defaults(true);
    calf_plugins::preset_vector &factory_presets = calf_plugins::get_builtin_presets().presets;

    ttl += "\n";
    
    // We'll collect TTL for presets here, maps plugin label to TTL generated so far
    map<string, string> preset_data;

    for (unsigned int i = 0; i < factory_presets.size(); i++)
    {
        plugin_preset &pr = factory_presets[i];
        map<string, string>::iterator ilm = id_to_label.find(pr.plugin);
        if (ilm == id_to_label.end())
            continue;
        
        string presets_ttl;
        // if this is the first preset, add a header
        if (!preset_data.count(ilm->second))
            presets_ttl = presets_ttl_head;
        
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
        
        preset_data[ilm->second] += presets_ttl;
    }
    for (map<string, string>::iterator i = preset_data.begin(); i != preset_data.end(); i++)
    {
        FILE *f = fopen((path_prefix + "presets-" + i->first + ".ttl").c_str(), "w");
        fprintf(f, "%s\n", i->second.c_str());
        fclose(f);
    }

    for (unsigned int i = 0; i < plugins.size(); i++)
    {
        string label = plugins[i]->get_plugin_info().label;
        ttl += string("<" + plugin_uri_prefix) 
            + string(plugins[i]->get_plugin_info().label)
            + "> a lv2:Plugin ;\n    lv2:binary <calf.so> ; rdfs:seeAlso <" + label + ".ttl> ";
        if (preset_data.count(label))
            ttl += ", <presets-" + label + ".ttl>";
        ttl += ".\n";
        
    }
    FILE *f = fopen((path_prefix+"manifest.ttl").c_str(), "w");
    fprintf(f, "%s\n", ttl.c_str());
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
    const plugin_registry::plugin_vector &plugins = plugin_registry::instance().get_all();
    path_prefix += "/gui-";
    for (unsigned int i = 0; i < plugins.size(); i++)
    {
        const plugin_metadata_iface *pi = plugins[i];
        
        stringstream xml;
        int graphs = 0;
        for (int j = 0; j < pi->get_param_count(); j++)
        {
            const parameter_properties &props = *pi->get_param_props(j);
            if (props.flags & PF_PROP_GRAPH)
                graphs++;
        }
        xml << "<table rows=\"" << pi->get_param_count() << "\" cols=\"" << (graphs ? "4" : "3") << "\">\n";
        for (int j = 0; j < pi->get_param_count(); j++)
        {
            if (j)
                xml << "\n    <!-- -->\n\n";
            const parameter_properties &props = *pi->get_param_props(j);
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
                const parameter_properties &props = *pi->get_param_props(j);
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
        fprintf(stderr, "calfmakerdf: Mode '%s' unsupported in this version\n", mode.c_str());
        return 1;
    }
    return 0;
}
