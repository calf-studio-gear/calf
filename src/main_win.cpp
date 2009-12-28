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
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 * Boston, MA  02110-1301  USA
 */
 
#include <assert.h>
#include <config.h>
#include <calf/ctl_led.h>
#include <calf/giface.h>
#include <calf/gui.h>
#include <calf/preset.h>
#include <calf/preset_gui.h>
#include <calf/main_win.h>

using namespace calf_plugins;
using namespace std;

static const char *ui_xml = 
"<ui>\n"
"  <menubar>\n"
"    <menu action=\"FileMenuAction\">\n"
"      <menuitem action=\"FileOpen\"/>\n"
"      <menuitem action=\"FileSave\"/>\n"
"      <menuitem action=\"FileSaveAs\"/>\n"
"      <separator/>\n"
"      <menuitem action=\"FileQuit\"/>\n"
"    </menu>\n"
"    <menu action=\"AddPluginMenuAction\" />\n"
"  </menubar>\n"
"</ui>\n"
;

static void open_action(GtkWidget *widget, main_window *main)
{
    main->open_file();
}

static void save_action(GtkWidget *widget, main_window *main)
{
    main->save_file();
}

static void save_as_action(GtkWidget *widget, main_window *main)
{
    main->save_file_as();
}

static void exit_action(GtkWidget *widget, main_window *main)
{
    gtk_widget_destroy(GTK_WIDGET(main->toplevel));
}

static const GtkActionEntry actions[] = {
    { "FileMenuAction", NULL, "_File", NULL, "File-related operations", NULL },
    { "FileOpen", GTK_STOCK_OPEN, "_Open", "<Ctrl>O", "Open a rack file", (GCallback)open_action },
    { "FileSave", GTK_STOCK_SAVE, "_Save", "<Ctrl>S", "Save a rack file", (GCallback)save_action },
    { "FileSaveAs", GTK_STOCK_SAVE_AS, "Save _as...", NULL, "Save a rack file as", (GCallback)save_as_action },
    { "HostMenuAction", NULL, "_Host", NULL, "Host-related operations", NULL },
    { "AddPluginMenuAction", NULL, "_Add plugin", NULL, "Add a plugin to the rack", NULL },
    { "FileQuit", GTK_STOCK_QUIT, "_Quit", "<Ctrl>Q", "Exit application", (GCallback)exit_action },
};

main_window::main_window()
{
    toplevel = NULL;
    owner = NULL;
    is_closed = true;
    draw_rackmounts = true;
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
            row = c->top_attach;
            break;
        }
    }
    g_assert(row != -1);
    
    vector<GtkWidget *> to_destroy;
    for(GList *p = GTK_TABLE(strips_table)->children; p != NULL; p = p->next)
    {
        GtkTableChild *c = (GtkTableChild *)p->data;
        if (c->top_attach >= row && c->top_attach < row + 4)
            to_destroy.push_back(c->widget);
        if (c->top_attach >= row + 4)
        {
            c->top_attach -= 4;
            c->bottom_attach -= 4;
        }
    }
    
    for (unsigned int i = 0; i < to_destroy.size(); i++)
        gtk_container_remove(GTK_CONTAINER(strips_table), to_destroy[i]);
    //for (unsigned int i = 0; i < to_destroy.size(); i++)
    //    gtk_widget_destroy(to_destroy[i]);
    
    plugins.erase(plugin);
    int rows = 0, cols = 0;
    g_object_get(G_OBJECT(strips_table), "n-rows", &rows, "n-columns", &cols, NULL);
    gtk_table_resize(GTK_TABLE(strips_table), rows - 4, cols);
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
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(strip->button), gui_win != NULL);
        
    GtkToggleButton *tb = GTK_TOGGLE_BUTTON(strip->button);
    if (strip->gui_win) {
        gtk_button_set_label(GTK_BUTTON(tb), "Hide");
    } else {
        gtk_button_set_label(GTK_BUTTON(tb), "Show");
    }
    
}

void main_window::refresh_all_presets(bool builtin_too)
{
    for (std::map<plugin_ctl_iface *, plugin_strip *>::iterator i = plugins.begin(); i != plugins.end(); i++)
    {
        if (i->second && i->second->gui_win) {
            char ch = '0';
            i->second->gui_win->fill_gui_presets(true, ch);
            i->second->gui_win->fill_gui_presets(false, ch);
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
        gtk_button_set_label(GTK_BUTTON(tb), "Show");
    } else {
        strip->main_win->open_gui(strip->plugin);
        gtk_button_set_label(GTK_BUTTON(tb), "Hide");
    }
    return TRUE;
}

static gboolean
extra_button_pressed(GtkWidget *button, main_window::plugin_strip *strip)
{
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
    gtk_table_resize(GTK_TABLE(strips_table), row + 4, cols);
    
    if(draw_rackmounts) {
        // images for left side
        GtkWidget *nwImg     = gtk_image_new_from_file(PKGLIBDIR "/side_d_nw.png");
        GtkWidget *swImg     = gtk_image_new_from_file(PKGLIBDIR "/side_d_sw.png");
        GtkWidget *wImg      = gtk_image_new_from_file(PKGLIBDIR "/side_d_w.png");
        gtk_widget_set_size_request(GTK_WIDGET(wImg), 56, 1);
        
        // images for right side
        GtkWidget *neImg     = gtk_image_new_from_file(PKGLIBDIR "/side_d_ne.png");
        GtkWidget *seImg     = gtk_image_new_from_file(PKGLIBDIR "/side_d_se.png");
        GtkWidget *eImg      = gtk_image_new_from_file(PKGLIBDIR "/side_d_e.png");
        gtk_widget_set_size_request(GTK_WIDGET(eImg), 56, 1);
        
        // pack left box
        GtkWidget *leftBox = gtk_vbox_new(FALSE, 0);
        gtk_box_pack_start(GTK_BOX(leftBox), GTK_WIDGET(nwImg), FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(leftBox), GTK_WIDGET(wImg), TRUE, TRUE, 0);
        gtk_box_pack_end(GTK_BOX(leftBox), GTK_WIDGET(swImg), FALSE, FALSE, 0);
        gtk_table_attach(GTK_TABLE(strips_table), leftBox, 0, 1, row, row + 4, (GtkAttachOptions)(0), (GtkAttachOptions)(GTK_EXPAND | GTK_FILL), 0, 0);
        gtk_widget_show_all(GTK_WIDGET(leftBox));
        
         // pack right box
        GtkWidget *rightBox = gtk_vbox_new(FALSE, 0);
        gtk_box_pack_start(GTK_BOX(rightBox), GTK_WIDGET(neImg), FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(rightBox), GTK_WIDGET(eImg), TRUE, TRUE, 0);
        gtk_box_pack_end(GTK_BOX(rightBox), GTK_WIDGET(seImg), FALSE, FALSE, 0);
        gtk_table_attach(GTK_TABLE(strips_table), rightBox, 5, 6, row, row + 4, (GtkAttachOptions)(0), (GtkAttachOptions)(GTK_EXPAND | GTK_FILL), 0, 0);
        gtk_widget_show_all(GTK_WIDGET(rightBox));
    }
    
    // top light
    GtkWidget *topImg     = gtk_image_new_from_file(PKGLIBDIR "/light_top.png");
    gtk_widget_set_size_request(GTK_WIDGET(topImg), 1, 1);
    gtk_table_attach(GTK_TABLE(strips_table), topImg, 1, 5, row, row + 1, (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), (GtkAttachOptions)(0), 0, 0);
    gtk_widget_show(topImg);
    strip->name = topImg;
    row ++;
    
    // title @ 1, 1
    char buf[128];
    sprintf(buf, "<span size=\"15000\">%s</span>", plugin->get_label());
    GtkWidget *title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title), buf);
    gtk_table_attach(GTK_TABLE(strips_table), title, 1, 2, row, row + 1, ao, GTK_SHRINK, 20, 10);
    gtk_widget_show(title);
    
    // open button
    GtkWidget *label = gtk_toggle_button_new_with_label("Show");
    strip->button = label;
    gtk_widget_set_size_request(GTK_WIDGET(label), 110, -1);
    gtk_signal_connect(GTK_OBJECT(label), "toggled", G_CALLBACK(gui_button_pressed), 
        (plugin_ctl_iface *)strip);
    gtk_widget_show(strip->button);

    // delete buton
    GtkWidget *extra = gtk_button_new_with_label("Remove");
    strip->extra = extra;
    gtk_widget_set_size_request(GTK_WIDGET(extra), 110, -1);
    gtk_signal_connect(GTK_OBJECT(extra), "clicked", G_CALLBACK(extra_button_pressed), 
        (plugin_ctl_iface *)strip);
    gtk_widget_show(strip->extra);
    
    // button box @ 1, 2
    GtkWidget *buttonBox = gtk_hbox_new(TRUE, 10);
    gtk_box_pack_start(GTK_BOX(buttonBox), GTK_WIDGET(strip->button), TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(buttonBox), GTK_WIDGET(strip->extra), TRUE, TRUE, 0);
    gtk_table_attach(GTK_TABLE(strips_table), buttonBox, 1, 2, row + 1, row + 2, (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), GTK_EXPAND, 10, 10);
    gtk_widget_show(buttonBox);
    
    // midi box
    if (plugin->get_midi()) {
        label = calf_led_new();
        GtkWidget *midiBox = gtk_vbox_new(FALSE, 1);
        gtk_box_pack_start(GTK_BOX(midiBox), GTK_WIDGET(gtk_label_new("MIDI")), TRUE, TRUE, 0);
        gtk_box_pack_start(GTK_BOX(midiBox), GTK_WIDGET(label), TRUE, TRUE, 0);
        gtk_table_attach(GTK_TABLE(strips_table), midiBox, 2, 3, row, row + 1, GTK_FILL, GTK_EXPAND, 5, 3);
        gtk_widget_set_size_request(GTK_WIDGET(label), 40, 36);
        strip->midi_in = label;
        gtk_widget_show_all(midiBox);
    } else {
        label = gtk_label_new("");
        gtk_table_attach(GTK_TABLE(strips_table), label, 2, 3, row, row + 1, GTK_FILL, GTK_EXPAND, 5, 3);
        gtk_widget_set_size_request(GTK_WIDGET(label), 40, 36);
        strip->midi_in = label;
        gtk_widget_show(strip->midi_in);
    }
    strip->midi_in = label;
    

    for (int i = 0; i < 2; i++)
        strip->audio_in[i] = strip->audio_out[i] = NULL;
    
    
    if (plugin->get_input_count() == 2) {
        
        GtkWidget *inBox  = gtk_vbox_new(FALSE, 1);
        
        gtk_box_pack_start(GTK_BOX(inBox), gtk_label_new("audio in"),TRUE, TRUE, 0);
        
        label = calf_vumeter_new();
        calf_vumeter_set_falloff(CALF_VUMETER(label), 2.5);
        calf_vumeter_set_hold(CALF_VUMETER(label), 1.5);
        gtk_box_pack_start(GTK_BOX(inBox), label,TRUE, TRUE, 0);
        strip->audio_in[0] = label;
        
        label = calf_vumeter_new();
        calf_vumeter_set_falloff(CALF_VUMETER(label), 2.5);
        calf_vumeter_set_hold(CALF_VUMETER(label), 1.5);
        gtk_box_pack_start(GTK_BOX(inBox), label,TRUE, TRUE, 0);
        strip->audio_in[1] = label;
        
        gtk_widget_show_all(inBox);
        gtk_table_attach(GTK_TABLE(strips_table), inBox, 3, 4, row, row + 1, GTK_FILL, GTK_SHRINK, 5, 3);
        
        gtk_widget_set_size_request(GTK_WIDGET(inBox), 160, -1);
    }

    if (plugin->get_output_count() == 2) {
        
        GtkWidget *outBox  = gtk_vbox_new(FALSE, 1);
        
        gtk_box_pack_start(GTK_BOX(outBox), gtk_label_new("audio out"),TRUE, TRUE, 0);
        
        label = calf_vumeter_new();
        calf_vumeter_set_falloff(CALF_VUMETER(label), 2.5);
        calf_vumeter_set_hold(CALF_VUMETER(label), 1.5);
        gtk_box_pack_start(GTK_BOX(outBox), label,TRUE, TRUE, 0);
        strip->audio_out[0] = label;
        
        label = calf_vumeter_new();
        calf_vumeter_set_falloff(CALF_VUMETER(label), 2.5);
        calf_vumeter_set_hold(CALF_VUMETER(label), 1.5);
        gtk_box_pack_start(GTK_BOX(outBox), label,TRUE, TRUE, 0);
        strip->audio_out[1] = label;
        
        gtk_widget_show_all(outBox);
        gtk_table_attach(GTK_TABLE(strips_table), outBox, 4, 5, row, row + 1, GTK_FILL, GTK_SHRINK, 5, 3);
        
        gtk_widget_set_size_request(GTK_WIDGET(outBox), 160, -1);
    }

    // other stuff bottom right
    GtkWidget *paramBox = gtk_hbox_new(TRUE, 10);
    
    gtk_box_pack_start(GTK_BOX(paramBox), gtk_label_new(NULL), TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(paramBox), gtk_label_new(NULL), TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(paramBox), gtk_label_new(NULL), TRUE, TRUE, 0);
    
    GtkWidget *logoImg     = gtk_image_new_from_file(PKGLIBDIR "/logo_button.png");
    gtk_box_pack_end(GTK_BOX(paramBox), GTK_WIDGET(logoImg), FALSE, FALSE, 0);
    
    gtk_table_attach(GTK_TABLE(strips_table), paramBox, 3, 5, row + 1, row + 2, (GtkAttachOptions)(GTK_EXPAND | GTK_FILL), (GtkAttachOptions)(GTK_EXPAND | GTK_FILL), 10, 0);
    gtk_widget_show_all(GTK_WIDGET(paramBox));
    
    row += 2;
    
    // bottom light
    GtkWidget *botImg     = gtk_image_new_from_file(PKGLIBDIR "/light_bottom.png");
    gtk_widget_set_size_request(GTK_WIDGET(botImg), 1, 1);
    gtk_table_attach(GTK_TABLE(strips_table), botImg, 1, 5, row, row + 1, (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), (GtkAttachOptions)(0), 0, 0);
    gtk_widget_show(botImg);
    
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
    gui_win->create(plugin, (prefix + plugin->get_label()).c_str(), plugin->get_id());
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
    std::vector<plugin_metadata_iface *> plugins;
    calf_plugins::get_all_plugins(plugins);
    for(unsigned int i = 0; i < plugins.size(); i++)
    {
        plugin_metadata_iface *p = plugins[i];
        string action_name = "Add" + string(p->get_id())+"Action";
        s += string("<menuitem action=\"") + action_name + "\" />";
        GtkActionEntry ae = { action_name.c_str(), NULL, p->get_label(), NULL, NULL, (GCallback)add_plugin_action };
        gtk_action_group_add_actions_full(actions, &ae, 1, (gpointer)new add_plugin_params(this, p->get_id()), action_destroy_notify);
        delete p;
    }
    plugins.clear();
    return s + plugin_post_xml;
}

void main_window::create()
{
    toplevel = GTK_WINDOW(gtk_window_new (GTK_WINDOW_TOPLEVEL));
    gtk_widget_set_name(GTK_WIDGET(toplevel), "Calf-Rack");
    gtk_window_set_default_icon_name("calf");
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
    
    strips_table = gtk_table_new(0, 6, FALSE);
    gtk_table_set_col_spacings(GTK_TABLE(strips_table), 0);
    gtk_table_set_row_spacings(GTK_TABLE(strips_table), 0);
    
//    gtk_table_attach(GTK_TABLE(strips_table), gtk_label_new(""), 0, 1, 0, 1, GTK_FILL, GTK_SHRINK, 28, 5);
//    gtk_table_attach(GTK_TABLE(strips_table), gtk_label_new("Module"), 1, 2, 0, 1, GTK_FILL, GTK_SHRINK, 80, 5);
//    gtk_table_attach(GTK_TABLE(strips_table), gtk_label_new("MIDI"), 2, 3, 0, 1, GTK_FILL, GTK_SHRINK, 15, 5);
//    gtk_table_attach(GTK_TABLE(strips_table), gtk_label_new("audio in"), 3, 4, 0, 1, GTK_FILL, GTK_SHRINK, 80, 5);
//    gtk_table_attach(GTK_TABLE(strips_table), gtk_label_new("audio out"), 4, 5, 0, 1, GTK_FILL, GTK_SHRINK, 80, 5);
//    gtk_table_attach(GTK_TABLE(strips_table), gtk_label_new(""), 5, 6, 0, 1, GTK_FILL, GTK_SHRINK, 28, 5);
    
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
    
    gtk_widget_set_name(GTK_WIDGET(strips_table), "Calf-Container");
    
    gtk_window_add_accel_group(toplevel, gtk_ui_manager_get_accel_group(ui_mgr));
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
                calf_led_set_value (CALF_LED (strip->midi_in), plugin->get_level(idx++));
            }
        }
    }
    return TRUE;
}

void main_window::open_file()
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
        const char *error = owner->open_file(filename);
        if (error) 
            display_error(error, filename);
        else
            current_filename = filename;
        g_free (filename);
    }
    gtk_widget_destroy (dialog);
}

void main_window::save_file()
{
    if (!current_filename.empty()) {
        const char *error = owner->save_file(current_filename.c_str());
        if (error)
            display_error(error, current_filename.c_str());
    }
    else
        save_file_as();
}

void main_window::save_file_as()
{
    GtkWidget *dialog;
    dialog = gtk_file_chooser_dialog_new ("Save File",
        toplevel,
        GTK_FILE_CHOOSER_ACTION_SAVE,
        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
        GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
        NULL);
    if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
    {
        char *filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
        const char *error = owner->save_file(filename);
        if (error) 
            display_error(error, filename);
        else
            current_filename = filename;
        g_free (filename);
    }
    gtk_widget_destroy (dialog);
}

void main_window::display_error(const char *error, const char *filename)
{
    GtkWidget *dialog;
    dialog = gtk_message_dialog_new_with_markup (toplevel, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, error, filename, NULL);
    gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);
}
