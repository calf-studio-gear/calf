/* Calf DSP Library
 * GUI functions for preset management.
 * Copyright (C) 2007 Krzysztof Foltman
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
 
#include <map>
#include <glade/glade.h>
#include <calf/giface.h>
#include <calf/gui.h>
#include <calf/preset.h>
#include <calf/preset_gui.h>

using namespace synth;
using namespace std;

// this way of filling presets menu is not that great

GladeXML *store_preset_xml = NULL;
GtkWidget *store_preset_dlg = NULL;

void store_preset_dlg_destroy(GtkWindow *window, gpointer data)
{
    store_preset_dlg = NULL;
}

void store_preset_ok(GtkAction *action, plugin_gui *gui)
{
    plugin_preset sp;
    sp.name = gtk_entry_get_text(GTK_ENTRY(glade_xml_get_widget(store_preset_xml, "preset_name")));
    sp.bank = 0;
    sp.program = 0;
    sp.plugin = gui->effect_name;
    int count = gui->plugin->get_param_count();
    for (int i = 0; i < count; i++) {
        sp.param_names.push_back(gui->plugin->get_param_props(i)->name);
        sp.values.push_back(gui->plugin->get_param_value(i));
    }
    add_preset(sp);
    gtk_widget_destroy(store_preset_dlg);
    gui->window->fill_gui_presets();
}

void store_preset_cancel(GtkAction *action, plugin_gui *gui)
{
    gtk_widget_destroy(store_preset_dlg);
}

void synth::store_preset(GtkWindow *toplevel, plugin_gui *gui)
{
    if (store_preset_dlg)
    {
        gtk_window_present(GTK_WINDOW(store_preset_dlg));
        return;
    }
    store_preset_xml = glade_xml_new(PKGLIBDIR "/calf.glade", NULL, NULL);
    store_preset_dlg = glade_xml_get_widget(store_preset_xml, "store_preset");
    gtk_signal_connect(GTK_OBJECT(store_preset_dlg), "destroy", G_CALLBACK(store_preset_dlg_destroy), NULL);
    gtk_signal_connect(GTK_OBJECT(glade_xml_get_widget(store_preset_xml, "ok_button")), "clicked", G_CALLBACK(store_preset_ok), gui);
    gtk_signal_connect(GTK_OBJECT(glade_xml_get_widget(store_preset_xml, "cancel_button")), "clicked", G_CALLBACK(store_preset_cancel), gui);
    gtk_window_set_transient_for(GTK_WINDOW(store_preset_dlg), toplevel);
}

void synth::activate_preset(GtkAction *action, activate_preset_params *params)
{
    plugin_gui *gui = params->gui;
    plugin_preset &p = presets[params->preset];
    if (p.plugin != gui->effect_name)
        return;
    map<string, int> names;
    int count = gui->plugin->get_param_count();
    for (int i = 0; i < count; i++) 
        names[gui->plugin->get_param_props(i)->name] = i;
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

