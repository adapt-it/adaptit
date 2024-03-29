/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			Welcome.cpp
/// \author			Bill Martin
/// \date_created	28 April 2004
/// \rcs_id $Id$
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the CWelcome class. 
/// The CWelcome class provides an opening splash screen to welcome a new user.
/// The screen has a checkbox to allow the user to turn it off for subsequent
/// runs of the application.
/// \derivation		The CWelcome class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////
// Pending Implementation Items in Welcome.cpp (in order of importance): (search for "TODO")
// 1. 
//
// Unanswered questions: (search for "???")
// 1. 
// 
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "Welcome.h"
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
#include "Welcome.h"
#include "Adapt_ItView.h"

// event handler table
BEGIN_EVENT_TABLE(CWelcome, AIModalDialog)
	EVT_INIT_DIALOG(CWelcome::InitDialog)
	EVT_CHECKBOX(IDC_CHECK_NOLONGER_SHOW, CWelcome::OnCheckNolongerShow)
END_EVENT_TABLE()


CWelcome::CWelcome(wxWindow* parent) // dialog constructor
	: AIModalDialog(parent, -1, _T(""),
				wxDefaultPosition, wxDefaultSize, wxCAPTION | wxRESIZE_BORDER)
{
	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// whm 24Feb2022 added FALSE to second parameter below.
	// Some of the dialog's controls and label text are likely going to be truncated on the Mac unless we 
	// resize the dialog to fit it. Note: The constructor's call of WelcomeDlgFunc(this, FALSE, TRUE);
	// has its second parameter as FALSE to allow this resize.
	pWelcomeDlgSizer = WelcomeDlgFunc(this, FALSE, TRUE);
	// The declaration is: NameFromwxDesignerDlgFunc( wxWindow *parent, bool call_fit, bool set_sizer );

    // whm 5Mar2019 Note: The WelcomeDlgFunc() created by wxDesigner has a checkbox in the same
    // horizontal sizer along with an OK button. We do not need to call the ReverseOkCancelButtonsForMac()
    // function in this case.
	
	CAdapt_ItApp* pApp = (CAdapt_ItApp*)&wxGetApp();
	wxASSERT(pApp);

#ifdef _UNICODE
	this->SetTitle(_("Adapt It UNICODE version"));
#else
	this->SetTitle(_("Adapt It REGULAR (non-Unicode) version"));
#endif

	// initially check/uncheck box according to global pApp->m_bSuppressWelcome
	pCheckB = (wxCheckBox*)FindWindowById(IDC_CHECK_NOLONGER_SHOW);
	wxASSERT(pCheckB != NULL);
	// use generic validator to transfer between control and local var
	//pCheckB->SetValidator(wxGenericValidator(&m_bSuppressWelcome));

	pTextCtrlAsStaticWelcome = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_AS_STATIC_WELCOME);
	wxASSERT(pTextCtrlAsStaticWelcome != NULL);
	pTextCtrlAsStaticWelcome->SetBackgroundColour(pApp->sysColorBtnFace); //(wxSYS_COLOUR_WINDOW);

	// Set focus to the OK button
	pOKBtn = (wxButton*)FindWindow(wxID_OK); // use FinWindow here to find child window only
	wxASSERT(pOKBtn != NULL);
	pOKBtn->SetFocus();

	// other attribute initializations
}

CWelcome::~CWelcome() // destructor
{
	
}

void CWelcome::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{
	//InitDialog() is not virtual, no call needed to a base class
	
	CAdapt_ItApp* pApp = (CAdapt_ItApp*)&wxGetApp();
	wxASSERT(pApp);
	pCheckB->SetValue(pApp->m_bSuppressWelcome);

	// On some Linux distros the Welcome dialog doesn't size properly so ask for resize here.
	pWelcomeDlgSizer->SetSizeHints(this);
	pWelcomeDlgSizer->Layout();
	// Some of the dialog's controls and label text is likely going to be truncated on the Mac unless we 
	// resize the dialog to fit it. Note: The constructor's call of AboutDlgFunc(this, FALSE, TRUE);
	// has its second parameter as FALSE to allow this resize here in InitDialog().
	wxSize dlgSize;
	dlgSize = pWelcomeDlgSizer->ComputeFittingWindowSize(this);
	this->SetSize(dlgSize);
	this->CenterOnParent();
}

// event handling functions

void CWelcome::OnCheckNolongerShow(wxCommandEvent& WXUNUSED(event))
{
	m_bSuppressWelcome = pCheckB->GetValue();
}

// OnOK() calls wxWindow::Validate, then wxWindow::TransferDataFromWindow.
// If this returns TRUE, the function either calls EndModal(wxID_OK) if the
// dialog is modal, or sets the return value to wxID_OK and calls Show(FALSE)
// if the dialog is modeless.
void CWelcome::OnOK(wxCommandEvent& event) 
{
	// whm 13Jan12 Note: The View gets the m_bSuppressWelcome value directly (which is
	// set above in the OnCheckNolongerShow() handler above, therefore we technically
	// do not need to assign the m_bSuppressWelcom value from the control window, but
	// it won't hurt to do so
	m_bSuppressWelcome = pCheckB->GetValue();
	event.Skip(); //EndModal(wxID_OK); //AIModalDialog::OnOK(event); // not virtual in wxDialog
}


