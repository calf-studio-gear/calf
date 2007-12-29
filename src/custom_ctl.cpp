#include <calf/custom_ctl.h>
#include <cairo/cairo.h>
#include <math.h>

static gboolean
calf_line_graph_expose (GtkWidget *widget, GdkEventExpose *event)
{
    g_assert(CALF_IS_LINE_GRAPH(widget));
    
    CalfLineGraph *lg = CALF_LINE_GRAPH(widget);
    int ox = widget->allocation.x + 1, oy = widget->allocation.y + 1;
    int sx = widget->allocation.width - 2, sy = widget->allocation.height - 2;
    
    cairo_t *c = gdk_cairo_create(GDK_DRAWABLE(widget->window));

    GdkColor sc = { 0, 0, 0, 0 };

    gdk_cairo_set_source_color(c, &sc);
    cairo_rectangle(c, ox, oy, sx, sy);
    cairo_fill(c);

    if (lg->source) {
        float *data = new float[2 * sx];
        GdkColor sc2 = { 0, 0, 65535, 0 };
        gdk_cairo_set_source_color(c, &sc2);
        cairo_set_line_join(c, CAIRO_LINE_JOIN_MITER);
        cairo_set_line_width(c, 1);
        for(int gn = 0; lg->source->get_graph(lg->source_id, gn, data, 2 * sx, c); gn++)
        {
            for (int i = 0; i < 2 * sx; i++)
            {
                int y = (int)(oy + sy / 2 - (sy / 2 - 1) * data[i]);
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
    
    // CalfLineGraph *lg = CALF_LINE_GRAPH(widget);
}

static void
calf_line_graph_size_allocate (GtkWidget *widget,
                           GtkAllocation *allocation)
{
    g_assert(CALF_IS_LINE_GRAPH(widget));
    
    widget->allocation = *allocation;
    // printf("allocation %d x %d\n", allocation->width, allocation->height);
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
    GtkWidget *widget = GTK_WIDGET(self);
    GTK_WIDGET_SET_FLAGS (widget, GTK_NO_WINDOW);
    widget->requisition.width = 40;
    widget->requisition.height = 40;
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

///////////////////////////////////////// knob ///////////////////////////////////////////////

static gboolean
calf_knob_expose (GtkWidget *widget, GdkEventExpose *event)
{
    g_assert(CALF_IS_KNOB(widget));
    
    CalfKnob *self = CALF_KNOB(widget);
    GdkWindow *window = widget->window;
    GtkAdjustment *adj = gtk_range_get_adjustment(GTK_RANGE(widget));

    // printf("adjustment = %p value = %f\n", adj, adj->value);
    int ox = widget->allocation.x, oy = widget->allocation.y;
    
    ox += (widget->allocation.width - 40) / 2;
    oy += (widget->allocation.height - 40) / 2;
    
    int phase = (adj->value - adj->lower) * 64 / (adj->upper - adj->lower);
    if (self->knob_type == 1 && phase == 32) {
        double pt = (adj->value - adj->lower) * 2.0 / (adj->upper - adj->lower) - 1.0;
        if (pt < 0)
            phase = 31;
        if (pt > 0)
            phase = 33;
    }
    gdk_draw_pixbuf(GDK_DRAWABLE(widget->window), widget->style->fg_gc[0], CALF_KNOB_CLASS(GTK_OBJECT_GET_CLASS(widget))->knob_image, phase * 40, self->knob_type * 40, ox, oy, 40, 40, GDK_RGB_DITHER_NORMAL, 0, 0);
    // printf("exposed %p %d+%d\n", widget->window, widget->allocation.x, widget->allocation.y);
    
    return TRUE;
}

static void
calf_knob_size_request (GtkWidget *widget,
                           GtkRequisition *requisition)
{
    g_assert(CALF_IS_KNOB(widget));
    
    CalfKnob *self = CALF_KNOB(widget);
}

static gboolean
calf_knob_button_press (GtkWidget *widget, GdkEventButton *event)
{
    g_assert(CALF_IS_KNOB(widget));
    CalfKnob *self = CALF_KNOB(widget);

    // CalfKnob *lg = CALF_KNOB(widget);
    gtk_widget_grab_focus(widget);
    gtk_grab_add(widget);
    self->start_x = event->x;
    self->start_y = event->y;
    self->start_value = gtk_range_get_value(GTK_RANGE(widget));
    
    return TRUE;
}

static gboolean
calf_knob_button_release (GtkWidget *widget, GdkEventButton *event)
{
    g_assert(CALF_IS_KNOB(widget));

    if (GTK_WIDGET_HAS_GRAB(widget))
        gtk_grab_remove(widget);
    return FALSE;
}

static gboolean
calf_knob_pointer_motion (GtkWidget *widget, GdkEventMotion *event)
{
    g_assert(CALF_IS_KNOB(widget));
    CalfKnob *self = CALF_KNOB(widget);

    if (GTK_WIDGET_HAS_GRAB(widget)) 
        gtk_range_set_value(GTK_RANGE(widget), self->start_value - (event->y - self->start_y) / 100);
    return FALSE;
}

static void
calf_knob_class_init (CalfKnobClass *klass)
{
    // GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);
    widget_class->expose_event = calf_knob_expose;
    widget_class->size_request = calf_knob_size_request;
    widget_class->button_press_event = calf_knob_button_press;
    widget_class->button_release_event = calf_knob_button_release;
    widget_class->motion_notify_event = calf_knob_pointer_motion;
    GError *error = NULL;
    klass->knob_image = gdk_pixbuf_new_from_file(PKGLIBDIR "/knob.png", &error);
    g_assert(klass->knob_image != NULL);
}

static void
calf_knob_init (CalfKnob *self)
{
    GtkWidget *widget = GTK_WIDGET(self);
    GTK_WIDGET_SET_FLAGS (GTK_WIDGET(self), GTK_CAN_FOCUS);
    widget->requisition.width = 40;
    widget->requisition.height = 40;
}

GtkWidget *
calf_knob_new()
{
    GtkAdjustment *adj = (GtkAdjustment *)gtk_adjustment_new(0, 0, 1, 0.01, 0.5, 0);
    return calf_knob_new_with_adjustment(adj);
}

static gboolean calf_knob_value_changed(gpointer obj)
{
    GtkWidget *widget = (GtkWidget *)obj;
    gtk_widget_queue_draw(widget);
    return FALSE;
}

GtkWidget *calf_knob_new_with_adjustment(GtkAdjustment *_adjustment)
{
    GtkWidget *widget = GTK_WIDGET( g_object_new (CALF_TYPE_KNOB, NULL ));
    if (widget) {
        gtk_range_set_adjustment(GTK_RANGE(widget), _adjustment);
        gtk_signal_connect(GTK_OBJECT(widget), "value-changed", G_CALLBACK(calf_knob_value_changed), widget);
    }
    return widget;
}

GType
calf_knob_get_type (void)
{
  static GType type = 0;
  if (!type) {
    static const GTypeInfo type_info = {
      sizeof(CalfKnobClass),
      NULL, /* base_init */
      NULL, /* base_finalize */
      (GClassInitFunc)calf_knob_class_init,
      NULL, /* class_finalize */
      NULL, /* class_data */
      sizeof(CalfKnob),
      0,    /* n_preallocs */
      (GInstanceInitFunc)calf_knob_init
    };
    
    type = g_type_register_static(GTK_TYPE_RANGE,
                                  "CalfKnob",
                                  &type_info,
                                  (GTypeFlags)0);
  }
  return type;
}

