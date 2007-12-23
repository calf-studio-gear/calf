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

#include <vector>
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
    GtkWidget *label, *widget;
    
    param_control() { gui = NULL; param_no = -1; label = NULL; }
    inline parameter_properties &get_props();
    
    virtual GtkWidget *create_label(int param_idx);
    virtual void update_label();
    /// called to create a widget for a control
    virtual GtkWidget *create(plugin_gui *_gui, int _param_no)=0;
    /// called to transfer the value from control to parameter(s)
    virtual void get()=0;
    /// called to transfer the value from parameter(s) to control
    virtual void set()=0;
    virtual ~param_control() {}
};

struct hscale_param_control: public param_control
{
    GtkHScale *scale;

    virtual GtkWidget *create(plugin_gui *_gui, int _param_no);
    virtual void get();
    virtual void set();
    static void hscale_value_changed(GtkHScale *widget, gpointer value);
    static gchar *hscale_format_value(GtkScale *widget, double arg1, gpointer value);
};

struct toggle_param_control: public param_control
{
    GtkCheckButton *scale;

    virtual GtkWidget *create(plugin_gui *_gui, int _param_no);
    virtual void get();
    virtual void set();
    static void toggle_value_changed(GtkCheckButton *widget, gpointer value);
};

struct combo_box_param_control: public param_control
{
    GtkComboBox *scale;

    virtual GtkWidget *create(plugin_gui *_gui, int _param_no);
    virtual void get();
    virtual void set();
    static void combo_value_changed(GtkComboBox *widget, gpointer value);
};

#if USE_PHAT
struct knob_param_control: public param_control
{
    PhatKnob *knob;
    
    virtual GtkWidget *create(plugin_gui *_gui, int _param_no);
    virtual void get();
    virtual void set();
    static void knob_value_changed(PhatKnob *widget, gpointer value);
};
#endif

class plugin_gui_window;

class plugin_gui
{
protected:
    int param_count;
public:
    plugin_gui_window *window;
    GtkWidget *table;
    const char *effect_name;
    plugin_ctl_iface *plugin;
    std::vector<param_control *> params;

    plugin_gui(plugin_gui_window *_window);
    GtkWidget *create(plugin_ctl_iface *_plugin, const char *title);

    void refresh();

#if USE_PHAT
    static void knob_value_changed(PhatKnob *widget, gpointer value);
#endif
};

class plugin_gui_window
{
public:
    plugin_gui *gui;
    GtkWindow *toplevel;
    GtkUIManager *ui_mgr;
    GtkActionGroup *std_actions, *preset_actions;

    plugin_gui_window();
    std::string make_gui_preset_list(GtkActionGroup *grp);
    void fill_gui_presets();
    void create(plugin_ctl_iface *_plugin, const char *title, const char *effect);
    ~plugin_gui_window();
};

inline parameter_properties &param_control::get_props() 
{ 
    return  *gui->plugin->get_param_props(param_no);
}

};

#endif
