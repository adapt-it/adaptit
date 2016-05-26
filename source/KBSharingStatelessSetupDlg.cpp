/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			KBSharingStatelessSetupDlg.cpp
/// \author			Bruce Waters
/// \date_created	7 October 2013
/// \rcs_id $Id: KBSharingStatelessSetupDlg.cpp 3028 2013-01-15 11:38:00Z bruce_waters@sil.org $
/// \copyright		2013 Bruce Waters, Bill Martin, Erik Brommers, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the KBSharingStatelessSetupDlg class.
/// The KBSharingStatelessSetupDlg class provides a dialog for turning on KB sharing, in a
/// context where no connection to any local KB is wanted, and the username can be anyone
/// listed for the KBserver with given url.
/// The dialog is not used in the legacy way, but rather as stateless, when
/// authenticating someone-or-other (an administrator typically) to use
/// the Knowledge Base Sharing Manager GUI - available from the Administrator menu, to
/// add,edit or remove users of the KBserver, and/or add or edit definitions for shared
/// KBs, etc. A flag m_bStateless is always TRUE when this class is instantiated.
/// Note: This stateless instantiation creates a stateless instance of KbServer on the
/// heap, in the creator, and deletes it in the destructor. This instance of KbServer
/// supplies needed resources for the Manager GUI, but makes no assumptions about whether
/// or not any kb definitions are in place, and no assumptions about any projects being
/// previously made sharing ones, or not.
/// That is, this 'stateless' instantiation is what is used for authenticating within the
/// AuthenticateCheckAndSetupKBSharing() function, which we use everywhere, ie. for user
/// authentication, and for KB Sharing Manager authentication. The latter can be done by
/// any username so long as it is a username the KBserver recognises and which has sufficient
/// privileges. KBSharingStatelessSetupDlg uses an instance of GetKbServer[0] (an 'adaptations'
/// one) and throws that instance away when authentication etc is done. It's the m_bStateless
/// being TRUE that causes this to happen. However, the creator requires m_bUserAuthenticating
/// be passed in, as TRUE or FALSE. FALSE is to be used when authenticating to the Manager.
/// A further use of m_bUserAuthenticating being FALSE is to force saving of temporary values for
/// certain parameters such as url, username, password and two other flags to be divorced from
/// the similar ones (on the app) for saving state for a user authentication, as described above.
/// \derivation		The KBSharingStatelessSetupDlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "KBSharingStatelessSetupDlg.h"
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
#include "helpers.h"
#include "KbServer.h"
#include "KBSharingStatelessSetupDlg.h"

// event handler table
BEGIN_EVENT_TABLE(KBSharingStatelessSetupDlg, AIModalDialog)
	EVT_INIT_DIALOG(KBSharingStatelessSetupDlg::InitDialog)
	EVT_BUTTON(wxID_OK, KBSharingStatelessSetupDlg::OnOK)
	EVT_BUTTON(wxID_CANCEL, KBSharingStatelessSetupDlg::OnCancel)
END_EVENT_TABLE()

// The stateless contructor (default, internally sets m_bStateless to TRUE)
KBSharingStatelessSetupDlg::KBSharingStatelessSetupDlg(wxWindow* parent, bool bUserAuthenticating) // dialog constructor
	: AIModalDialog(parent, -1, _("Authenticate"),
				wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{

	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	kb_sharing_stateless_setup_func(this, TRUE, TRUE);
	// The declaration is: functionname( wxWindow *parent, bool call_fit, bool set_sizer );
	m_pApp = &wxGetApp();
	bool bOK;
	bOK = m_pApp->ReverseOkCancelButtonsForMac(this);
	bOK = bOK; // avoid warning
	m_bStateless = TRUE;
	m_strStatelessUsername.Empty();
	m_strStatelessURL.Empty();
	m_strStatelessPassword.Empty();
	m_bUserIsAuthenticating = bUserAuthenticating;
#if defined(_DEBUG)
	// For my use, to save time getting authenticated to kbserver.jmarsden.org, I'll
	// initialize to me and his server here, for the debug build, & my personal pwd too
	//m_strStatelessUsername = _T("bruce_waters@sil.org");
	//m_strStatelessURL = _T("https://kbserver.jmarsden.org");
	// m_strStatelessPassword <- similarly set to my personal password in Mainfrm.cpp
	// line 2630, within GetKBSvrPasswordFromUser(wxString& url, wxString& hostname)
#endif
	// This contructor doesn't make any attempt to link to any CKB instance, or to support
	// only the hardware's current user; params are in the signature: whichType (we use 1, 
	// for an adapting type even though type is internally irrelevant for the stateless one),
	// and bool bStateless, which must be TRUE -- since KbServer class has the bool
	// m_bStateless member also
	// Make sure we are starting afresh...
	if (m_pApp->m_pKbServer_Occasional != NULL)
	{
		delete m_pApp->m_pKbServer_Occasional;
	}
	m_pApp->m_pKbServer_Occasional = new KbServer(1, TRUE);
	m_pStatelessKbServer = m_pApp->m_pKbServer_Occasional;
}

KBSharingStatelessSetupDlg::~KBSharingStatelessSetupDlg() // destructor
{
}

void KBSharingStatelessSetupDlg::InitDialog(wxInitDialogEvent& WXUNUSED(event))
{
	// If service discovery succeeded, a valid URL should now be in the app member
	// m_strKbServerURL. But other scenarios are possible -- see next comment...
	m_bError = FALSE;
    // In a new session, there may have been a URL stored in the app config file, and so it
    // is now available, but the user may want to type a different URL. It's also possible,
    // eg. first session for running a KBserver, that no URL has yet been stored in the
    // config file.
    // Don't show the top message box if the user is wanting service discovery done, or if
    // not wanting service discovery but there is no URL yet to show in the box
	m_saveOldURLStr = m_pApp->m_strKbServerURL; // save value locally in this class (value could be 
												// empty) Note, app has a member identically named
	/* deprecated, 18May16
	// BEW 18May16, I think the top message should always be there; and service discovery
	// has changed, so it shouldn't be a criterion for any logic here
	m_pMessageAtTop = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_URLMSG);
	m_pSizer = m_pTopSizer;
	if (m_pApp->m_bServiceDiscoveryWanted) 
	{
		// We can reenter the dialog if the password is empty or if there was a curl error
		// etc, so the second entry could have the message at top destroyed, so we'd crash
		// if we don't check for NULL here
		if (m_pMessageAtTop != NULL)
		{
			// m_pMessageAtTop->Destroy(); <- not helpful if program counter enters having
			// earlier deleted the msg box, changing to manual url type-in means the msg
			// box doesn't get seen. I'll try hiding and showing instead
			m_pMessageAtTop->Hide();
			m_pSizer->Layout();
		}
	}
	else
	{
		// Service discovery is not wanted. So show the message at the top, but if there is
		// no existing url available, don't bother - as the user will have to type one - and
		// obviously so
		if (!m_saveOldURLStr.IsEmpty())
		{
			if (m_pMessageAtTop != NULL)
			{
				m_pMessageAtTop->Show();
				m_pSizer->Layout();
			}
		}
		else
		{
			// There is no existing URL to show, and service discovery is not wanted. In
			// this circumstance the message is superfluous, so we'd want to ensure it
			// is hidden
			if (m_pMessageAtTop != NULL)
			{
				m_pMessageAtTop->Hide();
				m_pSizer->Layout();
			}
		}
	}
	*/

	m_pPasswordCtrl = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_KBSERVER_PWD);

	// This "Authenticate" dialog is used for both normal logins, but also when an
	// administrator grabs the machine to do some setup using the KB Sharing Manager. In
	// the latter circumstance, we don't want the normal user's setup parameters to be
	// clobbered, so we store the Manager-related settings elsewhere
	if (m_bUserIsAuthenticating)
	{
		m_saveOldUsernameStr = m_pApp->m_strUserID; // a first guess, we can override later
		m_savePassword = m_pApp->GetMainFrame()->GetKBSvrPassword(); // might be empty
	}
	else
	{
		m_saveOldUsernameStr.Empty();
		if (!m_pApp->m_strStatelessUsername.IsEmpty())
		{
			// If there was something from before, take it as a first guess
			m_strStatelessUsername = m_pApp->m_strStatelessUsername;
		}
		m_savePassword.Empty(); // force a password to be typed, if not a user authentication
	}
	// It's a project which has sharing turned on, if one or both of the adapting or
	// glossing KBs has been designated as for sharing
	//m_saveIsONflag = m_pApp->m_bIsKBServerProject || m_pApp->m_bIsGlossingKBServerProject;

	m_pURLCtrl = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_SERVER_URL_STATELESS);
	m_pUsernameCtrl = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_USERNAME_STATELESS);
	m_pUsernameLabel = (wxStaticText*)FindWindowById(ID_TEXT_USERNAME_LABEL_STATELESS);
	m_pUsernameMsgLabel = (wxStaticText*)FindWindowById(ID_TEXT_USERNAME_MSG_LABEL_STATELESS);

	wxString otherLabel = _("This is your current unique username");
	wxString managerLabel = _("For the Sharing Manager dialog, this username is temporary");
	if (m_bUserIsAuthenticating)
	{
		// Flag is TRUE, so user is authenticating and user is an owner of this KBserver
		// If a username string is available, claim it as the current unique one; if not,
		// hide this message line
		if (m_saveOldUsernameStr.IsEmpty())
		{
			m_pUsernameMsgLabel->Show(FALSE);
		}
		else
		{
			m_pUsernameMsgLabel->SetLabel(otherLabel);
		}
	}
	else
	{
		// Flag is FALSE, user is probably an administrator who is setting up on behalf of
		// this computer's owner; so don't claim the username belongs to the project owner
		m_pUsernameMsgLabel->SetLabel(managerLabel);
	}
    // BEW added 14Jul13, When m_bStateless is TRUE, we want anybody (e.g. the user's
	// administrator or project supervisor) to be able to grab his machine, open the
	// Knowledge Base Sharing Manager GUI and do things like add users, create kb
	// definitions, and so forth without having to impersonate the actual machine owner,
	// and without any project yet having become a KB Sharing one

	// If the app members have values for the url and username already (from having been
	// just set earlier, or from the project config file, then reinstate them so that if
	// the kb sharing was turned off it can be quickly re-enabled
	// When running for authentication to the KB Sharing Manager, don't try initialize
	// these boxes -- they start off empty. But for normal authentication, try to
	// initialize them, but allow the username one to be editable because someone with
	// valid credentials may be using the normal user's computer, and wants to log in
	m_pURLCtrl->ChangeValue(m_saveOldURLStr);
	if (m_bUserIsAuthenticating)
	{
		m_pUsernameCtrl->ChangeValue(m_saveOldUsernameStr);
		// Don't set it as selected, rather, we assume it is correct
#if defined(_DEBUG) && defined(AUTHENTICATE_AS_BRUCE) // see top of Adapt_It.h
		// Simplify my life during development
		m_pURLCtrl->ChangeValue(m_strStatelessURL);
		m_pUsernameCtrl->ChangeValue(m_strStatelessUsername);
#endif
		//m_pUsernameCtrl->SetEditable(FALSE); <- no, allow others to log in
	}
	else
	{
		wxString emptyStr = _T("");
		m_pUsernameCtrl->ChangeValue(m_strStatelessUsername);
		m_pUsernameCtrl->SetSelection(0L,-1L); // select it all, as it may be
			// incorrect so it's a good idea to make it instantly removable by
			// typing a character into it
#if defined(_DEBUG) && defined(AUTHENTICATE_AS_BRUCE) // see top of Adapt_It.h
		// Simplify my life during development
		m_pURLCtrl->ChangeValue(m_strStatelessURL);
		m_pUsernameCtrl->ChangeValue(m_strStatelessUsername);
#endif
	}
}

void KBSharingStatelessSetupDlg::OnOK(wxCommandEvent& myevent)
{
	wxUnusedVar(myevent);

	wxString strURL;
	strURL = m_pURLCtrl->GetValue();
	if (strURL.IsEmpty())
	{
		wxString msg = _("The URL text box at the top is empty, please type the address of the server.");
		wxString title = _("Type URL");
		wxMessageBox(msg, title, wxICON_EXCLAMATION | wxOK);
		this->Raise(); // make sure the Authenticate dialog will be at the top of the z-order
		return; // to the dialog window to have another try
	}
	// We now have something in the URL box -- it might not be a valid URL, but we'll try
	// it. We save values on the app, because the KbServer instance we are using is
	// m_pApp->m_pKbServer_Occasional, and the latter we destroy immediately on exit
	// from this Authentication dialog - so no use trying to store values in any of it's
	// members. Store the url locally, don't commit to it until all checks succeed
	m_strStatelessURL = strURL;

	wxString strUsername;
	strUsername = m_pUsernameCtrl->GetValue();	
	if (strUsername.IsEmpty())
	{
		// No username, so tell him what to do and let him retry
		wxString msg = _("The username text box is empty. Please type a valid username known to the KBserver.");
		wxString title = _("Type username");
		wxMessageBox(msg, title, wxICON_EXCLAMATION | wxOK);
		this->Raise(); // make sure the Authenticate dialog will be at the top of the z-order
		return; // to the active dialog window
	}
	
	// A username was present so set it. Whether or not m_bUserIsAuthenticating is TRUE,
	// put the username that was typed only in the app member m_strStatelessUsername.
	// We want to keep the app member m_strUserID locked to the AI project's user-defined
	// unique username that he types in for document ownership, xhtml export, etc; so
	// we must not override that member string. So when this function is called within
	// the AuthenticateCheckAndSetupKBSharing() function, the user's choice of username
	// as got from KBSharingStatelessSetupDlg should be sought from m_strStatelessUsername.
	// However, here we only store it in the local m_strStatelessUsername wxString member,
	// not on the app. If later checks prove valid, we'll commit to it and store it then
	// in the app member
	m_strStatelessUsername = strUsername;
#if defined(_DEBUG)
	wxLogDebug(_T("Authenticate Dlg (KBSharingSetupDlg.cpp) username: %s , stored locally so far in: m_strStatelessUsername"),
		m_strStatelessUsername.c_str());
#endif

	// Get the server password. Returns an empty string if nothing is typed, or if the
	// user Cancels from the dialog. m_bStateless being TRUE, the password will be the one
	// belonging to the administrator currently using the KB Sharing Manager GUI, not the
	// owner of the machine who will later do adapting work
	CMainFrame* pFrame = m_pApp->GetMainFrame();

	// Authentication has got the url and username, so hide the dialog before showing
	// the child dialog for typing in the password -- then, if control reenters the
	// dialog, it will reappear on the screen

	//this->Show(FALSE); <<-- with pwd field in the dialog, we don't need to hide it now
	wxString hostname = _("<unknown>"); // hostname is advisory only, but may not be known
										// add code here to make it something other than <unknown> 
										// if it proves possible to get a valid hostname here
	if (!m_pApp->m_strKbServerHostname.IsEmpty())
	{
		hostname = m_pApp->m_strKbServerHostname;
	}

	// Now get the password...
	// we do this from the wxWidgets password utility dialog <<-- deprecated 18May16, get from dlg itself now
	//wxString pwd = pFrame->GetKBSvrPasswordFromUser(m_strStatelessURL, hostname); // show the password dialog
	wxString pwd = m_pPasswordCtrl->GetValue();

	wxString msg_empty = _("The password was empty. Please try again.");
	wxString title_empty = _("No password");

	if (m_bUserIsAuthenticating)
	{
		if (!pwd.IsEmpty())
		{
			m_strStatelessPassword = pwd; // store it temporarily in this class's instance

#if defined(_DEBUG)
			wxLogDebug(_T("Authenticate (KBSharingSetupDlg.cpp) password typed in: %s  In User block."),
					pwd.c_str());
#endif
            // Since a password has now been typed, we can check if this username is listed
            // in the user table. If he isn't, he cannot authenticate and so is denied
            // access to the server and setup of sharing remains off until this is fixed

            // The following call will set up a temporary instance of the adapting KbServer
            // class instance in order to call it's LookupUser() member, to check that this
            // user has an entry in the entry table; and delete the temporary instance
            // before returning
			bool bUserIsValid = CheckForValidUsernameForKbServer(m_strStatelessURL, m_strStatelessUsername, pwd);
			if (!bUserIsValid)
			{
				// Username is unknown to the KBserver. Setup of sharing won't be turned
				// on until a valid username is supplied
				m_pApp->LogUserAction(_T("In OnOK() of KBSharingStatelessSetupDlg.cpp, password not recognised by KBserver"));
				wxString msg = _("Most likely your chosen KBserver is not yet running. Check.\nOr maybe the username ( %s ) is not in the list of users for this knowledge base server.\nOr the URL, or password, is not correct.\nPerhaps ask your server administrator to help you.\nYou can continue working, but KB sharing will be OFF.");
				msg = msg.Format(msg, m_strStatelessUsername.c_str());
				wxMessageBox(msg, _("Authentication: a check failed"), wxICON_WARNING | wxOK);
				//this->Show(TRUE); // make the dialog visible again, we aren't done with it yet
				this->Raise(); // make sure the Authenticate dialog will be at the top of the z-order
				m_bError = TRUE;
			}
		}
		else
		{
			// Password was empty. Tell user and return to the active dialog for a retry
			// of OnOK()
			wxMessageBox(msg_empty, title_empty, wxICON_WARNING | wxOK); // warn about the empty password (the
											// dialog has a Cancel button to allow bailing out from there)
			//this->Show(TRUE); // make the dialog visible again
			this->Raise(); // make sure the Authenticate dialog will be at the top of the z-order
			return; // to the dialog
		} // end of else block for test: if (!pwd.IsEmpty())

		// Passed the credentials test, so commit to this typed password, and to the username
		// Store it for KbServer[0] and / or KbServer[1] instance(s) to use
		pFrame->SetKBSvrPassword(pwd);
		//m_pApp->m_strStatelessUsername; // we never override m_pApp->m_strUserID with
									    // a username typed only for authentication
	}
	else
	{
		// Somebody, probably an administrator, is authenticating to the KB Sharing 
		// Manager tabbed dialog 
		m_pApp->m_bIsKBServerProject = FALSE; // Turn it back off, the Manager doesn't want
											  // to assume the user's settings are this or that
		if (!pwd.IsEmpty())
		{
			m_strStatelessPassword = pwd; // store it temporarily in this class's instance								  
#if defined(_DEBUG)
			wxLogDebug(_T("Authenticate (KBSharingSetupDlg.cpp) password typed in: %s  In Non-User block."),
						pwd.c_str());
#endif
            // Since a password has now been typed, we can check if this username is listed
            // in the user table. If he isn't, he cannot authenticate and so is denied
            // access to the KB Sharing Manager GUI

			// The following call will set up a temporary instance of the adapting KbServer in
			// order to call it's LookupUser() member, to check that this user has an entry in
			// the entry table; and delete the temporary instance before returning
			bool bUserIsValid = CheckForValidUsernameForKbServer(m_strStatelessURL, m_strStatelessUsername, pwd);
			if (!bUserIsValid)
			{
				// Access to the Manager GUI is denied to this user
				m_pApp->LogUserAction(_T("In OnOK() of KBSharingStatelessSetupDlg.cpp, username or password not recognised by KBserver"));
				wxString msg = _("Most likely your chosen KBserver is not yet running. Check.\nOr maybe the username ( %s ) is not in the list of users for this knowledge base server.\nOr the URL, or password, is not correct.\nPerhaps ask your server administrator to help you.\nYou can continue working, but KB sharing will be OFF");
				msg = msg.Format(msg, m_strStatelessUsername.c_str());
				wxMessageBox(msg, _("Authentication: a check failed"), wxICON_WARNING | wxOK);
				//this->Show(TRUE); // make the dialog visible again, we aren't done with it yet
				this->Raise(); // make sure the Authenticate dialog will be at the top of the z-order
				m_bError = TRUE;
			}
		} // end of TRUE block for test: if (!pwd.IsEmpty())
		else
		{
			// Password was empty. Tell user and return to the active dialog for a retry
			// of OnOK()
			wxMessageBox(msg_empty, title_empty, wxICON_WARNING | wxOK);
			//this->Show(TRUE); // make the dialog visible again, we aren't done with it yet
			this->Raise(); // make sure the Authenticate dialog will be at the top of the z-order
			return; // to the dialog
		} // end of else block for test: if (!pwd.IsEmpty())

		// Passed the credentials test, so commit to this typed password, and to the username
		// Store them for potential use for Authentication later on
		m_pApp->m_strStatelessPassword = m_strStatelessPassword; // not seen by config files
		m_pApp->m_strStatelessUsername = m_strStatelessUsername; // we never override
						// m_pApp->m_strUserID with a username typed only for authentication
	} // end of else block for test: if (m_bUserIsAuthenticating)

	// All's well, commit to the URL; I'll store the url to different app locations, but
	// presumably it's usually the same KBserver, so I'm playing safe here
	if (m_bUserIsAuthenticating)
	{
		// This app member is where a url stored in the project config file gets stored
		m_pApp->m_strKbServerURL = m_strStatelessURL; // put the url in the storage which
										   // is associated with the project config file
#if defined(_DEBUG)
		wxLogDebug(_T("Authenticate Dlg (KBSharingStatelessSetupDlg.cpp) URL: %s , stored in m_pApp->m_strKbSErverURL"),
			m_pApp->m_strStatelessURL.c_str());
#endif
	}
	else
	{
		// This app member is for use when, say, we authenticate when opening the KB
		// Sharing Manager 
		m_pApp->m_strStatelessURL = m_strStatelessURL;
#if defined(_DEBUG)
wxLogDebug(_T("Authenticate Dlg (KBSharingStatelessSetupDlg.cpp) URL: %s , stored in m_pApp->m_strStatelessURL"),
		   m_pApp->m_strStatelessURL.c_str());
#endif
	}

	// Delete the temporary KbServer instance we've been using, set its pointer to NULL
	delete m_pApp->m_pKbServer_Occasional; // makes our local copy, m_strStatelessKbServer, also invalid
	m_pApp->m_pKbServer_Occasional = NULL;

    // BEW 19Aug15, in Code::Blocks in ubuntu laptop, OK button click is returning
    // wxID_CANCEL rather than wxID_OK, so I'll try setting wxID_OK explicitly here using
    // the SetReturnCode() function
    //SetReturnCode(wxID_OK);

	myevent.Skip(); // the dialog will be exited now
}

void KBSharingStatelessSetupDlg::OnCancel(wxCommandEvent& myevent)
{
	delete m_pApp->m_pKbServer_Occasional; // makes our local copy, m_strStatelessKbServer, also invalid
	m_pApp->m_pKbServer_Occasional = NULL;

	// Cancelling from here should turn off sharing...
	// ReleaseKBServer(int) int = 1 or 2, does several things. It stops the download timer, deletes it
	// and sets its pointer to NULL; it also saves the last timestamp value to its on-disk file; it
	// then deletes the KbServer instance that was in use for supplying resources to the sharing code
	if (m_pApp->KbServerRunning(1))
	{
		m_pApp->ReleaseKBServer(1); // the adaptations one
	}
	if (m_pApp->KbServerRunning(2))
	{
		m_pApp->ReleaseKBServer(2); // the glossings one
	}

	m_pApp->m_bIsKBServerProject = FALSE;
	m_pApp->m_bIsGlossingKBServerProject = FALSE;
	myevent.Skip();
}

#endif
