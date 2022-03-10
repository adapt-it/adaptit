/////////////////////////////////////////////////////////////////////////////
/// \project		WdrTweaks, a sub-project of the adaptit solution 
/// \file			WdrTweaks.cpp
/// \author			Bill Martin, inspired from code taken from console.cpp by Vadim Zeitlin
/// \date_created	31 March 2019
/// \rcs_id $Id$
/// \copyright		2019 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is implementation file for a concole application that 
/// applies tweaks to wxDesigner source files (mainly Adapt_It_wdr.cpp) to 
/// eliminate run-time asserts due to incorrect wxALIGN... flags output by 
/// wxDesigner, and updates old deprecated wxFont symbols output by wxDesigner 
/// to new symbols. WdrTweaks.cpp : Defines the entry point for the console 
/// application. There is no corresponding WdrTweaks.h header file.
/////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================
// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "stdafx.h"

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWidgets headers)
#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include <wx/app.h>
#include <wx/cmdline.h>
#include <wx/textfile.h>
#include <wx/tokenzr.h>
#include <wx/filename.h>
#include <wx/ffile.h>

// ============================================================================
// implementation
// ============================================================================

static const wxCmdLineEntryDesc cmdLineDesc[] =
{
    { wxCMD_LINE_SWITCH, "h", "help", "show this help message",
    wxCMD_LINE_VAL_NONE, wxCMD_LINE_OPTION_HELP },
    { wxCMD_LINE_SWITCH, "v", "verbose", "show all changes",
    wxCMD_LINE_VAL_NONE, 0 },
    { wxCMD_LINE_SWITCH, "s", "secret", "a secret switch",
	//wxCMD_LINE_VAL_NONE, wxCMD_LINE_HIDDEN }, // whm 16Feb2021 changed second field to 0 since wxCMD_LINE_HIDDEN 'undeclared identifier'
	wxCMD_LINE_VAL_NONE, 0 },
	{ wxCMD_LINE_PARAM, "<path>", "<input-path>", "path to input wxDesigner's .cpp file from",
    wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL },
    { wxCMD_LINE_PARAM, "<path>", "<output-path>", "path to output wxDesigner's .cpp file to",
    wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL },
    // ... your other command line options here...

    wxCMD_LINE_DESC_END
};

// NOTE FOR REFERENCE: the following commented-out function DoInsert() is copied 
// from the wxWidgets-3.1.2 library source file sizer.cpp (lines 2056-2115). It 
// defines which alignment flags/flag combinations are illegal ("ignored") for an 
// item added/inserted into a wxBoxSizer - of wxVERTICAL or wxHORIZONTAL orientation. 
//wxSizerItem *wxBoxSizer::DoInsert(size_t index, wxSizerItem *item)
//{
//    const int flags = item->GetFlag();
//    if ( IsVertical() )
//    {
//        wxASSERT_MSG
//        (
//            !(flags & wxALIGN_BOTTOM),
//            wxS("Vertical alignment flags are ignored in vertical sizers")
//        );
//
//        // We need to accept wxALIGN_CENTRE_VERTICAL when it is combined with
//        // wxALIGN_CENTRE_HORIZONTAL because this is known as wxALIGN_CENTRE
//        // and we accept it historically in wxSizer API.
//        if ( !(flags & wxALIGN_CENTRE_HORIZONTAL) )
//        {
//            wxASSERT_MSG
//            (
//                !(flags & wxALIGN_CENTRE_VERTICAL),
//                wxS("Vertical alignment flags are ignored in vertical sizers")
//            );
//        }
//
//        if ( flags & wxEXPAND )
//        {
//            wxASSERT_MSG
//            (
//                !(flags & (wxALIGN_RIGHT | wxALIGN_CENTRE_HORIZONTAL)),
//                wxS("Horizontal alignment flags are ignored with wxEXPAND")
//            );
//        }
//    }
//    else // horizontal
//    {
//        wxASSERT_MSG
//        (
//            !(flags & wxALIGN_RIGHT),
//            wxS("Horizontal alignment flags are ignored in horizontal sizers")
//        );
//
//        if ( !(flags & wxALIGN_CENTRE_VERTICAL) )
//        {
//            wxASSERT_MSG
//            (
//                !(flags & wxALIGN_CENTRE_HORIZONTAL),
//                wxS("Horizontal alignment flags are ignored in horizontal sizers")
//            );
//        }
//
//        if ( flags & wxEXPAND )
//        {
//            wxASSERT_MSG(
//                !(flags & (wxALIGN_BOTTOM | wxALIGN_CENTRE_VERTICAL)),
//                wxS("Vertical alignment flags are ignored with wxEXPAND")
//            );
//        }
//    }
//
//    return wxSizer::DoInsert(index, item);
//}

// Note: In the above wx library function, the flag wxEXPAND is used, but wxDesigner uses 
// its equivalent flag wxGROW, so we will use wxGROW in our flag find-and-replace below:
// Also, in the above function the word CENTRE is used within some strings, but wxDesigner
// consistently uses the American spelling CENTER, so we will CENTER in our find-and-replace
// coding below.


wxString headerStr = _T("// This file was processed by the WdrTweaks program");

// This function removes an wxALIGN... flag specified in flagStr from the
// input string addItemStr, then returns the result as a wxString.
// The function attempts to handle adjacent '|' OR symbols appropriately.
// The caller only calls this function when it determines the flagStr exists,
// and needs to be removed, so we assume the flagStr will exist.
// Here are some examples of before and after strings, illustrating that
// if the wxALIGN_CENTER_HORIZONTAL flag is to be removed we also need to remove 
// a preceding or following '|' OR symbol so '|' OR symbols are not doubled, or
// left as preceding or following orphans:
// Example 1: 
//   [before]"    item1->Add( item16, 0, wxGROW|wxALIGN_CENTER_HORIZONTAL|wxTOP|wxBOTTOM, 5 );"
//   [after]"    item1->Add( item16, 0, wxGROW|wxTOP|wxBOTTOM, 5 );"
//   [wrong]"    item1->Add( item16, 0, wxGROW||wxTOP|wxBOTTOM, 5 );" // doubled '||'
// Example 2:
//   [before]"    item6->Add( item20, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 0 ); 
//   [after]"    item6->Add( item20, 0, wxALL, 0 ); 
//   [wrong]"    item6->Add( item20, 0, |wxALL, 0 ); // prefixed orphan '|'
// Example 3:
//   [before]"    item1->Add( item2, 0, wxGROW|wxALIGN_CENTER_HORIZONTAL, 5 );"
//   [after]"    item1->Add( item2, 0, wxGROW, 5 );"
//   [wrong]"    item1->Add( item2, 0, wxGROW|, 5 );" // suffixed orphan '|'
// Example 4:
//   [before]"    item21->Add( item22, 0, wxALIGN_CENTER_VERTICAL, 0 );
//   [after]"    item21->Add( item22, 0, 0, 0 );
//   [wrong]"    item21->Add( item22, 0, , 0 ); // If no alignment flag is given the style value must be 0
// Example 5: many Add() items have 5 fields and no itemXY as first param. These add spaces to the itemNN sizer
//   [before]"    item1->Add( 10, 20, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 0 );
//   [after]"    item1->Add( 10, 20, 0, wxALL, 0 );
// Strategy:
// 0. The itemNN->Add(...) strings are highly regularized as to their overall format.
//    There are two forms:
//        Form one - for adding item objects, which have 4 fields separated by commas, single spaces 
//          after each comma, and single space after opening ( and before closing ). In this form
//          the alignment flags are in the 3rd field - after the 2nd comma.
//        Form two - for adding spaces, which have 5 fields separated by commas, single spaces
//          after each comma, and single space after opening ( and before closing ). In this form
//          the alignment flags are in the 4th field - after the 3rd comma.
//    The whole string is prefixed by 4 spaces and ends with a semi-colon.
// 1. Parse out the sub-string value into 4 or 5 parts delimited by comma ',' chars,
//    saving each part in separate substrings. For the 4 part form, the third sub-string has 
//    the flags if present, or '0' if no style flags are present. For the 5 part form, the fourth
//    sub-string has the flags if present, or '0' if no style flags are present.
// 2. For the sub-string part containing the flags, trim off leading and following space(s).
// 3. Get an array of individual flags by parsing the flags sub-string delimited by '|' chars.
// 4. Scan the array and remove any array element that matches the specified flagStr (second function parameter).
// 5. Create a new sub-string by concatenating array elements with the '|' OR char.
//    If the array is empty, substitute a 0 character for the sub-string.
// 6. Pad the new sub-string with a leading and following space.
// 7. Concatenate the sub-string parts of the string back together - using the new sub-string flags 
//    in place of the old sub-string flags part - the only segment of the string that will be 
//    different from the original.
// 8. Return the new string.
wxString RemoveFlagFromString(wxString addItemStr, wxString flagStr, bool bVerbose)
{
    wxString Str = addItemStr;
    if (bVerbose)
        wxPrintf("Str in : [%s]\n", Str.c_str());
    wxString StrOut;
    wxString subStrPart;
    wxString subStrPartOut;
    wxString flagsStr;
    wxString singleFlag;
    wxArrayString subStringPartArray;
    wxArrayString flagStringPartArray;

    Str.Trim(FALSE);
    Str.Trim(TRUE);
    // Tokenize the Str into separate parts delimited by commas.
    wxStringTokenizer tkz(Str, _T(","), wxTOKEN_RET_EMPTY_ALL);
    while (tkz.HasMoreTokens())
    {

        subStrPart = tkz.GetNextToken();
        subStrPart.Trim(FALSE);
        subStrPart.Trim(TRUE);
        subStringPartArray.Add(subStrPart);
    }

    // The array must have eith 4 or 5 parts/fields according to the wxDesigner's
    // output of its itemNN->Add(...) statements. 
    // When the array has 4 parts, we are interested in subStringPartArray[2] - the 
    // third field - which contains the flags which must include the flag the caller 
    // wants removed. When the array has 5 parts, we are interested in subStringPartArray[3]
    // - the fourth field - which contains the flags which must include the flag the
    // caller wants removed.
    if (subStringPartArray.GetCount() == 4)
    {

        flagsStr = subStringPartArray.Item(2); // the third element
    }
    else if (subStringPartArray.GetCount() == 5)
    {
        flagsStr = subStringPartArray.Item(3); // the fourth element
    }
    flagsStr.Trim(FALSE);
    flagsStr.Trim(TRUE);
    // Tokenize the flagsStr into separate flags if multiple flags are ORed together.
    // tokenize the flag parts delimited by '|' chars
    wxStringTokenizer tkz2(flagsStr, _T("|"), wxTOKEN_RET_EMPTY_ALL);
    while (tkz2.HasMoreTokens())
    {
        singleFlag = tkz2.GetNextToken();
        singleFlag.Trim(FALSE);
        singleFlag.Trim(TRUE);
        flagStringPartArray.Add(singleFlag);
    }

    if (flagStringPartArray.GetCount() == 1)
    {
        // There was just a single flag, no '|' ORed parts, and this
        // single flag must be the one that gets removed. No flag in the style
        // is represented by a '0' style value.
        wxASSERT(flagStringPartArray[0] == flagStr);
        subStrPartOut = _T("0");
    }
    else
    {
        // There are at least two flags to process, use for loop to concatenate them
        int ct;
        int tot = (int)flagStringPartArray.GetCount();
        wxString tempStr;
        for (ct = 0; ct < tot; ct++)
        {
            tempStr = flagStringPartArray[ct];
            // Concatenate subStrPartOut, filtering out the flag that matches the input parameter flagStr
            if (tempStr != flagStr)
            {
                subStrPartOut = subStrPartOut + tempStr + _T("|");
            }
        }
        if (subStrPartOut.GetChar(subStrPartOut.Length() - 1) == _T('|')) // remove final '|'
        {
            subStrPartOut = subStrPartOut.RemoveLast(1);
        }
    }
    // Build the StrOut from its 4 or 5 parts
    if (subStringPartArray.GetCount() == 4)
        StrOut = _T("    ") + subStringPartArray[0] + _T(", ") + subStringPartArray[1] + _T(", ") + subStrPartOut + _T(", ") + subStringPartArray[3];
    else if (subStringPartArray.GetCount() == 5)
        StrOut = _T("    ") + subStringPartArray[0] + _T(", ") + subStringPartArray[1] + _T(", ") + subStringPartArray[2] + _T(", ") + subStrPartOut + _T(", ") + subStringPartArray[4];

    if (bVerbose)
        wxPrintf("Str out: [%s]\n", StrOut.c_str());
    return StrOut;
}

int main(int argc, char **argv)
{
    wxApp::CheckBuildOptions(WX_BUILD_OPTIONS_SIGNATURE, "program");

    // The main program coding starts here.

    wxString param1;
    wxString param2;
    bool bVerbose = FALSE;
    bool OutputRequiresNewFile = FALSE;
    wxArrayString params; // contains unnamed parameters for input path and output path
    wxString inFiles[] = { "Adapt_It_wdr.cpp", "ExportSaveAsDlg.cpp", "PeekAtFile.cpp" };
    wxArrayString inputFiles(3, inFiles); // First param is size/num of wxString elements in wxString inFiles[] array.

    wxInitializer initializer;
    if (!initializer)
    {
        fprintf(stderr, "Failed to initialize the wxWidgets library, aborting.");
        return -1;
    }

    wxCmdLineParser parser(cmdLineDesc, argc, argv);
    switch (parser.Parse())
    {
    case -1:
        // help was given, terminating
        break;

    case 0:
        // everything is ok; proceed
        if (parser.Found("v"))
        {
            bVerbose = TRUE;
            wxPrintf("bVerbose switch activated - all changes are shown in console output.\n");
        }
        if (parser.Found("s"))
        {
            wxPrintf("Secret switch was given...\n");
        }
        // to get at your unnamed parameters use
        for (int i = 0; i < (int)parser.GetParamCount(); i++)
        {
            params.Add(parser.GetParam(i));
        }

        break;

    default:
        break;
    }

    // whm 26Apr2021 NOTE:
    // When WdrTweaks.exe is run under Visual Studio the current working directory is in relation to WdrTweaks.vcxproj location
    // which is: something like: C:\Users\Bill\Documents\adaptit\bin\win32\WdrTweaks\
    // However, when WdrTweaks.exe is called as a Pre-Build Event the current working directory detected is different, and may be
    // any of the following: 
    //   C:\Users\Bill\Documents\adaptit\bin\win32\, or
    //   C:\Users\Bill\Documents\adaptit\bin\win32\WdrTweaks\, or
    //   C:\Users\Bill\Documents\adaptit\bin\win32\Debug_WdrTweaks\, or
    //   C:\Users\Bill\Documents\adaptit\bin\win32\Release_WdrTweaks\
    //
    // To be a good neighbor, we'll save the current work directory and restore that saved value upon exit
    wxString saveCurrWorkDir;
    saveCurrWorkDir = wxGetCwd();

    wxString dirPathSeparator = wxFileName::GetPathSeparator();
    wxString sourcePathOnly;
    wxString outputPathOnly;
    wxString wdrExePathOnly;
    wxString currWorkDir;
    int paramct;
    int paramtotal = (int)params.GetCount();
    if (paramtotal == 0)
    {
        // No parameters were given, assume default path values as calculated below:
        wxString tempStr;
        for (paramct = 0; paramct < paramtotal; paramct++)
        {
            tempStr = params[paramct];
            if (bVerbose)
                wxPrintf("Parameter %d: %s\n", paramct + 1, tempStr.c_str());
        }

        // whm 26Apr2021 modified to obtain a more reliable determination of the absolute paths after tweaking the 
        // Pre-Build/Post-Build Events for the Adapt_It and WdrTweaks projects. 
        // Set up the paths to the source directory and the executable directory using relative paths in relation
        // to the currWorkDir value as determined by a call to ::wxGetDwd(), and normalize them to absolute paths. 
        // Note: The paths to the source and executable directories could be changed by input 
        // parameters below, but we don't normally do so within the Visual Studio projects.
        // 
        // Note: The WdrTweaks.exe executable is no longer called from the adaptit/scripts directory since the 
        // WdrTweaks's Post-Build Event no longer attempts to copy the WdrTweaks.exe executable to that 
        // location. Moreover, the Adapt_It Pre-Build Event now simply calls the WdrTweaks.exe executable 
        // from its build location which is:
        //   C:\Users\Bill\Documents\adaptit\bin\win32\Debug_WdrTweaks\WdrTweaks.exe, or
        //   C:\Users\Bill\Documents\adaptit\bin\win32\Release_WdrTweaks\WdrTweaks.exe
        // The sourcePathOnly starts as a relative path as calculated relative to the current work directory which
        // could be the WdrTweaks.vcxproj project file or another location such as the win32 directory. We'll check the 
        // top level directory of the currWorkDir and work from that relative node.
        // Note: The returned string from the ::wxGetCwd() call does NOT have a final path separator, and will usually
        // be one of the following:
        //   C:\Users\Bill\Documents\adaptit\bin\win32\WdrTweaks  [when Debug_WdrTweaks or Release_WdrTweaks are run as debug process]
        //   C:\Users\Bill\Documents\adaptit\bin\win32 [when WdrTweaks.exe is called from Adapt_It project's Pre-Build Event]
        //   ??? other path locations with topDir of "Debug_WdrTweaks" or "Release_WdrTweaks" - where WdrTweaks.exe resides ???
        // We check the above possible topDir locations below and assign relative paths accordingly.
        currWorkDir = ::wxGetCwd();
        wxLogDebug("WdrTweaks currWorkDir: %s", currWorkDir.c_str());
        wxPrintf("*** WdrTweaks currWorkDir: %s\n", currWorkDir.c_str());

        // determine the last directory in the currWordDir string. 
        // If it is win32 we bump back 2 levels to the adaptit (or adaptit-git) directory
        // If it is WdrTweaks or Debug_WdrTweaks or Release_WdrTweaks we bump back 3 levels to the adaptit (or adaptit-git) directory
        wxString topDir;
        topDir = currWorkDir.AfterLast(_T('\\'));
        if (topDir == _T("win32"))
        {
            sourcePathOnly = currWorkDir + dirPathSeparator + _T("..") + dirPathSeparator + _T("..") + dirPathSeparator + _T("source") + dirPathSeparator; 
            // sourcePathOnly is:
            // ..\..\source\ 
            //
#if defined (_DEBUG) 
            wdrExePathOnly = currWorkDir + dirPathSeparator + _T("Debug_WdrTweaks") + dirPathSeparator; 
            // wdrExePathOnly is:
            // C:\Users\Bill\Documents\adaptit\bin\win32\Debug_WdrTweaks\ 
            //
#else
            wdrExePathOnly = currWorkDir + dirPathSeparator + _T("Release_WdrTweaks") + dirPathSeparator;
            // wdrExePathOnly is:
            // C:\Users\Bill\Documents\adaptit\bin\win32\Release_WdrTweaks\ 
            //
#endif
        }
        else if (topDir == _T("WdrTweaks") || topDir == _T("Debug_WdrTweaks") || topDir == _T("Release_WdrTweaks"))
        {
            sourcePathOnly = currWorkDir + dirPathSeparator + _T("..") + dirPathSeparator + _T("..") + dirPathSeparator + _T("..") + dirPathSeparator + _T("source") + dirPathSeparator; 
            // sourcePathOnly is:
            // ..\..\..\source\ 
            //
            wdrExePathOnly = currWorkDir + dirPathSeparator; 
            // wdrExePathOnly may be:
            // C:\Users\Bill\Documents\adaptit\bin\win32\WdrTweaks\ or  
            // C:\Users\Bill\Documents\adaptit\bin\win32\Debug_WdrTweaks\ or  
            // C:\Users\Bill\Documents\adaptit\bin\win32\Release_WdrTweaks\ 
            //
        }
        else
        {
            wxString msg = _T("topDir of currWorkDir had unexpected value: %s");
            msg = msg.Format(msg, topDir.c_str());
            wxPrintf("*** ERROR - %s ***",msg.c_str());
            return 1;
        }

        wxFileName fn1(sourcePathOnly);
        fn1.Normalize(); // gets the absolute path without internal \..\ relativizing dots
        sourcePathOnly = fn1.GetFullPath();//  (sourcePathOnly, &parentDir, NULL, NULL);
        wxLogDebug("WdrTweaks sourcePathOnly: %s", sourcePathOnly.c_str());
        wxPrintf("*** WdrTweaks sourcePathOnly: %s\n", sourcePathOnly.c_str());

        // The outputPathOnly is the same as the sourcePathOnly, i.e. the .cpp file(s) written back to their
        // same location.
        outputPathOnly = sourcePathOnly;
        wxLogDebug("WdrTweaks outputPathOnly: %s", outputPathOnly.c_str());
        wxPrintf("*** WdrTweaks outputPathOnly: %s\n", outputPathOnly.c_str());

        wxFileName fn2(wdrExePathOnly);
        fn2.Normalize(); // gets the absolute path without internal \..\ relativizing dots
        wdrExePathOnly = fn2.GetFullPath(); // gets it in native format, and absolute path including final separator
        wxLogDebug("***WdrTweaks wdrExePathOnly: %s", wdrExePathOnly.c_str());
        wxPrintf("*** WdrTweaks wdrExePathOnly: %s\n", wdrExePathOnly.c_str());
    }
    else
    {
        // whm 26Apr2021 Currently we do not invoke WdrTweaks.exe with input parameters from the Adapt_it project's Pre-Build Event
        // therefore this else block should not execute in normal usage within Visual Studio
        // At least one path parameter was given.
        if (paramtotal == 1)
        {
            sourcePathOnly = params[0];
            outputPathOnly = sourcePathOnly;
        }
        else if (paramtotal == 2)
        {
            sourcePathOnly = params[0];
            outputPathOnly = params[1];
        }
    }
    if (sourcePathOnly != outputPathOnly)
    {
        // Parameters require output go to a different path than the input came from.
        // To do this with wxTextFile, we need to create a new file and iterate through
        // all lines of the open file, and save the changes made in memory out to the
        // new output file.
        OutputRequiresNewFile = TRUE;
    }

    // Set up loop to process each source file in inputFiles array.
    int filect;
    int filetotal = (int)inputFiles.GetCount();
    for (filect = 0; filect < filetotal; filect++)
    {
        // inputFiles in array are: "Adapt_It_wdr.cpp", "ExportSaveAsDlg.cpp", "PeekAtFile.cpp"
        wxString inputFilename = inputFiles[filect];
        // The default outputFilename is same as inputFilename - name doesn't change when output
        wxString outputFilename = inputFilename;

        // Ensure the sourcePathOnly and outputPathOnly end in path separator (user may not have typed 
        // path parameter(s) with terminating separators).
        if (sourcePathOnly.Right(1) != wxFileName::GetPathSeparator())
            sourcePathOnly = sourcePathOnly + dirPathSeparator;
        if (outputPathOnly.Right(1) != wxFileName::GetPathSeparator())
            outputPathOnly = outputPathOnly + dirPathSeparator;

        wxString inputFilePathAndName = sourcePathOnly + inputFilename;
        wxString outputFilePathAndName = outputPathOnly + outputFilename;
        wxString executableFilePathAndName = wdrExePathOnly + _T("WdrTweaks.exe");
        //if (bVerbose)
        //{
        wxPrintf("***********************************************************************\n");
        //wxPrintf("The executable path/name is: %s\n", executableFilePathAndName.c_str());
        wxPrintf("The input path/name is: %s\n", inputFilePathAndName.c_str());
        wxPrintf("The output path/name is: %s\n", outputFilePathAndName.c_str());
        //}
        currWorkDir = wxFileName::GetCwd();

        wxTextFile file(inputFilePathAndName);
        wxPrintf("    Opening and reading %s - please wait... ",inputFilename.c_str());
        bool bOK;
        bOK = file.Open();
        if (!bOK)
        {
            wxPrintf("Could NOT OPEN input path/name at: %s\n", inputFilePathAndName.c_str());
            return 1;
        }
        // Check for the "// This file was processed by the sedConvert.sh script" header comment.
        // If it already exists we need not continue
        int totalLineCt = 0;
        totalLineCt = file.GetLineCount();
        wxPrintf("total number of lines read from file: %d.\n", totalLineCt);

        bool bFileAlreadyProcessed = FALSE;
        wxString firstLine = file.GetLine(0);
        if (firstLine.Find(headerStr) != wxNOT_FOUND)
        {
            bFileAlreadyProcessed = TRUE;
            wxPrintf("      %s was already processed by the conversion script.\n", inputFilename.c_str());
        }
        else
        {
            bFileAlreadyProcessed = FALSE;
            wxPrintf("      Processing %s ", inputFilename.c_str()); // don't include \n here so dots in routines below can be added on same line
        }

        if (!bFileAlreadyProcessed)
        {

            // Note: The following two arrays collect info about lines that have deprecated font symbols
            // used by wxDesifner. These arrays are in parallel and represent the in-file line number
            // and the whole content of the corresponding in-file line - both as strings.
            wxArrayString deprecatedFont_LineNumStr_Array;
            wxArrayString deprecatedFont_LineContent_Array;
            //
            // Note: The four wxArrayString arrays below have indices that start from 0 within each 
            // wxDesigner function.
            // The following two arrays are in parallel and describe the wxBoxSizer items created with 'new'. 
            // They indicate the itemNN of the created wxBoxSizer, and its corresponding orientation flag, 
            // either wxHORIZONTAL or wxVERTICAL. We will remove/adjust certain wxALIGN... flags that
            // wxWidgets-3.1.x issues run-time asserts for as 'ignored' for the given orientation.
            wxArrayString wxBoxSizerItemNNStr_Array;
            wxArrayString wxBoxSizerOrientationHorV_Array;
            // The followoing two arrays are in parallel and describe the itemNN->Add( itemXY ...) lines, in
            // which itemNN refers to a previously created wxBoxSizer, and itemXY refers to the object being
            // added/inserted into that wxBoxSizer. These two arrays indicate the in-file line number and
            // the whole content of the corresponding in-file line - both as strings.
            wxArrayString addItem_LineNumStr_Array;  // array of the in-file line numbers (as strings), i.e., '201'
            wxArrayString addItem_LineContent_Array; // array of the whole line content (as strings), i.e., '    item30->Add( item31, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 0 );'

            wxString currLine;
            wxString itemNNStr = _T("");
            wxString wxHorVStr = _T("");
            wxString funcName = _T("");
            int inFuncBoxSizerItemArrayIndex = 0;
            int i;
            int numLinesChanged = 0;
            for (i = 0; i < totalLineCt; i++)
            {
                currLine = file.GetLine(i);

                // Collect info on any deprecated font symbols into parallel arrays
                // Collect the lines and line numbers that have deprecated font symbols
                // The find and replace will be done later below after alignment flag 
                // processing.
                if (currLine.Find(_T("wxNORMAL")) != wxNOT_FOUND
                    || currLine.Find(_T("wxSWISS")) != wxNOT_FOUND
                    || currLine.Find(_T("wxROMAN")) != wxNOT_FOUND
                    || currLine.Find(_T("wxBOLD")) != wxNOT_FOUND
                    || currLine.Find(_T("wxITALIC")) != wxNOT_FOUND)
                {
                    wxString lineNumStr;
                    lineNumStr << i;
                    deprecatedFont_LineNumStr_Array.Add(lineNumStr);
                    deprecatedFont_LineContent_Array.Add(currLine);
                }

                if (currLine.Find(_T("( wxWindow *parent, bool call_fit, bool set_sizer )")) != wxNOT_FOUND)
                {
                    // currLine is at the beginning of a new wxDesigner func.
                    // Parse out the function name.
                    if (bVerbose)
                        wxPrintf("Function: %s\n", currLine.c_str());
                    int posnFuncName = currLine.Find(_T("wxSizer *"));
                    int posnFollParen = posnFuncName + 9; // "wxSizer *"
                    int ct = posnFollParen;
                    while (currLine.GetChar(ct) != '(')
                    {
                        funcName = funcName + currLine.GetChar(ct);
                        ct++;
                    }
                    //wxLogDebug(_T("Function Name: %s"),funcName.c_str());
                    // Clear arrays and reset array indices to 0.
                    inFuncBoxSizerItemArrayIndex = 0;
                    wxBoxSizerItemNNStr_Array.Clear();
                    wxBoxSizerOrientationHorV_Array.Clear();
                    addItem_LineContent_Array.Clear();
                    addItem_LineNumStr_Array.Clear();
                }
                else if (currLine.Find(_T("wxBoxSizer *item")) != wxNOT_FOUND)
                {
                    // currLine is at the creation of a 'new' wxBoxSizer.
                    // Example: wxBoxSizer *item7 = new wxBoxSizer( wxHORIZONTAL );
                    // Parse out the itemNNStr and Add it to the wxBoxSizerItemNNStr_Array array, 
                    // and parse out the wxHorVStr orientation and Add it to the parallel 
                    // wxBoxSizerOrientationHorV_Array array.
                    int posnBS = currLine.Find(_T("wxBoxSizer *item"));
                    int posnFollSp = posnBS + 16; // point to end of "wxBoxSizer *item"
                    int ct = posnFollSp;
                    itemNNStr = "";
                    // Gather the number NN suffixed to "item"
                    while (currLine.GetChar(ct) != ' ')
                    {
                        itemNNStr = itemNNStr + currLine.GetChar(ct);
                        ct++;
                    }
                    int posnNB = currLine.Find(_T("new wxBoxSizer( "));
                    int posnFollSpNB = posnNB + 16; // point to the end of "new wxBoxSizer( "
                    ct = posnFollSpNB;
                    wxHorVStr = "";
                    // Gather the orientation flag, "wxVERTICAL" or "wxHORIZONTAL"
                    while (currLine.GetChar(ct) != ' ')
                    {
                        wxHorVStr = wxHorVStr + currLine.GetChar(ct);
                        ct++;
                    }
                    itemNNStr = _T("item") + itemNNStr;
                    // Populate the parallel wxBoxSizer info arrays
                    wxBoxSizerItemNNStr_Array.Add(itemNNStr);
                    wxBoxSizerOrientationHorV_Array.Add(wxHorVStr);
                    //wxLogDebug("item as str: %s, H or V: %s",itemNNStr.c_str(), wxHorVStr.c_str());
                    inFuncBoxSizerItemArrayIndex++;
                }
                else if (currLine.Find(_T("wxStaticBoxSizer *item")) != wxNOT_FOUND)
                {
                    // currLine is at the creation of a 'new' wxStaticBoxSizer (also has orientation).
                    // Example: wxStaticBoxSizer *item8 = new wxStaticBoxSizer( item9, wxVERTICAL );
                    // NOTE: The wxStaticBoxSizer constructor has an itemMM parameter before the
                    // wxVERTICAL or wxHORIZONTAL orientation flag, so we parse the line accordingly
                    // below.
                    // Parse out the itemNNStr and Add it to the wxBoxSizerItemNNStr_Array array, 
                    // and parse out the wxHorVStr orientation and Add it to the parallel 
                    // wxBoxSizerOrientationHorV_Array array.
                    int posnBS = currLine.Find(_T("wxStaticBoxSizer *item"));
                    int posnFollSp = posnBS + 22; // point to end of "wxStaticBoxSizer *item"
                    int ct = posnFollSp;
                    itemNNStr = "";
                    // Gather the number NN suffixed to "item"
                    while (currLine.GetChar(ct) != ' ')
                    {
                        itemNNStr = itemNNStr + currLine.GetChar(ct);
                        ct++;
                    }
                    // Parse past the first itemMM parameter of the wsStaticBoxSizer constructor.
                    int posnNB = currLine.Find(_T("new wxStaticBoxSizer( "));
                    int posnFollSpNB = posnNB + 22; // point to the end of "new wxBoxSizer( "
                    ct = posnFollSpNB;
                    // Advance over 'itemMM,' to the next space after the comma to point to the beginning of the orientation flag
                    while (currLine.GetChar(ct) != ' ')
                    {
                        ct++;
                    }
                    ct++; // point at 'w' of wxVERTICAL or wxHORIZONTAL
                    wxHorVStr = "";
                    // Now gather the orientation flag, "wxVERTICAL" or "wxHORIZONTAL"
                    while (currLine.GetChar(ct) != ' ')
                    {
                        wxHorVStr = wxHorVStr + currLine.GetChar(ct);
                        ct++;
                    }
                    itemNNStr = _T("item") + itemNNStr;
                    // Populate the parallel wxBoxSizer info arrays
                    wxBoxSizerItemNNStr_Array.Add(itemNNStr);
                    wxBoxSizerOrientationHorV_Array.Add(wxHorVStr);
                    //wxPrintf("wxStaticBoxSizer item as str: %s, H or V: %s\n",itemNNStr.c_str(), wxHorVStr.c_str()); // TODO: comment out
                    inFuncBoxSizerItemArrayIndex++;
                }
                else if (currLine.Find(_T("->Add( ")) != wxNOT_FOUND)
                {
                    // Example 1: item0->Add( item19, 0, wxALL, 0 ); // uses 4 parameters to add sizer item - flags at field 3
                    // Example 2: item43->Add( 15, 10, 0, wxALIGN_CENTER|wxALL, 5 ); // uses 5 parameters to add space - flags at field 4
                    // currLine is at the Add item line (with flags to possibly remove/adjust).
                    // Add a lineNumStr to the addItem_LineNumStr_Array and currLine content to 
                    // the parallel addItem_LineContent_Array array.
                    wxString lineNumStr;
                    lineNumStr << i;
                    // Populate the parallel Add item info arrays
                    addItem_LineNumStr_Array.Add(lineNumStr);
                    addItem_LineContent_Array.Add(currLine);
                }
                else if (currLine.Find(_T("return item0")) != wxNOT_FOUND)
                {
                    // currLine is at the end of the wxDesigner func.
                    // This else if block is where the work of alignment flag modification is done.
                    inFuncBoxSizerItemArrayIndex--;
                    int totalItemNNArrayElements = (int)wxBoxSizerItemNNStr_Array.GetCount();
                    int sizerItemNNCt;
                    for (sizerItemNNCt = 0; sizerItemNNCt < totalItemNNArrayElements; sizerItemNNCt++)
                    {
                        //wxLogDebug(_T("item as str: [%s], H or V: [%s]"),wxBoxSizerItemNNStr_Array[sizerItemNNCt].c_str(),wxBoxSizerOrientationHorV_Array[sizerItemNNCt].c_str());
                    }
                    //wxLogDebug("Total wxBoxSizers in function %s: %d",funcName.c_str(),totalItemNNArrayElements);

                    // Now that we are at the end of a wxDesigner function, scan through the itemNN->Add( itemXY, 0, <flags>, 0) 
                    // statements' array data, and detect any bad flag styles associated with the itemNN's wxBoxSizer orientation. 
                    // We have detect itemNN's orientation - wxHORIZONTAL or wxVERTICAL - and for the given orientation
                    // check the flags of the <flags> being assigned to the itemXY added item.
                    int totalAddItemElements = (int)addItem_LineContent_Array.GetCount();
                    int addItemCt;
                    if (bVerbose)
                        wxPrintf("-----------------------------------------------------------------------\n");
                    for (addItemCt = 0; addItemCt < totalAddItemElements; addItemCt++)
                    {
                        wxString addItemLineContent = addItem_LineContent_Array[addItemCt];
                        wxString itemToAddTo = addItemLineContent;
                        int posHyphen = itemToAddTo.Find(_T("-"));
                        itemToAddTo = itemToAddTo.Left(posHyphen);
                        itemToAddTo.Trim(FALSE);
                        itemToAddTo.Trim(TRUE);
                        // Lookup the itemToAddTo in the wxBoxSizerItemNNStr_Array, get its index, 
                        // then read the value in the wxBoxSizerOrientationHorV_Array at 
                        // that index to determine if it is wxHORIZONTAL or wxVERTICAL
                        int ii;
                        wxString horv = _T("");
                        for (ii = 0; ii < (int)wxBoxSizerItemNNStr_Array.GetCount(); ii++)
                        {
                            if (itemToAddTo == wxBoxSizerItemNNStr_Array[ii])
                            {
                                horv = wxBoxSizerOrientationHorV_Array[ii];
                                break;
                            }
                        }
                        if (horv == _T(""))
                        {
                            horv = _T("nonBoxSizer");
                        }
                        // Note: when horv is an empty string, it means that the itemNN was not a wxBoxSizer, 
                        // but something else such as a wxFlexGridSizer. We are only concerned with the 
                        // flags of items added to wxBoxSizers.

                        //wxLogDebug("%s",itemToAddTo.c_str());
                        //wxLogDebug("CurrLine:%s:%s:%s:%s",addItem_LineNumStr_Array[addItemCt].c_str(), itemToAddTo.c_str(), horv.c_str(), addItem_LineContent_Array[addItemCt].c_str());
                        // Detect run-time assert situations (ala conditions checked for in sizer.cpp) and adjust flags in 
                        // the wxTextFile line at addItem_LineNumStr_Array, and save the modified 
                        // line back to that same line in the wxTextFile.
                        bool changeMadeToAddItemContentLine = FALSE;
                        if (horv == "wxVERTICAL")
                        {
                            // The wxBoxSizer item being added to (itemNN) is vertical oriented, and
                            // according to wx sizer.cpp, it must not have these flags/flag combinations:
                            // wxALIGN_BOTTOM
                            // wxALIGN_CENTER_VERTICAL (without also wxALIGN_CENTER_HORIZONTAL present)
                            // wxGROW plus either wxALIGN_RIGHT | wxALIGN_CENTER_HORIZONTAL [two conditions; wxGROW plus wxALIGN_RIGHT, wxGROW plus wxALIGN_CENTER_HORIZONTAL]
                            if (addItemLineContent.Find(_T("wxALIGN_BOTTOM")) != wxNOT_FOUND)
                            {
                                // wxALIGN_BOTTOM is a no-no flag for wxVERTICAL oriented wxBoxSizers, 
                                // so remove the wxALIGN_BOTTOM flag.
                                if (bVerbose)
                                    wxPrintf("Need to remove: wxALIGN_BOTTOM flag in %s Line: %s\n", inputFilename.c_str(), addItem_LineNumStr_Array[addItemCt].c_str());
                                else if (numLinesChanged % 10 == 0)
                                    wxPrintf(".");
                                addItemLineContent = RemoveFlagFromString(addItemLineContent, _T("wxALIGN_BOTTOM"), bVerbose);
                                changeMadeToAddItemContentLine = TRUE;
                                numLinesChanged++;
                            }
                            if (addItemLineContent.Find(_T("wxALIGN_CENTER_VERTICAL")) != wxNOT_FOUND && addItemLineContent.Find(_T("wxALIGN_CENTER_HORIZONTAL")) == wxNOT_FOUND)
                            {
                                // A wxALIGN_CENTER_VERTICAL flag was present, without there also being a 
                                // wxALIGN_CENTER_HORIZONTAL flag present, so remove the wxALIGN_CENTER_VERTICAL 
                                // flag.
                                if (bVerbose)
                                    wxPrintf("Need to remove: wxALIGN_CENTER_VERTICAL flag in %s Line: %s\n", inputFilename.c_str(), addItem_LineNumStr_Array[addItemCt].c_str());
                                else if (numLinesChanged % 10 == 0)
                                    wxPrintf(".");
                                addItemLineContent = RemoveFlagFromString(addItemLineContent, _T("wxALIGN_CENTER_VERTICAL"), bVerbose);
                                changeMadeToAddItemContentLine = TRUE;
                                numLinesChanged++;
                            }
                            if (addItemLineContent.Find(_T("wxGROW")) != wxNOT_FOUND && addItemLineContent.Find(_T("wxALIGN_RIGHT")) != wxNOT_FOUND)
                            {
                                if (bVerbose)
                                    wxPrintf("Need to remove: wxALIGN_RIGHT flag in %s Line: %s\n", inputFilename.c_str(), addItem_LineNumStr_Array[addItemCt].c_str());
                                else if (numLinesChanged % 10 == 0)
                                    wxPrintf(".");
                                addItemLineContent = RemoveFlagFromString(addItemLineContent, _T("wxALIGN_RIGHT"), bVerbose);
                                changeMadeToAddItemContentLine = TRUE;
                                numLinesChanged++;
                            }
                            if (addItemLineContent.Find(_T("wxGROW")) != wxNOT_FOUND && addItemLineContent.Find(_T("wxALIGN_CENTER_HORIZONTAL")) != wxNOT_FOUND)
                            {
                                if (bVerbose)
                                    wxPrintf("Need to remove: wxALIGN_CENTER_HORIZONTAL flag in %s Line: %s\n", inputFilename.c_str(), addItem_LineNumStr_Array[addItemCt].c_str());
                                else if (numLinesChanged % 10 == 0)
                                    wxPrintf(".");
                                addItemLineContent = RemoveFlagFromString(addItemLineContent, _T("wxALIGN_CENTER_HORIZONTAL"), bVerbose);
                                changeMadeToAddItemContentLine = TRUE;
                                numLinesChanged++;
                            }
                        }
                        else if (horv == "wxHORIZONTAL")
                        {
                            // The wxBoxSizer item being added to (itemNN) is horizontal oriented, and
                            // according to wx sizer.cpp, it must not have these flags/flag combinations:
                            // wxALIGN_RIGHT
                            // wxALIGN_CENTER_HORIZONTAL (without also wxALIGN_CENTER_VERTICAL present)
                            // wxGROW plus either wxALIGN_BOTTOM | wxALIGN_CENTER_VERTICAL [two conditions; wxGROW plus wxALIGN_BOTTOM, wxGROW plus wxALIGN_CENTER_VERTICAL]
                            if (addItemLineContent.Find(_T("wxALIGN_RIGHT")) != wxNOT_FOUND)
                            {
                                // wxALIGN_RIGHT is a no-no flag for wxHORIZONTAL oriented wxBoxSizers, so remove it
                                if (bVerbose)
                                    wxPrintf("Need to remove: wxALIGN_RIGHT flag in %s Line: %s\n", inputFilename.c_str(), addItem_LineNumStr_Array[addItemCt].c_str());
                                else if (numLinesChanged % 10 == 0)
                                    wxPrintf(".");
                                addItemLineContent = RemoveFlagFromString(addItemLineContent, _T("wxALIGN_RIGHT"), bVerbose);
                                changeMadeToAddItemContentLine = TRUE;
                                numLinesChanged++;
                            }
                            if (addItemLineContent.Find(_T("wxALIGN_CENTER_HORIZONTAL")) != wxNOT_FOUND && addItemLineContent.Find(_T("wxALIGN_CENTER_VERTICAL")) == wxNOT_FOUND)
                            {
                                // A wxALIGN_CENTER_HORIZONTAL flag was present, without there also being a 
                                // wxALIGN_CENTER_VERTICAL flag present, so remove the wxALIGN_CENTER_HORIZONTAL 
                                // flag.
                                if (bVerbose)
                                    wxPrintf("Need to remove: wxALIGN_CENTER_HORIZONTAL flag in %s Line: %s\n", inputFilename.c_str(), addItem_LineNumStr_Array[addItemCt].c_str());
                                else if (numLinesChanged % 10 == 0)
                                    wxPrintf(".");
                                addItemLineContent = RemoveFlagFromString(addItemLineContent, _T("wxALIGN_CENTER_HORIZONTAL"), bVerbose);
                                changeMadeToAddItemContentLine = TRUE;
                                numLinesChanged++;
                            }
                            if (addItemLineContent.Find(_T("wxGROW")) != wxNOT_FOUND && addItemLineContent.Find(_T("wxALIGN_BOTTOM")) != wxNOT_FOUND)
                            {
                                if (bVerbose)
                                    wxPrintf("Need to remove: wxALIGN_BOTTOM flag in %s Line: %s\n", inputFilename.c_str(), addItem_LineNumStr_Array[addItemCt].c_str());
                                else if (numLinesChanged % 10 == 0)
                                    wxPrintf(".");
                                addItemLineContent = RemoveFlagFromString(addItemLineContent, _T("wxALIGN_BOTTOM"), bVerbose);
                                changeMadeToAddItemContentLine = TRUE;
                                numLinesChanged++;
                            }
                            if (addItemLineContent.Find(_T("wxGROW")) != wxNOT_FOUND && addItemLineContent.Find(_T("wxALIGN_CENTER_VERTICAL")) != wxNOT_FOUND)
                            {
                                if (bVerbose)
                                    wxPrintf("Need to remove: wxALIGN_CENTER_VERTICAL flag in %s Line: %s\n", inputFilename.c_str(), addItem_LineNumStr_Array[addItemCt].c_str());
                                else if (numLinesChanged % 10 == 0)
                                    wxPrintf(".");
                                addItemLineContent = RemoveFlagFromString(addItemLineContent, _T("wxALIGN_CENTER_VERTICAL"), bVerbose);
                                changeMadeToAddItemContentLine = TRUE;
                                numLinesChanged++;
                            }
                        }
                        else
                        {
                            // the itemToAddTo is neither wxVERTICAL nor wxHORIZONTAL orientation, 
                            // it must not be wxBoxSizer, and so we can ignore it
                            ;
                        }
                        if (changeMadeToAddItemContentLine == TRUE)
                        {
                            // The Add item line was changed, so write it back to its line location in wxTextFile
                            // wxTextFile doesn't have a change line method, so must call wxTextFile's RemoveLine(n) 
                            // and InsertLine(str, n) methods.

                            int lineNum;
                            lineNum = wxAtoi(addItem_LineNumStr_Array[addItemCt]);
                            file.RemoveLine(lineNum);
                            file.InsertLine(addItemLineContent, lineNum);
                        }
                    }
                    if (bVerbose)
                        wxPrintf("-----------------------------------------------------------------------\n");

                    // Clear data arrays for next function.
                    funcName = _T("");
                    wxBoxSizerItemNNStr_Array.Clear();
                    wxBoxSizerOrientationHorV_Array.Clear();
                    addItem_LineContent_Array.Clear();
                    addItem_LineNumStr_Array.Clear();

                } // end of else

                  // We need only find-and-replace within the wxDesigner dialog functions.
                  // When we get to the wxMenuBar *AIMenuBarFunc() the dialogs are done and
                  // we can quit processing at about line 9700 out of about 24,800 lines
                  // total in the wxDesigner generated Adapt_It_wdr.cpp file. The last 15,000
                  // or so lines are mostly menu bar, tool bar and XPM bitmap images which do 
                  // not need to be processed.
                if (currLine.Find("wxMenuBar *AIMenuBarFunc()") == 0)
                    break;
            } // end of for loop
            if (bVerbose)
                wxPrintf("Number of lines where flags were changed: %d\n", numLinesChanged);

            // Now do the find-and-replace deprecated font symbols
            int fontLineCt;
            wxASSERT(deprecatedFont_LineNumStr_Array.GetCount() == deprecatedFont_LineContent_Array.GetCount());
            int fontLineTotal = (int)deprecatedFont_LineNumStr_Array.GetCount();
            int fontLineNum;
            wxString fontLineContent;
            if (bVerbose)
                wxPrintf("-----------------------------------------------------------------------\n");
            for (fontLineCt = 0; fontLineCt < fontLineTotal; fontLineCt++)
            {
                fontLineNum = wxAtoi(deprecatedFont_LineNumStr_Array[fontLineCt]);
                fontLineContent = deprecatedFont_LineContent_Array[fontLineCt];
                // Find the deprecated font symbols and replace them with accepted font symbols.
                // Here are the sed find-replace patterns we've used in the script model,
                // which we can do below with simple wxString::Replace() calls.
                //s/wxNORMAL, wxNORMAL/wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL/g
                //s/wxNORMAL,/wxFONTSTYLE_NORMAL,/g
                //s/, wxNORMAL )/, wxFONTWEIGHT_NORMAL )/g
                //s/wxSWISS/wxFONTFAMILY_SWISS/g
                //s/wxROMAN/wxFONTFAMILY_ROMAN/g
                //s/wxBOLD/wxFONTWEIGHT_BOLD/g
                //s/wxITALIC/wxFONTSTYLE_ITALIC/g

                if (fontLineContent.Find(_T("wxNORMAL, wxNORMAL")) != wxNOT_FOUND)
                {
                    if (bVerbose)
                        wxPrintf("Need to replace: 'wxNORMAL, wxNORMAL' with 'wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL' in %s Line: %s\n", inputFilename.c_str(), deprecatedFont_LineNumStr_Array[fontLineCt].c_str());
                    else if (fontLineCt % 5 == 0)
                        wxPrintf(".");
                    fontLineContent.Replace("wxNORMAL, wxNORMAL", "wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL");
                }

                if (fontLineContent.Find(_T("wxNORMAL,")) != wxNOT_FOUND)
                {
                    if (bVerbose)
                        wxPrintf("Need to replace: 'wxNORMAL,' with 'wxFONTSTYLE_NORMAL,' in %s Line: %s\n", inputFilename.c_str(), deprecatedFont_LineNumStr_Array[fontLineCt].c_str());
                    else if (fontLineCt % 5 == 0)
                        wxPrintf(".");
                    fontLineContent.Replace("wxNORMAL,", "wxFONTSTYLE_NORMAL,");
                }

                if (fontLineContent.Find(_T(", wxNORMAL )")) != wxNOT_FOUND)
                {
                    if (bVerbose)
                        wxPrintf("Need to replace: ', wxNORMAL )' with ', wxFONTWEIGHT_NORMAL )' in %s Line: %s\n", inputFilename.c_str(), deprecatedFont_LineNumStr_Array[fontLineCt].c_str());
                    else if (fontLineCt % 5 == 0)
                        wxPrintf(".");
                    fontLineContent.Replace(", wxNORMAL )", ", wxFONTWEIGHT_NORMAL )");
                }

                if (fontLineContent.Find(_T("wxSWISS")) != wxNOT_FOUND)
                {
                    if (bVerbose)
                        wxPrintf("Need to replace: 'wxSWISS' with 'wxFONTFAMILY_SWISS' in %s Line: %s\n", inputFilename.c_str(), deprecatedFont_LineNumStr_Array[fontLineCt].c_str());
                    else if (fontLineCt % 5 == 0)
                        wxPrintf(".");
                    fontLineContent.Replace("wxSWISS", "wxFONTFAMILY_SWISS");
                }

                if (fontLineContent.Find(_T("wxROMAN")) != wxNOT_FOUND)
                {
                    if (bVerbose)
                        wxPrintf("Need to replace: 'wxROMAN' with 'wxFONTFAMILY_ROMAN' in %s Line: %s\n", inputFilename.c_str(), deprecatedFont_LineNumStr_Array[fontLineCt].c_str());
                    else if (fontLineCt % 5 == 0)
                        wxPrintf(".");
                    fontLineContent.Replace("wxROMAN", "wxFONTFAMILY_ROMAN");
                }

                if (fontLineContent.Find(_T("wxBOLD")) != wxNOT_FOUND)
                {
                    if (bVerbose)
                        wxPrintf("Need to replace: 'wxBOLD' with 'wxFONTWEIGHT_BOLD' in %s Line: %s\n", inputFilename.c_str(), deprecatedFont_LineNumStr_Array[fontLineCt].c_str());
                    else if (fontLineCt % 5 == 0)
                        wxPrintf(".");
                    fontLineContent.Replace("wxBOLD", "wxFONTWEIGHT_BOLD");
                }

                if (fontLineContent.Find(_T("wxITALIC")) != wxNOT_FOUND)
                {
                    if (bVerbose)
                        wxPrintf("Need to replace: 'wxITALIC' with 'wxFONTSTYLE_ITALIC' in %s Line: %s\n", inputFilename.c_str(), deprecatedFont_LineNumStr_Array[fontLineCt].c_str());
                    else if (fontLineCt % 5 == 0)
                        wxPrintf(".");
                    fontLineContent.Replace("wxITALIC", "wxFONTSTYLE_ITALIC");
                }

                // Save the changed fontLineContent back to the wxTextFile. We have to
                // do it via wxTextFile's RemoveLine(n) and InsertLine(str, n) methods.
                file.RemoveLine(fontLineNum);
                file.InsertLine(fontLineContent, fontLineNum);
            }
            if (!bVerbose)
                wxPrintf(".\n");
            if (bVerbose)
            {
                wxPrintf("***********************************************************************\n");
                wxPrintf("Number of lines where deprecated font symbols were updated: %d\n", fontLineTotal);
                wxPrintf("-----------------------------------------------------------------------\n");
            }

            file.InsertLine(headerStr, 0);
            if (OutputRequiresNewFile)
            {
                // Create new file for text output.
                wxFFile fileOut;
                const wxChar writemode[] = _T("w");
                fileOut.Open(outputFilePathAndName, writemode);
                wxPrintf("    Writing output to file: %s\n", outputFilePathAndName.c_str()); // always print this
                for (i = 0; i < totalLineCt; i++)
                {
                    currLine = file.GetLine(i);
                    // TODO: check if need to add eol char?
                    // TODO: After debuggin uncomment following line to write changes to the external file
                    fileOut.Write(currLine); // Write each line from memory to new external file.
                }
                fileOut.Close();
                file.Close(); // Close the input wxTextFile without writing to it.
            }
            else
            {
                wxPrintf("    Writing output to file: %s\n", outputFilePathAndName.c_str());
                file.Write(); // Write whole wxTextFile from memory to external file.
            }
        } // end of if (!bFileAlreadyProcessed)
        else
        {
            // File was already processed, so just Close the file.
            file.Close();
        }
    } // end of for (filect = 0; filect < filetotal; filect++)

    // whm 26Apr2021 added restore current working directory
    wxLogNull logNo;	// eliminates any spurious messages from the system if the wxSetWorkingDirectory() returns FALSE
    ::wxSetWorkingDirectory(saveCurrWorkDir); // ignore failures

    return 0;
} // end of main()