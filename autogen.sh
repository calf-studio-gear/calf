#!/bin/sh
aclocal
autoheader
automake --force-missing
autoconf
./configure --prefix=/usr $@
