# Calf DSP library - example modules

INCLUDES = -I$(top_srcdir) -I$(srcdir)

SUBDIRS = calf

ladspadir = $(with_ladspa_dir)

bin_PROGRAMS = calfjackhost

calfjackhost_SOURCES = jackhost.cpp
calfjackhost_LDADD = libcalfstatic.la

noinst_LTLIBRARIES = libcalf.la libcalfstatic.la

libcalf_la_SOURCES = modules.cpp giface.cpp
libcalf_la_LDFLAGS = -rpath $(ladspadir) -version-info 0:0:0

libcalfstatic_la_SOURCES = modules.cpp giface.cpp
libcalfstatic_la_LDFLAGS = -static

clean-local:
	$(RM) -f calfjackhost *~

install-data-hook:
	install -d -m 755 $(DESTDIR)$(with_ladspa_dir)
	install -c -m 755 $(top_builddir)/src/.libs/libcalf.so.0.0.0 $(DESTDIR)$(with_ladspa_dir)/calf.so

uninstall-hook:
	$(RM) -f $(with_ladspa_dir)/calf.so

#install:
#	$(INSTALL) calfjackhost $(bindir)
