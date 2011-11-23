/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			DocPage.cpp
/// \author			Bill Martin
/// \date_created	3 May 2004
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the CDocPage class. 
/// The CDocPage class creates a wizard panel that allows the user
/// to either create a new document or select a document to work on 
/// from a list of existing documents.
/// \derivation		The CDocPage class is derived from wxWizardPage.
/////////////////////////////////////////////////////////////////////////////
// Pending Implementation Items in DocPage.cpp (in order of importance): (search for "TODO")
// 1. 
//
// Unanswered questions: (search for "???")
// 1. 
// 
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "DocPage.h"
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
#include <wx/valgen.h> // for wxGenericValidator
#include <wx/wizard.h>
#include <wx/filesys.h> // for wxFileName

#include "Adapt_It.h"
#include "MainFrm.h"
#include "Adapt_ItDoc.h"
#include "Adapt_ItView.h"
#include "Adapt_ItCanvas.h"
#include "ProjectPage.h"
#include "LanguagesPage.h"
#include "FontPage.h"
#include "PunctCorrespPage.h"
#include "CaseEquivPage.h"
#include "UsfmFilterPage.h"
#include "DocPage.h"
#include "scrollingwizard.h" // whm added 13Nov11 for wxScrollingWizard - need to include this here before "StartWorkingWizard.h" below
#include "StartWorkingWizard.h"
//#include "SourceBundle.h"
#include "Pile.h"
#include "Layout.h"
#include "WhichBook.h"
#include "helpers.h"

// globals

/// BEW added 28Nov05. If the user reached the Document page, in book mode, and then cancelled out
/// he likely did so because there were no files in the book folder yet and he cancelled out in
/// order to use the Move command to move one or more from the Adaptations folder into the book
/// folder. Without the following change, the status bar continues to say that the current folder is
/// the Adaptations one, when in fact it is not. So we have the status bar updated here too.
/// TODO: Determine if gbReachedDocPage is still needed in the wx version. See DoStartWorkingWizard() 
/// in the App where its value is tested .
bool gbReachedDocPage;

/// BEW added test on 21Mar07, to distinguish 
/// any other kind of failure to create a new document in OnNewDocument(), from a failure 
/// in OnNewDocument() specifically due to encountering in the input file a 3-letter id code 
/// that doesn't match the currently active book folder - thus preventing the document being 
/// constructed.
bool gbMismatchedBookCode = FALSE; // TRUE if the user is creating a document which
								   // belongs to the active book folder

// next two are for version 2.0 which includes the option of a 3rd line for glossing

/// This global is defined in Adapt_ItView.cpp.
extern bool	gbIsGlossing; // when TRUE, the phrase box and its line have glossing text

// This global is defined in Adapt_ItView.cpp.
//extern bool	gbGlossingVisible; // TRUE makes Adapt It revert to Shoebox functionality only

//extern bool gbGlossingUsesNavFont;

//extern bool gbRemovePunctuationFromGlosses;

/// This global is defined in Adapt_It.cpp.
extern bool gbWizardNewProject;

/// This global is defined in Adapt_It.cpp.
extern CStartWorkingWizard* pStartWorkingWizard; 

/// This global is defined in Adapt_It.cpp.
extern CProjectPage* pProjectPage;

/// This global is defined in Adapt_It.cpp.
extern CUsfmFilterPageWiz* pUsfmFilterPageWiz;

/// This global is defined in Adapt_It.cpp.
extern CDocPage* pDocPage;

/// This global is defined in Adapt_It.cpp.
extern CAdapt_ItApp* gpApp; // if we want to access it fast

/// When TRUE forces GetNewFile() to read the input text as UTF-8 text.
bool gbForceUTF8 = FALSE;

/// This global is defined in Adapt_It.cpp.
extern bool gbDoingInitialSetup;

// whm added following 23Mar07
extern bool gbLTRLayout; // defined in FontPage.cpp and initialized there to TRUE (unless changed by user)
extern bool gbRTLLayout; // defined in FontPage.cpp and initialized there to FALSE (unless changed by user)

IMPLEMENT_DYNAMIC_CLASS( CDocPage, wxWizardPage )

// event handler table
BEGIN_EVENT_TABLE(CDocPage, wxWizardPage)
	EVT_INIT_DIALOG(CDocPage::InitDialog)
    EVT_WIZARD_PAGE_CHANGING(-1, CDocPage::OnWizardPageChanging)
    EVT_WIZARD_CANCEL(-1, CDocPage::OnWizardCancel)
	EVT_LISTBOX_DCLICK(IDC_LIST_NEWDOC_AND_EXISTINGDOC, CDocPage::OnCallWizardFinish)// double click also activates OnWizardFinish
	EVT_BUTTON(IDC_BUTTON_WHAT_IS_DOC, CDocPage::OnButtonWhatIsDoc)
	EVT_CHECKBOX(IDC_CHECK_FORCE_UTF8, CDocPage::OnCheckForceUtf8)
	//EVT_CHECKBOX(IDC_SAVE_DOCSKB_AS_XML, CDocPage::OnCheckSaveUsingXML)
	EVT_BUTTON(IDC_CHANGE_FOLDER_BTN, CDocPage::OnButtonChangeFolder)
	EVT_CHECKBOX(IDC_CHECK_CHANGE_FIXED_SPACES_TO_REGULAR_SPACES, CDocPage::OnBnClickedCheckChangeFixedSpacesToRegularSpaces)
	EVT_LISTBOX(IDC_LIST_NEWDOC_AND_EXISTINGDOC, CDocPage::OnLbnSelchangeListNewdocAndExistingdoc)
END_EVENT_TABLE()

CDocPage::CDocPage()
{
}

CDocPage::CDocPage(wxWizard* parent) // dialog constructor
{
	Create( parent );

	// Note: The wizard page should handle all initialization here in the
	// constructor rather than in InitDialog, which is not called in the
	// construction of a wizard since ShowModal is not used. Only this 
	// constructor is called at the point in the App where new CDocPage is
	// called.

	// Note: This constructor is only called when the new CDocPage statement is
	// encountered in the App before the wizard displays. It is not called
	// when the page changes to/from the docPage in the Wizard.

	// TODO: Is the following needed for WX in light of other m_currProjectPath initializations below for WX
	gbReachedDocPage = TRUE; // we can't set nLastWizPage to doc_page, because that would clobber the
							 // memory of which page was the last one (usually project_page) so use
							 // a dedicated global boolean instead; we need to use it for a Cancel button
							 // click from DoModal() for the property sheet for the wizard, where we need
							 // to know that the document page was reached so as to avoid emptying the
							 // app's m_curProjectPath CString member (we need to empty the path if control
							 // did not reach the doc page because that means no project is current and we then
							 // use an empty path to suppress writing a project file from SaveModified()
							 // if the user then exits, which would otherwise clobber settings in the last
							 // written out project config file unnecessarily and unwantedly)
	m_bForceUTF8 = FALSE;

	m_pListBox = (wxListBox*)FindWindowById(IDC_LIST_NEWDOC_AND_EXISTINGDOC);
	
	// whm added 21Apr05 - moved below to InitDialog() because it needs to be set after reading 
	// the project config file and this constructor is called earlier
	//pChangeFixedSpaceToRegular = (wxCheckBox*)FindWindowById(IDC_CHECK_CHANGE_FIXED_SPACES_TO_REGULAR_SPACES);
	//wxASSERT(pChangeFixedSpaceToRegular != NULL);
	//pChangeFixedSpaceToRegular->Show(FALSE); // start with fixed space ... check box hidden
	//bChangeFixedSpaceToRegularSpace = gpApp->m_bChangeFixedSpaceToRegularSpace;
	//pChangeFixedSpaceToRegular->SetValue(bChangeFixedSpaceToRegularSpace);

	wxTextCtrl* pTextCtrlAsStaticDocPage = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_AS_STATIC_DOCPAGE);
	wxASSERT(pTextCtrlAsStaticDocPage != NULL);
	wxColor backgrndColor = this->GetBackgroundColour();
	//pTextCtrlAsStaticDocPage->SetBackgroundColour(backgrndColor);
	pTextCtrlAsStaticDocPage->SetBackgroundColour(gpApp->sysColorBtnFace);

	wxCheckBox* pCheckB;
	pCheckB = (wxCheckBox*)FindWindowById(IDC_CHECK_FORCE_UTF8);
	pCheckB->SetValue(FALSE);
	//pCheckB->SetValidator(wxGenericValidator(&m_bForceUTF8));
#ifndef _UNICODE
	pCheckB->Hide();
#endif
}

CDocPage::~CDocPage() // destructor
{
	
}

bool CDocPage::Create( wxWizard* parent)
{
	wxWizardPage::Create( parent );
	CreateControls();
	return TRUE;
}

void CDocPage::CreateControls()
{
	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	pDocPageSizer = DocPageFunc(this, TRUE, TRUE);
	//m_scrolledWindow = new wxScrolledWindow( this, -1, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxHSCROLL|wxVSCROLL );
	//m_scrolledWindow->SetSizer(pDocPageSizer);
}

// implement wxWizardPage functions
wxWizardPage* CDocPage::GetPrev() const 
{ 
	// The returned wizard page depends on whether the user had previously chosen an existing 
	// project or was creating a <New Project> (indicated by the gbWizardNewProject global). 
	// If the user had previously chosen an existing project, GetPrev() here in the docPage
	// skips the other pages and directly returns the pProjectPage. If the user had previously 
	// chosen <New Project> then GetPrev() returns the next-to-the-last page in the longer 
	// wizard (the filterPage).
	if (gbWizardNewProject)
	{
		return pUsfmFilterPageWiz;
	}
	else
	{
		return pProjectPage;
	}

}
wxWizardPage* CDocPage::GetNext() const
{
	// This GetNext() is never called since the docPage is the last page of the wizard
    return NULL;
}

void CDocPage::OnWizardCancel(wxWizardEvent& WXUNUSED(event))
{
	// This OnWizardCancel is only called when cancel is hit
	// while on the docPage.
    //if ( wxMessageBox(_T("Do you really want to cancel?"), _T("Question"),
    //                    wxICON_QUESTION | wxYES_NO, this) != wxYES )
    //{
    //    // not confirmed
    //    event.Veto();
    //}
}

void CDocPage::OnWizardPageChanging(wxWizardEvent& event)
{
	// NOTE: OnWizardPageChanging is called when the page
	// is about to change by means of "next" from the previous
	// page, "previous" from the current page, or when clicking
	// "finish" from the current page.

	// Can put any code that needs to execute regardless of whether
	// Next or Prev button was pushed here.

	// Determine which direction we're going and implement
	// the MFC equivalent of OnWizardNext() and OnWizardBack() here
	bool bMovingForward = event.GetDirection();
	if (bMovingForward)
	{
		if (!ListBoxPassesSanityCheck((wxControlWithItems*)m_pListBox))
		{
			wxMessageBox(_("You must Select a document (or <New Document>) from the list before continuing."), _T(""), wxICON_EXCLAMATION);
			event.Veto();
			return;
		}

		int nSel;
		nSel = m_pListBox->GetSelection(); //nSel = m_listBox.GetCurSel();
		// wx note: All items in the Linux/GTK listbox can be deselected by the user
		// by a single click on the already selected item. So we do the same here as
		// we did in the projectPage's OnWizardPageChanging handler - we check to see
		// if there is more than one item in the list; if so and nSel is -1 we veto
		// the page change and tell the user a selection must be made
		// whm: With the addition of ListBoxPassesSanityCheck() above the code block
		// below should no longer be necessary.
		//if (nSel == -1)
		//{
		//	if (m_pListBox->GetCount() > 1)
		//	{
		//		wxMessageBox(_("You must Select a document (or <New Document>) from the list before continuing."), _T(""), wxICON_EXCLAMATION);
		//		event.Veto();
		//		return;
		//	}
		//	else
		//	{
		//		wxASSERT(m_pListBox->GetCount() == 1);
		//		// User deselected <New Project> (possibly by clicking on it once in wxGTK), but since it 
		//		// is the only item listed (GetCount should have returned 1), we assume the user wants to 
		//		// start a <New Project>, so we'll select it for him automatically and allow the page to 
		//		// change with <New Project> selected.
		//		m_pListBox->SetSelection(0);
		//		nSel = m_pListBox->GetSelection();
		//		wxASSERT(nSel == 0);
		//	}
		//}
		// Finish wizard button was selected
		// Since I cannot get OnWizardFinish handler to work, I'll call its function here
		OnWizardFinish(event); //event.Skip(); // do nothing
	}
	else
	{
		// Prev wizard button was selected
		// if book mode was ON, we here unilaterally turn it off, since we cannot be
		// certain the project page choice will be for a project which previously had book mode
		// turned on; nor can we tell whether we arrived at the project page by a launch of the
		// app, or by a click of the <Back button from deeper within the Wizard; so the safe thing
		// is to turn it off, and let a project config file be read in and possibly turn it back on again
		// (if book mode is disabled because of a bad read of the books.xml file, don't change the
		// m_bDisableBookMode flag back to FALSE, only a subsequent good read of the xml file
		// should do that)
		// 
		// whm 2Sep11 modification. Unilaterally turning OFF book folder mode should NOT be
		// done here in OnWizardPageChanging()! Doing so here would result in a FALSE value 
		// for m_bBookMode, and -1 for m_nBookIndex being stored in the current project config
		// file, when the pProjectPage->InitDialog() call is made below in preparation for
		// moving backwards to the ProjectPage.
		/*
		// event.GetDirection() returns TRUE if moving forward, FALSE if moving backwards
		if (event.GetDirection() == FALSE)
		{
			if (gpApp->m_bBookMode)
			{
				gpApp->m_bBookMode = FALSE;
				gpApp->m_nBookIndex = -1;
				gpApp->m_bibleBooksFolderPath.Empty();
				gpApp->m_pCurrBookNamePair = NULL;
				gpApp->m_nLastBookIndex = gpApp->m_nDefaultBookIndex;
			}
		}
		*/

		if (gbWizardNewProject == TRUE)
		{
			// we setting up a new project, so the "< Back" button should take us back through
			// all wizard pages
			wxASSERT(this == pDocPage);
		}
		else
		{
			// it is not a new project, but an existing project, therefore we should go
			// back to the projectPage
			wxASSERT(this == pDocPage);
		}
		// ensure the project page is up to date
		wxInitDialogEvent idevent;
		pProjectPage->InitDialog(idevent);

	}
    //event.Veto();
}

// This InitDialog is called from the DoStartWorkingWizard() function
// in the App. Most of its code comes from the MFC's DocPage.cpp OnSetActive().
void CDocPage::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{

	//InitDialog() is not virtual, no call needed to a base class

	wxCheckBox* pCheck = (wxCheckBox*)FindWindowById(IDC_CHECK_FORCE_UTF8);
	wxASSERT(pCheck != NULL);

#ifndef _UNICODE
	pCheck->Show(FALSE);
#else
	pCheck->Show(TRUE);
#endif

	OnSetActive(); // wx version calls our internal function from InitDialog
	gpApp->RefreshStatusBarInfo();
}

// wx version note: This OnSetActive() is not called by the EVT_ACTIVATE handler in wx, so it
// is called internally from InitDialog above and OnButtonChangeFolder(), so we retain it as a 
// wx function here.
void CDocPage::OnSetActive() 
{
	gbReachedDocPage = TRUE; // we can't set nLastWizPage to doc_page, because that would clobber the
							 // memory of which page was the last one (usually project_page) so use
							 // a dedicated global boolean instead; we need to use it for a Cancel button
							 // click from DoModal() for the property sheet for the wizard, where we need
							 // to know that the document page was reached so as to avoid emptying the
							 // app's m_curProjectPath CString member (we need to empty the path if control
							 // did not reach the doc page because that means no project is current and we then
							 // use an empty path to suppress writing a project file from SaveModified()
							 // if the user then exits, which would otherwise clobber settings in the last
							 // written out project config file unnecessarily and unwantedly)

	// get pointers to our static text controls, their content will change depending on mode
	wxStaticText* pModeMsg = (wxStaticText*)FindWindowById(IDC_STATIC_WHICH_MODE);
	wxStaticText* pFolderMsg = (wxStaticText*)FindWindowById(IDC_STATIC_WHICH_FOLDER);

	// hide or show the Change Folder... button depending on whether book mode is off or on
	wxButton* pBtn = (wxButton*)FindWindowById(IDC_CHANGE_FOLDER_BTN);
	if (gpApp->m_bBookMode && !gpApp->m_bDisableBookMode)
	{
			pBtn->Show();
	}
	else
	{
			pBtn->Hide();
	}

	CAdapt_ItApp* pApp = (CAdapt_ItApp*)&wxGetApp();
	wxASSERT(pApp != NULL);
	// first, use the current source language font for the list box
	// (in case there are special characters in the language name)

	// clear the box of any earlier content (since we can come here more than once now
	// that the user can click the "Change Folder" button)
	m_pListBox->Clear();

	// make the fonts show user's desired point size in the dialog
	#ifdef _RTL_FLAGS
	gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pNavTextFont, NULL, NULL,
								m_pListBox, NULL, gpApp->m_pDlgSrcFont, gpApp->m_bNavTextRTL);
	#else // Regular version, only LTR scripts supported, so use default FALSE for last parameter
	gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pNavTextFont, NULL, NULL, 
								m_pListBox, NULL, gpApp->m_pDlgSrcFont);
	#endif

	// we'll press m_pDlgTgtFont into service for the mode information static texts,
	// but use the source text font for displaying them, at 12 point size
	// BEW changed 27Oct05...NO!!! If, for example, the source text font is a hacked single-byte 
	// font with Devangri glyphs, Uniscribe is smart enough to display it in the Unicode app,
	// but we only see unintelligible Devenagri gobbledegook. So play safe... use NavText font.
	CopyFontBaseProperties(pApp->m_pNavTextFont,pApp->m_pDlgTgtFont);
	pApp->m_pDlgTgtFont->SetPointSize(12); // 12 point
	pModeMsg->SetFont(*pApp->m_pDlgTgtFont);
	pFolderMsg->SetFont(*pApp->m_pDlgTgtFont);

	// put the appropriate mode information into the two static texts
	wxString s1;
	if (gpApp->m_bBookMode && !gpApp->m_bDisableBookMode)
	{
		// IDS_ON
		s1 = s1.Format(_("ON")); // get "ON" into s1
		// IDS_MODE_MSG
		m_staticModeStr = m_staticModeStr.Format(_("Book folder mode is  %s"),s1.c_str());
		// IDS_SAVE_LOC_MSG
		m_staticFolderStr = m_staticFolderStr.Format(_("Documents are being saved to the  %s  folder"),gpApp->m_pCurrBookNamePair->seeName.c_str());
	}
	else
	{
		// book mode is OFF
		// IDS_OFF
		s1 = s1.Format(_("OFF")); // get "ON" into s1
		// IDS_MODE_MSG
		m_staticModeStr = m_staticModeStr.Format(_("Book folder mode is  %s"),s1.c_str());
		// IDS_SAVE_LOC_MSG
		m_staticFolderStr = m_staticFolderStr.Format(_("Documents are being saved to the  %s  folder"),gpApp->m_adaptionsFolder.c_str());
	}
	pModeMsg->SetLabel(m_staticModeStr);
	pFolderMsg->SetLabel(m_staticFolderStr);

	// generate a CStringList of all the possible adaption documents
	wxArrayString possibleAdaptions; //CStringList possibleAdaptions;
	wxString strNewDoc;
	// IDS_NEW_DOCUMENT
	strNewDoc = strNewDoc.Format(_("<New Document>"));

	// There are now two possible folders: if book mode is OFF, we must access the Adaptations
	// folder (legacy behaviour), but if book mode is ON, the user will have choosen a book folder
	// in which to work, and the app's m_bibleBooksFolderPath member has the absolute path to it
	if (gpApp->m_bBookMode && !gpApp->m_bDisableBookMode)
		gpApp->GetPossibleAdaptionDocuments(&possibleAdaptions,gpApp->m_bibleBooksFolderPath);
	else
		gpApp->GetPossibleAdaptionDocuments(&possibleAdaptions,gpApp->m_curAdaptionsPath);

	// whm modified 20Oct11 to sort possibleAdaptations before adding the
	// <New Document> as the first item. This change is needed to get a sorted
	// list on the Linux port. Windows and Mac seem to grab the list of folders
	// in sorted order.
	possibleAdaptions.Sort();
	possibleAdaptions.Insert(strNewDoc,0); // first in list

	// fill the list box with the document name strings
	int count;
	for (count = 0; count < (int)possibleAdaptions.GetCount(); count++)
	{
		wxString str = possibleAdaptions.Item(count);
		m_pListBox->Append(str);
	}
	possibleAdaptions.Clear();

	// hilight the first doc name (ie. <New Document>)
	m_pListBox->SetSelection(0, TRUE);

	// if we have chosen same project as when app last closed, then instead select
	// the last document that was open; if book mode is in effect, then we also must
	// check if the project's config file's m_nBookIndex (unless we modified it in
	// the prior Which Book Folder? dialog) is the same book name as found at the
	// start of the path substring returned by the Mid call - if so, we are in the same
	// book folder as before
	if (gpApp->m_bEarlierProjectChosen)
	{
		wxString lastOpenedDoc = _T("");
		int nFound = gpApp->m_lastDocPath.Find(gpApp->m_curAdaptionsPath);
		if (nFound != -1)
		{
			// remove the path information, we just want the filename (after backslash)
			// or the book name, slash and filename
			int len = gpApp->m_curAdaptionsPath.Length();
			lastOpenedDoc = gpApp->m_lastDocPath.Mid(++len);
			wxASSERT(!lastOpenedDoc.IsEmpty());
			int index;

			if (gpApp->m_bBookMode && !gpApp->m_bDisableBookMode)
			{
				// lastOpenedDoc is something like "Luke\<doc filename>", so check
				// out if we are still in the same book folder as before
				int bookIndex = gpApp->m_nBookIndex;
				wxString bookFolderName = ((BookNamePair*)(*gpApp->m_pBibleBooks)[bookIndex])->dirName;
				int nFound = lastOpenedDoc.Find(bookFolderName);
				if (nFound == 0)
				{
					// the book name is at the start of the string, so remove it and check for a following backslash
					len = bookFolderName.Length();
					lastOpenedDoc = lastOpenedDoc.Mid(len);
					nFound = lastOpenedDoc.Find(gpApp->PathSeparator);
					if (nFound == 0)
					{
						// we are still in the same book folder, remove the backslash and then check
						lastOpenedDoc = lastOpenedDoc.Mid(1);
						goto m;
					}
				}
			}
			else
			{
m:				index = m_pListBox->FindString(lastOpenedDoc);
				if (index != -1) //LB_ERR
				{
					// we found the filename, so select it
					m_pListBox->SetSelection(index, TRUE);
					if (index > 0)
						m_pListBox->SetFirstItem(index); // make the item visible if it would require scrolling whm added 10Mar08
				}
			}
		}
	}

	// whm added 21Apr05 - moved below to OnSetActive() which is called by InitDialog() because 
	// the pointers and pChangeFixedSpaceToRegular needs to be set after reading the project 
	// config file; this DocPage's constructor is called earlier.
	pChangeFixedSpaceToRegular = (wxCheckBox*)FindWindowById(IDC_CHECK_CHANGE_FIXED_SPACES_TO_REGULAR_SPACES);
	wxASSERT(pChangeFixedSpaceToRegular != NULL);
	pChangeFixedSpaceToRegular->Show(FALSE); // start with fixed space ... check box hidden
	bChangeFixedSpaceToRegularSpace = gpApp->m_bChangeFixedSpaceToRegularSpace;
	pChangeFixedSpaceToRegular->SetValue(bChangeFixedSpaceToRegularSpace);

	int nSel;
	wxString tempStr;
	nSel = m_pListBox->GetSelection();
	tempStr = m_pListBox->GetStringSelection();
	if (tempStr.GetChar(0) == _T('<')) // check for an initial < (because localizing may 
									   // produce a different text); this is how we tell
									   // that the user chose <New Document>
	{
		// The docPage has <New Document> selected, so cause the Fixed Space ... check box control
		// to appear
		pChangeFixedSpaceToRegular->Show(TRUE); // start with fixed space ... check box showing
		
	}
	else
	{
		// hide the Fixed Space... check box control
		pChangeFixedSpaceToRegular->Show(FALSE); // start with fixed space ... check box hidden
	}

	m_pListBox->SetFocus(); // whm added 11Aug08 otherwise the read only text ctrl at top gets focus and shows blinking cursor in it

	// whm added below for wx version: Since label lengths can change call the docPage's sizer's Layout() 
	// method to resize the dialog if necessary
	// whm 8 Jun 09 moved here from above since the hiding of the pChangeFixedSpaceToRegular control can affect layout
	pDocPageSizer->Layout();

	//return CPropertyPage::OnSetActive();
}
// event handling functions

void CDocPage::OnButtonWhatIsDoc(wxCommandEvent& WXUNUSED(event)) 
{
	wxString s;
	wxString accum;
	// IDS_NOT_START_EMPTY
	s = s.Format(_("An Adapt It document does not start off empty. Instead, it starts out containing all of the source text you wish to translate in that document - perhaps a whole book such as Romans."));
	accum += s;
	accum += _T(' ');
	// IDS_GET_SOURCE_INTO
	s = s.Format(_("To get the source text into the document, you must first double-click the <New Document> item. Then you will see a file dialog which you can use to locate and get the file which has the source text."));
	accum += s;
	accum += _T(' ');
	// IDS_ANY_EXISTING_DOC
	s = s.Format(_("Any existing Adapt It documents will have their names shown in the list below the <New Document> item. To open one of those documents for further work, just double-click it."));
	accum += s;
	accum += _T(' ');
	// IDS_BE_SURE_SAVE
	s = s.Format(_("You don't have to finish translating a document in one session. Just be sure that it is saved when you close it. Adapt It stores your documents in your project's Adaptations folder."));
	accum += s;
	accum += _T(' ');
	// IDS_OTHERS_CANNOT_READ
	s = s.Format(_("Adapt It documents cannot be read by other computer programs. To do that you must first have your document open in Adapt It, then export its translation to a file which another program can read."));
	accum += s;

	wxMessageBox(accum, _T(""), wxICON_INFORMATION);
}

void CDocPage::OnCheckForceUtf8(wxCommandEvent& WXUNUSED(event)) 
{
	wxCheckBox* pCheck = (wxCheckBox*)FindWindowById(IDC_CHECK_FORCE_UTF8);
	wxASSERT(pCheck != NULL);

	// set or clear the global, to force or unforce UTF8 conversions on source data input
	gbForceUTF8 = pCheck->GetValue(); 
}

// BEW added 04Aug05
//void CDocPage::OnCheckSaveUsingXML(wxCommandEvent& WXUNUSED(event)) 
//{
//	wxCheckBox* pCheck = (wxCheckBox*)FindWindowById(IDC_SAVE_DOCSKB_AS_XML);
//
//	// wx version: remind user that the wx version only handles xml docs and kb files
//	wxMessageBox(_("Sorry, this version of Adapt It only handles Adapt It documents\nand knowledge bases in xml format. If you have .adt or .KB\nfiles created by a previous version of Adapt It, you will need\nto first convert those files to .xml format by saving them\nusing Version 3.x of Adapt It."),_T(""),wxICON_INFORMATION);
//	pCheck->SetValue(TRUE);
//	gpApp->m_bSaveAsXML = TRUE;
//	return;
//}

void CDocPage::OnCallWizardFinish(wxCommandEvent& WXUNUSED(event)) // since 2.5.3 two handlers cant call same function
{
	wxWizardEvent wevent(wxEVT_WIZARD_FINISHED);
	OnWizardFinish(wevent);
}

void CDocPage::OnButtonChangeFolder(wxCommandEvent& event)
{
	// force the ask, and save doc & clear window
	CAdapt_ItDoc* pDoc = gpApp->GetDocument();
	CKB* pKB;
	if (gbIsGlossing)
		pKB = gpApp->m_pGlossingKB;
	else
		pKB = gpApp->m_pKB;
	if (pKB != NULL && gpApp->m_pLayout->GetStripCount() > 0)
	{
		// doc is open, so close it first
		pDoc->OnFileClose(event); // my version, which does not call OnCloseDocument
		gpApp->GetMainFrame()->canvas->Update(); // force immediate repaint // TODO: is this needed in WX???
	}

	// put up the Which Book Folder dialog
	CWhichBook whichBkDlg(gpApp->GetMainFrame());
	whichBkDlg.Centre();
	whichBkDlg.ShowModal(); //whichBkDlg.DoModal();

	// get things updated to reflect user choice
	// MFC's OnSetActive() has no direct equivalent in wxWidgets, but we use
	// our local version of OnSetActive()
	OnSetActive();
	pDocPageSizer->Layout(); // force dialog to resize if necessary
}

void CDocPage::OnWizardFinish(wxWizardEvent& WXUNUSED(event))
{
	CAdapt_ItApp* pApp = (CAdapt_ItApp*)&wxGetApp();
	wxASSERT(pApp);
	wxCommandEvent dummyevent;
	int nSel;

	nSel = m_pListBox->GetSelection();
	if (nSel == -1) // LB_ERR
	{
		// this should never happen even on Linux/GTK, since we've forced a selection back in
		// the OnWizardPageChanging() handler.
		wxMessageBox(_("List box error when getting the current selection"), 
						_T(""), wxICON_EXCLAMATION);
		wxASSERT(FALSE);
		wxExit();
	}
	m_docName = m_pListBox->GetString(nSel);

	CAdapt_ItDoc* pDoc = pApp->GetDocument();
	CAdapt_ItView* pView = pApp->GetView();

	// send the app the current size & position data, for saving to config on closure
	// 
	// whm modification 14Apr09. I've commented out the assigning of frame position, size, and zoom
	// characteristics below, because these should not be changed here at the time the docPage calls
	// OnWizardFinish(). Instead, the main frame around any new or existing document should retain its dimensions
	// as previously established by reading the basic config file or (if no config file has yet been
	// saved) by the defaults set in OnInit(). The problem with having these set here is that, when our
	// CMainFrame instance is created in the App's OnInit(), it is created an arbitrary size before the
	// basic config file is read - and hence the position and size detection below will be wrong and
	// obliterate any values that were already established.
	/*
	wxRect rectFrame;
	CMainFrame *pFrame = wxGetApp().GetMainFrame();
	rectFrame = pFrame->GetRect(); // screen coords
	rectFrame = NormalizeRect(rectFrame); // use our own from helpers.h
	pApp->m_ptViewTopLeft.x = rectFrame.GetX();
	pApp->m_ptViewTopLeft.y = rectFrame.GetY();

	pApp->m_szView.SetWidth(rectFrame.GetWidth());
	pApp->m_szView.SetHeight(rectFrame.GetHeight());
	pApp->m_bZoomed = pFrame->IsMaximized(); //pFrame->IsZoomed();
	*/
	
#ifdef _RTL_FLAGS // whm added23Mar07
	// We've just defined a new project (via previous wizard pages), or we've loaded 
	// an existing project config file (via projectPage). In either case, we may now 
	// have a different RTL layout and/or RTL fonts so call AdjustAlignmentMenu to 
	// ensure that the Layout menu item's text is set correctly.
	if (pApp->m_bSrcRTL == TRUE && pApp->m_bTgtRTL == TRUE)
	{
		gbLTRLayout = FALSE; // use these to set layout direction on user's behalf, when possible
		gbRTLLayout = TRUE;
	}
	else if (pApp->m_bSrcRTL == FALSE && pApp->m_bTgtRTL == FALSE)
	{
		gbLTRLayout = TRUE; // use these to set layout direction on user's behalf, when possible
		gbRTLLayout = FALSE;
	}

	pApp->GetView()->AdjustAlignmentMenu(gbRTLLayout,gbLTRLayout); // whm added23Mar07
#endif // for _RTL_FLAGS

	if (m_docName.GetChar(0) == _T('<')) // check for an initial < (because localizing may 
									   // produce a different text); this is how we tell
									   // that the user chose <New Document>
	{
        // since it takes a few seconds for the "Input Text File For Adaptation" file
        // dialog to appear, I'm going to have the main frame's layout redrawn so the
        // toolbar and controlbar looks better while waiting for the file dialog to appear
		pApp->GetMainFrame()->Update();

		// whm modified 26Jul11 to use the pApp's m_sourceInputsFolderName if no previous
		// m_lastSourceInputPath was stored. See similar code in the Doc's OnNewDocument().
		// ensure that the current work folder is the project one for default
		wxString dirPath;
		if (pApp->m_lastSourceInputPath.IsEmpty())
		{
			if (!pApp->m_curProjectPath.IsEmpty())
				dirPath = pApp->m_curProjectPath + pApp->PathSeparator + pApp->m_sourceInputsFolderName; // __SOURCE_INPUTS
			else
				dirPath = pApp->m_workFolderPath; // typically, C:\Users\<userName>\Documents\Adapt It <Unicode> Work

		}
		else
		{
			dirPath = pApp->m_lastSourceInputPath; // from the path that was last used
		}

		bool bOK = ::wxSetWorkingDirectory(dirPath);
		if (!bOK)
		{
			wxString str;
			// IDS_BAD_LAST_DOC_PATH
			str = str.Format(_(
"Warning: invalid path to the last new document: %s A safe default path will be used instead. "),
			dirPath.c_str());
			wxMessageBox(str, _T(""), wxICON_INFORMATION);
			dirPath = pApp->m_workFolderPath;
			bOK = ::wxSetWorkingDirectory(dirPath);
			if (!bOK)
			{
				// should not fail, but if it did, then exit the new operation with message,
				wxMessageBox(_T(
"Failure trying to set the current directory. Check the LastNewDocumentFolder entry in both the basic and project configuration files."),
				_T(""), wxICON_EXCLAMATION);
				gbDoingInitialSetup = FALSE;
			}
		}

		// get a pointer to the currently being used KB
		CKB* pKB;
		if (gbIsGlossing)
			pKB = pApp->m_pGlossingKB;
		else
			pKB = pApp->m_pKB;

		// we can come here with an existing doc open, so we must first close it &
		// also prompt for doc save & kb save if user does not save the doc, then go on
		if (pKB != NULL && pApp->m_pLayout->GetStripCount() > 0)
		{
			// doc is open, so close it first
			pDoc->OnFileClose(dummyevent); // my version, which does not call 
										   // OnCloseDocument
		}

		// use the OnNewDocument() function to go to the file dialog for inputting
		// a source text file to use as the new document
		pStartWorkingWizard->Show(FALSE); 
        // wx version note: The Linux/GTK version in debug mode reports that the EndModal
        // call gets called twice, so I've commented the call to EndModal below. EndModal
        // apparently is automatically called by the wizard class (under Windows it didn't
        // assert).
		//pStartWorkingWizard->EndModal(1);
		
        // default the m_nActiveSequNum value to -1 when getting the doc created
		pApp->m_nActiveSequNum = -1; 
		bool bResult = pDoc->OnNewDocument();
		// whm 17Apr11 changed test below to include check for m_nActiveSequNum being
		// -1 after return of OnNewDocument(). If user cancels the file dialog within
		// OnNewDocument() it still returns TRUE within bResult (due to a change Bruce
		// made on 25Aug10). The test is needed to prevent a crash if the Preferences
		// dialog is called when no document is active.
		if (!bResult || pApp->m_nActiveSequNum == -1)
		{
            // BEW added test on 21Mar07, to distinguish a failure due to a 3-letter code
            // mismatch preventing the document being constructed for the currently active
            // book folder, and any other kind of failure
			if (gbMismatchedBookCode)
			{
                // OnNewDocument() has already done the required warning, so just leave the
                // user in the Document page of the wizard, to try again with a different
                // file, or click the Change Book folder and then try again with the last
                // tried file
				gbMismatchedBookCode = FALSE;
				wxCommandEvent uevent;
				pDoc->OnFileOpen(uevent); // get the Document page of the wizard open again
				return; // TRUE; // MFC note: FALSE means: don't destroy the property sheet; 
                    // TRUE means destroy it and as the above OnFileOpen() call is a nested
                    // call, a successful document creation should result in the wizard
                    // being destroyed, and so the remaining function exits just only need
                    // to return TRUE (or FALSE) as they will do nothing and rightly so
			}
			else
			{
				// other failures, just warn and close the wizard
				// IDS_NO_OUTPUT_FILE
				// whm modified 18Jun09 The Doc's GetNewFile() function now returns an enum so that
				// OnNewDocument() reports the specific error, therefore, no additional error needs to
				// be reported here.
				//wxMessageBox(_("Sorry, opening the new document failed. Perhaps you cancelled the output filename dialog, or maybe the source text file is open in another application?"), _T(""),wxICON_INFORMATION);
				gbDoingInitialSetup = FALSE;
				return; //return CPropertyPage::OnWizardFinish();
			}
		}
		gbMismatchedBookCode = FALSE; // ensure it is off before exitting
		pView->Invalidate();
		pApp->m_pLayout->PlaceBox();
		pStartWorkingWizard = (CStartWorkingWizard*)NULL;
		CMainFrame *pFrame = (CMainFrame*)pView->GetFrame();
		pFrame->Raise();
		if (pApp->m_bZoomed)
			pFrame->SetWindowStyle(wxDEFAULT_FRAME_STYLE 
						| wxFRAME_NO_WINDOW_MENU | wxMAXIMIZE);
		else
			pFrame->SetWindowStyle(wxDEFAULT_FRAME_STYLE 
						| wxFRAME_NO_WINDOW_MENU);

		pApp->m_bEarlierProjectChosen = FALSE;
		pApp->m_bEarlierDocChosen = FALSE;
		pApp->nLastActiveSequNum = 0;

		// initialize m_nActiveSequNum to the nLastActiveSequNum value
		pApp->m_nActiveSequNum = pApp->nLastActiveSequNum;
		// set the active pile
		pApp->m_pActivePile = pView->GetPile(0);

		gbDoingInitialSetup = FALSE;

		pApp->RefreshStatusBarInfo();
		if (pApp->m_bReadOnlyAccess)
		{
			// try get an extra paint job done, so background will show all pink from the
			// outset
			pView->canvas->Refresh();
		}
		return; //return CPropertyPage::OnWizardFinish();
	}
	else // the user did not choose <New Document>
	{
        // it's an existing document that we want to open; but first we can come here with
        // an existing doc open, so we must first close it & also prompt for doc save & kb
        // save if user does not save the doc, then go on
		CKB* pKB;
		if (gbIsGlossing)
			pKB = pApp->m_pGlossingKB;
		else
			pKB = pApp->m_pKB;
		if (pKB != NULL && pApp->m_pLayout->GetStripCount() > 0)
		{
			// doc is open, so close it first
			pDoc->OnFileClose(dummyevent); // my version, which does not call 
										   // OnCloseDocument
			pApp->GetMainFrame()->canvas->Update(); // force immediate repaint // TODO: Need this ???
		}
				
		// construct the path to the wanted document, then call CAdapt_ItDoc's
		// OnOpenDocument function to open it & get the view laid out, etc.
		// path depends on m_bBookMode flag
		wxString docPath;
		if (gpApp->m_bBookMode && !gpApp->m_bDisableBookMode)
		{
			docPath = pApp->m_curAdaptionsPath + pApp->PathSeparator + 
											pApp->m_pCurrBookNamePair->dirName;
			docPath += pApp->PathSeparator + m_docName;
		}
		else
		{
			docPath = pApp->m_curAdaptionsPath + gpApp->PathSeparator + m_docName;
		}

#ifdef SHOW_DOC_I_O_BENCHMARKS
	wxDateTime dt1 = wxDateTime::Now(),
			   dt2 = wxDateTime::UNow();
#endif
        // default the m_nActiveSequNum value to 0 when getting the doc open, we check
        // later for what the config file gave for the last active location, and jump there
        // if not at sequence number 0
		pApp->m_nActiveSequNum = 0; 
		bool bOK = pDoc->OnOpenDocument(docPath);
		if (!bOK)
		{
			// IDS_LOAD_DOC_FAILURE
			wxMessageBox(_(
"Sorry, loading the document failed. (The file may be in use by another application. Or the file has become corrupt and must be deleted.)"),
			_T(""), wxICON_STOP);
			return; // wxExit(); whm modified 27May11
		}

		// put the focus in the phrase box, after any text
		if (pApp->m_pTargetBox->GetHandle() != NULL && !pApp->m_targetPhrase.IsEmpty()
			&& (pApp->m_pTargetBox->IsShown()))
		{
			int len = pApp->m_pTargetBox->GetLineLength(0); // line number zero 
															// for our phrasebox
			pApp->m_nStartChar = len;
			pApp->m_nEndChar = len;
			pApp->m_pTargetBox->SetFocus();
		}
		else
		{
			if (pApp->m_pTargetBox->GetHandle() != NULL && (pApp->m_pTargetBox->IsShown()))
			{
				pApp->m_nStartChar = 0;
				pApp->m_nEndChar = 0;
				pApp->m_pTargetBox->SetFocus();
			}
		}

		// close off the wizard
		pStartWorkingWizard->Show(FALSE); //pStartWorkingWizard->ShowWindow(SW_HIDE);
            // wx version note: The Linux/GTK version in debug mode reports that the
            // EndModal call gets called twice, so I've commented the call to EndModal
            // below. EndModal apparently is automatically called by the wizard class
            // (under Windows it didn't assert).
		//pStartWorkingWizard->EndModal(1);
		pStartWorkingWizard = (CStartWorkingWizard*)NULL;
		CMainFrame* pFrame = (CMainFrame*)pView->GetFrame();
		pFrame->Raise();
		if (pApp->m_bZoomed)
			pFrame->SetWindowStyle(wxDEFAULT_FRAME_STYLE 
						| wxFRAME_NO_WINDOW_MENU | wxMAXIMIZE);
		else
			pFrame->SetWindowStyle(wxDEFAULT_FRAME_STYLE 
						| wxFRAME_NO_WINDOW_MENU);

		// MainFrame title set in OnOpenDocument(docPath) but the doc/view framework
		// overwrites it with "unnamed1", so we'll set it again here in the wizard
		// In some places it seems pDoc->SetTitle() as well as pFrame->SetTitle() is
		// needed.
		wxDocTemplate* pTemplate = pDoc->GetDocumentTemplate();
		wxASSERT(pTemplate != NULL);
		wxString typeName,fpath,fname,fext;
		typeName = pTemplate->GetDescription(); // should be "Adapt It" or "Adapt It Unicode"
		wxFileName fn(docPath);
		fn.SplitPath(docPath,&fpath,&fname,&fext);
		pFrame->SetTitle(fname + _T(" - ") + typeName);

		pDoc->SetFilename(docPath,TRUE); // here TRUE means "notify the views" whereas in 
										 // MFC means add to MRU
	
		// determine whether user opened the same document, using info saved in the 
		// config file
		pApp->m_bEarlierDocChosen = FALSE;
		if (!pApp->m_lastDocPath.IsEmpty())
		{
			int nFound = pApp->m_lastDocPath.Find(docPath);
			if (nFound != -1)
			{
				// it is a subpath or identical, so same document was chosen
				pApp->m_bEarlierDocChosen = TRUE;
			}
		}

        // if the user opened the same project and document, then jump to the last active
        // location when the app was last closed
		if (pApp->m_bEarlierProjectChosen && pApp->m_bEarlierDocChosen)
		{
            // if the user did some operation that resulted in more sourcephrase instances
            // being created (eg. such as a rebuild from a punctuation change affecting
            // quote characters) and had the phrase box near the end in the document but
            // did not save the document, then the nLastActiveSequNum saved in the
            // configuration file could be greater than the number of sourcephrases that
            // would be read in when the document was next opened, so we must test for this
            // and reduce the sequ number value to a safe value before trying to set up a
            // pile at that location.
			if (pApp->nLastActiveSequNum >= (int)pApp->m_pSourcePhrases->GetCount())
				pApp->nLastActiveSequNum = pApp->m_pSourcePhrases->GetCount() - 1;

			// initialize m_nActiveSequNum to the nLastActiveSequNum value
			pApp->m_nActiveSequNum = pApp->nLastActiveSequNum;
			// set the active pile
			CPile* pPile;
			pPile = pView->GetPile(pApp->nLastActiveSequNum);
			wxASSERT(pPile != NULL);

            // this could turn out to be a retranslation pile (if user removed a
            // retranslation just before exiting the application without saving the
            // document, it would be), so check & if necessary move beyond the
            // retranslation or to an earlier safe location
			int nFinish = 1; // the next function was designed for retranslation use, 
				// but it should work fine anywhere provided we set nFinish to 1 (or zero).
			bool bSetSafely;
			bSetSafely = pView->SetActivePilePointerSafely(pApp,pApp->m_pSourcePhrases,
								pApp->nLastActiveSequNum,pApp->m_nActiveSequNum,nFinish);
            // BEW 30Jun10, removed 3 lines below because a call to Jump() happens in the
            // above call, and so the ones below should be redundant (and if
            // ChooseTranslation was called in the earlier Jump() it would then get called
            // again, which is confusing - so we need to not have this second call)
            // Legacy comment: m_nActiveSequNum might have been changed by the preceding
            // call, so reset the active pile
			//pPile = pView->GetPile(pApp->m_nActiveSequNum);
			//CSourcePhrase* pSrcPhrase = pPile->GetSrcPhrase();
			//pView->Jump(pApp,pSrcPhrase); // jump there
		}
		gbDoingInitialSetup = FALSE;

		// make sure the menu command is checked or unchecked as necessary
		wxMenuBar* pMenuBar = pFrame->GetMenuBar(); 
		wxASSERT(pMenuBar != NULL);
		wxMenuItem * pAdvBookMode = pMenuBar->FindItem(ID_ADVANCED_BOOKMODE);
		//wxASSERT(pAdvBookMode != NULL);
		if (gpApp->m_bBookMode && !gpApp->m_bDisableBookMode)
		{
			// mark it checked
			if (pAdvBookMode != NULL)
			{
				pAdvBookMode->Check(TRUE);
			}
		}
		else
		{
			// mark it unchecked
			if (pAdvBookMode != NULL)
			{
				pAdvBookMode->Check(FALSE);
			}
		}

		// BEW added 02Nov05
        // Even though the first placement of the phrase box makes the document
        // theoretically dirty, it should never really give any problem for us to declare
        // the document clean when just reopened, so we'll do so because we then buy the
        // behaviour that a just-opened document is immediately ready for a Split operation
        // without the user having to realize a save must first be done to get the document
        // clean for the Split to be enabled
		pDoc->Modify(FALSE);
		gpApp->RefreshStatusBarInfo();

#ifdef SHOW_DOC_I_O_BENCHMARKS
		dt1 = dt2;
		dt2 = wxDateTime::UNow();
		wxLogDebug(_T("OnWizardFinish - OpenDocument executed in %s ms"), 
						(dt2 - dt1).Format(_T("%l")).c_str());
#endif

		if (pApp->m_bReadOnlyAccess)
		{
			// try get an extra paint job done, so background will show all pink from the
			// outset
			pView->canvas->Refresh();
		}
		return; //return CPropertyPage::OnWizardFinish();
	}
}

void CDocPage::OnBnClickedCheckChangeFixedSpacesToRegularSpaces(wxCommandEvent& WXUNUSED(event))
{
	bChangeFixedSpaceToRegularSpace = pChangeFixedSpaceToRegular->GetValue();
}

void CDocPage::OnLbnSelchangeListNewdocAndExistingdoc(wxCommandEvent& WXUNUSED(event))
{
	// whm note: under Linux/GTK this "SelChanged..." method is also called if the
	// listbox Clear() method is called (which it is in OnSetActive), so we must bleed
	// off the case by executing an immediate return if there are no items in the list
	// box (which is the case after a call to Clear).
	if (m_pListBox->GetCount() == 0)
		return;
	
	// This handler executes also on a double click event. The double click
	// event is then passed on to the OnDblclkListNewAndExistingDoc() handler
	int nSel;
	wxString tempStr;
	nSel = m_pListBox->GetSelection();
	// wx note: On Linux/GTK is it possible for all listbox items to be deselected by the 
	// user or by other events (intervening dialog), so we must be able to handle -1 values 
	// from all GetSelection() calls gracefully - by insuring that the first (and maybe only) 
	// item in the listbox is selected before moving on to GetString() or GetStringSelection()
	// calls which assert if called on -1.
	if (nSel == -1)
	{
		m_pListBox->SetSelection(0);
		nSel = 0;
	}
	tempStr = m_pListBox->GetString(nSel);
	if (tempStr.GetChar(0) == _T('<')) // check for an initial < (because localizing may 
									   // produce a different text); this is how we tell
									   // that the user chose <New Document>
	{
		// User clicked on <New Document>, so cause the Fixed Space ... check box control
		// to appear
		pChangeFixedSpaceToRegular->Show(TRUE); // start with fixed space ... check box showing
		
	}
	else
	{
		// hide the Fixed Space... check box control
		pChangeFixedSpaceToRegular->Show(FALSE); // start with fixed space ... check box hidden
	}
	pDocPageSizer->Layout();

}

