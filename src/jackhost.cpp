/* Calf DSP Library Utility Application - calfjackhost
 * Standalone application module wrapper example.
 * Copyright (C) 2007 Krzysztof Foltman
 *
 * Note: This module uses phat graphics library, so it's 
 * licensed under GPL license, not LGPL.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */
#include <getopt.h>
#include <stdint.h>
#include <stdlib.h>
#include <config.h>
#include <fcntl.h>
#include <expat.h>
#include <glade/glade.h>
#include <jack/jack.h>
#include <calf/giface.h>
#include <calf/jackhost.h>
#include <calf/modules.h>
#include <calf/modules_dev.h>
#include <calf/gui.h>
#include <calf/preset.h>

using namespace synth;
using namespace std;

// I don't need anyone to tell me this is stupid. I already know that :)
GtkWidget *toplevel;
plugin_gui *gui;
GtkUIManager *ui_mgr;

const char *effect_name = "flanger";
const char *client_name = "calfhost";
const char *input_name = "input";
const char *output_name = "output";
const char *midi_name = "midi";

void destroy(GtkWindow *window, gpointer data)
{
    gtk_main_quit();
}

static const char *ui_xml = 
"<ui>\n"
"  <menubar>\n"
"    <menu action=\"HostMenuAction\">\n"
"      <menuitem action=\"exit\"/>\n"
"    </menu>\n"
"    <menu action=\"PresetMenuAction\">\n"
"      <menuitem action=\"store-preset\"/>\n"
"      <separator/>\n"
"      <placeholder name=\"presets\"/>\n"
"    </menu>\n"
"  </menubar>\n"
"</ui>\n"
;

static const char *preset_pre_xml = 
"<ui>\n"
"  <menubar>\n"
"    <menu action=\"PresetMenuAction\">\n"
"      <placeholder name=\"presets\">\n";

static const char *preset_post_xml = 
"      </placeholder>\n"
"    </menu>\n"
"  </menubar>\n"
"</ui>\n"
;

void exit_gui()
{
    gtk_widget_destroy(toplevel);
}

vector<plugin_preset> presets;

string get_preset_filename()
{
    const char *home = getenv("HOME");
    return string(home)+"/.calfpresets";
}

enum PresetParserState
{
    START,
    LIST,
    PRESET,
    VALUE,
} xml_parser_state;

plugin_preset parser_preset;

void xml_start_element_handler(void *user_data, const char *name, const char *attrs[])
{
    switch(xml_parser_state)
    {
    case START:
        if (!strcmp(name, "presets")) {
            xml_parser_state = LIST;
            return;
        }
        break;
    case LIST:
        if (!strcmp(name, "preset")) {
            parser_preset.bank = parser_preset.program = 0;
            parser_preset.name = "";
            parser_preset.plugin = "";
            parser_preset.blob = "";
            parser_preset.param_names.clear();
            parser_preset.values.clear();
            for(; *attrs; attrs += 2) {
                if (!strcmp(attrs[0], "bank")) parser_preset.bank = atoi(attrs[1]);
                else
                if (!strcmp(attrs[0], "program")) parser_preset.program = atoi(attrs[1]);
                else
                if (!strcmp(attrs[0], "name")) parser_preset.name = attrs[1];
                else
                if (!strcmp(attrs[0], "plugin")) parser_preset.plugin = attrs[1];
            }
            xml_parser_state = PRESET;
            return;
        }
        break;
    case PRESET:
        if (!strcmp(name, "param")) {
            std::string name;
            float value = 0.f;
            for(; *attrs; attrs += 2) {
                if (!strcmp(attrs[0], "name")) name = attrs[1];
                else
                if (!strcmp(attrs[0], "value")) {
                    istringstream str(attrs[1]);
                    str >> value;
                }
            }
            parser_preset.param_names.push_back(name);
            parser_preset.values.push_back(value);
            xml_parser_state = VALUE;
            return;
        }
        break;
    case VALUE:
        break;
    }
    g_warning("Invalid XML element: %s", name);
}

void xml_end_element_handler(void *user_data, const char *name)
{
    switch(xml_parser_state)
    {
    case START:
        break;
    case LIST:
        if (!strcmp(name, "presets")) {
            xml_parser_state = START;
            return;
        }
        break;
    case PRESET:
        if (!strcmp(name, "preset")) {
            presets.push_back(parser_preset);
            xml_parser_state = LIST;
            return;
        }
        break;
    case VALUE:
        if (!strcmp(name, "param")) {
            xml_parser_state = PRESET;
            return;
        }
        break;
    }
    g_warning("Invalid XML element close: %s", name);
}

void load_presets()
{
    xml_parser_state = START;
    XML_Parser parser = XML_ParserCreate("UTF-8");
    string filename = get_preset_filename();
    int fd = open(filename.c_str(), O_RDONLY);
    if (fd < 0) {
        g_warning("Could not save the presets in %s", filename.c_str());
        return;
    }
    XML_SetElementHandler(parser, xml_start_element_handler, xml_end_element_handler);
    char buf[4096];
    do
    {
        int len = read(fd, buf, 4096);
        // XXXKF not an optimal error/EOF handling :)
        if (len <= 0)
            break;
        XML_Parse(parser, buf, len, 0);
    } while(1);
    XML_Parse(parser, buf, 0, 1);
    close(fd);
}

void save_presets()
{
    string xml = "<presets>\n";
    for (unsigned int i = 0; i < presets.size(); i++)
    {
        xml += presets[i].to_xml();
    }
    xml += "</presets>";
    string filename = get_preset_filename();
    int fd = open(filename.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0640);
    if (fd < 0 || ((unsigned)write(fd, xml.c_str(), xml.length()) != xml.length())) {
        g_warning("Could not save the presets in %s", filename.c_str());
        return;
    }
    close(fd);
}

GladeXML *store_preset_xml = NULL;
GtkWidget *store_preset_dlg = NULL;

void store_preset_dlg_destroy(GtkWindow *window, gpointer data)
{
    store_preset_dlg = NULL;
}

void store_preset_ok()
{
    plugin_preset sp;
    sp.name = gtk_entry_get_text(GTK_ENTRY(glade_xml_get_widget(store_preset_xml, "preset_name")));
    sp.bank = 0;
    sp.program = 0;
    sp.plugin = effect_name;
    int count = gui->plugin->get_param_count();
    for (int i = 0; i < count; i++) {
        sp.param_names.push_back(gui->plugin->get_param_names()[i]);
        sp.values.push_back(gui->plugin->get_param_value(i));
    }
    presets.push_back(sp);
    gtk_widget_destroy(store_preset_dlg);
}

void store_preset_cancel()
{
    gtk_widget_destroy(store_preset_dlg);
}

void store_preset()
{
    if (store_preset_dlg)
    {
        gtk_window_present(GTK_WINDOW(store_preset_dlg));
        return;
    }
    store_preset_xml = glade_xml_new("calf.glade", NULL, NULL);
    store_preset_dlg = glade_xml_get_widget(store_preset_xml, "store_preset");
    gtk_signal_connect(GTK_OBJECT(store_preset_dlg), "destroy", G_CALLBACK(store_preset_dlg_destroy), NULL);
    gtk_signal_connect(GTK_OBJECT(glade_xml_get_widget(store_preset_xml, "ok_button")), "clicked", G_CALLBACK(store_preset_ok), NULL);
    gtk_signal_connect(GTK_OBJECT(glade_xml_get_widget(store_preset_xml, "cancel_button")), "clicked", G_CALLBACK(store_preset_cancel), NULL);
    gtk_window_set_transient_for(GTK_WINDOW(store_preset_dlg), GTK_WINDOW(toplevel));
}

static const GtkActionEntry entries[] = {
    { "HostMenuAction", "", "_Host", NULL, "Application-wide actions", NULL },
    { "exit", "", "E_xit", NULL, "Exit the application", exit_gui },
    { "PresetMenuAction", "", "_Preset", NULL, "Preset operations", NULL },
    { "store-preset", "", "_Store preset", NULL, "Store a current setting as preset", store_preset },
};

void activate_preset(GtkAction *action, int preset)
{
    plugin_preset &p = presets[preset];
    if (p.plugin != effect_name)
        return;
    map<string, int> names;
    int count = gui->plugin->get_param_count();
    for (int i = 0; i < count; i++) 
        names[gui->plugin->get_param_names()[i]] = i;
    // no support for unnamed parameters... tough luck :)
    for (unsigned int i = 0; i < min(p.param_names.size(), p.values.size()); i++)
    {
        map<string, int>::iterator pos = names.find(p.param_names[i]);
        if (pos == names.end())
            continue;
        gui->plugin->set_param_value(pos->second, p.values[i]);
    }
    gui->refresh();
}

void make_gui(jack_host_base *jh, const char *title, const char *effect)
{
    toplevel = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    GtkVBox *vbox = GTK_VBOX(gtk_vbox_new(false, 5));
    
    gtk_window_set_title(GTK_WINDOW (toplevel), (string(title) + " - " + effect).c_str());
    gtk_container_add(GTK_CONTAINER(toplevel), GTK_WIDGET(vbox));

    GtkUIManager *ui_mgr = gtk_ui_manager_new();
    GtkActionGroup *act_grp = gtk_action_group_new("default");
    gtk_action_group_add_actions(act_grp, entries, sizeof(entries)/sizeof(entries[0]), jh);
    GError *error = NULL;
    gtk_ui_manager_insert_action_group(ui_mgr, act_grp, 0);
    int merge_ui = gtk_ui_manager_add_ui_from_string(ui_mgr, ui_xml, -1, &error);
    
    string preset_xml = preset_pre_xml;
    for (unsigned int i = 0; i < presets.size(); i++)
    {
        if (presets[i].plugin != effect_name)
            continue;
        stringstream ss;
        ss << "preset" << i;
        preset_xml += "          <menuitem name=\""+presets[i].name+"\" action=\""+ss.str()+"\"/>\n";
        
        GtkActionEntry ae = { ss.str().c_str(), NULL, presets[i].name.c_str(), NULL, NULL, (GCallback)activate_preset };
        gtk_action_group_add_actions(act_grp, &ae, 1, (gpointer)i);
        
    }
    preset_xml += preset_post_xml;
    
    gtk_ui_manager_add_ui_from_string(ui_mgr, preset_xml.c_str(), -1, &error);
    GtkWidget *menu_bar = gtk_ui_manager_get_widget(ui_mgr, "/ui/menubar");
    
    
    gtk_container_add(GTK_CONTAINER(vbox), GTK_WIDGET(menu_bar));
    
    gui = new plugin_gui(toplevel);
    GtkWidget *table = gui->create(jh, effect);

    gtk_container_add(GTK_CONTAINER(vbox), table);

    gtk_signal_connect(GTK_OBJECT(toplevel), "destroy", G_CALLBACK(destroy), NULL);
    gtk_widget_show_all(toplevel);
    
}

static struct option long_options[] = {
    {"help", 0, 0, 'h'},
    {"version", 0, 0, 'v'},
    {"client", 1, 0, 'c'},
    {"effect", 1, 0, 'e'},
    {"plugin", 1, 0, 'p'},
    {"input", 1, 0, 'i'},
    {"output", 1, 0, 'o'},
    {0,0,0,0},
};

int main(int argc, char *argv[])
{
    gtk_init(&argc, &argv);
    glade_init();
    while(1) {
        int option_index;
        int c = getopt_long(argc, argv, "c:e:i:o:m:p:hv", long_options, &option_index);
        if (c == -1)
            break;
        switch(c) {
            case 'h':
            case '?':
                printf("JACK host for Calf effects\n"
                    "Syntax: %s [--plugin reverb|flanger|filter|monosynth] [--client <name>] [--input <name>]"
                    "       [--output <name>] [--midi <name>] [--help] [--version]\n", 
                    argv[0]);
                return 0;
            case 'v':
                printf("%s\n", PACKAGE_STRING);
                return 0;
            case 'e':
            case 'p':
                effect_name = optarg;
                break;
            case 'c':
                client_name = optarg;
                break;
            case 'i':
                input_name = optarg;
                break;
            case 'o':
                output_name = optarg;
                break;
            case 'm':
                midi_name = optarg;
                break;
        }
    }
    try {
        load_presets();
        jack_host_base *jh = NULL;
        if (!strcmp(effect_name, "reverb"))
            jh = new jack_host<reverb_audio_module>();
        else if (!strcmp(effect_name, "flanger"))
            jh = new jack_host<flanger_audio_module>();
        else if (!strcmp(effect_name, "filter"))
            jh = new jack_host<filter_audio_module>();
        else if (!strcmp(effect_name, "monosynth"))
            jh = new jack_host<monosynth_audio_module>();
#ifdef ENABLE_EXPERIMENTAL
        else if (!strcmp(effect_name, "organ"))
            jh = new jack_host<organ_audio_module>();
#endif
        else {
#ifdef ENABLE_EXPERIMENTAL
            fprintf(stderr, "Unknown plugin name; allowed are: reverb, flanger, filter, monosynth, organ\n");
#else
            fprintf(stderr, "Unknown plugin name; allowed are: reverb, flanger, filter, monosynth\n");
#endif
            return 1;
        }
        jh->open(client_name, input_name, output_name, midi_name);
        make_gui(jh, client_name, effect_name);
        gtk_main();
        delete gui;
        jh->close();
        save_presets();
        delete jh;
    }
    catch(std::exception &e)
    {
        fprintf(stderr, "%s\n", e.what());
        exit(1);
    }
    return 0;
}
