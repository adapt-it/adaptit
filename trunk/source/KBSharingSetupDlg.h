/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			KBSharingSetupDlg.h
/// \author			Bruce Waters
/// \date_created	15 January 2013
/// \rcs_id $Id: KBSharingSetupDlg.h 3028 2013-01-15 11:38:00Z bruce_waters@sil.org $
/// \copyright		2013 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the KBSharingSetupDlg class.
/// The KBSharing class provides a dialog for the turning on, or off, KB Sharing for the
/// currently open Adapt It project. It allows authentication, or removal of the
/// connection between the Adapt It project and the remote server with given url.
/// \derivation		The KBSharingSetupDlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

#ifndef KBSharingSetupDlg_h
#define KBSharingSetupDlg_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "KBSharingSetupDlg.h"
#endif

#if defined(_KBSERVER)

class KBSharingSetupDlg : public AIModalDialog
{
public:
	// constructor (this one defaults m_bStateless to FALSE)
	KBSharingSetupDlg(wxWindow* parent);

	virtual ~KBSharingSetupDlg(void); // destructor

	// A public boolean member to specify whether stateless (ie. opened by anyone
	// regardless of whether a project is open or whether it is one for kb sharing), or
	// not (default is not to be stateless). The stateless instantiation if for use by
	// the KB Sharing Manager - which needs to get a temporary instance of KbServer class
	// open (just the adapting one will do) in order to get access to it's methods and the
	// stateless storage - i.e. wxString stateless_username, etc.
	bool m_bStateless;

	// other methods

	// Next two are needed because the user can invoke this dialog on an existing setup
	// not realizing it is already running, and so we need to be able to check for that
	// and tell him all's well; and we also need to be able to check for when the user
	// wants to change servers on the fly
	wxString m_saveOldURLStr;
	wxString m_saveOldUsernameStr;
	wxString m_savePassword; // so it can be restored if the user Cancels
	bool	 m_saveSharingAdaptationsFlag;
	bool	 m_saveSharingGlossesFlag;

protected:
	void InitDialog(wxInitDialogEvent& WXUNUSED(event));
	void OnOK(wxCommandEvent& event);
	void OnCancel(wxCommandEvent& event);
	void OnButtonRemoveSetup(wxCommandEvent& WXUNUSED(event));
	void OnUpdateButtonRemoveSetup(wxUpdateUIEvent& event);

	CAdapt_ItApp* m_pApp;

	wxTextCtrl*	m_pURLCtrl;
	wxTextCtrl*	m_pUsernameCtrl;
	wxButton*   m_pRemoveSetupBtn; //ID_KB_SHARING_REMOVE_SETUP

	// Note: I've stored the to-be-typed-just-once kb server password in the CMainFrame
	// instance, and the dialog for getting the user to type it in is there too, and a
	// private member wxString, m_kbserverPassword which keeps the value for the session's
	// duration. When either the adapting or glossing KbServer instance needs the password
	// it can get it from there with the accessor: GetKBSvrPassword()

	DECLARE_EVENT_TABLE()
};

#endif

#endif /* KBSharingSetupDlg_h */
