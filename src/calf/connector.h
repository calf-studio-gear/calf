/* Calf Library
 * Jack Connector Window
 *
 * Copyright (C) 2014 Markus Schmidt / Krzysztof Foltman
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

#include <jack/jack.h>

#ifndef __CALF_CONNECTOR_H
#define __CALF_CONNECTOR_H

namespace calf_plugins {

class calf_connector;

struct connector_port
{
    int type;
    int id;
    calf_connector *connector;
};

class calf_connector {
private:
    GtkWidget *window;
    GtkWidget *toggle;
    GSList *selector;
    GtkListStore *jacklist;
    const plugin_ctl_iface *plugin;
    void create_window();
    connector_port inports[8];
    connector_port outports[8];
    connector_port midiports[8];
    connector_port *active_port;
    jack_client_t *jackclient;
public:
    static void connector_clicked(GtkCellRendererToggle *cell_renderer, gchar *path, gpointer data);
    static void port_clicked(GtkWidget *button, gpointer port_);
    static void on_destroy_window(GtkWidget *window, gpointer self);
    calf_connector(plugin_ctl_iface *plugin_, GtkWidget *toggle_);
    ~calf_connector();
    void fill_list();
    void close();
};

};

#endif
