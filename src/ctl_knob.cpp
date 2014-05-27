/* Calf DSP Library
 * Knob control.
 * Copyright (C) 2007-2010 Krzysztof Foltman, Torben Hohn, Markus Schmidt
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
#include "config.h"
#include <calf/ctl_knob.h>
#include <gdk/gdkkeysyms.h>
#include <cairo/cairo.h>
#if !defined(__APPLE__)
#include <malloc.h>
#endif
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <gdk/gdk.h>

///////////////////////////////////////// knob ///////////////////////////////////////////////

static gboolean
calf_knob_expose (GtkWidget *widget, GdkEventExpose *event)
{
    g_assert(CALF_IS_KNOB(widget));
    
    float widths[6]  = {0, 2, 4, 4.5, 4.5, 5.5};
    float margins[6] = {0, 2, 3, 5.5, 5, 7.5};
    float pins_m[6]  = {0, 4, 8, 12, 11, 21};
    float pins_s[6]  = {0, 3, 4, 4, 4, 5};
    
    CalfKnob *self = CALF_KNOB(widget);
    GtkAdjustment *adj = gtk_range_get_adjustment(GTK_RANGE(widget));
    cairo_t *ctx = gdk_cairo_create(GDK_DRAWABLE(widget->window));
    
    int ox = widget->allocation.x, oy = widget->allocation.y;
    ox += (widget->allocation.width - self->knob_size * 20) / 2;
    oy += (widget->allocation.height - self->knob_size * 20) / 2;
    int size  = self->knob_size * 20;
    int rad   = size / 2;
    int from  = self->knob_type == 3 ? 270 : 135;
    int to    = self->knob_type == 3 ? -90 : 45;
    int phase = (adj->value - adj->lower) * 270 / (adj->upper - adj->lower) + 135;
    int start;
    int neg_b = 0;
    int neg_l = 0;
    
    cairo_rectangle(ctx, ox, oy, size + size / 2, size + size / 2);
    cairo_clip(ctx);
    
    switch (self->knob_type) {
        case 0:
        default:
            // normal knob
            start = 135;
            break;
        case 1:
            // centered @ 270°
            if (adj->value < 0.5) {
                neg_l = 1;
            } else {
                phase = (adj->value - adj->lower) * 270 / (adj->upper - adj->lower) -225;
            }
            start = -90;
            break;
        case 2:
            // reversed
            neg_l = 1;
            start = 45;
            break;
        case 3:
            // 360°
            neg_l = 1;
            neg_b = 1;
            phase = (adj->value - adj->lower) * 360 / (adj->upper - adj->lower) + -90;
            start = phase;
            break;
    }
    
    if (self->knob_type == 1 && phase == 270) {
        double pt = (adj->value - adj->lower) * 2.0 / (adj->upper - adj->lower) - 1.0;
        if (pt < 0)
            phase = 269;
        if (pt > 0)
            phase = 273;
    }
    
    static const double dash[] = {2, 1};
    cairo_set_dash(ctx, dash, 2, 0);
    cairo_set_line_width(ctx, widths[self->knob_size]);
    
    // draw background
    gdk_draw_pixbuf(GDK_DRAWABLE(widget->window),
                    widget->style->fg_gc[0],
                    CALF_KNOB_CLASS(GTK_OBJECT_GET_CLASS(widget))->knob_image[self->knob_size - 1],
                    0, 0, ox, oy,
                    gdk_pixbuf_get_width(CALF_KNOB_CLASS(GTK_OBJECT_GET_CLASS(widget))->knob_image[self->knob_size - 1]),
                    gdk_pixbuf_get_height(CALF_KNOB_CLASS(GTK_OBJECT_GET_CLASS(widget))->knob_image[self->knob_size - 1]),
                    GDK_RGB_DITHER_NORMAL, 0, 0);
    
    // draw unlit
    if (neg_b)
        cairo_arc_negative (ctx, ox + rad, oy + rad, rad - margins[self->knob_size], from * (M_PI / 180.), to * (M_PI / 180.));
    else
        cairo_arc (ctx, ox + rad, oy + rad, rad - margins[self->knob_size], from * (M_PI / 180.), to * (M_PI / 180.));
    cairo_set_source_rgb(ctx, 0, 0.1, 0.1);
    cairo_stroke(ctx);
    
    // draw lit
    float pos1 = (rad - margins[self->knob_size] + widths[self->knob_size] / 2.) / rad;
    float pos2 = (rad - margins[self->knob_size]) / rad;
    float pos3 = (rad - margins[self->knob_size] - widths[self->knob_size] / 2.) / rad;
    cairo_pattern_t *pat = cairo_pattern_create_radial(ox + rad, oy + rad, 0, ox + rad, oy + rad, rad);
    cairo_pattern_add_color_stop_rgba(pat, pos1, 0, 0.9, 1, 0.75);
    cairo_pattern_add_color_stop_rgba(pat, pos2, 0,   1, 1, 1.);
    cairo_pattern_add_color_stop_rgba(pat, pos3, 0, 0.9, 1, 0.75);
    cairo_set_source(ctx, pat);
    if (neg_l)
        cairo_arc_negative (ctx, ox + rad, oy + rad, rad - margins[self->knob_size], start * (M_PI / 180.), phase * (M_PI / 180.));
    else
        cairo_arc (ctx, ox + rad, oy + rad, rad - margins[self->knob_size], start * (M_PI / 180.), phase * (M_PI / 180.));
    cairo_stroke(ctx);
    
    // draw light
    float x = ox + rad + (rad - margins[self->knob_size]) * cos(phase * (M_PI / 180.));
    float y = oy + rad + (rad - margins[self->knob_size]) * sin(phase * (M_PI / 180.));
    if (neg_b)
        cairo_arc_negative (ctx, ox + rad, oy + rad, rad - margins[self->knob_size], from * (M_PI / 180.), to * (M_PI / 180.));
    else
        cairo_arc (ctx, ox + rad, oy + rad, rad - margins[self->knob_size], from * (M_PI / 180.), to * (M_PI / 180.));
    pat = cairo_pattern_create_radial(x, y, widths[self->knob_size] / 2, x, y, widths[self->knob_size]);
    cairo_pattern_add_color_stop_rgba(pat, 0, 1, 1, 1, 1);
    cairo_pattern_add_color_stop_rgba(pat, 1, 0, 0.5, 0.8, 0.);
    cairo_set_source(ctx, pat);
    cairo_stroke(ctx);
    
    // draw shine
    cairo_rectangle(ctx, ox, oy, size, size);
    pat = cairo_pattern_create_radial(x, y, 0, x, y, widths[self->knob_size] * 1.5);
    cairo_pattern_add_color_stop_rgba(pat, 0, 0.8, 1, 1, 0.7);
    cairo_pattern_add_color_stop_rgba(pat, 1, 0, 0.75, 1, 0.);
    cairo_set_source(ctx, pat);
    cairo_fill(ctx);
    
    // draw other shine
    if (neg_l)
        cairo_arc_negative (ctx, ox + rad, oy + rad, rad - margins[self->knob_size] + widths[self->knob_size], start * (M_PI / 180.), phase * (M_PI / 180.));
    else
        cairo_arc (ctx, ox + rad, oy + rad, rad - margins[self->knob_size], start * (M_PI / 180.), phase * (M_PI / 180.));
    pos1 = (rad - margins[self->knob_size] + widths[self->knob_size]) / rad;
    pos2 = (rad - margins[self->knob_size]) / rad;
    pos3 = (rad - margins[self->knob_size] - widths[self->knob_size]) / rad;
    pat = cairo_pattern_create_radial(ox + rad, oy + rad, 0, ox + rad, oy + rad, rad);
    cairo_pattern_add_color_stop_rgba(pat, pos1,   0, 1, 1, 0.);
    cairo_pattern_add_color_stop_rgba(pat, pos2, 0.8, 1, 1, 0.6);
    cairo_pattern_add_color_stop_rgba(pat, pos3,   0, 1, 1, 0.);
    cairo_set_source(ctx, pat);
    cairo_set_line_width(ctx, widths[self->knob_size] * 2.);
    cairo_stroke(ctx);
    
     
    cairo_pattern_destroy(pat);
    
    // draw pin
    float x1 = ox + rad + (rad - pins_m[self->knob_size]) * cos(phase * (M_PI / 180.));
    float y1 = oy + rad + (rad - pins_m[self->knob_size]) * sin(phase * (M_PI / 180.));
    float x2 = ox + rad + (rad - pins_s[self->knob_size] - pins_m[self->knob_size]) * cos(phase * (M_PI / 180.));
    float y2 = oy + rad + (rad - pins_s[self->knob_size] - pins_m[self->knob_size]) * sin(phase * (M_PI / 180.));
    cairo_move_to(ctx, x1, y1);
    cairo_line_to(ctx, x2, y2);
    cairo_set_dash(ctx, dash, 0, 0);
    float col = 0;
    cairo_set_source_rgba(ctx, col, col, col,0.5);
    cairo_set_line_width(ctx, widths[self->knob_size] / 2.);
    cairo_stroke(ctx);
    cairo_destroy(ctx);
    
    return TRUE;
}

static void
calf_knob_size_request (GtkWidget *widget,
                           GtkRequisition *requisition)
{
    g_assert(CALF_IS_KNOB(widget));

    CalfKnob *self = CALF_KNOB(widget);

    // width/height is hardwired at 40px now
    // is now chooseable by "size" value in XML (1-4)
    requisition->width  = 20 * self->knob_size;
    requisition->height = 20 * self->knob_size;
}

static void
calf_knob_incr (GtkWidget *widget, int dir_down)
{
    g_assert(CALF_IS_KNOB(widget));
    CalfKnob *self = CALF_KNOB(widget);
    GtkAdjustment *adj = gtk_range_get_adjustment(GTK_RANGE(widget));

    int oldstep = (int)(0.5f + (adj->value - adj->lower) / adj->step_increment);
    int step;
    int nsteps = (int)(0.5f + (adj->upper - adj->lower) / adj->step_increment); // less 1 actually
    if (dir_down)
        step = oldstep - 1;
    else
        step = oldstep + 1;
    if (self->knob_type == 3 && step >= nsteps)
        step %= nsteps;
    if (self->knob_type == 3 && step < 0)
        step = nsteps - (nsteps - step) % nsteps;

    // trying to reduce error cumulation here, by counting from lowest or from highest
    float value = adj->lower + step * double(adj->upper - adj->lower) / nsteps;
    gtk_range_set_value(GTK_RANGE(widget), value);
    // printf("step %d:%d nsteps %d value %f:%f\n", oldstep, step, nsteps, oldvalue, value);
}

static gboolean
calf_knob_key_press (GtkWidget *widget, GdkEventKey *event)
{
    g_assert(CALF_IS_KNOB(widget));
    CalfKnob *self = CALF_KNOB(widget);
    GtkAdjustment *adj = gtk_range_get_adjustment(GTK_RANGE(widget));

    switch(event->keyval)
    {
        case GDK_Home:
            gtk_range_set_value(GTK_RANGE(widget), adj->lower);
            return TRUE;

        case GDK_End:
            gtk_range_set_value(GTK_RANGE(widget), adj->upper);
            return TRUE;

        case GDK_Up:
            calf_knob_incr(widget, 0);
            return TRUE;

        case GDK_Down:
            calf_knob_incr(widget, 1);
            return TRUE;
            
        case GDK_Shift_L:
        case GDK_Shift_R:
            self->start_value = gtk_range_get_value(GTK_RANGE(widget));
            self->start_y = self->last_y;
            return TRUE;
    }

    return FALSE;
}

static gboolean
calf_knob_key_release (GtkWidget *widget, GdkEventKey *event)
{
    g_assert(CALF_IS_KNOB(widget));
    CalfKnob *self = CALF_KNOB(widget);

    if(event->keyval == GDK_Shift_L || event->keyval == GDK_Shift_R)
    {
        self->start_value = gtk_range_get_value(GTK_RANGE(widget));
        self->start_y = self->last_y;
        return TRUE;
    }
    
    return FALSE;
}

static gboolean
calf_knob_button_press (GtkWidget *widget, GdkEventButton *event)
{
    g_assert(CALF_IS_KNOB(widget));
    CalfKnob *self = CALF_KNOB(widget);

    if (event->type == GDK_2BUTTON_PRESS) {
        gtk_range_set_value(GTK_RANGE(widget), self->default_value);
    }

    // CalfKnob *lg = CALF_KNOB(widget);
    gtk_widget_grab_focus(widget);
    gtk_grab_add(widget);
    self->start_x = event->x;
    self->last_y = self->start_y = event->y;
    self->start_value = gtk_range_get_value(GTK_RANGE(widget));
    
    return TRUE;
}

static gboolean
calf_knob_button_release (GtkWidget *widget, GdkEventButton *event)
{
    g_assert(CALF_IS_KNOB(widget));

    if (GTK_WIDGET_HAS_GRAB(widget))
        gtk_grab_remove(widget);
    return FALSE;
}

static inline float endless(float value)
{
    if (value >= 0)
        return fmod(value, 1.f);
    else
        return fmod(1.f - fmod(1.f - value, 1.f), 1.f);
}

static inline float deadzone(GtkWidget *widget, float value, float incr)
{
    // map to dead zone
    float ov = value;
    if (ov > 0.5)
        ov = 0.1 + ov;
    if (ov < 0.5)
        ov = ov - 0.1;
    
    float nv = ov + incr;
    
    if (nv > 0.6)
        return nv - 0.1;
    if (nv < 0.4)
        return nv + 0.1;
    return 0.5;
}

static gboolean
calf_knob_pointer_motion (GtkWidget *widget, GdkEventMotion *event)
{
    g_assert(CALF_IS_KNOB(widget));
    CalfKnob *self = CALF_KNOB(widget);

    float scale = (event->state & GDK_SHIFT_MASK) ? 2500 : 250;
    gboolean moved = FALSE;
    
    if (GTK_WIDGET_HAS_GRAB(widget)) 
    {
        if (self->knob_type == 3)
        {
            gtk_range_set_value(GTK_RANGE(widget), endless(self->start_value - (event->y - self->start_y) / scale));
        }
        else
        if (self->knob_type == 1)
        {
            gtk_range_set_value(GTK_RANGE(widget), deadzone(GTK_WIDGET(widget), self->start_value, -(event->y - self->start_y) / scale));
        }
        else
        {
            gtk_range_set_value(GTK_RANGE(widget), self->start_value - (event->y - self->start_y) / scale);
        }
        moved = TRUE;
    }
    self->last_y = event->y;
    return moved;
}

static gboolean
calf_knob_scroll (GtkWidget *widget, GdkEventScroll *event)
{
    calf_knob_incr(widget, event->direction);
    return TRUE;
}

static void
calf_knob_class_init (CalfKnobClass *klass)
{
    // GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);
    widget_class->expose_event = calf_knob_expose;
    widget_class->size_request = calf_knob_size_request;
    widget_class->button_press_event = calf_knob_button_press;
    widget_class->button_release_event = calf_knob_button_release;
    widget_class->motion_notify_event = calf_knob_pointer_motion;
    widget_class->key_press_event = calf_knob_key_press;
    widget_class->key_release_event = calf_knob_key_release;
    widget_class->scroll_event = calf_knob_scroll;
    GError *error = NULL;
    klass->knob_image[0] = gdk_pixbuf_new_from_file(PKGLIBDIR "/knob1.png", &error);
    klass->knob_image[1] = gdk_pixbuf_new_from_file(PKGLIBDIR "/knob2.png", &error);
    klass->knob_image[2] = gdk_pixbuf_new_from_file(PKGLIBDIR "/knob3.png", &error);
    klass->knob_image[3] = gdk_pixbuf_new_from_file(PKGLIBDIR "/knob4.png", &error);
    klass->knob_image[4] = gdk_pixbuf_new_from_file(PKGLIBDIR "/knob5.png", &error);
    g_assert(klass->knob_image != NULL);
}

static void
calf_knob_init (CalfKnob *self)
{
    GtkWidget *widget = GTK_WIDGET(self);
    GTK_WIDGET_SET_FLAGS (GTK_WIDGET(self), GTK_CAN_FOCUS);
    widget->requisition.width = 40;
    widget->requisition.height = 40;
}

GtkWidget *
calf_knob_new()
{
    GtkAdjustment *adj = (GtkAdjustment *)gtk_adjustment_new(0, 0, 1, 0.01, 0.5, 0);
    return calf_knob_new_with_adjustment(adj);
}

static gboolean calf_knob_value_changed(gpointer obj)
{
    GtkWidget *widget = (GtkWidget *)obj;
    gtk_widget_queue_draw(widget);
    return FALSE;
}

GtkWidget *calf_knob_new_with_adjustment(GtkAdjustment *_adjustment)
{
    GtkWidget *widget = GTK_WIDGET( g_object_new (CALF_TYPE_KNOB, NULL ));
    if (widget) {
        gtk_range_set_adjustment(GTK_RANGE(widget), _adjustment);
        g_signal_connect(GTK_OBJECT(widget), "value-changed", G_CALLBACK(calf_knob_value_changed), widget);
    }
    return widget;
}

GType
calf_knob_get_type (void)
{
    static GType type = 0;
    if (!type) {
        
        static const GTypeInfo type_info = {
            sizeof(CalfKnobClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            (GClassInitFunc)calf_knob_class_init,
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof(CalfKnob),
            0,    /* n_preallocs */
            (GInstanceInitFunc)calf_knob_init
        };
        
        for (int i = 0; ; i++) {
            char *name = g_strdup_printf("CalfKnob%u%d", 
                ((unsigned int)(intptr_t)calf_knob_class_init) >> 16, i);
            if (g_type_from_name(name)) {
                free(name);
                continue;
            }
            type = g_type_register_static(GTK_TYPE_RANGE,
                                          name,
                                          &type_info,
                                          (GTypeFlags)0);
            free(name);
            break;
        }
    }
    return type;
}
