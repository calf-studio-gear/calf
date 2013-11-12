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
#include <calf/custom_ctl.h>
#include <gdk/gdkkeysyms.h>
#include <cairo/cairo.h>
#include <math.h>
#include <gdk/gdk.h>
#include <sys/time.h>

#include <iostream>

///////////////////////////////////////// phase graph ///////////////////////////////////////////////

static void
calf_phase_graph_copy_cache_to_window( cairo_surface_t *pg, cairo_t *c )
{
    cairo_save( c );
    cairo_set_source_surface( c, pg, 0,0 );
    cairo_paint( c );
    cairo_restore( c );
}

static gboolean
calf_phase_graph_expose (GtkWidget *widget, GdkEventExpose *event)
{
    g_assert(CALF_IS_PHASE_GRAPH(widget));

    CalfPhaseGraph *pg = CALF_PHASE_GRAPH(widget);
    //int ox = widget->allocation.x + 1, oy = widget->allocation.y + 1;
    int ox = 5, oy = 5, pad;
    int sx = widget->allocation.width - ox * 2, sy = widget->allocation.height - oy * 2;
    sx += sx % 2 - 1;
    sy += sy % 2 - 1;
    int rad = sx / 2 * 0.8;
    int cx = ox + sx / 2;
    int cy = oy + sy / 2;
    cairo_t *c = gdk_cairo_create(GDK_DRAWABLE(widget->window));
    GdkColor sc = { 0, 0, 0, 0 };

    bool cache_dirty = 0;
    bool fade_dirty = 0;

    if( pg->cache_surface == NULL ) {
        // looks like its either first call or the widget has been resized.
        // create the cache_surface.
        cairo_surface_t *window_surface = cairo_get_target( c );
        pg->cache_surface = cairo_surface_create_similar( window_surface, 
                                  CAIRO_CONTENT_COLOR,
                                  widget->allocation.width,
                                  widget->allocation.height );
        cache_dirty = 1;
    }
    if( pg->fade_surface == NULL ) {
        // looks like its either first call or the widget has been resized.
        // create the cache_surface.
        cairo_surface_t *window_surface = cairo_get_target( c );
        pg->fade_surface = cairo_surface_create_similar( window_surface, 
                                  CAIRO_CONTENT_COLOR,
                                  widget->allocation.width,
                                  widget->allocation.height );
        fade_dirty = 1;
    }

    cairo_select_font_face(c, "Bitstream Vera Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(c, 9);
    gdk_cairo_set_source_color(c, &sc);
    
    cairo_impl cimpl;
    cimpl.context = c;

    if (pg->source) {
        std::string legend;
        float *data = new float[2 * sx];
        GdkColor sc2 = { 0, (int)(0.35 * 65535), (int)(0.4 * 65535), (int)(0.2 * 65535) };

        if( cache_dirty ) {
            
            cairo_t *cache_cr = cairo_create( pg->cache_surface );
        
//            if(widget->style->bg_pixmap[0] == NULL) {
                cairo_set_source_rgb(cache_cr, 0, 0, 0);
//            } else {
//                gdk_cairo_set_source_pixbuf(cache_cr, GDK_PIXBUF(&style->bg_pixmap[GTK_STATE_NORMAL]), widget->allocation.x, widget->allocation.y + 20);
//            }
            cairo_paint(cache_cr);
            
            // outer (black)
            pad = 0;
            cairo_rectangle(cache_cr, pad, pad, sx + ox * 2 - pad * 2, sy + oy * 2 - pad * 2);
            cairo_set_source_rgb(cache_cr, 0, 0, 0);
            cairo_fill(cache_cr);
            
            // inner (bevel)
            pad = 1;
            cairo_rectangle(cache_cr, pad, pad, widget->allocation.width - pad * 2, widget->allocation.height - pad + 2);
            cairo_pattern_t *pat2 = cairo_pattern_create_linear (0, 0, 0, sy + oy * 2 - pad * 2);
            cairo_pattern_add_color_stop_rgba (pat2, 0, 0.23, 0.23, 0.23, 1);
            cairo_pattern_add_color_stop_rgba (pat2, 0.5, 0, 0, 0, 1);
            cairo_set_source (cache_cr, pat2);
            cairo_fill(cache_cr);
            cairo_pattern_destroy(pat2);
            
            cairo_rectangle(cache_cr, ox - 1, oy - 1, sx + 2, sy + 2);
            cairo_set_source_rgb (cache_cr, 0, 0, 0);
            cairo_fill(cache_cr);
            
            cairo_pattern_t *pt = cairo_pattern_create_linear(ox, oy, ox, sy);
            cairo_pattern_add_color_stop_rgb(pt, 0.0,     0.44,    0.44,    0.30);
            cairo_pattern_add_color_stop_rgb(pt, 0.04,   0.89,    0.99,    0.54);
            cairo_pattern_add_color_stop_rgb(pt, 0.4,     0.78,    0.89,    0.45);
            cairo_pattern_add_color_stop_rgb(pt, 0.400001,0.71,    0.82,    0.33);
            cairo_pattern_add_color_stop_rgb(pt, 1.0,     0.89,    1.00,    0.45);
            cairo_set_source (cache_cr, pt);
            cairo_rectangle(cache_cr, ox, oy, sx, sy);
            cairo_fill(cache_cr);
            
            // lights
            int div = 1;
            int light_w = sx;
            while(light_w / div > 300) 
                div += 1;
            int w = sx / div;
            cairo_rectangle(cache_cr, ox, oy, sx, sy);
            for(int i = 0; i < div; i ++) {
                cairo_pattern_t *pt = cairo_pattern_create_radial(
                    ox + w * i + w / 2.f, oy, 1,
                    ox + w * i + w / 2.f, ox + sy * 0.25, w / 2.f);
                cairo_pattern_add_color_stop_rgba (pt, 0, 1, 1, 0.8, 0.9);
                cairo_pattern_add_color_stop_rgba (pt, 1, 0.89, 1.00, 0.45, 0);
                cairo_set_source (cache_cr, pt);
                cairo_fill_preserve(cache_cr);
                pt = cairo_pattern_create_radial(
                    ox + w * i + w / 2.f, oy + sy, 1,
                    ox + w * i + w / 2.f, ox + sy * 0.75, w / 2.f);
                cairo_pattern_add_color_stop_rgba (pt, 0, 1, 1, 0.8, 0.9);
                cairo_pattern_add_color_stop_rgba (pt, 1, 0.89, 1.00, 0.45, 0);
                cairo_set_source (cache_cr, pt);
                cairo_fill_preserve(cache_cr);
            }
            
            gdk_cairo_set_source_color(cache_cr, &sc2);
            
            cairo_select_font_face(cache_cr, "Bitstream Vera Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
            cairo_set_font_size(cache_cr, 9);
            cairo_text_extents_t te;
            
            cairo_text_extents (cache_cr, "M", &te);
            cairo_move_to (cache_cr, cx + 5, oy + 12);
            cairo_show_text (cache_cr, "M");
            
            cairo_text_extents (cache_cr, "S", &te);
            cairo_move_to (cache_cr, ox + 5, cy - 5);
            cairo_show_text (cache_cr, "S");
            
            cairo_text_extents (cache_cr, "L", &te);
            cairo_move_to (cache_cr, ox + 18, oy + 12);
            cairo_show_text (cache_cr, "L");
            
            cairo_text_extents (cache_cr, "R", &te);
            cairo_move_to (cache_cr, ox + sx - 22, oy + 12);
            cairo_show_text (cache_cr, "R");
            
            cairo_impl cache_cimpl;
            cache_cimpl.context = cache_cr;

            // draw style elements here
            cairo_set_line_width(cache_cr, 1);
            
            cairo_move_to(cache_cr, ox, oy + sy * 0.5);
            cairo_line_to(cache_cr, ox + sx, oy + sy * 0.5);
            cairo_stroke(cache_cr);
            
            cairo_move_to(cache_cr, ox + sx * 0.5, oy);
            cairo_line_to(cache_cr, ox + sx * 0.5, oy + sy);
            cairo_stroke(cache_cr);
            
            cairo_set_source_rgba(cache_cr, 0, 0, 0, 0.2);
            cairo_move_to(cache_cr, ox, oy);
            cairo_line_to(cache_cr, ox + sx, oy + sy);
            cairo_stroke(cache_cr);
            
            cairo_move_to(cache_cr, ox, oy + sy);
            cairo_line_to(cache_cr, ox + sx, oy);
            cairo_stroke(cache_cr);
        }
        
        
        float * phase_buffer = 0;
        int length = 0;
        int mode = 2;
        float fade = 0.05;
        bool use_fade = true;
        int accuracy = 1;
        bool display = true;
        
        pg->source->get_phase_graph(&phase_buffer, &length, &mode, &use_fade, &fade, &accuracy, &display);
        
        fade *= 0.35;
        fade += 0.05;
        accuracy *= 2;
        accuracy = 12 - accuracy;
        
        cairo_t *cache_cr = cairo_create( pg->fade_surface );
        cairo_set_source_surface(cache_cr, pg->cache_surface, 0, 0);
        if(fade_dirty or !use_fade or ! display) {
            cairo_paint(cache_cr);
        } else {
            cairo_paint_with_alpha(cache_cr, fade);
        }
        
        if(display) {
            cairo_rectangle(cache_cr, ox, oy, sx, sy);
            cairo_clip(cache_cr);
            //gdk_cairo_set_source_color(cache_cr, &sc3);
            cairo_set_source_rgba(cache_cr, 0.15, 0.2, 0.0, 0.5);
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
                        cairo_rectangle (cache_cr, x * rad + cx, y * rad + cy, 1, 1);
                        break;
                    case 1:
                        // medium dots
                        cairo_rectangle (cache_cr, x * rad + cx - 0.25, y * rad + cy - 0.25, 1.5, 1.5);
                        break;
                    case 2:
                        // big dots
                        cairo_rectangle (cache_cr, x * rad + cx - 0.5, y * rad + cy - 0.5, 2, 2);
                        break;
                    case 3:
                        // fields
                        if(i == 0) cairo_move_to(cache_cr,
                            x * rad + cx, y * rad + cy);
                        else cairo_line_to(cache_cr,
                            x * rad + cx, y * rad + cy);
                        break;
                    case 4:
                        // lines
                        if(i == 0) cairo_move_to(cache_cr,
                            x * rad + cx, y * rad + cy);
                        else cairo_line_to(cache_cr,
                            x * rad + cx, y * rad + cy);
                        break;
                }
            }
            // draw
            switch(mode) {
                case 0:
                case 1:
                case 2:
                    cairo_fill (cache_cr);
                    break;
                case 3:
                    cairo_fill (cache_cr);
                    break;
                case 4:
                    cairo_set_line_width(cache_cr, 1);
                    cairo_stroke (cache_cr);
                    break;
            }
        }
        cairo_destroy( cache_cr );
        calf_phase_graph_copy_cache_to_window( pg->fade_surface, c );
        
        
//        cairo_set_line_width(c, 1);
//        for(int phase = 1; phase <= 2; phase++)
//        {
//            for(int gn=grid_n_save; legend = std::string(), cairo_set_source_rgba(c, 0, 0, 0, 0.6), lg->source->get_gridline(lg->source_id, gn, pos, vertical, legend, &cimpl); gn++)
//            {
//                calf_line_graph_draw_grid( c, legend, vertical, pos, phase, sx, sy );
//            }
//        }

//        gdk_cairo_set_source_color(c, &sc2);
//        cairo_set_line_join(c, CAIRO_LINE_JOIN_MITER);
//        cairo_set_line_width(c, 1);
//        for(int gn = graph_n; lg->source->get_graph(lg->source_id, gn, data, 2 * sx, &cimpl); gn++)
//        {
//            calf_line_graph_draw_graph( c, data, sx, sy );
//        }
        delete []data;
    }

    cairo_destroy(c);

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

    if( lg->cache_surface )
        cairo_surface_destroy( lg->cache_surface );
    lg->cache_surface = NULL;
    if( lg->fade_surface )
        cairo_surface_destroy( lg->fade_surface );
    lg->fade_surface = NULL;
    
    widget->allocation = *allocation;
    GtkAllocation &a = widget->allocation;
    if (a.width > a.height)
    {
        a.x += (a.width - a.height) / 2;
        a.width = a.height;
    }
    if (a.width < a.height)
    {
        a.y += (a.height - a.width) / 2;
        a.height = a.width;
    }
    parent_class->size_allocate( widget, &a );
}

static void
calf_phase_graph_class_init (CalfPhaseGraphClass *klass)
{
    // GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);
    widget_class->expose_event = calf_phase_graph_expose;
    widget_class->size_request = calf_phase_graph_size_request;
    widget_class->size_allocate = calf_phase_graph_size_allocate;
}

static void
calf_phase_graph_unrealize (GtkWidget *widget, CalfPhaseGraph *pg)
{
    if( pg->cache_surface )
        cairo_surface_destroy( pg->cache_surface );
    pg->cache_surface = NULL;
    if( pg->fade_surface )
        cairo_surface_destroy( pg->fade_surface );
    pg->fade_surface = NULL;
}

static void
calf_phase_graph_init (CalfPhaseGraph *self)
{
    GtkWidget *widget = GTK_WIDGET(self);
    widget->requisition.width = 40;
    widget->requisition.height = 40;
    self->cache_surface = NULL;
    self->fade_surface = NULL;
    g_signal_connect(GTK_OBJECT(widget), "unrealize", G_CALLBACK(calf_phase_graph_unrealize), (gpointer)self);
}

GtkWidget *
calf_phase_graph_new()
{
    return GTK_WIDGET( g_object_new (CALF_TYPE_PHASE_GRAPH, NULL ));
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
