/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			AdminEditMenuProfile.cpp
/// \author			Bill Martin
/// \date_created	20 August 2010
/// \date_revised	20 August 2010
/// \copyright		2010 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the CAdminEditMenuProfile class. 
/// The CAdminEditMenuProfile class allows a program administrator to 
/// simplify a user's interface by only making certain menu items and 
/// other settings available (visible) and other menu items unavailable 
/// (hidden) to the user. A tabbed dialog is created that has one tab for a 
/// "Novice" profile, one for a "Custom 1" profile, and one for a "Custom 2" 
/// profile. Each tab page contains a checklist of interface menu items and 
/// other settings preceded by check boxes. Each profile tab starts with a
/// subset of preselected items, to which the administrator can tweak to 
/// his liking, checking those menu items he wants to be visible in the 
/// interface and un-checking the menu items that are to be hidden. After 
/// adjusting the visibility of the desired menu items for a given profile, 
/// the administrator can select the profile to be used, and the program 
/// will continue to use that profile each time the application is run. 
/// The selection is saved in the basic and project config files, and the 
/// profile information is saved in an external xml control file. 
/// \derivation		The CAdminEditMenuProfile class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////
// Pending Implementation Items in AdminEditMenuProfile.cpp (in order of importance): (search for "TODO")
// 1. 
//
// Unanswered questions: (search for "???")
// 1. 
// 
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "AdminEditMenuProfile.h"
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
//#include <wx/valtext.h> // for wxTextValidator
#include "Adapt_It.h"
#include "AdminEditMenuProfile.h"

/// This global is defined in Adapt_ItView.cpp.
extern CAdapt_ItApp* gpApp;

// event handler table
BEGIN_EVENT_TABLE(CAdminEditMenuProfile, AIModalDialog)
	EVT_INIT_DIALOG(CAdminEditMenuProfile::InitDialog)// not strictly necessary for dialogs based on wxDialog
	// Samples:
	//EVT_BUTTON(wxID_OK, CAdminEditMenuProfile::OnOK)
	//EVT_MENU(ID_SOME_MENU_ITEM, CAdminEditMenuProfile::OnDoSomething)
	//EVT_UPDATE_UI(ID_SOME_MENU_ITEM, CAdminEditMenuProfile::OnUpdateDoSomething)
	//EVT_BUTTON(ID_SOME_BUTTON, CAdminEditMenuProfile::OnDoSomething)
	//EVT_CHECKBOX(ID_SOME_CHECKBOX, CAdminEditMenuProfile::OnDoSomething)
	//EVT_RADIOBUTTON(ID_SOME_RADIOBUTTON, CAdminEditMenuProfile::DoSomething)
	//EVT_LISTBOX(ID_SOME_LISTBOX, CAdminEditMenuProfile::DoSomething)
	//EVT_COMBOBOX(ID_SOME_COMBOBOX, CAdminEditMenuProfile::DoSomething)
	//EVT_TEXT(IDC_SOME_EDIT_CTRL, CAdminEditMenuProfile::OnEnChangeEditSomething)
	// ... other menu, button or control events
END_EVENT_TABLE()

CAdminEditMenuProfile::CAdminEditMenuProfile(wxWindow* parent) // dialog constructor
	: AIModalDialog(parent, -1, _("User Workflow Profiles"),
				wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	MenuEditorDlgFunc(this, TRUE, TRUE);
	// The declaration is: NameFromwxDesignerDlgFunc( wxWindow *parent, bool call_fit, bool set_sizer );
	
	m_pApp = (CAdapt_ItApp*)&wxGetApp();
	wxASSERT(m_pApp != NULL);
	
	bool bOK;
	bOK = m_pApp->ReverseOkCancelButtonsForMac(this);

	pNotebook = (wxNotebook*)FindWindowById(ID_MENU_EDITOR_NOTEBOOK);
	wxASSERT(pNotebook != NULL);
	pRadioBox = (wxRadioBox*)FindWindowById(ID_RADIOBOX);
	wxASSERT(pRadioBox != NULL);
	pCheckListBox = (wxCheckListBox*)FindWindowById(ID_CHECKLISTBOX_MENU_ITEMS);
	wxASSERT(pCheckListBox != NULL);

	// use wxValidator for simple dialog data transfer
	// sample text control initialization below:
	//wxTextCtrl* pEdit;
	//pEdit = (wxTextCtrl*)FindWindowById(IDC_TEXTCONTROL);
	//pEdit->SetValidator(wxGenericValidator(&m_stringVariable));
	//pEdit->SetBackgroundColour(sysColorBtnFace);

	// sample radio button control initialization below:
	//wxRadioButton* pRadioB;
	//pRadioB = (wxRadioButton*)FindWindowById(IDC_RADIO_BUTTON);
	//pRadioB->SetValue(TRUE);
	//pRadioB->SetValidator(wxGenericValidator(&m_bVariable));

	// other attribute initializations
}

CAdminEditMenuProfile::~CAdminEditMenuProfile() // destructor
{
	
}

void CAdminEditMenuProfile::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{
	//InitDialog() is not virtual, no call needed to a base class

	tempWorkflowProfile = gpApp->m_nWorkflowProfile;
	
	// Reread the AI_UserProfiles.xml file (this also loads the App's 
	// m_pUserProfiles data structure with latest values stored on disk).
	// Note: the AI_UserProfiles.xml file was read when the app first 
	// ran from OnInit() and the app's menus configured for those values.
	// We need to reread the AI_UserProfiles.xml each time the InitDialog()
	// is called because we change the interface dynamically and need to 
	// reload the data from the xml file in case the administrator previously
	// changed the profile during the current session (which automatically
	// saved any changes to AI_UserProfiles.xml).
	// TODO: Borrow code from App's OnInit().
	
	// Select whatever tab the administrator has set if any, first tab if none.
	int pageCount = pNotebook->GetPageCount();
	if (tempWorkflowProfile < 0 || tempWorkflowProfile > (int)pNotebook->GetPageCount() -1)
	{
		tempWorkflowProfile = 0;
		// TODO: warn user
	}
	pNotebook->ChangeSelection(tempWorkflowProfile); // ChangeSelection does not generate page changing events
	
}

// event handling functions

//CAdminEditMenuProfile::OnDoSomething(wxCommandEvent& event)
//{
//	// handle the event
	
//}

//CAdminEditMenuProfile::OnUpdateDoSomething(wxUpdateUIEvent& event)
//{
//	if (SomeCondition == TRUE)
//		event.Enable(TRUE);
//	else
//		event.Enable(FALSE);	
//}

// OnOK() calls wxWindow::Validate, then wxWindow::TransferDataFromWindow.
// If this returns TRUE, the function either calls EndModal(wxID_OK) if the
// dialog is modal, or sets the return value to wxID_OK and calls Show(FALSE)
// if the dialog is modeless.
void CAdminEditMenuProfile::OnOK(wxCommandEvent& event) 
{
	// sample code
	//wxListBox* pListBox;
	//pListBox = (wxListBox*)FindWindowById(IDC_LISTBOX_ADAPTIONS);
	//int nSel;
	//nSel = pListBox->GetSelection();
	//if (nSel == LB_ERR) // LB_ERR is #define -1
	//{
	//	wxMessageBox(_T("List box error when getting the current selection"), _T(""), wxICON_EXCLAMATION);
	//}
	//m_projectName = pListBox->GetString(nSel);
	
	event.Skip(); //EndModal(wxID_OK); //wxDialog::OnOK(event); // not virtual in wxDialog
}


// other class methods

