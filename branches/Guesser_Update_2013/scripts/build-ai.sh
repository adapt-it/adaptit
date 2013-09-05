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

# troubleshooting helps
set -e
set -x

# remove files from previous builds
rm -rf $TRUNK/bin/linux/UnicodeDebug
if [ $? -ne 0 ]
then
  echo "Unable to remove UnicodeDebug directory: $?"
  exit 1
fi

# Build adaptit (UnicodeDebug) and return the results
cd $TRUNK/bin/linux
# make sure the old configure and friends are gone
rm Makefile.in configure config.sub config.guess aclocal.m4 ltmain.sh
# call autogen to generate configure and friends
./autogen.sh
#bin/linux/autogen.sh
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

