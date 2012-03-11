/* Calf DSP Library
 * Toplevel window that hosts the plugin GUI.
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
 
#include <calf/gui_config.h>
#include <calf/gui_controls.h>
#include <calf/preset.h>
#include <calf/preset_gui.h>
#include <gdk/gdk.h>

#include <iostream>

using namespace calf_plugins;
using namespace std;

/******************************* Actions **************************************************/
 
namespace {
    
void store_preset_action(GtkAction *action, plugin_gui_window *gui_win)
{
    if (gui_win->gui->preset_access)
        gui_win->gui->preset_access->store_preset();
}

struct activate_preset_params
{
    preset_access_iface *preset_access;
    int preset;
    bool builtin;
    
    activate_preset_params(preset_access_iface *_pai, int _preset, bool _builtin)
    : preset_access(_pai), preset(_preset), builtin(_builtin)
    {
    }
    static void action_destroy_notify(gpointer data)
    {
        delete (activate_preset_params *)data;
    }
};

void activate_preset(GtkAction *action, activate_preset_params *params)
{
    params->preset_access->activate_preset(params->preset, params->builtin);
}

void help_action(GtkAction *action, plugin_gui_window *gui_win)
{
    string uri = "file://" PKGDOCDIR "/" + string(gui_win->gui->plugin->get_metadata_iface()->get_label()) + ".html";
    GError *error = NULL;
    if (!gtk_show_uri(gtk_window_get_screen(gui_win->toplevel), uri.c_str(), time(NULL), &error))
    {
        GtkMessageDialog *dlg = GTK_MESSAGE_DIALOG(gtk_message_dialog_new(gui_win->toplevel, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_OTHER, GTK_BUTTONS_OK, "%s", error->message));
        if (!dlg)
            return;
        
        gtk_dialog_run(GTK_DIALOG(dlg));
        gtk_widget_destroy(GTK_WIDGET(dlg));
        
        g_error_free(error);
    }
}

void about_action(GtkAction *action, plugin_gui_window *gui_win)
{
    GtkAboutDialog *dlg = GTK_ABOUT_DIALOG(gtk_about_dialog_new());
    if (!dlg)
        return;
    
    static const char *artists[] = {
        "Markus Schmidt (GUI, icons)",
        "Thorsten Wilms (previous icon)",
        NULL
    };
    
    static const char *authors[] = {
        "Krzysztof Foltman <wdev@foltman.com>",
        "Hermann Meyer <brummer-@web.de>",
        "Thor Harald Johansen <thj@thj.no>",
        "Thorsten Wilms <t_w_@freenet.de>",
        "Hans Baier <hansfbaier@googlemail.com>",
        "Torben Hohn <torbenh@gmx.de>",
        "Markus Schmidt <schmidt@boomshop.net>",
        "Christian Holschuh <chrisch.holli@gmx.de>",
        "Tom Szilagyi <tomszilagyi@gmail.com>",
        "Damien Zammit <damien.zammit@gmail.com>",
        "David T\xC3\xA4ht <d@teklibre.com>",
        "Dave Robillard <dave@drobilla.net>",
        NULL
    };
    
    static const char translators[] = 
        "Russian: Alexandre Prokoudine <alexandre.prokoudine@gmail.com>\n"
    ;
    
    string label = gui_win->gui->plugin->get_metadata_iface()->get_label();
    gtk_about_dialog_set_name(dlg, ("About Calf " + label).c_str());
    gtk_about_dialog_set_program_name(dlg, ("Calf " + label).c_str());
    gtk_about_dialog_set_version(dlg, PACKAGE_VERSION);
    gtk_about_dialog_set_website(dlg, "http://calf.sourceforge.net/");
    gtk_about_dialog_set_copyright(dlg, "Copyright \xC2\xA9 2001-2011 Krzysztof Foltman, Thor Harald Johansen, Markus Schmidt and others.\nSee AUTHORS file for the full list.");
    gtk_about_dialog_set_logo_icon_name(dlg, "calf");
    gtk_about_dialog_set_artists(dlg, artists);
    gtk_about_dialog_set_authors(dlg, authors);
    gtk_about_dialog_set_translator_credits(dlg, translators);
    gtk_dialog_run(GTK_DIALOG(dlg));
    gtk_widget_destroy(GTK_WIDGET(dlg));
}

void tips_tricks_action(GtkAction *action, plugin_gui_window *gui_win)
{
    static const char tips_and_tricks[] = 
    "1. Knob control\n"
    "\n"
    "Use SHIFT-dragging for increased precision. Mouse wheel is also supported.\n"
    "\n"
    "2. Rack ears\n"
    "\n"
    "If you consider those a waste of screen space, you can turn them off in Preferences dialog in Calf JACK host. The setting affects all versions of the GUI (DSSI, LV2 GTK+, LV2 External, JACK host).\n"
    "\n"
    ;
    GtkMessageDialog *dlg = GTK_MESSAGE_DIALOG(gtk_message_dialog_new(gui_win->toplevel, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_OTHER, GTK_BUTTONS_OK, "%s", tips_and_tricks));
    if (!dlg)
        return;
    
    
    gtk_window_set_title(GTK_WINDOW(dlg), "Tips and Tricks");
    gtk_dialog_run(GTK_DIALOG(dlg));
    gtk_widget_destroy(GTK_WIDGET(dlg));
}

};

void calf_plugins::activate_command(GtkAction *action, activate_command_params *params)
{
    plugin_gui *gui = params->gui;
    gui->plugin->execute(params->function_idx);
    gui->refresh();
}


static const GtkActionEntry actions[] = {
    { "PresetMenuAction", NULL, "_Preset", NULL, "Preset operations", NULL },
    { "BuiltinPresetMenuAction", NULL, "_Built-in", NULL, "Built-in (factory) presets", NULL },
    { "UserPresetMenuAction", NULL, "_User", NULL, "User (your) presets", NULL },
    { "CommandMenuAction", NULL, "_Command", NULL, "Plugin-related commands", NULL },
    { "HelpMenuAction", NULL, "_Help", NULL, "Help-related commands", NULL },
    { "store-preset", "gtk-save-as", "Store preset", NULL, "Store a current setting as preset", (GCallback)store_preset_action },
    { "about", "gtk-about", "_About...", NULL, "About this plugin", (GCallback)about_action },
    { "HelpMenuItemAction", "gtk-help", "_Help", NULL, "Show manual page for this plugin", (GCallback)help_action },
    { "tips-tricks", NULL, "_Tips and tricks...", NULL, "Show a list of tips and tricks", (GCallback)tips_tricks_action },
};

/***************************** GUI window ********************************************/

static const char *ui_xml = 
"<ui>\n"
"  <menubar>\n"
"    <menu action=\"PresetMenuAction\">\n"
"      <menuitem action=\"store-preset\"/>\n"
"      <separator/>\n"
"      <placeholder name=\"builtin_presets\"/>\n"
"      <separator/>\n"
"      <placeholder name=\"user_presets\"/>\n"
"    </menu>\n"
"    <placeholder name=\"commands\"/>\n"
"    <menu action=\"HelpMenuAction\">\n"
"      <menuitem action=\"HelpMenuItemAction\"/>\n"
"      <menuitem action=\"tips-tricks\"/>\n"
"      <separator/>\n"
"      <menuitem action=\"about\"/>\n"
"    </menu>\n"
"  </menubar>\n"
"</ui>\n"
;

static const char *general_preset_pre_xml = 
"<ui>\n"
"  <menubar>\n"
"    <menu action=\"PresetMenuAction\">\n";

static const char *builtin_preset_pre_xml = 
"        <placeholder name=\"builtin_presets\">\n";

static const char *user_preset_pre_xml = 
"        <placeholder name=\"user_presets\">\n";

static const char *preset_post_xml = 
"        </placeholder>\n"
"    </menu>\n"
"  </menubar>\n"
"</ui>\n"
;

static const char *command_pre_xml = 
"<ui>\n"
"  <menubar>\n"
"    <placeholder name=\"commands\">\n"
"      <menu action=\"CommandMenuAction\">\n";

static const char *command_post_xml = 
"      </menu>\n"
"    </placeholder>\n"
"  </menubar>\n"
"</ui>\n"
;

plugin_gui_window::plugin_gui_window(gui_environment_iface *_env, main_window_iface *_main)
{
    toplevel = NULL;
    ui_mgr = NULL;
    std_actions = NULL;
    builtin_preset_actions = NULL;
    user_preset_actions = NULL;
    command_actions = NULL;
    environment = _env;
    notifier = NULL;
    main = _main;
    assert(environment);

}

void plugin_gui_window::on_window_destroyed(GtkWidget *window, gpointer data)
{
    delete (plugin_gui_window *)data;
}

string plugin_gui_window::make_gui_preset_list(GtkActionGroup *grp, bool builtin, char &ch)
{
    preset_access_iface *pai = gui->preset_access;
    string preset_xml = string(general_preset_pre_xml) + (builtin ? builtin_preset_pre_xml : user_preset_pre_xml);
    preset_vector &pvec = (builtin ? get_builtin_presets() : get_user_presets()).presets;
    GtkActionGroup *preset_actions = builtin ? builtin_preset_actions : user_preset_actions;
    for (unsigned int i = 0; i < pvec.size(); i++)
    {
        if (pvec[i].plugin != gui->effect_name)
            continue;
        stringstream ss;
        ss << (builtin ? "builtin_preset" : "user_preset") << i;
        preset_xml += "          <menuitem name=\"" + pvec[i].name+"\" action=\""+ss.str()+"\"/>\n";
        if (ch != ' ' && ++ch == ':')
            ch = 'A';
        if (ch > 'Z')
            ch = ' ';

        string sv = ss.str();
        string prefix = ch == ' ' ? string() : string("_")+ch+" ";
        string name = prefix + pvec[i].name;
        GtkActionEntry ae = { sv.c_str(), NULL, name.c_str(), NULL, NULL, (GCallback)activate_preset };
        gtk_action_group_add_actions_full(preset_actions, &ae, 1, (gpointer)new activate_preset_params(pai, i, builtin), activate_preset_params::action_destroy_notify);
    }
    preset_xml += preset_post_xml;
    return preset_xml;
}

string plugin_gui_window::make_gui_command_list(GtkActionGroup *grp)
{
    string command_xml = command_pre_xml;
    plugin_command_info *ci = gui->plugin->get_metadata_iface()->get_commands();
    if (!ci)
        return "";
    for(int i = 0; ci->name; i++, ci++)
    {
        stringstream ss;
        ss << "          <menuitem name=\"" << ci->name << "\" action=\"" << ci->label << "\"/>\n";
        
        GtkActionEntry ae = { ci->label, NULL, ci->name, NULL, ci->description, (GCallback)activate_command };
        gtk_action_group_add_actions_full(command_actions, &ae, 1, (gpointer)new activate_command_params(gui, i), activate_preset_params::action_destroy_notify);
        command_xml += ss.str();
    }
    command_xml += command_post_xml;
    return command_xml;
}

void plugin_gui_window::fill_gui_presets(bool builtin, char &ch)
{
    GtkActionGroup *&preset_actions = builtin ? builtin_preset_actions : user_preset_actions;
    if(preset_actions) {
        gtk_ui_manager_remove_action_group(ui_mgr, preset_actions);
        preset_actions = NULL;
    }
    
    if (builtin)
        builtin_preset_actions = gtk_action_group_new("builtin_presets");
    else
        user_preset_actions = gtk_action_group_new("user_presets");
    string preset_xml = make_gui_preset_list(preset_actions, builtin, ch);
    gtk_ui_manager_insert_action_group(ui_mgr, preset_actions, 0);    
    GError *error = NULL;
    gtk_ui_manager_add_ui_from_string(ui_mgr, preset_xml.c_str(), -1, &error);
}

gboolean plugin_gui_window::on_idle(void *data)
{
    plugin_gui_window *self = (plugin_gui_window *)data;
    self->gui->on_idle();
    return TRUE;
}

void plugin_gui_window::create(plugin_ctl_iface *_jh, const char *title, const char *effect)
{
    toplevel = GTK_WINDOW(gtk_window_new (GTK_WINDOW_TOPLEVEL));
    gtk_window_set_icon_name(toplevel, "calf_plugin");
    gtk_window_set_type_hint(toplevel, GDK_WINDOW_TYPE_HINT_DIALOG);
    gtk_window_set_role(toplevel, "plugin_ui");
    GtkVBox *vbox = GTK_VBOX(gtk_vbox_new(false, 0));
    gtk_widget_set_name(GTK_WIDGET(vbox), "Calf-Plugin");
    
    GtkRequisition req, req2;
    gtk_window_set_title(GTK_WINDOW (toplevel), title);
    gtk_container_add(GTK_CONTAINER(toplevel), GTK_WIDGET(vbox));

    gui = new plugin_gui(this);
    gui->effect_name = effect;

    ui_mgr = gtk_ui_manager_new();
    std_actions = gtk_action_group_new("default");
    gtk_action_group_add_actions(std_actions, actions, sizeof(actions)/sizeof(actions[0]), this);
    GError *error = NULL;
    gtk_ui_manager_insert_action_group(ui_mgr, std_actions, 0);
    gtk_ui_manager_add_ui_from_string(ui_mgr, ui_xml, -1, &error);    
    
    command_actions = gtk_action_group_new("commands");

    char ch = '0';
    fill_gui_presets(true, ch);
    fill_gui_presets(false, ch);
    
    gtk_box_pack_start(GTK_BOX(vbox), gtk_ui_manager_get_widget(ui_mgr, "/ui/menubar"), false, false, 0);
    gtk_widget_set_name(GTK_WIDGET(gtk_ui_manager_get_widget(ui_mgr, "/ui/menubar")), "Calf-Menu");
    
    // determine size without content
    gtk_widget_show_all(GTK_WIDGET(vbox));
    gtk_widget_size_request(GTK_WIDGET(vbox), &req2);
    // printf("size request %dx%d\n", req2.width, req2.height);
    
    GtkWidget *container;
    const char *xml = _jh->get_metadata_iface()->get_gui_xml();
    assert(xml);
    container = gui->create_from_xml(_jh, xml);
    
    GtkWidget *sw = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw), GTK_SHADOW_NONE);
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(sw), GTK_WIDGET(container));
    gtk_widget_set_name(GTK_WIDGET(sw), "Calf-Container");
    gtk_box_pack_start(GTK_BOX(vbox), sw, true, true, 0);
    
    gtk_widget_show_all(GTK_WIDGET(sw));
    
    gui->show_rack_ears(environment->get_config()->rack_ears);
    
    gtk_widget_size_request(GTK_WIDGET(container), &req);
    int wx = max(req.width + 10, req2.width);
    int wy = req.height + req2.height + 10;
    // printf("size request %dx%d\n", req.width, req.height);
    // printf("resize to %dx%d\n", max(req.width + 10, req2.width), req.height + req2.height + 10);
    gtk_window_set_default_size(GTK_WINDOW(toplevel), wx, wy);
    gtk_window_resize(GTK_WINDOW(toplevel), wx, wy);
    //gtk_widget_set_size_request(GTK_WIDGET(toplevel), max(req.width + 10, req2.width), req.height + req2.height + 10);
    // printf("size set %dx%d\n", wx, wy);
    // gtk_scrolled_window_set_vadjustment(GTK_SCROLLED_WINDOW(sw), GTK_ADJUSTMENT(gtk_adjustment_new(0, 0, req.height, 20, 100, 100)));
    gtk_signal_connect (GTK_OBJECT (toplevel), "destroy", G_CALLBACK (on_window_destroyed), (plugin_gui_window *)this);
    if (main)
        main->set_window(gui->plugin, this);

    source_id = g_timeout_add_full(G_PRIORITY_DEFAULT, 1000/30, on_idle, this, NULL); // 30 fps should be enough for everybody
    gtk_ui_manager_ensure_update(ui_mgr);
    gui->plugin->send_configures(gui);
    
    notifier = environment->get_config_db()->add_listener(this);
}

void plugin_gui_window::on_config_change()
{
    environment->get_config()->load(environment->get_config_db());
    gui->show_rack_ears(environment->get_config()->rack_ears);
}

void plugin_gui_window::cleanup()
{
    if (notifier)
    {
        delete notifier;
        notifier = NULL;
    }
    if (source_id)
        g_source_remove(source_id);
    source_id = 0;
}

void plugin_gui_window::close()
{
    cleanup();
    gtk_widget_destroy(GTK_WIDGET(toplevel));
}

plugin_gui_window::~plugin_gui_window()
{
    if (main)
        main->set_window(gui->plugin, NULL);
    cleanup();
    delete gui;
}

