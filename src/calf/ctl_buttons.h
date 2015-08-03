/* Calf DSP Library
 * A few useful button widgets
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
#ifndef __CALF_CTL_BUTTONS
#define __CALF_CTL_BUTTONS

#include <cairo/cairo.h>
#include <gtk/gtk.h>
#include <gtk/gtkbutton.h>
#include <calf/drawingutils.h>
#include <calf/gui.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>

G_BEGIN_DECLS


/// BUTTON /////////////////////////////////////////////////////////////


#define CALF_TYPE_BUTTON          (calf_button_get_type())
#define CALF_BUTTON(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), CALF_TYPE_BUTTON, CalfButton))
#define CALF_IS_BUTTON(obj)       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CALF_TYPE_BUTTON))
#define CALF_BUTTON_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass),  CALF_TYPE_BUTTON, CalfButtonClass))
#define CALF_IS_BUTTON_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((klass),  CALF_TYPE_BUTTON))

struct CalfButton
{
    GtkButton parent;
};

struct CalfButtonClass
{
    GtkButtonClass parent_class;
};

extern GtkWidget *calf_button_new(const gchar *label);
extern GType calf_button_get_type();


/// TOGGLE BUTTON //////////////////////////////////////////////////////


#define CALF_TYPE_TOGGLE_BUTTON          (calf_toggle_button_get_type())
#define CALF_TOGGLE_BUTTON(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), CALF_TYPE_TOGGLE_BUTTON, CalfToggleButton))
#define CALF_IS_TOGGLE_BUTTON(obj)       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CALF_TYPE_TOGGLE_BUTTON))
#define CALF_TOGGLE_BUTTON_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass),  CALF_TYPE_TOGGLE_BUTTON, CalfToggleButtonClass))
#define CALF_IS_TOGGLE_BUTTON_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((klass),  CALF_TYPE_TOGGLE_BUTTON))

struct CalfToggleButton
{
    GtkToggleButton parent;
};

struct CalfToggleButtonClass
{
    GtkToggleButtonClass parent_class;
};

extern GtkWidget *calf_toggle_button_new(const gchar *label);
extern GType calf_toggle_button_get_type();


/// RADIO BUTTON //////////////////////////////////////////////////////


#define CALF_TYPE_RADIO_BUTTON          (calf_radio_button_get_type())
#define CALF_RADIO_BUTTON(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), CALF_TYPE_RADIO_BUTTON, CalfRadioButton))
#define CALF_IS_RADIO_BUTTON(obj)       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CALF_TYPE_RADIO_BUTTON))
#define CALF_RADIO_BUTTON_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass),  CALF_TYPE_RADIO_BUTTON, CalfRadioButtonClass))
#define CALF_IS_RADIO_BUTTON_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((klass),  CALF_TYPE_RADIO_BUTTON))

struct CalfRadioButton
{
    GtkRadioButton parent;
};

struct CalfRadioButtonClass
{
    GtkRadioButtonClass parent_class;
};

extern GtkWidget *calf_radio_button_new(const gchar *label);
extern GType calf_radio_button_get_type();


/// TOGGLE /////////////////////////////////////////////////////////////


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
    GdkPixbuf *toggle_image;
};

struct CalfToggleClass
{
    GtkRangeClass parent_class;
};

extern GtkWidget *calf_toggle_new();
extern GtkWidget *calf_toggle_new_with_adjustment(GtkAdjustment *_adjustment);
extern void calf_toggle_set_size(CalfToggle *self, int size);
extern void calf_toggle_set_pixbuf(CalfToggle *self, GdkPixbuf *pixbuf);
extern GType calf_toggle_get_type();


/// TAP BUTTON /////////////////////////////////////////////////////////


#define CALF_TYPE_TAP_BUTTON          (calf_tap_button_get_type())
#define CALF_TAP_BUTTON(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), CALF_TYPE_TAP_BUTTON, CalfTapButton))
#define CALF_IS_TAP_BUTTON(obj)       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CALF_TYPE_TAP_BUTTON))
#define CALF_TAP_BUTTON_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass),  CALF_TYPE_TAP_BUTTON, CalfTapButtonClass))
#define CALF_IS_TAP_BUTTON_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((klass),  CALF_TYPE_TAP_BUTTON))

struct CalfTapButton
{
    GtkButton parent;
    GdkPixbuf *image[3];
    int state;
};

struct CalfTapButtonClass
{
    GtkButtonClass parent_class;
};

extern GtkWidget *calf_tap_button_new();
extern GType calf_tap_button_get_type();
extern void calf_tap_button_set_pixbufs (CalfTapButton *self, GdkPixbuf *image1, GdkPixbuf *image2, GdkPixbuf *image3);

G_END_DECLS

#endif
