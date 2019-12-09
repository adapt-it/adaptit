#!/bin/bash
# Usage: codeblocks-setup.sh 
# codeblocks-setup.sh -- adds the CodeBlocks repo to the sources.list, 
# downloads and adds the CodeBlocks public key, installs CodeBlocks 16.01. 
# Author: Bill Martin <bill_martin@sil.org>
# Date: 2017-01-26
# Requires an Internet connection to use apt-get install

# Test for Internet connection
echo -e "GET http://google.com HTTP/1.0\n\n" | nc google.com 80 > /dev/null 2>&1
if [ $? -eq 0 ]; then
    echo "Internet connection detected."
else
    echo "No Internet connection detected. You must have an Internet connection"
    echo "to use this script. Aborting..."
    exit 1
fi

#
APT_SOURCES_LIST_DIR="/etc/apt/sources.list"
supportedDistIDs="LinuxMint Ubuntu"
supportedCodenames="maya qiana rebecca rafaela rosa sarah serena sonya sylvia tara precise trusty utopic vivid wily xenial yakkety zesty artful bionic"
echo -e "\nDetermine if system is LinuxMint or Ubuntu and its Codename"
# Determine whether we are setting up a LinuxMint/Wasta system or a straight Ubuntu system
# The 'lsb_release -is' command returns "LinuxMint" on Mint systems and "Ubuntu" on Ubuntu systems.
distID=`lsb_release -is`
echo "  This system is: $distID"
# Determine what the Codename is of the system
# The 'lsb_release -cs' command returns "maya", "rafaela", "rosa", "sarah", "serena" on Mint LTS systems, 
#   and "precise", "trusty", "utopic", "vivid", "wily", "xenial", "yakkety" on Ubuntu systems"
distCodename=`lsb_release -cs`
echo "  The Codename is: $distCodename"
if echo "$supportedDistIDs" | grep -q "$distID"; then
  echo "$distID is a system supported by this script"
  if echo "$supportedCodenames" | grep -q "$distCodename"; then
    echo "The $distCodename Codename is supported by this script"
  else
    echo "But this script does not support setup on $distID $distCodename"
    echo "Aborting..."
  fi
else
  echo "This script does not support setup on $distID"
  echo "Aborting..."
  exit 1
fi

# Get the current Codename.
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
  "sonya")
  distCodename="xenial"
  ;;
  "sylvia")
  distCodename="xenial"
  ;;
  "tara")
  distCodename="bionic"
  ;;
esac
echo "  The Modified Codename for Deveopment is: $distCodename"



# Setup CodeBlocks
echo "Seting up CodeBlocks..."


# ***************************************************************************
# Install CodeBlocks PPA Repository and Authorization Key
# ***************************************************************************

# Add CodeBlocks repository
echo -e "\nAdding CodeBlocks PPA repository to software sources"
case $distCodename in
  "precise")
    # 
    sudo sed -i -e '$a deb http://ppa.launchpad.net/damien-moore/codeblocks-stable/ubuntu precise main' \
        -i -e '\@deb http://ppa.launchpad.net/damien-moore/codeblocks-stable/ubuntu precise main@d' \
        $APT_SOURCES_LIST_DIR
  ;;
  "trusty")
    # 
    sudo sed -i -e '$a deb http://ppa.launchpad.net/damien-moore/codeblocks-stable/ubuntu trusty main' \
        -i -e '\@deb http://ppa.launchpad.net/damien-moore/codeblocks-stable/ubuntu trusty main@d' \
        $APT_SOURCES_LIST_DIR
  ;;
  "xenial")
    # 
    sudo sed -i -e '$a deb http://ppa.launchpad.net/damien-moore/codeblocks-stable/ubuntu xenial main' \
        -i -e '\@deb http://ppa.launchpad.net/damien-moore/codeblocks-stable/ubuntu xenial main@d' \
        $APT_SOURCES_LIST_DIR
  ;;
esac

# Ensure CodeBlocks key is installed
# Note: The CodeBlocks key seems difficult to retrieve from its ppa.launchpad.net location,
# so we'll generate it from a heredoc and not bother with getting the key from the Internet.
echo -e "\nEnsuring the CodeBlocks key is installed for ppa.launchpad.net repository..."
CBSIGNKEY="C99E40E1"
CBKey=`apt-key list | grep $CBSIGNKEY`
if [ -z "$CBKey" ]; then
  echo "The CodeBlocks key is NOT installed."
  # Made a temporary directory to generate a cb.gpg file with the public key's data
  TEMPDIR=$(mktemp -dt "$(basename $0).XXXXXXXXXX")
  TEMPFILE=$TEMPDIR/cb.gpg
cat <<EOF >$TEMPFILE
-----BEGIN PGP PUBLIC KEY BLOCK-----
Version: SKS 1.1.5
Comment: Hostname: keyserver.ubuntu.com

mI0ESveZFQEEAKThHGzw+SstcL3VGb77mygRPQHUmw99SHUWU+rNxeAfZh8Nrk6XLaIqHFxV
5eBUuFCVlafPElLIqzYnRc+/fcnp1ULEXcBYrCOnroHWR0zcdgbSAIhQaRYe8hXTuGKp/zXa
SsgXqeX4Q8QaYLx7oJYX2MTLLrtbo+p6M+AnNgItABEBAAG0GExhdW5jaHBhZCBQUEEgZm9y
IHNwaWxseoi2BBMBAgAgBQJK95kVAhsDBgsJCAcDAgQVAggDBBYCAwECHgECF4AACgkQh7sH
tMmeQOFZywQAorsav42L8CCfEtdsMIHsGZxkYLfY2FRkEzyXAneIWIFwTeibxbjIcxYg4NzA
oEoDJ1//6+YRAoqooBRGQSdrTYMfkokl4Rr+/98hV1LPCO8TSVTmvmeCIBsENB32Pn1zEbYC
vqNpsgdbrHxLhtNf+bZIO1sfpslepi6o8B9u0jU=
=DYMr
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

exit 0

# Install the latest CodeBlocks (17.12) package
echo -e "\nInstalling the CodeBlocks package..."
sudo apt-get install codeblocks -y

LASTERRORLEVEL=$?
if [ $LASTERRORLEVEL != 0 ]; then
   echo -e "\nCould not install CodeBlocks."
   echo "Make sure that no other package manager is running and try again."
   return $LASTERRORLEVEL
else
   echo -e "\nInstallation of CodeBlocks completed."
fi


