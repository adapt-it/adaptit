/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			ConflictResolutionDlg.cpp
/// \author			Bill Martin
/// \date_created	11 April 2011
/// \date_revised	11 April 2011
/// \copyright		2011 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the CConflictResolutionDlg class. 
/// The CConflictResolutionDlg class provides a dialog in which a user can resolve the conflicts between
/// different translations of a given portion (verse) of source text.
/// \derivation		The CConflictResolutionDlg class is derived from wxDialog.
/////////////////////////////////////////////////////////////////////////////
// Pending Implementation Items in ConflictResolutionDlg.cpp (in order of importance): (search for "TODO")
// 1. 
//
// Unanswered questions: (search for "???")
// 1. 
// 
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "ConflictResolutionDlg.h"
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
#include "ConflictResolutionDlg.h"

// event handler table
BEGIN_EVENT_TABLE(CConflictResolutionDlg, AIModalDialog)
	EVT_INIT_DIALOG(CConflictResolutionDlg::InitDialog)// not strictly necessary for dialogs based on wxDialog
	// Samples:
	//EVT_BUTTON(wxID_OK, CConflictResolutionDlg::OnOK)
	//EVT_MENU(ID_SOME_MENU_ITEM, CConflictResolutionDlg::OnDoSomething)
	//EVT_UPDATE_UI(ID_SOME_MENU_ITEM, CConflictResolutionDlg::OnUpdateDoSomething)
	//EVT_BUTTON(ID_SOME_BUTTON, CConflictResolutionDlg::OnDoSomething)
	//EVT_CHECKBOX(ID_SOME_CHECKBOX, CConflictResolutionDlg::OnDoSomething)
	//EVT_RADIOBUTTON(ID_SOME_RADIOBUTTON, CConflictResolutionDlg::DoSomething)
	//EVT_LISTBOX(ID_SOME_LISTBOX, CConflictResolutionDlg::DoSomething)
	//EVT_COMBOBOX(ID_SOME_COMBOBOX, CConflictResolutionDlg::DoSomething)
	//EVT_TEXT(IDC_SOME_EDIT_CTRL, CConflictResolutionDlg::OnEnChangeEditSomething)
	// ... other menu, button or control events
END_EVENT_TABLE()

CConflictResolutionDlg::CConflictResolutionDlg(wxWindow* parent) // dialog constructor
	: AIModalDialog(parent, -1, _("Source Text versions conflict"),
				wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	ConflictResolutionDlgFunc(this, TRUE, TRUE);
	// The declaration is: ConflictResolutionDlgFunc( wxWindow *parent, bool call_fit, bool set_sizer );
	
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

CConflictResolutionDlg::~CConflictResolutionDlg() // destructor
{
	
}

void CConflictResolutionDlg::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{
	//InitDialog() is not virtual, no call needed to a base class

}

// event handling functions

//CConflictResolutionDlg::OnDoSomething(wxCommandEvent& event)
//{
//	// handle the event
	
//}

//CConflictResolutionDlg::OnUpdateDoSomething(wxUpdateUIEvent& event)
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
void CConflictResolutionDlg::OnOK(wxCommandEvent& event) 
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

