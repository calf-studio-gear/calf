/* Calf DSP Library
 * A few useful widgets - a line graph, a knob, a tube - Panama!
 *
 * Copyright (C) 2008-2010 Krzysztof Foltman, Torben Hohn, Markus
 * Schmidt and others
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
 * Boston, MA 02111-1307, USA.
 */
 
#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#ifndef __CALF_CUSTOM_CTL
#define __CALF_CUSTOM_CTL

#include <cairo/cairo.h>
#include <gtk/gtk.h>
#include <gtk/gtkframe.h>
#include <gtk/gtkcombobox.h>
#include <gtk/gtknotebook.h>
#include <gtk/gtkrange.h>
#include <gtk/gtkscale.h>
#include <gtk/gtkbutton.h>
#include <calf/giface.h>
#include <calf/drawingutils.h>
#include <calf/ctl_vumeter.h>

G_BEGIN_DECLS

/// METER SCALE //////////////////////////////////////////////////////////////


#define CALF_TYPE_METER_SCALE          (calf_meter_scale_get_type())
#define CALF_METER_SCALE(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), CALF_TYPE_METER_SCALE, CalfMeterScale))
#define CALF_IS_METER_SCALE(obj)       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CALF_TYPE_METER_SCALE))
#define CALF_METER_SCALE_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass),  CALF_TYPE_METER_SCALE, CalfMeterScaleClass))
#define CALF_IS_METER_SCALE_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((klass),  CALF_TYPE_METER_SCALE))

struct CalfMeterScale
{
    GtkDrawingArea parent;
    std::vector<double> marker;
    CalfVUMeterMode mode;
    int position;
    int dots;
};

struct CalfMeterScaleClass
{
    GtkDrawingAreaClass parent_class;
};

extern GtkWidget *calf_meter_scale_new();
extern GType calf_meter_scale_get_type();


/// PHASE GRAPH ////////////////////////////////////////////////////////


#define CALF_TYPE_PHASE_GRAPH           (calf_phase_graph_get_type())
#define CALF_PHASE_GRAPH(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), CALF_TYPE_PHASE_GRAPH, CalfPhaseGraph))
#define CALF_IS_PHASE_GRAPH(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CALF_TYPE_PHASE_GRAPH))
#define CALF_PHASE_GRAPH_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST ((klass),  CALF_TYPE_PHASE_GRAPH, CalfPhaseGraphClass))
#define CALF_IS_PHASE_GRAPH_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE ((klass),  CALF_TYPE_PHASE_GRAPH))
#define CALF_PHASE_GRAPH_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj),  CALF_TYPE_PHASE_GRAPH, CalfPhaseGraphClass))

struct CalfPhaseGraph
{
    GtkDrawingArea parent;
    const calf_plugins::phase_graph_iface *source;
    int source_id;
    cairo_surface_t *background, *cache;
    inline float _atan(float x, float l, float r) {
        if(l >= 0 and r >= 0)
            return atan(x);
        else if(l >= 0 and r < 0)
            return M_PI + atan(x);
        else if(l < 0 and r < 0)
            return M_PI + atan(x);
        else if(l < 0 and r >= 0)
            return (2.f * M_PI) + atan(x);
        return 0.f;
    }
};

struct CalfPhaseGraphClass
{
    GtkDrawingAreaClass parent_class;
};

extern GtkWidget *calf_phase_graph_new();

extern GType calf_phase_graph_get_type();


/// TOGGLE /////////////////////////////////////////////////////////////


#define CALF_TYPE_TOGGLE          (calf_toggle_get_type())
#define CALF_TOGGLE(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), CALF_TYPE_TOGGLE, CalfToggle))
#define CALF_IS_TOGGLE(obj)       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CALF_TYPE_TOGGLE))
#define CALF_TOGGLE_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass),  CALF_TYPE_TOGGLE, CalfToggleClass))
#define CALF_IS_TOGGLE_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((klass),  CALF_TYPE_TOGGLE))

struct CalfToggle
{
    GtkRange parent;
    int size;
    int width;
    int height;
    const char *icon;
    GdkPixbuf *toggle_image;
};

struct CalfToggleClass
{
    GtkRangeClass parent_class;
};

extern GtkWidget *calf_toggle_new();
extern GtkWidget *calf_toggle_new_with_adjustment(GtkAdjustment *_adjustment);
extern void calf_toggle_set_size(CalfToggle *self, int size);
extern void calf_toggle_set_icon(CalfToggle *self, const gchar *icon);
extern GType calf_toggle_get_type();


/// FRAME //////////////////////////////////////////////////////////////


#define CALF_TYPE_FRAME          (calf_frame_get_type())
#define CALF_FRAME(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), CALF_TYPE_FRAME, CalfFrame))
#define CALF_IS_FRAME(obj)       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CALF_TYPE_FRAME))
#define CALF_FRAME_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass),  CALF_TYPE_FRAME, CalfFrameClass))
#define CALF_IS_FRAME_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((klass),  CALF_TYPE_FRAME))

struct CalfFrame
{
    GtkFrame parent;
};

struct CalfFrameClass
{
    GtkFrameClass parent_class;
};

extern GtkWidget *calf_frame_new(const char *label);
extern GType calf_frame_get_type();


/// COMBOBOX ///////////////////////////////////////////////////////////


#define CALF_TYPE_COMBOBOX          (calf_combobox_get_type())
#define CALF_COMBOBOX(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), CALF_TYPE_COMBOBOX, CalfCombobox))
#define CALF_IS_COMBOBOX(obj)       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CALF_TYPE_COMBOBOX))
#define CALF_COMBOBOX_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass),  CALF_TYPE_COMBOBOX, CalfComboboxClass))
#define CALF_IS_COMBOBOX_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((klass),  CALF_TYPE_COMBOBOX))

struct CalfCombobox
{
    GtkComboBox parent;
};

struct CalfComboboxClass
{
    GtkComboBoxClass parent_class;
};

extern GtkWidget *calf_combobox_new();
extern GType calf_combobox_get_type();


/// NOTEBOOK ///////////////////////////////////////////////////////////


#define CALF_TYPE_NOTEBOOK          (calf_notebook_get_type())
#define CALF_NOTEBOOK(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), CALF_TYPE_NOTEBOOK, CalfNotebook))
#define CALF_IS_NOTEBOOK(obj)       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CALF_TYPE_NOTEBOOK))
#define CALF_NOTEBOOK_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass),  CALF_TYPE_NOTEBOOK, CalfNotebookClass))
#define CALF_IS_NOTEBOOK_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((klass),  CALF_TYPE_NOTEBOOK))

struct CalfNotebook
{
    GtkNotebook parent;
};

struct CalfNotebookClass
{
    GtkNotebookClass parent_class;
    GdkPixbuf *screw;
};

extern GtkWidget *calf_notebook_new();
extern GType calf_notebook_get_type();


/// FADER //////////////////////////////////////////////////////////////


#define CALF_TYPE_FADER          (calf_fader_get_type())
#define CALF_FADER(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), CALF_TYPE_FADER, CalfFader))
#define CALF_IS_FADER(obj)       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CALF_TYPE_FADER))
#define CALF_FADER_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass),  CALF_TYPE_FADER, CalfFaderClass))
#define CALF_IS_FADER_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((klass),  CALF_TYPE_FADER))

struct CalfFaderLayout
{
    int x, y, w, h;
    int tx, ty, tw, th, tc;
    int scw, sch, scx1, scy1, scx2, scy2;
    int sx, sy, sw, sh;
    int slx, sly, slw, slh;
};

struct CalfFader
{
    GtkScale parent;
    int horizontal, size;
    GdkPixbuf *screw;
    GdkPixbuf *slider;
    GdkPixbuf *sliderpre;
    CalfFaderLayout layout;
    bool hover;
};

struct CalfFaderClass
{
    GtkScaleClass parent_class;
};



extern GtkWidget *calf_fader_new(const int horiz, const int size, const double min, const double max, const double step);
extern GType calf_fader_get_type();


/// BUTTON /////////////////////////////////////////////////////////////


#define CALF_TYPE_BUTTON          (calf_button_get_type())
#define CALF_BUTTON(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), CALF_TYPE_BUTTON, CalfButton))
#define CALF_IS_BUTTON(obj)       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CALF_TYPE_BUTTON))
#define CALF_BUTTON_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass),  CALF_TYPE_BUTTON, CalfButtonClass))
#define CALF_IS_BUTTON_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((klass),  CALF_TYPE_BUTTON))

struct CalfButton
{
    GtkButton parent;
};

struct CalfButtonClass
{
    GtkButtonClass parent_class;
};

extern GtkWidget *calf_button_new(const gchar *label);
extern GType calf_button_get_type();


/// TOGGLE BUTTON //////////////////////////////////////////////////////


#define CALF_TYPE_TOGGLE_BUTTON          (calf_toggle_button_get_type())
#define CALF_TOGGLE_BUTTON(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), CALF_TYPE_TOGGLE_BUTTON, CalfToggleButton))
#define CALF_IS_TOGGLE_BUTTON(obj)       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CALF_TYPE_TOGGLE_BUTTON))
#define CALF_TOGGLE_BUTTON_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass),  CALF_TYPE_TOGGLE_BUTTON, CalfToggleButtonClass))
#define CALF_IS_TOGGLE_BUTTON_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((klass),  CALF_TYPE_TOGGLE_BUTTON))

struct CalfToggleButton
{
    GtkToggleButton parent;
};

struct CalfToggleButtonClass
{
    GtkToggleButtonClass parent_class;
};

extern GtkWidget *calf_toggle_button_new(const gchar *label);
extern GType calf_toggle_button_get_type();


/// RADIO BUTTON //////////////////////////////////////////////////////


#define CALF_TYPE_RADIO_BUTTON          (calf_radio_button_get_type())
#define CALF_RADIO_BUTTON(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), CALF_TYPE_RADIO_BUTTON, CalfRadioButton))
#define CALF_IS_RADIO_BUTTON(obj)       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CALF_TYPE_RADIO_BUTTON))
#define CALF_RADIO_BUTTON_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass),  CALF_TYPE_RADIO_BUTTON, CalfRadioButtonClass))
#define CALF_IS_RADIO_BUTTON_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((klass),  CALF_TYPE_RADIO_BUTTON))

struct CalfRadioButton
{
    GtkRadioButton parent;
};

struct CalfRadioButtonClass
{
    GtkRadioButtonClass parent_class;
};

extern GtkWidget *calf_radio_button_new(const gchar *label);
extern GType calf_radio_button_get_type();


/// TAP BUTTON /////////////////////////////////////////////////////////


#define CALF_TYPE_TAP_BUTTON          (calf_tap_button_get_type())
#define CALF_TAP_BUTTON(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), CALF_TYPE_TAP_BUTTON, CalfTapButton))
#define CALF_IS_TAP_BUTTON(obj)       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CALF_TYPE_TAP_BUTTON))
#define CALF_TAP_BUTTON_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass),  CALF_TYPE_TAP_BUTTON, CalfTapButtonClass))
#define CALF_IS_TAP_BUTTON_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((klass),  CALF_TYPE_TAP_BUTTON))

struct CalfTapButton
{
    GtkButton parent;
    GdkPixbuf *image[3];
    int state;
};

struct CalfTapButtonClass
{
    GtkButtonClass parent_class;
};

extern GtkWidget *calf_tap_button_new();
extern GType calf_tap_button_get_type();



G_END_DECLS


/// TUNER ////////////////////////////////////////////////////////


#define CALF_TYPE_TUNER           (calf_tuner_get_type())
#define CALF_TUNER(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), CALF_TYPE_TUNER, CalfTuner))
#define CALF_IS_TUNER(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CALF_TYPE_TUNER))
#define CALF_TUNER_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST ((klass),  CALF_TYPE_TUNER, CalfTunerClass))
#define CALF_IS_TUNER_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE ((klass),  CALF_TYPE_TUNER))
#define CALF_TUNER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj),  CALF_TYPE_TUNER, CalfTunerClass))

struct CalfTuner
{
    GtkDrawingArea parent;
    int note;
    float cents;
    cairo_surface_t *background;
};

struct CalfTunerClass
{
    GtkDrawingAreaClass parent_class;
};

extern GtkWidget *calf_tuner_new();

extern GType calf_tuner_get_type();


#endif
