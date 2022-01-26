#!/bin/bash
# debupload.sh -- uploads .deb files for adaptit version provided as parameter to packages.sil.org
#
# Author: Jonathan Marsden <jmarsden@fastmail.fm>
#
# Date: 2012-10-19
# Revised by Bill Martin <bill_martin@sil.org>
#  - Display help for usage if no version number parameter is supplied

if [ -z "$1" ]
then
  echo -e "\nScript was called without parameters."
  echo "Script Usage:"
  echo "  ./debupload.sh <release-number>"
  echo "for example:  ./debupload.sh 6.5.9"
  echo "Notes: This script is designed to be used after the release packages for"
  echo "adaptit have been created using the release.sh script and after the packages"
  echo "have been signed by calling the debsign.sh script."
  exit 1
fi

# Is packages.sil.org in ~/.dput.cf ?  If not, add it
if grep -sq packager.lsdev.sil.org ~/.dput.cf ; then
 : # No addition to ~/.dput.cf is needed
else
  # Add packages.sil.org to ~/.dput.cf
cat <<"EOF" >>~/.dput.cf
[packager]
method	= rsync
fqdn    = packager.lsdev.sil.org
login   = upload
incoming= %(packager)s
EOF
fi

# Now do the actual uploading using dput. The form of the command for uploading
# a single distribution's packages at the command line is (for example, uploading 
# the trusty amd64 packages):
#   cd ~/pbuilder/trusty-amd64_result/
#   dput packager:ubuntu adaptit_6.10.5-1+trusty_amd64.changes
# Note: dput will ask for your passphrase for the public ssh signing key you supplied to PSO.
# The above single dist command may fail. If so, just remove any unwanted
# *_result folders from the pbuilder folder and run this debupload.sh scrip with
# the appropriate version number as its only parameter.
# The command/script should end with the output: "Successfully uploaded packages."

for i in $(find ~/pbuilder/*_result -type f -name "adaptit_*+*.changes")
do
  dput packager:ubuntu/$(echo $i |sed -e 's/^.*+//' -e 's/_.*$//') $i
  LASTERRORLEVEL=$?
  if [ $LASTERRORLEVEL != 0 ]; then
     echo -e "\nCould not upload $i to packager.lsdev.sil.org - dput error: $LASTERRORLEVEL"
     echo "Aborting upload..."
     exit $LASTERRORLEVEL
  fi
done
