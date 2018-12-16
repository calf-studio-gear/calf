/* Calf DSP Library
 * Universal GUI module
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
 * Boston, MA 02111-1307, USA.
 */
#ifndef __CALF_GUI_H
#define __CALF_GUI_H

#include <expat.h>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <gtk/gtk.h>
#include "giface.h"
#include "gui_config.h"
#include "jackhost.h"

namespace calf_plugins {

class window_update_controller
{
    int refresh_counter;
public:
    window_update_controller() : refresh_counter() {}
    bool check_redraw(GtkWidget *toplevel);
};


struct image_factory
{
    std::string path;
    std::map<std::string, GdkPixbuf*> i;
    GdkPixbuf *create_image (std::string image);
    void recreate_images ();
    void set_path (std::string p);
    GdkPixbuf *get (std::string image);
    gboolean available (std::string image);
    image_factory (std::string p = "");
    ~image_factory();
};

class plugin_gui;
class jack_host;

class control_base
{
public:
    typedef std::map<std::string, std::string> xml_attribute_map;

    GtkWidget *widget;
    std::string control_name;
    xml_attribute_map attribs;
    plugin_gui *gui;
public:
    void require_attribute(const char *name);
    void require_int_attribute(const char *name);
    int get_int(const char *name, int def_value = 0);
    float get_float(const char *name, float def_value = 0.f);
    std::vector<double> get_vector(const char *name, std::string &value);

public:
    virtual GtkWidget *create(plugin_gui *_gui) { return NULL; }
    virtual bool is_container() { return GTK_IS_CONTAINER(widget); };
    virtual void set_visibilty(bool state);
    virtual void add(control_base *ctl) { gtk_container_add(GTK_CONTAINER(widget), ctl->widget); }
    /// called from created() to set all the properties
    virtual void set_std_properties();
    /// called after the control is created
    virtual void created();
    virtual ~control_base() {}
};

#define _GUARD_CHANGE_ if (in_change) return; guard_change __gc__(this);

class param_control: public control_base
{    
protected:
    GtkWidget *entrywin;
public:
    int param_no;
    std::string param_variable;
    int in_change;
    bool has_entry;
    float old_displayed_value;
    
    struct guard_change {
        param_control *pc;
        guard_change(param_control *_pc) : pc(_pc) { pc->in_change++; }
        ~guard_change() { pc->in_change--; }
    };
    
    param_control();
    inline const parameter_properties &get_props();
    
    virtual GtkWidget *create(plugin_gui *_gui);
    /// called to create a widget for a control
    virtual GtkWidget *create(plugin_gui *_gui, int _param_no)=0;
    /// called to transfer the value from control to parameter(s)
    virtual void get()=0;
    /// called to transfer the value from parameter(s) to control
    virtual void set()=0;
    /// called after the control is created
    virtual void created();
    /// called on DSSI configure()
    virtual void configure(const char *key, const char *value) {}
    /// called from created() to add hooks for parameters
    virtual void hook_params();
    /// called from created() to add context menu handlers
    virtual void add_context_menu_handler();
    virtual void on_idle() {}
    virtual ~param_control();
    virtual void do_popup_menu();
    static gboolean on_button_press_event(GtkWidget *widget, GdkEventButton *event, void *user_data);
    static gboolean on_popup_menu(GtkWidget *widget, void *user_data);
    virtual void create_value_entry(GtkWidget *widget, int x, int y);
    virtual void destroy_value_entry();
    static gboolean value_entry_action(GtkEntry *widget, GdkEvent *event, void *user_data);
    static gboolean value_entry_unfocus(GtkWidget *widget, GdkEventFocus *event, void *user_data);
    static gboolean value_entry_click(GtkWidget *widget, GdkEventButton *event, void *user_data);
};


class plugin_gui_widget;
class plugin_gui_window;

class plugin_gui: public send_configure_iface, public send_updates_iface
{
protected:
    int param_count;
    std::multimap<int, param_control *> par2ctl;
    XML_Parser parser;
    control_base *top_container;
    std::map<std::string, int> param_name_map;
    int ignore_stack;
    int last_status_serial_no;
    std::map<int, GSList *> param_radio_groups;
    int context_menu_param_no;
    uint32_t context_menu_last_designator;
    std::vector<control_base *> stack;

    struct automation_menu_entry {
        plugin_gui *gui;
        int source;
        automation_menu_entry(plugin_gui *_gui, int _source)
        : gui(_gui), source(_source) {}
    };
    std::vector<automation_menu_entry *> automation_menu_callback_data;

    static void on_automation_add(GtkWidget *widget, void *user_data);
    static void on_automation_delete(GtkWidget *widget, void *user_data);
    static void on_automation_set_lower(GtkWidget *widget, void *user_data);
    static void on_automation_set_upper(GtkWidget *widget, void *user_data);
    void on_automation_set_lower_or_upper(automation_menu_entry *ame, bool is_upper);
public:
    plugin_gui_widget *window;
    GtkWidget *container;
    const char *effect_name;
    plugin_ctl_iface *plugin;
    preset_access_iface *preset_access;
    std::vector<param_control *> params;
    std::vector<int> read_serials;
    
    /* For optional lv2ui:show interface. */
    bool optclosed;
    GtkWidget* optwidget;
    GtkWidget* optwindow;
    const char* opttitle;

    plugin_gui(plugin_gui_widget *_window);
    GtkWidget *create_from_xml(plugin_ctl_iface *_plugin, const char *xml);
    control_base *create_widget_from_xml(const char *element, const char *attributes[]);

    void add_param_ctl(int param, param_control *ctl) { par2ctl.insert(std::pair<int, param_control *>(param, ctl)); }
    void remove_param_ctl(int param, param_control *ctl);
    void refresh();
    void refresh(int param_no, param_control *originator = NULL);
    int get_param_no_by_name(std::string param_name);
    void xml_element_start(const char *element, const char *attributes[]);
    void set_param_value(int param_no, float value, param_control *originator = NULL);
    /// Called on change of configure variable
    void send_configure(const char *key, const char *value);
    /// Called on change of status variable
    void send_status(const char *key, const char *value);
    void on_idle();
    /// Get a radio button group (if it exists) for a parameter
    GSList *get_radio_group(int param);
    /// Set a radio button group for a parameter
    void set_radio_group(int param, GSList *group);
    /// Pop-up a context menu for a control
    void on_control_popup(param_control *ctl, int param_no);
    /// Clean up callback data allocated for the automation pop-up menu
    void cleanup_automation_entries();
    /// Destroy all the widgets in the container
    void destroy_child_widgets(GtkWidget *parent);
    ~plugin_gui();
    static void xml_element_start(void *data, const char *element, const char *attributes[]);
    static void xml_element_end(void *data, const char *element);
};

struct main_window_iface;
struct main_window_owner_iface;

/// A class used to inform the plugin GUIs about the environment they run in
/// (currently: what plugin features are accessible)
struct gui_environment_iface
{
    virtual bool check_condition(const char *name) = 0;
    virtual calf_utils::config_db_iface *get_config_db() = 0;
    virtual calf_utils::gui_config *get_config() = 0;
    virtual calf_plugins::image_factory *get_image_factory() = 0;
    virtual ~gui_environment_iface() {}
};

/// An interface that wraps UI-dependent elements of the session
struct session_environment_iface
{
    /// Called to initialise the UI libraries
    virtual void init_gui(int &argc, char **&argv) = 0;
    /// Create an appropriate version of the main window
    virtual main_window_iface *create_main_window() = 0;
    /// Called to start the UI loop
    virtual void start_gui_loop() = 0;
    /// Called from within event handlers to finish the UI loop
    virtual void quit_gui_loop() = 0;
    virtual ~session_environment_iface() {}
};

/// Trivial implementation of gui_environment_iface
class gui_environment: public gui_environment_iface
{
private:
    GKeyFile *keyfile;
    calf_utils::config_db_iface *config_db;
    calf_utils::gui_config gui_config;
public:
    std::set<std::string> conditions;
    image_factory images;
    gui_environment();
    virtual bool check_condition(const char *name) { return conditions.count(name) != 0; }
    virtual calf_utils::config_db_iface *get_config_db() { return config_db; }
    virtual calf_utils::gui_config *get_config() { return &gui_config; }
    virtual calf_plugins::image_factory *get_image_factory() { return &images; }
    ~gui_environment();
};

/// Interface used by the plugin to communicate with the main hosting window
struct main_window_iface: public progress_report_iface
{
    /// Set owner pointer
    virtual void set_owner(main_window_owner_iface *owner) = 0;
    /// Add a condition to the list of conditions supported by the host
    virtual void add_condition(const std::string &name) = 0;
    /// Create the actual window associated with this interface
    virtual void create() = 0;
    /// Create the actual window associated with this interface
    virtual void create_status_icon() = 0;
    /// Add the plugin to the window
    virtual void add_plugin(jack_host *plugin) = 0;
    /// Remove the plugin from the window
    virtual void del_plugin(plugin_ctl_iface *plugin) = 0;
    // Rename the plugin
    virtual void rename_plugin(plugin_ctl_iface *plugin, std::string name) = 0;
    /// Refresh the plugin UI
    virtual void refresh_plugin(plugin_ctl_iface *plugin) = 0;
    /// Refresh the plugin UI
    virtual void refresh_plugin_param(plugin_ctl_iface *plugin, int param_no) = 0;
    /// Bind the plugin window to the plugin
    virtual void set_window(plugin_ctl_iface *plugin, plugin_gui_window *window) = 0;
    /// Refresh preset lists on all windows (if, for example, a new preset has been created)
    virtual void refresh_all_presets(bool builtin_too) = 0;
    /// Default open file operation
    virtual void open_file() = 0;
    /// Default save file operation
    virtual bool save_file() = 0;
    /// Called to clean up when host quits
    virtual void on_closed() = 0;
    /// Show an error message
    virtual void show_error(const std::string &text) = 0;
    
    
    virtual ~main_window_iface() {}
};

struct main_window_owner_iface
{
    virtual void new_plugin(const char *name) = 0;
    virtual void remove_plugin(plugin_ctl_iface *plugin) = 0;
    virtual char *open_file(const char *name) = 0;
    virtual char *save_file(const char *name) = 0;
    virtual void reorder_plugins() = 0;
    virtual void rename_plugin(plugin_ctl_iface *plugin, const char *name) = 0;
    /// Return JACK client name (or its counterpart) to put in window title bars
    virtual std::string get_client_name() const = 0;
    /// Called on 'destroy' event of the main window
    virtual void on_main_window_destroy() = 0;
    /// Called from idle handler
    virtual void on_idle() = 0;
    /// Get the file name of the current rack
    virtual std::string get_current_filename() const = 0;    
    /// Set the file name of the current rack
    virtual void set_current_filename(const std::string &name) = 0;    
    virtual ~main_window_owner_iface() {}
};

struct window_state {
    GdkScreen *screen;
    int x, y, width, height;
};

class plugin_gui_widget: public calf_utils::config_listener_iface
{
private:
    window_update_controller refresh_controller;
    int source_id;
private:
    static gboolean on_idle(void *data);
protected:
    void create_gui(plugin_ctl_iface *_jh);
    static void on_window_destroyed(GtkWidget *window, gpointer data);
    void cleanup();
protected:
    plugin_gui *gui;
    GtkWidget *container;
    gui_environment_iface *environment;
    main_window_iface *main;
public:
    std::string prefix;
    GtkWidget *toplevel;
    window_state winstate;
public:
    plugin_gui_widget(gui_environment_iface *_env, main_window_iface *_main);
    GtkWidget *create(plugin_ctl_iface *_plugin);
    gui_environment_iface *get_environment() { return environment; }
    main_window_iface *get_main_window() { return main; }
    plugin_gui *get_gui() { return gui; }
    void refresh();
    virtual void on_config_change() { }
    ~plugin_gui_widget();
};

class plugin_gui_window: public plugin_gui_widget
{
public:
    GtkUIManager *ui_mgr;
    GtkActionGroup *std_actions, *builtin_preset_actions, *user_preset_actions, *command_actions;
    calf_utils::config_notifier_iface *notifier;
    GtkWidget *leftBG, *rightBG;
    plugin_gui_window(gui_environment_iface *_env, main_window_iface *_main);
    std::string make_gui_preset_list(GtkActionGroup *grp, bool builtin, char &ch);
    std::string make_gui_command_list(GtkActionGroup *grp, const plugin_metadata_iface *metadata);
    void fill_gui_presets(bool builtin, char &ch);
    void create(plugin_ctl_iface *_plugin, const char *title, const char *effect);
    GtkWidget *decorate(GtkWidget *widget);
    void show_rack_ears(bool show);
    void close();
    virtual void on_config_change();
    ~plugin_gui_window();
    static void about_action(GtkAction *action, plugin_gui_window *gui_win);
    static void help_action(GtkAction *action, plugin_gui_window *gui_win);
    static void store_preset_action(GtkAction *action, plugin_gui_window *gui_win);

};


inline const parameter_properties &param_control::get_props() 
{ 
    return  *gui->plugin->get_metadata_iface()->get_param_props(param_no);
}

class null_audio_module;

struct activate_command_params
{
    typedef void (*CommandFunc)(null_audio_module *);
    plugin_gui *gui;
    int function_idx;
    
    activate_command_params(plugin_gui *_gui, int _idx)
    : gui(_gui), function_idx(_idx)
    {
    }
};

void activate_command(GtkAction *action, activate_command_params *params);

class cairo_impl: public calf_plugins::cairo_iface
{
public:
    cairo_t *context;
    virtual void set_source_rgba(float r, float g, float b, float a = 1.f) { cairo_set_source_rgba(context, r, g, b, a); }
    virtual void set_line_width(float width) { cairo_set_line_width(context, width); }
    virtual void set_dash(const double *dash, int length) { cairo_set_dash(context, dash, length, 0); }
    virtual void draw_label(const char *label, float x, float y, int pos, float margin, float align) {
        cairo_text_extents_t extents;
        cairo_text_extents(context, label, &extents);
        switch(pos) {
            case 0:
            default:
                // top
                cairo_move_to(context, x - extents.width / 2, y - margin);
                break;
            case 1:
                // right
                cairo_move_to(context, x + margin, y + 2);
                break;
            case 2:
                // bottom
                cairo_move_to(context, x - extents.width / 2, y + margin + extents.height);
                break;
            case 3:
                // left
                cairo_move_to(context, x - margin - extents.width, y + 2);
                break;
        }
        cairo_show_text(context, label);
    }
};



};

#endif
