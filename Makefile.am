SRCDIRS = src gui icons

SUBDIRS = $(SRCDIRS)

distdir = $(PACKAGE)-$(VERSION)

desktopfilesdir = $(datadir)/applications
dist_desktopfiles_DATA = calf.desktop

docdir = $(datadir)/doc/${PACKAGE}

EXTRA_DIST = COPYING.GPL TODO autogen.sh presets.xml calf.7 calfjackhost.1 calf-gui.xml doc/manuals

dist_man_MANS = calf.7 calfjackhost.1

clean-local:
	rm -f *~ *.old *.bak
	rm -rf autom4te.cache

install-data-local:
	install -d -m 755 $(DESTDIR)$(docdir)
	install -d -m 755 $(DESTDIR)$(docdir)/images
	install -d -m 755 $(DESTDIR)$(docdir)/scripts
	install -c -m 644 $(top_srcdir)/doc/manuals/*.html $(DESTDIR)$(docdir)
	install -c -m 644 $(top_srcdir)/doc/manuals/images/*.png $(DESTDIR)$(docdir)/images/
	install -c -m 644 $(top_srcdir)/gui/background_dark.png $(DESTDIR)$(docdir)/images/
	install -c -m 644 $(top_srcdir)/doc/manuals/scripts/*.css $(DESTDIR)$(docdir)/scripts/
	install -c -m 644 $(top_srcdir)/doc/manuals/scripts/*.js $(DESTDIR)$(docdir)/scripts/
        
uninstall-local:
	rm -f $(DESTDIR)$(docdir)/*.html
	rm -f $(DESTDIR)$(docdir)/images/*.png
	rm -f $(DESTDIR)$(docdir)/scripts/*.css
	rm -f $(DESTDIR)$(docdir)/scripts/*.js
	rmdir $(DESTDIR)$(docdir)/scripts $(DESTDIR)$(docdir)/images $(DESTDIR)$(docdir) || true
