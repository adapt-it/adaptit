/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			KBSharingSetup.h
/// \author			Bruce Waters
/// \date_created	15 January 2013
/// \rcs_id $Id: KBSharingSetup.h 3028 2013-01-15 11:38:00Z jmarsden6@gmail.com $
/// \copyright		2013 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the KBSharing class.
/// The KBSharing class provides a dialog for the turning on or off KB Sharing, and for
/// controlling the non-automatic functionalities within the KB sharing feature.
/// \derivation		The KBSharing class is derived from AIModalDialog.
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
	KBSharingSetupDlg(wxWindow* parent); // constructor
	virtual ~KBSharingSetupDlg(void); // destructor

	// other methods
	wxTextCtrl*	m_pURLCtrl;
	wxTextCtrl*	m_pUsernameCtrl;

	// Next two are needed because the user can invoke this dialog on an existing setup
	// not realizing it is already running, and so we need to be able to check for that
	// and tell him all's well; and we also need to be able to check for when the user
	// wants to change servers on the fly
	wxString m_saveOldURLStr;
	wxString m_saveOldUsernameStr;
	wxString m_savePassword; // so it can be restored if the user Cancels
	bool	 m_saveIsONflag;

protected:
	void InitDialog(wxInitDialogEvent& WXUNUSED(event));
	void OnOK(wxCommandEvent& event);
	void OnCancel(wxCommandEvent& event);
	void OnButtonRemoveSetup(wxCommandEvent& WXUNUSED(event));
	void OnUpdateButtonRemoveSetup(wxUpdateUIEvent& event);

	CAdapt_ItApp* m_pApp;
	

	// Note: I've stored the to-be-typed-just-once kb server password in the CMainFrame
	// instance, and the dialog for getting the user to type it in is there too, and a
	// private member wxString, m_kbserverPassword which keeps the value for the session's
	// duration. When either the adapting or glossing KbServer instance needs the password
	// it can get it from there with the accessor: GetKBSvrPassword()

	DECLARE_EVENT_TABLE()
};

#endif

#endif /* KBSharingSetupDlg_h */
