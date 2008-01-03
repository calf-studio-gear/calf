# Calf DSP library - example modules

INCLUDES = -I$(top_srcdir) -I$(srcdir)

SUBDIRS = calf

ladspadir = $(with_ladspa_dir)

bin_PROGRAMS = 

noinst_PROGRAMS = calfbenchmark calfdssigui
noinst_LTLIBRARIES = calf.la libcalfstatic.la

AM_CXXFLAGS = -ffast-math -march=i686 -finline-limit=80 -DPKGLIBDIR=\"$(pkglibdir)\"

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
endif
if ENABLE_EXPERIMENTAL
AM_CXXFLAGS += -DENABLE_EXPERIMENTAL=1
endif

if USE_LADSPA
AM_CXXFLAGS += -DUSE_LADSPA=1
bin_PROGRAMS += calfmakerdf
calfmakerdf_SOURCES = makerdf.cpp
calfmakerdf_LDADD = libcalfstatic.la
endif
if USE_DSSI
AM_CXXFLAGS += -DUSE_DSSI=1
endif

calfbenchmark_SOURCES = benchmark.cpp
calfbenchmark_LDADD =  -lglib-2.0 libcalfgui.la

calfdssigui_SOURCES = dssigui.cpp
calfdssigui_LDADD =  -lglib-2.0 libcalfgui.la

calf_la_SOURCES = modules.cpp giface.cpp monosynth.cpp organ.cpp preset.cpp synth.cpp 
calf_la_LDFLAGS = -rpath $(ladspadir) -avoid-version -module -lexpat

libcalfstatic_la_SOURCES = modules.cpp giface.cpp monosynth.cpp organ.cpp preset.cpp synth.cpp
libcalfstatic_la_LDFLAGS = -static -lexpat

libcalfgui_la_SOURCES = gui.cpp preset_gui.cpp custom_ctl.cpp osctl.cpp
libcalfgui_la_LDFLAGS = -static

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
	$(DESTDIR)$(bindir)/calfmakerdf > $(DESTDIR)$(with_ladspa_rdf_dir)/calf.rdf
endif
if USE_DSSI
	install -d -m 755 $(DESTDIR)$(with_dssi_dir)
	install -d -m 755 $(DESTDIR)$(with_dssi_dir)/calf
	install -c -m 755 $(top_builddir)/src/.libs/calf.so $(DESTDIR)$(with_dssi_dir)/calf.so
	install -c -m 755 $(top_builddir)/src/calfdssigui $(DESTDIR)$(with_dssi_dir)/calf/calf_gtk
endif

#remove calf.so, calf.rdf and - if empty - ladspa dir in usr/share
uninstall-hook:
if USE_JACK
	rm -f $(DESTDIR)$(pkglibdir)/calf.glade 
endif
	rm -f $(DESTDIR)$(pkglibdir)/presets.xml
	rm -f $(DESTDIR)$(pkglibdir)/knob.png
	rmdir $(DESTDIR)$(pkglibdir)    
if USE_LADSPA
	$(RM) -f $(DESTDIR)$(with_ladspa_dir)/calf.so
	$(RM) -f $(DESTDIR)$(with_ladspa_rdf_dir)/calf.rdf
	rmdir $(DESTDIR)$(with_ladspa_rdf_dir)
endif
if USE_DSSI
	$(RM) -f $(DESTDIR)$(with_dssi_dir)/calf.so
	$(RM) -f $(DESTDIR)$(with_dssi_dir)/calf/calf_gtk
	$(RMDIR) -f $(DESTDIR)$(with_dssi_dir)/calf
endif

#install:
#	$(INSTALL) calfjackhost $(bindir)
