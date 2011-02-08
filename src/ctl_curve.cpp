/* Calf DSP Library
 * Barely started curve editor widget. Standard GtkCurve is
 * unreliable and deprecated, so I need to make my own.
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
#include <calf/ctl_curve.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

static gpointer parent_class = NULL;

GtkWidget *
calf_curve_new(unsigned int point_limit)
{
    GtkWidget *widget = GTK_WIDGET( g_object_new (CALF_TYPE_CURVE, NULL ));
    g_assert(CALF_IS_CURVE(widget));
    
    CalfCurve *self = CALF_CURVE(widget);
    self->point_limit = point_limit;
    return widget;
}

static gboolean
calf_curve_expose (GtkWidget *widget, GdkEventExpose *event)
{
    g_assert(CALF_IS_CURVE(widget));
    
    CalfCurve *self = CALF_CURVE(widget);
    GdkWindow *window = widget->window;
    cairo_t *c = gdk_cairo_create(GDK_DRAWABLE(window));
    GdkColor scHot = { 0, 65535, 0, 0 };
    GdkColor scPoint = { 0, 65535, 65535, 65535 };
    GdkColor scLine = { 0, 32767, 32767, 32767 };
    if (self->points->size())
    {
        gdk_cairo_set_source_color(c, &scLine);
        for (size_t i = 0; i < self->points->size(); i++)
        {
            const CalfCurve::point &pt = (*self->points)[i];
            if (i == (size_t)self->cur_pt && self->hide_current)
                continue;
            float x = pt.first, y = pt.second;
            self->log2phys(x, y);
            if (!i)
                cairo_move_to(c, x, y);
            else
                cairo_line_to(c, x, y);
        }
        cairo_stroke(c);
    }
    for (size_t i = 0; i < self->points->size(); i++)
    {
        if (i == (size_t)self->cur_pt && self->hide_current)
            continue;
        const CalfCurve::point &pt = (*self->points)[i];
        float x = pt.first, y = pt.second;
        self->log2phys(x, y);
        gdk_cairo_set_source_color(c, (i == (size_t)self->cur_pt) ? &scHot : &scPoint);
        cairo_rectangle(c, x - 2, y - 2, 5, 5);
        cairo_fill(c);
    }
    cairo_destroy(c);

    return TRUE;
}

static void
calf_curve_realize(GtkWidget *widget)
{
    GTK_WIDGET_SET_FLAGS(widget, GTK_REALIZED);

    GdkWindowAttr attributes;
    attributes.event_mask = GDK_EXPOSURE_MASK | GDK_BUTTON1_MOTION_MASK | 
        GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK | 
        GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | 
        GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK;
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
calf_curve_size_request (GtkWidget *widget,
                           GtkRequisition *requisition)
{
    g_assert(CALF_IS_CURVE(widget));
    
    requisition->width = 64;
    requisition->height = 32;
}

static void
calf_curve_size_allocate (GtkWidget *widget,
                           GtkAllocation *allocation)
{
    g_assert(CALF_IS_CURVE(widget));
    
    widget->allocation = *allocation;
    
    if (GTK_WIDGET_REALIZED(widget))
        gdk_window_move_resize(widget->window, allocation->x, allocation->y, allocation->width, allocation->height );
}

static int 
find_nearest(CalfCurve *self, int ex, int ey, int &insert_pt)
{
    float dist = 5;
    int found_pt = -1;
    for (int i = 0; i < (int)self->points->size(); i++)
    {
        float x = (*self->points)[i].first, y = (*self->points)[i].second;
        self->log2phys(x, y);
        float thisdist = std::max(fabs(ex - x), fabs(ey - y));
        if (thisdist < dist)
        {
            dist = thisdist;
            found_pt = i;
        }
        if (ex > x)
            insert_pt = i + 1;
    }
    return found_pt;
}

static gboolean
calf_curve_button_press (GtkWidget *widget, GdkEventButton *event)
{
    g_assert(CALF_IS_CURVE(widget));
    CalfCurve *self = CALF_CURVE(widget);
    int found_pt, insert_pt = -1;
    found_pt = find_nearest(self, event->x, event->y, insert_pt);
    if (found_pt == -1 && insert_pt != -1)
    {
        // if at point limit, do not start anything
        if (self->points->size() >= self->point_limit)
            return TRUE;
        float x = event->x, y = event->y;
        bool hide = false;
        self->phys2log(x, y);
        self->points->insert(self->points->begin() + insert_pt, CalfCurve::point(x, y));
        self->clip(insert_pt, x, y, hide);
        if (hide)
        {
            // give up
            self->points->erase(self->points->begin() + insert_pt);
            return TRUE;
        }
        (*self->points)[insert_pt] = CalfCurve::point(x, y);
        found_pt = insert_pt;
    }
    gtk_widget_grab_focus(widget);
    self->cur_pt = found_pt;
    gtk_widget_queue_draw(widget);
    if (self->sink)
        self->sink->curve_changed(self, *self->points);
    gdk_window_set_cursor(widget->window, self->hand_cursor);
    return TRUE;
}

static gboolean
calf_curve_button_release (GtkWidget *widget, GdkEventButton *event)
{
    g_assert(CALF_IS_CURVE(widget));
    CalfCurve *self = CALF_CURVE(widget);
    if (self->cur_pt != -1 && self->hide_current)
        self->points->erase(self->points->begin() + self->cur_pt);        
    self->cur_pt = -1;
    self->hide_current = false;
    if (self->sink)
        self->sink->curve_changed(self, *self->points);
    gtk_widget_queue_draw(widget);
    gdk_window_set_cursor(widget->window, self->points->size() >= self->point_limit ? self->arrow_cursor : self->pencil_cursor);
    return FALSE;
}

static gboolean
calf_curve_pointer_motion (GtkWidget *widget, GdkEventMotion *event)
{
    g_assert(CALF_IS_CURVE(widget));
    if (event->is_hint)
    {
#if GTK_CHECK_VERSION(2,12,0)
        gdk_event_request_motions(event);
#else
        int a, b;
        GdkModifierType mod;
        gdk_window_get_pointer (event->window, &a, &b, (GdkModifierType*)&mod);
#endif
    }
    CalfCurve *self = CALF_CURVE(widget);
    if (self->cur_pt != -1)
    {
        float x = event->x, y = event->y;
        self->phys2log(x, y);
        self->clip(self->cur_pt, x, y, self->hide_current);
        (*self->points)[self->cur_pt] = CalfCurve::point(x, y);
        if (self->sink)
            self->sink->curve_changed(self, *self->points);
        gtk_widget_queue_draw(widget);        
    }
    else
    {
        int insert_pt = -1;
        if (find_nearest(self, event->x, event->y, insert_pt) == -1)
            gdk_window_set_cursor(widget->window, self->points->size() >= self->point_limit ? self->arrow_cursor : self->pencil_cursor);
        else
            gdk_window_set_cursor(widget->window, self->hand_cursor);
    }
    return FALSE;
}

static void
calf_curve_finalize (GObject *obj)
{
    g_assert(CALF_IS_CURVE(obj));
    CalfCurve *self = CALF_CURVE(obj);
    
    delete self->points;
    self->points = NULL;
    
    G_OBJECT_CLASS(parent_class)->finalize(obj);
}

static void
calf_curve_class_init (CalfCurveClass *klass)
{
    parent_class = g_type_class_peek_parent (klass);
    
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);
    widget_class->realize = calf_curve_realize;
    widget_class->expose_event = calf_curve_expose;
    widget_class->size_request = calf_curve_size_request;
    widget_class->size_allocate = calf_curve_size_allocate;
    widget_class->button_press_event = calf_curve_button_press;
    widget_class->button_release_event = calf_curve_button_release;
    widget_class->motion_notify_event = calf_curve_pointer_motion;
    // widget_class->key_press_event = calf_curve_key_press;
    // widget_class->scroll_event = calf_curve_scroll;
    
    G_OBJECT_CLASS(klass)->finalize = calf_curve_finalize;
}

static void
calf_curve_init (CalfCurve *self)
{
    GtkWidget *widget = GTK_WIDGET(self);
    GTK_WIDGET_SET_FLAGS (widget, GTK_CAN_FOCUS);
    self->points = new CalfCurve::point_vector;
    // XXXKF: destructor
    self->points->push_back(CalfCurve::point(0.f, 1.f));
    self->points->push_back(CalfCurve::point(1.f, 1.f));
    self->x0 = 0.f;
    self->x1 = 1.f;
    self->y0 = 1.f;
    self->y1 = 0.f;
    self->cur_pt = -1;
    self->hide_current = false;
    self->pencil_cursor = gdk_cursor_new(GDK_PENCIL);
    self->hand_cursor = gdk_cursor_new(GDK_FLEUR);
    self->arrow_cursor = gdk_cursor_new(GDK_ARROW);
}

void CalfCurve::log2phys(float &x, float &y) {
    x = (x - x0) / (x1 - x0) * (parent.allocation.width - 2) + 1;
    y = (y - y0) / (y1 - y0) * (parent.allocation.height - 2) + 1;
}

void CalfCurve::phys2log(float &x, float &y) {
    x = x0 + (x - 1) * (x1 - x0) / (parent.allocation.width - 2);
    y = y0 + (y - 1) * (y1 - y0) / (parent.allocation.height - 2);
}

void CalfCurve::clip(int pt, float &x, float &y, bool &hide)
{
    hide = false;
    sink->clip(this, pt, x, y, hide);
    
    float ymin = std::min(y0, y1), ymax = std::max(y0, y1);
    float yamp = ymax - ymin;
    if (pt != 0 && pt != (int)(points->size() - 1))
    {
        if (y < ymin - yamp || y > ymax + yamp)
            hide = true;
    }
    if (x < x0) x = x0;
    if (y < ymin) y = ymin;
    if (x > x1) x = x1;
    if (y > ymax) y = ymax;
    if (pt == 0) x = 0;
    if (pt == (int)(points->size() - 1))
        x = (*points)[pt].first;
    if (pt > 0 && x < (*points)[pt - 1].first)
        x = (*points)[pt - 1].first;
    if (pt < (int)(points->size() - 1) && x > (*points)[pt + 1].first)
        x = (*points)[pt + 1].first;
}

void calf_curve_set_points(GtkWidget *widget, const CalfCurve::point_vector &src)
{
    g_assert(CALF_IS_CURVE(widget));
    CalfCurve *self = CALF_CURVE(widget);
    if (self->points->size() != src.size())
        self->cur_pt = -1;
    *self->points = src;
    
    gtk_widget_queue_draw(widget);
}

GType
calf_curve_get_type (void)
{
    static GType type = 0;
    if (!type) {
        
        static const GTypeInfo type_info = {
            sizeof(CalfCurveClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            (GClassInitFunc)calf_curve_class_init,
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof(CalfCurve),
            0,    /* n_preallocs */
            (GInstanceInitFunc)calf_curve_init
        };
        
        for (int i = 0; ; i++) {
            char *name = g_strdup_printf("CalfCurve%u%d", 
                ((unsigned int)(intptr_t)calf_curve_class_init) >> 16, i);
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

