SRCDIRS = src gui icons

SUBDIRS = $(SRCDIRS)

distdir = $(PACKAGE)-$(VERSION)

desktopfilesdir = $(datadir)/applications
dist_desktopfiles_DATA = calf.desktop

EXTRA_DIST = COPYING.GPL TODO autogen.sh presets.xml calf.7 calfjackhost.1 calf-gui.xml

dist_man_MANS = calf.7 calfjackhost.1

clean-local:
	rm -f *~ *.old *.bak
	rm -rf autom4te.cache

install-data-local:
        
uninstall-local:
