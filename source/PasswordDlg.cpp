/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			PasswordDlg.h
/// \author			Bruce Waters
/// \date_created	11 February 2016
/// \rcs_id $Id: PasswordDlg.h 3028 2016-01-25 11:38:00Z bruce_waters@sil.org $
/// \copyright		2013 Bruce Waters, Bill Martin, Erik Brommers, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the PasswordDlg class.
/// The PasswordDlg class provides a relocatable dialog for letting the user 
/// type in a password. This dialog is used in the KB Sharing module.
/// \derivation		The PasswordDlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "KbSvrHowGetUrl.h"
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

#if defined(_KBSERVER)

// other includes
#include "Adapt_It.h"
#include "MainFrm.h"
#include "Adapt_ItView.h"
#include "PasswordDlg.h"

// event handler table
BEGIN_EVENT_TABLE(PasswordDlg, AIModalDialog)
	EVT_INIT_DIALOG(PasswordDlg::InitDialog)
	EVT_BUTTON(wxID_OK, PasswordDlg::OnOK)
	EVT_BUTTON(wxID_CANCEL, PasswordDlg::OnCancel)
END_EVENT_TABLE()

PasswordDlg::PasswordDlg(wxWindow* parent) // dialog constructor
	: AIModalDialog(parent, -1, _("Type the server's password"),
			wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{

    // This dialog function below is generated in wxDesigner, and defines the controls and
    // sizers for the dialog. The first parameter is the parent which is the pointer this.
    // (Warning, do not use the frame window as the parent - the dialog will show empty and
    // the frame window will resize down to the dialog's size!) The second and third
    // parameters should both be TRUE to utilize the sizers and create the right size
    // dialog.
	PasswordDlgFunc(this, TRUE, TRUE);
	// The declaration is: functionname( wxWindow *parent, bool call_fit, bool set_sizer );
	bool bOK;
	bOK = m_pApp->ReverseOkCancelButtonsForMac(this);
	wxUnusedVar(bOK);
	m_pApp = &wxGetApp();
 }

PasswordDlg::~PasswordDlg() // destructor
{
}

void PasswordDlg::InitDialog(wxInitDialogEvent& WXUNUSED(event))
{
	m_pTextPasswordCtrl = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_PASSWORD);
	wxUnusedVar(m_pTextPasswordCtrl);
	m_pTextPasswordCtrl->Clear();
	m_bUserClickedCancel = FALSE;
	m_pApp->GetView()->PositionDlgNearTop(this);
}

void PasswordDlg::OnOK(wxCommandEvent& myevent)
{
	m_password = m_pTextPasswordCtrl->GetValue();
	m_bUserClickedCancel = FALSE;
	myevent.Skip(); // close the PasswordDdlg dialog
}

void PasswordDlg::OnCancel(wxCommandEvent& myevent)
{
	m_bUserClickedCancel = TRUE;
	m_password = wxEmptyString;
	myevent.Skip();  // close dialog
}

#endif