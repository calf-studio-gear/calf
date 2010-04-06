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
#include <stdio.h>
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

std::string osc_socket::get_url() const
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
    const char *orig_url = url;
    if (strncmp(url, "osc.udp://", 10))
        throw osc_net_bad_address(url);
    url += 10;
    
    const char *pos = strchr(url, ':');
    const char *pos2 = strchr(url, '/');
    if (!pos || !pos2)
        throw osc_net_bad_address(orig_url);
    
    // XXXKF perhaps there is a default port for osc.udp?
    if (pos2 - pos < 0)
        throw osc_net_bad_address(orig_url);
    
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

void osc_server::read_from_socket()
{
    do {
        char buf[16384];
        int len = recv(socket, buf, 16384, MSG_DONTWAIT);
        if (len > 0)
        {
            if (buf[0] == '/')
            {
                parse_message(buf, len);
            }
            if (buf[0] == '#')
            {
                // XXXKF bundles are not supported yet
            }
        }
        else
            break;
    } while(1);
}

osc_server::~osc_server()
{
}
