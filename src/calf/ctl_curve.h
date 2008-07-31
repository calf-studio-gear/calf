/* Calf DSP Library
 * Barely started curve editor widget. Standard GtkCurve is
 * unreliable and deprecated, so I need to make my own.
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
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#ifndef CALF_CTL_CURVE_H
#define CALF_CTL_CURVE_H

#include <gtk/gtk.h>
#include <vector>

G_BEGIN_DECLS

#define CALF_TYPE_CURVE          (calf_curve_get_type())
#define CALF_CURVE(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), CALF_TYPE_CURVE, CalfCurve))
#define CALF_IS_CURVE(obj)       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CALF_TYPE_CURVE))
#define CALF_CURVE_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass),  CALF_TYPE_CURVE, CalfCurveClass))
#define CALF_IS_CURVE_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((klass),  CALF_TYPE_CURVE))

struct CalfCurve
{
    typedef std::pair<float, float> point;
    typedef std::vector<point> point_vector;
    
    struct EventSink
    {
        virtual void curve_changed(const point_vector &data) = 0;
    };

    struct EventAdapter: public EventSink
    {
        virtual void curve_changed(const point_vector &data) {}
    };

    struct EventTester: public EventAdapter
    {
        virtual void curve_changed(const point_vector &data) {
            for(size_t i = 0; i < data.size(); i++)
                g_message("Point %d: (%f, %f)", (int)i, data[i].first, data[i].second);
        }
    };

    GtkWidget parent;
    point_vector *points;
    float x0, y0, x1, y1;
    int cur_pt;
    bool hide_current;
    EventSink *sink;
    GdkCursor *hand_cursor, *pencil_cursor;

    void log2phys(float &x, float &y);
    void phys2log(float &x, float &y);
    void clip(int pt, float &x, float &y, bool &hide);
};

struct CalfCurveClass
{
    GtkWidgetClass parent_class;
};

extern GtkWidget *calf_curve_new();
extern GType calf_curve_get_type();
extern void calf_curve_set_points(GtkWidget *widget, const CalfCurve::point_vector &src);

G_END_DECLS

#endif

