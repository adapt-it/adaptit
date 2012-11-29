#!/bin/bash
# build-ai.sh -- extracts 3pt libs and builds AdaptIt on Ubuntu
# Currently this only builds the UnicodeDebug release against wx2.9.4,
# but the commented out blocks will build the Unicode Release target.

DIR=$( cd "$( dirname "$0" )" && pwd )
TRUNK=$DIR/../
VENDOR=$DIR/../../vendor
WXGTK=$VENDOR/wxwidgets/current/wxWidgets

# check for 7za
if ! type "7za" > /dev/null; then
  sudo apt-get -y install p7zip-full
fi
# check for dos2unix
if ! type "dos2unix" > /dev/null; then
  sudo apt-get -y install dos2unix
fi
# some distros use tofrodos - install it if the dos2unix package isn't there
# and create a symlink (alias) so we can use dos2unix
if ! type "dos2unix" > /dev/null; then
    sudo apt-get -y install tofrodos
    sudo ln -s fromdos dos2unix
fi

# Extract wxWidgets from the vendor branch
7za x -o$WXGTK -y $VENDOR/wxwidgets/current/wxWidgets-2.9.4.7z
#7za x -o$VENDOR/wxwidgets/current -y $VENDOR/wxwidgets/current/wxWidgets-2.9.4.tar
# change the line endings from Windows (CRLF) to Linux
find $WXGTK -type f -exec dos2unix {} \; 
# copy over the appropriate setup.h so wxwidgets knows which platform to build
cp -f $WXGTK/include/wx/gtk/setup.h $WXGTK/include/wx/setup.h
# allow a couple wxwidgets scripts to execute
chmod 755 $WXGTK/configure
chmod 755 $WXGTK/src/stc/gen_iface.py

# Build wxWidgets (debug)
cd $WXGTK
mkdir -p buildgtkud
# release --> requires buildgtku directory to be made
# (cd $WXGTK/buildgtku && $WXGTK/configure --with-gtk --enable-unicode --with-gnomeprint && make)
#if [ $? -ne 0 ]
#then
#  echo "Error building wxWidgets Unicode Release: $?"
#  exit $?
#fi
(cd $WXGTK/buildgtkud && $WXGTK/configure --with-gtk --enable-unicode --enable-debug --with-gnomeprint && make)
if [ $? -ne 0 ]
then
  echo "Error building wxWidgets Unicode Debug: $?"
  exit $?
fi

# Configure svn in adaptit-standard way
# (just in case it isn't already set up like this)
sed -i -e 's/^# enable-auto-props = yes/enable-auto-props = yes/' ~/.subversion/config
for x in c cpp h
do 
  sed -i -e "s/^# \*\.$x = .*$/*.$x = svn:eol-style=CRLF/" ~/.subversion/config
done

# Build adaptit (debug)
cd $TRUNK/bin/linux/
mkdir -p UnicodeDebug
#(cd $TRUNK/bin/linux/Unicode && ../configure --with-wx-config=$WXGTK/buildgtku/wx-config && make)
#if [ $? -ne 0 ]
#then
#  echo "Error building Adapt It Unicode Release: $?"
#  exit $?
#fi
(cd $TRUNK/bin/linux/UnicodeDebug && ../configure --with-wx-config=$WXGTK/buildgtkud/wx-config && make)
if [ $? -ne 0 ]
then
  echo "Error building Adapt It Unicode Debug: $?"
  exit $?
fi

