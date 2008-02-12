/* Calf DSP Library Utility Application - calfjackhost
 * GUI - main window
 *
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
#ifndef __CALF_MAIN_WIN_H
#define __CALF_MAIN_WIN_H

#include <expat.h>
#include <map>
#include <set>
#include <vector>
#include <gtk/gtk.h>
#include <calf/gui.h>
#include <calf/jackhost.h>
#include "custom_ctl.h"

namespace synth {

    class main_window: public main_window_iface
    {
    public:
        struct plugin_strip
        {
            main_window *main_win;
            plugin_ctl_iface *plugin;
            plugin_gui_window *gui_win;
            GtkWidget *name, *midi_in, *audio_in[2], *audio_out[2];
        };
        
    public:
        GtkWindow *toplevel;
        GtkWidget *all_vbox;
        GtkWidget *strips_table;
        jack_client *client;
        std::map<plugin_ctl_iface *, plugin_strip *> plugins;
        std::set<std::string> conditions;
        std::vector<plugin_ctl_iface *> plugin_queue;
        std::string prefix;
        bool is_closed;
        int source_id;

    protected:
        plugin_strip *create_strip(plugin_ctl_iface *plugin);
        void update_strip(plugin_ctl_iface *plugin);
        static gboolean on_idle(void *data);

    public:
        main_window();
        void add_plugin(plugin_ctl_iface *plugin);
        void del_plugin(plugin_ctl_iface *plugin);
        void set_window(plugin_ctl_iface *iface, plugin_gui_window *window);
        void refresh_all_presets();
        void refresh_plugin(plugin_ctl_iface *plugin);
        void on_closed();
        void close_guis();
        void open_gui(plugin_ctl_iface *plugin);
        bool check_condition(const char *cond) {
            return conditions.count(cond) != 0;
        }
    
        void create();
    };
};

#endif
