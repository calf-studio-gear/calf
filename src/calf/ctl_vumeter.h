/* Calf DSP Library
 * Volume meter widget
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
#ifndef CALF_VUMETER_H
#define CALF_VUMETER_H

#include <gtk/gtk.h>
#include <calf/giface.h>

G_BEGIN_DECLS

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
    VU_MONOCHROME_REVERSE,
    VU_STANDARD_CENTER,
    VU_MONOCHROME_CENTER
};

struct CalfVUMeter
{
    GtkDrawingArea parent;
    CalfVUMeterMode mode;
    float value;
    float vumeter_hold;
    bool holding;
    long last_hold;
    float last_value;
    float vumeter_falloff;
    bool falling;
    float last_falloff;
    long last_falltime;
    int meter_width;
    int vumeter_width;
    int vumeter_height;
    float disp_value;
    int vumeter_position;
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
extern void calf_vumeter_set_hold(CalfVUMeter *meter, float value);
extern float calf_vumeter_get_hold(CalfVUMeter *meter);
extern void calf_vumeter_set_falloff(CalfVUMeter *meter, float value);
extern float calf_vumeter_get_falloff(CalfVUMeter *meter);
extern void calf_vumeter_set_width(CalfVUMeter *meter, int value);
extern int calf_vumeter_get_width(CalfVUMeter *meter);
extern void calf_vumeter_set_height(CalfVUMeter *meter, int value);
extern int calf_vumeter_get_height(CalfVUMeter *meter);
extern void calf_vumeter_set_position(CalfVUMeter *meter, int value);
extern int calf_vumeter_get_position(CalfVUMeter *meter);

G_END_DECLS

#endif
