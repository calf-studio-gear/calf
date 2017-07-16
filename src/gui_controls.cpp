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
 
#include <calf/gui_controls.h>

using namespace calf_plugins;
using namespace calf_utils;
using namespace std;

#define SANITIZE(value) (std::abs(value) < small_value<float>()) ? 0.f : value

/******************************** control/container base class **********************/

void control_base::require_attribute(const char *name)
{
    if (attribs.count(name) == 0) {
        g_error("Missing attribute '%s' in control '%s'", name, control_name.c_str());
    }
}

void control_base::require_int_attribute(const char *name)
{
    require_attribute(name);
    if (attribs[name].empty() || attribs[name].find_first_not_of("0123456789") != string::npos) {
        g_error("Wrong data type on attribute '%s' in control '%s' (required integer)", name, control_name.c_str());
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

std::vector<double> control_base::get_vector(const char *name, std::string &value)
{
    std::vector<double> t;
    
    if (attribs.count(name)) {
        value = attribs[name];
    }
        
    string::size_type lpos = value.find_first_not_of(" ", 0);
    string::size_type pos  = value.find_first_of(" ", lpos);
    while (string::npos != pos || string::npos != lpos) {
        double val;
        stringstream stream(value.substr(lpos, pos - lpos).c_str());
        stream >> val;
        t.push_back(val);
        lpos = value.find_first_not_of(" ", pos);
        pos  = value.find_first_of(" ", lpos);
    }
    return t;
}

void control_base::set_visibilty(bool state)
{
    if (state) {
        gtk_widget_show(widget);
    } else {
        gtk_widget_hide(widget);
    }
}

void control_base::set_std_properties()
{
    if (widget && attribs.find("widget-name") != attribs.end())
    {
        string name = attribs["widget-name"];
        gtk_widget_set_name(widget, name.c_str());
    }
    if (widget && GTK_IS_CONTAINER(widget))
    {
        gtk_container_set_border_width(GTK_CONTAINER(widget), get_int("border"));
    }
}

static void on_control_destroy(GtkWidget *w, gpointer p)
{
    delete (control_base *)p;
}

void control_base::created()
{
    set_std_properties();
    g_signal_connect(GTK_OBJECT(widget), "destroy", (GCallback)on_control_destroy, this);
}

/************************* param-associated control base class **************/

param_control::param_control()
{
    gui = NULL;
    param_no = -1;
    in_change = 0;
    old_displayed_value = -1.f;
    has_entry = false;
}

GtkWidget *param_control::create(plugin_gui *_gui)
{
    if (attribs.count("param"))
    {
        int pno = _gui->get_param_no_by_name(attribs["param"]);
        param_variable = _gui->plugin->get_metadata_iface()->get_param_props(pno)->short_name;
        return create(_gui, pno);
    }
    else
        return create(_gui, -1);
}

void param_control::hook_params()
{
    if (param_no != -1) {
        gui->add_param_ctl(param_no, this);
    }
    gui->params.push_back(this);
}

void param_control::created() {
    control_base::created();
    set();
    hook_params();
    add_context_menu_handler();
}

param_control::~param_control()
{
    if (param_no != -1)
        gui->remove_param_ctl(param_no, this);
    //if (GTK_IS_WIDGET(widget))
    //    gtk_widget_destroy(widget);
}

void param_control::add_context_menu_handler()
{
    if (widget)
    {
        g_signal_connect(GTK_OBJECT(widget), "button-press-event", (GCallback)on_button_press_event, this);
    }
}

gboolean param_control::on_button_press_event(GtkWidget *widget, GdkEventButton *event, void *user_data)
{
    param_control *self = (param_control *)user_data;
    const parameter_properties &props = self->get_props();
    if (event->button == 3 && !(props.flags & PF_PROP_OUTPUT))
    {
        self->do_popup_menu();
        return TRUE;
    }
    else if (event->button == 2)
    {
        if (!strcmp(gtk_widget_get_name(widget), "Calf-LineGraph")) {
            CalfLineGraph *clg = CALF_LINE_GRAPH(widget);
            if (clg->freqhandles && clg->handle_hovered >= 0)  {
                FreqHandle * fh = &clg->freq_handles[clg->handle_hovered];
                self->param_no = fh->param_x_no;
            } else
                return FALSE;
        }
        self->create_value_entry(widget, event->x_root, event->y_root);
        return TRUE;
    }
    return FALSE;
}

void param_control::do_popup_menu()
{
    if (gui)
        gui->on_control_popup(this, param_no);
}

void param_control::destroy_value_entry ()
{
    // remove the window containing the entry
    gtk_widget_destroy(GTK_WIDGET(entrywin));
    has_entry = false;
}
gboolean param_control::value_entry_unfocus(GtkWidget *widget, GdkEventFocus *event, void *user_data)
{
    // destroy window if it looses focus
    param_control *self = (param_control *)user_data;
    self->destroy_value_entry();
    return TRUE;
}
gboolean param_control::value_entry_action(GtkEntry *widget, GdkEvent *event, void *user_data)
{
    // called when a key was hit, sorts out and treats RETURN and ESC
    param_control *self = (param_control *)user_data;
    const parameter_properties &props = self->get_props();
    GdkEventKey *key = (GdkEventKey*)event;
    if(key->keyval == GDK_Escape)
        self->destroy_value_entry();
    else if (key->keyval == GDK_Return) {
        float val = props.string_to_value(gtk_entry_get_text(widget));
        self->gui->plugin->set_param_value(self->param_no, val);
        self->set();
        self->destroy_value_entry();
    }
    return FALSE;
}
void param_control::create_value_entry(GtkWidget *widget, int x, int y)
{
    if (has_entry) {
        // kill an existing entry window on re-trigger
        destroy_value_entry();
        return;
    }
    
    if (param_no < 0)
        return;
        
    const parameter_properties &props = get_props();
    float value = gui->plugin->get_param_value(param_no);
    
    // no chance for a menu, so we have to do everything by hand
    entrywin = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_widget_set_name(GTK_WIDGET(entrywin), "Calf-Value-Entry");
    gtk_window_set_title (GTK_WINDOW(entrywin), "Calf Value Entry");
    gtk_window_set_resizable (GTK_WINDOW(entrywin), FALSE);
    gtk_window_set_decorated (GTK_WINDOW(entrywin), FALSE);
    gtk_window_set_skip_taskbar_hint (GTK_WINDOW(entrywin), TRUE);
    gtk_window_set_skip_pager_hint (GTK_WINDOW(entrywin), TRUE);
    gtk_window_set_transient_for (GTK_WINDOW(entrywin), GTK_WINDOW (gtk_widget_get_toplevel(gui->window->toplevel)));
    gtk_window_set_gravity(GTK_WINDOW(entrywin), GDK_GRAVITY_CENTER);
    gtk_widget_set_events (GTK_WIDGET(entrywin), GDK_FOCUS_CHANGE_MASK);
    g_signal_connect (G_OBJECT(entrywin), "focus-out-event", G_CALLBACK (value_entry_unfocus), this);
    
    // create the text entry
    GtkWidget *entry = gtk_entry_new();
    gtk_widget_set_name(GTK_WIDGET(entry), "Calf-Entry");
    gtk_entry_set_width_chars(GTK_ENTRY(entry), props.get_char_count());
    gtk_entry_set_text(GTK_ENTRY(entry), props.to_string(value).c_str());
    gtk_widget_add_events (entry, GDK_KEY_PRESS_MASK);
    g_signal_connect (entry, "key-press-event", (GCallback)value_entry_action, this);
    
    // stitch together and show
    gtk_container_add(GTK_CONTAINER (entrywin), entry);
    gtk_widget_show_all(entrywin);
    gtk_window_move(GTK_WINDOW (entrywin), x, y);
    
    has_entry = true;
}


/******************************** Combo Box ********************************/

GtkWidget *combo_box_param_control::create(plugin_gui *_gui, int _param_no)
{
    gui = _gui;
    param_no = _param_no;
    lstore = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING); // value, key
    populating = false;
    
    const parameter_properties &props = get_props();
    widget  = calf_combobox_new ();
    if (param_no != -1 && props.choices)
    {
        for (int j = (int)props.min; j <= (int)props.max; j++)
            gtk_list_store_insert_with_values (lstore, NULL, j - (int)props.min, 0, props.choices[j - (int)props.min], 1, calf_utils::i2s(j).c_str(), -1);
    }
    
    // set pixbuf
    calf_combobox_set_arrow(CALF_COMBOBOX(widget),
        gui->window->get_environment()->get_image_factory()->get("combo_arrow"));
    
    gtk_combo_box_set_model (GTK_COMBO_BOX(widget), GTK_TREE_MODEL(lstore));
    g_signal_connect (GTK_OBJECT (widget), "changed", G_CALLBACK (combo_value_changed), (gpointer)this);
    gtk_widget_set_name(widget, "Calf-Combobox");
    return widget;
}

void combo_box_param_control::set()
{
    _GUARD_CHANGE_
    if (param_no != -1)
    {
        const parameter_properties &props = get_props();
        gtk_combo_box_set_active (GTK_COMBO_BOX (widget), (int)gui->plugin->get_param_value(param_no) - (int)props.min);
        gtk_widget_queue_draw(widget);
    }
}

void combo_box_param_control::get()
{
    if (param_no != -1)
    {
        const parameter_properties &props = get_props();
        gui->set_param_value(param_no, gtk_combo_box_get_active (GTK_COMBO_BOX(widget)) + props.min, this);
    }
}

void combo_box_param_control::combo_value_changed(GtkComboBox *widget, gpointer value)
{
    combo_box_param_control *jhp = (combo_box_param_control *)value;
    if (jhp->populating)
        return;
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
        if (value == last_list)
            return;
        populating = true;
        last_list = value;
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
        populating = false;
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

/******************************** Horizontal Fader ********************************/

static gboolean
scale_to_default (gpointer data)
{
    hscale_param_control *jhp = (hscale_param_control *)data;
    const parameter_properties &props = jhp->get_props();
    gtk_range_set_value (GTK_RANGE (jhp->widget), props.to_01(props.def_value));

    return FALSE;
}

static gboolean
scale_button_press (GtkWidget *widget, GdkEventKey *event, gpointer *user_data)
{
  if (event->type == GDK_2BUTTON_PRESS) {
    // this actually creates a harmless race condition, but diving deep
    // into gtk signal handling code wouldn't and the resulting complexity
    // would not really be worth the effort
    // The timeout is set high enough that most of the time the race
    // will turn out in our/the users favor
    g_timeout_add (200, (GSourceFunc)scale_to_default, user_data);
    return TRUE;
  }

  return FALSE;
}

GtkWidget *hscale_param_control::create(plugin_gui *_gui, int _param_no)
{
    gui = _gui;
    param_no = _param_no;

    widget = calf_fader_new(1, get_int("size", 2), 0, 1, get_props().get_increment());
    
    g_signal_connect (GTK_OBJECT (widget), "value-changed", G_CALLBACK (hscale_value_changed), (gpointer)this);
    g_signal_connect (GTK_OBJECT (widget), "format-value", G_CALLBACK (hscale_format_value), (gpointer)this);
    g_signal_connect (GTK_OBJECT (widget), "button-press-event", G_CALLBACK (scale_button_press), (gpointer)this);
    
    if(get_int("inverted", 0) > 0) {
        gtk_range_set_inverted(GTK_RANGE(widget), TRUE);
    }
    int size = get_int("size", 2);
    
    // set pixbuf
    image_factory *images = gui->window->get_environment()->get_image_factory();
    char iname[64];
    sprintf(iname, "slider_%d_horiz", size);
    calf_fader_set_pixbuf(CALF_FADER(widget), images->get(iname));
    
    char *name = g_strdup_printf("Calf-HScale%i", size);
    gtk_widget_set_name(GTK_WIDGET(widget), name);
    gtk_widget_set_size_request (widget, size * 100, -1);
    g_free(name);

    if (attribs.count("width"))
        gtk_widget_set_size_request (widget, get_int("width", 200), -1);
    if (attribs.count("position")) {
        string v = attribs["position"];
        if (v == "top") gtk_scale_set_value_pos(GTK_SCALE(widget), GTK_POS_TOP);
        if (v == "bottom") gtk_scale_set_value_pos(GTK_SCALE(widget), GTK_POS_BOTTOM);
        if (v == "left") gtk_scale_set_value_pos(GTK_SCALE(widget), GTK_POS_LEFT);
        if (v == "right") gtk_scale_set_value_pos(GTK_SCALE(widget), GTK_POS_RIGHT);
    }
    return widget;
}

void hscale_param_control::set()
{
    _GUARD_CHANGE_
    const parameter_properties &props = get_props();
    gtk_range_set_value (GTK_RANGE (widget), props.to_01 (gui->plugin->get_param_value(param_no)));
    // hscale_value_changed (GTK_HSCALE (widget), (gpointer)this);
}

void hscale_param_control::get()
{
    const parameter_properties &props = get_props();
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

/******************************** Vertical Fader ********************************/

GtkWidget *vscale_param_control::create(plugin_gui *_gui, int _param_no)
{
    gui = _gui;
    param_no = _param_no;
    widget = calf_fader_new(0, get_int("size", 2), 0, 1, get_props().get_increment());
    g_signal_connect (GTK_OBJECT (widget), "value-changed", G_CALLBACK (vscale_value_changed), (gpointer)this);
    g_signal_connect (GTK_OBJECT (widget), "button-press-event", G_CALLBACK (scale_button_press), (gpointer)this);

    gtk_scale_set_draw_value(GTK_SCALE(widget), FALSE);
    
    if(get_int("inverted", 0) > 0) {
        gtk_range_set_inverted(GTK_RANGE(widget), TRUE);
    }
    int size = get_int("size", 2);
    
    // set pixbuf
    image_factory *images = gui->window->get_environment()->get_image_factory();
    char iname[64];
    sprintf(iname, "slider_%d_vert", size);
    calf_fader_set_pixbuf(CALF_FADER(widget), images->get(iname));
    
    char *name = g_strdup_printf("Calf-VScale%i", size);
    gtk_widget_set_size_request (widget, -1, size * 100);
    gtk_widget_set_name(GTK_WIDGET(widget), name);
    g_free(name);

    if (attribs.count("height"))
        gtk_widget_set_size_request (widget, -1, get_int("height", 200));

    return widget;
}

void vscale_param_control::set()
{
    _GUARD_CHANGE_
    const parameter_properties &props = get_props();
    gtk_range_set_value (GTK_RANGE (widget), props.to_01 (gui->plugin->get_param_value(param_no)));
    // vscale_value_changed (GTK_HSCALE (widget), (gpointer)this);
}

void vscale_param_control::get()
{
    const parameter_properties &props = get_props();
    float cvalue = props.from_01 (gtk_range_get_value (GTK_RANGE (widget)));
    gui->set_param_value(param_no, cvalue, this);
}

void vscale_param_control::vscale_value_changed(GtkHScale *widget, gpointer value)
{
    vscale_param_control *jhp = (vscale_param_control *)value;
    jhp->get();
}

/******************************** Label ********************************/

GtkWidget *label_param_control::create(plugin_gui *_gui, int _param_no)
{
    gui = _gui, param_no = _param_no;
    string text;
    if (param_no != -1 && !attribs.count("text"))
        text = get_props().name;
    else
        text = attribs["text"];
    widget = gtk_label_new(text.c_str());
    gtk_misc_set_alignment (GTK_MISC (widget), get_float("align-x", 0.5), get_float("align-y", 0.5));
    gtk_widget_set_name(GTK_WIDGET(widget), "Calf-Label");
    return widget;
}

/******************************** Value ********************************/

GtkWidget *value_param_control::create(plugin_gui *_gui, int _param_no)
{
    gui = _gui;
    param_no = _param_no;
    
    widget = gtk_label_new ("");
    if (param_no != -1)
    {
        const parameter_properties &props = get_props();
        int width = get_int("width", 0);
        gtk_label_set_width_chars (GTK_LABEL (widget),
            width ? width : props.get_char_count());
    }
    else
    {
        require_attribute("key");
        require_int_attribute("width");
        param_variable = attribs["key"];
        gtk_label_set_width_chars (GTK_LABEL (widget), get_int("width"));
    }
    gtk_misc_set_alignment (GTK_MISC (widget), get_float("align-x", 0.5), get_float("align-y", 0.5));
    gtk_widget_set_name(GTK_WIDGET(widget), "Calf-Value");
    return widget;
}

void value_param_control::set()
{
    if (param_no == -1)
        return;
    _GUARD_CHANGE_

    const parameter_properties &props = get_props();
    string value = props.to_string(gui->plugin->get_param_value(param_no));
    
    if (value == old_value)
        return;
    old_value = value;
    gtk_label_set_text (GTK_LABEL (widget), value.c_str());    
}

void value_param_control::send_status(const char *key, const char *value)
{
    if (key == param_variable)
    {
        gtk_label_set_text (GTK_LABEL (widget), value);    
    }
}

/******************************** VU Meter ********************************/

GtkWidget *vumeter_param_control::create(plugin_gui *_gui, int _param_no)
{
    gui = _gui, param_no = _param_no;
    // const parameter_properties &props = get_props();
    widget = calf_vumeter_new ();
    CalfVUMeter *vu = CALF_VUMETER(widget);
    gtk_widget_set_name(GTK_WIDGET(widget), "calf-vumeter");
    calf_vumeter_set_mode (vu, (CalfVUMeterMode)get_int("mode", 0));
    vu->vumeter_hold = get_float("hold", 0);
    vu->vumeter_falloff = get_float("falloff", 0.f);
    vu->vumeter_width = get_int("width", 80);
    vu->vumeter_height = get_int("height", 18);
    vu->vumeter_position = get_int("position", 0);
    gtk_widget_set_name(GTK_WIDGET(widget), "Calf-VUMeter");
    return widget;
}

void vumeter_param_control::set()
{
    _GUARD_CHANGE_
    // const parameter_properties &props = get_props();
    calf_vumeter_set_value (CALF_VUMETER (widget), gui->plugin->get_param_value(param_no));
}

// LED

GtkWidget *led_param_control::create(plugin_gui *_gui, int _param_no)
{
    gui = _gui, param_no = _param_no;
    // const parameter_properties &props = get_props();
    widget = calf_led_new ();
    gtk_widget_set_name(GTK_WIDGET(widget), "calf-led");
    CALF_LED(widget)->led_mode = get_int("mode", 0);
    CALF_LED(widget)->size = get_int("size", 1);
    gtk_widget_set_name(GTK_WIDGET(widget), "Calf-LED");
    return widget;
}

void led_param_control::set()
{
    _GUARD_CHANGE_
    // const parameter_properties &props = get_props();
    calf_led_set_value (CALF_LED (widget), gui->plugin->get_param_value(param_no));
}

// tube

GtkWidget *tube_param_control::create(plugin_gui *_gui, int _param_no)
{
    gui = _gui, param_no = _param_no;
    // const parameter_properties &props = get_props();
    GtkWidget *widget = calf_tube_new ();
    CalfTube *tube = CALF_TUBE(widget);
    gtk_widget_set_name(widget, "calf-tube");
    tube->size = get_int("size", 2);
    tube->direction = get_int("direction", 2);
    gtk_widget_set_name(widget, "Calf-Tube");
    return widget;
}

void tube_param_control::set()
{
    _GUARD_CHANGE_
    // const parameter_properties &props = get_props();
    calf_tube_set_value (CALF_TUBE (widget), gui->plugin->get_param_value(param_no));
}

/******************************** Check Box ********************************/

GtkWidget *check_param_control::create(plugin_gui *_gui, int _param_no)
{
    gui = _gui;
    param_no = _param_no;
    
    widget  = gtk_check_button_new ();
    g_signal_connect (GTK_OBJECT (widget), "toggled", G_CALLBACK (check_value_changed), (gpointer)this);
    gtk_widget_set_name(GTK_WIDGET(widget), "Calf-Checkbox");
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

/******************************** Radio Button ********************************/

GtkWidget *radio_param_control::create(plugin_gui *_gui, int _param_no)
{
    gui = _gui;
    param_no = _param_no;
    require_attribute("value");
    value = -1;
    string value_name = attribs["value"];
    const parameter_properties &props = get_props();
    if (props.choices && (value_name < "0" || value_name > "9"))
    {
        for (int i = 0; props.choices[i]; i++)
        {
            if (value_name == props.choices[i])
            {
                value = i + (int)props.min;
                break;
            }
        }
    }
    if (value == -1)
        value = get_int("value");
    
    if (attribs.count("label"))
        widget  = gtk_radio_button_new_with_label (gui->get_radio_group(param_no), attribs["label"].c_str());
    else
        widget  = gtk_radio_button_new_with_label (gui->get_radio_group(param_no), props.choices[value - (int)props.min]);
    gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (widget), FALSE);
        
    gui->set_radio_group(param_no, gtk_radio_button_get_group (GTK_RADIO_BUTTON (widget)));
    g_signal_connect (GTK_OBJECT (widget), "clicked", G_CALLBACK (radio_clicked), (gpointer)this);
    gtk_widget_set_name(GTK_WIDGET(widget), "Calf-RadioButton");
    return widget;
}

void radio_param_control::radio_clicked(GtkRadioButton *widget, gpointer value)
{
    param_control *jhp = (param_control *)value;
    jhp->get();
}

void radio_param_control::get()
{
    // const parameter_properties &props = get_props();
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
        gui->set_param_value(param_no, value, this);
}

void radio_param_control::set()
{
    _GUARD_CHANGE_
    const parameter_properties &props = get_props();
    float pv = gui->plugin->get_param_value(param_no);
    if (fabs(value-pv) < 0.5)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), value == ((int)gui->plugin->get_param_value(param_no) - (int)props.min));
}

/******************************** Spin Button ********************************/

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
    g_signal_connect (GTK_OBJECT (widget), "value-changed", G_CALLBACK (value_changed), (gpointer)this);
    gtk_widget_set_name(GTK_WIDGET(widget), "Calf-SpinButton");
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

/******************************** Button ********************************/

GtkWidget *button_param_control::create(plugin_gui *_gui, int _param_no)
{
    gui = _gui;
    param_no = _param_no;
    widget  = calf_button_new ((gchar*)get_props().name);
    g_signal_connect (GTK_OBJECT (widget), "pressed", G_CALLBACK (button_clicked), (gpointer)this);
    g_signal_connect (GTK_OBJECT (widget), "released", G_CALLBACK (button_clicked), (gpointer)this);
    gtk_widget_set_name(GTK_WIDGET(widget), "Calf-Button");
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
    gui->set_param_value(param_no, gtk_widget_get_state(widget) == GTK_STATE_ACTIVE ? props.max : props.min, this);
}

void button_param_control::set()
{
    _GUARD_CHANGE_
    const parameter_properties &props = get_props();
    if (gui->plugin->get_param_value(param_no) - props.min >= 0.5)
        gtk_button_clicked (GTK_BUTTON (widget));
}

/******************************** Knob ********************************/

GtkWidget *knob_param_control::create(plugin_gui *_gui, int _param_no)
{
    gui = _gui;
    param_no = _param_no;
    const parameter_properties &props = get_props();
    widget = calf_knob_new();
    gtk_widget_set_name(GTK_WIDGET(widget), "Calf-Knob");
    CalfKnob * knob = CALF_KNOB(widget);
    
    float increment = props.get_increment();
    gtk_range_get_adjustment(GTK_RANGE(widget))->step_increment = increment;
    
    knob->default_value = props.to_01(props.def_value);
    knob->type = get_int("type");
    calf_knob_set_size(knob, get_int("size", 2));
    
    // set pixbuf
    char imgname[16];
    sprintf(imgname, "knob_%d", get_int("size", 2));
    calf_knob_set_pixbuf(knob, gui->window->get_environment()->get_image_factory()->get(imgname));
    
    //char ticks[128];
    std::ostringstream ticks_;
    //std::string str = ticks.str();
    double min = double(props.min);
    double max = double(props.max);
    switch (knob->type) {
        default:
        case 0: ticks_ << min << " " << max; break;
        case 1: ticks_ << min << " " << props.from_01(0.5) << " " << max; break;
        case 2: ticks_ << min << " " << max; break;
        case 3: ticks_ << min << " " << props.from_01(0.25) << " " << props.from_01(0.5) << " " << props.from_01(0.75) << " " << max; break;
    }
    std::string ticks = ticks_.str();
    vector<double> t = get_vector("ticks", ticks);
    std::sort(t.begin(), t.end());
    for (unsigned int i = 0; i < t.size(); i++)
        t[i] = props.to_01(t[i]);
    knob->ticks = t;
    g_signal_connect(GTK_OBJECT(widget), "value-changed", G_CALLBACK(knob_value_changed), (gpointer)this);
    return widget;
}

void knob_param_control::get()
{
    const parameter_properties &props = get_props();
    float value = props.from_01(gtk_range_get_value(GTK_RANGE(widget)));
    gui->set_param_value(param_no, value, this);
}

void knob_param_control::set()
{
    _GUARD_CHANGE_
    const parameter_properties &props = get_props();
    gtk_range_set_value(GTK_RANGE(widget), props.to_01 (gui->plugin->get_param_value(param_no)));
}

void knob_param_control::knob_value_changed(GtkWidget *widget, gpointer value)
{
    param_control *jhp = (param_control *)value;
    jhp->get();
}

/******************************** Toggle Button ********************************/

GtkWidget *toggle_param_control::create(plugin_gui *_gui, int _param_no)
{
    gui = _gui;
    param_no = _param_no;
    widget  = calf_toggle_new ();
    CalfToggle * toggle = CALF_TOGGLE(widget);
    calf_toggle_set_size(toggle, get_int("size", 2));
    
    // set pixbuf
    image_factory *images = gui->window->get_environment()->get_image_factory();
    char imgname[64];
    if (attribs.count("icon") != 0) {
        sprintf(imgname, "toggle_%d_%s", get_int("size", 2), attribs["icon"].c_str());
        if (!images->available(imgname))
            sprintf(imgname, "toggle_%d", get_int("size", 2));
    } else
        sprintf(imgname, "toggle_%d", get_int("size", 2));
    calf_toggle_set_pixbuf(toggle, images->get(imgname));
    
    g_signal_connect (GTK_OBJECT (widget), "value-changed", G_CALLBACK (toggle_value_changed), (gpointer)this);
    //gtk_widget_set_name(GTK_WIDGET(widget), "Calf-ToggleButton");
    return widget;
}

void toggle_param_control::get()
{
    const parameter_properties &props = get_props();
    float value = props.from_01(gtk_range_get_value(GTK_RANGE(widget)));
    gui->set_param_value(param_no, value, this);
}

void toggle_param_control::set()
{
    _GUARD_CHANGE_
    const parameter_properties &props = get_props();
    float value = gui->plugin->get_param_value(param_no);
    gtk_range_set_value(GTK_RANGE(widget), props.to_01(value));
}

void toggle_param_control::toggle_value_changed(GtkWidget *widget, gpointer value)
{
    param_control *jhp = (param_control *)value;
    jhp->get();
}

/******************************** Tap Button ********************************/

GtkWidget *tap_button_param_control::create(plugin_gui *_gui, int _param_no)
{
    gui       = _gui;
    param_no  = _param_no;
    last_time = 0;
    avg_value = 0;
    value     = 0;
    timer     = 0;
    widget    = calf_tap_button_new ();
    // set pixbuf
    calf_tap_button_set_pixbufs(CALF_TAP_BUTTON(widget),
        gui->window->get_environment()->get_image_factory()->get("tap_inactive"),
        gui->window->get_environment()->get_image_factory()->get("tap_prelight"),
        gui->window->get_environment()->get_image_factory()->get("tap_active"));
    //CALF_TAP(widget)->size = get_int("size", 2);
    g_signal_connect (GTK_OBJECT (widget), "button-press-event", G_CALLBACK (tap_button_pressed), (gpointer)this);
    g_signal_connect (GTK_OBJECT (widget), "released", G_CALLBACK (tap_button_released), (gpointer)this);
    g_signal_connect (GTK_OBJECT (widget), "leave", G_CALLBACK (tap_button_released), (gpointer)this);
    gtk_widget_set_name(GTK_WIDGET(widget), "Calf-TapButton");
    return widget;
}

void tap_button_param_control::get()
{
    gui->set_param_value(param_no, value, this);
}

void tap_button_param_control::set()
{
    _GUARD_CHANGE_
}

gboolean tap_button_param_control::tap_button_pressed(GtkWidget *widget, GdkEventButton *event, gpointer value)
{
    tap_button_param_control *ctl = (tap_button_param_control *)value;
    CalfTapButton *tap = CALF_TAP_BUTTON(widget);
    
    guint time = 0;
    if (event->type == GDK_BUTTON_PRESS and event->button == 1) {
        time = event->time;
        tap->state = 2;
        if(ctl->last_time) {
            if(ctl->avg_value)
                ctl->avg_value = (ctl->avg_value * 3 + (time - ctl->last_time)) / 4.f;
            else
                ctl->avg_value = time - ctl->last_time;
            ctl->value = 60.f / (float)(ctl->avg_value / 1000.f);
            
            if (ctl->value > 30 and ctl->value < 300)
                ctl->get();
        }
        ctl->last_time = time;
        if (ctl->timer)
            gtk_timeout_remove(ctl->timer);
        ctl->timer = gtk_timeout_add(2000, (GtkFunction)tap_button_stop_waiting, (gpointer)ctl);
        gtk_widget_queue_draw(widget);
    }
    return FALSE;
}
void tap_button_param_control::tap_button_stop_waiting(gpointer data)
{
    tap_button_param_control *ctl = (tap_button_param_control *)data;
    if (ctl->timer) {
        ctl->avg_value = 0;
        ctl->last_time = 0;
        CALF_TAP_BUTTON(ctl->widget)->state = 0;
        gtk_widget_queue_draw(ctl->widget);
        gtk_timeout_remove(ctl->timer);
        ctl->timer = 0;
        gtk_widget_queue_draw(ctl->widget);
    }
}
gboolean tap_button_param_control::tap_button_released(GtkWidget *widget, gpointer value)
{
    tap_button_param_control *ctl = (tap_button_param_control *)value;
    CalfTapButton *tap = CALF_TAP_BUTTON(widget);
    tap->state = ctl->last_time ? 1 : 0;
    gtk_widget_queue_draw(widget);
    return FALSE;
}

/******************************** Keyboard ********************************/

GtkWidget *keyboard_param_control::create(plugin_gui *_gui, int _param_no)
{
    gui = _gui;
    param_no = _param_no;
    // const parameter_properties &props = get_props();
    
    widget = calf_keyboard_new();
    kb = CALF_KEYBOARD(widget);
    kb->nkeys = get_int("octaves", 4) * 7 + 1;
    kb->sink = new CalfKeyboard::EventAdapter;
    gtk_widget_set_name(GTK_WIDGET(widget), "Calf-Keyboard");
    return widget;
}

/******************************** Curve ********************************/

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
    gtk_widget_set_name(GTK_WIDGET(widget), "Calf-Curve");
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

/******************************** Meter Scale ********************************/

GtkWidget *meter_scale_param_control::create(plugin_gui *_gui, int _param_no)
{
    gui = _gui;
    param_no = _param_no;
    widget  = calf_meter_scale_new ();
    CalfMeterScale *ms = CALF_METER_SCALE(widget);
    gtk_widget_set_name(widget, "Calf-MeterScale");
    string str   = "0 0.5 1";
    ms->marker   = get_vector("marker", str);
    ms->mode     = (CalfVUMeterMode)get_int("mode", 0);
    ms->position = get_int("position", 0);
    ms->dots     = get_int("dots", 0);
    return widget;
}

/******************************** Entry ********************************/

GtkWidget *entry_param_control::create(plugin_gui *_gui, int _param_no)
{
    gui = _gui;
    param_no = _param_no;
    require_attribute("key");
    
    widget = gtk_entry_new();
    entry = GTK_ENTRY(widget);
    g_signal_connect(GTK_OBJECT(widget), "changed", G_CALLBACK(entry_value_changed), (gpointer)this);
    gtk_editable_set_editable(GTK_EDITABLE(entry), get_int("editable", 1));
    gtk_widget_set_name(GTK_WIDGET(widget), "Calf-Entry");
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

/******************************** File Chooser ********************************/

GtkWidget *filechooser_param_control::create(plugin_gui *_gui, int _param_no)
{
    gui = _gui;
    param_no = _param_no;
    require_attribute("key");
    require_attribute("title");
    
    widget = gtk_file_chooser_button_new(attribs["title"].c_str(), GTK_FILE_CHOOSER_ACTION_OPEN);
    filechooser = GTK_FILE_CHOOSER_BUTTON(widget);
    // XXXKF this is GTK+ 2.12 function, does any replacement exist?
    // MS: switched from g_signal_connect to g_signal_connect for better emission of signals
    g_signal_connect(GTK_OBJECT(widget), "file-set", G_CALLBACK(filechooser_value_changed), (gpointer)this);
    if (attribs.count("width"))
        gtk_widget_set_size_request (widget, get_int("width", 200), -1);
    if (attribs.count("width_chars")) {
        gtk_file_chooser_button_set_width_chars (filechooser, get_int("width_chars"));
        gtk_widget_set_name(GTK_WIDGET(widget), "Calf-FileButton");
    }
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

/******************************** Line Graph ********************************/

void line_graph_param_control::on_idle()
{
    if (get_int("refresh", 0))
        set();
}

static float to_x_pos(float freq)
{
    return log(freq / 20.0) / log(1000);
}

static float from_x_pos(float pos)
{
    float a = pos * 3.0;
    float b = powf(10.0, a);
    float c = b * 20.0;
    return c;
}

static float to_y_pos(CalfLineGraph *lg, float gain)
{
                //log(gain) * (1.0 / log(32));
    return 0.5 - dB_grid(gain, 128 * lg->zoom, lg->offset) / 2.0;
}

static float from_y_pos(CalfLineGraph *lg, float pos)
{
    float gain = powf(128.0 * lg->zoom, (0.5 - pos) * 2.0 - lg->offset);
    return gain;
}

GtkWidget *line_graph_param_control::create(plugin_gui *a_gui, int a_param_no)
{
    gui             = a_gui;
    param_no        = a_param_no;
    //last_generation = -1;
    
    widget                     = calf_line_graph_new ();
    
    CalfLineGraph *clg         = CALF_LINE_GRAPH(widget);
    widget->requisition.width  = get_int("width", 40);
    widget->requisition.height = get_int("height", 40);
    
    calf_line_graph_set_square(clg, get_int("square", 0));
    
    clg->source                = gui->plugin->get_line_graph_iface();
    clg->source_id             = param_no;
    clg->fade                  = get_float("fade", 1.0);
    clg->mode                  = get_int("mode", 0);
    clg->use_crosshairs        = get_int("crosshairs", 0);
    clg->freqhandles           = get_int("freqhandles", 0);
    clg->enforce_handle_order  = get_int("enforce-handle-order", 0);
    clg->min_handle_distance   = get_float("min-handle-distance", 0.01);
    
    const string &zoom_name = attribs["zoom"];
    if (zoom_name != "")
        clg->param_zoom = gui->get_param_no_by_name(zoom_name);
    
    const string &offset_name = attribs["offset"];
    if (offset_name != "")
        clg->param_offset = gui->get_param_no_by_name(offset_name);
        
    if (clg->freqhandles > 0)
    {
        for(int i = 0; i < clg->freqhandles; i++)
        {
            FreqHandle *handle = &clg->freq_handles[i];

            stringstream handle_x_attribute;
            handle_x_attribute << "handle" << i + 1 << "-x";
            const string &param_x_name = attribs[handle_x_attribute.str()];
            if(param_x_name == "")
                break;

            int param_x_no = gui->get_param_no_by_name(param_x_name);
            const parameter_properties &handle_x_props = *gui->plugin->get_metadata_iface()->get_param_props(param_x_no);
            handle->dimensions = 1;
            handle->param_x_no = param_x_no;
            handle->value_x = to_x_pos(gui->plugin->get_param_value(param_x_no));
            handle->default_value_x = to_x_pos(handle_x_props.def_value);

            stringstream handle_y_attribute;
            handle_y_attribute << "handle" << i + 1 << "-y";
            const string &param_y_name = attribs[handle_y_attribute.str()];
            if(param_y_name != "") {
                int param_y_no = gui->get_param_no_by_name(param_y_name);
                const parameter_properties &handle_y_props = *gui->plugin->get_metadata_iface()->get_param_props(param_y_no);
                handle->dimensions = 2;
                handle->param_y_no = param_y_no;
                handle->value_y = to_y_pos(clg, gui->plugin->get_param_value(param_y_no));
                handle->default_value_y = to_y_pos(clg, handle_y_props.def_value);
            } else {
                handle->param_y_no = -1;
            }

            stringstream handle_z_attribute;
            handle_z_attribute << "handle" << i + 1 << "-z";
            const string &param_z_name = attribs[handle_z_attribute.str()];
            if(param_z_name != "") {
                int param_z_no = gui->get_param_no_by_name(param_z_name);
                const parameter_properties &handle_z_props = *gui->plugin->get_metadata_iface()->get_param_props(param_z_no);
                handle->param_z_no = param_z_no;
                handle->value_z = handle_z_props.to_01(gui->plugin->get_param_value(param_z_no));
                handle->default_value_z = handle_z_props.to_01(handle_z_props.def_value);
                handle->props_z = handle_z_props;
            } else {
                handle->param_z_no = -1;
            }

            stringstream label_attribute;
            label_attribute << "label" << i + 1;
            string label = attribs[label_attribute.str()];
            if (!label.empty()) {
                handle->label = strdup(label.c_str());
            }
            
            stringstream active_attribute;
            active_attribute << "active" << i + 1;
            const string &active_name = attribs[active_attribute.str()];
            if (active_name != "") {
                handle->param_active_no = gui->get_param_no_by_name(active_name);
            } else {
                handle->param_active_no = -1;
            }
            
            stringstream style_attribute;
            style_attribute << "style" << i + 1;
            const string style = style_attribute.str();
            clg->freq_handles[i].style = get_int(style.c_str(), 0);
            if(clg->freq_handles[i].style == 1 or clg->freq_handles[i].style == 4) {
                clg->freq_handles[i].dimensions = 1;
            }
            handle->data = (gpointer) this;
        }
        g_signal_connect(G_OBJECT(widget), "freqhandle-changed", G_CALLBACK(freqhandle_value_changed), this);
    }

    gtk_widget_set_name(GTK_WIDGET(widget), "Calf-LineGraph");
    return widget;
}

void line_graph_param_control::get()
{
    GtkWidget *tw = gtk_widget_get_toplevel(widget);
    CalfLineGraph *clg = CALF_LINE_GRAPH(widget);

    if (tw && GTK_WIDGET_TOPLEVEL(tw) && widget->window)
    {
        int ws = gdk_window_get_state(widget->window);
        if (ws & (GDK_WINDOW_STATE_WITHDRAWN | GDK_WINDOW_STATE_ICONIFIED))
            return;

        if(clg->handle_grabbed >= 0) {
            FreqHandle *handle = &clg->freq_handles[clg->handle_grabbed];
            if(handle->dimensions >= 2) {
                float value_y = from_y_pos(clg, handle->value_y);
                gui->set_param_value(handle->param_y_no, value_y, this);
            }

            float value_x = from_x_pos(handle->value_x);
            gui->set_param_value(handle->param_x_no, value_x, this);
        } else if(clg->handle_hovered >= 0) {
            FreqHandle *handle = &clg->freq_handles[clg->handle_hovered];

            if(handle->param_z_no > -1) {
                const parameter_properties &handle_z_props = *gui->plugin->get_metadata_iface()->get_param_props(handle->param_z_no);
                float value_z = handle_z_props.from_01(handle->value_z);
                gui->set_param_value(handle->param_z_no, value_z, this);
            }
        }
    }
}

void line_graph_param_control::set()
{
    _GUARD_CHANGE_
    GtkWidget *tw = gtk_widget_get_toplevel(widget);
    CalfLineGraph *clg = CALF_LINE_GRAPH(widget);
    if (tw && GTK_WIDGET_TOPLEVEL(tw) && widget->window)
    {
        bool force = false;
        int ws = gdk_window_get_state(widget->window);
        if (ws & (GDK_WINDOW_STATE_WITHDRAWN | GDK_WINDOW_STATE_ICONIFIED))
            return;
        
        if (clg->param_zoom >= 0) {
            float _z = gui->plugin->get_param_value(clg->param_zoom);
            if (_z != clg->zoom) {
                force = true;
                clg->zoom = _z;
                clg->force_redraw = true;
            }
        }
        
        if (clg->param_offset >= 0) {
            float _z = gui->plugin->get_param_value(clg->param_offset);
            if (_z != clg->offset) {
                force = true;
                clg->offset = _z;
                clg->force_redraw = true;
            }
        }
        
        for (int i = 0; i < clg->freqhandles; i++) {
            FreqHandle *handle = &clg->freq_handles[i];

            if (handle->param_x_no >= 0)
            {
                float value_x = gui->plugin->get_param_value(handle->param_x_no);
                handle->value_x = to_x_pos(value_x);
                if (dsp::_sanitize(handle->value_x - handle->last_value_x)) {
                    clg->handle_redraw = 1;
                }
                handle->last_value_x = handle->value_x;
                if(handle->dimensions >= 2 && handle->param_y_no >= 0) {
                    float value_y = gui->plugin->get_param_value(handle->param_y_no);
                    handle->value_y = to_y_pos(clg, value_y);
                    if (dsp::_sanitize(handle->value_y - handle->last_value_y)) {
                        clg->handle_redraw = 1;
                    }
                    handle->last_value_y = handle->value_y;
                }
            }

            if(handle->param_z_no >= 0) {
                const parameter_properties &handle_z_props = *gui->plugin->get_metadata_iface()->get_param_props(handle->param_z_no);
                float value_z = gui->plugin->get_param_value(handle->param_z_no);
                handle->value_z = handle_z_props.to_01(value_z);
                if (dsp::_sanitize(handle->value_z - handle->last_value_z)) {
                    clg->handle_redraw = 1;
                }
                handle->last_value_z = handle->value_z;
            }
            bool _a = handle->active;
            if(handle->param_active_no >= 0) {
                handle->active = bool(gui->plugin->get_param_value(handle->param_active_no));
            } else {
                handle->active = true;
            }
            if (handle->active != _a) {
                force = true;
                clg->handle_redraw = true;
            }
        }
        calf_line_graph_expose_request(widget, force);
    }
}

void line_graph_param_control::freqhandle_value_changed(GtkWidget *widget, gpointer p)
{
    assert(p!=NULL);
    FreqHandle *handle = (FreqHandle *)p;
    param_control *jhp = (param_control *)handle->data;
    jhp->get();
}


line_graph_param_control::~line_graph_param_control()
{
    
}

/******************************** Phase Graph ********************************/

void phase_graph_param_control::on_idle()
{
    if (get_int("refresh", 0))
        set();
}

GtkWidget *phase_graph_param_control::create(plugin_gui *_gui, int _param_no)
{
    gui = _gui;
    param_no = _param_no;
    widget = calf_phase_graph_new ();
    CalfPhaseGraph *clg = CALF_PHASE_GRAPH(widget);
    widget->requisition.width = get_int("size", 40);
    widget->requisition.height = get_int("size", 40);
    clg->source = gui->plugin->get_phase_graph_iface();
    clg->source_id = param_no;
    gtk_widget_set_name(GTK_WIDGET(widget), "Calf-PhaseGraph");
    return widget;
}

void phase_graph_param_control::set()
{
    _GUARD_CHANGE_
    GtkWidget *tw = gtk_widget_get_toplevel(widget);
    if (tw && GTK_WIDGET_TOPLEVEL(tw) && widget->window) {
        gtk_widget_queue_draw(widget);
    }
}

phase_graph_param_control::~phase_graph_param_control()
{
}


/******************************** Tuner ********************************/

void tuner_param_control::on_idle()
{
    if (get_int("refresh", 0))
        set();
}

GtkWidget *tuner_param_control::create(plugin_gui *_gui, int _param_no)
{
    gui = _gui;
    param_no = _param_no;
    widget = calf_tuner_new ();
    //CalfTuner *tuner = CALF_TUNER(widget);
    widget->requisition.width = get_int("width", 40);
    widget->requisition.height = get_int("height", 40);
    gtk_widget_set_name(GTK_WIDGET(widget), "Calf-Tuner");
    
    const string &cents_name = attribs["param_cents"];
    if (cents_name != "")
        cents_no = gui->get_param_no_by_name(cents_name);
    else
        cents_no = 0;
    return widget;
}

void tuner_param_control::set()
{
    _GUARD_CHANGE_
    GtkWidget *tw = gtk_widget_get_toplevel(widget);
    CalfTuner *tuner = CALF_TUNER(widget);
    tuner->note = gui->plugin->get_param_value(param_no);
    tuner->cents = gui->plugin->get_param_value(cents_no);
    if (tw && GTK_WIDGET_TOPLEVEL(tw) && widget->window) {
        gtk_widget_queue_draw(widget);
    }
}

tuner_param_control::~tuner_param_control()
{
}

/******************************** Pattern ********************************/

GtkWidget *pattern_param_control::create(plugin_gui *_gui, int _param_no)
{
    gui = _gui;
    param_no = _param_no;
    widget = calf_pattern_new ();
    widget->requisition.width = get_int("width", 300);
    widget->requisition.height = get_int("height", 60);
    const string &beats_name = attribs["beats"];
    if (beats_name != "") {
        param_beats = gui->get_param_no_by_name(beats_name);
        gui->add_param_ctl(param_beats, this);
    } else param_beats = -1;
    const string &bars_name = attribs["bars"];
    if (bars_name != "") {
        param_bars = gui->get_param_no_by_name(bars_name);
        gui->add_param_ctl(param_bars, this);
    } else param_bars = -1;
    gtk_widget_set_name(GTK_WIDGET(widget), "Calf-Pattern");
    g_signal_connect(GTK_OBJECT(widget), "handle-changed", (GCallback)on_handle_changed, this);
    return widget;
}

void pattern_param_control::set()
{
    _GUARD_CHANGE_
    CalfPattern *p = CALF_PATTERN(widget);

    int b;
    if (param_beats >= 0) {
        b = gui->plugin->get_param_value(param_beats);
        if (b != p->beats) {
            p->beats = b;
            p->force_redraw = true;
            gtk_widget_queue_draw(widget);
        }
    }
    if (param_bars >= 0) {
        b = gui->plugin->get_param_value(param_bars);
        if (b != p->bars) {
            p->bars = b;
            p->force_redraw = true;
            gtk_widget_queue_draw(widget);
        }
    }
}

void pattern_param_control::send_configure(const char *key, const char *value)
{
    string orig_key = attribs["key"];
    if (orig_key != key)
        return;

    CalfPattern *p = CALF_PATTERN(widget);
    stringstream ss(value);
    _GUARD_CHANGE_
    for (int i = 0; i < p->bars; i++) {
        for (int j = 0; j < p->beats; j++) {
            ss >> p->values[i][j];
        }
    }
    p->force_redraw = true;
    gtk_widget_queue_draw(widget);
}

void pattern_param_control::on_handle_changed(CalfPattern *widget, calf_pattern_handle *handle, pattern_param_control *pThis)
{
    CalfPattern *p = CALF_PATTERN(widget);
    stringstream ss;
    for (int i = 0; i < p->bars; i++) {
        for (int j = 0; j < p->beats; j++) {
            ss << p->values[i][j] << " ";
        }
    }
    assert(pThis);
    string key = pThis->attribs["key"];
    const char *error_or_null = pThis->gui->plugin->configure(key.c_str(), ss.str().c_str());
    if (error_or_null)
        g_warning("Unexpected error: %s", error_or_null);
}

/******************************** List View ********************************/

GtkWidget *listview_param_control::create(plugin_gui *_gui, int _param_no)
{
    gui = _gui;
    param_no = _param_no;
    
    string key = attribs["key"];
    tmif = gui->plugin->get_metadata_iface()->get_table_metadata_iface(key.c_str());
    if (!tmif)
    {
        g_error("Missing table_metadata_iface for variable '%s'", key.c_str());
        return NULL;
    }
    positions.clear();
    const table_column_info *tci = tmif->get_table_columns();
    assert(tci);
    cols = 0;
    while (tci[cols].name != NULL)
        cols++;
    
    GType *p = new GType[cols];
    for (int i = 0; i < cols; i++)
        p[i] = G_TYPE_STRING;
    lstore = gtk_list_store_newv(cols, p);
    if (tmif->get_table_rows() != 0)
        set_rows(tmif->get_table_rows());
    widget = gtk_tree_view_new_with_model(GTK_TREE_MODEL(lstore));
    delete []p;
    tree = GTK_TREE_VIEW (widget);
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
        g_signal_connect (GTK_OBJECT (cr), "edited", G_CALLBACK (on_edited), (gpointer)this);
        g_signal_connect (GTK_OBJECT (cr), "editing-canceled", G_CALLBACK (on_editing_canceled), (gpointer)this);
        gtk_tree_view_insert_column_with_attributes(tree, i, tci[i].name, cr, "text", i, NULL);
    }
    gtk_tree_view_set_headers_visible(tree, TRUE);
    gtk_widget_set_name(GTK_WIDGET(widget), "Calf-ListView");
    return widget;
}

void listview_param_control::set_rows(unsigned int needed_rows)
{
    while(positions.size() < needed_rows)
    {
        GtkTreeIter iter;
        gtk_list_store_insert(lstore, &iter, positions.size());
        for (int j = 0; j < cols; j++)
        {
            gtk_list_store_set(lstore, &iter, j, "", -1);
        }
        positions.push_back(iter);
    }
}

void listview_param_control::send_configure(const char *key, const char *value)
{
    string orig_key = attribs["key"] + ":";
    bool is_rows = false;
    int row = -1, col = -1;
    if (parse_table_key(key, orig_key.c_str(), is_rows, row, col))
    {
        if (is_rows && tmif->get_table_rows() == 0)
        {
            int rows = atoi(value);
            set_rows(rows);
            return;
        }
        else
        if (row != -1 && col != -1)
        {
            int max_rows = tmif->get_table_rows();
            if (col < 0 || col >= cols)
            {
                g_warning("Invalid column %d in key %s", col, key);
                return;
            }
            if (max_rows && (row < 0 || row >= max_rows))
            {
                g_warning("Invalid row %d in key %s, this is a fixed table with row count = %d", row, key, max_rows);
                return;
            }
            
            if (row >= (int)positions.size())
                set_rows(row + 1);
            
            gtk_list_store_set(lstore, &positions[row], col, value, -1);
            return;
        }
    }
}

void listview_param_control::on_edited(GtkCellRenderer *renderer, gchar *path, gchar *new_text, listview_param_control *pThis)
{
    const table_column_info *tci = pThis->tmif->get_table_columns();
    int column = ((table_column_info *)g_object_get_data(G_OBJECT(renderer), "column")) - tci;
    string key = pThis->attribs["key"] + ":" + i2s(atoi(path)) + "," + i2s(column);
    string error;
    const char *error_or_null = pThis->gui->plugin->configure(key.c_str(), new_text);
    if (error_or_null)
        error = error_or_null;
    
    if (error.empty()) {
        pThis->send_configure(key.c_str(), new_text);
        gtk_widget_grab_focus(pThis->widget);
        GtkTreePath *gpath = gtk_tree_path_new_from_string (path);
        gtk_tree_view_set_cursor_on_cell (GTK_TREE_VIEW (pThis->widget), gpath, NULL, NULL, FALSE);
        gtk_tree_path_free (gpath);
    }
    else
    {
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(pThis->gui->window->toplevel), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, 
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

/******************************** GtkNotebook control ********************************/

GtkWidget *notebook_param_control::create(plugin_gui *_gui, int _param_no)
{
    gui = _gui;
    param_no = _param_no;
    //const parameter_properties &props = get_props();
    if (param_no < 0)
        page = 0;
    else
        page = gui->plugin->get_param_value(param_no);
    GtkWidget *nb = calf_notebook_new();
    widget = GTK_WIDGET(nb);
    calf_notebook_set_pixbuf(CALF_NOTEBOOK(nb),
        gui->window->get_environment()->get_image_factory()->get("notebook_screw"));
    gtk_widget_set_name(widget, "Calf-Notebook");
    gtk_notebook_set_current_page(GTK_NOTEBOOK(widget), page);
    return nb;
}
void notebook_param_control::created()
{
    hook_params();
    gtk_widget_show_all(widget);
    gtk_notebook_set_current_page(GTK_NOTEBOOK(widget), page);
    g_signal_connect (GTK_OBJECT (widget), "switch-page", G_CALLBACK (notebook_page_changed), (gpointer)this);
    //set();
}
void notebook_param_control::get()
{
    if (param_no >= 0)
        gui->set_param_value(param_no, page, this);
}
void notebook_param_control::set()
{
    if (param_no < 0)
        return;
    _GUARD_CHANGE_
    page = (gint)gui->plugin->get_param_value(param_no);
    gtk_notebook_set_current_page(GTK_NOTEBOOK(widget), page);
}
void notebook_param_control::add(control_base *base)
{
    gtk_notebook_append_page(GTK_NOTEBOOK(widget), base->widget, gtk_label_new_with_mnemonic(base->attribs["page"].c_str()));
}
void notebook_param_control::notebook_page_changed(GtkWidget *widget, GtkWidget *page, guint id, gpointer user)
{
    notebook_param_control *jhp = (notebook_param_control *)user;
    jhp->page = (int)id;
    jhp->get();
}

/******************************** GtkTable container ********************************/

GtkWidget *table_container::create(plugin_gui *_gui)
{
    require_int_attribute("rows");
    require_int_attribute("cols");
    int homog = get_int("homogeneous", 0);
    int sx = get_int("spacing-x", 2);
    int sy = get_int("spacing-y", 2);
    GtkWidget *table = gtk_table_new(get_int("rows", 1), get_int("cols", 1), false);
    if(homog > 0) {
        gtk_table_set_homogeneous(GTK_TABLE(table), TRUE);
    }
    gtk_table_set_col_spacings(GTK_TABLE(table), sx);
    gtk_table_set_row_spacings(GTK_TABLE(table), sy);
    widget = table;
    gtk_widget_set_name(GTK_WIDGET(table), "Calf-Table");
    return table;
}

void table_container::add(control_base *base)
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
    gtk_table_attach(GTK_TABLE(widget), base->widget, x, x + w, y, y + h, (GtkAttachOptions)fillx, (GtkAttachOptions)filly, padx, pady);
} 

/******************************** alignment container ********************************/

GtkWidget *alignment_container::create(plugin_gui *_gui)
{
    widget = gtk_alignment_new(get_float("align-x", 0.5), get_float("align-y", 0.5), get_float("scale-x", 0), get_float("scale-y", 0));
    gtk_widget_set_name(widget, "Calf-Align");
    return widget;
}

/******************************** GtkFrame container ********************************/

GtkWidget *frame_container::create(plugin_gui *_gui)
{
    widget = calf_frame_new(attribs["label"].c_str());
    gtk_widget_set_name(widget, "Calf-Frame");
    return widget;
}

/******************************** GtkBox type of containers ********************************/

void box_container::add(control_base *base)
{
    gtk_container_add_with_properties(GTK_CONTAINER(widget), base->widget, "expand", get_int("expand", 1), "fill", get_int("fill", 1), NULL);
}

/******************************** GtkHBox container ********************************/

GtkWidget *hbox_container::create(plugin_gui *_gui)
{
    widget = gtk_hbox_new(get_int("homogeneous") >= 1, get_int("spacing", 2));
    gtk_widget_set_name(widget, "Calf-HBox");
    return widget;
}

/******************************** GtkVBox container ********************************/

GtkWidget *vbox_container::create(plugin_gui *_gui)
{
    widget = gtk_vbox_new(get_int("homogeneous") >= 1, get_int("spacing", 2));
    gtk_widget_set_name(widget, "Calf-VBox");
    return widget;
}

/******************************** GtkNotebook container ********************************/

GtkWidget *scrolled_container::create(plugin_gui *_gui)
{
    GtkAdjustment *horiz = NULL, *vert = NULL;
    int width = get_int("width", 0), height = get_int("height", 0);
    if (width)
        horiz = GTK_ADJUSTMENT(gtk_adjustment_new(get_int("x", 0), 0, width, get_int("step-x", 1), get_int("page-x", width / 10), 100));
    if (height)
        vert = GTK_ADJUSTMENT(gtk_adjustment_new(get_int("y", 0), 0, width, get_int("step-y", 1), get_int("page-y", height / 10), 10));
    widget = gtk_scrolled_window_new(horiz, vert);
    gtk_widget_set_size_request(widget, get_int("req-x", -1), get_int("req-y", -1));
    gtk_widget_set_name(widget, "Calf-ScrolledWindow");
    return widget;
}

void scrolled_container::add(control_base *base)
{
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(widget), base->widget);
}
