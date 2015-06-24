#!/bin/bash
# pkg-dev-setup.sh -- Set up environment for packaging Adapt It Desktop (AID) and Wasta on Ubuntu 12.04, 14.04 or higher
# Note: This scipt may be called from the setup-work-dev-tools.sh script (option 3), or it 
#       can be called independently as a stand-alone script.
# Date: 2015-06-23
# Author: Bill Martin <bill_martin@sil.org>

# Setup Packaging development tools
echo "Seting up Packaging Tools..."

WAIT=60
PACKAGING_TOOLS="apt-file build-essential autoconf automake libtool quilt gnupg \
  devscripts lintian fakeroot lsb-release pbuilder dephelper dh-make dput git"

# Setup packaging tools
echo "Seting up Packaging tools for AIM and Wasta..."

# TODO: Query user for the DEBFULLNAME and DEBEMAIL values to use.

# Add env settings to .profile for pbuilder and make .profile settings active for session
grep -sq "^export DEBFULLNAME=" ~/.profile \
  || echo 'export DEBFULLNAME=Bill\ Martin' >>~/.profile
grep -sq "^export DEBEMAIL=" ~/.profile \
  || echo 'export DEBEMAIL=bill_martin@sil.org' >>~/.profile
source ~/.profile

echo -e "\nRefresh apt lists via apt-get update"
apt-get -q update
# Install tools for development work focusing on AID and Wasta packaging
sudo apt-get install PACKAGING_TOOLS -y

# TODO: Query user whether to install optional packages gnote and virtualbox-4.3

# Install handy (but optional) packages
sudo apt-get install gnote virtualbox-4.3 -y


echo -e "\nOther things to check and/or install manually (script these later):"
echo "* For proper network access in VirtualBox VMs Check that the wired network"
echo "  interface is using eth0. Modify /etc/udev/rules.d/70-persistent-net.rules"
echo "  to use eth0 for the machine's Ethernet MAC address (HWaddr). The eth*"
echo "  value and HWaddr MAC address as shown by ifconfig are:"
echo "    `ifconfig | grep HWaddr`"
echo "* "

