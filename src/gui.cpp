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
    if (param_no != -1)
        gui->add_param_ctl(param_no, this);
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
    printf("param value %d = %f\n", param_no, gui->plugin->get_param_value(param_no));
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

    widget = gtk_vscale_new_with_range (0, 1, 0.01);
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
    string text = "<no name>";
    if (param_no != -1)
        text = get_props().name;
    widget = gtk_label_new(text.c_str());
    return widget;
}

// value

GtkWidget *value_param_control::create(plugin_gui *_gui, int _param_no)
{
    gui = _gui, param_no = _param_no;
    parameter_properties &props = get_props();
    widget = gtk_label_new ("");
    gtk_label_set_width_chars (GTK_LABEL (widget), props.get_char_count());
    gtk_misc_set_alignment (GTK_MISC (widget), 0.5, 0.5);
    return widget;
}

void value_param_control::set()
{
    _GUARD_CHANGE_
    parameter_properties &props = get_props();
    gtk_label_set_text (GTK_LABEL (widget), props.to_string(gui->plugin->get_param_value(param_no)).c_str());    
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

// knob

#if USE_PHAT
GtkWidget *knob_param_control::create(plugin_gui *_gui, int _param_no)
{
    gui = _gui;
    param_no = _param_no;
    const parameter_properties &props = get_props();

    float increment = props.get_increment();
    widget = phat_knob_new_with_range (props.to_01 (gui->plugin->get_param_value(param_no)), 0, 1, increment);
    gtk_widget_set_size_request(widget, 50, 50);
    gtk_signal_connect(GTK_OBJECT(widget), "value-changed", G_CALLBACK(knob_value_changed), (gpointer)this);
    return widget;
}

#else

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

#endif

void knob_param_control::get()
{
    const parameter_properties &props = get_props();
#if USE_PHAT
    float value = props.from_01(phat_knob_get_value(PHAT_KNOB(widget)));
#else
    float value = props.from_01(gtk_range_get_value(GTK_RANGE(widget)));
#endif
    gui->set_param_value(param_no, value, this);
    if (label)
        update_label();
}

void knob_param_control::set()
{
    _GUARD_CHANGE_
    const parameter_properties &props = get_props();
#if USE_PHAT
    phat_knob_set_value(PHAT_KNOB(widget), props.to_01 (gui->plugin->get_param_value(param_no)));
#else
    gtk_range_set_value(GTK_RANGE(widget), props.to_01 (gui->plugin->get_param_value(param_no)));
#endif
    if (label)
        update_label();
}

void knob_param_control::knob_value_changed(GtkWidget *widget, gpointer value)
{
    param_control *jhp = (param_control *)value;
    jhp->get();
}

// line graph

gboolean line_graph_param_control::update(void *data)
{
    line_graph_param_control *self = (line_graph_param_control *)data;
    self->set();
    return TRUE;
}

GtkWidget *line_graph_param_control::create(plugin_gui *_gui, int _param_no)
{
    gui = _gui;
    param_no = _param_no;
    // const parameter_properties &props = get_props();
    
    widget = calf_line_graph_new ();
    CalfLineGraph *clg = CALF_LINE_GRAPH(widget);
    widget->requisition.width = get_int("width", 40);
    widget->requisition.height = get_int("height", 40);
    clg->source = gui->plugin->get_line_graph_iface();
    clg->source_id = param_no;
    
    if (get_int("refresh", 0))
        source_id = g_timeout_add_full(G_PRIORITY_LOW, 1000/30, update, this, NULL); // 30 fps should be enough for everybody
    
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
        gtk_widget_queue_draw(widget);
    }
}

line_graph_param_control::~line_graph_param_control()
{
    if (get_int("refresh", 0))
        g_source_remove(source_id);
}

/******************************** GUI proper ********************************/

plugin_gui::plugin_gui(plugin_gui_window *_window)
: window(_window)
{
    
}

static void action_destroy_notify(gpointer data)
{
    delete (activate_preset_params *)data;
}

GtkWidget *plugin_gui::create(plugin_ctl_iface *_plugin)
{
    plugin = _plugin;
    param_count = plugin->get_param_count();
    params.resize(param_count);
    for (int i = 0; i < param_count; i++) {
        params[i] = NULL;
    }
    
    container = gtk_table_new (param_count, 3, FALSE);
    
    for (int i = 0; i < param_count; i++) {
        int trow = i;
        parameter_properties &props = *plugin->get_param_props(i);
        GtkWidget *label = gtk_label_new (props.name);
        gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
        gtk_table_attach (GTK_TABLE (container), label, 0, 1, trow, trow + 1, GTK_FILL, GTK_FILL, 2, 2);
        
        
        GtkWidget *widget = NULL;
        
        if ((props.flags & PF_TYPEMASK) == PF_ENUM && 
            (props.flags & PF_CTLMASK) == PF_CTL_COMBO)
        {
            params[i] = new combo_box_param_control();
            widget = params[i]->create(this, i);
            gtk_table_attach (GTK_TABLE (container), widget, 1, 3, trow, trow + 1, GTK_EXPAND, GTK_SHRINK, 0, 0);
        }
        else if ((props.flags & PF_TYPEMASK) == PF_BOOL && 
                 (props.flags & PF_CTLMASK) == PF_CTL_TOGGLE)
        {
            params[i] = new toggle_param_control();
            widget = params[i]->create(this, i);
            gtk_table_attach (GTK_TABLE (container), widget, 1, 3, trow, trow + 1, GTK_EXPAND, GTK_SHRINK, 0, 0);
        }
        else if ((props.flags & PF_CTLMASK) != PF_CTL_FADER)
        {
            params[i] = new knob_param_control();
            widget = params[i]->create(this, i);
            gtk_table_attach (GTK_TABLE (container), widget, 1, 2, trow, trow + 1, GTK_SHRINK, GTK_SHRINK, 0, 0);
            gtk_table_attach (GTK_TABLE (container), params[i]->create_label(), 2, 3, trow, trow + 1, (GtkAttachOptions)(GTK_SHRINK | GTK_FILL), GTK_SHRINK, 0, 0);
        }
        else
        {
            gtk_misc_set_alignment (GTK_MISC (label), 1.0, 1.0);
            params[i] = new hscale_param_control();
            widget = params[i]->create(this, i);
            gtk_table_attach (GTK_TABLE (container), widget, 1, 3, trow, trow + 1, (GtkAttachOptions)(GTK_EXPAND | GTK_FILL), GTK_SHRINK, 0, 0);
        }
        params[i]->hook_params();
        params[i]->set();
    }
    return container;
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
    int fillx = (base->get_int("fill-x", 1) ? GTK_FILL : 0) | (base->get_int("expand-x", 1) ? GTK_EXPAND : 0) | (base->get_int("shrink-x", 0) ? GTK_SHRINK : 0);
    int filly = (base->get_int("fill-y", 1) ? GTK_FILL : 0) | (base->get_int("expand-y", 1) ? GTK_EXPAND : 0) | (base->get_int("shrink-y", 0) ? GTK_SHRINK : 0);
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
    if (!strcmp(element, "label"))
        return new label_param_control;
    if (!strcmp(element, "value"))
        return new value_param_control;
    if (!strcmp(element, "line-graph"))
        return new line_graph_param_control;
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
            g_error("Incorrect <if cond=\"[!]symbol\"> element");
        string cond = xam["cond"];
        unsigned int exp_count = 1;
        if (cond.substr(0, 1) == "!") {
            exp_count = 0;
            cond.erase(0, 1);
        }
        if (window->conditions.count(cond) == exp_count)
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
            current_control->create(this, param_no);
            current_control->init_xml(element);
            current_control->set();
            current_control->hook_params();
            params.push_back(current_control);
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
        return;
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
        g_error("Parse error: %s in XML", XML_ErrorString(XML_GetErrorCode(parser)));
    
    XML_ParserFree(parser);
    return GTK_WIDGET(top_container->container);
}

void plugin_gui::refresh()
{
    for (unsigned int i = 0; i < params.size(); i++)
    {
        if (params[i] != NULL)
            params[i]->set();
    }
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
    fill_gui_presets();
    
    gtk_box_pack_start(GTK_BOX(vbox), gtk_ui_manager_get_widget(ui_mgr, "/ui/menubar"), false, false, 0);

    // determine size without content
    gtk_widget_show_all(GTK_WIDGET(vbox));
    gtk_widget_size_request(GTK_WIDGET(vbox), &req2);
    // printf("size request %dx%d\n", req2.width, req2.height);
    
    GtkWidget *container;
    const char *xml = _jh->get_gui_xml();
    if (xml)
        container = gui->create_from_xml(_jh, xml);
    else
        container = gui->create(_jh);
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
    all_windows.insert(this);
}

plugin_gui_window::~plugin_gui_window()
{
    all_windows.erase(this);
    delete gui;
}

std::set<plugin_gui_window *> plugin_gui_window::all_windows;

void plugin_gui_window::refresh_all_presets()
{
    for (std::set<plugin_gui_window *>::iterator i = all_windows.begin(); i != all_windows.end(); i++)
        (*i)->fill_gui_presets();
}

