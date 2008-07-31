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
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#ifndef CALF_CTL_KEYBOARD_H
#define CALF_CTL_KEYBOARD_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define CALF_TYPE_KEYBOARD          (calf_keyboard_get_type())
#define CALF_KEYBOARD(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), CALF_TYPE_KEYBOARD, CalfKeyboard))
#define CALF_IS_KEYBOARD(obj)       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CALF_TYPE_KEYBOARD))
#define CALF_KEYBOARD_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass),  CALF_TYPE_KEYBOARD, CalfKeyboardClass))
#define CALF_IS_KEYBOARD_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((klass),  CALF_TYPE_KEYBOARD))

struct CalfKeyboard
{
    struct KeyInfo
    {
        double x, y, width, height;
        int note;
        bool black;
    };
    
    struct EventSink
    {
        virtual void set_instance(CalfKeyboard *kb)=0;
        virtual bool pre_draw(cairo_t *c, KeyInfo &ki)=0;
        virtual bool pre_draw_outline(cairo_t *c, KeyInfo &ki)=0;
        virtual void post_draw(cairo_t *c, KeyInfo &ki)=0;
        virtual void post_all(cairo_t *c)=0;
        virtual void note_on(int note, int vel) = 0;
        virtual void note_off(int note) = 0;
    };

    struct EventAdapter: public EventSink
    {
        CalfKeyboard *kb;
        virtual void set_instance(CalfKeyboard *_kb) { kb = _kb; }
        virtual bool pre_draw(cairo_t *c, KeyInfo &ki) { return false; }
        virtual bool pre_draw_outline(cairo_t *c, KeyInfo &ki) { return false; }
        virtual void post_draw(cairo_t *c, KeyInfo &ki) {}
        virtual void post_all(cairo_t *c) {}
        virtual void note_on(int note, int vel) {}
        virtual void note_off(int note) {}
    };

    struct EventTester: public EventAdapter
    {
        virtual bool pre_draw(cairo_t *c, KeyInfo &ki) { if (ki.note == 60) cairo_set_source_rgb(c, 1.0, 1.0, 0.5); return false; }
        virtual void post_draw(cairo_t *c, KeyInfo &ki) { 
            if (ki.note % 12 != 0 && ki.note % 12 != 3 && ki.note % 12 != 7)
                return;
            cairo_rectangle(c, ki.x + ki.width / 2 - 2, ki.y + ki.height - 8, 4, 4);
            if (ki.black)
                cairo_set_source_rgb(c, 1.0, 1.0, 1.0);
            else
                cairo_set_source_rgb(c, 0.0, 0.0, 0.0);
            cairo_fill(c); 
        }
        virtual void note_on(int note, int vel) { g_message("note on %d %d", note, vel); }
        virtual void note_off(int note) { g_message("note off %d", note); }
    };

    GtkWidget parent;
    int nkeys;
    EventSink *sink;
    int last_key;
    bool interactive;
};

struct CalfKeyboardClass
{
    GtkWidgetClass parent_class;
};

extern GtkWidget *calf_keyboard_new();

extern GType calf_keyboard_get_type();

G_END_DECLS

#endif

