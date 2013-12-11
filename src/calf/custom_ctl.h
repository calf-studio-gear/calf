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
#include <gtk/gtkframe.h>
#include <gtk/gtkcomboboxtext.h>
#include <calf/giface.h>

G_BEGIN_DECLS

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
    cairo_surface_t *background, *cache;
    inline float _atan(float x, float l, float r) {
        if(l >= 0 and r >= 0)
            return atan(x);
        else if(l >= 0 and r < 0)
            return M_PI + atan(x);
        else if(l < 0 and r < 0)
            return M_PI + atan(x);
        else if(l < 0 and r >= 0)
            return (2.f * M_PI) + atan(x);
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



#define CALF_TYPE_FRAME          (calf_frame_get_type())
#define CALF_FRAME(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), CALF_TYPE_FRAME, CalfFrame))
#define CALF_IS_FRAME(obj)       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CALF_TYPE_FRAME))
#define CALF_FRAME_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass),  CALF_TYPE_FRAME, CalfFrameClass))
#define CALF_IS_FRAME_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((klass),  CALF_TYPE_FRAME))

struct CalfFrame
{
    GtkFrame parent;
};

struct CalfFrameClass
{
    GtkFrameClass parent_class;
};

extern GtkWidget *calf_frame_new(const char *label);
extern GType calf_frame_get_type();


#define CALF_TYPE_COMBOBOX          (calf_combobox_get_type())
#define CALF_COMBOBOX(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), CALF_TYPE_COMBOBOX, CalfCombobox))
#define CALF_IS_COMBOBOX(obj)       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CALF_TYPE_COMBOBOX))
#define CALF_COMBOBOX_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass),  CALF_TYPE_COMBOBOX, CalfComboboxClass))
#define CALF_IS_COMBOBOX_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((klass),  CALF_TYPE_COMBOBOX))

struct CalfCombobox
{
    GtkComboBoxText parent;
};

struct CalfComboboxClass
{
    GtkComboBoxTextClass parent_class;
};

extern GtkWidget *calf_combobox_new();
extern GType calf_combobox_get_type();

G_END_DECLS

#endif
