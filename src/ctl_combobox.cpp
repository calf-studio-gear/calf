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
 
#include <calf/ctl_combobox.h>

using namespace calf_plugins;
using namespace dsp;


///////////////////////////////////////// combo box ///////////////////////////////////////////////


//#define GTK_COMBO_BOX_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GTK_TYPE_COMBO_BOX, GtkComboBoxPrivate))

GtkWidget *
calf_combobox_new()
{
    GtkWidget *widget = GTK_WIDGET( g_object_new (CALF_TYPE_COMBOBOX, NULL ));
    GtkCellRenderer *column = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(widget), column, TRUE);
    gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(widget), column,
                                   "text", 0,
                                   NULL);
    return widget;
}
static gboolean
calf_combobox_expose (GtkWidget *widget, GdkEventExpose *event)
{
    g_assert(CALF_IS_COMBOBOX(widget));
    
    if (gtk_widget_is_drawable (widget)) {
        
        int padx = widget->style->xthickness;
        int pady = widget->style->ythickness;
        
        GtkComboBox *cb = GTK_COMBO_BOX(widget);
        CalfCombobox *ccb = CALF_COMBOBOX(widget);
        GdkWindow *window = widget->window;
        cairo_t *c = gdk_cairo_create(GDK_DRAWABLE(window));
        
        GtkTreeModel *model = gtk_combo_box_get_model(cb);
        GtkTreeIter iter;
        gchar *lab;
        if (gtk_combo_box_get_active_iter (cb, &iter))
            gtk_tree_model_get (model, &iter, 0, &lab, -1);
        else
            lab = g_strdup("");
            
        int x  = widget->allocation.x;
        int y  = widget->allocation.y;
        int sx = widget->allocation.width;
        int sy = widget->allocation.height;
        gint mx, my;
        bool hover = false;
        
        create_rectangle(c, x, y, sx, sy, 0);
        cairo_clip(c);
        
        gtk_widget_get_pointer(widget, &mx, &my);
        if (mx >= 0 and mx < sx and my >= 0 and my < sy)
            hover = true;
            
        // background
        float radius, bevel, shadow, lights, lightshover, dull, dullhover;
        gtk_widget_style_get(widget, "border-radius", &radius, "bevel",  &bevel, "shadow", &shadow, "lights", &lights, "lightshover", &lightshover, "dull", &dull, "dullhover", &dullhover, NULL);
        display_background(widget, c, x, y, sx - padx * 2, sy - pady * 2, padx, pady, radius, bevel, g_ascii_isspace(lab[0]) ? 0 : 1, shadow, hover ? lightshover : lights, hover ? dullhover : dull);
        
        // text
        gtk_container_propagate_expose (GTK_CONTAINER (widget),
            GTK_BIN (widget)->child, event);

        // arrow
        if (ccb->arrow) {
            int pw = gdk_pixbuf_get_width(ccb->arrow);
            int ph = gdk_pixbuf_get_height(ccb->arrow);
            gdk_draw_pixbuf(GDK_DRAWABLE(window), widget->style->fg_gc[0],
                ccb->arrow, 0, 0,
                x + sx - padx - pw, y + (sy - ph) / 2, pw, ph,
                GDK_RGB_DITHER_NORMAL, 0, 0);
        }
        
        g_free(lab);
        cairo_destroy(c);
    }
    return FALSE;
}

void
calf_combobox_set_arrow (CalfCombobox *self, GdkPixbuf *arrow)
{
    self->arrow = arrow;
}

static void
calf_combobox_class_init (CalfComboboxClass *klass)
{
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);
    widget_class->expose_event = calf_combobox_expose;
    gtk_widget_class_install_style_property(
        widget_class, g_param_spec_float("border-radius", "Border Radius", "Generate round edges",
        0, 24, 4, GParamFlags(G_PARAM_READWRITE)));
    gtk_widget_class_install_style_property(
        widget_class, g_param_spec_float("bevel", "Bevel", "Bevel the object",
        -2, 2, 0.2, GParamFlags(G_PARAM_READWRITE)));
    gtk_widget_class_install_style_property(
        widget_class, g_param_spec_float("shadow", "Shadow", "Draw shadows inside",
        0, 16, 4, GParamFlags(G_PARAM_READWRITE)));
    gtk_widget_class_install_style_property(
        widget_class, g_param_spec_float("lights", "Lights", "Draw lights inside",
        0, 1, 0, GParamFlags(G_PARAM_READWRITE)));
    gtk_widget_class_install_style_property(
        widget_class, g_param_spec_float("lightshover", "Lights Hover", "Draw lights inside when hovered",
        0, 1, 0.5, GParamFlags(G_PARAM_READWRITE)));
    gtk_widget_class_install_style_property(
        widget_class, g_param_spec_float("dull", "Dull", "Draw dull inside",
        0, 1, 0.25, GParamFlags(G_PARAM_READWRITE)));
    gtk_widget_class_install_style_property(
        widget_class, g_param_spec_float("dullhover", "Dull Hover", "Draw dull inside when hovered",
        0, 1, 0.1, GParamFlags(G_PARAM_READWRITE)));
}

static void
calf_combobox_init (CalfCombobox *self)
{
    GtkWidget *widget = GTK_WIDGET(self);
    widget->requisition.width = 40;
    widget->requisition.height = 20;
}

GType
calf_combobox_get_type (void)
{
    static GType type = 0;
    if (!type) {
        static const GTypeInfo type_info = {
            sizeof(CalfComboboxClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            (GClassInitFunc)calf_combobox_class_init,
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof(CalfCombobox),
            0,    /* n_preallocs */
            (GInstanceInitFunc)calf_combobox_init
        };

        for (int i = 0; ; i++) {
            const char *name = "CalfCombobox";
            //char *name = g_strdup_printf("CalfCombobox%u%d", 
                //((unsigned int)(intptr_t)calf_combobox_class_init) >> 16, i);
            if (g_type_from_name(name)) {
                //free(name);
                continue;
            }
            type = g_type_register_static(GTK_TYPE_COMBO_BOX,
                                          name,
                                          &type_info,
                                          (GTypeFlags)0);
            //free(name);
            break;
        }
    }
    return type;
}
