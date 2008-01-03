/* Calf DSP Library
 * Benchmark for selected parts of the library.
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
#include <getopt.h>
#include <stdint.h>
#include <stdlib.h>
#include <config.h>
#include <calf/giface.h>
#include <calf/modules.h>
#include <calf/modules_dev.h>
#include <calf/benchmark.h>

using namespace std;
using namespace dsp;

#include <calf/osctl.h>
using namespace osctl;

#if 0
void osctl_test()
{
    string sdata = string("\000\000\000\003123\000test\000\000\000\000\000\000\000\001\000\000\000\002", 24);
    osc_stream is(sdata);
    vector<osc_data> data;
    is.read("bsii", data);
    assert(is.pos == sdata.length());
    assert(data.size() == 4);
    assert(data[0].type == osc_blob);
    assert(data[1].type == osc_string);
    assert(data[2].type == osc_i32);
    assert(data[3].type == osc_i32);
    assert(data[0].strval == "123");
    assert(data[1].strval == "test");
    assert(data[2].i32val == 1);
    assert(data[3].i32val == 2);
    osc_stream os("");
    os.write(data);
    assert(os.buffer == sdata);
    osc_server srv;
    srv.bind("0.0.0.0", 4541);
    
    osc_client cli;
    cli.bind("0.0.0.0", 0);
    cli.set_addr("0.0.0.0", 4541);
    if (!cli.send("/blah", data))
        g_error("Could not send the OSC message");
    
    g_main_loop_run(g_main_loop_new(NULL, FALSE));
}
#endif

void help(char *argv[])
{
    printf("GTK+ user interface for Calf DSSI plugins\nSyntax: %s [--help] [--version] <osc-url> <so-file> <plugin-label> <instance-name>\n", argv[0]);
}

static struct option long_options[] = {
    {"help", 0, 0, 'h'},
    {"version", 0, 0, 'v'},
    {0,0,0,0},
};

GMainLoop *mainloop;

struct dssi_osc_server: public osc_server, public osc_message_sink
{
    dssi_osc_server()
    {
        sink = this;
    }
    
    void receive_osc_message(std::string address, std::string type_tag, const std::vector<osc_data> &args)
    {
        dump.receive_osc_message(address, type_tag, args);
        if (address == prefix + "/update" && args.size() && args[0].type == osc_string)
        {
            printf("UPDATE: %s\n", args[0].strval.c_str());
            return;
        }
        if (address == prefix + "/quit")
        {
            printf("QUIT\n");
            g_main_loop_quit(mainloop);
            return;
        }
    }
};

int main(int argc, char *argv[])
{
    while(1) {
        int option_index;
        int c = getopt_long(argc, argv, "hv", long_options, &option_index);
        if (c == -1)
            break;
        switch(c) {
            case 'h':
                help(argv);
                return 0;
            case 'v':
                printf("%s\n", PACKAGE_STRING);
                return 0;
        }
    }
    if (argc - optind != 4)
    {
        help(argv);
        exit(0);
    }
    dssi_osc_server srv;
    srv.prefix = "/dssi/"+string(argv[optind + 1]) + "/" + string(argv[optind + 2]);
    srv.bind();
    
    mainloop = g_main_loop_new(NULL, FALSE);

    osc_client cli;
    cli.bind();
    cli.set_url(argv[optind]);
    
    printf("URI = %s\n", srv.get_uri().c_str());
    
    vector<osc_data> data;
    data.push_back(osc_data(srv.get_uri(), osc_string));
    if (!cli.send("/update", data))
    {
        g_error("Could not send the initial update message via OSC to %s", argv[optind]);
        return 1;
    }
    
    g_main_loop_run(mainloop);
    
    return 0;
}
