#!/usr/bin/python

templ_h = """// Header
#ifndef [PREF]_CTL_[TYPE]_H
#define [PREF]_CTL_[TYPE]_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define [PREF]_TYPE_[TYPE]          ([pref]_[type]_get_type())
#define [PREF]_[TYPE](obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), [PREF]_TYPE_[TYPE], [Pref][Type]))
#define [PREF]_IS_[TYPE](obj)       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), [PREF]_TYPE_[TYPE]))
#define [PREF]_[TYPE]_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass),  [PREF]_TYPE_[TYPE], [Pref][Type]Class))
#define [PREF]_IS_[TYPE]_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((klass),  [PREF]_TYPE_[TYPE]))

/// Instance object for [Pref][Type]
struct [Pref][Type]
{
    GtkWidget parent;
};

/// Class object for [Pref][Type]
struct [Pref][Type]Class
{
    GtkWidgetClass parent_class;
};

/// Create new [Pref][Type]
extern GtkWidget *[pref]_[type]_new();

/// Get GObject type
extern GType [pref]_[type]_get_type();

G_END_DECLS

#endif
"""

templ_cpp = """// Source

GtkWidget *
[preftype]_new()
{
    GtkWidget *widget = GTK_WIDGET( g_object_new ([PREF]_TYPE_[TYPE], NULL ));
    return widget;
}

static gboolean
[preftype]_expose (GtkWidget *widget, GdkEventExpose *event)
{
    g_assert([PREF]_IS_[TYPE](widget));
    
    [PrefType] *self = [PREFTYPE](widget);
    GdkWindow *window = widget->window;
    cairo_t *c = gdk_cairo_create(GDK_DRAWABLE(window));
    cairo_destroy(c);

    return TRUE;
}

static void
[preftype]_realize(GtkWidget *widget)
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
[preftype]_size_request (GtkWidget *widget,
                           GtkRequisition *requisition)
{
    g_assert([PREF]_IS_[TYPE](widget));
    
    // width/height is hardwired at 40px now
    requisition->width = ...;
    requisition->height = ...;
}

static void
[preftype]_size_allocate (GtkWidget *widget,
                           GtkAllocation *allocation)
{
    g_assert([PREF]_IS_[TYPE](widget));
    
    widget->allocation = *allocation;
    
    if (GTK_WIDGET_REALIZED(widget))
        gdk_window_move_resize(widget->window, allocation->x, allocation->y, allocation->width, allocation->height );
}

static void
[preftype]_class_init ([PrefType]Class *klass)
{
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);
    widget_class->realize = [preftype]_realize;
    widget_class->expose_event = [preftype]_expose;
    widget_class->size_request = [preftype]_size_request;
    widget_class->size_allocate = [preftype]_size_allocate;
    // widget_class->button_press_event = [preftype]_button_press;
    // widget_class->button_release_event = [preftype]_button_release;
    // widget_class->motion_notify_event = [preftype]_pointer_motion;
    // widget_class->key_press_event = [preftype]_key_press;
    // widget_class->scroll_event = [preftype]_scroll;
}

static void
[preftype]_init ([PrefType] *self)
{
    GtkWidget *widget = GTK_WIDGET(self);
    GTK_WIDGET_SET_FLAGS (GTK_WIDGET(self), GTK_CAN_FOCUS);
}

GType
[preftype]_get_type (void)
{
    static GType type = 0;
    if (!type) {
        
        static const GTypeInfo type_info = {
            sizeof([PrefType]Class),
            NULL, /* base_init */
            NULL, /* base_finalize */
            (GClassInitFunc)[preftype]_class_init,
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof([PrefType]),
            0,    /* n_preallocs */
            (GInstanceInitFunc)[preftype]_init
        };
        
        for (int i = 0; ; i++) {
            char *name = g_strdup_printf("[PrefType]%u%d", 
                ((unsigned int)(intptr_t)[preftype]_class_init) >> 16, i);
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
"""

def subst(text, pref, type):
    text = text.replace("[pref]", pref.lower()).replace("[PREF]", pref.upper()).replace("[Pref]", pref)
    text = text.replace("[type]", type.lower()).replace("[TYPE]", type.upper()).replace("[Type]", type)    
    text = text.replace("[preftype]", pref.lower()+"_"+type.lower()).replace("[PrefType]", pref+type)
    text = text.replace("[PREFTYPE]", pref.upper()+"_"+type.upper())
    return text
    
print subst(templ_h, "Calf", "Led")
print subst(templ_cpp, "Calf", "Led")
