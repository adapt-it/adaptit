/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			NewUserCredentials.cpp
/// \author			Bruce Waters
/// \date_created	30 December 2021
/// \rcs_id $Id: NewUserCredentials.cpp  2021-12-30 00:44:00Z bruce_waters@sil.org $
/// \copyright		2021 Bruce Waters, Bill Martin, Erik Brommers, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implemention file for the NewUserCredentials class.
/// The NewUserCredentials class provides dialog for typing in the credentials for
/// adding a new user to the MariaDB kbserver user table. Four strings and one boolean 
/// choice are required. The new username, his/her fullname (can be a pseudo name),
/// a password for the new username, and a setting for kbadmin value - which in MariaDB 
/// kbserver user table is either 1 (meaning the new user is granted permission to add
/// further new users - either from a menu choice, or from the KB Sharing Manager's 
/// user page), or 0, which means the new user is denied permission to add other users
/// (but can still use the kbserver service for adding, editing, or pseudo-deleting
/// entries to the kbserver entry table).
// \derivation		NewUserCredentials class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "NewUserCredentialsDlg.h"
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

//#include <wx/docview.h> // needed for classes that reference wxView or wxDocument

#include "Adapt_It.h"
#include "NewUserCredentialsDlg.h"

// event handler table
BEGIN_EVENT_TABLE(NewUserCredentialsDlg, AIModalDialog)
EVT_INIT_DIALOG(NewUserCredentialsDlg::InitDialog)
EVT_BUTTON(wxID_OK, NewUserCredentialsDlg::OnOK)
EVT_BUTTON(wxID_CANCEL, NewUserCredentialsDlg::OnCancel)
END_EVENT_TABLE()

NewUserCredentialsDlg::NewUserCredentialsDlg(wxWindow* parent) // dialog constructor
	: AIModalDialog(parent, -1, _("Type New User Details"),
		wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	NewUserCredentialsFunc(this, TRUE, TRUE);
	// The declaration is: functionname( wxWindow *parent, bool call_fit, bool set_sizer );

//#if defined (_DEBUG)
//	int halt_here = 1;
//#endif
}

NewUserCredentialsDlg::~NewUserCredentialsDlg() // destructor
{
}

void NewUserCredentialsDlg::InitDialog(wxInitDialogEvent& WXUNUSED(event))
{
	// BEW 30Dec21 we need to use Bill's little class for AutoCorrect support... 
	// copied from user1_user2_lookup_func -- see next two comment lines
	// whm 31Aug2021 modified line below to use the AutoCorrectTextCtrl class which is now
	// used as a custom control in wxDesigner's user1_user2_lookup_func() dialog.
	m_pNewUsernameCtrl = (AutoCorrectTextCtrl*)FindWindowById(ID_TEXTCTRL_NEW_USERNM);
	m_pNewFullnameCtrl = (AutoCorrectTextCtrl*)FindWindowById(ID_TEXTCTRL_NEW_FULLNAME);
	m_pNewPasswordCtrl = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_NEW_USERS_PWD);
	m_pCheck_GrantPermission = (wxCheckBox*)FindWindowById(ID_CHECKBOX_GRANT_PERMISSION);
	m_pCheck_GrantPermission->SetValue(FALSE); // start off unticked
	wxString empty = wxEmptyString;
	m_pNewUsernameCtrl->ChangeValue(empty);
	m_pNewFullnameCtrl->ChangeValue(empty);
	m_pNewPasswordCtrl->ChangeValue(empty);
}

void NewUserCredentialsDlg::OnCancel(wxCommandEvent& event)
{
	wxString empty = wxEmptyString;
	m_pNewUsernameCtrl->ChangeValue(empty);
	m_pNewFullnameCtrl->ChangeValue(empty);
	m_pNewPasswordCtrl->ChangeValue(empty);
	event.Skip();
}

void NewUserCredentialsDlg::OnOK(wxCommandEvent& event)
{
	wxUnusedVar(event);
	strNewUser = m_pNewUsernameCtrl->GetValue();
	strNewFullname = m_pNewFullnameCtrl->GetValue();
	strNewPassword = m_pNewPasswordCtrl->GetValue();
	if (strNewUser.IsEmpty() ||
		strNewFullname.IsEmpty() ||
		strNewPassword.IsEmpty()
		)
	{
		wxString caption = _("A value is missing");
		wxString msg = _("Each text control must have a non-empty value. One or more are empty. Fix this by typing appropriate values into the empty boxes.");
		wxMessageBox(msg, caption, wxICON_INFORMATION | wxOK);
		return; // an immediate return keeps the dialog open, for the user to
				// rectify the missing field contents string
	}
	event.Skip();
}