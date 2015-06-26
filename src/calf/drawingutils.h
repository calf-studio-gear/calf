/* Calf DSP Library
 * A few useful drawing functions
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


#ifndef CALF_DRAWINGUTILS_H
#define CALF_DRAWINGUTILS_H

#include <gtk/gtk.h>

void display_background(GtkWidget *widget, cairo_t* c, int x, int y, int sx, int sy, int ox, int oy, float radius = 0, float bevel = 0.2, float brightness = 1, int shadow = 7, float lights = 0.9, float dull = 0.15) ;

void draw_rect(GtkWidget * widget, const gchar * type, GtkStateType * state, gint x, gint y, gint width, gint height, float rad, float bevel);
void draw_inset(GtkWidget *widget, gint x, gint y, gint width, gint height, float rad, gint depth);
void draw_glass(GtkWidget *widget, gint x, gint y, gint width, gint height, float rad);
void _draw_inset(cairo_t *cr, gint x, gint y, gint width, gint height, float rad, gint depth);
void _draw_glass(cairo_t *cr, gint x, gint y, gint width, gint height, float rad);

void get_bg_color(GtkWidget * widget, GtkStateType * state, float * r, float * g, float * b);
void get_fg_color(GtkWidget * widget, GtkStateType * state, float * r, float * g, float * b);
void get_base_color(GtkWidget * widget, GtkStateType * state, float * r, float * g, float * b);
void get_text_color(GtkWidget * widget, GtkStateType * state, float * r, float * g, float * b);
void get_color(GtkWidget * widget, const gchar * type, GtkStateType * state, float * r, float * g, float * b);

void clip_context(GtkWidget * widget, cairo_t * cr, GdkRegion *region);
void create_rectangle(cairo_t * cr, gint x, gint y, gint width, gint height, float rad);
void draw_bevel(cairo_t * cr, gint x, gint y, gint width, gint height, float rad, float bevel);

#endif /* CALF_DRAWINGUTILS_H */
