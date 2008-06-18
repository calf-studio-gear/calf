import os
import sys
import fakeserv
import types

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
            if type(h) is types.FunctionType or type(h) is types.MethodType:
                h(self)
            else:
                h.receiveData(self)
        
    def calledOnError(self, message):
        self.error = message
        print "Error: %s(%s) -> %s" % (self.type, self.args, message)
        for h in self.errorHandlers:
            if type(h) is types.FunctionType or type(h) is types.MethodType:
                h(self)
            else:
                h.receiveError(self)

class LV2Port(object):
    def __init__(self, data):
        for prop in ["symbol", "name", "classes", "isAudio", "isControl", "isEvent", "isInput", "isOutput", "default", "maximum", "minimum", "scalePoints", "index"]:
            if prop in data:
                self.__dict__[prop] = data[prop]
            else:
                self.__dict__[prop] = None

class LV2Plugin(object):
    def __init__(self, data):
        self.uri = data["uri"]
        self.name = data["name"]
        self.license = data["license"]
        self.classes = set(data["classes"])
        self.requiredFeatures = set(data["requiredFeatures"])
        self.optionalFeatures = set(data["optionalFeatures"])
        self.ports = []
        for port in data["ports"]:
            self.ports.append(LV2Port(port))
        
    def decode(self, data):
        self.name = data['doap:name']

class BBClient(object):
    def __init__(self):
        self.plugins = []
        cmd = CommandExec('get_uris', 'http://lv2plug.in/ns/lv2core#')
        cmd.onOK(self.uris_received)
        cmd.run()
    def uris_received(self, data):
        self.uris = data.result
        for uri in self.uris:
            cmd = CommandExec('get_plugin_info', uri)
            cmd.onOK(self.uri_info_received)
            cmd.run()
    def uri_info_received(self, data):
        self.plugins.append(LV2Plugin(data.result))
