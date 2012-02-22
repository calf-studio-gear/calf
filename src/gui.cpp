/* Calf DSP Library
 * GUI functions for a plugin.
 * Copyright (C) 2007-2011 Krzysztof Foltman
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
 
#include <calf/gui_config.h>
#include <calf/gui_controls.h>
#include <calf/preset.h>
#include <calf/preset_gui.h>
#include <gdk/gdk.h>

#include <iostream>

using namespace calf_plugins;
using namespace std;

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
    preset_access = new gui_preset_access(this);
}

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
    if (!strcmp(element, "check"))
        return new check_param_control;
    if (!strcmp(element, "radio"))
        return new radio_param_control;
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
    if (!strcmp(element, "phase-graph"))
        return new phase_graph_param_control;
    if (!strcmp(element, "keyboard"))
        return new keyboard_param_control;
    if (!strcmp(element, "curve"))
        return new curve_param_control;
    if (!strcmp(element, "led"))
        return new led_param_control;
    if (!strcmp(element, "tube"))
        return new tube_param_control;
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
        if (window->environment->check_condition(cond.c_str()) == state)
            return;
        ignore_stack = 1;
        return;
    }
    control_container *cc = create_container_from_xml(element, attributes);
    if (cc != NULL)
    {
        cc->attribs = xam;
        cc->create(this, element, xam);
        cc->set_std_properties();
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
            current_control->control_name = element;
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
                current_control->param_variable = plugin->get_metadata_iface()->get_param_props(param_no)->short_name;
            current_control->create(this, param_no);
            current_control->set_std_properties();
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
    int size = plugin->get_metadata_iface()->get_param_count();
    for (int i = 0; i < size; i++)
        param_name_map[plugin->get_metadata_iface()->get_param_props(i)->short_name] = i;
    
    XML_SetUserData(parser, this);
    XML_SetElementHandler(parser, xml_element_start, xml_element_end);
    XML_Status status = XML_Parse(parser, xml, strlen(xml), 1);
    if (status == XML_STATUS_ERROR)
    {
        g_error("Parse error: %s in XML", XML_ErrorString(XML_GetErrorCode(parser)));
    }
    
    XML_ParserFree(parser);
    last_status_serial_no = plugin->send_status_updates(this, 0);
    GtkWidget *eventbox  = gtk_event_box_new();
    GtkWidget *decoTable = gtk_table_new(3, 1, FALSE);
    
    // decorations
    // overall background
    //GtkWidget *bgImg     = gtk_image_new_from_file(PKGLIBDIR "/background.png");
    //gtk_widget_set_size_request(GTK_WIDGET(bgImg), 1, 1);

    // images for left side
    GtkWidget *nwImg     = gtk_image_new_from_file(PKGLIBDIR "/side_nw.png");
    GtkWidget *swImg     = gtk_image_new_from_file(PKGLIBDIR "/side_sw.png");
    GtkWidget *wImg      = gtk_image_new_from_file(PKGLIBDIR "/side_w.png");
    gtk_widget_set_size_request(GTK_WIDGET(wImg), 56, 1);
    
    // images for right side
    GtkWidget *neImg     = gtk_image_new_from_file(PKGLIBDIR "/side_ne.png");
    GtkWidget *seImg     = gtk_image_new_from_file(PKGLIBDIR "/side_se.png");
    GtkWidget *eImg      = gtk_image_new_from_file(PKGLIBDIR "/side_e.png");
    GtkWidget *logoImg   = gtk_image_new_from_file(PKGLIBDIR "/side_e_logo.png");
    gtk_widget_set_size_request(GTK_WIDGET(eImg), 56, 1);
    
    // pack left box
    leftBox = gtk_vbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(leftBox), GTK_WIDGET(nwImg), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(leftBox), GTK_WIDGET(wImg), TRUE, TRUE, 0);
    gtk_box_pack_end(GTK_BOX(leftBox), GTK_WIDGET(swImg), FALSE, FALSE, 0);
    
     // pack right box
    rightBox = gtk_vbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(rightBox), GTK_WIDGET(neImg), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(rightBox), GTK_WIDGET(eImg), TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(rightBox), GTK_WIDGET(logoImg), FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(rightBox), GTK_WIDGET(seImg), FALSE, FALSE, 0);
    
    //gtk_table_attach(GTK_TABLE(decoTable), GTK_WIDGET(bgImg),     0, 2, 0, 2, (GtkAttachOptions)(GTK_EXPAND | GTK_SHRINK | GTK_FILL), (GtkAttachOptions)(GTK_EXPAND | GTK_SHRINK | GTK_FILL), 0, 0);
    gtk_table_attach(GTK_TABLE(decoTable), GTK_WIDGET(leftBox),   0, 1, 0, 1, (GtkAttachOptions)(0), (GtkAttachOptions)(GTK_EXPAND | GTK_FILL), 0, 0);
    gtk_table_attach(GTK_TABLE(decoTable), GTK_WIDGET(rightBox),  2, 3, 0, 1, (GtkAttachOptions)(0), (GtkAttachOptions)(GTK_EXPAND | GTK_FILL), 0, 0);
        
    gtk_table_attach(GTK_TABLE(decoTable), GTK_WIDGET(top_container->container), 1, 2, 0, 1, (GtkAttachOptions)(GTK_EXPAND | GTK_FILL), (GtkAttachOptions)(GTK_EXPAND | GTK_FILL), 15, 5);
    gtk_container_add( GTK_CONTAINER(eventbox), decoTable );
    gtk_widget_set_name( GTK_WIDGET(eventbox), "Calf-whatever" );
    
    // create window with viewport
//    GtkWidget *sw = gtk_scrolled_window_new(NULL, NULL);
//    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_NEVER, GTK_POLICY_NEVER);
//    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw), GTK_SHADOW_NONE);
//    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(sw), GTK_WIDGET(decoTable));
    
    return GTK_WIDGET(eventbox);
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
            const parameter_properties &props = *plugin->get_metadata_iface()->get_param_props(params[i]->param_no);
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

/// Get a radio button group (if it exists) for a parameter
GSList *plugin_gui::get_radio_group(int param)
{
    map<int, GSList *>::const_iterator i = param_radio_groups.find(param);
    if (i == param_radio_groups.end())
        return NULL;
    else
        return i->second;
}

/// Set a radio button group for a parameter
void plugin_gui::set_radio_group(int param, GSList *group)
{
    param_radio_groups[param] = group;
}

void plugin_gui::show_rack_ears(bool show)
{
    // if hidden, add a no-show-all attribute so that LV2 host doesn't accidentally override
    // the setting by doing a show_all on the outermost container
    gtk_widget_set_no_show_all(leftBox, !show);
    gtk_widget_set_no_show_all(rightBox, !show);
    if (show)
    {
        gtk_widget_show(leftBox);
        gtk_widget_show(rightBox);
    }
    else
    {
        gtk_widget_hide(leftBox);
        gtk_widget_hide(rightBox);
    }
}

plugin_gui::~plugin_gui()
{
    delete preset_access;
    for (std::vector<param_control *>::iterator i = params.begin(); i != params.end(); i++)
    {
        delete *i;
    }
}

/***************************** GUI environment ********************************************/

gui_environment::gui_environment()
{
    keyfile = g_key_file_new();
    
    gchar *fn = g_build_filename(getenv("HOME"), ".calfrc", NULL);
    string filename = fn;
    g_free(fn);
    g_key_file_load_from_file(keyfile, filename.c_str(), (GKeyFileFlags)(G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS), NULL);
    
    config_db = new calf_utils::gkeyfile_config_db(keyfile, filename.c_str(), "gui");
    gui_config.load(config_db);
}

gui_environment::~gui_environment()
{
    delete config_db;
    if (keyfile)
        g_key_file_free(keyfile);
}

