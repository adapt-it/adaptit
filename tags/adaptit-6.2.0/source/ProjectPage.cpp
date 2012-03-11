/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			ProjectPage.cpp
/// \author			Bill Martin
/// \date_created	3 May 2004
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
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
#include <wx/progdlg.h> // for wxFileName

#include "LanguagesPage.h"
#include "FontPage.h"
#include "PunctCorrespPage.h"
#include "CaseEquivPage.h"
#include "UsfmFilterPage.h"
#include "DocPage.h"
#include "scrollingwizard.h" // whm added 13Nov11 for wxScrollingWizard - need to include this here before "StartWorkingWizard.h" below
#include "StartWorkingWizard.h"
#include "Adapt_It.h"
#include "helpers.h"
#include "CollabUtilities.h"
#include "KB.h" 
#include "Adapt_ItView.h"
#include "Adapt_ItDoc.h"
#include "Adapt_ItCanvas.h"
#include "MainFrm.h"
#include "WaitDlg.h"
#include "ChooseCollabOptionsDlg.h"
#include "GetSourceTextFromEditor.h"
#include "ReadOnlyProtection.h"
#include "ProjectPage.h"

// globals

/// This global is defined in Adapt_It.cpp.
extern CStartWorkingWizard* pStartWorkingWizard; 

/// This global is defined in Adapt_It.cpp.
extern CProjectPage* pProjectPage;

/// This global is defined in Adapt_It.cpp.
extern CLanguagesPage* pLanguagesPage;

/// This global is defined in Adapt_It.cpp.
extern CDocPage* pDocPage;

/// This global is defined in Adapt_It.cpp.
extern CAdapt_ItApp*	gpApp; // if we want to access it fast

/// This global is defined in Adapt_It.cpp.
extern bool gbWizardNewProject; // for initiating a 4-page wizard

/// This global is defined in Adapt_It.cpp
extern wxString szProjectConfiguration;

IMPLEMENT_DYNAMIC_CLASS( CProjectPage, wxWizardPage )

// event handler table
BEGIN_EVENT_TABLE(CProjectPage, wxWizardPage)
	EVT_INIT_DIALOG(CProjectPage::InitDialog)
    EVT_WIZARD_PAGE_CHANGING(-1, CProjectPage::OnWizardPageChanging) // handles MFC's OnWizardNext() and OnWizardBack
	EVT_WIZARD_PAGE_CHANGED(-1, CProjectPage::OnWizardPageChanged)
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
	return TRUE;
}

void CProjectPage::CreateControls()
{
	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
    
	// Testing below !!!
 //   pProjectPageSizer = new wxBoxSizer(wxVERTICAL);
 //   this->SetSizer(pProjectPageSizer); // pProjectPageSizer is the top level sizer

	//wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
	//// add a vertical sizer to the top level sizer
	//pProjectPageSizer->Add(itemBoxSizer2,1,wxGROW|wxALL,5);

	//wxBoxSizer* itemBoxSizer3 = new wxBoxSizer(wxVERTICAL);
	//// add another vertical sizer to the one just added
 //   itemBoxSizer2->Add(itemBoxSizer3, 1, wxGROW|wxALL, 5);

	//// create our scrolled window whose parent is the overall page  
	//m_scrolledWindow = new wxScrolledWindow( this, -1, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxHSCROLL|wxVSCROLL );
	//// now create the rest of the dialog controls using the wxDesigner function with our scrolled
	//// window as their parent window
	//wxSizer* itemBoxSizer4 = ProjectPageFunc(m_scrolledWindow, TRUE, TRUE);
	//// finally add the sizer representing those controls to the last vertical sizer we created above
	//itemBoxSizer3->Add(itemBoxSizer4, 1, wxGROW|wxALL, 5);
	// Testing above !!!
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
	// whm: We could use our ListBoxPassesSanityCheck() here, but the logic below
	// should be sufficient for this handler. We use it in the OnWizardPageChanging()
	// handler below.

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

	pApp->GetPossibleAdaptionProjects(&possibleAdaptions);
	
	// whm modified 28Jul11 to sort possibleAdaptations before adding the
	// <New Project> as the first item. This change is needed to get a sorted
	// list on the Linux port. Windows and Mac seem to grab the list of folders
	// in sorted order.
	possibleAdaptions.Sort();

	// Add <New Project> to the listbox unless the current user profile says to hide 
	// the <New Project> item from the interface
	if (gpApp->m_bShowNewProjectItem)
	{
		wxString str;
		// IDS_NEW_PROJECT
		str = str.Format(_("<New Project>"));
		possibleAdaptions.Insert(str,0);
	}

	// fill the list box with the folder name strings
	wxString showItem;
	size_t ct = possibleAdaptions.GetCount();
	// whm 11Aug11 modified test below to examine the first item in
	// possibleAdaptions to see if it is _("<New Project>") rather 
	// than testing if the count ct is 1 to determine whether the
	// gbWizardNewProject flag should be set to TRUE or not. This is
	// necessary because when the user profile removes the <New Project> 
	// item from the list, the count is not a sufficient test.
	if (ct == 1 && possibleAdaptions.Item(0) == _("<New Project>"))
	{
		// with just one in listbox and the only possible choice is 
		// <New Project>, set the global gbWizardNewProject to TRUE
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
		// manually deleted the project from the work folder externally to the application, then
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
	// ensure the listbox is in focus
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

// whm added 10Mar12
void CProjectPage::OnWizardPageChanged(wxWizardEvent& event)
{
	bool bDirection;
	bDirection = event.GetDirection();
	if (bDirection == FALSE)
	{
		// whm 10Mar12 Note.
		// We changed to the ProjectPage via a < Back click from a later page.
		// In this case we need to reset any read-only settings that may have 
		// been in effect, namely we call RemoveReadOnlyProtection(), set 
		// the App's m_bReadOnlyAccess and m_bFictitiousReadOnlyAccess to FALSE, 
		// and do a canvas->Refresh().
		// This block of code is similar to blocks called in:
		//    CStartWorkingWizard::OnCancel()
		//    ProjectPage::OnWizardPageChanged()
		//    CAdapt_ItDoc::EraseKB()
		//    CAdapt_ItDoc::OnFileClose() [m_bFictitiousReadOnlyAccess not set FALSE here]
		// Note: All but the gpApp->m_bFictitiousReadOnlyAccess = FALSE statement
		// is also called in CAdapt_ItDoc::OnFileClose() which shouldn't reset the
		// Apps m_bFictitiousReadOnlyAccess flag. 
		wxASSERT(gpApp != NULL);
		wxASSERT(gpApp->GetView() != NULL);
		wxASSERT(gpApp->GetView()->canvas != NULL);
		wxASSERT(gpApp->m_pROP != NULL);
		if (!gpApp->m_curProjectPath.IsEmpty())
		{
			bool bRemoved = gpApp->m_pROP->RemoveReadOnlyProtection(gpApp->m_curProjectPath);
			bRemoved = bRemoved; // to avoid warning
			// we are leaving this folder, so the local process must have m_bReadOnlyAccess unilaterally
			// returned to a FALSE value - whether or not a ~AIROP-*.lock file remains in the folder
			gpApp->m_bReadOnlyAccess = FALSE;
			// whm 7Mar12 added. The project is being closed, so unilaterally set m_bFictitiousReadOnlyAccess
			// to FALSE
			gpApp->m_bFictitiousReadOnlyAccess = FALSE; // ditto
			gpApp->GetView()->canvas->Refresh(); // force color change back to normal white background
		}
	}
}

void CProjectPage::OnWizardPageChanging(wxWizardEvent& event)
// modified to handle glossing KB as well as normal KB, for version 2.0 and onwards
// Note: This method is also called when the user double clicks on a project or
// <New Project> listed in the list box of the projectPage.
{
	CAdapt_ItApp* pApp = (CAdapt_ItApp*)&wxGetApp();
	wxASSERT(pApp);
	//int nSel;
	
	bool bMovingForward = event.GetDirection(); 
	wxASSERT(bMovingForward == TRUE); // we can only move forward from the projectPage
	
	if (!ListBoxPassesSanityCheck((wxControlWithItems*)m_pListBox))
	{
		wxMessageBox(_("You must Select a project (or <New Project>) from the list before continuing."), _T(""), wxICON_EXCLAMATION);
		event.Veto();
		return;
	}

	m_projectName = m_pListBox->GetStringSelection();

	// there might be a KB open currently (eg. just returned via the <Back button) so ensure 
	// it is clobbered, otherwise the existing project branch would fail; do the same for
	// the glossing KB
	// whm - this removal of any existing the KBs structures in memory should be done at this
	// point whether the projectPage is moving forward or backwards.
	if (pApp->m_pKB != NULL || pApp->m_pGlossingKB != NULL)
	{
		UnloadKBs(pApp);
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
			
            // Movement through wizard pages is sequential - the next page is the
            // languagesPage. The pLanguagesPage's InitDialog need to be called here just
            // before going to it
			wxInitDialogEvent idevent;
			pLanguagesPage->InitDialog(idevent);
		}
		else
		{
			// Note: The name of the project entered is stored in the local m_projectName
			// variable
			// 
			// it's an existing project, so we'll create KBs for it and show only the
			// two-page wizard (Project Page and Doc Page)

            // Roland Fumey requested that AI show a progress dialog during the project
            // loading since large KBs can take a while to create backup copies and load.
			
			// fill out the app's member variables for the paths etc.
			pApp->m_curProjectName = m_projectName;
			// BEW added 9Sep09, test for a custom work folder path & if to be used, use it
			if (!pApp->m_customWorkFolderPath.IsEmpty() && pApp->m_bUseCustomWorkFolderPath)
			{
				pApp->m_curProjectPath = pApp->m_customWorkFolderPath + pApp->PathSeparator 
										 + pApp->m_curProjectName;
			}
			else
			{
				pApp->m_curProjectPath = pApp->m_workFolderPath + pApp->PathSeparator 
										 + pApp->m_curProjectName;
			}
			pApp->m_sourceInputsFolderPath = pApp->m_curProjectPath + pApp->PathSeparator + 
											pApp->m_sourceInputsFolderName; 

            // make sure the path to the Adaptations folder is correct (if omitted, it
            // would use the basic config file's "DocumentsFolderPath" line - which could
            // have been the Adaptations folder in a different project)
			pApp->m_curAdaptionsPath = pApp->m_curProjectPath + pApp->PathSeparator 
									   + pApp->m_adaptionsFolder;

			// whm added 8Feb12. The user may have created an "empty" project, for example
			// "test1 to test2 adaptations" by simply creating such a folder in the "Adapt It
			// Unicode Work" folder. In this case AI will also need to have an "Adaptations" 
			// folder within the user-created project folder if it does not already exist
			if (!::wxDirExists(pApp->m_curAdaptionsPath))
			{
				// The "Adaptations" folder does not yet exist, so create it
				bool bOK = FALSE;
				bOK = ::wxMkdir(pApp->m_curAdaptionsPath);
				if (!bOK)
				{
					wxString msg = _T("Unable to create an Adaptations folder within the %s project folder");
					msg = msg.Format(msg,pApp->m_curProjectName.c_str());
					wxMessageBox(msg,_T(""),wxICON_WARNING);
					event.Veto();
					return;
				}
			}

			// Read the project config file for all of the project settings including the
			// project-specific collaboration settings to be in effect when 
			// GetSourceTextFromEditorDlg dialog is invoked after the wizard terminates below. 
			// When the collaboration-related settings were stored app-wide in the basic 
			// configuration file, it would have been read in its entirety before invoking
			// the GetSourceTextFromEditorDlg dialog.
			gpApp->GetProjectConfiguration(pApp->m_curProjectPath);

			// whm modified 18Feb12. Now that the project config file has been read we 
			// can determine what the collaboration settings are for this AI project. If
			// collaboration with PT/BE has been setup by the administrator for hooking
			// up with this project, we query the user if s/he wants to turn collaboration
			// ON, OFF, or (for advisors/consultants mainly) open the project read-only.
			// We query via the ChooseCollabOptionsDlg dialog.
			if (pApp->AIProjectIsACollabProject(m_projectName))
			{
				CChooseCollabOptionsDlg collabOptDlg(pStartWorkingWizard);
				collabOptDlg.Center();
				collabOptDlg.m_aiProjName = m_projectName;
				if (collabOptDlg.ShowModal() == wxID_CANCEL)
				{
					// We don't need to revert any project configuration settings before return 
					// from a Cancel because the AIProjectIsACollabProject() function called at
					// the top of this block did not assign any configuration settings when it
					// snooped in the m_projectName's project config file.
					// Call the wxWizard's Veto() method here before the return is executed. It
					// causes control to go back to the wizard just as it was before the "Next >"
					// button was pressed, so the user can choose the same or different AI project
					// again.
					event.Veto();
					return;
				}
				else
				{
					// The user made a selection. Get the selection and handle it
					// Note: The collabOptDlg.m_bEditorIsAvailable is set in the CChooseCollabOptionsDlg
					// based on the value of the App's m_bParatextIsInstalled and m_bBibleditIsInstalled 
					// flags.
					if (collabOptDlg.m_bRadioSelectCollabON && collabOptDlg.m_bEditorIsAvailable)
					{
						// Make sure that book folder mode is OFF.
						if (pApp->m_bBookMode)
						{
							// Quietly turn if off. When an administrator sets up a collaboration project
							// the App turns book folder mode off, but just in case someone edited the project
							// config file to turn it back on, we'll play safe here
							pApp->m_bBookMode = FALSE;
							pApp->m_nBookIndex = -1;
							pApp->m_nDefaultBookIndex = 39;
							pApp->m_nLastBookIndex = 39;
						}

						// The user selected to "Turn Collaboration ON and a collaboration editor is 
						// available for that purpose. With project-specific collaboration the user now 
						// determines the values for m_bCollaboratingWithParatext 
						// and m_bCollaboratingWithBibledit having made that decision in the
						// CChooseCollabOptionsDlg dialog.
						if (pApp->m_collaborationEditor == _T("Paratext"))
						{
							pApp->m_bCollaboratingWithParatext = TRUE;
						}
						else if (pApp->m_collaborationEditor == _T("Bibledit"))
						{
							pApp->m_bCollaboratingWithBibledit = TRUE;
						}
						
						pApp->LogUserAction(_T("Collaboration turned ON by user"));
						
						// The user wants to start the collaboration session.
						// In this case we set the App's m_bJustLaunched flag to cause the 
						// main frame's OnIdle() method to call DoStartupWizardOnLaunch(), and 
						// also set the m_bStartWorkUsingCollaboration flag to put up the 
						// GetSourceTextFromEditorDlg dialog instead of the actual wizard.
						// Then we kill the current wizard session by calling EndModal().
						pApp->m_bJustLaunched = TRUE;
						// This should be the only place in the app where m_bStartWorkUsingCollaboration
						// is set to TRUE.
						pApp->m_bStartWorkUsingCollaboration = TRUE;

						// Set the File > Open and File > Save menu items to have the parenthetical information
						// during collaboration.
						pApp->MakeMenuInitializationsAndPlatformAdjustments();

						// whm Note: After the wizard closes (below), the GetSourceTextFromEditorDlg dialog
						// will appear, and when the user clicks OK in that dialog, the HookUpToExistingAIProject()
						// function there will read the project config file, so we need to save this 
						// m_bStartWorkUsingCollaboration collaboration setting to the project config file
						// before leaving this block of control
						bool bOK;
						bOK = pApp->WriteConfigurationFile(szProjectConfiguration, pApp->m_curProjectPath,projectConfigFile);
						bOK = bOK; // was unused, so prevent compiler warning
						pStartWorkingWizard->Show(FALSE);
						pStartWorkingWizard->EndModal(1);
						pStartWorkingWizard = (CStartWorkingWizard*)NULL;
						// whm Note: Even though the wizard is being destroyed by the
						// above calls, the remainder of this OnWizardPageChanging() method
						// would continue to execute unless we call return at this point.
						return; // no Veto() called here as we want the wizard to end
					}
					else if (collabOptDlg.m_bRadioSelectCollabOFF && !collabOptDlg.m_bRadioSelectReadOnlyON)
					{
						// The user selected to "Turn Collaboration OFF", or a collaboration editor is 
						// not available.
						// The user now determines the values for m_bCollaboratingWithParatext 
						// and m_bCollaboratingWithBibledit and made that decision above.
						if (pApp->m_collaborationEditor == _T("Paratext"))
						{
							pApp->m_bCollaboratingWithParatext = FALSE;
						}
						else if (pApp->m_collaborationEditor == _T("Bibledit"))
						{
							pApp->m_bCollaboratingWithBibledit = FALSE;
						}
						// The user wants to continue through the wizard with no 
						// collaboration.
						
						pApp->LogUserAction(_T("Collaboration turned OFF by user"));
						
						pApp->m_bStartWorkUsingCollaboration = FALSE;
						
						// Set the File > Open and File > Save menu items back to their normal
						// state - without the parenthetical information in the labels.
						pApp->MakeMenuInitializationsAndPlatformAdjustments();
					}
					else if (collabOptDlg.m_bRadioSelectReadOnlyON)
					{
						// For the advisor/consultant's Read-Only Protection selection
						// we make sure that collaboration is OFF
						if (pApp->m_collaborationEditor == _T("Paratext"))
						{
							pApp->m_bCollaboratingWithParatext = FALSE;
						}
						else if (pApp->m_collaborationEditor == _T("Bibledit"))
						{
							pApp->m_bCollaboratingWithBibledit = FALSE;
						}
						
						pApp->LogUserAction(_T("Read-Only Mode turned ON by user"));
						
						// The administrator/consultant/user wants to continue through
						// the wizard with no collaboration, but with any document
						// opened in read-only mode.
						pApp->m_bStartWorkUsingCollaboration = FALSE;
						
						// whm Note: The ForceFictitiousReadOnlyProtection() function below does
						// the following:
						// Check for the existence of a zombie readonly protection file 
						// left after an abnormal exit, and check for someone having the project 
						// already open remotely for writing. We only set the fictitious ROPFile
						// in the project folder if it is NOT already owned, as we do not want them
						// to be disenfranchised by clobbering their readonly protection file and
						// substituting one for a fictitious process. ForceFictitiousReadOnlyProtection()
						// calls the IsTheProjectFolderOwnedForWriting() function which has the side
						// effects of: (1) Assigns any active non-zombie ROPFile's name to the App's
						// m_strOwningReadOnlyProtectionFilename member, and (2) removes any zombies.
						// As of version 6.1.0, we have disallowed any second instance of Adapt It 
						// being run by the same user, so if the folder is already owned, it would 
						// never be owned by "me". The only reason that bForcedRopOK would become
						// FALSE is if ForceFictitiousReadOnlyProtection() could not open the 
						// m_pROPwxFile for writing. That situation would be unusual, and if the App's
						// m_bReadOnlyAccess flag is also FALSE, we notify the user that read-only mode
						// could not be set for some unknown reason, and the user should proceed with
						// caution if they do not intend to make changes.
						bool bForcedRopOK;
						bForcedRopOK = pApp->m_pROP->ForceFictitiousReadOnlyProtection(pApp->m_curProjectPath);
						if (!bForcedRopOK)
						{
							// Could not create a fictitious ROPFile
							if (!pApp->m_bReadOnlyAccess)
							{
								wxString msg = _("Adapt It could not enter read-only mode for some unknown reason. You may continue to access the user's documents, but proceed with care.");
								wxMessageBox(msg,_("Read Only Protection Failed"),wxICON_WARNING);
								pApp->LogUserAction(msg);
							}
							else
							{
								wxASSERT_MSG(FALSE,_T("ForceFictitiousReadOnlyProtection() failed and m_bReadOnlyAccess is TRUE. Programmer Error?"));
							}
						}
						else
						{
							pApp->m_bFictitiousReadOnlyAccess = TRUE;
						}
						
						// Set the File > Open and File > Save menu items back to their normal
						// state - without the parenthetical information in the labels.
						pApp->MakeMenuInitializationsAndPlatformAdjustments();
					}
				}
			}

			// If we get here we are continuing through the wizard page changing process
			// and no collaboration is in effect.
			wxASSERT(pApp->m_bStartWorkUsingCollaboration == FALSE);

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

			// whm modified 28Aug11 to use a new CreateAndLoadKBs() function since
			// we are dealing with an existing project.
			// The CreateAndLoadKBs() is called from here as well as from the the App's 
			// SetupDirectories(), the View's OnCreate(), and the CollabUtilities' 
			// HookUpToExistingAIProject().
			// 
			// If CreateAndLoadKBs() fails to create the necessary KBs, the code 
			// in CreateAndLoadKBs() issues error messages and the FALSE return block
			// of CreateAndLoadKBs() below closes the Start Working Wizard.
			// 
			if (!pApp->CreateAndLoadKBs())
			{
				// deal with failures here
				// whm Note: The user would probably have to close down the app
				// to do anything at this point, since no project is open (since
				// there are no KBs created or loaded upon failure of 
				// CreateAndLoadKBs(). 
				// close the start working wizard
				pStartWorkingWizard->Show(FALSE);
				pStartWorkingWizard->EndModal(1);
				pStartWorkingWizard = (CStartWorkingWizard*)NULL;
			}

			// whm 28Aug11 Note: The following code if-else block was within the
			// KB loading code that existed before using the CreateAndLoadKBs() 
			// function here in OnWizardPageChanging(). I am putting it here since
			// it would execute on a successful load of the KB in the old code.
			// 
			// TODO: Determine if the "reminder" should always be issued whenever 
			// the App's m_bAutoBackupKB is FALSE in all locations where 
			// CreateAndLoadKBs() is called. If so, it could go within CreateAndLoadKBs()
			// as long as it is appropriate to issue such a reminder in all places where 
			// CreateAndLoadKBs() is called.
			// The CreateAndLoadKBs() is called from here as well as from the the App's 
			// SetupDirectories(), the View's OnCreate(), and the CollabUtilities' 
			// HookUpToExistingAIProject().
			// do the KB backing up, if the user wants it done; inform the user if it is
			// currently turned off
			if (pApp->m_bAutoBackupKB)
			{
				// whm 15Jan11 commented out this DoKBBackup() call. I don't think it should be called
				// when a project is first opened when no changes have been made.
				;
				// pApp->DoKBBackup(); // use the bSuppressOKMessage = TRUE option
			}
			else
			{
				// IDS_KB_BACKUP_OFF
				if (!pApp->m_bUseCustomWorkFolderPath)
				{
					wxMessageBox(
_("A reminder: backing up of the knowledge base is currently turned off.\nTo turn it on again, see the Knowledge Base tab within the Preferences dialog."),
					_T(""), wxICON_INFORMATION);
				}
			}
			
			// The pDocPage's InitDialog need to be called here just before going to it
			// make sure the pDocPage is initialized to show the documents for the selected project
			wxInitDialogEvent idevent;
			pDocPage->InitDialog(idevent);

		}
        // whm added 12Jun11. Ensure the inputs and outputs directories are created.
        // SetupDirectories() normally takes care of this for a new project, but we also
        // want existing projects created before version 6 to have these directories too.
		wxString pathCreationErrors = _T("");
		// BEW 1Aug11, added test for m_curProjectPath not empty. The calls fails
		// otherwise in the following scenario:
		// Launch in Paratext or BE collaboration mode, Cancel out of the collaboration
		// dialog, go to Administrator menu and turn off Paratext (or BE) collaboration -
		// the wizard then shows the Projects list, select <New Project> and then the app
		// will crash when control here calls CreateInputsAndOutputsDirectories() because
		// at this point m_curProjectPath is empty
		if (!pApp->m_curProjectPath.IsEmpty())
		{
			pApp->CreateInputsAndOutputsDirectories(pApp->m_curProjectPath, pathCreationErrors);
			// ignore dealing with any unlikely pathCreationErrors at this point
		}
	}
}
