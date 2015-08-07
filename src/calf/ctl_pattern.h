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
#ifndef __CALF_CTL_PATTERN
#define __CALF_CTL_PATTERN

#include <gtk/gtk.h>
#include <calf/gui.h>

G_BEGIN_DECLS

#define CALF_TYPE_PATTERN           (calf_pattern_get_type())
#define CALF_PATTERN(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), CALF_TYPE_PATTERN, CalfPattern))
#define CALF_IS_PATTERN(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CALF_TYPE_PATTERN))
#define CALF_PATTERN_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST ((klass),  CALF_TYPE_PATTERN, CalfPatternClass))
#define CALF_IS_PATTERN_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE ((klass),  CALF_TYPE_PATTERN))
#define CALF_PATTERN_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj),  CALF_TYPE_PATTERN, CalfPatternClass))

struct CalfPattern
{
    GtkEventBox parent;
    bool force_redraw;
    const static int border = 2;
    const static int minner = 1;
    const static int mbars = 4;
    float pad_x, pad_y;
    float size_x, size_y;
    float mouse_x, mouse_y;
    float x, y, width, height, border_h, border_v, bar_width, beat_width, beat_height;
    int beats, bars;
    int handle_grabbed;
    int handle_hovered;
    int handle_redraw;
    double values[64];
    cairo_surface_t *background_surface;
    /// Cached hand (drag) cursor
    GdkCursor *hand_cursor;
    /// Cached arrow (drag) cursor
    GdkCursor *arrow_cursor;
};

struct CalfPatternClass
{
    GtkEventBoxClass parent_class;
};

extern GtkWidget *calf_pattern_new();

extern GType calf_pattern_get_type();

extern void calf_pattern_expose_request (GtkWidget *widget, bool force = false);

G_END_DECLS

#endif
