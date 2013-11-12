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
/// listed for the kbserver with given url.
/// The dialog is not used in the legacy way, but rather as stateless, when
/// authenticating someone-or-other (an administrator typically) to use
/// the Knowledge Base Sharing Manager GUI - available from the Administrator menu, to
/// add,edit or remove users of the kbserver, and/or add or edit definitions for shared
/// KBs, etc. A flag m_bStateless is always TRUE when this class is instantiated.
/// Note: This stateless instantiation creates a stateless instance of KbServer on the
/// heap, in the creator, and deletes it in the destructor. This instance of KbServer
/// supplies needed resources for the Manager GUI, but makes no assumptions about whether
/// or not any kb definitions are in place, and no assumptions about any projects being
/// previously made sharing ones, or not.
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
	// line 2630, within GetKBSvrPasswordFromUser()
#endif
	// This contructor doesn't make any attempt to link to any CKB instance, or to support
	// only the hardware's current user; params are in t whichType (we use 1, for an adapting
	// type even though type is internally irrelevant for the stateless one), 
	// and bool bStateless, which must be TRUE -- since KbServer class has the bool
	// m_bStateless member also
	m_pStatelessKbServer = new KbServer(1, TRUE); 
}

KBSharingStatelessSetupDlg::~KBSharingStatelessSetupDlg() // destructor
{
	// The stateless KbServer instance that was being used has already been freed in
	// either OnOK() or OnCancel() of the KBSharingMgrTabbedDlg instance, which was
	// destroyed before this present destructor gets called
}

void KBSharingStatelessSetupDlg::InitDialog(wxInitDialogEvent& WXUNUSED(event))
{
	if (m_bUserIsAuthenticating)
	{
	m_saveOldURLStr = m_pApp->m_strKbServerURL; // save existing value (could be empty)
	m_saveOldUsernameStr = m_pApp->m_strUserID; // ditto
	m_savePassword = m_pApp->GetMainFrame()->GetKBSvrPassword(); // might be empty
	}
	else
	{
		m_saveOldURLStr.Empty();
		m_saveOldUsernameStr.Empty();
		m_savePassword.Empty();
	}
	// It's a project which has sharing turned on, if one or both of the adapting or
	// glossing KBs has been designated as for sharing
	//m_saveIsONflag = m_pApp->m_bIsKBServerProject || m_pApp->m_bIsGlossingKBServerProject;

	m_pURLCtrl = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_SERVER_URL_STATELESS);
	m_pUsernameCtrl = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_USERNAME_STATELESS);
	m_pUsernameLabel = (wxStaticText*)FindWindowById(ID_TEXT_USERNAME_LABEL_STATELESS);

	wxString otherLabel = _("This is your unique username\n(If it is empty, Cancel then click the Edit menu item Change Username...)");
	if (m_bUserIsAuthenticating)
	{
		m_pUsernameLabel->SetLabel(otherLabel);
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
	// initialize them, and then set the username one to be read-only because a unique
	// username should be set in 6.4.3 and later version for this and other features on
	// launch, if it has not existed before - and then kept in the basic config file for
	// reuse thereafter
	if (m_bUserIsAuthenticating)
	{
		m_pURLCtrl->ChangeValue(m_saveOldURLStr);
		m_pUsernameCtrl->ChangeValue(m_saveOldUsernameStr);
#if defined(_DEBUG) && defined(AUTHENTICATE_AS_BRUCE) // see top of Adapt_It.h
		// Simplify my life during development
		m_pURLCtrl->ChangeValue(m_strStatelessURL);
		m_pUsernameCtrl->ChangeValue(m_strStatelessUsername);
#endif
		m_pUsernameCtrl->SetEditable(FALSE);
	}
	else
	{
		wxString emptyStr = _T("");
		m_pURLCtrl->ChangeValue(emptyStr);
		m_pUsernameCtrl->ChangeValue(emptyStr);

#if defined(_DEBUG) && defined(AUTHENTICATE_AS_BRUCE) // see top of Adapt_It.h
		// Simplify my life during development
		m_pURLCtrl->ChangeValue(m_strStatelessURL);
		m_pUsernameCtrl->ChangeValue(m_strStatelessUsername);
#endif
	}
}

void KBSharingStatelessSetupDlg::OnOK(wxCommandEvent& myevent)
{
	wxString msg_empty = _("The password was empty. Please try again.");
	wxString title_empty = _("No password");

	wxString strURL;
	strURL = m_pURLCtrl->GetValue();
	if (strURL.IsEmpty())
	{
		wxString msg = _("The URL text box at the top is empty, please type the address of the server.");
		wxString title = _("Type URL");
		wxMessageBox(msg, title, wxICON_EXCLAMATION | wxOK);
		return;
	}
	// We now have something in the URL box -- it might not be a valid URL, but we'll try
	// it; And since we are running stateless, then we don't store the URL in the app, but
	// only in a public member m_strStatelessURL, of this class, where (at the end of this
	// function) it is transferred to the stateless KbServer instance, and the KB Sharing
	// Manager's GUI will then get it too by a further transfer to it. But for normal
	// authenticating of the machine's user, we store in the app member variables which
	// the Project config file access
	if (m_bUserIsAuthenticating)
	{
		m_pApp->m_strKbServerURL = strURL; // put the url in the storage which is connected
										   // to the project config file
	}
	else
	{
	m_strStatelessURL = strURL;
	}
#if defined(_DEBUG)
		wxLogDebug(_T("KBSharingStatelessSetupDlg.cpp strURL = %s"), strURL.c_str());
#endif

    // If running for KB Sharing Mgr authentication, the username box is read-write
    // enabled, and should start off empty; but if authenticating the machine's normal
    // user, the username box should be read-only, and the url should be picked up from the
    // project config file if it exists from an earlier sesssion
	wxString strUsername;
	strUsername = m_pUsernameCtrl->GetValue();
	if (m_bUserIsAuthenticating)
	{
		// In this case the username box is read-only. If the box is empty, user can't
		// type into it - we have to tell him how to set the username in another place
		if (strUsername.IsEmpty())
		{
			wxString msg = _("The username text box is empty, and it is read-only, so you cannot type into it.\nCancel now, then go to the Edit menu and click Change Username.\nAfter you have setup your unique username there, retry setting up knowledge base sharing.");
			wxString title = _("Setup correct unique username");
		wxMessageBox(msg, title, wxICON_EXCLAMATION | wxOK);
			return; // return to the active dialog window, he can then type Cancel and 
					// follow the above instruction & retry after that
	}
	}
	else
	{
		if (strUsername.IsEmpty())
		{
			wxString msg = _("The username text box is empty.\nType now a unique username known to this server, then click OK again.");
			wxString title = _("Type correct unique username");
			wxMessageBox(msg, title, wxICON_EXCLAMATION | wxOK);
			return; // return to the active dialog window, he can then type it and try OK button again
		}
	}
	// A username was present so set it - it's already set for normal authenticating, so
	// we only need handle the username for when given in authenticating to the KB Sharing
	// Manager gui
	if (!m_bUserIsAuthenticating)
	{
	m_strStatelessUsername = strUsername;
	}
#if defined(_DEBUG)
		wxLogDebug(_T("KBSharingSetupDlg.cpp strUsername = %s"), strUsername.c_str());
#endif
		
	// Get the server password. Returns an empty string if nothing is typed, or if the
	// user Cancels from the dialog. m_bStateless being TRUE, the password will be the one
	// belonging to the administrator currently using the KB Sharing Manager GUI, not the
	// owner of the machine who will later do adapting work
	CMainFrame* pFrame = m_pApp->GetMainFrame();
	wxString pwd = pFrame->GetKBSvrPasswordFromUser(); // show the password dialog

	if (m_bUserIsAuthenticating)
	{
		if (!pwd.IsEmpty())
		{
			m_pApp->GetMainFrame()->SetKBSvrPassword(pwd); // KbServer instance(s) can now access it
#if defined(_DEBUG)
			wxLogDebug(_T("KBSharingSetupDlg.cpp pwd = %s"), pwd.c_str());
#endif
            // Since a password has now been typed, we can check if this username is listed
            // in the user table. If he isn't, he cannot authenticate and so is denied
            // access to the server and setup of sharing remains off until this is fixed

			// The following call will set up a temporary instance of the adapting KbServer in
			// order to call it's LookupUser() member, to check that this user has an entry in
			// the entry table; and delete the temporary instance before returning
			bool bUserIsValid = CheckForValidUsernameForKbServer(strURL, m_saveOldUsernameStr, pwd);
			if (!bUserIsValid)
			{
				// Username is unknown to the kbserver. Setup of sharing won't be turned
				// on until a valid username is supplied
				m_pApp->LogUserAction(_T("Trying to authenticate, but user unknown to server; in OnOK() of KBSharingStatelessSetupDlg.cpp"));
				wxString msg = _("The username ( %s ) is not in the list of users for this knowledge base server.\nYou are unable to turn on sharing until this is fixed.\nPerhaps ask your server administrator to help you. Click Cancel to continue working.\nYou can use the Edit menu item Change Username... to set a different one.");
				msg = msg.Format(msg, m_saveOldUsernameStr.c_str());
				wxMessageBox(msg, _("Unknown username"), wxICON_WARNING | wxOK);
				return; // to the dialog
			}
		}
		else
		{
			// Password was empty. Tell user and return to the active dialog for a retry
			// of OnOK()
			wxMessageBox(msg_empty, title_empty, wxICON_WARNING | wxOK);
			return; // to the dialog
		}
	}
	else
	{
		if (!pwd.IsEmpty())
		{
			m_strStatelessPassword = pwd; // store it publicly only in this class's instance
										  // so that the stateless KbServer instance can access
										  // it from the app's member of the same name
										  // when it does kbserver transmissions
#if defined(_DEBUG)
		wxLogDebug(_T("KBSharingSetupDlg.cpp pwd = %s"), pwd.c_str());
#endif
            // Since a password has now been typed, we can check if this username is listed
            // in the user table. If he isn't, he cannot authenticate and so is denied
            // access to the KB Sharing Manager GUI

			// The following call will set up a temporary instance of the adapting KbServer in
			// order to call it's LookupUser() member, to check that this user has an entry in
			// the entry table; and delete the temporary instance before returning
			bool bUserIsValid = CheckForValidUsernameForKbServer(m_strStatelessURL,
											m_strStatelessUsername, m_strStatelessPassword);
			if (!bUserIsValid)
			{
				// Access to the Manager GUI is denied to this user
				m_pApp->LogUserAction(_T("Stateless Kbserver user is unknown; in OnOK() of KBSharingStatelessSetupDlg.cpp"));
				wxString msg = _("The username ( %s ) is not in the list of users for this knowledge base server.\nYou are not permitted to access the Knowledge Base Sharing Manager dialog.\nAsk your kbserver administrator to do it for you. Click Cancel to continue working.");
				msg = msg.Format(msg, m_strStatelessUsername.c_str());
				wxMessageBox(msg, _("Unknown username"), wxICON_WARNING | wxOK);
				return;
			}
			else
			{
				// Once control reaches here, we have a valid URL, we have accessed the
				// server once to verify that the user specified by m_strStatelessUsername
				// is listed in that particular kbserver, we have an instance of KbServer
				// ready for use - and it knows it is stateless. So opening the manager
				// can now be done -- but first, get the URL and password and username
				// into the stateless KbServer instance we've created on the heap using
				// the creator function for the present class's instance
				wxASSERT(m_pStatelessKbServer->m_bStateless);
				m_pStatelessKbServer->SetKBServerPassword(m_strStatelessPassword);
				m_pStatelessKbServer->SetKBServerUsername(m_strStatelessUsername);
				m_pStatelessKbServer->SetKBServerURL(m_strStatelessURL);
			}
		}
		else
		{
			// Password was empty. Tell user and return to the active dialog for a retry
			// of OnOK()
			wxMessageBox(msg_empty, title_empty, wxICON_WARNING | wxOK);
			return; // to the dialog
		}
	} // end of TRUE block for test: if (m_bStateless)

	myevent.Skip(); // the dialog will be exited now
}

void KBSharingStatelessSetupDlg::OnCancel(wxCommandEvent& myevent)
{
	myevent.Skip();
}

#endif