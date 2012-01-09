@echo off
Break ON
echo This batch file calls cloc-1.08.exe to count the various kinds of source code
echo lines in the source directory of the Adapt It project.
cloc-1.08.exe source
pause
