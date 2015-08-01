/* Calf DSP Library
 * A few useful widgets - a line graph, a knob, a tube - Panama!
 *
 * Copyright (C) 2008-2010 Krzysztof Foltman, Torben Hohn, Markus
 * Schmidt and others
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
 * Boston, MA 02111-1307, USA.
 */
 
#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#ifndef __CALF_CTL_LINEGRAPH
#define __CALF_CTL_LINEGRAPH

#include <gtk/gtk.h>
#include <calf/giface.h>
#include <calf/gui.h>

G_BEGIN_DECLS

#define CALF_TYPE_LINE_GRAPH           (calf_line_graph_get_type())
#define CALF_LINE_GRAPH(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), CALF_TYPE_LINE_GRAPH, CalfLineGraph))
#define CALF_IS_LINE_GRAPH(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CALF_TYPE_LINE_GRAPH))
#define CALF_LINE_GRAPH_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST ((klass),  CALF_TYPE_LINE_GRAPH, CalfLineGraphClass))
#define CALF_IS_LINE_GRAPH_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE ((klass),  CALF_TYPE_LINE_GRAPH))
#define CALF_LINE_GRAPH_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj),  CALF_TYPE_LINE_GRAPH, CalfLineGraphClass))

struct FreqHandle
{
    bool active;
    int dimensions;
    int style;
    char *label;

    int param_active_no;
    int param_x_no;
    int param_y_no;
    int param_z_no;
    double value_x;
    double value_y;
    double value_z;
    double last_value_x;
    double last_value_y;
    double last_value_z;
    double default_value_x;
    double default_value_y;
    double default_value_z;
    double pos_x;
    double pos_y;
    double pos_z;
    calf_plugins::parameter_properties props_z;
    float left_bound;
    float right_bound;
    gpointer data;

    inline bool is_active() { return (param_active_no < 0 || active); }
};

#define FREQ_HANDLES 32
#define HANDLE_WIDTH 20.0

struct CalfLineGraph
{
    static const int debug = 0; // 0 - 3
    
    GtkEventBox parent;
    const calf_plugins::line_graph_iface *source;
    int source_id;
    bool force_cache;
    bool force_redraw;
    int recreate_surfaces;
    bool is_square;
    float fade;
    int mode, movesurf;
    int generation;
    unsigned int layers;
    int pad_x, pad_y;
    int size_x, size_y;
    int x, y;
    float zoom, offset;
    int param_zoom, param_offset;
    
    cairo_surface_t *background_surface;
    cairo_surface_t *grid_surface;
    cairo_surface_t *cache_surface;
    cairo_surface_t *moving_surface[2];
    cairo_surface_t *handles_surface;
    cairo_surface_t *realtime_surface;

    // crosshairs and FreqHandles
    gdouble mouse_x, mouse_y;
    bool use_crosshairs;
    bool crosshairs_active;

    int freqhandles;
    bool use_freqhandles_buttons;
    bool enforce_handle_order;
    float min_handle_distance;
    int handle_grabbed;
    int handle_hovered;
    int handle_redraw;
    FreqHandle freq_handles[FREQ_HANDLES];  
    /// Cached hand (drag) cursor
    GdkCursor *hand_cursor;
    /// Cached arrow (drag) cursor
    GdkCursor *arrow_cursor;
};

struct CalfLineGraphClass
{
    GtkEventBoxClass parent_class;
};

extern GtkWidget *calf_line_graph_new();

extern GType calf_line_graph_get_type();

extern void calf_line_graph_set_square(CalfLineGraph *graph, bool is_square);

extern void calf_line_graph_expose_request (GtkWidget *widget, bool force = false);

G_END_DECLS

#endif
