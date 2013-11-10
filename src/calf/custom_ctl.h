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
#ifndef __CALF_CUSTOM_CTL
#define __CALF_CUSTOM_CTL


#include <gtk/gtk.h>
#include <calf/giface.h>

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
    
    float left_bound;
    float right_bound;
    gpointer data;

    inline bool is_active() { return (param_active_no < 0 || active); }
};

#define FREQ_HANDLES 32
#define HANDLE_WIDTH 16.0

struct CalfLineGraph
{
    static const int debug = 1;
    
    GtkDrawingArea parent;
    const calf_plugins::line_graph_iface *source;
    int source_id;
    bool force_cache;
    int recreate_surfaces;
    bool is_square;
    float fade;
    int mode, moving, movesurf;
    int count;
    
    static const int pad_x = 5, pad_y = 5;
    int size_x, size_y;
    
    cairo_surface_t *background_surface;
    cairo_surface_t *grid_surface;
    cairo_surface_t *cache_surface;
    cairo_surface_t *moving_surface[2];
    cairo_surface_t *handles_surface;
    cairo_surface_t *final_surface;

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
    GtkDrawingAreaClass parent_class;
};

extern GtkWidget *calf_line_graph_new();

extern GType calf_line_graph_get_type();

extern void calf_line_graph_set_square(CalfLineGraph *graph, bool is_square);

extern int calf_line_graph_update_if(CalfLineGraph *graph, int generation);

#define CALF_TYPE_PHASE_GRAPH           (calf_phase_graph_get_type())
#define CALF_PHASE_GRAPH(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), CALF_TYPE_PHASE_GRAPH, CalfPhaseGraph))
#define CALF_IS_PHASE_GRAPH(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CALF_TYPE_PHASE_GRAPH))
#define CALF_PHASE_GRAPH_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST ((klass),  CALF_TYPE_PHASE_GRAPH, CalfPhaseGraphClass))
#define CALF_IS_PHASE_GRAPH_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE ((klass),  CALF_TYPE_PHASE_GRAPH))
#define CALF_PHASE_GRAPH_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj),  CALF_TYPE_PHASE_GRAPH, CalfPhaseGraphClass))

struct CalfPhaseGraph
{
    GtkDrawingArea parent;
    const calf_plugins::phase_graph_iface *source;
    int source_id;
    cairo_surface_t *cache_surface;
    cairo_surface_t *fade_surface;
    inline float _atan(float x, float l, float r) {
        // this is a wrapper for a CPU friendly implementation of atan()
        if(l >= 0 and r >= 0) {
            return atan(x);
        } else if(l >= 0 and r < 0) {
            return M_PI + atan(x);
        } else if(l < 0 and r < 0) {
            return M_PI + atan(x);
        } else if(l < 0 and r >= 0) {
            return (2.f * M_PI) + atan(x);
        }
        return 0.f;
    }
};

struct CalfPhaseGraphClass
{
    GtkDrawingAreaClass parent_class;
};

extern GtkWidget *calf_phase_graph_new();

extern GType calf_phase_graph_get_type();

#define CALF_TYPE_TOGGLE          (calf_toggle_get_type())
#define CALF_TOGGLE(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), CALF_TYPE_TOGGLE, CalfToggle))
#define CALF_IS_TOGGLE(obj)       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CALF_TYPE_TOGGLE))
#define CALF_TOGGLE_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass),  CALF_TYPE_TOGGLE, CalfToggleClass))
#define CALF_IS_TOGGLE_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((klass),  CALF_TYPE_TOGGLE))

struct CalfToggle
{
    GtkRange parent;
    int size;
    int width;
    int height;
};

struct CalfToggleClass
{
    GtkRangeClass parent_class;
    GdkPixbuf *toggle_image[2];
};

extern GtkWidget *calf_toggle_new();
extern GtkWidget *calf_toggle_new_with_adjustment(GtkAdjustment *_adjustment);

extern GType calf_toggle_get_type();

G_END_DECLS

class cairo_impl: public calf_plugins::cairo_iface
{
public:
    cairo_t *context;
    virtual void set_source_rgba(float r, float g, float b, float a = 1.f) { cairo_set_source_rgba(context, r, g, b, a); }
    virtual void set_line_width(float width) { cairo_set_line_width(context, width); }
    virtual void set_dash(const double *dash, int length) { cairo_set_dash(context, dash, length, 0); }
};

#endif
