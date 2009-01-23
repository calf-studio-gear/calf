EXTRA_DIST = calf_16x16.png calf_24x24.png calf_32x32.png calf_48x48.png
SIZES = 16x16 24x24 32x32 48x48

install-data-hook:
	-for fn in $(SIZES); do \
		mkdir -p "$(datadir)/icons/hicolor/$${fn}/apps/"; \
		cp "calf_$${fn}.png" "$(datadir)/icons/hicolor/$${fn}/apps/calf.png"; \
	done
	@-gtk-update-icon-cache -f -t $(datadir)/icons/hicolor
	
uninstall-hook:
	@-for fn in $(SIZES); do \
		rm -f "$(datadir)/icons/hicolor/$${fn}/apps/calf.png"; \
	done
	@-gtk-update-icon-cache -f -t $(datadir)/icons/hicolor
	
