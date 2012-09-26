/* Calf DSP Library
 * A tube widget for overdrive-type plugins (saturator etc).
 *
 * Copyright (C) 2010-2012 Markus Schmidt and others
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
 
#ifndef CALF_CTL_TUBE_H
#define CALF_CTL_TUBE_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define CALF_TYPE_TUBE           (calf_tube_get_type())
#define CALF_TUBE(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), CALF_TYPE_TUBE, CalfTube))
#define CALF_IS_TUBE(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CALF_TYPE_TUBE))
#define CALF_TUBE_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST ((klass),  CALF_TYPE_TUBE, CalfTubeClass))
#define CALF_IS_TUBE_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE ((klass),  CALF_TYPE_TUBE))
#define CALF_TUBE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj),  CALF_TYPE_TUBE, CalfTubeClass))

struct CalfTube
{
    GtkDrawingArea parent;
    int size;
    int direction;
    float value;
    float last_value;
    float tube_falloff;
    bool falling;
    float last_falloff;
    long last_falltime;
    cairo_surface_t *cache_surface;
};

struct CalfTubeClass
{
    GtkDrawingAreaClass parent_class;
};

extern GtkWidget *calf_tube_new();
extern GType calf_tube_get_type();
extern void calf_tube_set_value(CalfTube *tube, float value);

G_END_DECLS

#endif
