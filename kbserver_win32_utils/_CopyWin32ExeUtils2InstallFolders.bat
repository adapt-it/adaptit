@echo off
Break ON
echo This batch file copies the kbserver_win32_utils folder's .exe 
echo files to each of the following installation folders for 
echo incorporation into these Inno Setup script installers:
echo     1. Setup Unicode
echo     2. Setup Unicode - Minimal
echo     3. setup Unicode - No Html Help
echo ===================================================
echo The names of the Windows exe files are:
echo do_mdns.exe
echo mdns.exe
echo do_mdns_report.exe
echo ===================================================

@echo on
rem 1. The following copies the win32_utils exe files to the "setup Unicode" folder
@echo off
mkdir "..\setup Unicode\" "..\setup Unicode\"
copy "do_mdns.exe" "..\setup Unicode\"
copy "mdns.exe" "..\setup Unicode\"
copy "do_mdns_report.exe" "..\setup Unicode\"

@echo on
rem 2. The following copies win32_utils exe  files to the "setup Unicode - Minimal" folder
@echo off
mkdir "..\setup Unicode - Minimal\"
copy "do_mdns.exe" "..\setup Unicode - Minimal\"
copy "mdns.exe" "..\setup Unicode - Minimal\"
copy "do_mdns_report.exe" "..\setup Unicode - Minimal\"

@echo on
rem 3. The following copies win32_utils exe  files to the "setup Unicode - No Html Help" folder
@echo off
mkdir "..\setup Unicode - No Html Help\"
copy "do_mdns.exe" "..\setup Unicode - No Html Help\"
copy "mdns.exe" "..\setup Unicode - No Html Help\"
copy "do_mdns_report.exe" "..\setup Unicode - No Html Help\"

echo ===================================================
echo Copy process completed.
