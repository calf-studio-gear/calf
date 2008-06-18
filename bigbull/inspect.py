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
            cmd = client.CommandExec('get_plugin_info', uri)
            cmd.run()
            res = cmd.result
            plugins[res["name"]] = res

cmd = client.CommandExec('get_uris', 'http://lv2plug.in/ns/lv2core#')
cmd.onOK(URIListReceiver())
cmd.run()

for p in plugins.keys():
    pl = plugins[p]
    plugin = client.LV2Plugin(pl)
    print "Plugin: %s" % plugin.name
    print "License: %s" % plugin.license
    print "Classes: %s" % plugin.classes
    print "Required features: %s" % list(plugin.requiredFeatures)
    print "Optional features: %s" % list(plugin.optionalFeatures)
    print "Ports:"
    types = ["Audio", "Control", "Event", "Input", "Output"]
    for port in plugin.ports:
        extra = []
        for type in types:
            if port.__dict__["is" + type]:
                extra.append(type)
        for sp in ["default", "minimum", "maximum"]:
            if port.__dict__[sp] != None:
                extra.append("%s=%s" % (sp, port.__dict__[sp]))
        print "%4s %-20s %-40s %s" % (port.index, port.symbol, port.name, ", ".join(extra))
        splist = port.scalePoints
        splist.sort(lambda x, y: cmp(x[1], y[1]))
        if len(splist):
            for sp in splist:
                print "       Scale point %s: %s" % (sp[1], sp[0])
        #print port
    print 
