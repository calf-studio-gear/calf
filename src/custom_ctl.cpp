#include <calf/custom_ctl.h>
#include <cairo/cairo.h>
#include <math.h>

static gboolean
calf_line_graph_expose (GtkWidget *widget, GdkEventExpose *event)
{
    g_assert(CALF_IS_LINE_GRAPH(widget));
    
    CalfLineGraph *lg = CALF_LINE_GRAPH(widget);
    GdkWindow *window = widget->window;
    int ox = widget->allocation.x + 1, oy = widget->allocation.y + 1;
    int sx = widget->allocation.width - 2, sy = widget->allocation.height - 2;
    
    cairo_t *c = gdk_cairo_create(GDK_DRAWABLE(widget->window));

    GdkColor sc = { 0, 0, 0, 0 }, sc2 = { 0, 0, 65535, 0 };

    gdk_cairo_set_source_color(c, &sc);
    cairo_rectangle(c, ox, oy, sx, sy);
    cairo_fill(c);

    if (lg->source) {
        float *data = new float[2 * sx];
        if (lg->source->get_graph(lg->source_id, data, 2 * sx))
        {
            gdk_cairo_set_source_color(c, &sc2);
            cairo_set_line_join(c, CAIRO_LINE_JOIN_MITER);
            cairo_set_line_width(c, 1);
            for (int i = 0; i < 2 * sx; i++)
            {
                int y = oy + sy / 2 - (sy / 2 - 1) * data[i];
                if (y < oy) y = oy;
                if (y >= oy + sy) y = oy + sy - 1;
                if (i)
                    cairo_line_to(c, ox + i * 0.5, y);
                else
                    cairo_move_to(c, ox, y);
            }
            cairo_stroke(c);
        }
        delete []data;
    }
    
    cairo_destroy(c);
    
    gtk_paint_shadow(widget->style, widget->window, GTK_STATE_NORMAL, GTK_SHADOW_IN, NULL, widget, NULL, ox - 1, oy - 1, sx + 2, sy + 2);
    // printf("exposed %p %d+%d\n", widget->window, widget->allocation.x, widget->allocation.y);
    
    return TRUE;
}

static void
calf_line_graph_size_request (GtkWidget *widget,
                           GtkRequisition *requisition)
{
    g_assert(CALF_IS_LINE_GRAPH(widget));
    
    CalfLineGraph *lg = CALF_LINE_GRAPH(widget);

    requisition->width = 64;
    requisition->height = 64;
}

static void
calf_line_graph_size_allocate (GtkWidget *widget,
                           GtkAllocation *allocation)
{
    g_assert(CALF_IS_LINE_GRAPH(widget));
    
    widget->allocation = *allocation;
    printf("allocation %d x %d\n", allocation->width, allocation->height);
}

static void
calf_line_graph_class_init (CalfLineGraphClass *klass)
{
    // GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);
    widget_class->expose_event = calf_line_graph_expose;
    widget_class->size_request = calf_line_graph_size_request;
    widget_class->size_allocate = calf_line_graph_size_allocate;
}

static void
calf_line_graph_init (CalfLineGraph *self)
{
    GTK_WIDGET_SET_FLAGS (GTK_WIDGET(self), GTK_NO_WINDOW);
}

GtkWidget *
calf_line_graph_new()
{
    return GTK_WIDGET( g_object_new (CALF_TYPE_LINE_GRAPH, NULL ));
}

GType
calf_line_graph_get_type (void)
{
  static GType type = 0;
  if (!type) {
    static const GTypeInfo type_info = {
      sizeof(CalfLineGraphClass),
      NULL, /* base_init */
      NULL, /* base_finalize */
      (GClassInitFunc)calf_line_graph_class_init,
      NULL, /* class_finalize */
      NULL, /* class_data */
      sizeof(CalfLineGraph),
      0,    /* n_preallocs */
      (GInstanceInitFunc)calf_line_graph_init
    };
    
    type = g_type_register_static(GTK_TYPE_WIDGET,
                                  "CalfLineGraph",
                                  &type_info,
                                  (GTypeFlags)0);
  }
  return type;
}

