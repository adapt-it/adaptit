@echo off
Break ON
echo This batch file copies the XML files to each of
echo the following installation folders for Setup Generator:
echo      1. Setup Regular
echo      2. setup Regular - Minimal
echo      3. setup Regular - No Html Help
echo      4. Setup Unicode
echo      5. Setup Unicode - Minimal
echo      6. setup Unicode - No Html Help
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
echo Press CRTL-C to abort or
pause

@echo on
rem 1. The following copies Documentation files to the "setup Regular" folder
@echo off
mkdir "..\setup Regular\"
copy "AI_USFM.xml" "..\setup Regular\"
copy "books.xml" "..\setup Regular\"
copy "AI_UserProfiles.xml" "..\setup Regular\"
copy "aiDefault.css" "..\setup Regular\"
copy "rdwrtp7.exe" "..\setup Regular\"
copy "ParatextShared.dll" "..\setup Regular\"
copy "NetLoc.dll" "..\setup Regular\"
copy "Interop.XceedZipLib.dll" "..\setup Regular\"
copy "ICSharpCode.SharpZipLib.dll" "..\setup Regular\"
copy "Utilities.dll" "..\setup Regular\"

@echo on
rem 2. The following copies Documentation files to the "setup Regular - Minimal" folder
@echo off
mkdir "..\setup Regular - Minimal\"
copy "AI_USFM.xml" "..\setup Regular - Minimal\"
copy "books.xml" "..\setup Regular - Minimal\"
copy "AI_UserProfiles.xml" "..\setup Regular - Minimal\"
copy "aiDefault.css" "..\setup Regular - Minimal\"
copy "rdwrtp7.exe" "..\setup Regular - Minimal\"
copy "ParatextShared.dll" "..\setup Regular - Minimal\"
copy "NetLoc.dll" "..\setup Regular - Minimal\"
copy "Interop.XceedZipLib.dll" "..\setup Regular - Minimal\"
copy "ICSharpCode.SharpZipLib.dll" "..\setup Regular - Minimal\"
copy "Utilities.dll" "..\setup Regular - Minimal\"

@echo on
rem 3. The following copies Documentation files to the "setup Regular - No Html Help" folder
@echo off
mkdir "..\setup Regular - No Html Help\"
copy "AI_USFM.xml" "..\setup Regular - No Html Help\"
copy "books.xml" "..\setup Regular - No Html Help\"
copy "AI_UserProfiles.xml" "..\setup Regular - No Html Help\"
copy "aiDefault.css" "..\setup Regular - No Html Help\"
copy "rdwrtp7.exe" "..\setup Regular - No Html Help\"
copy "ParatextShared.dll" "..\setup Regular - No Html Help\"
copy "NetLoc.dll" "..\setup Regular - No Html Help\"
copy "Interop.XceedZipLib.dll" "..\setup Regular - No Html Help\"
copy "ICSharpCode.SharpZipLib.dll" "..\setup Regular - No Html Help\"
copy "Utilities.dll" "..\setup Regular - No Html Help\"

@echo on
rem 4. The following copies Documentation files to the "setup Unicode" folder
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
rem 5. The following copies Documentation files to the "setup Unicode - Minimal" folder
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
rem 6. The following copies Documentation files to the "setup Unicode - No Html Help" folder
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
pause