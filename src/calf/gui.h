/* Calf DSP Library
 * Universal GUI module
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
#ifndef __CALF_GUI_H
#define __CALF_GUI_H

#include <expat.h>
#include <map>
#include <set>
#include <vector>
#include <gtk/gtk.h>
#include "custom_ctl.h"

struct CalfCurve;
struct CalfKeyboard;
struct CalfLed;

namespace calf_plugins {

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
    /// called on DSSI configure()
    virtual void configure(const char *key, const char *value) {}
    virtual void hook_params();
    virtual void on_idle() {}
    virtual ~param_control();
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

struct notebook_container: public control_container
{
    virtual void add(GtkWidget *w, control_base *base);
    virtual GtkWidget *create(plugin_gui *_gui, const char *element, xml_attribute_map &attributes);
};

struct scrolled_container: public control_container
{
    virtual void add(GtkWidget *w, control_base *base);
    virtual GtkWidget *create(plugin_gui *_gui, const char *element, xml_attribute_map &attributes);
};

/// Display-only control: static text
struct label_param_control: public param_control
{
    virtual GtkWidget *create(plugin_gui *_gui, int _param_no);
    virtual void get() {}
    virtual void set() {}
};

/// Display-only control: value text
struct value_param_control: public param_control
{
    virtual GtkWidget *create(plugin_gui *_gui, int _param_no);
    virtual void get() {}
    virtual void set();
};

/// Display-only control: volume meter
struct vumeter_param_control: public param_control
{
    CalfVUMeter *meter;
    
    virtual GtkWidget *create(plugin_gui *_gui, int _param_no);
    virtual void get() {}
    virtual void set();
};

/// Display-only control: LED
struct led_param_control: public param_control
{
    CalfLed *meter;
    
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

struct button_param_control: public param_control
{
    GtkCheckButton *scale;

    virtual GtkWidget *create(plugin_gui *_gui, int _param_no);
    virtual void get();
    virtual void set();
    static void button_clicked(GtkButton *widget, gpointer value);
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

    virtual GtkWidget *create(plugin_gui *_gui, int _param_no);
    virtual void get() {}
    virtual void set();
    virtual void on_idle();
    virtual ~line_graph_param_control();
};

struct knob_param_control: public param_control
{
    CalfKnob *knob;
    
    virtual GtkWidget *create(plugin_gui *_gui, int _param_no);
    virtual void get();
    virtual void set();
    static void knob_value_changed(GtkWidget *widget, gpointer value);
};

struct keyboard_param_control: public param_control
{
    CalfKeyboard *kb;
    
    virtual GtkWidget *create(plugin_gui *_gui, int _param_no);
    virtual void get() {}
    virtual void set() {}
};

struct curve_param_control: public param_control, public send_configure_iface
{
    CalfCurve *curve;
    
    virtual GtkWidget *create(plugin_gui *_gui, int _param_no);
    virtual void get() {}
    virtual void set() {}
    virtual void send_configure(const char *key, const char *value);
};

class plugin_gui_window;

class plugin_gui: public send_configure_iface
{
protected:
    int param_count;
    std::multimap<int, param_control *> par2ctl;
    XML_Parser parser;
    param_control *current_control;
    std::vector<control_container *> container_stack;
    control_container *top_container;
    std::map<std::string, int> param_name_map;
    int ignore_stack;
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
    void refresh(int param_no, param_control *originator = NULL);
    void xml_element_start(const char *element, const char *attributes[]);
    void set_param_value(int param_no, float value, param_control *originator = NULL);
    void send_configure(const char *key, const char *value);
    void on_idle();
    ~plugin_gui();
    static void xml_element_start(void *data, const char *element, const char *attributes[]);
    static void xml_element_end(void *data, const char *element);
};

class main_window_owner_iface;
    
class main_window_iface
{
public:
    virtual void set_owner(main_window_owner_iface *owner)=0;

    virtual void add_plugin(plugin_ctl_iface *plugin)=0;
    virtual void del_plugin(plugin_ctl_iface *plugin)=0;
    
    virtual void set_window(plugin_ctl_iface *plugin, plugin_gui_window *window)=0;
    virtual void refresh_all_presets(bool builtin_too)=0;
    virtual bool check_condition(const char *name)=0;
    virtual ~main_window_iface() {}
};

class main_window_owner_iface
{
public:
    virtual void new_plugin(const char *name) = 0;
    virtual void remove_plugin(plugin_ctl_iface *plugin) = 0;
    virtual ~main_window_owner_iface() {}
};

class plugin_gui_window
{
public:
    plugin_gui *gui;
    GtkWindow *toplevel;
    GtkUIManager *ui_mgr;
    GtkActionGroup *std_actions, *builtin_preset_actions, *user_preset_actions, *command_actions;
    main_window_iface *main;
    int source_id;

    plugin_gui_window(main_window_iface *_main);
    std::string make_gui_preset_list(GtkActionGroup *grp, bool builtin, char &ch);
    std::string make_gui_command_list(GtkActionGroup *grp);
    void fill_gui_presets(bool builtin, char &ch);
    void create(plugin_ctl_iface *_plugin, const char *title, const char *effect);
    void close();
    static gboolean on_idle(void *data);
    ~plugin_gui_window();
};


inline parameter_properties &param_control::get_props() 
{ 
    return  *gui->plugin->get_param_props(param_no);
}

class null_audio_module;

struct activate_command_params
{
    typedef void (*CommandFunc)(null_audio_module *);
    plugin_gui *gui;
    int function_idx;
    
    activate_command_params(plugin_gui *_gui, int _idx)
    : gui(_gui), function_idx(_idx)
    {
    }
};

void activate_command(GtkAction *action, activate_command_params *params);

};

#endif
