/* Calf DSP Library
 * Custom controls (line graph, knob).
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
 
#include <calf/ctl_meterscale.h>

using namespace calf_plugins;
using namespace dsp;


///////////////////////////////////////// meter scale ///////////////////////////////////////////////


GtkWidget *
calf_meter_scale_new()
{
    GtkWidget *widget = GTK_WIDGET( g_object_new (CALF_TYPE_METER_SCALE, NULL ));
    //CalfMeterScale *self = CALF_METER_SCALE(widget);
    return widget;
}
static gboolean
calf_meter_scale_expose (GtkWidget *widget, GdkEventExpose *event)
{
    g_assert(CALF_IS_METER_SCALE(widget));
    CalfMeterScale *ms = CALF_METER_SCALE(widget);
    if (gtk_widget_is_drawable (widget)) {
        
        GdkWindow *window = widget->window;
        cairo_t *cr = gdk_cairo_create(GDK_DRAWABLE(window));
        cairo_text_extents_t extents;
        
        double ox = widget->allocation.x;
        double oy = widget->allocation.y;
        double width  = widget->allocation.width;
        double height = widget->allocation.height;
        double xthick = widget->style->xthickness;
        double text_w = 0, bar_x = 0, bar_width = 0, bar_y = 0;
        float r, g, b;
        double text_m = 3;
        double dot_s  = 2;
        double dot_m  = 2;
        double dot_y  = 0;
        double dot_y2 = 0;
        cairo_rectangle(cr, ox, oy, width, height);
        cairo_clip(cr);
        
        if (ms->position) {
            cairo_select_font_face(cr, "cairo:sans-serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
            cairo_set_font_size(cr, 8);
            cairo_text_extents(cr, "-88.88", &extents);
            text_w = extents.width;
        }
        switch(ms->position) {
            case 1:
                // label on top
                bar_x = ox + xthick;
                bar_width = width - 2 * xthick;
                break;
            case 2:
                // label right
                bar_x = ox + xthick;
                bar_width = width - text_w - 2 * xthick - 2 * text_m;
                break;
            case 3:
                // label bottom
                bar_x = ox + xthick;
                bar_width = width - 2 * xthick;
                break;
            case 4:
                // label left
                bar_x = ox + xthick + text_w + 2 * text_m;
                bar_width = width - text_w - 2 * xthick - 2 * text_m;
                break;
        }
        
        switch (ms->dots) {
            case 0:
            default:
                // no ticks
                bar_y = height / 2;
                break;
            case 1:
                // tick top
                bar_y = oy + dot_s + dot_m + extents.height;
                dot_y = oy + dot_s / 2;
                break;
            case 2:
                // ticks bottom
                bar_y = oy + height - dot_s - dot_m - extents.height + extents.y_bearing;
                dot_y = oy + height - dot_s / 2;
                break;
            case 3:
                // ticks center
                bar_y = oy + height / 2 - extents.y_bearing / 2;
                dot_y = oy + height - dot_s / 2;
                dot_y2 = oy + dot_s / 2;
                break;
        }
        
        const unsigned int s = ms->marker.size();
        get_fg_color(widget, NULL, &r, &g, &b);
        cairo_set_source_rgb(cr, r, g, b);
        for (unsigned int i = 0; i < s; i++) {
            double val = log10(1 + ms->marker[i] * 9);
            if (ms->dots) {
                cairo_arc(cr, bar_x + bar_width * val, dot_y, dot_s / 2, 0, 2*M_PI);
                cairo_fill(cr);
            }
            if (ms->dots == 3) {
                cairo_arc(cr, bar_x + bar_width * val, dot_y2, dot_s / 2, 0, 2*M_PI);
                cairo_fill(cr);
            }
            char str[32];
            if (val < 1.0 / 32768.0)
                snprintf(str, sizeof(str), "-inf");
            else
                snprintf(str, sizeof(str), "%.f", amp2dB(ms->marker[i]));
            cairo_text_extents(cr, str, &extents);
            cairo_move_to(cr,
                std::min(ox + width, std::max(ox, bar_x + bar_width * val - extents.width / 2)), bar_y);
            cairo_show_text(cr, str);
        }
        cairo_destroy(cr);
    }
    return FALSE;
}

static void
calf_meter_scale_size_request (GtkWidget *widget,
                           GtkRequisition *requisition)
{
    g_assert(CALF_IS_METER_SCALE(widget));
    CalfMeterScale *self = CALF_METER_SCALE(widget);
    
    double ythick = widget->style->ythickness;
    double text_h = 8; // FIXME: Pango layout should be used here
    double dot_s  = 2;
    double dot_m  = 2;
    
    requisition->height = ythick*2 + text_h + (dot_m + dot_s) * (self->dots == 3 ? 2 : 1);
}

static void
calf_meter_scale_class_init (CalfMeterScaleClass *klass)
{
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);
    widget_class->expose_event = calf_meter_scale_expose;
    widget_class->size_request = calf_meter_scale_size_request;
}

static void
calf_meter_scale_init (CalfMeterScale *self)
{
    GtkWidget *widget = GTK_WIDGET(self);
    gtk_widget_set_has_window(widget, FALSE);
    widget->requisition.width = 40;
    widget->requisition.height = 12;
    self->mode     = VU_STANDARD;
    self->position = 0;
    self->dots     = 0;
}

GType
calf_meter_scale_get_type (void)
{
    static GType type = 0;
    if (!type) {
        static const GTypeInfo type_info = {
            sizeof(CalfMeterScaleClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            (GClassInitFunc)calf_meter_scale_class_init,
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof(CalfMeterScale),
            0,    /* n_preallocs */
            (GInstanceInitFunc)calf_meter_scale_init
        };

        for (int i = 0; ; i++) {
            //char *name = g_strdup_printf("CalfMeterScale%u%d", 
                //((unsigned int)(intptr_t)calf_meter_scale_class_init) >> 16, i);
            const char *name = "CalfMeterScale";
            if (g_type_from_name(name)) {
                //free(name);
                continue;
            }
            type = g_type_register_static(GTK_TYPE_DRAWING_AREA,
                                          name,
                                          &type_info,
                                          (GTypeFlags)0);
            //free(name);
            break;
        }
    }
    return type;
}
