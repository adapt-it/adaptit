/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			KBSharingSetupDlg.h
/// \author			Bruce Waters
/// \date_created	15 January 2013
/// \rcs_id $Id: KBSharingSetupDlg.h 3028 2013-01-15 11:38:00Z jmarsden6@gmail.com $
/// \copyright		2013 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the KBSharingSetupDlg class.
/// The KBSharingSetupDlg class provides two text boxes, the first for the KB server's URL,
/// the second for the username --the user's email address is recommended, but not
/// enforced. It's best because it will be unique. The KB server's password is not
/// entered in the dialog, otherwise it would be visible. Instead, a password dialog is
/// shown when the OK button is clicked. (The password dialog appears at other times too -
/// such as when turning ON KB sharing after it has been off, or when reentering a KB
/// sharing project previously designed as one for sharing of the KB.)
/// BEW added 14Jul13, The dialog can be used in the legacy way, non-stateless, when
/// authenticating by the machine owner to do more adapting work with a shared KB; or in a
/// stateless way, when authenticating someone-or-other (an administrator typically) to use
/// the Knowledge Base Sharing Manager GUI - available from the Administrator menu, to
/// add,edit or remove users of the kbserver, and/or add or edit definitions for shared
/// KBs. A flag m_bStateless tracks which instantiation we want, pass TRUE to instantiate
/// the creator for the stateless version.
/// Note: The stateless instantiation creates a stateless instance of KbServer on the
/// heap, in the creator, and deletes it in the destructor. This instance of KbServer
/// supplies needed resources for the Manager GUI, but makes no assumptions about whether
/// or not any kb definitions are in place, and no assumptions about any projects being
/// previously made sharing ones, or not.
/// \derivation		The KBSharingSetupDlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////
// Pending Implementation Items in KBSharingSetupDlg.cpp (in order of importance): (search for "TODO")
// 1.
//
// Unanswered questions: (search for "???")
// 1.
//
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "KBSharingSetupDlg.h"
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
//#include <wx/docview.h> // needed for classes that reference wxView or wxDocument
#include "Adapt_It.h"
#include "MainFrm.h"
#include "helpers.h"
#include "KbServer.h"
#include "KBSharingSetupDlg.h"

/// This global is defined in Adapt_It.cpp.
//extern CAdapt_ItApp* gpApp; // if we want to access it fast

// event handler table
BEGIN_EVENT_TABLE(KBSharingSetupDlg, AIModalDialog)
	EVT_INIT_DIALOG(KBSharingSetupDlg::InitDialog)
	EVT_BUTTON(wxID_OK, KBSharingSetupDlg::OnOK)
	EVT_BUTTON(wxID_CANCEL, KBSharingSetupDlg::OnCancel)
	EVT_BUTTON(ID_KB_SHARING_REMOVE_SETUP, KBSharingSetupDlg::OnButtonRemoveSetup)
	EVT_UPDATE_UI(ID_KB_SHARING_REMOVE_SETUP, KBSharingSetupDlg::OnUpdateButtonRemoveSetup)
	//EVT_BUTTON(ID_GET_ALL, KBSharingSetup::OnBtnGetAll)

END_EVENT_TABLE()

// The non-stateless contructor (default, internally sets m_bStateless to FALSE)
KBSharingSetupDlg::KBSharingSetupDlg(wxWindow* parent) // dialog constructor
	: AIModalDialog(parent, -1, _("Setup Or Remove Knowledge Base Sharing"),
				wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{

	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	kb_sharing_setup_func(this, TRUE, TRUE);
	// The declaration is: functionname( wxWindow *parent, bool call_fit, bool set_sizer );
	m_pApp = &wxGetApp();
	bool bOK;
	bOK = m_pApp->ReverseOkCancelButtonsForMac(this);
	bOK = bOK; // avoid warning
	m_bStateless = FALSE;
	m_strStatelessUsername.Empty();
	m_strStatelessURL.Empty();
	m_strStatelessPassword.Empty();
	m_pStatelessKbServer = NULL;
}

// The stateless contructor (default, set m_bStateless to TRUE) -- we use this in the support
// of the Knowledge Base Sharing Manager tabbed dialog, available from Administrator menu
KBSharingSetupDlg::KBSharingSetupDlg(wxWindow* parent, bool bStateless) // dialog constructor
	: AIModalDialog(parent, -1, _("Setup Or Remove Knowledge Base Sharing"),
				wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{

	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	kb_sharing_setup_func(this, TRUE, TRUE);
	// The declaration is: functionname( wxWindow *parent, bool call_fit, bool set_sizer );
	m_pApp = &wxGetApp();
	bool bOK;
	bOK = m_pApp->ReverseOkCancelButtonsForMac(this);
	bOK = bOK; // avoid warning
	m_bStateless = bStateless;
	m_strStatelessUsername.Empty();
	m_strStatelessURL.Empty();
	m_strStatelessPassword.Empty();
	// The 1 param is a hack, (it's int whichType) to enable the compiler to distinguish
	// this constructor apart from the KbServer(int whichType) one which it was wrongly
	// calling when I had this additional constructor as just KbServer(bool bStateless).
	// This contructor doesn't make any attempt to link to any CKB instance, or to support
	// only the hardware's current user
	m_pStatelessKbServer = new KbServer(1, bStateless); // we should pass only TRUE
}

KBSharingSetupDlg::~KBSharingSetupDlg() // destructor
{
	if (m_bStateless && m_pStatelessKbServer != NULL)
	{
		// Don't leak memory (the lists are in KbServer as members, m_usersList and
		// m_kbsList, and so don't need to be deleted once they are emptied - but their
		// contents, if nonempty, are structs stored on the heap)
		KbsList* pKbsList = m_pStatelessKbServer->GetKbsList();
		m_pStatelessKbServer->ClearKbsList(pKbsList);
		UsersList* pUsersList = m_pStatelessKbServer->GetUsersList();
		m_pStatelessKbServer->ClearUsersList(pUsersList);
		// the KB Sharing Manager won't have created any KbServerEntry structs, so no need
		// to clear any
		delete  m_pStatelessKbServer;
	}
}

void KBSharingSetupDlg::InitDialog(wxInitDialogEvent& WXUNUSED(event))
{
	m_saveOldURLStr = m_pApp->m_strKbServerURL; // save existing value (could be empty)
	m_saveOldUsernameStr = m_pApp->m_strUserID; // ditto

	m_savePassword = m_pApp->GetMainFrame()->GetKBSvrPassword(); // might be empty
	m_saveIsONflag = m_pApp->m_bIsKBServerProject;

	m_pURLCtrl = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_SERVER_URL);
	m_pRemoveSetupBtn = (wxButton*)FindWindowById(ID_KB_SHARING_REMOVE_SETUP);
	m_pUsernameCtrl = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_USERNAME);
    // BEW added 14Jul13, If instantiated non-stateless, then the username is taken from
    // the current owner, m_strUserID string, and so can't be changed so we make the control
	// read-only in that case; when m_bStateless is TRUE, we want anybody (e.g. the user's
	// administrator or project supervisor) to be able to grab his machine, open the
	// Knowledge Base Sharing Manager GUI and do things like add users, create kb
	// definitions, and so forth without having to impersonate the actual machine owner,
	// and without any project yet having become a KB Sharing one
	if (!m_bStateless)
	{
		m_pUsernameCtrl->SetEditable(FALSE); // it's read-only when we are not in KB Sharing Mgr handler
	}
	else
	{
		// When stateless, we don't need nor want to see the Remove Setup button as there
		// is no setup being done, just someone using the KB Sharing Manager
		m_pRemoveSetupBtn->Hide(); // same as Show(FALSE)

		// we also want to change the label over the username box
		wxStaticText* pUserTextLabel = (wxStaticText*)FindWindowById(ID_TEXT_USERNAME_LABEL);
		wxString newLabelStr = _("Type your username:");
		pUserTextLabel->SetLabel(newLabelStr);

		// and we want to hide the caution at the bottom about the Remove Setup button
		wxStaticText* pCautionTextLabel = (wxStaticText*)FindWindowById(ID_TEXT_PWD_CAUTION_LABEL);
		pCautionTextLabel->Hide();
	
	}
	// If the app members have values for the url and username already (from having been
	// just set earlier, or from the project config file, then reinstate them so that if
	// the kb sharing was turned off it can be quickly re-enabled
	// When running statelessly, don't try initialize these boxes, they start off empty
	if (m_bStateless)
	{
		wxString emptyStr = _T("");
		m_pURLCtrl->ChangeValue(emptyStr);
		m_pUsernameCtrl->ChangeValue(emptyStr);
	}
	else
	{
		if (!m_pApp->m_strKbServerURL.IsEmpty())
		{
			m_pURLCtrl->ChangeValue(m_pApp->m_strKbServerURL);
		}
		if (!m_pApp->m_strUserID.IsEmpty())
		{
			m_pUsernameCtrl->ChangeValue(m_pApp->m_strUserID);
		}
	}
}

void KBSharingSetupDlg::OnOK(wxCommandEvent& myevent)
{
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
	// it; but if we are running stateless, then we don't store the URL in the app, but
	// only in a public member m_strStatelessURL, of this class, where (after the dialog
	// is closed) it can be grabbed by the stateless KbServer instance, and the KB Sharing
	// Manager's GUI
	if (m_bStateless)
	{
		m_strStatelessURL = strURL;
	}
	else
	{
		m_pApp->m_strKbServerURL = strURL;
	}
#if defined(_DEBUG)
		wxLogDebug(_T("KBSharingSetupDlg.cpp m_strStatelessURL = %s"), m_strStatelessURL.c_str());
#endif

    // If running non-stateless, then what should appear in the username box is the
    // contents of m_strUserID which applies to both KB sharing, and to DVCS; and the box
    // should be read-only. If running stateless, the box is read-write enabled, and should
    // start off empty
	wxString strUsername;
	strUsername = m_pUsernameCtrl->GetValue();
	if (strUsername.IsEmpty() || (strUsername == _T("****")))
	{
		wxString msg = _("The username text box is empty or its contents invalid.\nPlease type the username that you were told to use.");
		wxString title = _("Type correct username");
		wxMessageBox(msg, title, wxICON_EXCLAMATION | wxOK);
		return;
	}
	if (m_bStateless)
	{
		m_strStatelessUsername = strUsername;
	}
#if defined(_DEBUG)
		wxLogDebug(_T("KBSharingSetupDlg.cpp m_strStatelessUsername = %s"), m_strStatelessUsername.c_str());
#endif


	// Get the server password. Returns an empty string if nothing is typed, or if the
	// user Cancels from the dialog. If m_bStateless is TRUE, the password will be the one
	// belonging to the administrator currently using the KB Sharing Manager GUI, not the
	// owner of the machine who will later do adapting work
	CMainFrame* pFrame = m_pApp->GetMainFrame();
	wxString pwd = pFrame->GetKBSvrPasswordFromUser();

	// Here we must provide two processing paths, one for when we are authenticating to
	// use the KB Sharing Manager, the other for normal work in a shared Adapt It project
	if (m_bStateless)
	{
		if (!pwd.IsEmpty())
		{
			m_strStatelessPassword = pwd; // store it publicly only in this class's instance
#if defined(_DEBUG)
		wxLogDebug(_T("KBSharingSetupDlg.cpp m_strStatelessPassword = %s"), m_strStatelessPassword.c_str());
#endif

            // Since a password has now been typed, we can check if this username is listed
            // in the user table. If he isn't, he cannot authenticate and so is denied
            // access to the KB Sharing Manager GUI

			// The following call will set up a temporary instance of the adapting KbServer in
			// order to call it's LookupUser() member, to check that this user has an entry in
			// the entry table; and delete the temporary instance before returning
/* TODO  fix this - it's failing, and so is jonathan's lookupuser c-client
			bool bUserIsValid = CheckForValidUsernameForKbServer(m_strStatelessURL,
											m_strStatelessUsername, m_strStatelessPassword);
*/

            bool bUserIsValid = TRUE; // this is temporary, see just above


			if (!bUserIsValid)
			{
				// Access to the Manager GUI is denied to this user
				m_pApp->LogUserAction(_T("Stateless Kbserver user is unknown; in OnOK() of KBSharingSetupDlg.cpp"));
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
	} // end of TRUE block for test: if (m_bStateless)
	else
	{
		// Normal work...

        // There will be a beep if no password was typed, or it the user cancelled; and an
        // empty string is returned if so
		if (!pwd.IsEmpty())
		{
			pFrame->SetKBSvrPassword(pwd); // store the password in CMainFrame's instance,
										   // ready for SetupForKBServer() below to use it
			// Control only gets this far provided there is a string in the URL box, and a
			// string in the username box. Since a password has now been typed, we can check
			// if this username is listed in the user table. If he isn't, send him back to the
			// dialog after giving him an instruction to either Cancel there, or hit the
			// Remove Setup button - either of which will land him out of the dialog with no
			// kb sharing turned on currently, at least, for this project.

			// The following call will set up a temporary instance of the adapting KbServer in
			// order to call it's LookupUser() member, to check that this user has an entry in
			// the entry table; and delete the temporary instance before returning
			bool bUserIsValid = CheckForValidUsernameForKbServer(m_pApp->m_strKbServerURL, m_pApp->m_strUserID, pwd);
			if (!bUserIsValid)
			{
				// Access is denied to this user, so turn off the setting
				// which says that this project is one for sharing, and tell
				// the user
				m_pApp->LogUserAction(_T("Kbserver user is invalid; in OnOK() in KBSharingSetupDlg.cpp"));
				m_pApp->ReleaseKBServer(1); // the adapting one, but should not yet be instantiated
				m_pApp->ReleaseKBServer(2); // the glossing one, but should not yet be instantiated
				m_pApp->m_bIsKBServerProject = FALSE;
				wxString msg = _("The username ( %s ) is not in the list of users for this knowledge base server.\nYou may continue working; but for you, knowledge base sharing is turned off.\nIf you need to share the knowledge base, ask your kbserver administrator to add your username to the server's list.\n Click Cancel, or click Remove Setup.\n(The username box is read-only. To change the username, use the Change Username item in the Edit menu.");
				msg = msg.Format(msg, m_pApp->m_strUserID.c_str());
				wxMessageBox(msg, _("Invalid username"), wxICON_WARNING | wxOK);
				return;
			}
			else
			{
				// What we do next will depend on what the current state is for KB Sharing within
				// this project. There are 3 possibilities.
				// 1. This server url and username are already in effect and this project is a kb
				// sharing one already - that is, there was no point in the user opening the
				// dialog unless he misspelled the password and needs to make a second attempt at it.
				// 2. This project is not a KB sharing one yet, or KB sharing was previously
				// defined for it, but whichever is the case, KB sharing is turned off (ie.
				// m_bIsKBServerProject is currently FALSE. This setup therefore turns it on with
				// the credentials supplied. (If the credentials have typos, error messages when
				// transmissions are done will indicate the problem. It would be good to have a
				// URL checker from somewhere if we can find one, too.)
				// 3. There was a different server connected to before the dialog was opened, and
				// the user wants to use a different KB server - in this case, detect that the
				// settings have changed, and if so, shut down the old setup and establish the new
				// one with the new credentials just typed.

				// BEW 20May13, don't let whatever is in the username box be able to update the
				// m_strUserID value; that variable must only be reset from an explicit call to
				// the UsernameInput dialog. We accomplish this by the text box being
				// read-only. To change the username, use the UsernameInput dialog and it can
				// be opened with the Edit menu's "Change Username" item, and the current
				// username reset there - that's the only place to do it (but a manual edit of
				// the basic config file would also work) strUsername; <<-- this is what
				// kbserver will use for authenticating in this session

				// Test that whatever is in the username text control matches m_strUserID. If it
				// doesn't, then don't go further. And tell the user to use Change Username dialog
				// to get the name right and then try setting up KB Sharing again
				if (strUsername != m_saveOldUsernameStr)
				{
					wxString msg = _("The username is incorrect.\n Click the Remove Setup button. Then open the Change Username dialog from the Edit menu.\nType the correct username there, also type an informal name in the second text box.\nThen try to set up knowledge base sharing again.");
					wxString title = _("Wrong username");
					wxMessageBox(msg, title, wxICON_EXCLAMATION | wxOK);
					return; // stay in the dialog so the user can do what the above message says
				}

				if (m_saveOldURLStr == strURL)
				{
					// The user has unchanged credentials for username and url, maybe he needs to
					// retype the password; so clobber the old setting and setup again with the
					// newer one
					if (m_pApp->m_bIsKBServerProject)
					{
						// there's an attempted share already defined for this project, so remove
						// it and re-establish it
						m_pApp->ReleaseKBServer(1); // the adaptations one
						m_pApp->ReleaseKBServer(2); // the glossings one
					}
					m_pApp->LogUserAction(_T("SetupForKBServer() re-attempted in setup dialog"));
					// instantiate an adapting and a glossing KbServer class instance
					if (!m_pApp->SetupForKBServer(1) || !m_pApp->SetupForKBServer(2))
					{
						// an error message will have been shown, so just log the failure
						m_pApp->LogUserAction(_T("SetupForKBServer() failed when maybe trying different password)"));
						m_pApp->m_bIsKBServerProject = FALSE; // no option but to turn it off
					}
				}
				else if(m_pApp->m_bIsKBServerProject)
				{
					// This project is a KB sharing one, and most likely the use of a different KB
					// server is wanted for this session, so remove the old settings and set up with
					// the new credentials
					m_pApp->ReleaseKBServer(1); // the adaptations one
					m_pApp->ReleaseKBServer(2); // the glossings one
					m_pApp->LogUserAction(_T("Switching the server: so calling SetupForKBServer() again"));
					// instantiate an adapting and a glossing KbServer class instance
					if (!m_pApp->SetupForKBServer(1) || !m_pApp->SetupForKBServer(2))
					{
						// an error message will have been shown, so just log the failure
						m_pApp->LogUserAction(_T("SetupForKBServer(): setup failed when trying to switch server"));
						m_pApp->m_bIsKBServerProject = FALSE; // no option but to turn it off
					}
				}
				else
				{
					// This project is not currently one designated for KB sharing, so use the
					// credentials supplied to turn KB sharing ON. The caller will have verified
					// the following:
					// The username (which needs to be identical to what's in
					// m_strUserID) has an entry in the mysql database's user table, and the
					// kb table has the needed languagecode pair for making this particular AI
					// project be one that supports KB Sharing
					m_pApp->LogUserAction(_T("Not currently sharing, authorized to call SetupForKBServer() now"));
					// instantiate an adapting and a glossing KbServer class instance
					if (!m_pApp->SetupForKBServer(1) || !m_pApp->SetupForKBServer(2))
					{
						// an error message will have been shown, so just log the failure
						m_pApp->LogUserAction(_T("SetupForKBServer(): authorized setup failed"));
						m_pApp->m_bIsKBServerProject = FALSE; // no option but to turn it off
					}
				}
				// ensure sharing starts off enabled; but if we've not succeeded in
				// instantiating, then jump these
				if (m_pApp->GetKbServer(1) != NULL)
				{
					// Success if control gets to this line (only needs to be set in one
					// place, since adapting and glossing setups are done together, or
					// destroyed together
					m_pApp->m_bIsKBServerProject = TRUE;
					m_pApp->GetKbServer(1)->EnableKBSharing(TRUE);
				}
				if (m_pApp->GetKbServer(2) != NULL)
				{
					m_pApp->GetKbServer(2)->EnableKBSharing(TRUE);
				}
			} // end of else block for test: if (!bUserIsValid)
		}
		else
		{
			// tell the user that a password is needed
			wxString msg = _("No password was typed. Setup is incomplete without a correct password.\nIf you do not know the password, click the Remove Setup button, then ask your administrator to help you setup.");
			wxString title = _("Type The Password");
			wxMessageBox(msg, title, wxICON_EXCLAMATION | wxOK);
			return; // stay in the dialog
		}
	} // end of else block for test: if (m_bStateless)

	myevent.Skip(); // the dialog will be exited now
}

void KBSharingSetupDlg::OnCancel(wxCommandEvent& myevent)
{
	if (!m_bStateless)
	{
		// Do nothing, change no settings; so restore what was saved
		m_pApp->m_strKbServerURL = m_saveOldURLStr;
		m_pApp->m_strUserID = m_saveOldUsernameStr;
		m_pApp->GetMainFrame()->SetKBSvrPassword(m_savePassword);
		m_pApp->m_bIsKBServerProject = m_saveIsONflag;
	}

	myevent.Skip();
}

void KBSharingSetupDlg::OnUpdateButtonRemoveSetup(wxUpdateUIEvent& event)
{
	if (!m_bStateless)
	{
		if (m_pApp->m_bIsKBServerProject)
		{
			event.Enable(TRUE);
		}
		else
		{
			event.Enable(FALSE);
		}
	}
	// If we are running stateless, the Remove Setup button is hidden
}

void KBSharingSetupDlg::OnButtonRemoveSetup(wxCommandEvent& WXUNUSED(event))
{
	if (!m_bStateless) // unnecessary, but harmless (the button is hidden if m_bStateless is TRUE)
	{
		m_pApp->m_bIsKBServerProject = FALSE;
		m_pApp->ReleaseKBServer(1); // the adaptations one
		m_pApp->ReleaseKBServer(2); // the glossings one

		// Removal of the sharing status for this Adapt It project may not be permanent, so
		// removal should not clear the server's url, nor the username for access. So the next
		// two lines are commented out. If we change this protocol, then uncomment them out.
		// For temporary removal & later re-enabling it is better to use the Disable Sharing /
		// Enable Sharing button though, because the password doesn't need to be retyped
		//m_pApp->m_strKbServerURL.Empty();
		//m_pApp->m_strUserID.Empty();

		// While we don't change the app members, the removal should clear the text boxes in
		// the dialog so that the user has feedback that the removal has been effected. To
		// reinstate the setup immediately he either must retype the information into the text
		// boxes, or click OK and then reopen the dialog (the text boxes will be filled with
		// the earlier values automatically) and click OK (and give the password of course)
		m_pURLCtrl->Clear();
		m_pUsernameCtrl->Clear();

		// make the dialog close (a good way to say, "it's been done";
		// also, we don't want a subsequent OK button click, because the user would then see a
		// message about the empty top editctrl, etc
		EndModal(wxID_OK);
	}
}

#endif
