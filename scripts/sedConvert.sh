#!/bin/bash
# sedConvert.sh -- tweak wxDesigner C++ code to work with wxWidgets-3.1.x
# Author: Graeme Costin <adaptit@costincomputingservices.com.au>
# Date: 2019-01-28
# Revised by Bill Martin <bill_martin@sil.org> to incorporate into adaptit repo
#
# Add any new source files to process to the $FILE_NAMES_TO_PROCESS string list 
# below leaving a space between file names.
FILE_NAMES_TO_PROCESS="Adapt_It_wdr.cpp ExportSaveAsDlg.cpp PeekAtFile.cpp"
HEADER_LINE="This file was processed by the sedConvert.sh script"
SOURCE_DIR="../source/"

for file in $FILE_NAMES_TO_PROCESS
do
  grep -Fq "$HEADER_LINE" $SOURCE_DIR$file
  GREPRESULTINT=$?
  if [ $GREPRESULTINT -ne 0 ]; then
    # grep returned non-0 result indicating the HEADER_LINE was not found in the file. 
    # so the file was not previously processed with sedConvert.sh, but was recently 
    # exported as C++ from wxDesigner, and so we need to process it with sed to tweak 
    # it for use with wxWidgets-3.1.x.
    # The next 2 lines were for debugging:
    #line=$(head -n 1 $SOURCE_DIR$file)
    #echo "Debug: line is [$line]"
    echo "Processing wxDesigner's C++ code in $SOURCE_DIR$file for wxWidgets-3.1.x builds"
    # Use sed to process text substitution commands from sedCommandFile.txt in-place
    # sed options:
    # -i sed command does "in-place" editing
    # -f sed uses the patterns in sedCommandFile.txt
    sed -i -f sedCommandFile.txt $SOURCE_DIR$file
    # Add comment line at top of file: "// This file was processed by the sedConvert.sh script\n/"
    # The presence of this comment line at top of file will inform the grep command above
    # to return a zero
    sed -i '1s/^/\/\/ This file was processed by the sedConvert.sh script\n/' $SOURCE_DIR$file
  else
    # grep returned 0, meaning it found the HEADER_LINE string in the file.
    echo "The $SOURCE_DIR$file source file was already processed by sedConvert.sh"
  fi
done 

