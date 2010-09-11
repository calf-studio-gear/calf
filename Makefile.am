SRCDIRS = src gui icons

SUBDIRS = $(SRCDIRS)

distdir = $(PACKAGE)-$(VERSION)

desktopfilesdir = $(datadir)/applications
dist_desktopfiles_DATA = calf.desktop

EXTRA_DIST = COPYING.GPL TODO autogen.sh calf.glade presets.xml calf.7 calfjackhost.1 calf-gui.xml

dist_man_MANS = calf.7 calfjackhost.1

clean-local:
	rm -f *~ *.old *.bak
	rm -rf autom4te.cache

install-data-local:
	GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $(top_srcdir)/calf.schemas
        
uninstall-local:
	GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-uninstall-rule $(top_srcdir)/calf.schemas
        
