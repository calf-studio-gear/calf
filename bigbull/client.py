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
        print "OK: %s(%s) -> %s" % (self.type, self.args, self.result)
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

class URIListReceiver:
    def receiveData(self, data):
        #print "URI list received: " + repr(data.result)
        for uri in data.result:
            cmd = CommandExec('get_info', uri)
            cmd.run()

cmd = CommandExec('get_uris', 'http://lv2plug.in/ns/lv2core#')
cmd.onOK(URIListReceiver())
cmd.run()
