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

class PortPalette(object):
    def __init__(self, stroke, fill, activated_palette = None):
        self.stroke = stroke
        self.fill = fill
        self.activated_palette = activated_palette
    def get_stroke(self, port):
        if self.activated_palette is not None and port.is_dragged():
            return self.activated_palette.get_stroke(port)
        return self.stroke
    def get_fill(self, port):
        if self.activated_palette is not None and port.is_dragged():
            return self.activated_palette.get_fill(port)
        return self.fill

class Colors:
    frame = 0x4C4C4CFF
    text = 0xE0E0E0FF
    box = 0x242424FF
    defPort = 0x404040FF
    activePort = PortPalette(0xF0F0F0FF, 0x808080FF)
    audioPort = PortPalette(0x204A87FF, 0x183868FF, activePort)
    controlPort = PortPalette(0x008000FF, 0x00800080, activePort)
    eventPort = PortPalette(0xA40000FF, 0x7C000080, activePort)
    draggedWire = 0xFFFFFFFF
    connectedWire = 0x808080FF

def path_move(x, y):
    return "M %s %s " % (x, y)
    
def path_line(x, y):
    return "L %s %s " % (x, y)

def path_curve(x1, y1, x2, y2, x3, y3):
    return "C %0.0f,%0.0f %0.0f,%0.0f %0.0f,%0.0f" % (x1, y1, x2, y2, x3, y3)

def wireData(x1, y1, x2, y2):
    dist = (x2 - x1) / 2
    if dist < 30:
        dist = 30 + (30 - dist) / 2
        if dist > 60:
            dist = 60
    return path_move(x1, y1) + path_curve(x1 + dist, y1, x2 - dist, y2, x2, y2)

class VisibleWire():
    def __init__(self, src, dest):
        """src is source PortView, dst is destination PortView"""
        self.src = src
        self.dest = dest
        self.mask = goocanvas.Path(parent = src.get_graph().get_root(), line_width=12, stroke_color_rgba = 0, pointer_events = goocanvas.EVENTS_ALL)
        self.mask.type = "wire"
        self.mask.object = self
        self.wire = goocanvas.Path(parent = src.get_graph().get_root(), stroke_color_rgba = Colors.connectedWire, pointer_events = 0)
        self.wire.type = "wirecore"
        self.wire.object = self
        self.update_shape()
    
    def remove(self):
        if self.wire is not None:
            self.wire.remove()
            self.mask.remove()
            self.src.module.wires.remove(self)
            self.dest.module.wires.remove(self)
            self.wire = None
            self.mask = None

    def update_shape(self):
        (x1, y1) = self.src.get_endpoint()
        (x2, y2) = self.dest.get_endpoint()
        data = wireData(x1, y1, x2, y2)
        self.wire.props.data = data
        self.mask.props.data = data
        

class Dragging():
    def __init__(self, port_view, x, y):
        self.module = port_view.module
        self.port_view = port_view
        self.x = x
        self.y = y
        self.drag_wire = goocanvas.Path(parent = self.get_graph().get_root(), stroke_color_rgba = Colors.draggedWire)
        self.drag_wire.type = "tmp wire"
        self.drag_wire.object = None
        self.drag_wire.raise_(None)
        self.connect_candidate = None
        self.update_drag_wire(x, y)
        
    def get_graph(self):
        return self.module.graph
        
    def update_shape(self, x2, y2):
        if self.port_view.isInput:
            self.drag_wire.props.data = wireData(x2, y2, self.x, self.y)
        else:
            self.drag_wire.props.data = wireData(self.x, self.y, x2, y2)
        
    def dragging(self, x2, y2):
        self.update_drag_wire(x2, y2)

    def update_drag_wire(self, x2, y2):
        items = self.get_graph().get_data_items_at(x2, y2)
        found = None
        for type, obj, item in items:
            if type == 'port':
                if item.module != self and self.get_graph().get_controller().can_connect(self.port_view.model, obj.model):
                    found = obj
        self.set_connect_candidate(found)
        if found is not None:
            x2, y2 = found.get_endpoint()
        self.update_shape(x2, y2)

    def has_port_view(self, port_view):
        if port_view == self.port_view:
            return True
        if port_view == self.connect_candidate:
            return True
        return False

    def set_connect_candidate(self, item):
        if self.connect_candidate != item:
            old = self.connect_candidate
            self.connect_candidate = item
            if item is not None:
                item.update_style()
            if old is not None:
                old.update_style()

    def end_drag(self, x2, y2):
        # self.update_drag_wire(tuple, x2, y2)
        src, dst = self.port_view, self.connect_candidate
        self.get_graph().dragging = None
        src.update_style()
        self.drag_wire.remove()
        self.drag_wire = None
        if dst is not None:
            # print "Connect: " + tuple[1] + " with " + self.connect_candidate.get_id()
            dst.update_style()
            if src.isInput:
                src, dst = dst, src
            self.get_graph().get_controller().connect(src.model, dst.model)
    
class PortView():
    fontName = "DejaVu Sans 11px"
    type = "port"
    def __init__(self, module, model):
        self.module = module
        self.model = model
        self.isInput = model.is_port_input()
        self.box = self.title = None

    def get_graph(self):
        return self.module.graph

    def get_controller(self):
        return self.module.get_controller()
        
    def get_id(self):
        return self.model.get_id()

    def calc_width(self, ctx):
        return calc_extents(ctx, self.fontName, self.model.get_name())[0] + 4 * self.module.margin + 15

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
        portName = self.model.get_name()
        title = goocanvas.Text(parent = parent, text = portName, font = self.fontName, width = width - 2 * margin, x = margin, y = y, alignment = al, fill_color_rgba = Colors.text, hint_metrics = cairo.HINT_METRICS_ON, pointer_events = False, wrap = False)
        height = 1 + int(title.get_requested_height(ctx, width - 2 * margin))
        title.ensure_updated()
        bnds = title.get_bounds()
        bw = bnds.x2 - bnds.x1 + 2 * margin
        if not self.isInput:
            title.translate(width - bw - 2, 0)
        else:
            title.translate(2, 0)
        bw += 10
        if self.isInput:
            box = goocanvas.Path(parent = parent, data = self.input_arrow(0.5, y - 0.5, bw, height + 1))
        else:
            box = goocanvas.Path(parent = parent, data = self.output_arrow(width - bw, y - 0.5, bw, height + 1))
        box.lower(title)
        y += height + spacing
        box.type = "port"
        box.object = box.module = self
        box.model = self.model
        title.model = self.model
        self.box = box
        self.title = title
        self.update_style()
        return y
    
    def update_style(self):
        color = self.model.get_port_color()
        self.box.set_properties(fill_color_rgba = color.get_fill(self), stroke_color_rgba = color.get_stroke(self), line_width = 1, pointer_events = goocanvas.EVENTS_ALL)

    def is_dragged(self):
        dragging = self.get_graph().dragging
        if dragging is not None and dragging.has_port_view(self):
            return True
        return False
        
    def get_endpoint(self):
        bounds = self.box.get_bounds()
        if self.isInput:
            x = bounds.x1
        else:
            x = bounds.x2
        y = (bounds.y1 + bounds.y2) / 2
        return (x, y)
        
    def get_connections(self):
        return [wire for wire in self.module.wires if wire.src == self or wire.dest == self]
        
class ModuleView():
    margin = 2
    spacing = 4
    fontName = "DejaVu Sans Bold 9"

    def __init__(self, controller, parent, model, graph):
        self.controller = controller
        self.graph = graph
        self.group = None
        self.connect_candidate = None
        self.parent = parent
        self.model = model
        self.group = goocanvas.Group(parent = self.parent, pointer_events = goocanvas.EVENTS_ALL)
        self.group.type = "module"
        self.group.object = self.group.module = self
        self.group.raise_(None)
        self.rect = None
        self.titleItem = None
        self.ports = []
        self.wires = []
        self.refresh()
        
    def get_controller(self):
        return self.controller

    def refresh(self):
        self.title = self.model.get_name()
        self.ports = []
        self.portDict = {}
        for model in self.model.get_port_list():
            self.add_port(model)
        self.render()
    
    def remove(self):
        while self.group.get_n_children() > 0:
            self.group.remove_child(0)
        self.rect = None
        self.titleItem = None
        
    def render(self):
        self.remove()
        ctx = self.group.get_canvas().create_cairo_context()
        self.width = max([self.get_title_width(ctx)] + [port_view.calc_width(ctx) for port_view in self.ports])
        y = self.render_title(ctx, 0.5)
        for port_view in self.ports:
            y = port_view.render(ctx, self.group, y)
        self.rect = goocanvas.Rect(parent = self.group, x = 0.5, width = self.width, height = y)
        self.update_style()
        self.rect.lower(self.titleItem)
        self.rect.type = "module"
        self.rect.object = self.group.module = self
        self.group.ensure_updated()
        
    def add_port(self, model, pos = None):
        if pos is None:
            pos = len(self.ports)
        view = PortView(self, model)
        self.ports.insert(pos, view)
        self.portDict[model.get_id()] = view
        return view
        
    def del_port(self, pos):
        del self.portDict[self.ports[pos].model.get_id()]
        self.ports.pop(pos)
        
    def get_title_width(self, ctx):
        return calc_extents(ctx, self.fontName, self.title)[0] + 4 * self.margin
        
    def render_title(self, ctx, y):
        self.titleItem = goocanvas.Text(parent = self.group, font = self.fontName, text = self.title, width = self.width, x = 0, y = y, alignment = "center", use_markup = True, fill_color_rgba = Colors.text, hint_metrics = cairo.HINT_METRICS_ON, antialias = cairo.ANTIALIAS_GRAY, pointer_events = goocanvas.EVENTS_NONE)
        y += self.titleItem.get_requested_height(ctx, self.width) + 2 * self.spacing
        return y
        
    def render_port(self, ctx, port_view, y):
        return port_view.render(ctx, self.group, y)
        #port_view.box.connect_object("button-press-event", self.port_button_press, port_view)
        #port_view.title.connect_object("button-press-event", self.port_button_press, port_view)        
        
    def delete_items(self):
        self.group.remove()
        for w in list(self.wires):
            w.remove()
            
    def update_wires(self):
        for wire in self.wires:
            wire.update_shape()
            
    def translate(self, dx, dy):
        self.group.translate(dx, dy)
        self.group.ensure_updated()
        self.update_wires()
        
    def update_style(self):
        self.rect.set_properties(line_width = 1, stroke_color_rgba = Colors.frame, fill_color_rgba = Colors.box, antialias = cairo.ANTIALIAS_GRAY, pointer_events = goocanvas.EVENTS_ALL)
        
class ConnectionGraphEditor:
    def __init__(self, app, controller):
        self.app = app
        self.controller = controller
        self.moving = None
        self.dragging = None
        self.modules = set()
        pass
        
    def get_controller(self):
        return self.controller

    def create(self, sx, sy):
        self.create_canvas(sx, sy)
        return self.canvas
        
    def create_canvas(self, sx, sy):
        self.canvas = goocanvas.Canvas()
        self.canvas.props.automatic_bounds = True
        self.canvas.set_size_request(sx, sy)
        self.canvas.set_scale(1)
        #self.canvas.connect("size-allocate", self.update_canvas_bounds)
        self.canvas.props.background_color_rgb = 0
        self.canvas.props.integer_layout = False
        self.canvas.update()
        self.canvas.connect("button-press-event", self.canvas_button_press_handler)
        self.canvas.connect("motion-notify-event", self.canvas_motion_notify)
        self.canvas.connect("button-release-event", self.canvas_button_release)
    
    def get_canvas(self):
        return self.canvas
    
    def get_root(self):
        return self.canvas.get_root_item()
    
    def get_items_at(self, x, y):
        return self.canvas.get_items_at(x, y, True)
        
    def get_module_map(self, model):
        map = {}
        for view in self.modules:
            map[view.model] = view
        return map
        
    def get_module_view(self, model):
        for view in self.modules:
            if view.model == model:
                return view
        return None
        
    def get_port_map(self):
        map = {}
        for mod in self.modules:
            map.update(mod.portDict)
        return map
        
    def get_port_view(self, port_model):
        for mod in self.modules:
            for p in mod.ports:
                if p.model == port_model:
                    return p
        return None

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

    def add_module(self, model, x, y):
        mbox = ModuleView(self.controller, self.canvas.get_root_item(), model, self)
        self.modules.add(mbox)
        bounds = self.canvas.get_bounds()
        if x == None:
            (x, y) = (int(random.uniform(bounds[0], bounds[2] - 100)), int(random.uniform(bounds[1], bounds[3] - 50)))
        mbox.group.translate(x, y)
        return mbox
        
    def delete_module(self, module):
        self.modules.remove(mbox)
        module.delete_items()
        
    def canvas_button_press_handler(self, widget, event):
        if event.button == 1:
            for itype, iobject, item in self.get_data_items_at(event.x, event.y):
                if itype == 'module':
                    group = iobject.group
                    group.raise_(None)
                    for w in group.module.wires:
                        w.wire.raise_(None)
                        
                    self.moving = group.module
                    self.motion_x = event.x
                    self.motion_y = event.y
                    return True
                if itype == 'port':
                    port_view = iobject
                    module = port_view.module
                    (x, y) = port_view.get_endpoint()
                    self.dragging = Dragging(port_view, x, y)
                    port_view.update_style()
                    print "Port URI is " + port_view.get_id()
                    return True
        elif event.button == 3:
            self.app.canvas_popup_menu(event.x, event.y, event.time)
            return True
        
    def canvas_motion_notify(self, widget, event):
        if self.dragging is not None:
            self.dragging.dragging(event.x, event.y)
        if self.moving is not None:
            self.moving.translate(event.x - self.motion_x, event.y - self.motion_y)
            self.moving.update_wires()
            self.motion_x, self.motion_y = event.x, event.y

    def canvas_button_release(self, widget, event):
        if event.button == 1:
            if self.moving is not None:
                self.moving = None
            if self.dragging is not None:
                self.dragging.end_drag(event.x, event.y)

    # Connect elements visually (but not logically, that's what controller is for)
    def connect(self, src, dest):
        wire = VisibleWire(src, dest)
        src.module.wires.append(wire)
        dest.module.wires.append(wire)
        return wire
        
    def disconnect(self, src, dest):
        for w in src.module.wires:
            if w.dest == dest:
                w.remove()
                return True
        return False
        
    def clear(self):
        for m in self.modules:
            m.delete_items()
        self.modules.clear()
        self.moving = None
        self.dragging = None
        
    def blow_up(self):
        GraphDetonator().blow_up(self)
        
# I broke this at some point, and not in mood to fix it now
class GraphDetonator:
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
        
    def blow_up(self, graph):
        modules = graph.modules
        canvas = graph.canvas
        for m in modules:
            m.velocity = 0+0j
            m.bounds = m.group.get_bounds()
        damping = 0.5
        step = 2.0
        cr = canvas.create_cairo_context()
        w, h = canvas.allocation.width, graph.canvas.allocation.height
        temperature = 100
        while True:
            energy = 0.0
            x = y = 0
            for m1 in modules:
                x += (m1.bounds.x1 + m1.bounds.x2) / 2
                y += (m1.bounds.y1 + m1.bounds.y2) / 2
            x /= len(modules)
            y /= len(modules)
            gforce = w / 2 - x + 1j * (h / 2 - y)
            for m1 in modules:
                force = gforce
                force += temperature * random.random()
                for m2 in modules:
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
            for m1 in modules:
                print "Velocity is (%s, %s)" % (m1.velocity.real, m1.velocity.imag)
                m1.group.translate(step * m1.velocity.real, step * m1.velocity.imag)
                m1.group.ensure_updated()
                #m1.group.update(True, cr, m1.bounds)
                m1.update_wires()
            damping *= 0.99
            temperature *= 0.99
            canvas.draw(gtk.gdk.Rectangle(0, 0, w, h))
            print "Energy is %s" % energy
            if energy < 0.1:
                break
                
