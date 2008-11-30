# Calf DSP library - example modules

INCLUDES = -I$(top_srcdir) -I$(srcdir)

SUBDIRS = calf

ladspadir = $(with_ladspa_dir)
lv2dir = $(with_lv2_dir)

bin_PROGRAMS = 

noinst_PROGRAMS = calfbenchmark
noinst_LTLIBRARIES = calf.la libcalfstatic.la
if USE_DSSI_GUI
noinst_PROGRAMS += calfdssigui
endif
if USE_LV2_GUI
noinst_LTLIBRARIES += calflv2gui.la
endif

AM_CXXFLAGS = -ffast-math -finline-limit=80 

if USE_GUI
AM_CXXFLAGS += $(GUI_DEPS_CFLAGS)
endif
if USE_JACK
AM_CXXFLAGS += $(JACK_DEPS_CFLAGS)
noinst_LTLIBRARIES += libcalfgui.la
bin_PROGRAMS += calfjackhost 
calfjackhost_SOURCES = jackhost.cpp
calfjackhost_LDADD = libcalfgui.la libcalfstatic.la $(JACK_DEPS_LIBS) $(GUI_DEPS_LIBS)
if USE_LASH
AM_CXXFLAGS += $(LASH_DEPS_CFLAGS)
calfjackhost_LDADD += $(LASH_DEPS_LIBS)
endif
endif

AM_CXXFLAGS += $(GLIB_DEPS_CFLAGS)
noinst_PROGRAMS += calfmakerdf
calfmakerdf_SOURCES = makerdf.cpp 
calfmakerdf_LDADD = libcalfstatic.la

calfbenchmark_SOURCES = benchmark.cpp
calfbenchmark_LDADD = $(GLIB_DEPS_LIBS) libcalfstatic.la

if USE_DSSI_GUI
calfdssigui_SOURCES = dssigui.cpp
calfdssigui_LDADD = libcalfstatic.la libcalfgui.la $(GLIB_DEPS_LIBS) $(GUI_DEPS_LIBS)
calfbenchmark_CXXFLAGS = $(AM_CXXFLAGS) -DTEST_OSC
calfbenchmark_LDADD += libcalfgui.la
endif

calf_la_SOURCES = modules.cpp modules_dsp.cpp modules_small.cpp giface.cpp monosynth.cpp organ.cpp osctl.cpp plugin.cpp preset.cpp synth.cpp utils.cpp
if USE_DEBUG
calf_la_LDFLAGS = -rpath $(ladspadir) -avoid-version -module -lexpat -disable-static
else
calf_la_LDFLAGS = -rpath $(ladspadir) -avoid-version -module -lexpat -disable-static -export-symbols-regex "(ladspa_|lv2_|dssi_)descriptor"
endif

if USE_LV2_GUI
calflv2gui_la_SOURCES = gui.cpp ctl_curve.cpp ctl_keyboard.cpp ctl_led.cpp custom_ctl.cpp modules.cpp giface.cpp preset.cpp synth.cpp lv2gui.cpp main_win.cpp utils.cpp
if USE_DEBUG
calflv2gui_la_LDFLAGS = -rpath $(lv2dir) -avoid-version -module -lexpat $(GUI_DEPS_LIBS) -disable-static
else
calflv2gui_la_LDFLAGS = -rpath $(lv2dir) -avoid-version -module -lexpat -export-symbols-regex "lv2_gui_descriptor" $(GUI_DEPS_LIBS) -disable-static
endif
endif

libcalfstatic_la_SOURCES = modules.cpp modules_dsp.cpp modules_small.cpp giface.cpp monosynth.cpp organ.cpp osctl.cpp preset.cpp synth.cpp utils.cpp
libcalfstatic_la_LDFLAGS = -static -lexpat -disable-shared

if USE_GUI
libcalfgui_la_SOURCES = gui.cpp ctl_curve.cpp ctl_keyboard.cpp ctl_led.cpp preset_gui.cpp custom_ctl.cpp osctl.cpp osctlnet.cpp main_win.cpp utils.cpp
libcalfgui_la_LDFLAGS = -static -disable-shared
endif

clean-local:
	$(RM) -f calfjackhost *~

install-data-hook:
	install -d -m 755 $(DESTDIR)$(pkglibdir)    
	install -c -m 644 $(top_srcdir)/presets.xml $(DESTDIR)$(pkglibdir)
	install -c -m 644 $(top_srcdir)/knob.png $(DESTDIR)$(pkglibdir)
	$(top_builddir)/src/calfmakerdf -m gui -p $(DESTDIR)$(pkglibdir)
	install -c -m 644 $(top_srcdir)/gui/gui-*.xml $(DESTDIR)$(pkglibdir)
if USE_JACK
	install -c -m 644 $(top_srcdir)/calf.glade $(DESTDIR)$(pkglibdir)
endif
if USE_LADSPA
	install -d -m 755 $(DESTDIR)$(with_ladspa_dir)
	install -d -m 755 $(DESTDIR)$(with_ladspa_rdf_dir)
	install -c -m 755 $(top_builddir)/src/.libs/calf.so $(DESTDIR)$(with_ladspa_dir)/calf.so
	$(top_builddir)/src/calfmakerdf > $(DESTDIR)$(with_ladspa_rdf_dir)/calf.rdf
endif
if USE_DSSI
	install -d -m 755 $(DESTDIR)$(with_dssi_dir)
	install -d -m 755 $(DESTDIR)$(with_dssi_dir)/calf
	install -c -m 755 $(top_builddir)/src/.libs/calf.so $(DESTDIR)$(with_dssi_dir)/calf.so
if USE_DSSI_GUI
	install -c -m 755 $(top_builddir)/src/calfdssigui $(DESTDIR)$(with_dssi_dir)/calf/calf_gtk
endif
endif
if USE_LV2
	install -d -m 755 $(DESTDIR)$(with_lv2_dir)/calf.lv2
	install -c -m 755 $(top_builddir)/src/.libs/calf.so $(DESTDIR)$(with_lv2_dir)/calf.lv2/calf.so
if USE_LV2_GUI
	install -c -m 755 $(top_builddir)/src/.libs/calflv2gui.so $(DESTDIR)$(with_lv2_dir)/calf.lv2/calflv2gui.so
endif
	rm -f $(DESTDIR)$(with_lv2_dir)/calf.lv2/*.ttl
	$(top_builddir)/src/calfmakerdf -m ttl -p $(DESTDIR)$(with_lv2_dir)/calf.lv2/
endif

#remove calf.so, calf.rdf and - if empty - ladspa dir in usr/share
uninstall-hook:
if USE_LV2
if USE_LV2_GUI
	rm -f $(DESTDIR)$(with_lv2_dir)/calf.lv2/calflv2gui.so
endif
	rm -f $(DESTDIR)$(with_lv2_dir)/calf.lv2/calf.so
	rm -f $(DESTDIR)$(with_lv2_dir)/calf.lv2/*.ttl
	rmdir -p $(DESTDIR)$(with_lv2_dir)/calf.lv2 || true
endif
if USE_JACK
	rm -f $(DESTDIR)$(pkglibdir)/calf.glade 
endif
	rm -f $(DESTDIR)$(pkglibdir)/gui-*.xml
	rm -f $(DESTDIR)$(pkglibdir)/presets.xml
	rm -f $(DESTDIR)$(pkglibdir)/knob.png
	rmdir $(DESTDIR)$(pkglibdir) || true
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

#install:
#	$(INSTALL) calfjackhost $(bindir)
