#!/usr/bin/python
import pygtk
pygtk.require('2.0')
import gtk
import lv2
import fakeserv
import client
import cairo
import goocanvas
import random

class ModuleBox():
    def __init__(self, parent, plugin):
        self.width = 200
        self.group = None
        self.parent = parent
        self.plugin = plugin
        self.group = goocanvas.Group(parent = self.parent)
        self.create_items()
        
    def create_items(self):
        fontName = "DejaVu Sans Mono Book 10"
        self.group.module = self
        while self.group.get_n_children() > 0:
            self.group.remove_child(0)
        ctx = self.group.get_canvas().create_cairo_context()
        width = 100
        margin = 2
        spacing = 6
        portBoxes = {}
        portTitles = {}
        frameColor = 0xC0C0C0FF
        textColor = 0xFFFFFFFF
        self.title = goocanvas.Text(parent = self.group, font = fontName, text = "<b>" + self.plugin.name + "</b>", width = 100, x = 0, y = 0, alignment = "center", use_markup = True, fill_color_rgba = textColor)
        y = self.title.get_requested_height(ctx, width) + spacing
        for port in self.plugin.ports:
            al = "center"
            if port.isInput: 
                al = "left"
            elif port.isOutput:
                al = "right"
            title = goocanvas.Text(parent = self.group, text = port.name, font = fontName, width = width - 2 * margin, x = margin, y = y, alignment = al, fill_color_rgba = textColor)
            height = 1 + int(title.get_requested_height(ctx, width - 2 * margin))
            title.ensure_updated()
            bnds = title.get_bounds()
            bw = bnds.x2 - bnds.x1 + 2 * margin
            color = 0x404040FF
            if port.isAudio:
                color = 0x000080FF
            if port.isControl:
                color = 0x008000FF
            if port.isEvent:
                color = 0x800000FF
            if port.isInput:
                box = goocanvas.Rect(parent = self.group, x = 0, y = y - 1, width = bw, height = height + 2, line_width = 1, fill_color_rgba = color, stroke_color_rgba = frameColor)
            elif port.isOutput:
                box = goocanvas.Rect(parent = self.group, x = bnds.x2 - margin, y = y - 1, width = width - bnds.x2 + margin, height = height + 2, line_width = 1, fill_color_rgba = color, stroke_color_rgba = frameColor)
            box.lower(title)
            y += height + spacing
            portBoxes[port.uri] = box
            portTitles[port.uri] = title
            box.uri = port.uri
            box.connect_object("button-press-event", self.port_button_press, port.uri)
            title.connect_object("button-press-event", self.port_button_press, port.uri)
        self.rect = goocanvas.Rect(parent = self.group, width = 100, height = y, line_width = 1, stroke_color_rgba = frameColor, fill_color_rgba = 0x202020FF)
        self.rect.lower(self.title)
        self.portBoxes = portBoxes
        self.portTitles = portTitles
        self.group.ensure_updated()
        
    def delete_items(self):
        self.group.remove()
        
    def port_button_press(self, port_uri, box, event):
        if event.button == 1:
            print "Port URI is " + port_uri
            return True

class App:
    def __init__(self):
        fakeserv.start()
        self.lv2db = lv2.LV2DB()
        
    def create(self):
        self.window = gtk.Window(gtk.WINDOW_TOPLEVEL)
        self.window.connect("delete_event", self.delete_event)
        self.window.connect("destroy", self.destroy)
        self.main_vbox = gtk.VBox()
        self.create_main_menu()
        self.scroll = gtk.ScrolledWindow()
        self.create_canvas()
        self.scroll.add(self.canvas)
        self.main_vbox.pack_start(self.menu_bar, expand = False, fill = False)
        self.main_vbox.add(self.scroll)
        self.window.add(self.main_vbox)
        self.window.show_all()
        self.moving = None
        
    def create_main_menu(self):
        self.menu_bar = gtk.MenuBar()
        
        self.file_menu = self.add_submenu(self.menu_bar, "_File")
        self.add_option(self.file_menu, "_Exit", self.exit)
        self.plugin_menu = self.add_submenu(self.menu_bar, "_Plugins")
        for uri in self.lv2db.getPluginList():
            plugin = self.lv2db.getPluginInfo(uri)
            self.add_option(self.plugin_menu, plugin.name, self.add_plugin, (plugin, None, None))
        
    def create_canvas(self):
        self.canvas = goocanvas.Canvas()
        self.canvas.props.automatic_bounds = True
        self.canvas.set_size_request(640, 480)
        self.canvas.set_scale(1)
        #self.canvas.connect("size-allocate", self.update_canvas_bounds)
        self.canvas.props.background_color_rgb = 0
        #self.canvas.props.integer_layout = True
        self.canvas.update()
        self.canvas.get_root_item().connect("button-press-event", self.canvas_button_press)
        
    def add_submenu(self, menu, option):
        submenu = gtk.Menu()
        item = gtk.MenuItem(option)
        menu.append(item)
        item.set_submenu(submenu)
        return submenu
    
    def add_option(self, menu, option, handler, data = None):
        item = gtk.MenuItem(option)
        item.connect_object("activate", handler, data)
        menu.add(item)
        
    def add_plugin(self, params):
        (plugin, x, y) = params
        mbox = ModuleBox(self.canvas.get_root_item(), plugin)
        bounds = self.canvas.get_bounds()
        if x == None:
            (x, y) = (random.randint(bounds[0], bounds[2] - 100), random.randint(bounds[1], bounds[3] - 50))
        mbox.group.translate(x, y)
        mbox.group.connect("button-press-event", self.box_button_press)
        mbox.group.connect("button-release-event", self.box_button_release)
        mbox.group.connect("motion-notify-event", self.box_motion_notify)
        
    def canvas_button_press(self, widget, item, event):
        if event.button == 3:
            menu = gtk.Menu()
            for uri in self.lv2db.getPluginList():
                plugin = self.lv2db.getPluginInfo(uri)
                self.add_option(menu, plugin.name, self.add_plugin, (plugin, event.x, event.y))
            menu.show_all()
            menu.popup(None, None, None, event.button, event.time)
        
    def box_button_press(self, group, widget, event):
        if event.button == 1:
            self.moving = group
            self.motion_x = event.x
            self.motion_y = event.y
            return True
        elif event.button == 3:
            menu = gtk.Menu()
            self.add_option(menu, "Delete", self.delete_block, group.module)
            menu.show_all()
            menu.popup(None, None, None, event.button, event.time)
            return True
    
    def box_button_release(self, group, widget, event):
        if event.button == 1:
            self.moving = None
    
    def box_motion_notify(self, group, widget, event):
        if self.moving == group:
            self.moving.translate(event.x - self.motion_x, event.y - self.motion_y)
    
    def exit(self, data):
        self.window.destroy()
        
    def delete_block(self, data):
        data.delete_items()

    def delete_event(self, widget, data = None):
        gtk.main_quit()
    def destroy(self, widget, data = None):
        gtk.main_quit()
        
app = App()
app.create()
gtk.main()
