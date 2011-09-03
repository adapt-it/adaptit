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
echo ===================================================
echo Press CRTL-C to abort or
pause

@echo on
rem 1. The following copies Documentation files to the "setup Regular" folder
@echo off
mkdir "..\setup Regular\"
copy "AI_USFM.xml" "..\setup Regular\"
copy "books.xml" "..\setup Regular\"

@echo on
rem 2. The following copies Documentation files to the "setup Regular - Minimal" folder
@echo off
mkdir "..\setup Regular - Minimal\"
copy "AI_USFM.xml" "..\setup Regular - Minimal\"
copy "books.xml" "..\setup Regular - Minimal\"

@echo on
rem 3. The following copies Documentation files to the "setup Regular - No Html Help" folder
@echo off
mkdir "..\setup Regular - No Html Help\"
copy "AI_USFM.xml" "..\setup Regular - No Html Help\"
copy "books.xml" "..\setup Regular - No Html Help\"

@echo on
rem 4. The following copies Documentation files to the "setup Unicode" folder
@echo off
mkdir "..\setup Unicode\"
copy "AI_USFM.xml" "..\setup Unicode\"
copy "books.xml" "..\setup Unicode\"

@echo on
rem 5. The following copies Documentation files to the "setup Unicode - Minimal" folder
@echo off
mkdir "..\setup Unicode - Minimal\"
copy "AI_USFM.xml" "..\setup Unicode - Minimal\"
copy "books.xml" "..\setup Unicode - Minimal\"

@echo on
rem 6. The following copies Documentation files to the "setup Unicode - No Html Help" folder
@echo off
mkdir "..\setup Unicode - No Html Help\"
copy "AI_USFM.xml" "..\setup Unicode - No Html Help\"
copy "books.xml" "..\setup Unicode - No Html Help\"

echo ===================================================
echo Copy process completed.
pause