#!/bin/bash
# debupload.sh -- uploads .deb files for adaptit version provided as parameter to packages.sil.org
#
# Author: Jonathan Marsden <jmarsden@fastmail.fm>
#
# Date: 2012-10-05

# Is packages.sil.org in ~/.dput.cf ?  If not, add it
if grep -sq packager.lsdev.sil.org ~/dput.cf ; then
 : # No addition to dput.cf is needed
else
  # Add packages.sil.org to ~/dput.cf
cat >>EOF
[packager]
method	= rsync
fqdn    = packager.lsdev.sil.org
login   = upload
incoming= %(packager)s
EOF
fi

# Now do the actual uploading
for i in $(find ~/pbuilder/*_result -type f -name "adaptit_*+*.changes")
do
  dput packages:ubuntu/$(echo $i |sed -e 's/^.*+//' -e 's/_.*$//') $i
done
