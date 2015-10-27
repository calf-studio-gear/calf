SRCDIRS = icons/LV2 
SUBDIRS = $(SRCDIRS)
STYLES = $(wildcard $(srcdir)/styles/*)

install-data-hook:
	$(top_builddir)/src/calfmakerdf -m gui -p $(DESTDIR)$(pkgdatadir)
	for f in ${STYLES}; do \
		sn=`basename $${f}` ; \
		sed 's#@guidatadir@#$(pkgdatadir)/styles/'$${sn}'#g' "$${f}"/gtk.rc.in >"$${f}"/gtk.rc ; \
	done
	cp -r $(srcdir)/styles $(DESTDIR)$(pkgdatadir)
	rm -f $(srcdir)/styles/*/*.rc
	rm -f $(DESTDIR)$(pkgdatadir)/styles/*/*.rc.in
	cp -r $(srcdir)/gui $(DESTDIR)$(pkgdatadir)
	cp -r $(srcdir)/strips $(DESTDIR)$(pkgdatadir)

uninstall-hook:
	rm -f $(DESTDIR)$(pkgdatadir)/*.rc
	rm -rf $(DESTDIR)$(pkgdatadir)/gui
	rm -rf $(DESTDIR)$(pkgdatadir)/strips
	rm -rf $(DESTDIR)$(pkgdatadir)/icons
	rm -rf $(DESTDIR)$(pkgdatadir)/styles
	rm -r $(DESTDIR)$(pkgdatadir) || true
