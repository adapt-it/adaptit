/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			Adapt_It.h
/// \author			Bill Martin
/// \date_created	05 January 2004
/// \rcs_id $Id$
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public
///                 License (see license directory)
/// \description This is the header file for the CAdapt_ItApp class, the AIModalDialog
/// class and the AutoCorrectTextCtrl class.
/// The CAdapt_ItApp class initializes Adapt It's application and gets it running. Most of
/// Adapt It's global enums, structs and variables are declared either as members of the
/// CAdapt_ItApp class or in this source file's global space. The AIModalDialog class
/// provides Adapt It with a modal dialog base class which turns off Idle and UIUpdate
/// processing while the dialog is being shown. The AutoCorrectTextCtrl class defines
/// a wxTextCtrl class that implements the AutoCorrect functions.
/// \derivation	The CAdapt_ItApp class is derived from wxApp, and inherits its support
/// for the document/view framework. The AIModalDialog class is derived from 
/// wxScrollingDialog when built with wxWidgets prior to version 2.9.x, but derived 
/// from wxDialog for version 2.9.x and later. The AutoCorrectTextCtrl class derives 
/// from wxTextCtrl.
/////////////////////////////////////////////////////////////////////////////
#ifndef Adapt_It_h
#define Adapt_It_h

// Comment out USE_LEGACY_PARSER to cause the simpler and better refactored (Nov-Dec,2016)
// ParseWord2() to be used, and the m_bBoundary setting code done in ParseWord2() rather
// than in the legacy place, after the propagation code in TokenizeText()
//
// **** NOTE**** If commenting out, be sure to do the same to the same #define in
// line 43 of AdaptItConstants.h
#define USE_LEGACY_PARSER
#include <wx/string.h>

// comment out to turn off the frequent logging of value of boolean: m_bTypedNewAdaptationInChooseTranslation
//#define TRACK_PHRBOX_CHOOSETRANS_BOOL

// Use the following #define, in _DEBUG mode, to turn on the use of a logging function called
// LogDropdownState() which uses a set of wxLogDebug() calls internally to track the values of
// parameters like m_TargetBox::m_bAbandonable flag, the box's contents, and app's 
// string contents for the member m_targetPhrase, pActiveSrcPhrase's m_key and m_adaption and
// m_nSequNumber; and from the KB, the pTU's inventory of CRefString instances'm_translation 
// string values and the value of each's m_bDeleted  boolflag. And to print the callers name and
// line number for where LogDropdownState() is being called. Callable only in _DEBUG builds.
//#define _ABANDONABLE

//#define AUTHENTICATE_AS_BRUCE

// BEW 16Feb22 it's time to suppress a number of wxLogDebug calls used when fixing the GUI layout for version 6.10.5
//#define GUIFIX

// whm added 5Jun12 for debugging purposes. The FORCE_BIBLEDIT_IS_INSTALLED_FLAG
// is set at the beginning of Adapt_It.h. When set it does the following:
// 1. Forces the this BibleditIsInstalled() function to return TRUE
// 2. Forces the App's m_bBibleditIsInstalled member to be TRUE
// 3. Forces the GetListOfBEProjects() function to return an array of three
//    dummy Bibledit project names: Nyindrou, Tok Pisin, and Free Trans. See the
//    GetListOfBEProjects() function.
// 4. Forces the CollabProjectIsEditable() function (in CollabUtilities.cpp) to
//    return TRUE.
// 5. Forces the CollabProjectHasAtLeastOneBook() function (in CollabUtilities.cpp)
//    to return TRUE.
// WARNING: This define should be set only for debugging the Setup Collaboration
// dialog. It will not enable any fetching or saving of documents, so it should
// not be defined when the GetSourceTextFromEditor dialog is called!!
#if defined(_DEBUG)
//#define FORCE_BIBLEDIT_IS_INSTALLED_FLAG
#endif
//#define _NEWDRAW
//#define _EXPAND
//#define _OVERLAP

// support for incremental building of KB Server client code !! BEW 3Oct12, Moved to be a
// preprocessor symbol in the Debug build!!
//#if defined(_DEBUG)
//#define _KBSERVER
//#endif
#define FIRST_TRY

// When src word has no puncts, and user adds puncts to tgt (copied) word, because its an
// unknown meaning, different processing paths are required to prevent unwanted loss of
// the temporary puncts by the phrasebox landing at that place and then going elsewhere.
// Define a wxLogDebug() that tracks src word with it's puncts, tgt word (with its puncts),
// to get a handle on places where the tgt word's temporary puncts get lost unwittingly
//#define TEMP_PUNCTS -- I didn't take this line of refactoring further - too much hacking for too little gain

// BEW changed to force m_bAbandonable to forever by FALSE everywhere - lesser of two
// evils, because of users forgetting to click in box to make an abandonable adaptation
// (typically a source copy) "stick" - leading to holes unexpected back in the translation
// and unnoticed
#define ABANDON_NOT


// BEW added 10Dec12, a #define for the workaround for scrollPos bug in GTK build;
// the added code needs to be present in the app permanently because the problem comes
// from an structural defect in wxWidgets which won't change soon if ever; but I'll
// leave SCROLLPOS defined as it's a handy way to hunt for all the bits and pieces of
// the workaround
#if defined(__WXGTK__)
#define SCROLLPOS
#endif

// The following define makes the Interlinear RTF export routine use the older
// MS Word compatibility structure for RTF Tables (which AI used before 22Jul11,
// i.e., before svn r. 1633).
//#define USE_OLD_WORD_RTF_TABLE_SPECS


// BEW note 14Jan13, for the KB sharing menu item in Advanced menu will use value 980 for
// the present and add the menu item and preceding separator only in the _DEBUG build while
// developing the KB Sharing functionality (see further below), and wrapped by conditional
// define using _KBSERVER symbol (the latter is #defined only in the Debug build in the
// C/C++ config properties, the preprocessor section)

// Action codes for calling the DVCS:
enum{	DVCS_CHECK, DVCS_COMMIT_FILE,
        DVCS_SETUP_VERSIONS, DVCS_GET_VERSION, DVCS_ANY_CHANGES };
				// More to be added if they come up, though actually I seem to be removing them!

class DVCS;         // class of the object giving access to the DVCS operations
class DVCSNavDlg;   // dialog for navigating through previous versions
class TranslationsList; // the CTargetUnit's list of CRefString instances

//#if defined(_KBSERVER)

class CServDisc_KBserversDlg; // BEW 12Jan16
class CWaitDlg; // BEW 8Feb16

class test_system_call;

#if wxVERSION_NUMBER < 2900
//DECLARE_EVENT_TYPE(wxServDiscHALTING, -1);
#else
//wxDECLARE_EVENT(wxServDiscHALTING, wxCommandEvent);
#endif

//#endif // _KBSERVER

// while Graeme and Bruce work on the codefix refactoring, Graeme needs to test his
// boolean removal efforts with existing xml adaptation documents, and Bruce needs to test
// his version 5 parsing of xml documents - so Bruce will wrap his code changes in a
// conditional #define using the following symbol
//#define _DOCVER5  I moved this to a preprocessor #define
//
// Likewise, so that Bruce's testing could go ahead before the CFreeTrans class is completed
// but Graeme wanted to prepare the hooks in FreeTrans.h/.cpp while work on FreeTrans.h/.cpp
// proceeded, Graeme compiled his incomplete changes with
//#define _FREETR  This was moved this to a preprocessor #define
// Once CFreeTrans is completed, a search and destroy operation was carried out to remove
// the old code wherever _FREETR was found

class wxDocManager;
class wxPageSetupDialogData;
class wxFontData;
class AIPrintout;
// for debugging m_bNoAutoSave not getting preserved across app closure and relaunch...
// comment out when the wxLogDebug() calls are no longer needed
//#define Test_m_bNoAutoSave

class NavProtectNewDoc; // for user navigation protection feature
// for support of the  m_pKbServer public member (pointer to the current instance of
// KbServer class - which is non_NULL only when KBserver support is instantiated for an
// adaptation project designated as one which is to support KB sharing

// _KBSERVER has been moved to be a precompilation define (both debug and release builds)
//#if defined(_KBSERVER)

// forward declaration
class KbServer;
class KBSharingMgrTabbedDlg;
class test_system_call;

// for a temporary ID for the "Controls For Knowledge Base Sharing" menu item on Advanced
// menu; the menu item and a preceding separator are setup (for the _DEBUG build only,
// while developing KB sharing functionality, near the end of the app function OnInit()) -
// at approx lines 21,454-471
//const int ID_MENU_SHOW_KBSERVER_DLG	= 9999; // was 980, then was wxNewId(), now keep less than 10000
// for a temporary ID for the "Setup Knowledgebase Base Sharing" menu item on Advanced
// menu; the menu item is setup (for the _DEBUG build only, while developing KB sharing
// functionality, in the app function OnInit())
//const int ID_MENU_SHOW_KBSERVER_SETUP_DLG	= 9998; // was 979, then was wxNewId(), now keep less than 10000

//#endif

// This define is to support Dennis Walters request for treating / as a whitespace wordbreak char
// Code wrapped with this conditional compile directive is to be a user-choosable permanent feature
// if this experimental support works well (BEW 23Apr15) - it does, but keep this #define because
// it locates all code throughout the app which implements the support for this feature
//#define FWD_SLASH_DELIM


/////////////////// MFC to wxWidgets Type Conversions //////////////////////////////////////
// MFC type:					wxWidgets Equivalent:
//	DWORD (unsigned long)			wxUint32
//	COLORREF (unsigned long)		WXCOLORREF (unsigned long)
//	UINT (unsigned int)				wxUint32
//	BYTE (unsigned char)			wxUint8
//	WORD							wxUint16 (or wxInt16 ???)
//	TCHAR							wxChar (Note: wxWidgets pre-defines TCHAR as wchar_t
//											in Unicode builds, char in ANSI builds)
//	LPCTSTR							const wxChar*
//	LPTSTR							wxChar*
//	HBRUSH							wxUint32
/////////////////// MFC to wxWidgets Type Conversions //////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "Adapt_It.h"
#endif

// ******************************* my #defines *********************************************

// whm 16Sep2021 modifications of application version numbers and dates for consolidation
// and simplification.
// The defines for version number are now consolidated in the _AIandGitVersionNumbers.h 
// header file which is #include'd below.
// 
// Note: the FileVersion and ProductVersion strings in the Adapt_It.rc file in bin/win32 
// now are updated automatically from the defines in the _AIandGitVersionNumbers.h header 
// file. 
// Warning: Do NOT edit the Adapt_It.rc file using the Visual Studio IDE's Resource View
// editor directly - doing so will recreate the Adatp_It.rc file adding Windows stuff we
// don't want in it and obliterating the wx stuff we do want in it.
// Instead, CLOSE Visual Studio 2008 and edit the Adapt_It.rc file in a plain
// text editor such as Notepad. If Visual Studio is open during the editing of
// Adapt_It.rc in an external editor, the IDE will crash when it tries to reload the
// Adapt_It.rc file after sensing that it was changed by the external program.
//
// The application version numbers and dates are now mainly set in the
// _AIandGitVersionNumbers.h header file (included below). However, the version numbers 
// and/or date numbers for the following still need to be set in these locations:
// 1. The applicationCompatibility attribute in the AI_UserProfiles.xml file in the 
//    adaptit/xml folder.
// 2. The Visual Studio 2019 Adapt_It > Properties > Linker > General > Version (do for 
//    both the Unicode Debug and Unicode Release Configurations).
//    The Version in the Linker settings, just uses the first two version digits - the
//    MAJOR and MINOR numbers, i.e., 6.10 which keeps things compatible with newer 
//    versions of Visual Studio.
// 3. The Mac's Info.plist file in adaptit/bin/mac/ (Erik B. does Mac builds for the 
//    Adapt_It-x.x.x.dmg distribution).
// 4. The changelog file at: adaptit/debian/changelog
// 5. Various documentation files in the adaptit/docs folder files including: 
//    [ ] Adapt It changes.txt
//    [ ] Readme_Unicode_Version.txt
//    [ ] Known Issues and Limitations.txt
//    [ ] Adapt It Reference.odt (also Save As Adapt It Reference.doc)
// 6. Within the AboutDlgFunc() in wxDesigner change the version number for the
//    ID_ABOUT_VERSION_NUM wxStaticText to the current version number. Also
//    update the release date there.
//    TODO:
//    Find out why for the Mac build this wxStaticText value is not getting
//    updated from the code in MainFrm.cpp to use the current version number
//    which includes the build number.

// The header file included below defines the application's version and date numbers.
// It also includes the Git version numbers for our Git downloads.
#include "_AIandGitVersionNumbers.h"

// whm 29Jun2021 added the following defines for the current version of Git
// The following three defines are the only three lines that need updating within Adapt It's
// source files. The App's GetVersionNumberAsString() function converts the individual version
// number integers into a formatted string, for example 2.32.0 and the source file named
// InstallGitOptionsDlg.cpp builds the git installer and our git downloader file names from
// these defines. The current git installer from github is:
//   Git-2.32.0-32-bit.exe - which is downloadable from:
//   https://github.com/git-for-windows/git/releases/download/v2.32.0.windows.1/Git-2.32.0-32-bit.exe
// Our git downloader that is created by Inno Setup is named:
// Git_Downloader_2_32_0_4AI.exe - generated by our Adapt It Unicode Git.iss Inno Setup script.
// When revising code to download a newer version of the git installer from github, and
// generate the corresponding newer version of our git downloader YOU MUST ALSO modify the git 
// version #defines near the beginning of the Inno Setup scripts to match these defined here:
//#define GIT_VERSION_MAJOR 2
//#define GIT_VERSION_MINOR 32
//#define GIT_REVISION 0

inline int GetAISvnVersion()
{
	// return the integer found at the end of the svnVerStr string
	return atoi(svnVerStr.Mid(svnVerStr.Find(_T(" ")) + 1, (svnVerStr.Length() - svnVerStr.Find(_T(" ")) - 1)).ToAscii());
}

//#define Print_failure
//#define _Trace_FilterMarkers

// BEW 27Mar20 added the following #define to enable me to progressively refactor our PlaceHolder code as per Bill's suggestion
#define _PHRefactor

// whm added 30Jan12 to force all platforms to use TCP based IPC - even on the Windows
// platform rather that its usual DDE.
// BEW 21Nov18 Bill commented it out, which means that DDE (which is windows only for
// inter process communications) got used. TCP/IP is more cross-platform friendly, and
// probably should be used because emails from Joshua Hahn in Nov 20 and thereabouts
// indicated that his Mac OSX machine running Windows in Parallels fails in the OnInit()
// so I will reinstate it and try get a clean compile. Got 23 errors. Commenting out again for the present.
//#define useTCPbasedIPC

// whm added 20Oct10 for user profiles support
#define PROFILE_VERSION_MAJOR_PART 1
#define PROFILE_VERSION_MINOR_PART 0

#define _NEW_LAYOUT // BEW May09, if not #defined, strips are only destroyed & rebuilt,
                      // never kept & tweaked; if #defined, piles & strips are retained &
                      // tweaked where necessary to update after user editing -- our final
                      // design requires this #define be set

// Use 0 to use Bibledit's command-line interface to fetch text and write text from/to its project data files.
// Use 1 to fetch text and write text directly from/to Bibledit's project data files (not using command-line
// interface).
#define _EXCHANGE_DATA_DIRECTLY_WITH_BIBLEDIT 0

// Use 1 to force the use of the CHtmlFileViewer for testing purposes instead of viewing the
// Help for Administrators.htm file within the default browser. Set to 0 for normal execution
// and distribution of the Application.
//#define _USE_HTML_FILE_VIEWER

// uncomment to turn on wxLogDebug tracking of gnBeginInsertionsSequNum & gnEndInsertionsSequNum
//#define Highlighting_Bug

// Note: The _DEBUG symbol is the only symbol defined that indicates a debug
// build in the application. It should be used to define code blocks
// that execute only in Debug or Unicode Debug configurations within all
// IDEs that are used to build Adapt It. Do not use __WXDEBUG__ for
// that purpose.

// The following define is for testing the wxSplitterWindow on the wx version.
// To add the splitter window uncomment the line below and rebuild. (It may be buggy.
// Leave it, nobody seems to use it.)
//#define _USE_SPLITTER_WINDOW

// The following define causes the SIL Converters specific code to be included in the build. Initially
// this may only work for the Windows port, so I've also conditionally defined the USE_SIL_CONVERTERS
// define to be available only for __WXMSW__
#ifdef __WXMSW__
// Comment out the line below to test the removal of SIL Converters code on Windows builds
#define USE_SIL_CONVERTERS
#endif

// uncomment the define below to output KB I/O benchmarks (in debug mode only)
//#define SHOW_KB_I_O_BENCHMARKS

// whm 12May2020 added defines below to output benchmarks for the OnePass() and LookAhead() functions (in debug mode only)
#define SHOW_LOOK_AHEAD_BENCHMARKS
#define SHOW_ONEPASS_BENCHMARKS

#define SORTKB 1 // change to 0 for output of legacy unsorted KB map entries

// uncomment the define below to output I/O benchmarks for OnNewDocument() and/or OnOpenDocument
// as wxLogDebug output (in debug mode only)
#define SHOW_DOC_I_O_BENCHMARKS

#ifdef _UNICODE
#define _RTL_FLAGS  // for the m_bSrcRTL etc flags and supporting code, the Layout menu, etc
#endif

// *************************** end my #defines *********************************************

// The MFC version uses resource.h to store resource symbols, however we are using
// Adapt_It_Resources.h which simply includes the wxDesigner produced C++ resources by
// including "Adapt_It_wdr.h"
#include "Adapt_It_Resources.h"
#include "AdaptitConstants.h"
#include <wx/datetime.h>
#include <wx/file.h>
#include <wx/ffile.h>
#include <wx/mstream.h> // edb 08June2012 - add for embedded .png support
#include "GuesserAffix.h"

// Does wxWidgets recognize/utilize these clipboard defines???
#ifdef _UNICODE
#define CF_CLIPBOARDFORMAT CF_UNICODETEXT
#else
#define CF_CLIPBOARDFORMAT CF_TEXT
#endif

#if wxCHECK_VISUALC_VERSION(14)
#if wxCHECK_VERSION(2,9,0)
// whm 13Jan2017 added
// The wx2.9 and wx3.x libraries enable asserts even in release builds. Use the following
// macro to disable asserts in release builds.
// 
// whm 24Feb2022 updated. The following #include and callling the wxDISABLE_ASSERTS_IN_RELEASE_BUILD
// macro never worked to disable/prevent asserts in release builds.
// According to this post on the wxWidgets Discussion Forum at: https://forums.wxwidgets.org/viewtopic.php?t=34137
// the proper way to disable/prevent asserts in release builds is to do the following:
// 1. Tweak the wxWidgets-3.1.5\include\wx\setup.h file's lines starting about line 90 to 
// check if NDEBUG is defined, and if so to #define wxDEBUG_LEVEL 0.
// that is change the lines in setup.h to the following:
/*
#ifdef NDEBUG
#define wxDEBUG_LEVEL 0
// #else
//  #define wxDEBUG_LEVEL 2
#endif
*/
// 2. After tweaking setup.h you must rebuild the wxWidgets library that is being used.
// 3. In Visual Studio's Adapt_It project properties for Unicode Release > C/C++ > Preprocessor flags add NDEBUG to the list.
//    Only add NDEBUG to the Unicode Release's preprocessor flags, don't add it to the Unicode Debug flags!
// With the above settings, Adapt It's release version should no longer produce asserts in Unicode Release builds.

// whm 24Feb2022 modified - The following 2 lines are now commented out
//#include <wx/debug.h>
//wxDISABLE_ASSERTS_IN_RELEASE_BUILD();
#endif
#endif

#if wxCHECK_VERSION(2,9,0)
	// Use the built-in scrolling dialog features available in wxWidgets  2.9.x
#else
	// The wxWidgets library being used is pre-2.9.x, so use our own modified
	// version named wxScrollingDialog located in scrollingdialog.h
#include "scrollingdialog.h"
#endif

//#if defined(_KBSERVER)
// for wxJson support
#include "json_defs.h"
//#endif

#include "PhraseBox.h"
#include "FindReplace.h"
#include "Retranslation.h"
#include "CorGuess.h"

// forward references (added to wxWidgets version):
class wxSingleInstanceChecker;
//class wxConfig;
class wxFileConfig;
class wxCmdLineParser;
class wxTextFile;
class CMainFrame; // added for GetMainFrame()
// forward references (from MFC version) below:
class CKB;
class CAdapt_ItView;
class CPhraseBox;
class CAdapt_ItDoc;
class CPunctCorrespPageCommon;
class CUsfmFilterPageCommon;
class CStdioFile;
class CPrintingDlg;
class CStdioFileEx;

// forward references added when moved data structures from the View to the App
class CSourceBundle;
class CPile;
class CCell;
class CEarlierTranslationDlg;
class CFindReplace;
class CNoteDlg;
class CViewFilteredMaterialDlg;
class wxHelpControllerBase;
class wxHtmlHelpController;
class CConsistentChanger;
class wxPropertySheetDialog;

// forward references for refactored view layout support
class CLayout;
class CSourcePhrase;
class SPList;

// forward references for CFreeTrans support
class CFreeTrans;

// forward references for CNotes support
class CNotes;

// forward references for CRetranslation support
class CRetranslation;

// forward references for CPlaceholder support
class CPlaceholder;

// forward reference for read-only support
class ReadOnlyProtection;

// forward reference for Oxes export support
// BEW removed 15Jun11 until we provide OXES support
// BEW reinstated 19May12, for OXES v1 support
//class Oxes;
class Xhtml;

// forward for Admin Help
class CHtmlFileViewer;

//#include "BString.h"

#if defined(_DEBUG) && defined(__WXGTK__)
// forward reference that ties a Log Debug (normal) window, always on top, to the wx logging output
// because CodeBlocks lacks any way of gathering and displaying wxLogDebug() output, so this does it
// -- see the end of the OnInit() function for the code which creates the window and ties it to the
// logging mechanism
// whm 25Nov12 Removed the following
//class wxLogDebug;
#endif

// The following constants were originally declared in the global space of XML.h. G++ 3.x
// could find them but the g++ 4.x linker can't find them even though XML.h is included
// above, so I've moved them here to the App's header.
// whm CAUTION: Some of the original identifier names/symbols used in the MFC version are
// very short containing only one or two letters, which makes it more likely that they
// might conflict with other defined symbols in the code. Therefore I've prefixed the
// symbols with "xml_" in the wx version.

/// the standard entities
const char xml_amp[] = "&amp;";
/// the standard entities
const char xml_quote[] = "&quot;";
/// the standard entities
const char xml_apos[] = "&apos;";
/// the standard entities
const char xml_lt[] = "&lt;";
/// the standard entities
const char xml_gt[] = "&gt;";
/// the standard entities
const char xml_tab[] = "&#9;"; // whm added 24May11


/// For Adapt It document output as XML, and parsing of XML elements.
const char xml_adaptitdoc[] = "AdaptItDoc";
/// For Adapt It document output as XML, and parsing of XML elements.
const char xml_settings[] = "Settings";
/// For Adapt It document output as XML, and parsing of XML elements.
const char xml_scap[] = "S";
/// For Adapt It document output as XML, and parsing of XML elements.
const char xml_mpcap[] = "MP";
/// For Adapt It document output as XML, and parsing of XML elements.
const char xml_mmcap[] = "MM";

// Attribute names for Adapt It XML documents

/// Attribute name used in Adapt It XML documents
const char xml_docversion[] = "docVersion";
/// Attribute name used in Adapt It XML documents
const char xml_bookName[] = "bookName";
/// Attribute name used in Adapt It XML documents
const char xml_activeSequNum[] = "actseqnum";
/// Attribute name used in Adapt It XML documents
const char xml_owner[] = "owner";
/// Attribute name used in Adapt It XML documents
const char xml_commitcnt[] = "commitcnt";
/// Attribute name used in Adapt It XML documents
const char xml_revdate[] = "revdate";
/// Attribute name used in Adapt It XML documents
const char xml_sizex[] = "sizex";
/// Attribute name used in Adapt It XML documents
const char xml_sizey[] = "sizey";
/// Attribute name used in Adapt It XML documents
const char xml_ftsbp[] = "ftsbp"; // for "free translation section by punctuation" flag value
/// Attribute name used in Adapt It XML documents
const char xml_specialcolor[] = "specialcolor";
/// Attribute name used in Adapt It XML documents
const char xml_retranscolor[] = "retranscolor";
/// Attribute name used in Adapt It XML documents
const char xml_navcolor[] = "navcolor";
/// Attribute name used in Adapt It XML documents
const char xml_curchap[] = "curchap";
/// Attribute name used in Adapt It XML documents
const char xml_srcname[] = "srcname";
/// Attribute name used in Adapt It XML documents
const char xml_tgtname[] = "tgtname";
/// Attribute name used in Adapt It XML documents
const char xml_srccode[] = "srccode";
/// Attribute name used in Adapt It XML documents
const char xml_tgtcode[] = "tgtcode";
/// Attribute name used in Adapt It XML documents
const char xml_others[] = "others";

// next ones added by BEW 1Jun10 for kbv2 support
/// Attribute name used in Adapt It XML documents
const char xml_kbversion[] = "kbVersion";
/// Attribute name used in Adapt It XML documents
const char xml_glossingKB[] = "glossingKB";
/// Attribute name used in Adapt It XML documents
const char xml_creationDT[] = "cDT";
/// Attribute name used in Adapt It XML documents
const char xml_modifiedDT[] = "mDT";
/// Attribute name used in Adapt It XML documents
const char xml_deletedDT[] = "dDT";
/// Attribute name used in Adapt It XML documents
const char xml_whocreated[] = "wC";
/// Attribute name used in Adapt It XML documents
const char xml_deletedflag[] = "df";


// next ones are for the sourcephrases themselves

/// Attribute name used in Adapt It XML documents
const char xml_a[] = "a"; // m_adaption
/// Attribute name used in Adapt It XML documents
const char xml_k[] = "k"; // m_key
/// Attribute name used in Adapt It XML documents
const char xml_s[] = "s"; // m_srcPhrase
/// Attribute name used in Adapt It XML documents
const char xml_t[] = "t"; // m_targetStr
/// Attribute name used in Adapt It XML documents
const char xml_g[] = "g"; // m_gloss
/// Attribute name used in Adapt It XML documents
const char xml_f[] = "f"; // flags (32digit number, all 0 or 1)
/// Attribute name used in Adapt It XML documents
const char xml_sn[] = "sn"; // m_nSequNumber
/// Attribute name used in Adapt It XML documents
const char xml_w[] = "w"; // m_nSrcWords
/// Attribute name used in Adapt It XML documents
const char xml_ty[] = "ty"; // m_curTextType
/// Attribute name used in Adapt It XML documents
const char xml_pp[] = "pp"; // m_precPunct
/// Attribute name used in Adapt It XML documents
const char xml_fp[] = "fp"; // m_follPunct
/// Attribute name used in Adapt It XML documents
const char xml_i[] = "i"; // m_inform
/// Attribute name used in Adapt It XML documents
const char xml_c[] = "c"; // m_chapterVerse
/// Attribute name used in Adapt It XML documents
const char xml_m[] = "m"; // m_markers
/// Attribute name used in Adapt It XML documents
const char xml_mp[] = "mp"; // some medial punctuation
/// Attribute name used in Adapt It XML documents
const char xml_mm[] = "mm"; // one or more medial markers (no filtered stuff)

// new ones, Feb 2010 and 11Oct10, for doc version = 5
/// Attribute name used in Adapt It XML documents
const char xml_em[] = "em"; // m_endMarkers
/// Attribute name used in Adapt It XML documents
const char xml_ft[] = "ft"; // m_freeTrans
/// Attribute name used in Adapt It XML documents
const char xml_no[] = "no"; // m_note
/// Attribute name used in Adapt It XML documents
const char xml_bt[] = "bt"; // m_collectedBackTrans
/// Attribute name used in Adapt It XML documents
const char xml_fi[] = "fi"; // m_filteredInfo
/// Attribute name used in Adapt It XML documents
const char xml_fiA[] = "fiA"; // m_filteredInfo_After
/// Attribute name used in Adapt It XML documents
const char xml_fop[] = "fop"; // m_follOuterPunct
/// Attribute name used in Adapt It XML documents
const char xml_iBM[] = "iBM"; // m_inlineBindingMarkers
/// Attribute name used in Adapt It XML documents
const char xml_iBEM[] = "iBEM"; // m_inlineBindingEndMarkers
/// Attribute name used in Adapt It XML documents
const char xml_iNM[] = "iNM"; // m_inlineNonbindingMarkers
/// Attribute name used in Adapt It XML documents
const char xml_iNEM[] = "iNEM"; // m_inlineNonbindingEndMarkers

// new ones, Feb 13 2012, for support of doc version = 6
/// Attribute name used in Adapt It XML documents
const char xml_lapat[] = "lapat"; // m_lastAdaptionsPattern
/// Attribute name used in Adapt It XML documents
const char xml_tmpat[] = "tmpat"; // m_tgtMkrPattern
/// Attribute name used in Adapt It XML documents
const char xml_gmpat[] = "gmpat"; // m_glossMkrPattern
/// Attribute name used in Adapt It XML documents
const char xml_pupat[] = "pupat"; // m_punctsPattern

// new ones, July 9 2014, for support of doc version = 9, for SEAsian languages using ZWSP etc
/// Attribute name used in Adapt It XML documents
const char xml_srcwdbrk[] = "swbk"; // m_srcWordBreak (a wxString in CSourcePhrase)
/// Attribute name used in Adapt It XML documents
const char xml_tgtwdbrk[] = "twbk"; // for target wordbreak, in a retranslation


// entity names (utf-8) for the ZWSP etc special space word-breaking delimiters
const char xml_nbsp[] =         "&#x00A0;"; // standard Non-Breaking SPace
const char xml_enquad[] =       "&#x2000;"; // En Quad or 'en space'
const char xml_emquad[] =       "&#x2001;"; // Em Quad or 'em space'
const char xml_enspace[] =      "&#x2002;"; // En Space or 'space', approx = latin space
const char xml_emspace[] =      "&#x2003;"; // Em Space or 'em space'
const char xml_3peremspace[] =  "&#x2004;"; // slightly thinner space
const char xml_4peremspace[] =  "&#x2005;"; // mid space
const char xml_6peremspace[] =  "&#x2006;"; // thin space
const char xml_figurespace[] =  "&#x2007;"; // digit with of fixed width digits
const char xml_punctspace[] =   "&#x2008;"; // punctuation space (equals narrow punctn of font)
const char xml_thinspace[] =    "&#x2009;"; // thin space (about a fifth of an em)
const char xml_hairspace[] =    "&#x200A;"; // hair space (thinner than thin, narrowest available)
const char xml_zwsp[] =         "&#x200B;"; // ZWSP or 'zero width space'
const char xml_wdjoinr[] =      "&#x2060;"; // zero width non-breaking Word Joiner (called WJ)


// tag & attribute names for KB i/o

/// Tag name used in Adapt It XML KB i/o
const char xml_aikb[] = "AdaptItKnowledgeBase";
/// Tag name used in Adapt It XML KB i/o
const char xml_kb[] = "KB";
// Tag name used in Adapt It XML KB i/o
//const char xml_gkb[] = "GKB"; // deprecated
/// Tag name used in Adapt It XML KB i/o
const char xml_map[] = "MAP";
/// Tag name used in Adapt It XML KB i/o
const char xml_tu[] = "TU";
/// Tag name used in Adapt It XML KB i/o
const char xml_rs[] = "RS";
/// Attribute name used in Adapt It XML KB i/o
const char xml_srcnm[] = "srcName";
/// Attribute name used in Adapt It XML KB i/o
const char xml_tgtnm[] = "tgtName";
/// Attribute name used in Adapt It XML KB i/o
const char xml_srccod[] = "srcCode";
/// Attribute name used in Adapt It XML KB i/o
const char xml_tgtcod[] = "tgtCode";
/// Attribute name used in Adapt It XML KB i/o
const char xml_n[] = "n";
/// Attribute name used in Adapt It XML KB i/o
const char xml_max[] = "max";
/// Attribute name used in Adapt It XML KB i/o
const char xml_mn[] = "mn";
/// Attribute name used in Adapt It XML KB i/o
const char xml_xmlns[] = "xmlns";

// tag & attribute names for Guesser Prefix/Suffix i/o
/// Tag name used in Adapt It XML Guesser i/o
const char xml_prefix[] = "PREFIX";
const char xml_pre[] = "PRE";
const char xml_affixversion[] = "affixVersion";
const char xml_source[] = "source";
const char xml_target[] = "target";
const char xml_suffix[] = "SUFFIX";
const char xml_suf[] = "SUF";

// tag names for LIFT i/o

/// Tag name used in LIFT XML i/o
const char xml_lift[] = "lift";
/// Tag name used in LIFT XML i/o
const char xml_entry[] = "entry";
/// Tag name used in LIFT XML i/o
const char xml_lexical_unit[] = "lexical-unit";
/// Tag name used in LIFT XML i/o
const char xml_form[] = "form";
/// Tag name used in LIFT XML i/o
const char xml_text[] = "text";
/// Tag name used in LIFT XML i/o
const char xml_sense[] = "sense";
/// Tag name used in LIFT XML i/o
const char xml_definition[] = "definition";
/// Tag name used in LIFT XML i/o
const char xml_gloss[] = "gloss";
/// Tag name used in LIFT XML i/o
const char xml_lift_version[] = "version";
/// Tag name used in LIFT XML i/o
const char xml_guid[] = "guid";
/// Tag name used in LIFT XML i/o
const char xml_id[] = "id";
/// Tag name used in LIFT XML i/o
const char xml_lang[] = "lang";
/* the function MyParentsAre() makes these unneeded
const char xml_annotation[] = "annotation";
const char xml_grammatical_info[] = "grammatical-info";
const char xml_relation[] = "relation";
const char xml_note[] = "note";
const char xml_example[] = "example";
const char xml_reversal[] = "reversal";
const char xml_illustration[] = "illustration";
*/

// this group of tags are for the AI_UserProfiles.xml file
const char userprofilessupport[] = "UserProfilesSupport";
const char menu[] = "MENU";
const char profile[] = "PROFILE";
//const char menuStructure[] = "MENU_STRUCTURE";
//const char main_menu[] = "MAIN_MENU";
//const char sub_menu[] = "SUB_MENU";
const char end_userprofilessupport[] = "/UserProfilesSupport";
const char end_menu[] = "/MENU";
const char end_profile[] = "/PROFILE";
//const char end_menuStructure[] = "/MENU_STRUCTURE";
//const char end_main_menu[] = "/MAIN_MENU";
//const char end_sub_menu[] = "/SUB_MENU";

// this group are for the attribute names for AI_UserProfiles.xml
const char profileVersion[] = "profileVersion";
const char applicationCompatibility[] = "applicationCompatibility";
const char adminModified[] = "adminModified";
const char definedProfile[] = "definedProfile"; // the xml will actually have a number suffix
												// i.e., definedProfile1, definedProfile2, etc.
const char descriptionProfile[] = "descriptionProfile"; // the xml will actually have a number suffix
												// i.e., descriptionProfile1, descriptionProfile2, etc.
const char itemID[] = "itemID";
const char itemType[] = "itemType";
const char itemText[] = "itemText";
const char itemDescr[] = "itemDescr";
const char itemAdminCanChange[] = "adminCanChange";
const char itemUserProfile[] = "userProfile";
const char itemVisibility[] = "itemVisibility";
const char factory[] = "factory";

const char mainMenuID[] = "mainMenuID";
const char mainMenuLabel[] = "mainMenuLabel";
const char subMenuID[] = "subMenuID";
const char subMenuLabel[] = "subMenuLabel";
const char subMenuHelp[] = "subMenuHelp";
const char subMenuKind[] = "subMenuKind";

// this group are for the email problem and email feedback reports
const char adaptitproblemreport[] = "AdaptItProblemReport";
const char adaptitfeedbackreport[] = "AdaptItFeedbackReport";
const char reportemailheader[] = "ReportEmailHeader";
const char reportemailbody[] = "ReportEmailBody";
const char reportattachmentusagelog[] = "ReportAttachmentUsageLog";
const char reportattachmentpackeddocument[] = "ReportAttachmentPackedDocument";
const char emailfrom[] = "emailFrom";
const char emailto[] = "emailTo";
const char emailsubject[] = "emailSubject";
const char emailsendersname[] = "emailsendersname";
const char usagelogfilepathname[] = "usageLogFilePathName";
const char packeddocumentfilepathname[] = "packedDocumentFilePathName";


/// struct for saving top and bottom logical coord offsets for printing pages, stored in
/// m_pagesList Instances of PageOffsets are populated in the PaginateDoc() function in the
/// View.
struct PageOffsets
{
	int nTop; // logical coord offset of first strip on page (from start of the
			  // logical/virtual document)
	int nBottom; // logical coord offset of last strip on page (from start of the
				 // logical/virtual document)
	int nFirstStrip; // 0-based index of the first strip to appear on the current page
	int nLastStrip; // 0-based index of the last strip to appear on the current page
};

// BEW 27Jul21 added this struct to support refactored layout now that dropdown
// list has to be factored into the overall design. See detailed comments preceding
// the ResizeBox() function, in Adapt_ItView.cpp
// A permanent public instance of this struct will be added to the CLayout instance, and
// writing values to it etc will happen, (after clearing,) dynamically after the layout
// is recalculated and before that is followed up by being drawn.
/* BEW 17Aug21 unused now, remove it 
struct LayoutCache
{
	int	nActiveSequNum;
	wxString strActiveAdaption;
	int nDropdownWidth;
	int nDropdownHeight;
	int nTextBoxWidth;
};
*/

/// wxList declaration and partial implementation of the POList class being
/// a list of pointers to PageOffsets objects
WX_DECLARE_LIST(PageOffsets, POList); // see list definition macro in .cpp file

/// wxList declaration and partial implementation of the CCellList class being
/// a list of pointers to CCell objects
WX_DECLARE_LIST(CCell, CCellList); // see list definition macro in .cpp file

// globals

//GDLC 2010-02-12 Definition of FreeTrElement moved to FreeTrans.h

/// An enum for selecting which code block to use within the FixBasicConfigPaths()
/// function, called from MakeForeignBasicConfigFilesSafe()
enum ConfigFixType
{
    defaultPathsFix,
    customPathsFix
};

enum DelayedFreeTransOperations
{
	no_op,
	join_with_next,
	join_with_previous,
	split_it
};



/// An enum for selecting which configuration file type in GetConfigurationFile()
/// whether basic configuration file, or project configuration file.
enum ConfigFileType
{
    basicConfigFile = 1,
    projectConfigFile
};

/// An enum for selecting which kind of text to generate in order to send it back to the external
/// editor (PT or BE), whether target text, or a free translation. Used in ExportFunctions.cpp
/// and in CollabUtilities.cpp
enum SendBackTextType
{
	makeTargetText = 1,
	makeFreeTransText
};

//#if defined(_KBSERVER)

/// Initial possible state after basic config file has first been read in a new session
enum ServDiscInitialDetail
{
	SDInit_NoStoredIpAddr,
	SDInit_StoredIpAddr
};

/// states, various user action possibilities
/// An enum for reporting errors, or user choice, or other service discovery details
/// pertinent to subsequent processing after a service discovery attempt - such as
/// whether one or more than one KBserver was discovered on the LAN, various error
enum ServDiscDetail
{
	SD_NoResultsYet,
	SD_NoKBserverFound,
	SD_ServiceDiscoveryError,
	SD_NoResultsAndUserCancelled,
	SD_SameIpAddrAsInConfigFile,
	SD_IpAddrDiffers_UserAcceptedIt,
	SD_SingleIpAddr_UserAcceptedIt,
	SD_SingleIpAddr_UserCancelled,
	SD_SingleIpAddr_ButNotChosen,
	SD_MultipleIpAddr_UserCancelled,
	SD_MultipleIpAddr_UserChoseOne,
	SD_MultipleIpAddr_UserChoseNone,
	SD_ValueIsIrrelevant
};

// Don't use an enum, int values are simpler
const int noDatFile = 0;
const int credentials_for_user = 1;
const int lookup_user = 2;
const int list_users = 3;
const int create_entry = 4;
const int pseudo_delete = 5;
const int pseudo_undelete = 6;
const int lookup_entry = 7;
const int changed_since_timed = 8;
const int upload_local_kb = 9;
const int change_permission = 10;
const int change_fullname = 11;
const int change_password = 12;
// add more here, as our solution matures
const int blanksEnd = 13; // this one changes as we add more above

//#endif


// whm NOTE 21Sep10: Moved this TopLevelMenu enum to become a private member of the
// App because its enumerations should not be accessed directly, but only through
// the GetTopLevelMenuName(TopLevelMenu topLevelMenu) function. We may decide at
// some point to dynamically change the ordering of top level menus. We already
// change the number of top level menus, i.e., the "Layout" menu is
// removed for ANSI builds, and the "Administrator" menu is removed except when made
// visible by an administrator.
//
// An enum for the indices for the top level menus
// Note: item0 is the menubar itself, but that does not mean that the File menu is
// at index 1; the menus in a menu bar are indexed from the first (File menu) starting
// with index == 0
/*
enum TopLevelMenu
{
    fileMenu,
    editMenu,
    viewMenu,
    toolsMenu,
	exportImportMenu,
    advancedMenu,
    layoutMenu,
    helpMenu,
	administratorMenu
};
*/

enum ExportType
{
	sourceTextExport,
	targetTextExport,
	glossesTextExport,
	freeTransTextExport
};

enum KBExportSaveAsType
{
	// whm 31Mar11 changed default to be KBExportSaveAsSFM, i.e., enum 0
	KBExportSaveAsSFM_TXT,
	KBExportSaveAsLIFT_XML,
	KBExportAllFiles
};

enum KBImportFileOfType
{
	// whm 31Mar11 changed default to be KBImportFileOfSFM_TXT, i.e., enum 0
	KBImportFileOfSFM_TXT,
	KBImportFileOfLIFT_XML,
	KBImportAllFiles
};

/// An enum for specifying the selection extension direction,
/// either right or left.
enum extendSelDir
{
	toright, // BEW 2Oct13, changed from right to toright because of ambiguity with std::ios_base
	toleft   // BEW 3Oct13, changed from left to toleft because of simlar ambiguity in std::ios_base
};

/// An enum for specifying how the ChooseInterfaceLanguage() function
/// is called. Struct members include: userInitiated and firstRun.
enum SetInterfaceLanguage
{
	userInitiated,
	firstRun
};

/// An enum for specifying the search case type, either caseSensitive
/// or caseInsensitive.
enum SearchCaseType
{
	caseSensitive,
	caseInsensitive
};

/// An enum for specifying the search string length type, either
/// subString or exactString.
enum SearchStrLengthType
{
	subString,
	exactString
};

enum StartFromType
{
	fromFirstListPos,
	fromCurrentSelPosToListEnd,
	fromCurrentSelPosCyclingBack
};

/// An enum for specifying the affix type, either prefix
/// or suffix. Used by DoGuesserAffixWriteXML(). -KLB Sept 2013
enum GuesserAffixType
{
	// whm 31Mar11 changed default to be KBExportSaveAsSFM, i.e., enum 0
	GuesserPrefix,
	GuesserSuffix
};

enum LangCodesChoice {
	all_possibilities,
	source_and_target_only,
	source_and_glosses_only
}; // keep 'Codes' in name, LanguageCodesDlg.cpp & .h needs them, for xhtml support

/// A struct for specifying time settings. Struct members include:
/// m_tsDoc, m_tsKB, m_tLastDocSave, and m_tLastKBSave.
struct TIMESETTINGS
{
	wxTimeSpan	m_tsDoc;
	wxTimeSpan	m_tsKB;
	wxDateTime	m_tLastDocSave;
	wxDateTime	m_tLastKBSave;
};

/*
// These are the "restored default" settings, which WriteCacheDefaults()
// will set using the following struct. m_pFindDlg points at the FindReplace.h,
// class creator, see line 38
pApp->m_pFindDlg->m_srcStr.Empty();
pApp->m_pFindDlg->m_tgtStr.Empty();
pApp->m_pFindDlg->m_replaceStr.Empty();
pApp->m_pFindDlg->m_sfm.Empty();
pApp->m_pFindDlg->m_markerStr.Empty();
pApp->m_pFindDlg->m_bFindRetransln = FALSE;
pApp->m_pFindDlg->m_bFindNullSrcPhrase = FALSE;
pApp->m_pFindDlg->m_bFindSFM = FALSE;
pApp->m_pFindDlg->m_bSrcOnly = TRUE;
pApp->m_pFindDlg->m_bTgtOnly = FALSE;
pApp->m_pFindDlg->m_bSrcAndTgt = FALSE;
pApp->m_pFindDlg->m_bSpecialSearch = FALSE;
pApp->m_pFindDlg->m_bFindDlg = TRUE;
pApp->m_pFindDlg->m_bSpanSrcPhrases = FALSE;
pApp->m_pFindDlg->m_bIncludePunct = FALSE;
pApp->m_pFindDlg->m_bIgnoreCase = FALSE;
pApp->m_pFindDlg->m_nCount = 0;
// There are 3 functions which use this struct, from FindReplace.cpp's instance:
// bool WriteCacheDefaults() - which sets defaultFindConfig struct to have the above values
// bool WriteFindCache(pApp->m_pFindDlg) - which sets readwriteFindConfig struct to have the
// values as at the Find Next button press (so that config is not lost when Ctrl+F is done)
// bool ReadFindCache(pApp->m_pFindDlg) - which takes the cached values in the 
// readwriteFindConfig struct,to restore them when Ctrl+F is done to call OnFind(), 
// at its start, when doing a Find Next with unchanged configuration values. 
// Return FALSE if there was an error so that the caller can take evasive action 
// to prevent a crash 
*/
struct CacheFindReplaceConfig
{
	wxString srcStr;
	wxString tgtStr;
	wxString replaceStr;
	int marker;
	wxString markerStr;
	wxString sfm;
	bool bFindRetranslation;
	bool bFindNullSrcPhrase; // NullSrcPhrase is a Placeholder, as of years ago
	bool bFindSFM;
	bool bSrcOnly;
	bool bTgtOnly;
	bool bSrcAndTgt;
	bool bSpecialSearch;
	bool bFindDlg;
	bool bSpanSrcPhrases;
	bool bIncludePunct;
	// whm 10Mar2022 modified
	//bool bIgnoreCase = FALSE; // For VS 2008 and .msi builds initializing bIgnoreCase
	// here to a value generates "error C2864: 'CacheFindReplaceConfig::bIgnoreCase' : only 
	// static const integral data members can be initialized within a class"
	// Note: All of the above struct members are initialized in the App's WriteCacheDefaults() 
	// function so no initialization need be done here.
	bool bIgnoreCase;
	// count of how many srcPhrase instances were matched 
	// (value is not valid when there was no match)
	int nCount;
};

struct CacheReplaceConfig
{
	wxString srcStr;
	wxString tgtStr;
	wxString replaceStr;
	//int marker;
	//wxString markerStr;
	//wxString sfm;
	//bool bFindRetranslation;
	//bool bFindNullSrcPhrase; // NullSrcPhrase is a Placeholder
	//bool bFindSFM;
	bool bSrcOnly;
	bool bTgtOnly;
	//bool bSrcAndTgt;
	//bool bSpecialSearch;
	bool bFindDlg; // retain
	bool bReplaceDlg; // added
	bool bSpanSrcPhrases;
	bool bIncludePunct;
	// whm 10Mar2022 modified
	//bool bIgnoreCase = FALSE; // For VS 2008 and .msi builds initializing bIgnoreCase
	// here to a value generates "error C2864: 'CacheReplaceConfig::bIgnoreCase' : only 
	// static const integral data members can be initialized within a class"
	// Note: All of the above struct members are initialized in the App's WriteCacheDefaults() 
	// function so no initialization need be done here.
	bool bIgnoreCase;
	// count of how many srcPhrase instances were matched 
	// (value is not valid when there was no match)
	//int nCount;
};

/// A struct for specifying paired source and target punctuation characters.
/// Struct members include: charSrc and charTgt.
struct PUNCTPAIR
{
	wxChar		charSrc;
	wxChar		charTgt;
};

/// A struct for specifying paired source and target two-punctuation characters. Struct
/// members include: twocharSrc[2] and twocharTgt[2].
struct TWOPUNCTPAIR
{
	wxChar	twocharSrc[2];
	wxChar	twocharTgt[2];
};

/// A struct for use in the DoKBRestore() function, stores key string plus number of words
/// in it. Struct members include: key and count.
struct KeyPlusCount
{
	wxString key;
	int		count;
};

/// A struct for storing font information that is mainly relevant to the MFC version. In
/// the wx version it is mainly used for config file input and output of font data, so that
/// the wx version config files maintain compatibility with the MFC version config files.
/// Struct members include: fLangType, fHeight, fPointSize, fWidth, fEscapement,
/// fOrientation, fWeight, fWeightConfSave, fItalic, fStyle, fUnderline, fStrikeOut,
/// fCharset, fEncoding, fOutPrecision, fClipPrecision, fQuality, fPitchAndFamily, fFamily,
/// and fFaceName.
struct fontInfo
{
	wxString fLangType;	// = "SourceFont", "TargetFont", or "NavTextFont"
						// Note: those marked *(n) are used in wxWidgets'
						// wxFont constructor n = order of parameters
	int fHeight;		// = conversion of point size to lfHeight using
						// PointSizeToNegHeight(); saved in conf
	int fPointSize;		//(1) = conversion of lfHeight to point size using
						// NegHeightToPointSize(); used by program
	int fWidth;			// = lfWidth (written as 0 but not used in wxWidgets)
	int fEscapement;	// = lfEscapement (written as 0 but not used in wxWidgets)
	int fOrientation;	// = lfOrientation (written as 0 but not used in wxWidgets)
	int fWeight;		//(4) = lfWeight (wxWidgets <-> MFC: wxLIGHT <-> 400,
						// wxNORMAL <-> 500, and wxBOLD <-> 700)
	int fWeightConfSave;// (4) = lfWeight (raw value read from/saved to conf file)
	bool fItalic;		// = lfItalic (wxWidgets <-> MFC: wxNORMAL <-> 0,
						// wxSLANT and wxITALIC <-> 1)
	int fStyle;			//(3) = (wxNORMAL, wxSLANT, and wxITALIC in wxWidgets
						// converted to/from fItalic)
	bool fUnderline;	//(5) = lfUnderline (written as 0 for FALSE, 1 for TRUE)
	bool fStrikeOut;	// = lfStrikeOut (written as 0 for FALSE, 1 for TRUE,
						// not used in wxWidgets)
	int fCharset;		// = lfCharSet (a compatibility value to maintain
                        // transferrable config files with the MFC app. This value is
                        // written to config file in as an int value consistent with MFC's
                        // fCharset as used in its LOGFONT structure. When the config file
                        // is read from the config file, the fCharset value is mapped to
                        // the appropriate encoding enum value for the font used in the
                        // wxWidgets' context. This enum value is stored in the fEncoding
                        // struct member below, and is the actual value which is used for
                        // the font during the running of the wx app.
	wxFontEncoding fEncoding;//(7) Not part of MS LOGFONT struct, but used in the wx app
                        // as the mapped encoding value for the font. We'll use
                        // wxFONTENCODING_DEFAULT regardless of what lfCharSet is stored in
                        // config
	int fOutPrecision;	// = lfOutPrecision (written as 3 but not used in wxWidgets)
	int fClipPrecision;	// = lfClipPrecision (written as 2 but not used in wxWidgets)
	int fQuality;		// = lfQuality (written as 1 but not used in wxWidgets)
	int fPitchAndFamily;// = conversion of family and DEFAULT_PITCH to lfPitchAndFamily
						// using bit mask; saved in conf
	int fFamily;		//(2) = conversion of lfPitchAndFamily to family using bit mask
	wxString fFaceName;	//(6) = lfFaceName (not to exceed 32 characters in Windows)
};

// the following adapted from PoEdit source code Copyright (C) 2003-2006 Vaclav Slavik

/// A struct for specifying interface language information. Struct members include:
/// fullName, code and shortName.
struct LangInfo
{
    const wxChar *fullName;
    wxLanguage code;
    const wxChar *shortName;
};

struct CurrLocalizationInfo
{
	int curr_UI_Language;			// for languages known to wxWidgets this is
									// equivalent to a wxLanguage enum
	wxString curr_shortName;		// the ISO 639 name of the form xx or xx_XX
									// where xx is the 2-letter language code
	wxString curr_fullName;			// the common descriptive name for the language
	// whm 8Dec11 note: The curr_localizationPath is stored in the struct, but is
	// no longer saved in the external Adapt_It_WX.ini file
	wxString curr_localizationPath;	// the path where the above CurrLocalizationInfo
					// folder (curr_shortName) is located (which contains <appName>.mo)
};

//#if defined(__UNIX__) && !defined(__WXMAC__)
//    #define NEED_CHOOSELANG_UI 0
//#else
//    #define NEED_CHOOSELANG_UI 1
//#endif

/// wxList declaration and partial implementation of the KPlusCList class being
/// a list of pointers to KeyPlusCount objects
WX_DECLARE_LIST(KeyPlusCount, KPlusCList); // see list definition macro in .cpp file

// WX NOTE: The MFC CPtrArray collection class supports arrays of void pointers.
// wxWidgets does not have an exact equivalent, but the docs say it predefines
// the following standard array classes: wxArrayInt, wxArrayLong, and wxArrayPtrVoid.
// We should be able to get the desired behavior with the wxArrayPtrVoid class.

/// A struct for Bible Book folders support. Struct members include: dirName,
/// seeName and bookCode.
struct BookNamePair
{
	wxString dirName;
	wxString seeName;
	wxString bookCode;
};

// whm - the next five were declared static functions in MFC, but not defined that way in
// their implementations. so I've removed "static" here, to avoid gcc warnings
void			SetupSinglesArray(wxArrayPtrVoid* pBooks); // used in default setup,
														   // if "books.xml" absent
void			SetupBookCodesArray(wxArrayPtrVoid* pBookCodes); // used in default
													// setup if "books.xml" is absent
BookNamePair*	MakePair(BookNamePair*& pPair, wxChar* dirNm, wxChar* seeNm);
void			SetupDefaultBooksArray(wxArrayPtrVoid* pBookStructs);

// for USFM and Filtering support
void SetupDefaultStylesMap();

/// An enum for specifying the type of rebuilding to be done when calling
/// DoUsfmSetChanges(), DoPunctuationChanges() and DoUsfmFilterChanges(), either DoReparse
/// or NoReparse.
enum Reparse
{
	DoReparse,
	NoReparse
};

/// An enum for specifying the type of filter status when calling
/// GetUnknownMarkersFromDoc(), one of the following: setAllUnfiltered, setAllFiltered,
/// useCurrentUnkMkrFilterStatus, or preserveUnkMkrFilterStatusInDoc.
enum SetInitialFilterStatus
{
    setAllUnfiltered,
    setAllFiltered,
    useCurrentUnkMkrFilterStatus,
	preserveUnkMkrFilterStatusInDoc
};

/// An enumb for specifying how to handle the loading of data into the filterPage's list
/// boxes, one of the following: LoadInitialDefaults, UndoEdits,
/// ReloadAndUpdateFromDocList or ReloadAndUpdateFromProjList.
enum ListBoxProcess
{
	LoadInitialDefaults,
	UndoEdits,
	ReloadAndUpdateFromDocList,
	ReloadAndUpdateFromProjList,
	ReloadAndUpdateFromFactoryList // this one is now unused
};

/// An enum for specifying which standard format marker set is to be used or is active, one
/// of the following: UsfmOnly, PngOnly, or UsfmAndPng.
enum SfmSet
{
	UsfmOnly,
	PngOnly,
	UsfmAndPng
};

/// An enum for specifying how SetupFilterPageArrays() gets its data for populating its
/// filter flag arrays, can be either useMap or useString.
enum UseStringOrMap
{
	useMap,
	useString
};

/// An enum for specifying how the filter status should be set for unknown markers in the
/// filterPage's unknownMkrStr, either setAsFiltered or setAsUnfiltered.
enum UnkMkrFilterSetting
{
	setAsFiltered,
	setAsUnfiltered
};

/// An enum for specifying how the filter flag in the USFMAnalysis objects should be set in
/// the ResetUSFMFilterStructs() function, either allInSet or onlyThoseInString.
enum resetMarkers
{
	allInSet,
	onlyThoseInString,
};

/// An enum for specifying the footnote caller type when processing and writing destination
/// text, one of the following: auto_number, no_caller, word_literal_caller or
/// supplied_by_parameter.
enum CallerType
{
	auto_number,
	no_caller,
	word_literal_caller,
	supplied_by_parameter
};

/// An enum for specifying the type of error encountered when parsing footnotes, endnotes
/// and cross-references, one of the following: no_error, no_end_marker or
/// premature_buffer_end.
enum ParseError
{
	no_error,
	no_end_marker,
	premature_buffer_end
};

/// An enum for specifying the destination text type in calls to
/// ProcessAndWriteDestinationText(), one of the following: footnoteDest, endnoteDest or
/// crossrefDest.
enum DestinationTextType
{
	footnoteDest,
	endnoteDest,
	crossrefDest
};

enum CollabTextType
{
	collabSrcText,
	collabTgtText,
	collabFreeTransText
};

enum AiProjectCollabStatus
{
	projConfigFileMissing,
	projConfigFileUnableToOpen,
	collabProjMissingFromConfigFile,
	collabProjMissingFromEditorList,
	collabProjExistsButEditorNotInstalled,
	collabProjExistsAndIsValid,
	collabProjExistsButIsInvalid,
	collabProjNotConfigured,
};

// whm 4Feb2020 the following enum is now deprecated
//enum PTVersionsInstalled
//{
//    PTNotInstalled,
//    PTVer7,
//    PTVer8,
//    PTVer7and8,
//    PTVer9, // whm 4Feb2020 added for Paratext 9
//    PTLinuxVer7,
//    PTLinuxVer8,
//    PTLinuxVer7and8, // whm 27Nov2016 note: This enum existed but was not utilized in AI version 6.8.0, but is utilized as of AI 6.8.1
//    PTLinuxVer9 // whm 4Feb2020 added for Paratext9 when it is available for Linux
//};

/// a struct for use in collaboration with Paratext or Bibledit (BEW added 22Jun11)
struct EthnologueCodePair {
	wxString srcLangCode;
	wxString tgtLangCode;
	wxString projectFolderName;
	wxString projectFolderPath;
};

// An enum for specifying the program mode for use in the
// MakeMenuInitializationsAdnPlatformAdjustments(). Values can
// be one of the following: collabIndeterminate, collabAvailableTurnedOn,
// collabAvailableTurnedOff, or collabAvailableReadOnlyOn.
//enum ProgramMenuMode
//{
//	collabIndeterminate,
//	collabAvailableTurnedOn,
//	collabAvailableTurnedOff,
//	collabAvailableReadOnlyOn
//};

/// An enum for specifying the general style type of a standard format marker. Can be one
/// of the following: paragraph, character, table_type, footnote_caller, footnote_text,
/// default_para_font, footerSty, headerSty, horiz_rule, boxed_para ir hidden_note.
enum StyleType
{
	paragraph,
	character,
	table_type,	// whm added 21Oct05 and below
	footnote_caller,
	footnote_text,
	default_para_font,
	footerSty,
	headerSty,
	horiz_rule,
	boxed_para,
	hidden_note
	//note; // usfm.sty has this also as a style type
};

/// An enum for specifying the type of justification to be used in formatting a font's
/// text. Can be one of the following: leading, center, following or justified.
enum Justification
{
	leading,
	center,
	following,
	justified // added for version 3
};

/// An enum for specifying the type of boxed text to be placed around certain types of RTF
/// output. Can be either single_border or double_border.
enum BoxedParagraphType
{
	single_border,
	double_border
};

enum ProgressDialogType
{
	XML_Input_Chunks,
	App_SourcePhrases_Count,
	Adapting_KB_Item_Count,
	Glossing_KB_Item_Count
};

/// An enum for specifying what to do with edit box contents when the
/// StoreFreeTranslation() function is called. Can be either remove_editbox_contents or
/// retain_editbox_contents.
enum EditBoxContents
{
	remove_editbox_contents,
	retain_editbox_contents
};

/// An enum for specifying which pass is being processed in the call to
/// SetupForSFMSetChange(). Can be either first_pass or second_pass.
enum WhichPass // BEW added 10Jun05 for support of SFM set changes done on the fly
{
	first_pass,
	second_pass
};

/// An enum for specifying the fields of a USFMAnalysis struct, used in
/// ParseAndFillStruct(). Can be one of the following: marker, endMarker, description,
/// usfm, png, filter, userCanSetFilter, inLine, special, bdryOnLast, inform,
/// navigationText, textType, wrap, styleName, styleType, fontSize, color, italic, bold,
/// underline, smallCaps, superScript, justification, spaceAbove, spaceBelow,
/// leadingMargin, followingMargin, firstLineIndent, basedOn, nextStyle, keepTogether or
/// keepWithNext.
enum USFMAnalysisField
{
	// these field name are referenced in ParseAndFillStruct() in Adapt_It.cpp
	marker,
	endMarker,
	description,
	usfm,
	png,
	filter,
	userCanSetFilter,
	inLine,
	special,
	bdryOnLast,
	inform,
	navigationText,
	textType,
	wrap,
	styleName,	// addition for version 3
	styleType,
	fontSize,
	color,
	italic,
	bold,
	underline,
	smallCaps,
	superScript,
	justification,
	spaceAbove,
	spaceBelow,
	leadingMargin,
	followingMargin,
	firstLineIndent,
	basedOn,		// addition for version 3
	nextStyle,		// addition for version 3
	keepTogether,	// addition for version 3
	keepWithNext	// addition for version 3
};

// for USFM and Filtering support
// When adding or remove attributes from USFMAnalysis, the following need to be
// checked for consistency:
// 1. This USFMAnalysis struct
// 2. The enum USFMAnalysisField above plus any dependent enum for the added attribute
// 3. The ParseAndFillStruct function in Adapt_It.cpp
// 4. Add any necessary const char defined string literals to beginning of XML.cpp
// 5. Change/add any necessary else if clause to the AtSFMAttr function in XML.cpp
// 6. Change/add any necessary else if clause to the ParseXMLAttribute function in
//    XML.cpp (this would be appropriate for any attribute additions involving enum values)
// 7. Change/add any necessary new USFMAnalysis struct default value for the attribute
//    in AtSFMTag in XML.cpp
// 8. Change/add any necessary > if(comment) dup else '' endif  line in UsfmXml.cct if
//    there is a default form of the attribute which should not be placed in AI_USFM.xml
//    to save space.
// 9. The AI_USFM_full.xml attributes need to be added to the markers defined there,
//    if applicable.
// 10. Using the stand-alone Consistent Changes program with UsfmXml.cct and
//    UsfmXmlTidy.cct, then the resulting AI_USFM.xml file deposited/installed to the
//    Adapt It (Unicode) Work directory.
// 11. The const wxString defaultSFM[] unix-style default strings (defined in
//    Adapt_It.cpp) need to be re-assembled by moving the updated AI_USFM_full.xml file
//    to the Adapt It (Unicode) Work directory, then uncommenting the
//    #define Output_Default_Style_Strings symbol near the beginning of the
//    XML.h header file, and compiling/running the program until it reaches the
//    Start Working Wizard. An AI_USFM_full.txt file is generated automatically in
//    the Adapt It (Unicode) Work directory that contains the unix style default
//    strings to be copied over those located in Adapt_It.cpp.

/// A struct for storing the attributes of usfm markers. Structs are created on the heap
/// and their pointers are associated with the standard format marker name in high-speed
/// maps for each of the three possible sfm sets. Struct members include the following:
/// marker, endMarker, description, usfm, png, filter, userCanSetFilter, inLine, special,
/// bdryOnLast, inform, navigationText, textType, wrap, styleName, styleType, fontSize,
/// color, italic, bold, underline, smallCaps, superScript, justification, spaceAbove,
/// spaceBelow, leadingMargin, followingMargin, firstLineIndent, basedOn, nextStyle,
/// keepTogether or keepWithNext.
struct USFMAnalysis
{
	wxString marker; // this is the marker, without the initial gSFescapechar
	wxString endMarker; // likewise, lacks initial gSFescapechar
	wxString description;
	bool usfm;
	bool png;
	bool filter;
	bool userCanSetFilter;
	bool inLine;
	bool special;
	bool bdryOnLast;
	bool inform;
	wxString navigationText;
	enum TextType textType;
	bool wrap;
	wxString styleName; // addition for version 3
	enum StyleType styleType;
	int fontSize;
	int color;
	bool italic;
	bool bold;
	bool underline;
	bool smallCaps;
	bool superScript;
	enum Justification justification;
	int spaceAbove;
	int spaceBelow;
	float leadingMargin;
	float followingMargin;
	float firstLineIndent;
	wxString basedOn;	// addition for version 3
	wxString nextStyle;	// addition for version 3
	bool keepTogether;	// addition for version 3
	bool keepWithNext;	// addition for version 3
};

// whm added 31Aug10 for User Workflow Profiles support

enum VersionComparison
{
	sameAppVersion,
	runningAppVersionIsNewer,
	runningAppVersionIsOlder,
	profileVersionDiffers
};

// edb added 23Oct12 for new toolbar support
enum ToolbarButtonSize
{
	btnSmall,	// 16x16 (old tb size)
	btnMedium,	// 22x22 (standard linux tb size)
	btnLarge	// 32x32 (OLPC XO size)
};

struct ToolbarButtonInfo
{
	int toolId;
    wxString label;
    wxString shortHelpString;
    wxString longHelpString;
	wxBitmap bmpSmall;
	wxBitmap bmpMedium;
	wxBitmap bmpLarge;
};

struct UserProfileItem
{
	wxString itemID;
	int itemIDint;
	wxString itemType;
	wxString itemText;
	wxString itemDescr;
	wxString adminCanChange;
	wxArrayString usedProfileNames;
	wxArrayString usedVisibilityValues;
	wxArrayString usedFactoryValues;
};

// whm 4Feb2020 checked - no revisions needed for PT 9
// whm 1Sep2021 added a wxString member to the struct
// that indicates a path to the PT project's autocorrect.txt
// file. If a PT project has no autocorrect.txt file in its
// project folder the autoCorrectPath value will remain an 
// empty string
struct Collab_Project_Info_Struct // whm added 26Apr11 for AI-PT Collaboration support
{
	// Note: Paratext .ssf files also have some tag fields that provide file naming
	// structure, i.e., <FileNameForm>, <FileNamePostPart>, <FileNamePrePart>. Since
	// by using rdwrtp7.exe/rdwrtp8.exe/rdwrtp9.exe, we don't have to know the actual 
    // Paratext file names, or do any file name parsing, those fields are not really 
    // significant to Adapt It.
	bool bProjectIsNotResource; // default is TRUE
	bool bProjectIsEditable; // default is TRUE
	wxString versification; // default is _T("");
	wxString fullName; // default is _T("");
	wxString shortName; // default is _T(""); // same as projectDir below
	wxString languageName; // default is _T("");
    wxString fileNamePrePart; // whm added 30Nov2016
    wxString fileNamePostPart; //  whm added 30Nov2016
    wxString fileNameBookNameForm; // whm added 30Nov2016
	wxString ethnologueCode; // default is _T(""); // PT 8 Settings.xml tag is <LanguageIsoCode> and maps to this ethnologueCode
	wxString projectDir; // default is _T(""); // In PT 8 this is taken from the dir name that the Settings.xml file is located in
	wxString booksPresentFlags; // default is _T("");
	wxString chapterMarker; // default is _T("c"); // no longer used in PT but doesn't hurt to have it
	wxString verseMarker; // default is _T("v"); // no longer used in PT 8 but doesn't hurt to have it
	wxString defaultFont; // default is _T("Arial")
	wxString defaultFontSize; // default is _T("10");
	wxString leftToRight; // default is _T("T");
	wxString encoding; // default is _T("65001"); // 65001 is UTF8
    wxString collabProjectGUID;
	wxString autoCorrectPath; // default is _T("") // whm added 1Sep2021
};

/// wxList declaration and partial implementation of the ProfileItemList class being
/// a list of pointers to UserProfileItem objects
WX_DECLARE_LIST(UserProfileItem, ProfileItemList); // see list definition macro in .cpp file

struct UserProfiles
{
	wxString profileVersion;
	wxString applicationCompatibility;
	wxString adminModified;
	wxArrayString definedProfileNames;
	wxArrayString descriptionProfileTexts;
	ProfileItemList profileItemList;
};

struct AI_SubMenuItem
{
	//wxString subMenuID;
	int subMenuIDint;
	wxString subMenuLabel;
	wxString subMenuHelp;
	wxString subMenuKind;
};

/// wxList declaration and partial implementation of the SubMenuItemList class being
/// a list of pointers to AI_SubMenuItem objects
WX_DECLARE_LIST(AI_SubMenuItem, SubMenuItemList); // see list definition macro in .cpp file

struct AI_MainMenuItem
{
	//wxString mainMenuID;
	int mainMenuIDint;
	wxString mainMenuLabel;
	SubMenuItemList aiSubMenuItems;
};

/// wxList declaration and partial implementation of the MainMenuItemList class being
/// a list of pointers to AI_MainMenuItem objects
WX_DECLARE_LIST(AI_MainMenuItem, MainMenuItemList); // see list definition macro in .cpp file


struct AI_MenuStructure
{
	MainMenuItemList aiMainMenuItems;
};

enum EmailReportType
{
	problemReport,
	feedbackReport
};

struct EmailReportData
{
	enum EmailReportType reportType;
	wxString fromAddress;
	wxString toAddress;
	wxString subjectSummary;
	wxString emailBody;
	wxString sendersName;
	wxString usageLogFilePathName;
	wxString packedDocumentFilePathName;
};

enum freeTransModeSwitch
{
	ftModeOFF,
	ftModeON
};

//enum composeBarViewSwitch // whm 24Nov2015 moved to MainFrm.h
//{
//	composeBarHide,
//	composeBarShow
//};

enum UniqueFileIncrementMethod
{
	incrementViaNextAvailableNumber,
	incrementViaDate_TimeStamp
};

/// wxHashMap declaration for the MapMenuLabelStrToIdInt class - a mapped association
/// of Menu label keys (wxString) with integers representing Menu Id int values.
WX_DECLARE_HASH_MAP(wxString,
					int,
					wxStringHash,
					wxStringEqual,
					MapMenuLabelStrToIdInt);

/// wxHashMap declaration for the MapProfileChangesToStringValues class - a mapped
/// association of profile item (compound) keys (wxString) with strings representing
/// their values. Instantiated in m_mapProfileChangesToStringValues.
WX_DECLARE_HASH_MAP(wxString,
					wxString,
					wxStringHash,
					wxStringEqual,
					MapProfileChangesToStringValues);


/// wxHashMap declaration for the MapSfmToUSFMAnalysisStruct class - a mapped association
/// of sfm marker keys (wxString) with pointers to USFMAnalysis objects.
WX_DECLARE_HASH_MAP( wxString,		// the map key is the sfm marker (bare marker without backslash)
                    USFMAnalysis*,	// the map value is the pointer to the USFMAnalysis struct instance
                    wxStringHash,
                    wxStringEqual,
                    MapSfmToUSFMAnalysisStruct ); // the name of the map class declared by this macro

/// wxHashMap declaration for the MapWholeMkrToFilterStatus class - a mapped association of
/// whole sfm markers including backslash (wxString) mapped with wxString equivalents of
/// zero "0" or one "1".
WX_DECLARE_HASH_MAP( wxString,		// the map key is the whole sfm marker (with backslash)
                    wxString,		// the map value is the string "0" or "1"
                    wxStringHash,
                    wxStringEqual,
                    MapWholeMkrToFilterStatus ); // the name of the map class declared by this macro

/// whm 23Aug2021 added the following for the AutoCorrect feature.
/// wxHashMap declaration for the MapAutoCorrectStrings class - a mapped association of 
/// left-hand sub-strings to right-hand replacements/corrections that are read from any
/// autocorrect.txt file stored in a user's project folder.
WX_DECLARE_HASH_MAP(wxString,		// the map key is the left-hand string of an autocorrect.txt rule
					wxString,		// the map value is the right-hand string of an autocorrect.txt rule
					wxStringHash,
					wxStringEqual,
					MapAutoCorrectStrings); // the name of the map class declared by this macro

enum box_cursor {
	select_all,
	cursor_at_text_end,
	cursor_at_offset
};

enum removeFrom {
	from_source_text,
	from_target_text
};

// The following enums and struct were added by Bruce 12Sep08 for support of Vertical
// Editing. They were located in the global space of CAdapt_ItView.h in the MFC version.

enum ListEnum
{
	adaptationsList,
	glossesList,
	freeTranslationsList,
	notesList
};
enum WhichContextEnum
{
	precedingContext,
	followingContext
};
// BEW added 16Apr08 to support refactored source text editing and vertical editing in
// general
enum EntryPoint
{
	noEntryPoint,
	sourceTextEntryPoint,
	adaptationsEntryPoint,
	glossesEntryPoint,
	freeTranslationsEntryPoint
};

enum EditStep
{
	noEditStep, // the value when no vertical editing is going on currently
	sourceTextStep,
	adaptationsStep,
	glossesStep,
	freeTranslationsStep,
	backTranslationsStep
};

// enums for the action selector for Vertical Edit transition dialog
enum ActionSelector {
	pleaseIgnore = 0,
	nextStep,
	previousStep,
	endNow,
	cancelAllSteps
};

// enums for the GetBar() function
enum VertEditBarType
{
	Vert_Edit_RemovalsBar,
	Vert_Edit_Bar,				// IDD_VERT_EDIT_BAR
	Vert_Edit_ComposeBar
};

// enum for user navigation protection support, or more specifically, to allow control of
// whether or not a call of EnumerateLoadableSourceTextFiles() is to do filtering or not
// of loadable versus unloadable files for doc creation
enum LoadabilityFilter {
	doNotFilterOutUnloadableFiles,
	filterOutUnloadableFiles
};

enum ReadOnlyProtectionInclude
{
	includeFictitiousROP,
	excludeFictitiousROP
};

typedef struct
{
    // Note: the "editable span" means the user's selection, except when it encroaches on
    // part of a retranslation, in which case the span is programmatically increased to
    // include the whole of the retranslation in the editable span. (And if the user's edit
    // is accepted, the retranslation is lost automatically and if wanted, would have to be
    // manually reconstituted.)

    // Note 2: after a source text edit, the user will be given opportunity to adapt the
    // new material - when he does this, he will be free to create mergers; hence, some of
    // the indices and the count of the number of CSourcePhrase instances in the editable
    // span after editing was completed, which are defined below, will need to be
    // programmatically decreased accordingly. Additional structs will be used for such
    // lower level entry points to the editing process, so that the user can roll back the
    // total vertical editing process at any stage before it ends. Once it ends, the memory
    // of what was done is deleted and the only rollback strategy is manual editing, or to
    // do a new source text edit at the original location.

    // Note 3: our design for the spans involves 2 spans with associated CObLists
    // containing deep copies of CSourcePhrase instances from m_pSourcePhrases list in the
    // document class: the "cancel span" to be used in restoring the original document (see
    // further comments below) if the user Cancels later on, and a modifications span
    // (coextensive with the cancel span) in which modifications are made to the deep
    // copies of the CSourcePhrase instances, readying them for the user's OK button click.
    // There are also three additional overlapping subspans, defined by starting and ending
    // index pairs using sequence number values, but these subspans do not have separate
    // CObLists associated with them, rather, they are defined on both the cancel span and
    // the modifications span, as those spans are coextensive.

    // The first (and minimal length) subspan is the editable span. This is the user's
    // selection, but if the selection overlaps one or more retranslations, the span is
    // widened to include the retranslation/s in its/their entirety.
    // The second subspan is the "free translations span" - it is a span of CSourcePhrase
    // instances where free translation removals need to be done.
    // The third subspan is the "collected back translations span" - it is a span of
    // CSourcePhrase instances where stored back translations have their scope overlapping
    // the information in the editable span (remembering that the editable span may be
    // exended programmatically wider than the user's selection due to the presence of one
    // or more retranslations in the user's selection, or overlapping the user's
    // selection.)
    // These subspans overlap, and involve an inclusion hierarchy: the editable span is
    // equal to or included in the free translation span which is equal to or included in
    // the (collected) back translations span.
    //
    // The modifications span is coextensive with the cancel span and contains deep copies
    // of the CSourcePhrase instances (with sequence numbering preserved). The
    // modifications are to make ready for the CSourcePhrase instances to be inserted in
    // the document's m_pSourcePhrases list once the user dismisses the edit source text
    // dialog with an OK button click. These placements involve replacement of the
    // instances in the editable subspan with CSourcePhrase instances created anew from the
    // edited source text (there may be zero, if the user edits out the whole of the
    // contents of the editable source text string seen in the dialog); but for any
    // extended regions preceding and following the editable span up to the bounds of the
    // cancel span, replacements of the CSourcePhrase instances are done in those regions
    // after the OK button click using the modified CSourcePhrase instances from the
    // modifications span. (The modifications involve the removal of notes, free
    // translations and collected back translations in the appropriate subspans,so that the
    // user does not have to look at their distracting (and possibly copious) information
    // while editing the editable span.) (The cancel span is maintained in case the user
    // Cancels out of the whole vertical editing process, it is not needed for cancelling
    // from the Edit Source Text dialog because when the user presses the Cancel button
    // there, no changes have been made to the original document up to that point.)

    // Note 4: \bt information might be found to have been stored in the editable subspan,
    // and more than one instance could be stored there; the final one of any such could
    // have a "span" (over which collection originally happened) which extends beyond the
    // editable subspan, and may even extend beyond the end of the free translations
    // subspan too - as far as it takes to get to the next location defined by
    // HaltCurrentCollection(). The nBackTrans_EndingSequNum member tracks the
    // CSourcePhrase instance immediately preceding this halt location.

    // Note 5: Because endmarkers are stored on the CSourcePhrase instance immediately
    // following the span of information to which they logically belong, it is possible for
    // the editable span's bounding CSourcePhrase instances to have (a) endmarkers at the
    // start which belong to the preceding context, and / or (b) endmarkers immediately
    // following the end of the editable span which logically belong within it. So we
    // maintain two CString members to store any such endmarker sequences from either or
    // both locations, so that the user can be shown the most logically meaningful editable
    // text string, and to enable replacement of either or both endmarker strings after the
    // edit is done - if that is an appropriate editing operation for the user to do.

	// For docVersion = 5, we no longer store endmarkers at the beggining of a following
	// srcPhrase's m_markers member, but on the relevant one which ends the information
	// type, on its new m_endMarkers member. This makes the comment for Note 5 above
	// obsolete. At the start of the editable span, if the previous CSourcePhrase instance
	// ends a information type with an endmarker, that endmarker is stored in the
	// preceding context in that CSourcePhrase instance's m_endMarkers member. Also, if
	// the editable span ends with endmarkers, they are now stored on the last
	// CSourcePhrase instance of the editable span, again in its m_endMarkers member. So
	// we don't have to make any adjustments for endmarker transfers any longer.

    // Note 6: We also maintain an integer array to store the sequence numbers for the
    // storage locations of any Adapt It Notes which lie within the editable subspan. These
    // location values will be used (and possibly changed, depending on what the user does
    // in his editing - he may remove all source text where they originally were located)
    // in order to programmatically determine suitable locations for rebuilding the notes
    // programmatically once the edit is done. Notes content is not changed - if changes
    // are warranted, the user will have to do them manually after the total vertical edit
    // process ends.

	CSourcePhrase* activeCSourcePhrasePtr; // store the ptr to the active one, on entry
										   // BEW added this member on 21Nov12
	bool	bGlossingModeOnEntry; // at entry, TRUE if glossing mode is ON, FALSE if
                // adapting mode is ON, vertical edit is not enabled in any other mode
                // (such as free trans mode). Default is adaptations mode is currently ON,
                // even when it isn't; this is safe because the flag is set or cleared at
                // the start of vertical editing and it is used only for restoring the
                // initial entry state after vertical editing finishes
	bool	bSeeGlossesEnabledOnEntry; // TRUE if See Glosses is ON (ie. gbGlossingEnabled
                // is TRUE) on entry, FALSE if it is OFF on entry (if off, gbIsGlossing is
                // also FALSE; but if on, gbIsGlossing can be TRUE or FALSE). Used for
                // restoring initial entry state after vert edit finishes
	bool	bEditSpanHasAdaptations; // TRUE if adaptations are detected within the
				// editable span
	bool	bEditSpanHasGlosses; //  TRUE if glosses are detected within the editable span
	bool	bEditSpanHasFreeTranslations; // TRUE if free translations are in the editable
                // span (the process looks at CSourcePhrase instances earlier and later
                // than the editable span in order to make sure the editable span is
                // included within an integral number of free translation sections)
	bool	bEditSpanHasBackTranslations; // TRUE if collected backtranslations are in the
                // editable span. Because each back translation is stored on only one
                // CSourcePhrase instance which is first in the span over which its
                // collection happened, and sets no flag in CSourcePhrase, we need only
                // look for \bt marker content in the editable span, and in any preceding
                // context - going back only as far as the first halt position as
                // determined by the HaltCurrentCollection() function.
	bool	bCollectedFromTargetText; // TRUE if back translations were collected from the
				// target text line, FALSE if collected from the gloss line

	int		nSaveActiveSequNum; // location of the phrase box at entry
				// (needed only for a Cancel operation)
	wxString	oldPhraseBoxText; // contents of the phrase box when the edit was invoked
	TextType	nStartingTextType; // value of m_curTextType at the CSourcePhrase with
				// sequence number nStartingSequNum
	TextType	nEndingTextType; // value of m_curTextType at the CSourcePhrase with
                // sequence number nEndingSequNum (It is not possible to select across a
                // TextType boundary, and the 'none' TextType never puts a value 6 into the
                // document, instead it propagates the context's TextType through that
                // section, and so we can be confident that the TextType won't change
                // unless the user's edit has changed or introduced SF markers which change
                // the TextType -- in that case we'll need to propagate the new
                // valueforward, potentially past the end of the editable span, as far as
                // the next m_bFirstOfType == TRUE locations, but not including that
                // location.)
	wxArrayString	deletedAdaptationsList; // CStringList	deletedAdaptationsList; this is
                // the last 100 deletions resulting from either source text editing or any
                // adaptations edit operations. This list is maintained for the life of the
                // Adapt It session. A "Deletions List" button in a bar of the GUI will
                // show the list data in a combobox, in adapting mode.
	wxArrayString	deletedGlossesList; // CStringList	deletedGlossesList; // this is
                //the last 100 deletions resulting from either source text editing or any
                //adaptations edit operations - provided the CSourcePhrase instances which
                //are involved also had glosses stored on them (not necessarily on all of
                //them). This list is maintained for the life of the Adapt It session. A
                //"Deletions List" button in a bar of the GUI will show the list data in a
                //combobox, in glossing mode.
	wxArrayString deletedFreeTranslationsList; // CStringList	deletedFreeTranslationsList;
                // this isthe last 100 deletions resulting from either source text,
                // adaptations editing, or any use of the Remove Free Translation button in
                // the View Filtered Information dialog, or any use of the Remove button
                // when in Free Translation mode, or editing of an existing free free
                // translation. This list is maintained for the life of the Adapt It
                // session. It's members will be available in the GUI in free translation
                // mode via a combobox, as above.
	wxArrayString storedNotesList; // CStringList	storedNotesList; // the is the text
                // strings for any Adapt It notes within the span defined by the editable
                // span when doing a source text edit are stored temporarily here, and at
                // the end of the editing process they are automatically put back into the
                // document at approximately the same relative positions, the order of
                // these notes is never scrambled, and if the new source text is very short
                // or empty, they are stored in the immediate context wholely or partly as
                // circumstances require.
	int		nStartingSequNum;	// value of m_nSequenceNum for the start of the editable
                // span, or for adaptations editing, the start of the span of CSourcePhrase
                // instances that were involved.
	int		nEndingSequNum;	// value of m_nSequenceNum for the last CSourcePhrase of the
                // editable span, or for editing of adaptations, the end of the span of
                // CSourcePhrase instances that were involved
	int		nFreeTrans_StartingSequNum; // sequence number for the CSourcePhrase instance
                // at the start of the span of removed free translations (it can coincide
                // with the start of the editable span but more commonly is going to be
                // somewhere preceding that location)
	int		nFreeTrans_EndingSequNum; // sequence number for the last CSourcePhrase instance
                // at the end of the span of removed free translations (it can coincide
                // with the end of the editable span but more commonly is going to be
                // somewhere following that location) the span of removed free translations
                // which follow (overlap) the editable span
	int		nBackTrans_StartingSequNum; // sequence number for the CSourcePhrase instance
                // from which the first (or possibly only) \bt marker and its back
                // translation content were removed, this span (if it exists) has its start
                // at the free translation one's start or even before that, and if no free
                // translation span occurs then it would be at or preceding the start of
                // the editable span
	int		nBackTrans_EndingSequNum; // sequence number for the end of the back
                // translation span, this span (if it exists) at the end of the editable
                // span, or somewhere following that, depending on where the helper
                // function, HaltCurrentCollection(), determines that collecting back
                // translations needs to end once the editable span has been traversed.
	int		nCancelSpan_StartingSequNum; // sequence number for the first CSourcePhrase
                // in the span delineated for restoring the original document state if the
                // user cancels after a source text edit has been accepted and he is
                // cancelling out of subsequent dependent (vertical) edits.
	int		nCancelSpan_EndingSequNum; // sequence number for the last CSourcePhrase in
                // the span delineated for restoring the original document state if the
                // user cancels after a source text edit has been accepted and he is
                // cancelling out of subsequent dependent (vertical) edits.
	SPList cancelSpan_SrcPhraseList; // sequence of deep copied unedited CSourcePhrase
                // instances from the span for of CSourcePhrase instances which get changed
                // in any way (not all need to be changed, the span is widened at either
                // end to include the leftmost and rightmost modified instances.)
	SPList	modificationsSpan_SrcPhraseList; // sequence of deep copied CSourcePhrase
                // instances, coextensive with the cancel span (but containing fresh deep
                // copies), which will be modified (by having notes, free translations and
                // collected back translations removed) prior to showing the Edit Source
                // Text dialog, reading the information for the possibility that the user
                // will click the OK button rather than the Cancel button. Up until the OK
                // button click, no CSourcePhrase instances in the document's original
                // m_pSourcePhrases list are modified in any way; after the click, what is
                // in the modificationSpan list is used to make the document comply with
                // the user's edit result.
	SPList editableSpan_NewSrcPhraseList; // the sequence of CSourcePhrase instances
                // resulting from the TokenizeTextString() call with the user's edited
                // source text string as a parameter
	SPList propagationSpan_SrcPhraseList; // stores a deep copy of the CSourcePhrase
                // instance which is first in the following context (deep copied before
                // sequence numbers are changed), and any of the CSourcePhrase instances
                // which follow it which take part in any propagation of special text and
                // TextType values as a consequence of changes made within the user's
                // edited source text
	int nPropagationSpan_StartingSequNum; // the index for the first CSourcePhrase instance
				// of the following context
	int nPropagationSpan_EndingSequNum; // the index for the last CSourcePhrase in the
                // following context which was affected by the propagation process
	wxArrayInt arrNotesSequNumbers; //CArray<int,int> arrNotesSequNumbers; // preserve old
				// location of each removed note in the editable subspan
	int nOldSpanCount; // the original (after any extension) editable span's number of
				// CSourcePhrase instances
	int	nNewSpanCount; // the final number of CSourcePhrase instances in the editable span
                // after the user has completed his editing of the source text (or whatever
                // the edit did, eg. removal of a merger - but this additional stuff will
                // only be possible within the wxWidgets versions)
	bool bSpecialText; // stores the m_bSpecialText boolean value for the first
                // CSourcePhrase instance in the editable span; TRUE if it was special
                // text, FALSE if verse or poetry
	SPList follNotesMoveSpanList; // one or more CSourcePhrase pointers from the
                // context following the edit span, which may have had Notes moved at the
                // Note restoration stage, only used if arrNotesSequNumbers has non-zero
                // content (and the latter determines how many instances without notes on
                // them are stored in follNotesMoveSpanList to ensure safe restoration if
                // the user clicks Cancel)
	SPList precNotesMoveSpanList; // one or more CSourcePhrase pointers from the context
                // preceding the edit span, which may have had Notes moved at the Note
                // restoration stage, only used if arrNotesSequNumbers has non-zero content
                // (and the latter determines how many instances without notes on them are
                // stored in precNotesMoveSpanList to ensure safe restoration if the user
                // clicks Cancel)
	bool bTransferredFilterStuffFromCarrierSrcPhrase; // FALSE, except it is TRUE in the
                // special circumstance that the user's edit resulted in a single
                // CSourcePhrase with empty key, and m_precPunct empty and m_filteredInfo
                // containing filtered information (this can only happen when he edited a
                // misspelled marker and the final marker form matches one designated as
                // "to be filtered out") and the whole, or end, of the user's edit string
                // was then filtered and placed in the m_filteredInfo member of the final
                // carrier CSourcePhrase; typically it would be the 'whole' rather than the
                // 'end' because the user normally cannot select across a TextType boundary
				// in the first place) This boolean is advisory only, it gets set TRUE
				// only if transfer of filtered info was done; it isn't used in a test in
				// order to govern how some part of the code should work

                // BEW added comment 22Mar10 for doc version 5. The above is still true
                // except that there is not a single location for any transferred filtered
                // stuff, but, depending on what info was there, it could be one or more of
                // m_freeTrans, m_note, m_collectedBackTrans, or m_filteredInfo

	bool bDocEndPreventedTransfer; // FALSE by default; TRUE if there was created an empty
                // carrier CSourcePhrase for storing final endmarker(s), or now-filtered
                // information, and there is no following context to which transfer of this
                // information to the m_markers member of a CSourcePhrase there is
                // possible. This flag is ignored, except when the user Cancels or an error
                // causes bail out to be done; it is used as a quick way to inform the
                // bailout function that document restoration requires the document-ending
                // CSourcePhrase instance to be deleted (when TRUE). When there is a
                // following context, transfer will have been possible and this flag will
                // be FALSE, and the earier state of the CSourcePhrase which was initial in
                // the following context will be the one at the start of the propagation
                // span.
	bool bExtendedForFiltering; // FALSE by default. TRUE if marker edit results in the
                // edited marker being one which should be filtered, AND, the editable span
                // did not include all of the marker's filterable content - so that AI had
                // to extend the editable span by one or more words to get all that
                // material into the new source text string (we may not need to use this
                // bool value, but we store it in case it becomes important when I work on
                // the backtracking mechanism for stepping back through steps in the
                // vertical edit process, and/or bailout recovery)

    // next ones are for support of the adaptations update step; but when entry is at the
    // adaptations level (ie. user has changed an established adaptation), free
    // translations etc will need to be removed and a cancelSpan set up, so for that, the
    // cancelSpan supporting members above will be used.
    // The main things to note about the adaptations update step are that:
    // (1) CSourcePhrases outside the span must not be changed (this can be done by
    // limiting the selections possible),
    // (2) the start of the span is fixed (because no source text changes are done),
    // (3) the end of the span may grow or contract (the former caused by one or more long
    // retranslations, the latter by one or more mergers, (4) extra members needed in the
    // EditRecord are few
	bool bAdaptationStepEntered; // default FALSE, TRUE if this step is set up on the
                // screen, even if user then jumps to next step (TRUE is a flag which says
                // that the step was entered, even if immediately exitt ed; so data members
                // here can be expected to perhaps have content, and therefore need to be
                // looked at by processes such as cancel or bailout)
	SPList adaptationStep_SrcPhraseList; // the list of CSourcePhrase instances in the
                // editable span, as it is at the start of the adaptations update step
                // before the user has had a chance to do anything
	int	nAdaptationStep_StartingSequNum; // the (fixed) sequence number value for the first
				// CSourcePhrase in the span
	int	nAdaptationStep_EndingSequNum; // the (fixed) sequence number value for the last
				// CSourcePhrase in the span
	int nAdaptationStep_OldSpanCount; // how many instances are in the span when the step
				// is first entered
	int nAdaptationStep_NewSpanCount; // how many are in the span as the user does his
                // updating of adaptations - mergers and / or retranslations and / or
                // placeholder insertions can alter the span breadth.
	int nAdaptationStep_ExtrasFromUserEdits; // the final number of extras (can be -ve)
                // due to the cumulative effect of the user's editing work in the
                // adaptationsStep (other steps need this value so as to be able to adjust
                // their spans when control enters them

    // next ones are for the support of the glosses update step; but when entry is at the
    // adaptations level (ie. user has changed an established adaptation), free
    // translations etc will need to be removed and a cancelSpan set up, so for that, the
    // cancelSpan supporting members above will be used.
	// The main things to note about the glosses update step are that:
    // (1) CSourcePhrases outside the span must not be changed (this can be done by
    // limiting the selections possible),
    // (2) the start of the span is fixed (because no source text changes are done),
    // (3) the end of the span is also fixed - because this mode does not permit mergers
    // etc,
	// (4) extra members needed are equivents for those for adaptationsStep
	bool bGlossStepEntered; // default FALSE, TRUE if this step is set up on the screen,
                // even if user then jumps to next step (TRUE is a flag which says that the
                // mode was set up, but data members here may or may not be expected to
                // have content, and therefore need to be looked at by processes such as
                // cancel or bailout)
	SPList glossStep_SrcPhraseList; // the list of CSourcePhrase instances in the editable
                // span, as it is at the start of the glosses update step before the user
                // has had a chance to do anything; so it actually preserves the state of
                // whatever span was the last step
	int	nGlossStep_StartingSequNum; // the (fixed) sequence number value for the first
				// CSourcePhrase in the span
	int	nGlossStep_EndingSequNum;   // the (fixed) sequence number value for the last
				// CSourcePhrase in the span
	int nGlossStep_SpanCount; // how many instances are in the span when the step is
				// first entered (stays constant)

	// next group are unique to the free translations update step
	bool bFreeTranslationStepEntered; // TRUE once control has been in this step once,
				// even if briefly
	bool bVerseBasedSection; // default FALSE, TRUE if it looks like the section was
				// created with the radio button "Verse" turned on
	SPList freeTranslationStep_SrcPhraseList; // we only need the initial list, because
                // the user is unable to return to this step once backTranslationsStep has
                // been entered, and so the state of the span when freeTranslationsStep is
                // first entered is actually the final state of the last step - that's
                // what we are storing
	int			nFreeTranslationStep_StartingSequNum;
	int			nFreeTranslationStep_EndingSequNum;
	int 		nFreeTranslationStep_SpanCount;

    // span for collected back translations can be worked out from nBackTrans... starting
    // and ending indices, together with the difference between the old and new span counts
    // in the adaptations Step; backTranslationsStep is not reversible and once there the
    // process is automatically completed without user intervention and the vertical edit
    // process terminates
} EditRecord;

/// The AIModalDialog class is used as the base class for most of Adapt It's modal dialogs.
/// Its primary purpose is to turn off background idle processing while the dialog is being
/// displayed.
/// \derivation The AIModalDialog is derived from wxDialog in wxWidgets 2.9.x; wxScrollingDialog
/// for earlier versions of wxWidgets.
// whm 14Jun12 modified to use wxDialog for wxWidgets 2.9.3 and later; wxScrollingDialog for pre-2.9.x
#if wxCHECK_VERSION(2,9,0)
// Use the built-in wxConvAuto from <wx/version.h>
class AIModalDialog : public wxDialog
#else
// The wxWidgets library being used is pre-2.9.x, so use our own
// wxScrollingDialog located in scrollingdialog.h
class AIModalDialog : public wxScrollingDialog
#endif
{
public:
    AIModalDialog(wxWindow *parent, const wxWindowID id, const wxString& title,
            const wxPoint& pos = wxDefaultPosition,
            const wxSize& size = wxDefaultSize,
            const long style = wxDEFAULT_DIALOG_STYLE);
	~AIModalDialog(); // destructor calls wxIdleEvent::SetMode(wxIDLE_PROCESS_ALL)
					  // before calling the class destructor
	int ShowModal(); // calls wxIdleEvent::SetMode(wxIDLE_PROCESS_SPECIFIED) before
					 // calling ::ShowModal()
};

// whm 31Aug2021 added a simple class named AutoCorrectTextCtrl
// that derives directly from the wxWidgets wxTextCtrl.
// This is a class that will allow us to access the text control's
// OnChar() method (like CPhraseBox does), in order to implement an
// AutoCorrect feature for text controls in used in AI's dialogs that
// accept target language text where auto-correct can be of assistance.
class AutoCorrectTextCtrl : public wxTextCtrl
{
public:
	AutoCorrectTextCtrl(void);
	AutoCorrectTextCtrl(wxWindow* parent, wxWindowID id, const wxString& value,
		const wxPoint& pos, const wxSize& size, int style = 0)
		: wxTextCtrl(parent, id, value, pos, size, style)
	{
	}
	virtual ~AutoCorrectTextCtrl(void); // destructor

	// other methods
	//void OnKeyUp(wxKeyEvent& event);
	//void OnKeyDown(wxKeyEvent& event);
	void OnChar(wxKeyEvent& event);
	//void OnEditBoxChanged(wxCommandEvent& WXUNUSED(event));
private:
	// class attributes

	//DECLARE_CLASS(AutoCorrectTextCtrl);
	// Used inside a class declaration to declare that the class should
	// be made known to the class hierarchy, but objects of this class
	// cannot be created dynamically. The same as DECLARE_ABSTRACT_CLASS.

	// or, comment out above and uncomment below to
	DECLARE_DYNAMIC_CLASS(AutoCorrectTextCtrl)
	// Used inside a class declaration to declare that the objects of
	// this class should be dynamically creatable from run-time type
	// information.

	DECLARE_EVENT_TABLE()
};


class wxDynamicLibrary;
class AI_Server;

//#if defined(_KBSERVER)
class Timer_KbServerChangedSince;
//#endif // _KBSRVER

//////////////////////////////////////////////////////////////////////////////////
/// The CAdapt_ItApp class initializes Adapt It's application and gets it running. Most of
/// Adapt It's global enums, structs and variables are declared either as members of the
/// CAdapt_ItApp class or in this source file's global space.
/// \derivation		The CAdapt_ItApp class is derived from wxApp, and inherits its support
///                 for the document/view framework.
class CAdapt_ItApp : public wxApp
{
  public:
    CAdapt_ItApp();
	virtual ~CAdapt_ItApp(); // whm make all destructors virtual
    CMainFrame* GetMainFrame();

	wxTimer m_timer;

    // whm 15Apr2019 enabled FilterEvent()
    virtual int FilterEvent(wxEvent& event);
    
    // whm 13Mar2020 added the following bool to prevent CPlaceholder::InsertNullSourcePhrase()
    // from sending a spurious Enter key event to the CPhraseBox::OnKeyUp()
    // whm 20Mar2020 implemented directional insertion of placeholders, so removed the following
    // global from the App.
    //bool b_Spurious_Enter_Tab_Propagated;

    // whm 15Apr2019 added the following m_nDropDownClickedItemIndex member in order to
    // correct index of a clicked on item if a sudden scroll at left-click would cause
    // a wrong index value for the item user intended to click on. It is initialized
    // in OnInit() to -1. It is set by FilterEvent() to the index of the dropdown list
    // item that user initially clicks on. It is interpreted by CPhraseBox::On
    int m_nDropDownClickedItemIndex;

	// BEW 12May16 We need a way to prevent OnIdle() events from asking the user for a KBserver
	// login choice while the wizard is running. OnIdle() will, without this, check only for
	// the flag m_bEnteringKBserverProject being true, and it is defaulted to true in OnInit()
	// so the ask happens at the Next> click at the wizard's Projects page - which is *not* what
	// we want to happen at that time. So we'll add this second boolean to the test, so that
	// the first idle event AFTER the wizard has closed, will trigger the connection request dlg
	bool m_bWizardIsRunning; // it's easier for a non _KBSERVER build to just leave it out of the _KBSERVER wrapper

	// For suppressing repeated timeconsuming function calls in the OnUpdateButtonNullSource() call;
	// when the active pile address changes, reset counter to zero & do a new func call; when it
	// does not change, grab the cached value in m_bIsWithin_Prohib & supply to OnUpdateButtonNullSource()
	int m_entryCount_updateHandler; // augment every time the OnUpdateButtonNullSource() func is called
	bool m_bIsWithin_ProhibSpan; // cache the returned value from IsWithinSpanProhibitingPlaceholderInsertion()
	CPile* m_pCachedActivePile; // compare m_pActivePile with this, if different, call the
								// doc's IsWithinSpanProhibitingPlaceholderInsertion(pSrcPhrase) func

//#if defined(_KBSERVER)

	// BEW 15Jul20 next two used to be local strings in DoDiscoverKBservers(); but now
	// they are app-wide accessible string vars. Makes connecting up Leon's solution
	// to the legacy code easier.
	wxString resultsPath;
	wxString resultsStr;
	wxString discoveryPath; // added this one because it will allow the path to the  <<-- deprecate 
		// kbserver discovery code to be stored - it may temporarily be different
		// from resultsPath or execPath

    // whm 22Feb2021 removed the following as they are no longer needed for determing the
    // path to the dist folder or the executable path. Use the App members m_dataKBsharingPath
    // and m_appInstallPathOnly + PathSeparator. 
	//wxString distPath; // to Leon's dist folder - always a child of the app's folder

	// BEW 19Oct21, dist folder no longer used, and the particular dLss_win.exe or dLss_win10.exe
	// file needs to be moved to the executablePath folder, so that there is no path prefix to
	// it's call. For instance, exacutablePath when working in the IDE with Unicode Debug as the
	// location of the Adapt_It_Unicode.exe executable file, will have the executable in the
	// Unicode Debug folder, which is exactly what we want, for DoDiscoverKBservers() to work
	// corectly
	wxString executablePath; // has app at end (app name needs to be removed)
	wxString execPath; // the "executablePath" with the executable app at end removed
	//wxString GetDistFolder(); // ending in the path separator

	bool m_bDoingChangeFullname; // BEW added, 9Dec20, default FALSE - for KB Sharing Mgr
	bool m_bDoingChangePassword; // BEW added, 11Jan21, default FALSE - for KB Sharing Mgr

	// BEW 18Dec20 Legacy code for KB Sharing Mgr is too convoluted, use a set of arrays.
	// So I'm refactoring to use wxArrayString instances and wxArrayInt for useradmin value
	// declared here, so I can have better control and checking of the code. I've given
	// up sorting the user's list - too hard to support with this new solution.
	wxArrayString m_mgrUsernameArr;
	wxArrayString m_mgrFullnameArr;
	wxArrayString m_mgrPasswordArr;
	wxArrayInt m_mgrUseradminArr;
	// This set of 4 for copies of the above 4, made before the user has a chance to
	// do any mgr changes - use these to make comparisons after changes are done
	wxArrayString m_mgrSavedUsernameArr;
	wxArrayString m_mgrSavedFullnameArr;
	wxArrayString m_mgrSavedPasswordArr;
	wxArrayInt m_mgrSavedUseradminArr;

	void ConvertLinesToMgrArrays(wxArrayString& arrLines); // def'n at .cpp 37,163
	void EmptyMgrArraySet(bool bNormalSet); // def'n at .cpp 37,243
	wxString DumpManagerArray(wxArrayString& arr);
	wxString DumpManagerUseradmins(wxArrayInt& arr);

	// BEW 20Au20 deprecation of the total KBSharingManager is pending, 'ForManager'
	// variables, and anything managerish, will be weeded out. Our simpler system has
	// a user table and an entry table. The calls to mariaDB/kbserver will be fewer,
	// and will drop files as an audit trail (Leon's preference). Replacing 'manager'
	// stuff will be variables, bools, functions, etc with the word 'Foreign' in it.
	// For example,  m_bUseForeignOption (default FALSE) will specify, if TRUE, that
	// 'foreign' users - ie. uses on other machines than one's own, are in focus. Eg...
	// A techie going to the kbserver to enter a bunch of usernames for various people
	// in a translation team, the code would use a .dat file like "add_foreign_user.dat"
	bool m_bUseForeignOption;  // FALSE (default) means "normal local user stuff used,
							   // such as m_strUserId and m_strFullname, etc
							   // TRUE means 'other-user' focus, such as accessing the
							   // kbserver with kbadmin username, kbauth pwd, etc
	//wxString m_strForeignPassword; // store the non-local-user one here
	// BEW 23Nov20 temp string vars for support of adding new users to user table, since
	// it can be done from a menu item in view class, or from settings typed into the
	// relevant fields of the KB Sharing Manager's user page. Adding a new user can
	// only be done by one handler or the other, so lodging the input strings here
	// enables me to have one implementation once these values are set up right.
	// Distinguishing which handler is setting them is m_bUseForeignOption's job.
	wxString m_temp_username;
	wxString m_temp_fullname;
	wxString m_temp_password;
	wxString m_temp_useradmin_flag;

	// BEW 20jUl17, the following boolean, when TRUE, is used to switch to being on,
	// the discovery of one or more KBservers which are able to accept access, in a single session.
	bool m_bDiscoverKBservers;

	// BEW 1Sep20 for our refactored 2-page KB Sharing Manager, we'll store some
	// helpful wxStrings here, which our Manager's code can pick up as needed, or set,
	wxString m_curAuthUsername; // The username used for authenticating for a Manager access
	wxString m_Username2;       // For LookupUser() only, second username - to be looked up
	wxString m_curAuthPassword; // The password that goes with curAuthUsername
	wxString m_curFullname;     // may not need it, but it's here for the'fullname' field's value
								// if we want it.
	bool m_bcurUseradmin;  // TRUE if user table table has m_curAuthUsername's useradmin value = 1, 
						  // FALSE otherwise
	bool m_bcurKbadmin;    // *ALWAYS* TRUE now, and no longer in user table's schema
	wxString m_curIpAddr; // the ipaddr used for authenticating with m_curAuthUsername etc
	wxString m_curSrcLangName; // source language name from current "src to tgt adaptations" str
	wxString m_curTgtLangName; // target .... ditto
	wxString m_curGlossLangName; // if user elects to have glossing enabled
	wxString m_curFreeTransLangName; // user may want to send free trans to collaborating Paratext
									 // in a language different from all the preceding ones

	// BEW 30Mar22 PutPhraseBoxAtDocEnd() is called when closing down a document. The move of the
	// phrasebox involves an internal call of PlacePhraseBox(), and this in turn, if the project
	// is a kbserver one, will result in the box landing and a do_pseudo_delete.exe call being made.
	// The latter would cause whatever was the document-final adaptation (or gloss if in glossing 
	// mode), to pseudo-delete in the entry table of kbserver, whatever is the value. Such a deletion
	// needs to be suppressed, because we are only wanting a save doc closure that changes nothing.
	// So here I add to the app, a boolean which, when TRUE, tells our code for calling 
	// do_pseudo_delete.exe (wherever it may be called at), to be skipped. This boolean is only
	// set TRUE in two places: Doc 2227 [ OnSaveAndCommit() ] and 2829 [ DoFileSave_Protected() ].
	// If the project is not a kbserver one, this boolean will have no effect.
	bool m_bSuppressPseudoDeleteWhenClosingDoc;

	// BEW 2Sep20 public Updaters for the above public variables.
	void UpdateCurAuthUsername(wxString str); 
	void UpdateUsername2(wxString str); // This just for the username being looked up by LookupUser()
	void UpdateCurAuthPassword(wxString str);
	void UpdateCurFullname(wxString str);
	void UpdatebcurUseradmin(bool bUseradmin);
	void UpdatebcurKbadmin(); // always TRUE
	void UpdateIpAddr(wxString str);
	void UpdateCurSrcLangName(wxString str);
	void UpdateCurTgtLangName(wxString str);
	void UpdateCurGlossLangName(wxString str);
	void UpdateCurFreeTransLangName(wxString str);

	// BEW 5Sep20 - a parallel set of vars and updaters for the 'normal'
	// user accesses, i.e. when m_bUserIsAuthenticating is TRUE
	wxString m_curNormalUsername; // The username used for authenticating for a Manager access
	wxString m_curNormalUsername2; // for the "look for this user" box, but may never be used
	wxString m_curNormalPassword; // The password that goes with curAuthUsername
	wxString m_curNormalFullname; // may not need it, but it's here for the'fullname'
								  // field's value if we want it.
	bool m_bcurNormalUseradmin;   // TRUE if user table table has m_curNormalUsername's 
						   // useradmin value = 1, FALSE otherwise
	bool m_bcurNormalKbadmin;    // *ALWAYS* TRUE now, and no longer in user table's schema
	//wxString m_curNormalIpAddr; // BEW 24Sep20 nah, legacy m_strKbServerIpAddr is what we want
	wxString m_curNormalSrcLangName; // source language name from current "src to tgt adaptations" str
	wxString m_curNormalTgtLangName; // target .... ditto
	wxString m_curNormalGlossLangName; // if glossing enabled; but projects defined only by src & tgt
									   // language names; so this may not be important
	wxString m_curNormalFreeTransLangName; // user may want to send free trans to collaborating
								// Paratext in a language different from all the preceding ones

	// BEW 2Sep20 public Updaters for the above public variables.
	void UpdateCurNormalUsername(wxString str);
	void UpdateCurNormalPassword(wxString str);
	void UpdateCurNormalFullname(wxString str);
	void UpdatebcurNormalUseradmin(bool bUseradmin);
	void UpdatebcurNormalKbadmin(); // always TRUE
	void UpdateNormalIpAddr(wxString str);
	void UpdateCurNormalSrcLangName(wxString str);
	void UpdateCurNormalTgtLangName(wxString str);
	void UpdateCurNormalGlossLangName(wxString str);
	void UpdateCurNormalFreeTransLangName(wxString str);

	bool m_bHasUseradminPermission; // governs whether user can access the KB Sharing Manager
	bool m_bKBSharingMgrEntered; // TRUE if user is allowed entry, clear to FALSE when exiting Mgr
	bool m_bWithinMgr; // TRUE when successfully within the KB Sharing Manager where the
					   // possibility exists for user1 (for kbserver authenticating) to
					   // be different than user2 (the  selected user when operating within
					   // the Mgr, if FALSE, m_bKBSharingMgrEntered should be FALSE too
	bool m_bConfigMovedDatFileError; //TRUE for error, FALSE for no error (Sharing Mgr)

	//BEW 10Dec20 next three are for roaming the list of users in the KB Sharing Manager
	// they are public, and I won't bother with Update...() functions
	// By having these, the manager's local set of string variables, etc, can stay local
	// and I achieve access outside the manager to those values provided these three are
	// set from within the manager, as they become known
	wxString m_curSelectedUsername;
	wxString m_curSelectedFullname;
	wxString m_curSelectedUseradmin;

	wxArrayString m_arrLines; // For use by the ListUsers() call - which we want the Sharing
							  // Manager to be able to access, as well as AI's CallExecute()

	// BEW 14Nov20 cloned this from KbServer.cpp - I need it in CallExecute() for ListUsers() support
	bool DatFile2StringArray(wxString& execPath, wxString& resultFile, wxArrayString& arrLines);

	// BEW 20Jul20 added, for scanning for active mariaDB-service (mysql)
//	Ctest_system_call* pCtest_system_call = Cnew test_system_call();

	// The following is the timer for incremental downloads; defaulted to
	// 5 minutes, but settable by the user to other values in the range 1-120 minutes,
	// and the minutes value will be stored in the project config file
	Timer_KbServerChangedSince* m_pKbServerDownloadTimer; // for periodic incremental
											// download of entries from the KBserver
	// If ::wxExecute() fails, we want the commandLine it used to be in LogUserAction()
	wxString m_curCommandLine;

	// OnIdle() will be used for initiating a download of the incremental type.
	// It will happen only after a boolean flag goes TRUE; the flag is the following
	bool m_bKbServerIncrementalDownloadPending;

	// Periodicity for the new entry downloads (in minutes; but for use with the
	// timer, multiply by 1000*60 since the timer's units are milliseconds)
	int  m_nKbServerIncrementalDownloadInterval;

	// Storage of username's value for the boolean flags, kbadmin, and useradmin; we store
	// them here rather than in the KbServer class itself, because the value of these
	// flags need to be known before either of the adapting or glossing KbServer classes
    // are instantiated (i.e. when checking if the user is in user table, and if the user
    // is authorized to create an entry in the kb table). These two flags implement the
    // non-basic privilege levels for users of the KBserver. As of Aug 2020, m_kbserver_kbadmin
	// is always TRUE, allowing anyone to make a new KB, even if they cannot add a new user.

	bool m_kbserver_kbadmin;   // initialize to always TRUE in OnInit()
	bool m_kbserver_useradmin; // initialize to default FALSE in OnInit()

	// BEW 18Aug20 This function takes an int value, and uses an internal switch to call
	// the appropriate function which configures a .DAT file, locating it in the app's
	// folder, and populating it with the comma separated commandLine for that .DAT file
	bool ConfigureDATfile(const int funcNumber);
	bool m_bKBEditorEntered; // TRUE if View.cpp OnToolKbEditor(unused wxCommandEvent event) is invoked
							 // FALSE when the handler is exited. Used for invoking the speedier
							 // CreateEntry function, for enum value create_entry (=4) when user
							 // is NOT in the KB Editor tabbed dialog
	// BEW 18Aug20 next three calls are used in sequence to (a) delete the old
	// .dat file from the execPath, (b) move the 'blank' .dat file up from the _DATA_KB_SHARING folder
	// to the executable's folder, the execPath folder, and (c) configure the moved  .dat boilerplate
	// internal lines by shortening the contents to have a descriptive line followed by the
	// filled out commandLine - with the appropriate values for the final .exe file to have
	void DeleteOldDATfile(wxString filename, wxString execFolderPath);
	void MoveBlankDatFile(wxString filename, wxString dataFolderPath, wxString execFolderPath);
	void ConfigureMovedDatFile(const int funcNumber, wxString& filename, wxString& execFolderPath);
	bool CallExecute(const int funcNumber, wxString execFileName, wxString execPath,
			wxString resultFile, int waitSuccess, int waitFailure, bool bReportResult = FALSE);
	wxString m_resultDatFileName;  // scratch variable for getting filename from ConfigureDATfile()
								   // into CallExecute() function
	void RemoveEmptyInitialLine(const int funcNumber, wxString execPath, wxString resultFile);  // BEW 26Mar22
	// BEW created next function 15Nov21 to avoid a link error when the executable is being linked, 
	// ("System Error ... unable to find libmariadb.dll file") because in the folder where the object files
	// are (e.g. when developing for Unicode Debug build, it's the folder ...\bin\win32\Unicode Debug\ )
	// because, Leon said, on 64 bit machines, 32 bit compilation is an emulation process, and that can be
	// dodgy - and our problem (even though we don't use a .dll) was that the libmariadb.dll was being 
	// searched for for linking, and it was not "there". He said a "clean Adapt It" is sometimes not
	// finding an external resource needed for linking, and so we need to provide it where the object
	// files happen to be. When I manually added libmariadb.dll to Unicode Debug folder, then linkage
	// worked right. Leon found the same behaviour in his testing too. So the next function is for
	// grabbing the .dll file from the installed C:\Program Files (x86)\MariaDB 10.5\lib\ folder, and
	// plonking it into ...adaptit-git\adaptit\bin\win32\Unicode Debug\ folder, or the Release one
	// if building the release build. Then it's 'seen' by the linker. Run this function from OnInit()
	//bool MoveMariaDLLtoOBJfolder(wxString& objFolder); <<-- No, it has to happen at compile & link, not at OnInit()

	// The following ones are for use in adapting - using the refactored KbServer class's signatures' values
	// These are not permanently stored, except in the entry table of kbserver; in AI they are scratch variables
	wxString m_curNormalSource;   // CSourcePhrase's m_key value
	wxString m_curNormalTarget;   // CSourcePhrase's m_adaption value
	wxString m_curNormalGloss;    // CSourcePhrase's m_gloss value, when in glossing mode
	wxString m_curExecPath; // transferred from ConfigureMovedDatFile() so that CallExecute() can pick it up

	// variables for speeding up the adapting/glossing workflow, by keeping count
	// and reusing the .dat file without deletion but by field value substitution
	int m_nCreateAdaptionCount; // if 0, move .dat up, clear, and fill with values; if >0 keep & refill fields
	int m_nCreateGlossCount;    // if 0, move .dat up, clear, and fill with values; if >0 keep & refill fields
	int m_nPseudoDeleteAdaptionCount; // as above
	int m_nPseudoDeleteGlossCount;    // as above
	int m_nPseudoUndeleteAdaptionCount; // as above
	int m_nPseudoUndeleteGlossCount;    // as above, etc
	//int m_nChangeQueuedAdaptionCount; // dont need these, ChangedSince_Queued() is no longer needed
	//int m_nChangeQueuedGlossCount;
	int m_nLookupEntryAdaptationCount;
	int m_nLookupEntryGlossCount;
	int m_nChangedSinceTypedGlossCount;
	int m_nChangedSinceTypedAdaptationCount;

	// Next ones are independent of the adaptation versus glossing choice
	// not speed critical, so make these always 0 (zero), to use move up way
	int m_nAddUsersCount;
	int m_nLookupUserCount;
	int m_nListUsersCount;

	// BEW 18Apr16 CServiceDiscovery*  m_pServDisc;    // The top level class which manages the service discovery module

	wxArrayString		m_theIpAddrs;      // lines of form <ipAddress> from CServiceDiscovery::m_ipAddrsArr
	wxArrayString		m_theHostnames; // parallel array of hostnames for each url in m_urlsArr
	wxArrayString		m_ipAddrs_Hostnames; // for storage of each string <ipaddress>@@@<hostname>

	wxString			m_SD_Message_For_Connect; // set near start of OnInit()
	wxString			m_SD_Message_For_Discovery; // ditto
	bool				m_bShownFromServiceDiscoveryAttempt; // TRUE if dialog is shown from
							// Discover KBservers; FALSE if shown from ConnectUsingServiceDiscoverResults()
							// The boolean governs which message is displayed in the dialog
							// and consequently what the Cancel button does
	ServDiscDetail			m_sd_Detail;
	ServDiscInitialDetail	m_sd_InitialDetail;
	bool m_bUserDecisionMadeAtDiscovery; // TRUE if at the OK click there was a user
		// selection current, or if he clicked Cancel button. If no selection,
		// then FALSE
	// The next two are used to store the ipAddress and (host)name of a discovered KBserver
	// from a call of Discover KBservers
	wxString m_chosenIpAddr;
	wxString m_chosenHostname;

	// BEW 25Jul20 deprecated comment - wxServeDisc (with zeroconf use) is removed from app
	// NOTE - IMPORTANT. The service discovery code, at the top levels, is copiously
	// commented and there are many wxLogDebug() calls. Timing annotations in the debugger
	// window and those logging messages are VITAL for understanding how the module works,
	// and when things go wrong, what might be causing the error. DO NOT DELETE THE
	// wxLogDebug() calls from the code in the class wxServDisc !! They can be turned off
	// by commenting out a #define near the top of the .cpp file


	// for support of service discovery
	wxString		m_saveOldIpAddrStr;
	wxString		m_saveOldHostnameStr;
	wxString		m_saveOldUsernameStr;
	wxString		m_savePassword;
	bool			m_saveSharingAdaptationsFlag;
	bool			m_saveSharingGlossesFlag;
	CWaitDlg*		m_pWaitDlg; // for feedback messages, connection succeeded, or, sharing is OFF
						// Keep m_pWaitDlg NULL except when a message is up, so that OnIdle() can
						// destroy the dlg message when the wait timespan has expired (1.3 secs)

						// The timespan value is in AdaptitConstants.h - two #defines
	wxDateTime		m_msgShownTime; // set by a call to wxDateTime::Now()
	wxString		mariadb_path; // set to C:\Program Files (x86)\MariaDB 10.5\  at end of OnInit()

//#endif // _KBSERVER

	wxUint32 maxProgDialogValue; // whm added 25Aug11 as a temporary measure until we can
							// sub-class the wxProgressDialog which currently has no
							// way to get its maximum value held as a private member.
							// It is set to MAXINT in OnInit()

	bool m_bAdminMoveOrCopyIsInitializing;

    /// The application's m_pDocManager member is one of the main players in the
    /// document-view framework as implemented in wxWidgets. It is created in OnInit() and
    /// mostly it takes care of itself. It is
    /// also referenced in the creation of our pDocTemplate and m_pMainFrame which
    /// participate in doc-view matters. We force our application to be of the "Single
    /// Document Interface (SDI)" type by calling the doc manager's SetMaxDocsOpen(1)
    /// method.
    wxDocManager* m_pDocManager; // for managing the doc/view // made doc manager public
								 // so wxGetApp() can access it in the View

    /// The application's m_pConfig member is used to store and retrieve certain settings
    /// and/or information that needs to persist between application sessions. Since it is
    /// cross-platform it is clever enough to use the Registry on Windows, but uses hidden
    /// plain text configuration files on Linux and the Mac. We could use it to keep track
    /// of most of the basic config file settings, but for the sake of compatibility with
    /// the MFC version we only use it to store the following things: (1) the user
    /// interface language of choice, (2) the recent files used list (file history), and
	/// (3) the Html Help Controller window's size, position, fonts, etc. (4) Paratext
	/// collaboration parameters, and when implemented, also Bibledit collaboration
	/// parameters.
	wxFileConfig* m_pConfig;

    /// whm 13Jul2018 TODO: Bruce should check to see if the m_nCacheLeavingLocation
    /// and m_nOnLButtonDownEntranceCount hacks are still necessary with the new phrasebox.
	/// BEW 26Jul18, removed unnecessary entrance count, retained the cache integer - it's needed

	/// Cache the sequence number of the location at which the phrasebox lands, so that
	/// when the next click to jump to some other location (or to the same location)
	/// the cached sequ num value is used to get at the correct pile, and hence the correct
	/// pSrcPhrase at the 'leaving' location, in order that the GUI shows correct strings, and the
	/// KB gets the correct entry added, and the old pile's sequence number worked out, and
	/// from that the old pile reset, and from that ResetPartnerPileWidth() called to
	// make invalid the location at which the user action of leaving for another location
	// happened, etc. (Comment updated, BEW 26Jul18)
	int m_nCacheLeavingLocation; // -1 (wxNOT_FOUND) when not set, set in OnLButtonDown()

    /// The application's m_pParser member can be used to process command line arguments.
    /// The command line processing in the MFC version was implemented but did not work
    /// correctly. Although available for future use, it has not been implemented fully in
    /// the wxWidgets version.
	wxCmdLineParser* m_pParser;
	//wxCmdLineParser* m_pParser2; // BEW added 12Nov09 for parsing export command (Hatton,McEvoy)
	wxString m_autoexport_command;
	wxString m_autoexport_projectname;
	wxString m_autoexport_docname;
	wxString m_autoexport_outputpath;

    /// The application's array of 4 m_pConsistentChanger members are created on demand in
    /// OnToolsDefineCC(). They are used load and process consistent change tables in the
    /// application.
	CConsistentChanger* m_pConsistentChanger[4]; // all four initialized to NULL in Adapt_It.cpp

    /// The list of directory paths that should be searched for Adapt It files (XML
    /// configuration files, localization files, help files, etc.). On Linux this includes
    /// the directory where Adapt It was installed, plus the current user's .audacity-files
    /// directory. Additional directories can be specified using the AUDACITY_PATH
    /// environment variable. On Windows or Mac OS, this will include the directory which
    /// contains the Audacity program.
	wxArrayString adaptitPathList; // TODO: Implement this!

	// a string used for restoring the appropriate phrase box contents after filtering or
	// unfiltering changes have been made to the document. Enables the source text at a
	// possibly new active location in the document determined within the
	// GetSafePhraseBoxLocationUsingList() function to be communicated to a test in the
	// DoUsfmFilterChanges() function
	wxString m_strFiltering_SrcText_AtNewLocation;

	// When doing Find Next, and the matched text overlaps or is within a retranslation,
	// set this boolean TRUE so that the phrase box's pile can have its source text
	// unhighlighted because the retranslation text will have the selection instead, and
	// otherwise if the user Cancels the Find Next and wants to edit the retranslation,
	// he'd have to destroy the selection and highlight and reselect. This flag is FALSE
	// in all other circumstances; but the TRUE value is significant to a number of
	// functions which must treat matches within a retranslation as a match within a
	// "unit" of text but the unit is internally complex (ie. a sequence of words, not a
	// single word), such as DoExtendedSearch(), etc
	// BEW 30Mar21, refactoring the Find... for a Special Search for retranslations, this
	// variable was not getting set TRUE, and needs to be. Set TRUE in 
	bool m_bMatchedRetranslation;
	// support for read-only protection
	ReadOnlyProtection* m_pROP;

	// BEW added 23Nov12, for support of Cancel All Steps button in Vertical Edit mode
	bool m_bCalledFromOnVerticalEditCancelAllSteps; // default is FALSE, initialized in OnInit()
	// BEW 4May18, a helper for forcing the first CSourcePhrase in a adaptationStep or 
	// glossingStep of VerticalEdit mode to retain the user's typed-in adaptation, or
	// gloss, when the phrasebox is moved forward by an Enter key press (this issue has
	// plagued the VerticalEdit feature for many years, hopefully now we'll have a robust fix)
	bool m_bVertEdit_AtFirst; // TRUE (set from custom event handler) when a Step is just
							  // entered, FALSE when phrasebox moves on in the step. Used to
							  // limit the fix to just the first CSourcePhrase in the span

	ToolbarButtonSize m_toolbarSize;
	bool m_bShowToolbarIconAndText; // default is FALSE
	bool m_bToolbarButtons[50];		// which buttons to display on the toolbar
public:
	wxString RemovePathPrefix(wxString cmdLine, wxString& pathPrefix);

private:

	// whm: This enum is made private because its enumerations should not
	// be accessed directly, but through the public
	// GetTopLevelMenuName(TopLevelMenu topLevelMenu) function which
	// queries the defaultTopLevelMenuNames[] array of default menu names.
	enum TopLevelMenu
	{
		fileMenu,
		editMenu,
		viewMenu,
		toolsMenu,
		exportImportMenu,
		advancedMenu,
		layoutMenu,
		helpMenu,
		administratorMenu
	};

public: // BEW 3Dec14 made these public because GuesserUdate in KB.cpp needs to access them
	CGuesserAffixArray	m_GuesserPrefixArray; // list of input prefixes to improve guesser
	CGuesserAffixArray	m_GuesserSuffixArray; // list of input suffixes to improve guesser
private:
	/// These variables signal that the prefix and suffix files for the guesser have or
	///     have not been loaded yet.
	/// Initially they will only be loaded at startup. These are set to false in
	///     CAdapt_ItApp::OnInit() and set to true in
	///     CAdapt_ItApp::LoadGuesser(CKB* m_pKB) (in version 1)
	/// KLB 09/2013
	bool GuesserPrefixesLoaded;
	bool GuesserSuffixesLoaded;
	/// BEW 3Dec14 made these public, because GuesserUpdate() in KB.cpp needs to access them
public:
	bool GuesserPrefixCorrespondencesLoaded;
	bool GuesserSuffixCorrespondencesLoaded;
	void ClobberGuesser(); // call to make sure nothing can transfer over to a different project, and in OnExit()
private:

    /// The application's m_pMainFrame member serves as the backbone for Adapt It's
    /// interface and its document-view framework. It is created in the App's OnInit()
    /// function and is the "parent" window for almost all other parts of Adapt It's
    /// interface including the menuBar, toolBar, controlBar, composeBar, statusBar and
    /// Adapt It's scrolling main client window.
	CMainFrame* m_pMainFrame;
    DECLARE_EVENT_TABLE(); // MFC uses DECLARE_MESSAGE_MAP()

// MFC version code below
public:

	// If the current instance of AI is the first running instance m_pServer is created.
	// Then m_pServer will listen for other AI instances that may be started. Any other
	// instance that starts up will just ask the current instance (through a connection
	// with m_pServer) to just raise the current instance's main frame.
	AI_Server* m_pServer;

    /// This holds the platform specific end-of-line character string for external text
    /// files. On Windows this is \r\n; on Linux it is \n; on Macintosh it is \r. The
    /// appropriate end-of-line character sequence(s) is stored in m_eolStr by a call to
    /// wxTextFile::GetEOL() in the App's OnInit() function.
	wxString m_eolStr;

	// The next boolean is in support of Collaboration with Paratext or Bibledit. When the
	// GetSourceTextFromEditor::OnOK() call returns, a lookup of the KB is safe because if
	// at that time a Choose Translation dialog is put up, OnOK() will have finished. So
	// we set this boolean at the end of that OnOK(), and then the internal call of
	// OpenDocWithMerger() which calls PlacePhraseBox() (and the latter does lookup of the
	// KB) can be wrapped with a test for this boolean TRUE and if so, the call to
	// PlacePhraseBox is not made then. Instead, OnIdle() will look for this flag with a
	// TRUE value, and when that is the case, it will make the appropriate
	// PlacePhraseBox() call, with selector set to 2 ("don't do first block, do do second
	// block" - see PlacePhraseBox description for explanation), and passing in a pCell
	// value calculated from the current active pile. Then OnIdle() clears the boolean to
	// FALSE so that this suppression is not done again unless a document is being set up
	// in collaboration mode again
	bool bDelay_PlacePhraseBox_Call_Until_Next_OnIdle;

    /// The m_strNR string originally held the string "NR " for signifying what was called
    /// the "Non-Roman" version of Adapt It. For version 2.0.6 and onwards, m_strNR holds
    /// _T("Unicode ") because the unicode version is now called "Adapt It Unicode"; and
    /// the old _T("NR ") string is now in m_strOldNR, and this latter one was used within
    /// the function RenameNRtoUnicode( ) which searched for the old "Adapt It NR Work"
    /// folder and if it found it, it silently changed the name permanently to "Adapt It
    /// Unicode Work". This function is not needed or used in the wx version. The only use
    /// for m_strNR now is for using in the wxString::Format() method to add the word
    /// "Unicode " to compose the name of the Unicode version's work folder "Adapt It
    /// Unicode Work".
	wxString m_strNR;

    /// The title of the application which is written in the title bar of Adapt It's main
    /// frame, and also serves as the doc template name.
	wxString m_FrameAndDocProgramTitle; // title for main frame and doc template name

	// BEW 13Jun19 need a flag which, when TRUE, indicates that the application when
	// processing OnInit(), is currently doing a first launch which is creating the
	// Adapt It Unicode Work folder, at the standard path location, also m_workFolderPath,
	// and current working directory, as determined internally by EnsureWorkFolderPresent()
	// in OnInit(). This is so that we can limit initial setting up of the frame window
	// to the one OnInit() which gets the work folder created.
	bool m_bWorkFolderBeingSetUp;  // default FALSE, set TRUE only in EnsureWorkFolderPresent()

	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // Variable declarations below were moved here from the Doc's public area because the
    // wxWidgets doc/view framework deletes the Doc and recreates it afresh (calling the
    // Doc's constructor)

    /// The m_saveList member is a SPList which serves as a place to save m_pSourcePhrases
    /// when printing a selection or a ch/verse range. It is used in the View's
    /// SetupRangePrintOp() and in AIPrintout's OnPreparePrinting() and in its destructor.
	SPList m_saveList;

    /// A pointer to the m_saveList member which serves as a place to save m_pSourcePhrases
    /// when printing a selection or a ch/verse range. It is used in the View's
    /// SetupRangePrintOp() and in AIPrintout's OnPreparePrinting() and in its destructor,
    /// and, for the __WXGTK__ build, in the workaround for making a user's page range
    /// printing choice succeed (wxGnomePrinter fails to support it) - where we use a
    /// workaround function called SetupPagePrintOp().
	SPList* m_pSaveList;

    /// The m_curOutputFilename member holds the name of a file currently functioning as an
    /// output file. It generally only includes the file name and not the path (which is
    /// stored in m_curOutputPath).
	wxString m_curOutputFilename;

    /// The m_curOutputPath member holds the path of the file currently functioining as an
    /// output file. It generally only represents the path and not the file name itself
    /// (which is stored in m_curOutputFilename).
	wxString m_curOutputPath;

    /// The m_curOutputBackupFilename member holds the name of a file currently functioning
    /// as a backup file. The name of the backup file is of the form *.BAK for the wx
    /// version which only uses the xml files and not binary files.
	wxString m_curOutputBackupFilename; // BEW added 15Aug05 to get a consistent file
										// and path naming protocol

    /// The m_nInputFileLength member represented in the MFC version the serializable # of
    /// chars in untokenized source text (including terminating null). MFC used DWORD.
	wxUint32 m_nInputFileLength;

	// !!! Doxygen commenting up to here!!!
	wxString	m_punctuationSet; // retained in order to keep backwards compatibility in the
								  // doc serialization (with no content), otherwise unused

	// store doc size parameters
	wxSize		m_docSize;	// stores the virtual doc size (in pixels)
	wxSize		m_saveDocSize; // used for restoring original doc size across a printing
							   // operation
	int			m_nAIPrintout_Destructor_ReentrancyCount; // ~AIPrintout() is entered
                // twice in closing off a Print Preview and parameter cleanup done there is
                // fouling things, so I'll count times reentered and have the function exit
                // immediately if the count gets over 1; the count is initialized to 1 in
                // the AIPrintout() constructor, and in a few other places
    bool		m_bSuppressFreeTransRestoreAfterPrint; // BEW added 18Nov13, so that the last
					// code block in RecalcLayout() which sets up the active location's free
					// translation, won't try to do so at the end of printing cleanup in
					// DoPrintCleanup(). The RecalcLayout() call there should have this set
					// TRUE beforehand, and default FALSE restored after it returns
	SPList*		m_pSourcePhrases;
	wxString*	m_pBuffer;  // in legacy versions was used to store source (untokenized)
                // text data on the heap; and still does so while parsing source text, but
                // from late versions 2 the source text was not kept there, not output in
                // the document, and from version 3 the buffer is also used for composing
                // settings information in the preparation of the XML form of the Adapt It
                // document
	wxString	buffer;
	wxString	m_curChapter;

	MapWholeMkrToFilterStatus m_FilterStatusMap;// entries are a wholeMarker (ie. with
                // backslash) as key with either a "0" or "1" string as its associated
                // value
    // Note: CFilterPage maintains a temporary set of the following variables for
    // displaying the GUI state of unknown markers. When the user accepts any changes to
    // the filtering state of unknown markers, these variables on the app are updated.
    wxString m_poetryMkrs; // USFM ones
	wxArrayString m_unknownMarkers; // array of unknown whole markers -
									// no end markers stored here

	// whm added these two string arrays 8Jul12 which are initialized in Adapt_It.cpp::OnInit()
	wxArrayString m_crossRefMarkerSet;
	wxArrayString m_footnoteMarkerSet;

	wxArrayInt	m_filterFlagsUnkMkrs;
	wxString	m_currentUnknownMarkersStr;	// whm added 31May05. Stores the current
                // active list of any unknown whole markers in a = delimited string (that
                // is, = immediately follows each marker (then 0 or 1). This string is
                // stored in the doc's Buffer member (5th field).
	// next three added by BEW, 10June05 in support of Reconstituting the document after
	// user changes the SFM set
	wxString	m_filterMarkersBeforeEdit;	// contents of gCurrentFilterMarkers is copied
                // to here when the user invokes the Preferences... command; this material
                // is used only if an SFM set change is requested
	wxString	m_secondPassFilterMarkers;	// the set of filter markers, with any common
                // ones with what is in m_saveOriginalSfmSetFilterMarkers which are to
                // remain filtered, removed
	enum SfmSet	m_sfmSetBeforeEdit;	// the gCurrentSfmSet value is copied here when the
                // user invokes the Preferences... command; this is used subsequently only
                // if an SFM set change is requested
	enum SfmSet m_sfmSetAfterEdit; // we use this after the unfiltering pass for the old
                // SFM set when doing doc rebuild, in order to restore the gCurrentSfmSet
                // value to what is current
	wxString	m_filterMarkersAfterEdit;	//whm added 10Jun05 for Bruce
	bool		m_bWantSourcePhrasesOnly;	// Added by JF.  This is used to allow
                // load/save operations to be performed _without_ affecting any globals or
                // the current view. This is used during the join operation to load
                // multiple files into memory (one at a time) without closing the current
                // file. This allows us to simply append the source phrases from each file
                // to the current document, rather than having to reconstruct or reload the
                // was-current document after reading the other documents for the join
                // operation. (If TRUE, document loading does nothing except set up the
                // list of CSourcePhrase instances.)

	// whm 23Aug2021 added the following for AutoCorrect feature support
	// The following hash map is used to map typed sub-strings of target text to 
	// replace matching sub-strings with their "corrected" counterpart using the 
	// replacement rules stored in any existing autocorrect.txt file found in the 
	// associated project folder..
	// This is designed to be equivalent to the same Auto Correct feature that is 
	// available in Paratext 8 and 9. In fact, when an admin/user sets up AI=PT
	// collaboration using the SetupEditorCollaboration dialog and chooses a particular
	// Paratext project for AI's target text, the dialog will notify the admin/user
	// of the presence of an autocorrect.txt file in the Paratext project's folder,
	// and offer to copy it over for use in AI - if an autocorrect.txt file doesn't
	// already exist (or is older) in the AI project that utilizes that Paratext project
	// as its target text language.
	MapAutoCorrectStrings m_AutoCorrectMap;
	bool		m_bUsingAutoCorrect; // whm 23Aug2021 added
	bool		m_bAutoCorrectIsMalformed; // whm 23Aug2021 added
	int			m_longestAutoCorrectKeyLen; // whm 23Aug2021 added
	bool		AutoCorrected(wxTextCtrl* pTextCtrl, wxKeyEvent* pevent); // whm 31Aug2021 added
	bool		AreTextFilesTheSame(wxString filePath1, wxString filePath2); // whm 2Sept2021 added
	void		EmptyMapAndInitializeAutoCorrect(); // whm 3Sept2021 added
	wxString	ConvertBackslashUxxxxHexValsInStringToStringChars(wxString stringWithHexChars, bool& bSuccess); // whm 4Sep2021 added

	// mrh 2012-04-17 The current user of AI, introduced for version control.  This will
	//  probably be the user name + machine name.  "****" means "no owner, up for grabs".
	//  We only assign this string if version control is enabled.  If a different user
	//  opens the document, it will come up read-only.

#define  NOOWNER	_T("****")			// a real user can't have asterisks
#define  NOCODE		_T("qqq")			// no language code defined.  qaa - qtz are defined in ISO 639-2
										//  as being for private use, so thany in this range are 'custom'

	wxString	m_AIuser;				// the currently logged-in user - e.g. joe bloggs@joesMachine. 
										// Used for setting m_owner in XML.cpp near 3466
    wxString	m_strUserID;            // for the unique username for the kb server and DVCS, typically
                                        // the user's email address if he has one, if not, any
                                        // unique string will do provided the server administrator
                                        // approves it
    wxString    m_strFullname;          // needed by git (DVCS), as well as "email address" for which
										// we'll use m_strUserID; kbserver will also use
										// this as an "informal human-readable username" 'fullname' in user table
	// BEW 27Jan22 added the next four archiving strings, for preserving MariaDB access/login credentials,
	// set uniquely for whatever user is accessing the database. These will be used in the .dat input file
	// called: add_foreign_users.dat  - which has the ipAddress first, and then the m_strUserID_Archived value,
	// and that followed by the m_strPassword_Archived - and other values will follow those three. See
	// AI.cpp's ConfigureMovedDatFile() for code using these. The root pwd for kbserver access will also
	// be stored in m_rootPassword_Archived. Under no circumstances allow any of these values to find their
	// way into an AI configuration file, the .ini file, or into KBsharing documentation files
	wxString m_strUserID_Archived;
	wxString m_strFullname_Archived;
	wxString m_strPassword_Archived;
	wxString m_rootPassword_Archived;
	// When a new user is added to the user table, successfully, make the new username, fullname, password
	// and useradmin value (1 or 0) get stored in the next 4 members. Then we can test for a ChangeUsername()
	// which changes the username, matching what's in m_justAddedUsername provided m_justAddedPermission is '1'
	// and if there's a match, we can switch away from kbadmin/kbauth to the newly added user
	wxString m_justAddedUsername;
	wxString m_justAddedFullname;
	wxString m_justAddedPassword;
	wxChar   m_justAddedPermission; // '1' or '0'
	// Two new archiving storage strings, for funcNumber = 3, etc, for ListUsers() and do_list_users.exe
	// which are for logging in (if credentialed) to the Knowledge Base Sharing Manager. These are
	// used for authenticating at login, and for credentialing a Add User button press. 
	// do_list_users.exe will read the input .dat file, list_users.dat - and that will require
	// additional parameters which are available from storage already in the app from years ago
	// NOTE. since any of the 12 or so do....exe functions can only be called one at a time, all
	// of them except a few with low funcNumber values, can use these two - they act as scratch
	// strings, to take params from a function like ListUsers() to it's Leon-designed .exe 
	// function, such as list_users.dat from which the commandLine is formed. Hence, for pseudo delete, etc....
	// The 3rd authenticating member is the ipAddress, which is easily got from m_chosenIpAddr
	wxString m_DB_username;
	wxString m_DB_password;
	// BEW 10Feb22, Need a do_lookup_user.exe which for a given username in the user table, returns the
	//  values for:
	// (a) that username, (b) it's fullname, and (c) the associated useradmin flag value (1 or 0)
	// do_list_users.exe is for getting the KB Sharing Manager's user table values shown, but this
	// is only permitted by Adapt It provided the authenticating user (m_strUserID) is permitted to
	// access the Manager dialog. The only reliable way to determine this is to lookup up the relevant
	// row's useradmin value. So might as well get username and fullname as well, which is what LookupUser()
	// used to do.
	wxString m_server_username_looked_up; 
	wxString m_server_fullname_looked_up; 
	wxChar   m_server_useradmin_looked_up;  // '1' or '0'

	// Version control variables, relating to the current document
	int			m_commitCount;			// Counts commits done on this file.  At present just used to check
										//  if the file is actually under version control.
										//   -1 = not under version control
										//    0 = under VC, but no commits done yet
                                        //    n = n commits have been done
	int			m_trialVersionNum;		// non-negative if we're trialling a look at an earlier version.  Negative means no trial.
    int         m_versionCount;         // total number of versions in the log (applies to current trial)
    bool        m_bBackedUpForTrial;    // true if a backup has been created over a trial

	wxDateTime	m_versionDate;			// when this version was committed
	wxString	m_owner;				// owner of this document, in the same format as m_AIuser.
										// m_owner and m_strUserID must match before a commit is allowed,
										// unless either is "no owner".
//  bool        m_saved_with_commit;    // true if last save also did a commit (to avoid a possible redundant commit) -- now superseded by calling git diff
    bool        m_DVCS_installed;       // true if our DVCS engine (git) is actually installed
    bool        m_recovery_pending;     // true if we hit an error reading a document, but it's under version control so
                                        //  we'll be trying to restore the latest revision
    bool        m_reopen_recovered_doc; // we call the recovery code from 3 different places, and there's only one where we try to reopen the doc
    wxString    m_sourcePath;           // used only when recovering a doc in another project folder
    bool        m_suppress_KB_messages; // set TRUE when we're importing a project for glossing, when a summary message is OK

	DVCS*		m_pDVCS;				// the one and only DVCS object, giving access to the DVCS operations
    DVCSNavDlg* m_pDVCSNavDlg;          // the dialog for navigating over previous versions of the doc

    wxArrayString m_DVCS_log;           // copy of the log returned from git, so our log dialog can get at it


	/////////////////////////////////////////////////////////////////////////////////
    // Variable declarations moved here from the View because the wxWidgets doc/view
    // framework deletes the View and recreates it afresh (calling the View's constructor)

	bool m_bECConnected; // whm added for wx version
	bool bECDriverDLLLoaded; // set TRUE or FALSE in OnInit()
	CFindDlg* m_pFindDlg; // whm added to partly replace original
						  // m_pFindReplaceDlg (non-modal)
	CReplaceDlg* m_pReplaceDlg; // whm added to partly replace original
								// m_pFindReplaceDlg (non-modal)
	CViewFilteredMaterialDlg* m_pViewFilteredMaterialDlg; // non-modal
	CEarlierTranslationDlg* m_pEarlierTransDlg;

	CHtmlFileViewer* m_pHtmlFileViewer; // whm 14Sep11 added

	extendSelDir m_curDirection; // current seln direction, either 'right' or 'left'
	wxPoint m_mouse;			 // last mouse position (log coords) during drag selection
								 // for use in tokenizing retranslations
	static bool bLookAheadMerge; // TRUE when merging a matched multiword phrase
	CPile* m_pActivePile;	// where the phrase box is to be located
	int m_nActiveSequNum;	// sequence number of the srcPhrase at the active
							// pile location

    // For "The HACK", BEW 8Aug13, trying to diagnose & fix a rare m_targetStr value 'non
    // stick' bug after edited phrase box value was edited; observed first by RossJones on
    // Win7, and then by Bill and JerryPfaff on Linux, me a few times Win7 and not at all
    // on Linux. The hack is a block at the end of OnIdle(), where if (limiter == 0) is tested
	//int limiter; // bug fixed 24Sept13 BEW

	// for selection (other parameters are also involved besides this one)
	CCellList	m_selection; // list of selected CCell instances
	CCell* m_pAnchor;		// anchor element for the selection
	int m_selectionLine;	// index of line which has the selection (0 to 1, -1
							// if none) Selections typically have line index 0, the
							// circumstances where line with index 1 (target text) is
							// used as the selection are rare, perhaps not in WX version
	// the next 3 for saving a selection so it can be copied, restored and cleared with utility functions
	// SaveSelection(), RestoreSelection() and ClearSavedSelection() - we save using different
	// params, which allow a selection to be set up again correctly even after a recalc of the layout
	// (all 3 have value -1 when no selection is saved)
	int m_savedSelectionLine; // which cell of the piles has the selection
	int m_savedSelectionAnchorIndex; // the sequence number for the pile where saved selection starts
	int m_savedSelectionCount; // how many consecutive CCell (or CPile) instances are in the selection


	bool m_bSelectByArrowKey; // TRUE when user is using ALT + arrow key to extend sel'n
                //next two cannot be removed for refactored layout, because they are needed
                //for backwards compatibility of the config files; retain them, but make no
                //use of them except to put TRUE values in the config files - initialize
                //those in the app's OnInit()
	bool m_bSuppressLast;	// deprecated: (suppress last target text line of display when TRUE)
	bool m_bSuppressFirst;	// deprecated: (suppress first source text line of display when TRUE)
	bool m_bDrafting; // suggested by Bill Martin: a Drafting versus Review mode choice;
                // with the single-step versus automatic insertion choices possible only in
                // drafting mode, while in review mode (m_bDrafting == FALSE) hitting the
                // RETURN key takes user to the immediate next source phrase, whether empty
                // or not, and the Automatic checkbox is disabled until Drafting mode is
                // chosen again. (This review mode is needed so users can review and edit
                // their work, without the phrase box jumping miles ahead trying to find an
                // empty source phrase every time the user finishes editing something &
                // hits RETURN) This flag's functionality is the same for adapting and for
                // glossing.
	bool m_bForce_Review_Mode; // added by BEW, 23Oct09, to support Bob Eaton's wish for
				// shell opening to add a frm switch to have the document opened be opened
				// in a launched app where review mode radio button is obligatory on, and
				// the drafting & review mode radio buttons are hidden so the user can only
				// do his back translating where no lookup is done, but the KB still gets
				// populated by his back translation choices
	bool m_bSingleStep; // when FALSE, application looks ahead to try find a match for
                // source text in one of the 10 knowledge base maps and does not stop to
                // ask the user provided there is only a single match; when TRUE, every
                // match is shown and the app waits for a RETURN key press before
                // proceeding. When glossing is ON, lookahead will be constrained to the
                // next sourcephrase instance only, but otherwise the functionality is the
                // same as when adapting.
	bool m_bAcceptDefaults; // enabled only when its not single step, cons.changes are
                // on, and copy source is on. If TRUE, auto-inserting continues normally
                // until lookup fails to make a match, and then the source word is grabbed
                // and after the source word has been put through the cc table(s), then
                // m_bAutoInsert is turned back on automatically (no user intervention) so
                // that processing continues unhalted. Can halt by a click, but if not
                // done, then processing will continue to file end. Intended as a way to
                // get changes done which rely on cc tables alone, or primarily, without
                // user having to repeatedly hit RETURN key.
	bool m_bRestorePunct; // when TRUE, 4th line visible and user gets asked to manually
				// locate in the target text any medial punction encountered
	bool m_bComposeWndVisible; // TRUE whenever the compose window is potentially
				// visible (it must always be on top)
	bool m_bUseConsistentChanges; // turn on the use of consistent changes when copying
                // source word or phrase as a default (initial) target word or phrase. For
                // either glossing or adapting.
	bool m_bUseSilConverter; // turn on the use of the configured SilConverter when
                // copying source word or phrase as a default (initial) target word or
                // phrase. For either glossing or adapting. NOTE: this flag is mutually
                // exclusive with m_bUseConsistentChanges -- only one can be TRUE at a
                // time.
	bool m_bSaveToKB; // a temporary mode, settable for next potential StoreAdaption
                // only, or StoreGloss only, thereafter reverts to TRUE until explicitly
                // reset; when FALSE, the active src phrase will have its m_bNotInKB flag
                // set TRUE if glossing is not on, but when glossing is on this flag will
                // not be changeable and will be TRUE
	bool m_bForceAsk; // a temporary mode, for next StoreAdaption or StoreGloss only,
                // it forces the CTargetUnit's m_bAlwaysAsk flag to TRUE, even if there is
                // only one translation, or gloss, so far
	bool m_bCopySource; // when TRUE, if no match, copy the source key as first guess
                // at the target adaption or gloss, when FALSE, an empty string is used
                // instead
    // whm 2Aug2018 added the following
    bool m_bSelectCopiedSource; // when TRUE, copied source text within phrasebox is selected
                // Whem FALSE (the default), copied source text in phrasebox is not selected, 
                // and the insertion point is placed at end of the text.
	bool m_bMarkerWrapsStrip; // when TRUE, a st.format marker causes a wrap of the strip
	bool m_bRespectBoundaries; // TRUE by default, if FALSE, user can violate
	bool m_bHidePunctuation;	// when TRUE, punctuation is not shown in lines 0 & 1, if
                // FALSE (the default value) then punctuation shows in lines 0 and 1
	bool m_bStartViaWizard; // used in OnIdle handler to force text in phrase box
				// to be selected
	bool m_bUserTypedSomething; // FALSE when phrase box first created, TRUE when
                // something typed provided it is not an ENTER key, or other control key
	bool m_bUseToolTips;	// shows tooltips when TRUE, none shown when false; default
				// is TRUE
	int m_nTooltipDelay; // amount in milliseconds of time tooltips display before
                // disappearing if m_bUseToolTips is TRUE the default time is 20000 (20
                // seconds)
	bool m_bExecutingOnXO;  // TRUE if command-line switch -xo is used, FALSE otherwise

	// The following weren't initialized in the view's constructor but moved here from the
	// View for safety.
	int	m_nMaxToDisplay; // max # of words/phrases to display at one time (this has to
                // be retrained in our refactoring for backwards compatibility with
                // configuration files - formerly this was bundle's count of sourcephrases,
                // but we are now saving the m_pSourcePhrases list's m_count value
	// next two retained for backwards compatibility of config files, but we make no use
	// of them - just output values 30 & 40 every time, respectively, to basic config file
	int	m_nPrecedingContext; // minimum # of words/phrases in preceding context
	int	m_nFollowingContext; // ditto, for following context
	int	m_curLeading;	 // the between-strips leading value
	int	m_curLMargin;	 // if user wants a left margin, he can set this
	int m_nMinPileWidth; // BEW added 19May15, user-settable from View tab of Preferences; in basic config file
	int	m_curGapWidth;	 // inter-pile gap, measured in pixels (follows a pile)
	int m_saveCurGapWidth; // put normal width in here when free translating (which uses different gap)

	// BEW 24Aug21 restoring a boolean deleted years ago, it resufaced in Bill's old code for
	// updating a changed box width -- one call, sets it FALSE at one call in OnKeyDown() in PhraseBox.cpp line 7667, no idea if ever it's TRUE - ask Bill
	bool m_bSuppressRecalcLayout;

	// from TEXTMETRICs, heights of source & target text lines & the editbox
	int	m_nSrcHeight;	// line height for source language text
	int	m_nTgtHeight;	// ditto for target lines & the pApp->m_targetBox
	int	m_nNavTextHeight; // for use by CCell when working out where to print the
                // nav text & the value is set in the App's UpdateTextHeights() function
	int	m_nSaveActiveSequNum; // a location to save the active sequ number across
				// an operation
	// window size and position saving & restoring
	wxPoint		m_ptViewTopLeft; // client area
	wxSize		m_szView;

	// BEW added 10Feb09 for refactored view layout support
	CLayout* m_pLayout;

	// BEW added 15Aug16 for suppressing option to have or not have collab mode restored
	// in a Shift-Launch
	bool m_bDoNormalProjectOpening; // default TRUE; it can be made FALSE in GetProjectConfiguration()
						   // and the place to restore default would therefore be when
						   // writing out the project configuration file

	// GDLC 2010-02-12
	// Pointer to the free translation display manager
	// Set by the return value from CFreeTrans creator
	CFreeTrans*	m_pFreeTrans;

	// whm added 6Jan12
	bool m_bStatusBarVisible;
	bool m_bToolBarVisible;
	bool m_bModeBarVisible;
	// BEW added 20May15
	bool m_bNoFootnotesInCollabToPTorBE;

	CNotes* m_pNotes;
	CNotes* GetNotes();

	CRetranslation* m_pRetranslation;
	CRetranslation* GetRetranslation();

	CPlaceholder* m_pPlaceholder;
	CPlaceholder* GetPlaceholder();
	bool m_bDisablePlaceholderInsertionButtons; // Set & cleared in PlacePhraseBox() when
				// m_bLanding is TRUE, and FALSE, given the value returned by pDoc->
				// bool IsWithinSpanProhibitingPlaceholderInsertion(CSourcePhrase* pSrcPhrase)

	// values for members of printing support structures
	wxPageSetupDialogData* pPgSetupDlgData; // for page setup
	// Note: When the page setup dialog has been closed, the app can query the
	// wxPageSetupDialogData object associated with the dialog.

	wxPrintData* pPrintData; // for print settings

	POList	m_pagesList; // MFC uses CPtrList
	int		m_pageWidth;
	int		m_pageLength;
	int		m_marginTop;
	int		m_marginBottom;
	int		m_marginLeft;
	int		m_marginRight;
	bool	m_bIsInches;
	bool	m_bIsPortraitOrientation;
	int		m_pageWidthMM;		// m_pageWidth converted to milimeters
	int		m_pageLengthMM;		// m_pageLength converted to milimeters
	int		m_marginTopMM;		// m_marginTop converted to milimeters
	int		m_marginBottomMM;	// m_marginBottom converted to milimeters
	int		m_marginLeftMM;		// m_marginLeft converted to milimeters
	int		m_marginRightMM;	// m_marginRight converted to milimeters
	int		m_paperSizeCode;	// keep as MFC paper size code enum (internally
								// we convert to wxPaperSize)

    // although a more exact conversion is 2.54, because of rounding in the dialogs, the
    // values below work better than their more precise values
	float	config_only_thousandthsInchToMillimetres;
	float	config_only_millimetresToThousandthsInch;
	float	thousandthsInchToMillimetres;
	float	millimetresToThousandthsInch;

	// for note support
	CNoteDlg*	m_pNoteDlg; // non-modal

	// attributes in support of the travelling edit box (CPhraseBox)
    // wxWidgets design Note: whm I've changed the MFC m_targetBox to m_pTargetBox.
    // whm 3May2018 Instances of the new PhraseBox are now created in the App function
    // DoCreatePhraseBox(). That function is called in two places (1) in the View's
    // OnCreate() method, and in the OnOK() handler of the CFontPagePrefs class.
    // Instead of destroying it and recreating the targetBox repeatedly as the MFC 
    // version did, it now lives undestroyed for most of the life of the View
    // (unless there is a font change), and will simply be shown, hidden, 
    // moved, and/or resized where necessary.
    void DoCreatePhraseBox();
	CPhraseBox*		m_pTargetBox; // Our PhraseBox with a dropdown list
	wxString		m_targetPhrase; // the text currently in the m_targetBox
	long			m_nStartChar;   // start of selection in the target box
	long			m_nEndChar;		// end of selection in the target box

	int				m_width_of_w; // BEW 15Aug21 added, for defining a multiple of this value as a default pile width
								  // and other things, eg. the miniSlop, and calculations for expand or contract
								  // of the phrasebox...

	int				nCalcPileWidth_entryCount; // BEW 11Oct for logging of entries to CalcPileWidth on doc entry
	int				nCallCount2; // another counter for same CalcPileWidth entries

	// BEW 17Sep21, added miniSlop, and a setter & getter for it. It's public, so accessible from pApp directly too;
	// but I set both that and m_width_of_w in OnInit()  BEW removed 7Aug21
	//int				miniSlop;
	//void			SetMiniSlop(int width);
	//int				GetMiniSlop();

    // whm modified 10Jan2018 after implementing CPhraseBox dropdown list
    bool m_bChooseTransInitializePopup;

    // whm 24Feb2018 modified by moving some globals out of global space
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    bool m_bInhibitMakeTargetStringCall; // moved here from Adapt_ItView.cpp global space
    bool m_bMergeSucceeded; // moved here from Adapt_ItView.cpp global space
    bool m_bSuppressDefaultAdaptation;
    CTargetUnit* pCurTargetUnit;

   // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    bool m_bLegacySourceTextCopy;
    bool m_bIgnoreScriptureReference_Send;
    wxString m_OldChapVerseStr;

	// Single instance checker added as a new feature to wxWidgets version
	wxSingleInstanceChecker* m_pChecker; // used for preventing more than one
				// program instance by the same user. See OnInit()
	wxString PathSeparator;

    // MFC Note: for support of splitter windows - clicks in green wedge of the non-first
    // pane crash because gpApp->GetView() as coded for the legacy app returns the first
    // view found in the doc template, but the one being clicked in won't be that one, and
    // so subsequent code failed. To get around this, I've overrided OnActivateView() and
    // used the new active view to set this m_pCurView member on the app; and then
    // gpApp->GetView() will scan the list of views and return the one that matches what
    // m_pCurView points at
	// whm WX version not initially supporting splitter window (& probably never will, BEW)

	// BEW added 06Mar06, to store topleft & bottomright coords of the desktop window
	// (used for error checking on window location & size when reading basic config file)
	wxPoint wndTopLeft;
	wxPoint wndBotRight;

	// BEW added for Bob Eaton 26Apr06; for transliteration mode support
	bool		m_bTransliterationMode;

	// for support of user-defined font size for vernacular text in dialogs (version 2.4.0)
	int m_dialogFontSize; // default 12 point size

	// for adding a time delay  for inserting when in Automatic mode
	int			m_nCurDelay;

	// for autosaving
	TIMESETTINGS m_timeSettings;
	bool		 m_bIsDocTimeButton;
	int			 m_nMoves;

	// cc related globals
	bool			m_bCCTableLoaded[4];
	bool			m_bTablesLoaded;
	// paths to cc tables & their names
	wxString		m_tableName[4];
	wxString		m_tableFolderPath[4];

	// whm 6Aug11 Note: The following path values are used mainly when navigation
	// protection for the specific types of inputs/outputs is NOT in effect. These
	// values are saved in the indicated config file(s). When navigation
	// protection is in effect for the specific inputs/outputs, we use the special
	// (upper case) folders and generated file names that are automatically used
	// and manditory. Generally these values record the last...Path that was used
	// for the input/output, so that if navigation protection is OFF for a given
	// input/export type, the values here will still (at least initially) point to
	// the folder that was being used when nav protection was on in previous
	// session(s).

	// whm Note: There was some inconsistency in where some of the following paths
	// were stored in the config files in versions prior to version 6.x.x. and the
	// config file labels that were used. Starting with version 6.x.x I've
	// reassigned some of them to be saved in only one configuration file and the
	// one that is most appropriate for the usage scenarios, with appropriate
	// coding modifications in the Read... and Write... configuration file routines
	// adjusted so that any older config values coming into version 6.x.x will be
	// preserved in the appropriate config file.

	// Note: The following m_last...Path variables are grouped according to which
	// config file the are saved in.

	// /////////// Basic config file (AI-BasicConfiguration.aic) ///////////


    // whm 22Apr2019 added following two members for email Report and Feedback settings for AI-BasicConfiguration.aic

    /// Use: m_serverURL stores the dir path of server for email reporting and feedback function
    wxString m_serverURL;

    /// Use: m_phpFileName stores the php file name for email reporting and feedback function
    wxString m_phpFileName;
    
    /// Use: m_lastCcTablePath stores the last cc table path.
	/// Now: in version 6.x.x saved in basic config file under LastCCTablePath.
	/// Previously: saved in both basic and project config files under DefaultCCTablePath
	/// Related Nav Protect folder: _CCTABLE_INPUTS_OUTPUTS
	wxString	m_lastCcTablePath;

	/// Use: m_lastRetransReportPath stores the last retranslation reports path.
	/// Now: in version 6.x.x saved in basic config file under LastRetranslationReportPath.
	/// Previously: saved in basic config file under RetranslationReportPath.
	/// Related Nav Protect folder: _REPORTS_OUTPUTS
	wxString	m_lastRetransReportPath;

	/// Use: m_lastPackedOutputPath stores the last packed document output path.
	/// Now: in version 6.x.x saved in basic config file under LastPackedDocumentPath.
	/// Previously: None - new for version 6.x.x.
	/// Related Nav Protect folder: _PACKED_INPUTS_OUTPUTS
	wxString	m_lastPackedOutputPath;

	/// Use: m_lastKbOutputPath stores the last KB SFM export/import path.
	/// Now: in version 6.x.x saved in basic config file under LastKBExportPath.
	/// Previously: saved in basic config file under KB_ExportPath.
	/// Related Nav Protect folder: _KB_INPUTS_OUTPUTS
	wxString	m_lastKbOutputPath;

	/// Use: m_lastKbLiftOutputPath stores the last KB LIFT format export/import path.
	/// Now: in version 6.x.x saved in basic config file under LastKBLIFTExportPath.
	/// Previously: None - new for version 6.x.x.
	/// Related Nav Protect folder: _LIFT_INPUTS_OUTPUTS
	wxString	m_lastKbLiftOutputPath;

	// /////////// Project config file (AI-ProjectConfiguration.aic) ///////////

	/// Use: m_lastDocPath stores the last adaptation document path.
	/// Now: in version 6.x.x saved in project config file under LastDocumentPath.
	/// Previously: saved in basic config file under LastDocumentPath.
	/// Related Nav Protect folder: None - always saved in a project's Adaptations folder.
	wxString	m_lastDocPath;

	/// Use: m_lastSourceInputPath stores the last source text input path.
	/// Now: in version 6.x.x saved in project config files under LastNewDocumentFolder.
	/// Previously: saved in both basic and project config files under LastNewDocumentFolder.
	/// Related Nav Protect folder: __SOURCE_INPUTS.
	wxString	m_lastSourceInputPath;

	/// Use: m_lastInterlinearRTFOutputPath stores the last Interlinear RTF export path.
	/// Now: in version 6.x.x saved in project config file under LastInterlinearRTFOutputPath.
	/// Previously: Saved in basic config file under a generic RTFExportPath.
	/// Related Nav Protect folder: _INTERLINEAR_RTF_OUTPUTS
	wxString	m_lastInterlinearRTFOutputPath;

	/// Use: m_lastSourceOutputPath stores the last source text export path.
	/// Now in version 6.x.x saved in project config file under LastSourceTextExportPath.
	/// Previously: saved in basic config file under LastSourceTextExportPath.
	/// Related Nav Protect folder: _SOURCE_OUTPUTS
	wxString	m_lastSourceOutputPath;

	/// Use: m_lastSourceRTFOutputPath stores the last source text RTF export path.
	/// Now: in version 6.x.x saved in project config file under .
	/// Previously: None - new for version 6.x.x.
	/// Related Nav Protect folder: _SOURCE_RTF_OUTPUTS
	wxString	m_lastSourceRTFOutputPath;

	/// Use: m_lastTargetOutputPath stores the last target text export  path.
	/// Now: in version 6.x.x saved in project config file under LastTargetExportPath.
	/// Previously: saved in both basic and project config files under LastExportPath.
	/// Related Nav Protect folder: _TARGET_OUTPUTS
	wxString	m_lastTargetOutputPath;

	/// Use: m_lastTargetRTFOutputPath stores the last target text RTF export path.
	/// Now: in version 6.x.x saved in project config file under LastTargetRTFExportPath.
	/// Previously: None - new for version 6.x.x.
	/// Related Nav Protect folder: _TARGET_RTF_OUTPUTS
	wxString	m_lastTargetRTFOutputPath;

	/// Use: m_lastGlossesOutputPath stores the last glosses as text export path.
	/// Now: in version 6.x.x saved in project config file under LastGlossesTextExportPath.
	/// Previously: None - new for version 6.x.x.
	/// Related Nav Protect folder: _GLOSS_OUTPUTS
	wxString	m_lastGlossesOutputPath;

	/// Use: m_lastGlossesRTFOutputPath stores the last glosses as text RTF export path.
	/// Now: in version 6.x.x saved in project config file under LastGlossesTextRTFExportPath.
	/// Previously: None - new for version 6.x.x.
	/// Related Nav Protect folder: _GLOSS_RTF_OUTPUTS
	wxString	m_lastGlossesRTFOutputPath;

	/// Use: m_lastFreeTransOutputPath stores the last free translations export path.
	/// Now: in version 6.x.x saved in project config file under LastFreeTransExportPath.
	/// Previously: None - new for version 6.x.x.
	/// Related Nav Protect folder: _FREETRANS_OUTPUTS
	wxString	m_lastFreeTransOutputPath;

	/// Use: m_lastFreeTransRTFOutputPath stores the last free translations RTF export path.
	/// Now: in version 6.x.x saved in project config file under LastFreeTransRTFExportPath.
	/// Previously: None - new for version 6.x.x.
	/// Related Nav Protect folder: _FREETRANS_RTF_OUTPUTS
	wxString	m_lastFreeTransRTFOutputPath;

	// Use: m_lastXhtmlOutputPath stores the last Xhtml documents export path
	// Now: in version 6.2.3 saved in project config file under LastXhtmlExportPath
	// Previously: None - new for version 6.2.3.
	// Related Nav Protect folder: _XHTML_OUTPUTS
	wxString	m_lastXhtmlOutputPath;

	// Use: m_lastPathwayOutputPath stores the last Pathway documents export path
	// Now: in version 6.2.3 saved in project config file under LastPathwayExportPath
	// Previously: None - new for version 6.2.3.
	// Related Nav Protect folder: _PATHWAY_OUTPUTS
	wxString	m_lastPathwayOutputPath;

	wxString	m_foldersProtectedFromNavigation; // whm 12Jun11 added for inputs and
												// outputs dirs that are protected from
												// navigation. The dir names are delimited
												// by ':' delimiters within this string which
												// is saved in the project config file

	// BEW added 16Aug09, to support removing \note,\note*,\free,\free*,\bt from exports
	// of either the glosses text or free translation text
	bool			m_bExportingGlossesAsText;
	bool			m_bExportingFreeTranslation;

	// font stuff
	wxFont*		m_pSourceFont;
	wxFont*		m_pTargetFont;
	wxFont*		m_pNavTextFont;
	// NOTE: free translations are done in whatever font (and directionality) that is used
	// for the target text; so until this fails to be satisfactory, we won't define and
	// maintain a separate wxFont for free translations (i.e. there's no m_pFreeTransFont)

    // wxFontData added for wxWidgets version holds font info with get/set methods for font
    // information such as Get/SetColour(), Get/SetChosenFont() etc. A wxFontData object is
    // the last parameter in a wxFontDialog constructor in order to interoperate with the
    // dialog, receiving the user's font choice data when the dialog is dismissed. We use
    // is mainly for GetChosenFont() and for Get/SetColour().
	wxFontData* m_pSrcFontData;
	wxFontData* m_pTgtFontData;
	wxFontData* m_pNavFontData;
	wxFont*		m_pDlgSrcFont;
	wxFont*		m_pDlgTgtFont;
	wxFont*		m_pDlgGlossFont;
	wxFont*		m_pComposeFont;
	wxFont*		m_pRemovalsFont; // BEW added 11July08, so setting of the font and size can
                // be done for the removals bar, & the dynamic changing of the other dlg
                // font pointers won't then clobber this one (as would be the case if I
                // used one of them for the removals bar)
	wxFont*		m_pVertEditFont; // BEW added 23July08, so setting of the font and size can
                // be done for the vertical edit message and controls bar, & the dynamic
                // changing of the other dlg font pointers won't then clobber this one (as
                // would be the case if I used one of them for the vertical edit controls
                // bar)
    // whm: In our wxWidgets version Windows API's LOGFONT is not necessary; we're using
    // our own fontInfo struct

	wxColour	m_sourceColor;
	wxColour	m_targetColor;
	wxColour	m_navTextColor;
	wxColour	m_specialTextColor;
	wxColour	m_reTranslnTextColor;
	wxColour	m_tgtDiffsTextColor;
	wxColour	m_AutoInsertionsHighlightColor;
	wxColour	m_GuessHighlightColor; // whm added 1Nov10 for Guesser support
	wxColour	m_freeTransDefaultBackgroundColor; // it will be light pastel green
				// (set in app constructor)
	wxColour	m_freeTransCurrentSectionBackgroundColor; // it will be light
				// pastel pink
	wxColour	m_freeTransTextColor; // temporarily set to dark purple (0x640064)
				// -- in app constructor
	wxColour sysColorBtnFace; // color used for read-only text controls displaying
				// static text info button face color
	wxColour sysColorWindow;  // color used for read-only text controls displaying
				// static text info window background color

	int			m_backgroundMode;	// whm added 6July2006 Set to wxSOLID in App's
                // OnInit because wxDC's SetTextBackground uses by default background mode
                // of wxTRANSPARENT which doesn't show up on screen regardless of what
                // color we set with SetTextBackground(). Each call to SetTextBackground
                // should have a prior call to SetBackgroundMode(m_backgroundMode), in
                // which the m_backgroundMode = wxSOLID.

	// BEW 19Oct15 vertical edit needs some help. When fixing Seth's bug, dropping into
	// vertical edit at the edit of the source text where the wrongly moved word intial
	// " character gets moved to end of previous word after <space>, editing out the
	// space after " so as to get " on the next word's start again, that was fine, but
	// exiting vert edit mode, the last phrasebox's text was carried unilaterally back
	// to the first active location of the two words, and rubbed out the meaning in the
	// box there. Turns out, some refactoring is needed in a few places.
	int			  m_vertEdit_LastActiveSequNum;  // where the phrasebox last was
	wxString	  m_vertEdit_LastActiveLoc_Adaptation; // if in adaption mode, the m_pTargetBox value
	wxString	  m_vertEdit_LastActiveLoc_Gloss; // if in gloss mode, the m_pTargetBox value
	bool		  m_bVertEdit_WithinSpan; // whatever DoConditionalStore() has for bWithinSpan
										  // default it to FALSE (which results in a hack)
	bool		  m_bVertEdit_IsGlossing; // set TRUE if glossing mode was on at entry
										  // (using pRec->bGlossingModeOnEntry)
	bool		  m_bVertEdit_SeeGlosses; // set TRUE if gbGlosingEnabled was TRUE on entry
										  // (using pRec->bSeeGlossesEnabledAtEntry)

	// source encoding as in input data, and system encoding as defined by system codepage,
	// and the target encoding (determined by tellenc.cpp 3rd party encoding detector)
	wxFontEncoding	m_srcEncoding;
	wxFontEncoding	m_tgtEncoding;

	// whm added following encoding variables to the wx version
	wxFontEncoding m_navtextFontEncoding;
	wxFontEncoding m_dlgSrcFontEncoding;
	wxFontEncoding m_dlgTgtFontEncoding;
	wxFontEncoding m_dlgGlossFontEncoding;
	wxFontEncoding m_composeFontEncoding;
	wxFontEncoding m_removalsFontEncoding;
	wxFontEncoding m_vertEditFontEncoding;
	wxFontEncoding	m_systemEncoding;	// In OnInit: m_systemEncoding =
				// wxLocale::GetSystemEncoding();

	bool m_bConfigFileHasFontEncodingInfo;

	// whm added below for cross-platform locale and encoding considerations.
	wxString	m_systemEncodingName; // In OnInit: m_systemEncodingName =
				// wxLocale::GetSystemEncodingName();
	int			m_systemLanguage;	  // In OnInit: m_systemLanguage =
				// wxLocale::GetSystemLanguage();
	const wxLanguageInfo* m_languageInfo; // In OnInit: m_languageInfo =
				// wxLocale::GetLanguageInfo(m_systemLanguage);

	CurrLocalizationInfo currLocalizationInfo;

	wxLocale* m_pLocale; // pointer for creating our wxLocale object (using
                // the non-default constructor which does not require use of
                // wxLocale::Init()) to which we will add localization catalogs, associate
                // catalog lookup paths, and to which we can add wxLanguage values.


	// Next two added 18Oct13, in support of a bulk pseudo-delete of user's chosen entries
	// for deletion in KB Editor - these two arrays work in parallel. They are put here
	// rather than in CKB instance, because if kbserver is involved in a high latency
	// network, it might take many minutes or much longer if lots are to be deleted,
	// before the kbserver's entries can get their deleted flag's set to 1; and in that
	// time the project may be exited - which would leave the local KB and the remote
	// kbserver not in sync - with no good way to fix it.
	// Note, these are needed when the project is not a KB Sharing one, so they are not
	// wrapped with #define(_KBSERVER)...#endif
	wxArrayString m_arrSourcesForPseudoDeletion;
	wxArrayString m_arrTargetsForPseudoDeletion;

	// project-defining attibutes
	wxString	m_sourceName; // name of the source language
	wxString	m_targetName; // name of the target language
	wxString	m_glossesName; // name of the glossing language (usually set by a LIFT import)
							   // but the user can do it in KBPage of the Preferences... dlg
	wxString	m_freeTransName; // name of the language used for doing free translations
		// The m_freeTransName is supported, but has only a little functionality. It's input 
		// and and output via the basic and project config files; and the other use is for 
		// KB.cpp where in the Kb page of the Preferences, if there is variance of m_sourceName,
		// m_targetName, m_glossesName, or m_freeTransName due to user edit of any of these, 
		// or discovered differences, the changed ones can be brought into line with the
		// project's names.

	// whm added 10May10 for KB LIFT XML Export support; also used for xhtml exports,
	// and BEW 23July12 added m_freeTransLanguageCode to make all four text types be supported
	wxString	m_sourceLanguageCode; // code of the source language
	wxString	m_targetLanguageCode; // code for the target language
	wxString	m_glossesLanguageCode; // BEW 3Dec11 added, since LIFT can support glossing KB too
	wxString	m_freeTransLanguageCode;  // the 2- or 3-letter code for free translation language

	// BEW 5Sep20 changed these to have 'Name' -to get past some compile errors - maybe remove later
	wxString	m_sourceLanguageName; // name of the source language
	wxString	m_targetLanguageName; // name for the target language
	wxString	m_glossesLanguageName; // name
	wxString	m_freeTransLanguageName;  // name for free translation language

	// BEW 25Mar21 Find or Find & Replace support, using the struct
	// CacheFindReplaceConfig - defined above at line 1017
	bool        WriteCacheDefaults(); // writes default values into the struct
	bool        WriteFindCache();
	bool        ReadFindCache();
	// Use the next 3, one in each of the above functions, to hold the config values
	CacheFindReplaceConfig defaultFindConfig;
	CacheFindReplaceConfig readwriteFindConfig;
	wxComboBox*	m_pComboSFM; 
	// BEW 3Apr21... and for the WriteReplaceCache() for use in m_pReplaceDlg
	bool        WriteReplaceCache();
	bool        ReadReplaceCache();
	CacheReplaceConfig readwriteReplaceConfig; // the struct for config values


	// Status bar support
	void	 RefreshStatusBarInfo();
	void	 StatusBarMessage(wxString& message);

	wxString m_strSpacelessSourcePuncts; // for use in TokenizeText()
	wxString m_strSpacelessTargetPuncts; // ditto
	wxString MakeTargetFinalPuncts(wxString tgtPuncts); // includes a space
	wxString m_finalTgtPuncts; // stores what MakeTargetFinalPuncts() returns
	wxString MakeSourceFinalPuncts(wxString srcPuncts); // does not include a space
	wxString m_finalSrcPuncts; // stores what MakeSourceFinalPuncts() returns
	bool     m_bParsingSource;
	wxString m_chapterNumber_for_ParsingSource;
	wxString m_verseNumber_for_ParsingSource;

    bool	 m_bMakeDocCreationLogfile;
	bool	 m_bFinalTypedPunctsGrabbedAlready;

	bool	 m_bALT_KEY_DOWN; // BEW added 31Jul16 to track ALT key down (TRUE), and up (back to FALSE)

	// BEW 20Jul21, added boolean to support user clicks in tgt text of a retranslation causing
	// the retranslation edit dlg to open for editing
	bool	 m_bUserClickedTgtWordInRetranslation; // if TRUE OnLButtonDown() opens the dlg, bypasses auiToolbar

//#if defined(_KBSERVER)
	// support for Status bar showing "Deleting n of m" while deleting a kb from KBserver
	void StatusBar_ProgressOfKbDeletion();
	void StatusBar_EndProgressOfKbDeletion();

//#endif

//#if defined(_KBSERVER)

	KBSharingMgrTabbedDlg* m_pKBSharingMgrTabbedDlg;
	KBSharingMgrTabbedDlg* GetKBSharingMgrTabbedDlg();
	// Next three are set when authenticating with the bool '...ForManager' 2nd param of the
	// KBSharingAuthenticationDlg constructor set TRUE. When TRUE, someone is authenticating
	// to use the KB Sharing Manager gui; his credentials must be stored separately from
	// the normal user's otherwise the advisor or administrator would overwrite the user's
	// settings when he uses the mananger gui
	wxString	m_strForManagerUsername;
	wxString    m_strForManagerUsername2; // for LookupUser()
	wxString	m_strForManagerIpAddr;
	wxString	m_strForManagerPassword;
	// Next one is for the ChangePermission feature in the KB Sharing Manager,
	// where ConfigureMovedDatFile(), when building commandLine, needs to know
	// what the manager's looked up "selected_user" (ie. user2) is, so it can
	// be added to the commandLine being built (as the last param) And ditto
	// for change_fullname, at lines 3430-1
	wxString	m_strChangePermission_User2;
	wxString	m_strChangeFullname_User2; // BEW added 9Dec20
	wxString	m_strChangeFullname;       // BEW added 9Dec20
	wxString	m_strChangePassword_User2;    // BEW added 11Jan21
	wxString	m_ChangePassword_NewPassword; // BEW added 11Jan21

	bool		m_bUserAuthenticating;
	bool		m_bUser1IsUser2; // default FALSE - needed in LookupUser()

	// Use a switch internally in CreatInputDatBlanks to make the constant text lines
	// for the various *.dat 'input' files which are dynamically copied to the execPath
	// folder and there their final line, the command line of parameters, is replaced 
	// with the needed parameters for the call by an .exe file using ::wxExecute(...exe);
    // whm 22Feb2021 removed the & from the function signature since execPth should be
    // a value parameter and not a reference parameter.
	void CreateInputDatBlanks(wxString execPth);
	bool AskIfPermissionToAddMoreUsersIsWanted();

/*
// Don't use an enum, int values are simpler
	const int noDatFile = 0;
	const int credentials_for_user = 1;
	const int lookup_user = 2;
	const int list_users = 3;
	const int create_entry = 4;
	const int pseudo_delete = 5;
	const int pseudo_undelete = 6;
	const int lookup_entry = 7;
	const int changed_since_timed = 8;
	const int upload_local_kb = 9;
	const int change_permission = 10;
	const int change_fullname = 11;
	const int change_password = 12;
	// add more here, as our solution matures
	const int blanksEnd = 13; // this one changes as we add more above

*/
	// Handler files for the cases in the KBserverDAT_Blanks switch, in CreateInputDatBlanks()
	void MakeAddForeignUsers(const int funcNumber, wxString dataPath); // = 1 // whm Note: removed execPath parameter
	void MakeLookupUser(const int funcNumber, wxString dataPath); // =2  // whm Note: removed execPath parameter
	void MakeListUsers(const int funcNumber, wxString dataPath); // = 3  // whm Note: removed execPath parameter
	void MakeCreateEntry(const int funcNumber, wxString dataPath); // = 4  // whm Note: removed execPath parameter
	void MakePseudoDelete(const int funcNumber, wxString dataPath); // = 5  // whm Note: removed execPath parameter
	void MakePseudoUndelete(const int funcNumber, wxString dataPath); // = 6  // whm Note: removed execPath parameter
	void MakeLookupEntry(const int funcNumber, wxString dataPath); // = 7  // whm Note: removed execPath parameter
	void MakeChangedSinceTimed(const int funcNumber, wxString dataPath); // = 8  // whm Note: removed execPath parameter
	void MakeUploadLocalKb(const int funcNumber, wxString dataPath); // = 9  // whm Note: removed execPath parameter
	void MakeChangePermission(const int funcNumber, wxString dataPath); // = 10  // whm Note: removed execPath parameter
	void MakeChangeFullname(const int funcNumber, wxString dataPath); // = 11  // whm Note: removed execPath parameter
	void MakeChangePassword(const int funcNumber, wxString dataPath); // = 12  // whm Note: removed execPath parameter

	bool m_bAddUser2UserTable; // BEW 24Dec21, defaul FALSE, but True in app's OnAddUsersToKBserver() handler

	void CheckForDefinedGlossLangName();

	wxString m_datPath; // BEW 9Oct20, copy from ConfigureMovedDatFile() to here so that
						// CallExecute() can pick it up to delete it from the execPath folder
						// after the system() call has finished. This way makes sure old
						// input .dat files don't linger to be a potential source of error
	wxString m_ChangedSinceTimed_Timestamp;
	bool m_bUserRequestsTimedDownload; // set True if user uses GUI to ask for a bulk
						// download, or ChangedSince (ie. incremental) download. If
						// TRUE, skip the OnIdle() ChangedSince_Timed request

	// BEW 1Oct12
	// Note: the choice to locate m_pKBServer[2] pointers here, rather than one in each of
	// m_pKB and m_pGlossingKB, so that they are created and destroyed when the adapting
	// CKB instances are created and destroyed, respectively, is deliberate. There are
	// times when local KBs are instantiated for processes that are best handled without
	// an active connection to a remote KBserver database - for instance, transferring
	// adaptations to glosses in a new project; KB restoration via the File > Restore
	// Knowledge Base command; reconstituting a CKB from a git repository, and maybe
	// others. So we'll instantiate KbServer instances only when appropriate.

	// m_pKbServer is an array of two pointers to KbServer instances. Each is NULL
	// everywhere except in a project designated for KB sharing, and that project is
	// currently active (creation and destruction are handled within SetupForKBServer()
	// and ReleaseKBServer(), respectively) See KbServer.cpp, and KbServer.h. The first
	// is for an adapting KB, the second for a glossing KB.
private:
	KbServer* m_pKbServer[2]; // [0] one for adapting, [1] one for glossing
public:
	KbServer* m_pKbServer_Persistent; // use this one from the
			// KbSharingStatelessSetupDlg as it may need to be persistent (as
			// when the m_bKbSvrMgr_DeleteAllIsInProgress flag is TRUE) for the
			// session, or much of the session; but when that flag is false, it
			// would be deleted when the KB Sharing Manager gets deleted. So
			// we don't want either the Manager class, or the
			// KBSharingAuthenticationDlg class owning this pointer.
			// (It needs to persist when a deletion of the entries in a remote
			// kb is being done - deleting is done one by one, so it may take
			// anything from minutes to hours or over a day - depending on
			// how many hundreds, or thousands of entries are to be deleted.
			// A KBserver on the LAN, deletes about 20 a second. One on the
			// web with a high latency (4 secs per entry deleted) can tie up
			// a machine for over a day!!
			// I think the way to handle the problem is to create the instance
			// in the correct active project (because a KbServer instance has a
			// pointer to local CKB instance - but deletion doesn't use the
			// local CKB, so we should be able to safely set that ptr to NULL)
			// and then once the queue is populated with the entries to delete,
			// we can have the persistent KbServer instance delete it. (But openssl
			// leaks memory from a thread, so we must do it synchronously now.)
			//
            // The basic sharing functionalities, however, will not use this persistent
            // KbServer instance - but rather create their KbServer instances on demand
            // (when entering a project for example, deleting when leaving the project
            // etc).
			// Our KB Sharing Manager code only permits one kb deletion at a
			// time, and so we won't get two or more processes trying to access
			// this KbServer instance, so we don't need a mutex protection.
			// The KBSharingMgrTabbedDlg::OnButtonKbsPageRemoveKb(...event)
			// is the ONLY place that points at this KbServer instance; the
			// sharing Manager's other handlers will use m_pKbServer_ForManager
	KbServer* m_pKbServer_ForManager; // As above, created in OnInit(),
			// destroyed in OnExit(), but used for short term KBserver accesses
			// such as authentications. No mutex proection needed

	void	  SetKbServer(int whichType, KbServer* pKbSvr);
	KbServer* GetKbServer(int whichType); // getter for whichever m_pKbServer is current, adapting or glossing
	void	  DeleteKbServer(int whichType);
	bool	  SetupForKBServer(int whichType);
	bool	  ReleaseKBServer(int whichType);
	bool	  KbServerRunning(int whichType); // Checks m_pKbServer[0] or [1] for non-NULL or NULL
    // GDLC 20JUL16, BEW 7Nov20 added 3rd test - for m_bUserLoggedIn, to next two
    bool      KbAdaptRunning(void); // True if AI is in adaptations mode and an adaptations KB server is running
    bool      KbGlossRunning(void); // True if AI is in glossing mode and a glossing KB server is running
	// BEW 7Nov20 made next one to call whichever of the above two is appropriate for
	// the current running mode, as determined by gbIsGlossing (this is not called everywhere
	// because some calls are for authentication or some other purpose)
	bool	  AllowSvrAccess(bool bIsGlossingMode);

	// BEW added next, 26Nov15
	bool	  ConnectUsingDiscoveryResults(wxString curIpAddr, wxString& chosenIpAddress,
								 wxString& chosenHostname, enum ServDiscDetail &result);
	bool	  m_bServiceDiscoveryWanted; // TRUE if ConnectUsingDiscoveryResults is wanted, 
					// FALSE for manual ipAddress entry; and don't ever store the value in 
					// any config file; default TRUE
	bool	  m_bEnteringKBserverProject; // used in OnIdle() to delay connection attempt until doc is displayed
	void	  DoDiscoverKBservers(); // BEW 20Jul17 scan for publishing kbservers - by Leon's scripts
	bool	  m_bServDiscSingleRunIsCurrent;
	bool	  m_bAuthenticationCancellation;
	bool	  m_bUserLoggedIn; // TRUE when the user (not an administrator) logs in successfully
	bool	  m_bLoginFailureErrorSeen; // an aid to prevent too many error messages

	void	  ExtractIpAddrAndHostname(wxString& result, wxString& ipaddr, wxString& hostname);
    bool      UpdateExistingAppCompositeStr(wxString& ipaddr, wxString& hostname, wxString& composite);
    bool      AddUniqueStrCase(wxArrayString* pArrayStr, wxString& str, bool bCase);
    bool      CommaDelimitedStringToArray(wxString& str, wxArrayString* pArr);

	int		  GetKBTypeForServer(); // returns 1 or 2

	// These next two are not part of the AI_UserProfiles feature, we want them for every profile
	void	  OnKBSharingManagerTabbedDlg(wxCommandEvent& WXUNUSED(event));
	void      OnUpdateKBSharingManagerTabbedDlg(wxUpdateUIEvent& event);
	int		  m_nMgrSel; // (public) index value (0 based) for selection in the the listbox of 
				 // the manager's user page, and has value wxNOT_FOUND when nothing is selected
	bool	  m_bChangePermission_DifferentUser; // BEW 5Jan21 moved from Share Mgr .h to here, default FALSE
	wxString  m_ChangePermission_OldUser;
	wxString  m_ChangePermission_NewUser;
	bool	  m_bChangingPermission; // BEW 7Jan21 needed to simplify control in LoadDataForPage(0)
	wxString  m_strNewUserLine; // this is a scratch string, for use with adding a new user in KB Sharing Mgr
							   // add_foreign_KBUsers_results.dat doesn't have all the
							   // because values needing to be added to the comma-separated arrLines line
							   // (see code at AI.cpp 19,746 or thereabouts, in the else block)


	wxString  GetFieldAtIndex(wxArrayString& arr, int index); // BEW created 21Dec20
	int		  GetIntAtIndex(wxArrayInt& arr, int index); // BEW Created 21Dec20
	void	  SetFieldAtIndex(wxArrayString& arr, int index, wxString field); // BEW created 21Dec20
	void	  SetIntAtIndex(wxArrayInt& arr, int index, int field); // BEW Created 21Dec20
	void      CopyMgrArray(wxArrayString& srcArr, wxArrayString& destArr); // BEW created 21Dec20
	void      CopyMgrArrayInt(wxArrayInt& srcArr, wxArrayInt& destArr); // BEW created 21Dec20
	void	  MgrCopyFromSet2DestSet(bool bNormalToSaved); // BEW 21Dec20 TRUE copies 'normal set' 
					// to the 'saved set', FALSE is reverse, 'saved set' to 'normal set'

	// Next two for opening Leon's little dialog for adding new users to user table
	void	  OnAddUsersToKBserver(wxCommandEvent& WXUNUSED(event));
	void      OnUpdateAddUsersToKBserver(wxUpdateUIEvent& event);

//#endif
//#if !defined(_KBSERVER)
//	void	  OnUpdateKBSharingManagerTabbedDlg(wxUpdateUIEvent& event);
//#endif
//#if defined(_KBSERVER)
	// Next three are stored in the project configuration file
	bool		m_bIsKBServerProject; // TRUE if the user wants an adapting kbserver for
								// sharing kb data between clients in the same AI project
	bool		m_bIsGlossingKBServerProject; // TRUE for sharing a glossing KB
									  // in the same AI project as for previous member
	wxString	m_strKbServerIpAddr; // for the server's ipAddr, something like 192.168.2.8 on a LAN
	wxString	m_strKbServerHostname; // we support naming of the KBserver installations, BEW added 13Apr16
	// BEW added next, 7Sep15, to store whether or not sharing is temporarily disabled
	bool		m_bKBSharingEnabled; // the setting applies to the one, or both kbserver types
									 // simultaneously if sharing both was requested
	// BEW 26May16, added next two, because any move of the phrasebox will call PlacePhraseBox()
	// and that will internally try to do DoStore_ForPlacePhraseBox() and that in turn, if
	// m_bIsKBServerProject (from config file) is TRUE, will try to access a connected KBserver -
	// but if one is not yet connected, the app crashes. So we have to protect from such crashes.
	bool m_bAdaptationsKBserverReady; // TRUE if a connection is current, to an adaptations KBserver
	bool m_bGlossesKBserverReady; // TRUE if a connection is current, to a glosses KBserver
	// The above didn't make it into the Linux code on git, so this will line force an update
	bool m_bAdaptationsLookupSucceeded; // if FileToEntryStruct() got filled successfully, TRUE
	bool m_bGlossesLookupSucceeded; // if FileToEntryStruct() got filled successfully, TRUE, glossing mode on

	// m_bIsKBServerProject and m_bIsGlossingKBServerProject, while set from the project config
	// file, can be cleared to FALSE at initialization of a setup, losing the values from the
	// config file. So I've defined two new booleans which likewise are set from the project
	// config file, but don't get cleared to FALSE anywhere, except at start of OnInit()
	// and also potentially in the dialog shown to the user when Setup Or Remove Knowledge
	// Base Sharing is interacted with - via it's two checkboxes (the latter, if different
	// valued than what was gotten from earlier settings or from config file, must be obeyed).
	// This way I can, at any time, determine what the project settings for sharing the
	// adapting and/or glossing KB actually currently are; or alter them via the
	// KbSharingSetup instance as mentioned just above.
	bool		m_bIsKBServerProject_FromConfigFile;
	bool		m_bIsGlossingKBServerProject_FromConfigFile;

	// Deleting an entire KB's entries in the entry table of kbserver will be done as a
	// background task - so we need storage capability that persists after the KB Sharing
	// Manager GUI has been closed (the button for getting the job started is in the GUI)
	// - the job may take as long as a couple of days to complete, so the needed storage
	// of ID values is in the queue in the KbServer instance we use here for the job
	bool		m_bKbSvrMgr_DeleteAllIsInProgress; // use to prevent 'entire deletion'
                    // of more than one, of a shared kb from the currently accessed
                    // kbserver, at a time; this will also absolve us of the need to set up
                    // a mutex for this job, because the ChangedSince_Queued() download
                    // that gets the array of IDs for entries in the entry table that have
                    // to be deleted will be done synchronously at the start of the job,
                    // and only after that will the background thread be fired to do the
                    // job of emptying of entries
	//KbServer*		m_pKbServerForDeleting; // create a stateless one on heap, using
						// this member - the creation is done in the button handler
						// of the KB sharing manager's GUI, on Kbs page...
	// Note: m_pKbServerForDeleting has its own DownloadsQueue m_queue, which will store
	// KbServerEntry structs from which we can extract the ID value from each; so no mutex
	// is needed for our synchronous call of ChangedSince_Queued() to download the entries
	// which we need to delete (ChangedSince_Queued() is supported by the s_QueueMutex, but
	// we will not be synchronously trying to remove any queue members, so we can ignore
	// that mutex - we'll do the removals after the download has filled the queue.)
	size_t			m_nQueueSize; // set to entry count after download completes
	size_t			m_nIterationCounter; // the N value of progress shown as "N:M" or N of M
	long			kbID_OfDefinitionForDeletion; // store the kbID here, for when we need it
	// BEW deprecated next three, 18Feb16, use values from m_pKbServer_Persistent instead
	//wxString		m_srcLangCodeOfCurrentRemoval;
	//wxString		m_nonsrcLangCodeOfCurrentRemoval;
	//int			m_kbTypeOfCurrentRemoval; // either undefined (-1) or 1 (adapting) or 2 (glossing)
	// The next two store the state of the KB Sharing Manager gui when it is instantiated.
	// These values only have meaning provided app's m_pKBSharingMgrTabbedDlg is not NULL
	bool			m_bKbPageIsCurrent; // default is FALSE (these two are initialized in OnInit())
	bool			m_bAdaptingKbIsCurrent; // default is TRUE


//#endif // for _KBSERVER

	wxString m_strSentFinalPunctsTriggerCaps; // list of sentence final punctuation characters
				// that cause capitalization at the start of next sentence (no GUI, just a project
				// configuration line - having content is the flag for the feature being activated
	void	 EnsureProperCapitalization(int nCurrSequNum, wxString& tgtText); // call it, if conditions
																// comply, from start of StoreText()
	bool	 m_bSentFinalPunctsTriggerCaps; // TRUE if above string is non-empty, FALSE otherwise

	bool	 m_bMergerIsCurrent; // BEW created 14Apr16 due to bug report on 13thApril by Stefan Kasarik
		// His problems was this. If a four word source text:  cao cao building end
		// was adapted, (cao is Vietnamese for 'tall'), as follows: first instance adapt
		// with any meaning (I chose 'tall'), then press Enter key. Next cao is auto-adapted
		// and box halts at 'building'. SHIFT+TAB to take the box back to the second cao.
		// Doing that reduces m_refCount from 2 back to 1. Then do ALT+RightArrow to select
		// 'cao buildin' in order to make a phrase. Start typing an adaptation - I chose to
		// type 'skyscraper'. As soon as I typed the 's', the merge is done and in doing so
		// it internally again calls RemoveRefString(), and the built in filters don't apply
		// and so control gets to the bit of code in the m_refCount == 1 section where the
		// ref count is to be decremented - which would take it to 0. The current active
		// location is the second cao instance, at m_nSequNumber = 1. m_refCount going to 0
		// means that the word is not adapted anywhere - which is a bogus conclusion, and
		// then pRefString->m_bDeleted is set TRUE. This has the nasty consequence of
		// removing the cao/tall entry from the KB. This loss of data by doing a merger
		// is NOT what we want AI to do. Solution: use this new boolean in
		// OnButtonMerge() - set it TRUE when entered, and FALSE when leaving. Then in
		// CReferenceString object, put a filtering test to check for TRUE, and when
		// so, skip the code which sets m_refCount to 0 and pRefString->m_bDeleted to TRUE.

	// BEW added 2Dec2011 for supporting LIFT multilanguage glosses or definitions
	// (these are used for getting a target text entry, if the import is redone in
	// glossing mode in order to populate the glossing KB, these are wiped out and
	// reset from the 2nd import of the same LIFT file) The first four of the
	// following members are set by GetLIFTlanguageCodes() (see XML.cpp), the
	// fifth defaults to comma and semicolon - and is set in OnInit()
	bool		m_bLIFT_use_gloss_entry; // TRUE if parsing looks at <gloss> entries, FALSE
										 // if parsing looks at <definition> entries
	wxString	m_LIFT_src_lang_code; // src language ethnologue code obtained from LIFT file
	wxString	m_LIFT_chosen_lang_code; // user's chosen language (that is, its code)
	wxArrayString m_LIFT_multilang_codes; // the codes for each language in entries
	wxString	m_LIFT_subfield_delimiters; // for comma and any other subfield delimiters allowed
	wxString	m_LIFT_cur_lang_code; // set to "value" every time a new lang="value" attr is parsed
	wxArrayString m_LIFT_formsArray; // one or more PCDATA strings from <text> in <form> element
	// of the next two, only one is used per LIFT import, chosen by m_LIST_use_gloss_entry
	// value - if the latter is TRUE, m_LIFT_glossesArray is used, if FALSE, m_LIFT_definitionsArray
	// is used
	wxArrayString m_LIFT_glossesArray; // one or more PCDATA strings from <text> in <gloss> element
	wxArrayString m_LIFT_definitionsArray; // one or more PCDATA strings from <text> in <definition> element

	bool		m_bExistingAdaption;
	bool		m_bKBReady;
	CKB*		m_pKB; // pointer to the knowledge base
	bool		m_bGlossingKBReady;
	CKB*		m_pGlossingKB; // pointer to the glossing knowledge base

	// autosaving flag
	bool		m_bNoAutoSave;

	// flags and variables for restoring earlier location on launch
	bool		m_bEarlierProjectChosen;
	bool		m_bEarlierDocChosen;
	int			nLastActiveSequNum;		// going west with docVersion 8, but still used by docVersion 7

	// whm For following see block above of other variables I've moved from the View
    // BEW 22Sep05: moved here from the view class, because setting it in the view's
    // creator is a problem when a new view is created - as happens when the user splits
    // the window, and that would otherwise cause OnIdle() to spuriously open the Start
    // Working wizard
	bool		m_bJustLaunched;

#ifdef _RTL_FLAGS
	// flags for RTLReading languages
	bool		m_bSrcRTL; // true when source language reads right to left
	bool		m_bTgtRTL; // ditto, for target language
	bool		m_bNavTextRTL; // ditto, for navigation text
#endif

	bool		m_bZoomed; // is main frame zoomed or not

	// source & target punctuation strings, for adaptation use
	wxString	m_punctuation[2];		// can be unicode
	wxString	m_punctWordBuilding[2]; // can be unicode

	// punctuations correspondences, for adaptation use
	PUNCTPAIR		m_punctPairs[MAXPUNCTPAIRS];
	TWOPUNCTPAIR	m_twopunctPairs[MAXTWOPUNCTPAIRS];

    // for supporting ordinary single-quote as punctuation, or not as punctuation, so that
    // IsOpeningQuote() and IsClosingQuote() can include, or not include, it in the
    // testing; and also for ordinary double-quote (we'll assume user will not ever make
    // curly quotes word-building characters - but if that had to be supported then we'd
    // need four more booleans for those and they'd have to be set/reset in
    // OnEditPunctCorresp() and used in IsOpeningQuote(), IsClosingQuote() in the same way
    // as those below are used there)
	bool		m_bSingleQuoteAsPunct; // default TRUE set in OnInit() & InitializePunctuation() as of 2Nov16, BEW
	bool		m_bDoubleQuoteAsPunct; // default TRUE set in OnInit() & InitializePunctuation()

	// file i/o and directory structures & support for custom work folder locations
	wxString	m_workFolderPath;	// default path to the "Adapt It Work" or "Adapt It
									// Unicode Work" folder, depending on which build
	wxString	m_customWorkFolderPath; // user or advisor defined work folder location
										// used when the following boolean is TRUE
	bool		m_bUseCustomWorkFolderPath; // default FALSE for legacy behaviour, set
									// TRUE when Adapt It is pointed at a custom work
									// folder location
	bool		m_bLockedCustomWorkFolderPath; // TRUE if a custom work folder location has
									// been made persistent, else FALSE

	wxString	m_userProfileFileWorkFolderPath; // whm added 7Sep10

	wxString	m_usageLogFilePathAndName; // whm added 8Nov10
	wxFile*		m_userLogFile; // whm added 12Nov10 the wxFile descriptor used with m_usageLogFilePathAndName

    // whm 6Apr2020 added below for document creation log file
    wxString    m_docCreationFilePathAndName; 
    wxFile*     m_docCreationLogFile;

	//wxString	m_packedDocumentFilePathOnly; // whm added 8Nov10
	//wxString	m_ccTableFilePathOnly; // whm added 14Jul11

    // whm added 5Jun09 for alternate "forced" work folder path (forced by use of -wf
    // <path> command-line option)
	wxString	m_wf_forced_workFolderPath; // any path following a -wf
				// command-line option
	wxString	m_newdoc_forced_newDocPath; // any path following a -newdoc
				// command-line option
	wxString	m_exports_forced_exportsPath; // any path following a -exports
				// command-line option

	wxString	m_theWorkFolder;		// "Adapt It Work" or "Adapt It Unicode Work" or
										// since Bill's -wf switch, even some other folder
										// name
	wxString	m_localPathPrefix;	// the part of the workfolder path before the
				// m_theWorkFolder part
	wxString	m_adaptationsFolder;	// "Adaptations" folder
	wxString	m_curProjectName;	// <Project Name> in the form
				// "<SourceLanguageName> to <TargetLanguageName> Adaptations"
	wxString	m_curProjectPath;	// "C:\My Documents\Adapt It Work\<Project Name>"
	wxString	m_curAdaptationsPath;	// "C:\My Documents\Adapt It Work\<Project Name>\Adaptations"

	wxString	m_setupFolder;			// whm renamed m_setupFolder to this
										// and moved to App class 31July06
	wxString	m_executingAppPathName;	// whm added to get the path and file
										// name of executing app
	// BEW 14July10, Added next two to support hiding folder navigation from the user), along
	// with the m_sourceInputsFolderPath variable which follows.
	// whm 12Jun11 modified folder names and added more variables for other inputs and outputs
	bool		m_bProtectSourceInputsFolder;
	wxString	m_sourceInputsFolderName; // in OnInit() we set to "__SOURCE_INPUTS"
	wxString	m_sourceInputsFolderPath; // always a child of folder that m_curProjectPath
										// points to; the path is defined where m_curProjectPath
										// gets defined
	bool		m_bProtectFreeTransOutputsFolder;
	wxString	m_freeTransOutputsFolderName; // in OnInit() we set to "_FREETRANS_OUTPUTS"
	wxString	m_freeTransOutputsFolderPath; // always a child of folder that m_curProjectPath
										// points to; the path is defined where m_curProjectPath
										// gets defined
	bool		m_bProtectFreeTransRTFOutputsFolder;
	wxString	m_freeTransRTFOutputsFolderName; // in OnInit() we set to "_FREETRANS_RTF_OUTPUTS"
	wxString	m_freeTransRTFOutputsFolderPath; // always a child of folder that m_curProjectPath
										// points to; the path is defined where m_curProjectPath
										// gets defined
	bool		m_bProtectGlossOutputsFolder ;
	wxString	m_glossOutputsFolderName; // in OnInit() we set to "_GLOSS_OUTPUTS"
	wxString	m_glossOutputsFolderPath; // always a child of folder that m_curProjectPath
										// points to; the path is defined where m_curProjectPath
										// gets defined
	bool		m_bProtectGlossRTFOutputsFolder;
	wxString	m_glossRTFOutputsFolderName; // in OnInit() we set to "_GLOSS_RTF_OUTPUTS"
	wxString	m_glossRTFOutputsFolderPath; // always a child of folder that m_curProjectPath
										// points to; the path is defined where m_curProjectPath
										// gets defined
	bool		m_bProtectInterlinearRTFOutputsFolder;
	wxString	m_interlinearRTFOutputsFolderName; // in OnInit() we set to "_INTERLINEAR_RTF_OUTPUTS"
	wxString	m_interlinearRTFOutputsFolderPath; // always a child of folder that m_curProjectPath
										// points to; the path is defined where m_curProjectPath
										// gets defined
	bool		m_bProtectSourceOutputsFolder;
	wxString	m_sourceOutputsFolderName; // in OnInit() we set to "_SOURCE_OUTPUTS"
	wxString	m_sourceOutputsFolderPath; // always a child of folder that m_curProjectPath
										// points to; the path is defined where m_curProjectPath
										// gets defined
	bool		m_bProtectSourceRTFOutputsFolder;
	wxString	m_sourceRTFOutputsFolderName; // in OnInit() we set to "_SOURCE_RTF_OUTPUTS"
	wxString	m_sourceRTFOutputsFolderPath; // always a child of folder that m_curProjectPath
										// points to; the path is defined where m_curProjectPath
										// gets defined
	bool		m_bProtectTargetOutputsFolder;
	wxString	m_targetOutputsFolderName; // in OnInit() we set to "_TARGET_OUTPUTS"
	wxString	m_targetOutputsFolderPath; // always a child of folder that m_curProjectPath
										// points to; the path is defined where m_curProjectPath
										// gets defined
	bool		m_bProtectTargetRTFOutputsFolder;
	wxString	m_targetRTFOutputsFolderName; // in OnInit() we set to "_TARGET_RTF_OUTPUTS"
	wxString	m_targetRTFOutputsFolderPath; // always a child of folder that m_curProjectPath
										// points to; the path is defined where m_curProjectPath
										// gets defined
	// whm added next three 23Jul12
	bool		m_bProtectXhtmlOutputsFolder;
	wxString	m_xhtmlOutputsFolderName; // in OnInit() we set to "_XHTML_OUTPUTS"
	wxString	m_xhtmlOutputsFolderPath; // always a child of folder that m_curProjectPath
										// points to; the path is defined where m_curProjectPath
										// gets defined
	// whm added next three 14Aug12
	bool		m_bProtectPathwayOutputsFolder;
	wxString	m_pathwayOutputsFolderName; // in OnInit() we set to "_PATHWAY_OUTPUTS"
	wxString	m_pathwayOutputsFolderPath; // always a child of folder that m_curProjectPath
										// points to; the path is defined where m_curProjectPath
										// gets defined
	bool		m_bProtectKbInputsAndOutputsFolder;
	wxString	m_kbInputsAndOutputsFolderName; // in OnInit() we set to "_KB_INPUTS_OUTPUTS"
	wxString	m_kbInputsAndOutputsFolderPath; // always a child of folder that m_curProjectPath
										// points to; the path is defined where m_curProjectPath
										// gets defined
	bool		m_bProtectLiftInputsAndOutputsFolder;
	wxString	m_liftInputsAndOutputsFolderName; // in OnInit() we set to "_LIFT_INPUTS_OUTPUTS"
	wxString	m_liftInputsAndOutputsFolderPath; // always a child of folder that m_curProjectPath
										// points to; the path is defined where m_curProjectPath
										// gets defined
	bool		m_bProtectPackedInputsAndOutputsFolder;
	wxString	m_packedInputsAndOutputsFolderName; // in OnInit() we set to "_PACKED_INPUTS_OUTPUTS"
	wxString	m_packedInputsAndOutputsFolderPath; // always a child of folder that m_workFolderPath
										// or m_customWorkFolderPath points to; the path is defined in
										// OnInit()
	bool		m_bProtectCCTableInputsAndOutputsFolder;
	wxString	m_ccTableInputsAndOutputsFolderName; // in OnInit() we set to "_CCTABLE_INPUTS_OUTPUTS"
	wxString	m_ccTableInputsAndOutputsFolderPath; // always a child of folder that m_workFolderPath
										// or m_customWorkFolderPath points to; the path is defined in
										// OnInit()
//#if defined(FWD_SLASH_DELIM)
	wxString	m_ccTableInstallPath;   // Set in OnInit(). Two .cct table files are required for support
										// of / as a word-breaking (pseudo) whitespace character, for some
										// east asian languages. When Adapt It is installed, they are stored
										// in the CC folder within the install folder (the path to the latter
										// is, on all platforms, the contents of m_xmlInstallPath member.
										// Code in OnInit() will copy these two to the _CCTABLE_INPUTS_OUTPUTS
										// folder, or update what is there if the ones here are newer
//#endif
	bool		m_bProtectReportsOutputsFolder;
	wxString	m_reportsOutputsFolderName; // in OnInit() we set to "_REPORTS_OUTPUTS"
	wxString	m_reportsOutputsFolderPath; // always a child of folder that m_curProjectPath
										// points to; the path is defined where m_curProjectPath
										// gets defined

	wxString	m_logsEmailReportsFolderName; // in OnInit() we set to "_LOGS_EMAIL_REPORTS"
	wxString	m_logsEmailReportsFolderPath; // Not in the nav protection scheme - what goes here
										// alwaiys goes here regardless of nav protection


	wxArrayString m_sortedLoadableFiles; // for use by the NavProtectNewDoc class's dialog

	/// m_appUserConfigDir stores the path (only the path, not path and name) where the
	/// wxFileConfig's (.)Adapt_It_WX(.ini) file is located beginning with version 6.0.0.
	/// m_appUserConfigDir is set by calling wxStandardPaths::GetUserConfigDir().
	/// On wxMSW: "C:\Users\Bill Martin\AppData\Roaming"
	/// On wxGTK: "/home/wmartin"
	/// On wxMac: "/Users/wmartin"
	wxString m_appUserConfigDir;


	/// m_appUserConfigDir stores the path and name of the wxFileConfig file's
	/// on-disk configuration file. It is of the form: Adapt_It_WX.ini on Windows
	/// and a .Adapt_It_WX hidden file on Linux and the Mac.
	/// On Windows the Adapt_It_WX.ini file is first used with version 6.0.0 which
	/// automatically transitions some settings previously stored in the Windows
	/// registry into the external Adapt_It_WX.ini file.
	/// m_appUserConfigDir is set by calling wxStandardPaths::GetUserConfigDir().
	/// On wxMSW: "C:\Users\Bill Martin\AppData\Roaming\Adapt_It_WX.ini"
	/// On wxGTK: "/home/wmartin/.Adapt_It_WX"
	/// On wxMac: "/Users/wmartin/.Adapt_It_WX"
	wxString m_wxFileConfigPathAndName;

	/// m_PathPrefix stores the path prefix for Linux installations as returned by
	/// GetAdaptItInstallPrefixForLinux() in helpers. For other platforms
	/// m_PathPrefix is an empty string. When the Linux version is installed from a
	/// debian package, this prefix is generally /usr/. When Linux is installed using
	/// 'sudo make install' by a developer or user building the application from source
	/// on their local machine, m_PathPrefix would generally be /usr/local/. I think
	/// there are probably ways a user could also manually determine the prefix by use
	/// of a -prefix switch during a build, so we need to be flexible enough to have the
	/// m_PathPrefix be whatever is currently in use via whatever install options are used.
	wxString m_PathPrefix; // whm added 6Dec11

	/// m_appInstallPathOnly stores the path (only the path, not path and name) where the
    /// executable application file is installed on the given platform. The value of this
	/// variable is determined by calling FindAppPath(). It doesn't end with PathSeparator
    /// at least on Windows.
    /// On wxMSW: C:\Program Files\Adapt It WX Unicode"
    /// On wxGTK: "/usr/bin/" or "/usr/local/bin/" depending on m_PathPrefix
    /// On wxMac: "/Programs/"
	wxString m_appInstallPathOnly;

    /// m_appInstallPathName stores the path and name where the executable application file
    /// is installed on the given platform.
	/// On wxMSW:   "C:\Program Files\Adapt It WX\Adapt_It.exe or
	///             C:\Program Files\Adapt It WX Unicode\Adapt_It_Unicode.exe"
	/// On wxGTK:   "/usr/bin/adaptit" or "/usr/local/bin/adaptit" depending on m_PathPrefix
	///             [adaptit is the name of the executable, not a directory]
	/// On wxMac: "/Programs/AdaptIt.app"
	wxString m_appInstallPathAndName;

	/// m_ParatextInstallDirPath stores the path to the Paratext installation
	/// directory, usually "c:\Program Files\Paratext 7".
	/// This variable is determined by calling the GetParatextInstallDirPath()
	/// function.
	wxString m_ParatextInstallDirPath;

	/// m_BibleditInstallDirPath stores the path to the Bibledit installation
	/// directory, usually "/usr/bin", but it could be different if Bibledit
	/// if found at a different location/path by searching for bibledit-gtk
	/// on the system's PATH environment variable.
	/// This variable is determined by calling the GetBibleditInstallDirPath()
	/// function.
	wxString m_BibleditInstallDirPath;

	/// m_ParatextProjectsDirPath stores the path to the Paratext user's
	/// project directory. The default location where Paratext creates the
	/// user's projects is at c:\My Paratext Projects".
	/// This variable is determined by calling the GetParatextProjectsDirPath()
	/// function.
	wxString m_ParatextProjectsDirPath;

	/// m_BibleditProjectsDirPath stores the path to the Bibledit user's
	/// project directory. Bibledit installs a .bibledit hidden folder upon
	/// installation at this location which is normally the user's home
	/// directory ~/ which would be something like /home/wmartin
	/// This variable is determined by calling the GetBibleditProjectsDirPath()
	/// function.
	wxString m_BibleditProjectsDirPath;

	/// m_xmlInstallPath stores the path where the AI_USFM.xml and books.xml files are
    /// installed on the given platform.
	/// On wxMSW:   "C:\Program Files\Adapt It WX\ or
	///             C:\Program Files\Adapt It WX Unicode\"
	/// On wxGTK:   "/usr/share/adaptit/" or "/usr/local/share/adaptit" depending on m_PathPrefix
	///             [adaptit here is the name of a directory]
	/// On wxMac:   "AdaptIt.app/Contents/Resources"
	wxString m_xmlInstallPath;	// whm added for path where the AI_USFM.xml, AI_UserProfiles.xml
								// and books.xml files are installed

    /// whm 22Feb2021 added a new App member m_dataKBsharingFolderName to hold
    /// the folder name "_DATA_KB_SHARING". The assignment of that folder name is done in
    /// the .cpp file at about line 23750. See next comment. (Formerly the folder was "dist"
	/// in a different location)
    wxString m_dataKBsharingFolderName;

    /// whm 13Feb2021 added the new App member m_dataKBsharingPath
    /// The new App member m_dataKBsharingPath is the absolute path to the
    /// "_DATA_KB_SHARING" folder for storing KB Sharing .DAT files, and their sets of
	/// either 12 .exe externally compiled and linked files, or, for Linux or OSC builds,
	/// their releated 12 python *.py files (to be run under python 3.x or higher) - 
	/// ending with PathSeparator
    /// On 22Feb2021 the team decided to place the "_DATA_KB_SHARING" folder in the
    /// "Adapt It Unicode Work" folder. The m_dataKBsharingPath location on ALL platforms 
	/// will be either:
    /// m_customWorkFolderPath + PathSeparator + _T("_DATA_KB_SHARING") + PathSeparator, or
    /// m_workFolderPath + PathSeparator +  _T("_DATA_KB_SHARING") + PathSeparator,
    /// depending on whether the user/admin has set up a custom work folder or not.
    /// The actual value for m_dataKBsharingPath has to be determined later below in 
    /// this OnInit() function after it is determined whether the work folder is
    /// a custom work folder or the normal world folder. The code that assigns the
    /// proper value for m_dataKBsharingPath is done in Adapt_It.cpp at lines 26781 and 26802.
    wxString m_dataKBsharingPath;

    /// m_localizationInstallPath stores the path where the <lang> localization files are
    /// installed on the given platform.
	/// On wxMSW:   "C:\Program Files\Adapt It WX\Languages\ or
	///             C:\Program Files\Adapt It WX Unicode\Languages\"
	/// On wxGTK:   "/usr/share/locale/"    which then contains multiple
	///             <lang>/LC_MESSAGES/adaptit.mo
	/// On wxMac:   "AdaptIt.app/Contents/Resources/locale" [bundle subdirectory]
	///             this is where Poedit puts its localization files.
	wxString m_localizationInstallPath;	// whm added for path where top level <lang>
				// localization directory is installed

	/// m_helpInstallPath stores the path where the help files are installed on the given
	/// platform.
	/// On wxMSW:   "C:\Program Files\Adapt It WX\ or
	///             C:\Program Files\Adapt It WX Unicode\"
	/// On wxGTK:   "/usr/share/adaptit/help/"  containing: common/.gif and .css
	///             containing: <lang>/  .html .hhp .hhc etc
	/// On wxMac:   "AdaptIt.app/Contents/SharedSupport" [bundle subdirectory] ???
	///             TODO: check this location
	wxString m_helpInstallPath;

	/// m_htbHelpFileName stores the actual name of the help file for the given platform.
	/// This name is of the form <appName>.htb and is stored at the m_helpInstallPath.
	/// On wxMSW: Adapt_It.htb
	/// On wxGTK: adaptit.htb
	/// On wxMac: AdaptIt.htb
	wxString m_htbHelpFileName;

	/// m_adminHelpFileName stores the actual name of the admin help file for the given platform.
	/// This name is _T("Help_for_Administrators.htm") on all platforms.
	wxString m_adminHelpFileName;

	/// m_quickStartHelpFileName stores the actual name of the Adapt_It_Quick_Start.htm help file
	/// for the given platform.
	/// This name is _T("Adapt_It_Quick_Start.htm") on all platforms.
	wxString m_quickStartHelpFileName;

	/// m_rfc5646MessageFileName stores the actual name of the RFC5646message.htm message file
	/// for the given platform.
	/// This name is _T("RFC5646message.htm") on all platforms.
	wxString m_rfc5646MessageFileName; // whm added 30Jul13 for BEW

 	/// m_GuesserExplanationMessageFileName stores the actual name of the GuesserExplanation.htm
	/// message file for the given platform.
	/// This name is _T("GuesserExplanation.htm") on all platforms.
	wxString m_GuesserExplanationMessageFileName; // BEW added 7Mar14

	/// m_licenseInstallPath stores the path where the license files are installed on the
    /// given platform.
	/// On wxMSW:   "C:\Program Files\Adapt It WX\ or C:\Program Files\Adapt It WX Unicode\"
	/// On wxGTK:   "/usr/share/doc/adaptit/license/"
	/// On wxMac:   "~/Documents"
	wxString m_licenseInstallPath; // whm added for path where license files are installed

    /// m_documentsInstallPath stores the path where the Adapt It documents are installed
    /// on the given platform.
	/// On wxMSW:   "C:\Program Files\Adapt It WX\ or C:\Program Files\Adapt It WX Unicode\"
	/// On wxGTK:   "/usr/share/doc/adaptit/docs/"
	/// On wxMac:   "~/Documents"
	wxString m_documentsInstallPath; // whm added for path where documents, reference docs,
				// etc are installed

	/// m_desktopInstallPath stores the path where the Adapt It desktop menu configuration
	/// (adaptit.desktop) file is installed on the given platform.
	/// On wxMSW:   n/a
	/// On wxGTK:   "/usr/share/applications/"
	/// On wxMac:   ???
	wxString m_desktopInstallPath; // whm added for path where adaptit.desktop file is
				// installed

	wxString	m_curKBName;
	wxString	m_curKBPath;
	wxString	m_curKBBackupPath;
	wxString	m_curGlossingKBName;	// all projects use same Glossing.KB name
	wxString	m_curGlossingKBBackupPath;
	wxString	m_curGlossingKBPath;	// same path as m_curKBPath except for
				// filename difference

	wxArrayString m_acceptedFilesList;	// for use in OnFileRestoreKB function and
                // elsewhere, such as consistency check, retranslation report, transforming
                // adaptations to glosses
	bool		m_bBackupDocument;
	bool		m_bAutoBackupKB;

	// wizard window suppression
	bool		m_bSuppressWelcome;
	bool		m_bUseStartupWizardOnLaunch; // this always remains TRUE since version 3
				// & although deprecated, we retain it for backwards compatibility in reading
				// configuration files
	bool		m_bSuppressTargetHighlighting;

	// auto insertion
	bool		m_bAutoInsert;  // when TRUE, OnIdle handler tries repetitive matching and
                // inserting provided the view's m_bSingleStep flag is FALSE; when FALSE,
                // no repetitive matching is tried.

    // whm added 22Mar2018 for detecting callers of PlaceBox()
    bool        m_bMovingToDifferentPile;

    short       m_nExpandBox;
	bool		m_bFinalGapWidthReady; // False, until set TRUE by a RecalcLayout() call.

    /// Use this multiplier to calculate when text gets too near the RHS of the phrase box, so
    /// that expansion becomes necessary - see the FixBox() function in CPhraseBox class.
    short		m_nNearEndFactor;

    /// An int for saving a source phrase's old sequence number in case it is required
    /// for the toolbar's Back button; or for saving the active location in a variety of
    /// command handlers. When there is no earlier location, it is set to -1, but you should
    /// never rely on it having the value -1 unless you know you've set -1 earlier
    int m_nOldSequNum;



    // enable or suppress RTL reading checkboxes in the fontpage, for the Wizard, and
    // preferences (for ANSI version, we want them unseen; for unicode NR version, we want
    // them seen)
	bool		m_bShowRTL_GUI;
    // boolean for the LTR or RTL reading order choice in the layout (see CreateStrip() in
    // the view class)
	bool		m_bRTL_Layout;

    /// BEW added 01Oct06, so that calling WriteProjectSettingsConfiguration() which is called
    /// at the end, now, of OnNewDocument() and OnOpenDocument(), does not get called in
    /// OnNewDocument() when m_bPassedAppInitialization is FALSE, since the latter function call
    /// is called by the framework within ProcessShellCommand() which is called from within
    /// InitInstance() - and since the m_bBookMode defaults to FALSE, and m_nBookIndex defaults
    /// to -1 at every launch, this MFC call of OnNewDocument() would otherwise unlaterally
    /// turn off book mode which was on when the app last exitted. We can't have this happen,
    /// so we use this new flag to suppress the config file write for the project settings
    /// until we are actually in the wizard (and hence passed all the initializations).
    bool m_bPassedAppInitialization;



    // auto-capitalization support, for version 2.0 and later (also a menagerie of global
    // flags defined in Adapt_ItApp.cpp)
	wxString	m_srcLowerCaseChars; // paired with m_srcUpperCaseChars, equivalences at
				// same index
	wxString	m_tgtLowerCaseChars; // paired with m_tgtUpperCaseChars, equivalences at
				// same index
	wxString	m_glossLowerCaseChars; // paired with m_glossUpperCaseChars, equivalences
				// at same index
	wxString	m_srcUpperCaseChars;
	wxString	m_tgtUpperCaseChars;
	wxString	m_glossUpperCaseChars;

	// support for Bible book folders
	bool		m_bBookMode; // default is FALSE, to get legacy app functionalities only
	wxArrayPtrVoid*	m_pBibleBooks; // MFC uses CPtrArray*
	BookNamePair*	m_pCurrBookNamePair; // currently set ptr to book name pair's struct
	int			m_nBookIndex; // 0-based index into an array of structs for the
				// (localizable) book folders
	wxString	m_bibleBooksFolderPath; // absolute path to the currently active Bible
				// book folder
	int			m_nDivSize[5];		// size of each division for the 5 radio buttons
	wxString	m_strDivLabel[5];	// radio button labels for history, wisdom, etc
	int			m_nDefaultBookIndex; // settable from an element in the  books.xml file
	int			m_nLastBookIndex;	// store last one, it is a better default when
				// turning mode back on
	bool		m_bDisableBookMode; // TRUE if the books.xml file did not parse without
				// error
	int			m_nTotalBooks;		// total count of all the book folder names in the
				// books.xml file

	// whm added 19Jan05 AI_USFM.xml file processing and USFM and Filtering
	bool		m_bUsingDefaultUsfmStyles;

	// BEW added 25Aug11, to suppress warning message about project existing coming from
	// SetupDirectories() when a Retranslation Report is being processed in collab mode
	bool		m_bRetransReportInProgress;

	// mrh added 2May12, likewise to suppress the warning when we're re-opening a document that's
	// been changed externally.
	bool		m_bDocReopeningInProgress;

	// mrh added 20Sep12, to control whether a progress dialog should come up when opening a document.
	//  Default is TRUE.
	bool		m_bShowProgress;

    // flag for skipping USFM fixed space "~" (tilde) when parsing source text (if TRUE,
    // then skip, if FALSE then don't check for its presence) used in the ParseWord()
    // function in the document class
	bool		m_bChangeFixedSpaceToRegularSpace;

	UserProfiles* m_pUserProfiles; // a struct on the heap that contains the profileVersion,
									// applicationCompatibility, adminModified, an array of
									// definedProfileNames, descriptionProfileTexts and list
									// of pointers to the UserProfileItem objects on the heap
	UserProfiles* m_pFactoryUserProfiles; // a struct on the heap that is similar to m_pUserProfiles
									// above, but always represents the factory version of the
									// user profiles since it is created in OnInit() from the
									// internal default unix-like strings in the defaultProfileItems[]
									// array. It is created by calling SetupDefaultUserProfiles()
	int			m_nWorkflowProfile; // zero-based index to the defined user workflow
									// profiles and corresponds to the tab index of any
									// selected workflow profile selected by an administrator
									// in the User Workflow Profiles dialog.
									// If the "None" profile is selected (the default), this
									// value is 0 (zero). This value is also saved in the
									// basic and project config files as "WorkflowProfile N"
									// where N is the string value of the int.
	bool m_bTemporarilyRestoreProfilesToDefaults; // Flag that is FALSE by default and at each program // whm added 14Feb12
									// launch. Its value is not saved in a config file because it should
									// revert to default of FALSE on each launch. It is set TRUE
									// on a temporary basis by toggling the Administrator menu's
									// "Temporarily restore all default user profile items..." item.
									// It can be reset to FALSE by untoggling the same menu item.
									// This feature was added at Kim B.'s request so that an
									// administrator can temporarily get default menus to be seen
									// while working on a user's computer system.
	bool m_bAiSessionExpectsUserDefinedProfile; // The m_bAiSessionExpectsUserDefinedProfile flag
									// is FALSE at initial program startup, but would be set to TRUE if,
									// when reading the basic config file, the WorkflowProfile value
									// there is a non-zero value. The flag could be set back to false
									// during the current session if the administrator were to access
									// the Administrator menu's "User Workflow Profiles..." menu and
									// change the user profile setting s back to "None".
	int m_nTempWorkflowProfile;		// A variable to save temporarily the m_nWorkflowProfile value
									// while the m_bTemporarilyRestoreProfilesToDefaults (above) is TRUE.
									// The value of m_nTempWorkflowProfile is always -1 while the value
									// of m_bTemporarilyRestoreProfilesToDefaults is FALSE. Its value is
									// set to -1 by default on each program launch in OnInit(), and its
									// value is not saved in any config file.
	AI_MenuStructure* m_pAI_MenuStructure; // a struct on the heap that conatins the default
									// AI menu structure used to reconstruct the default menu
									// structure when changing profiles or when the "None"
									// user workflow profile is selected after having used a
									// different user workflow profile.
	wxArrayPtrVoid* m_pRemovedMenuItemArray; // an array of pointers to wxMenuItem instances
									// which have been removed for the current profile
	MapMenuLabelStrToIdInt m_mapMenuLabelStrToIdInt; // map of menu string ids to the menu int ids
	MapProfileChangesToStringValues m_mapProfileChangesToStringValues; // map of profile items
									// that have changed to their string values. The map's
									// key value is:
									//    a contatenation of the string values of
									//    itemText + ":" + userProfile for UserProfileItems,
									//    for example: "Save As...:Novice"; a key may also be
									//    the string "descriptionProfileN" where N is
									//    1, 2, 3, or 4 for those top level items, for example
									//    "description3". The key maps to a variable length
									//    string for the descriptionProfileN key, and to a
									//    "1" or a "2" string for UserProfileItems visibility
									//    items.
	bool m_bShowNewProjectItem;		// If TRUE <New Project> is to be shown as first item in the
									// projectPage's list box of projects; if FALSE, the <New
									// Project> item is not included in the projectPage's list box.
									// The value is determined by the current user workflow profile's
									// visibility value for the <New Project> profile item. If the
									// <New Project> item is absent from the list box, it
									// effectively prevents the user from creating new projects.
	bool m_bUseAdaptationsGuesser;	// If TRUE(the default is FALSE) use the Guesser for adaptations
	bool m_bIsGuess;				// If TRUE there is a guess for the current target text
	int  m_nGuessingLevel;			// The guesser level (can range from 0 to 100, default is 50)
	bool m_bAllowGuesseronUnchangedCCOutput; // If TRUE the Guesser can operate on unchanged
									// Consistent Changes output; default is FALSE
	// BEW 5Dec14, added the following three (treat each as a string when storing in Project config file
	int  m_iMaxPrefixes;			// Maximum number allowed, per word, for prefixes (99 = no limit, else 0, 1 or 2, -1 = unset)
	int  m_iMaxSuffixes;			// Maximum number allowed, per word, for suffixes (99 = no limit, else 0, 1, 2 or 3, -1 = unset)

	Guesser* m_pAdaptationsGuesser;	// our Guesser object for adaptations
	Guesser* m_pGlossesGuesser;		// out Guesser object for glosses
	int m_nCorrespondencesLoadedInAdaptationsGuesser;
	int m_nCorrespondencesLoadedInGlossingGuesser;

	CGuesserAffixArray*	GetGuesserPrefixes(); // get list of prefixes (if previously input) to improve guesser performance
	size_t	FindGuesserPrefixIndex( CGuesserAffix affix ); // Find index by value
	CGuesserAffixArray*	GetGuesserSuffixes(); // get list of prefixes (if previously input) to improve guesser performance
	size_t	FindGuesserSuffixIndex( CGuesserAffix affix ); // Find index by value
	bool DoGuesserPrefixWriteToFile(wxFile* pFile = NULL); // Write Guesser prefixes to file
	bool DoGuesserSuffixWriteToFile(wxFile* pFile = NULL); // Write Guesser suffixes to file
	bool DoGuesserAffixWriteXML(wxFile* pFile, enum GuesserAffixType inGuesserAffixType); // Write Guesser Affix XML to file, file pointer required
	size_t m_numLastEntriesAggregate; // for the adapting KB, see OnIdle() near it's end for an explanation
	size_t m_numLastGlossingEntriesAggregate; // for the glossing KB

	EmailReportData* m_pEmailReportData; // EmailReportData struct used in the CEmailReportDlg class
	wxString m_aiDeveloperEmailAddresses; // email addresses of AI developers (used in EmailReportDlg.cpp)
	void	RemoveEmptiesFromMaps(CKB* pKB);

	// BEW added 20 Apr 05 in support of toggling suppression/enabling of copying of
	// source text punctuation on a CSourcePhrase instance at the active location down
	// to the word or phrase in the phrase box
	bool		m_bCopySourcePunctuation;

	// RDE: added 3Apr06 in support of calling SilEncConverters for preprocessing
	// the target word form (c.f. Consistent Changes)
    // whm: the following two need to be always available in order to interact with MFC
    // produced legacy config files which contain
	wxString	m_strSilEncConverterName;
	bool        m_bSilConverterDirForward;
	int			m_eSilConverterNormalizeOutput;

	enum SfmSet gCurrentSfmSet;	// whm 6May05. Restricted gCurrentSfmSet to indicate
                //only the current active SfmSet. It is initialized to UsfmOnly, but is
                //assigned whatever value is stored in the project config file. If there is
                //no previous project config file, it is assumed that the user has not used
                //Adapt It previously on this computer, and UsfmOnly will remain the
                //default sfm set used to create any documents. If it is the first use of
                //version 3, but a config file exists indicating the user is upgrading from
                //a previous Adapt It version, and the config file has no UseSFMarkerSet
                //recorded in the config file, the gCurrentSfmSet value and the value for
                //gProjectSfmSetForConfig are now both be set to PngOnly, because all Adapt
                //It documents created prior to version 3 will have been created with a
                //PngOnly inventory of standard format markers. See note below for
                //gProjectSfmSetForConfig.
	enum SfmSet gProjectSfmSetForConfig; // whm added 6May05. At program startup
                // gProjectSfmSetForConfig is set equal to gCurrentSfmSet. If the project
                // config file is successfully read, gProjectSfmSetForConfig and
                // gCurrentSfmSet are both set to the value last stored in the config file.
                // If, however, the user loads a document that was created with a different
                // set, gCurrentSfmSet will be set equal to the SfmSet that was active when
                // the document was last saved (as stored in the doc's Buffer member). In
                // this case, gProjectSfmSetForConfig will still retain the original config
                // SfmSet value and in any case is now always what is used to store the
                // user's explicitly chosen SfmSet in the project config file. The only
                // time gProjectSfmSetForConfig changes is when the user explicitly changes
                // the current working SfmSet to something else by means of Edit
                // Preferences USFM and Filtering tab (forcing a rebuild) or by means of the
                // Start Working... wizard USFM and Filtering wizard page (no doc open, no
                // rebuild).
	enum SfmSet gFactorySfmSet;	// will retain the factory default sfm set which is assigned
				// to be UsfmOnly
	MapSfmToUSFMAnalysisStruct*	m_pUsfmStylesMap;  // stores associations of key and ptr to
				// USFMAnalysis instances where the key is a USFM marker without the backslash
	MapSfmToUSFMAnalysisStruct* m_pPngStylesMap;   // "" for PNG styles
	MapSfmToUSFMAnalysisStruct* m_pUsfmAndPngStylesMap;   // "" for combined USFM and PNG
				// styles (USFM has priority)
	wxArrayPtrVoid*		m_pMappedObjectPointers;// array of all USFMAnalysis struct pointers
                // on the heap used for ease of deleting them in the destructor
	wxString	UsfmWrapMarkersStr;	// will hold a list of usfm wrap markers
	wxString	PngWrapMarkersStr;	// will hold a list of png wrap markers
	wxString	UsfmAndPngWrapMarkersStr;	// will hold a list of usfm and png wrap markers
	wxString	UsfmSectionHeadMarkersStr;	// will hold a list of usfm sectionHead wrap markers
	wxString	PngSectionHeadMarkersStr;	// will hold a list of png sectionHead wrap markers
	wxString	UsfmAndPngSectionHeadMarkersStr; // will hold a list of usfm and png
				// sectionHead wrap markers
	wxString	UsfmInLineMarkersStr;	// will hold a list of usfm inLine markers
	wxString	PngInLineMarkersStr;	// will hold a list of png inLine markers
	wxString	UsfmAndPngInLineMarkersStr;	// will hold a list of usfm and png inLine markers
	wxString	UsfmFilterMarkersStr;	// will hold a list of usfm filter markers
	wxString	PngFilterMarkersStr;	// will hold a list of png filter markers
	wxString	UsfmAndPngFilterMarkersStr;	// will hold a list of usfm and png filter markers

	wxString	gFactoryFilterMarkersStr;	// will retain the factory default filter
                // markers which is defined to be the contents of the UsfmFilterMarkersStr
                // as it was initially built when it was read from the AI_USFM.xml file.

    // The following two are used to keep track of Filter Markers in a similar way that the
    // gCurrentSfmSet and gProjectSfmSetForConfig above are used to keep track of user's
    // sfm set.
	wxString	gCurrentFilterMarkers; // whm added 13May05. Stores the current active list
                // of whole Filter Markers. This variable is assigned the contents of
                // UsfmFilterMarkersStr, PngFilterMarkersStr, or
                // UsfmAndPngFilterMarkersStr, depending on the currently selected sfm set.
                // If the user saved a document that was created with a modified filter
                // marker list (via changes to USFM and Filtering tab), the string value stored
                // in it will be overridden upon serialization (loading) of the doc by the
                // filter marker list stored in the doc's Buffer member, regardless of the
                // filter marker string stored in the project config file (see the next
                // variable gProjectFilterMarkersForConfig below).
	wxString	gProjectFilterMarkersForConfig; // whm added 13May05. At program startup
                // gProjectFilterMarkersForConfig is set equal to gCurrentFilterMarkers. If
                // the project config file is successfully read,
                // gProjectFilterMarkersForConfig and gCurrentFilterMarkers are both set to
                // the value last stored in the config file. If, however, the user loads a
                // document that was created with a different filter marker list,
                // gCurrentFilterMarkers will be set equal to the filter marker list that
                // was active when the document was last saved (as stored in the doc's
                // Buffer member). In this case, gProjectFilterMarkersForConfig will still
                // retain the original config filter makers list and in any case is now
                // always what is used to store the user's explicitly chosen list of filter
                // markers in the project config file. The only time
                // gProjectFilterMarkersForConfig changes is when the user explicitly
                // changes the current working set of filter markers to something else by
                // means of Edit Preferences Filter tab or by means of the Start Working...
                // wizard Filter page.

	wxString m_inlineNonbindingMarkers; // BEW 11Oct10, the begin and end markers which are
				// inline, but which do not bind more closely than punctuation
				// (these are \wj \qt \sls \tl \fig; and their endmarkers are a separate
				// wxString member -- see next line)
	bool	 m_bIsEmbeddedJmpMkr; // BEW 13Mar20 added - to differentiate \+jmp from \jmp behaviours
				// (see extended comment regarding need for this, lines 15,600-15,635 of ParsePreWord())
	wxString m_FootnoteMarkers;
	wxString m_CrossReferenceMarkers;
	wxString m_inlineNonbindingEndMarkers;
	wxString m_inlineBindingMarkers; // beginmarkers, needed for docV4 to docV5 conversions
	wxString m_usfmIndicatorMarkers; // some common beginmarkers found only in USFM
	wxString m_pngIndicatorMarkers; // some common beginmarkers found only in PNG SFM 1998 set
	// the following makes GetWholeMarker() (the one in doc class, and the other in helpers.cpp)
	// smarter about when to close off parsing a marker - in the event of bad markup (no
	// whitespace between marker end and certain things which must NEVER be in a marker)
	wxString m_forbiddenInMarkers; // set contents in OnInit()
	wxString m_prohibitiveBeginMarkers; // given contents in .cpp at 20,374++
	wxString m_prohibitiveEndMarkers;   // ditto
	bool     FindProhibitiveBeginMkr(CSourcePhrase* pSrcPhrase, wxString& beginMkr); // BEW 11Apr20
	bool	 FindProhibitiveEndMkr(CSourcePhrase* pSrcPhrase, wxString& endMkr);     // BEW 11Apr20

	bool	 m_bExt_ex_NotFiltered; //BEW 18Apr20, TRUE when unfiltered \ex ... \ex* is being parsed in
	bool	 m_bExt_ef_NotFiltered; //BEW 18Apr20, TRUE when unfiltered \ef ... \ef* is being parsed in
	bool	 m_bMkr_x_NotFiltered; //BEW 18Apr20, TRUE when unfiltered \x ... \x* is being parsed in
	bool	 m_bUnfiltering_ef_Filtered; //BEW 18Apr20, TRUE when unfiltered filtered \ef ... \ef*
	bool	 m_bUnfiltering_ex_NotFiltered; //BEW 18Apr20, TRUE when unfiltered filtered \ex ... \ex*
	wxString m_chapterVerseAttrSpan; // BEW 18Apr20, store nearest earlier ch:vs ref for error message

	// BEW 22Apr20 marker and endmarker sets for the new way to handle text colouring & TextType
	// propagation, and type-changing decisions
	wxString m_RedBeginMarkers;
	wxString m_RedEndMarkers;
	wxString m_BlueBeginMarkers; // these have few endmarkers, and are the verse & poetry sets
	wxString m_BlueEndMarkers;
	wxString m_EmbeddedIgnoreMarkers;
	wxString m_EmbeddedIgnoreEndMarkers;
	wxString m_charFormatMkrs;
	wxString m_charFormatEndMkrs;
	int	FindLastBackslash(wxString beginMkrs); // BEW 22Apr20, within m_markers string, returns offset
	wxString FindLastBeginMkr(wxString beginMkrs, int offset); // BEW 22Apr20
	wxString m_verseTypeMkrs;
	wxString m_verseTypeEndMkrs;
	wxString m_poetryTypeMkrs;
	wxString m_poetryTypeEndMkrs;
	wxString m_sectionHeadMkrs;
	wxString m_mainTitleMkrs;
	wxString m_secondaryTitleMkrs;
	// markers of type 'none' are tested for by doc.cpp IsBeginMarkerForTestTypeNone(pChar)
	wxString m_footnoteMkrs;
	wxString m_xrefMkrs;
	wxString m_headerMkrs;
	wxString m_identificationMkrs;
	wxString m_bkSlash;
	// ignore 'rightMarginReference' I can't find it in USFM3
	// custom marker \note can be ignored, such data never shows in the GUI layout
	// Useful bools for the text colouring changes
	bool    m_bTextTypeChangePending;

	// For our "cache" for recall of what was returned from TokenizeText's AI_USFM lookup for a marker
	wxString m_marker_Cached; // stored with initial backslash
	wxString m_endMarker_Cached; // stored with initial backslash
	bool	m_bInLine_Cached;
	bool	m_bSpecial_Cached;
	bool	m_bBdryOnLast_Cached;
	TextType m_bMainTextType_Cached; // use for default, ie' 'verse' for any of
				// the verse or chapter mkrs.
	// Next two are independent of USFMAnalysis struct, use for dynamic setting
	// (a) the Temp one, for things like poetry or footnotes, xrefs, extended ones
	// (b) the None or NoType one, for note, none, noType - things we want to skip
	// over without changing the TextType. When we default these two, it's to
	// what the XML defaults the TextType to - to 'verse'
	TextType m_bTempTextType_Cached;
	TextType m_bNoneOrNoType_Cached;

	void	SetDefaultTextType_Cached(); // BEW 23Apr20
	// BEW 23Apr20, copies the pAnalysis members to the cache variables *_Cached above
	void	SetTextType_Cached(USFMAnalysis* pAnalysis, TextType ttTemp = verse, TextType ttNoneOr = verse);
	TextType GetTextTypeFromBeginMkr(wxString mkr); // pass in un-augmented begin-mkr
	bool	IsNoType(wxString mkr); // pass in un-augmented begin-mkr

	//bool	m_bInform_Cached;
	//wxString m_navigationText_Cached;


	int m_nSequNumBeingViewed;	// The sequ num of the src phrase whose m_markers is
				// being viewed in the ViewFilteredMaterial dialog

	wxSize	sizeLongestSfm;	// Used to determine the text extent of the largest sfm being
                // used in the program (using main window - system font)
    wxSize	sizeSpace; // Used to determine the text extent of a space (using main window -
                // system font)
	// ***********************************
	// BEW 25Mar20 additions to support the suppression of the phrasebox jumping ahead (to next
	// hole in Drafting mode) or to next CSourcePhrase instance (in Reviewing mode). The nature
	// of the additions are to detect when certain dialogs or wxMessageBox dialogs show, with
	// a boolean. And to detect when the user used Enter or Tab to get the default button to
	// do its job. And to use these two things to cause OnePass() to consume the event without
	// moving the phrasebox forward, by a bleeding test at it start to cause control to exit
	// OnePass() before doing anything within it.
	bool m_bUserDlgOrMessageRequested; // default to FALSE, set TRUE in the InitDialog()
			// function of dialogs which need this kind of protective hack; or preceding a
			// wxMessageDialog() call which needs the same type of protection - such as
			// the dialog for right- or left association when inserting a placeholder.
			// Make sure the boolean is reset to FALSE after being so used.
	bool m_bUserHitEnterOrTab; // default FALSE, but set TRUE in the appropriate block
			// within CPhraseBox::OnKeyUp(); for user mouse clicks it remains FALSE
			// because that does not generate the spurious extra Enter or Tab event
			// which causes the phrasebox to move ahead before user can do anything
			// at the old location

	// **********************************
	// BEW added 21Jun05 for support of free translation mode
	bool	m_bFreeTranslationMode; // TRUE when free translation mode is in effect,
				// FALSE otherwise
	bool	m_bTargetIsDefaultFreeTrans; // TRUE if the user wants target text to be
                // composed into the default free translation text for a section and shown
                // in the edit box within the Compose Bar when that particular free
                // translation section is defined, FALSE if not, and default setting is
                // FALSE
	bool	m_bGlossIsDefaultFreeTrans; // TRUE if the user wants glosses text to be
                // composed into the default free translation text for a section and shown
                // in the edit box within the Compose Bar when that particular free
                // translation section is defined, FALSE if not, and default setting is
                // FALSE
    bool	m_bRestorePhraseBoxToActiveLocAfterFreeTransExited; // default FALSE, used by OnIdle()
	bool	m_bEnableDelayedFreeTransOp; // BEW 29Nov13 extensions available in Adjust dialog
				// and Split... button interfere with Drawing if the posted custom event which
				// calls their handlers results in the handler operating before the previous
				// Draw() of the view is completed (this happened once, mostly it
				// doesn't), so the posting of the events has to be done later - from the
				// OnIdle() handler. This new boolean is default FALSE, and the posting of
				// the custom events from the OnIdle() handler is then suppressed; but when
				// it is true, the code for posting the events (a switch, based on an enum)
				// gets the event for the wanted handler posted - and then the event enum
				// is reset to no-op, and this boolean cleared back to default FALSE. The result
				// of all this is that the running of the handler happens after the Draw
				// of  the view has completed, and so the handler does not clear m_pFreeTransArray
				// until after the Draw has finished needing it.
				// The use of this boolean goes hand-in-hand with the enum DelayedFreeTransOperations
				// which is defined at the top of this file - about lines 682-90
	enum DelayedFreeTransOperations m_enumWhichFreeTransOp;
	bool	m_bFreeTrans_EventPending; // TRUE when Adjust dialog has asked for an option
                // to be done (such as joining to previous), the OnIdle() block which
                // handles the event resets it to FALSE. The idea of this member boolean is
                // to prevent a second showing of the Adjust dialog when an edit operation
                // in the composebar's editbox results in two sequential events being
                // generated (two Draw() calls then each put up the Adjust dialog if the
                // text is overlong) - so I'll use this boolean to wrap the Adjust dialog
                // call, suppressing the dialog when the boolean is TRUE
	bool	m_bComposeBarWasAskedForFromViewMenu; // TRUE if the user used the Compose Bar
                // command on the View menu to open the Compose Bar (which then inhibits
                // being able to turn on free translation mode), FALSE when the Compose Bar
                // gets closed by any command, and FALSE is the default value. The OnUpdate
                // handler for the Free Translation Mode command inspects this bool value
                // to ensure the bar is not already open
	bool	m_bDefineFreeTransByPunctuation; // TRUE by default (gives smaller free
                // translation sections), if FALSE then sections are defined as the whole
                // verse (but a 'significant' USFM marker or endmarker occurring before verse
                // end will cause the section to be closed off and a new one opened)
    bool	m_bForceVerseSectioning; // FALSE by default. If the user toggles it to TRUE
				// by doing Advanced > Force Free Translation Sectioning By Verse, then
				// all free translation button handlers will commence with a function that
				// tests if m_bDefineFreeTransByPunctuation is TRUE, and if so, changes it
				// to FALSE (i.e. 'by verse' sectioning) and makes the radio buttons agree,
				// so that only 'by verse' is in effect for as long as this boolen is TRUE.
				// Also, since the document xml stores the m_bDefineFreeTransByPunctuation
				// value in its <Setting> attribute's ftsbp attribute, which gets set at
				// a File > Save, the function will also be in the file save handlers too.
				// The function is wrapped by a test of m_bForceVerseSectioning so that
				// it is called only when the flag is TRUE. (Requested by Kim Blewett, Oct2014)
	bool	m_bNotesExist; // set TRUE by OnIdle() if there are one or more Adapt It notes
				// in the document, else FALSE

	bool	m_bDocumentDestroyed; // BEW added 13Jul19 as insurance against document
				// closure and auto-saving running concurrently so that the auto-saving
				// fails to save the m_pSourcePhrases list's contents - because if that
				// happens the user's work is permanently lost, and recovery depends on
				// their being a .BAK undestroyed or a recent snapshot in GIT repository - 
				// neither of which are guaranteed to exist, especially with novice users.
				// So, if this flag goes true, DoAutoSaveDoc() will exit immediately
				// without doing any attempt to construct the xml for the doc save. The
				// flag will stay true until a new doc, previous doc, or unpack, is done.
	// BEW added 10Jan06
	bool	m_bUnpacking;	// TRUE when Unpack Document... is in progress, else FALSE
				// (used in SetupDirectories())

	// next two for read-only support....
	// the next boolean is for support of read-only protection of the data accessed in a
	// remote folder by a user on a remote machine, or a different account on local machine
	// and it works in conjunction with the ReadOnlyProtection class -- the latter determines
	// when m_bReadOnlyAccess should be set TRUE or cleared to FALSE
	bool	m_bReadOnlyAccess; // default FALSE, set TRUE if your running instances accesses
				// a remote project folder which is already "owned" by the remote user
				// (ownership is dynamically assigned to whoever accesses the project folder
				// first; a file with name ~AIROP-machinename-username.lock will be opened for
                // writing, but never written to, by whoever gets to have ownership; and is
                // closed for writing and removed when ownership of the project folder is
                // relinquished - that then gives whoever came second, and so only got
                // read-only access, the chance to re-open the project folder and so gain
                // ownership). AIROP has no deep significance other than providing a few
                // ascii characters to decrease the likelihood of an accidental name clash
                // when searching for the file within a project folder; AIROP means "Adapt
                // It Read Only Protection"
	bool	m_bFictitiousReadOnlyAccess; // default is FALSE, can be set TRUE by an advisor
				// or consultant selecting the third radio button in the ChooseCollabOptionsDlg
				// dialog. This is set TRUE when advisor/consultant/user opens a project with
				// the third radio button selected (read-only). It is only set false when the
				// project closes, or the app closes and restarts.
	wxFile* m_pROPwxFile; // will be created in OnInit(), on the heap, and killed in OnExit(),
				// we then use wxFile::Open() as needed. We do it this way because if we use
				// a local wxFile and open with a path and wxFile(), then when the local wxFile
				// goes out of scope, the wxFile is automatically closed - which we don't want
				// to happen.

	bool	m_bAutoExport; // default FALSE, set TRUE if the export command is used on launch,
				// the command line should be export followed by "project folder name" followed
				// by "document filename" followed by "path to output folder"
	bool	m_bControlIsWithinOnInit; // TRUE when OnInit() has not finished, FALSE thereafter
				// there is an extensive explanation of the need for this boolean at the end
				// of Adapt_ItDoc::OnNewDocument() - it suppresses setting read-only protection
				// while OnInit() is running
	bool	m_bForceFullConsistencyCheck;
	bool	m_bBlindFixInConsCheck; // BEW added 1Sep15, if TRUE, then if inconsistency is found
				// and there is only a single adaptation in KB for the source text, then it is
				// used in the document at that location without asking the user via a dialog
				// (requested by Mike Hore)
	int     nCount_NonDeleted; // initialize to -1 for what would be the case if pTU is NULL


public:

	// BEW added, 19Jan17, the following 5 strings in support of ParseWord2()'s
	// post-word filtering of filterable markers and their content (eg. \x,  \f, \fe ...)
	// These are used in ParsePostWordStuff() - a CAdaptItDoc member function; their
	// values are initialized to empty strings in the app method OnInit(). ParsePostWordStuff()
	// will use them to store the m_key value, the end marker, following punctuation.
	// respectively. strSearchForAfter is a scratch string for doing a post-word search.
	wxString	strAfterWord;
	wxString	strAfterEndMkr;
	wxString	strAfterPunct;
	wxString    strAfterSpace;
	wxString	strSearchForAfter;


	// arrays for storing (size_t)wxChar for when the user uses the Punctuation tab of
	// Preferences to change the project's punctuation settings, either source or
	// target or both
	wxArrayInt srcPunctsRemovedArray;
	wxArrayInt srcPunctsAddedArray;
	wxArrayInt tgtPunctsRemovedArray;
	wxArrayInt tgtPunctsAddedArray;

	// Oxes export support  BEW removed 15Jun11 until we support OXES
	// BEW 19May12 reinstated OXES support for version 1 of OXES -- abandoned 9June12 (unfinished)
	//Oxes* m_pOxes; // app creator sets to NULL, and OnInit() creates the class on the heap

	Xhtml* m_pXhtml; // app creator sets to NULL, & OnInit() creates the class on the heap

	/// BEW 25Oct11, next four used to be globals; these are for Printing Support
	bool	m_bIsPrinting;  // TRUE when OnPreparePrinting is called, cleared only in
							// the AIPrintout destructor
	bool	m_bPrintingRange; // TRUE when the user wants to print a chapter/verse range
	bool	m_bPrintingSelection;
	int		m_nCurPage; // to make current page being printed accessible to CStrip;s Draw()

    // BEW added 15Nov11
#if defined(__WXGTK__) // print-related
    bool    m_bPrintingPageRange;
    int     m_userPageRangePrintStart;
    int     m_userPageRangePrintEnd;
    // BEW added 29Nov11, so that printing a page range doesn't put into the footers page
    // numbering starting at 1, but starting from the string page requested
    int     m_userPageRangeStartPage; // (when printing, add this value less 1 to get the right page number)
#endif

	// BEW 28Oct11, added (maybe temporarily) for allowing view drawing to proceed when
	// required during display of print-related dialogs, such as the Print dialog
	bool	m_bPagePrintInProgress; // initialized to FALSE in OnInit(), set TRUE at
							// the start of OnPrintPage(), cleared back to fALSE at
							// the end of OnPrintPage(); CStrip::Draw() needs to use it

	AIPrintout* pAIPrintout; // set instance on heap when printing

	// BEW added 5Oct11, because I want to do two kludges to improve print preview
	// display, and printed strip on paper display. Print preview shows less strips than
	// the actual printed page, but on the printed page the bottom of free translation
	// overlaps about 10 pixes or so of the top of the navText whiteboard; so I want to
	// make the printed page's strip.Top() coord be progressively further down the
	// PageOffset struct, but the Print Preview page is going to need some squeezing
	// together vertically - so I need to know when the PageOffsets structs are being used
	// for page printing, or Print Preview display. This boolean allows me to do it. It is
	// set by wxPrintout's IsPreview() call. wxPrintout is the base class for AIPrintout.h
	// & .cpp
	bool m_bIsPrintPreviewing; //set at the start of OnPreparePrinting()
	bool m_bFrozenForPrinting; // set in view's PaginateDoc(), clear in ~PrintOptionsDlg()


	// Overrides
    bool	OnInit();// wxApp uses non-virtual OnInit() instead of virtual bool InitInstance()
    int		OnExit();// wxApp uses non-virtual OnExit() instead of virtual int ExitInstance()
    // OnIdle() handler moved to CMainFrame. Having it here in the App was causing File |
    // Exit and x App cancel to become unresponsive

// Declaration of event handlers

	void OnTimer(wxTimerEvent& WXUNUSED(event));
	void CheckLockFileOwnership();

	void OnFileNew(wxCommandEvent& event);
	void OnFileExportKb(wxCommandEvent& WXUNUSED(event)); // moved from view class 16Nov10
	void OnUpdateFileExportKb(wxUpdateUIEvent& event); // moved from view class 16Nov10
	void OnFileRestoreKb(wxCommandEvent& WXUNUSED(event));
	void OnFileBackupKb(wxCommandEvent& WXUNUSED(event));
	void OnToolsDefineCC(wxCommandEvent& WXUNUSED(event));
	void OnToolsUnloadCcTables(wxCommandEvent& WXUNUSED(event));

    // whm added the following two 24March2017
    void OnToolsInstallGit(wxCommandEvent& WXUNUSED(event));
    void OnUpdateInstallGit(wxUpdateUIEvent& event);


	void OnFileChangeFolder(wxCommandEvent& event);
	void OnUpdateAdvancedBookMode(wxUpdateUIEvent& event);
	void OnAdvancedBookMode(wxCommandEvent& event);
	//void OnAdvancedChangeWorkFolderLocation(wxCommandEvent& event);
	void OnUpdateAdvancedChangeWorkFolderLocation(wxUpdateUIEvent& WXUNUSED(event));

    // whm added next four 20April2017
    void OnAdvancedProtectEditorFmGettingChangesForThisDoc(wxCommandEvent& WXUNUSED(event));
    void OnUpdateAdvancedProtectEditorFmGettingChangesForThisDoc(wxUpdateUIEvent& event);
    void OnAdvancedAllowEditorToGetChangesForThisDoc(wxCommandEvent& WXUNUSED(event));
    void OnUpdateAdvancedAllowEditorToGetChangesForThisDoc(wxUpdateUIEvent& event);

	void OnFilePageSetup(wxCommandEvent& WXUNUSED(event));
	void OnUpdateFileChangeFolder(wxUpdateUIEvent& event);
	void OnUpdateFileBackupKb(wxUpdateUIEvent& event);
	void OnUpdateFileRestoreKb(wxUpdateUIEvent& event);
	void OnUpdateFileStartupWizard(wxUpdateUIEvent& event);
	void OnUpdateFilePageSetup(wxUpdateUIEvent& event);
	void OnUpdateUnloadCcTables(wxUpdateUIEvent& event);
	void OnUpdateLoadCcTables(wxUpdateUIEvent& event);

	void OnToolsClipboardAdapt(wxCommandEvent& WXUNUSED(event)); // BEW added 9May14
	void OnButtonGetFromClipboard(wxCommandEvent& WXUNUSED(event)); // BEW added 27Mar18
	void OnUpdateToolsClipboardAdapt(wxUpdateUIEvent& event); // ditto

public:

	void OnAdvancedTransformAdaptationsIntoGlosses(wxCommandEvent& WXUNUSED(event));
	void OnUpdateAdvancedTransformAdaptationsIntoGlosses(wxUpdateUIEvent& event);
	void OnToolsAutoCapitalization(wxCommandEvent& event);
	void OnUpdateToolsAutoCapitalization(wxUpdateUIEvent& event);
	void OnMakeAllKnowledgeBaseEntriesAvailable(wxCommandEvent& event);
	void OnUpdateMakeAllKnowledgeBaseEntriesAvailable(wxUpdateUIEvent& event);

	void OnUpdateCustomWorkFolderLocation(wxUpdateUIEvent& event);
	void OnCustomWorkFolderLocation(wxCommandEvent& WXUNUSED(event));
	void OnUpdateSetPassword(wxUpdateUIEvent& event);
	void OnSetPassword(wxCommandEvent& WXUNUSED(event));
	void OnUpdateRestoreDefaultWorkFolderLocation(wxUpdateUIEvent& event);
	void OnRestoreDefaultWorkFolderLocation(wxCommandEvent& WXUNUSED(event));
	void OnUpdateLockCustomLocation(wxUpdateUIEvent& event);
	void OnLockCustomLocation(wxCommandEvent& event);
	void OnUpdateUnlockCustomLocation(wxUpdateUIEvent& event);
	void OnUnlockCustomLocation(wxCommandEvent& event);
	void OnUpdateMoveOrCopyFoldersOrFiles(wxUpdateUIEvent& event);
	void OnMoveOrCopyFoldersOrFiles(wxCommandEvent& event);
	void OnAssignLocationsForInputsAndOutputs(wxCommandEvent& WXUNUSED(event));
	void OnUpdateAssignLocationsForInputsAndOutputs(wxUpdateUIEvent& event);
    void OnSetupEditorCollaboration(wxCommandEvent& WXUNUSED(event));
    void OnUpdateSetupEditorCollaboration(wxUpdateUIEvent& event);
    void OnManageDataTransfersToEditor(wxCommandEvent& WXUNUSED(event));
    void OnUpdateManageDataTransfersToEditor(wxUpdateUIEvent& event);
    void OnTempRestoreUserProfiles(wxCommandEvent& WXUNUSED(event)); // whm added 14Feb12
	void OnUpdateTempRestoreUserProfiles(wxUpdateUIEvent& event); // whm added 14Feb12
	void OnEditUserMenuSettingsProfiles(wxCommandEvent& WXUNUSED(event));
	void OnUpdateEditUserMenuSettingsProfiles(wxUpdateUIEvent& event);
	void OnHelpForAdministrators(wxCommandEvent& WXUNUSED(event));
	void OnUpdateHelpForAdministrators(wxUpdateUIEvent& event);

	void OnEditChangeUsername(wxCommandEvent& WXUNUSED(event)); // BEW added 30May13
	//void OnPasswordProtectCollaborationSwitching(wxCommandEvent& WXUNUSED(event));
	//void OnUpdatePasswordProtectCollaborationSwitching(wxUpdateUIEvent& event);

protected:

	void	AddWedgePunctPair(wxChar wedge);
	bool	DoTransformationsToGlosses(wxArrayString& tgtDocsList, CAdapt_ItDoc* pDoc,
				wxString& folderPath, wxString& bookFolderName, bool bSuppressStatistics = FALSE);
	void	FixBasicConfigPaths(enum ConfigFixType pathType, wxTextFile* pf,
						wxString& basePath, wxString& localPath);
	void	FixConfigFileFonts(wxTextFile* pf); // whm added 24Feb10
	void	GetValue(const wxString strReadIn, wxString& strValue, wxString& name);
	wxSize	GetExtentOfLongestSfm(wxDC* pDC);
	bool	IsAdaptitProjectDirectory(wxString title);
	void	MakeForeignBasicConfigFileSafe(wxString& configFName,wxString& folderPath,
											wxString* adminConfigFNamePtr);
	void	MakeForeignProjectConfigFileSafe(wxString& configFName,wxString& folderPath,
											wxString* adminConfigFNamePtr); // whm added 9Mar10
	void	PunctPairsToString(PUNCTPAIR pp[MAXPUNCTPAIRS], wxString& rStr);
	void	SetDefaults(bool bAllowCustomLocationCode = TRUE);
	void	StringToPunctPairs(PUNCTPAIR pp[MAXPUNCTPAIRS], wxString& rStr);
	void	StringToTwoPunctPairs(TWOPUNCTPAIR pp[MAXTWOPUNCTPAIRS], wxString& rStr);
	void	TwoPunctPairsToString(TWOPUNCTPAIR pp[MAXTWOPUNCTPAIRS], wxString& rStr);


#ifdef _UNICODE
	void	ChangeNRtoUnicode(wxString& s);
	void	PunctPairsToTwoStrings(PUNCTPAIR pp[MAXPUNCTPAIRS], wxString& rSrcStr, wxString& rTgtStr);
	void	TwoPunctPairsToTwoStrings(TWOPUNCTPAIR pp[MAXTWOPUNCTPAIRS], wxString& rSrcStr, wxString& rTgtStr);
	void	TwoStringsToPunctPairs(PUNCTPAIR pp[MAXPUNCTPAIRS], wxString& rSrcStr, wxString& rTgtStr);
	void	TwoStringsToTwoPunctPairs(TWOPUNCTPAIR pp[MAXTWOPUNCTPAIRS], wxString& rStr, wxString& rTgtStr);
#endif

public:
	bool IsDocumentOpen();	// edb 5Nov12 helper method to determine whether a document is currently open

// EDB 08June2012 Image processing
#define wxGetBitmapFromMemory(name) _wxGetBitmapFromMemory(name, sizeof(name))
inline wxBitmap _wxGetBitmapFromMemory(const unsigned char *data, int length) {
    wxMemoryInputStream is(data, length);
    return wxBitmap(wxImage(is, wxBITMAP_TYPE_ANY, -1), -1);
}
// end EDB

	// whm 25Sep11 added the following two functions
	bool	DocHasGlosses(SPList* pSPList);
	bool	DocHasFreeTranslations(SPList* pSPList);

    // a helper in getting ethnologue language codes from an Adapt It project config file,
    // for use in matching an AI project to a Paratext or Bibledit project pair;
    // return TRUE if all went well, FALSE if there was an error along the way
	bool	ExtractEthnologueLangCodesFromProjConfigFile(wxString& projectFolderPath,
				EthnologueCodePair* pCodePair);
	bool	GetEthnologueLangCodePairsForAIProjects(wxArrayPtrVoid* pCodePairsArray);

#ifdef _DEBUG
	// a debugging helper to send contents of UsfmFilterMarkersStr to debug window
	void	ShowFilterMarkers(int refNum); // refNum is any number I want to pass in, it is shown too
#endif

	// whm added 21Sep10 the following for user profile support
	void	BuildUserProfileXMLFile(wxTextFile* textFile);
	void	ConfigureInterfaceForUserProfile();
	void	ConfigureMenuBarForUserProfile();
	void	ConfigureModeBarForAutoCorrect(); // whm 30Aug2021 added
	void	ConfigureModeBarForUserProfile();
	void	ConfigureToolBarForUserProfile();
	void	ConfigureWizardForUserProfile();
	void	RemoveModeBarItemsFromModeBarSizer(wxSizer* pModeBarSizer);
	void	MakeMenuInitializationsAndPlatformAdjustments(); //(enum ProgramMenuMode progMenuMode);
	void	ReportMenuAndUserProfilesInconsistencies();
	bool	MenuItemIsVisibleInThisProfile(const int nProfile, const int menuItemIDint);
	bool	ModeBarItemIsVisibleInThisProfile(const int nProfile, const wxString itemLabel);
	bool	ToolBarItemIsVisibleInThisProfile(const int nProfile, const wxString itemLongStringHelp);
	bool	NewProjectItemIsVisibleInThisProfile(const int nProfile);
	wxString GetTopLevelMenuLabelForThisTopLevelMenuID(int IDint);
	wxArrayString GetBooksArrayFromBookFlagsString(wxString bookFlagsStr);
	wxString RemoveMenuLabelDecorations(wxString menuLabel);
	wxString GetMenuItemKindAsString(wxItemKind itemKind);
	wxItemKind GetMenuItemKindFromString(wxString itemKindStr);
	wxArrayPtrVoid GetMenuStructureItemsArrayForThisTopLevelMenu(AI_MainMenuItem* pMainMenuItem);
	int		GetTopLevelMenuID(const wxString topLevelMenuLabel);
	int		GetTopLevelMenuID(TopLevelMenu topLevelMenu);
	wxString GetTopLevelMenuName(TopLevelMenu topLevelMenu);
	wxMenu* GetTopLevelMenuFromAIMenuBar(TopLevelMenu topLevelMenu);
	int		GetSubMenuItemIdFromAIMenuBar(wxString mainMenuItemLabel,wxString menuItemLabel, wxMenuBar* tempMenuBar);
	int		ReplaceVisibilityStrInwxTextFile(wxTextFile* f, wxString itemTextStr, wxString profileStr, wxString valueStr);
	int		ReplaceDescriptionStrInwxTextFile(wxTextFile* f, wxString descrProfileN, wxString valueStr);
	void	UpdateAdminModifiedLineToYesOrNo(wxTextFile* f);
	void	SetupDefaultUserProfiles(UserProfiles*& pUserProfiles);
	bool	CommonItemsInProfilesDiffer(UserProfiles* compareUserProfiles, UserProfiles* baseUserProfiles);
	void	GetAndAssignIdValuesToUserProfilesStruct(UserProfiles*& pUserProfiles);
	void	GetVersionAndModInfoFromProfilesXMLFile(wxString AIuserProfilesWorkFolderPath,
				wxString& profileVersionStr,wxString& applicationCompatibilityStr,wxString& adminModifiedStr);
	bool	BackupExistingUserProfilesFileInWorkFolder(wxString AIuserProfilesWorkFolderPath, wxString& backupPathNameUsed);
	void	SetupDefaultMenuStructure(AI_MenuStructure*& pMenuStructure, MapMenuLabelStrToIdInt& m_mapMenuLabelStrToIdInt);
	void	SetupUnTranslatedMapMenuLabelStrToIdInt(MapMenuLabelStrToIdInt& m_mapMenuLabelStrToIdInt);
	void	DestroyUserProfiles(UserProfiles*& pUserProfiles);
	void	DestroyMenuStructure(AI_MenuStructure*& pMenuStructure);
	bool	SaveUserProfilesMergingDataToXMLFile(wxString fullFilePath);
	void	MapChangesInUserProfiles(UserProfiles* tempUserProfiles, UserProfiles* appUserProfiles);
	enum VersionComparison CompareRunningVersionWithWorkFolderVersion(wxString oldProfileVersion, wxString oldApplicationCompatibility);
	wxString GetAppVersionOfRunningAppAsString();
	wxString GetVersionNumberAsString(int nVersionMajor, int nVersionMinor, int nVersionRevision, wxString separator);
	wxString GetProfileVersionOfRunningAppAsString();
	// The following functions are currently unused and possibly incomplete/untested, but are left
	// here because they might contain useful material for future use:
	//bool	MenuItemExistsInAIMenuBar(wxString mainMenuLabel, wxString subMenuLabel, wxString itemKind);
	//void	AddSubMenuItemToAIMenuBar(AI_MainMenuItem* pMainMenuItem,AI_SubMenuItem* pSubMenuItem); // unused
	//void	RemoveSubMenuItemFromAIMenuBar(AI_MainMenuItem* pMainMenuItem,AI_SubMenuItem* pSubMenuItem); // unused
	//bool	AddMenuItemBeforeThisOne(AI_SubMenuItem* pMenuItemToAdd, // unused
	//										wxMenuItem* pMenuItemComingNext,
	//										bool& bAppendInsteadOfInsert);
	//wxString GetTopLevelMenuLabelForThisSubMenuID(wxString IDStr); // unused
	//wxArrayString GetMenuItemsThatFollowThisSubMenuID(wxString IDStr); // unused
	//wxArrayString GetMenuItemsThatFollowThisSubMenuID(wxString IDStr, wxString Label); // unused

	// whm added 9Feb11 (first 21 declarations) and then on 15Apr11 the ones which follow
	// those: for AI-Paratext or AI-Bibledit Collaboration (these are nearly all public)
	bool m_bParatextIsInstalled;
	bool m_bBibleditIsInstalled;
	bool m_bParatextIsRunning;
	bool m_bCollaboratingWithParatext;
	bool m_bCollaboratingWithBibledit;
	// BEW 16Sep20 during collaboration, source text .txt file copies of the PT or BE
	// source text grabbed from either, are deposited (unless user directs otherwise)
	// in the project's _SOURCE_INPUTS folder - as 'archived temp files' which collab
	// uses for mdfsums, milestoning comparisons, and whatever else. These, are not
	// to be used for loading in with New... command - if a naive user did that and made
	// a new document of them, the good .xml adapted document of the same name would be
	// overwritten, losing all it's adaptations. Gotta prevent this. So I'm going to
	// store the paths, when either of the two bools above a true, at the appropriate
	// place in the app - which appears to be a CollabUtilities.cpp line 2294 just
	// after the filePath is created (with the collab doc and .txt added at end).
	// I'll store them in a wxArrayString dedicated to this task.
	// Then, at OnExit() from the app, the array can be looked up in a loop, test for
	// the file's existence in _SOURCE_INPUTS (don't need to worry if that folder is
	// absent), and if there, use a file deletion function from wx, wxRemoveFile() I
	// presume, to remove them from the folder. There is not reason for accumulating 
	// them. By doing this when the above flags - one or both is true, I'll only be
	// deleting the temporary session ones that are no longer wanted. Anything else
	// my be there from work done outside collaboration mode, and should remain.
	wxArrayString m_arrFilePathsToRemove;

	// BEW 2Jul11, added next three booleans
	bool m_bCollaborationExpectsFreeTrans;
	bool m_bCollaborationDocHasFreeTrans;
	bool m_bSaveCopySourceFlag_For_Collaboration; // save value so it can be
												  // later restored in File / Close
	// BEW added 10Jul15 for support of conflict resolution in collaboration mode
	bool m_bRetainPTorBEversion;
	bool m_bForceAIversion;
	bool m_bConflictResolutionTurnedOn;
	bool m_bUseConflictResolutionDlg;
	wxString m_Collab_BookCode; // the 3-letter book code for the currently open collaborating document
	wxString m_Collab_LastChapterStr; // the chapter reference string (e.g.  "15", or for a chunk "3-5")
									  // last encountered when populating CollabAction structs
	// whm 27Mar12 Note:
	// If Paratext is installed (either on Windows or Linux) we give priority to
	// it as the installed external Scripture editor for collaboration. If neither
	// Paratext nor Bibledit are installed, the m_collaborationEditor is an empty
	// string. Since m_collaborationEditor can be an empty string, code should call
	// wxASSERT(!m_collaborationEditor.IsEmpty()) before using it for %s string
	// substitutions.
	wxString m_collaborationEditor;

    // whm 4Feb2020 updated Note:
    // If Paratext is installed, we need to know which major version of Paratext is
    // associated with a given AI project. String values for this member can be:
    // "PTVersion7", "PTVersion8", "PTVersion9", "PTLinuxVersion7", "PTLinuxVersion8", or "PTLinuxVersion9"
    // If no Paratext versions are installed the string value will be null _T("").
    wxString m_ParatextVersionForProject;

	// BEW 21May14, the following boolean is to support when only punctuation changes are made in PT or
	// BE's source text project; without it, the changes made it back to the AI document,
	// but preEdit and postEdit md5 sums were identical  - so the lack of different md5sum
	// values meant that GetUpdatedText_USFMsUnchanged() was used, and that meant that the
	// in PT or in BE text already there gets retained, so that the user is left wondering
	// why his punct changes don't make it back to the PT or BE adaptation project. So, we
	// will detect punct changes in the fromPT or fromBE source text merge, and set this
	// booean TRUE when at least one such change is detected. Then we'll use the TRUE
	// value of it to force calling GetUpdatedText_USFMsChanged() - which will detect
	// different md5sum values where the punctuation in the PT or BE verse differs from
	// that in the AI verse, and do the required transfer of the in AI verse material
	// overwriting that in the external editor's verse.
	bool m_bPunctChangesDetectedInSourceTextMerge;

	wxString m_CollabProjectForSourceInputs;
	wxString m_CollabProjectForTargetExports;
	wxString m_CollabProjectForFreeTransExports;
	wxString m_CollabAIProjectName; // whm added 7Sep11
	wxString m_CollabBookSelected;
	bool     m_bCollabByChapterOnly;
	wxString m_CollabChapterSelected;
	wxString m_CollabSourceLangName; // whm added 4Sep11
	wxString m_CollabTargetLangName; // whm added 4Sep11
    wxString m_CollabBooksProtectedFromSavingToEditor; // whm added 2February2017
    bool     m_bCollabDoNotShowMigrationDialogForPT7toPT8; // whm added 6April2017
	bool     m_bUserWantsNoCollabInShiftLaunch;

	bool     m_bStartWorkUsingCollaboration; // whm added 19Feb12
	wxArrayPtrVoid*	m_pArrayOfCollabProjects;
	//bool m_bEnableDelayedGetChapterHandler; // BEW added 15Sep14
	bool     m_bEnableDelayedGet_Handler; // BEW added 15Sep14

	// whm 17Oct11 for collaboration support
    wxArrayString GetListOfPTProjects(wxString PTVersion); // an override of the GetListOfPTProjects() function
	wxArrayString GetListOfBEProjects();
	wxString GetBibleditBooksPresentFlagsStr(wxString projPath);
	Collab_Project_Info_Struct* GetCollab_Project_Struct(wxString projShortName);
	wxString GetStringBetweenXMLTags(wxTextFile* f, wxString lineStr, wxString beginTag, wxString endTag);
	wxString GetBookCodeFromBookName(wxString bookName);
	wxString GetCanonTypeFromBookCode(wxString bookCode);
	wxString GetBookNameFromBookCode(wxString bookCode, wxString collabEditor);
    int GetBookFlagIndexFromFullBookName(wxString fullBookName);
	int GetNumberFromBookCodeForFileNaming(wxString bookStr);
    wxString GetBookNumberAsStrFromName(wxString bookName);
    wxString GetBookNumberAsStrFromBookCode(wxString bookCode);
    wxString GetBookCodeFastFromDiskFile(wxString pathAndName);
	wxString FindBookFileContainingThisReference(wxString folderPath, wxString reference, wxString extensionFilter);
	bool BookHasChapterAndVerseReference(wxString fileAndPath, wxString chapterStr, wxString verseStr);
	//bool SeparateChapterAndVerse(wxString chapterVerse, wxString& strChapter, wxString& strVerse);

	void	TransitionWindowsRegistryEntriesTowxFileConfig(); // whm added 2Nov10
	void	RemoveCollabSettingsFromFailSafeStorageFile(); // whm added 29Feb12
	wxString InsertEntities(wxString str); // similar to Bruce's function in XML.cpp but takes a wxString and returns a wxString
	void	LogUserAction(wxString msg);
    void    LogDocCreationData(wxString ParsedWordDataLine); // whm 6Apr2020 added
    void InventoryCollabEditorInstalls();
    bool IsThisParatextVersionInstalled(wxString PTVersion); // whm added 4Feb2020
    wxString ValidateCollabEditorAndVersionStrAgainstInstallationData(wxString &collabEditor, wxString collabEditorVerStr);
    //PTVersionsInstalled ParatextVersionInstalled(); // whm modified 20Apr2016 - Deprecated as of 4Feb2020. See IsThisParatextVersionInstalled()
    wxString GetLinuxPTVersionNumberFromPTVersionFile(wxString PTVersionFilePath);
	bool	BibleditIsInstalled(); // whm added 13Jun11
	bool	ParatextIsRunning(); // whm added 9Feb11
	bool	BibleditIsRunning(); // whm added 13Jun11

//#if defined(__WXGTK__)
	wxString GetParatextEnvVar(wxString strVariableName, wxString PTverStr); // edb added 19Mar12
//#endif
	wxString GetParatextProjectsDirPath(wxString PTVersion); // whm added 9Feb11, modified 25June2016
	wxString GetBibleditProjectsDirPath();
	wxString GetParatextInstallDirPath(wxString PTVersion); // whm modified 25June2016
	wxString GetBibleditInstallDirPath(); // whm added 13Jun11
	wxString GetAdaptit_Bibledit_rdwrtInstallDirPath(); // whm added 18Dec11
	wxString GetFileNameForCollaboration(wxString collabPrefix, wxString bookCode,
				wxString ptProjectShortName, wxString chapterNumStr, wxString extStr);
	void GetCollaborationSettingsOfAIProject(wxString projectName, wxArrayString& collabLabelsArray,
													   wxArrayString& collabSettingsArray);
    wxString GetVersificationNameFromEnumVal(int vrsEnum);
    wxString GetVersificationFileNameFromEnumVal(int vrsEnum);
	wxString GetCollabSettingsAsStringForLog();
	bool IsAIProjectOpen();
	bool AIProjectHasCollabDocs(wxString m_projectName);
	bool AIProjectIsACollabProject(wxString m_projectName);
	enum AiProjectCollabStatus GetAIProjectCollabStatus(wxString m_projectName, wxString& errorStr,
		bool& bChangeMadeToCollabSettings, wxString& errorProjects, bool& bBothPT7AndPT8InstalledPT8ProjectsWereMigrated);
    void SetFolderProtectionFlagsFromCombinedString(wxString combinedStr);

	// edb 8Aug12 - Pathway support
	bool m_bPathwayIsInstalled;
	wxString m_PathwayInstallDirPath;
	wxString GetPathwayInstallDirPath();
	bool	PathwayIsInstalled();

    // Support for clipboard-based adaptation (and free translation)
    bool     m_bClipboardAdaptMode; // BEW added 9May14, a feature suggested by Bob Eaton (SAG)
    bool	 m_bClipboardTextLoaded; // TRUE when the text is in the doc, need for update handler
    bool	 m_bADocIsLoaded; // TRUE if there was a normal doc active when the feature was invoked
    int      m_nSaveSequNumForDocRestore;
    wxString m_savedTextBoxStr; // cache the phrasebox contents here
    SPList*  m_pSavedDocForClipboardAdapt;
    void     OnUpdateButtonCopyToClipboard(wxUpdateUIEvent& event);
    void     OnButtonCopyToClipboard(wxCommandEvent& WXUNUSED(event));
    void     OnUpdateButtonCopyFreeTransToClipboard(wxUpdateUIEvent& event);
    void     OnButtonCopyFreeTransToClipboard(wxCommandEvent& WXUNUSED(event));
    // don't need an update handler for the Close button, it's always enabled
    void     OnButtonCloseClipboardAdaptDlg(wxCommandEvent& WXUNUSED(event));
    wxString m_savedDocTitle; // temporarily save Titlebar's title string here when doing clipboard adapt
                              // End of new variables for support of clipboard-based adaptation & free translation

	// BEW 16Aug18 a useful logger function - unfortunately, need to keep the
	// wxLogDebug() call outside the call, so that the correct file and function
	// and line number get reported right
	// Two versions, first for finding general location; second for drilling down to the bad spot
	void MyLogger(); // this version is useful at the start and end of functions, provided the caller indicates
					 // by means of a separate wxLogDebug() that entry or exit if the function has happened
	void MyLogger(int& sequNum, wxString& srcStr, wxString& tgt_or_glossStr,
		wxString& contents, int& width);

	// BEW added next 6 lines 10July, for storing and retrieving the USFM text strings for
	// the exported target text, and exported free translation text, when collaborating
	// with the external editor. The exports are done and stored here when the doc has
	// just been built and before the user has any chance to change something, and the
	// strings stored here are accessed at the next File / Save, used, and then replaced
	// by the post-edit exports at the File / Save time. Of course, since stored here they
	// can persist only for the life of the session, but since they need to persist only
	// from Save to Save, or doc setup to next Save, that is sufficient.
private:
	wxString m_targetTextBuffer_PreEdit;
	wxString m_freeTransTextBuffer_PreEdit;
	wxString m_sourceTextBuffer_PostEdit; // export the source text, in collaboration, to here; now public as conflict res dialog needs it
public:
	void     StoreTargetText_PreEdit(wxString s);
	void     StoreFreeTransText_PreEdit(wxString s);
	void	 StoreSourceText_PostEdit(wxString s); // accessor for storing the src text
	wxString GetStoredTargetText_PreEdit();
	wxString GetStoredFreeTransText_PreEdit();
	wxString GetStoredSourceText_PostEdit(); // accessor for getting the src text
	// end of collaboration declarations

	// BEW 7Apr20 for hidden attribute data support - exports
	bool	HasBarFirstInPunctsPattern(CSourcePhrase* pSrcPhrase); // A better test than m_bUnused == TRUE
					// because m_bUnused TRUE supports two unrelated functionalities

	// BEW added 6Aug2012 for maintaining a book name (user defined, although a suggestion
	// is offered) in each document that has a valid bookID. This is needed for XHTML and
	// Pathway exports (we support only the scripture canon, not the deuterocanon)
	wxString m_bookName_Current;

	// support for saving and restoring the selection, and clearing saved selection members
	// (the members are 3 ints, m_savedSelectionLine, m_nSavedSelectionAnchorIndex, m_savedSelectionCount)
	bool SaveSelection();
	bool RestoreSelection(bool bRestoreCCellsFlagToo = FALSE);
	void ClearSavedSelection(); // only makes the above 3 ints have the value -1

	/// BEW added 7Mar13, support for two scrolling regimes (Ross Jones requested the
	/// legacy 'box moves down, strips don't move until they must' ScrollIntoView()) - I
	/// recovered the latter's code from SVN revision 500, of 9th May 2009, which was about
	/// a month before the refactoring in early June 2009 which resulted in the until-now
	/// 'keep the box midscreen' behaviour - the latter is visually unhelpful if the
	/// computer screen is projected to a text-reviewing audience of mother-tongue speakers.
	bool m_bKeepBoxMidscreen; // default is TRUE, if set FALSE in the View page of Preferences
							  // then ScrollIntoView uses the restored legacy code; the boolean
							  // value is stored in the project config file
	// BEW added 15Dec14, for better support of stacked diacritics (requested by Martin Schumacher
	// of NTM in Laos) in email of 14Dec etc
	int m_nExtraPixelsForDiacritics; // default is 0, the legacy (pre-6.5.5) situation

	int GetMaxRangeForProgressDialog(enum ProgressDialogType progDlgType, wxString pathAndXMLFileName = wxEmptyString);
	// for localization support
	CurrLocalizationInfo ProcessUILanguageInfoFromConfig();
	bool	LocalizationFilesExist();
	wxString GetLanguageNameFromBinaryMoFile(wxString pathAndMoFileName);
	// Functions that let the user select/change Adapt It's interface language
	bool	ChooseInterfaceLanguage(enum SetInterfaceLanguage setInterface);
	bool	ReverseOkCancelButtonsForMac(wxDialog* dialog);

	// printing support
	void	DoPrintCleanup();
	bool	CalcPrintableArea_LogicalUnits(int& nPagePrintingWidthLU, int& nPagePrintingLengthLU);

	// miscellaneous (this group in alphabetical order mostly)
	void	ChangeUILanguage();
	//bool	FitWithScrolling(wxDialog* dialog, wxScrolledWindow* scrolledWindow, wxSize maxSize);
	wxString GetBasePathForLocalizationSubDirectories();
	wxString GetLocalizationMoFilePath(wxString langCode);
	wxString GetDefaultPathForXMLControlFiles();
	wxString GetDefaultPathForHelpFiles();
	int		GetFirstAvailableLanguageCodeOtherThan(const int codeToAvoid, wxArrayString keyArray);
	void	GetListOfSubDirectories(const wxString initialPath, wxArrayString &arrayStr);
	wxLanguage GetLanguageFromFullLangName(const wxString fullLangName);
	wxLanguage GetLanguageFromDirStr(const wxString dirStr, wxString &fullLangName);
	int		GetMaxIndex(); // BEW added 21Mar09
	int		GetOptimumDlgEditBoxHeight(int pointSize);
	int		GetOptimumDlgFontSize(int pointSize);
	bool	m_bLanding; // used when printing (I think)
	bool	PathHas_mo_LocalizationFile(wxString dirPath, wxString subFolderName);
	void	SaveCurrentUILanguageInfoToConfig();
	void	SaveUserDefinedLanguageInfoStringToConfig(int &wxLangCode,
				const wxString shortName, const wxString fullName,
				const wxString localizationPath);
	void	RemoveUserDefinedLanguageInfoStringFromConfig(const wxString shortName,
				const wxString fullName);
	void	UpdateFontInfoStruct(wxFont* font, fontInfo& fInfo);

	// the following are mostly in alphabetical order, but there are exceptions
	bool	AccessOtherAdaptationProject();
	bool	AreBookFoldersCreated(wxString dirPath);
	void	ClearKB(CKB* pKB, CAdapt_ItDoc* pDoc);
	bool	ContainsOrdinaryQuote(wxString s, wxChar ch);
	void	CreateBookFolders(wxString dirPath, wxArrayPtrVoid* pFolders);
	void	DoAutoSaveDoc();
	void	DoAutoSaveKB();
	bool	DoUsfmSetChanges(CUsfmFilterPageCommon* pUsfmFilterPageCommon,
				bool& bSetChanged, enum Reparse reparseDoc); // whm added 23May05; BEW added bSetChanged 12Jun05
	bool	DoPunctuationChanges(CPunctCorrespPageCommon* punctPgCommon, enum Reparse reparseDoc);
	void	DoKBBackup();
	void	DoGlossingKBBackup();
	void	DoFileOpen(); // DoFileOpen() calls OnOpenDocument() which is in the Doc
	bool	DoStartWorkingWizard(wxCommandEvent& WXUNUSED(event));
	bool	DoUsfmFilterChanges(CUsfmFilterPageCommon* pUsfmFilterPageCommon,
				enum Reparse reparseDoc); // whm revised 23May05 and 5Oct10
	size_t 	EnumerateAllDocFiles(wxArrayString& paths, wxString adaptationsFolderPath); //BEW added 7Sep15,
					// for support of removing a certain <Not In KB> in all docs & re-storing its pair to KB
	bool	EnumerateDocFiles(CAdapt_ItDoc* WXUNUSED(pDoc), wxString folderPath,
				bool bSuppressDialog = FALSE);
	bool	EnumerateDocFiles_ParametizedStore(wxArrayString& docNamesList, wxString folderPath); // BEW added 6July10
	bool	EnumerateLoadableSourceTextFiles(wxArrayString& array, wxString& folderPath,
												enum LoadabilityFilter filtered); // BEW added 6Aug10
	// BEW 15Nov21 remove, this is declared and defined, but nowhere used
	//bool    IsA_dLss_winFile(wxString absPath, wxString& candidate); // pass in absolute path (from C:\) to the file,
															// to match "candidate" value at filename start
	int		FindArrayString(const wxString& findStr, wxArrayString* strArray);
	int		FindArrayStringUsingSubString(const wxString& subStr, wxArrayString* strArray, int atPosn); // whm added 8Jul12
	wxString CleanupFilterMarkerOrphansInString(wxString strFilterMarkersSavedInDoc);
	int		FindListBoxItem(wxListBox* pListBox, wxString searchStr,
				enum SearchCaseType searchType, enum SearchStrLengthType searchStrLenType);
	int		FindListBoxItem(wxListBox* pListBox, wxString searchStr,
				enum SearchCaseType searchType, enum SearchStrLengthType searchStrLenType,
				enum StartFromType startFromType);
	void	FormatMarkerAndDescriptionsStringArray(wxClientDC* pDC,
				wxArrayString* MkrAndDescrArray, int minNumSpaces,
				wxArrayInt* pUserCanSetFilterFlags);
	bool	GetBasePointers(CAdapt_ItDoc*& pDoc, CAdapt_ItView*& pView, CPhraseBox*& pBox);
	MapSfmToUSFMAnalysisStruct* GetCurSfmMap(enum SfmSet sfmSet);
	CAdapt_ItDoc* GetDocument();	// convenience function for accessing the Doc
	CFreeTrans*	GetFreeTrans();		// convenience function for accessing the free translations manager
	CLayout*	GetLayout();		// convenience function for accessing the CLayout object
	int		GetPageOrientation();
	void	GetPossibleAdaptionDocuments(wxArrayString* pList, wxString dirPath);
	void	GetPossibleAdaptionProjects(wxArrayString* pList);
	void	GetPossibleAdaptionCollabProjects(wxArrayString* aiCollabProjectNamesArray);
	void	GetPunctuationSets(wxString& srcPunctuation, wxString& tgtPunctuation);
	// BEW 20Jul21 refactored to have a second parameter, pSrcPhrase, which will be
	// default NULL for legacy calls, but set to a valid pSrcPhrase when used in support
	// of the user clicking a target text word in the retranslation in order to get the
	// retranslation dialog opened for editing
	int		GetSafePhraseBoxLocationUsingList(CAdapt_ItView* pView);
	int		GetSafeLocationForRetransTgtClick(CAdapt_ItView* pView, CSourcePhrase* pFirstSP);
	int		m_nSavedActiveSequNum; // for use after the above GetSafe...() has returned 
	wxString m_strMyCachedActiveTgtText; // cache the target text at m_nSavedActiveSequNum in this member
	int		m_nCacheNewActiveSequNum; // cache the calculated new location after 
				// GetSafePhraseBoxLocationUsingList(m_pView, pFirstSrcPhrase) has determined it

	CAdapt_ItView* GetView();		// convenience function for accessing the View
	void	InitializeFonts(); // whm added
	void	InitializePunctuation(); // whm added
	bool	InitializeLanguageLocale(wxString shortLangName, wxString longLangName,
				wxString pathPrefix);
	bool	IsDirectoryWithin(wxString& dir, wxArrayPtrVoid*& pBooks);
    bool    IsGitInstalled(wxString& versionNumStr); // whm 29Jun2021 added ref parameter to return git version if installed


    // the following ones were added by BEW to complete JF's implementation of Split, Join
    // and Move functionalities
	bool		IsValidBookID(wxString& id);
	wxString	GetBookIDFromDoc();
	wxString	GetBookID(); // this one handles both book mode on or off
	void		AddBookIDToDoc(SPList* pSrcPhrasesList, wxString id);

	bool	LayoutAndPaginate(int& nPagePrintingWidthLU, int& nPagePrintingLengthLU);
	bool	LoadKB(bool bShowProgress);
	bool	LoadGlossingKB(bool bShowProgress);
	void	LoadGuesser(CKB* Kb);
	// BEW added 10May18 for governance of dropdown list removed KB entry reinsertion 
	// within the dropdown list on 'landing'at a new location
	bool m_bLandingBox;
	bool m_bTypedNewAdaptationInChooseTranslation; // to support getting a new adaptation into the combo list direct from ChooseTranslation() dialog
			//ChooseTranslation() new typed value to force an update in CPhraseBox::OnPhraseBoxChanged()
	void	LogDropdownState(wxString functionName, wxString fileName, int lineNumber); // BEW 17Apr18 a  
				// self-contained logger for feedback about m_bAbandonable and friends, to be used when 
				// _ABANDONABLE is #defined
	wxString SimplePunctuationRestoration(CSourcePhrase* pSrcPhrase, wxString endingStr); // BEW added 17May18
				// makes the adaptation string with saved punctuation restored, for setting m_targetStr;
				// and on 19Feb20 added the second param endingStr to handle user typed ] or ) as punctuation;
				
	bool	BuildTempDropDownComboList(CTargetUnit* pTU, wxString* pAdaption, int& matchedItem); // BEW 9May18
	wxString GetMostCommonForm(CTargetUnit* pTU, wxString* pNotInKBstr); // BEW added 21Jan15
	bool	CreateAndLoadKBs(); // whm 28Aug11 added
	wxFontEncoding MapMFCCharsetToWXFontEncoding(const int Charset);
	int		MapWXFontEncodingToMFCCharset(const wxFontEncoding fontEnc);
	int		MapWXtoMFCPaperSizeCode(wxPaperSize id);
	wxPaperSize MapMFCtoWXPaperSizeCode(int id);

	void	RemoveMarkerFromString(wxString& filterMkrStr, wxString wholeMarker);
	wxString MakeExtensionlessName(wxString anyName); // removes .xml if at end of anyName
	void	SetFontAndDirectionalityForDialogControl(wxFont* pFont, wxTextCtrl* pEdit1,
				wxTextCtrl* pEdit2, wxListBox* pListBox1, wxListBox* pListBox2,
				wxFont*& pDlgFont, bool bIsRTL = FALSE);
	void	SetFontAndDirectionalityForComboBox(wxFont* pFont, wxComboBox* pCombo,
				wxFont*& pDlgFont, bool bIsRTL = FALSE);
	void	SetFontAndDirectionalityForStatText(wxFont* pFont, int pointsize,
								wxStaticText* pStatTxt, wxFont*& pDlgFont, bool bIsRTL);
    // rde: version 3.4.1 and up, determining the correct 'encoding=' string to put in an
    // XML file takes some extra thought...
	void	GetEncodingStringForXmlFiles(CBString& aStr);
	wxUint32 m_nCodePage;

	bool	DealWithThePossibilityOfACustomWorkFolderLocation(); // BEW added 12Oct09
	bool	SaveKB(bool bAutoBackup, bool bShowProgress = true);
	bool	SaveGlossingKB(bool bAutoBackup);
	void	SetProjectDefaults(wxString projectFolderPath); // whm 25Oct13 added
	void	SetDefaultCaseEquivalences();
	void	SetAllProjectLastPathStringsToEmpty();
	void	SetLanguageNamesAndCodesStringsToEmpty();
	void	SetCollabSettingsToNewProjDefaults();
	void	SetPageOrientation(bool bIsPortrait);
	bool	SetupDirectories();
    void    RemoveOldDocCreationLogFiles();
    bool	CreateInputsAndOutputsDirectories(wxString curProjectPath, wxString& pathCreationErrors);
	void	SetupKBPathsEtc();
	void	SetupMarkerStrings();
	bool	StoreGlossingKB(bool bShoWaitDlg, bool bAutoBackup);
	bool	StoreKB(bool bShoWaitDlg, bool bAutoBackup);
	void	SubstituteKBBackup(bool bDoOnGlossingKB = FALSE);
	void	Terminate();
	void	UpdateTextHeights(CAdapt_ItView* WXUNUSED(pView));
	bool	UseSourceDataFolderOnlyForInputFiles(); // BEW created 22July10, to support
				// user-protection from folder navigation when creating a new document
				// for adaptation
	wxArrayString	GetCollabSettingsFromINIFile();
	void	SaveAppCollabSettingsToINIFile(wxString projectPathAndName);
	wxString	ReplaceColonsWithAtSymbol(wxString inputStr);
	wxString	ReplaceAtSymbolWithColons(wxString inputStr);
	void	WriteFontConfiguration(const fontInfo fi, wxTextFile* pf);
	void	WriteBasicSettingsConfiguration(wxTextFile* pf);
	void	WriteProjectSettingsConfiguration(wxTextFile* pf);
	bool	WriteConfigurationFile(wxString configFilename, wxString destinationFolder,
				ConfigFileType configType);
	void	GetProjectConfiguration(wxString projectFolderPath); // whm moved here from the Doc
	bool	GetBasicConfiguration(); // whm 20Jan08 changed signature to return bool
    // Note: The three methods below had _UNICODE and non Unicode equivalents in the MFC
    // version wxTextFile is Unicode enabled, so we should only need a single version of
    // these in the wxWidgets code.
	void	GetBasicSettingsConfiguration(wxTextFile* pf, bool& bBasicConfigHasCollabSettingsData,
					wxString& collabProjectName);
	bool	MoveCollabSettingsToProjectConfigFile(wxString collabProjName);
	bool	GetFontConfiguration(fontInfo& pfi, wxTextFile* pf);
	void	GetProjectSettingsConfiguration(wxTextFile* pf);
#ifdef _UNICODE
	void	ConvertAndWrite(wxFontEncoding WXUNUSED(eEncoding),wxFile* pFile,
				wxString& str); // for unicode conversions
	void	Convert8to16(CBString& bstr,wxString& convStr); // converts a CBString to a
				// wide char CString
	wxString	Convert8to16(CBString& bstr); // BEW added 09Aug05 - overloaded version
				// (it's more useful this way)
	CBString	Convert16to8(const wxString& str); // BEW added 10Aug05, for XML output
				// in Unicode version
#endif
// GDLC 16Sep11 DoInputConversion no longer needs bHasBOM
//	void	DoInputConversion(wxString& pBuf,const char* pbyteBuff,
//				wxFontEncoding eEncoding/*,bool WXUNUSED(bHasBOM = FALSE)*/); // for unicode conversions
// GDLC 19Nov11 Added fourth parameter for byte buffer length
// GDLC 7Dec11 Changed from returning a wxString to returning a wxChar buffer
//	void	DoInputConversion(wxString*& pBuf,const char* pbyteBuff,
	void	DoInputConversion(wxChar*& pBuf,wxUint32& bufLen, const char* pbyteBuff,
				wxFontEncoding eEncoding, size_t byteBufLen=wxNO_LEN); // for unicode conversions
//#endif
	bool	GetConfigurationFile(wxString configFilename, wxString sourceFolder, ConfigFileType configType);
	void	GetSrcAndTgtLanguageNamesFromProjectName(wxString& project, wxString& srcName,
				wxString& tgtName);

	// Getters, setters, and shorthands.  Added by JF.
    // Ultimately, these are best inlined for execution speed. (BEW - but none of these are
    // involved in speed-critical functionalities, so I'd prefer not to inline them.)
	wxString	GetCurrentBookDisplayName();
	wxString	GetCurrentBookFolderPath();
	wxString	GetCurrentBookFolderName();
	wxString	GetAdaptationsFolderDisplayName();
	wxString	GetAdaptationsFolderPath();
	wxString	GetAdaptationsFolderName();
	wxString	GetCurrentDocFileName();
	wxString	GetCurrentDocPath();
	wxString	GetCurrentDocFolderDisplayName();
	wxString	GetCurrentDocFolderPath();
	void		ChangeDocUnderlyingFileNameInPlace(wxString NewFileName);
	void		ChangeDocUnderlyingFileDetailsInPlace(wxString NewFolderPath,
					wxString NewFileName);
	void		ChangeDocUnderlyingFileDetailsInPlace(wxString NewFileName);
	bool		IsOpenDoc(wxString FolderPath, wxString FileName);
	bool		GetDocHasUnsavedChanges();
	wxString	GetBookIndicatorStringForCurrentBookFolder();
	bool		GetBookMode();
	void		SaveDocChanges();
	void		DiscardDocChanges();
	void		CloseDocDiscardingAnyUnsavedChanges();
	static SPList *LoadSourcePhraseListFromFile(wxString FilePath);
	bool		AppendSourcePhrasesToCurrentDoc(SPList *ol, wxString& curBookID,
					bool IsLastAppendUsingThisMethodRightNow);
	SPList*		GetSourcePhraseList();
	CSourcePhrase	*GetCurrentSourcePhrase();
	void		SetCurrentSourcePhrase(CSourcePhrase *sp);
	CSourcePhrase	*FindNextChapter(SPList *ol, CSourcePhrase *sp);
	static wxString	ApplyDefaultDocFileExtension(wxString s);
	void	DeleteSourcePhraseListContents(SPList *l); // deletes partner piles too,
            // one-by-one (this is inefficient, but doing an en masse deletion of the
            // partner piles would require having an overloaded
            // CLayout::DestroyPiles(PileList* list) function which we don't yet have, and
            // we'd need extra code in the SplitDialog.cpp file in order to calculate the
            // sublist list which corresponds to the one passed in to
            // DeleteSourcePhraseListContents(); so I've been lazy and sacrificed some
            // speed to avoid this, and just let the DeleteSingleSrcPhrase() call delete
            // the partner pile each time through the internal loop
	void	CascadeSourcePhraseListChange(bool DoFullScreenUpdate);// If
            // DoFullScreenUpdate, the Phrase Box is moved. Otherwise, we update internal
            // variables but don't redraw. This is useful if you are going to make a series
            // of changes and want the state to be valid between each change but you don't
            // want the screen to flash with changes as the changes are made. For example,
            // when splitting a document into multiple files, one per chapter, you may wish
            // to call this function multiple times with DoFullScreenUpdate = false, and
            // finally call it with DoFullScreenUpdate = true only after the last change.
	CSourcePhrase	*GetSourcePhraseByIndex(int Index);
	void	SetCurrentSourcePhraseByIndex(int Index);

	// BEW added 17Aug09, to support custom work folder locations
	// BEW removed 19Aug09, because it was only called in reading project config file and
	// we no longer store flag nor custom work folder path in any config file
	//bool	EnsureCustomWorkFolderPathIsSet(wxString customPath, wxString& newCustomPath,
	//											bool& bNewPathFound);
	bool	IsValidWorkFolder(wxString path);
	bool	LocateCustomWorkFolder(wxString defaultPath, wxString& returnedPath,
									bool& bUserCancelled);
	bool	m_bDoNotWriteConfigFiles; // default FALSE, TRUE to suppress config file writing
	bool	IsURI(wxString& uriPath); // return TRUE if uriPath begins with \\ or //, else FALSE
	bool	m_bFailedToRemoveCustomWorkFolderLocationFile; // set TRUE in
			// OnUnlockCustomLocation() if the CustomWorkFolderLocation file failed
			// at the ::RemoveFile() call, else FALSE
	//bool	IsConfigFileWithin(wxString path, wxString& configFilePath, bool& bIsAdminBasic); // unused

	// whm added 5Jan04 FindAppPath() from suggestion by Julian Smart in wxWidgets
	// docs re "Writing installers for wxWidgets applications"
	wxString FindAppPath(const wxString& argv0, const wxString& cwd,
				const wxString& appVariableName);
	// GDLC Added 16Apr11 from suggestion by Bill Martin in order to provide a realistic response
	// for the amount of free memory - wxGetFreeMemory() doesn't do anything useful on MacOS.
	wxMemorySize GetFreeMemory(void);

private:
	void	EnsureWorkFolderPresent();
	bool	SetupCustomWorkFolderLocation();
	void	RemoveUnwantedOldUserProfilesFiles(); // BEW added 22Apr13
public:

	bool    m_bJustKeyedBackspace; // BEW 3Sep21
#if defined(_DEBUG)
//	void	RemoveDeveloperMenuItem(); // BEW added 10Oct19 & removed same day
#endif
	// a couple of members to be used for (hopefully) limiting the CPlaceInternalPunct
	// dialog, at the one location, from being shown twice or more
	int		m_nPlacePunctDlgCallNumber; // set to 0 in OnInit() and at end of CLayout::Draw()
				// and in CAdapt_ItView::OnChar() -- the latter is because if the user is
				// typing in the phrasebox, any non-zero value of this variable would be
				// and error (because the next call of MakeTargetStringIncludingPunctuation()
				// would increase the value to 2 or more, and that would suppress using
				// whatever is passed into that function's 2nd parameter, targetStr, to
				// update pSrcPhrase->m_targetStr to whatever the user typed. A Save
				// set a non-0 value, and that was the source of what we called the
				// "Non-sticking / Truncation" bug -- the value of m_targetStr was not
				// being updated to what the user typed when the phrasebox moved on.
				// Clearing the variable to 0 in OnChar() fixes this bug, and the kludge
				// in OnIdle() can now be removed
	int		m_nCurSequNum_ForPlacementDialog;

	// BEW 9Jul14 Support enabling/disabling of ZWSP (zero width space) insertion using
	// a shift+ctrl+spacebar key combination - implemented for any focused wxTextCtrl
	bool	m_bEnableZWSPInsertion; // initialize to FALSE in OnInit()

	// variables related to the Administrator menu
	// a boolean for whether or not to show the Administrator menu
	bool		m_bShowAdministratorMenu;
	wxString	m_adminPassword; // store password here (but not "admin" the latter always
								 // will work, and is hard-coded)
	//bool		m_bPwdProtectCollabSwitching; // whm added 2Feb12
	//wxString	m_collabSwitchingPassword; // whm added 2Feb12
	bool		m_bAdminMenuRemoved; // TRUE when removed from menu bar, but still on heap
									 // FALSE when appended (and visible) on the menu bar
	wxMenu*		m_pRemovedAdminMenu; // store the removed Administrator menu here until needed
	wxString	m_adminMenuTitle; // to restore the menu to the menu bar, the function needs the
								  // title to be passed in as well, so have to save it
								  // here too
	wxString	m_pathTo_FoundBasicConfigFile; // empty, or either path to AI-BasicConfiguration.aic
										// or to AI-AdminBasicConfiguration.aic in a custom
										// work folder (IsValidWorkFolder() found it)
	bool		m_bTypeOf_FoundBasicConfigFile; // TRUE if Admin type, FALSE if normal type,
										// for the m_pathTo_FoundBasicConfigFile member;
										// value is rubbish if that string is NULL
	bool		m_bSkipBasicConfigFileCall; // always FALSE except if
					// DealWithThePossibilityOfACustomWorkFolderLocation() has called SetDefaults()
					// because the custom work folder path failed to find the required basic config
					// file at the custom work folder location; we use a TRUE value to suppress the
					// GetBasicConfiguration() call in OnInit() since SetDefaults() has done
					// the setup job for us already
	// members for support of KB search functionality added Jan-Feb 2010 by BEW
	wxArrayString m_arrSearches; // set of search strings for dialog's multiline wxTextCtrl
	wxArrayString m_arrOldSearches; // old search strings accumulated while in this project, this
					                // array (and the one above)should be cleared when the project is exitted

	// whm removed the NavProtectNewDoc* m_pNavProtectDlg pointer below after creating
    // the dialog on the stack rather than on the heap (which can sometimes lead to a crash
    // in GTK/Linux version if ShowModal() is then called).
    //// member for support of user-navigation-protection feature; the dialog is created in
	//// OnInit() whether or not this feature is being used, but only shown when
	//// appropriate; it is destroyed in OnExit()
	//NavProtectNewDoc* m_pNavProtectDlg;

    // ConsChk_Empty_NoTU_Dlg class uses these as input parameters for the dialog handler.
    // The 'adaptation' variants of the following are passed in via the creator when
    // gbIsGlossing is FALSE, when it is TRUE the 'gloss' variants are passed in. The 'not
    // in kb' one is passed in to both, but either disabled or its radio button hidden when
	// glossing mode is ON
	wxString m_modeWordAdapt; // adaptation
	wxString m_modeWordGloss; //  gloss
	wxString m_modeWordAdaptPlural; // adaptations
	wxString m_modeWordGlossPlural; //  glosses
	wxString m_modeWordAdaptPlusArticle;
	wxString m_modeWordGlossPlusArticle;
	wxString m_strNotInKB; // this one is never localizable,
				// and this KB entry type is not available in glossing mode either
	wxString m_strNoAdapt; // <no adaptation>
	wxString m_strNoGloss; // <no gloss>
	wxString m_strTitle; // Inconsistency Found
	wxString m_strAdaptationText; // Adaptation text
	wxString m_strGlossText; // Gloss text
	wxString m_consCheck_msg1; // An adaptation exists. A knowledge base entry is expected, but is absent
	wxString m_consCheck_msg2; // The knowledge base entry is %s, the document does not agree

	// For enhanced guesser rejection support
	wxString m_preGuesserStr; // set it in view's CopySourceKey() just before DoGuesser() is tried
							  // and empty it when phrasebox moves for any reason BEW added 27Nov14

	bool m_bXhtmlExportInProgress; // BEW 9Jun12

	bool m_bUsePrefixExportTypeOnFilename; // whm 9Dec11 added flag to include/exclude prefixing an
										   // export type (i.e., target_text_) on export file names
	bool m_bUseSuffixExportDateTimeOnFilename; // whm 9Dec11 added flag to include/exclude prefixing an
										       // export type (i.e., target_text_) on export file names
	bool m_bUsePrefixExportProjectNameOnFilename; // whm 21Feb12 added flag to include/exclude prefixing
											// the ai project name (i.e., Telei-English) on export file names
	bool m_bClosingDown; // defaults to FALSE, set TRUE at the start of OnExit(),
						 // used to prevent at least on update handler,
						 // CPlaceholder::OnUpdateButtonRemoveNullSrcPhrase()
						 // from trying to access a deleted CSourcePhrase instance
						 // while the app is shutting down in collab mode

	bool m_bDoLegacyLowerCaseLookup; // no longer supported, no longer used, but we'll keep it
									 // because project config files for versions 6.4.3 contain
									 // this boolean (as 1 or 0) and so we'll read it if there,
									 // but not write it anymore

	// The next flag turns on or off the copy of the src text word break character for
	// composing programmatically target text words into groups, default is TRUE
	bool m_bUseSrcWordBreak;
	// The next flag gets set TRUE if we detect that the document contains a ZWSP. We assume
	// that the presence of one implies many others, and so the language would be one of
	// Sth East Asia; and hence free translations should break at latin spaces.
	bool m_bZWSPinDoc;
	bool IsZWSPinDoc(SPList* pList); // use to set of clear m_bZWSPinDoc at doc load
									 // or tfer from PT or BE
//#if defined(FWD_SLASH_DELIM)
	// BEW 23Apr15 support / as a word-breaking character for some asian languages during
	// prepublication processing
	bool m_bFwdSlashDelimiter; // public access
	// An enum with two values, which will select which conversion to do, either fwd slash
	// insertion at punctuation (using the FwdSlashInsertAtPuncts.cct table), or removal
	// of fwd slash at punctuation (using the FwdSlashRemoveAtPuncts.ccp table). We will
	// need a custom function to do the CC processing, called DoFwdSlashConsistenChanges()
	// which will be a tweak of the view class's DoConsistentChanges(), but our new function
	// will be generic, it will take in the path to the table file, and use a large input
	// buffer, and a large output buffer, internally (DoConsistentChanges only uses 1KB buffers),
	// dynamically sized
	enum FwdSlashDelimiterSupport
	{
		insertAtPunctuation,
		removeAtPunctuation
	}; // same definition is in helpers.h, because the compiler did not pick it up from here
	   // but we'll leave it here too, for when other files besides helpers.cpp need it
//#endif
	// BEW 21May15 variables used for the support of freezing/unfreezing the canvas window
	// when a series of auto-inserts is happening. NUMINSERTS is #defined in AdaptitConstants.h
	// (currently is set at 8) Other booleans which are already defined and which will help to
	// support this feature are: m_bAutoInsert, m_bSingleStep, m_bDrafting. This feature will
	// appear first in 6.5.9 Pre-Release version (limited distribution)
	// whm 11Jun2022 removed the support for freezing the canvas window. It wasn't working
	// correctly, and there is no need for it since the dropdown phrasebox was introduced.
	//bool m_bIsFrozen;
	//bool m_bDoFreeze;
	//bool m_bSupportFreeze;
	//int  m_nInsertCount;

#if defined(SCROLLPOS) && defined(__WXGTK__)
    // BEW added 10Dec12
    //This workaround is needed to avoid wxWidgets input focus mechanism from later resetting
    // the focus to the canvas (ie. wxScrolledWindow's subclass) from within wxWidgets
    // windows.cpp's gtk_window_focus_in_callback() function, at the point in that function
    // where its code thinks our canvas window doesn't have focus and so calls the
    // DoSendFocusEvents() on the canvas. That in turn leads to scrlwing.cpp's function
    // HandleOnChildFocus() being called which internally calls its own Scroll(x_Pos, y_Pos)
    // function on the canvas - but with the y_Pos value as stored from before our
    // ScrollIntoView() function gets called - with the final effect of all this being to
    // restore the scroll car to whatever was it's position prior to ScrollIntoView() being
    // called -- and then a repaint event causes the view to be drawn at the old location
    // in the document rather than at the strip where our ScollIntoView() code worked out it
    // should be at.
    // I tried a workaround - to call SetFocus()
    // on the canvas explicitly, so as to avoid wxWidgets having to do it with a wrong y_Pos        // the canvas window the input focus explicitly here, the the DoSendFocusEvents()
    // value; but I couldn't find a place to make that work: tried at the start of
    // ScrollIntoView(), at the end of ScrollIntoView() and also at the end of canvas's
    // OnPaint() call (after it's pView->Draw() call); but the scrollPos value still got
    // clobbered every time, being reset to its old position.
    // Therefore, I conclude the problem is an architectural design flaw in wxWidgets
    // implementation of the interaction between the input focus mechanism anhd scrolling
    // support. The only thing we can do is therefore program a way around the problem, by
    // forcing a further resetting of scrollPos back to where it should be, then repainting
    // the canvas. To ensure the repositioning of the scroll car is done after wxWidgets
    // has done its worst to mess it up for us, I post the needed custom event from an OnIdle()
    // call; and use a boolean m_bAdjustScrollPos defined in the CAdapt_ItApp class to
    // indicate, by setting it TRUE, when this extra adjustment should be done.
    // So far I've identified 3 features that need this support (but only in the __WxXGTK__
    // build): (1) When opening a document in which the initial location of the phrase box
    // is below the bottom of the client window. (2) When requesting a GoTo... some other
    // part of the document beyond the bounds of the currently visible part of the document.
    // (3) When hitting the Cancel And Jump Here button in the View Translation or Glosses
    // Elsewhere In The Document dialog. (Windows and OS X builds work correctly without
    // this kludge.)
    private:

    // boolean flag refered to in the comment above
    bool m_bAdjustScrollPos;

    public:
    // getter and setter for this flag
    void SetAdjustScrollPosFlag(bool bDoAdjustment);
    bool GetAdjustScrollPosFlag();
#endif
};

DECLARE_APP(CAdapt_ItApp);
// This is used in headers to create a forward declaration of the wxGetApp function It
// creates the declaration className& wxGetApp(void). It is implemented by IMPLEMENT_APP.

#endif /* Adapt_It_h */


