/* Calf DSP Library Utility Application - calfjackhost
 * Class for managing calfjackhost's session.
 *
 * Copyright (C) 2007-2011 Krzysztof Foltman
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */
#ifndef CALF_HOST_SESSION_H
#define CALF_HOST_SESSION_H

#include <config.h>

#include "gui.h"
#include "jackhost.h"
#include "session_mgr.h"

namespace calf_plugins {

class main_window;

class host_session: public main_window_owner_iface, public session_client_iface
{
private:
    static host_session *instance;
public:
    /// Requested JACK client name.
    std::string client_name;
    /// Template for input names.
    std::string input_name;
    /// Template for output names.
    std::string output_name;
    /// Template for MIDI port names.
    std::string midi_name;
    /// Name of the file to load at start.
    std::string load_name;
    /// Use load_name as session state name - doesn't signal an error if the file doesn't exist, but uses it for saving.
    bool only_load_if_exists;
    /// Plugins to create on startup, in create_plugins_from_list (based on command line).
    std::vector<std::string> plugin_names;
    /// Requested presets for the plugins in plugin_names.
    std::map<int, std::string> presets;
    /// Selected session manager (if any).
    session_manager_iface *session_manager;
    /// Save has been requested from SIGUSR1 handler
    volatile bool save_file_on_next_idle_call;
    /// If non-zero, quit has been requested through signal with same value
    volatile int quit_on_next_idle_call;
    /// File name of the current rack
    std::string current_filename;
    
    // these are not saved
    jack_client client;
    std::string autoconnect_midi;
    int autoconnect_midi_index;
    std::set<int> chains;
    std::vector<jack_host *> plugins;
    main_window_iface *main_win;
    std::set<std::string> instances;
    plugin_gui_window *gui_win;
    session_environment_iface *session_env;
    
    host_session(session_environment_iface *);
    void open();
    void add_plugin(std::string name, std::string preset, std::string instance_name = std::string());
    void create_plugins_from_list();
    void connect();
    void close();
    bool activate_preset(int plugin, const std::string &preset, bool builtin);
    void remove_all_plugins();
    std::string get_next_instance_name(const std::string &effect_name);
    
    /// Set handlers for SIGUSR1 (that LADISH uses to invoke Save function), SIGTERM and SIGHUP
    void set_signal_handlers();
    
    /// unix signal handler
    static void signal_handler(int signum);
    
    /// Client name for window title bar
    std::string get_client_name() const;
    
public:
    /// Implementation of open file functionality (TODO)
    virtual char *open_file(const char *name);
    /// Implementation of save file functionality
    virtual char *save_file(const char *name);

    /// Load from session manager
    virtual void load(session_load_iface *);
    /// Save to session manager
    virtual void save(session_save_iface *);
    
    virtual void new_plugin(const char *name);    
    virtual void remove_plugin(plugin_ctl_iface *plugin);
    virtual void on_main_window_destroy();
    virtual void on_idle();
    virtual void reorder_plugins();
    virtual std::string get_current_filename() const;
    virtual void set_current_filename(const std::string &name);
};

};

#endif
