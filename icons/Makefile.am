EXTRA_DIST = calf_16x16.png calf_24x24.png calf_22x22.png calf_32x32.png calf_48x48.png calf_64x64.png calf_128x128.png calf_256x256.png calf_scalable.svg calf_plugin_16x16.png calf_plugin_24x24.png calf_plugin_22x22.png calf_plugin_32x32.png calf_plugin_48x48.png calf_plugin_64x64.png calf_plugin_128x128.png calf_plugin_256x256.png calf_plugin_scalable.svg
SIZES = 16x16 22x22 24x24 32x32 48x48 64x64 128x128 256x256

install-data-hook:
	-for fn in $(SIZES); do \
		mkdir -p "$(DESTDIR)$(datadir)/icons/hicolor/$${fn}/apps/"; \
		cp "$(top_srcdir)/icons/calf_$${fn}.png" "$(DESTDIR)$(datadir)/icons/hicolor/$${fn}/apps/calf.png"; \
		cp "$(top_srcdir)/icons/calf_plugin_$${fn}.png" "$(DESTDIR)$(datadir)/icons/hicolor/$${fn}/apps/calf_plugin.png"; \
	done
	mkdir -p "$(DESTDIR)$(datadir)/icons/hicolor/scalable/apps/"; \
	cp "$(top_srcdir)/icons/calf_scalable.svg" "$(DESTDIR)$(datadir)/icons/hicolor/scalable/apps/calf.svg";
	cp "$(top_srcdir)/icons/calf_plugin_scalable.svg" "$(DESTDIR)$(datadir)/icons/hicolor/scalable/apps/calf_plugin.svg";
	@-test -z "$(DESTDIR)" && gtk-update-icon-cache -f -t $(DESTDIR)$(datadir)/icons/hicolor
	
uninstall-hook:
	@-for fn in $(SIZES); do \
		rm -f "$(DESTDIR)$(datadir)/icons/hicolor/$${fn}/apps/calf.png"; \
		rm -f "$(DESTDIR)$(datadir)/icons/hicolor/$${fn}/apps/calf_plugin.png"; \
	done
	rm -f "$(DESTDIR)$(datadir)/icons/hicolor/scalable/apps/calf.svg";
	rm -f "$(DESTDIR)$(datadir)/icons/hicolor/scalable/apps/calf_plugin.svg";
	@-test -z "$(DESTDIR)" && gtk-update-icon-cache -f -t $(DESTDIR)$(datadir)/icons/hicolor
	
