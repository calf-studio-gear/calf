/* Calf DSP Library
 * GUI functions for preset management.
 * Copyright (C) 2007 Krzysztof Foltman
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */
#ifndef __CALF_PRESET_GUI_H
#define __CALF_PRESET_GUI_H


#include <calf/giface.h>
#include <calf/gui.h>

namespace calf_plugins {
    
struct gui_preset_access: public preset_access_iface
{
    plugin_gui *gui;
    GtkWidget *store_preset_dlg;
    
    gui_preset_access(plugin_gui *_gui);
    virtual void store_preset();
    virtual void activate_preset(int preset, bool builtin);
    virtual ~gui_preset_access() {} 
        
    static void on_dlg_destroy_window(GtkWindow *window, gpointer data);
};

};

#endif
