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

#include <calf/ctl_notebook.h>

using namespace calf_plugins;
using namespace dsp;


///////////////////////////////////////// notebook ///////////////////////////////////////////////


#define GTK_NOTEBOOK_PAGE(_glist_)         ((GtkNotebookPage *)((GList *)(_glist_))->data)
struct _GtkNotebookPage
{
  GtkWidget *child;
  GtkWidget *tab_label;
  GtkWidget *menu_label;
  GtkWidget *last_focus_child;  /* Last descendant of the page that had focus */

  guint default_menu : 1;   /* If true, we create the menu label ourself */
  guint default_tab  : 1;   /* If true, we create the tab label ourself */
  guint expand       : 1;
  guint fill         : 1;
  guint pack         : 1;
  guint reorderable  : 1;
  guint detachable   : 1;

  /* if true, the tab label was visible on last allocation; we track this so
   * that we know to redraw the tab area if a tab label was hidden then shown
   * without changing position */
  guint tab_allocated_visible : 1;

  GtkRequisition requisition;
  GtkAllocation allocation;

  gulong mnemonic_activate_signal;
  gulong notify_visible_handler;
};

GtkWidget *
calf_notebook_new()
{
    GtkWidget *widget = GTK_WIDGET( g_object_new (CALF_TYPE_NOTEBOOK, NULL ));
    return widget;
}
static gboolean
calf_notebook_expose (GtkWidget *widget, GdkEventExpose *event)
{
    g_assert(CALF_IS_NOTEBOOK(widget));
    
    GtkNotebook *notebook = GTK_NOTEBOOK(widget);
    CalfNotebook *nb = CALF_NOTEBOOK(widget);
    
    if (gtk_widget_is_drawable (widget)) {
        
        GdkWindow *window = widget->window;
        cairo_t *c = gdk_cairo_create(GDK_DRAWABLE(window));
        cairo_pattern_t *pat = NULL;
        
        int x  = widget->allocation.x;
        int y  = widget->allocation.y;
        int sx = widget->allocation.width;
        int sy = widget->allocation.height;
        int tx = widget->style->xthickness;
        int ty = widget->style->ythickness;
        int lh = 19;
        int bh = lh + 2 * ty;
        
        float r, g, b;
        float alpha;
        gtk_widget_style_get(widget, "background-alpha", &alpha, NULL);
        
        cairo_rectangle(c, x, y, sx, sy);
        cairo_clip(c);
        
        int add = 0;
        
        if (notebook->show_tabs) {
            GtkNotebookPage *page;
            GList *pages;
            
            gint sp;
            gtk_widget_style_get(widget, "tab-overlap", &sp, NULL);
            
            pages = notebook->children;
            
            int cn = 0;
            while (pages) {
                page = GTK_NOTEBOOK_PAGE (pages);
                pages = pages->next;
                if (page->tab_label->window == event->window &&
                    gtk_widget_is_drawable (page->tab_label)) {
                    int lx = page->tab_label->allocation.x;
                    int lw = page->tab_label->allocation.width;
                    
                    // fix the labels position
                    page->tab_label->allocation.y = y + ty;
                    page->tab_label->allocation.height = lh;
                    
                    // draw tab background
                    cairo_rectangle(c, lx - tx, y, lw + 2 * tx, bh);
                    get_base_color(widget, NULL, &r, &g, &b);
                    cairo_set_source_rgba(c, r,g,b, page != notebook->cur_page ? alpha / 2 : alpha);
                    cairo_fill(c);
                    
                    if (page == notebook->cur_page) {
                        // draw tab light
                        get_bg_color(widget, NULL, &r, &g, &b);
                        cairo_rectangle(c, lx - tx + 2, y + 2, lw + 2 * tx - 4, 2);
                        cairo_set_source_rgb(c, r, g, b);
                        cairo_fill(c);
                        
                        cairo_rectangle(c, lx - tx + 2, y + 1, lw + 2 * tx - 4, 1);
                        cairo_set_source_rgba(c, 0,0,0,0.5);
                        cairo_fill(c);
                        
                        cairo_rectangle(c, lx - tx + 2, y + 4, lw + 2 * tx - 4, 1);
                        cairo_set_source_rgba(c, 1,1,1,0.3);
                        cairo_fill(c);
                    
                    }
                    // draw labels
                    gtk_container_propagate_expose (GTK_CONTAINER (notebook), page->tab_label, event);
                }
                cn++;
            }
            add = bh;
        }
        
        // draw main body
        get_base_color(widget, NULL, &r, &g, &b);
        cairo_rectangle(c, x, y + add, sx, sy - add);
        cairo_set_source_rgba(c, r, g, b, alpha);
        cairo_fill(c);
        
        // draw frame
        cairo_rectangle(c, x + 0.5, y + add + 0.5, sx - 1, sy - add - 1);
        pat = cairo_pattern_create_linear(x, y + add, x, y + sy - add);
        cairo_pattern_add_color_stop_rgba(pat,   0,   0,   0,   0, 0.3);
        cairo_pattern_add_color_stop_rgba(pat, 0.5, 0.5, 0.5, 0.5,   0);
        cairo_pattern_add_color_stop_rgba(pat,   1,   1,   1,   1, 0.2);
        cairo_set_source (c, pat);
        cairo_set_line_width(c, 1);
        cairo_stroke_preserve(c);
                    
        int sw = gdk_pixbuf_get_width(nb->screw);
        int sh = gdk_pixbuf_get_height(nb->screw);
        
        // draw screws
        if (nb->screw) {
            gdk_cairo_set_source_pixbuf(c, nb->screw, x, y + add);
            cairo_fill_preserve(c);
            gdk_cairo_set_source_pixbuf(c, nb->screw, x + sx - sw, y + add);
            cairo_fill_preserve(c);
            gdk_cairo_set_source_pixbuf(c, nb->screw, x, y + sy - sh);
            cairo_fill_preserve(c);
            gdk_cairo_set_source_pixbuf(c, nb->screw, x + sx - sh, y + sy - sh);
            cairo_fill_preserve(c);
        }
        // propagate expose to all children
        if (notebook->cur_page)
            gtk_container_propagate_expose (GTK_CONTAINER (notebook),
                notebook->cur_page->child,
                event);
        
        cairo_pattern_destroy(pat);
        cairo_destroy(c);
        
    }
    return FALSE;
}

void
calf_notebook_set_pixbuf (CalfNotebook *self, GdkPixbuf *image)
{
    self->screw = image;
    gtk_widget_queue_draw(GTK_WIDGET(self));
}

static void
calf_notebook_class_init (CalfNotebookClass *klass)
{
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);
    widget_class->expose_event = calf_notebook_expose;
    gtk_widget_class_install_style_property(
        widget_class, g_param_spec_float("background-alpha", "Alpha Background", "Alpha of background",
        0.0, 1.0, 0.5, GParamFlags(G_PARAM_READWRITE)));
}

static void
calf_notebook_init (CalfNotebook *self)
{
    //GtkWidget *widget = GTK_WIDGET(self);
}

GType
calf_notebook_get_type (void)
{
    static GType type = 0;
    if (!type) {
        static const GTypeInfo type_info = {
            sizeof(CalfNotebookClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            (GClassInitFunc)calf_notebook_class_init,
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof(CalfNotebook),
            0,    /* n_preallocs */
            (GInstanceInitFunc)calf_notebook_init
        };

        for (int i = 0; ; i++) {
            const char *name = "CalfNotebook";
            //char *name = g_strdup_printf("CalfNotebook%u%d", 
                //((unsigned int)(intptr_t)calf_notebook_class_init) >> 16, i);
            if (g_type_from_name(name)) {
                //free(name);
                continue;
            }
            type = g_type_register_static(GTK_TYPE_NOTEBOOK,
                                          name,
                                          &type_info,
                                          (GTypeFlags)0);
            //free(name);
            break;
        }
    }
    return type;
}
