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
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __CALF_OSCTLNET_H
#define __CALF_OSCTLNET_H

#include <glib.h>

namespace osctl
{
    
struct osc_socket
{
    GIOChannel *ioch;
    int socket, srcid;
    std::string prefix;

    osc_socket() : ioch(NULL), socket(-1), srcid(0) {}
    void bind(const char *hostaddr = "0.0.0.0", int port = 0);
    std::string get_uri() const;
    virtual void on_bind() {}
    virtual ~osc_socket();
};

struct osc_server: public osc_socket
{
    static osc_message_dump dump;
    osc_message_sink *sink;
    
    osc_server() : sink(&dump) {}
    
    virtual void on_bind();
    
    static gboolean on_data(GIOChannel *channel, GIOCondition cond, void *obj);
    void parse_message(const char *buffer, int len);    
};

struct osc_client: public osc_socket
{
    sockaddr_in addr;
    
    void set_addr(const char *hostaddr, int port);
    void set_url(const char *url);
    bool send(const std::string &address, const std::vector<osc_data> &args);
    bool send(const std::string &address);
};

};

#endif
