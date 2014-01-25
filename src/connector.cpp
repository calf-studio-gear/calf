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

#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include <calf/giface.h>
#include <calf/connector.h>
#include <calf/custom_ctl.h>
#include <calf/jackhost.h>

#include <jack/jack.h>
#include <jack/midiport.h>

using namespace std;
using namespace calf_plugins;

calf_connector::calf_connector(plugin_strip *strip_)
{
    strip = strip_;
    window = NULL;
    create_window();
}

calf_connector::~calf_connector()
{
    
}

void calf_connector::on_destroy_window(GtkWidget *window, gpointer data)
{
    calf_connector *self = (calf_connector *)data;
    if (self->strip->con)
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(self->strip->con), FALSE);
    self->strip->connector = NULL;
    delete self;
}

void calf_connector::port_clicked(GtkWidget *button, gpointer data)
{
    connector_port *port = (connector_port *)data;
    if (port == port->connector->active_port)
        return;
    port->connector->active_port = port;
    port->connector->fill_list(port->connector);
}

void calf_connector::connector_clicked(GtkCellRendererToggle *cell_renderer, gchar *path_, gpointer data)
{
    calf_connector *self = (calf_connector *)data;
    GtkTreeIter iter;
    gchar *port;
    gboolean enabled;
    GtkTreePath* path = gtk_tree_path_new_from_string(path_);
    gtk_tree_model_get_iter(GTK_TREE_MODEL(self->jacklist), &iter, path);
    gtk_tree_model_get(GTK_TREE_MODEL(self->jacklist), &iter, 0, &port, 2, &enabled, -1);
    gtk_list_store_set(GTK_LIST_STORE (self->jacklist), &iter, 2, !enabled, -1);
    string source = "";
    string dest   = "";
    switch (self->active_port->type) {
        case 0:
        default:
            source = port;
            dest   = (string)jack_get_client_name(self->jackclient)
                   + ":" + self->strip->plugin->inputs[self->active_port->id].nice_name;
            break;
        case 1:
            source = (string)jack_get_client_name(self->jackclient)
                   + ":" + self->strip->plugin->outputs[self->active_port->id].nice_name;
            dest   = port;
            break;
        case 2:
            source = port;
            dest   = (string)jack_get_client_name(self->jackclient)
                   + ":" + self->strip->plugin->midi_port.nice_name;
            break;
    }
    if (!enabled) {
        jack_connect(self->jackclient, source.c_str(), dest.c_str());
    } else {
        jack_disconnect(self->jackclient, source.c_str(), dest.c_str());
    }
}

//void calf_connector::jack_port_connect_callback(jack_port_id_t a, jack_port_id_t b, int connect, gpointer data)
//{
    //calf_connector *self = (calf_connector *)data;
    //self->fill_list(data);
//}
//void calf_connector::jack_port_rename_callback(jack_port_id_t port, const char *old_name, const char *new_name, gpointer data)
//{
    //calf_connector *self = (calf_connector *)data;
    //self->fill_list(data);
//}
//void calf_connector::jack_port_registration_callback(jack_port_id_t port, int register, gpointer data)
//{
    //calf_connector *self = (calf_connector *)data;
    //self->fill_list(data);
//}
    
void calf_connector::create_window()
{
    char buf[256];
    
    // CREATE WINDOW
    
    string title = "Calf " + (string)strip->plugin->get_metadata_iface()->get_label() + " Connector";
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), title.c_str());
    gtk_window_set_destroy_with_parent(GTK_WINDOW(window), TRUE);
    //gtk_window_set_keep_above(GTK_WINDOW(window), TRUE);
    gtk_window_set_icon_name(GTK_WINDOW(window), "calf_plugin");
    gtk_window_set_role(GTK_WINDOW(window), "calf_connector");
    gtk_window_set_default_size(GTK_WINDOW(window), -1, -1);
    //gtk_widget_set_name(GTK_WIDGET(window), "Calf-Connector");
    
    g_signal_connect(G_OBJECT(window), "destroy",
                     G_CALLBACK (on_destroy_window), (gpointer)this);
    
    // CONTAINER STUFF
    
    GtkHBox *hbox = GTK_HBOX(gtk_hbox_new(false, 0));
    gtk_container_add(GTK_CONTAINER(window), GTK_WIDGET(hbox));
    //gtk_widget_set_name(GTK_WIDGET(hbox), "Calf-Container");
    
    GtkVBox *ports = GTK_VBOX(gtk_vbox_new(false, 0));
    gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(ports), false, false, 0);
    gtk_container_set_border_width (GTK_CONTAINER(ports), 5);
    
    GtkWidget *inframe = gtk_frame_new("Inputs");
    gtk_box_pack_start(GTK_BOX(ports), GTK_WIDGET(inframe), false, true, 10);
    gtk_container_set_border_width (GTK_CONTAINER(inframe), 0);
    
    GtkWidget *inputs = gtk_vbox_new(false, 5);
    gtk_container_add(GTK_CONTAINER(inframe), GTK_WIDGET(inputs));
    
    GtkWidget *outframe = gtk_frame_new("Outputs");
    gtk_box_pack_start(GTK_BOX(ports), GTK_WIDGET(outframe), false, true, 10);
    gtk_container_set_border_width (GTK_CONTAINER(outframe), 0);
    
    GtkWidget *outputs = gtk_vbox_new(false, 5);
    gtk_container_add(GTK_CONTAINER(outframe), GTK_WIDGET(outputs));
    
    GtkWidget *midframe = gtk_frame_new("MIDI");
    gtk_box_pack_start(GTK_BOX(ports), GTK_WIDGET(midframe), false, true, 10);
    gtk_container_set_border_width (GTK_CONTAINER(midframe), 0);
    
    GtkWidget *midi = gtk_vbox_new(false, 5);
    gtk_container_add(GTK_CONTAINER(midframe), GTK_WIDGET(midi));
    
    gtk_widget_show_all(window);
    
    // ALL THOSE PORT BUTTONS
    GtkRadioButton *last = NULL;
    GtkRadioButton *first = NULL;
    int c = 0;
    // inputs
    for (int i = 0; i < strip->plugin->in_count; i++) {
        sprintf(buf, "Input #%d", (i + 1));
        GtkWidget *in = gtk_radio_button_new_with_label(NULL, buf);
        gtk_box_pack_start(GTK_BOX(inputs), in, false, true, 0);
        gtk_widget_set_size_request(GTK_WIDGET(in), 110, 27);
        gtk_widget_show(GTK_WIDGET(in));
        if (!i)
            last = GTK_RADIO_BUTTON(in);
        else
            gtk_radio_button_set_group(GTK_RADIO_BUTTON(in), gtk_radio_button_get_group(last));
        inports[c].type = 0;
        inports[c].id = c;
        inports[c].connector = this;
        g_signal_connect(GTK_OBJECT(in), "clicked", G_CALLBACK(port_clicked), (gpointer*)&inports[c]);
        if (!first) {
            first = GTK_RADIO_BUTTON(in);
            active_port = &inports[c];
        }
        c++;
    }
    if (!c)
        gtk_widget_hide(GTK_WIDGET(inframe));
    c = 0;
    // outputs
    for (int i = 0; i < strip->plugin->out_count; i++) {
        sprintf(buf, "Output #%d", (i + 1));
        GtkWidget *out = gtk_radio_button_new_with_label(NULL, buf);
        gtk_box_pack_start(GTK_BOX(outputs), out, false, true, 0);
        gtk_widget_set_size_request(GTK_WIDGET(out), 110, 27);
        gtk_widget_show(GTK_WIDGET(out));
        if (!i and !last)
            last = GTK_RADIO_BUTTON(out);
        else
            gtk_radio_button_set_group(GTK_RADIO_BUTTON(out), gtk_radio_button_get_group(last));
        outports[c].type = 1;
        outports[c].id = c;
        outports[c].connector = this;
        g_signal_connect(GTK_OBJECT(out), "clicked", G_CALLBACK(port_clicked), (gpointer*)&outports[c]);
        if (!first) {
            first = GTK_RADIO_BUTTON(out);
            active_port = &outports[c];
        }
        c++;
    }
    if (!c)
        gtk_widget_hide(GTK_WIDGET(outframe));
    c = 0;
    // midi
    if (strip->plugin->get_metadata_iface()->get_midi()) {
        GtkWidget *mid = gtk_radio_button_new_with_label(NULL, "MIDI");
        gtk_box_pack_start(GTK_BOX(midi), mid, false, true, 0);
        gtk_widget_set_size_request(GTK_WIDGET(mid), 110, 27);
        gtk_radio_button_set_group(GTK_RADIO_BUTTON(mid), gtk_radio_button_get_group(last));
        midiports[c].type = 2;
        midiports[c].id = c;
        midiports[c].connector = this;
        g_signal_connect(GTK_OBJECT(mid), "clicked", G_CALLBACK(port_clicked), (gpointer*)&midiports[c]);
        gtk_widget_show(GTK_WIDGET(mid));
        if (!first) {
            first = GTK_RADIO_BUTTON(mid);
            active_port = &midiports[c];
        }
        c++;
    }
    if (!c)
        gtk_widget_hide(GTK_WIDGET(midframe));
    
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(first), TRUE);
    
    // LIST STUFF
    
    GtkTreeViewColumn *col;
    GtkCellRenderer *renderer;
    
    // scroller
    GtkWidget *scroller = gtk_scrolled_window_new(NULL, NULL);
    gtk_box_pack_start(GTK_BOX(hbox), scroller, true, true, 0);
    gtk_widget_set_size_request(GTK_WIDGET(scroller), 360, 360);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroller), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_widget_show(GTK_WIDGET(scroller));
    
    // list store / tree view
    jacklist = gtk_list_store_new(3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_BOOLEAN);
    GtkWidget *jackview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(jacklist));
    gtk_container_add(GTK_CONTAINER(scroller), jackview);
    gtk_widget_show(GTK_WIDGET(jackview));
    
    // toggle column
    col = gtk_tree_view_column_new();
    gtk_tree_view_column_set_title(col, "☑");
    renderer = gtk_cell_renderer_toggle_new();
    gtk_tree_view_column_pack_start(col, renderer, TRUE);
    gtk_tree_view_column_add_attribute(col, renderer, "active", 2);
    gtk_tree_view_append_column(GTK_TREE_VIEW(jackview), col);
    g_signal_connect(GTK_OBJECT(renderer), "toggled", G_CALLBACK(connector_clicked), this);
    
    // text column
    col = gtk_tree_view_column_new();
    gtk_tree_view_column_set_title(col, "Port");
    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(col, renderer, TRUE);
    gtk_tree_view_column_add_attribute(col, renderer, "text", 1);
    gtk_tree_view_append_column(GTK_TREE_VIEW(jackview), col);
    
    gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(jackview)),
                                GTK_SELECTION_NONE);
                              
    g_object_unref(jacklist);
    
    // JACK STUFF
    
    jackclient = strip->plugin->client->client;
    
    //jack_set_port_connect_callback(jackclient, jack_port_connect_callback, this);
    //jack_set_port_rename_callback(jackclient, jack_port_rename_callback, this);
    //jack_set_port_registration_callback(jackclient, jack_port_registration_callback , this);
    
    fill_list(this);
}

void calf_connector::fill_list(gpointer data)
{
    calf_connector *self = (calf_connector *)data;
    if (!self->jacklist)
        return;
    GtkTreeIter iter;
    gtk_list_store_clear(self->jacklist);
    const char **ports;
    switch (self->active_port->type) {
        case 0:
        default:
            ports = jack_get_ports(self->jackclient, ".*:.*", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput);
            break;
        case 1:
            ports = jack_get_ports(self->jackclient, ".*:.*", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput);
            break;
        case 2:
            ports = jack_get_ports(self->jackclient, ".*:.*", JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput);
            break;
    }
    if (!ports)
        return;
    int c = 0;
    do {
        gtk_list_store_append(self->jacklist, &iter);
        if (gtk_list_store_iter_is_valid(self->jacklist, &iter))
            gtk_list_store_set(self->jacklist, &iter, 0, ports[c], 1, ports[c], 2,  FALSE, -1);
    } while (ports[++c]);
    jack_free(ports);
    self->set_toggles(data);
}

void calf_connector::set_toggles(gpointer data)
{
    calf_connector *self = (calf_connector *)data;
    if (!self->jacklist)
        return;
    const char** cons;
    const char** ports;
    char path[16];
    GtkTreeIter iter;
    jack_port_t *port = NULL;
    int p = 0, c = 0, f = 0, portflag = 0;
    const char * porttype;
    string s  = "", nn = "";
    switch (self->active_port->type) {
        case 0:
        default:
            portflag = JackPortIsOutput;
            porttype = JACK_DEFAULT_AUDIO_TYPE;
            nn       = (string)jack_get_client_name(jackclient)
                     + ":" + self->strip->plugin->inputs[active_port->id].nice_name;
            break;
        case 1:
            portflag = JackPortIsInput;
            porttype = JACK_DEFAULT_AUDIO_TYPE;
            nn       = (string)jack_get_client_name(jackclient)
                     + ":" + self->strip->plugin->outputs[active_port->id].nice_name;
            break;
        case 2:
            portflag = JackPortIsOutput;
            porttype = JACK_DEFAULT_MIDI_TYPE;
            nn       = (string)jack_get_client_name(jackclient)
                     + ":" + self->strip->plugin->midi_port.nice_name;
            break;
    }
    port  = jack_port_by_name(self->jackclient, nn.c_str());
    cons  = jack_port_get_all_connections(self->jackclient, port);
    ports = jack_get_ports(self->jackclient,
                           NULL,
                           porttype,
                           portflag);
    if (!ports or !cons) {
        jack_free(cons);
        jack_free(ports);
        return;
    }
    p = 0;
    do {
        sprintf(path, "%d", p);
        gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(self->jacklist), &iter, path);
        c = f = 0;
        do {
            if (cons[c] == ports[p]) {
                gtk_list_store_set (jacklist, &iter, 2, TRUE, -1);
                f = 1;
            }
        } while (cons[++c]);
        if (!f)
            gtk_list_store_set (jacklist, &iter, 2, FALSE, -1);
    } while (ports[++p]);
    jack_free(cons);
    jack_free(ports);
}

void calf_connector::close()
{
    if (window)
        gtk_widget_destroy(GTK_WIDGET(window));
}
