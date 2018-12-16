/* Calf DSP Library
 * GUI main window.
 * Copyright (C) 2007-2011 Krzysztof Foltman
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
 
#include <calf/gtk_main_win.h>

using namespace calf_plugins;
using namespace std;

gtk_main_window::gtk_main_window()
{
    toplevel = NULL;
    owner = NULL;
    notifier = NULL;
    is_closed = true;
    progress_window = NULL;
    images = image_factory();
}

static const char *ui_xml = 
"<ui>\n"
"  <menubar>\n"
"    <menu action=\"FileMenuAction\">\n"
"      <menuitem action=\"FileOpen\"/>\n"
"      <menuitem action=\"FileSave\"/>\n"
"      <menuitem action=\"FileSaveAs\"/>\n"
"      <separator/>\n"
"      <menuitem action=\"FileReorder\"/>\n"
"      <separator/>\n"
"      <menuitem action=\"FilePreferences\"/>\n"
"      <separator/>\n"
"      <menuitem action=\"FileQuit\"/>\n"
"    </menu>\n"
"    <menu action=\"AddPluginMenuAction\" />\n"
"  </menubar>\n"
"</ui>\n"
;

const GtkActionEntry gtk_main_window::actions[] = {
    { "FileMenuAction", NULL, "_File", NULL, "File-related operations", NULL },
    { "FileOpen", GTK_STOCK_OPEN, "_Open", "<Ctrl>O", "Open a rack file", (GCallback)on_open_action },
    { "FileSave", GTK_STOCK_SAVE, "_Save", "<Ctrl>S", "Save a rack file", (GCallback)on_save_action },
    { "FileSaveAs", GTK_STOCK_SAVE_AS, "Save _as...", NULL, "Save a rack file as", (GCallback)on_save_as_action },
    { "HostMenuAction", NULL, "_Host", NULL, "Host-related operations", NULL },
    { "AddPluginMenuAction", NULL, "_Add plugin", NULL, "Add a plugin to the rack", NULL },
    { "FileReorder", NULL, "_Reorder plugins", NULL, "Reorder plugins to minimize latency (experimental)", (GCallback)on_reorder_action },
    { "FilePreferences", GTK_STOCK_PREFERENCES, "_Preferences...", NULL, "Adjust preferences", (GCallback)on_preferences_action },
    { "FileQuit", GTK_STOCK_QUIT, "_Quit", "<Ctrl>Q", "Exit application", (GCallback)on_exit_action },
};

void gtk_main_window::on_open_action(GtkWidget *widget, gtk_main_window *main)
{
    main->open_file();
}

void gtk_main_window::on_save_action(GtkWidget *widget, gtk_main_window *main)
{
    main->save_file();
}

void gtk_main_window::on_save_as_action(GtkWidget *widget, gtk_main_window *main)
{
    main->save_file_as();
}

void gtk_main_window::on_reorder_action(GtkWidget *widget, gtk_main_window *main)
{
    main->owner->reorder_plugins();
}

void gtk_main_window::on_preferences_action(GtkWidget *widget, gtk_main_window *main)
{
    GtkBuilder *prefs_builder = gtk_builder_new();
    GError *error = NULL;
    const gchar *objects[] = { "preferences", NULL };
    if (!gtk_builder_add_objects_from_file(prefs_builder, PKGLIBDIR "/calf-gui.xml", (gchar **)objects, &error))
    {
        g_warning("Cannot load preferences dialog: %s", error->message);
        g_error_free(error);
        g_object_unref(G_OBJECT(prefs_builder));
        return;
    }
    
    // styles selector
    GtkCellRenderer *cell;
    GtkListStore *styles = main->get_styles();
    GtkComboBox *cb = GTK_COMBO_BOX(gtk_builder_get_object(prefs_builder, "rcstyles"));
    gtk_combo_box_set_model(cb, GTK_TREE_MODEL(styles));
    cell = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(cb), cell, TRUE);
    gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(cb), cell, "text", 0, NULL);
    GtkTreeIter iter;
    gboolean valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(styles), &iter);
    while (valid) {
        GValue path = G_VALUE_INIT;
        gtk_tree_model_get_value(GTK_TREE_MODEL(styles), &iter, 1, &path);
        if (main->get_config()->style.compare(g_value_get_string(&path)) == 0) {
            gtk_combo_box_set_active_iter(cb, &iter);
            break;
        }
        valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(styles), &iter);
        g_value_unset(&path);
    }
    
    GtkComboBoxText *rack_float = GTK_COMBO_BOX_TEXT(gtk_builder_get_object(prefs_builder, "rack-float"));
    gtk_combo_box_text_append_text(rack_float, "Rows");
    gtk_combo_box_text_append_text(rack_float, "Columns");
    gtk_combo_box_set_active(GTK_COMBO_BOX(rack_float), main->get_config()->rack_float);
    GtkWidget *preferences_dlg = GTK_WIDGET(gtk_builder_get_object(prefs_builder, "preferences"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(prefs_builder, "show-rack-ears")), main->get_config()->rack_ears);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(prefs_builder, "win-to-tray")), main->get_config()->win_to_tray);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(prefs_builder, "win-start-hidden")), main->get_config()->win_start_hidden);
    gtk_spin_button_set_range(GTK_SPIN_BUTTON(gtk_builder_get_object(prefs_builder, "float-size")), 1, 32);
    gtk_spin_button_set_increments(GTK_SPIN_BUTTON(gtk_builder_get_object(prefs_builder, "float-size")), 1, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(gtk_builder_get_object(prefs_builder, "float-size")), main->get_config()->float_size);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(prefs_builder, "show-vu-meters")), main->get_config()->vu_meters);
    int response = gtk_dialog_run(GTK_DIALOG(preferences_dlg));
    if (response == GTK_RESPONSE_OK)
    {
        GValue path_ = G_VALUE_INIT;
        GtkTreeIter iter;
        gtk_combo_box_get_active_iter(cb, &iter);
        gtk_tree_model_get_value(GTK_TREE_MODEL(styles), &iter, 1, &path_);
        main->get_config()->rack_ears = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(prefs_builder, "show-rack-ears")));
        main->get_config()->win_to_tray = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(prefs_builder, "win-to-tray")));
        main->get_config()->win_start_hidden = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(prefs_builder, "win-start-hidden")));
        main->get_config()->rack_float = gtk_combo_box_get_active(GTK_COMBO_BOX(rack_float));
        main->get_config()->float_size = gtk_spin_button_get_value(GTK_SPIN_BUTTON(gtk_builder_get_object(prefs_builder, "float-size")));
        main->get_config()->vu_meters = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(prefs_builder, "show-vu-meters")));
        main->get_config()->style = g_value_get_string(&path_);
        main->get_config()->save(main->get_config_db());
        //main->load_style(g_value_get_string(&path_));
        g_value_unset(&path_);
    }
    gtk_widget_destroy(preferences_dlg);
    g_object_unref(G_OBJECT(prefs_builder));
}

void gtk_main_window::on_exit_action(GtkWidget *widget, gtk_main_window *main)
{
    gtk_widget_destroy(GTK_WIDGET(main->toplevel));
}

void gtk_main_window::add_plugin(jack_host *plugin)
{
    if (toplevel)
    {
        plugin_strip *strip = create_strip(plugin);
        plugins[plugin] = strip;
        update_strip(plugin);
        sort_strips();
    }
    else {
        plugin_queue.push_back(plugin);
        //plugins[plugin] = NULL;
    }
}

void gtk_main_window::del_plugin(plugin_ctl_iface *plugin)
{
    if (!plugins.count(plugin))
        return;
    plugin_strip *strip = plugins[plugin];
    if (strip->gui_win)
        strip->gui_win->close();
    vector<GtkWidget *> to_destroy;
    for (std::map<plugin_ctl_iface *, plugin_strip *>::iterator i = plugins.begin(); i != plugins.end(); ++i)
    {
        if (i->second == strip)
            to_destroy.push_back(i->second->strip_table);
        else if(i->second->id > strip->id)
            i->second->id--;
    }
    for (unsigned int i = 0; i < to_destroy.size(); i++)
        gtk_container_remove(GTK_CONTAINER(strips_table), to_destroy[i]);
    plugins.erase(plugin);
    sort_strips();
}

void gtk_main_window::rename_plugin(plugin_ctl_iface *plugin, std::string name)
{
    if (!plugins.count(plugin))
        return;
    plugin_strip *strip = plugins[plugin];
    gtk_label_set_text(GTK_LABEL(strip->name), name.c_str());
    if (strip->gui_win) {
        gtk_window_set_title(GTK_WINDOW(strip->gui_win->toplevel), ("Calf - " + name).c_str());
    }
}

void gtk_main_window::set_window(plugin_ctl_iface *plugin, plugin_gui_window *gui_win)
{
    if (!plugins.count(plugin))
        return;
    plugin_strip *strip = plugins[plugin];
    if (!strip)
        return;
    strip->gui_win = gui_win;
    if (!is_closed)
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(strip->button), gui_win != NULL ? TRUE : FALSE);    
}

void gtk_main_window::refresh_all_presets(bool builtin_too)
{
    for (std::map<plugin_ctl_iface *, plugin_strip *>::iterator i = plugins.begin(); i != plugins.end(); ++i)
    {
        if (i->second && i->second->gui_win) {
            char ch = '0';
            i->second->gui_win->fill_gui_presets(true, ch);
            i->second->gui_win->fill_gui_presets(false, ch);
        }
    }
}

static gboolean
gui_button_pressed(GtkToggleButton *button, plugin_strip *strip)
{
    if (gtk_toggle_button_get_active(button) == (strip->gui_win != NULL))
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
connect_button_pressed(GtkWidget *button, plugin_strip *strip)
{
    if (strip->connector) {
        strip->connector->close();
        strip->connector = NULL;
    } else {
        strip->connector = new calf_connector(strip);
    }
    return TRUE;
}
static gboolean
extra_button_pressed(GtkWidget *button, plugin_strip *strip)
{
    if (strip->connector)
        strip->connector->close();
    strip->main_win->owner->remove_plugin(strip->plugin);
    return TRUE;
}

void gtk_main_window::show_rack_ears(bool show)
{
    for (std::map<plugin_ctl_iface *, plugin_strip *>::iterator i = plugins.begin(); i != plugins.end(); ++i)
    {
        if (show)
        {
            gtk_widget_show(i->second->leftBG);
            gtk_widget_show(i->second->rightBG);
        }
        else
        {
            gtk_widget_hide(i->second->leftBG);
            gtk_widget_hide(i->second->rightBG);
        }
    }
}

void gtk_main_window::show_vu_meters(bool show)
{
    for (std::map<plugin_ctl_iface *, plugin_strip *>::iterator i = plugins.begin(); i != plugins.end(); ++i)
    {
        if (show)
        {
            if (i->second->inBox)
                gtk_widget_show(i->second->inBox);
            if (i->second->outBox)
                gtk_widget_show(i->second->outBox);
        }
        else
        {
            if (i->second->inBox)
                gtk_widget_hide(i->second->inBox);
            if (i->second->outBox)
                gtk_widget_hide(i->second->outBox);
        }
    }
}

void gtk_main_window::on_edit_title(GtkWidget *ebox, GdkEventButton *event, plugin_strip *strip) {
    gtk_entry_set_text(GTK_ENTRY(strip->entry), gtk_label_get_text(GTK_LABEL(strip->name)));
    gtk_widget_grab_focus(strip->entry);
    gtk_widget_hide(strip->name);
    gtk_widget_show(strip->entry);
}

void gtk_main_window::on_activate_entry(GtkWidget *entry, plugin_strip *strip) {
    const char *txt = gtk_entry_get_text(GTK_ENTRY(entry));
    const char *old = gtk_label_get_text(GTK_LABEL(strip->name));
    if (strcmp(old, txt) and strlen(txt))
        strip->main_win->owner->rename_plugin(strip->plugin, txt);
    gtk_widget_hide(strip->entry);
    gtk_widget_show(strip->name);
}
gboolean gtk_main_window::on_blur_entry(GtkWidget *entry, GdkEvent *event, plugin_strip *strip) {
    gtk_widget_hide(strip->entry);
    gtk_widget_show(strip->name);
    return FALSE;
}

GtkWidget *gtk_main_window::create_vu_meter() {
    GtkWidget *vu = calf_vumeter_new();
    calf_vumeter_set_falloff(CALF_VUMETER(vu), 2.5);
    calf_vumeter_set_hold(CALF_VUMETER(vu), 1.5);
    calf_vumeter_set_width(CALF_VUMETER(vu), 100);
    calf_vumeter_set_height(CALF_VUMETER(vu), 12);
    calf_vumeter_set_position(CALF_VUMETER(vu), 2);
    return vu;
}

GtkWidget *gtk_main_window::create_meter_scale() {
    GtkWidget *vu = calf_meter_scale_new();
    CalfMeterScale *ms = CALF_METER_SCALE(vu);
    const unsigned long sz = 7;
    const double cv[sz] = {0., 0.0625, 0.125, 0.25, 0.5, 0.71, 1.};
    const vector<double> ck(cv, &cv[sz]);
    ms->marker   = ck;
    ms->dots     = 1;
    ms->position = 2;
    gtk_widget_set_name(vu, "Calf-MeterScale");
    return vu;
}

void gtk_main_window::on_table_clicked(GtkWidget *table, GdkEvent *event) {
    gtk_widget_grab_focus(table);
}

plugin_strip *gtk_main_window::create_strip(jack_host *plugin)
{
    /*    0          1    2             3      4          5           6          7
     *  0 ┌──────────┬────────────────────────────────────────────────┬──────────┐ 0
     *    │          │                   top light                    │          │
     *  1 │          ├────┬─────────────┬──────┬──────────┬───────────┤          │ 1
     *    │          │    │             │      │          │           │          │
     *    │          │ X  │    label    │ MIDI │  inputs  │  outputs  │          │
     *    │          │    │             │      │          │           │          │
     *  2 │   left   ├────┼─────────────┼──────┼──────────┼───────────┤   right  │ 2
     *    │   box    │    ╎             ╎      ╎          ╎           │    box   │
     *    │          │   buttons and params    ╎          ╎           │          │
     *    │          │    ╎             ╎      ╎          ╎           │          │
     *  3 │          ├────┴─────────────┴──────┴──────────┴───────────┤          │ 3
     *    │          │                  bottom light                  │          │
     *  4 └──────────┴────────────────────────────────────────────────┴──────────┘ 4
     *    0          1    2             3      4          5           6          7
     */
    
    plugin_strip *strip = new plugin_strip;
    strip->main_win = this;
    strip->plugin = plugin;
    strip->gui_win = NULL;
    strip->connector = NULL;
    strip->id = plugins.size();
    
    const plugin_metadata_iface *metadata = plugin->get_metadata_iface();
    GtkAttachOptions ao = (GtkAttachOptions)(GTK_EXPAND | GTK_FILL);
    
    strip->strip_table = gtk_table_new(7, 4, FALSE);
    gtk_table_set_col_spacings(GTK_TABLE(strip->strip_table), 0);
    gtk_table_set_row_spacings(GTK_TABLE(strip->strip_table), 0);
    
    // images for left side
    GtkWidget *nwImg     = gtk_image_new_from_pixbuf(images.get("side_d_nw"));
    GtkWidget *swImg     = gtk_image_new_from_pixbuf(images.get("side_d_sw"));
    GtkWidget *wImg      = gtk_image_new_from_pixbuf(images.get("side_d_w"));
    gtk_widget_set_size_request(GTK_WIDGET(wImg), 56, 1);
    
    // images for right side
    GtkWidget *neImg     = gtk_image_new_from_pixbuf(images.get("side_d_ne"));
    GtkWidget *seImg     = gtk_image_new_from_pixbuf(images.get("side_d_se"));
    GtkWidget *eImg      = gtk_image_new_from_pixbuf(images.get("side_d_e"));
    gtk_widget_set_size_request(GTK_WIDGET(eImg), 56, 1);
    
    // pack left box @ 0, 0
    GtkWidget *leftBG  = gtk_event_box_new();
    GtkWidget *leftBox = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(leftBG), leftBox);
    gtk_widget_set_name(leftBG, "CalfMainLeft");
    gtk_box_pack_start(GTK_BOX(leftBox), GTK_WIDGET(nwImg), FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(leftBox), GTK_WIDGET(swImg), FALSE, FALSE, 0);
    gtk_widget_show_all(GTK_WIDGET(leftBG));
    if (!get_config()->rack_ears)
        gtk_widget_hide(GTK_WIDGET(leftBG));
    gtk_table_attach(GTK_TABLE(strip->strip_table), leftBG, 0, 1, 0, 4, (GtkAttachOptions)(0), ao, 0, 0);
    
     // pack right box
    GtkWidget *rightBG = gtk_event_box_new();
    GtkWidget *rightBox = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(rightBG), rightBox);
    gtk_widget_set_name(rightBG, "CalfMainRight");
    gtk_box_pack_start(GTK_BOX(rightBox), GTK_WIDGET(neImg), FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(rightBox), GTK_WIDGET(seImg), FALSE, FALSE, 0);
    gtk_widget_show_all(GTK_WIDGET(rightBG));
    if (!get_config()->rack_ears)
        gtk_widget_hide(GTK_WIDGET(rightBG));
    gtk_table_attach(GTK_TABLE(strip->strip_table), rightBG, 6, 7, 0, 4, (GtkAttachOptions)(0), ao, 0, 0);
    
    strip->leftBG = leftBG;
    strip->rightBG = rightBG;
    
    // top light
    GtkWidget *topImg     = gtk_image_new_from_pixbuf(images.get("light_top"));
    gtk_widget_set_size_request(GTK_WIDGET(topImg), 1, 1);
    gtk_table_attach(GTK_TABLE(strip->strip_table), topImg, 1, 6, 0, 1, (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), (GtkAttachOptions)(0), 0, 0);
    gtk_widget_show(topImg);
    
    // remove buton 1, 1
    strip->extra = calf_button_new("×");
    g_signal_connect(GTK_OBJECT(strip->extra), "clicked", G_CALLBACK(extra_button_pressed), 
        (plugin_ctl_iface *)strip);
    gtk_widget_show(strip->extra);
    gtk_table_attach(GTK_TABLE(strip->strip_table), GTK_WIDGET(strip->extra), 1, 2, 1, 2, (GtkAttachOptions)0, (GtkAttachOptions)0, 5, 5);
    
    // title @ 2, 1
    strip->name = gtk_label_new(NULL);
    gtk_widget_set_name(GTK_WIDGET(strip->name), "Calf-Rack-Title");
    gtk_label_set_markup(GTK_LABEL(strip->name), plugin->instance_name.c_str());//metadata->get_label());
    gtk_label_set_justify(GTK_LABEL(strip->name), GTK_JUSTIFY_RIGHT);
    
    GtkWidget * align = gtk_alignment_new(0.0, 0.5, 0.0, 0.0);
    gtk_widget_set_size_request(align, 180, -1);
    
    GtkWidget *ebox = gtk_event_box_new ();
    gtk_event_box_set_visible_window(GTK_EVENT_BOX(ebox), FALSE);
    gtk_widget_set_events(ebox, GDK_BUTTON_PRESS_MASK);
    gtk_signal_connect(GTK_OBJECT(ebox), "button_press_event", GTK_SIGNAL_FUNC(on_edit_title), strip);
    
    gtk_container_add(GTK_CONTAINER(align), strip->name);
    gtk_container_add(GTK_CONTAINER(ebox), align);
    gtk_table_attach(GTK_TABLE(strip->strip_table), ebox, 2, 3, 1, 2, (GtkAttachOptions)0, ao, 10, 0);
    gtk_widget_show_all(ebox);
    
    strip->entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(strip->entry), "Calf-Rack-Entry");
    gtk_widget_set_name(strip->entry, "Calf-Rack-Entry");
    gtk_widget_set_size_request(strip->entry, 180, -1);
    gtk_table_attach(GTK_TABLE(strip->strip_table), strip->entry, 2, 3, 1, 2, (GtkAttachOptions)0, ao, 10, 0);
    gtk_widget_show_all(strip->entry);
    gtk_signal_connect(GTK_OBJECT(strip->entry), "activate", GTK_SIGNAL_FUNC(on_activate_entry), strip);
    gtk_signal_connect(GTK_OBJECT(strip->entry), "focus-out-event", GTK_SIGNAL_FUNC(on_blur_entry), strip);
    gtk_widget_hide(strip->entry);
    
    // open button
    strip->button = calf_toggle_button_new("Open");
    g_signal_connect(GTK_OBJECT(strip->button), "toggled", G_CALLBACK(gui_button_pressed), 
        (plugin_ctl_iface *)strip);
    gtk_widget_show(strip->button);
    //g_signal_connect (GTK_OBJECT (widget), "value-changed", G_CALLBACK (toggle_value_changed), (gpointer)this);

    // connect button
    strip->con = calf_toggle_button_new("Connect");
    g_signal_connect(GTK_OBJECT(strip->con), "toggled", G_CALLBACK(connect_button_pressed), 
        (plugin_ctl_iface *)strip);
    gtk_widget_show(strip->con);
    
    // button box @ 1, 2
    GtkWidget *balign = gtk_alignment_new(0, 0.8, 0, 0);
    GtkWidget *buttonBox = gtk_hbox_new(FALSE, 5);
    gtk_box_pack_start(GTK_BOX(buttonBox), GTK_WIDGET(strip->button), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(buttonBox), GTK_WIDGET(strip->con), FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(balign), buttonBox);
    gtk_table_attach(GTK_TABLE(strip->strip_table), balign, 1, 3, 2, 3, ao, ao, 5, 5);
    gtk_widget_show_all(balign);
    
    // param box
    GtkWidget *palign = gtk_alignment_new(1, 1, 0, 0);
    plugin_gui_widget *widget = new plugin_gui_widget(this, this);
    strip->gui_widget = widget;
    GtkWidget *paramBox = widget->create(plugin);
    gtk_container_add(GTK_CONTAINER(palign), paramBox);
    gtk_table_attach(GTK_TABLE(strip->strip_table), palign, 3, 6, 2, 3, ao, (GtkAttachOptions)0, 5, 5);
    gtk_widget_show_all(palign);
    
    // midi box @ 2, 1
    if (metadata->get_midi()) {
        GtkWidget *led = calf_led_new();
        GtkWidget *midiBox = gtk_vbox_new(FALSE, 1);
        gtk_box_pack_start(GTK_BOX(midiBox), GTK_WIDGET(gtk_label_new("MIDI")), FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(midiBox), GTK_WIDGET(led), FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(midiBox), gtk_label_new(""), TRUE, TRUE, 0);
        gtk_table_attach(GTK_TABLE(strip->strip_table), midiBox, 3, 4, 1, 2, (GtkAttachOptions)0, (GtkAttachOptions)0, 5, 3);
        gtk_widget_set_size_request(GTK_WIDGET(led), 25, 25);
        strip->midi_in = led;
        gtk_widget_show_all(midiBox);
    } else {
        GtkWidget *led = gtk_label_new("");
        gtk_table_attach(GTK_TABLE(strip->strip_table), led, 3, 4, 1, 2, GTK_FILL, GTK_EXPAND, 5, 3);
        gtk_widget_set_size_request(GTK_WIDGET(led), 25, 25);
        strip->midi_in = led;
        gtk_widget_show(strip->midi_in);
    }
    
    strip->inBox = NULL;
    strip->outBox = NULL;
    strip->audio_in.clear();
    strip->audio_out.clear();
    
    GtkWidget *vu;
    if (metadata->get_input_count()) {
        
        GtkWidget *inBox  = gtk_vbox_new(FALSE, 1);
        
        gtk_box_pack_start(GTK_BOX(inBox), gtk_label_new("Audio In"),FALSE, FALSE, 0);
        
        for (int i = 0; i < metadata->get_input_count(); i++)
        {
            vu = create_vu_meter();
            gtk_box_pack_start(GTK_BOX(inBox), vu, TRUE, TRUE, 0);
            strip->audio_in.push_back(vu);
        }
        vu = create_meter_scale();
        gtk_box_pack_start(GTK_BOX(inBox), vu, TRUE, TRUE, 0);
        
        strip->inBox = gtk_alignment_new(0.f, 0.f, 1.f, 0.f);
        gtk_container_add(GTK_CONTAINER(strip->inBox), inBox);
        
        gtk_table_attach(GTK_TABLE(strip->strip_table), strip->inBox, 4, 5, 1, 2, ao, ao, 5, 3);
        
        if (get_config()->vu_meters)
            gtk_widget_show_all(strip->inBox);
            
        gtk_widget_set_size_request(GTK_WIDGET(strip->inBox), 180, -1);
    } else {
        GtkWidget *inBox = gtk_label_new("");
        gtk_table_attach(GTK_TABLE(strip->strip_table), inBox, 4, 5, 1, 2, GTK_FILL, GTK_EXPAND, 5, 3);
        gtk_widget_set_size_request(GTK_WIDGET(inBox), 180, -1);
        gtk_widget_show(inBox);
    }

    if (metadata->get_output_count()) {
        
        GtkWidget *outBox  = gtk_vbox_new(FALSE, 1);
        
        gtk_box_pack_start(GTK_BOX(outBox), gtk_label_new("Audio Out"),TRUE, TRUE, 0);
        
        for (int i = 0; i < metadata->get_output_count(); i++)
        {
            vu = create_vu_meter();
            gtk_box_pack_start(GTK_BOX(outBox), vu, TRUE, TRUE, 0);
            strip->audio_out.push_back(vu);
        }
        vu = create_meter_scale();
        gtk_box_pack_start(GTK_BOX(outBox), vu, TRUE, TRUE, 0);
        
        strip->outBox = gtk_alignment_new(0.f, 0.f, 1.f, 0.f);
        gtk_container_add(GTK_CONTAINER(strip->outBox), outBox);
        
        gtk_table_attach(GTK_TABLE(strip->strip_table), strip->outBox, 5, 6, 1, 2, ao, ao, 5, 3);
        
        if (get_config()->vu_meters)
            gtk_widget_show_all(strip->outBox);
            
        gtk_widget_set_size_request(GTK_WIDGET(strip->outBox), 180, -1);
    } else {
        GtkWidget *outBox = gtk_label_new("");
        gtk_table_attach(GTK_TABLE(strip->strip_table), outBox, 5, 6, 1, 2, GTK_FILL, GTK_EXPAND, 5, 3);
        gtk_widget_set_size_request(GTK_WIDGET(outBox), 180, -1);
        gtk_widget_show(outBox);
    }
    
    
    // bottom light
    GtkWidget *botImg     = gtk_image_new_from_pixbuf(images.get("light_bottom"));
    gtk_widget_set_size_request(GTK_WIDGET(botImg), 1, 1);
    gtk_table_attach(GTK_TABLE(strip->strip_table), botImg, 1, 6, 3, 4, (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), (GtkAttachOptions)(0), 0, 0);
    gtk_widget_show(botImg);
    
    gtk_widget_show(GTK_WIDGET(strip->strip_table));
    
    return strip;
}

void gtk_main_window::sort_strips()
{
    if(plugins.size() <= 0) return;
    int rack_float = get_config()->rack_float; // 0=horiz, 1=vert
    int float_size = get_config()->float_size; // amout of rows/cols before line break
    int posx, posy;
    gtk_table_resize(GTK_TABLE(strips_table), (int)(plugins.size() / float_size + 1), float_size);
    for (std::map<plugin_ctl_iface *, plugin_strip *>::iterator i = plugins.begin(); i != plugins.end(); ++i)
    {
        switch (rack_float) {
            case 0:
            default:
                posx = i->second->id % float_size;
                posy = (int)(i->second->id / float_size);
                break;
            case 1:
                posy = i->second->id % float_size;
                posx = (int)(i->second->id / float_size);
                break;
        }
        bool rem = false;
        if(i->second->strip_table->parent != NULL) {
            rem = true;
            g_object_ref(i->second->strip_table);
            gtk_container_remove(GTK_CONTAINER(strips_table), GTK_WIDGET(i->second->strip_table));
        }
        gtk_table_attach(GTK_TABLE(strips_table), i->second->strip_table, posx, posx + 1, posy, posy + 1, (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 0, 0);
        if(rem) g_object_unref(i->second->strip_table);
    }
}

void gtk_main_window::update_strip(plugin_ctl_iface *plugin)
{
    // plugin_strip *strip = plugins[plugin];
    // assert(strip);
    
}

void gtk_main_window::open_gui(plugin_ctl_iface *plugin)
{
    plugin_gui_window *gui_win = new plugin_gui_window(this, this);
    std::string title = "Calf - ";
    gui_win->create(plugin, (title + ((jack_host *)plugin)->get_instance_name()).c_str(), plugin->get_metadata_iface()->get_id()); //(owner->get_client_name() + " - " + plugin->get_metadata_iface()->get_label()).c_str(), plugin->get_metadata_iface()->get_id());
    gtk_widget_show(GTK_WIDGET(gui_win->toplevel));
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
#define countof(X) ( (size_t) ( sizeof(X)/sizeof*(X) ) )
void gtk_main_window::register_icons()
{
    const char *names[]={"Allpass", "Amplifier", "Analyser",
                         "Bandpass", "Chorus", "Comb", "Compressor",
                         "Constant", "Converter", "Delay", "Distortion",
                         "Dynamics", "Envelope", "EQ", "Expander",
                         "Filter", "Flanger", "Function", "Gate",
                         "Generator", "Highpass", "Instrument",
                         "Limiter", "Mixer", "Modulator", "MultiEQ",
                         "Oscillator", "ParaEQ", "Phaser", "Pitch",
                         "Reverb", "Simulator", "Spatial", "Spectral",
                         "Utility", "Waveshaper"};
    factory = gtk_icon_factory_new ();
    for (size_t i = 0; i < countof(names); i++) {
        char name[1024];
        strcpy(name, "LV2-");
        strcat(name, names[i]);
        if (!gtk_icon_factory_lookup(factory, name)) {
            std::string iname = std::string(PKGLIBDIR) + "icons/LV2/" + names[i] + ".svg";
            GdkPixbuf *buf    = gdk_pixbuf_new_from_file_at_size(iname.c_str(), 64, 64, NULL);
            GtkIconSet *icon  = gtk_icon_set_new_from_pixbuf(buf);
            gtk_icon_factory_add (factory, name, icon);
            gtk_icon_set_unref(icon);
            g_object_unref(buf);
        }
    }
    gtk_icon_factory_add_default(factory);
}

void gtk_main_window::add_plugin_action(GtkWidget *src, gpointer data)
{
    add_plugin_params *app = (add_plugin_params *)data;
    app->main_win->new_plugin(app->name.c_str());
}

static void action_destroy_notify(gpointer data)
{
    delete (gtk_main_window::add_plugin_params *)data;
}

std::string gtk_main_window::make_plugin_list(GtkActionGroup *actions)
{
    string s = plugin_pre_xml;
    const plugin_registry::plugin_vector &plugins = plugin_registry::instance().get_all();
    std::string type   = "";
    std::string tmp    = "";
    std::string last   = "";
    unsigned int count = 0;
    unsigned int size  = plugins.size();
    
    const plugin_metadata_iface *p = plugins[0];
    
    for(unsigned int i = 0; i <= size; i++)
    {
        if (i < size) {
            p = plugins[i];
            type = (p->get_plugin_info()).plugin_type;
            type = type.substr(0, type.length() - 6);
        }
        if (type != last or i >= size or !i) {
            
            if (i) {
                if (count > 1) {
                    s += "<menu action='" + last + "'>" + tmp + "</menu>";
                    GtkAction *a = gtk_action_new(last.c_str(), last.c_str(), NULL, ("LV2-" + last).c_str());
                    gtk_action_group_add_action(actions, a);
                } else {
                    s += tmp;
                }
            }
            tmp = "";
            last = type;
            count = 0;
        }
        if (i < size) {
            std::string action = "Add" + string(p->get_id()) + "Action";
            std::string stock  = "LV2-" + type;
            // TODO:
            // add lv2 stock icons to plug-ins and not just to menus
            // GTK_STOCK_OPEN -> ("LV2_" + type).c_str()
            GtkActionEntry ae  = { action.c_str(), stock.c_str(), p->get_label(), NULL, NULL, (GCallback)add_plugin_action };
            gtk_action_group_add_actions_full(actions, &ae, 1, (gpointer)new add_plugin_params(this, p->get_id()), action_destroy_notify);
            tmp   += string("<menuitem always-show-image=\"true\" action=\"") + action + "\" />";
            count += 1;
        }
    }
    return s + plugin_post_xml;
}


window_state describe_window (GtkWindow *win)
{
    window_state state = {};
    int x, y, width, height;
    gtk_window_get_position(win, &x, &y);
    gtk_window_get_size(win, &width, &height);
    state.screen = gtk_window_get_screen(win);
    state.x = x;
    state.y = y;
    state.width = width;
    state.height = height;
    return state;
}

void position_window (GtkWidget *win, window_state state)
{
    gdk_window_move_resize(win->window, state.x, state.y, state.width, state.height);
    gtk_window_set_screen(GTK_WINDOW(win), state.screen);
}

static void tray_activate_cb(GObject *icon, gtk_main_window *main)
{
    GtkWidget *widget = GTK_WIDGET(main->toplevel);
    GtkWindow *window = GTK_WINDOW(widget);
    GtkWidget *pwid;
    GtkWindow *pwin;
    gboolean visible = gtk_widget_get_visible(widget);
    if (visible) {
        main->winstate = describe_window(window);
        gtk_widget_hide(widget);
        for (std::map<plugin_ctl_iface *, plugin_strip *>::iterator i = main->plugins.begin(); i != main->plugins.end(); ++i) {
            if (i->second && i->second->gui_win) {
                pwid = i->second->gui_win->toplevel;
                pwin = GTK_WINDOW(pwid);
                i->second->gui_win->winstate = describe_window(pwin);
                gtk_widget_hide(pwid);
            }
        }
    } else {
        gtk_widget_show(widget);
        gtk_window_deiconify(window);
        position_window(widget, main->winstate);
        for (std::map<plugin_ctl_iface *, plugin_strip *>::iterator i = main->plugins.begin(); i != main->plugins.end(); ++i) {
            if (i->second && i->second->gui_win) {
                pwid = i->second->gui_win->toplevel;
                pwin = GTK_WINDOW(pwid);
                gtk_widget_show(pwid);
                gtk_window_deiconify(pwin);
                position_window(pwid, i->second->gui_win->winstate);
            }
        }
    }
}

static void tray_popup_cb(GtkStatusIcon *icon, guint button, guint32 time, gpointer menu)
{
    gtk_menu_popup(GTK_MENU(menu), NULL, NULL, gtk_status_icon_position_menu, icon, button, time);
}

static gint window_delete_cb(GtkWindow *window, GdkEvent *event, gtk_main_window *main)
{
    if (main->get_config()->win_to_tray) {
        tray_activate_cb(NULL, main);
        return TRUE;
    } else {
        return FALSE;
    }
}

static void window_destroy_cb(GtkWindow *window, gtk_main_window *main)
{
    main->owner->on_main_window_destroy();
}

static gint window_hide (gtk_main_window *main)
{
    main->winstate = describe_window(main->toplevel);
    gtk_widget_hide(GTK_WIDGET(main->toplevel));
    return FALSE;
}

void gtk_main_window::create()
{
    register_icons();
    toplevel = GTK_WINDOW(gtk_window_new (GTK_WINDOW_TOPLEVEL));
    std::string title = "Calf JACK Host";
    if (!owner->get_client_name().empty())
        title = title + " - session: " + owner->get_client_name();
    gtk_window_set_title(toplevel, title.c_str());
    gtk_window_set_icon_name(toplevel, "calf");
    gtk_window_set_role(toplevel, "calf_rack");
    //due to https://developer.gnome.org/gtk2/stable/GtkWindow.html
    //gtk_window_set_wmclass (). even though it says not to use this
    //function, it is the only way to get primitive WMs like fluxbox
    //to separate calf instances so that it can remember different positions.
    //Unlike what is stated there gtk_window_set_role() is not 
    //interpreted correctly by fluxbox and thus wmclass call is not
    //yet obsolete
    gtk_window_set_wmclass(toplevel, title.c_str(), "calfjackhost");
    
    load_style((PKGLIBDIR "styles/" + get_config()->style).c_str());
    
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
    
    gtk_widget_set_size_request(GTK_WIDGET(gtk_ui_manager_get_widget(ui_mgr, "/ui/menubar")), 640, -1);
    
    gtk_widget_set_name(GTK_WIDGET(gtk_ui_manager_get_widget(ui_mgr, "/ui/menubar")), "Calf-Menu");
    
    plugin_actions = gtk_action_group_new("plugins");
    string plugin_xml = make_plugin_list(plugin_actions);
    gtk_ui_manager_insert_action_group(ui_mgr, plugin_actions, 0);    
    gtk_ui_manager_add_ui_from_string(ui_mgr, plugin_xml.c_str(), -1, &error);
    
    strips_table = gtk_table_new(0, 1, FALSE);
    gtk_table_set_col_spacings(GTK_TABLE(strips_table), 0);
    gtk_table_set_row_spacings(GTK_TABLE(strips_table), 0);
    
    for(GList *p = GTK_TABLE(strips_table)->children; p != NULL; p = p->next)
    {
        GtkTableChild *c = (GtkTableChild *)p->data;
        if (c->top_attach == 0) {
            gtk_misc_set_alignment(GTK_MISC(c->widget), 0.5, 0);
        }
    }
    for (std::vector<jack_host *>::iterator i = plugin_queue.begin(); i != plugin_queue.end(); ++i)
    {
        plugin_strip *st = create_strip(*i);
        plugins[*i] = st;
        update_strip(*i);        
    }
    sort_strips();
    
    GtkWidget *evbox = gtk_event_box_new();
    gtk_widget_set_name(evbox, "Calf-Rack");
    gtk_container_add(GTK_CONTAINER(evbox), strips_table);
    gtk_container_add(GTK_CONTAINER(all_vbox), evbox);
    gtk_container_add(GTK_CONTAINER(toplevel), all_vbox);
    
    gtk_signal_connect(GTK_OBJECT(evbox), "button-press-event", GTK_SIGNAL_FUNC(on_table_clicked), NULL);
    gtk_widget_set_can_focus(evbox, TRUE);
    
    gtk_widget_set_name(GTK_WIDGET(strips_table), "Calf-Container");
    
    gtk_window_add_accel_group(toplevel, gtk_ui_manager_get_accel_group(ui_mgr));
    
    gtk_widget_show(GTK_WIDGET(strips_table));
    gtk_widget_show(GTK_WIDGET(evbox));
    gtk_widget_show(GTK_WIDGET(all_vbox));
    gtk_widget_show(GTK_WIDGET(toplevel));
    
    source_id = g_timeout_add_full(G_PRIORITY_DEFAULT, 1000/30, on_idle, this, NULL); // 30 fps should be enough for everybody
    
    notifier = get_config_db()->add_listener(this);
    on_config_change();
    g_signal_connect(GTK_OBJECT(toplevel), "destroy", G_CALLBACK(window_destroy_cb), this);
    g_signal_connect(GTK_OBJECT(toplevel), "delete_event", G_CALLBACK(window_delete_cb), this);
    
    if (get_config()->win_start_hidden)
        g_idle_add((GSourceFunc)window_hide, this);
}

void gtk_main_window::create_status_icon()
{
    GtkStatusIcon *icon = gtk_status_icon_new_from_icon_name("calf");
    
    GtkWidget *menu = gtk_menu_new();
    GtkWidget *view = gtk_menu_item_new_with_label ("View");
    GtkWidget *exit = gtk_menu_item_new_with_label ("Exit");
    g_signal_connect (G_OBJECT (view), "activate", G_CALLBACK (tray_activate_cb), this);
    g_signal_connect (G_OBJECT (exit), "activate", G_CALLBACK (window_destroy_cb), this);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), view);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), exit);
    gtk_widget_show_all (menu);
    
    gtk_status_icon_set_tooltip (icon, "Calf Studio Gear");
    
    g_signal_connect(GTK_STATUS_ICON (icon), "activate", GTK_SIGNAL_FUNC (tray_activate_cb), this);
    g_signal_connect(GTK_STATUS_ICON (icon), "popup-menu", GTK_SIGNAL_FUNC (tray_popup_cb), menu);
}

void gtk_main_window::on_config_change()
{
    get_config()->load(get_config_db());
    show_rack_ears(get_config()->rack_ears);    
    show_vu_meters(get_config()->vu_meters);
    sort_strips();
}

void gtk_main_window::refresh_plugin(plugin_ctl_iface *plugin)
{
    if (plugins.count(plugin))
    {
        if (plugins[plugin]->gui_win)
            plugins[plugin]->gui_win->refresh();
        if (plugins[plugin]->gui_widget)
            plugins[plugin]->gui_widget->refresh();
    }
}

void gtk_main_window::refresh_plugin_param(plugin_ctl_iface *plugin, int param_no)
{
    if (plugins.count(plugin))
    {
        if (plugins[plugin]->gui_win)
            plugins[plugin]->gui_win->get_gui()->refresh(param_no);
        if (plugins[plugin]->gui_widget)
            plugins[plugin]->gui_widget->get_gui()->refresh(param_no);
    }
}

void gtk_main_window::on_closed()
{
    if (notifier)
    {
        delete notifier;
        notifier = NULL;
    }
    if (source_id)
        g_source_remove(source_id);
    is_closed = true;
    toplevel = NULL;

    for (std::map<plugin_ctl_iface *, plugin_strip *>::iterator i = plugins.begin(); i != plugins.end(); ++i)
    {
        if (i->second && i->second->gui_win) {
            i->second->gui_win->close();
        }
    }
    plugins.clear();
}

static inline float LVL(float value)
{
    return value; //sqrt(value) * 0.75;
}

gboolean gtk_main_window::on_idle(void *data)
{
    gtk_main_window *self = (gtk_main_window *)data;

    self->owner->on_idle();
    
    if (!self->refresh_controller.check_redraw(GTK_WIDGET(self->toplevel)))
        return TRUE;

    for (std::map<plugin_ctl_iface *, plugin_strip *>::iterator i = self->plugins.begin(); i != self->plugins.end(); ++i)
    {
        if (i->second)
        {
            plugin_ctl_iface *plugin = i->first;
            plugin_strip *strip = i->second;
            int idx = 0;
            if (strip->inBox && gtk_widget_is_drawable (strip->inBox)) {
                for (int i = 0; i < (int)strip->audio_in.size(); i++) {
                    calf_vumeter_set_value(CALF_VUMETER(strip->audio_in[i]), LVL(plugin->get_level(idx++)));
                }
            }
            else
                idx += strip->audio_in.size();
            if (strip->outBox && gtk_widget_is_drawable (strip->outBox)) {
                for (int i = 0; i < (int)strip->audio_out.size(); i++) {
                    calf_vumeter_set_value(CALF_VUMETER(strip->audio_out[i]), LVL(plugin->get_level(idx++)));
                }
            }
            else
                idx += strip->audio_out.size();
            if (plugin->get_metadata_iface()->get_midi()) {
                calf_led_set_value (CALF_LED (strip->midi_in), plugin->get_level(idx++));
            }
        }
    }
    return TRUE;
}

void gtk_main_window::open_file()
{
    GtkWidget *dialog;
    dialog = gtk_file_chooser_dialog_new ("Open File",
        toplevel,
        GTK_FILE_CHOOSER_ACTION_OPEN,
        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
        GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
        NULL);
    if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
    {
        char *filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
        char *error = owner->open_file(filename);
        if (error) 
            display_error(error, filename);
        else
            owner->set_current_filename(filename);
        g_free (filename);
        free (error);
    }
    gtk_widget_destroy (dialog);
}

bool gtk_main_window::save_file()
{
    if (owner->get_current_filename().empty())
        return save_file_as();

    const char *error = owner->save_file(owner->get_current_filename().c_str());
    if (error)
    {
        display_error(error, owner->get_current_filename().c_str());
        return false;
    }
    return true;
}

bool gtk_main_window::save_file_as()
{
    GtkWidget *dialog;
    bool success = false;
    dialog = gtk_file_chooser_dialog_new ("Save File",
        toplevel,
        GTK_FILE_CHOOSER_ACTION_SAVE,
        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
        GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
        NULL);
    if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
    {
        char *filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
        char *error = owner->save_file(filename);
        if (error) 
            display_error(error, filename);
        else
        {
            owner->set_current_filename(filename);
            success = true;
        }
        g_free (filename);
        free(error);
    }
    gtk_widget_destroy (dialog);
    return success;
}

void gtk_main_window::display_error(const char *error, const char *filename)
{
    GtkWidget *dialog;
    dialog = gtk_message_dialog_new_with_markup (toplevel, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, error, filename, NULL);
    gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);
}

GtkWidget *gtk_main_window::create_progress_window()
{
    GtkWidget *tlw = gtk_window_new ( GTK_WINDOW_TOPLEVEL );
    gtk_window_set_type_hint (GTK_WINDOW (tlw), GDK_WINDOW_TYPE_HINT_DIALOG);
    GtkWidget *pbar = gtk_progress_bar_new();
    gtk_container_add (GTK_CONTAINER(tlw), pbar);
    gtk_widget_show_all (pbar);
    return tlw;
}

void gtk_main_window::report_progress(float percentage, const std::string &message)
{
    if (percentage < 100)
    {
        if (!progress_window) {
            progress_window = create_progress_window();
            gtk_window_set_modal (GTK_WINDOW (progress_window), TRUE);
            if (toplevel)
                gtk_window_set_transient_for (GTK_WINDOW (progress_window), toplevel);
            gtk_widget_show(progress_window);
        }
        GtkWidget *pbar = gtk_bin_get_child (GTK_BIN (progress_window));
        if (!message.empty())
            gtk_progress_bar_set_text (GTK_PROGRESS_BAR (pbar), message.c_str());
        gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (pbar), percentage / 100.0);
    }
    else
    {
        if (progress_window) {
            gtk_window_set_modal (GTK_WINDOW (progress_window), FALSE);
            gtk_widget_destroy (progress_window);
            progress_window = NULL;
        }
    }
    
    while (gtk_events_pending ())
        gtk_main_iteration ();
}

void gtk_main_window::add_condition(const std::string &name)
{
    conditions.insert(name);
}

void gtk_main_window::show_error(const std::string &text)
{
    GtkWidget *widget = gtk_message_dialog_new (NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, "%s", text.c_str());
    gtk_dialog_run (GTK_DIALOG (widget));
    gtk_widget_destroy (widget);
}

GtkListStore *gtk_main_window::get_styles()
{
    std::vector <calf_utils::direntry> list = calf_utils::list_directory(PKGLIBDIR"styles");
    GtkListStore *store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
    for (std::vector<calf_utils::direntry>::iterator i = list.begin(); i != list.end(); i++) {
        string title = i->name;
        std::string rcf = i->full_path + "/gtk.rc";
        ifstream infile(rcf.c_str());
        if (infile.good()) {
            string line;
            getline(infile, line);
            title = line.substr(1);
        }
        gtk_list_store_insert_with_values(store, NULL, -1,
                                  0, title.c_str(),
                                  1, i->name.c_str(),
                                  -1);
    }
    return store;
}
void gtk_main_window::load_style(std::string path) {
    gtk_rc_parse((path + "/gtk.rc").c_str());
    gtk_rc_reset_styles(gtk_settings_get_for_screen(gdk_screen_get_default()));
    images.set_path(path);
}
