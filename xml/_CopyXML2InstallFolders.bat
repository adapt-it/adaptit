@echo off
Break ON
echo This batch file copies the XML files to each of
echo the following installation folders for use be the Inno Setup 
echo script installers:
echo     1. Setup Unicode
echo     2. Setup Unicode - Minimal
echo     3. setup Unicode - No Html Help
echo ===================================================
echo AI_USFM.xml is copied to the above installation folders
echo books.xml is copied to the above installation folders
echo AI_UserProfiles.xml is copied to the above installation folders
echo aiDefault.css is copied to the above installation folders
rem whm 4 April 2017 removed the installation of the rdwrtp7i.exe and related
rem PT dll files from this batch file and from the Inno Setup installers.
rem echo rdwrtp7.exe is copied to the above installation folders
rem echo along with the following 5 Paratext dll files:
rem echo    ParatextShared.dll
rem echo    NetLoc.dll 
rem echo    Interop.XceedZipLib.dll
rem echo    ICSharpCode.SharpZipLib.dll
rem echo    Utilities.dll
echo ===================================================

@echo on
rem 1. The following copies Documentation files to the "setup Unicode" folder
@echo off
mkdir "..\setup Unicode\"
copy "AI_USFM.xml" "..\setup Unicode\"
copy "books.xml" "..\setup Unicode\"
copy "AI_UserProfiles.xml" "..\setup Unicode\"
copy "aiDefault.css" "..\setup Unicode\"
rem copy "rdwrtp7.exe" "..\setup Unicode\"
rem copy "ParatextShared.dll" "..\setup Unicode\"
rem copy "NetLoc.dll" "..\setup Unicode\"
rem copy "Interop.XceedZipLib.dll" "..\setup Unicode\"
rem copy "ICSharpCode.SharpZipLib.dll" "..\setup Unicode\"
rem copy "Utilities.dll" "..\setup Unicode\"

@echo on
rem 2. The following copies Documentation files to the "setup Unicode - Minimal" folder
@echo off
mkdir "..\setup Unicode - Minimal\"
copy "AI_USFM.xml" "..\setup Unicode - Minimal\"
copy "books.xml" "..\setup Unicode - Minimal\"
copy "AI_UserProfiles.xml" "..\setup Unicode - Minimal\"
copy "aiDefault.css" "..\setup Unicode - Minimal\"
rem copy "rdwrtp7.exe" "..\setup Unicode - Minimal\"
rem copy "ParatextShared.dll" "..\setup Unicode - Minimal\"
rem copy "NetLoc.dll" "..\setup Unicode - Minimal\"
rem copy "Interop.XceedZipLib.dll" "..\setup Unicode - Minimal\"
rem copy "ICSharpCode.SharpZipLib.dll" "..\setup Unicode - Minimal\"
rem copy "Utilities.dll" "..\setup Unicode - Minimal\"

@echo on
rem 3. The following copies Documentation files to the "setup Unicode - No Html Help" folder
@echo off
mkdir "..\setup Unicode - No Html Help\"
copy "AI_USFM.xml" "..\setup Unicode - No Html Help\"
copy "books.xml" "..\setup Unicode - No Html Help\"
copy "AI_UserProfiles.xml" "..\setup Unicode - No Html Help\"
copy "aiDefault.css" "..\setup Unicode - No Html Help\"
rem copy "rdwrtp7.exe" "..\setup Unicode - No Html Help\"
rem copy "ParatextShared.dll" "..\setup Unicode - No Html Help\"
rem copy "NetLoc.dll" "..\setup Unicode - No Html Help\"
rem copy "Interop.XceedZipLib.dll" "..\setup Unicode - No Html Help\"
rem copy "ICSharpCode.SharpZipLib.dll" "..\setup Unicode - No Html Help\"
rem copy "Utilities.dll" "..\setup Unicode - No Html Help\"

echo ===================================================
echo Copy process completed.
