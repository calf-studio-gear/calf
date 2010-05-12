#!/usr/bin/python
import pygtk
pygtk.require('2.0')
import gtk
import lv2
import fakeserv
import client
import calfpytools
import conndiagram
from calfgtkutils import *
Colors = conndiagram.Colors        

class LV2GraphParser():
    def get_port_color(self, portData):
        color = Colors.defPort
        if portData.isAudio:
            color = Colors.audioPort
        if portData.isControl:
            color = Colors.controlPort
        if portData.isEvent:
            color = Colors.eventPort
        return color
    def is_port_input(self, portData):
        return portData.isInput
    def get_port_name(self, portData):
        return portData.name
    def get_port_id(self, portData):
        return portData.uri
    def get_module_name(self, moduleData):
        return moduleData.name
    def get_module_port_list(self, moduleData):
        return [(port.uri, port) for port in moduleData.ports if lv2.epi + "notAutomatic" not in port.properties]
    def can_connect(self, first, second):
        return first.connectableTo(second)
    def connect(self, first, second):
        print "Connection not implemented yet"
        pass
            
class App:
    def __init__(self):
        fakeserv.start()
        self.lv2db = lv2.LV2DB()
        self.parser = LV2GraphParser()
        self.cgraph = conndiagram.ConnectionGraphEditor(self, self.parser)
        
    def canvas_popup_menu_handler(self, widget):
        self.canvas_popup_menu(0, 0, 0)
        return True
        
    def canvas_button_press_handler(self, widget, event):
        if event.button == 3:
            self.canvas_popup_menu(event.x, event.y, event.time)
            return True
        
    def add_module_cb(self, params):
        self.cgraph.add_module(*params)

    def canvas_popup_menu(self, x, y, time):
        menu = gtk.Menu()
        items = self.cgraph.get_data_items_at(x, y)
        types = set([di[0] for di in items])
        if 'port' in types:
            pass
            #add_option(menu, "-Port-", self.cgraph.delete_module_cb, (None, x, y))
        elif 'module' in types:
            for mod in [di for di in items if di[0] == "module"]:
                add_option(menu, "Delete "+mod[1].plugin.name, self.cgraph.delete_module, mod[1])            
        else:
            for uri in self.lv2db.getPluginList():
                plugin = self.lv2db.getPluginInfo(uri)
                add_option(menu, plugin.name, self.add_module_cb, (plugin, x, y))
        menu.show_all()
        menu.popup(None, None, None, 3, time)
        
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
        
    def create_main_menu(self):
        self.menu_bar = gtk.MenuBar()
        
        self.file_menu = add_submenu(self.menu_bar, "_File")
        add_option(self.file_menu, "_Exit", self.exit)
        self.plugin_menu = add_submenu(self.menu_bar, "_Plugins")
        self.submenus = dict()
        self.category_to_path = dict()
        for (path, uri) in self.lv2db.get_categories():
            if path == []:
                continue;
            parent_menu = self.plugin_menu
            parent_path = "/".join(path[:-1])
            if parent_path in self.submenus:
                parent_menu = self.submenus[parent_path]
            self.submenus["/".join(path)] = add_submenu(parent_menu, path[-1])
            self.category_to_path[uri] = path
        for uri in self.lv2db.getPluginList():
            plugin = self.lv2db.getPluginInfo(uri)
            parent_menu = self.plugin_menu
            best_path = self.get_best_category(plugin)
            if best_path in self.submenus:
                parent_menu = self.submenus[best_path]
            add_option(parent_menu, plugin.name, self.add_module_cb, (plugin, None, None))
        
    def get_best_category(self, plugin):
        max_len = -1
        best_path = []
        for path in [self.category_to_path[c] for c in plugin.classes if c in self.category_to_path]:
            if len(path) > max_len:
                best_path = path
                max_len = len(path)
        return "/".join(best_path)
        
    def exit(self, data):
        self.window.destroy()
        
    def delete_event(self, widget, data = None):
        gtk.main_quit()
    def destroy(self, widget, data = None):
        gtk.main_quit()
        
app = App()
app.create()
gtk.main()
