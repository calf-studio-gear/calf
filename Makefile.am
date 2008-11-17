SRCDIRS = src gui

SUBDIRS = $(SRCDIRS)

distdir = $(PACKAGE)-$(VERSION)

desktopfilesdir = $(datadir)/applications
dist_desktopfiles_DATA = calf.desktop

EXTRA_DIST = COPYING.GPL TODO autogen.sh calf.glade presets.xml knob.png calf.7 calfjackhost.1

dist_man_MANS = calf.7 calfjackhost.1

clean-local:
	rm -f *~ *.old *.bak
	rm -rf autom4te.cache
