@echo off
Break ON
echo This batch file runs the Doxygen program using the Doxyfile configuration
echo file.
echo ===================================================
echo Press CRTL-C to abort or
pause


"C:\Program Files\doxygen\bin\doxywizard.exe" doxyfile

echo ===================================================
echo Doxygen process completed.
pause