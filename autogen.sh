#!/bin/sh
aclocal
libtoolize --force --automake --copy
autoheader --force
autoconf --force
automake --force-missing --copy
./configure --prefix=/usr --enable-experimental $@
