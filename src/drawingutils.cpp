/* Calf DSP Library
 * A few useful drawing funcitons
 * Copyright (C) 2007-2015 Krzysztof Foltman, Torben Hohn, Markus Schmidt
 * and others
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

#include "calf/drawingutils.h"
#include <cstring> 
#include <algorithm> 
#include <math.h> 


void display_background(GtkWidget *widget, cairo_t* c, int x, int y, int sx, int sy, int ox, int oy, float radius, float bevel, float brightness, int shadow, float lights, float dull) 
{
    float br = brightness * 0.5 + 0.5;
    //printf("bevel:%.2f brightness:%.2f shadow:%d lights:%.2f dull:%.2f\n", bevel, brightness, shadow, lights, dull);
    if (!c) {
        GdkWindow *window = widget->window;
        c = gdk_cairo_create(GDK_DRAWABLE(window));
    }
    
    float r, g, b;
    get_bg_color(widget, NULL, &r, &g, &b);
    
    create_rectangle(c, x, y, sx + ox * 2, sy + oy * 2, radius);
    cairo_set_source_rgb (c, r, g, b);
    cairo_fill(c);
    draw_bevel(c, x, y, sx + ox * 2, sy + oy * 2, radius, bevel);

    // inner screen
    get_base_color(widget, NULL, &r, &g, &b);
    cairo_pattern_t *pt = cairo_pattern_create_linear(x + ox, y + oy, x + ox, y + sy);
    float l = (1. - 0.25 * lights);
    cairo_pattern_add_color_stop_rgb(pt, 0.0, br * r * l, br * g * l, br * b * l);
    cairo_pattern_add_color_stop_rgb(pt, 1.0, br * r, br * g, br * b);
    cairo_set_source (c, pt);
    cairo_rectangle(c, x + ox, y + oy, sx, sy);
    cairo_fill(c);
    cairo_pattern_destroy(pt);

    if (shadow) {
        // top shadow
        pt = cairo_pattern_create_linear(x + ox, y + oy, x + ox, y + oy + shadow);
        cairo_pattern_add_color_stop_rgba(pt, 0.0, 0,0,0,0.6);
        cairo_pattern_add_color_stop_rgba(pt, 1.0, 0,0,0,0);
        cairo_set_source (c, pt);
        cairo_rectangle(c, x + ox, y + oy, sx, shadow);
        cairo_fill(c);
        cairo_pattern_destroy(pt);

        // left shadow
        pt = cairo_pattern_create_linear(x + ox, y + oy, x + ox + (float)shadow * 0.7, y + oy);
        cairo_pattern_add_color_stop_rgba(pt, 0.0, 0,0,0,0.3);
        cairo_pattern_add_color_stop_rgba(pt, 1.0, 0,0,0,0);
        cairo_set_source (c, pt);
        cairo_rectangle(c, x + ox, y + oy, (float)shadow * 0.7, sy);
        cairo_fill(c);
        cairo_pattern_destroy(pt);

        // right shadow
        pt = cairo_pattern_create_linear(float(x + ox + sx) - (float)shadow * 0.7, y + oy, x + ox + sx, y + oy);
        cairo_pattern_add_color_stop_rgba(pt, 0.0, 0,0,0,0);
        cairo_pattern_add_color_stop_rgba(pt, 1.0, 0,0,0,0.3);
        cairo_set_source (c, pt);
        cairo_rectangle(c, x + ox + sx - (float)shadow * 0.7, y + oy, (float)shadow * 0.7, sy);
        cairo_fill(c);
        cairo_pattern_destroy(pt);
    }

    if(dull) {
        pt = cairo_pattern_create_linear(x + ox, y + oy, x + ox + sx, y + oy);
        cairo_pattern_add_color_stop_rgba(pt, 0.0, 0,0,0,dull);
        cairo_pattern_add_color_stop_rgba(pt, 0.5, 0,0,0,0);
        cairo_pattern_add_color_stop_rgba(pt, 1.0, 0,0,0,dull);
        cairo_set_source (c, pt);
        cairo_rectangle(c, x + ox, y + oy, sx, sy);
        cairo_fill(c);
        cairo_pattern_destroy(pt);
    }
    
    if(lights > 0) {
        // light sources
        int div = 1;
        while(sx / div > 300)
            div += 1;
        float w = float(sx) / float(div);
        cairo_rectangle(c, x + ox, y + oy, sx, sy);
        for(int i = 0; i < div; i ++) {
            cairo_pattern_t *pt = cairo_pattern_create_radial(
               x + ox + w * i + w / 2.f, y + oy, 1,
               x + ox + w * i + w / 2.f, std::min(w / 2.0 + y + oy, y + oy + sy * 0.25) - 1, w / 2.f);
            cairo_pattern_add_color_stop_rgba (pt, 0, r * 1.8, g * 1.8, b * 1.8, lights);
            cairo_pattern_add_color_stop_rgba (pt, 1, r, g, b, 0);
            cairo_set_source (c, pt);
            cairo_fill_preserve(c);
            pt = cairo_pattern_create_radial(
               x + ox + w * i + w / 2.f, y + oy + sy, 1,
               x + ox + w * i + w / 2.f, std::max(sy - w / 2.0 + y + oy, y + oy + sy * 0.75) + 1, w / 2.f);
            cairo_pattern_add_color_stop_rgba (pt, 0, r * 1.8, g * 1.8, b * 1.8, lights);
            cairo_pattern_add_color_stop_rgba (pt, 1, r, g, b, 0);
            cairo_set_source (c, pt);
            cairo_fill_preserve(c);
            cairo_pattern_destroy(pt);
        }
    }
    cairo_new_path(c);
}

void draw_rect (GtkWidget * widget, const gchar * type, GtkStateType * state, gint x, gint y, gint width, gint height, float rad, float bevel) {
    cairo_t * cr = gdk_cairo_create(GDK_DRAWABLE(widget->window));
    float r, g, b;
    get_color(widget, type, state, &r, &g, &b);
    create_rectangle(cr, x, y, width, height, rad);
    cairo_set_source_rgb(cr, r, g, b);
    cairo_fill(cr);
    
    if (bevel)
        draw_bevel(cr, x, y, width, height, rad, bevel);
        
    cairo_destroy(cr);
}
void _draw_inset (cairo_t * cr, gint x, gint y, gint width, gint height, float rad, gint depth) {
    cairo_pattern_t *pat = cairo_pattern_create_linear (x, y, x, y + height);
    cairo_pattern_add_color_stop_rgba(pat, 0.0, 0.0, 0.0, 0.0, 0.33);
    cairo_pattern_add_color_stop_rgba(pat, 1.0, 1.0, 1.0, 1.0, 0.1);
    cairo_set_source(cr, pat);
    create_rectangle(cr, x-depth*0.5, y-depth, width+depth, height+2*depth, rad);
    cairo_fill(cr);
}
void draw_inset (GtkWidget * widget, gint x, gint y, gint width, gint height, float rad, gint depth) {
    cairo_t * cr = gdk_cairo_create(GDK_DRAWABLE(widget->window));
    _draw_inset(cr, x, y, width, height, rad, depth);
    cairo_destroy(cr);
}
void _draw_glass (cairo_t *cr, gint x, gint y, gint width, gint height, float rad) {
    cairo_pattern_t *pat = cairo_pattern_create_linear (x, y, x, y + 3);
    cairo_pattern_add_color_stop_rgba(pat, 0.0, 0.0, 0.0, 0.0, 0.5);
    cairo_pattern_add_color_stop_rgba(pat, 1.0, 0.0, 0.0, 0.0, 0.0);
    cairo_set_source(cr, pat);
    create_rectangle(cr, x, y, width, height, rad);
    cairo_fill(cr);
}
void draw_glass (GtkWidget * widget, gint x, gint y, gint width, gint height, float rad) {
    cairo_t * cr = gdk_cairo_create(GDK_DRAWABLE(widget->window));
    _draw_glass(cr, x, y, width, height, rad);
    cairo_destroy(cr);
}

void get_bg_color (GtkWidget * widget, GtkStateType * state, float * r, float * g, float * b) {
    get_color(widget, "bg", state, r, g, b);
}
void get_fg_color (GtkWidget * widget, GtkStateType * state, float * r, float * g, float * b) {
    get_color(widget, "fg", state, r, g, b);
}
void get_base_color (GtkWidget * widget, GtkStateType * state, float * r, float * g, float * b) {
    get_color(widget, "base", state, r, g, b);
}
void get_text_color (GtkWidget * widget, GtkStateType * state, float * r, float * g, float * b) {
    get_color(widget, "text", state, r, g, b);
}
void get_color (GtkWidget * widget, const gchar * type, GtkStateType * state, float * r, float * g, float * b) {
    GdkColor color;
    GtkStyle * style = gtk_widget_get_style (widget);
    if (style != NULL) {
        GtkStateType s;
        if (state)
            s = *state;
        else
            s = gtk_widget_get_state(widget);
        color = style->bg[s];
        if (!strcmp(type, "bg"))
            color = style->bg[s];
        if (!strcmp(type, "fg"))
            color = style->fg[s];
        if (!strcmp(type, "base"))
            color = style->base[s];
        if (!strcmp(type, "text"))
            color = style->text[s];
        *r = float(color.red)   / 65535;
        *g = float(color.green) / 65535;
        *b = float(color.blue)  / 65535;
    }
}

void clip_context (GtkWidget * widget, cairo_t * cr, GdkRegion *region) {
    GdkRegion *reg = gdk_region_rectangle(&widget->allocation);
    if (region)
        gdk_region_intersect(reg, region);
    gdk_cairo_region(cr, reg);
    cairo_clip (cr);
}

void create_rectangle (cairo_t * cr, gint x, gint y, gint width, gint height, float rad) {
    if (rad == 0) {
        cairo_rectangle(cr, x, y, width, height);
        return;
    }
    //cairo_move_to(cr,x+rad,y);                      // Move to A
    //cairo_line_to(cr,x+width-rad,y);                    // Straight line to B
    //cairo_curve_to(cr,x+width,y,x+width,y,x+width,y+rad);       // Curve to C, Control points are both at Q
    //cairo_line_to(cr,x+width,y+height-rad);                  // Move to D
    //cairo_curve_to(cr,x+width,y+height,x+width,y+height,x+width-rad,y+height); // Curve to E
    //cairo_line_to(cr,x+rad,y+height);                    // Line to F
    //cairo_curve_to(cr,x,y+height,x,y+height,x,y+height-rad);       // Curve to G
    //cairo_line_to(cr,x,y+rad);                      // Line to H
    //cairo_curve_to(cr,x,y,x,y,x+rad,y);             // Curve to A
    // top left
    cairo_move_to(cr, x, y + rad);
    cairo_arc (cr, x + rad, y + rad, rad, 1 * M_PI, 1.5 * M_PI);
    // top
    cairo_line_to(cr, x + width - rad, y);
    // top right
    cairo_arc (cr, x + width - rad, y + rad, rad, 1.5 * M_PI, 2 * M_PI);
    // right
    cairo_line_to(cr, x + width, y + height - rad);
    // bottom right
    cairo_arc (cr, x + width - rad, y + height - rad, rad, 0 * M_PI, 0.5 * M_PI);
    // bottom
    cairo_line_to(cr, x + rad, y + height);
    // bottom left
    cairo_arc (cr, x + rad, y + height - rad, rad, 0.5 * M_PI, 1 * M_PI);
    // left
    cairo_line_to(cr, x, y + rad);
        
}

void draw_bevel (cairo_t * cr, gint x, gint y, gint width, gint height, float rad, float bevel) {
    if (bevel == 0)
        return;
    cairo_save(cr);
    create_rectangle(cr, x, y, width, height, rad);
    cairo_pattern_t * pat;
    if (bevel > 0)
        pat = cairo_pattern_create_linear (x, y, x, y + height);
    else
        pat = cairo_pattern_create_linear (x, y + height, x, y);
    if (bevel < 0) bevel *= -1;
    cairo_pattern_add_color_stop_rgba(pat, 0.0, 1.0, 1.0, 1.0, bevel / 2);
    cairo_pattern_add_color_stop_rgba(pat, 1.0, 0.0, 0.0, 0.0, bevel);
    cairo_set_source(cr, pat);
    cairo_set_operator(cr, CAIRO_OPERATOR_SOFT_LIGHT);
    cairo_fill_preserve(cr);
    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
    cairo_fill(cr);
    cairo_pattern_destroy (pat);
    cairo_restore(cr);
}
