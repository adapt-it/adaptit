/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			_AIandGitVersionNumbers.h
/// \author			Bill Martin
/// \date_created	15 September 2021
/// \rcs_id $Id$
/// \copyright		2021 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file that defines the AI and Git version numbers 
/// and date numbers.
/////////////////////////////////////////////////////////////////////////////

#ifndef _AIandGitVersionNumbers_h
#define _AIandGitVersionNumbers_h

// This is the _AIandGitVersionNumbers.h file which defines the AI and Git version numbers. 
// This file is included in the adaptit repo, and also in our Inno Setup scripts 
// for creating Windows installers for Adapt It and our custom Git Downloader.
// The Adapt It source code files where this header file is included are: Adapt_it.h
// and Adapt_It.rc.
// The Inno Setup scripts where this header file is included are:
//    Adapt It Unicode Git.iss
//    Adapt It Unicode.iss
//    Adapt It Unicode - No Html Help.iss
//    Adapt It Unicode - Minimal.iss
//    Adapt It Unicode - Localizations Only.iss
//    Adapt It Unicode - Documentation Only.iss
//    
// When changing Adapt It's version number, the version numbers must be changed in the
// defines below (both the string defines and the integer defines). This header file is
// the main location in our source code files where the version numbers are defined.
// 
// When changing our Git Downloader's version number, the version numbers must be 
// changed in the defines below. The version numbers for our Git Downloader should be 
// identical to version numbers of the current Git installer that is available 
// at: https://git-scm.com/downloads.

// The following defines are the main lines that need updating within Adapt It 
// sources when releasing a new version of Adapt It:
// **** NOTE: Version numbers and dates are defined in both string and integer forms below.  ****
// **** Be sure to keep them in sync when changing version numbers/dates for a new release!! ****

// When changing these string values make sure to also change the corresponding int values below
#define AI_VERSION_MAJOR_STR "6"
#define AI_VERSION_MINOR_STR "11"
#define AI_VERSION_BUILD_PART_STR "6"
#define AI_VERSION_MAJOR 6
#define AI_VERSION_MINOR 11
#define AI_VERSION_BUILD_PART 6

// when changing these string values make sure to also change the corresponding int values below
#define VERSION_DATE_DAY_STR "7"
#define VERSION_DATE_MONTH_STR "3"
#define VERSION_DATE_YEAR_STR "2025"
#define VERSION_DATE_DAY 7
#define VERSION_DATE_MONTH 3
#define VERSION_DATE_YEAR 2025

// set PRE_RELEASE define below to 0 (zero) for normal releases; 1 to indicate "Pre-Release" in About Dialog
#define PRE_RELEASE 0

#define AI_VERSION_DOT "."
#define AI_VERSION_COMMA ","
#define AI_VERSION_SP " "

// whm 13Oct2021 Note: The Inno Setup compiler doesn't know how to concatenate strings in a #define statement
// as is done for the AI_VERSION_STR and RC_FILEVER_STR statements below, therefore we use the ISPP_INVOKED
// to exclude the following block of code from compilation by the Inno Setup compiler.
#ifndef ISPP_INVOKED

#define AI_VERSION_STR AI_VERSION_MAJOR_STR AI_VERSION_DOT AI_VERSION_MINOR_STR AI_VERSION_DOT AI_VERSION_BUILD_PART_STR
#define AI_VERSION_REVISION_PART ${svnversion}
const wxString appVerStr(AI_VERSION_STR);
const wxString svnVerStr(_T("$LastChangedRevision$"));

// Below is for the Adapt_It.rc file (where this header is also #include'd)
#define RC_FILEVER_STR AI_VERSION_MAJOR_STR AI_VERSION_COMMA AI_VERSION_SP AI_VERSION_MINOR_STR AI_VERSION_COMMA AI_VERSION_SP AI_VERSION_BUILD_PART_STR AI_VERSION_COMMA AI_VERSION_SP // "6, 10, 6, "
#define RC_VERSION_MAJOR AI_VERSION_MAJOR
#define RC_VERSION_MINOR AI_VERSION_MINOR
#define RC_VERSION_BUILD_PART AI_VERSION_BUILD_PART

#endif

// The following three defines are the only three lines that need updating when releasing 
// a new version of git to be downloaded by the Adapt It installers.
// The 'Adapt It Unicode Git.iss' Inno Setup script uses these version numbers which are
// used primarily in the Adapt It source code file InstallGitOptionsDlg.cpp.
#define GIT_VERSION_MAJOR 2
#define GIT_VERSION_MINOR 32
#define GIT_REVISION 0

#endif
