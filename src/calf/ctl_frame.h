/* Calf DSP Library
 * A frame widget
 *
 * Copyright (C) 2008-2015 Krzysztof Foltman, Torben Hohn, Markus
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
#ifndef __CALF_CTL_FRAME
#define __CALF_CTL_FRAME

#include <cairo/cairo.h>
#include <gtk/gtk.h>
#include <gtk/gtkframe.h>
//#include <calf/giface.h>
#include <calf/drawingutils.h>
#include <calf/gui.h>

G_BEGIN_DECLS


/// FRAME //////////////////////////////////////////////////////////////


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

G_END_DECLS

#endif
