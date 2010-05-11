import pygtk
pygtk.require('2.0')
import gtk
import cairo
import pangocairo
import pango
import goocanvas
import random
import math

def calc_extents(ctx, fontName, text):
    layout = pangocairo.CairoContext(ctx).create_layout()
    layout.set_font_description(pango.FontDescription(fontName))
    layout.set_text(text)
    return layout.get_pixel_size()

class Colors:
    frame = 0x808080FF
    text = 0xE0E0E0FF
    box = 0x303030FF
    defPort = 0x404040FF
    audioPort = 0x004060FF
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

def path_move(x, y):
    return "M %s %s" % (x, y)
def path_line(x, y):
    return "L %s %s" % (x, y)

class ModulePort():
    fontName = "DejaVu Sans 9"
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

    def calc_width(self, ctx):
        return calc_extents(ctx, self.fontName, self.get_parser().get_port_name(self.portData))[0] + 4 * self.module.margin + 15

    @staticmethod
    def input_arrow(x, y, w, h):
        return path_move(x, y) + path_line(x + w - 10, y) + path_line(x + w, y + h / 2) + path_line(x + w - 10, y + h) + path_line(x, y + h) + path_line(x, y)

    @staticmethod
    def output_arrow(x, y, w, h):
        return path_move(x + w, y) + path_line(x + 10, y) + path_line(x, y + h / 2) + path_line(x + 10, y + h) + path_line(x + w, y + h) + path_line(x + w, y)

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
        bw += 10
        if self.isInput:
            box = goocanvas.Path(parent = parent, data = self.input_arrow(0.5, y - 0.5, bw, height + 1), line_width = 0, fill_color_rgba = color, stroke_color_rgba = Colors.frame)
        else:
            box = goocanvas.Path(parent = parent, data = self.output_arrow(width - bw, y - 0.5, bw, height + 1), line_width = 0, fill_color_rgba = color, stroke_color_rgba = Colors.frame)
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
    margin = 2
    spacing = 3
    fontName = "DejaVu Sans Bold 9"

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
        self.title = self.get_parser().get_module_name(self.moduleData)
        self.portDict = {}
        width = self.get_title_width(ctx)
        for (id, portData) in self.get_parser().get_module_port_list(self.moduleData):
            mport = self.create_port(id, portData)
            new_width = mport.calc_width(ctx)
            if new_width > width:
                width = new_width
        self.width = width
        y = self.render_title(ctx, 0)
        for (id, portData) in self.get_parser().get_module_port_list(self.moduleData):
            y = self.render_port(ctx, id, y)
        self.rect = goocanvas.Rect(parent = self.group, width = self.width, height = y, line_width = 2, stroke_color_rgba = Colors.frame, fill_color_rgba = Colors.box, antialias = cairo.ANTIALIAS_GRAY)
        self.rect.lower(self.titleItem)
        self.rect.type = "module"
        self.rect.object = self.rect.module = self
        self.group.ensure_updated()
        self.wire = None
        
    def create_port(self, portId, portData):
        mport = ModulePort(self, portData)
        self.portDict[portId] = mport
        return mport
        
    def get_title_width(self, ctx):
        return calc_extents(ctx, self.fontName, self.title)[0] + 4 * self.margin
        
    def render_title(self, ctx, y):
        self.titleItem = goocanvas.Text(parent = self.group, font = self.fontName, text = self.title, width = self.width, x = 0, y = y, alignment = "center", use_markup = True, fill_color_rgba = Colors.text, hint_metrics = cairo.HINT_METRICS_ON, antialias = cairo.ANTIALIAS_GRAY)
        y += self.titleItem.get_requested_height(ctx, self.width) + self.spacing
        return y
        
    def render_port(self, ctx, portId, y):
        mport = self.portDict[portId]
        y = mport.render(ctx, self.group, y)
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
        return mbox
        
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
        
    def seg_distance(self, min1, max1, min2, max2):
        if min1 > min2:
            return self.seg_distance(min2, max2, min1, max1)
        if min2 > max1 + 10:
            return 0
        return min2 - (max1 + 10)
    
    # Squared radius of the circle containing the box
    def box_radius2(self, box):
        return (box.x2 - box.x1) ** 2 + (box.y2 - box.y1) ** 2
    
    def repulsion(self, b1, b2):
        minr2 = (self.box_radius2(b1) + self.box_radius2(b2)) 
        b1x = (b1.x1 + b1.x2) / 2
        b1y = (b1.y1 + b1.y2) / 2
        b2x = (b2.x1 + b2.x2) / 2
        b2y = (b2.y1 + b2.y2) / 2
        r2 = (b2x - b1x) ** 2 + (b2y - b1y) ** 2
        # Not repulsive ;)
        #if r2 > minr2:
        #    return 0
        return (b1x - b2x + 1j * (b1y - b2y)) * 2 * minr2/ (2 * r2**1.5)
        
    def attraction(self, box, wire):
        sign = +1
        if box == wire.src.module:
            sign = -1
        b1 = wire.src.box.get_bounds();
        b2 = wire.dest.box.get_bounds();
        k = 0.003
        if b1.x2 > b2.x1 - 40:
            return k * 8 * sign * (b1.x2 - (b2.x1 - 40) + 1j * (b1.y1 - b2.y1))
        return k * sign * (b1.x2 - b2.x1 + 1j * (b1.y1 - b2.y1))
        
    def blow_up(self):
        return
        for m in self.modules:
            m.velocity = 0+0j
            m.bounds = m.group.get_bounds()
        damping = 0.5
        step = 2.0
        cr = self.canvas.create_cairo_context()
        w, h = self.canvas.allocation.width, self.canvas.allocation.height
        temperature = 100
        while True:
            energy = 0.0
            x = y = 0
            for m1 in self.modules:
                x += (m1.bounds.x1 + m1.bounds.x2) / 2
                y += (m1.bounds.y1 + m1.bounds.y2) / 2
            x /= len(self.modules)
            y /= len(self.modules)
            gforce = w / 2 - x + 1j * (h / 2 - y)
            for m1 in self.modules:
                force = gforce
                force += temperature * random.random()
                for m2 in self.modules:
                    if m1 == m2:
                        continue
                    force += self.repulsion(m1.bounds, m2.bounds)
                for wi in m1.wires:
                    force += self.attraction(m1, wi) 
                if m1.bounds.x1 < 10: force -= m1.bounds.x1 - 10
                if m1.bounds.y1 < 10: force -= m1.bounds.y1 * 1j - 10j
                if m1.bounds.x2 > w: force -= (m1.bounds.x2 - w)
                if m1.bounds.y2 > h: force -= (m1.bounds.y2 - h) * 1j
                m1.velocity = (m1.velocity + force) * damping
                energy += abs(m1.velocity) ** 2
            for m1 in self.modules:
                print "Velocity is (%s, %s)" % (m1.velocity.real, m1.velocity.imag)
                m1.group.translate(step * m1.velocity.real, step * m1.velocity.imag)
                m1.group.update(True, cr)
                #m1.group.update(True, cr, m1.bounds)
                m1.update_wires()
            damping *= 0.99
            temperature *= 0.99
            self.canvas.draw(gtk.gdk.Rectangle(0, 0, w, h))
            print "Energy is %s" % energy
            if energy < 0.1:
                break
                
