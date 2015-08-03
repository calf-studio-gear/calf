/* Calf DSP Library
 * A combo box widget
 *
 * Copyright (C) 2008-2015 Krzysztof Foltman, Torben Hohn, Markus
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
#ifndef __CALF_CTL_COMBOBOX
#define __CALF_CTL_COMBOBOX

#include <cairo/cairo.h>
#include <gtk/gtk.h>
#include <gtk/gtkcombobox.h>
//#include <calf/giface.h>
#include <calf/drawingutils.h>
#include <calf/gui.h>

G_BEGIN_DECLS


/// COMBOBOX ///////////////////////////////////////////////////////////


#define CALF_TYPE_COMBOBOX          (calf_combobox_get_type())
#define CALF_COMBOBOX(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), CALF_TYPE_COMBOBOX, CalfCombobox))
#define CALF_IS_COMBOBOX(obj)       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CALF_TYPE_COMBOBOX))
#define CALF_COMBOBOX_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass),  CALF_TYPE_COMBOBOX, CalfComboboxClass))
#define CALF_IS_COMBOBOX_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((klass),  CALF_TYPE_COMBOBOX))

struct CalfCombobox
{
    GtkComboBox parent;
    GdkPixbuf *arrow;
};

struct CalfComboboxClass
{
    GtkComboBoxClass parent_class;
};

extern void calf_combobox_set_arrow(CalfCombobox *self, GdkPixbuf *arrow);
extern GtkWidget *calf_combobox_new();
extern GType calf_combobox_get_type();


G_END_DECLS

#endif
