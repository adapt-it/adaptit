#!/bin/bash
# aid-cb-prebuild-checks.sh -- Perform prebuild checks for installed dependencies 
# before building Adapt It Desktop (AID) within CodeBlocks.
# Note: This scipt is designed to be called from within the "Pre-build steps" of CodeBlocks adaptit configurations.
# It can also be called as a stand-alone script.
# Date: 2017-01-28
# Author: Bill Martin <bill_martin@sil.org>

# The following Linux package dependencies are needed to build the Linux version of Adapt It
# on a fresh xenial system:
# 1. libcurl4-gnutls-dev
# 2. uuid-dev
# Other Linux packages are needed to do packaging of the Linux versions for distribution.
# Those packages may include:
# 1. git-core | git
# 2. libwxgtk3.0-0

ABORT="FALSE"

  # Compiling adaptit will fail if the libcurl4-gnutls-dev package is not installed,
  # so check if libcurl4-gnutls-dev package is installed, and if not notify the user.
  PkgInstalled=$(dpkg-query -W --showformat='${Status}\n' libcurl4-gnutls-dev | grep "install ok installed")
  if [ "x$PkgInstalled" == x"" ]; then
    # The sudo command below should not be executed when the script is called from CodeBlocks
    # since it must execute without input from the user. 
    #echo -e "\nInstalling the libcurl4-gnutls-dev package which is needed to build wxWidgets..."
    #sudo apt-get -y install libcurl4-gnutls-dev
    echo "***You must install the libcurl4-gnutls-dev package to build adaptit"
    echo "***Suggested command: sudo apt-get install libcurl4-gnutls-dev"
    ABORT="TRUE"
  else
    echo "  The libcurl4-gnutls-dev package is already installed"
  fi
  
  # Compiling adaptit will fail if the uuid-dev package is not installed,
  # so check if uuid-dev package is installed, and if not notify the user.
  PkgInstalled=$(dpkg-query -W --showformat='${Status}\n' uuid-dev | grep "install ok installed")
  if [ "x$PkgInstalled" == x"" ]; then
    # The sudo command below should not be executed when the script is called from CodeBlocks
    # since it must execute without input from the user. 
    #echo -e "\nInstalling the uuid-dev package which is needed to build wxWidgets..."
    #sudo apt-get -y install uuid-dev
    echo "***You must install the uuid-dev package to build adaptit"
    echo "***Suggested command: sudo apt-get install uuid-dev"
    ABORT="TRUE"
  else
    echo "  The uuid-dev package is already installed"
  fi
  
  # adaptit needs the git package for history snapshots
  # so check if git package is installed, and if not notify the user.
  PkgInstalled=$(dpkg-query -W --showformat='${Status}\n' git | grep "install ok installed")
  if [ "x$PkgInstalled" == x"" ]; then
    # The sudo command below should not be executed when the script is called from CodeBlocks
    # since it must execute without input from the user. 
    #echo -e "\nInstalling the git package which is needed to build wxWidgets..."
    #sudo apt-get -y install git
    echo "***You should install the git package to package and/or run adaptit"
    echo "***Suggested command: sudo apt-get install git"
    # Do not Abort with error from this script for lack of the git package.
    #ABORT="TRUE"
  else
    echo "  The git package is already installed"
  fi
  
  # Compiling adaptit will fail if the libwxgtk3.0-dev package is not installed,
  # so check if libwxgtk3.0-dev package is installed, and if not notify the user.
  PkgInstalled=$(dpkg-query -W --showformat='${Status}\n' libwxgtk3.0-dev | grep "install ok installed")
  if [ "x$PkgInstalled" == x"" ]; then
    # The sudo command below should not be executed when the script is called from CodeBlocks
    # since it must execute without input from the user. 
    #echo -e "\nInstalling the libwxgtk3.0-dev package which is needed to build wxWidgets..."
    #sudo apt-get -y install libwxgtk3.0-dev
    echo "***You should install the libwxgtk3.0-dev package to package/or run adaptit"
    echo "***Suggested command: sudo apt-get install libwxgtk3.0-dev"
    # Do not Abort with error from the script for lack of the libwxgtk3.0-dev package.
    #ABORT="TRUE"
  else
    echo "  The libwxgtk3.0-dev package is already installed"
  fi
  
  # We only Abort for lack of the libcurl4-gnutls-dev and uuid-dev packages.
  # The other packages are not critical for the adaptit compile/build process.
  if [ x"$ABORT" = x"TRUE" ]; then
    exit 1
  fi 

