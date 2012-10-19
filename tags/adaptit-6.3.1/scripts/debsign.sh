#!/bin/bash
# debsign.sh -- sign all binary adaptit packeges with +distro suffixes under ~/pbuilder
#
# Author: Jonathan Marsden <jmarsden@fastmail.fm>
#
# Date: 2012-10-05

for i in $(find ~/pbuilder/*_result -type f -name "adaptit_*+*.changes")
do
  debsign $i
done 