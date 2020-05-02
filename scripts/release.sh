#!/bin/bash
# release-sh -- Downloads and creates release pacakges of AdaptIt
#               First parameter must be a version number e.g. 6.2.2
#               Second parameter must be a space-delimited set
#                 of distro names to build for, e.g. "precise trusty utopic vivid"
#
# Author: Jonathan Marsden <jmarsden@fastmail.fm>
# Date: 2012-10-03
# Revised 2015-01-21 by Bill Martin to include "utopic"
# Revised 2015-05-26 by Bill Martin to remove non-empty .git dirs from sources
# Revised 2015-06-24 by Bill Martin
#   - Made version number and distro names parameters required to run the script
#   - (for better control and monitoring of packaging process)
#   - Provided help message and notes for proper script usage if parameters are not used
#   - Added pbuilder to the DEVTOOLS list
#   - Add vivid to OSRELEASES and UBUNTU_SUITES
#   - Changed debian stable to 'jessie' and debian testing to 'stretch'
#   - Add cases to recognize LinuxMint DIST aliases
#   - When pbuilder hooks file A05suffix needs to be created added queries to obtain
#        user's debian packaging DEBFULLNAME and DEBEMAIL to use in the A05suffix file.
#   - Added echo statements throughout to better track building process.
# Revised 2015-06-30 by Bill Martin
#   - Assigned the appropriate URL to AID_GITURL which had gone missing.
#   - Revised the handling of the ~/packaging dir contents. The script now retains any
#        previously cloned git repository, so that on subsequent runs, the script just
#        updates the repo with 'git pull' before the git checkout call. This saves 
#        having to clone the whole remote git repo for subsequent runs of the script.
#   - Added command to rename the $DIST_result folders to $DIST-amd64_result to add 
#     clarity to the naming of the amd64 result folders.
#   - Added test at beginning for 64 bit machine. Building 64-bit packages requires a
#     64 bit machine.
# Revised 2016-12-08 by Bill Martin
#   - Add xenial and yakkety to OSRELEASES and UBUNTU_SUITES
#   - Added build-essential to the DEVTOOLS list
#   - Add "serena" to LinuxMint DIST aliases
# Revised 2017-01-14 by Bill Martin
#   - Always generates A05suffix hook and uses sed to update existing DEBFULLNAME and DEBEMAIL
#   - Always generates A06makewx hook file to build static custom wx library within pbuilder chroot
#   - Add echo comments to console output to more easily track progress 
# Revised 2018-02-05 by Bill Martin
#   - Add zesty, artful and bionic to OSRELEASES and UBUNTU_SUITES
# Revised 2018-04-17
#   - Corrected the A05suffix hook script to cd to /tmp/buildd/*/ instead of ~/*?
#   - Moved wx library static build parts of script to release-static.sh
# Revised 2018-08-25
#   - Added a call to autogen.sh from within the bin/linux dir of the checkout folder
#        $PACKAGING_DIR/adaptit-${RELEASE}, for example, $HOME/packaging/adaptit-6.9.1/bin/linux
#        The call of autogen.sh is now done before the cleanup of unwanted non-source files.
# Revised 2019-02-15
#   - Add cosmic to OSRELEASES and UBUNTU_SUITES
#   - Add disco to OSRELEASES and UBUNTU_SUITES
# Revised 2020-05-02 by Bill Martin
#   - Add eoan and focal to OSRELEASES and UBUNTU_SUITES
#   - Add tina and tricia to Linux Mint dists

AID_GITURL="https://github.com/adapt-it/adaptit.git"
PBUILDFOLDER=${PBUILDFOLDER:-$HOME/pbuilder}
HOOKSDIR="/hooks"
PROJECTS_DIR="$HOME/projects"	# AID development default file location for the adaptit repo
PACKAGING_DIR="$HOME/packaging"      # default location for the packaging copy of the adaptit repo
OSRELEASES=${2:-"lucid maverick natty oneiric precise quantal raring saucy trusty utopic vivid wily xenial yakkety zesty artful bionic cosmic disco eoan focal sid"}
DEVTOOLS="build-essential ubuntu-dev-tools debhelper pbuilder libtool quilt git subversion"
BUILDDEPS="libwxgtk2.8-dev zip uuid-dev libcurl3-gnutls-dev"

# The building of 64 bit packages requires that we are using a 64-bit architecture machine
Arch=`dpkg --print-architecture`
if [ x"$Arch" = x"amd64" ]; then
  echo -e "\nArchitecture of this machine is 64-bit ( $Arch )\n"
else
  echo -e "\nThe architecture of this machine is NOT 64-bit ( $Arch )"
  echo "Building Adapt It packages requires a 64 bit (amd64) machine in order to"
  echo "build both 32 bit (i386) and 64 bit (amd64) packages."
  echo "This $0 script can only be used on a 64 bit machine. Aborting..."
  exit 1
fi

if [ -z "$1" ]
then
  echo -e "\nScript was called without parameters."
  echo "Script Usage:"
  echo "  ./release.sh <tagged-release-number> \"<distro-names>\""
  echo "for example:  ./release.sh 6.8.1 \"precise trusty xenial\""
  echo "Notes: The tagged release number must be a valid up-to-date git tag in the repo"
  echo "       You can use the tagretag.sh script to create a current git tag if needed"
  echo "       Use quotes on distro names string if more than one distro is given"
  exit 1
fi

# Get any existing DEBFULLNAME and DEBEMAIL env values to put into the A05suffix hook file generated below
SAVEDDEBFULLNAME=$DEBFULLNAME
SAVEDDEBEMAIL=$DEBEMAIL

# Install development tools as required
sudo apt-get update && sudo apt-get upgrade -y
sudo apt-get install $DEVTOOLS -y

# Always install/update the ~/.pbuilderrc pbuilder settings file
# Note: Using << "EOF" to create HERE DOC parameter expansion is suppressed; 
#       Using << EOF to create HERE DOC parameter expansion happens
#[ -f $HOME/.pbuilderrc ] || cat >$HOME/.pbuilderrc <<"EOF"
  cat >$HOME/.pbuilderrc <<"EOF"
echo ""
echo "******************************************************************************"
echo " Begin executing the $HOME/.pbuilderrc pbuilder setup script "
echo "******************************************************************************"

# Codenames for Debian suites according to their alias.
UNSTABLE_CODENAME="sid"
TESTING_CODENAME="stretch"
STABLE_CODENAME="jessie"

# List of Debian suites.
DEBIAN_SUITES=($UNSTABLE_CODENAME $TESTING_CODENAME $STABLE_CODENAME \
    "experimental" "unstable" "testing" "stable")

# List of Ubuntu suites. Update these when needed.
UBUNTU_SUITES=("focal" "eoan" "disco" "cosmic" "bionic" "artful" "zesty" "yakkety" \
    "xenial" "wily" "vivid" "utopic" \
    "trusty" "saucy" "raring" "quantal" \
    "precise" "oneiric" "natty" "maverick" \
    "lucid" "karmic" "jaunty" \
    "intrepid" "hardy" "gutsy")

# Mirrors to use. Update these to your preferred mirror.
DEBIAN_MIRROR="ftp.us.debian.org"
UBUNTU_MIRROR="mirrors.kernel.org"

# Use the changelog of a package to determine the suite to use if none set.
if [ -z "${DIST}" ] && [ -r "debian/changelog" ]; then
    DIST=$(dpkg-parsechangelog | awk '/^Distribution: / {print $2}')
    # Use the unstable suite for Debian experimental packages.
    if [ "${DIST}" == "experimental" ]; then
        DIST="$UNSTABLE_CODENAME"
    fi
fi

# Optionally set a default distribution if none is used. Note that you can set
# your own default (i.e. ${DIST:="unstable"}).
: ${DIST:="$(lsb_release --short --codename)"}

# Optionally change Debian or LinuxMint codenames in $DIST to their aliases.
case "$DIST" in
    $UNSTABLE_CODENAME)
        DIST="sid"
        ;;
    $TESTING_CODENAME)
        DIST="stretch"
        ;;
    $STABLE_CODENAME)
        DIST="jessie"
        ;;
    "maya")
        DIST="precise"
        ;;
    "qiana")
        DIST="trusty"
        ;;
    "rebecca")
        DIST="trusty"
        ;;
    "rafaela")
        DIST="trusty"
        ;;
    "rosa")
        DIST="trusty"
        ;;
    "sarah")
        DIST="xenial"
        ;;
    "serena")
        DIST="xenial"
        ;;
    "sonya")
        DIST="xenial"
        ;;
    "sylvia")
        DIST="xenial"
        ;;
    "tara")
        DIST="bionic"
        ;;
    "tessa")
        DIST="bionic"
        ;;
    "tina")
        DIST="bionic"
        ;;
    "tricia")
        DIST="bionic"
        ;;
esac
echo "Distribution DIST Codename is: $DIST"

# Optionally set the architecture to the host architecture if none set. Note
# that you can set your own default (i.e. ${ARCH:="i386"}).
: ${ARCH:="$(dpkg --print-architecture)"}

NAME="$DIST"
if [ -n "${ARCH}" ]; then
    NAME="$NAME-$ARCH"
    DEBOOTSTRAPOPTS=("--arch" "$ARCH" "${DEBOOTSTRAPOPTS[@]}")
fi
BASETGZ="/var/cache/pbuilder/$NAME-base.tgz"
DISTRIBUTION="$DIST"
BUILDRESULT="/var/cache/pbuilder/$NAME/result/"
APTCACHE="/var/cache/pbuilder/$NAME/aptcache/"
BUILDPLACE="/var/cache/pbuilder/build/"

if $(echo ${DEBIAN_SUITES[@]} | grep -q $DIST); then
    # Debian configuration
    MIRRORSITE="http://$DEBIAN_MIRROR/debian/"
    COMPONENTS="main contrib non-free"
elif $(echo ${UBUNTU_SUITES[@]} | grep -q $DIST); then
    # Ubuntu configuration
    MIRRORSITE="http://$UBUNTU_MIRROR/ubuntu/"
    COMPONENTS="main restricted universe multiverse"
else
    echo "Unknown distribution: $DIST"
    exit 1
fi

#############################
# Local mods below here. JM #
#############################

# Work under ~/pbuilder/
BASETGZ=~/pbuilder/"$NAME-base.tgz"
BUILDRESULT=~/pbuilder/"${NAME}_result/"
BUILDPLACE=~/pbuilder/build/
APTCACHE=~/pbuilder/"${NAME}_aptcache/"
APTCACHEHARDLINK=no

## Use local packages from ~/pbuilder/deps
#DEPDIR=~/pbuilder/deps
#OTHERMIRROR="deb file://"$DEPDIR" ./"
#BINDMOUNTS=$DEPDIR
HOOKDIR=~/pbuilder/hooks
EXTRAPACKAGES="wget libgtk-3-dev libwxgtk3.0-dev" #"apt-utils"

if [ x"$DIST" = x"lucid" ];then
    # Running a Ubuntu Lucid chroot.  Add lucid-updates mirror site too.
    OTHERMIRROR="deb http://us.archive.ubuntu.com/ubuntu/ ${DIST}-updates main restricted multiverse universe"
fi
echo "******************************************************************************"
echo " Finished executing the $HOME/.pbuilderrc pbuilder setup script "
echo "******************************************************************************"
EOF
# EOF of $HOME/.pbuilderrc

# Always install/update the pbuilder A05suffix hook to add a distro suffix to package versions
mkdir -p ${PBUILDFOLDER}$HOOKSDIR
# Note: Using << "EOF" to create HERE DOC parameter expansion is suppressed; 
#       Using << EOF to create HERE DOC parameter expansion happens
echo -e "\nCreating A05suffix hook file at: $PBUILDFOLDER$HOOKSDIR/"
  cat >${PBUILDFOLDER}$HOOKSDIR/A05suffix <<"EOF"
#!/bin/bash
# pbuilder hook for adding distro name to package version
#
# Neil Mayhew - 2010-12-08
# Jonathan Marsden - 2012-09-23 Added sed and changed build location
# whm - 2015-06-24 Tweaked to recognize LinuxMint DIST aliases
# whm - 2016-04-20 Added rosa => trusty and sarah => xenial DISTs
# whm - 2016-12-09 Added serena => xenial DIST
# whm - 2017-01-14 Always generates A05suffix hook and uses sed to update existing DEBFULLNAME and DEBEMAIL
# whm - 2018-04-17 Corrected to cd to /tmp/buildd/*/

echo ""
echo "******************************************************************************"
echo " Begin executing the A05suffix pbuilder hook script "
echo "******************************************************************************"

TYPE=$(lsb_release -si)
if [ x"$TYPE" = x"LinuxMint" ]; then
  TYPE="Ubuntu"
fi
DIST=$(lsb_release -sc)
case "$DIST" in
    $UNSTABLE_CODENAME)
        DIST="sid"
        ;;
    $TESTING_CODENAME)
        DIST="stretch"
        ;;
    $STABLE_CODENAME)
        DIST="jessie"
        ;;
    "maya")
        DIST="precise"
        ;;
    "qiana")
        DIST="trusty"
        ;;
    "rebecca")
        DIST="trusty"
        ;;
    "rafaela")
        DIST="trusty"
        ;;
    "rosa")
        DIST="trusty"
        ;;
    "sarah")
        DIST="xenial"
        ;;
    "serena")
        DIST="xenial"
        ;;
    "sonya")
        DIST="xenial"
        ;;
    "sylvia")
        DIST="xenial"
        ;;
    "tara")
        DIST="bionic"
        ;;
    "tessa")
        DIST="bionic"
        ;;
    "tina")
        DIST="bionic"
        ;;
    "tricia")
        DIST="bionic"
        ;;
esac
USER=$(id -un)
HOST=$(uname -n)
export DEBFULLNAME="pbuilder"
export DEBEMAIL="$USER@$HOST"

if [ "$TYPE" = Ubuntu ]
then
    echo ""
    echo "******************************************************************************"
    echo " Add $DIST to package version and remove trailing 1 from distribution suffix"
    echo "******************************************************************************"
    echo "ls -la /tmp/buildd/"
    ls -la /tmp/buildd/*/
    PWDDIR=`pwd`
    echo "Current pwd Dir is: $PWDDIR"
    echo "Changing dir before debchange..."
    #cd ~/*/
    cd /tmp/buildd/*/
    PWDDIR=`pwd`
    echo "Current pwd Dir is: $PWDDIR"
    debchange --local=+$DIST "Build for $DIST"
    # Remove unwanted trailing 1 from distribution suffix
    sed -i -e "1s/+${DIST}1/+${DIST}/" debian/changelog
fi
echo "******************************************************************************"
echo " Finished A05suffix script and adding DIST name to package version"
echo "******************************************************************************"
EOF
# end of EOF ${PBUILDFOLDER}$HOOKSDIR/A05suffix

# whm - 2017-01-14 Added. For initial setup and creation of the user's pbuilder/hooks/A05suffix
# file, we always generate the file using the HERE DOC, and substitute any DEBFULLNAME and DEBEMAIL
# values that existed in the environment at the time this release.sh script is executed.
# 
# Test if SAVEDDEBFULLNAME contains anything other than null or "pbuilder"; if so use sed to 
# substitute that existing value into the A05suffix hooks file; if not, query user for the 
# correct value.
if [ "x$SAVEDDEBFULLNAME" = "x" ] || [ x"$SAVEDDEBFULLNAME" = x"pbuilder" ]; then
  echo -e "\n"
  read -p "What is your DEBFULLNAME for authentication in debian packaging? " DebFullName
  # Replace current value of DEBFULLNAME with user-specified $DebFillName value
  DEBFULLNAME="$DebFullName"
fi
# Test if SAVEDDEBFULLNAME contains anything other than null or "pbuilder"; if so use sed to 
# substitute that existing value into the A05suffix hooks file; if not, query user for the 
# correct value.
if [ "x$SAVEDDEBEMAIL" = "x" ] || [ x"$SAVEDDEBEMAIL" = x"$USER@$HOST" ]; then
  echo -e "\n"
  read -p "What is your DEBEMAIL for authentication in debian packaging? " DebEmail
  # Replace current value of DEBEMAIL with user-specified $DebEmail value
  DEBEMAIL="$DebEmail"
fi
sed -i "s/\(DEBFULLNAME *= *\).*/\1\"$DEBFULLNAME\"/" ${PBUILDFOLDER}$HOOKSDIR/A05suffix
sed -i "s/\(DEBEMAIL *= *\).*/\1\"$DEBEMAIL\"/" ${PBUILDFOLDER}$HOOKSDIR/A05suffix
chmod 0755 ${PBUILDFOLDER}$HOOKSDIR/A05suffix
export DEBFULLNAME
export DEBEMAIL

echo -e "\nExisting value of DEBFULLNAME is: $SAVEDDEBFULLNAME"
echo      "Existing value of DEBEMAIL is: $SAVEDDEBEMAIL"
DebFullNameInA05suffix=`grep "DEBFULLNAME=" ${PBUILDFOLDER}$HOOKSDIR/A05suffix`
DebEmailInA05suffix=`grep "DEBEMAIL=" ${PBUILDFOLDER}$HOOKSDIR/A05suffix`
echo -e "\nValues of DEBFULLNAME and DEBEMAIL exported from ~/pbuilder/hooks/A05suffix:"
echo "   $DebFullNameInA05suffix"
echo "   $DebEmailInA05suffix"

# Install build dependencies
echo -e "\nInstalling build dependencies"
sudo apt-get install $BUILDDEPS -y

# Check out the code
# whm 2015-06-24 modified to do the release packaging in ~/packaging
# otherwise the packaging related stuff will go into the dir from which
# release.sh is being called from (the ~/projects/adaptit/scripts/ dir).
echo -e "\nPreparing an adaptit repository at $PACKAGING_DIR/adaptit"
mkdir -p $PACKAGING_DIR

# whm - Modified 2015-06-30 Check for an existing adaptit git repo on the current machine
# first at ~/packaging/adaptit from previous packaging efforts. If the repo is found there, 
# we can use it for packaging the current release. We execute a 'git pull' on the repo 
# at ~/packaging/adaptit before continuing with the packaging process. 
# If no adaptit repo exits that we can reuse, we just do the git clone operation.
# 
cd $PACKAGING_DIR

# Check for a git 'adaptit' repo in ~/packaging/ that we might be able to update and use.
echo -e "\nChecking for a git repo at: $PACKAGING_DIR/adaptit"
if [ -f "$PACKAGING_DIR/adaptit/.git/config" ]; then
  echo "A repo was found at: $PACKAGING_DIR/adaptit"
  cd $PACKAGING_DIR/adaptit
  # The repo shouldn't have any code changes, but do a git reset --hard just in case
  echo "Doing 'git reset --hard' on the repo"
  git reset --hard
  # Bring the local repo up to date
  echo "Doing 'git pull' on the repo"
  git pull
else
  echo -e "\nCloning the Adapt It Desktop (AID) sources..."
  echo      "  from: $AID_GITURL"
  echo      "  to $PACKAGING_DIR/adaptit"
  [ -d adaptit ] || git clone $AID_GITURL
fi

# Remove any existing adaptit-<tag> packaging repos if any already exist
echo -e "\nRemoving any existing packaging repos at: $PACKAGING_DIR/adaptit-*"
rm -rf $PACKAGING_DIR/adaptit-*

# At this point there should be an up-to-date repo at ~/packaging/adaptit
# It will function as our backup repo for packaging purposes. We'll make
# a copy of this repo, renaming it with the desired tag as a suffix which
# will function as our new branch for the git checkout command below.
# First, figure out which release to build -- default is latest numbered release
cd $PACKAGING_DIR/adaptit
RELEASE=${1:-$(git describe --tags $(git rev-list --tags --max-count=1))}
RELEASE=${RELEASE#adaptit-}    # Remove any leading adaptit- prefix
echo -e "\nThe tag we're using for packaging is: $RELEASE"

# whm modified 2015-07-03 to not just rename the adaptit dir to adaptit-${RELEASE} 
# but to copy/sync the adaptit dir to adaptit-${RELEASE}. This leaves a copy of
# the adaptit repo in ~/packaging/ that can be used for future release packaging
# if there is no current repo at ~/projects/adaptit/ to draw from.
echo -e "\nSyncing repo with rsync..."
echo      "  from $PACKAGING_DIR/adaptit/"
echo      "  to $PACKAGING_DIR/adaptit-${RELEASE}"
rsync -aq --delete --ignore-times --checksum --exclude="build_*" $PACKAGING_DIR/adaptit/ $PACKAGING_DIR/adaptit-${RELEASE}

# Check out the desired release from git
echo -e "\nCreate new git branch name and start at: adaptit-${RELEASE}"
cd $PACKAGING_DIR/adaptit-${RELEASE}
#git checkout tags/${RELEASE} -b ${RELEASE} || exit 1
git checkout -b ${RELEASE} adaptit-${RELEASE} || exit 1

# call autogen.sh in bin/linux to recreate the build environment
cd $PACKAGING_DIR/adaptit-${RELEASE}/bin/linux
echo -e "\nCall autogen.sh to recreate the build environment"
pathbefore=`pwd`
echo "Path before autogen.sh: $pathbefore"
./autogen.sh

#cd ..
cd $PACKAGING_DIR
pathafter=`pwd`
echo "Path after autogen.sh: $pathafter"

# Delete unwanted non-source files here
echo -e "\nRemoving unwanted non-source files from adaptit-${RELEASE}"
#find adaptit-${RELEASE} -type f -iname "*.hhc" -delete
find adaptit-${RELEASE} -type f -iname "*.dll" -delete
find adaptit-${RELEASE} -type f -iname "*.exe" -delete
find adaptit-${RELEASE} -type f -iname "bin2c" -delete
if [ -d adaptit-${RELEASE}/.git ]; then rm -rf adaptit-${RELEASE}/.git; fi
if [ -d adaptit-${RELEASE}/bin/source ]; then rm -rf adaptit-${RELEASE}/bin/source; fi
if [ -f adaptit-${RELEASE}/.gitignore ]; then rm adaptit-${RELEASE}/.gitignore; fi
if [ -f adaptit-${RELEASE}/.travis.yml ]; then rm adaptit-${RELEASE}/.travis.yml; fi

# Tar it up and create symlink for .orig.bz2
echo -e "\nTar up the release and create symlink for .orig.bz2"
tar jcf adaptit-${RELEASE}.tar.bz2 adaptit-${RELEASE} || exit 3
if [ -h adaptit_${RELEASE}.orig.tar.bz2 ]; then
  echo "Link: adaptit_${RELEASE}.orig.tar.bz2 already exists"
else
  echo "Creating link to target: adaptit_${RELEASE}.orig.tar.bz2"
  ln -s adaptit-${RELEASE}.tar.bz2 adaptit_${RELEASE}.orig.tar.bz2
fi

# Do an initial unsigned source build in host OS environment
echo -e "\nDo initial unsigned source build in host OS environment"
cd adaptit-${RELEASE}
# whm added Inject LINTIAN_PROFILE=ubuntu into the debuild environment
LINTIAN_PROFILE=ubuntu debuild -eLINTIAN_PROFILE -S -sa -us -uc || exit 4
# whm added 25Jun2015 get the build suffix from /debian/changelog to use in pbuilder build below
buildSuffix=`dpkg-parsechangelog | awk '/^Version: / {print $2}' | cut -d '-' -f2`
echo -e "\nBuild Suffix: $buildSuffix"

cd $PACKAGING_DIR

for i in $OSRELEASES; do
  export DIST=$i	# $DIST is used in .pbuilderrc
  # Ensure pbuilder-dist symlinks for relevant releases exist
  echo -e "\nEnsuring pbuilder-dist symlinks for relevant releases exist"
  [ -L /usr/bin/pbuilder-$i ] || (cd /usr/bin ; sudo ln -s pbuilder-dist pbuilder-$i)
  [ -L /usr/bin/pbuilder-$i-i386 ] || (cd /usr/bin ; sudo ln -s pbuilder-dist pbuilder-$i-i386)

  # Create pbuilder chroots if they do not already exist
  echo -e "\nCreating pbuilder chroots if they don't already exist"
  [ -f ${PBUILDFOLDER}/$i-base.tgz ] || pbuilder-$i create
  [ -f ${PBUILDFOLDER}/$i-i386-base.tgz ] || pbuilder-$i-i386 create

  # Update and clean pbuilder chroots, add extra packages so A05suffix hook works
  echo -e "\nUpdating & cleaning pbuilder chroots, add extrapackages so A05suffix hook works"
  pbuilder-$i update --extrapackages "lsb-release devscripts"
  pbuilder-$i-i386 update --extrapackages "lsb-release devscripts"
  pbuilder-$i clean
  pbuilder-$i-i386 clean
  
  # TODO: Pass an option on Xenial that will change build configuration of AID to use 
  # static and the custom wx library?
  # Or, instead of working with the A06makewx hook script:
  # Try calling pbuilder --execute <relative-path-to>/adaptit/scripts/makewx.sh "release" 
  # for xenial and higher dists. Pbuilder doc says about its --execute command:
  # "Execute a script or command inside the chroot, in a similar manner to --login 
  # The file specified in the command-line argument will be copied into the chroot, and invoked.
  # The remaining arguments are passed on to the script."

  # Build in each pbuilder
  echo -e "\nBuilding in each pbuilder with command:"
  echo "  pbuilder-$i build adaptit_${RELEASE}-$buildSuffix.dsc"
  pbuilder-$i build adaptit_${RELEASE}-$buildSuffix.dsc
  echo -e "\nBuilding in each pbuilder with command:"
  echo "  pbuilder-$i-i386 build --binary-arch adaptit_${RELEASE}-$buildSuffix.dsc"
  pbuilder-$i-i386 build --binary-arch adaptit_${RELEASE}-$buildSuffix.dsc

  # mkfit -p adaptit-debs-${RELEASE}
  #mv -v $(find ${PBUILDFOLDER}/*_result -name "adaptit*${RELEASE}*${DIST}*.deb") adaptit-debs-${RELEASE}/

  # whm changed 2015-07-03 If $DIST-amd64_result dir exists, copy new amd64 packages there from $DIST_result,
  # and remove the $DIST_result dir. If $DIST-amd64_result dir doesn't yet exist, just rename the $DIST_result
  # folder to $DIST-amd64_result. Having amd64 package files in a dir with amd64 in its name has more clarity.
  if [ -d "${PBUILDFOLDER}/${DIST}_result" ]; then
    if [ -d "${PBUILDFOLDER}/${DIST}-amd64_result" ]; then
      echo -e "\nThe ${DIST}-amd64_result already exists."
      echo "Copying/Syncing package files from ${DIST}_result to ${DIST}-amd64_result..."
      cd ${PBUILDFOLDER}
      rsync -aq --update "${DIST}_result/" "${DIST}-amd64_result"
      if [ "$?" -eq 0 ]; then
        echo "and removing the ${DIST}_result dir."
        rm -rf "${DIST}_result"
      fi
      cd $PACKAGING_DIR
    else
      cd ${PBUILDFOLDER}
      echo -e "\nRenaming the ${DIST}_result dir to ${DIST}-amd64_result"
      mv "${DIST}_result" "${DIST}-amd64_result"
      cd $PACKAGING_DIR
    fi
  fi
done

echo -e "\n"
echo -e "$0: Completed. Adaptit version ${RELEASE} package files may be found in:"
find ${PBUILDFOLDER}/*_result -type d
