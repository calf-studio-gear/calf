/* Calf DSP Library
 * Custom controls (line graph, knob).
 * Copyright (C) 2007-2015 Krzysztof Foltman, Torben Hohn, Markus Schmidt
 * and others
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
 
#include <calf/ctl_buttons.h>

using namespace calf_plugins;
using namespace dsp;


///////////////////////////////////////// toggle ///////////////////////////////////////////////


static gboolean
calf_toggle_expose (GtkWidget *widget, GdkEventExpose *event)
{
    g_assert(CALF_IS_TOGGLE(widget));
    CalfToggle *self = CALF_TOGGLE(widget);
    if (!self->toggle_image)
        return FALSE;
    float off = floor(.5 + gtk_range_get_value(GTK_RANGE(widget)));
    float pw  = gdk_pixbuf_get_width(self->toggle_image);
    float ph  = gdk_pixbuf_get_height(self->toggle_image);
    float wcx = widget->allocation.x + widget->allocation.width / 2;
    float wcy = widget->allocation.y + widget->allocation.height / 2;
    float pcx = pw / 2;
    float pcy = ph / 4;
    float sy = off * ph / 2;
    float x = wcx - pcx;
    float y = wcy - pcy;
    gdk_draw_pixbuf(GDK_DRAWABLE(widget->window), widget->style->fg_gc[0],
                    self->toggle_image, 0, sy, x, y, pw, ph / 2, GDK_RGB_DITHER_NORMAL, 0, 0);
    return TRUE;
}

static void
calf_toggle_size_request (GtkWidget *widget,
                           GtkRequisition *requisition)
{
    g_assert(CALF_IS_TOGGLE(widget));
    requisition->width  = widget->style->xthickness;
    requisition->height = widget->style->ythickness;
}

static gboolean
calf_toggle_button_press (GtkWidget *widget, GdkEventButton *event)
{
    g_assert(CALF_IS_TOGGLE(widget));
    GtkAdjustment *adj = gtk_range_get_adjustment(GTK_RANGE(widget));
    if (gtk_range_get_value(GTK_RANGE(widget)) == adj->lower)
    {
        gtk_range_set_value(GTK_RANGE(widget), adj->upper);
    } else {
        gtk_range_set_value(GTK_RANGE(widget), adj->lower);
    }
    return TRUE;
}

static gboolean
calf_toggle_key_press (GtkWidget *widget, GdkEventKey *event)
{
    switch(event->keyval)
    {
        case GDK_Return:
        case GDK_KP_Enter:
        case GDK_space:
            return calf_toggle_button_press(widget, NULL);
    }
    return FALSE;
}

static void
calf_toggle_class_init (CalfToggleClass *klass)
{
    // GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);
    widget_class->expose_event = calf_toggle_expose;
    widget_class->size_request = calf_toggle_size_request;
    widget_class->button_press_event = calf_toggle_button_press;
    widget_class->key_press_event = calf_toggle_key_press;
}

static void
calf_toggle_init (CalfToggle *self)
{
    GtkWidget *widget = GTK_WIDGET(self);
    GTK_WIDGET_SET_FLAGS (GTK_WIDGET(self), GTK_CAN_FOCUS);
    widget->requisition.width = 30;
    widget->requisition.height = 20;
    self->size = 1;
}

void
calf_toggle_set_size (CalfToggle *self, int size)
{
    char name[128];
    GtkWidget *widget = GTK_WIDGET(self);
    self->size = size;
    sprintf(name, "%s_%d\n", gtk_widget_get_name(widget), size);
    gtk_widget_set_name(widget, name);
    gtk_widget_queue_resize(widget);
}
void
calf_toggle_set_pixbuf (CalfToggle *self, GdkPixbuf *pixbuf)
{
    GtkWidget *widget = GTK_WIDGET(self);
    self->toggle_image = pixbuf;
    gtk_widget_queue_resize(widget);
}

GtkWidget *
calf_toggle_new()
{
    GtkAdjustment *adj = (GtkAdjustment *)gtk_adjustment_new(0, 0, 1, 1, 0, 0);
    return calf_toggle_new_with_adjustment(adj);
}

static gboolean calf_toggle_value_changed(gpointer obj)
{
    GtkWidget *widget = (GtkWidget *)obj;
    CalfToggle *self = CALF_TOGGLE(widget);
    float sx = self->size ? self->size : 1.f / 3.f * 2.f;
    float sy = self->size ? self->size : 1;
    gtk_widget_queue_draw_area(widget,
                               widget->allocation.x - sx * 2,
                               widget->allocation.y - sy * 3,
                               self->size * 34,
                               self->size * 26);
    return FALSE;
}

GtkWidget *calf_toggle_new_with_adjustment(GtkAdjustment *_adjustment)
{
    GtkWidget *widget = GTK_WIDGET( g_object_new (CALF_TYPE_TOGGLE, NULL ));
    if (widget) {
        gtk_range_set_adjustment(GTK_RANGE(widget), _adjustment);
        g_signal_connect(GTK_OBJECT(widget), "value-changed", G_CALLBACK(calf_toggle_value_changed), widget);
    }
    return widget;
}

GType
calf_toggle_get_type (void)
{
    static GType type = 0;
    if (!type) {
        
        static const GTypeInfo type_info = {
            sizeof(CalfToggleClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            (GClassInitFunc)calf_toggle_class_init,
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof(CalfToggle),
            0,    /* n_preallocs */
            (GInstanceInitFunc)calf_toggle_init
        };
        
        for (int i = 0; ; i++) {
            //char *name = g_strdup_printf("CalfToggle%u%d", 
                //((unsigned int)(intptr_t)calf_toggle_class_init) >> 16, i);
            const char *name = "CalfToggle";
            if (g_type_from_name(name)) {
                //free(name);
                continue;
            }
            type = g_type_register_static( GTK_TYPE_RANGE,
                                           name,
                                           &type_info,
                                           (GTypeFlags)0);
            //free(name);
            break;
        }
    }
    return type;
}


///////////////////////////////////////// button ///////////////////////////////////////////////

GtkWidget *
calf_button_new(const gchar *label)
{
    GtkWidget *widget = GTK_WIDGET( g_object_new (CALF_TYPE_BUTTON, NULL ));
    gtk_button_set_label(GTK_BUTTON(widget), label);
    return widget;
}
static gboolean
calf_button_expose (GtkWidget *widget, GdkEventExpose *event)
{
    g_assert(CALF_IS_BUTTON(widget) || CALF_IS_TOGGLE_BUTTON(widget) || CALF_IS_RADIO_BUTTON(widget));
    
    if (gtk_widget_is_drawable (widget)) {
        
        GdkWindow *window    = widget->window;
        GtkWidget *child     = GTK_BIN (widget)->child;
        cairo_t *c           = gdk_cairo_create(GDK_DRAWABLE(window));
        
        int x  = widget->allocation.x;
        int y  = widget->allocation.y;
        int sx = widget->allocation.width;
        int sy = widget->allocation.height;
        int ox = widget->style->xthickness;
        int oy = widget->style->ythickness;
        int bx = x + ox + 1;
        int by = y + oy + 1;
        int bw = sx - 2 * ox - 2;
        int bh = sy - 2 * oy - 2;
        
        float r, g, b;
        float radius, bevel, inset;
        GtkBorder *border;
        
        cairo_rectangle(c, x, y, sx, sy);
        cairo_clip(c);
        
        get_bg_color(widget, NULL, &r, &g, &b);
        gtk_widget_style_get(widget, "border-radius", &radius, "bevel",  &bevel, "inset", &inset, NULL);
        gtk_widget_style_get(widget, "inner-border", &border, NULL);
        
        // inset
        draw_bevel(c, x, y, sx, sy, radius, inset*-1);
        
        // space
        create_rectangle(c, x + ox, y + oy, sx - ox * 2, sy - oy * 2, std::max(0.f, radius - ox));
        cairo_set_source_rgba(c, 0, 0, 0, 0.6);
        cairo_fill(c);
        
        // button
        create_rectangle(c, bx, by, bw, bh, std::max(0.f, radius - ox - 1));
        cairo_set_source_rgb(c, r, g, b);
        cairo_fill(c);
        draw_bevel(c, bx, by, bw, bh, std::max(0.f, radius - ox - 1), bevel);
        
        // pin
        if (CALF_IS_TOGGLE_BUTTON(widget) or CALF_IS_RADIO_BUTTON(widget)) {
            int pinh;
            int pinm = 6;
            gtk_widget_style_get(widget, "indicator", &pinh, NULL);
            get_text_color(widget, NULL, &r, &g, &b);
            float a;
            if (widget->state == GTK_STATE_PRELIGHT)
                gtk_widget_style_get(widget, "alpha-prelight", &a, NULL);
            else if (widget->state == GTK_STATE_ACTIVE)
                gtk_widget_style_get(widget, "alpha-active", &a, NULL);
            else
                gtk_widget_style_get(widget, "alpha-normal", &a, NULL);
            cairo_rectangle(c, x + sx - border->right - ox + pinm, y + sy / 2 - pinh / 2,
                border->right - pinm * 2 - ox, pinh);
            cairo_set_source_rgba(c, r, g, b, a);
            cairo_fill(c);
        }
        
        cairo_destroy(c);
        gtk_container_propagate_expose (GTK_CONTAINER (widget), child, event);
    }
    return FALSE;
}

static void
calf_button_class_init (CalfButtonClass *klass)
{
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);
    widget_class->expose_event = calf_button_expose;
    gtk_widget_class_install_style_property(
        widget_class, g_param_spec_float("border-radius", "Border Radius", "Generate round edges",
        0, 24, 4, GParamFlags(G_PARAM_READWRITE)));
    gtk_widget_class_install_style_property(
        widget_class, g_param_spec_float("bevel", "Bevel", "Bevel the object",
        -2, 2, 0.2, GParamFlags(G_PARAM_READWRITE)));
    gtk_widget_class_install_style_property(
        widget_class, g_param_spec_float("alpha-normal", "Alpha Normal", "Alpha of ring in normal state",
        0.0, 1.0, 0.2, GParamFlags(G_PARAM_READWRITE)));
    gtk_widget_class_install_style_property(
        widget_class, g_param_spec_float("alpha-prelight", "Alpha Prelight", "Alpha of ring in prelight state",
        0.0, 1.0, 1.0, GParamFlags(G_PARAM_READWRITE)));
    gtk_widget_class_install_style_property(
        widget_class, g_param_spec_float("alpha-active", "Alpha Active", "Alpha of ring in active state",
        0.0, 1.0, 0.2, GParamFlags(G_PARAM_READWRITE)));
    gtk_widget_class_install_style_property(
        widget_class, g_param_spec_float("inset", "Inset", "Amount of inset effect",
        0.0, 1.0, 0.2, GParamFlags(G_PARAM_READWRITE)));
}

static void
calf_button_init (CalfButton *self)
{
    GtkWidget *widget = GTK_WIDGET(self);
    widget->requisition.width = 40;
    widget->requisition.height = 20;
}

GType
calf_button_get_type (void)
{
    static GType type = 0;
    if (!type) {
        static const GTypeInfo type_info = {
            sizeof(CalfButtonClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            (GClassInitFunc)calf_button_class_init,
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof(CalfButton),
            0,    /* n_preallocs */
            (GInstanceInitFunc)calf_button_init
        };

        for (int i = 0; ; i++) {
            const char *name = "CalfButton";
            //char *name = g_strdup_printf("CalfButton%u%d", 
                //((unsigned int)(intptr_t)calf_button_class_init) >> 16, i);
            if (g_type_from_name(name)) {
                //free(name);
                continue;
            }
            type = g_type_register_static(GTK_TYPE_BUTTON,
                                          name,
                                          &type_info,
                                          (GTypeFlags)0);
            //free(name);
            break;
        }
    }
    return type;
}


///////////////////////////////////////// toggle button ///////////////////////////////////////////////

GtkWidget *
calf_toggle_button_new(const gchar *label)
{
    GtkWidget *widget = GTK_WIDGET( g_object_new (CALF_TYPE_TOGGLE_BUTTON, NULL ));
    gtk_button_set_label(GTK_BUTTON(widget), label);
    return widget;
}

static void
calf_toggle_button_class_init (CalfToggleButtonClass *klass)
{
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);
    widget_class->expose_event = calf_button_expose;
    gtk_widget_class_install_style_property(
        widget_class, g_param_spec_float("border-radius", "Border Radius", "Generate round edges",
        0, 24, 4, GParamFlags(G_PARAM_READWRITE)));
    gtk_widget_class_install_style_property(
        widget_class, g_param_spec_float("bevel", "Bevel", "Bevel the object",
        -2, 2, 0.2, GParamFlags(G_PARAM_READWRITE)));
    gtk_widget_class_install_style_property(
        widget_class, g_param_spec_float("alpha-normal", "Alpha Normal", "Alpha of ring in normal state",
        0.0, 1.0, 0.2, GParamFlags(G_PARAM_READWRITE)));
    gtk_widget_class_install_style_property(
        widget_class, g_param_spec_float("alpha-prelight", "Alpha Prelight", "Alpha of ring in prelight state",
        0.0, 1.0, 1.0, GParamFlags(G_PARAM_READWRITE)));
    gtk_widget_class_install_style_property(
        widget_class, g_param_spec_float("alpha-active", "Alpha Active", "Alpha of ring in active state",
        0.0, 1.0, 0.2, GParamFlags(G_PARAM_READWRITE)));
    gtk_widget_class_install_style_property(
        widget_class, g_param_spec_float("inset", "Inset", "Amount of inset effect",
        0.0, 1.0, 0.2, GParamFlags(G_PARAM_READWRITE)));
    gtk_widget_class_install_style_property(
        widget_class, g_param_spec_int("indicator", "Indicator", "Height of indicator",
        0, 20, 3, GParamFlags(G_PARAM_READWRITE)));
}

static void
calf_toggle_button_init (CalfToggleButton *self)
{
    GtkWidget *widget = GTK_WIDGET(self);
    widget->requisition.width = 40;
    widget->requisition.height = 20;
}

GType
calf_toggle_button_get_type (void)
{
    static GType type = 0;
    if (!type) {
        static const GTypeInfo type_info = {
            sizeof(CalfToggleButtonClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            (GClassInitFunc)calf_toggle_button_class_init,
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof(CalfToggleButton),
            0,    /* n_preallocs */
            (GInstanceInitFunc)calf_toggle_button_init
        };

        for (int i = 0; ; i++) {
            const char *name = "CalfToggleButton";
            //char *name = g_strdup_printf("CalfToggleButton%u%d", 
                //((unsigned int)(intptr_t)calf_toggle_button_class_init) >> 16, i);
            if (g_type_from_name(name)) {
                //free(name);
                continue;
            }
            type = g_type_register_static(GTK_TYPE_TOGGLE_BUTTON,
                                          name,
                                          &type_info,
                                          (GTypeFlags)0);
            //free(name);
            break;
        }
    }
    return type;
}

///////////////////////////////////////// radio button ///////////////////////////////////////////////

GtkWidget *
calf_radio_button_new(const gchar *label)
{
    GtkWidget *widget = GTK_WIDGET( g_object_new (CALF_TYPE_RADIO_BUTTON, NULL ));
    gtk_button_set_label(GTK_BUTTON(widget), label);
    return widget;
}

static void
calf_radio_button_class_init (CalfRadioButtonClass *klass)
{
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);
    widget_class->expose_event = calf_button_expose;
    gtk_widget_class_install_style_property(
        widget_class, g_param_spec_float("border-radius", "Border Radius", "Generate round edges",
        0, 24, 4, GParamFlags(G_PARAM_READWRITE)));
    gtk_widget_class_install_style_property(
        widget_class, g_param_spec_float("bevel", "Bevel", "Bevel the object",
        -2, 2, 0.2, GParamFlags(G_PARAM_READWRITE)));
    gtk_widget_class_install_style_property(
        widget_class, g_param_spec_float("alpha-normal", "Alpha Normal", "Alpha of ring in normal state",
        0.0, 1.0, 0.2, GParamFlags(G_PARAM_READWRITE)));
    gtk_widget_class_install_style_property(
        widget_class, g_param_spec_float("alpha-prelight", "Alpha Prelight", "Alpha of ring in prelight state",
        0.0, 1.0, 1.0, GParamFlags(G_PARAM_READWRITE)));
    gtk_widget_class_install_style_property(
        widget_class, g_param_spec_float("alpha-active", "Alpha Active", "Alpha of ring in active state",
        0.0, 1.0, 0.2, GParamFlags(G_PARAM_READWRITE)));
    gtk_widget_class_install_style_property(
        widget_class, g_param_spec_float("inset", "Inset", "Amount of inset effect",
        0.0, 1.0, 0.2, GParamFlags(G_PARAM_READWRITE)));
    gtk_widget_class_install_style_property(
        widget_class, g_param_spec_int("indicator", "Indicator", "Height of indicator",
        0, 20, 3, GParamFlags(G_PARAM_READWRITE)));
}

static void
calf_radio_button_init (CalfRadioButton *self)
{
    GtkWidget *widget = GTK_WIDGET(self);
    widget->requisition.width = 40;
    widget->requisition.height = 20;
}

GType
calf_radio_button_get_type (void)
{
    static GType type = 0;
    if (!type) {
        static const GTypeInfo type_info = {
            sizeof(CalfRadioButtonClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            (GClassInitFunc)calf_radio_button_class_init,
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof(CalfRadioButton),
            0,    /* n_preallocs */
            (GInstanceInitFunc)calf_radio_button_init
        };

        for (int i = 0; ; i++) {
            const char *name = "CalfRadioButton";
            //char *name = g_strdup_printf("CalfRadioButton%u%d", 
                //((unsigned int)(intptr_t)calf_radio_button_class_init) >> 16, i);
            if (g_type_from_name(name)) {
                //free(name);
                continue;
            }
            type = g_type_register_static(GTK_TYPE_RADIO_BUTTON,
                                          name,
                                          &type_info,
                                          (GTypeFlags)0);
            //free(name);
            break;
        }
    }
    return type;
}

///////////////////////////////////////// tap button ///////////////////////////////////////////////

GtkWidget *
calf_tap_button_new()
{
    GtkWidget *widget = GTK_WIDGET( g_object_new (CALF_TYPE_TAP_BUTTON, NULL ));
    return widget;
}

static gboolean
calf_tap_button_expose (GtkWidget *widget, GdkEventExpose *event)
{
    g_assert(CALF_IS_TAP_BUTTON(widget));
    CalfTapButton *self = CALF_TAP_BUTTON(widget);
    
    if (!self->image[self->state])
        return FALSE;
        
    int width = gdk_pixbuf_get_width(self->image[0]);
    int height = gdk_pixbuf_get_height(self->image[0]);
    int x = widget->allocation.x + widget->allocation.width / 2 - width / 2;
    int y = widget->allocation.y + widget->allocation.height / 2 - height / 2;
    
    gdk_draw_pixbuf(GDK_DRAWABLE(widget->window),
                    widget->style->fg_gc[0],
                    self->image[self->state],
                    0,
                    0,
                    x,
                    y,
                    width,
                    height,
                    GDK_RGB_DITHER_NORMAL, 0, 0);
    return TRUE;
}

void
calf_tap_button_set_pixbufs (CalfTapButton *self, GdkPixbuf *image1, GdkPixbuf *image2, GdkPixbuf *image3)
{
    GtkWidget *widget = GTK_WIDGET(self);
    self->image[0] = image1;
    self->image[1] = image2;
    self->image[2] = image3;
    widget->requisition.width = gdk_pixbuf_get_width(self->image[0]);
    widget->requisition.height = gdk_pixbuf_get_height(self->image[0]);
    gtk_widget_queue_resize(widget);
}

static void
calf_tap_button_size_request (GtkWidget *widget,
                           GtkRequisition *requisition)
{
    g_assert(CALF_IS_TAP_BUTTON(widget));
    requisition->width  = 70;
    requisition->height = 70;
}
static void
calf_tap_button_class_init (CalfTapButtonClass *klass)
{
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);
    widget_class->expose_event = calf_tap_button_expose;
    widget_class->size_request = calf_tap_button_size_request;
}
static void
calf_tap_button_init (CalfTapButton *self)
{
    self->state = 0;
}

GType
calf_tap_button_get_type (void)
{
    static GType type = 0;
    if (!type) {
        static const GTypeInfo type_info = {
            sizeof(CalfTapButtonClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            (GClassInitFunc)calf_tap_button_class_init,
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof(CalfTapButton),
            0,    /* n_preallocs */
            (GInstanceInitFunc)calf_tap_button_init
        };

        for (int i = 0; ; i++) {
            const char *name = "CalfTapButton";
            //char *name = g_strdup_printf("CalfTapButton%u%d", 
                //((unsigned int)(intptr_t)calf_tap_button_class_init) >> 16, i);
            if (g_type_from_name(name)) {
                //free(name);
                continue;
            }
            type = g_type_register_static(GTK_TYPE_BUTTON,
                                          name,
                                          &type_info,
                                          (GTypeFlags)0);
            //free(name);
            break;
        }
    }
    return type;
}
