/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			UnitsPage.cpp
/// \author			Bill Martin
/// \date_created	18 August 2004
/// \rcs_id $Id$
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the CUnitsPage class. 
/// The CUnitsPage class creates a wxPanel that allows the 
/// user to define the various document and knowledge base saving parameters. 
/// The panel becomes a "Auto-Saving" tab of the EditPreferencesDlg.
/// The interface resources are loaded by means of the UnitsPageFunc()
/// function which was developed and is maintained by wxDesigner.
/// \derivation		The CUnitsPage class is derived from wxPanel.
/////////////////////////////////////////////////////////////////////////////
// Pending Implementation Items in UnitsPage.cpp (in order of importance): (search for "TODO")
// 1. 
//
// Unanswered questions: (search for "???")
// 1. 
// 
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "UnitsPage.h"
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
#include "UnitsPage.h"

#include "Adapt_It.h"

IMPLEMENT_DYNAMIC_CLASS( CUnitsPage, wxPanel )

// event handler table
BEGIN_EVENT_TABLE(CUnitsPage, wxPanel)
	EVT_INIT_DIALOG(CUnitsPage::InitDialog)
	EVT_RADIOBUTTON(IDC_RADIO_INCHES, CUnitsPage::OnRadioUseInches)
	EVT_RADIOBUTTON(IDC_RADIO_CM, CUnitsPage::OnRadioUseCentimeters)
END_EVENT_TABLE()

CUnitsPage::CUnitsPage()
{
}

CUnitsPage::CUnitsPage(wxWindow* parent) // dialog constructor
{
	Create( parent );

	tempUseInches = FALSE; // in wx version page setup only has mm for margins, so we only use Metric for now
	m_pRadioUseInches = (wxRadioButton*)FindWindowById(IDC_RADIO_INCHES);
	m_pRadioUseCentimeters = (wxRadioButton*)FindWindowById(IDC_RADIO_CM);
}

CUnitsPage::~CUnitsPage() // destructor
{
}

bool CUnitsPage::Create( wxWindow* parent)
{
	wxPanel::Create( parent );
	CreateControls();
	GetSizer()->Fit(this);
	return TRUE;
}

void CUnitsPage::CreateControls()
{
	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	pUnitsPageSizer = UnitsPageFunc(this, TRUE, TRUE);
}

void CUnitsPage::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{
	//InitDialog() is not virtual, no call needed to a base class
	CAdapt_ItApp* pApp = (CAdapt_ItApp*)&wxGetApp();
	wxASSERT(pApp != NULL);

	// initialize our local temp variables from those on the App
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

	// Since most users won't likely want any particular setting in this
	// panel, we won't set focus to any particular control.
}

void CUnitsPage::OnRadioUseInches(wxCommandEvent& WXUNUSED(event)) 
{	
#ifdef __WXMSW__
	wxString msg;
	msg = _("Sorry, only metric units (Centimeters) can be set on the Windows platform at this time.");
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

void CUnitsPage::OnRadioUseCentimeters(wxCommandEvent& WXUNUSED(event)) 
{
	m_pRadioUseInches->SetValue(FALSE);
	m_pRadioUseCentimeters->SetValue(TRUE);
	tempUseInches = FALSE;
}

// MFC's OnSetActive() has no direct equivalent in wxWidgets. 
// It would not be needed in any case since in our
// design InitDialog is moved to public and called once 
// in the App where the wizard pages are constructed before 
// wizard itself starts.

// UnitsPage not used in any wizards, only as tab in notebook dialogs
//void CUnitsPage::OnWizardPageChanging(wxWizardEvent& event)
//{
//}

void CUnitsPage::OnOK(wxCommandEvent& WXUNUSED(event))
{
	// Validation of the unitsPage data should be done in the caller's
	// OnOK() method before calling CUnitsPage::OnOK().

	// User pressed OK so assume user wants to store the dialog's values.
	// put the source & target language names in storage on the App
	CAdapt_ItApp* pApp = (CAdapt_ItApp*)&wxGetApp();

	// Update values on the App
	pApp->m_bIsInches = tempUseInches;
}



