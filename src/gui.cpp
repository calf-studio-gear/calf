/* Calf DSP Library
 * GUI functions for a plugin.
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
 
#include <calf/giface.h>
#include <calf/gui.h>

using namespace synth;


/******************************** controls ********************************/

GtkWidget *param_control::create_label(int param_idx)
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

// combo box

GtkWidget *combo_box_param_control::create(plugin_gui *_gui, int _param_no)
{
    gui = _gui;
    param_no = _param_no;
    
    parameter_properties &props = get_props();
    widget  = gtk_combo_box_new_text ();
    for (int j = (int)props.min; j <= (int)props.max; j++)
        gtk_combo_box_append_text (GTK_COMBO_BOX (widget), props.choices[j - (int)props.min]);
    gtk_signal_connect (GTK_OBJECT (widget), "changed", G_CALLBACK (combo_value_changed), (gpointer)this);
    
    return widget;
}

void combo_box_param_control::set()
{
    parameter_properties &props = get_props();
    gtk_combo_box_set_active (GTK_COMBO_BOX (widget), (int)gui->plugin->get_param_value(param_no) - (int)props.min);
}

void combo_box_param_control::get()
{
    parameter_properties &props = get_props();
    gui->plugin->set_param_value(param_no, gtk_combo_box_get_active (GTK_COMBO_BOX(widget)) + props.min);
}

void combo_box_param_control::combo_value_changed(GtkComboBox *widget, gpointer value)
{
    param_control *jhp = (param_control *)value;
    jhp->get();
}

// horizontal fader

GtkWidget *hscale_param_control::create(plugin_gui *_gui, int _param_no)
{
    gui = _gui;
    param_no = _param_no;

    gtk_misc_set_alignment (GTK_MISC (label), 1.0, 1.0);
    
    widget = gtk_hscale_new_with_range (0, 1, 0.01);
    gtk_signal_connect (GTK_OBJECT (widget), "value-changed", G_CALLBACK (hscale_value_changed), (gpointer)this);
    gtk_signal_connect (GTK_OBJECT (widget), "format-value", G_CALLBACK (hscale_format_value), (gpointer)this);
    gtk_widget_set_size_request (widget, 200, -1);

    return widget;
}

void hscale_param_control::set()
{
    parameter_properties &props = get_props();
    gtk_range_set_value (GTK_RANGE (widget), props.to_01 (gui->plugin->get_param_value(param_no)));
    hscale_value_changed (GTK_HSCALE (widget), (gpointer)this);
}

void hscale_param_control::get()
{
    parameter_properties &props = get_props();
    float cvalue = props.from_01 (gtk_range_get_value (GTK_RANGE (widget)));
    gui->plugin->set_param_value(param_no, cvalue);
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
    plugin_ctl_iface &pif = *gui->plugin;
    pif.set_param_value(param_no, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(widget)) + props.min);
}

void toggle_param_control::set()
{
    const parameter_properties &props = get_props();
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), (int)gui->plugin->get_param_value(param_no) - (int)props.min);
}

// knob

#if USE_PHAT
GtkWidget *knob_param_control::create(plugin_gui *_gui, int _param_no)
{
    gui = _gui;
    param_no = _param_no;
    const parameter_properties &props = get_props();
    
    widget = phat_knob_new_with_range (props.to_01 (gui->plugin->get_param_value(param_no)), 0, 1, 0.01);
    gtk_signal_connect(GTK_OBJECT(widget), "value-changed", G_CALLBACK(knob_value_changed), (gpointer)this);
    return widget;
}

void knob_param_control::get()
{
    const parameter_properties &props = get_props();
    float value = props.from_01(phat_knob_get_value(PHAT_KNOB(widget)));
    gui->plugin->set_param_value(param_no, value);
}

void knob_param_control::set()
{
    const parameter_properties &props = get_props();
    phat_knob_set_value(PHAT_KNOB(widget), props.to_01 (gui->plugin->get_param_value(param_no)));
    knob_value_changed(PHAT_KNOB(widget), (gpointer)this);
}

void knob_param_control::knob_value_changed(PhatKnob *widget, gpointer value)
{
    param_control *jhp = (param_control *)value;
    plugin_ctl_iface &pif = *jhp->gui->plugin;
    const parameter_properties &props = jhp->get_props();
    float cvalue = props.from_01 (phat_knob_get_value (widget));
    pif.set_param_value(jhp->param_no, cvalue);
    jhp->update_label();
}
#endif

/******************************** GUI proper ********************************/

plugin_gui::plugin_gui(GtkWidget *_toplevel)
: toplevel(_toplevel)
{
    
}


GtkWidget *plugin_gui::create(plugin_ctl_iface *_plugin, const char *title)
{
    plugin = _plugin;
    param_count = plugin->get_param_count();
    params.resize(param_count);
    for (int i = 0; i < param_count; i++) {
        params[i] = NULL;
    }
    
    table = gtk_table_new (param_count, 3, FALSE);
    
    for (int i = 0; i < param_count; i++) {
        int trow = i;
        GtkWidget *label = gtk_label_new (plugin->get_param_names()[i]);
        gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
        gtk_table_attach (GTK_TABLE (table), label, 0, 1, trow, trow + 1, GTK_FILL, GTK_FILL, 2, 2);
        
        parameter_properties &props = *plugin->get_param_props(i);
        
        GtkWidget *widget = NULL;
        
        if ((props.flags & PF_TYPEMASK) == PF_ENUM && 
            (props.flags & PF_CTLMASK) == PF_CTL_COMBO)
        {
            params[i] = new combo_box_param_control();
            widget = params[i]->create(this, i);
            gtk_table_attach (GTK_TABLE (table), widget, 1, 3, trow, trow + 1, GTK_EXPAND, GTK_SHRINK, 0, 0);
        }
        else if ((props.flags & PF_TYPEMASK) == PF_BOOL && 
                 (props.flags & PF_CTLMASK) == PF_CTL_TOGGLE)
        {
            params[i] = new toggle_param_control();
            widget = params[i]->create(this, i);
            gtk_table_attach (GTK_TABLE (table), widget, 1, 3, trow, trow + 1, GTK_EXPAND, GTK_SHRINK, 0, 0);
        }
#if USE_PHAT
        else if ((props.flags & PF_CTLMASK) != PF_CTL_FADER)
        {
            params[i] = new knob_param_control();
            widget = params[i]->create(this, i);
            gtk_table_attach (GTK_TABLE (table), widget, 1, 2, trow, trow + 1, GTK_SHRINK, GTK_SHRINK, 0, 0);
            gtk_table_attach (GTK_TABLE (table), params[i]->create_label(i), 2, 3, trow, trow + 1, (GtkAttachOptions)(GTK_SHRINK | GTK_FILL), GTK_SHRINK, 0, 0);
        }
#endif
        else
        {
            params[i] = new hscale_param_control();
            widget = params[i]->create(this, i);
            gtk_table_attach (GTK_TABLE (table), widget, 1, 3, trow, trow + 1, (GtkAttachOptions)(GTK_EXPAND | GTK_FILL), GTK_SHRINK, 0, 0);
        }
        params[i]->set();
    }
    return table;
}

void plugin_gui::refresh()
{
    for (unsigned int i = 0; i < params.size(); i++)
    {
        if (params[i] != NULL)
            params[i]->set();
    }
}

