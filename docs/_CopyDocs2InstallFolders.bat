@echo off
Break ON
echo This batch file copies the Adapt It WX Documentation files
echo to the following adaptit installation folders for input
echo to the Windows installation packager Setup Generator:
echo    1. setup Regular
echo    2. setup Regular - Minimal
echo    3. setup Regular - No Html Help
echo    4. setup Regular Documentation Only
echo    5. setup Regular Localizations Only
echo    6. setup Unicode
echo    7. setup Unicode - Minimal
echo    8. setup Unicode - No Html Help
echo    9. setup Unicode Documentation Only
echo   10. setup Unicode Localizations Only
echo ===================================================
echo The following files are copied to all except the Minimal 
echo and Localizations Only folders:
echo    Adapt It Reference.doc
echo    Adapt_It_Quick_Start.htm
echo    Help_for_Administrators.htm
echo    Images\Adapt_It_Quick_Start\*.*
echo    Images\Admin_help\*.*
echo    Images\adaptit.ico
echo    Adapt It Tutorial.doc
echo    Adapt It changes.txt
echo    Known Issues and Limitations.txt
echo    Tok Pisin fragment 1John.txt
echo    KJV 1Jn 2.12-17.txt
echo    SILConverters in AdaptIt.doc
echo    Localization_Readme.txt
echo The following DLL file is copied to all except Documentation Only
echo and Localization Only:
echo    curl-ca-bundle.crt
echo    iso639-3codes.txt
echo.
echo To all except Minimal, Documentation Only and Localizations Only:
echo    CC\*.*
echo To all:
echo    ..\license\*.txt 
echo To all setup Regular:
echo    Readme.txt
echo To all setup Unicode:
echo    Readme_Unicode_Version.txt
echo ===================================================
echo Adapt It changes.txt is copied to ChangeLog in parent adaptit folder (11.)
echo adapt_it.ico is copied from res folder to Images folder of 
echo    setup Regular and setup Unicode (12.)
echo ===================================================
echo Press CRTL-C to abort or
pause

@echo on
rem 1. The following copies Documentation files to the "setup Regular" folder
@echo off
xcopy "Adapt It Reference.doc" "..\setup Regular\" /Y
xcopy "Adapt_It_Quick_Start.htm" "..\setup Regular\" /Y
xcopy "Help_for_Administrators.htm" "..\setup Regular\" /Y
mkdir "..\setup Regular\Images\Adapt_It_Quick_Start\"
xcopy "Images\Adapt_It_Quick_Start\*.*" "..\setup Regular\Images\Adapt_It_Quick_Start\*.*" /Y /Q /EXCLUDE:..\Exclude.txt
mkdir "..\setup Regular\Images\Admin_help\"
xcopy "Images\Admin_help\*.*" "..\setup Regular\Images\Admin_help\*.*" /Y /Q /EXCLUDE:..\Exclude.txt
mkdir "..\setup Regular\CC\"
xcopy "CC\*.*" "..\setup Regular\CC\*.*" /Y /Q /EXCLUDE:..\Exclude.txt
xcopy "Adapt It Tutorial.doc" "..\setup Regular\" /Y
xcopy "Adapt It changes.txt" "..\setup Regular\" /Y
xcopy "Known Issues and Limitations.txt" "..\setup Regular\" /Y
xcopy "Tok Pisin fragment 1John.txt" "..\setup Regular\" /Y
xcopy "KJV 1Jn 2.12-17.txt" "..\setup Regular\" /Y
xcopy "SILConverters in AdaptIt.doc" "..\setup Regular\" /Y
xcopy "Localization_Readme.txt" "..\setup Regular\" /Y
xcopy "..\license\*.txt" "..\setup Regular\*.*" /Y
xcopy "Readme.txt" "..\setup Regular\" /Y
xcopy "curl-ca-bundle.crt" "..\setup Regular\" /Y
xcopy "iso639-3codes.txt" "..\setup Regular\" /Y

@echo on
rem 2. The following copies Documentation files to the "setup Regular - Minimal" folder
@echo off
xcopy "..\license\*.txt" "..\setup Regular - Minimal\*.*" /Y
xcopy "Readme.txt" "..\setup Regular - Minimal\" /Y
xcopy "curl-ca-bundle.crt" "..\setup Regular - Minimal\" /Y
xcopy "iso639-3codes.txt" "..\setup Regular - Minimal\" /Y

@echo on
rem 3. The following copies Documentation files to the "setup Regular - No Html Help" folder
@echo off
xcopy "Adapt It Reference.doc" "..\setup Regular - No Html Help\" /Y
xcopy "Adapt_It_Quick_Start.htm" "..\setup Regular - No Html Help\" /Y
xcopy "Help_for_Administrators.htm" "..\setup Regular - No Html Help\" /Y
mkdir "..\setup Regular - No Html Help\Images\Adapt_It_Quick_Start\"
xcopy "Images\Adapt_It_Quick_Start\*.*" "..\setup Regular - No Html Help\Images\Adapt_It_Quick_Start\*.*" /Y /Q /EXCLUDE:..\Exclude.txt
mkdir "..\setup Regular - No Html Help\Images\Admin_help\"
xcopy "Images\Admin_help\*.*" "..\setup Regular - No Html Help\Images\Admin_help\*.*" /Y /Q /EXCLUDE:..\Exclude.txt
mkdir "..\setup Regular - No Html Help\CC\"
xcopy "CC\*.*" "..\setup Regular - No Html Help\CC\*.*" /Y /Q /EXCLUDE:..\Exclude.txt
xcopy "Adapt It Tutorial.doc" "..\setup Regular - No Html Help\" /Y
xcopy "Adapt It changes.txt" "..\setup Regular - No Html Help\" /Y
xcopy "Known Issues and Limitations.txt" "..\setup Regular - No Html Help\" /Y
xcopy "Tok Pisin fragment 1John.txt" "..\setup Regular - No Html Help\" /Y
xcopy "KJV 1Jn 2.12-17.txt" "..\setup Regular - No Html Help\" /Y
xcopy "SILConverters in AdaptIt.doc" "..\setup Regular - No Html Help\" /Y
xcopy "Localization_Readme.txt" "..\setup Regular - No Html Help\" /Y
xcopy "..\license\*.txt" "..\setup Regular - No Html Help\*.*" /Y
xcopy "Readme.txt" "..\setup Regular - No Html Help\" /Y
xcopy "curl-ca-bundle.crt" "..\setup Regular - No Html Help\" /Y
xcopy "iso639-3codes.txt" "..\setup Regular - No Html Help\" /Y

@echo on
rem 4. The following copies Documentation files to the "setup Regular Documentation Only" folder
@echo off
xcopy "Adapt It Reference.doc" "..\setup Regular Documentation Only\" /Y
xcopy "Adapt_It_Quick_Start.htm" "..\setup Regular Documentation Only\" /Y
xcopy "Help_for_Administrators.htm" "..\setup Regular Documentation Only\" /Y
mkdir "..\setup Regular Documentation Only\Images\Adapt_It_Quick_Start\"
xcopy "Images\Adapt_It_Quick_Start\*.*" "..\setup Regular Documentation Only\Images\Adapt_It_Quick_Start\*.*" /Y /Q /EXCLUDE:..\Exclude.txt
mkdir "..\setup Regular Documentation Only\Images\Admin_help\"
xcopy "Images\Admin_help\*.*" "..\setup Regular Documentation Only\Images\Admin_help\*.*" /Y /Q /EXCLUDE:..\Exclude.txt
xcopy "Adapt It Tutorial.doc" "..\setup Regular Documentation Only\" /Y
xcopy "Adapt It changes.txt" "..\setup Regular Documentation Only\" /Y
xcopy "Known Issues and Limitations.txt" "..\setup Regular Documentation Only\" /Y
xcopy "Tok Pisin fragment 1John.txt" "..\setup Regular Documentation Only\" /Y
xcopy "KJV 1Jn 2.12-17.txt" "..\setup Regular Documentation Only\" /Y
xcopy "SILConverters in AdaptIt.doc" "..\setup Regular Documentation Only\" /Y
xcopy "..\license\*.txt" "..\setup Regular Documentation Only\*.*" /Y
xcopy "Readme.txt" "..\setup Regular Documentation Only\" /Y
xcopy "Localization_Readme.txt" "..\setup Regular Documentation Only\" /Y

@echo on
rem 5. The following copies Documentation files to the "setup Regular Localizations Only" folder
@echo off
xcopy "Localization_Readme.txt" "..\setup Regular Localizations Only\" /Y
xcopy "..\license\*.txt" "..\setup Regular Localizations Only\*.*" /Y
xcopy "Readme.txt" "..\setup Regular Localizations Only\" /Y

@echo on
rem 6. The following copies Documentation files to the "setup Unicode" folder
@echo off
xcopy "Adapt It Reference.doc" "..\setup Unicode\" /Y
xcopy "Adapt_It_Quick_Start.htm" "..\setup Unicode\" /Y
xcopy "Help_for_Administrators.htm" "..\setup Unicode\" /Y
mkdir "..\setup Unicode\Images\Adapt_It_Quick_Start\"
xcopy "Images\Adapt_It_Quick_Start\*.*" "..\setup Unicode\Images\Adapt_It_Quick_Start\*.*" /Y /Q /EXCLUDE:..\Exclude.txt
mkdir "..\setup Unicode\Images\Admin_help\"
xcopy "Images\Admin_help\*.*" "..\setup Unicode\Images\Admin_help\*.*" /Y /Q /EXCLUDE:..\Exclude.txt
mkdir "..\setup Unicode\CC\"
xcopy "CC\*.*" "..\setup Unicode\CC\*.*" /Y /Q /EXCLUDE:..\Exclude.txt
xcopy "Adapt It Tutorial.doc" "..\setup Unicode\" /Y
xcopy "Adapt It changes.txt" "..\setup Unicode\" /Y
xcopy "Known Issues and Limitations.txt" "..\setup Unicode\" /Y
xcopy "Tok Pisin fragment 1John.txt" "..\setup Unicode\" /Y
xcopy "KJV 1Jn 2.12-17.txt" "..\setup Unicode\" /Y
xcopy "SILConverters in AdaptIt.doc" "..\setup Unicode\" /Y
xcopy "Localization_Readme.txt" "..\setup Unicode\" /Y
xcopy "..\license\*.txt" "..\setup Unicode\*.*" /Y
xcopy "Readme_Unicode_Version.txt" "..\setup Unicode\" /Y
xcopy "curl-ca-bundle.crt" "..\setup Unicode\" /Y
xcopy "iso639-3codes.txt" "..\setup Unicode\" /Y

@echo on
rem 7. The following copies Documentation files to the "setup Unicode - Minimal" folder
@echo off
xcopy "..\license\*.txt" "..\setup Unicode - Minimal\*.*" /Y
xcopy "Readme_Unicode_Version.txt" "..\setup Unicode - Minimal\" /Y
xcopy "curl-ca-bundle.crt" "..\setup Unicode - Minimal\" /Y
xcopy "iso639-3codes.txt" "..\setup Unicode - Minimal\" /Y

@echo on
rem 8. The following copies Documentation files to the "setup Unicode - No Html Help" folder
@echo off
xcopy "Adapt It Reference.doc" "..\setup Unicode - No Html Help\" /Y
xcopy "Adapt_It_Quick_Start.htm" "..\setup Unicode - No Html Help\" /Y
xcopy "Help_for_Administrators.htm" "..\setup Unicode - No Html Help\" /Y
mkdir "..\setup Unicode - No Html Help\Images\Adapt_It_Quick_Start\"
xcopy "Images\Adapt_It_Quick_Start\*.*" "..\setup Unicode - No Html Help\Images\Adapt_It_Quick_Start\*.*" /Y /Q /EXCLUDE:..\Exclude.txt
mkdir "..\setup Unicode - No Html Help\Images\Admin_help\"
xcopy "Images\Admin_help\*.*" "..\setup Unicode - No Html Help\Images\Admin_help\*.*" /Y /Q /EXCLUDE:..\Exclude.txt
mkdir "..\setup Unicode - No Html Help\CC\"
xcopy "CC\*.*" "..\setup Unicode - No Html Help\CC\*.*" /Y /Q /EXCLUDE:..\Exclude.txt
xcopy "Adapt It Tutorial.doc" "..\setup Unicode - No Html Help\" /Y
xcopy "Adapt It changes.txt" "..\setup Unicode - No Html Help\" /Y
xcopy "Known Issues and Limitations.txt" "..\setup Unicode - No Html Help\" /Y
xcopy "Tok Pisin fragment 1John.txt" "..\setup Unicode - No Html Help\" /Y
xcopy "KJV 1Jn 2.12-17.txt" "..\setup Unicode - No Html Help\" /Y
xcopy "SILConverters in AdaptIt.doc" "..\setup Unicode - No Html Help\" /Y
xcopy "Localization_Readme.txt" "..\setup Unicode - No Html Help\" /Y
xcopy "..\license\*.txt" "..\setup Unicode - No Html Help\*.*" /Y
xcopy "Readme_Unicode_Version.txt" "..\setup Unicode - No Html Help\" /Y
xcopy "curl-ca-bundle.crt" "..\setup Unicode - No Html Help\" /Y
xcopy "iso639-3codes.txt" "..\setup Unicode - No Html Help\" /Y

@echo on
rem 9. The following copies Documentation files to the "setup Unicode Documentation Only" folder
@echo off
xcopy "Adapt It Reference.doc" "..\setup Unicode Documentation Only\" /Y
xcopy "Adapt_It_Quick_Start.htm" "..\setup Unicode Documentation Only\" /Y
xcopy "Help_for_Administrators.htm" "..\setup Unicode Documentation Only\" /Y
mkdir "..\setup Unicode Documentation Only\Images\Adapt_It_Quick_Start\"
xcopy "Images\Adapt_It_Quick_Start\*.*" "..\setup Unicode Documentation Only\Images\Adapt_It_Quick_Start\*.*" /Y /Q /EXCLUDE:..\Exclude.txt
mkdir "..\setup Unicode Documentation Only\Images\Admin_help\"
xcopy "Images\Admin_help\*.*" "..\setup Unicode Documentation Only\Images\Admin_help\*.*" /Y /Q /EXCLUDE:..\Exclude.txt
xcopy "Adapt It Tutorial.doc" "..\setup Unicode Documentation Only\" /Y
xcopy "Adapt It changes.txt" "..\setup Unicode Documentation Only\" /Y
xcopy "Known Issues and Limitations.txt" "..\setup Unicode Documentation Only\" /Y
xcopy "Tok Pisin fragment 1John.txt" "..\setup Unicode Documentation Only\" /Y
xcopy "KJV 1Jn 2.12-17.txt" "..\setup Unicode Documentation Only\" /Y
xcopy "SILConverters in AdaptIt.doc" "..\setup Unicode Documentation Only\" /Y
xcopy "..\license\*.txt" "..\setup Unicode Documentation Only\*.*" /Y
xcopy "Readme_Unicode_Version.txt" "..\setup Unicode Documentation Only\" /Y
xcopy "Localization_Readme.txt" "..\setup Unicode Documentation Only\" /Y

@echo on
rem 10. The following copies Documentation files to the "setup Unicode Localizations Only" folder
@echo off
xcopy "Localization_Readme.txt" "..\setup Unicode Localizations Only\" /Y
xcopy "..\license\*.txt" "..\setup Unicode Localizations Only\*.*" /Y
xcopy "Readme_Unicode_Version.txt" "..\setup Unicode Localizations Only\" /Y

rem 11. Copy Adapt It changes.txt to ChangeLog file in parent adaptit folder (for Linux packaging)
copy "Adapt It changes.txt" "..\ChangeLog"

rem 12. Copy adapt_it.ico to the docs/Images folder of the "setup Regular" and "setup Unicode" folders
xcopy "..\res\ai_32.ico" "..\setup Regular\Images\adapt_it.ico" /Y
xcopy "..\res\ai_32.ico" "..\setup Unicode\Images\adapt_it.ico" /Y

echo ===================================================
echo Copy process completed.
pause