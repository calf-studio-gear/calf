/* Calf DSP Library
 * GUI widget object implementations.
 * Copyright (C) 2007-2009 Krzysztof Foltman
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
#include <calf/gui_controls.h>
#include <calf/preset.h>
#include <calf/preset_gui.h>
#include <calf/main_win.h>
#include <gdk/gdk.h>

using namespace calf_plugins;
using namespace std;

/******************************** controls ********************************/

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

GtkWidget *check_param_control::create(plugin_gui *_gui, int _param_no)
{
    gui = _gui;
    param_no = _param_no;
    
    widget  = gtk_check_button_new ();
    gtk_signal_connect (GTK_OBJECT (widget), "toggled", G_CALLBACK (check_value_changed), (gpointer)this);
    return widget;
}

void check_param_control::check_value_changed(GtkCheckButton *widget, gpointer value)
{
    param_control *jhp = (param_control *)value;
    jhp->get();
}

void check_param_control::get()
{
    const parameter_properties &props = get_props();
    gui->set_param_value(param_no, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(widget)) + props.min, this);
}

void check_param_control::set()
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
    CALF_KNOB(widget)->knob_size = get_int("size", 2);
    if(CALF_KNOB(widget)->knob_size > 4) {
        CALF_KNOB(widget)->knob_size = 4;
    } else if (CALF_KNOB(widget)->knob_size < 1) {
        CALF_KNOB(widget)->knob_size = 1;
    }
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

// Toggle Button

GtkWidget *toggle_param_control::create(plugin_gui *_gui, int _param_no)
{
    gui = _gui;
    param_no = _param_no;
    widget  = calf_toggle_new ();
    
    CALF_TOGGLE(widget)->size = get_int("size", 2);
    if(CALF_TOGGLE(widget)->size > 2) {
        CALF_TOGGLE(widget)->size = 2;
    } else if (CALF_TOGGLE(widget)->size < 1) {
        CALF_TOGGLE(widget)->size = 1;
    }
    
    gtk_signal_connect (GTK_OBJECT (widget), "value-changed", G_CALLBACK (toggle_value_changed), (gpointer)this);
    return widget;
}

void toggle_param_control::get()
{
    const parameter_properties &props = get_props();
    float value = props.from_01(gtk_range_get_value(GTK_RANGE(widget)));
    gui->set_param_value(param_no, value, this);
    if (label)
        update_label();
}

void toggle_param_control::set()
{
    _GUARD_CHANGE_
    const parameter_properties &props = get_props();
    gtk_range_set_value(GTK_RANGE(widget), props.to_01 (gui->plugin->get_param_value(param_no)));
    if (label)
        update_label();
}

void toggle_param_control::toggle_value_changed(GtkWidget *widget, gpointer value)
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
    update_store();
    widget = gtk_tree_view_new_with_model(GTK_TREE_MODEL(lstore));
    delete []p;
    tree = GTK_TREE_VIEW (widget);
    assert(teif);
    g_object_set (G_OBJECT (tree), "enable-search", FALSE, "rules-hint", TRUE, "enable-grid-lines", TRUE, NULL);
    
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

void listview_param_control::update_store()
{
    gtk_list_store_clear(lstore);
    uint32_t rows = teif->get_table_rows(param_no);
    for (uint32_t i = 0; i < rows; i++)
    {
        GtkTreeIter iter;
        gtk_list_store_insert(lstore, &iter, i);
        for (int j = 0; j < cols; j++)
        {
            gtk_list_store_set(lstore, &iter, j, teif->get_cell(param_no, i, j).c_str(), -1);
        }
        positions.push_back(iter);
    }
}

void listview_param_control::send_configure(const char *key, const char *value)
{
    if (attribs["key"] == key)
    {
        update_store();
    }
}

void listview_param_control::on_edited(GtkCellRenderer *renderer, gchar *path, gchar *new_text, listview_param_control *pThis)
{
    const table_column_info *tci = pThis->teif->get_table_columns(pThis->param_no);
    int column = ((table_column_info *)g_object_get_data(G_OBJECT(renderer), "column")) - tci;
    string error;
    pThis->teif->set_cell(pThis->param_no, atoi(path), column, new_text, error);
    if (error.empty()) {
        pThis->update_store();
        gtk_widget_grab_focus(pThis->widget);
        if (atoi(path) < (int)pThis->teif->get_table_rows(pThis->param_no))
        {
            GtkTreePath *gpath = gtk_tree_path_new_from_string (path);
            gtk_tree_view_set_cursor_on_cell (GTK_TREE_VIEW (pThis->widget), gpath, NULL, NULL, FALSE);
            gtk_tree_path_free (gpath);
        }
    }
    else
    {
        GtkWidget *dialog = gtk_message_dialog_new(pThis->gui->window->toplevel, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, 
            "%s", error.c_str());
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        gtk_widget_grab_focus(pThis->widget);
    }
}

void listview_param_control::on_editing_canceled(GtkCellRenderer *renderer, listview_param_control *pThis)
{
    gtk_widget_grab_focus(pThis->widget);
}

