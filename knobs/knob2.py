#!/usr/bin/env python

import cairo
from math import pi, cos, sin

WIDTH, HEIGHT = 40, 40
x, y = WIDTH / 2, HEIGHT / 2
lwidth = WIDTH / 10
radius = WIDTH / 2 - lwidth
radiusplus = radius + lwidth / 2
radiusminus = radius - lwidth / 2
radiusminus2 = radius - lwidth
radiusminus3 = radius - lwidth * 3 / 2
radiusint = (radius - lwidth / 2) * 0.25
value = 0.7
arrow = WIDTH / 10
phases = 65

# Setup Cairo
surface = cairo.ImageSurface(cairo.FORMAT_ARGB32, phases * WIDTH, HEIGHT * 3)
ctx = cairo.Context(surface)
ctx.set_source_rgba(0.75, 0.75, 0.75, 0)
ctx.rectangle(0, 0, phases * WIDTH, 3 * HEIGHT)
ctx.fill()

for variant in range(0, 3):
    x = WIDTH / 2
    y = HEIGHT * (variant + 0.5)
    for phase in range(0, phases):
        # Draw out the triangle using absolute coordinates
        value = phase * 1.0 / (phases - 1)
        sangle = (180-45)*pi/180
        eangle = (360+45)*pi/180
        vangle = sangle + value * (eangle - sangle)
        c, s = cos(vangle), sin(vangle)

        nleds = 31
        midled = (nleds - 1) / 2
        midphase = (phases - 1) / 2
        spacing = pi / nleds
        for led in range(0, nleds):
            adelta = (eangle - sangle - spacing) / (nleds - 1)
            lit = False
            hilite = False
            lvalue = led * 1.0 / (nleds - 1)
            pvalue = phase * 1.0 / (phases - 1)
            if variant == 0: lit = (pvalue == 1.0) or pvalue > lvalue
            if variant == 1: 
                if pvalue < 0.5:
                    lit = (lvalue > pvalue or pvalue == 0.0) and lvalue <= 0.5
                else:
                    lit = (lvalue < pvalue or pvalue == 1.0) and lvalue >= 0.5
                if led == midled:
                    lit = (phase == midphase)
                    hilite = True
            if variant == 2: lit = pvalue == 0 or pvalue < lvalue
            if not lit:
                ctx.set_source_rgb(0, 0, 0)
            else:
                if hilite:
                    ctx.set_source_rgb(1, 1, 0)
                else:
                    ctx.set_source_rgb(1, 0.5, 0)
            ctx.set_line_width(3)
            ctx.arc(x, y, radius, sangle + adelta * led, sangle + adelta * led + spacing)
            ctx.stroke()

        #ctx.set_line_width(lwidth)
        #ctx.set_source_rgb(1, 0.5, 0)
        #ctx.arc(x, y, radius, sangle, vangle)
        #ctx.line_to(x + radiusint * c, y + radiusint * s)
        #ctx.stroke()

        grad = cairo.LinearGradient(x - radius / 2, y - radius / 2, x + radius / 2, y + radius / 2)
        #grad.add_color_stop_rgb(0.0, 0.5, 0.5, 0.5)
        #grad.add_color_stop_rgb(1.0, 0.8, 0.8, 0.8)
        grad.add_color_stop_rgb(0.0, 0.5, 0.5, 0.5)
        grad.add_color_stop_rgb(1.0, 0.8, 0.8, 0.8)
        ctx.set_source(grad)
        # ctx.set_source_rgb(0.8, 0.8, 0.8)
        ctx.set_line_width(2)
        ctx.arc(x, y, radiusminus2, 0, 2 * pi)
        ctx.fill()

        grad = cairo.LinearGradient(x - radius / 2, y - radius / 2, x + radius / 2, y + radius / 2)
        grad.add_color_stop_rgb(0.0, 0.8, 0.8, 0.8)
        grad.add_color_stop_rgb(1.0, 0.5, 0.5, 0.5)
        ctx.set_source(grad)
        ctx.arc(x, y, radiusminus3, 0, 2 * pi)
        ctx.fill()
        ctx.set_source_rgb(0, 0, 0)
        ctx.set_line_width(2)
        ctx.arc(x, y, radiusminus2, 0, 2 * pi)
        ctx.stroke()

        ctx.set_source_rgba(0, 0, 0, 0.5)
        ctx.set_line_width(1)
        mtx = ctx.get_matrix()
        ctx.translate(x + radiusminus2 * c, y + radiusminus2 * s)
        ctx.rotate(vangle)
        ctx.move_to(0, 0)
        ctx.line_to(-radius/2, 0)
        ctx.stroke()
        ctx.set_matrix(mtx)
        x += WIDTH

#ctx.set_source_rgb(1, 0.5, 0)
#ctx.line_to(x + radiusplus * c, y + radiusplus * s)
#ctx.stroke()

# Output a PNG file
surface.write_to_png("knob.png")
