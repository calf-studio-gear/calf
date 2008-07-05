#!/bin/sh
aclocal
libtoolize --force --automake --copy
autoheader --force
autoconf --force
automake -a --copy
if test "$NOCONFIGURE" != 1; then
  ./configure --enable-experimental $@
fi
