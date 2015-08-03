/* Calf DSP Library
 * A meter scale widget
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
#ifndef __CALF_CTL_METERSCALE
#define __CALF_CTL_METERSCALE

#include <cairo/cairo.h>
#include <gtk/gtk.h>
//#include <gtk/gtkrange.h>
//#include <gtk/gtkscale.h>
//#include <calf/giface.h>
#include <calf/drawingutils.h>
#include <calf/ctl_vumeter.h>
#include <calf/gui.h>


//#include <gtk/gtkrange.h>
//#include "config.h"
//#include <calf/giface.h>
//#include <gdk/gdkkeysyms.h>
//#include <sys/stat.h>
//#include <math.h>
//#include <gdk/gdk.h>
//#include <gtk/gtk.h>
//#include <sys/time.h>
//#include <algorithm>
//#include <iostream>
//#include <calf/drawingutils.h>


G_BEGIN_DECLS


/// METER SCALE //////////////////////////////////////////////////////////////


#define CALF_TYPE_METER_SCALE          (calf_meter_scale_get_type())
#define CALF_METER_SCALE(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), CALF_TYPE_METER_SCALE, CalfMeterScale))
#define CALF_IS_METER_SCALE(obj)       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CALF_TYPE_METER_SCALE))
#define CALF_METER_SCALE_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass),  CALF_TYPE_METER_SCALE, CalfMeterScaleClass))
#define CALF_IS_METER_SCALE_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((klass),  CALF_TYPE_METER_SCALE))

struct CalfMeterScale
{
    GtkDrawingArea parent;
    std::vector<double> marker;
    CalfVUMeterMode mode;
    int position;
    int dots;
};

struct CalfMeterScaleClass
{
    GtkDrawingAreaClass parent_class;
};

extern GtkWidget *calf_meter_scale_new();
extern GType calf_meter_scale_get_type();

G_END_DECLS

#endif
