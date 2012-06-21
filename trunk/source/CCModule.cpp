/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			CCModule.cpp
/// \author			Bill Martin
/// \date_created	15 March 2008
/// \date_revised	9 April 2008
/// \copyright		See below for original copyright notice for original cc source code.
///                 Modified and adapted 2008 by permission for inclusion in the WX version 
///                 by Bruce Waters, Bill Martin, SIL International.
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the CCCModule class. 
/// The CCCModule class encapsulates the traditional SIL Consistent Changes functionality
/// in a single C++ class. It only captures the CC functions which load a .cct table and
/// process consistent changes defined by that table on a buffer.
/// \derivation		The CCCModule class is not a derived class.
/////////////////////////////////////////////////////////////////////////////
// * PROPRIETARY RIGHTS NOTICE:  All rights reserved.  This material contains
// *		the valuable properties of Jungle Aviation and Radio Service, Inc.
// *		of Waxhaw, North Carolina, United States of America (JAARS)
// *		embodying substantial creative efforts and confidential information,
// *		ideas and expressions, no part of which may be reproduced or transmitted
// *		in any form or by any means, electronic, mechanical, or otherwise,
// *		including photocopying and recording or in connection with any
// *		information storage or retrieval system without the permission in
// *		writing from JAARS.
// *
// * COPYRIGHT NOTICE:  Copyright <C> 1980, 1983, 1987, 1988, 1991, 1993,
// *    1994, 1995, 1996,1997, 1998
// *		an unpublished work by the Summer Institute of Linguistics, Inc.
/////////////////////////////////////////////////////////////////////////////
// Notice/Disclaimer:
// My main goal in creating this CCCModule was to encapsulate the Consistent
// Changes functionality for use in the wxWidgets based cross-platform 
// versions of Adapt It so that the same code base could build versions of
// Adapt It for Windows, Linux and the Macintosh, without resort to creating 
// separate stand alone .dll or .so dynamic libraries. This CCCModule can 
// be plugged in to almost any other wxWidgets based application without need 
// for further changes.
// 
// I have tried to preserve all the original cc algorithms as is - at least
// I have tried to avoid accidentally messing with them. The most significant 
// changes I've made relate to file i/o. A lot could still be done
// to bring the code up to more modern standards, avoid use of fixed buffers,
// remove deprecated library functions, etc. The reader will also note that 
// I have not attempted to revise any of the original authors' comments that 
// document the various functions taken from the various Consistent Changes 
// source files. Hence, many of them refer to parameters or variables that 
// are no longer part of the functions. Most variables that were declared 
// in global scope as static variables in various source files are no longer 
// declared as static. To make the code easier to follow, I've also eliminated
// most of the numerous conditional defines. -whm.
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "CCModule.h"
#endif

// For compilers that support precompilation, includes "wx.h".
#include <wx/wxprec.h>

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
// Include your minimal set of headers here, or wx.h
#include <wx/wx.h>
#endif

// other includes
#include <wx/docview.h> // needed for classes that reference wxView or wxDocument
#include <wx/file.h>
#include <wx/filename.h>
#include "CCModule.h"

wxString wxNullStr = _T("");
/* Binary read/write parameters */
const wxString BINREAD = _T("rb");
const wxString BINWRITE = _T("wb");

wxString err_math[4] =  { _T("non-number"),
                              _T("number greater than 2,000,000,000"),
                              _T("overflow"),
                              _T("divide by zero") };
char *zero = "an empty store";


struct errorstruct
{
    wxString errmsg; // this is the error/warning message to be put out
    enum ccMsgFormatType msgFormatType;   // this denotes the type or format of the message
    enum ccMsgType msgWarnOrError;   // denotes if the message is warning or error message
};

struct errorstruct errortable[] =
    {
		// whm Note: I've left all CC errors enclosed within _T() so they won't be localized.
		// Most CC error messages are so cryptic that they would end up being mostly just
		// transliterations and as such most non-English speakers wouldn't make sense of 
		// them anyway.
        /*   0 */  { _T("?CC-E-Backed up too far\n"), MSG_noparms, ERROR_MESSAGE},
        /*   1 */  { _T("CC-Warning caseless ignored, incompatible with doublebyte\n\n"),
                     MSG_noparms, WARNING_MESSAGE },
        /*   2 */  { _T("Warning, group %s excluded but not active\n"),
                     MSG_S, WARNING_MESSAGE },
        /*   3 */  { _T("Value %ld in 'back val(store)' inappropriate.\n"),
                     MSG_LD, WARNING_MESSAGE},
        /*   4 */  { _T("Storage overflow of store %s\n"), MSG_S, WARNING_MESSAGE },
        /*   5 */  { _T("?CC-E-Use of more than %ld groups\n"), MSG_LD, ERROR_MESSAGE },
        /*   6 */  { _T("contents of store %s unchanged\n"), MSG_S, WARNING_MESSAGE },
        /*   7 */  { _T("100%% complete\n"), MSG_noparms, WARNING_MESSAGE },
        /*   8 */  { _T("Warning, store() content stack overflow.  Current storage context not saved.\n"),
                     MSG_noparms, WARNING_MESSAGE },
        /*   9 */  { _T("Warning, store() content stack underflow.  Current storage context not restored.\n"),
                     MSG_noparms, WARNING_MESSAGE },
        /*  10 */  { _T("Warning, value %ld in 'back val(store)' construct larger than the size of the back buffer. Executing a 'back(1)' command\n"),
                     MSG_LD, WARNING_MESSAGE },
        /*  11 */  { _T("?CC-E-Backed too far storing\n"), MSG_noparms,
                     ERROR_MESSAGE },
        /*  12 */  { _T("Warning, cannot decr new variable, it is not decremented\n"),
                     MSG_noparms, WARNING_MESSAGE },
        /*  13 */  { _T("Warning, cannot decr variable that is zero, it is not decremented\n"),
                     MSG_noparms, WARNING_MESSAGE },
        /*  14 */  { _T("Warning, doublebyte settings inconsistent.  Verify it is specified correctly in begin section\n"),
                     MSG_noparms, WARNING_MESSAGE },
        /*  15 */  { _T("Output file full, processing aborted\n"), MSG_noparms,
                     ERROR_MESSAGE },
        /*  16 */  { _T("Disk with output file full.  File is only partly complete!\n"), MSG_noparms,
                     WARNING_MESSAGE },
        /*  17 */  { _T("Warning, doublebyte output no longer matches doublebyte criteria\n"),
                     MSG_noparms, WARNING_MESSAGE },
        /*  18 */  { _T("Warning, doublebyte command has more than two arguments, others ignored\n"),
                     MSG_noparms, WARNING_MESSAGE },
        /*  19 */  { _T("Arithmetic: %s in group %s\n"), MSG_S_S, ERROR_MESSAGE },
        /*  20 */  { _T("%s %c %s\n"), MSG_S_C_S, WARNING_MESSAGE },
        /*  21 */  { _T("Input and output files must be different!\n"),
                     MSG_noparms, ERROR_MESSAGE },
        /*  22 */  { _T("Wildcard mismatch between input \"%s\" and output \"%s\"\n"),
                     MSG_S_S, ERROR_MESSAGE },
        /*  23 */  { _T("?CC-WARNING: defaulting to empty group %s\n"),
                     MSG_S, WARNING_MESSAGE },
        /*  24 */  { _T("Input filename \"%s\" too long for available characters in output filename \"%s\". No processing occurred\n"),
                     MSG_S_S, ERROR_MESSAGE },
        /*  25 */  { _T("?CC-F-Defaulting to non-existent group 1\n"),
                     MSG_noparms, ERROR_MESSAGE },
        /*  26 */  { _T("?CC-E-No definition for do(%s)\n"), MSG_S,ERROR_MESSAGE },
        /*  27 */  { _T("%s does not exist in specified path.\n"),
                     MSG_S, ERROR_MESSAGE },
        /*  28 */  { _T("Total number of files must not exceed %ld.\n"),
                     MSG_LD, ERROR_MESSAGE },
        /*  29 */  { _T("Memory allocation error. Exiting program.\n"),
                     MSG_noparms, ERROR_MESSAGE },
        /*  30 */  { _T("Unable to do changes.\n"), MSG_noparms, ERROR_MESSAGE },
        /*  31 */  { _T("CC fatal error; aborting host application\n"),
                     MSG_noparms, ERROR_MESSAGE },
        /*  32 */  { _T("WARNING: Store %s used but never stored into\n"),
                     MSG_S, WARNING_MESSAGE },
        /*  33 */  { _T("WARNING: Switch %s tested but never set or cleared\n"),
                     MSG_S, WARNING_MESSAGE },
        /*  34 */  { _T("WARNING: use(%s) encountered, but group never defined\n"),
                     MSG_S, WARNING_MESSAGE },
        /*  35 */  { _T("WARNING: do(%s) used but never defined\n"),
                     MSG_S, WARNING_MESSAGE },
        /*  36 */  { _T("Not enough memory\n  (your font file is probably too big)\n"),
                     MSG_noparms, ERROR_MESSAGE },
        /*  37 */  { _T("Only %ld bytes available to be allocated.  More is needed!\n"),
                     MSG_LD, WARNING_MESSAGE },
        /*  38 */  { _T("?CC-F-Too many changes, limit is %ld\n"), MSG_LD, ERROR_MESSAGE },
        /*  39 */  { _T("?CC-F-Group %s multiply defined\n"), MSG_S,ERROR_MESSAGE },
        /*  40 */  { _T("?CC-W-Font section of table is ignored in CC\n"),
                     MSG_noparms, WARNING_MESSAGE },
        /*  41 */  { _T("?CC-E-Group command not in front of change\n"),
                     MSG_noparms, ERROR_MESSAGE },
        /*  42 */  { _T("?CC-E-Do nested deeper than %ld\n"), MSG_LD, ERROR_MESSAGE },
        /*  43 */  { _T("?CC-E-Fwd too many\n"), MSG_noparms, ERROR_MESSAGE },
        /*  44 */  { _T("?CC-E-Omit too many\n"), MSG_noparms, ERROR_MESSAGE },
        /*  45 */  { _T("?CC-E-\'caseless\' command not in begin section\n"),
                     MSG_noparms, ERROR_MESSAGE },
        /*  46 */  { _T("?CC-E-\'binary\' command not in begin section\n"),
                     MSG_noparms, ERROR_MESSAGE },
        /*  47 */  { _T("?CC-E-\'doublebyte\' command not in begin section\n"),
                     MSG_noparms, ERROR_MESSAGE },
        /*  48 */  { _T("?CC-E-\'unsorted\' command not in begin section\n"),
                     MSG_noparms, ERROR_MESSAGE },
        /*  49 */  { _T("?CC-E-Illegal use of command \'%s\' after >\n"),
                     MSG_S, ERROR_MESSAGE },
        /*  50 */  { _T("FATAL ERROR!\nexcl command in group %s removes all active groups\n"),
                     MSG_S, ERROR_MESSAGE },
        /*  51 */  { _T("Too many prec()'s in succession, using only the first %ld"),
                     MSG_LD, WARNING_MESSAGE },
        /*  52 */  { _T("?CC-E-Illegal use of command \'%s\' before >\n"),
                     MSG_S, ERROR_MESSAGE },
        /*  53 */  { _T("%s\n?CC-F-%s\n"), MSG_S_S, ERROR_MESSAGE },
        /*  54 */  { _T("Unrecognized option %s will be ignored.\n"),
                     MSG_S, WARNING_MESSAGE },
        /*  55 */  { _T("Change table %s not found.\n"),
                     MSG_S, ERROR_MESSAGE },
        /*  56 */  { _T("?CC-E-Table too large.\n"), MSG_noparms, ERROR_MESSAGE },
        /*  57 */  { _T("There were errors in the change table.\nCorrect the errors and rerun.\n"),
                     MSG_noparms, ERROR_MESSAGE },
        /*  58 */  { _T("Unable to open %s\n"),
                     MSG_S, ERROR_MESSAGE },
        /*  59 */  { _T("Table file overflow.\n"),
                     MSG_noparms, ERROR_MESSAGE },
        /*  60 */  { _T("Could not reallocate aux output buffer.  Need more memory!\n"),
                     MSG_noparms, ERROR_MESSAGE },
        /*  61 */  { _T("Aux output buffer allocation failed.  Need more memory!\n"),
                     MSG_noparms, ERROR_MESSAGE },
        /*  62 */  { _T("Auxiliary output buffer GlobalLock Failed!\n"),
                     MSG_noparms, ERROR_MESSAGE },
        /*  63 */  { _T("CC save state GlobalAlloc failed.  Probably need more memory!\n"),
                     MSG_noparms, ERROR_MESSAGE },
        /*  64 */  { _T("CC save state GlobalLock failed!\n"),
                     MSG_noparms, ERROR_MESSAGE },
        /*  65 */  { _T("CC save state GlobalUnlock failed in unlocking %s!\n"),
                     MSG_S, ERROR_MESSAGE },
        /*  66 */  { _T("CC restore state was passed NULL handle.  Call CC with valid handle!\n"),
                     MSG_noparms, ERROR_MESSAGE },
        /*  67 */  { _T("CC restore state GlobalLock failed!\n"),
                     MSG_noparms, ERROR_MESSAGE },
        /*  68 */  { _T("CC restore state GlobalUnlock failed!\n"),
                     MSG_noparms, ERROR_MESSAGE },
        /*  69 */  { _T("CCLoadTable was passed invalid name of CC change table.  Pass in valid name!\n"),
                     MSG_noparms, ERROR_MESSAGE },
        /*  70 */  { _T("CCLoadTable encountered CC error.  Fix problem and rerun!\n"),
                     MSG_noparms, ERROR_MESSAGE },
        /*  71 */  { _T("CCReinitializeTable was passed NULL handle.  Call CC with valid handle!\n"),
                     MSG_noparms, ERROR_MESSAGE },
        /*  72 */  { _T("CCReinitializeTable encountered CC error.  Fix problem and rerun!\n"),
                     MSG_noparms, ERROR_MESSAGE },
        /*  73 */  { _T("CCUnloadTable was passed NULL handle.  Call CC with valid handle!\n"),
                     MSG_noparms, ERROR_MESSAGE },
        /*  74 */  { _T("CCUnloadTable GlobalUnlock failed!\n"),
                     MSG_noparms, ERROR_MESSAGE },
        /*  75 */  { _T("CCUnloadTable GlobalFree failed!\n"),
                     MSG_noparms, ERROR_MESSAGE },
        /*  76 */  { _T("CCSetErrorCallBack was passed NULL handle.  Call CC with valid handle!\n"),
                     MSG_noparms, ERROR_MESSAGE },
        /*  77 */  { _T("CCSetUpInputFilter was passed NULL handle.  Call CC with valid handle!\n"),
                     MSG_noparms, ERROR_MESSAGE },
        /*  78 */  { _T("CCSetUpInputFilter GlobalAlloc failed.  Need more memory!\n"),
                     MSG_noparms, ERROR_MESSAGE },
        /*  79 */  { _T("CCSetUpInputFilter GlobalLock failed!\n"),
                     MSG_noparms, ERROR_MESSAGE },
        /*  80 */  { _T("CCGetBuffer was passed NULL handle.  Call CC with valid handle!\n"),
                     MSG_noparms, ERROR_MESSAGE },
        /*  81 */  { _T("CCGetBuffer encountered CC error.  Fix problem and rerun!\n"),
                     MSG_noparms, ERROR_MESSAGE },
        /*  82 */  { _T("CCProcessBuffer was passed NULL handle.  Call CC with valid handle!\n"),
                     MSG_noparms, ERROR_MESSAGE },
        /*  83 */  { _T("CCProcessBuffer was passed output buffer of only %ld bytes, more needed!\nRerun with larger output buffer!\n"),
                     MSG_LD, ERROR_MESSAGE },
        /*  84 */  { _T("CCProcessBuffer encountered CC error.  Fix problem and rerun!\n"),
                     MSG_noparms, ERROR_MESSAGE },
        /*  85 */  { _T("CCGetBuffer has NULL callback address!\nMust have successful call to CCSetUpInputFilter first!\n"),
                     MSG_noparms, ERROR_MESSAGE },
        /*  86 */  { _T("CCPutBuffer has NULL callback address!\nMust have successful call to CCSetUpOutputFilter first!\n"),
                     MSG_noparms, ERROR_MESSAGE },
        /*  87 */  { _T("CCSetUpOutputFilter was passed NULL handle.  Call CC with valid handle!\n"),
                     MSG_noparms, ERROR_MESSAGE },
        /*  88 */  { _T("CCSetUpOutputFilter GlobalAlloc failed.  Need more memory!\n"),
                     MSG_noparms, ERROR_MESSAGE },
        /*  89 */  { _T("CCSetUpOutputFilter GlobalLock failed!\n"),
                     MSG_noparms, ERROR_MESSAGE },
        /*  90 */  { _T("CCPutBuffer was passed NULL handle.  Call CC with valid handle!\n"),
                     MSG_noparms, ERROR_MESSAGE },
        /*  91 */  { _T("CCPutBuffer encountered CC error.  Fix problem and rerun!\n"),
                     MSG_noparms, ERROR_MESSAGE },
        /*  92 */  { _T("CCPutBuffer encountered error in output callback function.\nFix problem and rerun!\n"),
                     MSG_noparms, ERROR_MESSAGE },
        /*  93 */  { _T("CCGetBuffer called with no output data requested.\nReturning to caller after doing nothing!\n"),
                     MSG_noparms, WARNING_MESSAGE },
        /*  94 */  { _T("CCProcessBuffer called with no input data.\nReturning to caller after doing nothing!\n"),
                     MSG_noparms, WARNING_MESSAGE },
        /*  95 */  { _T("CCProcessFile input routine FillFunction had DLL failure getting segment address to data for calling task!\n"),
                     MSG_noparms, ERROR_MESSAGE },
        /*  96 */  { _T("CCPutBuffer was passed NULL input buffer pointer, but input data length greater than zero!.\nPlease fix problem and rerun!\n"),
                     MSG_noparms, ERROR_MESSAGE },
        /*  97 */  { _T("CCProcessBuffer call to CCReinitializeTable unsuccessful!\n"),
                     MSG_noparms, ERROR_MESSAGE },
        /*  98 */  { _T("CCLoadTable was passed NULL pointer to handle.  Pass in valid value!\n"),
                     MSG_noparms, ERROR_MESSAGE },
        /*  99 */  { _T("CC-Warning binary keyword ignored, incompatible with doublebyte\n\n"),
                     MSG_noparms, WARNING_MESSAGE },
        /* 100 */  { _T("CCSetUpInputFilter was passed NULL function pointer.  Call CC with valid value!\n"),
                     MSG_noparms, ERROR_MESSAGE },
        /* 101 */  { _T("CCGetBuffer was passed NULL output buffer pointer.  Call CC with valid value!\n"),
                     MSG_noparms, ERROR_MESSAGE },
        /* 102 */  { _T("CCProcessBuffer was passed NULL output buffer length pointer.  Call CC with valid value!\n"),
                     MSG_noparms, ERROR_MESSAGE },
        /* 103 */  { _T("CCProcessBuffer was passed NULL input buffer pointer.  Call CC with valid value!\n"),
                     MSG_noparms, ERROR_MESSAGE },
        /* 104 */  { _T("CCProcessBuffer was passed NULL output buffer pointer.  Call CC with valid value!\n"),
                     MSG_noparms, ERROR_MESSAGE },
        /* 105 */  { _T("CCMultiProcessBuffer was passed NULL handle.  Call CC with valid handle!\n"),
                     MSG_noparms, ERROR_MESSAGE },
        /* 106 */  { _T("CCMultiProcessBuffer was passed NULL input buffer pointer.  Call CC with valid value!\n"),
                     MSG_noparms, ERROR_MESSAGE },
        /* 107 */  { _T("CCMultiProcessBuffer was passed NULL output buffer length pointer.  Call CC with valid value!\n"),
                     MSG_noparms, ERROR_MESSAGE },
        /* 108 */  { _T("CCMultiProcessBuffer called asking for no output data.\nReturning to caller after doing nothing!\n"),
                     MSG_noparms, WARNING_MESSAGE },
        /* 109 */  { _T("CCMultiProcessBuffer was passed NULL output buffer pointer.  Call CC with valid value!\n"),
                     MSG_noparms, ERROR_MESSAGE },
        /* 110 */  { _T("CCMultiProcessBuffer encountered CC error.  Fix problem and rerun!\n"),
                     MSG_noparms, ERROR_MESSAGE },
        /* 111 */  { _T("CCMultiProcessBuffer called after it passed back completion return code!\n  Must first call CCReinitializeTable before calling again!\n"),
                     MSG_noparms, ERROR_MESSAGE },
        /* 112 */  { _T("CCGetBuffer called after it passed back completion return code!\n  Must first call CCReinitializeTable before calling again!\n"),
                     MSG_noparms, ERROR_MESSAGE },
        /* 113 */  { _T("CCMultiProcessBuffer passed input data.\nReturn value from last call indicates it must be passed no input data this time!\n"),
                     MSG_noparms, ERROR_MESSAGE },
        /* 114 */  { _T("CCLoadTable could not register the DLL task!\n"),
                     MSG_noparms, ERROR_MESSAGE },
        /* 115 */  { _T("CCReinitializeTable had DLL failure getting segment address to data for calling task!\n"),
                     MSG_noparms, ERROR_MESSAGE },
        /* 116 */  { _T("CCUnloadTable had DLL failure getting segment address to data for calling task!\n"),
                     MSG_noparms, ERROR_MESSAGE },
        /* 117 */  { _T("CCSetErrorCallBack had DLL failure getting segment address to data for calling task!\n"),
                     MSG_noparms, ERROR_MESSAGE },
        /* 118 */  { _T("CCSetUpInputFilter had DLL failure getting segment address to data for calling task!\n"),
                     MSG_noparms, ERROR_MESSAGE },
        /* 119 */  { _T("CCGetBuffer had DLL failure getting segment address to data for calling task!\n"),
                     MSG_noparms, ERROR_MESSAGE },
        /* 120 */  { _T("CCProcessBuffer had DLL failure getting segment address to data for calling task!\n"),
                     MSG_noparms, ERROR_MESSAGE },
        /* 121 */  { _T("CCMulti{rocessBuffer had DLL failure getting segment address to data for calling task!\n"),
                     MSG_noparms, ERROR_MESSAGE },
        /* 122 */  { _T("CCSetUpOutputFilter had DLL failure getting segment address to data for calling task!\n"),
                     MSG_noparms, ERROR_MESSAGE },
        /* 123 */  { _T("CCPutBuffer had DLL failure getting segment address to data for calling task!\n"),
                     MSG_noparms, ERROR_MESSAGE },
        /* 124 */  { _T("CCLoadTable called with NULL handle (hinstance) for calling task!\n"),
                     MSG_noparms, ERROR_MESSAGE },
        /* 125 */  { _T("CCProcessFile was passed NULL handle.  Call CC with valid handle!\n"),
                     MSG_noparms, ERROR_MESSAGE },
        /* 126 */  { _T("CCProcessFile was passed NULL input file pointer! Fix problem and rerun!\n"),
                     MSG_noparms, ERROR_MESSAGE },
        /* 127 */  { _T("CCProcessFile was passed NULL output file pointer! Fix problem and rerun!\n"),
                     MSG_noparms, ERROR_MESSAGE },
        /* 128 */  { _T("CCProcessFile call to CCSetUpInputFilter failed!\n"),
                     MSG_noparms, ERROR_MESSAGE },
        /* 129 */  { _T("CCProcessFile could not open input file it was passed!\nInput file: %s\nFix problem and rerun!\n"),
                     MSG_S, ERROR_MESSAGE },
        /* 130 */  { _T("CCProcessFile could not open output file it was passed!\nOutput file: %s\nFix problem and rerun!\n"),
                     MSG_S, ERROR_MESSAGE },
        /* 131 */  { _T("CCProcessFile encountered CC error.  Fix problem and rerun!\n"),
                     MSG_noparms, ERROR_MESSAGE },
        /* 132 */  { _T("CCProcessFile had DLL failure getting segment address to data for the calling task!\n"),
                     MSG_noparms, ERROR_MESSAGE },
        /* 133 */  { _T("CCFlush was passed NULL handle.  Call CC with valid handle!\n"),
                     MSG_noparms, ERROR_MESSAGE },
        /* 134 */  { _T("CCLoadTable had DLL failure getting segment address to data for calling task!\n"),
                     MSG_noparms, ERROR_MESSAGE },
        /* 135 */  { _T("CCLoadTable was passed a change table filename that is too long!\nPlease call it with a shorter change table file path name.\n"),
                     MSG_S, ERROR_MESSAGE },
        /* 136 */  { _T("?CC-W-Change Table is empty: No changes will be made to file.\n"),
                     MSG_noparms, WARNING_MESSAGE },
        /* 137 */  { _T("CCProcessBuffer was passed an output buffer length of 0.\nSet the output buffer length to the length of the output buffer before calling CCProcessBuffer!\n"),
                     MSG_noparms, ERROR_MESSAGE },
        /* 138 */  { _T("CCSetUTF8Encoding was passed NULL handle.  Call CC with valid handle!\n"),
                     MSG_noparms, ERROR_MESSAGE },
    };

typedef void (*stor_noarg) (char, char, char);
stor_noarg stornoarg,storarg,storoparg,stordbarg;

struct cmstruct cmsrch[] =
    {
        { "nl",		  stornoarg, CARRIAGERETURN, FALSE,			0 },
        { "tab",	  stornoarg, TAB,            FALSE,			0 },
        { "endfile",  stornoarg, ENDFILECMD,	  FALSE,			0 },
        { "define",   storarg,	 DEFINECMD,		  DEFINED,		DEFINE_HEAD },
        { "any",	  storarg,	 ANYCMD,		  REFERENCED,	STORE_HEAD },
        { "prec",	  storarg,	 PRECCMD,		  REFERENCED,	STORE_HEAD },
        { "fol",	  storarg,	 FOLCMD,		  REFERENCED,	STORE_HEAD },
        { "wd",		  storarg,	 WDCMD,			  REFERENCED,	STORE_HEAD },
        { "anyu",	  storarg,	 ANYUCMD,		  REFERENCED,	STORE_HEAD },
        { "precu",	  storarg,	 PRECUCMD,		  REFERENCED,	STORE_HEAD },
        { "folu",	  storarg,	 FOLUCMD,		  REFERENCED,	STORE_HEAD },
        { "wdu",	  storarg,	 WDUCMD,		  REFERENCED,	STORE_HEAD },
        { "cont",	  storarg,	 CONTCMD,		  REFERENCED,	STORE_HEAD },
        { NULL,		  NULL,		0,				  FALSE,			0 }
    };

/* Replacement Arguments (right side) */

struct cmstruct cmrepl[] =
    {
        { "nl",		  stornoarg, CARRIAGERETURN, FALSE,			0 },
        { "tab",		  stornoarg, TAB,            FALSE,			0 },
        { "endfile",  stornoarg, ENDFILECMD,	  FALSE,			0 },
        { "cont",	  storarg,	 CONTCMD,		  REFERENCED,	STORE_HEAD },
        { "if",		  storarg,	 IFCMD,			  REFERENCED,	SWITCH_HEAD },
        { "ifn",		  storarg,	 IFNCMD,			  REFERENCED,	SWITCH_HEAD },
        { "else",	  stornoarg, ELSECMD,		  FALSE,			0 },
        { "endif",	  stornoarg, ENDIFCMD,		  FALSE,			0 },
        { "set",		  storarg,	 SETCMD,			  DEFINED,		SWITCH_HEAD },
        { "clear",	  storarg,	 CLEARCMD,		  DEFINED,		SWITCH_HEAD },
        { "begin",	  stornoarg, BEGINCMD,		  FALSE,			0 },
        { "endstore", stornoarg, ENDSTORECMD,	  FALSE,			0 },
        { "store",	  storarg,	 STORCMD,		  DEFINED,		STORE_HEAD },
        { "append",   storarg,	 APPENDCMD,		  DEFINED,		STORE_HEAD },
        { "out",		  storarg,	 OUTCMD,			  REFERENCED,	STORE_HEAD },
        { "outs",	  storarg,	 OUTSCMD,		  REFERENCED,	STORE_HEAD },
        { "dup",		  stornoarg, DUPCMD,			  FALSE,			0 },
        { "back",	  storoparg, BACKCMD,		  FALSE,			0 },
        { "next",	  stornoarg, NEXTCMD,		  FALSE,			0 },
        { "ifeq",	  storarg,	 IFEQCMD,		  REFERENCED,	STORE_HEAD },
        { "ifneq",	  storarg,	 IFNEQCMD,		  REFERENCED,	STORE_HEAD },
        { "ifgt",	  storarg,	 IFGTCMD,		  REFERENCED,	STORE_HEAD },
        { "ifngt",    storarg,   IFNGTCMD,       REFERENCED,  STORE_HEAD },
        { "iflt",     storarg,   IFLTCMD,        REFERENCED,  STORE_HEAD },
        { "ifnlt",    storarg,   IFNLTCMD,       REFERENCED,  STORE_HEAD },
        { "end",		  stornoarg, ENDCMD,			  FALSE,			0 },
        { "repeat",   stornoarg, REPEATCMD,		  FALSE,			0 },
        { "group",	  storarg,	 GROUPCMD,		  DEFINED,		GROUP_HEAD },
        { "do",		  storarg,	 DOCMD,			  REFERENCED,	DEFINE_HEAD },
        { "incl",	  storarg,	 INCLCMD,		  REFERENCED,	GROUP_HEAD },
        { "excl",	  storarg,	 EXCLCMD,		  REFERENCED,	GROUP_HEAD },
        { "fwd",		  storoparg, FWDCMD,			  FALSE,			0 },
        { "omit",	  storoparg, OMITCMD,		  FALSE,			0 },
        { "incr",	  storarg,	 INCRCMD,		  DEFINED,		STORE_HEAD },
        { "decr",     storarg,   DECRCMD,        DEFINED,     STORE_HEAD },
        { "add",		  storarg,	 ADDCMD,			  DEFINED,		STORE_HEAD },
        { "sub",		  storarg,	 SUBCMD,			  DEFINED,		STORE_HEAD },
        { "mul",		  storarg,	 MULCMD,			  DEFINED,		STORE_HEAD },
        { "div",		  storarg,	 DIVCMD,			  DEFINED,		STORE_HEAD },
        { "mod",		  storarg,	 MODCMD,			  DEFINED,		STORE_HEAD },
        { "write",	  stornoarg, WRITCMD,		  FALSE,			0 },
        { "wrstore",  storarg,	 WRSTORECMD,	  REFERENCED,	STORE_HEAD },
        { "read",	  stornoarg, READCMD,		  FALSE,			0 },
        { "caseless", stornoarg, CASECMD,		  FALSE,			0 },
        { "binary",   stornoarg, BINCMD,         FALSE,       0 },
        { "doublebyte",stordbarg,DOUBLECMD,      FALSE,       0 },
        { "unsorted",stornoarg,  UNSORTCMD,      FALSE,       0 },
        { "ifsubset", storarg,	 IFSUBSETCMD,	  REFERENCED,	STORE_HEAD },
        { "len",		  storarg,	 LENCMD,			  REFERENCED,	STORE_HEAD },
        { "utf8",     stornoarg, UTF8CMD,		  FALSE,			0 },
        { "backu",	  storoparg, BACKUCMD,		  FALSE,			0 },
        { "fwdu",	  storoparg, FWDUCMD,		  FALSE,			0 },
        { "omitu",	  storoparg, OMITUCMD,		  FALSE,			0 },
        { NULL,		  NULL,		0,				  FALSE,			0 }
    };

char bytesFromUTF8[256] = {
                              0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                              0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                              0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                              0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                              0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                              0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                              1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
                              2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 3,3,3,3,3,3,3,3,4,4,4,4,5,5,5,5};

char *names[4] =
    {
        "Stores:  ",
        "Switches:",
        "Groups:  ",
        "Defines: "
    };

CCCModule::CCCModule() // constructor
{
	
}

CCCModule::~CCCModule() // destructor
{
	CCUnloadTable();
}

/****************************************************************************/
void  CCCModule::CallCCMainPart ()
/****************************************************************************/
/*                                                                          */
/* This is the main (and only) entry point for when _WINDLL is defined      */
/* This directs the overall flow for CC for when _WINDLL is defined.        */
/*                                                                          */
/*                                                                          */
/* This routine is where the CC DLL Interfaces call the main part of CC     */
/* here when they want to perform the CC functions.  This is called         */
/* here whether the user wants to treat CC (via the DLL) as a front end     */
/* process or a back end process, or whether they want to call it in a      */
/* "Visual Basic style" with just passing data in and out relatively        */
/* simply without callbacks for processing more data.                       */
/*                                                                          */
/*                                                                          */
/*                                                                          */
/* Inputs:              Many global variables                               */
/*                                                                          */
/* Outputs:             Many global variables                               */
/*                                                                          */
/* Results:             CC operations performed                             */
/*                                                                          */
/****************************************************************************/
{

    // this is where we do the DLL processing for the first time DLL is called

    if (bFirstDLLCall == TRUE)
    {
        bFirstDLLCall = FALSE;  // reset for the next time through
        begin_found = FALSE;    // we have not found begin statement yet
        nInSoFar = 0;           // have not processed any data yet
        bEndofInputData = FALSE;// indicate that we still have data to process
        bSavedDblChar = FALSE;  // denote do not have a saved output character
        bSavedInChar = FALSE;   // denote do not have a saved input character

        bAuxBufUsed = FALSE;    // denote aux buffer not allocated or used yet
        nCharsInAuxBuf = 0;     // mark that no space in buffer used as yet
        iAuxBuf= 0;
        hAuxBuf = NULL;         // mark special aux buffer handle as not used
        nAuxLength = 0;         // initialize special aux buffer length to zero
        bBeginExecuted= FALSE;
        eof_written = FALSE;
    }
    else
    {
        // if we one saved char from last time (second half of doublebyte)
        // then output that char first before we go on to any new data
        if ( bSavedDblChar)
        {
            out_char_buf(dblSavedChar);       // output character into buffer
            bSavedDblChar = FALSE;            // turn off flag for next time
        }

        // If we have saved off data before in special auxiliary area, then
        // now go through that and output that data first before getting more
        if (bAuxBufUsed)
        {
            while ((!bOutputBufferFull) && (iAuxBuf < nCharsInAuxBuf))
            {
                out_char_buf( *lpAuxNextToOutput++);
                iAuxBuf++;
            }
            // if we have exhausted this data, then set things to indicate that
            if (iAuxBuf == nCharsInAuxBuf)
            {
                lpAuxNextToOutput = lpAuxBufStart;
                lpAuxOutBuf = lpAuxBufStart;
                bAuxBufUsed = FALSE;
                nCharsInAuxBuf = 0;
                iAuxBuf= 0;
            }
        }
    }

    if (!bOutputBufferFull)
    {
        if(!bBeginExecuted)
            fillmatch();

        if(bBeginExecuted)
            execcc();
    }
}  /* End--MAIN (for Windows DLL) */


int CCCModule::CCLoadTable(wxString lpszCCTableFile)
{
    int rc;
    rc= CCLoadTableCore(lpszCCTableFile);
    return rc;
}

int CCCModule::CCLoadTableCore(wxString lpszCCTableFile)
/**************************************************************************/
/*
 * Description:
 *                This DLL interface is passed a string with the name of 
 *                the CC table to be used.  It performs some CC processing
 *                to start things up with that CC table, and then it returns
 *                a handle for the area with the global variables after that.
 *                This needs to be the first CC DLL interface called.  This
 *                routine also registers the calling task.
 *
 *
 * Input values:
 *                *lpszCCTableFile has the name of the CC Table to be used.
 *
 *                hParent has the hinstance (handle) of the invoking task
 *
 *
 * Output values:
 *                *hpLoadHandle points to the handle for CC variables.
 *
 *
 * Return values:
 *                CC_SUCCESS          If everything is fine, files are to be
 *                                    opened in the usual text mode.
 *
 *                CC_SUCCESS_BINARY   If everything is fine, files are to be
 *                                    opened in binary mode.
 *
 *                non-zero            Indicates an error occurred.
 *
 *
 */
{
    lpszCCTableBuffer= NULL;

    if ( lpszCCTableFile.IsEmpty() )
    {
        Process_msg(69, wxNullStr, 0);
        return(-1);
    }

    // make sure that path is not too long, then copy it into our
    // space so we are not dependent on the user keeping it around
	// whm Note: Now that cctpath is a wxString, it doesn't make sense to check sizeof(cctpath) which
	// will never represent the length of a buffer. I've commented out this error check as it is no
	// longer required, and in any case, would always trip erroneously.
    //if ( lpszCCTableFile.Length() + 1 > sizeof(cctpath) )
    //{
    //    Process_msg(135, wxNullStr, (long unsigned) &lpszCCTableFile);
    //    return(-1);
    //}
    //else
    cctpath = lpszCCTableFile; //strcpy(cctpath, lpszCCTableFile);

    // initialize variables to NULL here that are initialized to NULL
    // in cc.h for the non-DLL case

    tablefile = NULL;
    parsepntr = NULL;
    parse2pntr = NULL;
    tablelimit = NULL;
    tloadpointer = NULL;
    maintablend = NULL;
    storelimit = NULL;
    backbuf = NULL;
    backinptr = NULL;
    backoutptr = NULL;
    dupptr = NULL;
    cngpointers = NULL;
    cngpend = NULL;
    match = NULL;
    matchpntr = NULL;
    matchpntrend = NULL;
    executetableptr= NULL;
    lpOutBuf = NULL;
    lpOutBufStart = NULL;
    lpInBuf = NULL;
    lpInBufStart = NULL;
    lDLLUserInputCBData = 0;
    lDLLUserOutputCBData = 0;
    lDLLUserErrorCBData = 0;
    table = NULL;
    store_area = NULL;
    binary_mode = FALSE;
    utf8encoding = FALSE;
    doublebyte_mode = FALSE;

    errors = FALSE;            // no CC errors yet
    bProcessingDone = FALSE;   // have not passed user completion retcode
    bPassNoData = FALSE;       // do not want user to pass no data in
    bPassedAllData = FALSE;    // have not yet received all input data
    hInputBuffer = 0;          // have not yet allocated input buffer area
    hOutputBuffer = 0;         // have not yet allocated output buffer
    hAuxBuf = NULL;            // have not yet allocated aux buffer area
    nInBuf = 0;                  // set input buffer length
    nInSoFar= 0;
    nullname[0] = '\0';        // initialize some variables here
    noiseless = TRUE;
    begin_found = FALSE;

    bMoreNextTime = FALSE;     // denote we have no data waiting to be processed
    bFirstDLLCall = TRUE;      // initialize to the first call
    bBeginExecuted = FALSE;

    tblarg = cctpath; //tblarg = &cctpath[0];      // set up input for next calls

    hTableLineIndexes= malloc(MAXTABLELINES * sizeof(unsigned));

    if (hTableLineIndexes == NULL)
    {
        Process_msg(78, wxNullStr, 0);
        return(-1);
    }

    TableLineIndexes= (unsigned *)hTableLineIndexes;

    iCurrentLine= 0;

    tblcreate();               // allocate memory for CC table

    compilecc();               // get cc table and compile it

    hTableLineIndexes= realloc((void *)hTableLineIndexes, (iCurrentLine + 1) * sizeof(unsigned));

    if (hTableLineIndexes == NULL)
    {
        Process_msg(78, wxNullStr, 0);
        return(-1);
    }

    TableLineIndexes= (unsigned *)hTableLineIndexes;

    if (!errors)
        startcc();

    if ( !errors )
        SaveState();  // alloc space and save global variables
    else
    {
        CleanUp();
        Process_msg(70, wxNullStr, 0);
        return(CC_SYNTAX_ERROR);
    }

    if ( binary_mode )
        return(CC_SUCCESS_BINARY);     // return success, binary mode
    else
        return(CC_SUCCESS);            // return success, text mode

}  // End - CCLoadTableCore

/**************************************************************************/
int CCCModule::CCReinitializeTable()
/**************************************************************************/
/*
 * Description:
 *               This DLL routine is called to reinitialize CC without   
 *               starting over totally from scratch.  E.g. use this with
 *               new data, but using same change table.  It uses the global
 *               variables that are in the area pointed to by the handle.
 *
 *
 *
 * Input values:
 *               hReHandle is the handle pointing to the global variables.
 *
 *
 *
 * Output values:
 *               The global variables referenced by hReHandle are updated.
 *
 *
 *
 *
 * Return values:
 *                0        If everything is fine
 *
 *                non-zero indicates an error occurred
 *
 *
 *
 */
{
    register int i;     // Loop index for initializing storage pointers
    unsigned storemax;

    RestoreState();  // this restores our global variables

    storemax = max_heap() / sizeof(SSINT);

    hAuxBuf = NULL;           // denote have not yet allocated aux buffer area

    bMoreNextTime = FALSE;    // denote have no data waiting to be processed
    bFirstDLLCall = TRUE;     // initialize to the first call
    bBeginExecuted = FALSE;
    bProcessingDone = FALSE;  // denote have not passed user completion rc
    bPassNoData = FALSE;      // denote do not want user to pass no data in
    bSavedInChar = FALSE;     // denote we do not have saved input character
    bSavedDblChar = FALSE;    // denote we do not have saved output character
    lpOutBuf = lpOutBufStart; // set to NULL or start of buffer as appropriate
    bOutputBufferFull= FALSE;
    nInBuf = 0;                  // set input buffer length
    nInSoFar= 0;

    // This starts a section that is basically a subset of the startcc()
    // routine that is in ccexec.c.  The difference is that we do not free
    // and then just re-malloc the storage area again, we just keep it.

    bytset(switches, FALSE, MAXARG+1);     // Clear all switches

    storemax = max_heap() / sizeof(SSINT);
    if ( store_area != NULL )
        free(store_area);
    store_area = (SSINT *) tblalloc(storemax, (sizeof(SSINT)));
    storelimit = store_area + storemax;

    for ( i = 0; i <= NUMSTORES; i++ )     // Initialize storage pointers
    {
        storebegin[i] = storend[i] = store_area;
        storeact[i] = FALSE;
        storepre[i] = 0;
    }

    curstor = 0;                           // Clear storing and overflow
    storeoverflow = FALSE;
    iStoreStackIndex = 0;
    setcurrent = FALSE;                    // Letterset not current
    eof_written = FALSE;                   // We haven't written EOF yet
    numgroupson = 0;

    if ( groupbeg[1] != 0 )
        groupinclude(1);                    // Start in group 1, if it exist

    backinptr = backoutptr = backbuf;      // Initialize backup buffer
    ssbytset( backbuf, (SSINT)' ',
              BACKBUFMAX);                 // make spaces so debug looks OK

    SaveState();     // save global variables into saved area

    if ( errors )
    {
        Process_msg(72, wxNullStr, 0);
        return(-1);
    }

    return(0);

}  // End - CCReinitializeTable

int CCCModule::CleanUp()
{
    int retcode;
    retcode = 0;

    freeMem();                 // free all allocated memory other than globals

    if (hTableLineIndexes != 0)
    {
        free((void *)hTableLineIndexes);
    }

    if ( hInputBuffer != 0 )
    {
        free((void *)hInputBuffer);    // free space now that we are done with it
	}

    if ( hOutputBuffer != 0 )
    {
        free((void *)hOutputBuffer);    // free space now that we are done with it
	}

    if ( hAuxBuf != 0 )
    {
        free((void *)hAuxBuf);    // free space now that we are done with it
	}
    return(retcode);
}

/************************************************************************/
void CCCModule::Process_msg(short nMsgIndex, wxString errStr, long unsigned lParam)
/************************************************************************/
/*
 *  NOTE: See comments at the start of ccerror.h that describe what
 *        this routine does, and how users would write their own
 *        error checking routine in Windows DLL mode.
 *
 */
{
    int formatType;
	wxString tempStr;
	int MsgIcon;
	if (errortable[nMsgIndex].msgWarnOrError == WARNING_MESSAGE)
		MsgIcon = wxICON_EXCLAMATION | wxOK;
	else
		MsgIcon = wxICON_INFORMATION | wxOK;

    formatType = errortable[nMsgIndex].msgFormatType;
    switch (formatType)
    {
    case MSG_S_S:
		tempStr = tempStr.Format(errortable[nMsgIndex].errmsg,
			((MSG_STRUCT_S_S *) lParam)->string1.c_str(),
			((MSG_STRUCT_S_S *) lParam)->string2.c_str());
		wxMessageBox(tempStr,_("Consistent Changes Error"),MsgIcon);
        break;
    case MSG_S_C_S:
		tempStr = tempStr.Format(errortable[nMsgIndex].errmsg,
                ((MSG_STRUCT_S_C_S *) lParam)->string1.c_str(),
                ((MSG_STRUCT_S_C_S *) lParam)->char1,
                ((MSG_STRUCT_S_C_S *) lParam)->string2.c_str());
		wxMessageBox(tempStr,_T(""),MsgIcon);
        break;
    case MSG_S:
		tempStr = tempStr.Format(errortable[nMsgIndex].errmsg, errStr.c_str()); 
		wxMessageBox(tempStr,_T(""),MsgIcon);
        break;
    case MSG_LD:
		tempStr = tempStr.Format(errortable[nMsgIndex].errmsg, 
                (long signed) lParam);
		wxMessageBox(tempStr,_T(""),MsgIcon);
        break;
    case MSG_noparms:
    default:            // We should never just default actually!
		tempStr = tempStr.Format(errortable[nMsgIndex].errmsg);
		wxMessageBox(tempStr,_T(""),MsgIcon);
        break;
    }
}

/****************************************************************************/
void* CCCModule::tblalloc(unsigned count, unsigned size)			/* Allocate tables */
/****************************************************************************/
//count,	/* # of objects in the requested table */
//size;		/* Length of the objects in bytes */

/* Description -- Try to allocate the requested table.
 *						If unsuccessful
 *							Display an error message.
 *							Exit with exit code = BADEXIT.
 *
 *						Return a pointer to the table.
 *
 * Return values: Return a pointer to requested table.
 *
 * Globals input: none
 *
 * Globals output: none
 *
 * Error conditions:  If malloc fails we will be unable to continue,
 *								so display an informative message and exit
 *								to the operating system.
 *
 * Other functions called: none
 *
 */

{
    void *mp;  /* Pointer to the table (returned by malloc) */
    mp = malloc( (unsigned) (count * size));	/* Try to allocate the table */

    if ( mp == NULL )
    {
        Process_msg(36, wxNullStr, 0);
        bailout(BADEXIT, FALSE);				/*  and bail out		 */
    }
    return(mp);			/* Return a pointer to the table */

} /* End--tblalloc */


/****************************************************************************/
void CCCModule::tblcreate()		 /* Create tables */
/****************************************************************************/

/* Description:
 *						Allocate the tables, using tblalloc (above).
 *
 * Return values: All dynamically allocated tables allocated.
 *
 * Globals input: backbuf -- ring buffer for backup
 *						cngpointers -- pointers to changes in table
 *						match -- match area for searching
 *  (MS only)		tabs -- tabs array
 *
 * Globals output: All of those input, pointing to actual data areas
 *
 * Error conditions: If we encounter an error allocating anything but
 *							  table, tblalloc will exit to the OS for us.
 *
 * Other functions called: tblalloc -- allocate a table
 *
 */

{
    //these should work well for DOS EXE or FUNC versions
    // perhaps we should replace the older code after memory alloc testing
    if (backbuf == NULL)										//7.4.15
        backbuf = (SSINT *) tblalloc(BACKBUFMAX, sizeof(*backbuf));
    if (cngpointers == NULL)
        cngpointers = (tbltype *) tblalloc(MAXCHANGES+1, sizeof(*cngpointers));
    if (match == NULL)
        match = (SSINT *) tblalloc(MAXMATCH, sizeof(*match));
} /* End--tblcreate */

/****************************************************************************/
void CCCModule::groupinclude(register int group)  /* Include another group of changes in search area */
/****************************************************************************/

/* This procedure includes a new group of changes in the list of groups
 *   being searched.
 *
 * Description -- If trying to use too many groups
 *							Display an error message on the screen.
 *							Return.
 *
 *						If the group is already in use
 *							Make it the last group to be searched.
 *						Else
 *							Increment count of groups in use.
 *							Add the new group to the array of groups in use.
 *							Say that the letter set for matching is
 *							  not up to date.
 *
 * Return values: none
 *
 * Globals input: numgroupson -- number of groups currently being used
 *											  for matching
 *						curgroups -- array of group numbers for searching
 *						setcurrent -- boolean: TRUE == letter set for matching
 *																	is up to date
 *
 * Globals output: numgroupson -- if not too many groups being searched
 *												incremented by 1
 *						 curgroups -- if not too many groups being searched and
 *											 new group added as last entry
 *						 setcurrent -- if not too many groups being searched
 *											  set to FALSE
 *
 * Error conditions: if trying to search too many groups
 *							  an error message will be displayed on the screen
 *
 * Other functions called: none
 *
 */

{
    register int i;		/* Loop index for searching curgroups */
    char found;				/* Boolean: TRUE == group is currently active */


    for (i = 1, found = FALSE; i <= numgroupson && !found; i++)
    {										/* See if the group is already active */

        if ( curgroups[i] == group )
        {
            found = TRUE;						/* Group is already active, */
            while (i < numgroupson) 		/*	make it the last group searched */
            {
                curgroups[i] = curgroups[i+1];
                i++;
            }
            curgroups[numgroupson] = group;
            setcurrent = FALSE;
        }
    }

    if ( !found )
    {
        if ( numgroupson == GROUPSATONCE )
        {
            Process_msg(5, wxNullStr, (long unsigned) GROUPSATONCE);
        }
        else
        {
            numgroupson++;						/* Not there, so we need to add it */
            curgroups[numgroupson] = group;

            setcurrent = FALSE;							/* Must recompute letterset */
        }
    }

} /* End--groupinclude */

/****************************************************************************/
void CCCModule::groupexclude(register int group)				  /* Exclude a group of changes */
/****************************************************************************/

/* This procedure excludes a group from the list of groups being searched.
 *
 * Description -- If group is not currently in use.
 *							Display an error message on the screen.
 *							Return.
 *
 *						If the group is not currently active
 *							Give a warning message.
 *						If the group is in use
 *							Delete it from the list.
 *							Decrement the count of groups in use.
 *						Say that the letter set for matching is
 *						  not up to date.
 *
 * Return values: none
 *
 * Globals input: numgroupson -- number of groups currently being used
 *                               for matching
 *						curgroups -- array of group numbers for searching
 *						setcurrent -- boolean: TRUE == letter set for matching
 *																	is up to date
 *
 * Globals output: numgroupson -- if group was in use
 *												decremented by 1
 *						 curgroups -- if the group was in use,
 *											 group deleted
 *						 setcurrent -- if the group was in use
 *											  set to FALSE
 *
 * Error conditions: if the group was not in use
 *							  an error message will be displayed on the screen
 *
 * Other functions called: none
 *
 */

{
    register int i;	/* Loop index for searching curgroups */
    char found;			/* Boolean: TRUE == found the group */


    for( i = 1, found = FALSE; i <= numgroupson; i++ )
    {																/* Is the group active? */
        if ( curgroups[i] == group )
            found = TRUE;						/* group is active */

        /* Compress curgroups to get rid of
        *	 the entry for the group
        */
        if ( found )
            curgroups[i] = curgroups[(i + 1)];
    }
    if ( found )
    {
        numgroupson--;
        setcurrent = FALSE;							/* Must recompute letterset */
    }
    else
        Process_msg(2, wxNullStr, (long unsigned) sym_name(group, GROUP_HEAD));

} /* End--groupexclude */


/****************************************************************************/
void CCCModule::compilecc()				 /* Initialize CC part of program */
/****************************************************************************/

/* This procedure is the initialization of the Consistent Change part of the
program. */

/* Description:
 *						Initialize the Consistent Change part of the program,
 *					including loading the change table if there is one and
 *					compiling it if necessary.
 *
 * Return values: none
 *
 * Globals input: 
 *						sym_table -- table used for symbolic names within the
 *												change table
 *						tablelimit -- highest valid address in table
 *						fontsection -- boolean: TRUE == compiling font section
 *																	 of table
 * (MS only)		mandisplay -- boolean: TRUE == echo MS code to screen
 *						uppercase -- boolean:
 *						caseless -- boolean: TRUE == ignore case on input text
 *						debug -- boolean: TRUE == give debug display
 *						mydebug -- boolean:
 *						tablefile -- pointer to CC table input file,
 *											also used for compiled output file
 *						filenm -- file name buffer
 *						namelen -- length of contents of filenm
 *						tblarg -- table name if input on command line
 *						notable -- boolean: TRUE == no change table
 *						parsepntr -- pointer into input line
 *						parse2pntr -- pointer into input line
 *						was_math -- boolean: TRUE == last command parsed was a
 *																 math operator
 *																(add, sub, mul, div)
 *
 * Globals output:  All booleans set.
 *						  Pointers in unknown state.
 *
 * Error conditions: If a file is not found or is too small,
 *							  the user will be given another chance.
 *							If there is an error while compiling the table,
 *							  an appropriate error message will be displayed.
 *
 * Other functions called:
 *                         storechar -- store an element in the table
 *									inputaline -- read a line of input from
 *														the table file
 *									wedgeinline -- is there a > in the input line?
 *									parse -- find the next logical group in
 *												  the input line
 *									storeelement -- store an element in the internal
 *															change table
 *									cctsetup -- set up the pointers for the
 *													  internal change table
 *									cctsort -- sort the array of pointers into the
 *													 change table
 *									fontmsg -- display an appropriate message because
 *													 we found a font section
 *  (debugging only)			cctdump -- dump the internal change table to the
 *													 screen
 *
 */

{
    unsigned tablemax;		  /* largest valid subscript +1 for table */
    unsigned tablesize = 0;		  /* actual loaded size of table */
    SSINT ch;
    int i;		/* Miscellaneous loop index */
    int check_prec;		// 7.4.30 BJY

    /* Allocate space for compiled CC table (freeing the old one first
    if necessary) */
    if (table != NULL)
        free(table);
    // in DLL mode just go for the maximum size, don't mess around
    tablemax = MAX_ALLOC / sizeof(SSINT);
    table = (SSINT *) tblalloc(tablemax, (sizeof(SSINT)));
    tablelimit = table + tablemax;		/* Initialize limits */

    /* Initialize internal booleans */
    fontsection = FALSE;
    was_math = FALSE;
    was_string = FALSE;
    uppercase = FALSE;
    caseless = FALSE;
    pstrBuffer= strBuffer;
    memset(TableLineIndexes, 0, sizeof(TableLineIndexes));
    /* Initialize the symbol table */
    for ( i = 0; i < 4; i++ )
    {
        sym_table[i].list_head = NULL;
        sym_table[i].next_number = MAXARG;
    }

    bytset(storepre, 0, NUMSTORES+1);   // do this before processing cc table

    errors = FALSE;			 /* initialize */
    tblfull = FALSE;
    doublebyte_mode = FALSE;       /* initialize doublebyte due to early scan  */
    doublebyte_recursion = FALSE;
    doublebyte1st = doublebyte2nd = 0;
    utf8encoding= FALSE;
    tloadpointer = table;
    cngpend = cngpointers;

    if (lpszCCTableBuffer)
    {
		// changes table is already in allocated buffer
        tablefile= NULL;
    }
    else
    {
        filenm = tblarg;           /* copy name from command line */
        namelen = filenm.Length(); 

		// tries to open the file using tblarg name
		if ((tablefile= wfopen(filenm,BINREAD)) == NULL)
        {
			// open failed, if it didn't end in .cct, try opening it after adding .cct to tblarg name
            if (filenm.Find(_T(".cct")) == -1 && filenm.Find(_T(".CCT")) == -1)
            {
                filenm += _T(".CCT"); /* Try an extension of .CCT */
                tablefile= wfopen(filenm,BINREAD);
            }

            if (tablefile == NULL)
            {
				// still couldn't open changes table, so issue error
                Process_msg(55, filenm, 0);
                errors = TRUE;                                      //7.4.15
                return;
            }

        }
    }
    notable = FALSE;

    ch = wgetc(tablefile); // original cc sources used xgetc

    if (ch == EOF)
        storechar(SPECPERIOD);
    else
    {
        /* see if we have precompiled table (by looking at first byte)   */
        if ((ch & 0x80) && ((ch & 0xFF) != 0xEF))
            /* precompiled tables have one byte per value, if high bit is
            	on then it was a command, so turn on high half of it       */
        {                             /* precompiled table            */
            ch = ch | 0xff00;             /* turn this back into command  */
            storechar(ch);                /* read it directly             */

            while ( (ch = wgetc(tablefile)) != EOF )   /* into table area  */ // original cc sources used xgetc
            {
                if ( ch & 0x80 )           /* if meant to be a command     */
                    ch = ch | 0xff00;       /* turn this back into command  */
                else                       /* old compiled tables had      */
                    if ( ch == CTRLZ )      /*  control Z, make that into   */
                        ch = ENDFILECMD;     /*   the new ENDFILECMD instead */
                storechar(ch);
            }
        }
        else										/* table not precompiled */
        {
			/* Check for Byte Order Mark and ignore if necessary */
			if ((ch & 0xFF) == 0xEF)
			{
				SSINT ch1 = wgetc(tablefile); // original cc sources used xgetc
				if ((ch1 & 0xFF) == 0xBB)
				{
					SSINT ch2 = wgetc(tablefile); // original cc sources used xgetc
					if ((ch2 & 0xFF) != 0xBF)
					{
						wungetc(ch2, tablefile); // original cc sources used xungetc
						wungetc(ch1, tablefile); // original cc sources used xungetc
						wungetc(ch, tablefile); // original cc sources used xungetc
					}

				} else {
					wungetc(ch1, tablefile); // original cc sources used xungetc
					wungetc(ch, tablefile); // original cc sources used xungetc
				}
			} else {
				wungetc(ch, tablefile); // original cc sources used xungetc
			}
            iCurrentLine= 0;

            while (!wfeof(tablefile)) // original cc sources used xfeof
            {											/* compile table */
                inputaline();
                iCurrentLine++;
                
				parse2pntr = line;

                if (!wedgeinline())
                {
                    flushstringbuffer(FALSE);
                    if (TableLineIndexes[iCurrentLine - 1] == 0)
                    {
                        if (strlen(line) > 0)
                            TableLineIndexes[iCurrentLine - 1]= (unsigned)((long)tloadpointer - (long)table) / sizeof(SSINT);
                        else
                            TableLineIndexes[iCurrentLine - 1]= ~0u;	// Neil Mayhew - 6 Nov 98
                    }
                }
                else
                {										/* search parse */
                    flushstringbuffer(FALSE);
                    storechar( SPECPERIOD);
                    check_prec = TRUE;		// 7.4.30 BJY
                    precparse = NULL;       // 7.4.30
                    TableLineIndexes[iCurrentLine - 1]= (unsigned)((long)tloadpointer - (long)table) / sizeof(SSINT);

                    for (;;)
                    {
                        parse( &parsepntr, &parse2pntr, TRUE);
                        if ( !*parsepntr || *parsepntr == WEDGE )
                            break;
                        if (check_prec)		// 7.4.30 BJY Handle prec() command(s) before string
                            check_prec = chk_prec_cmd();
                        else
                            storeelement( TRUE);
                        if ( parsepntr > parse2pntr )
                            parse2pntr = parsepntr;	/*The parse may have gone
                        						* beyond the current element
                        						*/
                    }
                    flushstringbuffer(TRUE);
                    storechar( SPECWEDGE);
                }
                for (;;)
                {									/* replacement parse */
                    parse( &parsepntr, &parse2pntr, TRUE);
                    if ( !*parsepntr )
                        break;
                    storeelement( FALSE);
                    if ( parsepntr > parse2pntr )
                        parse2pntr = parsepntr;		/*The parse may have gone
                    							* beyond the current element
                    							*/
                }
                if (fontsection)
                    break;
            }
            flushstringbuffer(FALSE);
            storechar(SPECPERIOD);
        }
    }
    if (tablefile)
    {
        wfclose(tablefile);						/* recover file BUFFER */
        tablefile= NULL;
    }
    TableLineIndexes[iCurrentLine]= (unsigned)((long)tloadpointer - (long)table) / sizeof(SSINT);

    for (i= iCurrentLine; i > 0;)
    {
        i--;

		// TableLineIndexes below was only declared when _WINDLL was defined in original cc sources.
		// 
		// whm Note: Here in compilecc() is a test to see if TableLineIndexes[i] == -1 (a negative number). 
		// This appears to be comparing the address of an array with a negative number. GCC gives a warning 
		// at this point that says, "warning: comparison between signed and unsigned integer expressions."
        if (TableLineIndexes[i] == -1)
            TableLineIndexes[i]= TableLineIndexes[i + 1];
    }
    if ( fontsection )					/* Old style fonts */
        fontmsg();

    if (tblfull)
    {
        Process_msg(56, wxNullStr, 0);
        errors = TRUE;		/* Mark this as a fatal error */
    }

    if (errors)
    {
        Process_msg(57, wxNullStr, 0);
    }
    else
    {														/* error flag check */

        /* Now shrink allocation for 'table' down to miminum */
        tablesize = tloadpointer - table;
        table = (tbltype)realloc(table, tablesize * sizeof(SSINT));
        tablelimit = table + tablesize;
        tloadpointer= tablelimit; // ADDED 7-31-95 DAR

        /* Check for symbols that were referenced, but not defined */
        check_symbol_table();

        /* Setup pointers into table[] and sort cngpointers */
        if ( cctsetup() )
            cctsort();
    }
} /* End--compilecc */

#define RestoreVariable(x) x = ccGlobals.x // x = Global_Vars->x
#define RestoreArray(x, size) memcpy(x, ccGlobals.x, (size) * sizeof(*x)) // Global_Vars->x

/**********************************************************************/
void CCCModule::RestoreState()    /* restore our global variables  */
/**********************************************************************/
/*
 * Description:
 *                This routine restores most of CC's global variables.
 *                (It does not store ones that the DLL does not care about,
 *                ones related to input and output files for example).
 *                This routines unlocks and frees the area that the handle
 *                is for after restoring the global variables.
 *
 *
 *
 * Input Value:
 *                hGlobin is a handle for the old global variables that have
 *                been saved there.
 *
 *
 * Output Value:
 *                This routines unlocks the area that the handle
 *                is for after restoring the global variables.
 *
 *
 */
{
    int  i;

    RestoreVariable(sym_table[0]);
    RestoreVariable(sym_table[1]);
    RestoreVariable(sym_table[2]);
    RestoreVariable(sym_table[3]);                                                                                                   

    cctpath = ccGlobals.cctpath; //RestoreArray(cctpath,  PATH_LEN + 1);
    tblarg = cctpath; //tblarg = &cctpath[0];
    RestoreVariable(tablefile);
    RestoreVariable(bEndofInputData);
    RestoreVariable(nextstatusupdate);
    RestoreVariable(nullname[0]);

    RestoreVariable(parsepntr);
    RestoreVariable(parse2pntr);
    RestoreVariable(fontsection);
    RestoreVariable(notable);
    RestoreVariable(was_math);
    RestoreVariable(was_string);
    RestoreVariable(table);
    RestoreVariable(tablelimit);
    RestoreVariable(tloadpointer);
    RestoreVariable(maintablend);
    RestoreVariable(storelimit);
    RestoreArray(switches, MAXARG + 1);

    for ( i = 0; i < NUMSTORES + 1; i++ )
    {
        RestoreVariable(storebegin[i]);
        RestoreVariable(storend[i]);
        RestoreVariable(storeact[i]);
        RestoreVariable(storepre[i]);
    }

    RestoreVariable(curstor);
    RestoreVariable(iStoreStackIndex);
    RestoreVariable(storeoverflow);
    RestoreVariable(doublebyte1st);
    RestoreVariable(doublebyte2nd);

    for ( i = 0; i < MAXGROUPS + 1; i++ )
    {
        RestoreVariable(groupbeg[i]);
        RestoreVariable(groupxeq[i]);
        RestoreVariable(groupend[i]);
    }

    RestoreArray(curgroups, GROUPSATONCE + 1);
    RestoreVariable(cgroup);
    RestoreVariable(numgroupson);
    RestoreArray(letterset, 256);
    RestoreVariable(setcurrent);
    RestoreArray(defarray, MAXARG + 1);
    RestoreArray(stack, STACKMAX + 1);
    RestoreVariable(stacklevel);
    RestoreVariable(backbuf);
    RestoreVariable(backinptr);
    RestoreVariable(backoutptr);
    RestoreVariable(dupptr);
    RestoreVariable(cngpointers);
    RestoreVariable(cngpend);
    RestoreVariable(match);
    RestoreVariable(matchpntr);
    RestoreVariable(maxsrch);
    RestoreVariable(errors);
    RestoreVariable(bFileErr);
    RestoreVariable(tblfull);
    RestoreVariable(eof_written);
    RestoreVariable(mandisplay);
    RestoreVariable(single_step);
    RestoreVariable(caseless);
    RestoreVariable(uppercase);
    RestoreVariable(unsorted);
    RestoreVariable(binary_mode);
    RestoreVariable(utf8encoding);
    RestoreVariable(doublebyte_mode);
    RestoreVariable(doublebyte_recursion);
    RestoreVariable(quiet_flag);
    RestoreVariable(noiseless);
    RestoreVariable(hWndMessages);

    RestoreArray(StoreStack, STORESTACKMAX);
    RestoreVariable(mchlen[0]);
    RestoreVariable(mchlen[1]);
    RestoreVariable(matchlength);
    RestoreVariable(tblxeq[0]);
    RestoreVariable(tblxeq[1]);
    RestoreVariable(tblptr);
    RestoreVariable(mchptr);
    RestoreVariable(matchpntrend);
    RestoreVariable(executetableptr);
    RestoreVariable(firstletter);
    RestoreVariable(cngletter);
    RestoreVariable(cnglen);
    RestoreVariable(endoffile_in_mch[0]);
    RestoreVariable(endoffile_in_mch[1]);
    RestoreVariable(endoffile_in_match);
    RestoreVariable(store_area);
    RestoreArray(keyword, 20 + 1);
    RestoreVariable(begin_found);
    RestoreVariable(precparse);
    RestoreVariable(nInBuf);
    RestoreVariable(nInSoFar);
    RestoreVariable(bPassedAllData);
    RestoreVariable(nMaxOutBuf);
    RestoreVariable(nUsedOutBuf);
    RestoreVariable(bMoreNextTime);
    RestoreVariable(bNeedMoreInput);
    RestoreVariable(bOutputBufferFull);
    RestoreVariable(bFirstDLLCall);
    RestoreVariable(bBeginExecuted);
    RestoreVariable(hInputBuffer);
    RestoreVariable(hOutputBuffer);
    RestoreVariable(bSavedDblChar);
    RestoreVariable(dblSavedChar);
    RestoreVariable(bSavedInChar);
    RestoreVariable(inSavedChar);
    RestoreVariable(bAuxBufUsed);
    RestoreVariable(bProcessingDone);
    RestoreVariable(bPassNoData);
    RestoreVariable(nCharsInAuxBuf);
    RestoreVariable(iAuxBuf);
    RestoreVariable(nAuxLength);
    RestoreVariable(hAuxBuf);
    RestoreVariable(lDLLUserInputCBData);
    RestoreVariable(lDLLUserOutputCBData);
    RestoreVariable(lDLLUserErrorCBData);
    RestoreVariable(hTableLineIndexes);
    RestoreVariable(iCurrentLine);

    if ( hTableLineIndexes != 0 )    // lock Table Line Indexes space if allocated yet
    {
        TableLineIndexes = (unsigned *)hTableLineIndexes; // lock input buffer

        if ( TableLineIndexes == NULL )
        {
            Process_msg(67, wxNullStr, 0);
            return;
        }
    }

    if ( hInputBuffer == 0 )    // lock input buffer space if allocated yet
    {
        RestoreVariable(lpInBufStart);
        RestoreVariable(lpInBuf);
    }
    else
    {
        lpInBufStart = (char*)hInputBuffer; // lock input buffer

        if ( lpInBufStart == NULL )
        {
            Process_msg(67, wxNullStr, 0);
            return;
        }
        lpInBuf = lpInBufStart + nInSoFar;
    }

    if ( hOutputBuffer == 0 )   // lock output buffer space if allocated yet
    {
        RestoreVariable(lpOutBuf);
        RestoreVariable(lpOutBufStart);
    }
    else
    {
        lpOutBufStart = (char *)hOutputBuffer;  // lock space for output buffer
        if ( lpOutBufStart == NULL )
        {
            Process_msg(67, wxNullStr, 0);
            return;
        }
        lpOutBuf= lpOutBufStart + nUsedOutBuf;
    }

    if ( hAuxBuf != 0 )       // lock auxiliary buffer space if allocated
    {
        lpAuxBufStart = (char *)hAuxBuf;  // lock space for aux buffer area
        if ( lpAuxBufStart == NULL )
        {
            Process_msg(67, wxNullStr, 0);
            return;
        }

        lpAuxNextToOutput= lpAuxBufStart + iAuxBuf;
        lpAuxOutBuf= lpAuxBufStart + nCharsInAuxBuf;
    }
}  // End - RestoreState

#define SaveVariable(x) ccGlobals.x = x
#define SaveArray(x, size) memcpy(ccGlobals.x, x, (size) * sizeof(*x))
/************************************************************************/
void CCCModule::SaveState()    /* save state of global variables */
/************************************************************************/
/*
 * Description:
 *                This routine saves the state of many of CC's global 
 *                variables.  It saves the state of the variables that
 *                are relevent when CC is called from a DLL, it does not
 *                save variables related to input and output files.
 *
 *
 *
 * Input values:
 *                hInHandle == NULL means to allocate/use a new area/handle.
 *
 *                hInHandle == non-NULL means to use that handle.
 *
 *
 *
 * Return values:
 *                0    If an error occurred either allocating or locking
 *                     the space where we wanted to save global variables.
 *
 *                A valid handle for the area with the variables is the
 *                normal successful non-error return value.
 *
 *
 */
{
    int  i;

    SaveVariable(sym_table[0]);
    SaveVariable(sym_table[1]);
    SaveVariable(sym_table[2]);
    SaveVariable(sym_table[3]);
    ccGlobals.cctpath = cctpath; //SaveArray(cctpath, PATH_LEN + 1);
    SaveVariable(tblarg);
    SaveVariable(tablefile);
    SaveVariable(bEndofInputData);
    SaveVariable(nextstatusupdate);
    SaveVariable(nullname[0]);

    // We assume that parsepntr and parse2pntr pt to alloc'ed areas
    SaveVariable(parsepntr);
    SaveVariable(parse2pntr);
    SaveVariable(fontsection);
    SaveVariable(notable);
    SaveVariable(was_math);
    SaveVariable(was_string);
    SaveVariable(table);
    SaveVariable(tablelimit);
    SaveVariable(tloadpointer);
    SaveVariable(maintablend);
    SaveVariable(storelimit);
    SaveArray(switches, MAXARG + 1);

    for ( i = 0; i < NUMSTORES + 1; i++ )
    {
        SaveVariable(storebegin[i]);
        SaveVariable(storend[i]);
        SaveVariable(storeact[i]);
        SaveVariable(storepre[i]);
    }

    SaveVariable(curstor);
    SaveVariable(iStoreStackIndex);
    SaveVariable(storeoverflow);
    SaveVariable(doublebyte1st);
    SaveVariable(doublebyte2nd);

    for ( i = 0; i < MAXGROUPS + 1; i++ )
    {
        SaveVariable(groupbeg[i]);
        SaveVariable(groupxeq[i]);
        SaveVariable(groupend[i]);
    }

    SaveArray(curgroups,  GROUPSATONCE + 1);

    SaveVariable(cgroup);
    SaveVariable(numgroupson);

    SaveArray(letterset, 256);

    SaveVariable(setcurrent);

    SaveArray(defarray, MAXARG + 1);
    SaveArray(stack, STACKMAX + 1);

    SaveVariable(stacklevel);
    SaveVariable(backbuf);
    SaveVariable(backinptr);
    SaveVariable(backoutptr);
    SaveVariable(dupptr);
    SaveVariable(cngpointers);
    SaveVariable(cngpend);
    SaveVariable(match);
    SaveVariable(matchpntr);
    SaveVariable(maxsrch);
    SaveVariable(errors);
    SaveVariable(bFileErr);
    SaveVariable(tblfull);
    SaveVariable(eof_written);
    SaveVariable(mandisplay);
    SaveVariable(single_step);
    SaveVariable(caseless);
    SaveVariable(uppercase);
    SaveVariable(unsorted);
    SaveVariable(binary_mode);
    SaveVariable(utf8encoding);
    SaveVariable(doublebyte_mode);
    SaveVariable(doublebyte_recursion);
    SaveVariable(quiet_flag);
    SaveVariable(noiseless);
    SaveVariable(hWndMessages);

    SaveArray(StoreStack, STORESTACKMAX);
    SaveVariable(mchlen[0]);
    SaveVariable(mchlen[1]);
    SaveVariable(matchlength);
    SaveVariable(tblxeq[0]);
    SaveVariable(tblxeq[1]);
    SaveVariable(tblptr);
    SaveVariable(mchptr);
    SaveVariable(matchpntrend);
    SaveVariable(executetableptr);
    SaveVariable(firstletter);
    SaveVariable(cngletter);
    SaveVariable(cnglen);
    SaveVariable(endoffile_in_mch[0]);
    SaveVariable(endoffile_in_mch[1]);
    SaveVariable(endoffile_in_match);
    SaveVariable(store_area);
    SaveArray(keyword, 20 + 1);
    SaveVariable(begin_found);
    SaveVariable(precparse);
    SaveVariable(nInBuf);
    SaveVariable(nInSoFar);
    SaveVariable(bPassedAllData);
    SaveVariable(nMaxOutBuf);
    SaveVariable(nUsedOutBuf);
    SaveVariable(bMoreNextTime);
    SaveVariable(bNeedMoreInput);
    SaveVariable(bOutputBufferFull);
    SaveVariable(bFirstDLLCall);
    SaveVariable(bBeginExecuted);
    SaveVariable(hInputBuffer);
    SaveVariable(hOutputBuffer);
    SaveVariable(bSavedDblChar);
    SaveVariable(dblSavedChar);
    SaveVariable(bSavedInChar);
    SaveVariable(inSavedChar);
    SaveVariable(bAuxBufUsed);
    SaveVariable(bProcessingDone);
    SaveVariable(bPassNoData);
    SaveVariable(nCharsInAuxBuf);
    SaveVariable(iAuxBuf);
    SaveVariable(nAuxLength);
    SaveVariable(hAuxBuf);
    SaveVariable(lDLLUserInputCBData);
    SaveVariable(lDLLUserOutputCBData);
    SaveVariable(lDLLUserErrorCBData);
    SaveVariable(hTableLineIndexes);
    SaveVariable(iCurrentLine);
    SaveVariable(lpInBufStart);
    SaveVariable(lpInBuf);
    SaveVariable(lpOutBuf);
    SaveVariable(lpOutBufStart);

}  // End - SaveState


/**************************************************************************/
int CCCModule::CCProcessBuffer (char *lpInputBuffer, int nInBufLen,
                                 char *lpOutputBuffer, int *npOutBufLen)
/**************************************************************************/
/*
 * Description:
 *                This DLL routine is called by the user to have CC
 *                operations performed on one user input buffer, and have
 *                the results placed into one user output buffer.
 *                This interface does not use any callbacks at all.
 *                This does not save any data across calls, it processes
 *                all of the data that was passed into it.  This calls
 *                CCReinitializeTable near the start of its processing,
 *                so a user that calls this repeatedly with different data
 *                does not have to bother with that.
 *
 *
 *
 * Input values:
 *                hProHandle is the handle with input global data.
 *
 *                *lpInputBuffer points to a user buffer area with user input.
 *
 *                nInBufLen is the size of the user input buffer.  Note that
 *                          if the user wants a null character at the end of
 *                          this to be considered part of the input this
 *                          length should include the null character.  If the
 *                          user does not want a null character included this
 *                          should not include that, but note that the output
 *                          will then not contain a null character at the
 *                          end of the data, the user must then totally rely
 *                          upon the *npOutBufLen output buffer length value.
 *
 *                *lpOutputBuffer points to a user buffer area for CC output.
 *
 *                *npOutBufLen is the max size of the buffer CC outputs to.
 *                             Note that this will have to be longer than
 *                             nInBufLen above if CC adds to the size of
 *                             the user output at all (which it often does).
 *                             If it is not big enough, CC terminates with
 *                             an error message.
 *
 *
 *
 * Output values: 
 *                The global variables referenced by hProHandle are updated.
 *                
 *                *lpOutputBuffer points to buffer with output data from CC.
 *
 *                *npOutBufLen points to the used size of the output buffer.
 *
 *
 *
 * Return values:
 *                0           Success
 *
 *                Other       Error occurred.
 *
 */
{
    // NOTE: This routine has many similarities to CCMultiProcessBuffer.
    //       Changes made here might apply to those routines as well!

    if ( npOutBufLen == NULL )
    {
        Process_msg(102, wxNullStr, 0);
        return(-1);
    }

    if ( *npOutBufLen == 0 )      // if passed in no data then just return
    {
        Process_msg(137, wxNullStr, 0);
        *npOutBufLen = 0;
        return(-1);
    }

    if ( lpInputBuffer == NULL )
    {
        Process_msg(103, wxNullStr, 0);
        *npOutBufLen = 0;
        return(-1);
    }

    if ( lpOutputBuffer == NULL )
    {
        Process_msg(104, wxNullStr, 0);
        *npOutBufLen = 0;
        return(-1);
    }

    if ( nInBufLen <= 0 )      // if passed in no data then just return
    {
        Process_msg(94, wxNullStr, 0);
        *npOutBufLen = 0;
        return(-1);
    }

    if ( CCReinitializeTable() != 0 )
    {
        Process_msg(97, wxNullStr, 0);
        return(-1);
    }

    RestoreState();            // restore our global variables

    nMaxOutBuf = *npOutBufLen;           // set maximum size of output buffer
    bPassedAllData = TRUE;               // have all our input data already
    bNeedMoreInput= FALSE;
    nInBuf = nInBufLen;                  // set input buffer length
    nInSoFar= 0;
    lpInBufStart = lpInputBuffer;        // point to start of input buffer
    lpInBuf = lpInBufStart;              // point to start of input buffer
    lpOutBuf = lpOutputBuffer;           // point to output buffer area
    bOutputBufferFull= FALSE;
    nUsedOutBuf = 0;                     // denote no data has been output yet

    CallCCMainPart();                    // call the "main" part of CC

    // If the size of the output buffer we tried to pass all of the
    // output data to was not big enough, then we have one of these
    // flags on.  If so, then put out appropriate error message.
    if (( backoutptr != backinptr )
            || ( bSavedDblChar )
            || ( bAuxBufUsed ))
    {
        SaveState();            // save global variables
        Process_msg(83, wxNullStr, (long unsigned) *npOutBufLen);
        *npOutBufLen = 0;                 // pass no data back if error
        return(-1);
    }

    if ( errors )
    {
        SaveState();            // save global variables
        Process_msg(84, wxNullStr, 0);
        *npOutBufLen = 0;                 // pass no data back if error
        return(-1);
    }

    *npOutBufLen = nUsedOutBuf;          // return amount of data sent back

    SaveState();               // save global variables

    return(0);

}  // End - CCProcessBuffer

/**************************************************************************/
int CCCModule::CCSetUTF8Encoding(bool lutf8encoding)
/**************************************************************************/
/*
 * Description:
 *                This DLL routine is called by the user to enable or disable
 *                utf-8 character handling.
 *
 *
 * Input values:
 *                hCCTableHandle has our input handle for global variables.
 *
 *                lutf8encoding is set to TRUE to enable UTF-8 character
 *                processing and set to FALSE to disable UTF-8 character.
 *                note the the "u" encoding directive will still expand 
 *                a USC4 character to UTF-8 within the cc table even if
 *                UTF-8 processing is disabled.
 *
 * Output values: The global variable utf8encoding is updated,
 *
 * Return values:
 *                0           Success.
 *                
 *                non-zero    Error occurred.
 *
 *
 */
{
    RestoreState();  // this restores our global variables

    utf8encoding= lutf8encoding;

    SaveState();     // save global variables into saved area

    return(0);

}  // End - CCSetUTF8Encoding

/**************************************************************************/
int CCCModule::CCUnloadTable()
/**************************************************************************/
/*
 * Description:
 *                This DLL routine is called when the user is done with CC
 *                to free the global variable structure area and other stuff.
 *                This needs to be the last CC DLL interface called.  This
 *                routine also unregisters the calling task.
 *
 *
 *
 * Input Value:
 *                hUnlHandle is the handle for the area to be freed.
 *
 *
 *
 * Output Value:
 *                The area for hUnlHandle etc are unlocked and freed.
 *
 *
 *
 * Return values:
 *                0          Successful completion.
 *                          
 *                non-zero   An error occurred.
 *
 *
 */
{
    int retcode;

    RestoreState();  // this restores the global variable structure

    retcode= CleanUp();

    return(retcode);

}   // End - CCUnloadTable


/****************************************************************************/
void CCCModule::out_char_buf(char inchar)               /* Output a single character  */
/****************************************************************************/
/*
 *
 * Description: This is a Windows DLL-specific subroutine of the out_char()
 *              function above.  With the DLL we want to output to the user
 *              only a specific number of bytes at a time.  We can usually
 *              control this nicely with our logic, but at times like when
 *              we output a lot of data at EOF or when a match is found etc
 *              we will need to put out a lot of data.  If we would overflow
 *              our regular Windows DLL output buffer, then we will instead 
 *              put that to a separate buffer, and later copy that into the
 *              regular output buffer.  This routine checks for that case,
 *              and if we need to do that it allocates (or enlarges) the 
 *              new buffer, and moves the data there as appropriate.  Or in
 *              the usual case it will just copy data to the output buffer.
 *
 *
 */
{
    if (bOutputBufferFull)     // if would "overflow" output buffer
    {

        if ( nAuxLength > 0 )           // if we have already allocated area
        {
            if ( nCharsInAuxBuf >= nAuxLength )  // if area not big enough
            {                                 // then make it bigger
                nAuxLength = nAuxLength * 2;      // try doubling the size
                hAuxBuf = realloc((void *)hAuxBuf, nAuxLength);
                if (hAuxBuf == NULL)
                {
                    errors = TRUE;
                    Process_msg(60, wxNullStr, 0);
                    return;
                }
            }
        }
        else                            // need to allocate special area
        {
            nAuxLength = AUX_BUFFER_LEN; // set this as initial aux buffer size
            hAuxBuf = malloc(nAuxLength);  // allocate space

            if (hAuxBuf == NULL)
            {
                errors = TRUE;
                Process_msg(61, wxNullStr, 0);
                return;
            }
            lpAuxBufStart = (char *)hAuxBuf;  // lock space
            if (lpAuxBufStart == NULL)
            {
                errors = TRUE;
                Process_msg(62, wxNullStr, 0);
                return;
            }
            lpAuxOutBuf = lpAuxBufStart;
            lpAuxNextToOutput = lpAuxBufStart;
        }
        *lpAuxOutBuf++ = inchar;        // put char to our special area
        nCharsInAuxBuf++;               // increment special area count
        bAuxBufUsed = TRUE;             // denote special area is used
    }
    else                               // would not overflow output buffer
    {
        *lpOutBuf++ = inchar;           // output character to output buffer
        nUsedOutBuf++;                  // count used bytes of output buffer

        if (nUsedOutBuf >= nMaxOutBuf)
        {
            bOutputBufferFull= TRUE;
        }
    }
}

/****************************************************************************/
void CCCModule::fillmatch()                          /* Fill match area, execute begin */
/****************************************************************************/
/* Description:
 *                fill up match area, execute the BEGIN if there is one.
 *
 * Return values: none
 *
 * Globals input: tloadpointer -- pointer to the next available space beyond
 *												the change table array
 *						numgroupson -- number of groups currently being searched
 *						curgroups -- array of group numbers to be checked
 *						notable -- boolean: TRUE == no change table
 *						matchpntr -- pointer to the place to start the next
 *											attempted match
 *						table -- array containing the change table
 *						debug -- table debugging flag
 *
 * Globals output: numgroupson -- set to either zero (no table or no group 1)
 *										 or one (group one exists)
 *						 matchpntr -- set to the beginning of the match area
 *
 * Error conditions: none
 *
 * Other functions called: inputchar -- get a char from the input file
 *									execute -- execute the replacement part of a change
 *
 */

{
    register int i;		   /* Loop index for initializing storage pointers */
    register tbltype tpntr; /* For finding the physical first group if
    								 *	 defaulting to an empty group
    								 */

    if (executetableptr)
        executetableptr= execute( 0, executetableptr, TRUE );
    else
    {
        refillinputbuffer();

        if (table[0] == SPECWEDGE)							/* Do begin command */
        {
            matchlength= 0;
            executetableptr= execute( 0, &table[1], TRUE );
        }
    }

    if (!executetableptr)
    {
        bBeginExecuted= TRUE;

		// whm combined the first conditional statement together with the following one to avoid 
		// compiler warning of "potentially uninitialized local variable 'i' used"
        if ( numgroupson == 1 )
		{
            i = curgroups[1];					/* Get the currently active group */

			if (groupbeg[i] == groupend[i])
			{
				/*   Start at the beginning of the table and
				* go until we find the beginning of a group.

            			* INTERNALLY (!!) this is indicated by two
				* SPECPERIODs with only one byte between,
				* because the GROUPCMD was replaced with a
				* SPECPERIOD at setup time.
				*
				* The next byte will contain the number of
				* first group in the table.
				*/
				for ( tpntr = table; tpntr < tloadpointer; tpntr++ )
				{
					if ( (*tpntr == SPECPERIOD) && ( *(tpntr + 2) == SPECPERIOD) )
					{
						curgroups[1] = (int) *(tpntr + 1);	/* Get the group number */
						break;
					}
					if ( *tpntr == TOPBITCMD )
						tpntr++;						/* Next char is a high-bit literal
                									* so skip it, to avoid a possible
                									* "false match"
                									*/
				} /* End--search for first group */

				if ( curgroups[1] == i )	/* No explicitly-defined group in table */
				{
					if (tloadpointer - table > 2)
						Process_msg(23, wxNullStr, (long unsigned) sym_name(i, GROUP_HEAD) );
					else
						Process_msg(136, wxNullStr, (long unsigned) sym_name(i, GROUP_HEAD) );
				}
			} /* End--starting to default to an empty group */
		}

        /*   Do the after display here, now that the real
        * starting group has been established
        */

        if ( !numgroupson && !notable )
        {
            Process_msg(25, wxNullStr, 0);
            bailout(BADEXIT, FALSE);
        }
    }
} /* End--fillmatch */

/****************************************************************************/
void CCCModule::execcc()			  /* Execute CC */
/****************************************************************************/
{
    while (!eof_written && !bNeedMoreInput && !bOutputBufferFull)
    {
        ccin();
    }

    if (eof_written)
        while (( backoutptr != backinptr ) && (!bOutputBufferFull ))
        {
            writechar();
        }
}

/****************************************************************************/
void CCCModule::completterset()					 /* Compute first-letter set for matching */
/****************************************************************************/

/* Description:
 *						Go through the letterset array, setting each element for
 *					which there is a change starting with that letter to TRUE.
 *					If there is a null-match ("" or '') set every element to TRUE.
 *
 * Return values: none
 *
 * Globals input: setcurrent -- boolean: TRUE == letter set is up to date
 *						letterset -- array of booleans: TRUE == there is a match
 *																				that begins with
 *																				that letter
 *						storeact -- array of booleans: TRUE == that store is used in
 *																				a left-executable
 *																				function
 *						curgroups -- array of group numbers for groups currently
 *											being searched for possible matches
 *						groupbeg -- array of pointers to the beginning of groups
 *						groupxeq -- array of pointers to the beginning of the
 *										  left-executable portions of groups
 *						groupend -- array of pointers to the ends of groups
 *
 * Globals output: setcurrent -- set to TRUE
 *						 letterset, storeact -- correct for the currently
 *														active groups
 *
 * Error conditions: none
 *
 * Other functions called: bytset -- set a block of memory to a specified
 *													one-byte value
 *                         ssbytset -- set a block of memory to a specified
 *                                     two-byte value
 *
 */

{
    register tbltype tp;		/* Working pointer into storage area */
    tbltype tp9;				/* Working pointer to the end of the store */
    /*  currently being checked */
    register cngtype cp;		/* Working pointer for going through the changes */
    cngtype clp9;				/* Working pointer to the end of the */
    /*  regular changes for the current group */
    cngtype cxp9;				/* Working pointer to the end of the current group */

    /* Note:  The above variables are declared separately because */
    /*				DECUS C does not allow regular typedef's	*/
    SSINT ch;                                               /* Local temp for comparisons */
    int cg,						/* Index into curgroups array */
    group;				  /* Current group being checked */

    setcurrent = TRUE;						/* Say letter set is up to date */

    bytset( letterset, FALSE, sizeof(letterset));    /* Re-initialize arrays */
    bytset( storeact, FALSE, sizeof(storeact));

    for (cg = 1; cg <= numgroupson; cg++ )				/* Step through groups */
    {
        group = curgroups[cg];					/* Set up working pointers */
        cp = groupbeg[group];					 /*  for this group */
        clp9 = groupxeq[group];
        cxp9 = groupend[group];

        /* Compile first letters of changes starting with character */

        while ( cp < clp9 )
            letterset[(**cp++ & 0x00ff)] = TRUE;

        /* Compile first letters of changes starting with function */

        while ( cp < cxp9 )
        {
            tp = *cp++;
            while ( (ch=(*tp++)) != SPECWEDGE )
            {
                switch ( ch )
                {
                case ANYCMD:												/* any */
                case ANYUCMD:												/* any */
                    storeact[*tp] = TRUE;
                    tp9 = storend[*tp];
                    for ( tp = storebegin[*tp]; tp < tp9; )
                        letterset[(*tp++) & 0xFF] = TRUE;
                    goto L60;
                case CONTCMD:												/* cont */
                    storeact[*tp] = TRUE;
                    letterset[*storebegin[*tp] & 0xFF] = TRUE;
                    goto L60;
                case FOLCMD:												/* fol */
                case PRECCMD:												/* prec */
                case WDCMD:													/* wd */
                case FOLUCMD:												/* fol */
                case PRECUCMD:												/* prec */
                case WDUCMD:												/* wd */
                    tp++;
                    break;						/* Doesn't affect 1st letter */

                case TOPBITCMD:                  /* High-bit element */
                    /* DAR use only lower byte of letter.
                    	 Otherwise letterset would have to be 64K */
                    letterset[*tp & 0x00ff] = TRUE;
                    goto L60;
                default:
                    if ( (ch & HIGHBIT) == 0 )
                    {
                        letterset[(ch & 0x00ff)] = TRUE;
                        goto L60;
                    }
                } /* End--switch */

            } /* End--while (ch != SPECWEDGE) */

            /* Any starting character is game */
            bytset( letterset, TRUE, sizeof(letterset));   /* (null match) */
            goto L90;
L60:		;
        } /* End--while (cp < cxp9) */

    } /* End--for (step through curgroups array) */
L90:
    ;

} /* End--completterset */

/****************************************************************************/
int CCCModule::gmatch(int group)										  /* Check through one group */
/****************************************************************************/
//int group;

/* Description:
 *						Go through one group and attempt to find a match.
 *					If we find one, execute it.  If multiple matches are found,
 *					execute the longest one.  If 2 matches of equal length are
 *					found, but one contains a left-executable function (cont(),
 *					wd(), etc.), execute the change without the left-executable
 *					function.
 *
 * Return values:  TRUE == We found a match and executed the change
 *						FALSE == No match found
 *
 * Globals input: groupbeg -- array of pointers to beginnings of changes
 *										  for groups
 *						groupxeq -- array of pointers to the beginnings of
 *										  changes containing left-executable functions
 *										  for groups
 *						groupend -- array of pointers to the end of changes
 *										  for groups
 *						mchlen -- array of lengths of matches found, one length
 *										for regular changes and one for changes
 *										containing left-executable functions
 *						firstletter -- first letter to be checked for matches
 *						matchpntr -- pointer into the match area
 *						cgroup -- group from which currently executing change came
 *
 * Globals output: mchlen -- if match found for that particular type of
 *										 change, contains length of match,
 *										 otherwise, contains -1
 *						 matchpntr -- if a change was found, updated to point to
 *											 the next unchecked character
 *						 cgroup -- if a change was found, group from which it came
 *
 * Error conditions: none
 *
 * Other functions called: cmatch -- try to match a change, letter by letter
 *									execute -- execute the replacement part of a change
 *
 */

{
    register cngtype clp;	/* Pointer to beginning of regular changes */
    register cngtype cxp;	/* Pointer to beginning of left-executable changes */
    register cngtype clp9;	/* Pointer to end of regular changes */
    register cngtype cxp9;	/* Pointer to end of left-executable changes */
    int match_choice;		/* type of match found */

    endoffile_in_mch[0] = FALSE;
    endoffile_in_mch[1] = FALSE;

    clp = groupbeg[group];					/* Initialize change pointers */
    clp9 = cxp = groupxeq[group];
    cxp9 = groupend[group];

    mchlen[0] = mchlen[1] = -1;

    /* if nothing in match buffer then first letter is not really valid
    	don't check for match in this case */
    if ((matchpntrend - matchpntr) != 0)    // ADDED 08-01-95 DAR
    {
        /* Find first change in table, if any, */
        /*  with correct starting letter */
        while ( (clp < clp9) && (**clp < firstletter)  )           // 7b.4r5 ALB Fix Windows GPF on clp
            clp++;

        /* Match changes starting with correct letter */

        while ( (clp < clp9) && (**clp == firstletter) && !cmatch(clp, 1) )   // 7b.4r5 ALB Fix Windows GPF on clp
            clp++;
        /* Match changes starting with left-executable function */

    }
    while ( (cxp < cxp9) && !cmatch(cxp,0) )
        cxp++;

    /* Examine mchlen[0] and mchlen[1] to see what matches were found. */
    /*
    NOTE: Use the following logic: There are two types of matches, type 0 are
    left-execute matches (i.e., leftmost element on match side was a command),
    and type 1 are regular matches (i.e., leftmost element is a letter).
    Either or both types may have been successfully matched in this group.
    If only one type of match was found, then choose that match. If both types
    were found, then compare their lengths to decide which match should be
    used. If their lengths are the same (or the 'unsorted' flag is TRUE)
    then choose the match which is physically first in the table, otherwise
    choose the longest of the two matches.
    */
    match_choice = -1;		/* Assume no match */

    if (endoffile_in_mch[0])
        mchlen[0]++;

    if (endoffile_in_mch[1])
        mchlen[1]++;

    if (mchlen[0] >= 0)			/* Was type 0 match found? */
    {								/* yes */
        if (mchlen[1] >= 0)				/* Was type 1 match also found? */
        {									/* yes, both types were found. */
            if (mchlen[0] == mchlen[1] || unsorted)	/* If lengths are same */
            {													/* or unsorted, then... */
                if (tblxeq[0] < tblxeq[1])					/* choose first in table */
                    match_choice = 0;
                else
                    match_choice = 1;
            }
            else				/* If lengths are different, choose longest */
            {
                if (mchlen[0] > mchlen[1])
                    match_choice = 0;
                else
                    match_choice = 1;
            }
        }
        else		/* Type 0 found, type 1 not found */
            match_choice = 0;		/* obviously choose 0 */
    }
    else if (mchlen[1] >= 0)		/* Was type 1 found? */
        match_choice = 1;

    if (endoffile_in_mch[0])
        mchlen[0]--;

    if (endoffile_in_mch[1])
        mchlen[1]--;

    /* Upon dropping out of the above logic, if match_choice == -1, then no
    match was found, otherwise match_choice indicates which match is to
    be used */
    if (match_choice >= 0)
    {							/* Match was found */

        matchpntr += mchlen[match_choice];	/* Adjust match pointer */
        cnglen = mchlen[match_choice];		/* Tell debug how long match is */
        cgroup = group;		/* Tell the world which group the match is in */
        endoffile_in_match= endoffile_in_mch[match_choice];

        //		execute(mchlen[match_choice],
        //			tblxeq[match_choice], FALSE);		/* Execute replacement */
        //		return(TRUE);
    }
    //	else
    //		return(FALSE);			/* No match */
    return match_choice;

} /* End--gmatch */

/****************************************************************************/
int CCCModule::cmatch(register cngtype cp, int ofs)						/* Match a change, letter by letter */
/****************************************************************************/
//register cngtype cp;		/* Pointer to the match we're trying */
//int ofs;						/* Offset into the change at which to start */
/*  (0 for left-executable matches,  */
/*	1 for regular matches, since we have already */
/*	matched the first letter)		*/

/* Description:
 *						Attempt to match a change, letter by letter,
 *					starting ofs bytes into the change.
 *
 * Return values:  TRUE == we have a match
 *						FALSE == no match
 *
 * Globals input: tblptr -- working pointer to the change
 *						mchptr -- working pointer into the match area
 *						matchpntr -- pointer to the beginning of the current
 *											trial match
 *						cngletter -- copy of the current character from the table
 *						mchlen -- array of lengths of successful matches, one entry
 *										for regular changes and
 *										one for left-executable changes
 *						tblxeq -- array of pointers to replacement strings for
 *										successful matches, one entry
 *										for regular changes and
 *										one for left-executable changes
 *
 * Globals output: tblptr -- if successful match, points to the WEDGE,
 *										 otherwise, contents are unimportant
 *						 mchptr -- if successful match, points to the next
 *										 character beyond the end of the match,
 *										 otherwise, contents are unimportant
 *						 mchlen -- if successful match,
 *										 mchlen[ofs] contains the length of the match,
 *										 otherwise, unchanged
 *						 tblxeq -- if successful match,
 *										 tblxeq[ofs] points to the replacement part
 *										 of the change,
 *										 otherwise, unchanged
 *						 cngletter -- contains the last character from the table
 *											 that was tested for a match
 *
 * Error conditions: none
 *
 * Other functions called: leftexec -- try to match using a left-executable
 *													  function
 *
 */

{
    tblptr = *cp + ofs;							/* Pointer into the table */
    mchptr = matchpntr + ofs;						/* Pointer into the match area */

    for (;;)									/* Do forever (exit is via return) */
    {
        cngletter= *tblptr;

        if (cngletter  & HIGHBIT )
        {
            if ( cngletter == SPECWEDGE )
            {															 /* Successful match */
                mchlen[ofs] = mchptr - matchpntr;		/* Length of match */
                tblxeq[ofs] = tblptr + 1;				/* Pointer to replacement */
                return( TRUE );
            }
            else
            {
                if ( !leftexec(ofs) )				/* Try a left-executable function */
                    return( FALSE );
            }
        }
        else if ( mchptr == matchpntrend ||
                  cngletter != *mchptr)
            //					 (!caseless && cngletter != *mchptr) ||
            //					 (caseless &&  cngletter != tolower(*mchptr)))			/* Compare raw letters */  /* FIX FOR CASELESS DAR 10-29-96 */
            return( FALSE );
        else
            mchptr++;

        tblptr++;						/* On to the next char in the table */

    } /* End--for */

} /* End--cmatch */

/****************************************************************************/
int CCCModule::leftexec(int ofs)					 /* Execute commands on left side of WEDGE */
/****************************************************************************/

/* Description:
 *   This procedure executes commands found on the left of the WEDGE. It
 * expects the command to be in the global cngletter.
 *
 * Return values:  TRUE == successful match
 *						FALSE == no match
 *
 * Globals input: cngletter -- the current command to be executed
 *						mchptr -- pointer to the current trial match
 *						backinptr -- input pointer for the ring buffer
 *						backbuf -- array used for the ring buffer
 *						tblptr -- pointer into the current change
 *						curstor -- if storing, store #, else zero
 *
 * Globals output: mchptr -- advanced based on the success of the match
 *						 tblptr -- advanced to the character following the command
 *										(store number for FOL, PREC, WD, and CONT)
 *										(char with high-bit stripped off for TOPBITCMD)
 *
 * Error conditions: if an illegal command is found,
 *							  an error message will be displayed to the screen
 *
 * Other functions called: anycheck -- is the specified char in the store?
 *									contcheck -- does the next part of the input
 *														match the contents of the
 *														specified store?
 *
 */

#define MAX_STACKED_PREC 10

{
    int f;		/* Return value for anycheck */
    int prec_cnt;	/* Count of fol()'s or prec()'s executed in succession */
	int prec_index; /* current prec command being processed */
    int i;		/* Handy-dandy loop indices */
	int mchoffset;      /* offset into match character buffer */
    tbltype tp_temp;	/* Local copy of tblptr */
	bool storeexhausted;
	SSINT * utf8char = NULL;

    SSINT prec_array[ MAX_STACKED_PREC ];
    /* Array of store #'s for stacked prec()'s
    *   to allow for reversing the order,
    *   so that they work in the order that
    *   they were entered
    *		 (instead of right to left)
    */
	SSINT * precptr; /* Pointer into buffer for the current character for an
					  * anyutfcheck during a prec check 
					  */


    if (cngletter != ENDFILECMD)
    {
        tblptr++;			 /* Assumes all commands here have one arg */
        /* except ENDFILECMD                      */

        if (mchptr == matchpntrend &&
		    cngletter != PRECCMD &&
		    cngletter != PRECUCMD) /* all commands except ENDFILECMD and PRECMD need something in the match
                                               buffer to process, return if the match buffer is empty */
            return FALSE;
    }


    switch ( cngletter )
    {
    case ANYUCMD:
    case ANYCMD:
		if (utf8encoding || cngletter == ANYUCMD)
		{
			f = anyutf8check(mchptr);

			if (f)
				mchptr+= (UTF8AdditionalBytes((UTF8)*mchptr) + 1);
		}
		else
		{
			f = anycheck(*mchptr);

			if (f)
				mchptr++;			
		}
        return f;

    case ENDFILECMD:
        if ((bEndofInputData) && (mchptr == matchpntrend))
        {
            endoffile_in_mch[ofs]= TRUE;

            return TRUE;
        }
        else
            return FALSE;
	case FOLUCMD:
    case FOLCMD:													/* Fol */
        mchoffset = 0;

		if (utf8encoding || cngletter == FOLUCMD)
			f = anyutf8check( mchptr );
		else
			f = anycheck(*mchptr);

        for ( ;; )					/* Do forever, exit is via return */
        {
            /* If we get a failure at any point, the match has failed,
            	*   so return
            	*/
            if ( !f || ( (*(tblptr + 1) != FOLCMD) && (*(tblptr + 1) != FOLUCMD)) )
                return( f );
            else
            {
                tblptr += 2;		/* Skip the current store # and the next FOL */
				if (utf8encoding || cngletter == FOLUCMD)
					mchoffset+= UTF8AdditionalBytes((UTF8)*(mchptr + mchoffset)) + 1;					
				else
					mchoffset++;
            }
			if (utf8encoding || cngletter == FOLUCMD)
				f = (f && anyutf8check( mchptr + mchoffset ));
			else
				f = (f && anycheck( *(mchptr + mchoffset) ));

        } /* End--do forever */

    case PRECUCMD:													/* Prec */
    case PRECCMD:													/* Prec */
        /*
         * Go through and load prec_array from the bottom,
         *  with <store-#>, so that they will be processed
         *  in reverse order.  This will produce the effect most people
         *  will expect:
         *					 store(1) 'a' endstore
		 *					 store(2) 'b' endstore
		 *
         *   'c' prec(1) prec(2)  will match the c in 'abc'
         */

        for ( i = MAX_STACKED_PREC-1, prec_cnt = 1; ; i--, prec_cnt++)
        {
            if ( i >= 0 )
                prec_array[i] = *tblptr;
            if ( *(tblptr + 1) != PRECCMD &&
                 *(tblptr + 1) != PRECUCMD)
                break;					/* Found the end of the stacked prec()'s */
            else
            {
                tblptr += 2;		/* Skip the store # and then next PRECCMD */
            }
        } /* End--load prec()'s into the array */

        if ( prec_cnt > MAX_STACKED_PREC )		/* Too many in a row */
        {
            Process_msg(51, wxNullStr, (long unsigned) MAX_STACKED_PREC);

            /* Only use the first <n>, like we just told the user
            	*/
            prec_cnt = MAX_STACKED_PREC;
            i = 0;
        }

        tp_temp = tblptr;
        tblptr = &prec_array[i];		/* Execute out of the array */

        /* Now try them */
		f = TRUE; 

		if (!curstor || storend[curstor] == storebegin[curstor])
		{
			storeexhausted = TRUE;
			precptr = backinptr;
		}
		else
		{
			storeexhausted = FALSE;
			precptr = storend[curstor];
		}

        for ( prec_index = 0; (prec_index < prec_cnt) && f
                ; prec_index++, tblptr++ )
        {
			if (!storeexhausted)
			{
				if (precptr == storebegin[curstor])
				{
					storeexhausted= TRUE;
					precptr = backinptr;
				}
				else
				{
					precptr--;

					if (utf8encoding || cngletter == PRECUCMD)
					{
						while (precptr > storebegin[curstor] && 
							   ((*precptr) & 0xC0) == 0x80)
							precptr--;

						utf8char = precptr;
					}
				}				
			}

			if (storeexhausted)
			{
				if (precptr == backoutptr) /* BOF condition */
					f = FALSE;
				else
				{
					if (utf8encoding || cngletter == PRECUCMD)
						utf8char = utf8characterfrombackbuffer(&precptr);
					else
						precptr = (precptr > backbuf) ? precptr-1 :  backbuf + BACKBUFMAX - 1;
				}
			}

			if (utf8encoding || cngletter == PRECUCMD)
				f = f && anyutf8check( utf8char );
			else
				f = f && anycheck( *precptr );
        } /* End--try prec()'s */

        tblptr = tp_temp;
        return( f );

    case WDUCMD:														/* Wd */
    case WDCMD:														/* Wd */
        if ( curstor && (storebegin[curstor] != storend[curstor]) )
        {
			precptr= storend[curstor] - 1;

			if (utf8encoding || cngletter == WDUCMD)
			{
				while (precptr > storebegin[curstor] && 
					   ((*precptr) & 0xC0) == 0x80)
					precptr--;

				utf8char = precptr;
			}        
		}
        else
        {
			precptr = backinptr;
			if (precptr == backoutptr) /* BOF condition */
				f = FALSE;
			else
			{
				if (utf8encoding || cngletter == WDUCMD)
					utf8char = utf8characterfrombackbuffer(&precptr);
				else
					precptr = (precptr > backbuf) ? precptr-1 :  backbuf + BACKBUFMAX - 1;
			}
        }

		if (utf8encoding || cngletter == WDUCMD)
	        return( anyutf8check( utf8char ) && anyutf8check(mchptr) );
		else
	        return( anycheck(*precptr) && anycheck(*mchptr) );

    case CONTCMD:													/* Cont */
        return( contcheck() );

    case TOPBITCMD:                    /* High-bit element */
        if ( (*tblptr | HIGHBIT) == (*mchptr++ & 0xffff) )
            return( TRUE );
        else
            return( FALSE );

    default:															/* Bad */
        Process_msg(52, wxNullStr, (long unsigned) cmdname((char)cngletter, TRUE) );
        break;
    }
    return( FALSE);

} /* End--leftexec */

/****************************************************************************/
int CCCModule::anyutf8check(SSINT * mcptr)             /* Is item in the specified store? */
/****************************************************************************/
/* Description:
 *   This procedure checks to see if the argument is equal to any of the
 * characters in the specified store area.  If it is, it returns TRUE,
 * otherwise FALSE.	This procedure executes any(), fol(), prec() & wd().
 *
 * Return values:  TRUE == char equals one of the chars in the specified store
 *						FALSE == no match
 *
 * Globals input: storebegin -- array of pointers to the beginnings of stores
 *						storend -- array of pointers to the ends of stores
 *
 * Globals output: none
 *
 * Error conditions: none
 *
 * Other functions called: none
 *
 */

{
    register SSINT *wptr,           /* Working pointer */
                   *endptr;         /* Pointer to end of store */ 
	register int i;					/* Index into UTF-8 character */
	register int utf8charactersize; /* Size of UTF-8 character */

    endptr = storend[*tblptr];				 /* End of store, for loop termination */
	utf8charactersize = UTF8AdditionalBytes((UTF8)*mcptr) +1;
    for ( wptr = storebegin[*tblptr]; wptr < endptr; wptr+= (UTF8AdditionalBytes((UTF8)*wptr) +1))
    {
        for (i= 0; i < utf8charactersize; i++)
			if ( mcptr[i] != wptr[i])
				break;
		if (i == utf8charactersize)
			return (TRUE);
    } /* End--for */

    return(FALSE);								/* No match -- failure */
} /* End--anyutf8check */

/****************************************************************************/
int CCCModule::anycheck(SSINT mc)                                                /* Is item in the specified store? */
/****************************************************************************/

/* Description:
 *   This procedure checks to see if the argument is equal to any of the
 * characters in the specified store area.  If it is, it returns TRUE,
 * otherwise FALSE.	This procedure executes any(), fol(), prec() & wd().
 *
 * Return values:  TRUE == char equals one of the chars in the specified store
 *						FALSE == no match
 *
 * Globals input: storebegin -- array of pointers to the beginnings of stores
 *						storend -- array of pointers to the ends of stores
 *
 * Globals output: none
 *
 * Error conditions: none
 *
 * Other functions called: none
 *
 */

{
    register SSINT *wptr,           /* Working pointer */
    *endptr;  /* Pointer to end of store */

    endptr = storend[*tblptr];				 /* End of store, for loop termination */

    for ( wptr = storebegin[*tblptr]; wptr < endptr; wptr++ )
    {
        if ( caseless )
        {
            if ( tolower( mc ) == tolower( *wptr ) )
                return( TRUE );					/* Successful caseless match */
        }
        else											/* Case is important */
        {
            if ( mc == *wptr )
                return(TRUE);						/* Successful case-sensitive match */
        }
    } /* End--for */

    return(FALSE);								/* No match -- failure */
} /* End--anycheck */


/************************************************************************/
void CCCModule::freeMem()			/* frees memory allocated during execution */
/************************************************************************/
/* This function is used to free up allocated memory after DoChangesToFile()
   (or the DLL routine CallCCMainPart) has completed. While it cleans
   things up well, it may not be absolutely required, since subsequent
   runs of DoChangesToFile (or DLL routine CallCCMainPart) would use the
   already allocated memory.
   This is not an issue for the DOS EXE version, since the exit to DOS 
	automatically frees all allocated memory.
   note: For DoChangesToString(), which will be called repeatedly, we should
	 probably not free the memory each time. However, if strange errors occur
    on subsequent calls to DoChangesToString(), try freeing the mem each time.
*/    
{
    tblfree();
    storefree();
}

/****************************************************************************/
void CCCModule::tblfree()					/* Free all memory used by tables */ //7.4.15
/****************************************************************************/

/* Description:
 *				Free all memory used by various tables, allocated by
 *					tblcreate() and compilecc() and symbol table compilation
 *
 * All the malloc/free code needs to be tested by a memory debugger
 *
 * Return values: none
 *
 * Globals input:	backbuf -- ring buffer for backup
 *					cngpointers -- pointers to changes in table
 *					match -- match area for searching
 *                  table -- compiled CC table
 *					sym_table --
 *
 * Globals output: Global pointers to the former tables are set to NULL
 *
 * Error conditions: none
 *
 * Other functions called: symb
 *
 */

{
    //allocated by tblcreate()
    if (backbuf != NULL)										//7.4.15
    {
        free (backbuf);
        backbuf = NULL;
    }
    if (cngpointers != NULL)
    {
        free (cngpointers);
        cngpointers = NULL;
    }
    if (match != NULL)
    {
        free (match);
        match = NULL;
    }

    //allocated by compilecc()
    if (table != NULL)
    {
        free (table);
        table = NULL;
    }

    //allocated by symbol table compilation
    discard_symbol_table();

} /* End--tblfree */

/****************************************************************************/
void CCCModule::storefree()		  /*free memory allocated for stores */
/****************************************************************************/
// This function can be called after cc has run one (or more) times.
{
    if (store_area != NULL)
    {
        free (store_area);
        store_area = NULL;
    }
}


 /*****************************************************************************/
void CCCModule::discard_symbol_table()	  /* Discard symbol table */
 /*****************************************************************************/

 /*
 * Description:	Free symbol table
 *
 * Return values: none
 *
 * Globals input:
 *						 sym_table -- symbol table array
 *
 * Globals output:	none
 *
 * Error conditions: none
 *
 * Other procedures called: none
 *
 */

{
    int i;  /* Loop index */
    register CC_SYMBOL *sym_pntr;  /* Pointer for traversing the lists */
    register CC_SYMBOL *temp_pntr;

    /* For all 4 symbol tables, loop through freeing all entries */
    for (i = 0; i < 4; i++ )
    {
        sym_pntr = sym_table[i].list_head;
        while ( sym_pntr != NULL )
        {
            free(sym_pntr->name);
            temp_pntr = sym_pntr;
            sym_pntr = sym_pntr->next;
            free(temp_pntr);
        }
        sym_table[i].list_head = NULL;
    } /* End--for */

} /* End--discard_symbol_table */

// whm revised 31Mar08 and simplified to use wxFile methods for file i/o.
// whm Note: Original version used the now-archaic OpenFile() function.
// MSDN says re the OpenFile function, "you cannot use the OpenFile 
// function to open a file whose path length exceeds 128 characters. 
// The CreateFile function does not have such a path length limitation."
// This limitation is likely the reason Bruce encountered the problem
// of too long path names for .cct table files and wrote the hack of
// copying the .cct table to a location with a shorter path.
// Although they recommend using CreateFile() I am implementing a more 
// portable cross-platform solution via wxWidgets' wxFile class.
WFILE * CCCModule::wfopen(wxString pszFileName, wxString mode)
{
// whm Note: the only actual access mode characters used in the cc source are:
// r   Opens for reading. If the file does not exist or cannot be found, the 
//     fopen call fails.
// a   Opens for writing at the end of the file (appending) without removing 
//     the EOF marker before writing new data to the file; creates the file 
//     first if it doesn't exist.
// w   Opens an empty file for writing. If the given file exists, its contents 
//     are destroyed.
// rb  Open [for reading, see r] in binary (untranslated) mode; translations 
//     involving carriage-return and linefeed characters are suppressed.
// ab  Open [for writing at end of file - see a] in binary (untranslated) mode; 
//     translations involving carriage-return and linefeed characters are 
//     suppressed.
// wb  Open [an empty file for writing - see w] in binary (untranslated) mode; 
//     translations involving carriage-return and linefeed characters are 
//     suppressed.

// Note: "rb" is the only mode actually used in wfopen calls in the present 
// CCCModule. In the 'b' mode, any carriage-return and linefeed characters 
// are read as is for the given platform.
// 
// The possible wxWidgets' wxFile::OpenMode values are:
//   wxFile::read          Open file for reading or test if it can be opened 
//                         for reading with Access(). Use for 'r' or 'rb' mode.
//   wxFile::write         Open file for writing deleting the contents of the 
//                         file if it already exists or test if it can be 
//                         opened for writing with Access(). Use for 'w' or
//                         'wb' mode.
//   wxFile::read_write    Open file for reading and writing; can not be used 
//                         with Access().
//   wxFile::write_append  Open file for appending: the file is opened for 
//                         writing, but the old contents of the file is not 
//                         erased and the file pointer is initially placed at 
//                         the end of the file; can not be used with Access(). 
//                         This is the same as wxFile::write if the file 
//                         doesn't exist. Use for 'a' or 'ab' mode.
//   wxFile::write_excl    Open the file securely for writing 
//                         (Uses O_EXCL | O_CREAT). Will fail if the file 
//                         already exists, else create and open it atomically. 
//                         Useful for opening temporary files without being 
//                         vulnerable to race exploits.
// whm: In the original declaration of the WFILE struct (see CCModule.h) the hfile 
// member was defined as pointer to wxFile (wxFile* hfFile) when defined(UNIX), but 
// as a reference to the actual wxFile (wxFile hfile) otherwise. I have defined 
// hfFile the same way for both UNIX or non-unix ports, which enable us to get
// rid of a number of #if defined(UNIX) ... #else...#endif conditional defines.
    WFILE * stream;
	stream = (WFILE *)NULL;

    wxFile::OpenMode wxmode;
	wxString strMode(mode);
    wxFileOffset dist;
    wxString strFileName;
	strFileName = pszFileName;

	if (strMode == _T("rb") || strMode == _T("r"))
		wxmode = wxFile::read;
	else if (strMode == _T("ab") || strMode == _T("a"))
		wxmode = wxFile::write_append;
	else if (strMode == _T("wb") || strMode == _T("w"))
		wxmode = wxFile::write;
	else
		return NULL; // mode not valid - abort wfopen

    stream = (WFILE *)malloc(sizeof(WFILE));

	// whm: In the wx version we need to create the file on the heap and assign its pointer to
	// stream->hfFile. The memory allocated is deleted in wfclose() to avoid memory leaks.
	wxFile* f;
	f = new wxFile;
	stream->hfFile = f;

    stream->iBufferFront= UNGETBUFFERSIZE;
    stream->iBufferEnd= UNGETBUFFERSIZE;
    stream->fEndOfFile= FALSE;

    if (mode == _T("b"))
        stream->fTextMode= FALSE; // fTextMode is also used in wgetc
    else
        stream->fTextMode= TRUE; // fTextMode is also used in wgetc
	
	bool bOpenOK;
	bOpenOK = FALSE;
	//bool bFileAlreadyExists;
	//bFileAlreadyExists = FALSE;
	bOpenOK = stream->hfFile->Open(strFileName, wxmode);
	if (!bOpenOK)
	{
		free(stream); 
		stream= NULL;
		// Any file opening error message should be done in the caller of wfopen.
		return stream;
	}

    if (wxmode == wxFile::write_append)
    {
		dist = stream->hfFile->Seek(0, wxFromEnd);
		dist = dist; // avoid "set but not used" warning 
    }

    if (wxmode == wxFile::write)
        stream->fWrite= TRUE; // check if fWrite is needed elsewhere in wx version
    else
        stream->fWrite= FALSE; // check if fWrite is needed elsewhere in wx version
    return stream;
}

int CCCModule::wfclose(WFILE * stream)
{
// whm modified 31Mar08 to use wxWidgets' wxFile methods
    bool bCloseOK;
    wfflush(stream);
    bCloseOK =  stream->hfFile->Close();
	delete stream->hfFile;
    free(stream);
    if (!bCloseOK)
    {
        wxMessageBox(_("Error closing file"),_("Consistent Changes"), wxICON_ERROR | wxOK);
        return EOF;
    }
    return 0;
}

/****************************************************************************/
void CCCModule::startcc()									/* Start up CC */
/****************************************************************************/

/* Description:
 *                Set everything up for CC.
 *
 * Return values: none
 *
 * Globals input: switches -- array of booleans, each representing a switch
 *						storebegin -- array of pointers to the beginnings of stores
 *						storend -- array of pointers to the ends of stores
 *						curstor -- if storing, the number of the currently
 *										 active store
 *						storeoverflow -- boolean: TRUE == overflow has occurred
 *																		in a store
 *						eof_written -- boolean: TRUE == CC has output EOF
 *						notable -- boolean: TRUE == no change table
 *						backinptr -- input pointer for the ring buffer
 *						backoutptr -- output pointer for the ring buffer
 *						backbuf -- pointer to the beginning of the ring buffer
 *						table -- array containing the change table
 *
 * Globals output: switches -- all entries set to FALSE
 *						 storebegin -- all entries set to store_area
 *						 storend -- all entries set to store_area
 *						 curstor -- set to zero
 *						 storeoverflow -- set to FALSE
 *						 eof_written -- set to FALSE
 *						 setcurrent -- set to FALSE
 *						 backinptr -- set to the beginning of the ring buffer
 *						 backoutptr -- set to the beginning of the ring buffer
 *						 backbuf -- entire ring buffer set to SPACEs
 *
 * Error conditions: none
 *
 * Other functions called: bytset -- set a block of memory to the given
 *													one-byte value
 *                         ssbytset -- set a block of memory to the given
 *                                     two-byte value
 *                           groupinclude -- add a group to the list currently
 *															being searched
 *
 */

{
    unsigned storemax;					/* Size of store area */
    register int i;		/* Loop index for initializing storage pointers */

    bytset( switches, FALSE, MAXARG+1);					/* Clear all switches */

    if (store_area != NULL)
        free(store_area);
    storemax = max_heap() / sizeof(SSINT);   /* Get as much space as available */
    store_area = (SSINT *) tblalloc(storemax, (sizeof(SSINT)));  /* Allocate store area */
    storelimit = store_area + storemax;        /* Initialize storelimit */
    for (i = 0; i <= NUMSTORES; i++)				/* Initialize storage pointers */
        storebegin[i] = storend[i] = store_area;

    curstor = 0;										/* Clear storing and overflow */
    storeoverflow = FALSE;
    iStoreStackIndex = 0;

    setcurrent = FALSE;									/* Letterset not current */

    eof_written = FALSE;						/* We haven't written EOF yet */

    numgroupson = 0;
    if (groupbeg[1] != 0)
        groupinclude(1);						/* Start in group 1, if it exists */

    if (notable)								/* If no table show, no groups on */
        numgroupson = 0;

    backinptr = backoutptr = backbuf;		 	/* Initialize backup buffer */
    ssbytset( backbuf, (SSINT)' ', BACKBUFMAX);      /* (SPACEs so debug looks ok) */


} /* End--startcc */

/****************************************************************************/
SSINT *CCCModule::ssbytcpy(SSINT *d, SSINT *s, int n)               /* Forward element move */
/****************************************************************************/
{
    memmove(d, s, sizeof(SSINT) * n);
    return( d+n);
}

/****************************************************************************/
char *CCCModule::bytset(char *d, char c, int n)	/* Set n bytes to the specified value */
/****************************************************************************/
{
    memset(d, c, n);
    return( d + n);
}

/****************************************************************************/
SSINT *CCCModule::ssbytset(SSINT *d, SSINT c, int n)  /* Set n elements to the specified value */
/****************************************************************************/
{
    int i;

    for ( i = 0; i < n; i++)
    {
        *d = c;
        d++;
    }
    return( d );
}
/****************************************************************************/
SSINT CCCModule::inputchar()     /* Input a char.  Return <CR> on EOL, <CTRL-Z> on EOF */
/****************************************************************************/

/* Description:
 *						This function inputs a char.	On <END-OF-LINE> it inputs
 *                a CARRIAGERETURN. On EOF it inputs a CTRLZ.
 *
 * Return values: on EOF, return <CTRL-Z>
 *						on EOL, return <CARRIAGERETURN>
 *                otherwise, return input char as an SSINT.
 *
 * Globals input: bEndofInputData -- boolean: TRUE == we're at EOF
 *						infile -- pointer to the input file
 *						filenm -- file name buffer
 *						namelen -- length of file name
 *						argvec -- argvec from command line
 *						inparg -- index into argvec
 *
 * Globals output: bEndofInputData -- updated
 *						 infile -- if we hit EOF on a file, it will either be NULL
 *										 or point to the next file
 *						 filenm -- if we hit EOF on a file, it will either be empty
 *										 or contain the name of the next file
 *						 namelen -- updated
 *						 inparg -- if we hit EOF
 *										 on a file, it will be incremented
 *
 * Error conditions: if we hit EOF, and the requested file was not found
 *							  an error message will be displayed and the user
 *							  will be prompted again
 *
 * Other functions called:
 *
 */

{
    register SSINT ch;  /* Input value from getc */
    SSINT ch2;          /* used to see if we have doublebyte "character" */

    if (bEndofInputData)					/* Return <CTRL-Z> on EOF */
        return(CTRLZ);

L1:

    // If the last character in the previous input buffer was a possible first
    // half of a doublebyte character, then get that back as first input char
    if (bSavedInChar)
    {
        bSavedInChar = FALSE;         // turn off again for next time through
        ch = inSavedChar & 0xFF;      // use last char from previous buffer
    }
    else
    {
        if (nInSoFar == nInBuf)
        {
                if (bPassedAllData)
                    nInBuf= 0;
                else
                {
                    bNeedMoreInput= TRUE;
                    return -1;
                }
            //}
        }

        if (nInBuf == 0)
        {
            bEndofInputData = TRUE;

            if(doublebyte_recursion)
                return(-1);
            else
                return(CTRLZ);
        }

        nInSoFar++;
        ch= (*lpInBuf++) & 0x00ff;
    }

    if ((doublebyte_mode) && (ch >= doublebyte1st)
            // do this so don't get premature efosw and lose final byte in file
            && ( nInSoFar < nInBuf )
            && (!doublebyte_recursion))
    {
        doublebyte_recursion = TRUE;   /* turn on temporarily to prevent further recursion */
        ch2 = inputchar() & 0x00ff;
        doublebyte_recursion = FALSE;  /* reset to its normal state   */
        /* if second byte matches criteria also, then make them a doublebyte,
           unless first is newline, and second is EOF.  */
        if (( ch2 >= doublebyte2nd ) &&
                (( ch != '\n' ) || ( ch2 != 0x00ff )))
        {
            ch = ch * 256 + ch2;                /* treat two bytes as one   */
            return(ch);                         /* return doublebyte "char" */
        }
        else
        {
            if (ch2 != 0x00ff)          /* do not put char back if hit eof   */
            {
                lpInBuf--;               // "put back" char for the next call
                nInSoFar--;              // reset buffer position counter again
            }
        }
    }
    else
    {
        // if we are in doublebyte mode and this is the last character in
        // the input buffer and it is a possible first byte of a doublebyte
        // pair then store it, and denote that we have stored it so that
        // we can pick it up first thing the next time through here
        // (only do this if bPassedAllData says there will be a next time thru).
        if ((doublebyte_mode) && (ch >= doublebyte1st)
                && (!bPassedAllData)
                && ( nInSoFar == nInBuf ) && (!doublebyte_recursion))
        {
            inSavedChar = (char) ch;    // save the final character in buffer
            bSavedInChar = TRUE;        // denote we have saved an input char
            bEndofInputData = TRUE;     // denote that we are done with buffer
            nInSoFar++;                 // count this last char
            lpInBuf++;                  // point beyond this last character
        }
    }

    if (binary_mode)
        return(ch);
    else
    {
        switch (ch)					/* Process the character */
        {
        case '\0':							/*   Ignore NULs, because they will hang
            								  * the new font system, which deals with
            								  * NUL-terminated strings
            								  */
            // in DLL case if we have exceeded buffer just return NUL (zero)
            if ( nInSoFar > nInBuf )
                return(ch);

        case CARRIAGERETURN:				/* Ignore carriage returns */
            goto L1;
        case '\n':							/* use line feed as NEWLINE character */
            /* if possible second byte of a doublebyte
               character (doublebyte_recursion TRUE)
               is newline, then return it immediately */
            if (doublebyte_recursion)
                return('\n');            /* we want this kept as is   */

            if ((nInSoFar < nInBuf)
                    && ((ch = (SSINT)(*lpInBuf++ & 0x00ff)) == EOF))
            {
                nInSoFar++;                   // denote we read another byte
            }
            else
                lpInBuf--;                    // back up in buffer to this byte

            return(CARRIAGERETURN);
        default:
            return(
                      ch);
        }
    }
} /* End--inputchar */

int CCCModule::UCS4toUTF8(UCS4 c, UTF8 * pszUTF8)
{
    int count;
    UTF8 UTF8Buffer[UTF8SIZE];

    count= 1;

    if (c <= 0x7F)
        pszUTF8[0]= (UTF8) c;
    else
    {
        UCS4 prefix= 0xC0;

        do
        {
            UTF8Buffer[UTF8SIZE - count]= (UTF8)(0x80 | (c & 0x3F));
            c= c >> 6;
            count++;
            prefix= (prefix >> 1) | 0x80;
        } while (c  > ((~prefix) & 0xFF) );

        UTF8Buffer[UTF8SIZE - count]= (UTF8)(c | (prefix << 1));
        memcpy(pszUTF8, &UTF8Buffer[UTF8SIZE - count], count);
    }

    return count;
}

int CCCModule::UTF8AdditionalBytes(UTF8 InitialCharacter)
{
    return bytesFromUTF8[InitialCharacter];
}

SSINT * CCCModule::utf8characterfrombackbuffer(SSINT** buffer)
{
	static SSINT utf8character[MAXUTF8BYTES];
	SSINT * bptr;
	SSINT * charptr;

	bptr = *buffer;
	charptr = utf8character + MAXUTF8BYTES;

	do 
	{
		bptr = bptr > backbuf ? bptr - 1 : backbuf + BACKBUFMAX - 1;
		*--charptr = *bptr;
	} while ((*bptr & 0xC0) == 0x80 &&
			 bptr != backoutptr);

	*buffer = bptr;
	
	return charptr;
}

/****************************************************************************/
int CCCModule::contcheck()		 /* Is next part of match == contents of store? */
/****************************************************************************/

/* Description:
 *   This procedure tests the next part of the match to see if it is equal to
 * the content of the specified store.  If it is, it returns TRUE,
 * otherwise FALSE.
 *
 * Return values:  TRUE == current input matches contents of the
 *									  specified store
 *						FALSE == no match
 *
 * Globals input: mchptr -- pointer to the current position within the
 *										potential match
 *						storebegin -- array of pointers to the beginnings of stores
 *						storend -- array of pointers to the ends of stores
 *
 * Globals output: mchptr -- if everything matched, set to the address of the
 *										 next char beyond the match
 *
 * Error conditions: none
 * 
 * Other functions called: none
 *
 */

{
    register SSINT *mp,      /* Pointer to input being matched */
    *tp,	 /* Pointer to contents of store being matched */
    *tp9;  /* Pointer to the end of the store being matched */

    mp = mchptr;								/* Match pointer */

    tp9 = storend[*tblptr];					/* End of store, for loop termination */

    for ( tp = storebegin[*tblptr]; tp < tp9;  tp++ )
    {
        if ( caseless )
        {
            if ( tolower( *mp ) != tolower( *tp ) )
                return( FALSE );								/* Caseless failure */
        }
        else
        {
            if ( *mp != *tp )
                return( FALSE);								/* Case-sensitive failure */
        }
        mp++;
    } /* End--for */

    mchptr = mp;							/* Update pointer to trial match */

    return( TRUE);								/* This part matched */
} /* End--contcheck */


/************************************************************************/
void CCCModule::incrstore(int j)			/* Increment ASCII number in store */
/************************************************************************/
/*
 * Description -- If the store is being used in a match
 *                   Say that the letter set for matches is not current.
 *
 *                Get a local copy of the beginning address of the store.
 *
 *                Go through the store, starting at the right-most char
 *                   If char is ASCII 9
 *                      Set char to ASCII 0.
 *                   Else
 *                      Increment char.
 *                      Return.
 *
 *                (we must have overflowed)
 *
 *                Store another zero as the right-most digit.
 *                Put a 1 in as the left-most digit.
 *
 * Return values: none
 *
 * Globals input: storeact -- array of pointers for stores being used
 *                              for matching
 *                setcurrent -- boolean: TRUE == letter set for matches is
 *                                                 up to date
 *                storebegin -- array of pointers to the beginning of stores
 *                storend -- array of pointers to the end of stores
 *
 * Globals output: setcurrent -- if the store is being used in matching,
 *                                 set to FALSE
 *
 * Error conditions: if we ran out of room in the store area,
 *                     storeoverflow will be set to TRUE
 *                   if the store was being used in matching,
 *                     setcurrent will be set to FALSE
 *
 * Other functions called: storch -- add a char to a store
 *
 */
{

    SSINT *tb;            /* Local copy of the beginning address of the store */
    register SSINT *tp;   /* Working pointer */

    if ( storeact[j] )		/* Is this store being used for matching? */
        setcurrent = FALSE;

    tb = storebegin[j];		/* Get a local copy of beginning address */

    for ( tp = storend[j]; --tp >= tb; )		/* Go through the store, */
    {													/*  right to left */
        if ( *tp != '9' )
        {						/* Increment digit */
            (*tp)++;
            return;
        }
        else
            *tp = '0';			/* Make 9 into 0 */
    }

    /* Overflow has occured when for exits normally */

    storch(j, (SSINT)'0');         /* Add the right-most zero */

    *tb = '1';					/* Insert a 1 as the new left-most digit */

} /* End--incrstore */


/************************************************************************/
void CCCModule::decrstore(int j)                /* Decrement ASCII number in store */
/************************************************************************/
/*
 * Description -- If the store is being used in a match
 *                   Say that the letter set for matches is not current.
 *
 *                Get a local copy of the beginning address of the store.
 *
 *                Go through the store to see if is all char ASCII 0's
 *                   If so then print appropriate warning message, return.
 *
 *                Go through the store, starting at the right-most char
 *                   If char is ASCII 0
 *                      Set char to ASCII 9.
 *                   Else
 *                      Decrement char.
 *                      Return.
 *
 *
 * Return values: none
 *
 * Globals input: storeact -- array of pointers for stores being used
 *                              for matching
 *                setcurrent -- boolean: TRUE == letter set for matches is
 *                                                 up to date
 *                storebegin -- array of pointers to the beginning of stores
 *                storend -- array of pointers to the end of stores
 *
 * Globals output: setcurrent -- if the store is being used in matching,
 *                                 set to FALSE
 *
 * Error conditions: if the store was being used in matching,
 *                     setcurrent will be set to FALSE
 *
 */
{

    SSINT *tb;            /* Local copy of the beginning address of the store */
    register SSINT *tp;   /* Working pointer */

    if ( storeact[j] )		/* Is this store being used for matching? */
        setcurrent = FALSE;

    tb = storebegin[j];        /* Get a local copy of beginning address */

    tp = storend[j];

    tp--;                      /* get to first character.  */

    if (tp < tb)
    {
        Process_msg(12, wxNullStr, 0);
    }
    /* Go through the store from the right end
       until we get to the leftmost character or
       the first non-zero.  Then check that character
       (be it a non-zero or the leftmost character)
       to see if we have totally zeroes.          */
    while ((tp > tb) && (*tp == '0'))              /* Go through the store  */
    {
        tp--;
    }

    if (*tp == '0')                                 /* Is store all zeros? */
    {
        Process_msg(13, wxNullStr, 0);
        return;
    }

    for ( tp = storend[j]; --tp >= tb; )		/* Go through the store, */
    {													/*  right to left */
        if ( *tp != '0' )
        {                               /* Decrement digit */
            (*tp)--;
            return;
        }
        else
            *tp = '9';                      /* Make 0 into 9 */
    }

} /* End--decrstore */

/****************************************************************************/
void CCCModule::ccmath(SSINT oprtor, SSINT **tblpnt)   /* Perform a mathematical operation in CC */
/****************************************************************************/
/*
 * Description:
 *						This function will perform the CC mathematical functions
 *					add, sub(tract), mul(tiply), div(ide), and mod(ulo).
 *
 * Return values: the store specified by the command will contain the
 *						  result of the operation.
 *
 * Globals input: storeact -- array of flags for stores being used
 *											for matching
 *						setcurrent -- boolean: TRUE == letter set for matches is
 *																	up to date
 *						cgroup -- group the currently-executing change is in
 *
 * Globals output: setcurrent -- if the destination store for the operation
 *												is being used in matching, set to FALSE
 *
 * Error conditions: If a command has a non-numeric argument, the store
 *							specified in the command will be unchanged.
 *
 * Other procedures called:
 *									long_abs -- absolute value of a long int
 *									valnum -- validate a numeric argument
 *
 */

{
    char sign_check,        /* Flag byte for signs of the operands */
    operation;          /* What we're doing (+, -, *, or /) */
    register SSINT *tp;     /* Copy of the table pointer */
    long int first_operand,
    second_operand,
    l_tmp_1,
    l_tmp_2,			/* Used to store absolute values of operands,
    					*	 because DECUS C doesn't correctly compare
    					*	 the results of long int functions
    					*/
    l_temp;			/* Used in checking for overflow */
    int store,					/* Store used for the first operand */
    valcode1, valcode2,  /* Return values from validation routine */
    i;

    char num_buf[12];      /* Buffer for converting result back to ASCII
    						*	(10 digits, sign, and string-terminating NUL)
    						*/
    char  op1_buf[20],
    op2_buf[20];     /* Buffers for the two operands for valnum */

    switch ( oprtor )
    {
    case ADDCMD:
        operation = '+';
        break;
    case SUBCMD:
        operation = '-';
        break;
    case MULCMD:
        operation = '*';
        break;
    case DIVCMD:
        operation = '/';
        break;
    case MODCMD:
        operation = '%';
        break;
    default:
        operation = '?';			/* How did we get here then? */
    } /* End--switch */

    tp = *tblpnt;			/* Get a working copy of the table pointer */
    store = *(tp);			/* Get the store # for the destination */
    if ( storeact[store] )		/* Is this store being used for matching? */
        setcurrent = FALSE;

    /* Validate the operands */
    valcode1 = valnum( tp, op1_buf, 20, &first_operand, TRUE);
    tp++;											/* Move to the second operand */
    valcode2 = valnum( tp, op2_buf, 20, &second_operand, FALSE);
    if ( (!valcode1) || (!valcode2) )
    {
        math_error( op1_buf, op2_buf, operation, NON_NUMBER, store );
        goto DONE;
    }
    if ( (valcode1 == -1) || (valcode2 == -1) )
    {
        math_error( op1_buf, op2_buf, operation, BIG_NUMBER, store );
        goto DONE;
    }

    /* Check signs of both operands */
    if ( first_operand < 0L )		/* Get sign of the first operand */
        sign_check = 0x2;
    else
        sign_check = 0;
    if ( second_operand < 0L )		/* Get the sign of the second operand */
        sign_check |= 0x1;

    /* Perform the operation */
    switch( oprtor )
    {
    case ADDCMD:
        if ( (sign_check != 1) && (sign_check != 2) )
        {												/* signs are the same */
            l_tmp_1 = long_abs( first_operand );
            l_tmp_2 = long_abs( second_operand );
            l_temp = l_tmp_1 + l_tmp_2;
            if ( (l_temp < 0L) || (l_temp > 1999999999L) )
            {
                math_error( op1_buf, op2_buf, operation, CCOVERFLOW, store );
                break;
            }
        }
        first_operand = first_operand + second_operand;
        break;

    case SUBCMD:
        if ( (sign_check != 0) && (sign_check != 3) )
        {									/* signs are different */
            l_tmp_1 = long_abs( first_operand );
            l_tmp_2 = long_abs( second_operand );
            l_temp = l_tmp_1 + l_tmp_2;
            if ( (l_temp < 0L) || (l_temp > 1999999999L) )
            {
                math_error( op1_buf, op2_buf, operation, CCOVERFLOW, store );
                break;
            }
        }
        first_operand = first_operand - second_operand;
        break;

    case MULCMD:
        l_tmp_1 = long_abs( first_operand );
        l_tmp_2 = long_abs( second_operand );
        l_temp = l_tmp_1 * l_tmp_2;
        if ( (l_temp < l_tmp_1) && (l_temp < l_tmp_2) )
        {
            math_error( op1_buf, op2_buf, operation, CCOVERFLOW, store );
            break;
        }
        first_operand = first_operand * second_operand;
        break;

    case DIVCMD:
        if ( second_operand == 0L )
        {
            math_error( op1_buf, op2_buf, operation, DIVIDE_BY_ZERO, store );
            break;
        }
        first_operand = first_operand / second_operand;
        break;

    case MODCMD:
        if ( second_operand == 0L )
        {
            math_error( op1_buf, op2_buf, operation, DIVIDE_BY_ZERO, store );
            break;
        }
        first_operand = first_operand % second_operand;
        break;
    } /* End--switch */

    /* Store the result */
    sprintf( num_buf, "%ld", first_operand );     /* Convert to ASCII */
    storend[store] = storebegin[store];

    for (i = 0; num_buf[i] != '\0'; i++)     /* put string into store */
        storch(store, (SSINT) num_buf[i]);

    /* Skip to the next command */
DONE:
    tp = (*tblpnt) + 1;				/* Skip the store # */
    if ( *tp != CONTCMD )
    {												/* Second operand is a string */
        for ( ; !(*tp & HIGHBIT) || (*tp == TOPBITCMD); tp++ )
            ;
    }
    else
        tp += 2;						/* Skip cont and its store # */

    *tblpnt = tp;			/* Update the pointer before returning */

} /* End--ccmath */

/****************************************************************************/
void CCCModule::math_error(char *op_1, char *op_2, char operation, int err_type, int storenum)
/****************************************************************************/
/*
 * Description:	Print a warning message on screen.
 *
 * Globals input: cgroup -- group the currently-executing change is in
 *
 */

{
    MSG_STRUCT_S_S *structss;
    MSG_STRUCT_S_C_S *structscs;

    if ( *op_1 == '\0' )
        op_1 = zero;
    if ( *op_2 == '\0' )
        op_2 = zero;

    Msg_s_s.string1 = err_math[err_type];
    Msg_s_s.string2 = wxString(sym_name(cgroup, GROUP_HEAD),wxConvISO8859_1); // whm: TODO: check this!!!
    structss = &Msg_s_s;
    Process_msg(19, wxNullStr, (long unsigned) structss);
    Msg_s_c_s.string1 = wxString(op_1,wxConvISO8859_1);// whm: TODO: check this!!!
    Msg_s_c_s.char1 = operation;
    Msg_s_c_s.string2 = wxString(op_2,wxConvISO8859_1); // whm: TODO: check this!!!
    structscs = &Msg_s_c_s;
    Process_msg(20, wxNullStr, (long unsigned) structscs);
    Process_msg(6, wxNullStr, (long unsigned) sym_name(storenum, STORE_HEAD));
}

/****************************************************************************/
long int CCCModule::long_abs(long value)		/* Absolute value of a long int */
/****************************************************************************/
//long int value;
{
    if ( value < 0L )
        return( -(value) );
    else
        return( value );
} /* End--long_abs */


/************************************************************************/
unsigned int CCCModule::max_heap() /* Compute size of remaining heap space */
/************************************************************************/
/*
 * DESCRIPTION: Repeatedly calls 'malloc' and 'free' until it has found
 * the largest malloc-able buffer.  The size of this buffer is
 * returned.  The buffer is freed.
 *
 * RETURN VALUES: Returns an UNSIGNED int containing size (in bytes) of
 * largest malloc-able buffer.
 *
 * GLOBALS INPUT:		none
 * GLOBALS OUTPUT:		none
 * ERROR CONDITIONS:	none
 */
/************************************************************************/
{
    char *buffer;                    /* buffer pointer */
    unsigned int high, low, t;       /* loop controls  */

    //for computers of the 1990's, try MAX_ALLOC (nearly 64K) first,
    // and use that size if successful
    if (
        (buffer = (char *) malloc((size_t)MAX_ALLOC)) != NULL)			//7.4.15
    {
        free (buffer);
        return MAX_ALLOC;
    }
    //try various smaller sizes
    high = MAX_ALLOC;           /* Highest amount to request */
    low = 0;                    /* Lowest amount */
    while ( low < (high-1) )    /* Loop until only 1-byte difference */
    {
        t = low + (high-low)/ 2; /* Compute trial buffer size */
        if ((buffer = (char *)malloc(t)) != NULL) /* Attempt to allocate that size */
        {						/* If successful... */
            free(buffer);		/* ...free the buffer and... */
            low = t;				/* ...raise the 'low' value */
        }
        else						/* else... */
            high = t;			/* ...lower the 'high' value */
    }

    /* When finished, the 'low' value is the largest malloc-able buffer */
    /* Subtract out RESERVEDSPACE so as not to totally exhaust heap. Bail out
    if there is too little heap space available. */
    if ( low > (unsigned) RESERVEDSPACE )
        low -= RESERVEDSPACE;	/* Leave space for file buffers */
    /*  and prntr's tables */
    else
    {
        Process_msg(37, wxNullStr, (unsigned long) low);
        bailout(BADEXIT, FALSE);
    }

    return(low);
} /* End--max_heap */

/************************************************************************/
void CCCModule::bailout(int WXUNUSED(exit_code), int WXUNUSED(ctrlc_flag))		/* Bail out with given exit code */
/************************************************************************/
//int exit_code;		/* Exit code */
//int ctrlc_flag;	/* TRUE if user is bailing out by pressing ^C */
{
	/* whm: removed bailout
	*/
}

/****************************************************************************/
void CCCModule::storechar(SSINT ch)		  /* Store a char in the internal table */
/****************************************************************************/
/* Description -- If there is room in the table
 *							Store ch in the table.
 *						Else
 *							Set the boolean tblfull to TRUE.
 *
 * Return values: none
 *
 * Globals input: tloadpointer -- pointer for storing into table
 *                tablelimit -- address of last element of table
 *						tblfull -- boolean indicating whether there's room
 *										 left in table
 *
 * Globals output: tloadpointer -- if character was successfully stored,
 *												 it will point to the next position
 *												 in table, otherwise it will be unchanged
 *
 *						 tblfull -- if the table fills up, it will be set to TRUE.
 *
 * Error conditions: if table fills up, tblfull will be set to TRUE
 *
 * Other functions called: none
 *
 */

{
    if ( tloadpointer < tablelimit )
        *tloadpointer++ = ch;			/* Store the char in the table */
    else
        tblfull = TRUE;					/* Set error flag */
} /* End--storechar */

/****************************************************************************/
int CCCModule::srchlen(register tbltype tp1) /* Get length of match of change */
/****************************************************************************/

/* Description -- Go through the table, starting at tp1, until we find
 *						a wedge, return the length from tp1 to the wedge. Count
 *						each full character as SRCHLEN_FACTOR, count any() as
 *						SRCHLEN_FACTOR, count prec() or fol() as 1, and count wd()
 *						and cont() as 2.
 *
 * Return values: returns length of the match.
 *
 * Globals input: none
 *
 * Globals output: none
 *
 * Error conditions: none
 *
 * Other functions called: none
 *
 */

{
    register tbltype tp2;  /* Pointer for going through the table */
    register int length;   /* Length to be returned */

    length = 0;

    for ( tp2 = tp1; *tp2 != SPECWEDGE; tp2++)  /* Find a wedge */
    {
        switch (*tp2)
        {
        case FOLCMD:		/* fol(): count as 1, skip parameter byte */
        case PRECCMD:		/* prec(): count as 1, skip parameter byte */
        case FOLUCMD:		/* folu(): count as 1, skip parameter byte */
        case PRECUCMD:		/* precu(): count as 1, skip parameter byte */
            length++;
            tp2++;
            break;

        case TOPBITCMD:   /* Top bit set: count as 1 full element */
        case ANYCMD:		/* any(): count as 1 full char, skip param. byte */
        case ANYUCMD:		/* anyu(): count as 1 full char, skip param. byte */
            length+=SRCHLEN_FACTOR;
            tp2++;
            break;

        case CONTCMD:		/* cont(): count as 2, skip parameter byte */
        case WDCMD:			/* wd(): count as 2, skip parameter byte */
        case WDUCMD:			/* wd(): count as 2, skip parameter byte */
            length+=2;
            tp2++;
            break;

        default:				/* ordinary character: count as 1 full character */
            length+=SRCHLEN_FACTOR;
        }
    }

    return( length );  /* Return the length from tp1 to the wedge */

} /* End--srchlen */

/****************************************************************************/
bool CCCModule::wedgeinline()			 /* Is there a WEDGE in the line? */
/****************************************************************************/

/* Description -- Set 2 local pointers to point to the beginning of line.
 *
 *						Parse each element of line until we find either
 *						  a WEDGE or end-of-line.
 *
 *						If we found end-of-line
 *							Return FALSE.
 *						Else	(we must have found a WEDGE)
 *							Return TRUE.
 *
 *  Note:  Parse must be used rather than simply checking for a WEDGE
 *				 because otherwise we might find a WEDGE that was actually
 *				 inside a quoted string.
 *
 * Return values:  TRUE == There is a > in line
 *						FALSE == There is no > in line
 *
 * Globals input: line -- current input line
 *
 * Globals output: none altered
 *
 * Error conditions: none
 *
 * Functions called: parse -- get the next logical group from the input line
 *
 */

{
    char *wp, *wp2;			/* pointers used for parse calls */

    wp = wp2 = line;
    while ( *wp && *wp != WEDGE )  /* Look for either a WEDGE or end-of-line */
        parse( &wp, &wp2, FALSE);
    return( *wp != '\0' );

} /* End--wedgeinline */

/****************************************************************************/
void CCCModule::flushstringbuffer(int search)
/****************************************************************************/
/*
 *   Note: This written by DAR 4/96
 *
 */
{
    int sLen;

    if (pstrBuffer != strBuffer)
    {
        sLen = pstrBuffer - strBuffer;
        storestring(search, strBuffer, sLen);
        pstrBuffer = strBuffer;
    }
    else
        // if zero-length string, still denote it wi a string.    7/96
        was_string = TRUE;
} /* End--flushstringbuffer */

/****************************************************************************/
void CCCModule::parse( char **pntr, char **pntr2, int errormessage) /* Parse an element of a line */
/****************************************************************************/
//char **pntr,	/* Pointer to pointer to beginning of element found */
// *pntr2;	/* Pointer to pointer to character just beyond element */
//int errormessage;		/* Boolean: TRUE == output an error message if
//							 *							 an unmatched quote is found
//							 */

/* Description:
 *						Parse an element of a line in the change table, skipping
 *					over white space (TAB, SPACE, FF).	Handle quoted strings, but
 *					do not allow a line break inside a quoted string.
 *
 * Return values: *pntr points to the beginning of the element found
 *						*pntr2 points to the character just beyond the end
 *						  of the element
 *
 * Globals input: line -- current input line
 *
 * Globals output: none altered
 *
 * Error conditions: an unmatched quote will give *pntr pointing to the
 *							  open quote and *pntr2 pointing to the \0 at the
 *							  end of line.
 *							*pntr == *pntr2 implies that nothing was found
 *													(blank line or a comment)
 *
 * Other functions called: err -- display an error message
 *
 */

{
    register char *lp,	 /* Pointer to beginning of element */
    *lp2;   /* Pointer to next char beyond end of element */

    lp = *pntr2;								/* Set starting pointer */

    while ( (*lp == ' ') || (*lp == '\t') )		/* Skip spaces */
        lp++;
    lp2 = lp;

    if ( *lp == '"' || *lp == '\'' )			/* Quoted string */
    {
        while ( *(++lp2) && (*lp != *lp2) )			/* (skip to end of string) */
            ;
        if ( *lp2 )		/* Point lp2 to next char beyond the end of element */
            lp2++;
        else
            if ( errormessage )
            {
                err( "Unmatched quote");
            }
    }
    else if (*lp == WEDGE)							/* A wedge */
        lp2++;		/* end of wedge is next character */
    else if (*lp == '(')
    {
        if ( errormessage )
        {
            err("Parenthesized parameter(s) must be adjacent to a keyword");
        }
        while ( *(++lp2) && (*lp2 != ')') )		/* Skip to closing paren */
            ;
        if (*lp2 == ')')
            lp2++;			/* Skip past closing paren (if any) */
    }
    else												/* Keyword */
        while ( *lp2										/* (move to end of it) */
                && ( strchr( " \t>'\"", *(++lp2) ) == NULL) )
            ;

    if ( ((lp+1) == lp2) && (toupper(*lp) == 'C') )			/* Comment */
        lp = lp2 = line + strlen(line);					 /*  (skip to end of it) */

    *pntr = lp;
    *pntr2 = lp2;
}/* End--parse */

/****************************************************************************/
bool CCCModule::chk_prec_cmd()
/****************************************************************************/
/* Function: chk_prec_cmd
 *
 * Parameters:  none
 *
 * Description -- handle occurences of prec() command before match string
 *				If a prec() command is found at the beginning of a line,
 *				we wait until after the string or any() command before compiling
 *				the prec() command(s) into memory.
 *
 * Globals input: parsepntr parse2pntr precparse
 *
 * Globals output: parse2pntr precparse
 *
 * Error conditions: none
 *
 * Other functions called: none
 *
 * Return values: TRUE if str points to prec() command
 * Function added 2/95 by BJY for 7.4.30
 */

{
    char *saveparse2;

    if (!strncmp( parsepntr, "prec(", 5))
    {
        if (!precparse)
            precparse = parsepntr;	// Save position of first prec() command for later compilation
        return TRUE;
    }
    else
    {
        storeelement(TRUE);
        if (precparse)
        {
            saveparse2 = parse2pntr;	// Save pointer for later restoration
            parse2pntr = precparse;
            while (1)				// go store prec() command(s) now
            {
                parse( &parsepntr, &parse2pntr, TRUE);
                if (parse2pntr >= saveparse2)
                    break;
                storeelement(TRUE);
            }
            parse2pntr = saveparse2;	// restore position in line
        }
        return FALSE;
    }
}

/****************************************************************************/
void CCCModule::storeelement(int search)	/* Store a parsed element in internal change table */
/****************************************************************************/
//int search;  /* Boolean --  TRUE == we're on the search side of a change, */
/* FALSE == we're on the replacement side */

/* Description:
 *		This procedure stores a parsed element in the internal change table.
 * It recognizes quoted strings, octal numbers, and keywords.	It gives an
 * error message if the element is bad.  It uses search to tell if it is
 * storing a search element or a replacement element.
 *
 * Return values: none
 *
 * Globals input: parsepntr -- pointer to the beginning of the current element
 *						parse2pntr -- pointer to just beyond the current element
 *						keyword -- array used for comparing suspected keywords
 *						fontsection -- boolean: TRUE == the table has a font section
 *						tloadpointer -- pointer to the next empty byte in the table
 *						table -- pointer to the beginning of the table
 *                was_math -- boolean: TRUE == last command was a math command
 *                                               (add, sub, mul, div)
 *                was_string -- boolean: TRUE == last command was a quoted string
 *
 * Globals output: parsepntr -- updated based on what we found
 *						 keyword -- may contain a suspected keyword
 *						 fontsection -- may be set to TRUE if we found the beginning
 *												of the font section of the table
 *						was_math -- updated
 *						was_string -- updated
 *
 * Error conditions: any error found will result in errors being set to TRUE
 *							  and an appropriate error message being displayed
 *
 * Other functions called: storechar -- store an element in the internal
 *														change table
 *									octal_decode -- decode an ASCII octal number
 *									hex_decode -- decode an ASCII hexadecimal number
 *									buildkeyword -- copy a suspected keyword into
 *															the keyword array for checking
 *									storarg -- store a command with a numeric argument
 *									valnum -- validate a numeric argument for
 *													the arithmetic routines
 *									err -- display an error message
 *
 */

{
    int number,             /* General purpose integer variable            */
    last_command;        /* Previous command stored in the table        */
    int ch;                 /* Single character                            */

    struct cmstruct *cmpt;
    int nbytes;

    /* If we have already processed the begin, compilecc should have    */
    /*  stored a SPECWEDGE as the first char of the table.  Otherwise   */
    /*  we have an error */
    if ( begin_found && (tloadpointer == table) )
    {
        err( "Illegal search string in begin statement");
        return;
    }

    if ( *parsepntr == '"' || *parsepntr == '\'' )
    {                                                /* Quoted string   */
        ++parsepntr;
        nbytes= parse2pntr - parsepntr - 1;

        if (nbytes > 0)
        {
            strncpy(pstrBuffer, parsepntr, nbytes);
            pstrBuffer+= nbytes;
        }
    }
    else if ( *parsepntr == '0' )  /* If starts with 0 it must be a number */
    {
        parsepntr++;	/* Skip over the '0' */
        if ( (ch = tolower(*parsepntr)) == 'x' )
        {
            nbytes= hex_decode(pstrBuffer);
            pstrBuffer+= nbytes;
        }
        else if (ch == 'u')
        {
            nbytes= ucs4_decode(pstrBuffer);
            pstrBuffer+= nbytes;
        }
        else
        {
            if (ch == 'd')
                decimal_decode( &number );			/* Decimal code number */
            else
                octal_decode( &number );			/* Octal code number */
            *pstrBuffer++= number & 0xFF;
        }						// moved to fix problem with 0x syntax 4/7/99
    }
    else if ( *parsepntr >= '0' && *parsepntr <= '7')
    {											/* Octal number */
        octal_decode( &number );
        *pstrBuffer++= number & 0xFF;

    }
    else if ( (tolower( *parsepntr ) == 'd') && isdigit( *(parsepntr + 1)) )
    {																/* Decimal number */
        decimal_decode( &number );
        *pstrBuffer++= number & 0xFF;
    }
    else if ( (tolower(*parsepntr) == 'x') && isxdigit( *(parsepntr + 1)) )
    {																/* Hexadecimal number */
        nbytes= hex_decode(pstrBuffer);
        pstrBuffer+= nbytes;
    }
    else if ( (tolower(*parsepntr) == 'u') && isxdigit( *(parsepntr + 1)) )
    {
        nbytes= ucs4_decode(pstrBuffer);
        pstrBuffer+= nbytes;
    }
    else if ( *parsepntr == '(' )	/* Bogus parameter list */
        ;							/* simply ignore */
    else
    {																/* Keyword */
        flushstringbuffer(search);
        buildkeyword();
        cmpt = NULL;
        if ( search )
        {															/* Search side */
            if ( !strcmp(keyword,"begin") )	/* begin */
            {
                if ( tloadpointer == table+1 )
                {
                    tloadpointer--;
                    begin_found = TRUE;					/* We have a begin statement */
                }
                else
                    err( "Begin command not first in table");
            }
            else
                cmpt = cmsrch;						/* Search the search-side array */
        }
        else
        {															/* Replacement side */
            if ( (tloadpointer == table) && !strcmp(keyword,"begin") )
            {
                err( "No wedge on \'begin\' line");
            }
            else if ( !strcmp(keyword,"use") )
            {										/* Use */
                storechar( USECMD );
                storarg( INCLCMD, REFERENCED, GROUP_HEAD );
            }
            else if ( !strcmp(keyword,"font") )
            {												/* Font */
                fontsection = TRUE;
                storechar( SPECPERIOD);						/* Terminate last change */
            }
            else
                cmpt = cmrepl;					 /* Search the replacement-side array */
        }

        if ( cmpt != NULL )
        {
            for ( ; cmpt->cmname; cmpt++ )				/* Look for a match */
                if ( !strcmp(keyword, cmpt->cmname) )
                {
                    number = cmpt->cmcode;       // Get command code for comparisons
                    if (number == READCMD)       // 7.4.15
                        return;
                    if (number != CONTCMD)
                    {
                        if ( was_math )
                            err( "Illegal command following arithmetic operator" );
                        else
                        {
                            if ( tloadpointer >= (table + 2) )
                            {
                                last_command = *(tloadpointer - 2);
                            }
                            else
                            {
                                last_command = 0;			/* Nothing there yet */
                            }

                            if ( !(was_string || number == '\r') &&
                                    ((last_command == IFEQCMD) ||
                                     (last_command == IFGTCMD) ||
                                     (last_command == IFNGTCMD) ||
                                     (last_command == IFLTCMD) ||
                                     (last_command == IFNLTCMD) ||
                                     (last_command == IFNEQCMD) ||
                                     (last_command == IFSUBSETCMD) ))
                                err( "Illegal command following comparison operator" );
                        }
                    } /* End--command was not cont */

                    if ( was_math )
                    {								/* Last command was a math operation */
                        was_math = FALSE;
                    }

                    /*   Store the command,
                    * using the function
                    * from the search table
                    */
                    (*(cmpt->cmfunc))
                    ( cmpt->cmcode, cmpt->symbolic, cmpt->symbol_index );

                    was_string = FALSE;
                    /* Did we just process a math command? */
                    if (	number == ADDCMD ||
                            number == SUBCMD ||
                            number == MULCMD ||
                            number == DIVCMD ||
                            number == MODCMD )
                        was_math = TRUE;
                    else
                        was_math = FALSE;

                    return;
                }

            err( "Unrecognized keyword");
        } /* End--look for a match */
        was_string = FALSE;
    } /* End--keyword */

} /* End--storeelement */

/****************************************************************************/
void CCCModule::fontmsg()	  /* Handle font section appropriately */
/****************************************************************************/

/* Description -- If CC or MS and only doing CC
 *							Display "...ignored..." message.
 *						Else
 *							Display "...not supported..." message.
 *
 *							Set global errors to TRUE.
 *
 * Globals output: errors -- set to TRUE if MS and running full MS
 *
 */
{
    Process_msg(40, wxNullStr, 0);
}

/*****************************************************************************/
void CCCModule::check_symbol_table() /* Check symbol table for possible typos */
/*****************************************************************************/

/*
 * Description:
 *						Check the symbol table for symbols which were referenced
 *						  but not defined, which usually indicates a
 *						  misspelled name.
 *
 * Return values: none
 *
 * Globals input:
 *						 sym_table -- symbol table array
 *
 * Globals output: none
 *
 * Error conditions: If a symbol is found that is referenced but not
 *							  defined, an appropriate warning message will be
 *							  displayed on the screen.
 *
 * Other procedures called: none
 *
 */

{
    int i;  /* Loop index */
    register CC_SYMBOL *sym_pntr;  /* Pointer for traversing the lists */

    for (i = 0; i < 4; i++ )
    {
        if ( (sym_pntr = sym_table[i].list_head) != NULL )
        {
            while ( sym_pntr != NULL )				/* Check the symbol table */
            {
                if ( sym_pntr->use == REFERENCED )
                {
                    switch( i )
                    {
                    case STORE_HEAD:
                        // do not print warning message if we are looking at an
                        // out command with a predefined store
                        if (( storepre[sym_pntr->number] != 0 ) &&
                                (strncmp(sym_pntr->name, "cccurrentdate", 13) == 0 ))
                            break;
                        if (( storepre[sym_pntr->number] != 0 ) &&
                                (strncmp(sym_pntr->name, "cccurrenttime", 13) == 0 ))
                            break;
                        if (( storepre[sym_pntr->number] != 0 ) &&
                                (strncmp(sym_pntr->name, "ccversionmajor", 14) == 0))
                            break;
                        if (( storepre[sym_pntr->number] != 0 ) &&
                                (strncmp(sym_pntr->name, "ccversionminor", 14) == 0))
                            break;
                        Process_msg(32, wxNullStr, (long unsigned) sym_pntr->name);
                        break;
                    case SWITCH_HEAD:
                        Process_msg(33, wxNullStr, (long unsigned) sym_pntr->name);
                        break;
                    case GROUP_HEAD:
                        Process_msg(34, wxNullStr, (long unsigned) sym_pntr->name);
                        break;
                    case DEFINE_HEAD:
                        Process_msg(35, wxNullStr, (long unsigned) sym_pntr->name);
                        break;
                    } /* End--switch */

                } /* End--if (referenced-but-undefined symbol) */

                sym_pntr = sym_pntr->next;
            } /* End--traverse the symbol table */

        } /* End--if non-empty symbol table */

    } /* End--for */

} /* End--check_symbol_table */

/****************************************************************************/
int CCCModule::cctsetup()					/* Set up pointers into table */
/****************************************************************************/
{
    /* Description:
     *						Go through and set up the internal representation of the
     *					changes table.
     *
     * Return values:  TRUE == everything is set up OK
     *						FALSE == there was a problem somewhere
     *
     * Globals input: 
     *						errors -- Boolean: TRUE == table contains errors
     *						fontstart -- table of pointers to the start of fonts
     *						fontsection -- Boolean: TRUE == table contains old-style
     *																	 fonts
     *						maintablend -- pointer to the end of the changes
     *											  part of the table
     *						tloadpointer -- pointer to the end of what was read
     *												into table
     *						table -- storage area for changes and fonts
     *						defarray -- table of pointers to DEFINEs
     *						cngpend -- pointer to the end of the table of
     *										 change pointers
     *						cngpointers -- table of pointers to changes
     *						groupbeg -- table of pointers to the beginning of groups
     *						maxsrch -- length of longest search string
     *						groupend -- table of pointers to the end of groups
     *
     * Globals output: if there were no problems, everybody is set up.
     *						 if there was a serious problem, errors is set to TRUE.
     *
     * Error conditions: if the table was too big, FALSE will be returned.
     *							if group one was missing or a group is multiply defined,
     *							  FALSE will be returned and
     *							  errors will be set to TRUE, indicating a major error.
     *							Other errors, such as a font section when running CC,
     *							  will only generate warning messages for the user.
     *
     * Other functions called: fontmsg -- display an appropriate message because
     *													 we found a font section in the table
     *
     */

    register tbltype tp;
    register tbltype *ptp;
    cngtype cp;
    int group, i, j;

    /* Look for 'unsorted' and 'binary' and 'doublebyte' commands in 'begin'
    section.  Set global flags accordingly */
    unsorted = FALSE;
    binary_mode = FALSE;
    doublebyte_mode = FALSE;
    utf8encoding = FALSE;
    if (table[0] == SPECWEDGE)		/* If there is a begin section */
    {									/* scan through it */
        for (tp = table+1; *tp != SPECPERIOD; tp++)
        {
            if (*tp == UNSORTCMD)
                unsorted = TRUE;
            else if (*tp == BINCMD)
                binary_mode = TRUE;
            else if (*tp == UTF8CMD)
                utf8encoding = TRUE;
            else if (*tp == DOUBLECMD)
            {
                tp++;                      /* point to doublebyte argument */
                *tp = *tp & 0x00ff;        /* turn off any possible high bits */
                doublebyte_mode = TRUE;
                doublebytestore(*tp);      /* pass argument */
                tp--;                      /* restore tp    */
            }
        }
        // binary is incompatible and not needed with doublebyte mode
        if (( binary_mode == TRUE ) && ( doublebyte_mode == TRUE ))
        {
            binary_mode = FALSE;
            Process_msg(99, wxNullStr, 0);
        }

    }

    maintablend = tloadpointer;							/* set main table end */
    for ( tp = table; tp < tloadpointer; tp++ )
    {
        if (*tp == FONTCMD)			/* Set table end to beginning of font section */
        {
            fontmsg();						/* We have old-style fonts */
            maintablend = tp;
            break;
        }
    }

    for (ptp = defarray; ptp <= defarray+MAXARG; )		/* clear define array */
        *ptp++ = NULL;

    cngpend = cngpointers;								/* build changepointers array */

    for ( tp = table; tp < maintablend-1; )
    {
        if ( *tp++ == SPECPERIOD )
        {
            if ( *tp == DEFINECMD )				/* It's a define, so save its addr */
                defarray[*(tp+1)] = tp + 2;	 /*  in define array */
            else
            {
                if ( cngpend - cngpointers < MAXCHANGES )  /* Must be a change */
                    *cngpend++ = tp;								/*  so save its addr */
                else
                {
                    Process_msg(38, wxNullStr, (long unsigned) MAXCHANGES);
                    return( FALSE);						/* Error */
                }
            } /* End--else */
        } /* End--if */
    } /* End--for */

    for (i = 1; i <= MAXGROUPS; i++)  /* Initialize group array */
        groupbeg[i] = NULL;

    group = 0;

    maxsrch = 0;

    for ( cp = cngpointers; cp < cngpend; cp++)
    {
        tp = *cp;							/* Find length of longest search */
        j = srchlen( tp ) / SRCHLEN_FACTOR;
        if ( j > maxsrch )
            maxsrch = j;

        if ( (tp >= table+3) && (*(tp-3) == GROUPCMD) )  /* Valid group start? */
        {
            *(tp-3) = SPECPERIOD;	 /* Eff remove GROUPCMD */

            if ( group != 0 )
            {
                groupend[group] = cp;	/* Close previous group */
            }
            else
            {
                groupbeg[1] = cngpointers;			/* First group is implied group 1 */
                groupend[1] = cp;
            }

            /* If the group has already been defined */
            /*  AND it's non-empty, squawk */

            group = *(tp-2);
            if ( (groupbeg[group] != NULL) && (groupend[group] != groupbeg[group]) )
            {
                Process_msg(39, wxNullStr, (long unsigned) sym_name(group, GROUP_HEAD));
                errors = TRUE;												/* Major error */
                return(FALSE);
            }
            else
                groupbeg[group] = cp;		/* Save the start of the current group */
        }
    }

    if ( group == 0 )					/* If no groups, set up group 1 */
    {
        group = 1;
        groupbeg[1] = cngpointers;
    }
    groupend[group] = cngpend;			/* Close last group */

    // increase maxsrch so debug display window will display at least 500 characters
    if (maxsrch < MINMATCH)
        maxsrch= MINMATCH;

    return( TRUE);		/* No errors detected */

} /* End--cctsetup */

/************************************************************************/
void CCCModule::doublebytestore (SSINT doublearg)  /* process doublebyte argument  */
/************************************************************************/
/*
 * Description -- Process doublebyte argument,
 *                However, the early scanning for doublebyte already
 *                did this, so this just evrifies that the input now
 *                looks OK, and if not puts outs warning message(s).
 *
 * Return values: none
 *
 * Globals input: doublebyte1st set to valid derived from earlier scan
 *                doublebyte2nd set to valid derived from earlier scan
 *                doublebyte_mode should already be set to TRUE
 *
 * Globals output: none
 *
 */
{
    if (doublebyte_mode != TRUE)
        Process_msg(14, wxNullStr, 0);
    /* if values are non-zero (doublebyte1st non-zero), then this argument
       should match at least one of the values, else put out warning message.
    */
    if ((doublebyte1st != 0) &&
            (doublearg != doublebyte1st) &&
            (doublearg != doublebyte2nd))
        Process_msg(14, wxNullStr, 0);

} /* End--doublebytestore */


/****************************************************************************/
void CCCModule::cctsort()									/* Sort CC table */
/****************************************************************************/

/* Description -- Go through the changes table, group by group and sort
 *						the changes primarily in ascending order of first letter
 *						of match and secondarily in descending order of length
 *						of match.
 *						
 *						Find the beginning of left-executes for the group
 *						by going through the group, using j, until
 *						either the first letter of a change has the
 *						high bit set or we hit the end of the group.
 *
 *						Save the beginning of left-executes in groupexeq[group].
 *
 * Return values: none
 *
 * Globals input: groupbeg -- table of pointers to the beginning of groups
 *						groupxeq -- table of pointers to the beginning of
 *										  the left-execute portions of groups
 *
 * Globals output: groupbeg -- the changes within each group will be sorted
 *											numerically by ASCII sequence for
 *											constant match strings and by length
 *											with longest match first within each
 *											character, and any left-executes at the end
 *											of the group.
 *						 groupxeq -- each non-NULL element of groupxeq will point
 *											to the first left-execute for the
 *											corresponding group.
 *
 * Error conditions: there shouldn't be any
 *
 * Other functions called: goes_before -- compares two changes and returns
 *										TRUE if first should go before second
 *
 */

{

    register cngtype k;	/* Inner loop pointer */
    register cngtype j;	/* Outer loop pointer */
    tbltype hold;			/* Pointer to change pointer be inserted */
    int group;  			/* Current group # */

    for ( group = 1; group <= MAXGROUPS; group++ )
    {
        if ( groupbeg[group] != NULL)		/* If group exists... */
        {										/* ...sort the changes */
            /* NOTE: The following is a simple insertion sort. It has the
            	desirable property that is is stable -- that is, changes
            	with matching keys are maintained in the order in which
            	they were found */
            for (j = groupbeg[group] + 1; j < groupend[group]; j++)
            {
                hold = *j;			/* Save pointer to insert */
                for (	k = j - 1;
                        k >= groupbeg[group] && goes_before(hold, *k);
                        k--)
                {
                    *(k+1) = *k;	/* Move other pointers up as needed */
                }
                *(k+1) = hold;		/* Insert the saved pointer */
            }

            /* Set up pointer to the first left-execute */

            for (j = groupbeg[group]; j < groupend[group]; j++)
                if ( **j & HIGHBIT )
                    break;				/* Found one! */
            groupxeq[group] = j;

        } /* End--group exists */
    } /* End--loop through group table */
} /* End--cctsort */

/****************************************************************************/
bool CCCModule::goes_before(tbltype x,tbltype y)		/* Compare two changes */
/****************************************************************************/
//tbltype x;			/* first change */
//tbltype y;			/* second change */

/* Description -- Compares two changes and returns TRUE if x should sort
 *						before y in the table. NOTE: This function is 
 *
 * Return values: TRUE if x goes before y, FALSE otherwise
 *
 * Globals input:	unsorted -- if TRUE then compare only first letter,
 *										otherwise compare first letter, then length
 *										of match.
 *
 * Globals output: none
 *
 * Error conditions: none
 *
 * Other functions called:	srchlen -- how long is the match?
 *
 */
{
    unsigned int xltr;      /* First letter of change x */
    unsigned int yltr;      /* First letter of change y */

    /* Compute first letter of each change. If change starts with a command
    	(e.g., any(1) "xyz" > ...) then the first letter code is HIGHBIT */
    /* Following 2 lines changed by DAR/DRC 4/24/96 and 5/8/96.
       Changed to do sorting correctly, and then to be unsigned.     */
	/* if *x or *y is SPECWEDGE, then we have a zero length string on the left side and it
	   always be sorted at the end */
	xltr = wxMin( (unsigned) (*x & 0xFFFF), (unsigned) HIGHBIT);
	yltr = wxMin( (unsigned) (*y & 0xFFFF), (unsigned) HIGHBIT);

    /* If first letters match, then compare search lengths */
    if (xltr == yltr && !unsorted)
        return( srchlen(x) > srchlen(y) );
    else
        return( xltr < yltr );
}

/************************************************************************/
int CCCModule::msg_putc(int ch)		/* Put a character to screen */
/************************************************************************/
//int ch;				/* Character to put */
{
    return( putc(ch, msgfile) );	/* else send ch to stdout */
}

/************************************************************************/
int CCCModule::msg_puts(char *s)			/* Put a string to screen */
/************************************************************************/
//char *s;			/* String to put */
{
    return( fputs(s, msgfile) );		/* else do fputs to stdout */
}

/******************************************************************/
void CCCModule::upcase_str( wxString &s )      /* force string to upper case */
/*-----------------------------------------------------------------
* Description -- Do toupper( ch ) on each character in string
* Return values -- none
* Globals input -- none
* Globals output -- none
* Error conditions -- none
*/
/******************************************************************/
{
	// whm modified to use wxString methods
	s.MakeUpper();
}

// whm modified 31Mar08 to use wxWidgets' wxFile methods
int CCCModule::wgetc(WFILE * stream)
{
    int ch;

    if (!stream)
    {
        if (!lpszCCTableBuffer || !*lpszCCTableBuffer)
            return EOF;
        else
            return (*lpszCCTableBuffer++) & 0xFF;
    }

    if (stream->fEndOfFile)
        return EOF;

    else if (stream->iBufferFront == stream->iBufferEnd)
    {
        stream->iBufferFront= UNGETBUFFERSIZE;

        stream->iBufferEnd= stream->hfFile->Read(stream->Buffer + UNGETBUFFERSIZE, BUFFERSIZE) + UNGETBUFFERSIZE;

		if (stream->iBufferEnd == UNGETBUFFERSIZE)
        {
            stream->fEndOfFile= TRUE;
            return EOF;
        }
    }

    ch= stream->Buffer[(stream->iBufferFront)] & 0xFF;

    (stream->iBufferFront)++;

    /* if text mode ignore CR */
    if (stream->fTextMode && ch == '\r')
        return wgetc(stream);
    else
        return ch;
}

int CCCModule::wungetc(int ch, WFILE * stream)
{
    if (!stream)
    {
        if (!lpszCCTableBuffer)
            return EOF;
        else
        {
            *--lpszCCTableBuffer= ch;

            return ch;
        }
    }

    if (stream->iBufferFront == 0)
        return EOF;

    stream->Buffer[--(stream->iBufferFront)]= ch;

    return ch;
}

bool CCCModule::wfeof(WFILE * stream)
{
    if (stream)
        return stream->fEndOfFile;
    else
    {
        if (!lpszCCTableBuffer)
            return TRUE;
        else
            return *lpszCCTableBuffer == 0;
    }
}

/****************************************************************************/
void CCCModule::inputaline()	  /* Input a line and set len to its length */
/****************************************************************************/

/* Description -- Read in a line until \n, formfeed, or EOF found.
 *						  NULs and <CR>s are ignored.
 *						  Horizontal TABs are converted to single SPACEs.
 *
 *						If line is too long
 *							Display an error message, using err.
 *							Return.
 *
 *						If character read is " or '
 *							Treat as quote char and pass everything up
 *							  to the next occurrence of the same char
 *							  without checking for SPACEs or TABs.
 *
 * Return values: none
 *
 * Globals input: line -- current input line
 *
 * Globals output: line -- contains new input line, terminated by a \0
 *
 * Error conditions: input line longer than LINEMAX chars will cause an error
 *							  and only get the first LINEMAX chars.
 *
 * Other functions called: err -- display an error message
 *
 */

{
    register SSINT ch,      /* Input character */
    quotechar;	/* Close-quote character we're looking for */
    register char *lp;  /* Pointer into line */

    lp = line;				/* Initialize local variables */
    quotechar = '\0';

    while ((ch = wgetc(tablefile)) != EOF)
    {
        ch &= 0xff;

        if ( ch == CTRLZ )							/* CTRLZ is internal EOF marker */
            break;

        if (ch == '\0' || ch == CARRIAGERETURN)  /* Ignore NUL or <CR> */
            ;													/*  in source text */

        else if (ch == '\n' || ch == FORMFEED)  /* End-of-line characters */
            break;
        else if ( lp >= line+LINEMAX)		/* Line too long? */
        {
            *lp = '\0';
            err("Line too long, end cut off");
            break;
        }
        else
        {
            if (quotechar != '\0')	/* Check for close-quote */
            {
                if (ch == quotechar)		/* Close quote found, ignore it */
                    quotechar = '\0';		 /*  and say we're no longer in quotes */
            }
            else
            {
                if (ch == HT)				/* Horizontal TAB to space */
                    ch = ' ';
                else if (ch == '"' || ch == '\'')  /* Open quote */
                    quotechar = ch;
            }
            *lp++ = (char)(ch & 0xff); /* Put the char into line */
        } /* End--else */

    } /* End--while */

    /* trim white space from end of line */
    while (lp > line && isspace(*(lp-1)))
        lp--;

    *lp = '\0';							/* Mark the end of the line */

} /* End--inputaline */

/****************************************************************************/
SSINT * CCCModule::execute(int mlen, SSINT * tpx, int beginflag)	/* Execute replacement part of a change */
/****************************************************************************/
/* Description:
 *						This procedure executes the replacement part of a change,
 *					including output and switches.  It is used both for
 *					begin and for replacements.
 *
 * Return values: point to where table was executed
 *
 * Globals input: uppercase -- boolean: TRUE == uppercase character
 *						switches -- array of booleans, each representing
 *										  one switch
 *						curstor -- if non-zero, number of the currently active store
 *						storend -- array of pointers to the ends of stores
 *						storebegin -- array of pointers to the beginnings of stores
 *						storeoverflow -- boolean: TRUE == overflow has occurred in a
 *																		store
 *						storeact -- array of pointers to stores which are currently
 *										  being used in matches
 *						setcurrent -- boolean: TRUE == the set of first letters of
 *																	stores being used in
 *																	matches is up to date
 *						backinptr -- input pointer for the ring buffer
 *						backoutptr -- output pointer for the ring buffer
 *						matchpntr -- pointer to where to start looking for a match
 *						match -- pointer to start of current match
 *						stacklevel -- stack pointer for stack used in executing
 *											 DEFINEs
 *						numgroupson -- number of groups currently being searched
 *											  when looking for a match
 *						defarray -- array of pointers to the beginning of DEFINEs
 *						stack -- array used as a stack for executing DEFINEs
 *						eof_written -- boolean: TRUE == CC has output EOF, so
 *																	 all groups should be
 *																	 turned off
 *
 * Globals output: everything updated as a result of executing the current
 *							change
 *
 * Error conditions:  If an error occurs, a message will be displayed on
 *								the screen.  In some cases, an error flag will be
 *								set.
 *
 * Other functions called: tblskip -- skip to the END for this change
 *									output -- move a byte out of the backup buffer
 *									storematch -- compare a store to the next
 *														 thing in the change table
 *									groupinclude -- add a group to the list of
 *															groups being searched
 *									groupexclude -- remove a group from the list of
 *															groups being searched
 *									writestore -- write the contents of a store
 *														 to the screen
 *									displbefore -- do the "before" debug display
 *									displafter -- do the "after" debug display
 *									ccmath -- execute math functions
 *													(incr, add, sub, mul, div)
 *
 */

{
    register SSINT ch, *cp;        /* current character from table */
    register SSINT tempch;         /* character from dup or store buffer */
    register int i;                /* Generic loop index */
    register unsigned j, k;        /* Unsigned loop indices */
    register tbltype tp;
    SSINT cha;                     /* next character in table */
    bool bOmitDone;                /* true only if fwd or omit done with this match */

    if (!dupptr)
    {
        dupptr = (SSINT*)malloc(mlen * sizeof(SSINT));
        memcpy(dupptr, matchpntr - mlen, mlen * sizeof(SSINT));          /* Save initial matchpntr */
    }
    bOmitDone = FALSE;

    while (( *tpx != SPECPERIOD) &&
            (nUsedOutBuf < nMaxOutBuf) &&
            !bNeedMoreInput &&
            ( !bOmitDone ))
    {
        ch= *tpx++;

        if ( (ch & HIGHBIT) == 0 )
        {											/* Not command, so output */
            if (uppercase)
            {
                ch = (char)toupper(ch);
                uppercase = FALSE;
            }
            /* if we are in doublebyte mode and if the first character
               we are looking at qualifies as the first half of a doublebyte
               pair then look at the next byte, if it also qualifies then
               store the pair of bytes as one doublebyte element.  Otherwise
               do business as normal (store one char as one SSINT element).
            */
            if ( (doublebyte_mode==TRUE) && (ch >= doublebyte1st))
            {
                cha = *tpx++;
                if ( cha >= doublebyte2nd )
                    output((SSINT)((ch * 256) + ((int) (cha & 0x00ff))));  /* combine both as one */
                else
                {
                    output(ch);
                    tpx--;                  /* back up for next time thru */
                }
            }
            else
                output(ch);
        }
        else
        {													/* See if command */
            switch (ch)
            {
            case IFCMD:											/* If */
                if ( !switches[*tpx++] )
                    tblskip( &tpx);
                break;
            case IFNCMD:										/* Ifn */
                if ( switches[*tpx++] )
                    tblskip( &tpx);
                break;
            case ELSECMD:										/* Else */
                tblskip( &tpx);
                break;
            case ENDIFCMD:										/* Endif */
                break;
            case SETCMD:										/* Set */
                switches[*tpx++] = TRUE;
                break;
            case CLEARCMD:										/* Clear */
                switches[*tpx++] = FALSE;
                break;
            case STORCMD:										/* Store */
                curstor = *tpx++;
                storend[curstor] = storebegin[curstor];
                storeoverflow = FALSE;
                if ( storeact[curstor] )
                    setcurrent = FALSE;
                break;
            case APPENDCMD:									/* Append */
                curstor = *tpx++;
                break;
            case ENDSTORECMD:									/* Endstore */
                curstor = 0;
                break;
            case PUSHSTORECMD:
                PushStore();
                break;
            case POPSTORECMD:
                PopStore();
                break;
            case OUTCMD:											/* Out */
                curstor = 0;						/* Prevent storing during output */

            case OUTSCMD:											/* Outs */
                i = *tpx++;										/* Get store # */
                k = storend[i] - storebegin[i];		/* Get # of chars in store */

                // if nothing in the store and predefined flag is non-zero then
                // we are to output one of the predefined output values
                if (( k == 0 ) && ( storepre[i] != 0 ))
                {
                    resolve_predefined(i);
                    for ( j = 0; j < strlen(predefinedBuffer); j++)
                        output((SSINT) predefinedBuffer[j]);
                }

                /*   Explicit array indexing is used here because input may be going */
                /* to a lower-numbered store.  This could cause the contents of		*/
                /* store[i] to move elsewhere in memory, giving erroneous results.	*/

                for ( j = 0; j < k; j++ )
                {
                    tempch = *(storebegin[i] + j);	// 7.4.16
                    if (uppercase)					// first char may need capitalized
                    {                           //
                        tempch = toupper( tempch);  //
                        uppercase = FALSE;          //
                    }                           //
                    /* if we are in doublebyte mode and if the element we are
                       looking at has non-empty high order half, then output
                       it as two separate bytes.
                    */
                    if ( (doublebyte_mode==TRUE) && (tempch / 256 != 0) )
                    {
                        cha = (tempch >> 8) & 0x00ff;  /* get hi order half of doublebyte char */
                        tempch = tempch & 0x00ff; /* keep low order half here  */
                        output(cha);              /* output high order half first */

                        /* If after we have done all our operations to the stored data
                           it no longer matches the doublebyte input criteria, still
                           split it back to two bytes and output it, but give warning
                           to the user that (s)he may have hosed up their data.  */
                        if ((cha < doublebyte1st) ||
                                ((cha == doublebyte1st) && (tempch < doublebyte2nd)))
                            Process_msg(17, wxNullStr, 0);
                    }
                    output( tempch );
                }
                break;
            case BACKCMD:                                      /* Back */
                BackCommand(&tpx, utf8encoding);
                break;
            case BACKUCMD:                                      /* Back utf-8 encoding */
                BackCommand(&tpx, TRUE);
                break;
            case DUPCMD:													/* Dup */
                uppercase = FALSE;	/* 7.4.16 ensure following literal is lowercase */
                for ( cp = dupptr; cp < dupptr + mlen; cp++)
                    output( *cp);

                if (endoffile_in_match)
                {
                    curstor = 0;
                    numgroupson = 0;		  /* All groups off for any further endfile */
                    setcurrent = FALSE;
                    eof_written = TRUE;
                }

                break;
            case IFEQCMD:													/* Ifeq */
                if ( storematch(&tpx) != 0 )
                    tblskip(&tpx);
                break;
            case IFNEQCMD:													/* Ifneq */
                if ( storematch(&tpx) == 0 )
                    tblskip(&tpx);
                break;
            case IFGTCMD:													/* Ifgt */
                if ( storematch(&tpx) <= 0 )
                    tblskip(&tpx);
                break;
            case IFNGTCMD:                                     /* Ifngt */
                if ( storematch(&tpx) > 0 )
                    tblskip(&tpx);
                break;
            case IFLTCMD:                                      /* Iflt */
                if ( storematch(&tpx) >= 0 )
                    tblskip(&tpx);
                break;
            case IFNLTCMD:                                     /* Ifnlt */
                if ( storematch(&tpx) < 0 )
                    tblskip(&tpx);
                break;
            case LENCMD:
                LengthStore(&tpx);
                break;
            case IFSUBSETCMD:
                if ( IfSubset(&tpx) == 0 )
                    tblskip(&tpx);
                break;
            case NEXTCMD:													/* Next */
                while ( *tpx++ != SPECWEDGE)
                    ;
                break;
            case BEGINCMD:													/* Begin */
                break;
            case ENDCMD:													/* End */
                break;
            case REPEATCMD:												/* Repeat */
                while ( *(tpx - 1) != SPECWEDGE && *--tpx != BEGINCMD )	// 7b.43 ALB stop at wedge if no begin found
                    ;
                break;
            case USECMD:													/* Use */
                numgroupson = 0;	  					/* Compile puts incl next */
                break;
            case INCLCMD:													/* Include */
                groupinclude( *tpx++ );
                break;
            case EXCLCMD:													/* Exclude */
                groupexclude( *tpx++ );
                break;
            case GROUPCMD:													/* Group */
                Process_msg(41, wxNullStr, 0);
                break;
            case DOCMD:														/* Do */
                if (stacklevel >= STACKMAX)
                    Process_msg(42, wxNullStr, (long unsigned) STACKMAX);
                else
                {
                    if ((tp = defarray[*tpx++]) == NULL)
                        Process_msg(26, wxNullStr, (long unsigned) sym_name(*(tpx-1), DEFINE_HEAD));
                    else
                    {
                        stack[++stacklevel] = tpx;
                        tpx = tp+1;
                    }
                }
                break;
            case TOPBITCMD:								/* Number with top bit on */
                output((SSINT)((*tpx++) | HIGHBIT));
                break;
            case FWDCMD:                                       /* Fwd  */
            case OMITCMD:                                      /* Omit */
                FwdOmitCommand(&tpx, &bOmitDone, (bool) (ch == FWDCMD), utf8encoding);
                break;
            case FWDUCMD:                                       /* Fwd utf-8 */
            case OMITUCMD:                                      /* Omit utf-8 */
                FwdOmitCommand(&tpx, &bOmitDone, (bool) (ch == FWDUCMD), TRUE);
                break;
            case INCRCMD:													/* Incr */
                incrstore( *tpx++ );
                break;
            case DECRCMD:                                                                                                   /* Decr */
                decrstore( *tpx++ );
                break;
            case ADDCMD:													/* Add */
            case SUBCMD:													/* Sub */
            case MULCMD:													/* Mul */
            case DIVCMD:													/* Div */
            case MODCMD:													/* Mod */
                ccmath( ch, &tpx );
                break;
            case WRITCMD:													/* Write */
                while ( TRUE )
                {
                    if ( *tpx & HIGHBIT || *tpx == CTRLZ )
                        break;
                    tpx++;
                }
                break;
            case WRSTORECMD:												/* Wrstore */
                tpx++;                              // skip this element
                break;
            case READCMD:													/* Read */
                break;
            case CASECMD:													/* Caseless */
                if (beginflag)
                {
                    if (doublebyte_mode)
                        Process_msg(1, wxNullStr, 0);
                    else
                        caseless = TRUE;
                }
                else
                    Process_msg(45, wxNullStr, 0);
                break;
            case UTF8CMD:													/* Caseless */
                if (beginflag)
                {
                    if (doublebyte_mode)
                        Process_msg(1, wxNullStr, 0);
                    else
                        utf8encoding = TRUE;
                }
                else
                    Process_msg(45, wxNullStr, 0);
                break;
            case BINCMD:												/* Binary mode */
                if (!beginflag)
                    Process_msg(46, wxNullStr, 0);
                break;
            case DOUBLECMD:                                                                                         /* Doublebyte mode */
                if (!beginflag)
                    Process_msg(47, wxNullStr, 0);
                ch = (*tpx++);   /* skip past the following (already processed) argument */
                break;
            case UNSORTCMD:                                 /* Unsort mode */
                if (!beginflag)
                    Process_msg(48, wxNullStr, 0);
                break;
            case ENDFILECMD:
                curstor = 0;
                numgroupson = 0;		  /* All groups off for any further endfile */
                setcurrent = FALSE;
                eof_written = TRUE;
                break;


            default:
                Process_msg(49, wxNullStr, (long unsigned) cmdname((char)ch, FALSE) );
            }		  /* end of switch */
        }			/* else */

        /* unstack any do in effect */
        while ( (stacklevel > 0) && ((ch=(*tpx)) == SPECPERIOD) )
            tpx = stack[stacklevel--];

    } /* End--while */

    if (*tpx == SPECPERIOD ||
            bOmitDone)
    {
        free(dupptr);
        dupptr= NULL;
        tpx= NULL;
    }

    if ( (numgroupson == 0) && !eof_written )
    {													/* NO active groups--oops! */
        Process_msg(50, wxNullStr, (long unsigned) sym_name(cgroup, GROUP_HEAD) );
        bailout(BADEXIT, FALSE);					/* Bail out! */
    }
    return tpx;

} /* End--execute */

/****************************************************************************/
void CCCModule::output(register SSINT ch)											/* Output or store a char */
/****************************************************************************/
//register SSINT ch;

/* Description:
 *						This procedure outputs a char or stores the char if
 *					currently storing.  On output, if the char is a CR it outputs
 *					a NEWLINE.	 If the char is a <CTRL-Z> for EOF, it turns off
 *					storage and all groups.
 *
 * Return values: none
 *
 * Globals input: curstor -- if non-zero, the number of the currently
 *										 active store
 *						numgroupson -- number of groups currently active
 *                setcurrent -- boolean: TRUE == letter set for searches
 *																	is current
 *						backinptr -- input pointer for the backup buffer
 *						backoutptr -- output pointer for the backup buffer
 *						backbuf -- address of the start of the backup buffer
 *						eof_written -- boolean: TRUE == CTRLZ has been output
 *																	 by CC, so numgroupson
 *																	 SHOULD be zero
 *
 * Globals output: curstor -- set to zero on EOF
 *						 numgroupson -- set to zero on EOF
 *						 setcurrent -- set to FALSE on EOF
 *						 eof_written -- set to TRUE on EOF
 *						 backinptr -- updated
 *						 backoutptr -- updated
 *
 * Error conditions: on EOF (char == <CTRL-Z>)
 *							  any active store and groups will be turned off
 *
 *							if the buffer fills up, writechar will be called to free
 *							  up one space
 *
 * Other functions called: storch -- add a char to a store
 *									writechar -- move a char out of the backup buffer
 *
 */

{
    if (ch > 128)
        eof_written= FALSE;

    if (curstor > 0)								/* See if storing */
        storch(curstor, ch);
    else
    {												/* Put char in ring buffer */
        *backinptr++ = ch;
        if (backinptr >= backbuf+BACKBUFMAX)
            backinptr = backbuf;
        if (backinptr == backoutptr)				/* If backbuffer full */
            writechar();
    }

} /* End--output */

/****************************************************************************/
void CCCModule::storch(register int store, SSINT ch)		/* Store a char at the end of the current store */
/****************************************************************************/
//register int store;
//SSINT ch;

/* Description:
 *						This procedure stores character ch at the end of the
 *					store. It first checks to see if there is room in the store.
 *					If there is not, it tries to make room by moving everything
 *					above up 10 characters. If it can't, it gives a message, and
 *					sets storeoverflow to prevent repeating the message until
 *					non-overflow.
 *
 * Return values: none
 *
 * Globals input: setcurrent -- boolean: TRUE == letter set for searches
 *																	is up to date
 *						storeact -- array of pointers to stores being used
 *										  in matching, if a store is not being used
 *										  its pointer will be set to NULL
 *						storebegin -- array of pointers to the beginning of stores
 *						storend -- array of pointers to the end of stores
 *						storeoverflow -- boolean: TRUE == we have overflowed the
 *																		store area
 *
 * Globals output: setcurrent -- if store was the target of a match,
 *											  setcurrent will be set to FALSE
 *						 storebegin, storend -- if we had to shift things down
 *														  the pointers will be updated
 *						 storeoverflow -- if overflow occurred
 *												  storeoverflow will be set to TRUE
 *
 * Error conditions: if this call to storch caused overflow,
 *							  storeoverflow will get set to TRUE and
 *							  an error message will be displayed
 *
 * Other functions called: None.
 *
 */

{
    register tbltype tp;					/* Working pointer */
    register tbltype end_of_store;	/* Pointer to the end of store */

    /* Note:  The above variables are declared separately because */
    /*				DECUS C does not allow regular typedef's	*/

    int i;

    if ( storeact[store] )			  /* Is store being used in a match? */
        setcurrent = FALSE;				  /* Yes, letterset is no longer current */

    end_of_store = storend[store];						/* Point to end of store */

    if (end_of_store >= storebegin[(store + 1)]) {  /* If store needs to move up */
        if (storebegin[NUMSTORES] + 11 >= storelimit)
        {															/* If overflow */

            if (!storeoverflow)		/* Only give overflow message once */
            {
                Process_msg(4, wxNullStr, (long unsigned) sym_name(store, STORE_HEAD));

                storeoverflow = TRUE;			/* Avoid repeating the error message */
            }
            return;
        }
        else													 /* Move up to make room */
        {
            /* Shift contents */
            tp = storebegin[NUMSTORES];

            do
            {
                *(tp+10) = *tp;
            } while	(tp-- > end_of_store);


            for ( i = store + 1; i <= NUMSTORES; i++ )  /* Now update pointers */
            {
                storebegin[i] += 10;
                storend[i] += 10;
            }
        } /* End--else (move up to make room) */
	}
    *storend[store]++ = ch;       /* Add the char to store */

    /* storend[store] += 1; */

} /* End--storch */

/****************************************************************************/
void CCCModule::tblskip(SSINT **tblpnt)  /* Skip to end-of-entry, else, or endif in table entry */
/****************************************************************************/
//SSINT **tblpnt;

/* Description:
 *						This procedure skips over table entries until an else,
 *					endif, or end of entry is found.
 *
 * Return values: if end of entry, *tblpnt points to the SPECPERIOD,
 *						  otherwise, *tblpnt points to the char just beyond
 *						  the command
 *
 * Globals input: none
 *
 * Globals output: none
 *
 * Error conditions: none
 *
 * Other functions called: none
 *
 */

{
    register SSINT *tp;              /* Working pointer into table */
    register SSINT ch;            /* Temp storage for char from table */
    int level;					 /* Level of nesting of begin and end blocks */

    level = 0;												/* Initializations */
    tp = *tblpnt;

    for ( ;; )									/* Do forever	(exit is via a break) */
    {
        ch = *tp++;							/* Get current char for comparison */
        if ( ch == SPECPERIOD )
        {													/* End of entry */
            tp--;										/* Back up to end of entry */
            break;											/* Go exit */
        }
        else if ( ch == BEGINCMD )				/* Deeper nesting */
            level++;
        else if ( ch == ENDCMD )
        {
            if ( --level == 0 )				/* Don't exit at a deeper level than */
                /*  we entered */
                if ( *tp == ELSECMD )
                    tp++;							/* Point to the command beyond the ELSE */
            if ( level <= 0 )
                break;							/* Exit on END */
        }
        else if ( level == 0 )
            if ( ch == ELSECMD || ch == ENDIFCMD )  /* Exit on ELSE or ENDIF */
                break;										  /*	at the current level */
    }

    *tblpnt = tp;						/* Update tblpnt */

} /* End--tblskip */

/************************************************************************/
void CCCModule::PushStore()
/************************************************************************/
/* Description:
 *			This function saves the currect storing context on a stack.  It
 *			is used in conjuction with PopStore to allow the CC programmer
 *       to write groups that don't disturbe the current storing context.
 *
 * Return values:  none
 *
 * Globals input: curstor -- number of the store currently in use.
 *                iStoreStackIndex -- depth in current stack.
 *
 * Globals output: none
 *
 * Error conditions: Store stack overflow.
 *
 */
{
    if(iStoreStackIndex >= STORESTACKMAX)
    {
        Process_msg(8, wxNullStr, 0);
    }
    else
        StoreStack[iStoreStackIndex++] = curstor;
}

/************************************************************************/
void CCCModule::PopStore()
/************************************************************************/
/* Description:
 *			This function restores a previously saved storing context. It
 *			is used in conjuction with PushStore to allow the CC programmer
 *       to write groups that don't disturbe the current storing context.
 *
 * Return values:  none
 *
 * Globals input: curstor -- number of the store currently in use.
 *                iStoreStackIndex -- depth in current stack.
 *
 * Globals output: none
 *
 * Error conditions: Store stack underflow.
 *
 */
{
    if(iStoreStackIndex == 0)
    {
        Process_msg(9, wxNullStr, 0);
    }
    else
        curstor = StoreStack[--iStoreStackIndex];
}

/****************************************************************************/
void CCCModule::resolve_predefined(int index)    /* Resolve value of predefined store  */
/****************************************************************************/
/*
 * Description:
 *             This routine resolves the value of predefined stores.   
 *             (Examples of these are cccurrentdate, cccurrenttime,
 *              ccversionmajor, and ccversionminor).
 *
 * Input: index -- the store number
 *
 * Globals output: predefinedBuffer -- this array gets the value of the
 *                 predefined store.  Note that this is set at the time
 *                 it is referenced, not when the change table is read.
 *
 */
{
    time_t long_time;           // seconds since time started on 1-1-1970
    struct tm *newtime;         // structure for time and date information

    switch (storepre[index])
    {
    case CCCURRENTDATE:
        time (&long_time);
        newtime = localtime(&long_time);
        sprintf(predefinedBuffer, "%02d/%02d/%4d", newtime->tm_mon + 1,
                newtime->tm_mday, 1900 + newtime->tm_year);
        break;
    case CCCURRENTTIME:
        time (&long_time);
        newtime = localtime(&long_time);
        sprintf(predefinedBuffer, "%02d:%02d:%02d", newtime->tm_hour,
                newtime->tm_min, newtime->tm_sec);
        break;
    case CCVERSIONMAJOR:
        sprintf(predefinedBuffer, "%s", VERSIONMAJOR);
        break;
    case CCVERSIONMINOR:
        sprintf(predefinedBuffer, "%s", VERSIONMINOR);
        break;
    default:
        predefinedBuffer[0] = 0;    // just put end of string marker
        break;                      // we should never default actually
    }
}

/************************************************************************/
void CCCModule::BackCommand(SSINT **cppTable, bool utf8)
/************************************************************************/
{
    SSINT *cpTbl, *cpTmp;
    int i;
    long lStoreValue;
    char  caOperandBuffer[20];  /* dummy buffer for valnum() */
    bool bErrorInProcessing= FALSE;

    cpTbl = *cppTable;
    i = *cpTbl++;
    if( i == VALCMD )
    {
        valnum( ++cpTbl, caOperandBuffer, 20, &lStoreValue, TRUE);
        /* Check for reasonable size */
        if((lStoreValue > 32767L) || (lStoreValue < 0))
        {
            Process_msg(3, wxNullStr, (long unsigned) lStoreValue);
            lStoreValue = 1L;
        }
        i = (int)(lStoreValue & 0x7FFF);
        if (curstor == 0)
        {
            if(lStoreValue > (long)(BACKBUFMAX))
            {
                Process_msg(10, wxNullStr, (long unsigned) i);
                i = 1;
            }
        }
        else
        {     /* If value is larger than the store, back the whole store */
            if(i > (storend[curstor] - storebegin[curstor]))
            {
                i = (storend[curstor] - storebegin[curstor]);
            }
        }
        cpTbl++;
    }

    if (i > 0 &&
            eof_written)
    {
        eof_written= FALSE;
        i--;
    }

    for ( ; !bErrorInProcessing && i-- > 0; )
    {
        do {
            if (curstor == 0)
            {													/* If not storing */
                if ( (backinptr == backbuf) && (backinptr != backoutptr) )
                    cpTmp = backbuf+BACKBUFMAX-1;			/* Handle back-wrap */
                else
                    cpTmp = backinptr - 1;

                if ( (backinptr == backoutptr) || (matchpntr <= match) )
                {
                    /* Error if backbuf empty or match full */
                    Process_msg(0, wxNullStr, 0);
                    bErrorInProcessing= TRUE;
                    break;
                }
                else
                {
                    *--matchpntr = *cpTmp;			/* Transfer one char */
                    backinptr = cpTmp;				/* Update global pointer */
                }
            }
            /* Storing */

            else if ( storend[curstor] <= storebegin[curstor] ||
                      matchpntr <= match )
            {
                Process_msg(11, wxNullStr, 0);
                bErrorInProcessing= TRUE;
                break;
            }
            else
                *--matchpntr = *--storend[curstor];

        } while (utf8 && ((*matchpntr & 0xC0) == 0x80));
    } /* end for */
    *cppTable = cpTbl;   /* Update table pointer */
}

/****************************************************************************/
int CCCModule::valnum(register SSINT * operand, char *opbuf, int buflen, long int *value, char first)  /* Validate numeric arg */
/****************************************************************************/
//register SSINT *operand;
//char *opbuf;            /* Buffer for the operand, in case it's bad */
//int buflen;					/* Length of opbuf */
//long int *value;
//char first;			/* Boolean: TRUE == first operand, so the first thing
						 //*							 we'll see is a store #
						 //*/
/*
 * Description:
 *						Check the operand to see if it's a valid numeric argument
 *					for the arithmetic functions.
 *						At the same time, copy the operand into opbuf so the caller
 *					can use it for displaying an error message.
 *
 * Return values:
 *						TRUE == operand is a valid number,
 *								  *value will contain its value
 *
 * Note: an empty store is assumed to have a value of zero
 *
 *						FALSE == operand is not a valid argument
 *
 *						-1 == number was too long (i.e. won't fit in a long int)
 *
 *						opbuf == the operand, as a NUL-terminated string.
 *									if the operand is longer than (buflen - 1), only
 *									  the first (buflen - 1) chars will be copied
 *
 * Globals input: storebegin -- array of pointers to the beginning of stores
 *						storend -- array of pointers to the end of stores
 *
 * Globals output: none
 *
 * Error conditions: If the operand is not a valid argument,
 *								FALSE will be returned, and *value will be undefined
 *							If the number won't fit in a long int,
 *								-1 will be returned, and *value will be undefined
 *
 * Other procedures called:
 *									atol -- convert an ASCII string to a long int
 *
 * Note: This procedure is called during compilation to check for
 *				valid numeric strings at compile time.
 */

{
    char  numbuf[20];       /* used for building the string for
                               conversion by atol */
    SSINT *end_of_operand;  /* Address of either the end of the operand
                             * or end of the store used by cont
                             */
    char firsthalf;         /* first half doublebyte pair of valid digits */

    register char *numptr;  /* Pointer for loading numbuf    */


    SSINT *opptr;  /* Pointer for finding end of operand     */

    int store,		/* store used by a cont command */
    i;				/* counter for digits (a 32-bit int is <= 10 digits) */

    buflen--;		/* Allow for the NUL */

    if ( (*operand == CONTCMD) || (*operand == VALCMD) || first )
    {
        if ( first )
            store = *operand;
        else
            store = *(++operand);
        end_of_operand = storend[store];
        operand = storebegin[store];
    }
    else
    {							/* Find the end of the operand */
        for ( opptr = operand; (!(*opptr & HIGHBIT) || (*opptr == TOPBITCMD));
                opptr++)
            ;
        end_of_operand = opptr;
    }

    if ( operand == end_of_operand )
    {
        *value = 0L;						/* Empty store or string, so value is zero */
        *opbuf = '\0';				/* Nothing there, so return an empty string */
        return( TRUE );
    }

    if ( (*operand == '-') || (*operand == '+') )		/* Catch the sign */
    {
        *opbuf++ = (char)*operand;            /* Copy it into the buffer */
        buflen--;
        numbuf[0] = (char)*operand++;
    }
    else
    {
        numbuf[0] = '+';							  /* Default to a positive number */
    }
    /* if just a + or - in the operand then this is not a valid number */

    if (operand == end_of_operand)
    {
        *opbuf = '\0';
        return( FALSE );			/* Invalid digit encountered */
    }

    for ( i = 1, numptr = numbuf + 1; (operand < end_of_operand) && (i < 19);
            operand++, numptr++, i++)
        if ( isdigit(*operand) )              /* DRC removed casting 8-19-96 */
        {
            // if we are in doublebyte mode and if the high order part of the
            // element *operand points to is also a valid digit, (and if we
            // have room in buffer for both halves of the number), then process
            // the high order firsthalf of the element before low order half
            if (doublebyte_mode == TRUE)
            {
                firsthalf = (char) (*operand >> 8);
                if (( isdigit( firsthalf )) && ( i < 18 ))
                {
                    *numptr++ = firsthalf;
                    i++;
                    if ( buflen )
                    {
                        *opbuf++ = (char)*operand;
                        buflen--;
                    }
                }
            }

            *numptr = (char)*operand;
            if ( buflen )
            {
                *opbuf++ = (char)*operand;
                buflen--;
            }
        }
        else
        {
            while( buflen && (operand < end_of_operand) )
            {														/* Get the rest of it */

                if ( *operand == TOPBITCMD )
                {
                    *opbuf++ = (*(operand + 1) | HIGHBIT);
                    operand += 2;
                }
                else
                    *opbuf++ =  (char)*operand++;
                buflen--;
            }
            *opbuf = '\0';
            return( FALSE );			/* Invalid digit encountered */
        }

    *numptr = '\0';						/* Terminate the strings */
    *opbuf = '\0';

    if ( (i > 11) || ((i == 11) && (numbuf[1] > '1')) )
        return( -1 );				/* Number was too big */

    *value = atol( numbuf );

    return( TRUE );
} /* End--valnum */

/****************************************************************************/
int CCCModule::storematch(SSINT **tblpnt)	/* Compare a store with next thing in table */
/****************************************************************************/
//SSINT **tblpnt;  /* Address of pointer into table */

/* Description:
 *						This function compares the content of the storage area
 *					whose number is next in the table with the following string
 *					in the table (terminated by a command or begin).  If the
 *					first char following is a cont command, it picks up the store
 *					number from it and compares the two storage areas instead.
 *					It returns -1 for less than, 0 for equal and 1 for
 *					greater than.
 *
 * Return values:  1 == store > string or contents of second store
 *						 0 == store == string or contents of second store
 *						-1 == store < string or contents of second store
 *
 * Globals input: storebegin -- array of pointers to beginning of stores
 *						storend -- array of pointers to end of stores
 *
 * Globals output: none
 *
 * Error conditions: none
 *
 * Other functions called: valnum -- check for a valid numeric argument
 *                                     (returns TRUE if one found)
 *
 */

{
    register SSINT *sp, /* (fast) Pointer to beginning of store */
    *sp2;  /* (fast) Pointer to the thing to compare to
    *	(either a store or a string in the table)
    */
    SSINT *se,   /* Pointer to end of store */
    *se2,		 /* Pointer to end of store specfied in font */
    *tp,		 /* Working pointer into table */
    ch_temp;  /* Char temp for handling 8-bit stuff */

    char operand_buffer[20];  /* dummy buffer for valnum() */

    long first_val = 0;    /* Value of first operand */ // whm initialized to 0 here to avoid compiler warning
    long second_val;     /* Value of second operand */

    bool bPreNumeric;  /* TRUE if have predefined store with numeric value */

    int i,             /* Index into storebegin and storend arrays         */
    ans;            /* Return value                                     */
    unsigned j;        /* index for when evaluating a predefined store     */

    tp = *tblpnt;                     /* Get working copy of table pointer */

    // if we have nothing in this store, and if the flag is on saying that
    // this is a predefined store then call routine to resolve that store

    if (( storend[*tp] == storebegin[*tp] ) && ( storepre[*tp] != 0 ))
    {
        resolve_predefined(*tp);         // resolve the predefined store
        // (puts value into predefinedBuffer)

        // see if all used characters of predefined store are numeric or not
        // if so leave bPreNumeric as TRUE and set value in first_val

        first_val = 0;
        bPreNumeric = TRUE;
        for (j = 0; j < strlen(predefinedBuffer); j++)
        {
            if (( predefinedBuffer[j] >= '0' ) && ( predefinedBuffer[j] <= '9' ))
                first_val = (first_val * 10) + (predefinedBuffer[j] - '0');
            else
                bPreNumeric = FALSE;       // denote not numeric predefined store
        }
    }
    else
    {
        predefinedBuffer[0] = 0;         // denote we have no predefined store
        bPreNumeric = FALSE;             // denote not numeric predefined store
    }

    /*
     * Check to see if both operands are numeric.
     *  If they are, handle the comparison in terms of their values.
     *
     * Note: an empty store is considered to be a valid numeric argument,
     *        with a value of ZERO
     */

    /* See if the first operand contains a valid numeric value  */
    if ((( predefinedBuffer[0] != 0) && ( bPreNumeric ))
            || (( predefinedBuffer[0] == 0 )
                && ( valnum( tp, operand_buffer, 20, &first_val, TRUE) )))
    {
        /* See if the second operand contains a valid numeric value */
        if ( valnum( (tp + 1), operand_buffer, 20, &second_val, FALSE) )
        {
            /* Both operands are numeric */
            if ( first_val > second_val )
                ans = 1;
            else if ( first_val < second_val )
                ans = -1;
            else
                ans = 0;                   /* They're equal */

            /*
            	* Skip over the match, now that we've executed it
            	*/

            tp++;                      /* Skip the store number */
            if ( *tp != CONTCMD )
            {                    	/* Second operand is a string */
                for ( ; !(*tp & HIGHBIT) || (*tp == TOPBITCMD); tp++ )
                    ;
            }
            else
                tp += 2;                /* Skip cont and its store # */

            *tblpnt = tp;        /* Update table pointer */
            return( ans );
        } /* End--both operands are numeric */
    } /* End--first operand is numeric */

    i = *tp++;						/* Get store number from first command */

    sp = storebegin[i];			/* Get begin and end pointers for store */
    se = storend[i];

    ans = -1;									/* Assume less than */
    if ( *tp == CONTCMD )
    {												/* Content command */
        tp++;
        i = *tp++;						/* Get store number from cont command */
        sp2 = storebegin[i];
        se2 = storend[i];			/* Get begin and end pointers for second store */
        for (;;)
        {
            if ( sp >= se )				/* End of first store? */
            {
                if ( sp2 >= se2 )				/* End of second also? */
                    ans = 0;								/* Must be equal */
                break;
            }
            if ( sp2 >= se2 )			/* End of second store? */
            {
                ans = 1;					/* First store is longer, so */
                break;					 /*  it is greater */
            }
            if ( *sp != *sp2 )					/* Non match? */
            {													/* Which is greater? */
                if ( (unsigned)(*sp) > (unsigned)(*sp2) ) /* DAR fixed for double byte compare  4/24/96 */
                    ans = 1;
                break;
            }
            sp++;					/* Increment pointers */
            sp2++;
        }
    }
    else
    {									/* Match string from table */

        sp2 = tp;		/* Use a fast pointer into the table */
        j = 0;         /* initialize index to predefined store (if any)  */
        for (;;)
        {
            if ( *sp2 == TOPBITCMD )  /* Handle possible elements with high bit on */
            {
                sp2++;
                ch_temp = (*sp2 | HIGHBIT);
            }
            else
                ch_temp = *sp2;

            /* end of the store?          */
            if ((( predefinedBuffer[0] != 0 ) && ( j >= strlen(predefinedBuffer) ))
                    || (( predefinedBuffer[0] == 0) && ( sp >= se )))
            {
                if ( *sp2 & HIGHBIT )       /* End of the string, too?    */
                    ans = 0;                 /*    Must be equal           */
                break;
            }
            if ( *sp2 & HIGHBIT )          /* String done?               */
            {
                ans = 1;                    /* Store is longer,           */
                break;                      /*    so the store is greater */
            }

            /* non-match?                            */
            if ((( predefinedBuffer[0] != 0 )
                    && ( (SSINT)predefinedBuffer[j] != ch_temp) )
                    || (( predefinedBuffer[0] == 0) && ( *sp != ch_temp )))
            {
                /* Which is greater?          */
                if ( predefinedBuffer[0] != 0 )
                {
                    if ( (unsigned)predefinedBuffer[j] > (unsigned)(ch_temp) ) /* DAR fixed for doublebyte compare 4/24/96 */
                        ans = 1;
                }
                else
                {
                    if ( (unsigned)(*sp) > (unsigned)(ch_temp) ) /* DAR fixed for doublebyte compare 4/24/96 */
                        ans = 1;
                }
                break;
            }

            if ( predefinedBuffer[0] != 0 )
                j++;                        /* increment pointer          */
            else
                sp++;                       /* Increment pointer          */
            sp2++;
        }

        while ( !(*sp2 & HIGHBIT)
                || (*sp2 == TOPBITCMD) ) /* Go past unmatched in table */
        {
            if ( *sp2 == TOPBITCMD )       /* Skip element with high bit on */
                sp2++;
            sp2++;
        }

        tp = sp2;
    }			  /* else */
    *tblpnt = tp;                        /* Update table pointer       */
    return(ans);
} /* End--storematch */

/************************************************************************/
void CCCModule::LengthStore(SSINT **cppTable)
/************************************************************************/
/* Description:
 *			This function returns to the output stream an ASCII string
 *       which gives the number of characters in the given store.
 *
 * Return values:  none
 *
 * Globals input: storebegin -- array of pointers to beginning of stores
 *						storend -- array of pointers to end of stores
 *
 * Globals output: none
 *
 * Error conditions: none
 *
 */
{
    long lLength;
    int i;
    SSINT *cpWorkTable;    /* Working copy of table pointer */
    char   cpAsciiLength[15];
    char  *cpAscii;

    cpWorkTable = *cppTable;	 	/* Get working copy of table pointer */

    i = *cpWorkTable++;			/* Get store number from first command */
    lLength = (long)(storend[i] - storebegin[i]);

    sprintf(cpAsciiLength,"%ld", lLength);

    cpAscii = &cpAsciiLength[0];
    while(*cpAscii)
        output(*cpAscii++);

    *cppTable = cpWorkTable;   /* Update pointer */

}

/****************************************************************************/
int CCCModule::IfSubset(SSINT **tblpnt)	/* TRUE if each character in store is found in following
                      * string. */
/****************************************************************************/
//SSINT **tblpnt;  /* Address of pointer into table */

/* Description:
 *						This function compares the store with the following string.
 *             The function returns TRUE if each character in the store is
 *             found in the following string.  Dupicate characters in the
 *             following string are both redudant and ignored. The store
 *             with no characters is a subset of any following string.
 *
 * Return values:  1 == Each store member is in following string
 *						 0 == If any store member is not in following string
 *
 * Globals input: storebegin -- array of pointers to beginning of stores
 *						storend -- array of pointers to end of stores
 *
 * Globals output: none
 *
 * Error conditions: none
 *
 */

{
    register SSINT *sp, /* (fast) Pointer to beginning of store */
    *sp2; /* (fast) Pointer to the thing to compare to
    *	(either a store or a string in the table)
    */
    SSINT *se,      /* Pointer to end of store */
    *se2,		 /* Pointer to end of store specfied in font */
    *tp,		 /* Working pointer into table */
    ch_temp;  /* Char temp for handling 8-bit stuff */

    int i,	 /* Index into storebegin and storend arrays */
    ans;	/* Return value */

    tp = *tblpnt;						/* Get working copy of table pointer */

    i = *tp++;						/* Get store number from first command */

    sp = storebegin[i];			/* Get begin and end pointers for store */
    se = storend[i];

    ans = 0;									/* Assume FALSE */
    if ( *tp == CONTCMD )
    {												/* Content command */
        tp++;
        i = *tp++;						/* Get store number from cont command */
        se2 = storend[i];			/* Get begin and end pointers for second store */
        for (;;)
        {
            if ( sp >= se )				/* End of first store? */
            {
                ans = 1;                /* All must have been found */
                break;
            }
            for(sp2 = storebegin[i];;)
            {
                if ( sp2 >= se2 )				/* End folowing string? */
                {
                    ans = 0;
                    break;
                }
                if ( (unsigned)(*sp) == (unsigned)(*sp2) ) /* DAR fixed for doublebyte compare 4/24/96 */
                {
                    ans = 1;						/* Found match */
                    break;
                }
                sp2++;                     /* Check next character */
            }
            if(!ans)
                break;                     /* This character failed to match */
            sp++;                         /* Match found, try next one */
        }
    }
    else
    {									/* Match string from table */

        se2 = tp;                  /* Find end of string in table */
        while ( !(*se2 & HIGHBIT)
                || (*se2 == TOPBITCMD) ) /* Go past unmatched in table */
        {
            if ( *se2 == TOPBITCMD )    /* Skip element with high bit on */
                se2++;
            se2++;
        }

        for (;;)
        {
            if ( sp >= se )				/* End of the store? */
            {
                ans = 1;						/* All must have matched */
                break;
            }
            for(sp2 = tp;;)   /* Use a fast pointer into the table */
            {
                if ( sp2 >= se2 )				/* End folowing string? */
                {
                    ans = 0;               /* Must be no match */
                    break;
                }
                if ( *sp2 == TOPBITCMD )  /* Handle possible element with high bit on */
                {
                    sp2++;
                    ch_temp = (*sp2 | HIGHBIT);
                }
                else
                    ch_temp = *sp2;

                if ( (unsigned)(*sp) == (unsigned)(ch_temp) ) /* DAR fixed for doublebyte compare 4/24/96 */
                {
                    ans = 1;             /* Match found */
                    break;
                }

                sp2++;
            }
            if(!ans)
                break;

            sp++;
        }

        tp = se2;
    }			  /* else */
    *tblpnt = tp;								/* Update table pointer */
    return(ans);
} /* End--IfSubset */

void CCCModule::refillinputbuffer()
{
    int nbytesleft;
    register SSINT *cp;   /* Working pointer for replenishing match buffer */
    SSINT ch;

    nbytesleft = matchpntrend - matchpntr;

    if (nbytesleft < maxsrch && !bEndofInputData)
    {
        /* Shift the active buffer contents to the beginning */
        /*  of the buffer, to allow some space */

        if (nbytesleft)
            cp = ssbytcpy( match+BACKBUFMAX, matchpntr, nbytesleft);
        else
            cp = match+BACKBUFMAX;

        while ( cp < match+MAXMATCH-(MAXUTF8BYTES-1) && !bEndofInputData && !bNeedMoreInput)			/* Refill the emptied space */
        {
            ch = inputchar();

            // this actually checks "end of data" for this call in DLL case
            if (!bEndofInputData && !bNeedMoreInput)
                *cp++ = ch;

            // Make sure we have a complete UTF8 character
            if (utf8encoding && ((ch & 0xC0) == 0xC0))
            {
                int AdditionalBytes;

                for (AdditionalBytes= UTF8AdditionalBytes((UTF8)ch); AdditionalBytes != 0 && !bEndofInputData && !bNeedMoreInput; AdditionalBytes--)
                {
                    ch = inputchar();

                    if (!bEndofInputData && !bNeedMoreInput)
                        *cp++ = ch;
                }
            }
        }

        matchpntr = match+BACKBUFMAX;   /* Update pointer */
        matchpntrend = cp;
    }
}

/************************************************************************/
char *CCCModule::sym_name(int number, int type)	/* Return symbolic name for a given number */
/************************************************************************/
//int number;			/* Number of store, switch, group or define */
//int type;			/* 0,1,2 or 3 to indicate which store, switch, group */
/* or define, respectively */
{
    static char name_buffer[20];	/* Buffer used if no symbolic name exists */
    register CC_SYMBOL *sym_pntr;	/* Pointer for traversing the list */

    /* Search appropriate symbol table for a matching number */
    sym_pntr = sym_table[type].list_head;
    while ( sym_pntr != NULL )
    {
        if (sym_pntr->number == number)		/* If matching number found... */
            return( sym_pntr->name );			/* return pointer to name */
        else
            sym_pntr = sym_pntr->next;			/* try next in list */
    }

    /* If number was not found, then return number converted to a string */
    sprintf(name_buffer, "%d", number);
    return( name_buffer );
}

/************************************************************************/
void CCCModule::FwdOmitCommand(SSINT **cppTable, bool *pbOmitDone, bool fwd, bool utf8)
/************************************************************************/
{
    SSINT *tpx;
    int i;

    tpx= *cppTable;

    if (*tpx > maxsrch)
        maxsrch= *tpx;

    refillinputbuffer();

    if (bNeedMoreInput)
    {
        tpx--;
    }
    else
    {
        for ( i = *tpx++; !eof_written && i-- > 0; )
        {
            do
            {
                // if we have already processed all the data, then skip out
                if ( matchpntr == matchpntrend )                // fix CC_041
                {
                    eof_written = TRUE;   // do not keep going any more
                    tpx++;                // skip over argument to omit or fwd
                    *pbOmitDone = TRUE;   // fwd or omit done with this match
                    break;
                }

                if ( fwd )
                    output( *matchpntr);

                matchpntr++;
            } while (utf8 && ((*matchpntr & 0xC0) == 0x80));
        }
    }
    *cppTable= tpx;
}

/****************************************************************************/
void CCCModule::ccin()		/* Do CC until one char output or one table entry performed */
/****************************************************************************/

/* Description:
 *   This procedure does consistent changes until one character has been
 * output, or until one entry in the changes table has been performed.
 *
 * Return values: none
 *
 * Globals input: firstletter -- first letter of the potential match
 *						matchpntr -- pointer into the match buffer
 *						caseless -- boolean: TRUE == ignore case of first letters
 *																 of matches
 *						uppercase -- boolean: TRUE == first letter of the current
 *																  match is upper case
 *						setcurrent -- boolean: TRUE == set of first letters of
 *																	possible matches is
 *																	up to date
 *						storeact -- array of booleans, TRUE == the corresponding
 *																			 store is being used
 *																			 in a left-executable
 *																			 function
 *						letterset -- array of booleans, TRUE == there is a change
 *																			 beginning with the
 *																			 corresponding letter
 *						curgroups -- array of group numbers for groups currently
 *											being searched, stored in the order that the
 *											groups are to be searched
 *						matchpntr -- pointer to the logical beginning of the
 *											match buffer
 *						match -- pointer to the physical beginning of the
 *									  match buffer
 *
 * Globals output: firstletter -- contains the first letter to be checked
 *												for a possible match, set to lower case
 *												if we are doing a caseless search and
 *												it was actually upper case
 *						 uppercase -- if TRUE, firstletter was actually upper case
 *											 but we are ignoring case for matches
 *						 setcurrent -- set to TRUE if the set of first letters is
 *											  up to date
 *						 matchpntr -- updated
 *
 * Error conditions: none
 *
 * Other functions called: completterset -- set up the array of first letters
 *															 of matches
 *									gmatch -- go through a group and try to find
 *													a match
 *									output -- move a byte out of the backup buffer
 *                         ssbytcpy --  move a block of memory
 *
 */

{
    int cg;				/* Group number for stepping through the active groups */
    int match_choice;
    match_choice= -1;

    /* first check if we were in the middle of a replacement */
    if (executetableptr != NULL)
    {

        executetableptr= execute(matchlength, executetableptr, FALSE);		/* Execute replacement */

        /* if we still haven't finished with the replacement then return */
        if (executetableptr != NULL)
            return;
    }

    /* Be sure enough characters are in match buffer */
    /*  to allow for the longest possible match */
    refillinputbuffer();

    if (bNeedMoreInput)
        return;

    /* if nothing in match buffer then first letter is not really valid
    	don't bother with first letter in this case */
    if ((matchpntrend - matchpntr) != 0)    // ADDED 08-01-95 DAR
    {
        firstletter = *matchpntr;						/* Set up first letter */

        if (caseless) {											/* Caseless */
            if ( isupper(firstletter) )
            {
                firstletter = (char)tolower( firstletter );
                uppercase = TRUE;
            }
            else
                uppercase = FALSE;
		}
        if ( !setcurrent && !storeact[curstor] )
            completterset();				/* Don't recompute while storing */

        if ( letterset[(firstletter & 0xff)] || !setcurrent )
            for (cg = 1; cg <= numgroupson; cg++ )		/* Step through groups */
                if ((match_choice= gmatch(curgroups[cg])) >= 0)
                    break;
    }
    else
    {
        for (cg = 1; cg <= numgroupson; cg++ )		/* Step through groups */
            if ((match_choice= gmatch(curgroups[cg])) >= 0 )
                break;
    }

    if (match_choice >= 0)
    {
        matchlength= mchlen[match_choice];
        stacklevel= 0;
        executetableptr= execute(matchlength, tblxeq[match_choice], FALSE);		/* Execute replacement */
    }
    else
    {
        if ((matchpntrend - matchpntr) == 0)
        {
            // if we have no more input data
            if ( bEndofInputData )
            {
                curstor = 0;
                numgroupson = 0;       /* All groups off for any further endfile */
                setcurrent = FALSE;
                eof_written = TRUE;
            }
        }
        else
            output( *matchpntr++);					/* No match, so output a char */
    }

} /* End--ccin */

/****************************************************************************/
void CCCModule::writechar()							/* Write a char from the backup buffer */
/****************************************************************************/

/* Description:
 *						This procedure writes a single character from the backup
 *					buffer.	If just CC, it goes to the output file.
 *					If Manuscripter, it goes into Manuscripter's buffer.
 *
 * Return values: none
 *
 * Globals input: backoutptr -- output pointer for the backup buffer
 *						backbuf -- pointer to the beginning of the backup buffer
 *  (MS only)		mandisplay -- boolean: TRUE == echoing MS code to the screen
 *
 * Globals output: backoutptr -- updated
 *
 * Error conditions: none
 *
 * Other functions called:	out_char -- write a char to the output file
 *									storch -- add a char to a store
 *
 */

{
    if (*backoutptr == CARRIAGERETURN && !binary_mode)		/* If NEWLINE... */
        out_char((SSINT)'\n');                     /* output a newline */
    else
        out_char(*backoutptr);           /* else output the char itself */

    if (++backoutptr >= backbuf+BACKBUFMAX)	/* Handle wraparound */
        backoutptr = backbuf;						/* in the ring buffer */

} /* End--writechar */

/****************************************************************************/
void CCCModule::out_char(SSINT ch )                           /* Output a single character  */
/****************************************************************************/
//SSINT ch;          /* Char to be output */
{
    SSINT ch_first;             /* first of doublebyte pair of characters */

    /* if we are in doublebyte mode, and high order half of element
       is non-zero, then output doublebyte element as two bytes    */

    if (doublebyte_mode && ((ch / 256 != 0) || (doublebyte1st == 0 && doublebyte2nd == 0)))
    {
        ch_first = (ch >> 8) & 0x00ff;
        out_char_buf((char) ch_first);  // output first of two characters
        ch = ch & 0x00ff;

        /* If after we have done everything the doublebyte characters
           that we put out as two characters no longer match the doublebyte
           input criteria, then alert the user with warning message.  */
        if ((ch_first < doublebyte1st) ||
                ((ch_first == doublebyte1st) && (ch < doublebyte2nd)))
            Process_msg(17, wxNullStr, 0);
        // if we likely will just overflow our output buffer by one byte
        // (the second half of a doublebyte element) then just store that
        // byte and set a flag, and pick it up next time, instead of
        // allocating space etc to improve performance.
        if (( nUsedOutBuf >= nMaxOutBuf ) &&
                ( nCharsInAuxBuf == 0 ) && ( !bSavedDblChar ))
        {
            dblSavedChar = (char) ch;   // save character to output next time
            bSavedDblChar = TRUE;       // indicate we have a saved character
        }
        else
            out_char_buf((char) ch);    // output the character
    }
    else
    {
        out_char_buf((char) ch);       // output the character
    }
}

int CCCModule::wfflush(WFILE * stream)
{
	// whm modified 31Mar08 to use wxWidgets' wxFile methods. The original function uses fwrite (for UNIX)
	// and _lwrite otherwise. Although wxFile has a Flush() method, we'll use the original's WFILE
	// struct's design to flush the buffers.
    int nBytesWritten; //ReturnCode;
    if (stream->fWrite && stream->iBufferEnd != UNGETBUFFERSIZE)
    {
        // In original code if defined(UNIX) the routine used:
        //fwrite(stream->Buffer + UNGETBUFFERSIZE, stream->iBufferEnd - UNGETBUFFERSIZE, 1, stream->hfFile)
        // and other systems used: 
		//ReturnCode= _lwrite(stream->hfFile, stream->Buffer + UNGETBUFFERSIZE, stream->iBufferEnd - UNGETBUFFERSIZE);
		// The wxFile::Write() method override that uses a buffer returns the number of bytes actually
		// written. The _lwrite() function also returns number of bytes written unless there is an
		// error, in which case the return code is -1.
        nBytesWritten = stream->hfFile->Write(stream->Buffer + UNGETBUFFERSIZE, stream->iBufferEnd - UNGETBUFFERSIZE);
        if (nBytesWritten == -1)
        {
            wxMessageBox(_T("Error flushing file"), _T("CC.DLL"), wxICON_ERROR | wxOK); //MessageBox(NULL, "Error flushing file", "CC.DLL", MB_OK);
            return EOF;
        }
    }

    stream->iBufferFront= UNGETBUFFERSIZE;
    stream->iBufferEnd= UNGETBUFFERSIZE;

    return 0;
}

/****************************************************************************/
void CCCModule::storestring(int search, char * string, int sLen)
/****************************************************************************/
/*
 *  NOTE: This written by DAR 4/96
 *
 */

{
    SSINT *valstart = NULL;        /* Start of a quoted string, for valnum()      */
    char valbuf[20];        /* Dummy buffer for valnum()                   */
    long val_dummy;         /* Dummy for valnum()                          */
    SSINT double_value;     /* Accumulate potential doublebyte elements    */
    int number;
    char *tmp;

    if ( was_math )
        valstart = tloadpointer;        /* Set up to check the string to
    							* see if it's a valid number
    							*/
    was_string = TRUE;

    /* store low order byte (and zero out high order byte if any)       */
    while ( sLen )
    {
        /* if doublebyte mode and we are on the left side of replacement */
        if (( doublebyte_mode ) && ( search ) && sLen > 1)
        {
            /* first get possible first half of doublebyte pair, and
            	then test whether these two meet the criteria or not    */
            double_value = (SSINT) (*string & 0x00ff);
            string++;
            sLen--;

            if (( ((SSINT) (*string & 0x00ff)) >= doublebyte2nd ) &&
                    ( double_value >= doublebyte1st ))
            {
                double_value = (double_value * 256)
                               | ((SSINT) (*string & 0x00ff));

                string++;
                sLen--;

                if ( double_value & HIGHBIT )
                    storechar( TOPBITCMD );

                storechar((SSINT) (double_value & 0x7fff));

            }
            else
                /* doublebyte, but did not meet both criteria tests     */
            {
                storechar((SSINT)(double_value & 0x00ff));
            }
        }
        else
        {
            storechar ( (SSINT)((*string) & 0x00ff));
            string++;
            sLen--;
        }
    }

    if ( was_math )
    {                /* Be sure it's a valid number in the
        						* range -1,999,999,999 to +1,999,999,999
        						*/

        storechar( SPECPERIOD ); /* Temporarily put a command after the string */

        if ( (number = valnum( valstart, valbuf, 20, &val_dummy, FALSE))
                != TRUE )
        {
            if ( number == -1 )
            {
                /* this hack adjusts the cursor in the error message to point
                   either to the end of the failing item, or at least to point
                   before the subsequent item (instead of pointing after the
                   subsequent item which gives the user very bad information). */
                tmp = parse2pntr;
                parse2pntr = parsepntr;  /* back up to start of this entry */
                parse2pntr--;            /* back up                        */
                parse2pntr--;            /*   to end of prior element      */
                err( "Number too big, must be less than 2,000,000,000" );
                parse2pntr = tmp;        /* restore this to where we were */
            }
            else
            {
                /* this hack adjusts the cursor in the error message to point
                   either to the end of the failing item, or at least to point
                   before the subsequent item (instead of pointing after the
                   subsequent item which gives the user very bad information). */
                tmp = parse2pntr;
                parse2pntr = parsepntr;  /* back up to start of this entry */
                parse2pntr--;            /* back up                        */
                parse2pntr--;            /*   to end of prior element      */
                err( "Invalid number for arithmetic" );
                parse2pntr = tmp;        /* restore this to where we were */
            }
        }

        tloadpointer--;          /* "Un-store" the SPECPERIOD from above */
        was_math = FALSE;
    } /* End--previous command was a math operator */
} /* End--storestring */

/****************************************************************************/
void CCCModule::err(char *message)											/* Display an error message */
/****************************************************************************/
//char *message;

/* Description -- Give a 3-line error message in the form of:
 *
 *						  current line
 *						  pointer to current parsing position
 *						  error message
 *
 *						Set errors to TRUE.
 *
 * Return values: none
 *
 * Globals input: errors -- global error flag
 *
 * Globals output: errors set to TRUE
 *
 * Error conditions: if this routine gets called, there must be one.
 *
 * Other functions called: none
 *
 */
{
    MSG_STRUCT_S_S *structss;
    register char *cp;         // points to the source of the error info
    register char *lp;         // points to the target of the error info
    {
        lp= errorLine;

        for ( cp = line; *cp != '\0'; )          // copy current (erroneous) line
        {
            *lp++ = *cp++;
        }
        *lp++ = '\n';

        for ( cp = line; cp++ < parse2pntr; )    // Blanks until
            *lp++ = ' ';                          // current parsing position
        *lp++ = '^';                             // ^ shows where error is
        *lp++ = '\0';                            // (end of string)

        Msg_s_s.string1 = wxString(errorLine,wxConvISO8859_1); //&errorLine[0]; // whm: TODO: check this!!!
        Msg_s_s.string2 = wxString(message,wxConvISO8859_1); // whm: TODO: check this!!!
        structss = &Msg_s_s;
        Process_msg(53, wxNullStr, (unsigned long) structss);
    }
    errors = TRUE;

} /* End--err */

/****************************************************************************/
int CCCModule::hex_decode(char * pstr)  /* Decode a hexadecimal sequence        */
/****************************************************************************/

/*
 * Description:
 *		Check the length of the string (which can be of any length) to find out
 * how many digits we have.  If there is an odd number of digits,
 * right-justify the number (the first byte will have a left nibble of zero).
 * The result is a string in pstr
 *		If we encounter an invalid hex digit, display an error message.
 *
 * Note: When we get here, we know we have a leading 'x' or 'X', followed
 *			by at least one valid hex digit.
 *
 *
 *
 * Return values: length of resulting string
 *
 * Globals input: parsepntr -- pointer to the beginning of the current element
 *						parse2pntr -- pointer to just beyond the current element
 *
 * Globals output: parsepntr -- updated based on what we found
 *
 * Error conditions: If an invalid hex digit is found, an error message will
 *							be displayed and errors will be set to TRUE, via the
 *							routine err().
 *
 * Other procedures called: err -- display an error message
 *									 a_to_hex -- convert an ASCII char to hex
 *
 */
{
    register char next_byte;   /* The next "byte" (element) to store */
    char * pstrstart= pstr;

    parsepntr++;				/* Skip over the leading 'x' */

    if ( odd( parse2pntr - parsepntr ) )
    {
        next_byte = a_to_hex( *parsepntr++ );  /* Handle the odd digit */
        *pstr++= next_byte;
    }

    next_byte = 0;

    while ( parsepntr < parse2pntr )
    {
        if ( isxdigit( *parsepntr ) )
            next_byte = next_byte | (((a_to_hex( *parsepntr++ )) << 4) & 0x00ff);
        else
        {
            err( "Invalid hexadecimal digit" );
            return 0;
        }

        if ( isxdigit( *parsepntr ) )
        {
            next_byte = next_byte | (a_to_hex( *parsepntr++ ) & 0x00ff);
        }
        else
        {
            err( "Invalid hexadecimal digit" );
            return 0;
        }

        *pstr++= next_byte;
        next_byte = 0;                /* clear this out for next time */
    } /* End--while */

    return pstr - pstrstart;

} /* End--hex_decode */

/****************************************************************************/
int CCCModule::ucs4_decode(char * pstr)  /* Decode a usc4 string converting it to utf8 */
/****************************************************************************/

/*
 * Description:
 *		Check the length of the string (which can be of any length) to find out
 * how many digits we have.  If there is an odd number of digits,
 * right-justify the number (the first byte will have a left nibble of zero).
 * The result is a string in pstr
 *		If we encounter an invalid hex digit, display an error message.
 *
 * Note: When we get here, we know we have a leading 'u' or 'u', followed
 *			by at least one valid hex digit.
 *
 *
 *
 * Return values: length of resulting string
 *
 * Globals input: parsepntr -- pointer to the beginning of the current element
 *						parse2pntr -- pointer to just beyond the current element
 *
 * Globals output: parsepntr -- updated based on what we found
 *
 * Error conditions: If an invalid hex digit is found, an error message will
 *							be displayed and errors will be set to TRUE, via the
 *							routine err().
 *
 * Other procedures called: err -- display an error message
 *									 a_to_hex -- convert an ASCII char to hex
 *
 */
{
    register unsigned char next_byte;   /* The next "byte" (element) to store */
    UCS4 ch;
    int count;

    parsepntr++;				/* Skip over the leading 'u' */

    if ( odd( parse2pntr - parsepntr ) )
        ch = a_to_hex( *parsepntr++ );  /* Handle the odd digit */
    else
        ch= 0;

    next_byte = 0;


    while ( parsepntr < parse2pntr )
    {
        if ( isxdigit( *parsepntr ) )
            next_byte = next_byte | (((a_to_hex( *parsepntr++ )) << 4) & 0x00ff);
        else
        {
            err( "Invalid UCS4 digit" );
            return 0;
        }

        if ( isxdigit( *parsepntr ) )
        {
            next_byte = next_byte | (a_to_hex( *parsepntr++ ) & 0x00ff);
        }
        else
        {
            err( "Invalid UCS4 digit" );
            return 0;
        }

        ch= ch << 8 | next_byte;
        next_byte = 0;                /* clear this out for next time */
    } /* End--while */

    count= UCS4toUTF8(ch, (UTF8 *)pstr); // whm added (UTF8 *) cast

    return count;

} /* End--ucs4_decode */

/****************************************************************************/
void CCCModule::decimal_decode(int * number )	 /* Decode a decimal number */
/****************************************************************************/
//register int *number;
/*
 * Description:
 *			Skip the leading 'd' (indicating decimal). Then go through the
 *		string, decoding one digit at a time.	If a non-digit is encountered
 *		before the end of the element, give an error message and return.
 *
 * Note: We know before we get called that we have a leading 'd' or 'D',
 *			followed by at least one digit.
 *
 * Return values: *number will contain whatever was decoded.
 *
 * Globals input: parsepntr -- pointer to the beginning of the current element
 *						parse2pntr -- pointer to just beyond the current element
 *
 * Globals output: parsepntr -- updated based on what we found
 *
 * Error conditions: If an invalid digit is found or the number is greater than
 *							255, an error message will be displayed
 *							and errors will be set to TRUE.
 *
 * Other procedures called: err -- display an error message
 *
 */
{
    parsepntr++;			/* Skip over the 'd' or 'D' */

    *number = 0;			/* Initialize target */

    while ( parsepntr < parse2pntr )				/* Decode the whole thing */
    {
        if ( isdigit( *parsepntr ) )
        {
            *number = (*number * 10) + (*parsepntr++ - '0');
            if ( *number > 255 )
            {
                err( "Decimal number too big, must be less than 256");
                return;
            }
        }
        else
        {
            err( "Invalid decimal digit");
            return;
        }
    } /* End--while */
} /* End--decimal_decode */

/****************************************************************************/
void CCCModule::octal_decode(register int *number)			/* Decode an octal number */
/****************************************************************************/

/* Description:
 *						Go through the current element, which is assumed to be
 *					in octal, and build an 8-bit binary number.
 *
 * Return values: number will be set to the result
 *
 * Globals input: parsepntr -- pointer to the current element being parsed
 *
 * Globals output: parsepntr -- updated
 *
 * Error conditions: any errors will be reported via err.
 *
 * Other functions called: err -- display an error message
 *
 */

{
    *number = 0;						/* Initialize number */
    while ( parsepntr < parse2pntr )		/* Decode the whole thing */
    {
        if ( *parsepntr >= '0' && *parsepntr <= '7' )
        {
            *number = (*number * 8) + (*parsepntr++ - '0');
            if ( *number > 255 )
            {
                err( "Octal number too big, must not exceed 377");
                return;
            }
        }
        else
        {
            err( "Invalid octal digit");
            return;
        }
    } /* End--while */
} /* End--octal_decode */

/****************************************************************************/
void CCCModule::buildkeyword()			/* Copy suspected keyword into the array keyword */
/****************************************************************************/
/* Description -- Loop until we get to the end of the current element
 *						  (loop exit is via a break)
 *
 *							If current character is (
 *								Break.
 *							If we're not fixing to overflow the array keyword
 *								Copy the character into keyword.
 *							Increment parsepntr (pointer into current element).
 *							Put a string-terminating \0 at the current end
 *							  of keyword.
 *
 * Return values: none
 *
 * Globals input: keyword -- array used for checking keywords
 *						parsepntr -- pointer to the current element
 *						parse2pntr -- pointer to just beyond the current element
 *
 * Globals output: keyword -- now contains the suspected keyword
 *						 parsepntr -- points to one char beyond the end of
 *											 the suspected keyword
 *
 * Error conditions: if the suspected keyword will overflow the array keyword,
 *							  the excess will be ignored
 *
 * Other functions called: none
 *
 */

{
    register char *kp;

    for ( kp = keyword; parsepntr < parse2pntr; )
    {
        if ( *parsepntr == '(' )		/* End of suspected keyword */
            break;

        if ( kp < keyword+sizeof(keyword)-1 )	/* Don't overflow array */
            *kp++ = (char)tolower(*parsepntr);

        parsepntr++;	/* Keep moving through the element, whether we're */
        /*  storing what we see or not */

    }
    *kp = '\0';						/* Put terminator in keyword array */

} /* End--buildkeyword */

/****************************************************************************/
void CCCModule::stornoarg(char comand, char dummy1, char dummy2) /* Store commands which take no args */
/****************************************************************************/
//char	comand;	/* Command to store */
//char	dummy1, dummy2;	
								/* These are only used by storarg, but they
								 *   are passed to this procedure because it is called
								 *   using a pointer to a function.
								 */

/* Description -- If the comand has no arguments
 *							Store it in the table, using storechar.
 *						Else
 *							Give an error message, using err.
 *
 * Return values: none
 *
 * Globals input: parsepntr -- pointer to the last element parsed
 *
 * Globals output: none altered
 *
 * Error conditions: If there was an error, errors will be set to TRUE
 *							  via a call to err.
 *
 * Other functions called: storechar -- store an element into the internal
 *														change table
 *									err -- display an error message
 *
 */

{
	dummy1 = dummy1; // whm added to avoid compiler warnings
	dummy2 = dummy2; // whm added to avoid compiler warnings
    if ( *parsepntr == ' ' || !*parsepntr )
        storechar( comand);
    else
        err( "Illegal parenthesis");

} /* End--stornoarg */

/****************************************************************************/
void CCCModule::storarg( char comand, char sym_args, char table_head ) /* Store commands taking args */
/****************************************************************************/
//char	comand,		/* Command to be stored */
//sym_args,	/* Boolean: TRUE == command can take symbolic arguments */
//table_head; /* Index into the symbol table */

/* Description:
 *						This procedure stores commands with arguments such as
 *					if, set, and clear.	It is used during compiling to parse
 *					out these arguments.  It first checks for the open paren,
 *					then stores the number.
 *
 * Return values: none
 *
 * Globals input: parsepntr -- pointer to the current element being decoded
 *
 * Globals output: parsepntr -- updated
 *
 * Error conditions: any errors will set errors to TRUE via err.
 *
 * Other functions called: storechar -- store an element into the internal
 *														change table
 *									err -- display an error message
 *
 */

{
    register SSINT number;   /* number for building argument */
    int flag;                /* Flag for no digits between parentheses */
    char *pPredefined;       /* used to check for predefined store for out */
    bool bPredefinedFound;   /* TRUE if we find a predefined store for out */
    int  nPredefinedType;    /* if a predefined special store is found for
                                out command, this saves what type it is    */

    if ( *parsepntr != '(' )
    {
        //ptokenstart= parsepntr; // ptokenstart is never referenced in CCCModule code
        err("Missing parenthesis");
        return;
    }

    for ( ;; )			/* Do forever (exit is via a break, below) */
    {
        bPredefinedFound = FALSE;        // have not found special store
        nPredefinedType = 0;             // no special store type found yet
        if ( sym_args )
        {										/* Command can take symbolic arguments */

            // check to see if we have predefined stores with any of the
            // commands that support the predefined stores
            if (( comand == OUTCMD ) || ( comand == OUTSCMD )
                    || ( comand == IFEQCMD ) || ( comand == IFNEQCMD )
                    || ( comand == IFGTCMD ) || ( comand == IFNGTCMD )
                    || ( comand == IFLTCMD ) || ( comand == IFNLTCMD )
                    || ( comand == WRSTORECMD ))
            {
                pPredefined = parsepntr;   // point to delimeter before argument
                pPredefined++;             // point to argument for the command

                // do not continue checking for predefined stores if what we
                // are pointing at is not long enough for them.  Since the
                // predefined stores all have lengths of 13 or 14 bytes, and are
                // followed by a ',' or ')', then we can test for 14 bytes.
                if ( strlen(pPredefined) >= 14 )
                {
                    if ( strncmp(pPredefined, "cccurrentdate", 13) == 0 )
                    {
                        bPredefinedFound = TRUE;
                        nPredefinedType = CCCURRENTDATE;
                    }
                    else if ( strncmp(pPredefined, "cccurrenttime", 13) == 0 )
                    {
                        bPredefinedFound = TRUE;
                        nPredefinedType = CCCURRENTTIME;
                    }
                    else if ( strncmp(pPredefined, "ccversionmajor", 14) == 0 )
                    {
                        bPredefinedFound = TRUE;
                        nPredefinedType = CCVERSIONMAJOR;
                    }
                    else if ( strncmp(pPredefined, "ccversionminor", 14) == 0 )
                    {
                        bPredefinedFound = TRUE;
                        nPredefinedType = CCVERSIONMINOR;
                    }
                }
            }

            if ( (number = symbol_number( table_head, sym_args )) != 0 ) /* Get the number */
            {
                flag = 1;				/* Say everything is OK for below */
            }
            else
                return;					/* We had an error */

        } /* End--command takes symbolic arguments */
        else
        {
            if ( comand == DOUBLECMD )
                number = parsedouble( &flag );  /* set number and flag here */
            else
            {
                number = flag = 0;   /* Initialize for decoding number */
                while ( *++parsepntr >= '0' && *parsepntr <= '9' )
                {
                    number = 10 * number + *parsepntr - '0';   /* Decode the number */
                    flag = 1;                     /* (say we found a digit) */
                }
            }
        } /* End--command doesn't take symbolic arguments */

        if ( !flag )							/* Just 2 parentheses */
            err("Bad number");
        else if (( number > MAXARG ) && (comand != DOUBLECMD))  /* Other errors */
            err( "Number too big");
        else if ( number == 0 )
            err( "Number cannot be zero");
        else
        {
            /* Do doublebyte "pre-parsing" here.  We need to know early
               what out doublebyte args are, so determine that here.
            */
            if ( comand == DOUBLECMD)
            {
                if ( number > MAXDBLARG )
                    err( "Doublebyte number too big");
                else if ( doublebyte1st == 0)
                    doublebyte1st = number;
                else
                    if ( doublebyte2nd == 0)
                        doublebyte2nd = number;
                    else
                        Process_msg(18, wxNullStr, 0);
            }
            storechar( comand);           /* store command  */
            storechar( number);           /* store argument */

            // if we found a predefined store with the out command, mark that
            if ( bPredefinedFound == TRUE )
                storepre[number] = nPredefinedType;

            if ( *parsepntr == ',')							/* Multiple arguments */
                continue;
            else if ( *parsepntr != ')' )
            {
                //ptokenstart= parsepntr; // ptokenstart is never referenced in CCCModule code
                err( "No close parenthesis");			/* Error */
            }
            else
                parsepntr++;							/* Skip the close parenthesis */
        } /* End--else */

        break;											/* Exit from the loop */
    }

} /* End--storarg */

/****************************************************************************/
void CCCModule::storoparg( char comand, char dummy1, char dummy2) /* Store commands taking optional args */
/****************************************************************************/
//char	comand;		/* Command to be stored */
//char	dummy1, dummy2;	
								/* These are only used by storarg, but they
								 *   are passed to this procedure because it is called
								 *   using a pointer to a function.
								 */

/* Description:
 *						This procedure stores commands with optional arguments,
 *                                      namely fwd, back, and omit.
 *                                      It stores an argument
 *                                      of 1 if none is found.
 *
 * Return values: none
 *
 * Globals input: parsepntr -- pointer to the current element
 *
 * Globals output: table updated
 *
 * Error conditions: any errors will be reported by storarg
 *
 * Other functions called: storarg -- store a command having an argument
 *                         storechar -- store an element into the internal
 *														change table
 *
 */

{
	dummy1 = dummy1; // whm added to avoid compiler warnings
	dummy2 = dummy2; // whm added to avoid compiler warnings
    if ( *parsepntr == '(' )
        storarg(comand, FALSE, 0);				/* Arg with the command */
    else
    {
        storechar(comand);				/* No arg, so use a default of 1 */
        storechar(1);
    }

} /* End--storoparg */

/****************************************************************************/
void CCCModule::stordbarg( char comand, char dummy1, char dummy2) /* Store dooublebyte command, args  */
/****************************************************************************/
//char	comand;		/* Command to be stored */
//char	dummy1, dummy2;	
								/* These are only used by storarg, but they
								 *   are passed to this procedure because it is called
								 *   using a pointer to a function.
								 */

/* Description:
 *                This procedure stores the doublebyte command, which can take
 *                                      0, 1, or 2 arguments.  It stores an argument
 *                                      of 1 if none is found.
 *                This routine is separated out since we need to do the
 *                parsing for this command right at first so we know whether
 *                to make doublebyte elements out of the store argument.
 *
 * Return values: none
 *
 * Globals input: parsepntr -- pointer to the current element
 *
 * Globals output: table updated
 *
 * Error conditions: any errors will be reported by storarg
 *
 * Other functions called: storarg -- store a command having an argument
 *                         storechar -- store an element into the internal
 *														change table
 *
 */

{
	dummy1 = dummy1; // whm added to avoid compiler warnings
	dummy2 = dummy2; // whm added to avoid compiler warnings
    doublebyte_mode = TRUE;

    if ( *parsepntr == '(' )
    {
        storarg(comand, FALSE, 0);				/* Arg with the command */
    }
    else
    {
        storechar(comand);				/* No arg, so use a default of 1 */
        storechar(1);
    }

} /* End--stordbarg */

/****************************************************************************/
char *CCCModule::cmdname(char cmd, int search)	/* Get name for given command code */
/****************************************************************************/
//char cmd;		/* Command code */
//int search;		/* Boolean --  TRUE == we're on the search side of a change, */
/* FALSE == we're on the replacement side */
{
    struct cmstruct *cmpt;

    /* Point to proper search table */
    if (search)
        cmpt = cmsrch;
    else
        cmpt = cmrepl;

    /* Look for a matching command code */
    for ( ; cmpt->cmname; cmpt++ )
    {
        if (cmd == cmpt->cmcode)
            return(cmpt->cmname);
    }

    /* If no match was found return a bad name */
    return("UNKNOWN");
}

/*************************************************************************/
int CCCModule::symbol_number(int sym_type, char sym_use) /* Get number for symbolic name */
/*************************************************************************/
//int sym_type;	/* Index into the sym_table array (0..3) */
//char sym_use;	/* One of two equates DEFINED or REFERENCED, from ms01.h */

/*
 * Description:
 *						Parse the next element within a command, search the
 *			symbol table for it and add it to the table if necessary.  Return
 *			the number (1..MAXARG) for the element.
 *
 *  Note: if the element is a numeric value, the number returned may or may not
 *				be the same as the element, since the number may have already been
 *				used.
 *						  (Talk about confusion!)
 *
 * Return values: if successful, return an integer in the range 1..MAXARG
 *						if error, return 0.
 *
 * Globals input:
 *						parsepntr -- pointer to the current element being decoded
 *						sym_table -- symbol table head array
 *
 * Globals output:
 *						parsepntr -- updated
 *						sym_table -- updated
 *
 * Error conditions: if an error occurs, an error message will be displayed
 *							  via the procedure err(), and 0 will be returned.
 *
 * Other procedures called:
 *									err -- display an error message
 *
 */

{
    register char *beginning_of_symbol,  /* Pointers to name/number */
    *end_of_symbol;

    register CC_SYMBOL *cursym;  /* Used for traversing symbol table */

    CC_SYMBOL *sym_temp;		/* Used while adding a new element to the table */

    char replaced_char;		/* Temp for char beyond end of symbol */
    int overload;				/* Boolean: TRUE == numeric value of the element
    								*							has already been assigned
    								*/

    int  numval;		  /*	 (possible) numeric value, if an actual numeric arg
    							* was given
    							*/

    int i;		/* Integer temp */

    overload = FALSE;		/* Initialize locals */

    /* Find the beginning of the name/number */

    for ( beginning_of_symbol = ++parsepntr; ; beginning_of_symbol++ )
        if ( (*beginning_of_symbol == CARRIAGERETURN)
                || ( *beginning_of_symbol == ',' ) || ( *beginning_of_symbol == ')' )
                || !isspace( *beginning_of_symbol ) )
        {
            if ( (beginning_of_symbol == parsepntr)
                    && ( *beginning_of_symbol == ')' ) )
            {
                err( "Missing parameter" );	  /* Nothing there */
                return( 0 );
            }
            else
                break;												/* Found the beginning */
        }

    /* Now find the end */

    for ( end_of_symbol = beginning_of_symbol; ; end_of_symbol++ )
        if ( (*end_of_symbol == CARRIAGERETURN)
                || ( *end_of_symbol == ',' ) || ( *end_of_symbol == ')' )
                || isspace( *end_of_symbol ) )
            break;

    /*   Temporarily convert it to a
    	* NUL-terminated string
    	*/
    replaced_char = *end_of_symbol;
    *end_of_symbol = '\0';

    numval = atoi( beginning_of_symbol );	/*  Try converting to a numeric value */

    if ( (i = search_table( sym_type, sym_use,
                           beginning_of_symbol, 0, NAME_SEARCH )) != 0 )
    {
        *end_of_symbol = replaced_char;	 /* Get rid of the NUL */

        while ( isspace( *end_of_symbol ) )			/* Skip trailing white space */
            end_of_symbol++;

        parsepntr = end_of_symbol;			 /* Update parsepntr */
        return( i );
    }

    /* Number already assigned? */
    overload = search_table( sym_type, sym_use, NULL, numval, NUMBER_SEARCH );

    if ( (sym_temp = (CC_SYMBOL *) malloc( sizeof(CC_SYMBOL) )) != NULL )
    {
        sym_temp->next = NULL;				/* Mark the new end of the list */

        if ( !overload && (numval != 0) && (numval <= MAXARG) )
            sym_temp->number = numval;			 /* Use its actual numeric value */
        else
        {											 /* Assign it one */
            i = sym_table[ sym_type ].next_number;
            /* Find the next available number */
            while ( search_table( sym_type, sym_use, NULL, i, NUMBER_SEARCH ) )
                i--;
            sym_temp->number = i;
            if ( i <= 0 )
            {									/* No number available */

                *end_of_symbol = replaced_char;		/* Get rid of the NUL */
                switch( sym_type )
                {
                case STORE_HEAD:
                    err( "Too many stores" );
                    break;
                case SWITCH_HEAD:
                    err( "Too many switches" );
                    break;
                case DEFINE_HEAD:
                    err( "Too many defines" );
                    break;
                case GROUP_HEAD:
                    err( "Too many groups" );
                    break;
                }	/* End--invalid number */

                return( 0 );		/* Error return */
            }
            /* Update next available number */
            sym_table[ sym_type ].next_number = --i;
        }

        sym_temp->name = (char *) malloc(strlen(beginning_of_symbol) + 1);
        if ( sym_temp->name != NULL )
        {
            strcpy( sym_temp->name, beginning_of_symbol );	/* Copy the name */

            *end_of_symbol = replaced_char;			/* Get rid of the NUL */
        }
        else
        {
            *end_of_symbol = replaced_char;			/* Get rid of the NUL */

            err( "Unable to allocate space for symbolic name" );	/* No more space */
            return( 0 );
        }
        sym_temp->use = sym_use;						/* Set initial usage */

    } /* End--successful allocation of new symbol */
    else
    {
        *end_of_symbol = replaced_char;			/* Get rid of the NUL */

        err( "Unable to allocate space for symbolic name");
        return( 0 );			  /* Error, unable to allocate space */
    }

    if ( (cursym = sym_table[ sym_type ].list_head) == NULL )
        sym_table[ sym_type ].list_head = sym_temp;	 /* List was empty */
    else
    {
        while ( cursym->next != NULL )		 /* Find the end of the list */
            cursym = cursym->next;

        cursym->next = sym_temp;	/* Add the element on to the end of the list */
    }

    while ( isspace( *end_of_symbol ) )		/* Skip trailing white space */
        end_of_symbol++;

    parsepntr = end_of_symbol;		/* Update parsepntr */

    return( sym_temp->number );	 /* Return the number for the new element */
} /* End--symbol_number */

/****************************************************************************/
SSINT CCCModule::parsedouble (int *myflag)            /* parse doublebyte arguments */
/****************************************************************************/
//int *myflag;      /* Return 1 for found valid digit, 0 otherwise  */

/* Description:
 *                This procedure does the parsing for the arguments for  
 *                the doublebyte command.  This is passed whatever is
 *                within the parenthesis and commas.  The default is to 
 *                parse in octal, but we will parse in decimal or hex if
 *                the input is preceeded by 'd' or 'x' (or 'D' or 'X').
 *
 * Return values: returns the numeric value parsed
 *                *myflag is set to 0, or 1 if found valid digit(s)
 *
 * Globals input: parsepntr -- pointer to the current element
 *
 * Globals output: parsepntr is updated to point beyond last valid digit
 *
 * Error conditions: any errors will be reported by storarg
 *
 */

{
    int answer = 0;    /* value to return  */
    int num;

    *myflag = 0;       /* denote no valid digits found yet */

    if ( ( *++parsepntr == 'd' ) || ( *parsepntr == 'D' ) )
    {
        /* user wants number treated as a decimal number */
        while ( *++parsepntr >= '0' && *parsepntr <= '9' )
        {
            answer = 10 * answer + *parsepntr - '0';   /* Decode number */
            *myflag = 1;                      /* (say we found a digit) */
        }
        return(answer);
    }

    else if ( ( *parsepntr == 'x' ) || ( *parsepntr == 'X' ) )
    {
        /* user wants number treated as hexadecimal number */
        num = a_to_hex(*++parsepntr);   /* get numeric value of hex char */
        while ( (num >= 0) && (num <= 15) )
        {
            answer = 16 * answer + num;     /* Decode hex number */
            *myflag = 1;                    /* (say we found a digit) */
            num = a_to_hex(*++parsepntr);   /* get numeric value of hex char */
        }
        return(answer);

    }

    else
    {
        /* default to assuming octal number      */
        while ( *parsepntr >= '0' && *parsepntr <= '9' )
        {
            /* first check to validate we have legal octal number */
            if ( ( *parsepntr == '8') || ( *parsepntr == '9' ) )
                err( "Error - doublebyte argument is illegal octal number");
            answer = 8 * answer + *parsepntr - '0';    /* Decode number */
            *myflag = 1;                      /* (say we found a digit) */
            parsepntr++;
        }
        return(answer);
    }
}

/*************************************************************************/
int CCCModule::search_table(int table_index, char sym_usage, char *sym_name, 
							int sym_num, int type_of_search)
/*************************************************************************/
//int table_index;	  /* index into sym_table array */
//char sym_usage,	  /* either REFERENCED or DEFINED */
//*sym_name;	  /* if (type_of_search) name to be searched for */
//int sym_num;		  /* if ( !type_of_search ) number to be searched for */
//int type_of_search; 
						/* 1 == search for name match
							* 0 == search for number match
							*/
/*
 * Description:	Search the symbol table, looking for a match of either
 *					 names (type_of_search == 1) or numbers (type_of_search == 0).
 *
 * Return values:
 *					 If a match is found, return the number of the symbol,
 *					 otherwise return zero.
 *
 *					 If searching by name and a match is found,
 *						OR the usage into the use field of the symbol
 *
 * Globals input: sym_table -- symbol table head array
 *
 * Globals output: none
 *
 * Error conditions: if no match is found, zero will be returned
 *
 * Other procedures called: none
 *
 */
{
    register CC_SYMBOL *list_pointer;  /* Pointer for traversing the list */

    list_pointer = sym_table[ table_index ].list_head;

    for ( ; list_pointer != NULL; list_pointer = list_pointer->next )
    {
        if ( type_of_search )
        {												 /* Looking for a name match */
            if ( !strcmp( list_pointer->name, sym_name ) )
            {
                /* Add the current usage to the use field
                *	 of the symbol, to allow checking for
                *	 used-but-not-defined symbols
                *		(typically caused by typos)
                */
                list_pointer->use = list_pointer->use | sym_usage;
                return( list_pointer->number );			  /* names match */
            }
        }
        else
        {												 /* Looking for a number match */
            if ( list_pointer->number == sym_num )
                return( list_pointer->number );			  /* numbers match */
        }
    } /* End--for */

    return( 0 );	/* No match found */

} /* End--search_table */

/****************************************************************************/
int CCCModule::a_to_hex(register char ch )	/* Convert an ASCII char to hex */
/****************************************************************************/
{
    ch = (char)toupper( ch );
    if ( isdigit( ch ) )
        return( ch - '0' );
    else
        return( ch - ('A' - 0x0a) );
}

/****************************************************************************/
int CCCModule::odd(register int i)				/* Is numeric argument odd? */
/****************************************************************************/
{
    return(i & 1);
}
