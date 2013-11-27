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
#include <calf/giface.h>
#include <calf/custom_ctl.h>
#include <gdk/gdkkeysyms.h>
#include <cairo/cairo.h>
#include <math.h>
#include <gdk/gdk.h>
#include <sys/time.h>

#include <iostream>

using namespace calf_plugins;
using namespace dsp;

///////////////////////////////////////// phase graph ///////////////////////////////////////////////

static void
calf_phase_graph_draw_background( cairo_t *ctx, int sx, int sy, int ox, int oy )
{
    int cx = ox + sx / 2;
    int cy = oy + sy / 2;
    
    line_graph_background(ctx, sx, sy, ox, oy);
    cairo_set_source_rgb(ctx, 0.35, 0.4, 0.2);
    cairo_select_font_face(ctx, "Bitstream Vera Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
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
calf_phase_graph_copy_surface(cairo_t *ctx, cairo_surface_t *source, float fade = 1.f)
{
    // copy a surface to a cairo context
    cairo_save(ctx);
    cairo_set_source_surface(ctx, source, 0, 0);
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
    int ox = 5, oy = 5;
    int sx = widget->allocation.width - ox * 2, sy = widget->allocation.height - oy * 2;
    sx += sx % 2 - 1;
    sy += sy % 2 - 1;
    int rad = sx / 2;
    int cx = ox + sx / 2;
    int cy = oy + sy / 2;
    
    // some values as pointers for the audio plug-in call
    std::string legend;
    float *data = new float[2 * sx];
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
        cairo_surface_t *window_surface = cairo_get_target(c);
        pg->background = cairo_surface_create_similar(window_surface, 
                                  CAIRO_CONTENT_COLOR,
                                  widget->allocation.width,
                                  widget->allocation.height );
        pg->cache = cairo_surface_create_similar(window_surface, 
                                  CAIRO_CONTENT_COLOR,
                                  widget->allocation.width,
                                  widget->allocation.height );
        
        // ...and draw some bling bling onto it...
        ctx_back = cairo_create(pg->background);
        calf_phase_graph_draw_background(ctx_back, sx, sy, ox, oy);
        // ...and copy it to the cache
        ctx_cache = cairo_create(pg->cache); 
        calf_phase_graph_copy_surface(ctx_cache, pg->background, 1);
    } else {
        ctx_back = cairo_create(pg->background);
        ctx_cache = cairo_create(pg->cache); 
    }
    
    pg->source->get_phase_graph(&phase_buffer, &length, &mode, &use_fade,
                                &fade, &accuracy, &display);
    
    // process some values set by the plug-in
    accuracy *= 2;
    accuracy = 12 - accuracy;
    
    calf_phase_graph_copy_surface(ctx_cache, pg->background, use_fade ? fade : 1);
    
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
            float x = (-1.f)*_R * cos(_a);
            float y = _R * sin(_a);
            // mask the cached values
            switch(mode) {
                case 0:
                    // small dots
                    cairo_rectangle (ctx_cache, x * rad + cx, y * rad + cy, 1, 1);
                    break;
                case 1:
                    // medium dots
                    cairo_rectangle (ctx_cache, x * rad + cx - 0.25, y * rad + cy - 0.25, 1.5, 1.5);
                    break;
                case 2:
                    // big dots
                    cairo_rectangle (ctx_cache, x * rad + cx - 0.5, y * rad + cy - 0.5, 2, 2);
                    break;
                case 3:
                    // fields
                    if(i == 0) cairo_move_to(ctx_cache,
                        x * rad + cx, y * rad + cy);
                    else cairo_line_to(c,
                        x * rad + cx, y * rad + cy);
                    break;
                case 4:
                    // lines
                    if(i == 0) cairo_move_to(ctx_cache,
                        x * rad + cx, y * rad + cy);
                    else cairo_line_to(c,
                        x * rad + cx, y * rad + cy);
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
                cairo_set_line_width(c, 1);
                cairo_stroke(ctx_cache);
                break;
        }
    }
    
    calf_phase_graph_copy_surface(c, pg->cache, 1);
    
    cairo_destroy(c);
    cairo_destroy(ctx_back);
    cairo_destroy(ctx_cache);
    delete []data;
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
    GdkWindow *window = widget->window;

    int ox = widget->allocation.x + widget->allocation.width / 2 - self->size * 15;
    int oy = widget->allocation.y + widget->allocation.height / 2 - self->size * 10;
    int width = self->size * 30, height = self->size * 20;

    gdk_draw_pixbuf(GDK_DRAWABLE(widget->window), widget->style->fg_gc[0], CALF_TOGGLE_CLASS(GTK_OBJECT_GET_CLASS(widget))->toggle_image[self->size - 1], 0, height * floor(.5 + gtk_range_get_value(GTK_RANGE(widget))), ox, oy, width, height, GDK_RGB_DITHER_NORMAL, 0, 0);
    if (gtk_widget_is_focus(widget))
    {
        gtk_paint_focus(widget->style, window, GTK_STATE_NORMAL, NULL, widget, NULL, ox, oy, width, height);
    }

    return TRUE;
}

static void
calf_toggle_size_request (GtkWidget *widget,
                           GtkRequisition *requisition)
{
    g_assert(CALF_IS_TOGGLE(widget));

    CalfToggle *self = CALF_TOGGLE(widget);

    requisition->width  = 30 * self->size;
    requisition->height = 20 * self->size;
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
    GError *error = NULL;
    klass->toggle_image[0] = gdk_pixbuf_new_from_file(PKGLIBDIR "/toggle1_silver.png", &error);
    klass->toggle_image[1] = gdk_pixbuf_new_from_file(PKGLIBDIR "/toggle2_silver.png", &error);
    g_assert(klass->toggle_image != NULL);
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

GtkWidget *
calf_toggle_new()
{
    GtkAdjustment *adj = (GtkAdjustment *)gtk_adjustment_new(0, 0, 1, 1, 0, 0);
    return calf_toggle_new_with_adjustment(adj);
}

static gboolean calf_toggle_value_changed(gpointer obj)
{
    GtkWidget *widget = (GtkWidget *)obj;
    gtk_widget_queue_draw(widget);
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
