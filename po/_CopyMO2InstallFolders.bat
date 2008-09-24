@echo off
Break ON
rem Be sure to include any new/additional localizations as they become available.
echo This batch file copies the mo localization files to each of
echo the following installation folders for Setup Generator and VS build folders :
echo      1. Setup Regular
echo      2. Setup Regular - No Html Help
echo      3. Setup Regular - Localizations Only
echo      4. Setup Unicode
echo      5. Setup Unicode - No Html Help
echo      6. Setup Unicode - Localizations Only
echo      - - - - - - - - - - - - - - - - - - - - - - - -
echo      7. Visual Studio build folder "bin\win32\Debug"
echo      8. Visual Studio build folder "bin\win32\Release"
echo      9. Visual Studio build folder "bin\win32\Unicode Debug"
echo     10. Visual Studio build folder "bin\win32\Unicode Release"
echo The mo files are renamed to Adapt_It.mo or Adapt_It_Unicode.mo and placed 
echo    in their proper localization folders as follows:
echo ===================================================
echo es.mo is copied as Adapt_It.mo to the Languages\es folder of each installation
echo fr.mo is copied as Adapt_It.mo to the Languages\fr folder of each installation
echo id.mo is copied as Adapt_It.mo to the Languages\id folder of each installation
echo pt.mo is copied as Adapt_It.mo to the Languages\pt folder of each installation
echo ru.mo is copied as Adapt_It.mo to the Languages\ru folder of each installation
echo tpi.mo is copied as Adapt_It.mo to the Languages\tpi folder of each installation
echo zh.mo is copied as Adapt_It.mo to the Languages\zh folder of each installation
echo defauls.mo is copied under the same name to the Languages folder of each installation
echo ===================================================
echo Press CRTL-C to abort or
pause

@echo on
rem 1. The following copies localization files to the "setup Regular" folder
@echo off
mkdir "..\setup Regular\Languages\es"
mkdir "..\setup Regular\Languages\fr"
mkdir "..\setup Regular\Languages\id"
mkdir "..\setup Regular\Languages\pt"
mkdir "..\setup Regular\Languages\ru"
mkdir "..\setup Regular\Languages\tpi"
mkdir "..\setup Regular\Languages\zh"
copy es.mo "..\setup Regular\Languages\es\Adapt_It.mo"
copy es.po "..\setup Regular\Languages\es\es.po"
copy wxstd-es.mo "..\setup Regular\Languages\es\wxstd.mo"
copy fr.mo "..\setup Regular\Languages\fr\Adapt_It.mo"
copy fr.po "..\setup Regular\Languages\fr\fr.po"
copy wxstd-fr.mo "..\setup Regular\Languages\fr\wxstd.mo"
copy id.mo "..\setup Regular\Languages\id\Adapt_It.mo"
copy id.po "..\setup Regular\Languages\id\id.po"
rem Indonesian does not have a wxWidgets wxstd.mo localization file yet
copy pt.mo "..\setup Regular\Languages\pt\Adapt_It.mo"
copy pt.po "..\setup Regular\Languages\pt\pt.po"
copy wxstd-pt.mo "..\setup Regular\Languages\pt\wxstd.mo"
copy ru.mo "..\setup Regular\Languages\ru\Adapt_It.mo"
copy ru.po "..\setup Regular\Languages\ru\ru.po"
copy wxstd-ru.mo "..\setup Regular\Languages\ru\wxstd.mo"
copy tpi.mo "..\setup Regular\Languages\tpi\Adapt_It.mo"
copy tpi.po "..\setup Regular\Languages\tpi\tpi.po"
copy tpi_readme.txt "..\setup Regular\Languages\tpi\tpi_readme.txt"
copy books_tpi.xml "..\setup Regular\Languages\tpi\books_tpi.xml"
copy wxstd-tpi.mo "..\setup Regular\Languages\tpi\wxstd.mo"
copy zh.mo "..\setup Regular\Languages\zh\Adapt_It.mo"
copy zh.po "..\setup Regular\Languages\zh\zh.po"
copy wxstd-zh.mo "..\setup Regular\Languages\zh\wxstd.mo"
copy default.mo "..\setup Regular\Languages\default.mo"
copy default.po "..\setup Regular\Languages\default.po"

@echo on
rem 2. The following copies localization files to the "setup Regular - No Html Help" folder
@echo off
mkdir "..\setup Regular - No Html Help\Languages\es"
mkdir "..\setup Regular - No Html Help\Languages\fr"
mkdir "..\setup Regular - No Html Help\Languages\id"
mkdir "..\setup Regular - No Html Help\Languages\pt"
mkdir "..\setup Regular - No Html Help\Languages\ru"
mkdir "..\setup Regular - No Html Help\Languages\tpi"
mkdir "..\setup Regular - No Html Help\Languages\zh"
copy es.mo "..\setup Regular - No Html Help\Languages\es\Adapt_It.mo"
copy es.po "..\setup Regular - No Html Help\Languages\es\es.po"
copy wxstd-es.mo "..\setup Regular - No Html Help\Languages\es\wxstd.mo"
copy fr.mo "..\setup Regular - No Html Help\Languages\fr\Adapt_It.mo"
copy fr.po "..\setup Regular - No Html Help\Languages\fr\fr.po"
copy wxstd-fr.mo "..\setup Regular - No Html Help\Languages\fr\wxstd.mo"
copy id.mo "..\setup Regular - No Html Help\Languages\id\Adapt_It.mo"
copy id.po "..\setup Regular - No Html Help\Languages\id\id.po"
rem Indonesian does not have a wxWidgets wxstd.mo localization file yet
copy pt.mo "..\setup Regular - No Html Help\Languages\pt\Adapt_It.mo"
copy pt.po "..\setup Regular - No Html Help\Languages\pt\pt.po"
copy wxstd-pt.mo "..\setup Regular - No Html Help\Languages\pt\wxstd.mo"
copy ru.mo "..\setup Regular - No Html Help\Languages\ru\Adapt_It.mo"
copy ru.po "..\setup Regular - No Html Help\Languages\ru\ru.po"
copy wxstd-ru.mo "..\setup Regular - No Html Help\Languages\ru\wxstd.mo"
copy tpi.mo "..\setup Regular - No Html Help\Languages\tpi\Adapt_It.mo"
copy tpi.po "..\setup Regular - No Html Help\Languages\tpi\tpi.po"
copy tpi_readme.txt "..\setup Regular - No Html Help\Languages\tpi\tpi_readme.txt"
copy books_tpi.xml "..\setup Regular - No Html Help\Languages\tpi\books_tpi.xml"
copy wxstd-tpi.mo "..\setup Regular - No Html Help\Languages\tpi\wxstd.mo"
copy zh.mo "..\setup Regular - No Html Help\Languages\zh\Adapt_It.mo"
copy zh.po "..\setup Regular - No Html Help\Languages\zh\zh.po"
copy wxstd-zh.mo "..\setup Regular - No Html Help\Languages\zh\wxstd.mo"
copy default.mo "..\setup Regular - No Html Help\Languages\default.mo"
copy default.po "..\setup Regular - No Html Help\Languages\default.po"

@echo on
rem 3. The following copies localization files to the "setup Regular Localizations Only" folder
@echo off
mkdir "..\setup Regular Localizations Only\Languages\es"
mkdir "..\setup Regular Localizations Only\Languages\fr"
mkdir "..\setup Regular Localizations Only\Languages\id"
mkdir "..\setup Regular Localizations Only\Languages\pt"
mkdir "..\setup Regular Localizations Only\Languages\ru"
mkdir "..\setup Regular Localizations Only\Languages\tpi"
mkdir "..\setup Regular Localizations Only\Languages\zh"
copy es.mo "..\setup Regular Localizations Only\Languages\es\Adapt_It.mo"
copy es.po "..\setup Regular Localizations Only\Languages\es\es.po"
copy wxstd-es.mo "..\setup Regular Localizations Only\Languages\es\wxstd.mo"
copy fr.mo "..\setup Regular Localizations Only\Languages\fr\Adapt_It.mo"
copy fr.po "..\setup Regular Localizations Only\Languages\fr\fr.po"
copy wxstd-fr.mo "..\setup Regular Localizations Only\Languages\fr\wxstd.mo"
copy id.mo "..\setup Regular Localizations Only\Languages\id\Adapt_It.mo"
copy id.po "..\setup Regular Localizations Only\Languages\id\id.po"
rem Indonesian does not have a wxWidgets wxstd.mo localization file yet
copy pt.mo "..\setup Regular Localizations Only\Languages\pt\Adapt_It.mo"
copy pt.po "..\setup Regular Localizations Only\Languages\pt\pt.po"
copy wxstd-pt.mo "..\setup Regular Localizations Only\Languages\pt\wxstd.mo"
copy ru.mo "..\setup Regular Localizations Only\Languages\ru\Adapt_It.mo"
copy ru.po "..\setup Regular Localizations Only\Languages\ru\ru.po"
copy wxstd-ru.mo "..\setup Regular Localizations Only\Languages\ru\wxstd.mo"
copy tpi.mo "..\setup Regular Localizations Only\Languages\tpi\Adapt_It.mo"
copy tpi.po "..\setup Regular Localizations Only\Languages\tpi\tpi.po"
copy tpi_readme.txt "..\setup Regular Localizations Only\Languages\tpi\tpi_readme.txt"
copy books_tpi.xml "..\setup Regular Localizations Only\Languages\tpi\books_tpi.xml"
copy wxstd-tpi.mo "..\setup Regular Localizations Only\Languages\tpi\wxstd.mo"
copy zh.mo "..\setup Regular Localizations Only\Languages\zh\Adapt_It.mo"
copy zh.po "..\setup Regular Localizations Only\Languages\zh\zh.po"
copy wxstd-zh.mo "..\setup Regular Localizations Only\Languages\zh\wxstd.mo"
copy default.mo "..\setup Regular Localizations Only\Languages\default.mo"
copy default.po "..\setup Regular Localizations Only\Languages\default.po"

@echo on
rem 4. The following copies localization files to the "setup Unicode" folder
@echo off
mkdir "..\setup Unicode\Languages\es"
mkdir "..\setup Unicode\Languages\fr"
mkdir "..\setup Unicode\Languages\id"
mkdir "..\setup Unicode\Languages\pt"
mkdir "..\setup Unicode\Languages\ru"
mkdir "..\setup Unicode\Languages\tpi"
mkdir "..\setup Unicode\Languages\zh"
copy es.mo "..\setup Unicode\Languages\es\Adapt_It_Unicode.mo"
copy es.po "..\setup Unicode\Languages\es\es.po"
copy wxstd-es.mo "..\setup Unicode\Languages\es\wxstd.mo"
copy fr.mo "..\setup Unicode\Languages\fr\Adapt_It_Unicode.mo"
copy fr.po "..\setup Unicode\Languages\fr\fr.po"
copy wxstd-fr.mo "..\setup Unicode\Languages\fr\wxstd.mo"
copy id.mo "..\setup Unicode\Languages\id\Adapt_It_Unicode.mo"
copy id.po "..\setup Unicode\Languages\id\id.po"
rem Indonesian does not have a wxWidgets wxstd.mo localization file yet
copy pt.mo "..\setup Unicode\Languages\pt\Adapt_It_Unicode.mo"
copy pt.po "..\setup Unicode\Languages\pt\pt.po"
copy wxstd-pt.mo "..\setup Unicode\Languages\pt\wxstd.mo"
copy ru.mo "..\setup Unicode\Languages\ru\Adapt_It_Unicode.mo"
copy ru.po "..\setup Unicode\Languages\ru\ru.po"
copy wxstd-ru.mo "..\setup Unicode\Languages\ru\wxstd.mo"
copy tpi.mo "..\setup Unicode\Languages\tpi\Adapt_It_Unicode.mo"
copy tpi.po "..\setup Unicode\Languages\tpi\tpi.po"
copy tpi_readme.txt "..\setup Unicode\Languages\tpi\tpi_readme.txt"
copy books_tpi.xml "..\setup Unicode\Languages\tpi\books_tpi.xml"
copy wxstd-tpi.mo "..\setup Unicode\Languages\tpi\wxstd.mo"
copy zh.mo "..\setup Unicode\Languages\zh\Adapt_It_Unicode.mo"
copy zh.po "..\setup Unicode\Languages\zh\zh.po"
copy wxstd-zh.mo "..\setup Unicode\Languages\zh\wxstd.mo"
copy default.mo "..\setup Unicode\Languages\default.mo"
copy default.po "..\setup Unicode\Languages\default.po"

@echo on
rem 5. The following copies localization files to the "setup Unicode - No Html Help" folder
@echo off
mkdir "..\setup Unicode - No Html Help\Languages\es"
mkdir "..\setup Unicode - No Html Help\Languages\fr"
mkdir "..\setup Unicode - No Html Help\Languages\id"
mkdir "..\setup Unicode - No Html Help\Languages\pt"
mkdir "..\setup Unicode - No Html Help\Languages\ru"
mkdir "..\setup Unicode - No Html Help\Languages\tpi"
mkdir "..\setup Unicode - No Html Help\Languages\zh"
copy es.mo "..\setup Unicode - No Html Help\Languages\es\Adapt_It_Unicode.mo"
copy es.po "..\setup Unicode - No Html Help\Languages\es\es.po"
copy wxstd-es.mo "..\setup Unicode - No Html Help\Languages\es\wxstd.mo"
copy fr.mo "..\setup Unicode - No Html Help\Languages\fr\Adapt_It_Unicode.mo"
copy fr.po "..\setup Unicode - No Html Help\Languages\fr\fr.po"
copy wxstd-fr.mo "..\setup Unicode - No Html Help\Languages\fr\wxstd.mo"
copy id.mo "..\setup Unicode - No Html Help\Languages\id\Adapt_It_Unicode.mo"
copy id.po "..\setup Unicode - No Html Help\Languages\id\id.po"
rem Indonesian does not have a wxWidgets wxstd.mo localization file yet
copy pt.mo "..\setup Unicode - No Html Help\Languages\pt\Adapt_It_Unicode.mo"
copy pt.po "..\setup Unicode - No Html Help\Languages\pt\pt.po"
copy wxstd-pt.mo "..\setup Unicode - No Html Help\Languages\pt\wxstd.mo"
copy ru.mo "..\setup Unicode - No Html Help\Languages\ru\Adapt_It_Unicode.mo"
copy ru.po "..\setup Unicode - No Html Help\Languages\ru\ru.po"
copy wxstd-ru.mo "..\setup Unicode - No Html Help\Languages\ru\wxstd.mo"
copy tpi.mo "..\setup Unicode - No Html Help\Languages\tpi\Adapt_It_Unicode.mo"
copy tpi.po "..\setup Unicode - No Html Help\Languages\tpi\tpi.po"
copy tpi_readme.txt "..\setup Unicode - No Html Help\Languages\tpi\tpi_readme.txt"
copy books_tpi.xml "..\setup Unicode - No Html Help\Languages\tpi\books_tpi.xml"
copy wxstd-tpi.mo "..\setup Unicode - No Html Help\Languages\tpi\wxstd.mo"
copy zh.mo "..\setup Unicode - No Html Help\Languages\zh\Adapt_It_Unicode.mo"
copy zh.po "..\setup Unicode - No Html Help\Languages\zh\zh.po"
copy wxstd-zh.mo "..\setup Unicode - No Html Help\Languages\zh\wxstd.mo"
copy default.mo "..\setup Unicode - No Html Help\Languages\default.mo"
copy default.po "..\setup Unicode - No Html Help\Languages\default.po"

@echo on
rem 6. The following copies localization files to the "setup Unicode Localizations Only" folder
@echo off
mkdir "..\setup Unicode Localizations Only\Languages\es"
mkdir "..\setup Unicode Localizations Only\Languages\fr"
mkdir "..\setup Unicode Localizations Only\Languages\id"
mkdir "..\setup Unicode Localizations Only\Languages\pt"
mkdir "..\setup Unicode Localizations Only\Languages\ru"
mkdir "..\setup Unicode Localizations Only\Languages\tpi"
mkdir "..\setup Unicode Localizations Only\Languages\zh"
copy es.mo "..\setup Unicode Localizations Only\Languages\es\Adapt_It_Unicode.mo"
copy es.po "..\setup Unicode Localizations Only\Languages\es\es.po"
copy wxstd-es.mo "..\setup Unicode Localizations Only\Languages\es\wxstd.mo"
copy fr.mo "..\setup Unicode Localizations Only\Languages\fr\Adapt_It_Unicode.mo"
copy fr.po "..\setup Unicode Localizations Only\Languages\fr\fr.po"
copy wxstd-fr.mo "..\setup Unicode Localizations Only\Languages\fr\wxstd.mo"
copy id.mo "..\setup Unicode Localizations Only\Languages\id\Adapt_It_Unicode.mo"
copy id.po "..\setup Unicode Localizations Only\Languages\id\id.po"
rem Indonesian does not have a wxWidgets wxstd.mo localization file yet
copy pt.mo "..\setup Unicode Localizations Only\Languages\pt\Adapt_It_Unicode.mo"
copy pt.po "..\setup Unicode Localizations Only\Languages\pt\pt.po"
copy wxstd-pt.mo "..\setup Unicode Localizations Only\Languages\pt\wxstd.mo"
copy ru.mo "..\setup Unicode Localizations Only\Languages\ru\Adapt_It_Unicode.mo"
copy ru.po "..\setup Unicode Localizations Only\Languages\ru\ru.po"
copy wxstd-ru.mo "..\setup Unicode Localizations Only\Languages\ru\wxstd.mo"
copy tpi.mo "..\setup Unicode Localizations Only\Languages\tpi\Adapt_It_Unicode.mo"
copy tpi.po "..\setup Unicode Localizations Only\Languages\tpi\tpi.po"
copy tpi_readme.txt "..\setup Unicode Localizations Only\Languages\tpi\tpi_readme.txt"
copy books_tpi.xml "..\setup Unicode Localizations Only\Languages\tpi\books_tpi.xml"
copy wxstd-tpi.mo "..\setup Unicode Localizations Only\Languages\tpi\wxstd.mo"
copy zh.mo "..\setup Unicode Localizations Only\Languages\zh\Adapt_It_Unicode.mo"
copy zh.po "..\setup Unicode Localizations Only\Languages\zh\zh.po"
copy wxstd-zh.mo "..\setup Unicode Localizations Only\Languages\zh\wxstd.mo"
copy default.mo "..\setup Unicode Localizations Only\Languages\default.mo"
copy default.po "..\setup Unicode Localizations Only\Languages\default.po"

@echo on
rem 7. The following copies localization files to the "bin\win32\Debug" folder
@echo off
mkdir "..\bin\win32\Debug\Languages\es"
mkdir "..\bin\win32\Debug\Languages\fr"
mkdir "..\bin\win32\Debug\Languages\id"
mkdir "..\bin\win32\Debug\Languages\pt"
mkdir "..\bin\win32\Debug\Languages\ru"
mkdir "..\bin\win32\Debug\Languages\tpi"
mkdir "..\bin\win32\Debug\Languages\zh"
copy es.mo "..\bin\win32\Debug\Languages\es\Adapt_It.mo"
copy es.po "..\bin\win32\Debug\Languages\es\es.po"
copy wxstd-es.mo "..\bin\win32\Debug\Languages\es\wxstd.mo"
copy fr.mo "..\bin\win32\Debug\Languages\fr\Adapt_It.mo"
copy fr.po "..\bin\win32\Debug\Languages\fr\fr.po"
copy wxstd-fr.mo "..\bin\win32\Debug\Languages\fr\wxstd.mo"
copy id.mo "..\bin\win32\Debug\Languages\id\Adapt_It.mo"
copy id.po "..\bin\win32\Debug\Languages\id\id.po"
rem Indonesian does not have a wxWidgets wxstd.mo localization file yet
copy pt.mo "..\bin\win32\Debug\Languages\pt\Adapt_It.mo"
copy pt.po "..\bin\win32\Debug\Languages\pt\pt.po"
copy wxstd-pt.mo "..\bin\win32\Debug\Languages\pt\wxstd.mo"
copy ru.mo "..\bin\win32\Debug\Languages\ru\Adapt_It.mo"
copy ru.po "..\bin\win32\Debug\Languages\ru\ru.po"
copy wxstd-ru.mo "..\bin\win32\Debug\Languages\ru\wxstd.mo"
copy tpi.mo "..\bin\win32\Debug\Languages\tpi\Adapt_It.mo"
copy tpi.po "..\bin\win32\Debug\Languages\tpi\tpi.po"
copy tpi_readme.txt "..\bin\win32\Debug\Languages\tpi\tpi_readme.txt"
copy books_tpi.xml "..\bin\win32\Debug\Languages\tpi\books_tpi.xml"
copy wxstd-tpi.mo "..\bin\win32\Debug\Languages\tpi\wxstd.mo"
copy zh.mo "..\bin\win32\Debug\Languages\zh\Adapt_It.mo"
copy zh.po "..\bin\win32\Debug\Languages\zh\zh.po"
copy wxstd-zh.mo "..\bin\win32\Debug\Languages\zh\wxstd.mo"
copy default.mo "..\bin\win32\Debug\Languages\default.mo"
copy default.po "..\bin\win32\Debug\Languages\default.po"

@echo on
rem 8. The following copies localization files to the "bin\win32\Release" folder
@echo off
mkdir "..\bin\win32\Release\Languages\es"
mkdir "..\bin\win32\Release\Languages\fr"
mkdir "..\bin\win32\Release\Languages\id"
mkdir "..\bin\win32\Release\Languages\pt"
mkdir "..\bin\win32\Release\Languages\ru"
mkdir "..\bin\win32\Release\Languages\tpi"
mkdir "..\bin\win32\Release\Languages\zh"
copy es.mo "..\bin\win32\Release\Languages\es\Adapt_It.mo"
copy es.po "..\bin\win32\Release\Languages\es\es.po"
copy wxstd-es.mo "..\bin\win32\Release\Languages\es\wxstd.mo"
copy fr.mo "..\bin\win32\Release\Languages\fr\Adapt_It.mo"
copy fr.po "..\bin\win32\Release\Languages\fr\fr.po"
copy wxstd-fr.mo "..\bin\win32\Release\Languages\fr\wxstd.mo"
copy id.mo "..\bin\win32\Release\Languages\id\Adapt_It.mo"
copy id.po "..\bin\win32\Release\Languages\id\id.po"
rem Indonesian does not have a wxWidgets wxstd.mo localization file yet
copy pt.mo "..\bin\win32\Release\Languages\pt\Adapt_It.mo"
copy pt.po "..\bin\win32\Release\Languages\pt\pt.po"
copy wxstd-pt.mo "..\bin\win32\Release\Languages\pt\wxstd.mo"
copy ru.mo "..\bin\win32\Release\Languages\ru\Adapt_It.mo"
copy ru.po "..\bin\win32\Release\Languages\ru\ru.po"
copy wxstd-ru.mo "..\bin\win32\Release\Languages\ru\wxstd.mo"
copy tpi.mo "..\bin\win32\Release\Languages\tpi\Adapt_It.mo"
copy tpi.po "..\bin\win32\Release\Languages\tpi\tpi.po"
copy tpi_readme.txt "..\bin\win32\Release\Languages\tpi\tpi_readme.txt"
copy books_tpi.xml "..\bin\win32\Release\Languages\tpi\books_tpi.xml"
copy wxstd-tpi.mo "..\bin\win32\Release\Languages\tpi\wxstd.mo"
copy zh.mo "..\bin\win32\Release\Languages\zh\Adapt_It.mo"
copy zh.po "..\bin\win32\Release\Languages\zh\zh.po"
copy wxstd-zh.mo "..\bin\win32\Release\Languages\zh\wxstd.mo"
copy default.mo "..\bin\win32\Release\Languages\default.mo"
copy default.po "..\bin\win32\Release\Languages\default.po"

@echo on
rem 9. The following copies localization files to the "bin\win32\Unicode Debug" folder
@echo off
mkdir "..\bin\win32\Unicode Debug\Languages\es"
mkdir "..\bin\win32\Unicode Debug\Languages\fr"
mkdir "..\bin\win32\Unicode Debug\Languages\id"
mkdir "..\bin\win32\Unicode Debug\Languages\pt"
mkdir "..\bin\win32\Unicode Debug\Languages\ru"
mkdir "..\bin\win32\Unicode Debug\Languages\tpi"
mkdir "..\bin\win32\Unicode Debug\Languages\zh"
copy es.mo "..\bin\win32\Unicode Debug\Languages\es\Adapt_It_Unicode.mo"
copy es.po "..\bin\win32\Unicode Debug\Languages\es\es.po"
copy wxstd-es.mo "..\bin\win32\Unicode Debug\Languages\es\wxstd.mo"
copy fr.mo "..\bin\win32\Unicode Debug\Languages\fr\Adapt_It_Unicode.mo"
copy fr.po "..\bin\win32\Unicode Debug\Languages\fr\fr.po"
copy wxstd-fr.mo "..\bin\win32\Unicode Debug\Languages\fr\wxstd.mo"
copy id.mo "..\bin\win32\Unicode Debug\Languages\id\Adapt_It_Unicode.mo"
copy id.po "..\bin\win32\Unicode Debug\Languages\id\id.po"
rem Indonesian does not have a wxWidgets wxstd.mo localization file yet
copy pt.mo "..\bin\win32\Unicode Debug\Languages\pt\Adapt_It_Unicode.mo"
copy pt.po "..\bin\win32\Unicode Debug\Languages\pt\pt.po"
copy wxstd-pt.mo "..\bin\win32\Unicode Debug\Languages\pt\wxstd.mo"
copy ru.mo "..\bin\win32\Unicode Debug\Languages\ru\Adapt_It_Unicode.mo"
copy ru.po "..\bin\win32\Unicode Debug\Languages\ru\ru.po"
copy wxstd-ru.mo "..\bin\win32\Unicode Debug\Languages\ru\wxstd.mo"
copy tpi.mo "..\bin\win32\Unicode Debug\Languages\tpi\Adapt_It_Unicode.mo"
copy tpi.po "..\bin\win32\Unicode Debug\Languages\tpi\tpi.po"
copy tpi_readme.txt "..\bin\win32\Unicode Debug\Languages\tpi\tpi_readme.txt"
copy books_tpi.xml "..\bin\win32\Unicode Debug\Languages\tpi\books_tpi.xml"
copy wxstd-tpi.mo "..\bin\win32\Unicode Debug\Languages\tpi\wxstd.mo"
copy zh.mo "..\bin\win32\Unicode Debug\Languages\zh\Adapt_It_Unicode.mo"
copy zh.po "..\bin\win32\Unicode Debug\Languages\zh\zh.po"
copy wxstd-zh.mo "..\bin\win32\Unicode Debug\Languages\zh\wxstd.mo"
copy default.mo "..\bin\win32\Unicode Debug\Languages\default.mo"
copy default.po "..\bin\win32\Unicode Debug\Languages\default.po"

@echo on
rem 10. The following copies localization files to the "bin\win32\Unicode Release" folder
@echo off
mkdir "..\bin\win32\Unicode Release\Languages\es"
mkdir "..\bin\win32\Unicode Release\Languages\fr"
mkdir "..\bin\win32\Unicode Release\Languages\id"
mkdir "..\bin\win32\Unicode Release\Languages\pt"
mkdir "..\bin\win32\Unicode Release\Languages\ru"
mkdir "..\bin\win32\Unicode Release\Languages\tpi"
mkdir "..\bin\win32\Unicode Release\Languages\zh"
copy es.mo "..\bin\win32\Unicode Release\Languages\es\Adapt_It_Unicode.mo"
copy es.po "..\bin\win32\Unicode Release\Languages\es\es.po"
copy wxstd-es.mo "..\bin\win32\Unicode Release\Languages\es\wxstd.mo"
copy fr.mo "..\bin\win32\Unicode Release\Languages\fr\Adapt_It_Unicode.mo"
copy fr.po "..\bin\win32\Unicode Release\Languages\fr\fr.po"
copy wxstd-fr.mo "..\bin\win32\Unicode Release\Languages\fr\wxstd.mo"
copy id.mo "..\bin\win32\Unicode Release\Languages\id\Adapt_It_Unicode.mo"
copy id.po "..\bin\win32\Unicode Release\Languages\id\id.po"
rem Indonesian does not have a wxWidgets wxstd.mo localization file yet
copy pt.mo "..\bin\win32\Unicode Release\Languages\pt\Adapt_It_Unicode.mo"
copy pt.po "..\bin\win32\Unicode Release\Languages\pt\pt.po"
copy wxstd-pt.mo "..\bin\win32\Unicode Release\Languages\pt\wxstd.mo"
copy ru.mo "..\bin\win32\Unicode Release\Languages\ru\Adapt_It_Unicode.mo"
copy ru.po "..\bin\win32\Unicode Release\Languages\ru\ru.po"
copy wxstd-ru.mo "..\bin\win32\Unicode Release\Languages\ru\wxstd.mo"
copy tpi.mo "..\bin\win32\Unicode Release\Languages\tpi\Adapt_It_Unicode.mo"
copy tpi.po "..\bin\win32\Unicode Release\Languages\tpi\tpi.po"
copy tpi_readme.txt "..\bin\win32\Unicode Release\Languages\tpi\tpi_readme.txt"
copy books_tpi.xml "..\bin\win32\Unicode Release\Languages\tpi\books_tpi.xml"
copy wxstd-tpi.mo "..\bin\win32\Unicode Release\Languages\tpi\wxstd.mo"
copy zh.mo "..\bin\win32\Unicode Release\Languages\zh\Adapt_It_Unicode.mo"
copy zh.po "..\bin\win32\Unicode Release\Languages\zh\zh.po"
copy wxstd-zh.mo "..\bin\win32\Unicode Release\Languages\zh\wxstd.mo"
copy default.mo "..\bin\win32\Unicode Release\Languages\default.mo"
copy default.po "..\bin\win32\Unicode Release\Languages\default.po"

echo ===================================================
echo Copy process completed.
pause