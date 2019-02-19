#!/bin/bash
# convertIANA-registry.sh -- downloads the latest IANA language codes from:
# https://www.iana.org/assignments/language-subtag-registry/language-subtag-registry
# and processes the IANA list into a more concise form writing it to iso639-3codes.txt
# in the project's docs directory, where Installers/packaging routines can find it.
# Adapt It reads it and displays the contents of iso639-3codes.txt to the user within 
# the LanguageCodesDlg dialog's list of language codes.
# Parameter (optional):
# Can take one optional parameter $1 which, if used, overrides the default output path
# of "../docs/" for the output location of the iso639-3codes.txt file. For example "."
# would output the iso639-3codes.txt file to the same directory the script is executed 
# from; "~/Desktop/" would output the iso639-3codes.txt file to the desktop.
# Author: Bill Martin <bill_martin@sil.org>
# Date: 2019-02-09
#
# Comments:
#The original IANA file looks like the following (with %% delimiting separate records):
#
#File-Date: 2018-11-30 [first line in the file is always File-Data...]
#%%
#Type: language
#Subtag: aa
#Description: Afar
#Added: 2005-10-16
#%%
#Type: language
#Subtag: ab
#Description: Abkhazian
#Added: 2005-10-16
#Suppress-Script: Cyrl
# ... [2-letter language Subtag codes continue until about line 1161]
#%%
#Type: language
#Subtag: aaa
#Description: Ghotuo
#Added: 2009-07-29
#%%
#Type: language
#Subtag: aab
#Description: Alumu-Tesu
#Added: 2009-07-29
# ... [to line 43823 are remaining 8000+ 3-letter 'Type: language' and 'Type: extlang' Subtag data records after this point!]
# ... [lines 43824 to 47757 end-of-file are other records of 'Type: script', 'Type: region', 'Type: variant', 
#      'Type: grandfathered' and 'Type: redundant', that we arent' interested in and are left out of output]
#
# The processed file output to the iso639-3codes.txt using this script will look like the
# following, with abbreviated records - each record on a singl line, including a tab to 
# delimit the code from the remaining fields (delimited by spaces):
#
#Id	Print_Name [File-Date: <yyyy-mm-dd> field changed to 'Id	Print_Name' header line for Adapt It use]
#aa	Afar
#ab	Abkhazian
# ... [2-letter codes continue to line 191; one record per line = 190 2-letter codes, 6 of which are "deprecated"]
#aaa Ghotuo
#aab Alumu-Tesu
# ... [3-letter codes continue to line 8375; one record per line = 8375 records total]
# 
# Note: Some records in final output, especially the extlang records will have more 
# extended comments, notes about deprecated codes, etc.

IANA_LANG_SUBTAG_REGISTRY_URL="https://www.iana.org/assignments/language-subtag-registry/language-subtag-registry"
iso639_3CodesFileName="iso639-3codes.txt"
DOCS_DIR="../docs/" # adaptit/docs dir relative to the adaptit/scripts dir.
# Accept only a parameter representing a valid path
if [[ "x$1" != "x" ]] && [[ -d "$1" ]]; then
  # A parameter $1 was input to the script
  # Ensure the parameter path ends with a slash /
  TEMPDIR="${1%/}"
  #echo "Debug: TEMPDIR is [$TEMPDIR]"
  DOCS_DIR="$TEMPDIR/"
  #echo "Debug: DOCS_DIR is [$DOCS_DIR]"
fi
echo -e "\nOutput to $DOCS_DIR""iso639_3CodesFileName"
sleep 3s
DELIMITER="%%"
COUNT=0
OUTFILE=$DOCS_DIR$iso639_3CodesFileName

DOWNLOAD_DATE=`date +%Y-%m-%d@%H:%M:%S`
start=`date +%s` # keep track of run-time of script

# Test for Internet access; warn and abort if www.iana.org cannot be accessed
ping -c1 -q www.iana.org
if [ "$?" != 0 ]; then
  echo -e "\n****** WARNING ******"
  echo -"Internet access to www.iana.org is not currently available."
  echo "This script cannot continue without access to the Internet."
  echo "Make sure the computer has access to the Internet, then try again."
  echo "****** WARNING ******"
  echo "Aborting..."
  exit 1
fi

# wget options:
# -q quiet
# -O allows us to specify the name of the file into which wget dumps the page contents. 
# - wget dumps to standard output, and collect that into the variable RAW_DATA.
RAW_DATA=$(wget https://www.iana.org/assignments/language-subtag-registry/language-subtag-registry -q -O -)

# first backup the $iso639_3CodesFileName file, if it exists, to $iso639_3CodesFileName.bak
if [[ -f "$OUTFILE" ]]; then
  mv --force $OUTFILE $OUTFILE".bak"
fi
#Split the String into substring records. RAW_DATA delimits records by %%.
IFS='%%' s_split=($RAW_DATA)
echo "Total preprocess Lines - ${#s_split[@]}"
IANA_FILE_DATE=$s_split # Assigns 1st array element to IANA_FILE_DATE variable (has a final \n)
IANA_FILE_DATE=${IANA_FILE_DATE//$'\n'/} # Remove any (final) newlines
echo -e "\nIANA Data $IANA_FILE_DATE"
sleep 3s

echo -e "\n******************************************************************************"
echo -e "ID\tPrint_Name IANA data $IANA_FILE_DATE - download date: $DOWNLOAD_DATE" | tee -a $OUTFILE
for i in ${s_split[@]}
do
    i=${i//$'\n'/$' '} # Replace all newlines with spaces.
    i="$(echo "$i" | awk '{$1=$1};1')" # remove leading and trailing space or tab chars, squeeze multiple spaces/tabs to single space/tab
    # We ignore all records of 'Type: script', 'Type: region', 'Type: variant', 'Type: grandfathered' and 'Type: redundant',
    # and select out only records of 'Type: language' and 'Type: extlang'.
    if [[ "$i" == "Type: language "* ]] || [[ "$i" == "Type: extlang "* ]]; then
      # Only process records that are prefixed with "Type: language Subtag: " and "Type: extlang Subtag: "
      if [[ "$i" == "Type: language "* ]]; then
        i=${i#"Type: language Subtag: "} # Remove "Type: language Subtag: " from the beginning of the array item
      fi
      if [[ "$i" == "Type: extlang "* ]]; then
        i=${i#"Type: extlang Subtag: "} # Remove "Type: extlang Subtag: " from the beginning of the array item
      fi
      i=${i//"Description: "} # remove all instances of this string from $i
      # Remove the " Added: <date>" part but leave the remaining fields (comments preferred, etc)
      i=${i//" Added: "????-??-??}  # remove all instances of this string from $i, where ????-??-?? is the date to be removed also.
      # Replace the first space with a tab, so that there is a tab char between the code and the remainder of the string.
      i=$(echo "$i" | sed 's/ /'$'\t''/')
      let COUNT=COUNT+1
      # Output to the iso639-3codes.txt file and tee to console
      echo "$i" | tee -a $OUTFILE
    fi
done
echo -e "\n******************************************************************************"
echo "No of Lines output to $OUTFILE: $COUNT"
echo "Script running time: $((($(date +%s)-$start))) seconds"

