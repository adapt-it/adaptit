/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			Welcome.cpp
/// \author			Bill Martin
/// \date_created	28 April 2004
/// \date_revised	15 January 2008
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
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	WelcomeDlgFunc(this, TRUE, TRUE);
	// The declaration is: NameFromwxDesignerDlgFunc( wxWindow *parent, bool call_fit, bool set_sizer );
	
	CAdapt_ItApp* pApp = (CAdapt_ItApp*)&wxGetApp();
	wxASSERT(pApp);

#ifdef _UNICODE
	this->SetTitle(_("Adapt It UNICODE version"));
#else
	this->SetTitle(_("Adapt It REGULAR (non-Unicode) version"));
#endif

	// initially check/uncheck box according to global pApp->m_bSuppressWelcome
	wxCheckBox* pCheckB;
	pCheckB = (wxCheckBox*)FindWindowById(IDC_CHECK_NOLONGER_SHOW);
	pCheckB->SetValue(pApp->m_bSuppressWelcome);
	// use generic validator to transfer between control and local var
	pCheckB->SetValidator(wxGenericValidator(&m_bSuppressWelcome));

	wxTextCtrl* pTextCtrlAsStaticWelcome = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_AS_STATIC_WELCOME);
	wxASSERT(pTextCtrlAsStaticWelcome != NULL);
	pTextCtrlAsStaticWelcome->SetBackgroundColour(pApp->sysColorBtnFace); //(wxSYS_COLOUR_WINDOW);

	// Set focus to the OK button
	wxButton* pOKBtn = (wxButton*)FindWindow(wxID_OK); // use FinWindow here to find child window only
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

	//Note: The MFC code set up the font characteristics including
	// size and color here in InitDialog. However, we are able to
	// do everything in the wxDesigner dialog resources.
	// Hence, CRedEdit() and CClolouEdit() are not needed in the wx version.
}

// event handling functions

void CWelcome::OnCheckNolongerShow(wxCommandEvent& WXUNUSED(event))
{
	m_bSuppressWelcome = m_bSuppressWelcome == TRUE ? FALSE : TRUE;
}

// OnOK() calls wxWindow::Validate, then wxWindow::TransferDataFromWindow.
// If this returns TRUE, the function either calls EndModal(wxID_OK) if the
// dialog is modal, or sets the return value to wxID_OK and calls Show(FALSE)
// if the dialog is modeless.
void CWelcome::OnOK(wxCommandEvent& event) 
{
	event.Skip(); //EndModal(wxID_OK); //AIModalDialog::OnOK(event); // not virtual in wxDialog
}


