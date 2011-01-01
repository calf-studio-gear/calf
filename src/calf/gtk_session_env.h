/* Calf DSP Library Utility Application - calfjackhost
 * GTK+ implementation of session_environment_iface.
 *
 * Copyright (C) 2007-2011 Krzysztof Foltman
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#include <calf/gui.h>

namespace calf_plugins
{

class gtk_session_environment: public session_environment_iface
{
public:
    virtual void init_gui(int &argc, char **&argv);
    virtual void start_gui_loop();
    virtual void quit_gui_loop();
    virtual main_window_iface *create_main_window();
    ~gtk_session_environment();
};
    
};
