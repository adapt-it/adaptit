#!/bin/bash
PBUILDFOLDER=${PBUILDFOLDER:-~/pbuilder}
RELEASES="lucid maverick natty oneiric precise quantal"

# check out the desired release from svn
svn checkout http://adaptit.googlecode.com/svn/tags/adaptit-${1} adaptit

# Export it ready for creating a source tarball
cd adaptit
svn export . ../adaptit-${1}

# Delete unwanted files here using find and rm
cd ../adaptit-${1}
#find . -type f -iname "*.hhc" -delete
find . -type f -iname "*.dll" -delete
find . -type f -iname "*.exe" -delete

# Tar it up and create symlink for .orig.bz2
cd ..
tar jcf adaptit-${1}.tar.bz2 adaptit-${1}
ln -s adaptit-${1}.tar.bz2 adaptit_${1}.orig.tar.bz2

# Do an initial source build in host OS environment
cd adaptit-${1}
debuild -S -sa

for i in $RELEASES; do
  # Ensure pbuilder-dist symlinks for relevant releases exist
  [ -L /usr/bin/pbuilder-$i ] || (cd /usr/bin ; sudo ln -s pbuilder-dist pbuilder-$i)
  [ -L /usr/bin/pbuilder-$i-i386 ] || (cd /usr/bin ; sudo ln -s pbuilder-dist pbuilder-$i-i386)

  # Create pbuilder chroots if they do not already exist
  [ -f ${PBUILDFOLDER}/$i-base.tgz ] || pbuilder-$i create
  [ -f ${PBUILDFOLDER}/$i-i386-base.tgz ] || pbuilder-$i-i386 create

  # Update and clean pbuilder chroots
  pbuilder-$i update
  pbuilder-$i-i386 update
  pbuilder-$i clean
  pbuilder-$i-i386 clean

  # Build in each pbuilder
  pbuilder-$i build adaptit_${1}-1.dsc
  pbuilder-$i-i386 build adaptit_${1}-1.dsc
done
