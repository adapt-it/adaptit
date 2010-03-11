/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			Adapt_ItDoc.cpp
/// \author			Bill Martin
/// \date_created	05 January 2004
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the CAdapt_ItDoc class. 
/// The CAdapt_ItDoc class implements the storage structures and methods 
/// for the Adapt It application's persistent data. Adapt It's document 
/// consists mainly of a list of CSourcePhrases stored in order of occurrence 
/// of source text words. The document's data structures are kept logically 
/// separate from and independent of the view class's in-memory data structures. 
/// This schema is an implementation of the document/view framework. 
/// \derivation		The CAdapt_ItDoc class is derived from wxDocument.
/////////////////////////////////////////////////////////////////////////////
// Pending Implementation Items in MainFrm (in order of importance): (search for "TODO")
// 1.
// Unanswered questions: (search for "???")
// 1. 
// 
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "Adapt_ItDoc.h"
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

#include <wx/docview.h>	// includes wxWidgets doc/view framework
#include "Adapt_ItCanvas.h"
#include "Adapt_It_Resources.h"
#include <wx/filesys.h>
#include <wx/file.h>
#include <wx/wfstream.h>
#include <wx/zipstrm.h> // for wxZipInputStream & wxZipOutputStream
#include <wx/datstrm.h> // permanent
#include <wx/txtstrm.h> // temporary
#include <wx/mstream.h> // for wxMemoryInputStream
#include <wx/font.h> // temporary
#include <wx/fontmap.h> // temporary
#include <wx/fontenum.h> // temporary
#include <wx/list.h>
#include <wx/tokenzr.h>
#include <wx/progdlg.h>
#include <wx/busyinfo.h>

#if !defined(__APPLE__)
#include <malloc.h>
#else
#include <malloc/malloc.h>
#endif

// The following are from IBM's International Components for Unicode (icu) used under the LGPL license.
//#include "csdetect.h" // used in GetNewFile(). 
//#include "csmatch.h" // " "

// Other includes uncomment as implemented
#include "Adapt_It.h"
#include "OutputFilenameDlg.h"
#include "helpers.h"
#include "MainFrm.h"
#include "SourcePhrase.h"
#include "KB.h"
#include "AdaptitConstants.h"
#include "TargetUnit.h"
#include "Adapt_ItView.h"
#include "Strip.h"
#include "Pile.h" // must precede the include for the document
#include "Cell.h"
#include "Adapt_ItDoc.h"
#include "RefString.h"
//#include "ProgressDlg.h" // removed in svn revision #562
#include "WaitDlg.h"
#include "XML.h"
#include "MoveDialog.h"
#include "SplitDialog.h"
#include "JoinDialog.h"
#include "UnpackWarningDlg.h"
#include "Layout.h"
#include "ExportFunctions.h"
#include "ReadOnlyProtection.h"

// forward declarations for functions called in tellenc.cpp
// GDLC Temporary work around for PPC STL library bug
#if defined(__WXMAC__) && defined(__POWERPC__ )
// tellenc() not used in PPC builds pending bug fix in PPC STL
#else
void init_utf8_char_table();
const char* tellenc(const char* const buffer, const size_t len);
#endif

/// This global is defined in Adapt_ItView.cpp.
extern bool gbVerticalEditInProgress;

/// This global is defined in Adapt_ItView.cpp.
extern EditRecord gEditRecord; // defined at start of Adapt_ItView.cpp

/// This global is defined in Adapt_It.cpp.
extern enum TextType gPreviousTextType; // moved to global space in the App, made extern here

// Other declarations from MFC version below

/// Length of the byte-order-mark (BOM) which consists of the three bytes 0xEF, 0xBB and 0xBF
/// in UTF-8 encoding.
#define nBOMLen 3

/// Length of the byte-order-mark (BOM) which consists of the two bytes 0xFF and 0xFE in
/// in UTF-16 encoding.
#define nU16BOMLen 2

#ifdef _UNICODE

/// The UTF-8 byte-order-mark (BOM) consists of the three bytes 0xEF, 0xBB and 0xBF
/// in UTF-8 encoding. Some applications like Notepad prefix UTF-8 files with
/// this BOM.
static wxUint8 szBOM[nBOMLen] = {0xEF, 0xBB, 0xBF}; // MFC uses BYTE

/// The UTF-16 byte-order-mark (BOM) which consists of the two bytes 0xFF and 0xFE in
/// in UTF-16 encoding.
static wxUint8 szU16BOM[nU16BOMLen] = {0xFF, 0xFE}; // MFC uses BYTE

#endif

/// This global is defined in Adapt_ItView.cpp. 
extern int	gnBeginInsertionsSequNum;	

/// This global is defined in Adapt_ItView.cpp. 
extern int	gnEndInsertionsSequNum;	

/// This global boolean informs the Doc's BackupDocument() function whether a split or 
/// join operation is in progress. If gbDoingSplitOrJoin is TRUE BackupDocument() exits 
/// immediately without performing any backup operations. Split operations especially 
/// could produce a plethora of backup docs, especially for a single-chapters document split.
bool gbDoingSplitOrJoin = FALSE; // TRUE during one of these 3 operations

/// This global is defined in DocPage.cpp. 
extern bool gbMismatchedBookCode; // BEW added 21Mar07

/// This global is defined in Adapt_It.cpp.
extern bool	gbTryingMRUOpen; // see Adapt_It.cpp globals list for an explanation

/// This global is defined in MainFrm.cpp.
extern bool gbIgnoreScriptureReference_Receive;

/// This global is defined in MainFrm.cpp.
extern bool gbIgnoreScriptureReference_Send;

/// This global is used only in RetokenizeText() to increment the number n associated with
/// the final number n composing the "Rebuild Logn.txt" files, which inform the user of
/// any problems encountered during document rebuilding.
int gnFileNumber = 0; // used for output of Rebuild Logn.txt file, to increment n each time

/// This global is defined in TransferMarkersDlg.cpp.
extern bool gbPropagationNeeded;

/// This global is defined in TransferMarkersDlg.cpp.
extern TextType gPropagationType;

// This global is defined in Adapt_ItView.cpp.  BEW removed 27Jan09
extern bool gbInhibitLine4StrCall; // see view for reason for this

/// This global is defined in Adapt_ItView.cpp.
extern bool gbIsUnstructuredData;

// next four are for version 2.0 which includes the option of a 3rd line for glossing

/// This global is defined in Adapt_ItView.cpp.
extern bool	gbIsGlossing; // when TRUE, the phrase box and its line have glossing text

/// This global is defined in Adapt_ItView.cpp.
extern bool	gbEnableGlossing; // TRUE makes Adapt It revert to Shoebox functionality only

/// This global is defined in Adapt_ItView.cpp.
extern bool gbGlossingUsesNavFont;

/// This global is defined in DocPage.cpp.
extern bool  gbForceUTF8; // defined in CDocPage

/// This global is defined in Adapt_It.cpp.
extern wxChar gSFescapechar; // the escape char used for start of a standard format marker

/// This global is defined in Adapt_It.cpp.
extern bool  gbSfmOnlyAfterNewlines;

/// This global is defined in Adapt_It.cpp.
extern bool  gbDoingInitialSetup;

/// This global is defined in Adapt_It.cpp.
extern CAdapt_ItApp* gpApp; // if we want to access it fast

/// Indicates if a source word or phrase is to be considered special text when the propagation
/// of text attributes needs to be considered as after editing the source text or after rebuilding
/// the source text subsequent to filtering changes. Normally used to set or store the m_bSpecialText 
/// attribute of a source phrase instance.
bool   gbSpecialText = FALSE;

/// This global is defined in Adapt_ItView.cpp.
extern CSourcePhrase* gpPrecSrcPhrase; 

/// This global is defined in Adapt_ItView.cpp.
extern CSourcePhrase* gpFollSrcPhrase;

/// This global is defined in Adapt_ItView.cpp.
extern	bool	gbShowTargetOnly;

/// This global is defined in Adapt_ItView.cpp.
extern	int		gnSaveLeading;

/// This global is defined in Adapt_ItView.cpp.
extern	int		gnSaveGap;

/// This global is defined in PhraseBox.cpp.
extern	wxString	translation;

/// Indicates if the user has cancelled an operation.
bool	bUserCancelled = FALSE;

/// This global is defined in Adapt_It.cpp.
extern	bool	gbViaMostRecentFileList;

/// This global is defined in Adapt_ItView.cpp.
extern	bool	gbConsistencyCheckCurrent;

/// This global is defined in Adapt_ItView.cpp.
extern	int		gnOldSequNum;

/// This global is defined in Adapt_It.cpp.
extern	bool	gbAbortMRUOpen;

/// This global is defined in Adapt_It.cpp.
extern bool		gbPassedAppInitialization;

/// This global is defined in Adapt_It.cpp.
extern wxString szProjectConfiguration;

/// This global is defined in Adapt_It.cpp.
extern wxString szAdminProjectConfiguration;

/// This global is defined in Adapt_It.cpp.
extern bool gbHackedDataCharWarningGiven;

// support for USFM and SFM Filtering
// Since these special filter markers will be visible to the user in certain
// dialog controls such as the CTransferMarkersDlg dialog, I've opted to use 
// marker labels that should be unique (starting with \~) and yet still 
// recognizable by containing the word 'FILTER' as part of their names.

/// A marker string used to signal the beginning of filtered material stored in a source phrase's
/// m_markers member.
const wxChar* filterMkr = _T("\\~FILTER");

/// A marker string used to signal the end of filtered material stored in a source phrase's
/// m_markers member.
const wxChar* filterMkrEnd = _T("\\~FILTER*");

/////////////////////////////////////////////////////////////////////////////
// CAdapt_ItDoc

IMPLEMENT_DYNAMIC_CLASS(CAdapt_ItDoc, wxDocument)

BEGIN_EVENT_TABLE(CAdapt_ItDoc, wxDocument)

	// The events that are normally handled by the doc/view framework use predefined
	// event identifiers, i.e., wxID_NEW, wxID_SAVE, wxID_CLOSE, wxID_OPEN, etc. 
	EVT_MENU(wxID_NEW, CAdapt_ItDoc::OnFileNew)
	EVT_MENU(wxID_SAVE, CAdapt_ItDoc::OnFileSave)
	EVT_UPDATE_UI(wxID_SAVE, CAdapt_ItDoc::OnUpdateFileSave)
	EVT_MENU(wxID_CLOSE, CAdapt_ItDoc::OnFileClose)
	EVT_UPDATE_UI(wxID_CLOSE, CAdapt_ItDoc::OnUpdateFileClose)
	EVT_MENU(wxID_OPEN, CAdapt_ItDoc::OnFileOpen)
	EVT_MENU(ID_TOOLS_SPLIT_DOC, CAdapt_ItDoc::OnSplitDocument)
	EVT_UPDATE_UI(ID_TOOLS_SPLIT_DOC, CAdapt_ItDoc::OnUpdateSplitDocument)
	EVT_MENU(ID_TOOLS_JOIN_DOCS, CAdapt_ItDoc::OnJoinDocuments)
	EVT_UPDATE_UI(ID_TOOLS_JOIN_DOCS, CAdapt_ItDoc::OnUpdateJoinDocuments)
	EVT_MENU(ID_TOOLS_MOVE_DOC, CAdapt_ItDoc::OnMoveDocument)
	EVT_UPDATE_UI(ID_TOOLS_MOVE_DOC, CAdapt_ItDoc::OnUpdateMoveDocument)
	EVT_UPDATE_UI(ID_FILE_PACK_DOC, CAdapt_ItDoc::OnUpdateFilePackDoc)
	EVT_UPDATE_UI(ID_FILE_UNPACK_DOC, CAdapt_ItDoc::OnUpdateFileUnpackDoc)
	EVT_MENU(ID_FILE_PACK_DOC, CAdapt_ItDoc::OnFilePackDoc)
	EVT_MENU(ID_FILE_UNPACK_DOC, CAdapt_ItDoc::OnFileUnpackDoc)
	EVT_MENU(ID_ADVANCED_RECEIVESYNCHRONIZEDSCROLLINGMESSAGES, CAdapt_ItDoc::OnAdvancedReceiveSynchronizedScrollingMessages)
	EVT_UPDATE_UI(ID_ADVANCED_RECEIVESYNCHRONIZEDSCROLLINGMESSAGES, CAdapt_ItDoc::OnUpdateAdvancedReceiveSynchronizedScrollingMessages)
	EVT_MENU(ID_ADVANCED_SENDSYNCHRONIZEDSCROLLINGMESSAGES, CAdapt_ItDoc::OnAdvancedSendSynchronizedScrollingMessages)
	EVT_UPDATE_UI(ID_ADVANCED_SENDSYNCHRONIZEDSCROLLINGMESSAGES, CAdapt_ItDoc::OnUpdateAdvancedSendSynchronizedScrollingMessages)
END_EVENT_TABLE()

/////////////////////////////////////////////////////////////////////////////
// CAdapt_ItDoc construction/destruction

/// **** DO NOT PUT INITIALIZATIONS IN THE DOCUMENT'S CONSTRUCTOR *****
/// **** ALL INITIALIZATIONS SHOULD BE DONE IN THE APP'S OnInit() METHOD *****
CAdapt_ItDoc::CAdapt_ItDoc()
{
	// WX Note: All Doc constructor initializations moved to the App
	// **** DO NOT PUT INITIALIZATIONS HERE IN THE DOCUMENT'S CONSTRUCTOR *****
	// **** ALL INITIALIZATIONS SHOULD BE DONE IN THE APP (OnInit) ************
}


/// **** ALL CLEANUP SHOULD BE DONE IN THE APP'S OnExit() METHOD ****
CAdapt_ItDoc::~CAdapt_ItDoc() // from MFC version
{
	// **** ALL CLEANUP SHOULD BE DONE IN THE APP'S OnExit() METHOD ****
}

///////////////////////////////////////////////////////////////////////////////
/// \return TRUE if new document was created successfully, FALSE otherwise
/// \remarks
/// Called from: the DocPage's OnWizardFinish() function.
/// In OnNewDocument, we aren't creating the document via serialization
/// of data from persistent storage (as does OnOpenDocument()), rather 
/// we are creating the new document from scratch, by doing the following:
/// 1. Making sure our working directory is set properly.
/// 2. Calling parts of the virtual base class wxDocument::OnNewDocument() method
/// 3. Create the buffer and list structures that will hold our data
/// 4. Providing KB structures are ready, call GetNewFile() to get
///    the sfm file for import into our app.
/// 5. Get an output file name from the user.
/// 6. Tidy up the frame's window title.
/// 7. Create/Recreate the list of paired source and target punctuation
///    correspondences, updating also the View's punctuation settings
/// 8. Remove any Ventura Publisher optional hyphens from the text buffer.
/// 9. Call TokenizeText, which separates the text into words, stores them
///    in m_pSourcePhrases list and returns the number
/// 10. Calculate the App's text heights, and get the View to calculate
///     its initial indices and do its RecalcLayout()
/// 11. Show/place the initial phrasebox at first empty target slot
/// 12. Keep track of sequence numbers and set initial global src phrase
///     node position.
/// 13. [added] call OnInitialUpdate() which needs to be called before the
///     view is shown.
/// BEW added 13Nov09: call of m_bReadOnlyAccess = SetReadOnlyProtection(), in order to give
/// the local user in the project ownership for writing permission (if FALSE is returned)
/// or READ-ONLY access (if TRUE is returned). (Also added to LoadKB() and OnOpenDocument()
/// and OnCreate() for the view class.)
///////////////////////////////////////////////////////////////////////////////
bool CAdapt_ItDoc::OnNewDocument()
// ammended for support of glossing or adapting
{
	// refactored 10Mar09
	CAdapt_ItApp* pApp = GetApp();
	pApp->m_nSaveActiveSequNum = 0; // reset to a default initial value, safe for any length of doc 

	gnBeginInsertionsSequNum = -1; // reset for "no current insertions"
	gnEndInsertionsSequNum = -1; // reset for "no current insertions"

 	// get a pointer to the view
	CAdapt_ItView* pView = (CAdapt_ItView*) pApp->GetView();
	wxASSERT(pView->IsKindOf(CLASSINFO(CAdapt_ItView)));

    // BEW comment 6May09 -- this OnInitialUpdate() call contains a RecalcLayout() call
    // within its call of OnSize() call. If the RecalcLayout() call is late enough, it can
    // try the recalculation while piles do not exist, leading to a crash. So I've moved
    // OnInitialUpdate() to be early in OnNewDocument() and protected from a crash by
    // having the recalculation do nothing at all with the layout until all the layout
    // components are in place... (moved here from end of function)
	// 
	// whm added OnInitialUpdate(), since in WX the doc/view framework doesn't call it 
	// automatically we need to call it manually here. MFC calls its OnInitialUpdate()
	// method sometime after exiting its OnNewDocument() and before showing the View. See
	// Notes at OnInitialUpdate() for more info.
	pView->OnInitialUpdate(); // need to call it here because wx's doc/view doesn't 
								// automatically call it
	// ensure that the current work folder is the project one for default
	wxString dirPath = pApp->m_workFolderPath;
	bool bOK;
	if (pApp->m_lastSourceFileFolder.IsEmpty())
		bOK = ::wxSetWorkingDirectory(dirPath);
	else
		bOK = ::wxSetWorkingDirectory(pApp->m_lastSourceFileFolder);

	// the above may have failed, so if so use m_workFolderPath as the folder, 
	// or even the C: drive top level, so we can proceed to the file dialog safely
	// whm Note: TODO: The following block needs to be made cross-platform friendly
	if (!bOK)
	{
		pApp->m_lastSourceFileFolder = dirPath;
		bOK = ::wxSetWorkingDirectory(dirPath); // this should work, since m_workFolderPath can hardly 
												// be wrong!
		if (!bOK)
		{
			bOK = ::wxSetWorkingDirectory(_T("C:"));
			if (!bOK)
			{
				// we should never get a failure for the above, so just an English message will do
				wxMessageBox(_T(
				"OnNewDocument() failed, when setting current directory to C drive"),
				_T(""), wxICON_ERROR);
				return FALSE;
			}
		}
	}

	//if (!wxDocument::OnNewDocument()) // don't use this because it calls OnCloseDocument()
	//	return FALSE;
	// whm NOTES: The wxWidgets base class OnNewDocument() calls OnCloseDocument()
	// which fouls up the KB structures due to the OnCloseDocument() calls to
	// EraseKB(), etc. To get around this problem which arises because of 
	// different calling orders in the two doc/view frameworks, we'll not 
	// call the base class wxDocument::OnNewDocument() method in wxWidgets,
	// but instead we call the remainder of its contents here:
	// whm verified the need for this 20July2006
     DeleteContents();
     Modify(FALSE);
     SetDocumentSaved(FALSE);
     wxString name;
     GetDocumentManager()->MakeDefaultName(name);
     SetTitle(name);
     SetFilename(name, TRUE);
	 // above calls come from wxDocument::OnNewDocument()
	 // Note: The OnSaveModified() call is handled when needed in 
	 // the Doc's Close() and/or OnOpenDocument()

	// (SDI documents will reuse this document)
	if (pApp->m_pBuffer != 0)
	{
		delete pApp->m_pBuffer; // make sure wxString is not in existence
		pApp->m_pBuffer = (wxString*)NULL; // MFC had = 0
	}

	// BEW added 21Apr08; clean out the global struct gEditRecord & clear its deletion lists,
	// because each document, on opening it, it must start with a truly empty EditRecord; and
	// on doc closure and app closure, it likewise must be cleaned out entirely (the deletion
	// lists in it have content which persists only for the life of the document currently open)
	pView->InitializeEditRecord(gEditRecord);
	if (!gEditRecord.deletedAdaptationsList.IsEmpty())
		gEditRecord.deletedAdaptationsList.Clear(); // remove any stored deleted adaptation strings
	if (!gEditRecord.deletedGlossesList.IsEmpty())
		gEditRecord.deletedGlossesList.Clear(); // remove any stored deleted gloss strings
	if (!gEditRecord.deletedFreeTranslationsList.IsEmpty())
		gEditRecord.deletedFreeTranslationsList.Clear(); // remove any stored deleted free translations


	int width = wxSystemSettings::GetMetric(wxSYS_SCREEN_X);
#ifdef _RTL_FLAGS
	pApp->m_docSize = wxSize(width - 40,600); // a safe default width, the length doesn't matter 
											  // (it will change shortly)
#else
	pApp->m_docSize = wxSize(width - 80,600); // ditto
#endif

	// need a SPList to store the source phrases
	if (pApp->m_pSourcePhrases == NULL)
		pApp->m_pSourcePhrases = new SPList;
	wxASSERT(pApp->m_pSourcePhrases != NULL);


	bool bKBReady = FALSE;
	if (gbIsGlossing)
		bKBReady = pApp->m_bGlossingKBReady;
	else
		bKBReady = pApp->m_bKBReady;
	if (bKBReady)
	{
		pApp->m_nActiveSequNum = -1; // default, till positive value on layout of file

		pApp->m_pBuffer = new wxString; // on the heap, because this could be a large block of source text
		wxASSERT(pApp->m_pBuffer != NULL);
		pApp->m_nInputFileLength = 0;
		wxString filter = _T("*.*");
		wxString fileTitle = _T(""); // stores name (minus extension) of user's chosen source file

		// The following wxFileDialog part was originally in GetNewFile(), but moved here 19Jun09 to
		// consolidate file error message processing.
		wxString defaultDir;
		if (gpApp->m_lastSourceFileFolder.IsEmpty())
		{
			defaultDir = gpApp->m_workFolderPath;
		}
		else
		{
			defaultDir = gpApp->m_lastSourceFileFolder;
		}

		wxFileDialog fileDlg(
			(wxWindow*)wxGetApp().GetMainFrame(), // MainFrame is parent window for file dialog
			_("Input Text File For Adaptation"),
			defaultDir,	// default dir (either m_workFolderPath, or m_lastSourceFileFolder)
			_T(""),		// default filename
			filter,
		wxFD_OPEN); // | wxHIDE_READONLY); wxHIDE_READONLY deprecated in 2.6 - the checkbox is never shown
					// GDLC wxOPEN deprecated in 2.8
		fileDlg.Centre();
		// open as modal dialog
		int returnValue = fileDlg.ShowModal(); // MFC has DoModal()
		if (returnValue == wxID_CANCEL)
		{
			// user cancelled, so cancel the New... command
			// IDS_USER_CANCELLED
			wxMessageBox(_(
"Adapt It cannot do any useful work unless you select a source file to adapt. Please try again."),
			_T(""), wxICON_INFORMATION);

			// check if there was a document current, and if so, reinitialize everything
			if (pView != 0)
			{
				pApp->m_pTargetBox->SetValue(_T(""));
				delete pApp->m_pBuffer;
				pApp->m_pBuffer = (wxString*)NULL; // MFC had = 0
				pView->Invalidate();
				// BEW added next line 16Nov09 for following reason:
                // the flag is set TRUE in OnFileNew(), before handing control over to
                // wxWidgets app class's OnFileNew(), and eventually view's OnCreate() is
                // called -- this is okay if the user chooses a file to use for creating a
                // new document, but if he cancels out of the file dialog, the cancel
                // block's code (which is right here) didn't clear bUserSelectedFileNew to
                // FALSE, which lead to an error, so we here do this fix
				pApp->bUserSelectedFileNew = FALSE;	
				GetLayout()->PlaceBox();
			}
			return FALSE;
		}
		else // must be wxID_OK 
		{
			wxString pathName;
			pathName = fileDlg.GetPath(); //MFC's GetPathName() and wxFileDialog.GetPath both get whole dir + file name.
			fileTitle = fileDlg.GetFilename(); // just the file name

			wxFileName fn(pathName);
			wxString fnExtensionOnly = fn.GetExt(); // GetExt() returns the extension NOT including the dot

            // get the file, and it's length (which includes null termination byte/s) whm
            // modified 18Jun09 GetNewFile() now returns an enum getNewFileState (see
            // Adapt_It.h) which more specifically reports the success or error state
            // encountered in getting the file for input. It now uses a switch() structure.
			switch(GetNewFile(pApp->m_pBuffer,pApp->m_nInputFileLength,pathName))
			{
			case getNewFile_success:
			{
				wxString tempSelectedFullPath = fileDlg.GetPath();

				// wxFileDialog.GetPath() returns the full path with directory and filename. We
				// only want the path part, so we also call ::wxPathOnly() on the full path to
				// get only the directory part.
				gpApp->m_lastSourceFileFolder = ::wxPathOnly(tempSelectedFullPath);
		
                // Check if it has an \id line. If it does, get the 3-letter book code. If
                // a valid code is present, check that it is a match for the currently
                // active book folder. If it isn't tell the user and abort the <New
                // Document> operation, leaving control in the Document page of the wizard
                // for a new attempt with a different source text file, or a change of book
                // folder to be done and the same file reattempted after that. If it is a
                // matching book code, continue with setting up the new document.
				if (gpApp->m_bBookMode && !gpApp->m_bDisableBookMode)
				{
					// do the test only if Book Mode is turned on
					wxString strIDMarker = _T("\\id");
					int pos = (*gpApp->m_pBuffer).Find(strIDMarker);
					if ( pos != -1)
					{
						// the marker is in the file, so we need to go ahead with the check, but first
						// work out what the current book folder is and then what its associated code is
						wxString aBookCode = ((BookNamePair*)(*gpApp->m_pBibleBooks)[gpApp->m_nBookIndex])->bookCode;
						wxString seeNameStr = ((BookNamePair*)(*gpApp->m_pBibleBooks)[gpApp->m_nBookIndex])->seeName;
						gbMismatchedBookCode = FALSE;

						// get the code by advancing over the white space after the \id marker, and then taking
						// the next 3 characters as the code
						const wxChar* pStr = gpApp->m_pBuffer->GetData();
						wxChar* ptr = (wxChar*)pStr;
						ptr += pos;
						ptr += 4; // advance beyond \id and whatever white space character is next
						while (*ptr == _T(' ') || *ptr == _T('\n') || *ptr == _T('\r') || *ptr == _T('\t'))
						{
							// advance over any additional space, newline, carriage return or tab
							ptr++;
						}
						wxString theCode(ptr,3);	// make a 3-letter code, but it may be rubbish as we can't be
													// sure there is actually a valid one there

						// test to see if the string contains a valid 3-letter code
						bool bMatchedBookCode = CheckBibleBookCode(gpApp->m_pBibleBooks, theCode);
						if (bMatchedBookCode)
						{
							// it matches one of the 67 codes known to Adapt It, so we need to check if it
							// is the correct code for the active folder; if it's not, tell the user and
							// go back to the Documents page of the wizard; if it is, just let processing 
							// continue (the Title of a message box is just "Adapt It", only Palm OS permits naming)
							if (theCode != aBookCode)
							{
								// the codes are different, so the document does not belong in the active folder
								wxString aTitle;
								// IDS_INVALID_DATA_BOX_TITLE
								aTitle = _("Invalid Data For Current Book Folder");
								wxString msg1;
								// IDS_WRONG_THREELETTER_CODE_A
								msg1 = msg1.Format(_(
"The source text file's \\id line contains the 3-letter code %s which does not match the 3-letter \ncode required for storing the document in the currently active %s book folder.\n"),
												theCode.c_str(),seeNameStr.c_str());
								wxString msg2;
								//IDS_WRONG_THREELETTER_CODE_B
								msg2 = _(
"\nChange to the correct book folder and try again, or try inputting a different source text file \nwhich contains the correct code.");
								msg1 += msg2; // concatenate the messages
								wxMessageBox(msg1,aTitle, wxICON_WARNING); // I want a title on this other than "Adapt It"
								gbMismatchedBookCode = TRUE;// tell the caller about the mismatch

								return FALSE; // returns to OnWizardFinish() in DocPage.cpp
							}
						}
						else
						{
							// not a known code, so we'll assume we accessed random text after the \id marker,
							// and so we just let processing proceed & the user can live with whatever happens
							;
						}
					}
					else
					{
						// if the \id marker is not in the source text file, then it is up to the user
						// to keep the wrong data from being stored in the current book folder, so all
						// we can do for that situation is to let processing proceed
						;
					}
				}

				// get a suitable output filename for use with the auto-save feature
				wxString strUserTyped;
				COutputFilenameDlg dlg(GetDocumentWindow());
				dlg.Centre();
				dlg.m_strFilename = fileTitle;
				if (dlg.ShowModal() == wxID_OK)
				{
					// get the filename
					strUserTyped = dlg.m_strFilename;
					
					// The COutputFilenameDlg::OnOK() handler checks for duplicate file name or a file name
					// with bad characters in it.
					// abort the operation if user gave no explicit or bad output filename
					if (strUserTyped.IsEmpty())
					{
						// warn user to specify a non-null document name with valid chars
						// IDS_EMPTY_OUTPUT_FILENAME
						if (strUserTyped.IsEmpty())
							wxMessageBox(_(
"Sorry, Adapt It needs an output document name. (An .xml extension will be automatically added.) Please try the New... command again."),
										_T(""),wxICON_INFORMATION);


						// reinitialize everything
						pApp->m_pTargetBox->ChangeValue(_T(""));
						delete pApp->m_pBuffer;
						pApp->m_pBuffer = (wxString*)NULL; // MFC had = 0
						pApp->m_curOutputFilename = _T("");
						pApp->m_curOutputPath = _T("");
						pApp->m_curOutputBackupFilename = _T("");
						pApp->m_altOutputBackupFilename = _T("");
						pView->Invalidate(); // our own
						GetLayout()->PlaceBox();
						return FALSE;
					}

					// remove any extension user may have typed -- we'll keep control ourselves
					SetDocumentWindowTitle(strUserTyped, strUserTyped); // extensionless name is 
												// returned as the last parameter in the signature

					// BEW changed 06Aug06, and again 15Aug05
					if (gpApp->m_bSaveAsXML) // always true in the wx version
					{
						// for XML output
						pApp->m_curOutputFilename = strUserTyped + _T(".xml");
						pApp->m_curOutputBackupFilename = strUserTyped + _T(".BAK") + _T(".xml");
						// also make the alternate name be defined, in case DoFileSave() needs it
						pApp->m_altOutputBackupFilename = strUserTyped + _T(".BAK");
					}
				}
				else
				{
					// user cancelled, so cancel the New... command too
					// IDS_NO_OUTPUT_FILENAME
					wxMessageBox(_(
"Sorry, Adapt It will not work correctly unless you specify an output document name. Please try again."),
								_T(""), wxICON_INFORMATION);

					// reinitialize everything
					pApp->m_pTargetBox->ChangeValue(_T(""));
					delete pApp->m_pBuffer;
					pApp->m_pBuffer = (wxString*)NULL; // MFC had = 0
					pApp->m_curOutputFilename = _T("");
					pApp->m_curOutputPath = _T("");
					pApp->m_curOutputBackupFilename = _T("");
					pView->Invalidate();
					GetLayout()->PlaceBox();
					return FALSE;
				}

				// BEW modified 11Nov05, because the SetDocumentWindowTitle() call now updates
				// the window title
				// Set the document's path to reflect user input; the destination folder will
				// depend on whether book mode is ON or OFF; likewise for backups if turned on
				if (gpApp->m_bBookMode && !gpApp->m_bDisableBookMode)
				{
					pApp->m_curOutputPath = pApp->m_bibleBooksFolderPath + pApp->PathSeparator 
						+ pApp->m_curOutputFilename; // to send to the app when saving m_lastDocPath to
													 // config files
				}
				else
				{
					pApp->m_curOutputPath = pApp->m_curAdaptionsPath + pApp->PathSeparator 
						+ pApp->m_curOutputFilename; // to send to the app when saving m_lastDocPath to
													 // config files
				}

				SetFilename(pApp->m_curOutputPath,TRUE);// TRUE notify all views
				Modify(FALSE);

				// remove any optional hyphens in the source text for use by Ventura Publisher
				// (skips over any <-> sequences, and gives new m_pBuffer contents & new 
				// m_nInputFileLength value)
				RemoveVenturaOptionalHyphens(pApp->m_pBuffer);

                // whm wx version: moved the following OverwriteUSFMFixedSpaces and
                // OverwriteUSFMDiscretionaryLineBreaks calls here from within TokenizeText
                // if user requires, change USFM fixed spaces (marked by the !$
                // two-character sequence) to a pair of spaces - this does not change the
                // length of the data in the buffer
				if (gpApp->m_bChangeFixedSpaceToRegularSpace)
					OverwriteUSFMFixedSpaces(pApp->m_pBuffer);

                // Change USFM discretionary line breaks // to a pair of spaces. We do this
                // unconditionally because these types of breaks are not likely to be
                // located in the same place if allowed to pass through to the target text,
                // and are usually placed in the translation in the final typesetting
                // stage. This does not change the length of the data in the buffer.
				OverwriteUSFMDiscretionaryLineBreaks(pApp->m_pBuffer);

	#ifndef __WXMSW__
	#ifndef _UNICODE
				// whm added 12Apr2007
				OverwriteSmartQuotesWithRegularQuotes(pApp->m_pBuffer);
	#endif
	#endif
				// parse the input file
				int nHowMany;
				nHowMany = TokenizeText(0,pApp->m_pSourcePhrases,*pApp->m_pBuffer,
										(int)pApp->m_nInputFileLength);

                // Get any unknown markers stored in the m_markers member of the Doc's
                // source phrases whm ammended 29May06: Bruce desired that the filter
                // status of unk markers be preserved for new documents created within the
                // same project within the same session, so I've changed the last parameter
                // of GetUnknownMarkersFromDoc from setAllUnfiltered to
                // useCurrentUnkMkrFilterStatus.
				GetUnknownMarkersFromDoc(gpApp->gCurrentSfmSet, &gpApp->m_unknownMarkers, 
										&gpApp->m_filterFlagsUnkMkrs, 
										gpApp->m_currentUnknownMarkersStr, 
										useCurrentUnkMkrFilterStatus);

	#ifdef _Trace_UnknownMarkers
				TRACE0("In OnNewDocument AFTER GetUnknownMarkersFromDoc (setAllUnfiltered) call:\n");
				TRACE1(" Doc's unk mrs from arrays  = %s\n", GetUnknownMarkerStrFromArrays(&m_unknownMarkers, &m_filterFlagsUnkMkrs));
				TRACE1(" m_currentUnknownMarkersStr = %s\n", m_currentUnknownMarkersStr);
	#endif

				// calculate the layout in the view
				int srcCount;
				srcCount = pApp->m_pSourcePhrases->GetCount(); // unused
				if (pApp->m_pSourcePhrases->IsEmpty())
				{
					// IDS_NO_SOURCE_DATA
					wxMessageBox(_(
"Sorry, but there was no source language data in the file you input, so there is nothing to be displayed. Try a different file."),
								_T(""), wxICON_EXCLAMATION);

					// restore everything
					pApp->m_pTargetBox->ChangeValue(_T(""));
					delete pApp->m_pBuffer;
					pApp->m_pBuffer = (wxString*)NULL; // MFC had = 0
					pView->Invalidate();
					GetLayout()->PlaceBox();
					return FALSE;
				}

				// try this for the refactored layout design....
				CLayout* pLayout = GetLayout();
				pLayout->SetLayoutParameters(); // calls InitializeCLayout() and 
							// UpdateTextHeights() and calls other relevant setters
#ifdef _NEW_LAYOUT
				bool bIsOK = pLayout->RecalcLayout(pApp->m_pSourcePhrases, create_strips_and_piles);
#else
				bool bIsOK = pLayout->RecalcLayout(pApp->m_pSourcePhrases, create_strips_and_piles);
#endif
				if (!bIsOK)
				{
					// unlikely to fail, so just have something for the developer here
					wxMessageBox(_T("Error. RecalcLayout(TRUE) failed in OnNewDocument()"),
					_T(""), wxICON_STOP);
					wxASSERT(FALSE);
					wxExit();
				}

				// mark document as modified
				Modify(TRUE);

				// show the initial phraseBox - place it at the first empty target slot
				pApp->m_pActivePile = GetPile(0);

				pApp->m_nActiveSequNum = 0;
				bool bTestForKBEntry = FALSE;
				CKB* pKB;
				if (gbIsGlossing) // should not be allowed to be TRUE when OnNewDocument is called,
								  // but I will code for safety, since it can be handled okay
				{
					bTestForKBEntry = pApp->m_pActivePile->GetSrcPhrase()->m_bHasGlossingKBEntry;
					pKB = pApp->m_pGlossingKB;
				}
				else
				{
					bTestForKBEntry = pApp->m_pActivePile->GetSrcPhrase()->m_bHasKBEntry;
					pKB = pApp->m_pKB;
				}
				if (bTestForKBEntry)
				{
					// it's not an empty slot, so search for the first empty one & do it there; but if
					// there are no empty ones, then revert to the first pile
					CPile* pPile = pApp->m_pActivePile;
					pPile = pView->GetNextEmptyPile(pPile);
					if (pPile == NULL)
					{
						// there was none, so we must place the box at the first pile
						pApp->m_pTargetBox->m_textColor = pApp->m_targetColor;
						pView->PlacePhraseBox(pApp->m_pActivePile->GetCell(1));
						pView->Invalidate();
						pApp->m_nActiveSequNum = 0;
						gnOldSequNum = -1; // no previous location exists yet
						// get rid of the stored rebuilt source text, leave a space there instead
						if (pApp->m_pBuffer)
							*pApp->m_pBuffer = _T(' ');
						return TRUE;
					}
					else
					{
						pApp->m_pActivePile = pPile;
						pApp->m_nActiveSequNum = pPile->GetSrcPhrase()->m_nSequNumber;
					}
				}

				// BEW added 10Jun09, support phrase box matching of the text colour chosen
				if (gbIsGlossing && gbGlossingUsesNavFont)
				{
					pApp->m_pTargetBox->SetOwnForegroundColour(pLayout->GetNavTextColor());
				}
				else
				{
					pApp->m_pTargetBox->SetOwnForegroundColour(pLayout->GetTgtColor());
				}

				// set initial location of the targetBox
				pApp->m_targetPhrase = pView->CopySourceKey(pApp->m_pActivePile->GetSrcPhrase(),FALSE);
				translation = pApp->m_targetPhrase;
				pApp->m_pTargetBox->m_textColor = pApp->m_targetColor;
				pView->PlacePhraseBox(pApp->m_pActivePile->GetCell(1),2); // calls RecalcLayout()

				// save old sequ number in case required for toolbar's Back button - in this case 
				// there is no earlier location, so set it to -1
				gnOldSequNum = -1;

				// set the initial global position variable
				break;
			}// end of case getNewFile_success
			case getNewFile_error_at_open:
			{
				wxString strMessage;
				strMessage = strMessage.Format(_("Error opening file %s."),pathName.c_str());
				wxMessageBox(strMessage,_T(""), wxICON_ERROR);
				gpApp->m_lastSourceFileFolder = gpApp->m_workFolderPath;
				break;
			}
			case getNewFile_error_opening_binary:
			{
                // A binary file - probably not a valid input file such as a MS Word doc.
                // Notify user that Adapt It cannot read binary input files, and abort the
                // loading of the file.
				wxString strMessage = _(
				"The file you selected for input appears to be a binary file.");
				if (fnExtensionOnly.MakeUpper() == _T("DOC"))
				{
					strMessage += _T("\n");
					strMessage += _(
					"Adapt It cannot use Microsoft Word Document (doc) files as input files.");
				}
				else if (fnExtensionOnly.MakeUpper() == _T("ODT"))
				{
					strMessage += _T("\n");
					strMessage += _(
					"Adapt It cannot use OpenOffice's Open Document Text (odt) files as input files.");
				}
				strMessage += _T("\n");
				strMessage += _("Adapt It input files must be plain text files.");
				wxString strMessage2;
				strMessage2 = strMessage2.Format(_("Error opening file %s."),pathName.c_str());
				strMessage2 += _T("\n");
				strMessage2 += strMessage;
				wxMessageBox(strMessage2,_T(""), wxICON_ERROR);
				gpApp->m_lastSourceFileFolder = gpApp->m_workFolderPath;
				break;
			}
			case getNewFile_error_no_data_read:
			{
				// we got no data, so this constitutes a read failure
				wxMessageBox(_("File read error: no data was read in"),_T(""),wxICON_ERROR);
				break;
			}
			case getNewFile_error_unicode_in_ansi:
			{
				// The file is a type of Unicode, which is an error since this is the ANSI build. Notify
				// user that Adapt It Regular cannot read Unicode input files, and abort the loading of the
				// file.
				wxString strMessage = _("The file you selected for input is a Unicode file.");
				strMessage += _T("\n");
				strMessage += _("This Regular version of Adapt It cannot process Unicode text files.");
				strMessage += _T("\n");
				strMessage += _(
				"You should install and use the Unicode version of Adapt It to process Unicode text files.");
				wxString strMessage2;
				strMessage2 = strMessage2.Format(_("Error opening file %s."),pathName.c_str());
				strMessage2 += _T("\n");
				strMessage2 += strMessage;
				wxMessageBox(strMessage2,_T(""), wxICON_ERROR);
				gpApp->m_lastSourceFileFolder = gpApp->m_workFolderPath;
				break;
			}
			}// end of switch()
		} // end of else wxID_OK
	}// end of if (bKBReady)
	
	// get rid of the stored rebuilt source text, leave a space there instead (the value of
	// m_nInputFileLength can be left unchanged)
	if (pApp->m_pBuffer)
		*pApp->m_pBuffer = _T(' ');
	gbDoingInitialSetup = FALSE; // turn it back off, the pApp->m_targetBox now exists, etc

    // BEW added 01Oct06: to get an up-to-date project config file saved (in case user
    // turned on or off the book mode in the wizard) so that if the app subsequently
    // crashes, at least the next launch will be in the expected mode
	if (gbPassedAppInitialization && !pApp->m_curProjectPath.IsEmpty())
	{
		bool bOK;
		if (pApp->m_bUseCustomWorkFolderPath && !pApp->m_customWorkFolderPath.IsEmpty())
		{
			// whm 10Mar10, must save using what paths are current, but when the custom
			// location has been locked in, the filename lacks "Admin" in it, so that it
			// becomes a "normal" project configuration file in m_curProjectPath at the 
			// custom location.
			if (pApp->m_bLockedCustomWorkFolderPath)
				bOK = pApp->WriteConfigurationFile(szProjectConfiguration, pApp->m_curProjectPath,projectConfigFile);
			else
				bOK = pApp->WriteConfigurationFile(szAdminProjectConfiguration, pApp->m_curProjectPath,projectConfigFile);
		}
		else
		{
			bOK = pApp->WriteConfigurationFile(szProjectConfiguration, pApp->m_curProjectPath,projectConfigFile);
		}
		// below is original
		//bOK = pApp->WriteConfigurationFile(szProjectConfiguration,pApp->m_curProjectPath,projectConfigFile);
	}

	// Note: On initial program startup OnNewDocument() is executed from OnInit()
	// to get a temporary doc and view. pApp->m_curOutputPath will be empty in 
	// that case, so only call AddFileToHistory() when it's not empty.
	if (!pApp->m_curOutputPath.IsEmpty())
	{
		wxFileHistory* fileHistory = pApp->m_pDocManager->GetFileHistory();
		fileHistory->AddFileToHistory(pApp->m_curOutputPath);
        // The next two lines are a trick to get past AddFileToHistory()'s behavior of
        // extracting the directory of the file you supply and stripping the path of all
        // files in history that are in this directoy. RemoveFileFromHistory() doesn't do
        // any tricks with the path, so the following is a dirty fix to keep the full
        // paths.
		fileHistory->AddFileToHistory(wxT("[tempDummyEntry]"));
		fileHistory->RemoveFileFromHistory(0); // 
	}

    // BEW added 13Nov09, for setting or denying ownership for writing permission. This is
    // something we want to do each time a doc is created (or opened) - if the local user
    // already has ownership for writing, no change is done and he retains it; but if he
    // had read only access, and the other person has relinquished the project, then the
    // local user will now get ownership. 
    // BEW modified 18Nov09: there is an OnFileNew() call made in OnInit() at application
    // initialization time, and control goes to wxWidgets CreateDocument() which internally
    // calls its OnNewDocument() function which then calls Adapt_ItDoc::OnNewDocument().
    // If, in OnInit() we here, therefore, allow SetReadOnlyProtection() to be called,
    // we'll end up setting read-only access off when the application is the only instance
    // running and accessing the last used project folder (and OnInit() then has to be
    // given code to RemoveReadOnlyProtection() immediately after the OnFileNew() call,
    // because the latter is bogus, it is just to get the wxWidgets doc/view framework set
    // up, and the "real" access of a project folder comes later, after OnInit() ends and
    // OnIdle() runs and so the start working wizard runs, etc. All this is fine until we
    // do the following: the user starts Adapt It and opens a certain project; then the
    // user starts a second instance of Adapt It and opens the same project -- when this
    // second process runs, and while still within the OnInit() function, it detects that
    // the read-only protection file is currently open - and it is unable to remove it
    // because this is not the original process (although a 'bogus' one) that obtained
    // ownership of the project - and our code then aborts the second running Adapt It
    // instance giving a message that it is going to abort. This is unsatisfactory because
    // we want anyone, whether the same user or another, to be able to open a second
    // instance of Adapt It in read-only mode to look at what is being done, safely, in the
    // first running instance. Removing another process's open file is forbidden, so the
    // only recourse is to prevent the 'bogus' OnFileNew() call within OnInit() from
    // creating a read-only protection file here in OnNewDocument() if the call of the
    // latter is caused from within OnInit(). We can do this by having an app boolean
    // which is TRUE during OnInit() and FALSE thereafter. We'll call it:
    // m_bControlIsWithinOnInit
	if (!pApp->m_bControlIsWithinOnInit)
	{
		pApp->m_bReadOnlyAccess = pApp->m_pROP->SetReadOnlyProtection(pApp->m_curProjectPath);

		if (pApp->m_bReadOnlyAccess)
		{
			// if read only access is turned on, force the background colour change to show
			// now, instead of waiting for a user action requiring a canvas redraw
			pApp->GetView()->canvas->Refresh(); // needed? the call in OnIdle() is more efffective
		}
	}
	return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
/// \return nothing
/// \param	event	-> wxCommandEvent (unused)
/// \remarks
/// Called from: the doc/view framework when wxID_SAVE event is generated. Also called from
/// CMainFrame's SyncScrollReceive() when it is necessary to save the current document before
/// opening another document when sync scrolling is on.
/// OnFileSave simply calls DoFileSave().
///////////////////////////////////////////////////////////////////////////////
void CAdapt_ItDoc::OnFileSave(wxCommandEvent& WXUNUSED(event)) 
{
	// I have done my own OnFileSave override because the MFC version, while appearing to work
	// correctly (no exceptions thrown, correct full path, disk activity when expected, etc.)
	// did not write a file to the destination folder. The folder shows all files, including
	// hidden ones, but remained empty. Nor was the written file anywhere else on the disk. A
	// shortcut to the file gets created in C:\Windows\Recent, with correct path to where the
	// file should be, but it is not there (ie. in the Adaptations folder). So I will do my own.
	DoFileSave(TRUE); // TRUE - show wait/progress dialog - don't care about return value here
}

///////////////////////////////////////////////////////////////////////////////
/// \return nothing
/// \param      event   -> the wxUpdateUIEvent that is generated when the File Menu is about
///                         to be displayed
/// \remarks
/// Called from: The wxUpdateUIEvent mechanism when the associated menu item is selected,
/// and before the menu is displayed.
/// Enables or disables menu and/or toolbar items associated with the wxID_SAVE identifier.
/// If Vertical Editing is in progress the File Save menu item is always disabled, and this
/// handler returns immediately. Otherwise, the item is enabled if the KB exists, and if 
/// m_pSourcePhrases has at least one item in its list, and IsModified() returns TRUE; 
/// otherwise the item is disabled.
/// BEW modified 13Nov09, if the local user has only read-only access to a remote
/// project folder, do not let him save his local copy of the remote document to the
/// remote machine, otherwise the remote user is almost certainly to lose some edits
///////////////////////////////////////////////////////////////////////////////
void CAdapt_ItDoc::OnUpdateFileSave(wxUpdateUIEvent& event) 
{
	CAdapt_ItApp* pApp = &wxGetApp();
	wxASSERT(pApp != NULL);
	if (pApp->m_bReadOnlyAccess)
	{
		event.Enable(FALSE);
		return;
	}
	if (gbVerticalEditInProgress)
	{
		event.Enable(FALSE);
		return;
	}
	if (pApp->m_pKB != NULL && pApp->m_pSourcePhrases->GetCount() > 0 && IsModified())
		event.Enable(TRUE);
	else
		event.Enable(FALSE);	
}

///////////////////////////////////////////////////////////////////////////////
/// \return TRUE if the document is successfully opened, otherwise FALSE.
/// \param	lpszPathName	-> the name and path of the document to be opened
/// \remarks
/// Called from: the App's DoTransformationsToGlosses( ) function.
/// Opens a document in another project in preparation for transforming its adaptations into 
/// glosses in the current project. The other project's documents get copied in the process, but
/// are left unchanged in the other project. Since we are not going to look at the contents of
/// the document, we don't do anything except get it into memory ready for transforming.
/// BEW changed 31Aug05 so it would handle either .xml or .adt documents automatically (code pinched
/// from start of OnOpenDocument())
///////////////////////////////////////////////////////////////////////////////
bool CAdapt_ItDoc::OpenDocumentInAnotherProject(wxString lpszPathName)
{
	// BEW added 31Aug05 for XML doc support (we have to find out what extension it has
	// and then choose the corresponding code for loading that type of doc 
	wxString thePath = lpszPathName;
	wxString extension = thePath.Right(4);
	extension.MakeLower();
	wxASSERT(extension[0] == _T('.')); // check it really is an extension

	wxFileName fn(thePath);
	wxString fullFileName;
	fullFileName = fn.GetFullName();
	if (extension == _T(".xml"))
	{
		// we have to input an xml document
		bool bReadOK = ReadDoc_XML(thePath,this);
		if (!bReadOK)
		{
			wxString s;
			s = _(
"There was an error parsing in the XML file. If you edited the XML file earlier, you may have introduced an error. Edit it in a word processor then try again.");
			wxMessageBox(s, fullFileName, wxICON_INFORMATION);
			return FALSE; // return FALSE to tell caller we failed
		}
	}
	else
	{
		wxMessageBox(_(
		"Sorry, the wxWidgets version of Adapt It does not read legacy .adt document format; it only reads the .xml format.")
		,fullFileName,wxICON_WARNING);
		return FALSE;
	}
	return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
/// \return nothing
/// \param	event	-> wxCommandEvent (unused)
/// \remarks
/// Called automatically within the doc/view framework when an event associated with the
/// wxID_OPEN identifier (such as File | Open) is generated within the framework. It is
/// also called by the Doc's OnOpenDocument(), by the DocPage's OnWizardFinish() and by
/// SplitDialog's SplitIntoChapters_Interactive() function.
/// Rather than using the doc/view's default behavior for OnFileOpen() this function calls 
/// our own DoFileOpen() function after setting the current work folder to the Adaptations
/// path, or the current book folder if book mode is on.
///////////////////////////////////////////////////////////////////////////////
void CAdapt_ItDoc::OnFileOpen(wxCommandEvent& WXUNUSED(event)) 
{
	CAdapt_ItApp* pApp = &wxGetApp();
	wxASSERT(pApp != NULL);
    // ensure that the current work folder is the Adaptations one for default; unless book
    // mode is ON, in which case it must the the current book folder.
	wxString dirPath;
	if (pApp->m_bBookMode && !pApp->m_bDisableBookMode)
		dirPath = pApp->m_bibleBooksFolderPath;
	else
		dirPath = pApp->m_curAdaptionsPath;
	bool bOK;
	bOK = ::wxSetWorkingDirectory(dirPath); // ignore failures

	// NOTE: This OnFileOpen() handler calls DoFileOpen() in the App, which now simply
	// calls DoStartWorkingWizard().
	pApp->DoFileOpen();
}

///////////////////////////////////////////////////////////////////////////////
/// \return nothing
/// \param	event	-> wxCommandEvent associated with the wxID_CLOSE identifier.
/// \remarks
/// This function is called automatically by the doc/view framework when an event
/// associated with the wxID_CLOSE identifier is generated. It is also called by: the App's
/// OnFileChangeFolder() and OnAdvancedBookMode(), by the View's OnFileCloseProject(), and
/// by DocPage's OnButtonChangeFolder() and OnWizardFinish() functions.
/// This override of OnFileClose does not close the app, it just clears out all the current view
/// structures, after calling our version of SaveModified. It simply closes files & leave the
/// app ready for other files to be opened etc. Our SaveModified() & this OnFileClose are
/// not OLE compliant. (A New... or Open... etc. will call DeleteContents on the doc structures
/// before a new doc can be made or opened). For version 2.0, which supports glossing, if one KB
/// gets saved, then the other should be too - this needs to be done in our SaveModified( ) function
/// NOTE: we don't change the values of the four flags associated with glossing, because this
/// function may be called for processes which serially open and close each document of a 
/// project, and the flags will have to maintain their values across the calls to ClobberDocument;
/// and certainly ClobberDocumen( ) will be called each time even if this one isn't.
///////////////////////////////////////////////////////////////////////////////
void CAdapt_ItDoc::OnFileClose(wxCommandEvent& event)
{
	CAdapt_ItApp* pApp = &wxGetApp();
	wxASSERT(pApp != NULL);
	
	if (gbVerticalEditInProgress)
	{
		// don't allow doc closure until the vertical edit is finished
		::wxBell(); 
		return;
	}

	if (gpApp->m_bFreeTranslationMode)
	{
		// free translation mode is on, so we must first turn it off
		gpApp->GetView()->OnAdvancedFreeTranslationMode(event);
	}
	
	bUserCancelled = FALSE; // default
	if(!OnSaveModified())
	{
		bUserCancelled = TRUE;
		return;
	}

	// BEW added 19Nov09, for read-only support; when a document is closed, attempt
	// to remove any read-only protection that is current for this project folder, because
	// the owning process may have come to have abandoned its ownership prior to the local
	// user closing this document, and that gives the next document opened in this project
	// by the local user the chance to own it for writing
	if (!pApp->m_curProjectPath.IsEmpty())
	{
		// if unowned, or if my process has the ownership, then ownership will be removed
		// at the doc closure (the ~AIROP-*.lock file will have been deleted), and TRUE
		// will be returned to bRemoved - which is then used to clear m_bReadOnlyAccess
		// to FALSE. This makes this project ownable by whoever next opens a document in it.
		bool bRemoved = pApp->m_pROP->RemoveReadOnlyProtection(pApp->m_curProjectPath);
		if (bRemoved)
		{
			pApp->m_bReadOnlyAccess = FALSE; // project folder is now ownable for writing
			pApp->GetView()->canvas->Refresh(); // try force color change back to normal 
				// white background -- it won't work as the canvas is empty, but the
				// removal of read only protection is still done if possible
		}
		else
		{
			pApp->m_bReadOnlyAccess = TRUE; // this project folder is still read-only
				// for this running process, as we are still in this project folder
		}
	}
	
	bUserCancelled = FALSE;
	CAdapt_ItView* pView = (CAdapt_ItView*) GetFirstView();
	wxASSERT(pView != NULL);
	pView->ClobberDocument();

	// delete the buffer containing the filed-in source text
	if (pApp->m_pBuffer != 0)
	{
		delete pApp->m_pBuffer;
		pApp->m_pBuffer = (wxString*)NULL; // MFC had = 0
	}

	// show "Untitled" etc
	wxString viewTitle = _("Untitled - Adapt It");
	SetTitle(viewTitle);
	SetFilename(viewTitle,TRUE);	// here TRUE means "notify the views" whereas
									// in the MFC version TRUE meant "add to MRU list"
    // Note: SetTitle works, but the doc/view framework overwrites the result with "Adapt
    // It [unnamed1]", etc unless SetFilename() is also used.
	// 
	// whm modified 13Mar09: 
	// When the doc is explicitly closed on Linux, the Ctrl+O hot key doesn't work unless the focus is
	// placed on an existing element such as the toolbar's Open icon (which is where the next action
	// would probably happen).
	CMainFrame* pFrame = pApp->GetMainFrame();
	wxASSERT(pFrame != NULL);
	wxASSERT(pFrame->m_pControlBar != NULL);
	pFrame->m_pControlBar->SetFocus();
}

///////////////////////////////////////////////////////////////////////////////
/// \return nothing
/// \param      event   -> the wxUpdateUIEvent that is generated when the File Menu is about
///                         to be displayed
/// \remarks
/// Called from: The wxUpdateUIEvent mechanism when the associated menu item is selected, and before
/// the menu is displayed.
/// Enables or disables menu item associated with the wxID_CLOSE identifier.
/// If Vertical Editing is in progress the File Close menu item is disabled, and this handler
/// immediately returns. Otherwise, the item is enabled if m_pSourcePhrases has at least one 
/// item in its list; otherwise the item is disabled.
///////////////////////////////////////////////////////////////////////////////
void CAdapt_ItDoc::OnUpdateFileClose(wxUpdateUIEvent& event) 
{
	if (gbVerticalEditInProgress)
	{
		event.Enable(FALSE);
		return;
	}
	CAdapt_ItApp* pApp = &wxGetApp();
	wxASSERT(pApp != NULL);
	if (pApp->m_pSourcePhrases->GetCount() > 0)
	{
		event.Enable(TRUE);
	}
	else
	{
		event.Enable(FALSE);
	}
}

///////////////////////////////////////////////////////////////////////////////
/// \return TRUE if the document is successfully backed up, otherwise FALSE.
/// \param	pApp	-> currently unused
/// \remarks
/// Called by the Doc's DoFileSave() function.
/// BEW added 23June07; do no backup if gbDoingSplitOrJoin is TRUE;
/// these operations could produce a plethora of backup docs, especially for a
/// single-chapters document split, so we just won't permit splitting, or joining
/// (except for the resulting joined file), or moving to generate new backups.
///////////////////////////////////////////////////////////////////////////////
bool CAdapt_ItDoc::BackupDocument(CAdapt_ItApp* WXUNUSED(pApp))
{
	if (gbDoingSplitOrJoin)
		return TRUE;

	wxFile f; // create a CFile instance with default constructor
	wxString altFilename;

	// make the working directory the "Adaptations" one; or a bible book folder 
	// if in book mode
	bool bOK;
	if (gpApp->m_bBookMode && !gpApp->m_bDisableBookMode)
	{
		bOK = ::wxSetWorkingDirectory(gpApp->m_bibleBooksFolderPath);
	}
	else
	{
		bOK = ::wxSetWorkingDirectory(gpApp->m_curAdaptionsPath);
	}
	if (!bOK)
	{
		wxString str;
		//IDS_DOC_BACKUP_PATH_ERR
		if (gpApp->m_bBookMode && !gpApp->m_bDisableBookMode)
			str = str.Format(_(
			"Warning: document backup failed for the path:  %s   No backup was done."),
			GetApp()->m_bibleBooksFolderPath.c_str());
		else
			str = str.Format(_(
			"Warning: document backup failed for the path:  %s   No backup was done."),
			GetApp()->m_curAdaptionsPath.c_str());
		wxMessageBox(str,_T(""), wxICON_EXCLAMATION);
		return FALSE;
	}

	// BEW changed 15Aug05 & changed again on 23Jun07
	// m_curOutputBackupFilename and m_altOutputBackupFilename, are both defined before
	// BackupDocument() is called -- but they can not be in compliance with the
	// m_bSaveAsXML value, so we ensure it now. (DoFileSave() has already made this call,
	// so it is redundant for saves to have it here, but because there may be other
	// control paths to the BackupDocument() function, the safest thing to do is to
	// ensure compliance immediately before the filename strings need to be used

	// make sure the backup filenames comply too (BEW added 23June07) -- the function
	// sets m_curOutputBackupFilename and m_altOutputBackupFilename based on the
	// passed in filename string, and the current m_bSaveAsXML value; but we cannot
	// be certain a backup with an alternative type's name will not have been already
	// saved, so we save the current values of these filenames too so we can check for
	// that possibility too -- that is, any backup file with the same filetitle has to
	// go, and the BackupDocument() can be certain to write only a fully compliant type
	wxString strSavePrimary = gpApp->m_curOutputBackupFilename;
	wxString strSaveAlternate = gpApp->m_altOutputBackupFilename;
	MakeOutputBackupFilenames(gpApp->m_curOutputFilename,gpApp->m_bSaveAsXML);

	// now we know all the possibilities, so remove any which have the file title
	wxString aFilename = strSavePrimary;
	if (wxFileExists(aFilename))
	{
		// this backed up document file is on the disk, so delete it
		if (!::wxRemoveFile(aFilename))
		{
			wxString s;
			s = s.Format(_(
	"Could not remove the backed up document file: %s; the application will continue"),
			aFilename.c_str());
			wxMessageBox(s, _T(""), wxICON_EXCLAMATION);
			// do nothing else, let the app continue
		}
	}
	else
	{
		aFilename = strSaveAlternate;
		if (wxFileExists(aFilename))
		{
			// this backed up document file (of alternate name) is on the disk, so delete it
			if (!::wxRemoveFile(aFilename))
			{
			wxString s;
				s = s.Format(_(
	"Could not remove the backed up document file: %s; the application will continue"),
				aFilename.c_str());
				wxMessageBox(s, _T(""), wxICON_EXCLAMATION);
				// do nothing else, let the app continue
			}
		}
	}
	aFilename = gpApp->m_curOutputBackupFilename; // try again, with the compliant 
            // backup filenames (if we have already removed a backup file in the above
            // block, this will do nothing
	if (wxFileExists(aFilename))
	{
		// this backed up document file is on the disk, so delete it
		if (!::wxRemoveFile(aFilename))
		{
			wxString s;
			s = s.Format(_(
	"Could not remove the backed up document file: %s; the application will continue"),
			aFilename.c_str());
			wxMessageBox(s, _T(""), wxICON_EXCLAMATION);
			// do nothing else, let the app continue
		}
	}
	else
	{
		aFilename = gpApp->m_curOutputBackupFilename;
		if (wxFileExists(aFilename))
		{
			// this backed up document file (of alternate name) is on the disk,
			// so delete it
			if (!::wxRemoveFile(aFilename))
			{
			wxString s;
				s = s.Format(_(
	"Could not remove the backed up document file: %s; the application will continue"),
				aFilename.c_str());
				wxMessageBox(s, _T(""), wxICON_EXCLAMATION);
				// do nothing else, let the app continue
			}
		}
	}
	// the new backup will have the name which is in m_curOutputBackupFilename
	int len = gpApp->m_curOutputBackupFilename.Length();
	if (gpApp->m_curOutputBackupFilename.IsEmpty() || len <= 4)
	{
		wxString str;
		// IDS_DOC_BACKUP_NAME_ERR
		str = str.Format(_(
"Warning: document backup failed because the following name is not valid: %s    No backup was done."),
		gpApp->m_curOutputBackupFilename.c_str());
		wxMessageBox(str,_T(""),wxICON_EXCLAMATION);
		return FALSE;
	}
	
	// copied from DoFileSave - I didn't change the share options, not likely to matter here
	bool bFailed = FALSE;
	if (!f.Open(gpApp->m_curOutputBackupFilename,wxFile::write))
	{
		wxString s;
		s = s.Format(_(
		"Could not open a file stream for backup, in BackupDocument(), for file %s"),
		gpApp->m_curOutputBackupFilename.c_str());
		wxMessageBox(s,_T(""),wxICON_EXCLAMATION);
		return FALSE; 
	}

	if (gpApp->m_bSaveAsXML) // always true in the wx version
	{
		CSourcePhrase* pSrcPhrase;
		CBString aStr;
		CBString openBraceSlash = "</"; // to avoid "warning: 
				// deprecated conversion from string constant to 'char*'"

		// prologue (BEW changed 02July07 to use Bob's huge switch in the
		// GetEncodingStrongForXmlFiles() function which he did, to better support
		// legacy KBs & doc conversions in SILConverters conversion engines)
		gpApp->GetEncodingStringForXmlFiles(aStr);
		DoWrite(f,aStr);

		// add the comment with the warning about not opening the XML file in MS WORD 
		// 'coz is corrupts it - presumably because there is no XSLT file defined for it
		// as well. When the file is then (if saved in WORD) loaded back into Adapt It,
		// the latter goes into an infinite loop when the file is being parsed in.
		aStr = MakeMSWORDWarning(); // the warning ends with \r\n 
									// so we don't need to add them here
		// doc opening tag
		aStr += "<";
		aStr += xml_adaptitdoc;
		aStr += ">\r\n";
		DoWrite(f,aStr);

		// place the <Settings> element at the start of the doc
		aStr = ConstructSettingsInfoAsXML(1);
		DoWrite(f,aStr);

		// add the list of sourcephrases
		SPList::Node* pos = gpApp->m_pSourcePhrases->GetFirst();
		while (pos != NULL)
		{
			pSrcPhrase = (CSourcePhrase*)pos->GetData();
			pos = pos->GetNext();
			aStr = pSrcPhrase->MakeXML(1);
			DoWrite(f,aStr);
		}

		// doc closing tag
		aStr = xml_adaptitdoc;
		aStr = openBraceSlash + aStr; //"</" + aStr;
		aStr += ">\r\n";
		DoWrite(f,aStr);

		// close the file
		f.Close();
		f.Flush();
	}	
	if (bFailed)
		return FALSE;
	else
		return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
/// \return a CBString composed of settings info formatted as XML.
/// \param	nTabLevel	-> defines how many indenting tab characters are placed before each
///			               constructed XML line; 1 gives one tab, 2 gives two, etc.
/// Called by the Doc's BackupDocument(), DoFileSave(), and DoTransformedDocFileSave()
/// functions. Creates a CBString that contains the XML prologue and settings information
/// formatted as XML.
///////////////////////////////////////////////////////////////////////////////
CBString CAdapt_ItDoc::ConstructSettingsInfoAsXML(int nTabLevel)
{
	CBString bstr;
	bstr.Empty();
	CBString btemp;
	int i;
	wxString tempStr;
    // wx note: the wx version in Unicode build refuses to assign a CBString to char
    // numStr[24] so I'll declare numStr as a CBString also
	CBString numStr; //char numStr[24];
#ifdef _UNICODE

	// first line -- element name and 4 attributes
	for (i = 0; i < nTabLevel; i++)
	{
		bstr += "\t"; // tab the start of the line
	}
	bstr += "<Settings docVersion=\"";
	tempStr.Empty();
    // wx note: The itoa() operator is Microsoft specific and not standard; unknown to g++
    // on Linux/Mac. The wxSprintf() statement below in Unicode build won't accept CBString
    // or char numStr[24] for first parameter, therefore, I'll simply do the int to string
    // conversion in UTF-16 with wxString's overloaded insertion operatior << then convert
    // to UTF-8 with Bruce's Convert16to8() method. [We could also do it here directly with
    // wxWidgets' conversion macros rather than calling Convert16to8() - see the
    // Convert16to8() function in the App.]
	tempStr << (int)VERSION_NUMBER; // tempStr is UTF-16
	numStr = gpApp->Convert16to8(tempStr);
	bstr += numStr; // add versionable schema number string

	bstr += "\" sizex=\"";
	tempStr.Empty(); // needs to start empty, otherwise << will append the string value of the int
	tempStr << gpApp->m_docSize.x;
	numStr = gpApp->Convert16to8(tempStr);
	bstr += numStr; // add doc width number string
	// TODO: Bruce substituted m_nActiveSequNum for the m_docSize.cy value here. Should this be rolled back?
	// I think he no longer needed it this way for the Dana's progress gauge, and hijacking the m_docSize.cy
	// value for such purposes caused a Beep in OpenDocument on certain size documents because m_docSize was 
	// out of bounds. (BEW note, the error no longer happens, so we should keep this for
	// support of the Dana's Palm OS version of Adapt It)
	bstr += "\" sizey=\"";
	tempStr.Empty(); // needs to start empty, otherwise << will append the string value of the int
	tempStr << gpApp->m_nActiveSequNum; // should m_docSize.cy be used instead???
	numStr = gpApp->Convert16to8(tempStr);
	bstr += numStr; // add index of active location's string (Dana uses this) // add doc length number string
	bstr += "\" specialcolor=\"";
	tempStr.Empty(); // needs to start empty, otherwise << will append the string value of the int
	tempStr << WxColour2Int(gpApp->m_specialTextColor);
	numStr = gpApp->Convert16to8(tempStr);
	bstr += numStr; // add specialText color number string
	bstr += "\"\r\n";

	// second line -- 5 attributes
	for (i = 0; i < nTabLevel; i++)
	{
		bstr += "\t"; // tab the start of the line
	}
	bstr += "retranscolor=\"";
	tempStr.Empty(); // needs to start empty, otherwise << will append the string value of the int
	tempStr << WxColour2Int(gpApp->m_reTranslnTextColor);
	numStr = gpApp->Convert16to8(tempStr);
	bstr += numStr; // add retranslation text color number string
	bstr += "\" navcolor=\"";
	tempStr.Empty(); // needs to start empty, otherwise << will append the string value of the int
	tempStr << WxColour2Int(gpApp->m_navTextColor);
	numStr = gpApp->Convert16to8(tempStr);
	bstr += numStr; // add navigation text color number string
	bstr += "\" curchap=\"";
	btemp = gpApp->Convert16to8(gpApp->m_curChapter);
	bstr += btemp; // add current chapter text color number string (app makes no use of this)
	bstr += "\" srcname=\"";
	btemp = gpApp->Convert16to8(gpApp->m_sourceName);
	bstr += btemp; // add name of source text's language
	bstr += "\" tgtname=\"";
	btemp = gpApp->Convert16to8(gpApp->m_targetName);
	bstr += btemp; // add name of target text's language
	bstr += "\"\r\n"; // TODO: EOL chars need adjustment for Linux and Mac???

	// third line - one attribute (potentially large, containing unix strings with filter markers,
	// unknown markers, etc -- entities should not be needed for it though)
	for (i = 0; i < nTabLevel; i++)
	{
		bstr += "\t"; // tab the start of the line
	}
	bstr += "others=\"";
	btemp = gpApp->Convert16to8(SetupBufferForOutput(gpApp->m_pBuffer));
	bstr += btemp; // all all the unix string materials (could be a lot)
	bstr += "\"/>\r\n"; // TODO: EOL chars need adjustment for Linux and Mac??
	return bstr;
#else // regular version

	// first line -- element name and 4 attributes
	for (i = 0; i < nTabLevel; i++)
	{
		bstr += "\t"; // tab the start of the line
	}
	bstr += "<Settings docVersion=\"";
	// wx note: The itoa() operator is Microsoft specific and not standard; unknown to g++ on Linux/Mac.
	// The use of wxSprintf() below seems to work OK in ANSI builds, but I'll use the << insertion
	// operator here as I did in the Unicode build block above, so the code below should be the same
	// as that for the Unicode version except for the Unicode version's use of Convert16to8().
	tempStr.Empty(); // needs to start empty, otherwise << will append the string value of the int
	tempStr << (int)VERSION_NUMBER;
	numStr = tempStr;
	bstr += numStr; // add versionable schema number string
	bstr += "\" sizex=\"";
	tempStr.Empty(); // needs to start empty, otherwise << will append the string value of the int
	tempStr << gpApp->m_docSize.x;
	numStr = tempStr;
	bstr += numStr; // add doc width number string
	// TODO: Bruce substituted m_nActiveSequNum for the m_docSize.cy value here. Should this be rolled back?
	// I think he no longer needed it this way for the Dana's progress gauge, and hijacking the m_docSize.cy
	// value for such purposes caused a Beep in OpenDocument on certain size documents because m_docSize was 
	// out of bounds.
	bstr += "\" sizey=\"";
	tempStr.Empty(); // needs to start empty, otherwise << will append the string value of the int
	//wxSprintf(numStr,"%d",(int)gpApp->m_nActiveSequNum);
	tempStr << gpApp->m_nActiveSequNum; // should m_docSize.cy be used instead???
	numStr = tempStr;
	bstr += numStr; // add index of active location's string (Dana uses this) // add doc length number string
	bstr += "\" specialcolor=\"";
	tempStr.Empty(); // needs to start empty, otherwise << will append the string value of the int
	tempStr << WxColour2Int(gpApp->m_specialTextColor);
	numStr = tempStr;
	bstr += numStr; // add specialText color number string
	bstr += "\"\r\n"; // TODO: EOL chars need adjustment for Linux and Mac??

	// second line -- 5 attributes
	for (i = 0; i < nTabLevel; i++)
	{
		bstr += "\t"; // tab the start of the line
	}
	bstr += "retranscolor=\"";
	tempStr.Empty(); // needs to start empty, otherwise << will append the string value of the int
	tempStr << WxColour2Int(gpApp->m_reTranslnTextColor);
	numStr = tempStr;
	bstr += numStr; // add retranslation text color number string
	bstr += "\" navcolor=\"";
	tempStr.Empty(); // needs to start empty, otherwise << will append the string value of the int
	tempStr << WxColour2Int(gpApp->m_navTextColor);
	numStr = tempStr;
	bstr += numStr; // add navigation text color number string
	bstr += "\" curchap=\"";
	btemp = gpApp->m_curChapter;
	bstr += btemp; // add current chapter text color number string (app makes no use of this)
	bstr += "\" srcname=\"";
	btemp = gpApp->m_sourceName;
	bstr += btemp; // add name of source text's language
	bstr += "\" tgtname=\"";
	btemp = gpApp->m_targetName;
	bstr += btemp; // add name of target text's language
	bstr += "\"\r\n"; // TODO: EOL chars need adjustment for Linux and Mac??

	// third line - one attribute (potentially large, containing unix strings with filter markers,
	// unknown markers, etc -- entities should not be needed for it though)
	for (i = 0; i < nTabLevel; i++)
	{
		bstr += "\t"; // tab the start of the line
	}
	bstr += "others=\"";
	btemp = SetupBufferForOutput(gpApp->m_pBuffer);
	bstr += btemp; // add all the unix string materials (could be a lot)
	bstr += "\"/>\r\n"; // TODO: EOL chars need adjustment for Linux and Mac??
	return bstr;
#endif
}

///////////////////////////////////////////////////////////////////////////////
/// \return nothing
/// \param	buffer	-> a wxString formatted into delimited fields containing the book mode,
///						the book index, the current sfm set, a list of the filtered markers,
///						and a list of the unknown markers
/// \remarks
/// Called from: the AtDocAttr() in XML.cpp.
/// RestoreDocParamsOnInput parses the buffer string and uses its stored information to
/// update the variables held on the App that hold the corresponding information.
///////////////////////////////////////////////////////////////////////////////
void CAdapt_ItDoc::RestoreDocParamsOnInput(wxString buffer)
{
	int dataLen = buffer.Length();
    // This function encapsulates code which formerly was in the Serialize() function, but
    // now that version 3 allows xml i/o, we need this functionality for both binary input
    // and for xml input. For version 2.3.0 and onwards, we don't store the source text in
    // the document so when reading in a document produced from earlier versions, we change
    // the contents of the buffer to a space so that a subsequent save will give a smaller
    // file; recent changes to use this member for serializing in/out the book mode
    // information which is needed for safe use of the MRU list, mean that we might have
    // that info read in, or it could be a legacy document's source text data. We can
    // distinguish these by the fact that the book mode information will be a string of 3,
    // 4 at most, characters followed by a null byte and EOF; whereas a valid legacy doc's
    // source text will be much much longer [see more comments within the function].
	// whm revised 6Jul05
	// We have to account for three uses of the wxString buffer here:
	// 1. Legacy use in which the buffer contained the entire source text.
	//    In this case we simply ignore the text and overwrite it with a space
	// 2. The extended app version in which only the first 3 or 4 characters were
	//    used to store the m_bBookMode and m_nBookIndex values.
	//    In this case the length of buffer string will be < 5 characters, and we handle
	//    the parsing of the book mode and book index as did the extended app.
	// 3. The current version 3 app which adds a unique identifier @#@#: to the beginning
	//    of the string buffer, then follows this by colon delimited fields concatenated in
	//    the buffer string to represent config data.
	//    In this case we verify we have version 3 structure by presence of the @#@#: initial
	//    5 characters, then parse the string like a unix data string. The book mode and book
	//    index will be the first two fields, followed by version 3 specific config values.
    // Note: The gCurrentSfmSet is always changed to be the same as was last saved in the
    // document. The gCurrentFilterMarkers is also always changed to be the same as way
    // last saved in the document. To insure that the current settings in the active
    // USFMAnalysis structs are in agreement with what was last saved in the document we
    // call ResetUSFMFilterStructs().
	//
	// BEW modified 09Nov05 as follows:
    // The current value for the Book Folder mode (True or False), and the current book
    // index (-1 if book mode is not currently on) have to be made to override the values
    // saved in the document if different than what is in the document - but only provided
    // the project is still the same one as the project under which the document was last
    // saved. The reason is as follows. Suppose book mode is off, and you save the document
    // - it goes into the Adaptations folder. Now suppose you use turn book mode on and use
    // the enabled Move command to move the document to the oppropriate book folder. The
    // document is now in a book folder, but internally it still contains the information
    // that it was saved with book mode off, and so no book index is stored there either
    // but just a -1 value. If you then, from within the Start Working wizard open the
    // moved document, the RestoreDocParamsOnInput() function would, unless modified to not
    // do so, restore book mode to off, and set the book index to -1 (whether for an XML
    // file read, or a binary one). This is not what we want or expect. We must also check
    // that the project is unchanged, because the user has the option of opening an
    // arbitary recent document from any project by clicking its name in the MRU list, and
    // the stored source and target language names and book mode info is then used to set
    // up the right path to the document and work out what its project was and make that
    // project the current one -- when you do this, it would be most unlikely that that
    // document was saved after a Move and you did not then open it but changed to the
    // current project, so in this case the book mode and book index as stored in the
    // document SHOULD be used (ie. potentially can reset the mode and change the book
    // index) so that you are returning to the most likely former state. There is no way to
    // detect that the former project's document was moved without being opened within the
    // project, so if the current mode differs, then the constructed path would not be
    // valid and Adapt It will not do the file read -- but this failure is detected and the
    // user is told the document probably no longer exists and is then put into the Start
    // Working wizard -- where he can then turn the mode back on (or off) and locate the
    // document and open it safely and continue working, so the MRU open will not lead to a
    // crash even if the above very unlikely scenario obtains. But if the doc was opened in
    // the earlier project, then using the saved book mode and index values as described
    // above will indeed find it successfully. So, in summary, if the project is different,
    // we must use the stored info in the doc, but if the project is unchanged, then we
    // must override the info in the doc because the fact that it was just opened means
    // that we got it from whatever folder is consistent with the current mode (ie.
    // Adaptations if book folder mode is off, a book folder if it is on) and so the
    // current setting is what we must go with. Whew!! Hope you cotton on to all this!
	wxString curSourceName; // can't use app's m_sourceName & m_targetName because these will already be
	wxString curTargetName; // overwritten so we set curSourceName and curTargetName 
                // by extracting the names from the app's m_curProjectName member which
                // doesn't get updated until the doc read has been successfully completed
	gpApp->GetSrcAndTgtLanguageNamesFromProjectName(gpApp->m_curProjectName, curSourceName, curTargetName);
	bool bSameProject = (curSourceName == gpApp->m_sourceName) && (curTargetName == gpApp->m_targetName);
    // BEW added 27Nov05 to keep settings straight when doc may have been pasted here in
    // Win Explorer but was created and stored in another project
	if (!bSameProject && !gbTryingMRUOpen)
	{
        // bSameProject being FALSE may be because we opened a doc created and saved in a
        // different project and it was a legacy *adt doc and so m_sourceName and
        // m_targetName will have been set wrongly, so we override the document's stored
        // values in favour of curSourceName and curTargetName which we know to be correct
		gpApp->m_sourceName = curSourceName;
		gpApp->m_targetName = curTargetName;
	}

	bool bVersion3Data;
	wxString strFilterMarkersSavedInDoc; // inventory of filtered markers read from the Doc's Buffer

	// initialize strFilterMarkersSavedInDoc to the App's gCurrentFilterMarkers
	strFilterMarkersSavedInDoc = gpApp->gCurrentFilterMarkers;

	// initialize SetSavedInDoc to the App's gCurrentSfmSet
	// Note: The App's Get proj config routine may change the gCurrentSfmSet to PngOnly
	enum SfmSet SetSavedInDoc = gpApp->gCurrentSfmSet;

	// check for version 3 special buffer prefix
	bVersion3Data = (buffer.Find(_T("@#@#:")) == 0);
	wxString field;
	if (bVersion3Data)
	{
		// case 3 above
		// assume we have book mode and index information followed by version 3 data
		int curPos;
		curPos = 0;
		int fieldNum = 0;

		// Insure that first token is _T("@#@#");
		wxASSERT(buffer.Find(_T("@#@#:")) == 0);

		wxStringTokenizer tkz(buffer, _T(":"), wxTOKEN_RET_EMPTY_ALL );

		while (tkz.HasMoreTokens())
		{
			field = tkz.GetNextToken();
			switch(fieldNum)
			{
			case 0: // this is the first field which should be "@#@#" - we don't do anything with it
					break;
			case 1: // book mode field
				{
                    // BEW modified 27Nov05 to only use the T or F values when doing an MRU
                    // open; since for an Open done by a wizard selection in the Document
                    // page, the doc is accessed either in Adaptations folder or a book
                    // folder, and so we must go with whichever mode was the case when we
                    // did that (m_bBookMode false for the former, true for the latter) and
                    // we certainly don't want the document to be able to set different
                    // values (which it could do if it was a foreign document just copied
                    // into a folder and we are opening it on our computer for the first
                    // time). I think the bSameProject value is not needed actually, an MRU
                    // open requires we try using what's on the doc, and an ordinary wizard
                    // open requires us to ignore what's on the doc.
					if (gbTryingMRUOpen)
					{
						// let the app's current setting stand except when an MRU open is tried
						if (field == _T("T"))
						{
							gpApp->m_bBookMode = TRUE;
						}
						else if (field == _T("F"))
						{
							gpApp->m_bBookMode = FALSE;
						}
						else
							goto t;
					}
					break;
				}
			case 2: // book index field
				{
					// see comments above about MRU
					if (gbTryingMRUOpen)
					{
						// let the app's current setting stand except when an MRU open is tried
						// use the file's saved index setting
						int i = wxAtoi(field);
						gpApp->m_nBookIndex = i;
						if (i >= 0  && !gpApp->m_bDisableBookMode)
						{
							gpApp->m_pCurrBookNamePair = ((BookNamePair*)(*gpApp->m_pBibleBooks)[i]);
						}
						else
						{
							// it's a -1 index, or the mode is disabled due to a bad parse of the 
							//  books.xml file, so ensure no named pair and the folder path is empty
							gpApp->m_nBookIndex = -1;
							gpApp->m_pCurrBookNamePair = NULL;
							gpApp->m_bibleBooksFolderPath.Empty();
						}
					}
					break;
				}
			case 3: // gCurrentSfmSet field
				{
					// gCurrentSfmSet is updated below.
					SetSavedInDoc = (SfmSet)wxAtoi(field); //_ttoi(field);
					break;
				}
			case 4: // filtered markers string field
				{
					// gCurrentFilterMarkers is updated below.
					// Note: All Unknown markers that were also filtered, will also be listed
					// in the field input string.
					strFilterMarkersSavedInDoc = field;
							break;
				}
			case 5: // unknown markers string field
				{
					// The doc has not been serialized in yet so we cannot use 
					// GetUnknownMarkersFromDoc() here, so we'll populate the
					// unknown markers arrays here.
					gpApp->m_currentUnknownMarkersStr = field;

					// Initialize the unknown marker data arrays to zero, before we populate them
					// with any unknown markers saved with this document being serialized in
					gpApp->m_unknownMarkers.Clear();
					gpApp->m_filterFlagsUnkMkrs.Clear(); // wxArrayInt

					wxString tempUnkMrksStr = gpApp->m_currentUnknownMarkersStr;

					// Parse out the unknown markers in tempUnkMrksStr 
					wxString unkField, wholeMkr, fStr;

					wxStringTokenizer tkz2(tempUnkMrksStr); // use default " " whitespace here

					while (tkz2.HasMoreTokens())
					{
						unkField = tkz2.GetNextToken();
						// field1 should contain a token in the form of "\xx=0 " or "\xx=1 " 
						int dPos1 = unkField.Find(_T("=0"));
						int dPos2 = unkField.Find(_T("=1"));
						wxASSERT(dPos1 != -1 || dPos2 != -1);
						int dummyIndex;
						if (dPos1 != -1)
						{
							// has "=0", so the unknown marker is unfiltered
							wholeMkr = unkField.Mid(0,dPos1);
							fStr = unkField.Mid(dPos1,2); // get the "=0" filtering delimiter part
							if (!MarkerExistsInArrayString(&gpApp->m_unknownMarkers, wholeMkr, dummyIndex))
							{
								gpApp->m_unknownMarkers.Add(wholeMkr);
								gpApp->m_filterFlagsUnkMkrs.Add(FALSE);
							}
						}
						else
						{
							// has "=1", so the unknown marker is filtered
							wholeMkr = unkField.Mid(0,dPos2);
							fStr = unkField.Mid(dPos2,2); // get the "=1" filtering delimiter part
							if (!MarkerExistsInArrayString(&gpApp->m_unknownMarkers, wholeMkr, dummyIndex))
							{
								gpApp->m_unknownMarkers.Add(wholeMkr);
								gpApp->m_filterFlagsUnkMkrs.Add(TRUE);
							}
						}
					}

					break;
				}
				default:
				{
					// unknown field - ignore
					;
				}
			}
			fieldNum++;
		} // end of while (tkz.HasMoreTokens())
	}
	else if (dataLen < 5) 
	{
		// case 2 above
		// assume we have book mode information - so restore it
		wxChar ch = buffer.GetChar(0);
		if (ch == _T('T'))
			gpApp->m_bBookMode = TRUE;
		else if (ch == _T('F'))
			gpApp->m_bBookMode = FALSE;
		else
		{
			// oops, it's not book mode info, so do the other block instead
			goto t;
		}
		buffer = buffer.Mid(1); // get the index's string
		int i = wxAtoi(buffer);
		gpApp->m_nBookIndex = i;

		// set the BookNamePair pointer, but we don't have enough info for recreating the
		// m_bibleBooksFolderPath here, but SetupDirectories() can recreate it from the
		// doc-serialized m_sourceName and m_targetName strings, and so we do it there;
		// however, if book mode was off when this document was serialized out, then the
		// saved index value was -1, so we must check for this and not try to set up a 
		// name pair when that is the case
		if (i >= 0  && !gpApp->m_bDisableBookMode)
		{
			gpApp->m_pCurrBookNamePair = ((BookNamePair*)(*gpApp->m_pBibleBooks)[i]);
		}
		else
		{
			// it's a -1 index, or the mode is disabled due to a bad parse of the books.xml file, 
			// so ensure no named pair and the folder path is empty
			gpApp->m_pCurrBookNamePair = NULL;
			gpApp->m_bibleBooksFolderPath.Empty();
		}
	}
	else
	{
		// BEW changed 27Nov05, because we only let doc settings be used when MRU was being tried
t:		if (gbTryingMRUOpen /* && !bSameProject */)
		{
			// case 1 above
			// assume we have legacy source text data - for this there was no such thing
			// as book mode in those legacy application versions, so we can have book mode off
			gpApp->m_bBookMode = FALSE;
			gpApp->m_nBookIndex = -1;
			gpApp->m_pCurrBookNamePair = NULL;
			gpApp->m_bibleBooksFolderPath.Empty();
		}
	}

	// whm ammended 6Jul05 below in support of USFM and SFM Filtering
	// Apply any changes to the App's gCurrentSfmSet and gCurrentFilterMarkers indicated
	// by any existing values saved in the Doc's Buffer member
	gpApp->gCurrentSfmSet = SetSavedInDoc;
	gpApp->gCurrentFilterMarkers = strFilterMarkersSavedInDoc;

	// ResetUSFMFilterStructs also calls SetupMarkerStrings() and SetupMarkerStrings
	// builds the various rapid access marker strings including the Doc's unknown marker
	// string pDoc->m_currentUnknownMarkersStr, and adds the unknown markers to the App's
	// gCurrentFilterMarkers string.
	ResetUSFMFilterStructs(gpApp->gCurrentSfmSet, strFilterMarkersSavedInDoc, allInSet);
}


///////////////////////////////////////////////////////////////////////////////
/// \return a wxString
/// \param	pCString	-> pointer to a wxString formatted into delimited fields containing the 
///						book mode, the book index, the current sfm set, a list of the filtered
///						markers, and a list of the unknown markers
/// \remarks
/// Called from: ConstructSettingsInfoAsXML().
/// Creates a wxString composed of delimited fields containing the current book mode, the 
/// book index, the current sfm set, a list of the filtered markers, and a list of the 
/// unknown markers used in the document.
///////////////////////////////////////////////////////////////////////////////
wxString CAdapt_ItDoc::SetupBufferForOutput(wxString* pCString)
{
	// This function encapsulates code which formerly was in the Serialize() function, but now that
	// version 3 allows xml i/o, we need this code for both binary output and for xml output
	pCString = pCString; // to quiet warning
	// wx version: whatever contents pCString had will be ignored below
	wxString buffer; // = *pCString;
	// The legacy app (pre version 2+) used to save the source text to the document file, but this
	// no longer happens; so since doc serializating is not versionable I can use this CString buffer
	// to store book mode info (T for true, F for false, followed by the _itot() conversion of the
	// m_nBookIndex value; and reconstruct these when serializing back in. The doc has to have
	// the book mode info in it, otherwise I cannot make MRU list choices restore the correct state
	// and folder when a document was saved in book mode, from a Bible book folder
	// whm added 26Feb05 in support of USFM and SFM Filtering
	// Similar reasons require that we use this m_pBuffer space to store some things pertaining
	// to USFM and Filtering support that did not exist in the legacy app. The need for this
	// arises due to the fact that with version 3, what is actually adapted in the source text
	// is dependent upon which markers are filtered and on which sfm set the user has chosen,
	// which the user can change at any time.
	wxString strResult;
	// add the version 3 special buffer prefix
	buffer.Empty();
	buffer << _T("@#@#:"); // RestoreDocParamsOnInput case 0:
	// add the book mode
	if (gpApp->m_bBookMode)
	{
		buffer << _T("T:");  // RestoreDocParamsOnInput case 1:
	}
	else
	{
		buffer << _T("F:");  // RestoreDocParamsOnInput case 1:
	}
	// add the book index
	buffer << gpApp->m_nBookIndex;  // RestoreDocParamsOnInput case 2:
	buffer << _T(":");

#ifdef _Trace_FilterMarkers
		TRACE0("In SERIALIZE OUT DOC SAVE:\n");
		TRACE1("   App's gCurrentSfmSet = %d\n",gpApp->gCurrentSfmSet);
		TRACE1("   App's gCurrentFilterMarkers = %s\n",gpApp->gCurrentFilterMarkers);
		TRACE1("   Doc's m_sfmSetBeforeEdit = %d\n",gpApp->m_sfmSetBeforeEdit);
		TRACE1("   Doc's m_filterMarkersBeforeEdit = %s\n",gpApp->m_filterMarkersBeforeEdit);
#endif

	// add the sfm user set enum
	// whm note 6May05: We store the gCurrentSfmSet value, not the gProjectSfmSetForConfig in the doc
	// value which may have been different.
	buffer << (int)gpApp->gCurrentSfmSet;
	buffer << _T(":");

	buffer << gpApp->gCurrentFilterMarkers;
	buffer << _T(":");

	buffer << gpApp->m_currentUnknownMarkersStr;
	buffer << _T(":");
	return buffer;
}

///////////////////////////////////////////////////////////////////////////////
/// \return TRUE if file was successfully saved; FALSE otherwise
/// \param	bShowWaitDlg	-> if TRUE the wait/progress dialog is shown, otherwise it is not shown
/// \remarks
/// Called from: the App's DoAutoSaveDoc(), the Doc's OnFileSave(), OnSaveModified() and
/// OnFilePackDoc(), the View's OnEditConsistencyCheck() and DoConsistencyCheck(), and
/// SplitDialog's SplitAtPhraseBoxLocation_Interactive() and DoSplitIntoChapters().
/// Saves the current document and KB files in XML format and takes care of the necessary 
/// housekeeping involved.
/// Ammended for handling saving when glossing or adapting.
/// BEW modified 13Nov09, if the local user has only read-only access to a remote
/// project folder, the local user must not be able to cause his local copy of the 
/// remote document to be saved to the remote user's disk; if that could happen, the
/// remote user would almost certainly lose some of his edits
///////////////////////////////////////////////////////////////////////////////
bool CAdapt_ItDoc::DoFileSave(bool bShowWaitDlg)
{

	// refactored 9Mar09
	wxFile f; // create a CFile instance with default constructor
	CAdapt_ItApp* pApp = &wxGetApp();
	wxASSERT(pApp != NULL);

	if (pApp->m_bReadOnlyAccess)
	{
		return TRUE; // let the caller think all is well, evene though the save is suppressed
	}

    CAdapt_ItView* pView = (CAdapt_ItView*) GetFirstView();
	
	// make the working directory the "Adaptations" one; or the current Bible book folder
	// if the m_bBookMode flag is TRUE

	// There are at least three ways within wxWidgets to change the current
	// working directory:
	// (1) Use ChangePathTo() method of the wxFileSystem class,
	// (2) Use the static SetCwd() method of the wxFileName class,
	// (3) Use the global namespace method ::wxSetWorkingDirectory()
	// We'll regularly use ::wxSetWorkingDirectory()
	bool bOK;
	if (pApp->m_bBookMode && !pApp->m_bDisableBookMode)
	{
		// save to the folder specified by app's member  m_bibleBooksFolderPath
		bOK = ::wxSetWorkingDirectory(pApp->m_bibleBooksFolderPath);
	}
	else
	{
		// do legacy save, to the Adaptations folder
		bOK = ::wxSetWorkingDirectory(pApp->m_curAdaptionsPath);
	}
	if (!bOK)
	{
		// ??? Should we just return FALSE or give the user the following error message ???
		// Comment out the error message below if it is redundant
		wxMessageBox(_(
		"Failed to set the current directory to the Adaptations folder. Command aborted."),
		_T(""), wxICON_EXCLAMATION);
		return FALSE;
	}

    // if the phrase box is visible and has the focus, then its contents will have been
    // removed from the KB, so we must restore them to the KB, then after the save is done,
    // remove them again; but only provided the pApp->m_targetBox's window exists
    // (otherwise GetStyle call will assert)
	bool bNoStore = FALSE;
	bOK = FALSE;

	// In code below simply calling if (m_targetBox) or if (m_targetBox != NULL)
	// should be a sufficient test. 
	if (pApp->m_pTargetBox != NULL)
	{
		if (pApp->m_pTargetBox->IsShown())// not focused on app closure
		{
			if (!gbIsGlossing)
			{
				pView->MakeLineFourString(pApp->m_pActivePile->GetSrcPhrase(),pApp->m_targetPhrase);
				pView->RemovePunctuation(this,&pApp->m_targetPhrase,from_target_text); //1 = from tgt
			}
			gbInhibitLine4StrCall = TRUE;
			bOK = pView->StoreText(pView->GetKB(),pApp->m_pActivePile->GetSrcPhrase(),pApp->m_targetPhrase);
			gbInhibitLine4StrCall = FALSE;
			if (!bOK)
			{
				// something is wrong if the store did not work, but we can tolerate the error 
				// & continue
				// IDS_KB_STORE_FAIL
				wxMessageBox(_(
"Warning: the word or phrase was not stored in the knowledge base. This error is not destructive and can be ignored."),
				_T(""),wxICON_EXCLAMATION);
				bNoStore = TRUE;
			}
			else
			{
				if (gbIsGlossing)
					pApp->m_pActivePile->GetSrcPhrase()->m_bHasGlossingKBEntry = TRUE;
				else
					pApp->m_pActivePile->GetSrcPhrase()->m_bHasKBEntry = TRUE;
			}
		}
	}

	bool bFailed = FALSE;
	
	// ensure the extension is what it should be (what was opened may be a file of different
	// type than what is to be output now, and if so then the extension must be brought into
	// line with what the value of the m_bSaveAsXML flag happens to be, that is, .xml if that
	// flag is TRUE, or .adt if it is FALSE)
	wxString thisFilename = gpApp->m_curOutputFilename;
	if (gpApp->m_bSaveAsXML) // always true in the wx version
	{
		// we want an .xml extension - make it so if it happens to be .adt
		thisFilename = MakeReverse(thisFilename);
		wxString extn = thisFilename.Left(3);
		extn = MakeReverse(extn);
		if (extn != _T("xml"))
		{
			thisFilename = thisFilename.Mid(3); // remove the adt extension
			thisFilename = MakeReverse(thisFilename);
			thisFilename += _T("xml"); // it's now *.xml
		}
		else
			thisFilename = MakeReverse(thisFilename); // it's already *.xml
	}

	gpApp->m_curOutputFilename = thisFilename;	// m_curOutputFilename now complies with the 
												// m_bSaveAsXML flag's value

	// make sure the backup filenames comply too (BEW added 23June07)
	MakeOutputBackupFilenames(gpApp->m_curOutputFilename,gpApp->m_bSaveAsXML);

	// the m_curOutputPath member can be redone now that m_curOutputFilename is what is wanted
	if (gpApp->m_bBookMode && !gpApp->m_bDisableBookMode)
	{
		gpApp->m_curOutputPath = gpApp->m_bibleBooksFolderPath + 
									gpApp->PathSeparator + gpApp->m_curOutputFilename;
	}
	else
	{
		gpApp->m_curOutputPath = gpApp->m_curAdaptionsPath + 
									gpApp->PathSeparator + gpApp->m_curOutputFilename;
	}


    // m_curOutputFilename was set when user created the doc; or it an existing doc was
    // read back in, then code above will have made the extension conform to the
    // m_bSaveAsXML flag's value if it had the other extension (ie. binary when xml is
    // wanted, or xml when binary is wanted)
	if (!f.Open(gpApp->m_curOutputFilename,wxFile::write))
	{
		return FALSE; // if we get here, we'll miss unstoring from the KB, but its not likely
					  // to happen, so we'll not worry about it - it wouldn't matter much anyway
	}

	CSourcePhrase* pSrcPhrase;
	CBString aStr;
	CBString openBraceSlash = "</"; // to avoid "warning: deprecated conversion from string constant to 'char*'"

	// prologue (Changed BEW 02July07 at Bob Eaton's request)
	gpApp->GetEncodingStringForXmlFiles(aStr);
	DoWrite(f,aStr);

	// add the comment with the warning about not opening the XML file in MS WORD 
	// 'coz is corrupts it - presumably because there is no XSLT file defined for it
	// as well. When the file is then (if saved in WORD) loaded back into Adapt It,
	// the latter goes into an infinite loop when the file is being parsed in.
	aStr = MakeMSWORDWarning(); // the warning ends with \r\n so we don't need to add them here

	// doc opening tag
	aStr += "<";
	aStr += xml_adaptitdoc;
	aStr += ">\r\n"; // eol chars OK for cross-platform???
	DoWrite(f,aStr);

	// place the <Settings> element at the start of the doc
	aStr = ConstructSettingsInfoAsXML(1);
	DoWrite(f,aStr);

	int counter;
	counter = 0;
	int nTotal = gpApp->m_pSourcePhrases->GetCount();
	wxString progMsg = _("%s  - %d of %d Total words and phrases");
	wxString msgDisplayed = progMsg.Format(progMsg,gpApp->m_curOutputFilename.c_str(),1,nTotal);
	wxProgressDialog* pProgDlg;
	pProgDlg = (wxProgressDialog*)NULL;
	if (bShowWaitDlg)
	{
#ifdef __WXMSW__
		// whm note 27May07: Saving long documents takes some noticeable time, so I'm adding a
		// progress dialog here (not done in the MFC version)
		//wxProgressDialog progDlg(_("Saving File"),
		pProgDlg = new wxProgressDialog(_("Saving File"),
						msgDisplayed,
						nTotal,    // range
						gpApp->GetMainFrame(),   // parent
						//wxPD_CAN_ABORT |
						//wxPD_CAN_SKIP |
						wxPD_APP_MODAL |
						// wxPD_AUTO_HIDE | -- try this as well
						wxPD_ELAPSED_TIME |
						wxPD_ESTIMATED_TIME |
						wxPD_REMAINING_TIME
						| wxPD_SMOOTH // - makes indeterminate mode bar on WinXP very small
						);

#else
		// wxProgressDialog tends to hang on wxGTK so I'll just use the simpler CWaitDlg
		// notification on wxGTK and wxMAC
		// put up a Wait dialog - otherwise nothing visible will happen until the operation is done
		CWaitDlg waitDlg(gpApp->GetMainFrame());
		// indicate we want the reading file wait message
		waitDlg.m_nWaitMsgNum = 4;	// 4 "Please wait while Adapt It Saves the File..."
		waitDlg.Centre();
		waitDlg.Show(TRUE);
		waitDlg.Update();
		// the wait dialog is automatically destroyed when it goes out of scope below.
#endif
	}
	// add the list of sourcephrases
	SPList::Node* pos = gpApp->m_pSourcePhrases->GetFirst();
	while (pos != NULL)
	{
		if (bShowWaitDlg)
		{
#ifdef __WXMSW__
			counter++;
			if (counter % 1000 == 0) 
			{
				msgDisplayed = progMsg.Format(progMsg,gpApp->m_curOutputFilename.c_str(),counter,nTotal);
				pProgDlg->Update(counter,msgDisplayed);
			}
#endif
		}
		pSrcPhrase = (CSourcePhrase*)pos->GetData();
		pos = pos->GetNext();
		aStr = pSrcPhrase->MakeXML(1); // 1 = indent the element lines with a single tab
		DoWrite(f,aStr);
	}

	// doc closing tag
	aStr = xml_adaptitdoc;
	aStr = openBraceSlash + aStr; //"</" + aStr;
	aStr += ">\r\n"; // eol chars OK for cross-platform???
	DoWrite(f,aStr);

	// close the file
	f.Close();
	f.Flush();

	// We won't worry about any .adt files in WX version

    // recompute m_curOutputPath, so it can be saved to config files as m_lastDocPath,
    // because the path computed at the end of OnOpenDocument() will have been invalidated
    // if the filename extension was changed by code earlier in DoFileSave()
	if (gpApp->m_bBookMode && !gpApp->m_bDisableBookMode)
	{
		gpApp->m_curOutputPath = pApp->m_bibleBooksFolderPath + 
									gpApp->PathSeparator + gpApp->m_curOutputFilename;
	}
	else
	{
		gpApp->m_curOutputPath = pApp->m_curAdaptionsPath + 
									gpApp->PathSeparator + gpApp->m_curOutputFilename;
	}
	gpApp->m_lastDocPath = gpApp->m_curOutputPath; // make it agree with what path was 
												   // used for this save operation
	if (bShowWaitDlg)
	{
#ifdef __WXMSW__
		progMsg = _("Please wait while Adapt It saves the KB...");
		pProgDlg->Pulse(progMsg); // more general message during KB save
#endif
	}
	// do the document backup if required
	if (gpApp->m_bBackupDocument)
	{
		bool bBackedUpOK = BackupDocument(gpApp);
		if (!bBackedUpOK)
			//IDS_DOC_BACKUP_FAILED
			wxMessageBox(_(
			"Warning: the attempt to backup the current document failed."),
			_T(""), wxICON_EXCLAMATION);
	}

	Modify(FALSE); // declare the document clean
	if (gpApp->m_bBookMode && !gpApp->m_bDisableBookMode)
		SetFilename(pApp->m_bibleBooksFolderPath+pApp->PathSeparator + 
			pApp->m_curOutputFilename,TRUE); // TRUE = notify all views
	else
		SetFilename(pApp->m_curAdaptionsPath+pApp->PathSeparator + 
			pApp->m_curOutputFilename,TRUE); // TRUE = notify all views

    // the KBs (whether glossing KB or normal KB) must always be kept up to date with a
    // file, so must store both KBs, since the user could have altered both since the last
    // save

	gpApp->StoreGlossingKB(FALSE); // FALSE = don't want backup produced
	gpApp->StoreKB(FALSE);
	
	// remove the phrase box's entry again (this code is sensitive to whether glossing is on
	// or not, because it is an adjustment pertaining to the phrasebox contents only, to undo
	// what was done above - namely, the entry put into either the glossing KB or the normal KB)
	if (pApp->m_pTargetBox != NULL)
	{
		if (pApp->m_pTargetBox->IsShown() && 
			pView->GetFrame()->FindFocus() == (wxWindow*)pApp->m_pTargetBox && !bNoStore)
		{
			CRefString* pRefString;
			if (gbIsGlossing)
			{
				if (!bNoStore)
				{
					pRefString = pView->GetRefString(pView->GetKB(), 1,
											pApp->m_pActivePile->GetSrcPhrase()->m_key,
											pApp->m_pActivePile->GetSrcPhrase()->m_gloss);
					pView->RemoveRefString(pRefString,pApp->m_pActivePile->GetSrcPhrase(), 1);
				}
				pApp->m_pActivePile->GetSrcPhrase()->m_bHasGlossingKBEntry = FALSE;
			}
			else
			{
				if (!bNoStore)
				{
					pRefString = pView->GetRefString(pView->GetKB(),
											pApp->m_pActivePile->GetSrcPhrase()->m_nSrcWords,
											pApp->m_pActivePile->GetSrcPhrase()->m_key,
											pApp->m_pActivePile->GetSrcPhrase()->m_adaption);
					pView->RemoveRefString(pRefString,pApp->m_pActivePile->GetSrcPhrase(),
											pApp->m_pActivePile->GetSrcPhrase()->m_nSrcWords);
				}
				pApp->m_pActivePile->GetSrcPhrase()->m_bHasKBEntry = FALSE;
			}
		}
	}

#ifdef __WXMSW__
	if (pProgDlg != NULL)
		pProgDlg->Destroy();
#endif

	if (bFailed)
		return FALSE;
	else
		return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
/// \return TRUE if file at path was successfully saved; FALSE otherwise
/// \param	path	-> path of the file to be saved
/// \remarks
/// Called from: the App's DoTransformationsToGlosses( ) in order to save another project's 
/// document, which has just had its adaptations transformed into glosses in the current
/// project. The full path is passed in - it will have been made an *.xml path in the
/// caller.
/// We don't have to worry about the view, since the document is not visible during any part of the 
/// transformation process.
/// We return TRUE if all went well, FALSE if something went wrong; but so far the caller makes no
/// use of the returned Boolean value and just assumes the function succeeded.
/// The save is done to a Bible book folder when that is appropriate, whether or not book mode is
/// currently in effect.
///////////////////////////////////////////////////////////////////////////////
bool CAdapt_ItDoc::DoTransformedDocFileSave(wxString path)
{
	wxFile f; // create a CFile instance with default constructor
	bool bFailed = FALSE;
	
	if (!f.Open(path,wxFile::write))
	{
		wxString s;
		s = s.Format(_(
		"When transforming documents, the Open function failed, for the path: %s"),
			path.c_str()); 
		wxMessageBox(s,_T(""), wxICON_EXCLAMATION);
		return FALSE;
	}

	if (gpApp->m_bSaveAsXML) // always true in the wx version
	{
		CSourcePhrase* pSrcPhrase;
		CBString aStr;
		CBString openBraceSlash = "</"; // to avoid "warning: 
					// deprecated conversion from string constant to 'char*'"

		// prologue (BEW changed 02July07)
		gpApp->GetEncodingStringForXmlFiles(aStr);
		DoWrite(f,aStr);
	
		// add the comment with the warning about not opening the XML file in MS WORD 
		// 'coz is corrupts it - presumably because there is no XSLT file defined for it
		// as well. When the file is then (if saved in WORD) loaded back into Adapt It,
		// the latter goes into an infinite loop when the file is being parsed in.
		aStr = MakeMSWORDWarning(); // the warning ends with \r\n so 
									// we don't need to add them here
	
		// doc opening tag
		aStr += "<";
		aStr += xml_adaptitdoc;
		aStr += ">\r\n"; // eol chars OK in cross-platform version ???
		DoWrite(f,aStr);

		// place the <Settings> element at the start of the doc
		aStr = ConstructSettingsInfoAsXML(1);
		DoWrite(f,aStr);

		// add the list of sourcephrases
		SPList::Node* pos = gpApp->m_pSourcePhrases->GetFirst();
		while (pos != NULL)
		{
			pSrcPhrase = (CSourcePhrase*)pos->GetData();
			pos = pos->GetNext();
			aStr = pSrcPhrase->MakeXML(1); // 1 = indent the element lines with a single tab
			DoWrite(f,aStr);
		}

		// doc closing tag
		aStr = xml_adaptitdoc;
		aStr = openBraceSlash + aStr; //"</" + aStr;
		aStr += ">\r\n"; // eol chars OK in cross-platform version ???
		DoWrite(f,aStr);

		// close the file
		f.Close();
		f.Flush();
	}
	if (bFailed)
		return FALSE;
	else
		return TRUE;
}


///////////////////////////////////////////////////////////////////////////////
/// \return TRUE if currently opened document was successfully saved; FALSE otherwise
/// \remarks
/// Called from: the Doc's OnFileClose() and CMainFrame's OnMRUFile().
/// Takes care of saving a modified document, saving the project configuration file, and 
/// other housekeeping tasks related to file saves.
/// BEW modified 13Nov09: if local user has read-only access to a remote project
/// folder, don't let any local actions result in saving the local copy of the document
/// on the remote machine, otherwise some loss of edits may happen
///////////////////////////////////////////////////////////////////////////////
bool CAdapt_ItDoc::OnSaveModified()
{
	// save project configuration fonts and settings
	CAdapt_ItApp* pApp = &wxGetApp();

	if (pApp->m_bReadOnlyAccess)
	{
		return TRUE; // make the caller think a save etc was done
	}

	wxCommandEvent dummyevent;

	// should not close a document or project while in "show target only" mode; so detect if that 
	// mode is still active & if so, restore normal mode first
	if (gbShowTargetOnly)
	{
		//restore normal mode
		pApp->GetView()->OnFromShowingTargetOnlyToShowingAll(dummyevent);
	}

	// get name/title of document
	wxString name = pApp->m_curOutputFilename;

	wxString prompt;
	bool bUserSavedDoc = FALSE; // use this flag to cause the KB to be automatically saved
								// if the user saves the Doc, without asking; but if the user
								// does not save the doc, he should be asked for the KB
	bool bOK; // we won't care whether it succeeds or not, 
			  // since the later Get... can use defaults
	if (!pApp->m_curProjectPath.IsEmpty())
	{
		if (pApp->m_bUseCustomWorkFolderPath && !pApp->m_customWorkFolderPath.IsEmpty())
		{
			// whm 10Mar10, must save using what paths are current, but when the custom
			// location has been locked in, the filename lacks "Admin" in it, so that it
			// becomes a "normal" project configuration file in m_curProjectPath at the 
			// custom location.
			if (pApp->m_bLockedCustomWorkFolderPath)
				bOK = pApp->WriteConfigurationFile(szProjectConfiguration, pApp->m_curProjectPath,projectConfigFile);
			else
				bOK = pApp->WriteConfigurationFile(szAdminProjectConfiguration, pApp->m_curProjectPath,projectConfigFile);
		}
		else
		{
			bOK = pApp->WriteConfigurationFile(szProjectConfiguration, pApp->m_curProjectPath,projectConfigFile);
		}
		// original code below
		//bOK = pApp->WriteConfigurationFile(szProjectConfiguration,pApp->m_curProjectPath,projectConfigFile);
	}

	bool bIsModified = IsModified();
	if (!bIsModified)
		return TRUE;        // ok to continue

	// BEW added 11Aug06; for some reason IsModified() returns TRUE in the situation when the
	// user first launches the app, creates a project but cancels out of document creation, and
	// then closes the application by clicking the window's close box at top right. We don't
	// want MFC to put up the message "Save changes for ?", because if the user says OK, then
	// the app crashes. So we detect an empty document next and prevent the message from appearing
	if (pApp->m_pSourcePhrases->GetCount() == 0)
	{
		// if there are none, there is no document to save, so useless to go on, 
		// so return TRUE immediately
		return TRUE;
	}

	prompt = prompt.Format(_("The document %s has changed. Do you want to save it? "),name.c_str());
	int result = wxMessageBox(prompt, _T(""), wxYES_NO | wxCANCEL); //AFX_IDP_ASK_TO_SAVE
	switch (result)
	{
	case wxCANCEL:
		return FALSE;       // don't continue

	case wxYES:
		// If so, either Save or Update, as appropriate
		bUserSavedDoc = DoFileSave(TRUE); // TRUE - show wait/progress dialog	
		if (!bUserSavedDoc)
		{
			wxMessageBox(_("Warning: document save failed for some reason.\n"),
							_T(""), wxICON_EXCLAMATION);
			return FALSE;       // don't continue
		}
		break;

	case wxNO:
		// If not saving changes, revert the document (& ask for a KB save)
		break;

	default:
		wxASSERT(FALSE);
		break;
	}
	return TRUE;    // keep going	
}

////////////////////////////////////////////////////////////////////////////////////////
// NOTE: This OnSaveDocument() is from the docview sample program. 
//
// The wxWidgets OnSaveDocument() method "Constructs an output file stream 
// for the given filename (which must not be empty), and then calls SaveObject. 
// If SaveObject returns TRUE, the document is set to unmodified; otherwise, 
// an error message box is displayed.
//
//bool CAdapt_ItDoc::OnSaveDocument(const wxString& filename) // from wxWidgets mdi sample
//{
//    CAdapt_ItView* view = (CAdapt_ItView*) GetFirstView();
//
//    if (!view->textsw->SaveFile(filename))
//        return FALSE;
//    Modify(FALSE);
//    return TRUE;
//}

// below is code from the docview sample's original override (which doesn't call the
// base class member) converted to the first Adapt It prototype.
// The wxWidgets OnOpenDocument() "Constructs an input file stream
// for the given filename (which must not be empty), and calls LoadObject().
// If LoadObject returns TRUE, the document is set to unmodified; otherwise,
// an error message box is displayed. The document's views are notified that
// the filename has changed, to give windows an opportunity to update their
// titles. All of the document's views are then updated."
//
//bool CAdapt_ItDoc::OnOpenDocument(const wxString& filename) // from wxWidgets mdi sample
//{
//    CAdapt_ItView* view = (CAdapt_ItView*) GetFirstView();
//    
//    if (!view->textsw->LoadFile(filename)) 
//        return FALSE;
//
//    SetFilename(filename, TRUE);
//    Modify(FALSE);
//    UpdateAllViews();
//    
//    return TRUE;
//}

////////////////////////////////////////////////////////////////////////////////////////
// NOTE: The differences in design between MFC's doc/view framework
// and the wxWidgets implementation of doc/view necessitate some
// adjustments in order to not foul up the state of AI's data structures.
//
// Here below is the contents of the MFC base class CDocument::OnOpenDocument() 
// method (minus __WXDEBUG__ statements):
//BOOL CDocument::OnOpenDocument(LPCTSTR lpszPathName)
//{
//	CFileException fe;
//	CFile* pFile = GetFile(lpszPathName,
//		CFile::modeRead|CFile::shareDenyWrite, &fe);
//	if (pFile == NULL)
//	{
//		ReportSaveLoadException(lpszPathName, &fe,
//			FALSE, AFX_IDP_FAILED_TO_OPEN_DOC);
//		return FALSE;
//	}
//	DeleteContents();
//	SetModifiedFlag();  // dirty during de-serialize
//
//	CArchive loadArchive(pFile, CArchive::load | CArchive::bNoFlushOnDelete);
//	loadArchive.m_pDocument = this;
//	loadArchive.m_bForceFlat = FALSE;
//	TRY
//	{
//		CWaitCursor wait;
//		if (pFile->GetLength() != 0)
//			Serialize(loadArchive);     // load me
//		loadArchive.Close();
//		ReleaseFile(pFile, FALSE);
//	}
//	CATCH_ALL(e)
//	{
//		ReleaseFile(pFile, TRUE);
//		DeleteContents();   // remove failed contents
//
//		TRY
//		{
//			ReportSaveLoadException(lpszPathName, e,
//				FALSE, AFX_IDP_FAILED_TO_OPEN_DOC);
//		}
//		END_TRY
//		DELETE_EXCEPTION(e);
//		return FALSE;
//	}
//	END_CATCH_ALL
//
//	SetModifiedFlag(FALSE);     // start off with unmodified
//
//	return TRUE;
//}

// Here below is the contents of the wxWidgets base class WxDocument::OnOpenDocument() 
// method (minus alternate wxUSE_STD_IOSTREAM statements):
//bool wxDocument::OnOpenDocument(const wxString& file)
//{
//    if (!OnSaveModified())
//        return FALSE;
//
//    wxString msgTitle;
//    if (wxTheApp->GetAppName() != wxT(""))
//        msgTitle = wxTheApp->GetAppName();
//    else
//        msgTitle = wxString(_("File error"));
//
//    wxFileInputStream store(file);
//    if (store.GetLastError() != wxSTREAM_NO_ERROR)
//    {
//        (void)wxMessageBox(_("Sorry, could not open this file."), msgTitle, wxOK|wxICON_EXCLAMATION,
//                           GetDocumentWindow());
//        return FALSE;
//    }
//    int res = LoadObject(store).GetLastError();
//    if ((res != wxSTREAM_NO_ERROR) &&
//        (res != wxSTREAM_EOF))
//    {
//        (void)wxMessageBox(_("Sorry, could not open this file."), msgTitle, wxOK|wxICON_EXCLAMATION,
//                           GetDocumentWindow());
//        return FALSE;
//    }
//    SetFilename(file, TRUE);
//    Modify(FALSE);
//    m_savedYet = TRUE;
//
//    UpdateAllViews();
//
//    return TRUE;
//}

// The significant differences in the BASE class methods are:
// 1. MFC OnOpenDocument() calls DeleteContents() before loading the archived 
//    file (with Serialize(loadArchive)). The base class DeleteContents() of
//    both MFC and wxWidgets do nothing themselves. The overrides of DeleteContents()
//    have the same code in both versions.
// 2. wxWidgets' OnOpenDocument() does NOT call DeleteContents(), but first calls 
//    OnSaveModified(). OnSaveModified() calls Save() if the doc is dirty. Save() 
//    calls either SaveAs() or OnSaveDocument() depending on whether the doc was 
//    previously saved with a name. SaveAs() takes care of getting a name from 
//    user, then eventually also calls OnSaveDocument(). OnSaveDocument() finally
//    calls SaveObject(store).
// The Implications for our conversion to wxWidgets:
// 1. In our OnOpenDocument() override we need to first call DeleteContents().
// 2. We just comment out the call to the wxDocument::OnOpenDocument() base class 
//    transfer its calls to our override and make any appropriate adjustments to
//    the stream error messages.

///////////////////////////////////////////////////////////////////////////////
/// \return TRUE if file at filename was successfully opened; FALSE otherwise
/// \param	filename	-> the path/name of the file to open
/// \remarks
/// Called from: the App's DoKBRestore() and DiscardDocChanges(), the Doc's 
/// LoadSourcePhraseListFromFile() and DoUnpackDocument(), the View's OnEditConsistencyCheck(),
/// DoConsistencyCheck() and DoRetranslationReport(), the DocPage's OnWizardFinish(), and
/// CMainFrame's SyncScrollReceive() and OnMRUFile().
/// Opens the document at filename and does the necessary housekeeping and initialization of
/// KB data structures for an open document.
/// [see also notes within in the function]
/// BEW added 13Nov09: call of m_bReadOnlyAccess = SetReadOnlyProtection(), in order to give
/// the local user in the project ownership for writing permission (if FALSE is returned)
/// or READ-ONLY access (if TRUE is returned). (Also added to LoadKB() and OnNewDocument()
/// and OnCreate() for the view class.)
///////////////////////////////////////////////////////////////////////////////
bool CAdapt_ItDoc::OnOpenDocument(const wxString& filename) 
{
	// refactored 10Mar09
	gpApp->m_nSaveActiveSequNum = 0; // reset to a default initial value, safe for any length of doc 
	
    // whm Version 3 Note: Since the WX version i/o is strictly XML, we do not need nor use
    // the legacy version's OnOpenDocument() serialization facilities, and can thus avoid
    // the black box problems it caused.
	// Old legacy version notes below:
	// The MFC code called the virtual methods base class OnOpenDocument here:
	//if (!CDocument::OnOpenDocument(lpszPathName)) // The MFC code
	//	return FALSE;
	// The wxWidgets equivalent is:
	//if (!wxDocument::OnOpenDocument(filename))
	//	return FALSE;
	// wxWidgets Notes:
	// 1. The wxWidgets base class wxDocument::OnOpenDocument() method DOES NOT
	//    automatically call DeleteContents(), so we must do so here. Also,
	// 2. The OnOpenDocument() base class handles stream errors with some
	//    generic messages. For these reasons then, rather than calling the base 
	//    class method, we first call DeleteContents(), then we just transfer 
	//    and/or merge the relevant contents of the base class method here, and 
	//    taylor its stream error messages to Adapt It's needs, as was done in 
	//    DoFileSave().

	// BEW added 06Aug05 for XML doc support (we have to find out what extension it has
	// and then choose the corresponding code for loading that type of doc
	// BEW modified 14Nov05 to add the doc instance to the XML doc reading call, and to
	// remove the assert which assumed that there would always be a backslash in the
	// lpszPathName string, and replace it with a test on curPos instead (when doing a
	// consistency check, the full path is not passed in)

	gnBeginInsertionsSequNum = -1; // reset for "no current insertions"
	gnEndInsertionsSequNum = -1; // reset for "no current insertions"

	wxString thePath = filename;
	wxString extension = thePath.Right(4);
	extension.MakeLower();
	wxASSERT(extension[0] == _T('.')); // check it really is an extension
	bool bWasXMLReadIn = TRUE;

	bool bBookMode;
	bBookMode = gpApp->m_bBookMode; // for debugging only. 01Oct06
	int nItsIndex;
	nItsIndex = gpApp->m_nBookIndex; // for debugging only

	// get the filename
	wxString fname = thePath;
	fname = MakeReverse(fname);
	int curPos = fname.Find(gpApp->PathSeparator);
	if (curPos != -1)
	{
		fname = fname.Left(curPos);
	}
	fname = MakeReverse(fname);
	wxString extensionlessName;

    // Note: the m_bSaveAsXML flag could be TRUE or FALSE; so the user may well have opened
    // a binary file when xml saves are wanted, or opened an xml file when binary saves are
    // wanted. Our approach here is to just assume that what he chose to open is
    // appropriate for the m_bSaveAsXML flag value, even if it isn't, and we set up the
    // m_curOutputFilename, m_curOutputPath, and the backup filename & alternate filenames
    // with the same assumptions. We actually do the check for compliance with the
    // m_bSaveAsXML flag's value in DoFileSave() and there we make any needed switches of
    // filename extension and reconstruct the path, and so forth to for the backup
    // filenames (BEW added note 23June07 after scratching my head in perplexity for a long
    // while because the code had gone cold. I've also added a MakeOutputBackupFilenames()
    // to the doc class to help get these backup filenames correct in all circumstances
    // where BackupDocument() is called, as I found that it was not being done right for
    // Split Document and/or Join Document, nor in DoFileSave() -- the new function looks
    // at the m_bSaveAsXML value and grabs m_curOutputFilename, and makes backup filenames
    // which comply)
	if (extension == _T(".xml"))
	{
		// we have to input an xml document
		// BEW modified 07Nov05, to add pointer to the document, since we may be reading
		// in XML documents for joining to the current document, and we want to
		// populate the correct document's CSourcePhrase list
		wxString thePath = filename;
		wxFileName fn(thePath);
		wxString fullFileName;
		fullFileName = fn.GetFullName();
		bool bReadOK = ReadDoc_XML(thePath,this);
		if (!bReadOK)
		{
			wxString s;
			if (gbTryingMRUOpen)
			{
				// a nice warm & comfy message about the file perhaps not actually existing
				// any longer will keep the user from panic
				// IDS_MRU_NO_FILE
				s = _(
"The file you clicked could not be opened. It probably no longer exists. When you click OK the Start Working... wizard will open to let you open a project and document from there instead.");
				wxMessageBox(s, fullFileName, wxICON_INFORMATION);
				wxCommandEvent dummyevent;
				OnFileOpen(dummyevent); // have another go, via the Start Working wizard
				return TRUE;
			}
			else
			{
				// uglier message because we expect a good read, but we allow the user to continue
				// IDS_XML_READ_ERR
				s = _(
"There was an error parsing in the XML file.\nIf you edited the XML file earlier, you may have introduced an error.\nEdit it in a word processor then try again.");
				wxMessageBox(s, fullFileName, wxICON_INFORMATION);
			}
			return TRUE; // return TRUE to allow the user another go at it
		}
	}

	if (gpApp->m_bWantSourcePhrasesOnly) return TRUE; // Added by JF.  
        // From here on in for the rest of this function, all we do is set globals,
        // filenames, config file parameters, and change the view, all things we're not to
        // do if m_bWantSourcePhrasesOnly is set. Hence, we simply exit early; because all
        // we are wanting is the list of CSourcePhrase instances.
        
	// update the window title
	SetDocumentWindowTitle(fname, extensionlessName);

	// set the document's member for storing the filename etc - according to what the
	// extension was on the chosen doc, then use UpdateFilenamesAndPaths() to get everything
	// in agreement with the m_bSaveAsXML flag's value, since this value and the extension on
	// the read in file may not be in agreement (eg. the flag may be TRUE, but the user may
	// have clicked an .adt document produced by a legacy version of the app)
	wxFileName fn(filename);
	wxString filenameStr = fn.GetFullName(); //GetFileName(filename);
	if (bWasXMLReadIn)
	{
		// it was an *.xml file the user opened
		gpApp->m_curOutputFilename = filenameStr;

		// construct the backup's filename
		// BEW changed 23June07 to allow for the possibility that more than one period
		// may be used in a filename
		filenameStr = MakeReverse(filenameStr);
		filenameStr.Remove(0,4); //filenameStr.Delete(0,4); // remove "lmx."
		filenameStr = MakeReverse(filenameStr);
		filenameStr += _T(".BAK");
		filenameStr += _T(".xml"); // produces *.BAK.xml
	}
	gpApp->m_curOutputBackupFilename = filenameStr;
	gpApp->m_curOutputPath = filename;

    // we also calculate the alternate backup name too, because it is possible that the
    // backup file is binary when the doc is xml, or vise versa, or both may be the same
    // type; so we must allow the backup one to be either type, and compute
    // m_curOutputBackupFilename and m_altOutputBackupFilename to allow for either
    // possibility
	wxString thisBackupFilename = gpApp->m_curOutputBackupFilename;
	if (bWasXMLReadIn)
	{
		// m_curOutputBackupFilename is of the form *.BAK.xml
		thisBackupFilename = MakeReverse(thisBackupFilename);
		int nFound = thisBackupFilename.Find(_T('.'));
		wxString extn = thisBackupFilename.Left(nFound);
		extn = MakeReverse(extn);
		if (extn != _T("xml"))
		{
			// it found BAK at the end
			thisBackupFilename = MakeReverse(thisBackupFilename);
			gpApp->m_altOutputBackupFilename = thisBackupFilename;// the *.BAK filename is the alternative
		}
		else
		{
			// found xml at the end
			thisBackupFilename = thisBackupFilename.Mid(4); // remove the ".xml" (reversed) at the start
			thisBackupFilename = MakeReverse(thisBackupFilename);
			gpApp->m_altOutputBackupFilename = thisBackupFilename; // alternative now has the form *.BAK
		}
	}
    // filenames and paths for the doc and any backup are now what they should be - which
    // might be different w.r.t. extensions from what the user actually used when opening
    // the doc
	CAdapt_ItApp* pApp = GetApp();
	CAdapt_ItView* pView = pApp->GetView();
//#ifdef __WXDEBUG__
//	wxLogDebug(_T("OnOpenDocument at %d ,  Active Sequ Num  %d"),1,pApp->m_nActiveSequNum);
//#endif

	int width = wxSystemSettings::GetMetric(wxSYS_SCREEN_X);
	if (pApp->m_docSize.GetWidth() < 100 || pApp->m_docSize.GetWidth() > width)
	{
		::wxBell(); // tell me it was wrong
		pApp->m_docSize = wxSize(width - 40,600); // ensure a correctly sized document
		pApp->GetMainFrame()->canvas->SetVirtualSize(pApp->m_docSize);
	}

	// refactored version: try the following here
	CLayout* pLayout = GetLayout();
	pLayout->SetLayoutParameters(); // calls InitializeCLayout() and UpdateTextHeights()
									// and other setters
#ifdef _NEW_LAYOUT
	bool bIsOK = pLayout->RecalcLayout(pApp->m_pSourcePhrases, create_strips_and_piles);
#else
	bool bIsOK = pLayout->RecalcLayout(pApp->m_pSourcePhrases, create_strips_and_piles);
#endif
	if (!bIsOK)
	{
		// unlikely to fail, so just have something for the developer here
		wxMessageBox(_T("Error. RecalcLayout(TRUE) failed in OnOpenDocument()"),
		_T(""),wxICON_STOP);
		wxASSERT(FALSE);
		wxExit();
	}

	if (pApp->m_pSourcePhrases->GetCount() == 0)
	{
		// nothing to show
		// IDS_NO_DATA
		wxMessageBox(_(
"There is no data in this file. This document is not properly formed and so cannot be opened. Delete it."),
		_T(""), wxICON_EXCLAMATION);
		return FALSE;
	}
	pApp->m_pActivePile = GetPile(0); // a safe default for starters....
	pApp->m_nActiveSequNum = 0; // and this is it's sequ num; use these values 
								// unless changed below
	// BEW added 21Apr08; clean out the global struct gEditRecord & clear its deletion lists,
	// because each document, on opening it, it must start with a truly empty EditRecord; and
	// on doc closure and app closure, it likewise must be cleaned out entirely (the deletion
	// lists in it have content which persists only for the life of the document currently open)
	pView->InitializeEditRecord(gEditRecord);
	gEditRecord.deletedAdaptationsList.Clear(); // remove any stored deleted adaptation strings
	gEditRecord.deletedGlossesList.Clear(); // remove any stored deleted gloss strings
	gEditRecord.deletedFreeTranslationsList.Clear(); // remove any stored deleted free translations

	// if we get here by having chosen a document file from the Recent_File_List, then it is
	// possible to in that way to choose a file from a different project; so the app will crash
	// unless we here set up the required directory structures and load the document's KB
	if (pApp->m_pKB == NULL)
	{
		//ensure we have the right KB & project (the parameters SetupDirectories() needs are
		// stored in the document, so will be already serialized back in; and our override of
		// CWinApp's OnOpenRecentFile() first does a CloseProject (which ensures m_pKB gets set to 
		// null) before OnOpenDocument is called. WX Note: We override CMainFrame's OnMRUFile().
		// If we did not come via a File... MRU file choice,
		// then the m_pKB will not be null and the following call won't be made. The 
		// SetupDirectories call has everything needed in order to silently change the project to
		// that of the document being opened
		// note; if gbAbortMRUOpen is TRUE, we can't open this document because it was saved
		// formerly in folder mode, and folder mode has become disabled (due to a bad file read of
		// books.xml or a failure to parse the XML document therein correctly) The gbAbortMRUOpen
		// flag is only set true by a test within SetupDirectories() - and we are interested only in this
		// after a click of an MRU item on the File menu.
		pApp->SetupDirectories();
		if (gbViaMostRecentFileList)
		{
			// test for the ability to get the needed information from the document - we can't get
			// the BookNamePair info (needed for setting up the correct project path) if the
			// books.xml parse failed, the latter failure sets m_bDisableBookMode to true.
			if (gbAbortMRUOpen)
			{
				pApp->GetView()->ClobberDocument();
				gbAbortMRUOpen = FALSE; // restore default value
				// IDS_NO_MRU_NOW
				wxMessageBox(_(
"Sorry, while book folder mode is disabled, using the Most Recently Used menu to click a document saved earlier in book folder mode will not open that file."),
				_T(""), wxICON_EXCLAMATION);
				return FALSE;
			}
			bool bSaveFlag = gpApp->m_bBookMode;
			int nSaveIndex = gpApp->m_nBookIndex;
            // the next call may clobber user's possible earlier choice of mode and index,
            // so restore these after the call (project config file is not updated until
            // project exitted, and so the user could have changed the mode or the book
            // folder from what is in the config file)
			pApp->GetProjectConfiguration(pApp->m_curProjectPath); // ensure gbSfmOnlyAfterNewlines
															   // is set to what it should be,
															   // and same for gSFescapechar
			gpApp->m_bBookMode = bSaveFlag;
			gpApp->m_nBookIndex = nSaveIndex;
		}
		gbAbortMRUOpen = FALSE; // make sure the flag has its default setting again
		gbViaMostRecentFileList = FALSE; // clear it to default setting
	}

	gbDoingInitialSetup = FALSE; // turn it back off, the pApp->m_targetBox now exists, etc

    // place the phrase box, but inhibit placement on first pile if doing a consistency
    // check, because otherwise whatever adaptation is in the KB for the first word/phrase
    // gets removed unconditionally from the KB when that is NOT what we want to occur!
	if (!gbConsistencyCheckCurrent)
	{
		// ensure its not a retranslation - if it is, move the active location to first 
		// non-retranslation pile
		if (pApp->m_pActivePile->GetSrcPhrase()->m_bRetranslation)
		{
			// it is a retranslation, so move active location
			CPile* pNewPile;
			CPile* pOldPile = pApp->m_pActivePile;
			do {
				pNewPile = pView->GetNextPile(pOldPile);
				wxASSERT(pNewPile);
				pOldPile = pNewPile;
			} while (pNewPile->GetSrcPhrase()->m_bRetranslation);
			pApp->m_pActivePile = pNewPile;
			pApp->m_nActiveSequNum = pNewPile->GetSrcPhrase()->m_nSequNumber;
		}

		// BEW added 10Jun09, support phrase box matching of the text colour chosen
		if (gbIsGlossing && gbGlossingUsesNavFont)
		{
			pApp->m_pTargetBox->SetOwnForegroundColour(pLayout->GetNavTextColor());
		}
		else
		{
			pApp->m_pTargetBox->SetOwnForegroundColour(pLayout->GetTgtColor());
		}


		pView->PlacePhraseBox(pApp->m_pActivePile->GetCell(1),2); // selector = 2, because we
			// were not at any previous location, so inhibit the initial StoreText call,
			// but enable the removal from KB storage of the adaptation text (see comments under
			// the PlacePhraseBox function header, for an explanation of selector values)

		// save old sequ number in case required for toolbar's Back button - no earlier one yet,
		// so just use the value -1
		gnOldSequNum = -1;
	}
	
	// update status bar with project name
	gpApp->RefreshStatusBarInfo();

	// determine m_curOutputPath, so it can be saved to config files as m_lastDocPath
	if (gpApp->m_bBookMode && !gpApp->m_bDisableBookMode)
	{
		pApp->m_curOutputPath = pApp->m_bibleBooksFolderPath + pApp->PathSeparator 
								+ pApp->m_curOutputFilename;
	}
	else
	{
		pApp->m_curOutputPath = pApp->m_curAdaptionsPath + pApp->PathSeparator 
								+ pApp->m_curOutputFilename;
	}

    // BEW added 01Oct06: to get an up-to-date project config file saved (in case user
    // turned on or off the book mode in the wizard) so that if the app subsequently
    // crashes, at least the next launch will be in the expected mode (see near top of
    // CAdapt_It.cpp for an explanation of the gbPassedAppInitialization global flag)
	// BEW added 12Nov09, m_bAutoExport test to suppress writing the project config file
	// when export is done from the command line export command
	if (gbPassedAppInitialization && !pApp->m_curProjectPath.IsEmpty() && !pApp->m_bAutoExport)
	{
        // BEW on 4Jan07 added change to WriteConfiguration to save the external current
        // work directory and reestablish it at the end of the WriteConfiguration call,
        // because the latter function resets the current directory to the project folder
        // before saving the project config file - and this clobbered the restoration of a
        // KB from the 2nd doc file accessed
		bool bOK;
		if (pApp->m_bUseCustomWorkFolderPath && !pApp->m_customWorkFolderPath.IsEmpty())
		{
			// whm 10Mar10, must save using what paths are current, but when the custom
			// location has been locked in, the filename lacks "Admin" in it, so that it
			// becomes a "normal" project configuration file in m_curProjectPath at the 
			// custom location.
			if (pApp->m_bLockedCustomWorkFolderPath)
				bOK = pApp->WriteConfigurationFile(szProjectConfiguration, pApp->m_curProjectPath,projectConfigFile);
			else
				bOK = pApp->WriteConfigurationFile(szAdminProjectConfiguration, pApp->m_curProjectPath,projectConfigFile);
		}
		else
		{
			bOK = pApp->WriteConfigurationFile(szProjectConfiguration, pApp->m_curProjectPath,projectConfigFile);
		}
		// original code below
		//bOK = pApp->WriteConfigurationFile(szProjectConfiguration,pApp->m_curProjectPath,projectConfigFile);
	}

	// wx version addition:
	// Add the file to the file history MRU
	// BEW added 12Nov09, m_bAutoExport test to suppress history update when export is
	// done from the command line export command
	if (!pApp->m_curOutputPath.IsEmpty() && !pApp->m_bAutoExport)
	{
		wxFileHistory* fileHistory = pApp->m_pDocManager->GetFileHistory();
		fileHistory->AddFileToHistory(pApp->m_curOutputPath);
        // The next two lines are a trick to get past AddFileToHistory()'s behavior of
        // extracting the directory of the file you supply and stripping the path of all
        // files in history that are in this directoy. RemoveFileFromHistory() doesn't do
        // any tricks with the path, so the following is a dirty fix to keep the full
        // paths.
		fileHistory->AddFileToHistory(wxT("[tempDummyEntry]"));
		fileHistory->RemoveFileFromHistory(0); // 
	}
	// Note: Tests of the MFC version show that OnInitialUpdate() should not be
	// called at all from OnOpenDocument()

	// BEW added 12Nov09, do the auto-export here, if asked for, and shut
	// down the app before returning; otherwise, continue for normal user
	// GUI interaction
	if (pApp->m_bAutoExport)
	{
		wxLogNull logNo; // avoid spurious messages from the system

		// set up output path using m_autoexport_outputpath
		wxString docName = pApp->m_autoexport_docname;
		docName = MakeReverse(docName);
		docName = docName.Mid(4); // remove reversed ".xml"
		docName = MakeReverse(docName); // back to normal order without extension
		docName = docName + _T(".txt"); // make a plain text file
		pApp->m_curOutputFilename = docName;
		pApp->m_curOutputPath = pApp->m_autoexport_outputpath + pApp->PathSeparator + docName;

		// pinch what I need from ExportFunctions.cpp
		wxString target;	// a export data's buffer
		target.Empty();
		int nTextLength;
		nTextLength = RebuildTargetText(target);
		FormatMarkerBufferForOutput(target);
		target = RemoveMultipleSpaces(target);

		// now write out the exported data string
		wxFile f;
		if( !f.Open(pApp->m_curOutputPath, wxFile::write))
		{
			wxString msg;
			msg = msg.Format(_("Unable to open the file for exporting the target text with path:\n%s"),pApp->m_curOutputPath.c_str());
			wxMessageBox(msg,_T(""),wxICON_EXCLAMATION);
			pApp->OnExit();
			return FALSE;
		}
		#ifndef _UNICODE // ANSI
		f.Write(target);
		#else // _UNICODE
		wxFontEncoding enc = wxFONTENCODING_UTF8; 
		pApp->ConvertAndWrite(enc,&f,target);
		#endif // for _UNICODE
		f.Close();
		// shut down forcefully
		//pApp->OnExit();
		unsigned long pid = ::wxGetProcessId();
		enum wxKillError killErr;
		int rv = ::wxKill(pid,wxSIGTERM,&killErr); // makes OnExit() be called
		rv  = rv; // prevent compiler warning

		return FALSE;
	}
	else
	{
		// BEW added 13Nov09, for setting or denying ownership for writing permission.
		// This is something we want to do each time a doc is opened - if the local user
		// already has ownership for writing, no change is done and he retains it; but
		// if he had read only access, and the other person has relinquished the project,
		// then the local user will now get ownership. We do this here in the else block
		// because we don't want to support this functionality for automated adaptation
		// exports from the command line because those have the app open only for a few 
		// seconds at most, and when they happen they change nothing so can be done
		// safely no matter who currently has ownership for writing.
		pApp->m_bReadOnlyAccess = pApp->m_pROP->SetReadOnlyProtection(pApp->m_curProjectPath);

		if (pApp->m_bReadOnlyAccess)
		{
			// if read only access is turned on, force the background colour change to show
			// now, instead of waiting for a user action requiring a canvas redraw
			pApp->GetView()->canvas->Refresh(); // needed? the call in OnIdle() is more effective
		}
	}

	return TRUE;
}

CLayout* CAdapt_ItDoc::GetLayout()
{
	CAdapt_ItApp* pApp = &wxGetApp();
	return pApp->m_pLayout;
}

// return the CPile* at the passed in index, or NULL if the index is out of bounds;
// the pile list is at CLayout::m_pileList.
// CAdapt_ItView also has a member function of the same name
CPile* CAdapt_ItDoc::GetPile(const int nSequNum)
{
	// refactored 10Mar09, for new view layout design (no bundles)
	CLayout* pLayout = GetLayout();
	wxASSERT(pLayout != NULL);
	PileList* pPiles = pLayout->GetPileList();
	int nCount = pPiles->GetCount();
	if (nSequNum < 0 || nSequNum >= nCount)
	{
		// bounds error, so return NULL
		return (CPile*)NULL;
	}
	PileList::Node* pos = pPiles->Item(nSequNum); // relies on parallelism of 
								// the m_pSourcePhrases and m_pileList lists
	wxASSERT(pos != NULL);
	return pos->GetData();
}

///////////////////////////////////////////////////////////////////////////////
/// \return TRUE if the current document has been modified; FALSE otherwise
/// \remarks
/// Called from: the App's GetDocHasUnsavedChanges(), OnUpdateFileSave(), OnSaveModified(),
/// CMainFrame's SyncScrollReceive() and OnIdle().
/// Internally calls the wxDocument::IsModified() method and the canvas->IsModified() method.
///////////////////////////////////////////////////////////////////////////////
bool CAdapt_ItDoc::IsModified() const // from wxWidgets mdi sample
{
  CAdapt_ItView* view = (CAdapt_ItView*) GetFirstView();

  if (view)
  {
      return (wxDocument::IsModified() || wxGetApp().GetMainFrame()->canvas->IsModified());
  }
  else
      return wxDocument::IsModified();
}

///////////////////////////////////////////////////////////////////////////////
/// \return nothing
/// \param mod		-> if FALSE, discards any edits
/// \remarks
/// Called from: all places that need to set the document as either dirty or clean including:
/// the App's DoUsfmFilterChanges() and DoUsfmSetChanges(), the Doc's OnNewDocument(), 
/// OnFileSave(), OnCloseDocument(), the View's PlacePhraseBox(), StoreText(), StoreTextGoingBack(),
/// ClobberDocument(), OnAdvancedRemoveFilteredFreeTranslations(), OnButtonDeleteAllNotes(),
/// OnAdvancedRemoveFilteredBacktranslations(), the DocPage's OnWizardFinish(), the CKBEditor's
/// OnButtonUpdate(), OnButtonAdd(), OnButtonRemove(), OnButtonMoveUp(), OnButtonMoveDown(),
/// the CMainFrame's OnMRUFile(), the CNoteDlg's OnBnClickedNextBtn(), OnBnClickedPrevBtn(),
/// OnBnClickedFirstBtn(), OnBnClickedLastBtn(), OnBnClickedFindNextBtn(), the CPhraseBox's
/// OnPhraseBoxChanged(), CViewFilteredMaterialDlg's UpdateContentOnRemove(), OnOK(), 
/// OnBnClickedRemoveBtn().
/// Sets the Doc's dirty flag according to the value of mod by calling wxDocument::Modify(mod).
///////////////////////////////////////////////////////////////////////////////
void CAdapt_ItDoc::Modify(bool mod) // from wxWidgets mdi sample
{
  CAdapt_ItView* view = (CAdapt_ItView*) GetFirstView();
  CAdapt_ItApp* pApp = &wxGetApp();
  wxASSERT(pApp != NULL);
  wxDocument::Modify(mod);

  if (!mod && view && pApp->GetMainFrame()->canvas)
    pApp->GetMainFrame()->canvas->DiscardEdits();
}


///////////////////////////////////////////////////////////////////////////////
/// \return nothing
/// \param      pList -> pointer to a SPList of source phrases
/// \remarks
/// Called from: the View's InitializeEditRecord(), OnEditSourceText(), 
/// OnCustomEventAdaptationsEdit(), and OnCustomEventGlossesEdit().
/// If pList has any items this function calls DeleteSingleSrcPhrase() for each item in the 
/// list.
///////////////////////////////////////////////////////////////////////////////
void CAdapt_ItDoc::DeleteSourcePhrases(SPList* pList, bool bDoPartnerPileDeletionAlso)
{
    // BEW added 21Apr08 to pass in a pointer to the list which is to be deleted (overload
    // of the version which has no input parameters and internally assumes the list is
    // m_pSourcePhrases) This new version is required so that in the refactored Edit Source
    // Text functionality we can delete the deep-copied sublists using this function.
	// BEW modified 27May09, to take the bool bDoPartnerPileDeletionAlso parameter,
	// defaulting to FALSE because the deep copied sublists never have partner piles
	//CAdapt_ItApp* pApp = &wxGetApp();
	//wxASSERT(pApp != NULL);
	if (pList != NULL)
	{
		if (!pList->IsEmpty())
		{
			// delete all the tokenizations of the source text
			SPList::Node *node = pList->GetFirst();
			while (node)
			{
				CSourcePhrase* pSrcPhrase = (CSourcePhrase*)node->GetData();
				node = node->GetNext();
#ifdef __WXDEBUG__
				//wxLogDebug(_T("   DeleteSourcePhrases pSrcPhrase at %x = %s"),
				//pSrcPhrase->m_srcPhrase, pSrcPhrase->m_srcPhrase.c_str());
#endif
				DeleteSingleSrcPhrase(pSrcPhrase, bDoPartnerPileDeletionAlso); // default
					// for the boolean passed in to DeleteSourcePhrases() is FALSE
			}
			pList->Clear(); 
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
/// \return nothing
/// \remarks
/// Called from: the Doc's DeleteContents(), the View's ClobberDocument().
/// If the App's m_pSourcePhrases SPList has any items this function calls 
/// DeleteSingleSrcPhrase() for each item in the list.
///////////////////////////////////////////////////////////////////////////////
void CAdapt_ItDoc::DeleteSourcePhrases()
{
	CAdapt_ItApp* pApp = &wxGetApp();
	wxASSERT(pApp != NULL);
	if (pApp->m_pSourcePhrases != NULL)
	{
		if (!pApp->m_pSourcePhrases->IsEmpty())
		{
			// delete all the tokenizations of the source text
			SPList::Node *node = pApp->m_pSourcePhrases->GetFirst();
			while (node)
			{
				CSourcePhrase* pSrcPhrase = (CSourcePhrase*)node->GetData();
				node = node->GetNext();
				DeleteSingleSrcPhrase(pSrcPhrase,FALSE); // FALSE is the
					// value for bDoPartnerPileDeletionAlso, because it is
					// more efficient to delete them later en masse with a call
					// to CLayout::DestroyPiles(), rather than one by one, as
					// the  one by one way involves a search to find the
					// partner, so it is slower
			}
			pApp->m_pSourcePhrases->Clear(); 
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
/// \return nothing
/// \param		pSrcPhrase -> the source phrase to be deleted
/// \remarks
/// Deletes the passed in CSourcePhrase instance, and if the bDoPartnerPileDeletionAlso bool value
/// is TRUE (its default value), then the partner pile in the CLayout::m_pileList list which
/// points at it is also deleted. Pass FALSE for this boolean if the CSourcePhrase being destroyed
/// is a temporary one in a list other than m_pSourcePhrases.
/// 
/// Called from: the App's DoTransformationsToGlosses(), DeleteSourcePhraseListContents(),
/// the Doc's DeleteSourcePhrases(), ConditionallyDeleteSrcPhrase(), 
/// ReconstituteOneAfterPunctuationChange(), ReconstituteOneAfterFilteringChange(),
/// DeleteListContentsOnly(), ReconstituteAfterPunctuationChange(), the View's 
/// ReplaceCSourcePHrasesInSpan(), TransportWidowedEndmarkersToFollowingContext(), 
/// TransferCOmpletedSrcPhrases(), and CMainFrame's DeleteSourcePhrases_ForSyncScrollReceive().
/// 
/// Clears and deletes any m_pMedialMarkers, m_pMedialPuncts and m_pSavedWords before deleting
/// pSrcPhrase itself.
///////////////////////////////////////////////////////////////////////////////
void CAdapt_ItDoc::DeleteSingleSrcPhrase(CSourcePhrase* pSrcPhrase, bool bDoPartnerPileDeletionAlso)
{
	// refactored 12Mar09
	if (pSrcPhrase == NULL)
		return;

	// if requested delete the CPile instance in CLayout::m_pileList which 
	// points to this pSrcPhrase
	if (bDoPartnerPileDeletionAlso)
	{
        // this call is safe to make even if there is no partner pile, or if a matching
        // pointer is not in the list
		DeletePartnerPile(pSrcPhrase); // marks its strip as invalid as well
	}

	if (pSrcPhrase->m_pMedialMarkers != NULL)
	{
		if (pSrcPhrase->m_pMedialMarkers->GetCount() > 0)
		{
			pSrcPhrase->m_pMedialMarkers->Clear();
		}
		delete pSrcPhrase->m_pMedialMarkers;
		pSrcPhrase->m_pMedialMarkers = (wxArrayString*)NULL;
	}

	if (pSrcPhrase->m_pMedialPuncts != NULL)
	{
		if (pSrcPhrase->m_pMedialPuncts->GetCount() > 0)
		{
			pSrcPhrase->m_pMedialPuncts->Clear();
		}
		delete pSrcPhrase->m_pMedialPuncts;
		pSrcPhrase->m_pMedialPuncts = (wxArrayString*)NULL;
	}

	// also delete any saved CSourcePhrase instances forming a phrase (and these
	// will never have medial puctuation nor medial markers nor will they store
	// any saved minimal phrases since they are CSourcePhrase instances for single
	// words only (nor will it point to any CRefString instances) (but these will
	// have SPList instances on heap, so must delete those)
	if (pSrcPhrase->m_pSavedWords != NULL)
	{
		if (pSrcPhrase->m_pSavedWords->GetCount() > 0)
		{
			SPList::Node *node = pSrcPhrase->m_pSavedWords->GetFirst();
			while (node)
			{
				CSourcePhrase* pSP = (CSourcePhrase*)node->GetData();
				node = node->GetNext(); // need this for wxList
				delete pSP->m_pSavedWords;
				pSP->m_pSavedWords = (SPList*)NULL;
				delete pSP->m_pMedialMarkers;
				pSP->m_pMedialMarkers = (wxArrayString*)NULL;
				delete pSP->m_pMedialPuncts;
				pSP->m_pMedialPuncts = (wxArrayString*)NULL;
				delete pSP;
				pSP = (CSourcePhrase*)NULL;
			}
		}
		delete pSrcPhrase->m_pSavedWords; // delete the SPList* too
		pSrcPhrase->m_pSavedWords = (SPList*)NULL;
	}
	delete pSrcPhrase;
	pSrcPhrase = (CSourcePhrase*)NULL;
}

///////////////////////////////////////////////////////////////////////////////
/// \return     nothing
/// \param		pSrcPhrase -> the source phrase that was deleted
/// \remarks
/// Created 12Mar09 for layout refactoring. The m_pileList's CPile instances point to
/// CSourcePhrase instances and deleting a CSourcePhrase from the doc's m_pSourcePhrases
/// list usually needs to also have the CPile instance which points to it also deleted from
/// the corresponding place in the m_pileList. That task is done here. Called from
/// DeleteSingleSrcPhrase(), the latter having a bool parameter,
/// bDoPartnerPileDeletionAlso, which defaults to TRUE, and when FALSE is passed in this
/// function will not be called. (E.g. when the source phrase belongs to a temporary list
/// and so has no partner pile)
/// Note: the deletion will be done for some particular pSrcPhrase in the app's
/// m_pSourcePhrases list, and we want to have at least an approximate idea of the index
/// of which strip the copy of the pile pointer was in, because when we tweak the layout
/// we will want to know which strips to concentrate our efforts on. Therefore before we
/// do the deletion, we work out which strip the pile belongs to, mark it as invalid, and
/// store its index in CLayout::m_invalidStripArry. Later, our strip tweaking code will
/// use this information to make a speedy tweak of the layout before drawing is done (but
/// the information is not used whenever RecalcLayout() does a full rebuild of the
/// document's strips)
///////////////////////////////////////////////////////////////////////////////
void CAdapt_ItDoc::DeletePartnerPile(CSourcePhrase* pSrcPhrase)
{
	// refactored 4May09
	CLayout* pLayout = GetLayout();
	PileList* pPiles = pLayout->GetPileList();
	if (pPiles->IsEmpty())
		return;
	PileList::Node* posPile = pPiles->GetFirst();
	wxASSERT(posPile!= NULL);
	CPile* pPile = NULL;
	while (posPile != NULL)
	{
		pPile = posPile->GetData();
		if (pSrcPhrase == pPile->GetSrcPhrase())
		{
			// we have found the partner pile, so break out
			break;
		}
		posPile = posPile->GetNext(); // go to next Node*
	} // end of while loop with test: posPile != NULL
	if (posPile == NULL)
	{
		return; // we didn't find a partner pile, so just return;
	}
	else
	{
		// found the partner pile in CLayout::m_pileList, so delete it...
 		
        // get the CPile* instance currently at index, from it we can determine which strip
        // the deletion will take place from (even if we get this a bit wrong, it won't
        // matter)
		pPile = posPile->GetData();
		wxASSERT(pPile != NULL);
		MarkStripInvalid(pPile); // sets CStrip::m_bValid to FALSE, and adds the 
								 // strip index to CLayout::m_invalidStripArray

		// now go ahead and get rid ot the partner pile for the passed in pSrcPhrase
		pPile->SetStrip(NULL);

		// if destroying the CPile instance pointed at by app's m_pActivePile, set the
		// latter to NULL as well (otherwise, in the debugger it would have the value
		// 0xfeeefeee which is useless for pile ptr != NULL tests as it gives a spurious
		// positive result
		if (pLayout->m_pApp->m_nActiveSequNum != -1 && pLayout->m_pApp->m_pActivePile == pPile)
		{
			pLayout->m_pApp->m_pActivePile = NULL;
		}
		pLayout->DestroyPile(pPile,pLayout->GetPileList()); // destroy the pile, & remove 
					// its node from m_pileList, because bool param, bRemoveFromListToo, is
					// default TRUE
		pPile = NULL;
	}
}

///////////////////////////////////////////////////////////////////////////////
/// \return     nothing
/// \param		pSrcPhrase -> the source phrase that was created and inserted in the 
///                           application's m_pSourcePhrases list
/// \remarks
/// Created 13Mar09 for layout refactoring. The m_pileList's CPile instances point to
/// CSourcePhrase instances and creating a new CSourcePhrase for the doc's m_pSourcePhrases
/// list always needs to have the CPile instance which points to it also created,
/// initialized and inserted into the corresponding place in the m_pileList. That task is
/// done here.
/// 
/// Called from various places. It is not made a part of the CSourcePhrase creation process
/// for a good reason. Quite often CSourcePhrase instances are created and are only
/// temporary - such as those deep copied to be saved in various local lists during the
/// vertical edit process, and in quite a few other contexts as well. Also, when documents
/// are loaded from disk and very many CSourcePhrase instances are created in that process,
/// it is more efficient to not create CPile instances as part of that process, but rather
/// to create them all in a loop after the document has been loaded. For example, the
/// current code creates new CSourcePhrases on the heap in about 30 places in the app's
/// code, but only about 25% of those instances require a CPile partner created for the
/// CLayout::m_pileList; therefore we call CreatePartnerPile() only when needed, and it
/// should be called immediately after a newly created CSourcePhrase has just been inserted
/// into the app's m_pSourcePhrases list - so that the so that the strip where the changes
/// happened can be marked as "invalid".
/// Note: take care when CSourcePhrase(s) are appended to the end of the m_pSourcePhrases
/// list, because Creating the partner piles cannot handle discontinuities in the sequence
/// of piles in PileList. So, iterate from left to right over the new pSrcPhrase at the
/// list end, so that each CreatePartnerPile call is creating the CPile instance which is
/// next to be appended to PileList. We test for non-compliance with this rule and abort
/// the application if it happens, because to continue would inevitably lead to an app crash.
///////////////////////////////////////////////////////////////////////////////
void CAdapt_ItDoc::CreatePartnerPile(CSourcePhrase* pSrcPhrase)
{
	// refactor 13Mar09
	CLayout* pLayout = GetLayout();
	int index = IndexOf(pSrcPhrase); // the index in m_pSourcePhrases for the passed
									 // in pSrcPhrase
	PileList::Node* aPosition = NULL;
	CPile* aPilePtr = NULL;
	wxASSERT(index != wxNOT_FOUND); // it must return a valid index!
	PileList* pPiles = pLayout->GetPileList();

	// if the pSrcPhrase is one added to the end of the document, there won't be any
	// existing pile pointers in the PileList with indices that large, so check for this
	// because an Item(index) call with an index out of range will crash the app, it
	// doesn't return a NULL which is what I erroneously though would happen
	PileList::Node* posPile = NULL;
	int lastPileIndex = pLayout->GetPileList()->GetCount() - 1;
	bool bAppending = FALSE;
	if (index > lastPileIndex + 1)
	{
		// we've skipped a pile somehow, this is a fatal error, tell developer and abort
		wxMessageBox(_T(
"Ouch! CreatePartnerPile() has skipped a pSrcPhrase added to doc end, or they creations are not being done in left to right sequence. Must abort now."),
		_T(""), wxICON_ERROR);
		wxASSERT(FALSE);
		wxExit();
	}
	else if (index == lastPileIndex + 1)
	{
		// we are creating the CPile instance which is due to be appended next tp PileList
		bAppending = TRUE;
	}
	else
	{
		// if control gets here with bAppending still FALSE, then an insertion is required
		posPile = pPiles->Item(index);
	}
	CPile* pNewPile = pLayout->CreatePile(pSrcPhrase); // creates a detached CPile
					// instance, initializes it, sets m_nWidth and m_nMinWidth etc
	if (!bAppending)
	{
        // we are inserting a new partner pile's pointer in the CLayout::m_pileList does
        // not get it also inserted in the CLayout::m_stripArray, and so the laid out
        // strips don't know of it. However, we can work out which strip it would be
        // inserted within, or thereabouts, and mark that strip as invalid and put its
        // index into CLayout::m_invalidStripArray. The inventory of invalid strips does
        // not have to be 100% reliable - they are approximate indicators where the layout
        // needs to be tweaked, which is all we need for the RecalcLayout() call later on.
        // Therefore, use the current pPile pointer in the m_pileList at index, and find
        // which strip that one is in, even though it is not the newly created CPile
		aPosition = pPiles->Item(index);
		aPilePtr = aPosition->GetData();
		if (aPilePtr != NULL)
			MarkStripInvalid(aPilePtr); // use aPilePtr to have a good shot at which strip
                // will receive the newly created pile, and mark it as invalid, and save
                // its index in CLayout::m_invalidStripArray; nothing is done if
                // aPilePtr->m_pOwningStrip is NULL

        // the indexed location is within the unaugmented CLayout::m_pileList; therefore an
        // insert operation is required; the index posPile value determined by index is the
        // place where the insertion must be done
		posPile = pPiles->Insert(posPile, pNewPile);
	}
	else
	{
		// appending, (see comment in block above for more details), so just mark the last
		// strip in m_stripArray as invalid, don't call MarkStripInvalid()
		aPosition = pPiles->GetLast(); // the one after which we will append the new one
		aPilePtr = aPosition->GetData();
		if (aPilePtr != NULL)
		{
			// do this manually... just mark the last strip as the invalid one, etc
			CStrip* pLastStrip = (CStrip*)pLayout->GetStripArray()->Last();
			pLastStrip->SetValidityFlag(FALSE); // makes m_bValid be FALSE
			int nStripIndex = pLastStrip->GetStripIndex();
			pLayout->GetInvalidStripArray()->Add(nStripIndex); 
		}
		// now do the append
		posPile = pPiles->Append(pNewPile); // do this only after aPilePtr is calculated
	}
}

// return the index in m_pSourcePhrases for the passed in pSrcPhrase
int CAdapt_ItDoc::IndexOf(CSourcePhrase* pSrcPhrase)
{
	wxASSERT(pSrcPhrase != NULL);
	SPList* pList = GetApp()->m_pSourcePhrases;
	int nIndex = pList->IndexOf(pSrcPhrase);
	return nIndex;
}

void CAdapt_ItDoc::ResetPartnerPileWidth(CSourcePhrase* pSrcPhrase,
										   bool bNoActiveLocationCalculation)
{
	// refactored 13Mar09 & some more on 27Apr09
	int index = IndexOf(pSrcPhrase); // the index in m_pSourcePhrases for the passed in 
									 // pSrcPhrase, in the app's m_pSourcePhrases list
	wxASSERT(index != wxNOT_FOUND); // it must return a valid index!
	PileList* pPiles = GetLayout()->GetPileList();
	PileList::Node* posPile = pPiles->Item(index); // returns NULL if index lies beyond 
												   // the end of m_pileList
	if (posPile != NULL)
	{
		CPile* pPile = posPile->GetData();
		wxASSERT(pPile != NULL);
		pPile->SetMinWidth(); // set m_nMinWidth - it's the maximum extent of the src, 
							  // adapt or gloss text

        // if it is at the active location, then the width needs to be wider -
        // SetPhraseBoxGapWidth() in CPile does that, and sets m_nWidth in the partner pile
        // instance (but if not at the active location, the default value m_nMinWidth will
        // apply)
		if (!bNoActiveLocationCalculation)
		{
            // EXPLANATION FOR THE ABOVE TEST: we need the capacity to have the phrase box
            // be at the active location, but not have the gap width left in the layout be
            // wide enough to accomodate the box - this situation happens in
            // PlacePhraseBox() when we want a box width calculation for the pile which is
            // being left behind (and which at the point when the pile's width is being
            // calculated it is still the active pile, but later in the function when the
            // clicked pile is instituted as the new active location, the gap will have to
            // be based on what is at that new location, not the old one; hence setting the
            // bNoActiveLocationCalculation parameter TRUE just for such a situation
            // prevents the unwanted large gap calculation at the old location
			pPile->SetPhraseBoxGapWidth();
		}

		// mark the strip invalid and put the parent strip's index into 
		// CLayout::m_invalidStripArray
		MarkStripInvalid(pPile);
	}
	else
	{
		// if it is null, this is a catastrophic error, and we must terminate the application;
		// or in DEBUG mode, give the developer a chance to look at the call stack
		wxMessageBox(_T(
		"Ouch! ResetPartnerPileWidth() was unable to find the partner pile. Must abort now."),
		_T(""), wxICON_EXCLAMATION);
		wxASSERT(FALSE);
		wxExit();
	}
}

void CAdapt_ItDoc::MarkStripInvalid(CPile* pChangedPile)
{
	CLayout* pLayout = GetLayout();
	// we can mark a strip invalid only provided it exists; so if calling this in
	// RecalcLayout() after the strips were destroyed, and before they are rebuilt, we'd
	// be trying to access freed memory if we went ahead here without a test for strips
	// being in existence - so return if the m_stripArray has no contents currently
	if (pLayout->GetStripArray()->IsEmpty())
		return;
	// pChangedPile has to have a valid m_pOwningStrip (ie. a non-zero value), which may
	// not be the case for a set of newly created piles at the end of the document, so for
	// end-of-document scenarios we will code the caller to just assume the last of the
	// current strips and not call MarkStripInvalid() at all; but just in case one sneaks
	// through, test here and if it has a zero m_pOwningStrip value then exit without
	// doing anything (unfortunately we can't assume it will always be at the doc end)
	if (pChangedPile->GetStrip() == NULL)
	{
		// it's a newly created pile not yet within the current set of strips, so its
		// m_pOwningStrip member returned was NULL, we shouldn't have called this
		// function for this pChangedPile pointer, but since we did, we can only return
		// without doing anything
		return;
	}
    // if control gets to here, there are CStrip pointers stored in m_stripArray, and there
    // is an owning strip defined, so go ahead and mark the owning strip invalid
	wxArrayInt* pInvalidStripArray = pLayout->GetInvalidStripArray();
	CStrip* pStrip = pChangedPile->GetStrip();
	pStrip->SetValidityFlag(FALSE); // makes m_bValid be FALSE
	int nStripIndex = pStrip->GetStripIndex();
	pInvalidStripArray->Add(nStripIndex); // this array makes it easy to quickly compute 
										  // which strips are invalid
}

///////////////////////////////////////////////////////////////////////////////
/// \return nothing
/// \param		pSrcPhrase -> the source phrase to be deleted
/// \param		pOtherList -> another list of source phrases
/// \remarks
/// Called from : the Doc's ReconstituteAfterPunctuationChange().
/// This function is used in document reconstitution after a punctuation change forces a 
/// rebuild. 
/// Clears and deletes any m_pMedialMarkers, m_pMedialPuncts before deleting
/// pSrcPhrase itself.
/// SmartDeleteSingleSrcPhrase deletes only those pSrcPhrase instances in its m_pSavedWords
/// list which are not also in the pOtherList.
///////////////////////////////////////////////////////////////////////////////
void CAdapt_ItDoc::SmartDeleteSingleSrcPhrase(CSourcePhrase* pSrcPhrase, SPList* pOtherList)
{
	// refactored 12Mar09 - nothing was done here because it can be called only in
	// ReconstituteAdfterPunctuationChange() - and only then provided a merger could not be
	// reconstituted -- and so because all partner piles will need to be recreated anyway,
	//  just dealing with a few is pointless; code in that function will handle all instead
	if (pSrcPhrase == NULL)
		return;

	if (pSrcPhrase->m_pMedialMarkers != NULL)
	{
		if (pSrcPhrase->m_pMedialMarkers->GetCount() > 0)
		{
			pSrcPhrase->m_pMedialMarkers->Clear();
		}
		delete pSrcPhrase->m_pMedialMarkers;
	}

	if (pSrcPhrase->m_pMedialPuncts != NULL)
	{
		if (pSrcPhrase->m_pMedialPuncts->GetCount() > 0)
		{
			pSrcPhrase->m_pMedialPuncts->Clear();
		}
		delete pSrcPhrase->m_pMedialPuncts;
	}

	// also delete any saved CSourcePhrase instances forming a phrase (and these
	// will never have medial puctuation nor medial markers nor will they store
	// any saved minimal phrases since they are CSourcePhrase instances for single
	// words only (nor will it point to any CRefString instances) (but these will
	// have SPList instances on heap, so must delete those)
	if (pSrcPhrase->m_pSavedWords != NULL)
	{
		if (pSrcPhrase->m_pSavedWords->GetCount() > 0)
		{
			SPList::Node* pos = pSrcPhrase->m_pSavedWords->GetFirst();
			while (pos != NULL)
			{
				CSourcePhrase* pSP = (CSourcePhrase*)pos->GetData();
				pos = pos->GetNext();
				SPList::Node* pos1 = pOtherList->Find(pSP);
				if (pos1 != NULL)
					continue; // it's in the other list, so don't delete it
				delete pSP->m_pSavedWords;
				delete pSP->m_pMedialMarkers;
				delete pSP->m_pMedialPuncts;
				delete pSP;
			}
		}
		pSrcPhrase->m_pSavedWords->Clear();
		delete pSrcPhrase->m_pSavedWords; // delete the SPList* too
	}
	delete pSrcPhrase;
}

///////////////////////////////////////////////////////////////////////////////
/// \return nothing
/// \param		pSrcPhrase -> the source phrase to be deleted
/// \param		pOtherList -> another list of source phrases
/// \remarks
/// Called from: the Doc's ReconstituteAfterPunctuationChange().
/// This function calls DeleteSingleSrcPhrase() but only if pSrcPhrase is not found in
/// pOtherList. ConditionallyDeleteSrcPhrase() deletes pSrcPhrase conditionally - the
/// condition is whether or not its pointer is present in the pOtherList. If it occurs in
/// that list, it is not deleted (because the other list has to continue to manage it), but
/// if not in that list, it gets deleted. (Don't confuse this function with
/// SmartDeleteSingleSrcPhrase() which also similarly deletes dependent on not being
/// present in pOtherList - in that function, it is not the passed in sourcephrase which is
/// being considered, but only each of the sourcephrase instances in its m_pSavedWords
/// member's sublist).
///////////////////////////////////////////////////////////////////////////////
void CAdapt_ItDoc::ConditionallyDeleteSrcPhrase(CSourcePhrase* pSrcPhrase, SPList* pOtherList)
{
	SPList::Node* pos = pOtherList->Find(pSrcPhrase);
	if (pos != NULL)
	{
		// pSrcPhrase exists in the list pOtherList, so don't delete it
		return;
	}
	else
	{
		// pSrcPhrase is absent from pOtherList, so it can safely be deleted
		DeleteSingleSrcPhrase(pSrcPhrase);
	}
}

///////////////////////////////////////////////////////////////////////////////
/// \return TRUE to indicate to the caller all is OK. Except for a retranslation, return FALSE to 
///			indicate to the caller that fixesStr must have a reference added, and the new 
///			CSourcePhrase instances must, in the m_pSourcePhrases list, replace the pSrcPhrase 
///			instance passed in.
/// \param		pView		-> pointer to the View
/// \param		pList		-> pointer to m_pSourcePhrases
/// \param		pos			-> the iterator position locating the passed in pSrcPhrase pointer
/// \param		pSrcPhrase	<- pointer of the source phrase
/// \param		fixesStr	-> currently unused
/// \param		pNewList	<- the parsed new source phrase instances
/// \param		bIsOwned	-> specifies whether or not the pSrcPhrase passed in is one which is 
///								owned by another sourcephrase instance or not (ie. TRUE means that 
///								it is one of the originals stored in an owning CSourcePhrase's 
///								m_pSavedWords list member, FALSE means it is not owned by another 
///								and so is a candidate for adaptation/glossing and entry of its
///								data in the KB; owned ones cannot be stored in the KB - at least 
///								not while they continue as owned ones)
/// \remarks
/// Called from: the Doc's ReconstituteAfterPunctuationChange()
/// Handles one pSrcPhrase and ignores the m_pSavedWords list, since that is handled within
/// the ReconstituteAfterPunctuationChange() function for the owning srcPhrase with m_nSrcWords > 1.
/// For the return value, see ReconstituteAfterPunctuationChange()'s return value - same deal applies here.
///////////////////////////////////////////////////////////////////////////////
bool CAdapt_ItDoc::ReconstituteOneAfterPunctuationChange(CAdapt_ItView* pView, 
						SPList*& pList, SPList::Node* pos, CSourcePhrase*& pSrcPhrase, 
						wxString& WXUNUSED(fixesStr), SPList*& pNewList, bool bIsOwned)
{
	// BEW added 5Apr05
	bool bHasTargetContent = TRUE; // assume it has an adaptation, clear below if not true
	bool bPlaceholder = FALSE; // default
	bool bNotInKB = FALSE; // default
	bool bRetranslation = FALSE; // default
	if (pSrcPhrase->m_bNullSourcePhrase) bPlaceholder = TRUE;
	if (pSrcPhrase->m_bRetranslation) bRetranslation = TRUE;
	if (pSrcPhrase->m_bNotInKB) bNotInKB = TRUE;

	wxString srcPhrase; // copy of m_srcPhrase member
	wxString targetStr; // copy of m_targetStr member
	wxString key; // copy of m_key member
	wxString adaption; // copy of m_adaption member
	wxString gloss; // copy of m_gloss member
	key.Empty(); adaption.Empty(); gloss.Empty();

	// setup the srcPhrase, targetStr and gloss strings - we must handle glosses too regardless of the current mode
	// (whether adapting or not)
	int numElements = 1; // default
	srcPhrase = pSrcPhrase->m_srcPhrase; // this member has all the source 
										 // punctuation, if any on this word or phrase
	gloss = pSrcPhrase->m_gloss; // we don't care if glosses have punctuation or not
	if (pSrcPhrase->m_adaption.IsEmpty())
		bHasTargetContent = FALSE;	// use to suppress copying of source punctuation 
									// to an adaptation not yet existing
	else
		targetStr = pSrcPhrase->m_targetStr; // likewise, has punctuation, if any

    // handle placeholders - these have elipsis ... as their m_key and m_srcPhrase
    // members, and so there is no possibility of punctuation changes having any effect
    // on these except possibly for the m_adaption member. Placeholders can occur
    // independently, or as part of a retranslation - the same treatment can be given
    // to instances occurring in either environment.
	SPList::Node* fpos;
	fpos = NULL;
	CSourcePhrase* pNewSrcPhrase;
	SPList::Node* newpos;
	if (bPlaceholder)
		goto b;

    // BEW 8Jul05: a pSrcPhrase with empty m_srcPhrase and empty m_key can't produce
    // anything when passed to TokenizeTextString, and so to prevent numElements being
    // set to zero we must here detect any such sourcephrases and just return TRUE -
    // for these punctuation changes can produce no effect
	if (pSrcPhrase->m_srcPhrase.IsEmpty() || pSrcPhrase->m_key.IsEmpty())
		return TRUE; // causes the caller to use pSrcPhase 'as is'

    // reparse the srcPhrase string - if we get a single CSourcePhrase as the result,
    // we have a simple job to complete the rebuild for this passed in pSrcPhrase; if
    // we get more than one, we'll have to abandon the adaptation or gloss and set up
    // the flags etc accordingly, and return the two or more new instances to the
    // caller - the caller will know what to do (eg. we might be processing an owned
    // original CSourcePhrase instance from a former merge, and if so we'd want to
    // collect all sibling new CSourcePhrase instances so the caller could abandon the
    // merge and insert the collected instances into m_pSourcePhrases; and that's a
    // different scenario from being at the top level already for a pSrcPhrase with
    // m_pSavedWords empty and which has returned more than one new instance from the
    // reparse - the caller will distinguish these and act accordingly)
	
	// important, ParseWord() doesn't work right if the first character is a space
	srcPhrase.Trim(TRUE); // trim right end
	srcPhrase.Trim(FALSE); // trim left end
	numElements = pView->TokenizeTextString(pNewList, srcPhrase,  pSrcPhrase->m_nSequNumber);
	wxASSERT(numElements >= 1);
	pNewSrcPhrase = NULL;
	newpos = NULL;
	if (numElements == 1)
	{
        // simple case - we can complete the rebuild in this block; note, the passed in
        // pSrcPhrase might be storing quite complex data - such as filtered material,
        // chapter & verse information and so forth, so we have to copy everything
        // across as well as update the source and target string members and
        // punctuation. The simplest direction for this copy is to copy from the parsed
        // new source phrase instance back to the original, since only m_key,
        // m_adaption, m_targetStr, precPunct and follPunct can possibly be different
        // in the new parse
		fpos = pNewList->GetFirst();
		pNewSrcPhrase = fpos->GetData();
		pSrcPhrase->m_key = pNewSrcPhrase->m_key;
		pSrcPhrase->m_precPunct = pNewSrcPhrase->m_precPunct;
		pSrcPhrase->m_follPunct = pNewSrcPhrase->m_follPunct;

		// the adaptation, and gloss if any, is already presumably what the user wants, 
		// so we'll just remove punctuation from the adaptation, and set the relevant members
		// (m_targetStr is already correct) - but only provided there is an existing adaptation
b:		pSrcPhrase->m_gloss = gloss;
		// remove any initial or final spaces before using it
		targetStr.Trim(TRUE); // trim right end
		targetStr.Trim(FALSE); // trim left end
		if (bHasTargetContent)
		{
			adaption = targetStr;
			pView->RemovePunctuation(this,&adaption,from_target_text); // 1 = from tgt
			pSrcPhrase->m_adaption = adaption;

			// update the KBs (both glossing and adapting KBs) provided it is 
			// appropriate to do so
			if (!bPlaceholder && !bRetranslation && !bNotInKB && !bIsOwned)
				pView->StoreKBEntryForRebuild(pSrcPhrase, adaption, gloss);
		}

		return TRUE;
	}
	else
	{
        // oops, somehow we got more than a single sourcephrase instance -- we'll have
        // to transfer the legacy information (such as filtered stuff, markers, etc) to
        // the first of the new instances here, and then return to the caller so it can
        // have those instances inserted into the document's list in place of the
        // original single instance. (User will need to manually edit at this location
        // later on.) All CSourcePhrase instances handled in this block have to be
        // marked as m_bHasKBEntry == FALSE, and m_bHasGlossingKBEntry == FALSE too,
        // since the user will have to readapt/regloss later for these ones.
		CSourcePhrase* pSPnew = NULL;
		SPList::Node* pos2 = pNewList->GetFirst();
		bool bIsFirst = TRUE;
		bool bIsLast = FALSE;
		while (pos2 != NULL)
		{
			pSPnew = (CSourcePhrase*)pos2->GetData(); // m_srcPhrase, m_key, 
                        // m_precPunct, m_follPunct, m_nSrcWords are set, but it has no
                        // idea what context it belongs to and so all other flags and
                        // strings have to be copied from the original pSrcPhrase
                        // passed in, but we copy only to the new first instance of
                        // pSPnew, and for subsequent ones just propagate what is
                        // necessary to those
			pos2 = pos2->GetNext();
			if (pos2 == NULL)
				bIsLast = TRUE;
			if (bIsFirst)
			{
                // copying everything to the first instance ensures that the source
                // text's original order of characters is not violated despite the
                // reparse generating unexpected additional sourcephrase instances; so
                // first copy all the original's flag values
				CopyFlags(pSPnew,pSrcPhrase);

                // copy the other stuff, except for the sublists (since m_pSavedWords,
                // m_pMedialPuncts, and m_pMedialMarkers we must leave for the caller
                // to handle, since these pertain only to a passed in pSrcPhrase which
                // is a merged one - so the caller will need extra apparatus to handle
                // such instances correctly; moreover, those three lists are always
                // empty for owned instances and for non-owned instances which are not
                // merged, so it is safe to ignore them here)
				pSPnew->m_curTextType = pSrcPhrase->m_curTextType;
				pSPnew->m_inform = pSrcPhrase->m_inform;
				pSPnew->m_chapterVerse = pSrcPhrase->m_chapterVerse;
				pSPnew->m_markers = pSrcPhrase->m_markers;
				pSPnew->m_nSequNumber= pSrcPhrase->m_nSequNumber;


				bIsFirst = FALSE;
			}
			else
			{
				// for non-first instances, copy the essential members which 
				// require propagation
				pSPnew->m_bRetranslation = pSrcPhrase->m_bRetranslation;
				pSPnew->m_bSpecialText = pSrcPhrase->m_bSpecialText;
				pSPnew->m_curTextType = pSrcPhrase->m_curTextType;

                // in the case of a <Not In KB> instance (whether merged or not) we
                // preserve that flag value and propagate it to each new instance, but
                // retranslations work differently - see below
				pSPnew->m_bNotInKB = pSrcPhrase->m_bNotInKB;

				// the m_bFirstOfType cannot possibly be set TRUE on any of the 
				// non-first new instances
				pSPnew->m_bFirstOfType = FALSE;

			}
            // set up booleans which record that the instance is the last of a
            // retranslation or footnote; and handle m_bBoundary likewise, since
            // boundaries are 'at the end' kind of things
			if (pSrcPhrase->m_bEndRetranslation && !bIsLast)
				pSPnew->m_bEndRetranslation = FALSE; // a TRUE value will move to the last in the series
			if (pSrcPhrase->m_bEndRetranslation && bIsLast)
				pSPnew->m_bEndRetranslation = TRUE;
			if (pSrcPhrase->m_bFootnoteEnd && !bIsLast)
				pSPnew->m_bFootnoteEnd = FALSE; // a TRUE value will move to the last in the series
			if (pSrcPhrase->m_bFootnoteEnd && bIsLast)
				pSPnew->m_bFootnoteEnd = TRUE;
			if (pSrcPhrase->m_bBoundary && !bIsLast)
				pSPnew->m_bBoundary = FALSE; // a TRUE value would move to the last in the series
			if (pSrcPhrase->m_bBoundary && bIsLast)
				pSPnew->m_bBoundary = TRUE;

            // ensure that the "Has..." boolean values are FALSE, since we have
            // abandoned the gloss and adaptation strings
			pSPnew->m_bHasKBEntry = pSrcPhrase->m_bHasKBEntry;
			pSPnew->m_bHasGlossingKBEntry = pSrcPhrase->m_bHasGlossingKBEntry;
		} // end of loop for iterating over the new CSourcePhrase instances 
		  // resulting from the reparse

        // Note: a changed number of CSourcePhrase instances is okay if the original is
        // from a retranslation - we then return TRUE. This is because retranslations
        // don't care how many CSourcePhrase instances comprise them, so adding an
        // extra one or more is immaterial - hence we can treat this situation as if it
        // was normal
		if (bRetranslation)
		{
            // since we will return TRUE from this block rather then FALSE, the caller
            // will not replace the original pSrcPhrase with the newly parsed ones - so
            // we must do that job here instead
			SPList::Node* pos3 = pNewList->GetFirst();
			while (pos3 != NULL)
			{
				CSourcePhrase* pSP = (CSourcePhrase*)pos3->GetData();
				pos3 = pos3->GetNext();
				pList->Insert(pos,pSP); // insert before the passed in 
										// POSITION in original list
			}
			DeleteSingleSrcPhrase(pSrcPhrase); // delete the old one
			pList->DeleteNode(pos); // remove its element from m_pSourcePhrases or 
									// whatever was the passed in list
			return TRUE; // tell caller all is okay
		}
		return FALSE;	// except for a retranslation, we always return FALSE from 
                        // this block so the caller will know that fixesStr must have a
                        // reference added, and the new CSourcePhrase instances must,
                        // in the m_pSourcePhrases list, replace the pSrcPhrase
                        // instance passed in
	}
}

///////////////////////////////////////////////////////////////////////////////
/// \return		FALSE if the rebuild fails internally at one or more locations so that the 
///				user must later inspect the doc visually and do something corrective at such 
///				locations; TRUE if the rebuild was successful everywhere.
/// \param		pView		-> pointer to the View
/// \param		pList		-> pointer to m_pSourcePhrases
/// \param		fixesStr	-> currently unused
/// \remarks
/// Called from: the Doc's RetokenizeText().
/// Rebuilds the document after a filtering change is indicated. The new document reflects
/// the new filtering changes.
///////////////////////////////////////////////////////////////////////////////
bool CAdapt_ItDoc::ReconstituteAfterFilteringChange(CAdapt_ItView* pView,
													SPList*& pList, wxString& fixesStr)
{
	// refactored 18Mar09
	// BEW added 18May05
	// Filtering has changed
	bool bSuccessful = TRUE;
	wxString endingMkrsStr; // BEW added 25May05 to handle endmarker sequences like \fq*\f*

	// Recalc the layout in case the view does some painting when the progress 
	// was removed earlier or when the bar is recreated below
	UpdateSequNumbers(0); // get the numbers updated, as a precaution
#ifdef _NEW_LAYOUT
	GetLayout()->RecalcLayout(gpApp->m_pSourcePhrases, keep_strips_keep_piles);
#else
	GetLayout()->RecalcLayout(gpApp->m_pSourcePhrases, create_strips_keep_piles);
#endif
	gpApp->m_pActivePile = pView->GetPile(gpApp->m_nActiveSequNum);
	GetLayout()->PlaceBox();

	// put up a progress indicator
	int nOldTotal = pList->GetCount();
	if (nOldTotal == 0)
	{
		return 0;
	}
	int nOldCount = 0;

#ifdef __WXMSW__
	wxString progMsg = _("Pass 1 - File: %s  - %d of %d Total words and phrases");
	wxString msgDisplayed = progMsg.Format(progMsg,gpApp->m_curOutputFilename.c_str(),1,nOldTotal);
    wxProgressDialog progDlg(_("Processing Filtering Change(s)"),
                            msgDisplayed,
                            nOldTotal,    // range
                            GetDocumentWindow(),   // parent
                            //wxPD_CAN_ABORT |
                            //wxPD_CAN_SKIP |
                            wxPD_APP_MODAL |
                            // wxPD_AUTO_HIDE | -- try this as well
                            wxPD_ELAPSED_TIME |
                            wxPD_ESTIMATED_TIME |
                            wxPD_REMAINING_TIME
                            | wxPD_SMOOTH // - makes indeterminate mode bar on WinXP very small
                            );
#else
	// wxProgressDialog tends to hang on wxGTK so I'll just use the simpler CWaitDlg
	// notification on wxGTK and wxMAC
	// put up a Wait dialog - otherwise nothing visible will happen until the operation is done
	CWaitDlg waitDlg(gpApp->GetMainFrame());
	// indicate we want the reading file wait message
	waitDlg.m_nWaitMsgNum = 5;	// 5 hides the static leaving only "Please wait..." in title bar
	waitDlg.Centre();
	waitDlg.Show(TRUE);
	waitDlg.Update();
	// the wait dialog is automatically destroyed when it goes out of scope below.
#endif

	// BEW added 29Jul09, turn off CLayout Draw() while strips and piles could get
	// inconsistent with each other
	GetLayout()->m_bInhibitDraw = TRUE;

	// Set up a rapid access string for the markers changed to be now unfiltered,
	// and another for the markers now to be filtered. Unfiltering has to be done
	// before filtering is done.
	wxString strMarkersToBeUnfiltered;
	wxString strMarkersToBeFiltered;
	strMarkersToBeUnfiltered.Empty();
	strMarkersToBeFiltered.Empty();
	wxString valStr;
	// the m_FilterStatusMap has only the markers which have their filtering status changed
	MapWholeMkrToFilterStatus::iterator iter;
	for (iter = gpApp->m_FilterStatusMap.begin(); iter != gpApp->m_FilterStatusMap.end(); ++iter)
	{
		if (iter->second == _T("1"))
		{
			strMarkersToBeFiltered += iter->first + _T(' ');
		}
		else
		{
			strMarkersToBeUnfiltered += iter->first + _T(' ');
		}
	}

	// define some useful flags which will govern the code blocks to be entered
	bool bUnfilteringRequired = TRUE;
	bool bFilteringRequired = TRUE;
	if (strMarkersToBeFiltered.IsEmpty())
		bFilteringRequired = FALSE;
	if (strMarkersToBeUnfiltered.IsEmpty())
		bUnfilteringRequired = FALSE;

	// in the block below we determine which SFM set's map to use, and determine what the full list
	// of filter markers is (the changed ones will be in m_FilterStatusMap); we need the map so we
	// can look up USFMAnalysis struct instances
	MapSfmToUSFMAnalysisStruct* pSfmMap;
	pSfmMap = gpApp->GetCurSfmMap(gpApp->gCurrentSfmSet);

	// reset the appropriate USFMAnalysis structs so that TokenizeText() calls will access
	// the changed filtering settings rather than the old settings (this also updates the app's
	// rapid access strings, by a call to SetupMarkerStrings() done just before returning)
	ResetUSFMFilterStructs(gpApp->gCurrentSfmSet, strMarkersToBeFiltered, strMarkersToBeUnfiltered);
	
	// Initialize for the loops. We must loop through the sourcephrases list twice - the
	// first pass will do all the needed unfilterings, the second pass will do the
	// required new filterings - trying to do these tasks in one pass would be much more
	// complicated and therefore error-prone.
	SPList::Node* pos;
	CSourcePhrase* pSrcPhrase = NULL;
	CSourcePhrase* pLastSrcPhrase = NULL;
	wxString mkr;
	mkr.Empty();
	int nFound = -1;
	int start = 0;
	int end = 0;
	int offset = 0;
	int offsetNextSection = 0;
	wxString preStr;
	wxString remainderStr;
	SPList* pSublist = new SPList;

	// do the unfiltering pass through m_pSourcePhrases
	int curSequNum = -1;
	if (bUnfilteringRequired)
	{
		pos = pList->GetFirst();
		bool bDidSomeUnfiltering;
		while (pos != NULL)
		{
			SPList::Node* oldPos = pos;
			pSrcPhrase = (CSourcePhrase*)pos->GetData(); // just gets the pSrcPhrase
			pos = pos->GetNext(); // moves the pointer/iterator to the next node
			curSequNum = pSrcPhrase->m_nSequNumber;
			bDidSomeUnfiltering = FALSE;

			SPList::Node* prevPos = oldPos;
			pLastSrcPhrase = prevPos->GetData(); // abandon; this one is the pSrcPhrase one
			prevPos = prevPos->GetPrevious();
			if (prevPos != NULL)
			{
				// the actual "previous" one
				pLastSrcPhrase = prevPos->GetData();
				prevPos = prevPos->GetPrevious();
				wxASSERT(pLastSrcPhrase);
			}
			else
			{
				// we were at the start of the document, so there is no previous instance
				pLastSrcPhrase = NULL;
			}

			offset = 0;
			offsetNextSection = 0; // points to next char beyond a moved section or unfiltered section
			preStr.Empty();
			remainderStr.Empty();
			bool bWeUnfilteredSomething = FALSE;
			wxString bareMarker;
			bool bGotEndmarker = FALSE; // whm initialized
			bool bHasEndMarker = FALSE; // whm initialized

			// acts on ONE instance of pSrcPhrase only each time it loops, but in so doing
			// it may add many by removing FILTERED status for a series of sourcephrase instances
			if (!pSrcPhrase->m_markers.IsEmpty()) // do nothing when m_markers is empty
			{
				// m_markers often has an initial space, which is a nuisance, so check and remove
				// it if present
				if (pSrcPhrase->m_markers[0] == _T(' '))
					pSrcPhrase->m_markers = pSrcPhrase->m_markers.Mid(1);

				// loop across any filtered substrings in m_markers, until no more are found
				while ((offset = FindFromPos(pSrcPhrase->m_markers,filterMkr,offset)) != -1)
				{
					// get the next one, its prestring and remainderstring too; on return start
					// will contain the offset to \~FILTER and end will contain the offset to the
					// character immediately following the space following the matching \~FILTER*
					mkr = GetNextFilteredMarker(pSrcPhrase->m_markers,offset,start,end);
					if (mkr.IsEmpty())
					{
						// there was an error here... post a reference to its location
						// sequence numbers may not be uptodate, so do so first over whole list so that
						// the attempt to find the chapter:verse reference string will not fail
						pView->UpdateSequNumbers(0);
						if (!gbIsUnstructuredData)
						{
							fixesStr += pView->GetChapterAndVerse(pSrcPhrase);
							fixesStr += _T("   ");
						}
						bSuccessful = FALSE; // make sure the caller will show the error box
						break; // exit this inner loop & iterate to the next CSourcePhrase instance
					}

					// get initial material into preStr, if any
					if (offset > 0)
					{
						// get the marker information which is not filtered material - this stuff
						// will be transfered to the m_markers member of the first CSourcePhrase
						// instance in the first unfiltered string we extract -- and we may append
						// to it in blocks below; but if we dont find a marker to unfilter, the
						// stuff put into preStr will be abandoned because m_markers on the sourcephrase
						// will not have been altered
						preStr += pSrcPhrase->m_markers.Mid(offsetNextSection,offset - offsetNextSection);
						offsetNextSection = offset; // update, since this section has been (potentially) moved
					}

                    // Depending on what Bill thinks we should do, we can either
                    // universally exclude unknown markers from being filtered (for safety,
                    // they should default to being shown in the main window when the doc
                    // is first created), in which case pSfm == NULL would be sufficient
                    // for determining we have found an unknown marker, and such a marker
                    // would not be able to be got into the strMarkersToBeUnfiltered string
                    // from the GUI (we are about to test for inclusion in this string
                    // below, and later to get pSfm too),
					//
                    // OR, unknown markers will be ALL possible candidates for
                    // filtering/unfiltering, and so we'd have to determine whether or not
                    // this marker is in strMarkersToBeUnfiltered - and if it is then we'd
                    // unfilter it, even though pSfm would be NULL when we try to create
                    // the pointer. I'll code for the latter protocol.

                    // determine whether or not the marker we found is one of those
                    // designated for being unfiltered (ie. its content made visible in the
                    // adaptation window)
					wxString mkrPlusSpace = mkr + _T(' '); // prevents spurious Finds, 
														   // such as \h finding \hr, etc
					if (strMarkersToBeUnfiltered.Find(mkrPlusSpace) == -1)
					{
                        // it's not one we need to unfilter, so append it, its content, and
                        // the bracking \~FILTER and \~FILTER* markers, as is, to preStr
						wxASSERT(offset == start);
						wxASSERT(end > start);
						wxString appendStr = pSrcPhrase->m_markers.Mid(offset, end - start);
						preStr += appendStr;
						offsetNextSection = end; // update, this section is deemed moved
					}
					else
					{
                        // we have found a marker with content needing to be unfiltered, so
                        // do so (note: the unfiltering operation will consume some of the
                        // early material, or perhaps all the material, in
                        // pSrcPhase->m_markers; so we will shorten the latter before we
                        // iterate after having done the unfilter for this marker
						bDidSomeUnfiltering = TRUE; // used for updating navText on original pSrcPhrase when done
						bWeUnfilteredSomething = TRUE; // used for reseting initial conditions in inner loop
						pSublist->Clear(); //pSublist->RemoveAll(); // clear list in preparation for Tokenizing
						wxASSERT(offset == start);
						wxASSERT(end > start);
						wxString extractedStr = pSrcPhrase->m_markers.Mid(offset, end - start);

#ifdef _Trace_RebuildDoc
						TRACE2("UNFILTERING: gCurrentSfmSet %d STRING: %s\n", gpApp->gCurrentSfmSet, extractedStr);
#endif

						extractedStr = RemoveAnyFilterBracketsFromString(extractedStr); // we'll tokenize this
                        // note, extractedStr will lack a trailing space if there is a
                        // contentless filtered marker in the passed in string, so we need
                        // to test & if needed adjust m_markers below
						remainderStr = pSrcPhrase->m_markers.Mid(end); // remainder, from end of \~FILTER*
						if (remainderStr[0] == _T(' '))
							remainderStr = remainderStr.Mid(1); // remove an initial space if there is one
						offsetNextSection = end; // update, this section is to be 
												 // unfiltered (on this pass at least)
						// tokenize the substring
						wxASSERT(extractedStr[0] == gSFescapechar);
						int count = pView->TokenizeTextString(pSublist,extractedStr,pSrcPhrase->m_nSequNumber);
						bool bIsContentlessMarker = FALSE;
						
						USFMAnalysis* pSfm = NULL;	// whm moved here from below to insure it is initialized before
													// call to AnalyseMarker

                        // if the unfiltered section ended with an endmarker, then the
                        // TokenizeText() parsing algorithm will create a final
                        // CSourcePhrase instance with m_key empty, but the endmarker
                        // stored in its m_markers member. This last sourcephrase is
                        // therefore spurious, and its stored endmarker really belongs in
                        // the original pSrcPhrase phrase's m_markers member at its
                        // beginning - so check for this and do the transfer if needed,
                        // then delete the spurious sourcephrase instance before
                        // continuing; but if there was no endmarker, then there will be no
                        // spurious sourcephrase instances created so no delete would be
                        // done -- but that is only true if the marker being unfiltered has
                        // text content following; so when unfiltering an unknown filtered
                        // contentless marker, we'll also get a spurious sourcephrase
                        // produced (ie. m_key and m_srcPhrase are empty, but m_markers has
                        // the unknown marker; so we have to take this special case into
                        // consideration too - we don't want to end up inserting this
                        // spurious sourcephrase into the doc's list.
						bGotEndmarker = FALSE; // if it remains false, the endmarker was 
											   // omitted in the source
						bool bHaventAClueWhatItIs = FALSE; // if it's really something
														   // unexpected, set this TRUE below
						int curPos = -1;
						if (!pSublist->IsEmpty())
						{
                           // Note: wxList::GetLast() returns a node, not a pointer to a
                            // data item, so we do the GetLast operation in two parts
							SPList::Node* lastPos = pSublist->GetLast();
							CSourcePhrase* pTailSrcPhrase = lastPos->GetData();
							if (pTailSrcPhrase)
							{
								if (pTailSrcPhrase->m_key.IsEmpty() && !pTailSrcPhrase->m_markers.IsEmpty())
								{
									// m_markers should end with a space, so check and add one if necessary
									// transfer of an endmarker is required, or it may not be an endmarker
									// but a contentless marker - use a bool to track which it is
									wxString endmarkersStr = pTailSrcPhrase->m_markers;
									curPos = endmarkersStr.Find(_T('*'));
									if (curPos == -1)
									{
										// it's not an endmarker, but a contentless (probably unknown) marker
										bIsContentlessMarker = TRUE;
										endingMkrsStr.Empty(); // no endmarkers, so none to later insert
															   // on this iteration
										goto f;
									}

									// it's not contentless, so it must be an endmarker, so ensure the 
									// endmarker is present
									curPos = endmarkersStr.Find(gSFescapechar);
									if (curPos == -1)
									{
                                        // we found no marker at all, don't expect this,
                                        // but do what we must (ie. ignore the rest of the
                                        // block), and set the bool which tells us we are
                                        // stymied (whatever it is, we'll just make it
                                        // appear as adaptable source text further down in
                                        // the code)
										bHaventAClueWhatItIs = TRUE;
										endingMkrsStr.Empty(); // can't preserve what we failed to find
										goto h; // skip all the marker calcs, since it's not a marker
									}
                                    // look for an asterisk after it - if we find one, we
                                    // assume we have an endmarker (but we don't need to
                                    // extract it here because we will do that from the
                                    // USFMAnalysis struct below, and that is safe because
                                    // unknown markers will, we assume, never have a
                                    // matching endmarker -- that is a requirement for
                                    // unknown markers as far as Adapt It is concerned, for
                                    // their safe processing within the app whm note:
                                    // placed .Mid on endmarkerStr since wxString:Find
                                    // doesn't have a start position parameter.
									curPos = FindFromPos(endmarkersStr,_T('*'),curPos + 1);
									if (curPos == -1)
									{
                                        // we did not find an asterisk, so ignore the rest
                                        // (we are not likely to ever enter this block)
										endingMkrsStr.Empty();
										goto f;
									}
									else
									{
										bGotEndmarker = TRUE;

                                        // BEW added 26May06; remember the string, because
                                        // it might not just be the matching endmarker we
                                        // are looking for, but that preceded by an
                                        // embedded endmarker (eg. \fq* preceding a \f*),
                                        // and so we'll later want to copy this string
                                        // verbatim into the start of the m_markers member
                                        // of the pSrcPhrase which is current in the outer
                                        // loop, so that the unfiltered section is
                                        // terminated correctly as well as any nested
                                        // markers being terminated correctly, if in fact
                                        // the last of any such have the endmarker (for
                                        // example, \fq ... \fq* can be nested within a \f
                                        // ... \f* footnote stretch, but while \fq has an
                                        // endmarker it is not obligatory to use it, and so
                                        // we have to reliably handle both \fq*\f* or \f*
                                        // as termination for nesting and the whole
                                        // footnote, as both of these are legal markup
                                        // situations)
										endingMkrsStr = endmarkersStr;
									}

                                    // remove the spurious sourcephrase when we've got an
                                    // endmarker identified (but for the other two cases
                                    // where we skip this bit of code, we'll do further
                                    // things with the spurious sourcephrase below) wxList
                                    // has no direct equivalent to RemoveTail(), but we can
                                    // point an iterator to the last element and erase it
									SPList::Node* lastOne;
									lastOne = pSublist->GetLast();
									pSublist->Erase(lastOne);

									// delete the memory block to prevent a leak, update the count
									DeleteSingleSrcPhrase(pTailSrcPhrase,FALSE); // don't delete the partner pile
									count--;
								} // end of block for detecting the parsing of an endmarker
							}
						}

						// make the marker accessible, minus its backslash
f:						bareMarker = mkr.Mid(1); // remove backslash

						// determine if there is an endmarker
						bHasEndMarker = FALSE;
						extractedStr = MakeReverse(extractedStr);
						curPos = extractedStr.Find(_T('*')); // remember, extractedStr is reversed!!
						// determine if the extracted string has an endmarker at its end
						if (bIsContentlessMarker)
						{
							bHasEndMarker = FALSE;
						}
						else
						{
							if (curPos != -1)
							{
                                // there is an asterisk, but it may be in the text rather
                                // than an endmarker, so check it is part of a genuine
                                // endmarker (this is a safer test than checking
                                // bareMarker's USFMAnalysis, since some endmarkers can be
                                // optional)
								int nStart = curPos + 1; // point past it
								curPos = FindFromPos(extractedStr,gSFescapechar,nStart); // find the next backslash
								wxString possibleEndMkr = extractedStr.Mid(nStart, curPos - nStart);
								possibleEndMkr = MakeReverse(possibleEndMkr); // if an endmarker, this should equal bareMarker
								bHasEndMarker = bareMarker == possibleEndMkr;
							}
						}
						extractedStr = MakeReverse(extractedStr);// restore normal order

                        // point at the marker's backslash in the buffer (ready for calling
                        // AnalyseMarker) 
                        // whm moved code block that was here down past the
                        // h: label

						{	// this extra block extends for the next 28 lines. It avoids the bogus 
							// warning C4533: initialization of 'f_iter' is skipped by 'goto h'
							// by putting its code within its own scope
						bool bFound = FALSE;
						MapSfmToUSFMAnalysisStruct::iterator f_iter;
						f_iter = pSfmMap->find(bareMarker); // find returns an iterator
						if (f_iter != pSfmMap->end())
							bFound = TRUE; 
						if (bFound)
						{
							pSfm = f_iter->second;
							// if it was not found, then pSfm will remain NULL, and we know it
							// must be an unknown marker
						}
						else
						{
							pSfm = (USFMAnalysis*)NULL;
						}

						// we'll need this value for the first of the parsed sourcephrases from the
						// extracted text for unfiltering
						bool bWrap;
						if (bFound)
						{
							bWrap = pSfm->wrap;
						}
						else
						{
							bWrap = FALSE;
						}
						} // end of extra block

                        // set the members appropriately, note intial and final require
                        // extra code -- the TokenizeTextString call tokenizes without any
                        // context, and so we can assume that some sourcephrase members are
                        // not set up correctly (eg. m_bSpecialText, and m_curTextType) so
                        // we'll have to use some of TokenizeText's processing code to get
                        // things set up right. (a POSITION pos2 value of zero is
                        // sufficient test for being at the final sourcephrase, after the
                        // GetNext() call has obtained the final one)
h:						bool bIsInitial = TRUE;
						int nWhich = -1;

						int extractedStrLen = extractedStr.Length();
                        // wx version note: Since we require a read-only buffer we use
                        // GetData which just returns a const wxChar* to the data in the
                        // string.
						const wxChar* pChar = extractedStr.GetData();
						wxChar* pBufStart = (wxChar*)pChar;
						wxChar* pEnd;
						pEnd = (wxChar*)pChar + extractedStrLen; // whm added
						wxASSERT(*pEnd == _T('\0'));
						// lookup the marker in the active USFMAnalysis struct map, 
						// get its struct
						int mkrLen = mkr.Length(); // we want the length including 
												   // backslash for AnalyseMarker()
						SPList::Node* pos2 = pSublist->GetFirst();
						CSourcePhrase* pSPprevious = NULL;
						while (pos2 != NULL)
						{
							SPList::Node* savePos;
							savePos = pos2;
							CSourcePhrase* pSP = (CSourcePhrase*)pos2->GetData();
							pos2 = pos2->GetNext();
							wxASSERT(pSP);
							nWhich++; // 0-based value for the iteration number
							if (bIsInitial)
							{
                                // call AnalyseMarker() and set the flags etc correctly,
                                // taking context into account, for this we need the
                                // pLastSrcPhrase pointer - but it is okay if it is NULL
                                // (Note: pSP is still in the temporary list pSublist,
                                // while pLastSrcPhrase is in the m_pSourcePhrases main
                                // list of the document.)
								pSP->m_curTextType = verse; // assume verse unless AnalyseMarker changes it
								pSP->m_bSpecialText = AnalyseMarker(pSP,pLastSrcPhrase,pBufStart,mkrLen,pSfm);

                                // we have to handle the possibility that pSP might have a
                                // contentless marker, or actually something not a marker
                                // somehow in m_markers, so do these below
								if (bIsContentlessMarker)
								{
                                    //  we want this added 'as is' (including its following
                                    //  space) to pSrcPhrase's m_markers member, in the
                                    //  appropriate place and the remainderStr added, and
                                    //  this pSublist element removed (since its
                                    //  contentless, there can only be this one in the
                                    //  sublist), and continue - to effect the needed
                                    //  result we must set up remainderStr to have preStr
                                    //  plus this marker and space plus remainderStr's
                                    //  previous content, in that order
									wxString s = preStr;
									s += pSP->m_markers;
									s += remainderStr;
									remainderStr = s;
									nWhich = 0;
									DeleteSingleSrcPhrase(pSP,FALSE); // don't leak memory; 
														// don't delete the partner pile
									pSublist->Clear();
									break;
								}
								if (bHaventAClueWhatItIs)
								{
                                    // when we expected a marker in m_markers but instead
                                    // found text (which we hope would never be the case) -
                                    // but just in case something wrongly got shoved into
                                    // m_markers, we want to make it visible and adaptable
                                    // in in the document - if it's something which
                                    // shouldn't be there, then the user can edit the
                                    // source text manually to remove it. In this case, our
                                    // 'spurious' sourcephrase is going to be treated as
                                    // non-spurious, and we'll move the m_markers content
                                    // to m_srcPhrase, and remove punctuation etc and set
                                    // up m_key, m_precPunct and m_follPunct.
									SPList* pSublist2 = new SPList;
									wxString unexpectedStr = pSP->m_markers;
									int count;
									count = pView->TokenizeTextString(pSublist2,unexpectedStr,
																pSrcPhrase->m_nSequNumber);
                                    // the actual sequence number doesn't matter because we
                                    // renumber the whole list later on after the
                                    // insertions are done
									wxASSERT(count > 0);
									CSourcePhrase* pSP2;
									SPList::Node* posX = pSublist2->GetFirst();
									pSP2 = (CSourcePhrase*)posX->GetData();
									posX = posX->GetNext();
                                    // we'll make an assumption that there is only one
                                    // element in pSublist2, which should be a safe
                                    // assumption, and if not -- well, we'll just add the
                                    // append the extra strings and won't worry about
                                    // punctuation except what's on the first element
									pSP->m_markers.Empty();
									pSP->m_srcPhrase = pSP2->m_srcPhrase;
									pSP->m_key = pSP2->m_key;
									pSP->m_precPunct = pSP2->m_precPunct;
									pSP->m_follPunct = pSP2->m_follPunct;
                                    // that should do it, but if there's more, well just
                                    // add the text so we don't lose anything - user will
                                    // have the option of editing what he sees afterwards
									while (posX != NULL)
									{
										pSP2 = (CSourcePhrase*)posX->GetData();
										posX = posX->GetNext();
										pSP->m_srcPhrase += _T(" ") + pSP2->m_srcPhrase;
										pSP->m_key += _T(" ") + pSP2->m_key;
									}
									// delete all the elements in pSP2, and then delete 
									// the list itself
									DeleteListContentsOnly(pSublist2);
									delete pSublist2;
									bHaventAClueWhatItIs = FALSE;
								}

								// is it PNG SFM or USFM footnote marker?
								// comparing first two chars in mkr
								if (mkr.Left(2) == _T("\\f")) // is it PNG SFM or USFM footnote marker?
								{
									// if not already set, then do it here
									if (!pSP->m_bFootnote)
										pSP->m_bFootnote = TRUE;
								}

                                // add any m_markers material to be copied from the parent
                                // (this stuff, if any, is in preStr, and it must be added
                                // BEFORE the current m_markers material in order to
                                // preserve correct source text ordering of previously
                                // filtered stuff)
								if (!preStr.IsEmpty())
								{
									pSP->m_markers = preStr + pSP->m_markers;
								}

                                // completely redo the navigation text, so it accurately
                                // reflects what is in the m_markers member of the
                                // unfiltered section

#ifdef _Trace_RebuildDoc
								TRACE1("UNFILTERING: for old navText: %s\n", pSP->m_inform);
#endif

								pSP->m_inform = RedoNavigationText(pSP);

								pSPprevious = pSP; // set pSPprevious for the next iteration, for propagation
								bIsInitial = FALSE;
							}

							// when not the 0th iteration, we need to propagate the flags, 
							// texttype, etc
							if (nWhich > 0)
							{
								// do propagation
								wxASSERT(pSPprevious);
								pSP->CopySameTypeParams(*pSPprevious);
							}

                            // for the last pSP instance, there could be an endmarker which
                            // follows it; if that is the case, we can assume the main
                            // list's sourcephrase which will follow this final pSP
                            // instance after we've inserted pSublist into the main list,
                            // will already have its correct TextType and m_bSpecialText
                            // value set, and so we won't try change it (and won't call
                            // AnalyseMarker() again to invoke its endmarker-support code
                            // either) instead we will just set sensible end conditions -
                            // such as m_bBoundary set TRUE, and we'll let the TextType
                            // propagation do its job. We will need to check if we have
                            // just unfiltererd a footnote, and if so, set the
                            // m_bFootnoteEnd flag.
							if (pos2 == NULL || count == 1)
							{
                                // pSP is the final in pSublist, so do what needs to be
                                // done for such an instance; (if there is only one
                                // instance in pSublist, then the first one is also the
                                // last one, so we check for that using the count == 1 test
                                // -- which is redundant really since pos2 should be NULL
                                // in that case too, but no harm is done with the extra
                                // test)
								pSP->m_bBoundary = TRUE;
								// rely on the foonote TextType having been propagated
								if (pSP->m_curTextType == footnote)
									pSP->m_bFootnoteEnd = TRUE; 
							}
						} // end of while loop for pSublist elements

                       // now insert the completed CSourcePhrase instances into the
                        // m_pSourcePhrases list preceding the oldPos POSITION
						pos2 = pSublist->GetFirst();
						while (pos2 != NULL)
						{
							CSourcePhrase* pSP = (CSourcePhrase*)pos2->GetData();
							pos2 = pos2->GetNext();
							wxASSERT(pSP);
							// wxList::Insert() inserts before specified position in the list
							pList->Insert(oldPos,pSP); // m_pSourcePhrases will manage these now
						}

                        // remove the moved and unfiltered part from pSrcPhrase->m_markers,
                        // so that the next iteration starts with the as-yet-unhandled text
                        // material in that member (this stuff was stored in the local
                        // wxString remainderStr above) - but if there was an unfiltered
                        // endmarker, we'll later also have to put that preceding this
                        // stuff as well
						pSrcPhrase->m_markers = remainderStr; // this could be empty, and usually would be
						if (bIsContentlessMarker)
						{
							pSrcPhrase->m_inform = RedoNavigationText(pSrcPhrase);
							bIsContentlessMarker = FALSE;
						}

						pSublist->Clear(); // clear the local list (but leave the memory chunks in RAM)

 						wxLogDebug(_T("Loc doc4603 INNER LOOP ; before SequNum Update: curSequNum %d ,  SN = %d"), curSequNum, gpApp->m_nActiveSequNum);
						// update the active sequence number on the view
						// BEW changed 29Jul09, the test needs to be > rather than >=,
						// because otherwise a spurious increment by 1 can happen at the
						// end of the first run through this block
						if (gpApp->m_nActiveSequNum > curSequNum)
						{
                            // adjustment of the value is needed (for unfilterings, the box
                            // location remains a valid one (but not necessarily so when
                            // filtering)
							gpApp->m_nActiveSequNum += count;
						}
					}

					// do the unfiltering adjustments needed when we unfiltered something
					if (bWeUnfilteredSomething)
					{
						bWeUnfilteredSomething = FALSE; // reset for next iteration of inner loop

                        // now it's time to put in the endmarker and space at the beginning
                        // of m_markers, if we found one - (both bGotEndmarker and
                        // bHasEndMarker must both be TRUE if we actually did find a valid
                        // endmarker)
						if (bGotEndmarker && bHasEndMarker)
						{
                            // BEW changed 26May06 to handle endmarker sequences reliably
                            // as well as single endmarkers as before...
							// copy to here the one or more endmarkers that we stored earlier
							pSrcPhrase->m_markers = endingMkrsStr + pSrcPhrase->m_markers;
						}

						// do the setup for next iteration of the loop
						preStr.Empty();
						remainderStr.Empty();
						offset = 0;
						offsetNextSection = 0;
						UpdateSequNumbers(0); // get all the sequence 
											  // numbers into correct order
					}
					else
					{
						// we didn't unfilter anything, so prepare for the next 
						// iteration of inner loop
						offset = offsetNextSection;
					}
				}
			}

			if (bDidSomeUnfiltering)
			{
                // the original pSrcPhrase still stores its original nav text in its
                // m_inform member and this is now out of date because some of its content
                // has been unfiltered and made visible, so we have to recalculate its
                // navtext now
				pSrcPhrase->m_inform = RedoNavigationText(pSrcPhrase);
			}

#ifdef __WXMSW_
			// update progress bar every 20 iterations
			++nOldCount;
			if (nOldCount % 1000 == 0) //if (20 * (nOldCount / 20) == nOldCount)
			{
				msgDisplayed = progMsg.Format(progMsg,gpApp->m_curOutputFilename.c_str(),nOldCount,nOldTotal);
				progDlg.Update(nOldCount,msgDisplayed);
			}
#endif

			endingMkrsStr.Empty();
		} // loop end for checking each pSrcPhrase for presence of material to be unfiltered

	} // end block for test bUnfilteringRequired

	// reinitialize the variables we need
	pos = NULL;
	pSrcPhrase = NULL;
	mkr.Empty();
	nFound = -1;
	bool bBoxLocationDestroyed = FALSE; // set TRUE if the box was within a filtered 
                // section, since that will require resetting it an arbitrary location and
                // the latter could be within a retranslation - so we'd have to do an
                // adjustment
	// do the filtering pass now
	curSequNum = -1;
	if (bFilteringRequired)
	{
		// reinitialize the progress bar, and recalc the layout in case the view
		// does some painting when the progress bar is tampered with below
		UpdateSequNumbers(0); // get the numbers updated, as a precaution

		// reinitialize the progress window for the filtering loop
		nOldTotal = pList->GetCount();
		if (nOldTotal == 0)
		{
            // wx version note: Since the progress dialog is modeless we do not need to
            // destroy or otherwise end its modeless state; it will be destroyed when
            // ReconstituteAfterFilteringChange goes out of scope
			pSublist->Clear();
			delete pSublist;

			// BEW added 29Jul09, turn back on CLayout Draw() so drawing of the view
			// can now be done -- no, do it at end of second_pass
			//GetLayout()->m_bInhibitDraw = FALSE;

			return 0;
		}
		nOldCount = 0;

#ifdef __WXMSW_
		progMsg = _("Pass 2 - File: %s  - %d of %d Total words and phrases");
		msgDisplayed = progMsg.Format(progMsg,gpApp->m_curOutputFilename.c_str(),1,nOldCount);
#endif

        // the following variables are for tracking how the active sequence number has to
        // be updated after each span of material designated for filtering is filtered out
		bool bBoxBefore = TRUE; // TRUE when the active location is before the first sourcephrase being filtered
		bool bBoxAfter = FALSE; // TRUE when it is located after the last sourcephrase being filtered
		// if both are FALSE, then the active location is within the section being filtered out
		int nStartLocation = -1; // gets set to the sequence number for the first sourcephrase being filtered
		int nAfterLocation = -1; // gets set to the sequ num for the first source phrase after the filter section
		int nCurActiveSequNum = gpApp->m_nActiveSequNum;
		wxASSERT(nCurActiveSequNum >= 0);

		wxString wholeMkr;
		wxString bareMkr; // wholeMkr without the latter's backslash
		wxString shortMkr; // wholeShortMkr without the latter's initial backslash
		wxString wholeShortMkr;
		wxString endMkr;
		endMkr.Empty();
		bool bHasEndmarker = FALSE;
		pos = pList->GetFirst();
		SPList::Node* posStart = NULL; // location of first sourcephrase instance 
									   // being filtered out
		SPList::Node* posEnd = NULL; // location of first sourcephrase instance 
									 // after the section being filtered out
		wxString filteredStr; // accumulate filtered source text here
		wxString tempStr;
		preStr.Empty(); // store in here m_markers content (from first pSrcPhrase) 
                        // which precedes a marker for filtering out
		remainderStr.Empty(); // store here the filtering marker and anything which 
                              // follows it (from first pSrcPhrase)
		while (pos != NULL)
		{
            // acts on ONE instance of pSrcPhrase only each time it loops, but in so doing
            // it may remove many by imposing FILTERED status on a series of instances
            // starting from when it finds a marker which is to be filtered out in an
            // instance's m_markers member - when that happens the loop takes up again at
            // the sourcephrase immediately after the section just filtered out
			SPList::Node* oldPos = pos;
			pSrcPhrase = (CSourcePhrase*)pos->GetData();
			pos = pos->GetNext();
			curSequNum = pSrcPhrase->m_nSequNumber;
			bool bGotEndmarker = FALSE; // need this to be set TRUE when we get one,
                    // since we must delay accumulating it into filteredStr until just
                    // before we add the final \~FILTER* bracketing endmarker (ie, after
                    // accumulating the last sourcephrase instance's m_srcPhrase member's
                    // contents)
			nCurActiveSequNum = gpApp->m_nActiveSequNum;
			wxString embeddedMkr;
			wxString embeddedEndMkr;

            // loop until we find a sourcephrase which is a candidate for filtering - such
            // a one will satisfy all of the following requirements:
			// 1. m_markers is not empty
			// 2. m_markers has at least one marker in it (look for gSFescapechar)
			// 3. the candidate marker, if there are more than one in m_markers, must not 
            //       be enclosed by \~FILTER and \~FILTER* markers (the stuff between these
            //       is filtered already)
			// 4. the candidate marker will NEVER be an endmarker
			// 5. the marker's USFMAnalysis struct's filter member is currently set to TRUE
			//		(note: markers like xk, xr, xt, xo, fk, fr, ft etc which are inline 
            //      between \x and its matching \x* or \f and its matching \f*, etc, are
            //      marked filter==TRUE, but they have to be skipped, and their
            //      userCanSetFilter member is **always** FALSE. We won't use that fact,
            //      but we use the facts that the first character of their markers is
            //      always the same, x for cross references, f for footnotes, etc, and that
            //      they will have inLine="1" ie, their inLine value in the struct will be
            //      TRUE. Then when parsing over a stretch of text which is marked by a
            //      marker which has no endmarker, we'll know to halt parsing if we come to
            //      a marker with inLine == FALSE; but if TRUE, then a second test is
            //      needed, textType="none" NOT being current will effect the halt - so
            //      this pair of tests should enable us to prevent parsing overrun. (Note:
            //      we want our code to correctly filter a misspelled marker which is
            //      always to be filtered, after the user has edited it to be spelled
            //      correctly.
			// 6. the marker is listed for filtering in the wxString strMarkersToBeFiltered 
            //      (determined from the m_FilterStatusMap map which is set from the
            //      Filtering page of the GUI)
			wxString markersStr;
			if (pSrcPhrase->m_markers.IsEmpty())
			{

#ifdef __WXMSW__
				// the following copied from bottom of loop to here in order to 
				// remove the goto c; and label
				++nOldCount;
				if (nOldCount % 1000 == 0) //if (20 * (nOldCount / 20) == nOldCount)
				{
					msgDisplayed = progMsg.Format(progMsg,
						gpApp->m_curOutputFilename.c_str(),nOldCount,nOldTotal);
						progDlg.Update(nOldCount,msgDisplayed);
				}
#endif

				continue;
			}

            // NOTE: **** this algorithm allows the user to put italics substrings (marked
            // by \it ... \it*), or similar marker & endmarker pairs, within text spans
            // potentially filterable - this should be a safe because such embedded content
            // marker pairs should have a TextType of none in the XML marker specifications
            // document, and Adapt It will skip such ones, but stop scanning when either
            // inLine is FALSE, or if TRUE, then when TextType is not none ****
			markersStr = pSrcPhrase->m_markers;
			bool bIsUnknownMkr = FALSE;
			int nStartingOffset = 0;
g:			int filterableMkrOffset = ContainsMarkerToBeFiltered(gpApp->gCurrentSfmSet,
						markersStr,strMarkersToBeFiltered,wholeMkr,wholeShortMkr,endMkr,
						bHasEndmarker,bIsUnknownMkr,nStartingOffset);
			if (filterableMkrOffset == -1)
			{
                // either wholeMkr is not filterable, or its not in strMarkersToBeFiltered,
                // or its an endmarker -- if so, just iterate to the next sourcephrase

#ifdef __WXMSW__
				// the following copied from bottom of loop to here in order to remove the 
				// goto c; and label
				++nOldCount;
				if (nOldCount % 1000 == 0) //if (20 * (nOldCount / 20) == nOldCount)
				{
					msgDisplayed = progMsg.Format(progMsg,
						gpApp->m_curOutputFilename.c_str(),nOldCount,nOldTotal);
						progDlg.Update(nOldCount,msgDisplayed);
				}
#endif

				continue;
			}

            // if we get there, it's a marker which is to be filtered out, and we know its
            // offset - so set up preStr and remainderStr so we can commence the filtering
            // properly...
			//
            // We have to be careful here, we can't assume that .Mid(filterableMkrOffset)
            // will deliver the correct remainderStr to be stored till later on, because a
            // filterable marker like \b could be followed by an unfiltered marker like \v,
            // or something filtered (and hence \~FILTER would follow), or even a different
            // filterable marker (like \x), and so we have to check here for the presence
            // of another marker which follows it - if there is one, we have found a marker
            // which is to be filtered, but which has no content - such as \b, and in that
            // case all we need do is bracket it with \~FILTER and \~FILTER* and then retry
            // the ContainsMarkerToBeFiltered() call above.
			//
            // Note: markers like \b which have no content must always be
            // userCanSetFilter="0" because they must always be filtered, or always be
            // unfiltered, but never be able to have their filtering status changed. This
            // is because out code for filtering out when a marker has been changed always
            // assumes there is some following content to the marker, but in the case of \b
            // or similar contentless markers this would not be the case, and our code
            // would then incorrectly filter out whatever follows (it could be inspired
            // text!) until the next marker is encountered. At present, we have specified
            // that \b is always to be filtered, so the code below will turn \b as an
            // unknown and unfiltered marker when PngOnly is the SFM set, to \~FILTER \b
            // \~FILTER* when the user changes to the UsfmOnly set, or the UsfmAndPng set.
            // Similarly for other contentless markers.
			int itsLen = wholeMkr.Length();			
			int nOffsetToNextBit = filterableMkrOffset + itsLen;
			if ((wholeMkr != _T("\\f")) && (wholeMkr != _T("\\x")) &&
				(nFound = FindFromPos(markersStr,gSFescapechar,nOffsetToNextBit)) != -1) 
			{
                // there is a following SF marker which is not a \f or \x (the latter two
                // can have a following marker within their scope, so whether that happens
                // or not, they are not to be considered as contentless), so the current
                // one cannot have any text content -- this follows from the fact that the
                // text content of a marker cannot appear in m_markers unless it has been
                // filtered out earlier (in which case it will be bracketed by filterMkr
                // and filterMkrEnd), so if we find another marker then we know the
                // previous one is contentless.

				// extract the marker, including its following space
				wxString contentlessMkr = markersStr.Mid(filterableMkrOffset,
											nOffsetToNextBit - filterableMkrOffset);
                // wx version note: Since we require a read-only buffer we use GetData
                // which just returns a const wxChar* to the data in the string.
				const wxChar* ptr = contentlessMkr.GetData();
                // BEW changed 05Oct05, because GetFilteredItemBracketed call with wholeMkr
                // as the first parameter results in the SF marker in wholeMkr being
                // overwritten by the temp string's contents internally; so I changed the
                // function to not have the first parameter anymore
				
				// whm added two lines below because wxStringBuffer needs to insure buffer 
				// ends with null char
				wxChar* pEnd;
				pEnd = (wxChar*)ptr + itsLen;
				wxASSERT(*pEnd == _T('\0'));

				wxString temp;
				temp = GetFilteredItemBracketed(ptr,itsLen); // temp lacks a final space
				temp += _T(' '); // add the required trailing space

				// now replace the contentless marker with the bracketed contentless marker
				//markersStr.Delete(filterableMkrOffset,nOffsetToNextBit - filterableMkrOffset);
				markersStr.Remove(filterableMkrOffset,nOffsetToNextBit - filterableMkrOffset);
				// for an unknown reason it does not delete the space, so I have to test and
				// if so, delete it
				if (markersStr[filterableMkrOffset] == _T(' '))
				{
					// wxString::Remove needs 1 as second parameter otherwise it truncates 
					// remainder of string
					markersStr.Remove(filterableMkrOffset,1); // delete extra space 
															  // if one is here
				}
				markersStr = InsertInString(markersStr,filterableMkrOffset,temp);

				// now update the m_markers member to contain this marker 
				// appropriately filtered
				pSrcPhrase->m_markers = markersStr;

                // get the navigation text set up correctly (the contentless marker just
                // now filtered out should then no longer appear in the nav text line)
				pSrcPhrase->m_inform = RedoNavigationText(pSrcPhrase);

				// advance beyond this just-filtered contentless marker's location 
				// and try again
				int tempLen = temp.Length();
				nStartingOffset = nOffsetToNextBit - itsLen + tempLen - 1;
				goto g;
			}
			else
			{
				// there is no following SF marker, so the current one may be 
				// assumed to have content
				;
			}
			preStr = markersStr.Left(filterableMkrOffset); // this stuff we 
														// accumulate later on
			remainderStr = markersStr.Mid(filterableMkrOffset); // this stuff is the 
                                // beginning of the filtered text, ie. marker and whatever
                                // follows
			bareMkr = wholeMkr.Mid(1); // remove the backslash, we already know 
									   // it has no final *

            // it is not valid to have formed a prefix like \f or \x if the marker from
            // which it is being formed is already only a single character preceded by a
            // backslash, so if this is the case then make WholeShortMkr, and shortMkr,
            // empty strings
            // BEW extended test, 2Jul05, to prevent a marker like \imt (which gives a
            // short form of \i) from giving spurious shortMkr matches with unrelated
            // markers like \ip \ipq \im \io1 2 or 3, \is1 etc.
			if (wholeShortMkr == wholeMkr || 
				((wholeShortMkr != _T("\\f")) && (wholeShortMkr != _T("\\x"))) )
			{
                // this will prevent spurious matches in our code below, and prevent \f or
                // \x from being wrongly interpretted as the embedded context markers like
                // \fr, \ft, etc, or \xo, etc
				shortMkr.Empty();
				wholeShortMkr.Empty();
			}
			else
			{
				shortMkr = wholeShortMkr.Mid(1);
			}
            // the only shortMkr forms which get made, as a result of the above, are \f and
            // \x - anything else will be forced to an empty string, so that the code for
            // determining the ending sourcephrase will not wrongly result in loop
            // iteration when loop ending should happen

            // okay, we've found a marker to be filtered, we now have to look ahead to find
            // which sourcephrase instance is the last one in this filtering sequence - we
            // will assume the following:
			// 1. unknown markers never have an associated endmarker - so their extent ends 
            //      when a subsequent marker is encounted with a TextType different than
            //      none
			// 2. if the marker has an endmarker, then any other markers to be skipped over 
            //      before the endmarker is encountered will have the same one-character
            //      prefix, eg. f for footnotes (since other markers acceptable in
            //      footnotes are \fk, \fr, \ft etc - all begin with \f), x for cross
            //      references, (and so no other markers will appear in such sections - but
            //      if they do, then our code will still be ok so long as their TextType is
            //      none - because these will get skipped, otherwise such markers will
            //      terminate the section prematurely, & some of the content would remain
            //      unfiltered)
			// 3. filterable markers which lack an endmarker will end their content when the 
            //      next marker is encountered which has the following properties, (1) its
            //      inLine value is FALSE, or if inLine is TRUE then (2) its TextType value
            //      is anything other than none (none is an enum with value 6)
			// 4. markers which have an optional endmarker will end using criterion 3. above,
            //      unless the marker at that location has its first two characters
            //      identical to wholeShortMkr - in which case the section will end when
            //      the first marker is encountered for which that is not so

			// we can commence to build filteredStr now
			filteredStr = filterMkr; // add the \~FILTER beginning marker
			filteredStr += _T(' '); // add space
			filteredStr += remainderStr; // add the marker etc (it may be followed by others,
								// eg. \f could be followed by space and then \fr and space)
			if (pSrcPhrase->m_srcPhrase.IsEmpty())
			{
				// this should not ever be the case, but we'll code defensively so 
				// we don't get consecutive spaces
				;
			}
			else
			{
				filteredStr += pSrcPhrase->m_srcPhrase + _T(' '); // m_srcPhrase NEVER 
																  // ends with a space
			}
			// NOTE: we'll deal with preStr when we set up pUnfilteredSrcPhrase's 
			// m_markers member later on

			// we can now partly or fully determine where the active location is in 
			// relation to this location
			if (nCurActiveSequNum < pSrcPhrase->m_nSequNumber)
			{
				// if control comes here, the location is determinate
				bBoxBefore = TRUE;
				bBoxAfter = FALSE;
			}
			else
			{
                // if control comes here, the location is indeterminate - it might yet be
                // within the filtering section, or after it - we'll assume the latter, and
                // change it later if it is wrong when we get to the first sourcephrase
                // instance following the section for filtering
				bBoxBefore = FALSE;
				bBoxAfter = TRUE;
			}
			nStartLocation = pSrcPhrase->m_nSequNumber;

            // destroy this pSrcPhrase, but leave its pointer in pList until all in this
            // section are dealt with (then use posStart and posEnd to remove their
            // pointers from pList - ie from m_pSourcePhrases)
			posStart = oldPos; // preserve this starting location for later on

            // enter an inner loop which accumulates the filtered marker's source text
            // contents (& replacing any m_markers content if non-empty) until the end of
            // the section for filtering is determined
			SPList::Node* pos2; // this is the 'next' location
			CSourcePhrase* pSrcPhr;
			CSourcePhrase* pSrcPhraseNext; // this could have in its m_markers the 
										   // endmarker which ends the section
			SPList::Node* savePos2 = NULL;
			bool bHasEmbeddedMarker = FALSE;
			bool bNeedEmbeddedEndMarker = FALSE;
			bool bAddRemovedEmbeddedEndMarker = FALSE;

			for (pos2 = pos; pos2 != NULL; )
			{
				savePos2 = pos2;
				pSrcPhr = (CSourcePhrase*)pos2->GetData();
				pos2 = pos2->GetNext();
				wxASSERT(pSrcPhr);
				bool bAtEnd = FALSE;

                // If there are embedded markers in this section, such as keyword markers
                // (\k \k*) or italics or bold etc, these have endmarkers - but the marker
                // which prompted this section for filtering may not take an endmarker (and
                // so the bHasEndmarker flag will continue FALSE for each sourcephrase
                // dealt with), so the endmarkers on embedded markers will require special
                // code to handle the embedding properly. We do it here by having a block
                // which will test for such embedded markers and do the required handling
                // before the code block for bHasEndmarker gets to look at the content of
                // m_markers, and our code here has to do whatever adjustments to m_markers
                // are required (such as removing a matching endmarker from the start of
                // m_markers and resaving m_markers on pSrcPhrase without the endmarker) so
                // that the code which follows this section only has to deal with decisions
                // relating to the span being filtered and its marker (and potential
                // endmarker). We assume any embedded markers occur without any other
                // marker, and don't embed within another such pair (if that happens, it
                // would defeat our code below); and the only embedded markers this code
                // block is going to handle are those with TextType == none
				int embeddedOffset;
				int embeddedItemLen;
				if (!pSrcPhr->m_markers.IsEmpty()) // skip if there is nothing in m_markers
				{
					wxString embMkrs = pSrcPhr->m_markers;
					embeddedOffset = embMkrs.Find(gSFescapechar);
					if (embeddedOffset >= 0) // there is a marker in m_markers
					{
						int len = embMkrs.Length();
                        // wx version note: Since we require a read-only buffer we use
                        // GetData which just returns a const wxChar* to the data in the
                        // string.
						const wxChar* pChar = embMkrs.GetData();
						wxChar* pBufStart = (wxChar*)pChar;
						wxChar* pEnd;
						pEnd = (wxChar*)pChar + len;
						wxASSERT(*pEnd == _T('\0'));

						embeddedItemLen = ParseMarker(pBufStart + embeddedOffset);
						wxString strMkr(pChar + embeddedOffset, embeddedItemLen);
						embeddedMkr = strMkr;

						// if its an endmarker, then this block is not interested in it
						strMkr = MakeReverse(strMkr);
						if (strMkr[0] != _T('*')) // exit block if it's an endmarker
						{
							wxString bareMkr = embeddedMkr.Mid(1);
							USFMAnalysis* pStruct = LookupSFM(bareMkr);
							if (pStruct != NULL) // exit block if the marker 
												 // is an unknown one
							{
                                // we can now determine if this is an embedded marker which
                                // requires an endmarker -- since the point of this code
                                // block to to handle such ones (the rest of the code
                                // already handles other possibilities)
								if (pStruct->inLine && (pStruct->textType == none) && 
									!pStruct->endMarker.IsEmpty())
								{
                                    // we don't have to actually move the m_markers
                                    // contents anywhere in this block because it gets done
                                    // by code at the end of the inner loop anyway, but we
                                    // do here need to set the bool values which enable the
                                    // embedded endmarker, once encountered, to be removed
                                    // from the begining of the pSrcPhrase->m_markers which
                                    // has it as its first marker
									bHasEmbeddedMarker = TRUE;
									bNeedEmbeddedEndMarker = TRUE;
									bAddRemovedEmbeddedEndMarker = FALSE; // this gets set 
														// in next block eventually 
								}
							}
						}
					}
				}
                // unilaterally check for the matching embedded endmarker on the 'next'
                // sourcephrase whenever the above block exits with bHasEmbeddedMarker TRUE
                // and bNeedEmbeddedEndMarker TRUE, remembering that 'next' might be well
                // down the track after several more sourcephrases have been processed
				if (bHasEmbeddedMarker && bNeedEmbeddedEndMarker)
				{
					CSourcePhrase* pSrcPhraseNext = (CSourcePhrase*)pos2->GetData();
					wxASSERT(pSrcPhraseNext);
					if (!pSrcPhraseNext->m_markers.IsEmpty())
					{
						embeddedEndMkr = embeddedMkr + _T("*"); // construct the matching 
																// embedded endmarker
						int curPos = pSrcPhraseNext->m_markers.Find(embeddedEndMkr);

                        // we are only interested in the endmarker being initial in
                        // m_markers, that's the only way it can be doing the job of ending
                        // off an embedded marker section
						if (curPos == 0) // when TRUE, it's the one we want at the 
										 // start where we expect it
						{
                            // since we know it's the matching endmarker for one which is
                            // inLine == TRUE and has TextType of none, we don't need to
                            // access the USFMAnalysis struct, but we do need to enable the
                            // flag which will govern the delayed addition of the endmarker
                            // to the being-accumulated filteredStr wxString text (it's to
                            // be done after the current pSrcPhr's m_srcPhrase member has
                            // been added to filteredStr - which is done further below);
                            // and we have to remove the embeddedEndMkr from
                            // pSrcPhraseNext's m_markers member now, (keeping it for later
                            // on), so it won't be present when the inner loop's code has
                            // to handle finishing off the current section being filtered
                            // so that the whole lot can receive the final \~FILTER*
                            // bracking marker
							bAddRemovedEmbeddedEndMarker = TRUE; // cleared to FALSE again 
                                    // after the delayed addition to filteredStr has been
                                    // done (below)
							int len = embeddedEndMkr.Length();
                            // remove it, and any space which may follow (and if that's the
                            // only marker present, that would clear m_markers as well)
							pSrcPhraseNext->m_markers.Remove(0,len);
							while (pSrcPhraseNext->m_markers[0] == _T(' '))
								pSrcPhraseNext->m_markers = pSrcPhraseNext->m_markers.Mid(1);
							// embeddedEndMkr preserves the marker itself for later on 
							// when the addition happens
						}
					}
				}

                // check to determine if we require an endmarker, and look for it at the
                // sourcephrase beyond the current one - this would be a NULL pointer if
                // pos2 was advanced beyond the end of the m_pSourcePhrases list, which
                // would be a valid criterion for terminating the section anyway;
                // otherwise, we are interested to know if there is the matching endmarker
                // at the start of pSrcPhraseNext's m_markers wxString member; otherwise we
                // are interested in whether or not pSrcPhr's m_markers string contains
                // another marker - if so, we are at the end
				if (pos2 == NULL)
				{
					// we are at the end of the doc, and at the end of the section for 
					// filtering out
					bAtEnd = TRUE;
					pSrcPhraseNext = NULL;
				}
				else if (bHasEndmarker)
				{
                    // the marker in wholeMkr should have a matching endmarker, but beware,
                    // some such markers can optionally have the matching endmarker
                    // omitted, so we can't rely on it being present - that means that the
                    // failure to detect the endmarker on pSrcPhraseNext is not an
                    // indication that the loop should continue, but rather we have to
                    // allow the block at label d: below to be done in case the endmarker
                    // was omitted and we really are at the end of the section for
                    // filtering
					pSrcPhraseNext = (CSourcePhrase*)pos2->GetData();
					wxASSERT(pSrcPhraseNext);
					int curPos = pSrcPhraseNext->m_markers.Find(endMkr);
					if (curPos == -1)
					{
                        // pSrcPhraseNext does not contain the matching endmarker, so check
                        // out the possibility that it was merely omitted
						goto e;
					}
					else
					{
                        // pSrcPhraseNext does contain the matching endmarker, so we must
                        // extract it and any preceding nested endmarker (such as \fq*
                        // before \f*) from the m_markers string (updating the latter to
                        // not have it any longer) and later add it to the end of
                        // filteredStr after the m_srcPhrase member's contents of
                        // pSrcPhrase (NOT pSrcPhraseNext) have been accumulated at label
                        // d: below
						bAtEnd = TRUE;
						bGotEndmarker = TRUE; // used below to add the endmarker after 
                            // m_srcPhrase is added because if we do it here it would be
                            // too early in filteredStr

						wxString nextMarkersStr = pSrcPhraseNext->m_markers;
						wxASSERT(!nextMarkersStr.IsEmpty());

                        // the endmarker will usually be first, but it may have a nested
                        // endmarker before it, so we must take the endmarker and any
                        // nested preceding one as a substring, remove it and store in
                        // endingMkrsStr so we can add it later in the section commencing
                        // at label d: below.
						nFound = curPos;
						int lenEndMkr = endMkr.Length();
						nFound += lenEndMkr; // nFound now is the offset to first 
											 // character after endMkr
						endingMkrsStr = nextMarkersStr.Left(nFound); // store all that 
																	 // stuff till later
						nextMarkersStr = nextMarkersStr.Mid(nFound); // remove it from 
																	 // nextMarkersStr
                        // if there is a following space after endMkr, we will accumulate
                        // it too, but any extra spaces after that we'll throw away (BEW
                        // 25May06)
						if (nextMarkersStr[0] == _T(' '))
						{
							endingMkrsStr += _T(' ');
							nextMarkersStr = nextMarkersStr.Mid(1);
						}
						while (nextMarkersStr[0] == _T(' '))
						{
                            // remove any additional initial spaces (since we've already
                            // normalized, we can be certain that \r or \n will not be
                            // there)
							nextMarkersStr = nextMarkersStr.Mid(1);
						}
						pSrcPhraseNext->m_markers = nextMarkersStr; // update pSrcPhraseNext
					}
				}
				else // there is no endmarker on the 'next' sourcephrase, so check 
					 // if the current one is the end
				{
                    // the marker in wholeMkr does not have a matching endmarker, so we are
                    // looking for a sourcephrase instance which has a stored marker
                    // different than in wholeMkr; and because there is no matching
                    // endmarker, the condition reduces to any sourcephrase instance whose
                    // m_markers string is non-empty and contains the marker which is
                    // inLine == FALSE, or if that is TRUE, then TextType is not equal to
                    // none --- except when we get here from the block above which checks
                    // for an endmarker in pSrcPhraseNext and did not find it, in which
                    // case we may still be at the end, so we use the above criteria as
                    // well, and if a shortened marker form exists we must allow for that
                    // possibility here too (these we would need to skip over and so filter
                    // them out plus their content - but code above ensures that the only
                    // short markers which get to here are \f or \x, anything else would
                    // give errors in in the logic below)
e:					if (pSrcPhr->m_markers.IsEmpty())
					{
						// this can't be the sourcephrase instance which is the end of the 
						// section for filtering
						goto d; // accumulate this instance's source text 
								// into the string being filtered
					}
					if (pSrcPhr->m_markers.Find(gSFescapechar) != -1)
					{
                        // its m_markers member has at least one marker in it, so we are
                        // potentially at the end (so we'll assume we are not at the end if
                        // the wholeShortMkr exists, and is in this m_markers member;
                        // otherwise we could be at the end and so we have to make more
                        // careful tests to determine if that is so - by looking at inLine,
                        // and TextType)
						if (wholeShortMkr.IsEmpty())
						{
                            // we might be at the end, because there's no shortened marker
                            // to look for, -- so check it out
							bAtEnd = IsEndingSrcPhrase(gpApp->gCurrentSfmSet, pSrcPhr->m_markers);
							if (!bAtEnd)
							{
								// accumulate the m_markers material here, m_srcPhrase is 
								// done below
								filteredStr += pSrcPhr->m_markers;
							}
						}
						else
						{
							// the shortened marker form exists, so check for its presence
							if (pSrcPhr->m_markers.Find(wholeShortMkr) == -1)
							{
								// the wholeShortMkr is absent, so we might be at the end 
								// - so check it out
								bAtEnd = IsEndingSrcPhrase(gpApp->gCurrentSfmSet, pSrcPhr->m_markers);
								if (!bAtEnd)
								{
									// accumulate the m_markers material here, m_srcPhrase 
									// is done below
									filteredStr += pSrcPhr->m_markers;
								}
							}
							else
							{
                                // we found a marker with a matching short form, so we must
                                // handle it appropriately (ie. accumulate it and allow
                                // iteration to continue by leaving bAtEnd remain FALSE)
								filteredStr += pSrcPhr->m_markers;
							}
						}
					}
					else
					{
                        // its m_markers has some non-marker content - this should not
                        // happen, but we'll code defensively and permit iteration of the
                        // loop
						filteredStr += pSrcPhr->m_markers; // accumulate whatever it is
					}
				}

				// accumulate source text substring being filtered out
d:				if (!bAtEnd || bGotEndmarker)
				{
                    // note, if we entered here because bGotEndmarker was TRUE, then bAtEnd
                    // will also be TRUE (and the endmarker will have been detected in the
                    // pSrcPhraseNext's m_markers member, which means that the current
                    // pSrcPh instance has m_srcPhrase text needing to be accumulated now,
                    // and code further on will extract the endmarker from pSrcPhraseNext
                    // and accumulate it too)
					filteredStr += pSrcPhr->m_srcPhrase + _T(' '); // BEW 26May06 the space 
																   // added here is benign

                    // if we have encountered an embedded marker stretch with an endmarker,
                    // then we'll have looked ahead to the next sourcephrase and determine
                    // if it has the required embeddedEndMkr, and set a flag to tell us so
                    // when we get here - use the flag now to add the closing embedded
                    // endmarker text (when the flag is TRUE, the 'next' sourcephrase has
                    // already had that initial embeddedEndMkr removed from its m_markers
                    // string); and adding the endmarker must also clear the flags, in case
                    // there are several subspans of embedded markers
					if (bAddRemovedEmbeddedEndMarker)
					{
						filteredStr += embeddedEndMkr + _T(' ');// BEW 26May06 the space 
                                // added here is benign because if the embedded end marker
                                // was placed in the source prior to final punctuation,
                                // then the punctuation will be detached an in its own
                                // CSourcePhrase with empty m_key, and code elsewhere in
                                // the app will detect detached & isolated final
                                // punctuation following an endmarker (embedded or not) and
                                // remove the space so as to tuck the final punctuation
                                // back up to where it should be - that is, immediately
                                // after the endmarker. This kind of adjustment is done at
                                // export time, and also for RTF interlinear exports.
						bAddRemovedEmbeddedEndMarker = FALSE;
						bHasEmbeddedMarker = FALSE;
						bNeedEmbeddedEndMarker = FALSE;
					}
				}
				else if ((bAtEnd && bHasEndmarker && !bGotEndmarker) || 
							(bAtEnd && !bHasEndmarker))
				{
					// don't accumulate here if either test succeeds, because we will have 
					// got here either:
					// (a) we expected an endmarker on pSrcPhraseNext (bHasEndmarker is TRUE) 
                    // but did not actually find one there; so an extra iteration of the
                    // loop has been done and then the code at e: has determined that on
                    // the former 'next' sourcephrase which has now become the current one
                    // (ie. pSrcPh) there is a marker in its m_markers member which halts
                    // scanning;
					// or
					// (b) we don't expect an endmarker (bHasEndmarker is FALSE), and the code
                    // at e: has determined that the current sourcephrase (pSrcPh) has a
                    // marker which halts scanning (bAtEnd == TRUE).
					//
                    // Both scenario (a) or (b) result in a halt, but with the scan having
                    // gone one iteration too far - by that we mean that we don't want to
                    // accumulate the m_srcPhrase text on the pSrcPh which is current at
                    // this time because it is a sourcephrase which is not to be filtered -
                    // it is in fact the first one of the next section when scanning
                    // resumes. However, pos2 has been updated beyond the current location
                    // -- so we have to reset it to the earlier location in order that the
                    // outer loop will take off from the correct POSITION in the list.
					pos2 = savePos2; // adjust pos2 to be at the current location, not the 
									 // next one
				}

				// are we at the end sourcephrase instance for this section?
				if (bAtEnd)
				{
					if (bGotEndmarker)
					{
                        // BEW changed 25May06; accumulate the endmarker, or endmarker and
                        // a preceding nested endmarker, and a following space (if there
                        // was one, it will be already in endingMkrsStr), since we located
                        // the endmarker earlier but set bGotEndmarker to TRUE so we could
                        // accumulate the endmarker, or endmarker pair, here rather than
                        // earlier when we found it on the 'next' sourcephrase
						filteredStr += endingMkrsStr;
						endingMkrsStr.Empty(); // ready for potential next iteration
					}
					bAtEnd = FALSE;
					break; // exit the inner loop
				}
				// iterate the inner loop, if bAtEnd was not set TRUE
			} // end of inner loop, for searching for the place where to end the 
			  // filtered section
			posEnd = pos2; // valid assignment, even if pos2 is NULL because the 
						   // end of the doc was reached

            // complete the determination of where the active location is in relation to
            // this filtered section, and work out the active sequ number adjustment needed
            // and make the adjustment
			if (bBoxBefore == FALSE)
			{
				// adjustment maybe needed only when we know the box was not located 
				// preceding the filter section
				if (posEnd == NULL)
				{
                    // at the document end, so everything up to the end is to be filtered;
                    // so either the active location is before the filtered section (ie.
                    // bBoxBefore == TRUE), or it is within the filtered section (it.
                    // bBoxBefore == FALSE)
					bBoxAfter = FALSE;
				}
				else
				{
					// posEnd is defined, so get the sequence number for this location
					nAfterLocation = posEnd->GetData()->m_nSequNumber;

					// work out if an adjustment to bBoxAfter is needed (bBoxAfter is 
					// set TRUE so far)
					if (nCurActiveSequNum < nAfterLocation)
						bBoxAfter = FALSE; // the box lay within this section 
										   // being filtered
				}
			}
			bool bPosEndNull = posEnd == NULL ? TRUE : FALSE;

			// add the bracketing end filtermarker \~FILTER*, and a final space
			filteredStr += filterMkrEnd;
			filteredStr += _T(' ');

            // remove the pointers from the m_pSourcePhrases list (ie. those which were
            // filtered out), and delete their memory chunks; any adaptations on these are
            // lost forever, but not from the KB unless the latter is rebuilt from the
            // document contents at a later time
			SPList::Node* pos3; //POSITION pos3;
			int filterCount = 0;
			for (pos2 = posStart; (pos3 = pos2) != posEnd; )
			{
				filterCount++;
				CSourcePhrase* pSP = (CSourcePhrase*)pos2->GetData();
				pos2 = pos2->GetNext();
				DeleteSingleSrcPhrase(pSP,FALSE); // don't leak memory, don't delete 
												  // the partner pile
				pList->DeleteNode(pos3);
			}

            // update the sequence numbers on the sourcephrase instances which remain in
            // the list and reset nAfterLocation and nStartLocation accordingly
			UpdateSequNumbers(0);
			nAfterLocation = nStartLocation;
			nStartLocation = nStartLocation > 0 ? nStartLocation - 1: 0;

			// we can now work out what adjustment is needed
			// 1. if the active location was before the filter section, no adjustment is 
			//     needed
			// 2. if it was after the filter section, the active sequence number must be 
            //     decreased by the number of sourcephrase instances in the section being
            //     filtered out
			// 3. if it was within the filter section, it will not be possible to preserve 
            //     its location in which case we must try find a safe location (a) as close
            //     as possible after the filtered section (when posEnd exists), or (b), as
            //     close as possible before the filtered section (when posEnd is NULL)
			if (!bBoxBefore)
			{
				if (bBoxAfter)
				{
					nCurActiveSequNum -= filterCount;
				}
				else
				{
					// the box was located within the span of the material which was 
					// filtered out
					bBoxLocationDestroyed = TRUE;
					if (bPosEndNull)
					{
                        // put the box before the filtered section (this may not be a valid
                        // location, eg. it might be a retranslation section - but we'll
                        // adjust later when we set the bundle indices)
						nCurActiveSequNum = nStartLocation;
					}
					else
					{
                        // put it after the filtered section (this may not be a valid
                        // location, eg. it might be a retranslation section - but we'll
                        // adjust later when we set the bundle indices)
						nCurActiveSequNum = nAfterLocation;
					}
				}
			}
			gpApp->m_nActiveSequNum = nCurActiveSequNum;

            // construct m_markers on the first sourcephrase following the filtered
            // section; if the filtered section is at the end of the document (shouldn't
            // happen, but who knows what a user will do?) then we will need to detect this
            // and create a CSourcePhrase instance with empty key in order to be able to
            // store the filtered content in its m_markers member, and add it to the tail
            // of the doc. A filtered section at the end of the document will manifest by
            // pos2 being NULL on exit of the above loop
			CSourcePhrase* pUnfilteredSrcPhrase = NULL;
			if (posEnd == NULL)
			{
				pUnfilteredSrcPhrase = new CSourcePhrase;

				// force it to at least display an asterisk in nav text area to alert the 
				// user to its presence
				pUnfilteredSrcPhrase->m_bNotInKB = TRUE; 
				pList->Append(pUnfilteredSrcPhrase);
			}
			else
			{
				// get the first sourcephrase instance following the filtered section
				pUnfilteredSrcPhrase = (CSourcePhrase*)posEnd->GetData();
			}
			wxASSERT(pUnfilteredSrcPhrase);

			// fill its m_markers with the material it needs to store, 
			// in the correct order
			tempStr = pUnfilteredSrcPhrase->m_markers; // hold this stuff temporarily, 
							// as we must later add it to the end of everything else
			if (tempStr[0] == _T(' ')) tempStr = tempStr.Mid(1); // lop off any initial
															// space (it's superfluous)
			pUnfilteredSrcPhrase->m_markers = preStr; // any previously assumulated 
													  // filtered info, or markers
			pUnfilteredSrcPhrase->m_markers += filteredStr; // append the newly 
															// filtered material

#ifdef _Trace_RebuildDoc
			TRACE1("FILTERING: starting with old m_inform: %s\n", tempStr);
			TRACE1("FILTERING: adding preStr: %s\n", preStr);
			TRACE2("FILTERING: gCurrentSfmSet %d STRING: %s\n", gpApp->gCurrentSfmSet, filteredStr);
#endif

			pUnfilteredSrcPhrase->m_markers += tempStr; // append whatever was originally 
										// on this srcPhrase in its m_markers member
			preStr.Empty();
			remainderStr.Empty();

			// get the navigation text set up correctly
			pUnfilteredSrcPhrase->m_inform = RedoNavigationText(pUnfilteredSrcPhrase);

			// enable iteration from this location
			if (posEnd == NULL)
			{
				pos = NULL;
			}
			else
			{
				pos = posEnd; // this could be the start of a consecutive section 
							  // for filtering out
			}
			// update progress bar every 20 iterations
//c:			
#ifdef __WXMSW__
			++nOldCount;
			if (nOldCount % 1000 == 0) //if (20 * (nOldCount / 20) == nOldCount)
			{
				msgDisplayed = progMsg.Format(progMsg,
					gpApp->m_curOutputFilename.c_str(),nOldCount,nOldTotal);
					progDlg.Update(nOldCount,msgDisplayed);
			}
#endif
		} // end of loop for scanning contents of successive pSrcPhrase instances

        // prepare for update of view... locate the phrase box approximately where it was,
        // but if that is not a valid location then put it at the end of the doc
		int numElements = pList->GetCount();
		if (gpApp->m_nActiveSequNum > gpApp->GetMaxIndex())
			gpApp->m_nActiveSequNum = numElements - 1;

	} // end of block for bIsFilteringRequired == TRUE

    // BEW changed 29Jul09: do this call unilaterally, bBoxLocationDestroyed is not set
    // TRUE in as many circumstances as we would want, and in the caller use a test of
    // bFilterChangeInDoc being TRUE to suppress resetting phrase box contents using
    // strSavePhraseBox's string value, since we set it here instead - we assume the box
    // location changed and we'll use the m_adaption member at the new location, but if
    // that turns out to be the old active location, the caller will instead use the saved
    // phrase box contents (because m_adaption won't have been set at the time we needed to
    // save the box contents, so we can't use m_adaption there)
    /*
	if (bBoxLocationDestroyed)
	{
		gpApp->GetSafePhraseBoxLocationUsingList(pView);
	}
	*/
    // GetSavePhraseBoxLocationUsingList calculates a safe location (ie. not in a
    // retranslation), sets the view's m_nActiveSequNumber member to that value, and
    // calculates and sets m_targetPhrase to agree with what will be the new phrase box
	// location; it doesn't move the location if it is already safe; in either case it
	// sets the box text to be the m_adaption contents for the current active location
	// at the time this call is made
	gpApp->GetSafePhraseBoxLocationUsingList(pView);

	// remove the progress window, clear out the sublist from memory
	// wx version note: Since the progress dialog is modeless we do not need to destroy
	// or otherwise end its modeless state; it will be destroyed when 
	// ReconstituteAfterFilteringChange goes out of scope
	if (pSublist)
	{
		pSublist->Clear();
		delete pSublist;
	}


	// BEW added 29Jul09, turn back on CLayout Draw() so drawing of the view
	// can now be done
	GetLayout()->m_bInhibitDraw = FALSE;

	return bSuccessful;
}

///////////////////////////////////////////////////////////////////////////////
/// \return		TRUE when we want the caller to copy the pLastSrcPhrase->m_curTextType value to the
///				global enum, gPreviousTextType; otherwise we return FALSE to suppress that global
///				from being changed by whatever marker has just been parsed (and the caller will
///				reset the global to default TextType verse when an endmarker is encountered).
/// \param		pChar		-> points at the marker just found (ie. at its backslash)
/// \param		pAnalysis	-> points at the USFMAnalysis struct for this marker, if the marker 
///								is not unknown otherwise it is NULL.
/// \remarks
/// Called from: the Doc's RetokenizeText().
/// TokenizeText() calls AnalyseMarker() to try to determine, among other things, what the TextType
/// propagation characteristics should be for any given marker which is not an endmarker; for some
/// such contexts, AnalyseMarker will want to preserve the TextType in the preceding context so it
/// can be restored when appropriate - so IsPreviousTextTypeWanted determines when this preservation
/// is appropriate so the caller can set the global which preserves the value
///////////////////////////////////////////////////////////////////////////////
bool CAdapt_ItDoc::IsPreviousTextTypeWanted(wxChar* pChar,USFMAnalysis* pAnalysis)
{
	wxString bareMkr = GetBareMarkerForLookup(pChar);
	wxASSERT(!bareMkr.IsEmpty());
	wxString markerWithoutBackslash = GetMarkerWithoutBackslash(pChar);

	// if we have a \f or \x marker, then we always want to get the TextType on whatever
	// is the sourcephrase preceding either or these
	if (markerWithoutBackslash == _T("f") || markerWithoutBackslash == _T("x"))
		return TRUE;
	// for other markers, we want the preceding sourcephrase's TextType whenever we
	// have encountered some other inLine == TRUE marker which has TextType of none
	// because these are the ones we'll want to propagate the previous type across
	// their text content - to check for these, we need to look inside pAnalysis
	if (pAnalysis == NULL)
	{
		return FALSE;
	}
	else
	{
		// its a known marker, so check if it's an inline one
		if (pAnalysis->inLine && pAnalysis->textType == none)
		{
			return TRUE;
		}
		else
		{
			return FALSE;
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
/// \return		
/// \param		filename	-> the filename to associate with the current document
/// \param		notifyViews	-> defaults to FALSE; if TRUE wxView's OnChangeFilename 
///                            is called for all views 
/// \remarks
/// Called from: the App's OnInit(), DoKBRestore(), DoTransformationsToGlosses(),
/// ChangeDocUnderlyingFileDetailsInPlace(), the Doc's OnNewDocument(), OnFileClose(),
/// DoFileSave(), SetDocumentWindowTitle(), DoUnpackDocument(), the View's
/// OnEditConsistencyCheck(), DoConsistencyCheck(), DoRetranslationReport(), the DocPage's
/// OnWizardFinish(), CMainFrame's SyncScrollReceive() and OnMRUFile().
/// Sets the file name associated internally with the current document.
///////////////////////////////////////////////////////////////////////////////
void CAdapt_ItDoc::SetFilename(const wxString& filename, bool notifyViews)
{
    m_documentFile = filename;
    if ( notifyViews )
    {
        // Notify the views that the filename has changed
        wxNode *node = m_documentViews.GetFirst();
        while (node)
        {
            wxView *view = (wxView *)node->GetData();
            view->OnChangeFilename(); 
			// OnChangeFilename() is called when the filename has changed. The default 
			// implementation constructs a suitable title and sets the title of 
			// the view frame (if any).
            node = node->GetNext();
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \return		enum getNewFileState indicating success or error state when reading the file.
/// \param		pstrBuffer	<- a wxString which receives the text file once loaded
/// \param		nLength		<- the length of the loaded text file
/// \param		pathName	-> path and name of the file to read into pstrBuffer
/// \remarks
/// Called from: the Doc's OnNewDocument().
/// Opens and reads a standard format input file into our wxString buffer pstrBuffer which 
/// is used by the caller to tokenize and build the in-memory data structures used by the 
/// View to present the data to the user. Note: the pstrBuffer is null-terminated.
///////////////////////////////////////////////////////////////////////////////
enum getNewFileState CAdapt_ItDoc::GetNewFile(wxString*& pstrBuffer, 
											  wxUint32& nLength, wxString pathName)
{
    // Bruce's Note on GetNewFileBaseFunct():
    // BEW changed 8Apr06: to remove alloca()-dependence for UTF-8 conversion... Note: the
    // legacy (ie. pre 3.0.9 version) form of this function used Bob Eaton's conversion
    // macros defined in SHConv.h. But since ATL 7.0 has introduced 'safe' heap buffer
    // versions of conversion macros, these will be used here. (If Adapt_It.cpp's
    // Conver8to16(), Convert16to8(), DoInputConversion() and ConvertAndWrite() are
    // likewise changed to the safe macros, then SHConv.h could be eliminated from the
    // app's code entirely. And, of course, whatever the export functionalities use has to
    // be checked and changed too...)
    // 
    // whm revised 19Jun09 to simplify (via returning an enum value) and move error
    // messages and presentation of the standard file dialog back to the caller
    // OnNewDocument. The tellenc.cpp encoding detection algorithm was also incorporated to
    // detect encodings, detect when an input file is actually a binary file (i.e., Word
    // documents are binary); detect when the Regular version attempts to load a Unicode
    // input file; and detect and properly handle when the user inputs a file with 8-bit
    // encoding into the Unicode version (converting it as much as possible - similarly to
    // what the legacy MFC app did). The revision also eliminates some memory leaks that
    // would happen if the routine returned prematurely with an error.

	// wxWidgets Notes: 
	// 1. See MFC code for version 2.4.0 where Bruce needed to monkey
	//    with the call to GetNewFile() function using GetNewFileBaseFunct()
	//    and GetNewFileUsingPtr() in order to get the Chinese localized
	//    version to correctly load resources. I've not implemented those
	//    changes to GetNewFile's behavior here because the wxWidgets version
	//    handles all resources differently.
	// 2. GetNewFile() is called by OnNewDocument() in order to get a
	//    standard format input file into our wxString buffer pstrBuffer
	//    which is used by the caller to tokenize and build the in-memory
	//    data structures used by the View to present the data to the user.
	//    It also remembers where the input file came from by storing its
	//    path in m_lastSourceFileFolder.

	// get a CFile and check length of file
	// Since the wxWidgets version doesn't use exceptions, we'll
	// make use of the Open() method which will return false if
	// there was a problem opening the file.
	wxFile file;
	if (!file.Open(pathName, wxFile::read))
	{
		return getNewFile_error_at_open;
	}

	// file is now open, so find its logical length (always in bytes)
	nLength = file.Length(); // MFC has GetLength()

    // whm Design Note: There is no real need to separate the reading of the file into
    // Unicode and non-Unicode versions. In both cases we could use a pointer to wxChar to
    // point to our byte buffer, since in in Unicode mode we use char* and in ANSI mode,
    // wxChar resolves to just char* anyway. We could then read the file into the byte
    // buffer and use tellenc only once before handling the results with _UNICODE
    // conditional compiles.

#ifndef _UNICODE // ANSI version, no unicode support 

	// create the required buffer and then read in the file (no conversions needed)
	// BEW changed 8Apr06; use malloc to remove the limitation of the finite stack size
	wxChar* pBuf = (wxChar*)malloc(nLength + 1); // allow for terminating null byte 
	memset(pBuf,0,nLength + 1);
	wxUint32 numRead = file.Read(pBuf,(wxUint32)nLength);
	pBuf[numRead] = '\0'; // add terminating null
	nLength += 1; // allow for terminating null (sets m_nInputFileLength in the caller)
	
	// The following source code is used by permission. It is taken and adapted
	// from work by Wu Yongwei Copyright (C) 2006-2008 Wu Yongwei <wuyongwei@gmail.com>.
	// See tellenc.cpp source file for Copyright, Permissions and Restrictions.

	init_utf8_char_table();
	const char* enc = tellenc(pBuf, numRead - 1); // don't include null char at buffer end
	if (!(enc) || strcmp(enc, "unknown") == 0)
	{
		gpApp->m_srcEncoding = wxFONTENCODING_DEFAULT;
	}
	else if (strcmp(enc, "latin1") == 0) // "latin1" is a subset of "windows-1252"
	{
		gpApp->m_srcEncoding = wxFONTENCODING_ISO8859_1; // West European (Latin1)
	}
	else if (strcmp(enc, "windows-1252") == 0)
	{
		gpApp->m_srcEncoding = wxFONTENCODING_CP1252; // Microsoft analogue of ISO8859-1 "WinLatin1"
	}
	else if (strcmp(enc, "ascii") == 0)
	{
		// File was all pure ASCII characters, so assume same as Latin1
		gpApp->m_srcEncoding = wxFONTENCODING_ISO8859_1; // West European (Latin1)
	}
	else if (strcmp(enc, "utf-8") == 0
		|| strcmp(enc, "utf-16") == 0
		|| strcmp(enc, "utf-16le") == 0
		|| strcmp(enc, "ucs-4") == 0
		|| strcmp(enc, "ucs-4le") == 0)
	{
		free((void*)pBuf);
		return getNewFile_error_unicode_in_ansi;
	}
	else if (strcmp(enc, "binary") == 0)
	{
		free((void*)pBuf);
		return getNewFile_error_opening_binary;
	}
	else if (strcmp(enc, "gb2312") == 0)
	{
		gpApp->m_srcEncoding = wxFONTENCODING_GB2312; // same as wxFONTENCODING_CP936 Simplified Chinese
	}
	else if (strcmp(enc, "cp437") == 0)
	{
		gpApp->m_srcEncoding = wxFONTENCODING_CP437; // original MS-DOS codepage
	}
	else if (strcmp(enc, "big5") == 0)
	{
		gpApp->m_srcEncoding = wxFONTENCODING_BIG5; // same as wxFONTENCODING_CP950 Traditional Chinese
	}
	
	*pstrBuffer = pBuf; // copy to the caller's CString (on the heap) before malloc
						// buffer is destroyed
	
	free((void*)pBuf);

#else	// Unicode version supports ASCII, ANSI (but may not be rendered right 
    // when converted using CP_ACP), UTF-8, and UTF-16 input (code taken from Bob Eaton's
    // modifications to AsyncLoadRichEdit.cpp for Carla Studio) We use a temporary buffer
    // allocated on the stack for input, and the conversion macros (which allocated another
    // temp buffer on the stack), to end up with UTF-16 for interal string encoding BEW
    // changed 8Apr06, requested by Geoffrey Hunt, to remove the file size limitation
    // caused by using the legacy macros, which use alloca() to do the conversions in a
    // stack buffer; the VS 2003 macros are size-safe, and use malloc for long strings.
	wxUint32 nNumRead;
	bool bHasBOM = FALSE;

	wxUint32 nBuffLen = (wxUint32)nLength + sizeof(wxChar);
	char* pbyteBuff = (char*)malloc(nBuffLen);

	memset(pbyteBuff,0,nBuffLen);
	nNumRead = file.Read(pbyteBuff,nLength);
	nLength = nNumRead + sizeof(wxChar);

	// now we have to find out what kind of encoding the data is in, and set the 
	// encoding and we convert to UTF-16 in the DoInputConversion() function
	if (nNumRead <= 0)
	{
		// free the original read in (const) char data's chunk
		free((void*)pbyteBuff);
		return getNewFile_error_no_data_read;
	}
    // check for UTF-16 first; we allow it, but don't expect it (and we assume it would
    // have a BOM)
	if (!memcmp(pbyteBuff,szU16BOM,nU16BOMLen))
	{
		// it's UTF-16
		gpApp->m_srcEncoding = wxFONTENCODING_UTF16;
		bHasBOM = TRUE;
	}
	else
	{
		// see if it is UTF-8, whether with or without a BOM; if so,
		if (!memcmp(pbyteBuff,szBOM,nBOMLen))
		{
			// the UTF-8 BOM is present
			gpApp->m_srcEncoding = wxFONTENCODING_UTF8;
			bHasBOM = TRUE;
		}
		else
		{
			if (gbForceUTF8)
			{
				// the app is mucking up the source data conversion, so the user wants
				// to force UTF8 encoding to be used
				gpApp->m_srcEncoding = wxFONTENCODING_UTF8;
			}	
			else
			{
                // The MFC version uses Microsoft's IMultiLanguage2 interface to detect
                // whether the file buffer contains UTF-8, UTF-16 or some form of 8-bit
                // encoding (using GetACP()), but Microsoft's stuff is not cross-platform,
                // nor open source.
                // 
                // One possibility for encoding detection is to use IBM's International
                // Components for Unicode (icu) under the LGPL. This is a very large, bulky
                // library of tools and would considerably inflate the size of Adapt It's
                // distribution.
				
                // The following source code is used by permission. It is taken and adapted
                // from work by Wu Yongwei Copyright (C) 2006-2008 Wu Yongwei
                // <wuyongwei@gmail.com>. See tellenc.cpp source file for Copyright,
                // Permissions and Restrictions.

// GDLC Temporary work around for PPC STL library bug
#if defined(__WXMAC__) && defined(__POWERPC__ )
//[code here would build for PowerPC Macs only]
				const char* enc = "utf-8";
				gpApp->m_srcEncoding = wxFONTENCODING_UTF8;
#else
				init_utf8_char_table();
				const char* enc = tellenc(pbyteBuff, nLength - sizeof(wxChar)); // don't include null char at buffer end
#endif
				if (!(enc) || strcmp(enc, "unknown") == 0)
				{
					gpApp->m_srcEncoding = wxFONTENCODING_DEFAULT;
				}
				else if (strcmp(enc, "latin1") == 0) // "latin1" is a subset of "windows-1252"
				{
					gpApp->m_srcEncoding = wxFONTENCODING_ISO8859_1; // West European (Latin1)
				}
				else if (strcmp(enc, "windows-1252") == 0)
				{
					gpApp->m_srcEncoding = wxFONTENCODING_CP1252; // Microsoft analogue of ISO8859-1 "WinLatin1"
				}
				else if (strcmp(enc, "ascii") == 0)
				{
					// File was all pure ASCII characters, so assume same as Latin1
					gpApp->m_srcEncoding = wxFONTENCODING_ISO8859_1; // West European (Latin1)
				}
				else if (strcmp(enc, "utf-8") == 0) // Only valid UTF-8 sequences
				{
					gpApp->m_srcEncoding = wxFONTENCODING_UTF8;
				}
				else if (strcmp(enc, "utf-16") == 0)
				{
					gpApp->m_srcEncoding = wxFONTENCODING_UTF16;
				}
				else if (strcmp(enc, "utf-16le") == 0)
				{
					gpApp->m_srcEncoding = wxFONTENCODING_UTF16LE; // UTF-16 big and little endian are both handled by wxFONTENCODING_UTF16
				}
				else if (strcmp(enc, "ucs-4") == 0)
				{
					gpApp->m_srcEncoding = wxFONTENCODING_UTF32;
				}
				else if (strcmp(enc, "ucs-4le") == 0)
				{
					 gpApp->m_srcEncoding = wxFONTENCODING_UTF32LE;
				}
				else if (strcmp(enc, "binary") == 0)
				{
					// free the original read in (const) char data's chunk
					free((void*)pbyteBuff);
					return getNewFile_error_opening_binary;
				}
				else if (strcmp(enc, "gb2312") == 0)
				{
					gpApp->m_srcEncoding = wxFONTENCODING_GB2312; // same as wxFONTENCODING_CP936 Simplified Chinese
				}
				else if (strcmp(enc, "cp437") == 0)
				{
					gpApp->m_srcEncoding = wxFONTENCODING_CP437; // original MS-DOS codepage
				}
				else if (strcmp(enc, "big5") == 0)
				{
					gpApp->m_srcEncoding = wxFONTENCODING_BIG5; // same as wxFONTENCODING_CP950 Traditional Chinese
				}
			}
		}
	}

	// do the converting and transfer the converted data to pstrBuffer (which then 
	// persists while doc lives)
	gpApp->DoInputConversion(*pstrBuffer,pbyteBuff,gpApp->m_srcEncoding,bHasBOM);

	// update nLength (ie. m_nInputFileLength in the caller, include terminating null in
	// the count)
	nLength = pstrBuffer->Length() + 1; // # of UTF16 characters + null character 
											// (2 bytes)
	// free the original read in (const) char data's chunk
	free((void*)pbyteBuff);

#endif
	file.Close();
	return getNewFile_success;
}

///////////////////////////////////////////////////////////////////////////////
/// \return		a pointer to the running application (CAdapt_ItApp*)
/// \remarks
/// Called from: Most routines in the Doc which need a pointer to refer to the App.
/// A convenience function.
///////////////////////////////////////////////////////////////////////////////
CAdapt_ItApp* CAdapt_ItDoc::GetApp()
{
	return (CAdapt_ItApp*)&wxGetApp();
}

///////////////////////////////////////////////////////////////////////////////
/// \return		TRUE if the character immediately before prt is a newline character (\n)
/// \param		ptr			-> a pointer to a character being examined/referenced
/// \param		pBufStart	-> the start of the buffer being examined
/// \remarks
/// Called from: the Doc's IsMarker().
/// Determines if the previous character in the buffer is a newline character, providing ptr
/// is not pointing at the beginning of the buffer (pBufStart).
///////////////////////////////////////////////////////////////////////////////
bool CAdapt_ItDoc::IsPrevCharANewline(wxChar* ptr, wxChar* pBufStart)
{
	if (ptr <= pBufStart)
		return TRUE; // treat start of buffer as a virtual newline
	--ptr; // point at previous character
	if (*ptr == _T('\n'))
		return TRUE;
	else
		return FALSE;
}

///////////////////////////////////////////////////////////////////////////////
/// \return		TRUE if the character at ptr is a whitespace character, FALSE otherwise.
/// \param		pChar	-> a pointer to a character being examined/referenced
/// \remarks
/// Called from: the Doc's ParseWhiteSpace(), ParseNumber(), IsVerseMarker(), ParseMarker(),
/// MarkerAtBufPtr(), ParseWord(), IsChapterMarker(), TokenizeText(), DoMarkerHousekeeping(),
/// the View's DetachedNonQuotePunctuationFollows(), DoExportSrcOrTgtRTF(), DoesTheRestMatch(),
/// PrecedingWhitespaceHadNewLine(), NextMarkerIsFootnoteEndnoteCrossRef(), 
/// the CViewFilteredMaterialDlg's InitDialog().
/// Note: The XML.cpp file has its own IsWhiteSpace() function which is used within XML.cpp (it
/// does not use wxIsspace() internally but defines whitespace explicitly as a space, tab, \r 
/// or \n.
/// Whitespace is generally defined as a space, a tab, or an end-of-line character/sequence.
///////////////////////////////////////////////////////////////////////////////
bool CAdapt_ItDoc::IsWhiteSpace(wxChar *pChar)
{
	// returns true for tab 0x09, return 0x0D or space 0x20
	if (wxIsspace(*pChar) == 0)// _istspace not recognized by g++ under Linux
		return FALSE;
	else
		return TRUE;

	// equivalent code:
	//if (*pChar == _T('\t') || *pChar == _T('\r') || *pChar == _T(' '))
	//	return TRUE;
	//else
	//	return FALSE;
}

///////////////////////////////////////////////////////////////////////////////
/// \return		the number of whitespace characters parsed
/// \param		pChar	-> a pointer to a character being examined/referenced
/// \remarks
/// Called from: the Doc's GetMarkersAndTextFromString(), TokenizeText(), DoMarkerHousekeeping(),
/// the View's DetachedNonQuotePunctuationFollows(), FormatMarkerBufferForOutput(),
/// FormatUnstructuredTextBufferForOutput(), DoExportInterlinearRTF(), DoExportSrcOrTgtRTF(),
/// DoesTheRestMatch(), ProcessAndWriteDestinationText(), ApplyOutputFilterToText(),
/// ParseAnyFollowingChapterLabel(), NextMarkerIsFootnoteEndnoteCrossRef().
/// Parses through a buffer's whitespace beginning at pChar.
///////////////////////////////////////////////////////////////////////////////
int CAdapt_ItDoc::ParseWhiteSpace(wxChar *pChar)
{
	int	length = 0;
	wxChar* ptr = pChar;
	while (IsWhiteSpace(ptr))
	{
		length++;
		ptr++;
	}
	return length;
}

///////////////////////////////////////////////////////////////////////////////
/// \return		the number of filtering sfm characters parsed
/// \param		wholeMkr	-> the whole marker (including backslash) to be parsed
/// \param		pChar		-> pointer to the backslash character at the beginning of the marker
/// \param		pBufStart	-> pointer to the start of the buffer
/// \param		pEnd		-> pointer at the end of the buffer
/// \remarks
/// Called from: the Doc's TokenizeText().
/// Parses through the filtering marker beginning at pChar (the initial backslash).
/// Upon entry pChar must point to a filtering marker determined by a prior call to 
/// IsAFilteringSFM(). Parsing will include any embedded (inline) markers belonging to the 
/// parent marker.
///////////////////////////////////////////////////////////////////////////////
int CAdapt_ItDoc::ParseFilteringSFM(const wxString wholeMkr, wxChar *pChar, 
									wxChar *pBufStart, wxChar *pEnd)
{
	// whm added 10Feb2005 in support of USFM and SFM Filtering support
	// BEW ammended 10Jun05 to have better parse termination criteria
	// Used in TokenizeText(). For a similar named function used
	// only in DoMarkerHousekeeping(), see ParseFilteredMarkerText().
	// Upon entry pChar must point to a filtering marker determined
	// by prior call to IsAFilteringSFM().
	// ParseFilteringSFM advances the ptr until one of the following
	// conditions is true:
	// 1. ptr == pEnd (end of buffer is reached).
	// 2. ptr points just past a corresponding end marker.
	// 3. ptr points to a subsequent non-inLine and non-end marker. This
	//    means that the "content markers"
	// whm ammended 30Apr05 to include "embedded content markers" in
	// the parsed filtered marker, i.e., any \xo, \xt, \xk, \xq, and 
	// \xdc that follow the marker to be parsed will be included within
	// the span that is parsed. The same is true for any footnote content
	// markers (see notes below).
	int	length = 0;
	int endMkrLength = 0;
	wxChar* ptr = pChar;
	if (ptr < pEnd)
	{
		// advance pointer one to point past wholeMkr's initial backslash
		length++;
		ptr++;
	}
	while (ptr != pEnd)
	{
		if (IsMarker(ptr,pBufStart))
		{
			if (IsCorresEndMarker(wholeMkr,ptr,pEnd))
			{
				// it is the corresponding end marker so parse it
				// Since end markers may not be followed by a space we cannot
				// use ParseMarker to reliably parse the endmarker, so
				// we'll just add the length of the end marker to the length
				// of the filtered text up to the end marker
				endMkrLength = wholeMkr.Length() + 1; // add 1 for *
				return length + endMkrLength;
			}
			else if (IsInLineMarker(ptr, pEnd) && *(ptr + 1) == wholeMkr.GetChar(1))
			{
				; // continue parsing
				// We continue incrementing ptr past all inLine markers following a 
				// filtering marker that start with the same initial letter (after 
				// the backslash) since those can be assumed to be "content markers"
				// embedded within the parent marker. For example, if our filtering
				// marker is the footnote marker \f, any of the footnote content 
				// markers \fr, \fk, \fq, \fqa, \ft, \fdc, \fv, and \fm that happen to 
				// follow \f will also be filtered. Likewise, if the cross reference
				// marker \x if filtered, any inLine "content" markers such as \xo, 
				// \xt, \xq, etc., that might follow \x will also be subsumed in the
				// parse and therefore become filtered along with the \x and \x*
				// markers. The check to match initial letters of the following markers
				// with the parent marker should eliminate the possibility that another
				// unrelated inLine marker (such as \em emphasis) would accidentally
				// be parsed over
			}
			else
			{
				wxString bareMkr = GetBareMarkerForLookup(ptr);
				wxASSERT(!bareMkr.IsEmpty());
				USFMAnalysis* pAnalysis = LookupSFM(bareMkr);
				if (pAnalysis)
				{
					if (pAnalysis->textType == none)
					{
						; // continue parsing
						// We also increment ptr past all inLine markers following a filtering
						// marker, if those inLine markers are ones which pertain to character
						// formatting for a limited stretch, such as italics, bold, small caps,
						// words of Jesus, index entries, ordinal number specification, hebrew or
						// greek words, and the like. Currently, these are: ord, bd, it, em, bdit,
						// sc, pro, ior, w, wr, wh, wg, ndx, k, pn, qs -- and their corresponding
						// endmarkers (not listed here) -- this list is specific to Adapt It, it
						// is not a formally defined subset within the USFM standard
					}
					else
					{
						break;	// it's another marker other than corresponding end marker, or
								// a subsequent inLine marker or one with TextType none, so break 
								// because we are at the end of the filtered text.
					}
				}
				else
				{
					// pAnalysis is null, this indicates either an unknown marker, or a marker from
					// a different SFM set which is not in the set currently active - eiher way, we
					// treat these as inLine == FALSE, and so such a marker halts parsing
					break;
				}
			}
		}
		length++;
		ptr++;
	}
	return length;
}

///////////////////////////////////////////////////////////////////////////////
/// \return		the number of characters parsed
/// \param		wholeMkr	-> the whole marker (including backslash) to be parsed
/// \param		pChar		-> pointer to the backslash character at the beginning of the marker
/// \param		pBufStart	-> pointer to the start of the buffer
/// \param		pEnd		-> pointer at the end of the buffer
/// \remarks
/// Called from: the Doc's DoMarkerHousekeeping().
/// Parses through the filtered material beginning at the initial backslash of \~FILTER
/// and ending when ptr points just past the asterisk of the corresponding \~FILTER* end marker.
/// Upon entry pChar must point to a \~FILTER marker  as determined by a prior call 
/// to IsFilteredBracketMarker().
/// ParseFilteredMarkerText advances the ptr until ptr points just past a
/// corresponding \~FILTER* end marker.
///////////////////////////////////////////////////////////////////////////////
int CAdapt_ItDoc::ParseFilteredMarkerText(const wxString wholeMkr, wxChar *pChar, 
									wxChar *pBufStart, wxChar *pEnd)
{
	// whm added 10Feb2005 in support of USFM and SFM Filtering support
	// This function differs from ParseFilteringSFM() in that this
	// ParseFilteredMarkerText() expects to be pointing to a programatically
	// added \~FILER marker as would be the case in DoMarkerHouseKeeping().
	// Upon entry pChar must point to a \~FILTER marker determined
	// by prior call to IsFilteredBracketMarker().
	// ParseFilteredMarkerText advances the ptr until the following
	// conditions is true:
	// 1. ptr points just past a corresponding \~FILTER* end marker.
	int	length = 0;
	int endMkrLength = 0;
	wxChar* ptr = pChar;
	if (ptr < pEnd)
	{
		// advance pointer one to point past wholeMkr's initial backslash
		length++;
		ptr++;
	}
	while (ptr != pEnd)
	{
		if (IsMarker(ptr,pBufStart))
		{
			if (IsCorresEndMarker(wholeMkr,ptr,pEnd))
			{
				// it is the corresponding end marker so parse it
				// Since end markers may not be followed by a space we cannot
				// use ParseMarker to reliably parse the endmarker, so
				// we'll just add the length of the end marker to the length
				// of the filtered text up to the end marker
				endMkrLength = wholeMkr.Length() + 1; // add 1 for *
				return length + endMkrLength;
			}
		}
		length++;
		ptr++;
	}
	return length;
}

/* // currently unused
///////////////////////////////////////////////////////////////////////////////
/// \return		TRUE if the character pointed to by pChar is a number, FALSE otherwise.
/// \param		pChar		-> pointer to the first character to be examined
/// \remarks
/// Called from: 
/// Determines if the character at pChar is a number. Called after determining that a character
/// sequence was a verse marker followed by whitespace.
///////////////////////////////////////////////////////////////////////////////
bool CAdapt_ItDoc::IsVerseNumber(wxChar *pChar)
{
	// test for digits
	if (wxIsdigit(*pChar) == 0)
		return FALSE;
	else
		return TRUE;
}
*/

///////////////////////////////////////////////////////////////////////////////
/// \return		the number of numeric characters parsed
/// \param		pChar		-> pointer to the first numeric character
/// \remarks
/// Called from: the Doc's TokenizeText(), DoMarkerHousekeeping(), and 
/// DoExportInterlinearRTF().
/// Parses through the number until whitespace is encountered (generally a newline)
///////////////////////////////////////////////////////////////////////////////
int CAdapt_ItDoc::ParseNumber(wxChar *pChar)
{
	wxChar* ptr = pChar;
	int length = 0;
	while (!IsWhiteSpace(ptr))
	{
		ptr++;
		length++;
	}
	return length;
}

///////////////////////////////////////////////////////////////////////////////
/// \return		TRUE if the marker being pointed to by pChar is a verse marker, FALSE otherwise.
/// \param		pChar		-> pointer to the first character to be examined (a backslash)
/// \param		nCount		<- returns the number of characters forming the marker
/// \remarks
/// Called from: the Doc's TokenizeText() and DoMarkerHousekeeping(), 
/// DoExportInterlinearRTF() and DoExportSrcOrTgtRTF().
/// Determines if the marker at pChar is a verse marker. Intelligently handles verse markers
/// of the form \v and \vn.
///////////////////////////////////////////////////////////////////////////////
bool CAdapt_ItDoc::IsVerseMarker(wxChar *pChar, int& nCount)
// version 1.3.6 and onwards will accomodate Indonesia branch's use
// of \vn as the marker for the number part of the verse (and \vt for
// the text part of the verse - AnalyseMarker() handles the latter)
{
	wxChar* ptr = pChar;
	ptr++;
	if (*ptr == _T('v'))
	{
		ptr++;
		if (*ptr == _T('n'))
		{
			// must be an Indonesia branch \vn 'verse number' marker
			// if white space follows
			ptr++;
			nCount = 3;
		}
		else
		{
			nCount = 2;
		}
		return IsWhiteSpace(ptr);
	}
	else
		return FALSE;
}

///////////////////////////////////////////////////////////////////////////////
/// \return		TRUE if pChar points at a \~FILTER (beginning filtered material marker)
/// \param		pChar		-> a pointer to the first character to be examined (a backslash)
/// \param		pEnd		-> a pointer to the end of the buffer
/// \remarks
/// Called from: the Doc's GetMarkersAndTextFromString() and DoMarkerHousekeeping(), 
/// Determines if the marker being pointed at is a \~FILTER marking the beginning of filtered
/// material.
///////////////////////////////////////////////////////////////////////////////
bool CAdapt_ItDoc::IsFilteredBracketMarker(wxChar *pChar, wxChar* pEnd)
{
	// whm added 10Feb2005 in support of USFM and SFM Filtering support
	// determines if pChar is pointing at the filtered text begin bracket \~FILTER
	wxChar* ptr = pChar;
	for (int i = 0; i < (int)wxStrlen_(filterMkr); i++) //_tcslen
	{
		if (ptr + i >= pEnd)
			return FALSE;
		if (*(ptr + i) != filterMkr[i])
			return FALSE;
	}
	return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
/// \return		TRUE if pChar points at a \~FILTER* (ending filtered material marker)
/// \param		pChar		-> a pointer to the first character to be examined (a backslash)
/// \param		pEnd		-> a pointer to the end of the buffer
/// \remarks
/// Called from: the Doc's GetMarkersAndTextFromString().
/// Determines if the marker being pointed at is a \~FILTER* marking the end of filtered
/// material.
///////////////////////////////////////////////////////////////////////////////
bool CAdapt_ItDoc::IsFilteredBracketEndMarker(wxChar *pChar, wxChar* pEnd)
{
	// whm added 18Feb2005 in support of USFM and SFM Filtering support
	// determines if pChar is pointing at the filtered text end bracket \~FILTER*
	wxChar* ptr = pChar;
	for (int i = 0; i < (int)wxStrlen_(filterMkrEnd); i++) //_tcslen
	{
		if (ptr + i >= pEnd)
			return FALSE;
		if (*(ptr + i) != filterMkrEnd[i])
			return FALSE;
	}
	return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
/// \return		the number of characters parsed
/// \param		pChar		-> a pointer to the first character to be parsed (a backslash)
/// \remarks
/// Called from: the Doc's ReconstituteAfterFilteringChange(), GetWholeMarker(), TokenizeText(),
/// DoMarkerHousekeeping(), IsEndingSrcPhrase(), ContainsMarkerToBeFiltered(), 
/// RedoNavigationText(), GetNextFilteredMarker(), the View's FormatMarkerBufferForOutput(),
/// DoExportSrcOrTgtRTF(), FindFilteredInsertionLocation(), IsFreeTranslationEndDueToMarker(),
/// ParseFootnote(), ParseEndnote(), ParseCrossRef(), ProcessAndWriteDestinationText(),
/// ApplyOutputFilterToText(), ParseMarkerAndAnyAssociatedText().
/// Parses through to the end of a standard format marker.
/// Caution: This function will fail unless the marker pChar points at is followed
/// by whitespace of some sort - a potential crash problem if ParseMarker is used for parsing
/// markers in local string buffers; insure the buffer ends with a space so that if an end
/// marker is at the end of a string ParseMarker won't crash (TCHAR(0) won't help at the end
/// of the buffer here because _istspace which is called from IsWhiteSpace() only recognizes
/// 0x09 ?0x0D or 0x20 as whitespace for most locales.
///////////////////////////////////////////////////////////////////////////////
int CAdapt_ItDoc::ParseMarker(wxChar *pChar)
{
    // whm Note: Caution: This function will fail unless the marker pChar points at is
    // followed by whitespace of some sort - a potential crash problem if ParseMarker is
    // used for parsing markers in local string buffers; insure the buffer ends with a
    // space so that if an end marker is at the end of a string ParseMarker won't crash
    // (TCHAR(0) won't help at the end of the buffer here because _istspace which is called
    // from IsWhiteSpace() only recognizes 0x09 ?0x0D or 0x20 as whitespace for most
    // locales.
    // whm modified 24Nov07 added the test to end the while loop if *ptr points to a null
    // char. Otherwise in the wx version a buffer containing "\fe" could end up with a
    // length of something like 115 characters, with an embedded null char after the third
    // character in the string. This would foul up subsequent comparisons and Length()
    // checks on the string, resulting in tests such as if (mkrStr == _T("\fe")) failing
    // even though mkrStr would appear to contain the simple string "\fe".
    // I still consider ParseMarker as designed to be dangerous and think it appropriate to
    // TODO: add a wxChar* pEnd parameter so that tests for the end of the buffer can be
    // made to prevent any such problems. The addition of the test for null seems to work
    // for the time being.
	int len = 0;
	wxChar* ptr = pChar; // was wchar_t
	wxChar* pBegin = ptr;
	while (!IsWhiteSpace(ptr) && *ptr != _T('\0')) // whm added test for *ptr != _T('\0') 24Nov07
	{
		if (ptr != pBegin && *ptr == gSFescapechar) // whm ammended 7June06 to halt if another marker is
													// encountered before whitespace
			break; 
		ptr++;
		len++;
		if (*(ptr -1) == _T('*')) // whm ammended 17May06 to halt after asterisk (end marker)
			break;
	}
	return len;
}

///////////////////////////////////////////////////////////////////////////////
/// \return		a wxString representing the marker being pointe to by pChar
/// \param		pChar		-> a pointer to the first character to be examined (a backslash)
/// \param		pEnd		-> a pointer to the end of the buffer
/// \remarks
/// Called from: the Doc's GetMarkersAndTextFromString().
/// Returns the whole marker by parsing through an existing marker until either whitespace is
/// encountered or another backslash is encountered.
///////////////////////////////////////////////////////////////////////////////
wxString CAdapt_ItDoc::MarkerAtBufPtr(wxChar *pChar, wxChar *pEnd) // whm added 18Feb05
{
	int len = 0;
	wxChar* ptr = pChar;
	while (ptr < pEnd && !IsWhiteSpace(ptr) && *ptr != _T('/'))
	{
		ptr++;
		len++;
	}
	return wxString(pChar,len);
}

///////////////////////////////////////////////////////////////////////////////
/// \return		TRUE if pChar is pointing to an opening quote mark
/// \param		pChar		-> a pointer to the character to be examined
/// \remarks
/// Called from: the Doc's ParseWord(), the View's DetachedNonQuotePunctuationFollows().
/// Determines is the character being examined is some sort of opening quote mark. An
/// opening quote mark may be a left angle wedge <, a Unicode opening quote char L'\x201C'
/// or L'\x2018', or an ordinary quote or double quote or char 145 or 147 in the ANSI set.
/// Assumes that " is defined as m_bDoubleQuoteAsPunct in the App and/or that ' is defined
/// as m_bSingleQuoteAsPunct in the App.
///////////////////////////////////////////////////////////////////////////////
bool CAdapt_ItDoc::IsOpeningQuote(wxChar* pChar)
{
	// next three functions added by BEW on 17 March 2005 for support of
	// more clever parsing of sequences of quotes with delimiting space between
	// -- these are to be used in a new version of ParseWord(), which will then
	// enable the final couple of hundred lines of code in TokenizeText() to be
	// removed
	// include legacy '<' as in SFM standard, as well as smart quotes
	// and normal double-quote, and optional single-quote
	if (*pChar == _T('<')) return TRUE; // left wedge
#ifdef _UNICODE
	if (*pChar == L'\x201C') return TRUE; // unicode Left Double Quotation Mark
	if (*pChar == L'\x2018') return TRUE; // unicode Left Single Quotation Mark
#else // ANSI version
	if ((unsigned char)*pChar == 147) return TRUE; // Left Double Quotation Mark
	if ((unsigned char)*pChar == 145) return TRUE; // Left Single Quotation Mark
#endif
	if (gpApp->m_bDoubleQuoteAsPunct)
	{
		if (*pChar == _T('\"')) return TRUE; // ordinary double quote
	}
	if (gpApp->m_bSingleQuoteAsPunct)
	{
		if (*pChar == _T('\'')) return TRUE; // ordinary single quote
	}
	return FALSE;
}

///////////////////////////////////////////////////////////////////////////////
/// \return		TRUE if pChar is pointing to a " or to a ' (apostrophe) quote mark
/// \param		pChar		-> a pointer to the character to be examined
/// \remarks
/// Called from: the Doc's ParseWord().
/// Assumes that " is defined as m_bDoubleQuoteAsPunct in the App and/or that ' is defined
/// as m_bSingleQuoteAsPunct in the App.
///////////////////////////////////////////////////////////////////////////////
bool CAdapt_ItDoc::IsAmbiguousQuote(wxChar* pChar)
{
	if (gpApp->m_bDoubleQuoteAsPunct)
	{
		if (*pChar == _T('\"')) return TRUE; // ordinary double quote
	}
	if (gpApp->m_bSingleQuoteAsPunct)
	{
		if (*pChar == _T('\'')) return TRUE; // ordinary single quote (ie. apostrophe)
	}
	return FALSE;
}

///////////////////////////////////////////////////////////////////////////////
/// \return		TRUE if pChar is pointing to a closing quote mark
/// \param		pChar		-> a pointer to the character to be examined
/// \remarks
/// Called from: the Doc's ParseWord().
/// Determines is the character being examined is some sort of closing quote mark.
/// An closing quote mark may be a right angle wedge >, a Unicode closing quote char L'\x201D'
/// or L'\x2019', or an ordinary quote or double quote or char 146 or 148 in the ANSI set.
/// Assumes that " is defined as m_bDoubleQuoteAsPunct in the App and/or that ' is defined
/// as m_bSingleQuoteAsPunct in the App.
///////////////////////////////////////////////////////////////////////////////
bool CAdapt_ItDoc::IsClosingQuote(wxChar* pChar)
{
	// include legacy '>' as in SFM standard, as well as smart quotes
	// and normal double-quote, and optional single-quote
	if (*pChar == _T('>')) return TRUE; // right wedge
#ifdef _UNICODE
	if (*pChar == L'\x201D') return TRUE; // unicode Right Double Quotation Mark
	if (*pChar == L'\x2019') return TRUE; // unicode Right Single Quotation Mark
#else // ANSI version
	if ((unsigned char)*pChar == 148) return TRUE; // Right Double Quotation Mark
	if ((unsigned char)*pChar == 146) return TRUE; // Right Single Quotation Mark
#endif
	if (gpApp->m_bDoubleQuoteAsPunct)
	{
		if (*pChar == _T('\"')) return TRUE; // ordinary double quote
	}
	if (gpApp->m_bSingleQuoteAsPunct)
	{
		if (*pChar == _T('\'')) return TRUE; // ordinary single quote
	}
	return FALSE;
}

///////////////////////////////////////////////////////////////////////////////
/// \return		the number of characters parsed
/// \param		pChar			-> a pointer to the first character of ordinary text to be parsed
/// \param		precedePunct	<- a wxString returned with accumulated preceding punctuation
/// \param		followPunct		<- a wxString returned with accumulated following punctuation
/// \param		nospacePuncts	-> a wxString which contains the source punctuation set with all 
///									spaces removed to help do the parsing of any punctuation 
///									immediately attached to the word (either before or after)
/// \remarks
/// Called from: the Doc's TokenizeText(), the View's RemovePunctuation(), DoExportSrcOrTgtRTF(),
/// ProcessAndWriteDestinationText().
/// Parses a word of ordinary text, intelligently handling (and accumulating) characters defined
/// as punctuation.
///////////////////////////////////////////////////////////////////////////////
int CAdapt_ItDoc::ParseWord(wxChar *pChar, wxString& precedePunct, wxString& followPunct,
													wxString& nospacePuncts)
// returns number of characters parsed over.
//
// From version 1.4.1 and onwards, we must choose which code we use according to the
// gbSfmOnlyAfterNewlines flag; when TRUE, any standard format marker escape characters
// which do not follow a newline are not assumed to belong to a sfm, and so we treat them
// in such cases as ordinary word-building characters (on the assumption we are dealing
// with a hacked legacy encoding in which the escape character is an alphabetic glyph in
// the font)
// BEW 17 March 2005 -- additions to the signature, and additional functions used...
// Accumulate preceding punctuation into precedPunct, following punctuation into
// followPunt, use the nospacePuncts string which contains the source set with all spaces
// removed to help do the parsing of any punctuation immediately attached to the word
// (either before or after) and IsOpeningQuote() and IsClosingQuote() to parse over any
// preceding or following detached quotation marks (of various kinds, including SFM < or >
// wedges)
{
	int len = 0;
	wxChar* ptr = pChar;
    // first, parse over any preceding punctuation, bearing in mind it may have sequences
    // of single and/or double opening quotation marks with one or more spaces between
    // each. We want to accumulate all such punctuation, and the spaces in-place, into the
    // precedePunct CString. We assume only left quotations and left wedges can be set off
    // by spaces from the actual word and whatever preceding punctuation is on it. We make
    // the same assumption for punctuation following the word - but in that case there
    // should be right wedges or right quotation marks. We'll allow ordinary (vertical)
    // double quotation, and single quotation if the latter is being considered to be
    // punctuation, even though this weakens the integrity of out algorithm - but it would
    // only be compromised if there were sequences of vertical quotes with spaces both at
    // the end of a word and at the start of the next word in the source text data, and
    // this would be highly unlikely to ever occur.
	bool bHasPrecPunct = FALSE;
	bool bHasOpeningQuote = FALSE;

	while (IsOpeningQuote(ptr) || IsWhiteSpace(ptr))
	{
        // this block gets us over all detached preceding quotes and the spaces which
        // detach them; we exit this block either when the word proper has been reached, or
        // with ptr pointing at some non-quote punctuation attached to the start of the
        // word. In the latter case, the next block will parse across any such punctuation
        // until the word proper has been reached.
		if (IsWhiteSpace(ptr))
		{
			precedePunct += _T(' '); // normalize while we are at it
			ptr++;
		}
		else
		{
			bHasOpeningQuote = TRUE; // FALSE is used later to stop regular opening 
                // quote (when initial in a following word) from being interpretted as
                // belonging to the current sourcephrase in the circumstance where there is
                // detached non-quote punctuation being spanned in this current block. That
                // is, we want "... word1 ! "word2" word3 ..." to be handled that way,
                // instead of being parsed as "... word1 ! " word2" word3 ..." for example
			precedePunct += *ptr++;
		}
		len++;
	}
	int nFound = -1;
	while (!IsEnd(ptr) && (nFound = nospacePuncts.Find(*ptr)) >= 0)
	{
        // the test checks to see if the character at the location of ptr belongs to the
        // set of source language punctuation characters (with space excluded from the
        // latter) - as long as the nFound value is positive we are parsing over
        // punctuation characters
		precedePunct += *ptr++;
		len++;
	}
	if (precedePunct.Length() > 0)
		bHasPrecPunct = TRUE;
	wxChar* pWordProper = ptr; // where the first character of the word starts
    // we've come to the word proper. We have to parse over it too, but be careful of the
    // fact that punctuation might be within it (eg. boy's) - so we parse to a space or
    // other determinate indicator of the end of the word, and then accumulate final
    // punctuation both preceding that space and following it - provided the latter is
    // right quotation marks or a right wedge (and we'll assume that ordinary vertical
    // double quote or apostrophe goes with the word which precedes, so long as there was
    // preceding punctuation found - otherwise we'll assume it belongs with the next word
    // to be parsed) We also don't card if there is a gFSescapechar in the next section -
    // we can assume it is being used as a word building character quite safely, because we
    // don't have to consider the possibility of such a character being the start of a
    // following (U)SFM until after the next white space character has been parsed over.

    // BEW changed 10Apr06, to remove the "&& *ptr != gSFescapechar" from the while's test,
    // and to put it instead in the code block with TRUE and FALSE code blocks, so as to
    // properly handle parsing across a backslash when the gbSfmOnlyAfterNewlines flag is
    // TRUE
	wxChar* pPunctStart = 0;
	wxChar* pPunctEnd = 0;
	bool bStarted = FALSE;
	while (!IsEnd(ptr) && !IsWhiteSpace(ptr))
	{
        // BEW added 25May06; detecting a SF marker immediately following final punctuation
        // would cause return to the caller from within the loop, without the followPunct
        // CString having any chance to get final punctuation characters put in it. So now
        // we have to detect when final punctuation commences, set pPunctStart there, and
        // set pPunctEnd to where it ends, so that if we have to return to the caller
        // early, we can check for these pointers being different and copy what lies
        // between them into followPunct, so that the caller can properly remove the
        // following punctuation and set up m_key correctly. (Detached punctuation will not
        // break this algorithm because it will already have been put into precedePunct)
		if ((nFound = nospacePuncts.Find(*ptr)) >= 0)
		{
			// we found a (following) punctuation character
			if (bStarted)
			{
				// we've already found at least one, so set pPunctEnd to the current 
				// location
				pPunctEnd = ptr + 1;
			}
			else
			{
				// we've not found one yet, so set both pointers to this location & 
				// turn on the flag
				bStarted = TRUE;
				pPunctStart = ptr;
				pPunctEnd = ptr + 1;
			}
		}
		else
		{
            // we did not find (following) punctuation at this location - what we do here
            // depends on whether we've already found at least one such, or not; it could
            // be a SF escape char here, so we must leave bTurnedON TRUE,
			if (bStarted)
			{
                // we have found one earlier, so we must set the ending pointer here (tests
                // below will determine whether this section is word-internal and to be
                // ignored, or actually extends to the location at which word parsing ends
                // - in which case we don't want to ignore it)
				pPunctEnd = ptr;
			}
			else
			{
                // we've not started spanning (following) punctuation yet, so update both
                // pointers to this location (BEW 23Feb07 added +1; this block is not very
                // important as these values get overridden, but adding +1 makes the value
                // correct because ptr here is pointing at a non-punctuation character and
                // if there is a punctuation character it cannot be at ptr, it may or may
                // not be at ptr + 1, and the iteration of the parse will determine that or
                // not)
				pPunctStart = ptr + 1;
				pPunctEnd = ptr + 1;
			}
		}

        // advance over the next character, or if the user wants USFM fixed space !$
        // sequence retained as a conjoiner, then check for it and advance instead by two
        // if such a sequence is at ptr; but if the gbSfmOnlyAfterNewlines flag is TRUE and
        // we are pointing at a backslash, then parse over it too (ie. don't interpret it
        // as the beginning of a valid SFM)
		if (*ptr != gSFescapechar)
		{
			// we are not pointing at a backslash...

			if (!gpApp->m_bChangeFixedSpaceToRegularSpace && wxStrncmp(ptr,_T("!$"),2) == 0)
			{
				ptr += 2;
				len += 2;
			}
			else
			{
				ptr++;
				len++;
			}

            // if we are started and not pointing at white space either, then turn off and
            // reset the pointers for a following punctuation span
			if (bStarted && !IsWhiteSpace(ptr) && (*ptr != gSFescapechar))
			{
				// the punctuation span was word-internal, so we forget about it
				bStarted = FALSE;
				pPunctStart = ptr;
				pPunctEnd = ptr;
			}
		}
		else
		{
            // we are pointing at a backslash, so either we have come to a SFM and parsing
            // of the word must halt, or it is a backslash which is to not be interpretted
            // as the onset of an SFM - the gbSfmOnlyAfterNewlines flag tells us which is
            // the case except when a word with backslash as its first character happens
            // (accidently) to be at the start of a line (we'll not check for when it
            // accidently may start the file - that's too unlikely to bother about), so for
            // such a possibility we'll do a USFMAnalysis lookup and if we detect one of
            // the markers, we'll assume that it's a valid SFM and halt parsing, if not a
            // known marker we'll let the gbSfmOnlyAfterNewlines flag decide.
			if (gbSfmOnlyAfterNewlines)
			{
				// the flag is on, so parse over the backslash provided it and what follows
				// is not identified as a character string identical to a known SFM
				USFMAnalysis* pUsfmAnalysis = LookupSFM(ptr);
				if (pUsfmAnalysis != NULL)
				{
					// it's a known marker, so halt right here
					goto m; // a little further down
				}
				else
				{
					// it's not a known marker, so assume its a word-building character & 
					// keep parsing

                    // if we are turned on and not pointing at white space either, then
                    // turn off and reset the pointers for a following punctuation span
					if (bStarted)
					{
						// the punctuation span was word-internal, so we forget about it
						bStarted = FALSE;
						pPunctStart = ptr;
						pPunctEnd = ptr;
					}
					// move on & iterate
					ptr++;
					len++;
				}
			}
			else
			{
				// the flag is off, so every backslash is to be interpretted as an SFM,
				// so halt parsing early, that is, right here; and return the len value....
				// But before we return, if detection of (following) punctuation was turned
				// on and the end of it is at the current location (ie. preceding SF escape
				// character), then we have to put the final punctuation into followPunct
				// so the caller can do what it has to do with word-final punctuation
				// whm 26Feb10. Removed the (wxUint32) casts from the pPunctEnd and pPunctStart
				// pointers in the if test and assignment of numChars below. They are not needed
				// for correct pointer math. The math works correctly without dividing by
				// sizeof(wxChar). I verified this by testing on both Unicode and ANSI builds.
				// While enclosing everything in (wxUint32) casts also works, it does NOT
				// work if the app is compiled for 64 bit systems - a fact reported by David 
				// Gardner who successfully compiled the sources for 64-bit AMD64, but only
				// by removing the (wxUint32) casts.
//m:				if (bStarted && ((wxUint32)pPunctEnd - (wxUint32)pPunctStart) > 0 
//					&& pPunctEnd == ptr)
m:				if (bStarted && (pPunctEnd - pPunctStart) > 0 
					&& pPunctEnd == ptr)
				{
					// there is word-final punctuation content to be dealt with
					// BEW modified 23Feb07; when working in Unicode, UTF-16 characters are
					// two bytes long, so setting numChars to the pointer difference will
					// double the correct value; we have to therefore divide by sizeof(wxChar)
					// to get numChars right in regular and unicode apps
					// whm 26Feb10 see note above the if statement.
					//int numChars = (int)((wxUint32)pPunctEnd - (wxUint32)pPunctStart) / 
					//						(wxUint32)sizeof(wxChar);
					int numChars = (int)(pPunctEnd - pPunctStart); // / sizeof(wxChar);
					wxString finals(pPunctStart,numChars);
					followPunct = finals;
				}
				return len;
			}
		}
	}
    // now, work backwards first - we may have stopped at a space and there could have been
    // several punctuation characters parsed over by the previous while loop; we don't have
    // to count these ones, so long as followPunct ends up containing all following
    // punctuation
	wxChar* pBack = ptr;
	do {
		--pBack; // point to the previous character
		if (pBack < pWordProper) break; // ptr did not advance in the previous while block, 
										// so break out
		if (IsClosingQuote(pBack) || (nFound = nospacePuncts.Find(*pBack)) >= 0)
		{
			// it is a punctuation character - either one of the closing quote ones or
			// or apostrophe is being treated as punctuation and it is an apostrophe; OR
			// it is one of the spaceless source language set
			wxString s = *pBack;
			followPunct = s + followPunct; // accumulate in text order
		}
	} while ( pBack > pWordProper && (IsClosingQuote(pBack) || nFound >= 0));
    // now parse forward from the location where we started parsing backwards - it's from
    // here on we have to be careful to allow for the possibility that the gSFescapechar
    // might or might not be an indicator of a new standard format marker being in the
    // source text stream; and we have to continue counting the characters we successfully
    // parse over. We parse over a small chunk until we determine we must halt. We don't
    // commit to the contents of the small chunk until we are sure we have parsed over at
    // least one genuine detached closing quote.

    // BEW note added on 10Apr06, the comment that it is here that gSFescapechar is
    // relevant is not correct; if ptr is pointing at a backslash, the stuff below does not
    // allow it to be parsed over, but only treated as an SFM onset - so I have to add the
    // checking for ignoring backslashes when gbSfmOnlyAfterNewlines is TRUE be done in the
    // loop above! The code below is therefore a bit more convoluted than it need be, but
    // I'll leave the sleeping dog to lie.

	if (IsEnd(ptr))
		return len; // we are at the end of the source data, so can't parse further
	wxString smchunk;
	smchunk.Empty();
	int nChunkLen = 0;
	bool bFoundDetachedRightQuote = FALSE;
	if (gbSfmOnlyAfterNewlines)
	{
        // treat the escape character as if it is an alphabetic word-building character and
        // so don't test for it as a loop ending criterion
		if (!IsEnd(ptr))
		{
			goto c;
		}
		else
		{
			goto d;
		}
	}
	else
	{
        // treat the escape character as indicating the presence of a (U)SFM, so test for
        // it as a loop ending criterion
		wxChar* ptr2;
		wxChar* ptr3;
a:		if (!IsEnd(ptr) && *ptr != gSFescapechar)
		{
c:			if	(IsWhiteSpace(ptr))
			{
				smchunk += _T(' '); // we may as well normalize to space 
									// while we are at it
				ptr++; // accumulate it and advance pointer then iterate
				goto a;
			}
			else
			{	
				// it's not white space, so what is it?
				if (IsClosingQuote(ptr))
				{
                    // it's one of the closing quote characters (but " or ' are ambiguous,
                    // so test further because " or ' might be preceding punctuation on a
                    // following word not yet parsed)
					if (IsAmbiguousQuote(ptr))
					{
                        // it's one of the two ambiguous ones; we'll assume this does not
                        // belong with our word if there was no preceding punctuation, or
                        // if there was preceding punctuation but we have already found at
                        // least one closing curly quote, or if bHasOpeningQuote is FALSE,
                        // otherwise we'll accept it as a detached following quote mark
						if (bHasPrecPunct)
						{
							if (!bHasOpeningQuote)
							{
                                // there was no opening quote on this word, but the word
                                // may be the end of a quoted section and so we must test
                                // further
								if (bFoundDetachedRightQuote)
								{
                                    // a detached right quote was found earlier, so we
                                    // should stop the iteration right here, and not
                                    // accumulate the non-curly quote symbol because it is
                                    // unlikely it would associate to the left
									goto g;
								}
								else
								{
                                    // we only know where was opening punctuation and no
                                    // detached closing quote has yet been found, so we
                                    // need to apply the tests in the next block to decide
                                    // what to do with the ambiguous quote at ptr; OR
                                    // control got directed here from the bHasOpeningQuote
                                    // == FALSE block and we've not found a detached right
                                    // quote earlier, so we must make our final decision
                                    // based on the tests in the block below
									goto e;
								}
							}
							else // next block is where the 'final' decisions will be 
                                 // made (only one option iterates from within the next
                                 // battery of tests)
							{
                                // there was an opening quote on this word, or control was
                                // directed here from the block immediately above; so this
                                // ambiguous quote may be a closing one, or it could belong
                                // to the next word - so we must test further
e:								ptr2 = ptr;
								ptr2++; // point at the next character
								if (IsWhiteSpace(ptr2))
								{
                                    // *ptr is bracketed by white space either side, so it
                                    // could associate either to the left or two the right
                                    // - so we must make some assumptions: we assume it is
                                    // detached quote for the current (ie. to the left)
                                    // word if the next character past ptr2 is not
                                    // punctuation (if it's a white space we jump it and
                                    // test again), if it is punctuation we assume the
                                    // quote at ptr associates to the right, and if the
                                    // character at ptr2 is not white space we assume we
                                    // have moved into the preceding punctuation of a
                                    // following word and so associate the quote at ptr
                                    // rightwards
									ptr2++; // point beyond the white space

									// skip over any additional white spaces
									while (IsWhiteSpace(ptr2)) {ptr2++;} 

									// find out what the first non-whitespace character is
									if (nospacePuncts.Find(*ptr2) == -1)
									{
                                        // the character at ptr2 is not punctuation, so we
                                        // will assume the character at ptr associates to
                                        // the left; if ptr2 is actually at the end of the
                                        // data (eg, when rebuilding a sourcephrase for
                                        // document rebuild) then this is also accomodated
                                        // by the same decision
f:										bFoundDetachedRightQuote = TRUE;
										smchunk += *ptr++; // accumulate it, advance pointer
										goto a; // and iterate
									}
									else
									{
                                        // it is punctuation at ptr2, so we could have a
                                        // series of detached quotes which associate left,
                                        // or a series which associates right. To
                                        // distinguish these we will favour rightmost
                                        // association if there is a next word with initial
                                        // punctuation; otherwise we'll assume we should
                                        // associate leftwards
										ptr3 = ptr2;
										ptr3++; // point at next char (it could be space, etc)
										if (IsEnd(ptr3)) goto f; // associate leftwards & iterate
										if (IsWhiteSpace(ptr3))
										{
											while (IsWhiteSpace(ptr3)) {ptr3++;} // skip any others
											if (IsEnd(ptr3)) goto f;
											if (nospacePuncts.Find(*ptr3) == -1)
											{
												// it's not punctuation
												goto f; // iterate
											}
											else
											{
                                                // it's punctuation; so we'll limit the
                                                // nesting of tests to a max of two
                                                // detached ambiguous quotes, so we will
                                                // here examine what follows - if it is a
                                                // space or the end of the data we will
                                                // assume it is the last of detached
                                                // punctuation associating to the left;
                                                // anything else, we'll have the quote at
                                                // ptr associated rightwards
												ptr3++;
												if (IsEnd(ptr3) || IsWhiteSpace(ptr3))
													goto f; // iterate
											}
										}
										// bale out (ie. associate right)
g:										followPunct += smchunk;
										nChunkLen = smchunk.Length();
										len += nChunkLen;
										return len;
									}
								}
								else
								{
                                    // it was not whitespace, so we assume the quote
                                    // character at ptr must associate to the right, so
                                    // bale out; however, if we are at the end of the text
                                    // (eg. when doing document rebuild) then associating
                                    // rightwards is impossible and we then associate to
                                    // the left
									if (IsEnd(ptr2))
									{
                                        // associate it to the left, that is, it is part of
                                        // the currently being parsed word
										bFoundDetachedRightQuote = TRUE;
										smchunk += *ptr++; // accumulate it, advance pointer
										goto a; // and iterate
									}
									// if not at the end, then assume it belongs to the 
									// next word
									goto g;
								}
							} // end of the "final battery of tests" block
						}
						else // bHasPredPunct was FALSE
						{
                            // there was no opening punctuation on our parsed word, but the
                            // ambiguous quote at ptr could still be a closing quote, or it
                            // could be a quote belonging to the next word - so additional
                            // tests are required
							goto e;
						}
					}
					else
					{
                        // it's a genuine curly closing quote or a right wedge, either way
                        // this is detached punctuation belonging to the previous word, so
                        // we must accumulate it & iterate
						bFoundDetachedRightQuote = TRUE;
						smchunk += *ptr++; // accumulate it, and advance to the 
										   // next character
						goto a; // iterate
					}
				}
				else
				{
					// it's not one of the possible closing quotes, so we have to stop 
					// iterating
b:					wxString spaceless = smchunk;
					while (spaceless.Find(_T(' ')) != -1)
					{
						spaceless.Remove(spaceless.Find(_T(' ')),1);
					}
					if (smchunk.Length() > 0 && bFoundDetachedRightQuote)
					{
						// there is something to accumulate
						followPunct += smchunk;
						nChunkLen = smchunk.Length();
						len += nChunkLen;
						return len;
					}
					else
					{
						// there is nothing worth accumulating
						return len;
					}
				}
			}
		}
		else
		{
			// we are at the end or at the start of a (U)SFM, so we cannot iterate further
d:			goto b;
		}	
	}
}

///////////////////////////////////////////////////////////////////////////////
/// \return		nothing
///	\param		useSfmSet	->	which of the three sfm set possibilities we are dealing with
/// \param		filterMkrs	->	concatenated markers (each with a following space) which are 
///								the markers formerly unfiltered but now designated by the user 
///								as to be filtered,
/// \param		unfilterMkrs ->	concatenated markers (each with a following space) which are 
///								the markers formerly filtered but now designated by the user 
///								as to be unfiltered.
/// \remarks
/// Called from: the App's DoUsfmFilterChanges(), the Doc's RestoreDocParamsOnInput(),
/// ReconstituteAfterFilteringChange(), RetokenizeText().
/// This is an overloaded version of another function called ResetUSFMFilterStructs.
/// Changes only the USFMAnalysis structs which deal with the markers in the filterMkrs
/// string and the unfilterMkrs string.
///////////////////////////////////////////////////////////////////////////////
void CAdapt_ItDoc::ResetUSFMFilterStructs(enum SfmSet useSfmSet, wxString filterMkrs, 
										  wxString unfilterMkrs)
{
    // BEW added 25May05 in support of changing filtering settings for USFM, SFM or
    // combined Filtering set The second and third strings must have been set up in the
    // caller by iterating through the map m_FilterStatusMap, which contains associations
    // between the bare marker as key (ie. no backslash or final *) and a literal string
    // which is "1" when the marker is unfiltered and about to be filtered, and "0" when it
    // is filtered and about to be unfiltered. This map is constructed when the user exits
    // the Filter tab of the Preferences or Start Working... wizard.
	MapSfmToUSFMAnalysisStruct* pSfmMap;
	USFMAnalysis* pSfm;
	wxString fullMkr;

	pSfmMap = gpApp->GetCurSfmMap(useSfmSet);

	MapSfmToUSFMAnalysisStruct::iterator iter;
    // enumerate through all markers in pSfmMap and set those markers that occur in the
    // filterMkrs string to the equivalent of filter="1" and those in unfilterMkrs to the
    // equivalent of filter="0"; doing this means that any call of TokenizeText() (or
    // functions which call it such as TokenizeTextString() etc) will, when they get to the
    // LookupSFM(marker) call, get the USFMAnalysis with the filtering settings which need
    // to be in place at the time the lookup is done

	for( iter = pSfmMap->begin(); iter != pSfmMap->end(); ++iter )
	{
		wxString key = iter->first; // use only for inspection
		pSfm = iter->second;
		fullMkr = gSFescapechar + pSfm->marker + _T(' '); // each marker in filterMkrs is 
														  // delimited by a space
		if (filterMkrs.Find(fullMkr) != -1)
		{
			pSfm->filter = TRUE;
            // because of how the caller constructs filterMkrs and unfilterMkrs, it is
            // never possible that a marker will be in both these strings, so if we do this
            // block we can skip the next
			continue; 
		}
		if (unfilterMkrs.Find(fullMkr) != -1)
		{
			pSfm->filter = FALSE;
		}
	}
	// redo the special fast access marker strings to reflect any changes to pSfm->filter
	// attributes 
	gpApp->SetupMarkerStrings();
}

///////////////////////////////////////////////////////////////////////////////
/// \return		nothing
///	\param		useSfmSet	->	which of the three sfm set possibilities we are dealing with
/// \param		filterMkrs	->	concatenated markers (each with a following space) which are 
///								the markers formerly unfiltered but now designated by the user 
///								as to be filtered,
/// \param		resetMkrs	-> an enum indicating whether to reset allInSet or onlyThoseInString
/// \remarks
/// Called from: the Doc's RestoreDocParamsOnInput().
/// The filterMkrs parameter is a wxString of concatenated markers delimited by following spaces. 
/// If resetMkrs == allInSet, ResetUSFMFilterStructs() sets the filter attributes of the 
/// appropriate SfmSet of markers to filter="1" if the marker is present in the 
/// filterMkrs string, and for all others the filter attribute is set to filter="0" 
/// if it is not already zero.
/// If resetMkrs == onlyThoseInString ResetUSFMFilterStructs() sets the filter attributes
/// of the appropriate SfmSet of markers to filter="1" of only those markers which
/// are present in the filterMkrs string.
/// ResetUSFMFilterStructs does nothing to the USFMAnalysis structs nor their 
/// maps in response to the presence of unknown markers (filtered or not), since unknown markers
/// do not have any identifiable attributes, except for being considered userCanSetFilter
/// as far as the filterPage is concerned.
///////////////////////////////////////////////////////////////////////////////
void CAdapt_ItDoc::ResetUSFMFilterStructs(enum SfmSet useSfmSet, wxString filterMkrs, 
										  enum resetMarkers resetMkrs)
{
	// whm added 5Mar2005 in support of USFM and SFM Filtering support
	MapSfmToUSFMAnalysisStruct* pSfmMap; 
	USFMAnalysis* pSfm;
	wxString key;
	wxString fullMkr;

	pSfmMap = gpApp->GetCurSfmMap(useSfmSet);

	MapSfmToUSFMAnalysisStruct::iterator iter;
	// enumerate through all markers in pSfmMap and set those markers that
	// occur in the filterMkrs string to filter="1" and, if resetMkrs is allInSet,
	// we also set those that don't occur in filterMkrs to filter="0"

	for( iter = pSfmMap->begin(); iter != pSfmMap->end(); ++iter )
	{
		wxString key = iter->first; // use only for inspection
		pSfm = iter->second;
		fullMkr = gSFescapechar + pSfm->marker + _T(' '); // each marker in filterMkrs 
														  // is delimited by a space
		if (filterMkrs.Find(fullMkr) != -1)
		{
			pSfm->filter = TRUE;
		}
		else if (resetMkrs == allInSet)
		{
			pSfm->filter = FALSE;
		}
	}
	// The m_filterFlagsUnkMkrs flags are already changed in the filterPage
	// so they should not be changed (reversed) here

    // redo the special fast access marker strings to reflect any changes to pSfm->filter
    // attributes or the presence of unknown markers
	gpApp->SetupMarkerStrings();
}


///////////////////////////////////////////////////////////////////////////////
/// \return		the whole standard format marker including the initial backslash and any ending *
/// \param		pChar			-> a pointer to the first character of the marker (a backslash)
/// \remarks
/// Called from: the Doc's GetMarkerWithoutBackslash(), IsInLineMarker(), IsCorresEndMarker(),
/// TokenizeText(), the View's RebuildSourceText(), FormatMarkerBufferForOutput(), 
/// DoExportInterlinearRTF(), ParseFootnote(), ParseEndnote(), ParseCrossRef(), 
/// ProcessAndWriteDestinationText(), ApplyOutputFilterToText(), IsCharacterFormatMarker(),
/// DetermineRTFDestinationMarkerFlagsFromBuffer().
/// Returns the whole standard format marker including the initial backslash and any ending asterisk.
///////////////////////////////////////////////////////////////////////////////
wxString CAdapt_ItDoc::GetWholeMarker(wxChar *pChar)
{
	// whm added 10Feb2005 in support of USFM and SFM Filtering support
	// returns the whole marker including backslash and any ending *
	wxChar* ptr = pChar;
	int itemLen = ParseMarker(ptr);
	return wxString(ptr,itemLen);
}

///////////////////////////////////////////////////////////////////////////////
/// \return		the whole standard format marker including the initial backslash 
///             and any ending *
/// \param		str	-> a wxString in which the initial backslash of the marker to be
///					   obtained is at the beginning of the string
/// \remarks
/// Called from: the View's RebuildSourceText().
/// Returns the whole standard format marker including the initial backslash and any ending
/// asterisk. Internally uses ParseMarker() just like the version of GetWholeMarker() that
/// uses a pointer to a buffer.
///////////////////////////////////////////////////////////////////////////////
wxString CAdapt_ItDoc::GetWholeMarker(wxString str)
{
	// BEW added 2Jun2006 for situations where a marker is at the start of a CString
	// returns the whole marker including backslash and any ending *
	int len = str.Length();
    // wx version note: Since we require a read-only buffer we use GetData which just
    // returns a const wxChar* to the data in the string.
	const wxChar* pChar = str.GetData();
	wxChar* pEnd;
	pEnd = (wxChar*)pChar + len;
	wxChar* pBufStart = (wxChar*)pChar;
	wxASSERT(*pEnd == _T('\0'));
	int itemLen = ParseMarker(pBufStart);
	wxString mkr = wxString(pChar,itemLen);
	return mkr;
}

///////////////////////////////////////////////////////////////////////////////
/// \return		the standard format marker without the initial backslash, but including 
///             any ending *
/// \param		pChar	-> a pointer to the first character of the marker (a backslash)
/// \remarks
/// Called from: the Doc's IsPreviousTextTypeWanted(), GetBareMarkerForLookup(), 
/// IsEndMarkerForTextTypeNone(), the View's InsertNullSourcePhrase().
/// Returns the standard format marker without the initial backslash, but includes any end
/// marker asterisk. Internally calls GetWholeMarker().
///////////////////////////////////////////////////////////////////////////////
wxString CAdapt_ItDoc::GetMarkerWithoutBackslash(wxChar *pChar)
{
	// whm added 10Feb2005 in support of USFM and SFM Filtering support
	// Strips off initial backslash but leaves any final asterisk in place.
	// The bare marker string returned is suitable for marker lookup only if
	// it is known that no asterisk is present; if unsure, call
	// GetBareMarkerForLookup() instead.
	wxChar* ptr = pChar;
	wxString Mkr = GetWholeMarker(ptr);
	return Mkr.Mid(1); // strip off initial backslash
}


///////////////////////////////////////////////////////////////////////////////
/// \return		the standard format marker without the initial backslash and 
///             without any ending *
/// \param		pChar	-> a pointer to the first character of the marker in the 
///                        buffer (a backslash)
/// \remarks
/// Called from: the Doc's IsPreviousTextTypeWanted(), ParseFilteringSFM(), LookupSFM(),
/// AnalyseMarker(), IsEndMarkerForTextTypeNone(), the View's InsertNullSourcePhrase(),
/// DoExportInterlinearRTF(), IsFreeTranslationEndDueToMarker(), HaltCurrentCollection(),
/// ParseFootnote(), ParseEndnote(), ParseCrossRef(), ProcessAndWriteDestinationText(),
/// ApplyOutputFilterToText().
/// Returns the standard format marker without the initial backslash, and without any end
/// marker asterisk. Internally calls GetMarkerWithoutBackslash().
///////////////////////////////////////////////////////////////////////////////
wxString CAdapt_ItDoc::GetBareMarkerForLookup(wxChar *pChar)
{
	// whm added 10Feb2005 in support of USFM and SFM Filtering support
    // Strips off initial backslash and any ending asterisk.
    // The bare marker string returned is suitable for marker lookup.
    wxChar* ptr = pChar;
    wxString bareMkr = GetMarkerWithoutBackslash(ptr);
    int posn = bareMkr.Find(_T('*'));
    // The following GetLength() call could on rare occassions return a 
	// length of 1051 when processing the \add* marker.
	// whm comment: the reason for the erroneous result from GetLength
	// stems from the problem with the original code used in ParseMarker.
	// (see caution statement in ParseMarker). 
    if (posn >= 0) // whm revised 7Jun05
        // strip off asterisk for attribute lookup
		bareMkr = bareMkr.Mid(0,posn);
    return bareMkr;
}

///////////////////////////////////////////////////////////////////////////////
/// \return		nothing
/// \param		pMkrList	<- a wxArrayString that holds standard format markers and 
///								associated parsed from the input string str
/// \param		str			-> the string containing standard format markers and associated text 
/// \remarks
/// Called from: the Doc's GetUnknownMarkersFromDoc(), the View's GetMarkerInventoryFromCurrentDoc(),
/// CPlaceInternalMarkers::InitDialog(), CTransferMarkersDlg::InitDialog(), and
/// CViewFilteredMaterialDlg::InitDialog().
/// Scans str and collects all standard format markers and their associated text into 
/// pMkrList, one marker and associated text per array item.
///////////////////////////////////////////////////////////////////////////////
void CAdapt_ItDoc::GetMarkersAndTextFromString(wxArrayString* pMkrList, 
											   wxString str) // whm added 18Feb05
{
	// Populates a wxArrayString containing sfms and their associated
	// text parsed from the input str. pMkrList will contain one list item for
	// each marker and associated text found in str in order from beginning of
	// str to end.
	int nLen = str.Length();
    // wx version note: Since we require a read-only buffer we use GetData which just
    // returns a const wxChar* to the data in the string.
	const wxChar* pBuf = str.GetData();
	wxChar* pEnd = (wxChar*)pBuf + nLen; // cast necessary because pBuf is const
	wxASSERT(*pEnd == _T('\0')); // whm added 18Jun06
	wxChar* ptr = (wxChar*)pBuf;
	wxChar* pBufStart = (wxChar*)pBuf; // cast necessary because pBuf is const
	wxString accumStr = _T("");
	// caller needs to call Clear() to start with empty list
	while (ptr < pEnd)
	{
		if (IsFilteredBracketMarker(ptr,pEnd))
		{
			// It's a filtered marker opening bracket. There should always
			// be a corresponding closing bracket, so parse and accumulate
			// chars until end of filterMkrEnd.
			while (ptr < pEnd && !IsFilteredBracketEndMarker(ptr,pEnd))
			{
				accumStr += *ptr;
				ptr++;
			}
			if (ptr < pEnd)
			{
				// accumulate the filterMkrEnd
				for (int i = 0; i < (int)wxStrlen_(filterMkrEnd); i++)
				{
					accumStr += *ptr;
					ptr++;
				}
			}
			accumStr.Trim(FALSE); // trim left end
			accumStr.Trim(TRUE); // trim right end
			// add the filter sfm and associated text to list
			pMkrList->Add(accumStr);
			accumStr.Empty();
		}
		else if (IsMarker(ptr,pBufStart))
		{
			// It's a non-filtered sfm. Non-filtered sfms can be followed by
			// a corresponding markers or no end markers. We'll parse and 
			// accumulate chars until we reach the next marker (or end of buffer).
			// If the marker is a corresponding end marker we'll parse and
			// accumulate it too, otherwise we'll not accumulate it with the
			// current accumStr.
			// First save the marker we are at to check that any end marker
			// that follows is indeed a corresponding end marker.
			wxString currMkr = MarkerAtBufPtr(ptr,pEnd);
			int itemLen;
			while (ptr < pEnd && *(ptr+1) != gSFescapechar)
			{
				accumStr += *ptr;
				ptr++;
			}
			itemLen = ParseWhiteSpace(ptr); // ignore return value
			ptr += itemLen;
			if (itemLen > 0)
				accumStr += _T(' ');
			if (IsEndMarker(ptr,pEnd))
			{
				//parse and accumulate to the * providing it is a corresponding end marker
				if (IsCorresEndMarker(currMkr,ptr,pEnd))
				{
					while (*ptr != _T('*'))
					{
						accumStr += *ptr;
						ptr++;
					}
					accumStr += *ptr; // add the end marker
					ptr++;
				}
			}
			accumStr.Trim(FALSE); // trim left end
			accumStr.Trim(TRUE); // trim right end
			// add the non-filter sfm and associated text to list
			pMkrList->Add(accumStr);
			accumStr.Empty();
		}
		else
			ptr++;
	} // end of while (ptr < pEnd)
	// We've finished building the wxArrayString
}

///////////////////////////////////////////////////////////////////////////////
/// \return     TRUE if there is a filename clash, FALSE if the typed name is unique
/// 
/// \remarks
/// Get the active document folder's document names into the app class's
/// m_acceptedFilesList and test them against the user's typed filename.
/// Use in OutputFilenameDlg.cpp's OnOK()button handler.
/// Before this protection was added in 22July08, an existing document with lots of
/// adaptation and other work contents already done could be wiped out without warning
/// merely by the user creating a new document with the same name as that document file.
///////////////////////////////////////////////////////////////////////////////
bool CAdapt_ItDoc::FilenameClash(wxString& typedName)
{
	gpApp->m_acceptedFilesList.Clear();
	wxString dirPath;
	if (gpApp->m_bBookMode && !gpApp->m_bDisableBookMode)
		dirPath = gpApp->m_bibleBooksFolderPath;
	else
		dirPath = gpApp->m_curAdaptionsPath;
	bool bOK;
	bOK = ::wxSetWorkingDirectory(dirPath); // ignore failures
	wxString docName;
	gpApp->GetPossibleAdaptionDocuments(&gpApp->m_acceptedFilesList, dirPath);
	int offset = -1;

	// remove any .xml or .adt which the user may have added to the passed in filename
	wxString rev = typedName;
	rev = MakeReverse(rev);
	wxString adtExtn = _T(".adt");
	wxString xmlExtn = _T(".xml");
	adtExtn = MakeReverse(adtExtn);
	adtExtn = MakeReverse(adtExtn);
	offset = rev.Find(adtExtn);
	if (offset == 0)
	{
		// it's there, so remove it
		rev = rev.Mid(4);
	}
	offset = rev.Find(xmlExtn);
	if (offset == 0)
	{
		// it's there, so remove it
		rev = rev.Mid(4);
	}
	rev = MakeReverse(rev);
	int len = rev.Length();

	// test for filename clash
	int ct;
	for (ct = 0; ct < (int)gpApp->m_acceptedFilesList.GetCount(); ct++)
	{
		docName = gpApp->m_acceptedFilesList.Item(ct);
		offset = docName.Find(rev);
		if (offset == 0)
		{
			// this one is a candidate for a clash, check further
			int docNameLen = docName.Length();
			if (docNameLen >= len + 1)
			{
				// there is a character at len, so see if it is the . of an extension
				wxChar ch = docName.GetChar(len);
				if (ch == _T('.'))
				{
					// the names clash
					gpApp->m_acceptedFilesList.Clear();
					return TRUE;
				}
			}
			else
			{
				// same length, and the search string lacks .adt or .xml, so this
				// is unlikely to be a clash, but we'll return TRUE and give a
				// beep as well
				::wxBell();
				gpApp->m_acceptedFilesList.Clear();
				return TRUE;
			}
		}
	}
	gpApp->m_acceptedFilesList.Clear();
	return FALSE;
}

///////////////////////////////////////////////////////////////////////////////
/// \return		a pointer to the USFMAnalysis struct associated with the marker at pChar,
///				or NULL if the marker was not found in the MapSfmToUSFMAnalysisStruct.
/// \param		pChar	-> a pointer to the first character of the marker in the 
///                        buffer (a backslash)
/// \remarks
/// Called from: the Doc's ParseWord(), IsMarker(), TokenizeText(), DoMarkerHousekeeping(),
/// IsEndMarkerForTextTypeNone(), the View's InsertNullSourcePhrase(),
/// Determines the marker pointed to at pChar and performs a look up in the 
/// MapSfmToUSFMAnalysisStruct hash map. If the marker has an association in the map it
/// returns a pointer to the USFMAnalysis struct. NULL is returned if no marker could be
/// parsed from pChar, or if the marker could not be found in the hash map.
///////////////////////////////////////////////////////////////////////////////
USFMAnalysis* CAdapt_ItDoc::LookupSFM(wxChar *pChar)
{
	// Looks up the sfm pointed at by pChar
	// Returns a USFMAnalysis struct filled out with attributes
	// if the marker is found in the appropriate map, otherwise
	// returns NULL.
	// whm ammended 11July05 to return the \bt USFM Analysis struct whenever
	// any bare marker of the form bt... exists at pChar
	wxChar* ptr = pChar;
	bool bFound = FALSE;
	// get the bare marker
	wxString bareMkr = GetBareMarkerForLookup(ptr);
	// look up and Retrieve the USFMAnalysis into our local usfmAnalysis struct 
	// variable. 
	// If bareMkr begins with bt... we will simply use bt which will return the
	// USFMAnalysis struct for \bt for all back-translation markers based on \bt...
	if (bareMkr.Find(_T("bt")) == 0)
	{
		// bareMkr starts with bt... so shorten it to simply bt for lookup purposes
		bareMkr = _T("bt");
	}
	MapSfmToUSFMAnalysisStruct::iterator iter;
	// The particular MapSfmToUSFMAnalysisStruct used for lookup below depends the appropriate 
	// sfm set being used as stored in gCurrentSfmSet enum.
	switch (gpApp->gCurrentSfmSet)
	{
		case UsfmOnly: 
			iter = gpApp->m_pUsfmStylesMap->find(bareMkr);
			bFound = (iter != gpApp->m_pUsfmStylesMap->end());
			break;
		case PngOnly: 
			iter = gpApp->m_pPngStylesMap->find(bareMkr);
			bFound = (iter != gpApp->m_pPngStylesMap->end()); 
			break;
		case UsfmAndPng: 
			iter = gpApp->m_pUsfmAndPngStylesMap->find(bareMkr);
			bFound = (iter != gpApp->m_pUsfmAndPngStylesMap->end()); 
			break;
		default: 
			iter = gpApp->m_pUsfmStylesMap->find(bareMkr);
			bFound = (iter != gpApp->m_pUsfmStylesMap->end());
	}
	if (bFound)
	{
		// iter->second points to the USFMAnalysis struct
		return iter->second;
	}
	else
	{
		return (USFMAnalysis*)NULL;
	}
}


///////////////////////////////////////////////////////////////////////////////
/// \return		a pointer to the USFMAnalysis struct associated with the bareMkr,
///				or NULL if the marker was not found in the MapSfmToUSFMAnalysisStruct.
/// \param		bareMkr	-> a wxString containing the bare marker to use in the lookup
/// \remarks
/// Called from: the Doc's ReconstituteAfterFilteringChange(), ParseFilteringSFM(),
/// GetUnknownMarkersFromDoc(), AnalyseMarker(), IsEndingSrcPhrase(), 
/// ContainsMarkerToBeFiltered(), RedoNavigationText(), DoExportInterlinearRTF(),
/// IsFreeTranslationEndDueToMarker(), HaltCurrentCollection(), ParseFootnote(),
/// ParseEndnote(), ParseCrossRef(), ParseMarkerAndAnyAssociatedText(), 
/// GetMarkerInventoryFromCurrentDoc(), MarkerTakesAnEndMarker(), 
/// CViewFilteredMaterialDlg::GetAndShowMarkerDescription().
/// Looks up the bareMkr in the MapSfmToUSFMAnalysisStruct hash map. If the marker has an
/// association in the map it returns a pointer to the USFMAnalysis struct. NULL is
/// returned if the marker could not be found in the hash map.
///////////////////////////////////////////////////////////////////////////////
USFMAnalysis* CAdapt_ItDoc::LookupSFM(wxString bareMkr)
{
	// overloaded version of the LookupSFM above to take bare marker
	// Looks up the bareMkr CString sfm in the appropriate map
	// Returns a USFMAnalysis struct filled out with attributes
	// if the marker is found in the appropriate map, otherwise
	// returns NULL.
	// whm ammended 11July05 to return the \bt USFM Analysis struct whenever
	// any bare marker of the form bt... is passed in
	if (bareMkr.IsEmpty())
		return (USFMAnalysis*)NULL;
	bool bFound = FALSE;
	// look up and Retrieve the USFMAnalysis into our local usfmAnalysis struct 
	// variable. 
	// If bareMkr begins with bt... we will simply use bt which will return the
	// USFMAnalysis struct for \bt for all back-translation markers based on \bt...
	if (bareMkr.Find(_T("bt")) == 0)
	{
		// bareMkr starts with bt... so shorten it to simply bt for lookup purposes
		bareMkr = _T("bt"); // bareMkr is value param so only affects local copy
	}
	MapSfmToUSFMAnalysisStruct::iterator iter;
    // The particular MapSfmToUSFMAnalysisStruct used for lookup below depends the
    // appropriate sfm set being used as stored in gCurrentSfmSet enum.
	switch (gpApp->gCurrentSfmSet)
	{
		case UsfmOnly: 
			iter = gpApp->m_pUsfmStylesMap->find(bareMkr);
			bFound = (iter != gpApp->m_pUsfmStylesMap->end());
			break;
		case PngOnly: 
			iter = gpApp->m_pPngStylesMap->find(bareMkr);
			bFound = (iter != gpApp->m_pPngStylesMap->end()); 
			break;
		case UsfmAndPng: 
			iter = gpApp->m_pUsfmAndPngStylesMap->find(bareMkr);
			bFound = (iter != gpApp->m_pUsfmAndPngStylesMap->end()); 
			break;
		default: 
			iter = gpApp->m_pUsfmStylesMap->find(bareMkr);
			bFound = (iter != gpApp->m_pUsfmStylesMap->end());
	}
	if (bFound)
	{
#ifdef _Trace_RebuildDoc
		TRACE2("LookupSFM: bareMkr = %s   gCurrentSfmSet = %d  USFMAnalysis FOUND\n",bareMkr,gpApp->gCurrentSfmSet);
		TRACE1("LookupSFM:         filtered?  %s\n", iter->second->filter == TRUE ? "YES" : "NO");
#endif
		// iter->second points to the USFMAnalysis struct
		return iter->second;
	}
	else
	{
#ifdef _Trace_RebuildDoc
		TRACE2("\n LookupSFM: bareMkr = %s   gCurrentSfmSet = %d  USFMAnalysis NOT FOUND  ...   Unknown Marker\n",
				bareMkr,gpApp->gCurrentSfmSet);
#endif
		return (USFMAnalysis*)NULL;
	}
}

///////////////////////////////////////////////////////////////////////////////
/// \return		TRUE if the passed-in marker unkMkr exists in any element of the pUnkMarkers 
///				array, FALSE otherwise
/// \param		pUnkMarkers		-> a pointer to a wxArrayString that contains a list of 
///									markers
/// \param		unkMkr			-> the whole marker being checked to see if it exists 
///                                in pUnkMarkers
/// \param		MkrIndex		<- the index into the pUnkMarkers array if unkMkr is 
///                                found, otherwise -1
/// \remarks
/// Called from: the Doc's RestoreDocParamsOnInput(), GetUnknownMarkersFromDoc(),
/// CFilterPageCommon::AddUnknownMarkersToDocArrays().
/// Determines if a standard format marker (whole marker including backslash) exists in any
/// element of the array pUnkMarkers.
/// If the whole marker exists, the function returns TRUE and the array's index where the
/// marker was found is returned in MkrIndex. If the marker doesn't exist in the array
/// MkrIndex returns -1.
///////////////////////////////////////////////////////////////////////////////
bool CAdapt_ItDoc::MarkerExistsInArrayString(wxArrayString* pUnkMarkers, 
											 wxString unkMkr, int& MkrIndex)
{
	// returns TRUE if the passed-in marker unkMkr already exists in the pUnkMarkers
	// array. MkrIndex is the index of the marker returned by reference.
	int ct;
	wxString arrayStr;
	MkrIndex = -1;
	for (ct = 0; ct < (int)pUnkMarkers->GetCount(); ct++)
	{
		arrayStr = pUnkMarkers->Item(ct);
		if (arrayStr.Find(unkMkr) != -1)
		{
			MkrIndex = ct;
			return TRUE;
		}
	}
	// if we get to here we didn't find unkMkr in the array; 
	// MkrIndex is still -1 and return FALSE
	return FALSE;
}

///////////////////////////////////////////////////////////////////////////////
/// \return		TRUE if the passed-in marker wholeMkr exists in MarkerStr, FALSE otherwise
/// \param		MarkerStr	-> a wxString to be examined
/// \param		wholeMkr	-> the whole marker being checked to see if it exists in MarkerStr
/// \param		markerPos	<- the index into the MarkerStr if wholeMkr is found, otherwise -1
/// \remarks
/// Called from: the App's SetupMarkerStrings().
/// Determines if a standard format marker (whole marker including backslash) exists in a
/// given string. If the whole marker exists, the function returns TRUE and the zero-based
/// index into MarkerStr is returned in markerPos. If the marker doesn't exist in the
/// string markerPos returns -1.
///////////////////////////////////////////////////////////////////////////////
bool CAdapt_ItDoc::MarkerExistsInString(wxString MarkerStr, wxString wholeMkr, int& markerPos)
{
    // returns TRUE if the passed-in marker wholeMkr already exists in the string of
    // markers MarkerStr. markerPos is the position of the wholeMkr in MarkerStr returned
    // by reference.
	markerPos = MarkerStr.Find(wholeMkr);
	if (markerPos != -1)
		return TRUE;
    // if we get to here we didn't find wholeMkr in the string MarkerStr, so markerPos is
    // -1 and return FALSE
	return FALSE;
}

///////////////////////////////////////////////////////////////////////////////
/// \return		TRUE if the pUsfmAnalysis represents a filtering marker, FALSE otherwise
/// \param		pUsfmAnalysis	-> a pointer to a USFMAnalysis struct
/// \remarks
/// Called from: the Doc's TokenizeText().
/// Determines if a USFMAnalysis struct indicates that the associated standard format marker
/// is a filtering marker.
/// Prior to calling IsAFilteringSFM, the caller should have called LookupSFM(wxChar* pChar)
/// or LookupSFM(wxString bareMkr) to populate the pUsfmAnalysis struct, which should then 
/// be passed to this function.
///////////////////////////////////////////////////////////////////////////////
bool CAdapt_ItDoc::IsAFilteringSFM(USFMAnalysis* pUsfmAnalysis)
{
	// whm added 10Feb2005 in support of USFM and SFM Filtering support
	// whm removed 2nd parameter 9Jun05
    // Prior to calling IsAFilteringSFM, the caller should have called LookupSFM(TCHAR
    // *pChar) or LookupSFM(CString bareMkr) to determine pUsfmAnalysis which should then
    // be passed to this function.

	// Determine the filtering state of the marker
	if (pUsfmAnalysis)
	{
		// we have a known filter marker so return its filter status from the USFMAnalysis
		// struct found by previous call to LookupSFM()
		return pUsfmAnalysis->filter;
	}
	else
	{
		// the passed in pUsfmAnalysis was NULL so
		return FALSE;
	}
}

///////////////////////////////////////////////////////////////////////////////
/// \return		TRUE if unkMkr is both an unknown marker and it also is designated as a 
///				filtering marker, FALSE otherwise.
/// \param		unkMkr	-> a bare marker (without a backslash)
/// \remarks
/// Called from: the Doc's AnalyseMarker(), RedoNavigationText().
/// Determines if a marker is both an unknown marker and it also is designated as a 
/// filtering marker.
/// unkMKr should be an unknown marker in bare form (without backslash)
/// Returns TRUE if unkMKr exists in the m_unknownMarkers array, and its filter flag 
/// in m_filterFlagsUnkMkrs is TRUE.
///////////////////////////////////////////////////////////////////////////////
bool CAdapt_ItDoc::IsAFilteringUnknownSFM(wxString unkMkr)
{
	// unkMKr should be an unknown marker in bare form (without backslash)
	// Returns TRUE if unkMKr exists in the m_unknownMarkers array, and its filter flag 
	// in m_filterFlagsUnkMkrs is TRUE.
	int ct;
	unkMkr = gSFescapechar + unkMkr; // add the backslash
	for (ct = 0; ct < (int)gpApp->m_unknownMarkers.GetCount(); ct++)
	{
	if (unkMkr == gpApp->m_unknownMarkers.Item(ct))
		{
		// we've found the unknown marker so check its filter status
			if (gpApp->m_filterFlagsUnkMkrs.Item(ct) == TRUE)
				return TRUE;
		}
	}
	// the unknown marker either wasn't found (an error), or wasn't flagged to be filtered, so
	return FALSE;
}

///////////////////////////////////////////////////////////////////////////////
/// \return		TRUE if pChar is pointing at a standard format marker, FALSE otherwise
/// \param		pChar		-> a pointer to a character in a buffer
/// \param		pBufStart	<- a pointer to the start of the buffer
/// \remarks
/// Called from: the Doc's ParseFilteringSFM(), ParseFilteredMarkerText(), 
/// GetMarkerAndTextFromString(), TokenizeText(), DoMarkerHousekeeping(), the View's
/// FormatMarkerBufferForOutput(), FormatUnstructuredTextBufferForOutput(), 
/// ApplyOutputFilterToText(), ParseMarkerAndAnyAssociatedText(), IsMarkerRTF().
/// Determines if pChar is pointing at a standard format marker in the given buffer. If
/// pChar is pointing at a backslash, further tests are made if gbSfmOnlyAfterNewlines is
/// TRUE. In that case, it is only considered to be a marker if it is immediately preceded
/// by a newline character. Additional checks are also made if the marker is an inline
/// marker (see comments within the function for details).
///////////////////////////////////////////////////////////////////////////////
bool CAdapt_ItDoc::IsMarker(wxChar *pChar, wxChar* pBufStart)
{
	// from version 1.4.1 onwards, we have to allow for contextually defined
	// sfms. If the gbSfmOnlyAfterNewlines flag is TRUE, then sfms are only
	// identified as such when a newline precedes the sfm escape character (a backslash)

    // BEW changed 10Apr06, because the legacy algorithm (pre 3.0.9) did not allow for the
    // fact that inLine markers may or may not occur in the source text with a newline
    // preceding but are valid SFMs nevertheless, so a smarter test is called for
	if (*pChar == gSFescapechar)
	{
		// pointing at a potential SFM, so check the flag which asks for markers
		// only to be defined if they follow newlines
		if (gbSfmOnlyAfterNewlines)
		{
			// the flag is turned on, but we'll have to pass this marker through as a
			// valid marker, unilaterally, if it is an inLine marker - because these can
			// be not line initial yet the source file still is marked up correctly; so
			// check for the inLine == TRUE value, by looking up the marker in its 
			// USFMAnalysis struct & checking the inLine value
			USFMAnalysis* pUsfmAnalysis = LookupSFM(pChar);
			if (pUsfmAnalysis == NULL)
			{
				// it is not a known marker, so treat it as an unknown marker only
				// provided a newline precedes it; otherwise, it's an instance of
				// backslash which we want to ignore for marker identification purposes
				goto a;
			}
			if (pUsfmAnalysis->inLine)
			{
				// we don't care whether or not newline precedes, its a valid SFM
				return TRUE;
			}
			else
			{
				// its not inLine == TRUE, so now it can be a valid SFM only provided
				// it follows a newline
a:				if (IsPrevCharANewline(pChar,pBufStart))
				{
					return TRUE; // well-formed SFM file, marker is at start of line
				}
				else
				{
                    // marker is not at the start of the line - it's either a malformed SFM
                    // file, or the 'marker' is not to be interpretted as an SFM so that
                    // the backslash is treated as part of the word
					return FALSE;
				}
			}
		}
		else
		{
			// the flag is not turned on, so this backslash is to be interpretted
			// as starting an SFM
			return TRUE;
		}
	}
	else
	{
		// not pointing at a backslash, so it is not a marker
		return FALSE;
	}
}

///////////////////////////////////////////////////////////////////////////////
/// \return		TRUE if pChar is pointing at a standard format marker which is also an end 
///				marker (ends with an asterisk), FALSE otherwise.
/// \param		pChar	-> a pointer to a character in a buffer
/// \param		pEnd	<- a pointer to the end of the buffer
/// \remarks
/// Called from: the Doc's GetMarkersAndTextFromString(), AnalyseMarker(), the View's
/// FormatMarkerBufferForOutput(), DoExportInterlinearRTF(), ProcessAndWriteDestinationText().
/// Determines if the marker at pChar is a USFM end marker (ends with an asterisk). 
///////////////////////////////////////////////////////////////////////////////
bool CAdapt_ItDoc::IsEndMarker(wxChar *pChar, wxChar* pEnd)
{
	// Returns TRUE if pChar points to a marker that ends with *
	wxChar* ptr = pChar;
	// Advance the pointer forward until one of the following conditions ensues:
	// 1. ptr == pEnd (return FALSE)
	// 2. ptr points to a space (return FALSE)
	// 3. ptr points to another marker (return FALSE)
	// 4. ptr points to a * (return TRUE)
	while (ptr < pEnd)
	{
		ptr++;
		if (*ptr == _T('*'))
			return TRUE;
		else if (*ptr == _T(' ') || *ptr == gSFescapechar)
			return FALSE;
	}
	return FALSE;
}

///////////////////////////////////////////////////////////////////////////////
/// \return		TRUE if pChar is pointing at a standard format marker which is also 
///             an inLine marker (or embedded marker), FALSE otherwise.
/// \param		pChar	-> a pointer to a character in a buffer
/// \param		pEnd	<- currently unused
/// \remarks
/// Called from: the Doc's ParseFilteringSFM(), the View's FormatMarkerBufferForOutput().
/// Determines if the marker at pChar is a USFM inLine marker, i.e., one which is defined
/// in AI_USFM.xml with inLine="1" attribute. InLine markers are primarily "character
/// style" markers and also include all the embedded content markers whose parent markers
/// are footnote, endnotes and crossrefs.
///////////////////////////////////////////////////////////////////////////////
bool CAdapt_ItDoc::IsInLineMarker(wxChar *pChar, wxChar* WXUNUSED(pEnd))
{
	// Returns TRUE if pChar points to a marker that has inLine="1" [true] attribute
	wxChar* ptr = pChar;
	wxString wholeMkr = GetWholeMarker(ptr);
	int aPos = wholeMkr.Find(_T('*'));
	if (aPos != -1)
		wholeMkr.Remove(aPos,1);
	// whm revised 13Jul05. In order to get an accurate Find of wholeMkr below we
	// need to insure that the wholeMkr is followed by a space, otherwise Find would
	// give a false positive when wholeMkr is "\b" and the searched string has \bd, \bk
	// \bdit etc.
	wholeMkr.Trim(TRUE); // trim right end
	wholeMkr.Trim(FALSE); // trim left end
	wholeMkr += _T(' '); // insure wholeMkr has a single final space

	switch(gpApp->gCurrentSfmSet)
	{
	case UsfmOnly: 
		if (gpApp->UsfmInLineMarkersStr.Find(wholeMkr) != -1)
		{
			// it's an inLine marker
			return TRUE;
		}
		break;
	case PngOnly:
		if (gpApp->PngInLineMarkersStr.Find(wholeMkr) != -1)
		{
			// it's an inLine marker
			return TRUE;
		}
		break;
	case UsfmAndPng:
		if (gpApp->UsfmAndPngInLineMarkersStr.Find(wholeMkr) != -1)
		{
			// it's an inLine marker
			return TRUE;
		}
		break;
	default:
		if (gpApp->UsfmInLineMarkersStr.Find(wholeMkr) != -1)
		{
			// it's an inLine marker
			return TRUE;
		}
		break;
	}
	return FALSE;
}

///////////////////////////////////////////////////////////////////////////////
/// \return		TRUE if pChar is pointing at a standard format marker which is also a
///				corresponding end marker for the specified wholeMkr, FALSE otherwise.
/// \param		wholeMkr	-> a wxString containing the 
/// \param		pChar		-> a pointer to a character in a buffer
/// \param		pEnd		<- a pointer to the end of the buffer
/// \remarks
/// Called from: the Doc's ParseFilteringSFM(), ParseFilteredMarkerText(), 
/// GetMarkersAndTextFromString(), the View's ParseFootnote(), ParseEndnote(),
/// ParseCrossRef(), ProcessAndWriteDestinationText(), ApplyOutputFilterToText()
/// ParseMarkerAndAnyAssociatedText(), and CViewFilteredMaterialDlg::InitDialog().
/// Determines if the marker at pChar is the corresponding end marker for the
/// specified wholeMkr. 
/// IsCorresEndMarker returns TRUE if the marker matches wholeMkr and ends with an
/// asterisk. It also returns TRUE if the gCurrentSfmSet is PngOnly and wholeMkr 
/// passed in is \f and marker being checked at ptr is \fe or \F.
///////////////////////////////////////////////////////////////////////////////
bool CAdapt_ItDoc::IsCorresEndMarker(wxString wholeMkr, wxChar *pChar, wxChar* pEnd)
{
	// Returns TRUE if the marker matches wholeMkr and ends with *
	// Also returns TRUE if the gCurrentSfmSet is PngOnly and wholeMkr passed in
	// is \f and marker being checked at ptr is \fe or \F
	wxChar* ptr = pChar;

	// First, handle the PngOnly special case of \fe footnote end marker
	if (gpApp->gCurrentSfmSet == PngOnly && wholeMkr == _T("\\f"))
	{
		wxString tempStr = GetWholeMarker(ptr);
		// debug
		int len;
		len = tempStr.Length();
		// debug
		if (tempStr == _T("\\fe") || tempStr == _T("\\F"))
		{
			return TRUE;
		}
	}

	// not a PngOnly footnote situation so do regular USFM check 
	// for like a marker ending with * 
	for (int i = 0; i < (int)wholeMkr.Length(); i++)
	{
		if (ptr < pEnd)
		{
			if (*ptr != wholeMkr[i])
				return FALSE;
			ptr++;
		}
		else
			return FALSE;
	}
	// markers match through end of wholeMkr
	if (ptr < pEnd)
	{
		if (*ptr != _T('*'))
			return FALSE;
	}
	// the marker at pChar has an asterisk on it so we have the corresponding end marker
	return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
/// \return		TRUE if pChar is pointing at a standard format marker which is also a
///				chapter marker (\c ), FALSE otherwise.
/// \param		pChar		-> a pointer to a character in a buffer
/// \remarks
/// Called from: the Doc's TokenizeText(), DoMarkerHousekeeping(), 
/// DoExportInterlinearRTF(), DoExportSrcOrTgtRTF().
/// Returns TRUE only if the character following the backslash is a c followed by whitespace,
/// FALSE otherwise. Does not check to see if a number follows the whitespace.
///////////////////////////////////////////////////////////////////////////////
bool CAdapt_ItDoc::IsChapterMarker(wxChar *pChar)
{
	wxChar* ptr = pChar;
	ptr++;
	if (*ptr == _T('c'))
	{
		ptr++;
		return IsWhiteSpace(ptr);
	}
	else
		return FALSE;
}

///////////////////////////////////////////////////////////////////////////////
/// \return		TRUE if pChar is pointing at a Null character, i.e., (wxChar)0.
/// \param		pChar		-> a pointer to a character in a buffer
/// \remarks
/// Called from: the Doc's ParseWord(), TokenizeText(), DoMarkerHousekeeping(), the View's
/// DoExportSrcOrTgtRTF() and ProcessAndWriteDestinationText().
/// Returns TRUE if the buffer character at pChar is the null character (wxChar)0.
///////////////////////////////////////////////////////////////////////////////
bool CAdapt_ItDoc::IsEnd(wxChar *pChar)
{
	return *pChar == (wxChar)0;
}

///////////////////////////////////////////////////////////////////////////////
/// \return		a wxString constructed of the characters from the buffer, starting with the
///				character at ptr and including the next itemLen-1 characters.
/// \param		dest		<- a wxString that gets concatenated with the composed src string
/// \param		src			<- a wxString constructed of the characters from the buffer, starting with the
///								character at ptr and including the next itemLen-1 characters
/// \param		ptr			-> a pointer to a character in a buffer
/// \param		itemLen		-> the number of buffer characters to use in composing the src string
/// \remarks
/// Called from: the Doc's TokenizeText() and DoMarkerHousekeeping().
/// AppendItem() actually does double duty. It not only returns the wxString constructed from
/// itemLen characters (starting at ptr); it also returns by reference the composed
/// string concatenated to whatever was previously in dest.
/// In actual code, no use is made of the returned wxString of the AppendItem() 
/// function itself; only the value returned by reference in dest is used.
/// TODO: Change to a Void function since no use is made of the wxString returned.
///////////////////////////////////////////////////////////////////////////////
wxString& CAdapt_ItDoc::AppendItem(wxString& dest,wxString& src, const wxChar* ptr, int itemLen)
{
	src = wxString(ptr,itemLen);
	dest += src;
	return src;
}

///////////////////////////////////////////////////////////////////////////////
/// \return		a wxString constructed of dest + src (with inserted space between them if 
///				dest doesn't already end with a space).
/// \param		dest		<- a wxString that gets concatenated with the composed src string
/// \param		src			-> a wxString to be appended/concatenated to dest
/// \remarks
/// Called from: the Doc's TokenizeText().
/// AppendFilteredItem() actually does double duty. It not only returns the wxString src;
/// it also returns by reference in dest the composed/concatenated dest + src (insuring
/// that a space intervenes between unless dest was originally empty.
/// In actual code, no use is made of the returned wxString of the AppendFilteredItem() 
/// function itself; only the value returned by reference in dest is used.
/// TODO: Change to a Void function since no use is made of the wxString returned.
///////////////////////////////////////////////////////////////////////////////
wxString& CAdapt_ItDoc::AppendFilteredItem(wxString& dest,wxString& src)
{
	// whm added 11Feb05
	// insure the filtered item is padded with space if it is not first 
	// in dest
    if (!dest.IsEmpty())
	{
		if (dest[dest.Length() - 1] != _T(' '))
			// append a space, but only if there is not already one at the end
			dest += _T(' ');
	}
    dest += src;
    dest += _T(' ');
    return src;
}

///////////////////////////////////////////////////////////////////////////////
/// \return		the wxString starting at ptr and composed of itemLen characters after enclosing
///				the string between \~FILTER and \~FILTER* markers.
/// \param		ptr			-> a pointer to a character in a buffer
/// \param		itemLen		-> the number of buffer characters to use in composing the bracketed string
/// \remarks
/// Called from: the Doc's ReconstituteAfterFilteringChange(), TokenizeText().
/// Constructs the string starting at ptr (whose length is itemLen in the buffer); then
/// makes the string a "filtered" item by bracketing it between \~FILTER ... \~FILTER* markers.
///////////////////////////////////////////////////////////////////////////////
wxString CAdapt_ItDoc::GetFilteredItemBracketed(const wxChar* ptr, int itemLen)
{
	// whm added 11Feb05; BEW changed 06Oct05 to simpify a little and remove the unneeded first argument
	// (which was a CString&  -- because it was being called with wholeMkr supplied as that
	// argument's string, which was clobbering the marker in the caller)
	// bracket filtered info with unique markers \~FILTER and \~FILTER*
	//wxString src;
	wxString temp(ptr,itemLen);
	temp.Trim(TRUE); // trim right end
	temp.Trim(FALSE); // trim left end
	//wx version handles embedded new lines correctly
	wxString temp2;
	temp2 << filterMkr << _T(' ') << temp << _T(' ') << filterMkrEnd;
	temp = temp2;

	return temp;
}

///////////////////////////////////////////////////////////////////////////////
/// \return		a wxString with any \~FILTER and \~FILTER* filter bracketing markers removed
/// \param		src		-> the string to be processed (which has \~FILTER and \~FILTER* markers)
/// \remarks
/// Called from: the View's IsWrapMarker().
/// Returns the string after removing any \~FILTER ... \~FILTER* filter bracketing markers 
/// that exist in the string. Strips out multiple sets of bracketing filter markers if found
/// in the string. If src does not have any \~FILTER and \~FILTER* bracketing markers, src is
/// returned unchanged. Trims off any remaining space at left end of the returned string.
///////////////////////////////////////////////////////////////////////////////
wxString CAdapt_ItDoc::GetUnFilteredMarkers(wxString& src)
{
	// whm added 11Feb05
	// If src does not have the unique markers \~FILTER and \~FILTER* we only need return 
	// the src unchanged
	// The src may have embedded \n chars in it. Note: Testing shows that use
	// of CString's Find method here, even with embedded \n chars works OK. 
	int beginMkrPos = src.Find(filterMkr);
	int endMkrPos = src.Find(filterMkrEnd);
	while (beginMkrPos != -1 && endMkrPos != -1)
	{
		// Filtered material markers exist so we can remove all text between
		// the two filter markers inclusive of the markers. Filtered material
		// is never embedded within other filtered material so we can assume
		// that each sequence of filtered text we encounter can be deleted as
		// we progress linearly from the beginning of the src string to its end.
		wxString temps = filterMkrEnd;
		src.Remove(beginMkrPos, endMkrPos - beginMkrPos + temps.Length());
		beginMkrPos = src.Find(filterMkr);
		endMkrPos = src.Find(filterMkrEnd);
	}
	// Note: The string returned by GetUnFilteredMarkers may have an initial 
	// space, which I think would not usually happen in the legacy app before
	// filtering. I have therefore added the following line, which is also
	// probably needed for proper functioning of IsWrapMarker in the View:
	src.Trim(FALSE); // FALSE trims left end only
	return src;
}

///////////////////////////////////////////////////////////////////////////////
/// \return		0 (zero)
/// \remarks
/// Called from: the Doc's TokenizeText(), DoMarkerHousekeeping(), 
/// Clear's the App's working buffer.
/// TODO: Eliminate this function and the App's working buffer and just declare and use a local 
/// wxString buffer in the two Doc functions that call ClearBuffer(), and the View's version of 
/// ClearBuffer().
///////////////////////////////////////////////////////////////////////////////
int CAdapt_ItDoc::ClearBuffer()
{
	CAdapt_ItApp* pApp = &wxGetApp();
	wxASSERT(pApp != NULL);
	pApp->buffer.Empty();
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \return		TRUE unless the text in rText contains at least one marker that defines it 
///				as "structured" text, in which case returns FALSE
/// \param		rText	-> the string buffer being examined
/// \remarks
/// Called from: the Doc's TokenizeText().
/// Returns TRUE if rText does not have any of the following markers: \id \v \vt \vn \c \p \f \s \q
/// \q1 \q2 \q3 or \x.
///////////////////////////////////////////////////////////////////////////////
bool CAdapt_ItDoc::IsUnstructuredPlainText(wxString& rText)
// we deem the absence of \id and any of \v \vt \vn \c \p \f \s \q
// \q1 \q2 \q3 or \x standard format markers to be sufficient
// evidence that it is unstructured plain text
{
	wxString s1 = gSFescapechar;
	int nFound = -1;
	wxString s = s1 + _T("id ");
	nFound = rText.Find(s);
	if (nFound >= 0)
		return FALSE; // has \id
	s = s1 + _T("v ");
	nFound = rText.Find(s);
	if (nFound >= 0)
		return FALSE; // has \v
	s = s1 + _T("vn ");
	nFound = rText.Find(s);
	if (nFound >= 0)
		return FALSE; // has \vn
	s = s1 + _T("vt ");
	nFound = rText.Find(s);
	if (nFound >= 0)
		return FALSE; // has \vt
	s = s1 + _T("c ");
	nFound = rText.Find(s);
	if (nFound >= 0)
		return FALSE; // has \c

	// BEW added 10Apr06 to support small test files with just a few markers
	s = s1 + _T("p ");
	nFound = rText.Find(s);
	if (nFound >= 0)
		return FALSE; // has \p
	s = s1 + _T("f ");
	nFound = rText.Find(s);
	if (nFound >= 0)
		return FALSE; // has \f
	s = s1 + _T("s ");
	nFound = rText.Find(s);
	if (nFound >= 0)
		return FALSE; // has \s
	s = s1 + _T("q ");
	nFound = rText.Find(s);
	if (nFound >= 0)
		return FALSE; // has \q
	s = s1 + _T("q1 ");
	nFound = rText.Find(s);
	if (nFound >= 0)
		return FALSE; // has \q1
	s = s1 + _T("q2 ");
	nFound = rText.Find(s);
	if (nFound >= 0)
		return FALSE; // has \q2
	s = s1 + _T("q3 ");
	nFound = rText.Find(s);
	if (nFound >= 0)
		return FALSE; // has \q3
	s = s1 + _T("x ");
	nFound = rText.Find(s);
	if (nFound >= 0)
		return FALSE; // has \x
	// that should be enough, ensuring correct identification 
	// of even small test files with only a few SFM markers
	return TRUE; // assume unstructured plain text
}

///////////////////////////////////////////////////////////////////////////////
/// \return		TRUE if some character other than end-of-line char(s) (\n and/or \r) is found 
///				past the nFound position in rText, otherwise FALSE.
/// \param		rText		-> the string being examined
/// \param		nTextLength	-> the length of the rText string
/// \param		nFound		-> the position in rText beyone which we examine content
/// \remarks
/// Called from: the Doc's AddParagraphMarkers().
/// Determines if there are any characters other than \n or \r beyond the nFound position in
/// rText. Used in AddParagraphMarkers() to add "\p " after each end-of-line in rText.
///////////////////////////////////////////////////////////////////////////////
bool CAdapt_ItDoc::NotAtEnd(wxString& rText, const int nTextLength, int nFound)
{
	nFound++; // get past the newline
	if (nFound >= nTextLength - 1)
		return FALSE; // we are at the end

	int index = nFound;
	wxChar ch;
	while (((ch = rText.GetChar(index)) == _T('\r') || (ch = rText.GetChar(index)) == _T('\n'))
			&& index < nTextLength)
	{
		index++; // skip the carriage return or newline
		if (index >= nTextLength)
			return FALSE; // we have arrived at the end
	}

	return TRUE; // we found some other character before the end was reached
}

///////////////////////////////////////////////////////////////////////////////
/// \return		nothing
/// \param		rText		-> the string being examined
/// \param		nTextLength	-> the length of the rText string
/// \remarks
/// Called from: the Doc's TokenizeText().
/// Adds "\p " after each end-of-line in rText. The addition of \p markers is done to provide
/// minimal structuring of otherwise "unstructured" text for Adapt It's internal operations.
///////////////////////////////////////////////////////////////////////////////
void CAdapt_ItDoc::AddParagraphMarkers(wxString& rText, int& nTextLength)
{
	// adds \p followed by a space following every \n in the text buffer
	wxString s = gSFescapechar; 
	s += _T("p ");
	const wxChar* paragraphMarker = (const wxChar*)s;
	int nFound = 0;
	int nNewLength = nTextLength;
	while (((nFound = FindFromPos(rText,_T("\n"),nFound)) >= 0) && 
			NotAtEnd(rText,nNewLength,nFound))
	{
		nFound++; // point past the newline

		// we are not at the end, so we insert \p here
		rText = InsertInString(rText,nFound,paragraphMarker);
		nNewLength = rText.Length();
	}
	nTextLength = nNewLength;
}

///////////////////////////////////////////////////////////////////////////////
/// \return		a wxString with any multiple spaces reduced to single spaces
/// \param		rString		<- 
/// \remarks
/// Called from: the Doc's TokenizeText() and the View's DoExportSrcOrTgt().
/// Cleans up rString by reducing multiple spaces to single spaces.
/// TODO: This function could do its work much faster if it were rewritten to use a read buffer
/// and a write buffer instead of reading each character from a write buffer and concatenating 
/// the character onto a wxString. See OverwriteUSFMFixedSpaces() for a shell routine to use
/// as a starting point for revision.
///////////////////////////////////////////////////////////////////////////////
wxString CAdapt_ItDoc::RemoveMultipleSpaces(wxString& rString)
// reduces multiple spaces to a single space (code to do it using Find( ) function fails,
// because Find( ) has a bug which causes it to return the wrong index value wxWidgets
// note: Our Find() probably works, but we'll convert this anyway
// As written this function doesn't really need a write buffer since rString is not
// modified by the routine. A better approach would be to get rString's buffer using
// GetData(), and set up a write buffer for atemp which is the same size as rString + 1.
// Then copy characters using pointers from the rString buffer to the atemp buffer (not
// advancing the pointer for where multiple spaces are adjacent to each other).
{
	int nLen = rString.Length();
	wxString atemp;
	atemp.Empty();
	wxASSERT(nLen >= 0);
	wxChar* pStr = rString.GetWriteBuf(nLen + 1);
	wxChar* pEnd = pStr + nLen;
	wxChar* pNext;
	while (pStr < pEnd)
	{
		if (*pStr == _T(' '))
		{
			atemp += *pStr;
			pNext = pStr;
x:			++pNext;
			if (pNext >= pEnd)
			{
				rString.UngetWriteBuf();
				return atemp;
			}
			if (*pNext == _T(' '))
				goto x;
			else
				pStr = pNext;
		}
		else
		{
			atemp += *pStr;
			++pStr;
		}
	}
	rString.UngetWriteBuf(); //ReleaseBuffer();

	// wxWidgets code below (avoids use of pointers and buffer)
	// wxWidgets Find works OK, but the Replace method is quite slow.
	//wxString atemp = rString;
	//while ( atemp.Find(_T("  ")) != -1 )
	//	atemp.Replace(_T("  "), _T(" "),FALSE);
	return atemp;
}

///////////////////////////////////////////////////////////////////////////////
/// \return		nothing
/// \param		pstr	<- the wxString buffer 
/// \remarks
/// Called from: the Doc's OnNewDocument(), 
/// Removes any existing fixed space !$ sequences in pstr by overwriting the sequence 
/// with spaces. The processed text is returned by reference in pstr. 
/// This function call would normally be followed by a call to RemoveMultipleSpaces() to
/// remove any remaining multiple spaces. In our case, the subsequent call of
/// TokenizeText() in OnNewDocument() discards any extra spaces left by
/// OverwriteUSFMFixedSpaces().
///////////////////////////////////////////////////////////////////////////////
void CAdapt_ItDoc::OverwriteUSFMFixedSpaces(wxString*& pstr)
{
    // whm revised in wx version to have input string by reference in first parameter and
    // to set up a write buffer within this function.
	int len = (*pstr).Length();
	wxChar* pBuffer = (*pstr).GetWriteBuf(len + 1);
	wxChar* pBufStart = pBuffer;
	wxChar* pEnd = pBufStart + len;
	wxASSERT(*pEnd == _T('\0'));
	wxChar* ptr = pBuffer;
	while (ptr < pEnd)
	{
		if (wxStrncmp(ptr,_T("!$"),2) == 0)
		{
			// we are pointing at an instance of !$, 
			// so overwrite it and continue processing
			*ptr++ = _T(' ');
			*ptr++ = _T(' ');
		}
		else
		{
			ptr++;
		}
	}
	// whm len should not have changed, just release the buffer
	(*pstr).UngetWriteBuf();
}

///////////////////////////////////////////////////////////////////////////////
/// \return		nothing
/// \param		pstr	<- the wxString buffer 
/// \remarks
/// Called from: the Doc's OnNewDocument(), 
/// Removes any existing discretionary line break // sequences in pstr by overwriting the
/// sequence with spaces. The processed text is returned by reference in pstr.
/// This function call would normally be followed by a call to RemoveMultipleSpaces() to
/// remove any remaining multiple spaces. In our case, the subsequent call of
/// TokenizeText() in OnNewDocument() discards any extra spaces left by
/// .OverwriteUSFMDiscretionaryLineBreaks().
void CAdapt_ItDoc::OverwriteUSFMDiscretionaryLineBreaks(wxString*& pstr)
{
	int len = (*pstr).Length();
	wxChar* pBuffer = (*pstr).GetWriteBuf(len + 1);
	wxChar* pBufStart = pBuffer;
	wxChar* pEnd = pBufStart + len;
	wxASSERT(*pEnd == _T('\0'));
	wxChar* ptr = pBuffer;
	while (ptr < pEnd)
	{
		if (wxStrncmp(ptr,_T("//"),2) == 0)
		{
			// we are pointing at an instance of //, 
			// so overwrite it and continue processing
			*ptr++ = _T(' ');
			*ptr++ = _T(' ');
		}
		else
		{
			ptr++;
		}
	}
	// whm len should not have changed, just release the buffer
	(*pstr).UngetWriteBuf();
}

#ifndef __WXMSW__
#ifndef _UNICODE
///////////////////////////////////////////////////////////////////////////////
/// \return		nothing
/// \param		pstr	<- the wxString buffer 
/// \remarks
/// Called from: the Doc's OnNewDocument().
/// Changes MS Word "smart quotes" to regular quotes. The character values for smart quotes
/// are negative (-108, -109, -110, and -111). Warns the user if other negative character 
/// values are encountered in the text, i.e., that he should use TecKit to convert the data 
/// to Unicode then use the Unicode version of Adapt It.
///////////////////////////////////////////////////////////////////////////////
void CAdapt_ItDoc::OverwriteSmartQuotesWithRegularQuotes(wxString*& pstr)
{
	// whm added 12Apr2007
	bool hackedFontCharPresent = FALSE;
	int hackedCt = 0;
	wxString hackedStr;
	hackedStr.Empty();
	int len = (*pstr).Length();
	wxChar* pBuffer = (*pstr).GetWriteBuf(len + 1);
	wxChar* pBufStart = pBuffer;
	wxChar* pEnd = pBufStart + len;
	wxASSERT(*pEnd == _T('\0'));
	wxChar* ptr = pBuffer;
	while (ptr < pEnd)
	{
		if (*ptr == -111) // left single smart quotation mark
		{
			// we are pointing at a left single smart quote mark, so convert it to a regular single quote mark
			*ptr++ = _T('\'');
		}
		else if (*ptr == -110) // right single smart quotation mark
		{
			// we are pointing at a right single smart quote mark, so convert it to a regular single quote mark
			*ptr++ = _T('\'');
		}
		else if (*ptr == -109) // left double smart quotation mark
		{
			// we are pointing at a left double smart quote mark, so convert it to a regular double quote mark
			*ptr++ = _T('\'');
		}
		else if (*ptr == -108) // right double smart quotation mark
		{
			// we are pointing at a left double smart quote mark, so convert it to a regular double quote mark
			*ptr++ = _T('\'');
		}
		else if (*ptr < 0)
		{
			// there is a hacked 8-bit character besides smart quotes. Warn user that the data will not
			// display correctly in this version, that he should use TecKit to convert the data to Unicode
			// then use the Unicode version of Adapt It
			hackedFontCharPresent = TRUE;
			hackedCt++;
			if (hackedCt < 10)
			{
				int charValue = (int)(*ptr);
				hackedStr += _T("\n   character with ASCII value: ");
				hackedStr << (charValue+256);
			}
			else if (hackedCt == 10)
				hackedStr += _T("...\n");
			ptr++; // advance but don't change the char (we warn user below)
		}
		else
		{
			ptr++;
		}
	}
	// whm len should not have changed, just release the buffer
	(*pstr).UngetWriteBuf();

	// In this case we should warn every time a new doc is input that has the hacked chars
	// so we don't test for  && !gbHackedDataCharWarningGiven here.
	if (hackedFontCharPresent)
	{
		gbHackedDataCharWarningGiven = TRUE;
		wxString msg2 = _("\nYou should not use this non-Unicode version of Adapt It.\nYour data should first be converted to Unicode using TecKit\nand then you should use the Unicode version of Adapt It.");
		wxString msg1 = _("Extended 8-bit ASCII characters were detected in your\ninput document:");
		msg1 += hackedStr + msg2;
		wxMessageBox(msg1,_("Warning: Invalid Characters Detected"),wxICON_WARNING);
	}
}
#endif
#endif

///////////////////////////////////////////////////////////////////////////////
/// \return		the number of elements/tokens in the list of source phrases (pList)
/// \param		nStartingSequNum	-> the initial sequence number
/// \param		pList				<- list of source phrases populated with word tokens
/// \param		rBuffer				-> the buffer from which words are tokenized and stored 
///										as source phrases in pList
/// \param		nTextLength			-> the initial text length
/// \remarks
/// Called from: the Doc's OnNewDocument(), the View's TokenizeTextString(),
/// DoExtendedSearch(), DoSrcOnlyFind(), DoTgtOnlyFind(), DoSrcAndTgtFind().
/// Intelligently parses the input text (rBuffer) and builds a list of source phrases from
/// the tokens. All the input text's source phrases are analyzed in the process to
/// determine each source phrase's many attributes and flags, stores any filtered
/// information in its m_markers member.
///////////////////////////////////////////////////////////////////////////////
int CAdapt_ItDoc::TokenizeText(int nStartingSequNum, SPList* pList, wxString& rBuffer, 
							   int nTextLength)
// returns the number of elements in the list pList
{
	CAdapt_ItApp* pApp = &wxGetApp();
	wxASSERT(pApp != NULL);

	// whm Note: I'm declaring a local tokBuffer, in place of the buffer that MFC had on the doc
	// and previously the wx version had on the App. This is in attempt to get beyond the string
	// corruption problems.
	wxString tokBuffer;
	tokBuffer.Empty();

    // BEW added 26May06 to support maintaining m_bSpecialText TRUE when between \fq ..
    // \fq* nested sections within a \f ... \f* footnote section (similarly for nested
    // sections in \x ... \x* section), otherwise, between \fq* and the next \fq, any text
    // would otherwise revert to verse TextType and be shown in normal colour, rather than
    // being either footnote type or crossReference type and being shown in whatever is
    // m_bSpecialText == TRUE's current colour setting
	bool bFootnoteIsCurrent = FALSE;
	bool bCrossRefIsCurrent = FALSE;

    // for support of parsing in and filtering a pre-existing free translation (it has to
    // have CSourcePhrase instances have their m_bStartFreeTrans, m_bEndFreeTrans &
    // m_bHasFreeTrans members set TRUE at appropriate places)
	bool bFreeTranslationIsCurrent = FALSE;
	int nFreeTransWordCount = 0;
	bool bFilteredMarkerAlreadyTurnedOffTheFlag = FALSE; // TRUE when filtered \x \x* 
        // section has had the bCrossRefIsCurrent flag reset to FALSE already, or filtered
        // \f \f* section has had the bFootnoteIsCurrent flag reset to FALSE already

	wxString spacelessSrcPuncts = pApp->m_punctuation[0];
	while (spacelessSrcPuncts.Find(_T(' ')) != -1)
	{
		// remove all spaces, leaving only the list of punctation characters
		spacelessSrcPuncts.Remove(spacelessSrcPuncts.Find(_T(' ')),1); 
	}
	wxString boundarySet = spacelessSrcPuncts;
	while (boundarySet.Find(_T(',')) != -1)
	{
		boundarySet.Remove(boundarySet.Find(_T(',')),1);
	}

    // if the user is inputting plain text unstructured, we do not want to destroy any
    // paragraphing by the normalization process. So we test for this kind of text (if
    // there is no \id at file start, and also none of \v \vn \vt \c markers in the text,
    // then we assume it is not a sfm file) and if it is such, then we add a \p and a space
    // (paragraph marker) following every newline; and later if the user exports the source
    // or target text, we check if it was a plain text unstructured file by looking for (1)
    // no \id marker on the first sourcephrase instance, and no instances of \v \vn or \vt
    // in any sourcephrase in the document - if it matches these conditions, we test for \p
    // markers and where such is found, change the preceding space to a newline in the
    // output.
	//
    // whm revised 11Feb05 to support USFM and SFM Filtering. When TokenizeText enounters
    // previously filtered text (enclosed within \~FILTER ... \~FILTER* brackets), it
    // strips off those brackets so that TokenizeText can evaluate anew the filtering
    // status of the marker(s) that had been embedded within the filtered text. If the
    // markers and associated text are still to be filtered (as determined by LookupSFM()
    // and IsAFilteringSFM()), the filtering brackets are added again. If the markers
    // should no longer be filtered, they and their associated text are processed normally.
	// 
	bool bIsUnstructured = IsUnstructuredPlainText(rBuffer);

	// if unstructured plain text, add a paragraph marker after any newline, to preserve 
	// user's paragraphing updating nTextLength for each one added
	int nDerivedLength = nTextLength;
	if (bIsUnstructured)
	{
		AddParagraphMarkers(rBuffer, nDerivedLength);
		wxASSERT(nDerivedLength >= nTextLength);
	}

	// continue with the parse
	int nTheLen;
	if (bIsUnstructured)
	{
		nTheLen = nDerivedLength; // don't use rBuffer.GetLength() - as any newlines 
        // don't get counted; Bruce commented out the next line 10May08, but I've left it
        // there because I've dealt with and checked that other code agrees with the code
        // as it stands.
		++nTheLen; // make sure we include space for a null
	}
	else
	{
		nTheLen = nTextLength; // nTextLength was probably m_nInputFileLength in 
							   // the caller, which already counts the null at the end
	}

    // whm revision: I've modified OverwriteUSFMFixedSpaces and
    // OverwriteUSFMDiscretionaryLineBreaks to use a write buffer internally, and moved
    // them out of TokenizeText, so now we can get along with a read-only buffer here in
    // TokenizeText
	const wxChar* pBuffer = rBuffer.GetData();
	int itemLen = 0;
	wxChar* ptr = (wxChar*)pBuffer;		 // point to start of text
	wxChar* pBufStart = ptr;	 // preserve start address for use in testing for
								 // contextually defined sfms
    // wx Note: the following line in MFC version originally was overwriting the char
    // position past the end of pBuffer (because nTheLen was incremented in code above or
    // in the caller. Therefore I have moved the pEndText pointer back one
	// See MFC version for BEW modification of 10May08 where he left it: 
	// TCHAR* pEndText = pBuffer + nTheLen - 1; 
	wxChar* pEnd = pBufStart + rBuffer.Length(); // bound past which we must not go
	wxASSERT(*pEnd == _T('\0')); // ensure there is a null there

    // whm moved the OverwriteUSFMFixedSpaces and OverwriteUSFMDiscretionaryLineBreaks
    // calls out of TokenizeText. They really only need to be called from OnNewDocument
    // before the call to TokenizeText there.

	wxString temp;		// small working buffer in which to build a string
	tokBuffer.Empty(); 
	int	 sequNumber = nStartingSequNum - 1;
	CSourcePhrase* pLastSrcPhrase = (CSourcePhrase*)NULL; // initially there isn't one
	bool bHitMarker;

	USFMAnalysis* pUsfmAnalysis = NULL; // whm added 11Feb05

	wxString bdrySet = gpApp->m_punctuation[0];
	// Note: wxString::Remove must have the second param as 1 otherwise it will truncate
	// the remainder of the string
	int posn = bdrySet.Find(_T(','),FALSE);
	while (posn != -1)
	{
		bdrySet.Remove(posn,1); // all punct chars except comma are indicators 
								// of a phrase boundary
		posn = bdrySet.Find(_T(','),FALSE);
	}
	bdrySet = RemoveMultipleSpaces(bdrySet); 

	while (ptr < pEnd)
	{
		// we are not at the end, so we must have a new CSourcePhrase instance ready
		CSourcePhrase* pSrcPhrase = new CSourcePhrase;
		wxASSERT(pSrcPhrase != NULL);
		sequNumber++;
		pSrcPhrase->m_nSequNumber = sequNumber; // number it in sequential order
		bHitMarker = FALSE;

		if (IsWhiteSpace(ptr))
		{
			itemLen = ParseWhiteSpace(ptr);
			// I've commented out the next line, because I don't want inter-word spaces
			// entered into the m_markers string, as they are not markers (otherwise, they
			// get treated as 'phrase-internal markers' when phrases are constructed
			// AppendItem(buffer,temp,ptr,itemLen); // add white space to buffer
			ptr += itemLen; // advance pointer past the white space
		}

		// are we at the end of the text?
		if (IsEnd(ptr) || ptr >= pEnd)
		{
			// BEW added 05Oct05
			if (bFreeTranslationIsCurrent)
			{
                // we default to always turning off a free translation section at the end
                // of the document if it hasn't been done already
				if (pLastSrcPhrase)
				{
					pLastSrcPhrase->m_bEndFreeTrans = TRUE;
				}
			}

			delete pSrcPhrase->m_pSavedWords;
			pSrcPhrase->m_pSavedWords = (SPList*)NULL;
			delete pSrcPhrase->m_pMedialMarkers;
			pSrcPhrase->m_pMedialMarkers = (wxArrayString*)NULL;
			delete pSrcPhrase->m_pMedialPuncts;
			pSrcPhrase->m_pMedialPuncts = (wxArrayString*)NULL;
			delete pSrcPhrase;
			pSrcPhrase = (CSourcePhrase*)NULL;
			goto d;
		}

		// are we pointing at a standard format marker?
b:		if (IsMarker(ptr,pBufStart))
		{
			bHitMarker = TRUE;
			int nMkrLen = 0;
			// its a marker of some kind
			if (IsVerseMarker(ptr,nMkrLen))
			{
				// its a verse marker
				if (nMkrLen == 2)
				{
					tokBuffer += gSFescapechar;
					tokBuffer += _T("v");
					ptr += 2; // point past the \v marker
				}
				else
				{
					tokBuffer += gSFescapechar;
					tokBuffer += _T("vn");
					ptr += 3; // point past the \vn marker (Indonesia branch)
				}

				itemLen = ParseWhiteSpace(ptr);
				AppendItem(tokBuffer,temp,ptr,itemLen); // add white space to buffer
				ptr += itemLen; // point at verse number

				itemLen = ParseNumber(ptr);
				AppendItem(tokBuffer,temp,ptr,itemLen); // add number (or range eg. 3-5) to buffer
				pSrcPhrase->m_chapterVerse = pApp->m_curChapter; // set to n: form
				pSrcPhrase->m_chapterVerse += temp; // append the verse number
				pSrcPhrase->m_bVerse = TRUE; // set the flag to signal start of a new verse
				ptr += itemLen; // point past verse number

				// set pSrcPhrase attributes
				pSrcPhrase->m_bVerse = TRUE;
				if (pSrcPhrase->m_curTextType != poetry) // poetry sfm comes before \v
					pSrcPhrase->m_curTextType = verse;
				pSrcPhrase->m_bSpecialText = FALSE;

				itemLen = ParseWhiteSpace(ptr); // past white space after the marker
				AppendItem(tokBuffer,temp,ptr,itemLen);  // add it to the buffer
				ptr += itemLen; // point past the white space

				// BEW added 05Oct05
				if (bFreeTranslationIsCurrent)
				{
                    // we default to always turning off a free translation section at a new
                    // verse if a section is currently open -- this prevents assigning a
                    // free translation to the rest of the document if we come to where no
                    // free translations were assigned in another project's adaptations
                    // which we are inputting
					if (pLastSrcPhrase)
					{
						pLastSrcPhrase->m_bEndFreeTrans = TRUE;
					}
				}

				goto b; // check if another marker follows:
			}
			else if (IsChapterMarker(ptr)) // some other kind of marker - 
										   // perhaps its a chapter marker?
			{
				// its a chapter marker
				tokBuffer << gSFescapechar;
				tokBuffer << _T("c");
				ptr += 2; // point past the \c marker

				itemLen = ParseWhiteSpace(ptr);
				AppendItem(tokBuffer,temp,ptr,itemLen); // add white space to buffer
				ptr += itemLen; // point at chapter number

				itemLen = ParseNumber(ptr);
				AppendItem(tokBuffer,temp,ptr,itemLen); // add chapter number to buffer
				pApp->m_curChapter = temp;
				pApp->m_curChapter += _T(':'); // get it ready to append verse numbers
				ptr += itemLen; // point past chapter number

				// set pSrcPhrase attributes
				pSrcPhrase->m_bChapter = TRUE;
				pSrcPhrase->m_bVerse = TRUE; // always have verses following a chapter
				if (pSrcPhrase->m_curTextType != poetry) // poetry sfm comes before \v
					pSrcPhrase->m_curTextType = verse;
				pSrcPhrase->m_bSpecialText = FALSE;

				itemLen = ParseWhiteSpace(ptr); // parse white space following the number
				AppendItem(tokBuffer,temp,ptr,itemLen); // add it to buffer
				ptr += itemLen; // point past it

				goto b; // check if another marker follows
			}
			else
			{
				// neither verse nor chapter, but some other marker
				pUsfmAnalysis = LookupSFM(ptr); // NULL if unknown marker

			
                // if we filter something out, we must delay advancing over that stuff
                // because we want AnalyseMarker() to set the m_inform member of the
                // currently being built pSrcPhrase to whatever nav text is appropriate for
                // the filtered marker, but AnalyseMarker() also sets the TextType and
                // m_bSpecialText members - which then would get set to whatever was
                // filtered, which would be inappropriate. Often this is not a problem
                // because another marker follows, and these parameters get reset to what
                // is appropriate for the text in the document at that location; but if
                // there is no other marker to do that, then the wrong text type gets set,
                // and the text might be also set to special text colour - for example, a
                // \note ... \note* sequence, which can occur anywhere, would result in the
                // text type being wrongly set to note type, and the text colour to special
                // text's colour. To prevent this we look at the pLastSrcPhrase instance
                // and save it's TextType and m_bSpecialText value, and if filtering is
                // done, then after AnalyseMarkers() has done its job we restore the
                // TextType and m_bSpecialText values that were in force earlier than the
                // filtered section. BEW added 08June05
				enum TextType saveType;
				bool bSaveSpecial;
				if (pLastSrcPhrase)
				{
					saveType = pLastSrcPhrase->m_curTextType;
					bSaveSpecial = pLastSrcPhrase->m_bSpecialText;
				}
				else
				{
					// at doc start, we'll assume verse
					saveType = verse;
					bSaveSpecial = FALSE;
				}
				bool bDidSomeFiltering = FALSE;

                // check if we have located an SFM designated as one to be filtered
                // pUsfmAnalysis is populated regardless of whether it's a filter marker or
                // not. If an unknown marker, pUsfmAnalysis will be NULL

                // BEW changed 26May06; because of the following scenario. Suppose \y is in
                // the source text. This is an unknown marker, and if the app has not seen
                // this marker yet or, if it has, and the user has not nominated it as one
                // which is to be filtered out, then it will not be listed in the
                // fast-access string gCurrentFilterMarkers. Earlier versions of
                // TokenizeText() did not examine the contents of gCurrentFilterMarkers
                // when parsing source text, consequently, when the app is in the state
                // where \y as been requested to be filtered out (eg. as when user opens a
                // document which has that marker and in which it was filtered out; or has
                // created a document earlier in which \y occurred and he then requested it
                // be filtered out and left that setting intact when creating the current
                // document (which also has an \y marker)) then the current document
                // (unless we do something different than before) would not look at
                // gCurrentFilterMarkers and so not filter it out when, in fact, it should.
                // Moreover this can get worse. Firstly, because the
                // IsAFilteringUnknownSFM() call in AnalyseMarker looks at
                // m_currentUnknownMarkersStr, it detects that \y is currently designated
                // as "to be filtered" - and so refrains from placing "?\y? in the m_inform
                // member of the CSourcePhrase where the (unfiltered) unknown marker starts
                // (and it has special text colour, of course). So then the user sees a
                // text colour change and does not know why. If the unknown marker happens
                // to occur after some other special text, such as a footnote, then (even
                // worse) both have the same colour and the text in the unknown marker
                // looks like it is part of the footnote! Yuck.
                // 
                // The solution is to ensure that TokenizeText() gets to look at the
                // contents of gCurrentFilterMarkers every time a new doc is created, and
                // if it finds the marker listed there, to ensure it is filtered out. It's
                // no good appealing to AnalyseMarker(), because it just uses what is in
                // pUsfmAnalysis, and that comes from AI_USFM.xml, which by definition,
                // never lists unknown markers. (That's why they ARE unknown, just in case
                // you are having a bad day!) So the changes in the next few lines fix all
                // this - the test after the || had to be added.

				wxString wholeMkr = GetWholeMarker(ptr);
				wxString augmentedWholeMkr = wholeMkr + _T(' '); // prevent spurious matches
				// If an unknown marker, pUsfmAnalysis will be NULL
				if (IsAFilteringSFM(pUsfmAnalysis) || 
					(gpApp->gCurrentFilterMarkers.Find(augmentedWholeMkr) != -1))
				{
					bDidSomeFiltering = TRUE;
					itemLen = ParseFilteringSFM(wholeMkr,ptr,pBufStart,pEnd);

					// get filtered text bracketed by \~FILTER and \~FILTER*
                    // BEW changed 05Oct05, because GetFilteredItemBracketed call with
                    // wholeMkr as the first parameter results in the SF marker in wholeMkr
                    // being overwritten by the temp string's contents internally, so I
                    // rewrote the function and removed the first parameter from the
                    // signature (the new version is no slower, because while it has to
                    // copy the local string to return it, the old version did an internal
                    // copy anyway and I've removed that)
					temp = GetFilteredItemBracketed(ptr,itemLen);

                    // BEW added 06Jun06; if we just filtered out a footnote or cross
                    // reference, then code later on for turning off bFootnoteIsCurrent
                    // when \f* is encountered, or for turning off bCrossRefIsCurrent when
                    // \x* is encountered, will not be activated because
                    // ParseFilteringSFM() will have consumed the \f* or \x* endmarker, so
                    // we must test what wholeMkr is again here and turn off these flags
                    // when either marker has been filtered -- wholeMkr still has the
                    // beginning marker, so test for that, not the endmarker
					if (wholeMkr == _T("\\f"))
					{
						bFootnoteIsCurrent = FALSE;
						bFilteredMarkerAlreadyTurnedOffTheFlag = TRUE;
					}
					if (wholeMkr == _T("\\x"))
					{
						bCrossRefIsCurrent = FALSE;
						bFilteredMarkerAlreadyTurnedOffTheFlag = TRUE;
					}

                    // BEW added 05Oct05; CSourcePhrase class has new BOOL attributes in
                    // support of notes, backtranslations and free translations, so we have
                    // to set these at appropriate places in the parse.

                    // We may be at some free translation's anchor pSrcPhrase, having just
                    // set up the filter string to be put into m_markers; and if so, this
                    // string will contain a count of the number of following words to
                    // which the free translation applies; and this count will be
                    // bracketted by |@ (bar followed by @) at the start and @|<space> at
                    // the end, so we can search for these and if found, we extract the
                    // number string and remove the whole substring because it is only
                    // there to inform the parse operation and we don't want it in the
                    // constructed document. We use the count to determine which pSrcPhrase
                    // later encountered is the one to have its m_bEndFreeTrans member set
                    // TRUE.
					int nFound = temp.Find(_T("|@"));
					if (nFound != -1)
					{
						// there is some free translation to be handled
						int nFound2 = temp.Find(_T("@| "));
						wxASSERT(nFound2 - nFound < 10); // characters between can't be 
														 // too big a number
						wxString aNumber = temp.Mid(nFound + 2, nFound2 - nFound - 2);
						nFreeTransWordCount = wxAtoi(aNumber);
						wxASSERT(nFreeTransWordCount >= 0);

						// now remove the substring
						temp.Remove(nFound, nFound2 + 3 - nFound);

                        // now check for a word count value of zero -- we would get this if
                        // the user, in the project which supplied the exported text data,
                        // free translated a section of source text but did not supply any
                        // target text (if a target text export was done -- the same can't
                        // happen for source text of course). When such a \free field
                        // occurs in the data, there will be no pSrcPhrase to hang it on,
                        // because the parser will just collect all the empty markers into
                        // m_markers; so when we get a count of zero we should throw away
                        // the propagated free translation & let the user type another if
                        // he later adapts this section
						if (nFreeTransWordCount == 0)
						{
							temp.Empty();
						}
					}

                    // we can append the temp string to buffer now, because any count
                    // within it has just been removed
					AppendFilteredItem(tokBuffer,temp); // temp might have been emptied 
												// because of a zero nFreeTransWordCount
					if (wholeMkr == _T("\\note"))
					{
						pSrcPhrase->m_bHasNote = TRUE;
					}
					if (bFreeTranslationIsCurrent)
					{
						// a free translation section is current
						if (wholeMkr == _T("\\free"))
						{
                            // we've arrived at the start of a new section of free
                            // translation -- we should have already turned it off, but
                            // since we obviously haven't we'll do so now
							if (nFreeTransWordCount != 0)
							{
								bFreeTranslationIsCurrent = TRUE; // turn on this flag to 
									// inform parser in subsequent iterations that a new 
									// one is current
								if (pLastSrcPhrase->m_bEndFreeTrans == FALSE)
								{
									pLastSrcPhrase->m_bEndFreeTrans = TRUE; // turn off 
																	// previous section
								}

								// indicate start of new section
								pSrcPhrase->m_bHasFreeTrans = TRUE;
								pSrcPhrase->m_bStartFreeTrans = TRUE;
							}
							else
							{
								// we are throwing this section away, so turn off the flag
								bFreeTranslationIsCurrent = FALSE;

								if (pLastSrcPhrase->m_bEndFreeTrans == FALSE)
								{
									pLastSrcPhrase->m_bEndFreeTrans = TRUE; // turn off 
																	// previous section
								}
							}
						}
						else
						{
                            // for any other marker, if the section is not already turned
                            // off, then just propagate the free translation section to
                            // this sourcephrase too
							pSrcPhrase->m_bHasFreeTrans = TRUE;
						}
					}
					else
					{
                        // no free translation section is currently in effect, so check to
                        // see if one is about to start
						if (wholeMkr == _T("\\free"))
						{
							bFreeTranslationIsCurrent = TRUE; // turn on this flag to inform 
                                // parser in subsequent iterations that one is current
							pSrcPhrase->m_bHasFreeTrans = TRUE;
							pSrcPhrase->m_bStartFreeTrans = TRUE;
						}
					}
				}
				else
				{
                    // BEW added comment 21May05: it's not a filtering one, so the marker's
                    // contents (if any) will be visible and adaptable. The code here will
                    // ensure the marker is added to the buffer variable, and eventually be
                    // saved in m_markers; but for endmarkers we have to suppress their use
                    // for display in the navigation text area - we must do that job in
                    // AnalyseMarker() below.
					itemLen = ParseMarker(ptr);
					AppendItem(tokBuffer,temp,ptr,itemLen);
                    // being a non-filtering marker, it can't possibly be \free, and so we
                    // don't have to worry about the filter BOOL values in pSrcPhrase here
				}

				// set default pSrcPhrase attributes
                // BEW added 26May06 to support maintaining proper text type and colour
                // across text between nested special text marker subsections; BEW added
                // extra bFilteredMarkerAlreadyTurnedOffTheFlag test, so that after having
                // turned off one of these earlier, we don't here turn it back on when we
                // shouldn't. We only want this block to turn a flag on when the marker
                // concerned was not filtered out
				if (!bFilteredMarkerAlreadyTurnedOffTheFlag)
				{
                    // if the marker was not filtered, then permit this block to set one of
                    // the two flags if the marker was \f or \x (we later could have a
                    // third flag for endnotes with embedded marker subsections, but won't
                    // do so till someone complains)
					if (wholeMkr == _T("\\f"))
						bFootnoteIsCurrent = TRUE;
					if (wholeMkr == _T("\\f*"))
						bFootnoteIsCurrent = FALSE;
					if (wholeMkr == _T("\\x"))
						bCrossRefIsCurrent = TRUE;
					if (wholeMkr == _T("\\x*"))
						bCrossRefIsCurrent = FALSE;
				}
				else
				{
					// if we skipped the above block, we must now clear the flag to 
					// its default value
					bFilteredMarkerAlreadyTurnedOffTheFlag = FALSE;
				}
				if (pSrcPhrase->m_curTextType != poetry)
					pSrcPhrase->m_curTextType = verse; // assume verse unless 
								// AnalyseMarker changes it, or the block after it

                // analyse the marker and set fields accordingly, but not when we have just
                // filtered out the currently-being-processed marker and its contents - we
                // don't want to show any nav text for filtered markers and these must not
                // have the chance to alter the m_bSpecialText value either
				if (!bDidSomeFiltering)
					pSrcPhrase->m_bSpecialText = AnalyseMarker(pSrcPhrase,pLastSrcPhrase,
														(wxChar*)ptr,itemLen,pUsfmAnalysis);
	
                // BEW 26May06,for when there is the possibility of nested marker sections
                // which don't have the TextType of none, we want to prevent defaulting to
                // verse type and not special text, so deal with those here
				if (bCrossRefIsCurrent || bFootnoteIsCurrent)
				{
					if (bFootnoteIsCurrent)
					{
						pSrcPhrase->m_bSpecialText = TRUE;
						pSrcPhrase->m_curTextType = footnote;
					}
					if (bCrossRefIsCurrent)
					{
						pSrcPhrase->m_bSpecialText = TRUE;
						pSrcPhrase->m_curTextType = crossReference;
					}
				}

				// advance pointer past the marker
				ptr += itemLen;

                // reset text parameters to what was in effect before, if filtering was
                // done for the marker just identified BEW added 08June05
				if (bDidSomeFiltering)
				{
					pSrcPhrase->m_bSpecialText = bSaveSpecial;
					pSrcPhrase->m_curTextType = saveType;
				}

				itemLen = ParseWhiteSpace(ptr); // parse white space following it
				AppendItem(tokBuffer,temp,ptr,itemLen); // add it to buffer
				ptr += itemLen; // point past it

				goto b; // check if another marker follows
			}
		}
		else
		{
			// not a marker
			if (!bHitMarker)
			{
                // if no marker was hit, we can assume that the text characteristics are
                // continuing unchanged, so copy from earlier word/phrase to this one
				if (pLastSrcPhrase != NULL)
				{
					pSrcPhrase->CopySameTypeParams(*pLastSrcPhrase);
					// CopySameTypeParams copies these members:
					// 	m_curTextType, m_bSpecialText and m_bRetranslation
					
                    // BEW 26May06,for when there is the possibility of nested marker
                    // sections which don't have the TextType of none, we want to prevent
                    // defaulting to verse type and not special text, so deal with those
                    // here
					if (bCrossRefIsCurrent || bFootnoteIsCurrent)
					{
						if (bFootnoteIsCurrent)
						{
							pSrcPhrase->m_bSpecialText = TRUE;
							pSrcPhrase->m_curTextType = footnote;
						}
						if (bCrossRefIsCurrent)
						{
							pSrcPhrase->m_bSpecialText = TRUE;
							pSrcPhrase->m_curTextType = crossReference;
						}
					}
				}
			}

			// must be a word or special text - anyway, we have to adapt it
			// BEW changed the code below, 17 March 2005, to improve parsing 
			// of < << punctuation, etc
			wxString precStr = _T("");
			wxString follStr = _T("");
			itemLen = ParseWord(ptr, precStr,follStr,spacelessSrcPuncts);

            // from version 1.4.1 and onwards we will do NormalizeToSpaces() only on the
            // string of standard format markers which we store on sourcephrase instances,
            // and this is done after marker interpretation is finished (interpretation
            // optionally needs newlines to be retained in the source text string) - this
            // allows us to have normalization where we need it (ie. in the markers only)
            // without eliminating the needed newlines from the data - we do the
            // normalization now because we must store the stuff accumulated (ie. markers
            // and filtered info) into m_markers next, since we've broken out the next
            // actual adaptable word from the source text stream at the ParseWord() call
            // above.
			//
            // From March 17, 2005 we also normalize any white space between detached
            // preceding or or following quotation punctuation marks - this is done within
            // the ParseWord() call above.
			
			tokBuffer = NormalizeToSpaces(tokBuffer);
			pSrcPhrase->m_markers = tokBuffer;
			tokBuffer.Empty(); //strLen = ClearBuffer();

			// BEW added 30May05, to remove any initial space that may be in m_markers 
			// from the parse
			if (pSrcPhrase->m_markers.GetChar(0) == _T(' '))
				pSrcPhrase->m_markers = pSrcPhrase->m_markers.Mid(1);

            // ParseWord(), the new version, has returned the preceding and following text
            // strings, so we don't need to do any punctuation stripping. Instead, set the
            // relevant members directly, once we work out the lengths of the substrings
            // involved
			wxString strPunctuatedWord(ptr,itemLen);
			// in case some \n or \r characters managed to squeeze through, trim them off 
			// before using the string
			while (strPunctuatedWord.Find(_T('\n')) != -1)
			{
				strPunctuatedWord.Remove(strPunctuatedWord.Find(_T('\n')),1);
			}    
			while (strPunctuatedWord.Find(_T('\r')) != -1) 
			{
				strPunctuatedWord.Remove(strPunctuatedWord.Find(_T('\r')),1);
			}
			pSrcPhrase->m_srcPhrase = strPunctuatedWord;
			// now do the key and punctuation substrings
			int aLength = precStr.Length();
			if (aLength > 0)
			{
				pSrcPhrase->m_precPunct = precStr;
			}
			wxString spanned(ptr,itemLen);
			spanned = spanned.Mid(aLength); // get the remainder after preceding 
											// punctuation is skipped over
			ptr += aLength; // point to start of the word proper
			itemLen -= aLength; // reduce itemLen by the size of the preceding 
								// punctuation substring
			// get the length of the following punctuation substring
			aLength = follStr.Length();
			int wordLen = itemLen - aLength;
			wxASSERT(wordLen >= 0);
			wxString theWord(ptr,wordLen);
			pSrcPhrase->m_key = theWord;
			ptr += wordLen;
			aLength = follStr.Length();
			if (aLength > 0)
			{
				pSrcPhrase->m_follPunct = follStr;

                // BEW added 04Nov05, because I forgot to set boundaries when I redesigned
                // the algorithm for parsing in a source text file...
				// determine if the m_bBoundary flag needs to be set, and do so
				wxChar anyChar = follStr[0]; // get the first punctuation character,
											 // any will do
				if (boundarySet.Find(anyChar) != -1)
				{
                    // we found a non-comma final punctuation character on this word, so we
                    // have to set a boundary here
					pSrcPhrase->m_bBoundary = TRUE;
				}
			}
			itemLen = aLength;
			ptr += itemLen;

			// get rid of any final spaces which make it through the parse
			pSrcPhrase->m_follPunct.Trim(TRUE); // trim right end
			pSrcPhrase->m_follPunct.Trim(FALSE); // trim left end 

            // handle propagation of the m_bHasFreeTrans flag, and termination of the free
            // translation section by setting m_bEndFreeTrans to TRUE, when we've counted
            // off the requisite number of words parsed - there will be one per
            // sourcephrase when parsing; we do this only when bFreeTranslationIsCurrent is
            // TRUE
			if (bFreeTranslationIsCurrent)
			{
				if (nFreeTransWordCount != 0)
				{
					// decrement the count
					nFreeTransWordCount--;
					pSrcPhrase->m_bHasFreeTrans = TRUE;
					if (nFreeTransWordCount == 0)
					{
                        // we decremented to zero, so we are at the end of the current free
                        // translation section, so set the flags accordingly
						pSrcPhrase->m_bEndFreeTrans = TRUE; // indicate 'end of section'
						bFreeTranslationIsCurrent = FALSE; // indicate next sourcephrase 
                            // is not in the section (but a \free marker may turn it back
                            // on at next word)
					}
					
				}
			}

			// store the pointer in the SPList (in order of occurrence in text)
			pList->Append(pSrcPhrase);

			// make this one be the "last" one for next time through
			pLastSrcPhrase = pSrcPhrase;
		}// end of else when not a marker
	}; // end of while (ptr < pEndText)

d:	tokBuffer.Empty();

	// fix the sequence numbers, so that they are in sequence with no gaps, from the
	// beginning 
	AdjustSequNumbers(nStartingSequNum,pList);
	return pList->GetCount();
}

///////////////////////////////////////////////////////////////////////////////
/// \return		nothing
/// \param		useSfmSet		-> an enum of type SfmSet: UsfmOnly, PngOnly, or UsfmAndPng
/// \param		pUnkMarkers		<- a wxArrayString that gets populated with unknown (whole) markers,
///									always poopulated in parallel with pUnkMkrsFlags.
/// \param		pUnkMkrsFlags	<- a wxArrayInt of flags that gets populated with ones or zeros, 
///									always populated in parallel with pUnkMarkers.
/// \param		unkMkrsStr		-> a wxString containing the current unknown (whole) markers within
///									the string - the markers are delimited by spaces following each 
///									whole marker.
/// \param		mkrInitStatus	-> an enum of type SetInitialFilterStatus: setAllUnfiltered,
///									setAllFiltered, useCurrentUnkMkrFilterStatus, or
///									preserveUnkMkrFilterStatusInDoc
/// \remarks
/// Called from: the Doc's OnNewDocument(), CFilterPageCommon::AddUnknownMarkersToDocArrays()
/// and CFilterPagePrefs::OnOK().
/// Scans all the doc's source phrase m_markers members and inventories
/// all the unknown markers used in the current document; it stores all unique
/// markers in pUnkMarkers, stores a flag (1 or 0) indicating the filtering status
/// of the marker in pUnkMkrsFlags, and maintains a string called unkMkrsStr which 
/// contains the unknown markers delimited by following spaces.
/// An unknown marker may occur more than once in a given document, but is only 
/// stored once in the unknown marker inventory arrays and string. 
/// The SetInitialFilterStatus enum values can be used as follows:
///	  The setAllUnfiltered enum would gather the unknown markers into m_unknownMarkers 
///      and set them all to unfiltered state in m_filterFlagsUnkMkrs (currently
///      unused);
///	  The setAllFiltered could be used to gather the unknown markers and set them all to 
///      filtered state (currently unused);
///	  The useCurrentUnkMkrFilterStatus would gather the markers and use any currently 
///      listed filter state for unknown markers it already knows about (by inspecting 
///     m_filterFlagsUnkMkrs), but process any other "new" unknown markers as unfiltered.
///   The preserveUnkMkrFilterStatusInDoc causes GetUnknownMarkersFromDoc to preserve 
///     the filter state of an unknown marker in the Doc, i.e., set m_filterFlagsUnkMkrs 
///     to TRUE if the unknown marker in the Doc was within \~FILTER ... \~FILTER* brackets, 
///     otherwise sets the flag in the array to FALSE.
///////////////////////////////////////////////////////////////////////////////
void CAdapt_ItDoc::GetUnknownMarkersFromDoc(enum SfmSet useSfmSet,
											wxArrayString* pUnkMarkers,
											wxArrayInt* pUnkMkrsFlags,
											wxString & unkMkrsStr,
											enum SetInitialFilterStatus mkrInitStatus)
{
	CAdapt_ItDoc* pDoc = gpApp->GetDocument();
	SPList* pList = gpApp->m_pSourcePhrases;
	wxArrayString MarkerList; // gets filled with all the currently used markers including
							// filtered ones
	wxArrayString* pMarkerList = &MarkerList;

    // save the previous state of m_unknownMarkers and m_filterFlagsUnkMkrs to be able to
    // restore any previously set filter settings for the unknown markers, i.e., when the
    // useCurrentUnkMkrFilterStatus enum parameter is passed-in.
	wxArrayString saveUnknownMarkers;
    // wxArrayString does not have a ::Copy method like MFC's CStringArray::Copy, so we'll
    // do it by brute force CStringArray::Copy removes any existing items in the
    // saveUnknownMarkers array before copying all items from the m_unknownMarkers array
    // into it.
	saveUnknownMarkers.Clear();// start with an empty array
	int act;
	for (act = 0; act < (int)gpApp->m_unknownMarkers.GetCount(); act++)
	{
        // copy all items from m_unknownMarkers into saveUnknownMarkers note: do NOT use
        // subscript notation to avoid assert; i.e., do not use saveUnknownMarkers[act] =
        // gpApp->m_unknownMarkers[act]; instead use form below
		saveUnknownMarkers.Add(gpApp->m_unknownMarkers.Item(act));
	}
	wxArrayInt saveFilterFlagsUnkMkrs;
	// again copy by brute force elements from m_filterFlagsUnkMkrs to
	// saveFilterFlagsUnkMkrs 
	saveFilterFlagsUnkMkrs.Empty();
	for (act = 0; act < (int)gpApp->m_filterFlagsUnkMkrs.GetCount(); act++)
	{
		// copy all items from m_unknownMarkers into saveUnknownMarkers
		saveFilterFlagsUnkMkrs.Add(gpApp->m_filterFlagsUnkMkrs.Item(act));
	}

	// start with empty data
	pUnkMarkers->Empty();
	pUnkMkrsFlags->Empty();
	unkMkrsStr.Empty();
	wxString EqZero = _T("=0 "); // followed by space for parsing efficiency
	wxString EqOne = _T("=1 "); // " " "

	USFMAnalysis* pSfm;
	wxString key;

	MapSfmToUSFMAnalysisStruct* pSfmMap;
	pSfmMap = gpApp->GetCurSfmMap(useSfmSet);

	// Gather markers from all source phrase m_markers strings
	MapSfmToUSFMAnalysisStruct::iterator iter;
	SPList::Node* posn;
	posn = pList->GetFirst();
	wxASSERT(posn != NULL);
	CSourcePhrase* pSrcPhrase;
	while (posn != 0)
	{
		// process the markers in each source phrase m_markers string individually
		pSrcPhrase = (CSourcePhrase*)posn->GetData();
		posn = posn->GetNext();
		wxASSERT(pSrcPhrase);
		if (!pSrcPhrase->m_markers.IsEmpty())
		{
			// m_markers for this source phrase has content to examine
			pMarkerList->Empty(); // start with an empty marker list

            // The GetMarkersAndTextFromString function below fills the CStringList
            // pMarkerList with all the markers and their associated texts, one per list
            // item. Each item will include end markers for those that have them. Also,
            // Filtered material enclosed within \~FILTER...\~FILTER* brackets will also be
            // listed as a single item (even though there may be other markers embedded
            // within the filtering brackets.
			GetMarkersAndTextFromString(pMarkerList, pSrcPhrase->m_markers);

            // Now iterate through the strings in pMarkerList, check if the markers they
            // contain are known or unknown.
			wxString resultStr;
			resultStr.Empty();
			wxString wholeMarker, bareMarker;
			bool markerIsFiltered;
			int mlct;
			for (mlct = 0; mlct < (int)pMarkerList->GetCount(); mlct++) 
			{
				// examine this string list item
				resultStr = pMarkerList->Item(mlct);
				wxASSERT(resultStr.Find(gSFescapechar) == 0);
				markerIsFiltered = FALSE;
				if (resultStr.Find(filterMkr) != -1)
				{
					resultStr = pDoc->RemoveAnyFilterBracketsFromString(resultStr);
					markerIsFiltered = TRUE;
				}
				resultStr.Trim(FALSE); // trim left end
				resultStr.Trim(TRUE);  // trim right end
				int strLen = resultStr.Length();
				int posm = 1;
				wholeMarker.Empty();
				// get the whole marker from the string
				while (posm < strLen && resultStr[posm] != _T(' ') && 
						resultStr[posm] != gSFescapechar)
				{
					wholeMarker += resultStr[posm];
					posm++;
				}
				wholeMarker = gSFescapechar + wholeMarker;
				// do not include end markers in this inventory, so remove any final *
				int aPos = wholeMarker.Find(_T('*'));
				if (aPos == (int)wholeMarker.Length() -1)
					wholeMarker.Remove(aPos,1); 

				wxString tempStr = wholeMarker;
				tempStr.Remove(0,1);
				bareMarker = tempStr;
				wholeMarker.Trim(TRUE); // trim right end
				wholeMarker.Trim(FALSE); // trim left end
				bareMarker.Trim(TRUE); // trim right end
				bareMarker.Trim(FALSE); // trim left end
				wxASSERT(wholeMarker.Length() > 0);
                // Note: The commented out wxASSERT above can trip if the input text had an
                // incomplete end marker \* instead of \f* for instance, or just an
                // isolated backslash marker by itself \ in the text. Such typos become
                // unknown markers and show in the nav text line as ?\*? etc.

				// lookup the bare marker in the active USFMAnalysis struct map
                // whm ammended 11Jul05 Here we want to use the LookupSFM() routine which
                // treats all \bt... initial back-translation markers as known markers all
                // under the \bt marker with its description "Back-translation"
				pSfm = LookupSFM(bareMarker); // use LookupSFM which properly handles 
											  // \bt... forms as \bt
				bool bFound = pSfm != NULL;
				if (!bFound)
				{
                    // it's an unknown marker, so process it as such only add marker to
                    // m_unknownMarkers if it doesn't already exist there
					int newArrayIndex = -1;
					if (!MarkerExistsInArrayString(pUnkMarkers, wholeMarker, 
											newArrayIndex)) // 2nd param not used here
					{
						bool bFound = FALSE;
						// set the filter flag to unfiltered for all unknown markers
						pUnkMarkers->Add(wholeMarker);
						if (mkrInitStatus == setAllUnfiltered) // unused condition
						{
							pUnkMkrsFlags->Add(FALSE);
						}
						else if (mkrInitStatus == setAllFiltered) // unused condition
						{
							pUnkMkrsFlags->Add(TRUE);
						}
						else if (mkrInitStatus == preserveUnkMkrFilterStatusInDoc)
						{
                            // whm added 27Jun05. After any doc rebuild is finished, we
                            // need to insure that the unknown marker arrays and
                            // m_currentUnknownMarkerStr are up to date from what is now
                            // the situation in the Doc.
                            // Use preserveUnkMkrFilterStatusInDoc to cause
                            // GetUnknownMarkersFromDoc to preserve the filter state of an
                            // unknown marker in the Doc, i.e., set m_filterFlagsUnkMkrs to
                            // TRUE if the unknown marker in the Doc was within \~FILTER
                            // ... \~FILTER* brackets, otherwise the flag is FALSE.
							pUnkMkrsFlags->Add(markerIsFiltered);
						}
						else // mkrInitStatus == useCurrentUnkMkrFilterStatus
						{
                            // look through saved passed-in arrays and try to make the
                            // filter status returned for any unknown markers now in the
                            // Doc conform to the filter status in any corresponding saved
                            // passed-in arrays.
							int mIndex;
							for (mIndex = 0; mIndex < (int)saveUnknownMarkers.GetCount(); mIndex++)
							{
								if (saveUnknownMarkers.Item(mIndex) == wholeMarker)
								{
                                    // the new unknown marker is same as was in the saved
                                    // marker list so make the new unknown marker use the
                                    // same filter status as the saved one had
									bFound = TRUE;
									int oldFlag = saveFilterFlagsUnkMkrs.Item(mIndex);
									pUnkMkrsFlags->Add(oldFlag);
									break;
								}
							}
							if (!bFound)
							{
								// new unknown markers should always start being unfiltered
								pUnkMkrsFlags->Add(FALSE);
							}
						}
						unkMkrsStr += wholeMarker; // add it to the unknown markers string
						if (pUnkMkrsFlags->Item(pUnkMkrsFlags->GetCount()-1) == FALSE)
						{
							unkMkrsStr += EqZero; // add "=0 " unfiltered unknown marker
						}
						else
						{
							unkMkrsStr += EqOne; // add "=1 " filtered unknown marker
						}
					}
				}// end of if (!bFound)
			}// end of while (posMkrList != NULL)
		}// end of if (!pSrcPhrase->m_markers.IsEmpty())
	}// end of while (posn != 0)
}

///////////////////////////////////////////////////////////////////////////////
/// \return		a wxString containing a list of whole unknown markers; each 
///             marker (xx) formatted as "\xx=0 " or "\xx=1 " with following space within
///             the string.
/// \param		pUnkMarkers*	-> pointer to a wxArrayString of whole unknown markers
/// \param		pUnkMkrsFlags*	-> pointer to a wxArrayInt of int flags indicating 
///                                whether the unknown marker is filtered (1) 
///                                or unfiltered (0).
/// \remarks
/// Called from: Currently GetUnknownMarkerStrFromArrays() is only called from debug trace
/// blocks of code and only when the _Trace_UnknownMarkers define is activated.
/// Composes a string of unknown markers suffixed with a zero flag and following space ("=0
/// ") if the filter status of the unknown marker is unfiltered; or with a one flag and
/// followoing space ("=1 ") if the filter status of the unknown marker is filtered. The
/// function also verifies the integrity of the arrays, i.e., that they are consistent in
/// length - required for them to operate in parallel.
///////////////////////////////////////////////////////////////////////////////
wxString CAdapt_ItDoc::GetUnknownMarkerStrFromArrays(wxArrayString* pUnkMarkers, 
													 wxArrayInt* pUnkMkrsFlags)
{
	int ctMkrs = pUnkMarkers->GetCount();
	// verify that our arrays are parallel
	pUnkMkrsFlags = pUnkMkrsFlags; // to avoid compiler warning
	wxASSERT (ctMkrs == (int)pUnkMkrsFlags->GetCount());
	wxString tempStr, mkrStr;
	tempStr.Empty();
	for (int ct = 0; ct < ctMkrs; ct++)
	{
		mkrStr = pUnkMarkers->Item(ct);
		mkrStr.Trim(FALSE); // trim left end
		mkrStr.Trim(TRUE); // trim right end
		mkrStr += _T("="); // add '='
		mkrStr << pUnkMkrsFlags->Item(ct); // add a 1 or 0 flag formatted as string
		mkrStr += _T(' '); // insure a single final space
		tempStr += mkrStr;
	}
	return tempStr;
}

///////////////////////////////////////////////////////////////////////////////
/// \return		nothing
/// \param		pNewSrcPhrasesList			-> a list of pointers to CSourcePhrase instances
/// \param		nNewCount					-> how many entries are in the pNewSrcPhrasesList
///												list (currently unused)
/// \param		propagationType				<- the TextType value for the last CSourcePhrase 
///												instance in the list
/// \param		bTypePropagationRequired	<- TRUE if the function determines that the caller 
///												must take the returned propagationType value 
///												and propagate it forwards until a CSourcePhrase 
///												instance with its m_bFirstOfType member set TRUE 
///												is encountered, otherwise FALSE (no propagation 
///												needed)
/// \remarks
/// Called from: the Doc's RetokenizeText(), the View's ReconcileLists() and 
/// CTransferMarkersDlg::OnOK()
/// There are two uses for this function:
///   (1) To do navigation text, special text colouring, and TextType value cleanup 
///         after the user has edited the source text - which potentially allows the user
///         to edit, add or remove markers and/or change their location. Editing of markers
///         potentially might make a typo marker into one currently designated as to be
///         filtered, so this is checked for and if it obtains, then the requisite
///         filtering is done at the end as an extra (automatic) step.
///   (2) To do the same type of cleanup job, but over the whole document from start to 
///         end, after the user has changed the SFM set (which may also involve changing
///         filtering settings in the newly chosen SFM set, or it may not) - when used in
///         this way, all filtering changes will already have been done by custom code for
///         that operation, so DoMarkerHousekeeping() only needs to do the final cleanup of
///         the navigation text and text colouring and (cryptic) TextType assignments.
/// NOTE: m_FilterStatusMap.Clear(); is done early in DoMarkerHousekeeping(), so the prior
///         contents of the former will be lost.
///////////////////////////////////////////////////////////////////////////////
void CAdapt_ItDoc::DoMarkerHousekeeping(SPList* pNewSrcPhrasesList,int WXUNUSED(nNewCount), 
							TextType& propagationType, bool& bTypePropagationRequired)
{
    // The following comments were in the legacy versions, when this function was only used
    // after the source text was edited...
    // Typically, when this function is entered, the TextType may need adjusting or
    // setting, chapter and verse numbers and associated strings may need adjusting,
    // certain flags may need setting or clearing. This ensures all the attributes in each
    // sourcephrase instance are mutually consistent with the standard format markers
    // resulting from the user's editing of the source text and subsequent marker
    // editing/transfer operations.
	// The following indented comments only apply to the pre-3.7.0 versions:
        // Note: gpFollSrcPhrase may need to be accessed; but because this function is
        // called before unwanted sourcephrase instances are removed from the main list in
        // the case when the new sublist is shorter than the modified selected instances
        // sublist, then there would be one or more sourcephrase instances between the end
        // of the new sublist and gpFollSrcPhrase. If TextType propagation is required
        // after the sublist is copied to the main list and any unwanted sourcephrase
        // instances removed, then the last 2 parameters enable the caller to know the fact
        // and act accordingly
    // For the refactored source text edit functionality of 3.7.0, the inserting of new
    // instances is done after the old user's selection span's instances have been removed,
    // so there are no intervening unwanted CSourcePhrase instances. Propagation still may
    // be necessary, so we still return the 2 parameters to the caller for it to do any
    // such propagating. The function cannot be called, however, if the passed in list is
    // empty - it is therefore the caller's job to detect this and refrain

	CAdapt_ItApp* pApp = &wxGetApp();
	wxASSERT(pApp != NULL);
	
	SPList* pL = pNewSrcPhrasesList;
	wxASSERT(pL);
	CSourcePhrase* pLastSrcPhrase = gpPrecSrcPhrase; // the one immediately preceding sublist 
													 // (it could be null)
	CSourcePhrase* pFollowing = gpFollSrcPhrase; // first, following the sublist (could be null)
	CSourcePhrase* pSrcPhrase = 0; // the current one, used in our iterations

	gpApp->m_FilterStatusMap.clear(); 
            // empty the map, we want to use it to handle filtering of an edited marker
            // when the editing changes it to be a marker designated as one which is to be
            // filtered; we repopulate it just before the AnalyseMarker() call below

    // we'll use code and functions used for parsing source text, so we need to set up some
    // buffers so we can simulate the data structures pertinent to those function calls we
    // have to propagate the preceding m_bSpecialText value, until a marker changes it; if
    // there is no preceding context, then we can assume it is FALSE (if a \id follows,
    // then it gets reset TRUE later on)
	bool bSpecialText = FALSE;
	if (gpPrecSrcPhrase != 0)
		bSpecialText = gpPrecSrcPhrase->m_bSpecialText;

	// set up some local variables
	wxString mkrBuffer; // where we will copy standard format marker strings to, for parsing
	int itemLen = 0;
	int strLen = ClearBuffer(); // clear's the class's buffer[256] buffer
	bool bHitMarker;

    // BEW added 01Oct06; if the sublist (ie. pNewSrcPhrasesList) is empty (because the
    // user deleted the selected source text, then we can't get a TextType value for the
    // end of the sublist contents; so we get it instead from the gpFollSrcPhrase global,
    // which will have been set in the caller already (if at the end of the doc we'll
    // default the value to verse so that the source text edit does not fail)
	TextType finalType; // set this to the TextType for the last sourcephrase instance 
                        // in the sublist -- but take note of the above comment sublist

    // whm Note: if the first position node of pL is NULL finalType will not have been
    // initialized (the while loop never entered) and a bogus value will get assigned to
    // propagationType after the while loop. It may never happen that pos == NULL, but to
    // be sure I'm initializing finalType to noType
	finalType = noType;

	USFMAnalysis* pUsfmAnalysis = NULL; // whm added 11Feb05

	SPList::Node* pos = pL->GetFirst();
	bool bInvalidLast = FALSE;
	if (pLastSrcPhrase == NULL) // MFC had == 0
		bInvalidLast = TRUE; // only possible if user edited source text at the very 
                             // first sourcephrase in the doc iterate over each
                             // sourcephrase instance in the sublist
	while (pos != 0) // pos will be NULL if the pL list is empty
	{
		pSrcPhrase = (CSourcePhrase*)pos->GetData();
		pos = pos->GetNext();
		wxASSERT(pSrcPhrase);
		pSrcPhrase->m_inform.Empty(); // because AnalyseMarker() does +=, not =, 
									  // so we must clear its contents first

		// get any marker text into mkrBuffer
		mkrBuffer = pSrcPhrase->m_markers;
		int lengthMkrs = mkrBuffer.Length();
        // wx version note: Since we require a read-only buffer we use GetData which just
        // returns a const wxChar* to the data in the string.
		const wxChar* pBuffer = mkrBuffer.GetData();
		wxChar* pEndMkrBuff; // set this dynamically, for each 
							 // source phrase's marker string
		wxString temp; // can build a string here
		wxChar* ptr = (wxChar*)pBuffer;
		wxChar* pBufStart = (wxChar*)pBuffer;
		pEndMkrBuff = pBufStart + lengthMkrs; // point to null at end
		wxASSERT(*pEndMkrBuff == _T('\0')); // whm added for wx version - needs to be 
											// set explicitly when mkrBuffer is empty
		if (mkrBuffer.IsEmpty())
		{
            // there is no marker string on this sourcephrase instance, so if we are at the
            // beginning of the document, m_bFirstOfType will be TRUE, otherwise, there
            // will be a preceding sourcephrase instance and m_bFirstOfType will be FALSE,
            // and we can just copy it's value
			if (bInvalidLast)
			{
				pSrcPhrase->m_bFirstOfType = TRUE;
				pSrcPhrase->m_curTextType = verse; // this is the only possibility, 
												   // at start of doc & no marker
				bInvalidLast = FALSE; // all subsequent sourcephrases will have a valid 
									  // preceding one
				pSrcPhrase->m_inform.Empty();
				pSrcPhrase->m_chapterVerse.Empty();
				pSrcPhrase->m_bSpecialText = bSpecialText; // can not be special text here
			}
			else
			{
				pSrcPhrase->m_bFirstOfType = FALSE;
				pSrcPhrase->m_curTextType = pLastSrcPhrase->m_curTextType; // propagate the 
																// earlier instance's type
				pSrcPhrase->m_inform.Empty();
				pSrcPhrase->m_chapterVerse.Empty();
				pSrcPhrase->m_bSpecialText = pLastSrcPhrase->m_bSpecialText; // propagate the
																			 // previous value
			}
		}
		else
		{
			// there is a marker string on this sourcephrase instance
			if (bInvalidLast)
			{
				// we are at the very beginning of the document
				pLastSrcPhrase = 0; // ensure its null
				bInvalidLast = FALSE; // all subsequent sourcephrases will have a valid 
									  // preceding one
				pSrcPhrase->m_bSpecialText = bSpecialText; // assume this value, the marker 
														   // may later change it 
				goto x; // code at x comes from TokenizeText, and should not break for 
						// pLast == 0
			}
			else
			{
				// we are not at the beginning of the document
				pSrcPhrase->m_bSpecialText = pLastSrcPhrase->m_bSpecialText; // propagate 
																	// previous, as default
x:				while (ptr < pEndMkrBuff)
				{
					bHitMarker = FALSE;

					if (IsWhiteSpace(ptr))
					{
						itemLen = ParseWhiteSpace(ptr);
						ptr += itemLen; // advance pointer past the white space
					}

					// are we at the end of the markers text string?
					if (IsEnd(ptr) || ptr >= pEndMkrBuff)
					{
						break;
					}

					// are we pointing at a standard format marker?
b:					if (IsMarker(ptr,pBufStart)) // pBuffer added for v1.4.1 
												 // contextual sfms
					{
						bHitMarker = TRUE;
						// its a marker of some kind
						int nMkrLen = 0;
						if (IsVerseMarker(ptr,nMkrLen))
						{
							if (nMkrLen == 2)
							{
								// its a verse marker
								pApp->buffer += gSFescapechar;
								pApp->buffer += _T("v");
								ptr += 2; // point past the \v marker
							}
							else
							{
								// its an Indonesia branch verse marker \vn
								pApp->buffer += gSFescapechar;
								pApp->buffer += _T("vn");
								ptr += 3; // point past the \vn marker
							}

							itemLen = ParseWhiteSpace(ptr);
							AppendItem(pApp->buffer,temp,ptr,itemLen); // add white 
																	   // space to buffer
							ptr += itemLen; // point at verse number

							itemLen = ParseNumber(ptr);
							AppendItem(pApp->buffer,temp,ptr,itemLen); // add number (or range 
																	   // eg. 3-5) to buffer
							if (pApp->m_curChapter.GetChar(0) == '0')
								pApp->m_curChapter.Empty(); // caller will have set it non-zero 
															// if there are chapters
							pSrcPhrase->m_chapterVerse = pApp->m_curChapter; // set to n: form
							pSrcPhrase->m_chapterVerse += temp; // append the verse number
							pSrcPhrase->m_bVerse = TRUE; // set the flag to signal start of a 
														 // new verse
							ptr += itemLen; // point past verse number

							// set pSrcPhrase attributes
							pSrcPhrase->m_bVerse = TRUE;
							if (pSrcPhrase->m_curTextType != poetry) // poetry sfm comes 
																	 // before \v
							pSrcPhrase->m_curTextType = verse;
							pSrcPhrase->m_bSpecialText = FALSE;

							itemLen = ParseWhiteSpace(ptr); // past white space which is 
															// after the marker
							AppendItem(pApp->buffer,temp,ptr,itemLen); // add it to the buffer
							ptr += itemLen; // point past the white space

							goto b; // check if another marker follows:
						}
						// whm added for USFM and SFM Filtering support
						else if (IsFilteredBracketMarker(ptr,pEndMkrBuff))
						{
                            // When doing marker housekeeping, we'll just skip over any
                            // filtered material residing in m_markers. All such filtered
                            // material in m_markers is marked or bracketed between special
                            // markers \~FILTER and \~FILTER* by TokenizeText when text is
                            // input. ParseFilteredMarkerText below advances the pointer to
                            // point past the end-of-filtered-text marker \~FILTER* which
                            // prevents it from being seen again by AnalyseMarker.
							itemLen = ParseFilteredMarkerText(filterMkr,ptr,pBufStart,pEndMkrBuff);
							AppendItem(gpApp->buffer,temp,ptr,itemLen);
							ptr += itemLen;

							itemLen = ParseWhiteSpace(ptr); // past any white space after 
															// the filtered material
							AppendItem(gpApp->buffer,temp,ptr,itemLen); // add it to the buffer
							ptr += itemLen; // point past the white space

							goto b; // check if another marker follows
						}
						else
						{
							// some other kind of marker - perhaps its a chapter marker?
							if (IsChapterMarker(ptr))
							{
								// its a chapter marker
								pApp->buffer += gSFescapechar;
								pApp->buffer += _T("c");
								ptr += 2; // point past the \c marker

								itemLen = ParseWhiteSpace(ptr);
								AppendItem(pApp->buffer,temp,ptr,itemLen); // add white space
																		   // to buffer
								ptr += itemLen; // point at chapter number
								itemLen = ParseNumber(ptr);
								AppendItem(pApp->buffer,temp,ptr,itemLen); // add chapter number
                                                                           // to buffer
								pApp->m_curChapter = temp;
								pApp->m_curChapter += _T(':'); // get it ready to 
															   // append verse numbers
								ptr += itemLen; // point past chapter number

								// set pSrcPhrase attributes
								pSrcPhrase->m_bChapter = TRUE;
								pSrcPhrase->m_bVerse = TRUE; // always have verses following a 
															 // chapter
								if (pSrcPhrase->m_curTextType != poetry) // poetry sfm comes 
																		 // before \v
									pSrcPhrase->m_curTextType = verse;
								pSrcPhrase->m_bSpecialText = FALSE;

								itemLen = ParseWhiteSpace(ptr); // parse white space following 
																// the number
								AppendItem(pApp->buffer,temp,ptr,itemLen); // add it to buffer
								ptr += itemLen; // point past it

								goto b; // check if another marker follows
							}
							else
							{
                                // neither verse nor chapter, so we don't have to worry
                                // about a following number, so just append the marker to
                                // the buffer string

								pUsfmAnalysis = LookupSFM(ptr); // NULL if unknown marker

								itemLen = ParseMarker(ptr);
								AppendItem(pApp->buffer,temp,ptr,itemLen);

                                // we wish to know if this marker, which is not within a
                                // span bracketed by \~FILTER followed by \~FILTER*, has
                                // potentially been edited so that it really needs to be
                                // filtered, along with its following text content, rather
                                // than left unfiltered and its content visible for
                                // adapting in the doc. If it should be filtered, we will
                                // put an entry into m_FilterStatusMap to that effect, and
                                // the caller will later use the fact that that map is not
                                // empty to call RetokenizeText() with the option for
                                // filter changes turned on (ie. BOOL parameter 2 in the
                                // call is TRUE), and that will accomplish the required
                                // filtering.
								wxString mkr(ptr,itemLen); // construct the wholeMarker
								wxString mkrPlusSpace = mkr + _T(' '); // add the trailing space
								int curPos = gpApp->gCurrentFilterMarkers.Find(mkrPlusSpace);
								if (curPos >= 0)
								{
									// its a marker, currently unfiltered, which should be
									// filtered 
									wxString valStr;
									if (gpApp->m_FilterStatusMap.find(mkr) == gpApp->m_FilterStatusMap.end())
									{
                                        // marker does not already exist in
                                        // m_FilterStatusMap so add it as an entry meaning
                                        // 'now to be filtered' (a 1 value)
										(gpApp->m_FilterStatusMap)[mkr] = _T("1");
									}
								}

								// set default pSrcPhrase attributes
								if (pSrcPhrase->m_curTextType != poetry)
									pSrcPhrase->m_curTextType = verse; // assume verse unless 
																	 // AnalyseMarker changes it

								// analyse the marker and set fields accordingly
								pSrcPhrase->m_bSpecialText = AnalyseMarker(pSrcPhrase,pLastSrcPhrase,
															(wxChar*)ptr,itemLen,pUsfmAnalysis);

								// advance pointer past the marker
								ptr += itemLen;

								itemLen = ParseWhiteSpace(ptr); // parse white space after it
								AppendItem(pApp->buffer,temp,ptr,itemLen); // add it to buffer
								ptr += itemLen; // point past it
								goto b; // check if another marker follows
							}
						}
					}
					else
					{
						// get ready for next iteration
						strLen = ClearBuffer(); // empty the small working buffer
						itemLen = 0;
						ptr++;	// whm added. The legacy did not increment ptr here.
                                // The legacy app never reached this else block, because,
                                // if it had, it would enter an endless loop. The version 3
                                // app can have filtered text and can potentially reach
                                // this else block, so we must insure that we avoid an
                                // endless loop by incrementing ptr here.
					}
				}
			}
		}

		// make this one be the "last" one for next time through
		pLastSrcPhrase = pSrcPhrase;
		finalType = pSrcPhrase->m_curTextType; // keep this updated continuously, to be used 
											   // below
		gbSpecialText = pSrcPhrase->m_bSpecialText; // the value to be propagated at end of 
													// OnEditSourceText()
	} // end of while (pos != 0) loop

    // BEW added 01Oct06; handle an empty list situation (the above loop won't have been
    // entered so finalType won't yet be set
	if (pL->IsEmpty())
	{
		finalType = verse; // the most likely value, so the best default if the code 
						   // below doesn't set it
		if (gpFollSrcPhrase != NULL)
		{
            // using the following CSourcePhrase's TextType value is a sneaky way to ensure
            // we don't get any propagation done when the sublist was empty; as we don't
            // expect deleting source text to bring about the need for any propagation
            // since parameters should be correct already
			finalType = gpFollSrcPhrase->m_curTextType;
		}
        // BEW added 19Jun08; we need to also give a default value for gbSpecialText in
        // this case to, because prior to this change it was set only within the loop and
        // not here, and leaving it unset here would result in who knows what being
        // propagated, it could have been TRUE or FALSE when this function was called
		gbSpecialText = FALSE; // assume we want 'inspired text' colouring
	}

    // at the end of the (sub)list, we may have a different TextType than for the
    // sourcephrase which follows, if so, we will need to propagate the type if a standard
    // format marker does not follow, provided we are not at the document end, until either
    // we reach the doc end, or till we reach an instance with m_bFirstOfType set TRUE; but
    // nothing need be done if the types are the same already. We also have to propagate
    // the m_bSpecialText value, by the same rules.... if we have been cleaning up after an
    // SFM set change, which is done over the whole document (ie. m_pSourcePhrases list is
    // the first parameter), then pFollowing will have been set null in the caller, and no
    // propagation would be required
	bTypePropagationRequired = FALSE;
	propagationType = finalType;
	if (pFollowing == NULL) // MFC had == 0
		return;		// we are at the end of the document, so no propagation is needed
	if (pFollowing->m_curTextType == finalType)
		return;		// types are identical, so no propagation is needed
	if (pFollowing->m_bFirstOfType)
		return; // type changes here obligatorily (probably due to a marker), so 
                // we cannot propagate

	// if we get here, then propagation is required - so return that fact to the caller
	bTypePropagationRequired = TRUE;
}

///////////////////////////////////////////////////////////////////////////////
/// \return		the value of the m_bSpecialText member of pSrcPhrase
/// \param		pSrcPhrase		<- a pointer to the source phrase instance on 
///									which this marker will be stored
/// \param		pLastSrcPhrase	<- a pointer to the source phrase immediately preceding
///									the current pSrcPhrase one (may be null)
/// \param		pChar			-> pChar points to the marker itself (ie. the marker's 
///									backslash)
/// \param		len				-> len is the length of the marker at pChar in characters 
///									(not bytes), determined by ParseMarker() in the caller
/// \param		pUsfmAnalysis	<- a pointer to the struct on the heap that a prior call to 
///									LookupSFM(ptr) returned, and could be NULL for an 
///									unknown marker. AnalyseMarker can potentially change
///									this to NULL, but it doesn't appear that such a change
///									affects pUsfmAnalysis in any calling routine
/// \remarks
/// Called from: the Doc's ReconstituteAfterFilteringChange(), TokenizeText() and 
/// DoMarkerHousekeeping().
/// Analyzes the current marker at pChar and determines what TextType and/or other 
/// attributes should be applied to the the associated pSrcPhrase, and particularly
/// what the m_curTextType member should be. Determines if the current TextType should 
/// be propagated or changed to something else. The return value is used to set the 
/// m_bSpecialText member of pSrcPhrase in the caller.
///////////////////////////////////////////////////////////////////////////////
bool CAdapt_ItDoc::AnalyseMarker(CSourcePhrase* pSrcPhrase, CSourcePhrase* pLastSrcPhrase,
									wxChar* pChar, int len, USFMAnalysis* pUsfmAnalysis)
// BEW ammended 21May05 to improve handling of endmarkers and suppressing their display
// in navigation text area of main window
// pSrcPhrase is the source phrase instance on which this marker will be stored, 
// pChar points to the marker itself (ie. the marker's backslash), 
// len is its length in characters (not bytes), determined by ParseMarker() in the caller
// pUsfmAnalysis is the struct on the heap that a prior call to LookupSFM(ptr) returned,
// and could be NULL for an unknown marker.
// The returned BOOL is the value of the m_bSpecialText member of pSrcPhrase.
{
	CSourcePhrase* pThis = pSrcPhrase;
	CSourcePhrase* pLast = pLastSrcPhrase;
	wxString str(pChar,len);// need this to get access to wxString's overloaded operators
							// (some people define huge standard format markers, so we need
							// a dynamic string)
	wxString strMkr; // for version 1.4.1 and onwards, to hold the marker less the esc char
	strMkr = str.Mid(1); // we want everything after the sfm escape character
	bool bEndMarker = IsEndMarker(pChar,pChar+len);

	// BEW added 23Mayo5; the following test only can return TRUE when the passed in marker
	// at pChar is a beginning marker for an inline section (these have the potential to
	// temporarily interrupt the propagation of the TextType value, while another value 
	// takes effect), or is the beginning marker for a footnote
	wxString nakedMkr = GetBareMarkerForLookup(pChar);
	bool bIsPreviousTextTypeWanted = FALSE;
	if (!bEndMarker)
		bIsPreviousTextTypeWanted = IsPreviousTextTypeWanted(pChar,pUsfmAnalysis);
	if (bIsPreviousTextTypeWanted)
	{
		if (pLast)
		{
			// set the global, and it will stay set until an endmarker or a footnote
			// endmarker makes use of it, and then clears it to FALSE
			gPreviousTextType = pLast->m_curTextType;
		}
		else
			gPreviousTextType = verse; // a reasonable default at the start of the doc
	}

	// BEW 23May05 Don't delete this summary from Bill, it is a good systematic
	// treatment of what needs to be handled by the code below.

	// Determine how to handle usfm markers that can occur embedded
	// within certain other usfm markers. These include:
	//
	// 1. The "footnote content elements" marked by \fr...\fr*, \fk...\fk*, 
	// \fq...\fq*, \fqa...\fqa*, \fv...\fv*, \ft...\ft* and \fdc...\fdc*.
	// These footnote content element markers would be found only between 
	// \f and \f*. Their presence outside of \f and \f*, i.e., outside of
	// the footnote textType, would be considered an error.
	// Note: The ending markers for these footnote content elements are 
	// optional according to UBS docs, and the default is to only use the 
	// beginning marker and \ft to return to regular footnote text.
	//
	// 2. The "cross reference content element" markers. These include:
	// \xo...\xo*, \xk...\xk*, \xq...\xq*, \xt...\xt* and \xdc...\xdc*.
	// These cross reference content elements would be found only between 
	// \x and \x*. Their presence outside of \x and \x*, i.e., outside of
	// the crossReference textType, would be considered an error.
	//
	// 3. The "Special kinds of text" markers. These include: 
	// \qt...\qt*, \nd...\nd*, \tl...\tl*, \dc...\dc*, \bk...\bk*, 
	// \sig...\sig*, \pn...\pn*, \wj...\wj*, \k...\k*, \sls...\sls*, 
	// \ord...\ord*, and \add...\add*. Note: \lit is a special kind of 
	// text but is a paragraph style. These special kinds of text markers
	// can occur in verse, poetry, note and noType textTypes. Most of the 
	// special kinds of text markers could also occur in footnote textType.
	//
	// 4. The "Character styling" markers. These are now considered 
	// "DEPRECATED" by UBS, but include:
	// \no...\no*, \bd...\bd*, \it...\it*, \bdit...\bdit*, \em...\em*, 
	// and \sc...\sc*. They also can potentially be found anywhere. Adapt
	// It could (optionally) convert bar coded character formatting to
	// the equivalent character styling markers.
	//
	// 5. The "Special features" markers. These include: 
	// \fig...\fig*  (with the bar code separated parameters 
	// Desc|Cat|Size|Loc|Copy|Cap|Ref), and also the markers \pro...\pro*, 
	// \w...\w*, \wh...\wh*, \wg...\wg*, and \ndx...\ndx*. These also
	// may potentially be found anywhere.
	// 
	// The only end markers used in the legacy app were the footnotes \f* and \fe.
	// In USFM, however, we can potentially have many end markers, so we need
	// to have some special processing. I think I've got it smart enough, but it
	// can be changed further if necessary.

	if (!bEndMarker)
		pThis->m_bFirstOfType = TRUE; //default
	bool bFootnoteEndMarker = FALSE; // BEW added 23May05
	// pUsfmAnalysis will be NULL for unknown marker
	if (pUsfmAnalysis)
	{
		if (bEndMarker)
		{
			// verify that the found USFM marker matches  
			if (pUsfmAnalysis->endMarker != strMkr)
			{
				bEndMarker = FALSE;		// it isn't a recognized end marker
				pUsfmAnalysis = NULL;	// although the bare form was found 
										// lookup didn't really succeed
			}
		}
		// check for legacy png \fe and \F endmarkers that have no asterisk 
        // (BEW changes below, 23May05)
		if (pUsfmAnalysis && pUsfmAnalysis->png && (pUsfmAnalysis->marker == _T("fe") 
								|| pUsfmAnalysis->marker == _T("F")))
		{
			bEndMarker = TRUE; // we have a png end marker
			bFootnoteEndMarker = TRUE; // need this for restoring previous TextType
		}
		if (pUsfmAnalysis && pUsfmAnalysis->usfm && pUsfmAnalysis->endMarker == _T("f*"))
			bFootnoteEndMarker = TRUE; // need this for restoring previous TextType
	}
    // If bEndMarker is TRUE, our marker is actually an end marker. If pUsfmAnalysis is
    // NULL we can assume that it's an unknown type, so we'll treat it as special text as
    // did the legacy app, and will assign similar default values to pLast and pThis.

    // In the legacy app paragraphs (\p) had a unique m_bParagraph set TRUE and was the
    // only marker that did not set m_bFirstOfType to TRUE
	if (strMkr == _T("p"))
	{
		pThis->m_bParagraph = TRUE; // insure backwards compatibility even
									// if not used in legacy app
	}

	// pUsfmAnalysis will be NULL for unknown marker
	bool bEndMarkerForTextTypeNone = FALSE;
	bool bIsFootnote = FALSE;
	if (pUsfmAnalysis)
	{
		// Handle common//typical cases first...

        // Beginning footnote markers must set m_bFootnote to TRUE for backwards
        // compatibility with the legacy app (fortunately both usfm and png use 
        // the same \f marker!)
		if (strMkr == _T("f"))
		{
			pThis->m_bFootnote = TRUE;
			bIsFootnote = TRUE;
		}

        // Version 3.x sets m_curTextType and m_inform according to the attributes
        // specified in AI_USFM.xml (or default strings embedded in program code when
        // AI_USFM.xml is not available); we set a default here, but the special cases
        // further down may set different values
		pThis->m_curTextType = pUsfmAnalysis->textType;
		if (pUsfmAnalysis->inform && !bEndMarker)
		{
			if (!pThis->m_inform.IsEmpty() &&
				pThis->m_inform[pThis->m_inform.Length()-1] != _T(' '))
			{
				pThis->m_inform += _T(' ');
			}
			pThis->m_inform += pUsfmAnalysis->navigationText;
		}
		// Handle the special cases below....

		if (pLast != 0)
		{
			// stuff in here requires a valid 'last source phrase ptr'
			if (!bEndMarker)
			{
                // initial markers may, or may not, set a boundary on the last sourcephrase
                // (eg. those with TextType == none never set a boundary on the last
                // sourcephrase)
				pLast->m_bBoundary = pUsfmAnalysis->bdryOnLast;

				if (pUsfmAnalysis->inLine)
				{
					if (bIsFootnote || pUsfmAnalysis->marker == _T("x"))
					{
						pLast->m_bBoundary = TRUE;
						pThis->m_bFirstOfType = TRUE;
					}
					else
					{
                        // its not a footnote, or cross reference, but it is an inline
                        // section, so determine whether or not it's a section with
                        // TextType == none
						if (pUsfmAnalysis->textType == none)
						{
                            // this section is one where we just keep propagating the
                            // preceding context across it
							pThis->m_curTextType = pLast->m_curTextType;
							pThis->m_bSpecialText = pLast->m_bSpecialText;
							pThis->m_bBoundary = FALSE;
							pThis->m_bFirstOfType = FALSE;
							return pThis->m_bSpecialText;
						}
						else
						{
                            // this section takes its own TextType value, and other flags
                            // to suite (we'll set m_bFirstOfType only if there is to be a
                            // boundary on the preceding sourcephrse)
							pThis->m_bFirstOfType = pUsfmAnalysis->bdryOnLast == TRUE;
						}
					}
				}
				else
				{
					// let the type set in the common section stand unchanged
					;
				}
			}
			else
			{
				// we are dealing with an endmarker
				bEndMarkerForTextTypeNone = IsEndMarkerForTextTypeNone(pChar); // see if 
                    // its an endmarker from the marker subset: ord, bd, it, em, bdit, sc,
                    // pro, ior, w, wr, wh, wg, ndx, k, pn, qs ?)
				if (bEndMarkerForTextTypeNone)
				{
                    // these have had the TextType value, and m_bSpecialText value,
                    // propagated across the marker's content span which is now being
                    // closed off, so we can be sure that the TextType, etc, just needs to
                    // be propaged on here
					pThis->m_curTextType = pLast->m_curTextType;
					pThis->m_bSpecialText = pLast->m_bSpecialText;
					pThis->m_bFirstOfType = FALSE;
					pLast->m_bBoundary = FALSE;
					return pThis->m_bSpecialText;
				}
				else
				{
                    // it's one of the endmarkers for a TextType other than none; the
                    // subspan just traversed will typically have a TextType different from
                    // verse, and we can't just assume we are reverting to the verse
                    // TextType now (we did so in the legacy app which supported only the
                    // png sfm set, and the only endmarkers were footnote ones, but now in
                    // the usfm context we've a much greater set of possibilities, so we
                    // need smarter code), so we have to work out what the reversion type
                    // should be (note, it can of course be subsequently changed if a
                    // following marker is parsed and that marker sets a different
                    // TextType). Our approach is to copy the texttype saved ealier in
                    // gPreviousTextType, but before doing that we try get the right
                    // m_bFirstOfType value set
					
					// bleed out the footnote end case 
					if (bFootnoteEndMarker)
					{
						// its the end of a footnote (either png set or usfm set) so retore previous
						// type 
						pLast->m_bFootnoteEnd = TRUE;
						pThis->m_bFirstOfType = TRUE; // it's going to be something 
													  // different from a footnote
					}
					else
					{
						// not the end of a footnote

                        // some inline markers have text type of poetry or verse, so we
                        // don't want to put a m_bFirstOfType == TRUE value here when we
                        // are at the endmarker for one of those, because the most likely
                        // assumption is that the type will continue on here to what
                        // follows; but otherwise we do need to set it TRUE here because
                        // the type is most likely about to change
						USFMAnalysis* pAnalysis = LookupSFM(nakedMkr);
						if (pAnalysis)
						{
							if (pAnalysis->textType == verse)
							{
								pThis->m_bFirstOfType = FALSE;
							}
							else if (pAnalysis->textType == poetry)
							{
								pThis->m_bFirstOfType = FALSE;
							}
							else
							{
								pThis->m_bFirstOfType = TRUE;
							}
						}
						else
						{
							// wasn't a recognised marker, so all we can do is 
							// default to verse TextType
							pThis->m_curTextType = verse;
						}
					}
					pThis->m_curTextType = gPreviousTextType; // restore previous 
															  // context's value
					if (gPreviousTextType == verse || gPreviousTextType == poetry)
						pThis->m_bSpecialText = FALSE;
					else
						pThis->m_bSpecialText = TRUE;
					gPreviousTextType = verse; // restore the global's default value 
											   // for safety's sake
					return pThis->m_bSpecialText; // once we know its value, 
												  // we must return with it
				}
			}
		}
		return pUsfmAnalysis->special;
	}
	else  // it's an unknown sfm, so treat as special text
	{
		// we don't have a pUsfmAnalysis for this situation
		// so set some reasonable defaults (as did the legacy app)
		if (pLast != NULL)
		{
			// stuff in here requires a valid 'last source phrase ptr'
			pLast->m_bBoundary = TRUE;
		}
		pThis->m_bFirstOfType = TRUE;
		pThis->m_curTextType = noType;
		// just show the marker bracketed by question marks, i.e., ?\tn?
        // whm Note 11Jun05: I assume that an unknown marker should not appear in the
        // navigation text line if it is filtered. I've also modified the code in
        // RedoNavigationText() to not include the unknown marker in m_inform when the
        // unknown marker is filtered there, and it seems that would be appropriate here
        // too. If Bruce thinks the conditional call to IsAFilteringUnknownSFM is not
        // appropriate here the conditional I've added should be removed; likewise the
        // parallel code I've added near the end of RedoNavigationText should be evaluated
        // for appropriateness.
        // BEW comment 15Jun05 - I agree unknowns which are filtered should not appear with
        // ?..? bracketing; in fact, I've gone as far as to say that no filtered marker
        // should have its nav text displayed in the main window -- and coded accordingly
		if (!IsAFilteringUnknownSFM(nakedMkr))
		{
		pThis->m_inform += _T("?");
			pThis->m_inform += str; // str is a whole marker here
		pThis->m_inform += _T("? ");
		}
		//pThis->m_inform.FreeExtra();
		return TRUE; //ie. assume it's special text
	}
}

///////////////////////////////////////////////////////////////////////////////
/// \return		always TRUE
/// \remarks
/// Called from: the doc/view framework - but at different times in MFC than 
/// in the wx framework.
/// Deletes the list of source phrases (by calling DeleteSourcePhrases) and destroys the
/// view's strips, piles and cells.
/// This override is never explicitly called in the MFC version. In the wx version,
/// however, DeleteContents() needs to be called explicitly from the Doc's OnNewDocument(),
/// OnCloseDocument() and the View's ClobberDocument() because the doc/view framework in wx
/// works differently. In wx code, the Doc's OnNewDocument() must avoid calling the
/// wxDocument::OnNewDocument base class - because it calls OnCloseDocument() which in turn
/// would foul up the KB structures because OnCloseDocument() calls EraseKB(), etc. Instead
/// the wx version just calls DeleteContents() explicitly where needed.
///////////////////////////////////////////////////////////////////////////////
bool CAdapt_ItDoc::DeleteContents()
// refactored 10Mar09
// This is an override of the doc/view method
// MFC docs say: "Called by the framework to delete the document's data without destroying
// the CDocument object itself. It is called just before the document is to be destroyed.
// It is also called to ensure that a document is empty before it is reused. This is
// particularly important for an SDI application, which uses only one document; the
// document is reused whenever the user creates or opens another document. Call this
// function to implement an "Edit Clear All" or similar command that deletes all of the
// document's data. The default implementation of this function does nothing. Override this
// function to delete the data in your document."
{
	CAdapt_ItApp* pApp = &wxGetApp();
	// zero the contents of the read-in file of source text data
	if (pApp->m_pBuffer != 0)
	{
		pApp->m_pBuffer->Empty();
	}

	// delete the source phrases
	DeleteSourcePhrases(); // this does not delete the partner piles as 
		// the internal DeleteSingleSrcPhrase() has the 2nd param FALSE so
		// that it does not call DeletePartnerPile() -- so delete those
		// separately below en masse with the DestroyPiles() call

	// the strips, piles and cells have to be destroyed to make way for the new ones
	CAdapt_ItView* pView = (CAdapt_ItView*)NULL;
	pView = (CAdapt_ItView*)GetFirstView();
	wxASSERT(pApp != NULL);
	if (pView != NULL)
	{
		CLayout* pLayout = GetLayout();
		pLayout->DestroyStrips();
		pLayout->DestroyPiles(); // destroy these en masse
		pLayout->GetPileList()->Clear();
		pLayout->GetInvalidStripArray()->Clear();

		if (pApp->m_pTargetBox != NULL)
		{
			pApp->m_pTargetBox->ChangeValue(_T(""));
		}

		pApp->m_targetPhrase = _T("");
	}

	translation = _T(""); // make sure the global var is clear
	
	return wxDocument::DeleteContents(); // just returns TRUE
}


///////////////////////////////////////////////////////////////////////////////
/// \return		nothing
/// \param		pKB		<- a pointer to the current KB instance
/// \remarks
/// Called from: the App's SubstituteKBBackup(), ClearKB(), AccessOtherAdaptionProject(),
/// the Doc's OnCloseDocument(), the View's OnFileCloseProject(), DoConsistencyCheck().
/// Deletes the Knowledge Base after emptying and deleting its maps and its contained 
/// list of CTargetUnit instances, and each CTargetUnit's contained list of CRefString 
/// objects. (Note: OnCloseDocument() is not called explicitly by the Adapt It
/// code. It is called only by the framework when the application is being shut down.
/// Therefore it really involves a project closure as well, and therefore Erasure
/// of the KB and GlossingKB is appropriate; if the doc is dirty, then the user will
/// be given a chance to save it, and if he does, the KBs get saved then automatically
/// too.)
/// BEW added 13Nov09: removal of read-only protection (which involves unlocking
/// the ~AIROP-machinename-user-name.lock file, and removing it from the project
/// folder, rendering the project ownable for writing by whoever first enters it
/// subsequently). Doing this is appropriate in EraseKB() because EraseKB() is
/// called whenever the project is being left by whoever has current ownership
/// permission.
///////////////////////////////////////////////////////////////////////////////
void CAdapt_ItDoc::EraseKB(CKB* pKB)
{
	// Empty the map and list and delete their contained objects
	if (pKB != NULL)
	{
		// Clear all elements from each map, and delete each map
		for (int i = 0; i < MAX_WORDS; i++)
		{
			if (pKB->m_pMap[i] != NULL) // test for NULL whm added 10May04
			{
				if (!pKB->m_pMap[i]->empty())
				{
					pKB->m_pMap[i]->clear();
				}
				delete pKB->m_pMap[i];
				pKB->m_pMap[i] = (MapKeyStringToTgtUnit*)NULL; // whm added 10May04
			}
		}

		// Scan through each CTargetUnit in the m_pTargetUnits list. Delete each 
		// CRefString in each CTargetUnit's TranslationsList, and delete each
		// CTargetUnit. 
		for (TUList::Node* node = pKB->m_pTargetUnits->GetFirst(); 
				node; node = node->GetNext())
		{
			CTargetUnit* pTU = (CTargetUnit*)node->GetData();
			if (pTU->m_pTranslations->GetCount() > 0)
			{
				for (TranslationsList::Node* tnode = pTU->m_pTranslations->GetFirst(); 
						tnode; tnode = tnode->GetNext())
				{
					CRefString* pRefStr = (CRefString*)tnode->GetData();
					if (pRefStr != NULL)
					{
						delete pRefStr;
						pRefStr = (CRefString*)NULL; // whm added 10May04
					}
				}
			}
			delete pTU;
			pTU = (CTargetUnit*)NULL; // whm added 10May04
		}
		// Clear the m_pTargetUnits list and delete the list.
		pKB->m_pTargetUnits->Clear();
		delete pKB->m_pTargetUnits;
		pKB->m_pTargetUnits = (TUList*)NULL;
	
	}
	if (pKB != NULL)
	{
		// Lastly delete the KB itself
		delete pKB;
		pKB = (CKB*)NULL;
	}

	// BEW added 13Nov09, for restoring the potential for ownership for write permission
	// of the current accessed project folder.
	if (!gpApp->m_curProjectPath.IsEmpty())
	{
		bool bRemoved = gpApp->m_pROP->RemoveReadOnlyProtection(gpApp->m_curProjectPath);
		if (bRemoved)
		{
			gpApp->m_bReadOnlyAccess = FALSE; // project folder is now ownable for writing
		}
		// we are leaving this folder, so the local process must have m_bReadOnlyAccess unilaterally
		// returned to a FALSE value - whether or not a ~AIROP-*.lock file remains in the folder
		gpApp->m_bReadOnlyAccess = FALSE;
		gpApp->GetView()->canvas->Refresh(); // force color change back to normal white background
	}
}

///////////////////////////////////////////////////////////////////////////////
/// \return		always TRUE
/// \remarks
/// Called from: the doc/view framework automatically calls OnCloseDocument() when
/// necessary; it is not explicitly called from program code.
/// Closes down the current document and clears out the KBs from memory. This override does
/// not call the base class wxDocument::OnCloseDocument(), but does some document
/// housekeeping and calls DeleteContents() and finally sets Modify(FALSE).
///////////////////////////////////////////////////////////////////////////////
bool CAdapt_ItDoc::OnCloseDocument()
// MFC Note: This function just closes the doc down, with no saving, and clears out 
// the KBs from memory with no saving of them either (presumably, they were saved 
// to disk earlier if their contents were of value.) The disk copies of the KBs 
// therefore are unchanged from the last save.
//
// whm notes on CLOSING the App, the View, and the Doc:
// This note describes the flow of control when the MFC app (CFrameWnd) does OnClose() 
// and the WX app (wxDocParentFrame) does OnCloseWindow():
// IN MFC:	CFrameWnd::OnClose()
// - ->		pApp::CloseAllDocuments(FALSE) // parameter BOOL bEndSession
// - ->		CDocMagager::CloseAllDocuments(FALSE) // parameter BOOL bEndSession
// - ->		CDocTemplate::CloseAllDocuments(FALSE) // parameter BOOL bEndSession
// - ->		CAdapt_ItDoc::OnCloseDocument() [see note following:]
// - ->		CDocument::OnCloseDocument() // MFC app overrides this and calls it at end of override
// The MFC AI override calls EraseKB on the adapting and glossing KBs, updates
// some settings for the view and the app for saving to config files on closure, then
// lastly calls the CDocument::OnCloseDocument() which itself calls pFrame->DestroyWindow()
// for any/all views, then calls the doc's DeleteContents(), and finally calls delete on
// "this" the document if necessary (i.e., if m_bAutoDelete is TRUE). The important thing
// to note here is that WITHIN OnCloseDocument() ALL 3 of the following are done in this 
// order:	(1) The view(s) are closed (and associated frames destroyed) - a flag is used
//				to prevent (3) below from happening while closing/deleting the views
//			(2) The Doc's DeleteContents() is called
//			(3) The doc itself is deleted
//
// IN WX:	wxDocParentFrame::OnCloseWindow() // deletes all views and documents, then the wxDocParentFrame and exits the app
// - ->		wxDocManager::Clear() // calls CloseDocuments(force) and deletes the doc templates
// - ->*	wxDocManager::CloseDocuments(bool force) // On each doc calls doc->Close() then doc->DeleteAllViews() then delete doc
// - ->		wxDocument::Close() // first calls OnSaveModified, then if ok, OnCloseDocument()
// - ->		CAdapt_ItDoc::OnCloseDocument() [see note following:]
// - ->		[wxDocument::OnCloseDocument()] // WX app overrides this
// The WX AI override calls EraseKB on the adapting and glossing KBs and updates settings
// just as the MFC override does. Problems occur at line marked - ->* above due to the
// EARLY DeleteAllViews() call. This DeleteAllViews() also calls view->Close() on any
// views, and view->Close() calls view::OnClose() whose default behavior calls
// wxDocument::Close to "close the associated document." This results in OnCloseDocument()
// being called a second time in the process of closing the view(s), with damaging
// additional calls to EraseKB (the m_pMap[i] members have garbage pointers the second time
// around). To avoid this problem we need to override one or more of the methods that
// result in the additional damaging call to OnCloseDocument, or else move the erasing of
// our CKB structures out of OnCloseDocument() to a more appropriate place.
// Trying the override route, I tried first overriding view::OnClose() and doc::DeleteAllViews

// WX Note: Compare the following differences between WX and MFC
	// In wxWidgets, the default docview.cpp wxDocument::OnCloseDocument() looks like this:
				//bool wxDocument::OnCloseDocument()
				//{
				//	// Tell all views that we're about to close
				//	NotifyClosing();  // calls OnClosingDocument() on each view which does nothing in base class
				//	DeleteContents(); // does nothing in base class
				//	Modify(FALSE);
				//	return TRUE;
				//}

	// In MFC, the default doccore.cpp CDocument::OnCloseDocument() looks like this:
				//void CDocument::OnCloseDocument()
				//	// must close all views now (no prompting) - usually destroys this
				//{
				//	// destroy all frames viewing this document
				//	// the last destroy may destroy us
				//	BOOL bAutoDelete = m_bAutoDelete;
				//	m_bAutoDelete = FALSE;  // don't destroy document while closing views
				//	while (!m_viewList.IsEmpty())
				//	{
				//		// get frame attached to the view
				//		CView* pView = (CView*)m_viewList.GetHead();
				//		CFrameWnd* pFrame = pView->GetParentFrame();
				//		// and close it
				//		PreCloseFrame(pFrame);
				//		pFrame->DestroyWindow();
				//			// will destroy the view as well
				//	}
				//	m_bAutoDelete = bAutoDelete;
				//	// clean up contents of document before destroying the document itself
				//	DeleteContents();
				//	// delete the document if necessary
				//	if (m_bAutoDelete)
				//		delete this;
				//}
	// Compare wxWidgets wxDocument::DeletAllViews() below to MFC's OnCloseDocument() above:
				//bool wxDocument::DeleteAllViews()
				//{
				//    wxDocManager* manager = GetDocumentManager();
				//
				//    wxNode *node = m_documentViews.First();
				//    while (node)
				//    {
				//        wxView *view = (wxView *)node->Data();
				//        if (!view->Close())
				//            return FALSE;
				//
				//        wxNode *next = node->Next();
				//
				//        delete view; // Deletes node implicitly
				//        node = next;
				//    }
				//    // If we haven't yet deleted the document (for example
				//    // if there were no views) then delete it.
				//    if (manager && manager->GetDocuments().Member(this))
				//        delete this;
				//
				//    return TRUE;
				//}
	// Conclusion: Our wxWidgets version of OnCloseDocument() should NOT call the
	// wxDocument::OnCloseDocument() base class method, but instead should make the
	// following calls in its place within the OnCloseDocument() override:
	//		DeleteAllViews() // assumes 'delete this' at end can come before DeleteContents()
	//		DeleteContents()
	//		Modify(FALSE)
{
	CAdapt_ItApp* pApp = &wxGetApp();

	// the EraseKB() call will also try to remove any read-only protection
	EraseKB(pApp->m_pKB); // remove KB data structures from memory - EraseKB in the App in wx
	pApp->m_pKB = (CKB*)NULL; // whm added
	EraseKB(pApp->m_pGlossingKB); // remove glossing KB structures from memory - 
								  // EraseKB in the App in wx
	pApp->m_pGlossingKB = (CKB*)NULL; // whm added
	
	CAdapt_ItView* pView;
	CAdapt_ItDoc* pDoc;
	CPhraseBox* pBox;
	pApp->GetBasePointers(pDoc,pView,pBox);
	wxASSERT(pView);
	if (pApp->m_nActiveSequNum == -1)
		pApp->m_nActiveSequNum = 0;
	pApp->m_lastDocPath = pApp->m_curOutputPath;
	pApp->nLastActiveSequNum = pApp->m_nActiveSequNum;

    // BEW added 21Apr08; clean out the global struct gEditRecord & clear its deletion
    // lists, because each document, on opening it, it must start with a truly empty
    // EditRecord; and on doc closure and app closure, it likewise must be cleaned out
    // entirely (the deletion lists in it have content which persists only for the life of
    // the document currently open)
	pView->InitializeEditRecord(gEditRecord);
	gEditRecord.deletedAdaptationsList.Clear(); // remove any stored deleted adaptation strings
	gEditRecord.deletedGlossesList.Clear(); // remove any stored deleted gloss strings
	gEditRecord.deletedFreeTranslationsList.Clear(); // remove any stored deleted free translations

	// send the app the current size & position data, for saving to config files on closure
	wxRect rectFrame;
	CMainFrame *pFrame = wxGetApp().GetMainFrame();
	wxASSERT(pFrame);
	rectFrame = pFrame->GetRect(); // screen coords
	rectFrame = NormalizeRect(rectFrame); // use our own from helpers.h
	pApp->m_ptViewTopLeft.x = rectFrame.GetX();
	pApp->m_ptViewTopLeft.y = rectFrame.GetY();
	
	pApp->m_szView.SetWidth(rectFrame.GetWidth());
	pApp->m_szView.SetHeight(rectFrame.GetHeight());
	pApp->m_bZoomed = pFrame->IsMaximized();

	DeleteContents(); // this is required to avoid leaking heap memory on exit
	Modify(FALSE);

	return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
/// \return		nothing
/// \param		nValueForFirst	-> the number to use for the first source phrase in pList
/// \param		pList			<- a SPList of source phrases whose m_nSequNumber members are
///									to be put in sequence
/// \remarks
/// Called from: the Doc's TokenizeText().
/// Resets the m_nSequNumber member of all the source phrases in pList so that they are in 
/// numerical sequence (ascending order) with no gaps, starting with nValueForFirst.
/// This function differs from UpdateSequNumbers() in that AdjustSequNumbers() effects its 
/// changes only on the pList passed to the function; in UpdateSequNumbers() the current 
/// document's source phrases beginning with nFirstSequNum are set to numerical sequence 
/// through to the end of the document.
///////////////////////////////////////////////////////////////////////////////
void CAdapt_ItDoc::AdjustSequNumbers(int nValueForFirst, SPList* pList)
{
	CSourcePhrase* pSrcPhrase;
	SPList::Node *node = pList->GetFirst();
	wxASSERT(node != NULL);
	int sn = nValueForFirst-1;
	while (node)
	{
		sn++;
		pSrcPhrase = (CSourcePhrase*)node->GetData();
		node = node->GetNext(); 
		pSrcPhrase->m_nSequNumber = sn;
	}
}

///////////////////////////////////////////////////////////////////////////////
/// \return		nothing
/// \param		pstr	<- a wxString buffer containing text to process
/// \remarks
/// Called from: the Doc's OnNewDocument().
/// Removes any ventura publisher optional hyphen codes ("<->") from string buffer pstr.
/// After removing any ventura optional hyphens it resets the App's m_nInputFileLength 
/// global to reflect the new length.
///////////////////////////////////////////////////////////////////////////////
void CAdapt_ItDoc::RemoveVenturaOptionalHyphens(wxString*& pstr)
{
	CAdapt_ItApp* pApp = &wxGetApp();
	wxASSERT(pApp != NULL);
	wxString strVOH = _T("<->");
	int nFound = 0;
	int nNewLength = (*pstr).Length();
	while ((nFound = FindFromPos((*pstr),strVOH,nFound)) != -1)
	{
		// found an instance, so delete it
		// Note: wxString::Remove must have second param otherwise it will just
		// truncate the remainder of the string
		(*pstr).Remove(nFound,3);
	}

	// set the new length
	nNewLength = (*pstr).Length();
	pApp->m_nInputFileLength = (wxUint32)(nNewLength + 1); // include terminating null char
}

///////////////////////////////////////////////////////////////////////////////
/// \return		a wxString in which any \n, \r, or \t characters have been converted to spaces
/// \param		str		-> a wxString that is examined for embedded whitespace characters
/// \remarks
/// Called from: the Doc's TokenizeText().
/// This version of NormalizeToSpaces() is used in the wx version only.
/// This function changes any kind of whitespace (\n, \r, or \t) in str to simple space(s).
///////////////////////////////////////////////////////////////////////////////
wxString CAdapt_ItDoc::NormalizeToSpaces(wxString str)
{
	// whm added to normalize white space when presented with wxString (as done in the wx version)
	str.Replace(_T("\n"),_T(" ")); // LF (new line on Linux and internally within Windows)
	str.Replace(_T("\r"),_T(" ")); // CR (new line on Mac)
	str.Replace(_T("\t"),_T(" ")); // tab
	return str;
	// alternate code below:
	//wxString temp;
	//temp = str;
	//int len = temp.Length();
	//int ct;
	//for (ct = 0; ct < len; ct++)
	//{
	//	if (wxIsspace(temp.GetChar(ct)))
	//		temp.SetChar(ct, _T(' '));
	//}
	//return temp;
}

///////////////////////////////////////////////////////////////////////////////
/// \return		TRUE if pChar is pointing at an end marker whose associated textType is none, 
///				otherwise FALSE
/// \param		pChar	-> a pointer to a character in a buffer
/// \remarks
/// Called from: the Doc's AnalyseMarker().
/// Determines if the marker at pChar in a buffer is an end marker and if so, if the end marker
/// has an associated textType of none.
///////////////////////////////////////////////////////////////////////////////
bool CAdapt_ItDoc::IsEndMarkerForTextTypeNone(wxChar* pChar)
{
	wxString bareMkr = GetBareMarkerForLookup(pChar);
	wxASSERT(!bareMkr.IsEmpty());
	USFMAnalysis* pAnalysis = LookupSFM(bareMkr);
	wxString marker = GetMarkerWithoutBackslash(pChar);
	wxASSERT(!marker.IsEmpty());
	if (marker == pAnalysis->endMarker && pAnalysis->textType == none)
		return TRUE;
	else
		return FALSE;

}

///////////////////////////////////////////////////////////////////////////////
/// \return		the count of how many CSourcePhrase instances are in the m_pSourcePhrases list
///				after the retokenize and doc rebuild is done.
/// \param		bChangedPunctuation	-> TRUE if punctuation has changed, FALSE otherwise
/// \param		bChangedFiltering	-> TRUE if one or more markers' filtering status has changed,
///										FALSE otherwise
/// \param		bChangedSfmSet		-> TRUE if the Sfm Set has changed, FALSE otherwise
/// \remarks
/// Called from: the App's DoPunctuationChanges(), DoUsfmFilterChanges(),
/// DoUsfmSetChanges(), the View's OnEditSourceText().
/// Calls the appropriate document rebuild function for the indicated changes. For
/// punctuation changes if calls ReconstituteAfterPunctuationChange(); for filtering
/// changes it calls ReconstituteAfterFilteringChange(); and for sfm set changes, the
/// document is processed three times, the first pass calls SetupForSFMSetChange() and
/// ReconstituteAfterFilteringChange() to unfilter any previously filtered material, the
/// second pass again calls SetupForSFMSetChange() with adjusted parameters and
/// ReconstituteAfterFilteringChange() to filter any new filtering changes. The third pass
/// calls DoMarkerHousekeeping() to insure that TextType, m_bSpecialText, and m_inform
/// members of pSrcPhrase are set correctly at each location after the other major changes
/// have been effected.
///////////////////////////////////////////////////////////////////////////////
int CAdapt_ItDoc::RetokenizeText(bool bChangedPunctuation,bool bChangedFiltering, 
								 bool bChangedSfmSet)
// bew modified signature 18Apr05
// Returns the count of how many CSourcePhrase instances are in the m_pSourcePhrases list
// after the retokenize and doc rebuild is done.
{
	CAdapt_ItApp* pApp = &wxGetApp();
	wxASSERT(pApp != NULL);
	CAdapt_ItView* pView = pApp->GetView();
	wxASSERT(pView->IsKindOf(CLASSINFO(CAdapt_ItView)));

    // first determine whether or not the data was unstructured plain text - we need to
    // know because we don't want to try accumulate chapter:verse references for locations
    // where the rebuild failed if in fact there are no \c and \v markers in the original
    // source data!
	gbIsUnstructuredData = pView->IsUnstructuredData(gpApp->m_pSourcePhrases);

    // set up the string which is to preserve a record of where rebuilding will need to be
    // done manually later on, because what was expected did not occur
	wxString fixesStr;
	if (gbIsUnstructuredData)
	{
        // this one will not change, since there are no chapter verse references able to be
        // constructed
		//IDS_MANUAL_FIXES_NO_REFS
		fixesStr = _(
"There were places where automatic rebuilding did not fully succeed and so either adaptations were abandoned, or filtered material not made visible, or visible material not filtered. Please visually check the document and perhaps edit where necessary.");
	}
	else
	{
        // it has standard format markers including \v and \c (presumably), so we can add
        // n:m style of references to the end of the string - we take whatever is in the
        // m_chapterVerse wxString member of the passed in pointer to the CSourcePhrase
        // instance
		// IDS_MANUAL_FIXES_REFS
		fixesStr = _(
"Please locate the following chapter:verse locations and perhaps manually edit any section where the original adaptation had to be abandoned, or filtered material was not made visible, or visible material was not filtered, in the rebuild of the document: ");
	}

	int nOldTotal = gpApp->m_pSourcePhrases->GetCount();
	if (nOldTotal == 0)
	{
		return 0;
	}

	// put up a progress indicator
    // whm Note: RetokenizeText doesn't really need a progress dialog; it is mainly called
    // by other routines that have their own progress dialog, with the result that to have
    // a separate progress dialog for RetokenizeText, we end up with two progress dialogs,
    // one partially hiding the other.

	int nOldCount = 0;

	// whatever initialization is needed
	SPList::Node *pos;
	SPList::Node *oldPos;
    CSourcePhrase* pSrcPhrase = NULL;
	bool bNeedMessage = FALSE;

	// perform each type of document rebuild
	bool bSuccessful;
    if (bChangedPunctuation)
    {
        pos = gpApp->m_pSourcePhrases->GetFirst();
        while (pos != NULL)
        {
			oldPos = pos;
			pSrcPhrase = (CSourcePhrase*)pos->GetData();
			pos = pos->GetNext();

            // acts on ONE instance of pSrcPhrase only each time it loops, & it may add
            // many to the list, or remove some, or leave number in the list unchanged
			bSuccessful = ReconstituteAfterPunctuationChange(pView, 
							gpApp->m_pSourcePhrases, oldPos, pSrcPhrase,fixesStr);
			if (!bSuccessful)
			{
                // adaptation abandoned, so add a chapter:verse reference to the fixesStr
                // if the source text was (U)SFM structured text. The code for adding to
                // fixesStr (a chapter:verse reference plus 3 spaces) needs to be within
                // each of ReconstituteAfterPunctuationChange's subfunctions, as it depends
                // on pSrcPhrase being correct and so we must do the update to fixesStr
                // before we mess with replacing the pSrcPhrase with what was the new
                // parse's CSourcePhrase instances when automatic rebuild could not be done
                // correctly
				bNeedMessage = TRUE;
			}
			// update progress bar every 20 iterations
			++nOldCount;
			//if (20 * (nOldCount / 20) == nOldCount)
			//{
			//	//prog.m_progress.SetValue(nOldCount); //prog.m_progress.SetPos(nOldCount);
			//	//prog.TransferDataToWindow(); //prog.UpdateData(FALSE);
			//	//wxString progMsg = _("Retokenizing - File: %s  - %d of %d Total words and phrases");
			//	//progMsg = progMsg.Format(progMsg,gpApp->m_curOutputFilename.c_str(),nOldCount,nOldTotal);
			//}
        }
    }

	if (bChangedSfmSet)
    {
        // We accomplish the desired effects by successive calls of
        // ReconstituteAfterFilteringChanges(), with a little massaging of the data
        // structures in which it relies, before each call.
		bool bSuccessful;

        // The first pass through the document has to be done with the previous SFM set in
        // effect, and the saved wxString of the previous filter markers for that set -
        // these have to be unfiltered, but we won't unfilter any which are markers in
        // common with the new set and which are also filtered in the new set. Since we are
        // going to fiddle with the SfmSet value, we need to save the current value and
        // restore it when we are done. m_sfmSetAfterEdit stores the current value in
        // effect after the Preferences are exited, so we will use that for restoring
        // gCurrentSfmSet later below

#ifdef _Trace_RebuildDoc
		TRACE1("\n saveCurrentSfmSet = %d\n",(int)m_sfmSetAfterEdit);
		TRACE3("\n bChangedSfmSet TRUE; origSet %d, newSet %d, origMkrs %s\n", m_sfmSetBeforeEdit,
					gpApp->gCurrentSfmSet, m_filterMarkersBeforeEdit);
		TRACE2("\n bChangedSfmSet TRUE; curFilterMkrs: %s\n and the secPassMkrs: %s, pass = FIRST\n\n", 
					gpApp->gCurrentFilterMarkers, m_secondPassFilterMarkers);
#endif

		SetupForSFMSetChange(gpApp->m_sfmSetBeforeEdit, gpApp->gCurrentSfmSet, 
				gpApp->m_filterMarkersBeforeEdit,gpApp->gCurrentFilterMarkers, 
				gpApp->m_secondPassFilterMarkers, first_pass);

		if (gpApp->m_FilterStatusMap.size() > 0)
		{
			// we only unfilter if there is something to unfilter
			bSuccessful = ReconstituteAfterFilteringChange(pView, 
								gpApp->m_pSourcePhrases, fixesStr);
			if (!bSuccessful)
			{
                // at least one error, so make sure there will be a message given (the
                // ReconstituteAfterFilteringChange() function will append the needed
                // material to fixesStr internally (each time there is such an error)
                // before returning
				bNeedMessage = TRUE;
			}
		}

        // restore the filtering status of the original set's markers, in case the user
        // later changes back to that set (gCurrentSfmSet's value is still the one for the
        // old set)
		ResetUSFMFilterStructs(gpApp->gCurrentSfmSet, gpApp->m_filterMarkersBeforeEdit, _T(""));

        // restore the current SFM set value. This is the value which the user changed to
        // in the USFMPage, and it has to be the current value when the second pass is
        // executed below
		gpApp->gCurrentSfmSet = gpApp->m_sfmSetAfterEdit;

#ifdef _Trace_RebuildDoc
		TRACE3("\n bChangedSfmSet TRUE; origSet %d, newSet %d, origMkrs %s\n", m_sfmSetBeforeEdit, 
				gpApp->gCurrentSfmSet, m_filterMarkersBeforeEdit);
		TRACE2("\n bChangedSfmSet TRUE; gCurrentFilterMarkers: %s\n and the secPassMkrs: %s, pass = SECOND\n\n", 
				gpApp->gCurrentFilterMarkers, m_secondPassFilterMarkers);
#endif

		SetupForSFMSetChange(gpApp->m_sfmSetBeforeEdit, 
					gpApp->gCurrentSfmSet, gpApp->m_filterMarkersBeforeEdit,
					gpApp->gCurrentFilterMarkers, gpApp->m_secondPassFilterMarkers, 
					second_pass);

		if (gpApp->m_FilterStatusMap.size() > 0)
		{
			// we only filter if there is something to filter
			bSuccessful = ReconstituteAfterFilteringChange(pView, 
											gpApp->m_pSourcePhrases, fixesStr);
			if (!bSuccessful)
			{
                // at least one error, so make sure there will be a message given (the
                // ReconstituteAfterFilteringChange() function will append the needed
                // material to fixesStr internally (each time there is such an error)
                // before returning
				bNeedMessage = TRUE;
			}
		}

        // Typically, after any refiltering is done, there will be errors remaining in the
        // document - these are old pSrcPhrase->m_inform strings which are now out of date,
        // TextType values which are set or changed at the wrong places and improperly
        // propagated in the light of the new SFM set now in effect, and likewise
        // m_bSpecialText will in many places be wrong, changed when it shouldn't be, etc.
        // To fix all this stuff we have to scan across the whole document with the
        // DoMarkerHousekeeping() function, which duplicates some of TokenizeText()'s code,
        // to get the TextType, m_bSpecialText, and m_inform members of pSrcPhrase correct
        // at each location Setup the globals for this call...
		gpFollSrcPhrase = NULL; // the "sublist" is the whole document, so there is no 		
		gpPrecSrcPhrase = NULL; // preceding or following source phrase to be considered
		gbSpecialText = FALSE;
		gPropagationType = verse; // default at start of a document
		gbPropagationNeeded = FALSE; // gpFollSrcPhrase is null, & we can't propagate 
									 // at end of doc
		int docSrcPhraseCount = gpApp->m_pSourcePhrases->size();
		DoMarkerHousekeeping(gpApp->m_pSourcePhrases,
								docSrcPhraseCount,gPropagationType,gbPropagationNeeded);
	}

    if (bChangedFiltering)
	{
        // if called, ReconstituteAfterFilteringChange() sets up the progress window again
        // and destroys it before returning
		bool bSuccessful = ReconstituteAfterFilteringChange(pView, 
													gpApp->m_pSourcePhrases, fixesStr);
		if (!bSuccessful)
		{
            // at least one error, so make sure there will be a message given (the
            // ReconstituteAfterFilteringChange() function will append the needed material
            // to fixesStr internally (each time there is such an error) before returning
			bNeedMessage = TRUE;
		}
	}

	// find out how many instances are in the list when all is done and return it to the
	// caller 
    int count = gpApp->m_pSourcePhrases->GetCount();

	// make sure everything is correctly numbered in sequence; shouldn't be necessary, 
    // but no harm done
	UpdateSequNumbers(0);

	// warn the user if he needs to do some visual checking etc.
	if (bNeedMessage)
	{
        // make sure the message is not too huge for display - if it exceeds 1200
        // characters, trim the excess and add "... for additional later locations where
        // manual editing is needed please check the document visually. A full list has
        // been saved in your project folder in Rebuild Log.txt" (the addition is not put
        // in the message if the data is unstructured as (U)SFM stuff)
		int len = fixesStr.Length();

		// build the path to the current project's folder and output the full log
		if (!gbIsUnstructuredData)
		{
			wxString path;
			path.Empty();
			path << gpApp->m_curProjectPath;
			path << gpApp->PathSeparator;
			path << _T("Rebuild Log");
            // add a unique number each time, incremented by one from previous number
            // (starts at 0 when app was launched)
			gnFileNumber++; // get next value
			path << gnFileNumber;
			path << _T(".txt");
		
			wxLogNull logNo; // avoid spurious messages from the system

			wxFile fout;
			bool bOK;
			bOK = fout.Open( path, wxFile::write );
			fout.Write(fixesStr,len);
			fout.Close();
		}

        // prepare a possibly shorter message - if there are not many bad locations it may
        // suffice; but if too long then the message itself will inform the user to look in
        // the project folder for the "Rebuild Log.txt" file
		if (len > 1200 && !gbIsUnstructuredData)
		{
			// trim the excess and add the string telling user to check visually 
			// & of the existence of Rebuild Log.txt
			fixesStr = fixesStr.Left(1200);
			fixesStr = MakeReverse(fixesStr);
			int nFound = fixesStr.Find(_T(' '));
			if (nFound != -1)
			{
				fixesStr = fixesStr.Mid(nFound);
			}
			fixesStr = MakeReverse(fixesStr);
			wxString appendStr;
			// IDS_APPEND_MSG
			appendStr = _(
" ... for additional later locations needing manual editing please check the document visually. The full list has been saved in your project folder in the file \"Rebuild Log.txt\"");
			fixesStr += appendStr;
		}
		else if (!gbIsUnstructuredData)
		{
			wxString appendLogStr;
			// IDS_APPEND_LOG_REF
			appendLogStr = _(
"    This list has been saved in your project folder in the file \"Rebuild Log.txt\"");
			fixesStr += appendLogStr; // tell the user about the log file:  Rebuild Log.txt
		}
        // display the message - in the case of unstructured data, there will be no list of
        // locations and the user will just have to search the document visually
		wxMessageBox(fixesStr, _T(""), wxICON_INFORMATION);
	}

	// this is where we need to have the layout updated. We will do the whole lot, that is,
	// destroy and recreate both piles and strips
#ifdef _NEW_LAYOUT
	GetLayout()->RecalcLayout(pApp->m_pSourcePhrases, create_strips_and_piles);
#else
	GetLayout()->RecalcLayout(pApp->m_pSourcePhrases, create_strips_and_piles);
#endif

	gpApp->m_pActivePile = pView->GetPile(gpApp->m_nActiveSequNum);
	GetLayout()->m_docEditOperationType = retokenize_text_op;
	return count;
}

///////////////////////////////////////////////////////////////////////////////
/// \return		nothing
/// \param		nFirstSequNum	-> the number to use for the first source phrase in pList
/// \remarks
/// Called from: the Doc's TokenizeText().
/// Fixes the m_nSequNumber member of the source phrases in the current document
/// starting with nFirstSequNum in pList, so that all the remaining source phrases' sequence
/// numbers continue in numerical sequence (ascending order) with no gaps. 
/// This function differs from AdjustSequNumbers() in that AdjustSequNumbers() effects its 
/// changes only on the pList passed to the function; in UpdateSequNumbers() the current 
/// document's source phrases beginning with nFirstSequNum are set to numerical sequence 
/// through to the end of the document.
/// BEW changed 16Jul09 to have a second parameter which defaults to NULL, but otherwise is
/// a pointer to the SPList on which the updating is to be done. This allows the function
/// to be used on sublists which have to be processed by RecalcLayout(), as when doing a
/// range print - the refactored view requires that the partner piles be accessible by a
/// sequence number, and this will only work if the sublist of CSourcePhrase instances has
/// the element's m_nSequNumber values reset so as to be 0-based and numbered from the
/// first in the sublist. pOtherList can be set to any list, including of course, the
/// m_pSourcePhrases list, if the former contents of that list have been stored elsewhere
/// beforehand
///////////////////////////////////////////////////////////////////////////////
void CAdapt_ItDoc::UpdateSequNumbers(int nFirstSequNum, SPList* pOtherList)
{
	CAdapt_ItApp* pApp = &wxGetApp();
	wxASSERT(pApp != NULL);
	SPList* pList;
	if (pOtherList == NULL)
		// use the normal list which defines the whole document
		pList = pApp->m_pSourcePhrases;
	else
        // use some other list, typically a sublist with fewer elements, that stores
        // temporary subset of (shallow) copies of the main list's CSourcePhrase instances
		pList = pOtherList;

	// get the first
	SPList::Node* pos = pList->Item(nFirstSequNum);
	wxASSERT(pos != NULL);
	CSourcePhrase* pSrcPhrase = (CSourcePhrase*)pos->GetData();
	pos = pos->GetNext();
	wxASSERT(pSrcPhrase); 
	pSrcPhrase->m_nSequNumber = nFirstSequNum;
	int index = nFirstSequNum;

	while (pos != 0)
	{
		pSrcPhrase = (CSourcePhrase*)pos->GetData();
		pos = pos->GetNext();
		wxASSERT(pSrcPhrase); 
		index++; // next sequence number
		pSrcPhrase->m_nSequNumber = index;
	}
}

///////////////////////////////////////////////////////////////////////////////
/// \return		nothing
/// \param		event	-> a wxCommandEvent associated with the wxID_NEW identifier
/// \remarks
/// Called from: the doc/view framework when File | New menu item is selected. In the wx
/// version this override sets the bUserSelectedFileNew flag to TRUE and then simply calls
/// the App's OnFileNew() method.
///////////////////////////////////////////////////////////////////////////////
void CAdapt_ItDoc::OnFileNew(wxCommandEvent& event)
{
	// called when File | New menu item selected specifically by user
	// Note: The App's OnInit() skips this and calls pApp->OnFileNew
	// directly, so we can initialize our flag here.
	CAdapt_ItApp* pApp = &wxGetApp();
	wxASSERT(pApp != NULL);
	pApp->bUserSelectedFileNew = TRUE; // causes the view->OnCreate() to reinit the KBs
	pApp->OnFileNew(event);
}

///////////////////////////////////////////////////////////////////////////////
/// \return		nothing
/// \param      event   -> the wxUpdateUIEvent that is generated when the File Menu
///                        is about to be displayed
/// \remarks
/// Called from: The wxUpdateUIEvent mechanism when the associated menu item is selected,
/// and before the menu is displayed.
/// If Vertical Editing is in progress the Split Document menu item is disabled and this
/// handler returns immediately. Otherwise, it enables the Split Document command on the 
/// Tools menu if a document is open, unless the app is in Free Translation Mode.
/// BEW modified 13Nov09, don't allow user with read-only access to cause document
/// change of this kind on the remote user's machine
///////////////////////////////////////////////////////////////////////////////
void CAdapt_ItDoc::OnUpdateSplitDocument(wxUpdateUIEvent& event)
{
	if (gpApp->m_bReadOnlyAccess)
	{
		event.Enable(FALSE);
		return;
	}
	if (gbVerticalEditInProgress)
	{
		event.Enable(FALSE);
		return;
	}
	if (gpApp->m_bFreeTranslationMode)
	{
		event.Enable(FALSE);
		return;
	}
    // BEW modified 03Nov05: let it be enabled for a dirty doc, but check for dirty flag
    // set and if so do an automatic save before attempting the split
	if (gpApp->m_pKB != NULL && gpApp->m_pSourcePhrases->GetCount() > 0)
		event.Enable(TRUE);
	else
		event.Enable(FALSE);	
}

///////////////////////////////////////////////////////////////////////////////
/// \return		nothing
/// \param      event   -> the wxUpdateUIEvent that is generated when the Tools Menu
///                        is about to be displayed
/// \remarks
/// Called from: The wxUpdateUIEvent mechanism when the associated menu item is selected,
/// and before the menu is displayed.
/// If Vertical Editing is in progress or if the app is in Free Translation mode, the 
/// Join Documents menu item is disabled and this handler returns immediately. It 
/// enables the Join Document command if a document is open, otherwise it
/// disables the command.
/// BEW added 13Nov09, don't allow local user with read-only access to a remote project
/// folder to make document changes of this kind on the remote machine
///////////////////////////////////////////////////////////////////////////////
void CAdapt_ItDoc::OnUpdateJoinDocuments(wxUpdateUIEvent& event)
{
	if (gpApp->m_bReadOnlyAccess)
	{
		event.Enable(FALSE);
		return;
	}
	if (gbVerticalEditInProgress)
	{
		event.Enable(FALSE);
		return;
	}
	if (gpApp->m_bFreeTranslationMode)
	{
		event.Enable(FALSE);
		return;
	}
	if (gpApp->m_pKB != NULL && gpApp->m_pSourcePhrases->GetCount() > 0)
		event.Enable(TRUE);
	else
		event.Enable(FALSE);	
}

///////////////////////////////////////////////////////////////////////////////
/// \return		nothing
/// \param      event   -> the wxUpdateUIEvent that is generated when the Tools Menu
///                        is about to be displayed
/// \remarks
/// Called from: The wxUpdateUIEvent mechanism when the associated menu item is selected,
/// and before the menu is displayed.
/// Disables the Move Document command on the Tools menu and returns immediately if
/// vertical editing is in progress, or if the application is in Free Translation Mode.
/// This event handler enables the Move Document command if a document is open, otherwise
/// it disables the command.
/// BEW added 13Nov09, don't allow local user with read-only access to a remote project
/// folder to make document changes of this kind on the remote machine
///////////////////////////////////////////////////////////////////////////////
void CAdapt_ItDoc::OnUpdateMoveDocument(wxUpdateUIEvent& event)
{
	if (gpApp->m_bReadOnlyAccess)
	{
		event.Enable(FALSE);
		return;
	}
	if (gbVerticalEditInProgress)
	{
		event.Enable(FALSE);
		return;
	}
	if (gpApp->m_bFreeTranslationMode)
	{
		event.Enable(FALSE);
		return;
	}
	if (gpApp->m_pKB != NULL && gpApp->m_bBookMode && gpApp->m_nBookIndex != -1 
			&& !gpApp->m_bDisableBookMode)
		event.Enable(TRUE);
	else
		event.Enable(FALSE);	
}

///////////////////////////////////////////////////////////////////////////////
/// \return		TRUE if there is a SF marker in the passed in markers string which for the 
///				SfmSet is determined to be a marker which should halt the scanning of successive 
///				pSrcPhase instances during doc rebuild for filtering out a marker and its content 
///				which are required to be filtered, FALSE otherwise.
/// \param		sfmSet	-> indicates which SFM set is to be considered active for the 
///							LookupSFM() call
/// \param		markers	-> the pSrcPhrase->m_markers string from the sourcephrase being 
///							currently considered
/// \remarks
/// Called from: the Doc's ReconstituteAfterFilteringChange(). 
/// Implements the protocol for determining when to stop scanning in a variety of situations 
///    - markers might contain filtered material (this must stop scanning because we can't 
///       embed filtered material),
///    - or it may contain a marker which Adapt It should ignore (eg. one with TextType == none), 
///    - or an unknown marker - these always halt scanning, 
///    - or an embedded content marker within an \x (cross reference section) or \f (footnote) 
///         or \fe (endnote) section - these don't halt scanning, 
///    - or an inLine == FALSE marker - these halt scanning.
///////////////////////////////////////////////////////////////////////////////
bool CAdapt_ItDoc::IsEndingSrcPhrase(enum SfmSet sfmSet, wxString& markers)
{
	int nFound = markers.Find(gSFescapechar);
	if (nFound == -1)
		return FALSE;

	int len = markers.Length();
    // wx version note: Since we require a read-only buffer we use GetData which just
    // returns a const wxChar* to the data in the string.
	const wxChar* pBuff = markers.GetData();
	wxChar* pEnd;
	pEnd = (wxChar*)pBuff + len; // point at the ending null
	wxASSERT(*pEnd == _T('\0')); // whm added, not in MFC
	wxChar* pBufStart = (wxChar*)pBuff;
	wxChar* pFound = pBufStart + nFound; // point at the backslash of the marker
	wxASSERT(pFound < pEnd);

	wxString fltrMkr(filterMkr); // filterMkr is a literal string defined at 
								 // start of the doc's .cpp file
	int lenFilterMkr = fltrMkr.Length();

    // if the first marker in markers is \~FILTER then we must be at the end of the section
    // for filtering out because we can't nest filtered material - so check out this case
    // first
	if (wxStrncmp(pFound,filterMkr,lenFilterMkr) == 0)
	{
		// it's a \~FILTER marker
		return TRUE;
	}

    // if we've been accumulating filtered sections and filtering them out, we could get to
    // a situation where the marker we want to be testing is not the one first one found
    // (which could be an unfiltered endmarker from much earlier in the list of
    // sourcephrases), but to test the last unfiltered marker - so we must check for this,
    // and if so, make the marker we are examining be that last one) (The following block
    // should only be relevant in only highly unusual circumstances)
	wxString str2 = markers;
	str2 = MakeReverse(str2);
	int itemLen2 = 0;
	int lenRest = len; // initialize to whole length
	int nFound2 = str2.Find(gSFescapechar);
	if (nFound2 > 0)
	{
		// there's a marker
		nFound2++; // include the backslash in the leftmost part (of the reversed 
				   // markers string)
		lenRest -= nFound2; // lenRest will be the offset of this marker, if we 
                    // determine we need to use it, later when we reverse back to normal
                    // order
		wxString endStr = str2.Left(nFound2); // this could be something like 
											  // "\v 1 " reversed
		endStr = MakeReverse(endStr); // restore natural order
		int len = endStr.Length(); // whm added 18Jun06
        // wx version note: Since we require a read-only buffer we use GetData which just
        // returns a const wxChar* to the data in the string.
		const wxChar* pChar = endStr.GetData();
		wxChar* pCharStart = (wxChar*)pChar;
		wxChar* pEnd;
		pEnd = (wxChar*)pChar + len; // whm added 18Jun06
		wxASSERT(*pEnd == _T('\0')); // whm added 18Jun06
		itemLen2 = ParseMarker(pCharStart);
        // determine if the marker at the end is actually the one at pFound, if so, we can
        // exit this block and just go on using the one at pFound; if not, we have to check
        // the marker at the end is not a filter endmarker, and providing it's not, we
        // would use the one at the end in subsequent code following this block
		if (wxStrncmp(pFound,pCharStart,itemLen2 + 1) == 0) // include space after the marker 
															// in the comparison
		{
			// the two are the same marker so no adjustment is needed
			;
		}
		else
		{
			// they are different markers, so we have to make sure we didn't just locate 
			// a \~FILTER* marker
			wxString theLastMkr(pCharStart,itemLen2);
			if (theLastMkr.Find(fltrMkr) != -1)
			{
                // theLastMkr is either \~FILTER or \~FILTER*, so there is no marker at the
                // end of m_markers which we need to be considering instead, so no
                // adjustment is needed
				;
			}
			else
			{
                // this marker has to be used instead of the one at the offset passed in;
                // to effect the required change, all we need do is exit this block with
                // pFound pointing at the marker at the end
				pFound = pBufStart + lenRest;
				wxASSERT(*pFound == gSFescapechar);
			}
		}
	}

	// get the marker which is at the backslash
	int itemLen = ParseMarker(pFound);
	wxString mkr(pFound,itemLen); // this is the whole marker, including its backslash
	wxString bareMkr = mkr;
	bareMkr = MakeReverse(bareMkr);
    // we must allow that the mkr which we have found might be an endmarker, since we could
    // be parsing across embedded content material, such as a keyword designation ( which
    // has a \k followed later by \k*), or italics, bold, or similar type of thing - these
    // markers don't indicate that parsing should end, so we have to allow for endmarkers
    // to be encountered
	if (bareMkr[0] == _T('*'))
	{
		bareMkr = bareMkr.Mid(1); // remove final * off an endmarker
	}
	bareMkr = MakeReverse(bareMkr); // restore normal order
	bareMkr = bareMkr.Mid(1); // remove initial backslash, so we can use it for Lookup

	// do lookup
	SfmSet saveSet = gpApp->gCurrentSfmSet; // save current set temporarily, 
											// as sfmSet may be different
	gpApp->gCurrentSfmSet = sfmSet; // install the set to be used - as passed in
	USFMAnalysis* analysis = LookupSFM(bareMkr); // internally uses gUserSfmSet
	if (analysis == NULL)
	{
        // this must be an unknown marker - we deem these all inLine == FALSE, so this
        // indicates we are located at an ending sourcephrase
		gpApp->gCurrentSfmSet = saveSet; // restore earlier setting
		//markers.ReleaseBuffer(); // whm moved to main block above
		return TRUE;
	}
	else
	{
		// this is a known marker in the sfmSet marker set, so check out inLine and
		// TextType 
		gpApp->gCurrentSfmSet = saveSet; // restore earlier setting
		//markers.ReleaseBuffer(); // whm moved to main block above
		if (analysis->inLine == FALSE)
			return TRUE; // it's not an inLine marker, 
						 // so it must end the filtering scan
		else
		{
			// its an inLine == TRUE marker, so we have to check if TextType == none
			if (analysis->textType == none)
			{
				// it's not the kind of marker that halts scanning
				return FALSE;
			}
			else
			{
                // it's something other than type none, so it must halt scanning - except
                // if the marker is one of the footnote or cross reference embedded content
                // markers - so check out those possibilities (note, in the compound test
                // below, if the last test were absent, then \f or \x would each satisfy
                // one of the first two tests and there would be no halt - but these, if
                // encountered, must halt the parse and so the test checks for shortMkr
                // being different than bareMkr (FALSE for \x or \f, but TRUE for any of
                // \xo, \xk, etc or \fr, \fk, \fv, fm, etc)) The OR test's RHS part is for
                // testing for embedded content markers within an endnote - these don't
                // halt scanning either.
				wxString shortMkr = bareMkr.Left(1);
				if (((shortMkr == _T("f") || shortMkr == _T("x")) && shortMkr != bareMkr) ||
					((shortMkr == _T("f")) && (bareMkr != _T("fe"))))
				{
					// its an embedded content marker of a kind which does 
					// not halt scanning
					return FALSE;
				}
				else
				{
					// it must halt scanning
					return TRUE;
				}
			}
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
/// \return		offset of the marker which is to be filtered (ie. offset of its backslash) if markers 
///				is a non-empty string containing a SFM which is designated as to be filtered (by being 
///				listed in filterList string) and it is not preceded by \~FILTER; otherwise it returns -1.
/// \param		sfmSet			-> indicates which SFM set is to be considered active for the 
///									LookupSFM() call
/// \param		markers			-> the pSrcPhrase->m_markers string from the sourcephrase being 
///									currently considered
/// \param		filterList		-> the list of markers to be filtered out, space delimited, including 
///									backslashes... (the list might be just those formerly unfiltered 
///									and now designated to be filtered, or in another context (such as 
///									changing the SFM set) it might be the full set of markers designated 
///									for filtering - which is the case is determined by the caller)
/// \param		wholeMkr		<- the SFM, including backslash, found to be designated for filtering out
/// \param		wholeShortMkr	<- the backslash and first character of wholeMkr (useful, when wholeMkr 
///									is \x or \f)
/// \param		endMkr			<- the endmarker for wholeMkr, or an empty string if it does not potentially 
///									take an endmarker
/// \param		bHasEndmarker	<- TRUE when wholeMkr potentially takes an endmarker (determined by Lookup())
/// \param		bUnknownMarker	<- TRUE if Lookup() determines the SFM does not exist in the sfmSet marker set
/// \param		startAt			-> the starting offset in the markers string at which the scanning is to be 
///									commenced
/// \remarks
/// Called from: the Doc's ReconstituteAfterFilteringChange().
/// Determines if a source phrase's m_markers member contains a marker to be filtered, and if so, returns
/// the offset into m_markers where the marker resides.
///////////////////////////////////////////////////////////////////////////////
int CAdapt_ItDoc::ContainsMarkerToBeFiltered(enum SfmSet sfmSet, wxString markers, 
				wxString filterList, wxString& wholeMkr, wxString& wholeShortMkr, 
				wxString& endMkr, bool& bHasEndmarker, bool& bUnknownMarker, 
				int startAt)
{
	int offset = startAt;
	if (markers.IsEmpty())
		return -1;
	bHasEndmarker = FALSE; // default
	bUnknownMarker = FALSE; // default
	int len = markers.Length();
    // wx version note: Since we require a read-only buffer we use GetData which just
    // returns a const wxChar* to the data in the string.
	const wxChar* pBuff = markers.GetData();
	wxChar* pEnd;
	pEnd = (wxChar*)pBuff + len; // point at the ending null
	wxASSERT(*pEnd == _T('\0')); // whm added 18Jun06, not in MFC
	wxChar* pBufStart = (wxChar*)pBuff;
	wxChar* ptr = pBufStart;
	wxChar* pFound = pBufStart; // pointer returned from _tcsstr() 
								// (ie. strstr() or wcsstr()), null if unfound
	pFound += offset; // get the scan start location (offset will be reused below, 
                      // so don't rely on it staying this present value)
	wxString fltrMkr(filterMkr); // filterMkr is a literal string defined at start
								 // of the doc's .cpp file
	int lenFilterMkr = fltrMkr.Length();
	int lenFilterMkrEnd = lenFilterMkr + 1;
	wxChar backslash[2] = {gSFescapechar,_T('\0')};
	int itemLen;

    // scan the buffer, looking for a filterable marker not bracketed by \~FILTER ...
    // \~FILTER* (because any between those is already filtered out)
	while ((pFound = wxStrstr(pFound, backslash)) != NULL)
	{	
		// we have come to a backslash
		ptr = pFound;
		itemLen = ParseMarker(ptr);
		wxString mkr(ptr,itemLen); // this is the whole marker, 
								   // including its backslash
        // if we have found a bracketing \~FILTER beginning marker, we have located an
        // already filtered section of material - this is of no interested, so we have to
        // jump to the matching \~FILTER* string (there will always be one) and continue
        // our loop from whatever follows that
		if (mkr == fltrMkr)
		{
			// we have found bracketed filtered material, so skip it
			pFound++; // skip the intial backslash
			pFound = wxStrstr(pFound, filterMkrEnd);
			if (pFound == NULL)
			{
				return -1;
			}
			pFound += lenFilterMkrEnd; // point past the \~FILTER* marker
			continue; // iterate
		}
		else
		{
            // it's either the endmarker for previous unfiltered material (which is to
            // remain unfiltered), or it is potentially a marker which might be one for
            // filtering out
			int nFound = mkr.Find(_T('*'));
			if (nFound > 0)
			{
				// it is an endmarker, so we are not interested in it
				pFound++;
				continue;
			}
			else
			{
				// it could be a marker which is to be filtered, so check it out
				wxString mkrPlusSpace = mkr + _T(' '); // prevent spurious matches,
													   // eg. \h finding \hr
				nFound = filterList.Find(mkrPlusSpace);
				if (nFound == -1)
				{
					// it's not in the list of markers designated as to be filtered, 
					// so keep iterating
					pFound++;
					continue;
				}
				else
				{
					// this marker is to be filtered, so set up the parameters to 
					// be returned etc
					offset = pFound - pBuff;
					wxASSERT(offset >= 0);
					wholeMkr = mkr;
					wholeShortMkr = wholeMkr.Left(2);

					// get its SFM characteristics, whether it has an endmarker, 
					// and whether it is unknown
					SfmSet saveSet = gpApp->gCurrentSfmSet; // save current set 
										// temporarily, as sfmSet may be different
					gpApp->gCurrentSfmSet = sfmSet; // install the set to be used 
													// - as passed in
					wxString bareMkr = wholeMkr.Mid(1); // lop off the backslash
					USFMAnalysis* analysis = LookupSFM(bareMkr); // uses gCurrentSfmSet
					if (analysis == NULL)
					{
						// this must be an unknown marker designated for filtering by 
						// the user in the GUI
						bUnknownMarker = TRUE;
						bHasEndmarker = FALSE; // unknown markers NEVER have endmarkers
						endMkr.Empty();
						gpApp->gCurrentSfmSet = saveSet; // restore earlier setting
						return offset; // return its offset
					}
					else
					{
						// this is a known marker in the sfmSet marker set
						endMkr = analysis->endMarker;
						bHasEndmarker = !endMkr.IsEmpty();
						endMkr = gSFescapechar + endMkr; // add the initial backslash
						gpApp->gCurrentSfmSet = saveSet; // restore earlier setting
						if (analysis->filter)
							return offset; // it's filterable, so return its offset
						else
                            // it's either not filterable, or we've forgotten to update the
                            // filter member of the USFMAnalysis structs prior to calling
                            // this function
							return -1;
					}
				}
			}
		}
	}
	// didn't find a filterable marker that was not already filtered
	wholeShortMkr.Empty();
	wholeMkr.Empty();
	endMkr.Empty();
	return -1;
}

///////////////////////////////////////////////////////////////////////////////
/// \return		a copy of the wxString which is the appropriate nav text message 
///             for the passed in CSourcePhrase instance (typically returned to the
///             m_inform member in the caller)
/// \param		pSrcPhrase	-> the CSourcePhrase instance which is to have its 
///                            m_inform member reconstructed, modified, partly or wholely,
///                            by the set of markers changed by the user in the GUI)
/// \remarks
/// Called from: the App's AddBookIDToDoc(), the Doc's ReconstituteAfterFilteringChange(),
/// the View's OnRemoveFreeTranslationButton().
/// Re-composes the navigation text that is stored in a source phrase's m_inform member. 
/// The idea behind this function is to get the appropriate m_inform text redone when
/// rebuilding the document - as when filtering changes are made, or a change of SFM set
/// which has the side effect of altering filtering settings as well, or the insertion of a
/// sourcephrase with an \id in m_markers and a 3-letter book ID code in the m_key member.
///////////////////////////////////////////////////////////////////////////////
wxString CAdapt_ItDoc::RedoNavigationText(CSourcePhrase* pSrcPhrase)
{
	wxString strInform; 
	strInform.Empty();

	// get the current m_markers contents
	wxString markersStr = pSrcPhrase->m_markers;
	if (markersStr.IsEmpty() || (markersStr.Find(gSFescapechar) == -1))
		return strInform;

	// there is something more to be handled
	wxString f(filterMkr);
	//int fltrMkrLen = f.Length(); // unused
	int markersStrLen = markersStr.Length() + 1; // allow for null at end
	const wxChar* ptr = NULL;
	int mkrLen = 0;
	int curPos = 0;
	bool bFILTERwasLastMkr = FALSE;
	while ((curPos = FindFromPos(markersStr,gSFescapechar,curPos)) != -1)
	{
        // wx version note: Since we require a read-only buffer we use GetData which just
        // returns a const wxChar* to the data in the string.
		ptr = markersStr.GetData();
		wxChar* pEnd;
		pEnd = (wxChar*)ptr + markersStrLen - 1; // whm added -1 compensates for 
												 // increment of markerStrLen above
		wxASSERT(*pEnd == _T('\0')); // whm added 18Jun96
		wxChar* pBufStart = (wxChar*)ptr;
		mkrLen = ParseMarker(pBufStart + curPos);
		wxASSERT(mkrLen > 0);
		wxString bareMkr(ptr + curPos + 1, mkrLen - 1); // no gSFescapechar 
														// (but may be an endmarker)
		curPos++; // increment, so the loop's .Find() test will advance curPos 
				  // to the next marker
		wxString nakedMkr = bareMkr;
		nakedMkr = MakeReverse(nakedMkr);
		bool bEndMarker = FALSE;
		if (nakedMkr[0] == _T('*'))
		{
			// it's an endmarker
			nakedMkr = nakedMkr.Mid(1);
			bEndMarker = TRUE;
		}
		nakedMkr = MakeReverse(nakedMkr);
		wxString wholeMkr = gSFescapechar;
		wholeMkr += bareMkr;
		if (wholeMkr == filterMkr)
		{
			bFILTERwasLastMkr = TRUE; // suppresses forming nav text for 
									// the next marker (coz it's filtered)
			continue; // skip \~FILTER
		}
		if (wholeMkr == filterMkrEnd)
		{
			bFILTERwasLastMkr = FALSE; // this will re-enable possible forming 
									   // of nav text for the next marker
			continue; // skip \~FILTER*
		}
		
		// we only show navText for markers which are not endmarkers, and not filtered
		if (bFILTERwasLastMkr)
			continue; // this marker was preceded by \~FILTER, 
					  // so it must not have nav text formed
		if (!bEndMarker)
		{
			USFMAnalysis* pAnalysis = LookupSFM(nakedMkr);
			wxString navtextStr;
			if (pAnalysis)
			{
				// the marker was found, so get the stored navText
				bool bInform = pAnalysis->inform;
				// only those markers whose inform member is TRUE are to be used
				// for forming navText
				if (bInform)
				{
					navtextStr = pAnalysis->navigationText;
					if (!navtextStr.IsEmpty())
					{
//a:						
						if (strInform.IsEmpty())
						{
							strInform = navtextStr;
						}
						else
						{
							strInform += _T(' ');
							strInform += navtextStr;
						}
					}
				}
			}
			else
			{
                // whm Note 11Jun05: I assume that an unknown marker should not appear in
                // the navigation text line if it is filtered. I've also modified the code
                // in AnalyseMarker() to not include the unknown marker in m_inform when
                // the unknown marker is filtered there, and it seems that would be
                // appropriate here too. If Bruce thinks the conditional call to
                // IsAFilteringUnknownSFM is not appropriate here the conditional I've
                // added should be removed; likewise the parallel code I've added near the
                // end of AnalyseMarker should be evaluated for appropriateness. 
                // ( <--Bill's addition is fine, BEW 15Jun05)
				if (!IsAFilteringUnknownSFM(nakedMkr))
				{
					// the marker was not found, so form a ?mkr? string instead
					navtextStr = _T("?");
					navtextStr += gSFescapechar; // whm added 10Jun05
					navtextStr += nakedMkr;
                    // BEW commented out next line. It fails for a naked marker such as
                    // lb00296b.jpg which appeared in Bob Eaton's data as a picture
                    // filename; what happens is that ? followed by space does not get
                    // appended, but the IDE shows the result as "?\lb00296b.jpg" and in
                    // actual fact the navtextStr buffer contains that plus a whole long
                    // list of dozens of arbitrary memory characters (garbage) which
                    // subsequently gets written out as navText - Ugh! So I've had to place
                    // the required characters in the buffer explicitly.
					navtextStr += _T("? ");
				}
				// code block at goto a; copied down here to get rid of goto a; and label 
                // (and gcc warning)
				if (strInform.IsEmpty())
				{
					strInform = navtextStr;
				}
				else
				{
					strInform += _T(' ');
					strInform += navtextStr;
				}
			}
		} // end block for !bEndMarker

	} // end loop for searching for all markers to be used for navText
	return strInform;
}

///////////////////////////////////////////////////////////////////////////////
/// \return		nothing
/// \param		pList	<- the list of source phrases
/// \remarks
/// Called from: the Doc's ReconstituteAfterFilteringChange() and
/// ReconstituteAfterPunctuationChange().
/// Deletes the source phrase instances held in pList, but retains the empty list. It is
/// called during the rebuilding of the document after a filtering or punctuation change.
/// BEW added 23May09: since it is called only from the above two functions, and both of
/// those when they return to the caller will have the view updated by a RecalcLayout()
/// call with the enum parameter valoue of create_piles_create_strips, we do not need to
/// have DeletePartnerPile() called when each of the DeleteSingleSrcPhrase(() calls are
/// made in the loop below; therefore we specify the FALSE parameter for the call of the
/// latter function in order to suppress deletion of the partner pile (as RecalcLayout()
/// will do it much more efficiently, en masse, and speedily).
/// 
/// Note: if ever we use this function elsewhere, and need the partner pile deletion to
/// work there, then we will need to alter the signature to become:
/// (SPList*& pList, bool bDoPartnerPileDeletionAlso = TRUE) instead
///////////////////////////////////////////////////////////////////////////////
void CAdapt_ItDoc::DeleteListContentsOnly(SPList*& pList)
{
	// DeleteListContentsOnly is a useful utility in the rebuilding of the doc
	SPList::Node* pos = pList->GetFirst();
	CSourcePhrase* pSrcPh;
	while (pos != NULL)
	{
		pSrcPh = (CSourcePhrase*)pos->GetData();
		pos = pos->GetNext();
		DeleteSingleSrcPhrase(pSrcPh,FALSE); // no need to bother to delete the 
											 // partner pile
	}
	pList->Clear();
}

///////////////////////////////////////////////////////////////////////////////
/// \return		a wxString with any filtered brackets removed from around its filtered 
///				information
/// \param		str		-> a wxString containing filtered information enclosed within 
///							\~FILTER ... \~FILTER* filtering brackets
/// \remarks
/// Called from: the Doc's ReconstituteAfterFilteringChange(), GetUnknownMarkersFromDoc(),
/// the View's RebuildSourceText(), DoPlacementOfMarkersInRetranslation(), 
/// RebuildTargetText(), DoExportInterlinearRTF(), GetMarkerInventoryFromCurrentDoc(),
/// and CViewFilteredMaterialDlg::InitDialog().
/// Removes any \~FILTER and \~FILTER* brackets from a string. The information the was
/// previously bracketed by these markers remains intact within the string.
///////////////////////////////////////////////////////////////////////////////
wxString CAdapt_ItDoc::RemoveAnyFilterBracketsFromString(wxString str) // whm added 6Apr05
{
    // ammended 29Apr05 to remove multiple sets of filter brackets from str. Previously the
    // function only removed the first set of brackets found in the str.
    // whm revised 2Oct05 to more properly handle the deletion of spaces upon/after the
    // removal of the filter brackets
	int mkrPos = str.Find(filterMkr);
	int endMkrPos = str.Find(filterMkrEnd);
	while (mkrPos != -1 && endMkrPos != -1 && endMkrPos > mkrPos)
	{
		str.Remove(endMkrPos, wxStrlen_(filterMkrEnd));
        // after deleting the end marker, endMkrPos will normally point to a following
        // space whenever the filtered material is medial to the string. In such cases we
        // want to also eliminate the following space. The only time there may not be a
        // space following the end marker is when the filtered material was the last item
        // in m_markers. In this case endMkrPos would no longer index a character within
        // the string after the deletion of the end marker.
		if (endMkrPos < (int)str.Length() && str.GetChar(endMkrPos) == _T(' '))
			str.Remove(endMkrPos,1);
		str.Remove(mkrPos, wxStrlen_(filterMkr));
		// after deleting the beginning marker, mkrPos should point to the space that
		// followed the beginning filter bracket marker - at least for well formed 
		// filtered material. Before deleting that space, however, we check to insure
		// it is indeed a space.
		if (str.GetChar(mkrPos) == _T(' '))
			str.Remove(mkrPos,1);
		// set positions for any remaining filter brackets in str
		mkrPos = str.Find(filterMkr);
		endMkrPos = str.Find(filterMkrEnd);
	}
	// change any left-over multiple spaces to single spaces
	int dblSpPos = str.Find(_T("  "));
	while (dblSpPos != -1)
	{
		str.Remove(dblSpPos, 1);
		dblSpPos = str.Find(_T("  "));
	}
	return str;
}

///////////////////////////////////////////////////////////////////////////////
/// \return		a wxString which is a copy of the filtered SFM or USFM next found  composed of
///				\~FILTER ... \~FILTER* plus the marker and associated string content in between,
///				or an empty string if one is not found
/// \param		markers	-> references the content of a CSourcePhrase instance's m_markers member
/// \param		offset	-> character offset to where to commence the Find operation for the
///						   \~FILTER marker
/// \param		nStart	<- offset (from beginning of markers) to the next \~FILTER instance found, 
///						   if none is found it is set to the offset value (so a Left() call can 
///						   extract any preceding non-filtered text stored in markers)
/// \param		nEnd	<- offset (ditto) to the first character past the matching \~FILTER* 
///						   (& space) instance, if none is found it is set to the offset value 
///						   (so a Mid() call can get the remainder)
/// \remarks
/// Called from: the Doc's ReconstituteAfterFilteringChange().
/// Copies from a m_markers string the whole string representing filtered information,
/// i.e., the filtering brackets \~FILTER ... \~FILTER* plus the marker and associated
/// string content in between. Offsets are returned in nBegin and nEnd that enable the
/// function to be called repeatedly within a while loop until no additional filtered
/// material is found.
/// The nStart and nEnd values can be used by the caller, along with Mid(), to extract the
/// filtered substring (including bracketing FILTER markers); nStart, along with Left() can
/// be used by the caller to extract whatever text precedes the \~FILTER instance (eg. to
/// store it in another CSourcePhrase instance's m_markers member), and nEnd, along with
/// Mid(), can be used by the caller to extract the remainder (if any) - which could be
/// useful in the caller when unfiltering in order to update m_markers after unfiltering
/// has ceased for the current CSourcePhrase instance. (To interpret the nStart and nEnd
/// values correctly in the caller upon return, the returned CString should be checked to
/// determine if it is empty or not.) The caller also can use the nEnd value to compute an
/// updated offset value for iterating the function call to find the next filtered marker.
///////////////////////////////////////////////////////////////////////////////
wxString CAdapt_ItDoc::GetNextFilteredMarker(wxString& markers, int offset, 
											 int& nStart, int& nEnd)
{
	wxString mkrStr;
	mkrStr.Empty();
	if (offset < 0 || offset > 10000)
		offset = 0; // assume it's a bad offset value, so start from zero

	// find the next \~FILTER marker starting from the offset (there may be no more)
	nStart = offset; // default 'not found' value
	nEnd = offset; // ditto
	int nFound = FindFromPos(markers,filterMkr,offset);
	if (nFound == -1)
	{
		// no \~FILTER marker was found at offset or beyond
		return mkrStr;
	}
	else
	{
		// we found one, so get the metrics calculated and the marker parsed
		nStart = nFound;
		wxString theRest = markers.Mid(nStart); // \~FILTER\marker followed 
												// by its content etc.
		wxString f(filterMkr); // so I can get a length calculation done 
							   // (this may be localized later)
		int len = f.Length() + 1; // + 1 because \~FILTER is always 
								  // followed by a space
		theRest = theRest.Mid(len); // \marker followed by its content etc.
		int len2 = theRest.Length();
        // wx version note: Since we require a read-only buffer we use GetData which just
        // returns a const wxChar* to the data in the string.
		const wxChar* ptr = theRest.GetData();
		wxChar* pEnd;
		pEnd = (wxChar*)ptr + len2; // whm added 19Jun06
		wxChar* pBufStart = (wxChar*)ptr;
		wxASSERT(*pEnd == _T('\0'));// whm added 19Jun06
		int len3 = ParseMarker(pBufStart);
		mkrStr = theRest.Left(len3);
		wxASSERT(mkrStr[0] == gSFescapechar); // make sure we got a valid marker

        // now find the end of the matching \~FILTER* endmarker - we must find this for the
        // function to succeeed; if we don't, then we must throw away the mkrStr value (ie.
        // empty it) and revert to the nStart and nEnd values for a failure
		int nStartFrom = nStart + len + len3; // offset to character after the marker 
											  // in mkrStr (a space)
		nFound = FindFromPos(markers,filterMkrEnd,nStartFrom);
		if (nFound == -1)
		{
			// no matching \~FILTER* marker; this is an error condition, so bail out
			nStart = nEnd = offset;
			mkrStr.Empty();
			return mkrStr;
		}
		else
		{
            // we found the matching filter marker which brackets the end, so get offset to
            // first character beyond it and its following space & we are done
			wxString fEnd(filterMkrEnd);
			len3 = fEnd.Length() + 1;
			nEnd = nFound + len3;
		}
	}
	return mkrStr;
}

///////////////////////////////////////////////////////////////////////////////
/// \return		nothing
/// \param	oldSet				-> indicates which SFM set was active before the user changed 
///									to a different one
/// \param	newSet				-> indicates which SFM set the user changed to
/// \param	oldFilterMarkers	-> the list of markers that was filtered out, space delimited, 
///									including backslashes, which were in effect when the oldSet 
///									was the current set,
/// \param	newFilterMarkers	-> the list of markers to be filtered out, along with their 
///									content, now that the newSet has become current
/// \param	secondPassFilterMkrs <- a list of the markers left after common ones have been 
///									removed from the newFilterMarkers string -- these markers 
///									are used on the second pass which does the filtering
/// \param	pass				-> an enum value, which can be first_pass or second_pass; the 
///									first pass through the document does unfiltering within 
///									the context of the oldSet, and the second pass does 
///									filtering within the context of the newSet. 
/// \remarks
/// Called from: the Doc's RetokenizeText().
/// Sets up the document's data structures for the two pass reconstitution that must be
/// done when there has been a SFM set change. The function gets all the required data
/// structures set up appropriately for whichever pass is about to be effected (The caller
/// must have saved the current SFM set gCurrentSfmSet before this function is called for
/// the first time, and it must restore it after this function is called the second time
/// and the changes effected.)
///////////////////////////////////////////////////////////////////////////////
void CAdapt_ItDoc::SetupForSFMSetChange(enum SfmSet oldSet, enum SfmSet newSet, 
					wxString oldFilterMarkers, wxString newFilterMarkers, 
					wxString& secondPassFilterMkrs, enum WhichPass pass)
{
    // get a MapWholeMkrToFilterStatus map filled first with the set of original filter
    // markers, then on the second pass to be filled with the new filter markers (minus
    // those in common which were removed on first pass)
	wxString mkr;
	MapWholeMkrToFilterStatus mkrMap;
	MapWholeMkrToFilterStatus* pMap = &mkrMap;

	if (pass == first_pass)
	{
		// fill the map
		GetMarkerMapFromString(pMap, oldFilterMarkers);

#ifdef _Trace_RebuildDoc
		TRACE1("first_pass    local mkrMap size = %d\n", pMap->GetSize());
#endif

        // iterate, getting each marker, adding a trailing space, and removing same from
        // each of oldFilterMarkers and newFilterMarkers if in each, but making no change
        // if not in each
		gpApp->m_FilterStatusMap.clear();
		MapWholeMkrToFilterStatus::iterator iter;
		for (iter = pMap->begin(); iter != pMap->end(); ++iter) 
		{
			mkr = iter->first;
			wxString mkrPlusSpace = mkr + _T(' ');
			bool bRemoved = RemoveMarkerFromBoth(mkrPlusSpace, oldFilterMarkers, 
												newFilterMarkers);

#ifdef _Trace_RebuildDoc
			TRACE2("first_pass    bRemoved = %d, mkr = %s\n", (int)bRemoved, mkr);
#endif

            // if it did not get removed from both, then we will need to unfilter this mkr
            // in the first pass - so put it in m_FilterStatusMap with a "0" string value
            // associated with the marker as the key, ready for the later
            // ReconstituteAfterFilteringChange() call in the caller
			if (!bRemoved)
			{
				(gpApp->m_FilterStatusMap)[mkr] = _T("0"); // "0" means 
											// "was filtered, but now to be unfiltered"
			}
		}

        // set secondPassFilterMkrs to whatever is left after common ones have been removed
        // from the newFilterMarkers string -- these will be used on the second pass which
        // does the filtering
		secondPassFilterMkrs = newFilterMarkers;

		// set the old SFM set to be the current one; setup is complete for the first pass 
        // through the doc
		gpApp->gCurrentSfmSet = oldSet;

#ifdef _Trace_RebuildDoc
		TRACE2("first_pass    gCurrentSfmSet = %d,\n\n Second pass marker set: =  %s\n", (int)oldSet, secondPassFilterMkrs);
#endif
	}
	else // this is the second_pass
	{
        // on the second pass, the set of markers which have to be filtered out are passed
        // in in the string secondPassFilterMkrs (computed on the previous call to
        // SetupForSFMSetChange()), so all we need to do here is set up m_FilterStatusMap
        // again, with the appropriate markers and an associated value of "1" for each,
        // ready for the ReconstituteAfterFilteringChange() call in the caller
		if (secondPassFilterMkrs.IsEmpty())
		{
			gpApp->m_FilterStatusMap.clear();
			goto h;
		}

		// fill the local map, then iterate through it filling m_FilterStatusMap
		GetMarkerMapFromString(pMap, secondPassFilterMkrs);

		// set up m_FilterStatusMap again
		gpApp->m_FilterStatusMap.clear();
		
		{	// this extra block extends for the next 28 lines. It avoids the bogus 
			// warning C4533: initialization of 'f_iter' is skipped by 'goto h'
			// by putting its code within its own scope
		MapWholeMkrToFilterStatus::iterator iter;
		for (iter = pMap->begin(); iter != pMap->end(); ++iter)
		{
			(gpApp->m_FilterStatusMap)[mkr] = _T("1"); // "1" means 
									// "was unfiltered, but now to be filtered"

#ifdef _Trace_RebuildDoc
			TRACE1("second_pass    m_FilterStatusMap.SetAt() =   %s    = \"1\"\n", mkr);
#endif
		}
		} // end of extra block

		// set the new SFM set to be the current one; second pass setup is now complete
h:		gpApp->gCurrentSfmSet = newSet; // caller has done it already, or should have

#ifdef _Trace_RebuildDoc
		TRACE1("second_pass    gCurrentSfmSet = %d    (0 is UsfmOnly, 1 is PngOnly)\n", (int)newSet);
#endif
	}
}

///////////////////////////////////////////////////////////////////////////////
/// \return		nothing
/// \param		pMkrMap		<- pointer to a map of wholeMkr (including backslash) strings 
///								derived from the str parameter, with the wholeMkr as key, 
///								and value being an empty string (we don't care a hoot about
///								the value, we just want the benefits of the hashing on the key)
/// \param		str			-> a sequence of whole markers (including backslashes), with a 
///								space following each, but there should not be any endmarkers 
///								in str, but we'll make them begin markers if there are
/// \remarks
/// Called from: the Doc's SetupForSFMSetChange().
/// Populates a MapWholeMkrToFilterStatus wxHashMap with bare markers. No mapping associations are
/// made from this function - all are simply associated with a null string.
/// Extracts each whole marker, removes the backslash, gets rid of any final * if an endmarkers somehow
/// crept in unnoticed, and if unique, stores the bareMkr in the map; if the input string str is empty,
/// it exits without doing anything.
///////////////////////////////////////////////////////////////////////////////
void CAdapt_ItDoc::GetMarkerMapFromString(MapWholeMkrToFilterStatus*& pMkrMap, 
										  wxString str) // BEW added 06Jun05
{
	wxChar nix[] = _T(""); // an arbitrary value, we don't care what it is
	wxString wholeMkr = str;

	// get the first marker
	wxStringTokenizer tkz(wholeMkr); // use default " " whitespace here

	while (tkz.HasMoreTokens())
	{
		wholeMkr = tkz.GetNextToken();
		// remove final *, if it has one (which it shouldn't)
		wholeMkr = MakeReverse(wholeMkr);
		if (wholeMkr[0] == _T('*'))
			wholeMkr = wholeMkr.Mid(1);
		wholeMkr = MakeReverse(wholeMkr);

		// put it into the map
		(*pMkrMap)[wholeMkr] = nix;
	};
}

///////////////////////////////////////////////////////////////////////////////
/// \return		TRUE if the passed in wholeMarker and trailing space were removed from both 
///				str1 and str2, FALSE otherwise
/// \param		mkr		-> the wholeMarker (includes backslash, but no * at end) plus a trailing space
/// \param		str1	<- a set of filter markers (wholeMarkers, with a single space following each)
/// \param		str2	<- another set of filter markers (wholeMarkers, with a single space following each)
/// \remarks
/// Called from: the Doc's SetupForSFMSetChange().
/// Removes any markers (and trailing space) from str1 and from str2 which are common to
/// both strings. Used in the filtering stage of changing from an earlier SFM set to a
/// different SFM set - we wish use RemoveMarkerFromBoth in order to remove from contention
/// any markers and their content which were previously filtered and are to remain filtered
/// when the change to the different SFM set has been effected (since these are already
/// filtered, we leave them that way)
///////////////////////////////////////////////////////////////////////////////
bool CAdapt_ItDoc::RemoveMarkerFromBoth(wxString& mkr, wxString& str1, wxString& str2)
{
	int curPos1 = str1.Find(mkr);
	wxASSERT(curPos1 >= 0); // mkr MUST be in str1, since the set of mkr strings 
							// was obtained from str1 earlier
	int curPos2 = str2.Find(mkr);
	if (curPos2 != -1)
	{
		// mkr is in both strings, so remove it from both
		int len = mkr.Length();
		str1.Remove(curPos1,len);
		str2.Remove(curPos2,len);
		return TRUE;
	}
	return FALSE;
}

///////////////////////////////////////////////////////////////////////////////
/// \return		nothing
/// \param		dest	<- the source phrase whose flags are made to agree with src's flags
/// \param		src		-> the source phrase whose flags are copied to dest's flags
/// \remarks
/// Called from: the Doc's ReconstituteOneAfterPunctuationChange().
/// Copies the boolean values from src source phrase to the dest source phrase. The flags
/// copied are: m_bFirstOfType, m_bFootnoteEnd, m_bFootnote, m_bChapter, m_bVerse,
/// m_bParagraph, m_bSpecialText, m_bBoundary, m_bHasInternalMarkers, m_bHasInternalPunct,
/// m_bRetranslation, m_bNotInKB, m_bNullSourcePhrase, m_bHasKBEntry,
/// m_bBeginRetranslation, m_bEndRetranslation, and m_bHasGlossingKBEntry.
///////////////////////////////////////////////////////////////////////////////
void CAdapt_ItDoc::CopyFlags(CSourcePhrase* dest, CSourcePhrase* src)
{
	// BEW added on 5April05, to simplify copying of CSourcePhrase flag values
	dest->m_bFirstOfType = src->m_bFirstOfType;
	dest->m_bFootnoteEnd = src->m_bFootnoteEnd;
	dest->m_bFootnote = src->m_bFootnote;
	dest->m_bChapter = src->m_bChapter;
	dest->m_bVerse = src->m_bVerse;
	dest->m_bParagraph = src->m_bParagraph;
	dest->m_bSpecialText = src->m_bSpecialText;
	dest->m_bBoundary = src->m_bBoundary;
	dest->m_bHasInternalMarkers = src->m_bHasInternalMarkers;
	dest->m_bHasInternalPunct = src->m_bHasInternalPunct;
	dest->m_bRetranslation = src->m_bRetranslation;
	dest->m_bNotInKB = src->m_bNotInKB;
	dest->m_bNullSourcePhrase= src->m_bNullSourcePhrase;
	dest->m_bHasKBEntry = src->m_bHasKBEntry;
	dest->m_bBeginRetranslation = src->m_bBeginRetranslation;
	dest->m_bEndRetranslation = src->m_bEndRetranslation;
	dest->m_bHasGlossingKBEntry = src->m_bHasGlossingKBEntry;
}

///////////////////////////////////////////////////////////////////////////////
/// \return		FALSE if the rebuild fails internally, TRUE if the rebuild was successful
/// \param		pView		-> a pointer to the View
/// \param		pList		<- the list of source phrases of the current document (m_pSourcePhrases)
/// \param		pos			-> the node location which stores the passed in pSrcPhrase pointer
/// \param		pSrcPhrase	<- a pointer to a passed in source phrase
/// \param		fixesStr	<- a reference to the caller's storage string for accumulating
///								a list of references to the locations where the rebuild 
///								failed for specific pSrcPhrase instances, if any
/// \remarks
/// Called from: the Doc's RetokenizeText(). 
/// Rebuilds the document after a punctuation change has been made.
/// If the function fails internally, that particular pSrcPhrase instance has to have its
/// adaptation (or gloss) abandoned and the user must later inspect the doc visually and
/// edit at such locations to restore the translation (or gloss) correctly.
/// A "fail" of the rebuild means that the rebuild did not, for the rebuild done on a
/// single CSourcePhrase instance, result in a single CSourcePhrase instance - but rather
/// two or more (it is not possible for the rebuild of a single instance to result in
/// none).
/// Our approach is as follows: if the rebuild of each generates a single instance, we
/// re-setup the members of that passed in instance with the correct new values, (and throw
/// away the rebuilt one - fixing that one up would be too time-consuming); but if the
/// rebuild fails, we go to the bother of copying member values across to the first
/// instance in the new list of resulting sourcephrases, insert the list into the main
/// document's list, and throw away the original passed in one.
/// We internally keep track of how many new CSourcePhrase instances are created and how
/// many of these proceed the view's m_nActiveSequNum value so we can progressively update
/// the latter and so reconstitute the phrase box at the correct location when done.
///////////////////////////////////////////////////////////////////////////////
bool CAdapt_ItDoc::ReconstituteAfterPunctuationChange(CAdapt_ItView* pView, 
					SPList*& pList, SPList::Node* pos, CSourcePhrase*& pSrcPhrase, 
					wxString& fixesStr)
{
	// whm added 4Apr05
	// BEW ammended definition and coded the function
	SPList* pResultList = new SPList; // where we'll store pointers to parsed 
									  // new CSourcePhrase instances
	bool bSucceeded = TRUE;

	int nActiveSequNum = gpApp->m_nActiveSequNum; // store original value so 
												  // we can update it as necessary
	int nThisSequNum = pSrcPhrase->m_nSequNumber; // store the passed in 
                                    // sourcephrase's sequence number (we update the active
                                    // location only for sourcephrases located before the
                                    // one which is the nActiveSequNum one)

    // remove the CRefString entries for this instance from the KBs, or decrement its count
    // if several seen before but do restoring of KB entries within the called functions
    // (because they know whether the rebuild succeeded or not and they have the required
    // strings at hand) - but do it only if not a placeholder, not a retranslation not
    // specified as "not in the KB", and there is a non-empty adaptation.
	if (!pSrcPhrase->m_bNullSourcePhrase && !pSrcPhrase->m_bRetranslation && 
		!pSrcPhrase->m_bNotInKB && !pSrcPhrase->m_adaption.IsEmpty())
	{
		pView->RemoveKBEntryForRebuild(pSrcPhrase);
	}
	// determine whether we are dealing with just one, 
	// or an owning one and the sublist containing those it owns
	if (pSrcPhrase->m_nSrcWords > 1)
	{
        // this is a merged sourcephrase; so we expect the tokenizing of its m_srcPhrase
        // string to result in several new CSourcePhrase instances, so we can't call
        // ReconstituteOneAfterPunctuationChange() here, instead plagiarize its code and
        // modify to handle the passed in merged CSourcePhrase instance here. Our primary
        // goal here is to determine if the count of the new srcPhrase instances from the
        // retokenization of m_srcPhrase is the same value as the value in the passed in
        // pSrcPhrase instance's member m_nSrcWords. If that is so, then this is a likely
        // candidate for a successful rebuild -- the only other criterion is that the total
        // count of the reparsed original (owned) sourcephrase instances does not exceed
        // the maximum (10) allowed for a merged sourcephrase. If both criteria are
        // satisfied, we can programmatically rebuild this merged pSrcPhrase; if either is
        // not satisfied, we have to abandon the merged instance and restore the list of
        // (retokenized) owned instances to the main m_pSourcePhrases list, and inform the
        // user of where he'll later need to manually remerge and readapt/regloss.
		int nOriginalCount = pSrcPhrase->m_nSrcWords;
		bool bNotInKB = FALSE; // default
		bool bRetranslation = FALSE; // default
		if (pSrcPhrase->m_bRetranslation) bRetranslation = TRUE;
		if (pSrcPhrase->m_bNotInKB) bNotInKB = TRUE;
		// placeholders omitted above - because they can't be merged so this block 
		// doesn't need to deal with them

		wxString srcPhrase; // copy of m_srcPhrase member
		wxString targetStr; // copy of m_targetStr member
		wxString adaption; // copy of m_adaption member
		wxString gloss; // copy of m_gloss member
		adaption.Empty(); 
		gloss.Empty();

        // setup the srcPhrase, targetStr and gloss strings - we must handle glosses too
        // regardless of the current mode (whether adapting or not) since rebuilding
        // affects everything
		int numLegacyElements = pSrcPhrase->m_nSrcWords;
		gloss = pSrcPhrase->m_gloss; // we don't care if glosses have punctuation or not
		targetStr = pSrcPhrase->m_targetStr; // likewise, has punctuation, if any
		srcPhrase = pSrcPhrase->m_srcPhrase; // this member has all the source punctuation, 
											 // if any on this word or phrase

		// reparse the srcPhrase string
		// important, ParseWord() doesn't work right if the first character is a space
		srcPhrase.Trim(TRUE); // trim right end
		srcPhrase.Trim(FALSE); // trim left end
		int numElements;
		numElements = pView->TokenizeTextString(pResultList, srcPhrase, 
												pSrcPhrase->m_nSequNumber);
		wxASSERT(numElements > 1);
		
		// test to see if we have a candidate for rebuilding successfully
		if ((int)pResultList->GetCount() == numLegacyElements)
		{
            // the numbers of old and new CSourcePhrase instances match, so check out the
            // owned instances next - after retokenizing them; we can't base the rebuild on
            // the above retokenization because it would leave the owned sourcephrase
            // instances unrebuilt, and if the user then later unmerged, he would restore
            // instances with the wrong punctuation settings. So we must rebuild each of
            // the owned ones and this process could conceivably generate a different total
            // for the number produced (it would be unexpected and I can't conceive how it
            // could happen but I'm going to play safe and assume it could) -- so we'll
            // base our final merger on a remerge of the rebuild owned ones, but we can do
            // that only if there are less than MAX_WORDS (ie. 10) - otherwise, we count
            // this build attempt as a "failure" and insert the rebuilt owned ones back
            // into the main list and throw away the original merged sourcephrase instance
            // passed in.
			SPList* pOwnedList = new SPList; // accumulate here the results of reparsing 
											 // all owned CSourcePhrases
			SPList::Node* posOwned = pSrcPhrase->m_pSavedWords->GetFirst();
			wxASSERT( posOwned != NULL);
			SPList* pParsedList = new SPList; // for returning the results of parsing 
											  // each owned instance
			while (posOwned != NULL)
			{
				SPList::Node* savePos = posOwned;
				CSourcePhrase* pOwnedSrcPhrase = (CSourcePhrase*)posOwned->GetData();
				posOwned = posOwned->GetNext();
				pParsedList->Clear(); 
				bool bParseCountUnchanged = ReconstituteOneAfterPunctuationChange(
												pView,pSrcPhrase->m_pSavedWords,
												savePos,pOwnedSrcPhrase,fixesStr,
												pParsedList,TRUE);
                // which sourcephrase instance(s) we transfer to pOwnedList depends on the
                // value of bParseCountUnchanged. If the latter is TRUE, then
                // ReconstituteOneAfterPunctuationChange will have updated the passed in
                // sourcephrase pOwnedSrcPhrase and so we'll abandon the contents of
                // pParsedList since the function would not have updated that list's one,
                // but if the return value was FALSE, the ones in pParsedList will have to
                // be copied to pOwnedList because they were the ones which the function
                // updated
				if (bParseCountUnchanged)
				{
					// retain the original
					pOwnedList->Append(pOwnedSrcPhrase);

					// delete the unwanted newly parsed sourcephrases in pParsedList
					DeleteListContentsOnly(pParsedList);
				}
				else
				{
                    // number of instances changed from one, so we want what is in
                    // pParsedList to replace the original - so we will not call
                    // DeleteListContentsOnly() here, and we'll not bother to delete the
                    // original in the m_pSavedWords list on the original merged
                    // sourcephrase instance because we will abandon that list's contents
                    // and replace the contents by the list pOwnedList (provided the number
                    // of saved instances in it is 10 or less) - but if more than 10 then
                    // we'll abandon the whole merger and instead put the contents of
                    // pOwnedList back into m_pSourcePhrases and get a reference to the
                    // fail location added to fixesStr. (NOTE. In this algorithm, a change
                    // in the number of the parsed CSourcePhrase instances when rebuilding
                    // one of the owned ones in the m_pSavedWords list is potentially being
                    // treated as a non- failure of the rebuild, provided we end up with
                    // less than 10 when the loop has finished. We will try to 'get away'
                    // with this on the grounds that the rebuilding of the merged phrase
                    // can be still be done reliably - its only the number of owned
                    // instances which has changed, and one or more of those will have had
                    // any adaptation & gloss abandoned - but such ones are rather unlikely
                    // to have either an adaptation or gloss anyway, in which case nothing
                    // much is lost; and provided the user does not subsequently unmerge
                    // this rebuilt merger, those owned originals will never be seen again
                    // because they will remain hidden in its m_pSavedWords list.)
					SPList::Node* ppos = pParsedList->GetFirst();
					while (ppos != NULL)
					{
						CSourcePhrase* pSP = (CSourcePhrase*)ppos->GetData();
						ppos = ppos->GetNext();
						pOwnedList->Append(pSP);
					}
				}
			} // end of loop for retokenizing the source text of each of the 
			  // owned sourcephrases of pSrcPhrase

            // pParsedList is no longer needed and any time its contents were retained the
            // sourcephrase pointers were transferred to pOwnedList and so are managed by
            // pOwnedList, and the blocks in the loop above will have already deleted any
            // sourcephrase instances not to be retained, so finish cleaning up
				pParsedList->Clear();
				delete pParsedList;

            // Find out if we have 10 or less; if 10 or less then we can successfully
            // rebuild the merger, if not, we are forced to abandon the merge and will have
            // to put the owned (new) instances back into m_pSourcePhrases and tell the
            // user where this happened
			int nNewCount = pOwnedList->GetCount();
			if (nNewCount > MAX_WORDS)
			{
                // No deal on the rebuild possibility with the rebuild owned instances, so
                // abandon the merge. First, add a reference to the bad location to
                // fixesStr (but only if it is (U)SFM structured data - since unstructured
                // data lacks chapter and verse markers)
				pView->UpdateSequNumbers(0); // make sure they are in sequence, 
											 // so next call won't fail
				if (!gbIsUnstructuredData)
				{
					fixesStr += pView->GetChapterAndVerse(pSrcPhrase);
					fixesStr += _T("   ");
				}

				// do the active sequence number updating
				if (nThisSequNum < nActiveSequNum)
					nActiveSequNum += nNewCount - 1; // 1 instance is being replaced by 
													 // nNewCount instances

                // insert the newly built list of CSourcePhrase instances into
                // m_pSourcePhrases preceding the pos position
				SPList::Node* newPOS = pOwnedList->GetFirst();
				CSourcePhrase* pSrcPhr = NULL;
				while (newPOS != NULL)
				{
					pSrcPhr = (CSourcePhrase*)newPOS->GetData();
					newPOS = newPOS->GetNext();
					pList->Insert(pos,pSrcPhr);
				}

				// delete the original merged instance and tidy up
				DeleteSingleSrcPhrase(pSrcPhrase,FALSE); // don't bother to delete 
														 // its partner pile
				pList->DeleteNode(pos);

				// tell caller there was a failure on this rebuild attempt
				bSucceeded = FALSE;
			}
			else // there were MAX_WORDS ie. 10 or less new CSourcePhrases 
				 // parsed from the m_pSavedWords list
			{
                // we are gunna get away with a rebuild, so get on with it -- we need to
                // use the Merge() function in the CSourcePhrase class, and pass it the new
                // CSourcePhrase instances in pOwnedList - the iterative calls to Merge
                // build the m_pSavedWords list (where the merging is done by all but the
                // first of pOwnedList being merged to the first in that list so that
                // m_pSavedWords is the list in that first instance in the pOwnedList list
				SPList::Node* pos2 = pOwnedList->GetFirst();
				wxASSERT(pos2);
				// we merge the second and all succeeding to the first one
				CSourcePhrase* pMergedSrcPhr = (CSourcePhrase*)pos2->GetData();
				pos2 = pos2->GetNext();
				while (pos2 != NULL)
				{
					CSourcePhrase* pSP = (CSourcePhrase*)pos2->GetData();
					pos2 = pos2->GetNext();
					if (pSP->m_bNotInKB)
						pSP->m_bNotInKB = FALSE;
					pMergedSrcPhr->Merge(pView,pSP);
				}

                // do the active sequence number updating; since we will get away with a
                // merge here, we must subtract nOriginalCount from the nNewCount value to
                // find out how many extras, if any, are involved
				if (nThisSequNum < nActiveSequNum)
					nActiveSequNum += nNewCount - nOriginalCount;

                // we now have this situation: the original merged sourcephrase
                // (pSrcPhrase) is due to be abandoned (ie. deleted) but we have rebuilt
                // the sourcephrases in its m_pSavedWords list, and earlier stored these
                // rebuilt ones in pOwnedList; the rebuilt ones are actually the same
                // objects as in the pSrcPhrase->m_pSavedWords list, except that if any one
                // of the rebuilt ones produced two or more in the rebuilding, then these
                // ones would not be identical to any in the pSrcPhrase->m_pSavedWords list
                // (see what ReconstituteOneAfterPunctuationChange() does when it fails, to
                // see why). To get rid of pSrcPhrase we have to therefore first delete any
                // owned sourcephrase instances in the pSrcPhrase->m_pSavedWords list which
                // are NOT also in the pOwnedList which (due to the merge) are now all in
                // the pMergedSrcPhr->m_pSavedWords list - and recall that the first in the
                // latter list is built by the copy constructor because pMergedSrcPhrase is
                // actually the first instance that was in pOwnedList - to which all the
                // others merged in the block above. We can't therefore just delete all the
                // instances in the pSrcPhrase->m_pSavedWords list because we'd be deleting
                // some which we must retain (ie. those which are pointed at by pointers in
                // both lists).
				pOwnedList->Clear(); // these are all managed elsewhere, either as 
                                     // pMergedSrcPhr or in the latter's m_pSavedWords list
				SPList::Node* posOld = pSrcPhrase->m_pSavedWords->GetFirst();
				wxASSERT(posOld);
				while (posOld != NULL)
				{
					CSourcePhrase* pSrcPhraseOld = (CSourcePhrase*)posOld->GetData();
					posOld = posOld->GetNext();
					SPList::Node* posNew = NULL;
					if (pSrcPhraseOld == pMergedSrcPhr)
					{
						// retain this one, so do nothing
						continue;
					}
					else // it's not the pMergedSrcPhr, so test for a match in the 
						 // latter's m_pSavedWords list
					{
						posNew = pMergedSrcPhr->m_pSavedWords->Find(pSrcPhraseOld);
						if (posNew == NULL)
						{
							// no matching CSourcePhrase instance was found, 
							// so this old one can be deleted
							DeleteSingleSrcPhrase(pSrcPhraseOld, FALSE); // don't 
											// bother to delete its partner pile
						}
						else
						{
							// there was a match, so we need to retain this one
							continue;
						}
					}
				}
                // remove the entries in the old m_pSavedWords list, but leave pointers
                // alone - anything retained is now managed by the
                // pMergedSrcPhr->m_pSavedWords list
				pSrcPhrase->m_pSavedWords->Clear();

				// copy across the passed-in pSrcPhrase's flag values to the new merged
				// instance 
				CopyFlags(pMergedSrcPhr,pSrcPhrase);

                // copy the other stuff (m_chapterVerse & m_markers are accumulated by the
                // Merge() call and the medial lists, if needed, set up; so we have fewer
                // things to copy here)
				pMergedSrcPhr->m_curTextType = pSrcPhrase->m_curTextType;
				pMergedSrcPhr->m_inform = pSrcPhrase->m_inform;
				pMergedSrcPhr->m_nSequNumber = pSrcPhrase->m_nSequNumber;// maybe 
														// incorrect, but we don't care

				bool bNotInKB = pSrcPhrase->m_bNotInKB; // preserve for store test below
                // The m_bHas... booleans need to be made FALSE since this instance is not
                // yet in the KB (we store it there below, if the original was stored
                // there)
				pMergedSrcPhr->m_bHasKBEntry = FALSE;
				pMergedSrcPhr->m_bHasGlossingKBEntry = FALSE;
				
                // the rebuilt sourcephrase instances may not have filled-in m_targetStr
                // and m_adaption members, so we have to copy pSrcPhrase's (merged) members
                // to pMergedSrcPhr in order to retain the user's adaptation - and
                // similarly for the m_gloss member (the m_key member should already be
                // correct, since it was built by the Merge() calls within the loop, along
                // with the m_srcPhrase member)
				targetStr.Trim(TRUE); // trim right end
				targetStr.Trim(FALSE); // trim left end
				gloss.Trim(TRUE); // trim right end
				gloss.Trim(FALSE); // trim left end

				pMergedSrcPhr->m_targetStr = targetStr;
				pMergedSrcPhr->m_gloss = gloss;
				adaption = targetStr;
				pView->RemovePunctuation(this,&adaption,from_target_text);
				pMergedSrcPhr->m_adaption = adaption;

				// now insert our rebuilt merged sourcephrase preceding the old one
				pList->Insert(pos,pMergedSrcPhr);

				// and delete the old one, and then remove its list entry
				DeleteSingleSrcPhrase(pSrcPhrase, FALSE); // don't bother to delete 
														  // its partner pile
				pList->DeleteNode(pos);

				// store its gloss and adaptation entries in the appropriate KBs, 
				// provided it is appropriate
				if (!bNotInKB)
					pView->StoreKBEntryForRebuild(pMergedSrcPhr,
							pMergedSrcPhr->m_adaption,pMergedSrcPhr->m_gloss);
			}

            // remove pOwnedList and remove its pointers, but don't delete the objects
            // since they belong to m_pSavedWords in the rebuilt merged sourcephrase; or
            // they have been inserted into the document's m_pSourcePhrases list - either
            // way, they must be retained
			pOwnedList->Clear();
			delete pOwnedList;
		}
		else // count of new instances after reparse differs from count of 
			 // legacy instances
		{
            // we got more (or maybe less) CSourcePhrases in the reparse, so we'll have to
            // abandon this merger; we'll handle this case by plagiarizing the code from
            // earlier in this function, and just omit some tests. We will have to call
            // ReconstituteOneAfterPunctuationChange() on all the owned ones in
            // m_pSavedWords, and then insert these rebuilt instances into the
            // m_pSourcePhrases list, delete the passed in pSrcPhrase, then abandon the old
            // adaptation, and make sure the user is informed about where this failure to
            // rebuild took place. (Comments removed in the code in the following block,
            // since it is copied from above.)
			SPList* pOwnedList = new SPList;
			SPList::Node* posOwned = pSrcPhrase->m_pSavedWords->GetFirst();
			wxASSERT( posOwned != NULL);
			SPList* pParsedList = new SPList; // for returning the results of parsing 
											  // each owned instance
			while (posOwned != NULL)
			{
				SPList::Node* savePos = posOwned;
				CSourcePhrase* pOwnedSrcPhrase = (CSourcePhrase*)posOwned->GetData();
				posOwned = posOwned->GetNext();
				pParsedList->Clear(); 
				bool bParseCountUnchanged = ReconstituteOneAfterPunctuationChange(
												pView,pSrcPhrase->m_pSavedWords,
												savePos,pOwnedSrcPhrase,fixesStr,
												pParsedList,TRUE);
				if (bParseCountUnchanged)
				{
					pOwnedList->Append(pOwnedSrcPhrase);
					DeleteListContentsOnly(pParsedList);
				}
				else
				{
                    // wxList's Append method can't append another list to this list, so
                    // we'll just add the items in pParsedList to pOwnedList one-by-one.
					SPList::Node* ppos = pParsedList->GetFirst();
					while (ppos != NULL)
					{
						CSourcePhrase* pSP = (CSourcePhrase*)ppos->GetData();
						ppos = ppos->GetNext();
						pOwnedList->Append(pSP);
					}
				}
			}
			pParsedList->Clear();
			delete pParsedList;
			pView->UpdateSequNumbers(0); // make sure they are in sequence, 
										 // so next call won't fail
			int nTheCount = pOwnedList->GetCount();
			if (nThisSequNum < nActiveSequNum)
				nActiveSequNum += nTheCount - 1; // 1 instance is being 
												 // replaced by nTheCount instances
			if (!gbIsUnstructuredData)
			{
				fixesStr += pView->GetChapterAndVerse(pSrcPhrase);
				fixesStr += _T("   ");
			}
			SPList::Node* newPOS = pOwnedList->GetFirst();
			CSourcePhrase* pSrcPhr = NULL;
			while (newPOS != NULL)
			{
				pSrcPhr = (CSourcePhrase*)newPOS->GetData();
				newPOS = newPOS->GetNext();
				pList->Insert(pos,pSrcPhr);
			}
			SmartDeleteSingleSrcPhrase(pSrcPhrase,pOwnedList);
			pList->DeleteNode(pos);
			pOwnedList->Clear();
			delete pOwnedList;
			bSucceeded = FALSE;
		}
	} // end of block for when dealing with a merged sourcephrase
	else // the test of pSrcPhrase->m_nSrcWords yielded 1 
		 // (ie. an unmerged sourcephrase)
	{
        // we are dealing with a plain vanila single-word non-owned sourcephrase in either
        // adaptation or glossing mode
		bool bWasOK = ReconstituteOneAfterPunctuationChange(
						pView,pList,pos,pSrcPhrase,fixesStr,pResultList,FALSE);
		if (!bWasOK)
		{
            // we got more than one in the reparse, so work out how many extras there are
            // and update nActiveSequNum accordingly
			int count = pResultList->GetCount();
			wxASSERT (count > 1);
			if (nThisSequNum < nActiveSequNum)
				nActiveSequNum += count - 1; // 1 instance is being replaced 
											 // by count instances

            // add a reference to where things went wrong for the Rebuild Log.txt file and
            // the message box
			pView->UpdateSequNumbers(0);
			if (!gbIsUnstructuredData)
			{
                // sequence numbers may not be uptodate, so do so first over whole list so
                // that the attempt to find the chapter:verse reference string will not
                // fail
				fixesStr += pView->GetChapterAndVerse(pSrcPhrase);
				fixesStr += _T("   ");
			}

            // we are now ready to replace the original pSrcPhrase in m_pSourcePhrases with
            // what was parsed within the above function - since we unexpectedly got more
            // sourcephrase instances than one in the parse (we don't add to the KB for
            // such as these, because we treat them as yet to be adapted/glossed)
			SPList::Node* pos2 = pResultList->GetFirst();
			while (pos2 != NULL)
			{
				CSourcePhrase* pSP = (CSourcePhrase*)pos2->GetData();
				pos2 = pos2->GetNext();
				SPList::Node* pos3;
				pos3 = pList->Insert(pos,pSP); // insert before the original POSITION
			}
			DeleteSingleSrcPhrase(pSrcPhrase, FALSE); // delete old one, 
									// don't bother to delete partner pile
			pList->DeleteNode(pos);

            // the sourcephrases stored in pResultList are now managed by m_pSourcePhrases
            // list, and so we must not delete them - hence just remove the pointers from
            // pResultList and return here rather than exiting the block
			pResultList->Clear();
			delete pResultList;
			bSucceeded = FALSE; // ensure caller knows we got a bad rebuild
			gpApp->m_nActiveSequNum = nActiveSequNum; // update the view's member so 
													  // all keeps in sync
			return bSucceeded;
		}
	}

	// delete the local list and its managed memory chunks - don't leak memory
	SPList::Node* apos = pResultList->GetFirst();
	CSourcePhrase* pASrcPhrase = NULL;
	while (apos != NULL)
	{
		pASrcPhrase = (CSourcePhrase*)apos->GetData();
		apos = apos->GetNext();

        // If pASrcPhrase belongs to a retranslation,
        // ReconstituteOneAfterPunctuationChange() will have already inserted any sublist
        // of newly built sourcephrases resulting from a 'fail' to reconstitute a passed in
        // single instance as as the same one rebuilt resulting in two or more - the
        // insertion having taken place in pList (ie. m_pSourcePhrases); whereas if the
        // single was rebuilt without failure, then the rebuilt one is to be thrown away
        // entirely. Hence, in the former case, if we unilaterally here delete the rebuilt
        // ones, those resulting from a failure and so are already in pList would then be
        // unilaterally deleted, and the result is hanging pointers and a corrupted
        // m_pSourcePhrases list. So for failures, we have to do a conditional delete -
        // that is, only delete an instance if it is NOT in pList.
        // (ConditionallyDeleteSrcPhrase() has a sufficient test that would allow us to use
        // the one call to replace all the next four lines, but pList is typically long,
        // and it is therefore quicker to confine the call of this function to only those
        // sourcephrase instances which could be affected - that is, retranslation ones
		if (pASrcPhrase->m_bRetranslation)
			ConditionallyDeleteSrcPhrase(pASrcPhrase,pList);
		else
			DeleteSingleSrcPhrase(pASrcPhrase,FALSE); // FALSE means 
                        // "don't delete its partner pile" as we'll let RecalcLayout()
                        // delete them all quickly en masse later
	}
	pResultList->Clear();
	delete pResultList;
	gpApp->m_nActiveSequNum = nActiveSequNum; // update the view's member 
											  // so all keeps in sync
	return bSucceeded;
}

///////////////////////////////////////////////////////////////////////////////
/// \return		nothing
/// \param		bKBFilename				-> if TRUE the KB's filename is to be updated
/// \param		bKBPath					-> if TRUE the KB path is to be updated
/// \param		bKBBackupPath			-> if TRUE the KB's backup path is to be updated
/// \param		bGlossingKBPath			-> if TRUE the glossing KB path is to be updated
/// \param		bGlossingKBBackupPath	-> if TRUE the glossing KB's backup path is to be updated
/// \remarks
/// Called from: the App's LoadGlossingKB(), LoadKB(), StoreGlossingKB(), StoreKB(), 
/// SaveKB(), SaveGlossingKB(), SubstituteKBBackup().
///////////////////////////////////////////////////////////////////////////////
void CAdapt_ItDoc::UpdateFilenamesAndPaths(bool bKBFilename,bool bKBPath,
						bool bKBBackupPath,bool bGlossingKBPath, 
						bool bGlossingKBBackupPath)
{
    // The following ones are KB and GlossingKB ones, and so need to preserve the
    // alternative name or path in another wxString

	// KB filename (m_curKBName)
	if (bKBFilename)
	{
		wxString thisFilename = gpApp->m_curKBName;
		if (gpApp->m_bSaveAsXML) // always true in the wx version
		{
			thisFilename = MakeReverse(thisFilename);
			int nFound = thisFilename.Find(_T('.'));
			wxString extn = thisFilename.Left(nFound);
			extn = MakeReverse(extn);
			if (extn != _T("xml"))
			{
				gpApp->m_altKBName = gpApp->m_curKBName; // the *.KB name
				thisFilename = thisFilename.Mid(nFound); // remove the KB extension
				thisFilename = MakeReverse(thisFilename);
				thisFilename += _T("xml");
			}
			else
			{
				wxString altStr = thisFilename; // it's still reversed
				nFound = altStr.Find(_T('.'));
				altStr = altStr.Mid(nFound);
				altStr = MakeReverse(altStr);
				gpApp->m_altKBName = altStr + _T("KB"); // the *.KB name
				thisFilename = MakeReverse(thisFilename); // the *.xml name (currently active)
			}
		}
		gpApp->m_curKBName = thisFilename;
	}
	
	// KB Path (m_curKBPath)
	if (bKBPath)
	{
		wxString thisPath = gpApp->m_curKBPath;
		if (gpApp->m_bSaveAsXML) // always true in the wx version
		{
			thisPath = MakeReverse(thisPath);
			int nFound = thisPath.Find(_T('.'));
			wxString extn = thisPath.Left(nFound);
			extn = MakeReverse(extn);
			if (extn != _T("xml"))
			{
				gpApp->m_altKBPath = gpApp->m_curKBPath; // the *.KB pathname
				thisPath = thisPath.Mid(nFound); // remove the KB extension
				thisPath = MakeReverse(thisPath);
				thisPath += _T("xml");
			}
			else
			{
				wxString altPath = thisPath; // it's still reversed
				nFound = altPath.Find(_T('.'));
				altPath = altPath.Mid(nFound);
				altPath = MakeReverse(altPath);
				gpApp->m_altKBPath = altPath + _T("KB"); // the *.KB pathname
				thisPath = MakeReverse(thisPath); // the *.xml pathname
			}
		}
		gpApp->m_curKBPath = thisPath;
	}
	
	// KB Backup Path (m_curKBBackupPath)
	if (bKBBackupPath)
	{
        // also do any needed adjustment to m_curKBBackupPath. Note -- this is different
        // than the two above, because we don't want KB files which could have been binary
        // (extension .KB) or XML to just have a .BAK extension for the backup one - since
        // then we'd not know from the extension whether the contents were binary or xml.
        // So for the backups, we'll have .BAK for a binary backup, and .BAK.xml for an XML
        // backup (ie. just add the .xml extension to the backup name as produced by the
        // legacy code)
		wxString thisBackupPath = gpApp->m_curKBBackupPath;
		if (gpApp->m_bSaveAsXML) // always true in the wx version
		{
			thisBackupPath = MakeReverse(thisBackupPath);
			int nFound = thisBackupPath.Find(_T('.'));
			wxString extn = thisBackupPath.Left(nFound);
			extn = MakeReverse(extn);
			if (extn != _T("xml"))
			{
				// it found BAK
				gpApp->m_altKBBackupPath = gpApp->m_curKBBackupPath;// the *.BAK pathname
				thisBackupPath = MakeReverse(thisBackupPath);
				thisBackupPath += _T(".xml"); // add ".xml" to the final .BAK already there
			}
			else
			{
				wxString altBackupPath = thisBackupPath; // it's still reversed
				nFound = altBackupPath.Find(_T('.'));
				altBackupPath = altBackupPath.Mid(nFound + 1); // remove ".xml"
				altBackupPath = MakeReverse(altBackupPath);
				gpApp->m_altKBBackupPath = altBackupPath; // the *.BAK pathname
				thisBackupPath = MakeReverse(thisBackupPath); // it's already correct as .BAK.xml
			}
		}
		gpApp->m_curKBBackupPath = thisBackupPath;
	}

	// Glossing KB Path
	if (bGlossingKBPath)
	{
		wxString thisGlossingPath = gpApp->m_curGlossingKBPath;
		if (gpApp->m_bSaveAsXML) // always true in the wx version
		{
			thisGlossingPath = MakeReverse(thisGlossingPath);
			int nFound = thisGlossingPath.Find(_T('.'));
			wxString extn = thisGlossingPath.Left(nFound);
			extn = MakeReverse(extn);
			if (extn != _T("xml"))
			{
				gpApp->m_altGlossingKBPath = gpApp->m_curGlossingKBPath; // the *.KB pathname
				thisGlossingPath = thisGlossingPath.Mid(nFound); // remove the KB extension
				thisGlossingPath = MakeReverse(thisGlossingPath);
				thisGlossingPath += _T("xml"); // the *.xml pathname
			}
			else
			{
				wxString altGlossingPath = thisGlossingPath; // it's still reversed
				nFound = altGlossingPath.Find(_T('.'));
				altGlossingPath = altGlossingPath.Mid(nFound);
				altGlossingPath = MakeReverse(altGlossingPath);
				gpApp->m_altGlossingKBPath = altGlossingPath + _T("KB"); // the *.KB pathname
				thisGlossingPath = MakeReverse(thisGlossingPath); // the *.xml pathname
			}
		}
		gpApp->m_curGlossingKBPath = thisGlossingPath;
	}
	
	// Glossing KB Backup Path
	if (bGlossingKBBackupPath)
	{
        // also do any needed adjustment to m_curGlossingKBBackupPath. Note -- this is
        // different than above, because we don't want KB files which could have been
        // binary (extension .KB) or XML to just have a .BAK extension for the backup one -
        // since then we'd not know from the extension whether the contents were binary or
        // xml. So for the backups, we'll have .BAK for a binary backup, and .BAK.xml for
        // an XML backup (ie. just add the .xml extension to the backup name as produced by
        // the legacy code)
		wxString thisGlossingBackupPath = gpApp->m_curGlossingKBBackupPath;
		if (gpApp->m_bSaveAsXML) // always true in the wx version
		{
			thisGlossingBackupPath = MakeReverse(thisGlossingBackupPath);
			int nFound = thisGlossingBackupPath.Find(_T('.'));
			wxString extn = thisGlossingBackupPath.Left(nFound);
			extn = MakeReverse(extn);
			if (extn != _T("xml"))
			{
				// it found BAK
				gpApp->m_altGlossingKBBackupPath = gpApp->m_curGlossingKBBackupPath;// the *.BAK pathname
				thisGlossingBackupPath = MakeReverse(thisGlossingBackupPath);
				thisGlossingBackupPath += _T(".xml"); // add ".xml" to the final .BAK already there
			}
			else
			{
				wxString altGlossingBackupPath = thisGlossingBackupPath; // it's still reversed
				nFound = altGlossingBackupPath.Find(_T('.'));
				altGlossingBackupPath = altGlossingBackupPath.Mid(nFound + 1); // remove ".xml"
				altGlossingBackupPath = MakeReverse(altGlossingBackupPath);
				gpApp->m_altGlossingKBBackupPath = altGlossingBackupPath; // the *.BAK pathname
				thisGlossingBackupPath = MakeReverse(thisGlossingBackupPath); // it's already correct as .BAK.xml
			}
		}
		gpApp->m_curGlossingKBBackupPath = thisGlossingBackupPath;
	}
}

///////////////////////////////////////////////////////////////////////////////
/// \return		nothing
/// \param		title				-> the new title to be used on the document's title bar
/// \param		nameMinusExtension	<- the same name as title, but minus any extension returned by
///										reference to the caller
/// \remarks
/// Called from: the Doc's OnNewDocument(), OnOpenDocument() and CMainFrame's OnIdle().
/// Sets or updates the main frame's title bar with the appropriate name of the current
/// document. It also suffixes " - Adapt It" or " - Adapt It Unicode" to the document title,
/// depending on which version of Adapt It is being used. 
/// The extension for all documents in the wx version is .xml. This function also calls 
/// the doc/view framework's SetFilename() to inform the framework of the change in the
/// document's name.
///////////////////////////////////////////////////////////////////////////////
void CAdapt_ItDoc::SetDocumentWindowTitle(wxString title, wxString& nameMinusExtension)
{
	// remove any extension user may have typed -- we'll keep control ourselves
	wxString noExtnName = gpApp->MakeExtensionlessName(title);
	nameMinusExtension = noExtnName; // return to the caller the name 
									 // without the final extension

    // we'll now put on it what the extension should be, according to the doc type we have
    // elected to save
	wxString extn;
	if (gpApp->m_bSaveAsXML) // always true in the wx version
		extn = _T(".xml");
	title = noExtnName + extn;

	// whm Note: the m_strTitle is internal to CDocument
	// update the MFC native storage for the doc title
	this->SetFilename(title, TRUE); // see above where default unnamedN is set - 
									// TRUE means "notify views"
		
    // our Adapt It main window should also show " - Adapt It" or " - Adapt It Unicode"
    // after the document title, so we'll set that up by explicitly overwriting the title
    // bar's document name (Also, via the Setup Wizard, an output filename is not
    // recognised as a name by MFC and the MainFrame continues to show "Untitled", so we
    // set the window title explicitly as above)
	wxDocTemplate* pTemplate = GetDocumentTemplate();
	wxASSERT(pTemplate != NULL);
	wxString typeName, typeName2; // see John's book p149
	// BEW added Unicode for unicode build, 06Aug05
	typeName = pTemplate->GetDocumentName(); // returns the document type name as passed
											 // to the doc template constructor
	typeName2 = pTemplate->GetDescription(); 
	if (!typeName.IsEmpty()) 
	{
		typeName = _T(" - Adapt It");
		#ifdef _UNICODE
		typeName += _T(" Unicode");
		#endif
	}
	else
	{
		typeName = _T(" - ") + typeName; // Untitled, I think
		#ifdef _UNICODE
		typeName += _T(" Unicode");
		#endif
	}
	this->SetTitle(title + typeName);
}

///////////////////////////////////////////////////////////////////////////////
/// \return		nothing
/// \param		curOutputFilename	-> the current m_curOutputFilename value
/// \param		bSaveAsXML			-> the current m_bSaveAsXML value
/// \remarks
/// Called from: the Doc's BackupDocument() and DoFileSave().
/// Insures that the App's m_altOutputBackupFilename is in a form that ends with ".BAK",
/// and that the App's m_curOutputBackupFilename is in a form that ends with ".BAK.xml".
/// The wx version does not handle the legacy .adt binary file types/extensions.
///////////////////////////////////////////////////////////////////////////////
void CAdapt_ItDoc::MakeOutputBackupFilenames(wxString& curOutputFilename, bool bSaveAsXML)
{
    // input should be the current m_curOutputFilename value, and the current m_bSaveAsXML
    // value; the function assumes that the caller's value for m_curOutputFilename is
    // correct and complies with the m_bSaveAsXML value.

    // we calculate the primary and the alternate backup names here, because it is possible
    // that the backup file needs to be binary (for a m_bSaveAsXML = FALSE value) binary
    // when the doc file that was read in was xml; or the backup needs to be xml (for
    // compliance with m_bSaveAsXML = TRUE value) when the doc file that was read in was
    // binary. However, most of the time the m_bSaveAsXML flag and the filename extensions
    // already in place will comply, but we won't assume so, instead we use this function
    // to ensure compliance and we call it just prior to any circumstance which needs
    // correct backup primary and alternate filenames - such as in BackupDocument()
	wxString baseFilename = curOutputFilename;
	wxString thisBackupFilename;
	baseFilename = MakeReverse(baseFilename);
	int nFound = baseFilename.Find(_T('.'));
	wxString extn;
	if (nFound > -1)
	{
		nFound += 1;
		extn = baseFilename.Left(nFound); // include period in the extension
		thisBackupFilename = baseFilename.Mid(extn.Length());
	}
	else
	{
		// no extension
		thisBackupFilename = baseFilename;
	}	
	thisBackupFilename = MakeReverse(thisBackupFilename);
	if (bSaveAsXML)
	{
		// saving will be done in XML format, so backup filenames must comply with that

		// add the required extensions
		thisBackupFilename += _T(".BAK");
		gpApp->m_altOutputBackupFilename = thisBackupFilename;// the *.BAK filename is the alternative
		gpApp->m_curOutputBackupFilename = thisBackupFilename + _T(".xml"); // the complying backup 
																	 // filename is *.BAK.xml
	}
}

///////////////////////////////////////////////////////////////////////////////
/// \return		nothing
/// \param		event	-> unused wxCommandEvent
/// \remarks
/// Called from: the Tools menu "Split Document..." command.
/// Invokes the CSplitDialog dialog.
///////////////////////////////////////////////////////////////////////////////
void CAdapt_ItDoc::OnSplitDocument(wxCommandEvent& WXUNUSED(event))
{
	CSplitDialog d(gpApp->GetMainFrame());
	d.ShowModal();
}

///////////////////////////////////////////////////////////////////////////////
/// \return		nothing
/// \param		event	-> unused wxCommandEvent
/// \remarks
/// Called from: the Tools menu "Join Document..." command.
/// Invokes the CJoinDialog dialog.
///////////////////////////////////////////////////////////////////////////////
void CAdapt_ItDoc::OnJoinDocuments(wxCommandEvent& WXUNUSED(event))
{
	CJoinDialog d(gpApp->GetMainFrame());
	d.ShowModal();
}

///////////////////////////////////////////////////////////////////////////////
/// \return		nothing
/// \param		event	-> unused wxCommandEvent
/// \remarks
/// Called from: the Tools menu "Move Document..." command.
/// Invokes the CMoveDialog dialog.
///////////////////////////////////////////////////////////////////////////////
void CAdapt_ItDoc::OnMoveDocument(wxCommandEvent& WXUNUSED(event))
{
	CMoveDialog d(gpApp->GetMainFrame());
	d.ShowModal(); // We don't care about the results of the dialog - 
				   // it does all it's own work.
}

///////////////////////////////////////////////////////////////////////////////
/// \return		a list of source phrases composing the document
/// \param		FilePath	-> the path + name of the file
/// \remarks
/// Called from: the App's LoadSourcePhraseListFromFile() and 
/// CJoinDialog::OnBnClickedJoinNow().
/// In the wx version the current doc's m_pSourcePhrases list is not on the Doc, but on the
/// App. Since OnOpenDocument() always places the source phrases it retrieves from the
/// opened file into the m_pSourcePhrases list on the App, this function temporarily saves
/// those source phrases from the currently open Doc, in order to allow OnOpenDocument() to
/// save its new source phrases in m_pSourcePhrases on the App. Then, once we have the new
/// ones loaded we copy them to the list being returned, and repopulate m_pSourcePhrases
/// list with the original (open) document's source phrases.
/// I'm taking this approach rather than redesigning things at this point. Having all the
/// doc's members moved to the App was necessitated at the beginning of the wx version
/// conversion effort (because of the volatility of the doc's member pointers within the wx
/// doc-view framework). It would have been helpful to redesign some other routines
/// (OnOpenDocument and the XML doc parsing routines) to pass the list of source phrases
/// being built as a parameter rather than keeping it as a global list.
/// At any rate, here we need to juggle the source phrase pointer lists in order to
/// load a source phrase list from a document file.
///////////////////////////////////////////////////////////////////////////////
SPList *CAdapt_ItDoc::LoadSourcePhraseListFromFile(wxString FilePath)
{
	SPList *rv; // Return Value.

	CAdapt_ItDoc d; // needed to call non-static OnOpenDocument() below
	// wx version note: In the wx version the current doc's m_pSourcePhrases list
	// is not on the doc, but on the app. Since OnOpenDocument() always places the
	// source phrases it retrieves from the opened file into the m_pSourcePhrases
	// list on the App, we need to temporarily save those source phrases from the
	// currently open doc, in order to allow OnOpenDocument() to save its new
	// source phrases in m_pSourcePhrases on the App. Then, once we have the new ones
	// we can copy them to the list being returned, and repopulate m_pSourcePhrases
	// list with the original (open) document's source phrases.
	// I'm taking this approach rather than redesigning things at this point. Having
	// all the doc's members moved to the App was necessitated at the beginning of the
	// wx version conversion effort (because of the volatility of the doc's member
	// pointers within the wx doc-view framework). It would have been helpful to 
	// redesign some other routines (OnOpenDocument and the XML doc parsing routines)
	// to pass the list of source phrases being built as a parameter rather than keeping
	// it as a global list.
	// At any rate, here we need to juggle the source phrase pointer lists in order to
	// load a source phrase list from a document file.
	
	rv = new SPList();
	rv->Clear();

	gpApp->m_bWantSourcePhrasesOnly = true;
	SPList* m_pSourcePhrasesSaveFromApp = new SPList(); // a temp list to save the 
												// SPList of the currently open document
	// save the list of pointers from those on the app to a temp list 
	// (the App's list to be restored later)
	for (SPList::Node *node = gpApp->m_pSourcePhrases->GetFirst(); 
			node; node = node->GetNext())
	{
		m_pSourcePhrasesSaveFromApp->Append((CSourcePhrase*)node->GetData());
	}
    // pointers are now saved in the temp SPList, so clear the list on the App to be ready
    // to receive the new list within OnOpenDocument()
	gpApp->m_pSourcePhrases->Clear();
	d.OnOpenDocument(FilePath); // OnOpenDocument loads source phrases into 
								// m_pSourcePhrases on the App
	gpApp->m_bWantSourcePhrasesOnly = false;
	// copy the pointers to the list we are returning from LoadSourcePhraseListFromFile
	for (SPList::Node *node = gpApp->m_pSourcePhrases->GetFirst(); 
			node; node = node->GetNext())
	{
		rv->Append((CSourcePhrase*)node->GetData());
	}
	// now restore original App list
	gpApp->m_pSourcePhrases->Clear();
	for (SPList::Node *node = m_pSourcePhrasesSaveFromApp->GetFirst(); 
			node; node = node->GetNext())
	{
		gpApp->m_pSourcePhrases->Append((CSourcePhrase*)node->GetData());
	}
	// now clear and delete the temp save list
	m_pSourcePhrasesSaveFromApp->Clear();
	delete m_pSourcePhrasesSaveFromApp;
	m_pSourcePhrasesSaveFromApp = NULL;
	// lastly return the new list loaded from the file
	return rv;
}

///////////////////////////////////////////////////////////////////////////////
/// \return		nothing
/// \param      event   -> the wxUpdateUIEvent that is generated when the File Menu
///                        is about to be displayed
/// \remarks
/// Called from: The wxUpdateUIEvent mechanism when the associated menu item is selected,
/// and before the menu is displayed.
/// If Vertical Editing is in progress it disables the File Pack Document menu item and
/// returns immediately. It enables the menu item if there is a KB ready (even if only a
/// stub), and the document is loaded, and documents are to be saved as XML is turned on;
/// and glossing mode is turned off, otherwise the command is disabled.
/// BEW modified 13Nov09, when read-only access to project folder, don't allow pack doc
/// BEW 25Nov09, to allow pack doc but to use the m_bReadOnlyAccess flag to suppress
/// doing a project config file write and a doc save, but instead to just take the project
/// config file and doc files as they currently are on disk in order to do the pack
///////////////////////////////////////////////////////////////////////////////
void CAdapt_ItDoc::OnUpdateFilePackDoc(wxUpdateUIEvent& event)
{
	//if (gpApp->m_bReadOnlyAccess)
	//{
	//	event.Enable(FALSE);
	//	return;
	//}
	if (gbVerticalEditInProgress)
	{
		event.Enable(FALSE);
		return;
	}
    // enable if there is a KB ready (even if only a stub), and the document loaded, and
    // documents are to be saved as XML is turned on; and glossing mode is turned off
	if (gpApp->m_pLayout->GetStripArray()->GetCount() > 0 &&
		gpApp->m_bKBReady && gpApp->m_bSaveAsXML && !gbIsGlossing)
	{
		event.Enable(TRUE);
	}
	else
	{
		event.Enable(FALSE);
	}
}

///////////////////////////////////////////////////////////////////////////////
/// \return		nothing
/// \param      event   -> the wxUpdateUIEvent that is generated when the File Menu is about
///                         to be displayed
/// \remarks
/// Called from: The wxUpdateUIEvent mechanism when the associated menu item is selected, and before
/// the menu is displayed.
/// If Vertical Editing is in progress it disables the Unpack Document..." command on the File 
/// menu, otherwise it enables the item as long as glossing mode is turned off.
/// BEW modified 13Nov09, if read-only access to project folder, don't permit unpack doc
///////////////////////////////////////////////////////////////////////////////
void CAdapt_ItDoc::OnUpdateFileUnpackDoc(wxUpdateUIEvent& event)
{
	if (gpApp->m_bReadOnlyAccess)
	{
		event.Enable(FALSE);
		return;
	}
	if (gbVerticalEditInProgress)
	{
		event.Enable(FALSE);
		return;
	}
    // enable provided glossing mode is turned off; we want it to be able to work even if
    // there is no project folder created yet, nor even a KB and/or document; but right
    // from the very first launch
	if (!gbIsGlossing)
	{
		event.Enable(TRUE);
	}
	else
	{
		event.Enable(FALSE);
	}
}

///////////////////////////////////////////////////////////////////////////////
/// \return		nothing
/// \param		event	-> unused wxCommandEvent
/// \remarks
/// Called from: the "Pack Document..." command on the File menu.
/// Packing creates a zipped (compressed *.aip file) containing sufficient information for
/// a remote Adapt It user to recreate the project folder, project settings, and unpack and
/// load the document on the remote computer. OnFilePackDoc collects six kinds of
/// information:
/// source language name; target language name; Bible book information; current output
/// filename for the document; the current project configuration file contents; the
/// document in xml format.
///////////////////////////////////////////////////////////////////////////////
void CAdapt_ItDoc::OnFilePackDoc(wxCommandEvent& WXUNUSED(event))
{
    // OnFilePackDoc(), for a unicode build, converts to UTF-8 internally, and so uses
    // CBString for the final output (config file and xml doc file are UTF-8 already).
	wxString packStr;
	packStr.Empty();

    // first character needs to be a 1 for the regular app doing the pack, or a 2 for the
    // Unicode app (as resulting from sizeof(wxChar) ) and the unpacking app will have to
    // check that it is matching; and if not, warn user that continuing the unpack might
    // not result in properly encoded text in the docment (but allow him to continue,
    // because if source and target text are all ASCII, the either app can read the packed
    // data from the other and give valid encodings in the doc when unpacked.)
	//
    // whm Note: The legacy logic doesn't work cross-platform! The sizeof(char) and
    // sizeof(w_char) is not constant across platforms. On Windows sizeof(char) is 1 and
    // sizeof(w_char) is 2; but on all Unix-based systems (i.e., Linux and Mac OS X) the
    // sizeof(char) is 2 and sizeof(w_char) is 4. We can continue to use '1' to indicate
    // the file was packed by an ANSI version, and '2' to indicate the file was packed by
    // the Unicode app for back compatibility. However, the numbers cannot signify the size
    // of char and w_char across platforms. They can only be used as pure signals for ANSI
    // or Unicode contents of the packed file. Here in OnFilePackDoc we will save the
    // string _T("1") if we're packing from an ANSI app, or the string _T("2") if we're
    // packing from a Unicode app. See DoUnpackDocument() for how we can interpret "1" and
    // "2" in a cross-platform manner.
	//
#ifdef _UNICODE
	packStr = _T("2");
#else
	packStr = _T("1");
#endif

	packStr += _T("|*0*|"); // the zeroth unique delimiter

	// get source and target language names, or whatever is used for these
	wxString curSourceName;
	wxString curTargetName;
	gpApp->GetSrcAndTgtLanguageNamesFromProjectName(gpApp->m_curProjectName, 
											curSourceName, curTargetName);

    // get the book information (mode flag, disable flag, and book index; as ASCII string
    // with colon delimited fields)
	wxString bookInfoStr;
	bookInfoStr.Empty();
	if (gpApp->m_bBookMode)
	{
		bookInfoStr = _T("1:");
	}
	else
	{
		bookInfoStr = _T("0:");
	}
	if (gpApp->m_bDisableBookMode)
	{
		bookInfoStr += _T("1:");
	}
	else
	{
		bookInfoStr += _T("0:");
	}
	if (gpApp->m_nBookIndex != -1)
	{
		bookInfoStr << gpApp->m_nBookIndex;
	}
	else
	{
		bookInfoStr += _T("-1");
	}

	wxLogNull logNo; // avoid spurious messages from the system

	// update and save the project configuration file
	bool bOK = TRUE; // whm initialized, BEW changed to default TRUE 25Nov09
	// BEW added flag to the following test on 25Nov09
	if (!gpApp->m_curProjectPath.IsEmpty() && !gpApp->m_bReadOnlyAccess)
	{
		if (gpApp->m_bUseCustomWorkFolderPath && !gpApp->m_customWorkFolderPath.IsEmpty())
		{
			// whm 10Mar10, must save using what paths are current, but when the custom
			// location has been locked in, the filename lacks "Admin" in it, so that it
			// becomes a "normal" project configuration file in m_curProjectPath at the 
			// custom location.
			if (gpApp->m_bLockedCustomWorkFolderPath)
				bOK = gpApp->WriteConfigurationFile(szProjectConfiguration, gpApp->m_curProjectPath,projectConfigFile);
			else
				bOK = gpApp->WriteConfigurationFile(szAdminProjectConfiguration,gpApp->m_curProjectPath,projectConfigFile);
		}
		else
		{
			bOK = gpApp->WriteConfigurationFile(szProjectConfiguration, gpApp->m_curProjectPath,projectConfigFile);
		}
		// original code below
		//bOK = gpApp->WriteConfigurationFile(szProjectConfiguration,gpApp->m_curProjectPath,projectConfigFile);
	}
	// we don't expect any failure here, so an English message hard coded will do
	if (!bOK)
	{
		wxMessageBox(_T(
		"Writing out the configuration file failed in OnFilePackDoc, command aborted\n"),
		_T(""), wxICON_EXCLAMATION);
		return;
	}

	// get the size of the configuration file, in bytes
	wxFile f;
	wxString configFile = gpApp->m_curProjectPath + gpApp->PathSeparator + 
									szProjectConfiguration + _T(".aic");
	int nConfigFileSize = 0;
	if (f.Open(configFile,wxFile::read))
	{
		nConfigFileSize = f.Length();
		wxASSERT(nConfigFileSize);
	}
	else
	{
		wxMessageBox(_T(
	"Getting the configuration file's size failed in OnFilePackDoc, command aborted\n"),
		_T(""), wxICON_EXCLAMATION);
		return;
	}
	f.Close(); // needed because in wx we opened the file

	// save the doc as XML (this handler can only be invoked when m_bSaveAsXML is TRUE)
	bool bSavedOK = TRUE;
	// BEW added test on 25Nov09, so documents can be packed when user has read only access
	if (!gpApp->m_bReadOnlyAccess)
	{
		bSavedOK = DoFileSave(TRUE); // TRUE - show wait/progress dialog
	}

	// construct the absolute path to the document as it currently is on disk; if the
	// local user has read-only access, the document on disk may not have been recently
	// saved. (Read-only access must not force document saves on a remote user
	// who has ownership of writing permission for data in the project; otherwise, doing
	// so could cause data to be lost)
	wxString docPath;
	if (gpApp->m_bBookMode && !gpApp->m_bDisableBookMode)
	{
		docPath = gpApp->m_bibleBooksFolderPath;
	}
	else
	{
		docPath = gpApp->m_curAdaptionsPath;
	}
	docPath += gpApp->PathSeparator + gpApp->m_curOutputFilename; // it will have .xml extension

	// get the size of the document's XML file, in bytes
	int nDocFileSize = 0;
	if (f.Open(docPath,wxFile::read))
	{
		nDocFileSize = f.Length();
		wxASSERT(nDocFileSize);
	}
	else
	{
		wxMessageBox(_T(
	"Getting the document file's size failed in OnFilePackDoc, command aborted\n"),
		_T(""), wxICON_EXCLAMATION);
		return;
	}
	f.Close(); // needed for wx version which opened the file to determine its size

	// construct the composed information required for the pack operation, as a wxString
	packStr += curSourceName;
	packStr += _T("|*1*|"); // the first unique delimiter
	packStr += curTargetName;
	packStr += _T("|*2*|"); // the second unique delimiter
	packStr += bookInfoStr;
	packStr += _T("|*3*|"); // the third unique delimiter
	packStr += gpApp->m_curOutputFilename;
	packStr += _T("|*4*|"); // the fourth unique delimiter

    // set up the byte string for the data, taking account of whether we have unicode data
    // or not
#ifdef _UNICODE
	CBString packByteStr = gpApp->Convert16to8(packStr);
#else
	CBString packByteStr(packStr);
#endif

	// from here on we work with bytes, and so use CBString rather than wxString for the data

	if (!f.Open(configFile,wxFile::read))
	{
		// if error, just return after telling the user about it -- English will do, 
		// it shouldn't happen
		wxString s;
		s = s.Format(_T(
"Could not open a file stream for project config, in OnFilePackDoc(), for file %s"),
		gpApp->m_curProjectPath.c_str());
		wxMessageBox(s,_T(""), wxICON_EXCLAMATION);
		return; 
	}
	int nFileLength = nConfigFileSize; // our files won't require more than 
									   // an int for the length

    // create a buffer large enough to receive the whole lot, allow for final null byte (we
    // don't do anything with the data except copy it and resave it, so a char buffer will
    // do fine for unicode too), then fill it
	char* pBuff = new char[nFileLength + 1];
	memset(pBuff,0,nFileLength + 1);
	int nReadBytes = f.Read(pBuff,nFileLength);
	if (nReadBytes < nFileLength)
	{
		wxMessageBox(_T(
		"Project file read was short, some data missed so abort the command\n"),
		_T(""), wxICON_EXCLAMATION);
		return; 
	}
	f.Close(); // assume no errors

	// append the configuration file's data to packStr and add the next 
	// unique delimiter string
	packByteStr += pBuff;
	packByteStr += "|*5*|"; // the fifth unique delimiter

	// clear the buffer, then read in the document file in similar fashion & 
	// delete the buffer when done
	delete[] pBuff;
	if (!f.Open(docPath,wxFile::read))
	{
		// if error, just return after telling the user about it -- English will do, 
		// it shouldn't happen
		wxString s;
		s = s.Format(_T(
"Could not open a file stream for the XML document as text, in OnFilePackDoc(), for file %s"),
		docPath.c_str());
		wxMessageBox(s,_T(""), wxICON_EXCLAMATION);
		return; 
	}
	nFileLength = nDocFileSize; // our files won't require more than an int for the length
	pBuff = new char[nFileLength + 1];	
	memset(pBuff,0,nFileLength + 1);
	nReadBytes = f.Read(pBuff,nFileLength);
	if (nReadBytes < nFileLength)
	{
		wxMessageBox(_T(
		"Document file read was short, some data missed so abort the command\n"),
		_T(""), wxICON_EXCLAMATION);
		return; 
	}
	f.Close(); // assume no errors
	packByteStr += pBuff;
	delete[] pBuff;

    // packByteStr now is complete; so we must ask the user to save it and then do so to
    // his nominated destination

	// whm Pack design notes for future consideration:
	// 1. Initial design calls for the packing/compression of a single Adapt It document
	//    at a time. With the freeware zip utils provided by Lucian Eischik (based on zlib
	//    and info-zip) it would be relatively easy in the future to have the capability of
	//    packing multiple files into the .aip zip archive.
	// 2. Packing/zipping can be accomplished by doing it on external files (as done below)
	//    or by doing it in internal buffers (in memory).
	// 3. If in future we want to do the packing/zipping in internal buffers, we would do it
	//    with the contents of packByteStr after this point in OnFilePackDoc, and before
	//    the pBuf is written out via CFile ff below.
	// 4. If done in a buffer, after compression we could add the following Warning statement 
	//    in uncompressed form to the beginning of the compressed character buffer (before 
	//    writing it to the .aip file): "|~WARNING: DO NOT ATTEMPT TO CHANGE THIS FILE WITH 
	//    AN EDITOR OR WORD PROCESSOR! IT CAN ONLY BE UNCOMPRESSED WITH THE UNPACK COMMAND
	//    FROM WITHIN ADAPT IT VERSION 3.X. COMPRESSED DATA FOLLOWS:~|" 
	//    The warning would serve as a warning to users if they were to try to load the file
	//    into a word processor, not to edit it or save it within the word processor,
	//    otherwise the packed file would be corrupted. The warning (without line breaks
	//    or quote marks) would be 192 bytes long. When the file would be read from disk
	//    by DoUnpackDocument, this 192 byte warning would be stripped off prior to
	//    uncompressing the remaining data using the zlib tools.
	
    // whm 22Sep06 update: The wx version now uses wxWidget's built-in wxZipOutputStream
    // facilities for compressing and uncompressing packed documents, hence, it no longer
    // needs the services of Lucian Eischik's zip and unzip library. The wxWidget's zip
    // format is based on the same free-ware zlib library, so there should be no problem
    // zipping and unzipping .aip files produced by the MFC version or the WX version.

	wxString filter;
	wxString DefaultExt;
	wxString defaultDir;
	defaultDir = gpApp->m_curProjectPath;
	// make a suitable default output filename for the packed data
	wxString exportFilename = gpApp->m_curOutputFilename;
	int len = exportFilename.Length();
	exportFilename.Remove(len-3,3); // remove the xml extension
	exportFilename += _T("aip"); // make it a *.aip file type

	// get a file Save As dialog for Source Text Output
	DefaultExt = _T("aip");
	filter = _("Packed Documents (*.aip)|*.aip||"); // set to "Packed Document (*.aip) *.aip"

	wxFileDialog fileDlg(
		(wxWindow*)wxGetApp().GetMainFrame(), // MainFrame is parent window for file dialog
		_("Filename For Packed Document"),
		defaultDir,
		exportFilename,
		filter,
		wxFD_SAVE | wxFD_OVERWRITE_PROMPT); 
		// wxHIDE_READONLY was deprecated in 2.6 - the checkbox is never shown
		// GDLC wxSAVE & wxOVERWRITE_PROMPT were deprecated in 2.8
	fileDlg.Centre();

	// set the default folder to be shown in the dialog (::SetCurrentDirectory does not
	// do it) Probably the project folder would be best.
	bOK = ::wxSetWorkingDirectory(gpApp->m_curProjectPath);

	if (fileDlg.ShowModal() != wxID_OK)
	{
		// user cancelled file dialog so return to what user was doing previously, because
		// this means he doesn't want the Pack Document... command to go ahead
		return; 
	}

	// get the length of the total byte string in packByteStr (exclude the null byte)
	int fileLength = packByteStr.GetLength();

	// get the user's desired path
	wxString exportPath = fileDlg.GetPath();
	
    // wx version: we use the wxWidgets' built-in zip facilities to create the zip file,
    // therefore we no longer need the zip.h, zip.cpp, unzip.h and unzip.cpp freeware files
    // required for the MFC version.
	// first, declare a simple output stream using the temp zip file name
	// we set up an input file stream from the file having the raw data to pack
	wxString tempZipFile;
	wxString nameInZip;
    int extPos = exportPath.Find(_T(".aip"));
	tempZipFile = exportPath.Left(extPos);
	extPos = exportFilename.Find(_T(".aip"));
	nameInZip = exportFilename.Left(extPos);
	nameInZip = nameInZip + _T(".aiz");
	
	wxFFileOutputStream zippedfile(exportPath);
	// then, declare a zip stream placed on top of it (as zip generating filter)
	wxZipOutputStream zipStream(zippedfile);
    // wx version: Since our pack data is already in an internal buffer in memory, we can
    // use wxMemoryInputStream to access packByteStr; run it through a wxZipOutputStream
    // filter and output the resulting zipped file via wxFFOutputStream.
	wxMemoryInputStream memStr(packByteStr,fileLength);
	// create a new entry in the zip file using the .aiz file name
	zipStream.PutNextEntry(nameInZip);
	// finally write the zipped file, using the data associated with the zipEntry
	zipStream.Write(memStr);
	if (!zipStream.Close() || !zippedfile.Close() || 
		zipStream.GetLastError() == wxSTREAM_WRITE_ERROR) // Close() finishes writing the 
													// zip returning TRUE if successfully
	{
		wxString msg;
		msg.Format(_("Could not write to the packed/zipped file: %s"),exportPath.c_str());
		wxMessageBox(msg,_T(""),wxICON_ERROR);
	} 
}

///////////////////////////////////////////////////////////////////////////////
/// \return		nothing
/// \param		event	-> unused wxCommandEvent
/// \remarks
/// Called from: the "Unpack Document..." command on the File menu.
/// OnFileUnpackDoc gets the name of a packed *.aip file from the user, uncompresses it and 
/// calls DoUnpackDocument() to do the remaining work of unpacking the document and loading
/// it into Adapt It ready to do work. 
/// If a document of the same name already exists on the destination machine in 
/// the same folder, the user is warned before the existing doc is overwritten by the document 
/// extracted from the packed file.
/// The .aip files pack with the Unicode version of Adapt It cannot be unpacked with the regular
/// version of Adapt It, nor vice versa.
///////////////////////////////////////////////////////////////////////////////
void CAdapt_ItDoc::OnFileUnpackDoc(wxCommandEvent& WXUNUSED(event))
{
	// OnFileUnpackDoc is the handler for the Unpack Document... command on the File menu. 
	// first, get the file and load it into a CBString
	wxString message;
	message = _("Load And Unpack The Compressed Document"); //IDS_UNPACK_DOC_TITLE
	wxString filter;
	wxString defaultDir;
	defaultDir = gpApp->m_curProjectPath; 
	filter = _("Packed Documents (*.aip)|*.aip||"); //IDS_PACKED_DOC_EXTENSION

	wxFileDialog fileDlg(
		(wxWindow*)wxGetApp().GetMainFrame(), // MainFrame is parent window for file dialog
		message,
		defaultDir,
		_T(""), // file name is null string
		filter,
		wxFD_OPEN); 
		// wxHIDE_READONLY was deprecated in 2.6 - the checkbox is never shown
		// GDLC wxOPEN weredeprecated in 2.8
	fileDlg.Centre();

	wxLogNull logNo; // avoid spurious messages from the system

	// open as modal dialog
	int returnValue = fileDlg.ShowModal();
	if (returnValue == wxID_CANCEL)
	{
		return; // user Cancelled, or closed the dialog box
	}
	else // must be wxID_OK
	{
		wxString pathName;
		pathName = fileDlg.GetPath();

        // whm Note: Since the "file" variable is created below and passed to
        // DoUnpackDocument and the DoUnpackDocument expects the file to already be
        // decompressed, we need decompress the file here before calling DoUnpackDocument.
        // We uncompress the packed file from the .aip compressed archive. It will have the
        // extension .aiz. We call DoUnpackDocument() on the .aiz file, then delete the
        // .aiz file which is of no usefulness after the unpacking and loading of the
        // document into Adapt It; we also would not want it hanging around for the user to
        // try to unpack it again which would fail because the routine would try to
        // uncompress an already uncompressed file and fail.

        // The wxWidgets version no longer needs the services of the separate freeware
        // unzip.h and unzip.cpp libraries.
        // whm 22Sep06 modified the following to use wxWidgets' own built-in zip filters
        // which act on i/o streams.
        // TODO: This could be simplified further by streaming the .aip file via
        // wxZipInputStream to a wxMemoryInputStream, rather than to an external
        // intermediate .aiz file, thus reducing complexity and the need to manipulate
        // (create, delete, rename) the external files.
		wxZipEntry* pEntry;
		// first we create a simple output stream using the zipped .aic file (pathName)
		wxFFileInputStream zippedfile(pathName);
        // then we construct a zip stream on top of this one; the zip stream works as a
        // "filter" unzipping the stream from pathName
		wxZipInputStream zipStream(zippedfile);
		wxString unzipFileName;
		pEntry = zipStream.GetNextEntry(); // gets the one and only zip entry in the 
										   // .aip file
		unzipFileName = pEntry->GetName(); // access the meta-data
        // construct the path to the .aiz file so that is goes temporarily in the project
        // folder this .aiz file is erased below after DoUnPackDocument is called on it
		pathName = gpApp->m_workFolderPath + gpApp->PathSeparator + unzipFileName;
		// get a buffered output stream
		wxFFileOutputStream outFile(pathName);
		// write out the filtered (unzipped) stream to the .aiz file
		outFile.Write(zipStream); // this form writes from zipStream to outFile until a 
								  // stream "error" (i.e., end of file)
		delete pEntry; // example in wx book shows the zip entry data being deleted
		outFile.Close();

		// get a CFile and do the unpack
		wxFile file;
		// In the wx version we need to explicitly call Open on the file to proceed.
		if (!file.Open(pathName,wxFile::read))
		{
			wxString msg;
			msg = msg.Format(_(
"Error uncompressing; cannot open the file: %s\n Make sure the file is not being used by another application and try again."),
			pathName.c_str());
			wxMessageBox(msg,_T(""),wxICON_WARNING);
			return;
		}
		if (!DoUnpackDocument(&file))//whm changed this to return bool for better error recovery
			return; // DoUnpackDocument issues its own error messages if it encounters an error

		// lastly remove the .aiz temporary file that was used to unpack from
		// leaving the compressed .aip in the work folder
		if (!::wxRemoveFile(pathName))
		{
			// if there was an error, we just get no unpack done, but app can continue; and
			// since we expect no error here, we will use an English message
			wxString strMessage;
			strMessage = strMessage.Format(_("Error removing %s after unpack document command."),
			pathName.c_str());
			wxMessageBox(strMessage,_T(""), wxICON_EXCLAMATION);
			return;
		}
	}
	return;
}

///////////////////////////////////////////////////////////////////////////////
/// \return		TRUE if the unpack operation was successful, FALSE otherwise
/// \param		pFile	-> pointer to the wxFile object being unpacked
/// \remarks
/// Called from: the Doc's OnFileUnpackDocument().
/// Does the main work of unpacking the uncompressed file received from OnFileUnpackDocument().
/// It creates the required folder structure (including project folder) or 
/// makes that project current if it already exists on the destination machine, and then 
/// updates its project configuration file, and stores the xml document file in whichever of 
/// the Adaptations folder or one of its book folders if pack was done from a book folder, 
/// and then reads in the xml document, parses it and sets up the document and view leaving 
/// the user in the project and document ready to do work. 
/// If a document of the same name already exists on the destination machine in 
/// the same folder, the user is warned before the existing doc is overwritten by the document 
/// extracted from the packed file.
/// The .aip files pack with the Unicode version of Adapt It cannot be unpacked with the regular
/// version of Adapt It, nor vice versa.
///////////////////////////////////////////////////////////////////////////////
bool CAdapt_ItDoc::DoUnpackDocument(wxFile* pFile) // whm changed to return bool 22Jul06
{
	CAdapt_ItView* pView = gpApp->GetView();

	// get the file size
	int nFileSize = (int)pFile->Length();
	int nResult;

	// create a CBString with an empty buffer large enough for all this file
	CBString packByteStr;
	char* pBuff = packByteStr.GetBuffer(nFileSize + 1);

	// read in the file & close it
	int nReadBytes = pFile->Read(pBuff,nFileSize);
	if (nReadBytes < nFileSize)
	{
		wxMessageBox(_T(
"Compressed document file read was short, some data missed so abort the command.\n"),
			_T(""), wxICON_EXCLAMATION);
		return FALSE; 
	}
	pFile->Close(); // assume no errors

	// get the length (private member) set correctly for the CBString
	packByteStr.ReleaseBuffer();

	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// whm Note 19Jan06: 
    // If the design included the addition of a warning string embedded in the compressed
    // file, and/or we wished to uncompress the data within an internal buffer, the
    // following considerations would need to be taken into account:
    // 1. Remove the Warning statement (see OnFilePackDoc) from the packByteStr which
    //    consists of the first 192 bytes of the file.
    // 2. The zlib inflate (decompression) call must be made at this point if executed on
    //    the compressed part of packByteStr expanding and uncompressing the data to a
    //    larger work buffer (CBString???), and the buffer receiving the uncompressed data
    //    needs to be used instead of the packByteStr one used below.
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    // Start extracting info. First we want to know whether Adapt It or Adapt It Unicode
    // did the packing? If our unpacking app is not same one, warn the user and disallow
    // the unpacking (The first byte will be an ascii 1 or 2, 1 for regular app did the
    // pack, 2 if unicode app did it)
	int offset = 0;
	char chDigit[2] = {'\0','\0'};

    // if the user has used winzip to unpack the packed doc file, maybe edited it, then
    // used Winzip to zip it and then changed the extension to .aip, Winzip (possibly only
    // if it detects UTF-8 data in the file, but I'm not sure of this) adds 3 bytes to the
    // start of the file - an i followed by a double right wedge character followed by an
    // upside down question mark character (as unsigned chars these have values of 239,
    // 187, and 191, respectively). This is all that prevents Adapt It from being able to
    // correctly unpack such a document - so to make it able to do so, instead of assuming
    // that the first character is a digit, we will scan till we find the first digit
    // (which will be 1 or 2) and then proceed with our unpack.
	unsigned char aOne = 49; // ASCII for digit 1 (isdigit() depends on locale, so 
							 // is unsafe to use here)
	unsigned char aTwo = 50; // ASCII for 2
	unsigned char charAtOffset = pBuff[offset];
	int limit = 7; // look at no more than first 6 bytes
	int soFar = 1;
	bool bFoundOneOrTwo = FALSE;
	while (soFar <= limit)
	{
		if (charAtOffset == aOne || charAtOffset == aTwo)
		{
			// we've successfully scanned past Winzip's inserted material
			offset = soFar - 1;
			bFoundOneOrTwo = TRUE;
			break;
		}
		else
		{
			// didn't find a 1 or 2, so try next byte
			soFar++;
			charAtOffset = pBuff[soFar - 1];
		}
	}
	if (!bFoundOneOrTwo)
	{
		// IDS_UNPACK_INVALID_PREDATA
		wxMessageBox(_T(
"Unpack failure. The uncompressed data has more than six unknown bytes preceding the digit 1 or 2, making interpretation impossible. Command aborted."),
		_T(""), wxICON_WARNING);
		return FALSE;
	}

    // we found a digit either at the start or no further than 7th byte from start, so
    // assume all is well formed and proceed from the offset value
	chDigit[0] = packByteStr[offset];
	offset += 6;
	packByteStr = packByteStr.Mid(offset); // remove extracted app type code 
										   // char & its |*0*| delimiter
	
    // whm Note: The legacy logic doesn't work cross-platform! The sizeof(char) and
    // sizeof(w_char) is not constant across platforms. On Windows sizeof(char) is 1 and
    // sizeof(w_char) is 2; but on all Unix-based systems (i.e., Linux and Mac OS X) the
    // sizeof(char) is 2 and sizeof(w_char) is 4. We can continue to use '1' to indicate
    // the file was packed by an ANSI version, and '2' to indicate the file was packed by
    // the Unicode app. However, the numbers cannot signify the size of char and w_char
    // across platforms. They can only be used as pure signals for ANSI or Unicode contents
    // of the packed file. Here in DoUnpackDocument we have to treat the string _T("1") as
    // an error if we're unpacking from within a Unicode app, or the string _T("2") as an
    // error if we're unpacking from an ANSI app.
    // In OnFilePackDoc() we simply pack the file using "1" if we are packing it from an
    // ANSI app; and we pack the file using "2" when we're packing it from a Unicode app,
    // regardless of the result returned by the sizeof operator on wxChar.
	//
	int nDigit = atoi(chDigit);
	// are we in the same app type?
#ifdef _UNICODE
	// we're a Unicode app and the packed file is ANSI, issue error and abort
	if (nDigit == 1)
#else
	// we're an ASNI app and the packed file is Unicode, issue error and abort
	if (nDigit == 2)
#endif
	{
		// mismatched application types. Doc data won't be rendered right unless all text was
		// ASCII 
		CUnpackWarningDlg dlg(gpApp->GetMainFrame());
		if (dlg.ShowModal() == wxID_OK)
		{
            // the only option is to halt the unpacking (there is only the one button);
            // because unicode app's config file format for punctuation is not compatible
            // with the regular app's config file (either type), and so while the doc would
            // render okay if ascii, the fact that the encapsulated config files are
            // incompatible makes it not worth allowing the user to continue
			return FALSE;
		}
	}

    // close the document and project currently open, clear out the book mode information
    // to defaults and the mode off, store and then erase the KBs, ready for the new
    // project and document being unpacked
	pView->CloseProject();

	// extract the rest of the information needed for setting up the document
	offset = packByteStr.Find("|*1*|");
	CBString srcName = packByteStr.Left(offset);
	offset += 5;
	packByteStr = packByteStr.Mid(offset); // remove the extracted source 
										   // language name & delimiter
	offset = packByteStr.Find("|*2*|");
	CBString tgtName = packByteStr.Left(offset);
	offset += 5;
	packByteStr = packByteStr.Mid(offset); // remove the extracted target 
										   // language name & delimiter
	offset = packByteStr.Find("|*3*|");
	CBString bookInfo = packByteStr.Left(offset);
	offset += 5;
	packByteStr = packByteStr.Mid(offset); // remove the extracted book 
										   // information & delimiter

    // offset now points at the start of the UTF-8 current output filename; if the packing
    // was done in the Unicode application, this will have to be converted back to UTF-16
    // further down before we make use of it

    // The book information has colon delimited subfields. Even in the unicode application
    // we can reliably compute from the char string without having to convert to UTF-16, so
    // do it now.
	int nFound = bookInfo.Find(':');
	wxASSERT(nFound);
	CBString theBool = bookInfo.Left(nFound);
	if (theBool == "1")
	{
		gpApp->m_bBookMode = TRUE;
	}
	else
	{
		gpApp->m_bBookMode = FALSE;
	}
	nFound++;
	bookInfo = bookInfo.Mid(nFound);
	nFound = bookInfo.Find(':');
	theBool = bookInfo.Left(nFound);
	if (theBool == "1")
	{
		gpApp->m_bDisableBookMode = TRUE;
	}
	else
	{
		gpApp->m_bDisableBookMode = FALSE;
	}
	nFound++;
	CBString theIndex = bookInfo.Mid(nFound);
	gpApp->m_nBookIndex = atoi(theIndex.GetBuffer()); // no ReleaseBuffer call 
            // is needed the later SetupDirectories() call will create the book folders
            // if necessary, and set up the current book folder and its path from the
            // m_nBookIndex value

	// extract the UTF-8 form of the current output filename
	offset = packByteStr.Find("|*4*|");
	CBString utf8Filename = packByteStr.Left(offset);
	offset += 5;
	packByteStr = packByteStr.Mid(offset); // remove the extracted utf-8 filename 
			// & delimiter (what remains is the project config file & document file)

    // we could be in the Unicode application, so we here might have to convert our srcName
    // and tgtName CBStrings into (Unicode) CStrings; we'll delay setting
    // m_curOutputFilename (using storeFilenameStr) to later on, to minimize the potential
    // for unwanted erasure.
#ifdef _UNICODE
	wxString sourceName;
	wxString targetName;
	wxString storeFilenameStr;
	gpApp->Convert8to16(srcName,sourceName);
	gpApp->Convert8to16(tgtName,targetName);
	gpApp->Convert8to16(utf8Filename,storeFilenameStr);
#else
	wxString sourceName = srcName.GetBuffer();
	wxString targetName = tgtName.GetBuffer();
	wxString storeFilenameStr(utf8Filename);
#endif

	// we now can set up the directory structures, if they are not already setup
	gpApp->m_sourceName = sourceName;
	gpApp->m_targetName = targetName;
	gpApp->m_bUnpacking = TRUE; // may be needed in SetupDirectories() 
								// if destination machine has same project folder
	bool bSetupOK = gpApp->SetupDirectories();
	if (!bSetupOK)
	{
		gpApp->m_bUnpacking = FALSE;
		wxMessageBox(_T(
"SetupDirectories returned false for Unpack Document.... The command will be ignored.\n"),
		_T(""), wxICON_EXCLAMATION);
		return FALSE;
	}
	gpApp->m_bUnpacking = FALSE;

	gpApp->m_bSaveAsXML = TRUE;

    // check for the same document already in the project folder - if it's there, then ask
    // the user whether or not to have the being-unpacked-one overwrite it
	wxFile f;

	// save various current paths so they can be restored if the user bails out because of
	// a choice not to overwrite an existing document with the one being unpacked
	wxString saveMFCfilename = GetFilename(); // m_strPathName is internal to MFC's doc-view
	wxString saveBibleBooksFolderPath = gpApp->m_bibleBooksFolderPath;
	wxString saveCurOutputFilename = gpApp->m_curOutputFilename;
	wxString saveCurAdaptionsPath = gpApp->m_curAdaptionsPath;
	wxString saveCurOutputPath = gpApp->m_curOutputPath;

	// set up the paths consistent with the unpacked info
	gpApp->m_curOutputFilename = storeFilenameStr;
	if (gpApp->m_bBookMode && !gpApp->m_bDisableBookMode)
	{
		// m_strPathName is a member of MFC's Document class
		SetFilename(gpApp->m_bibleBooksFolderPath + gpApp->PathSeparator + 
														gpApp->m_curOutputFilename,TRUE);
		gpApp->m_curOutputPath = gpApp->m_bibleBooksFolderPath + gpApp->PathSeparator + 
														gpApp->m_curOutputFilename;
	}
	else
	{
		SetFilename(gpApp->m_curAdaptionsPath + gpApp->PathSeparator + 
														gpApp->m_curOutputFilename,TRUE);
		gpApp->m_curOutputPath = gpApp->m_curAdaptionsPath + gpApp->PathSeparator + 
														gpApp->m_curOutputFilename;
	}
	
    // if the document does not exist in the unpacking computer yet, then an attempt to get
    // its status struct will return FALSE - we need to check both possible paths
	bool bAskUser = FALSE;
	bool bItsXML = TRUE;
	if (::wxFileExists(gpApp->m_curOutputPath)) //if (f.GetStatus(m_curOutputPath,status))
	{
		// the xml document file is in the folder, so the user must be asked 
		// whether to overwrite or not
		bAskUser = TRUE;
	}

	wxString s1,s2,s3,msg;
	wxFileName fn(gpApp->m_curOutputPath);
	if (bAskUser)
	{
		//IDS_UNPACK_ASK_OVERWRITE
		s1 = _(
"There is a document of the same name in an Adapt It project of the same name on this computer.");
		s2 = s2.Format(_("\n      Document name: %s"),fn.GetFullName().c_str());
		s3 = s3.Format(_("\n      Project path : %s"),fn.GetPath().c_str());
		msg = msg.Format(_(
"%s%s%s\nDo you want the document being unpacked to overwrite the one already on this computer?"),
		s1.c_str(),s2.c_str(),s3.c_str());
		nResult = wxMessageBox(msg, _T(""), wxYES_NO | wxICON_INFORMATION);
		if(nResult  == wxYES)
		{
			// user wants the current file overwritten...
            // we have a valid status struct for it, so use it to just remove it here;
            // doing it this way we can be sure we get rid of it; we can't rely on the
            // CFile::modeCreate style bit causing the file to be emptied first because the
            // being-unpacked doc will be xml but the existing doc file on the destination
            // machine might be binary (ie. have .adt extension)
			if (bItsXML)
			{
				if (!::wxRemoveFile(gpApp->m_curOutputPath))
				{
					wxString thismsg;
					thismsg = thismsg.Format(_(
					"Failed removing %s before overwrite."), 
					gpApp->m_curOutputPath.c_str());
					wxMessageBox(thismsg,_T(""),wxICON_WARNING);
					goto a; // restore paths & exit, allow app to continue
				}
			}
		}
		else
		{
			// abort the unpack - this means we should restore all the saved path strings
			// before we return
a:			SetFilename(saveMFCfilename,TRUE); //m_strPathName = saveMFCfilename;
			gpApp->m_bibleBooksFolderPath = saveBibleBooksFolderPath;
			gpApp->m_curOutputFilename = saveCurOutputFilename;
			gpApp->m_curAdaptionsPath = saveCurAdaptionsPath;
			gpApp->m_curOutputPath = saveCurOutputPath;
			// In the wx vesion m_bSaveAsXML is always true

			// restore the earlier document to the main window, if we have a valid path to
			// it
			wxString path = gpApp->m_curOutputPath;
			path = MakeReverse(path);
			int nFound1 = path.Find(_T("lmx."));
			if (nFound1 == 0) 
			{ 
                // m_curOutputPath is a valid path to a doc in a project's Adaptations or
                // book folder so open it again - the project is still in effect
				bool bGotItOK = OnOpenDocument(gpApp->m_curOutputPath);
				if (!bGotItOK)
				{
                    // some kind of error -- don't warn except for a beep, just leave the
                    // window blank (the user can instead use wizard to get a doc open)
					::wxBell();
				}
			}
			return FALSE;
		}
	}

	// if we get to here, then it's all systems go for updating the configuration file and
	// loading in the unpacked document and displaying it in the main window

	// next we must extract the embedded project configuration file
	offset = packByteStr.Find("|*5*|");
	CBString projConfigFileStr = packByteStr.Left(offset);
	offset += 5;
	packByteStr = packByteStr.Mid(offset); // remove the extracted configuration file 
                    // information & delimiter & the remainder now in packByteStr is the
                    // xml document file

	// construct the path to the project's configuration file so it can be saved to the 
	// project folder
	wxString projectPath = gpApp->m_curProjectPath;
	projectPath += gpApp->PathSeparator;
	projectPath += szProjectConfiguration;
	projectPath += _T(".aic");

    // temporarily rename any project file of this name already in the project folder - if
    // the new one fails to be written out, we must restore this renamed one before we
    // return to the caller, but if the new one is written out we must then delete this
    // renamed one
	wxString renamedPath;
	renamedPath.Empty();
	bool bRenamedConfigFile = FALSE;
	if (::wxFileExists(projectPath))
	{
		// do the renaming
		renamedPath = projectPath;
		int len = projectPath.Length();
		renamedPath.Remove(len-3,3); // delete the aic extension
		renamedPath += _T("BAK"); // make it a 'backup' type temporarily in case user 
								  // ever sees it in Win Explorer
		if (!::wxRenameFile(projectPath,renamedPath))
		{
			wxString message;
			message = message.Format(_(
			"Error renaming earlier configuration file with path %s."),
			projectPath.c_str());
			message += _("  Aborting the command.");
			wxMessageBox(message, _T(""), wxICON_INFORMATION);
			goto a;
		}
		bRenamedConfigFile = TRUE;
	}

	// get the length of the project configuration file's contents (exclude the null byte)
	int nFileLength = projConfigFileStr.GetLength();

    // write out the byte string (use CFile to avoid CStdioFile's mucking around with \n
    // and \r) because a config file written by CStdioFile's WriteString() won't be read
    // properly subsequently
	wxFile ff;
		
	wxLogNull logNo; // avoid spurious messages from the system

	if(!ff.Open(projectPath, wxFile::write))
	{
		wxString msg;
		msg = msg.Format(_(
"Unable to open the file for writing out the UTF-8 project configuration file, with path:\n%s"),
		projectPath.c_str());
		wxMessageBox(msg,_T(""), wxICON_EXCLAMATION);

		// if we renamed the earlier config file, we must restore its name before returning
		if (bRenamedConfigFile)
		{
			if (::wxFileExists(renamedPath))
			{
                // there is a renamed project config file to be restored; paths calculated
                // above are still valid, so just reverse their order in the parameter
                // block
				if (!::wxRenameFile(renamedPath,projectPath))
				{
					wxString message;
					message = message.Format(_(
					"Error restoring name of earlier configuration file with path %s."),
						renamedPath.c_str());
					message += _(
	"  Exit Adapt It and manually change .BAK to .aic for the project configuration file.");
					wxMessageBox(message,_T(""),wxICON_INFORMATION);
					goto a;
				}
			}
		}
		goto a;
	}

	// output the configuration file's content string
	char* pBuf = projConfigFileStr.GetBuffer(); // whm moved here from outer block above
	if (!ff.Write(pBuf,nFileLength))
	{
		// notify user, and then return without doing any more
		wxString thismsg;
		thismsg = _("Writing out the project configuration file's content failed.");
		wxString someMore;
		someMore = _(
" Exit Adapt It, and in Windows Explorer manually restore the .BAK extension on the renamed project configuration file to .aic before launching again. Beware, that configuration file may now be corrupted.");
		thismsg += someMore;
		wxMessageBox(thismsg);
		ff.Close();
		goto a;
	}
	projConfigFileStr.ReleaseBuffer(); // whm added 19Jun06

    // if control got here, we must remove the earlier (now renamed) project configuration
    // file, provided that we actually did find one with the same name earlier and renamed
    // it
	if (bRenamedConfigFile)
	{
		if (!::wxRemoveFile(renamedPath))
		{
			wxString thismsg;
			thismsg = _(
"Removing the renamed earlier project configuration file failed. Do it manually later in Windows Explorer - it has a .BAK extension.");
			wxMessageBox(thismsg,_T(""),wxICON_WARNING);
			goto a;
		}
	}

	// close the file
	ff.Close();

	// empty the buffer contents which are no longer needed
	projConfigFileStr.Empty();

    // the destination machine may not have Save As XML turned on when the unpack command
    // was given, but bSaveXMLflag preserves its value for when we exit. The loading in of
    // the overwritten project configuration file with the GetProjectConfiguration() call
    // below will, because m_bSaveAsXML must be TRUE for any Pack Document... operation,
    // cause the app to have the value TRUE restored, and the File menu's Save As XML
    // command ticked. So if bSaveXMLflag preserves a FALSE value, we'll have to undo these
    // two effects when we exit

    // we now need to parse in the configuration file, so the source user's settings are
    // put into effect; and we reset the KB paths in conformity with the config file's
    // m_bSaveAsXML value, which is about to be set TRUE in the next few lines, (but the KB
    // loading mechanism would not fail even if the flag was FALSE because of the
    // alternative paths which also get constructed in SetupKBPathsEtc().) The KBs have
    // already been loaded, or stubs created, so the SetupKBPathsEtc call here just has the
    // effect of getting paths set up for the xml form of KB i/o.
	//CAdapt_ItDoc* pDoc = gpApp->GetDocument();
	//wxASSERT(pDoc != NULL);
	gpApp->GetProjectConfiguration(gpApp->m_curProjectPath); // has flag side effect as 
															// noted in comments above
	gpApp->SetupKBPathsEtc();

    // now we can save the xml document file to the destination folder (either Adaptations
    // or a book folder), then parse it in and display the document in the main window.

	// write out the xml document file to the folder it belongs in and with the same
	// filename as on the source machine (path is given by m_curOutputPath above)
	nFileLength = packByteStr.GetLength();
	if(!ff.Open(gpApp->m_curOutputPath, wxFile::write))
	{
		wxString msg;
		msg = msg.Format(_(
		"Unable to open the xml text file for writing to doc folder, with path:\n%s"),
		gpApp->m_curOutputPath.c_str());
		wxMessageBox(msg,_T(""),wxICON_EXCLAMATION);
		return FALSE; // don't goto a; instead leave the new paths intact because the 
                // config file is already written out, so the user can do something in the
                // project if he wants
	}
	pBuf = packByteStr.GetBuffer(); // whm moved here from outer block above (local scope)
	if (!ff.Write(pBuf,nFileLength)) 
	{
		// notify user, and then return without doing any more
		wxString thismsg;
		thismsg = _("Writing out the xml document file's content failed.");
		wxMessageBox(thismsg,_T(""),wxICON_WARNING);
		ff.Close();
		return FALSE;
	}
	packByteStr.ReleaseBuffer(); // whm added 19Jun06
	ff.Close();

	// empty the buffer contents which are no longer needed
	packByteStr.Empty();

	// now parse in the xml document file, setting up the view etc
	bool bGotItOK = OnOpenDocument(gpApp->m_curOutputPath);
	if (!bGotItOK)
	{
		// some kind of error --warn user (this shouldn't happen)
		wxString thismsg;
		thismsg = _(
"Opening the xml document file for Unpack Document... failed. (It was stored successfully on disk. Try opening it with the Open command on the File menu.)");
		wxMessageBox(thismsg,_T(""),wxICON_WARNING);
        // just proceed, there is nothing smart that can be done. Visual inspection of the
        // xml document file is possible in Windows Explorer if the user wants to check out
        // what is in it. A normal Open command can also be tried too.
	}
	return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
/// \return		a wxString representing the path of the current working directory/folder
/// \remarks
/// Called from: the App's OnFileRestoreKb(), WriteConfigurationFile(), 
/// AccessOtherAdaptionProject(), the View's OnRetransReport() and CMainFrame's 
/// SyncScrollReceive().
/// Gets the path of the current working directory/folder as a wxString.
///////////////////////////////////////////////////////////////////////////////
wxString CAdapt_ItDoc::GetCurrentDirectory()
{
	// In wxWidgets it is simply:
	return ::wxGetCwd();
}

///////////////////////////////////////////////////////////////////////////////
/// \return		nothing
/// \param      event   -> the wxUpdateUIEvent that is generated when the Advanced Menu is about
///                         to be displayed
/// \remarks
/// Called from: The wxUpdateUIEvent mechanism when the associated menu item is selected, and before
/// the menu is displayed.
/// If Vertical Editing is in progress it disables "Receive Synchronized Scrolling Messages" item 
/// on the Advanced menu and this handler returns immediately. Otherwise, it enables the 
/// "Receive Synchronized Scrolling Messages" item on the Advanced menu as long as a project
/// is open.
///////////////////////////////////////////////////////////////////////////////
void CAdapt_ItDoc::OnUpdateAdvancedReceiveSynchronizedScrollingMessages(wxUpdateUIEvent& event)
{
	if (gbVerticalEditInProgress)
	{
		event.Enable(FALSE);
		return;
	}
	// the feature can be enabled only if we are in a project
	event.Enable(gpApp->m_bKBReady && gpApp->m_bGlossingKBReady);
#ifndef __WXMSW__
	event.Enable(FALSE); // sync scrolling not yet implemented on Linux and the Mac
#endif
}

///////////////////////////////////////////////////////////////////////////////
/// \return		nothing
/// \param		event	-> unused wxCommandEvent
/// \remarks
/// Called from: the Advanced menu's "Receive Synchronized Scrolling Messages" selection.
/// is open. Toggles the menu item's check mark on and off.
///////////////////////////////////////////////////////////////////////////////
void CAdapt_ItDoc::OnAdvancedReceiveSynchronizedScrollingMessages(wxCommandEvent& WXUNUSED(event))
{
	CMainFrame* pFrame = gpApp->GetMainFrame();
	wxASSERT(pFrame != NULL);
	wxMenuBar* pMenuBar = pFrame->GetMenuBar();
	wxASSERT(pMenuBar != NULL);
	wxMenuItem * pAdvancedMenuReceiveSSMsgs = 
				pMenuBar->FindItem(ID_ADVANCED_RECEIVESYNCHRONIZEDSCROLLINGMESSAGES);
	wxASSERT(pAdvancedMenuReceiveSSMsgs != NULL);

	// toggle the setting
	if (!gbIgnoreScriptureReference_Receive)
	{
		// toggle the checkmark to OFF
		pAdvancedMenuReceiveSSMsgs->Check(FALSE);
		gbIgnoreScriptureReference_Receive = TRUE;
	}
	else
	{
		// toggle the checkmark to ON
		pAdvancedMenuReceiveSSMsgs->Check(TRUE);
		gbIgnoreScriptureReference_Receive = FALSE;
	}
}

///////////////////////////////////////////////////////////////////////////////
/// \return		nothing
/// \param      event   -> the wxUpdateUIEvent that is generated when the Advanced Menu is about
///                         to be displayed
/// \remarks
/// Called from: The wxUpdateUIEvent mechanism when the associated menu item is selected, and before
/// the menu is displayed.
/// Enables the "Send Synchronized Scrolling Messages" item on the Advanced menu if a project
/// is open.
///////////////////////////////////////////////////////////////////////////////
void CAdapt_ItDoc::OnUpdateAdvancedSendSynchronizedScrollingMessages(wxUpdateUIEvent& event)
{
	// the feature can be enabled only if we are in a project
	event.Enable(gpApp->m_bKBReady && gpApp->m_bGlossingKBReady);
#ifndef __WXMSW__
	event.Enable(FALSE); // sync scrolling not yet implemented on Linux and the Mac
#endif
}

///////////////////////////////////////////////////////////////////////////////
/// \return		nothing
/// \param		event	-> unused wxCommandEvent
/// \remarks
/// Called from: the Advanced menu's "Send Synchronized Scrolling Messages" selection.
/// is open. Toggles the menu item's check mark on and off.
///////////////////////////////////////////////////////////////////////////////
void CAdapt_ItDoc::OnAdvancedSendSynchronizedScrollingMessages(wxCommandEvent& WXUNUSED(event))
{
	CMainFrame* pFrame = gpApp->GetMainFrame();
	wxASSERT(pFrame != NULL);
	wxMenuBar* pMenuBar = pFrame->GetMenuBar();
	wxASSERT(pMenuBar != NULL);
	wxMenuItem * pAdvancedMenuSendSSMsgs = 
				pMenuBar->FindItem(ID_ADVANCED_SENDSYNCHRONIZEDSCROLLINGMESSAGES);
	wxASSERT(pAdvancedMenuSendSSMsgs != NULL);

	// toggle the setting
	if (!gbIgnoreScriptureReference_Send)
	{
		// toggle the checkmark to OFF
		pAdvancedMenuSendSSMsgs->Check(FALSE);
		gbIgnoreScriptureReference_Send = TRUE;
	}
	else
	{
		// toggle the checkmark to ON
		pAdvancedMenuSendSSMsgs->Check(TRUE);
		gbIgnoreScriptureReference_Send = FALSE;
	}
}
