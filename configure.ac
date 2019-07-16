#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.63)
AC_INIT([calf],[0.90.3],[wdev@foltman.com])
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
LV2_GUI_ENABLED="no"
JACK_FOUND="no"
JACK_ENABLED="no"
OLD_JACK="no"
GUI_ENABLED="no"
LASH_ENABLED="no"
OBS_CHECK_ENABLED="yes"

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
AC_SUBST(LV2_DEPS_CFLAGS)
AC_SUBST(LV2_DEPS_LIBS)

############################################################################################
# Detect dependencies

# Checks for libraries.
PKG_PROG_PKG_CONFIG

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([memory.h stdint.h stdlib.h string.h time.h math.h])

AC_CHECK_HEADER(expat.h, true, AC_MSG_ERROR([Expat XML library not found]))
AC_CHECK_LIB(expat, XML_Parse, true, AC_MSG_ERROR([Expat XML library not found]))

# JACK
AC_CHECK_HEADERS([jack/jack.h], have_jack_header=yes, have_jack_header=no)

# GLib
PKG_CHECK_MODULES(GLIB_DEPS, glib-2.0 >= 2.0.0 gthread-2.0 >= 2.0.0, true, AC_MSG_ERROR([Calf requires glib2 and gthread2]))

# Fluidsynth
PKG_CHECK_MODULES(FLUIDSYNTH_DEPS, fluidsynth >= 1.0.7, true, AC_MSG_ERROR([fluidsynth library not found]))

# Sordi
AC_CHECK_PROG(SORDI_ENABLED, sordi, yes, no)

if test "$have_jack_header" = "yes"; then
  PKG_CHECK_MODULES(JACK_DEPS, jack >= 0.103.0,
    AC_CHECK_LIB([jack], [jack_port_register], JACK_FOUND="yes", ),)
fi

if test "$JACK_FOUND" = "yes"; then
  PKG_CHECK_MODULES(JACK_MIDI_DEPS, jack >= 0.105.0, OLD_JACK="no", OLD_JACK="yes")
fi
if test "$JACK_FOUND" = "yes"; then
  PKG_CHECK_MODULES(JACK_RENAME_PORT, jack >= 0.124.2 jack < 1.9.0, JACK_HAS_RENAME="yes", JACK_HAS_RENAME="no")
  PKG_CHECK_MODULES(JACK_RENAME_PORT, jack >= 1.9.11, JACK_HAS_RENAME="yes", JACK_HAS_RENAME_DUMMY="no")
fi

PKG_CHECK_MODULES(LV2_DEPS, lv2 >= 1.1.14, LV2_FOUND="yes", LV2_FOUND="no")

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

LV2_ENABLED=$LV2_FOUND
LASH_ENABLED=$LASH_FOUND

if test "$JACK_FOUND" = "yes" -o "$LV2_FOUND" = "yes"; then
  PKG_CHECK_MODULES(GUI_DEPS, gtk+-2.0 >= 2.12.0 cairo >= 1.2.0,
    GUI_ENABLED="yes",
    GUI_ENABLED="no (GTK+ 2.12 and cairo 1.2 or newer required)"
  )
fi

############################################################################################
# Look for settings and installed libraries to determine what to compile/install/use
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

AC_MSG_CHECKING([whether to check for old LADSPA plugin])
AC_ARG_WITH(obsolete-check,
AC_HELP_STRING([--without-obsolete-check],[do not check for obsolete files]),
  [if test "$withval" = "no"; then OBS_CHECK_ENABLED="no"; fi],[])
AC_MSG_RESULT($OBS_CHECK_ENABLED)


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

AC_MSG_CHECKING([whether to compile with SSE])
AC_ARG_ENABLE(sse,
  AC_HELP_STRING([--enable-sse],[compile with SSE extensions]),
  [set_enable_sse="$enableval"],
  [set_enable_sse="no"])
AC_MSG_RESULT($set_enable_sse)

############################################################################################
# Compute status shell variables

if test "$GUI_ENABLED" = "yes" -a "$JACK_FOUND" = "yes"; then
  JACK_ENABLED="yes"
fi

if test "$GUI_ENABLED" = "yes" -a "$LV2_ENABLED" = "yes"; then
  LV2_GUI_ENABLED="yes"
fi

if test "$set_enable_debug" = "yes"; then
  CXXFLAGS="$CXXFLAGS -O0 -g -Wall"
else
  # TODO: remove -finline options if clang is used
  CXXFLAGS="$CXXFLAGS -O3 -finline-functions -finline-functions-called-once -Wall"
fi

if test "$set_enable_sse" = "yes"; then
  CXXFLAGS="$CXXFLAGS -msse -mfpmath=sse"
fi

############################################################################################
# Create automake conditional symbols
AM_CONDITIONAL(USE_JACK, test "$JACK_ENABLED" = "yes")
AM_CONDITIONAL(USE_LV2, test "$LV2_ENABLED" = "yes")
AM_CONDITIONAL(USE_GUI, test "$GUI_ENABLED" = "yes")
AM_CONDITIONAL(USE_LV2_GUI, test "$LV2_GUI_ENABLED" = "yes")
AM_CONDITIONAL(USE_LASH, test "$LASH_ENABLED" = "yes")
AM_CONDITIONAL(USE_LASH_0_6, test "$LASH_0_6_ENABLED" = "yes")
AM_CONDITIONAL(USE_DEBUG, test "$set_enable_debug" = "yes")
AM_CONDITIONAL(USE_SORDI, test "$SORDI_ENABLED" = "yes")

############################################################################################
# Create autoconf symbols for config.h
if test "$LV2_ENABLED" = "yes"; then
  AC_DEFINE(USE_LV2, 1, [LV2 wrapper will be built])
fi
if test "$JACK_ENABLED" = "yes"; then
  AC_DEFINE(USE_JACK, 1, [JACK I/O will be used])
fi
if test "$OLD_JACK" = "yes"; then
  AC_DEFINE(OLD_JACK, 1, [Old JACK version (with extra nframes argument) is to be used])
fi
if test "$JACK_HAS_RENAME" = "yes"; then
  AC_DEFINE(JACK_HAS_RENAME, 1, [JACK function jack_port_rename should be used instead of jack_port_set_name])
fi
if test "$LASH_ENABLED" = "yes"; then
  AC_DEFINE(USE_LASH, 1, [LASH Audio Session Handler client functionality is enabled])
  if test "$LASH_0_6_FOUND" = "yes"; then
    AC_DEFINE(USE_LASH_0_6, 1, [Unstable LASH API is enabled])
  fi
fi
if test "$LV2_GUI_ENABLED" = "yes"; then
  AC_DEFINE(USE_LV2_GUI, 1, [GTK+ GUI executable will be used for LV2])
fi
if test "$set_enable_experimental" = "yes"; then
  AC_DEFINE([ENABLE_EXPERIMENTAL], [1], [Experimental features are enabled])
fi
if test "$SORDI_ENABLED" = "yes"; then
  AC_DEFINE(USE_SORDI, 1, [Sordi sanity checks are enabled])
fi
############################################################################################
# Output directories
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
AC_DEFINE_UNQUOTED(PKGDOCDIR,"$prefix/share/doc/calf/",[Calf documentation directory])


############################################################################################

# BASH completion

AC_ARG_WITH([bash-completion-dir],
    AS_HELP_STRING([--with-bash-completion-dir[=PATH]],
        [Install the bash auto-completion script in this directory. @<:@default=yes@:>@]),
    [],
    [with_bash_completion_dir=yes])
    
if test "x$with_bash_completion_dir" = "xyes"; then
    PKG_CHECK_MODULES([BASH_COMPLETION], [bash-completion >= 2.0],
        [BASH_COMPLETION_DIR="`pkg-config --define-variable=prefix=$prefix --variable=completionsdir bash-completion`"],
        [BASH_COMPLETION_DIR="$datadir/bash-completion/completions"])
else
    BASH_COMPLETION_DIR="$with_bash_completion_dir"
fi
AC_SUBST([BASH_COMPLETION_DIR])
AM_CONDITIONAL([ENABLE_BASH_COMPLETION],[test "x$with_bash_completion_dir" != "xno"])

############################################################################################
# Output files for configure step
AC_CONFIG_FILES([Makefile
                 calf.desktop
                 gui/Makefile
                 gui/icons/LV2/Makefile
                 icons/Makefile
                 src/Makefile
                 src/calf/Makefile])

############################################################################################
# Configuration message
AC_MSG_RESULT([
    Calf configured

    Debug mode:                  $set_enable_debug
    With SSE:                    $set_enable_sse
    Experimental plugins:        $set_enable_experimental
    Common GUI code:             $GUI_ENABLED
    LV2 enabled:                 $LV2_ENABLED
    LV2 GTK+ GUI enabled:        $LV2_GUI_ENABLED
    JACK host enabled:           $JACK_ENABLED
    LASH enabled:                $LASH_ENABLED])
if test "$LASH_ENABLED" = "yes"; then
  AC_MSG_RESULT([    Unstable LASH API:           $LASH_0_6_FOUND])
fi
AC_MSG_RESULT([    Old-style JACK MIDI:         $OLD_JACK
    JACK has jack_port_rename:   $JACK_HAS_RENAME
    
    Installation prefix:         $prefix
    
])

if test "$OBS_CHECK_ENABLED" == "yes"; then
for fn in $prefix/lib/ladspa /usr/lib/ladspa /usr/local/lib/ladspa `echo $LADSPA_PATH | tr : "\n"` ; do
    fn2=$fn/calf.so
    if test -a $fn2; then
        AC_MSG_ERROR([

Obsolete LADSPA version of the Calf plugins has been detected:

    $fn2

It will interfere with the current version. Please remove the file and try
again.
])
    fi
done
fnr=`dirname $prefix/lib/lv2/calf.lv2/calf.so`
for fn in /usr/lib/lv2 /usr/local/lib/lv2 `echo $LV2_PATH | tr : "\n"` ; do
    fn2=$fn/calf.lv2/calf.so
    echo "Checking $fn2"
    if test -a $fn2 -a "`dirname $fn2`" != "$fnr"; then
        AC_MSG_ERROR([

Another version of the Calf plugins has been detected:

    $fn2

It will interfere with the current version. Please remove the old version
from $fn/calf.lv2 and try again.
    ])
    fi
done
fi

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
