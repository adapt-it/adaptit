/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			KBSharingStatelessSetupDlg.h
/// \author			Bruce Waters
/// \date_created	7 October 2013
/// \rcs_id $Id: KBSharingStatelessSetup.h 3028 2013-01-15 11:38:00Z bruce_waters@sil.org $
/// \copyright		2013 Bruce Waters, Bill Martin, Erik Brommers, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the KBSharingStatelessSetupDlg class.
/// The KBSharingStatelessSetupDlg class provides an authentication dialog for KB sharing, in a
/// context where no connection to any local KB is wanted, and the username can be anyone
/// listed for the kbserver with given url. Authentication is that kind of context - so it
/// just wants to use a "stateless" KbServer instance for the services it provides for the
/// authentication operation.
/// We provided a boolean param when instantiating, because when used to set up computer
/// owner's normal sharing of a local KB within a project to be shared, it needs to store
/// the authentication and url details (except for password of course) in the project
/// config file - so there are app variables for that; however, if used for the KB Sharing
/// Manager tabbed dialog gui, then anyone might be authenticating (we can't assume it's
/// the computer owner - might be his advisor for example) and so we have different
/// "stateless" variables to receive the authentication strings - so that the owner's
/// settings don't get messed up by the interloper's authentication for some legitimate
/// purpose - such as adding a KB or a username, etc.
/// A flag m_bStateless is always TRUE when this class is instantiated.
/// Note: This stateless instantiation creates a stateless instance of KbServer on the
/// heap, in the creator, and deletes it in the destructor. This instance of KbServer
/// supplies needed resources for the Manager GUI, but makes no assumptions about whether
/// or not any kb definitions are in place, and no assumptions about any projects being
/// previously made sharing ones, or not.
/// \derivation		The KBSharingStatelessSetupDlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

#ifndef KBSharingStatelessSetupDlg_h
#define KBSharingStatelessSetupDlg_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "KBSharingStatelessSetupDlg.h"
#endif

#if defined(_KBSERVER)

class KBSharingStatelessSetupDlg : public AIModalDialog
{
public:
	// constructor (this one defaults m_bStateless to TRUE)
	KBSharingStatelessSetupDlg(wxWindow* parent, bool bUserAuthenticating);

	virtual ~KBSharingStatelessSetupDlg(void); // destructor

	// A public boolean member to specify whether stateless (ie. opened by anyone
	// regardless of whether a project is open or whether it is one for kb sharing), or
	// not (default is not to be stateless). This stateless instantiation if for use by
	// the KB Sharing Manager - which needs to get a temporary instance of KbServer class
	// open (just the adapting one will do) in order to get access to it's methods and the
	// stateless storage - i.e. wxString stateless_username, etc. And also 3 strings for
	// storing url, username, and password when running stateless, so that the person
	// using the Manager dialog can be accessing any kbserver accessible to him and in which
	// he's a listed user, without impinging on any other setup resulting from the similar
	// class, KBSharingSetupDlg. 
	bool m_bStateless;
	wxString m_strStatelessUsername;
	wxString m_strStatelessURL;
	wxString m_strStatelessPassword;
	KbServer* m_pStatelessKbServer;
	bool m_bUserIsAuthenticating; // TRUE if computer owner is authenticating, FALSE if some other
								  // person known to the kbserver is doing so

	// other methods

	// Next two are needed because the user can invoke this dialog on an existing setup
	// not realizing it is already running, and so we need to be able to check for that
	// and tell him all's well; and we also need to be able to check for when the user
	// wants to change servers on the fly
	wxString m_saveOldURLStr;
	wxString m_saveOldUsernameStr;
	wxString m_savePassword; // so it can be restored if the user Cancels
	//bool	 m_saveIsONflag;

protected:
	void InitDialog(wxInitDialogEvent& WXUNUSED(event));
	void OnOK(wxCommandEvent& event);
	void OnCancel(wxCommandEvent& event);

	CAdapt_ItApp* m_pApp;

	wxTextCtrl*	m_pURLCtrl;
	wxTextCtrl*	m_pUsernameCtrl;
	wxStaticText* m_pUsernameLabel;
	wxStaticText* m_pUsernameMsgLabel;

	// Note: I've stored the to-be-typed-just-once kb server password in the CMainFrame
	// instance, and the dialog for getting the user to type it in is there too, and a
	// private member wxString, m_kbserverPassword which keeps the value for the session's
	// duration. When either the adapting or glossing KbServer instance needs the password
	// it can get it from there with the accessor: GetKBSvrPassword()

	DECLARE_EVENT_TABLE()
};

#endif

#endif /* KBSharingStatelessSetupDlg_h */
