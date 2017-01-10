#!/bin/bash
# makewx.sh 
# -- Downloads, configures and builds the Debug and Release versions of the wxWidgets library
#    The wxWidgets builds are for static linking, monolithic, and without GTK+ Print support.
# -- Uses checkinstall to build a debian package for our customized wxWidgets 
# library that was built using the --without-gtkprint flag to force the library to use 
# Adobe PostScript 2.0 instead of PostScript 3.0 for printing to PostScript printers.
# Author: Bill Martin <bill_martin@sil.org>
# Date: 2017-01-09

WXVER="3.0.2"
WXDIR="$HOME/wxWidgets-$WXVER"
RELEASEBUILDDIR="/buildgtku"
DEBUGBUILDDIR="/buildgtkud"
WXGZARCHIVE="wxWidgets-$WXVER.tar.bz2"
SCRIPTNAME="makewx.sh"
WX302URL="https://sourceforge.net/projects/wxwindows/files/$WXVER/wxWidgets-$WXVER.tar.bz2"
DEBUGCONFIGOPTIONS="--enable-static --disable-shared --enable-monolithic --without-gtkprint --enable-debug"
RELEASECONFIGOPTIONS="--enable-static --disable-shared --enable-monolithic --without-gtkprint"

MAINTAINER="bill_martin@sil.org"
SUMMARY="wxWidgets library configured --without-gtkprint"
NAME="wx"
VERSION=$WXVER
RELEASE="1"
LICENSE="wxWindows"
GROUP="checkinstall"
ARCHITECTURE="amd64"
SOURCELOCATION=$WXDIR$RELEASEBUILDDIR
ALTERNATESOURCELOCATION=""
REQUIRES=""
PROVIDES="buildgtku"
CONFLICTS="libwxgtk2.8-0,libwxgtk3.0-0"
REPLACES="libwxgtk2.8-0,libwxgtk3.0-0"
BUILDS=""

case $# in
    0) 
      echo "$SCRIPTNAME was invoked without any parameters:"
      echo "  wxWidgets will be built for both debug and release versions"
      BUILDS="debug release"
        ;;
    1) 
      echo "$SCRIPTNAME was invoked with 1 parameter:"
      echo "  wxWidgets will be built for the $1 version only"
      BUILDS="$1"
        ;;
    2) 
      echo "$SCRIPTNAME was invoked with 2 parameters:"
      echo "  wxWidgets will be built for both $1 and $2 versions"
      BUILDS="$1 $2"
        ;;
    *)
      echo "Unrecognized parameters used with script."
      echo "  wxWidgets will be built for both debug and release versions"
      BUILDS="debug release"
        ;;
esac


distCodename=`lsb_release -cs`
case $distCodename in
  "maya")
  distCodename="precise"
  ;;
  "qiana")
  distCodename="trusty"
  ;;
  "rebecca")
  distCodename="trusty"
  ;;
  "rafaela")
  distCodename="trusty"
  ;;
  "rosa")
  distCodename="trusty"
  ;;
  "sarah")
  distCodename="xenial"
  ;;
  "serena")
  distCodename="xenial"
  ;;
esac
echo -e "\nThe Modified Codename for Deveopment is: $distCodename"

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

cd

if [ -f $HOME/$WXGZARCHIVE ]; then
  echo -e "\nFound wxWidgets-3.0.2.tar.bz2 already in $HOME so will use it"
  echo -e "\nExpanding the $WXGZARCHIVE archive"
  sleep 2
else
  echo -e "\nRetrieving the $WXGZARCHIVE archive from $WX302URL..."
  wget --no-clobber --no-directories $WX302URL
  echo -e "\nExpanding the $WXGZARCHIVE archive"
  sleep 2
fi

# can include v option to tar below for verbose output
tar xjf $WXGZARCHIVE

if [ -d $WXDIR ]; then
  echo -e "\nFound $WXDIR/ directory"
  for i in $BUILDS; do
    if [ x"$i" = x"debug" ]; then
      if [ -d $WXDIR$DEBUGBUILDDIR ]; then
        echo "   Removing stale $WXDIR$DEBUGBUILDDIR dir"
        rm -rf $WXDIR$DEBUGBUILDDIR
      fi
    fi  
    if [ x"$i" = x"release" ]; then
      if [ -d $WXDIR$RELEASEBUILDDIR ]; then
        echo "   Removing stale $WXDIR$RELEASEBUILDDIR dir"
        rm -rf $WXDIR$RELEASEBUILDDIR
      fi
    fi  
  done
else
  echo "Could not find the $WXDIR directory. Aborting..."
  exit 1
fi

for i in $BUILDS; do
  if [ x"$i" = x"debug" ]; then
    mkdir -p $WXDIR$DEBUGBUILDDIR
    cd $WXDIR$DEBUGBUILDDIR
    echo -e "\nConfiguring the $WXDIR$DEBUGBUILDDIR build using config options: "
    echo "   $DEBUGCONFIGOPTIONS"
    sleep 3
    ../configure $DEBUGCONFIGOPTIONS

    echo -e "\nClean and Make the $WXDIR$DEBUGBUILDDIR library. Please wait..."
    sleep 3
    make clean
    make
  fi

  if [ x"$i" = x"release" ]; then
    mkdir -p $WXDIR$RELEASEBUILDDIR
    cd $WXDIR$RELEASEBUILDDIR
    echo -e "\nConfiguring the $WXDIR$RELEASEBUILDDIR build using config options: "
    echo "   $RELEASECONFIGOPTIONS"
    sleep 3
    ../configure $RELEASECONFIGOPTIONS

    echo -e "\nClean and Make the $WXDIR$RELEASEBUILDDIR library. Please wait..."
    sleep 3
    make clean
    make
  fi
done

echo "Debug break"
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

