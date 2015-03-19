@echo off
Break ON
echo This batch file copies the XML files to each of
echo the following installation folders for Setup Generator:
echo     1. Setup Unicode
echo     2. Setup Unicode - Minimal
echo     3. setup Unicode - No Html Help
echo ===================================================
echo AI_USFM.xml is copied to the above installation folders
echo books.xml is copied to the above installation folders
echo AI_UserProfiles.xml is copied to the above installation folders
echo aiDefault.css is copied to the above installation folders
echo rdwrtp7.exe is copied to the above installation folders
echo along with the following 5 Paratext dll files:
echo    ParatextShared.dll
echo    NetLoc.dll 
echo    Interop.XceedZipLib.dll
echo    ICSharpCode.SharpZipLib.dll
echo    Utilities.dll
echo ===================================================

@echo on
rem 1. The following copies Documentation files to the "setup Unicode" folder
@echo off
mkdir "..\setup Unicode\"
copy "AI_USFM.xml" "..\setup Unicode\"
copy "books.xml" "..\setup Unicode\"
copy "AI_UserProfiles.xml" "..\setup Unicode\"
copy "aiDefault.css" "..\setup Unicode\"
copy "rdwrtp7.exe" "..\setup Unicode\"
copy "ParatextShared.dll" "..\setup Unicode\"
copy "NetLoc.dll" "..\setup Unicode\"
copy "Interop.XceedZipLib.dll" "..\setup Unicode\"
copy "ICSharpCode.SharpZipLib.dll" "..\setup Unicode\"
copy "Utilities.dll" "..\setup Unicode\"

@echo on
rem 2. The following copies Documentation files to the "setup Unicode - Minimal" folder
@echo off
mkdir "..\setup Unicode - Minimal\"
copy "AI_USFM.xml" "..\setup Unicode - Minimal\"
copy "books.xml" "..\setup Unicode - Minimal\"
copy "AI_UserProfiles.xml" "..\setup Unicode - Minimal\"
copy "aiDefault.css" "..\setup Unicode - Minimal\"
copy "rdwrtp7.exe" "..\setup Unicode - Minimal\"
copy "ParatextShared.dll" "..\setup Unicode - Minimal\"
copy "NetLoc.dll" "..\setup Unicode - Minimal\"
copy "Interop.XceedZipLib.dll" "..\setup Unicode - Minimal\"
copy "ICSharpCode.SharpZipLib.dll" "..\setup Unicode - Minimal\"
copy "Utilities.dll" "..\setup Unicode - Minimal\"

@echo on
rem 3. The following copies Documentation files to the "setup Unicode - No Html Help" folder
@echo off
mkdir "..\setup Unicode - No Html Help\"
copy "AI_USFM.xml" "..\setup Unicode - No Html Help\"
copy "books.xml" "..\setup Unicode - No Html Help\"
copy "AI_UserProfiles.xml" "..\setup Unicode - No Html Help\"
copy "aiDefault.css" "..\setup Unicode - No Html Help\"
copy "rdwrtp7.exe" "..\setup Unicode - No Html Help\"
copy "ParatextShared.dll" "..\setup Unicode - No Html Help\"
copy "NetLoc.dll" "..\setup Unicode - No Html Help\"
copy "Interop.XceedZipLib.dll" "..\setup Unicode - No Html Help\"
copy "ICSharpCode.SharpZipLib.dll" "..\setup Unicode - No Html Help\"
copy "Utilities.dll" "..\setup Unicode - No Html Help\"

echo ===================================================
echo Copy process completed.
