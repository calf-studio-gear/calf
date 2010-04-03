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

#ifndef __CALF_OSCTLSERV_H
#define __CALF_OSCTLSERV_H

#include <glib.h>
#include "osctlnet.h"

namespace osctl
{
    

    
struct osc_server: public osc_socket
{
    osc_message_dump<osc_strstream, std::ostream> dump;
    osc_message_sink<osc_strstream> *sink;
    
    osc_server() : dump(std::cout), sink(&dump) {}
    
    void read_from_socket();
    void parse_message(const char *buffer, int len);    
    ~osc_server();
};

struct osc_glib_server: public osc_server
{
    GIOChannel *ioch;
    
    osc_glib_server() : ioch(NULL) {}
    
    virtual void on_bind();
    
    static gboolean on_data(GIOChannel *channel, GIOCondition cond, void *obj);
    ~osc_glib_server();
};

};

#endif
