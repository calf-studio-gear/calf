#include <calf/osctl.h>
#include <assert.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <sstream>

using namespace osctl;
using namespace std;

#if 0
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
#endif

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

#if 0
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

void osc_message_dump::receive_osc_message(std::string address, std::string type_tag, const std::vector<osc_data> &args)
{
    printf("address: %s, type tag: %s\n", address.c_str(), type_tag.c_str());
    for (unsigned int i = 0; i < args.size(); i++)
    {
        printf("argument %d is %s\n", i, args[i].to_string().c_str());
    }
}

#endif
