# Calf DSP library - example modules

SUBDIRS = calf

lv2dir = $(with_lv2_dir)/calf.lv2

bin_PROGRAMS = 
noinst_LTLIBRARIES =

noinst_PROGRAMS = calfbenchmark
pkglib_LTLIBRARIES = libcalf.la

AM_CPPFLAGS = -I$(top_srcdir) -I$(srcdir)
# TODO: Remove -finline flags is clang is used
AM_CXXFLAGS = -ffast-math $(FLUIDSYNTH_DEPS_CFLAGS) $(LV2_DEPS_CFLAGS)

if USE_GUI
AM_CXXFLAGS += $(GUI_DEPS_CFLAGS)
endif
if USE_JACK
AM_CXXFLAGS += $(JACK_DEPS_CFLAGS)
noinst_LTLIBRARIES += libcalfgui.la
bin_PROGRAMS += calfjackhost 
calfjackhost_SOURCES = gtk_session_env.cpp host_session.cpp jack_client.cpp jackhost.cpp gtk_main_win.cpp connector.cpp session_mgr.cpp
calfjackhost_LDADD = libcalfgui.la libcalf.la $(JACK_DEPS_LIBS) $(GUI_DEPS_LIBS) $(FLUIDSYNTH_DEPS_LIBS)
if USE_LASH
AM_CXXFLAGS += $(LASH_DEPS_CFLAGS)
calfjackhost_LDADD += $(LASH_DEPS_LIBS)
endif
endif

AM_CXXFLAGS += $(GLIB_DEPS_CFLAGS)
noinst_PROGRAMS += calfmakerdf
calfmakerdf_SOURCES = makerdf.cpp
calfmakerdf_LDADD = libcalf.la

calfbenchmark_SOURCES = benchmark.cpp
calfbenchmark_LDADD = libcalf.la

libcalf_la_SOURCES = audio_fx.cpp analyzer.cpp lv2wrap.cpp metadata.cpp modules_tools.cpp modules_delay.cpp modules_comp.cpp modules_limit.cpp modules_dist.cpp modules_filter.cpp modules_mod.cpp modules_pitch.cpp fluidsynth.cpp giface.cpp monosynth.cpp organ.cpp osctl.cpp plugin.cpp preset.cpp synth.cpp utils.cpp wavetable.cpp modmatrix.cpp pffft.c shaping_clipper.cpp
libcalf_la_LIBADD = $(FLUIDSYNTH_DEPS_LIBS) $(GLIB_DEPS_LIBS)
libcalf_la_LDFLAGS = -rpath $(pkglibdir) -avoid-version -lexpat -disable-static

if USE_LV2_GUI

# Version WITH out-of-process GUI - links GTK+, UI controls etc.

if USE_LV2_GUI
pkglib_LTLIBRARIES += libcalflv2gui.la

libcalflv2gui_la_SOURCES = gui.cpp gui_config.cpp gui_controls.cpp ctl_curve.cpp ctl_keyboard.cpp ctl_knob.cpp ctl_led.cpp ctl_tube.cpp ctl_vumeter.cpp ctl_frame.cpp ctl_fader.cpp ctl_buttons.cpp ctl_notebook.cpp ctl_meterscale.cpp ctl_combobox.cpp ctl_tuner.cpp ctl_phasegraph.cpp ctl_pattern.cpp metadata.cpp giface.cpp plugin_gui_window.cpp preset.cpp preset_gui.cpp lv2gui.cpp osctl.cpp utils.cpp ctl_linegraph.cpp drawingutils.cpp

libcalflv2gui_la_LDFLAGS = -rpath $(pkglibdir) -avoid-version -lexpat $(GUI_DEPS_LIBS) -disable-static

if HAVE_LD_NODELETE
libcalflv2gui_la_LDFLAGS += -Wl,-z,nodelete
endif

endif

endif

if USE_GUI
libcalfgui_la_SOURCES = ctl_curve.cpp ctl_keyboard.cpp ctl_knob.cpp ctl_led.cpp ctl_tube.cpp ctl_vumeter.cpp ctl_frame.cpp ctl_fader.cpp ctl_buttons.cpp ctl_notebook.cpp ctl_meterscale.cpp ctl_combobox.cpp ctl_tuner.cpp ctl_phasegraph.cpp ctl_pattern.cpp gui.cpp gui_config.cpp gui_controls.cpp osctl.cpp plugin_gui_window.cpp preset_gui.cpp utils.cpp ctl_linegraph.cpp drawingutils.cpp
libcalfgui_la_LDFLAGS = -static -disable-shared -lexpat
endif

clean-local:
	$(RM) -f calfjackhost *~

install-data-hook:
	install -d -m 755 $(DESTDIR)$(pkgdatadir) 
	install -c -m 644 $(top_srcdir)/presets.xml $(DESTDIR)$(pkgdatadir)
if USE_GUI
	install -c -m 644 $(top_srcdir)/calf-gui.xml $(DESTDIR)$(pkgdatadir)
endif
if USE_LV2
	install -d -m 755 $(DESTDIR)$(lv2dir)
	ln -sf $(pkglibdir)/libcalf.so $(DESTDIR)$(lv2dir)/calf.so
if USE_LV2_GUI
	ln -sf $(pkglibdir)/libcalflv2gui.so $(DESTDIR)$(lv2dir)/calflv2gui.so
endif
	rm -f $(DESTDIR)$(lv2dir)/*.ttl
	$(top_builddir)/src/calfmakerdf -m ttl -p $(DESTDIR)$(lv2dir)/ -d $(DESTDIR)$(pkgdatadir)/
if USE_SORDI
	for f in $(DESTDIR)$(lv2dir)/*.ttl; do sout=`sordi -o turtle "$$f"`; if test $$? != 0; then echo "RDF file $$f is corrupted:"; echo $$sout"<<<"; fi; done
endif
endif

#remove calf.so, calf.rdf and - if empty - ladspa dir in usr/share
uninstall-hook:
if USE_LV2
if USE_LV2_GUI
	rm -f $(DESTDIR)$(lv2dir)/calflv2gui.so
endif
	rm -f $(DESTDIR)$(lv2dir)/calf.so
	rm -f $(DESTDIR)$(lv2dir)/*.ttl
	rmdir -p $(DESTDIR)$(lv2dir) || true
endif
if USE_GUI
	rm -f $(DESTDIR)$(pkgdatadir)/calf-gui.xml
endif
	rm -f $(DESTDIR)$(pkgdatadir)/presets*.xml
	rmdir $(DESTDIR)$(pkgdatadir) || true
