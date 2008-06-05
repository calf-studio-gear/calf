#include <calf/osctl.h>
#include <calf/osctlnet.h>
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
    if (ioch)
        g_source_remove(srcid);
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

