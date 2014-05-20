EXTRA_DIST = $(wildcard gui-*.xml) $(wildcard *.png) calf.rc.in

install-data-hook:
	install -d -m 755 $(DESTDIR)$(pkgdatadir)/icons/LV2
	install -c -m 644 $(srcdir)/*.png $(DESTDIR)$(pkgdatadir)
	$(top_builddir)/src/calfmakerdf -m gui -p $(DESTDIR)$(pkgdatadir)
	install -c -m 644 $(srcdir)/gui-*.xml $(DESTDIR)$(pkgdatadir)
	install -c -m 644 $(srcdir)/icons/LV2/*.svg $(DESTDIR)$(pkgdatadir)/icons/LV2
	sed 's#@guidatadir@#$(pkgdatadir)#g' $(srcdir)/calf.rc.in >$(top_builddir)/calf.rc
	install -c -m 644 $(top_builddir)/calf.rc $(DESTDIR)$(pkgdatadir)
	rm -f $(top_builddir)/calf.rc

uninstall-hook:
	rm -f $(DESTDIR)$(pkgdatadir)/calf.rc
	rm -f $(DESTDIR)$(pkgdatadir)/*.png
	rm -f $(DESTDIR)$(pkgdatadir)/gui-*.xml
	rm -f $(DESTDIR)$(pkgdatadir)/icons/LV2/*.svg
	rm -r $(DESTDIR)$(pkgdatadir) || true
