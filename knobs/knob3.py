#!/usr/bin/env python

import cairo
from math import pi, cos, sin

WIDTH, HEIGHT = 60, 60
background = "knob3_bg.png"
output = "knob3.png"
x, y = WIDTH / 2, HEIGHT / 2
lwidth = WIDTH / 12
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
surface = cairo.ImageSurface(cairo.FORMAT_ARGB32, phases * WIDTH, HEIGHT * 4)
ctx = cairo.Context(surface)
ctx.set_source_rgba(0.75, 0.75, 0.75, 0)
ctx.rectangle(0, 0, phases * WIDTH, 4 * HEIGHT)
ctx.fill()

for variant in range(0, 4):
    x = WIDTH / 2
    y = HEIGHT * (variant + 0.5)
    for phase in range(0, phases):
        # Draw background image
        bgimage = cairo.ImageSurface.create_from_png(background) 
        ctx.set_source_surface(bgimage, x - WIDTH / 2, y - HEIGHT / 2);
        ctx.rectangle(phase * WIDTH, variant * HEIGHT, WIDTH, HEIGHT)
        ctx.fill ();

        # Draw out the triangle using absolute coordinates
        value = phase * 1.0 / (phases - 1)
        if variant != 3:
            sangle = (180 - 45)*pi/180
            eangle = (360 + 45)*pi/180
            nleds = 31
        else:
            sangle = (270)*pi/180
            eangle = (270 + 360)*pi/180
            nleds = 32
        vangle = sangle + value * (eangle - sangle)
        c, s = cos(vangle), sin(vangle)

        midled = (nleds - 1) / 2
        midphase = (phases - 1) / 2
        thresholdP = midled + 1 + ((phase - midphase - 1) * 1.0 * (nleds - midled - 2) / (phases - midphase - 2))
        thresholdN = midled - 1 - ((midphase - 1 - phase) * 1.0 * (nleds - midled - 2) / (midphase - 1))
        
        spacing = pi / nleds
        for led in range(0, nleds):
            if variant == 3:
                adelta = (eangle - sangle) / (nleds)
            else:
                adelta = (eangle - sangle - spacing) / (nleds - 1)
            lit = False
            glowlit = False
            glowval = 0.5
            hilite = False
            lvalue = led * 1.0 / (nleds - 1)
            pvalue = phase * 1.0 / (phases - 1)
            if variant == 3: 
                # XXXKF works only for phases = 2 * leds
                exled = phase / 2.0
                lit = led == exled or (phase == phases - 1 and led == 0)
                glowlit = led == (exled + 0.5) or led == (exled - 0.5)
                glowval = 0.8
                hilite = (phase % ((phases - 1) / 4)) == 0
            if variant == 0: lit = (pvalue == 1.0) or pvalue > lvalue
            if variant == 1: 
                if led == midled:
                    lit = (phase == midphase)
                    #glowlit = (phase < midphase and thresholdN >= midled - 1) or (phase > midphase and thresholdP <= midled + 1)
                    glowlit = False
                    hilite = True
                elif led > midled and phase > midphase:
                    # led = [midled + 1, nleds - 1]
                    # phase = [midphase + 1, phases - 1]
                    lit = led <= thresholdP
                    glowlit = led <= thresholdP + 1
                    glowval = 0.4
                elif led < midled and phase < midphase:
                    lit = led >= thresholdN
                    glowlit = led >= thresholdN - 1
                    glowval = 0.4
                else:
                    lit = False
            if variant == 2: lit = pvalue == 0 or pvalue < lvalue
            if not lit:
                if not glowlit:
                    ctx.set_source_rgb(0, 0.1, 0.2)
                else:
                    ctx.set_source_rgb(0 * glowval, 0.5 * glowval, 1 * glowval)
            else:
                if hilite:
                    ctx.set_source_rgb(0, 1, 1)
                else:
                    ctx.set_source_rgb(0, 0.5, 1)
            ctx.set_line_width(3)
            if hilite:
                ctx.set_line_width(4)
            ctx.arc(x, y, radius, sangle + adelta * led, sangle + adelta * led + spacing)
            ctx.stroke()

        ctx.set_source_rgba(1, 1, 1, 1)
        ctx.set_line_width(2)
        mtx = ctx.get_matrix()
        ctx.translate(x + radiusminus2 * c, y + radiusminus2 * s)
        ctx.rotate(vangle)
        ctx.move_to(0, 0)
        ctx.line_to(-radius/5, 0)
        ctx.stroke()
        ctx.set_matrix(mtx)
        x += WIDTH

# Output a PNG file
surface.write_to_png(output)
