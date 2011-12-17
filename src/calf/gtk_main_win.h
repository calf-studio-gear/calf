/* Calf DSP Library Utility Application - calfjackhost
 * GUI - main window
 *
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
#ifndef __CALF_MAIN_WIN_H
#define __CALF_MAIN_WIN_H

#include "gui.h"
#include "gui_config.h"

namespace calf_plugins {

    class gtk_main_window: public main_window_iface, public gui_environment, public calf_utils::config_listener_iface
    {
    public:
        struct plugin_strip
        {
            int id;
            gtk_main_window *main_win;
            plugin_ctl_iface *plugin;
            plugin_gui_window *gui_win;
            GtkWidget *strip_table, *name, *button, *midi_in, *audio_in[2], *audio_out[2], *extra, *leftBox, *rightBox;
        };
        
        struct add_plugin_params
        {
            gtk_main_window *main_win;
            std::string name;
            add_plugin_params(gtk_main_window *_main_win, const std::string &_name)
            : main_win(_main_win), name(_name) {}
        };
        
    public:
        GtkWindow *toplevel;
        GtkWidget *all_vbox;
        GtkWidget *strips_table;
        GtkUIManager *ui_mgr;
        GtkActionGroup *std_actions, *plugin_actions;
        std::map<plugin_ctl_iface *, plugin_strip *> plugins;
        std::vector<plugin_ctl_iface *> plugin_queue;
        bool is_closed;
        bool draw_rackmounts;
        int source_id;
        main_window_owner_iface *owner;
        calf_utils::config_notifier_iface *notifier;

    protected:
        GtkWidget *progress_window;
    
    protected:
        plugin_strip *create_strip(plugin_ctl_iface *plugin);
        void update_strip(plugin_ctl_iface *plugin);
        void sort_strips();
        static gboolean on_idle(void *data);
        std::string make_plugin_list(GtkActionGroup *actions);
        static void add_plugin_action(GtkWidget *src, gpointer data);
        void display_error(const char *error, const char *filename);
        void on_config_change();
        /// Create a toplevel window with progress bar
        GtkWidget *create_progress_window();        

    public:
        gtk_main_window();
        void set_owner(main_window_owner_iface *_owner) { owner = _owner; }
        void new_plugin(const char *name) { owner->new_plugin(name); }
        void add_plugin(plugin_ctl_iface *plugin);
        void del_plugin(plugin_ctl_iface *plugin);
        void set_window(plugin_ctl_iface *iface, plugin_gui_window *window);
        void refresh_all_presets(bool builtin_too);
        void refresh_plugin(plugin_ctl_iface *plugin);
        void on_closed();
        void open_gui(plugin_ctl_iface *plugin);    
        void create();
        void open_file();
        bool save_file();
        bool save_file_as();
        void save_file_from_sighandler();
        void show_rack_ears(bool show);
        /// Implementation of progress_report_iface function
        virtual void report_progress(float percentage, const std::string &message);
        /// Mark condition as true
        virtual void add_condition(const std::string &name);
        /// Display an error dialog
        virtual void show_error(const std::string &text);
    private:
        static const GtkActionEntry actions[];
        static void on_open_action(GtkWidget *widget, gtk_main_window *main);
        static void on_save_action(GtkWidget *widget, gtk_main_window *main);
        static void on_save_as_action(GtkWidget *widget, gtk_main_window *main);
        static void on_preferences_action(GtkWidget *widget, gtk_main_window *main);
        static void on_reorder_action(GtkWidget *widget, gtk_main_window *main);
        static void on_exit_action(GtkWidget *widget, gtk_main_window *main);
    };
};

#endif
