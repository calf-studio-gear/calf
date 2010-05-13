#!/usr/bin/python
import pygtk
pygtk.require('2.0')
import dbus
import dbus.mainloop.glib
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
        self.ports = []
        for p in ports:
            self.add_port(*p)
    def get_name(self):
        return self.name
    def add_port(self, port_id, name, flags, format):
        model = JACKPortInfo("%s:%s" % (self.name, name), port_id, name, flags, format)
        self.ports.append(model)
        return model
    
class JACKGraphInfo(object):
    version = 0
    def __init__(self, version, clients, connections):
        self.version = version
        self.clients = []
        self.client_map = {}
        self.connections_id = set()
        self.connections_name = set()
        for c in clients:
            self.add_client(*c)
        for tuple in connections:
            self.add_connection(*tuple)
    
    def add_client(self, id, name, ports):
        c = JACKClientInfo(id, name, ports)
        self.clients.append(c)
        self.client_map[name] = c
            
    def add_connection(self, cid1, cname1, pid1, pname1, cid2, cname2, pid2, pname2, something):
        self.connections_name.add(("%s:%s" % (cname1, pname1), "%s:%s" % (cname2, pname2)))
        self.connections_id.add(("%s:%s" % (cid1, pid1), "%s:%s" % (cid2, pid2)))

class ClientModuleModel():
    def __init__(self, client, checker):
        (self.client, self.checker) = (client, checker)
    def get_name(self):
        return self.client.get_name()
    def get_port_list(self):
        return filter(self.checker, self.client.ports)

class JACKGraphController(object):
    def __init__(self):
        dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)
        self.bus = dbus.SessionBus()
        self.service = self.bus.get_object("org.jackaudio.service", "/org/jackaudio/Controller")
        self.patchbay = dbus.Interface(self.service, "org.jackaudio.JackPatchbay")
        self.graph = None
        self.view = None
        self.fetch_graph()
        self.patchbay.connect_to_signal('PortAppeared', self.port_appeared)
        self.patchbay.connect_to_signal('PortDisappeared', self.port_disappeared)
        self.patchbay.connect_to_signal('PortsConnected', self.ports_connected)
        self.patchbay.connect_to_signal('PortsDisconnected', self.ports_disconnected)
    def port_appeared(self, seq_no, client_id, client_name, port_id, port_name, flags, format):
        model = self.graph.client_map[str(client_name)].add_port(int(port_id), str(port_name), int(flags_format))
        print "PortAppeared", model
    def port_disappeared(self, seq_no, client_id, client_name, port_id, port_name):
        print "PortDisappeared", str(client_name), str(port_name)
    def ports_connected(self, *args):
        print "PortsConnected", args
        self.graph.add_connection(*args[1:])
    def ports_disconnected(self, *args):
        print "PortsDisconnected", args
    def fetch_graph(self):
        req_version = self.graph.version if self.graph is not None else 0
        graphdata = self.patchbay.GetGraph(req_version)
        if int(graphdata[0]) != req_version:
            self.graph = JACKGraphInfo(*graphdata)
    def can_connect(self, first, second):
        if first.is_port_input() == second.is_port_input():
            return False
        if first.is_audio() and second.is_audio():
            return True
        if first.is_midi() and second.is_midi():
            return True
        return False
    def is_connected(self, first, second):
        return (first.get_id(), second.get_id()) in self.graph.connections_name
    def connect(self, first, second):
        if self.is_connected(first, second):
            self.disconnect(first, second)
            return
        self.patchbay.ConnectPortsByName(first.client_name, first.name, second.client_name, second.name)
        self.graph.connections_name.add((first.get_id(), second.get_id()))
        self.view.connect(self.view.get_port_view(first), self.view.get_port_view(second))
    def disconnect(self, first, second):
        self.patchbay.DisconnectPortsByName(first.client_name, first.name, second.client_name, second.name)
        self.graph.connections_name.remove((first.get_id(), second.get_id()))
        self.view.disconnect(self.view.get_port_view(first), self.view.get_port_view(second))
    def refresh(self):
        self.fetch_graph()
        self.view.clear()
        self.add_all()
    def add_all(self):
        width, left_mods = self.add_clients(10.0, 10.0, lambda port: port.is_port_output())
        width2, right_mods = self.add_clients(width + 20, 10.0, lambda port: port.is_port_input())
        pmap = self.view.get_port_map()
        for (c, p) in self.graph.connections_name:
            if p in pmap and c in pmap:
                print "Connect %s to %s" % (c, p)
                self.view.connect(pmap[c], pmap[p])
            else:
                print "Connect %s to %s - not found" % (c, p)
        for m in left_mods:
            m.translate(width - m.width, 0)
        for m in right_mods:
            m.translate(950 - width - 30 - width2, 0)
    def add_clients(self, x, y, checker):
        margin = 10
        mwidth = 0
        mods = []
        for cl in self.graph.clients:
            mod = self.view.add_module(ClientModuleModel(cl, checker), x, y)
            y += mod.rect.props.height + margin
            if mod.rect.props.width > mwidth:
                mwidth = mod.rect.props.width
            mods.append(mod)
        return mwidth, mods    
        
class App:
    def __init__(self):
        self.controller = JACKGraphController()
        self.cgraph = conndiagram.ConnectionGraphEditor(self, self.controller)
        self.controller.view = self.cgraph
        
    def canvas_popup_menu_handler(self, widget):
        self.canvas_popup_menu(0, 0, 0)
        return True
        
    def canvas_popup_menu(self, x, y, time):
        menu = gtk.Menu()
        items = self.cgraph.get_data_items_at(x, y)
        found = False
        for i in items:
            if i[0] == "wire":
                wire = i[1]
                add_option(menu, "Disconnect", self.disconnect, wire)
                found = True
                break
            elif i[0] == "port":
                port = i[1]
                add_option(menu, "Disconnect All", self.disconnect_all, port, enabled = len(port.get_connections()))
                found = True
                break
        if found:
            menu.show_all()
            menu.popup(None, None, None, 3, time)
        
    def disconnect(self, wire):
        self.controller.disconnect(wire.src.model, wire.dest.model)
        wire.remove()
        
    def disconnect_all(self, port):
        for wire in list(port.module.wires):
            if wire.src == port or wire.dest == port:
                self.controller.disconnect(wire.src.model, wire.dest.model)
                wire.remove()
        
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
        self.controller.add_all()
        self.cgraph.clear()
        self.controller.add_all()
        self.cgraph.canvas.update()
        #this is broken
        #self.cgraph.blow_up()
                
    def create_main_menu(self):
        self.menu_bar = gtk.MenuBar()
        self.file_menu = add_submenu(self.menu_bar, "_File")
        add_option(self.file_menu, "_Refresh", self.refresh)
        add_option(self.file_menu, "_Blow-up", self.blow_up)
        add_option(self.file_menu, "_Exit", self.exit)
        
    def refresh(self, data):
        self.controller.refresh()
    
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
