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

static void
calf_line_graph_copy_cache_to_window( CalfLineGraph *lg, cairo_t *c )
{
    cairo_save( c );
    cairo_set_source_surface( c, lg->cache_surface, 0,0 );
    cairo_paint( c );
    cairo_restore( c );
}

static void
calf_line_graph_copy_window_to_cache( CalfLineGraph *lg, cairo_t *c )
{
    cairo_t *cache_cr = cairo_create( lg->cache_surface );
    cairo_surface_t *window_surface = cairo_get_target( c );
    cairo_set_source_surface( cache_cr, window_surface, 0,0 );
    cairo_paint( cache_cr );
    cairo_destroy( cache_cr );
}

static void
calf_line_graph_draw_grid( cairo_t *c, std::string &legend, bool vertical, float pos, int phase, int sx, int sy )
{
    int ox=5, oy=5;
    cairo_text_extents_t tx;
    if (!legend.empty())
        cairo_text_extents(c, legend.c_str(), &tx);
    if (vertical)
    {
        float x = floor(ox + pos * sx) + 0.5;
        if (phase == 1)
        {
            cairo_move_to(c, x, oy);
            cairo_line_to(c, x, oy + sy);
            cairo_stroke(c);
        }
        if (phase == 2 && !legend.empty()) {

            cairo_set_source_rgba(c, 0.0, 0.0, 0.0, 0.7);
            cairo_move_to(c, x - (tx.x_bearing + tx.width / 2.0) - 2, oy + sy - 2);
            cairo_show_text(c, legend.c_str());
        }
    }
    else
    {
        float y = floor(oy + sy / 2 - (sy / 2 - 1) * pos) + 0.5;
        if (phase == 1)
        {
            cairo_move_to(c, ox, y);
            cairo_line_to(c, ox + sx, y);
            cairo_stroke(c);
        }
        if (phase == 2 && !legend.empty()) {
            cairo_set_source_rgba(c, 0.0, 0.0, 0.0, 0.7);
            cairo_move_to(c, ox + sx - 4 - tx.width, y + tx.height/2);
            cairo_show_text(c, legend.c_str());
        }
    }
}

static void
calf_line_graph_draw_graph( cairo_t *c, float *data, int sx, int sy )
{
    int ox=5, oy=5;

    for (int i = 0; i < 2 * sx; i++)
    {
        int y = (int)(oy + sy / 2 - (sy / 2 - 1) * data[i]);
        //if (y < oy) y = oy;
        //if (y >= oy + sy) y = oy + sy - 1;
        if (i)
            cairo_line_to(c, ox + i * 0.5, y);
        else
            cairo_move_to(c, ox, y);
    }
    cairo_stroke(c);
}

static gboolean
calf_line_graph_expose (GtkWidget *widget, GdkEventExpose *event)
{
    g_assert(CALF_IS_LINE_GRAPH(widget));

    CalfLineGraph *lg = CALF_LINE_GRAPH(widget);
    //int ox = widget->allocation.x + 1, oy = widget->allocation.y + 1;
    int ox = 5, oy = 5, rad, pad;
    int sx = widget->allocation.width - ox * 2, sy = widget->allocation.height - oy * 2;

    cairo_t *c = gdk_cairo_create(GDK_DRAWABLE(widget->window));
    GtkStyle *style;
    style = gtk_widget_get_style(widget);
    GdkColor sc = { 0, 0, 0, 0 };

    bool cache_dirty = 0;

    if( lg->cache_surface == NULL ) {
        // looks like its either first call or the widget has been resized.
        // create the cache_surface.
        cairo_surface_t *window_surface = cairo_get_target( c );
        lg->cache_surface = cairo_surface_create_similar( window_surface, 
                                  CAIRO_CONTENT_COLOR,
                                  widget->allocation.width,
                                  widget->allocation.height );
        //cairo_set_source_surface( cache_cr, window_surface, 0,0 );
        //cairo_paint( cache_cr );
        
        cache_dirty = 1;
    }

    cairo_select_font_face(c, "Bitstream Vera Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(c, 9);
    gdk_cairo_set_source_color(c, &sc);
    
    cairo_impl cimpl;
    cimpl.context = c;

    if (lg->source) {


        float pos = 0;
        bool vertical = false;
        std::string legend;
        float *data = new float[2 * sx];
        GdkColor sc2 = { 0, 0.35 * 65535, 0.4 * 65535, 0.2 * 65535 };
        float x, y;
        int size = 0;
        GdkColor sc3 = { 0, 0.35 * 65535, 0.4 * 65535, 0.2 * 65535 };

        int graph_n, grid_n, dot_n, grid_n_save;

        int cache_graph_index, cache_dot_index, cache_grid_index;
        int gen_index = lg->source->get_changed_offsets( lg->source_id, lg->last_generation, cache_graph_index, cache_dot_index, cache_grid_index );

        if( cache_dirty || (gen_index != lg->last_generation) ) {
            
            cairo_t *cache_cr = cairo_create( lg->cache_surface );
        
//            if(widget->style->bg_pixmap[0] == NULL) {
                gdk_cairo_set_source_color(cache_cr,&style->bg[GTK_STATE_NORMAL]);
//            } else {
//                gdk_cairo_set_source_pixbuf(cache_cr, GDK_PIXBUF(&style->bg_pixmap[GTK_STATE_NORMAL]), widget->allocation.x, widget->allocation.y + 20);
//            }
            cairo_paint(cache_cr);
            
            // outer (light)
            pad = 0;
            rad = 6;
            cairo_arc(cache_cr, rad + pad, rad + pad, rad, 0, 2 * M_PI);
            cairo_arc(cache_cr, ox * 2 + sx - rad - pad, rad + pad, rad, 0, 2 * M_PI);
            cairo_arc(cache_cr, rad + pad, oy * 2 + sy - rad - pad, rad, 0, 2 * M_PI);
            cairo_arc(cache_cr, ox * 2 + sx - rad - pad, oy * 2 + sy - rad - pad, rad, 0, 2 * M_PI);
            cairo_rectangle(cache_cr, pad, rad + pad, sx + ox * 2 - pad * 2, sy + oy * 2 - rad * 2 - pad * 2);
            cairo_rectangle(cache_cr, rad + pad, pad, sx + ox * 2 - rad * 2 - pad * 2, sy + oy * 2 - pad * 2);
            cairo_pattern_t *pat2 = cairo_pattern_create_linear (0, 0, 0, sy + oy * 2 - pad * 2);
            cairo_pattern_add_color_stop_rgba (pat2, 0, 0, 0, 0, 0.3);
            cairo_pattern_add_color_stop_rgba (pat2, 1, 1, 1, 1, 0.6);
            cairo_set_source (cache_cr, pat2);
            cairo_fill(cache_cr);
            
            // inner (black)
            pad = 1;
            rad = 5;
            cairo_arc(cache_cr, rad + pad, rad + pad, rad, 0, 2 * M_PI);
            cairo_arc(cache_cr, ox * 2 + sx - rad - pad, rad + pad, rad, 0, 2 * M_PI);
            cairo_arc(cache_cr, rad + pad, oy * 2 + sy - rad - pad, rad, 0, 2 * M_PI);
            cairo_arc(cache_cr, ox * 2 + sx - rad - pad, oy * 2 + sy - rad - pad, rad, 0, 2 * M_PI);
            cairo_rectangle(cache_cr, pad, rad + pad, sx + ox * 2 - pad * 2, sy + oy * 2 - rad * 2 - pad * 2);
            cairo_rectangle(cache_cr, rad + pad, pad, sx + ox * 2 - rad * 2 - pad * 2, sy + oy * 2 - pad * 2);
            pat2 = cairo_pattern_create_linear (0, 0, 0, sy + oy * 2 - pad * 2);
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
            cairo_pattern_add_color_stop_rgb(pt, 0.025,   0.89,    0.99,    0.54);
            cairo_pattern_add_color_stop_rgb(pt, 0.4,     0.78,    0.89,    0.45);
            cairo_pattern_add_color_stop_rgb(pt, 0.400001,0.71,    0.82,    0.33);
            cairo_pattern_add_color_stop_rgb(pt, 1.0,     0.89,    1.00,    0.45);
            cairo_set_source (cache_cr, pt);
            cairo_rectangle(cache_cr, ox, oy, sx, sy);
            cairo_fill(cache_cr);
            
            cairo_select_font_face(cache_cr, "Bitstream Vera Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
            cairo_set_font_size(cache_cr, 9);
            
            cairo_impl cache_cimpl;
            cache_cimpl.context = cache_cr;

            lg->source->get_changed_offsets( lg->source_id, gen_index, cache_graph_index, cache_dot_index, cache_grid_index );
            lg->last_generation = gen_index;

            cairo_set_line_width(cache_cr, 1);
            for(int phase = 1; phase <= 2; phase++)
            {
                for(grid_n = 0; legend = std::string(), cairo_set_source_rgba(cache_cr, 0, 0, 0, 0.6), (grid_n<cache_grid_index) &&  lg->source->get_gridline(lg->source_id, grid_n, pos, vertical, legend, &cache_cimpl); grid_n++)
                {
                    calf_line_graph_draw_grid( cache_cr, legend, vertical, pos, phase, sx, sy );
                }
            }
            grid_n_save = grid_n;

            gdk_cairo_set_source_color(cache_cr, &sc2);
            cairo_set_line_join(cache_cr, CAIRO_LINE_JOIN_MITER);
            cairo_set_line_width(cache_cr, 1);
            for(graph_n = 0; (graph_n<cache_graph_index) && lg->source->get_graph(lg->source_id, graph_n, data, 2 * sx, &cache_cimpl); graph_n++)
            {
                calf_line_graph_draw_graph( cache_cr, data, sx, sy );
            }
            gdk_cairo_set_source_color(cache_cr, &sc3);
            for(dot_n = 0; (dot_n<cache_dot_index) && lg->source->get_dot(lg->source_id, dot_n, x, y, size = 3, &cache_cimpl); dot_n++)
            {
                int yv = (int)(oy + sy / 2 - (sy / 2 - 1) * y);
                cairo_arc(cache_cr, ox + x * sx, yv, size, 0, 2 * M_PI);
                cairo_fill(cache_cr);
            }

            // copy window to cache.
            cairo_destroy( cache_cr );
            calf_line_graph_copy_cache_to_window( lg, c );
        } else {
            grid_n_save = cache_grid_index;
            graph_n = cache_graph_index;
            dot_n = cache_dot_index;
            calf_line_graph_copy_cache_to_window( lg, c );
        }

        cairo_rectangle(c, ox, oy, sx, sy);
        cairo_clip(c);  
        
        cairo_set_line_width(c, 1);
        for(int phase = 1; phase <= 2; phase++)
        {
            for(int gn=grid_n_save; legend = std::string(), cairo_set_source_rgba(c, 0, 0, 0, 0.6), lg->source->get_gridline(lg->source_id, gn, pos, vertical, legend, &cimpl); gn++)
            {
                calf_line_graph_draw_grid( c, legend, vertical, pos, phase, sx, sy );
            }
        }

        gdk_cairo_set_source_color(c, &sc2);
        cairo_set_line_join(c, CAIRO_LINE_JOIN_MITER);
        cairo_set_line_width(c, 1);
        for(int gn = graph_n; lg->source->get_graph(lg->source_id, gn, data, 2 * sx, &cimpl); gn++)
        {
            calf_line_graph_draw_graph( c, data, sx, sy );
        }
        gdk_cairo_set_source_color(c, &sc3);
        for(int gn = dot_n; lg->source->get_dot(lg->source_id, gn, x, y, size = 3, &cimpl); gn++)
        {
            int yv = (int)(oy + sy / 2 - (sy / 2 - 1) * y);
            cairo_arc(c, ox + x * sx, yv, size, 0, 2 * M_PI);
            cairo_fill(c);
        }
        delete []data;
    }

    cairo_destroy(c);

    // printf("exposed %p %dx%d %d+%d\n", widget->window, event->area.x, event->area.y, event->area.width, event->area.height);

    return TRUE;
}

void calf_line_graph_set_square(CalfLineGraph *graph, bool is_square)
{
    g_assert(CALF_IS_LINE_GRAPH(graph));
    graph->is_square = is_square;
}

int calf_line_graph_update_if(CalfLineGraph *graph, int last_drawn_generation)
{
    g_assert(CALF_IS_LINE_GRAPH(graph));
    int generation = last_drawn_generation;
    if (graph->source)
    {
        int subgraph, dot, gridline;
        generation = graph->source->get_changed_offsets(graph->source_id, generation, subgraph, dot, gridline);
        if (subgraph == INT_MAX && dot == INT_MAX && gridline == INT_MAX && generation == last_drawn_generation)
            return generation;
        gtk_widget_queue_draw(GTK_WIDGET(graph));
    }
    return generation;
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
    CalfLineGraph *lg = CALF_LINE_GRAPH(widget);

    GtkWidgetClass *parent_class = (GtkWidgetClass *) g_type_class_peek_parent( CALF_LINE_GRAPH_GET_CLASS( lg ) );

    if( lg->cache_surface )
        cairo_surface_destroy( lg->cache_surface );
    lg->cache_surface = NULL;
    
    widget->allocation = *allocation;
    GtkAllocation &a = widget->allocation;
    if (lg->is_square)
    {
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
    }
    parent_class->size_allocate( widget, &a );
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
    widget->requisition.width = 40;
    widget->requisition.height = 40;
    self->cache_surface = NULL;
    self->last_generation = 0;
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

///////////////////////////////////////// vu meter ///////////////////////////////////////////////

static gboolean
calf_vumeter_expose (GtkWidget *widget, GdkEventExpose *event)
{
    g_assert(CALF_IS_VUMETER(widget));


    CalfVUMeter *vu = CALF_VUMETER(widget);
    GtkStyle	*style;
    
    int ox = 4, oy = 3, led = 3, inner = 1, rad, pad;
    int sx = widget->allocation.width - (ox * 2) - ((widget->allocation.width - inner * 2 - ox * 2) % led) - 1, sy = widget->allocation.height - (oy * 2);
    style = gtk_widget_get_style(widget);
    cairo_t *c = gdk_cairo_create(GDK_DRAWABLE(widget->window));

    if( vu->cache_surface == NULL ) {
        // looks like its either first call or the widget has been resized.
        // create the cache_surface.
        cairo_surface_t *window_surface = cairo_get_target( c );
        vu->cache_surface = cairo_surface_create_similar( window_surface, 
                                  CAIRO_CONTENT_COLOR,
                                  widget->allocation.width,
                                  widget->allocation.height );

        // And render the meterstuff again.

        cairo_t *cache_cr = cairo_create( vu->cache_surface );
        
        // theme background for reduced width and round borders
//        if(widget->style->bg_pixmap[0] == NULL) {
            gdk_cairo_set_source_color(cache_cr,&style->bg[GTK_STATE_NORMAL]);
//        } else {
//            gdk_cairo_set_source_pixbuf(cache_cr, GDK_PIXBUF(widget->style->bg_pixmap[0]), widget->allocation.x, widget->allocation.y + 20);
//        }
        cairo_paint(cache_cr);
        
        // outer (light)
        pad = 0;
        rad = 6;
        cairo_arc(cache_cr, rad + pad, rad + pad, rad, 0, 2 * M_PI);
        cairo_arc(cache_cr, ox * 2 + sx - rad - pad, rad + pad, rad, 0, 2 * M_PI);
        cairo_arc(cache_cr, rad + pad, oy * 2 + sy - rad - pad, rad, 0, 2 * M_PI);
        cairo_arc(cache_cr, ox * 2 + sx - rad - pad, oy * 2 + sy - rad - pad, rad, 0, 2 * M_PI);
        cairo_rectangle(cache_cr, pad, rad + pad, sx + ox * 2 - pad * 2, sy + oy * 2 - rad * 2 - pad * 2);
        cairo_rectangle(cache_cr, rad + pad, pad, sx + ox * 2 - rad * 2 - pad * 2, sy + oy * 2 - pad * 2);
        cairo_pattern_t *pat2 = cairo_pattern_create_linear (0, 0, 0, sy + oy * 2 - pad * 2);
        cairo_pattern_add_color_stop_rgba (pat2, 0, 0, 0, 0, 0.3);
        cairo_pattern_add_color_stop_rgba (pat2, 1, 1, 1, 1, 0.6);
        cairo_set_source (cache_cr, pat2);
        cairo_fill(cache_cr);
        
        // inner (black)
        pad = 1;
        rad = 5;
        cairo_arc(cache_cr, rad + pad, rad + pad, rad, 0, 2 * M_PI);
        cairo_arc(cache_cr, ox * 2 + sx - rad - pad, rad + pad, rad, 0, 2 * M_PI);
        cairo_arc(cache_cr, rad + pad, oy * 2 + sy - rad - pad, rad, 0, 2 * M_PI);
        cairo_arc(cache_cr, ox * 2 + sx - rad - pad, oy * 2 + sy - rad - pad, rad, 0, 2 * M_PI);
        cairo_rectangle(cache_cr, pad, rad + pad, sx + ox * 2 - pad * 2, sy + oy * 2 - rad * 2 - pad * 2);
        cairo_rectangle(cache_cr, rad + pad, pad, sx + ox * 2 - rad * 2 - pad * 2, sy + oy * 2 - pad * 2);
        pat2 = cairo_pattern_create_linear (0, 0, 0, sy + oy * 2 - pad * 2);
        cairo_pattern_add_color_stop_rgba (pat2, 0, 0.23, 0.23, 0.23, 1);
        cairo_pattern_add_color_stop_rgba (pat2, 0.5, 0, 0, 0, 1);
        cairo_set_source (cache_cr, pat2);
        //cairo_set_source_rgb(cache_cr, 0, 0, 0);
        cairo_fill(cache_cr);
        cairo_pattern_destroy(pat2);
        
        cairo_rectangle(cache_cr, ox, oy, sx, sy);
        cairo_set_source_rgb (cache_cr, 0, 0, 0);
        cairo_fill(cache_cr);
        
        cairo_set_line_width(cache_cr, 1);

        for (int x = ox + inner; x <= ox + sx - led; x += led)
        {
            float ts = (x - ox) * 1.0 / sx;
            float r = 0.f, g = 0.f, b = 0.f;
            switch(vu->mode)
            {
            case VU_STANDARD:
            default:
                if (ts < 0.75)
                r = ts / 0.75, g = 0.5 + ts * 0.66, b = 1 - ts / 0.75;
                else
                r = 1, g = 1 - (ts - 0.75) / 0.25, b = 0;
                //                if (vu->value < ts || vu->value <= 0)
                //                    r *= 0.5, g *= 0.5, b *= 0.5;
                break;
            case VU_MONOCHROME_REVERSE:
                r = 0, g = 170.0 / 255.0, b = 1;
                //                if (!(vu->value < ts) || vu->value >= 1.0)
                //                    r *= 0.5, g *= 0.5, b *= 0.5;
                break;
            case VU_MONOCHROME:
                r = 0, g = 170.0 / 255.0, b = 1;
                //                if (vu->value < ts || vu->value <= 0)
                //                    r *= 0.5, g *= 0.5, b *= 0.5;
                break;
            }
            GdkColor sc2 = { 0, (guint16)(65535 * r + 0.2), (guint16)(65535 * g), (guint16)(65535 * b) };
            GdkColor sc3 = { 0, (guint16)(65535 * r * 0.7), (guint16)(65535 * g * 0.7), (guint16)(65535 * b * 0.7) };
            gdk_cairo_set_source_color(cache_cr, &sc2);
            cairo_move_to(cache_cr, x + 0.5, oy + 1);
            cairo_line_to(cache_cr, x + 0.5, oy + sy - inner);
            cairo_stroke(cache_cr);
            gdk_cairo_set_source_color(cache_cr, &sc3);
            cairo_move_to(cache_cr, x + 1.5, oy + sy - inner);
            cairo_line_to(cache_cr, x + 1.5, oy + 1);
            cairo_stroke(cache_cr);
        }
        // create blinder pattern
        vu->pat = cairo_pattern_create_linear (ox, oy, ox, ox + sy);
        cairo_pattern_add_color_stop_rgba (vu->pat, 0, 0.5, 0.5, 0.5, 0.6);
        cairo_pattern_add_color_stop_rgba (vu->pat, 0.4, 0, 0, 0, 0.6);
        cairo_pattern_add_color_stop_rgba (vu->pat, 0.401, 0, 0, 0, 0.8);
        cairo_pattern_add_color_stop_rgba (vu->pat, 1, 0, 0, 0, 0.6);
        cairo_destroy( cache_cr );
    }

    cairo_set_source_surface( c, vu->cache_surface, 0,0 );
    cairo_paint( c );
    cairo_set_source( c, vu->pat );
    
    // get microseconds
    timeval tv;
    gettimeofday(&tv, 0);
    long time = tv.tv_sec * 1000 * 1000 + tv.tv_usec;
    
    // limit to 1.f
    float value_orig = vu->value > 1.f ? 1.f : vu->value;
    value_orig = value_orig < 0.f ? 0.f : value_orig;
    float value = 0.f;
    
    // falloff?
    if(vu->vumeter_falloff > 0.f and vu->mode != VU_MONOCHROME_REVERSE) {
        // fall off a bit
        float s = ((float)(time - vu->last_falltime) / 1000000.0);
        float m = vu->last_falloff * s * vu->vumeter_falloff;
        vu->last_falloff -= m;
        // new max value?
        if(value_orig > vu->last_falloff) {
            vu->last_falloff = value_orig;
        }
        value = vu->last_falloff;
        vu->last_falltime = time;
        vu->falling = vu->last_falloff > 0.000001;
    } else {
        // falloff disabled
        vu->last_falloff = 0.f;
        vu->last_falltime = 0.f;
        value = value_orig;
    }
    
    if(vu->vumeter_hold > 0.0) {
        // peak hold timer
        if(time - (long)(vu->vumeter_hold * 1000 * 1000) > vu->last_hold) {
            // time's up, reset
            vu->last_value = value;
            vu->last_hold = time;
            vu->holding = false;
        }
        if( vu->mode == VU_MONOCHROME_REVERSE ) {
            if(value < vu->last_value) {
                // value is above peak hold
                vu->last_value = value;
                vu->last_hold = time;
                vu->holding = true;
            }
            int hpx = round(vu->last_value * (sx - 2 * inner));
            hpx = hpx + (1 - (hpx + inner) % led);
            int vpx = round((1 - value) * (sx - 2 * inner));
            vpx = vpx + (1 - (vpx + inner) % led);
            int widthA = std::min(led + hpx, (sx - 2 * inner));
            int widthB = std::min(std::max((sx - 2 * inner) - vpx - led - hpx, 0), (sx - 2 * inner));
            cairo_rectangle( c, ox + inner, oy + inner, hpx, sy - 2 * inner);
            cairo_rectangle( c, ox + inner + widthA, oy + inner, widthB, sy - 2 * inner);
        } else {
            if(value > vu->last_value) {
                // value is above peak hold
                vu->last_value = value;
                vu->last_hold = time;
                vu->holding = true;
            }
            int hpx = round((1 - vu->last_value) * (sx - 2 * inner));
            hpx = hpx + (1 - (hpx + inner) % led);
            int vpx = round(value * (sx - 2 * inner));
            vpx = vpx + (1 - (vpx + inner) % led);
            int width = std::min(std::max((sx - 2 * inner) - vpx - led - hpx, 0), (sx - 2 * inner));
            cairo_rectangle( c, ox + inner + vpx, oy + inner, width, sy - 2 * inner);
            cairo_rectangle( c, ox + inner + (sx - 2 * inner) - hpx, oy + inner, hpx, sy - 2 * inner);
        }
    } else {
        // darken normally
        if( vu->mode == VU_MONOCHROME_REVERSE )
            cairo_rectangle( c, ox + inner,oy + inner, value * (sx - 2 * inner), sy - 2 * inner);
        else
            cairo_rectangle( c, ox + inner + value * (sx - 2 * inner), oy + inner, (sx - 2 * inner) * (1 - value), sy - 2 * inner );
    }
    cairo_fill( c );
    cairo_destroy(c);
    //gtk_paint_shadow(widget->style, widget->window, GTK_STATE_NORMAL, GTK_SHADOW_IN, NULL, widget, NULL, ox - 2, oy - 2, sx + 4, sy + 4);
    //printf("exposed %p %d+%d\n", widget->window, widget->allocation.x, widget->allocation.y);

    return TRUE;
}

static void
calf_vumeter_size_request (GtkWidget *widget,
                           GtkRequisition *requisition)
{
    g_assert(CALF_IS_VUMETER(widget));

    requisition->width = 50;
    requisition->height = 18;
}

static void
calf_vumeter_size_allocate (GtkWidget *widget,
                           GtkAllocation *allocation)
{
    g_assert(CALF_IS_VUMETER(widget));
    CalfVUMeter *vu = CALF_VUMETER(widget);
    
    GtkWidgetClass *parent_class = (GtkWidgetClass *) g_type_class_peek_parent( CALF_VUMETER_GET_CLASS( vu ) );

    parent_class->size_allocate( widget, allocation );

    if( vu->cache_surface )
        cairo_surface_destroy( vu->cache_surface );
    vu->cache_surface = NULL;
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
    //GTK_WIDGET_SET_FLAGS (widget, GTK_NO_WINDOW);
    widget->requisition.width = 50;
    widget->requisition.height = 18;
    self->cache_surface = NULL;
    self->falling = false;
    self->holding = false;
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

extern void calf_vumeter_set_value(CalfVUMeter *meter, float value)
{
    if (value != meter->value or meter->holding or meter->falling)
    {
        meter->value = value;
        gtk_widget_queue_draw(GTK_WIDGET(meter));
    }
}

extern float calf_vumeter_get_value(CalfVUMeter *meter)
{
    return meter->value;
}

extern void calf_vumeter_set_mode(CalfVUMeter *meter, CalfVUMeterMode mode)
{
    if (mode != meter->mode)
    {
        meter->mode = mode;
        if(mode == VU_MONOCHROME_REVERSE) {
            meter->value = 1.f;
            meter->last_value = 1.f;
        } else {
            meter->value = 0.f;
            meter->last_value = 0.f;
        }
        meter->vumeter_falloff = 0.f;
        meter->last_falloff = (long)0;
        meter->last_hold = (long)0;
        gtk_widget_queue_draw(GTK_WIDGET(meter));
    }
}

extern CalfVUMeterMode calf_vumeter_get_mode(CalfVUMeter *meter)
{
    return meter->mode;
}

extern void calf_vumeter_set_falloff(CalfVUMeter *meter, float value)
{
    if (value != meter->vumeter_falloff)
    {
        meter->vumeter_falloff = value;
        gtk_widget_queue_draw(GTK_WIDGET(meter));
    }
}

extern float calf_vumeter_get_falloff(CalfVUMeter *meter)
{
    return meter->vumeter_falloff;
}

extern void calf_vumeter_set_hold(CalfVUMeter *meter, float value)
{
    if (value != meter->vumeter_falloff)
    {
        meter->vumeter_hold = value;
        gtk_widget_queue_draw(GTK_WIDGET(meter));
    }
}

extern float calf_vumeter_get_hold(CalfVUMeter *meter)
{
    return meter->vumeter_hold;
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
    self->last_dz = -1.f;
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

    gdk_draw_pixbuf(GDK_DRAWABLE(widget->window), widget->style->fg_gc[0], CALF_TOGGLE_CLASS(GTK_OBJECT_GET_CLASS(widget))->toggle_image[self->size - 1], 0, height * gtk_range_get_value(GTK_RANGE(widget)), ox, oy, width, height, GDK_RGB_DITHER_NORMAL, 0, 0);
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
    klass->toggle_image[0] = gdk_pixbuf_new_from_file(PKGLIBDIR "/toggle1.png", &error);
    klass->toggle_image[1] = gdk_pixbuf_new_from_file(PKGLIBDIR "/toggle2.png", &error);
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
        gtk_signal_connect(GTK_OBJECT(widget), "value-changed", G_CALLBACK(calf_toggle_value_changed), widget);
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

///////////////////////////////////////// tube ///////////////////////////////////////////////

static gboolean
calf_tube_expose (GtkWidget *widget, GdkEventExpose *event)
{
    g_assert(CALF_IS_TUBE(widget));
    
    CalfTube  *self   = CALF_TUBE(widget);
    GdkWindow *window = widget->window;
    GtkStyle  *style  = gtk_widget_get_style(widget);
    cairo_t *c = gdk_cairo_create(GDK_DRAWABLE(window));
    
    int ox = 4, oy = 4, inner = 1, rad, pad;
    int sx = widget->allocation.width - (ox * 2), sy = widget->allocation.height - (oy * 2);
    
    if( self->cache_surface == NULL ) {
        // looks like its either first call or the widget has been resized.
        // create the cache_surface.
        cairo_surface_t *window_surface = cairo_get_target( c );
        self->cache_surface = cairo_surface_create_similar( window_surface, 
                                  CAIRO_CONTENT_COLOR,
                                  widget->allocation.width,
                                  widget->allocation.height );

        // And render the meterstuff again.
        cairo_t *cache_cr = cairo_create( self->cache_surface );
        // theme background for reduced width and round borders
//        if(widget->style->bg_pixmap[0] == NULL) {
            gdk_cairo_set_source_color(cache_cr,&style->bg[GTK_STATE_NORMAL]);
//        } else {
//            gdk_cairo_set_source_pixbuf(cache_cr, GDK_PIXBUF(widget->style->bg_pixmap[0]), widget->allocation.x, widget->allocation.y + 20);
//        }
        cairo_paint(cache_cr);
        
        // outer (light)
        pad = 0;
        rad = 6;
        cairo_arc(cache_cr, rad + pad, rad + pad, rad, 0, 2 * M_PI);
        cairo_arc(cache_cr, ox * 2 + sx - rad - pad, rad + pad, rad, 0, 2 * M_PI);
        cairo_arc(cache_cr, rad + pad, oy * 2 + sy - rad - pad, rad, 0, 2 * M_PI);
        cairo_arc(cache_cr, ox * 2 + sx - rad - pad, oy * 2 + sy - rad - pad, rad, 0, 2 * M_PI);
        cairo_rectangle(cache_cr, pad, rad + pad, sx + ox * 2 - pad * 2, sy + oy * 2 - rad * 2 - pad * 2);
        cairo_rectangle(cache_cr, rad + pad, pad, sx + ox * 2 - rad * 2 - pad * 2, sy + oy * 2 - pad * 2);
        cairo_pattern_t *pat2 = cairo_pattern_create_linear (0, 0, 0, sy + oy * 2 - pad * 2);
        cairo_pattern_add_color_stop_rgba (pat2, 0, 0, 0, 0, 0.3);
        cairo_pattern_add_color_stop_rgba (pat2, 1, 1, 1, 1, 0.6);
        cairo_set_source (cache_cr, pat2);
        cairo_fill(cache_cr);
        
        // inner (black)
        pad = 1;
        rad = 5;
        cairo_arc(cache_cr, rad + pad, rad + pad, rad, 0, 2 * M_PI);
        cairo_arc(cache_cr, ox * 2 + sx - rad - pad, rad + pad, rad, 0, 2 * M_PI);
        cairo_arc(cache_cr, rad + pad, oy * 2 + sy - rad - pad, rad, 0, 2 * M_PI);
        cairo_arc(cache_cr, ox * 2 + sx - rad - pad, oy * 2 + sy - rad - pad, rad, 0, 2 * M_PI);
        cairo_rectangle(cache_cr, pad, rad + pad, sx + ox * 2 - pad * 2, sy + oy * 2 - rad * 2 - pad * 2);
        cairo_rectangle(cache_cr, rad + pad, pad, sx + ox * 2 - rad * 2 - pad * 2, sy + oy * 2 - pad * 2);
        pat2 = cairo_pattern_create_linear (0, 0, 0, sy + oy * 2 - pad * 2);
        cairo_pattern_add_color_stop_rgba (pat2, 0, 0.23, 0.23, 0.23, 1);
        cairo_pattern_add_color_stop_rgba (pat2, 0.5, 0, 0, 0, 1);
        cairo_set_source (cache_cr, pat2);
        //cairo_set_source_rgb(cache_cr, 0, 0, 0);
        cairo_fill(cache_cr);
        cairo_pattern_destroy(pat2);
        
        cairo_rectangle(cache_cr, ox, oy, sx, sy);
        cairo_set_source_rgb (cache_cr, 0, 0, 0);
        cairo_fill(cache_cr);
        
        cairo_surface_t *image;
        switch(self->direction) {
            case 1:
                // vertical
                switch(self->size) {
                    default:
                    case 1:
                        image = cairo_image_surface_create_from_png (PKGLIBDIR "tubeV1.png");
                        break;
                    case 2:
                        image = cairo_image_surface_create_from_png (PKGLIBDIR "tubeV2.png");
                        break;
                }
                break;
            default:
            case 2:
                // horizontal
                switch(self->size) {
                    default:
                    case 1:
                        image = cairo_image_surface_create_from_png (PKGLIBDIR "tubeH1.png");
                        break;
                    case 2:
                        image = cairo_image_surface_create_from_png (PKGLIBDIR "tubeH2.png");
                        break;
                }
                break;
        }
        cairo_set_source_surface (cache_cr, image, widget->allocation.width / 2 - sx / 2 + inner, widget->allocation.height / 2 - sy / 2 + inner);
        cairo_paint (cache_cr);
        cairo_surface_destroy (image);
        cairo_destroy( cache_cr );
    }
    
    cairo_set_source_surface( c, self->cache_surface, 0,0 );
    cairo_paint( c );
    
    // get microseconds
    timeval tv;
    gettimeofday(&tv, 0);
    long time = tv.tv_sec * 1000 * 1000 + tv.tv_usec;
    
    // limit to 1.f
    float value_orig = self->value > 1.f ? 1.f : self->value;
    value_orig = value_orig < 0.f ? 0.f : value_orig;
    float value = 0.f;
    
    float s = ((float)(time - self->last_falltime) / 1000000.0);
    float m = self->last_falloff * s * 2.5;
    self->last_falloff -= m;
    // new max value?
    if(value_orig > self->last_falloff) {
        self->last_falloff = value_orig;
    }
    value = self->last_falloff;
    self->last_falltime = time;
    self->falling = self->last_falloff > 0.000001;
    cairo_pattern_t *pat;
    // draw upper light
    switch(self->direction) {
        case 1:
            // vertical
            cairo_arc(c, ox + sx * 0.5, oy + sy * 0.2, sx, 0, 2 * M_PI);
            pat = cairo_pattern_create_radial (ox + sx * 0.5, oy + sy * 0.2, 3, ox + sx * 0.5, oy + sy * 0.2, sx);
            break;
        default:
        case 2:
            // horizontal
            cairo_arc(c, ox + sx * 0.8, oy + sy * 0.5, sy, 0, 2 * M_PI);
            pat = cairo_pattern_create_radial (ox + sx * 0.8, oy + sy * 0.5, 3, ox + sx * 0.8, oy + sy * 0.5, sy);
            break;
    }
    cairo_pattern_add_color_stop_rgba (pat, 0,    1,    1,    1,    value);
    cairo_pattern_add_color_stop_rgba (pat, 0.3,  1,   0.8,  0.3, value * 0.4);
    cairo_pattern_add_color_stop_rgba (pat, 0.31, 0.9, 0.5,  0.1,  value * 0.5);
    cairo_pattern_add_color_stop_rgba (pat, 1,    0.0, 0.2,  0.7,  0);
    cairo_set_source (c, pat);
    cairo_fill(c);
    // draw lower light
    switch(self->direction) {
        case 1:
            // vertical
            cairo_arc(c, ox + sx * 0.5, oy + sy * 0.75, sx / 2, 0, 2 * M_PI);
            pat = cairo_pattern_create_radial (ox + sx * 0.5, oy + sy * 0.75, 2, ox + sx * 0.5, oy + sy * 0.75, sx / 2);
            break;
        default:
        case 2:
            // horizontal
            cairo_arc(c, ox + sx * 0.25, oy + sy * 0.5, sy / 2, 0, 2 * M_PI);
            pat = cairo_pattern_create_radial (ox + sx * 0.25, oy + sy * 0.5, 2, ox + sx * 0.25, oy + sy * 0.5, sy / 2);
            break;
    }
    cairo_pattern_add_color_stop_rgba (pat, 0,    1,    1,    1,    value);
    cairo_pattern_add_color_stop_rgba (pat, 0.3,  1,   0.8,  0.3, value * 0.4);
    cairo_pattern_add_color_stop_rgba (pat, 0.31, 0.9, 0.5,  0.1,  value * 0.5);
    cairo_pattern_add_color_stop_rgba (pat, 1,    0.0, 0.2,  0.7,  0);
    cairo_set_source (c, pat);
    cairo_fill(c);
    cairo_destroy(c);
    return TRUE;
}

static void
calf_tube_size_request (GtkWidget *widget,
                           GtkRequisition *requisition)
{
    g_assert(CALF_IS_TUBE(widget));

    CalfTube *self = CALF_TUBE(widget);
    switch(self->direction) {
        case 1:
            switch(self->size) {
                case 1:
                    widget->requisition.width = 82;
                    widget->requisition.height = 130;
                    break;
                default:
                case 2:
                    widget->requisition.width = 130;
                    widget->requisition.height = 210;
                    break;
            }
            break;
        default:
        case 2:
            switch(self->size) {
                case 1:
                    widget->requisition.width = 130;
                    widget->requisition.height = 82;
                    break;
                default:
                case 2:
                    widget->requisition.width = 210;
                    widget->requisition.height = 130;
                    break;
            }
            break;
    }
}

static void
calf_tube_size_allocate (GtkWidget *widget,
                           GtkAllocation *allocation)
{
    g_assert(CALF_IS_TUBE(widget));
    CalfTube *tube = CALF_TUBE(widget);
    
    GtkWidgetClass *parent_class = (GtkWidgetClass *) g_type_class_peek_parent( CALF_TUBE_GET_CLASS( tube ) );

    parent_class->size_allocate( widget, allocation );

    if( tube->cache_surface )
        cairo_surface_destroy( tube->cache_surface );
    tube->cache_surface = NULL;
}

static void
calf_tube_class_init (CalfTubeClass *klass)
{
    // GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);
    widget_class->expose_event = calf_tube_expose;
    widget_class->size_request = calf_tube_size_request;
    widget_class->size_allocate = calf_tube_size_allocate;
}

static void
calf_tube_init (CalfTube *self)
{
    GtkWidget *widget = GTK_WIDGET(self);
    GTK_WIDGET_SET_FLAGS (GTK_WIDGET(self), GTK_CAN_FOCUS);
    switch(self->direction) {
        case 1:
            switch(self->size) {
                case 1:
                    widget->requisition.width = 82;
                    widget->requisition.height = 130;
                    break;
                default:
                case 2:
                    widget->requisition.width = 130;
                    widget->requisition.height = 210;
                    break;
            }
            break;
        default:
        case 2:
            switch(self->size) {
                case 1:
                    widget->requisition.width = 130;
                    widget->requisition.height = 82;
                    break;
                default:
                case 2:
                    widget->requisition.width = 210;
                    widget->requisition.height = 130;
                    break;
            }
            break;
    }
    self->falling = false;
    self->cache_surface = NULL;
}

GtkWidget *
calf_tube_new()
{
    GtkWidget *widget = GTK_WIDGET( g_object_new (CALF_TYPE_TUBE, NULL ));
    return widget;
}

extern void calf_tube_set_value(CalfTube *tube, float value)
{
    if (value != tube->value or tube->falling)
    {
        tube->value = value;
        gtk_widget_queue_draw(GTK_WIDGET(tube));
    }
}

GType
calf_tube_get_type (void)
{
    static GType type = 0;
    if (!type) {
        
        static const GTypeInfo type_info = {
            sizeof(CalfTubeClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            (GClassInitFunc)calf_tube_class_init,
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof(CalfTube),
            0,    /* n_preallocs */
            (GInstanceInitFunc)calf_tube_init
        };
        
        for (int i = 0; ; i++) {
            char *name = g_strdup_printf("CalfTube%u%d", 
                ((unsigned int)(intptr_t)calf_tube_class_init) >> 16, i);
            if (g_type_from_name(name)) {
                free(name);
                continue;
            }
            type = g_type_register_static( GTK_TYPE_DRAWING_AREA,
                                           name,
                                           &type_info,
                                           (GTypeFlags)0);
            free(name);
            break;
        }
    }
    return type;
}
