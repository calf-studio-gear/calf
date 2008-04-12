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
    vector<osc_data> data;
    osc_stream str(string(buffer, len));
    str.read("ss", data);
    if (!data[0].strval.empty() && data[0].strval[0] == '/'
      &&!data[1].strval.empty() && data[1].strval[0] == ',')
    {
        vector<osc_data> data2;
        str.read(data[1].strval.substr(1).c_str(), data2);
        sink->receive_osc_message(data[0].strval, data[1].strval, data2);
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

bool osc_client::send(const std::string &address, const std::vector<osc_data> &args)
{
    vector<osc_data> data;
    std::string type_tag = ",";
    osc_stream str;
    str.write(prefix + address);
    for (unsigned int i = 0; i < args.size(); i++)
        type_tag += char(args[i].type);
    str.write(type_tag);
    
    for (unsigned int i = 0; i < args.size(); i++)
        str.write(args[i]);    
    
    // printf("sending %s\n", str.buffer.c_str());

    return ::sendto(socket, str.buffer.data(), str.buffer.length(), 0, (sockaddr *)&addr, sizeof(addr)) == (int)str.buffer.length();
}

bool osc_client::send(const std::string &address)
{
    vector<osc_data> data;
    std::string type_tag = ",";
    osc_stream str;
    str.write(prefix + address);
    str.write(type_tag);
    return ::sendto(socket, str.buffer.data(), str.buffer.length(), 0, (sockaddr *)&addr, sizeof(addr)) == (int)str.buffer.length();
}

osc_message_dump osc_server::dump;

