/* Calf DSP Library
 * Custom controls (line graph, knob).
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
#include <calf/drawingutils.h>
#include <calf/ctl_pattern.h>
#include <gdk/gdk.h>
#include <stdint.h>
#include <algorithm>

#define RGBAtoINT(r, g, b, a) ((uint32_t)(r * 255) << 24) + ((uint32_t)(g * 255) << 16) + ((uint32_t)(b * 255) << 8) + (uint32_t)(a * 255)
#define INTtoR(color) (float)((color & 0xff000000) >> 24) / 255.f
#define INTtoG(color) (float)((color & 0x00ff0000) >> 16) / 255.f
#define INTtoB(color) (float)((color & 0x0000ff00) >>  8) / 255.f
#define INTtoA(color) (float)((color & 0x000000ff) >>  0) / 255.f

using namespace std;
using namespace calf_plugins;

static GdkRectangle calf_pattern_handle_rect (CalfPattern *p, int bar, int beat, double value)
{
    g_assert(CALF_IS_PATTERN(p));
    
    float top    = round(p->pad_y + p->border_v + p->mbars);
    float bottom = round(top + p->beat_height);
    float height = round(p->beat_height * value);
    float max    = bottom - height;
    
    // move to bars left edge
    float x = p->pad_x + p->border_h + p->mbars + bar * p->bar_width;
    // move to beats left edge
    x += (p->beat_width + p->minner) * beat;
    x = floor(x);
    
    GdkRectangle rect;
    rect.x = (int)x;
    rect.y = (int)max;
    rect.width = (int)p->beat_width;
    rect.height = (int)height;
    return rect;
}

static void calf_pattern_draw_background (GtkWidget *wi, cairo_t *cr)
{
    g_assert(CALF_IS_PATTERN(wi));
    CalfPattern *p = CALF_PATTERN(wi);
    
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 8);
    
    cairo_text_extents_t tx, tx2;
    cairo_text_extents(cr, "Beats", &tx);
    
    p->border_h = tx.width + p->border;
    p->border_v = tx.height + p->border;
    p->bar_width = (p->size_x - p->border_h - p->mbars) / p->bars;
    p->beat_width = floor((p->bar_width - p->mbars - (p->beats - 1) * p->minner) / p->beats);
    p->beat_height = p->size_y - 2 * p->border_v - 2 * p->mbars;
    
    float r, g, b;
    get_text_color(wi, NULL, &r, &g, &b);
    cairo_set_source_rgb(cr, r, g, b);
    
    float _x = p->pad_x + p->border;
    float _y = p->pad_y + p->border - tx.y_bearing;
    
    //cairo_move_to(cr, _x, _y);
    //cairo_show_text(cr, "Bars");
    
    //cairo_move_to(cr, _x, p->height - p->pad_y - p->border + tx.height + tx.y_bearing);
    //cairo_show_text(cr, "Beats");
    
    cairo_move_to(cr, _x, _y + p->border + tx.height + p->mbars);
    cairo_show_text(cr, "100%"); 
    
    cairo_move_to(cr, _x, p->height / 2 - tx.height / 2 - tx.y_bearing);
    cairo_show_text(cr, "50%"); 
    
    cairo_move_to(cr, _x, p->height - p->pad_y - p->border * 2 - tx.height * 2 - tx.y_bearing - p->mbars);
    cairo_show_text(cr, "0%");
    
    for (int i = 0; i < p->bars; i++) {
        _x = p->pad_x + p->border_h + p->mbars + i * p->bar_width;
        char num[4];
        sprintf(num, "%d", i + 1);
        cairo_set_font_size(cr, 8);
        cairo_text_extents(cr, num, &tx2);
        get_text_color(wi, NULL, &r, &g, &b);
        cairo_set_source_rgb(cr, r, g, b);
        cairo_move_to(cr, _x + (p->bar_width - p->mbars) / 2 - tx2.width / 2 - 1, _y);
        cairo_show_text(cr, num);
        for (int j = 0; j < p->beats; j++) {
            calf_pattern_draw_handle(wi, cr, i, j, 0, 0, 1, 0.1, false);
            get_text_color(wi, NULL, &r, &g, &b);
            cairo_set_source_rgb(cr, r, g, b);
            sprintf(num, "%d", j + 1);
            cairo_set_font_size(cr, p->bars * p->beats * 7 > p->width ? 7 : 8);
            cairo_text_extents(cr, num, &tx2);
            cairo_move_to(cr, _x + (p->beat_width + p->minner) * j + p->beat_width / 2 - tx2.width / 2 - 1,
                              p->height - p->pad_y - p->border + tx2.height + tx2.y_bearing);
            cairo_show_text(cr, num);
        }
    }
}

void calf_pattern_draw_handle (GtkWidget *wi, cairo_t *cr, int bar, int beat, int x, int y, double value, float alpha, bool outline)
{
    g_assert(CALF_IS_PATTERN(wi));
    CalfPattern *p = CALF_PATTERN(wi);
    
    GdkRectangle rect = calf_pattern_handle_rect(p, bar, beat, value);
    // move to lower edge
    int bottom = y + rect.y + rect.height;
    int _y = bottom;
    
    int c = 0;
    float r, g, b;
    get_fg_color(wi, NULL, &r, &g, &b);
    cairo_set_source_rgba(cr, r, g, b, alpha);
    while (c++, _y > rect.y + y) {
        // loop over segments, begin at the bottom
        int next = std::max(y + rect.y, (int)round(bottom - p->beat_height / 10.f * c));
        cairo_rectangle(cr, x + rect.x, _y, rect.width, next - _y + 1);
        cairo_fill(cr);
        _y = next;
    }
}

static gboolean
calf_pattern_expose (GtkWidget *widget, GdkEventExpose *event)
{
    g_assert(CALF_IS_PATTERN(widget));
    CalfPattern *p = CALF_PATTERN(widget);
    cairo_t *c = gdk_cairo_create(GDK_DRAWABLE(widget->window));
    if (p->force_redraw) {
        p->pad_x  = widget->style->xthickness;
        p->pad_y  = widget->style->ythickness;
        p->x      = widget->allocation.x;
        p->y      = widget->allocation.y;
        p->width  = widget->allocation.width;
        p->height = widget->allocation.height;
        p->size_x = p->width - 2 * p->pad_x;
        p->size_y = p->height - 2 * p->pad_y;
        float radius, bevel, shadow, lights, dull;
        gtk_widget_style_get(widget, "border-radius", &radius, "bevel",  &bevel, "shadow", &shadow, "lights", &lights, "dull", &dull, NULL);
        cairo_t *bg = cairo_create(p->background_surface);
        display_background(widget, bg, 0, 0, p->size_x, p->size_y, p->pad_x, p->pad_y, radius, bevel, 1, shadow, lights, dull);
        calf_pattern_draw_background(widget, bg);
        cairo_destroy(bg);
    }
    cairo_rectangle(c, p->x, p->y, p->width, p->height);
    cairo_clip(c);
    
    cairo_rectangle(c, p->x, p->y, p->width, p->height);
    cairo_set_source_surface(c, p->background_surface, p->x, p->y);
    cairo_fill(c);
    
    for (int i = 0; i < p->bars; i ++) {
        for (int j = 0; j < p->beats; j++) {
            if  ((p->handle_grabbed.bar == i  and p->handle_grabbed.beat == j)
            or  ((p->handle_hovered.bar == i  and p->handle_hovered.beat == j)
            and  (p->handle_grabbed.bar == -1 and p->handle_grabbed.beat == -1))) {
                calf_pattern_draw_handle(widget, c, i, j, p->x, p->y, 1.0, 0.1);
            }
        }
    }
    
    for (int i = 0; i < p->bars; i++) {
        for (int j = 0; j < p->beats; j++) {
            double val = p->values[i][j];
            if (val > 0)
                calf_pattern_draw_handle(widget, c, i, j, p->x, p->y, val, 0.8);
        }
    }
    
    p->force_redraw = false;
    cairo_destroy(c);
    return TRUE;
}

static calf_pattern_handle
calf_pattern_get_handle_at(CalfPattern *p, double x, double y)
{
    g_assert(CALF_IS_PATTERN(p));
    GdkRectangle rect;
    calf_pattern_handle ret;
    ret.bar = -1;
    ret.beat = -1;
    for (int i = 0; i < p->bars; i++) {
        for (int j = 0; j < p->beats; j++) {
            rect = calf_pattern_handle_rect(p, i, j, 1.0);
            if (x > rect.x and x < rect.x + rect.width) {
                ret.bar = i;
                ret.beat = j;
                return ret;
            }
        }
    }
    return ret;
}

static double
calf_pattern_get_drag_value(CalfPattern *p, double y, double value)
{
    g_assert(CALF_IS_PATTERN(p));
    return std::max(0., std::min(1., value + (p->mouse_y - y) / p->beat_height));
}
static double
calf_pattern_get_value_from_y(CalfPattern *p, double y)
{
    g_assert(CALF_IS_PATTERN(p));
    double _y = (y - p->border_v - p->mbars - p->pad_y) / p->beat_height;
    return 1 - std::max(0., std::min(1., _y));
}

static gboolean
calf_pattern_pointer_motion (GtkWidget *widget, GdkEventMotion *event)
{
    g_assert(CALF_IS_PATTERN(widget));
    CalfPattern *p = CALF_PATTERN(widget);
   
    if (p->handle_grabbed.bar >= 0 and p->handle_grabbed.beat >= 0) {
        // handle grabbed
        double val = p->values[p->handle_grabbed.bar][p->handle_grabbed.beat];
        double new_value = calf_pattern_get_drag_value(p, event->y, val);
        p->values[p->handle_grabbed.bar][p->handle_grabbed.beat] = new_value;
        p->mouse_x = event->x;
        p->mouse_y = event->y;
        g_signal_emit_by_name(widget, "handle-changed", &p->handle_grabbed);
        gtk_widget_queue_draw(widget);
    } else {
        // no handle grabbed
        calf_pattern_handle hh = calf_pattern_get_handle_at(p, event->x, event->y);
        if (hh.bar != p->handle_hovered.bar or hh.beat != p->handle_hovered.beat) {
            if (hh.bar >= 0 and hh.beat >= 0) {
                //gdk_window_set_cursor(widget->window, p->hand_cursor);
                p->handle_hovered.bar  = hh.bar;
                p->handle_hovered.beat = hh.beat;
            } else {
                //gdk_window_set_cursor(widget->window, NULL);
                p->handle_hovered.bar  = -1;
                p->handle_hovered.beat = -1;
            }
            gtk_widget_queue_draw(widget);
        }
    }
    if (event->is_hint) {
        gdk_event_request_motions(event);
    }
    
    return TRUE;
}

static gboolean
calf_pattern_button_press (GtkWidget *widget, GdkEventButton *event)
{
    g_assert(CALF_IS_PATTERN(widget));
    CalfPattern *p = CALF_PATTERN(widget);
    bool inside_handle = false;
    
    p->mouse_x = event->x;
    p->mouse_y = event->y;

    calf_pattern_handle h = calf_pattern_get_handle_at(p, event->x, event->y);
    if (h.bar >= 0 and h.beat >= 0) {
        p->handle_grabbed.bar  = h.bar;
        p->handle_grabbed.beat = h.beat;
        inside_handle = true;
    }
    double val = p->values[p->handle_grabbed.bar][p->handle_grabbed.beat];
    p->startval = val;
    
    if (inside_handle && event->type == GDK_2BUTTON_PRESS) {
        // double click
        p->values[p->handle_grabbed.bar][p->handle_grabbed.beat] = val < 0.5 ? 1 : 0;
        g_signal_emit_by_name(widget, "handle-changed", &p->handle_grabbed);
        p->mouse_x = -1;
        p->mouse_y = -1;
        p->handle_grabbed.bar  = -1;
        p->handle_grabbed.beat = -1;
        p->dblclick = true;
    }
    
    gtk_widget_grab_focus(widget);
    gtk_grab_add(widget);
    gtk_widget_queue_draw(widget);
    return TRUE;
}

static gboolean
calf_pattern_button_release (GtkWidget *widget, GdkEventButton *event)
{
    g_assert(CALF_IS_PATTERN(widget));
    CalfPattern *p = CALF_PATTERN(widget);
    calf_pattern_handle h = p->handle_grabbed;
    if (h.bar < 0 or h.beat < 0)
        return FALSE;
        
    double val = p->values[h.bar][h.beat];
    if (!p->dblclick and abs(p->startval - val) < 0.05) {
        // single click
        val = calf_pattern_get_value_from_y(p, event->y);
        p->values[h.bar][h.beat] = val;
        g_signal_emit_by_name(widget, "handle-changed", &p->handle_grabbed);
    }
    p->dblclick            = false;
    p->mouse_x             = -1;
    p->mouse_y             = -1;
    p->handle_grabbed.bar  = -1;
    p->handle_grabbed.beat = -1;
    
    calf_pattern_handle hh = calf_pattern_get_handle_at(p, event->x, event->y);
    if (hh.bar >= 0 and hh.beat >= 0) {
        p->handle_hovered.bar  = hh.bar;
        p->handle_hovered.beat = hh.beat;
    }
        
    if (GTK_WIDGET_HAS_GRAB(widget))
        gtk_grab_remove(widget);
        
    gtk_widget_queue_draw(widget);
    return TRUE;
}

static gboolean
calf_pattern_scroll (GtkWidget *widget, GdkEventScroll *event)
{
    g_assert(CALF_IS_PATTERN(widget));
    CalfPattern *p = CALF_PATTERN(widget);

    calf_pattern_handle h = calf_pattern_get_handle_at(p, event->x, event->y);
    //printf("%d %d\n", h.bar, h.beat);
    if (h.bar >= 0 and h.beat >= 0) {
        if (event->direction == GDK_SCROLL_UP) {
            // raise handle value
            p->values[h.bar][h.beat] = std::min(1., p->values[h.bar][h.beat] + 0.1);
            g_signal_emit_by_name(widget, "handle-changed", &h);
        } else if (event->direction == GDK_SCROLL_DOWN) {
            //lower handle value
            p->values[h.bar][h.beat] = std::max(0., p->values[h.bar][h.beat] - 0.1);
            g_signal_emit_by_name(widget, "handle-changed", &h);
        }
        gtk_widget_queue_draw(widget);
    }
    return TRUE;
}

static gboolean
calf_pattern_leave (GtkWidget *widget, GdkEventCrossing *event)
{
    g_assert(CALF_IS_PATTERN(widget));
    CalfPattern *p = CALF_PATTERN(widget);
    p->handle_hovered.bar  = -1;
    p->handle_hovered.beat = -1;
    //gdk_window_set_cursor(widget->window, NULL);
    gtk_widget_queue_draw(widget);
    return TRUE;
}

static void
calf_pattern_size_request (GtkWidget *widget,
                           GtkRequisition *requisition)
{
    g_assert(CALF_IS_PATTERN(widget));
}

static void
calf_pattern_size_allocate (GtkWidget *widget,
                           GtkAllocation *allocation)
{
    g_assert(CALF_IS_PATTERN(widget));
    CalfPattern *p = CALF_PATTERN(widget);
    //GtkWidgetClass *parent_class = (GtkWidgetClass *) g_type_class_peek_parent( CALF_PATTERN_GET_CLASS( p ) );
    //GtkAllocation &a = widget->allocation;
    int sx = allocation->width  - p->pad_x * 2;
    int sy = allocation->height - p->pad_y * 2;
    if (sx != p->size_x or sy != p->size_y) {
        p->size_x = sx;
        p->size_y = sy;
        if (p->background_surface)
            cairo_surface_destroy( p->background_surface );
        p->background_surface = cairo_image_surface_create(
            CAIRO_FORMAT_ARGB32, allocation->width, allocation->height );
        p->force_redraw = true;
    }
    widget->allocation = *allocation;
}

static void
calf_pattern_class_init (CalfPatternClass *klass)
{
    // GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);
    widget_class->expose_event = calf_pattern_expose;
    widget_class->button_press_event = calf_pattern_button_press;
    widget_class->button_release_event = calf_pattern_button_release;
    widget_class->motion_notify_event = calf_pattern_pointer_motion;
    widget_class->scroll_event = calf_pattern_scroll;
    widget_class->leave_notify_event = calf_pattern_leave;
    widget_class->size_allocate = calf_pattern_size_allocate;
    widget_class->size_request = calf_pattern_size_request;
    gtk_widget_class_install_style_property(
        widget_class, g_param_spec_float("border-radius", "Border Radius", "Generate round edges",
        0, 24, 4, GParamFlags(G_PARAM_READWRITE)));
    gtk_widget_class_install_style_property(
        widget_class, g_param_spec_float("bevel", "Bevel", "Bevel the object",
        -2, 2, 0.2, GParamFlags(G_PARAM_READWRITE)));
    gtk_widget_class_install_style_property(
        widget_class, g_param_spec_float("shadow", "Shadow", "Draw shadows inside",
        0, 16, 4, GParamFlags(G_PARAM_READWRITE)));
    gtk_widget_class_install_style_property(
        widget_class, g_param_spec_float("lights", "Lights", "Draw lights inside",
        0, 1, 1, GParamFlags(G_PARAM_READWRITE)));
    gtk_widget_class_install_style_property(
        widget_class, g_param_spec_float("dull", "Dull", "Draw dull inside",
        0, 1, 0.25, GParamFlags(G_PARAM_READWRITE)));
        
    g_signal_new("handle-changed",
         G_TYPE_OBJECT, G_SIGNAL_RUN_FIRST,
         0, NULL, NULL,
         g_cclosure_marshal_VOID__POINTER,
         G_TYPE_NONE, 1, G_TYPE_POINTER);
}

static void
calf_pattern_unrealize (GtkWidget *widget, CalfPattern *p)
{
    cairo_surface_destroy(p->background_surface);
}

static void
calf_pattern_init (CalfPattern *p)
{
    GtkWidget *widget = GTK_WIDGET(p);
    
    GTK_WIDGET_SET_FLAGS (widget, GTK_CAN_FOCUS | GTK_SENSITIVE | GTK_PARENT_SENSITIVE);
    gtk_widget_add_events(widget, GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK | GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK);
    
    widget->requisition.width  = 300;
    widget->requisition.height = 60;
    p->pad_x                = widget->style->xthickness;
    p->pad_y                = widget->style->ythickness;
    p->force_redraw         = false;
    p->beats                = 1;
    p->bars                 = 1;
    p->hand_cursor          = gdk_cursor_new(GDK_DOUBLE_ARROW);
    
    g_signal_connect(GTK_OBJECT(widget), "unrealize", G_CALLBACK(calf_pattern_unrealize), (gpointer)p);
    
    p->handle_hovered.bar  = -1;
    p->handle_hovered.beat = -1;
    p->handle_grabbed.bar  = -1;
    p->handle_grabbed.beat = -1;
    
    p->background_surface = NULL;
    
    gtk_event_box_set_visible_window(GTK_EVENT_BOX(widget), FALSE);
}

GtkWidget *
calf_pattern_new()
{
    return GTK_WIDGET( g_object_new (CALF_TYPE_PATTERN, NULL ));
}

GType
calf_pattern_get_type (void)
{
    static GType type = 0;
    if (!type) {
        static const GTypeInfo type_info = {
            sizeof(CalfPatternClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            (GClassInitFunc)calf_pattern_class_init,
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof(CalfPattern),
            0,    /* n_preallocs */
            (GInstanceInitFunc)calf_pattern_init
        };

        GTypeInfo *type_info_copy = new GTypeInfo(type_info);

        for (int i = 0; ; i++) {
            const char *name = "CalfPattern";
            //char *name = g_strdup_printf("CalfPattern%u%d", ((unsigned int)(intptr_t)calf_line_graph_class_init) >> 16, i);
            if (g_type_from_name(name)) {
                //free(name);
                continue;
            }
            type = g_type_register_static( GTK_TYPE_EVENT_BOX,
                                           name,
                                           type_info_copy,
                                           (GTypeFlags)0);
            //free(name);
            break;
        }
    }
    return type;
}
