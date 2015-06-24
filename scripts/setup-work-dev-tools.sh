#!/bin/bash
# setup-work-dev.sh -- Installs user's choice of dev tools for: AID, AIM, and/or AID and Wasta packaging
# Author: Bill Martin <bill_martin@sil.org>
# Date: 2015-06-23
# This script queries the developer for which part of development tools to setup
# from the following menu of options:
# ------------------------------------------------------------------
# Main Menu of Setup Options:
# 1) Setup development tools for Adapt It Desktop (AID) only.
# 2) Setup development tools for Adapt It Mobile (AIM) only.
# 3) Setup development tools for Linux Packaging (AID and Wasta) only.
# 4) Quit without running this script.
# ------------------------------------------------------------------

AID_DEV_SETUP_SCRIPT="aid-dev-setup.sh"
AIM_DEV_SETUP_SCRIPT="aim-dev-setup.sh"
PKG_DEV_SETUP_SCRIPT="pkg-dev-setup.sh"
# On GitHub, we must grab the "raw" content to get the single aim-dev-setup.sh file
AID_SETUP_SCRIPT_URL=https://raw.githubusercontent.com/adapt-it/adaptit/master/scripts/aid-dev-setup.sh
AIM_SETUP_SCRIPT_URL=https://raw.githubusercontent.com/adapt-it/adapt-it-mobile/master/aim-dev-setup.sh
PKG_SETUP_SCRIPT_URL=https://raw.githubusercontent.com/adapt-it/adaptit/master/scripts/pkg-dev-setup.sh
WAIT=60

# ------------------------------------------------------------------------------
# Main program starts here
# ------------------------------------------------------------------------------

# ping the Internet to check for Internet access to www.archive.ubuntu.com
ping -c1 -q www.archive.ubuntu.com > /dev/null 2>&1
if [ "$?" != 0 ]; then
  echo -e "\nInternet access to software repos (www.archive.ubuntu.com) not available."
  echo "This script cannot continue without access to the software repositories."
  echo "Make sure the computer has access to the Internet, then try again."
  echo "Or, alternately, run wasta-offline and run this script without Internet access."
  echo "Aborting..."
  exit 1
else
  echo -e "\nInternet access to software repos (www.archive.ubuntu.com) appears to be OK!"
fi

SetupAidTools="false"
SetupAimTools="false"
SetupPackagingTools="false"

# Query the developer for which part of development tools to setup.
# This main menu has a 60 second timed countdown. If no reponse within
# 60 seconds, the script just quits.
echo -e "\nWhat development tools do you want to setup?"
echo "  1) Setup development tools for Adapt It Desktop (AID) only."
echo "  2) Setup development tools for Adapt It Mobile (AIM) only."
echo "  3) Setup development tools for Linux Packaging (AID and Wasta) only."
echo "  4) Quit without running this script."
for (( i=$WAIT; i>0; i--)); do
    printf "\rPlease press a number key from 1 to 4, or hit any key to abort - countdown $i "
    read -s -n 1 -t 1 SELECTION
    if [ $? -eq 0 ]
    then
        break
    fi
done

if [ ! $SELECTION ]; then
  echo -e "\nNo selection made, or no response within $WAIT seconds. Assuming response of 4)"
  echo "No setup will be done at this time. Run script again to select a setup option."
  exit 0
fi

echo -e "\nYour choice was $SELECTION"
case $SELECTION in
  "1")
     SetupAidTools="true"; echo "  Setup AID Tools"
   ;;
  "2")
     SetupAimTools="true"; echo "  Setup AIM Tools"
   ;;
  "3")
     SetupPackagingTools="true"; echo "  Setup Packaging Tools"
   ;;
    *)
    echo "Unrecognized response. Aborting..."
    exit 1
  ;;
esac

sleep 2


if [[ "$SetupAidTools" == "true" ]]
then
  # Use wget to retrieve the latest aid-dev-setup.sh script from the GitHub site
  # storing it in the user's ~/tmp directory.
  mkdir -p ~/tmp
  sudo chown $USER:$USER ~/tmp 
  cd ~/tmp
  # Remove any existing script(s) in ~/tmp previously retrived by wget
  rm $AID_DEV_SETUP_SCRIPT*
  wget --no-clobber --no-directories $AID_SETUP_SCRIPT_URL
  if [ -f "$AID_DEV_SETUP_SCRIPT" ]; then
    echo "The $AID_DEV_SETUP_SCRIPT setup script was found!"
    # make sure it is executable
    chmod +x $AID_DEV_SETUP_SCRIPT
    # execute the aim-dev-setup.sh script
    bash $AID_DEV_SETUP_SCRIPT
  else
    echo "The $AID_DEV_SETUP_SCRIPT setup script was NOT found!"
    echo "Cannot setup the development tools for AID."
    echo "You will need to locate and execute the $AID_DEV_SETUP_SCRIPT script later."
  fi
  echo -e "\nSetup of AID development tools has finished!"

fi

if [[ "$SetupAimTools" == "true" ]]
then
  # Use wget to retrieve the latest aim-dev-setup.sh script from the GitHub site
  # storing it in the user's ~/tmp directory.
  mkdir -p ~/tmp
  sudo chown $USER:$USER ~/tmp 
  cd ~/tmp
  # Remove any existing script(s) in ~/tmp previously retrived by wget
  rm $AIM_DEV_SETUP_SCRIPT*
  wget --no-clobber --no-directories $AIM_SETUP_SCRIPT_URL
  if [ -f "$AIM_DEV_SETUP_SCRIPT" ]; then
    echo "The $AIM_DEV_SETUP_SCRIPT setup script was found!"
    # make sure it is executable
    chmod +x $AIM_DEV_SETUP_SCRIPT
    # execute the aim-dev-setup.sh script
    bash $AIM_DEV_SETUP_SCRIPT
  else
    echo "The $AIM_DEV_SETUP_SCRIPT setup script was NOT found!"
    echo "Cannot setup the development tools for AIM."
    echo "You will need to locate and execute the $AIM_DEV_SETUP_SCRIPT script later."
  fi
  echo -e "\nSetup of AIM development tools has finished!"

fi

if [[ "$SetupPackagingTools" == "true" ]]
then
  # Use wget to retrieve the latest pkg-dev-setup.sh script from the GitHub site
  # storing it in the user's ~/tmp directory.
  mkdir -p ~/tmp
  sudo chown $USER:$USER ~/tmp 
  cd ~/tmp
  # Remove any existing script(s) in ~/tmp previously retrived by wget
  rm $PKG_DEV_SETUP_SCRIPT*
  wget --no-clobber --no-directories $PKG_SETUP_SCRIPT_URL
  if [ -f "$PKG_DEV_SETUP_SCRIPT" ]; then
    echo "The $PKG_DEV_SETUP_SCRIPT setup script was found!"
    # make sure it is executable
    chmod +x $PKG_DEV_SETUP_SCRIPT
    # execute the aim-dev-setup.sh script
    bash $PKG_DEV_SETUP_SCRIPT
  else
    echo "The $PKG_DEV_SETUP_SCRIPT setup script was NOT found!"
    echo "Cannot setup the development tools for Packaging AIM/Wasta."
    echo "You will need to locate and execute the $PKG_DEV_SETUP_SCRIPT script later."
  fi
  echo -e "\nSetup of AID/Wasta Packaging development tools has finished!"

fi


