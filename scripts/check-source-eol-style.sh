#!/bin/bash
# check-source-eol-style.sh -- checks all source files have svn:eol-style set to native

SVNURL=http://adaptit.googlecode.com/svn/trunk/source	# Where's the code?
LOCALSVNBASEDIR=~/subversion				# Where to work with svn locally
LOCALREPO=$(basename $0 .sh)				# Directory name of local copy
SOURCEFILEREGEX='\.c$|\.cpp$|\.h$'			# Which files are source files?

# Create or update local svn checkout
if [ -d "${LOCALSVNBASEDIR}/${LOCALREPO}" ]; then
  # Update existing checked out copy
  cd ${LOCALSVNBASEDIR}/${LOCALREPO}
  svn -q update
else
  # Grab a fresh copy
  cd ${LOCALSVNBASEDIR}
  svn -q co ${SVNURL} ${LOCALREPO}
fi

# Check each source file
cd ${LOCALSVNBASEDIR}/${LOCALREPO}
svn ls -R . | (while read i
do
  [ -f "$i" ] || continue 			# Ignore unless a file
  [[ "$i" =~ ${SOURCEFILEREGEX} ]] || continue	# Ignore unless a source file
  props=$(svn -q proplist "$i" )		# Does it have properties
  if [ -z "$props" ]; then
    # Alert that it is missing
    echo WARNING: $SVNURL/$i lacks eol-style. Fix with svn propset svn:eol-style native $i
  else
    if [[ "$props" =~ .*svn:eol-style$ ]]; then
      # Get the svn:eol-style property value
      eolstyle=$(svn propget svn:eol-style $i)
      # Alert if it is not native
      [ "$eolstyle" == "native" ] || echo ERROR: $SVNURL/$i has eol-style $eolstyle. Fix with svn propset svn:eol-style native $i
    fi
  fi
done)
