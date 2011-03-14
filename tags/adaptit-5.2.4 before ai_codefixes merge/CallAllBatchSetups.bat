@echo off
Break ON
echo This batch file calls all batch files for the Setup Generator folders 
echo in adaptit:
echo      1. _CopyDocs2InstallFolders.bat in the docs folder
echo      2. _CopyHelp2InstallFolders.bat in the hlp folder
echo      3. _CopyMO2InstallFolders.bat in the po folder
echo      4. _CopyXML2InstallFolders.bat in the xml folder
echo ===============================================================
echo Note: Calling the batch files populates the installation folders
echo with everything they need EXCEPT for the actual Regular and
echo Unicode executable to pack in the installation. The executables
echo are copied directly to the installation folders by Visual Studio
echo when Release and Unicode Release builds are done.
echo ===============================================================
echo Press CRTL-C to abort or
pause

@echo on
rem 1. Now calling _CopyDocs2InstallFolders.bat from the docs folder
@echo off
cd "docs"
call _CopyDocs2InstallFolders.bat

@echo on
rem 2. Now calling _CopyHelp2InstallFolders.bat from the hlp folder
@echo off
cd ..
cd "hlp"
call _CopyHelp2InstallFolders.bat

@echo on
rem 3. Now calling _CopyMO2InstallFolders.bat from the po folder
@echo off
cd ..
cd "po"
call _CopyMO2InstallFolders.bat

@echo on
rem 4. Now calling _CopyXML2InstallFolders.bat from the xml folder
@echo off
cd ..
cd "xml"
call _CopyXML2InstallFolders.bat
cd ..
echo ===================================================
echo Batch processes completed.
pause