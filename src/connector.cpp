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
//#include <calf/custom_ctl.h>
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

// CALLBACKS FOR CALLBACKS
void calf_connector::toggle_port(calf_connector *self, GtkListStore *list, gchar *path_, gchar **port, gboolean &enabled)
{
    GtkTreeIter iter;
    gtk_tree_model_get_iter(GTK_TREE_MODEL(list), &iter, gtk_tree_path_new_from_string(path_));
    gtk_tree_model_get(GTK_TREE_MODEL(list), &iter, 0, port, 2, &enabled, -1);
    gtk_list_store_set(GTK_LIST_STORE (list), &iter, 2, !enabled, -1);
    enabled = !enabled;
}
void calf_connector::connect(jack_client_t *client, gchar *source, gchar *dest, gboolean enabled)
{
    if (enabled) {
        jack_connect(client, source, dest);
    } else {
        jack_disconnect(client, source, dest);
    }
    set_toggles(this, 0);
    set_toggles(this, 1);
    set_toggles(this, 2);
}

// PORT CLICKS
void calf_connector::inport_clicked(GtkWidget *button, gpointer data)
{
    connector_port *port = (connector_port *)data;
    if (port == port->connector->active_in)
        return;
    port->connector->active_in = port;
    port->connector->fill_inlist(port->connector);
}
void calf_connector::outport_clicked(GtkWidget *button, gpointer data)
{
    connector_port *port = (connector_port *)data;
    if (port == port->connector->active_out)
        return;
    port->connector->active_out = port;
    port->connector->fill_outlist(port->connector);
}
void calf_connector::midiport_clicked(GtkWidget *button, gpointer data)
{
    connector_port *port = (connector_port *)data;
    if (port == port->connector->active_midi)
        return;
    port->connector->active_midi = port;
    port->connector->fill_midilist(port->connector);
}

// LIST CLICKS
void calf_connector::inconnector_clicked(GtkCellRendererToggle *cell_renderer, gchar *path_, gpointer data)
{
    calf_connector *self = (calf_connector *)data;
    gchar *port;
    gboolean enabled;
    self->toggle_port(self, self->inlist, path_, &port, enabled);
    string source = port;
    string dest   = (string)jack_get_client_name(self->jackclient)
                  + ":" + self->strip->plugin->inputs[self->active_in ? self->active_in->id : 0].nice_name;
    self->connect(self->jackclient, (gchar*)source.c_str(), (gchar*)dest.c_str(), enabled);
    self->set_toggles(self, 0);
}
void calf_connector::outconnector_clicked(GtkCellRendererToggle *cell_renderer, gchar *path_, gpointer data)
{
    calf_connector *self = (calf_connector *)data;
    gchar *port = NULL;
    gboolean enabled;
    self->toggle_port(self, self->outlist, path_, &port, enabled);
    string source = (string)jack_get_client_name(self->jackclient)
                  + ":" + self->strip->plugin->outputs[self->active_out ? self->active_out->id : 0].nice_name;
    string dest   = port;
    self->connect(self->jackclient, (gchar*)source.c_str(), (gchar*)dest.c_str(), enabled);
    self->set_toggles(self, 1);
}
void calf_connector::midiconnector_clicked(GtkCellRendererToggle *cell_renderer, gchar *path_, gpointer data)
{
    calf_connector *self = (calf_connector *)data;
    gchar *port = NULL;
    gboolean enabled;
    self->toggle_port(self, self->midilist, path_, &port, enabled);
    string source = port;
    string dest   = (string)jack_get_client_name(self->jackclient)
                  + ":" + self->strip->plugin->midi_port.nice_name;
    self->connect(self->jackclient, (gchar*)source.c_str(), (gchar*)dest.c_str(), enabled);
    self->set_toggles(self, 2);
}

// DISCONNECT CLICKS
void calf_connector::disconnect_inputs(GtkWidget *button, gpointer data)
{
    ((calf_connector *)data)->_disconnect(0);
}
void calf_connector::disconnect_outputs(GtkWidget *buttconson, gpointer data)
{
    ((calf_connector *)data)->_disconnect(1);
}
void calf_connector::disconnect_midi(GtkWidget *button, gpointer data)
{
    ((calf_connector *)data)->_disconnect(2);
}
void calf_connector::disconnect_all(GtkWidget *button, gpointer data)
{
    ((calf_connector *)data)->_disconnect(-1);
}

// JACK CALLBACKS
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
    

void calf_connector::fill_inlist(gpointer data)
{
    ((calf_connector *)data)->fill_list((calf_connector *)data, 0);
}
void calf_connector::fill_outlist(gpointer data)
{
    ((calf_connector *)data)->fill_list((calf_connector *)data, 1);
}
void calf_connector::fill_midilist(gpointer data)
{
    ((calf_connector *)data)->fill_list((calf_connector *)data, 2);
}

void calf_connector::fill_list(calf_connector *self, int type)
{
    GtkTreeIter iter;
    GtkListStore *list;
    const char **ports;
    switch (type) {
        case 0:
        default:
            list = self->inlist;
            gtk_list_store_clear(list);
            ports = jack_get_ports(self->jackclient, ".*:.*", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput);
            if (!ports) return;
            break;
        case 1:
            list = self->outlist;
            gtk_list_store_clear(list);
            ports = jack_get_ports(self->jackclient, ".*:.*", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput);
            if (!ports) return;
            break;
        case 2:
            list = self->midilist;
            gtk_list_store_clear(list);
            ports = jack_get_ports(self->jackclient, ".*:.*", JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput);
            if (!ports) return;
            break;
    }
    
    int c = 0;
    do {
        gtk_list_store_append(list, &iter);
        if (gtk_list_store_iter_is_valid(list, &iter))
            gtk_list_store_set(list, &iter, 0, ports[c], 1, ports[c], 2,  FALSE, -1);
    } while (ports[++c]);
    jack_free(ports);
    self->set_toggles(self, type);
}

void calf_connector::set_toggles(calf_connector *self, int type)
{
    const char** cons;
    const char** ports;
    char path[16];
    GtkTreeIter iter;
    GtkListStore *list;
    jack_port_t *port = NULL;
    int p = 0, c = 0, f = 0, portflag = 0;
    const char * porttype;
    string s  = "", nn = "";
    switch (type) {
        case 0:
        default:
            if (!self->active_in)
                return;
            portflag = JackPortIsOutput;
            porttype = JACK_DEFAULT_AUDIO_TYPE;
            nn       = (string)jack_get_client_name(self->jackclient)
                     + ":" + self->strip->plugin->inputs[self->active_in->id].nice_name;
            list     = self->inlist;
            break;
        case 1:
            if (!self->active_out)
                return;
            portflag = JackPortIsInput;
            porttype = JACK_DEFAULT_AUDIO_TYPE;
            nn       = (string)jack_get_client_name(self->jackclient)
                     + ":" + self->strip->plugin->outputs[self->active_out->id].nice_name;
            list     = self->outlist;
            break;
        case 2:
            portflag = JackPortIsOutput;
            porttype = JACK_DEFAULT_MIDI_TYPE;
            nn       = (string)jack_get_client_name(self->jackclient)
                     + ":" + self->strip->plugin->midi_port.nice_name;
            list     = self->midilist;
            break;
    }
    port  = jack_port_by_name(self->jackclient, nn.c_str());
    cons  = jack_port_get_all_connections(self->jackclient, port);
    ports = jack_get_ports(self->jackclient,
                           NULL,
                           porttype,
                           portflag);
    if (!ports) {
        jack_free(cons);
        jack_free(ports);
        return;
    }
    p = 0;
    do {
        sprintf(path, "%d", p);
        gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(list), &iter, path);
        c = f = 0;
        do {
            if (cons and cons[c] == ports[p]) {
                gtk_list_store_set (list, &iter, 2, TRUE, -1);
                f = 1;
            }
        } while (cons && cons[++c]);
        if (!f) {
            gtk_list_store_set (list, &iter, 2, FALSE, -1);
        }
    } while (ports[++p]);
    jack_free(cons);
    jack_free(ports);
}

void calf_connector::_disconnect(int type)
{
    string nn = "";
    const char** cons = NULL;
    jack_port_t *port = NULL;
    int c = 0;
    if (type == 0 or type == -1) {
        for (int i = 0; i < strip->plugin->in_count; i++) {
            nn   = (string)jack_get_client_name(jackclient)
                 + ":" + strip->plugin->inputs[i].nice_name;
            port = jack_port_by_name(jackclient, nn.c_str());
            cons = jack_port_get_all_connections(jackclient, port);
            if (cons) {
                c = 0;
                do {
                    jack_disconnect(jackclient, cons[c], jack_port_name(port));
                } while (cons[++c]);
                jack_free(cons);
            }
        }
    }
    if (type == 1 or type == -1) {
        for (int i = 0; i < strip->plugin->out_count; i++) {
            nn   = (string)jack_get_client_name(jackclient)
                 + ":" + strip->plugin->outputs[i].nice_name;
            port = jack_port_by_name(jackclient, nn.c_str());
            cons = jack_port_get_all_connections(jackclient, port);
            if (cons) {
                c = 0;
                do {
                    jack_disconnect(jackclient, jack_port_name(port), cons[c]);
                } while (cons[++c]);
                jack_free(cons);
            }
        }
    }
    if (type == 2 or type == -1) {
        nn   = (string)jack_get_client_name(jackclient)
             + ":" + strip->plugin->midi_port.nice_name;
        port = jack_port_by_name(jackclient, nn.c_str());
        cons = jack_port_get_all_connections(jackclient, port);
        if (cons) {
            c = 0;
            do {
                jack_disconnect(jackclient, cons[c], jack_port_name(port));
            } while (cons[++c]);
            jack_free(cons);
        }
    }
    set_toggles(this, 0);
    set_toggles(this, 1);
    set_toggles(this, 2);
}

void calf_connector::close()
{
    if (window)
        gtk_widget_destroy(GTK_WIDGET(window));
}
void calf_connector::close_window(GtkWidget *button, gpointer data)
{
    calf_connector *self = (calf_connector *)data;
    if (self->window)
        gtk_widget_destroy(GTK_WIDGET(self->window));
}
void calf_connector::on_destroy_window(GtkWidget *window, gpointer data)
{
    calf_connector *self = (calf_connector *)data;
    if (self->strip->con)
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(self->strip->con), FALSE);
    self->strip->connector = NULL;
    delete self;
}


void calf_connector::create_window()
{
    char buf[256];
    
    // WINDOW
    
    string title = "Calf " + (string)strip->plugin->get_metadata_iface()->get_label() + " Connector";
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), title.c_str());
    gtk_window_set_destroy_with_parent(GTK_WINDOW(window), TRUE);
    //gtk_window_set_keep_above(GTK_WINDOW(window), TRUE);
    gtk_window_set_icon_name(GTK_WINDOW(window), "calf_plugin");
    gtk_window_set_role(GTK_WINDOW(window), "calf_connector");
    gtk_window_set_default_size(GTK_WINDOW(window), 900, 400);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_MOUSE);
    gtk_widget_set_name(GTK_WIDGET(window), "Connector");
    
    g_signal_connect(G_OBJECT(window), "destroy",
                     G_CALLBACK (on_destroy_window), (gpointer)this);
    
    
    // CONTAINER STUFF
    
    GtkVBox *vbox = GTK_VBOX(gtk_vbox_new(false, 8));
    gtk_container_add(GTK_CONTAINER(window), GTK_WIDGET(vbox));
    //gtk_widget_set_name(GTK_WIDGET(vbox), "Calf-Container");
    gtk_container_set_border_width (GTK_CONTAINER(vbox), 8);
    
    GtkTable *table = GTK_TABLE(gtk_table_new(2, 5, FALSE));
    gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(table), true, true, 0);
    gtk_table_set_row_spacings(table, 5);
    gtk_table_set_col_spacings(table, 5);
    
    
    // FRAMES AND CONTAINER
    
    // Button frame
    GtkWidget *butframe = gtk_frame_new("Bulk Disconnect");
    gtk_table_attach(GTK_TABLE(table), GTK_WIDGET(butframe), 1, 4, 1, 2, (GtkAttachOptions)(GTK_FILL), (GtkAttachOptions)(GTK_FILL), 0, 0);
    // Button container
    GtkWidget *buttons = gtk_vbox_new(false, 2);
    gtk_container_add(GTK_CONTAINER(butframe), GTK_WIDGET(buttons));
    
    // Input frame
    GtkWidget *inframe = gtk_frame_new("Plug-In Inputs");
    gtk_table_attach(GTK_TABLE(table), GTK_WIDGET(inframe), 1, 2, 0, 1, (GtkAttachOptions)(GTK_FILL), (GtkAttachOptions)(GTK_FILL|GTK_EXPAND), 0, 0);
    gtk_widget_set_name(GTK_WIDGET(inframe), "Ports");
    gtk_widget_set_size_request(GTK_WIDGET(inframe), 100, -1);
    //gtk_widget_set_size_request(GTK_WIDGET(inframe), -1, 200);
    // Input container
    GtkWidget *inputs = gtk_vbox_new(false, 2);
    gtk_container_add(GTK_CONTAINER(inframe), GTK_WIDGET(inputs));
    // Disconnect input button
    GtkWidget *inbut = gtk_button_new_with_label("Inputs");
    g_signal_connect(G_OBJECT(inbut), "clicked", G_CALLBACK(disconnect_inputs), this);
    gtk_box_pack_start(GTK_BOX(buttons), inbut, false, false, 0);
    gtk_widget_show(GTK_WIDGET(inbut));
    
    // Output frame
    GtkWidget *outframe = gtk_frame_new("Plug-In Outputs");
    gtk_table_attach(GTK_TABLE(table), GTK_WIDGET(outframe), 3, 4, 0, 1, (GtkAttachOptions)(GTK_FILL), (GtkAttachOptions)(GTK_FILL|GTK_EXPAND), 0, 0);
    gtk_widget_set_name(GTK_WIDGET(outframe), "Ports");
    gtk_widget_set_size_request(GTK_WIDGET(outframe), 100, -1);
    //gtk_widget_set_size_request(GTK_WIDGET(outframe), -1, 200);
    // Output container
    GtkWidget *outputs = gtk_vbox_new(false, 2);
    gtk_container_add(GTK_CONTAINER(outframe), GTK_WIDGET(outputs));
    // Disconnect output button
    GtkWidget *outbut = gtk_button_new_with_label("Outputs");
    g_signal_connect(G_OBJECT(outbut), "clicked", G_CALLBACK(disconnect_outputs), this);
    gtk_box_pack_start(GTK_BOX(buttons), outbut, false, false, 0);
    gtk_widget_show(GTK_WIDGET(outbut));
    
    // MIDI frame
    GtkWidget *midiframe = gtk_frame_new("Plug-In MIDI");
    //gtk_table_attach(GTK_TABLE(table), GTK_WIDGET(midiframe), 1, 2, 1, 2, (GtkAttachOptions)GTK_FILL, (GtkAttachOptions)(GTK_FILL), 0, 0);
    // MIDI container
    GtkWidget *midi = gtk_vbox_new(false, 2);
    gtk_container_add(GTK_CONTAINER(midiframe), GTK_WIDGET(midi));
    // Disconnect midi button
    GtkWidget *midibut = gtk_button_new_with_label("MIDI");
    g_signal_connect(G_OBJECT(midibut), "clicked", G_CALLBACK(disconnect_midi), this);
    gtk_box_pack_start(GTK_BOX(buttons), midibut, false, false, 0);
    gtk_widget_show(GTK_WIDGET(midibut));
    
    // LABEL
    GtkWidget *label = gtk_label_new(strip->plugin->get_metadata_iface()->get_label());
    gtk_table_attach(GTK_TABLE(table), GTK_WIDGET(label), 2, 3, 0, 1, (GtkAttachOptions)(GTK_FILL), (GtkAttachOptions)(GTK_FILL), 10, 0);
    gtk_widget_set_name(GTK_WIDGET(label), "Title");
    gtk_label_set_angle(GTK_LABEL(label), 90);
    
    gtk_widget_show_all(window);
    
    // Disconnect all button
    GtkWidget *allbut = gtk_button_new_with_label("*All*");
    g_signal_connect(G_OBJECT(allbut), "clicked", G_CALLBACK(disconnect_all), this);
    gtk_box_pack_start(GTK_BOX(buttons), allbut, false, true, 0);
    gtk_widget_show(GTK_WIDGET(allbut));
    
    // Close button
    //GtkWidget *_close = gtk_alignment_new(1, 1, 0.5, 0.33);
    //gtk_table_attach(GTK_TABLE(table), GTK_WIDGET(_close), 3, 4, 1, 2, (GtkAttachOptions)(GTK_FILL), (GtkAttachOptions)(GTK_FILL), 0, 0);
    GtkWidget *close = gtk_button_new_with_label("Close");
    g_signal_connect(G_OBJECT(close), "clicked", G_CALLBACK(close_window), this);
    //gtk_container_add(GTK_CONTAINER(_close), close);
    gtk_widget_show(GTK_WIDGET(close));
    //gtk_widget_show(GTK_WIDGET(_close));
    gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(close), false, true, 0);
    
    
    // LIST STUFF
    
    GtkTreeViewColumn *col;
    GtkCellRenderer *renderer;
    GtkWidget *inscroller, *outscroller, *midiscroller;
    GtkWidget *inview, *outview, *midiview;
    
    // INPUT LIST
    
    // scroller
    inscroller = gtk_scrolled_window_new(NULL, NULL);
    gtk_table_attach_defaults(GTK_TABLE(table), GTK_WIDGET(inscroller), 0, 1, 0, 1);
    gtk_widget_set_size_request(GTK_WIDGET(inscroller), 300, -1);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(inscroller), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_show(GTK_WIDGET(inscroller));
    
    // list store / tree view
    inlist = gtk_list_store_new(3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_BOOLEAN);
    inview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(inlist));
    gtk_container_add(GTK_CONTAINER(inscroller), inview);
    gtk_widget_show(GTK_WIDGET(inview));
    
    // text column
    col = gtk_tree_view_column_new();
    gtk_tree_view_column_set_expand(col, TRUE);
    gtk_tree_view_column_set_title(col, "Available JACK Audio Outputs");
    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(col, renderer, TRUE);
    gtk_tree_view_column_add_attribute(col, renderer, "text", 1);
    gtk_tree_view_append_column(GTK_TREE_VIEW(inview), col);
    
    // toggle column
    col = gtk_tree_view_column_new();
    gtk_tree_view_column_set_expand(col, FALSE);
    gtk_tree_view_column_set_title(col, "⬌");
    renderer = gtk_cell_renderer_toggle_new();
    gtk_tree_view_column_pack_start(col, renderer, FALSE);
    gtk_tree_view_column_add_attribute(col, renderer, "active", 2);
    gtk_tree_view_append_column(GTK_TREE_VIEW(inview), col);
    g_signal_connect(GTK_OBJECT(renderer), "toggled", G_CALLBACK(inconnector_clicked), (gpointer*)this);
    
    
    gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(inview)),
                                GTK_SELECTION_NONE);
                              
    g_object_unref(inlist);
    
    // OUTPUT LIST
    
    // scroller
    outscroller = gtk_scrolled_window_new(NULL, NULL);
    gtk_table_attach_defaults(GTK_TABLE(table), GTK_WIDGET(outscroller), 4, 5, 0, 2);
    gtk_widget_set_size_request(GTK_WIDGET(outscroller), 280, -1);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(outscroller), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_show(GTK_WIDGET(outscroller));
    
    // list store / tree view
    outlist = gtk_list_store_new(3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_BOOLEAN);
    outview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(outlist));
    gtk_container_add(GTK_CONTAINER(outscroller), outview);
    gtk_widget_show(GTK_WIDGET(outview));
    
    // toggle column
    col = gtk_tree_view_column_new();
    gtk_tree_view_column_set_title(col, "⬌");
    renderer = gtk_cell_renderer_toggle_new();
    gtk_tree_view_column_pack_start(col, renderer, FALSE);
    gtk_tree_view_column_add_attribute(col, renderer, "active", 2);
    gtk_tree_view_append_column(GTK_TREE_VIEW(outview), col);
    g_signal_connect(GTK_OBJECT(renderer), "toggled", G_CALLBACK(outconnector_clicked), (gpointer*)this);
    
    // text column
    col = gtk_tree_view_column_new();
    gtk_tree_view_column_set_title(col, "Available JACK Audio Inputs");
    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(col, renderer, TRUE);
    gtk_tree_view_column_add_attribute(col, renderer, "text", 1);
    gtk_tree_view_append_column(GTK_TREE_VIEW(outview), col);
    
    gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(outview)),
                                GTK_SELECTION_NONE);
                              
    g_object_unref(outlist);
    
    // MIDI LIST
    
    // scroller
    midiscroller = gtk_scrolled_window_new(NULL, NULL);
    gtk_table_attach(GTK_TABLE(table), GTK_WIDGET(midiscroller), 0, 1, 1, 2, (GtkAttachOptions)(GTK_FILL|GTK_EXPAND), (GtkAttachOptions)(GTK_FILL), 0, 0);
    gtk_widget_set_size_request(GTK_WIDGET(midiscroller), 280, -1);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(midiscroller), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_show(GTK_WIDGET(midiscroller));
    
    // list store / tree view
    midilist = gtk_list_store_new(3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_BOOLEAN);
    midiview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(midilist));
    gtk_container_add(GTK_CONTAINER(midiscroller), midiview);
    gtk_widget_show(GTK_WIDGET(midiview));
    
    // text column
    col = gtk_tree_view_column_new();
    gtk_tree_view_column_set_expand(col, TRUE);
    gtk_tree_view_column_set_title(col, "Available JACK MIDI Outputs");
    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(col, renderer, TRUE);
    gtk_tree_view_column_add_attribute(col, renderer, "text", 1);
    gtk_tree_view_append_column(GTK_TREE_VIEW(midiview), col);
    
    // toggle column
    col = gtk_tree_view_column_new();
    gtk_tree_view_column_set_expand(col, FALSE);
    gtk_tree_view_column_set_title(col, "⬌");
    renderer = gtk_cell_renderer_toggle_new();
    gtk_tree_view_column_pack_start(col, renderer, FALSE);
    gtk_tree_view_column_add_attribute(col, renderer, "active", 2);
    gtk_tree_view_append_column(GTK_TREE_VIEW(midiview), col);
    g_signal_connect(GTK_OBJECT(renderer), "toggled", G_CALLBACK(midiconnector_clicked), (gpointer*)this);
    
    gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(midiview)),
                                GTK_SELECTION_NONE);
                              
    g_object_unref(midilist);
    
    // ALL THOSE PORT BUTTONS
    
    active_in = active_out = active_midi = NULL;
    GtkRadioButton *last  = NULL;
    GtkRadioButton *first = NULL;
    int c = 0;
    // Inputs
    for (int i = 0; i < strip->plugin->in_count; i++) {
        sprintf(buf, "Input #%d", (i + 1));
        GtkWidget *in = gtk_radio_button_new_with_label(NULL, buf);
        gtk_box_pack_start(GTK_BOX(inputs), in, false, true, 0);
        gtk_widget_show(GTK_WIDGET(in));
        if (!i)
            last = GTK_RADIO_BUTTON(in);
        else
            gtk_radio_button_set_group(GTK_RADIO_BUTTON(in), gtk_radio_button_get_group(last));
        inports[c].type = 0;
        inports[c].id = c;
        inports[c].connector = this;
        g_signal_connect(GTK_OBJECT(in), "clicked", G_CALLBACK(inport_clicked), (gpointer*)&inports[c]);
        if (!first) {
            first = GTK_RADIO_BUTTON(in);
            active_in = &inports[c];
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(first), TRUE);
        }
        c++;
    }
    if (!c) {
        gtk_widget_set_sensitive(GTK_WIDGET(inframe), FALSE);
        gtk_widget_set_sensitive(GTK_WIDGET(inview), FALSE);
        gtk_widget_set_sensitive(GTK_WIDGET(inbut), FALSE);
        gtk_widget_hide(GTK_WIDGET(inbut));
    }
    first = NULL;
    last  = NULL;
    c = 0;
    // Outputs
    for (int i = 0; i < strip->plugin->out_count; i++) {
        sprintf(buf, "Output #%d", (i + 1));
        GtkWidget *out = gtk_radio_button_new_with_label(NULL, buf);
        gtk_box_pack_start(GTK_BOX(outputs), out, false, true, 0);
        gtk_widget_show(GTK_WIDGET(out));
        if (!i and !last)
            last = GTK_RADIO_BUTTON(out);
        else
            gtk_radio_button_set_group(GTK_RADIO_BUTTON(out), gtk_radio_button_get_group(last));
        outports[c].type = 1;
        outports[c].id = c;
        outports[c].connector = this;
        g_signal_connect(GTK_OBJECT(out), "clicked", G_CALLBACK(outport_clicked), (gpointer*)&outports[c]);
        if (!first) {
            first = GTK_RADIO_BUTTON(out);
            active_out = &outports[c];
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(first), TRUE);
        }
        c++;
    }
    if (!c) {
        gtk_widget_set_sensitive(GTK_WIDGET(outframe), FALSE);
        gtk_widget_set_sensitive(GTK_WIDGET(outview), FALSE);
        gtk_widget_set_sensitive(GTK_WIDGET(outbut), FALSE);
        gtk_widget_hide(GTK_WIDGET(outbut));
    }
    first = NULL;
    last  = NULL;
    c = 0;
    // MIDI
    if (strip->plugin->get_metadata_iface()->get_midi()) {
        GtkWidget *mid = gtk_radio_button_new_with_label(NULL, "MIDI");
        gtk_box_pack_start(GTK_BOX(midi), mid, false, true, 0);
        midiports[c].type = 2;
        midiports[c].id = c;
        midiports[c].connector = this;
        g_signal_connect(GTK_OBJECT(mid), "clicked", G_CALLBACK(midiport_clicked), (gpointer*)&midiports[c]);
        gtk_widget_show(GTK_WIDGET(mid));
        if (!first) {
            first = GTK_RADIO_BUTTON(mid);
            active_midi = &midiports[c];
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(first), TRUE);
        }
        c++;
    }
    if (!c) {
        //gtk_widget_set_sensitive(GTK_WIDGET(midiframe), FALSE);
        //gtk_widget_set_sensitive(GTK_WIDGET(midiview), FALSE);
        //gtk_widget_set_sensitive(GTK_WIDGET(midibut), FALSE);
        gtk_widget_hide(GTK_WIDGET(midiview));
        gtk_widget_hide(GTK_WIDGET(midibut));
        gtk_container_child_set(GTK_CONTAINER(table), GTK_WIDGET(inscroller), "bottom-attach", 2, NULL);
    }
    
    // JACK STUFF
    
    jackclient = strip->plugin->client->client;
    
    //jack_set_port_connect_callback(jackclient, jack_port_connect_callback, this);
    //jack_set_port_rename_callback(jackclient, jack_port_rename_callback, this);
    //jack_set_port_registration_callback(jackclient, jack_port_registration_callback , this);
    
    fill_inlist(this);
    fill_outlist(this);
    fill_midilist(this);
}
