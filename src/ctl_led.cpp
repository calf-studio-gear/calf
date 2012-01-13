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
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 * Boston, MA  02110-1301  USA
 */
#include <calf/ctl_led.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>

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
    GtkStyle *style = gtk_widget_get_style(widget);
    
    int ox = 4;
    int oy = 3;
    int sx = widget->allocation.width - ox * 2;
    int sy = widget->allocation.height - oy * 2;
    int xc = widget->allocation.width / 2;
    int yc = widget->allocation.height / 2;
    int pad;
    
    if( self->cache_surface == NULL ) {
        // looks like its either first call or the widget has been resized.
        // create the cache_surface.
        cairo_surface_t *window_surface = cairo_get_target( c );
        self->cache_surface = cairo_surface_create_similar( window_surface, 
                                  CAIRO_CONTENT_COLOR,
                                  widget->allocation.width,
                                  widget->allocation.height );
        cairo_t *cache_cr = cairo_create( self->cache_surface );
        
//        if(widget->style->bg_pixmap[0] == NULL) {
            gdk_cairo_set_source_color(cache_cr,&style->bg[GTK_STATE_NORMAL]);
//        } else {
//            gdk_cairo_set_source_pixbuf(cache_cr, GDK_PIXBUF(&style->bg_pixmap[0]), widget->allocation.x, widget->allocation.y + 20);
//        }
        cairo_paint(cache_cr);
        
        // outer (black)
        pad = 0;
        cairo_rectangle(cache_cr, pad, pad, sx + ox * 2 - pad * 2, sy + oy * 2 - pad * 2);
        cairo_set_source_rgb(cache_cr, 0, 0, 0);
        cairo_fill(cache_cr);
        
        // inner (bevel)
        pad = 1;
        cairo_rectangle(cache_cr, pad, pad, sx + ox * 2 - pad * 2, sy + oy * 2 - pad * 2);
        cairo_pattern_t *pat2 = cairo_pattern_create_linear (0, 0, 0, sy + oy * 2 - pad * 2);
        cairo_pattern_add_color_stop_rgba (pat2, 0, 0.23, 0.23, 0.23, 1);
        cairo_pattern_add_color_stop_rgba (pat2, 0.5, 0, 0, 0, 1);
        cairo_set_source (cache_cr, pat2);
        cairo_fill(cache_cr);
        
        cairo_rectangle(cache_cr, ox, oy, sx, sy);
        cairo_set_source_rgb (cache_cr, 0, 0, 0);
        cairo_fill(cache_cr);
        
        cairo_destroy( cache_cr );
    }
    
    cairo_set_source_surface( c, self->cache_surface, 0,0 );
    cairo_paint( c );
    
    
    cairo_pattern_t *pt = cairo_pattern_create_radial(xc, yc, 0, xc, yc, xc > yc ? xc : yc);
    
    float value = self->led_value;
    
    if(self->led_mode >= 4 && self->led_mode <= 5 && value > 1.f) {
        value = 1.f;
    }
    switch (self->led_mode) {
        default:
        case 0:
            // blue-on/off
            cairo_pattern_add_color_stop_rgb(pt, 0.0, value > 0.f ? 0.2 : 0.0, value > 0.f ? 1.0 : 0.25, value > 0.f ? 1.0 : 0.5);
            cairo_pattern_add_color_stop_rgb(pt, 0.5, value > 0.f ? 0.1 : 0.0, value > 0.f ? 0.6 : 0.15, value > 0.f ? 0.75 : 0.3);
            cairo_pattern_add_color_stop_rgb(pt, 1.0, 0.0,                     value > 0.f ? 0.3 : 0.1,  value > 0.f ? 0.5 : 0.2);
            break;
        case 1:
            // red-on/off
            cairo_pattern_add_color_stop_rgb(pt, 0.0, value > 0.f ? 1.0 : 0.5,  value > 0.f ? 0.5 : 0.0, value > 0.f ? 0.2 : 0.0);
            cairo_pattern_add_color_stop_rgb(pt, 0.5, value > 0.f ? 0.75 : 0.3, value > 0.f ? 0.2 : 0.0, value > 0.f ? 0.1 : 0.0);
            cairo_pattern_add_color_stop_rgb(pt, 1.0, value > 0.f ? 0.5 : 0.2,  value > 0.f ? 0.1 : 0.0, 0.0);
            break;
        case 2:
        case 4:
            // blue-dynamic (limited)
            cairo_pattern_add_color_stop_rgb(pt, 0.0, value * 0.2, value * 0.75 + 0.25, value * 0.5 + 0.5);
            cairo_pattern_add_color_stop_rgb(pt, 0.5, value * 0.1, value * 0.45 + 0.15, value * 0.45 + 0.3);
            cairo_pattern_add_color_stop_rgb(pt, 1.0, 0.0,         value * 0.2 + 0.1,   value * 0.3 + 0.2);
            break;
        case 3:
        case 5:
            // red-dynamic (limited)
            cairo_pattern_add_color_stop_rgb(pt, 0.0, value * 0.5 + 0.5,  value * 0.5, value * 0.2);
            cairo_pattern_add_color_stop_rgb(pt, 0.5, value * 0.45 + 0.3, value * 0.2, value * 0.1);
            cairo_pattern_add_color_stop_rgb(pt, 1.0, value * 0.3 + 0.2,  value * 0.1, 0.0);
            break;
        case 6:
            // blue-dynamic with red peak at >= 1.f
            if(value < 1.0) {
                cairo_pattern_add_color_stop_rgb(pt, 0.0, value * 0.2, value * 0.75 + 0.25, value * 0.5 + 0.5);
                cairo_pattern_add_color_stop_rgb(pt, 0.5, value * 0.1, value * 0.45 + 0.15, value * 0.45 + 0.3);
                cairo_pattern_add_color_stop_rgb(pt, 1.0, 0.0,         value * 0.2 + 0.1,   value * 0.3 + 0.2);
            } else {
                cairo_pattern_add_color_stop_rgb(pt, 0.0, 1.0,  0.5, 0.2);
                cairo_pattern_add_color_stop_rgb(pt, 0.5, 0.75, 0.2, 0.1);
                cairo_pattern_add_color_stop_rgb(pt, 1.0, 0.5,  0.1, 0.0);
            }
            break;
        case 7:
            // off @ 0.0, blue < 1.0, red @ 1.0
            if(value < 1.f and value > 0.f) {
                // blue
                cairo_pattern_add_color_stop_rgb(pt, 0.0, 0.2, 1.0, 1.0);
                cairo_pattern_add_color_stop_rgb(pt, 0.5, 0.1, 0.6, 0.75);
                cairo_pattern_add_color_stop_rgb(pt, 1.0, 0.0, 0.3, 0.5);
            } else if(value == 0.f) {
                // off
                cairo_pattern_add_color_stop_rgb(pt, 0.0, 0.0, 0.25, 0.5);
                cairo_pattern_add_color_stop_rgb(pt, 0.5, 0.0, 0.15, 0.3);
                cairo_pattern_add_color_stop_rgb(pt, 1.0, 0.0, 0.1,  0.2);
            } else {
                // red
                cairo_pattern_add_color_stop_rgb(pt, 0.0, 1.0,  0.5, 0.2);
                cairo_pattern_add_color_stop_rgb(pt, 0.5, 0.75, 0.2, 0.1);
                cairo_pattern_add_color_stop_rgb(pt, 1.0, 0.5,  0.1, 0.0);
            }
            break;
    }
    
    cairo_rectangle(c, ox + 1, oy + 1, sx - 2, sy - 2);
    cairo_set_source (c, pt);
    cairo_fill_preserve(c);
    pt = cairo_pattern_create_linear (ox, oy, ox, ox + sy);
    cairo_pattern_add_color_stop_rgba (pt, 0,     1, 1, 1, 0.4);
    cairo_pattern_add_color_stop_rgba (pt, 0.4,   1, 1, 1, 0.1);
    cairo_pattern_add_color_stop_rgba (pt, 0.401, 0, 0, 0, 0.0);
    cairo_pattern_add_color_stop_rgba (pt, 1,     0, 0, 0, 0.2);
    cairo_set_source (c, pt);
    cairo_fill(c);
    cairo_pattern_destroy(pt);

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

    requisition->width = 24;
    requisition->height = 18;
}

static void
calf_led_size_allocate (GtkWidget *widget,
                           GtkAllocation *allocation)
{
    g_assert(CALF_IS_LED(widget));
    CalfLed *led = CALF_LED(widget);
    
    widget->allocation = *allocation;
    
    if( led->cache_surface )
        cairo_surface_destroy( led->cache_surface );
    led->cache_surface = NULL;
    
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
    GtkWidget *widget = GTK_WIDGET(self);
    // GtkWidget *widget = GTK_WIDGET(self);
    // GTK_WIDGET_SET_FLAGS (widget, GTK_CAN_FOCUS);
    self->led_value = 0.f;
    widget->requisition.width = 24;
    widget->requisition.height = 18;
}

void calf_led_set_value(CalfLed *led, float value)
{
    if (value != led->led_value)
    {
        led->led_value = value;
        GtkWidget *widget = GTK_WIDGET (led);
        if (GTK_WIDGET_REALIZED(widget))
            gtk_widget_queue_draw (widget);
    }
}

gboolean calf_led_get_value(CalfLed *led)
{
    return led->led_value;
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

