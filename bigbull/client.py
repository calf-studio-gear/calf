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

