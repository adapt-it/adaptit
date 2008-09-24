#!/bin/sh
LIBTOOLIZE=libtoolize
if [ "`uname`" == "Darwin" ] ; then
   LIBTOOLIZE=glibtoolize
fi
aclocal
$LIBTOOLIZE --automake --force --copy
#libtoolize --automake --force --copy
automake -a -c
autoconf

