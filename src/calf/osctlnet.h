/* Calf DSP Library
 * Open Sound Control UDP support
 *
 * Copyright (C) 2007 Krzysztof Foltman
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

#ifndef __CALF_OSCTLNET_H
#define __CALF_OSCTLNET_H

#include <arpa/inet.h>
#include <sys/socket.h>
#include <calf/osctl.h>

namespace osctl
{
    
struct osc_socket
{
    int socket, srcid;
    std::string prefix;

    osc_socket() : socket(-1), srcid(0) {}
    void bind(const char *hostaddr = "0.0.0.0", int port = 0);
    std::string get_url() const;
    virtual void on_bind() {}
    virtual ~osc_socket();
};

struct osc_client: public osc_socket
{
    sockaddr_in addr;
    
    void set_addr(const char *hostaddr, int port);
    void set_url(const char *url);
    bool send(const std::string &address, osctl::osc_typed_strstream &stream);
    bool send(const std::string &address);
};

/// Base implementation for OSC server - requires the interfacing code
/// to poll periodically for messages. Alternatively, one can use
/// osc_glib_server that hooks into glib main loop.
struct osc_server: public osc_socket
{
    osc_message_dump<osc_strstream, std::ostream> dump;
    osc_message_sink<osc_strstream> *sink;
    
    osc_server() : dump(std::cout), sink(&dump) {}
    
    void read_from_socket();
    void parse_message(const char *buffer, int len);    
    ~osc_server();
};

};

#endif
