#!/bin/bash
# debsign.sh -- sign all binary adaptit packages with +distro suffixes under ~/pbuilder
#
# Author: Jonathan Marsden <jmarsden@fastmail.fm>
#
# Date: 2012-10-05
# Revised by Bill Martin <bill_martin@sil.org> to provide a little error handling/info

echo -e "\nSearching for adaptit *.changes package files in ~/pbuilder/*_result dirs"

for i in $(find ~/pbuilder/*_result -type f -name "adaptit_*+*.changes")
do
  debsign --re-sign $i
  LASTERRORLEVEL=$?
  if [ $LASTERRORLEVEL != 0 ]; then
     echo -e "\nCould not sign $i debsign error: $LASTERRORLEVEL"
     echo "Aborting debsign.sh..."
     exit $LASTERRORLEVEL
  fi
done 
