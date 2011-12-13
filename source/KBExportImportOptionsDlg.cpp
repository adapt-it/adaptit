/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			KBExportImportOptionsDlg.cpp
/// \author			Bill Martin
/// \date_created	11 December 2011
/// \date_revised	11 December 2011
/// \copyright		2011 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the CKBExportImportOptionsDlg class. 
/// The CKBExportImportOptionsDlg class provides a dialog in which the user can select options for KB exports or imports
/// \derivation		The CKBExportImportOptionsDlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////
// Pending Implementation Items in KBExportImportOptionsDlg.cpp (in order of importance): (search for "TODO")
// 1. 
//
// Unanswered questions: (search for "???")
// 1. 
// 
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "KBExportImportOptionsDlg.h"
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
#include "KBExportImportOptionsDlg.h"

/// This global is defined in Adapt_It.cpp.
extern CAdapt_ItApp* gpApp; // if we want to access it fast

// event handler table
BEGIN_EVENT_TABLE(CKBExportImportOptionsDlg, AIModalDialog)
	EVT_INIT_DIALOG(CKBExportImportOptionsDlg::InitDialog)
	EVT_BUTTON(wxID_OK, CKBExportImportOptionsDlg::OnOK)
	// Samples:
	//EVT_MENU(ID_SOME_MENU_ITEM, CKBExportImportOptionsDlg::OnDoSomething)
	//EVT_UPDATE_UI(ID_SOME_MENU_ITEM, CKBExportImportOptionsDlg::OnUpdateDoSomething)
	//EVT_BUTTON(ID_SOME_BUTTON, CKBExportImportOptionsDlg::OnDoSomething)
	//EVT_CHECKBOX(ID_SOME_CHECKBOX, CKBExportImportOptionsDlg::OnDoSomething)
	//EVT_RADIOBUTTON(ID_SOME_RADIOBUTTON, CKBExportImportOptionsDlg::DoSomething)
	//EVT_LISTBOX(ID_SOME_LISTBOX, CKBExportImportOptionsDlg::DoSomething)
	//EVT_COMBOBOX(ID_SOME_COMBOBOX, CKBExportImportOptionsDlg::DoSomething)
	//EVT_TEXT(IDC_SOME_EDIT_CTRL, CKBExportImportOptionsDlg::OnEnChangeEditSomething)
	// ... other menu, button or control events
END_EVENT_TABLE()

CKBExportImportOptionsDlg::CKBExportImportOptionsDlg(wxWindow* parent) // dialog constructor
	: AIModalDialog(parent, -1, wxEmptyString,
				wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	pKBExportImportOptionsDlgSizer = KBExportImportOptionsFunc(this, FALSE, TRUE); // second param FALSE enables resize
	// The declaration is: NameFromwxDesignerDlgFunc( wxWindow *parent, bool call_fit, bool set_sizer );
	
	// use wxValidator for simple dialog data transfer
	// sample text control initialization below:
	//wxTextCtrl* pEdit;
	//pEdit = (wxTextCtrl*)FindWindowById(IDC_TEXTCONTROL);
	//pEdit->SetValidator(wxGenericValidator(&m_stringVariable));
	//pEdit->SetBackgroundColour(sysColorBtnFace);

	pRadioBoxSfmOrLIFT = (wxRadioBox*)FindWindowById(ID_RADIOBOX_KB_EXPORT_IMPORT_OPTIONS);
	wxASSERT(pRadioBoxSfmOrLIFT != NULL);

	pCheckUseSuffixExportDateTimeStamp = (wxCheckBox*)FindWindowById(ID_CHECKBOX_SUFFIX_EXPORT_DATETIME_STAMP);
	wxASSERT(pCheckUseSuffixExportDateTimeStamp != NULL);
	
	// other attribute initializations
}

CKBExportImportOptionsDlg::~CKBExportImportOptionsDlg() // destructor
{
	
}

void CKBExportImportOptionsDlg::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{
	// dialog's title substitution for %s is done in callers

	//InitDialog() is not virtual, no call needed to a base class
	pRadioBoxSfmOrLIFT->SetSelection(0); // selection 0 is sfm format

	// initialize the values of the checkbox from the App's value
	pCheckUseSuffixExportDateTimeStamp->SetValue(gpApp->m_bUseSuffixExportDateTimeOnFilename);
}

// event handling functions

//CKBExportImportOptionsDlg::OnDoSomething(wxCommandEvent& event)
//{
//	// handle the event
	
//}

//CKBExportImportOptionsDlg::OnUpdateDoSomething(wxUpdateUIEvent& event)
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
void CKBExportImportOptionsDlg::OnOK(wxCommandEvent& event) 
{
	// save the value to the flag on the App for saving in the basic config file
	gpApp->m_bUseSuffixExportDateTimeOnFilename = pCheckUseSuffixExportDateTimeStamp->GetValue();
	
	event.Skip(); //EndModal(wxID_OK); //AIModalDialog::OnOK(event); // not virtual in wxDialog
}


// other class methods

