/* Calf DSP Library
 * A fader widget
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
#ifndef __CALF_CTL_FADER
#define __CALF_CTL_FADER

#include <cairo/cairo.h>
#include <gtk/gtk.h>
#include <gtk/gtkrange.h>
#include <gtk/gtkscale.h>
//#include <calf/giface.h>
#include <calf/drawingutils.h>
#include <calf/gui.h>

G_BEGIN_DECLS

/// FADER //////////////////////////////////////////////////////////////

#define CALF_TYPE_FADER          (calf_fader_get_type())
#define CALF_FADER(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), CALF_TYPE_FADER, CalfFader))
#define CALF_IS_FADER(obj)       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CALF_TYPE_FADER))
#define CALF_FADER_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass),  CALF_TYPE_FADER, CalfFaderClass))
#define CALF_IS_FADER_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((klass),  CALF_TYPE_FADER))

struct CalfFaderLayout
{
    int x, y, w, h, iw, ih;
    int t1w, t1h, t1x1, t1y1, t1x2, t1y2;
    int t2w, t2h, t2x1, t2y1, t2x2, t2y2;
    int s1w, s1h, s1x1, s1y1, s1x2, s1y2;
    int s2w, s2h, s2x1, s2y1, s2x2, s2y2;;
    int sw, sh, sx1, sy1, sx2, sy2, sw2, sh2;
};

struct CalfFader
{
    GtkScale parent;
    int horizontal, size;
    GdkPixbuf *image;
    CalfFaderLayout layout;
    bool hover;
};

struct CalfFaderClass
{
    GtkScaleClass parent_class;
};

extern GtkWidget *calf_fader_new(const int horiz, const int size, const double min, const double max, const double step);
extern GType calf_fader_get_type();
extern void calf_fader_set_pixbuf(CalfFader *self, GdkPixbuf *image);
extern void calf_fader_set_layout(GtkWidget *widget);

G_END_DECLS

#endif
