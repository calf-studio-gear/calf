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

static const char *ui_xml = 
"<ui>\n"
"  <menubar>\n"
"    <menu action=\"HostMenuAction\">\n"
"      <menu action=\"AddPluginMenuAction\">\n"
"      </menu>\n"
"      <separator/>\n"
"      <menuitem action=\"exit\"/>\n"
"    </menu>\n"
"  </menubar>\n"
"</ui>\n"
;

static void exit_action(GtkWidget *widget, main_window *main)
{
    gtk_widget_destroy(GTK_WIDGET(main->toplevel));
}

static const GtkActionEntry actions[] = {
    { "HostMenuAction", "", "_Host", NULL, "Host-related operations", NULL },
    { "AddPluginMenuAction", "", "_Add plugin", NULL, "Add a plugin to the rack", NULL },
    { "exit", "", "_Exit", NULL, "Exit application", (GCallback)exit_action },
};

main_window::main_window()
{
    toplevel = NULL;
    owner = NULL;
    is_closed = true;
}

void main_window::add_plugin(plugin_ctl_iface *plugin)
{
    if (toplevel)
    {
        plugin_strip *strip = create_strip(plugin);
        plugins[plugin] = strip;
        update_strip(plugin);
    }
    else {
        plugin_queue.push_back(plugin);
        plugins[plugin] = NULL;
    }
}

void main_window::del_plugin(plugin_ctl_iface *plugin)
{
    if (!plugins.count(plugin))
        return;
    plugin_strip *strip = plugins[plugin];
    if (strip->gui_win)
        strip->gui_win->close();

    int row = -1;
    for(GList *p = GTK_TABLE(strips_table)->children; p != NULL; p = p->next)
    {
        GtkTableChild *c = (GtkTableChild *)p->data;
        if (c->widget == strip->name)
        {
            row = c->top_attach - 1;
            break;
        }
    }
    g_assert(row != -1);
    
    vector<GtkWidget *> to_destroy;
    for(GList *p = GTK_TABLE(strips_table)->children; p != NULL; p = p->next)
    {
        GtkTableChild *c = (GtkTableChild *)p->data;
        if (c->top_attach >= row && c->top_attach < row + 3)
            to_destroy.push_back(c->widget);
        if (c->top_attach >= row + 3)
        {
            c->top_attach -= 3;
            c->bottom_attach -= 3;
        }
    }
    
    for (unsigned int i = 0; i < to_destroy.size(); i++)
        gtk_container_remove(GTK_CONTAINER(strips_table), to_destroy[i]);
    //for (unsigned int i = 0; i < to_destroy.size(); i++)
    //    gtk_widget_destroy(to_destroy[i]);
    
    plugins.erase(plugin);
    int rows = 0, cols = 0;
    g_object_get(G_OBJECT(strips_table), "n-rows", &rows, "n-columns", &cols, NULL);
    gtk_table_resize(GTK_TABLE(strips_table), rows - 3, cols);
    /*
    // a hack to remove unneeded vertical space from the window
    // not perfect, as it undoes user's vertical resize
    // only needed when window is resizable though
    int width, height;
    gtk_window_get_size(toplevel, &width, &height);
    gtk_window_resize(toplevel, width, 1);
    */
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

void main_window::refresh_all_presets(bool builtin_too)
{
    for (std::map<plugin_ctl_iface *, plugin_strip *>::iterator i = plugins.begin(); i != plugins.end(); i++)
    {
        if (i->second && i->second->gui_win) {
            i->second->gui_win->fill_gui_presets(false);
            if (builtin_too)
                i->second->gui_win->fill_gui_presets(true);
        }
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

static gboolean
extra_button_pressed(GtkWidget *button, main_window::plugin_strip *strip)
{
    GtkButton *tb = GTK_BUTTON(button);
    strip->main_win->owner->remove_plugin(strip->plugin);
    return TRUE;
}

main_window::plugin_strip *main_window::create_strip(plugin_ctl_iface *plugin)
{
    plugin_strip *strip = new plugin_strip;
    strip->main_win = this;
    strip->plugin = plugin;
    strip->gui_win = NULL;
    
    GtkAttachOptions ao = (GtkAttachOptions)(GTK_EXPAND | GTK_FILL);
    
    int row = 0, cols = 0;
    g_object_get(G_OBJECT(strips_table), "n-rows", &row, "n-columns", &cols, NULL);
    gtk_table_resize(GTK_TABLE(strips_table), row + 3, cols);

    GtkWidget *sep = gtk_hseparator_new();
    gtk_table_attach(GTK_TABLE(strips_table), sep, 0, 5, row, row + 1, ao, GTK_SHRINK, 0, 0);
    gtk_widget_show(sep);
    row++;
    
    GtkWidget *label = gtk_toggle_button_new_with_label(plugin->get_label());
    gtk_table_attach(GTK_TABLE(strips_table), label, 0, 1, row, row + 2, ao, GTK_SHRINK, 0, 0);
    strip->name = label;
    gtk_signal_connect(GTK_OBJECT(label), "toggled", G_CALLBACK(gui_button_pressed), 
        (plugin_ctl_iface *)strip);
    gtk_widget_show(strip->name);
    
    label = gtk_label_new(plugin->get_midi() ? "?" : "");
    gtk_table_attach(GTK_TABLE(strips_table), label, 1, 2, row, row + 2, GTK_FILL, GTK_SHRINK, 0, 0);
    strip->midi_in = label;
    gtk_widget_show(strip->midi_in);

    for (int i = 0; i < 2; i++)
        strip->audio_in[i] = strip->audio_out[i] = NULL;
    
    if (plugin->get_input_count() == 2) {
        label = calf_vumeter_new();
        gtk_table_attach(GTK_TABLE(strips_table), label, 2, 3, row, row + 1, ao, GTK_SHRINK, 0, 0);
        strip->audio_in[0] = label;
        label = calf_vumeter_new();
        gtk_table_attach(GTK_TABLE(strips_table), label, 2, 3, row + 1, row + 2, ao, GTK_SHRINK, 0, 0);
        strip->audio_in[1] = label;
        gtk_widget_show(strip->audio_in[0]);
        gtk_widget_show(strip->audio_in[1]);
    }

    if (plugin->get_output_count() == 2) {
        label = calf_vumeter_new();
        gtk_table_attach(GTK_TABLE(strips_table), label, 3, 4, row, row + 1, ao, GTK_SHRINK, 0, 0);
        strip->audio_out[0] = label;
        label = calf_vumeter_new();
        gtk_table_attach(GTK_TABLE(strips_table), label, 3, 4, row + 1, row + 2, ao, GTK_SHRINK, 0, 0);
        strip->audio_out[1] = label;
        gtk_widget_show(strip->audio_out[0]);
        gtk_widget_show(strip->audio_out[1]);
    }
    GtkWidget *extra = gtk_button_new_with_label("Delete");
    gtk_table_attach(GTK_TABLE(strips_table), extra, 4, 5, row, row + 2, GTK_SHRINK, GTK_SHRINK, 0, 0);
    strip->extra = extra;
    gtk_signal_connect(GTK_OBJECT(extra), "clicked", G_CALLBACK(extra_button_pressed), 
        (plugin_ctl_iface *)strip);
    gtk_widget_show(strip->extra);
    
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

static const char *plugin_pre_xml = 
"<ui>\n"
"  <menubar>\n"
"    <menu action=\"AddPluginMenuAction\">\n"
"      <placeholder name=\"plugin\">\n";

static const char *plugin_post_xml = 
"      </placeholder>\n"
"    </menu>\n"
"  </menubar>\n"
"</ui>\n"
;

void main_window::add_plugin_action(GtkWidget *src, gpointer data)
{
    add_plugin_params *app = (add_plugin_params *)data;
    app->main_win->new_plugin(app->name.c_str());
}

static void action_destroy_notify(gpointer data)
{
    delete (main_window::add_plugin_params *)data;
}

/*
void main_window::new_plugin(const char *plugin)
{
    printf("new plugin %s\n", plugin);
}
*/

std::string main_window::make_plugin_list(GtkActionGroup *actions)
{
    string s = plugin_pre_xml;
    std::vector<giface_plugin_info> plugins;
    synth::get_all_plugins(plugins);
    for(unsigned int i = 0; i < plugins.size(); i++)
    {
        string action_name = "Add" + string(plugins[i].info->label)+"Action";
        s += string("<menuitem action=\"") + action_name + "\" />";
        GtkActionEntry ae = { action_name.c_str(), NULL, plugins[i].info->name, NULL, NULL, (GCallback)add_plugin_action };
        gtk_action_group_add_actions_full(actions, &ae, 1, (gpointer)new add_plugin_params(this, plugins[i].info->label), action_destroy_notify);
    }
    return s + plugin_post_xml;
}

void main_window::create()
{
    toplevel = GTK_WINDOW(gtk_window_new (GTK_WINDOW_TOPLEVEL));
    is_closed = false;
    gtk_window_set_resizable(toplevel, false);
    
    all_vbox = gtk_vbox_new(0, FALSE);

    ui_mgr = gtk_ui_manager_new();
    std_actions = gtk_action_group_new("default");
    gtk_action_group_add_actions(std_actions, actions, sizeof(actions)/sizeof(actions[0]), this);
    GError *error = NULL;
    gtk_ui_manager_insert_action_group(ui_mgr, std_actions, 0);
    gtk_ui_manager_add_ui_from_string(ui_mgr, ui_xml, -1, &error);    
    gtk_box_pack_start(GTK_BOX(all_vbox), gtk_ui_manager_get_widget(ui_mgr, "/ui/menubar"), false, false, 0);

    plugin_actions = gtk_action_group_new("plugins");
    string plugin_xml = make_plugin_list(plugin_actions);
    gtk_ui_manager_insert_action_group(ui_mgr, plugin_actions, 0);    
    gtk_ui_manager_add_ui_from_string(ui_mgr, plugin_xml.c_str(), -1, &error);

    
    strips_table = gtk_table_new(1, 6, FALSE);
    gtk_table_set_col_spacings(GTK_TABLE(strips_table), 10);
    gtk_table_set_row_spacings(GTK_TABLE(strips_table), 5);
    
    gtk_table_attach(GTK_TABLE(strips_table), gtk_label_new("Module"), 0, 1, 0, 1, GTK_FILL, GTK_SHRINK, 0, 0);
    gtk_table_attach(GTK_TABLE(strips_table), gtk_label_new("MIDI In"), 1, 2, 0, 1, GTK_FILL, GTK_SHRINK, 0, 0);
    gtk_table_attach(GTK_TABLE(strips_table), gtk_label_new("Audio In"), 2, 3, 0, 1, GTK_FILL, GTK_SHRINK, 0, 0);
    gtk_table_attach(GTK_TABLE(strips_table), gtk_label_new("Audio Out"), 3, 4, 0, 1, GTK_FILL, GTK_SHRINK, 0, 0);
    for(GList *p = GTK_TABLE(strips_table)->children; p != NULL; p = p->next)
    {
        GtkTableChild *c = (GtkTableChild *)p->data;
        if (c->top_attach == 0) {
            gtk_misc_set_alignment(GTK_MISC(c->widget), 0.5, 0);
        }
    }
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
    if (plugins[plugin]->gui_win)
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
