SRCDIRS = icons/LV2

SUBDIRS = $(SRCDIRS)

EXTRA_DIST = $(wildcard gui-*.xml) $(wildcard *.png) calf.rc.in

install-data-hook:
	install -c -m 644 $(srcdir)/*.png $(DESTDIR)$(pkgdatadir)
	$(top_builddir)/src/calfmakerdf -m gui -p $(DESTDIR)$(pkgdatadir)
	install -c -m 644 $(srcdir)/gui-*.xml $(DESTDIR)$(pkgdatadir)
	sed 's#@guidatadir@#$(pkgdatadir)#g' $(srcdir)/calf.rc.in >$(top_builddir)/calf.rc
	sed 's#@guidatadir@#$(pkgdatadir)#g' $(srcdir)/calf_old.rc.in >$(top_builddir)/calf_old.rc
	install -c -m 644 $(top_builddir)/*.rc $(DESTDIR)$(pkgdatadir)
	rm -f $(top_builddir)/*.rc

uninstall-hook:
	rm -f $(DESTDIR)$(pkgdatadir)/*.rc
	rm -f $(DESTDIR)$(pkgdatadir)/*.png
	rm -f $(DESTDIR)$(pkgdatadir)/gui-*.xml
	rm -r $(DESTDIR)$(pkgdatadir) || true
