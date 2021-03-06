/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			UnitsDlg.cpp
/// \author			Bill Martin
/// \date_created	22 May 2004
/// \rcs_id $Id$
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
	EVT_INIT_DIALOG(CUnitsDlg::InitDialog)
	EVT_BUTTON(wxID_OK, CUnitsDlg::OnOK)
	EVT_RADIOBUTTON(IDC_RADIO_INCHES, CUnitsDlg::OnRadioUseInches)
	EVT_RADIOBUTTON(IDC_RADIO_CM, CUnitsDlg::OnRadioUseCentimeters)
END_EVENT_TABLE()


CUnitsDlg::CUnitsDlg(wxWindow* parent) // dialog constructor
	: AIModalDialog(parent, -1, _("Units To Be Used For Printing"),
		wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	// The dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	UnitsDlgFunc(this, TRUE, TRUE);
	// The declaration is: UnitsDlgFunc( wxWindow *parent, bool call_fit, bool set_sizer );


    // whm 5Mar2019 Note: the UnitsDlgFunc() now uses the wxStdDialogButtonsSizer which
    // takes care of reversing buttons for the Mac, so we no longer call the App's 
    // ReverseOkCancelButtonsForMac() function here.
 //   bool bOK;
	//bOK = gpApp->ReverseOkCancelButtonsForMac(this);
	//bOK = bOK; // avoid warning
	tempUseInches = FALSE; // in wx version page setup only has mm for margins, so we only use Metric for now
	m_pRadioUseInches = (wxRadioButton*)FindWindowById(IDC_RADIO_INCHES);
	m_pRadioUseCentimeters = (wxRadioButton*)FindWindowById(IDC_RADIO_CM);

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

	tempUseInches = pApp->m_bIsInches;

	wxCommandEvent dummyevent;
	// initialize radio buttons
	if (tempUseInches)
	{
		OnRadioUseInches(dummyevent);
	}
	else
	{
		OnRadioUseCentimeters(dummyevent);
	}
}

void CUnitsDlg::OnRadioUseInches(wxCommandEvent& WXUNUSED(event))
{
#ifdef __WXMSW__
	wxString msg;
	msg = _("Sorry, only metric units (Centimeters) can be set on the Windows platform at this time.");
    // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
    gpApp->m_bUserDlgOrMessageRequested = TRUE;
    wxMessageBox(msg,_T(""),wxICON_INFORMATION | wxOK);
	m_pRadioUseInches->SetValue(FALSE);
	m_pRadioUseCentimeters->SetValue(TRUE);
	tempUseInches = FALSE;
#else
	m_pRadioUseInches->SetValue(TRUE);
	m_pRadioUseCentimeters->SetValue(FALSE);
	tempUseInches = TRUE;
#endif
}

void CUnitsDlg::OnRadioUseCentimeters(wxCommandEvent& WXUNUSED(event))
{
	m_pRadioUseInches->SetValue(FALSE);
	m_pRadioUseCentimeters->SetValue(TRUE);
	tempUseInches = FALSE;
}


// OnOK() calls wxWindow::Validate, then wxWindow::TransferDataFromWindow.
// If this returns TRUE, the function either calls EndModal(wxID_OK) if the
// dialog is modal, or sets the return value to wxID_OK and calls Show(FALSE)
// if the dialog is modeless.
void CUnitsDlg::OnOK(wxCommandEvent& event)
{
	CAdapt_ItApp* pApp = &wxGetApp();
	wxASSERT(pApp != NULL);

	// Update values on the App
	pApp->m_bIsInches = tempUseInches;

	event.Skip(); //EndModal(wxID_OK); //AIModalDialog::OnOK(event); // not virtual in wxDialog
}


// other class methods

