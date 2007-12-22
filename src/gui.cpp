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
#include <calf/preset.h>
#include <calf/preset_gui.h>

using namespace synth;
using namespace std;

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

plugin_gui::plugin_gui(plugin_gui_window *_window)
: window(_window)
{
    
}

static void action_destroy_notify(gpointer data)
{
    delete (activate_preset_params *)data;
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
        parameter_properties &props = *plugin->get_param_props(i);
        GtkWidget *label = gtk_label_new (props.name);
        gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
        gtk_table_attach (GTK_TABLE (table), label, 0, 1, trow, trow + 1, GTK_FILL, GTK_FILL, 2, 2);
        
        
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
            gtk_misc_set_alignment (GTK_MISC (label), 1.0, 1.0);
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

/******************************* Actions **************************************************/
 
static void store_preset_action(GtkAction *action, plugin_gui_window *gui_win)
{
    store_preset(GTK_WINDOW(gui_win->toplevel), gui_win->gui);
}

static void exit_gui(GtkAction *action, plugin_gui_window *gui_win)
{
    gtk_widget_destroy(GTK_WIDGET(gui_win->toplevel));
}

static const GtkActionEntry actions[] = {
    { "HostMenuAction", "", "_Host", NULL, "Application-wide actions", NULL },
    { "exit", "", "E_xit", NULL, "Exit the application", (GCallback)exit_gui },
    { "PresetMenuAction", "", "_Preset", NULL, "Preset operations", NULL },
    { "store-preset", "", "_Store preset", NULL, "Store a current setting as preset", (GCallback)store_preset_action },
};

/***************************** GUI window ********************************************/

static const char *ui_xml = 
"<ui>\n"
"  <menubar>\n"
"    <menu action=\"HostMenuAction\">\n"
"      <menuitem action=\"exit\"/>\n"
"    </menu>\n"
"    <menu action=\"PresetMenuAction\">\n"
"      <menuitem action=\"store-preset\"/>\n"
"      <separator/>\n"
"      <placeholder name=\"presets\"/>\n"
"    </menu>\n"
"  </menubar>\n"
"</ui>\n"
;

static const char *preset_pre_xml = 
"<ui>\n"
"  <menubar>\n"
"    <menu action=\"PresetMenuAction\">\n"
"      <placeholder name=\"presets\">\n";

static const char *preset_post_xml = 
"      </placeholder>\n"
"    </menu>\n"
"  </menubar>\n"
"</ui>\n"
;

plugin_gui_window::plugin_gui_window()
{
    toplevel = NULL;
    ui_mgr = NULL;
    std_actions = NULL;
    preset_actions = NULL;
}

string plugin_gui_window::make_gui_preset_list(GtkActionGroup *grp)
{
    string preset_xml = preset_pre_xml;
    preset_vector &pvec = global_presets.presets;
    for (unsigned int i = 0; i < pvec.size(); i++)
    {
        if (global_presets.presets[i].plugin != gui->effect_name)
            continue;
        stringstream ss;
        ss << "preset" << i;
        preset_xml += "          <menuitem name=\""+pvec[i].name+"\" action=\""+ss.str()+"\"/>\n";
        
        GtkActionEntry ae = { ss.str().c_str(), NULL, pvec[i].name.c_str(), NULL, NULL, (GCallback)activate_preset };
        gtk_action_group_add_actions_full(preset_actions, &ae, 1, (gpointer)new activate_preset_params(gui, i), action_destroy_notify);
    }
    preset_xml += preset_post_xml;
    return preset_xml;
}

void plugin_gui_window::fill_gui_presets()
{
    if(preset_actions) {
        gtk_ui_manager_remove_action_group(ui_mgr, preset_actions);
        preset_actions = NULL;
    }
    
    preset_actions = gtk_action_group_new("presets");
    string preset_xml = make_gui_preset_list(preset_actions);
    gtk_ui_manager_insert_action_group(ui_mgr, preset_actions, 0);    
    GError *error = NULL;
    gtk_ui_manager_add_ui_from_string(ui_mgr, preset_xml.c_str(), -1, &error);
}

void plugin_gui_window::create(plugin_ctl_iface *_jh, const char *title, const char *effect)
{
    toplevel = GTK_WINDOW(gtk_window_new (GTK_WINDOW_TOPLEVEL));
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
    fill_gui_presets();
    
    gtk_box_pack_start(GTK_BOX(vbox), gtk_ui_manager_get_widget(ui_mgr, "/ui/menubar"), false, false, 0);

    // determine size without content
    gtk_widget_show_all(GTK_WIDGET(toplevel));
    gtk_widget_size_request(GTK_WIDGET(toplevel), &req2);
    // printf("size request %dx%d\n", req2.width, req2.height);
    
    GtkWidget *table = gui->create(_jh, effect);
    GtkWidget *sw = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw), GTK_SHADOW_NONE);
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(sw), table);
    
    gtk_box_pack_start(GTK_BOX(vbox), sw, true, true, 0);
    
    gtk_widget_show_all(GTK_WIDGET(sw));
    gtk_widget_size_request(table, &req);
    // printf("size request %dx%d\n", req.width, req.height);
    gtk_window_resize(GTK_WINDOW(toplevel), max(req.width + 10, req2.width), req.height + req2.height + 10);
    // gtk_scrolled_window_set_vadjustment(GTK_SCROLLED_WINDOW(sw), GTK_ADJUSTMENT(gtk_adjustment_new(0, 0, req.height, 20, 100, 100)));

}

plugin_gui_window::~plugin_gui_window()
{
    delete gui;
}
