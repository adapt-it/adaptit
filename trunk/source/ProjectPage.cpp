/////////////////////////////////////////////////////////////////////////////
/// \project		AdaptItWX
/// \file			ProjectPage.cpp
/// \author			Bill Martin
/// \date_created	3 May 2004
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public LIcense v. 1.0 AND The wxWindows Library Licence (see License.txt)
/// \description	This is the implementation file for the CProjectPage class. 
/// The CProjectPage class creates a panel that is used in the Edit Preferenced property sheet. 
/// The CProjectPage class allows the user to choose an existing project to work on or 
/// a <New Project>.
/// The interface resources for the CProjectPage are defined in ProjectPageFunc() 
/// which was developed and is maintained by wxDesigner.
/// The CProjectPage class is derived from wxWizardPage, rather than wxWizardPageSimple
/// in order to utilize its GetPrev() and GetNext() handlers as a means to facilitate
/// an 8-page wizard when the user selects <New Project>.
/// \derivation		CProjectPage is derived from the wxWizardPage class.
/////////////////////////////////////////////////////////////////////////////
// Pending Implementation Items in ProjectPage.cpp (in order of importance): (search for "TODO")
// 1. 
//
// Unanswered questions: (search for "???")
// 1. 
// 
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "ProjectPage.h"
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
#include "ProjectPage.h"
#include "LanguagesPage.h"
#include "FontPage.h"
#include "PunctCorrespPage.h"
#include "CaseEquivPage.h"
#include "USFMPage.h"
#include "FilterPage.h"
#include "DocPage.h"
#include "StartWorkingWizard.h"
#include "Adapt_It.h"

#include "KB.h" 
#include "Adapt_ItView.h"
#include "Adapt_ItDoc.h"

// globals

/// This global is defined in Adapt_It.cpp.
extern CStartWorkingWizard* pStartWorkingWizard; 

/// This global is defined in Adapt_It.cpp.
extern CProjectPage* pProjectPage;

/// This global is defined in Adapt_It.cpp.
extern CLanguagesPage* pLanguagesPage;

//extern CPunctCorrespPageWiz* pPunctCorrespPageWiz;

//extern CCaseEquivPageWiz* pCaseEquivPageWiz;
//extern CUSFMPageWiz* pUsfmPageWiz;
//extern CFilterPageWiz* pFilterPageWiz;

/// This global is defined in Adapt_It.cpp.
extern CDocPage* pDocPage;

/// This global is defined in Adapt_It.cpp.
extern CAdapt_ItApp*	gpApp; // if we want to access it fast

/// This global is defined in Adapt_It.cpp.
extern bool gbWizardNewProject; // for initiating a 4-page wizard

IMPLEMENT_DYNAMIC_CLASS( CProjectPage, wxWizardPage )

// event handler table
BEGIN_EVENT_TABLE(CProjectPage, wxWizardPage)
	EVT_INIT_DIALOG(CProjectPage::InitDialog)// not strictly necessary for dialogs based on wxDialog
    EVT_WIZARD_PAGE_CHANGING(-1, CProjectPage::OnWizardPageChanging) // handles MFC's OnWizardNext() and OnWizardBack
    EVT_WIZARD_CANCEL(-1, CProjectPage::OnWizardCancel)
	EVT_LISTBOX_DCLICK(IDC_LIST_NEW_AND_EXISTING, CProjectPage::OnCallWizardNext)// double click simiulates OnWizardNext
	EVT_LISTBOX(IDC_LIST_NEW_AND_EXISTING, CProjectPage::OnLBSelectItem)
	EVT_BUTTON(IDC_BUTTON_WHAT_IS_PROJECT, CProjectPage::OnButtonWhatIsProject)
END_EVENT_TABLE()

CProjectPage::CProjectPage()
{
}

CProjectPage::CProjectPage(wxWizard* parent)
{
	// CProjectPage's constructor is called when CProjectPage is used in both the Start 
	// Working Wizard and when used in Edit Preferences.
	
	Create( parent );
	
	m_pListBox = (wxListBox*)FindWindowById(IDC_LIST_NEW_AND_EXISTING);
	wxASSERT(m_pListBox != NULL);

	wxTextCtrl* pTextCtrlAsStaticProjectPage = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_AS_STATIC_PROJECTPAGE);
	wxASSERT(pTextCtrlAsStaticProjectPage != NULL);
	wxColor backgrndColor = this->GetBackgroundColour();
	//pTextCtrlAsStaticProjectPage->SetBackgroundColour(backgrndColor);
	pTextCtrlAsStaticProjectPage->SetBackgroundColour(gpApp->sysColorBtnFace);

	m_curLBSelection = 0; // default to select the first item in list
}

CProjectPage::~CProjectPage() // destructor
{
	
}

bool CProjectPage::Create( wxWizard* parent)
{
	wxWizardPage::Create( parent );
	CreateControls();
	GetSizer()->Fit(this);
	return TRUE;
}

void CProjectPage::CreateControls()
{
	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	pProjectPageSizer = ProjectPageFunc(this, TRUE, TRUE);
}

// implement wxWizardPage functions GetPrev() and GetNext()
wxWizardPage* CProjectPage::GetPrev() const 
{ 
	// The GetPrev function is never used since there are no wizard pages prior to 
	// the projectPage.
	return NULL;
}
wxWizardPage* CProjectPage::GetNext() const
{
	// Determine if existing project is selected and, if so, return the docPage.
	// If <New Project> was selected then return the next page in the longer wizard,
	if (gbWizardNewProject)
		return pLanguagesPage;
	else
		return pDocPage;
}

void CProjectPage::OnLBSelectItem(wxCommandEvent& WXUNUSED(event))
{
	int nSel;
	nSel = m_pListBox->GetSelection();
	if (m_curLBSelection == nSel)
	{
		// Linux listboxes get automatically unselected when you click on an already
		// selected item. m_cruLBSelection has what was previously selected before the
		// current selection event, so, if the new selection (nSel) is the same we want
		// to make sure the item stays selected/highlighted in the list
		m_pListBox->SetSelection(nSel); //,TRUE);
	}
	if (nSel == -1) //LB_ERR
	{
		::wxBell();
		return;
	}
	wxString selStr;
	selStr = m_pListBox->GetStringSelection();
	if (selStr.GetChar(0) == _T('<'))	// the name might change in a localized version, 
										// so look for < as the indicator that <New Project>
										// was chosen by the user, rather than an existing one
	{
		gbWizardNewProject = TRUE;
	}
	else
	{
		gbWizardNewProject = FALSE;
	}
}

void CProjectPage::OnCallWizardNext(wxCommandEvent& WXUNUSED(event))
{
	// simulate a doubleclick as a click on "Next >" button by calling the appropriate ShowPage
	// Note: ShowPage() triggers OnWizardPageChanging, etc.
	if (gbWizardNewProject)
		pStartWorkingWizard->ShowPage(pLanguagesPage,TRUE); // TRUE = going forward
	else
		pStartWorkingWizard->ShowPage(pDocPage,TRUE); // TRUE = going forward
}

void CProjectPage::OnWizardCancel(wxWizardEvent& WXUNUSED(event))
{
    //if ( wxMessageBox(_T("Do you really want to cancel?"), _T("Question"),
    //                    wxICON_QUESTION | wxYES_NO, this) != wxYES )
    //{
    //    // not confirmed
    //    event.Veto();
    //}
}

// This InitDialog is called from the DoStartWorkingWizard() function
// in the App
void CProjectPage::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{
	//InitDialog() is not virtual, so no call needed to a base class.
	//
	// CProjectPage's InitDialog() handler is NOT automatically called when CProjectPage 
	// is used as a page in the Start Working Wizard. It must be called explicitly in program 
	// code to execute. We've made it a public function in ProjectPage.h and call it 
	// explicitly in the App's DoStartWorkingWizard().
	// TODO: Add note here to same effect if this behavior is also true for the edit preferences
	// dialog in the View's OnEditPreferencesDlg() function.

	CAdapt_ItApp* pApp = (CAdapt_ItApp*)&wxGetApp();
	wxASSERT(pApp);

	// if the user has tabbed back to this page while there is a project open,
	// that project has to be closed and its doc saved (ie user given chance to
	// do so), similarly for KB, before project can be made or existing one opened
	if (pApp->m_pKB != NULL)
	{
		pApp->GetView()->CloseProject();
	}
		
	// first, use the current navigation text font for the list box
	#ifdef _RTL_FLAGS
	gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pNavTextFont, NULL, NULL,
								m_pListBox, NULL, gpApp->m_pDlgSrcFont, gpApp->m_bNavTextRTL);
	#else // Regular version, only LTR scripts supported, so use default FALSE for last parameter
	gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pNavTextFont, NULL, NULL, 
								m_pListBox, NULL, gpApp->m_pDlgSrcFont);
	#endif

	// generate a CStringList of all the possible adaption projects
	wxArrayString possibleAdaptions;
	possibleAdaptions.Clear();
	m_pListBox->Clear();
	wxString str;
	// IDS_NEW_PROJECT
	str = str.Format(_("<New Project>"));
	possibleAdaptions.Add(str);
	pApp->GetPossibleAdaptionProjects(&possibleAdaptions);

	// fill the list box with the folder name strings
	wxString showItem;
	size_t ct = possibleAdaptions.GetCount();
	if (ct == 1)
	{
		// with just one in listbox the only possible choice is <New Project>
		// so set the global gbWizardNewProject to TRUE
		gbWizardNewProject = TRUE;
	}
	for (size_t index = 0; index < ct; index++)
	{
		showItem = possibleAdaptions.Item(index);
		m_pListBox->Append(showItem);
	}
	possibleAdaptions.Clear();

	// hilight the folder name of the project obtained from the profile settings, if possible
	// otherwise the first in the list
	if (pApp->m_curProjectName.IsEmpty())
	{
		wxASSERT(m_pListBox->GetCount() > 0);
		m_pListBox->SetSelection(0);
	}
	else
	{
		// must check if nReturned == LB_ERR, since, for example, if the user created a project then
		// manually deleted the project from the Adaptit Word folder externally to the application, then
		// it would try find it and fail, and the app would crash
		//if (nReturned == -1) //LB_ERR
		//	m_listBox.SetSelection(0,TRUE); // use the first one as default, if there was an error
		// Since SetStringSelection does not return a value in wx, we need to make
		// sure it actually exists first
		int selItem = m_pListBox->FindString(pApp->m_curProjectName);
		if (selItem != -1)
		{
			m_pListBox->SetSelection(selItem);
			if (selItem > 0)
				m_pListBox->SetFirstItem(selItem -1); // make the item visible if it would require scrolling whm added 10Mar08
		}
		else
		{
			wxASSERT(m_pListBox->GetCount() > 0);
			m_pListBox->SetSelection(0);
		}
		m_curLBSelection = selItem;
	}
	// insure the listbox is in focus
	m_pListBox->SetFocus();
	// make the list boxes scrollable
	// whm note: wxDesigner has the listbox style set for wxLB_HSCROLL which creates a horizontal scrollbar
	// if contents are too wide (Windows only)
}
void CProjectPage::OnButtonWhatIsProject(wxCommandEvent& WXUNUSED(event)) 
{
	wxString s;
	wxString accum;
	// IDS_PROJECT_STORES
	s = s.Format(_("A project stores your translation documents."));
	accum += s;
	accum += _T(' ');
	// IDS_IT_ALSO_STORES
	s = s.Format(_("It also stores your typed adaptations for the source text's words and phrases in a file called a \"knowledge base\". If you do any glossing, glosses will be stored in a different knowledge base."));
	accum += s;
	accum += _T(' ');
	// IDS_MAKES_A_NAME
	s = s.Format(_("Adapt It makes a name for a project by asking you to type in two names - the source language name and the target language name. For a given pair of languages, you do this only once."));
	accum += s;
	accum += _T(' ');
	// IDS_YOU_DONT_HAVE
	s = s.Format(_("You don't have to use language names for setting up a project. You can use any two names you like."));
	accum += s;
	accum += _T(' ');
	// IDS_DOCS_ARE_DIFFERENT
	s = s.Format(_("Documents are different from projects. Each project will store many documents. For example, a project for adapting a New Testament will store a document for each book, or part of a book, that you translate."));
	accum += s;
	accum += _T(' ');
	// IDS_IN_THE_LIST
	s = s.Format(_("In the list below, if you do not have your project set up yet, double click <New Project>. If it is already set up, you will see its name in the list - double click the name to open that project."));
	accum += s;

	wxMessageBox(accum, _T(""), wxICON_INFORMATION);
}

void CProjectPage::OnWizardPageChanging(wxWizardEvent& event)
// modified to handle glossing KB as well as normal KB, for version 2.0 and onwards
// Note: This method is also called when the user double clicks on a project or
// <New Project> listed in the list box of the projectPage.
{
	CAdapt_ItApp* pApp = (CAdapt_ItApp*)&wxGetApp();
	wxASSERT(pApp);
	int nSel;
	
	bool bMovingForward = event.GetDirection(); 
	wxASSERT(bMovingForward == TRUE); // we can only move forward from the projectPage

	nSel = m_pListBox->GetSelection();
	if (nSel == -1) //LB_ERR
	{
		// under wxGTK the user can deselect all items in the list so we'll check for that
		// possibility; if there are more items in the list (other than <New Project>), we'll
		// veto the page change until the user selects something; if there is only one item
		// in the list, we know it must be <New Project> and will assume that item is what the
		// user intended and select it automatically if it is not selected.
		if (m_pListBox->GetCount() > 1)
		{
			wxMessageBox(_("You must Select a project (or <New Project>) from the list before continuing."), _T(""), wxICON_EXCLAMATION);
			event.Veto();
			return;
		}
		else
		{
			wxASSERT(m_pListBox->GetCount() == 1);
			// User deselected <New Project> (possibly by clicking on it once in wxGTK), but since it 
			// is the only item listed (GetCount should have returned 1), we assume the user wants to 
			// start a <New Project>, so we'll select it for him automatically and allow the page to 
			// change with <New Project> selected.
			m_pListBox->SetSelection(0);
			nSel = m_pListBox->GetSelection();
			wxASSERT(nSel == 0);
		}
	}
	m_projectName = m_pListBox->GetStringSelection();

	// there might be a KB open currently (eg. just returned via the <Back button) so ensure 
	// it is clobbered, otherwise the existing project branch would fail; do the same for
	// the glossing KB
	// whm - this removal of any existing the KBs structures in memory should be done at this
	// point whether the projectPage is moving forward or backwards.
	if (pApp->m_pKB != NULL)
	{
		delete pApp->m_pKB;
		pApp->m_bKBReady = FALSE;
		pApp->m_pKB = (CKB*)NULL;
	}
	if (pApp->m_pGlossingKB != NULL)
	{
		delete pApp->m_pGlossingKB;
		pApp->m_bGlossingKBReady = FALSE;
		pApp->m_pGlossingKB = (CKB*)NULL;
	}

	if (bMovingForward) // we can only move forward from the projectPage
	{
		// user selected "Next >"
		if (m_projectName.GetChar(0) == _T('<')) // the name might change in a localized version, 
											// so look for < as the indicator that <New Project>
											// was chosen by the user, rather than an existing one
		{
			// we're starting a new project, so we'll show the full wizard
			m_projectName = _T(""); // start off empty

			wxASSERT(this == pProjectPage);
			// The following flag causes the full wizard to be run
			gbWizardNewProject = TRUE;

			// ensure app does not try to restore last saved doc and active location
			pApp->m_bEarlierProjectChosen = FALSE;
			pApp->nLastActiveSequNum = 0;

			// set default character case equivalences for a new project
			pApp->SetDefaultCaseEquivalences();

			// A new project will not yet have a project config file, so
			// set the new project's filter markers list to be equal to 
			// pApp->gCurrentFilterMarkers, which, if the program was just started,
			// was initialized by SetupMarkerStrings in InitInstance.
			pApp->gProjectFilterMarkersForConfig = pApp->gCurrentFilterMarkers;
			
			// Movement through wizard pages is sequential - the next page is the languagesPage.
			// The pLanguagesPage's InitDialog need to be called here just before going to it
			wxInitDialogEvent idevent;
			pLanguagesPage->InitDialog(idevent);
		}
		else
		{
			// it's an existing project, so we'll create KBs for it and show only the
			// two-page wizard (Project Page and Doc Page)

			// fill out the app's member variables for the paths etc.
			pApp->m_curProjectName = m_projectName;
			pApp->m_curProjectPath = pApp->m_workFolderPath + pApp->PathSeparator + pApp->m_curProjectName;

			// make sure the path to the Adaptations folder is correct (if omitted, it would use
			// the basic config file's "DocumentsFolderPath" line - which could have been the
			// Adaptations folder in a different project)
			pApp->m_curAdaptionsPath = pApp->m_curProjectPath + pApp->PathSeparator + pApp->m_adaptionsFolder;

			// BEW moved to here, 19Aug05, so that m_bSaveAsXML flag is read back in before we
			// try to set up the KB files.
			// Get project configuration from the config files in the project folder & set up fonts,
			// punctuation settings, colours, and default cc table path as per those files; and set
			// book mode on or off depending on what is in the config file, etc.
			CAdapt_ItDoc* pDoc = pApp->GetDocument();
			wxASSERT(pDoc);
			pDoc->GetProjectConfiguration(pApp->m_curProjectPath);

			// BEW changes 19Aug05 for XML versus binary support...
			// set up the expected KB and GlossingKB paths etc
			gpApp->SetupKBPathsEtc();

			// determine whether user opened the same project as was last saved to the 
			// app-level config file
			pApp->m_bEarlierProjectChosen = FALSE;
			if (!pApp->m_lastDocPath.IsEmpty())
			{
				int nFound = pApp->m_lastDocPath.Find(pApp->m_curProjectPath);
				if (nFound != -1)
				{
					// it is a subpath, so same project was chosen
					pApp->m_bEarlierProjectChosen = TRUE;
				}
			}

			// open the knowledge base and load its contents
			wxASSERT(pApp->m_pKB == NULL);
			pApp->m_pKB = new CKB;
			wxASSERT(pApp->m_pKB != NULL);
			bool bOK = pApp->LoadKB();
			if (bOK)
			{
				pApp->m_bKBReady = TRUE;

				// now do it for the glossing KB
				wxASSERT(pApp->m_pGlossingKB == NULL);
				pApp->m_pGlossingKB = new CKB;
				wxASSERT(pApp->m_pGlossingKB != NULL);
				bOK = pApp->LoadGlossingKB();
				if (bOK)
				{
					pApp->m_bGlossingKBReady = TRUE;
				}
				else
				{
					// IDS_GLOSSINGKB_OPEN_FAILED
					wxMessageBox(_("Sorry, loading the glossing knowledge base failed, and then the attempt to substitute a new (empty) one also failed. This error is fatal."), _T(""), wxICON_ERROR);
					wxASSERT(FALSE);
					wxExit();
				}

				// do the KB backing up, if the user wants it done; inform the user if it is
				// currently turned off
				if (pApp->m_bAutoBackupKB)
				{
					pApp->DoKBBackup(); // use the bSuppressOKMessage = TRUE option
				}
				else
				{
					// IDS_KB_BACKUP_OFF
					wxMessageBox(_("A reminder: backing up of the knowledge base is currently turned off.\nTo turn it on again, see the Knowledge Base tab within the Preferences dialog."),_T(""), wxICON_INFORMATION);
				}
			}
			else
			{
				// the load of the normal adaptation KB didn't work and the substitute empty KB 
				// was not created successfully, so delete the adaptation CKB & advise the user 
				// to Recreate the KB using the menu item for that purpose. Loading of the glossing
				// KB will not have been attempted if we get here.
				if (pApp->m_pKB != NULL)
					delete pApp->m_pKB;
				pApp->m_bKBReady = FALSE;
				pApp->m_pKB = (CKB*)NULL;
				// IDS_KB_NEW_EMPTY_FAILED
				wxMessageBox(_("Sorry, substituting a new empty knowledge base failed. Instead you should now try the Restore Knowledge Base command in the File menu. You need a valid knowledge base before doing any more work."),_T(""), wxICON_INFORMATION);

				pStartWorkingWizard->Show(FALSE);
				pStartWorkingWizard->EndModal(1);
				pStartWorkingWizard = (CStartWorkingWizard*)NULL;
			}

			// The pDocPage's InitDialog need to be called here just before going to it
			// make sure the pDocPage is initialized to show the documents for the selected project
			wxInitDialogEvent idevent;
			pDocPage->InitDialog(idevent);

		}


	}
}
