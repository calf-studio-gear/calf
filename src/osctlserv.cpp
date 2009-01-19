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
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <calf/osctl.h>
#include <calf/osctlserv.h>
#include <assert.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <sstream>

using namespace osctl;
using namespace std;

void osc_server::parse_message(const char *buffer, int len)
{
    osctl::string_buffer buf(string(buffer, len));
    osc_strstream str(buf);
    string address, type_tag;
    str >> address;
    str >> type_tag;
    // cout << "Address " << address << " type tag " << type_tag << endl << flush;
    if (!address.empty() && address[0] == '/'
      &&!type_tag.empty() && type_tag[0] == ',')
    {
        sink->receive_osc_message(address, type_tag.substr(1), str);
    }
}

void osc_socket::bind(const char *hostaddr, int port)
{
    socket = ::socket(PF_INET, SOCK_DGRAM, 0);
    if (socket < 0)
        throw osc_net_exception("socket");
    
    sockaddr_in sadr;
    sadr.sin_family = AF_INET;
    sadr.sin_port = htons(port);
    inet_aton(hostaddr, &sadr.sin_addr);
    if (::bind(socket, (sockaddr *)&sadr, sizeof(sadr)) < 0)
        throw osc_net_exception("bind");
    on_bind();
}

std::string osc_socket::get_uri() const
{
    sockaddr_in sadr;
    socklen_t len = sizeof(sadr);
    if (getsockname(socket, (sockaddr *)&sadr, &len) < 0)
        throw osc_net_exception("getsockname");
    
    char name[INET_ADDRSTRLEN], buf[32];
    
    inet_ntop(AF_INET, &sadr.sin_addr, name, INET_ADDRSTRLEN);
    sprintf(buf, "%d", ntohs(sadr.sin_port));
    
    return string("osc.udp://") + name + ":" + buf + prefix;
}

osc_socket::~osc_socket()
{
    close(socket);
}

void osc_server::on_bind()
{    
    ioch = g_io_channel_unix_new(socket);
    srcid = g_io_add_watch(ioch, G_IO_IN, on_data, this);
}

gboolean osc_server::on_data(GIOChannel *channel, GIOCondition cond, void *obj)
{
    osc_server *self = (osc_server *)obj;
    char buf[16384];
    int len = recv(self->socket, buf, 16384, 0);
    if (len > 0)
    {
        if (buf[0] == '/')
        {
            self->parse_message(buf, len);
        }
        if (buf[0] == '#')
        {
            // XXXKF bundles are not supported yet
        }
    }
    return TRUE;
}

osc_server::~osc_server()
{
    if (ioch)
        g_source_remove(srcid);
}
