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

namespace calf_plugins {

class plugin_gui;

struct control_base
{
    std::string control_name;
    typedef std::map<std::string, std::string> xml_attribute_map;
    xml_attribute_map attribs;
    plugin_gui *gui;
    void require_attribute(const char *name);
    void require_int_attribute(const char *name);
    int get_int(const char *name, int def_value = 0);
    float get_float(const char *name, float def_value = 0.f);
    /// called after creation, so that all standard properties can be set
    virtual void set_std_properties() = 0;
};

#define _GUARD_CHANGE_ if (in_change) return; guard_change __gc__(this);

struct param_control: public control_base
{    
    int param_no;
    std::string param_variable;
    GtkWidget *label, *widget;
    int in_change;
    float old_displayed_value;
    
    struct guard_change {
        param_control *pc;
        guard_change(param_control *_pc) : pc(_pc) { pc->in_change++; }
        ~guard_change() { pc->in_change--; }
    };
    
    param_control();
    inline const parameter_properties &get_props();
    
    virtual void init_xml(const char *element) {}
    virtual GtkWidget *create_label();
    virtual void update_label();
    /// called to create a widget for a control
    virtual GtkWidget *create(plugin_gui *_gui, int _param_no)=0;
    /// called to transfer the value from control to parameter(s)
    virtual void get()=0;
    /// called to transfer the value from parameter(s) to control
    virtual void set()=0;
    /// called on DSSI configure()
    virtual void configure(const char *key, const char *value) {}
    virtual void hook_params();
    virtual void on_idle() {}
    virtual void set_std_properties();
    virtual ~param_control();
};

struct control_container: public control_base
{
    GtkContainer *container;
    
    virtual GtkWidget *create(plugin_gui *_gui, const char *element, xml_attribute_map &attributes)=0;
    virtual void add(GtkWidget *w, control_base *base) { gtk_container_add(container, w); }
    virtual void set_std_properties();
    virtual ~control_container() {}
};

class plugin_gui_window;

class plugin_gui: public send_configure_iface, public send_updates_iface
{
protected:
    int param_count;
    std::multimap<int, param_control *> par2ctl;
    XML_Parser parser;
    param_control *current_control;
    std::vector<control_container *> container_stack;
    control_container *top_container;
    std::map<std::string, int> param_name_map;
    int ignore_stack;
    int last_status_serial_no;
    std::map<int, GSList *> param_radio_groups;
    GtkWidget *leftBox, *rightBox;
public:
    plugin_gui_window *window;
    GtkWidget *container;
    const char *effect_name;
    plugin_ctl_iface *plugin;
    preset_access_iface *preset_access;
    std::vector<param_control *> params;

    plugin_gui(plugin_gui_window *_window);
    GtkWidget *create_from_xml(plugin_ctl_iface *_plugin, const char *xml);
    param_control *create_control_from_xml(const char *element, const char *attributes[]);
    control_container *create_container_from_xml(const char *element, const char *attributes[]);

    void add_param_ctl(int param, param_control *ctl) { par2ctl.insert(std::pair<int, param_control *>(param, ctl)); }
    void refresh();
    void refresh(int param_no, param_control *originator = NULL);
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
    /// Show rack ear widgets
    void show_rack_ears(bool show);
    ~plugin_gui();
    static void xml_element_start(void *data, const char *element, const char *attributes[]);
    static void xml_element_end(void *data, const char *element);
};

class main_window_iface;
class main_window_owner_iface;

/// A class used to inform the plugin GUIs about the environment they run in
/// (currently: what plugin features are accessible)
struct gui_environment_iface
{
    virtual bool check_condition(const char *name) = 0;
    virtual calf_utils::config_db_iface *get_config_db() = 0;
    virtual calf_utils::gui_config *get_config() = 0;
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

public:
    gui_environment();
    virtual bool check_condition(const char *name) { return conditions.count(name) != 0; }
    virtual calf_utils::config_db_iface *get_config_db() { return config_db; }
    virtual calf_utils::gui_config *get_config() { return &gui_config; }
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
    /// Add the plugin to the window
    virtual void add_plugin(plugin_ctl_iface *plugin) = 0;
    /// Remove the plugin from the window
    virtual void del_plugin(plugin_ctl_iface *plugin) = 0;
    /// Refresh the plugin UI
    virtual void refresh_plugin(plugin_ctl_iface *plugin) = 0;
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

class plugin_gui_window: public calf_utils::config_listener_iface
{
private:
    void cleanup();
public:
    plugin_gui *gui;
    GtkWindow *toplevel;
    GtkUIManager *ui_mgr;
    GtkActionGroup *std_actions, *builtin_preset_actions, *user_preset_actions, *command_actions;
    gui_environment_iface *environment;
    main_window_iface *main;
    int source_id;
    calf_utils::config_notifier_iface *notifier;

    plugin_gui_window(gui_environment_iface *_env, main_window_iface *_main);
    std::string make_gui_preset_list(GtkActionGroup *grp, bool builtin, char &ch);
    std::string make_gui_command_list(GtkActionGroup *grp);
    void fill_gui_presets(bool builtin, char &ch);
    void create(plugin_ctl_iface *_plugin, const char *title, const char *effect);
    void close();
    virtual void on_config_change();
    static gboolean on_idle(void *data);
    static void on_window_destroyed(GtkWidget *window, gpointer data);
    ~plugin_gui_window();
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

};

#endif
