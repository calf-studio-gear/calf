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
#include <malloc.h>
#include <math.h>
#include <stdint.h>
#include <gdk/gdk.h>

///////////////////////////////////////// knob ///////////////////////////////////////////////

static gboolean
calf_knob_expose (GtkWidget *widget, GdkEventExpose *event)
{
    g_assert(CALF_IS_KNOB(widget));
    
    CalfKnob *self = CALF_KNOB(widget);
    GdkWindow *window = widget->window;
    GtkAdjustment *adj = gtk_range_get_adjustment(GTK_RANGE(widget));
     
    // printf("adjustment = %p value = %f\n", adj, adj->value);
    int ox = widget->allocation.x, oy = widget->allocation.y;
    ox += (widget->allocation.width - self->knob_size * 20) / 2;
    oy += (widget->allocation.height - self->knob_size * 20) / 2;

    int phase = (int)((adj->value - adj->lower) * 64 / (adj->upper - adj->lower));
    // skip middle phase except for true middle value
    if (self->knob_type == 1 && phase == 32) {
        double pt = (adj->value - adj->lower) * 2.0 / (adj->upper - adj->lower) - 1.0;
        if (pt < 0)
            phase = 31;
        if (pt > 0)
            phase = 33;
    }
    // endless knob: skip 90deg highlights unless the value is really a multiple of 90deg
    if (self->knob_type == 3 && !(phase % 16)) {
        if (phase == 64)
            phase = 0;
        double nom = adj->lower + phase * (adj->upper - adj->lower) / 64.0;
        double diff = (adj->value - nom) / (adj->upper - adj->lower);
        if (diff > 0.0001)
            phase = (phase + 1) % 64;
        if (diff < -0.0001)
            phase = (phase + 63) % 64;
    }
    gdk_draw_pixbuf(GDK_DRAWABLE(widget->window), widget->style->fg_gc[0], CALF_KNOB_CLASS(GTK_OBJECT_GET_CLASS(widget))->knob_image[self->knob_size - 1], phase * self->knob_size * 20, self->knob_type * self->knob_size * 20, ox, oy, self->knob_size * 20, self->knob_size * 20, GDK_RGB_DITHER_NORMAL, 0, 0);
    // printf("exposed %p %d+%d\n", widget->window, widget->allocation.x, widget->allocation.y);
    if (gtk_widget_is_focus(widget))
    {
        gtk_paint_focus(widget->style, window, GTK_STATE_NORMAL, NULL, widget, NULL, ox, oy, self->knob_size * 20, self->knob_size * 20);
    }

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

    float scale = (event->state & GDK_SHIFT_MASK) ? 1000 : 100;
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
        gtk_signal_connect(GTK_OBJECT(widget), "value-changed", G_CALLBACK(calf_knob_value_changed), widget);
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

