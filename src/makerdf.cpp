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
#if USE_LV2
#include <calf/lv2_event.h>
#endif

using namespace std;
using namespace synth;

static struct option long_options[] = {
    {"help", 0, 0, 'h'},
    {"version", 0, 0, 'v'},
    {"mode", 0, 0, 'm'},
    {0,0,0,0},
};

#if USE_LADSPA
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

    rdf += synth::get_builtin_modules_rdf();
    
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
        ss << ind << "lv2ev:supportsEvent lv2midi:midiEvent ;\n";
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
    "ue:midiNote" // question to SWH: the extension says midiNode, but that must be a typo
};

static void add_ctl_port(string &ports, parameter_properties &pp, int pidx, giface_plugin_info *gpi, int param)
{
    stringstream ss;
    const char *ind = "        ";

    
    if (ports != "") ports += " , ";
    ss << "[\n";
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
    if (unit > 0 && unit < (sizeof(units) / sizeof(char *)))
        ss << ind << "ue:unit " << units[unit - 1] << " ;\n";
    
    // for now I assume that the only tempo passed is the tempo the plugin should operate with
    // this may change as more complex plugins are added
    if (unit == PF_UNIT_BPM)
        ss << ind << "lv2:portProperty epp:reportsBpm ;\n";
    
    ss << "    ]";
    ports += ss.str();
}

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
        "@prefix pg: <http://ll-plugins.nongnu.org/lv2/ext/portgroups#> .\n"
        "@prefix ue: <http://lv2plug.in/ns/extensions/units#> .\n"
        "@prefix epp: <http://lv2plug.in/ns/dev/extportinfo#> .\n"

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
        
#if USE_LV2_GUI
    header += "<http://calf.sourceforge.net/plugins/gui/gtk2-gui>\n"
        "    a uiext:GtkUI ;\n"
        "    uiext:binary <calflv2gui.so> ;\n"
        "    uiext:requiredFeature uiext:makeResident .\n"
        "    \n";
#endif
    
    for (unsigned int i = 0; i < plugins.size(); i++) {
        synth::giface_plugin_info &pi = plugins[i];
        string uri = string("<http://calf.sourceforge.net/plugins/")  + string(pi.info->label) + ">";
        string ttl;
        ttl = "@prefix : " + uri + " .\n" + header;
        
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
            ttl += "    lv2:optionalFeature <" LV2_EVENT_URI "> ;\n";
            // ttl += "    lv2:requiredFeature <" LV2_EVENT_URI "> ;\n";
        
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
}

void make_manifest()
{
    string ttl;
    
    ttl = 
        "@prefix lv2:  <http://lv2plug.in/ns/lv2core#> .\n"
        "@prefix rdfs: <http://www.w3.org/2000/01/rdf-schema#> .\n\n";
    
    vector<synth::giface_plugin_info> plugins;
    synth::get_all_plugins(plugins);
    for (unsigned int i = 0; i < plugins.size(); i++)
        ttl += string("<http://calf.sourceforge.net/plugins/") 
            + string(plugins[i].info->label)
            + "> a lv2:Plugin ; lv2:binary <calf.so> ; rdfs:seeAlso <" + string(plugins[i].info->label) + ".ttl> .\n";

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
