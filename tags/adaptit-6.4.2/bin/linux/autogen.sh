#!/bin/sh
LIBTOOLIZE=libtoolize
if [ "`uname`" = "Darwin" ] ; then
   LIBTOOLIZE=glibtoolize
fi
aclocal -I m4
$LIBTOOLIZE --automake --force --copy
#libtoolize --automake --force --copy
automake -a -c -Wno-portability
autoconf

