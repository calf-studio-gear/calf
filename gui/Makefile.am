SRCDIRS = icons/LV2 
SUBDIRS = $(SRCDIRS)
STYLES = $(wildcard $(srcdir)/styles/*)
EXTRA_DIST = $(wildcard $(srcdir)/styles) $(wildcard $(srcdir)/strips) $(wildcard $(srcdir)/gui)

install-data-hook:
	$(top_builddir)/src/calfmakerdf -m gui -p $(DESTDIR)$(pkgdatadir)
	install -d -m 755 $(DESTDIR)$(pkgdatadir)/styles/
	for f in ${STYLES}; do \
		sn=`basename $${f}` ; \
		install -d -m 755 $(DESTDIR)$(pkgdatadir)/styles/$${sn} ; \
		install -c -m 644 $(srcdir)/styles/$${sn}/*.png $(DESTDIR)$(pkgdatadir)/styles/$${sn}/ ; \
		sed 's#@guidatadir@#$(pkgdatadir)/styles/'$${sn}'#g' "$${f}"/gtk.rc.in >"$(DESTDIR)$(pkgdatadir)/styles/$${sn}/gtk.rc" ; \
	done
	install -d -m 755 $(DESTDIR)$(pkgdatadir)/gui
	install -c -m 644 $(srcdir)/gui/*.xml $(DESTDIR)$(pkgdatadir)/gui/
	install -d -m 755 $(DESTDIR)$(pkgdatadir)/strips
	install -c -m 644 $(srcdir)/strips/*.xml $(DESTDIR)$(pkgdatadir)/strips/

uninstall-hook:
	rm -f $(DESTDIR)$(pkgdatadir)/*.rc
	rm -rf $(DESTDIR)$(pkgdatadir)/gui
	rm -rf $(DESTDIR)$(pkgdatadir)/strips
	rm -rf $(DESTDIR)$(pkgdatadir)/icons
	rm -rf $(DESTDIR)$(pkgdatadir)/styles
	rm -r $(DESTDIR)$(pkgdatadir) || true
