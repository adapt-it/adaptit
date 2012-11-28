#!/bin/bash
# build-ai.sh -- extracts 3pt libs and builds AdaptIt on Ubuntu

DIR=$( cd "$( dirname "$0" )" && pwd )
VENDOR=$DIR/../../vendor
TRUNK=$DIR/../

# Extract wxWidgets from the vendor branch
7za x -o$VENDOR/wxwidgets/current -y $VENDOR/wxwidgets/current/wxWidgets-2.9.4.tar.bz2
7za x -o$VENDOR/wxwidgets/current -y $VENDOR/wxwidgets/current/wxWidgets-2.9.4.tar
# change the line endings from Windows (CRLF) to Linux
# find . -path ~/svn/vendor/wxwidgets/current/wxWidgets -exec dos2unix {} \; 
cp -f $VENDOR/wxwidgets/current/wxWidgets-2.9.4/include/wx/gtk/setup.h $VENDOR/wxwidgets/current/wxWidgets-2.9.4/include/wx/setup.h
chmod 777 $VENDOR/wxwidgets/current/wxWidgets-2.9.4/configure

# Build wxWidgets (debug and release)
cd $VENDOR/wxwidgets/current/wxWidgets-2.9.4
mkdir -p buildgtku buildgtkud
(cd $VENDOR/wxwidgets/current/wxWidgets-2.9.4/buildgtku && $VENDOR/wxwidgets/current/wxWidgets-2.9.4/configure --with-gtk --enable-unicode --with-gnomeprint && make)
if [ $? -ne 0 ]
then
  echo "Error building wxWidgets Unicode Release: $?"
  exit $?
fi
(cd $VENDOR/wxwidgets/current/wxWidgets-2.9.4/buildgtkud && $VENDOR/wxwidgets/current/wxWidgets-2.9.4/configure --with-gtk --enable-unicode --enable-debug --with-gnomeprint && make)
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

# Build adaptit (debug and release)
cd $TRUNK/bin/linux/
mkdir -p Unicode UnicodeDebug
(cd $TRUNK/bin/linux/Unicode && ../configure --with-wx-config=$VENDOR/wxwidgets/current/wxWidgets-2.9.4/buildgtku/wx-config && make)
if [ $? -ne 0 ]
then
  echo "Error building Adapt It Unicode Release: $?"
  exit $?
fi
(cd $TRUNK/bin/linux/UnicodeDebug && ../configure --with-wx-config=$VENDOR/wxwidgets/current/wxWidgets-2.9.4/buildgtkud/wx-config && make)
if [ $? -ne 0 ]
then
  echo "Error building Adapt It Unicode Debug: $?"
  exit $?
fi

