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
    
    def __init__(self, full_name, id, name, flags, format):
        self.full_name = full_name
        self.client_name = full_name.split(":")[0]
        self.id = int(id)
        self.name = str(name)
        self.flags = int(flags)
        self.format = int(format)
        
    def get_name(self):
        return self.name
        
    def get_full_name(self):
        return self.full_name
        
    def get_type(self):
        return self.format
        
    def is_port_input(self):
        return (self.flags & 1) != 0
    
    def is_port_output(self):
        return (self.flags & 2) != 0
    
class JACKClientInfo(object):
    id = 0
    name = ""
    def __init__(self, id, name, ports):
        self.id = id
        self.name = name
        self.ports = [JACKPortInfo("%s:%s" % (name, p[1]), *p) for p in ports]
    
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
    def get_port_color(self, portData):
        if portData.get_type() == 0:
            color = Colors.audioPort
        elif portData.get_type() == 1:
            color = Colors.eventPort
        else:
            print "Unknown type %s" % portData.get_type()
        return color
    def is_port_input(self, portData):
        return portData.is_port_input()
    def get_port_name(self, portData):
        return portData.get_name()
    def get_port_id(self, portData):
        return portData.get_full_name()
    def get_module_name(self, moduleData):
        return moduleData.name
    def fetch_graph(self):
        self.graph = JACKGraphInfo(*self.patchbay.GetGraph(self.graph.known_version if self.graph is not None else 0))
    def get_module_port_list(self, moduleData):
        g = self.graph.client_map[moduleData.name]
        return [(port.get_full_name(), port) for port in g.ports if moduleData.checker(port)]
    def can_connect(self, first, second):
        if self.is_port_input(first) == self.is_port_input(second):
            return False
        if first.get_type() != second.get_type():
            return False
        return True
    def connect(self, first, second):
        self.patchbay.ConnectPortsByName(first.client_name, first.name, second.client_name, second.name)
    def disconnect(self, name_first, name_second):
        first = name_first.split(":")
        second = name_second.split(":")
        self.patchbay.DisconnectPortsByName(first[0], first[1], second[0], second[1])
        
class ClientBoxInfo():
    def __init__(self, name, checker):
        (self.name, self.checker) = (name, checker)

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
        self.parser.disconnect(wire.src.get_id(), wire.dest.get_id())
        wire.delete()
        
    def create(self):
        self.window = gtk.Window(gtk.WINDOW_TOPLEVEL)
        self.window.connect("delete_event", self.delete_event)
        self.window.connect("destroy", self.destroy)
        self.main_vbox = gtk.VBox()
        self.create_main_menu()
        self.scroll = gtk.ScrolledWindow()
        self.scroll.add(self.cgraph.create())
        self.main_vbox.pack_start(self.menu_bar, expand = False, fill = False)
        self.main_vbox.add(self.scroll)
        self.window.add(self.main_vbox)
        self.window.show_all()
        self.main_vbox.connect("popup-menu", self.canvas_popup_menu_handler)
        self.cgraph.canvas.connect("button-press-event", self.canvas_button_press_handler)
        self.cgraph.canvas.update()
        self.add_clients(0.0, 0.0, lambda port: port.is_port_output())
        self.add_clients(200, 0.0, lambda port: port.is_port_input())
        self.cgraph.canvas.update()
        self.add_wires()
        self.cgraph.blow_up()
        
    def add_clients(self, x, y, checker):
        for cl in self.parser.graph.clients:
            y += self.cgraph.add_module(ClientBoxInfo(cl.name, checker), x, y).rect.props.height
        
    def add_wires(self):
        pmap = self.cgraph.get_port_map()
        for (c, p) in self.parser.graph.connections_name:
            if p in pmap and c in pmap:
                print "Connect %s to %s" % (c, p)
                self.cgraph.connect(pmap[c], pmap[p])
            else:
                print "Connect %s to %s - not found" % (c, p)

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
