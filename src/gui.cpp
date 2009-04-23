/* Calf DSP Library
 * GUI functions for a plugin.
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
 * Boston, MA  02110-1301  USA
 */
 
#include <config.h>
#include <assert.h>
#include <calf/ctl_curve.h>
#include <calf/ctl_keyboard.h>
#include <calf/ctl_led.h>
#include <calf/giface.h>
#include <calf/gui.h>
#include <calf/preset.h>
#include <calf/preset_gui.h>
#include <calf/main_win.h>

#include <iostream>

using namespace calf_plugins;
using namespace std;

/******************************** controls ********************************/

GtkWidget *param_control::create_label()
{
    label = gtk_label_new ("");
    gtk_label_set_width_chars (GTK_LABEL (label), 12);
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    return label;
}

void param_control::update_label()
{
    parameter_properties &props = get_props();
    gtk_label_set_text (GTK_LABEL (label), props.to_string(gui->plugin->get_param_value(param_no)).c_str());
}

void param_control::hook_params()
{
    if (param_no != -1) {
        gui->add_param_ctl(param_no, this);
    }
    gui->params.push_back(this);
}

param_control::~param_control()
{
    if (label)
        gtk_widget_destroy(label);
    if (widget)
        gtk_widget_destroy(widget);
}

// combo box

GtkWidget *combo_box_param_control::create(plugin_gui *_gui, int _param_no)
{
    gui = _gui;
    param_no = _param_no;
    lstore = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING); // value, key
    
    parameter_properties &props = get_props();
    widget  = gtk_combo_box_new_text ();
    if (props.choices)
    {
        for (int j = (int)props.min; j <= (int)props.max; j++)
            gtk_list_store_insert_with_values (lstore, NULL, j - (int)props.min, 0, props.choices[j - (int)props.min], 1, calf_utils::i2s(j).c_str(), -1);
    }
    gtk_combo_box_set_model (GTK_COMBO_BOX(widget), GTK_TREE_MODEL(lstore));
    gtk_signal_connect (GTK_OBJECT (widget), "changed", G_CALLBACK (combo_value_changed), (gpointer)this);
    
    return widget;
}

void combo_box_param_control::set()
{
    _GUARD_CHANGE_
    parameter_properties &props = get_props();
    gtk_combo_box_set_active (GTK_COMBO_BOX (widget), (int)gui->plugin->get_param_value(param_no) - (int)props.min);
}

void combo_box_param_control::get()
{
    parameter_properties &props = get_props();
    gui->set_param_value(param_no, gtk_combo_box_get_active (GTK_COMBO_BOX(widget)) + props.min, this);
}

void combo_box_param_control::combo_value_changed(GtkComboBox *widget, gpointer value)
{
    combo_box_param_control *jhp = (combo_box_param_control *)value;
    if (jhp->attribs.count("setter-key"))
    {
        GtkTreeIter iter;
        gchar *key = NULL;
        if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (jhp->widget), &iter))
        {
            gtk_tree_model_get (GTK_TREE_MODEL (jhp->lstore), &iter, 1, &key, -1);
            if (key) {
                jhp->gui->plugin->configure(jhp->attribs["setter-key"].c_str(), key);
                free(key);
            }
        }
    }
    else
        jhp->get();
}

void combo_box_param_control::send_status(const char *key, const char *value)
{
    if (attribs.count("key") && key == attribs["key"])
    {
        gtk_list_store_clear (lstore);
        key2pos.clear();
        std::string v = value;
        int i = 0;
        size_t pos = 0;
        while (pos < v.length()) {
            size_t endpos = v.find("\n", pos);
            if (endpos == string::npos)
                break;
            string line = v.substr(pos, endpos - pos);
            string key, label;
            size_t tabpos = line.find('\t');
            if (tabpos == string::npos)
                key = label = line;
            else {
                key = line.substr(0, tabpos);
                label = line.substr(tabpos + 1);
            }
            GtkTreeIter gti;
            gtk_list_store_insert_with_values (lstore, &gti, i, 0, label.c_str(), 1, key.c_str(), -1);
            key2pos[key] = gti;
            pos = endpos + 1;
            i++;
        }
        set_to_last_key();
    }
    if (attribs.count("current-key") && key == attribs["current-key"])
    {
        last_key = value;
        set_to_last_key();
    }
}

void combo_box_param_control::set_to_last_key()
{
    map<string, GtkTreeIter>::iterator i = key2pos.find(last_key);
    if (i != key2pos.end())
    {
        gtk_combo_box_set_active_iter (GTK_COMBO_BOX (widget), &i->second);
        
    }
    else
        gtk_combo_box_set_active (GTK_COMBO_BOX (widget), -1);
}

// horizontal fader

GtkWidget *hscale_param_control::create(plugin_gui *_gui, int _param_no)
{
    gui = _gui;
    param_no = _param_no;

    widget = gtk_hscale_new_with_range (0, 1, get_props().get_increment());
    gtk_signal_connect (GTK_OBJECT (widget), "value-changed", G_CALLBACK (hscale_value_changed), (gpointer)this);
    gtk_signal_connect (GTK_OBJECT (widget), "format-value", G_CALLBACK (hscale_format_value), (gpointer)this);
    gtk_widget_set_size_request (widget, 200, -1);

    return widget;
}

void hscale_param_control::init_xml(const char *element)
{
    if (attribs.count("width"))
        gtk_widget_set_size_request (widget, get_int("width", 200), -1);
    if (attribs.count("position"))
    {
        string v = attribs["position"];
        if (v == "top") gtk_scale_set_value_pos(GTK_SCALE(widget), GTK_POS_TOP);
        if (v == "bottom") gtk_scale_set_value_pos(GTK_SCALE(widget), GTK_POS_BOTTOM);
    }
}

void hscale_param_control::set()
{
    _GUARD_CHANGE_
    parameter_properties &props = get_props();
    gtk_range_set_value (GTK_RANGE (widget), props.to_01 (gui->plugin->get_param_value(param_no)));
    // hscale_value_changed (GTK_HSCALE (widget), (gpointer)this);
}

void hscale_param_control::get()
{
    parameter_properties &props = get_props();
    float cvalue = props.from_01 (gtk_range_get_value (GTK_RANGE (widget)));
    gui->set_param_value(param_no, cvalue, this);
}

void hscale_param_control::hscale_value_changed(GtkHScale *widget, gpointer value)
{
    hscale_param_control *jhp = (hscale_param_control *)value;
    jhp->get();
}

gchar *hscale_param_control::hscale_format_value(GtkScale *widget, double arg1, gpointer value)
{
    hscale_param_control *jhp = (hscale_param_control *)value;
    const parameter_properties &props = jhp->get_props();
    float cvalue = props.from_01 (arg1);
    
    // for testing
    // return g_strdup_printf ("%s = %g", props.to_string (cvalue).c_str(), arg1);
    return g_strdup (props.to_string (cvalue).c_str());
}

// vertical fader

GtkWidget *vscale_param_control::create(plugin_gui *_gui, int _param_no)
{
    gui = _gui;
    param_no = _param_no;

    widget = gtk_vscale_new_with_range (0, 1, get_props().get_increment());
    gtk_signal_connect (GTK_OBJECT (widget), "value-changed", G_CALLBACK (vscale_value_changed), (gpointer)this);
    gtk_scale_set_draw_value(GTK_SCALE(widget), FALSE);
    gtk_widget_set_size_request (widget, -1, 200);

    return widget;
}

void vscale_param_control::init_xml(const char *element)
{
    if (attribs.count("height"))
        gtk_widget_set_size_request (widget, -1, get_int("height", 200));
}

void vscale_param_control::set()
{
    _GUARD_CHANGE_
    parameter_properties &props = get_props();
    gtk_range_set_value (GTK_RANGE (widget), props.to_01 (gui->plugin->get_param_value(param_no)));
    // vscale_value_changed (GTK_HSCALE (widget), (gpointer)this);
}

void vscale_param_control::get()
{
    parameter_properties &props = get_props();
    float cvalue = props.from_01 (gtk_range_get_value (GTK_RANGE (widget)));
    gui->set_param_value(param_no, cvalue, this);
}

void vscale_param_control::vscale_value_changed(GtkHScale *widget, gpointer value)
{
    vscale_param_control *jhp = (vscale_param_control *)value;
    jhp->get();
}

// label

GtkWidget *label_param_control::create(plugin_gui *_gui, int _param_no)
{
    gui = _gui, param_no = _param_no;
    string text;
    if (param_no != -1)
        text = get_props().name;
    else
        text = attribs["text"];
    widget = gtk_label_new(text.c_str());
    gtk_misc_set_alignment (GTK_MISC (widget), get_float("align-x", 0.5), get_float("align-y", 0.5));
    return widget;
}

// value

GtkWidget *value_param_control::create(plugin_gui *_gui, int _param_no)
{
    gui = _gui;
    param_no = _param_no;
    
    widget = gtk_label_new ("");
    if (param_no != -1)
    {
        parameter_properties &props = get_props();
        gtk_label_set_width_chars (GTK_LABEL (widget), props.get_char_count());
    }
    else
    {
        require_attribute("key");
        require_int_attribute("width");
        param_variable = attribs["key"];
        gtk_label_set_width_chars (GTK_LABEL (widget), get_int("width"));        
    }
    gtk_misc_set_alignment (GTK_MISC (widget), get_float("align-x", 0.5), get_float("align-y", 0.5));
    return widget;
}

void value_param_control::set()
{
    if (param_no == -1)
        return;
    _GUARD_CHANGE_
    parameter_properties &props = get_props();
    gtk_label_set_text (GTK_LABEL (widget), props.to_string(gui->plugin->get_param_value(param_no)).c_str());    
}

void value_param_control::send_status(const char *key, const char *value)
{
    if (key == param_variable)
    {
        gtk_label_set_text (GTK_LABEL (widget), value);    
    }
}

// VU meter

GtkWidget *vumeter_param_control::create(plugin_gui *_gui, int _param_no)
{
    gui = _gui, param_no = _param_no;
    // parameter_properties &props = get_props();
    widget = calf_vumeter_new ();
    calf_vumeter_set_mode (CALF_VUMETER (widget), (CalfVUMeterMode)get_int("mode", 0));
    return widget;
}

void vumeter_param_control::set()
{
    _GUARD_CHANGE_
    parameter_properties &props = get_props();
    calf_vumeter_set_value (CALF_VUMETER (widget), props.to_01(gui->plugin->get_param_value(param_no)));
    if (label)
        update_label();
}

// LED

GtkWidget *led_param_control::create(plugin_gui *_gui, int _param_no)
{
    gui = _gui, param_no = _param_no;
    // parameter_properties &props = get_props();
    widget = calf_led_new ();
    return widget;
}

void led_param_control::set()
{
    _GUARD_CHANGE_
    // parameter_properties &props = get_props();
    calf_led_set_state (CALF_LED (widget), gui->plugin->get_param_value(param_no) > 0);
    if (label)
        update_label();
}

// check box

GtkWidget *toggle_param_control::create(plugin_gui *_gui, int _param_no)
{
    gui = _gui;
    param_no = _param_no;
    
    widget  = gtk_check_button_new ();
    gtk_signal_connect (GTK_OBJECT (widget), "toggled", G_CALLBACK (toggle_value_changed), (gpointer)this);
    return widget;
}

void toggle_param_control::toggle_value_changed(GtkCheckButton *widget, gpointer value)
{
    param_control *jhp = (param_control *)value;
    jhp->get();
}

void toggle_param_control::get()
{
    const parameter_properties &props = get_props();
    gui->set_param_value(param_no, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(widget)) + props.min, this);
}

void toggle_param_control::set()
{
    _GUARD_CHANGE_
    const parameter_properties &props = get_props();
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), (int)gui->plugin->get_param_value(param_no) - (int)props.min);
}

// spin button

GtkWidget *spin_param_control::create(plugin_gui *_gui, int _param_no)
{
    gui = _gui;
    param_no = _param_no;
    
    const parameter_properties &props = get_props();
    if (props.step > 1)
        widget  = gtk_spin_button_new_with_range (props.min, props.max, (props.max - props.min) / (props.step - 1));
    if (props.step > 0)
        widget  = gtk_spin_button_new_with_range (props.min, props.max, props.step);
    else
        widget  = gtk_spin_button_new_with_range (props.min, props.max, 1);
    gtk_spin_button_set_digits (GTK_SPIN_BUTTON(widget), get_int("digits", 0));
    gtk_signal_connect (GTK_OBJECT (widget), "value-changed", G_CALLBACK (value_changed), (gpointer)this);
    return widget;
}

void spin_param_control::value_changed(GtkSpinButton *widget, gpointer value)
{
    param_control *jhp = (param_control *)value;
    jhp->get();
}

void spin_param_control::get()
{
    // const parameter_properties &props = get_props();
    gui->set_param_value(param_no, gtk_spin_button_get_value_as_float (GTK_SPIN_BUTTON (widget)), this);
}

void spin_param_control::set()
{
    _GUARD_CHANGE_
    // const parameter_properties &props = get_props();
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (widget), gui->plugin->get_param_value(param_no));
}

// button

GtkWidget *button_param_control::create(plugin_gui *_gui, int _param_no)
{
    gui = _gui;
    param_no = _param_no;
    
    widget  = gtk_button_new_with_label (get_props().name);
    gtk_signal_connect (GTK_OBJECT (widget), "clicked", G_CALLBACK (button_clicked), (gpointer)this);
    return widget;
}

void button_param_control::button_clicked(GtkButton *widget, gpointer value)
{
    param_control *jhp = (param_control *)value;
    
    jhp->get();
}

void button_param_control::get()
{
    const parameter_properties &props = get_props();
    gui->set_param_value(param_no, props.max, this);
}

void button_param_control::set()
{
    _GUARD_CHANGE_
    const parameter_properties &props = get_props();
    if (gui->plugin->get_param_value(param_no) - props.min >= 0.5)
        gtk_button_clicked (GTK_BUTTON (widget));
}

// knob

GtkWidget *knob_param_control::create(plugin_gui *_gui, int _param_no)
{
    gui = _gui;
    param_no = _param_no;
    const parameter_properties &props = get_props();
    
    //widget = calf_knob_new_with_range (props.to_01 (gui->plugin->get_param_value(param_no)), 0, 1, 0.01);
    widget = calf_knob_new();
    float increment = props.get_increment();
    gtk_range_get_adjustment(GTK_RANGE(widget))->step_increment = increment;
    CALF_KNOB(widget)->knob_type = get_int("type");
    gtk_signal_connect(GTK_OBJECT(widget), "value-changed", G_CALLBACK(knob_value_changed), (gpointer)this);
    return widget;
}

void knob_param_control::get()
{
    const parameter_properties &props = get_props();
    float value = props.from_01(gtk_range_get_value(GTK_RANGE(widget)));
    gui->set_param_value(param_no, value, this);
    if (label)
        update_label();
}

void knob_param_control::set()
{
    _GUARD_CHANGE_
    const parameter_properties &props = get_props();
    gtk_range_set_value(GTK_RANGE(widget), props.to_01 (gui->plugin->get_param_value(param_no)));
    if (label)
        update_label();
}

void knob_param_control::knob_value_changed(GtkWidget *widget, gpointer value)
{
    param_control *jhp = (param_control *)value;
    jhp->get();
}

// keyboard

GtkWidget *keyboard_param_control::create(plugin_gui *_gui, int _param_no)
{
    gui = _gui;
    param_no = _param_no;
    // const parameter_properties &props = get_props();
    
    widget = calf_keyboard_new();
    kb = CALF_KEYBOARD(widget);
    kb->nkeys = get_int("octaves", 4) * 7 + 1;
    kb->sink = new CalfKeyboard::EventAdapter;
    return widget;
}

// curve

struct curve_param_control_callback: public CalfCurve::EventAdapter
{
    curve_param_control *ctl;
    
    curve_param_control_callback(curve_param_control *_ctl)
    : ctl(_ctl) {}
    
    virtual void curve_changed(CalfCurve *src, const CalfCurve::point_vector &data) {
        stringstream ss;
        ss << data.size() << endl;
        for (size_t i = 0; i < data.size(); i++)
            ss << data[i].first << " " << data[i].second << endl;
        ctl->gui->plugin->configure(ctl->attribs["key"].c_str(), ss.str().c_str());
    }
    virtual void clip(CalfCurve *src, int pt, float &x, float &y, bool &hide)
    {
        // int gridpt = floor(x * 71 * 2);
        // clip to the middle of the nearest white key
        x = (floor(x * 71) + 0.5)/ 71.0;
    }
};

GtkWidget *curve_param_control::create(plugin_gui *_gui, int _param_no)
{
    gui = _gui;
    param_no = _param_no;
    require_attribute("key");
    
    widget = calf_curve_new(get_int("maxpoints", -1));
    curve = CALF_CURVE(widget);
    curve->sink = new curve_param_control_callback(this);
    // gtk_curve_set_curve_type(curve, GTK_CURVE_TYPE_LINEAR);
    return widget;
}

void curve_param_control::send_configure(const char *key, const char *value)
{
    // cout << "send conf " << key << endl;
    if (attribs["key"] == key)
    {
        stringstream ss(value);
        CalfCurve::point_vector pts;
        if (*value)
        {
            unsigned int npoints = 0;
            ss >> npoints;
            unsigned int i;
            float x = 0, y = 0;
            for (i = 0; i < npoints && i < curve->point_limit; i++)
            {
                ss >> x >> y;
                pts.push_back(CalfCurve::point(x, y));
            }
            calf_curve_set_points(widget, pts);
        }
    }
}

// entry

GtkWidget *entry_param_control::create(plugin_gui *_gui, int _param_no)
{
    gui = _gui;
    param_no = _param_no;
    require_attribute("key");
    
    widget = gtk_entry_new();
    entry = GTK_ENTRY(widget);
    gtk_signal_connect(GTK_OBJECT(widget), "changed", G_CALLBACK(entry_value_changed), (gpointer)this);
    gtk_editable_set_editable(GTK_EDITABLE(entry), get_int("editable", 1));
    return widget;
}

void entry_param_control::send_configure(const char *key, const char *value)
{
    // cout << "send conf " << key << endl;
    if (attribs["key"] == key)
    {
        gtk_entry_set_text(entry, value);
    }
}

void entry_param_control::entry_value_changed(GtkWidget *widget, gpointer value)
{
    entry_param_control *ctl = (entry_param_control *)value;
    ctl->gui->plugin->configure(ctl->attribs["key"].c_str(), gtk_entry_get_text(ctl->entry));
}

// filechooser

GtkWidget *filechooser_param_control::create(plugin_gui *_gui, int _param_no)
{
    gui = _gui;
    param_no = _param_no;
    require_attribute("key");
    require_attribute("title");
    
    widget = gtk_file_chooser_button_new(attribs["title"].c_str(), GTK_FILE_CHOOSER_ACTION_OPEN);
    filechooser = GTK_FILE_CHOOSER_BUTTON(widget);
    // XXXKF this is GTK+ 2.12 function, does any replacement exist?
    gtk_signal_connect(GTK_OBJECT(widget), "file-set", G_CALLBACK(filechooser_value_changed), (gpointer)this);
    if (attribs.count("width"))
        gtk_widget_set_size_request (widget, get_int("width", 200), -1);
    if (attribs.count("width_chars"))
         gtk_file_chooser_button_set_width_chars (filechooser, get_int("width_chars"));
    return widget;
}

void filechooser_param_control::send_configure(const char *key, const char *value)
{
    // cout << "send conf " << key << endl;
    if (attribs["key"] == key)
    {
        gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(filechooser), value);
    }
}

void filechooser_param_control::filechooser_value_changed(GtkWidget *widget, gpointer value)
{
    filechooser_param_control *ctl = (filechooser_param_control *)value;
    const char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(ctl->filechooser));
    if (filename)
        ctl->gui->plugin->configure(ctl->attribs["key"].c_str(), filename);
}

// line graph

void line_graph_param_control::on_idle()
{
    if (get_int("refresh", 0))
        set();
}

GtkWidget *line_graph_param_control::create(plugin_gui *_gui, int _param_no)
{
    gui = _gui;
    param_no = _param_no;
    last_generation = -1;
    // const parameter_properties &props = get_props();
    
    widget = calf_line_graph_new ();
    CalfLineGraph *clg = CALF_LINE_GRAPH(widget);
    widget->requisition.width = get_int("width", 40);
    widget->requisition.height = get_int("height", 40);
    calf_line_graph_set_square(clg, get_int("square", 0));
    clg->source = gui->plugin->get_line_graph_iface();
    clg->source_id = param_no;
        
    return widget;
}

void line_graph_param_control::set()
{
    GtkWidget *tw = gtk_widget_get_toplevel(widget);
    if (tw && GTK_WIDGET_TOPLEVEL(tw) && widget->window)
    {
        int ws = gdk_window_get_state(widget->window);
        if (ws & (GDK_WINDOW_STATE_WITHDRAWN | GDK_WINDOW_STATE_ICONIFIED))
            return;
        last_generation = calf_line_graph_update_if(CALF_LINE_GRAPH(widget), last_generation);
    }
}

line_graph_param_control::~line_graph_param_control()
{
}

// list view

GtkWidget *listview_param_control::create(plugin_gui *_gui, int _param_no)
{
    gui = _gui;
    param_no = _param_no;
    
    teif = gui->plugin->get_table_edit_iface();
    const table_column_info *tci = teif->get_table_columns(param_no);
    assert(tci);
    cols = 0;
    while (tci[cols].name != NULL)
        cols++;
    
    GType *p = new GType[cols];
    for (int i = 0; i < cols; i++)
        p[i] = G_TYPE_STRING;
    lstore = gtk_list_store_newv(cols, p);
    update_store("");
    widget = gtk_tree_view_new_with_model(GTK_TREE_MODEL(lstore));
    delete []p;
    tree = GTK_TREE_VIEW(widget);
    assert(teif);
    
    for (int i = 0; i < cols; i++)
    {
        GtkCellRenderer *cr = NULL;
        
        if (tci[i].type == TCT_ENUM) {
            cr = gtk_cell_renderer_combo_new ();
            GtkListStore *cls = gtk_list_store_new(2, G_TYPE_INT, G_TYPE_STRING);
            for (int j = 0; tci[i].values[j]; j++)
                gtk_list_store_insert_with_values(cls, NULL, j, 0, j, 1, tci[i].values[j], -1);
            g_object_set(cr, "model", cls, "editable", TRUE, "has-entry", FALSE, "text-column", 1, "mode", GTK_CELL_RENDERER_MODE_EDITABLE, NULL);
        }
        else {
            bool editable = tci[i].type != TCT_LABEL;
            cr = gtk_cell_renderer_text_new ();
            if (editable)
                g_object_set(cr, "editable", TRUE, "mode", GTK_CELL_RENDERER_MODE_EDITABLE, NULL);
        }
        g_object_set_data (G_OBJECT(cr), "column", (void *)&tci[i]);
        gtk_signal_connect (GTK_OBJECT (cr), "edited", G_CALLBACK (on_edited), (gpointer)this);
        gtk_signal_connect (GTK_OBJECT (cr), "editing-canceled", G_CALLBACK (on_editing_canceled), (gpointer)this);
        gtk_tree_view_insert_column_with_attributes(tree, i, tci[i].name, cr, "text", i, NULL);
    }
    gtk_tree_view_set_headers_visible(tree, TRUE);
    
    return widget;
}

void listview_param_control::update_store(const std::string &data)
{
    gtk_list_store_clear(lstore);
    uint32_t rows = teif->get_table_rows(param_no);
    for (uint32_t i = 0; i < rows; i++)
    {
        GtkTreeIter iter;
        gtk_list_store_insert(lstore, &iter, i);
        for (int j = 0; j < cols; j++)
        {
            gtk_list_store_set(lstore, &iter, j, teif->get_cell(i, j).c_str(), -1);
        }
        positions.push_back(iter);
    }
}

void listview_param_control::send_configure(const char *key, const char *value)
{
    if (attribs["key"] == key)
    {
        update_store(value);
    }
}

void listview_param_control::on_edited(GtkCellRenderer *renderer, gchar *path, gchar *new_text, listview_param_control *pThis)
{
    const table_column_info *tci = pThis->teif->get_table_columns(pThis->param_no);
    gtk_list_store_set(pThis->lstore, &pThis->positions[atoi(path)], ((table_column_info *)g_object_get_data(G_OBJECT(renderer), "column")) - tci, new_text, -1);
    gtk_widget_grab_focus(pThis->widget);
}

void listview_param_control::on_editing_canceled(GtkCellRenderer *renderer, listview_param_control *pThis)
{
    gtk_widget_grab_focus(pThis->widget);
}

/******************************** GUI proper ********************************/

plugin_gui::plugin_gui(plugin_gui_window *_window)
: last_status_serial_no(0)
, window(_window)
{
    ignore_stack = 0;
    top_container = NULL;
    current_control = NULL;
    param_count = 0;
    container = NULL;
    effect_name = NULL;
}

static void window_destroyed(GtkWidget *window, gpointer data)
{
    delete (plugin_gui_window *)data;
}

static void action_destroy_notify(gpointer data)
{
    delete (activate_preset_params *)data;
}

void control_base::require_attribute(const char *name)
{
    if (attribs.count(name) == 0) {
        g_error("Missing attribute: %s", name);
    }
}

void control_base::require_int_attribute(const char *name)
{
    if (attribs.count(name) == 0) {
        g_error("Missing attribute: %s", name);
    }
    if (attribs[name].empty() || attribs[name].find_first_not_of("0123456789") != string::npos) {
        g_error("Wrong data type on attribute: %s (required integer)", name);
    }
}

int control_base::get_int(const char *name, int def_value)
{
    if (attribs.count(name) == 0)
        return def_value;
    const std::string &v = attribs[name];
    if (v.empty() || v.find_first_not_of("-+0123456789") != string::npos)
        return def_value;
    return atoi(v.c_str());
}

float control_base::get_float(const char *name, float def_value)
{
    if (attribs.count(name) == 0)
        return def_value;
    const std::string &v = attribs[name];
    if (v.empty() || v.find_first_not_of("-+0123456789.") != string::npos)
        return def_value;
    stringstream ss(v);
    float value;
    ss >> value;
    return value;
}

/******************************** GtkTable container ********************************/

GtkWidget *table_container::create(plugin_gui *_gui, const char *element, xml_attribute_map &attributes)
{
    require_int_attribute("rows");
    require_int_attribute("cols");
    GtkWidget *table = gtk_table_new(get_int("rows", 1), get_int("cols", 1), false);
    container = GTK_CONTAINER(table);
    return table;
}

void table_container::add(GtkWidget *widget, control_base *base)
{
    base->require_int_attribute("attach-x");
    base->require_int_attribute("attach-y");
    int x = base->get_int("attach-x"), y = base->get_int("attach-y");
    int w = base->get_int("attach-w", 1), h = base->get_int("attach-h", 1);
    int shrinkx = base->get_int("shrink-x", 0);
    int shrinky = base->get_int("shrink-y", 0);
    int fillx = (base->get_int("fill-x", !shrinkx) ? GTK_FILL : 0) | (base->get_int("expand-x", !shrinkx) ? GTK_EXPAND : 0) | (shrinkx ? GTK_SHRINK : 0);
    int filly = (base->get_int("fill-y", !shrinky) ? GTK_FILL : 0) | (base->get_int("expand-y", !shrinky) ? GTK_EXPAND : 0) | (base->get_int("shrink-y", 0) ? GTK_SHRINK : 0);
    int padx = base->get_int("pad-x", 2);
    int pady = base->get_int("pad-y", 2);
    gtk_table_attach(GTK_TABLE(container), widget, x, x + w, y, y + h, (GtkAttachOptions)fillx, (GtkAttachOptions)filly, padx, pady);
} 

/******************************** alignment contaner ********************************/

GtkWidget *alignment_container::create(plugin_gui *_gui, const char *element, xml_attribute_map &attributes)
{
    GtkWidget *align = gtk_alignment_new(get_float("align-x", 0.5), get_float("align-y", 0.5), get_float("scale-x", 0), get_float("scale-y", 0));
    container = GTK_CONTAINER(align);
    return align;
}

/******************************** GtkFrame contaner ********************************/

GtkWidget *frame_container::create(plugin_gui *_gui, const char *element, xml_attribute_map &attributes)
{
    GtkWidget *frame = gtk_frame_new(attribs["label"].c_str());
    container = GTK_CONTAINER(frame);
    return frame;
}

/******************************** GtkBox type of containers ********************************/

void box_container::add(GtkWidget *w, control_base *base)
{
    gtk_container_add_with_properties(container, w, "expand", get_int("expand", 1), "fill", get_int("fill", 1), NULL);
}

/******************************** GtkHBox container ********************************/

GtkWidget *hbox_container::create(plugin_gui *_gui, const char *element, xml_attribute_map &attributes)
{
    GtkWidget *hbox = gtk_hbox_new(false, get_int("spacing", 2));
    container = GTK_CONTAINER(hbox);
    return hbox;
}

/******************************** GtkVBox container ********************************/

GtkWidget *vbox_container::create(plugin_gui *_gui, const char *element, xml_attribute_map &attributes)
{
    GtkWidget *vbox = gtk_vbox_new(false, get_int("spacing", 2));
    container = GTK_CONTAINER(vbox);
    return vbox;
}

/******************************** GtkNotebook container ********************************/

GtkWidget *notebook_container::create(plugin_gui *_gui, const char *element, xml_attribute_map &attributes)
{
    GtkWidget *nb = gtk_notebook_new();
    container = GTK_CONTAINER(nb);
    return nb;
}

void notebook_container::add(GtkWidget *w, control_base *base)
{
    gtk_notebook_append_page(GTK_NOTEBOOK(container), w, gtk_label_new_with_mnemonic(base->attribs["page"].c_str()));
}

/******************************** GtkNotebook container ********************************/

GtkWidget *scrolled_container::create(plugin_gui *_gui, const char *element, xml_attribute_map &attributes)
{
    GtkAdjustment *horiz = NULL, *vert = NULL;
    int width = get_int("width", 0), height = get_int("height", 0);
    if (width)
        horiz = GTK_ADJUSTMENT(gtk_adjustment_new(get_int("x", 0), 0, width, get_int("step-x", 1), get_int("page-x", width / 10), 100));
    if (height)
        vert = GTK_ADJUSTMENT(gtk_adjustment_new(get_int("y", 0), 0, width, get_int("step-y", 1), get_int("page-y", height / 10), 10));
    GtkWidget *sw = gtk_scrolled_window_new(horiz, vert);
    gtk_widget_set_size_request(sw, get_int("req-x", -1), get_int("req-y", -1));
    container = GTK_CONTAINER(sw);
    return sw;
}

void scrolled_container::add(GtkWidget *w, control_base *base)
{
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(container), w);
}

/******************************** GUI proper ********************************/

param_control *plugin_gui::create_control_from_xml(const char *element, const char *attributes[])
{
    if (!strcmp(element, "knob"))
        return new knob_param_control;
    if (!strcmp(element, "hscale"))
        return new hscale_param_control;
    if (!strcmp(element, "vscale"))
        return new vscale_param_control;
    if (!strcmp(element, "combo"))
        return new combo_box_param_control;
    if (!strcmp(element, "toggle"))
        return new toggle_param_control;
    if (!strcmp(element, "spin"))
        return new spin_param_control;
    if (!strcmp(element, "button"))
        return new button_param_control;
    if (!strcmp(element, "label"))
        return new label_param_control;
    if (!strcmp(element, "value"))
        return new value_param_control;
    if (!strcmp(element, "vumeter"))
        return new vumeter_param_control;
    if (!strcmp(element, "line-graph"))
        return new line_graph_param_control;
    if (!strcmp(element, "keyboard"))
        return new keyboard_param_control;
    if (!strcmp(element, "curve"))
        return new curve_param_control;
    if (!strcmp(element, "led"))
        return new led_param_control;
    if (!strcmp(element, "entry"))
        return new entry_param_control;
    if (!strcmp(element, "filechooser"))
        return new filechooser_param_control;
    if (!strcmp(element, "listview"))
        return new listview_param_control;
    return NULL;
}

control_container *plugin_gui::create_container_from_xml(const char *element, const char *attributes[])
{
    if (!strcmp(element, "table"))
        return new table_container;
    if (!strcmp(element, "vbox"))
        return new vbox_container;
    if (!strcmp(element, "hbox"))
        return new hbox_container;
    if (!strcmp(element, "align"))
        return new alignment_container;
    if (!strcmp(element, "frame"))
        return new frame_container;
    if (!strcmp(element, "notebook"))
        return new notebook_container;
    if (!strcmp(element, "scrolled"))
        return new scrolled_container;
    return NULL;
}

void plugin_gui::xml_element_start(void *data, const char *element, const char *attributes[])
{
    plugin_gui *gui = (plugin_gui *)data;
    gui->xml_element_start(element, attributes);
}

void plugin_gui::xml_element_start(const char *element, const char *attributes[])
{
    if (ignore_stack) {
        ignore_stack++;
        return;
    }
    control_base::xml_attribute_map xam;
    while(*attributes)
    {
        xam[attributes[0]] = attributes[1];
        attributes += 2;
    }
    
    if (!strcmp(element, "if"))
    {
        if (!xam.count("cond") || xam["cond"].empty())
        {
            g_error("Incorrect <if cond=\"[!]symbol\"> element");
        }
        string cond = xam["cond"];
        bool state = true;
        if (cond.substr(0, 1) == "!") {
            state = false;
            cond.erase(0, 1);
        }
        if (window->main->check_condition(cond.c_str()) == state)
            return;
        ignore_stack = 1;
        return;
    }
    control_container *cc = create_container_from_xml(element, attributes);
    if (cc != NULL)
    {
        cc->attribs = xam;
        cc->create(this, element, xam);
        gtk_container_set_border_width(cc->container, cc->get_int("border"));

        container_stack.push_back(cc);
        current_control = NULL;
        return;
    }
    if (!container_stack.empty())
    {
        current_control = create_control_from_xml(element, attributes);
        if (current_control)
        {
            current_control->attribs = xam;
            int param_no = -1;
            if (xam.count("param"))
            {
                map<string, int>::iterator it = param_name_map.find(xam["param"]);
                if (it == param_name_map.end())
                    g_error("Unknown parameter %s", xam["param"].c_str());
                else
                    param_no = it->second;
            }
            if (param_no != -1)
                current_control->param_variable = plugin->get_param_props(param_no)->short_name;
            current_control->create(this, param_no);
            current_control->init_xml(element);
            current_control->set();
            current_control->hook_params();
            return;
        }
    }
    g_error("Unxpected element %s in GUI definition\n", element);
}

void plugin_gui::xml_element_end(void *data, const char *element)
{
    plugin_gui *gui = (plugin_gui *)data;
    if (gui->ignore_stack) {
        gui->ignore_stack--;
        return;
    }
    if (!strcmp(element, "if"))
    {
        return;
    }
    if (gui->current_control)
    {
        (*gui->container_stack.rbegin())->add(gui->current_control->widget, gui->current_control);
        gui->current_control = NULL;
        return;
    }
    unsigned int ss = gui->container_stack.size();
    if (ss > 1) {
        gui->container_stack[ss - 2]->add(GTK_WIDGET(gui->container_stack[ss - 1]->container), gui->container_stack[ss - 1]);
    }
    else
        gui->top_container = gui->container_stack[0];
    gui->container_stack.pop_back();
}


GtkWidget *plugin_gui::create_from_xml(plugin_ctl_iface *_plugin, const char *xml)
{
    current_control = NULL;
    top_container = NULL;
    parser = XML_ParserCreate("UTF-8");
    plugin = _plugin;
    container_stack.clear();
    ignore_stack = 0;
    
    param_name_map.clear();
    int size = plugin->get_param_count();
    for (int i = 0; i < size; i++)
        param_name_map[plugin->get_param_props(i)->short_name] = i;
    
    XML_SetUserData(parser, this);
    XML_SetElementHandler(parser, xml_element_start, xml_element_end);
    XML_Status status = XML_Parse(parser, xml, strlen(xml), 1);
    if (status == XML_STATUS_ERROR)
    {
        g_error("Parse error: %s in XML", XML_ErrorString(XML_GetErrorCode(parser)));
    }
    
    XML_ParserFree(parser);
    last_status_serial_no = plugin->send_status_updates(this, 0);
    return GTK_WIDGET(top_container->container);
}

void plugin_gui::send_configure(const char *key, const char *value)
{
    // XXXKF this should really be replaced by a separate list of SCI-capable param controls
    for (unsigned int i = 0; i < params.size(); i++)
    {
        assert(params[i] != NULL);
        send_configure_iface *sci = dynamic_cast<send_configure_iface *>(params[i]);
        if (sci)
            sci->send_configure(key, value);
    }
}

void plugin_gui::send_status(const char *key, const char *value)
{
    // XXXKF this should really be replaced by a separate list of SUI-capable param controls
    for (unsigned int i = 0; i < params.size(); i++)
    {
        assert(params[i] != NULL);
        send_updates_iface *sui = dynamic_cast<send_updates_iface *>(params[i]);
        if (sui)
            sui->send_status(key, value);
    }
}

void plugin_gui::on_idle()
{
    for (unsigned int i = 0; i < params.size(); i++)
    {
        if (params[i]->param_no != -1)
        {
            parameter_properties &props = *plugin->get_param_props(params[i]->param_no);
            bool is_output = (props.flags & PF_PROP_OUTPUT) != 0;
            if (is_output) {
                params[i]->set();
            }
        }
        params[i]->on_idle();
    }    
    last_status_serial_no = plugin->send_status_updates(this, last_status_serial_no);
    // XXXKF iterate over par2ctl, too...
}

void plugin_gui::refresh()
{
    for (unsigned int i = 0; i < params.size(); i++)
        params[i]->set();
    plugin->send_configures(this);
    last_status_serial_no = plugin->send_status_updates(this, last_status_serial_no);
}

void plugin_gui::refresh(int param_no, param_control *originator)
{
    std::multimap<int, param_control *>::iterator it = par2ctl.find(param_no);
    while(it != par2ctl.end() && it->first == param_no)
    {
        if (it->second != originator)
            it->second->set();
        it++;
    }
}

void plugin_gui::set_param_value(int param_no, float value, param_control *originator)
{
    plugin->set_param_value(param_no, value);
    refresh(param_no);
}

plugin_gui::~plugin_gui()
{
    for (std::vector<param_control *>::iterator i = params.begin(); i != params.end(); i++)
    {
        delete *i;
    }
}


/******************************* Actions **************************************************/
 
static void store_preset_action(GtkAction *action, plugin_gui_window *gui_win)
{
    store_preset(GTK_WINDOW(gui_win->toplevel), gui_win->gui);
}

static const GtkActionEntry actions[] = {
    { "PresetMenuAction", "", "_Preset", NULL, "Preset operations", NULL },
    { "BuiltinPresetMenuAction", "", "_Built-in", NULL, "Built-in (factory) presets", NULL },
    { "UserPresetMenuAction", "", "_User", NULL, "User (your) presets", NULL },
    { "CommandMenuAction", "", "_Command", NULL, "Plugin-related commands", NULL },
    { "store-preset", "", "Store preset", NULL, "Store a current setting as preset", (GCallback)store_preset_action },
};

/***************************** GUI window ********************************************/

static const char *ui_xml = 
"<ui>\n"
"  <menubar>\n"
"    <menu action=\"PresetMenuAction\">\n"
"      <menuitem action=\"store-preset\"/>\n"
"      <separator/>\n"
"      <placeholder name=\"builtin_presets\"/>\n"
"      <separator/>\n"
"      <placeholder name=\"user_presets\"/>\n"
"    </menu>\n"
"    <placeholder name=\"commands\"/>\n"
"  </menubar>\n"
"</ui>\n"
;

static const char *general_preset_pre_xml = 
"<ui>\n"
"  <menubar>\n"
"    <menu action=\"PresetMenuAction\">\n";

static const char *builtin_preset_pre_xml = 
"        <placeholder name=\"builtin_presets\">\n";

static const char *user_preset_pre_xml = 
"        <placeholder name=\"user_presets\">\n";

static const char *preset_post_xml = 
"        </placeholder>\n"
"    </menu>\n"
"  </menubar>\n"
"</ui>\n"
;

static const char *command_pre_xml = 
"<ui>\n"
"  <menubar>\n"
"    <placeholder name=\"commands\">\n"
"      <menu action=\"CommandMenuAction\">\n";

static const char *command_post_xml = 
"      </menu>\n"
"    </placeholder>\n"
"  </menubar>\n"
"</ui>\n"
;

plugin_gui_window::plugin_gui_window(main_window_iface *_main)
{
    toplevel = NULL;
    ui_mgr = NULL;
    std_actions = NULL;
    builtin_preset_actions = NULL;
    user_preset_actions = NULL;
    command_actions = NULL;
    main = _main;
    assert(main);
}

string plugin_gui_window::make_gui_preset_list(GtkActionGroup *grp, bool builtin, char &ch)
{
    string preset_xml = string(general_preset_pre_xml) + (builtin ? builtin_preset_pre_xml : user_preset_pre_xml);
    preset_vector &pvec = (builtin ? get_builtin_presets() : get_user_presets()).presets;
    GtkActionGroup *preset_actions = builtin ? builtin_preset_actions : user_preset_actions;
    for (unsigned int i = 0; i < pvec.size(); i++)
    {
        if (pvec[i].plugin != gui->effect_name)
            continue;
        stringstream ss;
        ss << (builtin ? "builtin_preset" : "user_preset") << i;
        preset_xml += "          <menuitem name=\"" + pvec[i].name+"\" action=\""+ss.str()+"\"/>\n";
        if (ch != ' ' && ++ch == ':')
            ch = 'A';
        if (ch > 'Z')
            ch = ' ';

        string sv = ss.str();
        string prefix = ch == ' ' ? string() : string("_")+ch+" ";
        string name = prefix + pvec[i].name;
        GtkActionEntry ae = { sv.c_str(), NULL, name.c_str(), NULL, NULL, (GCallback)activate_preset };
        gtk_action_group_add_actions_full(preset_actions, &ae, 1, (gpointer)new activate_preset_params(gui, i, builtin), action_destroy_notify);
    }
    preset_xml += preset_post_xml;
    return preset_xml;
}

string plugin_gui_window::make_gui_command_list(GtkActionGroup *grp)
{
    string command_xml = command_pre_xml;
    plugin_command_info *ci = gui->plugin->get_commands();
    if (!ci)
        return "";
    for(int i = 0; ci->name; i++, ci++)
    {
        stringstream ss;
        ss << "          <menuitem name=\"" << ci->name << "\" action=\"" << ci->label << "\"/>\n";
        
        GtkActionEntry ae = { ci->label, NULL, ci->name, NULL, ci->description, (GCallback)activate_command };
        gtk_action_group_add_actions_full(command_actions, &ae, 1, (gpointer)new activate_command_params(gui, i), action_destroy_notify);
        command_xml += ss.str();
    }
    command_xml += command_post_xml;
    return command_xml;
}

void plugin_gui_window::fill_gui_presets(bool builtin, char &ch)
{
    GtkActionGroup *&preset_actions = builtin ? builtin_preset_actions : user_preset_actions;
    if(preset_actions) {
        gtk_ui_manager_remove_action_group(ui_mgr, preset_actions);
        preset_actions = NULL;
    }
    
    if (builtin)
        builtin_preset_actions = gtk_action_group_new("builtin_presets");
    else
        user_preset_actions = gtk_action_group_new("user_presets");
    string preset_xml = make_gui_preset_list(preset_actions, builtin, ch);
    gtk_ui_manager_insert_action_group(ui_mgr, preset_actions, 0);    
    GError *error = NULL;
    gtk_ui_manager_add_ui_from_string(ui_mgr, preset_xml.c_str(), -1, &error);
}

gboolean plugin_gui_window::on_idle(void *data)
{
    plugin_gui_window *self = (plugin_gui_window *)data;
    self->gui->on_idle();
    return TRUE;
}

void plugin_gui_window::create(plugin_ctl_iface *_jh, const char *title, const char *effect)
{
    toplevel = GTK_WINDOW(gtk_window_new (GTK_WINDOW_TOPLEVEL));
    gtk_window_set_default_icon_name("calf");
    gtk_window_set_type_hint(toplevel, GDK_WINDOW_TYPE_HINT_DIALOG);
    GtkVBox *vbox = GTK_VBOX(gtk_vbox_new(false, 5));
    
    GtkRequisition req, req2;
    gtk_window_set_title(GTK_WINDOW (toplevel), title);
    gtk_container_add(GTK_CONTAINER(toplevel), GTK_WIDGET(vbox));

    gui = new plugin_gui(this);
    gui->effect_name = effect;

    ui_mgr = gtk_ui_manager_new();
    std_actions = gtk_action_group_new("default");
    gtk_action_group_add_actions(std_actions, actions, sizeof(actions)/sizeof(actions[0]), this);
    GError *error = NULL;
    gtk_ui_manager_insert_action_group(ui_mgr, std_actions, 0);
    gtk_ui_manager_add_ui_from_string(ui_mgr, ui_xml, -1, &error);    
    
    command_actions = gtk_action_group_new("commands");

    char ch = '0';
    fill_gui_presets(true, ch);
    fill_gui_presets(false, ch);
    
    gtk_box_pack_start(GTK_BOX(vbox), gtk_ui_manager_get_widget(ui_mgr, "/ui/menubar"), false, false, 0);

    // determine size without content
    gtk_widget_show_all(GTK_WIDGET(vbox));
    gtk_widget_size_request(GTK_WIDGET(vbox), &req2);
    // printf("size request %dx%d\n", req2.width, req2.height);
    
    GtkWidget *container;
    const char *xml = _jh->get_gui_xml();
    assert(xml);
    container = gui->create_from_xml(_jh, xml);

    string command_xml = make_gui_command_list(command_actions);
    gtk_ui_manager_insert_action_group(ui_mgr, command_actions, 0);
    gtk_ui_manager_add_ui_from_string(ui_mgr, command_xml.c_str(), -1, &error);    
    
    GtkWidget *sw = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw), GTK_SHADOW_NONE);
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(sw), container);
    
    gtk_box_pack_start(GTK_BOX(vbox), sw, true, true, 0);
    
    gtk_widget_show_all(GTK_WIDGET(sw));
    gtk_widget_size_request(container, &req);
    int wx = max(req.width + 10, req2.width);
    int wy = req.height + req2.height + 10;
    // printf("size request %dx%d\n", req.width, req.height);
    // printf("resize to %dx%d\n", max(req.width + 10, req2.width), req.height + req2.height + 10);
    gtk_window_set_default_size(GTK_WINDOW(toplevel), wx, wy);
    gtk_window_resize(GTK_WINDOW(toplevel), wx, wy);
    //gtk_widget_set_size_request(GTK_WIDGET(toplevel), max(req.width + 10, req2.width), req.height + req2.height + 10);
    // printf("size set %dx%d\n", wx, wy);
    // gtk_scrolled_window_set_vadjustment(GTK_SCROLLED_WINDOW(sw), GTK_ADJUSTMENT(gtk_adjustment_new(0, 0, req.height, 20, 100, 100)));
    gtk_signal_connect (GTK_OBJECT (toplevel), "destroy", G_CALLBACK (window_destroyed), (plugin_gui_window *)this);
    main->set_window(gui->plugin, this);

    source_id = g_timeout_add_full(G_PRIORITY_LOW, 1000/30, on_idle, this, NULL); // 30 fps should be enough for everybody
    gtk_ui_manager_ensure_update(ui_mgr);
    gui->plugin->send_configures(gui);
}

void plugin_gui_window::close()
{
    if (source_id)
        g_source_remove(source_id);
    source_id = 0;
    gtk_widget_destroy(GTK_WIDGET(toplevel));
}

plugin_gui_window::~plugin_gui_window()
{
    if (source_id)
        g_source_remove(source_id);
    main->set_window(gui->plugin, NULL);
    delete gui;
}

void calf_plugins::activate_command(GtkAction *action, activate_command_params *params)
{
    plugin_gui *gui = params->gui;
    gui->plugin->execute(params->function_idx);
    gui->refresh();
}


