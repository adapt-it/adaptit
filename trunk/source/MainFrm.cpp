/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			MainFrm.cpp
/// \author			Bill Martin
/// \date_created	05 January 2004
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public
///                 License (see license directory)
/// \description	This is the implementation for the CMainFrame class.
/// The CMainFrame class defines Adapt It's basic interface, including its menu bar,
/// tool bar, control bar, compose bar, and status bar.
/// \derivation		The CMainFrame class is derived from wxDocParentFrame and inherits
/// its support for the document/view framework.
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "MainFrm.h"
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

//#define _debugLayout

//// the next three are for wxHtmlHelpController
//#include <wx/filesys.h>
//#include <wx/fs_zip.h>
//#include <wx/help.h> //(wxWidgets chooses the appropriate help controller class)
//#include <wx/helpbase.h> //(wxHelpControllerBase class)
//#include <wx/helpwin.h> //(Windows Help controller)
//#include <wx/msw/helpchm.h> //(MS HTML Help controller)
//#include <wx/generic/helpext.h> //(external HTML browser controller)
#include <wx/html/helpctrl.h> //(wxHTML based help controller: wxHtmlHelpController)

#include <wx/docview.h>
#include <wx/filename.h>
#include <wx/tooltip.h>
#include <wx/wizard.h>
#include <wx/fileconf.h>

#ifdef __WXMSW__
#include <wx/msw/registry.h> // for wxRegKey - used in SantaFeFocus sync scrolling mechanism
#endif

#if !wxUSE_WXHTML_HELP
    #error "This program can't be built without wxUSE_WXHTML_HELP set to 1"
#endif // wxUSE_WXHTML_HELP


#include "Adapt_It.h"
#include "Adapt_It_Resources.h"
#include "Adapt_ItCanvas.h"

// wx docs say: "By default, the DDE implementation is used under Windows. DDE works within one computer only.
// If you want to use IPC between different workstations you should define wxUSE_DDE_FOR_IPC as 0 before
// including this header [<wx/ipc.h>]-- this will force using TCP/IP implementation even under Windows."
#ifdef useTCPbasedIPC
#define wxUSE_DDE_FOR_IPC 0
#endif
#include <wx/ipc.h> // for wxServer, wxClient and wxConnection

#include "Adapt_ItDoc.h"
#include "Adapt_ItView.h"
#include "PhraseBox.h"
#include "Cell.h"
#include "Pile.h"
#include "SourcePhrase.h"
#include "Layout.h"
#include "MainFrm.h"
#include "helpers.h"
#include "AdaptitConstants.h"
#include "XML.h"
#include "ComposeBarEditBox.h" // BEW added 15Nov08
#include "FreeTrans.h"
#include "scrollingwizard.h" // whm added 13Nov11 for wxScrollingWizard - need to include this here before "StartWorkingWizard.h" below
#include "StartWorkingWizard.h"
#include "EmailReportDlg.h"
#include "HtmlFileViewer.h"
#include "DVCS.h"
// includes above

/// This global is defined in Adapt_It.cpp
extern CStartWorkingWizard* pStartWorkingWizard;

extern bool gbRTL_Layout; // temporary for debugging only

/// This global is defined in Adapt_ItView.cpp (for vertical edit functionality)
extern bool gbVerticalEditInProgress;

/// This global is defined in Adapt_ItView.cpp (for vertical edit functionality)
extern bool gbEditingSourceAndDocNotYetChanged; // the View sets it to TRUE but programmatically cleared to FALSE when doc is changed

/// This global is defined in Adapt_ItView.cpp (for vertical edit functionality)
extern EditRecord gEditRecord;

/// This global is defined in Adapt_ItView.cpp (for vertical edit functionality)
extern EntryPoint gEntryPoint;

/// This global is defined in Adapt_ItView.cpp (for vertical edit functionality)
extern bool gbAdaptBeforeGloss;

/// This global is defined in Adapt_ItView.cpp (for vertical edit functionality)
extern EditStep gEditStep;

/// This global is defined in Adapt_ItView.cpp (for vertical edit functionality)
extern EntryPoint gEntryPoint;

/// This global is defined in Adapt_ItView.cpp (for vertical edit functionality)
extern wxString gOldEditBoxTextStr;

/// This global is defined in Adapt_ItView.cpp (for vertical edit functionality)
extern int		gnWasSequNum;

/// This global is defined in Adapt_ItView.cpp (for vertical edit functionality)
extern int		gnWasNumWordsInSourcePhrase;

/// This global is defined in Adapt_ItView.cpp (for vertical edit functionality)
extern bool		gbWasGlossingMode;

/// This global is defined in Adapt_ItView.cpp (for vertical edit functionality)
extern bool		gbWasFreeTranslationMode;

/// This global is defined in Adapt_ItView.cpp
extern bool		gbGlossingVisible; // TRUE makes Adapt It revert to Shoebox functionality only

/// This global is defined in Adapt_ItView.cpp
extern bool		gbVerticalEdit_SynchronizedScrollReceiveBooleanWasON;

// next two are for version 2.0 which includes the option of a 3rd line for glossing

/// This global is defined in Adapt_ItView.cpp.
extern bool		gbIsGlossing; // when TRUE, the phrase box and its line have glossing text

/// This global is defined in Adapt_ItView.cpp.
extern bool		gbGlossingUsesNavFont;

/// This global is defined in Adapt_It.cpp.
extern CAdapt_ItApp* gpApp; // if we want to access it fast

extern  bool	gbFind;
extern  bool	gbFindIsCurrent;
extern  bool	gbJustReplaced;

/// This global is defined in FindReplace.cpp.
extern  bool    gbReplaceAllIsCurrent; // for use by OnIdle() in CAdapt_ItApp

/// This global is defined in Adapt_It.cpp.
extern bool		gbAutoCaps;

/// This global is defined in Adapt_It.cpp.
extern bool		gbSourceIsUpperCase;

/// This global is defined in Adapt_It.cpp.
extern bool		gbNonSourceIsUpperCase;

/// This global is defined in Adapt_It.cpp.
extern bool		gbMatchedKB_UCentry;

/// This global is defined in Adapt_It.cpp.
extern wxChar	gcharNonSrcUC;

/// This global is defined in PhraseBox.cpp.
extern bool		gbCameToEnd; // see PhraseBox.cpp

extern  int		gnRetransEndSequNum; // sequ num of last srcPhrase in a matched retranslation
extern  bool	gbHaltedAtBoundary;
extern	bool	gbFindOrReplaceCurrent;

/// This global is defined in Adapt_ItDoc.cpp.
extern	bool	bUserCancelled;

/// This global is defined in Adapt_It.cpp.
extern	bool	gbSuppressSetup;

/// This global is defined in Adapt_It.cpp.
extern bool		gbTryingMRUOpen;

/// This global is defined in Adapt_It.cpp.
extern	bool	gbViaMostRecentFileList;

/// This global is defined in Adapt_It.cpp.
extern  bool	gbUpdateDocTitleNeeded;

/// This global is defined in PhraseBox.cpp.
extern  int	nCurrentSequNum;

/// This global is defined in Adapt_It.cpp.
extern  int	nSequNumForLastAutoSave;

/// This global is defined in Adapt_It.cpp.
extern wxHtmlHelpController* m_pHelpController;

// This global is defined in Adapt_It.cpp.
//extern wxHelpController* m_pHelpController;

#ifdef __WXMSW__
static UINT NEAR WM_SANTA_FE_FOCUS = RegisterWindowMessage(_T("SantaFeFocus"));
#endif

IMPLEMENT_CLASS(CMainFrame, wxDocParentFrame)

// These custom events are declared in MainFrm.h:
//DECLARE_EVENT_TYPE(wxEVT_Adaptations_Edit, -1)
//DECLARE_EVENT_TYPE(wxEVT_Free_Translations_Edit, -1)
//DECLARE_EVENT_TYPE(wxEVT_Back_Translations_Edit, -1)
//DECLARE_EVENT_TYPE(wxEVT_V_Collected_Back_Translations_Edit, -1)
//DECLARE_EVENT_TYPE(wxEVT_End_Vertical_Edit, -1)
//DECLARE_EVENT_TYPE(wxEVT_Cancel_Vertical_Edit, -1)
//DECLARE_EVENT_TYPE(wxEVT_Glosses_Edit, -1)

DEFINE_EVENT_TYPE(wxEVT_Adaptations_Edit)
DEFINE_EVENT_TYPE(wxEVT_Free_Translations_Edit)
DEFINE_EVENT_TYPE(wxEVT_Back_Translations_Edit)
DEFINE_EVENT_TYPE(wxEVT_End_Vertical_Edit)
DEFINE_EVENT_TYPE(wxEVT_Cancel_Vertical_Edit)
DEFINE_EVENT_TYPE(wxEVT_Glosses_Edit)

// it may also be convenient to define an event table macro for the above event types
#define EVT_ADAPTATIONS_EDIT(id, fn) \
    DECLARE_EVENT_TABLE_ENTRY( \
        wxEVT_Adaptations_Edit, id, wxID_ANY, \
        (wxObjectEventFunction)(wxEventFunction) wxStaticCastEvent( wxCommandEventFunction, &fn ), \
        (wxObject *) NULL \
    ),

#define EVT_FREE_TRANSLATIONS_EDIT(id, fn) \
    DECLARE_EVENT_TABLE_ENTRY( \
        wxEVT_Free_Translations_Edit, id, wxID_ANY, \
        (wxObjectEventFunction)(wxEventFunction) wxStaticCastEvent( wxCommandEventFunction, &fn ), \
        (wxObject *) NULL \
    ),

#define EVT_BACK_TRANSLATIONS_EDIT(id, fn) \
    DECLARE_EVENT_TABLE_ENTRY( \
        wxEVT_Back_Translations_Edit, id, wxID_ANY, \
        (wxObjectEventFunction)(wxEventFunction) wxStaticCastEvent( wxCommandEventFunction, &fn ), \
        (wxObject *) NULL \
    ),

#define EVT_END_VERTICAL_EDIT(id, fn) \
    DECLARE_EVENT_TABLE_ENTRY( \
        wxEVT_End_Vertical_Edit, id, wxID_ANY, \
        (wxObjectEventFunction)(wxEventFunction) wxStaticCastEvent( wxCommandEventFunction, &fn ), \
        (wxObject *) NULL \
    ),

#define EVT_CANCEL_VERTICAL_EDIT(id, fn) \
    DECLARE_EVENT_TABLE_ENTRY( \
        wxEVT_Cancel_Vertical_Edit, id, wxID_ANY, \
        (wxObjectEventFunction)(wxEventFunction) wxStaticCastEvent( wxCommandEventFunction, &fn ), \
        (wxObject *) NULL \
    ),

#define EVT_GLOSSES_EDIT(id, fn) \
    DECLARE_EVENT_TABLE_ENTRY( \
        wxEVT_Glosses_Edit, id, wxID_ANY, \
        (wxObjectEventFunction)(wxEventFunction) wxStaticCastEvent( wxCommandEventFunction, &fn ), \
        (wxObject *) NULL \
    ),

BEGIN_EVENT_TABLE(CMainFrame, wxDocParentFrame)
	EVT_IDLE(CMainFrame::OnIdle) // this is now used in wxWidgets instead of a virtual function
	// The following carry over from the MFC version
	EVT_MENU(ID_VIEW_TOOLBAR, CMainFrame::OnViewToolBar)
	EVT_UPDATE_UI(ID_VIEW_TOOLBAR, CMainFrame::OnUpdateViewToolBar)
	EVT_MENU(ID_VIEW_STATUS_BAR, CMainFrame::OnViewStatusBar)
	EVT_UPDATE_UI(ID_VIEW_STATUS_BAR, CMainFrame::OnUpdateViewStatusBar)
	EVT_MENU(ID_VIEW_COMPOSE_BAR, CMainFrame::OnViewComposeBar)
	EVT_UPDATE_UI(ID_VIEW_COMPOSE_BAR, CMainFrame::OnUpdateViewComposeBar)
	EVT_MENU(ID_VIEW_MODE_BAR, CMainFrame::OnViewModeBar)
	EVT_UPDATE_UI(ID_VIEW_MODE_BAR, CMainFrame::OnUpdateViewModeBar)
	EVT_MENU(ID_VIEW_SHOW_ADMIN_MENU, CMainFrame::OnViewAdminMenu)
	EVT_UPDATE_UI(ID_VIEW_SHOW_ADMIN_MENU, CMainFrame::OnUpdateViewAdminMenu)
	EVT_UPDATE_UI(IDC_CHECK_KB_SAVE, CMainFrame::OnUpdateCheckKBSave)
	EVT_UPDATE_UI(IDC_CHECK_FORCE_ASK, CMainFrame::OnUpdateCheckForceAsk)
	EVT_UPDATE_UI(IDC_CHECK_SINGLE_STEP, CMainFrame::OnUpdateCheckSingleStep)
	EVT_ACTIVATE(CMainFrame::OnActivate) // to set focus to targetbox when visible
	EVT_MENU(wxID_HELP, CMainFrame::OnAdvancedHtmlHelp)
	EVT_MENU(ID_MENU_AI_QUICK_START, CMainFrame::OnQuickStartHelp)
	//EVT_MENU(ID_ONLINE_HELP, CMainFrame::OnOnlineHelp)
	EVT_MENU(ID_REPORT_A_PROBLEM, CMainFrame::OnHelpReportAProblem)
	EVT_MENU(ID_GIVE_FEEDBACK, CMainFrame::OnHelpGiveFeedback)
	EVT_MENU(ID_HELP_USE_TOOLTIPS, CMainFrame::OnUseToolTips)
	EVT_UPDATE_UI(ID_HELP_USE_TOOLTIPS, CMainFrame::OnUpdateUseToolTips)

	// support Mike's TEST_DVCS menu item
#if defined(TEST_DVCS)
	EVT_MENU (ID_MENU_DVCS_VERSION,			CMainFrame::OnDVCS_Version)
	EVT_UPDATE_UI(ID_MENU_DVCS_VERSION,		CMainFrame::OnUpdateUseToolTips)

	EVT_MENU (ID_MENU_INIT_REPOSITORY,		CMainFrame::OnInit_Repository)
	EVT_MENU (ID_MENU_DVCS_ADD_FILE,		CMainFrame::OnDVCS_Add_File)
	EVT_MENU (ID_MENU_DVCS_ADD_ALL_FILES,	CMainFrame::OnDVCS_Add_All_Files)
	EVT_MENU (ID_MENU_DVCS_REMOVE_FILE,		CMainFrame::OnDVCS_Remove_File)
	EVT_MENU (ID_MENU_DVCS_REMOVE_PROJECT,	CMainFrame::OnDVCS_Remove_Project)
	EVT_MENU (ID_MENU_DVCS_COMMIT_FILE,		CMainFrame::OnDVCS_Commit_File)
	EVT_MENU (ID_MENU_DVCS_COMMIT_PROJECT,	CMainFrame::OnDVCS_Commit_Project)
	EVT_MENU (ID_MENU_DVCS_LOG_FILE,		CMainFrame::OnDVCS_Log_File)
	EVT_MENU (ID_MENU_DVCS_LOG_PROJECT,		CMainFrame::OnDVCS_Log_Project)
	EVT_MENU (ID_MENU_DVCS_REVERT_FILE,		CMainFrame::OnDVCS_Revert_File)
#endif

	// TODO: uncomment two event handlers below when figure out why setting tooltip time
	// disables tooltips
	//EVT_MENU(ID_HELP_SET_TOOLTIP_DELAY,CMainFrame::OnSetToolTipDelayTime)
	//EVT_UPDATE_UI(ID_HELP_SET_TOOLTIP_DELAY, CMainFrame::OnUpdateSetToolTipDelayTime)

	EVT_COMBOBOX(IDC_COMBO_REMOVALS, CMainFrame::OnRemovalsComboSelChange)

	// The following are unique to the wxWidgets version
	EVT_MENU(wxID_ABOUT, CMainFrame::OnAppAbout) // MFC handles this in CAdapt_ItApp, wxWidgets' doc/view here
	EVT_SIZE(CMainFrame::OnSize)
	EVT_CLOSE(CMainFrame::OnClose)
	EVT_MENU_RANGE(wxID_FILE1, wxID_FILE9, CMainFrame::OnMRUFile)
#ifdef __WXMSW__
	// wx version doesn't use an event handling macro for handling broadcast Windows messages;
	// instead we first override the MSWWindowProc(WXUINT nMsg, WXWPARAM wParam, WXLPARAM lParam)
	// virtual method of our wxWindow-based class (which in our case is CMainFrame);
	// We then test if the nMsg parameter is the message we need to process (WM_SANTA_FE_FOCUS)
	// and perform the necessary action if it is, or call the base class method
	// (wxDocParentFrame::MSWWindowProc(WXUINT nMsg, WXWPARAM wParam, WXLPARAM lParam))
	// otherwise.
	// The following was for MFC only:
	// ON_REGISTERED_MESSAGE(WM_SANTA_FE_FOCUS, CMainFrame::OnSantaFeFocus)
#endif

	// Our Custom Event handlers:
	EVT_ADAPTATIONS_EDIT(-1, CMainFrame::OnCustomEventAdaptationsEdit)
	EVT_FREE_TRANSLATIONS_EDIT(-1, CMainFrame::OnCustomEventFreeTranslationsEdit)
	EVT_BACK_TRANSLATIONS_EDIT(-1, CMainFrame::OnCustomEventBackTranslationsEdit)
	EVT_END_VERTICAL_EDIT(-1, CMainFrame::OnCustomEventEndVerticalEdit)
	EVT_CANCEL_VERTICAL_EDIT(-1, CMainFrame::OnCustomEventCancelVerticalEdit)
	EVT_GLOSSES_EDIT(-1, CMainFrame::OnCustomEventGlossesEdit)

END_EVENT_TABLE()

const int ScriptureReferenceFocus = 1;

/// This global boolean is used to toggle the appropriate menu selections having to do with
/// sending and receiving of sync scrolling messages, and informs functions involved in the
/// SantaFeFocus message of the state of those sync scrolling settings.
bool	gbIgnoreScriptureReference_Receive = TRUE;

/// This global boolean is used to toggle the appropriate menu selections having to do with
/// sending and receiving of sync scrolling messages, and informs functions involved in the
/// SantaFeFocus message of the state of those sync scrolling settings.
bool	gbIgnoreScriptureReference_Send = TRUE;
int		gnMatchedSequNumber = -1; // set to the sequence number when a matching ch:verse is found

/// A temporary store for parsed in AI document's list of CSourcePhrase pointers.
SPList* gpDocList = NULL;

/*******************************
*	ExtractScriptureReferenceParticulars
*
*	Returns: nothing
*
*	Parameters:
*	strSantaFeMsg	->	input string from the registry in the form "GEN 13:8"
*	str3LetterCode	<-	the 3-letter (upper case) code, such as "GEN"
*	strChapVerse	<-	the chapter:verse part of the string, such as "13:8"
*	nChapter		<-	the chapter number value
*	nVerse			<-	the verse number value
*
*	Decomposes the input message string into its parts and presents them in
*	forms relevant to possible calling functions
********************************/
void ExtractScriptureReferenceParticulars(wxString& strSantaFeMsg, wxString& str3LetterCode,
								 wxString& strChapVerse, int& nChapter, int& nVerse)
{
	str3LetterCode = strSantaFeMsg.Left(3);
	str3LetterCode.MakeUpper(); // ensure it is upper case throughout, as that's what Adapt It wants
	strChapVerse = strSantaFeMsg.Mid(4);
	int index = strChapVerse.Find(_T(':'),TRUE); // TRUE finds from end
	if (index != -1)
	{
		wxString strChapter(strChapVerse[0],index);
		wxString strVerse = strChapVerse.Mid(index + 1);
		nChapter = wxAtoi(strChapter);
		nVerse = wxAtoi(strVerse);
	}
	else
	{
		// if the reference is screwed up, extract defaults which should be safe to use
		strChapVerse = _T("1:1");
		nChapter = 1;
		nVerse = 1;
	}
}

void FormatScriptureReference(const wxString& strThreeLetterBook, int nChap, int nVerse, wxString& strMsg)
{
	wxString tempStr = _T("");
	tempStr << strThreeLetterBook << _T(' ') << nChap << _T(':') << nVerse;
	// whm note: the first parameter of an AfxFormatString...() function receives the formatted string
	// therefore we need to assign tempStr to strMsg
	strMsg = tempStr;
}

// overloaded version, as my chapter & verse stored data is already natively in the form _T("Ch:Verse")
void FormatScriptureReference(const wxString& strThreeLetterBook, const wxString& strChapVerse, wxString& strMsg)
{
	strMsg = strThreeLetterBook;
	strMsg += _T(' ');

	// the passed in strChapVerse could be someting like 7:6-12  or 2:11,12 which would result in an incorrectly
	// formed message string if we used it 'as is' - so check, and if it is a range, just the range start to
	// form the reference
	int nFound = FindOneOf(strChapVerse,_T("-,")); //int nFound = strChapVerse.FindOneOf(_T("-,"));
	if (nFound > 0)
	{
		wxString shorter = strChapVerse.Left(nFound);
		strMsg += shorter;
	}
	else
	{
		strMsg += strChapVerse;
	}
}

/*******************************
*	MakeChapVerseStrForSynchronizedScroll
*
*	Returns:	a valid ch:verse string (any range information removed, verse being the start of the range if
				the input string had a verse range in it
*
*	Parameters:
*	chapVerseStr	->	the contents of a pSrcPhrase's non-empty m_chapterVerse member (which might lack any
*						chapter information and a colon, or have 0: preceding a verse or verse range, or just
*						have a verse range, or just a verse, or chapter:verse_range, or chapter:verse)
*
*	Takes whatever is found in the m_chapterVerse member (and Adapt It permits quite a range of things!) and
*	constructs a suitable "chapter:verse" string for returning to be used as a scripture reference focus string
*	for a 'send' broadcast. For a book with verses but no chapters, a chapter number of 1 is supplied as default.
*	If a range of verses is in the passed in string, then the verse which begins the range is all that is used for
*	the verse part of the scripture reference. The function should not be called if the passed in string is empty.
*
********************************/
wxString MakeChapVerseStrForSynchronizedScroll(wxString& chapVerseStr)
{
	wxString str = chapVerseStr; // make a copy
	int nColonIndex = str.Find(_T(':'));
	if (nColonIndex == -1)
	{
		// there was no colon in the string, so it can only be a verse, or a verse range
		int offset = FindOneOf(str,_T(",-"));
		if (offset == -1)
		{
			// neither character present, so we just have a verse number
			str = _T("1:") + str;
		}
		else
		{
			// there is a range, so only accept up to the value of offset as the wanted verse number string
			wxString theVerse(str,offset);
			str = _T("1:") + theVerse;
		}
	}
	else
	{
		// there is a colon present, so we have a chapter:verse, or chapter:verse_range situation, but beware
		// because Adapt It can store 0 as the chapter number when the book lacks \c markers
		if (str[0] == _T('0'))
		{
			// the chapter number is actually a zero, so to make a valid reference we need to use a
			// chapter number of 1
			str.SetChar(0,_T('1'));

		}
		wxString firstPart = str.Left(nColonIndex + 1); // everything up to and including the colon
		wxString theRest(str,nColonIndex + 1); // everything from the character after the colon
		int offset = FindOneOf(theRest,_T(",-"));
		if (offset == -1)
		{
			// neither character present, so we just have a verse number; hence str already has the correct format
			;
		}
		else
		{
			// there is a verse range, so only accept up to the value of offset as the wanted verse number string
			wxString theVerse(theRest,offset);
			str = firstPart + theVerse;
		}
	}
	return str;
}


/*******************************
*	Get3LetterCode
*
*	Returns: true if a valid 3 letter (upper case) code was obtained, false otherwise
*
*	Parameters:
*	pList			->	a list of CSourcePhrase pointer instances (either the current document, or a list
*						constructed from parsing in an XML document and storing in a temporary SPList)
*	strBookCode		<-	the 3-letter upper case code (if the document has lower case letters, we internally
*						convert to upper case and return the latter)
*
*	Gets the 3-letter code, if possible. If it exists in an Adapt It document, it will be stored on the
*	CSourcePhrase instance which is first in the document, that is, at sequence number 0. The pList parameter
*	will be pDoc->m_pSourcePhrases when we are checking a currently open document; but when scanning through
*	all stored documents for a given project (in Adaptations folder if book mode is off, in the set of Bible
*	book folders if book mode is on), it will be a temporary SPList allocated only to allow parsing in each
*	document file cryptically (they are not displayed)
*
********************************/
bool Get3LetterCode(SPList* pList, wxString& strBookCode)
{
	wxASSERT(pList != NULL);
	CSourcePhrase* pSrcPhrase = NULL;
	SPList::Node* fpos = pList->GetFirst();
	pSrcPhrase = (CSourcePhrase*)fpos->GetData();
	wxString codeStr = pSrcPhrase->m_key;
	codeStr.MakeUpper();
	bool bIsTheRealThing = CheckBibleBookCode(gpApp->m_pBibleBooks, codeStr);
	if (bIsTheRealThing)
	{
		strBookCode = codeStr;
		return TRUE;
	}
	return FALSE;
}


// Bruce: this is what you call to cause the other applications to sync.
//void SyncScrollSend(const wxString& strThreeLetterBook, int nChap, int nVerse) // Bob's old signature
void SyncScrollSend(const wxString& strThreeLetterBook, const wxString& strChapVerse)
{
#ifdef __WXMSW__
    wxRegKey keyScriptureRef(_T("HKEY_CURRENT_USER\\Software\\SantaFe\\Focus\\ScriptureReference")); //CRegKey keyScriptureRef;
	if (!gbIgnoreScriptureReference_Send)
	{
		if( keyScriptureRef.Create())
		{
			wxString strMsg;
			//FormatScriptureReference(strThreeLetterBook, nChap, nVerse, strMsg); // Bob's function
			FormatScriptureReference(strThreeLetterBook, strChapVerse, strMsg);
			// whm note: the first parameter of SetValue() below must be _T("") to set the value to strMsg
			if( keyScriptureRef.SetValue(_T(""),strMsg) )
			{
				// before sending the message to the 'other applications', turn off receiving the messages
				// so we don't end up responding to our own message
				bool bOriginal = gbIgnoreScriptureReference_Receive;
				gbIgnoreScriptureReference_Receive = TRUE;

				// send the message to the other applications
				// the following broadcast message works for Windows only
				// TODO: Use the wx facilities for IPC to do this on Linux and the Mac
				::SendMessage (HWND_BROADCAST, WM_SANTA_FE_FOCUS, ScriptureReferenceFocus, 0);

				// restore the original setting
				gbIgnoreScriptureReference_Receive = bOriginal;
			}
		}
	}
#else
	// TODO: implement wxGTK method of IPC for sending sync scroll info here

#endif
}

// Bruce: you need to implement this stub
// BEW 12Mar07, I added strChapVerse as Adapt It stores chapter;verse eg. "12:3" as a wxString in each CSourcePhrase where
// the verse number changes, so it makes sense to pass this substring intact to my locating code
// whm modified to return bool if sync scroll received successfully
// whm 26May11 modified to speed up the multi-document search when the strChapVerse reference is to be found
// in a document other than the one currently open
bool SyncScrollReceive(const wxString& strThreeLetterBook, int nChap, int nVerse, const wxString& strChapVerse)
{
    // do what you need to do scroll to the given reference in AdaptIt
    // e.g. GEN 12:3 would be:
    //  strThreeLetterBook = 'GEN';
    //  nChap = 12
    //  nVerse = 3
	// next 3 lines are just a bit of code Bob put there so I could see when something comes in;
	// delete these 3 lines when I've implemented my custom receive code
	/*
    wxString strMsg;
    FormatScriptureReference(strThreeLetterBook, nChap, nVerse, strMsg);
    ::MessageBox(0, strMsg, _T("Received Scripture Reference"), MB_OK);
	*/
	wxString strAdaptationsFolderPath; // use this when book folder mode is off
	wxString strBookFolderPath; // use this when book folder mode is on
	wxString strBookCode; // use as a scratch variable for the code in any doc we open while scanning etc
	int theBookIndex = -1;
	bool bGotCode = FALSE; // use for the result of a Get3LetterCode() call

	CAdapt_ItDoc* pDoc = NULL;
	CAdapt_ItView* pView = NULL;
	CPhraseBox* pBox = NULL;
	bool bGotPointersSuccessfully = gpApp->GetBasePointers(pDoc, pView, pBox);

	CFreeTrans* pFreeTrans = gpApp->GetFreeTrans();
	if (pFreeTrans == NULL) bGotPointersSuccessfully = FALSE;

	CNotes* pNotes = gpApp->GetNotes();
	if (pNotes == NULL) bGotPointersSuccessfully = FALSE;

	// process the message only if valid pointers to the view, document and phrasebox classes are obtained
	if (bGotPointersSuccessfully)
	{
		// process the message only if a project is open - Adapt It determines this by both KB pointers
		// being nonnull, or the following two flags both being TRUE - we will use the latter test
		if (gpApp->m_bKBReady && gpApp->m_bGlossingKBReady)
		{
			bool bNeedMultiDocScan = FALSE;
			// what we do next depends on whether or not there is a document currently open; a non-empty
			// m_pSourcePhrases list is a sufficient test for that, provided pDoc is also not null
			if (pDoc != NULL && gpApp->m_pSourcePhrases->GetCount() != 0)
			{
				// a document is open, so first we will try matching the book, chapter and verse in it
				bGotCode = Get3LetterCode(gpApp->m_pSourcePhrases,strBookCode);

				if (!bGotCode)
				{
					bNeedMultiDocScan = TRUE; // try a multi-document scan, because we couldn't find a 3-letter code
							   // in the currently open document
				}
				else
				{
					// we obtained a 3-letter code, so now we must check if it matches the 3-letter code
					// in the passed in synch scroll message
					if (strBookCode != strThreeLetterBook)
					{
						bNeedMultiDocScan = TRUE; //goto scan; // try a multi-document scan, because the open Adapt It document does
								   // not contain data from the sending application's open Bible book
					}
					else
					{
						// the open AI document has data from the correct book, so try to find a matching
						// scripture reference within it
						gnMatchedSequNumber = FindChapterVerseLocation(gpApp->m_pSourcePhrases,
																		nChap,nVerse,strChapVerse);
						if (gnMatchedSequNumber == -1)
						{
							// the scripture reference is not in the open document, so  try a
							//  multi-document scan
							bNeedMultiDocScan = TRUE;
						}
						else
						{
							// we matched the scripture reference!  So make that location the current
							// active location (beware, it could be miles from any CSourcePhrase instance
							// in the current bundle) (this code block taken from DocPage.cpp & tweaked)
							int nFinish = 1;	// the next function was designed for retranslation use, but
												// it should work fine anywhere provided we set nFinish to 1
							bool bSetSafely;
							bSetSafely = pView->SetActivePilePointerSafely(gpApp,gpApp->m_pSourcePhrases,
												gnMatchedSequNumber,gpApp->m_nActiveSequNum,nFinish);
							bSetSafely = bSetSafely; // avoid warning (retain, as is; app can tolerate this failure)
							CPile* pPile = pView->GetPile(gnMatchedSequNumber);
							gpApp->m_pActivePile = pPile;
							CSourcePhrase* pSrcPhrase = pPile->GetSrcPhrase();
							// whm added 27May11. If the Doc was not dirty before the Jump to the new
							// reference, we will set its modified status to not dirty again after the
							// Jump (below)
							bool bWasDirty = pDoc->IsModified();
							pView->Jump(gpApp,pSrcPhrase); // jump there
							if (!bWasDirty)
								pDoc->Modify(FALSE);
						}
					} // end block for trying to find a matching scripture reference in the open document
				} // end block for checking out the open document's 3-letter book code
			} // end block for testing an open AI document

			if (bNeedMultiDocScan)
			{
				// no document is open at present, or a document is open but we failed to match either
				// the book, or chapter;verse reference within it in the above code block, so we will
				// have to scan for the book before we can attempt to find the relevant chapter:verse
				// location (note: there could be several document files having the same 3-letter book code);
				// and if a doc is open while we are doing this we leave it open until the very last possible
				// minute, when we have a matched book and scripture reference to go to, and then we would
				// close it and open the other. This has the advantage that if our scanning fails to find
				// an appropriate document file, then exiting the SynchScrollReceive() function without
				// doing anything leaves the open document open and unaffected.

				// NOTE: we cannot redirect MFC serialization to gpDocList without building a whole lot of extra
				// code; but we can do it easily in XML.cpp's AtDocEndTag() with a simple test on the above
				// bool flag; so we will test here for an *.adt file being input - we will reject these, and
				// just look inside *.xml ones

				// save it here, so we can restore it later after scanning
				wxString strSavedCurrentDirectoryPath = pDoc->GetCurrentDirectory();

				// first thing to do is to set the appropriate path to the folder having the target files. It will
				// be the Adaptations folder if book mode is OFF, but a certain book folder (we have to find out
				// which) if book mode is on. The searching within doc files is done only in the one folder; and
				// in the case of book mode being on, we may have to effect a change to a different book folder
				// from the one currently active, in order to honour the received sync scroll message
				wxString strFolderPath;
				wxString strFolderName;
				bool bOK;
				wxArrayString* pList = &gpApp->m_acceptedFilesList; // holds the document filenames to be used
				pList->Clear(); // ensure it starts empty

				if (gpApp->m_bBookMode)
				{
					// get the folder name, and the index for it (these could be different from the current
					// active folder's name, and the app's m_bBookIndex value)
					Code2BookFolderName(gpApp->m_pBibleBooks, strThreeLetterBook, strFolderName, theBookIndex);
					if (strFolderName.IsEmpty())
					{
						// could not get the folder name, so just ignore this scripture reference message
						bOK = ::wxSetWorkingDirectory(strSavedCurrentDirectoryPath); // restore old current directory
						return FALSE;
					}
					// here is the absolute path to the doc files we are intested in looking inside
					strFolderPath = gpApp->m_curAdaptionsPath + gpApp->PathSeparator + strFolderName;
				}
				else
				{
					// book mode is OFF, so use the Adaptations folder for accessing doc files
					strFolderPath = gpApp->m_curAdaptionsPath;
				}
				// set current directory
				bOK = ::wxSetWorkingDirectory(strFolderPath);
				if (!bOK)
				{
					// we shouldn't ever fail in that call, but if we do, then just abandon the sync silently
					return FALSE;
				}

				// whm revised 26May11 to use fast functions that find the xml document containing the sync
				// scroll reference without calling ReadDoc_XML() and fully opening each possible document
				// file while searching for the one with the desired reference.
				wxString docExtToSearch = _T(".xml");
				wxString foundDocWithReferenceFilePathAndName;
				wxString refToFind = strThreeLetterBook + _T(' ') + strChapVerse;
				foundDocWithReferenceFilePathAndName = gpApp->FindBookFileContainingThisReference(strFolderPath, refToFind, docExtToSearch);
				if (foundDocWithReferenceFilePathAndName.IsEmpty())
				{
					// no document with the reference was found
					bool bOK;
					bOK = ::wxSetWorkingDirectory(strSavedCurrentDirectoryPath); // restore old current directory
					// we don't expect this call to have failed
					gpApp->LogUserAction(_T("SyncScrollReceive(): ::wxSetWorkingDirectory() failed, line 734 in MainFrm.cpp"));
					wxCHECK_MSG(bOK, FALSE, _T("SyncScrollReceive(): ::wxSetWorkingDirectory() failed, line 734 in MainFrm.cpp"));
					return FALSE;
				}
				else
				{
					// We found a document with the book code, and chapter and verse reference, so
					// open it and go to the desired reference.
					// Make this the active document...
					// First we have to close off the existing document (saving it if it has been modified).
					if (gpApp->m_pSourcePhrases->GetCount() > 0)
					{
						// there is an open document to be disposed of
						bOK = ::wxSetWorkingDirectory(strSavedCurrentDirectoryPath); // restore old current directory
						bool bModified = pDoc->IsModified();
						if (bModified)
						{
							// we need to save it - don't ask the user
							wxCommandEvent uevent;
							pDoc->OnFileSave(uevent);
						}
						// make sure free translation mode is off
						if (gpApp->m_bFreeTranslationMode)
						{
							// free translation mode is on, so we must first turn it off
							wxCommandEvent uevent;
							pFreeTrans->OnAdvancedFreeTranslationMode(uevent);
						}
						// erase the document, emtpy its m_pSourcePhrases list, delete its CSourcePhrase instances, etc
						pView->ClobberDocument();
						pView->Invalidate(); // force immediate update (user sees white client area)
						gpApp->m_pLayout->PlaceBox();
					}

					// next thing to do is check if book mode is on, and if a switch of book folder is
					// required -- and if so, do the path change and set the flag which says we must
					// close the currently open document first before we make the one in gpDocList visible
					if (gpApp->m_bBookMode && !gpApp->m_bDisableBookMode)
					{
						if (gpApp->m_nBookIndex != theBookIndex)
						{
							// it's a different book folder than is currently active, so update the path; this
							// means we have to reset m_bibleBooksFolderPath, and the book index
							gpApp->m_nLastBookIndex = gpApp->m_nBookIndex;
							gpApp->m_nBookIndex = theBookIndex;
							gpApp->m_bibleBooksFolderPath = strFolderPath;
						}
						else
						{
							// the book index is unchanged, so the path to the folder is the same (that is
							// m_bibleBooksFolderPath has the correct folder path already, but the
							// document may be different or the same - we'll assume different & close the
							// current one, and open the 'other' and no harm done if it is the same one
							gpApp->m_nLastBookIndex = gpApp->m_nBookIndex;
						}
					}
					else
					{
						// legacy mode - storing in the Adaptations folder
						gpApp->m_nBookIndex = -1;
						gpApp->m_bibleBooksFolderPath.Empty();

					}
					pDoc->SetFilename(foundDocWithReferenceFilePathAndName,TRUE);
					bOK = ::wxSetWorkingDirectory(strFolderPath); // set the active folder to the path
					// we don't expect this call to have failed
					gpApp->LogUserAction(_T("SyncScrollReceive(): ::wxSetWorkingDirectory() failed, line 797 in MainFrm.cpp"));
					wxCHECK_MSG(bOK, FALSE, _T("SyncScrollReceive(): ::wxSetWorkingDirectory() failed, line 797 in MainFrm.cpp"));

					// copy & tweak code from DocPage.cpp for getting the document open and view set up
					// with the phrase box at the wanted sequence number
					bOK = pDoc->OnOpenDocument(foundDocWithReferenceFilePathAndName);
					if (!bOK)
					{
						// IDS_LOAD_DOC_FAILURE
						wxMessageBox(_(
"Sorry, loading the document failed. (The file may be in use by another application. Or the file has become corrupt and must be deleted.)"),
						_T(""), wxICON_ERROR);
						bool bOK;
						bOK = ::wxSetWorkingDirectory(strSavedCurrentDirectoryPath); // restore old current directory
						// we don't expect this call to have failed
						gpApp->LogUserAction(_T("SyncScrollReceive(): ::wxSetWorkingDirectory() failed, line 813 in MainFrm.cpp"));
						wxCHECK_MSG(bOK, FALSE, _T("SyncScrollReceive(): ::wxSetWorkingDirectory() failed, line 813 in MainFrm.cpp"));
						return FALSE;
					}
					if (gpApp->nLastActiveSequNum >= (int)gpApp->m_pSourcePhrases->GetCount())
						gpApp->nLastActiveSequNum = gpApp->m_pSourcePhrases->GetCount() - 1;

					// whm added 26May11
					gnMatchedSequNumber = FindChapterVerseLocation(gpApp->m_pSourcePhrases,nChap,nVerse,strChapVerse);
					if (gnMatchedSequNumber == -1)
					{
						// this shouldn't happen
						wxASSERT(FALSE);
						bool bOK;
						bOK = ::wxSetWorkingDirectory(strSavedCurrentDirectoryPath); // restore old current directory
						// we don't expect this call to have failed
						gpApp->LogUserAction(_T("SyncScrollReceive(): ::wxSetWorkingDirectory() failed, line 829 in MainFrm.cpp"));
						wxCHECK_MSG(bOK, FALSE, _T("SyncScrollReceive(): ::wxSetWorkingDirectory() failed, line 829 in MainFrm.cpp"));
						return FALSE;
					}

					// whm added 26May11 - update frame's new document name in the main frame's title: <document name> - Adapt It Unicode
					wxDocTemplate* pTemplate = pDoc->GetDocumentTemplate();
					wxASSERT(pTemplate != NULL);
					wxString typeName, title;
					typeName = pTemplate->GetDescription(); // should be "Adapt It" or "Adapt It Unicode"
					wxFileName fn(foundDocWithReferenceFilePathAndName);
					title = fn.GetFullName() + _T(" - ") + typeName;
					gpApp->GetMainFrame()->SetTitle(title);

					gpApp->nLastActiveSequNum = gnMatchedSequNumber;
					CPile* pPile = pView->GetPile(gpApp->nLastActiveSequNum);
					wxASSERT(pPile != NULL);
					int nFinish = 1;
					// initialize m_nActiveSequNum to the nLastActiveSequNum value
					gpApp->m_nActiveSequNum = gpApp->nLastActiveSequNum;
					bool bSetSafely;
					bSetSafely = pView->SetActivePilePointerSafely(gpApp,gpApp->m_pSourcePhrases,
										gpApp->nLastActiveSequNum,gpApp->m_nActiveSequNum,nFinish);
					bSetSafely = bSetSafely; // avoid warnings TODO: check for failures?
					// m_nActiveSequNum might have been changed by the
					// preceding call, so reset the active pile
					pPile = pView->GetPile(gpApp->m_nActiveSequNum);
					gpApp->m_pActivePile = pPile;
					CSourcePhrase* pSrcPhrase = pPile->GetSrcPhrase();
					pView->Jump(gpApp,pSrcPhrase); // jump there
					pDoc->Modify(FALSE); // whm added 27May11 - newly opened doc is not dirty on opening
					gpApp->RefreshStatusBarInfo();
					return TRUE; // don't continue in the loop any longer
				}

			} // end of else block for doing a scan of all doc files
		}
	}
	// if we've gotten here we have not successfully the sync scroll message
	return FALSE;
}

// BEW 9July10, no changes needed for support of kbVersion 2
int FindChapterVerseLocation(SPList* pDocList, int nChap, int nVerse, const wxString& strChapVerse)
{
    // NOTE: in Adapt It, chapterless Bible books use 0 as the chapter number in the
    // ch:verse string, so we must test for nChap == 1 and if so, also check for a match of
    // 0:verse
	int sequNum = -1;
	bool bSecondTest = FALSE;
	int nOffset = -1;
	int nOffset2 = -1;
	wxString strAlternateRef;
	wxString refStr;
	if (nChap == 1)
	{
		bSecondTest = TRUE;
		strAlternateRef = strChapVerse;
		strAlternateRef.SetChar(0,_T('0')); // produce the "0:verse" string for the alternative test
	}

	SPList::Node* pos = pDocList->GetFirst();
	CSourcePhrase* pSrcPhrase = NULL;
	while (pos != NULL)
	{
		pSrcPhrase = pos->GetData();
		pos = pos->GetNext();
		if (!pSrcPhrase->m_chapterVerse.IsEmpty())
		{
			// there is content in the m_chapterVerse member, so check it out
			refStr = pSrcPhrase->m_chapterVerse; // copy to local wxString

            // the first of the test battery is to try find an initial ch:verse, or 0:verse
            // if the bSecondTest flag is TRUE, at the start of the refStr - if we succeed,
            // we need go no further
			nOffset = refStr.Find(strChapVerse); // returns -1 if not found
			if (bSecondTest)
			{
				nOffset2 = refStr.Find(strAlternateRef);
			}
			if (nOffset != -1 || (bSecondTest && nOffset2 != -1))
			{
				// we have a match
				sequNum = pSrcPhrase->m_nSequNumber;
				return sequNum; // this is where Adapt It has to put the active location
			}

			// we failed so far, it might be because refStr contains a verse range such as 12-15
			// or 3,5; so check for a range and if there is one do the second battery of tests
			int nFound = FindOneOf(refStr,_T("-,"));
			if (nFound != -1)
			{
				// probably its a range, so do the extra tests...
				// first, check we have a matching chapter reference
				int curPos = refStr.Find(_T(':'));
				if (curPos == -1) continue; // no colon: bogus ch:verse ref in CSourcePhrase, so iterate
				wxString aChapterNumberStr(refStr,curPos);
				int aChapterNumber = wxAtoi(aChapterNumberStr);
				if (nChap != aChapterNumber && nChap != 1) continue; // no match of the non-1 chapter
				if (nChap == 1 && (aChapterNumber != 1 || aChapterNumber != 0)) continue; // no match
					// of chapter 1 with a 1 or 0 in the m_chapterVerse member's chapter number
				if (aChapterNumber > nChap) return -1; // we can't possibly match, so indicate failure

				// if control gets to here, we've matched the chapter successfully,
				// so now try for a match with the verse range
				int nFirstVerse;
				int nLastVerse;
				wxString rangeStr = refStr.Mid(curPos + 1);
				int count = 0;
				int index = 0;
				while (wxIsdigit(rangeStr[index]) > 0)
				{
					// if the character at index is a digit, count it & increment index & iterate
					count++;
					index++;
				}
				wxString firstStr = rangeStr.Left(count);
				nFirstVerse = wxAtoi(firstStr);
				rangeStr = MakeReverse(rangeStr);
				count = index = 0;
				while (wxIsdigit(rangeStr[index]) > 0)
				{
					// if the character at index is a digit, count it & increment index & iterate
					count++;
					index++;
				}
				wxString lastStr = rangeStr.Left(count);
				lastStr = MakeReverse(lastStr);
				nLastVerse = wxAtoi(lastStr);

				// test for nVerse to be within the range, if it is, we have a match, if not
				// then continue iterating
				if (nVerse >= nFirstVerse && nVerse <= nLastVerse)
				{
					sequNum = pSrcPhrase->m_nSequNumber;
					return sequNum;
				}
			}
		}
	}
	return sequNum; // if control exits here, the value is still -1
}

// return TRUE if the passed in code matches one of the codes in the book array's stored struct pointers, else FALSE
bool CheckBibleBookCode(wxArrayPtrVoid* pBooksArray, wxString& str3LetterCode)
{
	wxString aBookCode;
	for (int i = 0; i < gpApp->m_nTotalBooks - 1; i++)
	{
		aBookCode = ((BookNamePair*)(*pBooksArray)[i])->bookCode;
		if (aBookCode == str3LetterCode)
			return TRUE;
	}
	return FALSE; // no match was found
}

// pass in a valid 3-letter book code as the second parameter, the function then returns the
// "dirName" member of the BookNamePair struct (ie. folder name as used by AI when the folder was created)
// in the third parameter; the first parameter is the array of BookNamePair structs from books.xml;
// returns an empty string if the code could not be matched to the contents of one of the structs.
void Code2BookFolderName(wxArrayPtrVoid* pBooksArray, const wxString& strBookCode, wxString& strFolderName,
						 int& nBookIndex)
{
	wxString aBookCode;
	strFolderName = _T(""); // start with empty string
	for (int i = 0; i < gpApp->m_nTotalBooks - 1; i++)
	{
		aBookCode = ((BookNamePair*)(*pBooksArray)[i])->bookCode;
		if (aBookCode == strBookCode)
		{
			strFolderName = ((BookNamePair*)(*pBooksArray)[i])->dirName;
			nBookIndex = i; // return the folder's book index too
			return;
		}
	}
	// if control gets to here, no match was made and so an empty string is returned
	nBookIndex = -1;
}

/* whm removed 27May11 - no longer needed
void DeleteSourcePhrases_ForSyncScrollReceive(CAdapt_ItDoc* pDoc, SPList* pList)
{
	CSourcePhrase* pSrcPhrase = NULL;
	if (pList != NULL)
	{
		if (!pList->IsEmpty())
		{
			// delete all the tokenizations of the source text
			SPList::Node* pos = pList->GetFirst();
			while (pos != NULL)
			{
				pSrcPhrase = pos->GetData();
				pos = pos->GetNext();
				pDoc->DeleteSingleSrcPhrase(pSrcPhrase,FALSE); // ignore partner piles
					// because this function works only on a temporary list of
					// CSourcePhrase instances and so there never are any partner piles
			}
			pList->Clear();
		}
	}
}
*/

#ifdef __WXMSW__
bool CMainFrame::DoSantaFeFocus(WXWPARAM wParam, WXLPARAM WXUNUSED(lParam))
{
	bool bMatchedBookCode = FALSE;
    if (!gbIgnoreScriptureReference_Receive && (wParam == ScriptureReferenceFocus))
    {
        // HKEY_CURRENT_USER\Software\SantaFe\Focus\ScriptureReference
        wxRegKey keyScriptureRef(_T("HKEY_CURRENT_USER\\Software\\SantaFe\\Focus\\ScriptureReference"));
        if( keyScriptureRef.Open(wxRegKey::Read) )
        {
			wxString lpValue;
			// whm note: the first parameter in QueryValue needs to be _T("") to retrieve the lpValue
            keyScriptureRef.QueryValue(_T(""),lpValue);
            wxString strValue = lpValue;
			/* Bob's original code
            wxString strThreeLetterBook(lpValue, 3);
            int nIndex = strValue.ReverseFind(':');
            if( nIndex != -1 )
            {
                wxString strChapter(&lpValue[4], nIndex - 4);
                wxString strVerse(&lpValue[nIndex + 1]);
                int nChap = _ttoi(strChapter);
                int nVerse = _ttoi(strVerse);
                SyncScrollReceive(strThreeLetterBook, nChap, nVerse);
            }
			*/
			wxString strThreeLetterBook;
			wxString strChapVerse;
			int nChapter;
			int nVerse;
			ExtractScriptureReferenceParticulars(strValue,strThreeLetterBook,strChapVerse,nChapter,nVerse);
			wxASSERT(gpApp->m_pBibleBooks != NULL);
			bMatchedBookCode = CheckBibleBookCode(gpApp->m_pBibleBooks, strThreeLetterBook);
			if (bMatchedBookCode)
			{
				// attempt the synch only if we have a known valid 3 letter code
				// whm changed SyncScrollReceive to return a bool of TRUE if successful otherwise FALSE
				return SyncScrollReceive(strThreeLetterBook, nChapter, nVerse, strChapVerse);
			}
        }
    }
    return FALSE;
}
#endif

#ifdef __WXMSW__
// In order to handle Windows broadcast messages in a wxWidgets program we must
// override the virtual MSWWindowProc() method of a wxWindow-derived class (here CMainFrame).
// We then test if the nMsg parameter is the message we need to process (WM_SANTA_FE_FOCUS)
// and perform the necessary action if it is, or call the base class method otherwise.
// TODO: Implement a wx system for doing IPC on Linux and Mac (Checkout Bibledit's use
// of its Bibledit Windows Outpost, a separate app that functions as a communication
// interface using a TCP/IP interface listening on port 51515, to enable Linux and Windows
// programs to be able to exchange info, do sync scrolling etc.)
WXLRESULT CMainFrame::MSWWindowProc(WXUINT nMsg, WXWPARAM wParam, WXLPARAM lParam)
{
    WXLRESULT rc = 0;
    bool processed = FALSE;

    if ( nMsg == WM_SANTA_FE_FOCUS )
    {
		// if we process the message
		processed = DoSantaFeFocus(wParam,lParam);
    }

    if ( !processed )
        rc = wxDocParentFrame::MSWWindowProc(nMsg, wParam, lParam);

	return rc;
}
#endif

CMainFrame::~CMainFrame()
{
	// restore to the menu bar the Administrator menu if it is currently removed
	if (gpApp->m_bAdminMenuRemoved)
	{
		// we append it because when the frame is destroyed the menus will be also, and so
		// we ensure no memory leak from this menu being detached
		wxMenuBar* pMenuBar = GetMenuBar();
		wxASSERT(pMenuBar != NULL);
		bool bAppendedOK = pMenuBar->Append(gpApp->m_pRemovedAdminMenu,gpApp->m_adminMenuTitle);
		wxASSERT(bAppendedOK);
		gpApp->m_pRemovedAdminMenu = NULL;
		gpApp->m_bAdminMenuRemoved = FALSE;
		bAppendedOK = bAppendedOK; // removes compiler warning
		// no Refresh() needed
	}
}

CMainFrame::CMainFrame(wxDocManager *manager, wxFrame *frame, wxWindowID id,
                     const wxString& title, const wxPoint& pos,
                     const wxSize& size, const long type) :
    wxDocParentFrame(manager, frame, id, title, pos, size, type, _T("myFrame"))
{


	idleCount = 0;
//#ifdef __WXDEBUG__
//	m_bShowScrollData = TRUE; // shows scroll parameters and client size in status bar
//#else
	m_bShowScrollData = FALSE;// does not show scroll parameters and client size in status bar
//#endif

	//m_bUsingHighResDPIScreen = FALSE;

	// these dummy ID values are placeholders for unused entries in the accelerator below
	// that are not implemented in the wx version
	int dummyID1 = -1;
	int dummyID2 = -1;
	int dummyID3 = -1;
	int dummyID4 = -1;
	int dummyID5 = -1;
	//int dummyID6 = -1;

	// Accelerators
	// ASSIGN THE ACCELERATOR HOT KEYS REQUIRED FOR THE DIFFERENT PLATFORMS
	// See also the the App's OnInit() where menu adjustments are made to
	// coordinate with these accelerator/hot key assignments.
	//
	// Note: The wx docs say, "On Windows, menu or button commands are supported, on GTK (Linux)
	// only menu commands are supported. Therefore, for those below which are accelerators for
	// toolbar buttons, probably won't work on Linux. There would probably need to be a (hidden?)
	// menu with the toolbar ID's on it and handlers, for the toolbar button hot keys to work.
	// Accelerators with "standard wxWidgets ID"s probably would work without being included
	// in this table because the wxWidgets framework already has them implemented.
	// Under wxWidgets, accelerators have a little different behavior. They remain enabled
	// even when the menu item or button's ID is disabled, unlike MFC which apparently turns
	// off accelerators when the item associated with them is disabled. The wx behavior mandates
	// that we put code within the handlers (similar to what is already in the Update UI handlers)
	// to prevent them from executing if the user types the accelerator key combination.
	//
	// whm modified 11Feb09 to conditionally compile for differences in preferred hot keys for wxMac.
#ifdef __WXMAC__
    wxAcceleratorEntry entries[37];
#else
    wxAcceleratorEntry entries[35];
#endif

#ifdef __WXMAC__
	// whm Note: On Mac Command-1 is reserved for View as Icons, so we'll use Command-Shift-1 on Mac to
	// avoid the reserved key.
    entries[0].Set(wxACCEL_CTRL | wxACCEL_SHIFT, (int) '1', ID_ALIGNMENT);
#else
    entries[0].Set(wxACCEL_CTRL, (int) '1', ID_ALIGNMENT);
#endif
    entries[1].Set(wxACCEL_ALT, WXK_RETURN, dummyID1); //ID_ALTENTER);
    entries[2].Set(wxACCEL_CTRL, (int) 'L', ID_BUTTON_CHOOSE_TRANSLATION); // whm checked OK
    entries[3].Set(wxACCEL_CTRL, (int) 'E', ID_BUTTON_EDIT_RETRANSLATION); // whm checked OK
    entries[4].Set(wxACCEL_CTRL, (int) 'M', ID_BUTTON_MERGE); // whm checked OK - OnButtonMerge() needed trap door added to avoid crash
    entries[5].Set(wxACCEL_CTRL, (int) 'I', ID_BUTTON_NULL_SRC); // whm checked OK
    entries[6].Set(wxACCEL_CTRL, (int) 'D', ID_BUTTON_REMOVE_NULL_SRCPHRASE); // whm checked OK
    entries[7].Set(wxACCEL_CTRL, (int) 'U', ID_BUTTON_RESTORE); // whm checked OK
    entries[8].Set(wxACCEL_CTRL, (int) 'R', ID_BUTTON_RETRANSLATION); // whm checked OK
    entries[9].Set(wxACCEL_SHIFT, WXK_F1, dummyID2); //ID_CONTEXT_HELP);
    entries[10].Set(wxACCEL_CTRL, (int) 'C', wxID_COPY); // standard wxWidgets ID // whm checked OK
    entries[11].Set(wxACCEL_CTRL, WXK_INSERT, wxID_COPY); // standard wxWidgets ID
    entries[12].Set(wxACCEL_SHIFT, WXK_DELETE, wxID_CUT); // standard wxWidgets ID
    entries[13].Set(wxACCEL_CTRL, (int) 'X', wxID_CUT); // standard wxWidgets ID // whm checked OK
#ifdef __WXMAC__
    // whm Note: On Mac Command-2 is reserved for View as List, so we'll use Command-Shift-2 on Mac
    // to avoid the reserved key.
    entries[14].Set(wxACCEL_CTRL | wxACCEL_SHIFT, (int) '2', ID_EDIT_MOVE_NOTE_BACKWARD); // whm checked OK
#else
    entries[14].Set(wxACCEL_CTRL, (int) '2', ID_EDIT_MOVE_NOTE_BACKWARD); // whm checked OK
#endif
#ifdef __WXMAC__
    // whm Note: On Mac Command-3 is reserved for View as Columns, so we'll use Command-Shift-3 on Mac
    // to avoid the reserved key.
    entries[15].Set(wxACCEL_CTRL | wxACCEL_SHIFT, (int) '3', ID_EDIT_MOVE_NOTE_FORWARD); // whm checked OK
#else
    entries[15].Set(wxACCEL_CTRL, (int) '3', ID_EDIT_MOVE_NOTE_FORWARD); // whm checked OK
#endif
    entries[16].Set(wxACCEL_CTRL, (int) 'V', wxID_PASTE); // standard wxWidgets ID
    entries[17].Set(wxACCEL_SHIFT, WXK_INSERT, wxID_PASTE); // standard wxWidgets ID
#ifdef __WXMAC__
	// whm Note: On Mac Command-Q is reserved for Quitting the Application. We'll use Command-Shift-E
	// as a compromise for Editing the Source text.
    entries[18].Set(wxACCEL_CTRL | wxACCEL_SHIFT, (int) 'E', ID_EDIT_SOURCE_TEXT); // whm checked OK
#else
    entries[18].Set(wxACCEL_CTRL, (int) 'Q', ID_EDIT_SOURCE_TEXT); // whm checked OK
#endif
    entries[19].Set(wxACCEL_ALT, WXK_BACK, wxID_UNDO); // standard wxWidgets ID
    entries[20].Set(wxACCEL_CTRL, (int) 'Z', wxID_UNDO); // standard wxWidgets ID
#ifdef __WXMAC__
	// whm Note: On Mac Command-J is reserved for Scrolling/Jumping to a selection, whereas Command-W
	// is normally used for hiding/closing the active window
    entries[21].Set(wxACCEL_CTRL, (int) 'W', ID_FILE_CLOSEKB); // whm checked OK - close project
#else
    entries[21].Set(wxACCEL_CTRL, (int) 'J', ID_FILE_CLOSEKB); // whm checked OK - close project
#endif
    entries[22].Set(wxACCEL_CTRL, (int) 'N', wxID_NEW); // standard wxWidgets ID // whm checked OK
    entries[23].Set(wxACCEL_CTRL, (int) 'O', wxID_OPEN); // standard wxWidgets ID // whm checked OK
    entries[24].Set(wxACCEL_CTRL, (int) 'P', wxID_PRINT); // standard wxWidgets ID // whm checked OK
    entries[25].Set(wxACCEL_CTRL, (int) 'S', wxID_SAVE); // standard wxWidgets ID // whm checked OK
#if defined (__WXMAC__) || defined (__WXGTK__)
	// whm Note: On Mac Command-W is reserved for closing the active window (equivalent to the close
	// command), so as a compromise we'll assign Command-Shift-O for Opening the start up wizard.
    entries[26].Set(wxACCEL_CTRL | wxACCEL_SHIFT, (int) 'O', ID_FILE_STARTUP_WIZARD); // whm checked OK
#else
    entries[26].Set(wxACCEL_CTRL, (int) 'W', ID_FILE_STARTUP_WIZARD); // whm checked OK
#endif
    entries[27].Set(wxACCEL_CTRL, (int) 'F', wxID_FIND); // standard wxWidgets ID // whm checked OK
	entries[28].Set(wxACCEL_CTRL, (int) 'G', ID_GO_TO); // On Mac Command-G is Find Next but this is close enough
    entries[29].Set(wxACCEL_NORMAL, WXK_F1, wxID_HELP); // standard wxWidgets ID // whm checked OK
    entries[30].Set(wxACCEL_NORMAL, WXK_F6, dummyID3); //ID_NEXT_PANE);
    entries[31].Set(wxACCEL_SHIFT, WXK_F6, dummyID4); //ID_PREV_PANE);
#ifdef __WXMAC__
	// whm Note: On Mac Command-H is reserved for hiding/closing the active window, whereas Command F
	// is used to open a combined Find and Find & Replace dialog. As a compromise we'll use
	// Command-Shift-F for opening the Replace dialog.
	entries[32].Set(wxACCEL_CTRL | wxACCEL_SHIFT, (int) 'F', wxID_REPLACE); // standard wxWidgets ID // Mac: Command-Shift-F
#else
    entries[32].Set(wxACCEL_CTRL, (int) 'H', wxID_REPLACE); // standard wxWidgets ID // whm checked OK
#endif
    entries[33].Set(wxACCEL_CTRL, (int) 'K', ID_TOOLS_KB_EDITOR); // whm checked OK
    entries[34].Set(wxACCEL_CTRL, WXK_RETURN, dummyID5); //ID_TRIGGER_NIKB); // CTRL+Enter is Transliterate Mode TODO: check
#ifdef __WXMAC__
	// whm Note: On Mac Command-Q is reserved for quitting the application, so we add an extra
	// accelerator here for it (the Edit Source Text on Mac was changed to Ctrl-Shift-E above and
	// Ctrl-Q remains defined above on Windows and Linux for Edit Source Text).
	entries[35].Set(wxACCEL_CTRL, (int) 'Q', wxID_EXIT);
	// whm Note: On Mac both Command-Shift-/ is the usual hot key for getting app help
    entries[36].Set(wxACCEL_CTRL | wxACCEL_SHIFT, (int) '/', wxID_HELP);
#endif

	//entries[35].Set(wxACCEL_ALT, (int) 'S', IDC_BUTTON_SHORTEN); // added to get compose bar button to work
    //entries[36].Set(wxACCEL_ALT, (int) 'L', IDC_BUTTON_LENGTHEN); // added to get compose bar button to work
    //entries[37].Set(wxACCEL_ALT, (int) 'R', IDC_BUTTON_REMOVE); // added to get compose bar button to work
    //entries[38].Set(wxACCEL_ALT, (int) 'P', IDC_BUTTON_PREV); // added to get compose bar button to work
    //entries[39].Set(wxACCEL_ALT, (int) 'N', IDC_BUTTON_NEXT); // added to get compose bar button to work
    //entries[40].Set(wxACCEL_ALT, (int) 'V', IDC_BUTTON_APPLY); // added to get compose bar button to work
    //entries[41].Set(wxACCEL_ALT, (int) 'U', IDC_RADIO_PUNCT_SECTION); // added to get compose bar button to work
    //entries[42].Set(wxACCEL_ALT, (int) 'E', IDC_RADIO_VERSE_SECTION); // added to get compose bar button to work
    wxAcceleratorTable accel(WXSIZEOF(entries), entries);
    SetAcceleratorTable(accel);

	// NOTE: This CMainFrame constructor is executed BEFORE CAdapt_ItApp::OnInit().
	// Therefore, do NOT attempt to call or manipulate anything here before it
	// is constructed there. For example, removing a menu from
	// the main frame cannot be done here because the main frame menu itself is
	// not loaded and constructed until the SetMenuBar call in CAdapt_ItApp::OnInit()
	// is executed with: m_pMainFrame->SetMenuBar(AIMenuBarFunc());
	// Ditto for accessing any fonts.

	m_pMenuBar = (wxMenuBar*) NULL;			// handle/pointer to the menuBar
	m_pStatusBar = (wxStatusBar*) NULL;		// handle/pointer to the statusBar
	m_pToolBar = (AIToolBar*) NULL;			// handle/pointer to the toolBar
	m_pControlBar = (wxPanel*) NULL;		// handle/pointer to the controlBar
	m_pComposeBar = (wxPanel*) NULL;		// handle/pointer to the composeBar
	m_pRemovalsBar = (wxPanel*) NULL;		// handle/pointer to the removalsBar
	m_pVertEditBar = (wxPanel*) NULL;		// handle/pointer to the vertEditBar
	//m_pVertEditStepTransBar = (wxPanel*) NULL; // handle/pointer to the vertEditStepTransBar
	m_pComposeBarEditBox = (wxTextCtrl*) NULL;	// handle/pointer to the composeBar's edit box

	m_toolBarHeight = 0;		// determined in CMainFrame constructor after toolBar is created
	m_controlBarHeight = 0;		// determined in CMainFrame constructor after controlBar is created
	m_composeBarHeight = 0;		// determined in CMainFrame constructor after composeBar is created
	m_statusBarHeight = 0;
	m_removalsBarHeight = 0;
	m_vertEditBarHeight = 0;
	//m_vertEditStepTransBarHeight = 0;


	canvas = (CAdapt_ItCanvas*) NULL; // added from docview sample 16Mar04

	// following moved here from App's OnInit()
	m_pMenuBar = AIMenuBarFunc(); // Designer's code creates the menuBar
    SetMenuBar(m_pMenuBar); // Associates the menuBar with the Main Frame; tells the frame
							// to show the given menu bar.
	// Notes on SetMenuBar(): WX Docs say,
	// "If the frame is destroyed, the menu bar and its menus will be destroyed also,
	// so do not delete the menu bar explicitly (except by resetting the frame's menu
	// bar to another frame or NULL).
	// Under Windows, a size event is generated, so be sure to initialize data members
	// properly before calling SetMenuBar.
	// Note that on some platforms, it is not possible to call this function twice for
	// the same frame object."

	// Create the main ToolBar for the app
	// wx revision 2Sep06
	// The original design for the wx toolbar was based on the toolbar sample, which
	// created and deleted the toolbar each time it was viewed/hidden. Rather than
	// doing that, we just hide the toolbar when not being viewed (via View menu).
	// We can also manage the toolbar in our mainsizer. along with the controlbar,
	// composebar and canvas.
    long style = /*wxNO_BORDER |*/ wxTB_FLAT | wxTB_HORIZONTAL;
	AIToolBar* toolBar = new AIToolBar(this, -1, wxDefaultPosition, wxDefaultSize, style);
	wxASSERT(toolBar != NULL);
	m_pToolBar = toolBar;

	// Determine the screen dpi to see if we are running on a 200dpi OLPC XO type screen.
	// If so, we use the alternate AIToolBar32x30Func which has double sized toolbar bitmaps for better
	// readability on the 200dpi screens.
	// whm Note 14Apr09: The XO machine running under Intrepid actually reports 100.0 DPI!
	// Therefore this test won't distinguish an XO from any other "normal" screen computer,
	// so we try using wxPlatformInfo::GetArchName()
	wxString archName,OSSystemID,OSSystemIDName,hostName;
	int OSMajorVersion, OSMinorVersion;
	wxPlatformInfo platInfo;
	archName = platInfo.GetArchName(); // returns "32 bit" on Windows
	OSSystemID = platInfo.GetOperatingSystemIdName(); // returns "Microsoft Windows NT" on Windows
	OSSystemIDName = platInfo.GetOperatingSystemIdName();
	//OSMajorVersion = platInfo.GetOSMajorVersion();
	//OSMinorVersion = platInfo.GetOSMinorVersion();
	wxOperatingSystemId sysID;
	sysID = ::wxGetOsVersion(&OSMajorVersion,&OSMinorVersion);
	sysID = sysID; // avoid warning
	wxMemorySize memSize;
	wxLongLong MemSizeMB;
	// GDLC 6May11 Used the app's GetFreeMemory()
	memSize = gpApp->GetFreeMemory();
	MemSizeMB = memSize / 1048576;
	hostName = ::wxGetHostName(); // "BILLDELL" on my desktop

	//wxSize displaySizeInPixels;
	//displaySizeInPixels = wxGetDisplaySize();
	//wxSize displaySizeInMM;
	//displaySizeInMM = wxGetDisplaySizeMM();
	//wxSize displaySizeInInches;
	//displaySizeInInches.x = displaySizeInMM.x / 25.4;
	//displaySizeInInches.y = displaySizeInMM.y / 25.4;
	//float screenDPI;
	//screenDPI = sqrt(float(displaySizeInPixels.x * displaySizeInPixels.x) + float(displaySizeInPixels.y * displaySizeInPixels.y))
	//	/ sqrt(float(displaySizeInInches.x * displaySizeInInches.x) + float(displaySizeInInches.y * displaySizeInInches.y));

	if (gpApp->m_bExecutingOnXO)
	{
		toolBar->SetToolBitmapSize(wxSize(32,30));
		AIToolBar32x30Func( toolBar );
	}
	else
	{
		AIToolBarFunc( toolBar ); // this calls toolBar->Realize(), but we want the frame to be parent
	}
	SetToolBar(toolBar);
	// Notes on SetToolBar(): WX Docs say,
	// "SetToolBar() associates a toolbar with the frame. When a toolbar has been created with
	// this function, or made known to the frame with wxFrame::SetToolBar, the frame will manage
	// the toolbar position and adjust the return value from wxWindow::GetClientSize to reflect
	// the available space for application windows. Under Pocket PC, you should always use this
	// function for creating the toolbar to be managed by the frame, so that wxWidgets can use
	// a combined menubar and toolbar. Where you manage your own toolbars, create a wxToolBar
	// as usual."

	m_pToolBar = GetToolBar();
	wxASSERT(m_pToolBar == toolBar);

	wxSize toolBarSize;
	toolBarSize = m_pToolBar->GetSize();
	m_toolBarHeight = toolBarSize.GetHeight();	// we shouldn't need this since doc/view
												// is supposed to manage the toolbar and
												// our Main Frame should account for its
												// presence when calculating the client size
												// with pMainFrame->GetClientSize()

	// whm Note 6Jan12: The CMainFrame constructor is called relatively early in the App's
	// OnInit() function. At this point the basic config file has not been
	// read, therefore the visibility of the toolBar, statusBar and modeBar will take on
	// their normal defaults - all visible. The changes of visibility according to the
	// user's preferences are done later in OnInit() when the
	// MakeMenuInitializationsAndPlatformAdjustments() function is called.
	//if (gpApp->m_bToolBarVisible)
	//{
	//	m_pToolBar->Show();
	//}
	//else
	//{
	//	m_pToolBar->Hide();
	//}

	// MFC version also has 3 lines here to EnableDocking() of toolBar. We won't use docking in
	// the wx version, although the wxGTK version enables docking by default (we turn it off).

	// Create the control bar using a wxPanel
	// whm note: putting the control bar on a panel could have been done in wxDesigner
	wxPanel *controlBar = new wxPanel(this, -1, wxDefaultPosition, wxDefaultSize, 0);
	wxASSERT(controlBar != NULL);
	m_pControlBar = controlBar;

	// The ControlBarFunc() function is located in Adapt_It_wdr.cpp.
	// To populate the controlBar panel we've used wxBoxSizers. The outer
	// most sizer is a vertical box sizer which has a horizontal line in
	// the upper part of the box (for the line between the toolbar and
	// controlbar), and the row of controls laid out in wxHORIZONTAL
	// alignment within an embedded horizontal box sizer, below the line
	// within the vertical box sizer

	if (gpApp->m_bExecutingOnXO) //if (m_bUsingHighResDPIScreen)
	{
		// We're running on a high res screen - probably a OLPC XO, so use the 2 line control bar for
		// better fit in main frame
		ControlBar2LineFunc( controlBar, TRUE, TRUE );
	}
	else
	{
		ControlBarFunc( controlBar, TRUE, TRUE );
	}

	// Note: We are creating a controlBar which the doc/view framework knows
	// nothing about. The mainFrameSizer below takes care of the controlBar's
	// layout within the Main Frame. The controlBar is visible by default on
	// the first run of the app. As of version 6.1.1 the control bar can
	// be made visible or hidden by menu choice and its state remains visible
	// or hidden until explicitly changed - subject to its view menu item
	// visibility in the current profile.
	// Here and in the OnSize() method, we calculate the canvas' client
	// size, which must exclude the height of the controlBar (if shown).
	// Get and save the native height of our controlBar.
	wxSize controlBarSize;
	controlBarSize = m_pControlBar->GetSize();
	m_controlBarHeight = controlBarSize.GetHeight();

	// whm Note 6Jan12: The CMainFrame constructor is called relatively early in the App's
	// OnInit() function. At this point the basic config file has not been
	// read, therefore the visibility of the toolBar, statusBar and modeBar will take on
	// their normal defaults - all visible. The changes of visibility according to the
	// user's preferences are done later in OnInit() when the
	// MakeMenuInitializationsAndPlatformAdjustments() function is called.
	//if (gpApp->m_bModeBarVisible)
	//{
	//	m_pControlBar->Show();
	//}
	//else
	//{
	//	m_pControlBar->Hide();
	//}

	// set the background color of the Delay edit control to button face
	CAdapt_ItApp* pApp = &wxGetApp(); // needed here!
	pApp->sysColorBtnFace = wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE); // needed here before App sets it
	wxTextCtrl* pDelayBox = (wxTextCtrl*)FindWindowById(IDC_EDIT_DELAY);
	wxASSERT(pDelayBox != NULL);
	pDelayBox->SetBackgroundColour(pApp->sysColorBtnFace);

	// Create the compose bar using a wxPanel
	wxPanel *composeBar = new wxPanel(this, -1, wxDefaultPosition, wxDefaultSize, 0);
	wxASSERT(composeBar != NULL);
	m_pComposeBar = composeBar;
	ComposeBarFunc( composeBar, TRUE, TRUE );
	// Note: We are creating a composeBar which the doc/view framework knows
	// nothing about. The mainFrameSizer below takes care of the composeBar's
	// layout within the Main Frame. The composeBar is not visible by default
	// but can be toggled on from the view menu or when it takes on the form
	// of the Free Translation compose bar in Free Translation mode. As of
	// version 6.1.1 the compose bar's state remains visible or hidden until
	// explicitly changed.
	// Here and in the OnSize() method, we calculate the canvas' client
	// size, which also must exclude the height of the composeBar (if shown).
	// Get and save the native height of our composeBar.
	wxSize composeBarSize;
	composeBarSize = m_pComposeBar->GetSize();
	m_composeBarHeight = composeBarSize.GetHeight();
	m_pComposeBarEditBox = (wxTextCtrl*)FindWindowById(IDC_EDIT_COMPOSE);
	wxASSERT(m_pComposeBarEditBox != NULL);

	// set the font used in the compose bar to the font & size as currently for the target text
	m_pComposeBarEditBox->SetFont(*pApp->m_pTargetFont);
	// The initial state of the composeBar, however is hidden. The OnViewComposeBar()
	// method takes care of hiding and showing the composeBar and OnSize()insures the
	// client window is adjusted accordingly and the screen redrawn as needed
	m_pComposeBar->Hide();

	// Create the removals bar using a wxPanel
	wxPanel *removalsBar = new wxPanel(this, -1, wxDefaultPosition, wxDefaultSize, 0);
	wxASSERT(removalsBar != NULL);
	m_pRemovalsBar = removalsBar;
	pRemovalsBarSizer = RemovalsBarFunc( removalsBar, TRUE, TRUE );
	// Note: We are creating a removalsBar which the doc/view framework knows
	// nothing about. The mainFrameSizer below takes care of the removalsBar's
	// layout within the Main Frame. The removalsBar is not visible by default
	// but can be made visible during source text/vertical editing.
	// Here and in the OnSize() method, we calculate the canvas' client
	// size, which also must exclude the height of the removalsBar (if shown).
	// Get and save the native height of our removalsBar.
	wxSize removalsBarSize;
	removalsBarSize = m_pRemovalsBar->GetSize();
	m_removalsBarHeight = removalsBarSize.GetHeight();
	m_pRemovalsBarComboBox = (wxComboBox*)FindWindowById(IDC_COMBO_REMOVALS);
	wxASSERT(m_pRemovalsBarComboBox != NULL);

	// MFC: BEW added 11July08 set the font & size used in the combobox in the removals bar to
	// the font as currently for the target text
	// whm:
	CopyFontBaseProperties(pApp->m_pTargetFont,pApp->m_pDlgTgtFont);
	// The CopyFontBaseProperties function above doesn't copy the point size, so
	// make the dialog font show in the proper dialog font size.
	pApp->m_pDlgTgtFont->SetPointSize(10);
	m_pRemovalsBarComboBox->SetFont(*pApp->m_pDlgTgtFont);
 	// Set removals combo box RTL alignment if necessary
	#ifdef _RTL_FLAGS
	if (gpApp->m_bTgtRTL)
		m_pRemovalsBarComboBox->SetLayoutDirection(wxLayout_RightToLeft);
	else
		m_pRemovalsBarComboBox->SetLayoutDirection(wxLayout_LeftToRight);
	#endif
	// whm Note: At this point the MFC code attempts to "set up the list size - make it fit whatever
	// is the client rectangle height for the frame wnd". I don't think this is necessary in the wx
	// version which controls the bar and its contents with sizers.
    // The initial state of the removalsBar, however is hidden. The m_pRemovalsBar is dynamically hidden
    // or shown depending on the state of any source text/vertical editing process. OnSize()insures the
    // client window is adjusted accordingly and the screen redrawn as needed
	m_pRemovalsBar->Hide();

	// Create the vertEditBar using a wxPanel
	wxPanel *vertEditBar = new wxPanel(this, -1, wxDefaultPosition, wxDefaultSize, 0);
	wxASSERT(vertEditBar != NULL);
	m_pVertEditBar = vertEditBar;
	pVertEditBarSizer = VertEditBarFunc( vertEditBar, TRUE, TRUE );
	// Note: We are creating a vertEditBar which the doc/view framework knows
	// nothing about. The mainFrameSizer below takes care of the vertEditBar's
	// layout within the Main Frame. The vertEditBar is not visible by default
	// but can be made visible during source text/vertical editing.
	// Here and in the OnSize() method, we calculate the canvas' client
	// size, which also must exclude the height of the vertEditBar (if shown).
	// Get and save the native height of our vertEditBar.
	wxSize vertEditBarSize;
	vertEditBarSize = m_pVertEditBar->GetSize();
	m_vertEditBarHeight = vertEditBarSize.GetHeight();
	m_pVertEditMsgBox = (wxTextCtrl*)FindWindowById(IDC_EDIT_MSG_TEXT);
	wxASSERT(m_pVertEditMsgBox != NULL);

	// MFC: BEW added 23July08 set the font & size used in the read-only CEdit in the vertical edit bar to
	// the font as currently for the navigation text -- we have our own font specifically for this bar
	// whm Note: We're not using the same font here as we did above in the removals bar; here we use
	// the dedicated m_pVertEditFont for the m_pVertEditMsgBox.
	CopyFontBaseProperties(pApp->m_pNavTextFont,pApp->m_pVertEditFont);
	// The CopyFontBaseProperties function above doesn't copy the point size, so
	// make the dialog font show in the proper dialog font size.
	pApp->m_pVertEditFont->SetPointSize(10);
	m_pVertEditMsgBox->SetFont(*pApp->m_pVertEditFont);
 	// Set removals combo box RTL alignment if necessary
	#ifdef _RTL_FLAGS
	if (gpApp->m_bNavTextRTL)
		m_pVertEditMsgBox->SetLayoutDirection(wxLayout_RightToLeft);
	else
		m_pVertEditMsgBox->SetLayoutDirection(wxLayout_LeftToRight);
	#endif

	// whm Note: At this point the MFC code attempts to "set up the CEdit's width - make it fit whatever
	// is the client rectangle height for the frame wnd". I don't think this is necessary in the wx
	// version which controls the bar and its contents with sizers.
	// The initial state of the vertEditBar, however is hidden. The m_pVertEditBar is dynamically hidden
    // or shown depending on the state of any source text/vertical editing process. OnSize()insures the
    // client window is adjusted accordingly and the screen redrawn as needed
	m_pVertEditBar->Hide();

	// BEW added 27Mar07 for support of receiving synchronized scroll messages
	// whm note: in the MFC version the following is located in CMainFrame::OnCreate()
	if (gpDocList == NULL)
	{
		// create the SPList, so it can be used if the user turns on message receiving
		gpDocList = new SPList;
	}

#ifdef _USE_SPLITTER_WINDOW
	// whm: See font.cpp sample which has a good example of creating a wxSplitterWindow
    splitter = new wxSplitterWindow(this);
#endif

	// All the potentially visible "bars" are now created and their data initialized.

#ifdef _USE_SPLITTER_WINDOW
	// the wxSplitterWindow is the parent of the canvas
 	this->canvas = CreateCanvas(splitter);
#else
	this->canvas = CreateCanvas(this);
#endif
	// now that canvas is created, set canvas' pointer to this main frame
	canvas->pFrame = this;

#ifdef _USE_SPLITTER_WINDOW
    splitter->SplitHorizontally(canvas,canvas,200); //splitter->Initialize(canvas);
	splitter->Show(TRUE);
#endif

	// Now create a mainFrameSizer and add our two non-standard "bars" and our canvas to it
	// Notes: canvas has its own canvasSizer and its own SetAutoLayout() and SetSizer()
	// see: the body of CMainFrame::CreateCanvas().
	// We do not include m_pMenuBar nor m_pToolBar under mainFrameSizer's control because
	// these interface elements are controlled by CMainFrame : wxDocParentFrame via
	// the SetMenuBar() and SetToolBar() calls above.
	//wxBoxSizer* mainFrameSizer = new wxBoxSizer(wxVERTICAL);
	//mainFrameSizer->Add(m_pControlBar, 0);
	//mainFrameSizer->Add(m_pComposeBar, 0);
	//mainFrameSizer->Add(canvas, 1, wxEXPAND);

	//SetAutoLayout(TRUE);
	// WX Notes on SetAutoLayout():
	// "SetAutoLayout() Determines whether the wxWindow::Layout function will be called
	// automatically when the window is resized. Please note that this only happens for
	// the windows usually used to contain children, namely wxPanel and wxTopLevelWindow
	// (and the classes deriving from them). This method is called implicitly by
	// wxWindow::SetSizer but if you use wxWindow::SetConstraints you should call it
	// manually or otherwise the window layout won't be correctly updated when its size
	// changes."
	//SetSizer(mainFrameSizer);
	// WX Notes on SetSizer():
	// "Sets the window to have the given layout sizer. The window will then own the object,
	// and will take care of its deletion. If an existing layout constraints object is
	// already owned by the window, it will be deleted if the deleteOld parameter is true.
	// Note that this function will also call SetAutoLayout implicitly with true parameter
	// if the sizer is non-NULL and false otherwise.
	// SetSizer now enables and disables Layout automatically, but prior to wxWidgets 2.3.3
	// the following applied:
	// You must call wxWindow::SetAutoLayout to tell a window to use the sizer automatically
	// in OnSize; otherwise, you must override OnSize and call Layout() explicitly. When
	// setting both a wxSizer and a wxLayoutConstraints, only the sizer will have effect."

	//mainSizer->FitInside(canvas);

	// FIXME: On wxX11, we need the MDI frame to process this
    // event, but on other platforms this should not
    // be done.
#ifdef __WXUNIVERSAL__
    event.Skip();
#endif

	// Create the StatusBar for the app
	CreateStatusBar(2);
	// Adjust the fields in the StatusBar
	// TODO: Implement indicators for statusBar. See MFC OnCreate() lines 68, 108-109
	m_pStatusBar = GetStatusBar();

	// wx version displays some scrolling data on the statusbar. m_bShowScrollData is
	// only true when __WXDEBUG__ is defined, so it will not appear in release versions.
	// Here we make room for it by making the second field width larger.
	if (m_bShowScrollData)
	{
		int fieldWidths[] = {0,1000};
		m_pStatusBar->SetStatusWidths(2, fieldWidths);
	}
	else
	{
		int fieldWidths[] = {-1, 100};
		m_pStatusBar->SetStatusWidths(2, fieldWidths);
	}

	wxSize statusBarSize;
	statusBarSize = m_pStatusBar->GetSize();
	m_statusBarHeight = statusBarSize.GetHeight();

	// whm Note 6Jan12: The CMainFrame constructor is called relatively early in the App's
	// OnInit() function. At this point the basic config file has not been
	// read, therefore the visibility of the toolBar, statusBar and modeBar will take on
	// their normal defaults - all visible. The changes of visibility according to the
	// user's preferences are done later in OnInit() when the
	// MakeMenuInitializationsAndPlatformAdjustments() function is called.
	//if (gpApp->m_bStatusBarVisible)
	//{
	//	m_pStatusBar->Show();
	//}
	//else
	//{
	//	m_pStatusBar->Hide();
	//}

	// set the font used in the compose bar to the font & size for the target font
	// Unlike the MFC version, the fonts haven't been created yet at this point,
	// so I've moved the code that sets the composebar font and RTL to the App's OnInit().
}

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

// Declare the AboutDlg class
class AboutDlg : public AIModalDialog
{
public:
    AboutDlg(wxWindow *parent);
};

// Implement the AboutDlg class
AboutDlg::AboutDlg(wxWindow *parent)
             : AIModalDialog(parent, -1, wxString(_T("About Adapt It")),
                        wxDefaultPosition,
						wxDefaultSize,
                        wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	wxSizer* pAboutDlgSizer;
	pAboutDlgSizer = AboutDlgFunc( this, TRUE, TRUE );
	// First parameter is the parent which is usually 'this'.
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	// The declaration is: AboutDlgFunc( wxWindow *parent, bool call_fit, bool set_sizer ).
	CAdapt_ItApp* pApp = &wxGetApp();

	// whm note: the routine below often failed to get the actual modification date
	//wxString appCreateDate,appModDate;
	//if (wxFileExists(pApp->m_executingAppPathName))
	//{
	//	wxFileName fn(pApp->m_executingAppPathName);
	//	wxDateTime dtModified,dtCreated;
	//	if (fn.GetTimes(NULL,&dtModified,&dtCreated))
	//	{
	//		appCreateDate = dtCreated.FormatISODate();
	//		appModDate = dtModified.FormatISODate();
	//	}
	//}
	//if (!appModDate.IsEmpty())
	//{
	//	wxStaticText* pStatic = (wxStaticText*)FindWindowById(ID_ABOUT_VERSION_DATE);
	//	pStatic->SetLabel(appModDate);
	//}

	// A better approach is to set the date from constants (since the Linux packages also need to
	// specify a package date.
	// Get date from string constants at beginning of Adapt_It.h.
	wxString versionDateStr;
	versionDateStr.Empty();
	versionDateStr << VERSION_DATE_YEAR;
	versionDateStr += _T("-");
	versionDateStr << VERSION_DATE_MONTH;
	versionDateStr += _T("-");
	versionDateStr << VERSION_DATE_DAY;
	wxStaticText* pStaticVersDate = (wxStaticText*)FindWindowById(ID_ABOUT_VERSION_DATE);
	pStaticVersDate->SetLabel(versionDateStr);


	// Create the version number from the defines in Adapt_It.h:
	wxString strVersionNumber;
	strVersionNumber.Empty();
	strVersionNumber << VERSION_MAJOR_PART;
	strVersionNumber += _T(".");
	strVersionNumber << VERSION_MINOR_PART;
	strVersionNumber += _T(".");
	strVersionNumber << VERSION_BUILD_PART;
	if (PRE_RELEASE != 0)
	{
		wxStaticText* pStaticWXVerLabel = (wxStaticText*)FindWindowById(ID_ABOUT_VERSION_LABEL);
		wxASSERT(pStaticWXVerLabel != NULL);
		pStaticWXVerLabel->SetLabel(_T("Pre-Release"));
	}
	wxStaticText* pVersionNum = (wxStaticText*)FindWindowById(ID_ABOUT_VERSION_NUM);
	pVersionNum->SetLabel(strVersionNumber);

	// Set About Dlg static texts for OS, system language and locale information
	wxString strHostOS, strHostArchitecture;
#if defined(__WXMSW__)
	strHostOS = _("Microsoft Windows");
#elif defined(__WXGTK__)
	strHostOS = _("Linux");
#elif defined(__WXMAC__)
	strHostOS = _("Mac OS X");
#else
	strHostOS = _("Unknown");
#endif

#if defined(__INTEL__) && !defined(__IA64__)
	strHostArchitecture = _T("Intel 32bit");
#endif
#if defined(__INTEL__) && defined(__IA64__)
	strHostArchitecture = _T("Intel 64bit");
#elif defined(__IA64__) // doesn't detect AMD64
	strHostArchitecture = _T("64bit");
#endif
#if defined(__POWERPC__)
	strHostArchitecture = _T("PowerPC");
#endif


	strHostOS.Trim(FALSE);
	strHostOS.Trim(TRUE);
	strHostOS = _T(' ') + strHostOS;
	if (!strHostArchitecture.IsEmpty())
		strHostOS += _T('-') + strHostArchitecture;
	wxStaticText* pStaticHostOS = (wxStaticText*)FindWindowById(ID_STATIC_HOST_OS);
	wxASSERT(pStaticHostOS != NULL);
	pStaticHostOS->SetLabel(strHostOS);
	wxStaticText* pStaticWxVersionUsed = (wxStaticText*)FindWindowById(ID_STATIC_WX_VERSION_USED);
	wxASSERT(pStaticWxVersionUsed != NULL);
	wxString versionStr;
	versionStr.Empty();
	versionStr << wxMAJOR_VERSION;
	versionStr << _T(".");
	versionStr << wxMINOR_VERSION;
	versionStr << _T(".");
	versionStr << wxRELEASE_NUMBER;
	pStaticWxVersionUsed->SetLabel(versionStr);

	wxString strUILanguage;
	// Fetch the UI language info from the global currLocalizationInfo struct
	strUILanguage = gpApp->currLocalizationInfo.curr_fullName;
	strUILanguage.Trim(FALSE);
	strUILanguage.Trim(TRUE);
	strUILanguage = _T(' ') + strUILanguage;
	wxStaticText* pStaticUILanguage = (wxStaticText*)FindWindowById(ID_STATIC_UI_LANGUAGE);
	wxASSERT(pStaticUILanguage != NULL);
	pStaticUILanguage->SetLabel(strUILanguage);
	wxString tempStr;
	wxString locale = pApp->m_pLocale->GetLocale();
	if (!locale.IsEmpty())
	{
		wxStaticText* pStatic = (wxStaticText*)FindWindowById(ID_STATIC_SYS_LANGUAGE);
		wxASSERT(pStatic != NULL);
		tempStr = pApp->m_pLocale->GetLocale();
		tempStr = _T(' ') + tempStr;
		pStatic->SetLabel(tempStr);
	}

	if (!pApp->m_pLocale->GetCanonicalName().IsEmpty())
	{
		wxStaticText* pStatic = (wxStaticText*)FindWindowById(ID_STATIC_CANONICAL_LOCALE_NAME);
		wxASSERT(pStatic != NULL);
		tempStr = pApp->m_pLocale->GetCanonicalName();
		tempStr = _T(' ') + tempStr;
		pStatic->SetLabel(tempStr);
	}
	if (!pApp->m_pLocale->GetSysName().IsEmpty())
	{
		wxStaticText* pStatic = (wxStaticText*)FindWindowById(ID_STATIC_SYS_LOCALE_NAME);
		wxASSERT(pStatic != NULL);
		tempStr = pApp->m_pLocale->GetSysName();
		// GetSysName() returns the following:
		// On wxMSW: English_United States.1252
		// On Ubuntu: en_US.UTF-8
		// On Mac OS X: C

		// On Linux Natty i386 version GetSysName() returns a long garbage string which
		// distorts the dialog, so we'll test for a long string (> 20 chars) and if detected
		// we'll just put '[Unknown]'
		if (tempStr.Length() > 20)
		{
			tempStr = _T("[Unknown]");
		}
#ifdef __WXGTK__
#endif

#ifdef __WXMAC__
		// GetSysName() on wxMac always returns "C", so we'll unilaterally change this to "MacOSX"
		tempStr = pApp->m_pLocale->GetCanonicalName() + _T(".") + _T("MacOSX");
#endif
		tempStr = _T(' ') + tempStr;
		pStatic->SetLabel(tempStr);
	}

	if (!pApp->m_systemEncodingName.IsEmpty())
	{
		wxStaticText* pStatic = (wxStaticText*)FindWindowById(ID_STATIC_SYS_ENCODING_NAME);
		tempStr = pApp->m_systemEncodingName; //m_systemEncodingName is assigned by calling wxLocale::GetSystemEncodingName() in the App's OnInit()
		// Windows: m_systemEncodingName = "windows-1252"
		//  Ubuntu: m_systemEncodingName = "UTF-8"
		//     Mac: m_systemEncodingName = "MacRoman" [See App's OnInit() where GetSystemEncodingName()
		//             is called. There we set this value to a Mac... encoding value]
		// Note: On Mac OS X, the default encoding depends on your chosen primary language
		// (System Preferences, International pane, Languages tab, list of languages).
		// On a typical Western-European language Mac OS X config, the default encoding will be MacRoman.

		tempStr.Trim(FALSE);
		tempStr.Trim(TRUE);
		tempStr = _T(' ') + tempStr;
		pStatic->SetLabel(tempStr);
	}

	// set static text to be either "This version designed for UNICODE data" or
	// This version designed for ANSI only data"
	wxString UnicodeOrAnsiBuild = _T("Regular (not Unicode)");
#ifdef _UNICODE
	UnicodeOrAnsiBuild = _T("UNICODE");
#endif

	wxStaticText* pUnicodeOrAnsiBuild = (wxStaticText*)FindWindowById(ID_STATIC_UNICODE_OR_ANSI);
	wxASSERT(pUnicodeOrAnsiBuild != NULL);
	tempStr = pUnicodeOrAnsiBuild->GetLabel();
	tempStr = tempStr.Format(tempStr,UnicodeOrAnsiBuild.c_str());
	pUnicodeOrAnsiBuild->SetLabel(_T(' ') + tempStr);

	// wx Note: the wxLayoutDirection enum and the methods to set and get the layout direction
	// exist, but are currently undocumented in wxWidgets 2.8.0. When GetLayoutDirection is
	// called on the App, it returns the Default System Layout Direction. When called on a
	// specific wxWindow it returns the layout direction set for the given window. The
	// method SetLayoutDirection() cannot be called on the App, but can be called on a given
	// window to set the layout direction of that window.
	wxLayoutDirection layoutDir = pApp->GetLayoutDirection();

	wxString layoutDirStr;
	layoutDirStr.Empty();
	switch (layoutDir)
	{
	case wxLayout_LeftToRight: // wxLayout_LeftToRight has enum value of 1
		layoutDirStr = _("Left-to-Right");
		break;
	case wxLayout_RightToLeft: // wxLayout_LeftToRight has enum value of 2
		layoutDirStr = _("Right-to-Left");
		break;
	default:
		layoutDirStr = _("System Default");// = wxLayout_Default which has enum value of 0
	}
	layoutDirStr.Trim(FALSE);
	layoutDirStr.Trim(TRUE);
	layoutDirStr = _T(' ') + layoutDirStr;
	wxStaticText* pStatic = (wxStaticText*)FindWindowById(ID_STATIC_SYSTEM_LAYOUT_DIR);
	pStatic->SetLabel(layoutDirStr);

	// Note: Set the current version number in wxDesigner in the AboutDlgFunc()
	pAboutDlgSizer->Layout();
}

// App command to run the dialog
void CMainFrame::OnAppAbout(wxCommandEvent& WXUNUSED(event))
{
	gpApp->LogUserAction(_T("Initiated OnAppAbout()"));
	// To Change version number, date, etc., use wxDesigner to change the AboutDlgFunc() in the
	// Adapt_It_wdr.cpp file
    AboutDlg dlg(this);
	dlg.Centre();
    dlg.ShowModal();
}

void CMainFrame::OnMRUFile(wxCommandEvent& event) //BOOL CAdapt_ItApp::OnOpenRecentFile(UINT nID)
// wx Note: I renamed this MFC handler OnMRUFile which is its virtual function name in wx's doc/view
// framework. It also does not return a bool nor does it use the UINT nID parameter, but the usual
// wxCommandEvent& event parameter.
// this should work right for glossing or adapting, since the CloseProject( ) call
// calls view's OnFileCloseProject( ) handler, which restores the four glossing flags to
// their default values (ie. glossing not enabled, glossing OFF, etc.)
{
	if (gbVerticalEditInProgress)
	{
		// don't allow opening any recent document file in the MRU list
		// until the vertical edit is finished
		::wxBell();
		return;
	}
	gbTryingMRUOpen = TRUE; // TRUE only while this function's stack frame exists
							// and used only in doc's OnOpenDocument() function

	CAdapt_ItView* pView = gpApp->GetView();
	if (pView != NULL)
		pView->CloseProject();
	gbViaMostRecentFileList = TRUE;

	// cause OnIdle() to update the window title if the open attempt failed,
	// otherwise the failed doc name remains in the window's title bar
	// Note, since OnOpenRecentFile always returns TRUE, we have to unilaterally
	// cause OnIdle to do the update of the title to ensure we catch the failures
	gbUpdateDocTitleNeeded = TRUE;

	CAdapt_ItApp* pApp = &wxGetApp();
	wxASSERT(pApp != NULL);
	CAdapt_ItDoc* pDoc = pApp->GetDocument();
	wxASSERT(pDoc != NULL);

	// we need to set m_pCurrBookNamePair etc to what is appropriate, for example,
	// we can't leave it NULL if we just MRU opened a document in a book folder
	if (gpApp->m_bBookMode && !gpApp->m_bDisableBookMode)
	{
		gpApp->m_pCurrBookNamePair = ((BookNamePair*)(*gpApp->m_pBibleBooks)[gpApp->m_nBookIndex]);
		gpApp->m_bibleBooksFolderPath = gpApp->m_curAdaptionsPath + gpApp->PathSeparator + gpApp->m_pCurrBookNamePair->dirName;
	}
	else
	{
		gpApp->m_pCurrBookNamePair = NULL;
		gpApp->m_bibleBooksFolderPath.Empty();
		gpApp->m_bBookMode = FALSE;
		gpApp->m_nBookIndex = -1;
		gpApp->m_nLastBookIndex = gpApp->m_nDefaultBookIndex;
	}
	// get a pointer to the CLayout object
	CLayout* pLayout = gpApp->m_pLayout;

	// WX Docs show the following example for its OnMRUFile handler:
	// TODO: need to tweak this to also to ensure that things are set up properly to
	// load a document from MRU when it is located in a different project; also it must
	// work when book folder mode was on in one project and off in the other, or vise versa;
	// also that it toggles the required booleans and sets up the correct paths to the
	// book folder or Adaptations folder as the case may be.
    wxString fileFromMRU(gpApp->m_pDocManager->GetHistoryFile(event.GetId() - wxID_FILE1));
    if (fileFromMRU != _T(""))
	{
		if (!::wxFileExists(fileFromMRU))
		{
			return; // FALSE;
		}
		else
		{
			if (!pDoc->OnSaveModified())
				return;
			pDoc->Modify(FALSE);

			// BEW 3Aug09, brought here from after OnOpenDoc file, otherwise,
			// m_bRTL_Layout value of OnOpenDocument() gets reset to earlier
			// project's value in OnInitialUpdate()
			pDoc->SetFilename(fileFromMRU,TRUE);

			pView->OnInitialUpdate(); // need to call it here because wx's doc/view doesn't automatically call it

			bool bOK = pDoc->OnOpenDocument(fileFromMRU);
			if (!bOK)
			{
				// whm TODO: put the code here for updating the doc title that MFC version had in its
				// OnIdle() handler. Then eliminate the if (gbUpdateDocTitleNeeded) block in OnIdle()
				// and do away with the if gbUpdateDocTitleNeeded global altogether.
				return;
			}

            // BEW added 3Aug09, the layout already done would have used the old project's
            // fonts & sizes etc, so if we don't layout again with the correct parameters
            // the view won't display properly; so first call SetLayoutParameters() to hook
            // up the font settings etc from the MRU opened document to the CLayout object,
            // and then layout
			pLayout->SetLayoutParameters();

			// BEW 3Aug09 added RecalcLayout() etc here, otherwise changing from RTL to
			// LTR results in a too deep whiteboard and a too big interpile gap as
			// settings from earlier project & document still persist - we have to rebuild
			// the piles from scratch with the updated settings, before we let
			// CMainFrame's later RecalcLayout() call with param create_strips_keep_piles
			// happen, otherwise the piles won't be right
			pLayout->RecalcLayout(gpApp->m_pSourcePhrases,create_strips_and_piles);
			gpApp->m_pActivePile = pView->GetPile(gpApp->m_nActiveSequNum);
			// CMainFrame will do an OnSize() which will call RecalcLayout() again and
            // then invalidate the view, so don't waste time here with an Invalidate() call
            // on the view
		}
	}
	else
	{
		return;
	}
	gbTryingMRUOpen = FALSE; // restore default value
	return;
}

//void CMainFrame::OnHelp(wxHelpEvent& WXUNUSED(event))
//{
//    ShowHelp(event.GetId(), m_help);
//}

//void CMainFrame::OnShowContextHelp(wxCommandEvent& WXUNUSED(event))
//{
//    // This starts context help mode, then the user
//    // clicks on a window to send a help message
//    wxContextHelp contextHelp(this);
//}

void CMainFrame::OnAdvancedHtmlHelp(wxCommandEvent& WXUNUSED(event))
{
	gpApp->LogUserAction(_T("Initiated OnAdvanceHtmlHelp()"));
	//int eID;
	//eID = event.GetId();
	//ShowHelp(event.GetId(), *m_pHelpController);
	wxString pathName = gpApp->m_helpInstallPath + gpApp->PathSeparator + gpApp->m_htbHelpFileName;
	bool bOK1;
	m_pHelpController->SetTempDir(_T(".")); // whm added 15Jan09 enables caching of helps for faster startups
	bOK1 = m_pHelpController->AddBook(wxFileName(pathName, wxPATH_UNIX)); // whm added wxPATH_UNIX which is OK on Windows and seems to be needed for Ubuntu Intrepid
	if (!bOK1)
	{
		wxString strMsg;
		strMsg = strMsg.Format(_T("Adapt It could not add book contents to its help file.\nThe name and location of the help file it looked for:\n %s\nTo ensure that help is available, this help file must be installed with Adapt It."),pathName.c_str());
		wxMessageBox(strMsg, _T(""), wxICON_WARNING);
		gpApp->LogUserAction(strMsg);
	}
	if (bOK1)
	{
		bool bOK2;
		bOK2 = m_pHelpController->DisplayContents();
		if (!bOK2)
		{
			wxString strMsg;
			strMsg = strMsg.Format(_T("Adapt It could not display the contents of its help file.\nThe name and location of the help file it looked for:\n %s\nTo ensure that help is available, this help file must be installed with Adapt It."),pathName.c_str());
			wxMessageBox(strMsg, _T(""), wxICON_WARNING);
			gpApp->LogUserAction(strMsg);
		}
	}
}

void CMainFrame::OnQuickStartHelp(wxCommandEvent& WXUNUSED(event))
{
	gpApp->LogUserAction(_T("Initiated OnQuickStartHelp()"));
	// Note: The option to load the help file into the user's default browser
	// a better option for this reason: When we use AI's own CHtmlWindow class,
	// although it is shown as a modeless dialog, it cannot be accessed
	// (to scroll, click on links, etc.) while other Adapt It dialogs (such
	// as the Setup Paratext Collaboration dialog) are being shown modal.
	// Therefore, the CHtmlWindow dialog is limited in what can be shown if
	// the administrator wants to follow its setup instructions during
	// the setup of the user's Adapt It settings. Loading the html help file
	// into the user's default browser has the advantage in that it can be
	// accessed at the same time the administrator is doing his setup using
	// modal dialogs within Adapt It. He might need to switch back and forth
	// between AI and the browser, of course, depending on how much screen
	// disktop is available to work with.
	//
	// The "Adapt_It_Quick_Start.htm" file should go into the m_helpInstallPath
	// for each platform, which is determined by the GetDefaultPathForHelpFiles() call.
	wxString quickStartHelpFilePath = gpApp->GetDefaultPathForHelpFiles() + gpApp->PathSeparator + gpApp->m_quickStartHelpFileName;

#ifdef _USE_HTML_FILE_VIEWER
	// for testing the CHtmlFileViewer class dialog
	bool bSuccess = FALSE;
#else
	// for normal execution of the app
	bool bSuccess = TRUE;
#endif

	if (bSuccess)
	{
		wxLogNull nogNo;
		bSuccess = wxLaunchDefaultBrowser(quickStartHelpFilePath,wxBROWSER_NEW_WINDOW); // result of using wxBROWSER_NEW_WINDOW depends on browser's settings for tabs, etc.
	}
	if (!bSuccess)
	{
		wxString msg = _("Could not launch the default browser to open the HTML file's URL at:\n\n%s\n\nYou may need to set your system's settings to open the .htm file type in your default browser.\n\nDo you want Adapt It to show the Help file in its own HTML viewer window instead?");
		msg = msg.Format(msg, quickStartHelpFilePath.c_str());
		int response = wxMessageBox(msg,_("Browser launch error"),wxYES_NO);
		gpApp->LogUserAction(msg);
		if (response == wxYES)
		{
			wxString title = _("Adapt It Quick Start");
			gpApp->m_pHtmlFileViewer = new CHtmlFileViewer(gpApp->GetMainFrame(),&title,&quickStartHelpFilePath);
			gpApp->m_pHtmlFileViewer->Show(TRUE);
			gpApp->LogUserAction(_T("Launched Adapt_It_Quick_Start.htm in browser"));
		}
	}
	else
	{
		gpApp->LogUserAction(_T("Launched Adapt_It_Quick_Start.htm in browser"));
	}
}

/*
// whm removed 2Oct11
void CMainFrame::OnOnlineHelp(wxCommandEvent& WXUNUSED(event))
{
	gpApp->LogUserAction(_T("Initiated OnOnlineHelp()"));
	const wxString onlineHelpURL = _T("http://adaptit.martintribe.org/Using_Adapt_It.htm");
	int flags;
	flags = wxBROWSER_NEW_WINDOW;
	bool bLaunchOK;
	bLaunchOK = ::wxLaunchDefaultBrowser(onlineHelpURL, flags);
	if (!bLaunchOK)
	{
		wxString strMsg;
		strMsg = strMsg.Format(_T("Adapt It could not launch your browser to access the online help.\nIf you have Internet access, you can try launching your browser\nyourself and view the online Adapt It help site by writing down\nthe following Internet address and typing it by hand in your browser:\n %s"),onlineHelpURL.c_str());
		wxMessageBox(strMsg, _T("Error launching browser"), wxICON_WARNING);
		gpApp->LogUserAction(strMsg);
	}
}
*/

// whm 5Nov10 removed the Adapt It Forum... Help menu item because the Adapt It Talk forum
// was never well utilized its functionality was soon to be removed by Google at the end
// of 2010.
//void CMainFrame::OnUserForum(wxCommandEvent& WXUNUSED(event))
//{
//	const wxString userForumURL = _T("http://groups.google.com/group/AdaptIt-Talk");
//	int flags;
//	flags = wxBROWSER_NEW_WINDOW;
//	bool bLaunchOK;
//	bLaunchOK = ::wxLaunchDefaultBrowser(userForumURL, flags);
//	if (!bLaunchOK)
//	{
//		wxString strMsg;
//		strMsg = strMsg.Format(_T("Adapt It could not launch your browser to access the user forum.\nIf you have Internet access, you can try launching your browser\nyourself and go to the Adapt It user forum by writing down\nthe following Internet address and typing it by hand in your browser:\n %s"),userForumURL.c_str());
//		wxMessageBox(strMsg, _T("Error launching browser"), wxICON_WARNING);
//	}
//}

// Provide a direct way to report a problem. This is a menu item that can be used
// to compose a report and sent it from Adapt It either indirectly, going first
// to the user's email program, or directly via MAPI email protocols (On Window
// systems) or SendMail protocols (on Unix - Linux & Mac systems).
void CMainFrame::OnHelpReportAProblem(wxCommandEvent& WXUNUSED(event))
{
	gpApp->LogUserAction(_T("Initiated OnHelpReportAProblem()"));
	CEmailReportDlg erDlg(this);
	erDlg.Centre();
	erDlg.reportType = erDlg.Report_a_problem;
	if (erDlg.ShowModal() == wxID_OK)
	{
		// Assign any new settings to the App's corresponding members if we
		// detect any changes made in EmailReportDlg.
	}
}

// Provide a direct way to give user feedback. This is a menu item that can be
// used to compose a report and sent it from Adapt It either indirectly, going
// first to the user's email program, or directly via MAPI email protocols (On
// Window systems) or SendMail protocols (on Unix - Linux & Mac systems).
void CMainFrame::OnHelpGiveFeedback(wxCommandEvent& WXUNUSED(event))
{
	gpApp->LogUserAction(_T("Initiated OnHelpGiveFeedback()"));
	CEmailReportDlg erDlg(this);
	erDlg.reportType = erDlg.Give_feedback;
	if (erDlg.ShowModal() == wxID_OK)
	{
		// Assign any new settings to the App's corresponding members if we
		// detect any changes made in EmailReportDlg.
	}
}

void CMainFrame::OnUpdateUseToolTips(wxUpdateUIEvent& event)
{
	// always available
	event.Enable(TRUE);
}

void CMainFrame::OnUseToolTips(wxCommandEvent& WXUNUSED(event))
{
	//wxMenuBar* pMenuBar = this->GetMenuBar();
	//wxASSERT(pMenuBar != NULL);
	//wxMenuItem * pUseToolTips = pMenuBar->FindItem(ID_HELP_USE_TOOLTIPS);
	//wxASSERT(pUseToolTips != NULL);

	wxLogNull logNo; // avoid spurious messages from the system

	if (gpApp->m_bUseToolTips)
	{
		wxToolTip::Enable(FALSE);
		GetMenuBar()->Check(ID_HELP_USE_TOOLTIPS,FALSE); //pUseToolTips->Check(FALSE);
		gpApp->m_bUseToolTips = FALSE;
	}
	else
	{
		wxToolTip::Enable(TRUE);
		GetMenuBar()->Check(ID_HELP_USE_TOOLTIPS,TRUE); //pUseToolTips->Check(TRUE);
		gpApp->m_bUseToolTips = TRUE;
	}

	// save current state of m_bUseToolTips in registry/hidden settings file
	wxString oldPath = gpApp->m_pConfig->GetPath(); // is always absolute path "/Recent_File_List"
	gpApp->m_pConfig->SetPath(_T("/Settings"));

	gpApp->m_pConfig->Write(_T("use_tooltips"), gpApp->m_bUseToolTips);

	gpApp->m_pConfig->Flush(); // write now, otherwise write takes place when m_p is destroyed in OnExit().
	// restore the oldPath back to "/Recent_File_List"
	gpApp->m_pConfig->SetPath(oldPath);
}

// Support Mike's testing of DVCS:

#if defined(TEST_DVCS)

int lastResultCode;

void CMainFrame::OnUpdateDVCS_Version (wxUpdateUIEvent& event)
{
	// always available
	event.Enable(TRUE);
}

void CMainFrame::OnDVCS_Version (wxCommandEvent& WXUNUSED(event))
{    
	int resultCode = CallDVCS (DVCS_VERSION);	
	lastResultCode = resultCode;		// avoid compiler warning about unused variable
}

void CMainFrame::OnInit_Repository (wxCommandEvent& WXUNUSED(event))
{
	int resultCode = CallDVCS (DVCS_INIT_REPOSITORY);
	lastResultCode = resultCode;
}

void CMainFrame::OnDVCS_Add_File (wxCommandEvent& WXUNUSED(event))
{
	int resultCode = CallDVCS (DVCS_ADD_FILE);
	lastResultCode = resultCode;
}

void CMainFrame::OnDVCS_Add_All_Files (wxCommandEvent& WXUNUSED(event))
{
	int resultCode = CallDVCS (DVCS_ADD_ALL_FILES);
	lastResultCode = resultCode;
}

void CMainFrame::OnDVCS_Remove_File (wxCommandEvent& WXUNUSED(event))
{
	int resultCode = CallDVCS (DVCS_REMOVE_FILE);
	lastResultCode = resultCode;
}

void CMainFrame::OnDVCS_Remove_Project (wxCommandEvent& WXUNUSED(event))
{
	int resultCode = CallDVCS (DVCS_REMOVE_PROJECT);
	lastResultCode = resultCode;
}

void CMainFrame::OnDVCS_Commit_File (wxCommandEvent& WXUNUSED(event))
{
	int resultCode = CallDVCS (DVCS_COMMIT_FILE);
	lastResultCode = resultCode;
}

void CMainFrame::OnDVCS_Commit_Project (wxCommandEvent& WXUNUSED(event))
{
	int resultCode = CallDVCS (DVCS_COMMIT_PROJECT);
	lastResultCode = resultCode;
}
void CMainFrame::OnDVCS_Log_File (wxCommandEvent& WXUNUSED(event))
{
	int resultCode = CallDVCS (DVCS_LOG_FILE);
	lastResultCode = resultCode;
}

void CMainFrame::OnDVCS_Log_Project (wxCommandEvent& WXUNUSED(event))
{
	int resultCode = CallDVCS (DVCS_LOG_PROJECT);
	lastResultCode = resultCode;
}

void CMainFrame::OnDVCS_Revert_File (wxCommandEvent& WXUNUSED(event))
{
	int resultCode = CallDVCS (DVCS_REVERT_FILE);
	lastResultCode = resultCode;
}

#endif

// TODO: uncomment EVT_MENU event handler for this function after figure out
// why SetDelay() disables tooltips
// BEW 26Mar10, no changes needed for support of doc version 5
void CMainFrame::OnSetToolTipDelayTime(wxCommandEvent& WXUNUSED(event))
{
	wxLogNull logNo; // avoid spurious messages from the system

	// we can only set the delay time if tooltips are in use
	if (gpApp->m_bUseToolTips)
	{
		// get a new time delay from user
		wxString prompt,caption,timeStr;
		caption = _("Change the amount of time tooltips are displayed");
		prompt = prompt.Format(
_("The current tooltip display time is %d milliseconds which is %d seconds.\nEnter a new amount of time in milliseconds:"),
		gpApp->m_nTooltipDelay,gpApp->m_nTooltipDelay / 1000);
		timeStr = ::wxGetTextFromUser(prompt,caption);
		timeStr.Trim(TRUE);
		timeStr.Trim(FALSE);
		if (timeStr.IsEmpty())
			timeStr << gpApp->m_nTooltipDelay;
		long msTime = (int)wxAtoi(timeStr);
		gpApp->m_nTooltipDelay = msTime;
		wxToolTip::SetDelay(msTime);
	}

	// save current state of m_bUseToolTips in registry/hidden settings file
	wxString oldPath = gpApp->m_pConfig->GetPath(); // is always absolute path "/Recent_File_List"
	gpApp->m_pConfig->SetPath(_T("/Settings"));

	gpApp->m_pConfig->Write(_T("time_tooltips_displayed_ms"), gpApp->m_nTooltipDelay);

	gpApp->m_pConfig->Flush(); // write now, otherwise write takes place when m_p is destroyed in OnExit().
	// restore the oldPath back to "/Recent_File_List"
	gpApp->m_pConfig->SetPath(oldPath);
}

// TODO: uncomment EVT_MENU event handler for this function after figure out
// why SetDelay() disables tooltips
// BEW 26Mar10, no changes needed for support of doc version 5
void CMainFrame::OnUpdateSetToolTipDelayTime(wxUpdateUIEvent& event)
{
	// enable the Set Time for Displaying ToolTips... menu item only if
	// tooltips are activated
	if (gpApp->m_bUseToolTips)
		event.Enable(TRUE);
	else
		event.Enable(FALSE);
}

/*
 Notes: ShowHelp uses section ids for displaying particular topics,
 but you might want to use a unique keyword to display a topic, instead.

 Section ids are specified as follows for the different formats.

 WinHelp

   The [MAP] section specifies the topic to integer id mapping, e.g.

   [MAP]
   #define intro       100
   #define functions   1
   #define classes     2
   #define about       3

   The identifier name corresponds to the label used for that topic.
   You could also put these in a .h file and #include it in both the MAP
   section and your C++ source.

   Note that Tex2RTF doesn't currently generate the MAP section automatically.

 MS HTML Help

   The [MAP] section specifies the HTML filename root to integer id mapping, e.g.

   [MAP]
   #define doc1       100
   #define doc3       1
   #define doc2       2
   #define doc4       3

   The identifier name corresponds to the HTML filename used for that topic.
   You could also put these in a .h file and #include it in both the MAP
   section and your C++ source.

   Note that Tex2RTF doesn't currently generate the MAP section automatically.

 Simple wxHTML Help and External HTML Help

   A wxhelp.map file is used, for example:

   0 wx.htm             ; wxWidgets: Help index; additional keywords like overview
   1 wx204.htm          ; wxWidgets Function Reference
   2 wx34.htm           ; wxWidgets Class Reference

   Note that Tex2RTF doesn't currently generate the MAP section automatically.

 Advanced HTML Help

   An extension to the .hhc file format is used, specifying a new parameter
   with name="ID":

   <OBJECT type="text/sitemap">
   <param name="Local" value="doc2.htm#classes">
   <param name="Name" value="Classes">
   <param name="ID" value=2>
   </OBJECT>

   Again, this is not generated automatically by Tex2RTF, though it could
   be added quite easily.

   Unfortunately adding the ID parameters appears to interfere with MS HTML Help,
   so you should not try to compile a .chm file from a .hhc file with
   this extension, or the contents will be messed up.
 */

/*
void CMainFrame::ShowHelp(int commandId, wxHelpControllerBase& helpController)
{
	int dummy;
	dummy = commandId;
	helpController.DisplayContents();
   //switch(commandId)
   //{
   //    case HelpDemo_Help_Classes:
   //    case HelpDemo_Html_Help_Classes:
   //    case HelpDemo_Advanced_Html_Help_Classes:
   //    case HelpDemo_MS_Html_Help_Classes:
   //    case HelpDemo_Best_Help_Classes:
   //       helpController.DisplaySection(2);
   //       //helpController.DisplaySection("Classes"); // An alternative form for most controllers
   //       break;

   //    case HelpDemo_Help_Functions:
   //    case HelpDemo_Html_Help_Functions:
   //    case HelpDemo_Advanced_Html_Help_Functions:
   //    case HelpDemo_MS_Html_Help_Functions:
   //       helpController.DisplaySection(1);
   //       //helpController.DisplaySection("Functions"); // An alternative form for most controllers
   //       break;

   //    case HelpDemo_Help_Help:
   //    case HelpDemo_Html_Help_Help:
   //    case HelpDemo_Advanced_Html_Help_Help:
   //    case HelpDemo_MS_Html_Help_Help:
   //    case HelpDemo_Best_Help_Help:
   //       helpController.DisplaySection(3);
   //       //helpController.DisplaySection("About"); // An alternative form for most controllers
   //       break;

   //    case HelpDemo_Help_Search:
   //    case HelpDemo_Html_Help_Search:
   //    case HelpDemo_Advanced_Html_Help_Search:
   //    case HelpDemo_MS_Html_Help_Search:
   //    case HelpDemo_Best_Help_Search:
   //    {
   //       wxString key = wxGetTextFromUser(_T("Search for?"),
   //                                        _T("Search help for keyword"),
   //                                        wxEmptyString,
   //                                        this);
   //       if(! key.IsEmpty())
   //          helpController.KeywordSearch(key);
   //    }
   //    break;

   //    case HelpDemo_Help_Index:
   //    case HelpDemo_Html_Help_Index:
   //    case HelpDemo_Advanced_Html_Help_Index:
   //    case HelpDemo_MS_Html_Help_Index:
   //    case HelpDemo_Best_Help_Index:
   //       helpController.DisplayContents();
   //       break;

   //    // These three calls are only used by wxExtHelpController

   //    case HelpDemo_Help_KDE:
   //       helpController.SetViewer(_T("kdehelp"));
   //       break;
   //    case HelpDemo_Help_GNOME:
   //       helpController.SetViewer(_T("gnome-help-browser"));
   //       break;
   //    case HelpDemo_Help_Netscape:
   //       helpController.SetViewer(_T("netscape"), wxHELP_NETSCAPE);
   //       break;
   //}
}
*/

// BEW 26Mar10, no changes needed for support of doc version 5
void CMainFrame::OnClose(wxCloseEvent& event)
{
	// OnClose() is always called regardless of how the user closes the program. It is called
	// if the user selects File | Exit, clicks on the x button in the title bar, or clicks on
	// "Close" on the system-menu of the main frame.
	// Sometimes the OnIdle() handler was still processing during the shut down process, which
	// was causing intermittent crashes. To prevent the OnIdle() handler from processing
	// during the shutdown process, I'm turning off the idle processes as soon as the main
	// frame begins to close.
	wxIdleEvent::SetMode(wxIDLE_PROCESS_SPECIFIED);
	wxUpdateUIEvent::SetMode(wxUPDATE_UI_PROCESS_SPECIFIED);
	event.Skip();	// to enable the close event to be propagated where needed elsewhere
					// to effect the closedown process
}

// BEW 26Mar10, no changes needed for support of doc version 5
#ifdef _USE_SPLITTER_WINDOW
// Creat a canvas for the splitter window held on the main frame.
CAdapt_ItCanvas *CMainFrame::CreateCanvas(wxSplitterWindow *parent)
#else
CAdapt_ItCanvas *CMainFrame::CreateCanvas(CMainFrame *parent)
#endif
{
	int width, height;
	parent->GetClientSize(&width, &height);

	// Create a canvas for the Adapt It main window
	CAdapt_ItCanvas* canvas = new CAdapt_ItCanvas(
		//view,
		parent,
		wxPoint(0, 0),
		wxSize(width, height),  // arbitrary initial size set same as main frame's client size; adjusted in OnSize
		wxVSCROLL ); //0 );
	wxASSERT(canvas != NULL);
	//canvas->SetCursor(wxCursor(wxCURSOR_PENCIL));

	// Give it scrollbars
	//canvas->SetScrollbars(20, 20, 50, 50); //arbitrary initial values - no data yet drawn on canvas
	// Note on SetScrollbars() above: This needs to be called here when the canvas is created,
	// otherwise the canvas will appear without scrollbars when a document is opened. The third and
	// fourth parameters (number of scroll units) is arbitrary and will be set to the proper values
	// once the virtual size of the canvas/doc is determined in RecalcLayout().

	canvas->SetScrollRate(20,20); // testing !!!

	//canvas->EnableScrolling(FALSE,TRUE); // testing !!!

	//canvas->SetClientSize(wxSize(800,600)); // NO!!! (see note below)
    // Note on SetClientSize() above: When SetClientSize was called here in CreateCanvas,
    // it is ignored in wxMSW, but in wxGTK, it appears to override the setting of the
    // canvas size by any later call to canvas->SetSize(), resulting in the lower part of
    // the canvas scrolled window and scrollbar being hidden below the bottom of the main
    // frame. In any case "Client Size" is more a property of our main frame than of our
    // canvas scrolled window. See our own GetCanvasClientSize() function here in
    // MainFrm.cpp.

	// Set the background of the canvas to white
	canvas->SetBackgroundColour(*wxWHITE);

	canvas->ClearBackground();

	return canvas; // return pointer to new canvas to caller
}

// whm added 24Feb07 after discovering that calling wxWindow::GetClientSize() on the
// canvas window directly does not give the same result on wxGTK as it does on wxMSW
// (the wxGTK return value is larger than the main frame's client window on wxGTK, but
// smaller on wxMSW). This function determines the canvas' client size by starting
// with the size of the main frame and removing the heights of the controlBar and
// composeBar (if present). It assumes that calling GetClientSize() on the frame returns
// a value that already accounts for the presence/absence of the menuBar, toolBar,
// and statusBar.
// BEW 26Mar10, no changes needed for support of doc version 5
wxSize CMainFrame::GetCanvasClientSize()
{
	wxSize frameClientSize, canvasSize;
	frameClientSize = GetClientSize();
	canvasSize.x = frameClientSize.x; // canvas always fills width of frame's client size
	// the height of the canvas is reduced by the height of the controlBar; and also the
	// height of the composeBar (if visible).
	// BEW modified for vertical edit bars to be taken into account also
	if (m_pComposeBar->IsShown())
	{
		canvasSize.y = frameClientSize.y - m_controlBarHeight - m_composeBarHeight;
	}
	else
	{
		canvasSize.y = frameClientSize.y - m_controlBarHeight;
	}
	if (gbVerticalEditInProgress)
	{
		// vertical edit is on currently
		if (m_pRemovalsBar->IsShown())
		{
			canvasSize.y -= m_removalsBarHeight;
		}
		if (m_pVertEditBar->IsShown())
		{
			canvasSize.y -= m_vertEditBarHeight;
		}
	}
	return canvasSize;
}

/////////////////////////////////////////////////////////////////////////////

// BEW 26Mar10, no changes needed for support of doc version 5
// whm 12Oct10 modified for user profiles support
// whm 7Jan12 modified for better viewing and hiding of tool bar
void CMainFrame::OnViewToolBar(wxCommandEvent& WXUNUSED(event))
{
	if (m_pToolBar != NULL)
	{
		if (gpApp->m_bToolBarVisible)
		{
			// Hide the tool bar
			m_pToolBar->Hide();
			GetMenuBar()->Check(ID_VIEW_TOOLBAR, FALSE);
			gpApp->LogUserAction(_T("Hide Tool bar"));
			gpApp->m_bToolBarVisible = FALSE;
			SendSizeEvent(); // needed to force redraw
		}
		else
		{
			// Show the tool bar
			m_pToolBar->Show(TRUE);
			GetMenuBar()->Check(ID_VIEW_TOOLBAR, TRUE);
			gpApp->LogUserAction(_T("Show Tool bar"));
			gpApp->m_bToolBarVisible = TRUE;
			SendSizeEvent(); // needed to force redraw
		}
	}
	else if (!gpApp->m_bToolBarVisible)
	{
        RecreateToolBar(); // whm 12Oct10 modified RecreateToolBar() for user profile compatibility
		wxMenuItem* pMenuItem = GetMenuBar()->FindItem(ID_VIEW_TOOLBAR);
		if (pMenuItem == NULL)
			return;
		GetMenuBar()->Check(ID_VIEW_TOOLBAR, TRUE);
		gpApp->m_bToolBarVisible = TRUE; // whm added 6Jan12
		gpApp->LogUserAction(_T("View Toolbar"));
		SendSizeEvent();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////
/// \return		nothing
/// \param      event   -> the wxUpdateUIEvent that is generated when the View Menu is about
///                         to be displayed
/// \remarks
/// Called from: The wxUpdateUIEvent mechanism when the associated menu item is selected, and before
/// the menu is displayed.
/// The "Toolbar" item on the View menu is always enabled by this handler.
/// BEW 26Mar10, no changes needed for support of doc version 5
////////////////////////////////////////////////////////////////////////////////////////////
void CMainFrame::OnUpdateViewToolBar(wxUpdateUIEvent& event)
{
	// View Toolbar menu toggle always available
    event.Enable(TRUE);
}

// BEW 26Mar10, no changes needed for support of doc version 5
// whm 7Jan12 modified for better viewing and hiding of status bar
void CMainFrame::OnViewStatusBar(wxCommandEvent& WXUNUSED(event))
{
	if (m_pStatusBar != NULL)
	{
		if (gpApp->m_bStatusBarVisible)
		{
			// Hide the status bar
			m_pStatusBar->Hide();
			GetMenuBar()->Check(ID_VIEW_STATUS_BAR, FALSE);
			gpApp->LogUserAction(_T("Hide Tool bar"));
			gpApp->m_bStatusBarVisible = FALSE;
			SendSizeEvent(); // needed to force redraw
		}
		else
		{
			// Show the status bar
			m_pStatusBar->Show(TRUE);
			GetMenuBar()->Check(ID_VIEW_STATUS_BAR, TRUE);
			gpApp->LogUserAction(_T("Show Tool bar"));
			gpApp->m_bStatusBarVisible = TRUE;
			SendSizeEvent(); // needed to force redraw
		}
	}
	else if (!gpApp->m_bStatusBarVisible)
	{
        DoCreateStatusBar();
		GetMenuBar()->Check(ID_VIEW_STATUS_BAR, TRUE);
		gpApp->m_bStatusBarVisible = TRUE; // whm added 6Jan12
		gpApp->LogUserAction(_T("View Statusbar"));
	}
	// update the status bar info
	gpApp->RefreshStatusBarInfo();
	/*
	CAdapt_ItApp* pApp = &wxGetApp();
    wxStatusBar *statbarOld = GetStatusBar();
    if ( statbarOld )
    {
        statbarOld->Hide();
        SetStatusBar(0);
		GetMenuBar()->Check(ID_VIEW_STATUS_BAR, FALSE);
		pApp->m_bStatusBarVisible = FALSE; // whm added 6Jan12
		pApp->LogUserAction(_T("Hide Statusbar"));
    }
    else
    {
        DoCreateStatusBar();
		GetMenuBar()->Check(ID_VIEW_STATUS_BAR, TRUE);
		pApp->m_bStatusBarVisible = TRUE; // whm added 6Jan12
		pApp->LogUserAction(_T("View Statusbar"));
    }
	// Need to call SendSizeEvent() for the frame to redraw itself after the
	// status bar is hidden at the bottom of the frame. Otherwise a status bar
	// ghost remains until the frame is resized or redrawn, rather than being
	// hidden immediately (see the __WXMSW__ conditional define below).
#ifdef __WXMSW__
    // The following is a kludge suggested by Vadim Zeitlin (one of the wxWidgets
    // authors) while we look for a proper fix..
    SendSizeEvent();
#endif
	*/
}

////////////////////////////////////////////////////////////////////////////////////////////
/// \return		nothing
/// \param      event   -> the wxUpdateUIEvent that is generated when the View Menu is about
///                         to be displayed
/// \remarks
/// Called from: The wxUpdateUIEvent mechanism when the associated menu item is selected, and before
/// the menu is displayed.
/// The "Status Bar" item on the View menu is always enabled by this handler.
/// BEW 26Mar10, no changes needed for support of doc version 5
////////////////////////////////////////////////////////////////////////////////////////////
void CMainFrame::OnUpdateViewStatusBar(wxUpdateUIEvent& event)
{
	// View StarusBar menu toggle always available
    event.Enable(TRUE);
}

// whm added 20Jul11 to provide easier access to the Administrator menu
// by having a "Show Administrator Menu... (Password protected)" menu
// item also located in the top level "View" menu (this is now in
// addition to a checkbox of the same label in the Edit | Preferences...
// | View tab page.
// Note: The Administrator menu is not subject to being made visible or
// invisible within the user profiles mecahnism. It strictly is made
// visible or invisible according to the Administrator's password
// protected actions.
void CMainFrame::OnViewAdminMenu(wxCommandEvent& WXUNUSED(event))
{
	CAdapt_ItApp* pApp = (CAdapt_ItApp*)&wxGetApp();
	//wxMenuBar* pMenuBar = pApp->GetMainFrame()->GetMenuBar();
	//wxASSERT(pMenuBar != NULL);
	pApp->LogUserAction(_T("Initiated OnViewAdminMenu"));

	if (pApp->m_bShowAdministratorMenu)
	{
		// The menu is currently shown and administrator wants it hidden, no password required
		// for hiding it.
		pApp->m_bShowAdministratorMenu = FALSE;
		pApp->LogUserAction(_T("Hide Administrator menu"));
	}
	else
	{
		// Someone wants to have the administrator menu made visible, this requires a
		// password and we always accept the secret default "admin" password; note,
		// although we won't document the fact, anyone can type an arbitrary password
		// string in the relevant line of the basic configuration file, and the code below
		// will accept it when next that config file is read in - ie. at next launch.
		wxString message = _("Access to Administrator privileges requires that you type a password");
		wxString caption = _("Type Administrator Password");
		wxString default_value = _T("");
		wxString password = ::wxGetPasswordFromUser(message,caption,default_value,this);
		if (password == _T("admin") ||
			(password == pApp->m_adminPassword && !pApp->m_adminPassword.IsEmpty()))
		{
			// a valid password was typed
			pApp->m_bShowAdministratorMenu = TRUE;
			pApp->LogUserAction(_T("Valid password entered - show Administrator menu"));
		}
		else
		{
			// invalid password - turn the checkbox back off, beep also
			::wxBell();
			pApp->LogUserAction(_T("Invalid password entered"));
			// whm Note: The "Show Administrator Menu... (Password protected)" menu
			// item is a toggle menu item. Its toggle state does not need to be
			// explicitly changed here.
		}
	}
	// Call the App's MakeMenuInitializationsAndPlatformAdjustments() to made the
	// Administrator menu visible/hidden and verify its toggle state
	pApp->MakeMenuInitializationsAndPlatformAdjustments(collabIndeterminate);
}


// whm added 20Jul11 to provide easier access to the Administrator menu
void CMainFrame::OnUpdateViewAdminMenu(wxUpdateUIEvent& event)
{
	// View "Show Administrator Menu... (Password protected)" menu toggle always available
    event.Enable(TRUE);
}

// BEW 26Mar10, no changes needed for support of doc version 5
void CMainFrame::OnSize(wxSizeEvent& WXUNUSED(event))
{
    // wx version notes about frame size changes:
    // CMainFrame is Adapt It's primary application frame or window. The CMainFrame is the
    // parent frame for everything that happens in Adapt It, including all first level
    // dialogs. CMainFrame knows about some of the windows attached to its frame, but
    // not others. It knows about the menu bar, tool bar and status bar. For those it
    // can adjust the value returned by GetClientSize to reflect the remaining size
    // available to application windows. But there are other windows that can appear in
    // the main frame and we must account for those. These are the m_pControlBar, the
    // m_pComposeBar, m_pRemovalsBar, m_pVertEditBar, and the actual canvas which the
    // user knows as Adapt It's "main window."
    //
    // These windows are managed here in OnSize, which dynamically adjusts the size of
    // mainFrameClientSize depending on which of these window elements are visible on the
    // main frame at any given time. OnSize insures that they fill their appropriate places
    // within the main frame and automatically resize them when individual elements are
    // made visible or invisible. The canvas always fills the remaining space in the main
    // frame.
    //
    // The wxWidgets' cross-platform library utilizes sizers which could be used to manage
    // the appearance of all the non-standard windows in Adapt It's CMainFrame (including
    // the the m_pControlBar, the m_pComposeBar, m_pRemovalsBar, m_pVertEditBar, and the
    // canvas window which fills all remaining space within the main frame), however,
    // because of the need to explicitly understand what is being done, and potentially
    // different behavior on wxGTK (and possibly wxMAC - see notes below) I've done the
    // specific calculations of these things here in CMainFrame's OnSize(). All of the
    // frame elements are created in the CMainFrame's constructor. The m_pControlBar, the
    // m_pComposeBar, m_pRemovalsBar, and the m_pVertEditBar simply remain hidden until
    // made to appear by menu command or by the vertical process (may also be done custom
    // event handlers).

	// We would do the following if we were going to control the main window's layout
	// with sizers:
	//Layout();	// causes the layout of controlBar, composeBar (if shown) and the canvas
				// within mainSizer to be adjusted according to the new size of the main
				// frame.

	// OnSize() is called the first time before the Main Frame constructor is finished and
	// before the controlBar, composeBar, and canvas are created, so check if any of these
	// are null and return from OnSize() if they do not exist yet.
	if (m_pControlBar == NULL || m_pComposeBar == NULL
		|| m_pRemovalsBar == NULL || m_pVertEditBar == NULL  // these added for vertical edit process
		|| canvas == NULL)
		return;

	// Notes:
	// 1. OnSize is called three times on application start, and once each time
	// the user does an actual mainframe resize
	// 2. The toolBar is managed by the doc/view framework, which also seems to
	// usually take care of redrawing the frame and client window when the
	// toolbar is toggled on and off.
	// 3. Under wxGTK calling GetClientSize on the canvas rather than on the main frame
	// produces quite different results than doing so on wxMSW (in the wxMSW case the
	// result for the client size of the canvas is smaller than the client size of the
	// main frame; but for wxGTK, the client size of the canvas is somewhat LARGER than
	// the client size of the main frame. This caused the wx app on wxGTK to not size the
	// canvas properly within the main frame (the canvas and its scroll bar were partially
	// hidden below the bottom of the main frame). Although using a mainFrameSizer would
	// probably work OK to manage the extra "bar" elements, and the canvas within the main
	// frame, especially once GetClientSize() is obtained from the main frame rather than
	// from the canvas itself, I've opted to place those windows within the main frame
	// manually (apart from using mainFrameSizer) so as to be able to better determine
	// the source of other scrolling problems.

    // First, set the position of the m_pControlBar in the main frame's client area. Note:
    // the m_pControlBar is never hidden, but always visible on the main frame.
	wxSize mainFrameClientSize;
	mainFrameClientSize = GetClientSize(); // determine the size of the main frame's client window

    // The VertDisplacementFromReportedMainFrameClientSize value is used to keep track of
    // the screen coord y displacement from the original GetClientSize() call above on this
    // main window frame. It represents the how far down inside the main frame's client
    // area we need to go to place any of the potentially visible "bars" in the main
    // window, and ultimately also the placement of the upper left corner of the canvas
    // itself (which fills the remainder of the client area. We start with the displacement
    // represented by the m_controlBarHeight if visible, and proceed with the other visible
    // elements.
	int VertDisplacementFromReportedMainFrameClientSize = 0;
    // The FinalHeightOfCanvas that we end up placing starts with the available height of
    // the mainFrameClientSize as determined by the GetClientSize() call above, now reduced
    // by the height of our always visible controlBar.
	int finalHeightOfCanvas = mainFrameClientSize.y;
	// The upper left position of the main frame's client area is always 0,0 so we position
    // the controlBar there if it is visible.
    // Note: SetSize sets both the position and the size.
    // SetSize() uses upper left coordinates in pixels x and y, plus a width and
    // height also in pixels. Its signature is SetSize(int x, int y, int width, int
    // height).

	// Adjust the Mode Bar's position in the main frame (if controlBar is visible).
	if (m_pControlBar->IsShown())
	{
		m_pControlBar->SetSize(0, 0, mainFrameClientSize.x, m_controlBarHeight);
						// width is mainFrameClientSize.x, height is m_controlBarHeight
		m_pControlBar->Refresh(); // this is needed to repaint the controlBar after OnSize

		// Increment VertDisplacementFromReportedMainFrameClientSize for the next placement
		VertDisplacementFromReportedMainFrameClientSize += m_controlBarHeight;
		finalHeightOfCanvas -= m_controlBarHeight;
	}

    // Next, set the size and placement for each of the visible "bars" that appear at the
    // top of the client area (under the controlBar. We increment the value of
    // VertDisplacementFromReportedMainFrameClientSize for each one that is visible (so
    // we'll know where to size/place the canvas after all other elements are sized/placed.

	// Adjust the composeBar's position in the main frame (if composeBar is visible).
	if (m_pComposeBar->IsShown())
	{
        // The composeBar is showing, so set its position (with existing size) just below
        // the controlBar. The upper left y coord now is represented by
        // VertDisplacementFromReportedMainFrameClientSize.
		m_pComposeBar->SetSize(0, VertDisplacementFromReportedMainFrameClientSize,
								mainFrameClientSize.x, m_composeBarHeight);
		m_pComposeBar->Refresh();

		// Increment VertDisplacementFromReportedMainFrameClientSize for the next placement
		VertDisplacementFromReportedMainFrameClientSize += m_composeBarHeight;
		finalHeightOfCanvas -= m_composeBarHeight;
	}

    // Next, adjust the removalsBar's position in the main frame (if removalsBar is
    // visible), and set the canvas' position and size to accommodate it.
	if (m_pRemovalsBar->IsShown())
	{
        // The removalsBar is showing, so set its position (with existing size) just below
        // the last placed element.
        // The upper left y coord now is represented by VertDisplacementFromReportedMainFrameClientSize.
		m_pRemovalsBar->SetSize(0, VertDisplacementFromReportedMainFrameClientSize,
									mainFrameClientSize.x, m_removalsBarHeight);
		pRemovalsBarSizer->Layout(); //m_pRemovalsBar->Refresh();

		// Increment VertDisplacementFromReportedMainFrameClientSize for the next placement
		VertDisplacementFromReportedMainFrameClientSize += m_removalsBarHeight;
		finalHeightOfCanvas -= m_removalsBarHeight;
	}

     // Next, adjust the verteditBar's position in the main frame (if verteditBar is
     // visible), and set the canvas' position and size to accommodate it.
	if (m_pVertEditBar->IsShown())
	{
        // The verteditBar is showing, so set its position (with existing size) just below
        // the last placed element.
        // The upper left y coord now is represented by VertDisplacementFromReportedMainFrameClientSize.
		m_pVertEditBar->SetSize(0, VertDisplacementFromReportedMainFrameClientSize,
									mainFrameClientSize.x, m_vertEditBarHeight);
		pVertEditBarSizer->Layout(); //m_pVertEditBar->Refresh();

		// Increment VertDisplacementFromReportedMainFrameClientSize for the next placement
		VertDisplacementFromReportedMainFrameClientSize += m_vertEditBarHeight;
		finalHeightOfCanvas -= m_vertEditBarHeight;
	}

#ifdef _USE_SPLITTER_WINDOW
    // whm Note: If the splitter window IsSplit, we have two versions of the canvas, one in
    // the top splitter window and one in the bottom splitter window. To size things
    // properly in this case, we check to see if splitter->IsSplit() returns TRUE or FALSE.
    // If FALSE we set the size of the single window to be the size of the remaining client
    // size in the frame. If TRUE, we set Window1 and Window2 separately depending on their
    // split sizes.
	wxWindow* pWindow1;
	wxWindow* pWindow2;
	wxSize win1Size;
	wxSize win2Size;
	//if (splitter->IsSplit())
	//{
	//	pWindow1 = splitter->GetWindow1();
	//	pWindow2 = splitter->GetWindow2();
	//	win1Size = pWindow1->GetSize();
	//	win2Size = pWindow2->GetSize();

	//}
	//else
	//{
		splitter->SetSize(0, VertDisplacementFromReportedMainFrameClientSize,
			mainFrameClientSize.x, finalHeightOfCanvas);
	//}
#else
		// arguments: top left x and y coords of canvas within client area, then
		// width and height of canvas
		canvas->SetSize(0, VertDisplacementFromReportedMainFrameClientSize,
						mainFrameClientSize.x, finalHeightOfCanvas);
#endif

    // whm Note: The remainder of OnSize() is taken from MFC's View::OnSize() routine BEW
    // added 05Mar06: Bill Martin reported (email 3 March) that if user is in free
    // translation mode, and uses either the Shorten of Lengthen buttons (these set
    // gbSuppressSetup to TRUE in the beginning of their handlers, to suppress resetting up
    // of the section for the change in the layout), and then he resizes the window, the
    // app will crash (invalid pile pointer usually). The easiest solution is just to allow
    // the section to be reset - this loses the effect of the shortening or lengthening,
    // but that can easily be done again by hitting the relevant button(s) after the
    // resized layout is redrawn.
    // Note: gpApp may not be initialized yet in the App's OnInit(), so we'll get a pointer
    // to the app here:
	CAdapt_ItApp* pApp = (CAdapt_ItApp*)&wxGetApp();
	if (pApp->m_bFreeTranslationMode)
	{
		gbSuppressSetup = FALSE;
	}

	// need to initiate a recalc of the layout with new m_docSize value, since strip-wrap is on
	// BEW added 6May09 -- but only provided the pile list is not currently empty, and
	// then only provided vertical edit is not in effect
	CAdapt_ItView* pView = (CAdapt_ItView*) pApp->GetView();
	CLayout* pLayout = pView->GetLayout();
	if (pView && !pLayout->GetPileList()->IsEmpty())
	{
#ifdef _NEW_LAYOUT
		pLayout->RecalcLayout(pApp->m_pSourcePhrases, create_strips_keep_piles);
#else
		pLayout->RecalcLayout(pApp->m_pSourcePhrases, create_strips_keep_piles);
#endif
		pApp->m_pActivePile = pView->GetPile(pApp->m_nActiveSequNum);
	}
	// code below was in docview sample
    // FIXME?: On wxX11, we need the MDI frame to process this
    // event, but on other platforms this should not
    // be done.
#ifdef __WXUNIVERSAL__
    event.Skip();
#endif

	if (pView && !pLayout->GetPileList()->IsEmpty())
	{
		pView->Invalidate();
		gpApp->m_pLayout->PlaceBox();
	}
}

// BEW 26Mar10, no changes needed for support of doc version 5
// whm 12Oct10 modified for configurable tool bar under user profiles
void CMainFrame::RecreateToolBar()
{
	CAdapt_ItApp* pApp = (CAdapt_ItApp*)&wxGetApp();
	// delete and recreate the toolbar
    AIToolBar *toolBar = GetToolBar();
    delete toolBar;
	toolBar = (AIToolBar*)NULL;
    SetToolBar(NULL);

    long style = /*wxNO_BORDER |*/ wxTB_FLAT | wxTB_HORIZONTAL;

    // We do not use CreateToolBar() now in the wx version. It is a method of
    // the Frame class and can be used to inform the doc/view of the existence
    // of the toolbar so the client window can allow space for it, but we manage
    // the calculations for the size of the client window on our own.
	//toolBar = new wxToolBar(this, -1, wxDefaultPosition, wxDefaultSize, style);
	toolBar = new AIToolBar(this, -1, wxDefaultPosition, wxDefaultSize, style);
	wxASSERT(toolBar != NULL);
	m_pToolBar = toolBar;

	if (gpApp->m_bExecutingOnXO)
	{
		toolBar->SetToolBitmapSize(wxSize(32,30));
		AIToolBar32x30Func( toolBar );
		// see the App's ConfigureToolBarForUserProfile() for notes on the following call
		pApp->RemoveToolBarItemsFromToolBar(toolBar);
	}
	else
	{
		AIToolBarFunc( toolBar );
		// see the App's ConfigureToolBarForUserProfile() for notes on the following call
		pApp->RemoveToolBarItemsFromToolBar(toolBar);
	}
	SetToolBar(toolBar);

	m_pToolBar = GetToolBar();
	wxASSERT(m_pToolBar == toolBar);

	wxSize toolBarSize;
	toolBarSize = m_pToolBar->GetSize();
	m_toolBarHeight = toolBarSize.GetHeight();
}

// this overrides the wxFrame::GetToolBar() method which returns a wxToolBar*
AIToolBar* CMainFrame::GetToolBar()
{
	return m_pToolBar;
}

// BEW 26Mar10, no changes needed for support of doc version 5
void CMainFrame::DoCreateStatusBar()
{
    wxStatusBar *statbarOld = GetStatusBar();
	// When the status bar is hidden GetStatusBar() returns 0 (false)
    if ( statbarOld )
    {
        statbarOld->Hide();
    }
    SetStatusBar(m_pStatusBar); // reassign the m_pStatusBar to the main frame
    GetStatusBar()->Show();
    PositionStatusBar();
}

// the next 5 handlers use the ON_UPDATE_COMMAND_UI mechanism to ensure the visible
// state of the checkboxes in the dialogbar is kept in synch with the current value
// of the relevant boolean flags (which are owned by CAdapt_ItView)

////////////////////////////////////////////////////////////////////////////////////////////
/// \return		nothing
/// \param      event   -> the wxUpdateUIEvent that is generated by the update idle mechanism
/// \remarks
/// Called from: The wxUpdateUIEvent mechanism during idle time processing.
/// This update handler insures that the "Automatic" checkbox is checked when the App's
/// m_bSingleStep flag is FALSE, and unchecked when the flag is TRUE.
/// BEW 26Mar10, no changes needed for support of doc version 5
////////////////////////////////////////////////////////////////////////////////////////////
void CMainFrame::OnUpdateCheckSingleStep(wxUpdateUIEvent& event)
{
	// This checkbox control was renamed to have "Automatic" as its label.
	// Note: OnUpdateCheckSingleStep() only is used here in CMainFrame. In the MFC
	// code UpdateCheckSingleStep() is not used in CMainFrame, only in CAdapt_ItView.
	// the flags we want are on the view, so get the view
	CAdapt_ItApp* pApp = &wxGetApp();
	wxASSERT(pApp != NULL);
	CAdapt_ItView* pView = (CAdapt_ItView*) pApp->GetView();
	if (pView != NULL)
	{
		wxASSERT(pView->IsKindOf(CLASSINFO(CAdapt_ItView)));
		event.Check(!pApp->m_bSingleStep);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////
/// \return		nothing
/// \param      event   -> the wxUpdateUIEvent that is generated by the update idle mechanism
/// \remarks
/// Called from: The wxUpdateUIEvent mechanism during idle time processing.
/// This update handler insures that the "Save To Knowledge Base" checkbox is checked when the
/// App's m_bSaveToKB flag is TRUE, and unchecked when the flag is FALSE.
/// BEW 26Mar10, no changes needed for support of doc version 5
////////////////////////////////////////////////////////////////////////////////////////////
void CMainFrame::OnUpdateCheckKBSave(wxUpdateUIEvent& event)
{
	// the flags we want are on the view, so get the view
	CAdapt_ItApp* pApp = &wxGetApp();
	wxASSERT(pApp != NULL);

	// whm added 26Mar12. Disable the Save to Knowledge Base checkbox when in read-only mode
	if (pApp->m_bReadOnlyAccess)
	{
		event.Enable(FALSE);
		return;
	}

	CAdapt_ItView* pView = (CAdapt_ItView*) pApp->GetView();
	if (pView != NULL)
	{
		wxASSERT(pView->IsKindOf(CLASSINFO(CAdapt_ItView)));
		event.Check(pApp->m_bSaveToKB);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////
/// \return		nothing
/// \param      event   -> the wxUpdateUIEvent that is generated by the update idle mechanism
/// \remarks
/// Called from: The wxUpdateUIEvent mechanism during idle time processing.
/// This update handler insures that the "Force Choice For This Item" checkbox is checked when the
/// App's m_bForceAsk flag is TRUE, and unchecked when the flag is FALSE.
/// BEW 26Mar10, no changes needed for support of doc version 5
////////////////////////////////////////////////////////////////////////////////////////////
void CMainFrame::OnUpdateCheckForceAsk(wxUpdateUIEvent& event)
{
	// the flags we want are on the view, so get the view
	CAdapt_ItApp* pApp = &wxGetApp();
	wxASSERT(pApp != NULL);
	
	if (pApp->m_bReadOnlyAccess)
	{
		event.Enable(FALSE);
		return;
	}
	
	CAdapt_ItView* pView = (CAdapt_ItView*) pApp->GetView();
	if (pView != NULL)
	{
		wxASSERT(pView->IsKindOf(CLASSINFO(CAdapt_ItView)));
		event.Check(pApp->m_bForceAsk);
	}
}

// BEW 26Mar10, no changes needed for support of doc version 5
void CMainFrame::OnViewComposeBar(wxCommandEvent& WXUNUSED(event))
{
	gpApp->m_bComposeBarWasAskedForFromViewMenu = TRUE;
	if (m_pComposeBar->IsShown())
		ComposeBarGuts(composeBarHide);
	else
		ComposeBarGuts(composeBarShow);
}

void CMainFrame::OnViewModeBar(wxCommandEvent& WXUNUSED(event))
{
	//wxMessageBox(_T("Not yet implemented!"),_T("Mode Bar"),wxICON_INFORMATION);
	wxView* pView = gpApp->GetView();
	if (pView != NULL)
	{
		wxASSERT(pView->IsKindOf(CLASSINFO(CAdapt_ItView)));
		// toggle the mode bar's flag and its visibility
		if (gpApp->m_bModeBarVisible)
		{
			// Hide it
			m_pControlBar->Hide();
			GetMenuBar()->Check(ID_VIEW_MODE_BAR, FALSE);
			gpApp->LogUserAction(_T("Hide Mode bar"));
			gpApp->m_bModeBarVisible = FALSE; // used in OnUpdateViewModeeBar() handler
			SendSizeEvent(); // needed to force redraw
		}
		else
		{
			// Show the composeBar
			m_pControlBar->Show(TRUE);
			GetMenuBar()->Check(ID_VIEW_MODE_BAR, TRUE);
			gpApp->LogUserAction(_T("Show Mode bar"));
			gpApp->m_bModeBarVisible = TRUE; // used in OnUpdateViewComposeBar() handler
			SendSizeEvent(); // needed to force redraw
		}

		if (!m_pControlBar->IsShown())
		{
			// the bar has just been made invisible
			// restore focus to the targetBox, if it is visible
			if (gpApp->m_pTargetBox != NULL)
				if (gpApp->m_pTargetBox->IsShown())
					gpApp->m_pTargetBox->SetFocus();
		}
	}
}

// BEW 22Feb10 no changes needed for support of doc version 5
// BEW 9July10, no changes needed for support of kbVersion 2
void CMainFrame::ComposeBarGuts(enum composeBarViewSwitch composeBarVisibility)
{
	if (m_pComposeBar == NULL)
		return;

	// the flags we want are on the view, so get the view
	CAdapt_ItApp* pApp = (CAdapt_ItApp*)&wxGetApp();
	wxASSERT(pApp);
	CAdapt_ItView* pAppView;
	CAdapt_ItDoc* pDoc;
	CPhraseBox* pBox;
	pApp->GetBasePointers(pDoc,pAppView,pBox);

    // depending on which command invoked this code, hide some buttons and show others --
    // there are two buttons shown when invoked from the View menu, and six different
    // buttons shown when invoked from the Advanced menu in order to turn on or off free
    // translation mode
	if (gpApp->m_bComposeBarWasAskedForFromViewMenu)
	{
		// show the Clear Contents and Select All buttons, hide the rest
		wxButton* pButton = (wxButton*)m_pComposeBar->FindWindowById(IDC_BUTTON_APPLY);
		pButton->Show(FALSE);
		pButton = (wxButton*)m_pComposeBar->FindWindowById(IDC_BUTTON_NEXT);
		pButton->Show(FALSE);
		pButton = (wxButton*)m_pComposeBar->FindWindowById(IDC_BUTTON_PREV);
		pButton->Show(FALSE);
		pButton = (wxButton*)m_pComposeBar->FindWindowById(IDC_BUTTON_REMOVE);
		pButton->Show(FALSE);
		pButton = (wxButton*)m_pComposeBar->FindWindowById(IDC_BUTTON_LENGTHEN);
		pButton->Show(FALSE);
		pButton = (wxButton*)m_pComposeBar->FindWindowById(IDC_BUTTON_SHORTEN);
		pButton->Show(FALSE);
		wxRadioButton* pRadioButton = (wxRadioButton*)m_pComposeBar->FindWindowById(IDC_RADIO_PUNCT_SECTION);
		pRadioButton->Show(FALSE);
		pRadioButton = (wxRadioButton*)m_pComposeBar->FindWindowById(IDC_RADIO_VERSE_SECTION);
		pRadioButton->Show(FALSE);
		wxStaticText* pStatic = (wxStaticText*)m_pComposeBar->FindWindowById(IDC_STATIC_SECTION_DEF);
		pStatic->Show(FALSE);
		// show these two only
		pButton = (wxButton*)m_pComposeBar->FindWindowById(IDC_BUTTON_CLEAR);
		pButton->Show(TRUE);
		pButton = (wxButton*)m_pComposeBar->FindWindowById(IDC_BUTTON_SELECT_ALL);
		pButton->Show(TRUE);
	}
	else
	{
		// free translation mode, hide the Clear Contents and Select All buttons, show the rest
		wxButton* pButton = (wxButton*)m_pComposeBar->FindWindowById(IDC_BUTTON_CLEAR);
		pButton->Show(FALSE);
		pButton = (wxButton*)m_pComposeBar->FindWindowById(IDC_BUTTON_SELECT_ALL);
		pButton->Show(FALSE);

		// show these ones
		pButton = (wxButton*)m_pComposeBar->FindWindowById(IDC_BUTTON_APPLY);
		pButton->Show(TRUE);
		pButton = (wxButton*)m_pComposeBar->FindWindowById(IDC_BUTTON_NEXT);
		pButton->Show(TRUE);
		pButton = (wxButton*)m_pComposeBar->FindWindowById(IDC_BUTTON_PREV);
		pButton->Show(TRUE);
		pButton = (wxButton*)m_pComposeBar->FindWindowById(IDC_BUTTON_REMOVE);
		pButton->Show(TRUE);
		pButton = (wxButton*)m_pComposeBar->FindWindowById(IDC_BUTTON_LENGTHEN);
		pButton->Show(TRUE);
		pButton = (wxButton*)m_pComposeBar->FindWindowById(IDC_BUTTON_SHORTEN);
		pButton->Show(TRUE);
		wxRadioButton* pRadioButton = (wxRadioButton*)m_pComposeBar->FindWindowById(IDC_RADIO_PUNCT_SECTION);
		pRadioButton->Show(TRUE);
		// set the value
		if (gpApp->m_bDefineFreeTransByPunctuation)
			pRadioButton->SetValue(TRUE);
		else
			pRadioButton->SetValue(FALSE);
		pRadioButton = (wxRadioButton*)m_pComposeBar->FindWindowById(IDC_RADIO_VERSE_SECTION);
		pRadioButton->Show(TRUE);
		// set the value
		if (!gpApp->m_bDefineFreeTransByPunctuation)
			pRadioButton->SetValue(TRUE);
		else
			pRadioButton->SetValue(FALSE);
		wxStaticText* pStatic = (wxStaticText*)m_pComposeBar->FindWindowById(IDC_STATIC_SECTION_DEF);
		pStatic->Show(TRUE);
	}

	wxView* pView = pApp->GetView();
	if (pView != NULL) // only have one kind of view in WX version
	{
		wxASSERT(pView->IsKindOf(CLASSINFO(CAdapt_ItView)));
		// toggle the compose bar's flag and its visibility - whm: simplified a little from MFC version
		if (composeBarVisibility == composeBarHide) //if (m_pComposeBar->IsShown())
		{
			// Hide it
			m_pComposeBar->Hide();
			GetMenuBar()->Check(ID_VIEW_COMPOSE_BAR, FALSE);
			gpApp->LogUserAction(_T("Hide Composebar"));
			gpApp->m_bComposeWndVisible = FALSE; // used in OnUpdateViewComposeBar() handler
			gpApp->m_bComposeBarWasAskedForFromViewMenu = FALSE; // needed for free translation mode
			SendSizeEvent(); // needed to force redraw
		}
		else if (composeBarVisibility == composeBarShow)
		{
			// Show the composeBar
			m_pComposeBar->Show(TRUE);
			GetMenuBar()->Check(ID_VIEW_COMPOSE_BAR, TRUE);
			gpApp->LogUserAction(_T("Show Composebar"));
			gpApp->m_bComposeWndVisible = TRUE; // used in OnUpdateViewComposeBar() handler
			SendSizeEvent(); // needed to force redraw
		}
		m_pComposeBar->GetSizer()->Layout(); // make compose bar resize for different buttons being shown

        // when the compose bar is turned off, whether by the View menu command or the
        // Advanced menu command to turn off free translation mode, we must clear
        // m_bComposeBarWasAskedForFromViewMenu to FALSE; and we don't need to do anything
        // with the fonts when we have closed the window
		if (!m_pComposeBar->IsShown()) //if (!gpApp->m_bComposeWndVisible)
		{
			// the bar has just been made invisible
			gpApp->m_bComposeBarWasAskedForFromViewMenu = FALSE; // needed for free translation mode

			// restore focus to the targetBox, if it is visible (moved here by BEW on 18Oct06)
			if (pApp->m_pTargetBox != NULL)
				if (pApp->m_pTargetBox->IsShown()) // MFC could use BOOL IsWindowVisible() here
					pApp->m_pTargetBox->SetFocus();
		}
		else
		{
            // the bar is visible, so set the font - normally m_pComposeFont will preserve
            // the setting, which by default will be based on m_pTargetFont, but when
            // glossing it could be the navText's font instead
			wxTextCtrl* pEdit = (wxTextCtrl*)FindWindowById(IDC_EDIT_COMPOSE);

			if (gbIsGlossing && gbGlossingUsesNavFont)
			{
				CopyFontBaseProperties(pApp->m_pNavTextFont,pApp->m_pComposeFont);
			}
			else
			{
				CopyFontBaseProperties(pApp->m_pTargetFont,pApp->m_pComposeFont);
			}
			pApp->m_pComposeFont->SetPointSize(12);
			pEdit->SetFont(*pApp->m_pComposeFont);

			if (gpApp->m_bFreeTranslationMode)
			{
				// when commencing free translation mode, show any pre-existing content selected
				// also clear the starting and ending character indices for the box contents
				pEdit->SetSelection(-1,-1);
			}
			else
			{
                // BEW added 18Oct06, since focus doesn't get automatically put into
                // compose bar's edit box when the bar was first opened...
				// when not Free Translation mode, set the focus to the edit box and
				// show all selected
				pEdit->SetSelection(-1,-1); // -1,-1 selects all
			}
			pEdit->SetFocus();
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////
/// \return		nothing
/// \param      event   -> the wxUpdateUIEvent that is generated when the View Menu is about
///                         to be displayed
/// \remarks
/// Called from: The wxUpdateUIEvent mechanism when the associated menu item is selected, and before
/// the menu is displayed.
/// This handler insures that the "Compose Bar" item on the View menu is enabled and checked when
/// the App's m_bComposeWndVisible flag is TRUE, and unchecked when m_bComposeWndVisible is FALSE.
/// The "Compose Bar" menu item will be disabled if the application is in free translation mode.
/// BEW 26Mar10, no changes needed for support of doc version 5
////////////////////////////////////////////////////////////////////////////////////////////
void CMainFrame::OnUpdateViewComposeBar(wxUpdateUIEvent& event)
{
	if (gbVerticalEditInProgress)
	{
		event.Enable(FALSE);
		return;
	}
	// the flags we want are on the view, so get the view
	CAdapt_ItApp* pApp = &wxGetApp();
	wxASSERT(pApp != NULL);
	CAdapt_ItView* pView = (CAdapt_ItView*) pApp->GetView();
	wxASSERT(pView->IsKindOf(CLASSINFO(CAdapt_ItView)));

	if (pApp->m_bFreeTranslationMode)
	{
		// free translation mode is in effect
		pApp->m_bComposeBarWasAskedForFromViewMenu = FALSE;
		event.Enable(FALSE);
	}
	else
	{
		// free translation mode is not in effect currently
		if (pView != NULL) //if (pView == pAppView)
		{
			event.Enable(TRUE);
			event.Check(pApp->m_bComposeWndVisible);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////
/// \return		nothing
/// \param      event   -> the wxUpdateUIEvent that is generated when the View Menu is about
///                         to be displayed
/// \remarks
/// Called from: The wxUpdateUIEvent mechanism when the associated menu item is selected, and before
/// the menu is displayed.
/// This handler insures that the "Mode Bar" item on the View menu is enabled and checked when
/// the App's m_bModeBarVisible flag is TRUE, and unchecked when m_bModeBarVisible is FALSE.
/// The "Mode Bar" menu item will be disabled if the application is in free translation mode.
////////////////////////////////////////////////////////////////////////////////////////////
void CMainFrame::OnUpdateViewModeBar(wxUpdateUIEvent& event)
{
	if (gbVerticalEditInProgress)
	{
		event.Enable(FALSE);
		return;
	}
	// the flags we want are on the view, so get the view
	CAdapt_ItApp* pApp = &wxGetApp();
	wxASSERT(pApp != NULL);
	CAdapt_ItView* pView = (CAdapt_ItView*) pApp->GetView();
	wxASSERT(pView->IsKindOf(CLASSINFO(CAdapt_ItView)));

	if (pView != NULL) //if (pView == pAppView)
	{
		event.Enable(TRUE);
		event.Check(pApp->m_bModeBarVisible);
	}
}

//void CMainFrame::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized) // MFC version
// NOTE: wxFrame::OnActivate() is MS Windows only. It is not a virtual function
// under wxWidgets, and takes a single wxActivateEvent& event parameter.
// BEW 26Mar10, no changes needed for support of doc version 5
void CMainFrame::OnActivate(wxActivateEvent& event)
{
	// NOTE: Setting a breakpoint in this type of function will be problematic
	// because the breakpoint will itself trigger the OnActivate() event!
	CAdapt_ItApp* pApp = &wxGetApp();
	wxASSERT(pApp != NULL);

	// m_pTargetBox is now on the App, and under wxWidgets the main frame's
	// OnActivate() method may be called after the view has been destroyed
	if (event.GetActive())
	{
        // if free translation mode is active, put the focus in the compose bar's
        // wxTextCtrl added 12May06 by request of Jim Henderson so return by ALT+TAB from
        // another window restores the focus to the edit box for free translation typing
		if (pApp->m_bFreeTranslationMode )
		{
			CMainFrame* pFrame = pApp->GetMainFrame();
			wxASSERT(pFrame != NULL);
			wxTextCtrl* pEdit = (wxTextCtrl*)FindWindowById(IDC_EDIT_COMPOSE);
			wxASSERT(pEdit != NULL);
			if (pFrame->m_pComposeBar->IsShown())
			{
				pEdit->SetFocus();
				return;
			}
		}
		// restore focus to the targetBox, if it is visible
		if (pApp->m_pTargetBox != NULL)
			if (pApp->m_pTargetBox->IsShown())
				pApp->m_pTargetBox->SetFocus();
	}
	// The docs for wxActivateEvent say skip should be called somewhere in the handler,
	// otherwise strange behavior may occur.
	event.Skip();
}

// OnIdle moved here from the App. When it was in the App it was causing
// the File | Exit and App x cancel commands to not be responsive there
// BEW 26Mar10, no changes needed for support of doc version 5
void CMainFrame::OnIdle(wxIdleEvent& event)
{
	idleCount++; // unused, may want to use this later

	// from exec sample below
    //size_t count = m_running.GetCount();
    //for ( size_t n = 0; n < count; n++ )
    //{
    //    if ( m_running[n]->HasInput() )
    //    {
    //        event.RequestMore();
    //    }
    //}
	//event.Skip(); // other examples include this (text.cpp)
	// Note: Skip() can be called by an event handler to tell the event system
	// that the event handler should be skipped, and the next valid handler
	// used instead.
	// end of exec sample

	// once system's work is done, my stuff below will be tried
	CAdapt_ItDoc* pDoc;
	CAdapt_ItView* pView;
	CPhraseBox* pBox;

	CAdapt_ItApp* pApp = &wxGetApp();
	wxASSERT(pApp != NULL);

	pApp->GetBasePointers(pDoc,pView,pBox);

    // if a document has just been created or opened in collaboration mode - collaborating
    // with Paratext or Bibledit, and the phrase box has been set up; and because setup of
    // the box potentially could cause a premature opening of the Choose Translation dialog
    // before the setup's OnOK() handler for the dialog has returned, we set this flag
    // there, and here we check for it being TRUE, and at this point OnOK() will have
    // returned and so we can now force the call of PlacePhraseBox() which we suppressed
    // for safety's sake
	if (pApp->bDelay_PlacePhraseBox_Call_Until_Next_OnIdle)
	{
		if (pApp->m_pActivePile != NULL && pApp->m_nActiveSequNum != -1)
		{
			CCell*pCell = pApp->m_pActivePile->GetCell(1);
			int selector = 2; // don't do KB save in first block, but do do KB save in 2nd block
			pView->PlacePhraseBox(pCell,selector);
		}
		// restore the flag's default ('not ON') value
		pApp->bDelay_PlacePhraseBox_Call_Until_Next_OnIdle = FALSE;
	}

    // wx version: Display some scrolling data on the statusbar. m_bShowScrollData is only
    // true when __WXDEBUG__ is defined, so it will not appear in release versions.
	if (m_bShowScrollData && this->m_pStatusBar->IsShown())
	{
		static size_t pgSize, scrPos, scrMax;
		static int pixPU, vStartx, vStarty;
		static wxSize csz, frsz, vsz, cz;
		bool dataChanged = FALSE;

		int vtempx, vtempy;
		canvas->GetViewStart(&vtempx,&vtempy);
		if (vtempx != vStartx || vtempy != vStarty)
		{
			dataChanged = TRUE;
			vStartx = vtempx;
			vStarty = vtempy;
		}
		if (canvas->GetScrollThumb(wxVERTICAL) != (int)pgSize)
		{
			dataChanged = TRUE;
			pgSize = canvas->GetScrollThumb(wxVERTICAL);
		}
		if (canvas->GetScrollPos(wxVERTICAL) != (int)scrPos)
		{
			dataChanged = TRUE;
			scrPos = canvas->GetScrollPos(wxVERTICAL);
		}
		if (canvas->GetScrollRange(wxVERTICAL) != (int)scrMax)
		{
			dataChanged = TRUE;
			scrMax = canvas->GetScrollRange(wxVERTICAL);
		}
		int tpixPU;
		canvas->GetScrollPixelsPerUnit(0,&tpixPU);
		if (tpixPU != pixPU)
		{
			dataChanged = TRUE;
			canvas->GetScrollPixelsPerUnit(0,&pixPU);
		}
		if (canvas->GetSize() != cz)
		{
			dataChanged = TRUE;
			cz = canvas->GetSize();
		}
		if (GetCanvasClientSize() != csz) // use our own calcs for canvas' "client" size
		{
			dataChanged = TRUE;
			csz = GetCanvasClientSize(); // use our own calcs for canvas' "client" size
		}
		if (this->GetClientSize() != frsz)
		{
			dataChanged = TRUE;
			frsz = GetClientSize();
		}
		if (canvas->GetVirtualSize() != vsz)
		{
			dataChanged = TRUE;
			vsz = canvas->GetVirtualSize();
		}

		if (dataChanged)
		{
			SetStatusText(wxString::Format(_T("vStart = %dy, pgsz = %d, scrpos = %d, scrmax = %d,")
				_T("pixpu = %d, clSzcan: %d,%d, clSzfr %d,%d virtSz:%dw%dh"),
										//vStartx,
										(int)vStarty,
										(int)pgSize,
										(int)scrPos,
										(int)scrMax,
										(int)pixPU,
										//cz.x,
										//cz.y,
										(int)csz.x,
										(int)csz.y,
										(int)frsz.x,
										(int)frsz.y,
										(int)vsz.x,
										(int)vsz.y),1);
		}
	}

	if (pApp->m_bSingleStep)
	{
		pApp->m_bAutoInsert = FALSE;
	}
	//if (pDoc != NULL && pApp != NULL && !pApp->m_bNoAutoSave && pApp->m_pKB != NULL)
	// whm added tests for BEW 18Aug10 to prevent calling DoAutoSaveDoc() whenever the
	// StartWorking wizard is active (non-NULL) and whenever there is not a real
	// document open.
	if (pDoc != NULL && pApp != NULL && !pApp->m_bNoAutoSave && pApp->m_pKB != NULL
		&& pStartWorkingWizard == NULL
		&& (gpApp->m_pSourcePhrases != NULL && !gpApp->m_pSourcePhrases->IsEmpty()))
	{
		wxDateTime time = wxDateTime::Now();
		wxTimeSpan span = time - pApp->m_timeSettings.m_tLastDocSave;

		if (pApp->m_bIsDocTimeButton)
		{
			if (span > pApp->m_timeSettings.m_tsDoc)
			{
				// we need to do a save, provided the document is dirty
				if (pDoc->IsModified())
					pApp->DoAutoSaveDoc();
			}
		}
		else
		{
			// we are counting phrase box moves for doing autosaves
			if (nCurrentSequNum > nSequNumForLastAutoSave + pApp->m_nMoves)
			{
				if(pDoc->IsModified())
					pApp->DoAutoSaveDoc();
			}
		}

		// now try the KB
		span = time - pApp->m_timeSettings.m_tLastKBSave;
		if (span > pApp->m_timeSettings.m_tsKB)
		{
			// due for a save of the KB
			pApp->DoAutoSaveKB();
		}
	}

	if (gbReplaceAllIsCurrent && pView != NULL && pApp->m_pReplaceDlg != NULL )
	{
		bool bSucceeded = pApp->m_pReplaceDlg->OnePassReplace();
		if (bSucceeded)
		{
			event.RequestMore(); // added
		}
		else
		{
			// we have come to the end
			pApp->m_pReplaceDlg->Show(TRUE);
			gbReplaceAllIsCurrent = FALSE;
		}
	}

	if (pApp->m_bJustLaunched)
	{
		pApp->m_bJustLaunched = FALSE; // moved up before DoStartupWizardOnLaunch()
		pView->DoStartupWizardOnLaunch();

		// next block is an brute force kludge to get around the fact that if we
		// have used the wizard to get a file on the screen, the phrase box won't
		// have the text selected when user first sees the doc, so this way I can get
		// it selected as if the File... New... route was taken, rather than the wiz.
		if (pApp->m_bStartViaWizard && pApp->m_pTargetBox != NULL)
		{
			pApp->m_pTargetBox->SetFocus();
			pApp->m_nEndChar = -1;
			pApp->m_nStartChar = -1;
			pApp->m_pTargetBox->SetSelection(pApp->m_nStartChar,pApp->m_nEndChar);
			pApp->m_bStartViaWizard = FALSE; // suppress this code from now on
		}

			// force the background colour to update, in case m_bReadOnlyAccess
			// has just become TRUE
			pView->canvas->Refresh();
	}
	event.Skip(); // let std processing take place too

	pApp->m_bNotesExist = FALSE;
	if (pDoc)
	{
		SPList* pList = pApp->m_pSourcePhrases;
		if (pList->GetCount() > 0)
		{
			SPList::Node* pos = pList->GetFirst();
			while (pos != NULL)
			{
				CSourcePhrase* pSrcPhrase = (CSourcePhrase*)pos->GetData();
				pos = pos->GetNext();
				if (pSrcPhrase->m_bHasNote)
				{
					// set the flag on the app
					pApp->m_bNotesExist = TRUE;
					break; // don't need to check further
				}
			}
		}
	}

	if (gbUpdateDocTitleNeeded)
	{
		wxString fname = pApp->m_curOutputFilename;
		wxString extensionlessName;
		pDoc->SetDocumentWindowTitle(fname, extensionlessName);
		gbUpdateDocTitleNeeded = FALSE; // turn it off until the next MRU doc open failure
	}

	if (gbCameToEnd)
	{
		gbCameToEnd = FALSE; // whm moved this above wxMessageBox because Linux version was repeatedly calling wxMessageBox causing crash
		// IDS_AT_END
		wxMessageBox(
		_("The end. Provided you have not missed anything earlier, there is nothing more to adapt in this file."),
		_T(""), wxICON_INFORMATION);
	}

	if (bUserCancelled)
		bUserCancelled = FALSE; // ensure its turned back off

	if (pApp->m_bAutoInsert)
	{
		if (pApp->m_nCurDelay > 0)
		{
			DoDelay(); // defined in Helpers.cpp (m_nCurDelay is in tick units)
		}
		bool bSuccessfulInsertAndMove =  pBox->OnePass(pView); // whm note: This is 
											// the only place OnePass() is called
												
		// whm added 20Nov10 reset the m_bIsGuess flag below. Can't do it in PlaceBox()
		// because PlaceBox() is called in OnePass via the MoveToNextPile() call near the beginning
		// of OnePass, then again near the end of OnePass - twice in the normal course of
		// auto-insert adaptations.
		pApp->m_bIsGuess = FALSE;
		if (bSuccessfulInsertAndMove)
		{
			//return TRUE; // enable next iteration
			event.RequestMore(); // added
		}
		else
		{
			// halt iterations, we did not make a match or could not move forward,
			// but continue to enable OnIdle calls
			pApp->m_bAutoInsert = FALSE;
		}
	}
}

// whm Note: this and following custom event handlers are in the View in the MFC version
//
// The following is the handler for a wxEVT_Adaptations_Edit event message, defined in the
// event table macro EVT_ADAPTATIONS_EDIT.
// The wxEVT_Adaptations_Edit event is sent to the window event loop by a
// wxPostEvent() call in OnEditSourceText().
// BEW 26Mar10, no changes needed for support of doc version 5
// BEW 9July10, no changes needed for support of kbVersion 2

void CMainFrame::OnCustomEventAdaptationsEdit(wxCommandEvent& WXUNUSED(event))
{
	// adaptations updating is required
	// Ensure that we have a valid pointer to the m_pVertEditBar member
	// of the frame window class
	wxASSERT(m_pVertEditBar != NULL);
	wxASSERT(m_pRemovalsBar != NULL);

	CAdapt_ItApp* pApp = &wxGetApp();
	wxASSERT(pApp != NULL);
	CAdapt_ItView* pView = pApp->GetView();
	wxASSERT(pView != NULL);
	CFreeTrans* pFreeTrans = gpApp->GetFreeTrans();
	wxASSERT(pFreeTrans != NULL);
	if (m_pVertEditBar == NULL)
	{
		wxMessageBox(_T("Failure to obtain pointer to the vertical edit control bar in \
						 OnCustomEventAdaptationsEdit()"),_T(""), wxICON_EXCLAMATION);
		return;
	}

	// determine what setup is required: control is coming from either sourceTextStep or
	// glossesStep, when the order is adaptions before glosses (see gbAdaptBeforeGloss flag),
	// but if the order is glosses before adaptations, then control is coming from either
	// glossesStep or freeTranslationsStep. These variations require separate initialization
	// blocks.
	EditRecord* pRec = &gEditRecord;
	SPList* pSrcPhrases = pApp->m_pSourcePhrases;
	bool bAllsWell = TRUE;

	if (gbVerticalEditInProgress)
	{
		if (gEntryPoint == sourceTextEntryPoint)
		{
			wxCommandEvent evt; // whm: a dummy event to pass to the handlers we're calling below
			if (gbAdaptBeforeGloss)
			{
				// user's chosen processing order is "do adaptations before glosses"
				if (gEditStep == sourceTextStep)
				{
					// the PostMessage() which we are handling is fired from the end of the
					// source text edit step, so control is still in that step, and about to
					// transition to the adaptations step if the editable span had some, if not,
					// we transition from adaptations step, after minimal setup, to the
					// glosses step


					// the mode in effect when the Edit Source Text dialog was visible may not
					// have been adaptations mode, it may have been glossing mode, so we need to
					// check and, if necessary, switch to adaptations mode; moreover, if there were
					// glosses in the active section, it would be helpful to make them visible in
					// the surrounding context (more info might help the user), but if there were
					// no glosses shown, don't unless the original mode had "See Glosses" turned ON
					if (gbIsGlossing)
					{
						// glossing mode is currently ON (and adaptations are always visible
						// in that mode too); so switch to adapting mode, but leave glosses
						// visible; but if there were none in the edit span, we'll make them
						// invisible
						pView->ToggleGlossingMode();
						if (!pRec->bEditSpanHasGlosses)
						{
							// the edit span lacked glosses, so assume they are not relevant or not
							// present in the surrounding context
							pView->ToggleSeeGlossesMode();
						}
					}
					else
					{
						// the view is in adapting mode; we want to make glosses visible though
						// if the edit span has some, but not if it has none
						if (pRec->bEditSpanHasGlosses)
						{
							// make them visible if not already so
							if (!gbGlossingVisible)
								pView->ToggleSeeGlossesMode();
						}
						else
						{
							// make sure glosses are not visible
							if (gbGlossingVisible)
							{
								// make them invisible, as there are none in the edit span
								pView->ToggleSeeGlossesMode();
							}
						}
					}

					// if the user removed all the CSourcePhrase instances of the editable span,
					// there are no adaptations to be updated, so check for this and if so, fill
					// in the relevant gEditRecord members and then post the event for the next
					// step -- we don't give the user a dialog to vary the step order here, because
					// he won't have seen any feedback up to now that he's about to be in adaptations
					// step, so we unilaterally send him on to the next step
					// Note: if either or both of the ...SpanCount variables are 0, it means the user
					// deleted the material in the edit span; if they are -1, it means the span was not
					// deleted but had no adaptations in it; in either case nothing gets done in this step
					// except the switch to adapting mode
					if (pRec->nNewSpanCount == 0)
					{
						// user deleted everything... so move on
						pRec->nAdaptationStep_OldSpanCount = 0;
						pRec->nAdaptationStep_NewSpanCount = 0;
						pRec->nAdaptationStep_StartingSequNum = -1;
						pRec->nAdaptationStep_EndingSequNum = -1;

						gEditStep = adaptationsStep;
						pRec->bAdaptationStepEntered = TRUE; // prevent reinitialization if user returns here

						// post event for progressing to gloss mode (we let its handler make the
						// same test for nNewSpanCount being 0 and likewise cause progression
						// immediately rather than make that next progression happen from here)
						// Note, we have not changed from adaptationsStep, hence the handler for
						// this message will become active with gEditStep still set to
						// adaptationsStep - this is how we inform the next step what the previous
						// step was, and so the handler for the next step will finish off this
						// current step before setting up for the commencement of the glossesStep
						//this->PostMessage(CUSTOM_EVENT_GLOSSES_EDIT,0,0);
						wxCommandEvent eventCustom(wxEVT_Glosses_Edit);
						wxPostEvent(this, eventCustom);
						return;
					}

					if (!pRec->bAdaptationStepEntered)
					{
						// put the values in the edit record, where the CCell drawing function can get them
						bool bAllWasOK;
						pRec->nAdaptationStep_StartingSequNum = pRec->nStartingSequNum;
						pRec->nAdaptationStep_EndingSequNum = pRec->nStartingSequNum + pRec->nNewSpanCount - 1;
						pRec->nAdaptationStep_OldSpanCount = pRec->nNewSpanCount;

						// initialize the new count to the old one; it will be modifed as necessary by the
						// user's subsequent editing actions
						pRec->nAdaptationStep_NewSpanCount = pRec->nAdaptationStep_OldSpanCount;
						pRec->nAdaptationStep_ExtrasFromUserEdits = 0; // at step end can be -ve, 0 or +ve

						// deep copy the initial editable span, before user has a chance to do
						// mergers, retranslations, placeholder insertions, etc.  This deep copy
						// must only be done once
						bAllWasOK = pView->DeepCopySourcePhraseSublist(pSrcPhrases, pRec->nAdaptationStep_StartingSequNum,
							pRec->nAdaptationStep_EndingSequNum, &pRec->adaptationStep_SrcPhraseList);
						if (!bAllWasOK)
						{
							// if this failed, we must bail out of this vertical edit process
							gEditStep = adaptationsStep;
							pRec->bAdaptationStepEntered = TRUE;
							goto cancel;
						}
					}

					// ensure gEditStep remains set to adaptationsStep here, before returning
					gEditStep = adaptationsStep;

					// record the fact that this step has been entered and initial values set up
					// (it is done once only, see above)
					pRec->bAdaptationStepEntered = TRUE;

					// (we need both the above 2 flags, the gEditStep is TRUE each time the user comes
					// back to this step within the one vertical edit, but bAdaptationsStepEntered is
					// not set until after step members are initialized in gEditRecord, and thereafter
					// is used to prevent reinitialization (and value corruption) if the user returns
					// to the adaptations step more than once in the one vertical edit process

					if (pRec->bEditSpanHasAdaptations)
					{
						// populate the combobox with the required removals data for adaptationsStep
						bAllsWell = pView->PopulateRemovalsComboBox(adaptationsStep, &gEditRecord);
						bAllsWell = bAllsWell; // avoid warning (keep processing, even if the combobox
											   // was not populated)
						// put the adaptations step's message in the multi-line read-only edit box
						// IDS_VERT_EDIT_ADAPTATIONS_MSG
						pView->SetVerticalEditModeMessage(
_("Vertical Editing - adaptations step: Type the needed adaptations in the editable region. Earlier adaptations are stored at the top of the Removed list. Gray text is not accessible. Adapting mode is currently on and all adaptation functionalities are enabled, including mergers, placeholder insertion and retranslations."));

						// setup the toolbar with the vertical edit process control buttons and user
						// message, and make it show-able
						// whm added: the MFC version of vertical editing had the RemovalsBar always
						// showing. I'm keeping it hidden until it really is needed.
						// update static text in removals bar to indicate that clicking on item in
						// list copies to the phrase box
						wxStaticText* pStatic = (wxStaticText*)m_pRemovalsBar->FindWindowById(ID_STATIC_TEXT_REMOVALS);
						wxASSERT(pStatic != NULL);
						pStatic->SetLabel(_("Clicking on an item in the above list copies it to the Phrase Box, overwriting anything there."));
						pRemovalsBarSizer->Layout(); //m_pRemovalsBar->Refresh();
						if (!m_pRemovalsBar->IsShown())
						{
							m_pRemovalsBar->Show(TRUE);
						}
						m_pVertEditBar->Show(TRUE);

						// initiate a redraw of the frame and the client area (Note, this is MFC's
						// CFrameWnd member function called RecalcLayout(), not my CAdapt_ItView
						// member function of same name
						//pFWnd->RecalcLayout(); // the bNotify parameter is default TRUE, let it do so,
						//					   // no harm if the window is not embedded
						// whm Note: Client area is changing size so send a size event to get the layout to change
						// since the doc/view framework won't do it for us.
						SendSizeEvent(); // forces the CMainFrame::SetSize() handler to run and do the needed redraw

						// place the phrase box at the start of the span, and update the layout etc
						int activeSequNum = pRec->nAdaptationStep_StartingSequNum;
						pView->PutPhraseBoxAtSequNumAndLayout(pRec,activeSequNum);

					}  // end block for result TRUE for test (pRec->bEditSpanHasAdaptations)
					else
					{
						// no adaptations formerly, so we don't do the above extra steps because the
						// user would not get to see their effect before they get clobbered by the setup
						// of the glossesStep transition
						wxCommandEvent eventCustom(wxEVT_Glosses_Edit);
						wxPostEvent(this, eventCustom);
						return;
					}
				}
/*rollback*/	else if (gEditStep == glossesStep)
				{
					// the user was in glossing step, and requested a roll back to adaptations step;
					// we allow this whether or not there were originally adaptations in the editable span
					// because we assume a rollback is intentional to get adaptations made or altered

                    // we know we are coming from glossing mode being currently on, so we
                    // have to switch to adapting mode; moreover, we expect glosses were
                    // made or updated, and the user will want to see them, so we make them
                    // visible (more info might help the user)
					pView->ToggleGlossingMode();

                    // If the user removed all the CSourcePhrase instances of the editable
                    // span, there are no adaptations to be updated, because the span has
                    // no extent. We will try to externally catch and prevent a user
                    // rollback to adaptations mode when there is no extent, but just in
                    // case we don't, just bring the vertical edit to an end immediately --
                    // this block should never be entered if I code correctly
					if (pRec->nNewSpanCount == 0)
					{
						wxCommandEvent eventCustom(wxEVT_End_Vertical_Edit);
						wxPostEvent(this, eventCustom);
                        // whm Note: This event calls OnCustomEventCancelVerticalEdit()
                        // which ends by hiding any of the vertical edit tool bars from the
                        // main window.
						gEditStep = adaptationsStep;
						pRec->bAdaptationStepEntered = TRUE;
						return;
					}

                    // the span was entered earlier, and we are rolling back, so we have to
                    // set up the state as it was at the start of the of the previous
                    // adaptationsStep; and we have to restore the phrase box to the start
                    // of the editable span too (we don't bother to make any reference
                    // count decrements in the KB, so this would be a source of ref counts
                    // being larger than strictly correct, but that is a totally benign
                    // effect)

					// the user may have shortened or lengthened the span, so we just
					// use the nAdaptationStp_NewSpanCount value as that is the value on
					// last exit from the adaptationsStep -- this is how many we must remove
					// in order to insert the original adaptationsStep's deep copies
					int nHowMany = pRec->nAdaptationStep_NewSpanCount;
					wxASSERT(nHowMany != 0);
					wxASSERT(pRec->adaptationStep_SrcPhraseList.GetCount() > 0);
					bool bWasOK;
					bWasOK = pView->ReplaceCSourcePhrasesInSpan(pSrcPhrases,
						pRec->nAdaptationStep_StartingSequNum,
						nHowMany, // defines how many to remove to make the gap for the insertions
						&pRec->adaptationStep_SrcPhraseList,
						0, // start at index 0, ie. insert whole of deep copied list
						pRec->nAdaptationStep_OldSpanCount);
					if (!bWasOK)
					{
                        // tell the user there was a problem, but keep processing even if
                        // FALSE was returned (English message will suffice, the error is
                        // unlikely
						wxMessageBox(_T("OnCustomEvenAdaptationsEdit() called ReplaceCSourcePhrasesInSpan() and the latter returned FALSE, at line 3842 in MainFrm.cpp; processing will continue however"));
						gpApp->LogUserAction(_T("OnCustomEvenAdaptationsEdit() called ReplaceCSourcePhrasesInSpan() and the latter returned FALSE, at line 3842 in MainFrm.cpp; processing continues"));
					}
					pView->UpdateSequNumbers(0); // make sure all are in proper sequence in the doc

					// restore original counts to pre-extras values
					pRec->nAdaptationStep_EndingSequNum -= pRec->nAdaptationStep_ExtrasFromUserEdits;
					pRec->nAdaptationStep_ExtrasFromUserEdits = 0; // reinitialize, as the user may
																   // now edit differently than before

					// re-initialize the new spancount parameter which has become incorrect
					pRec->nAdaptationStep_NewSpanCount = pRec->nAdaptationStep_OldSpanCount;

					// delete the instances in the glossesStep's list
					gpApp->GetDocument()->DeleteSourcePhrases(&pRec->glossStep_SrcPhraseList);

					// restore the Phrase Box to the start of the editable span
					// place the phrase box at the start of the span, and update the layout etc
					int activeSequNum = pRec->nAdaptationStep_StartingSequNum;
					pView->PutPhraseBoxAtSequNumAndLayout(pRec,activeSequNum);

					// populate the combobox with the required removals data for adaptationsStep
					bAllsWell = pView->PopulateRemovalsComboBox(adaptationsStep, &gEditRecord);
					bAllsWell = bAllsWell; // avoid warning (keep processing, even if the combobox
										   // was not populated)
					// put the adaptations step's message in the multi-line read-only CEdit box
					pView->SetVerticalEditModeMessage(
_("Vertical Editing - adaptations step: Type the needed adaptations in the editable region. Earlier adaptations are stored at the top of the Removed list. Gray text is not accessible. Adapting mode is currently on and all adaptation functionalities are enabled, including mergers, placeholder insertion and retranslations."));

					// setup the toolbar with the vertical edit process control buttons and user
					// message, and make it show-able
					// update static text in removals bar to indicate that clicking on item in
					// list copies to the phrase box
					wxStaticText* pStatic = (wxStaticText*)m_pRemovalsBar->FindWindowById(ID_STATIC_TEXT_REMOVALS);
					wxASSERT(pStatic != NULL);
					pStatic->SetLabel(_("Clicking on an item in the above list copies it to the Phrase Box, overwriting anything there."));
					pRemovalsBarSizer->Layout(); //m_pRemovalsBar->Refresh();
					if (!m_pRemovalsBar->IsShown())
					{
						m_pRemovalsBar->Show(TRUE);
					}
					m_pVertEditBar->Show(TRUE);

					// ensure gEditStep remains set to adaptationsStep here, before returning
					gEditStep = adaptationsStep;

					// record the fact that this step has been entered and initial values set up
					// (it is done once only, see above)
					pRec->bAdaptationStepEntered = TRUE;

					// initiate a redraw of the frame and the client area
					SendSizeEvent(); // forces the CMainFrame::SetSize() handler to run and
									 // do the needed redraw

					// place the phrase box at the start of the span, and update the layout etc
					int activeSequNum_ = pRec->nAdaptationStep_StartingSequNum;
					pView->PutPhraseBoxAtSequNumAndLayout(pRec,activeSequNum_);

				} // end block for result TRUE for test (gEditStep == glossesStep)
				else
				{
					// ought not to happen because we should only be able to get to adaptationsStep
					// by either normal progression from sourceTextStep or rollback from glossesStep,
					// so, cancel out
					gEditStep = adaptationsStep;
					pRec->bAdaptationStepEntered = TRUE;
					goto cancel;
				}
			} // end block for result TRUE for test (gbAdaptBeforeGloss)
/****/		else
			{
				// user's chosen order of steps is "glosses before adaptations", so we are coming
				// to this block with glossesStep still in effect & setting up for adaptationsStep
				if (gEditStep == glossesStep)
				{
                    // the wxPostEvent() which we are handling is fired from the end of the
                    // glosses step, so control is still in that step, and about to
                    // transition to the adaptations step if the editable span had some, if
                    // not, we transition from adaptations step, after minimal setup, to
                    // the free translations step

					// since we were in glossingStep we know that the glossing checkbox is ON
					// and the gbGlossingVisible global is TRUE; so all we need to do is turn
					// the checkbox off
					pView->ToggleGlossingMode();

                    // if the user removed all the CSourcePhrase instances of the editable
                    // span, there are no adaptations to be updated, so check for this and
                    // if so, fill in the relevant gEditRecord members and then post the
                    // event for the next step -- we don't give the user a dialog to vary
                    // the step order here, because he won't have seen any feedback up to
                    // now that he's about to be in adaptations step, so we unilaterally
                    // send him on to the next step
                    // Note: if either or both of the ...SpanCount variables are 0, it
                    // means the user deleted the material in the edit span, and populating
                    // the list is impossible too
					if (pRec->nNewSpanCount == 0)
					{
						// user deleted everything... so move on
						pRec->nAdaptationStep_OldSpanCount = 0;
						pRec->nAdaptationStep_NewSpanCount = 0;
						pRec->nAdaptationStep_StartingSequNum = -1;
						pRec->nAdaptationStep_EndingSequNum = -1;

						gEditStep = adaptationsStep; // we've been in this step, even though we did nothing
						pRec->bAdaptationStepEntered = TRUE; // prevent reinitialization if user returns here

                        // post event for progressing to gloss mode (we let its handler
                        // make the same test for nNewSpanCount being 0 and likewise cause
                        // progression immediately rather than make that next progression
                        // happen from here) Note, we have not changed from
                        // adaptationsStep, hence the handler for this message will become
                        // active with gEditStep still set to adaptationsStep - this is how
                        // we inform the next step what the previous step was, and so the
                        // handler for the next step will finish off this current step
                        // before setting up for the commencement of the glossesStep
						wxCommandEvent eventCustom(wxEVT_Free_Translations_Edit);
						wxPostEvent(this, eventCustom);
						return;
					}

					// if this is our first time in this step...
					if (!pRec->bAdaptationStepEntered)
					{
                        // put the values in the edit record, where the CCell drawing
                        // function can get them, we get them from the glossesStep
                        // parameters, since we have just come from there
						bool bAllWasOK;
						pRec->nAdaptationStep_StartingSequNum = pRec->nGlossStep_StartingSequNum;
						pRec->nAdaptationStep_EndingSequNum = pRec->nGlossStep_EndingSequNum;
						pRec->nAdaptationStep_OldSpanCount = pRec->nGlossStep_SpanCount;

						// initialize the new count to the old one; it will be modifed as necessary by the
						// user's subsequent editing actions in adaptationsStep
						pRec->nAdaptationStep_NewSpanCount = pRec->nAdaptationStep_OldSpanCount;
						pRec->nAdaptationStep_ExtrasFromUserEdits = 0; // at step end, it can be -ve, 0 or +ve

						// deep copy the initial editable span, before user has a chance to do
						// mergers, retranslations, placeholder insertions, etc.  This deep copy
						// must only be done once
						bAllWasOK = pView->DeepCopySourcePhraseSublist(pSrcPhrases, pRec->nAdaptationStep_StartingSequNum,
							pRec->nAdaptationStep_EndingSequNum, &pRec->adaptationStep_SrcPhraseList);
						if (!bAllWasOK)
						{
							// if this failed, we must bail out of this vertical edit process
							gEditStep = adaptationsStep;
							pRec->bAdaptationStepEntered = TRUE;
							goto cancel;
						}
					}

					// ensure gEditStep remains set to adaptationsStep here, before returning
					gEditStep = adaptationsStep;

                    // record the fact that this step has been entered and initial values
                    // set up (it is done once only, see above)
					pRec->bAdaptationStepEntered = TRUE;

                    // (we need both the above 2 flags, the gEditStep is TRUE each time the
                    // user comes back to this step within the one vertical edit, but
                    // bAdaptationsStepEntered is not set until after step members are
                    // initialized in gEditRecord, and thereafter is used to prevent
                    // reinitialization (and value corruption) if the user returns to the
                    // adaptations step more than once in the one vertical edit process

					if (pRec->bEditSpanHasAdaptations)
					{
						// populate the combobox with the required removals data for adaptationsStep
						bAllsWell = pView->PopulateRemovalsComboBox(adaptationsStep, &gEditRecord);
						bAllsWell = bAllsWell; // avoid warning (keep processing, even if the combobox
											   // was not populated)
						// put the adaptations step's message in the multi-line read-only CEdit box
						pView->SetVerticalEditModeMessage(
_("Vertical Editing - adaptations step: Type the needed adaptations in the editable region. Earlier adaptations are stored at the top of the Removed list. Gray text is not accessible. Adapting mode is currently on and all adaptation functionalities are enabled, including mergers, placeholder insertion and retranslations."));

						// setup the toolbar with the vertical edit process control buttons and user
						// message, and make it show-able
						// update static text in removals bar to indicate that clicking on item in
						// list copies to the phrase box
						wxStaticText* pStatic = (wxStaticText*)m_pRemovalsBar->FindWindowById(ID_STATIC_TEXT_REMOVALS);
						wxASSERT(pStatic != NULL);
						pStatic->SetLabel(_("Clicking on an item in the above list copies it to the Phrase Box, overwriting anything there."));
						pRemovalsBarSizer->Layout();
						if (!m_pRemovalsBar->IsShown())
						{
							m_pRemovalsBar->Show(TRUE);
						}
						m_pVertEditBar->Show(TRUE);

						// initiate a redraw of the frame and the client area
						SendSizeEvent(); // forces the CMainFrame::SetSize() handler to run and
										 // do the needed redraw

                        // BEW 13Jan09 added next 3 lines to fix bug reported by Roland
                        // Fumey 6Jan09 12:22pm, in which phrase box was left at end of the
                        // editable span, not returned to its start. place the phrase box
                        // at the start of the span, and update the layout etc
						int activeSequNum = pRec->nAdaptationStep_StartingSequNum;
						pView->PutPhraseBoxAtSequNumAndLayout(pRec,activeSequNum);
					}  // end block for result TRUE for test (pRec->bEditSpanHasAdaptations)
					else
					{
                        // no adaptations formerly, so we don't do the above extra steps
                        // because the user would not get to see their effect before they
                        // get clobbered by the setup of the freeTranslationsStep
                        // transition
						wxCommandEvent eventCustom(wxEVT_Free_Translations_Edit);
						wxPostEvent(this, eventCustom);
						return;
					}
				} // end of block for test (gEditStep == glossesStep)
/*rollback*/	else if (gEditStep == freeTranslationsStep)
				{
                    // the user was in free translations step, and requested a roll back to
                    // adaptations step; we allow this whether or not there were originally
                    // adaptations in the editable span because we assume a rollback is
                    // intentional to get adaptations made or altered

                    // Rollback has to set up the free translation span to be what it was
                    // before freeTranslationsStep was first entered -- which entails that
                    // the deep copied CSourcePhrase instances either side of the editable
                    // span be restored (these, after modifications were done to the
                    // modifications list, had the m_bHasFreeTrans, m_bStartFreeTrans,
                    // m_bEndFreeTrans flags all cleared. Failure to do this restoration
                    // will cause SetupCurrentFreeTranslationSection() to fail if
                    // freeTranslationsStep is entered a second time in this current
                    // vertical edit session. Similarly, rolling back from
                    // freeTransationsStep will possibly leave text in the edit box of the
                    // compose bar - it has to be cleared, etc.
					CMainFrame *pFrame = gpApp->GetMainFrame();
					wxASSERT(pFrame != NULL);
                    // whm: In wx the composeBar is created when the app runs and is always
                    // there but may be hidden.
					if (pFrame->m_pComposeBar != NULL)
					{
						wxTextCtrl* pEdit = (wxTextCtrl*)pFrame->m_pComposeBar->FindWindowById(IDC_EDIT_COMPOSE);
						wxASSERT(pEdit != NULL);
						if (pEdit != 0)
						{
							pEdit->ChangeValue(_T("")); // clear the box
						}
					}
					// now restore the free translation span to what it was at last entry
					// to freeTranslationsStep
					if (pRec->bFreeTranslationStepEntered && pRec->nFreeTranslationStep_SpanCount > 0 &&
						pRec->nFreeTrans_EndingSequNum != -1)
					{
                        // this gets the context either side of the user's selection
                        // correct, a further replacement below will replace the editable
                        // span with the instances at at the start of the earlier glossing
                        // step (the user may not have seen it in the GUI, but the list was
                        // populated nevertheless)
						int nHowMany = pRec->nFreeTranslationStep_SpanCount;
						wxASSERT(nHowMany != 0);
						wxASSERT(pRec->freeTranslationStep_SrcPhraseList.GetCount() > 0);
						bool bWasOK;
						bWasOK = pView->ReplaceCSourcePhrasesInSpan(pSrcPhrases,
							pRec->nFreeTranslationStep_StartingSequNum,
							nHowMany, // defines how many to remove to make the gap for the insertions
							&pRec->freeTranslationStep_SrcPhraseList,
							0, // start at index 0, ie. insert whole of deep copied list
							pRec->nFreeTranslationStep_SpanCount);
						if (!bWasOK)
						{
							// tell the user there was a problem, but keep processing even if
							// FALSE was returned (English message will suffice, the error is
							// unlikely
							wxMessageBox(_T("OnCustomEvenAdaptationsEdit() called ReplaceCSourcePhrasesInSpan() and the latter returned FALSE, at line 4109 in MainFrm.cpp; processing will continue however"));
							gpApp->LogUserAction(_T("OnCustomEvenAdaptationsEdit() called ReplaceCSourcePhrasesInSpan() and the latter returned FALSE, at line 4109 in MainFrm.cpp; processing continues"));
						}
						pView->UpdateSequNumbers(0); // make sure all are in proper sequence in the doc

                        // now make the free translation part of the gEditRecord think that
                        // freeTranslationsStep has not yet been entered
						pRec->bFreeTranslationStepEntered = FALSE;
						pView->GetDocument()->DeleteSourcePhrases(&pRec->freeTranslationStep_SrcPhraseList);
						pRec->nFreeTranslationStep_EndingSequNum = -1;
						pRec->nFreeTranslationStep_SpanCount = -1;
						pRec->nFreeTranslationStep_StartingSequNum = -1;
						// don't alter bVerseBasedSection because it is set only once
						// per vertical edit
					}
                    // we know we are coming from free translations mode being currently
                    // on, so we have to switch to adapting mode; moreover, we expect
                    // glosses were made or updated, and the user will want to see them, so
                    // we make them visible (more info might help the user). We also test
                    // to ensure adaptations mode is currently on, if not, we switch to it
                    // as well - we must do this first before calling the
                    // OnAdvancedFreeTranslationsMode() handler (it's a toggle) because
                    // when closing free translation mode it tests the gbIsGlossing flag in
                    // order to know whether to restore removed glosses or removed
                    // adaptations to the Removed combobox list
					if (gbIsGlossing)
					{
						// user must have manually switched to glossing mode,
						// so turn it back off
						pView->ToggleGlossingMode();
					}
					// toggle free translation mode off
					pFreeTrans->ToggleFreeTranslationMode();
                    // If the user removed all the CSourcePhrase instances of the editable
                    // span, there are no adaptations to be updated, because the span has
                    // no extent. We will try to externally catch and prevent a user
                    // rollback to adaptations mode when there is no extent, but just in
                    // case we don't, just bring the vertical edit to an end immediately --
                    // this block should never be entered if I code correctly
					if (pRec->nAdaptationStep_OldSpanCount == 0)
					{
						wxCommandEvent eventCustom(wxEVT_End_Vertical_Edit);
						wxPostEvent(this, eventCustom);
                        // whm Note: This event calls OnCustomEventCancelVerticalEdit()
                        // which ends by hiding any of the vertical edit tool bars from the
                        // main window.
						return;
					}

                    // the span was entered earlier, and we are rolling back, so we have to
                    // set up the state as it was at the start of the of the previous
                    // adaptationsStep; and we have to restore the phrase box to the start
                    // of the editable span too (we don't bother to make any reference
                    // count decrements in the KB, so this would be a source of ref counts
                    // being larger than strictly correct, but that is a totally benign
                    // effect)

					// the user may have shortened or lengthened the span, so we just
					// use the nAdaptationStp_NewSpanCount value as that is the value on
					// last exit from the adaptationsStep -- this is how many we must remove
					// in order to insert the original adaptationsStep's deep copies
					int nHowMany = pRec->nAdaptationStep_NewSpanCount;
					wxASSERT(nHowMany != 0);
					wxASSERT(pRec->adaptationStep_SrcPhraseList.GetCount() > 0);
					bool bWasOK;
					bWasOK = pView->ReplaceCSourcePhrasesInSpan(pSrcPhrases,
						pRec->nAdaptationStep_StartingSequNum,
						nHowMany, // defines how many to remove to make the gap for the insertions
						&pRec->adaptationStep_SrcPhraseList,
						0, // start at index 0, ie. insert whole of deep copied list
						pRec->nAdaptationStep_OldSpanCount);

					if (!bWasOK)
					{
						// tell the user there was a problem, but keep processing even if
						// FALSE was returned (English message will suffice, the error is
						// unlikely
						wxMessageBox(_T("OnCustomEvenAdaptationsEdit() called ReplaceCSourcePhrasesInSpan() and the latter returned FALSE, at line 4185 in MainFrm.cpp; processing will continue however"));
						gpApp->LogUserAction(_T("OnCustomEvenAdaptationsEdit() called ReplaceCSourcePhrasesInSpan() and the latter returned FALSE, at line 4185 in MainFrm.cpp; processing continues"));
					}
					pView->UpdateSequNumbers(0); // make sure all are in proper sequence in the doc

					// restore original counts to pre-user-edited values
					pRec->nAdaptationStep_EndingSequNum -= pRec->nAdaptationStep_ExtrasFromUserEdits;
					pRec->nAdaptationStep_ExtrasFromUserEdits = 0; // reinitialize, as the user may
																   // now edit differently than before

					// re-initialize the new spancount parameter which has become incorrect
					pRec->nAdaptationStep_NewSpanCount = pRec->nAdaptationStep_OldSpanCount;

					// delete the instances in the freeTranslationsStep's list
					pApp->GetDocument()->DeleteSourcePhrases(&pRec->freeTranslationStep_SrcPhraseList);

					// restore the Phrase Box to the start of the editable span
					// place the phrase box at the start of the span, and update the layout etc
					int activeSequNum = pRec->nAdaptationStep_StartingSequNum;
					pView->PutPhraseBoxAtSequNumAndLayout(pRec,activeSequNum);

					// populate the combobox with the required removals data for adaptationsStep
					// (this has been done already, maybe twice in fact, so once more won't hurt)
					bAllsWell = pView->PopulateRemovalsComboBox(adaptationsStep, &gEditRecord);
					bAllsWell = bAllsWell; // avoid warning (continue, even if the combobox was not populated)
					// put the adaptations step's message in the multi-line read-only CEdit box
					pView->SetVerticalEditModeMessage(
_("Vertical Editing - adaptations step: Type the needed adaptations in the editable region. Earlier adaptations are stored at the top of the Removed list. Gray text is not accessible. Adapting mode is currently on and all adaptation functionalities are enabled, including mergers, placeholder insertion and retranslations."));

					// setup the toolbar with the vertical edit process control buttons and user
					// message, and make it show-able
					// update static text in removals bar to indicate that clicking on item in
					// list copies to the phrase box
					wxStaticText* pStatic = (wxStaticText*)m_pRemovalsBar->FindWindowById(ID_STATIC_TEXT_REMOVALS);
					wxASSERT(pStatic != NULL);
					pStatic->SetLabel(_("Clicking on an item in the above list copies it to the Phrase Box, overwriting anything there."));
					pRemovalsBarSizer->Layout();
					if (!m_pRemovalsBar->IsShown())
					{
						m_pRemovalsBar->Show(TRUE);
					}
					m_pVertEditBar->Show(TRUE);

					// ensure gEditStep remains set to adaptationsStep here,
					// before returning
					gEditStep = adaptationsStep;

                    // record the fact that this step has been entered and initial values
                    // set up (it is done once only, see above)
					pRec->bAdaptationStepEntered = TRUE;

					// initiate a redraw of the frame and the client area
					SendSizeEvent(); // forces the CMainFrame::SetSize() handler to run and
									 // do the needed redraw

					// place the phrase box at the start of the span, and update the layout etc
					int activeSequNum_ = pRec->nAdaptationStep_StartingSequNum;
					pView->PutPhraseBoxAtSequNumAndLayout(pRec,activeSequNum_);

				} // end block for result TRUE for test (gEditStep == freeTranslationsStep)
				else
				{
                    // ought not to happen because we should only be able to get to
                    // adaptationsStep by either normal progression from glossesStep or
                    // rollback from freeTranslationsStep, so, cancel out
					gEditStep = adaptationsStep;
					pRec->bAdaptationStepEntered = TRUE;
					goto cancel;
				}
			} // end block for result FALSE for text (gbAdaptBeforeGloss)

		 } // end block for result TRUE for test (gEntryPoint == sourceTextEntryPoint)
		else if (gEntryPoint == adaptationsEntryPoint)
		{
			// user has just edited an existing adaptation... we'll support this
			// option only in the wxWidgets codebase
			; // **** TODO in wxWidgets, perhaps some day ****
		}
		else if (gEntryPoint == glossesEntryPoint)
		{
			// user has just edited an existing gloss... we'll support this
			// option only in the wxWidgets codebase
			; // **** TODO in wxWidgets, perhaps some day ****
		}
		else
		{
            // if none of these, we've got an unexpected state which should never happen,
            // so cancel out
cancel:		;

			// whm: In the wx version we don't keep the RemovalsBar visible, so since the
			wxCommandEvent eventCustom(wxEVT_Cancel_Vertical_Edit);
			wxPostEvent(this, eventCustom);
            // whm Note: This event calls OnCustomEventCancelVerticalEdit() which ends by
            // hiding any of the vertical edit tool bars from the main window.
			return;
		}
	}
}

// The following is the handler for a custom wxEVT_Glosses_Edit event message, sent
// to the window event loop by a wxPostEvent() call
// BEW 26Mar10, no changes needed for support of doc version 5
// BEW 9July10, no changes needed for support of kbVersion 2
void CMainFrame::OnCustomEventGlossesEdit(wxCommandEvent& WXUNUSED(event))
{
	// glosses updating is potentially required
	// Ensure that we have a valid pointer to the m_pVertEditBar member
	// of the frame window class
	wxASSERT(m_pVertEditBar != NULL);
	wxASSERT(m_pRemovalsBar != NULL);

	CAdapt_ItView* pView = gpApp->GetView();
	wxASSERT(pView != NULL);
	CFreeTrans* pFreeTrans = gpApp->GetFreeTrans();
	wxASSERT(pFreeTrans != NULL);

	if (m_pVertEditBar == NULL)
	{
		wxMessageBox(
_T("Failure to obtain pointer to the vertical edit control bar in OnCustomEventAdaptationsEdit()"),
		_T(""), wxICON_EXCLAMATION);
		return;
	}

    // determine what setup is required: control is coming from either adaptationsStep or
    // freeTranslationsStep, when the order is adaptations before glosses; but if the order
    // is glosses before adaptations, then control is coming from either sourceTextStep or
    // adaptationsStep. These variations require separate initialization blocks.
	CAdapt_ItDoc* pDoc = gpApp->GetDocument();
	EditRecord* pRec = &gEditRecord;
	SPList* pSrcPhrases = gpApp->m_pSourcePhrases;
	bool bAllsWell = TRUE;

	if (gbVerticalEditInProgress)
	{
		if (gEntryPoint == sourceTextEntryPoint)
		{
            // when entry is due to a source text edit, we can count on all the source text
            // editing-related parameters in gEditRecord having previously been filled
            // out...
			if (gbAdaptBeforeGloss)
			{
				// user's chosen processing order is "do adaptations before glosses"
				if (gEditStep == adaptationsStep)
				{
                    // the PostMessage() which we are handling was fired from the end of
                    // the adaptations updating step, so we are still in that step and
                    // about to do the things needed to transition to the glosses step

					// we now must get glossing mode turned on
					if (gbGlossingVisible)
					{
						// glosses are visible already, so we only need turn glossing mode on
						pView->ToggleGlossingMode();
					}
					else
					{
                        // glosses are not currently made visible, first make them visible
                        // and then turn on glossing mode
						pView->ToggleSeeGlossesMode();
						pView->ToggleGlossingMode();
					}

                    // If the user removed all the CSourcePhrase instances of the editable
                    // span, there are no glosses to be updated, so check for this and if
                    // so, fill in the relevant gEditRecord members and then post the event
                    // for the next step -- we don't give the user a dialog to vary the
                    // step order here, because he won't have seen any feedback up to now
                    // that he's about to be in glosses step, so we unilaterally send him
                    // on to the next step
					if (pRec->nNewSpanCount == 0)
					{
						// user deleted everything... so move on
						pRec->nGlossStep_SpanCount = 0;
						pRec->nGlossStep_StartingSequNum = -1;
						pRec->nGlossStep_EndingSequNum = -1;

						gEditStep = glossesStep;
						pRec->bGlossStepEntered = TRUE; // prevent reinitialization
														// if user returns here

                        // post event for progressing to free translation mode. Note, we
                        // have not changed from glossesStep, hence the handler for this
                        // message will become active with gEditStep still set to
                        // glossesStep - this is how we inform the next step what the
                        // previous step was, and so the handler for the next step will
                        // finish off this current step before showing the GUI for the next
                        // step to the user
						wxCommandEvent eventCustom(wxEVT_Free_Translations_Edit);
						wxPostEvent(this, eventCustom);
						return;
					}

                    // Initializing must be done only once (the user can return to this
                    // step using interface buttons) -- initializing is done the first time
                    // glossesStep is entered in the vertical edit, so it is put within a
                    // test to prevent multiple initializations -- the flag in the
                    // following test is not set until the end of this current function
					if (!pRec->bGlossStepEntered)
					{
                        // set up the values for the glossesStep in the edit record, where
                        // the CCell drawing function can get them
						bool bAllWasOK;
						pRec->nGlossStep_StartingSequNum = pRec->nAdaptationStep_StartingSequNum;
						pRec->nGlossStep_EndingSequNum = pRec->nGlossStep_StartingSequNum
														+ pRec->nAdaptationStep_NewSpanCount - 1;
						pRec->nGlossStep_SpanCount = pRec->nAdaptationStep_NewSpanCount;

                        // here we set the initial state of the glosses span which is of
                        // course the final state of the previous adaptationsStep's span
                        // after the user has done all his edits (which could include
                        // mergers, retranslations and placeholder insertions, and those
                        // functionalities have to keep the value of
                        // pRec->nAdaptationStep_NewSpanCount up to date as the user does
                        // his work, and also pRec->nAdaptationStep_EndingSequNum - because
                        // the CCell::Draw() function which keeps the relevant text grayed
                        // out needs to be continuously aware where the span ends even when
                        // the user does editing which changes the span length; so deep
                        // copy the initial editable span, before user has a chance to do
                        // do any editing. This deep copy must only be done once
						bAllWasOK = pView->DeepCopySourcePhraseSublist(pSrcPhrases,
										pRec->nGlossStep_StartingSequNum,
										pRec->nGlossStep_EndingSequNum,
										&pRec->glossStep_SrcPhraseList);
						if (!bAllWasOK)
						{
							// if this failed, we must bail out of this vertical edit process
							gEditStep = glossesStep;
							pRec->bGlossStepEntered = TRUE;
							goto cancel;
						}
					}

					// ensure gEditStep remains set to glossesStep here, before returning
					gEditStep = glossesStep;

                    // record the fact that this step has been entered and initial values
                    // set up (it is done once only, see above)
					pRec->bGlossStepEntered = TRUE;

					if (pRec->bEditSpanHasGlosses)
					{
						// populate the combobox with the required removals data for glossesStep
						bAllsWell = pView->PopulateRemovalsComboBox(glossesStep, &gEditRecord);
						bAllsWell = bAllsWell; // avoid warning (continue, even if not populated)
						// put the glosses step's message in the multi-line read-only CEdit box
						pView->SetVerticalEditModeMessage(
_("Vertical Editing - glosses step: Type the needed glosses in the editable region. Earlier glosses are stored at the top of the Removed list. Gray text is not accessible. Glossing  mode is currently on."));

						// setup the toolbar with the vertical edit process control buttons and user
						// message, and make it show-able
						// whm added: the MFC version of vertical editing had the RemovalsBar always
						// showing. I'm keeping it hidden until it really is needed.
						// update static text in removals bar to indicate that clicking on item in
						// list copies to the phrase box
						wxStaticText* pStatic = (wxStaticText*)m_pRemovalsBar->FindWindowById(ID_STATIC_TEXT_REMOVALS);
						wxASSERT(pStatic != NULL);
						pStatic->SetLabel(_("Clicking on an item in the above list copies it to the Phrase Box, overwriting anything there."));
						pRemovalsBarSizer->Layout();
						if (!m_pRemovalsBar->IsShown())
						{
							m_pRemovalsBar->Show(TRUE);
						}
						m_pVertEditBar->Show(TRUE);

						// initiate a redraw of the frame and the client area
                        // whm Note: Client area is changing size so send a size event to
                        // get the layout to change since the doc/view framework won't do
                        // it for us.
						SendSizeEvent(); // forces the CMainFrame::SetSize() handler to run and
										 // do the needed redraw

						// place the phrase box at the start of the span, and update the layout etc
						int activeSequNum = pRec->nGlossStep_StartingSequNum;
						pView->PutPhraseBoxAtSequNumAndLayout(pRec,activeSequNum);
					}  // end TRUE block for test (pRec->bEditSpanHasGlosses)
					else
					{
                        // no glosses formerly, so we don't do the above extra steps
                        // because the user would not get to see their effect before they
                        // get clobbered by the setup of the freeTranslationsStep
                        // transition
						wxCommandEvent eventCustom(wxEVT_Free_Translations_Edit);
						wxPostEvent(this, eventCustom);
						return;
					}
				} // end TRUE block for test (gbEditStep == adaptationsStep)
/*rollback*/	else if (gEditStep == freeTranslationsStep)
				{
                    // The user was in free translations step, and requested a return to
                    // glosses step; we allow this whether or not there were originally
                    // glosses in the editable span because we assume a rollback is
                    // intentional to get glosses made or altered.

                    // Rollback has to set up the free translation span to be what it was
                    // before freeTranslationsStep was first entered -- which entails that
                    // the deep copied CSourcePhrase instances either side of the editable
                    // span be restored (these, after modifications were done to the
                    // modifications list, had the m_bHasFreeTrans, m_bStartFreeTrans,
                    // m_bEndFreeTrans flags all cleared. Failure to do this restoration
                    // will cause SetupCurrentFreeTranslationSection() to fail if
                    // freeTranslationsStep is entered a second time in this current
                    // vertical edit session. Similarly, rolling back from
                    // freeTransationsStep will possibly leave text in the edit box of the
                    // compose bar - it has to be cleared, etc.
					CMainFrame *pFrame = gpApp->GetMainFrame();
					wxASSERT(pFrame != NULL);
                    // whm: In wx the composeBar is created when the app runs and is always
                    // there but may be hidden.
					if (pFrame->m_pComposeBar != NULL)
					{
						wxTextCtrl* pEdit = (wxTextCtrl*)pFrame->m_pComposeBar->FindWindowById(IDC_EDIT_COMPOSE);
						wxASSERT(pEdit != NULL);
						if (pEdit != 0)
						{
							pEdit->ChangeValue(_T("")); // clear the box
						}
					}
//#ifdef _debugLayout
//ShowSPandPile(393, 1);
//ShowSPandPile(394, 1);
//ShowInvalidStripRange();
//#endif
                    // now restore the free translation span to what it was at last entry
                    // to freeTranslationsStep
					if (pRec->bFreeTranslationStepEntered && pRec->nFreeTranslationStep_SpanCount > 0 &&
						pRec->nFreeTrans_EndingSequNum != -1)
					{
                        // this gets the context either side of the user's selection
                        // correct, a further replacement below will replace the editable
                        // span with the instances at at the start of the earlier glossing
                        // step (the user may not have seen it in the GUI, but the list was
                        // populated nevertheless)
						int nHowMany = pRec->nFreeTranslationStep_SpanCount;
						wxASSERT(nHowMany != 0);
						wxASSERT(pRec->freeTranslationStep_SrcPhraseList.GetCount() > 0);
						bool bWasOK;
						bWasOK = pView->ReplaceCSourcePhrasesInSpan(pSrcPhrases,
							pRec->nFreeTranslationStep_StartingSequNum,
							nHowMany, // defines how many to remove to make the gap for the insertions
							&pRec->freeTranslationStep_SrcPhraseList,
							0, // start at index 0, ie. insert whole of deep copied list
							pRec->nFreeTranslationStep_SpanCount);
						if (!bWasOK)
						{
							// tell the user there was a problem, but keep processing even if
							// FALSE was returned (English message will suffice, the error is
							// unlikely
							wxMessageBox(_T("OnCustomEvenGlossesEdit() called ReplaceCSourcePhrasesInSpan() and the latter returned FALSE, at line 4535 in MainFrm.cpp; processing will continue however"));
							gpApp->LogUserAction(_T("OnCustomEvenGlossesEdit() called ReplaceCSourcePhrasesInSpan() and the latter returned FALSE, at line 4535 in MainFrm.cpp; processing continues"));
						}
						pView->UpdateSequNumbers(0); // make sure all are in proper sequence in the doc
//#ifdef _debugLayout
//ShowSPandPile(393, 2);
//ShowSPandPile(394, 2);
//ShowInvalidStripRange();
//#endif

                        // now make the free translation part of the gEditRecord think that
                        // freeTranslationsStep has not yet been entered
						pRec->bFreeTranslationStepEntered = FALSE;
						pView->GetDocument()->DeleteSourcePhrases(&pRec->freeTranslationStep_SrcPhraseList);
						pRec->nFreeTranslationStep_EndingSequNum = -1;
						pRec->nFreeTranslationStep_SpanCount = -1;
						pRec->nFreeTranslationStep_StartingSequNum = -1;
						// don't alter bVerseBasedSection because it is set only once
						// per vertical edit
					}

                    // we know we are coming from free translations mode being currently
                    // on, so we have to switch to glossing mode
					pFreeTrans->ToggleFreeTranslationMode();
                    // free translation step is usually has adapting mode set ON on entry,
                    // and we are wanting to be in glossingStep, so check gbIsGlossing and
                    // set up appropriately
					if (gbIsGlossing)
					{
						// glossing mode is on, so nothing more to do
						;
					}
					else
					{
                        // adapting mode is on, but glosses may or may not be visible, so
                        // check gbGlossingVisible first and get glosses visible if not
                        // already TRUE, then turn on glossing mode
						if (!gbGlossingVisible)
						{
							pView->ToggleSeeGlossesMode();
						}
						pView->ToggleGlossingMode();
					}

                    // If the user removed all the CSourcePhrase instances of the editable
                    // span, there are no glosses to be updated, because the span has no
                    // extent. We will try to externally catch and prevent a user rollback
                    // to glosses step when there is no extent, but just in case we don't,
                    // just bring the vertical edit to an end immediately -- this block
                    // should never be entered if I code correctly
					if (pRec->nNewSpanCount == 0)
					{
						wxCommandEvent eventCustom(wxEVT_End_Vertical_Edit);
						wxPostEvent(this, eventCustom);
						gEditStep = glossesStep;
						pRec->bGlossStepEntered = TRUE;
						return;
					}

                    // the span was entered earlier, and we are rolling back, so we have to
                    // set up the state as it was at the start of the of the previous
                    // glossesStep; and we have to restore the phrase box to the start of
                    // the editable span too (we don't bother to make any reference count
                    // decrements in the glossingKB, so this would be a source of ref
                    // counts being larger than strictly correct, but that is a totally
                    // benign effect)
					int nHowMany = pRec->nGlossStep_SpanCount;
					wxASSERT(nHowMany != 0);
					wxASSERT(pRec->glossStep_SrcPhraseList.GetCount() > 0);
					bool bWasOK;
					bWasOK = pView->ReplaceCSourcePhrasesInSpan(pSrcPhrases,
						pRec->nGlossStep_StartingSequNum,
						nHowMany, // defines how many to remove to make the gap for the insertions
						&pRec->glossStep_SrcPhraseList,
						0, // start at index 0, ie. insert whole of deep copied list
						pRec->nGlossStep_SpanCount);
					if (!bWasOK)
					{
						// tell the user there was a problem, but keep processing even if
						// FALSE was returned (English message will suffice, the error is
						// unlikely
						wxMessageBox(_T("OnCustomEvenGlossesEdit() called ReplaceCSourcePhrasesInSpan() and the latter returned FALSE, at line 4616 in MainFrm.cpp; processing will continue however"));
						gpApp->LogUserAction(_T("OnCustomEvenGlossesEdit() called ReplaceCSourcePhrasesInSpan() and the latter returned FALSE, at line 4616 in MainFrm.cpp; processing continues"));
					}
					pView->UpdateSequNumbers(0); // make sure all are in proper sequence in the doc
//#ifdef _debugLayout
//ShowSPandPile(393, 3);
//ShowSPandPile(394, 3);
//ShowInvalidStripRange();
//#endif

					// delete the instances in the freeTranslationsStep's list
					pDoc->DeleteSourcePhrases(&pRec->freeTranslationStep_SrcPhraseList);

					// place the phrase box at the start of the span, and update the layout etc
					int activeSequNum = pRec->nGlossStep_StartingSequNum;
					pView->PutPhraseBoxAtSequNumAndLayout(pRec,activeSequNum);
//#ifdef _debugLayout
//ShowSPandPile(393, 4);
//ShowSPandPile(394, 4);
//ShowInvalidStripRange();
//#endif
					// populate the combobox with the required removals data for adaptationsStep
					bAllsWell = pView->PopulateRemovalsComboBox(glossesStep, &gEditRecord);
					bAllsWell = bAllsWell; // avoid warning (continue, even if not populated)
					// put the adaptations step's message in the multi-line read-only CEdit box
					pView->SetVerticalEditModeMessage(
_("Vertical Editing - glosses step: Type the needed glosses in the editable region. Earlier glosses are stored at the top of the Removed list. Gray text is not accessible. Glossing  mode is currently on."));

					// setup the toolbar with the vertical edit process control buttons and user
					// message, and make it show-able
					// update static text in removals bar to indicate that clicking on item in
					// list copies to the phrase box
					wxStaticText* pStatic = (wxStaticText*)m_pRemovalsBar->FindWindowById(ID_STATIC_TEXT_REMOVALS);
					wxASSERT(pStatic != NULL);
					pStatic->SetLabel(_("Clicking on an item in the above list copies it to the Phrase Box, overwriting anything there."));
					pRemovalsBarSizer->Layout(); //m_pRemovalsBar->Refresh();
					if (!m_pRemovalsBar->IsShown())
					{
						m_pRemovalsBar->Show(TRUE);
					}
					m_pVertEditBar->Show(TRUE);

					// ensure gEditStep remains set to adaptationsStep here, before returning
					gEditStep = glossesStep;
//#ifdef _debugLayout
//ShowSPandPile(393, 5);
//ShowSPandPile(394, 5);
//ShowInvalidStripRange();
//#endif

                    // record the fact that this step has been entered and initial values
                    // set up (it is done once only, see above)
					pRec->bGlossStepEntered = TRUE;

					// initiate a redraw of the frame and the client area
					SendSizeEvent(); // forces the CMainFrame::SetSize() handler to run and do the needed redraw

				} // end block for result TRUE for test (gEditStep == freeTranslationsStep)
				else
				{
                    // ought not to happen because we should only be able to get to
                    // glossesStep by either normal progression from adaptationsStep or
                    // rollback from freeTranslationsStep, so, cancel out
					gEditStep = glossesStep;
					pRec->bGlossStepEntered = TRUE;
					goto cancel;
				}
			} // end block for test if (gAdaptBeforeGloss)
/****/		else
			{
				// user's chosen processing order is "do glosses before adaptations"
				if (gEditStep == sourceTextStep)
				{
                    // the wxPostEvent() which we are handling is fired from the end of the
                    // source text edit step, so control is still in that step, and about
                    // to transition to the glosses step if the editable span had some, if
                    // not, we transition from glosses step, after minimal setup, to the
                    // adaptations step

                    // the mode in effect when the Edit Source Text dialog was visible may
                    // not have been glossing mode, it may have been adapting mode, so we
                    // need to check and, if necessary, switch to glossing mode; moreover,
                    // if there were glosses in the editable section, we must make them
                    // visible, but if there were no glosses in the span, we'll do only
                    // minimal setup and transition to the next step
					if (gbIsGlossing)
					{
                        // glossing mode is currently ON (and adaptations are always
                        // visible in that mode too)
						;
					}
					else
					{
						// adapting mode is in effect, so change the mode to glossing mode
						if (!gbGlossingVisible)
						{
							// glosses are not yet visible, so make them so
							pView->ToggleSeeGlossesMode();
						}
						pView->ToggleGlossingMode();
					}

                    // if the user removed all the CSourcePhrase instances of the editable
                    // span, there are no glosses to be updated, so check for this and if
                    // so, fill in the relevant gEditRecord members and then post the event
                    // for the next step -- we don't give the user a dialog to vary the
                    // step order here, because he won't have seen any feedback up to now
                    // that he's about to be in glosses step, so we unilaterally send him
                    // on to the next step
					if (pRec->nNewSpanCount == 0)
					{
						// user deleted everything... so move on
						pRec->nGlossStep_SpanCount = 0;
						pRec->nGlossStep_StartingSequNum = -1;
						pRec->nGlossStep_EndingSequNum = -1;

						gEditStep = glossesStep;
						pRec->bGlossStepEntered = TRUE; // prevent reinitialization if user returns here

						// post event for progressing to adaptationStep
						wxCommandEvent eventCustom(wxEVT_Adaptations_Edit);
						wxPostEvent(this, eventCustom);
						return;
					}

					if (!pRec->bGlossStepEntered)
					{
						// put the values in the edit record, where the CCell drawing
						// function can get them
						bool bAllWasOK;
						pRec->nGlossStep_StartingSequNum = pRec->nStartingSequNum;
						pRec->nGlossStep_EndingSequNum = pRec->nStartingSequNum + pRec->nNewSpanCount - 1;
						pRec->nGlossStep_SpanCount = pRec->nNewSpanCount;

						// deep copy the initial editable span.  This deep copy
						// must only be done once
						bAllWasOK = pView->DeepCopySourcePhraseSublist(pSrcPhrases,
											pRec->nGlossStep_StartingSequNum,
											pRec->nGlossStep_EndingSequNum,
											&pRec->glossStep_SrcPhraseList);
						if (!bAllWasOK)
						{
							// if this failed, we must bail out of this vertical edit process
							gEditStep = glossesStep;
							pRec->bGlossStepEntered = TRUE;
							goto cancel;
						}
					}

					// ensure gEditStep remains set to glossStep here, before returning
					gEditStep = glossesStep;

                    // record the fact that this step has been entered and initial values
                    // set up (it is done once only, see above)
					pRec->bGlossStepEntered = TRUE;

					if (pRec->bEditSpanHasGlosses)
					{
						// populate the combobox with the required removals data for glossesStep
						bAllsWell = pView->PopulateRemovalsComboBox(glossesStep, &gEditRecord);
						bAllsWell = bAllsWell; // avoid warning (continue, even if not populated)
						// put the adaptations step's message in the multi-line read-only CEdit box
						pView->SetVerticalEditModeMessage(
_("Vertical Editing - glosses step: Type the needed glosses in the editable region. Earlier glosses are stored at the top of the Removed list. Gray text is not accessible. Glossing  mode is currently on."));

						// setup the toolbar with the vertical edit process control buttons and user
						// message, and make it show-able
						// update static text in removals bar to indicate that clicking on item in
						// list copies to the phrase box
						wxStaticText* pStatic = (wxStaticText*)m_pRemovalsBar->FindWindowById(ID_STATIC_TEXT_REMOVALS);
						wxASSERT(pStatic != NULL);
						pStatic->SetLabel(_("Clicking on an item in the above list copies it to the Phrase Box, overwriting anything there."));
						pRemovalsBarSizer->Layout();
						if (!m_pRemovalsBar->IsShown())
						{
							m_pRemovalsBar->Show(TRUE);
						}
						m_pVertEditBar->Show(TRUE);

						// initiate a redraw of the frame and the client area
						SendSizeEvent(); // forces the CMainFrame::SetSize() handler to run and
						                 // do the needed redraw

						// place the phrase box at the start of the span, and update the layout etc
						int activeSequNum = pRec->nGlossStep_StartingSequNum;
						pView->PutPhraseBoxAtSequNumAndLayout(pRec,activeSequNum);

					}  // end block for result TRUE for test (pRec->bEditSpanHasGlosses)
					else
					{
                        // no glosses formerly, so we don't do the above extra steps
                        // because the user would not get to see their effect before they
                        // get clobbered by the setup of the adaptationsStep transition
						wxCommandEvent eventCustom(wxEVT_Adaptations_Edit);
						wxPostEvent(this, eventCustom);
						return;
					}
				}
/*rollback*/	else if (gEditStep == adaptationsStep)
				{
                    // the user was in adaptations step, and requested a roll back to
                    // glosses step; we allow this whether or not there were originally
                    // glosses in the editable span because we assume a rollback is
                    // intentional to get glosses made or altered

                    // we know we are coming from adaptations step being currently on, so
                    // we have to switch to glossing step; moreover, glosses may or may not
                    // have been visible, but we have to be sure they will be visible in
                    // glossesStep
					if (!gbGlossingVisible)
					{
						// make glosses visible and have the glossing checkbox be shown
						// on the command bar
						pView->ToggleSeeGlossesMode();
					}
					pView->ToggleGlossingMode();

                    // If the user removed all the CSourcePhrase instances of the editable
                    // span, there are no glosses to be updated, because the span has no
                    // extent. We will try to externally catch and prevent a user rollback
                    // to glosses step when there is no extent, but just in case we don't,
                    // just bring the vertical edit to an end immediately -- this block
                    // should never be entered if I code correctly
					if (pRec->nNewSpanCount == 0)
					{
						wxCommandEvent eventCustom(wxEVT_End_Vertical_Edit);
						wxPostEvent(this, eventCustom);
						gEditStep = glossesStep;
						pRec->bGlossStepEntered = TRUE;
						return;
					}
                    // the span was entered earlier, and we are rolling back, so we have to
                    // set up the state as it was at the start of the of the previous
                    // glossesStep; (while in adaptationsStep the user may have changed the
                    // length of the editable span by doing placeholder insertions,
                    // retranslations, mergers) and we have to restore the phrase box to
                    // the start of the editable span too (we don't bother to make any
                    // reference count decrements in the glossingKB, so this would be a
                    // source of ref counts being larger than strictly correct, but that is
                    // a totally benign effect)

					// the user may have shortened or lengthened the span, so we just
					// use the nAdaptationStep_NewSpanCount value as that is the value on
					// last exit from the adaptationsStep -- this is how many we must remove
					// in order to insert the original glossesStep's deep copies in the
					// document's m_pSourcePhrases list at the editable span location
					int nHowMany = pRec->nAdaptationStep_NewSpanCount;
					wxASSERT(nHowMany != 0);
					wxASSERT(pRec->adaptationStep_SrcPhraseList.GetCount() > 0);
					bool bWasOK;
					bWasOK = pView->ReplaceCSourcePhrasesInSpan(pSrcPhrases,
						pRec->nGlossStep_StartingSequNum,
						nHowMany, // defines how many to remove to make the gap for the insertions
						&pRec->glossStep_SrcPhraseList,
						0, // start at index 0, ie. insert whole of deep copied list
						pRec->nGlossStep_SpanCount);
					if (!bWasOK)
					{
						// tell the user there was a problem, but keep processing even if
						// FALSE was returned (English message will suffice, the error is
						// unlikely
						wxMessageBox(_T("OnCustomEvenGlossesEdit() called ReplaceCSourcePhrasesInSpan() and the latter returned FALSE, at line 4877 in MainFrm.cpp; processing will continue however"));
						gpApp->LogUserAction(_T("OnCustomEvenGlossesEdit() called ReplaceCSourcePhrasesInSpan() and the latter returned FALSE, at line 4877 in MainFrm.cpp; processing continues"));
					}
					pView->UpdateSequNumbers(0); // make sure all are in proper sequence in the doc

					// restore original adaptationsStep counts to pre-extras values
					pRec->nAdaptationStep_EndingSequNum -= pRec->nAdaptationStep_ExtrasFromUserEdits;
					pRec->nAdaptationStep_ExtrasFromUserEdits = 0; // reinitialize, as the user may
																   // now edit differently than before
					// re-initialize the new spancount parameter which has become incorrect
					pRec->nAdaptationStep_NewSpanCount = pRec->nAdaptationStep_OldSpanCount;

					// delete the instances in the adaptationsStep's list
					pDoc->DeleteSourcePhrases(&pRec->adaptationStep_SrcPhraseList);

					// place the phrase box at the start of the span, and update the layout etc
					int activeSequNum = pRec->nGlossStep_StartingSequNum;
					pView->PutPhraseBoxAtSequNumAndLayout(pRec,activeSequNum);

					// populate the combobox with the required removals data for glossesStep
					bAllsWell = pView->PopulateRemovalsComboBox(glossesStep, &gEditRecord);
					bAllsWell = bAllsWell; // avoid warning (continue, even if not populated)
					// put the glosses step's message in the multi-line read-only CEdit box
					pView->SetVerticalEditModeMessage(
_("Vertical Editing - glosses step: Type the needed glosses in the editable region. Earlier glosses are stored at the top of the Removed list. Gray text is not accessible. Glossing  mode is currently on."));

					// setup the toolbar with the vertical edit process control buttons and user
					// message, and make it show-able
					// update static text in removals bar to indicate that clicking on item in
					// list copies to the phrase box
					wxStaticText* pStatic = (wxStaticText*)m_pRemovalsBar->FindWindowById(ID_STATIC_TEXT_REMOVALS);
					wxASSERT(pStatic != NULL);
					pStatic->SetLabel(_("Clicking on an item in the above list copies it to the Phrase Box, overwriting anything there."));
					pRemovalsBarSizer->Layout(); //m_pRemovalsBar->Refresh();
					if (!m_pRemovalsBar->IsShown())
					{
						m_pRemovalsBar->Show(TRUE);
					}
					m_pVertEditBar->Show(TRUE);

					// ensure gEditStep remains set to adaptationsStep here,
					// before returning
					gEditStep = glossesStep;

                    // record the fact that this step has been entered and initial values
                    // set up (it is done once only, see above)
					pRec->bGlossStepEntered = TRUE;

					// initiate a redraw of the frame and the client area
					SendSizeEvent(); // forces the CMainFrame::SetSize() handler to run and
					                 // do the needed redraw
				} // end block for result TRUE for test (gEditStep == glossesStep)
				else
				{
                    // ought not to happen because we should only be able to get to
                    // glossesStep by either normal progression from sourceTextStep or
                    // rollback from adaptationsStep, so, cancel out
					gEditStep = glossesStep;
					pRec->bGlossStepEntered = TRUE;
					goto cancel;
				}
			}
		 } // end block for result TRUE for test (gEntryPoint == sourceTextEntryPoint)
		else if (gEntryPoint == adaptationsEntryPoint)
		{
			// user has just edited an existing adaptation... we'll support this
			// option only in the wxWidgets codebase
			; // **** TODO in wxWidgets, perhaps some day ****
		}
		else
		{
            // if none of these, we've got an unexpected state which should never happen,
            // so cancel out
cancel:		;
			wxCommandEvent eventCustom(wxEVT_Cancel_Vertical_Edit);
			wxPostEvent(this, eventCustom);
			return;
		}
	}
}

// The following is the handler for a custom wxEVT_Free_Translations_Edit event message,
// sent to the window event loop by a wxPostEvent call
// BEW 26Mar10, no changes needed for support of doc version 5
// BEW 9July10, no changes needed for support of kbVersion 2
void CMainFrame::OnCustomEventFreeTranslationsEdit(wxCommandEvent& WXUNUSED(event))
{
	// free translations updating is potentially required
	// Ensure that we have a valid pointer to the m_pVertEditBar member
	// of the frame window class
	wxASSERT(m_pVertEditBar != NULL);
	wxASSERT(m_pRemovalsBar != NULL);

	CAdapt_ItView* pView = gpApp->GetView();

	if (m_pVertEditBar == NULL)
	{
		wxMessageBox(
_T("Failure to obtain pointer to the vertical edit control bar in OnCustomEventAdaptationsEdit()"),
		_T(""), wxICON_EXCLAMATION);
		return;
	}
	CFreeTrans* pFreeTrans = gpApp->GetFreeTrans();
	wxASSERT(pFreeTrans != NULL);

    // determine what setup is required: control is coming from either adaptationsStep or
    // glossesStep, the former when the order is adaptations before glosses; but if the
    // order is glosses before adaptations, then the latter. These variations require
    // separate initialization blocks.
    EditRecord* pRec = &gEditRecord;
	SPList* pSrcPhrases = gpApp->m_pSourcePhrases;
	bool bAllsWell = TRUE;

	if (gbVerticalEditInProgress)
	{
		if (gEntryPoint == sourceTextEntryPoint)
		{
            // when entry is due to a source text edit, we can count on all the source text
            // editing-related parameters in gEditRecord having previously been filled
            // out...
			if (gbAdaptBeforeGloss)
			{
				// user's chosen processing order is "do adaptations before glosses"
				if (gEditStep == glossesStep)
				{
                    // the PostMessage() which we are handling was fired from the end of
                    // the glosses updating step, so we are still in that step and about to
                    // do the things needed to transition to the free translations step

                    // we now must get free translations mode turned on; and we assume that
                    // the user will be using it for free translating the adaptation line,
                    // and so we also have to get glossing mode turned off - but leave
                    // glosses visible (which permits the user to still free translate the
                    // gloss line if he chooses)
					if (gbIsGlossing)
					{
						// glossing mode is currently ON, so we only need turn glossing mode off
						pView->ToggleGlossingMode();
					}
					else
					{
						// adaptation mode is in effect already, make glosses visible if they are not
						// already so
						if (!gbGlossingVisible)
							pView->ToggleSeeGlossesMode();
					}

                    // If the user removed all the CSourcePhrase instances of the editable
                    // span, there are no glosses or adaptations to be updated, but it is
                    // quite probable that one or more free translation sections are
                    // affected by such removal. So we must give the user the chance to
                    // retranslate, or copy the old ones from the removals combobox list
                    // and tweak them. However, if the user removed all the selection, then
                    // we must look at the free translation span - if it is coextensive
                    // with the editable span, then whole free translation section(s) were
                    // removed and there is nothing for the user to do; but if it is larger
                    // than the editable span, then the user must be shown the GUI as he
                    // has some editing of free translations to do.
					bool bShowGUI = FALSE; // default is that there were no free translations
										   // impinging on the editable span, so no need to
										   // show the GUI in this step
					if (pRec->bEditSpanHasFreeTranslations)
					{
						// the GUI may need to be shown, check further
						if (pRec->nNewSpanCount > 0)
						{
							// the user did not remove everything, so some free translating
							// needs to be done
							bShowGUI = TRUE;
						}
						else
						{
                            // the user removed everything in the editable span; this span
                            // may be coextensive with the ends of one or more consecutive
                            // free translations - which also have been removed (to the
                            // combo box list); we assume that the user needs to see the
                            // GUI only if the one or more free translations that were
                            // present extended beyond the start or end, or both, of the
                            // editable span - in that case only would some editing of the
                            // free translations impinging on the editable span need their
                            // meanings modified. That is, if the source words and the free
                            // translations of them where together removed, what is in the
                            // context can be assumed to be correct.

                            // NOTE: The above assumption is not linguistically defensible,
                            // as typically conjunctions need to be tweaked, but we will
                            // let the user do such tweaks in a manual read-through later
                            // on, to keep things as simple as possible for the vertical
                            // edit process. (If users ever complain about this, we can
                            // here do an extension of the free translation span to include
                            // single free translation either side of the removed text, so
                            // that any minor meaning tweaks to maintain connected
                            // readability can be done. I'm assuming that the frequency of
                            // this ever being an issue is so small as to not be worth the
                            // time coding for it.)
							int nOverlapCount = pRec->nFreeTrans_EndingSequNum -
								pRec->nFreeTrans_StartingSequNum + 1 - pRec->nOldSpanCount;
							if (nOverlapCount > 0)
							{
                                // the user needs to see the GUI, because the end of the
                                // earlier free translation overlaps the start of the
                                // former editable span or the start of the following free
                                // translation overlaps the end of the former editable
                                // span, or both; so that strongly implies those free
                                // translations need significant editing
								bShowGUI = TRUE;
							}
						}
					}
					if (!bShowGUI)
					{
                        // user deleted everything... perhaps including one of more
                        // complete free translation sections, so assume no big need for
                        // editing exists and move on
						pRec->nFreeTranslationStep_SpanCount = 0;
						pRec->nFreeTranslationStep_StartingSequNum = -1;
						pRec->nFreeTranslationStep_EndingSequNum = -1;

                        // BEW added 16Jul09, removing a document final word where a free
                        // translation ends carries into limbo with it the m_bEndFreeTrans
                        // = TRUE value, so we have to check if m_bHasFreeTrans is TRUE on
                        // the last CSourcePhrase and if so, and ensure that
                        // m_bEndFreeTrans is set TRUE there as well
						SPList::Node* pLastPosition = gpApp->m_pSourcePhrases->GetLast();
						CSourcePhrase* pFinalSrcPhrase = pLastPosition->GetData();
						if (pFinalSrcPhrase->m_bHasFreeTrans)
						{
							pFinalSrcPhrase->m_bEndFreeTrans = TRUE;
						}

						gEditStep = freeTranslationsStep;
						pRec->bFreeTranslationStepEntered = TRUE; // prevent reinitialization
								// if user returns to here,  -- though I don't think I'm going
								// to make that possible once this block has been entered

						// post event for progressing to (re-collecting) back translations step.
						wxCommandEvent eventCustom(wxEVT_Back_Translations_Edit);
						wxPostEvent(this, eventCustom);
						return;
					}
									
                    // now switch free translations mode ON; we do it here so that we don't
                    // bother to switch it on if the user is not going to be shown the GUI
                    // for this step; but in the backTranslationsStep's handler we will
                    // then have to test the value of gpApp->m_bFreeTranslationMode to
                    // determine whether or not free translations mode was turned on at the
                    // earlier step so as to make sure we turn it off only because we know
                    // it was turned on earlier!
					pFreeTrans->ToggleFreeTranslationMode(); // sets radio buttons to
						// the values defined by app's m_bDefineFreeTransByPunctuation flag
						// which may result in wrong radio button settings, so a call below
						// will set the buttons to their needed values using the
						// temporary flag value below, obtained from the EditRecord

					// the GUI is going to be shown, so restore the original value that the
					// flag m_bDefineFreeTransByPunctuation had when the free translation
					// section was first created - we get the value from  the EditRecord
					bool bOriginal_FreeTransByPunctuationValue = !pRec->bVerseBasedSection;
					
                    // BEW 27Feb12, for docV6 support of m_bSectionByVerse flag. What we
                    // need to do here is use the EditRecord's bVerseBasedSection value to
                    // set the GUI radio buttons to what they should be to agree with the
                    // free translation section that we are about to enter, but keep the
                    // m_bDefineFreeTransByPunctuation flag unchanged. The above line is
                    // therefore placed after the ToggleFreeTranslationMode() call -
                    // because the latter includes a call to ComposeBarGuts() which would
                    // override the button settings to follow the
                    // m_bDefineFreeTransByPunctuation value, instead of the one obtained
                    // from the EditRecord

					// now it's safe to set the radio buttons temporarily to possibly different
					// values
					gpApp->GetFreeTrans()->SetupFreeTransRadioButtons(bOriginal_FreeTransByPunctuationValue);

                    // Initializing must be done only once (the user can return to this
                    // step using interface buttons) -- initializing is done the first time
                    // glossesStep is entered in the vertical edit, so it is put within a
                    // test to prevent multiple initializations -- the flag in the
                    // following test is not set until the end of this current function
					if (!pRec->bFreeTranslationStepEntered)
					{
                        // set up the values for the freeTranslationsStep in the edit
                        // record, where the CCell drawing function can get them
						bool bAllWasOK;
						pRec->nFreeTranslationStep_StartingSequNum = pRec->nFreeTrans_StartingSequNum;
						pRec->nFreeTranslationStep_EndingSequNum = pRec->nFreeTrans_EndingSequNum;
                        // the above value is based on the editable span prior to any
                        // span-length changes which the user made while editing in
                        // adaptationsStep (such as mergers, retranslations or placeholder
                        // insertions), so if he changed the span length - whether longer
                        // or shorter, we have to make the same adjustment to the ending
                        // sequence number now; we also must take account of any shortening
                        // or lengthening due to the source text edit itself
						pRec->nFreeTranslationStep_EndingSequNum += pRec->nAdaptationStep_ExtrasFromUserEdits;
						int nShorterBy = pRec->nOldSpanCount - pRec->nNewSpanCount; // can be -ve, 0 or +ve
						pRec->nFreeTranslationStep_EndingSequNum -= nShorterBy;
						// calculate the span count & store it
						if (pRec->nFreeTranslationStep_EndingSequNum < pRec->nFreeTranslationStep_StartingSequNum)
						{
							pRec->nFreeTranslationStep_SpanCount = 0;
						}
						else
						{
							pRec->nFreeTranslationStep_SpanCount = pRec->nFreeTranslationStep_EndingSequNum -
								pRec->nFreeTranslationStep_StartingSequNum + 1;
						}

                        // here we set the initial state of the free translations span
                        // which is of course the final state of the previous glossesStep's
                        // span; so deep copy the initial state. This deep copy must only
                        // be done once. This deep copy is not needed for forward
                        // progression, but invaluable for rollback, as it ensures the free
                        // translation flags are cleared in the context of the editable
                        // span when the earlier step is set up again
						bAllWasOK = pView->DeepCopySourcePhraseSublist(pSrcPhrases,
											pRec->nFreeTranslationStep_StartingSequNum,
											pRec->nFreeTranslationStep_EndingSequNum,
											&pRec->freeTranslationStep_SrcPhraseList);
						if (!bAllWasOK)
						{
							// if this failed, we must bail out of this vertical edit process
							gEditStep = freeTranslationsStep;
							pRec->bFreeTranslationStepEntered = TRUE;
							goto cancel;
						}
					}

					// ensure gEditStep remains set to freeTranslationsStep here, before returning
					gEditStep = freeTranslationsStep;

                    // record the fact that this step has been entered and initial values
                    // set up (it is done once only, see above)
					pRec->bFreeTranslationStepEntered = TRUE;

					if (bShowGUI)
					{
						// populate the combobox with the required removals data for
						// freeTranslationsStep
						bAllsWell = pView->PopulateRemovalsComboBox(freeTranslationsStep, &gEditRecord);
						bAllsWell = bAllsWell; // avoid warning (continue even if not populated)
						// put the glosses step's message in the multi-line read-only CEdit box
						pView->SetVerticalEditModeMessage(
_("Vertical Editing - free translations step: Type the needed free translations in the editable region. Earlier free translations are stored at the top of the Removed list. Clicking on one copies it immediately into the Compose Bar's edit box, overwriting the default free translation there. Gray text is not accessible. Free translations mode is currently on and all free translation functionalities are enabled."));

						// setup the toolbar with the vertical edit process control buttons and user
						// message, and make it show-able
						// whm added: the MFC version of vertical editing had the RemovalsBar always
						// showing. I'm keeping it hidden until it really is needed.
						// update static text in removals bar to indicate that clicking on item in
						// list copies to the compose bar's text box
						wxStaticText* pStatic = (wxStaticText*)m_pRemovalsBar->FindWindowById(ID_STATIC_TEXT_REMOVALS);
						wxASSERT(pStatic != NULL);
						pStatic->SetLabel(
_("Clicking on an item in the above list copies it to the Compose Bar's text box, overwriting anything there."));
						pRemovalsBarSizer->Layout();
						if (!m_pRemovalsBar->IsShown())
						{
							m_pRemovalsBar->Show(TRUE);
						}
						m_pVertEditBar->Show(TRUE);

						// initiate a redraw of the frame and the client area
						SendSizeEvent(); // forces the CMainFrame::SetSize() handler to run and
										 // do the needed redraw

                        // BEW added 16Jul09, removing a document final word where a free
                        // translation ends carries into limbo with it the m_bEndFreeTrans
                        // = TRUE value, so we have to check if m_bHasFreeTrans is TRUE on
                        // the last CSourcePhrase and if so, and ensure that
                        // m_bEndFreeTrans is set TRUE there as well
						SPList::Node* pLastPosition = gpApp->m_pSourcePhrases->GetLast();
						CSourcePhrase* pFinalSrcPhrase = pLastPosition->GetData();
						if (pFinalSrcPhrase->m_bHasFreeTrans)
						{
							pFinalSrcPhrase->m_bEndFreeTrans = TRUE;
						}

						// place the phrase box at the start of the span, and update the
						// layout etc
						int activeSequNum = pRec->nFreeTranslationStep_StartingSequNum;
						pView->PutPhraseBoxAtSequNumAndLayout(pRec,activeSequNum);

					}  // end TRUE block for test (bShowGUI)
					else
					{
                        // no free translations formerly, or no need for the user to do any
                        // free translation updating, so we don't do the above extra steps
                        // because the user would not get to see their effect before they
                        // get clobbered by the setup of the backTranslationsStep
                        // transition
						wxCommandEvent eventCustom(wxEVT_Back_Translations_Edit);
						wxPostEvent(this, eventCustom);
						return;
					}
				} // end TRUE block for test (gbEditStep == glossesStep)
/*rollback*/	else if (gEditStep == backTranslationsStep)
				{
                    // the vertical edit design currently does not provided a rollback
                    // possibility from back translations step - the latter is automatic,
                    // has no GUI, and we won't put up the dialog for user control of next
                    // step at its end, but just close the vertical edit process; so if
                    // somehow control gets here, then just end
					wxCommandEvent eventCustom(wxEVT_End_Vertical_Edit);
					wxPostEvent(this, eventCustom);
				}
				else
				{
                    // ought not to happen because we should only be able to get to
                    // freeTranslationsStep by either normal progression from glossesStep
                    // or adaptationsStep, so cancel out
					gEditStep = freeTranslationsStep;
					pRec->bFreeTranslationStepEntered = TRUE;
					goto cancel;
				}
			} // end block for test if (gAdaptBeforeGloss)
/****/		else
			{
				// user's chosen processing order is "do glosses before adaptations"
				if (gEditStep == adaptationsStep)
				{
                    // the message post which we are handling was fired from the end of
                    // the adaptations updating step, so we are still in that step and
                    // about to do the things needed to transition to the free translations
                    // step

                    // we now must get free translations mode turned on; and we assume that
                    // the user will be using it for free translating the adaptation line,
                    // and so we also have to get glosses visible if not already so (which
                    // permits the user to still free translate the gloss line if he
                    // chooses)
					if (!gbGlossingVisible)
					{
						pView->ToggleSeeGlossesMode();
					}

                    // If the user removed all the CSourcePhrase instances of the editable
                    // span, there are no glosses or adaptations to be updated, but it is
                    // quite probable that one or more free translation sections are
                    // affected by such removal. So we must give the user the chance to
                    // retranslate, or copy the old ones from the removals combobox list
                    // and tweak them. However, if the user removed all the selection, then
                    // we must look at the free translation span - if it is coextensive
                    // with the editable span, then whole free translation section(s) were
                    // removed and there is nothing for the user to do; but if it is larger
                    // than the editable span, then the user must be shown the GUI as he
                    // has some editing of free translations to do.
					bool bShowGUI = FALSE; // default is that there were no
										   // free translations impinging on the
										   // editable span, so no need to show
										   // the GUI in this step
					if (pRec->bEditSpanHasFreeTranslations)
					{
						// the GUI may need to be shown, check further
						if (pRec->nNewSpanCount > 0)
						{
							// the user did not remove everything, so some free translating
							// needs to be done
							bShowGUI = TRUE;
						}
						else
						{
                            // the user removed everything in the editable span; this span
                            // may be coextensive with the ends of one or more consecutive
                            // free translations - which also have been removed (to the
                            // combo box list); we assume that the user needs to see the
                            // GUI only if the one or more free translations that were
                            // present extended beyond the start or end, or both, of the
                            // editable span - in that case only would some editing of the
                            // free translations impinging on the editable span need their
                            // meanings modified. That is, if the source words and the free
                            // translations of them where together removed, what is in the
                            // context can be assumed to be correct.

                            // NOTE: The above assumption is not linguistically defensible,
                            // as typically conjunctions need to be tweaked, but we will
                            // let the user do such tweaks in a manual read-through later
                            // on, to keep things as simple as possible for the vertical
                            // edit process. (If users ever complain about this, we can
                            // here do an extension of the free translation span to include
                            // single free translation either side of the removed text, so
                            // that any minor meaning tweaks to maintain connected
                            // readability can be done. I'm assuming that the frequency of
                            // this ever being an issue is so small as to not be worth the
                            // time coding for it.)
							int nOverlapCount = pRec->nFreeTrans_EndingSequNum -
								pRec->nFreeTrans_StartingSequNum + 1 - pRec->nOldSpanCount;
							if (nOverlapCount > 0)
							{
                                // the user needs to see the GUI, because the end of the
                                // earlier free translation overlaps the start of the
                                // former editable span or the start of the following free
                                // translation overlaps the end of the former editable
                                // span, or both; so that strongly implies those free
                                // translations need significant editing
								bShowGUI = TRUE;
							}
						}
					}
					if (!bShowGUI)
					{
                        // user deleted everything... perhaps including one of more
                        // complete free translation sections, so assume no big need for
                        // editing exists and move on
						pRec->nFreeTranslationStep_SpanCount = 0;
						pRec->nFreeTranslationStep_StartingSequNum = -1;
						pRec->nFreeTranslationStep_EndingSequNum = -1;

                        // BEW added 16Jul09, removing a document final word where a free
                        // translation ends carries into limbo with it the m_bEndFreeTrans
                        // = TRUE value, so we have to check if m_bHasFreeTrans is TRUE on
                        // the last CSourcePhrase and if so, and ensure that
                        // m_bEndFreeTrans is set TRUE there as well
						SPList::Node* pLastPosition = gpApp->m_pSourcePhrases->GetLast();
						CSourcePhrase* pFinalSrcPhrase = pLastPosition->GetData();
						if (pFinalSrcPhrase->m_bHasFreeTrans)
						{
							pFinalSrcPhrase->m_bEndFreeTrans = TRUE;
						}

						gEditStep = freeTranslationsStep;
						pRec->bFreeTranslationStepEntered = TRUE; // prevent reinitialization
								// if user returns to here,  -- though I don't think I'm going
								// to make that possible once this block has been entered

						// post event for progressing to (re-collecting) back translations step.
						wxCommandEvent eventCustom(wxEVT_Back_Translations_Edit);
						wxPostEvent(this, eventCustom);
						return;
					}

                    // now switch free translations mode ON; we do it here so that we don't
                    // bother to switch it on if the user is not going to be shown the GUI
                    // for this step; but in the backTranslationsStep's handler we will
                    // then have to test the value of gpApp->m_bFreeTranslationMode to
                    // determine whether or not free translations mode was turned on at the
                    // earlier step so as to make sure we turn it off only because we know
                    // it was turned on earlier!
					pFreeTrans->ToggleFreeTranslationMode(); // sets radio buttons to
						// the values defined by app's m_bDefineFreeTransByPunctuation flag
						// which may result in wrong radio button settings, so a call below
						// will set the buttons to their needed values using the
						// temporary flag value below, obtained from the EditRecord

					// the GUI is going to be shown, so restore the original value that the
					// flag m_bDefineFreeTransByPunctuation had when the free translation
					// section was first created - we get the value from  the EditRecord
					bool bOriginal_FreeTransByPunctuationValue = !pRec->bVerseBasedSection;
					
                    // BEW 27Feb12, for docV6 support of m_bSectionByVerse flag. What we
                    // need to do here is use the EditRecord's bVerseBasedSection value to
                    // set the GUI radio buttons to what they should be to agree with the
                    // free translation section that we are about to enter, but keep the
                    // m_bDefineFreeTransByPunctuation flag unchanged. The above line is
                    // therefore placed after the ToggleFreeTranslationMode() call -
                    // because the latter includes a call to ComposeBarGuts() which would
                    // override the button settings to follow the
                    // m_bDefineFreeTransByPunctuation value, instead of the one obtained
                    // from the EditRecord

					// now it's safe to set the radio buttons temporarily to possibly different
					// values
					gpApp->GetFreeTrans()->SetupFreeTransRadioButtons(bOriginal_FreeTransByPunctuationValue);
					
                    // Initializing must be done only once (the user can return to this
                    // step using interface buttons) -- initializing is done the first time
                    // glossesStep is entered in the vertical edit, so it is put within a
                    // test to prevent multiple initializations -- the flag in the
                    // following test is not set until the end of this current function
					if (!pRec->bFreeTranslationStepEntered)
					{
                        // set up the values for the freeTranslationsStep in the edit
                        // record, where the CCell drawing function can get them
						bool bAllWasOK;
						pRec->nFreeTranslationStep_StartingSequNum = pRec->nFreeTrans_StartingSequNum;
						pRec->nFreeTranslationStep_EndingSequNum = pRec->nFreeTrans_EndingSequNum;
                        // the above value is based on the editable span prior to any
                        // span-length changes which the user made while editing in
                        // adaptationsStep (such as mergers, retranslations or placeholder
                        // insertions), so if he changed the span length - whether longer
                        // or shorter, we have to make the same adjustment to the ending
                        // sequence number now
						pRec->nFreeTranslationStep_EndingSequNum += pRec->nAdaptationStep_ExtrasFromUserEdits;
						int nShorterBy = pRec->nOldSpanCount - pRec->nNewSpanCount; // can be -ve, 0 or +ve
						pRec->nFreeTranslationStep_EndingSequNum -= nShorterBy;
						// calculate the span count & store it
						if (pRec->nFreeTranslationStep_EndingSequNum < pRec->nFreeTranslationStep_StartingSequNum)
						{
							pRec->nFreeTranslationStep_SpanCount = 0;
						}
						else
						{
							pRec->nFreeTranslationStep_SpanCount = pRec->nFreeTranslationStep_EndingSequNum -
								pRec->nFreeTranslationStep_StartingSequNum + 1;
						}

                        // here we set the initial state of the free translations span
                        // which is of course the final state of the previous glossesStep's
                        // span; so deep copy the initial state. This deep copy must only
                        // be done once. Actually, since I doubt I'll provide a way to
                        // rollback to the start of the free translations step once control
                        // has moved on, this copy is superfluous, but I'll do it so that
                        // if I change my mind everything has nevertheless been set up right
						bAllWasOK = pView->DeepCopySourcePhraseSublist(pSrcPhrases,
										pRec->nFreeTranslationStep_StartingSequNum,
										pRec->nFreeTranslationStep_EndingSequNum,
										&pRec->freeTranslationStep_SrcPhraseList);
						if (!bAllWasOK)
						{
							// if this failed, we must bail out of this vertical edit process
							gEditStep = freeTranslationsStep;
							pRec->bFreeTranslationStepEntered = TRUE;
							goto cancel;
						}
					}

					// ensure gEditStep remains set to freeTranslationsStep here,
					// before returning
					gEditStep = freeTranslationsStep;

                    // record the fact that this step has been entered and initial values
                    // set up (it is done once only, see above)
					pRec->bFreeTranslationStepEntered = TRUE;

					if (bShowGUI)
					{
						// populate the combobox with the required removals data for
						// freeTranslationsStep
						bAllsWell = pView->PopulateRemovalsComboBox(freeTranslationsStep, &gEditRecord);

						// put the glosses step's message in the multi-line read-only CEdit box
						pView->SetVerticalEditModeMessage(
_("Vertical Editing - free translations step: Type the needed free translations in the editable region. Earlier free translations are stored at the top of the Removed list. Clicking on one copies it immediately into the Compose Bar's edit box, overwriting the default free translation there. Gray text is not accessible. Free translations mode is currently on and all free translation functionalities are enabled."));

                        // setup the toolbar with the vertical edit process control buttons
                        // and user message, and make it show-able
                        // whm added: the MFC version of vertical editing had the
                        // RemovalsBar always showing. I'm keeping it hidden until it
                        // really is needed.
                        // update static text in removals bar to indicate that clicking on
                        // item in list copies to the compose bar's text box
						wxStaticText* pStatic = (wxStaticText*)m_pRemovalsBar->FindWindowById(ID_STATIC_TEXT_REMOVALS);
						wxASSERT(pStatic != NULL);
						pStatic->SetLabel(
_("Clicking on an item in the above list copies it to the Compose Bar's text box, overwriting anything there."));
						pRemovalsBarSizer->Layout();
						if (!m_pRemovalsBar->IsShown())
						{
							m_pRemovalsBar->Show(TRUE);
						}
						m_pVertEditBar->Show(TRUE);

						// initiate a redraw of the frame and the client area
                        // whm Note: Client area is changing size so send a size event to
                        // get the layout to change since the doc/view framework won't do
                        // it for us.
						SendSizeEvent(); // forces the CMainFrame::SetSize() handler to run and
										 // do the needed redraw

                        // BEW added 16Jul09, removing a document final word where a free
                        // translation ends carries into limbo with it the m_bEndFreeTrans
                        // = TRUE value, so we have to check if m_bHasFreeTrans is TRUE on
                        // the last CSourcePhrase and if so, and ensure that
                        // m_bEndFreeTrans is set TRUE there as well
						SPList::Node* pLastPosition = gpApp->m_pSourcePhrases->GetLast();
						CSourcePhrase* pFinalSrcPhrase = pLastPosition->GetData();
						if (pFinalSrcPhrase->m_bHasFreeTrans)
						{
							pFinalSrcPhrase->m_bEndFreeTrans = TRUE;
						}

						// place the phrase box at the start of the span, and update the layout etc
						int activeSequNum = pRec->nFreeTranslationStep_StartingSequNum;
						pView->PutPhraseBoxAtSequNumAndLayout(pRec,activeSequNum);

					}  // end TRUE block for test (bShowGUI)
					else
					{
                        // no free translations formerly, or no need for the user to do any
                        // free translation updating, so we don't do the above extra steps
                        // because the user would not get to see their effect before they
                        // get clobbered by the setup of the backTranslationsStep
                        // transition
						wxCommandEvent eventCustom(wxEVT_Back_Translations_Edit);
						wxPostEvent(this, eventCustom);
						return;
					}
				} // end TRUE block for test (gbEditStep == glossesStep)
/*rollback*/	else if (gEditStep == backTranslationsStep)
				{
                    // the vertical edit design currently does not provided a rollback
                    // possibility from back translations step - the latter is automatic,
                    // has no GUI, and we won't put up the dialog for user control of next
                    // step at its end, but just close the vertical edit process; so if
                    // somehow control gets here, then just end
					wxCommandEvent eventCustom(wxEVT_End_Vertical_Edit);
					wxPostEvent(this, eventCustom);
				}
				else
				{
                    // ought not to happen because we should only be able to get to
                    // freeTranslationsStep by either normal progression from glossesStep,
                    // so cancel out
					gEditStep = freeTranslationsStep;
					pRec->bFreeTranslationStepEntered = TRUE;
					goto cancel;
				}
			}
		 } // end block for result TRUE for test (gEntryPoint == sourceTextEntryPoint)
		else if (gEntryPoint == adaptationsEntryPoint)
		{
			// user has just edited an existing adaptation... we'll support this
			// option only in the wxWidgets codebase
			; // **** TODO in wxWidgets, some day perhaps ****
		}
		else
		{
			// if none of these, we've got an unexpected state which should never happen,
			// so cancel out
cancel:		;
			wxCommandEvent eventCustom(wxEVT_Cancel_Vertical_Edit);
			wxPostEvent(this, eventCustom);
			return;
		}
	}
}

// The following is the handler for a CUSTOM_EVENT_BACK_TRANSLATIONS_EDIT event message, sent
// to the window event loop by a PostMessage(CUSTOM_EVENT_BACK_TRANSLATIONS_EDIT,0,0) call
// BEW 26Mar10, no changes needed for support of doc version 5
// BEW 9July10, no changes needed for support of kbVersion 2
void CMainFrame::OnCustomEventBackTranslationsEdit(wxCommandEvent& WXUNUSED(event))
{
	CAdapt_ItView* pView = gpApp->GetView();
	wxASSERT(pView != NULL);
	CFreeTrans* pFreeTrans = gpApp->GetFreeTrans();
	wxASSERT(pFreeTrans != NULL);

	if (gpApp->m_bFreeTranslationMode)
	{
		// free translations mode was turned on in the freeTranslationsStep
		// (a call to StoreFreeTranslationOnLeaving() should have been done
		// prior to entry, and if not  then the final free translation would not have
		// been stored filtered at the anchor location)
		pFreeTrans->ToggleFreeTranslationMode();
	}
	bool bOK;
	bOK = pView->RecreateCollectedBackTranslationsInVerticalEdit(&gEditRecord, sourceTextEntryPoint);
	if (!bOK)
	{
		// unlikely to fail, give a warning if it does
		wxMessageBox(_("Warning: recollecting the back translations did not succeed. Try doing it manually."),
			_T(""), wxICON_EXCLAMATION);
	}

	// unilaterally end the vertical edit process - don't provide a rollback chance
	//this->PostMessage(CUSTOM_EVENT_END_VERTICAL_EDIT,0,0);
	wxCommandEvent eventCustom(wxEVT_End_Vertical_Edit);
	wxPostEvent(this, eventCustom);
	return;
}

/// BEW 26Mar10, no changes needed for support of doc version 5
/// BEW 9July10, no changes needed for support of kbVersion 2
void CMainFrame::OnCustomEventEndVerticalEdit(wxCommandEvent& WXUNUSED(event))
{
	CAdapt_ItDoc* pDoc = gpApp->GetDocument();
	EditRecord* pRec = &gEditRecord;
	CLayout* pLayout = gpApp->m_pLayout;
	CFreeTrans* pFreeTrans = gpApp->GetFreeTrans();
	wxASSERT(pFreeTrans != NULL);

	wxASSERT(m_pVertEditBar != NULL);
	wxASSERT(m_pRemovalsBar != NULL);

	// turn receiving of synchronized scrolling messages back on, if we temporarily have
	// turned them off
	if (gbVerticalEdit_SynchronizedScrollReceiveBooleanWasON)
	{
		// it was formerly ON, so restore it
		 gbIgnoreScriptureReference_Receive = FALSE;
		 wxCommandEvent evt;
		pDoc->OnAdvancedReceiveSynchronizedScrollingMessages(evt); // toggle it back ON
		gbVerticalEdit_SynchronizedScrollReceiveBooleanWasON = FALSE; // restore default setting
	}

	if (gbVerticalEditInProgress)
	{
		CAdapt_ItView* pView = gpApp->GetView();

		// if free translations mode is still ON, turn it off
		if (gpApp->m_bFreeTranslationMode)
		{
			pFreeTrans->ToggleFreeTranslationMode();
		}

		// hide the toolbar with the vertical edit process control buttons and user message,
		// and make it show-able
		if (m_pVertEditBar->IsShown())
		{
			m_pVertEditBar->Hide();
		}
		// whm: wx version also hides the removalsBar
		if (m_pRemovalsBar->IsShown())
		{
			m_pRemovalsBar->Hide();
		}

        // typically, the user will have entered text in the phrase box, and we don't want
        // it lost at the restoration of the original mode; while we can't be sure there
        // will be text in the box when this handler is called, in our favour is the fact
        // that there is no copy of the source text in vertical edit mode and so the phrase
        // box won't have anything in it unless the user typed something there. So, while
        // we risk putting a rubbish word into the KB, in most situations we won't be and
        // so we'll unilaterally do the store now - using whatever mode (either glossing or
        // adapting) is currently still in effect
		pView->DoConditionalStore(); // parameter defaults TRUE, FALSE, are in effect

		// restore the original mode,
		pView->RestoreMode(gbGlossingVisible, gbIsGlossing, &gEditRecord);
		gEditStep = noEditStep; // no need to pretend any longer that vertical edit is in a step

		// put the phrase box at a suitable and safe location in the document
		pView->RestoreBoxOnFinishVerticalMode();

		// populate the combobox with the required removals data for the returned-to state
		bool bFilledListOK = FALSE;
		if (pRec->bGlossingModeOnEntry)
		{
			// when the user first entered the vertical edit state, glossing mode was ON, so
			// populate the combobox with the list of removed glosses as it currently stands
			bFilledListOK = pView->PopulateRemovalsComboBox(glossesStep, pRec);
			bFilledListOK = bFilledListOK; // avoid warning (continue even if not filled)
		}
		else
		{
			// when the user first entered the vertical edit state, glossing mode was OFF, so
			// populate the combobox with the list of removed adaptations as it currently stands
			bFilledListOK = pView->PopulateRemovalsComboBox(adaptationsStep, pRec);
			bFilledListOK = bFilledListOK; // avoid warning (continue even if not filled)
		}

		// initiate a redraw of the frame and the client area (Note, this is MFC's CFrameWnd
		// member function called RecalcLayout(), not my CAdapt_ItView member function of same name
		//pFWnd->RecalcLayout(); // the bNotify parameter is default TRUE, let it do so, no harm
							   // if the window is not embedded
		SendSizeEvent(); // forces the CMainFrame::SetSize() handler to run and do the needed redraw

		// clear by initializing (but the gEditRecord's removed data lists, which are maintained
		// within the gEditRecord global struct, are left intact while the Adapt It session is alive)
		pView->InitializeEditRecord(gEditRecord); // clears gbVerticalEditInProgress as well
		gEntryPoint = noEntryPoint;
		gEditStep = noEditStep;
		gbEditingSourceAndDocNotYetChanged = TRUE;

		// ensure the active strip is layout out with correct pile locations, do this
		// even if we have scrolled and the active strip won't be drawn (needed because in
		// some situations after a vertical edit, the pile locations aren't updated correctly)
		int nActiveStripIndex = (pView->GetPile(gpApp->m_nActiveSequNum))->GetStripIndex();
		pLayout->RelayoutActiveStrip(pView->GetPile(gpApp->m_nActiveSequNum), nActiveStripIndex,
			pLayout->GetGapWidth(), pLayout->GetLogicalDocSize().x);

		pView->Invalidate(); // get the layout drawn
		pLayout->PlaceBox();
	}
	return;
}

/// BEW 26Mar10, no changes needed for support of doc version 5
// BEW 27Feb12 added gbVerticalEditIsEnding = TRUE;
void CMainFrame::OnCustomEventCancelVerticalEdit(wxCommandEvent& WXUNUSED(event))
{
	// turn receiving of synchronized scrolling messages back on, if we temporarily have
	// turned them off
	if (gbVerticalEdit_SynchronizedScrollReceiveBooleanWasON)
	{
		// it was formerly ON, so restore it
		 gbIgnoreScriptureReference_Receive = FALSE;
		 wxCommandEvent evt;
		gpApp->GetDocument()->OnAdvancedReceiveSynchronizedScrollingMessages(evt); // toggle it back ON
		gbVerticalEdit_SynchronizedScrollReceiveBooleanWasON = FALSE; // restore default setting
	}
	// roll back through the steps doing restorations and set up original situation

	//CAdapt_ItDoc* pDoc = gpApp->GetDocument();
	bool bWasOK = TRUE;
	CAdapt_ItView* pView = gpApp->GetView();
	CFreeTrans* pFreeTrans = gpApp->GetFreeTrans();
	SPList* pSrcPhrases = gpApp->m_pSourcePhrases;
	wxASSERT(pSrcPhrases != NULL);
	EditRecord* pRec = &gEditRecord;
	// whm: we are in CMainFrame, so just assert that the member's pointers to the bars are valid
	wxASSERT(m_pVertEditBar != NULL);
	wxASSERT(m_pComposeBar != NULL);
	wxTextCtrl* pEdit = (wxTextCtrl*)m_pComposeBar->FindWindowById(IDC_EDIT_COMPOSE);
	wxASSERT(pEdit != NULL);
	if (gbVerticalEditInProgress)
	{
        // gEditStep, on entry, has the step which is current when the cause of the
        // cancellation request posted the cancellation event; the switches in the blocks
        // below then determine where the cancellation rollback steps begin; and as each
        // step is rolled back, control falls through to the previous step. Rollback goes
        // only until the original entry point's state is restored.

        // The order of cancellation steps depends on whether the user's preferred step
        // order was adaptations before glosses (the default), or vise versa
		if (gbAdaptBeforeGloss)
		{
			// adaptations step entered before glosses step, which is default order
			switch (gEditStep)
			{
			case noEditStep:
				return; // don't do anything if we don't have vertical edit mode currently on
			case backTranslationsStep:
				// backTranslationsStep is never an entry point for the vertical edit process,
				// but because of the potential for a failure during backTranslationsStep, we
				// have to cater for cancellation from within this step

			case freeTranslationsStep:

				if (gEntryPoint == freeTranslationsEntryPoint)
				{
					// rollback this step and then exit after posting end request

					// clean up & restore original state
					//this->PostMessage(CUSTOM_EVENT_END_VERTICAL_EDIT,0,0);
					wxCommandEvent eventCustom(wxEVT_End_Vertical_Edit);
					wxPostEvent(this, eventCustom);
					// whm Note: This event also calls the code which hides any of the
					// vertical edit tool bars so they are not seen from the main window.
					return;
				}
				else
				{
					// entry point was at an earlier step
					gEditStep = freeTranslationsStep; // unneeded, but it documents where we are
                    // whm changed 1Apr09 SetValue() to ChangeValue() below so that is
                    // doesn't generate the wxEVT_COMMAND_TEXT_UPDATED event, which now
                    // deprecated SetValue() generates.
					pEdit->ChangeValue(_T("")); // clear the box
					// now restore the free translation span to what it was at last entry to
					// freeTranslationsStep
					if (pRec->bFreeTranslationStepEntered && pRec->nFreeTranslationStep_SpanCount > 0 &&
						pRec->nFreeTrans_EndingSequNum != -1)
					{
						int nHowMany = pRec->nFreeTranslationStep_SpanCount;
						wxASSERT(nHowMany != 0);
						wxASSERT(pRec->freeTranslationStep_SrcPhraseList.GetCount() > 0);
						bool bWasOK;
						bWasOK = pView->ReplaceCSourcePhrasesInSpan(pSrcPhrases,
							pRec->nFreeTranslationStep_StartingSequNum,
							nHowMany, // defines how many to remove to make the gap for the insertions
							&pRec->freeTranslationStep_SrcPhraseList,
							0, // start at index 0, ie. insert whole of deep copied list
							pRec->nFreeTranslationStep_SpanCount);
						if (!bWasOK)
						{
							// tell the user there was a problem, but keep processing even if
							// FALSE was returned (English message will suffice, the error is
							// unlikely
							wxMessageBox(_T("OnCustomEventCancelVerticalEdit() called ReplaceCSourcePhrasesInSpan() and the latter returned FALSE, at line 5791 in MainFrm.cpp; processing will continue however"));
							gpApp->LogUserAction(_T("OnCustomEventCancelVerticalEdit() called ReplaceCSourcePhrasesInSpan() and the latter returned FALSE, at line 5791 in MainFrm.cpp; processing continues"));
						}
						pView->UpdateSequNumbers(0); // make sure all are in proper sequence in the doc
					}
					pFreeTrans->ToggleFreeTranslationMode(); // turn off free translation mode
				} // fall through
			case glossesStep:
				if (gEntryPoint == glossesEntryPoint)
				{
					// rollback this step and then exit after posting end request

					// clean up & restore original state
					wxCommandEvent eventCustom(wxEVT_End_Vertical_Edit);
					wxPostEvent(this, eventCustom);
					// whm Note: This event also calls the code which hides any of the
					// vertical edit tool bars so they are not seen from the main window.
					return;
				}
				else
				{
					// entry point was at an earlier step
					if (!gbIsGlossing)
					{
						// turn on glossing mode if it is not currently on
						if (!gbGlossingVisible)
						{
							pView->ToggleSeeGlossesMode();
						}
						pView->ToggleGlossingMode();
					}
					gEditStep = glossesStep;
					if (pRec->nNewSpanCount > 0)
					{
						// restore the editable span to what it was at the start of glossesStep,
						// i.e. the end of the previous adaptationsStep
						int nHowMany = pRec->nGlossStep_SpanCount;
						wxASSERT(nHowMany != 0);
						wxASSERT(pRec->glossStep_SrcPhraseList.GetCount() > 0);
						bool bWasOK;
						bWasOK = pView->ReplaceCSourcePhrasesInSpan(pSrcPhrases,
							pRec->nGlossStep_StartingSequNum,
							nHowMany, // defines how many to remove to make the gap for the insertions
							&pRec->glossStep_SrcPhraseList,
							0, // start at index 0, ie. insert whole of deep copied list
							pRec->nGlossStep_SpanCount);
						if (!bWasOK)
						{
							// tell the user there was a problem, but keep processing even if
							// FALSE was returned (English message will suffice, the error is
							// unlikely
							wxMessageBox(_T("OnCustomEventCancelVerticalEdit() called ReplaceCSourcePhrasesInSpan() and the latter returned FALSE, at line 5842 in MainFrm.cpp; processing will continue however"));
							gpApp->LogUserAction(_T("OnCustomEventCancelVerticalEdit() called ReplaceCSourcePhrasesInSpan() and the latter returned FALSE, at line 5842 in MainFrm.cpp; processing continues"));
						}
						pView->UpdateSequNumbers(0); // make sure all are in proper sequence in the doc
						// leave deletion of contents of freeTranslationStep_SrcPhraseList until
						// the final call of InitializeEditRecord()
					}
					SendSizeEvent(); // forces the CMainFrame::SetSize() handler to run and do
									 // the needed redraw
					pView->ToggleGlossingMode(); // turn off glossing mode
				} // fall through
			case adaptationsStep:
				if (gEntryPoint == adaptationsEntryPoint)
				{
					// rollback this step and then exit after posting end request

					// clean up & restore original state
					wxCommandEvent eventCustom(wxEVT_End_Vertical_Edit);
					wxPostEvent(this, eventCustom);
					// whm Note: This event also calls the code which hides any of the
					// vertical edit tool bars so they are not seen from the main window.
					return;
				}
				else
				{
					// entry point was at an earlier step
					gEditStep = adaptationsStep;
					if (pRec->nNewSpanCount > 0)
					{
						// restore the editable span to what it was when adaptationsStep
						// was started
						int nHowMany = pRec->nAdaptationStep_NewSpanCount;
						wxASSERT(nHowMany != 0);
						wxASSERT(pRec->adaptationStep_SrcPhraseList.GetCount() > 0);
						bool bWasOK;
						bWasOK = pView->ReplaceCSourcePhrasesInSpan(pSrcPhrases,
							pRec->nAdaptationStep_StartingSequNum,
							nHowMany, // defines how many to remove to make the gap for the insertions
							&pRec->adaptationStep_SrcPhraseList,
							0, // start at index 0, ie. insert whole of deep copied list
							pRec->nAdaptationStep_OldSpanCount);
						if (!bWasOK)
						{
							// tell the user there was a problem, but keep processing even if
							// FALSE was returned (English message will suffice, the error is
							// unlikely
							wxMessageBox(_T("OnCustomEventCancelVerticalEdit() called ReplaceCSourcePhrasesInSpan() and the latter returned FALSE, at line 5888 in MainFrm.cpp; processing will continue however"));
							gpApp->LogUserAction(_T("OnCustomEventCancelVerticalEdit() called ReplaceCSourcePhrasesInSpan() and the latter returned FALSE, at line 5888 in MainFrm.cpp; processing continues"));
						}
						pView->UpdateSequNumbers(0); // make sure all are in proper sequence in the doc
					}
					SendSizeEvent(); // forces the CMainFrame::SetSize() handler to run and
									 // do the needed redraw
				} // fall through
			case sourceTextStep:
				if (gEntryPoint == sourceTextEntryPoint)
				{
                    // restore to the number of instances when editableSpan had not had any
                    // user edits yet (they don't have to be the correct ones, as the
                    // restoration of the cancel span will remove them all and insert the
                    // cancel span ones, later)
					gEditStep = sourceTextStep;
					//bool bWasOK = TRUE;
					int nHowMany = 0;
					bool bNewIsShorter = FALSE;
					bool bOldIsShorter = FALSE;
					if (pRec->nOldSpanCount > pRec->nNewSpanCount)
					{
						bNewIsShorter = TRUE;
						nHowMany = pRec->nOldSpanCount - pRec->nNewSpanCount; // this many insertions
					}
					else if (pRec->nNewSpanCount > pRec->nOldSpanCount)
					{
						bOldIsShorter = TRUE;
						nHowMany = pRec->nNewSpanCount - pRec->nOldSpanCount; // this many deletions
					}
					if (nHowMany != 0)
					{
						// only if shorter or longer do we need to make an insertion or deletion, respectively
						if (bNewIsShorter)
						{
                            // need to make some insertions, just take them from start of
                            // cancel span (the only need we have is to move the right
                            // context rightwards so is gets located correctly before the
                            // replacement later on)
							bool bWasOK;
							bWasOK = pView->ReplaceCSourcePhrasesInSpan(pSrcPhrases,
								pRec->nStartingSequNum + pRec->nNewSpanCount,
								0, // no deletions wanted
								&pRec->cancelSpan_SrcPhraseList,
								0, // start at index 0, ie. insert whole of deep copied list
								nHowMany);
						if (!bWasOK)
						{
							// tell the user there was a problem, but keep processing even if
							// FALSE was returned (English message will suffice, the error is
							// unlikely
							wxMessageBox(_T("OnCustomEventCancelVerticalEdit() called ReplaceCSourcePhrasesInSpan() and the latter returned FALSE, at line 5939 in MainFrm.cpp; processing will continue however"));
							gpApp->LogUserAction(_T("OnCustomEventCancelVerticalEdit() called ReplaceCSourcePhrasesInSpan() and the latter returned FALSE, at line 5939 in MainFrm.cpp; processing continues"));
						}
						}
						if (bOldIsShorter)
						{
							bool bWasOK;
							bWasOK = pView->ReplaceCSourcePhrasesInSpan(pSrcPhrases,
								pRec->nStartingSequNum + pRec->nOldSpanCount,
								nHowMany, // defines how many to delete
								&pRec->cancelSpan_SrcPhraseList,
								0, // need an index, but we don't use cancelSpan_SrcPhraseList
								0);
							if (!bWasOK)
							{
								// tell the user there was a problem, but keep processing even if
								// FALSE was returned (English message will suffice, the error is
								// unlikely
								wxMessageBox(_T("OnCustomEventCancelVerticalEdit() called ReplaceCSourcePhrasesInSpan() and the latter returned FALSE, at line 5957 in MainFrm.cpp; processing will continue however"));
								gpApp->LogUserAction(_T("OnCustomEventCancelVerticalEdit() called ReplaceCSourcePhrasesInSpan() and the latter returned FALSE, at line 5957 in MainFrm.cpp; processing continues"));
							}
						}
					}
                    // some of the instances in the span above are wrong, but the span is
                    // now co-extensive with the cancel span, so when we overwrite this
                    // with the cancel span, we've restored the original state (except
                    // perhaps if the propagation span sticks out past the end of the
                    // cancel span) - we do these copies in the sourceTextStp case below.
					pView->UpdateSequNumbers(0); // make sure all are in proper sequence in the doc

					// handle the user edits done in the Edit Source Text dialog
					nHowMany = pRec->nCancelSpan_EndingSequNum + 1 - pRec->nCancelSpan_StartingSequNum;
					wxASSERT(nHowMany != 0);
					bWasOK = pView->ReplaceCSourcePhrasesInSpan(pSrcPhrases,
						pRec->nCancelSpan_StartingSequNum,
						nHowMany, // defines how many to remove to make the gap for the insertions
						&pRec->cancelSpan_SrcPhraseList,
						0, // start at index 0, ie. insert whole of deep copied list
						nHowMany);
					if (!bWasOK)
					{
						// tell the user there was a problem, but keep processing even if
						// FALSE was returned (English message will suffice, the error is
						// unlikely
						wxMessageBox(_T("OnCustomEventCancelVerticalEdit() called ReplaceCSourcePhrasesInSpan() and the latter returned FALSE, at line 5983 in MainFrm.cpp; processing will continue however"));
						gpApp->LogUserAction(_T("OnCustomEventCancelVerticalEdit() called ReplaceCSourcePhrasesInSpan() and the latter returned FALSE, at line 5983 in MainFrm.cpp; processing continues"));
					}
					pView->UpdateSequNumbers(0); // make sure all are in proper sequence in the doc

					// if the end of the propagation span is beyond end of cancel span, restore
					// those extras too
					if (pRec->nPropagationSpan_EndingSequNum > pRec->nCancelSpan_EndingSequNum)
					{
						nHowMany = pRec->nPropagationSpan_EndingSequNum - pRec->nCancelSpan_EndingSequNum;
						bool bWasOK;
						bWasOK = pView->ReplaceCSourcePhrasesInSpan(pSrcPhrases,
							pRec->nPropagationSpan_StartingSequNum,
							nHowMany, // defines how many to remove to make the gap for the insertions
							&pRec->propagationSpan_SrcPhraseList,
							0, // index into propSpan list for start
							pRec->propagationSpan_SrcPhraseList.GetCount());
						if (!bWasOK)
						{
							// tell the user there was a problem, but keep processing even if
							// FALSE was returned (English message will suffice, the error is
							// unlikely
							wxMessageBox(_T("OnCustomEventCancelVerticalEdit() called ReplaceCSourcePhrasesInSpan() and the latter returned FALSE, at line 6005 in MainFrm.cpp; processing will continue however"));
							gpApp->LogUserAction(_T("OnCustomEventCancelVerticalEdit() called ReplaceCSourcePhrasesInSpan() and the latter returned FALSE, at line 6005 in MainFrm.cpp; processing continues"));
						}
						pView->UpdateSequNumbers(0); // make sure all are in proper sequence in the doc
					}
				}
				break;
			} // end of switch (gEditStep)
		} // end of TRUE block for test (gbAdaptBeforeGloss)
		else
		{
			// glosses step entered before adaptations step (an unusual order, selectable
			// in Preferences...)
			switch (gEditStep)
			{
			case noEditStep:
				return; // don't do anything if we don't have vertical edit mode currently on
			case backTranslationsStep:
				// backTranslationsStep is never an entry point for the vertical edit process,
				// but because of the potential for a failure during backTranslationsStep, we
				// have to cater for cancellation from within this step

			case freeTranslationsStep:
				if (gEntryPoint == freeTranslationsEntryPoint)
				{
					// rollback this step and then exit after posting end request

					// clean up & restore original state
					wxCommandEvent eventCustom(wxEVT_End_Vertical_Edit);
					wxPostEvent(this, eventCustom);
					// whm Note: This event also calls the code which hides any of the
					// vertical edit tool bars so they are not seen from the main window.
					return;
				}
				else
				{
					// entry point was at an earlier step
					gEditStep = freeTranslationsStep;
					pEdit->ChangeValue(_T("")); // clear the box

					// now restore the free translation span to what it was at last entry to
					// freeTranslationsStep ie. as it was at the end of adaptationsStep
					if (pRec->bFreeTranslationStepEntered &&
						pRec->nFreeTranslationStep_SpanCount > 0 &&
						pRec->nFreeTrans_EndingSequNum != -1)
					{
						int nHowMany = pRec->nFreeTranslationStep_SpanCount;
						wxASSERT(nHowMany != 0);
						wxASSERT(pRec->freeTranslationStep_SrcPhraseList.GetCount() > 0);
						bool bWasOK;
						bWasOK = pView->ReplaceCSourcePhrasesInSpan(pSrcPhrases,
							pRec->nFreeTranslationStep_StartingSequNum,
							nHowMany, // defines how many to remove to make the gap for the insertions
							&pRec->freeTranslationStep_SrcPhraseList,
							0, // start at index 0, ie. insert whole of deep copied list
							pRec->nFreeTranslationStep_SpanCount);
						if (!bWasOK)
						{
							// tell the user there was a problem, but keep processing even if
							// FALSE was returned (English message will suffice, the error is
							// unlikely
							wxMessageBox(_T("OnCustomEventCancelVerticalEdit() called ReplaceCSourcePhrasesInSpan() and the latter returned FALSE, at line 6066 in MainFrm.cpp; processing will continue however"));
							gpApp->LogUserAction(_T("OnCustomEventCancelVerticalEdit() called ReplaceCSourcePhrasesInSpan() and the latter returned FALSE, at line 6066 in MainFrm.cpp; processing continues"));
						}
						pView->UpdateSequNumbers(0); // make sure all are in proper sequence in the doc
					}
					pFreeTrans->ToggleFreeTranslationMode(); // turn off free translation mode
				} // fall through
			case adaptationsStep:
				if (gEntryPoint == adaptationsEntryPoint)
				{
					// rollback this step and then exit after posting end request

					// clean up & restore original state
					wxCommandEvent eventCustom(wxEVT_End_Vertical_Edit);
					wxPostEvent(this, eventCustom);
					// whm Note: This event also calls the code which hides any of the
					// vertical edit tool bars so they are not seen from the main window.
					return;
				}
				else
				{
					// entry point was at an earlier step
					if (gbIsGlossing)
					{
						pView->ToggleGlossingMode(); // turn glossing mode off, as well as See Glosses
						pView->ToggleSeeGlossesMode();
					}
					else  if (gbGlossingVisible)
					{
						// See Glosses is enabled, so turn if off, glossing mode is already off
						pView->ToggleSeeGlossesMode();
					}
					gEditStep = adaptationsStep;
					if (pRec->nNewSpanCount > 0)
					{
						// restore the editable span to what it was at the start of adaptationsStep
						// ie. how it was at the end of the previous glossesStep
						int nHowMany = pRec->nAdaptationStep_NewSpanCount;
						wxASSERT(nHowMany != 0);
						wxASSERT(pRec->adaptationStep_SrcPhraseList.GetCount() > 0);
						bool bWasOK;
						bWasOK = pView->ReplaceCSourcePhrasesInSpan(pSrcPhrases,
							pRec->nAdaptationStep_StartingSequNum,
							nHowMany, // defines how many to remove to make the gap for the insertions
							&pRec->adaptationStep_SrcPhraseList,
							0, // start at index 0, ie. insert whole of deep copied list
							pRec->nAdaptationStep_OldSpanCount);
						if (!bWasOK)
						{
							// tell the user there was a problem, but keep processing even if
							// FALSE was returned (English message will suffice, the error is
							// unlikely
							wxMessageBox(_T("OnCustomEventCancelVerticalEdit() called ReplaceCSourcePhrasesInSpan() and the latter returned FALSE, at line 6118 in MainFrm.cpp; processing will continue however"));
							gpApp->LogUserAction(_T("OnCustomEventCancelVerticalEdit() called ReplaceCSourcePhrasesInSpan() and the latter returned FALSE, at line 6118 in MainFrm.cpp; processing continues"));
						}
						pView->UpdateSequNumbers(0); // make sure all are in proper sequence in the doc
						// leave deletion of contents of freeTranslationStep_SrcPhraseList until
						// the final call of InitializeEditRecord()
					}
					SendSizeEvent(); // forces the CMainFrame::SetSize() handler to run and do
									 // the needed redraw
				} // fall through
			case glossesStep:
				if (gEntryPoint == glossesEntryPoint)
				{
					// rollback this step and then exit after posting end request

					// clean up & restore original state
					wxCommandEvent eventCustom(wxEVT_End_Vertical_Edit);
					wxPostEvent(this, eventCustom);
					// whm Note: This event also calls the code which hides any of the
					// vertical edit tool bars so they are not seen from the main window.
					return;
				}
				else
				{
					// entry point was at an earlier step
					if (!gbIsGlossing)
					{
						if (!gbGlossingVisible)
						{
							pView->ToggleSeeGlossesMode(); // turn on See Glosses
						}
						pView->ToggleGlossingMode(); // turn on glossing mode
					}
					gEditStep = glossesStep;
					if (pRec->nNewSpanCount > 0)
					{
						// restore the editable span to what it was at the start of glossesStep
						int nHowMany = pRec->nGlossStep_SpanCount;
						wxASSERT(nHowMany != 0);
						wxASSERT(pRec->glossStep_SrcPhraseList.GetCount() > 0);
						bool bWasOK;
						bWasOK = pView->ReplaceCSourcePhrasesInSpan(pSrcPhrases,
							pRec->nGlossStep_StartingSequNum,
							nHowMany, // defines how many to remove to make the gap for the insertions
							&pRec->glossStep_SrcPhraseList,
							0, // start at index 0, ie. insert whole of deep copied list
							pRec->nGlossStep_SpanCount);
						if (!bWasOK)
						{
							// tell the user there was a problem, but keep processing even if
							// FALSE was returned (English message will suffice, the error is
							// unlikely
							wxMessageBox(_T("OnCustomEventCancelVerticalEdit() called ReplaceCSourcePhrasesInSpan() and the latter returned FALSE, at line 6170 in MainFrm.cpp; processing will continue however"));
							gpApp->LogUserAction(_T("OnCustomEventCancelVerticalEdit() called ReplaceCSourcePhrasesInSpan() and the latter returned FALSE, at line 6170 in MainFrm.cpp; processing continues"));
						}
						pView->UpdateSequNumbers(0); // make sure all are in proper sequence in the doc
						// leave deletion of contents of freeTranslationStep_SrcPhraseList until
						// the final call of InitializeEditRecord()
					}
					SendSizeEvent(); // forces the CMainFrame::SetSize() handler to run and
									 // do the needed redraw
					pView->ToggleGlossingMode(); // turn glossing mode back off
				} // fall through
			case sourceTextStep:
				if (gEntryPoint == sourceTextEntryPoint)
				{
					// rollback this step and then exit...
                    // restore to the number of instances when editableSpan had not had any
                    // user edits yet (they don't have to be the correct ones, as the
                    // restoration of the cancel span will remove them all and insert the
                    // cancel span ones, later)
					int nHowMany = 0;
					bool bNewIsShorter = FALSE;
					bool bOldIsShorter = FALSE;

					if (pRec->nOldSpanCount > pRec->nNewSpanCount)
					{
						bNewIsShorter = TRUE;
						nHowMany = pRec->nOldSpanCount - pRec->nNewSpanCount; // this many insertions
					}
					else if (pRec->nNewSpanCount > pRec->nOldSpanCount)
					{
						bOldIsShorter = TRUE;
						nHowMany = pRec->nNewSpanCount - pRec->nOldSpanCount; // this many deletions
					}
					if (nHowMany != 0)
					{
						// only if shorter or longer do we need to make an insertion or deletion, respectively
						if (bNewIsShorter)
						{
                            // need to make some insertions, just take them from start of
                            // cancel span (the only need we have is to move the right
                            // context rightwards so is gets located correctly before the
                            // replacement later on)
							bool bWasOK;
							bWasOK = pView->ReplaceCSourcePhrasesInSpan(pSrcPhrases,
								pRec->nStartingSequNum + pRec->nNewSpanCount,
								0, // no deletions wanted
								&pRec->cancelSpan_SrcPhraseList,
								0, // start at index 0, ie. insert whole of deep copied list
								nHowMany);
							if (!bWasOK)
							{
								// tell the user there was a problem, but keep processing even if
								// FALSE was returned (English message will suffice, the error is
								// unlikely
								wxMessageBox(_T("OnCustomEventCancelVerticalEdit() called ReplaceCSourcePhrasesInSpan() and the latter returned FALSE, at line 6224 in MainFrm.cpp; processing will continue however"));
								gpApp->LogUserAction(_T("OnCustomEventCancelVerticalEdit() called ReplaceCSourcePhrasesInSpan() and the latter returned FALSE, at line 6224 in MainFrm.cpp; processing continues"));
							}
						}
						if (bOldIsShorter)
						{
							bool bWasOK;
							bWasOK = pView->ReplaceCSourcePhrasesInSpan(pSrcPhrases,
								pRec->nStartingSequNum + pRec->nOldSpanCount,
								nHowMany, // defines how many to delete
								&pRec->cancelSpan_SrcPhraseList,
								0, // need an index, but we don't use cancelSpan_SrcPhraseList
								0);
							if (!bWasOK)
							{
								// tell the user there was a problem, but keep processing even if
								// FALSE was returned (English message will suffice, the error is
								// unlikely
								wxMessageBox(_T("OnCustomEventCancelVerticalEdit() called ReplaceCSourcePhrasesInSpan() and the latter returned FALSE, at line 6242 in MainFrm.cpp; processing will continue however"));
								gpApp->LogUserAction(_T("OnCustomEventCancelVerticalEdit() called ReplaceCSourcePhrasesInSpan() and the latter returned FALSE, at line 6242 in MainFrm.cpp; processing continues"));
							}
						}
					}
                    // some of the instances in the span above are wrong, but the span is
                    // now co-extensive with the cancel span, so when we overwrite this
                    // with the cancel span, we've restored the original state (except
                    // perhaps if the propagation span sticks out past the end of the
                    // cancel span) - we do these copies in the sourceTextStp case below.
					pView->UpdateSequNumbers(0); // make sure all are in proper sequence in the doc
					gEditStep = sourceTextStep;

					// handle the user edits done in the Edit Source Text dialog
					nHowMany = pRec->nCancelSpan_EndingSequNum + 1 - pRec->nCancelSpan_StartingSequNum;
					wxASSERT(nHowMany != 0);
					bool bWasOK;
					bWasOK = pView->ReplaceCSourcePhrasesInSpan(pSrcPhrases,
						pRec->nCancelSpan_StartingSequNum,
						nHowMany, // defines how many to remove to make the gap for the insertions
						&pRec->cancelSpan_SrcPhraseList,
						0, // start at index 0, ie. insert whole of deep copied list
						nHowMany);
					if (!bWasOK)
					{
						// tell the user there was a problem, but keep processing even if
						// FALSE was returned (English message will suffice, the error is
						// unlikely
						wxMessageBox(_T("OnCustomEventCancelVerticalEdit() called ReplaceCSourcePhrasesInSpan() and the latter returned FALSE, at line 6270 in MainFrm.cpp; processing will continue however"));
						gpApp->LogUserAction(_T("OnCustomEventCancelVerticalEdit() called ReplaceCSourcePhrasesInSpan() and the latter returned FALSE, at line 6270 in MainFrm.cpp; processing continues"));
					}
					pView->UpdateSequNumbers(0); // make sure all are in proper sequence in the doc

					// if the end of the propagation span is beyond end of cancel span, restore those extras too
					if (pRec->nPropagationSpan_EndingSequNum > pRec->nCancelSpan_EndingSequNum)
					{
						nHowMany = pRec->nPropagationSpan_EndingSequNum - pRec->nCancelSpan_EndingSequNum;
						bool bWasOK;
						bWasOK = pView->ReplaceCSourcePhrasesInSpan(pSrcPhrases,
							pRec->nPropagationSpan_StartingSequNum,
							nHowMany, // defines how many to remove to make the gap for the insertions
							&pRec->propagationSpan_SrcPhraseList,
							0, // index into propSpan list for start
							pRec->propagationSpan_SrcPhraseList.GetCount());
						if (!bWasOK)
						{
							// tell the user there was a problem, but keep processing even if
							// FALSE was returned (English message will suffice, the error is
							// unlikely
							wxMessageBox(_T("OnCustomEventCancelVerticalEdit() called ReplaceCSourcePhrasesInSpan() and the latter returned FALSE, at line 6291 in MainFrm.cpp; processing will continue however"));
							gpApp->LogUserAction(_T("OnCustomEventCancelVerticalEdit() called ReplaceCSourcePhrasesInSpan() and the latter returned FALSE, at line 6291 in MainFrm.cpp; processing continues"));
						}
						pView->UpdateSequNumbers(0); // make sure all are in proper sequence in the doc
					}
				}
				break;
			} // end of switch (gEditStep)
		} // end of FALSE block for test (gbAdaptBeforeGloss)

		// clean up & restore original state
		wxCommandEvent eventCustom(wxEVT_End_Vertical_Edit);
		wxPostEvent(this, eventCustom);
		// whm Note: This event also calls the code which hides any of the
		// vertical edit tool bars so they are not seen from the main window.
	} // end of TRUE block for test (gbVerticalEditInProgress)

	// whm addition:
    // When vertical editing is canceled we should hide the m_pRemovalsBar, and
    // m_pVertEditBar, - any and all that are visible.
	if (m_pVertEditBar->IsShown())
	{
		m_pVertEditBar->Hide();
	}
	if (m_pRemovalsBar->IsShown())
	{
		m_pRemovalsBar->Hide();
	}
	SendSizeEvent(); // forces the CMainFrame::SetSize() handler to run and
					 // do the needed redraw;
	return;
}

// MFC: OnRemovalsComboSelChange() handles a click in the dropped down list, sending the
// string direct to the phrase box which is rebuilt, and with focus put there and cursor
// following the last character; but it doesn't handle a click on the already visible item
// in the combobox window - there isn't a way to make that work as far as I can tell
//
// (whm note: Bruce tried using a OnRemovalsComboSetFocus() handler, which worked, but
// blocked dropping down the list) so apparently the user will have to drop the list down
// manually rather than be able to just click on the text in the combo box window).
// BEW 26Mar10, no changes needed for support of doc version 5
void CMainFrame::OnRemovalsComboSelChange(wxCommandEvent& WXUNUSED(event))
{
	CAdapt_ItApp* pApp = &wxGetApp();
	CAdapt_ItView* pView = pApp->GetView();

	if (!ListBoxPassesSanityCheck((wxControlWithItems*)m_pRemovalsBarComboBox))
	{
		return;
	}

	int nIndex = m_pRemovalsBarComboBox->GetSelection();
	wxString theText;
	theText = m_pRemovalsBarComboBox->GetString(nIndex);
	wxASSERT(!theText.IsEmpty());

    // store the active srcPhrase's m_nSrcWords member's value for use in the test in
    // OnUpdateButtonUndoLastCopy() and other "state-recording" information, for use by the
    // same update handler
	gnWasNumWordsInSourcePhrase = gpApp->m_pActivePile->GetSrcPhrase()->m_nSrcWords; // the
														// merge state at active location
	gbWasGlossingMode = gbIsGlossing; // whether glossing or adapting when the copy is done
	gbWasFreeTranslationMode = gpApp->m_bFreeTranslationMode; // whether or not free
													// translation mode is on at that time
	gnWasSequNum = pApp->m_nActiveSequNum;

	// now put the text into m_targetBox in the view, and get the view redrawn,
	// or if freeTranslationsStep is in effect, into the edit box within the
	// ComposeBar
	gOldEditBoxTextStr.Empty();
	if (gEditStep == freeTranslationsStep || gpApp->m_bFreeTranslationMode)
	{
		wxASSERT(m_pComposeBar != NULL);
		CComposeBarEditBox* pEdit = (CComposeBarEditBox*)
										m_pComposeBar->FindWindowById(IDC_EDIT_COMPOSE);
		wxASSERT(pEdit != NULL);
		gOldEditBoxTextStr = pEdit->GetValue(); // in case Undo Last Copy button is clicked
		pEdit->ChangeValue(_T(""));
        // whm Note: SetValue() automatically (and by design) resets the dirty flag to
        // FALSE when called, because it is primarily designed to establish the initial
        // value of an edit control. Often when the initial value of a text control is
        // programatically set, we don't want it marked as "dirty". It should be marked
        // dirty when the user changes something. But, our ComposeBarEditBox is designed to
        // echo the compose bar's contents and is does that by checking for changes in the
        // compose bar's contents (the dirty flag). Therefore, calling SetValue() won't do
        // what we want because SetValue automatically resets the dirty flag to FALSE;
        // Instead, we need to call one of the other wxTextCtrl methods that sets the dirty
        // flag to TRUE. We could use Append(), WriteText() or even just use the <<
        // operator to insert text into the control. I'll use the WriteText() method, which
        // not only sets the dirty flag to TRUE, it also leaves the insertion point at the
        // end of the inserted text. Using WriteText() also has the benefit of setting the
        // insertion point at the end of the inserted text - so we don't need to call
        // SetSelection() to do so.
		pEdit->WriteText(theText); //pEdit->SetValue(theText);
		pEdit->SetFocus();

		return;
	}

	// if control gets here then the copy must therefore be going to the phrase box; so
	// store a copy of phrase box text here in case Undo Last Copy button is used later
	gOldEditBoxTextStr = pApp->m_pTargetBox->GetValue();

	// if auto capitalization is on, determine the source text's case propertiess
	bool bNoError = TRUE;
	if (gbAutoCaps)
	{
		bNoError = pApp->GetDocument()->SetCaseParameters(pApp->m_pActivePile->GetSrcPhrase()->m_key);
																// bIsSrcText is TRUE
		if (bNoError && gbSourceIsUpperCase && !gbMatchedKB_UCentry)
		{
			bNoError = pApp->GetDocument()->SetCaseParameters(theText,FALSE); // testing the non-source text
			if (bNoError && !gbNonSourceIsUpperCase && (gcharNonSrcUC != _T('\0')))
			{
				// a change to upper case is called for
				theText.SetChar(0,gcharNonSrcUC);
			}
		}
	}
	pApp->m_targetPhrase = theText;


	// the box may be bigger because of the text, so do a recalc of the layout
	CLayout* pLayout = pApp->m_pLayout;
#ifdef _NEW_LAYOUT
	pLayout->RecalcLayout(pApp->m_pSourcePhrases, keep_strips_keep_piles);
#else
	pLayout->RecalcLayout(pApp->m_pSourcePhrases, create_strips_keep_piles);
#endif
	pApp->m_pActivePile = pView->GetPile(pApp->m_nActiveSequNum);
	wxASSERT(pApp->m_pActivePile != NULL);

	// do a scroll if needed
	pApp->GetMainFrame()->canvas->ScrollIntoView(pApp->m_nActiveSequNum);

	// place cursor at end of the inserted text
	int length = theText.Length();
	pApp->m_nEndChar = pApp->m_nStartChar = length;

	// restore focus and make non-abandonable
	if (pApp->m_pTargetBox != NULL) // should always be the case
	{
		if (pApp->m_pTargetBox->IsShown())
		{
			pApp->m_pTargetBox->SetFocus();
			pApp->m_pTargetBox->m_bAbandonable = FALSE;
		}
	}
	pView->Invalidate(); // whm: Why call Invalidate here? (Because the text
        // could be different length than what was there before, and hence move piles about
        // or even cause the following strips to have different pile content. But to avoid
        // flicker we need to make use of clipping region soon. BEW)
	pLayout->PlaceBox();
}


