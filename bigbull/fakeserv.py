import re
import os
import sys
import glob
import yappy.parser

rdf = "http://www.w3.org/1999/02/22-rdf-syntax-ns#"
rdfs = "http://www.w3.org/2000/01/rdf-schema#"
rdf_type = rdf + "type"

class DumpRDFModel:
    def addTriple(self, s, p, o):
        print "%s [%s] %s" % (s, p, repr(o))

class SimpleRDFModel:
    def __init__(self):
        self.bySubject = {}
        self.byPredicate = {rdfs+"subClassOf":{}, rdfs+"subPropertyOf":{}}
    def getByType(self, classname):
        classes = self.bySubject["$classes"]
        if classname in classes:
            return classes[classname]
        return []
    def getByPropType(self, propname):
        if propname in self.byPredicate:
            return self.byPredicate[propname]
        return []
    def getByTypeWithSubclasses(self, classname):
        classes = set(self.getByType(classname))
        if classname in self.subclassesTrans:
            for sc in self.subclassesTrans[classname]:
                classes |= set(self.getByType(sc))
        return classes
    def getPropertyTrans(self, subject, props, optional = False, single = False):
        if type(props) is list:
            prop = props[0]
        else:
            prop = props
        # special case for rdf:value and literals
        if prop == rdf + "value":
            if type(subject) is not dict:
                if single:
                    return subject
                else:
                    return [subject]
            else:
                raise Exception, "multiple rdf:value triplets, which is correct but not supported yet"
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
        for subprop in self.getSubpropsTrans(prop):
            if subprop in subject:
                for o in subject[subprop]:
                    anyprops.add(o)
        if type(props) is list:
            if len(props) > 1:
                result = set()
                for v in anyprops:
                    if single:
                        value = self.getPropertyTrans(v, props[1:], optional = optional, single = True)
                        if value != None:
                            return value
                    else:
                        result |= set(self.getPropertyTrans(v, props[1:], optional = optional, single = False))
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
        self.reindex()
    def reindex(self):
        added = True
        self.subclasses = {}
        self.subclassesTrans = {}
        self.subprops = {}
        self.subpropsTrans = {}
        subjects = self.byPredicate[rdfs + "subClassOf"]
        for s in subjects.keys():
            for o in subjects[s]:
                if o not in self.subclasses:
                    self.subclasses[o] = set()
                self.subclasses[o].add(s)
        # does not understand triples like S rdfs:subPropertyOf rdfs:subPropertyOf . (new ways to express subpropertiness) - so what?
        subjects = self.byPredicate[rdfs + "subPropertyOf"]
        for s in subjects.keys():
            for o in subjects[s]:
                if o not in self.subprops:
                    self.subprops[o] = set()
                self.subprops[o].add(s)
        for classname in self.getByType(rdfs + "Class"):
            self.getSubclassesTrans(classname)
        for propname in self.getByType(rdfs + "Property"):
            self.getPropertiesTrans(propname)
    def printClasses(self):
        for classname in self.subclassesTrans.keys():
            if len(self.subclassesTrans[classname]) > 0:
                print "%s -> %s" % (classname, self.subclassesTrans[classname])
                
    def getAllSubclasses(self, classname):
        return set([classname]) | self.getSubclassesTrans(classname)
        
    def getSubclassesTrans(self, classname):
        if classname in self.subclassesTrans:
            if self.subclassesTrans[classname] == None:
                print "Warning: circular reference at " + classname
            return self.subclassesTrans[classname]
        self.subclassesTrans[classname] = None
        sct = set()
        if classname in self.subclasses:
            for cn in self.subclasses[classname]:
                sct.add(cn)
                sct |= self.getSubclassesTrans(cn)
        self.subclassesTrans[classname] = sct
        return sct
        
    def getSubpropsTrans(self, propname):
        if propname in self.subpropsTrans:
            if self.subpropsTrans[propname] == None:
                print "Warning: circular reference at " + propname
            return self.subpropsTrans[propname]
        self.subpropsTrans[propname] = None
        spt = set()
        if propname in self.subprops:
            for cn in self.subprops[propname]:
                spt.add(cn)
                spt |= self.getSubpropsTrans(cn)
        self.subpropsTrans[propname] = spt
        return spt
        
    def dump(self):
        for s in self.bySubject.keys():
            for p in self.bySubject[s].keys():
                print "%s %s %s" % (s, p, self.bySubject[s][p])

def parseTTL(uri, content, model):
    # Missing stuff: translated literals, blank nodes
    print "Parsing: %s" % uri
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
                if x[0] == '.': item = 0
                elif x[0] == ';': item = 1
                elif x[0] == ',': item = 2
            else:
                raise Exception, uri+": Unexpected " + x[0]
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
            if item != 2:
                raise Exception, "Incorrect use of ["
            uri2 = uri + "$anon$" + str(anoncnt)
            spo[2] = uri2
            spo_stack.append(spo)
            spo = [uri2, "", ""]
            item = 1
            anoncnt += 1
        elif x[0] == ']' or x[0] == ')':
            if item == 3:
                model.addTriple(spo[0], spo[1], spo[2])
                item = 0
            spo = spo_stack[-1]
            spo_stack = spo_stack[:-1]
            item = 3
        elif x[0] == '(':
            if item != 2:
                raise Exception, "Incorrect use of ("
            uri2 = uri + "$anon$" + str(anoncnt)
            spo[2] = uri2
            spo_stack.append(spo)
            spo = [uri2, "", ""]
            item = 2
            anoncnt += 1
        else:
            print uri + ": Unexpected: " + repr(x)

class FakeServer(object):
    def __init__(self):
        self.lv2 = "http://lv2plug.in/ns/lv2core#"
        self.lv2evt = "http://lv2plug.in/ns/ext/event#"
        self.initManifests()
        #parseTTL("http://lv2plug.in/ns/lv2core#", file("/usr/lib/lv2/lv2core.lv2/lv2.ttl").read(), m)
        
    def initManifests(self):
        lv2path = ["/usr/lib/lv2", "/usr/local/lib/lv2"]
        self.manifests = SimpleRDFModel()
        self.paths = {}
        self.plugin_info = dict()
        # Scan manifests
        for dir in lv2path:
            for bundle in glob.iglob(dir + "/*.lv2"):
                fn = bundle+"/manifest.ttl"
                if os.path.exists(fn):
                    parseTTL(fn, file(fn).read(), self.manifests)
        # Read all specifications from all manifests
        self.manifests.reindex()
        if (self.lv2 + "Specification" in self.manifests.bySubject["$classes"]):
            specs = self.manifests.getByTypeWithSubclasses(self.lv2 + "Specification")
            filenames = set()
            for spec in specs:
                subj = self.manifests.bySubject[spec]
                if rdfs+"seeAlso" in subj:
                    for fn in subj[rdfs+"seeAlso"]:
                        filenames.add(fn)
            for fn in filenames:
                parseTTL(fn, file(fn).read(), self.manifests)
                self.manifests.reindex()
        #fn = "/usr/lib/lv2/lv2core.lv2/lv2.ttl"
        #parseTTL(fn, file(fn).read(), self.manifests)
        self.plugins = self.manifests.getByTypeWithSubclasses(self.lv2 + "Plugin")
        
    def get_uris(self, base_uri):
        if base_uri == 'http://lv2plug.in/ns/lv2core#':
            return self.plugins
        raise StringException("Invalid base URI")
        
    def get_info(self, uri):
        if uri not in self.plugin_info:
            world = SimpleRDFModel()
            world.copyFrom(self.manifests)
            seeAlso = self.manifests.bySubject[uri]["http://www.w3.org/2000/01/rdf-schema#seeAlso"]
            for doc in seeAlso:
                # print "Loading " + doc
                parseTTL(doc, file(doc).read(), world)
            world.reindex()
            self.plugin_info[uri] = world                
        info = self.plugin_info[uri]
        dest = {}
        dest['name'] = info.bySubject[uri]['http://usefulinc.com/ns/doap#name'][0]
        dest['license'] = info.bySubject[uri]['http://usefulinc.com/ns/doap#license'][0]
        dest['classes'] = set(info.bySubject[uri]["a"])
        ports = []
        porttypes = {
            "isAudio" : info.getAllSubclasses(self.lv2 + "AudioPort"),
            "isControl" : info.getAllSubclasses(self.lv2 + "ControlPort"),
            "isEvent" : info.getAllSubclasses(self.lv2evt + "EventPort"),
            "isInput" : info.getAllSubclasses(self.lv2 + "InputPort"),
            "isOutput" : info.getAllSubclasses(self.lv2 + "OutputPort"),            
        }
        
        for port in info.bySubject[uri][self.lv2 + "port"]:
            psubj = info.bySubject[port]
            pdata = {}
            pdata['index'] = info.getPropertyTrans(psubj, self.lv2 + "index")[0]
            pdata['symbol'] = info.getPropertyTrans(psubj, self.lv2 + "symbol")[0]
            pdata['name'] = info.getPropertyTrans(psubj, self.lv2 + "name")[0]
            classes = set(info.getPropertyTrans(psubj, "a"))
            pdata['classes'] = classes
            for pt in porttypes.keys():
                pdata[pt] = len(classes & porttypes[pt])
            pdata['scalePoints'] = info.getPropertyTrans(psubj, self.lv2 + "scalePoint")
            pdata['default'] = info.getPropertyTrans(psubj, [self.lv2 + "default", rdf + "value"], optional = True, single = True)
            pdata['minimum'] = info.getPropertyTrans(psubj, [self.lv2 + "minimum", rdf + "value"], optional = True, single = True)
            pdata['maximum'] = info.getPropertyTrans(psubj, [self.lv2 + "maximum", rdf + "value"], optional = True, single = True)
            ports.append(pdata)
        dest['ports'] = ports
        return dest

def start():
    global instance
    instance = FakeServer()

def queue(cmdObject):
    global instance
    #try:
    cmdObject.calledOnOK(type(instance).__dict__[cmdObject.type](instance, *cmdObject.args))
    #except:
    #    cmdObject.calledOnError(repr(sys.exc_info()))
    
