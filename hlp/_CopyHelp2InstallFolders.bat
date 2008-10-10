@echo off
Break ON
echo This batch file does the following:
echo 1. Deletes any existing hlp_temp folder and all of its 
echo    contents,
echo 2. Copies the hlp folder to the temporary hlp_temp folder,
echo    excluding the .svn folders and other other files that
echo    should not be included in the help (.chm .htb .bat and
echo    the Exlcude.txt file itself),
echo 3. Calls 7Zip to zip up the contents of the hlp_temp folder,
echo 4. Renames the zipped file to Adapt_It.htb,
echo 5. Copies the Adapt_It.htb back to the hlp folder
echo 6. Copies the zipped Adapt_It.htb Help file from the hlp 
echo folder to each of the following installation folders for 
echo processing by Setup Generator into Windows installers:
echo      Setup Regular
echo      Setup Regular Documentation Only
echo      Setup Unicode
echo      Setup Unicode Documentation Only
echo ===================================================
echo Adapt_It_Help.chm is copied to the setup Regular folder
echo Adapt_It_Help.chm is copied to the setup Regular Documentation Only folder
echo Adapt_It_Help.chm is copied to the setup Unicode folder
echo Adapt_It_Help.chm is copied to the setup Unicode Documentation Only folder
echo ===================================================
echo Press CRTL-C to abort or
rem The following copies the entire hlp folder to a sister hlp_temp folder
rem excluding the .svn folders and contents. Then it zips the contents of
rem the hlp_temp folder and renames the zip file to Adapt_It.htb and copies
rem that Adapt_It.htb file back to the current hlp folder. This process
rem eliminates the 10MB or so of .svn contents from artifically inflating
rem the size of the final .htb help file.
pause
rmdir "..\hlp_temp\" /S /Q
xcopy *.* "..\hlp_temp\*.*" /r /k /y /e /c /EXCLUDE:Exclude.txt
"C:\Program Files\7-Zip\7za" a -tzip Adapt_It.zip "..\hlp_temp\"
del *.htb /Q
ren Adapt_It.zip Adapt_It.htb

echo Now we will copy the Adapt_It.htb files to installation
echo directories for Setup Generator.
pause
copy Adapt_It.htb "..\setup Regular\Adapt_It.htb" /Y
copy Adapt_It.htb "..\setup Regular Documentation Only\Adapt_It.htb" /Y
copy Adapt_It.htb "..\setup Unicode\Adapt_It_Unicode.htb" /Y
copy Adapt_It.htb "..\setup Unicode Documentation Only\Adapt_It_Unicode.htb" /Y
echo ===================================================
echo Copy process completed.
pause