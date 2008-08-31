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
import calfpytools

def add_option(menu, option, handler, data = None):
    item = gtk.MenuItem(option)
    item.connect_object("activate", handler, data)
    menu.add(item)

def add_submenu(menu, option):
    submenu = gtk.Menu()
    item = gtk.MenuItem(option)
    menu.append(item)
    item.set_submenu(submenu)
    return submenu
        
class Colors:
    frame = 0xC0C0C0FF
    text = 0xFFFFFFFF
    box = 0x202020FF
    defPort = 0x404040FF
    audioPort = 0x000080FF
    controlPort = 0x008000FF
    eventPort = 0x800000FF

class ModuleBox():
    def __init__(self, parent, plugin, graph):
        self.width = 200
        self.graph = graph
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
        portData = {}
        self.title = goocanvas.Text(parent = self.group, font = fontName, text = "<b>" + self.plugin.name + "</b>", width = 100, x = 0, y = 0, alignment = "center", use_markup = True, fill_color_rgba = Colors.text)
        y = self.title.get_requested_height(ctx, width) + spacing
        for port in self.plugin.ports:
            al = "center"
            if port.isInput: 
                al = "left"
            elif port.isOutput:
                al = "right"
            title = goocanvas.Text(parent = self.group, text = port.name, font = fontName, width = width - 2 * margin, x = margin, y = y, alignment = al, fill_color_rgba = Colors.text)
            height = 1 + int(title.get_requested_height(ctx, width - 2 * margin))
            title.ensure_updated()
            bnds = title.get_bounds()
            bw = bnds.x2 - bnds.x1 + 2 * margin
            color = Colors.defPort
            if port.isAudio:
                color = Colors.audioPort
            if port.isControl:
                color = Colors.controlPort
            if port.isEvent:
                color = Colors.eventPort
            if port.isInput:
                box = goocanvas.Rect(parent = self.group, x = 0, y = y - 1, width = bw, height = height + 2, line_width = 1, fill_color_rgba = color, stroke_color_rgba = Colors.frame)
            elif port.isOutput:
                box = goocanvas.Rect(parent = self.group, x = bnds.x2 - margin, y = y - 1, width = width - bnds.x2 + margin, height = height + 2, line_width = 1, fill_color_rgba = color, stroke_color_rgba = Colors.frame)
            else:
                continue
            box.lower(title)
            y += height + spacing
            portBoxes[port.uri] = box
            portTitles[port.uri] = title
            portData[port.uri] = port
            box.uri = port.uri
            box.connect_object("button-press-event", self.port_button_press, port.uri)
            title.connect_object("button-press-event", self.port_button_press, port.uri)
        self.rect = goocanvas.Rect(parent = self.group, width = 100, height = y, line_width = 1, stroke_color_rgba = Colors.frame, fill_color_rgba = Colors.box)
        self.rect.lower(self.title)
        self.portBoxes = portBoxes
        self.portTitles = portTitles
        self.portData = portData
        self.group.ensure_updated()
        self.wire = None
        
    def delete_items(self):
        self.group.remove()
        
    def port_button_press(self, port_uri, box, event):
        if event.button == 1:
            pb = self.portBoxes[port_uri]
            bounds = pb.get_bounds()
            if self.portData[port_uri].isOutput:
                x = bounds.x2
            elif self.portData[port_uri].isInput:
                x = bounds.x1
            y = (bounds.y1 + bounds.y2) / 2
            self.drag_wire = goocanvas.Path(parent = self.parent, stroke_color_rgba = Colors.frame)
            self.drag_wire.raise_(None)
            self.graph.dragging = (self, port_uri, self.drag_wire, x, y)
            self.update_wire(self.graph.dragging, x, y)
            print "Port URI is " + port_uri
            return True
            
    def dragging(self, tuple, x2, y2):
        boundsGrp = self.group.get_bounds()
        self.update_wire(tuple, x2 + boundsGrp.x1, y2 + boundsGrp.y1)
        
    def dragged(self, tuple, x2, y2):
        # self.update_wire(tuple, x2, y2)
        tuple[2].remove()
        self.graph.dragging = None
        
    def update_wire(self, tuple, x2, y2):
        (x, y, dx, dy) = (tuple[3], tuple[4], x2 - tuple[3], y2 - tuple[4])
        tuple[2].props.data = "M %0.0f,%0.0f C %0.0f,%0.0f %0.0f,%0.0f %0.0f,%0.0f" % (x, y, x+dx/2, y, x+dx/2, y+dy, x+dx, y+dy)

class ConnectionGraphEditor:
    def __init__(self, app):
        self.app = app
        self.moving = None
        self.dragging = None
        pass
        
    def create(self):
        self.create_canvas()
        return self.canvas
        
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
        self.canvas.get_root_item().connect("motion-notify-event", self.canvas_motion_notify)
        self.canvas.get_root_item().connect("button-release-event", self.canvas_button_release)
        
    def add_plugin(self, params):
        (plugin, x, y) = params
        mbox = ModuleBox(self.canvas.get_root_item(), plugin, self)
        bounds = self.canvas.get_bounds()
        if x == None:
            (x, y) = (int(random.uniform(bounds[0], bounds[2] - 100)), int(random.uniform(bounds[1], bounds[3] - 50)))
        mbox.group.translate(x, y)
        mbox.group.connect("button-press-event", self.box_button_press)
        mbox.group.connect("motion-notify-event", self.box_motion_notify)
        mbox.group.connect("button-release-event", self.box_button_release)
        
    def delete_plugin(self, data):
        data.delete_items()

    def canvas_button_press(self, widget, item, event):
        if event.button == 3:
            menu = gtk.Menu()
            for uri in self.app.lv2db.getPluginList():
                plugin = self.app.lv2db.getPluginInfo(uri)
                add_option(menu, plugin.name, self.add_plugin, (plugin, event.x, event.y))
            menu.show_all()
            menu.popup(None, None, None, event.button, event.time)
        
    def canvas_motion_notify(self, group, widget, event):
        if self.dragging != None:
            self.dragging[0].dragging(self.dragging, event.x, event.y)

    def canvas_button_release(self, group, widget, event):
        if self.dragging != None and event.button == 1:
            self.dragging[0].dragged(self.dragging, event.x, event.y)

    def box_button_press(self, group, widget, event):
        if event.button == 1:
            self.moving = group
            self.motion_x = event.x
            self.motion_y = event.y
            return True
        elif event.button == 3:
            menu = gtk.Menu()
            add_option(menu, "Delete", self.delete_plugin, group.module)
            menu.show_all()
            menu.popup(None, None, None, event.button, event.time)
            return True
    
    def box_button_release(self, group, widget, event):
        if event.button == 1:
            self.moving = None
    
    def box_motion_notify(self, group, widget, event):
        if self.moving == group:
            self.moving.translate(event.x - self.motion_x, event.y - self.motion_y)
            
class App:
    def __init__(self):
        fakeserv.start()
        self.lv2db = lv2.LV2DB()
        self.cgraph = ConnectionGraphEditor(self)
        
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
        
    def create_main_menu(self):
        self.menu_bar = gtk.MenuBar()
        
        self.file_menu = add_submenu(self.menu_bar, "_File")
        add_option(self.file_menu, "_Exit", self.exit)
        self.plugin_menu = add_submenu(self.menu_bar, "_Plugins")
        for uri in self.lv2db.getPluginList():
            plugin = self.lv2db.getPluginInfo(uri)
            add_option(self.plugin_menu, plugin.name, self.cgraph.add_plugin, (plugin, None, None))
        
    def exit(self, data):
        self.window.destroy()
        
    def delete_event(self, widget, data = None):
        gtk.main_quit()
    def destroy(self, widget, data = None):
        gtk.main_quit()
        
app = App()
app.create()
gtk.main()
