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
 
#include <calf/ctl_tuner.h>

using namespace calf_plugins;
using namespace dsp;


///////////////////////////////////////// tuner ///////////////////////////////////////////////


static void calf_tuner_create_dot(cairo_t *ctx, int dots, int dot, float rad)
{
    cairo_save(ctx);
    cairo_rotate(ctx, dot * M_PI / (dots * 8) * 2);
    cairo_move_to(ctx, 0, -rad);
    cairo_line_to(ctx, 0, 0);
    cairo_stroke(ctx);
    cairo_restore(ctx);
}

static void
calf_tuner_draw_background( GtkWidget *widget, cairo_t *ctx, int sx, int sy, int ox, int oy )
{
    int dw    = 2;
    int dm    = 1;
    int x0    = ox + 0.025;
    int x1    = ox + sx - 0.025;
    int a     = x1 - x0;
    int dots  = a * 0.5 / (dw + dm);
    float rad = sqrt(2.f) / 2.f * a;
    int cx    = ox + sx / 2;
    int cy    = ox + sy / 2;
    int ccy   = cy - sy / 3 + rad;
    
    display_background(widget, ctx, 0, 0, sx, sy, ox, oy);
    cairo_stroke(ctx);
    cairo_save(ctx);
    
    cairo_rectangle(ctx, ox * 2, oy * 2, sx - 2 * ox, sy - 2 * oy);
    cairo_clip(ctx);
    
    cairo_set_source_rgba(ctx, 0.35, 0.4, 0.2, 0.3);
    cairo_set_line_width(ctx, dw);
    cairo_translate(ctx, cx, ccy);
    
    for(int i = 2; i < dots + 2; i++) {
        calf_tuner_create_dot(ctx, dots, i, rad);
    }
    for(int i = -2; i > -dots - 2; i--) {
        calf_tuner_create_dot(ctx, dots, i, rad);
    }
    cairo_set_line_width(ctx, dw * 3);
    calf_tuner_create_dot(ctx, dots, 0, rad);
}

static void calf_tuner_draw_dot(cairo_t * ctx, float cents, int sx, int sy, int ox, int oy)
{
    cairo_rectangle(ctx, ox * 2, oy * 2, sx - 2 * ox, sy - 2 * oy);
    cairo_clip(ctx);
    
    int dw    = 2;
    int dm    = 1;
    int x0    = ox + 0.025;
    int x1    = ox + sx - 0.025;
    int a     = x1 - x0;
    int dots  = a * 0.5 / (dw + dm);
    int dot   = cents * 2.f * dots;
    float rad = sqrt(2.f) / 2.f * a;
    int cx    = ox + sx / 2;
    int cy    = ox + sy / 2;
    int ccy   = cy - sy / 3 + rad;
    
    int sign  = (dot > 0) - (dot < 0);
    int marg  = dot ? sign : 0;
    cairo_save(ctx);
    cairo_set_source_rgba(ctx, 0.35, 0.4, 0.2, 0.9);
    cairo_translate(ctx, cx, ccy);
    cairo_set_line_width(ctx, dw * (dot ? 1 : 3));
    calf_tuner_create_dot(ctx, dots, dot + marg, rad);
    cairo_restore(ctx);
}

static gboolean
calf_tuner_expose (GtkWidget *widget, GdkEventExpose *event)
{
    g_assert(CALF_IS_TUNER(widget));
    CalfTuner *tuner = CALF_TUNER(widget);
    
    //printf("%d %f\n", tuner->note, tuner->cents);
    
    // dimensions
    int ox = 5, oy = 5;
    int sx = widget->allocation.width - ox * 2, sy = widget->allocation.height - oy * 2;
    int marg = 10;
    int fpt  = 9;
    float fsize = fpt * sy / 25; // 9pt @ 25px height
    
    // cairo initialization stuff
    cairo_t *c = gdk_cairo_create(GDK_DRAWABLE(widget->window));
    cairo_t *ctx_back;
    
    if( tuner->background == NULL ) {
        // looks like its either first call or the widget has been resized.
        // create the background surface (stolen from line graph)...
        cairo_surface_t *window_surface = cairo_get_target(c);
        tuner->background = cairo_surface_create_similar(window_surface, 
                                  CAIRO_CONTENT_COLOR,
                                  widget->allocation.width,
                                  widget->allocation.height );
        
        // ...and draw some bling bling onto it...
        ctx_back = cairo_create(tuner->background);
        calf_tuner_draw_background(widget, ctx_back, sx, sy, ox, oy);
    } else {
        ctx_back = cairo_create(tuner->background);
    }
    
    cairo_set_source_surface(c, cairo_get_target(ctx_back), 0, 0);
    cairo_paint(c);
    
    calf_tuner_draw_dot(c, tuner->cents / 100, sx, sy, ox, oy);
    
    static const char notenames[] = "C\0\0C#\0D\0\0D#\0E\0\0F\0\0F#\0G\0\0G#\0A\0\0A#\0B\0\0";
    const char * note = notenames + (tuner->note % 12) * 3;
    int oct = int(tuner->note / 12) - 2;
    cairo_set_source_rgba(c, 0.35, 0.4, 0.2, 0.9);
    cairo_text_extents_t te;
    if (tuner->note) {
        // Note name
        cairo_select_font_face(c, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
        cairo_set_font_size(c, fsize);
        cairo_text_extents (c, note, &te);
        cairo_move_to (c, ox + marg - te.x_bearing, oy + marg - te.y_bearing);
        cairo_show_text (c, note);
        // octave
        char octn[20];
        sprintf(octn, "%d", oct);
        cairo_set_font_size(c, fsize / 2);
        cairo_text_extents (c, octn, &te);
        cairo_show_text(c, octn);
        // right hand side
        cairo_set_font_size(c, fsize / 4);
        cairo_select_font_face(c, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
        const char * mnotet = "MIDI Note: ";
        char mnotev[32];
        sprintf(mnotev, "%d", tuner->note);
        const char * centst = "Cents: ";
        char centsv[32];
        sprintf(centsv, "%.4f", tuner->cents);
        
        // calc text extents
        cairo_text_extents (c, mnotet, &te);
        int mtw = te.width;
        cairo_text_extents (c, "999", &te);
        int mvw = te.width;
        cairo_text_extents (c, centst, &te);
        int ctw = te.width;
        cairo_text_extents (c, "-9.9999", &te);
        int cvw = te.width;
        float xb = te.x_bearing;
        
        float tw = std::max(ctw, mtw);
        float vw = std::max(cvw, mvw);
        
        // draw MIDI note
        cairo_move_to(c, ox + sx - tw - vw - marg * 2, oy + marg - te.y_bearing);
        cairo_show_text(c, mnotet);
        cairo_move_to(c, ox + sx - vw - xb - marg, oy + marg - te.y_bearing);
        cairo_show_text(c, mnotev);
        // draw cents
        cairo_move_to(c, ox + sx - tw - vw - marg * 2, oy + marg + te.height + 5 - te.y_bearing);
        cairo_show_text(c, centst);
        cairo_move_to(c, ox + sx - vw - xb - marg, oy + marg + te.height + 5 - te.y_bearing);
        cairo_show_text(c, centsv);
    }
    
    cairo_destroy(c);
    cairo_destroy(ctx_back);
    return TRUE;
}

static void
calf_tuner_size_request (GtkWidget *widget,
                           GtkRequisition *requisition)
{
    g_assert(CALF_IS_TUNER(widget));
    // CalfLineGraph *lg = CALF_LINE_GRAPH(widget);
}

static void
calf_tuner_size_allocate (GtkWidget *widget,
                           GtkAllocation *allocation)
{
    g_assert(CALF_IS_TUNER(widget));
    CalfTuner *lg = CALF_TUNER(widget);

    if(lg->background)
        cairo_surface_destroy(lg->background);
    lg->background = NULL;
    
    widget->allocation = *allocation;
}

static void
calf_tuner_class_init (CalfTunerClass *klass)
{
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);
    widget_class->expose_event = calf_tuner_expose;
    widget_class->size_request = calf_tuner_size_request;
    widget_class->size_allocate = calf_tuner_size_allocate;
}

static void
calf_tuner_unrealize (GtkWidget *widget, CalfTuner *tuner)
{
    if( tuner->background )
        cairo_surface_destroy(tuner->background);
    tuner->background = NULL;
}

static void
calf_tuner_init (CalfTuner *self)
{
    GtkWidget *widget = GTK_WIDGET(self);
    widget->requisition.width = 40;
    widget->requisition.height = 40;
    self->background = NULL;
    g_signal_connect(GTK_OBJECT(widget), "unrealize", G_CALLBACK(calf_tuner_unrealize), (gpointer)self);
}

GtkWidget *
calf_tuner_new()
{
    return GTK_WIDGET(g_object_new (CALF_TYPE_TUNER, NULL));
}

GType
calf_tuner_get_type (void)
{
    static GType type = 0;
    if (!type) {
        static const GTypeInfo type_info = {
            sizeof(CalfTunerClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            (GClassInitFunc)calf_tuner_class_init,
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof(CalfTuner),
            0,    /* n_preallocs */
            (GInstanceInitFunc)calf_tuner_init
        };

        GTypeInfo *type_info_copy = new GTypeInfo(type_info);

        for (int i = 0; ; i++) {
            const char *name = "CalfTuner";
            //char *name = g_strdup_printf("CalfTuner%u%d", ((unsigned int)(intptr_t)calf_tuner_class_init) >> 16, i);
            if (g_type_from_name(name)) {
                //free(name);
                continue;
            }
            type = g_type_register_static( GTK_TYPE_DRAWING_AREA,
                                           name,
                                           type_info_copy,
                                           (GTypeFlags)0);
            //free(name);
            break;
        }
    }
    return type;
}
