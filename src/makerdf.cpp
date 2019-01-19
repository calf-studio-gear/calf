/* Calf DSP Library
 * RDF file generator for LV2 plugins.
 * Copyright (C) 2007-2011 Krzysztof Foltman and others.
 * See AUTHORS file for a complete list.
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
#include "lv2/lv2plug.in/ns/lv2core/lv2.h"
#include <calf/lv2_atom.h>
#include <calf/lv2_options.h>
#include <calf/lv2_state.h>
#include <calf/lv2_urid.h>
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
    {"mode", 1, 0, 'm'},
    {"path", 1, 0, 'p'},
#if USE_LV2
    {"data-dir", 1, 0, 'd'},
#endif
    {0,0,0,0},
};

static FILE *open_and_check(const std::string &filename)
{
    FILE *f = fopen(filename.c_str(), "w");
    if (!f)
    {
        fprintf(stderr, "Cannot write file '%s': %s\n", filename.c_str(), strerror(errno));
        exit(1);
    }
    return f;
}

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
    if (!strcmp(type, "atom:AtomPort")) {
        ss << ind << "atom:bufferType atom:Sequence ;\n"
           << ind << "atom:supports lv2midi:MidiEvent ;\n"
           << ind << "atom:supports atom:Property ;\n"<< endl;
    }
    if (!strcmp(std::string(symbol, 0, 4).c_str(), "in_l")) 
        ss << ind << "lv2:designation pg:left ;\n"
           << ind << "pg:group :in ;" << endl;
    else
    if (!strcmp(std::string(symbol, 0, 4).c_str(), "in_r")) 
        ss << ind << "lv2:designation pg:right ;\n"
           << ind << "pg:group :in ;" << endl;
    else
    if (!strcmp(std::string(symbol, 0, 5).c_str(), "out_l")) 
        ss << ind << "lv2:designation pg:left ;\n"
           << ind << "pg:group :out ;" << endl;
    else
    if (!strcmp(std::string(symbol, 0, 5).c_str(), "out_r")) 
        ss << ind << "lv2:designation pg:right ;\n"
           << ind << "pg:group :out ;" << endl;
    ss << "    ]";
    ports += ss.str();
}

static const char *units[] = { 
    "ue:db", 
    "ue:coef",
    "ue:hz",
    "ue:s",
    "ue:ms",
    "ue:cent",
    "ue:semitone12TET",
    "ue:bpm",
    "ue:degree",
    "ue:midiNote",
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
        ss << ind << "lv2:designation param:gain ;\n";
    if (type == PF_BOOL)
        ss << ind << "lv2:portProperty lv2:toggled ;\n";
    else if (type == PF_ENUM)
    {
        ss << ind << "lv2:portProperty lv2:integer ;\n";
        ss << ind << "lv2:portProperty lv2:enumeration ;\n";
        for (int i = (int)pp.min; i <= (int)pp.max; i++)
            ss << ind << "lv2:scalePoint [ rdfs:label \"" << pp.choices[i - (int)pp.min] << "\"; rdf:value " << i <<" ] ;\n";
    }
    else if (type == PF_INT || type == PF_ENUM_MULTI)
        ss << ind << "lv2:portProperty lv2:integer ;\n";
    else if ((pp.flags & PF_SCALEMASK) == PF_SCALE_LOG)
        ss << ind << "lv2:portProperty epp:logarithmic ;\n";
    ss << showpoint;
    if (!(pp.flags & PF_PROP_OUTPUT))
    {
        if (pp.def_value == (int)pp.def_value)
            ss << ind << "lv2:default " << (int)pp.def_value << " ;\n";
        else
            ss << ind << "lv2:default " << pp.def_value << " ;\n";
    }
    if (pp.min == (int)pp.min)
        ss << ind << "lv2:minimum " << (int)pp.min << " ;\n";
    else
        ss << ind << "lv2:minimum " << pp.min << " ;\n";
    if (pp.max == (int)pp.max)
        ss << ind << "lv2:maximum " << (int)pp.max << " ;\n";
    else
        ss << ind << "lv2:maximum " << pp.max << " ;\n";
    // XXX This value does not seem to match the definition of the property
    //if (pp.step > 1)
    //    ss << ind << "epp:rangeSteps " << pp.step << " ;\n";
    if (((pp.flags & PF_SCALEMASK) != PF_SCALE_GAIN) && unit > 0 && unit < (sizeof(units) / sizeof(char *)) && units[unit - 1] != NULL)
        ss << ind << "ue:unit " << units[unit - 1] << " ;\n";
    
    // for now I assume that the only tempo passed is the tempo the plugin should operate with
    // this may change as more complex plugins are added
    
    if ((pp.flags & PF_SYNC_BPM))
        ss << ind << "lv2:designation <http://lv2plug.in/ns/ext/time#beatsPerMinute> ;\n";
    
    ss << "    ]";
    ports += ss.str();
    return true;
}

void make_ttl(string path_prefix, const string *data_dir)
{
    if (path_prefix.empty())
    {
        fprintf(stderr, "Path parameter is required for TTL mode\n");
        exit(1);
    }
    
    string plugin_uri_prefix = "http://calf.sourceforge.net/plugins/";
    
    string header;
    
    header = 
        "@prefix rdf:  <http://www.w3.org/1999/02/22-rdf-syntax-ns#> .\n"
        "@prefix lv2:  <http://lv2plug.in/ns/lv2core#> .\n"
        "@prefix rdfs: <http://www.w3.org/2000/01/rdf-schema#> .\n"
        "@prefix dct: <http://purl.org/dc/terms/> .\n"
        "@prefix doap: <http://usefulinc.com/ns/doap#> .\n"
        "@prefix uiext: <http://lv2plug.in/ns/extensions/ui#> .\n"
        "@prefix atom: <http://lv2plug.in/ns/ext/atom#> .\n"
        "@prefix lv2midi: <http://lv2plug.in/ns/ext/midi#> .\n"
        "@prefix lv2ctx: <http://lv2plug.in/ns/dev/contexts#> .\n"
        "@prefix strport: <http://lv2plug.in/ns/dev/string-port#> .\n"
        "@prefix pg: <http://lv2plug.in/ns/ext/port-groups#> .\n"
        "@prefix ue: <http://lv2plug.in/ns/extensions/units#> .\n"
        "@prefix epp: <http://lv2plug.in/ns/ext/port-props#> .\n"
        "@prefix foaf: <http://xmlns.com/foaf/0.1/> .\n"
        "@prefix param: <http://lv2plug.in/ns/ext/parameters#> .\n"
        "\n"
        "<http://calf.sourceforge.net/team>\n"
        "    a foaf:Person ;\n"
        "    foaf:name \"Calf Studio Gear\" ;\n"
        "    foaf:mbox <mailto:info@calf-studio-gear.org> ;\n"
        "    foaf:homepage <http://calf-studio-gear.org/> .\n"
        "\n"
    ;
    
    printf("Retrieving plugin list\n");
    fflush(stdout);
    const plugin_registry::plugin_vector &plugins = plugin_registry::instance().get_all();
    
    map<string, string> classes;
    
    printf("Creating a list of plugin classes\n");
    fflush(stdout);
    const char *ptypes[] = {
        "Flanger", "Reverb", "Generator", "Instrument", "Oscillator",
        "Utility", "Converter", "Analyser", "Mixer", "Simulator",
        "Delay", "Modulator", "Phaser", "Chorus", "Filter",
        "Lowpass", "Highpass", "Bandpass", "Comb", "Allpass",
        "Amplifier", "Distortion", "Waveshaper", "Dynamics", "Compressor",
        "Expander", "Limiter", "Gate", "EQ", "ParaEQ", "MultiEQ", "Spatial",
        "Spectral", "Pitch", "Envelope", "Function", "Constant",
        NULL
    };
    
    for(const char **p = ptypes; *p; p++) {
        string name = string(*p) + "Plugin";
        classes[name] = "lv2:" + name;
    }
    classes["SynthesizerPlugin"] = "lv2:InstrumentPlugin";

    string gui_header;
    
#if USE_LV2_GUI
    string gtkgui_uri     = "<" + plugin_uri_prefix + "gui/gtk2-gui>";
    string gtkgui_uri_req = "<" + plugin_uri_prefix + "gui/gtk2-gui-req>";
    gui_header = gtkgui_uri + "\n"
        "    a uiext:GtkUI ;\n"
        "    lv2:extensionData uiext:idleInterface ,\n"
        "        uiext:showInterface ;\n"
        "    lv2:requiredFeature uiext:makeResident ;\n"
        "    lv2:optionalFeature <http://lv2plug.in/ns/ext/data-access> ;\n"
        //"    lv2:optionalFeature <" LV2_OPTIONS_URI "> ;\n"
        //"    lv2:optionalFeature <" LV2_ATOM_URI "> ;\n"
        "    uiext:binary <calflv2gui.so> .\n"
        "\n" + gtkgui_uri_req + "\n"
        "    a uiext:GtkUI ;\n"
        "    lv2:extensionData uiext:idleInterface ,\n"
        "        uiext:showInterface ;\n"
        "    lv2:requiredFeature uiext:makeResident ;\n"
        "    lv2:requiredFeature <http://lv2plug.in/ns/ext/instance-access> ;\n"
        "    lv2:requiredFeature <http://lv2plug.in/ns/ext/data-access> ;\n"
        //"    lv2:optionalFeature <" LV2_OPTIONS_URI "> ;\n"
        //"    lv2:optionalFeature <" LV2_ATOM_URI "> ;\n"
        "    uiext:binary <calflv2gui.so> .\n"
    ;
#endif
    
    map<string, pair<string, string> > id_to_info;
    
    for (unsigned int i = 0; i < plugins.size(); i++) {
        const plugin_metadata_iface *pi = plugins[i];
        const ladspa_plugin_info &lpi = pi->get_plugin_info();
        printf("Generating a .ttl file for plugin '%s'\n", lpi.label);
        fflush(stdout);
        string unquoted_uri = plugin_uri_prefix + string(lpi.label);
        string uri = string("<" + unquoted_uri + ">");
        id_to_info[pi->get_id()] = make_pair(lpi.label, uri);
        string ttl;
        ttl = "@prefix : <" + unquoted_uri + "#> .\n";
        ttl += header + gui_header + "\n";

#if USE_LV2_GUI
        string gui_uri = (pi->requires_instance_access()) ? gtkgui_uri_req : gtkgui_uri;

        for (int j = 0; j < pi->get_param_count(); j++)
        {
            const parameter_properties &props = *pi->get_param_props(j);
            if (props.flags & PF_PROP_OUTPUT)
            {
                string portnot = " uiext:portNotification [\n    uiext:plugin " + uri + " ;\n    uiext:portIndex " + i2s(j) + "\n] .\n\n";
                ttl += gui_uri + portnot;
            }
        }
#endif

        if(pi->get_input_count() == 1) {
            ttl += ":in a pg:MonoGroup , pg:InputGroup ;\n"
                "    lv2:symbol \"in\" ;\n"
                "    rdfs:label \"Input\" .\n\n";
        } else if(pi->get_input_count() >= 2) {
            ttl += ":in a pg:StereoGroup , pg:InputGroup ;\n"
                "    lv2:symbol \"in\" ;\n"
                "    rdfs:label \"Input\" .\n\n";
        }
        if(pi->get_output_count() == 1) {
            ttl += ":out a pg:MonoGroup , pg:OutputGroup ;\n"
                "    lv2:symbol \"out\" ;\n"
                "    rdfs:label \"Output\" .\n\n";
        }
        if(pi->get_output_count() >= 2) {
            ttl += ":out a pg:StereoGroup , pg:OutputGroup ;\n"
                "    lv2:symbol \"out\" ;\n"
                "    rdfs:label \"Output\" .\n\n";
        }
        
        ttl += uri;
        
        if (classes.count(lpi.plugin_type))
            ttl += " a lv2:Plugin, " + classes[lpi.plugin_type]+" ;\n";
        else
            ttl += " a lv2:Plugin ;\n";
            
        ttl += "    doap:name \"" + string(lpi.name) + "\" ;\n";
        ttl += "    doap:license <http://usefulinc.com/doap/licenses/lgpl> ;\n";
        ttl += "    doap:developer <http://calf.sourceforge.net/team> ;\n";
        ttl += "    doap:maintainer <http://calf.sourceforge.net/team> ;\n";

#if USE_LV2_GUI
        ttl += "    uiext:ui " + gui_uri + " ;\n";
#endif
        
        ttl += "    dct:replaces <urn:ladspa:" + i2s(lpi.unique_id) + "> ;\n";
        // XXXKF not really optional for now, to be honest
        ttl += "    lv2:optionalFeature epp:supportsStrictBounds ;\n";
        if (pi->is_rt_capable())
            ttl += "    lv2:optionalFeature lv2:hardRTCapable ;\n";
        if (pi->get_midi())
        {
            if (pi->requires_midi()) {
                ttl += "    lv2:requiredFeature <" LV2_URID_MAP_URI "> ;\n";
            }
            else {
                ttl += "    lv2:optionalFeature <" LV2_URID_MAP_URI "> ;\n";
            }
        }
        
        vector<string> configure_keys;
        pi->get_configure_vars(configure_keys);
        if (!configure_keys.empty())
        {
            ttl += "    lv2:extensionData <" LV2_STATE__interface "> ;\n";
        }

        if(pi->get_input_count() >= 1) {
            ttl += "    pg:mainInput :in ;\n";
        }
        if(pi->get_output_count() >= 2) {
            ttl += "    pg:mainOutput :out ;\n";
        }

        string ports = "";
        int pn = 0;
        const char *in_symbols[] = { "in_l", "in_r", "sidechain", "sidechain2" };
        const char *in_names[] = { "In L", "In R", "Sidechain", "Sidechain 2" };
        const char *out_symbols[] = { "out_l", "out_r", "out_l_2", "out_r_2", "out_l_3", "out_r_3", "out_l_4", "out_r_4" };
        const char *out_names[] = { "Out L", "Out R", "Out L 2", "Out R 2", "Out L 3", "Out R 3", "Out L 4", "Out R 4" };
        for (int i = 0; i < pi->get_input_count(); i++)
            if(i <= pi->get_input_count() - pi->get_inputs_optional() - 1 && !(i == 1 && pi->get_simulate_stereo_input()))
                add_port(ports, in_symbols[i], in_names[i], "Input", pn++);
            else
                add_port(ports, in_symbols[i], in_names[i], "Input", pn++, "lv2:AudioPort", true);
        for (int i = 0; i < pi->get_output_count(); i++)
            if(i <= pi->get_output_count() - pi->get_outputs_optional() - 1)
                add_port(ports, out_symbols[i], out_names[i], "Output", pn++);
            else
                add_port(ports, out_symbols[i], out_names[i], "Output", pn++, "lv2:AudioPort", true);
        for (int i = 0; i < pi->get_param_count(); i++)
            if (add_ctl_port(ports, *pi->get_param_props(i), pn, pi, i))
                pn++;
        bool needs_event_io = pi->sends_live_updates();
        if (pi->get_midi() || needs_event_io) {
            if (pi->get_midi())
                add_port(ports, "midi_in", "MIDI In", "Input", pn++, "atom:AtomPort", true);
            else
                add_port(ports, "events_in", "Events", "Input", pn++, "atom:AtomPort", true);
        }
        if (needs_event_io) {
            add_port(ports, "events_out", "Events", "Output", pn++, "atom:AtomPort", true);
        }
        if (!ports.empty())
            ttl += "    lv2:port " + ports + "\n";
        ttl += ".\n\n";
        
        FILE *f = open_and_check(path_prefix+string(lpi.label)+".ttl");
        fprintf(f, "%s\n", ttl.c_str());
        fclose(f);
    }
    // Generate a manifest
    printf("Writing presets\n");
    fflush(stdout);

    // Prefixes for the preset TTL
    string presets_ttl_head =
        "@prefix lv2:  <http://lv2plug.in/ns/lv2core#> .\n"
        "@prefix lv2p:  <http://lv2plug.in/ns/ext/presets#> .\n"
        "@prefix rdfs: <http://www.w3.org/2000/01/rdf-schema#> .\n"
        "@prefix dct: <http://purl.org/dc/terms/> .\n"
        "\n"
    ;
    
    // Prefixes for the manifest TTL
    string ttl = presets_ttl_head;
    
    calf_plugins::get_builtin_presets().load_defaults(true, data_dir);
    calf_plugins::preset_vector &factory_presets = calf_plugins::get_builtin_presets().presets;

    ttl += "\n";
    
    // We'll collect TTL for presets here, maps plugin label to TTL generated so far
    map<string, string> preset_data;

    for (unsigned int i = 0; i < factory_presets.size(); i++)
    {
        plugin_preset &pr = factory_presets[i];
        map<string, pair<string, string> >::iterator ilm = id_to_info.find(pr.plugin);
        if (ilm == id_to_info.end())
            continue;
        
        string presets_ttl;
        // if this is the first preset, add a header
        if (!preset_data.count(ilm->second.first))
            presets_ttl = presets_ttl_head;
        
        string uri = "<http://calf.sourceforge.net/factory_presets#"
            + pr.plugin + "_" + pr.get_safe_name()
            + ">";
        ttl += uri + " a lv2p:Preset ;\n"
            "    lv2:appliesTo " + ilm->second.second + " ;\n"
            "    rdfs:seeAlso <presets-" + ilm->second.first + ".ttl> .\n";
        
        presets_ttl += uri + 
            " a lv2p:Preset ;\n"
            "    rdfs:label \"" + pr.name + "\" ;\n"
            "    lv2:appliesTo " + ilm->second.second + " ;\n"
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
        
        preset_data[ilm->second.first] += presets_ttl;
    }
    for (map<string, string>::iterator i = preset_data.begin(); i != preset_data.end(); ++i)
    {
        FILE *f = open_and_check(path_prefix + "presets-" + i->first + ".ttl");
        fprintf(f, "%s\n", i->second.c_str());
        fclose(f);
    }

    printf("Generating a manifest file\n");
    fflush(stdout);
    for (unsigned int i = 0; i < plugins.size(); i++)
    {
        string label = plugins[i]->get_plugin_info().label;
        ttl += string("<" + plugin_uri_prefix) 
            + string(plugins[i]->get_plugin_info().label)
            + "> a lv2:Plugin ;\n    dct:replaces <urn:ladspa:"
            + i2s(plugins[i]->get_plugin_info().unique_id) + "> ;\n    "
            + "lv2:binary <calf.so> ; rdfs:seeAlso <" + label + ".ttl> ";
        if (preset_data.count(label))
            ttl += ", <presets-" + label + ".ttl>";
        ttl += ".\n";
        
    }
    FILE *f = open_and_check(path_prefix+"manifest.ttl");
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
        
        // check for empty item after all the params
        assert(pi->get_id());
        assert(pi->get_param_props(pi->get_param_count())->short_name == NULL);
        assert(pi->get_param_props(pi->get_param_count())->name == NULL);
        
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
            if (!props.short_name)
            {
                fprintf(stderr, "Plugin %s is missing short name for parameter %d\n", pi->get_name(), j);
                exit(1);
            }
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
        FILE *f = open_and_check(path_prefix+string(pi->get_id())+".xml");
        fprintf(f, "%s\n", xml.str().c_str());
        fclose(f);
    }
}

int main(int argc, char *argv[])
{
    string mode = "rdf";
    string path_prefix;
#if USE_LV2
    string pkglibdir_path;
#endif
    while(1) {
        int option_index;
        int c = getopt_long(argc, argv, "hvm:p:"
#if USE_LV2
        "d:"
#endif
            , long_options, &option_index);
        if (c == -1)
            break;
        switch(c) {
            case 'h':
            case '?':
                printf("LV2 TTL / XML GUI generator for Calf plugin pack\nSyntax: %s [--help] [--version] [--mode rdf|ttl|gui] [--path <path>]\n", argv[0]);
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
#if USE_LV2
            case 'd':
                pkglibdir_path = optarg;
                if (pkglibdir_path.empty())
                {
                    fprintf(stderr, "calfmakerdf: Data directory must not be empty\n");
                    exit(1);
                }
                if (pkglibdir_path[pkglibdir_path.length() - 1] != '/')
                    pkglibdir_path += '/';
                break;
#endif
            case 'p':
                path_prefix = optarg;
                if (path_prefix.empty())
                {
                    fprintf(stderr, "calfmakerdf: Path prefix must not be empty\n");
                    exit(1);
                }
                if (path_prefix[path_prefix.length() - 1] != '/')
                    path_prefix += '/';
                break;
        }
    }
#if USE_LV2
    if (mode == "ttl")
        make_ttl(path_prefix, !pkglibdir_path.empty() ? &pkglibdir_path : NULL);
    else
#endif
    if (mode == "gui")
        make_gui(path_prefix);
    else
    {
        fprintf(stderr, "calfmakerdf: Mode '%s' unsupported in this version\n", mode.c_str());
        return 1;
    }
    return 0;
}
