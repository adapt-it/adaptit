/////////////////////////////////////////////////////////////////////////////
/// \project		AdaptItWX
/// \file			MainFrm.cpp
/// \author			Bill Martin
/// \date_created	05 January 2004
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public LIcense v. 1.0 AND The wxWindows Library Licence (see License.txt)
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
#include <wx/help.h> //(wxWidgets chooses the appropriate help controller class)
//#include <wx/helpbase.h> //(wxHelpControllerBase class)
//#include <wx/helpwin.h> //(Windows Help controller)
//#include <wx/msw/helpchm.h> //(MS HTML Help controller)
//#include <wx/generic/helpext.h> //(external HTML browser controller)
//#include <wx/html/helpctrl.h> //(wxHTML based help controller: wxHtmlHelpController)

#include <wx/docview.h>
#include <wx/filename.h>
#include <wx/tooltip.h>
#include <wx/config.h> // for wxConfig

#ifdef __WXMSW__
#include <wx/msw/registry.h> // for wxRegKey - used in SantaFeFocus sync scrolling mechanism
#endif

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
// includes above

// next two are for version 2.0 which includes the option of a 3rd line for glossing

/// This global is defined in Adapt_ItView.cpp.
extern bool	gbIsGlossing; // when TRUE, the phrase box and its line have glossing text

/// This global is defined in Adapt_ItView.cpp.
extern bool gbGlossingUsesNavFont;

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
extern bool		gbSourceIsUpperCase;

/// This global is defined in Adapt_It.cpp.
extern bool		gbAutoCaps;

/// This global is defined in Adapt_It.cpp.
extern bool		gbNonSourceIsUpperCase;

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
extern  int		nCurrentSequNum;

/// This global is defined in Adapt_It.cpp.
extern  int		nSequNumForLastAutoSave;

//extern wxHtmlHelpController* m_pHelpController;

/// This global is defined in Adapt_It.cpp.
extern wxHelpController* m_pHelpController;

#ifdef __WXMSW__
static UINT NEAR WM_SANTA_FE_FOCUS = RegisterWindowMessage(_T("SantaFeFocus"));
#endif

IMPLEMENT_CLASS(CMainFrame, wxDocParentFrame)

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
#ifdef __WXDEBUG__
	m_bShowScrollData = TRUE; // shows scroll parameters and client size in status bar
#else
	m_bShowScrollData = FALSE;// does not show scroll parameters and client size in status bar
#endif

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
	m_pComposeBarEditBox = (wxTextCtrl*) NULL;	// handle/pointer to the composeBar's edit box

	m_toolBarHeight = 0;		// determined in CMainFrame constructor after toolBar is created
	m_controlBarHeight = 0;		// determined in CMainFrame constructor after controlBar is created
	m_composeBarHeight = 0;		// determined in CMainFrame constructor after composeBar is created
	m_statusBarHeight = 0;


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
	// layout within the Main Frame. The controlBar is not visible by default
	// but can be toggled on from the view menu or when it takes on the form
	// of the Free Translation compose bar in Free Translation mode.
	// Here and in the OnSize() method, we calculate the canvas' client
	// size, which also must exclude the height of the composeBar.
	// Get and save the native height of our composeBar.
	wxSize composeBarSize;
	composeBarSize = m_pComposeBar->GetSize();
	m_composeBarHeight = composeBarSize.GetHeight();

	m_pComposeBarEditBox = (wxTextCtrl*)FindWindowById(IDC_EDIT_COMPOSE);
	wxASSERT(m_pComposeBarEditBox != NULL);
	// The initial state of the composeBar, however is hidden. The OnViewComposeBar()
	// method takes care of hiding and showing the composeBar and OnSize()insures the
	// client window is adjusted accordingly and the screen redrawn as needed
	m_pComposeBar->Hide();

	// BEW added 27Mar07 for support of receiving synchronized scroll messages
	// whm note: in the MFC version the following is located in CMainFrame::OnCreate()
	if (gpDocList == NULL)
	{
		// create the SPList, so it can be used if the user turns on message receiving
		gpDocList = new SPList;
	}
	
 	this->canvas = CreateCanvas(this);
	// now that canvas is created, set canvas' pointer to this main frame
	canvas->pFrame = this;

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
	// To Change version number, date, etc., use wxDesigner to modify them in AboutDlgFunc().
	// First parameter is the parent which is usually 'this'.
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	// The declaration is: AboutDlgFunc( wxWindow *parent, bool call_fit, bool set_sizer ).
	CAdapt_ItApp* pApp = &wxGetApp();
	wxString appCreateDate,appModDate;
	if (wxFileExists(pApp->m_executingAppPathName))
	{
		wxFileName fn(pApp->m_executingAppPathName);
		wxDateTime dtModified,dtCreated;
		if (fn.GetTimes(NULL,&dtModified,&dtCreated))
		{
			appCreateDate = dtCreated.FormatISODate();
			appModDate = dtModified.FormatISODate();
		}
	}
	if (!appModDate.IsEmpty())
	{
		wxStaticText* pStatic = (wxStaticText*)FindWindowById(ID_ABOUT_VERSION_DATE);
		pStatic->SetLabel(appModDate);
	}

	// Create the version number from the defines in Adapt_It.h:
	//#define VERSION_MAJOR_PART 4
	//#define VERSION_MINOR_PART 0
	//#define VERSION_BUILD_PART 0
	wxString strVersionNumber;
	strVersionNumber.Empty();
	strVersionNumber << VERSION_MAJOR_PART;
	strVersionNumber += _T(".");
	strVersionNumber << VERSION_MINOR_PART;
	strVersionNumber += _T(".");
	strVersionNumber << VERSION_BUILD_PART;
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
		tempStr = pApp->m_systemEncodingName;
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
				return;
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
	bool bOK;
	bOK = m_pHelpController->DisplayContents();
	if (!bOK)
	{
		wxString strMsg;
		strMsg = strMsg.Format(_T("Adapt It could not find its help file.\nThe name and location of help file it looked for:\n %s\nTo insure that help is available, this help file must be installed with Adapt It."),gpApp->m_helpPathName.c_str());
		wxMessageBox(strMsg, _T(""), wxICON_WARNING);
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

// Creat a canvas for the main frame.
CAdapt_ItCanvas *CMainFrame::CreateCanvas(CMainFrame *parent)
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
	// CMainFrame is Adapt It's primary application frame or window. The CMainFrame
	// is the parent frame for everything that happens in Adapt It, including all
	// first level dialogs. CMainFrame's toolbar and statusbar are created by
	// wxFrame's CreateToolBar() and CreateStatusBar() methods, and so CMainFrame
	// knows how to manage these windows, adjusting the value returned by GetClientSize
	// to reflect the remaining size available to application windows. 
	//
	// Adapt It has three other windows to be managed within CMainFrame. These are
	// the controlbar, the composebar and the actual canvas which the user knows as
	// Adapt It's "main window." These windows are managed by a mainFrameSizer, which
	// takes care of insuring they fill their appropriate places within the main frame
	// and automatically resize them when the controlbar or composebar are made visible
	// or invisible (when one or both of these are made invisible, the sizer 
	// automatically adjusts the canvas to fill the remaining space in the main frame.
	//
	// wxWidgets' cross-platform library utilizes sizers which can be used to manage 
	// the appearance of all the non-standard windows in Adapt It's CMainFrame
	// (including the controlbar, the composebar, and the canvas window which fills
	// all remaining space within the main frame). All three of these are created in
	// the CMainFrame's constructor. The control bar and the compose bar simply
	// remain hidden until made to appear by menu command. When the control bar
	// and/or compose bar are shown/hidden, a call to Layout, adjusts the frame's
	// layout within the mainSizer. By placing these windows in the mainSizer and
	// calling SetAutoLayout(TRUE), and SetSizer(mainFrameSizer) within CMainFrame's
	// constructor, we get automatic control of the windows' sizes.

	//Layout();	// causes the layout of controlBar, composeBar (if shown) and the canvas
				// within mainSizer to be adjusted according to the new size of the main
				// frame.

	// OnSize() is called the first time before the Main Frame constructor is finished and
	// before the controlBar, composeBar, and canvas are created, so check if these are null
	// and return from OnSize() if they do not exist yet.
	if (m_pControlBar == NULL || m_pComposeBar == NULL || canvas == NULL)
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
	// hidden below the bottom of the main frame). Although using a mainFrameSize would
	// probably work OK to manage the controlBar, composeBat and canvas within the main
	// frame, especially once GetClientSize() is obtained from the main frame rather than
	// from the canvas itself, I've opted to place those windows within the main frame
	// manually (apart from using mainFrameSizer) so as to be able to better determine 
	// the source of other scrolling problems.

	// First, set the position of the controlBar in the main frame's client area.
	wxSize mainFrameClientSize;
	mainFrameClientSize = GetClientSize(); // determine the size of the main frame's client window
	// The upper left position of the main frame's client area is always 0,0 so we position
	// the controlBar there and use the main frame's client size width and maintain the existing
	// controlBar's height (note: SetSize sets both the position and the size).
	m_pControlBar->SetSize(0, 0, mainFrameClientSize.x, m_controlBarHeight);
	m_pControlBar->Refresh(); // this is needed to repaint the contorlBar after OnSize

	// Finally, adjust the composeBar's position in the main frame (if composeBar is visible), and
	// set the canvas' position and size
	if (m_pComposeBar->IsShown())
	{
		// composeBar is showing, set its position (with existing size) just below the controlBar
		m_pComposeBar->SetSize(0, m_controlBarHeight, mainFrameClientSize.x, m_composeBarHeight);
		m_pComposeBar->Refresh();
		// now fill up remaining part of main frame's client area with the canvas
		// we set the upper left coord just below the controlBar and the height to
		// fill the remaining vertical space within the main frame's client area
		canvas->SetSize(0, m_controlBarHeight + m_composeBarHeight, 
			mainFrameClientSize.x, mainFrameClientSize.y - m_controlBarHeight - m_composeBarHeight);
	}
	else
	{
		// the composeBar is not showing, so just position the canvas to fill up the remaining 
		// part of the main frame's client area below the controlBar
		canvas->SetSize(0, m_controlBarHeight, mainFrameClientSize.x, 
			mainFrameClientSize.y - m_controlBarHeight); // sets the canvas just below the controlBar
	}

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
		// toggle the compose bar's flag and its visibility
		if (m_pComposeBar->IsShown())
		{
			// Hide it
			m_pComposeBar->Hide();
			GetMenuBar()->Check(ID_VIEW_COMPOSE_BAR, FALSE);
			gpApp->m_bComposeWndVisible = FALSE; // needed???
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
			event.RequestMore();
		}
		//return TRUE; // enable next OnIdle call - this accomplished with event.RequestMore()
	//case 3: // auto capitalization support for the typed string in the phrasebox - we
			// poll the box and change the case if necessary

		if (pApp->m_pActivePile == NULL || pApp->m_nActiveSequNum == -1)
		{
			event.RequestMore();
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
						event.RequestMore(); // added
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
		event.RequestMore(); // added

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
					event.RequestMore(); // added
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
					event.RequestMore(); // added
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
		event.RequestMore(); // added

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
				event.RequestMore(); // added
			}
		}
		else
		{
			//return TRUE; // enable more idle messages
			event.RequestMore(); // added
		}

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
		//TODO:
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
				event.RequestMore(); // added
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
			event.RequestMore(); // added
		}
	//}
	//event.Skip();
}
