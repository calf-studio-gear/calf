SRCDIRS = icons/LV2
SUBDIRS = $(SRCDIRS)
EXTRA_DIST = $(wildcard gui-*.xml) $(wildcard *.png) calf.rc.in
STYLES = $(wildcard $(srcdir)/styles/*)

install-data-hook:
	$(top_builddir)/src/calfmakerdf -m gui -p $(DESTDIR)$(pkgdatadir)
	install -c -m 644 $(srcdir)/gui-*.xml $(DESTDIR)$(pkgdatadir)
	for f in ${STYLES}; do \
		sn=`basename $${f}` ; \
		sed 's#@guidatadir@#$(pkgdatadir)/styles/'$${sn}'#g' "$${f}"/gtk.rc.in >"$${f}"/gtk.rc ; \
	done
	cp -r $(srcdir)/styles $(DESTDIR)$(pkgdatadir)
	rm -f $(srcdir)/styles/*/*.rc
	rm -f $(DESTDIR)$(pkgdatadir)/styles/*/*.rc.in

uninstall-hook:
	rm -f $(DESTDIR)$(pkgdatadir)/*.rc
	rm -f $(DESTDIR)$(pkgdatadir)/gui-*.xml
	rm -rf $(DESTDIR)$(pkgdatadir)/icons
	rm -rf $(DESTDIR)$(pkgdatadir)/styles
	rm -r $(DESTDIR)$(pkgdatadir) || true
