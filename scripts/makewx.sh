#!/bin/bash
# makewx.sh 
# Usage: makewx.sh 
# -- Checks for current static release and debug builds of a library at $2 or ~/wxWidgets-3.0.2/
# -- If builds a found the script reports and finishes. If builds are not found the script
#    downloads, configures and builds the $1 specified "debug" or "release" or "debug release" 
#    or "release debug" versions of the wxWidgets library.
#    The builds are configured for static linking, monolithic, and without GTK+ Print support.
# -- Can test the use of checkinstall to build a debian package for our customized wxWidgets 
# -- Builds are configured using the --without-gtkprint flag to force the library to use 
#    Adobe PostScript 2.0 instead of PostScript 3.0 for printing to PostScript printers.
# Author: Bill Martin <bill_martin@sil.org>
# Date: 2017-01-28
# A sudo call should be avoided since this script is designed to be called from within CodeBlocks 
# as a Pre-build script where user interaction is not feasible.
# Requires an Internet connection to use wget if the wxWidgets archive is needed but doesn't 
# already exist, otherwise doesn't require Internet connection.

# The WXVER variable can be updated to a different version of wxWidgets
WXVER="3.0.2"
WXDIRNAME="wxWidgets-$WXVER"
RELEASEBUILDDIR="/buildgtku"
DEBUGBUILDDIR="/buildgtkud"
WXGZARCHIVE="wxWidgets-$WXVER.tar.bz2"
SCRIPTNAME="makewx.sh"
WX302URL="https://sourceforge.net/projects/wxwindows/files/$WXVER/wxWidgets-$WXVER.tar.bz2"
DEBUGCONFIGOPTIONS="--enable-static --disable-shared --enable-monolithic --without-gtkprint --enable-debug"
RELEASECONFIGOPTIONS="--enable-static --disable-shared --enable-monolithic --without-gtkprint"
BUILDS=""
ABORT="FALSE"
WXBASEDIR=$HOME  # Parameter $2 may change this location/path to other than $HOME

# The following variable definitions are unused except when testing checkinstall
MAINTAINER="bill_martin@sil.org"
SUMMARY="wxWidgets library configured --without-gtkprint"
NAME="wx"
VERSION=$WXVER
RELEASE="1"
LICENSE="wxWindows"
GROUP="checkinstall"
ARCHITECTURE="amd64"
SOURCELOCATION=$HOME$RELEASEBUILDDIR
ALTERNATESOURCELOCATION=""
REQUIRES=""
PROVIDES="buildgtku"
CONFLICTS="libwxgtk2.8-0,libwxgtk3.0-0"
REPLACES="libwxgtk2.8-0,libwxgtk3.0-0"

echo "--- The $SCRIPTNAME script has started ---"
case $# in
    0) 
      echo "$SCRIPTNAME was invoked without any parameters:"
      ABORT="TRUE"
        ;;
    1) 
      echo "$SCRIPTNAME was invoked with 1 parameter:"
      if [ x"$1" != x"debug" ] && [ x"$1" != x"release" ] && [ x"$1" != x"debug release" ] && [ x"$1" != x"release debug" ]; then
          echo "  Unrecognized first parameter $1 Use \"debug\" \"release\" or \"debug release\""
          ABORT="TRUE"
      else
          echo "  wxWidgets will be built for the $1 version only"
          echo "  wxWidgets will be built base path: $WXBASEDIR"
          BUILDS="$1"
      fi
        ;;
    2) 
      echo "$SCRIPTNAME was invoked with 2 parameters:"
      if [ x"$1" != x"debug" ] && [ x"$1" != x"release" ] && [ x"$1" != x"debug release" ] && [ x"$1" != x"release debug" ]; then
          echo "  Unrecognized first parameter $1 Use \"debug\" \"release\" or \"debug release\""
          ABORT="TRUE"
      else
        echo "  wxWidgets is specified to be built for the \"$1\" configuration(s)"
        # Abort unless path in $2 parameter is writeable
        ABORT="TRUE"
        WXBASEDIR=$2
        # The following 2 lines replace any initial ~/ on the $WXBASEDIR path received from $2
        result_string="${WXBASEDIR/~\//$HOME/}"
        WXBASEDIR=$result_string
        # The following 2 lines remove any final / on the WXBASEDIR path
        result_string=${WXBASEDIR%/}
        WXBASEDIR=$result_string
        # The following 2 lines remove any final /wxWidgets-3.0.2 on the WXBASEDIR path
        result_string=${WXBASEDIR%/$WXDIRNAME}
        WXBASEDIR=$result_string
        echo "  wxWidgets location base path is: $WXBASEDIR"
        #echo "WXBASEDIR is: $WXBASEDIR"
        mkdir -p $WXBASEDIR
        # The following touch ... line makes ABORT="FALSE" if $WXBASEDIR is writable
        # and if $WXBASEDIR is not writeable it suppresses the error message
        touch "${WXBASEDIR}/foo" 2>/dev/null && rm -f "${WXBASEDIR}/foo" && export ABORT="FALSE"
        #echo "ABORT is: $ABORT"
        if [ x"$ABORT" = x"TRUE" ]; then
            echo "***Cannot write to path: $WXBASEDIR"
        else
            echo "The base path: $WXBASEDIR appears to be writeable"
        fi
      fi
      BUILDS="$1"
        ;;
    *)
      echo "***Unrecognized or too many parameters used with script"
      ABORT="TRUE"
        ;;
esac

if [ x"$ABORT" = x"TRUE" ]; then
      echo "Usage: makewx.sh \"debug [release]\" [path/to/wxWidgets/source/tree]"
      echo "First parameter (required) can be \"debug\" \"release\" or \"debug release\""
      echo "Second parameter (optional) should be path to install wxWidgets source tree"
      echo "  which may be path to a host machine location via a VirtualBox shared folder."
      echo "  The path need not include the wxWidgets top-level folder which will"
      echo "  be $WXDIRNAME (but including it is a \$(#wxwin) global variable is OK)"
      echo "Example: bash makewx.sh \"debug\" \"/media/sf_bill\""
      echo "  will place source tree at: /media/sf_bill/$WXDIRNAME"
    exit 1
fi 
# If we get to here $ABORT is "FALSE"

#distCodename=`lsb_release -cs`
#case $distCodename in
#  "maya")
#  distCodename="precise"
#  ;;
#  "qiana")
#  distCodename="trusty"
#  ;;
#  "rebecca")
#  distCodename="trusty"
#  ;;
#  "rafaela")
#  distCodename="trusty"
#  ;;
#  "rosa")
#  distCodename="trusty"
#  ;;
#  "sarah")
#   distCodename="xenial"
#  ;;
#  "serena")
#  distCodename="xenial"
#  ;;
#esac
#echo "The Modified Codename for Deveopment is: $distCodename"

# Un-Comment the echo calls below when testing checkinstall
#echo "0 -  Maintainer: [ $MAINTAINER ]"
#echo "1 -  Summary: [ $SUMMARY ]"
#echo "2 -  Name:    [ $NAME ]"
#echo "3 -  Version: [ $VERSION ]"
#echo "4 -  Release: [ $RELEASE ]"
#echo "5 -  License: [ $LICENSE ]"
#echo "6 -  Group:   [ $GROUP ]"
#echo "7 -  Architecture: [ $ARCHITECTURE ]"
#echo "8 -  Source location: [ $SOURCELOCATION ]"
#echo "9 -  Alternate source location: [ $ALTERNATESOURCELOCATION ]"
#echo "10 - Requires: [ $REQUIRES ]"
#echo "11 - Provides: [ $PROVIDES ]"
#echo "12 - Conflicts: [ $CONFLICTS ]"
#echo "13 - Replaces: [ $REPLACES ]"

# Check for existing wxWidgets dir, download and builds at $WXBASEDIR
# Create $WXBASEDIR/$WXDIRNAME if it doesn't already exist
if ! [ -d $WXBASEDIR/$WXDIRNAME ]; then
  echo "Creating $WXDIRNAME folder at: $WXBASEDIR"
  mkdir -p $WXBASEDIR/$WXDIRNAME
  if [ $? -ne 0 ]; then
    echo "***Could not create the $WXBASEDIR/$WXDIRNAME directory."
    echo "Aborting..."
    exit 1
  fi 
else
  echo "Found the \"$WXDIRNAME\" directory at the base path"
fi 
SKIPRELEASEBUILD="FALSE"
SKIPDEBUGBUILD="FALSE"
WXARCHIVEFOUND="FALSE"
# If the static wx lib already exists for the current build (release or debug), 
# we can skip the configuring and build steps
# Check for a release version of the library libwx_gtk3u-3.0.a at:
# $WXBASEDIR/$WXDIRNAME$RELEASEBUILDDIR/lib/libwx_gtk3u-3.0.a
if [ -f $WXBASEDIR/$WXDIRNAME$RELEASEBUILDDIR/lib/libwx_gtk3u-3.0.a ]; then
  SKIPRELEASEBUILD="TRUE"
  if [ x"$1" = x"release" ] || [ x"$1" = x"debug release" ] || [ x"$1" = x"release debug" ]; then
    echo "  A release static build of the wx library already exists at the specified path"
    echo "    Nothing to be done for the release configuration!"
  fi
else
  SKIPRELEASEBUILD="FALSE"
  if [ x"$1" = x"release" ] || [ x"$1" = x"debug release" ] || [ x"$1" = x"release debug" ]; then
    echo "   ***A release static build of the wx library was not found there"
  fi
fi
# Check for a debug version of the library libwx_gtk3u-3.0.a at:
# $WXBASEDIR/$WXDIRNAME$DEBUGBUILDDIR/lib/libwx_gtk3u-3.0.a
if [ -f $WXBASEDIR/$WXDIRNAME$DEBUGBUILDDIR/lib/libwx_gtk3u-3.0.a ]; then
  SKIPDEBUGBUILD="TRUE"
  if [ x"$1" = x"debug" ] || [ x"$1" = x"debug release" ] || [ x"$1" = x"release debug" ]; then
    echo "  A debug static build of the wx library already exists at the specified path"
    echo "    Nothing to be done for the debug configuration!"
  fi
else
  SKIPDEBUGBUILD="FALSE"
  if [ x"$1" = x"debug" ] || [ x"$1" = x"debug release" ] || [ x"$1" = x"release debug" ]; then
    echo "   ***A debug static build of the wx library was not found there"
  fi
fi

if [ x"$SKIPDEBUGBUILD" = x"FALSE" ] || [ x"$SKIPRELEASEBUILD" = x"FALSE" ] ; then
  # A build will be necessary for either release or debug configuration.
  # Check for essential packages needed to build wxWidgets
  # The configure call will fail if the build tools are not installed,
  # so check if build-essential package is installed, and if not install it.
  echo "Checking for essential packages..."
  PkgInstalled=$(dpkg-query -W --showformat='${Status}\n' build-essential | grep "install ok installed")
  if [ "x$PkgInstalled" == x"" ]; then
    # The sudo command below should not be executed when the script is called from CodeBlocks
    # since it must execute without input from the user. 
    #echo -e "\nInstalling the build-essential package which is needed to build wxWidgets..."
    #sudo apt-get -y install build-essential
    echo "***You must install the build-essential package to build wxWidgets"
    echo "***Suggested command: sudo apt-get install build-essential"
    ABORT="TRUE"
  else
    echo "  The build-essential package is already installed"
  fi
  # The configure call will fail if the GTK+ development library is not installed,
  # so check if libgtk-3-dev package is installed, and if not install it.
  PkgInstalled=$(dpkg-query -W --showformat='${Status}\n' libgtk-3-dev | grep "install ok installed")
  if [ "x$PkgInstalled" == x"" ]; then
    # The sudo command below should not be executed when the script is called from CodeBlocks
    # since it must execute without input from the user. 
    #echo -e "\nInstalling the libgtk-3-dev package which is needed to build wxWidgets..."
    #sudo apt-get -y install libgtk-3-dev
    echo "***You must install the libgtk-3-dev package to build wxWidgets"
    echo "***Suggested command: sudo apt-get install libgtk-3-dev"
    ABORT="TRUE"
  else
    echo "  The libgtk-3-dev package is already installed"
  fi
  if [ x"$ABORT" = x"TRUE" ]; then
    exit 1
  fi 
  # If we get to here $ABORT is "FALSE"
fi

# Check for an existing copy of the $WXGZARCHIVE
if [ -f $WXBASEDIR/$WXGZARCHIVE ]; then
  echo "The $WXGZARCHIVE archive exists at: $WXBASEDIR"
  WXARCHIVEFOUND="TRUE"
else
  # The $WXARCHIVEFOUND archive was not found, so see if we need it for a build
  if [ x"$SKIPDEBUGBUILD" = x"FALSE" ] || [ x"$SKIPRELEASEBUILD" = x"FALSE" ] ; then
    # A build will be necessary for either release or debug configuration.
    # Need to download the $WXGZARCHIVE
    # Test for Internet connection
    echo -e "GET http://google.com HTTP/1.0\n\n" | nc google.com 80 > /dev/null 2>&1
    if [ $? -eq 0 ]; then
      echo "Internet connection detected."
    else
      echo "***No Internet connection detected. You must have an Internet connection"
      echo "to download the $WXGZARCHIVE archive. Aborting..."
      exit 1
    fi
    echo -e "\nRetrieving the $WXGZARCHIVE archive from:"
    echo    "   $WX302URL..."
    echo    "Downloading $WXGZARCHIVE to $WXBASEDIR"
    cd $WXBASEDIR
    wget --no-clobber --no-directories --quiet --show-progress $WX302URL
    if [ $? -ne 0 ]
    then
      echo "***Unable to download $WXGZARCHIVE: wget error: $?"
      exit 1
    fi
    echo "Expanding the $WXGZARCHIVE archive"
    # Extract wxWidgets from the .gz archive
    # can include v option to tar below for verbose output
    tar xjf $WXGZARCHIVE
  fi
fi

for i in $BUILDS; do
  if [ x"$i" = x"debug" ]; then
    if [ x"$SKIPDEBUGBUILD" = x"FALSE" ]; then
      echo -e "\n***************************************"
      echo      "**  Configuring the Debug version... **"
      echo      "***************************************"
      sleep 1
      if [ -d $WXBASEDIR/$WXDIRNAME$DEBUGBUILDDIR ]; then
        echo "   Removing stale $WXBASEDIR/$WXDIRNAME$DEBUGBUILDDIR dir"
        rm -rf $WXBASEDIR/$WXDIRNAME$DEBUGBUILDDIR
      fi
      mkdir -p $WXBASEDIR/$WXDIRNAME$DEBUGBUILDDIR
      cd $WXBASEDIR/$WXDIRNAME$DEBUGBUILDDIR
      echo -e "\nConfiguring the $WXBASEDIR/$WXDIRNAME$DEBUGBUILDDIR with options: "
      echo "   $DEBUGCONFIGOPTIONS"
      sleep 3
      ../configure $DEBUGCONFIGOPTIONS
      echo -e "\nClean the $WXBASEDIR/$WXDIRNAME$DEBUGBUILDDIR library..."
      sleep 3
      make clean
      echo -e "\n************************************"
      echo      "**  Building the Debug version... **"
      echo      "************************************"
      sleep 1
      make
    else
      echo "Using the currently existing static debug build..."
    fi
  fi
  
  if [ x"$i" = x"release" ]; then
    if [ x"$SKIPRELEASEBUILD" = x"FALSE" ]; then
      echo -e "\n*****************************************"
      echo      "**  Configuring the Release version... **"
      echo      "*****************************************"
      sleep 1
      if [ -d $WXBASEDIR/$WXDIRNAME$RELEASEBUILDDIR ]; then
        echo "   Removing stale $WXBASEDIR/$WXDIRNAME$RELEASEBUILDDIR dir"
        rm -rf $WXBASEDIR/$WXDIRNAME$RELEASEBUILDDIR
      fi
      mkdir -p $WXBASEDIR/$WXDIRNAME$RELEASEBUILDDIR
      cd $WXBASEDIR/$WXDIRNAME$RELEASEBUILDDIR
      echo -e "\nConfiguring the $WXBASEDIR/$WXDIRNAME$RELEASEBUILDDIR with options: "
      echo "   $RELEASECONFIGOPTIONS"
      sleep 3
      ../configure $RELEASECONFIGOPTIONS
      echo -e "\nClean the $WXBASEDIR/$WXDIRNAME$RELEASEBUILDDIR library..."
      sleep 3
      make clean
      echo -e "\n**************************************"
      echo      "**  Building the Release version... **"
      echo      "**************************************"
      sleep 1
      make
    else
      echo "Using the currently existing static release build..."
    fi
  fi  
done

echo "--- The $SCRIPTNAME script has completed ---"
# Comment out the exit 0 calls below to test checkinstall
exit 0

sudo checkinstall \
  --install=no \
  --type=debian \
  --pkgname=$NAME \
  --pkgversion=$VERSION \
  --pkgarch=$ARCHITECTURE \
  --pkgrelease=$RELEASE \
  --pkglicense=$LICENSE \
  --pkggroup=$GROUP \
  --pkgsource=$SOURCELOCATION \
  --pkgaltsource=$ALTERNATESOURCELOCATION \
  --maintainer=$MAINTAINER \
  --provides=$PROVIDES \
  --requires=$REQUIRES \
  --conflicts=$CONFLICTS \
  --replaces=$REPLACES

