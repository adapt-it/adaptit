#!/bin/bash
# aid-dev-setup.sh -- Set up environment for developing Adapt It Desktop (AID) on Ubuntu 12.04, 14.04 or higher
# Note: This scipt may be called from the setup-work-dev-tools.sh script (option 1), or it 
#       can be called independently as a stand-alone script.
# Date: 2015-06-23
# Author: Bill Martin <bill_martin@sil.org>

# Setup AID development tools
echo "Seting up AID Tools..."

WAIT=60
AID_GITURL=https://github.com/adapt-it/adaptit.git
AID_DEV_TOOLS="codeblocks gnome-common libgtk2.0-0-dbg libgtk2.0-dev \
  gcc-multilib uuid-dev curl libcurl4-gnutls-dev \
  libwxbase2.8-0 libwxbase2.8-dbg libwxbase2.8-dev libwxgtk2.8-0 libwxgtk2.8-dbg \
  libwxgtk2.8-dev wx-common wx2.8-headers wx2.8-i18n subversion"
# Removed libgnomeprintui2.2-dev from AID_DEV_TOOLS list above (it's not in 14.04)
supportedDistIDs="LinuxMint Ubuntu"
supportedCodenames="maya qiana rebecca precise trusty utopic vivid wily"
SILKEYURL="http://packages.sil.org/sil.gpg"
echo -e "\nDetermine if system is LinuxMint or Ubuntu and its Codename"
# Determine whether we are setting up a LinuxMint/Wasta system or a straight Ubuntu system
# The 'lsb_release -is' command returns "LinuxMint" on Mint systems and "Ubuntu" on Ubuntu systems.
distID=`lsb_release -is`
echo "  This system is: $distID"
# Determine what the Codename is of the system
# The 'lsb_release -cs' command returns "maya", "qiana" or "rebecca" on Mint LTS systems, 
#   and "precise", "trusty", "utopic", "vivid", or "wily" on Ubuntu systems"
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
#   LTS Codename, i.e., "precise" for maya, or "trusty" for either qiana or rebecca.
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
esac
echo -e "\nAdding 'deb http://packages.sil.org/ubuntu $distCodename main' to software sources"
sudo add-apt-repository "deb http://packages.sil.org/ubuntu $distCodename main"

echo -e "\nEnsuring the sil.gpg key is installed for the packages.sil.org repository..."
# Ensure sil.gpg key is installed
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

echo -e "\nRefresh apt lists via apt-get update"
sudo apt-get -q update
# Install tools for development work focusing on Adapt It Desktop (AID)
echo -e "\nInstalling AIM development tools..."
sudo apt-get install $AID_DEV_TOOLS -y

# Ask user if we should get the Adapt It sources from Github
# Provide a 60 second countdown for response. If no response assume "yes" response
echo -e "\nSetup can get the latest Adapt It Desktop (AID) sources from Github"
echo "The AID sources will be located in an ~/adaptit/ dir in your Home dir."
if [ -f ~/adaptit/.git/config ]; then
  echo "Do you want to Pull down any recent changes to ~/adaptit/? [y/n]?"
  # The git pull command is done in the case statement below
else
  echo "Do you want to Clone AID to ~/adaptit/? [y/n]"
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
    if [ -f ~/adaptit/.git/config ]; then
      echo -e "\nPulling in any adaptit changes to ~/adaptit/..."
      cd ~/adaptit
      git pull
    else
      echo -e "\nCloning the Adapt It Desktop (AID) sources to ~/adaptit/..."
      cd
      git clone $AID_GITURL
    fi

    # Check for an existing git user.name and user.email
    echo -e "\nTo help with AID development you should have a GitHub user.name and user.email."
    echo "Checking for previous configuration of git user name and git user email..."
    # work from the adaptit repo
    cd ~/adaptit
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
      # Build adaptit
      cd ~/adaptit/bin/linux/
      mkdir -p build_debug build_release
      echo -e "\n  Building the debug version..."
      sleep 1
      (cd build_debug && ../configure --prefix=/usr --enable-debug && make)
      echo -e "\n  Building the release version..."
      sleep 1
      (cd build_release && ../configure --prefix=/usr && make)

      # Explain how to run it
      echo -e "\n**Adapt It Desktop Developer Information**"
      echo "After building with make:"
      [ -f build_debug/adaptit ] && echo -e "\n   To run debug version type: ~/adaptit/bin/linux/build_debug/adaptit &"
      [ -f build_release/adaptit ] && echo "   To run release version type: ~/adaptit/bin/linux/build_release/adaptit &"

      ;;
     *)
        echo -e "\nBuilding the Adapt It Desktop (AID) sources was skipped."
        echo "If you want to build the AID sources you can and do:"
        echo "  cd ~/adaptit/bin/linux/"
        echo "  mkdir -p build_debug build_release"
        echo "Then, to build the debug version:"
        echo "  cd ~/adaptit/bin/linux/build_debug"
        echo "  ../configure --prefix=/usr --enable-debug"
        echo "  make"
        echo "Or, to build the release version:"
        echo "  cd ~/adaptit/bin/linux/build_release"
        echo "  ../configure --prefix=/usr"
        echo "  make"
        echo "After building run the release version (while in build_release dir):"
        echo "   ./adaptit &"
        echo "Or, run the debug version (while in the build_debug dir):"
        echo "   ./adaptit &"
        echo "From elsewhere run release version: ~/adaptit/bin/linux/build_release/adaptit &"
        echo "From elsewhere run debug version: ~/adaptit/bin/linux/build_debug/adaptit &"
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

