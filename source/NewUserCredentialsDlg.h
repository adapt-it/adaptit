#pragma once
/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			NewUserCredentials.h
/// \author			Bruce Waters
/// \date_created	30 December 2021
/// \rcs_id $Id: NewUserCredentials.h  2021-12-30 00:44:00Z bruce_waters@sil.org $
/// \copyright		2021 Bruce Waters, Bill Martin, Erik Brommers, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the NewUserCredentials class.
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

#ifndef NewUserCredentialsDlg_h
#define NewUserCredentialsDlg_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "NewUserCredentialsDlg.h"
#endif

//#include "Adapt_It.h"

class NewUserCredentialsDlg : public AIModalDialog
{
public:
	NewUserCredentialsDlg(wxWindow* parent);

	virtual ~NewUserCredentialsDlg(void); // destructor

	wxString strNewUser;
	wxString strNewFullname;
	wxString strNewPassword;
	wxCheckBox* m_pCheck_GrantPermission; // for useradmin 1 or 0
	wxCheckBox* m_pCheck_Grant_Permissions; // for MariaDB user, whether (1) or not (0) to grant access permissions
protected:
	void InitDialog(wxInitDialogEvent& WXUNUSED(event));
	void OnOK(wxCommandEvent& event);
	void OnCancel(wxCommandEvent& event);
public:
	wxTextCtrl* m_pNewUsernameCtrl;
	wxTextCtrl* m_pNewFullnameCtrl;
	wxTextCtrl* m_pNewPasswordCtrl;
protected:
	DECLARE_EVENT_TABLE()
};


#endif  /* NewUserCredentialsDlg_h */