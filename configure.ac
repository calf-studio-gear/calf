#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.63)
AC_INIT([calf],[0.0.19],[wdev@foltman.com])
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADER([config.h])
LT_INIT([dlopen])
LT_LANG([C++])

AM_INIT_AUTOMAKE(1.8)

if test "x$prefix" = "xNONE"; then 
  prefix=$ac_default_prefix
fi

# Checks for programs.
AC_PROG_INSTALL

############################################################################################
# Set initial values of shell variables
LADSPA_ENABLED="no"
EXEC_GUI_ENABLED="no"
DSSI_GUI_ENABLED="no"
LV2_GTK_GUI_ENABLED="no"
LV2_GUI_ENABLED="no"
JACK_FOUND="no"
JACK_ENABLED="no"
OLD_JACK="no"
GUI_ENABLED="no"
LASH_ENABLED="no"

############################################################################################
# Create configure substitutions for dependencies

AC_SUBST(GLIB_DEPS_CFLAGS)
AC_SUBST(GLIB_DEPS_LIBS)
AC_SUBST(GUI_DEPS_CFLAGS)
AC_SUBST(GUI_DEPS_LIBS)
AC_SUBST(JACK_DEPS_CFLAGS)
AC_SUBST(JACK_DEPS_LIBS)
AC_SUBST(FLUIDSYNTH_DEPS_CFLAGS)
AC_SUBST(FLUIDSYNTH_DEPS_LIBS)

############################################################################################
# Detect dependencies

# Checks for libraries.
PKG_PROG_PKG_CONFIG

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([memory.h stdint.h stdlib.h string.h time.h math.h])

AC_CHECK_HEADER(ladspa.h, LADSPA_FOUND="yes", LADSPA_FOUND="no")

AC_CHECK_HEADER(dssi.h, DSSI_FOUND="yes", DSSI_FOUND="no")

AC_CHECK_HEADER(expat.h, true, AC_MSG_ERROR([Expat XML library not found]))
AC_CHECK_LIB(expat, XML_Parse, true, AC_MSG_ERROR([Expat XML library not found]))

# JACK
AC_CHECK_HEADERS([jack/jack.h], have_jack_header=yes, have_jack_header=no)

# GLib
PKG_CHECK_MODULES(GLIB_DEPS, glib-2.0 >= 2.0.0, true, AC_MSG_ERROR([glib-2.0 library not found]))

# Fluidsynth
PKG_CHECK_MODULES(FLUIDSYNTH_DEPS, fluidsynth >= 1.0.7, true, AC_MSG_ERROR([fluidsynth library not found]))

if test "$have_jack_header" = "yes"; then
  PKG_CHECK_MODULES(JACK_DEPS, jack >= 0.103.0,
    AC_CHECK_LIB([jack], [jack_port_register], JACK_FOUND="yes", ),)
fi

if test "$JACK_FOUND" = "yes"; then
  PKG_CHECK_MODULES(JACK_MIDI_DEPS, jack >= 0.105.0, OLD_JACK="no", OLD_JACK="yes")
fi

PKG_CHECK_MODULES(LV2_DEPS, lv2core >= 1, LV2_FOUND="yes", LV2_FOUND="no")

PKG_CHECK_MODULES(LASH_DEPS, lash-1.0 >= 0.6.0,
  AC_CHECK_LIB([lash], [lash_client_is_being_restored], LASH_0_6_FOUND="yes", LASH_0_6_FOUND="no"),
  LASH_0_6_FOUND="no")
if test "$LASH_0_6_FOUND" = "yes"; then
  LASH_FOUND="yes"
else
  PKG_CHECK_MODULES(LASH_DEPS, lash-1.0 >= 0.5.2, LASH_FOUND="yes", LASH_FOUND="no")
fi
PKG_CHECK_MODULES(SLV2_HACK, slv2 <= 0.6.1, SLV2_UNSUPPORTED="yes", SLV2_UNSUPPORTED="no")

############################################################################################
# Assign initial values to feature control variables

LADSPA_ENABLED=$LADSPA_FOUND
DSSI_ENABLED=$DSSI_FOUND
LV2_ENABLED=$LV2_FOUND
LASH_ENABLED=$LASH_FOUND

if test "$JACK_FOUND" = "yes" -o "$DSSI_FOUND" = "yes" -o "$LV2_FOUND" = "yes"; then
  PKG_CHECK_MODULES(GUI_DEPS, gtk+-2.0 >= 2.8.0 gconf-2.0 >= 2.8.0 libglade-2.0 >= 2.4.0 cairo >= 1.2.0,
    GUI_ENABLED="yes",
    GUI_ENABLED="no (GTK+ 2.8, GConf 2.8, cairo 1.2 and libglade 2.4.0 or newer required)"
  )
fi

############################################################################################
# Look for settings and installed libraries to determine what to compile/install/use
if test "$LADSPA_FOUND" = "yes"; then
  AC_MSG_CHECKING([whether to allow LADSPA])
  AC_ARG_WITH(ladspa,
    AC_HELP_STRING([--without-ladspa],[disable LADSPA and DSSI interfaces]),
      [if test "$withval" = "no"; then LADSPA_ENABLED="no"; DSSI_ENABLED="no"; fi],[])
  AC_MSG_RESULT($LADSPA_ENABLED)
fi

if test "$DSSI_FOUND" = "yes"; then
  AC_MSG_CHECKING([whether to allow DSSI])
  AC_ARG_WITH(dssi,
    AC_HELP_STRING([--without-dssi],[disable DSSI interface]),
      [if test "$withval" = "no"; then DSSI_ENABLED="no"; fi],[])
  AC_MSG_RESULT($DSSI_ENABLED)
fi

if test "$LV2_FOUND" = "yes"; then
  AC_MSG_CHECKING([whether to allow LV2])
  AC_ARG_WITH(lv2,
    AC_HELP_STRING([--without-lv2],[disable LV2 interface]),
      [if test "$withval" = "no"; then LV2_ENABLED="no"; fi],[])
  AC_MSG_RESULT($LV2_ENABLED)
fi

if test "$LASH_FOUND" = "yes"; then
  AC_MSG_CHECKING([whether to allow LASH])
  AC_ARG_WITH(lash,
    AC_HELP_STRING([--without-lash],[disable LASH compatibility (GPL)]),
      [if test "$withval" = "no"; then LASH_ENABLED="no"; fi],[])
  AC_MSG_RESULT($LASH_ENABLED)
fi

AC_MSG_CHECKING([whether to enable experimental/unfinished features])
AC_ARG_ENABLE(experimental,
  AC_HELP_STRING([--enable-experimental],[enable unfinished features - not recommended!]),
  [set_enable_experimental="$enableval"],
  [set_enable_experimental="no"])
AC_MSG_RESULT($set_enable_experimental)

AC_MSG_CHECKING([whether to enable old-style in-process GTK+ GUI])
AC_ARG_ENABLE(in-process-lv2-gui,
  AC_HELP_STRING([--enable-in-process-lv2-gui],[enable old-style in-process GTK+ GUI - not recommended!]),
  [set_enable_gtk_gui="$enableval"],
  [set_enable_gtk_gui="no"])
AC_MSG_RESULT($set_enable_gtk_gui)

AC_MSG_CHECKING([whether to enable debugging mode])
AC_ARG_ENABLE(debug,
  AC_HELP_STRING([--enable-debug],[enable debug mode - slow!]),
  [set_enable_debug="$enableval"],
  [set_enable_debug="no"])
AC_MSG_RESULT($set_enable_debug)

if test "$GUI_ENABLED" = "yes" -a "$JACK_FOUND" = "yes"; then
  AC_PATH_PROG(GCONFTOOL, gconftool-2)
  AM_GCONF_SOURCE_2
fi

############################################################################################
# Compute status shell variables

if test "$GUI_ENABLED" = "yes" -a "$JACK_FOUND" = "yes"; then
  JACK_ENABLED="yes"
fi

if test "$GUI_ENABLED" = "yes" -a "$DSSI_ENABLED" = "yes"; then
  DSSI_GUI_ENABLED="yes"
  EXEC_GUI_ENABLED="yes"
fi

if test "$GUI_ENABLED" = "yes" -a "$LV2_ENABLED" = "yes"; then
  LV2_GUI_ENABLED="yes"
  EXEC_GUI_ENABLED="yes"
fi

if test "$set_enable_gtk_gui" = "yes" -a "$LV2_ENABLED" = "yes" -a "$GUI_ENABLED" = "yes"; then
  LV2_GTK_GUI_ENABLED="yes"
fi

if test "$set_enable_debug" = "yes"; then
  CXXFLAGS="$CXXFLAGS -O0 -g -Wall"
else
  CXXFLAGS="$CXXFLAGS -O3 -finline-functions -finline-functions-called-once -Wall"
fi

############################################################################################
# Create automake conditional symbols
AM_CONDITIONAL(USE_DSSI, test "$DSSI_ENABLED" = "yes")
AM_CONDITIONAL(USE_LADSPA, test "$LADSPA_ENABLED" = "yes")
AM_CONDITIONAL(USE_JACK, test "$JACK_ENABLED" = "yes")
AM_CONDITIONAL(USE_LV2, test "$LV2_ENABLED" = "yes")
AM_CONDITIONAL(USE_GUI, test "$GUI_ENABLED" = "yes")
AM_CONDITIONAL(USE_DSSI_GUI, test "$DSSI_GUI_ENABLED" = "yes")
AM_CONDITIONAL(USE_EXEC_GUI, test "$EXEC_GUI_ENABLED" = "yes")
AM_CONDITIONAL(USE_LV2_GUI, test "$LV2_GUI_ENABLED" = "yes")
AM_CONDITIONAL(USE_LV2_GTK_GUI, test "$set_enable_gtk_gui" = "yes")
AM_CONDITIONAL(USE_LASH, test "$LASH_ENABLED" = "yes")
AM_CONDITIONAL(USE_LASH_0_6, test "$LASH_0_6_ENABLED" = "yes")
AM_CONDITIONAL(USE_DEBUG, test "$set_enable_debug" = "yes")

############################################################################################
# Create autoconf symbols for config.h
if test "$LADSPA_ENABLED" = "yes"; then
  AC_DEFINE(USE_LADSPA, 1, [LADSPA wrapper will be built])
fi
if test "$DSSI_ENABLED" = "yes"; then
  AC_DEFINE(USE_DSSI, 1, [DSSI wrapper will be built])
fi
if test "$LV2_ENABLED" = "yes"; then
  AC_DEFINE(USE_LV2, 1, [LV2 wrapper will be built])
fi
if test "$JACK_ENABLED" = "yes"; then
  AC_DEFINE(USE_JACK, 1, [JACK I/O will be used])
fi
if test "$OLD_JACK" = "yes"; then
  AC_DEFINE(OLD_JACK, 1, [Old JACK version (with extra nframes argument) is to be used])
fi
if test "$LASH_ENABLED" = "yes"; then
  AC_DEFINE(USE_LASH, 1, "LASH Audio Session Handler client functionality is enabled")
  if test "$LASH_0_6_FOUND" = "yes"; then
    AC_DEFINE(USE_LASH_0_6, 1, "Unstable LASH API is enabled")
  fi
fi
if test "$EXEC_GUI_ENABLED" = "yes"; then
  AC_DEFINE(USE_EXEC_GUI, 1, [Out-of-process GTK+ GUI for LV2/DSSI will be built])
fi
if test "$DSSI_GUI_ENABLED" = "yes"; then
  AC_DEFINE(USE_DSSI_GUI, 1, [GTK+ GUI executable will be used for DSSI])
fi
if test "$LV2_GUI_ENABLED" = "yes"; then
  AC_DEFINE(USE_LV2_GUI, 1, [GTK+ GUI executable will be used for LV2])
fi
if test "$set_enable_experimental" = "yes"; then
  AC_DEFINE([ENABLE_EXPERIMENTAL], [1], "Experimental features are enabled")
fi
if test "$set_enable_gtk_gui" = "yes"; then
  AC_DEFINE([USE_LV2_GTK_GUI], [1], "In-process GTK+ LV2 GUI features is enabled")
fi

############################################################################################
# Output directories
AC_MSG_CHECKING(where to install LADSPA plugins)
AC_ARG_WITH(ladspa_dir,
  AC_HELP_STRING([--with-ladspa-dir],[install LADSPA plugins to DIR (default=$prefix/lib/ladspa/)]),
  ,
  [with_ladspa_dir="$prefix/lib/ladspa/"])
AC_MSG_RESULT($with_ladspa_dir)
AC_SUBST(with_ladspa_dir)

AC_MSG_CHECKING(where to install LADSPA RDF file)
AC_ARG_WITH(ladspa_rdf_dir,
  AC_HELP_STRING([--with-ladspa-rdf-dir],[install RDF file to DIR (default=$prefix/share/ladspa/rdf/)]),
  ,
  [with_ladspa_rdf_dir="$prefix/share/ladspa/rdf/"])
AC_MSG_RESULT($with_ladspa_rdf_dir)
AC_SUBST(with_ladspa_rdf_dir)

AC_MSG_CHECKING(where to install DSSI plugins)
AC_ARG_WITH(dssi_dir,
  AC_HELP_STRING([--with-dssi-dir],[install DSSI plugins to DIR (default=$prefix/lib/dssi/)]),
  ,
  [with_dssi_dir="$prefix/lib/dssi/"])
AC_MSG_RESULT($with_dssi_dir)
AC_SUBST(with_dssi_dir)

if test "$LV2_ENABLED" == "yes"; then
  AC_MSG_CHECKING(where to install LV2 plugins)
  AC_ARG_WITH(lv2_dir,
    AC_HELP_STRING([--with-lv2-dir],[install LV2 calf.lv2 bundle to DIR (default=$prefix/lib/lv2/)]),
    ,
    [with_lv2_dir="$prefix/lib/lv2/"])
  AC_MSG_RESULT($with_lv2_dir)
  AC_SUBST(with_lv2_dir)
fi

############################################################################################
# Other defines
AC_DEFINE_UNQUOTED(PKGLIBDIR,"$prefix/share/calf/",[Calf shared data directory (bitmaps, GUI XML etc.)])

############################################################################################
# Output files for configure step
AC_CONFIG_FILES([Makefile
                 calf.desktop
                 gui/Makefile
                 gui/calf.rc
                 icons/Makefile
                 src/Makefile
                 src/calf/Makefile])

############################################################################################
# Configuration message
AC_MSG_RESULT([
    Calf configured

    Debug mode:                  $set_enable_debug
    Experimental plugins:        $set_enable_experimental
    LADSPA enabled:              $LADSPA_ENABLED
    Common GUI code:             $GUI_ENABLED
    Out-of-process GUI code:     $EXEC_GUI_ENABLED
    DSSI enabled:                $DSSI_ENABLED
    DSSI GUI enabled:            $DSSI_GUI_ENABLED
    LV2 enabled:                 $LV2_ENABLED
    LV2 in-process GUI enabled:  $LV2_GTK_GUI_ENABLED
    LV2 GUI enabled:             $LV2_GUI_ENABLED
    JACK host enabled:           $JACK_ENABLED
    LASH enabled:                $LASH_ENABLED])
if test "$LASH_ENABLED" = "yes"; then
  AC_MSG_RESULT([    Unstable LASH API:           $LASH_0_6_FOUND])
fi
AC_MSG_RESULT([    Old-style JACK MIDI:         $OLD_JACK
    
    Installation prefix:         $prefix
])

if test "$SLV2_UNSUPPORTED" == "yes" -a "$LV2_ENABLED" == "yes"; then 
    AC_MSG_RESULT([
WARNING:
An incompatible version of slv2 library has been found on this system.

Some features of Calf LV2 plugins (and other LV2 plugins) may not work
as expected in hosts that use currently installed version of libslv2.

Please upgrade to slv2 0.6.2 or newer version.
])
fi

AC_OUTPUT
