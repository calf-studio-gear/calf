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

#include <calf/gtk_session_env.h>
#include <calf/gtk_main_win.h>

using namespace calf_plugins;

void gtk_session_environment::init_gui(int &argc, char **&argv)
{
    gtk_rc_add_default_file(PKGLIBDIR "calf.rc");
    gtk_init(&argc, &argv);
}

main_window_iface *gtk_session_environment::create_main_window()
{
    return new gtk_main_window;
}

void gtk_session_environment::start_gui_loop()
{
    gtk_main();
}

void gtk_session_environment::quit_gui_loop()
{
    gtk_main_quit();
}

gtk_session_environment::~gtk_session_environment()
{
}
