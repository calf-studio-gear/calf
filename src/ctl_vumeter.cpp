/* Calf DSP Library
 * Custom controls (line graph, knob).
 * Copyright (C) 2007-2010 Krzysztof Foltman, Torben Hohn, Markus Schmidt
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

#include "config.h"
#include <calf/ctl_vumeter.h>
#include <gdk/gdkkeysyms.h>
#include <cairo/cairo.h>
#include <math.h>
#include <gdk/gdk.h>
#include <sys/time.h>

///////////////////////////////////////// vu meter ///////////////////////////////////////////////

static void
calf_vumeter_draw_outline_part (cairo_t *c, int pad, int ix, int iy, int ox, int oy, int sx, int sy)
{
    cairo_rectangle(c, ix + pad, iy + pad, sx + ox * 2 - pad * 2, sy + oy * 2 - pad * 2);
}

static void
calf_vumeter_draw_outline (cairo_t *c, int ix, int iy, int ox, int oy, int sx, int sy)
{
    // outer (black)
    calf_vumeter_draw_outline_part (c, 0, ix, iy, ox, oy, sx, sy);
    cairo_set_source_rgb(c, 0, 0, 0);
    cairo_fill(c);
    
    // inner (bevel)
    calf_vumeter_draw_outline_part (c, 1, ix, iy, ox, oy, sx, sy);
    cairo_pattern_t *pat2 = cairo_pattern_create_linear (0, 0, 0, sy + oy * 2 - 1 * 2);
    cairo_pattern_add_color_stop_rgba (pat2, 0, 0.23, 0.23, 0.23, 1);
    cairo_pattern_add_color_stop_rgba (pat2, 0.5, 0, 0, 0, 1);
    cairo_set_source (c, pat2);
    cairo_fill(c);
    cairo_pattern_destroy(pat2);
}

static gboolean
calf_vumeter_expose (GtkWidget *widget, GdkEventExpose *event)
{
    g_assert(CALF_IS_VUMETER(widget));


    CalfVUMeter *vu = CALF_VUMETER(widget);
    GtkStyle	*style;
    
    int ox = 4, oy = 3, led = 3, inner = 1;
    int mx = vu->meter_width;
    if (mx > widget->allocation.width / 2)
        mx = 0;
    int sx = widget->allocation.width - (ox * 2), sy = widget->allocation.height - (oy * 2);
    int sx1 = sx - ((widget->allocation.width - inner * 2 - ox * 2) % led) - 1 - mx;
    style = gtk_widget_get_style(widget);
    cairo_t *c = gdk_cairo_create(GDK_DRAWABLE(widget->window));
    if( vu->cache_surface == NULL ) {
        // looks like its either first call or the widget has been resized.
        // create the cache_surface.
        cairo_surface_t *window_surface = cairo_get_target( c );
        vu->cache_surface = cairo_surface_create_similar( window_surface, 
                                  CAIRO_CONTENT_COLOR,
                                  widget->allocation.width,
                                  widget->allocation.height );

        // And render the meterstuff again.

        cairo_t *cache_cr = cairo_create( vu->cache_surface );
        
        // theme background for reduced width and round borders
//        if(widget->style->bg_pixmap[0] == NULL) {
            gdk_cairo_set_source_color(cache_cr,&style->bg[GTK_STATE_NORMAL]);
//        } else {
//            gdk_cairo_set_source_pixbuf(cache_cr, GDK_PIXBUF(widget->style->bg_pixmap[0]), widget->allocation.x, widget->allocation.y + 20);
//        }
        cairo_paint(cache_cr);
        
        calf_vumeter_draw_outline (cache_cr, 0, 0, ox, oy, sx, sy);
        
        sx = sx1;
        
        cairo_rectangle(cache_cr, ox, oy, sx, sy);
        cairo_set_source_rgb (cache_cr, 0, 0, 0);
        cairo_fill(cache_cr);
        
        cairo_set_line_width(cache_cr, 1);

        for (int x = ox + inner; x <= ox + sx - led; x += led)
        {
            float ts = (x - ox) * 1.0 / sx;
            float r = 0.f, g = 0.f, b = 0.f;
            switch(vu->mode)
            {
            case VU_STANDARD:
            default:
                if (ts < 0.75)
                r = ts / 0.75, g = 0.5 + ts * 0.66, b = 1 - ts / 0.75;
                else
                r = 1, g = 1 - (ts - 0.75) / 0.25, b = 0;
                //                if (vu->value < ts || vu->value <= 0)
                //                    r *= 0.5, g *= 0.5, b *= 0.5;
                break;
            case VU_MONOCHROME_REVERSE:
                r = 0, g = 170.0 / 255.0, b = 1;
                //                if (!(vu->value < ts) || vu->value >= 1.0)
                //                    r *= 0.5, g *= 0.5, b *= 0.5;
                break;
            case VU_MONOCHROME:
                r = 0, g = 170.0 / 255.0, b = 1;
                //                if (vu->value < ts || vu->value <= 0)
                //                    r *= 0.5, g *= 0.5, b *= 0.5;
                break;
            }
            GdkColor sc2 = { 0, (guint16)(65535 * r + 0.2), (guint16)(65535 * g), (guint16)(65535 * b) };
            GdkColor sc3 = { 0, (guint16)(65535 * r * 0.7), (guint16)(65535 * g * 0.7), (guint16)(65535 * b * 0.7) };
            gdk_cairo_set_source_color(cache_cr, &sc2);
            cairo_move_to(cache_cr, x + 0.5, oy + 1);
            cairo_line_to(cache_cr, x + 0.5, oy + sy - inner);
            cairo_stroke(cache_cr);
            gdk_cairo_set_source_color(cache_cr, &sc3);
            cairo_move_to(cache_cr, x + 1.5, oy + sy - inner);
            cairo_line_to(cache_cr, x + 1.5, oy + 1);
            cairo_stroke(cache_cr);
        }
        // create blinder pattern
        vu->pat = cairo_pattern_create_linear (ox, oy, ox, ox + sy);
        cairo_pattern_add_color_stop_rgba (vu->pat, 0, 0.5, 0.5, 0.5, 0.6);
        cairo_pattern_add_color_stop_rgba (vu->pat, 0.4, 0, 0, 0, 0.6);
        cairo_pattern_add_color_stop_rgba (vu->pat, 0.401, 0, 0, 0, 0.8);
        cairo_pattern_add_color_stop_rgba (vu->pat, 1, 0, 0, 0, 0.6);
        cairo_destroy( cache_cr );
    }
    
    sx = sx1;

    cairo_set_source_surface( c, vu->cache_surface, 0,0 );
    cairo_paint( c );
    cairo_set_source( c, vu->pat );
    
    // get microseconds
    timeval tv;
    gettimeofday(&tv, 0);
    long time = tv.tv_sec * 1000 * 1000 + tv.tv_usec;
    
    // limit to 1.f
    float value_orig = vu->value > 1.f ? 1.f : vu->value;
    value_orig = value_orig < 0.f ? 0.f : value_orig;
    float value = 0.f;
    
    // falloff?
    if(vu->vumeter_falloff > 0.f and vu->mode != VU_MONOCHROME_REVERSE) {
        // fall off a bit
        float s = ((float)(time - vu->last_falltime) / 1000000.0);
        float m = vu->last_falloff * s * vu->vumeter_falloff;
        vu->last_falloff -= m;
        // new max value?
        if(value_orig > vu->last_falloff) {
            vu->last_falloff = value_orig;
        }
        value = vu->last_falloff;
        vu->last_falltime = time;
        vu->falling = vu->last_falloff > 0.000001;
    } else {
        // falloff disabled
        vu->last_falloff = 0.f;
        vu->last_falltime = 0.f;
        value = value_orig;
    }
    
    if(vu->vumeter_hold > 0.0) {
        // peak hold timer
        if(time - (long)(vu->vumeter_hold * 1000 * 1000) > vu->last_hold) {
            // time's up, reset
            vu->last_value = value;
            vu->last_hold = time;
            vu->holding = false;
        }
        if( vu->mode == VU_MONOCHROME_REVERSE ) {
            if(value < vu->last_value) {
                // value is above peak hold
                vu->last_value = value;
                vu->last_hold = time;
                vu->holding = true;
            }
            int hpx = round(vu->last_value * (sx - 2 * inner));
            hpx = hpx + (1 - (hpx + inner) % led);
            int vpx = round((1 - value) * (sx - 2 * inner));
            vpx = vpx + (1 - (vpx + inner) % led);
            int widthA = std::min(led + hpx, (sx - 2 * inner));
            int widthB = std::min(std::max((sx - 2 * inner) - vpx - led - hpx, 0), (sx - 2 * inner));
            cairo_rectangle( c, ox + inner, oy + inner, hpx, sy - 2 * inner);
            cairo_rectangle( c, ox + inner + widthA, oy + inner, widthB, sy - 2 * inner);
        } else {
            if(value > vu->last_value) {
                // value is above peak hold
                vu->last_value = value;
                vu->last_hold = time;
                vu->holding = true;
            }
            int hpx = round((1 - vu->last_value) * (sx - 2 * inner));
            hpx = hpx + (1 - (hpx + inner) % led);
            int vpx = round(value * (sx - 2 * inner));
            vpx = vpx + (1 - (vpx + inner) % led);
            int width = std::min(std::max((sx - 2 * inner) - vpx - led - hpx, 0), (sx - 2 * inner));
            cairo_rectangle( c, ox + inner + vpx, oy + inner, width, sy - 2 * inner);
            cairo_rectangle( c, ox + inner + (sx - 2 * inner) - hpx, oy + inner, hpx, sy - 2 * inner);
        }
    } else {
        // darken normally
        if( vu->mode == VU_MONOCHROME_REVERSE )
            cairo_rectangle( c, ox + inner,oy + inner, value * (sx - 2 * inner), sy - 2 * inner);
        else
            cairo_rectangle( c, ox + inner + value * (sx - 2 * inner), oy + inner, (sx - 2 * inner) * (1 - value), sy - 2 * inner );
    }
    cairo_fill( c );
    
    if (mx > 0)
    {
        cairo_select_font_face(c, "cairo:sans-serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
        cairo_set_font_size(c, 10);
        
        const char *str = vu->meter_text ? vu->meter_text->c_str() : "";

        cairo_text_extents_t tx;
        cairo_text_extents(c, str, &tx);

        int mtl = 2;
        int ix = widget->allocation.width - mx + mtl;
        calf_vumeter_draw_outline(c, ix, 0, ox, oy, mx - mtl - 2 * ox, sy);

        cairo_move_to(c, ox + sx + mx - tx.width - inner, oy + sy / 2 + tx.height / 2);
        cairo_set_source_rgba (c, 0, 1, 0, 1);
        cairo_show_text(c, str);
        cairo_fill(c);
    }
    cairo_destroy(c);
    //gtk_paint_shadow(widget->style, widget->window, GTK_STATE_NORMAL, GTK_SHADOW_IN, NULL, widget, NULL, ox - 2, oy - 2, sx + 4, sy + 4);
    //printf("exposed %p %d+%d\n", widget->window, widget->allocation.x, widget->allocation.y);

    return TRUE;
}

static void
calf_vumeter_size_request (GtkWidget *widget,
                           GtkRequisition *requisition)
{
    g_assert(CALF_IS_VUMETER(widget));

    requisition->width = 50;
    requisition->height = 18;
}

static void
calf_vumeter_size_allocate (GtkWidget *widget,
                           GtkAllocation *allocation)
{
    g_assert(CALF_IS_VUMETER(widget));
    CalfVUMeter *vu = CALF_VUMETER(widget);
    
    GtkWidgetClass *parent_class = (GtkWidgetClass *) g_type_class_peek_parent( CALF_VUMETER_GET_CLASS( vu ) );

    parent_class->size_allocate( widget, allocation );

    if( vu->cache_surface )
        cairo_surface_destroy( vu->cache_surface );
    vu->cache_surface = NULL;
}

static void
calf_vumeter_class_init (CalfVUMeterClass *klass)
{
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);
    widget_class->expose_event = calf_vumeter_expose;
    widget_class->size_request = calf_vumeter_size_request;
    widget_class->size_allocate = calf_vumeter_size_allocate;
}

static void
calf_vumeter_init (CalfVUMeter *self)
{
    GtkWidget *widget = GTK_WIDGET(self);
    //GTK_WIDGET_SET_FLAGS (widget, GTK_NO_WINDOW);
    widget->requisition.width = 50;
    widget->requisition.height = 18;
    self->cache_surface = NULL;
    self->falling = false;
    self->holding = false;
    self->meter_width = 0;
    self->meter_text = NULL;
}

GtkWidget *
calf_vumeter_new()
{
    return GTK_WIDGET( g_object_new (CALF_TYPE_VUMETER, NULL ));
}

GType
calf_vumeter_get_type (void)
{
    static GType type = 0;
    if (!type) {
        static const GTypeInfo type_info = {
            sizeof(CalfVUMeterClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            (GClassInitFunc)calf_vumeter_class_init,
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof(CalfVUMeter),
            0,    /* n_preallocs */
            (GInstanceInitFunc)calf_vumeter_init
        };

        GTypeInfo *type_info_copy = new GTypeInfo(type_info);

        for (int i = 0; ; i++) {
            char *name = g_strdup_printf("CalfVUMeter%u%d", ((unsigned int)(intptr_t)calf_vumeter_class_init) >> 16, i);
            if (g_type_from_name(name)) {
                free(name);
                continue;
            }
            type = g_type_register_static( GTK_TYPE_DRAWING_AREA,
                                           name,
                                           type_info_copy,
                                           (GTypeFlags)0);
            free(name);
            break;
        }
    }
    return type;
}

extern void calf_vumeter_set_value(CalfVUMeter *meter, float value)
{
    if (value != meter->value or meter->holding or meter->falling)
    {
        meter->value = value;
        gtk_widget_queue_draw(GTK_WIDGET(meter));
    }
}

extern float calf_vumeter_get_value(CalfVUMeter *meter)
{
    return meter->value;
}

extern void calf_vumeter_set_mode(CalfVUMeter *meter, CalfVUMeterMode mode)
{
    if (mode != meter->mode)
    {
        meter->mode = mode;
        if(mode == VU_MONOCHROME_REVERSE) {
            meter->value = 1.f;
            meter->last_value = 1.f;
        } else {
            meter->value = 0.f;
            meter->last_value = 0.f;
        }
        meter->vumeter_falloff = 0.f;
        meter->last_falloff = (long)0;
        meter->last_hold = (long)0;
        gtk_widget_queue_draw(GTK_WIDGET(meter));
    }
}

extern CalfVUMeterMode calf_vumeter_get_mode(CalfVUMeter *meter)
{
    return meter->mode;
}

extern void calf_vumeter_set_falloff(CalfVUMeter *meter, float value)
{
    if (value != meter->vumeter_falloff)
    {
        meter->vumeter_falloff = value;
        gtk_widget_queue_draw(GTK_WIDGET(meter));
    }
}

extern float calf_vumeter_get_falloff(CalfVUMeter *meter)
{
    return meter->vumeter_falloff;
}

extern void calf_vumeter_set_hold(CalfVUMeter *meter, float value)
{
    if (value != meter->vumeter_falloff)
    {
        meter->vumeter_hold = value;
        gtk_widget_queue_draw(GTK_WIDGET(meter));
    }
}

extern float calf_vumeter_get_hold(CalfVUMeter *meter)
{
    return meter->vumeter_hold;
}

