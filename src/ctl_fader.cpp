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
 
#include <calf/ctl_fader.h>

using namespace calf_plugins;
using namespace dsp;


///////////////////////////////////////// fader ///////////////////////////////////////////////


void calf_fader_set_layout(GtkWidget *widget)
{
    GtkRange *range   = GTK_RANGE(widget);
    CalfFader *fader  = CALF_FADER(widget);
    CalfFaderLayout l = fader->layout;
    GdkRectangle t;
    gint sstart, send;
    
    gtk_range_get_range_rect(range, &t);
    gtk_range_get_slider_range(range, &sstart, &send);
    
    int hor = fader->horizontal;
    int slength;
    gtk_widget_style_get(widget, "slider-length", &slength, NULL);
    
    // widget layout
    l.x = widget->allocation.x + t.x;
    l.y = widget->allocation.y + t.y;
    l.w = t.width; //widget->allocation.width;
    l.h = t.height; //widget->allocation.height;
    
    // image layout
    l.iw = gdk_pixbuf_get_width(fader->image);
    l.ih = gdk_pixbuf_get_height(fader->image);
    
    // first screw layout
    l.s1w  = hor  ? slength : gdk_pixbuf_get_width(fader->image);
    l.s1h  = !hor ? slength : gdk_pixbuf_get_height(fader->image);
    l.s1x1 = 0;
    l.s1y1 = 0;
    l.s1x2 = l.x;
    l.s1y2 = l.y;
    
    // second screw layout
    l.s2w  = l.s1w;
    l.s2h  = l.s1h;
    l.s2x1 = hor  ? l.iw - 3 * l.s2w : 0; 
    l.s2y1 = !hor ? l.ih - 3 * l.s2h : 0; 
    l.s2x2 = hor  ? l.w - l.s2w + l.x : l.x;
    l.s2y2 = !hor ? l.h - l.s2h + l.y : l.y;
    
    // trough 1 layout
    l.t1w  = l.s1w;
    l.t1h  = l.s1h;
    l.t1x1 = hor  ? l.iw - 2 * l.s2w : 0; 
    l.t1y1 = !hor ? l.ih - 2 * l.s2h : 0; 
    
    // trough 2 layout
    l.t2w  = l.s1w;
    l.t2h  = l.s1h;
    l.t2x1 = hor  ? l.iw - l.s2w : 0; 
    l.t2y1 = !hor ? l.ih - l.s2h : 0; 
    
    // slit layout
    l.sw  = hor  ? l.iw - 4 * l.s1w : l.ih;
    l.sh  = !hor ? l.ih - 4 * l.s1h : l.iw;
    l.sx1 = hor  ? l.s1w : 0;
    l.sy1 = !hor ? l.s1h : 0;
    l.sx2 = hor  ? l.s1w + l.x : l.x;
    l.sy2 = !hor ? l.s1h + l.y : l.y;
    l.sw2 = hor  ? l.w - 2 * l.s1w : l.iw;
    l.sh2 = !hor ? l.h - 2 * l.s1h : l.ih;
    
    fader->layout = l;
}

GtkWidget *
calf_fader_new(const int horiz = 0, const int size = 2, const double min = 0, const double max = 1, const double step = 0.1)
{
    GtkObject *adj;
    gint digits;
    
    adj = gtk_adjustment_new (min, min, max, step, 10 * step, 0);
    
    if (fabs (step) >= 1.0 || step == 0.0)
        digits = 0;
    else
        digits = std::min(5, abs((gint) floor (log10 (fabs (step)))));
        
    GtkWidget *widget = GTK_WIDGET( g_object_new (CALF_TYPE_FADER, NULL ));
    CalfFader *self = CALF_FADER(widget);
    
    GTK_RANGE(widget)->orientation = horiz ? GTK_ORIENTATION_HORIZONTAL : GTK_ORIENTATION_VERTICAL;
    gtk_range_set_adjustment(GTK_RANGE(widget), GTK_ADJUSTMENT(adj));
    gtk_scale_set_digits(GTK_SCALE(widget), digits);
    
    self->size = size;
    self->horizontal = horiz;
    self->hover = 0;
    
    return widget;
}

static bool calf_fader_hover(GtkWidget *widget)
{
    CalfFader *fader  = CALF_FADER(widget);
    
    gint mx, my;
    gtk_widget_get_pointer(GTK_WIDGET(widget), &mx, &my);
    
    GtkRange *range   = GTK_RANGE(widget);
    GdkRectangle trough;
    gint sstart, send;
    gtk_range_get_range_rect(range, &trough);
    gtk_range_get_slider_range(range, &sstart, &send);
    
    int hor = fader->horizontal;
    
    int x1 = hor  ? sstart : trough.x;
    int x2 = hor  ? send : trough.x + trough.width;
    int y1 = !hor ? sstart : trough.y;
    int y2 = !hor ? send : trough.y + trough.height;
    
    return mx >= x1 and mx <= x2 and my >= y1 and my <= y2;
}
static void calf_fader_check_hover_change(GtkWidget *widget)
{
    CalfFader *fader = CALF_FADER(widget);
    bool hover = calf_fader_hover(widget);
    if (hover != fader->hover)
        gtk_widget_queue_draw(widget);
    fader->hover = hover;
}
static gboolean
calf_fader_motion (GtkWidget *widget, GdkEventMotion *event)
{
    calf_fader_check_hover_change(widget);
    return FALSE;
}

static gboolean
calf_fader_enter (GtkWidget *widget, GdkEventCrossing *event)
{
    calf_fader_check_hover_change(widget);
    return FALSE;
}

static gboolean
calf_fader_leave (GtkWidget *widget, GdkEventCrossing *event)
{
    CALF_FADER(widget)->hover = false;
    gtk_widget_queue_draw(widget);
    return FALSE;
}
static void
calf_fader_allocate (GtkWidget *widget, GtkAllocation *allocation)
{
    calf_fader_set_layout(widget);
}
static void
calf_fader_request (GtkWidget *widget, GtkAllocation *request)
{
    calf_fader_set_layout(widget);
}
static gboolean
calf_fader_expose (GtkWidget *widget, GdkEventExpose *event)
{
    g_assert(CALF_IS_FADER(widget));
    if (gtk_widget_is_drawable (widget)) {
        
        GdkWindow *window = widget->window;
        GtkScale  *scale  = GTK_SCALE(widget);
        GtkRange  *range  = GTK_RANGE(widget);
        CalfFader *fader  = CALF_FADER(widget);
        CalfFaderLayout l = fader->layout;
        cairo_t   *c      = gdk_cairo_create(GDK_DRAWABLE(window));
        int horiz         = fader->horizontal;
        cairo_rectangle(c, l.x, l.y, l.w, l.h);
        cairo_clip(c);
        
        // position
        double r0  = range->adjustment->upper - range->adjustment->lower;
        double v0 = range->adjustment->value - range->adjustment->lower;
        if ((horiz and gtk_range_get_inverted(range))
        or (!horiz and gtk_range_get_inverted(range)))
            v0 = -v0 + r0;
        int vp = v0 / r0 * (horiz ? l.w - l.s1w : l.h - l.s1h);
        
        l.t1x2 = l.t2x2 = horiz  ? l.x + vp : l.x;
        l.t1y2 = l.t2y2 = !horiz ? l.y + vp : l.y;
        
        GdkPixbuf *i = fader->image;
        
        // screw 1
        cairo_rectangle(c, l.s1x2, l.s1y2, l.s1w, l.s1h);
        gdk_cairo_set_source_pixbuf(c, i, l.s1x2 - l.s1x1, l.s1y2 - l.s1y1);
        cairo_fill(c);
        
        // screw 2
        cairo_rectangle(c, l.s2x2, l.s2y2, l.s2w, l.s2h);
        gdk_cairo_set_source_pixbuf(c, i, l.s2x2 - l.s2x1, l.s2y2 - l.s2y1);
        cairo_fill(c);
        
        // trough
        if (horiz) {
            int x = l.sx2;
            while (x < l.sx2 + l.sw2) {
                cairo_rectangle(c, x, l.sy2, std::min(l.sx2 + l.sw2 - x, l.sw), l.sh2);
                gdk_cairo_set_source_pixbuf(c, i, x - l.sx1, l.sy2 - l.sy1);
                cairo_fill(c);
                x += l.sw;
            }
        } else {
            int y = l.sy2;
            while (y < l.sy2 + l.sh2) {
                cairo_rectangle(c, l.sx2, y, l.sw2, std::min(l.sy2 + l.sh2 - y, l.sh));
                gdk_cairo_set_source_pixbuf(c, i, l.sx2 - l.sx1, y - l.sy1);
                cairo_fill(c);
                y += l.sh;
            }
        }
        
        // slider
        if (fader->hover or widget->state == GTK_STATE_ACTIVE) {
            cairo_rectangle(c, l.t1x2, l.t1y2, l.t1w, l.t1h);
            gdk_cairo_set_source_pixbuf(c, i, l.t1x2 - l.t1x1, l.t1y2 - l.t1y1);
        } else {
            cairo_rectangle(c, l.t2x2, l.t2y2, l.t2w, l.t2h);
            gdk_cairo_set_source_pixbuf(c, i, l.t2x2 - l.t2x1, l.t2y2 - l.t2y1);
        }
        cairo_fill(c);
        
        
        // draw value label
        if (scale->draw_value) {
            PangoLayout *layout;
            gint _x, _y;
            layout = gtk_scale_get_layout (scale);
            gtk_scale_get_layout_offsets (scale, &_x, &_y);
            gtk_paint_layout (widget->style, window, GTK_STATE_NORMAL, FALSE, NULL,
                              widget, horiz ? "hscale" : "vscale", _x, _y, layout);
        }
        
        cairo_destroy(c);
    }
    return FALSE;
}

void
calf_fader_set_pixbuf (CalfFader *self, GdkPixbuf *image)
{
    GtkWidget *widget = GTK_WIDGET(self);
    self->image = image;
    gtk_widget_queue_resize(widget);
}

static void
calf_fader_class_init (CalfFaderClass *klass)
{
    GtkWidgetClass *widget_class      = GTK_WIDGET_CLASS(klass);
    widget_class->expose_event        = calf_fader_expose;
}

static void
calf_fader_init (CalfFader *self)
{
    GtkWidget *widget = GTK_WIDGET(self);
    widget->requisition.width = 40;
    widget->requisition.height = 40;
    
    gtk_signal_connect(GTK_OBJECT(widget), "motion-notify-event", GTK_SIGNAL_FUNC (calf_fader_motion), NULL);
    gtk_signal_connect(GTK_OBJECT(widget), "enter-notify-event", GTK_SIGNAL_FUNC (calf_fader_enter), NULL);
    gtk_signal_connect(GTK_OBJECT(widget), "leave-notify-event", GTK_SIGNAL_FUNC (calf_fader_leave), NULL);
    gtk_signal_connect(GTK_OBJECT(widget), "size-allocate", GTK_SIGNAL_FUNC (calf_fader_allocate), NULL);
    gtk_signal_connect(GTK_OBJECT(widget), "size-request", GTK_SIGNAL_FUNC (calf_fader_request), NULL);
}

GType
calf_fader_get_type (void)
{
    static GType type = 0;
    if (!type) {
        static const GTypeInfo type_info = {
            sizeof(CalfFaderClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            (GClassInitFunc)calf_fader_class_init,
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof(CalfFader),
            0,    /* n_preallocs */
            (GInstanceInitFunc)calf_fader_init
        };

        for (int i = 0; ; i++) {
            const char *name = "CalfFader";
            //char *name = g_strdup_printf("CalfFader%u%d", 
                //((unsigned int)(intptr_t)calf_fader_class_init) >> 16, i);
            if (g_type_from_name(name)) {
                //free(name);
                continue;
            }
            type = g_type_register_static(GTK_TYPE_SCALE,
                                          name,
                                          &type_info,
                                          (GTypeFlags)0);
            //free(name);
            break;
        }
    }
    return type;
}
