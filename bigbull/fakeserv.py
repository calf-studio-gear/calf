import re
import os
import sys
import glob

lv2 = "http://lv2plug.in/ns/lv2core#"
lv2evt = "http://lv2plug.in/ns/ext/event#"
rdf = "http://www.w3.org/1999/02/22-rdf-syntax-ns#"
rdfs = "http://www.w3.org/2000/01/rdf-schema#"
rdf_type = rdf + "type"

class DumpRDFModel:
    def addTriple(self, s, p, o):
        print "%s [%s] %s" % (s, p, repr(o))

class SimpleRDFModel:
    def __init__(self):
        self.bySubject = {}
        self.byPredicate = {}
    def getByType(self, classname):
        classes = self.bySubject["$classes"]
        if classname in classes:
            return classes[classname]
        return []
    def getByPropType(self, propname):
        if propname in self.byPredicate:
            return self.byPredicate[propname]
        return []
    def getProperty(self, subject, props, optional = False, single = False):
        if type(props) is list:
            prop = props[0]
        else:
            prop = props
        if type(subject) is str:
            subject = self.bySubject[subject]
        elif type(subject) is dict:
            pass
        else:
            if single:
                return None
            else:
                return []
        anyprops = set()
        if prop in subject:
            for o in subject[prop]:
                anyprops.add(o)
        if type(props) is list:
            if len(props) > 1:
                result = set()
                for v in anyprops:
                    if single:
                        value = self.getProperty(v, props[1:], optional = optional, single = True)
                        if value != None:
                            return value
                    else:
                        result |= set(self.getProperty(v, props[1:], optional = optional, single = False))
                if single:
                    return None
                else:
                    return list(result)
        if single:
            if len(anyprops) > 0:
                if len(anyprops) > 1:
                    raise Exception, "More than one value of " + prop
                return list(anyprops)[0]
            else:
                return None
        return list(anyprops)
        
                
    def addTriple(self, s, p, o):
        if p == rdf_type:
            p = "a"
        if s not in self.bySubject:
            self.bySubject[s] = {}
        if p not in self.bySubject[s]:
            self.bySubject[s][p] = []
        self.bySubject[s][p].append(o)
        if p not in self.byPredicate:
            self.byPredicate[p] = {}
        if s not in self.byPredicate[p]:
            self.byPredicate[p][s] = []
        self.byPredicate[p][s].append(o)
        if p == "a":
            self.addTriple("$classes", o, s)
    def copyFrom(self, src):
        for s in src.bySubject:
            po = src.bySubject[s]
            for p in po:
                for o in po[p]:
                    self.addTriple(s, p, o)
    def dump(self):
        for s in self.bySubject.keys():
            for p in self.bySubject[s].keys():
                print "%s %s %s" % (s, p, self.bySubject[s][p])

class FakeServer(object):
    def __init__(self):
        pass

def start():
    global instance
    instance = FakeServer()

def queue(cmdObject):
    global instance
    #try:
    cmdObject.calledOnOK(type(instance).__dict__[cmdObject.type](instance, *cmdObject.args))
    #except:
    #    cmdObject.calledOnError(repr(sys.exc_info()))
    
