/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			TransformToGlossesDlg.cpp
/// \author			Bill Martin
/// \date_created	29 April 2004
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the CTransformToGlossesDlg class. 
/// The CTransformToGlossesDlg class is a dialog with a "Yes" and "No" buttons to verify
/// from the user that it is Ok to discard any adaptations from the current document in
/// order to create a glossing KB from the former adaptations KB. A "Yes" would then call
/// up the OpenExistingProjectDlg.
/// \derivation		The CTransformToGlossesDlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////
// Pending Implementation Items in TransformToGlossesDlg.cpp (in order of importance): (search for "TODO")
// 1. 
//
// Unanswered questions: (search for "???")
// 1. 
// 
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "TransformToGlossesDlg.h"
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
#include "Adapt_It.h"
#include "TransformToGlossesDlg.h"

/// This global is defined in Adapt_It.cpp.
extern CAdapt_ItApp* gpApp; // if we want to access it fast

// event handler table
BEGIN_EVENT_TABLE(CTransformToGlossesDlg, AIModalDialog)
	EVT_INIT_DIALOG(CTransformToGlossesDlg::InitDialog)
END_EVENT_TABLE()


CTransformToGlossesDlg::CTransformToGlossesDlg(wxWindow* parent) // dialog constructor
	: AIModalDialog(parent, -1, _("Transform Another Project's Adaptations Into The Current Project's Glosses (Using Same Source Text)"),
				wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	pTransformToGlossesDlgSizer = TransformToGlossesDlgFunc(this, TRUE, TRUE);
	// The declaration is: TransformToGlossesDlgFunc( wxWindow *parent, bool call_fit, bool set_sizer );
	
	bool bOK;
	bOK = gpApp->ReverseOkCancelButtonsForMac(this);

	// dialog needs size hints, otherwise it is too tall and too narrow. The following sets the
	// initial height of whole dialog (incl border and title) to 340 pixels heigh (the max height)
	// and 400 pixels wide (the minimum width). There should be plenty of room for localized text
	// to expand the three text controls used in the dialog for static text instructions.
	//this->SetSizeHints(wxSize(400,240),wxSize(600,340));


	wxColor backgrndColor = this->GetBackgroundColour();
	pTextCtrlStatic1 = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_TRANSFORM_TO_GLOSSES1);
	wxASSERT(pTextCtrlStatic1 != NULL);
	pTextCtrlStatic1->SetBackgroundColour(backgrndColor);

	pTextCtrlStatic2 = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_TRANSFORM_TO_GLOSSES2);
	wxASSERT(pTextCtrlStatic2 != NULL);
	pTextCtrlStatic2->SetBackgroundColour(backgrndColor);

	pTextCtrlStatic3 = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_TRANSFORM_TO_GLOSSES3);
	wxASSERT(pTextCtrlStatic3 != NULL);
	pTextCtrlStatic3->SetBackgroundColour(backgrndColor);

}

CTransformToGlossesDlg::~CTransformToGlossesDlg() // destructor
{
	
}

void CTransformToGlossesDlg::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{
	//InitDialog() is not virtual, no call needed to a base class
	pTransformToGlossesDlgSizer->Layout();
}

// OnOK() calls wxWindow::Validate, then wxWindow::TransferDataFromWindow.
// If this returns TRUE, the function either calls EndModal(wxID_OK) if the
// dialog is modal, or sets the return value to wxID_OK and calls Show(FALSE)
// if the dialog is modeless.
void CTransformToGlossesDlg::OnOK(wxCommandEvent& event) 
{
	event.Skip(); //EndModal(wxID_OK); //AIModalDialog::OnOK(event); // not virtual in wxDialog
}

