/* Calf DSP Library
 * Open Sound Control UDP support
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
#include <calf/osctl.h>
#include <calf/osctlnet.h>
#include <assert.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <sstream>

using namespace osctl;
using namespace std;

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

//////////////////////////////////////////////////////////////

void osc_client::set_addr(const char *hostaddr, int port)
{
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_aton(hostaddr, &addr.sin_addr);
}

void osc_client::set_url(const char *url)
{
    if (strncmp(url, "osc.udp://", 10))
        throw osc_net_bad_address(url);
    url += 10;
    
    const char *pos = strchr(url, ':');
    const char *pos2 = strchr(url, '/');
    if (!pos || !pos2)
        throw osc_net_bad_address(url);
    
    // XXXKF perhaps there is a default port for osc.udp?
    if (pos2 - pos < 0)
        throw osc_net_bad_address(url);
    
    string hostname = string(url, pos - url);
    int port = atoi(pos + 1);
    prefix = string(pos2);
    printf("hostname %s port %d\n", hostname.c_str(), port);

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    hostent *he = gethostbyname(hostname.c_str());
    if (!he)
        throw osc_net_dns_exception("gethostbyname");
    
    addr.sin_addr = *(struct in_addr *)he->h_addr;
}

bool osc_client::send(const std::string &address, osctl::osc_typed_strstream &stream)
{
    std::string type_tag = "," + stream.type_buffer->data;
    osc_inline_strstream hdr;
    hdr << prefix + address << "," + stream.type_buffer->data;
    string str = hdr.data + stream.buffer.data;
    
    // printf("sending %s\n", str.buffer.c_str());

    return ::sendto(socket, str.data(), str.length(), 0, (sockaddr *)&addr, sizeof(addr)) == (int)str.length();
}

bool osc_client::send(const std::string &address)
{
    osc_inline_strstream hdr;
    hdr << prefix + address << ",";
    
    return ::sendto(socket, hdr.data.data(), hdr.data.length(), 0, (sockaddr *)&addr, sizeof(addr)) == (int)hdr.data.length();
}

