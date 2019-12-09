#!/bin/bash
# diff-this-file.sh -- Does a 'git diff origin/master -- $1' on a given file in the current git repository.
#          -- Assumes the current directory is the local active git repository. The path to $1 needs to be
#             the relative path from the current dir to the file you want to diff against the external repo.
#
# Author: Bill Martin <bill_martin@sil.org>
#
# Date: 2015-06-27

if [ $# -eq 0 ]; then
  echo "The diff-this-file.sh script was invoked without any parameters:"
  echo "Usage: Call with one or more parameters representing the file(s) you"
  echo "  want to get diffs of, for example:"
  echo "  ./diff-this-file.sh ../source/Adapt_It.cpp"
  echo "Assumes the current directory is in the local active git repository."
  echo "Use of wild cards for files to diff is OK."
  echo "The path of the file(s) to diff needs to be the relative path from"
  echo "  the current dir, to the file(s) you want to get the diff against."
  exit 1
fi
git diff origin/master -- $@

