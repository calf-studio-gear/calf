/* Calf DSP Library
 * GUI main window.
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
 
#include <assert.h>
#include <config.h>
#include <calf/giface.h>
#include <calf/gui.h>
#include <calf/preset.h>
#include <calf/preset_gui.h>
#include <calf/main_win.h>

using namespace synth;
using namespace std;

main_window::main_window()
{
    toplevel = NULL;
    is_closed = true;
}

void main_window::add_plugin(plugin_ctl_iface *plugin)
{
    if (toplevel)
    {
        plugins[plugin] = create_strip(plugin);
    }
    else {
        plugin_queue.push_back(plugin);
        plugins[plugin] = NULL;
    }
}

void main_window::del_plugin(plugin_ctl_iface *plugin)
{
    plugins.erase(plugin);
}

void main_window::set_window(plugin_ctl_iface *plugin, plugin_gui_window *gui_win)
{
    if (!plugins.count(plugin))
        return;
    plugin_strip *strip = plugins[plugin];
    if (!strip)
        return;
    strip->gui_win = gui_win;
    if (!is_closed)
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(strip->name), gui_win != NULL);
}

void main_window::refresh_all_presets()
{
    for (std::map<plugin_ctl_iface *, plugin_strip *>::iterator i = plugins.begin(); i != plugins.end(); i++)
    {
        if (i->second && i->second->gui_win)
            i->second->gui_win->fill_gui_presets();
    }
}

static gboolean
gui_button_pressed(GtkWidget *button, main_window::plugin_strip *strip)
{
    GtkToggleButton *tb = GTK_TOGGLE_BUTTON(button);
    if ((gtk_toggle_button_get_active(tb) != 0) == (strip->gui_win != NULL))
        return FALSE;
    if (strip->gui_win) {
        strip->gui_win->close();
        strip->gui_win = NULL;
    } else {
        strip->main_win->open_gui(strip->plugin);
    }
    return TRUE;
}

main_window::plugin_strip *main_window::create_strip(plugin_ctl_iface *plugin)
{
    plugin_strip *strip = new plugin_strip;
    strip->main_win = this;
    strip->plugin = plugin;
    strip->gui_win = NULL;
    
    int row = 0, cols = 0;
    g_object_get(G_OBJECT(strips_table), "n-rows", &row, "n-columns", &cols, NULL);
    gtk_table_resize(GTK_TABLE(strips_table), row + 3, cols);

    gtk_table_attach_defaults(GTK_TABLE(strips_table), gtk_hseparator_new(), 0, 4, row, row + 1);
    row++;
    
    GtkWidget *label = gtk_toggle_button_new_with_label(plugin->get_name());
    gtk_table_attach_defaults(GTK_TABLE(strips_table), label, 0, 1, row, row + 2);
    strip->name = label;
    gtk_signal_connect(GTK_OBJECT(label), "toggled", G_CALLBACK(gui_button_pressed), 
        (plugin_ctl_iface *)strip);
    
    label = gtk_label_new(plugin->get_midi() ? "MIDI" : "");
    gtk_table_attach_defaults(GTK_TABLE(strips_table), label, 1, 2, row, row + 2);
    strip->midi_in = label;

    for (int i = 0; i < 2; i++)
        strip->audio_in[i] = strip->audio_out[i] = NULL;
    
    if (plugin->get_input_count() == 2) {
        label = calf_vumeter_new();
        gtk_table_attach_defaults(GTK_TABLE(strips_table), label, 2, 3, row, row + 1);
        strip->audio_in[0] = label;
        label = calf_vumeter_new();
        gtk_table_attach_defaults(GTK_TABLE(strips_table), label, 2, 3, row + 1, row + 2);
        strip->audio_in[1] = label;
    }

    if (plugin->get_output_count() == 2) {
        label = calf_vumeter_new();
        gtk_table_attach_defaults(GTK_TABLE(strips_table), label, 3, 4, row, row + 1);
        strip->audio_out[0] = label;
        label = calf_vumeter_new();
        gtk_table_attach_defaults(GTK_TABLE(strips_table), label, 3, 4, row + 1, row + 2);
        strip->audio_out[1] = label;
    }
    return strip;
}

void main_window::update_strip(plugin_ctl_iface *plugin)
{
    // plugin_strip *strip = plugins[plugin];
    // assert(strip);
    
}

void main_window::open_gui(plugin_ctl_iface *plugin)
{
    plugin_gui_window *gui_win = new plugin_gui_window(this);
    gui_win->create(plugin, (prefix + plugin->get_name()).c_str(), plugin->get_id());
    gtk_widget_show_all(GTK_WIDGET(gui_win->toplevel));
    plugins[plugin]->gui_win = gui_win; 
}

void main_window::create()
{
    toplevel = GTK_WINDOW(gtk_window_new (GTK_WINDOW_TOPLEVEL));
    is_closed = false;
    
    all_vbox = gtk_vbox_new(0, FALSE);
    strips_table = gtk_table_new(1, 5, FALSE);
    gtk_table_set_col_spacings(GTK_TABLE(strips_table), 10);
    gtk_table_set_row_spacings(GTK_TABLE(strips_table), 5);
    
    gtk_table_attach_defaults(GTK_TABLE(strips_table), gtk_label_new("Module"), 0, 1, 0, 1);
    gtk_table_attach_defaults(GTK_TABLE(strips_table), gtk_label_new("MIDI In"), 1, 2, 0, 1);
    gtk_table_attach_defaults(GTK_TABLE(strips_table), gtk_label_new("Audio In"), 2, 3, 0, 1);
    gtk_table_attach_defaults(GTK_TABLE(strips_table), gtk_label_new("Audio Out"), 3, 4, 0, 1);
    for (std::vector<plugin_ctl_iface *>::iterator i = plugin_queue.begin(); i != plugin_queue.end(); i++)
    {
        plugins[*i] = create_strip(*i);
        update_strip(*i);        
    }

    gtk_container_add(GTK_CONTAINER(all_vbox), strips_table);
    gtk_container_add(GTK_CONTAINER(toplevel), all_vbox);
    
    gtk_widget_show_all(GTK_WIDGET(toplevel));
    source_id = g_timeout_add_full(G_PRIORITY_LOW, 1000/30, on_idle, this, NULL); // 30 fps should be enough for everybody
}

void main_window::refresh_plugin(plugin_ctl_iface *plugin)
{
    plugins[plugin]->gui_win->gui->refresh();
}

void main_window::close_guis()
{
    for (std::map<plugin_ctl_iface *, plugin_strip *>::iterator i = plugins.begin(); i != plugins.end(); i++)
    {
        if (i->second && i->second->gui_win) {
            i->second->gui_win->close();
        }
    }
    plugins.clear();
}

void main_window::on_closed()
{
    if (source_id)
        g_source_remove(source_id);
    is_closed = true;
    toplevel = NULL;
}

static inline float LVL(float value)
{
    return sqrt(value) * 0.75;
}

gboolean main_window::on_idle(void *data)
{
    main_window *self = (main_window *)data;
    for (std::map<plugin_ctl_iface *, plugin_strip *>::iterator i = self->plugins.begin(); i != self->plugins.end(); i++)
    {
        if (i->second)
        {
            plugin_ctl_iface *plugin = i->first;
            plugin_strip *strip = i->second;
            int idx = 0;
            if (plugin->get_input_count() == 2) {
                calf_vumeter_set_value(CALF_VUMETER(strip->audio_in[0]), LVL(plugin->get_level(idx++)));
                calf_vumeter_set_value(CALF_VUMETER(strip->audio_in[1]), LVL(plugin->get_level(idx++)));
            }
            if (plugin->get_output_count() == 2) {
                calf_vumeter_set_value(CALF_VUMETER(strip->audio_out[0]), LVL(plugin->get_level(idx++)));
                calf_vumeter_set_value(CALF_VUMETER(strip->audio_out[1]), LVL(plugin->get_level(idx++)));
            }
            if (plugin->get_midi()) {
                gtk_label_set_text(GTK_LABEL(strip->midi_in), (plugin->get_level(idx++) > 0.f) ? "*" : "");
            }
        }
    }
    return TRUE;
}
