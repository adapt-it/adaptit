#!/bin/bash
# setup-adaptit-dev-tools.sh -- installs needed development tools for building AdaptIt on Ubuntu

SVNURL=http://adaptit.googlecode.com/svn/trunk
TEAMSVNURL=https://adaptit.googlecode.com/svn/trunk

echo "$0: Installing packages for AdaptIt development"
sudo add-apt-repository "deb http://packages.sil.org/ubuntu $(lsb_release -cs) main"
sudo apt-get -q update 
sudo apt-get -q install ubuntu-dev-tools subversion -y	# Install basic development tools
sudo apt-get -q build-dep adaptit -y			# Install things needed to build adaptit
sudo apt-get -q install libgnomeprintui2.2-0 -y		# Optional but helpful printing library

# Obtain adaptit sources from svn
echo "$0: Downloading AdaptIt sources using svn"
mkdir -p ~/subversion
if [ -z "$1" ]; then
  (cd ~/subversion && svn -q checkout $SVNURL adaptit)
else
  (cd ~/subversion && svn -q checkout --username "$1" $TEAMSVNURL adaptit)
fi  

# Configure svn in adaptit-standard way
sed -i -e 's/^# enable-auto-props = yes/enable-auto-props = yes/' ~/.subversion/config
for x in c cpp h
do 
  sed -i -e "s/^# \*\.$x = .*$/*.$x = svn:eol-style=CRLF/" ~/.subversion/config
done

# Build adaptit
cd ~/subversion/adaptit/bin/linux/
mkdir -p Unicode UnicodeDebug
(cd Unicode && ../configure && make)
# (cd UnicodeDebug && ../configure && make)  FIXME for debug wxwidget libraries

# Explain how to run it
[ -f Unicode/adaptit ] && echo "Run adaptit by typing ~/subversion/adaptit/bin/linux/Unicode/adaptit &"
[ -f UnicodeDebug/adaptit ] && echo "Run debug version of adaptit by typing ~/subversion/adaptit/bin/linux/UnicodeDebug/adaptit &"
