/* Calf DSP Library
 * Open Sound Control UDP server support
 *
 * Copyright (C) 2007-2009 Krzysztof Foltman
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

#include <calf/osctl_glib.h>

using namespace osctl;
using namespace std;

void osc_glib_server::on_bind()
{    
    ioch = g_io_channel_unix_new(socket);
    srcid = g_io_add_watch(ioch, G_IO_IN, on_data, this);
}

gboolean osc_glib_server::on_data(GIOChannel *channel, GIOCondition cond, void *obj)
{
    osc_server *self = (osc_server *)obj;
    self->read_from_socket();
    return TRUE;
}

osc_glib_server::~osc_glib_server()
{
    if (ioch)
        g_source_remove(srcid);
}
