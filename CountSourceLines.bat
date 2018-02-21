@echo off
Break ON
echo Usage: [path]CountSourceLines.bat [path]
echo This batch file calls cloc-1.08.exe to count the various kinds of
echo source code lines in the source directory of the Adapt It project.
echo Assumes cloc-1.08.exe is located in same folder as this batch file.
echo If this batch file is called from a different folder, enter the [path]
echo as a parameter (with final \) when calling this batch file.
set arg1=%1
echo parameter is: %arg1%
%arg1%cloc-1.08.exe %arg1%source
rem pause
