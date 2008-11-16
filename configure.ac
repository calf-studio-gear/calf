#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT([calf],[0.0.17],[wdev@foltman.com])
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADER([config.h])

AM_INIT_AUTOMAKE(1.8)

if test "x$prefix" = "xNONE"; then 
  prefix=$ac_default_prefix
fi

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LIBTOOL

# Checks for libraries.
PKG_PROG_PKG_CONFIG

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([memory.h stdint.h stdlib.h time.h math.h])

AC_CHECK_HEADER(ladspa.h, LADSPA_FOUND="yes", LADSPA_FOUND="no")

AC_CHECK_HEADER(dssi.h, DSSI_FOUND="yes", DSSI_FOUND="no")

AC_CHECK_HEADER(expat.h, true, AC_MSG_ERROR([Expat XML library not found]))
AC_CHECK_LIB(expat, XML_Parse, true, AC_MSG_ERROR([Expat XML library not found]))

AC_CHECK_HEADERS([jack/jack.h], have_jack_header=yes, have_jack_header=no)

PKG_CHECK_MODULES(GLIB_DEPS, glib-2.0 >= 2.0.0, true, AC_MSG_ERROR([glib-2.0 library not found]))

if test "$have_jack_header" = "yes"; then
  PKG_CHECK_MODULES(JACK_DEPS, jack >= 0.103.0,
    AC_CHECK_LIB([jack], [jack_port_register], JACK_FOUND="yes", JACK_FOUND="no"),
    JACK_FOUND="no")
else
  JACK_FOUND="no"
fi

if test "$JACK_FOUND" = "yes"; then
  PKG_CHECK_MODULES(JACK_MIDI_DEPS, jack >= 0.105.0, OLD_JACK="no", OLD_JACK="yes")
fi

AC_SUBST(GLIB_DEPS_CFLAGS)
AC_SUBST(GLIB_DEPS_LIBS)
AC_SUBST(GUI_DEPS_CFLAGS)
AC_SUBST(GUI_DEPS_LIBS)
AC_SUBST(JACK_DEPS_CFLAGS)
AC_SUBST(JACK_DEPS_LIBS)

PKG_CHECK_MODULES(LV2_DEPS, lv2core >= 1, LV2_ENABLED="yes", LV2_ENABLED="no")
PKG_CHECK_MODULES(LASH_DEPS, lash-1.0 >= 0.5.2, LASH_ENABLED="yes", LASH_ENABLED="no")

if test "$JACK_FOUND" = "yes" -o "$DSSI_FOUND" = "yes" -o "$LV2_ENABLED" = "yes"; then
  PKG_CHECK_MODULES(GUI_DEPS, gtk+-2.0 >= 2.8.0 libglade-2.0 >= 2.4.0 cairo >= 1.2.0,
    GUI_ENABLED="yes",
    GUI_ENABLED="no (GTK+ 2.8, cairo 1.2 and libglade 2.4.0 or newer required)"
  )
else
  GUI_ENABLED="no"
fi

if test "$GUI_ENABLED" = "yes" -a "$JACK_FOUND" = "yes"; then
  JACK_ENABLED="yes"
else
  JACK_ENABLED="no"
fi

if test "$GUI_ENABLED" = "yes" -a "$DSSI_FOUND" = "yes"; then
  DSSI_GUI_ENABLED="yes"
else
  DSSI_GUI_ENABLED="no"
fi

if test "$GUI_ENABLED" = "yes" -a "$LV2_ENABLED" = "yes"; then
  LV2_GUI_ENABLED="yes"
else
  LV2_GUI_ENABLED="no"
fi

AC_MSG_CHECKING([whether to enable experimental/unfinished features])
AC_ARG_ENABLE(experimental,
  AC_HELP_STRING([--enable-experimental],[enable unfinished features - not recommended!]),
  [set_enable_experimental="$enableval"],
  [set_enable_experimental="no"])
AC_MSG_RESULT($set_enable_experimental)

AC_MSG_CHECKING([whether to enable debugging mode])
AC_ARG_ENABLE(debug,
  AC_HELP_STRING([--enable-debug],[enable debug mode - slow!]),
  [set_enable_debug="$enableval"],
  [set_enable_debug="no"])
AC_MSG_RESULT($set_enable_debug)

if test "$set_enable_debug" = "yes"; then
  CXXFLAGS="$CXXFLAGS -O0 -g -Wall"
else
  CXXFLAGS="$CXXFLAGS -O3 -finline-functions -finline-functions-called-once -Wall"
fi

AM_CONDITIONAL(USE_DSSI, test "$DSSI_FOUND" = "yes")
AM_CONDITIONAL(USE_LADSPA, test "$LADSPA_FOUND" = "yes")
AM_CONDITIONAL(USE_JACK, test "$JACK_ENABLED" = "yes")
AM_CONDITIONAL(USE_LV2, test "$LV2_ENABLED" = "yes")
AM_CONDITIONAL(USE_GUI, test "$GUI_ENABLED" = "yes")
AM_CONDITIONAL(USE_DSSI_GUI, test "$DSSI_GUI_ENABLED" = "yes")
AM_CONDITIONAL(USE_LV2_GUI, test "$LV2_GUI_ENABLED" = "yes")
AM_CONDITIONAL(USE_LASH, test "$LASH_ENABLED" = "yes")
AM_CONDITIONAL(USE_DEBUG, test "$set_enable_debug" = "yes")
if test "$LADSPA_FOUND" = "yes"; then
  AC_DEFINE([USE_LADSPA], [1], [LADSPA wrapper will be built])
fi
if test "$DSSI_FOUND" = "yes"; then
  AC_DEFINE(USE_DSSI, 1, [DSSI wrapper will be built])
fi
if test "$DSSI_GUI_ENABLED" = "yes"; then
  AC_DEFINE(USE_DSSI_GUI, 1, [GTK+ GUI for DSSI  will be built])
fi
if test "$LV2_ENABLED" = "yes"; then
  AC_DEFINE(USE_LV2, 1, [LV2 wrapper will be built])
fi
if test "$LV2_GUI_ENABLED" = "yes"; then
  AC_DEFINE(USE_LV2_GUI, 1, [GTK+ GUI for LV2 plugins will be built])
fi
if test "$JACK_ENABLED" = "yes"; then
  AC_DEFINE(USE_JACK, 1, [JACK I/O will be used])
fi
if test "$OLD_JACK" = "yes"; then
  AC_DEFINE(OLD_JACK, 1, [Old JACK version (with extra nframes argument) is to be used])
fi
if test "$LASH_ENABLED" = "yes"; then
  AC_DEFINE(USE_LASH, 1, "LASH Audio Session Handler client functionality is enabled")
fi
if test "$set_enable_experimental" = "yes"; then
  AC_DEFINE([ENABLE_EXPERIMENTAL], [1], "Experimental features are enabled")
fi

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_C_INLINE
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_INT8_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

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

# Checks for library functions.
AC_CHECK_FUNCS([floor memset pow])

AC_CONFIG_FILES([Makefile
                 calf.desktop
                 src/Makefile
                 src/calf/Makefile])
AC_MSG_RESULT([
    Calf configured

    Debug mode:             $set_enable_debug
    LADSPA enabled:         $LADSPA_FOUND
    DSSI enabled:           $DSSI_FOUND
    DSSI GUI enabled:       $DSSI_GUI_ENABLED
    LV2 enabled:            $LV2_ENABLED
    LV2 GUI enabled:        $LV2_GUI_ENABLED
    JACK host enabled:      $JACK_ENABLED
    LASH enabled:           $LASH_ENABLED
    Old-style JACK MIDI:    $OLD_JACK
    
    Installation prefix:    $prefix
])
AC_OUTPUT
