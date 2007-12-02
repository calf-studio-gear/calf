#include <getopt.h>
#include <stdint.h>
#include <stdlib.h>
#include <config.h>
#include <jack/jack.h>
#include <calf/giface.h>
#include <calf/modules.h>

using namespace std;

static struct option long_options[] = {
    {"help", 0, 0, 'h'},
    {"version", 0, 0, 'v'},
    {"client", 1, 0, 'c'},
    {"effect", 1, 0, 'e'},
    {"input", 1, 0, 'i'},
    {"output", 1, 0, 'o'},
    {0,0,0,0},
};

int main(int argc, char *argv[])
{
    while(1) {
        int option_index;
        int c = getopt_long(argc, argv, "c:e:i:o:hv", long_options, &option_index);
        if (c == -1)
            break;
        switch(c) {
            case 'h':
                printf("LADSPA RDF generator for Calf plugin pack\nSyntax: %s [--help] [--version]\n", argv[0]);
                return 0;
            case 'v':
                printf("%s\n", PACKAGE_STRING);
                return 0;
        }
    }
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
    return 0;
}
