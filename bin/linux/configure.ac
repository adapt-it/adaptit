AC_INIT([adaptit],[0.1])
AC_CONFIG_SRCDIR([../../source/Adapt_It.cpp])
# whm 13Feb2021 added [subdir-objects] as parameter to AM_INIT_AUTOMAKE to avoid warning about subdir-objects
AM_INIT_AUTOMAKE([-Wno-portability] [subdir-objects])

AC_PROG_CXX
AC_PROG_INSTALL
#_LT_SET_OPTION([LT_INIT],[dlopen])
#AC_DIAGNOSE([obsolete],[AC_LIBTOOL_DLOPEN: Remove this warning and the call to _LT_SET_OPTION when you
#put the 'dlopen' option into LT_INIT's first parameter.])

LT_INIT([dlopen])

# Define _KBSERVER for all builds - whm added 11Dec2015
# Uncomment the following _KBSERVER define for version 7.x.x releases
#AC_DEFINE(_KBSERVER)

# Debug builds pass --debug to wx-config and define _DEBUG
AC_ARG_ENABLE([debug],
            [AS_HELP_STRING([--enable-debug],
              [enable debug build using debug wxwidgets libraries])],
            [],
            [enable_debug=no])

WXDEBUG=
AS_IF([test "x$enable_debug" != xno],
    [AC_SUBST([WXDEBUG], ["--debug"])
     AC_DEFINE([_DEBUG], [1],
               [Define for debug builds])
#     AC_DEFINE([_KBSERVER], [1],
#               [Define for KBServer builds])
    ])

CPPFLAGS="$CPPFLAGS -Wall -Wno-sign-compare -Wno-write-strings -Wno-unused-local-typedefs -Wno-sizeof-pointer-memaccess -g -fexceptions"
CXXFLAGS="$CPPFLAGS -Wall -Wno-sign-compare -Wno-write-strings -Wno-unused-local-typedefs -Wno-sizeof-pointer-memaccess -g -fexceptions"

WXCONFIG=wx-config
AC_ARG_WITH(wx-config,
[[  --with-wx-config=FILE     Use the given path to wx-config when determining
                            wxWidgets configuration; defaults to "wx-config"]],
[
    if test "$withval" != "yes" -a "$withval" != ""; then
        WXCONFIG=$withval
    fi
])

wxversion=0

AC_DEFUN([WXTEST],
[
	AC_REQUIRE([AC_PROG_AWK])
	AC_MSG_CHECKING([wxWidgets version])
	if wxversion=`$WXCONFIG --version`; then
		AC_MSG_RESULT([$wxversion])
	else
		AC_MSG_RESULT([not found])
		AC_MSG_ERROR([wxWidgets is required. Try --with-wx-config.])
	fi])

# Call WXTEST func
WXTEST

# Verify minimus requires
vers=`echo $wxversion | $AWK 'BEGIN { FS = "."; } { printf "% d", ($1 * 1000 + $2) * 1000 + $3;}'`
if test -n "$vers" && test "$vers" -ge 2003003; then
	WX_CPPFLAGS="`$WXCONFIG --cppflags $WXDEBUG`"
	WX_CXXFLAGS="`$WXCONFIG --cxxflags $WXDEBUG | sed -e 's/-fno-exceptions//'`"
	WX_LIBS="`$WXCONFIG --libs std,aui $WXDEBUG`"
else
	AC_MSG_ERROR([wxWidgets 2.3.3 or newer is required])
fi


CPPFLAGS="$CPPFLAGS $WX_CPPFLAGS"
CXXFLAGS="$CXXFLAGS $WX_CPPFLAGS"

AC_SUBST(WX_LIBS)

AM_GNU_GETTEXT([external])


# libcurl
CURL_LIBS="`curl-config --libs`"
AC_SUBST(CURL_LIBS)

AC_CONFIG_FILES([Makefile po/Makefile.in])
AC_OUTPUT
