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

using namespace std;
using namespace synth;

static struct option long_options[] = {
    {"help", 0, 0, 'h'},
    {"version", 0, 0, 'v'},
    {"mode", 0, 0, 'm'},
    {0,0,0,0},
};

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

static void add_port(string &ports, const char *symbol, const char *name, const char *direction, int pidx)
{
    stringstream ss;
    const char *ind = "        ";

    
    if (ports != "") ports += " , ";
    ss << "[\n";
    if (direction) ss << ind << "a lv2:" << direction << "Port ;\n";
    ss << ind << "a lv2:AudioPort ;\n";
    ss << ind << "lv2:index " << pidx << " ;\n";
    ss << ind << "lv2:symbol \"" << symbol << "\" ;\n";
    ss << ind << "lv2:name \"" << name << "\" ;\n";
    ss << "    ]";
    ports += ss.str();
}

static void add_ctl_port(string &ports, parameter_properties &pp, int pidx)
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
    ss << "    ]";
    ports += ss.str();
}

void make_ttl()
{
    string ttl;
    
    ttl = 
        "@prefix lv2:  <http://lv2plug.in/ns/lv2core#> .\n"
        "@prefix rdfs: <http://www.w3.org/2000/01/rdf-schema#> .\n"
        "@prefix doap: <http://usefulinc.com/ns/doap#> .\n"
        "@prefix guiext: <http://ll-plugins.nongnu.org/lv2/ext/gui#> .\n"
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
        
#if USE_LV2_GUI
    ttl += "<http://calf.sourceforge.net/plugins/gui/gtk2-gui>\n    a guiext:GtkGUI ;\n    guiext:binary <calflv2gui.so> .\n\n";
#endif
    
    for (unsigned int i = 0; i < plugins.size(); i++) {
        synth::giface_plugin_info &pi = plugins[i];
        ttl += string("<http://calf.sourceforge.net/plugins/") 
            + string(pi.info->label)
            + "> a lv2:Plugin ;\n";
        
        if (classes.count(pi.info->plugin_type))
            ttl += "    a " + classes[pi.info->plugin_type]+" ;\n";
        
            
        ttl += "    doap:name \""+string(pi.info->name)+"\" ;\n";

#if USE_LV2_GUI
        ttl += "    guiext:gui <http://calf.sourceforge.net/plugins/gui/gtk2-gui> ;\n";
#endif
        
#if USE_PHAT
        ttl += "    doap:license <http://usefulinc.com/doap/licenses/gpl> ;\n";
#else
        ttl += "    doap:license <http://usefulinc.com/doap/licenses/lgpl> ;\n";
#endif
        if (pi.rt_capable)
            ttl += "    lv2:optionalFeature lv2:hardRtCapable ;\n";
        
        string ports = "";
        int pn = 0;
        const char *in_names[] = { "in_l", "in_r" };
        const char *out_names[] = { "out_l", "out_r" };
        for (int i = 0; i < pi.inputs; i++)
            add_port(ports, in_names[i], in_names[i], "Input", pn++);
        for (int i = 0; i < pi.outputs; i++)
            add_port(ports, out_names[i], out_names[i], "Output", pn++);
        for (int i = 0; i < pi.params; i++)
            add_ctl_port(ports, pi.param_props[i], pn++);
        if (!ports.empty())
            ttl += "    lv2:port " + ports + "\n";
        ttl += ".\n\n";
    }
    printf("%s\n", ttl.c_str());
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
            + "> a lv2:Plugin ; lv2:binary <calf.so> ; rdfs:seeAlso <calf.ttl> .\n";

    printf("%s\n", ttl.c_str());
}

int main(int argc, char *argv[])
{
    string mode = "rdf";
    while(1) {
        int option_index;
        int c = getopt_long(argc, argv, "hvm:", long_options, &option_index);
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
        }
    }
    if (mode == "rdf")
        make_rdf();
    if (mode == "ttl")
        make_ttl();
    if (mode == "manifest")
        make_manifest();
    return 0;
}
