#!/bin/bash
# aid-dev-setup.sh -- Set up environment for developing Adapt It Desktop (AID) on Ubuntu/Wasta 12.04, 14.04 or 16.04.
# Note: This scipt may be called from the setup-work-dev-tools.sh script (option 1), or it 
#       can be called independently as a stand-alone script.
# Date: 2015-06-23
# Author: Bill Martin <bill_martin@sil.org>
# Revision: 29August2015 whm added support for Linux Mint Rafaela
# Revision: 13May2016 whm added support for Rosa, Sarah, Xenial.
#   Split AID_DEV_TOOLS into two groups Added newer 3.x packages 
#   to AID_DEV_TOOLS_NEW and 2.0 and 2.8 packages to AID_DEV_TOOLS_OLD.
# The 3.x packages are installed on systems starting with Xenial; the
# 2.0 and 2.8 packages are installed on all systems prior to Xenial.
# Revision: 15May2016 whm added CodeBlocks PPA and GPG key to software 
#   sources so the newest version of CodeBlocks can be installed.
#   06Oct2016 - added the build-essential package to AID_DEV_TOOLS_OLD and AID_DEV_TOOLS_NEW
#     to correct failure to build AI on systems without the g++ compiler.
#
# Setup AID development tools
echo "Seting up AID Tools..."

PROJECT_DIR=${1:-~/projects}	# AIM development file location, default ~/projects/
WAIT=60
AID_GITURL=https://github.com/adapt-it/adaptit.git
AID_DEV_TOOLS_OLD="codeblocks poedit git gnome-common libgtk2.0-0-dbg libgtk2.0-dev \
  build-essential gcc-multilib uuid-dev curl libcurl4-gnutls-dev \
  libwxbase2.8-0 libwxbase2.8-dbg libwxbase2.8-dev libwxgtk2.8-0 libwxgtk2.8-dbg \
  libwxgtk2.8-dev wx-common wx2.8-headers wx2.8-i18n"
AID_DEV_TOOLS_NEW="codeblocks poedit git gnome-common libgtk-3-0-dbg libgtk-3-dev \
  build-essential gcc-multilib uuid-dev curl libcurl4-gnutls-dev \
  libwxbase3.0-0v5 libwxbase3.0-0v5-dbg libwxbase3.0-dev libwxgtk3.0-0v5 libwxgtk3.0-0v5-dbg \
  libwxgtk3.0-dev wx-common wx3.0-headers wx3.0-i18n"
# Note: the wx2.8-i18n and wx3.0-i18n packages cannot be installed at the same time, 
# otherwise you get: apt-get error: wx2.8-i18n : Conflicts: wx-i18n
# Removed libgnomeprintui2.2-dev from AID_DEV_TOOLS... list above (it's not in 14.04 and not really needed)
supportedDistIDs="LinuxMint Ubuntu"
supportedCodenames="maya qiana rebecca rafaela rosa sarah precise trusty utopic vivid wily xenial"
echo -e "\nDetermine if system is LinuxMint or Ubuntu and its Codename"
# Determine whether we are setting up a LinuxMint/Wasta system or a straight Ubuntu system
# The 'lsb_release -is' command returns "LinuxMint" on Mint systems and "Ubuntu" on Ubuntu systems.
distID=`lsb_release -is`
echo "  This system is: $distID"
# Determine what the Codename is of the system
# The 'lsb_release -cs' command returns "maya", "qiana", "rebecca", "rafaela", "rosa", "sarah" on Mint LTS systems, 
#   and "precise", "trusty", "utopic", "vivid", "wily", "xenial" on Ubuntu systems"
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

# Ensure the apt repository is setup for the SIL repository using the proper Codename.
# On LinuxMint/Wasta systems, the Codename for the SIL repo must use the Ubuntu equivalent 
#   LTS Codename, i.e., "precise" for maya, or "trusty" for qiana, rebecca, or rafaela.
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
esac
echo "  The Modified Codename for Deveopment is: $distCodename"

# whm Note: the add-apt-repository command seems to have differing behaviors on
# different Ubuntu distributions - storing software sources list(s) always in
# sources.list in precise, but in separate *.list in sources.list.d on trusty
# and xenial - at least under Wasta-Linux. Using a bash grep command, as done
# below, should do the job without resulting in duplicates, since it won't change
# anything on Wasta-Linux distributions.
PSO_URL="deb http://packages.sil.org/ubuntu $distCodename main"
echo "PSO_URL is: $PSO_URL"
PSO_EXP_URL="deb http://packages.sil.org/ubuntu $distCodename-experimental main"
echo "PSO_EXP_URL is: $PSO_EXP_URL"
# The add-apt-repository command seems to result in duplicates
#sudo add-apt-repository "$PSO_URL"
# This one only works on precise and non-Wasta trusty and xenial distributions
# A Wasta installation will already have the PSO added so only add if PSO isn't already there
# Note: Grep returns 0 if selected lines are found, 1 if selected lines are not found.
# command1 || command2 - Command2 is executed if, and only if, command1 returns a non-zero exit status.
grep -qR "$PSO_URL" /etc/apt/sources.list* \
  || echo "$PSO_URL" | sudo tee -a /etc/apt/sources.list
grep -qR "$PSO_EXP_URL" /etc/apt/sources.list* \
  || echo "$PSO_EXP_URL" | sudo tee -a /etc/apt/sources.list

# Ensure sil.gpg key is installed
echo -e "\nEnsuring the sil.gpg key is installed for the packages.sil.org repository..."
SILKEYURL="http://packages.sil.org/sil.gpg"
SILKey=`apt-key list | grep archive@packages.sil.org`
if [ -z "$SILKey" ]; then
  echo "The SIL key is NOT installed."
  mkdir -p ~/tmp
  sudo chown $USER:$USER ~/tmp 
  cd ~/tmp
  if [ -f ~/tmp/sil.gpg ]; then
    echo "Found sil.gpg already in ~/tmp/ so will use it"
  else
    # Key not in ~/tmp/ so retrieve the sil.gpg key from the SIL external web site
    echo "Retrieving the sil.gpg key from $SILKEYURL to ~/tmp/"
    wget --no-clobber --no-directories $SILKEYURL
  fi
  if [ -f ~/tmp/sil.gpg ]; then
    echo "Installing the sil.gpg key..."
    sudo apt-key add ~/tmp/sil.gpg
  else
    echo "The SIL key could not be retrieved from the website."
    echo "You will need to download and install the SIL key later with:"
    echo "  sudo apt-key add <path>/sil.gpg"
  fi
else
  echo "The SIL key is already installed."
fi

# Add CodeBlocks repository
echo -e "\nAdding CodeBlocks PPA repository to software sources"
case $distCodename in
  "precise")
    # 
    sudo sed -i -e '$a deb http://ppa.launchpad.net/damien-moore/codeblocks-stable/ubuntu precise main' \
        -i -e '\@deb http://ppa.launchpad.net/damien-moore/codeblocks-stable/ubuntu precise main@d' \
        $APT_SOURCES
  ;;
  "trusty")
    # 
    sudo sed -i -e '$a deb http://ppa.launchpad.net/damien-moore/codeblocks-stable/ubuntu trusty main' \
        -i -e '\@deb http://ppa.launchpad.net/damien-moore/codeblocks-stable/ubuntu trusty main@d' \
        $APT_SOURCES
  ;;
  "xenial")
    # TODO: Check/Modify if xenial differs from trusty above
    # 
    sudo sed -i -e '$a deb http://ppa.launchpad.net/damien-moore/codeblocks-stable/ubuntu xenial main' \
        -i -e '\@deb http://ppa.launchpad.net/damien-moore/codeblocks-stable/ubuntu xenial main@d' \
        $APT_SOURCES
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

# Install tools for development work focusing on Adapt It Desktop (AID)
echo -e "\nInstalling AIM development tools for $distCodename..."
if [ "$distCodename" = "xenial" ]; then
  sudo apt-get install $AID_DEV_TOOLS_NEW -y
else
  sudo apt-get install $AID_DEV_TOOLS_OLD -y
fi

# Ask user if we should get the Adapt It sources from Github
# Provide a 60 second countdown for response. If no response assume "yes" response
echo -e "\nSetup can get the latest Adapt It Desktop (AID) sources from Github"
echo "The AID sources will be located in $PROJECT_DIR/adaptit/"
if [ -f $PROJECT_DIR/adaptit/.git/config ]; then
  echo "Do you want to Pull down any changes to $PROJECT_DIR/adaptit/? [y/n]?"
  # The git pull command is done in the case statement below
else
  echo "Do you want to Clone AID to $PROJECT_DIR/adaptit/? [y/n]"
  # The git clone command is done in the case statement below
fi
for (( i=$WAIT; i>0; i--)); do
    printf "\rPlease press the y (default) or n key, or hit any key to abort - countdown $i "
    read -s -n 1 -t 1 response1
    if [ $? -eq 0 ]
    then
        break
    fi
done
if [ ! $response1 ]; then
  echo -e "\nNo selection made, or no response within $WAIT seconds. Assuming response of y"
  response1="y"
fi
echo -e "\nYour choice was $response1"
case $response1 in
  [yY][eE][sS]|[yY]) 
    # Check for an existing local adaptit repo
    if [ -f $PROJECT_DIR/adaptit/.git/config ]; then
      echo -e "\nPulling in any adaptit changes to $PROJECT_DIR/adaptit/..."
      cd $PROJECT_DIR/adaptit
      git pull
    else
      echo -e "\nCloning the Adapt It Desktop (AID) sources to $PROJECT_DIR/adaptit/..."
      mkdir -p "${PROJECT_DIR}"
      cd ${PROJECT_DIR}
      [ -d adaptit ] || git clone $AID_GITURL
    fi

    # Check for an existing git user.name and user.email
    echo -e "\nTo help with AID development you should have a GitHub user.name and user.email."
    echo "Checking for previous configuration of git user name and git user email..."
    # work from the adaptit repo
    cd ${PROJECT_DIR}/adaptit
    gitUserName=`git config user.name`
    gitUserEmail=`git config user.email`
    if [ -z  "$gitUserName" ]; then
      echo "  A git user.name has not yet been configured."
      read -p "Type your git user name: " gitUserName
      if [ ! -z "$gitUserName" ]; then
        echo "  Setting $gitUserName as your git user.name"
        git config user.name "$gitUserName"
      else
        echo "  Nothing entered. No git configuration made for user.name!"
      fi
    else
      echo "  Found this git user.name: $gitUserName"
      echo "  $gitUserName will be used as your git name for the adaptit repository."
    fi
    
    if [ -z  "$gitUserEmail" ]; then
      echo "  A git user.email has not yet been configured."
      read -p "Type your git user email: " gitUserEmail
      if [ ! -z "$gitUserEmail" ]; then
        echo "  Setting $gitUserEmail as your git user.email"
        git config user.email "$gitUserEmail"
      else
        echo "  Nothing entered. No git configuration made for user.email"
      fi
    else
      echo "  Found this git user.email: $gitUserEmail"
      echo "  $gitUserEmail will be used as your git email for the adaptit repository."
    fi
    # Add 'git config push.default simple' command which will be the default in git version 2.0
    # In Ubuntu Precise 12.04, the git version is 1.7.9 which doesn't recognize a 'simple'
    # setting for push.default, so leave any setting up to the developer.
    #git config push.default simple.
    echo -e "\nThe git configuration settings for the adaptit repository are:"
    git config --list
    sleep 2

    echo -e "\nDo you want to Build the Adapt It Desktop project now? [y/n]"
    for (( i=$WAIT; i>0; i--)); do
        printf "\rPlease press the y (default) or n key, or hit any key to abort - countdown $i "
        read -s -n 1 -t 1 response2
        if [ $? -eq 0 ]
        then
            break
        fi
    done
    if [ ! $response2 ]; then
      echo -e "\nNo selection made, or no response within $WAIT seconds. Assuming response of y"
      response2="y"
    fi
    echo -e "\nYour choice was $response2"
    case $response2 in
      [yY][eE][sS]|[yY]) 
        echo -e "\nBuilding the Adapt It Desktop (AID) project..."

      # Build ideas taken from build-ai.sh that is used to build AID by Travis CI
      # remove files from previous builds
      rm -rf $PROJECT_DIR/adaptit/bin/linux/build_debug
      if [ $? -ne 0 ]
      then
        echo "Unable to remove build_debug directory: $?"
        exit 1
      fi

      rm -rf $PROJECT_DIR/adaptit/bin/linux/build_release
      if [ $? -ne 0 ]
      then
        echo "Unable to remove build_release directory: $?"
        exit 1
      fi

      # Build adaptit and return the results
      cd $PROJECT_DIR/adaptit/bin/linux/
      # make sure the old configure and friends are gone
      rm -f Makefile.in configure config.sub config.guess aclocal.m4 ltmain.sh
      # call autogen to generate configure and friends
      ./autogen.sh
      if [ $? -ne 0 ]
      then
        echo "Error in autogen.sh script: $?"
        exit 1
      fi

      mkdir -p build_debug
      echo -e "\n********************************************"
      echo      "**  Building the Unicode Debug version... **"
      echo      "********************************************"
      sleep 1
      (cd build_debug && ../configure --prefix=/usr --enable-debug)
      if [ $? -ne 0 ]
      then
        echo "Error configuring for Unicode Debug build: $?"
        exit 1
      fi
      (cd build_debug && make clean && make)
      if [ $? -ne 0 ]
      then
        echo "Error building Adapt It Unicode Debug: $?"
        exit 1
      fi

      cd $PROJECT_DIR/adaptit/bin/linux/
      mkdir -p build_release
      echo -e "\n**********************************************"
      echo      "**  Building the Unicode Release version... **"
      echo      "**********************************************"
      sleep 1
      (cd build_release && ../configure --prefix=/usr)
      if [ $? -ne 0 ]
      then
        echo "Error configuring for Unicode Release build: $?"
        exit 1
      fi
      (cd build_release && make clean && make)
      if [ $? -ne 0 ]
      then
        echo "Error building Adapt It Unicode Release: $?"
        exit 1
      fi

      echo " "
      echo "-------------------------------------------------"
      echo "-- Adapt It Debug and Release builds succeeded --"
      echo "-------------------------------------------------"
      echo " "

      # Explain how to run it
      echo -e "\n**Adapt It Desktop Developer Information**"
      echo "After building with make you can run the debug or release version:"
      [ -f build_debug/adaptit ] && echo -e "\n   Type: $PROJECT_DIR/adaptit/bin/linux/build_debug/adaptit &"
      [ -f build_release/adaptit ] && echo    "or Type: $PROJECT_DIR/adaptit/bin/linux/build_release/adaptit &"

      ;;
     *)
        echo -e "\nBuilding the Adapt It Desktop (AID) sources was skipped."
        echo -e "\n**Adapt It Desktop Developer Information**"
        echo "If you want to build the AID sources you can and do:"
        echo "    cd $PROJECT_DIR/adaptit/bin/linux/"
        echo "    mkdir -p build_debug build_release"
        echo "  Then, to build the debug version:"
        echo "    cd $PROJECT_DIR/adaptit/bin/linux/build_debug"
        echo "    ../configure --prefix=/usr --enable-debug"
        echo "    make"
        echo "  or, to build the release version:"
        echo "    cd $PROJECT_DIR/adaptit/bin/linux/build_release"
        echo "    ../configure --prefix=/usr"
        echo "    make"
        echo "  After building run the release version (while in build_release dir):"
        echo "     ./adaptit &"
        echo "  or, run the debug version (while in the build_debug dir):"
        echo "     ./adaptit &"
        echo "  Run the debug or release application from elsewhere:"
        echo "     type: $PROJECT_DIR/adaptit/bin/linux/build_release/adaptit &"
        echo "  or type: $PROJECT_DIR/adaptit/bin/linux/build_debug/adaptit &"
        ;;
    esac
    ;;
 *)
    echo -e "\nDownloading the Adapt It Desktop (AID) sources was skipped."
    echo "If you want to get the AID sources you can cd to the desired dir and do:"
    echo "  git clone $AID_GITURL"
    echo "to create an 'adaptit' working copy at the location the command is run from."
    ;;
esac

