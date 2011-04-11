/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			GetSourceTextFromEditorDlg.cpp
/// \author			Bill Martin
/// \date_created	10 April 2011
/// \date_revised	10 April 2011
/// \copyright		2011 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the CGetSourceTextFromEditorDlg class. 
/// The CGetSourceTextFromEditorDlg class represents a dialog in which a user can obtain a source text
/// for adaptation from an external editor such as Paratext or Bibledit. 
/// \derivation		The CGetSourceTextFromEditorDlg class is derived from wxDialog.
/////////////////////////////////////////////////////////////////////////////
// Pending Implementation Items in GetSourceTextFromEditorDlg.cpp (in order of importance): (search for "TODO")
// 1. 
//
// Unanswered questions: (search for "???")
// 1. 
// 
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "GetSourceTextFromEditorDlg.h"
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
#include "GetSourceTextFromEditor.h"

// event handler table
BEGIN_EVENT_TABLE(CGetSourceTextFromEditorDlg, AIModalDialog)
	EVT_INIT_DIALOG(CGetSourceTextFromEditorDlg::InitDialog)// not strictly necessary for dialogs based on wxDialog
	// Samples:
	//EVT_BUTTON(wxID_OK, CGetSourceTextFromEditorDlg::OnOK)
	//EVT_MENU(ID_SOME_MENU_ITEM, CGetSourceTextFromEditorDlg::OnDoSomething)
	//EVT_UPDATE_UI(ID_SOME_MENU_ITEM, CGetSourceTextFromEditorDlg::OnUpdateDoSomething)
	//EVT_BUTTON(ID_SOME_BUTTON, CGetSourceTextFromEditorDlg::OnDoSomething)
	//EVT_CHECKBOX(ID_SOME_CHECKBOX, CGetSourceTextFromEditorDlg::OnDoSomething)
	//EVT_RADIOBUTTON(ID_SOME_RADIOBUTTON, CGetSourceTextFromEditorDlg::DoSomething)
	//EVT_LISTBOX(ID_SOME_LISTBOX, CGetSourceTextFromEditorDlg::DoSomething)
	//EVT_COMBOBOX(ID_SOME_COMBOBOX, CGetSourceTextFromEditorDlg::DoSomething)
	//EVT_TEXT(IDC_SOME_EDIT_CTRL, CGetSourceTextFromEditorDlg::OnEnChangeEditSomething)
	// ... other menu, button or control events
END_EVENT_TABLE()

CGetSourceTextFromEditorDlg::CGetSourceTextFromEditorDlg(wxWindow* parent) // dialog constructor
	: AIModalDialog(parent, -1, _("Get Source Text from an External Editor"),
				wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	GetSourceTextFromEditorDlgFunc(this, TRUE, TRUE);
	// The declaration is: NameFromwxDesignerDlgFunc( wxWindow *parent, bool call_fit, bool set_sizer );
	
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

CGetSourceTextFromEditorDlg::~CGetSourceTextFromEditorDlg() // destructor
{
	
}

void CGetSourceTextFromEditorDlg::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{
	//InitDialog() is not virtual, no call needed to a base class

}

// event handling functions

//CGetSourceTextFromEditorDlg::OnDoSomething(wxCommandEvent& event)
//{
//	// handle the event
	
//}

//CGetSourceTextFromEditorDlg::OnUpdateDoSomething(wxUpdateUIEvent& event)
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
void CGetSourceTextFromEditorDlg::OnOK(wxCommandEvent& event) 
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

