/* Calf DSP Library
 * Barely started keyboard widget. Planned to be usable as
 * a ruler for curves, and possibly as input widget in future 
 * as well (that's what event sink interface is for, at least).
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
#include <calf/ctl_keyboard.h>
#include <stdint.h>
#include <stdlib.h>

static const int semitones_b[] = { 1, 3, -1, 6, 8, 10, -1 };
static const int semitones_w[] = { 0, 2, 4, 5, 7, 9, 11 };


GtkWidget *
calf_keyboard_new()
{
    GtkWidget *widget = GTK_WIDGET( g_object_new (CALF_TYPE_KEYBOARD, NULL ));
    return widget;
}

static gboolean
calf_keyboard_expose (GtkWidget *widget, GdkEventExpose *event)
{
    g_assert(CALF_IS_KEYBOARD(widget));
    
    cairo_pattern_t *pat;
    CalfKeyboard *self = CALF_KEYBOARD(widget);
    GdkWindow *window = widget->window;
    cairo_t *c = gdk_cairo_create(GDK_DRAWABLE(window));
    int sy = widget->allocation.height - 1;
    cairo_set_line_join(c, CAIRO_LINE_JOIN_MITER);
    cairo_set_line_width(c, 1);
    
    for (int i = 0; i < self->nkeys; i++)
    {
        CalfKeyboard::KeyInfo ki = { 0.5 + 12 * i, 0.5, 12, sy, 12 * (i / 7) + semitones_w[i % 7], false };
        cairo_new_path(c);
        if (!self->sink->pre_draw(c, ki))
        {
            cairo_rectangle(c, ki.x, ki.y, ki.width, ki.y + ki.height);
            
            pat = cairo_pattern_create_linear (ki.x, ki.y, ki.x, ki.y + ki.height);
            cairo_pattern_add_color_stop_rgb (pat, 0.0, 0.25, 0.25, 0.2);
            cairo_pattern_add_color_stop_rgb (pat, 0.1, 0.957, 0.914, 0.925);
            cairo_pattern_add_color_stop_rgb (pat, 1.0, 0.796, 0.787, 0.662);
            cairo_set_source(c, pat);
            cairo_fill(c);
            
            cairo_set_source_rgba(c, 0, 0, 0, 0.5);
            
            if (!self->sink->pre_draw_outline(c, ki))
                cairo_stroke(c);
            else
                cairo_new_path(c);
            self->sink->post_draw(c, ki);
        }
    }

    for (int i = 0; i < self->nkeys - 1; i++)
    {
        if ((1 << (i % 7)) & 59)
        {
            CalfKeyboard::KeyInfo ki = { 8.5 + 12 * i, 0.5, 8, sy * 3 / 5, 12 * (i / 7) + semitones_b[i % 7], true };
            cairo_new_path(c);
            cairo_rectangle(c, ki.x, ki.y, ki.width, ki.height);
//            gdk_cairo_set_source_color(c, &scBlackKey);
            if (!self->sink->pre_draw(c, ki))
            {
                pat = cairo_pattern_create_linear (ki.x, ki.y, ki.x, ki.height + ki.y);
                cairo_pattern_add_color_stop_rgb (pat, 0.0, 0, 0, 0);
                cairo_pattern_add_color_stop_rgb (pat, 0.1, 0.27, 0.27, 0.27);
                cairo_pattern_add_color_stop_rgb (pat, 1.0, 0, 0, 0);
                cairo_set_source(c, pat);
                cairo_fill(c);
                
                pat = cairo_pattern_create_linear (ki.x + 1, ki.y, ki.x + 1, (int)(ki.height * 0.8 + ki.y));
                cairo_pattern_add_color_stop_rgb (pat, 0.0, 0, 0, 0);
                cairo_pattern_add_color_stop_rgb (pat, 0.1, 0.55, 0.55, 0.55);
                cairo_pattern_add_color_stop_rgb (pat, 0.5, 0.45, 0.45, 0.45);
                cairo_pattern_add_color_stop_rgb (pat, 0.5001, 0.35, 0.35, 0.35);
                cairo_pattern_add_color_stop_rgb (pat, 1.0, 0.25, 0.25, 0.25);
                cairo_set_source(c, pat);
                cairo_rectangle(c, ki.x + 1, ki.y, ki.width - 2, (int)(ki.height * 0.8 + ki.y));
                cairo_fill(c);
                
                self->sink->post_draw(c, ki);
            }
        }
    }
    
    pat = cairo_pattern_create_linear (widget->allocation.x, widget->allocation.y, widget->allocation.x, (int)(widget->allocation.height * 0.2 + widget->allocation.y));
    cairo_pattern_add_color_stop_rgba (pat, 0.0, 0, 0, 0, 0.4);
    cairo_pattern_add_color_stop_rgba (pat, 1.0, 0, 0, 0, 0);
    cairo_rectangle(c, widget->allocation.x, widget->allocation.y, widget->allocation.width, (int)(widget->allocation.height * 0.2));
    cairo_set_source(c, pat);
    cairo_fill(c);
    
    self->sink->post_all(c);
    
    cairo_destroy(c);

    return TRUE;
}

static void
calf_keyboard_realize(GtkWidget *widget)
{
    GTK_WIDGET_SET_FLAGS(widget, GTK_REALIZED);

    GdkWindowAttr attributes;
    attributes.event_mask = GDK_EXPOSURE_MASK | GDK_BUTTON1_MOTION_MASK | 
        GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK | 
        GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK;
    attributes.x = widget->allocation.x;
    attributes.y = widget->allocation.y;
    attributes.width = widget->allocation.width;
    attributes.height = widget->allocation.height;
    attributes.wclass = GDK_INPUT_OUTPUT;
    attributes.window_type = GDK_WINDOW_CHILD;

    widget->window = gdk_window_new(gtk_widget_get_parent_window (widget), &attributes, GDK_WA_X | GDK_WA_Y);

    gdk_window_set_user_data(widget->window, widget);
    widget->style = gtk_style_attach(widget->style, widget->window);
}

static void
calf_keyboard_size_request (GtkWidget *widget,
                           GtkRequisition *requisition)
{
    CalfKeyboard *self = CALF_KEYBOARD(widget);
    g_assert(CALF_IS_KEYBOARD(widget));
    
    requisition->width = 12 * self->nkeys + 1;
    requisition->height = 40;
}

static void
calf_keyboard_size_allocate (GtkWidget *widget,
                           GtkAllocation *allocation)
{
    // CalfKeyboard *self = CALF_KEYBOARD(widget);
    g_assert(CALF_IS_KEYBOARD(widget));
    widget->allocation = *allocation;
    widget->allocation.width = widget->requisition.width;
    
    if (GTK_WIDGET_REALIZED(widget))
        gdk_window_move_resize(widget->window, 
            allocation->x + (allocation->width - widget->allocation.width) / 2, allocation->y, 
            widget->allocation.width, allocation->height );
}

static gboolean
calf_keyboard_key_press (GtkWidget *widget, GdkEventKey *event)
{
    g_assert(CALF_IS_KEYBOARD(widget));
    CalfKeyboard *self = CALF_KEYBOARD(widget);
    if (!self->sink)
        return FALSE;
    return FALSE;
}

int
calf_keyboard_pos_to_note (CalfKeyboard *kb, int x, int y, int *vel = NULL)
{
    // first try black keys
    if (y <= kb->parent.allocation.height * 3 / 5 && x >= 0 && (x - 8) % 12 < 8)
    {
        int blackkey = (x - 8) / 12;
        if (blackkey < kb->nkeys && (59 & (1 << (blackkey % 7))))
        {
            return semitones_b[blackkey % 7] + 12 * (blackkey / 7);
        }
    }
    // if not a black key, then which white one?
    int whitekey = x / 12;
    // semitones within octave + 12 semitones per octave
    return semitones_w[whitekey % 7] + 12 * (whitekey / 7);
}

static gboolean
calf_keyboard_button_press (GtkWidget *widget, GdkEventButton *event)
{
    g_assert(CALF_IS_KEYBOARD(widget));
    CalfKeyboard *self = CALF_KEYBOARD(widget);
    if (!self->interactive)
        return FALSE;
    gtk_widget_grab_focus(widget);
    int vel = 127;
    self->last_key = calf_keyboard_pos_to_note(self, (int)event->x, (int)event->y, &vel);
    if (self->last_key != -1)
        self->sink->note_on(self->last_key, vel);
    return FALSE;
}

static gboolean
calf_keyboard_button_release (GtkWidget *widget, GdkEventButton *event)
{
    g_assert(CALF_IS_KEYBOARD(widget));
    CalfKeyboard *self = CALF_KEYBOARD(widget);
    if (!self->interactive)
        return FALSE;
    if (self->last_key != -1)
        self->sink->note_off(self->last_key);
    return FALSE;
}

static gboolean
calf_keyboard_pointer_motion (GtkWidget *widget, GdkEventMotion *event)
{
    g_assert(CALF_IS_KEYBOARD(widget));
    CalfKeyboard *self = CALF_KEYBOARD(widget);
    if (!self->interactive)
        return FALSE;
    int vel = 127;
    int key = calf_keyboard_pos_to_note(self, (int)event->x, (int)event->y, &vel);
    if (key != self->last_key)
    {
        if (self->last_key != -1)
            self->sink->note_off(self->last_key);
        self->last_key = key;
        if (self->last_key != -1)
            self->sink->note_on(self->last_key, vel);
    }
    return FALSE;
}

static void
calf_keyboard_class_init (CalfKeyboardClass *klass)
{
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);
    widget_class->realize = calf_keyboard_realize;
    widget_class->size_allocate = calf_keyboard_size_allocate;
    widget_class->expose_event = calf_keyboard_expose;
    widget_class->size_request = calf_keyboard_size_request;
    widget_class->button_press_event = calf_keyboard_button_press;
    widget_class->button_release_event = calf_keyboard_button_release;
    widget_class->motion_notify_event = calf_keyboard_pointer_motion;
    widget_class->key_press_event = calf_keyboard_key_press;
    // widget_class->scroll_event = calf_keyboard_scroll;
}

static void
calf_keyboard_init (CalfKeyboard *self)
{
    static CalfKeyboard::EventAdapter default_sink;
    GtkWidget *widget = GTK_WIDGET(self);
    g_assert(CALF_IS_KEYBOARD(widget));
    GTK_WIDGET_SET_FLAGS (GTK_WIDGET(self), GTK_CAN_FOCUS);
    self->nkeys = 7 * 3 + 1;
    self->sink = &default_sink;
    self->last_key = -1;
}

GType
calf_keyboard_get_type (void)
{
    static GType type = 0;
    if (!type) {
        
        static const GTypeInfo type_info = {
            sizeof(CalfKeyboardClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            (GClassInitFunc)calf_keyboard_class_init,
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof(CalfKeyboard),
            0,    /* n_preallocs */
            (GInstanceInitFunc)calf_keyboard_init
        };
        
        for (int i = 0; ; i++) {
            char *name = g_strdup_printf("CalfKeyboard%u%d", 
                ((unsigned int)(intptr_t)calf_keyboard_class_init) >> 16, i);
            if (g_type_from_name(name)) {
                free(name);
                continue;
            }
            type = g_type_register_static(GTK_TYPE_WIDGET,
                                          name,
                                          &type_info,
                                          (GTypeFlags)0);
            free(name);
            break;
        }
    }
    return type;
}

