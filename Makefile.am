SRCDIRS = src gui icons

SUBDIRS = $(SRCDIRS)

distdir = $(PACKAGE)-$(VERSION)

desktopfilesdir = $(datadir)/applications
dist_desktopfiles_DATA = calf.desktop

docdir = $(datadir)/doc/${PACKAGE}

EXTRA_DIST = COPYING.GPL TODO autogen.sh presets.xml calf.7 calfjackhost.1 calf-gui.xml doc/manuals $(wildcard sf2/*.sf2)

dist_man_MANS = calf.7 calfjackhost.1

if ENABLE_BASH_COMPLETION
bashcompletiondir = $(BASH_COMPLETION_DIR)
dist_bashcompletion_DATA = calf
endif

AM_DISTCHECK_CONFIGURE_FLAGS = --with-bash-completion-dir=no --without-obsolete-check

clean-local:
	rm -f *~ *.old *.bak
	rm -rf autom4te.cache

install-data-local:
	install -d -m 755 $(DESTDIR)$(docdir)
	install -d -m 755 $(DESTDIR)$(docdir)/images
	install -d -m 755 $(DESTDIR)$(docdir)/images/icons
	install -d -m 755 $(DESTDIR)$(docdir)/images/prettyPhoto
	install -d -m 755 $(DESTDIR)$(docdir)/images/prettyPhoto/dark_rounded
	install -d -m 755 $(DESTDIR)$(docdir)/scripts
	install -d -m 755 $(DESTDIR)$(pkgdatadir)/sf2
	install -c -m 644 $(top_srcdir)/doc/manuals/*.html $(DESTDIR)$(docdir)
	install -c -m 644 $(top_srcdir)/doc/manuals/images/*.png $(DESTDIR)$(docdir)/images/
	install -c -m 644 $(top_srcdir)/doc/manuals/images/*.jpg $(DESTDIR)$(docdir)/images/
	install -c -m 644 $(top_srcdir)/doc/manuals/images/prettyPhoto/dark_rounded/* $(DESTDIR)$(docdir)/images/prettyPhoto/dark_rounded/
	install -c -m 644 $(top_srcdir)/doc/manuals/images/icons/* $(DESTDIR)$(docdir)/images/icons/
	install -c -m 644 $(top_srcdir)/doc/manuals/scripts/*.css $(DESTDIR)$(docdir)/scripts/
	install -c -m 644 $(top_srcdir)/doc/manuals/scripts/*.js $(DESTDIR)$(docdir)/scripts/
	install -c -m 644 $(top_srcdir)/sf2/*.sf2 $(DESTDIR)$(pkgdatadir)/sf2/
	    
uninstall-local:
	rm -f $(DESTDIR)$(docdir)/*.html
	rm -f $(DESTDIR)$(docdir)/images/*.png
	rm -f $(DESTDIR)$(docdir)/images/*.jpg
	rm -f $(DESTDIR)$(docdir)/images/icons/*.png
	rm -f $(DESTDIR)$(docdir)/images/prettyPhoto/dark_rounded/*
	rm -f $(DESTDIR)$(docdir)/scripts/*.css
	rm -f $(DESTDIR)$(docdir)/scripts/*.js
	rm -f $(DESTDIR)$(pkgdatadir)/sf2/*.sf2
	rmdir $(DESTDIR)$(docdir)/scripts $(DESTDIR)$(docdir)/images/icons $(DESTDIR)$(docdir)/images/prettyPhoto/dark_rounded $(DESTDIR)$(docdir)/images/prettyPhoto $(DESTDIR)$(docdir)/images $(DESTDIR)$(docdir) $(DESTDIR)$(pkgdatadir)/sf2 || true
