/* Calf DSP Library
 * Universal GUI module
 * Copyright (C) 2007 Krzysztof Foltman
 *
 * Note: This module uses phat graphics library, so it's 
 * licensed under GPL license, not LGPL.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */
#ifndef __CALF_GUI_H
#define __CALF_GUI_H

#include <gtk/gtk.h>
#if USE_PHAT
#include <phat/phatknob.h>
#endif

namespace synth {

class plugin_gui;

struct param_control
{
    plugin_gui *gui;
    int param_no;
    GtkWidget *label;
};

struct plugin_ctl_iface
{
    virtual parameter_properties *get_param_props(int param_no) = 0;
    virtual float get_param_value(int param_no) = 0;
    virtual void set_param_value(int param_no, float value) = 0;
    virtual int get_param_count() = 0;
    virtual const char ** get_param_names()=0;
    virtual ~plugin_ctl_iface() {}
};

class plugin_gui
{
protected:
    int param_count;
public:
    GtkWidget *toplevel;
    GtkWidget *table;
    plugin_ctl_iface *plugin;
    param_control *params;

    plugin_gui(GtkWidget *_toplevel);
    void create(plugin_ctl_iface *_plugin, const char *title);
    GtkWidget *create_label(int param_idx);
    static void hscale_value_changed(GtkHScale *widget, gpointer value);
    static gchar *hscale_format_value(GtkScale *widget, double arg1, gpointer value);
    static void combo_value_changed(GtkComboBox *widget, gpointer value);
    static void toggle_value_changed(GtkCheckButton *widget, gpointer value);

#if USE_PHAT
    static void knob_value_changed(PhatKnob *widget, gpointer value);
#endif
};

};

#endif
