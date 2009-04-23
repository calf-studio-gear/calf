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
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 * Boston, MA  02110-1301  USA
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

/// Mapping curve editor control. May be used for editing multisegment lines (key mapping,
/// velocity mapping etc). This version isn't suitable for envelope editing because both
/// ends (start and end) have fixed X coordinates and it doesn't resize.
struct CalfCurve
{
    /// A point with floating point X and Y coordinates
    typedef std::pair<float, float> point;
    /// A collection of points
    typedef std::vector<point> point_vector;
    
    /// User callbacks for handling curve events
    struct EventSink
    {
        /// Called when a point has been edited, added or removed
        virtual void curve_changed(CalfCurve *src, const point_vector &data) = 0;
        /// Called to clip/snap/otherwise adjust candidate point coordinates
        virtual void clip(CalfCurve *src, int pt, float &x, float &y, bool &hide) = 0;
	virtual ~EventSink() {}
    };

    /// Null implementation of EventSink
    struct EventAdapter: public EventSink
    {
        virtual void curve_changed(CalfCurve *src, const point_vector &data) {}
        virtual void clip(CalfCurve *src, int pt, float &x, float &y, bool &hide) {}
    };

    /// Debug implementation of EventSink
    struct EventTester: public EventAdapter
    {
        virtual void curve_changed(CalfCurve *src, const point_vector &data) {
            for(size_t i = 0; i < data.size(); i++)
                g_message("Point %d: (%f, %f)", (int)i, data[i].first, data[i].second);
        }
    };

    /// Base class instance members
    GtkWidget parent;
    /// Array of points
    point_vector *points;
    /// Coordinate ranges (in logical coordinates for top left and bottom right)
    float x0, y0, x1, y1;
    /// Currently selected point (when dragging/adding), or -1 if none is selected
    int cur_pt;
    /// If currently selected point is a candidate for deletion (ie. outside of graph+margin range)
    bool hide_current;
    /// Interface for notification
    EventSink *sink;
    /// Cached hand (drag) cursor
    GdkCursor *hand_cursor;
    /// Cached pencil (add point) cursor
    GdkCursor *pencil_cursor;
    /// Cached arrow (do not add point) cursor
    GdkCursor *arrow_cursor;
    /// Maximum number of points
    unsigned int point_limit;

    /// Convert logical (mapping) to physical (screen) coordinates
    void log2phys(float &x, float &y);
    /// Convert physical (screen) to logical (mapping) coordinates
    void phys2log(float &x, float &y);
    /// Clip function
    /// @param pt point being clipped
    /// @param x horizontal logical coordinate
    /// @param y vertical logical coordinate
    /// @param hide true if point is outside "valid" range and about to be deleted
    void clip(int pt, float &x, float &y, bool &hide);
};

struct CalfCurveClass
{
    GtkWidgetClass parent_class;
};

/// Create a CalfCurve
extern GtkWidget *calf_curve_new(unsigned int point_limit = -1);

/// Return a GObject type for class CalfCurve
extern GType calf_curve_get_type();

/// Set points and update the widget
extern void calf_curve_set_points(GtkWidget *widget, const CalfCurve::point_vector &src);

G_END_DECLS

#endif

