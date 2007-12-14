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

plugin_gui::plugin_gui(GtkWidget *_toplevel)
: toplevel(_toplevel)
{
    
}

void plugin_gui::hscale_value_changed(GtkHScale *widget, gpointer value)
{
    param_control *jhp = (param_control *)value;
    plugin_ctl_iface &pif = *jhp->gui->plugin;
    int nparam = jhp->param_no;
    const parameter_properties &props = *pif.get_param_props(nparam);
    float cvalue = props.from_01 (gtk_range_get_value (GTK_RANGE (widget)));
    pif.set_param_value(nparam, cvalue);
    // gtk_label_set_text (GTK_LABEL (jhp->label), props.to_string(cvalue).c_str());
}

gchar *plugin_gui::hscale_format_value(GtkScale *widget, double arg1, gpointer value)
{
    param_control *jhp = (param_control *)value;
    plugin_ctl_iface &pif = *jhp->gui->plugin;
    const parameter_properties &props = *pif.get_param_props (jhp->param_no);
    float cvalue = props.from_01 (arg1);
    
    // for testing
    // return g_strdup_printf ("%s = %g", props.to_string (cvalue).c_str(), arg1);
    return g_strdup (props.to_string (cvalue).c_str());
}

void plugin_gui::combo_value_changed(GtkComboBox *widget, gpointer value)
{
    param_control *jhp = (param_control *)value;
    plugin_ctl_iface &pif = *jhp->gui->plugin;
    int nparam = jhp->param_no;
    pif.set_param_value(nparam, gtk_combo_box_get_active (widget) + pif.get_param_props (jhp->param_no)->min);
}

void plugin_gui::toggle_value_changed(GtkCheckButton *widget, gpointer value)
{
    param_control *jhp = (param_control *)value;
    plugin_ctl_iface &pif = *jhp->gui->plugin;
    int nparam = jhp->param_no;
    pif.set_param_value(nparam, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(widget)) + pif.get_param_props (jhp->param_no)->min);
}

#if USE_PHAT
void plugin_gui::knob_value_changed(PhatKnob *widget, gpointer value)
{
    param_control *jhp = (param_control *)value;
    plugin_ctl_iface &pif = *jhp->gui->plugin;
    const parameter_properties &props = *pif.get_param_props (jhp->param_no);
    int nparam = jhp->param_no;
    float cvalue = props.from_01 (phat_knob_get_value (widget));
    pif.set_param_value(nparam, cvalue);
    gtk_label_set_text (GTK_LABEL (jhp->label), props.to_string(cvalue).c_str());
}
#endif

GtkWidget *plugin_gui::create_label(int param_idx)
{
    GtkWidget *widget  = gtk_label_new ("");
    gtk_label_set_width_chars (GTK_LABEL (widget), 12);
    gtk_misc_set_alignment (GTK_MISC (widget), 0.0, 0.5);
    params[param_idx].label = widget;
    return widget;
}

void plugin_gui::create(plugin_ctl_iface *_plugin, const char *title)
{
    plugin = _plugin;
    param_count = plugin->get_param_count();
    params = new param_control[param_count];
    
    for (int i = 0; i < param_count; i++) {
        params[i].gui = this;
        params[i].param_no = i;
    }

    table = gtk_table_new (param_count + 1, 3, FALSE);
    
    GtkWidget *title_label = gtk_label_new ("");
    gtk_label_set_markup (GTK_LABEL (title_label), (std::string("<b>")+title+"</b>").c_str());
    gtk_table_attach (GTK_TABLE (table), title_label, 0, 3, 0, 1, GTK_EXPAND, GTK_SHRINK, 2, 2);

    for (int i = 0; i < param_count; i++) {
        GtkWidget *label = gtk_label_new (plugin->get_param_names()[i]);
        gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
        gtk_table_attach (GTK_TABLE (table), label, 0, 1, i + 1, i + 2, GTK_FILL, GTK_FILL, 2, 2);
        
        parameter_properties &props = *plugin->get_param_props(i);
        
        GtkWidget *widget;
        
        if ((props.flags & PF_TYPEMASK) == PF_ENUM && 
            (props.flags & PF_CTLMASK) == PF_CTL_COMBO)
        {
            widget  = gtk_combo_box_new_text ();
            for (int j = (int)props.min; j <= (int)props.max; j++)
                gtk_combo_box_append_text (GTK_COMBO_BOX (widget), props.choices[j - (int)props.min]);
            gtk_combo_box_set_active (GTK_COMBO_BOX (widget), (int)plugin->get_param_value(i) - (int)props.min);
            gtk_signal_connect (GTK_OBJECT (widget), "changed", G_CALLBACK (combo_value_changed), (gpointer)&params[i]);
            gtk_table_attach (GTK_TABLE (table), widget, 1, 3, i + 1, i + 2, GTK_EXPAND, GTK_SHRINK, 0, 0);
        }
        else if ((props.flags & PF_TYPEMASK) == PF_BOOL && 
                 (props.flags & PF_CTLMASK) == PF_CTL_TOGGLE)
        {
            widget  = gtk_check_button_new ();
            gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), (int)plugin->get_param_value(i) - (int)props.min);
            gtk_signal_connect (GTK_OBJECT (widget), "toggled", G_CALLBACK (toggle_value_changed), (gpointer)&params[i]);
            gtk_table_attach (GTK_TABLE (table), widget, 1, 3, i + 1, i + 2, GTK_EXPAND, GTK_SHRINK, 0, 0);
        }
#if USE_PHAT
        else if ((props.flags & PF_CTLMASK) != PF_CTL_FADER)
        {
            GtkWidget *knob = phat_knob_new_with_range (props.to_01 (plugin->get_param_value(i)), 0, 1, 0.01);
            gtk_signal_connect (GTK_OBJECT (knob), "value-changed", G_CALLBACK (knob_value_changed), (gpointer)&params[i]);
            gtk_table_attach (GTK_TABLE (table), knob, 1, 2, i + 1, i + 2, GTK_SHRINK, GTK_SHRINK, 0, 0);
            gtk_table_attach (GTK_TABLE (table), create_label(i), 2, 3, i + 1, i + 2, (GtkAttachOptions)(GTK_SHRINK | GTK_FILL), GTK_SHRINK, 0, 0);
            knob_value_changed (PHAT_KNOB (knob), (gpointer)&params[i]);
        }
#endif
        else
        {
            gtk_misc_set_alignment (GTK_MISC (label), 1.0, 1.0);
            
            GtkWidget *knob = gtk_hscale_new_with_range (0, 1, 0.01);
            gtk_signal_connect (GTK_OBJECT (knob), "value-changed", G_CALLBACK (hscale_value_changed), (gpointer)&params[i]);
            gtk_signal_connect (GTK_OBJECT (knob), "format-value", G_CALLBACK (hscale_format_value), (gpointer)&params[i]);
            gtk_widget_set_size_request (knob, 200, -1);
            gtk_table_attach (GTK_TABLE (table), knob, 1, 3, i + 1, i + 2, (GtkAttachOptions)(GTK_EXPAND | GTK_FILL), GTK_SHRINK, 0, 0);
            
            gtk_range_set_value (GTK_RANGE (knob), props.to_01 (plugin->get_param_value(i)));
            hscale_value_changed (GTK_HSCALE (knob), (gpointer)&params[i]);
        }
    }
    gtk_container_add (GTK_CONTAINER (toplevel), table);
}
