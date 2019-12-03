#!/bin/bash
# sedConvert.sh -- Tweaks wxDesigner C++ code to work with wxWidgets-3.1.x
# sedCommandFile.txt -- companion file containing sed string replacement commands
# Author: Graeme Costin <adaptit@costincomputingservices.com.au>
# Date: 2019-01-28
# Revised by Bill Martin <bill_martin@sil.org> to incorporate into the adaptit repo.
# Revised by Bill Martin 18Feb2019:
#  - For sedCommandFile.txt testing, sedConvert.sh will accept two optional parameters
#    that allow retrieval of source files from a dir outside the adaptit tree
#    and write the changed source files to another dir outside the adaptit tree:
#      $1 is dir of source files to fetch into the current directory for processing,
#        usually a dir where processing results are not written to, so that each
#        test run using parameters gets the same original (unprocessed) files.
#        For example, $1 might be ~/Desktop/testdir or relative dir ../testdir
#      $2 is dir of destination to write the changed files - usually the current 
#        dir ./ but can be any valid directory to receive redirected 1> output.
#        For example, $2 might be ./ or relative dir ../outputdir
#Usage: ./sedConvert.sh [<FECTH_DIR>] [<OUTPUT_DIR>]
#
# Add any new source files to process to the $FILE_NAMES_TO_PROCESS string list 
# below leaving a space between file names.
FILE_NAMES_TO_PROCESS="Adapt_It_wdr.cpp ExportSaveAsDlg.cpp PeekAtFile.cpp"
HEADER_LINE="This file was processed by the sedConvert.sh script"
FETCH_DIR="../source/" # May be changed if parameter $1 is used.
OUTPUT_DIR="../source/" # May be changed if parameter $2 is used.

# Check for optional parameter $1 representing a valid directory to fetch files from.
if [[ "x$1" != "x" ]] && [[ -d "$1" ]]; then
  # A parameter $1 was input to the script indicating a valid dir to fetch the files from.
  # Ensure the parameter path ends with a slash /
  FETCH_DIR="${1%/}"
  FETCH_DIR="$FETCH_DIR""/"
  echo "Debug: FETCH_DIR is [$FETCH_DIR]"
  # Copy the $FILE_NAMES_TO_PROCESS files from $1 to the current directory ./
  for file in $FILE_NAMES_TO_PROCESS
  do
    # copy the file to the current dir where the script is located/run from.
    cp "$FETCH_DIR$file" .
  done
fi

# Check for optional parameter $2 representing a valid output path
if [[ "x$2" != "x" ]] && [[ -d "$2" ]]; then
  # A parameter $1 was input to the script indicating a valid dir to fecth the files from.
  # Ensure the parameter path ends with a slash /
  OUTPUT_DIR="${2%/}"
  OUTPUT_DIR="$OUTPUT_DIR/"
  echo "Debug: OUTPUT_DIR is [$OUTPUT_DIR]"
fi

for file in $FILE_NAMES_TO_PROCESS
do
  grep -Fq "$HEADER_LINE" $OUTPUT_DIR$file
  GREPRESULTINT=$?
  if [ $GREPRESULTINT -ne 0 ]; then
    # grep returned non-0 result indicating the HEADER_LINE was not found in the file. 
    # so the file was not previously processed with sedConvert.sh, but was recently 
    # exported as C++ from wxDesigner, and so we need to process it with sed to tweak 
    # it for use with wxWidgets-3.1.x.
    # The next 2 lines were for debugging:
    #line=$(head -n 1 $OUTPUT_DIR$file)
    #echo "Debug: line is [$line]"
    echo "Processing wxDesigner's C++ code in $OUTPUT_DIR$file for wxWidgets-3.1.x builds"
    # Use sed to process text substitution commands from sedCommandFile.txt in-place
    # sed options:
    # -i sed command does "in-place" editing
    # -f sed uses the patterns in sedCommandFile.txt
    sed -i -f sedCommandFile.txt $OUTPUT_DIR$file
    # Add comment line at top of file: "// This file was processed by the sedConvert.sh script\n/"
    # The presence of this comment line at top of file will inform the grep command above
    # to return a zero
    sed -i '1s/^/\/\/ This file was processed by the sedConvert.sh script\n/' $OUTPUT_DIR$file
  else
    # grep returned 0, meaning it found the HEADER_LINE string in the file.
    echo "The $OUTPUT_DIR$file source file was already processed by sedConvert.sh"
  fi
done 

