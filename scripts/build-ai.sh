#!/bin/bash
# build-ai.sh -- builds AdaptIt on Ubuntu
# Currently this only builds the UnicodeDebug release for the continuous builds
# on TeamCity.

DIR=$( cd "$( dirname "$0" )" && pwd )
TRUNK=$DIR/../

# remove files from previous builds
rm -rf $TRUNK/bin/linux/UnicodeDebug

# Configure svn in adaptit-standard way
# (just in case it isn't already set up like this)
sed -i -e 's/^# enable-auto-props = yes/enable-auto-props = yes/' ~/.subversion/config
for x in c cpp h
do 
  sed -i -e "s/^# \*\.$x = .*$/*.$x = svn:eol-style=CRLF/" ~/.subversion/config
done

# Build adaptit (UnicodeDebug) and return the results
cd $TRUNK/bin/linux/
mkdir -p UnicodeDebug
(cd $TRUNK/bin/linux/UnicodeDebug && ../configure --enable-debug && make)
if [ $? -ne 0 ]
then
  echo "Error building Adapt It Unicode Debug: $?"
  exit $?
fi

