#!/bin/bash
# build-ai.sh -- builds AdaptIt on Ubuntu
# Currently this only builds the UnicodeDebug release for the continuous builds
# on TeamCity.

DIR=$( cd "$( dirname "$0" )" && pwd )
TRUNK=$DIR/..
echo " "
echo "-- Adapt It build configurations --"
echo "            Current directory: $DIR"
echo "Base source control directory: $TRUNK"
echo "-------------------------------------"

# remove files from previous builds
rm -rf $TRUNK/bin/linux/UnicodeDebug
if [ $? -ne 0 ]
then
  echo "Unable to remove UnicodeDebug directory: $?"
  exit 1
fi

# Configure svn in adaptit-standard way
# (just in case it isn't already set up like this)
sed -i -e 's/^# enable-auto-props = yes/enable-auto-props = yes/' ~/.subversion/config
for x in c cpp h
do 
  sed -i -e "s/^# \*\.$x = .*$/*.$x = svn:eol-style=CRLF/" ~/.subversion/config
done

# Build adaptit (UnicodeDebug) and return the results
# call autogen
cd $TRUNK/bin/linux
./autogen.sh
if [ $? -ne 0 ]
then
  echo "Error in autogen.sh script: $?"
  exit 1
fi

# create the Unicode Debug directory and call configure
mkdir -p UnicodeDebug
(cd UnicodeDebug && ../configure --enable-debug)
if [ $? -ne 0 ]
then
  echo "Error configuring for UnicodeDebug build: $?"
  exit 1
fi

#($CONFIG/UnicodeDebug/make)
(cd UnicodeDebug && make)
if [ $? -ne 0 ]
then
  echo "Error building Adapt It Unicode Debug: $?"
  exit 1
fi

