@echo off
rem
rem program name 	: dsb-win7 alias dsb-win10 alias dsb-win
rem programmer		: L.Pearl c/- Bruce Water ( SIL wycliffe )
rem date written	: 31/01/2018
rem	
del *.dat >nul 2>&1
start /B "" dns-sd -B _kbserver._tcp > service_file.dat
timeout /t 3 /nobreak >nul 2>&1
taskkill /F /FI "IMAGENAME eq dns-sd.exe" >nul 2>&1
for /F "tokens=7" %%i in ( service_file.dat ) do @echo %%i >> kbs_List.dat
type kbs_list.dat | findstr /v Type | findstr /v Browsing | findstr /v Timestamp > kb_list.dat
set zerofilesize=1
for /F "tokens=1" %%i in ( 'dir /a-d/-c KB_list.dat ^| findstr /e kb_list.dat' ) do set size=%%i
if %size% LSS %zero_file_size% ( call:do_more_processing ; call:close_off_discovery ) else ( call:end_of_process ) 
rem =================================================================================================================
:do_more_processing
for /F "tokens=*" %%a in ( kb_list.dat ) do (
  start /B "" dns-sd -L %%a _kbserver._tcp local >> got_hosts.dat
  timeout /t 2 /nobreak >nul 2>&1
  taskkill /F /FI "IMAGENAME eq dns-sd.exe" >nul 2>&1
)
type got_hosts.dat | findstr /v Lookup >> reached_host_lot.dat
for /F "tokens=7" %%i in ( reached_host_lot.dat ) do @echo %%i >> reached_hosts_names.dat
for /F "tokens=1 delims='.'" %%i in ( reached_hosts_names.dat ) do @echo %%i >> hostnames_to_use_for_IPs.dat
for /F "tokens=1" %%i in ( hostnames_to_use_for_IPs.dat ) do (
    start /B "" dns-sd -G v4 %%i.local >>IPaddress_list.dat
    timeout /t 2 nobreak >nul 2>&1
    taskkill /F /FI "IMAGENAME eq dns-sd.exe " >nul 2>&1
)
type IPaddress_list.dat | findstr /v Timestamp > IPaddress_list_tmp.dat
for /F "tokens=6,5" %%i in ( IPaddress_list_tmp.dat ) do echo|set /p="%%j@@@%%i," >> report.dat 
exit

:close_off_discovery
::
:end_of_process
call:close_off_discovery
:: ==============================================================


