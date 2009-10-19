/* Calf DSP Library
 * Light emitting diode-like control.
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

#ifndef CALF_CTL_LED_H
#define CALF_CTL_LED_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define CALF_TYPE_LED          (calf_led_get_type())
#define CALF_LED(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), CALF_TYPE_LED, CalfLed))
#define CALF_IS_LED(obj)       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CALF_TYPE_LED))
#define CALF_LED_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass),  CALF_TYPE_LED, CalfLedClass))
#define CALF_IS_LED_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((klass),  CALF_TYPE_LED))

/// Instance object for CalfLed
struct CalfLed
{
    GtkWidget parent;
    cairo_surface_t *cache_surface;
    int led_mode;
    float led_value;
};

/// Class object for CalfLed
struct CalfLedClass
{
    GtkWidgetClass parent_class;
};

/// Create new CalfLed
extern GtkWidget *calf_led_new();

/// Get GObject type for the CalfLed
extern GType calf_led_get_type();

/// Set LED state (true - lit, false - unlit)
extern void calf_led_set_value(CalfLed *led, float value);

/// Get LED state (true - lit, false - unlit)
extern gboolean calf_led_get_value(CalfLed *led);

G_END_DECLS

#endif
