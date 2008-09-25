import pygtk
pygtk.require('2.0')
import gtk
import cairo
import goocanvas
import random

class Colors:
    frame = 0xC0C0C0FF
    text = 0xFFFFFFFF
    box = 0x202020FF
    defPort = 0x404040FF
    audioPort = 0x000080FF
    controlPort = 0x008000FF
    eventPort = 0x800000FF
    activePort = 0x808080FF
    draggedWire = 0xFFFFFFFF
    connectedWire = 0x808080FF

class VisibleWire():
    def __init__(self, src, dest, wire):
        """src is source ModulePort, dst is destination ModulePort, wire is a goocanvas.Path"""
        self.src = src
        self.dest = dest
        self.wire = wire
    def delete(self):
        self.wire.remove()
        self.src.module.remove_wire(self)
        self.dest.module.remove_wire(self)

class ModulePort():
    fontName = "DejaVu Sans Mono Book 10"
    type = "port"
    def __init__(self, module, portData):
        self.module = module
        self.portData = portData
        self.isInput = self.get_parser().is_port_input(portData)
        self.box = self.title = None
            
    def get_parser(self):
        return self.module.get_parser()
        
    def get_id(self):
        return self.get_parser().get_port_id(self.portData)

    def render(self, ctx, parent, y):
        module = self.module
        (width, margin, spacing) = (module.width, module.margin, module.spacing)
        al = "left"
        portName = self.get_parser().get_port_name(self.portData)
        title = goocanvas.Text(parent = parent, text = portName, font = self.fontName, width = width - 2 * margin, x = margin, y = y, alignment = al, fill_color_rgba = Colors.text, hint_metrics = cairo.HINT_METRICS_ON, pointer_events = False, wrap = False)
        height = 1 + int(title.get_requested_height(ctx, width - 2 * margin))
        title.ensure_updated()
        bnds = title.get_bounds()
        bw = bnds.x2 - bnds.x1 + 2 * margin
        if not self.isInput:
            title.translate(width - bw, 0)
        color = self.get_parser().get_port_color(self.portData)
        if self.isInput:
            box = goocanvas.Rect(parent = parent, x = 0.5, y = y - 1, width = bw, height = height + 2, line_width = 1, fill_color_rgba = color, stroke_color_rgba = Colors.frame)
        else:
            box = goocanvas.Rect(parent = parent, x = width - bw - 0.5, y = y - 1, width = bw, height = height + 2, line_width = 1, fill_color_rgba = color, stroke_color_rgba = Colors.frame)
        box.lower(title)
        y += height + spacing
        box.type = "port"
        self.orig_color = color
        box.object = box.module = self
        box.portData = self.portData
        title.portData = self.portData
        self.box = box
        self.title = title
        return y

class ModuleBox():
    width = 100
    margin = 2
    spacing = 6
    fontName = "DejaVu Sans Mono Book 10"

    def __init__(self, parser, parent, moduleData, graph):
        self.parser = parser
        self.graph = graph
        self.group = None
        self.connect_candidate = None
        self.parent = parent
        self.moduleData = moduleData
        self.group = goocanvas.Group(parent = self.parent)
        self.wires = []
        self.create_items()
        
    def get_parser(self):
        return self.parser

    def create_items(self):
        self.group.module = self
        while self.group.get_n_children() > 0:
            self.group.remove_child(0)
        ctx = self.group.get_canvas().create_cairo_context()
        self.portDict = {}
        moduleName = self.get_parser().get_module_name(self.moduleData)
        self.title = goocanvas.Text(parent = self.group, font = self.fontName, text = "<b>" + moduleName + "</b>", width = self.width, x = 0, y = 0, alignment = "center", use_markup = True, fill_color_rgba = Colors.text, hint_metrics = cairo.HINT_METRICS_ON, antialias = cairo.ANTIALIAS_GRAY)
        y = self.title.get_requested_height(ctx, self.width) + self.spacing
        for (id, portData) in self.get_parser().get_module_port_list(self.moduleData):
            y = self.create_port(ctx, id, portData, y)
        self.rect = goocanvas.Rect(parent = self.group, width = self.width, height = y, line_width = 1, stroke_color_rgba = Colors.frame, fill_color_rgba = Colors.box)
        self.rect.lower(self.title)
        self.rect.type = "module"
        self.rect.object = self.rect.module = self
        self.group.ensure_updated()
        self.wire = None
        
    def create_port(self, ctx, portId, portData, y):
        mport = ModulePort(self, portData)
        y = mport.render(ctx, self.group, y)
        self.portDict[portId] = mport
        mport.box.connect_object("button-press-event", self.port_button_press, mport)
        mport.title.connect_object("button-press-event", self.port_button_press, mport)        
        return y
        
    def delete_items(self):
        self.group.remove()
        
    def port_button_press(self, mport, box, event):
        if event.button == 1:
            port_id = mport.get_id()
            mport.box.props.fill_color_rgba = Colors.activePort
            (x, y) = self.graph.port_endpoint(mport)
            self.drag_wire = goocanvas.Path(parent = self.parent, stroke_color_rgba = Colors.draggedWire)
            self.drag_wire.type = "tmp wire"
            self.drag_wire.object = None
            self.drag_wire.raise_(None)
            self.graph.dragging = (self, port_id, self.drag_wire, x, y)
            self.set_connect_candidate(None)
            self.update_drag_wire(self.graph.dragging, x, y)
            print "Port URI is " + port_id
            return True
            
    def dragging(self, tuple, x2, y2):
        boundsGrp = self.group.get_bounds()
        self.update_drag_wire(tuple, x2 + boundsGrp.x1, y2 + boundsGrp.y1)

    def dragged(self, tuple, x2, y2):
        # self.update_drag_wire(tuple, x2, y2)
        wireitem = tuple[2]
        port = self.portDict[tuple[1]]
        self.graph.dragging = None
        port.box.props.fill_color_rgba = port.orig_color
        if self.connect_candidate != None:
            # print "Connect: " + tuple[1] + " with " + self.connect_candidate.get_id()
            try:
                self.graph.connect(port, self.connect_candidate, wireitem)
            except:
                wireitem.remove()
                raise
            finally:
                self.set_connect_candidate(None)
        else:
            wireitem.remove()
        
    def set_connect_candidate(self, item):
        if self.connect_candidate != item:
            if self.connect_candidate != None:
                self.connect_candidate.box.props.fill_color_rgba = self.connect_candidate.orig_color
            self.connect_candidate = item
        if item != None:
            item.box.props.fill_color_rgba = Colors.activePort
        
    def update_drag_wire(self, tuple, x2, y2):
        (uri, x, y, dx, dy, wireitem) = (tuple[1], tuple[3], tuple[4], x2 - tuple[3], y2 - tuple[4], tuple[2])
        wireitem.props.data = "M %0.0f,%0.0f C %0.0f,%0.0f %0.0f,%0.0f %0.0f,%0.0f" % (x, y, x+dx/2, y, x+dx/2, y+dy, x+dx, y+dy)
        items = self.graph.get_data_items_at(x+dx, y+dy)
        found = False
        for type, obj, item in items:
            if type == 'port':
                if item.module != self and self.get_parser().can_connect(self.portDict[uri].portData, obj.portData):
                    found = True
                    self.set_connect_candidate(obj)
        if not found and self.connect_candidate != None:
            self.set_connect_candidate(None)
            
    def update_wires(self):
        for wire in self.wires:
            self.graph.update_wire(wire)
            
    def remove_wire(self, wire):
        self.wires = [w for w in self.wires if w != wire]

class ConnectionGraphEditor:
    def __init__(self, app, parser):
        self.app = app
        self.parser = parser
        self.moving = None
        self.dragging = None
        self.modules = set()
        pass
        
    def get_parser(self):
        return self.parser

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
        self.canvas.props.integer_layout = False
        self.canvas.update()
        self.canvas.get_root_item().connect("motion-notify-event", self.canvas_motion_notify)
        self.canvas.get_root_item().connect("button-release-event", self.canvas_button_release)
        
    def get_items_at(self, x, y):
        cr = self.canvas.create_cairo_context()
        items = self.canvas.get_items_in_area(goocanvas.Bounds(x - 3, y - 3, x + 3, y + 3), True, True, False)
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

    def get_size(self):
        bounds = self.canvas.get_bounds()
        return (bounds[2] - bounds[0], bounds[3] - bounds[1])

    def add_module_cb(self, params):
        self.add_module(*params)

    def add_module(self, moduleData, x, y):
        mbox = ModuleBox(self.parser, self.canvas.get_root_item(), moduleData, self)
        self.modules.add(mbox)
        bounds = self.canvas.get_bounds()
        if x == None:
            (x, y) = (int(random.uniform(bounds[0], bounds[2] - 100)), int(random.uniform(bounds[1], bounds[3] - 50)))
        mbox.group.translate(x, y)
        mbox.group.connect("button-press-event", self.box_button_press)
        mbox.group.connect("motion-notify-event", self.box_motion_notify)
        mbox.group.connect("button-release-event", self.box_button_release)
        
    def delete_module(self, module):
        self.modules.remove(mbox)
        module.delete_items()
        
    def get_port_map(self):
        map = {}
        for mod in self.modules:
            map.update(mod.portDict)
        return map

    def canvas_motion_notify(self, group, widget, event):
        if self.dragging != None:
            self.dragging[0].dragging(self.dragging, event.x, event.y)

    def canvas_button_release(self, group, widget, event):
        if self.dragging != None and event.button == 1:
            self.dragging[0].dragged(self.dragging, event.x, event.y)

    def box_button_press(self, group, widget, event):
        if event.button == 1:
            group.raise_(None)
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
                        
    def port_endpoint(self, port):
        bounds = port.box.get_bounds()
        port = port.portData
        if self.get_parser().is_port_input(port):
            x = bounds.x1
        else:
            x = bounds.x2
        y = (bounds.y1 + bounds.y2) / 2
        return (x, y)
        
    def update_wire(self, wire):
        (x1, y1) = self.port_endpoint(wire.src)
        (x2, y2) = self.port_endpoint(wire.dest)
        xm = (x1 + x2) / 2
        wire.wire.props.data = "M %0.0f,%0.0f C %0.0f,%0.0f %0.0f,%0.0f %0.0f,%0.0f" % (x1, y1, xm, y1, xm, y2, x2, y2)
        
    def connect(self, p1, p2, wireitem = None):
        # p1, p2 are ModulePort objects
        # if wireitem is set, then this is manual connection, and parser.connect() is called
        # if wireitem is None, then this is automatic connection, and parser.connect() is bypassed
        if p2.isInput:
            (src, dest) = (p1, p2)
        else:
            (dest, src) = (p1, p2)
        if wireitem == None:
            wireitem = goocanvas.Path(parent = self.canvas.get_root_item())
        else:
            self.get_parser().connect(src.portData, dest.portData)
        wire = VisibleWire(src, dest, wireitem)
        wireitem.type = "wire"
        wireitem.object = wire
        wireitem.props.stroke_color_rgba = Colors.connectedWire
        src.module.wires.append(wire)
        dest.module.wires.append(wire)
        self.update_wire(wire)

