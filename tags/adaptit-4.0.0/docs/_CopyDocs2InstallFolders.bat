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
echo    Adapt It Quick Start.htm 
echo    Images\Adapt It Quick Start\*.*
echo    Images\adaptit.ico
echo    Adapt It Tutorial.doc
echo    Adapt It changes.txt
echo    Known Issues and Limitations.txt
echo    Tok Pisin fragment 1John.txt
echo    KJV 1Jn 2.12-17.txt
echo    SILConverters in AdaptIt.doc
echo.
echo To all except Minimal, Documentation Only and Localizations Only:
echo    CC\*.*
echo To all except Minimal and Documentation Only:
echo    Localization_Readme.txt
echo To all except Localization Only:
echo    ..\license\*.txt 
echo To all setup Regular except Localization Only:
echo    Readme.txt
echo To all setup Unicode except Localization Only:
echo    Readme_Unicode_Version.txt
echo ===================================================
echo Press CRTL-C to abort or
pause

@echo on
rem 1. The following copies Documentation files to the "setup Regular" folder
@echo off
xcopy "Adapt It Reference.doc" "..\setup Regular\" /Y
xcopy "Adapt It Quick Start.htm" "..\setup Regular\" /Y
mkdir "..\setup Regular\Images\Adapt It Quick Start\"
xcopy "Images\Adapt It Quick Start\*.*" "..\setup Regular\Images\Adapt It Quick Start\*.*" /Y /Q /EXCLUDE:..\Exclude.txt
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

@echo on
rem 2. The following copies Documentation files to the "setup Regular - Minimal" folder
@echo off
xcopy "..\license\*.txt" "..\setup Regular - Minimal\*.*" /Y
xcopy "Readme.txt" "..\setup Regular - Minimal\" /Y

@echo on
rem 3. The following copies Documentation files to the "setup Regular - No Html Help" folder
@echo off
xcopy "Adapt It Reference.doc" "..\setup Regular - No Html Help\" /Y
xcopy "Adapt It Quick Start.htm" "..\setup Regular - No Html Help\" /Y
mkdir "..\setup Regular - No Html Help\Images\Adapt It Quick Start\"
xcopy "Images\Adapt It Quick Start\*.*" "..\setup Regular - No Html Help\Images\Adapt It Quick Start\*.*" /Y /Q /EXCLUDE:..\Exclude.txt
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

@echo on
rem 4. The following copies Documentation files to the "setup Regular Documentation Only" folder
@echo off
xcopy "Adapt It Reference.doc" "..\setup Regular Documentation Only\" /Y
xcopy "Adapt It Quick Start.htm" "..\setup Regular Documentation Only\" /Y
mkdir "..\setup Regular Documentation Only\Images\Adapt It Quick Start\"
xcopy "Images\Adapt It Quick Start\*.*" "..\setup Regular Documentation Only\Images\Adapt It Quick Start\*.*" /Y /Q /EXCLUDE:..\Exclude.txt
xcopy "Adapt It Tutorial.doc" "..\setup Regular Documentation Only\" /Y
xcopy "Adapt It changes.txt" "..\setup Regular Documentation Only\" /Y
xcopy "Known Issues and Limitations.txt" "..\setup Regular Documentation Only\" /Y
xcopy "Tok Pisin fragment 1John.txt" "..\setup Regular Documentation Only\" /Y
xcopy "KJV 1Jn 2.12-17.txt" "..\setup Regular Documentation Only\" /Y
xcopy "SILConverters in AdaptIt.doc" "..\setup Regular Documentation Only\" /Y
xcopy "..\license\*.txt" "..\setup Regular Documentation Only\*.*" /Y
xcopy "Readme.txt" "..\setup Regular Documentation Only\" /Y

@echo on
rem 5. The following copies Documentation files to the "setup Regular Localizations Only" folder
@echo off
xcopy "Localization_Readme.txt" "..\setup Regular Localizations Only\" /Y

@echo on
rem 6. The following copies Documentation files to the "setup Unicode" folder
@echo off
xcopy "Adapt It Reference.doc" "..\setup Unicode\" /Y
xcopy "Adapt It Quick Start.htm" "..\setup Unicode\" /Y
mkdir "..\setup Unicode\Images\Adapt It Quick Start\"
xcopy "Images\Adapt It Quick Start\*.*" "..\setup Unicode\Images\Adapt It Quick Start\*.*" /Y /Q /EXCLUDE:..\Exclude.txt
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

@echo on
rem 7. The following copies Documentation files to the "setup Unicode - Minimal" folder
@echo off
xcopy "..\license\*.txt" "..\setup Unicode - Minimal\*.*" /Y
xcopy "Readme_Unicode_Version.txt" "..\setup Unicode - Minimal\" /Y

@echo on
rem 8. The following copies Documentation files to the "setup Unicode - No Html Help" folder
@echo off
xcopy "Adapt It Reference.doc" "..\setup Unicode - No Html Help\" /Y
xcopy "Adapt It Quick Start.htm" "..\setup Unicode - No Html Help\" /Y
mkdir "..\setup Unicode - No Html Help\Images\Adapt It Quick Start\"
xcopy "Images\Adapt It Quick Start\*.*" "..\setup Unicode - No Html Help\Images\Adapt It Quick Start\*.*" /Y /Q /EXCLUDE:..\Exclude.txt
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

@echo on
rem 9. The following copies Documentation files to the "setup Unicode Documentation Only" folder
@echo off
xcopy "Adapt It Reference.doc" "..\setup Unicode Documentation Only\" /Y
xcopy "Adapt It Quick Start.htm" "..\setup Unicode Documentation Only\" /Y
mkdir "..\setup Unicode Documentation Only\Images\Adapt It Quick Start\"
xcopy "Images\Adapt It Quick Start\*.*" "..\setup Unicode Documentation Only\Images\Adapt It Quick Start\*.*" /Y /Q /EXCLUDE:..\Exclude.txt
xcopy "Adapt It Tutorial.doc" "..\setup Unicode Documentation Only\" /Y
xcopy "Adapt It changes.txt" "..\setup Unicode Documentation Only\" /Y
xcopy "Known Issues and Limitations.txt" "..\setup Unicode Documentation Only\" /Y
xcopy "Tok Pisin fragment 1John.txt" "..\setup Unicode Documentation Only\" /Y
xcopy "KJV 1Jn 2.12-17.txt" "..\setup Unicode Documentation Only\" /Y
xcopy "SILConverters in AdaptIt.doc" "..\setup Unicode Documentation Only\" /Y
xcopy "..\license\*.txt" "..\setup Unicode Documentation Only\*.*" /Y
xcopy "Readme_Unicode_Version.txt" "..\setup Unicode Documentation Only\" /Y

@echo on
rem 10. The following copies Documentation files to the "setup Unicode Localizations Only" folder
@echo off
xcopy "Localization_Readme.txt" "..\setup Unicode Localizations Only\" /Y

echo ===================================================
echo Copy process completed.
pause