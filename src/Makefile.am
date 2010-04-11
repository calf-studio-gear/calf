# Calf DSP library - example modules

INCLUDES = -I$(top_srcdir) -I$(srcdir)

SUBDIRS = calf

ladspadir = $(with_ladspa_dir)
lv2dir = $(with_lv2_dir)/calf.lv2

bin_PROGRAMS = 
noinst_LTLIBRARIES =

noinst_PROGRAMS = calfbenchmark
pkglib_LTLIBRARIES = calf.la
if USE_DSSI_GUI
pkglib_PROGRAMS = calf_gtk
endif
if USE_LV2_GUI
noinst_LTLIBRARIES += calflv2gui.la
pkglib_PROGRAMS = calf_gtk
endif

AM_CXXFLAGS = -ffast-math -finline-limit=80 $(FLUIDSYNTH_DEPS_CFLAGS)

if USE_GUI
AM_CXXFLAGS += $(GUI_DEPS_CFLAGS)
endif
if USE_JACK
AM_CXXFLAGS += $(JACK_DEPS_CFLAGS)
noinst_LTLIBRARIES += libcalfgui.la
bin_PROGRAMS += calfjackhost 
calfjackhost_SOURCES = host_session.cpp jack_client.cpp jackhost.cpp main_win.cpp
calfjackhost_LDADD = libcalfgui.la calf.la $(JACK_DEPS_LIBS) $(GUI_DEPS_LIBS) $(FLUIDSYNTH_DEPS_LIBS)
if USE_LASH
AM_CXXFLAGS += $(LASH_DEPS_CFLAGS)
calfjackhost_LDADD += $(LASH_DEPS_LIBS)
endif
endif

AM_CXXFLAGS += $(GLIB_DEPS_CFLAGS)
noinst_PROGRAMS += calfmakerdf
calfmakerdf_SOURCES = makerdf.cpp 
calfmakerdf_LDADD = calf.la

calfbenchmark_SOURCES = benchmark.cpp
calfbenchmark_LDADD = $(GLIB_DEPS_LIBS) calf.la

if USE_EXEC_GUI
calf_gtk_SOURCES = dssigui.cpp
calf_gtk_LDADD = calf.la libcalfgui.la $(GLIB_DEPS_LIBS) $(GUI_DEPS_LIBS)
calfbenchmark_CXXFLAGS = $(AM_CXXFLAGS) -DTEST_OSC
calfbenchmark_LDADD += libcalfgui.la
endif

calf_la_SOURCES = audio_fx.cpp modules.cpp modules_dsp.cpp fluidsynth.cpp giface.cpp monosynth.cpp organ.cpp osctl.cpp osctlnet.cpp plugin.cpp preset.cpp synth.cpp utils.cpp wavetable.cpp modmatrix.cpp 
calf_la_LIBADD ?= $(FLUIDSYNTH_DEPS_LIBS)
if USE_DEBUG
calf_la_LDFLAGS = -rpath $(ladspadir) -avoid-version -module -lexpat -disable-static $(FLUIDSYNTH_DEPS_LIBS)
else
calf_la_LDFLAGS = -rpath $(ladspadir) -avoid-version -module -lexpat -disable-static -export-symbols-regex "(ladspa_|lv2_|dssi_)descriptor"  $(FLUIDSYNTH_DEPS_LIBS)
endif

if USE_LV2_GTK_GUI

# Version WITH out-of-process GUI - links GTK+, UI controls etc.

calflv2gui_la_SOURCES = gui.cpp gui_controls.cpp ctl_curve.cpp ctl_keyboard.cpp ctl_led.cpp custom_ctl.cpp modules.cpp giface.cpp preset.cpp preset_gui.cpp lv2gui.cpp osctl.cpp osctlnet.cpp utils.cpp

if USE_DEBUG
calflv2gui_la_LDFLAGS = -rpath $(lv2dir) -avoid-version -module -lexpat $(GUI_DEPS_LIBS) -disable-static
else
calflv2gui_la_LDFLAGS = -rpath $(lv2dir) -avoid-version -module -lexpat -export-symbols-regex "lv2_gui_descriptor" $(GUI_DEPS_LIBS) -disable-static
endif

else

# Version WITHOUT out-of-process GUI - links GLib only

if USE_LV2_GUI
calflv2gui_la_SOURCES = modules.cpp giface.cpp preset.cpp lv2gui.cpp osctl.cpp osctlnet.cpp utils.cpp

if USE_DEBUG
calflv2gui_la_LDFLAGS = -rpath $(lv2dir) -avoid-version -module -lexpat $(GLIB_DEPS_LIBS) -disable-static
else
calflv2gui_la_LDFLAGS = -rpath $(lv2dir) -avoid-version -module -lexpat -export-symbols-regex "lv2_gui_descriptor" $(GLIB_DEPS_LIBS) -disable-static
endif

endif

endif

if USE_GUI
libcalfgui_la_SOURCES = gui.cpp gui_controls.cpp ctl_curve.cpp ctl_keyboard.cpp ctl_led.cpp preset_gui.cpp custom_ctl.cpp osctl.cpp osctlnet.cpp osctl_glib.cpp utils.cpp
libcalfgui_la_LDFLAGS = -static -disable-shared
endif

clean-local:
	$(RM) -f calfjackhost *~

install-data-hook:
	install -d -m 755 $(DESTDIR)$(pkgdatadir) 
	install -c -m 644 $(top_srcdir)/presets.xml $(DESTDIR)$(pkgdatadir)
if USE_JACK
	install -c -m 644 $(top_srcdir)/calf.glade $(DESTDIR)$(pkgdatadir)
endif
if USE_LADSPA
	install -d -m 755 $(DESTDIR)$(with_ladspa_dir)
	install -d -m 755 $(DESTDIR)$(with_ladspa_rdf_dir)
	ln -sf $(DESTDIR)$(pkglibdir)/calf.so $(DESTDIR)$(with_ladspa_dir)/calf.so
	$(top_builddir)/src/calfmakerdf > $(DESTDIR)$(with_ladspa_rdf_dir)/calf.rdf
endif
if USE_DSSI
	install -d -m 755 $(DESTDIR)$(with_dssi_dir)
	install -d -m 755 $(DESTDIR)$(with_dssi_dir)/calf
	ln -sf $(DESTDIR)$(pkglibdir)/calf.so $(DESTDIR)$(with_dssi_dir)/calf.so
	ln -sf $(DESTDIR)$(pkglibdir)/calf_gtk $(DESTDIR)$(with_dssi_dir)/calf/calf_gtk
endif
if USE_LV2
	install -d -m 755 $(DESTDIR)$(lv2dir)
	ln -sf $(DESTDIR)$(pkglibdir)/calf.so $(DESTDIR)$(lv2dir)/calf.so
	ln -sf $(DESTDIR)$(pkglibdir)/calf_gtk $(DESTDIR)$(lv2dir)/calf_gtk
if USE_LV2_GUI
	install -c -m 755 $(top_builddir)/src/.libs/calflv2gui.so $(DESTDIR)$(lv2dir)/calflv2gui.so
endif
	rm -f $(DESTDIR)$(lv2dir)/*.ttl
	$(top_builddir)/src/calfmakerdf -m ttl -p $(DESTDIR)$(lv2dir)/
endif

#remove calf.so, calf.rdf and - if empty - ladspa dir in usr/share
uninstall-hook:
if USE_LV2
if USE_LV2_GUI
	rm -f $(DESTDIR)$(lv2dir)/calflv2gui.so
	rm -f $(DESTDIR)$(lv2dir)/calf_gtk
endif
	rm -f $(DESTDIR)$(lv2dir)/calf.so
	rm -f $(DESTDIR)$(lv2dir)/*.ttl
	rmdir -p $(DESTDIR)$(lv2dir) || true
endif
if USE_JACK
	rm -f $(DESTDIR)$(pkgdatadir)/calf.glade 
endif
	rm -f $(DESTDIR)$(pkgdatadir)/presets.xml
	rmdir $(DESTDIR)$(pkgdatadir) || true
if USE_LADSPA
	$(RM) -f $(DESTDIR)$(with_ladspa_dir)/calf.so
	$(RM) -f $(DESTDIR)$(with_ladspa_rdf_dir)/calf.rdf
	rmdir -p $(DESTDIR)$(with_ladspa_rdf_dir) || true
endif
if USE_DSSI
	$(RM) -f $(DESTDIR)$(with_dssi_dir)/calf.so
if USE_DSSI_GUI
	$(RM) -f $(DESTDIR)$(with_dssi_dir)/calf/calf_gtk
endif
	rmdir -p $(DESTDIR)$(with_dssi_dir)/calf || true
endif

