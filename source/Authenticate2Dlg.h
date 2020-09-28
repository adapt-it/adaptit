#pragma once
/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			Authenticate2Dlg.h
/// \author			Bruce Waters
/// \date_created	8 Sepember 2020
/// \rcs_id $Id: Authenticate2Dlg.h 3128 2020-09-08 15:40:00Z bruce_waters@sil.org $
/// \copyright		2020 Bruce Waters, Bill Martin, Erik Brommers, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the CAuthenticate2Dlg class.
/// The Authenticate2Dlg class provides an authentication dialog for KB sharing, for
/// just one function, the LookupUser() function, which takes a first username for 
/// authentication purposes, and a second username (same or different) which is to be
/// looked up. If the same name is used for both usernames, it becomes a sufficent 
/// authentication call for setting m_strUserID up as the owner of the kbserver KB
/// in use for that username. If different, it's just a lookup for the credentials for
/// any non-work user in the user table.
// \derivation		The Authenticate2Dlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

#ifndef Authenticate2Dlg_h
#define Authenticate2Dlg_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "Authenticate2Dlg.h"
#endif

#if defined(_KBSERVER)

class Authenticate2Dlg : public AIModalDialog
{
public:
	Authenticate2Dlg(wxWindow* parent, bool bUserAuthenticating);

	virtual ~Authenticate2Dlg(void); // destructor
	bool m_bForManager;
	bool m_bError;
	wxString m_strForManagerUsername;
	wxString m_strForManagerIpAddr;
	wxString m_strForManagerUsername2;
	wxString m_strForManagerPassword;
	// parallel set for 'normal' authentications
	wxString m_strNormalUsername;
	wxString m_strNormalUsername2;
	wxString m_strNormalIpAddr;
	wxString m_strNormalPassword;

	//KbServer* m_pForManagerKbServer;
	//bool m_bUserIsAuthenticating; 
	wxTextCtrl* m_pMessageAtTop;
	wxSizer* m_pSizer;
	wxString obfuscatedPassword;
	bool bUsrAuthenticate;

	// other methods

	// Next few are needed for restoration purposes
	// First, the 'for manager' set of 3
	wxString m_saveOldIpAddrStr;
	wxString m_saveOldUsernameStr;
	wxString m_saveOldUsername2Str;
	wxString m_savePassword; // so it can be restored if the user Cancels
							 // Next 3 are for the 'normal' scenario
	wxString m_saveOldNormalIpAddrStr;
	wxString m_saveOldNormalUsernameStr;
	wxString m_saveNormalPassword; // so it can be restored if the user Cancels

protected:
	void InitDialog(wxInitDialogEvent& WXUNUSED(event));
	void OnOK(wxCommandEvent& event);
	void OnCancel(wxCommandEvent& event);

	CAdapt_ItApp* m_pApp;

	wxTextCtrl*	  m_pIpAddrCtrl;
	wxTextCtrl*	  m_pUsernameCtrl;
	wxTextCtrl*	  m_pPasswordCtrl;
	wxTextCtrl*   m_pUsername2Ctrl;
	wxStaticText* m_pUsernameLabel;
	wxStaticText* m_pUsernameMsgLabel;
	DECLARE_EVENT_TABLE()
};

#endif

#endif /* Authenticate2_h */
