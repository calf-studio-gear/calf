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

/***************************** GUI widget ********************************************/

plugin_gui_widget::plugin_gui_widget(gui_environment_iface *_env, main_window_iface *_main)
{
    gui = NULL;
    toplevel = NULL;
    environment = _env;
    main = _main;
    assert(environment);
    prefix = "strips";
}

void plugin_gui_widget::on_window_destroyed(GtkWidget *window, gpointer data)
{
    plugin_gui_widget *self = (plugin_gui_widget *)data;
    self->gui->destroy_child_widgets(self->toplevel);
    delete self;
}

gboolean plugin_gui_widget::on_idle(void *data)
{
    plugin_gui_widget *self = (plugin_gui_widget *)data;
    //if (!self->refresh_controller.check_redraw(GTK_WIDGET(self->toplevel)))
        //return TRUE;
    self->gui->on_idle();
    return TRUE;
}

void plugin_gui_widget::create_gui(plugin_ctl_iface *_jh)
{
    gui = new plugin_gui(this);
    const char *xml = _jh->get_metadata_iface()->get_gui_xml(prefix.c_str());
    if (!xml) {
        xml = "<hbox />";
    }
    container = gui->create_from_xml(_jh, xml);
    source_id = g_timeout_add_full(G_PRIORITY_DEFAULT, 1000/30, on_idle, this, NULL); // 30 fps should be enough for everybody
    gui->plugin->send_configures(gui);
}

GtkWidget *plugin_gui_widget::create(plugin_ctl_iface *_jh)
{
    create_gui(_jh);
    gtk_widget_set_name(container, "Calf-Plugin-Strip");
    gtk_widget_show_all(container);
    toplevel = container;
    g_signal_connect (GTK_WIDGET(toplevel), "destroy", G_CALLBACK (on_window_destroyed), (plugin_gui_widget *)this);
    return container;
}

void plugin_gui_widget::refresh()
{
    if (gui)
        gui->refresh();
}

void plugin_gui_widget::cleanup()
{
    if (source_id)
        g_source_remove(source_id);
    source_id = 0;
}

plugin_gui_widget::~plugin_gui_widget()
{
    cleanup();
    delete gui;
    gui = NULL;
}

/******************************* Actions **************************************************/
 
void plugin_gui_window::store_preset_action(GtkAction *action, plugin_gui_window *gui_win)
{
    if (gui_win->gui->preset_access)
        gui_win->gui->preset_access->store_preset();
}

namespace {

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

}

void plugin_gui_window::help_action(GtkAction *action, plugin_gui_window *gui_win)
{
    string uri = "file://" PKGDOCDIR "/" + string(gui_win->gui->plugin->get_metadata_iface()->get_label()) + ".html";
    GError *error = NULL;
    if (!gtk_show_uri(gtk_window_get_screen(GTK_WINDOW(gui_win->toplevel)), uri.c_str(), time(NULL), &error))
    {
        GtkMessageDialog *dlg = GTK_MESSAGE_DIALOG(gtk_message_dialog_new(GTK_WINDOW(gui_win->toplevel), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_OTHER, GTK_BUTTONS_OK, "%s", error->message));
        if (!dlg)
            return;
        
        gtk_dialog_run(GTK_DIALOG(dlg));
        gtk_widget_destroy(GTK_WIDGET(dlg));
        
        g_error_free(error);
    }
}

void plugin_gui_window::about_action(GtkAction *action, plugin_gui_window *gui_win)
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
        "Damien Zammit <damien@zamaudio.com>",
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
    "1. Knob and Fader Control\n"
    "\n"
    "* Use SHIFT-dragging for increased precision\n"
    "* Mouse wheel is also supported\n"
    "* Middle click opens a text entry\n"
    "* Right click a knob to assign a MIDI controller\n"
    "\n"
    "2. Rack Ears\n"
    "\n"
    "If you consider those a waste of screen space, you can turn them off in Preferences dialog in Calf JACK host. The setting affects all versions of the GUI (LV2 GTK+, LV2 External, JACK host).\n"
    "\n"
    ;
    GtkMessageDialog *dlg = GTK_MESSAGE_DIALOG(gtk_message_dialog_new(GTK_WINDOW(gui_win->toplevel), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_OTHER, GTK_BUTTONS_OK, "%s", tips_and_tricks));
    if (!dlg)
        return;
    
    
    gtk_window_set_title(GTK_WINDOW(dlg), "Tips and Tricks");
    gtk_dialog_run(GTK_DIALOG(dlg));
    gtk_widget_destroy(GTK_WIDGET(dlg));
}

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
    { "CommandMenuAction", NULL, "_Commands", NULL, "Plugin-related commands", NULL },
    { "HelpMenuAction", NULL, "_Help", NULL, "Help-related commands", NULL },
    { "store-preset", "gtk-save-as", "Store preset", NULL, "Store a current setting as preset", (GCallback)plugin_gui_window::store_preset_action },
    { "about", "gtk-about", "_About...", NULL, "About this plugin", (GCallback)plugin_gui_window::about_action },
    { "HelpMenuItemAction", "gtk-help", "_Help", NULL, "Show manual page for this plugin", (GCallback)plugin_gui_window::help_action },
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
: plugin_gui_widget(_env, _main)
{
    ui_mgr = NULL;
    std_actions = NULL;
    builtin_preset_actions = NULL;
    user_preset_actions = NULL;
    command_actions = NULL;
    notifier = NULL;
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
        GtkActionEntry ae = { sv.c_str(), NULL, name.c_str(), NULL, NULL, (GCallback)::activate_preset };
        gtk_action_group_add_actions_full(preset_actions, &ae, 1, (gpointer)new activate_preset_params(pai, i, builtin), activate_preset_params::action_destroy_notify);
    }
    preset_xml += preset_post_xml;
    return preset_xml;
}

string plugin_gui_window::make_gui_command_list(GtkActionGroup *grp, const plugin_metadata_iface *metadata)
{
    string command_xml = command_pre_xml;
    plugin_command_info *ci = metadata->get_commands();
    if (!ci)
        return "";
    for(int i = 0; ci->name; i++, ci++)
    {
        stringstream ss;
        ss << "          <menuitem name=\"" << ci->name << "\" action=\"" << ci->label << "\"/>\n";
        
        GtkActionEntry ae = { ci->label, NULL, ci->name, NULL, ci->description, (GCallback)activate_command };
        gtk_action_group_add_actions_full(grp, &ae, 1, (gpointer)new activate_command_params(gui, i), activate_preset_params::action_destroy_notify);
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

void plugin_gui_window::create(plugin_ctl_iface *_jh, const char *title, const char *effect)
{
    prefix = "gui";
    GtkWidget *win = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_icon_name(GTK_WINDOW(win), "calf_plugin");
    gtk_window_set_type_hint(GTK_WINDOW(win), GDK_WINDOW_TYPE_HINT_NORMAL);
    gtk_window_set_role(GTK_WINDOW(win), "calf_plugin");
    
    GtkVBox *vbox = GTK_VBOX(gtk_vbox_new(false, 0));
    
    GtkRequisition req, req2;
    gtk_window_set_title(GTK_WINDOW (win), title);
    gtk_container_add(GTK_CONTAINER(win), GTK_WIDGET(vbox));
    
    create_gui(_jh);

    gui->effect_name = effect;
    gtk_widget_set_name(GTK_WIDGET(vbox), "Calf-Plugin");
    GtkWidget *decoTable = decorate(container);
    
    GtkWidget *eventbox  = gtk_event_box_new();
    gtk_widget_set_name( GTK_WIDGET(eventbox), "Calf-Plugin" );
    gtk_container_add( GTK_CONTAINER(eventbox), decoTable );
    
    gtk_widget_show(eventbox);
    
    ui_mgr = gtk_ui_manager_new();
    std_actions = gtk_action_group_new("default");
    gtk_action_group_add_actions(std_actions, actions, sizeof(actions)/sizeof(actions[0]), this);
    GError *error = NULL;
    gtk_ui_manager_insert_action_group(ui_mgr, std_actions, 0);
    gtk_ui_manager_add_ui_from_string(ui_mgr, ui_xml, -1, &error);
    
    command_actions = gtk_action_group_new("commands");
    string command_xml = make_gui_command_list(command_actions, _jh->get_metadata_iface());
    gtk_ui_manager_insert_action_group(ui_mgr, command_actions, 0);
    gtk_ui_manager_add_ui_from_string(ui_mgr, command_xml.c_str(), -1, &error);

    char ch = '0';
    fill_gui_presets(true, ch);
    fill_gui_presets(false, ch);
    
    gtk_box_pack_start(GTK_BOX(vbox), gtk_ui_manager_get_widget(ui_mgr, "/ui/menubar"), false, false, 0);
    gtk_widget_set_name(GTK_WIDGET(gtk_ui_manager_get_widget(ui_mgr, "/ui/menubar")), "Calf-Menu");
    
    // determine size without content
    gtk_widget_show_all(GTK_WIDGET(vbox));
    gtk_widget_size_request(GTK_WIDGET(vbox), &req2);
    
    GtkWidget *sw = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_show(GTK_WIDGET(sw));
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw), GTK_SHADOW_NONE);
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(sw), GTK_WIDGET(eventbox));
    gtk_widget_set_name(GTK_WIDGET(sw), "Calf-Container");
    gtk_box_pack_start(GTK_BOX(vbox), sw, true, true, 0);
    
    show_rack_ears(environment->get_config()->rack_ears);
    
    gtk_widget_size_request(GTK_WIDGET(container), &req);
    int wx = max(req.width + 10, req2.width);
    int wy = req.height + req2.height + 10;
    gtk_window_set_default_size(GTK_WINDOW(win), wx, wy);
    gtk_window_resize(GTK_WINDOW(win), wx, wy);
    g_signal_connect (GTK_WIDGET(win), "destroy", G_CALLBACK (on_window_destroyed), (plugin_gui_widget *)this);
    if (main)
        main->set_window(gui->plugin, this);

    gtk_ui_manager_ensure_update(ui_mgr);
    toplevel = win;
    notifier = environment->get_config_db()->add_listener(this);
}

void plugin_gui_window::on_config_change()
{
    environment->get_config()->load(environment->get_config_db());
    show_rack_ears(environment->get_config()->rack_ears);
}

void plugin_gui_window::close()
{
    gtk_widget_destroy(toplevel);
}

GtkWidget *plugin_gui_window::decorate(GtkWidget *widget) {
    GtkWidget *decoTable = gtk_table_new(3, 1, FALSE);
    
    // images for left side
    GtkWidget *nwImg     = gtk_image_new_from_pixbuf(environment->get_image_factory()->get("side_nw"));
    GtkWidget *swImg     = gtk_image_new_from_pixbuf(environment->get_image_factory()->get("side_sw"));
    
    // images for right side
    GtkWidget *neImg     = gtk_image_new_from_pixbuf(environment->get_image_factory()->get("side_ne"));
    GtkWidget *seImg     = gtk_image_new_from_pixbuf(environment->get_image_factory()->get("side_se"));
    
    // pack left box
    leftBG = gtk_event_box_new();
    GtkWidget *leftBox = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(leftBG), leftBox);
    gtk_box_pack_start(GTK_BOX(leftBox), GTK_WIDGET(nwImg), FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(leftBox), GTK_WIDGET(swImg), FALSE, FALSE, 0);
    gtk_widget_set_name(leftBG, "CalfPluginLeft");
    
    // pack right box
    rightBG  = gtk_event_box_new();
    GtkWidget *rightBox = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(rightBG), rightBox);
    gtk_box_pack_start(GTK_BOX(rightBox), GTK_WIDGET(neImg), FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(rightBox), GTK_WIDGET(seImg), FALSE, FALSE, 0);
    gtk_widget_set_name(rightBG, "CalfPluginRight");
    
    //gtk_table_attach(GTK_TABLE(decoTable), GTK_WIDGET(bgImg),     0, 2, 0, 2, (GtkAttachOptions)(GTK_EXPAND | GTK_SHRINK | GTK_FILL), (GtkAttachOptions)(GTK_EXPAND | GTK_SHRINK | GTK_FILL), 0, 0);
    gtk_table_attach(GTK_TABLE(decoTable), GTK_WIDGET(leftBG),   0, 1, 0, 1, (GtkAttachOptions)(0), (GtkAttachOptions)(GTK_EXPAND | GTK_FILL), 0, 0);
    gtk_table_attach(GTK_TABLE(decoTable), GTK_WIDGET(rightBG),  2, 3, 0, 1, (GtkAttachOptions)(0), (GtkAttachOptions)(GTK_EXPAND | GTK_FILL), 0, 0);
        
    gtk_table_attach(GTK_TABLE(decoTable), widget, 1, 2, 0, 1, (GtkAttachOptions)(GTK_EXPAND | GTK_FILL), (GtkAttachOptions)(GTK_EXPAND | GTK_FILL), 15, 5);
    
    gtk_widget_show_all(decoTable);
    return GTK_WIDGET(decoTable);
}


void plugin_gui_window::show_rack_ears(bool show)
{
    // if hidden, add a no-show-all attribute so that LV2 host doesn't accidentally override
    // the setting by doing a show_all on the outermost container
    gtk_widget_set_no_show_all(leftBG, !show);
    gtk_widget_set_no_show_all(rightBG, !show);
    if (show)
    {
        gtk_widget_show(leftBG);
        gtk_widget_show(rightBG);
    }
    else
    {
        gtk_widget_hide(leftBG);
        gtk_widget_hide(rightBG);
    }
}
plugin_gui_window::~plugin_gui_window()
{
    if (notifier)
    {
        delete notifier;
        notifier = NULL;
    }
    if (main)
        main->set_window(gui->plugin, NULL);
}
