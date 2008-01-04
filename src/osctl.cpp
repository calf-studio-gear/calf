#include <calf/osctl.h>
#include <assert.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sstream>

using namespace osctl;
using namespace std;

std::string osc_data::to_string() const
{
    std::stringstream ss;
    switch(type)
    {
    case osc_i32: ss << i32val; break;
    case osc_i64: ss << (int64_t)tsval; break;
    case osc_f32: ss << f32val; break;
    case osc_ts: ss << tsval; break;
    case osc_string: return strval;
    case osc_string_alt: return strval;
    case osc_blob: return strval;
    case osc_true: return "TRUE";
    case osc_false: return "FALSE";
    case osc_nil: return "NIL";
    case osc_inf: return "INF";
    case osc_start_array: return "[";
    case osc_end_array: return "]";
    default:
        return "?";
    }
    return ss.str();
}

const char *osctl::osc_type_name(osc_type type)
{
    switch(type)
    {
    case osc_i32: return "i32";
    case osc_i64: return "i64";
    case osc_f32: return "f32";
    case osc_ts: return "ts";
    case osc_string: return "str";
    case osc_string_alt: return "stralt";
    case osc_blob: return "blob";
    case osc_char: return "char";
    case osc_rgba: return "rgba";
    case osc_midi: return "midi";
    case osc_true: return "TRUE";
    case osc_false: return "FALSE";
    case osc_nil: return "NIL";
    case osc_inf: return "INF";
    case osc_start_array: return "[";
    case osc_end_array: return "]";
    default:
        return "unknown";
    }
}

void osc_stream::read(osc_type type, osc_data &od)
{
    od.type = type;
    switch(type)
    {
    case osc_i32:
    case osc_f32:
        copy_from(&od.i32val, 4);
        od.i32val = ntohl(od.i32val);
        break;
    case osc_ts:
        copy_from(&od.tsval, 8);
        if (ntohl(1) != 1) {
            uint32_t a = ntohl(od.tsval), b = ntohl(od.tsval >> 32);
            od.tsval = (((uint64_t)a) << 32) | b;
        }
        break;
    case osc_string:
    case osc_string_alt:
    {
        int len = 0, maxlen = 0;
        maxlen = buffer.length() - pos;
        while(len < maxlen && buffer[pos + len])
            len++;
        od.strval = buffer.substr(pos, len);
        pos += (len + 4) &~ 3;
        break;
    }
    case osc_blob: {
        copy_from(&od.i32val, 4);
        od.i32val = ntohl(od.i32val);
        od.strval = buffer.substr(pos, od.i32val);
        pos += (od.i32val + 3) &~ 3;
        break;
    }
    case osc_true:
    case osc_false:
    case osc_nil:
        return;
    default:
        assert(0);
    }
}

void osc_stream::write(const osc_data &od)
{
    uint32_t val;
    switch(od.type)
    {
    case osc_i32:
    case osc_f32:
    {
        val = ntohl(od.i32val);
        buffer += string((const char *)&val, 4);
        break;
    }
    case osc_ts: {
        uint64_t ts = od.tsval;
        if (ntohl(1) != 1) {
            uint32_t a = ntohl(od.tsval), b = ntohl(od.tsval >> 32);
            ts = (((uint64_t)a) << 32) | b;
        }
        buffer += string((const char *)&ts, 8);        
        break;
    }
    case osc_string:
    case osc_string_alt:
    {
        buffer += od.strval;
        val = 0;
        buffer += string((const char *)&val, 4 - (od.strval.length() & 3));
        break;
    }
    case osc_blob:
    {
        val = ntohl(od.strval.length());
        buffer += string((const char *)&val, 4);
        buffer += od.strval;
        val = 0;
        buffer += string((const char *)&val, 4 - (od.strval.length() & 3));
        break;
    }
    case osc_true:
    case osc_false:
    case osc_nil:
        return;
    default:
        assert(0);
    }
}
    
void osc_stream::read(const char *tags, vector<osc_data> &data)
{
    while(*tags)
    {
        data.push_back(osc_data());
        read((osc_type)*tags++, *data.rbegin());
    }
}

void osc_stream::write(const vector<osc_data> &data)
{
    unsigned int pos = 0;
    
    while(pos < data.size())
        write(data[pos++]);
}

osc_message_dump osc_server::dump;

void osc_message_dump::receive_osc_message(std::string address, std::string type_tag, const std::vector<osc_data> &args)
{
    printf("address: %s, type tag: %s\n", address.c_str(), type_tag.c_str());
    for (unsigned int i = 0; i < args.size(); i++)
    {
        printf("argument %d is %s\n", i, args[i].to_string().c_str());
    }
}

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
