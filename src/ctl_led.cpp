/* Calf DSP Library
 * Light emitting diode-like control.
 *
 * Copyright (C) 2008 Krzysztof Foltman
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
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include <calf/ctl_led.h>
#include <math.h>
#include <stdint.h>
#include <malloc.h>

GtkWidget *
calf_led_new()
{
    GtkWidget *widget = GTK_WIDGET( g_object_new (CALF_TYPE_LED, NULL ));
    return widget;
}

static gboolean
calf_led_expose (GtkWidget *widget, GdkEventExpose *event)
{
    g_assert(CALF_IS_LED(widget));
    
    CalfLed *self = CALF_LED(widget);
    GdkWindow *window = widget->window;
    cairo_t *c = gdk_cairo_create(GDK_DRAWABLE(window));

    gdk_cairo_set_source_color(c, &widget->style->bg[0]);
    cairo_rectangle(c, 0, 0, widget->allocation.width, widget->allocation.height);
    cairo_fill(c);

    int xc = widget->allocation.width / 2;
    int yc = widget->allocation.height / 2;
    
    int diameter = (widget->allocation.width < widget->allocation.height ? widget->allocation.width : widget->allocation.height) - 1;

    cairo_pattern_t *pt = cairo_pattern_create_radial(xc, yc + diameter / 4, 0, xc, yc, diameter / 2);
    cairo_pattern_add_color_stop_rgb(pt, 0.0, self->led_state ? 1.0 : 0.25, self->led_state ? 0.5 : 0.125, 0.0);
    cairo_pattern_add_color_stop_rgb(pt, 0.5, self->led_state ? 0.75 : 0.2, 0.0, 0.0);
    cairo_pattern_add_color_stop_rgb(pt, 1.0, self->led_state ? 0.25 : 0.1, 0.0, 0.0);

    cairo_arc(c, xc, yc, diameter / 2, 0, 2 * M_PI);
    cairo_set_line_join(c, CAIRO_LINE_JOIN_BEVEL);
    cairo_set_source (c, pt);
    cairo_fill(c);
    cairo_pattern_destroy(pt);
    
    cairo_arc(c, xc, yc, diameter / 2, 0, 2 * M_PI);
    cairo_set_line_width(c, 0.5);
    cairo_set_source_rgba (c, self->led_state ? 1.0 : 0.3, 0, 0, 0.5);
    cairo_stroke(c);
    
    cairo_destroy(c);

    return TRUE;
}

static void
calf_led_realize(GtkWidget *widget)
{
    GTK_WIDGET_SET_FLAGS(widget, GTK_REALIZED);

    GdkWindowAttr attributes;
    attributes.event_mask = GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK;
    attributes.x = widget->allocation.x;
    attributes.y = widget->allocation.y;
    attributes.width = widget->allocation.width;
    attributes.height = widget->allocation.height;
    attributes.wclass = GDK_INPUT_OUTPUT;
    attributes.window_type = GDK_WINDOW_CHILD;

    widget->window = gdk_window_new(gtk_widget_get_parent_window (widget), &attributes, GDK_WA_X | GDK_WA_Y);

    gdk_window_set_user_data(widget->window, widget);
    widget->style = gtk_style_attach(widget->style, widget->window);
}

static void
calf_led_size_request (GtkWidget *widget,
                           GtkRequisition *requisition)
{
    g_assert(CALF_IS_LED(widget));
    
    requisition->width = 12;
    requisition->height = 12;
}

static void
calf_led_size_allocate (GtkWidget *widget,
                           GtkAllocation *allocation)
{
    g_assert(CALF_IS_LED(widget));
    
    widget->allocation = *allocation;
    
    if (GTK_WIDGET_REALIZED(widget))
        gdk_window_move_resize(widget->window, allocation->x, allocation->y, allocation->width, allocation->height );
}

static gboolean
calf_led_button_press (GtkWidget *widget, GdkEventButton *event)
{
    return TRUE;
}

static void
calf_led_class_init (CalfLedClass *klass)
{
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);
    widget_class->realize = calf_led_realize;
    widget_class->expose_event = calf_led_expose;
    widget_class->size_request = calf_led_size_request;
    widget_class->size_allocate = calf_led_size_allocate;
    widget_class->button_press_event = calf_led_button_press;
}

static void
calf_led_init (CalfLed *self)
{
    // GtkWidget *widget = GTK_WIDGET(self);
    // GTK_WIDGET_SET_FLAGS (widget, GTK_CAN_FOCUS);
    self->led_state = FALSE;
}

void calf_led_set_state(CalfLed *led, gboolean state)
{
    if (state != led->led_state)
    {
        led->led_state = state;
        GtkWidget *widget = GTK_WIDGET (led);
        if (GTK_WIDGET_REALIZED(widget))
            gtk_widget_queue_draw (widget);
    }
}

gboolean calf_led_get_state(CalfLed *led)
{
    return led->led_state;
}

GType
calf_led_get_type (void)
{
    static GType type = 0;
    if (!type) {
        
        static const GTypeInfo type_info = {
            sizeof(CalfLedClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            (GClassInitFunc)calf_led_class_init,
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof(CalfLed),
            0,    /* n_preallocs */
            (GInstanceInitFunc)calf_led_init
        };
        
        for (int i = 0; ; i++) {
            char *name = g_strdup_printf("CalfLed%u%d", 
                ((unsigned int)(intptr_t)calf_led_class_init) >> 16, i);
            if (g_type_from_name(name)) {
                free(name);
                continue;
            }
            type = g_type_register_static(GTK_TYPE_WIDGET,
                                          name,
                                          &type_info,
                                          (GTypeFlags)0);
            free(name);
            break;
        }
    }
    return type;
}

