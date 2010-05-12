#!/usr/bin/python
import pygtk
pygtk.require('2.0')
import dbus
import gtk
import conndiagram
from calfgtkutils import *
Colors = conndiagram.Colors

class JACKPortInfo(object):
    id = 0
    name = ""
    flags = 0
    format = 0
    
    def __init__(self, full_name, port_id, name, flags, format):
        self.full_name = full_name
        self.client_name = full_name.split(":", 1)[0]
        self.port_id = int(port_id)
        self.name = str(name)
        self.flags = int(flags)
        self.format = int(format)
        
    def get_name(self):
        return self.name
        
    def get_id(self):
        return self.full_name
        
    def is_audio(self):
        return self.format == 0
        
    def is_midi(self):
        return self.format == 1
        
    def is_port_input(self):
        return (self.flags & 1) != 0
    
    def is_port_output(self):
        return (self.flags & 2) != 0
    
    def get_port_color(self):
        if self.is_audio():
            return Colors.audioPort
        elif self.is_midi():
            return Colors.eventPort
        else:
            print "Unknown type %s" % self.format
            return Colors.controlPort

class JACKClientInfo(object):
    id = 0
    name = ""
    def __init__(self, id, name, ports):
        self.id = id
        self.name = name
        self.ports = [JACKPortInfo("%s:%s" % (name, p[1]), *p) for p in ports]
    def get_name(self):
        return self.name
    
class JACKGraphInfo(object):
    version = 0
    clients = []
    client_map = {}
    connections_id = set()
    connections_name = set()
    def __init__(self, version, clients, connections):
        self.version = version
        self.clients = [JACKClientInfo(*c) for c in clients]
        self.client_map = {}
        for c in self.clients:
            self.client_map[c.name] = c
        self.connections_id = set()
        self.connections_name = set()
        for (cid1, cname1, pid1, pname1, cid2, cname2, pid2, pname2, something) in connections:
            self.connections_name.add(("%s:%s" % (cname1, pname1), "%s:%s" % (cname2, pname2)))
            self.connections_id.add(("%s:%s" % (cid1, pid1), "%s:%s" % (cid2, pid2)))

class JACKGraphParser(object):
    def __init__(self):
        self.bus = dbus.SessionBus()
        self.service = self.bus.get_object("org.jackaudio.service", "/org/jackaudio/Controller")
        self.patchbay = dbus.Interface(self.service, "org.jackaudio.JackPatchbay")
        self.graph = None
        self.fetch_graph()
    def fetch_graph(self):
        self.graph = JACKGraphInfo(*self.patchbay.GetGraph(self.graph.known_version if self.graph is not None else 0))
    def can_connect(self, first, second):
        if first.is_port_input() == second.is_port_input():
            return False
        if first.is_audio() and second.is_audio():
            return True
        if first.is_midi() and second.is_midi():
            return True
        return False
    def connect(self, first, second):
        self.patchbay.ConnectPortsByName(first.client_name, first.name, second.client_name, second.name)
    def disconnect(self, first, second):
        self.patchbay.DisconnectPortsByName(first.client_name, first.name, second.client_name, second.name)
        
class ClientModuleModel():
    def __init__(self, client, checker):
        (self.client, self.checker) = (client, checker)
    def get_name(self):
        return self.client.get_name()
    def get_port_list(self):
        return filter(self.checker, self.client.ports)

class App:
    def __init__(self):
        self.parser = JACKGraphParser()
        self.cgraph = conndiagram.ConnectionGraphEditor(self, self.parser)
        
    def canvas_popup_menu_handler(self, widget):
        self.canvas_popup_menu(0, 0, 0)
        return True
        
    def canvas_button_press_handler(self, widget, event):
        if event.button == 3:
            self.canvas_popup_menu(event.x, event.y, event.time)
            return True
        
    def canvas_popup_menu(self, x, y, time):
        menu = gtk.Menu()
        items = self.cgraph.get_data_items_at(x, y)
        types = set([di[0] for di in items])
        if 'wire' in types:
            for i in items:
                if i[0] == "wire":
                    wire = i[1]
                    add_option(menu, "Disconnect", self.disconnect, wire)
        else:
            return
        menu.show_all()
        menu.popup(None, None, None, 3, time)
        
    def disconnect(self, wire):
        self.parser.disconnect(wire.src.model, wire.dest.model)
        wire.delete()
        
    def create(self):
        self.window = gtk.Window(gtk.WINDOW_TOPLEVEL)
        self.window.connect("delete_event", self.delete_event)
        self.window.connect("destroy", self.destroy)
        self.main_vbox = gtk.VBox()
        self.create_main_menu()
        self.scroll = gtk.ScrolledWindow()
        self.scroll.add(self.cgraph.create(950, 650))
        self.main_vbox.pack_start(self.menu_bar, expand = False, fill = False)
        self.main_vbox.add(self.scroll)
        self.window.add(self.main_vbox)
        self.window.show_all()
        self.main_vbox.connect("popup-menu", self.canvas_popup_menu_handler)
        self.cgraph.canvas.connect("button-press-event", self.canvas_button_press_handler)
        self.add_all()
        self.cgraph.canvas.update()
        #this is broken
        #self.cgraph.blow_up()
        
    def add_all(self):    
        width, left_mods = self.add_clients(10.0, 10.0, lambda port: port.is_port_output())
        width2, right_mods = self.add_clients(width + 20, 10.0, lambda port: port.is_port_input())
        pmap = self.cgraph.get_port_map()
        for (c, p) in self.parser.graph.connections_name:
            if p in pmap and c in pmap:
                print "Connect %s to %s" % (c, p)
                self.cgraph.connect(pmap[c], pmap[p])
            else:
                print "Connect %s to %s - not found" % (c, p)
        for m in right_mods:
            m.translate(950 - width - 20 - width2, 0)
    
    def add_clients(self, x, y, checker):
        margin = 10
        mwidth = 0
        mods = []
        for cl in self.parser.graph.clients:
            mod = self.cgraph.add_module(ClientModuleModel(cl, checker), x, y)
            y += mod.rect.props.height + margin
            if mod.rect.props.width > mwidth:
                mwidth = mod.rect.props.width
            mods.append(mod)
        return mwidth, mods
        
    def create_main_menu(self):
        self.menu_bar = gtk.MenuBar()
        self.file_menu = add_submenu(self.menu_bar, "_File")
        add_option(self.file_menu, "_Blow-up", self.blow_up)
        add_option(self.file_menu, "_Exit", self.exit)
        
    def blow_up(self, data):
        self.cgraph.blow_up()
    
    def exit(self, data):
        self.window.destroy()
        
    def delete_event(self, widget, data = None):
        gtk.main_quit()
    def destroy(self, widget, data = None):
        gtk.main_quit()
        
app = App()
app.create()
gtk.main()
