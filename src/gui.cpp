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

plugin_gui::plugin_gui(plugin_gui_widget *_window)
: last_status_serial_no(0)
, window(_window)
{
    plugin = NULL;
    ignore_stack = 0;
    top_container = NULL;
    param_count = 0;
    container = NULL;
    effect_name = NULL;
    preset_access = new gui_preset_access(this);
    optclosed = false;
    optwidget = NULL;
    optwindow = NULL;
    opttitle = NULL;
}

control_base *plugin_gui::create_widget_from_xml(const char *element, const char *attributes[])
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
    if (!strcmp(element, "tap"))
        return new tap_button_param_control;
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
    if (!strcmp(element, "tuner"))
        return new tuner_param_control;
    if (!strcmp(element, "pattern"))
        return new pattern_param_control;
    if (!strcmp(element, "keyboard"))
        return new keyboard_param_control;
    if (!strcmp(element, "curve"))
        return new curve_param_control;
    if (!strcmp(element, "meterscale"))
        return new meter_scale_param_control;
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
    if (!strcmp(element, "notebook"))
        return new notebook_param_control;
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
    if (!strcmp(element, "scrolled"))
        return new scrolled_container;
    return NULL;
}

void plugin_gui::xml_element_start(void *data, const char *element, const char *attributes[])
{
    plugin_gui *gui = (plugin_gui *)data;
    gui->xml_element_start(element, attributes);
}

int plugin_gui::get_param_no_by_name(string param_name)
{
    int param_no = -1;
    map<string, int>::iterator it = param_name_map.find(param_name);
    if (it == param_name_map.end())
        g_error("Unknown parameter %s", param_name.c_str());
    else
        param_no = it->second;

    return param_no;
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
        if (window->get_environment()->check_condition(cond.c_str()) == state)
            return;
        ignore_stack = 1;
        return;
    }
    control_base *cc = create_widget_from_xml(element, attributes);
    if (cc == NULL)
        g_error("Unxpected element %s in GUI definition\n", element);
    
    cc->attribs = xam;
    cc->create(this);
    stack.push_back(cc);
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

    control_base *control = gui->stack.back();
    control->created();
    
    gui->stack.pop_back();
    if (gui->stack.empty())
    {
        gui->top_container = control;
        gtk_widget_show_all(control->widget);
    }
    else
        gui->stack.back()->add(control);
}


GtkWidget *plugin_gui::create_from_xml(plugin_ctl_iface *_plugin, const char *xml)
{
    top_container = NULL;
    parser = XML_ParserCreate("UTF-8");
    plugin = _plugin;
    stack.clear();
    ignore_stack = 0;
    
    param_name_map.clear();
    read_serials.clear();
    int size = plugin->get_metadata_iface()->get_param_count();
    read_serials.resize(size);
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
    return top_container->widget;
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

void plugin_gui::remove_param_ctl(int param, param_control *ctl)
{
    std::multimap<int, param_control *>::iterator it = par2ctl.find(param);
    while(it != par2ctl.end() && it->first == param)
    {
        if (it->second == ctl)
        {
            std::multimap<int, param_control *>::iterator orig = it;
            ++orig;
            par2ctl.erase(it, orig);
            it = orig;
        }
        else
            ++it;
    }
    unsigned last = params.size() - 1;
    for (unsigned i = 0; i < params.size(); ++i)
    {
        if (params[i] == ctl)
        {
            if (i != last)
                std::swap(params[i], params[last]);
            params.erase(params.begin() + last, params.end());
            last--;
        }
    }
}

void plugin_gui::on_idle()
{
    set<unsigned> changed;
    for (unsigned i = 0; i < read_serials.size(); i++)
    {
        int write_serial = plugin->get_write_serial(i);
        if (write_serial - read_serials[i] > 0)
        {
            read_serials[i] = write_serial;
            changed.insert(i);
        }
    }
    for (unsigned i = 0; i < params.size(); i++)
    {
        int param_no = params[i]->param_no;
        if (param_no != -1)
        {
            const parameter_properties &props = *plugin->get_metadata_iface()->get_param_props(param_no);
            bool is_output = (props.flags & PF_PROP_OUTPUT) != 0;
            if (is_output || (param_no != -1 && changed.count(param_no)))
                params[i]->set();
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
        ++it;
    }
}

void plugin_gui::set_param_value(int param_no, float value, param_control *originator)
{
    plugin->set_param_value(param_no, value);

    main_window_iface *main = window->get_main_window();
    if (main)
        main->refresh_plugin_param(plugin, param_no);
    else
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

void plugin_gui::on_automation_add(GtkWidget *widget, void *user_data)
{
    plugin_gui *self = (plugin_gui *)user_data;
    self->plugin->add_automation(self->context_menu_last_designator, automation_range(0, 1, self->context_menu_param_no));
}

void plugin_gui::on_automation_delete(GtkWidget *widget, void *user_data)
{
    automation_menu_entry *ame = (automation_menu_entry *)user_data;
    ame->gui->plugin->delete_automation(ame->source, ame->gui->context_menu_param_no);
}

void plugin_gui::on_automation_set_lower_or_upper(automation_menu_entry *ame, bool is_upper)
{
    const parameter_properties *props = plugin->get_metadata_iface()->get_param_props(context_menu_param_no);
    float mapped = props->to_01(plugin->get_param_value(context_menu_param_no));
    
    automation_map mappings;
    plugin->get_automation(context_menu_param_no, mappings);
    automation_map::const_iterator i = mappings.find(ame->source);
    if (i != mappings.end())
    {
        if (is_upper)
            plugin->add_automation(context_menu_last_designator, automation_range(i->second.min_value, mapped, context_menu_param_no));
        else
            plugin->add_automation(context_menu_last_designator, automation_range(mapped, i->second.max_value, context_menu_param_no));
    }
}

void plugin_gui::on_automation_set_lower(GtkWidget *widget, void *user_data)
{
    automation_menu_entry *ame = (automation_menu_entry *)user_data;
    ame->gui->on_automation_set_lower_or_upper(ame, false);
}

void plugin_gui::on_automation_set_upper(GtkWidget *widget, void *user_data)
{
    automation_menu_entry *ame = (automation_menu_entry *)user_data;
    ame->gui->on_automation_set_lower_or_upper(ame, true);
}

void plugin_gui::cleanup_automation_entries()
{
    for (int i = 0; i < (int)automation_menu_callback_data.size(); i++)
        delete automation_menu_callback_data[i];
    automation_menu_callback_data.clear();
}

void plugin_gui::on_control_popup(param_control *ctl, int param_no)
{
    cleanup_automation_entries();
    if (param_no == -1)
        return;
    context_menu_param_no = param_no;
    GtkWidget *menu = gtk_menu_new();
    
    multimap<uint32_t, automation_range> mappings;
    plugin->get_automation(param_no, mappings);
    
    context_menu_last_designator = plugin->get_last_automation_source();
    
    GtkWidget *item;
    if (context_menu_last_designator != 0xFFFFFFFF)
    {
        stringstream ss;
        ss << "_Bind to: Ch" << (1 + (context_menu_last_designator >> 8)) << ", CC#" << (context_menu_last_designator & 127);
        item = gtk_menu_item_new_with_mnemonic(ss.str().c_str());
        g_signal_connect(item, "activate", (GCallback)on_automation_add, this);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
    }
    else
    {
        item = gtk_menu_item_new_with_label("Send CC to automate");
        gtk_widget_set_sensitive(item, FALSE);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
    }
    
    for(multimap<uint32_t, automation_range>::const_iterator i = mappings.begin(); i != mappings.end(); ++i)
    {
        automation_menu_entry *ame = new automation_menu_entry(this, i->first);
        automation_menu_callback_data.push_back(ame);
        stringstream ss;
        ss << "Mapping: Ch" << (1 + (i->first >> 8)) << ", CC#" << (i->first & 127);
        item = gtk_menu_item_new_with_label(ss.str().c_str());
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
        
        GtkWidget *submenu = gtk_menu_new();        
        gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), submenu);
        
        item = gtk_menu_item_new_with_mnemonic("_Delete");
        g_signal_connect(item, "activate", (GCallback)on_automation_delete, ame);
        gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);

        item = gtk_menu_item_new_with_mnemonic("Set _lower limit");
        g_signal_connect(item, "activate", (GCallback)on_automation_set_lower, ame);
        gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);

        item = gtk_menu_item_new_with_mnemonic("Set _upper limit");
        g_signal_connect(item, "activate", (GCallback)on_automation_set_upper, ame);
        gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
        //g_signal_connect(item, "activate", (GCallback)on_automation_add, this);
        
    }
    
    gtk_widget_show_all(menu);
    gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, 3, gtk_get_current_event_time());
}

void plugin_gui::destroy_child_widgets(GtkWidget *parent)
{
    if (parent && GTK_IS_CONTAINER(parent))
    {
        GList *children = gtk_container_get_children(GTK_CONTAINER(parent));
        for(GList *p = children; p; p = p->next)
            gtk_widget_destroy(GTK_WIDGET(p->data));
        g_list_free(children);
    }
}

plugin_gui::~plugin_gui()
{
    cleanup_automation_entries();
    delete preset_access;
}

/***************************** GUI environment ********************************************/

bool window_update_controller::check_redraw(GtkWidget *toplevel)
{
    GdkWindow *gdkwin = gtk_widget_get_window(toplevel);
    if (!gdkwin)
        return false;

    if (!gdk_window_is_viewable(gdkwin))
        return false;
    GdkWindowState state = gdk_window_get_state(gdkwin);
    if (state & GDK_WINDOW_STATE_ICONIFIED)
    {
        ++refresh_counter;
        if (refresh_counter & 15)
            return false;
    }
    return true;
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
    images = image_factory();
    images.set_path(PKGLIBDIR "styles/" + get_config()->style);
}

gui_environment::~gui_environment()
{
    delete config_db;
    if (keyfile)
        g_key_file_free(keyfile);
}


/***************************** Image Factory **************************************/
GdkPixbuf *image_factory::create_image (string image) {
    string file = path + "/" + image + ".png";
    if (access(file.c_str(), F_OK))
        return NULL;
    return gdk_pixbuf_new_from_file(file.c_str(), NULL);
}
void image_factory::recreate_images () {
    for (map<string, GdkPixbuf*>::iterator i_ = i.begin(); i_ != i.end(); i_++) {
        if (i[i_->first])
            i[i_->first] = create_image(i_->first);
    }
}
void image_factory::set_path (string p) {
    path = p;
    recreate_images();
}
GdkPixbuf *image_factory::get (string image) {
    if (!i.count(image))
        return NULL;
    if (!i.at(image))
        i[image] = create_image(image);
    return i[image];
}
gboolean image_factory::available (string image) {
    string file = path + "/" + image + ".png";
    if (access(file.c_str(), F_OK))
        return false;
    return true;
}
image_factory::image_factory (string p) {
    set_path(p);
    
    i["combo_arrow"]              = NULL;
    i["light_top"]                = NULL;
    i["light_bottom"]             = NULL;
    i["notebook_screw"]           = NULL;
    i["logo_button"]              = NULL;
    
    i["knob_1"]                    = NULL;
    i["knob_2"]                    = NULL;
    i["knob_3"]                    = NULL;
    i["knob_4"]                    = NULL;
    i["knob_5"]                    = NULL;
    
    i["side_d_ne"]                = NULL;
    i["side_d_nw"]                = NULL;
    i["side_d_se"]                = NULL;
    i["side_d_sw"]                = NULL;
    
    i["side_ne"]                  = NULL;
    i["side_nw"]                  = NULL;
    i["side_se"]                  = NULL;
    i["side_sw"]                  = NULL;
    i["side_e_logo"]              = NULL;
    
    i["slider_1_horiz"]            = NULL;
    i["slider_1_vert"]             = NULL;
    i["slider_2_horiz"]            = NULL;
    i["slider_2_vert"]             = NULL;
    
    i["tap_active"]               = NULL;
    i["tap_inactive"]             = NULL;
    i["tap_prelight"]             = NULL;
    
    i["toggle_0"]                 = NULL;
    i["toggle_1"]                 = NULL;
    i["toggle_2"]                 = NULL;
    
    i["toggle_2_block"]           = NULL;
    i["toggle_2_bypass"]          = NULL;
    i["toggle_2_bypass2"]         = NULL;
    i["toggle_2_fast"]            = NULL;
    i["toggle_2_listen"]          = NULL;
    i["toggle_2_logarithmic"]     = NULL;
    i["toggle_2_magnetical"]      = NULL;
    i["toggle_2_mono"]            = NULL;
    i["toggle_2_muffle"]          = NULL;
    i["toggle_2_mute"]            = NULL;
    i["toggle_2_phase"]           = NULL;
    i["toggle_2_sc_comp"]         = NULL;
    i["toggle_2_sc_filter"]       = NULL;
    i["toggle_2_softclip"]        = NULL;
    i["toggle_2_solo"]            = NULL;
    i["toggle_2_sync"]            = NULL;
    i["toggle_2_void"]            = NULL;
    i["toggle_2_gui"]             = NULL;
    i["toggle_2_connect"]         = NULL;
    i["toggle_2_pauseplay"]       = NULL;
}
image_factory::~image_factory() {
    
}
