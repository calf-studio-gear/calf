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
#include <calf/gtk_main_win.h>


#ifndef __CALF_CONNECTOR_H
#define __CALF_CONNECTOR_H

namespace calf_plugins {

struct plugin_strip;
class calf_connector;

struct connector_port
{
    int type;
    int id;
    calf_connector *connector;
};

class calf_connector {
private:
    jack_client_t *jackclient;
    connector_port inports[8];
    connector_port outports[8];
    connector_port midiports[8];
    GtkListStore *inlist, *outlist, *midilist;
    connector_port *active_in, *active_out, *active_midi;
    GtkWidget *window;
    plugin_strip *strip;
    
    void create_window();
    static void on_destroy_window(GtkWidget *window, gpointer data);
    static void close_window(GtkWidget *button, gpointer data);
    
    static void inconnector_clicked(GtkCellRendererToggle *cell_renderer, gchar *path, gpointer data);
    static void outconnector_clicked(GtkCellRendererToggle *cell_renderer, gchar *path, gpointer data);
    static void midiconnector_clicked(GtkCellRendererToggle *cell_renderer, gchar *path, gpointer data);
    
    static void inport_clicked(GtkWidget *button, gpointer data);
    static void outport_clicked(GtkWidget *button, gpointer data);
    static void midiport_clicked(GtkWidget *button, gpointer data);
    
    static void disconnect_inputs(GtkWidget *button, gpointer data);
    static void disconnect_outputs(GtkWidget *button, gpointer data);
    static void disconnect_midi(GtkWidget *button, gpointer data);
    static void disconnect_all(GtkWidget *button, gpointer data);
    void _disconnect(int type);
public:
    calf_connector(plugin_strip *strip_);
    ~calf_connector();
    
    //static void jack_port_connect_callback(jack_port_id_t a, jack_port_id_t b, int connect, void *arg);
    //static void jack_port_rename_callback(jack_port_id_t port, const char *old_name, const char *new_name, void *arg);
    //static void jack_port_registration_callback(jack_port_id_t port, int register, void *arg);
    
    void toggle_port(calf_connector *self, GtkListStore *list, gchar *path_, gchar **port, gboolean &enabled);
    void connect(jack_client_t *client, gchar *source, gchar *dest, gboolean enabled);
    
    void fill_list(calf_connector *self, int type);
    void fill_inlist(gpointer data);
    void fill_outlist(gpointer data);
    void fill_midilist(gpointer data);
    
    void set_toggles(calf_connector *self, int type);
    
    void close();
};

};

#endif
