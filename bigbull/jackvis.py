#!/usr/bin/python
import pygtk
pygtk.require('2.0')
import gtk
import calfpytools
import conndiagram
from calfgtkutils import *
Colors = conndiagram.Colors        

class JACKGraphParser():
    def __init__(self):
        self.client = calfpytools.JackClient()
        self.client.open("jackvis")
    def get_port_color(self, portData):
        if portData.get_type() == calfpytools.JACK_DEFAULT_AUDIO_TYPE:
            color = Colors.audioPort
        if portData.get_type() == calfpytools.JACK_DEFAULT_MIDI_TYPE:
            color = Colors.eventPort
        return color
    def is_port_input(self, portData):
        return (portData.get_flags() & calfpytools.JackPortIsInput) != 0
    def get_port_name(self, portData):
        return portData.get_name()
    def get_port_id(self, portData):
        return portData.get_full_name()
    def get_module_name(self, moduleData):
        return moduleData.name
    def get_module_port_list(self, moduleData):
        return [(port, self.client.get_port(port)) for port in self.client.get_ports(moduleData.name+":.*", moduleData.type, moduleData.flags)]
    def can_connect(self, first, second):
        if self.is_port_input(first) == self.is_port_input(second):
            return False
        if first.get_type() != second.get_type():
            return False
        return True
    def connect(self, first, second):
        if self.client.connect(first.get_full_name(), second.get_full_name()) != True:
            raise RuntimeError, "Connection error"
    def disconnect(self, name_first, name_second):
        self.client.disconnect(name_first, name_second)
        
class JACKClientData():
    def __init__(self, name, type, flags):
        (self.name, self.type, self.flags) = (name, type, flags)

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
        self.add_clients(0.0, 0.0, calfpytools.JackPortIsOutput)
        self.add_clients(200, 0.0, calfpytools.JackPortIsInput)
        self.cgraph.canvas.update()
        self.add_wires()
        self.cgraph.blow_up()
        
    def add_clients(self, x, y, flags):
        ports = self.parser.client.get_ports("", "", flags)
        clients = set([p.split(":")[0] for p in ports])
        for cl in clients:
            y += self.cgraph.add_module(JACKClientData(cl, "", flags), x, y).rect.props.height
        
    def add_wires(self):
        ports = self.parser.client.get_ports("", "", calfpytools.JackPortIsInput)
        pmap = self.cgraph.get_port_map()
        for p in ports:
            conns = self.parser.client.get_port(p).get_connections()
            for c in conns:
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
