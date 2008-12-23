/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			OpenExistingProjectDlg.cpp
/// \author			Bill Martin
/// \date_created	20 March 2004
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for COpenExistingProjectDlg class. 
/// The COpenExistingProjectDlg class works together with the 
/// OpenExistingProjectDlgFunc() dialog-defining function. Together they implement
/// Adapt It's "Access An Existing Adaptation Project" Dialog, which is called from
/// the OpenExistingAdaptionProject() and AccessOtherAdaptionProject() functions 
/// (which is in turn called from OnAdvancedTransformAdaptationsIntoGlosses) in the App.
/// \derivation		The COpenExistingProjectDlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////
// Pending Implementation Items in MainFrm (in order of importance): (search for "TODO")
// 1. 
//
// Unanswered questions: (search for "???")
// 1. Should we implement a wxGenericValidator to mediate between the 
//    controls and the data of this simple dialog??? For complicated
//    dialogs, the validator provides a simpler, more straight-forward way 
//    of doing what CDataExchange does via DDX/DDV in MFC.
// 
/////////////////////////////////////////////////////////////////////////////
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "OpenExistingProjectDlg.h"
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

#include <wx/valgen.h>
#include "Adapt_It.h"
#include "helpers.h"
#include "Adapt_ItDoc.h"
#include "OpenExistingProjectDlg.h"
 
extern fontInfo NavFInfo;

/// This global is defined in Adapt_It.cpp.
extern CAdapt_ItApp* gpApp; // if we want to access it fast

/// Use this Boolean to exclude the current project when the user invokes this dialog from within the
/// handler (see the app class) for the Advanced menu's "Transform Adaptations Into Glosses" command's
/// handler - because the transformation must never be allowed within its own project since it clobbers
/// the adaptations in the docs and empties the adaptations KB as it converts them to glosses, so the
/// handler should only be invoked from a new empty project. But in all other circumstances, the dialog
/// should show all existing projects, as default.
bool gbExcludeCurrentProject = FALSE; 

/////////////////////////////////////////////////////////////////////////////
// COpenExistingProjectDlg dialog

BEGIN_EVENT_TABLE(COpenExistingProjectDlg, AIModalDialog)
	EVT_INIT_DIALOG(COpenExistingProjectDlg::InitDialog)// not strictly necessary for dialogs based on wxDialog
	EVT_BUTTON(wxID_OK, COpenExistingProjectDlg::OnOK)
	EVT_LISTBOX(IDC_LISTBOX_ADAPTIONS, COpenExistingProjectDlg::OnSelchangeListboxAdaptions)
	EVT_LISTBOX_DCLICK(IDC_LISTBOX_ADAPTIONS, COpenExistingProjectDlg::OnDblclkListboxAdaptions)
END_EVENT_TABLE()


COpenExistingProjectDlg::COpenExistingProjectDlg(wxWindow* parent)
	: AIModalDialog(parent, -1, _("Access An Existing Adaption Project"),
				wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	OpenExistingProjectDlgFunc(this, TRUE, TRUE);
	// This dialog function is generated in wxDesigner, and defines the controls and sizers
	// for the dialog.
	// The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	// The declaration is: OpenExistingProjectDlgFunc( wxWindow *parent, bool call_fit, bool set_sizer );
	m_projectName.Empty();
}


/////////////////////////////////////////////////////////////////////////////
// Dialog initialization
void COpenExistingProjectDlg::InitDialog(wxInitDialogEvent& WXUNUSED(event)) //MFC had BOOL COpenExistingProjectDlg::OnInitDialog()
{
	//InitDialog() is not virtual, no call needed to the base class

	// use the navigation text's font for the list box
	CAdapt_ItApp* pApp = &wxGetApp();
	wxASSERT(pApp != NULL);
	wxListBox* pListBox;
	pListBox = (wxListBox*)FindWindowById(IDC_LISTBOX_ADAPTIONS);
	pListBox->Clear(); // whm added 18Apr04

	// make the font show user's desired point size in the dialog
	#ifdef _RTL_FLAGS
	gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pNavTextFont, NULL, NULL,
								pListBox, NULL, gpApp->m_pDlgSrcFont, gpApp->m_bNavTextRTL);
	#else // Regular version, only LTR scripts supported, so use default FALSE for last parameter
	gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pNavTextFont, NULL, NULL, 
								pListBox, NULL, gpApp->m_pDlgSrcFont);
	#endif

	// generate a CStringList of all the possible adaption projects
	wxArrayString possibleAdaptions;
	pApp->GetPossibleAdaptionProjects(&possibleAdaptions);

	wxString showItem;
	size_t ct = possibleAdaptions.GetCount();
	for (size_t index = 0; index < ct; index++)
	{
		showItem = possibleAdaptions.Item(index);
		pListBox->Append(showItem);
	}

	// this class could be called from the startup wizard, or starthere command, to open a
	// project - and if so, then gbExcludeCurrentProject flag should be FALSE, so that all
	// projects appear in the list. But if called from the Advanced menu's "Transform Adapatation
	// To Glosses" command, then we must exclude the current project from the list - for this
	// situation the handler for that command will have set gbExcludeCurrentProject to TRUE - so
	// test for this and route processing accordingly.
	if (gbExcludeCurrentProject)
	{
		// we need to exclude the current project
		int nReturned = pListBox->FindString(pApp->m_curProjectName);
		if (nReturned != -1)
		{
			// no error means nReturned has the index of the entry which has the name of
			// the current project, so remove this from the list; if there was an error then
			// the current project is not in the list, so just exit the block without doing anything
			pListBox->Delete(nReturned);
		}
		pListBox->SetSelection(0, TRUE); // use the first one as default, as we can't know which the user
										// will want to choose
	}
	else // normal situation
	{
		// hilight the folder name of the project obtained from the
		// configuration settings, if possible otherwise the first in the list
		if (pApp->m_curProjectName.IsEmpty())
			pListBox->SetSelection(0,TRUE);
		else
		{
			// must check if nReturned == LB_ERR, since, for example, if the user created
			// a project then manually deleted the project from the Adapt It Work folder 
			// externally to the application, then it would try find it and fail, and the 
			// app would crash
			if (pListBox->FindString(pApp->m_curProjectName) != -1)
				pListBox->SetStringSelection(pApp->m_curProjectName, TRUE);
			else
				pListBox->SetSelection(0,TRUE);
		}
	}
}
/////////////////////////////////////////////////////////////////////////////
// COpenExistingProjectDlg message handlers

// OnOK() calls wxWindow::Validate, then wxWindow::TransferDataFromWindow.
// If this returns TRUE, the function either calls EndModal(wxID_OK) if the
// dialog is modal, or sets the return value to wxID_OK and calls Show(FALSE)
// if the dialog is modeless.
void COpenExistingProjectDlg::OnOK(wxCommandEvent& event) 
{
	wxListBox* pListBox;
	pListBox = (wxListBox*)FindWindowById(IDC_LISTBOX_ADAPTIONS);
	wxASSERT(pListBox != NULL);
	if (!ListBoxPassesSanityCheck((wxControlWithItems*)pListBox))
	{
		wxMessageBox(_T("List box error when getting the current selection"), _T(""), wxICON_EXCLAMATION);
		wxASSERT(FALSE);
		return;
	}
	int nSel;
	nSel = pListBox->GetSelection();
	//if (nSel == -1) // MFC LB_ERR is #define -1
	//{
	//	wxMessageBox(_T("List box error when getting the current selection"), _T(""), wxICON_EXCLAMATION);
	//	wxASSERT(FALSE);
	//	//wxExit();//AfxAbort();
	//}
	m_projectName = pListBox->GetString(nSel);

	// set m_curProjectPath so we can load the Project configuration file, we need to
	// make sure that the project's setting for m_bBookMode and m_nBookIndex are restored
	// before the user clicks the Open command on the file menu
	gpApp->m_curProjectPath = gpApp->m_workFolderPath + gpApp->PathSeparator + m_projectName;
	gpApp->GetDocument()->GetProjectConfiguration(gpApp->m_curProjectPath);
	
	event.Skip(); //EndModal(wxID_OK); //wxDialog::OnOK(event); // not virtual in wxDialog
}

void COpenExistingProjectDlg::OnSelchangeListboxAdaptions(wxCommandEvent& WXUNUSED(event)) 
{
	wxListBox* pListBox;
	pListBox = (wxListBox*)FindWindowById(IDC_LISTBOX_ADAPTIONS);
	wxASSERT(pListBox != NULL);
	// wx note: Under Linux/GTK ...Selchanged... listbox events can be triggered after a call to Clear()
	// so we must check to see if the listbox contains no items and if so return immediately
	if (pListBox->GetCount() == 0)
		return;
	
	if (!ListBoxPassesSanityCheck((wxControlWithItems*)pListBox))
	{
		wxMessageBox(_T("List box error when getting the current selection"), _T(""), wxICON_EXCLAMATION);
		wxASSERT(FALSE);
		return;
	}

	int nSel;
	nSel = pListBox->GetSelection();
	//if (nSel == -1) //LB_ERR
	//{
	//	wxMessageBox(_T("List box error when getting the current selection"), _T(""), wxICON_EXCLAMATION);
	//	wxASSERT(FALSE);
	//	//wxExit();
	//}
	m_projectName = pListBox->GetString(nSel);
}

void COpenExistingProjectDlg::OnDblclkListboxAdaptions(wxCommandEvent& WXUNUSED(event)) 
{
	wxListBox* pListBox;
	pListBox = (wxListBox*)FindWindowById(IDC_LISTBOX_ADAPTIONS);
	wxASSERT(pListBox != NULL);
	if (!ListBoxPassesSanityCheck((wxControlWithItems*)pListBox))
	{
		wxMessageBox(_T("List box error when getting the current selection"), _T(""), wxICON_EXCLAMATION);
		wxASSERT(FALSE);
		return;
	}
	int nSel;
	nSel = pListBox->GetSelection();
	//if (nSel == -1) //LB_ERR
	//{
	//	wxMessageBox(_T("List box error when getting the current selection"), _T(""), wxICON_EXCLAMATION);
	//	wxASSERT(FALSE);
	//	wxExit();
	//}
	m_projectName = pListBox->GetString(nSel);

	// set m_curProjectPath so we can load the Project configuration file, we need to
	// make sure that the project's setting for m_bBookMode and m_nBookIndex are restored
	// before the user clicks the Open command on the file menu
	gpApp->m_curProjectPath = gpApp->m_workFolderPath + gpApp->PathSeparator + m_projectName;
	gpApp->GetDocument()->GetProjectConfiguration(gpApp->m_curProjectPath);
	wxCommandEvent okevent = wxID_OK;
	OnOK(okevent); //EndModal(wxID_OK);//EndDialog(IDOK); // TODO: check that this closes the dialog
}

