import re
import os
import sys
import glob
import yappy.parser

class DumpRDFModel:
    def addTriple(self, s, p, o):
        print "%s [%s] %s" % (s, p, repr(o))

class SimpleRDFModel:
    def __init__(self):
        self.bySubject = {}
        self.byPredicate = {}
    def addTriple(self, s, p, o):
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
    def dump(self):
        for s in self.bySubject.keys():
            for p in self.bySubject[s].keys():
                print "%s %s %s" % (s, p, self.bySubject[s][p])

def parseTTL(uri, content, model):
    prefixes = {}
    lexer = yappy.parser.Lexer([
        (r"(?m)^\s*#[^\n]*", ""),
        ('"""(\n|\r|.)*?"""', lambda x : ("string", x[3:-3])),
        (r'"([^"\\]|\\.)+"', lambda x : ("string", x[1:-1])),
        (r"<>", lambda x : ("URI", uri)),
        (r"<[^>]*>", lambda x : ("URI", x[1:-1])),
        ("[-a-zA-Z0-9_]*:[-a-zA-Z0-9_]*", lambda x : ("prnot", x)),
        ("@prefix", lambda x : ("prefix", x)),
        (r"-?[0-9]+\.[0-9]+", lambda x : ("number", float(x))),
        (r"-?[0-9]+", lambda x : ("number", int(x))),
        ("[a-zA-Z0-9_]+", lambda x : ("symbol", x)),
        (r"[()\[\];.,]", lambda x : (x, x)),
        ("\s+", ""),
    ])
    spo_stack = []
    spo = ["", "", ""]
    item = 0
    anoncnt = 1
    for x in lexer.scan(content):
        if x[0] == '':
            continue
        if x[0] == 'prefix':
            spo[0] = "@prefix"
            item = 1
            continue
        elif (x[0] == '.' and spo_stack == []) or x[0] == ';' or x[0] == ',':
            if item == 3:
                if spo[0] == "@prefix":
                    prefixes[spo[1][:-1]] = spo[2]
                else:
                    model.addTriple(spo[0], spo[1], spo[2])
                    if spo[1] == "a":
                        model.addTriple("$classes", spo[2], spo[0])
                if x[0] == '.': item = 0
                elif x[0] == ';': item = 1
                elif x[0] == ',': item = 2
            else:
                print uri+": Unexpected " + x[0]
        elif x[0] == "prnot" and item < 3:
            prnot = x[1].split(":")
            if item != 0 and spo[0] == "@prefix":
                spo[item] = x[1]
            else:
                spo[item] = prefixes[prnot[0]] + prnot[1]
            item += 1
        elif (x[0] == 'URI' or x[0] == "string" or x[0] == "number" or (x[0] == "symbol" and x[1] == "a" and item == 1)) and (item < 3):
            if x[0] == "URI" and x[1].find(":") == -1 and x[1][0] != "/":
                # This is quite silly
                x = ("URI", os.path.dirname(uri) + "/" + x[1])
            spo[item] = x[1]
            item += 1
        elif x[0] == '[':
            spo_stack.append(spo)
            spo[0] = uri + "$anon$" + str(anoncnt)
            item = 1
            anoncnt += 1
        elif x[0] == ']' or x[0] == ')':
            spo = spo_stack[-1]
            spo_stack = spo_stack[:-1]
            item = 3
        elif x[0] == '(':
            spo_stack.append(spo)
            spo[0] = uri + "$anon$" + str(anoncnt)
            item = 2
            anoncnt += 1
        else:
            print uri + ": Unexpected: " + repr(x)

class FakeServer(object):
    def __init__(self):
        self.initManifests()
        #parseTTL("http://lv2plug.in/ns/lv2core#", file("/usr/lib/lv2/lv2core.lv2/lv2.ttl").read(), m)
        
    def initManifests(self):
        lv2path = ["/usr/lib/lv2", "/usr/local/lib/lv2"]
        self.manifests = SimpleRDFModel()
        self.paths = {}
        self.plugin_info = dict()
        for dir in lv2path:
            for bundle in glob.iglob(dir + "/*.lv2"):
                fn = bundle+"/manifest.ttl"
                if os.path.exists(fn):
                    parseTTL(fn, file(fn).read(), self.manifests)
        self.plugins = self.manifests.bySubject["$classes"]["http://lv2plug.in/ns/lv2core#Plugin"]
        
    def get_uris(self, base_uri):
        if base_uri == 'http://lv2plug.in/ns/lv2core#':
            return self.plugins
        raise StringException("Invalid base URI")
        
    def get_info(self, uri):
        if uri not in self.plugin_info:
            world = SimpleRDFModel()
            seeAlso = self.manifests.bySubject[uri]["http://www.w3.org/2000/01/rdf-schema#seeAlso"]
            for doc in seeAlso:
                # print "Loading " + doc
                parseTTL(doc, file(doc).read(), world)
            self.plugin_info[uri] = world                
        info = self.plugin_info[uri]
        dest = {}
        dest['name'] = info.bySubject[uri]['http://usefulinc.com/ns/doap#name'][0]
        dest['license'] = info.bySubject[uri]['http://usefulinc.com/ns/doap#license'][0]
        dest['classes'] = info.bySubject[uri]['http://usefulinc.com/ns/doap#license'][0]
        return dest

def start():
    global instance
    instance = FakeServer()

def queue(cmdObject):
    global instance
    try:
        cmdObject.calledOnOK(type(instance).__dict__[cmdObject.type](instance, *cmdObject.args))
    except:
        cmdObject.calledOnError(repr(sys.exc_info()))
    
