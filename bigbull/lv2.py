import re
import os
import sys
import glob
import calfpytools

lv2 = "http://lv2plug.in/ns/lv2core#"
lv2evt = "http://lv2plug.in/ns/ext/event#"
lv2str = "http://lv2plug.in/ns/dev/string-port#"
rdf = "http://www.w3.org/1999/02/22-rdf-syntax-ns#"
rdfs = "http://www.w3.org/2000/01/rdf-schema#"
epi = "http://lv2plug.in/ns/dev/extportinfo#"
rdf_type = rdf + "type"
tinyname_uri = "http://lv2plug.in/ns/dev/tiny-name"
foaf = "http://xmlns.com/foaf/0.1/"
doap = "http://usefulinc.com/ns/doap#"

event_type_names = {
    "http://lv2plug.in/ns/ext/midi#MidiEvent" : "MIDI"
}

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

def parseTTL(uri, content, model, debug):
    # Missing stuff: translated literals, blank nodes
    if debug:
        print "Parsing: %s" % uri
    prefixes = {}
    spo_stack = []
    spo = ["", "", ""]
    item = 0
    anoncnt = 1
    for x in calfpytools.scan_ttl_string(content):
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
                if x[0] == '.':
                    item = 0
                elif item != 0:
                    raise Exception, uri+": Unexpected " + x[0]
        elif x[0] == "prnot" and item < 3:
            prnot = x[1].split(":")
            if item != 0 and spo[0] == "@prefix":
                spo[item] = x[1]
            elif prnot[0] == "_":
                spo[item] = uri + "#" + prnot[1]
            else:
                if prnot[0] not in prefixes:
                    raise Exception, "Prefix %s not defined" % prnot[0]
                else:
                    spo[item] = prefixes[prnot[0]] + prnot[1]
            item += 1
        elif (x[0] == 'URI' or x[0] == "string" or x[0] == "number" or (x[0] == "symbol" and x[1] == "a" and item == 1)) and (item < 3):
            if x[0] == "URI" and x[1] == "":
                x = ("URI", uri)
            elif x[0] == "URI" and x[1].find(":") == -1 and x[1] != "" and x[1][0] != "/":
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

class LV2Port(object):
    def __init__(self):
        pass
    def connectableTo(self, port):
        if not ((self.isInput and port.isOutput) or (self.isOutput and port.isInput)):
            return False
        if self.isAudio != port.isAudio or self.isControl != port.isControl or self.isEvent != port.isEvent:
            return False
        if not self.isAudio and not self.isControl and not self.isEvent:
            return False
        return True

class LV2Plugin(object):
    def __init__(self):
        pass
        
class LV2DB:
    def __init__(self, debug = False):
        self.debug = debug
        self.initManifests()
        
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
                    parseTTL(fn, file(fn).read(), self.manifests, self.debug)
        # Read all specifications from all manifests
        if (lv2 + "Specification" in self.manifests.bySubject["$classes"]):
            specs = self.manifests.getByType(lv2 + "Specification")
            filenames = set()
            for spec in specs:
                subj = self.manifests.bySubject[spec]
                if rdfs+"seeAlso" in subj:
                    for fn in subj[rdfs+"seeAlso"]:
                        filenames.add(fn)
            for fn in filenames:
                parseTTL(fn, file(fn).read(), self.manifests, self.debug)
        #fn = "/usr/lib/lv2/lv2core.lv2/lv2.ttl"
        #parseTTL(fn, file(fn).read(), self.manifests)
        self.plugins = self.manifests.getByType(lv2 + "Plugin")
        self.categories = set()
        self.category_paths = []
        self.add_category_recursive([], lv2 + "Plugin")
        
    def add_category_recursive(self, tree_pos, category):
        cat_name = self.manifests.getProperty(category, rdfs + "label", single = True, optional = True)
        self.category_paths.append(((tree_pos + [cat_name])[1:], category))
        self.categories.add(category)
        items = self.manifests.byPredicate[rdfs + "subClassOf"]
        for subj in items:
            if subj in self.categories:
                continue
            for o in items[subj]:
                if o == category and subj not in self.categories:
                    self.add_category_recursive(list(tree_pos) + [cat_name], subj)
        
    def get_categories(self):
        return self.category_paths
        
    def getPluginList(self):
        return self.plugins
        
    def getPluginInfo(self, uri):
        if uri not in self.plugin_info:
            world = SimpleRDFModel()
            world.copyFrom(self.manifests)
            seeAlso = self.manifests.bySubject[uri]["http://www.w3.org/2000/01/rdf-schema#seeAlso"]
            try:
                for doc in seeAlso:
                    # print "Loading " + doc + " for plugin " + uri
                    parseTTL(doc, file(doc).read(), world, self.debug)
                self.plugin_info[uri] = world                
            except Exception, e:
                print "ERROR %s: %s" % (uri, str(e))
                return None
        info = self.plugin_info[uri]
        dest = LV2Plugin()
        dest.uri = uri
        dest.name = info.bySubject[uri][doap + 'name'][0]
        dest.license = info.bySubject[uri][doap + 'license'][0]
        dest.classes = info.bySubject[uri]["a"]
        dest.requiredFeatures = info.getProperty(uri, lv2 + "requiredFeature", optional = True)
        dest.optionalFeatures = info.getProperty(uri, lv2 + "optionalFeature", optional = True)
        dest.microname = info.getProperty(uri, tinyname_uri, optional = True)
        if len(dest.microname):
            dest.microname = dest.microname[0]
        else:
            dest.microname = None
        dest.maintainers = []
        if info.bySubject[uri].has_key(doap + "maintainer"):
            for maintainer in info.bySubject[uri][doap + "maintainer"]:
                maintainersubj = info.bySubject[maintainer]
                maintainerdict = {}
                maintainerdict['name'] = info.getProperty(maintainersubj, foaf + "name")[0]
                homepages = info.getProperty(maintainersubj, foaf + "homepage")
                if homepages:
                    maintainerdict['homepage'] = homepages[0]
                mboxes = info.getProperty(maintainersubj, foaf + "mbox")
                if mboxes:
                    maintainerdict['mbox'] = mboxes[0]
                dest.maintainers.append(maintainerdict)
        ports = []
        portDict = {}
        porttypes = {
            "isAudio" : lv2 + "AudioPort",
            "isControl" : lv2 + "ControlPort",
            "isEvent" : lv2evt + "EventPort",
            "isString" : lv2str + "StringPort",
            "isInput" : lv2 + "InputPort",
            "isOutput" : lv2 + "OutputPort",
            "isLarslMidi" : "http://ll-plugins.nongnu.org/lv2/ext/MidiPort",
        }
        
        for port in info.bySubject[uri][lv2 + "port"]:
            psubj = info.bySubject[port]
            pdata = LV2Port()
            pdata.uri = port
            pdata.index = int(info.getProperty(psubj, lv2 + "index")[0])
            pdata.symbol = info.getProperty(psubj, lv2 + "symbol")[0]
            pdata.name = info.getProperty(psubj, lv2 + "name")[0]
            classes = set(info.getProperty(psubj, "a"))
            pdata.classes = classes
            for pt in porttypes.keys():
                pdata.__dict__[pt] = porttypes[pt] in classes
            sp = info.getProperty(psubj, lv2 + "scalePoint")
            if sp and len(sp):
                splist = []
                for pt in sp:
                    name = info.getProperty(pt, rdfs + "label", optional = True, single = True)
                    if name != None:
                        value = info.getProperty(pt, rdf + "value", optional = True, single = True)
                        if value != None:
                            splist.append((name, value))
                pdata.scalePoints = splist
            else:
                pdata.scalePoints = []
            if pdata.isControl:
                pdata.defaultValue = info.getProperty(psubj, [lv2 + "default"], optional = True, single = True)
            elif pdata.isString:
                pdata.defaultValue = info.getProperty(psubj, [lv2str + "default"], optional = True, single = True)
            else:
                pdata.defaultValue = None
            pdata.minimum = info.getProperty(psubj, [lv2 + "minimum"], optional = True, single = True)
            pdata.maximum = info.getProperty(psubj, [lv2 + "maximum"], optional = True, single = True)
            pdata.microname = info.getProperty(psubj, [tinyname_uri], optional = True, single = True)
            pdata.properties = set(info.getProperty(psubj, [lv2 + "portProperty"], optional = True))
            pdata.events = set(info.getProperty(psubj, [lv2evt + "supportsEvent"], optional = True))
            ports.append(pdata)
            portDict[pdata.uri] = pdata
        ports.sort(lambda x, y: cmp(x.index, y.index))
        dest.ports = ports
        dest.portDict = portDict
        return dest

