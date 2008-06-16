import os
import sys
import fakeserv

class CommandExec:
    nextId = 1
    
    def __init__(self, type, *args):
        self.cmdId = CommandExec.nextId
        self.nextId += 1
        self.type = type
        self.args = args
        self.error = None
        self.okHandlers = []
        self.errorHandlers = []
    
    def onOK(self, handler):
        self.okHandlers.append(handler)

    def onError(self, handler):
        self.errorHandlers.append(handler)

    def run(self):
        fakeserv.queue(self)
        
    def calledOnOK(self, result):
        self.result = result
        #print "OK: %s(%s) -> %s" % (self.type, self.args, self.result)
        for h in self.okHandlers:
            h.receiveData(self)
        
    def calledOnError(self, message):
        self.error = message
        print "Error: %s(%s) -> %s" % (self.type, self.args, message)
        for h in self.errorHandlers:
            h.receiveError(self)

class LV2Plugin:
    def __init__(self, uri):
        self.uri = uri
        self.name = uri
        self.license = ""
        self.ports = []
        self.requiredFeatures = {}
        self.optionalFeatures = {}
        
    def decode(self, data):
        self.name = data['doap:name']

fakeserv.start()

plugins = {}

class URIListReceiver:
    def receiveData(self, data):
        #print "URI list received: " + repr(data.result)
        for uri in data.result:
            cmd = CommandExec('get_info', uri)
            cmd.run()
            res = cmd.result
            plugins[res["name"]] = res

cmd = CommandExec('get_uris', 'http://lv2plug.in/ns/lv2core#')
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
        print port
        for sp in ["default", "minimum", "maximum", "scalePoints"]:
            if port[sp] != None:
                extra.append("%s=%s" % (sp, port[sp]))
        print "%4s %-20s %-40s %s" % (port["index"], port["symbol"], port["name"], ", ".join(extra))
        #print port
    print 