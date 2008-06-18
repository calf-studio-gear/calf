import os
import sys
import fakeserv
import client

fakeserv.start()

plugins = {}

class URIListReceiver:
    def receiveData(self, data):
        #print "URI list received: " + repr(data.result)
        for uri in data.result:
            cmd = client.CommandExec('get_info', uri)
            cmd.run()
            res = cmd.result
            plugins[res["name"]] = res

cmd = client.CommandExec('get_uris', 'http://lv2plug.in/ns/lv2core#')
cmd.onOK(URIListReceiver())
cmd.run()

for p in plugins.keys():
    pl = plugins[p]
    print "Plugin: %s" % pl["name"]
    print "License: %s" % pl["license"]
    print "Classes: %s" % pl["classes"]
    print "Ports:"
    types = ["Audio", "Control", "Event", "Input", "Output"]
    for port in pl["ports"]:
        extra = []
        for type in types:
            if port["is" + type]:
                extra.append(type)
        for sp in ["default", "minimum", "maximum"]:
            if port[sp] != None:
                extra.append("%s=%s" % (sp, port[sp]))
        print "%4s %-20s %-40s %s" % (port["index"], port["symbol"], port["name"], ", ".join(extra))
        splist = port["scalePoints"]
        splist.sort(lambda x, y: cmp(x[1], y[1]))
        if len(splist):
            for sp in splist:
                print "       Scale point %s: %s" % (sp[1], sp[0])
        #print port
    print 
