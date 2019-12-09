#!/bin/sh
LIBTOOLIZE=libtoolize
if [ "`uname`" = "Darwin" ] ; then
   LIBTOOLIZE=glibtoolize
fi
# call aclocal to generate an m4 environment for autotools to use
aclocal -I m4
$LIBTOOLIZE --automake --force --copy
#libtoolize --automake --force --copy
# call automake to turn Makefile.am into a Makefile.in
automake -v -a -c -Wno-portability
# call autoconf to turn configure.ac into a configure script (which when invoked generates Makefile from Makefile.in)
autoconf

