#ifndef __CALF_CUSTOM_CTL
#define __CALF_CUSTOM_CTL

#include <gtk/gtk.h>
#include <calf/giface.h>

G_BEGIN_DECLS

#define CALF_TYPE_LINE_GRAPH          (calf_line_graph_get_type())
#define CALF_LINE_GRAPH(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), CALF_TYPE_LINE_GRAPH, CalfLineGraph))
#define CALF_IS_LINE_GRAPH(obj)       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CALF_TYPE_LINE_GRAPH))
#define CALF_LINE_GRAPH_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass),  CALF_TYPE_LINE_GRAPH, CalfLineGraphClass))
#define CALF_IS_LINE_GRAPH_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((klass),  CALF_TYPE_LINE_GRAPH))

struct CalfLineGraph
{
    GtkWidget parent;
    synth::line_graph_iface *source;
    int source_id;
};

struct CalfLineGraphClass
{
    GtkWidgetClass parent_class;
};

extern GtkWidget *calf_line_graph_new();

extern GType calf_line_graph_get_type();

#define CALF_TYPE_VUMETER          (calf_vumeter_get_type())
#define CALF_VUMETER(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), CALF_TYPE_VUMETER, CalfVUMeter))
#define CALF_IS_VUMETER(obj)       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CALF_TYPE_VUMETER))
#define CALF_VUMETER_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass),  CALF_TYPE_VUMETER, CalfVUMeterClass))
#define CALF_IS_VUMETER_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((klass),  CALF_TYPE_VUMETER))

struct CalfVUMeter
{
    GtkWidget parent;
    float value;
};

struct CalfVUMeterClass
{
    GtkWidgetClass parent_class;
};

extern GtkWidget *calf_vumeter_new();
extern GType calf_vumeter_get_type();
extern void calf_vumeter_set_value(CalfVUMeter *meter, float value);
extern float calf_vumeter_get_value(CalfVUMeter *meter);

#define CALF_TYPE_KNOB          (calf_knob_get_type())
#define CALF_KNOB(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), CALF_TYPE_KNOB, CalfKnob))
#define CALF_IS_KNOB(obj)       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CALF_TYPE_KNOB))
#define CALF_KNOB_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass),  CALF_TYPE_KNOB, CalfKnobClass))
#define CALF_IS_KNOB_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((klass),  CALF_TYPE_KNOB))

struct CalfKnob
{
    GtkRange parent;
    int knob_type;
    double start_x, start_y, start_value;
};

struct CalfKnobClass
{
    GtkRangeClass parent_class;
    GdkPixbuf *knob_image;
};

extern GtkWidget *calf_knob_new();
extern GtkWidget *calf_knob_new_with_adjustment(GtkAdjustment *_adjustment);

extern GType calf_knob_get_type();

G_END_DECLS

#endif
