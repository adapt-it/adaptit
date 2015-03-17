#!/bin/bash
# setup-adaptit-dev-tools.sh -- installs needed development tools for building AdaptIt on Ubuntu

GITURL=https://github.com/adapt-it/adaptit.git

echo "$0: Installing packages for AdaptIt development"
sudo add-apt-repository "deb http://packages.sil.org/ubuntu $(lsb_release -cs) main"
sudo apt-get -q update 
sudo apt-get -q install ubuntu-dev-tools git -y	# Install basic development tools
sudo apt-get -q build-dep adaptit -y			# Install things needed to build adaptit
sudo apt-get -q install libgnomeprintui2.2-0 -y		# Optional but helpful printing library

# Obtain adaptit sources
echo "$0: Downloading AdaptIt sources"
mkdir -p ~/git
(cd ~/git && git clone $GITURL)

# Configure svn in adaptit-standard way
# sed -i -e 's/^# enable-auto-props = yes/enable-auto-props = yes/' ~/.subversion/config
#for x in c cpp h
#do 
#  sed -i -e "s/^# \*\.$x = .*$/*.$x = svn:eol-style=CRLF/" ~/.subversion/config
#done

# Build adaptit
cd ~/git/adaptit/bin/linux/
mkdir -p Unicode UnicodeDebug
(cd Unicode && ../configure && make)
# (cd UnicodeDebug && ../configure && make)  FIXME for debug wxwidget libraries

# Explain how to run it
[ -f Unicode/adaptit ] && echo "Run adaptit by typing ~/git/adaptit/bin/linux/Unicode/adaptit &"
[ -f UnicodeDebug/adaptit ] && echo "Run debug version of adaptit by typing ~/git/adaptit/bin/linux/UnicodeDebug/adaptit &"
