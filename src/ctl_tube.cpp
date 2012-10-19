/* Calf DSP Library
 * Tube custom control.
 * Copyright (C) 2010-2012 Markus Schmidt and others
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
#include <calf/ctl_tube.h>
#include <cairo/cairo.h>
#include <malloc.h>
#include <math.h>
#include <stdint.h>
#include <gdk/gdk.h>
#include <sys/time.h>

static gboolean
calf_tube_expose (GtkWidget *widget, GdkEventExpose *event)
{
    g_assert(CALF_IS_TUBE(widget));
    
    CalfTube  *self   = CALF_TUBE(widget);
    GdkWindow *window = widget->window;
    GtkStyle  *style  = gtk_widget_get_style(widget);
    cairo_t *c = gdk_cairo_create(GDK_DRAWABLE(window));
    
    int ox = 4, oy = 4, inner = 1, pad;
    int sx = widget->allocation.width - (ox * 2), sy = widget->allocation.height - (oy * 2);
    
    if( self->cache_surface == NULL ) {
        // looks like its either first call or the widget has been resized.
        // create the cache_surface.
        cairo_surface_t *window_surface = cairo_get_target( c );
        self->cache_surface = cairo_surface_create_similar( window_surface, 
                                  CAIRO_CONTENT_COLOR,
                                  widget->allocation.width,
                                  widget->allocation.height );

        // And render the meterstuff again.
        cairo_t *cache_cr = cairo_create( self->cache_surface );
        // theme background for reduced width and round borders
//        if(widget->style->bg_pixmap[0] == NULL) {
            gdk_cairo_set_source_color(cache_cr,&style->bg[GTK_STATE_NORMAL]);
//        } else {
//            gdk_cairo_set_source_pixbuf(cache_cr, GDK_PIXBUF(widget->style->bg_pixmap[0]), widget->allocation.x, widget->allocation.y + 20);
//        }
        cairo_paint(cache_cr);
        
        // outer (black)
        pad = 0;
        cairo_rectangle(cache_cr, pad, pad, sx + ox * 2 - pad * 2, sy + oy * 2 - pad * 2);
        cairo_set_source_rgb(cache_cr, 0, 0, 0);
        cairo_fill(cache_cr);
        
        // inner (bevel)
        pad = 1;
        cairo_rectangle(cache_cr, pad, pad, sx + ox * 2 - pad * 2, sy + oy * 2 - pad * 2);
        cairo_pattern_t *pat2 = cairo_pattern_create_linear (0, 0, 0, sy + oy * 2 - pad * 2);
        cairo_pattern_add_color_stop_rgba (pat2, 0, 0.23, 0.23, 0.23, 1);
        cairo_pattern_add_color_stop_rgba (pat2, 0.5, 0, 0, 0, 1);
        cairo_set_source (cache_cr, pat2);
        cairo_fill(cache_cr);
        cairo_pattern_destroy(pat2);
        
        cairo_rectangle(cache_cr, ox, oy, sx, sy);
        cairo_set_source_rgb (cache_cr, 0, 0, 0);
        cairo_fill(cache_cr);
        
        cairo_surface_t *image;
        switch(self->direction) {
            case 1:
                // vertical
                switch(self->size) {
                    default:
                    case 1:
                        image = cairo_image_surface_create_from_png (PKGLIBDIR "tubeV1.png");
                        break;
                    case 2:
                        image = cairo_image_surface_create_from_png (PKGLIBDIR "tubeV2.png");
                        break;
                }
                break;
            default:
            case 2:
                // horizontal
                switch(self->size) {
                    default:
                    case 1:
                        image = cairo_image_surface_create_from_png (PKGLIBDIR "tubeH1.png");
                        break;
                    case 2:
                        image = cairo_image_surface_create_from_png (PKGLIBDIR "tubeH2.png");
                        break;
                }
                break;
        }
        cairo_set_source_surface (cache_cr, image, widget->allocation.width / 2 - sx / 2 + inner, widget->allocation.height / 2 - sy / 2 + inner);
        cairo_paint (cache_cr);
        cairo_surface_destroy (image);
        cairo_destroy( cache_cr );
    }
    
    cairo_set_source_surface( c, self->cache_surface, 0,0 );
    cairo_paint( c );
    
    // get microseconds
    timeval tv;
    gettimeofday(&tv, 0);
    long time = tv.tv_sec * 1000 * 1000 + tv.tv_usec;
    
    // limit to 1.f
    float value_orig = self->value > 1.f ? 1.f : self->value;
    value_orig = value_orig < 0.f ? 0.f : value_orig;
    float value = 0.f;
    
    float s = ((float)(time - self->last_falltime) / 1000000.0);
    float m = self->last_falloff * s * 2.5;
    self->last_falloff -= m;
    // new max value?
    if(value_orig > self->last_falloff) {
        self->last_falloff = value_orig;
    }
    value = self->last_falloff;
    self->last_falltime = time;
    self->falling = self->last_falloff > 0.000001;
    cairo_pattern_t *pat;
    // draw upper light
    switch(self->direction) {
        case 1:
            // vertical
            cairo_arc(c, ox + sx * 0.5, oy + sy * 0.2, sx, 0, 2 * M_PI);
            pat = cairo_pattern_create_radial (ox + sx * 0.5, oy + sy * 0.2, 3, ox + sx * 0.5, oy + sy * 0.2, sx);
            break;
        default:
        case 2:
            // horizontal
            cairo_arc(c, ox + sx * 0.8, oy + sy * 0.5, sy, 0, 2 * M_PI);
            pat = cairo_pattern_create_radial (ox + sx * 0.8, oy + sy * 0.5, 3, ox + sx * 0.8, oy + sy * 0.5, sy);
            break;
    }
    cairo_pattern_add_color_stop_rgba (pat, 0,    1,    1,    1,    value);
    cairo_pattern_add_color_stop_rgba (pat, 0.3,  1,   0.8,  0.3, value * 0.4);
    cairo_pattern_add_color_stop_rgba (pat, 0.31, 0.9, 0.5,  0.1,  value * 0.5);
    cairo_pattern_add_color_stop_rgba (pat, 1,    0.0, 0.2,  0.7,  0);
    cairo_set_source (c, pat);
    cairo_fill(c);
    // draw lower light
    switch(self->direction) {
        case 1:
            // vertical
            cairo_arc(c, ox + sx * 0.5, oy + sy * 0.75, sx / 2, 0, 2 * M_PI);
            pat = cairo_pattern_create_radial (ox + sx * 0.5, oy + sy * 0.75, 2, ox + sx * 0.5, oy + sy * 0.75, sx / 2);
            break;
        default:
        case 2:
            // horizontal
            cairo_arc(c, ox + sx * 0.25, oy + sy * 0.5, sy / 2, 0, 2 * M_PI);
            pat = cairo_pattern_create_radial (ox + sx * 0.25, oy + sy * 0.5, 2, ox + sx * 0.25, oy + sy * 0.5, sy / 2);
            break;
    }
    cairo_pattern_add_color_stop_rgba (pat, 0,    1,    1,    1,    value);
    cairo_pattern_add_color_stop_rgba (pat, 0.3,  1,   0.8,  0.3, value * 0.4);
    cairo_pattern_add_color_stop_rgba (pat, 0.31, 0.9, 0.5,  0.1,  value * 0.5);
    cairo_pattern_add_color_stop_rgba (pat, 1,    0.0, 0.2,  0.7,  0);
    cairo_set_source (c, pat);
    cairo_fill(c);
    cairo_destroy(c);
    return TRUE;
}

static void
calf_tube_size_request (GtkWidget *widget,
                           GtkRequisition *requisition)
{
    g_assert(CALF_IS_TUBE(widget));

    CalfTube *self = CALF_TUBE(widget);
    switch(self->direction) {
        case 1:
            switch(self->size) {
                case 1:
                    widget->requisition.width = 82;
                    widget->requisition.height = 130;
                    break;
                default:
                case 2:
                    widget->requisition.width = 130;
                    widget->requisition.height = 210;
                    break;
            }
            break;
        default:
        case 2:
            switch(self->size) {
                case 1:
                    widget->requisition.width = 130;
                    widget->requisition.height = 82;
                    break;
                default:
                case 2:
                    widget->requisition.width = 210;
                    widget->requisition.height = 130;
                    break;
            }
            break;
    }
}

static void
calf_tube_size_allocate (GtkWidget *widget,
                           GtkAllocation *allocation)
{
    g_assert(CALF_IS_TUBE(widget));
    CalfTube *tube = CALF_TUBE(widget);
    
    GtkWidgetClass *parent_class = (GtkWidgetClass *) g_type_class_peek_parent( CALF_TUBE_GET_CLASS( tube ) );

    parent_class->size_allocate( widget, allocation );

    if( tube->cache_surface )
        cairo_surface_destroy( tube->cache_surface );
    tube->cache_surface = NULL;
}

static void
calf_tube_class_init (CalfTubeClass *klass)
{
    // GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);
    widget_class->expose_event = calf_tube_expose;
    widget_class->size_request = calf_tube_size_request;
    widget_class->size_allocate = calf_tube_size_allocate;
}

static void
calf_tube_init (CalfTube *self)
{
    GtkWidget *widget = GTK_WIDGET(self);
    GTK_WIDGET_SET_FLAGS (GTK_WIDGET(self), GTK_CAN_FOCUS);
    switch(self->direction) {
        case 1:
            switch(self->size) {
                case 1:
                    widget->requisition.width = 82;
                    widget->requisition.height = 130;
                    break;
                default:
                case 2:
                    widget->requisition.width = 130;
                    widget->requisition.height = 210;
                    break;
            }
            break;
        default:
        case 2:
            switch(self->size) {
                case 1:
                    widget->requisition.width = 130;
                    widget->requisition.height = 82;
                    break;
                default:
                case 2:
                    widget->requisition.width = 210;
                    widget->requisition.height = 130;
                    break;
            }
            break;
    }
    self->falling = false;
    self->cache_surface = NULL;
}

GtkWidget *
calf_tube_new()
{
    GtkWidget *widget = GTK_WIDGET( g_object_new (CALF_TYPE_TUBE, NULL ));
    return widget;
}

extern void calf_tube_set_value(CalfTube *tube, float value)
{
    if (value != tube->value or tube->falling)
    {
        tube->value = value;
        gtk_widget_queue_draw(GTK_WIDGET(tube));
    }
}

GType
calf_tube_get_type (void)
{
    static GType type = 0;
    if (!type) {
        
        static const GTypeInfo type_info = {
            sizeof(CalfTubeClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            (GClassInitFunc)calf_tube_class_init,
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof(CalfTube),
            0,    /* n_preallocs */
            (GInstanceInitFunc)calf_tube_init
        };
        
        for (int i = 0; ; i++) {
            char *name = g_strdup_printf("CalfTube%u%d", 
                ((unsigned int)(intptr_t)calf_tube_class_init) >> 16, i);
            if (g_type_from_name(name)) {
                free(name);
                continue;
            }
            type = g_type_register_static( GTK_TYPE_DRAWING_AREA,
                                           name,
                                           &type_info,
                                           (GTypeFlags)0);
            free(name);
            break;
        }
    }
    return type;
}
