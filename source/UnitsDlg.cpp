/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			UnitsDlg.cpp
/// \author			Bill Martin
/// \date_created	22 May 2004
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the CUnitsDlg class. 
/// The CUnitsDlg class presents the user with a dialog with a choice between
/// Inches and Centimeters, for use primarily in page setup and printing.
/// \derivation		The CUnitsDlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////
// Pending Implementation Items in UnitsDlg.cpp (in order of importance): (search for "TODO")
// 1. 
//
// Unanswered questions: (search for "???")
// 1. 
// 
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "UnitsDlg.h"
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
#include "UnitsDlg.h"
#include "Adapt_ItView.h"

/// This global is defined in Adapt_It.cpp.
extern CAdapt_ItApp* gpApp; // if we want to access it fast

// event handler table
BEGIN_EVENT_TABLE(CUnitsDlg, AIModalDialog)
	EVT_INIT_DIALOG(CUnitsDlg::InitDialog)// not strictly necessary for dialogs based on wxDialog
	EVT_BUTTON(wxID_OK, CUnitsDlg::OnOK)
END_EVENT_TABLE()


CUnitsDlg::CUnitsDlg(wxWindow* parent) // dialog constructor
	: AIModalDialog(parent, -1, _("Units To Be Used For Printing"),
		wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	UnitsDlgFunc(this, TRUE, TRUE);
	// The declaration is: UnitsDlgFunc( wxWindow *parent, bool call_fit, bool set_sizer );
	
	bool bOK;
	bOK = gpApp->ReverseOkCancelButtonsForMac(this);
	
	// use wxValidator for simple dialog data transfer
	pRadioB = (wxRadioButton*)FindWindowById(IDC_RADIO_INCHES);
	pRadioB->SetValidator(wxGenericValidator(&m_bIsInches));

	// other attribute initializations
}

CUnitsDlg::~CUnitsDlg() // destructor
{
	
}

void CUnitsDlg::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{
	//InitDialog() is not virtual, no call needed to a base class
	CAdapt_ItApp* pApp = &wxGetApp();
	wxASSERT(pApp != NULL);
	wxRadioButton* pInchesBtn = (wxRadioButton*)FindWindowById(IDC_RADIO_INCHES);
	wxASSERT(pInchesBtn);
	wxRadioButton* pCmBtn = (wxRadioButton*)FindWindowById(IDC_RADIO_CM);
	wxASSERT(pCmBtn);

	m_bIsInches = pApp->m_bIsInches;

	if (m_bIsInches)
	{
		pInchesBtn->SetValue(TRUE);
		pCmBtn->SetValue(FALSE);
	}
	else
	{
		pInchesBtn->SetValue(FALSE);
		pCmBtn->SetValue(TRUE);
	}
	//pInchesBtn->Enable(FALSE); // for now we disable the inches button (until we find a was to use inches in wx version)
}

// OnOK() calls wxWindow::Validate, then wxWindow::TransferDataFromWindow.
// If this returns TRUE, the function either calls EndModal(wxID_OK) if the
// dialog is modal, or sets the return value to wxID_OK and calls Show(FALSE)
// if the dialog is modeless.
void CUnitsDlg::OnOK(wxCommandEvent& event) 
{
	wxRadioButton* pInchesBtn = (wxRadioButton*)FindWindowById(IDC_RADIO_INCHES);
	wxASSERT(pInchesBtn);

	m_bIsInches = pInchesBtn->GetValue(); // pApp->m_bIsInches is set in the View's OnUnits()
	
	event.Skip(); //EndModal(wxID_OK); //wxDialog::OnOK(event); // not virtual in wxDialog
}


// other class methods

