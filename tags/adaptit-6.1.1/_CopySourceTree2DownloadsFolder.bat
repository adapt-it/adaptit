@echo off
Break ON
echo This batch file copies the Adapt It sources tree to a downloads folder 
echo of the developer's local repository. It creates a "clean" adaptit folder 
echo which contains all the files needed for creating Windows installers. 
echo Since this is a Windows batch file, it creates a Windows zip archive 
echo by the name Adapt_It_Src-x.x.x.zip in which the file can be renamed to
echo use the actual version number in the x.x.x part of the name. It omits 
echo files that do not need to be distributed as part of Adapt It source 
echo packages. It includes some exe and dll files that are needed for 
echo collaboration with Paratext.
echo ===================================================
echo Press CRTL-C to abort or
pause

@echo on
rem Remove any existing "downloads" folder
@echo off
IF EXIST downloads (rmdir downloads\ /S /Q)
@echo on

rem Copy adaptit files to the "downloads\adaptit\" folder
@echo off
mkdir "downloads\adaptit\"
rem Do not use \E to copy sub-directories for the top-level adaptit folder's files
xcopy "*.*" "downloads\adaptit\*.*" /Y /Q /EXCLUDE:Exclude.txt

@echo on
rem Copy bin files and linux mac and win32 subfolders to the "downloads\adaptit\bin\" folder
@echo off
mkdir "downloads\adaptit\bin\Adapt It WX Regular Setup\"
mkdir "downloads\adaptit\bin\Adapt It WX Unicode Setup\"
mkdir "downloads\adaptit\bin\linux\" 
mkdir "downloads\adaptit\bin\mac\" 
mkdir "downloads\adaptit\bin\win32\" 
xcopy "bin\Adapt It WX Regular Setup\Adapt It WX Regular Setup.vdproj" "downloads\adaptit\bin\Adapt It WX Regular Setup\" /Y
xcopy "bin\Adapt It WX Unicode Setup\Adapt It WX Unicode Setup.vdproj" "downloads\adaptit\bin\Adapt It WX Unicode Setup\" /Y
xcopy "bin\linux" "downloads\adaptit\bin\linux" /Y /E /Q /EXCLUDE:Exclude.txt
xcopy "bin\mac" "downloads\adaptit\bin\mac" /Y /E /Q /EXCLUDE:Exclude.txt
xcopy "bin\win32\Adapt_It.cbp" "downloads\adaptit\bin\win32\" /Y
xcopy "bin\win32\Adapt_It.rc" "downloads\adaptit\bin\win32\" /Y
xcopy "bin\win32\Adapt_It.sln" "downloads\adaptit\bin\win32\" /Y
xcopy "bin\win32\Adapt_It.vcproj" "downloads\adaptit\bin\win32\" /Y
xcopy "bin\win32\Adapt_It_vs05.sln" "downloads\adaptit\bin\win32\" /Y
xcopy "bin\win32\Adapt_It_vs05.vcproj" "downloads\adaptit\bin\win32\" /Y
xcopy "bin\win32\Equivalents_for_reference.h" "downloads\adaptit\bin\win32\" /Y

rem The debian folder should not be included in the distributable source tree

rem The design-docs folder should not be included in the distributable source tree

@echo on
rem Copy docs files to the "downloads\adaptit\docs\" folder
@echo off
mkdir "downloads\adaptit\docs\"
xcopy "docs" "downloads\adaptit\docs" /Y /E /Q /EXCLUDE:Exclude.txt
copy "docs\Adapt It changes.txt" "downloads\adaptit\ChangeLog"
DOS2UNIX.EXE "downloads\adaptit\ChangeLog"

@echo on
rem Copy hlp files to the "downloads\adaptit\hlp\" folder
@echo off
mkdir "downloads\adaptit\hlp\"
xcopy "hlp" "downloads\adaptit\hlp\" /Y /E /Q /EXCLUDE:Exclude.txt

rem The Inno Setup Scripts folder should not be included in the distributable source tree

@echo on
rem Copy license files to the "downloads\adaptit\license\" folder
@echo off
mkdir "downloads\adaptit\license\"
xcopy "license\*.txt" "downloads\adaptit\license\*.*" /Y

rem The packaging and distribution docs folder should not be included in the distributable source tree

rem The php folder should not be included in the distributable source tree

@echo on
rem Copy platform setup docs files to the "downloads\adaptit\platform setup docs\" folder
@echo off
mkdir "downloads\adaptit\platform setup docs\"
xcopy "platform setup docs" "downloads\adaptit\platform setup docs\" /Y /E /Q /EXCLUDE:Exclude.txt

@echo on
rem Copy po files to the "downloads\adaptit\po\" folder
@echo off
rem The source tree needs to contain both mo and po files for Windows plus others specific to Linux and the Mac
mkdir "downloads\adaptit\po\"
xcopy "po" "downloads\adaptit\po\" /Y /E /Q /EXCLUDE:Exclude.txt

rem The rdwrtp7 folder should not be included in the distributable source tree (rdwrtp7.exe is in the xml folder)

@echo on
rem Copy res files to the "downloads\adaptit\res\" folder
@echo off
rem There are many other image files in res that are not required for distribution of the app
mkdir "downloads\adaptit\res\32x32\"
mkdir "downloads\adaptit\res\48x48\"
rem Use regular copy for changing name during copy of ai_32.ico to adapt-it.ico below
copy "res\adapt_it.ico" "downloads\adaptit\res\" /Y
copy "res\ai_32.ico" "downloads\adaptit\res\" /Y
xcopy "res\32x32\adaptit.png" "downloads\adaptit\res\32x32\" /Y
xcopy "res\48x48\adaptit.png" "downloads\adaptit\res\48x48\" /Y

@echo on
rem Copy source files to the "downloads\adaptit\source\" folder
@echo off
mkdir "downloads\adaptit\source\"
xcopy "source" "downloads\adaptit\source\" /Y /E /Q /EXCLUDE:Exclude.txt

@echo on
rem Copy testdocs files to the "downloads\adaptit\testdocs\" folder
@echo off
mkdir "downloads\adaptit\testdocs\"
xcopy "testdocs" "downloads\adaptit\testdocs\" /Y /E /Q /EXCLUDE:Exclude.txt

@echo on
rem Copy xml files to the "downloads\adaptit\xml\" folder
@echo off
rem The xml folder also contains rdwrtp7.exe and 5 Paratext dll files that need to go with Windows
mkdir "downloads\adaptit\xml\"
xcopy "xml" "downloads\adaptit\xml\" /Y /E /Q /EXCLUDE:Exclude.txt

@echo on
rem Creating the Adapt_It_Src.zip archive for Windows in the "downloads" folder
@echo off
cd downloads
rem The next line invokes the command line version of 7-Zip from its
rem Windows installation (where the full version of 7-Zip gets installed).
rem The syntax is:
rem   a             add files to the archive
rem   -tzip         means to create a standard zip archive
rem   -x!*.zip      (if used) means to exclude any *.zip file from going into the archive
rem   Adapt_It_Src-x.x.x.zip  is the name of the archive to create in the hlp dir
rem   *             a single asterisk tells 7za to include all dirs and files
"C:\Program Files\7-Zip\7za" a -tzip Adapt_It_Src-x.x.x.zip adaptit\

echo ===================================================
echo Copy process completed.
pause