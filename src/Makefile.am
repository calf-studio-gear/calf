# Calf DSP library - example modules

INCLUDES = -I$(top_srcdir) -I$(srcdir)

SUBDIRS = calf

ladspadir = $(with_ladspa_dir)

bin_PROGRAMS = 

noinst_PROGRAMS = calfbenchmark
noinst_LTLIBRARIES = libcalf.la libcalfstatic.la

AM_CXXFLAGS = -ffast-math -march=i686 -finline-limit=80

if USE_JACK
AM_CXXFLAGS += -DUSE_JACK=1
bin_PROGRAMS += calfjackhost 
calfjackhost_SOURCES = jackhost.cpp
calfjackhost_LDADD = libcalfstatic.la $(JACK_DEPS_LIBS) $(GUI_DEPS_LIBS)
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

calfbenchmark_SOURCES = benchmark.cpp
calfbenchmark_LDADD = 

libcalf_la_SOURCES = modules.cpp giface.cpp synth.cpp
libcalf_la_LDFLAGS = -rpath $(ladspadir) -version-info 0:0:0

libcalfstatic_la_SOURCES = modules.cpp giface.cpp synth.cpp
libcalfstatic_la_LDFLAGS = -static

clean-local:
	$(RM) -f calfjackhost *~

install-data-hook:
if USE_LADSPA
	install -d -m 755 $(DESTDIR)$(with_ladspa_dir)
	install -d -m 755 $(DESTDIR)$(with_ladspa_rdf_dir)
	install -c -m 755 $(top_builddir)/src/.libs/libcalf.so.0.0.0 $(DESTDIR)$(with_ladspa_dir)/calf.so
	$(DESTDIR)$(bindir)/calfmakerdf > $(DESTDIR)$(with_ladspa_rdf_dir)/calf.rdf
endif

#remove calf.so, calf.rdf and - if empty - ladspa dir in usr/share
uninstall-hook:
if USE_LADSPA
	$(RM) -f $(DESTDIR)$(with_ladspa_dir)/calf.so
	$(RM) -f $(DESTDIR)$(with_ladspa_rdf_dir)/calf.rdf
	rmdir $(DESTDIR)$(with_ladspa_rdf_dir)
endif

#install:
#	$(INSTALL) calfjackhost $(bindir)
