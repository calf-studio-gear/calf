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
 
#include <calf/gui.h>
#include <calf/preset.h>
#include <calf/preset_gui.h>

using namespace calf_plugins;
using namespace std;

// this way of filling presets menu is not that great

struct activate_preset_params
{
    plugin_gui *gui;
    int preset;
    bool builtin;
    
    activate_preset_params(plugin_gui *_gui, int _preset, bool _builtin)
    : gui(_gui), preset(_preset), builtin(_builtin)
    {
    }
};

gui_preset_access::gui_preset_access(plugin_gui *_gui)
{
    gui = _gui;
    store_preset_dlg = NULL;
}

void gui_preset_access::store_preset()
{
    if (store_preset_dlg)
    {
        gtk_window_present(GTK_WINDOW(store_preset_dlg));
        return;
    }
    GtkBuilder *store_preset_builder = gtk_builder_new();
    const gchar *objects[] = { "store_preset", NULL };
    GError *error = NULL;
    if (!gtk_builder_add_objects_from_file(store_preset_builder, PKGLIBDIR "/calf-gui.xml", (gchar **)objects, &error))
    {
        g_warning("Cannot load preset GUI dialog: %s", error->message);
        g_error_free(error);
        g_object_unref(G_OBJECT(store_preset_builder));
        return;
    }
    store_preset_dlg = GTK_WIDGET(gtk_builder_get_object(store_preset_builder, "store_preset"));
    gtk_signal_connect(GTK_OBJECT(store_preset_dlg), "destroy", G_CALLBACK(on_dlg_destroy_window), (gui_preset_access *)this);
//    gtk_widget_set_name(GTK_WIDGET(store_preset_dlg), "Calf-Preset");
    GtkWidget *preset_name_combo = GTK_WIDGET(gtk_builder_get_object(store_preset_builder, "preset_name"));
    GtkTreeModel *model = GTK_TREE_MODEL(gtk_list_store_new(1, G_TYPE_STRING));
    gtk_combo_box_set_model(GTK_COMBO_BOX(preset_name_combo), model);
    gtk_combo_box_entry_set_text_column(GTK_COMBO_BOX_ENTRY(preset_name_combo), 0);
    for(preset_vector::const_iterator i = get_user_presets().presets.begin(); i != get_user_presets().presets.end(); i++)
    {
        if (i->plugin != gui->effect_name)
            continue;
        gtk_combo_box_append_text(GTK_COMBO_BOX(preset_name_combo), i->name.c_str());
    }
    int response = gtk_dialog_run(GTK_DIALOG(store_preset_dlg));

    plugin_preset sp;
    sp.name = gtk_combo_box_get_active_text(GTK_COMBO_BOX(preset_name_combo));
    sp.bank = 0;
    sp.program = 0;
    sp.plugin = gui->effect_name;

    gtk_widget_destroy(store_preset_dlg);
    if (response == GTK_RESPONSE_OK)
    {
        sp.get_from(gui->plugin);
        preset_list tmp;
        try {
            tmp.load(tmp.get_preset_filename(false).c_str(), false);
        }
        catch(...)
        {
            tmp = get_user_presets();
        }
        bool found = false;
        for(preset_vector::const_iterator i = tmp.presets.begin(); i != tmp.presets.end(); i++)
        {
            if (i->plugin == gui->effect_name && i->name == sp.name)
            {
                found = true;
                break;
            }
        }
        if (found)
        {
            GtkWidget *dialog = gtk_message_dialog_new(gui->window->toplevel, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_QUESTION, GTK_BUTTONS_OK_CANCEL, 
                "Preset '%s' already exists. Overwrite?", sp.name.c_str());
            int response = gtk_dialog_run(GTK_DIALOG(dialog));
            gtk_widget_destroy(dialog);
            if (response != GTK_RESPONSE_OK)
                return;
        }
        tmp.add(sp);
        get_user_presets() = tmp;
        get_user_presets().save(tmp.get_preset_filename(false).c_str());
        if (gui->window->main)
            gui->window->main->refresh_all_presets(false);
    }
    g_object_unref(G_OBJECT(store_preset_builder));
}

void gui_preset_access::activate_preset(int preset, bool builtin)
{
    plugin_preset &p = (builtin ? get_builtin_presets() : get_user_presets()).presets[preset];
    if (p.plugin != gui->effect_name)
        return;
    if (!gui->plugin->activate_preset(p.bank, p.program))
        p.activate(gui->plugin);
    gui->refresh();
}

void gui_preset_access::on_dlg_destroy_window(GtkWindow *window, gpointer data)
{
    ((gui_preset_access *)data)->store_preset_dlg = NULL;
}

