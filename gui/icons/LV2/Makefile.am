EXTRA_DIST = $(wildcard *.svg)

install-data-hook:
	install -d -m 755 $(DESTDIR)$(pkgdatadir)/icons/LV2
	install -c -m 644 $(srcdir)/*.svg $(DESTDIR)$(pkgdatadir)/icons/LV2

uninstall-hook:
	rm -f $(DESTDIR)$(pkgdatadir)/icons/LV2/*.svg
