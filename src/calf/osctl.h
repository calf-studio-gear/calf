/* Calf DSP Library
 * Open Sound Control primitives
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

#include <string>
#include <vector>
#include <string.h>
#include <glib.h>
#include <errno.h>
#include <netinet/in.h>
#include <netdb.h>

namespace osctl
{
    
enum osc_type
{
    osc_i32 = 'i',
    osc_f32 = 'f',
    osc_string = 's',
    osc_blob = 'b',
    
    // unsupported
    osc_i64 = 'h',
    osc_ts = 't',
    osc_f64 = 'd',
    osc_string_alt = 'S',
    osc_char = 'c',
    osc_rgba = 'r',
    osc_midi = 'm',
    osc_true = 'T',
    osc_false = 'F',
    osc_nil = 'N',
    osc_inf = 'I',
    osc_start_array = '[',
    osc_end_array = ']'
};

extern const char *osc_type_name(osc_type type);

// perhaps a little inefficient, but this is OSC anyway, so efficiency
// goes out of the window right from start :)
struct osc_data
{
    osc_type type;
    union {
        int32_t i32val;
        uint64_t tsval;
        float f32val;
        double f64val;
    };
    std::string strval;
    osc_data()
    {
        type = osc_nil;
    }
    osc_data(std::string _sval, osc_type _type = osc_string)
    : type(_type)
    , strval(_sval)
    {
    }
    osc_data(int ival)
    : type(osc_i32)
    , i32val(ival)
    {
    }
    osc_data(float fval)
    : type(osc_f32)
    , f32val(fval)
    {
    }
    std::string to_string() const;
    ~osc_data()
    {
    }
};

struct osc_exception: public std::exception
{
    virtual const char *what() const throw() { return "OSC parsing error"; }
};
    
struct osc_stream
{
    std::string buffer;
    unsigned int pos;
    
    osc_stream() : pos(0) {}
    osc_stream(const std::string &_buffer, unsigned int _pos = 0)
    : buffer(_buffer), pos(_pos) {}
    
    inline void copy_from(void *dest, int bytes)
    {
        if (pos + bytes > buffer.length())
            throw osc_exception();
        memcpy(dest, &buffer[pos], bytes);
        pos += bytes;
    }
    
    void read(osc_type type, osc_data &od);
    void write(const osc_data &od);
    
    void read(const char *tags, std::vector<osc_data> &data);
    void write(const std::vector<osc_data> &data);
};

struct osc_net_bad_address: public std::exception
{
    std::string addr, error_msg;
    osc_net_bad_address(const char *_addr)
    {
        addr = _addr;
        error_msg = "Incorrect OSC URI: " + addr;
    }
    virtual const char *what() const throw() { return error_msg.c_str(); }
    virtual ~osc_net_bad_address() throw () {}
};

struct osc_net_exception: public std::exception
{
    int net_errno;
    std::string command, error_msg;
    osc_net_exception(const char *cmd, int _errno = errno)
    {
        command = cmd;
        net_errno = _errno;
        error_msg = "OSC error in "+command+": "+strerror(_errno);
    }
    virtual const char *what() const throw() { return error_msg.c_str(); }
    virtual ~osc_net_exception() throw () {}
};
    
struct osc_net_dns_exception: public std::exception
{
    int net_errno;
    std::string command, error_msg;
    osc_net_dns_exception(const char *cmd, int _errno = h_errno)
    {
        command = cmd;
        net_errno = _errno;
        error_msg = "OSC error in "+command+": "+hstrerror(_errno);
    }
    virtual const char *what() const throw() { return error_msg.c_str(); }
    virtual ~osc_net_dns_exception() throw () {}
};
    
struct osc_message_sink
{
    virtual void receive_osc_message(std::string address, std::string type_tag, const std::vector<osc_data> &args)=0;
    virtual ~osc_message_sink() {}
};

struct osc_message_dump: public osc_message_sink
{
    virtual void receive_osc_message(std::string address, std::string type_tag, const std::vector<osc_data> &args);
};

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
