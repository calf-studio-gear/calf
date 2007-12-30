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

#include <expat.h>
#include <map>
#include <set>
#include <vector>
#include <gtk/gtk.h>
#if USE_PHAT
#include <phat/phatknob.h>
#endif
#include "custom_ctl.h"

namespace synth {

class plugin_gui;

struct control_base
{
    typedef std::map<std::string, std::string> xml_attribute_map;
    xml_attribute_map attribs;
    plugin_gui *gui;
    void require_attribute(const char *name);
    void require_int_attribute(const char *name);
    int get_int(const char *name, int def_value = 0);
    float get_float(const char *name, float def_value = 0.f);
};

#define _GUARD_CHANGE_ if (in_change) return; guard_change __gc__(this);

struct param_control: public control_base
{    
    int param_no;
    GtkWidget *label, *widget;
    int in_change;
    
    struct guard_change {
        param_control *pc;
        guard_change(param_control *_pc) : pc(_pc) { pc->in_change++; }
        ~guard_change() { pc->in_change--; }
    };
    
    param_control() { gui = NULL; param_no = -1; label = NULL; in_change = 0;}
    inline parameter_properties &get_props();
    
    virtual void init_xml(const char *element) {}
    virtual GtkWidget *create_label();
    virtual void update_label();
    /// called to create a widget for a control
    virtual GtkWidget *create(plugin_gui *_gui, int _param_no)=0;
    /// called to transfer the value from control to parameter(s)
    virtual void get()=0;
    /// called to transfer the value from parameter(s) to control
    virtual void set()=0;
    virtual void hook_params();
    virtual ~param_control() {}
};

struct control_container: public control_base
{
    GtkContainer *container;
    
    virtual GtkWidget *create(plugin_gui *_gui, const char *element, xml_attribute_map &attributes)=0;
    virtual void add(GtkWidget *w, control_base *base) { gtk_container_add(container, w); }
    virtual ~control_container() {}
};

struct table_container: public control_container
{
    virtual GtkWidget *create(plugin_gui *_gui, const char *element, xml_attribute_map &attributes);
    virtual void add(GtkWidget *w, control_base *base);
};

struct alignment_container: public control_container
{
    virtual GtkWidget *create(plugin_gui *_gui, const char *element, xml_attribute_map &attributes);
};

struct frame_container: public control_container
{
    virtual GtkWidget *create(plugin_gui *_gui, const char *element, xml_attribute_map &attributes);
};

struct box_container: public control_container
{
    virtual void add(GtkWidget *w, control_base *base);
};

struct vbox_container: public box_container
{
    virtual GtkWidget *create(plugin_gui *_gui, const char *element, xml_attribute_map &attributes);
};

struct hbox_container: public box_container
{
    virtual GtkWidget *create(plugin_gui *_gui, const char *element, xml_attribute_map &attributes);
};

struct label_param_control: public param_control
{
    virtual GtkWidget *create(plugin_gui *_gui, int _param_no);
    virtual void get() {}
    virtual void set() {}
};

struct value_param_control: public param_control
{
    virtual GtkWidget *create(plugin_gui *_gui, int _param_no);
    virtual void get() {}
    virtual void set();
};

struct hscale_param_control: public param_control
{
    GtkHScale *scale;

    virtual GtkWidget *create(plugin_gui *_gui, int _param_no);
    virtual void get();
    virtual void set();
    virtual void init_xml(const char *element);
    static void hscale_value_changed(GtkHScale *widget, gpointer value);
    static gchar *hscale_format_value(GtkScale *widget, double arg1, gpointer value);
};

struct vscale_param_control: public param_control
{
    GtkHScale *scale;

    virtual GtkWidget *create(plugin_gui *_gui, int _param_no);
    virtual void get();
    virtual void set();
    virtual void init_xml(const char *element);
    static void vscale_value_changed(GtkHScale *widget, gpointer value);
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

struct line_graph_param_control: public param_control
{
    CalfLineGraph *graph;
    int source_id;

    virtual GtkWidget *create(plugin_gui *_gui, int _param_no);
    virtual void get() {}
    virtual void set();
    virtual ~line_graph_param_control();
    static gboolean update(void *data);
};

struct knob_param_control: public param_control
{
#if USE_PHAT
    PhatKnob *knob;
#else
    CalfKnob *knob;
#endif
    
    virtual GtkWidget *create(plugin_gui *_gui, int _param_no);
    virtual void get();
    virtual void set();
    static void knob_value_changed(GtkWidget *widget, gpointer value);
};

class plugin_gui_window;

class plugin_gui
{
protected:
    int param_count;
    std::multimap<int, param_control *> par2ctl;
    XML_Parser parser;
    param_control *current_control;
    std::vector<control_container *> container_stack;
    control_container *top_container;
    std::map<std::string, int> param_name_map;
public:
    plugin_gui_window *window;
    GtkWidget *container;
    const char *effect_name;
    plugin_ctl_iface *plugin;
    std::vector<param_control *> params;

    plugin_gui(plugin_gui_window *_window);
    GtkWidget *create(plugin_ctl_iface *_plugin);
    GtkWidget *create_from_xml(plugin_ctl_iface *_plugin, const char *xml);
    param_control *create_control_from_xml(const char *element, const char *attributes[]);
    control_container *create_container_from_xml(const char *element, const char *attributes[]);

    void add_param_ctl(int param, param_control *ctl) { par2ctl.insert(std::pair<int, param_control *>(param, ctl)); }
    void refresh();
    void xml_element_start(const char *element, const char *attributes[]);
    void set_param_value(int param_no, float value, param_control *originator = NULL);
    static void xml_element_start(void *data, const char *element, const char *attributes[]);
    static void xml_element_end(void *data, const char *element);

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
    static std::set<plugin_gui_window *> all_windows;

    plugin_gui_window();
    std::string make_gui_preset_list(GtkActionGroup *grp);
    void fill_gui_presets();
    void create(plugin_ctl_iface *_plugin, const char *title, const char *effect);
    static void refresh_all_presets();
    ~plugin_gui_window();
};


inline parameter_properties &param_control::get_props() 
{ 
    return  *gui->plugin->get_param_props(param_no);
}

};

#endif
