@echo off
Break ON
echo This batch file does the following:
echo 1. Deletes any existing hlp_temp folder and all of its 
echo    contents,
echo 2. Copies the hlp folder to the temporary hlp_temp folder,
echo    excluding the .svn folders and other other files that
echo    should not be included in the help (.chm .htb .bat and
echo    the Exlcude.txt file itself),
echo 3. Calls the command-line version of 7Zip (7za.exe) to zip up the 
echo    contents of the hlp_temp folder into an Adapt_It.zip archive,
echo 4. Renames the zipped archive file to Adapt_It.htb,
echo 5. Copies the Adapt_It.htb back to the hlp folder
echo 6. If Bill's Setup Generator folders exist, it copies the zipped 
echo    Adapt_It.htb Help file from the hlp folder to each of the 
echo    following installation folders for processing by Setup Generator 
echo    into Windows installers:
echo      Setup Regular
echo      Setup Regular Documentation Only
echo      Setup Unicode
echo      Setup Unicode Documentation Only
echo ===================================================
echo Adapt_It.htb is copied as Adapt_It.htb to the setup Regular folder
echo Adapt_It.htb is copied as Adapt_It.htb to the setup Regular Documentation Only folder
echo Adapt_It.htb is copied as Adapt_It_Unicode.htb to the setup Unicode folder
echo Adapt_It.htb is copied as Adapt_It_Unicode.htb to the setup Unicode Documentation Only folder
echo ===================================================

rem The next line deletes any existing temporary help folder named hlp_temp
rem along with any sub-folders and files it contains.

IF EXIST Adapt_It.htb (del Adapt_It.htb)

rem The next line uses xcopy to create a temporary hlp_temp directory that
rem is at the same level as the current hlp directory and copy all folders
rem and files from the hlp directory to the hlp_temp directory, but 
rem the copy process excludes folders and files with extensions contained
rem in the Exclude.txt file. 
rem xcopy *.* "..\hlp_temp\*.*" /r /k /y /e /c /EXCLUDE:Exclude.txt

rem Change to the hlp_temp directory so 7-zip will not include the hlp_temp
rem folder in the archive.
rem cd ..\hlp_temp

rem The next line invokes the command line version of 7-Zip from its
rem Windows installation (where the full version of 7-Zip gets installed.
rem The syntax is:
rem   a             add fi les to the archive
rem   -tzip         means to create a standard zip archive
rem   -x!*.htb      means to exclude any *.htb file from going into the archive
rem   ..\hlp\Adapt_It.htb  is the name of the archive to create in the hlp dir
rem   *             a single asterisk tells 7za to include all dirs and files
"C:\Program Files\7-Zip\7za" a -tzip Adapt_It.htb * -xr!.svn -xr!*.bat -xr!*.chm -xr!*.zip -xr!mac_excl.lst -xr!_errorlog.txt -xr!Adapt_It.htb 

rem If we're doing this on a machine that doesn't have Bill's directories
rem for Setup Generator, just end now.
IF EXIST "..\setup Regular\LICENSING.txt" GOTO SETUPGEN

GOTO END
:SETUPGEN
echo Now we will copy the Adapt_It.htb files to installation
echo directories for Setup Generator.
copy Adapt_It.htb "..\setup Regular\Adapt_It.htb" /Y
copy Adapt_It.htb "..\setup Regular Documentation Only\Adapt_It.htb" /Y
copy Adapt_It.htb "..\setup Unicode\Adapt_It_Unicode.htb" /Y
copy Adapt_It.htb "..\setup Unicode Documentation Only\Adapt_It_Unicode.htb" /Y
echo ===================================================

:END
echo Process completed.
