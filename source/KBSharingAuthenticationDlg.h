/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			KBSharingAuthenticationDlg.h
/// \author			Bruce Waters
/// \date_created	7 October 2013
/// \rcs_id $Id: KBSharingStatelessSetup.h 3028 2013-01-15 11:38:00Z bruce_waters@sil.org $
/// \copyright		2013 Bruce Waters, Bill Martin, Erik Brommers, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the KBSharingAuthenticationDlg class.
/// The KBSharingAuthenticationDlg class provides an authentication dialog for KB sharing, in a
/// context where no connection to any local KB is wanted, and the username can be anyone
/// listed for the KBserver with given url. Authentication is that kind of context - so it
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
/// That is, this 'stateless' instantiation is what is used for authenticating within the
/// AuthenticateCheckAndSetupKBSharing() function, which we use everywhere, ie. for user
/// authentication, and for KB Sharing Manager authentication. The latter can be done by
/// any username so long as it is a username the KBserver recognises and which has sufficient
/// privileges. KBSharingAuthenticationDlg uses an instance of GetKbServer[0] (an 'adaptations'
/// one) and throws that instance away when authentication etc is done. It's the m_bStateless
/// being TRUE that causes this to happen. However, the creator requires m_bUserAuthenticating
/// be passed in, as TRUE or FALSE. FALSE is to be used when authenticating to the Manager.
/// A further use of m_bUserAuthenticating with value FALSE is to force saving of temporary values for
/// certain parameters such as url, username, password and two other flags to be divorced from
/// the similar parameters for saving state for a user authentication, as described above.
/// \derivation		The KBSharingAuthenticationDlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

#ifndef KBSharingAuthenticationDlg_h
#define KBSharingAuthenticationDlg_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "KBSharingAuthenticationDlg.h"
#endif

#if defined(_KBSERVER)

class KBSharingAuthenticationDlg : public AIModalDialog
{
public:
    // Constructor (this one defaults m_bStateless to TRUE unilaterally), and pass in FALSE
    // for m_bUserAuthenticating if the computer user is assumed NOT to be whoever is
    // authenticating (eg when using the KB Sharing Manager), otherwise pass in TRUE - in
    // which case the username, and password get remembered in the session, etc.
    // The constructor sets up a new KbServer instance on the heap, to which
    // app's m_pKbServer_Occasional points; it is used while the class exists,
    // and deleted when the class is destroyed, and m_pKbServer_Occasional set back
    // to NULL. This way, if the user changes project it won't matter (in authentication
    // we always check that the local KB's language codes are in the KBserver, so there
    // is a weak dependency present. We can't possibly authenticate for sharing if 
    // sharing of the local KB is impossible because there is no complying kb definition
    // in the MySql kb table of the KBserver
	KBSharingAuthenticationDlg(wxWindow* parent, bool bUserAuthenticating);

	virtual ~KBSharingAuthenticationDlg(void); // destructor

	// A public boolean member to specify whether stateless (ie. opened by anyone
	// regardless of whether a project is open or whether it is one for kb sharing), or
	// not (default is not to be stateless). This stateless instantiation if for use by
	// the KB Sharing Manager - which needs to get a temporary instance of KbServer class
	// open (just the adapting one will do) in order to get access to it's methods and the
	// stateless storage - i.e. wxString m_strStatelessUsername, etc. And also 3 strings for
	// storing url, username, and password when running stateless, so that the person
	// using the Manager dialog can be accessing any KBserver accessible to him and in which
	// he's a listed user, without impinging on any other setup resulting from the similar
	// class, KBSharingSetupDlg. 
	bool m_bStateless;
	bool m_bError;
	wxString m_strStatelessUsername;
	wxString m_strStatelessURL;
	wxString m_strStatelessPassword;
	KbServer* m_pStatelessKbServer;
	bool m_bUserIsAuthenticating; // TRUE if computer owner is authenticating, FALSE if some other
								  // person known to the KBserver is doing so
	wxTextCtrl* m_pMessageAtTop;
	wxSizer* m_pSizer;

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

	wxTextCtrl*	  m_pURLCtrl;
	wxTextCtrl*	  m_pUsernameCtrl;
	wxTextCtrl*	  m_pPasswordCtrl;
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

#endif /* KBSharingAuthenticationDlg_h */
