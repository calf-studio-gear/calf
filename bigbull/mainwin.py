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
    activePort = 0x808080FF

class VisibleWire():
    def __init__(self, src, dest, wire):
        """src is source box, dst is destination box, wire is a goocanvas.Path"""
        self.src = src
        self.dest = dest
        self.wire = wire

class ModuleBox():
    def __init__(self, parent, plugin, graph):
        self.width = 200
        self.graph = graph
        self.group = None
        self.connect_candidate = None
        self.parent = parent
        self.plugin = plugin
        self.group = goocanvas.Group(parent = self.parent)
        self.wires = []
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
            box.type = "port"
            box.orig_color = color
            box.object = box.module = self
            box.port = port
            box.uri = port.uri
            box.connect_object("button-press-event", self.port_button_press, port.uri)
            title.connect_object("button-press-event", self.port_button_press, port.uri)
        self.rect = goocanvas.Rect(parent = self.group, width = 100, height = y, line_width = 1, stroke_color_rgba = Colors.frame, fill_color_rgba = Colors.box)
        self.rect.lower(self.title)
        self.rect.type = "module"
        self.rect.object = self.rect.module = self
        self.portBoxes = portBoxes
        self.portTitles = portTitles
        self.group.ensure_updated()
        self.wire = None
        
    def delete_items(self):
        self.group.remove()
        
    def port_endpoint(self, pb):
        bounds = pb.get_bounds()
        port = pb.port
        if port.isOutput:
            x = bounds.x2
        elif port.isInput:
            x = bounds.x1
        y = (bounds.y1 + bounds.y2) / 2
        return (x, y)
        
    def port_button_press(self, port_uri, box, event):
        if event.button == 1:
            pb = self.portBoxes[port_uri]
            pb.props.fill_color_rgba = Colors.activePort
            (x, y) = self.port_endpoint(pb)
            self.drag_wire = goocanvas.Path(parent = self.parent, stroke_color_rgba = Colors.frame)
            self.drag_wire.raise_(None)
            self.graph.dragging = (self, port_uri, self.drag_wire, x, y)
            self.set_connect_candidate(None)
            self.update_drag_wire(self.graph.dragging, x, y)
            print "Port URI is " + port_uri
            return True
            
    def dragging(self, tuple, x2, y2):
        boundsGrp = self.group.get_bounds()
        self.update_drag_wire(tuple, x2 + boundsGrp.x1, y2 + boundsGrp.y1)
        
    def dragged(self, tuple, x2, y2):
        # self.update_drag_wire(tuple, x2, y2)
        pb = self.portBoxes[tuple[1]]
        self.graph.dragging = None
        pb.props.fill_color_rgba = pb.orig_color
        if self.connect_candidate != None:
            print "Connect: " + tuple[1] + " with " + self.connect_candidate.uri
            wire = VisibleWire(pb, self.connect_candidate, tuple[2])
            self.wires.append(wire)
            self.update_wire(wire)
            self.connect_candidate.module.wires.append(VisibleWire(pb, self.connect_candidate, tuple[2]))
            self.set_connect_candidate(None)
        else:
            tuple[2].remove()
        
    def set_connect_candidate(self, box):
        if self.connect_candidate != box:
            if self.connect_candidate != None:
                self.connect_candidate.props.fill_color_rgba = self.connect_candidate.orig_color
            self.connect_candidate = box
        if box != None:
            box.props.fill_color_rgba = Colors.activePort
        
    def update_drag_wire(self, tuple, x2, y2):
        (uri, x, y, dx, dy) = (tuple[1], tuple[3], tuple[4], x2 - tuple[3], y2 - tuple[4])
        tuple[2].props.data = "M %0.0f,%0.0f C %0.0f,%0.0f %0.0f,%0.0f %0.0f,%0.0f" % (x, y, x+dx/2, y, x+dx/2, y+dy, x+dx, y+dy)
        items = self.graph.get_items_at(x+dx, y+dy)
        found = False
        for i in items:
            if 'uri' in i.__dict__:
                if i.module != self and i.port.connectableTo(self.plugin.portDict[uri]):
                    found = True
                    self.set_connect_candidate(i)
        if not found and self.connect_candidate != None:
            self.set_connect_candidate(None)
            
    def update_wires(self):
        for wire in self.wires:
            self.update_wire(wire)
            
    def update_wire(self, wire):
        (x1, y1) = self.port_endpoint(wire.src)
        (x2, y2) = self.port_endpoint(wire.dest)
        xm = (x1 + x2) / 2
        wire.wire.props.data = "M %0.0f,%0.0f C %0.0f,%0.0f %0.0f,%0.0f %0.0f,%0.0f" % (x1, y1, xm, y1, xm, y2, x2, y2)

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
        self.canvas.get_root_item().connect("motion-notify-event", self.canvas_motion_notify)
        self.canvas.get_root_item().connect("button-release-event", self.canvas_button_release)
        
    def get_items_at(self, x, y):
        cr = self.canvas.create_cairo_context()
        items = self.canvas.get_root_item().get_items_at(x, y, cr, True, True)
        return items
        
    def get_data_items_at(self, x, y):
        items = self.get_items_at(x, y)
        if items == None:
            return []
        data_items = []
        for i in items:
            if hasattr(i, 'type'):
                data_items.append((i.type, i.object, i))
        return data_items
        
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
    
    def box_button_release(self, group, widget, event):
        if event.button == 1:
            self.moving = None
    
    def box_motion_notify(self, group, widget, event):
        if self.moving == group:
            self.moving.translate(event.x - self.motion_x, event.y - self.motion_y)
            group.module.update_wires()
                        
class App:
    def __init__(self):
        fakeserv.start()
        self.lv2db = lv2.LV2DB()
        self.cgraph = ConnectionGraphEditor(self)
        
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
        if 'port' in types:
            pass
            #add_option(menu, "-Port-", self.cgraph.delete_plugin, (None, x, y))
        elif 'module' in types:
            for mod in [di for di in items if di[0] == "module"]:
                add_option(menu, "Delete "+mod[1].plugin.name, self.cgraph.delete_plugin, mod[1])            
        else:
            for uri in self.lv2db.getPluginList():
                plugin = self.lv2db.getPluginInfo(uri)
                add_option(menu, plugin.name, self.cgraph.add_plugin, (plugin, x, y))
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
