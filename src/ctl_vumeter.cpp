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
#include <calf/primitives.h>
#include <calf/ctl_vumeter.h>
#include <gdk/gdkkeysyms.h>
#include <cairo/cairo.h>
#include <math.h>
#include <gdk/gdk.h>
#include <sys/time.h>
#include <string>


///////////////////////////////////////// vu meter ///////////////////////////////////////////////

static gboolean
calf_vumeter_expose (GtkWidget *widget, GdkEventExpose *event)
{
    g_assert(CALF_IS_VUMETER(widget));

    CalfVUMeter *vu = CALF_VUMETER(widget);
    GtkStyle	*style;
    style = gtk_widget_get_style(widget);
    cairo_t *c = gdk_cairo_create(GDK_DRAWABLE(widget->window));
    
    int width = widget->allocation.width; int height = widget->allocation.height;
    int border_x = 1; int border_y = 1; // outer border
    int space_x = 1; int space_y = 1; // inner border around led bar
    int led = 2; // single LED size
    int led_m = 1; // margin between LED
    int led_s = led + led_m; // size of LED with margin
    int led_x = 5; int led_y = 4; // position of first LED
    int led_w = width - 2 * led_x + led_m; // width of LED bar w/o text calc (additional led margin, is removed later; used for filling the led bar completely w/o margin gap)
    int led_h = height - 2 * led_y; // height of LED bar w/o text calc
    int text_x = 0; int text_y = 0;
    int text_w = 0; int text_h = 0;
    int text_m = 3; // text margin

    // only valid if vumeter is enabled
    cairo_text_extents_t extents;
    
    if(vu->vumeter_position) {
        cairo_select_font_face(c, "cairo:sans-serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
        cairo_set_font_size(c, 8);

        cairo_text_extents(c, "-88.88", &extents);
        text_w = extents.width;
        text_h = extents.height;
        switch(vu->vumeter_position) {
            case 1:
                text_x = width / 2 - text_w / 2;
                text_y = border_y + text_m - extents.y_bearing;
                led_y += text_h + text_m;
                led_h -= text_h + text_m;
                break;
            case 2:
                text_x = width - border_x - text_m * 2 - text_w;
                text_y = height / 2 - text_h / 2 - extents.y_bearing;
                led_w -= text_m * 2 + text_w;
                break;
            case 3:
                text_x = width / 2 - text_w / 2;
                text_y = height - border_y - text_m - text_h - extents.y_bearing;
                led_h -= text_m * 2 + text_h;
                break;
            case 4:
                text_x = border_x + text_m;
                text_y = height / 2 - text_h / 2 - extents.y_bearing;
                led_x += text_m * 2 + text_w;
                led_w -= text_m * 2 + text_w;
                break;
        }
    }
    
    led_w -= led_w % led_s + led_m; //round LED width to LED size and remove margin gap, width is filled with LED without margin gap now
    
    if( vu->cache_surface == NULL ) {
        // looks like its either first call or the widget has been resized.
        // create the cache_surface.
        cairo_surface_t *window_surface = cairo_get_target( c );
        vu->cache_surface = cairo_surface_create_similar( window_surface, 
                                  CAIRO_CONTENT_COLOR,
                                  widget->allocation.width,
                                  widget->allocation.height );

        // And render the meterstuff

        cairo_t *cache_cr = cairo_create( vu->cache_surface );
        gdk_cairo_set_source_color(cache_cr,&style->bg[GTK_STATE_NORMAL]);
        cairo_paint(cache_cr);
        
        // outer (black)
        cairo_rectangle(cache_cr, 0, 0, width, height);
        cairo_set_source_rgb(cache_cr, 0, 0, 0);
        cairo_fill(cache_cr);
        
        // inner (bevel)
        cairo_rectangle(cache_cr,
                        border_x,
                        border_y,
                        width - border_x * 2,
                        height - border_y * 2);
        cairo_pattern_t *pat2 = cairo_pattern_create_linear (border_x,
                                                             border_y,
                                                             border_x,
                                                             height - border_y * 2);
        cairo_pattern_add_color_stop_rgba (pat2, 0, 0.23, 0.23, 0.23, 1);
        cairo_pattern_add_color_stop_rgba (pat2, 0.5, 0, 0, 0, 1);
        cairo_set_source (cache_cr, pat2);
        cairo_fill(cache_cr);
        cairo_pattern_destroy(pat2);
        
        // border around LED
        cairo_rectangle(cache_cr,
                        led_x - space_x,
                        led_y - space_y,
                        led_w + space_x * 2,
                        led_h + space_y * 2);
        cairo_set_source_rgb (cache_cr, 0, 0, 0);
        cairo_fill(cache_cr);
        
        // LED bases
        cairo_set_line_width(cache_cr, 1);
        for (int x = led_x; x + led <= led_x + led_w; x += led_s)
        {
            float ts = (x - led_x) * 1.0 / led_w;
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
                case VU_STANDARD_CENTER:
                    if (ts < 0.25)
                        // 0.0 -> 0.25
                        // green: 0.f -> 1.f
                        r = 1, g = (ts) / 0.25, b = 0;
                    else if (ts > 0.75)
                        // 0.75 -> 1.0
                        // green: 1.f -> 0.f
                        r = 1, g = 1 - (ts - 0.75) / 0.25, b = 0;
                    else if (ts > 0.5)
                        // 0.5 -> 0.75
                        // red: 0.f -> 1.f
                        // green: 0.5 -> 1.f
                        // blue: 1.f -> 0.f
                        r = (ts - 0.5) / 0.25, g = 0.5 + (ts - 0.5) * 2.f, b = 1 - (ts - 0.5) / 0.25;
                    else
                        // 0.25 -> 0.5
                        // red: 1.f -> 0.f
                        // green: 1.f -> 0.5
                        // blue: 0.f -> 1.f
                        r = 1 - (ts - 0.25) / 0.25, g = 1.f - (ts * 2.f - .5f), b = (ts - 0.25) / 0.25;
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
                case VU_MONOCHROME_CENTER:
                    r = 0, g = 170.0 / 255.0, b = 1;
                    //                if (vu->value < ts || vu->value <= 0)
                    //                    r *= 0.5, g *= 0.5, b *= 0.5;
                    break;
            }
            GdkColor sc2 = { 0, (guint16)(65535 * r + 0.2), (guint16)(65535 * g), (guint16)(65535 * b) };
            GdkColor sc3 = { 0, (guint16)(65535 * r * 0.7), (guint16)(65535 * g * 0.7), (guint16)(65535 * b * 0.7) };
            gdk_cairo_set_source_color(cache_cr, &sc2);
            cairo_move_to(cache_cr, x + 0.5, led_y);
            cairo_line_to(cache_cr, x + 0.5, led_y + led_h);
            cairo_stroke(cache_cr);
            gdk_cairo_set_source_color(cache_cr, &sc3);
            cairo_move_to(cache_cr, x + 1.5, led_y + led_h);
            cairo_line_to(cache_cr, x + 1.5, led_y);
            cairo_stroke(cache_cr);
        }
        // create blinder pattern
        vu->pat = cairo_pattern_create_linear (led_x, led_y, led_x, led_y + led_h);
        cairo_pattern_add_color_stop_rgba (vu->pat, 0, 0.5, 0.5, 0.5, 0.6);
        cairo_pattern_add_color_stop_rgba (vu->pat, 0.4, 0, 0, 0, 0.6);
        cairo_pattern_add_color_stop_rgba (vu->pat, 0.401, 0, 0, 0, 0.8);
        cairo_pattern_add_color_stop_rgba (vu->pat, 1, 0, 0, 0, 0.6);
        cairo_destroy( cache_cr );
    }
    
    // draw LED blinder
    cairo_set_source_surface( c, vu->cache_surface, 0,0 );
    cairo_paint( c );
    cairo_set_source( c, vu->pat );
    
    // get microseconds
    timeval tv;
    gettimeofday(&tv, 0);
    long time = tv.tv_sec * 1000 * 1000 + tv.tv_usec;
    
    // limit to 1.f
    float value_orig = std::max(std::min(vu->value, 1.f), 0.f);
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
        vu->falling = vu->last_falloff > 0.00000001;
    } else {
        // falloff disabled
        vu->last_falloff = 0.f;
        vu->last_falltime = 0.f;
        value = value_orig;
        vu->falling = false;
    }
    if(vu->vumeter_hold > 0.0) {
        // peak hold timer
        if(time - (long)(vu->vumeter_hold * 1000 * 1000) > vu->last_hold) {
            // time's up, reset
            vu->last_value = value;
            vu->last_hold = time;
            vu->holding = false;
            vu->disp_value = value_orig;
        }
        if( vu->mode == VU_MONOCHROME_REVERSE ) {
            if(value < vu->last_value) {
                // value is above peak hold
                vu->last_value = value;
                vu->last_hold = time;
                vu->holding = true;
            }
            
            // blinder left -> hold LED
            int hold_x = round((vu->last_value) * (led_w + led_m)); // add last led_m removed earlier
            hold_x -= hold_x % led_s + led_m;
            hold_x = std::max(0, hold_x);
            cairo_rectangle( c, led_x, led_y, hold_x, led_h);
            
            // blinder hold LED -> value
            int val_x = round((1 - value) * (led_w + led_m)); // add last led_m removed earlier
            val_x -= val_x % led_s;
            int blind_x = std::min(hold_x + led_s, led_w);
            int blind_w = std::min(std::max(led_w - val_x - hold_x - led_s, 0), led_w);
            cairo_rectangle(c, led_x + blind_x, led_y, blind_w, led_h);
        } else  if( vu->mode == VU_STANDARD_CENTER ) {
            if(value > vu->last_value) {
                // value is above peak hold
                vu->last_value = value;
                vu->last_hold = time;
                vu->holding = true;
            }
            int val_x = round((1 - value) / 2.f * (led_w + led_m)); // add last led_m removed earlier
            cairo_rectangle(c, led_x, led_y, val_x, led_h);
            cairo_rectangle(c, led_x + led_w - val_x, led_y, val_x, led_h);
            
        } else {
            if(value > vu->last_value) {
                // value is above peak hold
                vu->last_value = value;
                vu->last_hold = time;
                vu->holding = true;
            }
            
            int hold_x = round((1 - vu->last_value) * (led_w + led_m)); // add last led_m removed earlier
            hold_x -= hold_x % led_s;
            int val_x = round(value * (led_w + led_m)); // add last led_m removed earlier
            val_x -= val_x % led_s;
            int blind_w = led_w - hold_x - led_s - val_x;
            blind_w = std::min(std::max(blind_w, 0), led_w);
            cairo_rectangle(c, led_x + val_x, led_y, blind_w, led_h);
            cairo_rectangle( c, led_x + led_w - hold_x, led_y, hold_x, led_h);
        }
    } else {
        // darken normally
        if( vu->mode == VU_MONOCHROME_REVERSE )
            cairo_rectangle( c, led_x, led_y, value * led_w, led_h);
        else if( vu->mode == VU_STANDARD_CENTER ) {
            int val_x = round((1 - value) / 2.f * (led_w + led_m)); // add last led_m removed earlier
            cairo_rectangle(c, led_x, led_y, val_x, led_h);
            cairo_rectangle(c, led_x + led_w - val_x, led_y, val_x, led_h);
        } else
            cairo_rectangle( c, led_x + value * led_w, led_y, led_w * (1 - value), led_h);
    }
    cairo_fill( c );
    
    if (vu->vumeter_position)
    {
        char str[32];
        if((vu->value > vu->disp_value and vu->mode != VU_MONOCHROME_REVERSE)
        or (vu->value < vu->disp_value and vu->mode == VU_MONOCHROME_REVERSE))
            vu->disp_value = vu->value;
        if (vu->disp_value < 1.0 / 32768.0)
            sprintf(str, "-inf");
        else
            sprintf(str, "%0.2f", dsp::amp2dB(vu->disp_value));
        // draw value as number
        cairo_text_extents(c, str, &extents);
        cairo_move_to(c, text_x + (text_w - extents.width) / 2.0, text_y);
        if(vu->disp_value > 1.f and vu->mode != VU_MONOCHROME_REVERSE)
            cairo_set_source_rgba (c, 1, 0, 0, 0.8);
        else
            cairo_set_source_rgba (c, 0, 0.7, 1, 0.8);
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
    CalfVUMeter *self = CALF_VUMETER(widget);
    requisition->width = self->vumeter_width;
    requisition->height = self->vumeter_height;
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
    widget->requisition.width =  self->vumeter_width;
    widget->requisition.height = self->vumeter_height;
    self->cache_surface = NULL;
    self->falling = false;
    self->holding = false;
    self->meter_width = 0;
    self->disp_value = 0.f;
    self->value = 0.f;
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
    if (value != meter->vumeter_hold)
    {
        meter->vumeter_hold = value;
        gtk_widget_queue_draw(GTK_WIDGET(meter));
    }
}

extern float calf_vumeter_get_hold(CalfVUMeter *meter)
{
    return meter->vumeter_hold;
}

extern void calf_vumeter_set_width(CalfVUMeter *meter, int value)
{
    if (value != meter->vumeter_width)
    {
        meter->vumeter_width = value;
        gtk_widget_queue_draw(GTK_WIDGET(meter));
    }
}

extern int calf_vumeter_get_width(CalfVUMeter *meter)
{
    return meter->vumeter_width;
}

extern void calf_vumeter_set_height(CalfVUMeter *meter, int value)
{
    if (value != meter->vumeter_height)
    {
        meter->vumeter_height = value;
        gtk_widget_queue_draw(GTK_WIDGET(meter));
    }
}

extern int calf_vumeter_get_height(CalfVUMeter *meter)
{
    return meter->vumeter_height;
}
extern void calf_vumeter_set_position(CalfVUMeter *meter, int value)
{
    if (value != meter->vumeter_height)
    {
        meter->vumeter_position = value;
        gtk_widget_queue_draw(GTK_WIDGET(meter));
    }
}

extern int calf_vumeter_get_position(CalfVUMeter *meter)
{
    return meter->vumeter_position;
}
