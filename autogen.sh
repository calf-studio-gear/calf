#!/bin/sh

if type libtoolize >/dev/null 2>&1
then
    LIBTOOLIZE=libtoolize
else
    if which glibtoolize >/dev/null
    then
	# on the Mac it's called glibtoolize for some reason
	LIBTOOLIZE=glibtoolize
    else
	echo "libtoolize not found"
	exit 1
    fi
fi

aclocal --force
${LIBTOOLIZE} --force --automake --copy
autoheader --force
autoconf --force
automake -a --copy
if test "$NOCONFIGURE" != 1; then
  ./configure --enable-experimental $@
fi
