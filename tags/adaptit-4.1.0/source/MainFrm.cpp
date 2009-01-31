/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			MainFrm.cpp
/// \author			Bill Martin
/// \date_created	05 January 2004
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation for the CMainFrame class.
/// The CMainFrame class defines Adapt It's basic interface, including its menu bar, 
/// tool bar, control bar, compose bar, and status bar. 
/// \derivation		The CMainFrame class is derived from wxDocParentFrame and inherits 
/// its support for the document/view framework.
/////////////////////////////////////////////////////////////////////////////
// Pending Implementation Items in MainFrm (in order of importance): (search for "TODO")
// 1. Implement CAPS and NUMLOCK indicators on the status line
// 2. 
// 3. 
// 4. 
//
// Unanswered questions: (search for "???")
// 1. Do we want EnableDocking() of toolBar as in MFC?
// 
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
#include <wx/config.h> // for wxConfig

#ifdef __WXMSW__
#include <wx/msw/registry.h> // for wxRegKey - used in SantaFeFocus sync scrolling mechanism
#endif

#if !wxUSE_WXHTML_HELP
    #error "This program can't be built without wxUSE_WXHTML_HELP set to 1"
#endif // wxUSE_WXHTML_HELP

// wx docs say: "By default, the DDE implementation is used under Windows. DDE works within one computer only. 
// If you want to use IPC between different workstations you should define wxUSE_DDE_FOR_IPC as 0 before 
// including this header [<wx/ipc.h>]-- this will force using TCP/IP implementation even under Windows."
#ifdef useTCPbasedIPC
#define wxUSE_DDE_FOR_IPC 0
#include <wx/ipc.h> // for wxServer, wxClient and wxConnection
#endif

#include "Adapt_It.h"
#include "Adapt_It_Resources.h"
#include "Adapt_ItCanvas.h"

#include "Adapt_ItDoc.h"
#include "Adapt_ItView.h"
#include "PhraseBox.h"
#include "Cell.h"
#include "Pile.h"
#include "SourcePhrase.h"
#include "MainFrm.h"
#include "helpers.h"
#include "AdaptitConstants.h"
#include "XML.h"
#include "ComposeBarEditBox.h" // BEW added 15Nov08
// includes above

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
extern bool		gbEnableGlossing; // TRUE makes Adapt It revert to Shoebox functionality only

/// This global is defined in Adapt_ItView.cpp
extern bool		gbVerticalEdit_SynchronizedScrollReceiveBooleanWasON;

// next two are for version 2.0 which includes the option of a 3rd line for glossing

/// This global is defined in Adapt_ItView.cpp.
extern bool		gbIsGlossing; // when TRUE, the phrase box and its line have glossing text

/// This global is defined in Adapt_ItView.cpp.
extern bool		gbGlossingUsesNavFont;

/// This global is defined in Adapt_It.cpp.
extern CAdapt_ItApp* gpApp; // if we want to access it fast

/// This global is defined in PhraseBox.cpp.
extern  long	gnStart;

/// This global is defined in PhraseBox.cpp.
extern  long	gnEnd;

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

extern  bool	gbMatchedRetranslation;
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
	EVT_MENU(ID_EDIT_CONSISTENCY_CHECK, CMainFrame::OnEditConsistencyCheck)
	EVT_UPDATE_UI(ID_EDIT_CONSISTENCY_CHECK, CMainFrame::OnUpdateEditConsistencyCheck)
	EVT_UPDATE_UI(IDC_CHECK_KB_SAVE, CMainFrame::OnUpdateCheckKBSave)
	EVT_UPDATE_UI(IDC_CHECK_FORCE_ASK, CMainFrame::OnUpdateCheckForceAsk)
	EVT_UPDATE_UI(IDC_CHECK_SINGLE_STEP, CMainFrame::OnUpdateCheckSingleStep)
	EVT_ACTIVATE(CMainFrame::OnActivate) // to set focus to targetbox when visible
	//EVT_HELP(wxID_HELP,CMainFrame::OnHelp)
	EVT_MENU(wxID_HELP,CMainFrame::OnAdvancedHtmlHelp)
	EVT_MENU(ID_ONLINE_HELP,CMainFrame::OnOnlineHelp)
	EVT_MENU(ID_USER_FORUM,CMainFrame::OnUserForum)
	EVT_MENU(ID_HELP_USE_TOOLTIPS,CMainFrame::OnUseToolTips)
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

bool	gbSyncMsgReceived_DocScanInProgress = FALSE; // FALSE, except is TRUE if the document set on disk
													 // is being scanned in order to find the appropriate
													 // document file for honouring the synch scroll msg

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

	// process the message only if valid pointers to the view, document and phrasebox classes are obtained
	if (bGotPointersSuccessfully)
	{
		// process the message only if a project is open - Adapt It determines this by both KB pointers
		// being nonnull, or the following two flags both being TRUE - we will use the latter test
		if (gpApp->m_bKBReady && gpApp->m_bGlossingKBReady)
		{
			// what we do next depends on whether or not there is a document currently open; a non-empty
			// m_pSourcePhrases list is a sufficient test for that, provided pDoc is also not null
			if (pDoc != NULL && gpApp->m_pSourcePhrases->GetCount() != 0)
			{
				// a document is open, so first we will try matching the book, chapter and verse in it
				bGotCode = Get3LetterCode(gpApp->m_pSourcePhrases,strBookCode);

				if (!bGotCode)
				{
					goto scan; // try a multi-document scan, because we couldn't find a 3-letter code
							   // in the currently open document
				}
				else
				{
					// we obtained a 3-letter code, so now we must check if it matches the 3-letter code
					// in the passed in synch scroll message
					if (strBookCode != strThreeLetterBook)
					{
						goto scan; // try a multi-document scan, because the open Adapt It document does 
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
							goto scan;
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
							CPile* pPile = pView->GetPile(gnMatchedSequNumber);
							gpApp->m_pActivePile = pPile;
							CSourcePhrase* pSrcPhrase = pPile->m_pSrcPhrase;
							pView->Jump(gpApp,pSrcPhrase); // jump there
						}
					} // end block for trying to find a matching scripture reference in the open document
				} // end block for checking out the open document's 3-letter book code
			} // end block for testing an open AI document
			else
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
scan:			gbSyncMsgReceived_DocScanInProgress = TRUE; // turn on, so XML parsing goes to gpDocList

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
				wxString strDocName;
				wxString strDocPath;
				wxString strFolderPath;
				wxString strFolderName;
				bool bOK;
				wxArrayString* pList = &gpApp->m_acceptedFilesList; // holds the document filenames to be used
				pList->Clear(); // ensure it starts empty
				int nCount; // a count of how many files are in the CStringList

				if (gpApp->m_bBookMode)
				{
					// get the folder name, and the index for it (these could be different from the current
					// active folder's name, and the app's m_bBookIndex value)
					Code2BookFolderName(gpApp->m_pBibleBooks, strThreeLetterBook, strFolderName, theBookIndex);
					if (strFolderName.IsEmpty())
					{
						// could not get the folder name, so just ignore this scripture reference message
						gbSyncMsgReceived_DocScanInProgress = FALSE;
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
					gbSyncMsgReceived_DocScanInProgress = FALSE;
					return FALSE;
				}

				// now enumerate the files in the target folder (finds *.adt and *.xmx files, but excludes *.BAK.xml ones)
				bOK = gpApp->EnumerateDocFiles(pDoc, strFolderPath, TRUE); // TRUE == suppress Which Files dialog
				if (!bOK)
				{
					// if any directory contents give an error, we'll just not try further, and so do no sync
					gbSyncMsgReceived_DocScanInProgress = FALSE;
					bOK = ::wxSetWorkingDirectory(strSavedCurrentDirectoryPath); // restore old current directory
					return FALSE;
				}

				// our loop will loop across all the files, and we will not look into any *.adt ones; if there are
				// no files, then return without doing anything
				int nFound;
				int index;
				nCount = pList->GetCount();
				if (nCount == 0)
				{
					gbSyncMsgReceived_DocScanInProgress = FALSE;
					bOK = ::wxSetWorkingDirectory(strSavedCurrentDirectoryPath); // restore old current directory
					return FALSE;
				}
				for (index = 0; index < nCount; index++)
				{
					strDocName = pList->Item(index); //pList->GetNext(pos); // get a document name
		
					// check if it is a binary doc file - ignore it if so
					// wx version - we'll let the following test be made
					nFound = strDocName.Find(_T(".adt"));
					if (nFound > 0)
						continue; // its a legacy *.adt document (binary) file, so skip it

					// form the path to each of those remaining - these will be *.xml document files only
					strDocPath = strFolderPath + gpApp->PathSeparator + strDocName; 

					// clear the temporary list, gete ready for reading in the xml data and storing it in the list
					DeleteSourcePhrases_ForSyncScrollReceive(pDoc, gpDocList); // also removes gpDocList's contents

					// read in the XML data, forming CSourcePhrase instances and storing them in gpDocList (done
					// internally in AtDocEndTag() by testing for gbSyncMsgReceived_DocScanInProgress == TRUE)
					bool bReadOK;
					bReadOK = ReadDoc_XML(strDocPath,pDoc); // a global function, defined in XML.cpp

					// look inside to see if we can match the chapter:verse reference -- the following code
					// is cloned from the block above, and tweaked a bit (mostly m_pSourcePhrases replaced
					// with the global SPList pointer gpDocList). We have to check the 3-letter code here
					// because we might be iterating through all the docs in the Adaptations folder, and so
					// most would not be the wanted book.
					bGotCode = FALSE;
					bGotCode = Get3LetterCode(gpDocList,strBookCode);
					if (!bGotCode)
					{
						// we couldn't find a 3-letter code in the gpDocList list, so we'll ignore this
						// document file and skip to the next one, if any
						continue;
					}
					else
					{
						// we obtained a 3-letter code, so now we must check if it matches the 3-letter code
						// in the passed in synch scroll message
						if (strBookCode != strThreeLetterBook)
						{
							// the codes do not match, so skip to the next document file, if any
							continue;
						}
						else
						{
							// we matched the codes successfully, so we have a document file belonging to
							// the correct book. Now check if the target ch:verse reference is in it (remember
							// that there could be several doc files for the one Bible book, so failure here
							// does not mean we return, but just that we keep iterating)
							gnMatchedSequNumber = FindChapterVerseLocation(gpDocList,nChap,nVerse,strChapVerse);
							if (gnMatchedSequNumber == -1)
							{
								// the scripture reference is not in the gpDocList's document, so iterate
								continue;
							}
							else
							{
								// we successfully matched the scripture reference; make this the active document...
								// first we have to close off the existing document (saving it if it has been modified)
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
										pView->OnAdvancedFreeTranslationMode(uevent);
									}
									// erase the document, emtpy its m_pSourcePhrases list, delete its CSourcePhrase instances, etc
									pView->ClobberDocument();
									pView->Invalidate(); // force immediate update (user sees white client area)
								}

								// also, remove the gpDocList contents, we'll use OnOpenDocument to set up the document
								DeleteSourcePhrases_ForSyncScrollReceive(pDoc, gpDocList);

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
								pDoc->SetFilename(strDocPath,TRUE);
								bOK = ::wxSetWorkingDirectory(strFolderPath); // set the active folder to the path
								gbSyncMsgReceived_DocScanInProgress = FALSE; // clear it before parsing in the XML doc file
									// (otherwise, the CSourcePhrase instances would be stored in gpDocList, rather than
									// where we need them to go - which is m_pSourcePhrases in the document instance)

								// copy & tweak code from DocPage.cpp for getting the document open and view set up
								// with the phrase box at the wanted sequence number
								bOK = pDoc->OnOpenDocument(strDocPath);
								if (!bOK)
								{
									// IDS_LOAD_DOC_FAILURE
									wxMessageBox(_("Sorry, loading the document failed. (The file may be in use by another application. Or the file has become corrupt and must be deleted.)"), _T(""), wxICON_ERROR);
									// whm TODO: Should the app actually stop here ???
									wxExit(); //AfxAbort();
								}
								if (gpApp->nLastActiveSequNum >= (int)gpApp->m_pSourcePhrases->GetCount())
									gpApp->nLastActiveSequNum = gpApp->m_pSourcePhrases->GetCount() - 1;

								gpApp->nLastActiveSequNum = gnMatchedSequNumber;
								CPile* pPile = pView->GetPile(gpApp->nLastActiveSequNum);
								wxASSERT(pPile != NULL);
								int nFinish = 1;
								bool bSetSafely;
								bSetSafely = pView->SetActivePilePointerSafely(gpApp,gpApp->m_pSourcePhrases,
													gpApp->nLastActiveSequNum,gpApp->m_nActiveSequNum,nFinish);
								pPile = pView->GetPile(gpApp->m_nActiveSequNum);
								gpApp->m_pActivePile = pPile;
								CSourcePhrase* pSrcPhrase = pPile->m_pSrcPhrase;
								pView->Jump(gpApp,pSrcPhrase); // jump there
								return TRUE; // don't continue in the loop any longer
							}
						}
					}
				 } // end of loop for iterating over all doc files in the folder
				 gbSyncMsgReceived_DocScanInProgress = FALSE; // clear it
			} // end of else block for doing a scan of all doc files
		}
	}
	// if we've gotten here we have not successfully the sync scroll message
	return FALSE;
}

int FindChapterVerseLocation(SPList* pDocList, int nChap, int nVerse, const wxString& strChapVerse)
{
	// NOTE: in Adapt It, chapterless Bible books use 0 as the chapter number in the ch:verse string, so we must
	// test for nChap == 1 and if so, also check for a match of 0:verse
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

			// the first of the test battery is to try find an initial ch:verse, or 0:verse if
			// the bSecondTest flag is TRUE, at the start of the refStr - if we succeed, we need
			// go no further
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
				pDoc->DeleteSingleSrcPhrase(pSrcPhrase);
			}
			pList->Clear();
		}
	}
}

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

	// these dummy ID values are placeholders for unused entries in the accelerator below 
	// that are not implemented in the wx version
	int dummyID1 = -1;
	int dummyID2 = -1;
	int dummyID3 = -1;
	int dummyID4 = -1;
	int dummyID5 = -1;
	//int dummyID6 = -1;

    // Accelerators
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
    wxAcceleratorEntry entries[35]; //[43];
    entries[0].Set(wxACCEL_CTRL, (int) '1', ID_ALIGNMENT);
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
    entries[14].Set(wxACCEL_CTRL, (int) '2', ID_EDIT_MOVE_NOTE_BACKWARD); // whm checked OK
    entries[15].Set(wxACCEL_CTRL, (int) '3', ID_EDIT_MOVE_NOTE_FORWARD); // whm checked OK
    entries[16].Set(wxACCEL_CTRL, (int) 'V', wxID_PASTE); // standard wxWidgets ID
    entries[17].Set(wxACCEL_SHIFT, WXK_INSERT, wxID_PASTE); // standard wxWidgets ID
    entries[18].Set(wxACCEL_CTRL, (int) 'Q', ID_EDIT_SOURCE_TEXT); // whm checked OK
    entries[19].Set(wxACCEL_ALT, WXK_BACK, wxID_UNDO); // standard wxWidgets ID
    entries[20].Set(wxACCEL_CTRL, (int) 'Z', wxID_UNDO); // standard wxWidgets ID
    entries[21].Set(wxACCEL_CTRL, (int) 'J', ID_FILE_CLOSEKB); // whm checked OK - close project
    entries[22].Set(wxACCEL_CTRL, (int) 'N', wxID_NEW); // standard wxWidgets ID // whm checked OK
    entries[23].Set(wxACCEL_CTRL, (int) 'O', wxID_OPEN); // standard wxWidgets ID // whm checked OK
    entries[24].Set(wxACCEL_CTRL, (int) 'P', wxID_PRINT); // standard wxWidgets ID // whm checked OK
    entries[25].Set(wxACCEL_CTRL, (int) 'S', wxID_SAVE); // standard wxWidgets ID // whm checked OK
    entries[26].Set(wxACCEL_CTRL, (int) 'W', ID_FILE_STARTUP_WIZARD); // whm checked OK
    entries[27].Set(wxACCEL_CTRL, (int) 'F', wxID_FIND); // standard wxWidgets ID // whm checked OK
    entries[28].Set(wxACCEL_CTRL, (int) 'G', ID_GO_TO); // whm checked OK
    entries[29].Set(wxACCEL_NORMAL, WXK_F1, wxID_HELP); // standard wxWidgets ID // whm checked OK
    entries[30].Set(wxACCEL_NORMAL, WXK_F6, dummyID3); //ID_NEXT_PANE);
    entries[31].Set(wxACCEL_SHIFT, WXK_F6, dummyID4); //ID_PREV_PANE);
    entries[32].Set(wxACCEL_CTRL, (int) 'H', wxID_REPLACE); // standard wxWidgets ID // whm checked OK
    entries[33].Set(wxACCEL_CTRL, (int) 'K', ID_TOOLS_KB_EDITOR); // whm checked OK
    entries[34].Set(wxACCEL_CTRL, WXK_RETURN, dummyID5); //ID_TRIGGER_NIKB); // CTRL+Enter is Transliterate Mode TODO: check
    
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
	m_pToolBar = (wxToolBar*) NULL;			// handle/pointer to the toolBar
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
	wxToolBar* toolBar = new wxToolBar(this, -1, wxDefaultPosition, wxDefaultSize, style);
	wxASSERT(toolBar != NULL);
	m_pToolBar = toolBar;
	AIToolBarFunc( toolBar ); // this calls toolBar->Realize(), but we want the frame to be parent
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

	// MFC version also has 3 lines here to EnableDocking() of toolBar. Do won't use docking in 
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
	ControlBarFunc( controlBar, TRUE, TRUE );

	// Note: We are creating a controlBar which the doc/view framework knows
	// nothing about. The mainFrameSizer below takes care of the controlBar's
	// layout within the Main Frame. The controlBar is always visible.
	// Here and in the OnSize() method, we calculate the canvas' client
	// size, which must exclude the height of the controlBar.
	// Get and save the native height of our controlBar.
	wxSize controlBarSize;
	controlBarSize = m_pControlBar->GetSize();
	m_controlBarHeight = controlBarSize.GetHeight();

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
	// of the Free Translation compose bar in Free Translation mode.
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
	strVersionNumber << VERSION_MAJOR_PART; // 4
	strVersionNumber += _T(".");
	strVersionNumber << VERSION_MINOR_PART; // 0
	strVersionNumber += _T(".");
	strVersionNumber << VERSION_BUILD_PART; // 3
	wxStaticText* pVersionNum = (wxStaticText*)FindWindowById(ID_ABOUT_VERSION_NUM);
	pVersionNum->SetLabel(strVersionNumber);

	// Set About Dlg static texts for OS, system language and locale information
	wxString strHostOS;
#if defined(__WXMSW__)
	strHostOS = _("Microsoft Windows");
#elif defined(__WXGTK__)
	strHostOS = _("Linux");
#elif defined(__WXMAC__)
	strHostOS = _("Mac OS X");
#else
	strHostOS = _("Unknown");
#endif

	strHostOS.Trim(FALSE);
	strHostOS.Trim(TRUE);
	strHostOS = _T(' ') + strHostOS;
	wxStaticText* pStaticHostOS = (wxStaticText*)FindWindowById(ID_STATIC_HOST_OS);
	pStaticHostOS->SetLabel(strHostOS);
	
	wxString strUILanguage;
	// Fetch the UI language info from the global currLocalizationInfo struct
	strUILanguage = gpApp->currLocalizationInfo.curr_fullName;
	strUILanguage.Trim(FALSE);
	strUILanguage.Trim(TRUE);
	strUILanguage = _T(' ') + strUILanguage;
	wxStaticText* pStaticUILanguage = (wxStaticText*)FindWindowById(ID_STATIC_UI_LANGUAGE);
	pStaticUILanguage->SetLabel(strUILanguage);
	wxString tempStr;
	wxString locale = pApp->m_pLocale->GetLocale();
	if (!locale.IsEmpty())
	{
		wxStaticText* pStatic = (wxStaticText*)FindWindowById(ID_STATIC_SYS_LANGUAGE);
		tempStr = pApp->m_pLocale->GetLocale();
		tempStr = _T(' ') + tempStr;
		pStatic->SetLabel(tempStr);
	}

	if (!pApp->m_pLocale->GetSysName().IsEmpty())
	{
		wxStaticText* pStatic = (wxStaticText*)FindWindowById(ID_STATIC_SYS_LOCALE_NAME);
		tempStr = pApp->m_pLocale->GetSysName();
		// GetSysName() returns the following:
		// On wxMSW: English_United States.1252
		// On Ubuntu: en_US.UTF-8
		// On Mac OS X: C // TODO: check this!!
		tempStr = _T(' ') + tempStr;
		pStatic->SetLabel(tempStr);
	}

	if (!pApp->m_pLocale->GetCanonicalName().IsEmpty())
	{
		wxStaticText* pStatic = (wxStaticText*)FindWindowById(ID_STATIC_CANONICAL_LOCALE_NAME);
		tempStr = pApp->m_pLocale->GetCanonicalName();
		tempStr = _T(' ') + tempStr;
		pStatic->SetLabel(tempStr);
	}
	if (!pApp->m_systemEncodingName.IsEmpty())
	{
		wxStaticText* pStatic = (wxStaticText*)FindWindowById(ID_STATIC_SYS_ENCODING_NAME);
		tempStr = pApp->m_systemEncodingName; //m_systemEncodingName is assigned by calling wxLocale::GetSystemEncodingName() in the App's OnInit()
		// Windows: m_systemEncodingName = "windows-1252"
		//  Ubuntu: m_systemEncodingName = "UTF-8"
		//     Mac: m_systemEncodingName = <blank>
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

	// WX Docs show the following example for its OnMRUFile handler:
	// TODO: need to tweak this to also to insure that things are set up properly to 
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
			bool bOK = pDoc->OnOpenDocument(fileFromMRU);
			if (!bOK)
			{
				// whm TODO: put the code here for updating the doc title that MFC version had in its
				// OnIdle() handler. Then eliminate the if (gbUpdateDocTitleNeeded) block in OnIdle()
				// and do away with the if gbUpdateDocTitleNeeded global altogether.
				return;
			}
			pDoc->SetFilename(fileFromMRU,TRUE);
			pView->OnInitialUpdate(); // need to call it here because wx's doc/view doesn't automatically call it
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

void CMainFrame::OnAdvancedHtmlHelp(wxCommandEvent& event)
{
	int eID;
	eID = event.GetId();
	//ShowHelp(event.GetId(), *m_pHelpController);
	wxString pathName = gpApp->m_helpInstallPath + gpApp->PathSeparator + gpApp->m_htbHelpFileName;
	bool bOK1;
	m_pHelpController->SetTempDir(_T(".")); // whm added 15Jan09 enables caching of helps for faster startups
	bOK1 = m_pHelpController->AddBook(wxFileName(pathName, wxPATH_UNIX)); // whm added wxPATH_UNIX which is OK on Windows and seems to be needed for Ubuntu Intrepid
	if (!bOK1)
	{
		wxString strMsg;
		strMsg = strMsg.Format(_T("Adapt It could not add book contents to its help file.\nThe name and location of the help file it looked for:\n %s\nTo insure that help is available, this help file must be installed with Adapt It."),pathName.c_str());
		wxMessageBox(strMsg, _T(""), wxICON_WARNING);
	}
	if (bOK1)
	{
		bool bOK2;
		bOK2 = m_pHelpController->DisplayContents();
		if (!bOK2)
		{
			wxString strMsg;
			strMsg = strMsg.Format(_T("Adapt It could not display the contents of its help file.\nThe name and location of the help file it looked for:\n %s\nTo insure that help is available, this help file must be installed with Adapt It."),pathName.c_str());
			wxMessageBox(strMsg, _T(""), wxICON_WARNING);
		}
	}
}

void CMainFrame::OnOnlineHelp(wxCommandEvent& WXUNUSED(event))
{
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
	}
}

void CMainFrame::OnUserForum(wxCommandEvent& WXUNUSED(event))
{
	const wxString userForumURL = _T("http://groups.google.com/group/AdaptIt-Talk");
	int flags;
	flags = wxBROWSER_NEW_WINDOW;
	bool bLaunchOK;
	bLaunchOK = ::wxLaunchDefaultBrowser(userForumURL, flags);
	if (!bLaunchOK)
	{
		wxString strMsg;
		strMsg = strMsg.Format(_T("Adapt It could not launch your browser to access the user forum.\nIf you have Internet access, you can try launching your browser\nyourself and go to the Adapt It user forum by writing down\nthe following Internet address and typing it by hand in your browser:\n %s"),userForumURL.c_str());
		wxMessageBox(strMsg, _T("Error launching browser"), wxICON_WARNING);
	}
}

void CMainFrame::OnUseToolTips(wxCommandEvent& WXUNUSED(event))
{
	wxMenuBar* pMenuBar = this->GetMenuBar();
	wxASSERT(pMenuBar != NULL);
	wxMenuItem * pUseToolTips = pMenuBar->FindItem(ID_HELP_USE_TOOLTIPS);
	wxASSERT(pUseToolTips != NULL);
	if (gpApp->m_bUseToolTips)
	{
		wxToolTip::Enable(FALSE);
		pUseToolTips->Check(FALSE);
		gpApp->m_bUseToolTips = FALSE;
	}
	else
	{
		wxToolTip::Enable(TRUE);
		pUseToolTips->Check(TRUE);
		gpApp->m_bUseToolTips = TRUE;
	}
	
	// save current state of m_bUseToolTips in registry/hidden settings file
	wxString oldPath = gpApp->m_pConfig->GetPath(); // is always absolute path "/Recent_File_List"
	gpApp->m_pConfig->SetPath(_T("/Settings"));
    
	gpApp->m_pConfig->Write(_T("use_tooltips"), gpApp->m_bUseToolTips);
	
	// restore the oldPath back to "/Recent_File_List"
	gpApp->m_pConfig->SetPath(oldPath);
}

// TODO: uncomment EVT_MENU event handler for this function after figure out why SetDelay() disables tooltips
void CMainFrame::OnSetToolTipDelayTime(wxCommandEvent& WXUNUSED(event))
{
	// we can only set the delay time if tooltips are in use
	if (gpApp->m_bUseToolTips)
	{
		// get a new time delay from user
		wxString prompt,caption,timeStr;
		caption = _("Change the amount of time tooltips are displayed");
		prompt = prompt.Format(_("The current tooltip display time is %d milliseconds which is %d seconds.\nEnter a new amount of time in milliseconds:"),gpApp->m_nTooltipDelay,gpApp->m_nTooltipDelay / 1000);
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
	
	// restore the oldPath back to "/Recent_File_List"
	gpApp->m_pConfig->SetPath(oldPath);
}
	
// TODO: uncomment EVT_MENU event handler for this function after figure out why SetDelay() disables tooltips
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
	// Note on SetClientSize() above: When SetClientSize was called here in CreateCanvas, it is
	// ignored in wxMSW, but in wxGTK, it appears to override the setting of the canvas size 
	// by any later call to canvas->SetSize(), resulting in the lower part of the canvas scrolled 
	// window and scrollbar being hidden below the bottom of the main frame. In any case "Client
	// Size" is more a property of our main frame than of our canvas scrolled window. See our own
	// GetCanvasClientSize() function here in MainFrm.cpp.

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
wxSize CMainFrame::GetCanvasClientSize()
{
	wxSize frameClientSize, canvasSize;
	frameClientSize = GetClientSize();
	canvasSize.x = frameClientSize.x; // canvas always fills width of frame's client size
	// the height of the canvas is reduced by the height of the controlBar; and also the
	// height of the composeBar (if visible).
	if (m_pComposeBar->IsShown())
	{
		canvasSize.y = frameClientSize.y - m_controlBarHeight - m_composeBarHeight;
	}
	else
	{
		canvasSize.y = frameClientSize.y - m_controlBarHeight;
	}
	return canvasSize;
}

/////////////////////////////////////////////////////////////////////////////

void CMainFrame::OnViewToolBar(wxCommandEvent& WXUNUSED(event))
{
    wxToolBar *tbar = GetToolBar();

    if ( !tbar )
    {
        RecreateToolBar();
		GetMenuBar()->Check(ID_VIEW_TOOLBAR, TRUE);
		SendSizeEvent();
    }
    else
    {
        delete tbar;
		tbar = (wxToolBar*)NULL;
        SetToolBar(NULL);
		GetMenuBar()->Check(ID_VIEW_TOOLBAR, FALSE);
    }
}

// //////////////////////////////////////////////////////////////////////////////////////////
/// \return		nothing
/// \param      event   -> the wxUpdateUIEvent that is generated when the View Menu is about
///                         to be displayed
/// \remarks
/// Called from: The wxUpdateUIEvent mechanism when the associated menu item is selected, and before
/// the menu is displayed.
/// The "Toolbar" item on the View menu is always enabled by this handler.
// //////////////////////////////////////////////////////////////////////////////////////////
void CMainFrame::OnUpdateViewToolBar(wxUpdateUIEvent& event)
{
	// View Toolbar menu toggle always available
    event.Enable(TRUE);
}

void CMainFrame::OnViewStatusBar(wxCommandEvent& WXUNUSED(event))
{
    wxStatusBar *statbarOld = GetStatusBar();
    if ( statbarOld )
    {
        statbarOld->Hide();
        SetStatusBar(0);
		GetMenuBar()->Check(ID_VIEW_STATUS_BAR, FALSE);
    }
    else
    {
        DoCreateStatusBar();
		GetMenuBar()->Check(ID_VIEW_STATUS_BAR, TRUE);
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
}

// //////////////////////////////////////////////////////////////////////////////////////////
/// \return		nothing
/// \param      event   -> the wxUpdateUIEvent that is generated when the View Menu is about
///                         to be displayed
/// \remarks
/// Called from: The wxUpdateUIEvent mechanism when the associated menu item is selected, and before
/// the menu is displayed.
/// The "Status Bar" item on the View menu is always enabled by this handler.
// //////////////////////////////////////////////////////////////////////////////////////////
void CMainFrame::OnUpdateViewStatusBar(wxUpdateUIEvent& event)
{
	// View StarusBar menu toggle always available
    event.Enable(TRUE);
}

void CMainFrame::OnSize(wxSizeEvent& WXUNUSED(event))
{
    // wx version notes about frame size changes:
    // CMainFrame is Adapt It's primary application frame or window. The CMainFrame is the parent frame
    // for everything that happens in Adapt It, including all first level dialogs. CMainFrame's toolbar
    // and statusbar are created by wxFrame's CreateToolBar() and CreateStatusBar() methods, and so
    // CMainFrame knows how to manage these windows, adjusting the value returned by GetClientSize to
    // reflect the remaining size available to application windows.
    //
    // Adapt It has several other windows to be managed within CMainFrame. These are the m_pControlBar,
    // the m_pComposeBar, m_pRemovalsBar, m_pVertEditBar, and the actual canvas
    // which the user knows as Adapt It's "main window."
    // 
    // These windows are managed here in OnSize, which dynamically adjusts the size of
    // mainFrameClientSize depending on which of these window elements are visible on the main frame at
    // any given time. OnSize insures that they fill their appropriate places within the main frame and
    // automatically resize them when individual elements are made visible or invisible. The canvas
    // always fills the remaining space in the main frame.
    //
    // The wxWidgets' cross-platform library utilizes sizers which could be used to manage the
    // appearance of all the non-standard windows in Adapt It's CMainFrame (including the the
    // m_pControlBar, the m_pComposeBar, m_pRemovalsBar, m_pVertEditBar, and
    // the canvas window which fills all remaining space within the main frame), however, because of the
    // need to explicitly understand what is being done, and potentially different behavior on wxGTK
    // (and possibly wxMAC - see notes below) I've done the specific calculations of these things here
    // in CMainFrame's OnSize(). All of the frame elements are created in the CMainFrame's constructor.
    // The m_pControlBar, the m_pComposeBar, m_pRemovalsBar, and the m_pVertEditBar
    // simply remain hidden until made to appear by menu command or by the vertical process (may also be
    // done custom event handlers).

	// We would do the following if we were going to control the main window's layout with sizers:
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

	// whm Note: The remainder of OnSize() is taken from MFC's View::OnSize() routine
	// BEW added 05Mar06: Bill Martin reported (email 3 March) that if user is in free translation mode,
	// and uses either the Shorten of Lengthen buttons (these set gbSuppressSetup to TRUE in the beginning
	// of their handlers, to suppress resetting up of the section for the change in the layout), and then
	// he resizes the window, the app will crash (invalid pile pointer usually). The easiest solution is
	// just to allow the section to be reset - this loses the effect of the shortening or lengthening, but
	// that can easily be done again by hitting the relevant button(s) after the resized layout is redrawn.
	// Note: gpApp may not be initialized yet in the App's OnInit(), so we'll get a pointer to the app
	// here:
	//CAdapt_ItApp* pApp = (CAdapt_ItApp*)&wxGetApp();
	//if (pApp->m_bFreeTranslationMode)
	//{
	//	gbSuppressSetup = FALSE;
	//}

	// Initiate a recalc and redraw of the canvas' layout with new m_docSize value
	//CAdapt_ItView* pView = (CAdapt_ItView*) pApp->GetView();
	//if (pView)
	//	pView->RedrawEverything(pApp->m_nActiveSequNum);

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

	// First, set the position of the m_pControlBar in the main frame's client area. 
	// Note: the m_pControlBar is never hidden, but always visible on the main frame.
	wxSize mainFrameClientSize;
	mainFrameClientSize = GetClientSize(); // determine the size of the main frame's client window
    // The upper left position of the main frame's client area is always 0,0 so we position the
    // controlBar there and use the main frame's client size width and maintain the existing
    // controlBar's height (note: SetSize sets both the position and the size). 
	// Note: SetSize() uses upper left coordinates in pixels x and y, plus a width and height also in
	// pixels. Its signature is SetSize(int x, int y, int width, int height).
	m_pControlBar->SetSize(0, 0, mainFrameClientSize.x, m_controlBarHeight); // width is mainFrameClientSize.x, height is m_controlBarHeight
	m_pControlBar->Refresh(); // this is needed to repaint the controlBar after OnSize

    // The VertDisplacementFromReportedMainFrameClientSize value is used to keep track of the screen
    // coord y displacement from the original GetClientSize() call above on this main window frame. It
    // represents the how far down inside the main frame's client area we need to go to place any of the
	// potentially visible "bars" in the main window, and ultimately also the placement of the upper
	// left corner of the canvas itself (which fills the remainder of the client area.
	// Since the control bar is always visible, we start with the displacement represented by the 
	// m_controlBarHeight assigned to VertDisplacementFromReportedMainFrameClientSize.
	int VertDisplacementFromReportedMainFrameClientSize = m_controlBarHeight;
	// The FinalHeightOfCanvas that we end up placing starts with the available height of the
	// mainFrameClientSize as determined by the GetClientSize() call above, now reduced by the height
	// of our always visible controlBar.
	int finalHeightOfCanvas = mainFrameClientSize.y - m_controlBarHeight; 

	// Next, set the size and placement for each of the visible "bars" that appear at the top of the
	// client area (under the controlBar. We increment the value of
	// VertDisplacementFromReportedMainFrameClientSize for each one that is visible (so we'll know
	// where to size/place the canvas after all other elements are sized/placed.
	
	// Adjust the composeBar's position in the main frame (if composeBar is visible).
	if (m_pComposeBar->IsShown())
	{
        // The composeBar is showing, so set its position (with existing size) just below the controlBar.
        // The upper left y coord now is represented by VertDisplacementFromReportedMainFrameClientSize.
		m_pComposeBar->SetSize(0, VertDisplacementFromReportedMainFrameClientSize, mainFrameClientSize.x, m_composeBarHeight);
		m_pComposeBar->Refresh();

		// Increment VertDisplacementFromReportedMainFrameClientSize for the next placement
		VertDisplacementFromReportedMainFrameClientSize += m_composeBarHeight;
		finalHeightOfCanvas -= m_composeBarHeight;
	}

    // Next, adjust the removalsBar's position in the main frame (if removalsBar is visible), and set the
    // canvas' position and size to accommodate it.
	if (m_pRemovalsBar->IsShown())
	{
		// The removalsBar is showing, so set its position (with existing size) just below the last
		// placed element. 
        // The upper left y coord now is represented by VertDisplacementFromReportedMainFrameClientSize.
		m_pRemovalsBar->SetSize(0, VertDisplacementFromReportedMainFrameClientSize, mainFrameClientSize.x, m_removalsBarHeight);
		pRemovalsBarSizer->Layout(); //m_pRemovalsBar->Refresh();

		// Increment VertDisplacementFromReportedMainFrameClientSize for the next placement
		VertDisplacementFromReportedMainFrameClientSize += m_removalsBarHeight;
		finalHeightOfCanvas -= m_removalsBarHeight;
	}

     // Next, adjust the verteditBar's position in the main frame (if verteditBar is visible), and set the
    // canvas' position and size to accommodate it.
	if (m_pVertEditBar->IsShown())
	{
		// The verteditBar is showing, so set its position (with existing size) just below the last
		// placed element. 
        // The upper left y coord now is represented by VertDisplacementFromReportedMainFrameClientSize.
		m_pVertEditBar->SetSize(0, VertDisplacementFromReportedMainFrameClientSize, mainFrameClientSize.x, m_vertEditBarHeight);
		pVertEditBarSizer->Layout(); //m_pVertEditBar->Refresh();

		// Increment VertDisplacementFromReportedMainFrameClientSize for the next placement
		VertDisplacementFromReportedMainFrameClientSize += m_vertEditBarHeight;
		finalHeightOfCanvas -= m_vertEditBarHeight;
	}

     // Next, adjust the verteditsteptransBar's position in the main frame (if verteditsteptransBar is visible), and set the
    // canvas' position and size to accommodate it.
	//if (m_pVertEditStepTransBar->IsShown())
	//{
	//	// The verteditStepTransBar is showing, so set its position (with existing size) just below the last
	//	// placed element. 
 //       // The upper left y coord now is represented by VertDisplacementFromReportedMainFrameClientSize.
	//	m_pVertEditStepTransBar->SetSize(0, VertDisplacementFromReportedMainFrameClientSize, mainFrameClientSize.x, m_vertEditStepTransBarHeight);
	//	m_pVertEditStepTransBar->Refresh();

	//	// Increment VertDisplacementFromReportedMainFrameClientSize for the next placement
	//	VertDisplacementFromReportedMainFrameClientSize += m_vertEditStepTransBarHeight;
	//	finalHeightOfCanvas -= m_vertEditStepTransBarHeight;
	//}

   // Finally, set the canvas size to take up the remaining space in the main window (the value of 
	// VertDisplacementFromReportedMainFrameClientSize now represents the y coordinate displacement
	// from the original GetClientSize() call and is where we place the canvas. The width is always
	// mainFrameClientSize.x, and the height is our calculated finalHeightOfCanvas.
	//canvas->SetSize(0, VertDisplacementFromReportedMainFrameClientSize, // top left x and y coords of canvas within client area
	//	mainFrameClientSize.x, finalHeightOfCanvas); // width and height of canvas
	
#ifdef _USE_SPLITTER_WINDOW
	// whm Note: If the splitter window IsSplit, we have two versions of the canvas, one in the top
	// splitter window and one in the bottom splitter window. To size things properly in this case, we
	// check to see if splitter->IsSplit() returns TRUE or FALSE. If FALSE we set the size of the
	// single window to be the size of the remaining client size in the frame. If TRUE, we set Window1
	// and Window2 separately depending on their split sizes.
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
		splitter->SetSize(0, VertDisplacementFromReportedMainFrameClientSize, // top left x and y coords of canvas within client area
			mainFrameClientSize.x, finalHeightOfCanvas); // width and height of canvas
	//}
#else
		canvas->SetSize(0, VertDisplacementFromReportedMainFrameClientSize, // top left x and y coords of canvas within client area
			mainFrameClientSize.x, finalHeightOfCanvas); // width and height of canvas
#endif
	
	// whm Note: The remainder of OnSize() is taken from MFC's View::OnSize() routine
	// BEW added 05Mar06: Bill Martin reported (email 3 March) that if user is in free translation mode,
	// and uses either the Shorten of Lengthen buttons (these set gbSuppressSetup to TRUE in the beginning
	// of their handlers, to suppress resetting up of the section for the change in the layout), and then
	// he resizes the window, the app will crash (invalid pile pointer usually). The easiest solution is
	// just to allow the section to be reset - this loses the effect of the shortening or lengthening, but
	// that can easily be done again by hitting the relevant button(s) after the resized layout is redrawn.
	// Note: gpApp may not be initialized yet in the App's OnInit(), so we'll get a pointer to the app
	// here:
	CAdapt_ItApp* pApp = (CAdapt_ItApp*)&wxGetApp();
	if (pApp->m_bFreeTranslationMode)
	{
		gbSuppressSetup = FALSE;
	}

	// need to initiate a recalc of the layout with new m_docSize value, since strip-wrap is on
	CAdapt_ItView* pView = (CAdapt_ItView*) pApp->GetView();
	if (pView)
		pView->RedrawEverything(pApp->m_nActiveSequNum);

	// code below was in docview sample
    // FIXME: On wxX11, we need the MDI frame to process this
    // event, but on other platforms this should not
    // be done.
#ifdef __WXUNIVERSAL__   
    event.Skip();
#endif

}

void CMainFrame::RecreateToolBar()
{
    // delete and recreate the toolbar
    wxToolBar *toolBar = GetToolBar();
    delete toolBar;
	toolBar = (wxToolBar*)NULL;
    SetToolBar(NULL);

    long style = /*wxNO_BORDER |*/ wxTB_FLAT | wxTB_HORIZONTAL;

	// CreateToolBar is a method of the Frame class and informs the doc/view of the existence of
	// the toolbar so the client window allows space for it. 
	//toolBar = CreateToolBar(style, -1);
	// The following line can be used also to create the toolbar, but SetToolBar(toolBar) must be
	// then called to register it with the doc/view framework
	toolBar = new wxToolBar(this, -1, wxDefaultPosition, wxDefaultSize, style);
	wxASSERT(toolBar != NULL);

	m_pToolBar = toolBar;

	// AIToolBarFunc is located in the Adapt_It_Resources.cpp file and handles the construction
	// and initial state of all the toolBar items on the main frame
	AIToolBarFunc( toolBar );

	// Tidy up the toolbar by removing the three toggle-on state buttons (physically located
	// at the end of the AIToolBarFunc array). Removing them does not delete them, but allows 
	// us to insert them in the correct button slot after deleting the untoggled button or vice
	// versa depending on whether the toggle is on or off. The button swapping is done in event
	// handlers in the View.
	// Note: Just calling RemoveTool on a button removes it from the toolbar, but when program 
	// exits, the IDE will report "Detected memory leaks!" In tbarbase.h where RemoveTool() is 
	// declared it says, "RemoveTool: remove the tool from the toolbar: the caller is 
	// responsible for actually deleting the pointer". So, how do we get the pointer? 
	// RemoveTool returns a pointer to wxToolBarToolBase. So it should be sufficient to 
	// do the following:
	// NOTE: In the end I decided to dynamically insert and remove the toggle buttons in the View
	//wxToolBarToolBase *remToolptr;
	//remToolptr = toolBar->RemoveTool(ID_BUTTON_IGNORING_BDRY);
	//delete remToolptr;
	//remToolptr = toolBar->RemoveTool(ID_BUTTON_HIDING_PUNCT);
	//delete remToolptr;
	//remToolptr = toolBar->RemoveTool(ID_SHOW_ALL);
	//delete remToolptr;

	// If an application does not use CreateToolBar() to create the application toolbar
	// it must call SetToolBar() to register the toolBar with the doc/view framework so
	// that it will manage it properly
	SetToolBar(toolBar); // called in CreateToolBar() above

	// Deleting the pointers above do eliminate the "Detected memory leaks" message when
	// RemoveTool() is used.
	// Note: InsertTool, although it also returns a pointer to wxToolBarToolBase, 
	// InsertTool does not produce any memory leaks reported by the IDE on exiting the app.
	//toolBar->Realize(); // called in AIToolBarFunc() above - done by wxDesigner
}

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

// //////////////////////////////////////////////////////////////////////////////////////////
/// \return		nothing
/// \param      event   -> the wxUpdateUIEvent that is generated by the update idle mechanism
/// \remarks
/// Called from: The wxUpdateUIEvent mechanism during idle time processing. 
/// This update handler insures that the "Automatic" checkbox is checked when the App's 
/// m_bSingleStep flag is FALSE, and unchecked when the flag is TRUE.
// //////////////////////////////////////////////////////////////////////////////////////////
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

// //////////////////////////////////////////////////////////////////////////////////////////
/// \return		nothing
/// \param      event   -> the wxUpdateUIEvent that is generated by the update idle mechanism
/// \remarks
/// Called from: The wxUpdateUIEvent mechanism during idle time processing. 
/// This update handler insures that the "Save To Knowledge Base" checkbox is checked when the 
/// App's m_bSaveToKB flag is TRUE, and unchecked when the flag is FALSE.
// //////////////////////////////////////////////////////////////////////////////////////////
void CMainFrame::OnUpdateCheckKBSave(wxUpdateUIEvent& event)
{
	// the flags we want are on the view, so get the view
	CAdapt_ItApp* pApp = &wxGetApp();
	wxASSERT(pApp != NULL);
	CAdapt_ItView* pView = (CAdapt_ItView*) pApp->GetView();
	if (pView != NULL)
	{
		wxASSERT(pView->IsKindOf(CLASSINFO(CAdapt_ItView)));
		event.Check(pApp->m_bSaveToKB);
	}
}

// //////////////////////////////////////////////////////////////////////////////////////////
/// \return		nothing
/// \param      event   -> the wxUpdateUIEvent that is generated by the update idle mechanism
/// \remarks
/// Called from: The wxUpdateUIEvent mechanism during idle time processing. 
/// This update handler insures that the "Force Choice For This Item" checkbox is checked when the 
/// App's m_bForceAsk flag is TRUE, and unchecked when the flag is FALSE.
// //////////////////////////////////////////////////////////////////////////////////////////
void CMainFrame::OnUpdateCheckForceAsk(wxUpdateUIEvent& event)
{
	// the flags we want are on the view, so get the view
	CAdapt_ItApp* pApp = &wxGetApp();
	wxASSERT(pApp != NULL);
	CAdapt_ItView* pView = (CAdapt_ItView*) pApp->GetView();
	if (pView != NULL)
	{
		wxASSERT(pView->IsKindOf(CLASSINFO(CAdapt_ItView)));
		event.Check(pApp->m_bForceAsk);
	}
}

void CMainFrame::OnViewComposeBar(wxCommandEvent& WXUNUSED(event))
{
	gpApp->m_bComposeBarWasAskedForFromViewMenu = TRUE;
	ComposeBarGuts();
}

void CMainFrame::ComposeBarGuts()
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

	// depending on which command invoked this code, hide some buttons and show others -- there are
	// two buttons shown when invoked from the View menu, and six different buttons shown when
	// invoked from the Advanced menu in order to turn on or off free translation mode
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
		if (m_pComposeBar->IsShown())
		{
			// Hide it
			m_pComposeBar->Hide();
			GetMenuBar()->Check(ID_VIEW_COMPOSE_BAR, FALSE);
			gpApp->m_bComposeWndVisible = FALSE; // needed???
			gpApp->m_bComposeBarWasAskedForFromViewMenu = FALSE; // needed for free translation mode
			SendSizeEvent(); // needed to force redraw
		}
		else
		{
			// Show the composeBar
			m_pComposeBar->Show(TRUE);
			GetMenuBar()->Check(ID_VIEW_COMPOSE_BAR, TRUE);
			gpApp->m_bComposeWndVisible = TRUE; // needed???
			SendSizeEvent(); // needed to force redraw
		}
		//RecalcLayout(); // This RecalcLayout() is not the same one as in the View. In MFC this 
						// is a CFrameWnd member that "Repositions the control bars of the CFrameWnd object"
						// WX should be able to just call the compose bar's sizer's Layout() method.
		m_pComposeBar->GetSizer()->Layout(); // make compose bar resize for different buttons being shown

		// when the compose bar is turned off, whether by the View menu command or the Advanced
		// menu command to turn off free translation mode, we must clear m_bComposeBarWasAskedForFromViewMenu
		// to FALSE; and we don't need to do anything with the fonts when we have closed the window
		if (!gpApp->m_bComposeWndVisible) //if (!pAppView->m_bComposeWndVisible)
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
			// the bar is visible, so set the font - normally m_pComposeFont will preserve the setting, which
			// by default will be based on m_pTargetFont, but when glossing it could be the navText's font instead
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
				gnStart = 0;
				gnEnd = -1;
				pEdit->SetSelection(gnStart,gnEnd); // no scroll
			}
			else
			{
				// BEW added 18Oct06, since focus doesn't get automatically put into compose bar's
				// edit box when the bar was first opened...
				// when not Free Translation mode, set the focus to the edit box and show all selected
				pEdit->SetFocus();
				pEdit->SetSelection(-1,-1); // -1,-1 selects all
			}
		}
	}
}

// //////////////////////////////////////////////////////////////////////////////////////////
/// \return		nothing
/// \param      event   -> the wxUpdateUIEvent that is generated when the View Menu is about
///                         to be displayed
/// \remarks
/// Called from: The wxUpdateUIEvent mechanism when the associated menu item is selected, and before
/// the menu is displayed.
/// This handler insures that the "Compose Bar" item on the View menu is enabled and checked when 
/// the App's m_bComposeWndVisible flag is TRUE, and unchecked when m_bComposeWndVisible is FALSE.
/// The "Compose Bar" menu item will be disabled if the application is in free translation mode.
// //////////////////////////////////////////////////////////////////////////////////////////
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


void CMainFrame::OnEditConsistencyCheck(wxCommandEvent& WXUNUSED(event)) 
{
	CAdapt_ItApp* pApp = &wxGetApp();
	wxASSERT(pApp != NULL);
	CAdapt_ItDoc* pDoc = pApp->GetDocument(); // get pointer to the document associated with the view

	if (pDoc == NULL)
		return; // we are not ready yet
	if (pApp->m_pSourcePhrases->GetCount() > 0)
	{
		// proceed provided there is data in the document
		;
	}
	else
		return;	// no data, so don't do anything
}

// //////////////////////////////////////////////////////////////////////////////////////////
/// \return		nothing
/// \param      event   -> the wxUpdateUIEvent that is generated when the Edit Menu is about
///                         to be displayed
/// \remarks
/// Called from: The wxUpdateUIEvent mechanism when the associated menu item is selected, and before
/// the menu is displayed.
/// This handler enables the "Consistency Check..." item on the Edit menu as long as there is a
/// valid document and view and the list of source phrases in the App's m_pSourcePhrases is not
/// empty.
// //////////////////////////////////////////////////////////////////////////////////////////
void CMainFrame::OnUpdateEditConsistencyCheck(wxUpdateUIEvent& event) 
{
	if (gbVerticalEditInProgress)
	{
		event.Enable(FALSE);
		return;
	}
	CAdapt_ItApp* pApp = &wxGetApp();
	wxASSERT(pApp != NULL);
	CAdapt_ItDoc* pDoc = pApp->GetDocument(); // get pointer to the document associated with the view
	if (pDoc == NULL)
		return; // we are not ready yet

	bool bFlag = pApp->m_pSourcePhrases->GetCount() > 0;
	CAdapt_ItView* pView = (CAdapt_ItView*) pApp->GetView();
	if (pView != NULL)
	{
		event.Enable(bFlag);
	}
}

//void CMainFrame::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized) // MFC version
// NOTE: wxFrame::OnActivate() is MS Windows only. It is not a virtual function
// under wxWidgets, and takes a single wxActivateEvent& event parameter.
void CMainFrame::OnActivate(wxActivateEvent& event) 
{
	// NOTE: Setting a breakpoint in this type of function will be problematic
	// because the breakpoint will itself trigger the OnActivate() event!
	CAdapt_ItApp* pApp = &wxGetApp();
	wxASSERT(pApp != NULL);

	// m_pTargetBox is now on the App, and under wxWidgets the main frame's
	// OnActivate() method may be called after the view has been destroyed so
	// I have commented out reference to the view below
	//CAdapt_ItView* pView = (CAdapt_ItView*) pApp->GetView();// GetFirstView();
	//if (pView == NULL) return; // no view yet; nothing to set focus on
	if (event.GetActive())
	{
		// if free translation mode is active, put the focus in the compose bar's CEdit
		// added 12May06 by request of Jim Henderson so return by ALT+TAB from another window
		// restores the focus to the edit box for free translation typing
		if (pApp->m_bFreeTranslationMode )
		{
			CMainFrame* pFrame = pApp->GetMainFrame(); //wxPanel* cPanel = pApp->GetMainFrame()->m_pComposeBar;
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

	// MFC code below
	//CFrameWnd::OnActivate(nState, pWndOther, bMinimized);
	
}

// OnIdle moved here from the App. When it was in the App it was causing
// the File | Exit and App x cancel commands to not be responsive there
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

	// wx version: Display some scrolling data on the statusbar. m_bShowScrollData is
	// only true when __WXDEBUG__ is defined, so it will not appear in release versions.
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
	//switch (lCount)
	//{
	//case 2: // check if user or application has turned off auto inserting

		if (pApp->m_bSingleStep)
		{
			pApp->m_bAutoInsert = FALSE;
			//event.RequestMore(); // whm removed 2Jan09 TODO: do we need a call to wxWakeUpIdle elsewhere?
		}
		//return TRUE; // enable next OnIdle call - this accomplished with event.RequestMore()
	//case 3: // auto capitalization support for the typed string in the phrasebox - we
			// poll the box and change the case if necessary

		if (pApp->m_pActivePile == NULL || pApp->m_nActiveSequNum == -1)
		{
			//event.RequestMore(); // whm removed 2Jan09 TODO: do we need a call to wxWakeUpIdle elsewhere?
			//return TRUE; // we are at the end and no phrase box exists, so return
		}
		if (gbAutoCaps && pApp->m_pActivePile != NULL) // whm added && pApp->m_pActivePile != NULL
		{
			wxString str;
			CSourcePhrase* pSrcPhrase = pApp->m_pActivePile->m_pSrcPhrase;
			wxASSERT(pSrcPhrase != NULL);
			bool bNoError = pView->SetCaseParameters(pSrcPhrase->m_key);
			if (bNoError && gbSourceIsUpperCase)
			{
				// a change of case might be called for... first
				// make sure the box exists and is visible before proceeding further
				if (pApp->m_pTargetBox != NULL && 
										(pApp->m_pTargetBox->IsShown()))
				{
					// get the string currently in the phrasebox
					str = pApp->m_pTargetBox->GetValue(); //pView->m_targetBox.GetWindowText(str);

					// do the case adjustment only after the first character has been
					// typed, and be sure to replace the cursor afterwards
					int len = str.Length();
					if (len != 1)
					{
						//return TRUE;
						//event.RequestMore(); // whm removed 2Jan09 testing shows auto caps works OK
						//without this.
					}
					else
					{
						// set cursor offsets
						int nStart = 1; int nEnd = 1;
				
						// check out its case status
						bNoError = pView->SetCaseParameters(str,FALSE);

						// change to upper case if required
						if (bNoError && !gbNonSourceIsUpperCase && (gcharNonSrcUC != _T('\0')))
						{
							str.SetChar(0,gcharNonSrcUC);
							pApp->m_pTargetBox->SetValue(str); //pView->m_targetBox.SetWindowText(str);
							pApp->m_pTargetBox->Refresh(); //;pView->m_targetBox.Invalidate();
							pApp->m_targetPhrase = str;

							// fix the cursor location
							pApp->m_pTargetBox->SetSelection(nStart,nEnd); //pView->m_targetBox.SetSel(nStart,nEnd);
						}
					}
				}
			}
		}
		//return TRUE;
		//event.RequestMore(); // whm removed 2Jan09 TODO: do we need a call to wxWakeUpIdle elsewhere?
	//case 4: // autosaving

		// wx version whm added & pApp->m_pKB != NULL to if clause below because DoAutoSaveKB()
		// was being called in idle time when m_pKB was null while sitting for over 10 minutes 
		// in the wizard.
		if (pDoc != NULL && pApp != NULL && !pApp->m_bNoAutoSave && pApp->m_pKB != NULL)
		{
			wxDateTime time = wxDateTime::Now(); //CTime time = CTime::GetCurrentTime();
			wxTimeSpan span = time - pApp->m_timeSettings.m_tLastDocSave;

			if (pApp->m_bIsDocTimeButton)
			{
				if (span > pApp->m_timeSettings.m_tsDoc)
				{
					// we need to do a save, provided the document is dirty
					if (pDoc->IsModified())
						pApp->DoAutoSaveDoc();

					//return TRUE; // don't attempt a KB save
					//event.RequestMore(); // whm removed 2Jan09 testing shows autosaving works OK
					//without this.
				}
			}
			else
			{
				// we are counting phrase box moves for doing autosaves
				if (nCurrentSequNum > nSequNumForLastAutoSave + pApp->m_nMoves)
				{
					if(pDoc->IsModified())
						pApp->DoAutoSaveDoc();
					//return TRUE; // don't attempt a KB save
					//event.RequestMore(); // whm removed 2Jan09 testing shows autosaving works OK
					//without this.
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
		//return TRUE; // enable next OnIdle call
		//event.RequestMore(); // whm removed 2Jan09

	//case 6: // do the Replace All button handler
		if (gbReplaceAllIsCurrent && pView != NULL && pApp->m_pReplaceDlg != NULL )
		{
			// because of an obscure bug not in my code which turns on the gbReplaceAllIsCurrent
			// flag for no apparent reason (in rare circumstances, between end of CFindReplace's
			// OnFindNext() and the next invocation of OnFindNext()), when user has a Find going,
			// we have to check here for the flag being wrongly set and clear it. 
			// whm note: the "Find" and "Find and Replace" dialogs are separate dialogs
			// in the wx version, so I don't think this issue will be relevant.
			//if (pApp->m_pFindReplaceDlg->m_bFindDlg)
			//{
			//	// its a Find dialog, so this flag should NOT be set, so clear it
			//	gbReplaceAllIsCurrent = FALSE;
			//	//return TRUE;
			//	event.RequestMore(); // added
			//}
			
			bool bSucceeded = pApp->m_pReplaceDlg->OnePassReplace();
			if (bSucceeded)
			{
				//return TRUE; // enable next iteration
				event.RequestMore(); // added
			}
			else
			{
				// we have come to the end
				pApp->m_pReplaceDlg->Show(TRUE);
				gbReplaceAllIsCurrent = FALSE;
				//return TRUE;
				//event.RequestMore(); // whm removed 2Jan09 testing shows no need for RequestMore() here 
				//at end of replace all.
			}
		}
		//else
		//{
		//	//return TRUE; // enable more idle messages
		//	//event.RequestMore(); // whm removed 2Jan09 TODO: do we need a call to wxWakeUpIdle in CReplaceDlg?
		//}

	//case 7: // get the startup wizard up & running
		if (pApp->m_bJustLaunched)
		{
			pApp->m_bJustLaunched = FALSE; // moved up before DoStartupWizardOnLaunch()
			pView->DoStartupWizardOnLaunch();
			//pApp->m_bJustLaunched = FALSE;

			// next block is an brute force kludge to get around the fact that if we
			// have used the wizard to get a file on the screen, the phrase box won't
			// have the text selected when user first sees the doc, so this way I can get 
			// it selected as if the File... New... route was taken, rather than the wiz.
			if (pApp->m_bStartViaWizard && pApp->m_pTargetBox != NULL)
			{
				pApp->m_pTargetBox->SetFocus();
				pApp->m_nEndChar = -1;
				pApp->m_nStartChar = 0;
				pApp->m_pTargetBox->SetSelection(pApp->m_nStartChar,pApp->m_nEndChar);
				pApp->m_bStartViaWizard = FALSE; // suppress this code from now on
				gnStart = 0;
				gnEnd = -1;
			}

			// return FALSE;
			//if (m_bNoAutoSave)
			//{
			//	//return FALSE;  // we can disable further OnIdle calls now, since autosave is OFF
			//}
			//else
			//{
			//	//return TRUE; // leave it on, so that time processing will continue for autosave
			//	event.RequestMore(); // added
			//}
		}
		//else
		//{
		//event.RequestMore(TRUE);
		event.Skip(); // let std processing take place too
		//}
	//case 8: // set or clear the m_bNotesExist flag on the app (enables the update handlers for the
	//		// command bar buttons to disable the next, prev, and delete all buttons for the notes
	//		// mechanism depending on whether there are any notes still in the doc or not (we can
	//		// live without this, but it is nice to have the buttons disabled when there are no notes,
	//		// and we don't want to force the long check to happen for every toolbar update)
	//		// BEW added 12Sep05
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

		// also, do a doc window title update if it has been asked for by a MRU doc open failure
		if (gbUpdateDocTitleNeeded)
		{
			wxString fname = pApp->m_curOutputFilename;
			wxString extensionlessName;
			pDoc->SetDocumentWindowTitle(fname, extensionlessName);
			gbUpdateDocTitleNeeded = FALSE; // turn it off until the next MRU doc open failure
		}
	//	return TRUE; // enable next OnIdle call
	//case 9:
		if (gbCameToEnd)
		{
			// IDS_AT_END
			wxMessageBox(_("The end. Provided you have not missed anything earlier, there is nothing more to adapt in this file."), 
				_T(""), wxICON_INFORMATION);
			gbCameToEnd = FALSE;
		}
	//	return TRUE; // enable  next OnIdle call
	//case 5:
	//default: // autoinsert, if possible
		if (bUserCancelled)
			bUserCancelled = FALSE; // ensure its turned back off
		// WX Note: There is no ::IsWindow() equivalent in wxWidgets and the phrasebox is not
		// normally ever NULL; we need another flag to prevent autoinserts when the ChooseTranslationDlg
		// is already showing
		//if (pBox == NULL || !::IsWindow((HWND)pBox->GetHandle()))
		//if (pBox == NULL)
		//	return TRUE; // don't do anything here till we have a valid phrase box window

		if (pApp->m_bAutoInsert)
		{
			if (pApp->m_nCurDelay > 0)
			{
				DoDelay(); // defined in Helpers.cpp (m_nCurDelay is in tick units)
			}
			bool bSuccessfulInsertAndMove = pBox->OnePass(pView);
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
				//return TRUE;
				//event.RequestMore(); // whm removed 2Jan09 TODO: do we need a call to wxWakeUpIdle elsewhere?
			}
		}
		else
		{
			// do nothing, just enable another OnIdle call (this is the one which keeps
			// the CPU running at 100%, but if I turn off idle events by returning false
			// here, then the autocapitalization mechanism doesn't work, nor background
			// highlighting in all circumstances; the solution seems to be to allow a large
			// number of idle events, and then turn them off.)
			//if (lCount < 1000)
			//	return TRUE;
			//else
			//	return FALSE;
			//if (idleCount % 1000 == 0)
			//{
				//event.RequestMore(); // whm removed 2Jan09 TODO: do we need a call to wxWakeUpIdle elsewhere?
			//	idleCount = 0;
			//}
		}
	//}
	//event.Skip();
}

// whm Note: this and following custom event handlers are in the View in the MFC version
// 
// The following is the handler for a wxEVT_Adaptations_Edit event message, defined in the
// event table macro EVT_ADAPTATIONS_EDIT.
// The wxEVT_Adaptations_Edit event is sent to the window event loop by a 
// wxPostEvent() call in OnEditSourceText().
void CMainFrame::OnCustomEventAdaptationsEdit(wxCommandEvent& WXUNUSED(event))
{
	// adaptations updating is required
	// Insure that we have a valid pointer to the m_pVertEditBar member
	// of the frame window class
	wxASSERT(m_pVertEditBar != NULL);
	wxASSERT(m_pRemovalsBar != NULL);

	CAdapt_ItApp* pApp = &wxGetApp();
	wxASSERT(pApp != NULL);
	CAdapt_ItView* pView = pApp->GetView();
	wxASSERT(pView != NULL);

	if (m_pVertEditBar == NULL)
	{
		wxMessageBox(_T("Failure to obtain pointer to the vertical edit control bar in \
						 OnCustomEventAdaptationsEdit()"),_T(""), wxICON_EXCLAMATION);
		return;
	}

	// determine what setup is required: control is coming from either sourceTextStep or
	// glossesStep, when the order is adaptions before glosses (see gbAdaptBeforeGloss flag),
	// but if the order is glosses before adaptations, then control is coming from either
	// glossesStep or freeTranslationsStep. These variations require separate initializations
	// blocks.
	//CAdapt_ItDoc* pDoc = gpApp->GetDocument(); // unused
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
						//OnCheckIsGlossing(); // turns OFF the Glossing checkbox and clears 
											 // gbIsGlossing to FALSE; and leaves gbEnableGlossing
											 // with its TRUE value unchanged; populates list with
											 // removed adapatations
						if (!pRec->bEditSpanHasGlosses)
						{
							// the edit span lacked glosses, so assume they are not relevant or not
							// present in the surrounding context
							pView->ToggleSeeGlossesMode();
							//OnAdvancedEnableglossing(); // toggles gbEnableGlossing to FALSE, makes
														// the Glossing checkbox be hidden
														// leaves gbIsGlossing FALSE
						}
					}
					else
					{
						// the view is in adapting mode; we want to make glosses visible though
						// if the edit span has some, but not if it has none
						if (pRec->bEditSpanHasGlosses)
						{
							// make them visible if not already so
							if (!gbEnableGlossing)
								pView->ToggleSeeGlossesMode();
								//OnAdvancedEnableglossing(); // toggles gbEnableGlossing to TRUE,
														// makes the Glossing checkbox be unhidden
														// leaves gbIsGlossing FALSE
						}
						else
						{
							// make sure glosses are not visible
							if (gbEnableGlossing)
							{
								// make them invisible, as there are none in the edit span
								pView->ToggleSeeGlossesMode();
								//OnAdvancedEnableglossing(); // clears gbEnableGlossing to FALSE,
															// makes the Glossing checkbox be hidden,
															// leaves gbIsGlossing FALSE
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

						// put the adaptations step's message in the multi-line read-only edit box
						// IDS_VERT_EDIT_ADAPTATIONS_MSG
						pView->SetVerticalEditModeMessage(_("Vertical Editing - adaptations step: Type the needed adaptations in the editable region. Earlier adaptations are stored at the top of the Removed list. Gray text is not accessible. Adapting mode is currently on and all adaptation functionalities are enabled, including mergers, placeholder insertion and retranslations."));

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
						//this->PostMessage(CUSTOM_EVENT_GLOSSES_EDIT,0,0);
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

					// we know we are coming from glossing mode being currently on, so we have to switch to
					// adapting mode; moreover, we expect glosses were made or updated, and the user will want
					// to see them, so we make them visible (more info might help the user)
					pView->ToggleGlossingMode();

					// If the user removed all the CSourcePhrase instances of the editable span,
					// there are no adaptations to be updated, because the span has no extent. We
					// will try to externally catch and prevent a user rollback to adaptations mode when
					// there is no extent, but just in case we don't, just bring the vertical edit to
					// an end immediately -- this block should never be entered if I code correctly
					if (pRec->nNewSpanCount == 0)
					{
						//this->PostMessage(CUSTOM_EVENT_END_VERTICAL_EDIT,0,0);
						wxCommandEvent eventCustom(wxEVT_End_Vertical_Edit);
						wxPostEvent(this, eventCustom);
						// whm Note: This event calls OnCustomEventCancelVerticalEdit() which ends by hiding any of
						// the vertical edit tool bars from the main window.
						gEditStep = adaptationsStep;
						pRec->bAdaptationStepEntered = TRUE;
						return;
					}

					// the span was entered earlier, and we are rolling back, so we have to set up
					// the state as it was at the start of the of the previous adaptationsStep;
					// and we have to restore the phrase box to the start of the editable span too
					// (we don't bother to make any reference count decrements in the KB, so this
					// would be a source of ref counts being larger than strictly correct, but that
					// is a totally benign effect)

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

					// put the adaptations step's message in the multi-line read-only CEdit box
					// IDS_VERT_EDIT_ADAPTATIONS_MSG
					pView->SetVerticalEditModeMessage(_("Vertical Editing - adaptations step: Type the needed adaptations in the editable region. Earlier adaptations are stored at the top of the Removed list. Gray text is not accessible. Adapting mode is currently on and all adaptation functionalities are enabled, including mergers, placeholder insertion and retranslations."));

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

					// initiate a redraw of the frame and the client area (Note, this is MFC's
					// CFrameWnd member function called RecalcLayout(), not my CAdapt_ItView
					// member function of same name
					//pFWnd->RecalcLayout(); // the bNotify parameter is default TRUE, let it do so,
										   // no harm if the window is not embedded
					SendSizeEvent(); // forces the CMainFrame::SetSize() handler to run and do the needed redraw

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
					// the PostMessage() which we are handling is fired from the end of the
					// glosses step, so control is still in that step, and about to
					// transition to the adaptations step if the editable span had some, if not,
					// we transition from adaptations step, after minimal setup, to the
					// free translations step

					// since we were in glossingStep we know that the glossing checkbox is ON
					// and the gbEnableGlossing global is TRUE; so all we need to do is turn
					// the checkbox off
					pView->ToggleGlossingMode();
					//OnCheckIsGlossing(); // turns OFF the Glossing checkbox and clears 
										 // gbIsGlossing to FALSE; and leaves gbEnableGlossing
										 // with its TRUE value unchanged; populates list with
										 // removed adapatations

					// if the user removed all the CSourcePhrase instances of the editable span,
					// there are no adaptations to be updated, so check for this and if so, fill
					// in the relevant gEditRecord members and then post the event for the next
					// step -- we don't give the user a dialog to vary the step order here, because
					// he won't have seen any feedback up to now that he's about to be in adaptations
					// step, so we unilaterally send him on to the next step
					// Note: if either or both of the ...SpanCount variables are 0, it means the user
					// deleted the material in the edit span, and populating the list is impossible too
					if (pRec->nNewSpanCount == 0)
					{
						// user deleted everything... so move on
						pRec->nAdaptationStep_OldSpanCount = 0;
						pRec->nAdaptationStep_NewSpanCount = 0;
						pRec->nAdaptationStep_StartingSequNum = -1;
						pRec->nAdaptationStep_EndingSequNum = -1;

						gEditStep = adaptationsStep; // we've been in this step, even though we did nothing
						pRec->bAdaptationStepEntered = TRUE; // prevent reinitialization if user returns here

						// post event for progressing to gloss mode (we let its handler make the
						// same test for nNewSpanCount being 0 and likewise cause progression
						// immediately rather than make that next progression happen from here)
						// Note, we have not changed from adaptationsStep, hence the handler for
						// this message will become active with gEditStep still set to 
						// adaptationsStep - this is how we inform the next step what the previous
						// step was, and so the handler for the next step will finish off this
						// current step before setting up for the commencement of the glossesStep
						//this->PostMessage(CUSTOM_EVENT_FREE_TRANSLATIONS_EDIT,0,0);
						wxCommandEvent eventCustom(wxEVT_Free_Translations_Edit);
						wxPostEvent(this, eventCustom);
						return;
					}

					// if this is our first time in this step...
					if (!pRec->bAdaptationStepEntered)
					{
						// put the values in the edit record, where the CCell drawing function can get them,
						// we get them from the glossesStep parameters, since we have just come from there
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

						// put the adaptations step's message in the multi-line read-only CEdit box
						// IDS_VERT_EDIT_ADAPTATIONS_MSG
						pView->SetVerticalEditModeMessage(_("Vertical Editing - adaptations step: Type the needed adaptations in the editable region. Earlier adaptations are stored at the top of the Removed list. Gray text is not accessible. Adapting mode is currently on and all adaptation functionalities are enabled, including mergers, placeholder insertion and retranslations."));

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

						// initiate a redraw of the frame and the client area (Note, this is MFC's
						// CFrameWnd member function called RecalcLayout(), not my CAdapt_ItView
						// member function of same name
						//pFWnd->RecalcLayout(); // the bNotify parameter is default TRUE, let it do so,
											   // no harm if the window is not embedded
						SendSizeEvent(); // forces the CMainFrame::SetSize() handler to run and do the needed redraw

						// BEW 13Jan09 added next 3 lines to fix bug reported by Roland Fumey  6Jan09 12:22pm,
						// in which phrase box was left at end of the editable span, not returned to its start.
						// place the phrase box at the start of the span, and update the layout etc
						int activeSequNum = pRec->nAdaptationStep_StartingSequNum;
						pView->PutPhraseBoxAtSequNumAndLayout(pRec,activeSequNum);
					}  // end block for result TRUE for test (pRec->bEditSpanHasAdaptations)
					else
					{
						// no adaptations formerly, so we don't do the above extra steps because the
						// user would not get to see their effect before they get clobbered by the setup
						// of the freeTranslationsStep transition
						//this->PostMessage(CUSTOM_EVENT_FREE_TRANSLATIONS_EDIT,0,0);
						wxCommandEvent eventCustom(wxEVT_Free_Translations_Edit);
						wxPostEvent(this, eventCustom);
						return;
					}
				} // end of block for test (gEditStep == glossesStep)
/*rollback*/	else if (gEditStep == freeTranslationsStep)
				{
					// the user was in free translations step, and requested a roll back to adaptations step;
					// we allow this whether or not there were originally adaptations in the editable span
					// because we assume a rollback is intentional to get adaptations made or altered

					// Rollback has to set up the free translation span to be what it was before freeTranslationsStep
					// was first entered -- which entails that the deep copied CSourcePhrase instances either side of
					// the editable span be restored (these, after modifications were done to the modifications list,
					// had the m_bHasFreeTrans, m_bStartFreeTrans, m_bEndFreeTrans flags all cleared. Failure to do
					// this restoration will cause SetupCurrentFreeTranslationSection() to fail if freeTranslationsStep
					// is entered a second time in this current vertical edit session. Similarly, rolling back from
					// freeTransationsStep will possibly leave text in the edit box of the compose bar - it has to be
					// cleared, etc.
					CMainFrame *pFrame = gpApp->GetMainFrame(); //CFrameWnd* pMainFrm = GetParentFrame();
					wxASSERT(pFrame != NULL);
					// whm: In wx the composeBar is created when the app runs and is always there but may
					// be hidden.
					if (pFrame->m_pComposeBar != NULL)
					{
						wxTextCtrl* pEdit = (wxTextCtrl*)pFrame->m_pComposeBar->FindWindowById(IDC_EDIT_COMPOSE);
						wxASSERT(pEdit != NULL);
						if (pEdit != 0)
						{
							pEdit->SetValue(_T("")); // clear the box
						}
					}
					// now restore the free translation span to what it was at last entry to freeTranslationsStep
					if (pRec->bFreeTranslationStepEntered && pRec->nFreeTranslationStep_SpanCount > 0 &&
						pRec->nFreeTrans_EndingSequNum != -1)
					{
						// this gets the context either side of the user's selection correct, a further replacement
						// below will replace the editable span with the instances at at the start of the earlier
						// glossing step (the user may not have seen it in the GUI, but the list was populated
						// nevertheless)
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
						pView->UpdateSequNumbers(0); // make sure all are in proper sequence in the doc

						// now make the free translation part of the gEditRecord think that freeTranslationsStep
						// has not yet been entered
						pRec->bFreeTranslationStepEntered = FALSE;
						pView->GetDocument()->DeleteSourcePhrases(&pRec->freeTranslationStep_SrcPhraseList);
						pRec->nFreeTranslationStep_EndingSequNum = -1;
						pRec->nFreeTranslationStep_SpanCount = -1;
						pRec->nFreeTranslationStep_StartingSequNum = -1;
						// don't alter bVerseBasedSection because it is set only once per vertical edit
					}
					// we know we are coming from free translations mode being currently on, so we have to switch
					// to adapting mode; moreover, we expect glosses were made or updated, and the user will want
					// to see them, so we make them visible (more info might help the user). We also test to
					// ensure adaptations mode is currently on, if not, we switch to it as well - we must do this
					// first before calling the OnAdvancedFreeTranslationsMode() handler (it's a toggle) because
					// when closing free translation mode it tests the gbIsGlossing flag in order to know whether
					// to restore removed glosses or removed adaptations to the Removed combobox list
					if (gbIsGlossing)
					{
						// user must have manually switched to glossing mode, so turn it back off
						pView->ToggleGlossingMode();
						//OnCheckIsGlossing(); // turns OFF the Glossing checkbox and clears 
											 // gbIsGlossing to FALSE; and leaves gbEnableGlossing
											 // with its TRUE value unchanged; populates list with
											 // removed adapatations
					}
					// toggle free translation mode off
					pView->ToggleFreeTranslationMode();
					//OnAdvancedFreeTranslationMode();


					// If the user removed all the CSourcePhrase instances of the editable span,
					// there are no adaptations to be updated, because the span has no extent. We
					// will try to externally catch and prevent a user rollback to adaptations mode when
					// there is no extent, but just in case we don't, just bring the vertical edit to
					// an end immediately -- this block should never be entered if I code correctly
					if (pRec->nAdaptationStep_OldSpanCount == 0)
					{
						//this->PostMessage(CUSTOM_EVENT_END_VERTICAL_EDIT,0,0);
						wxCommandEvent eventCustom(wxEVT_End_Vertical_Edit);
						wxPostEvent(this, eventCustom);
						// whm Note: This event calls OnCustomEventCancelVerticalEdit() which ends by hiding any of
						// the vertical edit tool bars from the main window.
						return;
					}

					// the span was entered earlier, and we are rolling back, so we have to set up
					// the state as it was at the start of the of the previous adaptationsStep;
					// and we have to restore the phrase box to the start of the editable span too
					// (we don't bother to make any reference count decrements in the KB, so this
					// would be a source of ref counts being larger than strictly correct, but that
					// is a totally benign effect)

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

					// put the adaptations step's message in the multi-line read-only CEdit box
					// IDS_VERT_EDIT_ADAPTATIONS_MSG
					pView->SetVerticalEditModeMessage(_("Vertical Editing - adaptations step: Type the needed adaptations in the editable region. Earlier adaptations are stored at the top of the Removed list. Gray text is not accessible. Adapting mode is currently on and all adaptation functionalities are enabled, including mergers, placeholder insertion and retranslations."));

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

					// initiate a redraw of the frame and the client area (Note, this is MFC's
					// CFrameWnd member function called RecalcLayout(), not my CAdapt_ItView
					// member function of same name
					//pFWnd->RecalcLayout(); // the bNotify parameter is default TRUE, let it do so,
										   // no harm if the window is not embedded
					SendSizeEvent(); // forces the CMainFrame::SetSize() handler to run and do the needed redraw

					// place the phrase box at the start of the span, and update the layout etc
					int activeSequNum_ = pRec->nAdaptationStep_StartingSequNum;
					pView->PutPhraseBoxAtSequNumAndLayout(pRec,activeSequNum_);

				} // end block for result TRUE for test (gEditStep == freeTranslationsStep)
				else
				{
					// ought not to happen because we should only be able to get to adaptationsStep
					// by either normal progression from glossesStep or rollback from freeTranslationsStep,
					// so, cancel out
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
			; // **** TODO in wxWidgets ****
		}
		else if (gEntryPoint == glossesEntryPoint)
		{
			// user has just edited an existing gloss... we'll support this
			// option only in the wxWidgets codebase
			; // **** TODO in wxWidgets ****
		}
		else
		{
			// if none of these, we've got an unexpected state which should never happen,
			// so cancel out
cancel:		; //this->PostMessage(CUSTOM_EVENT_CANCEL_VERTICAL_EDIT,0,0);

			// whm: In the wx version we don't keep the RemovalsBar visible, so since the
			wxCommandEvent eventCustom(wxEVT_Cancel_Vertical_Edit);
			wxPostEvent(this, eventCustom);
			// whm Note: This event calls OnCustomEventCancelVerticalEdit() which ends by hiding any of
			// the vertical edit tool bars from the main window.
			return;
		}
	}
}

// The following is the handler for a CUSTOM_EVENT_GLOSSES_EDIT event message, sent
// to the window event loop by a PostMessage(CUSTOM_EVENT_GLOSSES_EDIT,0,0) call
void CMainFrame::OnCustomEventGlossesEdit(wxCommandEvent& WXUNUSED(event))
{
	// glosses updating is potentially required
	// Insure that we have a valid pointer to the m_pVertEditBar member
	// of the frame window class
	wxASSERT(m_pVertEditBar != NULL);
	wxASSERT(m_pRemovalsBar != NULL);
	
	CAdapt_ItView* pView = gpApp->GetView();
	wxASSERT(pView != NULL);
	//CMainFrame* pFWnd = gpApp->GetMainFrame();
	//wxASSERT(pFWnd != NULL);

	if (m_pVertEditBar == NULL)
	{
		wxMessageBox(_T("Failure to obtain pointer to the vertical edit control bar in \
						 OnCustomEventAdaptationsEdit()"),_T(""), wxICON_EXCLAMATION);
		return;
	}

	// determine what setup is required: control is coming from either adaptationsStep or freeTranslationsStep,
	// when the order is adaptations before glosses; but if the order is glosses before adaptations, then
	// control is coming from either sourceTextStep or adaptationsStep. These variations require separate
	// initialization blocks.
	CAdapt_ItDoc* pDoc = gpApp->GetDocument();
	EditRecord* pRec = &gEditRecord;
	SPList* pSrcPhrases = gpApp->m_pSourcePhrases;
	bool bAllsWell = TRUE;

	if (gbVerticalEditInProgress)
	{
		if (gEntryPoint == sourceTextEntryPoint)
		{
			// when entry is due to a source text edit, we can count on all the source text editing-related
			// parameters in gEditRecord having previously been filled out...
			if (gbAdaptBeforeGloss)
			{
				// user's chosen processing order is "do adaptations before glosses"
				if (gEditStep == adaptationsStep)
				{
					// the PostMessage() which we are handling was fired from the end of the adaptations
					// updating step, so we are still in that step and about to do the things needed to
					// transition to the glosses step

					// we now must get glossing mode turned on
					if (gbEnableGlossing)
					{
						// glosses are visible already, so we only need turn glossing mode on
						pView->ToggleGlossingMode();
						//OnCheckIsGlossing(); // turns ON the Glossing checkbox and sets 
												 // gbIsGlossing to TRUE; and leaves gbEnableGlossing
												 // with its TRUE value unchanged
					}
					else
					{
						// glosses are not currently made visible, first make them visible and then
						// turn on glossing mode
						pView->ToggleSeeGlossesMode();
						//OnAdvancedEnableglossing(); // sets gbEnableGlossing to TRUE, makes
													// the Glossing checkbox be unhidden,
													// leaves gbIsGlossing FALSE
						pView->ToggleGlossingMode();
						//OnCheckIsGlossing(); // turns ON the Glossing checkbox and sets 
											 // gbIsGlossing to TRUE; and leaves gbEnableGlossing
											 // with its TRUE value unchanged
					}

					// If the user removed all the CSourcePhrase instances of the editable span,
					// there are no glosses to be updated, so check for this and if so, fill
					// in the relevant gEditRecord members and then post the event for the next
					// step -- we don't give the user a dialog to vary the step order here, because
					// he won't have seen any feedback up to now that he's about to be in glosses
					// step, so we unilaterally send him on to the next step
					if (pRec->nNewSpanCount == 0)
					{
						// user deleted everything... so move on
						pRec->nGlossStep_SpanCount = 0;
						pRec->nGlossStep_StartingSequNum = -1;
						pRec->nGlossStep_EndingSequNum = -1;

						gEditStep = glossesStep;
						pRec->bGlossStepEntered = TRUE; // prevent reinitialization if user returns here

						// post event for progressing to free translation mode.
						// Note, we have not changed from glossesStep, hence the handler for
						// this message will become active with gEditStep still set to 
						// glossesStep - this is how we inform the next step what the previous
						// step was, and so the handler for the next step will finish off this
						// current step before showing the GUI for the next step to the user
						//this->PostMessage(CUSTOM_EVENT_FREE_TRANSLATIONS_EDIT,0,0);
						wxCommandEvent eventCustom(wxEVT_Free_Translations_Edit);
						wxPostEvent(this, eventCustom);
						return;
					}

					// Initializing must be done only once (the user can return to this step using interface
					// buttons) -- initializing is done the first time glossesStep is entered in the vertical
					// edit, so it is put within a test to prevent multiple initializations -- the flag in the
					// following test is not set until the end of this current function
					if (!pRec->bGlossStepEntered)
					{
						// set up the values for the glossesStep in the edit record, where the CCell drawing
						// function can get them
						bool bAllWasOK;
						pRec->nGlossStep_StartingSequNum = pRec->nAdaptationStep_StartingSequNum;
						pRec->nGlossStep_EndingSequNum = pRec->nGlossStep_StartingSequNum
														+ pRec->nAdaptationStep_NewSpanCount - 1;
						pRec->nGlossStep_SpanCount = pRec->nAdaptationStep_NewSpanCount;

						// here we set the initial state of the glosses span
						// which is of course the final state of the previous adaptationsStep's span after the
						// user has done all his edits (which could include mergers, retranslations and
						// placeholder insertions, and those functionalities have to keep the value of 
						// pRec->nAdaptationStep_NewSpanCount up to date as the user does his work, and also
						// pRec->nAdaptationStep_EndingSequNum - because the CCell::Draw() function which 
						// keeps the relevant text grayed out needs to be continuously aware where the span
						// ends even when the user does editing which changes the span length; so
						// deep copy the initial editable span, before user has a chance to do do any editing.
						// This deep copy must only be done once
						bAllWasOK = pView->DeepCopySourcePhraseSublist(pSrcPhrases, pRec->nGlossStep_StartingSequNum, 
							pRec->nGlossStep_EndingSequNum, &pRec->glossStep_SrcPhraseList);
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

					// record the fact that this step has been entered and initial values set up
					// (it is done once only, see above)
					pRec->bGlossStepEntered = TRUE;

					if (pRec->bEditSpanHasGlosses)
					{
						// populate the combobox with the required removals data for glossesStep
						bAllsWell = pView->PopulateRemovalsComboBox(glossesStep, &gEditRecord);

						// put the glosses step's message in the multi-line read-only CEdit box
						pView->SetVerticalEditModeMessage(_("Vertical Editing - glosses step: Type the needed glosses in the editable region. Earlier glosses are stored at the top of the Removed list. Gray text is not accessible. Glossing  mode is currently on."));

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
											   // no harm if the window is not embedded
						// whm Note: Client area is changing size so send a size event to get the layout to change
						// since the doc/view framework won't do it for us.
						SendSizeEvent(); // forces the CMainFrame::SetSize() handler to run and do the needed redraw

						// place the phrase box at the start of the span, and update the layout etc
						int activeSequNum = pRec->nGlossStep_StartingSequNum;
						pView->PutPhraseBoxAtSequNumAndLayout(pRec,activeSequNum);
					}  // end TRUE block for test (pRec->bEditSpanHasGlosses)
					else
					{
						// no glosses formerly, so we don't do the above extra steps because the
						// user would not get to see their effect before they get clobbered by the setup
						// of the freeTranslationsStep transition
						//this->PostMessage(CUSTOM_EVENT_FREE_TRANSLATIONS_EDIT,0,0);
						wxCommandEvent eventCustom(wxEVT_Free_Translations_Edit);
						wxPostEvent(this, eventCustom);
						return;
					}
				} // end TRUE block for test (gbEditStep == adaptationsStep)
/*rollback*/	else if (gEditStep == freeTranslationsStep)
				{
					// The user was in free translations step, and requested a return to glosses step;
					// we allow this whether or not there were originally glosses in the editable span
					// because we assume a rollback is intentional to get glosses made or altered.

					// Rollback has to set up the free translation span to be what it was before freeTranslationsStep
					// was first entered -- which entails that the deep copied CSourcePhrase instances either side of
					// the editable span be restored (these, after modifications were done to the modifications list,
					// had the m_bHasFreeTrans, m_bStartFreeTrans, m_bEndFreeTrans flags all cleared. Failure to do
					// this restoration will cause SetupCurrentFreeTranslationSection() to fail if freeTranslationsStep
					// is entered a second time in this current vertical edit session. Similarly, rolling back from
					// freeTransationsStep will possibly leave text in the edit box of the compose bar - it has to be
					// cleared, etc.
					CMainFrame *pFrame = gpApp->GetMainFrame(); //CFrameWnd* pMainFrm = GetParentFrame();
					wxASSERT(pFrame != NULL);
					// whm: In wx the composeBar is created when the app runs and is always there but may
					// be hidden.
					if (pFrame->m_pComposeBar != NULL)
					{
						wxTextCtrl* pEdit = (wxTextCtrl*)pFrame->m_pComposeBar->FindWindowById(IDC_EDIT_COMPOSE);
						wxASSERT(pEdit != NULL);
						if (pEdit != 0)
						{
							pEdit->SetValue(_T("")); // clear the box
						}
					}
					// now restore the free translation span to what it was at last entry to freeTranslationsStep
					if (pRec->bFreeTranslationStepEntered && pRec->nFreeTranslationStep_SpanCount > 0 &&
						pRec->nFreeTrans_EndingSequNum != -1)
					{
						// this gets the context either side of the user's selection correct, a further replacement
						// below will replace the editable span with the instances at at the start of the earlier
						// glossing step (the user may not have seen it in the GUI, but the list was populated
						// nevertheless)
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
						pView->UpdateSequNumbers(0); // make sure all are in proper sequence in the doc

						// now make the free translation part of the gEditRecord think that freeTranslationsStep
						// has not yet been entered
						pRec->bFreeTranslationStepEntered = FALSE;
						pView->GetDocument()->DeleteSourcePhrases(&pRec->freeTranslationStep_SrcPhraseList);
						pRec->nFreeTranslationStep_EndingSequNum = -1;
						pRec->nFreeTranslationStep_SpanCount = -1;
						pRec->nFreeTranslationStep_StartingSequNum = -1;
						// don't alter bVerseBasedSection because it is set only once per vertical edit
					}

					// we know we are coming from free translations mode being currently on, so we have to
					// switch to glossing mode
					pView->ToggleFreeTranslationMode();
					//OnAdvancedFreeTranslationMode(); // turn off free translation mode and populate the
													 // removals combo with either glosses or adaptations
													 // depending on current gbIsGlossing value
					// free translation step is usually has adapting mode set ON on entry, and we are
					// wanting to be in glossingStep, so check gbIsGlossing and set up appropriately
					if (gbIsGlossing)
					{
						// glossing mode is on, so nothing more to do
						;
					}
					else
					{
						// adapting mode is on, but glosses may or may not be visible, so check
						// gbEnableGlossing first and get glosses visible if not already TRUE,
						// then turn on glossing mode
						if (!gbEnableGlossing)
						{
							pView->ToggleSeeGlossesMode();
							//OnAdvancedEnableglossing(); // sets gbEnableGlossing TRUE, leaves gbIsGlossing FALSE
						}
						pView->ToggleGlossingMode();
						//OnCheckIsGlossing(); // turns ON the Glossing checkbox and sets 
										 // gbIsGlossing to TRUE; and leaves gbEnableGlossing
										 // with its TRUE value unchanged
					}

					// If the user removed all the CSourcePhrase instances of the editable span,
					// there are no glosses to be updated, because the span has no extent. We
					// will try to externally catch and prevent a user rollback to glosses step when
					// there is no extent, but just in case we don't, just bring the vertical edit to
					// an end immediately -- this block should never be entered if I code correctly
					if (pRec->nNewSpanCount == 0)
					{
						//this->PostMessage(CUSTOM_EVENT_END_VERTICAL_EDIT,0,0);
						wxCommandEvent eventCustom(wxEVT_End_Vertical_Edit);
						wxPostEvent(this, eventCustom);
						gEditStep = glossesStep;
						pRec->bGlossStepEntered = TRUE;
						return;
					}

					// the span was entered earlier, and we are rolling back, so we have to set up
					// the state as it was at the start of the of the previous glossesStep;
					// and we have to restore the phrase box to the start of the editable span too
					// (we don't bother to make any reference count decrements in the glossingKB, so
					// this would be a source of ref counts being larger than strictly correct, but
					// that is a totally benign effect)
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
					pView->UpdateSequNumbers(0); // make sure all are in proper sequence in the doc

					// delete the instances in the freeTranslationsStep's list
					pDoc->DeleteSourcePhrases(&pRec->freeTranslationStep_SrcPhraseList);

					// place the phrase box at the start of the span, and update the layout etc
					int activeSequNum = pRec->nGlossStep_StartingSequNum;
					pView->PutPhraseBoxAtSequNumAndLayout(pRec,activeSequNum);
					
					// populate the combobox with the required removals data for adaptationsStep
					bAllsWell = pView->PopulateRemovalsComboBox(glossesStep, &gEditRecord);

					// put the adaptations step's message in the multi-line read-only CEdit box
					// IDS_VERT_EDIT_GLOSSES_MSG
					pView->SetVerticalEditModeMessage(_("Vertical Editing - glosses step: Type the needed glosses in the editable region. Earlier glosses are stored at the top of the Removed list. Gray text is not accessible. Glossing  mode is currently on."));

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

					// record the fact that this step has been entered and initial values set up
					// (it is done once only, see above)
					pRec->bGlossStepEntered = TRUE;

					// initiate a redraw of the frame and the client area (Note, this is MFC's
					// CFrameWnd member function called RecalcLayout(), not my CAdapt_ItView
					// member function of same name
					//pFWnd->RecalcLayout(); // the bNotify parameter is default TRUE, let it do so,
										   // no harm if the window is not embedded
					SendSizeEvent(); // forces the CMainFrame::SetSize() handler to run and do the needed redraw

				} // end block for result TRUE for test (gEditStep == freeTranslationsStep)
				else
				{
					// ought not to happen because we should only be able to get to glossesStep
					// by either normal progression from adaptationsStep or rollback from 
					// freeTranslationsStep, so, cancel out
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
					// the PostMessage() which we are handling is fired from the end of the
					// source text edit step, so control is still in that step, and about to
					// transition to the glosses step if the editable span had some, if not,
					// we transition from glosses step, after minimal setup, to the
					// adaptations step

					// the mode in effect when the Edit Source Text dialog was visible may not
					// have been glossing mode, it may have been adapting mode, so we need to
					// check and, if necessary, switch to glossing mode; moreover, if there were 
					// glosses in the editable section, we must make them visible, but if there were
					// no glosses in the span, we'll do only minimal setup and transition to the next
					// step
					if (gbIsGlossing)
					{
						// glossing mode is currently ON (and adaptations are always visible
						// in that mode too)
						;
					}
					else
					{
						// adapting mode is in effect, so change the mode to glossing mode
						if (!gbEnableGlossing)
						{
							// glosses are not yet visible, so make them so
							pView->ToggleSeeGlossesMode();
							//OnAdvancedEnableglossing(); // toggles gbEnableGlossing to TRUE, makes
														// the Glossing checkbox be shown
														// leaves gbIsGlossing FALSE
						}
						pView->ToggleGlossingMode();
						//OnCheckIsGlossing(); // turns ON the Glossing checkbox and sets 
											 // gbIsGlossing to TRUE; and leaves gbEnableGlossing
											 // with its TRUE value unchanged; populates list with
											 // removed glosses
					}

					// if the user removed all the CSourcePhrase instances of the editable span,
					// there are no glosses to be updated, so check for this and if so, fill
					// in the relevant gEditRecord members and then post the event for the next
					// step -- we don't give the user a dialog to vary the step order here, because
					// he won't have seen any feedback up to now that he's about to be in glosses
					// step, so we unilaterally send him on to the next step
					if (pRec->nNewSpanCount == 0)
					{
						// user deleted everything... so move on
						pRec->nGlossStep_SpanCount = 0;
						pRec->nGlossStep_StartingSequNum = -1;
						pRec->nGlossStep_EndingSequNum = -1;

						gEditStep = glossesStep;
						pRec->bGlossStepEntered = TRUE; // prevent reinitialization if user returns here

						// post event for progressing to adaptationStep
						//this->PostMessage(CUSTOM_EVENT_ADAPTATIONS_EDIT,0,0);
						wxCommandEvent eventCustom(wxEVT_Adaptations_Edit);
						wxPostEvent(this, eventCustom);
						return;
					}

					if (!pRec->bGlossStepEntered)
					{
						// put the values in the edit record, where the CCell drawing function can get them
						bool bAllWasOK;
						pRec->nGlossStep_StartingSequNum = pRec->nStartingSequNum;
						pRec->nGlossStep_EndingSequNum = pRec->nStartingSequNum + pRec->nNewSpanCount - 1;
						pRec->nGlossStep_SpanCount = pRec->nNewSpanCount;

						// deep copy the initial editable span.  This deep copy
						// must only be done once
						bAllWasOK = pView->DeepCopySourcePhraseSublist(pSrcPhrases, pRec->nGlossStep_StartingSequNum, 
							pRec->nGlossStep_EndingSequNum, &pRec->glossStep_SrcPhraseList);
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

					// record the fact that this step has been entered and initial values set up
					// (it is done once only, see above)
					pRec->bGlossStepEntered = TRUE;

					if (pRec->bEditSpanHasGlosses)
					{
						// populate the combobox with the required removals data for glossesStep
						bAllsWell = pView->PopulateRemovalsComboBox(glossesStep, &gEditRecord);

						// put the adaptations step's message in the multi-line read-only CEdit box
						// IDS_VERT_EDIT_GLOSSES_MSG
						pView->SetVerticalEditModeMessage(_("Vertical Editing - glosses step: Type the needed glosses in the editable region. Earlier glosses are stored at the top of the Removed list. Gray text is not accessible. Glossing  mode is currently on."));

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

						// initiate a redraw of the frame and the client area (Note, this is MFC's
						// CFrameWnd member function called RecalcLayout(), not my CAdapt_ItView
						// member function of same name
						//pFWnd->RecalcLayout(); // the bNotify parameter is default TRUE, let it do so,
											   // no harm if the window is not embedded
						SendSizeEvent(); // forces the CMainFrame::SetSize() handler to run and do the needed redraw


						// place the phrase box at the start of the span, and update the layout etc
						int activeSequNum = pRec->nGlossStep_StartingSequNum;
						pView->PutPhraseBoxAtSequNumAndLayout(pRec,activeSequNum);

					}  // end block for result TRUE for test (pRec->bEditSpanHasGlosses)
					else
					{
						// no glosses formerly, so we don't do the above extra steps because the
						// user would not get to see their effect before they get clobbered by the setup
						// of the adaptationsStep transition
						//this->PostMessage(CUSTOM_EVENT_ADAPTATIONS_EDIT,0,0);
						wxCommandEvent eventCustom(wxEVT_Adaptations_Edit);
						wxPostEvent(this, eventCustom);
						return;
					}
				}
/*rollback*/	else if (gEditStep == adaptationsStep)
				{
					// the user was in adaptations step, and requested a roll back to glosses step;
					// we allow this whether or not there were originally glosses in the editable span
					// because we assume a rollback is intentional to get glosses made or altered

					// we know we are coming from adaptations step being currently on, so we have to switch to
					// glossing step; moreover, glosses may or may not have been visible, but we have to
					// be sure they will be visible in glossesStep
					if (!gbEnableGlossing)
					{
						// make glosses visible and have the glossing checkbox be shown on the command bar
						pView->ToggleSeeGlossesMode();
						//OnAdvancedEnableglossing();
					}
					pView->ToggleGlossingMode();
					//OnCheckIsGlossing(); // turns ON the Glossing checkbox and sets 
										 // gbIsGlossing to TREU; and leaves gbEnableGlossing
										 // with its TRUE value unchanged

					// If the user removed all the CSourcePhrase instances of the editable span,
					// there are no glosses to be updated, because the span has no extent. We
					// will try to externally catch and prevent a user rollback to glosses step when
					// there is no extent, but just in case we don't, just bring the vertical edit to
					// an end immediately -- this block should never be entered if I code correctly
					if (pRec->nNewSpanCount == 0)
					{
						//this->PostMessage(CUSTOM_EVENT_END_VERTICAL_EDIT,0,0);
						wxCommandEvent eventCustom(wxEVT_End_Vertical_Edit);
						wxPostEvent(this, eventCustom);
						gEditStep = glossesStep;
						pRec->bGlossStepEntered = TRUE;
						return;
					}

					// the span was entered earlier, and we are rolling back, so we have to set up
					// the state as it was at the start of the of the previous glossesStep; (while
					// in adaptationsStep the user may have changed the length of the editable span
					// by doing placeholder insertions, retranslations, mergers)
					// and we have to restore the phrase box to the start of the editable span too
					// (we don't bother to make any reference count decrements in the glossingKB, so this
					// would be a source of ref counts being larger than strictly correct, but that
					// is a totally benign effect)

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

					// put the glosses step's message in the multi-line read-only CEdit box
					// IDS_VERT_EDIT_GLOSSES_MSG
					pView->SetVerticalEditModeMessage(_("Vertical Editing - glosses step: Type the needed glosses in the editable region. Earlier glosses are stored at the top of the Removed list. Gray text is not accessible. Glossing  mode is currently on."));

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

					// record the fact that this step has been entered and initial values set up
					// (it is done once only, see above)
					pRec->bGlossStepEntered = TRUE;

					// initiate a redraw of the frame and the client area (Note, this is MFC's
					// CFrameWnd member function called RecalcLayout(), not my CAdapt_ItView
					// member function of same name
					//pFWnd->RecalcLayout(); // the bNotify parameter is default TRUE, let it do so,
										   // no harm if the window is not embedded

					SendSizeEvent(); // forces the CMainFrame::SetSize() handler to run and do the needed redraw
				} // end block for result TRUE for test (gEditStep == glossesStep)
				else
				{
					// ought not to happen because we should only be able to get to glossesStep
					// by either normal progression from sourceTextStep or rollback from
					// adaptationsStep, so, cancel out
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
			; // **** TODO in wxWidgets ****
		}
		else
		{
			// if none of these, we've got an unexpected state which should never happen,
			// so cancel out
cancel:		;
			//this->PostMessage(CUSTOM_EVENT_CANCEL_VERTICAL_EDIT,0,0);
			wxCommandEvent eventCustom(wxEVT_Cancel_Vertical_Edit);
			wxPostEvent(this, eventCustom);
			return;
		}
	}
}

// The following is the handler for a CUSTOM_EVENT_FREE_TRANSLATIONS_EDIT event message, sent
// to the window event loop by a PostMessage(CUSTOM_EVENT_FREE_TRANSLATIONS_EDIT,0,0) call
void CMainFrame::OnCustomEventFreeTranslationsEdit(wxCommandEvent& WXUNUSED(event))
{
	// free translations updating is potentially required
	// Insure that we have a valid pointer to the m_pVertEditBar member
	// of the frame window class
	wxASSERT(m_pVertEditBar != NULL);
	wxASSERT(m_pRemovalsBar != NULL);
	
	CAdapt_ItView* pView = gpApp->GetView();

	if (m_pVertEditBar == NULL)
	{
		wxMessageBox(_T("Failure to obtain pointer to the vertical edit control bar in \
						 OnCustomEventAdaptationsEdit()"),_T(""), wxICON_EXCLAMATION);
		return;
	}

	// determine what setup is required: control is coming from either adaptationsStep or glossesStep,
	// the former when the order is adaptations before glosses; but if the order is glosses before adaptations,
	// then the latter. These variations require separate initialization blocks.
	//CAdapt_ItDoc* pDoc = gpApp->GetDocument();
	EditRecord* pRec = &gEditRecord;
	SPList* pSrcPhrases = gpApp->m_pSourcePhrases;
	bool bAllsWell = TRUE;

	if (gbVerticalEditInProgress)
	{
		if (gEntryPoint == sourceTextEntryPoint)
		{
			// when entry is due to a source text edit, we can count on all the source text editing-related
			// parameters in gEditRecord having previously been filled out...
			if (gbAdaptBeforeGloss)
			{
				// user's chosen processing order is "do adaptations before glosses"
				if (gEditStep == glossesStep)
				{
					// the PostMessage() which we are handling was fired from the end of the glosses
					// updating step, so we are still in that step and about to do the things needed to
					// transition to the free translations step

					// we now must get free translations mode turned on; and we assume that the user
					// will be using it for free translating the adaptation line, and so we also have
					// to get glossing mode turned off - but leave glosses visible (which permits
					// the user to still free translate the gloss line if he chooses)
					if (gbIsGlossing)
					{
						// glossing mode is currently ON, so we only need turn glossing mode off
						pView->ToggleGlossingMode();
						//OnCheckIsGlossing(); // turns OFF the Glossing checkbox and clears 
												 // gbIsGlossing to FALSE; and leaves gbEnableGlossing
												 // with its TRUE value unchanged; also fills the
												 // removals combo with removed adaptations (we will
												 // fix the latter below)
					}
					else
					{
						// adaptation mode is in effect already, make glosses visible if they are not
						// already so
						if (!gbEnableGlossing)
							pView->ToggleSeeGlossesMode();
							//OnAdvancedEnableglossing(); // sets gbEnableGlossing to TRUE, makes
														// the Glossing checkbox be unhidden,
														// leaves gbIsGlossing FALSE
					}

					// If the user removed all the CSourcePhrase instances of the editable span,
					// there are no glosses or adaptations to be updated, but it is quite probable
					// that one or more free translation sections are affected by such removal. So
					// we must give the user the chance to retranslate, or copy the old ones from
					// the removals combobox list and tweak them. However, if the user removed all the selection,
					// then we must look at the free translation span - if it is coextensive with the editable
					// span, then whole free translation section(s) were removed and there is nothing for the
					// user to do; but if it is larger than the editable span, then the user must be shown
					// the GUI as he has some editing of free translations to do.
					bool bShowGUI = FALSE; // default is that there were no free translations impinging on the
										   // editable span, so no need to show the GUI in this step
					if (pRec->bEditSpanHasFreeTranslations)
					{
						// the GUI may need to be shown, check further
						if (pRec->nNewSpanCount > 0)
						{
							// the user did not remove everything, so some free translating needs to be done
							bShowGUI = TRUE;
						}
						else
						{
							// the user removed everything in the editable span; this span may be coextensive
							// with the ends of one or more consecutive free translations - which also have
							// been removed (to the combo box list); we assume that the user needs to see the
							// GUI only if the one or more free translations that were present extended beyond
							// the start or end, or both, of the editable span - in that case only would some
							// editing of the free translations impinging on the editable span need their
							// meanings modified. That is, if the source words and the free translations of
							// them where together removed, what is in the context can be assumed to be correct. 
							
							// NOTE: The above assumption is not linguistically defensible, as typically conjunctions
							// need to be tweaked, but we will let the user do such tweaks in a manual read-through
							// later on, to keep things as simple as possible for the vertical edit process. 
							// (If users ever complain about this, we can here do an extension of the free translation
							// span to include single free translation either side of the removed text, so that any
							// minor meaning tweaks to maintain connected readability can be done. I'm assuming
							// that the frequency of this ever being an issue is so small as to not be worth the
							// time coding for it.)
							int nOverlapCount = pRec->nFreeTrans_EndingSequNum - pRec->nFreeTrans_StartingSequNum
								+ 1 - pRec->nOldSpanCount;
							if (nOverlapCount > 0)
							{
								// the user needs to see the GUI, because the end of the earlier free translation
								// overlaps the start of the former editable span or the start of the following
								// free translation overlaps the end of the former editable span, or both; so that
								// strongly implies those free translations need significant editing
								bShowGUI = TRUE;
							}
						}
					}
					if (!bShowGUI)
					{
						// user deleted everything... perhaps including one of more complete free translation
						// sections, so assume no big need for editing exists and move on
						pRec->nFreeTranslationStep_SpanCount = 0;
						pRec->nFreeTranslationStep_StartingSequNum = -1;
						pRec->nFreeTranslationStep_EndingSequNum = -1;

						gEditStep = freeTranslationsStep;
						pRec->bFreeTranslationStepEntered = TRUE; // prevent reinitialization if user returns
											// to here,  -- though I don't think I'm going to make that possible
											// once this block has been entered

						// post event for progressing to (re-collecting) back translations step.
						//this->PostMessage(CUSTOM_EVENT_BACK_TRANSLATIONS_EDIT,0,0);
						wxCommandEvent eventCustom(wxEVT_Back_Translations_Edit);
						wxPostEvent(this, eventCustom);
						return;
					}

					// the GUI is going to be shown, so restore the original value of the
					// flag m_bDefineFreeTransByPunctuation
					gpApp->m_bDefineFreeTransByPunctuation = !pRec->bVerseBasedSection;

					// now switch free translations mode ON; we do it here so that we don't bother
					// to switch it on if the user is not going to be shown the GUI for this step;
					// but in the backTranslationsStep's handler we will then have to test the value of
					// gpApp->m_bFreeTranslationMode to determine whether or not free translations
					// mode was turned on at the earlier step so as to make sure we turn it off only
					// because we know it was turned on earlier!
					pView->ToggleFreeTranslationMode();
					//OnAdvancedFreeTranslationMode();

					// Initializing must be done only once (the user can return to this step using interface
					// buttons) -- initializing is done the first time glossesStep is entered in the vertical
					// edit, so it is put within a test to prevent multiple initializations -- the flag in the
					// following test is not set until the end of this current function
					if (!pRec->bFreeTranslationStepEntered)
					{
						// set up the values for the freeTranslationsStep in the edit record, where
						// the CCell drawing function can get them
						bool bAllWasOK;
						pRec->nFreeTranslationStep_StartingSequNum = pRec->nFreeTrans_StartingSequNum;
						pRec->nFreeTranslationStep_EndingSequNum = pRec->nFreeTrans_EndingSequNum;
						// the above value is based on the editable span prior to any span-length changes 
						// which the user made while editing in adaptationsStep (such as mergers, retranslations
						// or placeholder insertions), so if he changed the span length - whether longer or
						// shorter, we have to make the same adjustment to the ending sequence number now;
						// we also must take account of any shortening or lengthening due to the source text
						// edit itself
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

						// here we set the initial state of the free translations span which is
						// of course the final state of the previous glossesStep's span; so deep copy
						// the initial state. This deep copy must only be done once.  This deep copy is not needed
						// for forward progression, but invaluable for rollback, as it ensures the free translation
						// flags are cleared in the context of the editable span when the earlier step is set up again
						bAllWasOK = pView->DeepCopySourcePhraseSublist(pSrcPhrases, pRec->nFreeTranslationStep_StartingSequNum, 
							pRec->nFreeTranslationStep_EndingSequNum, &pRec->freeTranslationStep_SrcPhraseList);
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

					// record the fact that this step has been entered and initial values set up
					// (it is done once only, see above)
					pRec->bFreeTranslationStepEntered = TRUE;

					if (bShowGUI)
					{
						// populate the combobox with the required removals data for freeTranslationsStep
						bAllsWell = pView->PopulateRemovalsComboBox(freeTranslationsStep, &gEditRecord);

						// put the glosses step's message in the multi-line read-only CEdit box
						// IDS_VERT_EDIT_FREE_TRANSLATIONS_MSG
						pView->SetVerticalEditModeMessage(_("Vertical Editing - free translations step: Type the needed free translations in the editable region. Earlier free translations are stored at the top of the Removed list. Clicking on one copies it immediately into the Compose Bar's edit box, overwriting the default free translation there. Gray text is not accessible. Free translations mode is currently on and all free translation functionalities are enabled."));

						// setup the toolbar with the vertical edit process control buttons and user
						// message, and make it show-able
						// whm added: the MFC version of vertical editing had the RemovalsBar always
						// showing. I'm keeping it hidden until it really is needed.
						// update static text in removals bar to indicate that clicking on item in
						// list copies to the compose bar's text box
						wxStaticText* pStatic = (wxStaticText*)m_pRemovalsBar->FindWindowById(ID_STATIC_TEXT_REMOVALS);
						wxASSERT(pStatic != NULL);
						pStatic->SetLabel(_("Clicking on an item in the above list copies it to the Compose Bar's text box, overwriting anything there."));
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
											   // no harm if the window is not embedded
						SendSizeEvent(); // forces the CMainFrame::SetSize() handler to run and do the needed redraw
						
						// place the phrase box at the start of the span, and update the layout etc
						int activeSequNum = pRec->nFreeTranslationStep_StartingSequNum;
						pView->PutPhraseBoxAtSequNumAndLayout(pRec,activeSequNum);
					}  // end TRUE block for test (bShowGUI)
					else
					{
						// no free translations formerly, or no need for the user to do any free
						// translation updating, so we don't do the above extra steps because the
						// user would not get to see their effect before they get clobbered by the setup
						// of the backTranslationsStep transition
						//this->PostMessage(CUSTOM_EVENT_BACK_TRANSLATIONS_EDIT,0,0);
						wxCommandEvent eventCustom(wxEVT_Back_Translations_Edit);
						wxPostEvent(this, eventCustom);
						return;
					}
				} // end TRUE block for test (gbEditStep == glossesStep)
/*rollback*/	else if (gEditStep == backTranslationsStep)
				{
					// the vertical edit design currently does not provided a rollback possibility
					// from back translations step - the latter is automatic, has no GUI, and we
					// won't put up the dialog for user control of next step at its end, but just
					// close the vertical edit process; so if somehow control gets here, then just end
					//this->PostMessage(CUSTOM_EVENT_END_VERTICAL_EDIT,0,0);
					wxCommandEvent eventCustom(wxEVT_End_Vertical_Edit);
					wxPostEvent(this, eventCustom);
				}
				else
				{
					// ought not to happen because we should only be able to get to freeTranslationsStep
					// by either normal progression from glossesStep or adaptationsStep, so cancel out
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
					// the PostMessage() which we are handling was fired from the end of the adaptations
					// updating step, so we are still in that step and about to do the things needed to
					// transition to the free translations step

					// we now must get free translations mode turned on; and we assume that the user
					// will be using it for free translating the adaptation line, and so we also have
					// to get glosses visible if not already so (which permits the user to still free
					// translate the gloss line if he chooses)
					if (!gbEnableGlossing)
					{
						pView->ToggleSeeGlossesMode();
						//OnAdvancedEnableglossing(); // sets gbEnableGlossing to TRUE, makes
													// the Glossing checkbox be unhidden,
													// leaves gbIsGlossing FALSE
					}

					// If the user removed all the CSourcePhrase instances of the editable span,
					// there are no glosses or adaptations to be updated, but it is quite probable
					// that one or more free translation sections are affected by such removal. So
					// we must give the user the chance to retranslate, or copy the old ones from
					// the removals combobox list and tweak them. However, if the user removed all the selection,
					// then we must look at the free translation span - if it is coextensive with the editable
					// span, then whole free translation section(s) were removed and there is nothing for the
					// user to do; but if it is larger than the editable span, then the user must be shown
					// the GUI as he has some editing of free translations to do.
					bool bShowGUI = FALSE; // default is that there were no free translations impinging on the
										   // editable span, so no need to show the GUI in this step
					if (pRec->bEditSpanHasFreeTranslations)
					{
						// the GUI may need to be shown, check further
						if (pRec->nNewSpanCount > 0)
						{
							// the user did not remove everything, so some free translating needs to be done
							bShowGUI = TRUE;
						}
						else
						{
							// the user removed everything in the editable span; this span may be coextensive
							// with the ends of one or more consecutive free translations - which also have
							// been removed (to the combo box list); we assume that the user needs to see the
							// GUI only if the one or more free translations that were present extended beyond
							// the start or end, or both, of the editable span - in that case only would some
							// editing of the free translations impinging on the editable span need their
							// meanings modified. That is, if the source words and the free translations of
							// them where together removed, what is in the context can be assumed to be correct. 
							
							// NOTE: The above assumption is not linguistically defensible, as typically conjunctions
							// need to be tweaked, but we will let the user do such tweaks in a manual read-through
							// later on, to keep things as simple as possible for the vertical edit process. 
							// (If users ever complain about this, we can here do an extension of the free translation
							// span to include single free translation either side of the removed text, so that any
							// minor meaning tweaks to maintain connected readability can be done. I'm assuming
							// that the frequency of this ever being an issue is so small as to not be worth the
							// time coding for it.)
							int nOverlapCount = pRec->nFreeTrans_EndingSequNum - pRec->nFreeTrans_StartingSequNum
								+ 1 - pRec->nOldSpanCount;
							if (nOverlapCount > 0)
							{
								// the user needs to see the GUI, because the end of the earlier free translation
								// overlaps the start of the former editable span or the start of the following
								// free translation overlaps the end of the former editable span, or both; so that
								// strongly implies those free translations need significant editing
								bShowGUI = TRUE;
							}
						}
					}
					if (!bShowGUI)
					{
						// user deleted everything... perhaps including one of more complete free translation
						// sections, so assume no big need for editing exists and move on
						pRec->nFreeTranslationStep_SpanCount = 0;
						pRec->nFreeTranslationStep_StartingSequNum = -1;
						pRec->nFreeTranslationStep_EndingSequNum = -1;

						gEditStep = freeTranslationsStep;
						pRec->bFreeTranslationStepEntered = TRUE; // prevent reinitialization if user returns
											// to here,  -- though I don't think I'm going to make that possible
											// once this block has been entered

						// post event for progressing to (re-collecting) back translations step.
						//this->PostMessage(CUSTOM_EVENT_BACK_TRANSLATIONS_EDIT,0,0);
						wxCommandEvent eventCustom(wxEVT_Back_Translations_Edit);
						wxPostEvent(this, eventCustom);
						return;
					}

					// the GUI is going to be shown, so restore the original value of the
					// flag m_bDefineFreeTransByPunctuation
					gpApp->m_bDefineFreeTransByPunctuation = !pRec->bVerseBasedSection;

					// now switch free translations mode ON; we do it here so that we don't bother
					// to switch it on if the user is not going to be shown the GUI for this step;
					// but in the backTranslationsStep's handler we will then have to test the value of
					// gpApp->m_bFreeTranslationMode to determine whether or not free translations
					// mode was turned on at the earlier step so as to make sure we turn it off only
					// because we know it was turned on earlier!
					pView->ToggleFreeTranslationMode();
					//OnAdvancedFreeTranslationMode();

					// Initializing must be done only once (the user can return to this step using interface
					// buttons) -- initializing is done the first time glossesStep is entered in the vertical
					// edit, so it is put within a test to prevent multiple initializations -- the flag in the
					// following test is not set until the end of this current function
					if (!pRec->bFreeTranslationStepEntered)
					{
						// set up the values for the freeTranslationsStep in the edit record, where
						// the CCell drawing function can get them
						bool bAllWasOK;
						pRec->nFreeTranslationStep_StartingSequNum = pRec->nFreeTrans_StartingSequNum;
						pRec->nFreeTranslationStep_EndingSequNum = pRec->nFreeTrans_EndingSequNum;
						// the above value is based on the editable span prior to any span-length changes 
						// which the user made while editing in adaptationsStep (such as mergers, retranslations
						// or placeholder insertions), so if he changed the span length - whether longer or
						// shorter, we have to make the same adjustment to the ending sequence number now
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

						// here we set the initial state of the free translations span which is
						// of course the final state of the previous glossesStep's span; so deep copy
						// the initial state. This deep copy must only be done once.  Actually, since I doubt
						// I'll provide a way to rollback to the start of the free translations step once control
						// has moved on, this copy is superfluous, but I'll do it so that if I change my mind
						// everything has nevertheless been set up right
						bAllWasOK = pView->DeepCopySourcePhraseSublist(pSrcPhrases, pRec->nFreeTranslationStep_StartingSequNum, 
							pRec->nFreeTranslationStep_EndingSequNum, &pRec->freeTranslationStep_SrcPhraseList);
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

					// record the fact that this step has been entered and initial values set up
					// (it is done once only, see above)
					pRec->bFreeTranslationStepEntered = TRUE;

					if (bShowGUI)
					{
						// populate the combobox with the required removals data for freeTranslationsStep
						bAllsWell = pView->PopulateRemovalsComboBox(freeTranslationsStep, &gEditRecord);

						// put the glosses step's message in the multi-line read-only CEdit box
						// IDS_VERT_EDIT_FREE_TRANSLATIONS_MSG
						pView->SetVerticalEditModeMessage(_("Vertical Editing - free translations step: Type the needed free translations in the editable region. Earlier free translations are stored at the top of the Removed list. Clicking on one copies it immediately into the Compose Bar's edit box, overwriting the default free translation there. Gray text is not accessible. Free translations mode is currently on and all free translation functionalities are enabled."));

						// setup the toolbar with the vertical edit process control buttons and user
						// message, and make it show-able
						// whm added: the MFC version of vertical editing had the RemovalsBar always
						// showing. I'm keeping it hidden until it really is needed.
						// update static text in removals bar to indicate that clicking on item in
						// list copies to the compose bar's text box
						wxStaticText* pStatic = (wxStaticText*)m_pRemovalsBar->FindWindowById(ID_STATIC_TEXT_REMOVALS);
						wxASSERT(pStatic != NULL);
						pStatic->SetLabel(_("Clicking on an item in the above list copies it to the Compose Bar's text box, overwriting anything there."));
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
											   // no harm if the window is not embedded
						// whm Note: Client area is changing size so send a size event to get the layout to change
						// since the doc/view framework won't do it for us.
						SendSizeEvent(); // forces the CMainFrame::SetSize() handler to run and do the needed redraw
						
						// place the phrase box at the start of the span, and update the layout etc
						int activeSequNum = pRec->nFreeTranslationStep_StartingSequNum;
						pView->PutPhraseBoxAtSequNumAndLayout(pRec,activeSequNum);
					}  // end TRUE block for test (bShowGUI)
					else
					{
						// no free translations formerly, or no need for the user to do any free
						// translation updating, so we don't do the above extra steps because the
						// user would not get to see their effect before they get clobbered by the setup
						// of the backTranslationsStep transition
						//this->PostMessage(CUSTOM_EVENT_BACK_TRANSLATIONS_EDIT,0,0);
						wxCommandEvent eventCustom(wxEVT_Back_Translations_Edit);
						wxPostEvent(this, eventCustom);
						return;
					}
				} // end TRUE block for test (gbEditStep == glossesStep)
/*rollback*/	else if (gEditStep == backTranslationsStep)
				{
					// the vertical edit design currently does not provided a rollback possibility
					// from back translations step - the latter is automatic, has no GUI, and we
					// won't put up the dialog for user control of next step at its end, but just
					// close the vertical edit process; so if somehow control gets here, then just end
					//this->PostMessage(CUSTOM_EVENT_END_VERTICAL_EDIT,0,0);
					wxCommandEvent eventCustom(wxEVT_End_Vertical_Edit);
					wxPostEvent(this, eventCustom);
				}
				else
				{
					// ought not to happen because we should only be able to get to freeTranslationsStep
					// by either normal progression from glossesStep, so cancel out
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
			; // **** TODO in wxWidgets ****
		}
		else
		{
			// if none of these, we've got an unexpected state which should never happen,
			// so cancel out
cancel:		;
			//this->PostMessage(CUSTOM_EVENT_CANCEL_VERTICAL_EDIT,0,0);
			wxCommandEvent eventCustom(wxEVT_Cancel_Vertical_Edit);
			wxPostEvent(this, eventCustom);
			return;
		}
	}
}

// The following is the handler for a CUSTOM_EVENT_BACK_TRANSLATIONS_EDIT event message, sent
// to the window event loop by a PostMessage(CUSTOM_EVENT_BACK_TRANSLATIONS_EDIT,0,0) call
void CMainFrame::OnCustomEventBackTranslationsEdit(wxCommandEvent& WXUNUSED(event))
{
	CAdapt_ItView* pView = gpApp->GetView();
	wxASSERT(pView != NULL);
	if (gpApp->m_bFreeTranslationMode)
	{
		// free translations mode was turned on in the freeTranslationsStep
		// (a call to StoreFreeTranslationOnLeaving() should have been done
		// prior to entry, and if not  then the final free translation would not have
		// been stored filtered at the anchor location)
		pView->ToggleFreeTranslationMode();
		//OnAdvancedFreeTranslationMode();
	}

// ***** TODO *****   the functions which will do the checking and recollecting of back translations
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

void CMainFrame::OnCustomEventEndVerticalEdit(wxCommandEvent& WXUNUSED(event))
{
	CAdapt_ItDoc* pDoc = gpApp->GetDocument();
	EditRecord* pRec = &gEditRecord;
	//bool bAllsWell = TRUE;
	
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
			//OnAdvancedFreeTranslationMode();
			pView->ToggleFreeTranslationMode();
		}

		// hide the toolbar with the vertical edit process control buttons and user message,
		// and make it show-able
		//pBar->Show(FALSE);
		if (m_pVertEditBar->IsShown())
		{
			m_pVertEditBar->Hide();
		}
		// whm: wx version also hides the removalsBar
		if (m_pRemovalsBar->IsShown())
		{
			m_pRemovalsBar->Hide();
		}

		// typically, the user will have entered text in the phrase box, and we don't want it lost
		// at the restoration of the original mode; while we can't be sure there will be text in the
		// box when this handler is called, in our favour is the fact that there is no copy of the
		// source text in vertical edit mode and so the phrase box won't have anything in it unless the
		// user typed something there. So, while we risk putting a rubbish word into the KB, in most
		// situations we won't be and so we'll unilaterally do the store now - using whatever mode (either
		// glossing or adapting) is currently still in effect
		pView->DoConditionalStore(); // parameter defaults TRUE, FALSE, are in effect

		// restore the original mode, 
		pView->RestoreMode(gbEnableGlossing, gbIsGlossing, &gEditRecord);
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
		}
		else
		{
			// when the user first entered the vertical edit state, glossing mode was OFF, so
			// populate the combobox with the list of removed adaptations as it currently stands
			bFilledListOK = pView->PopulateRemovalsComboBox(adaptationsStep, pRec);
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
	}
	return;
}

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
		// gEditStep, on entry, has the step which is current when the cause of the cancellation request
		// posted the cancellation event; the switches in the blocks below then determine where the cancellation
		// rollback steps begin; and as each step is rolled back, control falls through to the previous step. 
		// Rollback goes only until the original entry point's state is restored.

		// The order of cancellation steps depends on whether the user's preferred step order was adaptations
		// before glosses (the default), or vise versa
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
					pEdit->SetValue(_T("")); // clear the box
					// now restore the free translation span to what it was at last entry to freeTranslationsStep
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
						pView->UpdateSequNumbers(0); // make sure all are in proper sequence in the doc
					}
					pView->ToggleFreeTranslationMode(); // turn off free translation mode
				} // fall through
			case glossesStep:
				if (gEntryPoint == glossesEntryPoint)
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
					if (!gbIsGlossing)
					{
						// turn on glossing mode if it is not currently on
						if (!gbEnableGlossing)
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
						pView->UpdateSequNumbers(0); // make sure all are in proper sequence in the doc
						// leave deletion of contents of freeTranslationStep_SrcPhraseList until
						// the final call of InitializeEditRecord()
					}
					//pFWnd->RecalcLayout(); // (MFC) the bNotify parameter is default TRUE, let it do so,
								 // no harm if the window is not embedded
					SendSizeEvent(); // forces the CMainFrame::SetSize() handler to run and do the needed redraw
					pView->ToggleGlossingMode(); // turn off glossing mode
				} // fall through
			case adaptationsStep:
				if (gEntryPoint == adaptationsEntryPoint)
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
					gEditStep = adaptationsStep;
					if (pRec->nNewSpanCount > 0)
					{
						// restore the editable span to what it was when adaptationsStep was started
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
						pView->UpdateSequNumbers(0); // make sure all are in proper sequence in the doc
						// restore original counts to step-initial values
						// pRec->nAdaptationStep_EndingSequNum -= pRec->nAdaptationStep_ExtrasFromUserEdits;
						// pRec->nAdaptationStep_ExtrasFromUserEdits = 0;
						// pRec->nAdaptationStep_NewSpanCount = pRec->nAdaptationStep_OldSpanCount;
						// deleting the contents of the glossStep_SrcPhraseList is left for InitializeEditRecord()
					}
					//pFWnd->RecalcLayout();
					SendSizeEvent(); // forces the CMainFrame::SetSize() handler to run and do the needed redraw
				} // fall through
			case sourceTextStep:
				if (gEntryPoint == sourceTextEntryPoint)
				{
					// restore to the number of instances when editableSpan had not had any user edits yet
					// (they don't have to be the correct ones, as the restoration of the cancel span will
					// remove them all and insert the cancel span ones, later)
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
							// need to make some insertions, just take them from start of cancel span (the
							// only need we have is to move the right context rightwards so is gets located
							// correctly before the replacement later on)
							bool bWasOK;
							bWasOK = pView->ReplaceCSourcePhrasesInSpan(pSrcPhrases,
								pRec->nStartingSequNum + pRec->nNewSpanCount, 
								0, // no deletions wanted
								&pRec->cancelSpan_SrcPhraseList, 
								0, // start at index 0, ie. insert whole of deep copied list
								nHowMany);
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
						}
					}
					// some of the instances in the span above are wrong, but the span is now co-extensive
					// with the cancel span, so when we overwrite this with the cancel span, we've restored
					// the original state (except perhaps if the propagation span sticks out past the end of
					// the cancel span) - we do these copies in the sourceTextStp case below.
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
						pView->UpdateSequNumbers(0); // make sure all are in proper sequence in the doc
					}
				}
				break;
			} // end of switch (gEditStep)
		} // end of TRUE block for test (gbAdaptBeforeGloss)
		else
		{
			// glosses step entered before adaptations step (an unusual order, selectable in Preferences...)
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
					gEditStep = freeTranslationsStep;
					//pEdit->SetWindowText(_T("")); // clear the box
					pEdit->SetValue(_T("")); // clear the box
					
					// now restore the free translation span to what it was at last entry to
					// freeTranslationsStep ie. as it was at the end of adaptationsStep
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
						pView->UpdateSequNumbers(0); // make sure all are in proper sequence in the doc
					}
					pView->ToggleFreeTranslationMode(); // turn off free translation mode
				} // fall through
			case adaptationsStep:
				if (gEntryPoint == adaptationsEntryPoint)
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
					if (gbIsGlossing)
					{
						pView->ToggleGlossingMode(); // turn glossing mode off, as well as See Glosses
						pView->ToggleSeeGlossesMode();
					}
					else  if (gbEnableGlossing)
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
						pView->UpdateSequNumbers(0); // make sure all are in proper sequence in the doc
						// leave deletion of contents of freeTranslationStep_SrcPhraseList until
						// the final call of InitializeEditRecord()
					}
					//pFWnd->RecalcLayout(); // (MFC) the bNotify parameter is default TRUE, let it do so,
								 // no harm if the window is not embedded
					SendSizeEvent(); // forces the CMainFrame::SetSize() handler to run and do the needed redraw
				} // fall through
			case glossesStep:
				if (gEntryPoint == glossesEntryPoint)
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
					if (!gbIsGlossing)
					{
						if (!gbEnableGlossing)
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
						pView->UpdateSequNumbers(0); // make sure all are in proper sequence in the doc
						// leave deletion of contents of freeTranslationStep_SrcPhraseList until
						// the final call of InitializeEditRecord()
					}
					//pFWnd->RecalcLayout(); // (MFC) the bNotify parameter is default TRUE, let it do so,
							 	// no harm if the window is not embedded
					SendSizeEvent(); // forces the CMainFrame::SetSize() handler to run and do the needed redraw
					pView->ToggleGlossingMode(); // turn glossing mode back off
				} // fall through
			case sourceTextStep:
				if (gEntryPoint == sourceTextEntryPoint)
				{
					// rollback this step and then exit...
					// restore to the number of instances when editableSpan had not had any user edits yet
					// (they don't have to be the correct ones, as the restoration of the cancel span will
					// remove them all and insert the cancel span ones, later)
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
							// need to make some insertions, just take them from start of cancel span (the
							// only need we have is to move the right context rightwards so is gets located
							// correctly before the replacement later on)
							bool bWasOK;
							bWasOK = pView->ReplaceCSourcePhrasesInSpan(pSrcPhrases,
								pRec->nStartingSequNum + pRec->nNewSpanCount, 
								0, // no deletions wanted
								&pRec->cancelSpan_SrcPhraseList, 
								0, // start at index 0, ie. insert whole of deep copied list
								nHowMany);
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
						}
					}
					// some of the instances in the span above are wrong, but the span is now co-extensive
					// with the cancel span, so when we overwrite this with the cancel span, we've restored
					// the original state (except perhaps if the propagation span sticks out past the end of
					// the cancel span) - we do these copies in the sourceTextStp case below.
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
						pView->UpdateSequNumbers(0); // make sure all are in proper sequence in the doc
					}
				}
				break;
			} // end of switch (gEditStep)
		} // end of FALSE block for test (gbAdaptBeforeGloss)

		// clean up & restore original state
		//this->PostMessage(CUSTOM_EVENT_END_VERTICAL_EDIT,0,0);
		wxCommandEvent eventCustom(wxEVT_End_Vertical_Edit);
		wxPostEvent(this, eventCustom);
		// whm Note: This event also calls the code which hides any of the
		// vertical edit tool bars so they are not seen from the main window.
	} // end of TRUE block for test (gbVerticalEditInProgress)
	
	// whm addition:
	// When vertical editing is canceled we should hide the m_pRemovalsBar, and m_pVertEditBar,
	//  - any and all that are visible.
	if (m_pVertEditBar->IsShown())
	{
		m_pVertEditBar->Hide();
	}
	if (m_pRemovalsBar->IsShown())
	{
		m_pRemovalsBar->Hide();
	}
	SendSizeEvent(); // forces the CMainFrame::SetSize() handler to run and do the needed redraw;
	return;
}

// MFC: OnRemovalsComboSelChange() handles a click in the dropped down list, sending the string direct to the
// phrase box which is rebuilt, and with focus put there and cursor following the last character; but it
// doesn't handle a click on the already visible item in the combobox window - there isn't a way to make that
// work as far as I can tell
// 
// (whm note: Bruce tried using a OnRemovalsComboSetFocus() handler, which worked, but blocked dropping down
// the list) so apparently the user will have to drop the list down manually rather than be able to just
// click on the text in the combo box window). void CMainFrame::OnRemovalsComboSelChange(wxCommandEvent&
// WXUNUSED(event))
void CMainFrame::OnRemovalsComboSelChange(wxCommandEvent& WXUNUSED(event))
{
	CAdapt_ItApp* pApp = &wxGetApp();
	//CAdapt_ItDoc* pDoc;
	CAdapt_ItView* pView = pApp->GetView();
	//CPhraseBox* pBox;
	//pApp->GetBasePointers(pDoc,pView,pBox);

	if (!ListBoxPassesSanityCheck((wxControlWithItems*)m_pRemovalsBarComboBox))
	{
		return;
	}

	int nIndex = m_pRemovalsBarComboBox->GetSelection(); //MFC has GetCurSel() which equates to the wxComboBox's base class wxControlWithItems::GetSelection()
	wxString theText;
	// whm: MFC's CComboBox::GetLBText() "Gets a string from the list box of a combo box." nIndex
	// contains the zero-based index of the list-box string to be copied. The second parameter points
	// to a CString that receives the string. In 
	theText = m_pRemovalsBarComboBox->GetString(nIndex); //m_pRemovalsBarComboBox->GetLBText(nIndex,theText);
	wxASSERT(!theText.IsEmpty());

	// store the active srcPhrase's m_nWords member's value for use in the test in OnUpdateButtonUndoLastCopy()
	// and other "state-recording" information, for use by the same update handler
	gnWasNumWordsInSourcePhrase = gpApp->m_pActivePile->m_pSrcPhrase->m_nSrcWords; // the merge state at active location
	gbWasGlossingMode = gbIsGlossing; // whether glossing or adapting when the copy is done
	gbWasFreeTranslationMode = gpApp->m_bFreeTranslationMode; // whether or not free translation mode is on at that time

//	gReplacementLocation_SequNum = pApp->m_nActiveSequNum;
	gnWasSequNum = pApp->m_nActiveSequNum;

	// now put the text into m_targetBox in the view, and get the view redrawn,
	// or if freeTranslationsStep is in effect, into the edit box within the
	// ComposeBar
	gOldEditBoxTextStr.Empty();
	if (gEditStep == freeTranslationsStep || gpApp->m_bFreeTranslationMode)
	{
		wxASSERT(m_pComposeBar != NULL);
		//wxTextCtrl* pEdit = (wxTextCtrl*)m_pComposeBar->FindWindowById(IDC_EDIT_COMPOSE);
		CComposeBarEditBox* pEdit = (CComposeBarEditBox*)m_pComposeBar->FindWindowById(IDC_EDIT_COMPOSE);
		wxASSERT(pEdit != NULL);
		gOldEditBoxTextStr = pEdit->GetValue(); // in case Undo Last Copy button is clicked
		pEdit->SetValue(_T("")); // SetValue() is OK to use here
        // whm Note: SetValue() automatically (and by design) resets the dirty flag to FALSE when
        // called, because it is primarily designed to establish the initial value of an edit control.
        // Often when the initial value of a text control is programatically set, we don't want it
        // marked as "dirty". It should be marked dirty when the user changes something. But, our
        // ComposeBarEditBox is designed to echo the compose bar's contents and is does that by checking
        // for changes in the compose bar's contents (the dirty flag). Therefore, calling SetValue()
        // won't do what we want because SetValue automatically resets the dirty flag to FALSE; Instead,
        // we need to call one of the other wxTextCtrl methods that sets the dirty flag to TRUE. We
        // could use Append(), WriteText() or even just use the << operator to insert text into the
        // control. I'll use the WriteText() method, which not only sets the dirty flag to TRUE, it also
        // leaves the insertion point at the end of the inserted text. Using WriteText() also has the
        // benefit of setting the insertion point at the end of the inserted text - so we don't need to
        // call SetSelection() to do so.
		pEdit->WriteText(theText); //pEdit->SetValue(theText);
		//long len = theText.Len();
		//pEdit->SetSelection(len,len); // not needed because WriteText() does this for us
		pEdit->SetFocus();
		
		return;
	}

	// if control gets here then the copy must therefore be going to the phrase box; so
	// store a copy of phrase box text here in case Undo Last Copy button is used later
	gOldEditBoxTextStr = gpApp->m_pTargetBox->GetValue(); 

	// if auto capitalization is on, determine the source text's case propertiess
	bool bNoError = TRUE;
	if (gbAutoCaps)
	{
		bNoError = pView->SetCaseParameters(pApp->m_pActivePile->m_pSrcPhrase->m_key); // bIsSrcText is TRUE
		if (bNoError && gbSourceIsUpperCase && !gbMatchedKB_UCentry)
		{
			bNoError = pView->SetCaseParameters(theText,FALSE); // testing the non-source text
			if (bNoError && !gbNonSourceIsUpperCase && (gcharNonSrcUC != _T('\0')))
			{
				// a change to upper case is called for
				theText.SetChar(0,gcharNonSrcUC);
			}
		}
	}
	pApp->m_targetPhrase = theText;


	// the box may be bigger because of the text, so do a recalc of the layout
	pView->RecalcLayout(pApp->m_pSourcePhrases,0,pApp->m_pBundle);

	// if a phrase jumps back on to the line due to the recalc of the layout, then the
	// current location for the box will end up too far right, so we must find out where the
	// active pile now is and reset m_ptCurBoxLocation before calling CreateBox, so
	// recalculate the active pile pointer (old was clobbered by the RecalcLayout call)
	pApp->m_pActivePile = pView->GetPile(pApp->m_nActiveSequNum);
	wxASSERT(pApp->m_pActivePile != NULL);
	pApp->m_pTargetBox->m_pActivePile = pApp->m_pActivePile; // put copy in the CPhraseBox too
	pApp->m_ptCurBoxLocation = pApp->m_pActivePile->m_pCell[2]->m_ptTopLeft;

	// do a scroll if needed
	pApp->GetMainFrame()->canvas->ScrollIntoView(pApp->m_nActiveSequNum);

	// place cursor at end of the inserted text
	int length = theText.Length();
	pApp->m_nEndChar = pApp->m_nStartChar = length;
	pView->RemakePhraseBox(pApp->m_pActivePile,pApp->m_targetPhrase);

	// restore focus and make non-abandonable
	if (pApp->m_pTargetBox != NULL) // should always be the case
	{
		if (pApp->m_pTargetBox->IsShown())
		{
			pApp->m_pTargetBox->SetFocus();
			pApp->m_pTargetBox->m_bAbandonable = FALSE;
		}
	}
	pView->Invalidate(); // whm: Why call Invalidate here? (Because the text could be different
						 // length than what was there before, and hence move piles about or even
						 // cause the following strips to have different pile content. But to
						 // avoid flicker we need to make use of clipping region soon. BEW)
}

