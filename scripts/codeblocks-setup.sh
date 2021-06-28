#!/bin/bash
# Usage: codeblocks-setup.sh 
# codeblocks-setup.sh -- adds the CodeBlocks repo to the sources.list, 
# downloads and adds the CodeBlocks public key, installs CodeBlocks 16.01. 
# Author: Bill Martin <bill_martin@sil.org>
# Date: 2017-01-26
# Requires an Internet connection to use apt-get install
# whm 22Jun2021 revised to add recent distros of Ubuntu and LinuxMint to 
#   the supportedCodenames variable and the case $distCodename in [case statement]. 
#   Removed sed routines for updating /etc/apt/sources... and used the more efficient
#   sudo add-apt-repository ppa:codeblocks-devs/release call instead.
#   Also updated the Codeblocks CBSIGNKEY and its PGP Public key block.

# Test for Internet connection
echo -e "GET http://google.com HTTP/1.0\n\n" | nc google.com 80 > /dev/null 2>&1
if [ $? -eq 0 ]; then
    echo "Internet connection detected."
else
    echo "No Internet connection detected. You must have an Internet connection"
    echo "to use this script. Aborting..."
    exit 1
fi

supportedDistIDs="LinuxMint Ubuntu"
supportedCodenames="maya qiana rebecca rafaela rosa sarah serena sonya sylvia tara tessa tina tricia ulyana ulyssa uma precise trusty utopic vivid wily xenial yakkety zesty artful bionic focal"
echo -e "\nDetermine if system is LinuxMint or Ubuntu and its Codename"
# Determine whether we are setting up a LinuxMint/Wasta system or a straight Ubuntu system
# The 'lsb_release -is' command returns "LinuxMint" on Mint systems and "Ubuntu" on Ubuntu systems.
distID=`lsb_release -is`
echo "  This system is: $distID"
# Determine what the Codename is of the system
# The 'lsb_release -cs' command returns "tara", "tessa", "tina", "tricia", "ulyana", "ulyssa", "uma", on Mint LTS systems, 
#   and "trusty", "xenial", "bionic", "focal", on Ubuntu systems.
distCodename=`lsb_release -cs`
echo "  The Codename is: $distCodename"
if echo "$supportedDistIDs" | grep -q "$distID"; then
  echo "$distID is a system supported by this script"
  if echo "$supportedCodenames" | grep -q "$distCodename"; then
    echo "The $distCodename Codename is supported by this script"
  else
    echo "But this script does not support setup on $distID $distCodename"
    echo "Aborting..."
    exit 1
  fi
else
  echo "This script does not support setup on $distID"
  echo "Aborting..."
  exit 1
fi

# ***************************************************************************
# Install CodeBlocks PPA Repository and Authorization Key
# ***************************************************************************
# Setup CodeBlocks
echo "calling add-apt-repository ppa:codeblocks-devs/release ..."
sudo add-apt-repository ppa:codeblocks-devs/release
sudo apt-get update


# Ensure CodeBlocks key is installed
# Note: The CodeBlocks key seems difficult to retrieve from its ppa.launchpad.net location,
# so we'll generate it from a heredoc and not bother with getting the key from the Internet.
echo -e "\nEnsuring the CodeBlocks key is installed for ppa.launchpad.net repository..."
# CBSIGNKEY="C99E40E1"
CBSIGNKEY="F6B68557"
CBKey=`apt-key list | grep $CBSIGNKEY`
if [ -z "$CBKey" ]; then
  echo "The CodeBlocks key is NOT installed."
  # Made a temporary directory to generate a cb.gpg file with the public key's data
  TEMPDIR=$(mktemp -dt "$(basename $0).XXXXXXXXXX")
  TEMPFILE=$TEMPDIR/cb.gpg
cat <<EOF >$TEMPFILE
-----BEGIN PGP PUBLIC KEY BLOCK-----
Comment: Hostname: 
Version: Hockeypuck ~unreleased

xsFNBFeyJAABEADU01CIvATGBQCZcZRLQNX7sVkTw1snGx1XW18Z/61PlNVJ5vCV
QNORuya3uWCWDAL7Ok4XExgfmFEW8O5FgjRFlyyJTQOJZiQ/MYw5Y1b0NqO/snnj
4EDIdclzHnL+BunsGXn1VoS4ae26fdLUBU2XRY+Rd/DEtGnSNviLs+TSm3uKIOSI
nyApiCSNYt8oecjrvUNJ9R5aEztunY6XxI4sgsnfsLIqNf9p/SBHAffmrNJuOL6y
oRt89zNh8AAzERxIfCLOl/wY7vx5Xe8RC3V5gSa1iRBvRX/dNKIuWF2CI30UTC/x
ymRo1cQpPk1zAy30Beb13HNl6gN42EKOv54elej89Gs2IveM9COWAQmMaYVIdKMY
W6H8Rx8xgeZ+aUMHSR7+CHyFtRMGAoNnRrKD2XBnxmtCbTviJqF6pE43zDbFhpjg
5DoJtCjyWOgnkipk09DENdLNpnyNyGUj2U0Fj3QL4fUWUqXWavzkOKQE9KpVUH4h
/XBlizL+dtA55woQlERSp+58+DsDN+GiKNZNEOPcwVkEZnRS1zZKQqpUHdoJZ1Ny
VdPGroRMBuMQLwHGI5Pikk3SZfNuuaRq9h9kilmoI+lnsL/kxElAk/jPoMxk2j/+
SHgfJK3v5YasDrmU1P+5S3KStP9wjuiOeNw1ITGPp4+E4Jiy9H3O/Y7qtQARAQAB
zSlMYXVuY2hwYWQgUFBBIGZvciBDb2RlOjpCbG9ja3MgRGV2ZWxvcGVyc8LBeAQT
AQIAIgUCV7IkAAIbAwYLCQgHAwIGFQgCCQoLBBYCAwECHgECF4AACgkQ9ujmeva2
hVfGhw//czgVP7yPtkHfs69MGd2XlHHMoHOIp6/l+6BUnTMxomvU0hBXjKKDdZAe
CQcYJZ6WcBzeXt+BR/3m4XVrpvG58b6WXyK+CGin0JCRYji1fkGsZfVHfbdNdnrw
NWG/qh8T262aRne/QYSvU+En4FifZcdi7k0ZgQxdNxpCiMQNb6iO34sDujbAoiiC
EOGsLYQxqG+GXcE+2qP3C/mML1NyeLHY0pshM/lFbx0JRynUi3Wqq75yHIywnHFg
dMJiZFFSU7bFX8/A35h5/CxxrkAHgCQ+Lg0IZ6i29bggVRobQz5tmvy8nhr03lGC
8nhBSmPtxFe57ce475Rw+itMLjLM02MFL0pEhpNPpJVOVP/G6P4gK4HYlxwwKuLi
Ngaf44YO9HBEBT0ihX504sBxG5wHZ73HMAsvi42rOBFqM3eT7Mh07+ipUlQgp/dX
SmkIHff7BYvGyePsc3LJws2WQTiQtbI/AXWUzwNOAKWyAUVeiJslDYEWWSqNQrSt
5L+enM4cHo+wQ/6TFXEfVh1YFt0c6B1B16lVt3hpTBRvuE6HRLHyGWiYiUsepeZL
uIhlq3T3D0BBQDCsvtruK92afrqg+YSupB0U+ot15Ij4dt0YZ9pTSAIFeuHKv4gH
jYfuR04qtIubHtYnHKimEBxyF62I1PO/praF15eSbIyg4bxKg9k=
=SZDD
-----END PGP PUBLIC KEY BLOCK-----

EOF

  if [ -f $TEMPFILE ]; then
    echo "Found a cb.gpg key so will use it"
    echo "Installing the cb.gpg key..."
    sudo apt-key add $TEMPFILE
    # Clean up - delete the temporary dir and contents
    sudo rm -rf $TEMPDIR
  else
    # Key not in ~/tmp/ so retrieve the sil.gpg key from the SIL external web site
    echo "Could not process a CodeBlocks key from a scripted PGP public key!"
    echo "You will need to download and install the CodeBlocks key later with:"
    echo "  sudo apt-key add <path>/<keyname>.gpg"
  fi
else
  echo "The CodeBlocks key is already installed."
fi

echo -e "\nRefresh apt lists via apt-get update"
sudo apt-get -q update

# Install the latest CodeBlocks package
echo -e "\nInstalling the CodeBlocks package..."
sudo apt-get install codeblocks codeblocks-contrib -y

LASTERRORLEVEL=$?
if [ $LASTERRORLEVEL != 0 ]; then
   echo -e "\nCould not install CodeBlocks."
   echo "Make sure that no other package manager is running and try again."
   return $LASTERRORLEVEL
else
   echo -e "\nInstallation of CodeBlocks completed."
fi


