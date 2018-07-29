#!/bin/sh
# whm 29Jul2018 replaced old autogen.sh with single line autoreconf -fi below:
# autoreconf options: -f force creation of all autotool helpers, -i copy missing ones
# Output versions
autoreconf --version
aclocal --version
libtoolize --version
automake --version
autoconf --version
gettext --version
autoreconf -fi

# Original autogen.sh commands below:
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

