/* Calf DSP Library
 * A few useful widgets - a line graph, a knob, a VU meter - Panama!
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
 * Boston, MA 02111-1307, USA.
 */
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

struct CalfLineGraph
{
    GtkDrawingArea parent;
    calf_plugins::line_graph_iface *source;
    int source_id;
    bool is_square;
    cairo_surface_t *cache_surface;
    //GdkPixmap *cache_pixmap;
    int last_generation;
};

struct CalfLineGraphClass
{
    GtkDrawingAreaClass parent_class;
};

extern GtkWidget *calf_line_graph_new();

extern GType calf_line_graph_get_type();

extern void calf_line_graph_set_square(CalfLineGraph *graph, bool is_square);

extern int calf_line_graph_update_if(CalfLineGraph *graph, int generation);

#define CALF_TYPE_VUMETER           (calf_vumeter_get_type())
#define CALF_VUMETER(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), CALF_TYPE_VUMETER, CalfVUMeter))
#define CALF_IS_VUMETER(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CALF_TYPE_VUMETER))
#define CALF_VUMETER_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST ((klass),  CALF_TYPE_VUMETER, CalfVUMeterClass))
#define CALF_IS_VUMETER_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE ((klass),  CALF_TYPE_VUMETER))
#define CALF_VUMETER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj),  CALF_TYPE_VUMETER, CalfVUMeterClass))

enum CalfVUMeterMode
{
    VU_STANDARD,
    VU_MONOCHROME,
    VU_MONOCHROME_REVERSE
};

struct CalfVUMeter
{
    GtkDrawingArea parent;
    CalfVUMeterMode mode;
    float value;
    int vumeter_hold;
    bool holding;
    long last_hold;
    float last_value;
    cairo_surface_t *cache_surface;
    cairo_pattern_t *pat;
};

struct CalfVUMeterClass
{
    GtkDrawingAreaClass parent_class;
};

extern GtkWidget *calf_vumeter_new();
extern GType calf_vumeter_get_type();
extern void calf_vumeter_set_value(CalfVUMeter *meter, float value);
extern float calf_vumeter_get_value(CalfVUMeter *meter);
extern void calf_vumeter_set_mode(CalfVUMeter *meter, CalfVUMeterMode mode);
extern CalfVUMeterMode calf_vumeter_get_mode(CalfVUMeter *meter);

#define CALF_TYPE_KNOB          (calf_knob_get_type())
#define CALF_KNOB(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), CALF_TYPE_KNOB, CalfKnob))
#define CALF_IS_KNOB(obj)       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CALF_TYPE_KNOB))
#define CALF_KNOB_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass),  CALF_TYPE_KNOB, CalfKnobClass))
#define CALF_IS_KNOB_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((klass),  CALF_TYPE_KNOB))

struct CalfKnob
{
    GtkRange parent;
    int knob_type;
    int knob_size;
    double start_x, start_y, start_value, last_y, last_x;
    double last_dz;
};

struct CalfKnobClass
{
    GtkRangeClass parent_class;
    GdkPixbuf *knob_image[4];
};

extern GtkWidget *calf_knob_new();
extern GtkWidget *calf_knob_new_with_adjustment(GtkAdjustment *_adjustment);

extern GType calf_knob_get_type();

#define CALF_TYPE_TOGGLE          (calf_toggle_get_type())
#define CALF_TOGGLE(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), CALF_TYPE_TOGGLE, CalfToggle))
#define CALF_IS_TOGGLE(obj)       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CALF_TYPE_TOGGLE))
#define CALF_TOGGLE_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass),  CALF_TYPE_TOGGLE, CalfToggleClass))
#define CALF_IS_TOGGLE_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((klass),  CALF_TYPE_TOGGLE))

struct CalfToggle
{
    GtkRange parent;
    int size;
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
};

#endif
