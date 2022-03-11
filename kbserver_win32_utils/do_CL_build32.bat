@echo off
cls
set arg1=%1
del %1.obj
del %1.exe
set vLinkOpt="C:\Program Files (x86)\MariaDB 10.5\lib\libmariadb.lib"
cl /I "c:\Program Files (x86)\MariaDB 10.5\include\mysql\mysql.h" %1.c /MD %vLinkOpt%
::%1.exe
::dir *.dat
::dir *.txt