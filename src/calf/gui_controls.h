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
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 * Boston, MA 02111-1307, USA.
 */
#ifndef __CALF_GUI_CONTROLS_H
#define __CALF_GUI_CONTROLS_H

#include "gui.h"

struct CalfCurve;
struct CalfKeyboard;
struct CalfLed;

namespace calf_plugins {

/////////////////////////////////////////////////////////////////////////////////////////////
// containers

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

////////////////////////////////////////////////////////////////////////////////////////////////////
// controls

/// Display-only control: static text
struct label_param_control: public param_control
{
    virtual GtkWidget *create(plugin_gui *_gui, int _param_no);
    virtual void get() {}
    virtual void set() {}
};

/// Display-only control: value text
struct value_param_control: public param_control, public send_updates_iface
{
    std::string old_value;

    virtual GtkWidget *create(plugin_gui *_gui, int _param_no);
    virtual void get() {}
    virtual void set();
    virtual void send_status(const char *key, const char *value);
};

/// Display-only control: volume meter
struct vumeter_param_control: public param_control
{
    virtual GtkWidget *create(plugin_gui *_gui, int _param_no);
    virtual void get() {}
    virtual void set();
};

/// Display-only control: LED
struct led_param_control: public param_control
{
    virtual GtkWidget *create(plugin_gui *_gui, int _param_no);
    virtual void get() {}
    virtual void set();
};

/// Display-only control: tube
struct tube_param_control: public param_control
{
    virtual GtkWidget *create(plugin_gui *_gui, int _param_no);
    virtual void get() {}
    virtual void set();
};

/// Horizontal slider
struct hscale_param_control: public param_control
{
    virtual GtkWidget *create(plugin_gui *_gui, int _param_no);
    virtual void get();
    virtual void set();
    virtual void init_xml(const char *element);
    static void hscale_value_changed(GtkHScale *widget, gpointer value);
    static gchar *hscale_format_value(GtkScale *widget, double arg1, gpointer value);
};

/// Vertical slider
struct vscale_param_control: public param_control
{
    virtual GtkWidget *create(plugin_gui *_gui, int _param_no);
    virtual void get();
    virtual void set();
    virtual void init_xml(const char *element);
    static void vscale_value_changed(GtkHScale *widget, gpointer value);
};

/// Spin button
struct spin_param_control: public param_control
{
    virtual GtkWidget *create(plugin_gui *_gui, int _param_no);
    virtual void get();
    virtual void set();
    static void value_changed(GtkSpinButton *widget, gpointer value);
};

/// Check box (Markus Schmidt)
struct check_param_control: public param_control
{
    virtual GtkWidget *create(plugin_gui *_gui, int _param_no);
    virtual void get();
    virtual void set();
    
    static void check_value_changed(GtkCheckButton *widget, gpointer value);
};

/// Toggle Button
struct toggle_param_control: public param_control
{
    virtual GtkWidget *create(plugin_gui *_gui, int _param_no);
    virtual void get();
    virtual void set();
    
    static void toggle_value_changed(GtkWidget *widget, gpointer value);
};

/// Radio button
struct radio_param_control: public param_control
{
    int value;
    
    virtual GtkWidget *create(plugin_gui *_gui, int _param_no);
    virtual void get();
    virtual void set();
    
    static void radio_clicked(GtkRadioButton *widget, gpointer value);
};

/// Push button
struct button_param_control: public param_control
{
    virtual GtkWidget *create(plugin_gui *_gui, int _param_no);
    virtual void get();
    virtual void set();
    static void button_clicked(GtkButton *widget, gpointer value);
    static void button_press_event(GtkButton *widget, GdkEvent *event, gpointer value);
};

/// Combo list box
struct combo_box_param_control: public param_control, public send_updates_iface
{
    GtkListStore *lstore;
    std::map<std::string, GtkTreeIter> key2pos;
    std::string last_key;
    
    virtual GtkWidget *create(plugin_gui *_gui, int _param_no);
    virtual void get();
    virtual void set();
    virtual void send_status(const char *key, const char *value);
    void set_to_last_key();
    static void combo_value_changed(GtkComboBox *widget, gpointer value);
};

/// Line graph
struct line_graph_param_control: public param_control
{
    int last_generation;

    virtual GtkWidget *create(plugin_gui *_gui, int _param_no);
    virtual void get() {}
    virtual void set();
    virtual void on_idle();
    virtual ~line_graph_param_control();
};

/// Phase graph
struct phase_graph_param_control: public param_control
{
    int last_generation;

    virtual GtkWidget *create(plugin_gui *_gui, int _param_no);
    virtual void get() {}
    virtual void set();
    virtual void on_idle();
    virtual ~phase_graph_param_control();
};

/// Knob
struct knob_param_control: public param_control
{
    virtual GtkWidget *create(plugin_gui *_gui, int _param_no);
    virtual void get();
    virtual void set();
    static void knob_value_changed(GtkWidget *widget, gpointer value);
};

/// Static keyboard image
struct keyboard_param_control: public param_control
{
    CalfKeyboard *kb;
    
    virtual GtkWidget *create(plugin_gui *_gui, int _param_no);
    virtual void get() {}
    virtual void set() {}
};

/// Curve editor
struct curve_param_control: public param_control, public send_configure_iface
{
    CalfCurve *curve;
    
    virtual GtkWidget *create(plugin_gui *_gui, int _param_no);
    virtual void get() {}
    virtual void set() {}
    virtual void send_configure(const char *key, const char *value);
};

/// Text entry
struct entry_param_control: public param_control, public send_configure_iface
{
    GtkEntry *entry;
    
    virtual GtkWidget *create(plugin_gui *_gui, int _param_no);
    virtual void get() {}
    virtual void set() {}
    virtual void send_configure(const char *key, const char *value);
    static void entry_value_changed(GtkWidget *widget, gpointer value);
};

/// File chooser button
struct filechooser_param_control: public param_control, public send_configure_iface
{
    GtkFileChooserButton *filechooser;
    
    virtual GtkWidget *create(plugin_gui *_gui, int _param_no);
    virtual void get() {}
    virtual void set() {}
    virtual void send_configure(const char *key, const char *value);
    static void filechooser_value_changed(GtkWidget *widget, gpointer value);
};

/// List view used for variable-length tabular data
struct listview_param_control: public param_control, public send_configure_iface
{
    GtkTreeView *tree;
    GtkListStore *lstore;
    const calf_plugins::table_metadata_iface *tmif;
    int cols;
    std::vector<GtkTreeIter> positions;
    
    virtual GtkWidget *create(plugin_gui *_gui, int _param_no);
    virtual void get() {}
    virtual void set() {}
    virtual void send_configure(const char *key, const char *value);
protected:
    void set_rows(unsigned int needed_rows);
    static void on_edited(GtkCellRenderer *renderer, gchar *path, gchar *new_text, listview_param_control *pThis);
    static void on_editing_canceled(GtkCellRenderer *renderer, listview_param_control *pThis);    
};

};

#endif
