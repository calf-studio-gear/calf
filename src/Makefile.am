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

AM_CXXFLAGS = -ffast-math -finline-limit=80 -DPKGLIBDIR=\"$(pkglibdir)\"

if USE_JACK
AM_CXXFLAGS += -DUSE_JACK=1 $(JACK_DEPS_CFLAGS) $(GUI_DEPS_CFLAGS)
noinst_LTLIBRARIES += libcalfgui.la
bin_PROGRAMS += calfjackhost 
calfjackhost_SOURCES = jackhost.cpp
calfjackhost_LDADD = libcalfgui.la libcalfstatic.la $(JACK_DEPS_LIBS) $(GUI_DEPS_LIBS)
if OLD_JACK
AM_CXXFLAGS += -DOLD_JACK=1
endif
if USE_PHAT
AM_CXXFLAGS += -DUSE_PHAT=1 $(PHAT_DEPS_CFLAGS)
calfjackhost_LDADD += $(PHAT_DEPS_LIBS)
endif
if USE_LASH
calfjackhost_LDADD += $(LASH_DEPS_LIBS)
endif
endif

if ENABLE_EXPERIMENTAL
AM_CXXFLAGS += -DENABLE_EXPERIMENTAL=1
endif

if USE_LADSPA
AM_CXXFLAGS += -DUSE_LADSPA=1
endif
noinst_PROGRAMS += calfmakerdf
calfmakerdf_SOURCES = makerdf.cpp
calfmakerdf_LDADD = libcalfstatic.la
if USE_DSSI
AM_CXXFLAGS += -DUSE_DSSI=1
endif
if USE_DSSI_GUI
AM_CXXFLAGS += -DUSE_DSSI_GUI=1
endif
if USE_LV2
AM_CXXFLAGS += -DUSE_LV2=1
endif
if USE_LV2_GUI
AM_CXXFLAGS += -DUSE_LV2_GUI=1
endif
if USE_LASH
AM_CXXFLAGS += -DUSE_LASH=1 $(LASH_DEPS_CFLAGS)
endif

calfbenchmark_SOURCES = benchmark.cpp
calfbenchmark_LDADD =  -lglib-2.0

if USE_DSSI_GUI
calfdssigui_SOURCES = dssigui.cpp
calfdssigui_LDADD =  -lglib-2.0 libcalfstatic.la libcalfgui.la $(GUI_DEPS_LIBS)
if USE_PHAT
calfdssigui_LDADD += $(PHAT_DEPS_LIBS)
endif
endif

calf_la_SOURCES = modules.cpp giface.cpp monosynth.cpp organ.cpp preset.cpp synth.cpp 
calf_la_LDFLAGS = -rpath $(ladspadir) -avoid-version -module -lexpat -export-symbols-regex "(ladspa_|lv2_|dssi_)descriptor"

if USE_LV2_GUI
calflv2gui_la_SOURCES = gui.cpp custom_ctl.cpp modules.cpp giface.cpp monosynth.cpp organ.cpp preset.cpp synth.cpp lv2gui.cpp
calflv2gui_la_LDFLAGS = -rpath $(lv2dir) -avoid-version -module -lexpat -export-symbols-regex "lv2_gui_descriptor" 
if USE_PHAT
calflv2gui_la_LIBADD = $(PHAT_DEPS_LIBS)
endif
endif

libcalfstatic_la_SOURCES = modules.cpp giface.cpp monosynth.cpp organ.cpp preset.cpp synth.cpp
libcalfstatic_la_LDFLAGS = -static -lexpat

if USE_GUI
libcalfgui_la_SOURCES = gui.cpp preset_gui.cpp custom_ctl.cpp osctl.cpp
libcalfgui_la_LDFLAGS = -static
endif

clean-local:
	$(RM) -f calfjackhost *~

install-data-hook:
	install -d -m 755 $(DESTDIR)$(pkglibdir)    
	install -c -m 644 $(top_srcdir)/presets.xml $(DESTDIR)$(pkglibdir)
	install -c -m 644 $(top_srcdir)/knob.png $(DESTDIR)$(pkglibdir)
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
	$(top_builddir)/src/calfmakerdf -m ttl >$(DESTDIR)$(with_lv2_dir)/calf.lv2/calf.ttl
	$(top_builddir)/src/calfmakerdf -m manifest >$(DESTDIR)$(with_lv2_dir)/calf.lv2/manifest.ttl
endif

#remove calf.so, calf.rdf and - if empty - ladspa dir in usr/share
uninstall-hook:
if USE_LV2
if USE_LV2_GUI
	rm -f $(DESTDIR)$(with_lv2_dir)/calf.lv2/calflv2gui.so
endif
	rm -f $(DESTDIR)$(with_lv2_dir)/calf.lv2/calf.so
	rm -f $(DESTDIR)$(with_lv2_dir)/calf.lv2/calf.ttl $(DESTDIR)$(with_lv2_dir)/calf.lv2/manifest.ttl
	rmdir -p $(DESTDIR)$(with_lv2_dir)/calf.lv2 || true
endif
if USE_JACK
	rm -f $(DESTDIR)$(pkglibdir)/calf.glade 
endif
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
