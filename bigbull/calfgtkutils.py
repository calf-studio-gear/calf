import pygtk
pygtk.require('2.0')
import gtk

def add_option(menu, option, handler, data = None, enabled = True):
    item = gtk.MenuItem(option)
    item.set_sensitive(enabled)
    item.connect_object("activate", handler, data)
    menu.add(item)

def add_submenu(menu, option):
    submenu = gtk.Menu()
    item = gtk.MenuItem(option)
    menu.append(item)
    item.set_submenu(submenu)
    return submenu

