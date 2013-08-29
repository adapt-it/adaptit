#!/bin/sh
LIBTOOLIZE=libtoolize
if [ "`uname`" = "Darwin" ] ; then
   LIBTOOLIZE=glibtoolize
fi
aclocal -verbose -I m4
$LIBTOOLIZE --automake --force --copy
#libtoolize --verbose --automake --force --copy
automake -v -a -c -Wno-portability
autoconf

