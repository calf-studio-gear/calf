/* Calf DSP Library Utility Application - calfjackhost
 * Class for managing calfjackhost's 
 *
 * Copyright (C) 2007 Krzysztof Foltman
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

#include <calf/jackhost.h>

namespace calf_plugins {

class main_window;

struct host_session: public main_window_owner_iface, public calf_plugins::progress_report_iface
{
    std::string client_name, input_name, output_name, midi_name;
    std::vector<std::string> plugin_names;
    std::map<int, std::string> presets;
#if USE_LASH
    int lash_source_id;
    lash_client_t *lash_client;
# if !USE_LASH_0_6
    lash_args_t *lash_args;
# endif
#endif
    
    // these are not saved
    jack_client client;
    std::string autoconnect_midi;
    int autoconnect_midi_index;
    std::set<int> chains;
    std::vector<jack_host *> plugins;
    main_window *main_win;
    bool restoring_session;
    std::set<std::string> instances;
    GtkWidget *progress_window;
    plugin_gui_window *gui_win;
    
    host_session();
    void open();
    void add_plugin(std::string name, std::string preset, std::string instance_name = std::string());
    void create_plugins_from_list();
    void connect();
    void close();
    bool activate_preset(int plugin, const std::string &preset, bool builtin);
#if USE_LASH
    static gboolean update_lash(void *self) { ((host_session *)self)->update_lash(); return TRUE; }
    void update_lash();
# if !USE_LASH_0_6
    void send_lash(LASH_Event_Type type, const std::string &data) {
        lash_send_event(lash_client, lash_event_new_with_all(type, data.c_str()));
    }
# endif
#endif
    virtual void new_plugin(const char *name);    
    virtual void remove_plugin(plugin_ctl_iface *plugin);
    void remove_all_plugins();
    std::string get_next_instance_name(const std::string &effect_name);
    
    /// Create a toplevel window with progress bar
    GtkWidget *create_progress_window();
    /// Implementation of progress_report_iface function
    void report_progress(float percentage, const std::string &message);
    
    /// Implementation of open file functionality (TODO)
    virtual char *open_file(const char *name);
    /// Implementation of save file functionality
    virtual char *save_file(const char *name);
    /// Implementation of connection to LASH (or, in future, other session manager)
    void connect_to_session_manager(int argc, char *argv[]);
};

};

#endif
