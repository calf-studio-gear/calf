/* Calf DSP Library
 * Custom controls (line graph, knob).
 * Copyright (C) 2007 Krzysztof Foltman
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
#include <calf/custom_ctl.h>
#include <gdk/gdkkeysyms.h>
#include <cairo/cairo.h>
#include <math.h>

/*
I don't really know how to do it, or if it can be done this way.
struct calf_ui_type_module
{
    GTypeModule *module;
    
    calf_ui_type_module()
    {
        module = g_type_module_new();
        g_type_module_set_name(module, "calf_custom_ctl");
        g_type_module_use(module);
    }
    ~calf_ui_type_module()
    {
        g_type_module_unuse(module);
    }
};

static calf_ui_type_module type_module;
*/

static gboolean
calf_line_graph_expose (GtkWidget *widget, GdkEventExpose *event)
{
    g_assert(CALF_IS_LINE_GRAPH(widget));
    
    CalfLineGraph *lg = CALF_LINE_GRAPH(widget);
    int ox = widget->allocation.x + 1, oy = widget->allocation.y + 1;
    int sx = widget->allocation.width - 2, sy = widget->allocation.height - 2;
    
    cairo_t *c = gdk_cairo_create(GDK_DRAWABLE(widget->window));

    GdkColor sc = { 0, 0, 0, 0 };

    gdk_cairo_set_source_color(c, &sc);
    cairo_rectangle(c, ox, oy, sx, sy);
    cairo_fill(c);

    if (lg->source) {
        float *data = new float[2 * sx];
        GdkColor sc2 = { 0, 0, 65535, 0 };
        gdk_cairo_set_source_color(c, &sc2);
        cairo_set_line_join(c, CAIRO_LINE_JOIN_MITER);
        cairo_set_line_width(c, 1);
        for(int gn = 0; lg->source->get_graph(lg->source_id, gn, data, 2 * sx, c); gn++)
        {
            for (int i = 0; i < 2 * sx; i++)
            {
                int y = (int)(oy + sy / 2 - (sy / 2 - 1) * data[i]);
                if (y < oy) y = oy;
                if (y >= oy + sy) y = oy + sy - 1;
                if (i)
                    cairo_line_to(c, ox + i * 0.5, y);
                else
                    cairo_move_to(c, ox, y);
            }
            cairo_stroke(c);
        }
        delete []data;
    }
    
    cairo_destroy(c);
    
    gtk_paint_shadow(widget->style, widget->window, GTK_STATE_NORMAL, GTK_SHADOW_IN, NULL, widget, NULL, ox - 1, oy - 1, sx + 2, sy + 2);
    // printf("exposed %p %d+%d\n", widget->window, widget->allocation.x, widget->allocation.y);
    
    return TRUE;
}

static void
calf_line_graph_size_request (GtkWidget *widget,
                           GtkRequisition *requisition)
{
    g_assert(CALF_IS_LINE_GRAPH(widget));
    
    // CalfLineGraph *lg = CALF_LINE_GRAPH(widget);
}

static void
calf_line_graph_size_allocate (GtkWidget *widget,
                           GtkAllocation *allocation)
{
    g_assert(CALF_IS_LINE_GRAPH(widget));
    
    widget->allocation = *allocation;
    // printf("allocation %d x %d\n", allocation->width, allocation->height);
}

static void
calf_line_graph_class_init (CalfLineGraphClass *klass)
{
    // GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);
    widget_class->expose_event = calf_line_graph_expose;
    widget_class->size_request = calf_line_graph_size_request;
    widget_class->size_allocate = calf_line_graph_size_allocate;
}

static void
calf_line_graph_init (CalfLineGraph *self)
{
    GtkWidget *widget = GTK_WIDGET(self);
    GTK_WIDGET_SET_FLAGS (widget, GTK_NO_WINDOW);
    widget->requisition.width = 40;
    widget->requisition.height = 40;
}

GtkWidget *
calf_line_graph_new()
{
    return GTK_WIDGET( g_object_new (CALF_TYPE_LINE_GRAPH, NULL ));
}

GType
calf_line_graph_get_type (void)
{
    static GType type = 0;
    if (!type) {
        static const GTypeInfo type_info = {
            sizeof(CalfLineGraphClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            (GClassInitFunc)calf_line_graph_class_init,
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof(CalfLineGraph),
            0,    /* n_preallocs */
            (GInstanceInitFunc)calf_line_graph_init
        };

        GTypeInfo *type_info_copy = new GTypeInfo(type_info);

        for (int i = 0; ; i++) {
            char *name = g_strdup_printf("CalfLineGraph%u%d", ((unsigned int)(intptr_t)calf_line_graph_class_init) >> 16, i);
            if (g_type_from_name(name)) {
                free(name);
                continue;
            }
            type = g_type_register_static( GTK_TYPE_WIDGET,
                                           name,
                                           type_info_copy,
                                           (GTypeFlags)0);
            free(name);
            break;
        }
    }
    return type;
}

///////////////////////////////////////// vu meter ///////////////////////////////////////////////

static gboolean
calf_vumeter_expose (GtkWidget *widget, GdkEventExpose *event)
{
    g_assert(CALF_IS_VUMETER(widget));
    
    CalfVUMeter *vu = CALF_VUMETER(widget);
    int ox = widget->allocation.x + 1, oy = widget->allocation.y + 1;
    int sx = widget->allocation.width - 2, sy = widget->allocation.height - 2;
    
    cairo_t *c = gdk_cairo_create(GDK_DRAWABLE(widget->window));

    GdkColor sc = { 0, 0, 0, 0 };
    gdk_cairo_set_source_color(c, &sc);
    cairo_rectangle(c, ox, oy, sx, sy);
    cairo_fill(c);
    cairo_set_line_width(c, 1);
    
    for (int x = ox; x <= ox + sx; x += 3)
    {
        float ts = (x - ox) * 1.0 / sx;
        float r, g, b;
        if (ts < 0.75)
            r = ts / 0.75, g = 1, b = 0;
        else
            r = 1, g = 1 - (ts - 0.75) / 0.25, b = 0;
        if (vu->value < ts || vu->value <= 0)
            r *= 0.5, g *= 0.5, b *= 0.5;
        GdkColor sc2 = { 0, (guint16)(65535 * r), (guint16)(65535 * g), (guint16)(65535 * b) };
        gdk_cairo_set_source_color(c, &sc2);
        cairo_move_to(c, x, oy);
        cairo_line_to(c, x, oy + sy + 1);
        cairo_move_to(c, x, oy + sy);
        cairo_line_to(c, x, oy );
        cairo_stroke(c);
    }

    cairo_destroy(c);
    
    gtk_paint_shadow(widget->style, widget->window, GTK_STATE_NORMAL, GTK_SHADOW_IN, NULL, widget, NULL, ox - 1, oy - 1, sx + 2, sy + 2);
    // printf("exposed %p %d+%d\n", widget->window, widget->allocation.x, widget->allocation.y);
    
    return TRUE;
}

static void
calf_vumeter_size_request (GtkWidget *widget,
                           GtkRequisition *requisition)
{
    g_assert(CALF_IS_VUMETER(widget));
    
    requisition->width = 50;
    requisition->height = 14;
}

static void
calf_vumeter_size_allocate (GtkWidget *widget,
                           GtkAllocation *allocation)
{
    g_assert(CALF_IS_VUMETER(widget));
    
    widget->allocation = *allocation;
}

static void
calf_vumeter_class_init (CalfVUMeterClass *klass)
{
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);
    widget_class->expose_event = calf_vumeter_expose;
    widget_class->size_request = calf_vumeter_size_request;
    widget_class->size_allocate = calf_vumeter_size_allocate;
}

static void
calf_vumeter_init (CalfVUMeter *self)
{
    GtkWidget *widget = GTK_WIDGET(self);
    GTK_WIDGET_SET_FLAGS (widget, GTK_NO_WINDOW);
    widget->requisition.width = 40;
    widget->requisition.height = 40;
    self->value = 0.5;
}

GtkWidget *
calf_vumeter_new()
{
    return GTK_WIDGET( g_object_new (CALF_TYPE_VUMETER, NULL ));
}

GType
calf_vumeter_get_type (void)
{
    static GType type = 0;
    if (!type) {
        static const GTypeInfo type_info = {
            sizeof(CalfVUMeterClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            (GClassInitFunc)calf_vumeter_class_init,
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof(CalfVUMeter),
            0,    /* n_preallocs */
            (GInstanceInitFunc)calf_vumeter_init
        };

        GTypeInfo *type_info_copy = new GTypeInfo(type_info);

        for (int i = 0; ; i++) {
            char *name = g_strdup_printf("CalfVUMeter%u%d", ((unsigned int)(intptr_t)calf_vumeter_class_init) >> 16, i);
            if (g_type_from_name(name)) {
                free(name);
                continue;
            }
            type = g_type_register_static( GTK_TYPE_WIDGET,
                                           name,
                                           type_info_copy,
                                           (GTypeFlags)0);
            free(name);
            break;
        }
    }
    return type;
}

extern void calf_vumeter_set_value(CalfVUMeter *meter, float value)
{
    if (value != meter->value)
    {
        meter->value = value;
        gtk_widget_queue_draw(GTK_WIDGET(meter));
    }
}

extern float calf_vumeter_get_value(CalfVUMeter *meter)
{
    return meter->value;
}

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
    
    ox += (widget->allocation.width - 40) / 2;
    oy += (widget->allocation.height - 40) / 2;
    
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
    gdk_draw_pixbuf(GDK_DRAWABLE(widget->window), widget->style->fg_gc[0], CALF_KNOB_CLASS(GTK_OBJECT_GET_CLASS(widget))->knob_image, phase * 40, self->knob_type * 40, ox, oy, 40, 40, GDK_RGB_DITHER_NORMAL, 0, 0);
    // printf("exposed %p %d+%d\n", widget->window, widget->allocation.x, widget->allocation.y);
    if (gtk_widget_is_focus(widget))
    {
        gtk_paint_focus(widget->style, window, GTK_STATE_NORMAL, NULL, widget, NULL, ox, oy, 40, 40);
    }
    
    return TRUE;
}

static void
calf_knob_size_request (GtkWidget *widget,
                           GtkRequisition *requisition)
{
    g_assert(CALF_IS_KNOB(widget));
    
    CalfKnob *self = CALF_KNOB(widget);
    requisition->width = 40;
    requisition->height = 40;
}

static void
calf_knob_incr (GtkWidget *widget, int dir_down)
{
    g_assert(CALF_IS_KNOB(widget));
    CalfKnob *self = CALF_KNOB(widget);
    GtkAdjustment *adj = gtk_range_get_adjustment(GTK_RANGE(widget));
    
    float oldvalue = adj->value;

    float halfrange = (adj->upper + adj->lower) / 2;
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
            
    }
    
    return FALSE;
}

static gboolean
calf_knob_button_press (GtkWidget *widget, GdkEventButton *event)
{
    g_assert(CALF_IS_KNOB(widget));
    CalfKnob *self = CALF_KNOB(widget);

    // CalfKnob *lg = CALF_KNOB(widget);
    gtk_widget_grab_focus(widget);
    gtk_grab_add(widget);
    self->start_x = event->x;
    self->start_y = event->y;
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

static gboolean
calf_knob_pointer_motion (GtkWidget *widget, GdkEventMotion *event)
{
    g_assert(CALF_IS_KNOB(widget));
    CalfKnob *self = CALF_KNOB(widget);

    if (GTK_WIDGET_HAS_GRAB(widget)) 
    {
        if (self->knob_type == 3)
        {
            gtk_range_set_value(GTK_RANGE(widget), endless(self->start_value - (event->y - self->start_y) / 100));
        }
        else
        {
            gtk_range_set_value(GTK_RANGE(widget), self->start_value - (event->y - self->start_y) / 100);
        }
    }
    return FALSE;
}

static gboolean
calf_knob_scroll (GtkWidget *widget, GdkEventScroll *event)
{
    calf_knob_incr(widget, event->direction);
    return FALSE;
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
    widget_class->scroll_event = calf_knob_scroll;
    GError *error = NULL;
    klass->knob_image = gdk_pixbuf_new_from_file(PKGLIBDIR "/knob.png", &error);
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

