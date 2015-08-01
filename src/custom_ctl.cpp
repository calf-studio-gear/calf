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
#include <gtk/gtkrange.h>
#include "config.h"
#include <calf/giface.h>
#include <calf/custom_ctl.h>
#include <gdk/gdkkeysyms.h>
#include <sys/stat.h>
#include <math.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <sys/time.h>
#include <algorithm>
#include <iostream>
#include <calf/drawingutils.h>

using namespace calf_plugins;
using namespace dsp;


///////////////////////////////////////// meter scale ///////////////////////////////////////////////

GtkWidget *
calf_meter_scale_new()
{
    GtkWidget *widget = GTK_WIDGET( g_object_new (CALF_TYPE_METER_SCALE, NULL ));
    //CalfMeterScale *self = CALF_METER_SCALE(widget);
    return widget;
}
static gboolean
calf_meter_scale_expose (GtkWidget *widget, GdkEventExpose *event)
{
    g_assert(CALF_IS_METER_SCALE(widget));
    CalfMeterScale *ms = CALF_METER_SCALE(widget);
    if (gtk_widget_is_drawable (widget)) {
        
        GdkWindow *window = widget->window;
        cairo_t *cr = gdk_cairo_create(GDK_DRAWABLE(window));
        cairo_text_extents_t extents;
        
        double ox = widget->allocation.x;
        double oy = widget->allocation.y;
        double sx = widget->allocation.width;
        double sy = widget->allocation.height;
        double width  = widget->allocation.width;
        double height = widget->allocation.height;
        double xthick = widget->style->xthickness;
        double text_w = 0, bar_x = 0, bar_width = 0, bar_y = 0;
        float r, g, b;
        double text_m = 3;
        double dot_s  = 2;
        double dot_m  = 2;
        double dot_y  = 0;
        double dot_y2 = 0;
        cairo_rectangle(cr, ox, oy, sx, sy);
        cairo_clip(cr);
        
        if (ms->position) {
            cairo_select_font_face(cr, "cairo:sans-serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
            cairo_set_font_size(cr, 8);
            cairo_text_extents(cr, "-88.88", &extents);
            text_w = extents.width;
        }
        switch(ms->position) {
            case 1:
                // label on top
                bar_x = ox + xthick;
                bar_width = width - 2 * xthick;
                break;
            case 2:
                // label right
                bar_x = ox + xthick;
                bar_width = width - text_w - 2 * xthick - 2 * text_m;
                break;
            case 3:
                // label bottom
                bar_x = ox + xthick;
                bar_width = width - 2 * xthick;
                break;
            case 4:
                // label left
                bar_x = ox + xthick + text_w + 2 * text_m;
                bar_width = width - text_w - 2 * xthick - 2 * text_m;
                break;
        }
        
        switch (ms->dots) {
            case 0:
            default:
                // no ticks
                bar_y = height / 2;
                break;
            case 1:
                // tick top
                bar_y = oy + dot_s + dot_m + extents.height;
                dot_y = oy + dot_s / 2;
                break;
            case 2:
                // ticks bottom
                bar_y = oy + height - dot_s - dot_m - extents.height + extents.y_bearing;
                dot_y = oy + height - dot_s / 2;
                break;
            case 3:
                // ticks center
                bar_y = oy + height / 2 - extents.y_bearing / 2;
                dot_y = oy + height - dot_s / 2;
                dot_y2 = oy + dot_s / 2;
                break;
        }
        
        const unsigned int s = ms->marker.size();
        get_fg_color(widget, NULL, &r, &g, &b);
        cairo_set_source_rgb(cr, r, g, b);
        for (unsigned int i = 0; i < s; i++) {
            double val = log10(1 + ms->marker[i] * 9);
            if (ms->dots) {
                cairo_arc(cr, bar_x + bar_width * val, dot_y, dot_s / 2, 0, 2*M_PI);
                cairo_fill(cr);
            }
            if (ms->dots == 3) {
                cairo_arc(cr, bar_x + bar_width * val, dot_y2, dot_s / 2, 0, 2*M_PI);
                cairo_fill(cr);
            }
            char str[32];
            if (val < 1.0 / 32768.0)
                snprintf(str, sizeof(str), "-inf");
            else
                snprintf(str, sizeof(str), "%.f", amp2dB(ms->marker[i]));
            cairo_text_extents(cr, str, &extents);
            cairo_move_to(cr,
                std::min(ox + width, std::max(ox, bar_x + bar_width * val - extents.width / 2)), bar_y);
            cairo_show_text(cr, str);
        }
        cairo_destroy(cr);
    }
    return FALSE;
}

static void
calf_meter_scale_size_request (GtkWidget *widget,
                           GtkRequisition *requisition)
{
    g_assert(CALF_IS_METER_SCALE(widget));
    CalfMeterScale *self = CALF_METER_SCALE(widget);
    
    double ythick = widget->style->ythickness;
    double text_h = 8; // FIXME: Pango layout should be used here
    double dot_s  = 2;
    double dot_m  = 2;
    
    requisition->height = ythick*2 + text_h + (dot_m + dot_s) * (self->dots == 3 ? 2 : 1);
}

static void
calf_meter_scale_class_init (CalfMeterScaleClass *klass)
{
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);
    widget_class->expose_event = calf_meter_scale_expose;
    widget_class->size_request = calf_meter_scale_size_request;
}

static void
calf_meter_scale_init (CalfMeterScale *self)
{
    GtkWidget *widget = GTK_WIDGET(self);
    gtk_widget_set_has_window(widget, FALSE);
    widget->requisition.width = 40;
    widget->requisition.height = 12;
    self->mode     = VU_STANDARD;
    self->position = 0;
    self->dots     = 0;
}

GType
calf_meter_scale_get_type (void)
{
    static GType type = 0;
    if (!type) {
        static const GTypeInfo type_info = {
            sizeof(CalfMeterScaleClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            (GClassInitFunc)calf_meter_scale_class_init,
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof(CalfMeterScale),
            0,    /* n_preallocs */
            (GInstanceInitFunc)calf_meter_scale_init
        };

        for (int i = 0; ; i++) {
            char *name = g_strdup_printf("CalfMeterScale%u%d", 
                ((unsigned int)(intptr_t)calf_meter_scale_class_init) >> 16, i);
            if (g_type_from_name(name)) {
                free(name);
                continue;
            }
            type = g_type_register_static(GTK_TYPE_DRAWING_AREA,
                                          name,
                                          &type_info,
                                          (GTypeFlags)0);
            free(name);
            break;
        }
    }
    return type;
}


///////////////////////////////////////// phase graph ///////////////////////////////////////////////

static void
calf_phase_graph_draw_background(GtkWidget *widget, cairo_t *ctx, int sx, int sy, int ox, int oy, float radius, float bevel )
{
    int cx = ox + sx / 2;
    int cy = oy + sy / 2;
    
    display_background(widget, ctx, 0, 0, sx, sy, ox, oy, radius, bevel);
    cairo_set_source_rgb(ctx, 0.35, 0.4, 0.2);
    cairo_select_font_face(ctx, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(ctx, 9);
    cairo_text_extents_t te;
    
    cairo_text_extents (ctx, "M", &te);
    cairo_move_to (ctx, cx + 5, oy + 12);
    cairo_show_text (ctx, "M");
    
    cairo_text_extents (ctx, "S", &te);
    cairo_move_to (ctx, ox + 5, cy - 5);
    cairo_show_text (ctx, "S");
    
    cairo_text_extents (ctx, "L", &te);
    cairo_move_to (ctx, ox + 18, oy + 12);
    cairo_show_text (ctx, "L");
    
    cairo_text_extents (ctx, "R", &te);
    cairo_move_to (ctx, ox + sx - 22, oy + 12);
    cairo_show_text (ctx, "R");
    
    cairo_set_line_width(ctx, 1);
    
    cairo_move_to(ctx, ox, oy + sy * 0.5);
    cairo_line_to(ctx, ox + sx, oy + sy * 0.5);
    cairo_stroke(ctx);
    
    cairo_move_to(ctx, ox + sx * 0.5, oy);
    cairo_line_to(ctx, ox + sx * 0.5, oy + sy);
    cairo_stroke(ctx);
    
    cairo_set_source_rgba(ctx, 0, 0, 0, 0.2);
    cairo_move_to(ctx, ox, oy);
    cairo_line_to(ctx, ox + sx, oy + sy);
    cairo_stroke(ctx);
    
    cairo_move_to(ctx, ox, oy + sy);
    cairo_line_to(ctx, ox + sx, oy);
    cairo_stroke(ctx);
}
static void
calf_phase_graph_copy_surface(cairo_t *ctx, cairo_surface_t *source, int x = 0, int y = 0, float fade = 1.f)
{
    // copy a surface to a cairo context
    cairo_save(ctx);
    cairo_set_source_surface(ctx, source, x, y);
    if (fade < 1.0) {
        float rnd = (float)rand() / (float)RAND_MAX / 100;
        cairo_paint_with_alpha(ctx, fade * 0.35 + 0.05 + rnd);
    } else {
        cairo_paint(ctx);
    }
    cairo_restore(ctx);
}
static gboolean
calf_phase_graph_expose (GtkWidget *widget, GdkEventExpose *event)
{
    g_assert(CALF_IS_PHASE_GRAPH(widget));
    CalfPhaseGraph *pg = CALF_PHASE_GRAPH(widget);
    if (!pg->source) 
        return FALSE;
    
    // dimensions
    int width  = widget->allocation.width;
    int height = widget->allocation.height;
    int ox     = widget->style->xthickness;
    int oy     = widget->style->ythickness;
    int sx     = width - ox * 2;
    int sy     = height - oy * 2;
    int x      = widget->allocation.x;
    int y      = widget->allocation.y;
    
    sx += sx % 2 - 1;
    sy += sy % 2 - 1;
    int rad = sx / 2;
    int cx = ox + sx / 2;
    int cy = oy + sy / 2;
    
    float radius, bevel;
    gtk_widget_style_get(widget, "border-radius", &radius, "bevel",  &bevel, NULL);
    
    // some values as pointers for the audio plug-in call
    float * phase_buffer = 0;
    int length = 0;
    int mode = 2;
    float fade = 0.05;
    bool use_fade = true;
    int accuracy = 1;
    bool display = true;
    
    // cairo initialization stuff
    cairo_t *c = gdk_cairo_create(GDK_DRAWABLE(widget->window));
    cairo_t *ctx_back;
    cairo_t *ctx_cache;
    
    if( pg->background == NULL ) {
        // looks like its either first call or the widget has been resized.
        // create the background surface (stolen from line graph)...
        pg->background = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height );
        pg->cache = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height );
        
        // ...and draw some bling bling onto it...
        ctx_back = cairo_create(pg->background);
        calf_phase_graph_draw_background(widget, ctx_back, sx, sy, ox, oy, radius, bevel);
        // ...and copy it to the cache
        ctx_cache = cairo_create(pg->cache); 
        calf_phase_graph_copy_surface(ctx_cache, pg->background, 0, 0, 1);
    } else {
        ctx_back = cairo_create(pg->background);
        ctx_cache = cairo_create(pg->cache); 
    }
    
    pg->source->get_phase_graph(&phase_buffer, &length, &mode, &use_fade,
                                &fade, &accuracy, &display);
    
    // process some values set by the plug-in
    accuracy *= 2;
    accuracy = 12 - accuracy;
    
    cairo_rectangle(ctx_cache, ox, oy, sx, sy);
    cairo_clip(ctx_cache);
    
    calf_phase_graph_copy_surface(ctx_cache, pg->background, 0, 0, use_fade ? fade : 1);
    
    if(display) {
        cairo_rectangle(ctx_cache, ox, oy, sx, sy);
        cairo_clip(ctx_cache);
        cairo_set_source_rgba(ctx_cache, 0.35, 0.4, 0.2, 1);
        double _a;
        for(int i = 0; i < length; i+= accuracy) {
            float l = phase_buffer[i];
            float r = phase_buffer[i + 1];
            if(l == 0.f and r == 0.f) continue;
            else if(r == 0.f and l > 0.f) _a = M_PI / 2.f;
            else if(r == 0.f and l < 0.f) _a = 3.f *M_PI / 2.f;
            else _a = pg->_atan(l / r, l, r);
            double _R = sqrt(pow(l, 2) + pow(r, 2));
            _a += M_PI / 4.f;
            float x_ = (-1.f)*_R * cos(_a);
            float y_ = _R * sin(_a);
            // mask the cached values
            switch(mode) {
                case 0:
                    // small dots
                    cairo_rectangle (ctx_cache, x_ * rad + cx, y_ * rad + cy, 1, 1);
                    break;
                case 1:
                    // medium dots
                    cairo_rectangle (ctx_cache, x_ * rad + cx - 0.25, y_ * rad + cy - 0.25, 1.5, 1.5);
                    break;
                case 2:
                    // big dots
                    cairo_rectangle (ctx_cache, x_ * rad + cx - 0.5, y_ * rad + cy - 0.5, 2, 2);
                    break;
                case 3:
                    // fields
                    if(i == 0) cairo_move_to(ctx_cache,
                        x_ * rad + cx, y_ * rad + cy);
                    else cairo_line_to(ctx_cache,
                        x_ * rad + cx, y_ * rad + cy);
                    break;
                case 4:
                    // lines
                    if(i == 0) cairo_move_to(ctx_cache,
                        x_ * rad + cx, y_ * rad + cy);
                    else cairo_line_to(ctx_cache,
                        x_ * rad + cx, y_ * rad + cy);
                    break;
            }
        }
        // draw
        switch(mode) {
            case 0:
            case 1:
            case 2:
                cairo_fill(ctx_cache);
                break;
            case 3:
                cairo_fill(ctx_cache);
                break;
            case 4:
                cairo_set_line_width(ctx_cache, 0.5);
                cairo_stroke(ctx_cache);
                break;
        }
    }
    
    calf_phase_graph_copy_surface(c, pg->cache, x, y);
    
    cairo_destroy(c);
    cairo_destroy(ctx_back);
    cairo_destroy(ctx_cache);
    // printf("exposed %p %dx%d %d+%d\n", widget->window, event->area.x, event->area.y, event->area.width, event->area.height);
    return TRUE;
}

static void
calf_phase_graph_size_request (GtkWidget *widget,
                           GtkRequisition *requisition)
{
    g_assert(CALF_IS_PHASE_GRAPH(widget));
    // CalfLineGraph *lg = CALF_LINE_GRAPH(widget);
}

static void
calf_phase_graph_size_allocate (GtkWidget *widget,
                           GtkAllocation *allocation)
{
    g_assert(CALF_IS_PHASE_GRAPH(widget));
    CalfPhaseGraph *lg = CALF_PHASE_GRAPH(widget);

    GtkWidgetClass *parent_class = (GtkWidgetClass *) g_type_class_peek_parent( CALF_PHASE_GRAPH_GET_CLASS( lg ) );

    if(lg->background)
        cairo_surface_destroy(lg->background);
    lg->background = NULL;
    
    widget->allocation = *allocation;
    GtkAllocation &a = widget->allocation;
    if (a.width > a.height) {
        a.x += (a.width - a.height) / 2;
        a.width = a.height;
    }
    if (a.width < a.height) {
        a.y += (a.height - a.width) / 2;
        a.height = a.width;
    }
    parent_class->size_allocate(widget, &a);
}

static void
calf_phase_graph_class_init (CalfPhaseGraphClass *klass)
{
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);
    widget_class->expose_event = calf_phase_graph_expose;
    widget_class->size_request = calf_phase_graph_size_request;
    widget_class->size_allocate = calf_phase_graph_size_allocate;
    gtk_widget_class_install_style_property(
        widget_class, g_param_spec_float("border-radius", "Border Radius", "Generate round edges",
        0, 24, 4, GParamFlags(G_PARAM_READWRITE)));
    gtk_widget_class_install_style_property(
        widget_class, g_param_spec_float("bevel", "Bevel", "Bevel the object",
        -2, 2, 0.2, GParamFlags(G_PARAM_READWRITE)));
}

static void
calf_phase_graph_unrealize (GtkWidget *widget, CalfPhaseGraph *pg)
{
    if( pg->background )
        cairo_surface_destroy(pg->background);
    pg->background = NULL;
}

static void
calf_phase_graph_init (CalfPhaseGraph *self)
{
    GtkWidget *widget = GTK_WIDGET(self);
    widget->requisition.width = 40;
    widget->requisition.height = 40;
    self->background = NULL;
    gtk_widget_set_has_window(widget, FALSE);
    g_signal_connect(GTK_OBJECT(widget), "unrealize", G_CALLBACK(calf_phase_graph_unrealize), (gpointer)self);
}

GtkWidget *
calf_phase_graph_new()
{
    return GTK_WIDGET(g_object_new (CALF_TYPE_PHASE_GRAPH, NULL));
}

GType
calf_phase_graph_get_type (void)
{
    static GType type = 0;
    if (!type) {
        static const GTypeInfo type_info = {
            sizeof(CalfPhaseGraphClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            (GClassInitFunc)calf_phase_graph_class_init,
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof(CalfPhaseGraph),
            0,    /* n_preallocs */
            (GInstanceInitFunc)calf_phase_graph_init
        };

        GTypeInfo *type_info_copy = new GTypeInfo(type_info);

        for (int i = 0; ; i++) {
            //const char *name = "CalfPhaseGraph";
            char *name = g_strdup_printf("CalfPhaseGraph%u%d", ((unsigned int)(intptr_t)calf_phase_graph_class_init) >> 16, i);
            if (g_type_from_name(name)) {
                free(name);
                continue;
            }
            type = g_type_register_static( GTK_TYPE_DRAWING_AREA,
                                           name,
                                           type_info_copy,
                                           (GTypeFlags)0);
            free(name);
            break;
        }
    }
    return type;
}


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
            char *name = g_strdup_printf("CalfToggle%u%d", 
                ((unsigned int)(intptr_t)calf_toggle_class_init) >> 16, i);
            if (g_type_from_name(name)) {
                free(name);
                continue;
            }
            type = g_type_register_static( GTK_TYPE_RANGE,
                                           name,
                                           &type_info,
                                           (GTypeFlags)0);
            free(name);
            break;
        }
    }
    return type;
}

///////////////////////////////////////// frame ///////////////////////////////////////////////


GtkWidget *
calf_frame_new(const char *label)
{
    GtkWidget *widget = GTK_WIDGET( g_object_new (CALF_TYPE_FRAME, NULL ));
    CalfFrame *self = CALF_FRAME(widget);
    gtk_frame_set_label(GTK_FRAME(self), label);
    return widget;
}
static gboolean
calf_frame_expose (GtkWidget *widget, GdkEventExpose *event)
{
    g_assert(CALF_IS_FRAME(widget));
    if (gtk_widget_is_drawable (widget)) {
        
        GdkWindow *window = widget->window;
        cairo_t *c = gdk_cairo_create(GDK_DRAWABLE(window));
        cairo_text_extents_t extents;
        
        int ox = widget->allocation.x;
        int oy = widget->allocation.y;
        int sx = widget->allocation.width;
        int sy = widget->allocation.height;
        
        float rad;
        gtk_widget_style_get(widget, "border-radius", &rad, NULL);
    
        double pad  = widget->style->xthickness;
        double txp  = 4;
        double m    = 0.5;
        double size = 10;
        
        float r, g, b;
    
        cairo_rectangle(c, ox, oy, sx, sy);
        cairo_clip(c);
        
        
        const gchar *lab = gtk_frame_get_label(GTK_FRAME(widget));
        
        cairo_select_font_face(c, "Sans",
              CAIRO_FONT_SLANT_NORMAL,
              CAIRO_FONT_WEIGHT_NORMAL);
        cairo_set_font_size(c, size);
        
        cairo_text_extents(c, lab, &extents);
        
        double lw = extents.width + txp * 2.;
        
        cairo_set_line_width(c, 1.);
        
        cairo_move_to(c, ox + rad + txp + m, oy + size - 2 + m);
        get_text_color(widget, NULL, &r, &g, &b);
        cairo_set_source_rgb(c, r, g, b);
        cairo_show_text(c, lab);
        get_fg_color(widget, NULL, &r, &g, &b);
        cairo_set_source_rgb(c, r, g, b);
        
        // top left
        cairo_move_to(c, ox + m, oy + pad + rad + m);
        cairo_arc (c, ox + rad + m, oy + rad + pad + m, rad, 1 * M_PI, 1.5 * M_PI);
        // top
        cairo_move_to(c, ox + rad + lw + m, oy + pad + m);
        cairo_line_to(c, ox + sx - rad - m, oy + pad + m);
        // top right
        cairo_arc (c, ox + sx - rad - m, oy + rad + pad + m, rad, 1.5 * M_PI, 2 * M_PI);
        // right
        cairo_line_to(c, ox + sx - m, oy + sy - rad - m);
        // bottom right
        cairo_arc (c, ox + sx - rad - m, oy + sy - rad - m, rad, 0 * M_PI, 0.5 * M_PI);
        // bottom
        cairo_line_to(c, ox + rad + m, oy + sy - m);
        // bottom left
        cairo_arc (c, ox + rad + m, oy + sy - rad - m, rad, 0.5 * M_PI, 1 * M_PI);
        // left
        cairo_line_to(c, ox + m, oy + rad + pad + m);
        cairo_stroke(c);
        
        cairo_destroy(c);
    }
    if (gtk_bin_get_child(GTK_BIN(widget))) {
        gtk_container_propagate_expose(GTK_CONTAINER(widget),
                                       gtk_bin_get_child(GTK_BIN(widget)),
                                       event);
    }
    return FALSE;
}

static void
calf_frame_class_init (CalfFrameClass *klass)
{
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);
    widget_class->expose_event = calf_frame_expose;
    gtk_widget_class_install_style_property(
        widget_class, g_param_spec_float("border-radius", "Border Radius", "Generate round edges",
        0, 24, 4, GParamFlags(G_PARAM_READWRITE)));
}

static void
calf_frame_init (CalfFrame *self)
{
    GtkWidget *widget = GTK_WIDGET(self);
    widget->requisition.width = 40;
    widget->requisition.height = 40;
}

GType
calf_frame_get_type (void)
{
    static GType type = 0;
    if (!type) {
        static const GTypeInfo type_info = {
            sizeof(CalfFrameClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            (GClassInitFunc)calf_frame_class_init,
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof(CalfFrame),
            0,    /* n_preallocs */
            (GInstanceInitFunc)calf_frame_init
        };

        for (int i = 0; ; i++) {
            //const char *name = "CalfFrame";
            char *name = g_strdup_printf("CalfFrame%u%d", 
                ((unsigned int)(intptr_t)calf_frame_class_init) >> 16, i);
            if (g_type_from_name(name)) {
                free(name);
                continue;
            }
            type = g_type_register_static(GTK_TYPE_FRAME,
                                          name,
                                          &type_info,
                                          (GTypeFlags)0);
            free(name);
            break;
        }
    }
    return type;
}

///////////////////////////////////////// combo box ///////////////////////////////////////////////

//#define GTK_COMBO_BOX_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GTK_TYPE_COMBO_BOX, GtkComboBoxPrivate))

GtkWidget *
calf_combobox_new()
{
    GtkWidget *widget = GTK_WIDGET( g_object_new (CALF_TYPE_COMBOBOX, NULL ));
    GtkCellRenderer *column = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(widget), column, TRUE);
    gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(widget), column,
                                   "text", 0,
                                   NULL);
    return widget;
}
static gboolean
calf_combobox_expose (GtkWidget *widget, GdkEventExpose *event)
{
    g_assert(CALF_IS_COMBOBOX(widget));
    
    if (gtk_widget_is_drawable (widget)) {
        
        int padx = widget->style->xthickness;
        int pady = widget->style->ythickness;
        
        GtkComboBox *cb = GTK_COMBO_BOX(widget);
        CalfCombobox *ccb = CALF_COMBOBOX(widget);
        GdkWindow *window = widget->window;
        cairo_t *c = gdk_cairo_create(GDK_DRAWABLE(window));
        
        GtkTreeModel *model = gtk_combo_box_get_model(cb);
        GtkTreeIter iter;
        gchar *lab;
        if (gtk_combo_box_get_active_iter (cb, &iter))
            gtk_tree_model_get (model, &iter, 0, &lab, -1);
        else
            lab = g_strdup("");
            
        int x  = widget->allocation.x;
        int y  = widget->allocation.y;
        int sx = widget->allocation.width;
        int sy = widget->allocation.height;
        gint mx, my;
        bool hover = false;
        
        create_rectangle(c, x, y, sx, sy, 0);
        cairo_clip(c);
        
        gtk_widget_get_pointer(widget, &mx, &my);
        if (mx >= 0 and mx < sx and my >= 0 and my < sy)
            hover = true;
            
        // background
        float radius, bevel;
        gtk_widget_style_get(widget, "border-radius", &radius, "bevel",  &bevel, NULL);
        display_background(widget, c, x, y, sx - padx * 2, sy - pady * 2, padx, pady, radius, bevel, g_ascii_isspace(lab[0]) ? 0 : 1, 4, hover ? 0.5 : 0, hover ? 0.1 : 0.25);
        
        // text
        gtk_container_propagate_expose (GTK_CONTAINER (widget),
            GTK_BIN (widget)->child, event);

        // arrow
        if (ccb->arrow) {
            int pw = gdk_pixbuf_get_width(ccb->arrow);
            int ph = gdk_pixbuf_get_height(ccb->arrow);
            gdk_draw_pixbuf(GDK_DRAWABLE(window), widget->style->fg_gc[0],
                ccb->arrow, 0, 0,
                x + sx - padx - pw, y + (sy - ph) / 2, pw, ph,
                GDK_RGB_DITHER_NORMAL, 0, 0);
        }
        
        g_free(lab);
        cairo_destroy(c);
    }
    return FALSE;
}

void
calf_combobox_set_arrow (CalfCombobox *self, GdkPixbuf *arrow)
{
    self->arrow = arrow;
}

static void
calf_combobox_class_init (CalfComboboxClass *klass)
{
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);
    widget_class->expose_event = calf_combobox_expose;
    gtk_widget_class_install_style_property(
        widget_class, g_param_spec_float("border-radius", "Border Radius", "Generate round edges",
        0, 24, 4, GParamFlags(G_PARAM_READWRITE)));
    gtk_widget_class_install_style_property(
        widget_class, g_param_spec_float("bevel", "Bevel", "Bevel the object",
        -2, 2, 0.2, GParamFlags(G_PARAM_READWRITE)));
}

static void
calf_combobox_init (CalfCombobox *self)
{
    GtkWidget *widget = GTK_WIDGET(self);
    widget->requisition.width = 40;
    widget->requisition.height = 20;
}

GType
calf_combobox_get_type (void)
{
    static GType type = 0;
    if (!type) {
        static const GTypeInfo type_info = {
            sizeof(CalfComboboxClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            (GClassInitFunc)calf_combobox_class_init,
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof(CalfCombobox),
            0,    /* n_preallocs */
            (GInstanceInitFunc)calf_combobox_init
        };

        for (int i = 0; ; i++) {
            //const char *name = "CalfCombobox";
            char *name = g_strdup_printf("CalfCombobox%u%d", 
                ((unsigned int)(intptr_t)calf_combobox_class_init) >> 16, i);
            if (g_type_from_name(name)) {
                free(name);
                continue;
            }
            type = g_type_register_static(GTK_TYPE_COMBO_BOX,
                                          name,
                                          &type_info,
                                          (GTypeFlags)0);
            free(name);
            break;
        }
    }
    return type;
}


///////////////////////////////////////// notebook ///////////////////////////////////////////////

#define GTK_NOTEBOOK_PAGE(_glist_)         ((GtkNotebookPage *)((GList *)(_glist_))->data)
struct _GtkNotebookPage
{
  GtkWidget *child;
  GtkWidget *tab_label;
  GtkWidget *menu_label;
  GtkWidget *last_focus_child;  /* Last descendant of the page that had focus */

  guint default_menu : 1;   /* If true, we create the menu label ourself */
  guint default_tab  : 1;   /* If true, we create the tab label ourself */
  guint expand       : 1;
  guint fill         : 1;
  guint pack         : 1;
  guint reorderable  : 1;
  guint detachable   : 1;

  /* if true, the tab label was visible on last allocation; we track this so
   * that we know to redraw the tab area if a tab label was hidden then shown
   * without changing position */
  guint tab_allocated_visible : 1;

  GtkRequisition requisition;
  GtkAllocation allocation;

  gulong mnemonic_activate_signal;
  gulong notify_visible_handler;
};

GtkWidget *
calf_notebook_new()
{
    GtkWidget *widget = GTK_WIDGET( g_object_new (CALF_TYPE_NOTEBOOK, NULL ));
    return widget;
}
static gboolean
calf_notebook_expose (GtkWidget *widget, GdkEventExpose *event)
{
    g_assert(CALF_IS_NOTEBOOK(widget));
    
    GtkNotebook *notebook = GTK_NOTEBOOK(widget);
    CalfNotebook *nb = CALF_NOTEBOOK(widget);
    
    if (gtk_widget_is_drawable (widget)) {
        
        GdkWindow *window = widget->window;
        cairo_t *c = gdk_cairo_create(GDK_DRAWABLE(window));
        cairo_pattern_t *pat = NULL;
        
        int x  = widget->allocation.x;
        int y  = widget->allocation.y;
        int sx = widget->allocation.width;
        int sy = widget->allocation.height;
        int tx = widget->style->xthickness;
        int ty = widget->style->ythickness;
        int lh = 19;
        int bh = lh + 2 * ty;
        
        float r, g, b;
        float alpha;
        gtk_widget_style_get(widget, "background-alpha", &alpha, NULL);
        
        cairo_rectangle(c, x, y, sx, sy);
        cairo_clip(c);
        
        int add = 0;
        
        if (notebook->show_tabs) {
            GtkNotebookPage *page;
            GList *pages;
            
            gint sp;
            gtk_widget_style_get(widget, "tab-overlap", &sp, NULL);
            
            pages = notebook->children;
            
            int cn = 0;
            while (pages) {
                page = GTK_NOTEBOOK_PAGE (pages);
                pages = pages->next;
                if (page->tab_label->window == event->window &&
                    gtk_widget_is_drawable (page->tab_label)) {
                    int lx = page->tab_label->allocation.x;
                    int lw = page->tab_label->allocation.width;
                    
                    // fix the labels position
                    page->tab_label->allocation.y = y + ty;
                    page->tab_label->allocation.height = lh;
                    
                    // draw tab background
                    cairo_rectangle(c, lx - tx, y, lw + 2 * tx, bh);
                    get_base_color(widget, NULL, &r, &g, &b);
                    cairo_set_source_rgba(c, r,g,b, page != notebook->cur_page ? alpha / 2 : alpha);
                    cairo_fill(c);
                    
                    if (page == notebook->cur_page) {
                        // draw tab light
                        get_bg_color(widget, NULL, &r, &g, &b);
                        cairo_rectangle(c, lx - tx + 2, y + 2, lw + 2 * tx - 4, 2);
                        cairo_set_source_rgb(c, r, g, b);
                        cairo_fill(c);
                        
                        cairo_rectangle(c, lx - tx + 2, y + 1, lw + 2 * tx - 4, 1);
                        cairo_set_source_rgba(c, 0,0,0,0.5);
                        cairo_fill(c);
                        
                        cairo_rectangle(c, lx - tx + 2, y + 4, lw + 2 * tx - 4, 1);
                        cairo_set_source_rgba(c, 1,1,1,0.3);
                        cairo_fill(c);
                    
                    }
                    // draw labels
                    gtk_container_propagate_expose (GTK_CONTAINER (notebook), page->tab_label, event);
                }
                cn++;
            }
            add = bh;
        }
        
        // draw main body
        get_base_color(widget, NULL, &r, &g, &b);
        cairo_rectangle(c, x, y + add, sx, sy - add);
        cairo_set_source_rgba(c, r, g, b, alpha);
        cairo_fill(c);
        
        // draw frame
        cairo_rectangle(c, x + 0.5, y + add + 0.5, sx - 1, sy - add - 1);
        pat = cairo_pattern_create_linear(x, y + add, x, y + sy - add);
        cairo_pattern_add_color_stop_rgba(pat,   0,   0,   0,   0, 0.3);
        cairo_pattern_add_color_stop_rgba(pat, 0.5, 0.5, 0.5, 0.5,   0);
        cairo_pattern_add_color_stop_rgba(pat,   1,   1,   1,   1, 0.2);
        cairo_set_source (c, pat);
        cairo_set_line_width(c, 1);
        cairo_stroke_preserve(c);
                    
        int sw = gdk_pixbuf_get_width(nb->screw);
        int sh = gdk_pixbuf_get_height(nb->screw);
        
        // draw screws
        if (nb->screw) {
            gdk_cairo_set_source_pixbuf(c, nb->screw, x, y + add);
            cairo_fill_preserve(c);
            gdk_cairo_set_source_pixbuf(c, nb->screw, x + sx - sw, y + add);
            cairo_fill_preserve(c);
            gdk_cairo_set_source_pixbuf(c, nb->screw, x, y + sy - sh);
            cairo_fill_preserve(c);
            gdk_cairo_set_source_pixbuf(c, nb->screw, x + sx - sh, y + sy - sh);
            cairo_fill_preserve(c);
        }
        // propagate expose to all children
        if (notebook->cur_page)
            gtk_container_propagate_expose (GTK_CONTAINER (notebook),
                notebook->cur_page->child,
                event);
        
        cairo_pattern_destroy(pat);
        cairo_destroy(c);
        
    }
    return FALSE;
}

void
calf_notebook_set_pixbuf (CalfNotebook *self, GdkPixbuf *image)
{
    self->screw = image;
    gtk_widget_queue_draw(GTK_WIDGET(self));
}

static void
calf_notebook_class_init (CalfNotebookClass *klass)
{
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);
    widget_class->expose_event = calf_notebook_expose;
    gtk_widget_class_install_style_property(
        widget_class, g_param_spec_float("background-alpha", "Alpha Background", "Alpha of background",
        0.0, 1.0, 0.5, GParamFlags(G_PARAM_READWRITE)));
}

static void
calf_notebook_init (CalfNotebook *self)
{
    //GtkWidget *widget = GTK_WIDGET(self);
}

GType
calf_notebook_get_type (void)
{
    static GType type = 0;
    if (!type) {
        static const GTypeInfo type_info = {
            sizeof(CalfNotebookClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            (GClassInitFunc)calf_notebook_class_init,
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof(CalfNotebook),
            0,    /* n_preallocs */
            (GInstanceInitFunc)calf_notebook_init
        };

        for (int i = 0; ; i++) {
            //const char *name = "CalfNotebook";
            char *name = g_strdup_printf("CalfNotebook%u%d", 
                ((unsigned int)(intptr_t)calf_notebook_class_init) >> 16, i);
            if (g_type_from_name(name)) {
                free(name);
                continue;
            }
            type = g_type_register_static(GTK_TYPE_NOTEBOOK,
                                          name,
                                          &type_info,
                                          (GTypeFlags)0);
            free(name);
            break;
        }
    }
    return type;
}

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
            char *name = g_strdup_printf("CalfFader%u%d", 
                ((unsigned int)(intptr_t)calf_fader_class_init) >> 16, i);
            if (g_type_from_name(name)) {
                free(name);
                continue;
            }
            type = g_type_register_static(GTK_TYPE_SCALE,
                                          name,
                                          &type_info,
                                          (GTypeFlags)0);
            free(name);
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
            //const char *name = "CalfButton";
            char *name = g_strdup_printf("CalfButton%u%d", 
                ((unsigned int)(intptr_t)calf_button_class_init) >> 16, i);
            if (g_type_from_name(name)) {
                free(name);
                continue;
            }
            type = g_type_register_static(GTK_TYPE_BUTTON,
                                          name,
                                          &type_info,
                                          (GTypeFlags)0);
            free(name);
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
            //const char *name = "CalfToggleButton";
            char *name = g_strdup_printf("CalfToggleButton%u%d", 
                ((unsigned int)(intptr_t)calf_toggle_button_class_init) >> 16, i);
            if (g_type_from_name(name)) {
                free(name);
                continue;
            }
            type = g_type_register_static(GTK_TYPE_TOGGLE_BUTTON,
                                          name,
                                          &type_info,
                                          (GTypeFlags)0);
            free(name);
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
            //const char *name = "CalfRadioButton";
            char *name = g_strdup_printf("CalfRadioButton%u%d", 
                ((unsigned int)(intptr_t)calf_radio_button_class_init) >> 16, i);
            if (g_type_from_name(name)) {
                free(name);
                continue;
            }
            type = g_type_register_static(GTK_TYPE_RADIO_BUTTON,
                                          name,
                                          &type_info,
                                          (GTypeFlags)0);
            free(name);
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
            char *name = g_strdup_printf("CalfTapButton%u%d", 
                ((unsigned int)(intptr_t)calf_tap_button_class_init) >> 16, i);
            if (g_type_from_name(name)) {
                free(name);
                continue;
            }
            type = g_type_register_static(GTK_TYPE_BUTTON,
                                          name,
                                          &type_info,
                                          (GTypeFlags)0);
            free(name);
            break;
        }
    }
    return type;
}


///////////////////////////////////////// tuner ///////////////////////////////////////////////

static void calf_tuner_create_dot(cairo_t *ctx, int dots, int dot, float rad)
{
    cairo_save(ctx);
    cairo_rotate(ctx, dot * M_PI / (dots * 8) * 2);
    cairo_move_to(ctx, 0, -rad);
    cairo_line_to(ctx, 0, 0);
    cairo_stroke(ctx);
    cairo_restore(ctx);
}

static void
calf_tuner_draw_background( GtkWidget *widget, cairo_t *ctx, int sx, int sy, int ox, int oy )
{
    int dw    = 2;
    int dm    = 1;
    int x0    = ox + 0.025;
    int x1    = ox + sx - 0.025;
    int a     = x1 - x0;
    int dots  = a * 0.5 / (dw + dm);
    float rad = sqrt(2.f) / 2.f * a;
    int cx    = ox + sx / 2;
    int cy    = ox + sy / 2;
    int ccy   = cy - sy / 3 + rad;
    
    display_background(widget, ctx, 0, 0, sx, sy, ox, oy);
    cairo_stroke(ctx);
    cairo_save(ctx);
    
    cairo_rectangle(ctx, ox * 2, oy * 2, sx - 2 * ox, sy - 2 * oy);
    cairo_clip(ctx);
    
    cairo_set_source_rgba(ctx, 0.35, 0.4, 0.2, 0.3);
    cairo_set_line_width(ctx, dw);
    cairo_translate(ctx, cx, ccy);
    
    for(int i = 2; i < dots + 2; i++) {
        calf_tuner_create_dot(ctx, dots, i, rad);
    }
    for(int i = -2; i > -dots - 2; i--) {
        calf_tuner_create_dot(ctx, dots, i, rad);
    }
    cairo_set_line_width(ctx, dw * 3);
    calf_tuner_create_dot(ctx, dots, 0, rad);
}

static void calf_tuner_draw_dot(cairo_t * ctx, float cents, int sx, int sy, int ox, int oy)
{
    cairo_rectangle(ctx, ox * 2, oy * 2, sx - 2 * ox, sy - 2 * oy);
    cairo_clip(ctx);
    
    int dw    = 2;
    int dm    = 1;
    int x0    = ox + 0.025;
    int x1    = ox + sx - 0.025;
    int a     = x1 - x0;
    int dots  = a * 0.5 / (dw + dm);
    int dot   = cents * 2.f * dots;
    float rad = sqrt(2.f) / 2.f * a;
    int cx    = ox + sx / 2;
    int cy    = ox + sy / 2;
    int ccy   = cy - sy / 3 + rad;
    
    int sign  = (dot > 0) - (dot < 0);
    int marg  = dot ? sign : 0;
    cairo_save(ctx);
    cairo_set_source_rgba(ctx, 0.35, 0.4, 0.2, 0.9);
    cairo_translate(ctx, cx, ccy);
    cairo_set_line_width(ctx, dw * (dot ? 1 : 3));
    calf_tuner_create_dot(ctx, dots, dot + marg, rad);
    cairo_restore(ctx);
}

static gboolean
calf_tuner_expose (GtkWidget *widget, GdkEventExpose *event)
{
    g_assert(CALF_IS_TUNER(widget));
    CalfTuner *tuner = CALF_TUNER(widget);
    
    //printf("%d %f\n", tuner->note, tuner->cents);
    
    // dimensions
    int ox = 5, oy = 5;
    int sx = widget->allocation.width - ox * 2, sy = widget->allocation.height - oy * 2;
    int marg = 10;
    int fpt  = 9;
    float fsize = fpt * sy / 25; // 9pt @ 25px height
    
    // cairo initialization stuff
    cairo_t *c = gdk_cairo_create(GDK_DRAWABLE(widget->window));
    cairo_t *ctx_back;
    
    if( tuner->background == NULL ) {
        // looks like its either first call or the widget has been resized.
        // create the background surface (stolen from line graph)...
        cairo_surface_t *window_surface = cairo_get_target(c);
        tuner->background = cairo_surface_create_similar(window_surface, 
                                  CAIRO_CONTENT_COLOR,
                                  widget->allocation.width,
                                  widget->allocation.height );
        
        // ...and draw some bling bling onto it...
        ctx_back = cairo_create(tuner->background);
        calf_tuner_draw_background(widget, ctx_back, sx, sy, ox, oy);
    } else {
        ctx_back = cairo_create(tuner->background);
    }
    
    cairo_set_source_surface(c, cairo_get_target(ctx_back), 0, 0);
    cairo_paint(c);
    
    calf_tuner_draw_dot(c, tuner->cents / 100, sx, sy, ox, oy);
    
    static const char notenames[] = "C\0\0C#\0D\0\0D#\0E\0\0F\0\0F#\0G\0\0G#\0A\0\0A#\0B\0\0";
    const char * note = notenames + (tuner->note % 12) * 3;
    int oct = int(tuner->note / 12) - 2;
    cairo_set_source_rgba(c, 0.35, 0.4, 0.2, 0.9);
    cairo_text_extents_t te;
    if (tuner->note) {
        // Note name
        cairo_select_font_face(c, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
        cairo_set_font_size(c, fsize);
        cairo_text_extents (c, note, &te);
        cairo_move_to (c, ox + marg - te.x_bearing, oy + marg - te.y_bearing);
        cairo_show_text (c, note);
        // octave
        char octn[4];
        sprintf(octn, "%d", oct);
        cairo_set_font_size(c, fsize / 2);
        cairo_text_extents (c, octn, &te);
        cairo_show_text(c, octn);
        // right hand side
        cairo_set_font_size(c, fsize / 4);
        cairo_select_font_face(c, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
        const char * mnotet = "MIDI Note: ";
        char mnotev[32];
        sprintf(mnotev, "%d", tuner->note);
        const char * centst = "Cents: ";
        char centsv[32];
        sprintf(centsv, "%.4f", tuner->cents);
        
        // calc text extents
        cairo_text_extents (c, mnotet, &te);
        int mtw = te.width;
        cairo_text_extents (c, "999", &te);
        int mvw = te.width;
        cairo_text_extents (c, centst, &te);
        int ctw = te.width;
        cairo_text_extents (c, "-9.9999", &te);
        int cvw = te.width;
        float xb = te.x_bearing;
        
        float tw = std::max(ctw, mtw);
        float vw = std::max(cvw, mvw);
        
        // draw MIDI note
        cairo_move_to(c, ox + sx - tw - vw - marg * 2, oy + marg - te.y_bearing);
        cairo_show_text(c, mnotet);
        cairo_move_to(c, ox + sx - vw - xb - marg, oy + marg - te.y_bearing);
        cairo_show_text(c, mnotev);
        // draw cents
        cairo_move_to(c, ox + sx - tw - vw - marg * 2, oy + marg + te.height + 5 - te.y_bearing);
        cairo_show_text(c, centst);
        cairo_move_to(c, ox + sx - vw - xb - marg, oy + marg + te.height + 5 - te.y_bearing);
        cairo_show_text(c, centsv);
    }
    
    cairo_destroy(c);
    cairo_destroy(ctx_back);
    return TRUE;
}

static void
calf_tuner_size_request (GtkWidget *widget,
                           GtkRequisition *requisition)
{
    g_assert(CALF_IS_TUNER(widget));
    // CalfLineGraph *lg = CALF_LINE_GRAPH(widget);
}

static void
calf_tuner_size_allocate (GtkWidget *widget,
                           GtkAllocation *allocation)
{
    g_assert(CALF_IS_TUNER(widget));
    CalfTuner *lg = CALF_TUNER(widget);

    if(lg->background)
        cairo_surface_destroy(lg->background);
    lg->background = NULL;
    
    widget->allocation = *allocation;
}

static void
calf_tuner_class_init (CalfTunerClass *klass)
{
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);
    widget_class->expose_event = calf_tuner_expose;
    widget_class->size_request = calf_tuner_size_request;
    widget_class->size_allocate = calf_tuner_size_allocate;
}

static void
calf_tuner_unrealize (GtkWidget *widget, CalfTuner *tuner)
{
    if( tuner->background )
        cairo_surface_destroy(tuner->background);
    tuner->background = NULL;
}

static void
calf_tuner_init (CalfTuner *self)
{
    GtkWidget *widget = GTK_WIDGET(self);
    widget->requisition.width = 40;
    widget->requisition.height = 40;
    self->background = NULL;
    g_signal_connect(GTK_OBJECT(widget), "unrealize", G_CALLBACK(calf_tuner_unrealize), (gpointer)self);
}

GtkWidget *
calf_tuner_new()
{
    return GTK_WIDGET(g_object_new (CALF_TYPE_TUNER, NULL));
}

GType
calf_tuner_get_type (void)
{
    static GType type = 0;
    if (!type) {
        static const GTypeInfo type_info = {
            sizeof(CalfTunerClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            (GClassInitFunc)calf_tuner_class_init,
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof(CalfTuner),
            0,    /* n_preallocs */
            (GInstanceInitFunc)calf_tuner_init
        };

        GTypeInfo *type_info_copy = new GTypeInfo(type_info);

        for (int i = 0; ; i++) {
            char *name = g_strdup_printf("CalfTuner%u%d", ((unsigned int)(intptr_t)calf_tuner_class_init) >> 16, i);
            if (g_type_from_name(name)) {
                free(name);
                continue;
            }
            type = g_type_register_static( GTK_TYPE_DRAWING_AREA,
                                           name,
                                           type_info_copy,
                                           (GTypeFlags)0);
            free(name);
            break;
        }
    }
    return type;
}
